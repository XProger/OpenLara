/* OpenLara - SNES / Super FX target
 * The port of ItemObj::collideRoom (item.h) and Lara's c_ handlers (lara.h).
 * See collision.h.
 *
 * Read beside upstream. What is deliberately not here:
 *   - collideStatic: ported; the packer ships the boxes, see level.h.
 *   - c_climbSide and the TR2 wall climb; the handstand.
 *   - angle.z, the lean: ItemObj has no room for it and the renderer turns an
 *     entity about Y only.
 *   - the wade-out arms of c_checkWallUW and c_surf, which stand her up in
 *     shallow water on TR2's animations. TR1 swims in the shallows.
 *   - stopOnLava: upstream's own lava handling is a TODO.
 *
 * Frame ranges. Upstream tests `frameIndex` against *global* frame numbers;
 * this port's frameIndex is local to the animation. Lara's are fixed data:
 * the run is animation 0 at frame 0, the walk animation 1 at frame 22, the
 * back-walk animation 41 at frame 598 (`OL_DUMP_ANIMS=1 ./build/packer`).
 * So a range of 29..47 on the walk is 7..25 here, and it applies only when
 * the animation *is* the walk.
 */

#include "collision.h"
#include "sound.h"
#include "common.h"
#include "level.h"
#include "room.h"
#include "object.h"
#include "lara.h"

CollisionInfo cinfo;

/* ---------------------------------------------------------------------------
 * CollisionInfo
 * ------------------------------------------------------------------------ */

#ifdef OL_C_MOTION
void CollisionInfo_setAngle(CollisionInfo* ci, uint16 value)
{
    ci->angle    = value;
    ci->quadrant = olQuadrant(value);
}

/* void CollisionInfo::setSide(SideType st, int32 floor, int32 ceiling) */
static void CollisionInfo_setSide(CollisionInfo* ci, CollisionSide* s,
                                  int floor, int ceiling, int sx, int sz,
                                  int middle)
{
    uint8 slantType;
    int ax = (sx < 0) ? -sx : sx;
    int az = (sz < 0) ? -sz : sz;

    if (sx == 0 && sz == 0) {
        slantType = SLANT_NONE;
    } else if (ax < 3 && az < 3) {
        slantType = SLANT_LOW;
    } else {
        slantType = SLANT_HIGH;
    }

    if (!middle) {
        if (ci->stopOnSlant && floor < 0 && slantType == SLANT_HIGH) {
            floor = -0x7FFF;
        } else if (ci->stopOnSlant && floor > 0 && slantType == SLANT_HIGH) {
            floor = 512;
        }
    }

    s->slantType = slantType;
    s->floor     = floor;
    s->ceiling   = ceiling;
}
#else
/* Native entry points; C_MOTION keeps the complete C implementation. */
void CollisionInfo_setSide(CollisionInfo* ci, CollisionSide* side,
                          int floor, int ceiling, int sx, int sz, int middle);
#endif

/* int32 alignOffset(int32 a, int32 b) -- how far to move `a` back into
   `b`'s sector. */
#ifdef OL_C_MOTION
static int alignOffset(int a, int b)
{
    int ca = a >> 10;
    int cb = b >> 10;

    if (ca == cb) return 0;

    a &= 1023;

    if (ca < cb) return 1025 - a;
    return -(a + 1);
}
#endif

/* ---------------------------------------------------------------------------
 * One probe point. Upstream's CHECK_HEIGHT: resolve the room from the last
 * point's room, take the floor under the point and the ceiling over it,
 * both relative to her, and let a bridge standing in the sector raise the
 * floor -- which is Sector::getFloor's getTriggerFloorCeiling.
 * ------------------------------------------------------------------------ */
/* Shared only by serial CPU probes; NMI never enters collision. */
unsigned int olColRoom, olColTrigger;
int olColSlantX, olColSlantZ;
#define colRoom olColRoom
#define colTrigger olColTrigger
#define colSlantX olColSlantX
#define colSlantZ olColSlantZ

/* s_checkFront only ever asks whether the answer is CT_FRONT or
   CT_FRONT_CEILING, and the left and right points can only produce CT_LEFT
   and CT_RIGHT -- so it does not pay for them. Two room queries, not four. */
int olColFrontOnly;
#define colFrontOnly olColFrontOnly

/* Upstream applies a bridge that the sector's FloorData names; the sector
   is the match here, not the room, because a probe above her head can
   resolve into the room above the bridge. A bridge counts when it is below
   the point and above the floor the room offered -- TR's Y grows down. */
#ifdef OL_C_MOTION
void Lara_itemFloorAt(int x, int y, int z, int* floor)
{
    unsigned int n;
    int sx = x >> 10, sz = z >> 10;

    if (sx < olBridgeBox[0] || sx > olBridgeBox[1] ||
        sz < olBridgeBox[2] || sz > olBridgeBox[3]) return;

    for (n = 0; n < olBridgeCount; n++)
    {
        ItemObj* it = items + olBridges[n];
        int h;
#ifdef OL_FX3
        if (it->type != 41 && it->type != 65 && it->type != 66) {
#endif
        if ((it->x >> 10) != sx) continue;
        if ((it->z >> 10) != sz) continue;
#ifdef OL_FX3
        }
#endif
        h = ItemObj_getBridgeFloor(it, x, z);
#ifdef OL_FX3
        if (h == WALL) continue;
#endif
        if (y <= h && h <= *floor) *floor = h;
    }
}

#endif

#ifdef OL_C_MOTION
static void colPoint(CollisionInfo* ci, CollisionSide* side,
                     int x, int cy, int z, int py, int y, int middle)
{
    RoomQuery q;
    int floor, ceiling, sx = 0, sz = 0;

    if (Room_query(colRoom, x, cy, z, &q))
    {
        colRoom = q.room;
        sx = q.floorSlopeX;
        sz = q.floorSlopeZ;
        colTrigger = q.trigger;

        floor = q.floor;
        if (floor != OL_NO_FLOOR) {
            Lara_itemFloorAt(x, cy, z, &floor);
            floor -= py;
        } else {
            floor = WALL;
        }

        ceiling = q.ceiling;
        if (ceiling != WALL) ceiling -= y;
    }
    else
    {
        floor = WALL;
        ceiling = WALL;
        colTrigger = 0xFFFF;
    }

    if (middle) {
        colSlantX = sx;
        colSlantZ = sz;
    }
    CollisionInfo_setSide(ci, side, floor, ceiling, sx, sz, middle);
}
#else
void colPoint(CollisionInfo* ci, CollisionSide* side,
              int x, int cy, int z, int py, int y, int middle);
#endif

/* ---------------------------------------------------------------------------
 * bool Room::collideStatic(CollisionInfo &cinfo, const vec3i &p, int32 height)
 *
 * Upstream walks the rooms near the point; this walks the point's own room.
 * A mesh straddling a room boundary would be missed from the other side,
 * which TR's level design does not do. The box comes from the packer already
 * turned to the mesh's quadrant, so what is left is the translation -- to
 * the point rather than to the world, because the world does not fit in
 * sixteen bits here and the distance to a mesh does.
 * ------------------------------------------------------------------------ */
#define X_ABS(v)                ((v) < 0 ? -(v) : (v))

