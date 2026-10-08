/* OpenLara - SNES / Super FX target
 * The port of src/fixed/nav.h.
 *
 * Upstream keeps an incremental search per creature, seeded at its target
 * box. GSU-2 shares one complete target-outward breadth-first flood per zone
 * kind instead. Each entry names the neighbour leading to the target.
 * FX3 uses the per-creature implementation in nav_fx3.c; only exact sector
 * lookup and the reset interface are shared here.
 *
 * Everything here is in *sector* units. They are absolute, they are small, and
 * they are the only coordinate on this side that cannot wrap -- TR's world is
 * wider than sixteen bits and ItemObj positions are int16. See olSectorX.
 */

#include "level.h"
#include "nav.h"
#include "room.h"
#ifdef OL_FX3
#include "nav_fx3.h"
#endif

#ifndef OL_FX3
unsigned int navNext[2][OL_BOX_MAX];
unsigned int navFrom[2];
unsigned int navReached;

/* The flood's queue, one for both kinds, and per kind a bit for every box
   the last flood of that kind wrote: the next flood of that kind clears
   those and not all of them. Clearing every box was 619 stores a flood in
   LEVEL1 and more in LEVEL2, on a tick that was already over budget there;
   a queue kept per kind for the same purpose was 2 KB of .bss where these
   bits are 256 bytes. */
#ifdef OL_C_MOTION
static unsigned int navQueue[OL_BOX_MAX];
static unsigned int navWrote[2][OL_BOX_MAX / 16];
#else
/* The native GSU-2 flood uses these same buffers; no additional work RAM. */
unsigned int navQueue[OL_BOX_MAX];
unsigned int navWrote[2][OL_BOX_MAX / 16];
void navInvalidateSectorsGsu2(void);
#endif

void navResetCache(void)
{
    unsigned int i;
#ifndef OL_C_MOTION
    navInvalidateSectorsGsu2();
#endif
    /* No initialised statics on this toolchain: the flood tables start
       here, once, and every flood after leaves them clear behind it. */
    for (i = 0; i < OL_BOX_MAX; i++) {
        navNext[0][i] = NO_BOX;
        navNext[1][i] = NO_BOX;
    }
    for (i = 0; i < OL_BOX_MAX / 16; i++) {
        navWrote[0][i] = 0;
        navWrote[1][i] = 0;
    }
    navReached = 0;
    navFrom[0] = navFrom[1] = NO_BOX;
}
#else
void navResetCache(void) { Nav_reset(); }
#endif

#ifdef OL_C_MOTION
/* Executable reference for the native room.asm entry point. */
unsigned int navBoxAt(unsigned int room, int x, int z)
{
    /* Separate guards: this 816-tcc miscompiles the combined || expression
       with two negated byte loads, bypassing the invalid-room return. */
    if (room >= olRoomCount) return NO_BOX;
    if (!olRoomXSectors(room)) return NO_BOX;
    if (!olRoomZSectors(room)) return NO_BOX;
    return navSectorBox(Room_sectorAt(room, x, z), olRoomZSectors(room));
}
#endif

#ifndef OL_FX3
int boxCentreX(unsigned int b)
{
    return (olBoxMinX(b) + olBoxMaxX(b)) >> 1;
}

int boxCentreZ(unsigned int b)
{
    return (olBoxMinZ(b) + olBoxMaxZ(b)) >> 1;
}

/* ---------------------------------------------------------------------------
 * void Nav::search(...)
 *
 * The flood. It stays inside the target's zone, which is what a creature could
 * reach in the first place -- TR's zones are the difference between a wolf
 * that waits on its side of the water and one that walks into it, and why
 * there are two floods rather than one.
 * ------------------------------------------------------------------------ */
#ifdef OL_C_MOTION
void navFlood(int flying, unsigned int start)
{
    unsigned int head = 0, tail = 0, zone, i;
    unsigned int* queue = navQueue;
    unsigned int* wrote = navWrote[flying];

    navFrom[flying] = start;
    for (i = 0; i < OL_BOX_MAX / 16; i++) {
        unsigned int w = wrote[i];
        unsigned int b = i << 4;
        while (w) {
            if (w & 1) navNext[flying][b] = NO_BOX;
            w >>= 1;
            b++;
        }
        wrote[i] = 0;
    }
    if (start >= olBoxCount || start >= OL_BOX_MAX) return;

    zone = olZone(start, flying);
    navNext[flying][start] = start;         /* the end of every path */
    wrote[start >> 4] |= (unsigned int)(1u << (start & 15));
    queue[tail++] = start;

    while (head < tail)
    {
        unsigned int cur = queue[head++];
        unsigned int overlap = olBoxOverlap(cur);
        unsigned int p;

        if (overlap & 0x4000) continue;     /* blocked, not merely blockable */
        p = overlap & 0x3FFF;

        for (;;)
        {
            unsigned int w = olOverlap(p);
            unsigned int b = w & 0x7FFF;

            if (b < olBoxCount && b < OL_BOX_MAX &&
                navNext[flying][b] == NO_BOX &&
                olZone(b, flying) == zone)
            {
                /* Reached from `cur`, so from b the way back is toward cur. */
                navNext[flying][b] = cur;
                wrote[b >> 4] |= (unsigned int)(1u << (b & 15));
                if (tail < OL_BOX_MAX) queue[tail++] = b;
            }

            if (w & 0x8000) break;
            p++;
        }
    }

    navReached = tail;
}
#endif
#endif
