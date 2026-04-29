#include <gtk/gtk.h>
#include <gdk/x11/gdkx.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

static GtkWidget *window = NULL;
static gboolean is_image = FALSE;

static void
fullscreen_clicked_cb(GtkWidget *button, gpointer unused)
{
	GtkWidget *widget_window = GTK_WIDGET(gtk_widget_get_root(button));
	gtk_window_fullscreen(GTK_WINDOW(widget_window));
}

static gboolean
toggle_fullscreen(GtkWidget *widget, GVariant *args, gpointer data)
{
	GdkSurface *surface;
	GdkToplevelState state;

	surface = gtk_native_get_surface(GTK_NATIVE(widget));
	state = gdk_toplevel_get_state(GDK_TOPLEVEL(surface));

	if (state & GDK_TOPLEVEL_STATE_FULLSCREEN)
		gtk_window_unfullscreen(GTK_WINDOW(widget));
	else
		gtk_window_fullscreen(GTK_WINDOW(widget));

	return TRUE;
}

static void
do_restart(GtkWidget *video)
{
	GtkMediaStream *stream = gtk_video_get_media_stream(GTK_VIDEO(video));

	if (!stream)
		return;

	GFile *file = gtk_media_file_get_file(GTK_MEDIA_FILE(stream));
	if (file) {
		g_object_ref(file);
		gtk_video_set_file(GTK_VIDEO(video), file);
		gtk_media_stream_play(
			gtk_video_get_media_stream(GTK_VIDEO(video)));
		g_object_unref(file);
	}
}

static void
on_ended_notify(GtkMediaStream *stream, GParamSpec *pspec, gpointer user_data)
{
}

static void
fix_volume_button_step(GtkWidget *widget)
{
	if (GTK_IS_SCALE_BUTTON(widget)) {
		GtkAdjustment *adj = gtk_scale_button_get_adjustment(
			GTK_SCALE_BUTTON(widget));
		if (adj) {
			gtk_adjustment_set_step_increment(adj, 0.1);
			gtk_adjustment_set_page_increment(adj, 0.1);
		}
		return;
	}
	GtkWidget *child = gtk_widget_get_first_child(widget);
	while (child) {
		fix_volume_button_step(child);
		child = gtk_widget_get_next_sibling(child);
	}
}

static void
on_media_stream_notify(GtkVideo *video, GParamSpec *pspec, gpointer user_data)
{
	GtkMediaStream *stream = gtk_video_get_media_stream(video);

	if (!stream)
		return;

	g_signal_connect(stream, "notify::ended",
		G_CALLBACK(on_ended_notify), NULL);

	fix_volume_button_step(GTK_WIDGET(video));
}

static GtkWidget *
find_video_picture(GtkWidget *widget)
{
	GtkWidget *child = gtk_widget_get_first_child(widget);
	while (child) {
		if (GTK_IS_PICTURE(child))
			return child;
		GtkWidget *found = find_video_picture(child);
		if (found)
			return found;
		child = gtk_widget_get_next_sibling(child);
	}
	return NULL;
}

static void
video_clicked_cb(GtkGestureClick *gesture, int n_press,
                 double x, double y, gpointer user_data)
{
	GtkWidget *video = GTK_WIDGET(user_data);
	GtkMediaStream *stream;

	if (is_image)
		return;

	stream = gtk_video_get_media_stream(GTK_VIDEO(video));
	if (!stream)
		return;

	if (gtk_media_stream_get_ended(stream)) {
		do_restart(video);
		return;
	}

	if (gtk_media_stream_get_playing(stream))
		gtk_media_stream_pause(stream);
	else
		gtk_media_stream_play(stream);
}

static void
load_file(GtkWidget *video, GFile *file)
{
	GFileInfo *info;
	const char *content_type;

	info = g_file_query_info(file,
		G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
		G_FILE_QUERY_INFO_NONE, NULL, NULL);

	content_type = info ? g_file_info_get_content_type(info) : NULL;
	is_image = content_type && g_str_has_prefix(content_type, "image/");

	if (is_image) {
		GtkWidget *overlay_picture = g_object_get_data(
			G_OBJECT(video), "overlay-picture");
		GtkWidget *image_overlay = g_object_get_data(
			G_OBJECT(video), "image-overlay");
		if (overlay_picture)
			gtk_picture_set_file(GTK_PICTURE(overlay_picture), file);
		if (image_overlay)
			gtk_widget_set_visible(image_overlay, TRUE);
	} else {
		GtkWidget *image_overlay = g_object_get_data(
			G_OBJECT(video), "image-overlay");
		if (image_overlay)
			gtk_widget_set_visible(image_overlay, FALSE);
		gtk_video_set_file(GTK_VIDEO(video), file);

		fix_volume_button_step(video);

		GtkWidget *pic = find_video_picture(video);
		if (pic) {
			GtkGesture *old = g_object_get_data(G_OBJECT(video),
				"click-gesture");
			if (old)
				gtk_widget_remove_controller(pic,
					GTK_EVENT_CONTROLLER(old));
			GtkGesture *gesture = gtk_gesture_click_new();
			g_signal_connect(gesture, "pressed",
				G_CALLBACK(video_clicked_cb), video);
			gtk_widget_add_controller(pic,
				GTK_EVENT_CONTROLLER(gesture));
			g_object_set_data(G_OBJECT(video),
				"click-gesture", gesture);
		}
	}

	if (info)
		g_object_unref(info);
}