/* Shared native caller; the static hit response stays in C. */
int Room_collideStatic(unsigned int room, CollisionInfo* ci,
                              int px, int py, int pz, int height)
{
    AABBi objBox, meshBox;
    vec3i off;
    int flip;

    ci->staticHit = 0;
    ci->offX = ci->offY = ci->offZ = 0;

    /* The walk over the room's placements, the reject by distance and the
       box test are src/room.asm's; what comes back is the box of the first
       one she meets, already where it stands relative to her. */
    if (!olStaticHit(room, px, py, pz, ci->radius, height, &meshBox)) return 0;

    objBox.minX = -ci->radius;
    objBox.maxX =  ci->radius;
    objBox.minZ = -ci->radius;
    objBox.maxZ =  ci->radius;
    objBox.minY = -height;
    objBox.maxY = 0;

    boxPushOut(&meshBox, &objBox, &off);
    ci->offX = off.x;
    ci->offY = off.y;
    ci->offZ = off.z;

    flip = (ci->quadrant > 1);

    if (ci->quadrant & 1) {
        if (X_ABS(ci->offZ) > ci->radius) {
            ci->offZ = ci->posZ - pz;
            if (ci->offX < 0 && ci->quadrant == 1) ci->type = CT_FRONT;
            if (ci->offX > 0 && ci->quadrant == 3) ci->type = CT_FRONT;
        } else if (ci->offZ != 0) {
            ci->offX = 0;
            ci->type = ((ci->offZ > 0) ^ flip) ? CT_RIGHT : CT_LEFT;
        } else {
            ci->offX = ci->offY = ci->offZ = 0;
        }
    } else {
        if (X_ABS(ci->offX) > ci->radius) {
            ci->offX = ci->posX - px;
            if (ci->offZ < 0 && ci->quadrant == 0) ci->type = CT_FRONT;
            if (ci->offZ > 0 && ci->quadrant == 2) ci->type = CT_FRONT;
        } else if (ci->offX != 0) {
            ci->offZ = 0;
            ci->type = ((ci->offX > 0) ^ flip) ? CT_LEFT : CT_RIGHT;
        } else {
            ci->offX = ci->offY = ci->offZ = 0;
        }
    }

    ci->staticHit = (ci->offX != 0 || ci->offZ != 0);
    return 1;
}

/* ---------------------------------------------------------------------------
 * void ItemObj::collideRoom(int32 height, int32 yOffset) const
 * ------------------------------------------------------------------------ */
#ifdef OL_C_MOTION
void ItemObj_collideRoom(ItemObj* item, CollisionInfo* ci, int height, int yOffset)
{
    int px = item->x;
    int py = item->y + yOffset;
    int pz = item->z;
    int y  = py - height;           /* the top of her head */
    int cy = y - 160;               /* and a little above it */
    int R  = ci->radius;
    int fx, fz, lx, lz, rx, rz;
    int dX, dY, dZ;
    int32 s, c;

    ci->type = CT_NONE;
    ci->offX = ci->offY = ci->offZ = 0;

    colRoom = item->room;

    /* middle */
    colPoint(ci, &ci->m, px, cy, pz, py, y, 1);
    ci->trigger = colTrigger;
    ci->slantX  = (int8)colSlantX;
    ci->slantZ  = (int8)colSlantZ;

    s = phd_sin((int32)ci->angle);
    c = phd_cos((int32)ci->angle);

    switch (ci->quadrant)
    {
        case 0:
            fx = (int)(((int32)R * s) >> FIXED_SHIFT); fz =  R;
            lx = -R; lz =  R;
            rx =  R; rz =  R;
            break;
        case 1:
            fx =  R; fz = (int)(((int32)R * c) >> FIXED_SHIFT);
            lx =  R; lz =  R;
            rx =  R; rz = -R;
            break;
        case 2:
            fx = (int)(((int32)R * s) >> FIXED_SHIFT); fz = -R;
            lx =  R; lz = -R;
            rx = -R; rz = -R;
            break;
        default:
            fx = -R; fz = (int)(((int32)R * c) >> FIXED_SHIFT);
            lx = -R; lz = -R;
            rx = -R; rz =  R;
            break;
    }

    fx += px; fz += pz;
    lx += px; lz += pz;
    rx += px; rz += pz;

    dX = ci->posX - px;
    dY = ci->posY - py;
    dZ = ci->posZ - pz;

    colPoint(ci, &ci->f, fx, cy, fz, py, y, 0);
    if (!colFrontOnly) {
        colPoint(ci, &ci->l, lx, cy, lz, py, y, 0);
        colPoint(ci, &ci->r, rx, cy, rz, py, y, 0);
    }

    /* static objects */
    Room_collideStatic(item->room, ci, px, py, pz, height);

    /* middle */
    if (ci->m.floor == WALL) {
        ci->offX = dX; ci->offY = dY; ci->offZ = dZ;
        ci->type = CT_FRONT;
        return;
    }

    if (ci->m.floor <= ci->m.ceiling) {
        ci->offX = dX; ci->offY = dY; ci->offZ = dZ;
        ci->type = CT_FLOOR_CEILING;
        return;
    }

    if (ci->m.ceiling >= 0) {
        ci->offY = ci->m.ceiling;
        ci->type = CT_CEILING;
    }

    /* front */
    if (ci->f.floor > ci->gapPos ||
        ci->f.floor < ci->gapNeg ||
        ci->f.ceiling > ci->gapCeiling)
    {
        if (ci->quadrant & 1) {
            ci->offX = alignOffset(fx, px);
            ci->offZ = dZ;
        } else {
            ci->offX = dX;
            ci->offZ = alignOffset(fz, pz);
        }
        ci->type = CT_FRONT;
        return;
    }

    /* front ceiling */
    if (ci->f.ceiling >= ci->gapCeiling) {
        ci->offX = dX; ci->offY = dY; ci->offZ = dZ;
        ci->type = CT_FRONT_CEILING;
        return;
    }

    if (colFrontOnly) return;

    /* left */
    if (ci->l.floor > ci->gapPos || ci->l.floor < ci->gapNeg) {
        if (ci->quadrant & 1) {
            ci->offZ = alignOffset(lz, fz);
        } else {
            ci->offX = alignOffset(lx, fx);
        }
        ci->type = CT_LEFT;
        return;
    }

    /* right */
    if (ci->r.floor > ci->gapPos || ci->r.floor < ci->gapNeg) {
        if (ci->quadrant & 1) {
            ci->offZ = alignOffset(rz, fz);
        } else {
            ci->offX = alignOffset(rx, fx);
        }
        ci->type = CT_RIGHT;
        return;
    }
}
#endif

/* ---------------------------------------------------------------------------
 * bool Lara::s_checkFront(int16 angleDelta, int32 radius)
 *
 * Upstream copies cinfo aside, probes, and copies it back. A struct copy is a
 * memcpy on this compiler and the runtime has none, so the probe runs on a
 * second CollisionInfo and cinfo is never touched. moveAngle likewise.
 * ------------------------------------------------------------------------ */
static CollisionInfo probe;

