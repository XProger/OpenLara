/* OpenLara - SNES / Super FX target
 * Exact sector lookup and the legacy GSU-2 shared flood. FX3's per-creature
 * incremental search is declared in nav_fx3.h.
 */

#ifndef H_OL_NAV
#define H_OL_NAV

#include "fixed.h"

#define NO_BOX      0xFFFF

/* Exact original sector mapping, including NO_BOX holes and stacked rooms.
   Point lookup uses Room_sectorAt's clamping and wrapped room coordinates. */
unsigned int navBoxAt(unsigned int room, int x, int z);
unsigned int navSectorBox(unsigned int sector, unsigned int roomDepth);
void         navResetCache(void);
#ifndef OL_FX3
extern unsigned int navNext[2][OL_BOX_MAX];  /* per zone kind, per box */
extern unsigned int navFrom[2];              /* the box each flood started at */
extern unsigned int navReached;              /* how many it connected */
int  boxCentreX(unsigned int b);
int  boxCentreZ(unsigned int b);
void navFlood(int flying, unsigned int start);
#endif

#endif
