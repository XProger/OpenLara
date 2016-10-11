@echo off
cls
set SRC=main.cpp
set PROJ=OpenLara
set FLAGS=-O3 -Wno-deprecated-register --llvm-opts 2 -std=c++11 -I../../
set PRELOAD=./LEVEL2_DEMO.PHD
echo.
call em++ %SRC% %FLAGS% -o %PROJ%.js --preload-file %PRELOAD%
gzip.exe -9 -f %PROJ%.data %PROJ%.js %PROJ%.js.mem