int Lara_checkFront(ItemObj* lara, unsigned int input, int angleDelta, int radius)
{
    int collide;

    probe.posX = lara->x;
    probe.posY = lara->y;
    probe.posZ = lara->z;
    CollisionInfo_setAngle(&probe, (uint16)(lara->angleY + angleDelta));
    probe.radius      = radius;
    probe.type        = CT_NONE;
    probe.gapPos      = -WALL;
    probe.gapNeg      = -LARA_STEP_HEIGHT;
    probe.stopOnSlant = 1;
    probe.stopOnLava  = 0;
    probe.gapCeiling  = 0;

    if ((uint16)angleDelta == (uint16)ANGLE_180) {
        if ((input & IN_WALK) || olLaraWaterState == WATER_STATE_WADE) {
            probe.gapPos     = LARA_STEP_HEIGHT;
            probe.stopOnLava = 1;
        }
    }

    colFrontOnly = 1;
    ItemObj_collideRoom(lara, &probe, LARA_HEIGHT, 0);
    colFrontOnly = 0;

    collide = (probe.type == CT_FRONT) || (probe.type == CT_FRONT_CEILING);
    return !collide;
}

/* ---------------------------------------------------------------------------
 * Lara::updateObjectsCollision, as far as it goes.
 *
 * TR blocks a doorway by rewriting the floor data behind the door. FX3 does
 * that (door.c); GSU-2 has no room for it, so a door is a box there, solid
 * only while its state says shut. A bridge is deliberately not solid: it is
 * something to stand on.
 * ------------------------------------------------------------------------ */
#define SOLID_RADIUS    128

#ifndef OL_FX3
#ifdef OL_C_MOTION
int Lara_collideItems(ItemObj* lara)
{
    unsigned int n;
    int hit = 0;

    for (n = 0; n < olDoorCount; n++)
    {
        ItemObj* it = items + olDoors[n];
        if (it->room != lara->room) continue;
        if (it->state != STATE_CLOSE) continue;
        if (!ItemObj_collideBounds(it, lara, SOLID_RADIUS, LARA_HEIGHT)) continue;
        ItemObj_collidePush(it, lara, SOLID_RADIUS);
        hit = 1;
    }
    return hit;
}
#endif
#endif

/* ---------------------------------------------------------------------------
 * The c_ helpers. `L` is her; the animation indices are her model's.
 * ------------------------------------------------------------------------ */
static unsigned int laraAnimBase;

#define LANIM(a)    (laraAnimBase + (a))

static void animSet(ItemObj* L, unsigned int anim, int resetState, unsigned int frame)
{
    ItemObj_animSetState(L, LANIM(anim), frame, resetState);
}

/* virtual void hit(int32 damage, ...) -- the health part. */
static void Lara_hit(ItemObj* L, int damage)
{
    if (L->health <= 0 || damage <= 0) return;
    L->health = (int16)(L->health - damage);
    if (L->health < 0) L->health = 0;
}

static void c_applyOffset(ItemObj* L)
{
    L->x = (int16)(L->x + cinfo.offX);
    L->y = (int16)(L->y + cinfo.offY);
    L->z = (int16)(L->z + cinfo.offZ);
    cinfo.offX = cinfo.offY = cinfo.offZ = 0;
}

static void c_angle(ItemObj* L, int angleDelta)
{
    uint16 a = (uint16)(L->angleY + angleDelta);
    olLaraMoveAngle = a;
    CollisionInfo_setAngle(&cinfo, a);
}

static int c_checkCeiling(ItemObj* L)
{
    if (cinfo.type != CT_CEILING && cinfo.type != CT_FLOOR_CEILING) return 0;

    animSet(L, LARA_ANIM_STAND, 1, 0);
    L->goalState = L->state;
    L->hSpeed = 0;
    L->vSpeed = 0;
    L->flags2 &= (uint8)~ITEM_FLAG_GRAVITY;
    L->x = (int16)cinfo.posX;
    L->y = (int16)cinfo.posY;
    L->z = (int16)cinfo.posZ;
    return 1;
}

#ifdef OL_FX3
/* angleDec(angle.z, ANGLE(2)): her lean eased off by a side wall */
static void leanEase(void)
{
    if (olLaraAngleZ > 0) { olLaraAngleZ -= OL_SPEED(ANGLE_1 * 2); if (olLaraAngleZ < 0) olLaraAngleZ = 0; }
    else if (olLaraAngleZ < 0) { olLaraAngleZ += OL_SPEED(ANGLE_1 * 2); if (olLaraAngleZ > 0) olLaraAngleZ = 0; }
}
#endif

static int c_checkWall(ItemObj* L)
{
    if (cinfo.type == CT_FRONT || cinfo.type == CT_FRONT_CEILING)
    {
        c_applyOffset(L);
        L->goalState = STATE_STOP;
        L->hSpeed = 0;
        L->flags2 &= (uint8)~ITEM_FLAG_GRAVITY;
        return 1;
    }

    /* the turn away from the wall, and (FX3) the lean easing off */
    if (cinfo.type == CT_LEFT) {
        c_applyOffset(L);
        L->angleY = (uint16)(L->angleY + ANGLE_1 * 5);
#ifdef OL_FX3
        leanEase();
#endif
    } else if (cinfo.type == CT_RIGHT) {
        c_applyOffset(L);
        L->angleY = (uint16)(L->angleY - ANGLE_1 * 5);
#ifdef OL_FX3
        leanEase();
#endif
    }
    return 0;
}

static int c_checkSlide(ItemObj* L)
{
    int realAngle;
    unsigned int d;

    if (olLaraWaterState == WATER_STATE_WADE) return 0;
    if (cinfo.m.slantType != SLANT_HIGH) return 0;

    c_applyOffset(L);

    if (cinfo.slantX > 2) {
        realAngle = -ANGLE_90;
    } else if (cinfo.slantX < -2) {
        realAngle = ANGLE_90;
    } else if (cinfo.slantZ > 2) {
        realAngle = (int)ANGLE_180;
    } else {
        realAngle = 0;
    }

    /* upstream's abs(realAngle - angle.y) <= ANGLE_90, as the shorter way
       round the circle so a 16-bit difference cannot mislead it. */
    d = (uint16)(realAngle - (int)L->angleY);
    if (d > 0x8000) d = 0x10000 - d;

    if (d <= ANGLE_90) {
        if (L->state != STATE_SLIDE) animSet(L, LARA_ANIM_SLIDE_FORTH, 1, 0);
        olLaraMoveAngle = (uint16)realAngle;
        L->angleY = (uint16)realAngle;
    } else {
        if (L->state != STATE_SLIDE_BACK) animSet(L, LARA_ANIM_SLIDE_BACK, 1, 0);
        olLaraMoveAngle = (uint16)realAngle;
        L->angleY = (uint16)(realAngle + ANGLE_180);
    }
    return 1;
}

static int c_checkFall(ItemObj* L, int height, unsigned int fallAnim)
{
    if (olLaraWaterState == WATER_STATE_WADE) return 0;
    if (cinfo.m.floor <= height) return 0;

    animSet(L, fallAnim, 1, 0);
    L->vSpeed = 0;
    L->flags2 |= ITEM_FLAG_GRAVITY;
    return 1;
}

static int c_checkLanding(ItemObj* L)
{
    int y = L->y;
    int v = L->vSpeed;

    if (L->state == STATE_FAST_DIVE && v > OL_SPEED(133)) {
        Lara_hit(L, LARA_MAX_HEALTH);
        return 1;
    }

    L->y = (int16)(L->y + cinfo.m.floor);
    L->roomFloor = L->y;

    checkTrigger(cinfo.trigger);

    L->y = (int16)y;

    if (v <= 140) return 0;

    if (v > 154) {
        Lara_hit(L, LARA_MAX_HEALTH);
    } else {
        int32 d = (int32)(v - 140);
        Lara_hit(L, (int)((d * d * LARA_MAX_HEALTH) / 196));
    }

    return L->health <= 0;
}

