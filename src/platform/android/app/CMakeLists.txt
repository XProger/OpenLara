cmake_minimum_required(VERSION 3.4.1)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DANDROID -std=c++11 -fno-rtti -fno-exceptions -fvisibility=hidden -Wall")

add_library( game SHARED
             src/main/cpp/main.cpp
             ../../../libs/stb_vorbis/stb_vorbis.c
             ../../../libs/minimp3/minimp3.cpp
             ../../../libs/tinf/tinflate.c
             )

include_directories(../../../)

target_link_libraries( game GLESv3 OpenSLES log )