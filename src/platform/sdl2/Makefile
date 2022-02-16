SRCS=main.cpp \
     ../../libs/stb_vorbis/stb_vorbis.c \
     ../../libs/minimp3/minimp3.cpp \
     ../../libs/tinf/tinflate.c
CC=g++
BIN=OpenLara

SDL_CFLAGS := $(shell sdl2-config --cflags)
SDL_LDFLAGS := $(shell sdl2-config --libs)

CFLAGS+=-DSDL2_GLES -D_GAPI_GLES2 -std=c++11 -O3 -fno-exceptions -fno-rtti -ffunction-sections -fdata-sections -Wl,--gc-sections -DNDEBUG -D__SDL2__ $(SDL_CFLAGS)
LDFLAGS+=-lGLESv2 -lEGL -lm -lrt -lpthread -lasound -ludev $(SDL_LDFLAGS)

INCLUDES+=-I../../

openlara : $(OBJS)
	$(CC) $(INCLUDES) $(CFLAGS) $(LDFLAGS) $(SRCS)

clean:
	@rm -f $(BIN) $(LIB)