/* ---------------------------------------------------------------------------
 * bool Lara::alignAngle(int16 &angle, int16 threshold)
 *
 * Upstream's, including its last branch, which is true for every angle and
 * so snaps anything not within the threshold of 0 or +-90 to 180 -- and its
 * return, which after that snap is always false. Kept as it is: the climb
 * and the hang both call it, and what they do next depends on its answer.
 * ------------------------------------------------------------------------ */
static int alignAngle(int* angle, int threshold)
{
    int a = (int)(int16)*angle;

    if (a >= -threshold && a <= threshold) {
        a = 0;
    } else if (a >= ANGLE_90 - threshold && a <= ANGLE_90 + threshold) {
        a = ANGLE_90;
    } else if (a >= -ANGLE_90 - threshold && a <= -ANGLE_90 + threshold) {
        a = -ANGLE_90;
    } else {
        a = (int)ANGLE_180;
    }

    *angle = a;
    return (a & (ANGLE_90 - 1)) > 0;
}

/* void Lara::alignWall(int32 radius): her radius off the wall she faces. */
void Lara_alignWall(ItemObj* L, int radius)
{
    int x = L->x & ~1023;
    int z = L->z & ~1023;

    switch (L->angleY)
    {
        case ANGLE_0:            L->z = (int16)(z + 1024 - radius); break;
        case ANGLE_90:           L->x = (int16)(x + 1024 - radius); break;
        case (uint16)-ANGLE_90:  L->x = (int16)(x + radius); break;
        case ANGLE_180:          L->z = (int16)(z + radius); break;
        default: break;
    }
}

/* The box of her current animation, which is what stands in for upstream's
   getBoundingBox(true) here: the packer unions the frames' boxes over the
   animation, so a hang measures against the animation's highest hand rather
   than the frame's. Index 2 is minY. */
#define LARA_BOX_MIN_Y(L)   olAnimBox((L)->animIndex, 2)

/* bool Lara::c_checkGrab(): true when she cannot grab -- occupied hands,
   no Action, nothing in front, or a ledge too slanted. */
static int c_checkGrab(unsigned int input)
{
    int d;
    if (olLaraWeaponState != WEAPON_STATE_FREE) return 1;
    if (!(input & IN_ACTION)) return 1;
    if (cinfo.type != CT_FRONT) return 1;
    d = cinfo.r.floor - cinfo.l.floor;
    return X_ABS(d) >= LARA_HANG_SLANT;
}

/* bool Lara::c_checkSpace(): true when there is no room to stand up there. */
static int c_checkSpace(void)
{
    return (cinfo.f.floor < cinfo.f.ceiling ||
            cinfo.l.floor < cinfo.l.ceiling ||
            cinfo.r.floor < cinfo.r.ceiling);
}

/* bool Lara::c_checkClimbUp(): Up+Action at a ledge. Two or three clicks
   she climbs; up to seven and a half she jumps for, with the launch speed
   worked out here and spent by the animation's JUMP command. */
static int c_checkClimbUp(ItemObj* L, unsigned int input)
{
    int realAngle;

    if (cinfo.f.floor == WALL) return 0;
    if (c_checkGrab(input)) return 0;

    realAngle = L->angleY;
    if (alignAngle(&realAngle, ANGLE_1 * 30)) return 0;

    if (cinfo.f.floor >= -640 && cinfo.f.floor <= -384) {
        if (c_checkSpace()) return 0;
        olLaraWeaponState = WEAPON_STATE_BUSY;
        animSet(L, LARA_ANIM_CLIMB_2, 1, 0);
        L->state = STATE_HANG_UP;
        L->y = (int16)(L->y + 512 + cinfo.f.floor);
    } else if (cinfo.f.floor >= -896 && cinfo.f.floor <= -640) {
        if (c_checkSpace()) return 0;
        olLaraWeaponState = WEAPON_STATE_BUSY;
        animSet(L, LARA_ANIM_CLIMB_3, 1, 0);
        L->state = STATE_HANG_UP;
        L->y = (int16)(L->y + 768 + cinfo.f.floor);
    } else if (cinfo.f.floor >= -1920 && cinfo.f.floor <= -896) {
        animSet(L, LARA_ANIM_STAND, 1, 0);
        L->goalState = STATE_JUMP_UP;
        olLaraVSpeedHack = (int)phd_sqrt((uint32)(-2 * LARA_GRAVITY * (cinfo.f.floor + 800))) + 3;
        ItemObj_animProcess(L);
    } else {
        return 0;
    }

    L->angleY = (uint16)realAngle;
    c_applyOffset(L);
    return 1;
}

/* bool Lara::c_checkSwing(): whether a running grab has a drop behind the
   ledge to swing over, which picks the swinging hang animation. */
static int c_checkSwing(ItemObj* L)
{
    RoomQuery q;
    int x = L->x, y = L->y, z = L->z;
    int floor, ceiling;

    switch (L->angleY) {
        case ANGLE_0:           z += 256; break;
        case ANGLE_90:          x += 256; break;
        case (uint16)-ANGLE_90: x -= 256; break;
        case ANGLE_180:         z -= 256; break;
        default: break;
    }

    if (!Room_query(L->room, x, y, z, &q)) return 0;
    if (q.floor == OL_NO_FLOOR) return 0;

    floor   = q.floor - y;
    ceiling = q.ceiling - y;
    return (floor > 0 && ceiling < -400);
}

/* bool Lara::c_checkHang(): a jump's hands meet a ledge. */
static int c_checkHang(ItemObj* L, unsigned int input)
{
    int h, v, a;

    if (c_checkGrab(input)) return 0;

    if (cinfo.f.ceiling > 0) return 0;
    if (cinfo.m.ceiling > -LARA_STEP_HEIGHT) return 0;
    if (cinfo.m.floor < 200 && L->state == STATE_REACH) return 0;

    h = cinfo.f.floor - LARA_BOX_MIN_Y(L);
    v = h + L->vSpeed;

    if (h < 0 && v < 0) return 0;
    if (h > 0 && v > 0) return 0;

    a = L->angleY;
    if (alignAngle(&a, ANGLE_1 * 35)) return 0;
    L->angleY = (uint16)a;

    if (L->state == STATE_REACH) {
        if (c_checkSwing(L)) {
            animSet(L, LARA_ANIM_HANG_SWING, 1, 0);
        } else {
            animSet(L, LARA_ANIM_HANG, 1, 0);
        }
    } else {
        animSet(L, LARA_ANIM_HANG, 1, 12);
    }

    olLaraWeaponState = WEAPON_STATE_BUSY;
    cinfo.offY = cinfo.f.floor - LARA_BOX_MIN_Y(L);
    c_applyOffset(L);

    L->flags2 &= (uint8)~ITEM_FLAG_GRAVITY;
    L->hSpeed = 0;
    L->vSpeed = 0;
    return 1;
}

static void c_default(ItemObj* L)
{
    cinfo.gapPos      = LARA_STEP_HEIGHT;
    cinfo.gapNeg      = -LARA_STEP_HEIGHT;
    cinfo.gapCeiling  = 0;
    cinfo.stopOnSlant = 1;

    ItemObj_collideRoom(L, &cinfo, LARA_HEIGHT, 0);
}

