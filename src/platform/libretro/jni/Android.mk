LOCAL_PATH := $(call my-dir)

CORE_DIR := $(LOCAL_PATH)/..

GLES  := 1
GLES3 := 1

include $(CORE_DIR)/Makefile.common

COREFLAGS := -Wall -ffast-math -DHAVE_OPENGLES -D__LIBRETRO__ $(INCFLAGS)

ifeq ($(GLES3),1)
   COREFLAGS += -DHAVE_OPENGLES3
   GLES_LIB := -lGLESv3
else ifeq ($(GLES),1)
   COREFLAGS += -DHAVE_OPENGLES2
   GLES_LIB := -lGLESv2
endif

include $(CLEAR_VARS)
LOCAL_MODULE    := retro
LOCAL_SRC_FILES := $(SOURCES_C) $(SOURCES_CXX)
LOCAL_CXXFLAGS  := -std=c++11 $(COREFLAGS)
LOCAL_CFLAGS    := $(COREFLAGS)
LOCAL_LDFLAGS   := -Wl,-version-script=$(CORE_DIR)/link.T
LOCAL_LDLIBS    := $(GLES_LIB)
include $(BUILD_SHARED_LIBRARY)
