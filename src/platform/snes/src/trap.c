/* OpenLara - SNES / Super FX target
 * The traps of src/fixed/object.h that LEVEL1 and LEVEL2 have: TrapDartEmitter
 * and Dart, TrapSwingBlade. FX3 only: the GSU-2 image has no resident room
 * for them, and keeps the emitter's open-and-shut.
 *
 * Upstream's emitter takes a dart from ItemObj's free list at every loop of
 * its FIRE animation. Every entity here has to exist from the load -- the
 * renderer's slot table and the host's item list are the packer's -- so the
 * packer appends two hidden darts per emitter (packer/out_SNES.h, the dart
 * pool) and ItemObj::add is "the first of them not in flight". A dart lives
 * up to 32 ticks in LEVEL1 (130 a tick, room 6's floor ramps up to meet it)
 * against the emitter's 30-tick loop: two each is never short where
 * upstream's list would not be.
 */

#include "object.h"
#include "enemy.h"
#include "room.h"
#include "lara.h"
#include "level.h"
#include "sound.h"
#include "collision.h"
#include "fx.h"

#define ITEM_DART               39
#define DART_EMITTER_IDLE       0
#define DART_EMITTER_FIRE       1
#define SND_DART                151
#define DART_DAMAGE             50

/* Darts in flight: Lara's collision looks for them only while there are any. */
unsigned int olDartsLive;

/* ItemObj::add(ITEM_DART, room, pos, angleY) and the Dart constructor. */
static ItemObj* Dart_add(unsigned int room, int x, int y, int z, uint16 angleY)
{
    unsigned int i;
    ItemObj* dart;

    for (i = 1; i < itemsCount; i++) {
        dart = items + i;
        if (dart->type != ITEM_DART) continue;
        if (ItemObj_status(dart) != ITEM_STATUS_INVISIBLE) continue;

        dart->room   = (uint8)room;
        dart->x      = (int16)x;
        dart->y      = (int16)y;
        dart->z      = (int16)z;
        dart->angleY = angleY;
        dart->hSpeed = 0;
        dart->vSpeed = 0;
        dart->flags2 &= (uint8)~ITEM_FLAG_GRAVITY;
        ItemObj_animSet(dart, olModelFirstAnim(dart->model));
        ItemObj_setStatus(dart, ITEM_STATUS_ACTIVE);
        ItemObj_activate(dart);
        ItemObj_touch(dart);
        olDartsLive++;
        return dart;
    }
    return 0;       /* upstream's ASSERT and NULL: nothing fired */
}

/* ItemObj::remove: off the active list and out of the picture. */
static void Dart_remove(ItemObj* dart)
{
    ItemObj_deactivate(dart);
    ItemObj_setStatus(dart, ITEM_STATUS_INVISIBLE);
    ItemObj_touch(dart);
    if (olDartsLive) olDartsLive--;
}

/* TrapDartEmitter::update. The FIRE test is before animProcess, so it reads
   the frame last tick's processing left: the first of the loop. */
void DartEmitter_update(ItemObj* item)
{
    item->goalState = ItemObj_isActive(item) ? DART_EMITTER_FIRE : DART_EMITTER_IDLE;

    if (item->state == DART_EMITTER_IDLE && item->goalState == DART_EMITTER_IDLE) {
        ItemObj_deactivate(item);
        return;
    }

    if (item->state == DART_EMITTER_FIRE && item->frameIndex == 0) {
        /* getBlockOffset(angle.y, 412), then 512 up */
        int x = item->x, z = item->z;
        if (item->angleY == ANGLE_0)                 z -= 412;
        else if (item->angleY == (uint16)ANGLE_180)  z += 412;
        else if (item->angleY == ANGLE_90)           x -= 412;
        else                                         x += 412;

        if (Dart_add(item->room, x, item->y - 512, z, item->angleY)) {
            Sound_playItem(SND_DART, item);
            Fx_smoke(item->room, x, item->y - 512, z);
        }
    }

    ItemObj_animProcess(item);
}

/* Dart::update: along on its animation's speed until the floor under it is
   at or above it. A wall's floor is upstream's WALL, which the host table
   reports as OL_NO_FLOOR. */
void Dart_update(ItemObj* dart)
{
    int floor;

    ItemObj_animProcess(dart);
    ItemObj_move(dart);
    ItemObj_updateRoom(dart, 0);

    floor = (dart->roomFloor == OL_NO_FLOOR) ? WALL : dart->roomFloor;
    if (dart->y < floor) return;

    Dart_remove(dart);
    Fx_ricochet(dart->room, dart->x, dart->y, dart->z, 0);
}

/* Dart::collide from Lara::updateObjectsCollision: her room's neighbours,
   4096 each way, then updateHitMask -- bounds as upstream, the current-frame
   boxes FX3's creature contact uses; the dart's sphere is not modelled. A
   hit takes 50 and leaves the dart flying, as upstream's does. */