static void c_step(ItemObj* L)
{
    cinfo.gapPos      = (olLaraWaterState == WATER_STATE_WADE) ? -WALL : 128;
    cinfo.gapNeg      = -128;
    cinfo.gapCeiling  = 0;
    cinfo.stopOnSlant = 1;

    ItemObj_collideRoom(L, &cinfo, LARA_HEIGHT, 0);

    if (c_checkCeiling(L)) return;

    if (c_checkWall(L)) {
        animSet(L, LARA_ANIM_STAND, 1, 0);
    }

    if (c_checkSlide(L)) return;

    L->y = (int16)(L->y + cinfo.m.floor);
}

static void c_fall(ItemObj* L, unsigned int input)
{
    if (L->vSpeed <= 0 || cinfo.m.floor > 0) return;

    if (c_checkLanding(L)) {
        L->goalState = STATE_DEATH;
    } else if (L->state == STATE_JUMP && (input & IN_UP) && !(input & IN_WALK)) {
        L->goalState = STATE_RUN;
    } else if (L->state == STATE_FALL) {
        animSet(L, LARA_ANIM_LANDING, 1, 0);
    } else {
        L->goalState = STATE_STOP;
    }
    Sound_stop(SND_SCREAM);

    L->y = (int16)(L->y + cinfo.m.floor);
    L->vSpeed = 0;
    L->flags2 &= (uint8)~ITEM_FLAG_GRAVITY;

    /* Upstream's animProcess() moves as well as advances. */
    if (L->state == STATE_JUMP) {
        ItemObj_animProcess(L);
        ItemObj_move(L);
    }
}

static void c_jump(ItemObj* L, unsigned int input)
{
    int slide;

    cinfo.gapPos     = -WALL;
    cinfo.gapNeg     = (L->state == STATE_REACH) ? 0 : -LARA_STEP_HEIGHT;
    cinfo.gapCeiling = 192;

    ItemObj_collideRoom(L, &cinfo,
                        (L->state == STATE_JUMP_UP) ? LARA_HEIGHT_JUMP : LARA_HEIGHT, 0);

    if (L->state == STATE_REACH || L->state == STATE_JUMP_UP) {
        if (c_checkHang(L, input)) return;
    }

    c_applyOffset(L);

    slide = (L->state == STATE_FALL) || (L->state == STATE_REACH) ||
            (L->state == STATE_JUMP_UP);

    if (cinfo.type == CT_CEILING || (slide && cinfo.type == CT_FRONT_CEILING))
    {
        if (L->vSpeed <= 0) L->vSpeed = 1;
    }
    else if (!slide && (cinfo.type == CT_FRONT || cinfo.type == CT_FRONT_CEILING))
    {
        animSet(L, LARA_ANIM_SMASH_JUMP, 1, 1);
        olLaraMoveAngle = (uint16)(olLaraMoveAngle + ANGLE_180);
        L->hSpeed = (int16)(L->hSpeed >> 2);
        if (L->vSpeed <= 0) L->vSpeed = 1;
    }
    else if (cinfo.type == CT_FLOOR_CEILING)
    {
        int32 s = phd_sin((int32)cinfo.angle);
        int32 c = phd_cos((int32)cinfo.angle);
        L->x = (int16)(L->x - (int)((s * LARA_RADIUS) >> FIXED_SHIFT));
        L->z = (int16)(L->z - (int)((c * LARA_RADIUS) >> FIXED_SHIFT));
        cinfo.m.floor = 0;
        L->hSpeed = 0;
        if (L->vSpeed <= 0) L->vSpeed = 16;
    }
    else if (cinfo.type == CT_LEFT)
    {
        L->angleY = (uint16)(L->angleY + ANGLE_1 * 5);
    }
    else if (cinfo.type == CT_RIGHT)
    {
        L->angleY = (uint16)(L->angleY - ANGLE_1 * 5);
    }

    c_fall(L, input);
}

static void c_slide(ItemObj* L)
{
    cinfo.gapPos      = -WALL;
    cinfo.gapNeg      = -512;
    cinfo.gapCeiling  = 0;
    cinfo.stopOnSlant = 1;

    ItemObj_collideRoom(L, &cinfo, LARA_HEIGHT, 0);

    if (c_checkCeiling(L)) return;

    c_checkWall(L);

    if (c_checkFall(L, 200, (L->state == STATE_SLIDE) ? LARA_ANIM_FALL_FORTH
                                                      : LARA_ANIM_FALL_BACK))
        return;

    c_checkSlide(L);

    L->y = (int16)(L->y + cinfo.m.floor);
}

static void c_roll(ItemObj* L)
{
    L->vSpeed = 0;
    L->flags2 &= (uint8)~ITEM_FLAG_GRAVITY;

    cinfo.gapPos      = -WALL;
    cinfo.gapNeg      = -LARA_STEP_HEIGHT;
    cinfo.gapCeiling  = 0;
    cinfo.stopOnSlant = 1;

    ItemObj_collideRoom(L, &cinfo, LARA_HEIGHT, 0);

    if (c_checkCeiling(L)) return;
    if (c_checkSlide(L)) return;

    if (c_checkFall(L, 200, (L->state == STATE_ROLL_START) ? LARA_ANIM_FALL_FORTH
                                                           : LARA_ANIM_FALL_BACK))
        return;

    c_applyOffset(L);

    L->y = (int16)(L->y + cinfo.m.floor);
}

/* ---------------------------------------------------------------------------
 * The water: upstream's c_checkWallUW, c_checkWallSurf, c_swim, c_surf and
 * c_checkWaterOut. Under water the probe is her swimming radius and height
 * about her middle; at the surface it reaches 700 down from where she
 * floats, which is how the front probe finds a rim she can climb. The
 * climb out is the one place the water reads the probe's room rather than
 * hers: collideRoom carries the room the middle point resolved to -- the
 * dry room over the pool -- into the front point, as TR's GetCollisionInfo
 * does, and that is what puts the rim under the front probe when her own
 * room's sector there is the pool's wall.
 * ------------------------------------------------------------------------ */

/* bool Lara::c_checkWallUW() */
static int c_checkWallUW(ItemObj* L)
{
    int level, depth;

    if (cinfo.type == CT_FRONT) {
        if (olLaraAngleX > 35 * ANGLE_1) {
            olLaraAngleX += 2 * ANGLE_1;
        } else if (olLaraAngleX < -35 * ANGLE_1) {
            olLaraAngleX -= 2 * ANGLE_1;
        } else {
            L->vSpeed = 0;
        }
    } else if (cinfo.type == CT_CEILING) {
        if (olLaraAngleX >= -45 * ANGLE_1) olLaraAngleX -= 2 * ANGLE_1;
    } else if (cinfo.type == CT_FRONT_CEILING) {
        L->vSpeed = 0;
    } else if (cinfo.type == CT_LEFT) {
        L->angleY = (uint16)(L->angleY + 5 * ANGLE_1);
    } else if (cinfo.type == CT_RIGHT) {
        L->angleY = (uint16)(L->angleY - 5 * ANGLE_1);
    } else if (cinfo.type == CT_FLOOR_CEILING) {
        L->x = (int16)cinfo.posX;
        L->y = (int16)cinfo.posY;
        L->z = (int16)cinfo.posZ;
        L->vSpeed = 0;
        return 1;
    }

    /* The floor under her: up onto it, and her nose up a little. */
    if (cinfo.m.floor < 0) {
        L->y = (int16)(L->y + cinfo.m.floor);
        olLaraAngleX += 2 * ANGLE_1;
    }

    /* A column with no water in it is nowhere to swim. Upstream's next arm,
       `depth <= 512`, stands her up wading on TR2's ANIM_SWIM_STAND; TR1
       has no such animation and swims on, so it is not here. */
    Room_water(L->room, L->x, L->z, &level, &depth);
    if (depth == WALL) {
        L->vSpeed = 0;
        L->x = (int16)cinfo.posX;
        L->y = (int16)cinfo.posY;
        L->z = (int16)cinfo.posZ;
    }

    return 0;
}

