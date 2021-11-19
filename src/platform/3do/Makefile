SDK        = C:/Projects/3do-devkit
FILESYSTEM = CD
EXENAME	   = $(FILESYSTEM)/LaunchMe
ISONAME    = OpenLara.iso
STACKSIZE  = 4096
BANNER	   = banner.bmp

CC	   = armcc
CXX	   = armcpp
AS 	   = armasm
LD	   = armlink
RM	   = rm
MODBIN     = modbin
MAKEBANNER = MakeBanner
3DOISO     = 3doiso
3DOENCRYPT = 3DOEncrypt

OPT      = -O2
CFLAGS   = $(OPT) -bi -za1 -zas1 -wn -ff -fa -d __3DO__=1 -cpu ARM6
CXXFLAGS = $(CFLAGS)
ASFLAGS	 = -BI -i $(SDK)/include/3do
INCPATH	 = -I $(SDK)/include/3do -I $(SDK)/include/ttl -I ../../fixed
LIBPATH	 = $(SDK)/lib/3do
LDFLAGS	 = -aif -reloc -ro-base 0 -libpath $(LIBPATH) -nodebug -remove -info Sizes
STARTUP	 = $(LIBPATH)/cstartup.o

LIBS = \
	$(LIBPATH)/clib.lib			\
	$(LIBPATH)/cpluslib.lib		\
	$(LIBPATH)/swi.lib			\
	$(LIBPATH)/lib3do.lib		\
	$(LIBPATH)/operamath.lib	\
	$(LIBPATH)/audio.lib		\
	$(LIBPATH)/music.lib		\
	$(LIBPATH)/filesystem.lib	\
	$(LIBPATH)/graphics.lib		\
	$(LIBPATH)/input.lib		\

SRC_S   = $(wildcard *.s)
SRC_C   = $(wildcard *.c)
SRC_CXX = $(wildcard *.cpp)

OBJ += $(SRC_S:%.s=build/%.s.o)
OBJ += $(SRC_C:%.c=build/%.c.o)
OBJ += $(SRC_CXX:%.cpp=build/%.cpp.o)
OBJ += build/common.cpp.o

all: clean launchme modbin banner iso run

launchme: builddir $(OBJ)
	$(LD) -dupok -o $(EXENAME) $(LDFLAGS) $(STARTUP) $(LIBS) $(OBJ)

modbin:
	$(MODBIN) --stack=$(STACKSIZE) $(EXENAME) $(EXENAME)

banner:
	$(MAKEBANNER) $(BANNER) $(FILESYSTEM)/BannerScreen

iso:
	$(3DOISO) -in $(FILESYSTEM) -out $(ISONAME)
	$(3DOENCRYPT) genromtags $(ISONAME)

run:
	/c/RetroArch/retroarch.exe -L C:\RetroArch\cores\opera_libretro.dll C:\Projects\OpenLara\src\platform\3do\OpenLara.iso

builddir:
	mkdir -p build/

build/%.s.o: %.s
	$(AS) $(INCPATH) $(ASFLAGS) $< -o $@

build/%.c.o: %.c
	$(CC) $(INCPATH) $(CFLAGS) -c $< -o $@

build/%.cpp.o: %.cpp
	$(CXX) $(INCPATH) $(CXXFLAGS) -c $< -o $@
    
build/common.cpp.o: ../../fixed/common.cpp
	$(CXX) $(INCPATH) $(CXXFLAGS) -c ../../fixed/common.cpp -o build/common.cpp.o
    
clean:
	$(RM) -vf $(OBJ) $(EXENAME) $(EXENAME).sym $(ISONAME)

.PHONY: clean modbin banner iso
