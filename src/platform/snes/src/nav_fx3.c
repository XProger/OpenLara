/* src/fixed/nav.h: independent target-outward incremental searches and the
 * original corridor clipping algorithm. FX3's concurrent ROM access keeps
 * code in FastROM and dedicated per-creature cells in WRAM. Hot paths use
 * nav_fx3.asm; C_MOTION retains their complete executable C reference.
 * All three TR1 zone families and both normal/alternate maps are available. */
#include "level.h"
#include "nav_fx3.h"

NavFx3 olNav[OL_NAV_SLOTS];
/* Exact sector/depth/count keys, not the old ambiguous X/Z geometry cache.
   A collision between direct-map slots is a miss, never a guessed box. */
uint16 olNavSectorCache[64][4];
unsigned int olZoneOffsets[3], olFlipped;

/* A single CPU navigation call runs at a time; NMI never enters this code.
   Keep the clipping workspace here, not on the 65816 stack. 816-tcc gives
   every 32-bit expression another spill slot and cannot address a >255-byte
   frame. Small side helpers also keep its temporary frames bounded. */
NavPoint olNavClipFrom;         /* also read by native Nav_inside */
#define clipFrom olNavClipFrom
NavPoint* olNavClipWP;
#define clipWP olNavClipWP
int32 olNavBounds[4];           /* shared with native Nav_bounds */
#define bMinX olNavBounds[0]
#define bMaxX olNavBounds[1]
#define bMinZ olNavBounds[2]
#define bMaxZ olNavBounds[3]
int32 olNavClipBounds[4];
#define minX olNavClipBounds[0]
#define maxX olNavClipBounds[1]
#define minZ olNavClipBounds[2]
#define maxZ olNavClipBounds[3]
unsigned int olNavClip;
#define clip olNavClip

void Nav_copy(NavPoint* d, const NavPoint* s);
int32 Nav_min(int32 a, int32 b);
int32 Nav_max(int32 a, int32 b);
int32 Nav_clamp(int32 v, int32 a, int32 b);
void Nav_bounds(unsigned int box);
void Nav_expand(NavFx3* n, unsigned int box);
int Nav_inside(void);

#ifdef OL_C_MOTION
void Nav_copy(NavPoint* d, const NavPoint* s)
{
    d->x = s->x; d->y = s->y; d->z = s->z;
}

int32 Nav_min(int32 a, int32 b) { return a < b ? a : b; }
int32 Nav_max(int32 a, int32 b) { return a > b ? a : b; }
int32 Nav_clamp(int32 v, int32 a, int32 b)
{
    if (v < a) v = a;
    if (v > b) v = b;
    return v;
}
#endif

void Nav_setFlipped(unsigned int flipped)
{
    unsigned int i, offset;
    olFlipped = flipped ? 1 : 0;
    offset = olFlipped ? 6 : 0;
    for (i = 0; i < 3; i++) olZoneOffsets[i] = olTable16(Zone, offset + (i << 1));
}

void Nav_reset(void)
{
    unsigned int i;
    for (i = 0; i < OL_NAV_SLOTS; i++) olNav[i].owner = NO_BOX;
    navInvalidateSectors();
    Nav_setFlipped(0);
}

#ifdef OL_C_MOTION
/* Executable reference for nav_fx3.asm's bank-aware native hot paths. */
void navInvalidateSectors(void)
{
    unsigned int i;
    for (i = 0; i < 64; i++) olNavSectorCache[i][0] = NO_BOX;
}

void Nav_point(NavPoint* p, unsigned int room, int x, int y, int z)
{
    p->x = ((int32)olRoomSectorX(room) << 10) + (int16)(x - olRoomX(room));
    p->y = y;
    p->z = ((int32)olRoomSectorZ(room) << 10) + (int16)(z - olRoomZ(room));
}

void Nav_init(NavFx3* n, unsigned int box)
{
    unsigned int i, zone, altZone;
    const unsigned char *normal, *alternate;
    n->zoneType = n->stepHeight == 256 ? OL_ZONE_GROUND_1 :
        n->stepHeight == 512 ? OL_ZONE_GROUND_2 : OL_ZONE_FLY;
    n->weight = 0;
    n->endBox = n->nextBox = n->headBox = n->tailBox = NO_BOX;
    n->mask = 0x4000;            /* box blocked, not an overlap-index bit */
    n->cellsCount = 0;
    for (i = 0; i < olBoxCount; i++) {
        NavCell* c = n->cells + i;
        c->end = c->next = NO_BOX;
        c->weight = 0;
    }
    if (box >= olBoxCount) return;
    normal = olZoneData + olTable16(Zone, n->zoneType << 1);
    alternate = olZoneData + olTable16(Zone, (n->zoneType << 1) + 6);
    zone = olData16(normal, box << 1);
    altZone = olData16(alternate, box << 1);
    for (i = 0; i < olBoxCount; i++) {
        if (olData16(normal, i << 1) == zone || olData16(alternate, i << 1) == altZone) {
            n->cells[n->cellsCount++].boxIndex = i;
        }
    }
}

