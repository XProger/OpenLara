set -e

# Use this compilation line to build SDL2/GLES version, GLES2 version.
g++ -DSDL2_GLES -D_GAPI_GLES2 -std=c++11 `sdl2-config --cflags` -O3 -fno-exceptions -fno-rtti -ffunction-sections -fdata-sections -Wl,--gc-sections -DNDEBUG -D__SDL2__ main.cpp ../../libs/stb_vorbis/stb_vorbis.c ../../libs/minimp3/minimp3.cpp ../../libs/tinf/tinflate.c -I../../ -o OpenLara `sdl2-config --libs` -lGLESv2 -lEGL -lm -lrt -lpthread -lasound -ludev

# Use this compilation line to build SDL2/GLES version, GLES3, which is an extension to GLES2 so we use -lGLESv2, too.
#g++ -DSDL2_GLES -std=c++11 `sdl2-config --cflags` -O3 -fno-exceptions -fno-rtti -ffunction-sections -fdata-sections -Wl,--gc-sections -DNDEBUG -D__SDL2__ main.cpp ../../libs/stb_vorbis/stb_vorbis.c ../../libs/minimp3/minimp3.cpp ../../libs/tinf/tinflate.c -I../../ -o OpenLara `sdl2-config --libs` -lGLESv2 -lEGL -lm -lrt -lpthread -lasound -ludev

# Use this compilation line to build SDL2/OpenGL version.
#g++ -std=c++11 `sdl2-config --cflags` -O3 -fno-exceptions -fno-rtti -ffunction-sections -fdata-sections -Wl,--gc-sections -DNDEBUG -D__SDL2__ -D_SDL2_OPENGL main.cpp ../../libs/stb_vorbis/stb_vorbis.c ../../libs/minimp3/minimp3.cpp ../../libs/tinf/tinflate.c -I../../ -o OpenLara `sdl2-config --libs` -lGL -lm -lrt -lpthread -lasound -ludev
