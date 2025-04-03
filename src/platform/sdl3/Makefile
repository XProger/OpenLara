SRCS=main.cpp \
     ../../libs/stb_vorbis/stb_vorbis.c \
     ../../libs/minimp3/minimp3.cpp \
     ../../libs/tinf/tinflate.c
CXX :=g++
DESTDIR :=
BINDIR :=/usr/bin

SDL_CFLAGS := $(shell pkg-config --cflags sdl3)
SDL_LDFLAGS := $(shell pkg-config --libs sdl3)

CFLAGS+=-std=c++11 -O2 -fno-exceptions -Wno-unused-variable -fno-rtti -ffunction-sections -fdata-sections -Wl,--gc-sections -D__SDL3__ $(SDL_CFLAGS) -Wall
LDFLAGS+=-lm -lrt -lpthread -lasound -ludev $(SDL_LDFLAGS)

INCLUDES+=-I../../

openlara : $(SRCS)
	$(CXX) $(INCLUDES) $(CFLAGS) $(LDFLAGS) $(SRCS) -o $@

install : openlara
	install -d $(DESTDIR)$(BINDIR)
	install openlara $(DESTDIR)$(BINDIR)/openlara

uninstall :
	rm -f $(DESTDIR)$(BINDIR)/openlara

clean:
	rm -f openlara