/* bool Lara::c_checkWallSurf() */
static void c_checkWallSurf(ItemObj* L)
{
    int stop = 0;

    if (cinfo.m.floor < 0 && cinfo.m.slantType == SLANT_HIGH) stop = 1;
    if (cinfo.type & (CT_FRONT | CT_CEILING | CT_FRONT_CEILING | CT_FLOOR_CEILING)) stop = 1;

    if (stop) {
        L->x = (int16)cinfo.posX;
        L->y = (int16)cinfo.posY;
        L->z = (int16)cinfo.posZ;
        L->vSpeed = 0;
    } else if (cinfo.type == CT_LEFT) {
        L->angleY = (uint16)(L->angleY + 5 * ANGLE_1);
    } else if (cinfo.type == CT_RIGHT) {
        L->angleY = (uint16)(L->angleY - 5 * ANGLE_1);
    }
}

/* void Lara::c_swim() */
static void c_swim(ItemObj* L)
{
    c_angle(L, ANGLE_0);

    ItemObj_collideRoom(L, &cinfo, LARA_HEIGHT_UW, LARA_HEIGHT_UW / 2);

    c_applyOffset(L);

    c_checkWallUW(L);
}

/* void Lara::c_surf() */
static void c_surf(ItemObj* L)
{
    int level, depth;

    ItemObj_collideRoom(L, &cinfo, LARA_HEIGHT_SURF + 100, LARA_HEIGHT_SURF);

    c_applyOffset(L);

    c_checkWallSurf(L);

    /* Pushed a hundred under, she is diving whether she asked to or not.
       What follows in upstream is TR2's wade-out and is commented out
       there for TR1. */
    Room_water(L->room, L->x, L->z, &level, &depth);
    if (level != WALL && level - L->y <= -100) Lara_dive(L);
}

/* bool Lara::c_checkWaterOut(): Action at a rim between two clicks below
   and a click and a quarter above the surface, square to it. */
static int c_checkWaterOut(ItemObj* L, unsigned int input)
{
    int h, a, d;

    if (!(input & IN_ACTION)) return 0;
    if (cinfo.type != CT_FRONT) return 0;
    if (cinfo.f.ceiling > 0) return 0;
    if (cinfo.m.ceiling > -LARA_STEP_HEIGHT) return 0;
    d = cinfo.r.floor - cinfo.l.floor;
    if (X_ABS(d) >= LARA_HANG_SLANT) return 0;

    h = cinfo.f.floor + LARA_HEIGHT_SURF;
    if (h <= -512 || h > 316) return 0;

    a = L->angleY;
    if (alignAngle(&a, 35 * ANGLE_1)) return 0;
    L->angleY = (uint16)a;

    L->y = (int16)(L->y + h - 5);

    ItemObj_updateRoom(L, -LARA_HEIGHT / 2);

    Lara_alignWall(L, -LARA_RADIUS);

    /* Upstream picks TR2's ANIM_SURF_OUT or ANIM_SURF_STAND by the height
       and then sets ANIM_WATER_OUT over it regardless; TR1 has the one. */
    animSet(L, LARA_ANIM_WATER_OUT, 1, 0);
    olLaraWeaponState = WEAPON_STATE_BUSY;

    olLaraWaterState = WATER_STATE_ABOVE;
    L->goalState = STATE_STOP;
    olLaraAngleX = 0;
#ifdef OL_FX3
    olLaraAngleZ = 0;
#endif
    L->hSpeed = 0;
    L->vSpeed = 0;
    L->flags2 &= (uint8)~ITEM_FLAG_GRAVITY;
    return 1;
}

/* C_HANDLER( STATE_DEATH_UW ): she floats up, dead. */
static void c_STATE_DEATH_UW(ItemObj* L)
{
    int level, depth;

    L->health = 0;
    olLaraOxygen = 0;

    Room_water(L->room, L->x, L->z, &level, &depth);
    if (level != WALL && level < L->y - LARA_RADIUS) {
        L->y = (int16)(L->y - LARA_FLOAT_UP_SPEED);
    }

    c_swim(L);
}

/* ---------------------------------------------------------------------------
 * The handlers, upstream's C_HANDLERs.
 * ------------------------------------------------------------------------ */

/* ---------------------------------------------------------------------------
 * void Lara::c_hang(int32 angleDelta)
 *
 * Hanging, and shimmying along the ledge when angleDelta is +-90. Two
 * probes: one the way she is moving, to know whether the ledge continues,
 * and one straight at the wall, two units in, to keep her against it.
 * ------------------------------------------------------------------------ */
static void c_hang(ItemObj* L, unsigned int input, int angleDelta)
{
    int noFloor, h, d;

    c_angle(L, angleDelta);
    cinfo.gapPos     = -WALL;
    cinfo.gapNeg     = WALL;
    cinfo.gapCeiling = 0;
    ItemObj_collideRoom(L, &cinfo, LARA_HEIGHT, 0);

    noFloor = cinfo.f.floor < 200;

    c_angle(L, ANGLE_0);
    cinfo.gapPos     = -WALL;
    cinfo.gapNeg     = -LARA_STEP_HEIGHT;
    cinfo.gapCeiling = 0;

    switch (cinfo.quadrant)
    {
        case 0:  L->z = (int16)(L->z + 2); break;
        case 1:  L->x = (int16)(L->x + 2); break;
        case 2:  L->z = (int16)(L->z - 2); break;
        default: L->x = (int16)(L->x - 2); break;
    }

    ItemObj_collideRoom(L, &cinfo, LARA_HEIGHT, 0);

    olLaraMoveAngle = (uint16)(L->angleY + angleDelta);

    if (L->health <= 0 || (input & IN_ACTION) == 0)  /* not || !: tools/tcclint.py */
    {
        animSet(L, LARA_ANIM_FALL_HANG, 1, 9);

        cinfo.offY = cinfo.f.floor - LARA_BOX_MIN_Y(L) + 2;
        c_applyOffset(L);

        L->hSpeed = 2;
        L->vSpeed = 1;
        L->flags2 |= ITEM_FLAG_GRAVITY;
        olLaraWeaponState = WEAPON_STATE_FREE;
        return;
    }

    L->vSpeed = 0;
    L->flags2 &= (uint8)~ITEM_FLAG_GRAVITY;

    d = cinfo.r.floor - cinfo.l.floor;
    if (noFloor || cinfo.type != CT_FRONT || cinfo.m.ceiling >= 0 ||
        X_ABS(d) >= LARA_HANG_SLANT)
    {
        if (L->state != STATE_HANG) {
            animSet(L, LARA_ANIM_HANG, 1, 21);
        }
        L->x = (int16)cinfo.posX;
        L->y = (int16)cinfo.posY;
        L->z = (int16)cinfo.posZ;
        return;
    }

    if (cinfo.quadrant & 1) {
        L->x = (int16)(L->x + cinfo.offX);
    } else {
        L->z = (int16)(L->z + cinfo.offZ);
    }

    h = cinfo.f.floor - LARA_BOX_MIN_Y(L);
    if (X_ABS(h) <= 256) {
        L->y = (int16)(L->y + h);
    }
}

