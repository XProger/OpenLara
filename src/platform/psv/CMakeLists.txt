cmake_minimum_required(VERSION 2.8)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()


project(OpenLara)

include("${VITASDK}/share/vita.cmake" REQUIRED)

set(VITA_APP_NAME "OpenLara")
set(VITA_TITLEID  "OPENLARA1")
set(VITA_VERSION  "01.00")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O3 -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O3 -std=c++11")
set(VITA_MKSFOEX_FLAGS "${VITA_MKSFOEX_FLAGS} -d PARENTAL_LEVEL=1")

include_directories(
  ../..
)

link_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
)

add_executable(OpenLara
  main.cpp
  ../../libs/stb_vorbis/stb_vorbis.c
  ../../libs/tinf/tinflate.c
)

target_link_libraries(OpenLara
  SceLibKernel_stub
  ScePower_stub
  SceRtc_stub
  SceDisplay_stub
  SceGxm_stub
  SceCtrl_stub
  SceAudio_stub 
  SceTouch_stub
  SceAppUtil_stub
)


vita_create_self(OpenLara.self OpenLara)
vita_create_vpk(OpenLara.vpk ${VITA_TITLEID} OpenLara.self
  VERSION ${VITA_VERSION}
  NAME ${VITA_APP_NAME}
  FILE sce_sys/icon0.png sce_sys/icon0.png
  FILE sce_sys/livearea/contents/bg.png sce_sys/livearea/contents/bg.png
  FILE sce_sys/livearea/contents/startup.png sce_sys/livearea/contents/startup.png
  FILE sce_sys/livearea/contents/template.xml sce_sys/livearea/contents/template.xml
)