static void
open_dialog_response_cb(GObject *source, GAsyncResult *result, void *user_data)
{
	GtkFileDialog *dialog = GTK_FILE_DIALOG(source);
	GtkWidget *video = user_data;
	GFile *file;

	file = gtk_file_dialog_open_finish(dialog, result, NULL);
	if (!file)
		return;

	load_file(video, file);
	g_object_unref(file);
}

static void
open_clicked_cb(GtkWidget *button, GtkWidget *video)
{
	GtkFileDialog *dialog;
	GtkFileFilter *filter;
	GListStore *filters;

	dialog = gtk_file_dialog_new();
	gtk_file_dialog_set_title(dialog, "Select a file");

	filters = g_list_store_new(GTK_TYPE_FILE_FILTER);

	filter = gtk_file_filter_new();
	gtk_file_filter_add_mime_type(filter, "video/*");
	gtk_file_filter_set_name(filter, "Videos");
	g_list_store_append(filters, filter);
	g_object_unref(filter);

	filter = gtk_file_filter_new();
	gtk_file_filter_add_mime_type(filter, "image/*");
	gtk_file_filter_set_name(filter, "Images");
	g_list_store_append(filters, filter);
	g_object_unref(filter);

	filter = gtk_file_filter_new();
	gtk_file_filter_add_pattern(filter, "*");
	gtk_file_filter_set_name(filter, "All Files");
	g_list_store_append(filters, filter);
	g_object_unref(filter);

	gtk_file_dialog_set_default_filter(dialog,
		g_list_model_get_item(G_LIST_MODEL(filters), 0));

	gtk_file_dialog_set_filters(dialog, G_LIST_MODEL(filters));
	g_object_unref(filters);

	gtk_file_dialog_open(dialog,
		GTK_WINDOW(gtk_widget_get_root(button)),
		NULL,
		open_dialog_response_cb, video);
}

static void
on_window_realize_icon(GtkWidget *widget, gpointer ud)
{
    /* Load PNG and set as _NET_WM_ICON using Xlib directly */
    GdkPixbuf *pb = gdk_pixbuf_new_from_file(
        "/usr/share/icons/mrrobotos/scalable/apps/mrplayer.png", NULL);
    if (!pb) return;

    int w = gdk_pixbuf_get_width(pb);
    int h = gdk_pixbuf_get_height(pb);
    guchar *pixels = gdk_pixbuf_get_pixels(pb);
    int channels = gdk_pixbuf_get_n_channels(pb);
    int rowstride = gdk_pixbuf_get_rowstride(pb);

    unsigned long *data = g_malloc((2 + w * h) * sizeof(unsigned long));
    data[0] = w;
    data[1] = h;
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            guchar *p = pixels + y * rowstride + x * channels;
            guchar r = p[0], g2 = p[1], b = p[2];
            guchar a = channels == 4 ? p[3] : 255;
            data[2 + y * w + x] = ((unsigned long)a << 24) |
                                   ((unsigned long)r << 16) |
                                   ((unsigned long)g2 << 8) |
                                   ((unsigned long)b);
        }
    }

    GdkSurface *surface = gtk_native_get_surface(GTK_NATIVE(widget));
    GdkDisplay *display = gdk_surface_get_display(surface);
    Display *xdisplay = gdk_x11_display_get_xdisplay(display);
    Window xwindow = gdk_x11_surface_get_xid(surface);
    Atom net_wm_icon = XInternAtom(xdisplay, "_NET_WM_ICON", False);
    XChangeProperty(xdisplay, xwindow, net_wm_icon,
        XA_CARDINAL, 32, PropModeReplace,
        (unsigned char*)data, 2 + w * h);
    XFlush(xdisplay);

    g_free(data);
    g_object_unref(pb);
}

