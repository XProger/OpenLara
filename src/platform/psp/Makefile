TARGET = OpenLara
OBJS   = main.o ../../libs/stb_vorbis/stb_vorbis.o

INCDIR   = ../../
CFLAGS   = -G0 -O3
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti -Wno-invalid-offsetof
ASFLAGS  = $(CFLAGS)

BUILD_PRX = 1

LIBDIR  =
LDFLAGS =
LIBS    = -lpspgum -lpspgu -lpspaudiolib -lpspaudio -lpsprtc -lpsppower -lpspmath -lm -lstdc++

COPY_DIR        = D:/Games/PSP/memstick/PSP/GAME/OL/
EXTRA_TARGETS   = EBOOT.PBP
PSP_EBOOT_TITLE = OpenLara
PSP_EBOOT_ICON  = ICON0.PNG
PSP_EBOOT_PIC1  = PIC1.PNG

PSPSDK = $(shell psp-config --pspsdk-path)
#USE_PSPSDK_LIBC=1
include $(PSPSDK)/lib/build.mak