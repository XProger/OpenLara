SRCS=main.cpp ../../libs/stb_vorbis/stb_vorbis.c ../../libs/minimp3/minimp3.cpp ../../libs/tinf/tinflate.c
CC=g++
OBJS=OpenLara.o
BIN=OpenLara

CFLAGS+=-D__SDL2__
LDFLAGS+=-lGLESv2 -lEGL -lSDL2 -lpthread -lrt -lm

INCLUDES+=-I./ 

openlara : $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

%.o: %.c
	@rm -f $@ 
	$(CC) $(CFLAGS) $(INCLUDES) -g -c $< -o $@ -Wno-deprecated-declarations

%.o: %.cpp
	@rm -f $@ 
	$(CXX) $(CFLAGS) $(INCLUDES) -g -c $< -o $@ -Wno-deprecated-declarations

%.bin: $(OBJS)
	$(CC) -o $@ -Wl,--whole-archive $(OBJS) $(LDFLAGS) -Wl,--no-whole-archive -rdynamic

%.a: $(OBJS)
	$(AR) r $@ $^

clean:
	for i in $(OBJS); do (if test -e "$$i"; then ( rm $$i ); fi ); done
	@rm -f $(BIN) $(LIB)

