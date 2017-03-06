¹NDK_TOOLCHAIN_VERSION := 4.8
NDK_TOOLCHAIN_VERSION := clang
APP_ABI := armeabi-v7a
#  Enable C++11. However, pthread, rtti and exceptions aren’t enabled 
APP_CPPFLAGS += -std=c++11
# Instruct to use the static GNU STL implementation
#APP_STL := gnustl_static
LOCAL_C_INCLUDES += ${ANDROID_NDK}/sources/cxx-stl/gnu-libstdc++/4.8/include