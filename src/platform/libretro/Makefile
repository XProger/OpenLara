ifeq ($(platform),)
	platform = unix
	ifeq ($(shell uname -a),)
		platform = win
	else ifneq ($(findstring MINGW,$(shell uname -a)),)
		platform = win
	else ifneq ($(findstring MSYS,$(shell uname -a)),)
		platform = win
	else ifneq ($(findstring Darwin,$(shell uname -a)),)
		platform = osx
		arch = intel
	ifeq ($(shell uname -p),powerpc)
		arch = ppc
	endif
	else ifneq ($(findstring win,$(shell uname -a)),)
		platform = win
	endif
   # cross compilation to windows from unix
   ifneq (,$(findstring mingw,$(CC)))
      platform = win
   endif
endif

# system platform
system_platform = unix
ifeq ($(shell uname -a),)
	EXE_EXT = .exe
	system_platform = win
else ifneq ($(findstring Darwin,$(shell uname -a)),)
	system_platform = osx
	arch = intel
	ifeq ($(shell uname -p),powerpc)
		arch = ppc
	endif
	else ifneq ($(findstring MINGW,$(shell uname -a)),)
	system_platform = win
endif

TARGET_NAME = openlara

CORE_DIR := .

ifeq ($(platform), unix)
   TARGET := $(TARGET_NAME)_libretro.so
   fpic := -fPIC
   SHARED := -shared -Wl,--version-script=link.T -Wl,--no-undefined
   GL_LIB := -lGL
   LIBS += -lpthread
   CFLAGS += -D_GNU_SOURCE
   CXXFLAGS += -D_GNU_SOURCE
else ifneq (,$(findstring osx,$(platform)))
   TARGET := $(TARGET_NAME)_libretro.dylib
   fpic := -fPIC
   SHARED := -dynamiclib
   GL_LIB := -framework OpenGL
   CXXFLAGS += -DOSX
   CFLAGS += -DOSX
	ifeq ($(arch),ppc)
		CXXFLAGS += -D__ppc__ -DOSX_PPC
		CFLAGS += -D__ppc__ -DOSX_PPC
	endif
	OSXVER = `sw_vers -productVersion | cut -d. -f 2`
	OSX_LT_MAVERICKS = `(( $(OSXVER) <= 9)) && echo "YES"`
	fpic += -mmacosx-version-min=10.1
else ifeq ($(platform), pi)
   TARGET := $(TARGET_NAME)_libretro.so
   fpic := -fPIC
   SHARED := -shared -Wl,--version-script=link.T -Wl,--no-undefined
   CXXFLAGS += -I/opt/vc/include -I/opt/vc/include/interface/vcos/pthreads -I/opt/vc/include/vmcs_host/linux
   CFLAGS += -I/opt/vc/include -I/opt/vc/include/interface/vcos/pthreads -I/opt/vc/include/vmcs_host/linux
   GLES := 1
   LIBS += -L/opt/vc/lib
else ifneq (,$(findstring ios,$(platform)))
   TARGET := $(TARGET_NAME)_libretro_ios.dylib
   fpic := -fPIC
   SHARED := -dynamiclib
GLES=1

ifeq ($(IOSSDK),)
   IOSSDK := $(shell xcodebuild -version -sdk iphoneos Path)
endif

   GL_LIB := -framework OpenGLES
   DEFINES := -DIOS
   CXXFLAGS += -DHAVE_OPENGLES -DHAVE_OPENGLES2 $(DEFINES)
   CFLAGS += -DHAVE_OPENGLES -DHAVE_OPENGLES2 $(DEFINES)
   CC = cc -arch armv7 -isysroot $(IOSSDK)
   CXX = c++ -arch armv7 -isysroot $(IOSSDK)
ifeq ($(platform),ios9)
	CC     += -miphoneos-version-min=8.0
	CXXFLAGS += -miphoneos-version-min=8.0
	CFLAGS += -miphoneos-version-min=8.0
else
	CC     += -miphoneos-version-min=5.0
	CXXFLAGS += -miphoneos-version-min=5.0
	CFLAGS += -miphoneos-version-min=5.0
endif
else ifneq (,$(findstring qnx,$(platform)))
   TARGET := $(TARGET_NAME)_libretro_qnx.so
   fpic := -fPIC
   SHARED := -shared -Wl,--version-script=link.T
   GL_LIB := -lGL

   CC = qcc -Vgcc_ntoarmv7le
   AR = qcc -Vgcc_ntoarmv7le
   GL_LIB := -lGLESv2
   GLES := 1
else ifneq (,$(findstring armv,$(platform)))
   CC ?= gcc
   TARGET := $(TARGET_NAME)_libretro.so
   fpic := -fPIC
   SHARED := -shared -Wl,--version-script=link.T -Wl,--no-undefined
   CXXFLAGS += -I.
   CFLAGS += -I.
   LIBS += -lpthread
