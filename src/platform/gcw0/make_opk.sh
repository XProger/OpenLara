#!/bin/sh

OPK_NAME=OpenLara.opk

echo ${OPK_NAME}

# create default.gcw0.desktop
cat > default.gcw0.desktop <<EOF
[Desktop Entry]
Name=OpenLara
Comment=Classic Tomb Raider open-source engine
Exec=OpenLara
Terminal=false
Type=Application
StartupNotify=true
Icon=icon
Categories=games;
X-OD-NeedsDownscaling=false
EOF

# create opk
FLIST="OpenLara"
FLIST="${FLIST} default.gcw0.desktop"
FLIST="${FLIST} icon.png"

rm -f ${OPK_NAME}
mksquashfs ${FLIST} ${OPK_NAME} -all-root -no-xattrs -noappend -no-exports

cat default.gcw0.desktop
rm -f default.gcw0.desktop
