set -e
g++ -std=c++11 -O0 -ggdb -fno-exceptions -fno-rtti -ffunction-sections -fdata-sections -Wl,--gc-sections -DNDEBUG -D__RPI__ main.cpp ../../libs/stb_vorbis/stb_vorbis.c ../../libs/minimp3/minimp3.cpp ../../libs/tinf/tinflate.c ./context.c -I../../ -I/usr/include/drm -o../../../bin/OpenLara -lm -lrt -lpthread -lasound -ludev -lSDL2 -lEGL -lGLESv2 -ldrm -lgbm
#strip ../../../bin/OpenLara --strip-all --remove-section=.comment --remove-section=.note