void Nav_search(NavFx3* n, unsigned int zone)
{
    unsigned int current = n->headBox;
    NavCell* curr = n->cells + current;
    unsigned int p = olBoxOverlap(current) & NAV_INDEX;
    unsigned int done;
    do {
        unsigned int box = olOverlap(p++);
        NavCell* next;
        unsigned int cw, nw;
        int32 diff;
        done = box & NAV_BLOCKED;
        if (done) box &= NAV_INDEX;
        if (box >= olBoxCount) continue;
        if (olZone(box, n->zoneType) != zone) continue;
        diff = (int32)olBoxFloor(box) - olBoxFloor(current);
        if (diff > n->stepHeight) continue;
        if (diff < n->dropHeight) continue;
        next = n->cells + box;
        cw = curr->weight & NAV_WEIGHT;
        nw = next->weight & NAV_WEIGHT;
        if (cw < nw) continue;
        if (curr->weight & NAV_BLOCKED) {
            if (cw == nw) continue;
            next->weight = curr->weight;
        } else {
            if (cw == nw) {
                if (!(next->weight & NAV_BLOCKED)) continue;
            }
            if (olBoxOverlap(box) & n->mask) {
                next->weight = curr->weight | NAV_BLOCKED;
            } else {
                next->weight = curr->weight;
                next->end = current;
            }
        }
        if (next->next == NO_BOX && box != n->tailBox) {
            n->cells[n->tailBox].next = box;
            n->tailBox = box;
        }
    } while (!done);
    n->headBox = curr->next;
    curr->next = NO_BOX;
    if (n->headBox == NO_BOX) n->tailBox = NO_BOX;
}
#endif

#ifdef OL_C_MOTION
void Nav_expand(NavFx3* n, unsigned int box)
{
    unsigned int i;
    if (n->nextBox < olBoxCount && n->nextBox != n->endBox) {
        NavCell* cell;
        /* Cell weight bit 15 marks blocked paths. Rebase before generation
           wrap can turn an ordinary search into a blocked one. */
        if (n->weight == NAV_WEIGHT) {
            unsigned int target = n->nextBox;
            Nav_init(n, box);
            n->nextBox = target;
        }
        n->endBox = n->nextBox;
        cell = n->cells + n->endBox;
        if (cell->next == NO_BOX && n->tailBox != n->endBox) {
            cell->next = n->headBox;
            if (n->headBox == NO_BOX) n->tailBox = n->endBox;
            n->headBox = n->endBox;
        }
        cell->weight = ++n->weight;
        cell->end = NO_BOX;
    }
    if (n->headBox != NO_BOX) {
        unsigned int zone = olZone(n->headBox, n->zoneType);
        for (i = 0; i < OL_NAV_STEPS && n->headBox != NO_BOX; i++) Nav_search(n, zone);
    }
}

void Nav_bounds(unsigned int box)
{
    bMinX = (int32)olBoxMinX(box) << 10;
    bMaxX = ((int32)olBoxMaxX(box) << 10) - 1;
    bMinZ = (int32)olBoxMinZ(box) << 10;
    bMaxZ = ((int32)olBoxMaxZ(box) << 10) - 1;
}
#endif

#ifdef OL_C_MOTION
int Nav_clipMinX(void)
{
    if (clipFrom.x >= bMinX) return 0;
    if (!(clip & 1)) goto clipOtherMinX;
    if (clipFrom.z < bMinZ) goto clipOtherMinX;
    if (clipFrom.z > bMaxZ) goto clipOtherMinX;
    clipWP->x = Nav_max(clipWP->x, bMinX + 512);
    if (clip & 16) return 1;
    minZ = Nav_max(minZ, bMinZ); maxZ = Nav_min(maxZ, bMaxZ);
    clip = 1;
    return 0;
clipOtherMinX:
    if (clip != 1) {
        clipWP->x = maxX - 512;
        if (clip != 15) return 1;
        clip = 16;
    }
    return 0;
}

int Nav_clipMaxX(void)
{
    if (clipFrom.x <= bMaxX) return 0;
    if (!(clip & 2)) goto clipOtherMaxX;
    if (clipFrom.z < bMinZ) goto clipOtherMaxX;
    if (clipFrom.z > bMaxZ) goto clipOtherMaxX;
    clipWP->x = Nav_min(clipWP->x, bMaxX - 512);
    if (clip & 16) return 1;
    minZ = Nav_max(minZ, bMinZ); maxZ = Nav_min(maxZ, bMaxZ);
    clip = 2;
    return 0;
clipOtherMaxX:
    if (clip != 2) {
        clipWP->x = minX + 512;
        if (clip != 15) return 1;
        clip = 16;
    }
    return 0;
}

