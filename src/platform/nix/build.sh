set -e
clang++ -std=c++11 -Os -s -fno-exceptions -fno-rtti -ffunction-sections -fdata-sections -Wl,--gc-sections -Wno-invalid-source-encoding -DNDEBUG -D_POSIX_THREADS main.cpp ../../libs/stb_vorbis/stb_vorbis.c ../../libs/minimp3/minimp3.cpp ../../libs/tinf/tinflate.c -I../../ -o../../../bin/OpenLara -lX11 -lGL -lm -lpthread -lpulse-simple -lpulse
strip ../../../bin/OpenLara --strip-all --remove-section=.comment --remove-section=.note
