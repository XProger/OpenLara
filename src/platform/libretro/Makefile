platform = unix
GLES := 1
# system platform
system_platform = unix

TARGET_NAME = openlara

CORE_DIR := .

TARGET := $(TARGET_NAME)_libretro.so
fpic := -fPIC
SHARED := -shared -Wl,--version-script=link.T -Wl,--no-undefined

GL_LIB := -lGLESv2
GLES := 1
GLES31 :=1
LIBS += -lpthread

CXXFLAGS += -I.
CFLAGS += -I.
CXXFLAGS += -I..
CFLAGS += -I..


CXXFLAGS += -DARM
CFLAGS += -DARM

CXXFLAGS += -I/opt/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu/aarch64-none-linux-gnu/include 
CFLAGS += -I/opt/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu/aarch64-none-linux-gnu/include
LIBS += --sysroot=/opt/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu/aarch64-none-linux-gnu

CC = /opt/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu/bin/aarch64-none-linux-gnu-gcc     
CXX = /opt/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu/bin/aarch64-none-linux-gnu-g++ 

ifeq ($(DEBUG), 1)
   CXXFLAGS += -O0 -g
   CFLAGS += -O0 -g
else
   CXXFLAGS += -Ofast -mcpu=cortex-a35 -ffast-math  -Wall -fomit-frame-pointer
   CFLAGS += -Ofast -mcpu=cortex-a35 -ffast-math  -Wall -fomit-frame-pointer
endif


CFLAGS += -std=c99

include Makefile.common

OBJECTS := $(SOURCES_C:.c=.o) $(SOURCES_CXX:.cpp=.o)
CXXFLAGS += $(fpic)
CFLAGS += -Wall -pedantic $(fpic)

CXXFLAGS += -std=c++11
CFLAGS   += $(INCFLAGS) $(FLAGS)
CXXFLAGS += $(INCFLAGS) $(FLAGS)

ifeq ($(GLES), 1)
   CXXFLAGS += -DHAVE_OPENGLES -DHAVE_OPENGLES2
   CFLAGS += -DHAVE_OPENGLES -DHAVE_OPENGLES2
   CXXFLAGS += -D__LIBRETRO_GLES__
   CFLAGS += -D__LIBRETRO_GLES__
   ifeq ($(GLES31), 1)
      CXXFLAGS += -DHAVE_OPENGLES3 -DHAVE_OPENGLES_3_1
      CFLAGS += -DHAVE_OPENGLES3 -DHAVE_OPENGLES_3_1
   else ifeq ($(GLES3), 1)
      CXXFLAGS += -DHAVE_OPENGLES3
      CFLAGS += -DHAVE_OPENGLES3
   endif
   LIBS += -lGLESv2 # Still link against GLESv2 when using GLES3 API, at least on desktop Linux.
#   LIBS += -lEGL
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