GtkWidget *
do_video_player(GtkWidget *do_widget)
{
	GtkWidget *title;
	GtkWidget *video;
	GtkWidget *outer_overlay;
	GtkWidget *overlay_picture;
	GtkWidget *button;
	GtkWidget *fullscreen_button;
	GtkEventController *controller;
	GtkCssProvider *css;

	if (!window) {
		window = gtk_window_new();
		if (do_widget)
			gtk_window_set_display(GTK_WINDOW(window),
				gtk_widget_get_display(do_widget));
		gtk_window_set_title(GTK_WINDOW(window), "Mr.Player");
		gtk_window_set_default_size(GTK_WINDOW(window), 800, 500);
		g_signal_connect(window, "realize", G_CALLBACK(on_window_realize_icon), NULL);
		g_object_add_weak_pointer(G_OBJECT(window), (gpointer *)&window);

		css = gtk_css_provider_new();
		gtk_css_provider_load_from_string(css,
			"video .controls {"
			"  opacity: 1;"
			"}"
			"video .controls-container {"
			"  opacity: 1;"
			"}");
		gtk_style_context_add_provider_for_display(
			gdk_display_get_default(),
			GTK_STYLE_PROVIDER(css),
			GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		g_object_unref(css);

		video = gtk_video_new();
		gtk_video_set_autoplay(GTK_VIDEO(video), TRUE);
		gtk_video_set_graphics_offload(GTK_VIDEO(video),
			GTK_GRAPHICS_OFFLOAD_ENABLED);
		gtk_widget_set_vexpand(video, TRUE);
		gtk_widget_set_hexpand(video, TRUE);

		g_signal_connect(video, "notify::media-stream",
			G_CALLBACK(on_media_stream_notify), NULL);

		g_signal_connect_swapped(video, "realize",
			G_CALLBACK(fix_volume_button_step), video);

		outer_overlay = gtk_overlay_new();
		gtk_widget_set_vexpand(outer_overlay, TRUE);
		gtk_widget_set_hexpand(outer_overlay, TRUE);
		gtk_overlay_set_child(GTK_OVERLAY(outer_overlay), video);

		overlay_picture = gtk_picture_new();
		gtk_picture_set_content_fit(GTK_PICTURE(overlay_picture),
			GTK_CONTENT_FIT_CONTAIN);
		gtk_widget_set_vexpand(overlay_picture, TRUE);
		gtk_widget_set_hexpand(overlay_picture, TRUE);
		gtk_widget_set_visible(overlay_picture, FALSE);
		gtk_overlay_add_overlay(GTK_OVERLAY(outer_overlay),
			overlay_picture);

		g_object_set_data(G_OBJECT(video),
			"image-overlay",   overlay_picture);
		g_object_set_data(G_OBJECT(video),
			"overlay-picture", overlay_picture);

		gtk_window_set_child(GTK_WINDOW(window), outer_overlay);

		title = gtk_header_bar_new();
		gtk_window_set_titlebar(GTK_WINDOW(window), title);

		button = gtk_button_new_with_mnemonic("_Open");
		g_signal_connect(button, "clicked",
			G_CALLBACK(open_clicked_cb), video);
		gtk_header_bar_pack_start(GTK_HEADER_BAR(title), button);

		fullscreen_button = gtk_button_new_from_icon_name(
			"view-fullscreen-symbolic");
		g_signal_connect(fullscreen_button, "clicked",
			G_CALLBACK(fullscreen_clicked_cb), NULL);
		gtk_accessible_update_property(GTK_ACCESSIBLE(fullscreen_button),
			GTK_ACCESSIBLE_PROPERTY_LABEL, "Fullscreen", -1);
		gtk_header_bar_pack_end(GTK_HEADER_BAR(title), fullscreen_button);

		controller = gtk_shortcut_controller_new();
		gtk_shortcut_controller_set_scope(
			GTK_SHORTCUT_CONTROLLER(controller),
			GTK_SHORTCUT_SCOPE_GLOBAL);
		gtk_widget_add_controller(window, controller);
		gtk_shortcut_controller_add_shortcut(
			GTK_SHORTCUT_CONTROLLER(controller),
			gtk_shortcut_new(
				gtk_keyval_trigger_new(GDK_KEY_F11, 0),
				gtk_callback_action_new(toggle_fullscreen,
					NULL, NULL)));
	}

	if (!gtk_widget_get_visible(window))
		gtk_widget_set_visible(window, TRUE);
	else
		gtk_window_destroy(GTK_WINDOW(window));

	return window;
}

typedef struct {
	char *path;
} AppData;

static void
activate(GtkApplication *app, gpointer user_data)
{
	AppData *data = user_data;
	GtkWidget *w = do_video_player(NULL);
	gtk_window_set_application(GTK_WINDOW(w), app);

	if (data->path) {
		GtkWidget *overlay = gtk_window_get_child(GTK_WINDOW(w));
		GtkWidget *video   = gtk_overlay_get_child(GTK_OVERLAY(overlay));
		GFile *file = g_file_new_for_commandline_arg(data->path);
		load_file(video, file);
		g_object_unref(file);
	}
}

int
main(int argc, char *argv[])
{
	GtkApplication *app;
	AppData data = { NULL };
	int status;
	int app_argc = 1;

	if (argc >= 2)
		data.path = argv[1];

	app = gtk_application_new("org.gtk.mrplayer",
		G_APPLICATION_NON_UNIQUE);
	g_signal_connect(app, "activate", G_CALLBACK(activate), &data);
	/* pass only argv[0] to g_application_run so it never sees the
	 * filename and doesn't try to handle it as a GFile */
	status = g_application_run(G_APPLICATION(app), app_argc, argv);
	g_object_unref(app);
	return status;
}
