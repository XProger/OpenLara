/* OpenLara - SNES / Super FX target
 * The two-sector drawbridge from src/fixed/item.h. Kept in its own FastROM
 * unit so later-level floors do not consume GSU-2's resident code budget.
 */
#include "item.h"
#include "room.h"

/* Load-only rejection box for the small platform list. Keep it in this
   bank with the floor helper instead of the crowded level-loader bank. */
void ItemObj_extendPlatformBounds(ItemObj* item, int extent)
{
    int sx = item->x >> 10, sz = item->z >> 10;
    int low = sx - extent, high = sx + extent;
    if (low < -32 || high > 31) { low = -32; high = 31; }
    if (low < olBridgeBox[0]) olBridgeBox[0] = low;
    if (high > olBridgeBox[1]) olBridgeBox[1] = high;
    low = sz - extent; high = sz + extent;
    if (low < -32 || high > 31) { low = -32; high = 31; }
    if (low < olBridgeBox[2]) olBridgeBox[2] = low;
    if (high > olBridgeBox[3]) olBridgeBox[3] = high;
}

int ItemObj_getDrawBridgeFloor(ItemObj* item, int x, int z)
{
    int dx = (item->x >> 10) - (x >> 10);
    int dz = (item->z >> 10) - (z >> 10);
    /* World positions are packed words: neighbouring sectors can straddle
       +32767/-32768. Recover the short difference on the 64-sector ring. */
    dx = (int)(((unsigned int)(dx + 32) & 63) - 32);
    dz = (int)(((unsigned int)(dz + 32) & 63) - 32);

    if (item->angleY == ANGLE_0) {
        if (dx == 0 && (dz == -1 || dz == -2)) return item->y;
    } else if (item->angleY == (uint16)ANGLE_180) {
        if (dx == 0 && (dz == 1 || dz == 2)) return item->y;
    } else if (item->angleY == ANGLE_90) {
        if (dz == 0 && (dx == -1 || dx == -2)) return item->y;
    } else if (item->angleY == (uint16)(0 - ANGLE_90)) {
        if (dz == 0 && (dx == 1 || dx == 2)) return item->y;
    }
    return WALL;
}
