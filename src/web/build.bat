set SRC=main.cpp
em++ %SRC% -O2 --llvm-opts 2 --closure 1 -std=c++11 -o OpenLara.js --preload-file ./LEVEL2_DEMO.PHD -I..\