ifneq (,$(findstring gles,$(platform)))
   GLES := 1
else
   GL_LIB := -lGL
endif
ifneq (,$(findstring cortexa8,$(platform)))
   CXXFLAGS += -marm -mcpu=cortex-a8
   CFLAGS += -marm -mcpu=cortex-a8
else ifneq (,$(findstring cortexa9,$(platform)))
   CXXFLAGS += -marm -mcpu=cortex-a9
   CFLAGS += -marm -mcpu=cortex-a9
endif
   CXXFLAGS += -marm
   CFLAGS += -marm
ifneq (,$(findstring neon,$(platform)))
   CXXFLAGS += -mfpu=neon
   CFLAGS += -mfpu=neon
endif
ifneq (,$(findstring softfloat,$(platform)))
   CXXFLAGS += -mfloat-abi=softfp
   CFLAGS += -mfloat-abi=softfp
else ifneq (,$(findstring hardfloat,$(platform)))
   CXXFLAGS += -mfloat-abi=hard
   CFLAGS += -mfloat-abi=hard
endif
   CXXFLAGS += -DARM
   CFLAGS += -DARM
# emscripten
else ifeq ($(platform), emscripten)
	TARGET := $(TARGET_NAME)_libretro_emscripten.bc
else
# windows
   CC ?= gcc
   TARGET := $(TARGET_NAME)_libretro.dll
   SHARED := -shared -static-libgcc -static-libstdc++ -Wl,--version-script=link.T -lwinmm -Wl,--no-undefined
   GL_LIB := -lopengl32
   CXXFLAGS += -I.. -DWIN32
   CFLAGS += -I.. -DWIN32
endif

# webOS
ifneq (,$(or $(findstring webos,$(CROSS_COMPILE)),$(findstring starfish,$(CROSS_COMPILE))))
   GL_LIB =
   GLES = 1
   CFLAGS += -DWEBOS
   CXXFLAGS += -DWEBOS
endif

ifeq ($(DEBUG), 1)
   CXXFLAGS += -O0 -g
   CFLAGS += -O0 -g
else
   CXXFLAGS += -O3
   CFLAGS += -O3
endif

ifeq ($(_DEBUG), 1)
   CXXFLAGS += -D_DEBUG
   ifeq ($(platform),win)
      LIBS += -lgdi32
   endif
   # TODO: Other platforms?
endif

ifneq (,$(findstring qnx,$(platform)))
	CFLAGS += -Wc,-std=c99
else
	CFLAGS += -std=c99
endif

include Makefile.common

OBJECTS := $(SOURCES_C:.c=.o) $(SOURCES_CXX:.cpp=.o)
CXXFLAGS += $(fpic)
CFLAGS += -Wall -pedantic $(fpic)

CXXFLAGS += -std=c++11
CFLAGS   += $(INCFLAGS) $(FLAGS)
CXXFLAGS += $(INCFLAGS) $(FLAGS)

ifeq ($(GLES), 1)
   CXXFLAGS += -DHAVE_OPENGLES
   CFLAGS += -DHAVE_OPENGLES
   CXXFLAGS += -D__LIBRETRO_GLES__
   CFLAGS += -D__LIBRETRO_GLES__
   ifeq ($(GLES31), 1)
      CXXFLAGS += -DHAVE_OPENGLES3 -DHAVE_OPENGLES_3_1 -D_GAPI_GLES
      CFLAGS += -DHAVE_OPENGLES3 -DHAVE_OPENGLES_3_1 -D_GAPI_GLES
   else ifeq ($(GLES3), 1)
      CXXFLAGS += -DHAVE_OPENGLES3 -D_GAPI_GLES
      CFLAGS += -DHAVE_OPENGLES3 -D_GAPI_GLES
   else
      CXXFLAGS += -DHAVE_OPENGLES2 -D_GAPI_GLES2
      CFLAGS += -DHAVE_OPENGLES2 -D_GAPI_GLES2
   endif
   LIBS += -lGLESv2 # Still link against GLESv2 when using GLES3 API, at least on desktop Linux.
   LIBS += -lEGL
else
   LIBS += $(GL_LIB)
endif

ifeq ($(CORE), 1)
   CXXFLAGS += -DCORE
   CFLAGS += -DCORE
endif

CXXFLAGS += -D__LIBRETRO__
CFLAGS += -D__LIBRETRO__

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(LDFLAGS) $(fpic) $(SHARED) $(INCLUDES) -o $@ $(OBJECTS) $(LIBS) -lm $(EXTRA_GL_LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(fpic) -c -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) $(fpic) -c -o $@ $<

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: clean