static void c_STATE_WALK(ItemObj* L, unsigned int input)
{
    unsigned int a = L->animIndex - laraAnimBase;
    unsigned int f = L->frameIndex;

    L->vSpeed = 0;
    L->flags2 &= (uint8)~ITEM_FLAG_GRAVITY;
    cinfo.stopOnLava = 1;

    c_angle(L, ANGLE_0);
    c_default(L);

    if (c_checkCeiling(L)) return;
    if (c_checkClimbUp(L, input)) return;

    if (c_checkWall(L))
    {
        /* upstream: 29..47 right, 22..28 and 48..57 left, on the walk */
        if (a == LARA_ANIM_WALK && f >= 7 && f <= 25) {
            animSet(L, LARA_ANIM_STAND_RIGHT, 0, 0);
        } else if (a == LARA_ANIM_WALK && (f <= 6 || (f >= 26 && f <= 35))) {
            animSet(L, LARA_ANIM_STAND_LEFT, 0, 0);
        } else {
            animSet(L, LARA_ANIM_STAND, 0, 0);
        }
    }

    if (c_checkFall(L, LARA_STEP_HEIGHT, LARA_ANIM_FALL_FORTH)) return;

    /* descend: upstream's 28..45 on the walk */
    if (cinfo.m.floor > 128)
    {
        if (a == LARA_ANIM_WALK && f >= 6 && f <= 23) {
            animSet(L, LARA_ANIM_WALK_DESCEND_RIGHT, 0, 0);
        } else {
            animSet(L, LARA_ANIM_WALK_DESCEND_LEFT, 0, 0);
        }
    }

    /* ascend: upstream's 27..44 on the walk */
    if (cinfo.m.floor >= -LARA_STEP_HEIGHT && cinfo.m.floor < -128)
    {
        if (a == LARA_ANIM_WALK && f >= 5 && f <= 22) {
            animSet(L, LARA_ANIM_WALK_ASCEND_RIGHT, 0, 0);
        } else {
            animSet(L, LARA_ANIM_WALK_ASCEND_LEFT, 0, 0);
        }
    }

    if (c_checkSlide(L)) return;

    L->y = (int16)(L->y + cinfo.m.floor);
}

static void c_STATE_RUN(ItemObj* L, unsigned int input)
{
    unsigned int a = L->animIndex - laraAnimBase;
    unsigned int f = L->frameIndex;

    c_angle(L, ANGLE_0);

    cinfo.gapPos      = -WALL;
    cinfo.gapNeg      = -LARA_STEP_HEIGHT;
    cinfo.gapCeiling  = 0;
    cinfo.stopOnSlant = 1;

    ItemObj_collideRoom(L, &cinfo, LARA_HEIGHT, 0);

    if (c_checkCeiling(L)) return;
    if (c_checkClimbUp(L, input)) return;

    if (c_checkWall(L))
    {
#ifdef OL_FX3
        olLaraAngleZ = 0;
#endif
        /* upstream: frameIndex < 22 is the run itself, < 10 its first half */
        if (cinfo.f.slantType == SLANT_NONE &&
            cinfo.f.floor < -LARA_SMASH_HEIGHT &&
            a == LARA_ANIM_RUN)
        {
            animSet(L, (f < 10) ? LARA_ANIM_SMASH_RUN_LEFT
                                : LARA_ANIM_SMASH_RUN_RIGHT, 0, 0);
            L->state = STATE_SPLAT;
            return;
        }

        animSet(L, LARA_ANIM_STAND, 1, 0);
    }

    if (c_checkFall(L, LARA_STEP_HEIGHT, LARA_ANIM_FALL_FORTH)) return;

    /* ascend: upstream's 3..14 on the run */
    if (cinfo.m.floor >= -LARA_STEP_HEIGHT && cinfo.m.floor < -128)
    {
        if (a == LARA_ANIM_RUN && f >= 3 && f <= 14) {
            animSet(L, LARA_ANIM_RUN_ASCEND_RIGHT, 0, 0);
        } else {
            animSet(L, LARA_ANIM_RUN_ASCEND_LEFT, 0, 0);
        }
    }

    if (c_checkSlide(L)) return;

    if (cinfo.m.floor >= 50) {
        L->y = (int16)(L->y + 50);
        return;
    }

    L->y = (int16)(L->y + cinfo.m.floor);
}

static void c_STATE_STOP(ItemObj* L, unsigned int input)
{
    L->vSpeed = 0;
    L->flags2 &= (uint8)~ITEM_FLAG_GRAVITY;

    c_angle(L, ANGLE_0);

    /* Four units forward on Up+Action alone, so the front probe reaches the
       ledge she is standing against; upstream's climb-up check wants it. */
    if (input == (IN_UP | IN_ACTION))
    {
        int32 sn = phd_sin((int32)cinfo.angle);
        int32 cs = phd_cos((int32)cinfo.angle);
        L->x = (int16)(L->x + (int)((sn * 4) >> FIXED_SHIFT));
        L->z = (int16)(L->z + (int)((cs * 4) >> FIXED_SHIFT));
    }

    c_default(L);

    if (c_checkClimbUp(L, input)) return;
    if (c_checkFall(L, 100, LARA_ANIM_FALL_FORTH)) return;
    if (c_checkSlide(L)) return;

    c_applyOffset(L);

    L->y = (int16)(L->y + cinfo.m.floor);
}

static void c_STATE_HANG(ItemObj* L, unsigned int input)
{
    c_hang(L, input, 0);

    if ((input & IN_UP) && L->goalState == STATE_HANG)
    {
        if (X_ABS(cinfo.f.floor) >= 850) return;
        if (c_checkSpace()) return;
        if (cinfo.staticHit) return;

        L->goalState = (input & IN_WALK) ? STATE_HANDSTAND : STATE_HANG_UP;
    }
}

static void c_STATE_REACH(ItemObj* L, unsigned int input)
{
    L->flags2 |= ITEM_FLAG_GRAVITY;
    c_angle(L, ANGLE_0);
    c_jump(L, input);
}

static void c_STATE_HANG_UP(ItemObj* L)
{
    c_angle(L, ANGLE_0);
    c_default(L);
}

static void c_STATE_BACK_FAST(ItemObj* L)
{
    L->vSpeed = 0;
    L->flags2 &= (uint8)~ITEM_FLAG_GRAVITY;

    c_angle(L, (int)ANGLE_180);

    cinfo.gapPos      = -WALL;
    cinfo.gapNeg      = -LARA_STEP_HEIGHT;
    cinfo.gapCeiling  = 0;
    cinfo.stopOnSlant = 1;

    ItemObj_collideRoom(L, &cinfo, LARA_HEIGHT, 0);

    if (c_checkCeiling(L)) return;
    if (c_checkFall(L, 200, LARA_ANIM_FALL_BACK)) return;

    if (c_checkWall(L)) {
        animSet(L, LARA_ANIM_STAND, 0, 0);
    }

    L->y = (int16)(L->y + cinfo.m.floor);
}

