clang++ -std=c++11 -Os -s -fno-exceptions -fno-rtti -ffunction-sections -fdata-sections -Wl,--gc-sections -DNDEBUG main.cpp ../../libs/stb_vorbis/stb_vorbis.c -I../../ -o../../../bin/OpenLara -lX11 -lGL -lm -lpthread -lpulse-simple -lpulse
strip ../../../bin/OpenLara --strip-all --remove-section=.comment --remove-section=.note
