set -e
clang++ -std=c++11 -Os -s -g -fno-exceptions -fno-rtti -ffunction-sections -fdata-sections -Wl,--gc-sections -DNDEBUG -D__RPI__ main.cpp ../../libs/stb_vorbis/stb_vorbis.c -I/opt/vc/include -I../../ -o../../../bin/OpenLara.bin -L/opt/vc/lib/ -lbrcmGLESv2 -lbrcmEGL -lX11 -lm -lrt -lpthread -lpulse-simple -lpulse -lbcm_host -ludev
strip ../../../bin/OpenLara --strip-all --remove-section=.comment --remove-section=.note
