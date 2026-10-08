/* Small level accessors, separated so the cold loader and creature updates
   can share the available resident code banks. */
#include "level.h"

int olLevelReady(void)
{
#ifdef OL_FX3
    unsigned char* ready = (unsigned char*)0x70F314LL;
    if (ready[0] != 1) return 0;
    return olHostTablesReady == 1;
#else
    unsigned int n = olBlob16(0);
    unsigned char* ready = (unsigned char*)0x70F314LL;
    return (ready[0] == 1 && n > 16 && n <= OL_ITEM_BLOB);
#endif
}

/* The room's absolute sector origin plus a short local offset disambiguates
   the 16-bit world-coordinate seam. LEVEL1 spans 98 sectors, not 32. */
int olSectorX(unsigned int room, int x)
{
    return olRoomSectorX(room) + ((x - olRoomX(room)) >> 10);
}

int olSectorZ(unsigned int room, int z)
{
    return olRoomSectorZ(room) + ((z - olRoomZ(room)) >> 10);
}
