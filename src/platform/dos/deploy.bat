rm *.obj *.err
copy ..\gba\render.iwram.cpp render.cpp /Y
wcl386.exe *.cpp ..\..\fixed\*.cpp -fe=OpenLara.exe -i="C:\WATCOM/h" -i="..\..\fixed" -wcd726 -w4 -e25 -zq -ox -d2 -6r -bt=dos -fo=.obj -zmf -xd -l=pmodew
C:\Dosbox\dosbox -conf dosbox.conf OpenLara.exe