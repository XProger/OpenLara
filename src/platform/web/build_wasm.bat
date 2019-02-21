@echo off
cls
set SRC=main.cpp ../../libs/stb_vorbis/stb_vorbis.c ../../libs/tinf/tinflate.c
set PROJ=OpenLara_wasm
set FLAGS=-s WASM=1 -O3 -ffast-math -Wno-deprecated-register --llvm-opts 2 -fmax-type-align=2 -std=c++11 -s USE_WEBGL2=1 -Wall -Wno-invalid-source-encoding -I../../
echo.
call em++ %SRC% %FLAGS% -o %PROJ%.js --preload-file ./level/1/TITLE.PSX --preload-file ./audio/1/dummy --preload-file ./audio/2/dummy --preload-file ./audio/3/dummy --preload-file ./level/2/dummy --preload-file ./level/3/dummy 
gzip.exe -9 -f %PROJ%.data %PROJ%.js %PROJ%.wasm %PROJ%.js.mem load-wasm-worker.js