set -e
/opt/gcw0-toolchain/usr/bin/mipsel-gcw0-linux-uclibc-g++ -o OpenLara -D__GCW0__ -std=c++11 -Os -s -g0 -mips32r2 -fno-exceptions -fno-rtti -ffunction-sections -fdata-sections -Wl,--gc-sections -Wno-invalid-source-encoding main.cpp ../../libs/stb_vorbis/stb_vorbis.c ../../libs/minimp3/minimp3.cpp ../../libs/tinf/tinflate.c -I/opt/vc/include -I../../ -L/opt/vc/lib/ -lGLESv2 -lEGL -lm -lpthread -lasound
/opt/gcw0-toolchain/usr/bin/mipsel-gcw0-linux-uclibc-strip ../../../bin/OpenLara --strip-all --remove-section=.comment --remove-section=.note
