/* Per-creature Nav for FX3. The GSU-2 shared flood remains in nav.c until
 * its resident-code/data layout can support this implementation too. */
#ifndef H_OL_NAV_FX3
#define H_OL_NAV_FX3

#include "nav.h"

#define OL_NAV_SLOTS 3          /* GBA/32X MAX_ENEMIES */
#define OL_NAV_STEPS 5          /* GBA/32X NAV_STEPS */
#define NAV_INDEX   0x3FFF
#define NAV_WEIGHT  0x7FFF
#define NAV_BLOCKED 0x8000

/* Waypoints need real world coordinates: box bounds span 256 sectors,
   whereas ItemObj stores the low 16 bits of each world coordinate. */
typedef struct NavPoint { int32 x, y, z; } NavPoint;
typedef struct NavCell { uint16 boxIndex, weight, end, next; } NavCell;
typedef struct NavFx3 {
    NavCell cells[OL_BOX_MAX];
    uint16 cellsCount, zoneType, weight, endBox, nextBox, headBox, tailBox;
    int16 stepHeight, dropHeight, vSpeed;
    uint16 mask, owner;
    NavPoint pos, waypoint;
} NavFx3;

/* This unit's .bss has a dedicated, link-checked FX3 WRAM slot at $7E:5800.
   No fixed C pointer and no enlarged general BSS/stack region. */
extern NavFx3 olNav[OL_NAV_SLOTS];
void Nav_reset(void);
/* Select navigation maps only. Room/renderer swapping is not implemented yet. */
void Nav_setFlipped(unsigned int flipped);
void Nav_init(NavFx3* nav, unsigned int box);
void Nav_search(NavFx3* nav, unsigned int zone);
void Nav_getWaypoint(NavFx3* nav, unsigned int box, const NavPoint* from);
void Nav_point(NavPoint* point, unsigned int room, int x, int y, int z);
void navInvalidateSectors(void);  /* after replacing the immutable sector map */

#endif
