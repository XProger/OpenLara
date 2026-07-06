APP_LABEL = OpenLara
APP_DESCRIPTION = OpenLara native build for Trimui Smart Pro

APP_LABEL_SLUG = $(shell echo "$(APP_LABEL)" | tr ' ' '_')
BUILD_DIR = build/$(APP_LABEL_SLUG)
OUT = $(BUILD_DIR)/$(APP_LABEL_SLUG)

TOOLCHAIN_DIR = /sdk/aarch64-linux-gnu-7.5.0-linaro
SYSROOT_DIR = /sdk/usr
SDL_DIR = /sdk/SDL2-2.26.1

# Compiler Setup
CXX = $(TOOLCHAIN_DIR)/bin/aarch64-linux-gnu-g++
CXXFLAGS = -I$(SDL_DIR)/include -I$(SYSROOT_DIR)/include -I.
LDFLAGS = -L$(SYSROOT_DIR)/lib -L$(SYSROOT_DIR)/lib/mali -Wl,-rpath-link=$(SYSROOT_DIR)/lib/mali
LIBS = -lSDL2 -lSDL2_ttf -lfreetype -lz -lbz2 -lGLESv2 -lEGL -lIMGegl -lsrv_um -lusc -lglslcompiler -lm

SDL_CFLAGS = -I$(SDL_DIR)/include -I$(SYSROOT_DIR)/include -I.
SDL_LDFLAGS = -L$(SYSROOT_DIR)/lib -L$(SYSROOT_DIR)/lib/mali -Wl,-rpath-link=$(SYSROOT_DIR)/lib/mali

SRCS=main.cpp \
     ../../libs/stb_vorbis/stb_vorbis.c \
     ../../libs/minimp3/minimp3.cpp \
     ../../libs/tinf/tinflate.c

CFLAGS+=-DSDL2_GLES -D_GAPI_GLES2 -std=c++11 -O3 -fno-exceptions -fno-rtti -ffunction-sections -fdata-sections -Wl,--gc-sections -D__SDL2__ $(SDL_CFLAGS) -Wall
LDFLAGS+=-lGLESv2 -lEGL -lm -lrt -lpthread -lasound -ludev $(SDL_LDFLAGS) $(LIBS)

INCLUDES+=-I../../

all: $(OUT) copy_resources

$(OUT): $(SRCS)
	mkdir -p $(BUILD_DIR)
	$(CXX) $(INCLUDES) $(CFLAGS) $(LDFLAGS) $(LIBS) $(SRCS) -o $(OUT) 

copy_resources:
	# Copy icon into folder
	cp icon.png $(BUILD_DIR)/icon.png

	# Create config.json
	echo '{' > $(BUILD_DIR)/config.json
	echo '  "label":"$(APP_LABEL)",' >> $(BUILD_DIR)/config.json
	echo '  "icon":"icon.png",' >> $(BUILD_DIR)/config.json
	echo '  "iconsel":"icon.png",' >> $(BUILD_DIR)/config.json
	echo '  "launch":"launch.sh",' >> $(BUILD_DIR)/config.json
	echo '  "description":"$(APP_DESCRIPTION)"' >> $(BUILD_DIR)/config.json
	echo '}' >> $(BUILD_DIR)/config.json

	# Create launch.sh
	echo '#!/bin/sh' > $(BUILD_DIR)/launch.sh
	echo 'cd $$(dirname "$$0")' >> $(BUILD_DIR)/launch.sh
	echo '' >> $(BUILD_DIR)/launch.sh
	echo 'export LD_LIBRARY_PATH=$$(dirname "$$0")/lib:$$LD_LIBRARY_PATH' >> $(BUILD_DIR)/launch.sh
	echo './$(APP_LABEL_SLUG) -d /mnt/SDCARD/Roms/OPENLARA' >> $(BUILD_DIR)/launch.sh
	chmod +x $(BUILD_DIR)/launch.sh

clean:
	rm -rf $(BUILD_DIR)

