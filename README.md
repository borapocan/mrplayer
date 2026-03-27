# mrplayer

GTK4 video player and image viewer for Mr.RobotOS.

## Features

- Play video files with built-in timeline and controls
- View image files
- Click on video to pause/resume
- Click on ended video to restart
- Volume control with 10% steps
- Fullscreen support (button or F11)
- File filter: Videos, Images, All Files

## Dependencies

- GTK 4
- GStreamer (for video playback, pulled in via GTK4)

### Arch Linux

```
sudo pacman -S gtk4 gstreamer gst-plugins-base gst-plugins-good gst-plugins-bad gst-plugins-ugly
```

### Debian / Ubuntu

```
sudo apt install libgtk-4-dev gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly
```

### Fedora

```
sudo dnf install gtk4-devel gstreamer1-plugins-base gstreamer1-plugins-good gstreamer1-plugins-bad-free gstreamer1-plugins-ugly
```

## Build

```
make
```

## Install

```
sudo make install
```

Installs the binary to `/usr/local/bin/mrplayer` and the desktop entry to
`/usr/local/share/applications/mrplayer.desktop`.

## Uninstall

```
sudo make uninstall
```

## Usage

Launch from your application menu or run directly:

```
mrplayer
```

Then click **Open** in the header bar to select a file.

### Keyboard shortcuts

| Key | Action          |
|-----|-----------------|
| F11 | Toggle fullscreen |

### Mouse

| Action                  | Result                  |
|-------------------------|-------------------------|
| Click video area        | Pause / Resume          |
| Click ended video       | Restart                 |
| Volume +/- buttons      | ±10% volume             |
| Drag timeline           | Seek                    |
| Fullscreen button       | Enter fullscreen        |

## License

MIT
