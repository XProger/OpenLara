/* OpenLara - SNES / Super FX target
 * The port of upstream's CollisionInfo, ItemObj::collideRoom and Lara's c_
 * collision handlers -- the part of lara.h that decides what the world does
 * to her after she has moved: walls, steps, ledges, slides, splats, ceilings
 * and landings.
 *
 * Every question it asks is a Room_query at an arbitrary point, which is why
 * it could not exist while the GSU answered one probe a frame. It asks four
 * a tick: under her, and at her radius ahead, left and right.
 */

#ifndef H_OL_COLLISION
#define H_OL_COLLISION

#include "item.h"
#include "room.h"

/* Upstream's CollisionType, as bits. */
#define CT_NONE             0
#define CT_FRONT            1
#define CT_LEFT             2
#define CT_RIGHT            4
#define CT_CEILING          8
#define CT_FRONT_CEILING    16
#define CT_FLOOR_CEILING    32

/* Upstream's SlantType. */
#define SLANT_NONE          0
#define SLANT_LOW           1
#define SLANT_HIGH          2

/* WALL is room.h's: the water column asks it too. */

typedef struct CollisionSide {
    int   floor;        /* relative to her feet: negative is a step up */
    int   ceiling;      /* relative to her head */
    uint8 slantType;
} CollisionSide;

typedef struct CollisionInfo {
    unsigned int  trigger;      /* the sector under the middle point's */
    CollisionSide m, f, l, r;   /* middle, front, left, right */
    int radius;
    int gapPos, gapNeg, gapCeiling;
    int offX, offY, offZ;       /* upstream's offset: how far to push her */
    int posX, posY, posZ;       /* upstream's pos: where she stood before the move */
    uint16       angle;         /* the direction the probe faces */
    unsigned int quadrant;
    uint8 type;                 /* CT_* */
    int8  slantX, slantZ;       /* the middle sector's floor slant */
    uint8 stopOnSlant, stopOnLava, enemyPush, enemyHit;
    uint8 staticHit;            /* collideStatic pushed her this tick */
} CollisionInfo;

/* Upstream's Lara::cinfo. */
extern CollisionInfo cinfo;

void CollisionInfo_setAngle(CollisionInfo* ci, uint16 value);

/* void Lara::alignWall(int32 radius): her radius off the wall she faces,
   on the sector grid. A block takes hold of her with it. */
void Lara_alignWall(ItemObj* L, int radius);

/* void ItemObj::collideRoom(int32 height, int32 yOffset) const */
void ItemObj_collideRoom(ItemObj* item, CollisionInfo* ci, int height, int yOffset);

/* The c_ handler of her current state: void Lara::updateCollision(). */
void Lara_updateCollision(ItemObj* lara, unsigned int input);

/* bool Lara::s_checkFront(int16 angleDelta, int32 radius): whether the way
   `angleDelta` from her facing is open, probed on a scratch CollisionInfo. */
int  Lara_checkFront(ItemObj* lara, unsigned int input, int angleDelta, int radius);

/* Lara::updateObjectsCollision, as far as it goes: the doors. */
int  Lara_collideItems(ItemObj* lara);

/* The floor a bridge puts under a point, if one stands in its sector. */
void Lara_itemFloorAt(int x, int y, int z, int* floor);

#endif
