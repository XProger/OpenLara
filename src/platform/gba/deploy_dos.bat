rm *.obj
wcl386.exe *.cpp -fe=OpenLara.exe -i="C:\WATCOM/h" -wcd726 -w4 -e25 -zq -ox -d2 -6r -bt=dos -fo=.obj -zmf -xd -l=pmodew
C:\Dosbox\dosbox -conf dosbox.conf OpenLara.exe