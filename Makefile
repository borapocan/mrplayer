CC      = gcc
TARGET  = mrplayer
SRC     = mrplayer.c
PREFIX  = /usr

CFLAGS  = $(shell pkg-config --cflags gtk4)
LIBS    = $(shell pkg-config --libs gtk4)

.PHONY: all clean install uninstall

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LIBS)

install: $(TARGET)
	install -Dm755 $(TARGET) $(PREFIX)/bin/$(TARGET)
	install -Dm644 mrplayer.desktop $(PREFIX)/share/applications/mrplayer.desktop

uninstall:
	rm -f $(PREFIX)/bin/$(TARGET)
	rm -f $(PREFIX)/share/applications/mrplayer.desktop

clean:
	rm -f $(TARGET)
