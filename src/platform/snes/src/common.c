/* OpenLara - SNES / Super FX target
 * The port of the box helpers in src/fixed/common.cpp.
 *
 * Pure functions, ported line for line. They are covered by tools/fixedcheck,
 * which compiles this same file for the desktop and runs it against an
 * independent model -- which is the only way to know a transcription is one.
 */

#include "common.h"

int boxIntersect(const AABBi* a, const AABBi* b)
{
    return !(a->maxX <= b->minX || a->minX >= b->maxX ||
             a->maxY <= b->minY || a->minY >= b->maxY ||
             a->maxZ <= b->minZ || a->minZ >= b->maxZ);
}

int boxContains(const AABBi* a, const vec3i* p)
{
    return !(a->minX > p->x || a->maxX < p->x ||
             a->minY > p->y || a->maxY < p->y ||
             a->minZ > p->z || a->maxZ < p->z);
}

/* How far to move `a` to get it out of `b`, along whichever axis is shallower.
   Upstream returns a vec3i by value; this writes through a pointer, because a
   struct return here is a memcpy and the runtime has none. */
void boxPushOut(const AABBi* a, const AABBi* b, vec3i* out)
{
    int ax = b->maxX - a->minX;
    int bx = a->maxX - b->minX;
    int az = b->maxZ - a->minZ;
    int bz = a->maxZ - b->minZ;

    out->y = 0;
    out->x = (ax < bx) ? -ax : bx;
    out->z = (az < bz) ? -az : bz;
}

void boxTranslate(AABBi* box, int x, int y, int z)
{
    box->minX += x;
    box->maxX += x;
    box->minY += y;
    box->maxY += y;
    box->minZ += z;
    box->maxZ += z;
}

/* Quadrant 2 is the identity. That reads oddly and is upstream's: the box is
   stored facing one way and the quadrant says how far round from *that* it
   has been turned. Getting this wrong is invisible on a symmetric box and
   obvious on a door. */
void boxRotateYQ(AABBi* box, int quadrant)
{
    int minX, maxX, minZ, maxZ;

    if (quadrant == 2)
        return;

    minX = box->minX;
    maxX = box->maxX;
    minZ = box->minZ;
    maxZ = box->maxZ;

    if (quadrant == 3) {
        box->minX = minZ;
        box->maxX = maxZ;
        box->minZ = -maxX;
        box->maxZ = -minX;
    } else if (quadrant == 1) {
        box->minX = -maxZ;
        box->maxX = -minZ;
        box->minZ = minX;
        box->maxZ = maxX;
    } else if (quadrant == 0) {
        box->minX = -maxX;
        box->maxX = -minX;
        box->minZ = -maxZ;
        box->maxZ = -minZ;
    }
}

void anglesFromVector(int x, int y, int z, int* angleX, int* angleY)
{
    *angleY = (int)phd_atan((int32)z, (int32)x);
    *angleX = (int)phd_atan((int32)phd_sqrt((uint32)((int32)x * x + (int32)z * z)),
                            (int32)(-y));
}