void Dart_collideLara(ItemObj* lara)
{
    unsigned int i;

    if (lara->health <= 0) return;

    for (i = 1; i < itemsCount; i++) {
        ItemObj* dart = items + i;
        int d;
        if (dart->type != ITEM_DART) continue;
        if (ItemObj_status(dart) == ITEM_STATUS_INVISIBLE) continue;
        d = lara->x - dart->x; if (d <= -4096 || d >= 4096) continue;
        d = lara->y - dart->y; if (d <= -4096 || d >= 4096) continue;
        d = lara->z - dart->z; if (d <= -4096 || d >= 4096) continue;
        if (!Room_isAdjacent(lara->room, dart->room)) continue;
        if (!ItemObj_collideFrameBounds(dart, lara, cinfo.radius)) continue;

        Fx_blood(lara->room, dart->x, dart->y, dart->z, lara->angleY, lara->hSpeed);
        lara->health = (int16)(lara->health - DART_DAMAGE);
        if (lara->health < 0) lara->health = 0;
        if (lara->health == 0) return;
    }
}

/* ---------------------------------------------------------------------------
 * TrapSwingBlade. LEVEL2's three hang in room 61: STATIC, BEGIN (20 frames)
 * and a 70-frame SWING whose current-frame box sweeps 2,400 units.
 * ------------------------------------------------------------------------ */
#define BLADE_STATIC            0
#define BLADE_SWING             2
#define BLADE_DAMAGE            100

uint8 olBlades[OL_BLADE_MAX];
unsigned int olBladeCount;

void SwingBlade_update(ItemObj* item)
{
    if (ItemObj_isActive(item)) {
        if (item->state == BLADE_STATIC) item->goalState = BLADE_SWING;
    } else {
        if (item->state == BLADE_SWING) item->goalState = BLADE_STATIC;
    }
    ItemObj_animProcess(item);
}

/* ---------------------------------------------------------------------------
 * ItemObj::getSpheres and ::collideSpheres, which GBA/32X (FAST_HITMASK)
 * keep for the swinging blade alone: every mesh's sphere, carried through
 * the model's node chain on the frame getFrame picks, blade against Lara.
 * Both are native (src/trap_fx3.asm) on the packer's SPHERE.BIN, upstream's
 * own numbers in FX3 CPU ROM: Trap_sphereTest, the fast path (her yaw-0
 * centres turned by her yaw, within a bound the packer proved over every
 * frame and yaw), and Trap_collideSpheres, GBA's int32 matrix chain exactly,
 * for what the fast path cannot decide. The runner checks both against
 * upstream's code (tools/spherecheck.cpp). The C reference cartridge
 * (C_MOTION), which has no native units, leaves the blade on its bounds.
 * ------------------------------------------------------------------------ */
#ifdef OL_C_MOTION
#define OL_BLADE_SPHERES 0
#endif
#ifndef OL_BLADE_SPHERES
#define OL_BLADE_SPHERES 1
#endif

int32 olSpheres[2][16 * 4];                 /* x, y, z, radius; blade, Lara */
int16 olSphStack[8 * 18];                   /* the chain's matrices, 36 bytes each */
int16 olSphFast[83];                        /* trap_fx3.asm's scratch */
unsigned int olSphExact;                    /* exact-chain answers, for the runner */
#if OL_BLADE_SPHERES
extern unsigned int Trap_sphereTest(ItemObj* blade, ItemObj* lara);
extern uint32 Trap_collideSpheres(ItemObj* blade, ItemObj* lara);
#endif

/* TrapSwingBlade::collide from Lara::updateObjectsCollision: ACTIVE and
   mid-swing, her room's neighbours within 4096, then updateHitMask's bounds
   (current-frame boxes), then its spheres against hers, as GBA's does --
   the fast path's certain answer, else the exact chain. Then the blood, at
   her position offset by four rand_logic draws -- the creatures share that
   generator; upstream's draws are the arguments of one call, whose order
   C++ leaves open, taken here left to right -- and hit(100). */
void SwingBlade_collideLara(ItemObj* lara)
{
    unsigned int n;

    if (lara->health <= 0) return;      /* once, before the pass, as upstream */
    for (n = 0; n < olBladeCount; n++) {
        ItemObj* blade = items + olBlades[n];
        int d;
        if (ItemObj_status(blade) != ITEM_STATUS_ACTIVE) continue;
        if (blade->state != BLADE_SWING) continue;
        d = lara->x - blade->x; if (d <= -4096 || d >= 4096) continue;
        d = lara->y - blade->y; if (d <= -4096 || d >= 4096) continue;
        d = lara->z - blade->z; if (d <= -4096 || d >= 4096) continue;
        if (!Room_isAdjacent(lara->room, blade->room)) continue;
        if (!ItemObj_collideFrameBounds(blade, lara, cinfo.radius)) continue;
#if OL_BLADE_SPHERES
        {                                                   /* updateHitMask */
            unsigned int touch = Trap_sphereTest(blade, lara);
            if (touch == 2) {
                olSphExact++;
                touch = Trap_collideSpheres(blade, lara) != 0;
            }
            if (!touch) continue;
        }
#endif

        {
            int ox = (int)((rand_logic() - 0x4000) >> 8);
            int oy = -256 - (int)(rand_logic() >> 6);
            int oz = (int)((rand_logic() - 0x4000) >> 8);
            int oa = (int)((rand_logic() - 0x4000) >> 3);
            Fx_blood(lara->room, lara->x + ox, lara->y + oy, lara->z + oz,
                     (uint16)(lara->angleY + oa), lara->hSpeed);
        }
        if (lara->health > 0) {                 /* hit() ignores the dead */
            lara->health = (int16)(lara->health - BLADE_DAMAGE);
            if (lara->health < 0) lara->health = 0;
        }
    }
}