int Nav_clipMinZ(void)
{
    if (clipFrom.z >= bMinZ) return 0;
    if (!(clip & 4)) goto clipOtherMinZ;
    if (clipFrom.x < bMinX) goto clipOtherMinZ;
    if (clipFrom.x > bMaxX) goto clipOtherMinZ;
    clipWP->z = Nav_max(clipWP->z, bMinZ + 512);
    if (clip & 16) return 1;
    minX = Nav_max(minX, bMinX); maxX = Nav_min(maxX, bMaxX);
    clip = 4;
    return 0;
clipOtherMinZ:
    if (clip != 4) {
        clipWP->z = maxZ - 512;
        if (clip != 15) return 1;
        clip = 16;
    }
    return 0;
}

int Nav_clipMaxZ(void)
{
    if (clipFrom.z <= bMaxZ) return 0;
    if (!(clip & 8)) goto clipOtherMaxZ;
    if (clipFrom.x < bMinX) goto clipOtherMaxZ;
    if (clipFrom.x > bMaxX) goto clipOtherMaxZ;
    clipWP->z = Nav_min(clipWP->z, bMaxZ - 512);
    if (clip & 16) return 1;
    minX = Nav_max(minX, bMinX); maxX = Nav_min(maxX, bMaxX);
    clip = 8;
    return 0;
clipOtherMaxZ:
    if (clip != 8) {
        clipWP->z = minZ + 512;
        if (clip != 15) return 1;
        clip = 16;
    }
    return 0;
}

void Nav_finish(NavFx3* n)
{
    if (clip & 3) clipWP->x = n->pos.x;
    else if (!(clip & 16)) clipWP->x = Nav_clamp(clipWP->x, bMinX + 512, bMaxX - 512);
    if (clip & 12) clipWP->z = n->pos.z;
    else if (!(clip & 16)) clipWP->z = Nav_clamp(clipWP->z, bMinZ + 512, bMaxZ - 512);
    clipWP->y = n->pos.y;
}

int Nav_inside(void)
{
    /* 816-tcc miscompiles a chain of && with 32-bit comparisons: the false
       branch of an early comparison can enter the final comparison. */
    if (clipFrom.x < bMinX) return 0;
    if (clipFrom.x > bMaxX) return 0;
    if (clipFrom.z < bMinZ) return 0;
    if (clipFrom.z > bMaxZ) return 0;
    return 1;
}

void Nav_deadEnd(NavFx3* n, unsigned int last)
{
    if (!(clip & 16)) {
        clipWP->x = Nav_clamp(clipWP->x, bMinX + 512, bMaxX - 512);
        clipWP->z = Nav_clamp(clipWP->z, bMinZ + 512, bMaxZ - 512);
    }
    clipWP->y = (int32)olBoxFloor(last) - (n->zoneType == OL_ZONE_FLY ? 384 : 0);
}

void Nav_getWaypoint(NavFx3* n, unsigned int box, const NavPoint* from)
{
    unsigned int i = 0, last;
    Nav_expand(n, box);
    Nav_copy(&n->waypoint, from);
    if (box >= olBoxCount) return;
    if (box == n->endBox) {
        Nav_copy(&n->waypoint, &n->pos);
        return;
    }
    Nav_copy(&clipFrom, from);
    clipWP = &n->waypoint;
    Nav_bounds(box);
    minX = bMinX; maxX = bMaxX; minZ = bMinZ; maxZ = bMaxZ;

    /* CLIP_MIN_X/MAX_X/MIN_Z/MAX_Z/DONE are 1/2/4/8/16. Keep the
       sequential tests: a corner may constrain both axes in one box. */
    clip = 15;
    do {
        last = box;
        Nav_bounds(box);
        if (Nav_inside()) {
            minX = bMinX; maxX = bMaxX; minZ = bMinZ; maxZ = bMaxZ;
        } else {
            if (Nav_clipMinX()) return;
            if (Nav_clipMaxX()) return;
            if (Nav_clipMinZ()) return;
            if (Nav_clipMaxZ()) return;
        }
        if (box == n->endBox) {
            Nav_finish(n);
            return;
        }
        box = n->cells[box].end;
        if (box >= olBoxCount) break;
        if (olBoxOverlap(box) & n->mask) break;
    } while (++i < olBoxCount); /* Corrupt/cyclic paths must not hang a tick. */
    Nav_deadEnd(n, last);
}
#endif
