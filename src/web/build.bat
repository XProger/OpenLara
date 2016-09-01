set SRC=main.cpp
call em++ %SRC% -O2 --llvm-opts 2 --closure 1 -std=c++11 -o OpenLara.js --preload-file ./LEVEL2_DEMO.PHD -I..\
gzip.exe -9 -f OpenLara.data OpenLara.js OpenLara.js.mem