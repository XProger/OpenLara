LOCAL_PATH := $(call my-dir)
SRC := ${LOCAL_PATH}/../../../../

include $(CLEAR_VARS)

#-ffunction-sections -fdata-sections #-frtti #-fexceptions #

LOCAL_CFLAGS        := -DANDROID -fvisibility=hidden
LOCAL_LDFLAGS       := -Wl,--gc-sections
LOCAL_MODULE        := game
LOCAL_SRC_FILES     :=  main.cpp\
                        $(SRC)libs/stb_vorbis/stb_vorbis.c\
                        $(SRC)libs/minimp3/minimp3.cpp
LOCAL_C_INCLUDES    := $(LOCAL_PATH)/../../../
LOCAL_LDLIBS        := -lGLESv2 -llog

include $(BUILD_SHARED_LIBRARY)