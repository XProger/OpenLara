ifdef $(GENDEV)
ROOTDIR = $(GENDEV)
else
ROOTDIR = /opt/toolchains/sega
endif

TARGET = OpenLara
BUILD = build
SOURCES = . asm ../../fixed

LDSCRIPTSDIR = $(ROOTDIR)/ldscripts

SHPREFIX = $(ROOTDIR)/sh-elf/bin/sh-elf-
SHCC = $(SHPREFIX)gcc
SHXX = $(SHPREFIX)g++
SHAS = $(SHPREFIX)as
SHLD = $(SHPREFIX)ld
SHOBJC = $(SHPREFIX)objcopy

CC_VER := $(shell $(SHCC) -dumpversion)

MDPREFIX = $(ROOTDIR)/m68k-elf/bin/m68k-elf-
MDAS = $(MDPREFIX)as
MDLD = $(MDPREFIX)ld

LIBPATH = -L$(ROOTDIR)/sh-elf/lib -L$(ROOTDIR)/sh-elf/lib/gcc/sh-elf/$(CC_VER) -L$(ROOTDIR)/sh-elf/sh-elf/lib
INCPATH = -I. -I../../fixed -I$(ROOTDIR)/sh-elf/include -I$(ROOTDIR)/sh-elf/sh-elf/include

SHCCFLAGS = -D__32X__ -m2 -mb -O2 -Wall -g -c -fno-rtti -fno-exceptions -fno-asynchronous-unwind-tables -fomit-frame-pointer -ffast-math -Wno-class-memaccess
SHLDFLAGS = -T $(LDSCRIPTSDIR)/mars.ld -Wl,-Map=$(BUILD)/output.map -nostdlib
SHASFLAGS = --big

MDLDFLAGS = -T $(LDSCRIPTSDIR)/md.ld --oformat binary
MDASFLAGS = -m68000 --register-prefix-optional

DD = dd
RM = rm -f

FILES_AS = $(foreach dir, $(SOURCES), $(wildcard $(dir)/*.s))
FILES_CC = $(foreach dir, $(SOURCES), $(wildcard $(dir)/*.c))
FILES_XX = $(foreach dir, $(SOURCES), $(wildcard $(dir)/*.cpp))

LIBS = $(LIBPATH) -lm -lc -lgcc -lgcc-Os-4-200 -lnosys
OBJS =  $(BUILD)/sh2_crt0.o
OBJS += $(addprefix $(BUILD)/, $(notdir $(FILES_AS:%.s=%.o)))
OBJS += $(addprefix $(BUILD)/, $(notdir $(FILES_CC:%.c=%.o)))
OBJS += $(addprefix $(BUILD)/, $(notdir $(FILES_XX:%.cpp=%.o)))

.PHONY: dump clean

all: $(BUILD) m68k_crt0.bin m68k_crt1.bin $(TARGET).32x

dump:
	@[ -d dump ] || mkdir -p dump
	$(SHXX) $(SHCCFLAGS) $(INCPATH) -S -o dump/main.s main.cpp
	$(SHXX) $(SHCCFLAGS) $(INCPATH) -S -o dump/render.s render.cpp
	$(SHXX) $(SHCCFLAGS) $(INCPATH) -S -o dump/sound.s sound.cpp
	$(SHXX) $(SHCCFLAGS) $(INCPATH) -S -o dump/common.s ../../fixed/common.cpp

$(BUILD):
	@[ -d $@ ] || mkdir -p $@

$(TARGET).32x: $(TARGET).elf
	$(SHOBJC) -O binary $< temp.bin
	$(DD) if=temp.bin of=$@ bs=1024K conv=sync
	$(RM) -f temp.bin

$(TARGET).elf: $(OBJS)
	$(SHCC) $(SHLDFLAGS) $(OBJS) $(LIBS) -o $(TARGET).elf

%.bin: crt/%.s
	$(MDAS) $(MDASFLAGS) -o $(BUILD)/$@.o $<
	$(MDLD) $(MDLDFLAGS) -o $(BUILD)/$@ $(BUILD)/$@.o

$(BUILD)/%.o: crt/%.s
	$(SHAS) $(SHASFLAGS) $(INCPATH) -o $@ $<

$(BUILD)/%.o: asm/%.s
	$(SHAS) $(SHASFLAGS) $(INCPATH) -o $@ $<

$(BUILD)/%.o: %.c
	$(SHCC) $(SHCCFLAGS) $(INCPATH) -o $@ $<

$(BUILD)/%.o: %.cpp
	$(SHXX) $(SHCCFLAGS) $(INCPATH) -o $@ $<

$(BUILD)/%.o: ../../fixed/%.cpp
	$(SHXX) $(SHCCFLAGS) $(INCPATH) -o $@ $<

clean:
	$(RM) $(BUILD)/* *.32x *.elf
