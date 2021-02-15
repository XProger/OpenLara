make
curl --ftp-method nocwd -T OpenLara.self ftp://192.168.1.59:1337/ux0:/app/OPENLARA1/eboot.bin
echo launch OPENLARA1 | ./nc.exe 192.168.1.59 1338
