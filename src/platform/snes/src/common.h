/* OpenLara - SNES / Super FX target
 * The port of src/fixed/common.h, as far as this side needs it.
 *
 * Upstream's common.h is 74 KB of every declaration in the engine. What is
 * here is the part the host uses and can use: the box helpers, which are pure
 * functions and therefore the easiest thing in the whole port to get right --
 * and the easiest to get subtly wrong by writing something that looks like
 * them instead.
 */

#ifndef H_OL_COMMON
#define H_OL_COMMON

#include "fixed.h"

/* Upstream's AABBi, in upstream's field order. */
typedef struct AABBi {
    int minX, maxX;
    int minY, maxY;
    int minZ, maxZ;
} AABBi;

typedef struct vec3i {
    int x, y, z;
} vec3i;

/* A quadrant is `(uint16)(angle + ANGLE_45) >> ANGLE_SHIFT_90`, and quadrant
   2 is the identity -- which is upstream's convention and not an obvious one,
   so it is worth writing down where the caller can see it. */
#define ANGLE_SHIFT_90  14
#define olQuadrant(a)   ((unsigned int)((a) + ANGLE_45) >> ANGLE_SHIFT_90)

int   boxIntersect(const AABBi* a, const AABBi* b);
int   boxContains(const AABBi* a, const vec3i* p);
/* How far to move `b` to get it clear of `a`: the obstacle first, the thing
   being pushed second, and the result applies to the second. Upstream's
   caller reads `boxPushOut(meshBox, objBox)` and moves the object, which is
   the only thing that says which way round it goes -- and getting it backwards
   returns a vector that pushes you further in.

   Upstream returns a vec3i by value; this writes through a pointer, because a
   struct return here is a memcpy and there is none in the runtime -- the link
   says so, which is the good kind of failure. */
void  boxPushOut(const AABBi* a, const AABBi* b, vec3i* out);
void  boxTranslate(AABBi* box, int x, int y, int z);
void  boxRotateYQ(AABBi* box, int quadrant);

void  anglesFromVector(int x, int y, int z, int* angleX, int* angleY);

#endif
