set -e
#g++ -std=c++11 -O3 -fno-exceptions -fno-rtti -ffunction-sections -fdata-sections -Wl,--gc-sections -DNDEBUG -D__SDL2__ main.cpp ../../libs/stb_vorbis/stb_vorbis.c ../../libs/minimp3/minimp3.cpp ../../libs/tinf/tinflate.c -I../../ -o OpenLara -lSDL2 -lGLESv2 -lEGL -lm -lrt -lpthread -lasound -ludev
g++ -std=c++11 `sdl2-config --cflags` -O3 -fno-exceptions -fno-rtti -ffunction-sections -fdata-sections -Wl,--gc-sections -DNDEBUG -D__SDL2__ main.cpp ../../libs/stb_vorbis/stb_vorbis.c ../../libs/minimp3/minimp3.cpp ../../libs/tinf/tinflate.c -I../../ -o OpenLara `sdl2-config --libs` -lGLESv2 -lEGL -lm -lrt -lpthread -lasound -ludev
#strip ../../../bin/OpenLara --strip-all --remove-section=.comment --remove-section=.note