static void c_STATE_TURN(ItemObj* L)
{
    c_angle(L, ANGLE_0);
    c_default(L);

    if (c_checkFall(L, 100, LARA_ANIM_FALL_FORTH)) return;
    if (c_checkSlide(L)) return;

    L->y = (int16)(L->y + cinfo.m.floor);
}

static void c_STATE_DEATH(ItemObj* L)
{
    cinfo.radius = LARA_RADIUS * 4;

    c_angle(L, ANGLE_0);
    c_default(L);

    c_applyOffset(L);
    L->y = (int16)(L->y + cinfo.m.floor);
}

static void c_STATE_SPLAT(ItemObj* L)
{
    c_angle(L, ANGLE_0);
    c_default(L);
    c_applyOffset(L);
}

static void c_STATE_COMPRESS(ItemObj* L)
{
    L->vSpeed = 0;
    L->flags2 &= (uint8)~ITEM_FLAG_GRAVITY;

    cinfo.gapPos     = -WALL;
    cinfo.gapNeg     = WALL;
    cinfo.gapCeiling = 0;

    ItemObj_collideRoom(L, &cinfo, LARA_HEIGHT, 0);

    if (cinfo.m.ceiling > -100)
    {
        animSet(L, LARA_ANIM_STAND, 1, 0);
        L->x = (int16)cinfo.posX;
        L->y = (int16)cinfo.posY;
        L->z = (int16)cinfo.posZ;
        L->hSpeed = 0;
        L->vSpeed = 0;
    }
}

static void c_STATE_BACK(ItemObj* L)
{
    L->vSpeed = 0;
    L->flags2 &= (uint8)~ITEM_FLAG_GRAVITY;

    c_angle(L, (int)ANGLE_180);

    cinfo.gapPos      = (olLaraWaterState == WATER_STATE_WADE) ? -WALL
                                                               : LARA_STEP_HEIGHT;
    cinfo.gapNeg      = -LARA_STEP_HEIGHT;
    cinfo.gapCeiling  = 0;
    cinfo.stopOnSlant = 1;

    ItemObj_collideRoom(L, &cinfo, LARA_HEIGHT, 0);

    if (c_checkCeiling(L)) return;

    if (c_checkWall(L)) {
        animSet(L, LARA_ANIM_STAND, 1, 0);
    }

    if (cinfo.m.floor > 128 && cinfo.m.floor < LARA_STEP_HEIGHT)
    {
        /* Upstream picks left below global frame 568; the back-walk starts
           at 598, so the right-foot descend is what it always chooses. */
        animSet(L, LARA_ANIM_BACK_DESCEND_RIGHT, 0, 0);
    }

    if (c_checkSlide(L)) return;

    L->y = (int16)(L->y + cinfo.m.floor);
}

void Lara_updateCollision(ItemObj* L, unsigned int input)
{
    laraAnimBase = olModelFirstAnim(L->model);

    switch (L->state)
    {
        case STATE_WALK:        c_STATE_WALK(L, input); break;
        case STATE_RUN:         c_STATE_RUN(L, input); break;
        case STATE_STOP:
        case STATE_POSE:
        case STATE_LAND:
        case STATE_TURN_FAST:   c_STATE_STOP(L, input); break;
        case STATE_HANG:        c_STATE_HANG(L, input); break;
        case STATE_REACH:       c_STATE_REACH(L, input); break;
        case STATE_HANDSTAND:
        case STATE_HANG_UP:     c_STATE_HANG_UP(L); break;
        case STATE_HANG_LEFT:   c_hang(L, input, -ANGLE_90); break;
        case STATE_HANG_RIGHT:  c_hang(L, input, ANGLE_90); break;
        case STATE_SWAN_DIVE:
        case STATE_FAST_DIVE:
        case STATE_JUMP:        c_angle(L, ANGLE_0);        c_jump(L, input); break;
        case STATE_BACK_FAST:   c_STATE_BACK_FAST(L); break;
        case STATE_TURN_RIGHT:
        case STATE_TURN_LEFT:   c_STATE_TURN(L); break;
        case STATE_DEATH:       c_STATE_DEATH(L); break;
        case STATE_FALL:        L->flags2 |= ITEM_FLAG_GRAVITY;
                                c_jump(L, input); break;
        case STATE_SPLAT:       c_STATE_SPLAT(L); break;
        case STATE_COMPRESS:    c_STATE_COMPRESS(L); break;
        case STATE_BACK:        c_STATE_BACK(L); break;
        case STATE_STEP_RIGHT:  c_angle(L, ANGLE_90);       c_step(L); break;
        case STATE_STEP_LEFT:   c_angle(L, -ANGLE_90);      c_step(L); break;
        case STATE_ROLL_END:    c_angle(L, (int)ANGLE_180); c_roll(L); break;
        case STATE_ROLL_START:  c_angle(L, ANGLE_0);        c_roll(L); break;
        case STATE_SLIDE:       c_angle(L, ANGLE_0);        c_slide(L); break;
        case STATE_SLIDE_BACK:  c_angle(L, (int)ANGLE_180); c_slide(L); break;
        case STATE_JUMP_BACK:   c_angle(L, (int)ANGLE_180); c_jump(L, input); break;
        case STATE_JUMP_RIGHT:  c_angle(L, ANGLE_90);       c_jump(L, input); break;
        case STATE_JUMP_LEFT:   c_angle(L, -ANGLE_90);      c_jump(L, input); break;
        case STATE_JUMP_UP:     c_angle(L, ANGLE_0);        c_jump(L, input); break;
        case STATE_FALL_BACK:   c_angle(L, (int)ANGLE_180); c_jump(L, input); break;
        case STATE_UW_TREAD:
        case STATE_UW_SWIM:
        case STATE_UW_GLIDE:
        case STATE_UW_DIVE:     c_swim(L); break;
        case STATE_DEATH_UW:    c_STATE_DEATH_UW(L); break;
        case STATE_SURF_TREAD:  c_angle(L, ANGLE_0);        c_surf(L); break;
        case STATE_SURF_SWIM:   cinfo.gapNeg = -LARA_STEP_HEIGHT;
                                c_angle(L, ANGLE_0);        c_surf(L);
                                c_checkWaterOut(L, input); break;
        case STATE_SURF_BACK:   c_angle(L, (int)ANGLE_180); c_surf(L); break;
        case STATE_SURF_LEFT:   c_angle(L, -ANGLE_90);      c_surf(L); break;
        case STATE_SURF_RIGHT:  c_angle(L, ANGLE_90);       c_surf(L); break;
        case STATE_WATER_OUT:   c_angle(L, ANGLE_0);        c_default(L); break;
        case STATE_BLOCK_PUSH:
        case STATE_BLOCK_PULL:
        case STATE_BLOCK_READY:
        case STATE_PICKUP:
        case STATE_SWITCH_DOWN:
        case STATE_SWITCH_UP:
        case STATE_USE_KEY:
        case STATE_USE_PUZZLE:  c_angle(L, ANGLE_0);        c_default(L); break;
        default: break;
    }
}
