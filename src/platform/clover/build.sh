set -e
clang++ -std=c++11 -Os -s -g -marm -march=armv7ve -mtune=cortex-a7 -mfloat-abi=hard -fno-exceptions -fno-rtti -ffunction-sections -fdata-sections -Wl,--gc-sections -Wno-invalid-source-encoding -DNDEBUG -D__CLOVER__ main.cpp ../../libs/stb_vorbis/stb_vorbis.c ../../libs/minimp3/minimp3.cpp ../../libs/tinf/tinflate.c -I/opt/vc/include -I../../ -L/opt/vc/lib/ -lGLESv2 -lEGL -lm -lrt -lpthread -lasound -ludev -o../../../bin/OpenLara
strip ../../../bin/OpenLara --strip-all --remove-section=.comment --remove-section=.note
