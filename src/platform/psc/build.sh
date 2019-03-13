#!/bin/bash
# This should really be a makefile with individual platforms but for now...
set -e
# General Info
GIT_COMMIT=$(echo "`git rev-parse --short HEAD``git diff-index --quiet HEAD -- || echo '-dirty'`")

# Build Rules
[ -z "CXX" ] && CXX="arm-linux-gnueabihf-g++-8"
CFLAGS="-std=c++11 -Os -s -g -marm -march=armv8-a -mtune=cortex-a35 -mfpu=neon-fp-armv8 -mfloat-abi=hard -fno-exceptions -fno-rtti -ffunction-sections -fdata-sections"
LDFLAGS="-Wl,--gc-sections -static-libgcc -static-libstdc++"
DEFINES="-DNDEBUG -D__PSC__"
SRC="main.cpp ../../libs/stb_vorbis/stb_vorbis.c ../../libs/minimp3/minimp3.cpp ../../libs/tinf/tinflate.c"
INCLUDES="-I/usr/arm-linux-gnueabihf/include/ -I../../"
LIBS="-L$LD_LIBRARY_PATH -lGLESv2 -lEGL -lm -lrt -lpthread -lasound -ludev -lwayland-client -lwayland-egl"
TARGET="-o../../../bin/OpenLara"

clear
echo "-------------------------------------------------------------------------------"
echo "OpenLara Builder v0.1 - OpenLara Build $GIT_COMMIT"
echo "-------------------------------------------------------------------------------"
echo "Build Rules:"
echo "CFLAGS:   $CFLAGS"
echo "LDFLAGS:  $LDFLAGS"
echo "DEFINES:  $DEFINES"
echo "SRC:      $SRC"
echo "INCLUDES: $INCLUDES"
echo "LIBS:     $LIBS"
echo "TARGET:   $TARGET"
echo "-------------------------------------------------------------------------------"
echo

$CXX $CFLAGS $LDFLAGS $DEFINES $SRC $INCLUDES $LIBS $TARGET