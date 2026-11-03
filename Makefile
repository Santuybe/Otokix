TARGET = otokix
PLATFORM = my282
MINUI_VERSION = v20251127-1
UPSTREAM_REPO = https://github.com/shauninman/MinUI
UPSTREAM_VERSION = $(MINUI_VERSION)

OTOKIX_SOURCES = src/main.c src/log.c src/audio.c src/init.c src/render.c src/draw.c src/destroy.c src/pollevent.c src/music.c src/music_ui.c src/decode.c src/music_scanner.c src/metadata.c

CC = $(CROSS_COMPILE)gcc
STRIP = $(CROSS_COMPILE)strip

INCDIR = -I. -Iinclude/ -Iplatform/$(PLATFORM)/include/ -Iminui/workspace/all/common/ -Iminui/workspace/$(PLATFORM)/platform/
SOURCE = $(OTOKIX_SOURCES) minui/workspace/all/common/scaler.c minui/workspace/all/common/utils.c minui/workspace/all/common/api.c minui/workspace/$(PLATFORM)/platform/platform.c
FLAGS = -ldl -lmsettings -lmstick -lSDL2 -lSDL2_image -lSDL2_ttf -lpthread -lm -lz

CFLAGS = -fomit-frame-pointer $(INCDIR) -DPLATFORM='"$(PLATFORM)"' -DUSE_SDL2 -O3 -std=gnu99 -marm

all: $(TARGET)

$(TARGET): minui
	-[ -d minui/workspace/$(PLATFORM)/libmstick ] && cd minui/workspace/$(PLATFORM)/libmstick && make
	cd minui/workspace/$(PLATFORM)/libmsettings && make
	$(CC) $(CFLAGS) $(SOURCE) -o $(TARGET) $(FLAGS)
	$(STRIP) $(TARGET)

minui:
	[ -d minui ] || git clone --branch $(UPSTREAM_VERSION) $(UPSTREAM_REPO) minui

pak: $(TARGET)
	rm -rf "Otokix.pak"
	mkdir -p "Otokix.pak"
	cp $(TARGET) "Otokix.pak/otokix"
	cp config.json "Otokix.pak/config.json"
	cp -r fonts "Otokix.pak/fonts"
	echo '#!/bin/sh' > "Otokix.pak/launch.sh"
	echo 'cd "$$(dirname "$$0")"' >> "Otokix.pak/launch.sh"
	echo './otokix' >> "Otokix.pak/launch.sh"
	chmod +x "Otokix.pak/launch.sh"
	rm -f Otokix-my282.pak.zip
	zip -r Otokix-my282.pak.zip Otokix.pak

pak-debug: CFLAGS += -DDEBUG=1
pak-debug: pak
	mv Otokix-my282.pak.zip Otokix-my282-debug.pak.zip

clean:
	rm -rf $(TARGET) minui "Otokix.pak" *.zip

.PHONY: all minui pak pak-debug clean
