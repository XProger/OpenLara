DEBUG = FALSE

GCC = nspire-gcc
AS  = nspire-as
GXX = nspire-g++
LD  = nspire-ld
GENZEHN = genzehn

GCCFLAGS = -marm -march=armv5te -mtune=arm926ej-s -std=c++11 -flto -ffast-math -fomit-frame-pointer -fno-exceptions -fno-rtti -ffunction-sections -fdata-sections -D__TNS__ -I../../fixed/
LDFLAGS = -Wl,--gc-sections -Wl,--as-needed -flto -Wno-alloc-size-larger-than
ZEHNFLAGS = --name "OpenLara"

ifeq ($(DEBUG),FALSE)
	GCCFLAGS += -Ofast
else
	GCCFLAGS += -O0 -g
endif

OBJS = $(patsubst %.c, %.o, $(shell find . -name \*.c))
OBJS += $(patsubst %.cpp, %.o, $(shell find . -name \*.cpp))
OBJS += $(patsubst %.s, %.o, $(shell find . -name \*.s))
EXE = OpenLara
DISTDIR = .
vpath %.tns $(DISTDIR)
vpath %.elf $(DISTDIR)

all: $(EXE).prg.tns

%.o: %.c
	$(GCC) $(GCCFLAGS) -c $<

%.o: %.cpp
	$(GXX) $(GCCFLAGS) -c $<
	
%.o: %.s
	$(AS) -c $<

$(EXE).elf: $(OBJS)
	cp ../gba/render.iwram.cpp render.cpp
	mkdir -p $(DISTDIR)
	$(LD) $^ -o $(DISTDIR)/$@ $(LDFLAGS)

$(EXE).tns: $(EXE).elf
	$(GENZEHN) --input $(DISTDIR)/$^ --output $(DISTDIR)/$@ $(ZEHNFLAGS)

$(EXE).prg.tns: $(EXE).tns
	make-prg $(DISTDIR)/$^ $(DISTDIR)/$@

clean:
	rm -f *.o $(DISTDIR)/$(EXE).tns $(DISTDIR)/$(EXE).elf $(DISTDIR)/$(EXE).prg.tns
