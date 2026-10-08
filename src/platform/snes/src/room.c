/* OpenLara - SNES / Super FX target
 * The port of src/fixed/room.h's checkTrigger.
 *
 * Upstream walks TR's FloorData here -- a bytecode of portals, slants and
 * triggers interleaved in one stream. This side never sees it: the packer
 * walks it once and a sector carries the answer, an index into a table of
 * "standing here activates these entities". So checkTrigger takes a trigger
 * rather than a `const FloorData*`, and everything below that is upstream's.
 *
 * The other half of room.h -- Room_query, the portal walks and the slants --
 * is in src/room.asm, for the cycles. See there.
 */

#include "level.h"
#include "item.h"
#include "room.h"
#include "object.h"
#include "camera.h"
#ifdef OL_FX3
#include "lara.h"
#endif

/* Room_query, Room_reset and the caches behind them are in src/room.asm.
   The point resolution is a dozen byte reads, and at 2,400 cycles a call
   from this compiler it was a fifth of the tick; the assembly is the same
   algorithm statement for statement, at a fraction of the cost. What is
   left here is the trigger side, which is not hot. */

/* Upstream's nextLevel, and gSaveGame.secrets. */
unsigned int olLevelEnd;
unsigned int olSecrets;

/* TR's trigger types, upstream's enum. */
#define TRIGGER_TYPE_ACTIVATE   0
#define TRIGGER_TYPE_PAD        1
#define TRIGGER_TYPE_SWITCH     2
#define TRIGGER_TYPE_KEY        3
#define TRIGGER_TYPE_PICKUP     4
#define TRIGGER_TYPE_OBJECT     5
#define TRIGGER_TYPE_ANTIPAD    6
#define TRIGGER_TYPE_COMBAT     7
#define TRIGGER_TYPE_DUMMY      8

/* ---------------------------------------------------------------------------
 * The ACTIVATE_OBJECT arm of upstream's checkTrigger, which is the whole of
 * how a trigger latches. Split out because a switch fires the trigger of the
 * sector *it* stands in rather than the one under Lara, and the rules are the
 * same either way.
 * ------------------------------------------------------------------------ */
void fireTrigger(unsigned int t, unsigned int first)
{
    unsigned int j;
    unsigned int mask;

    if (t >= olTriggerCount) return;

    mask = olTrigMask(t);

    for (j = first; j < olTrigItemCount(t); j++)
    {
        unsigned int k = olTrigItem(t, j);
        uint8 f;

        if (k >= itemsCount) continue;          /* two tests, not || !: tools/tcclint.py */
        if (!(items[k].flags & ITEM_FLAG_DRIVEN)) continue;

        f = items[k].flags;
        if (f & ITEM_FLAG_ONCE) continue;

        /* Upstream sets the timer here, before it checks whether the item is
           already active -- which is the whole reason a pad keeps a dart
           emitter firing while you stand on it. Set it only on a fresh
           activation and the emitter fires once and stops. */
#ifdef OL_FX3
        /* Dead enemies reuse this word for GBA's separate corpseTimer.
           A repeated floor pad must not overwrite that countdown. */
        if (items[k].health <= 0) {
            if (ItemObj_class(items[k].type) == CLASS_ENEMY) goto keepCorpseTimer;
        }
#endif
        items[k].timer = (int16)olTrigTimer(t);
        if (items[k].timer != 1) items[k].timer *= 30;
#ifdef OL_FX3
keepCorpseTimer:
#endif

        /* A switch toggles what a pad sets and an antipad clears. This is why
           a trigger carries a mask at all: a door wired to two floor pads
           opens on the second one. */
        if (olTrigType(t) == TRIGGER_TYPE_SWITCH) {
            f ^= (uint8)mask;
        } else if (olTrigType(t) == TRIGGER_TYPE_ANTIPAD) {
            f &= (uint8)~mask;
        } else {
            f |= (uint8)mask;
        }

        items[k].flags = f;

        /* Upstream stops here when the mask is short, and in particular does
           *not* clear ITEM_FLAG_ACTIVE: that flag says the entity is on the
           update list, and an antipad switches something off by clearing mask
           bits rather than by taking it off the list. isActive reads the
           mask, which is what makes that work. */
        if ((f & ITEM_FLAG_MASK) != ITEM_FLAG_MASK) continue;

        if (olTrigOnce(t)) items[k].flags |= ITEM_FLAG_ONCE;

        if (items[k].flags & ITEM_FLAG_ACTIVE) continue;

        ItemObj_activate(items + k);

        /* Upstream only promotes NONE after activate really admitted it.
           Keep evicted actors invisible until Enemy_update acquires Nav,
           and never reveal a corpse whose activate rejected the trigger. */
#ifdef OL_FX3
        if (!(items[k].flags & ITEM_FLAG_ACTIVE)) continue;
        if (ItemObj_status(items + k) == ITEM_STATUS_NONE) {
#else
        if (ItemObj_status(items + k) == ITEM_STATUS_NONE ||
            ItemObj_status(items + k) == ITEM_STATUS_INVISIBLE) {
#endif
            ItemObj_setStatus(items + k, ITEM_STATUS_ACTIVE);
            ItemObj_touch(items + k);
        }
    }
}

/* ---------------------------------------------------------------------------
 * void checkTrigger(const FloorData* fd, ItemObj* lara)
 *
 * Upstream's switch on the trigger type, for Lara. A pad fires only under
 * her feet -- jumping over one does nothing, which is a fact TR levels are
 * built on. A switch, a keyhole and a pickup keep their trigger in the
 * sector they stand in and name themselves first in it, and that first
 * entity is *asked* rather than activated: has the lever been pulled, is
 * the key in, has the thing been taken. Only then do the rest fire.
 *
 * FX3 also accepts an OBJECT trigger from a landed block, with no Lara type
 * checks. Its camera actions are replayed by Camera_prepare. DUMMY does
 * nothing by definition; GSU-2 retains its smaller Lara-only trigger path.
 * ------------------------------------------------------------------------ */
#ifdef OL_FX3
static void Room_checkTrigger(unsigned int t, unsigned int object)
#else
void checkTrigger(unsigned int t)
#endif
{
    unsigned int first = 0;
    ItemObj* used = 0;
#ifdef OL_FX3
    unsigned int camEvent = CAM_NO_ITEM;
#endif

    if (t >= olTriggerCount) return;

#ifdef OL_FX3
    /* checkCamera runs whatever the type decides; the camera update replays
       it (see Camera_prepare in camera.c) */
    if (olTrigActions(t) & (OL_TRIGGER_CAMERA | OL_TRIGGER_TARGET))
        camEvent = object ? Camera_noteTriggerObject(t) : Camera_noteTrigger(t);

    if (object) {
        if (olTrigType(t) != TRIGGER_TYPE_OBJECT) return;
    } else
#endif

    switch (olTrigType(t))
    {
        case TRIGGER_TYPE_ACTIVATE:
            break;

        case TRIGGER_TYPE_PAD:
        case TRIGGER_TYPE_ANTIPAD:
            if (items[0].y != items[0].roomFloor) return;
            break;

#ifdef OL_FX3
        case TRIGGER_TYPE_COMBAT:
            if (olLaraWeaponState != WEAPON_STATE_READY) return;
            break;
#endif

        case TRIGGER_TYPE_SWITCH:
        case TRIGGER_TYPE_KEY:
        case TRIGGER_TYPE_PICKUP:
        {
            unsigned int k = olTrigItem(t, 0);
            if (olTrigItemCount(t) == 0 || k >= itemsCount) return;
            used = items + k;
            if (olTrigType(t) == TRIGGER_TYPE_SWITCH) {
                if (!Switch_use(used, (int)olTrigTimer(t))) return;
            } else if (olTrigType(t) == TRIGGER_TYPE_KEY) {
                if (!Key_use(used)) return;
            } else {
                if (!Pickup_use(used)) return;
            }
            first = 1;
            break;
        }

        default:
            return;
    }

    fireTrigger(t, first);
#ifdef OL_FX3
    if (camEvent != CAM_NO_ITEM)
        Camera_passTrigger(camEvent, olTrigType(t) == TRIGGER_TYPE_SWITCH && used && used->state == 1);
#endif

    /* The two actions besides ACTIVATE_OBJECT that mean something here.
       END is upstream's nextLevel: there is no next level to load, so it
       is a flag the shell will read when there is one. SECRET is
       gSaveGame.secrets, a bit per secret found. Cameras are FX3's (above);
       flip maps and the soundtrack are parsed past by the packer. */
    {
        unsigned int a = olTrigActions(t);
        if (a & OL_TRIGGER_END)    olLevelEnd = 1;
        if (a & OL_TRIGGER_SECRET) olSecrets |= (unsigned int)(1u << olTrigSecret(t));
    }
}

#ifdef OL_FX3
void checkTrigger(unsigned int t)
{
    Room_checkTrigger(t, 0);
}

/* Upstream's checkTrigger(fd, NULL), currently from Block::update. */
void checkTriggerObject(unsigned int t)
{
    Room_checkTrigger(t, 1);
}
#endif

/* ---------------------------------------------------------------------------
 * The water column: upstream's Room::getWaterSector with ItemObj's
 * getWaterLevel and getWaterDepth on top of it.
 *
 * From a water room the walk goes up through the vertical portals while the
 * room above is water too; from a dry room it goes down until one is. The
 * sector it stops on is the top of the water, and its ceiling byte is the
 * surface -- exactly, when the room above is air, since a vertical portal is
 * flat; with the slant when there is no room above, which is what upstream's
 * getCeiling computes and what Room_query answers from just inside it. The
 * floor is Room_query's too, slants and all, from the same call.
 *
 * The water-column walk stays in C. Room_sectorAt shares the cached native
 * sector lookup with Room_query and navigation; its C reference is below.
 * ------------------------------------------------------------------------ */

/* upstream's Room::getSector: the clamped host sector index of (x, z) in
   room r. The differences are taken in sixteen bits as room.asm takes them:
   the room origin is wrapped like every other position and the offset
   within the room is small. */
#ifdef OL_C_MOTION
unsigned int Room_sectorAt(unsigned int r, int x, int z)
{
    int xs, zs;
    int sx, sz, d;

    if (r >= olRoomCount) return 0xFFFF;
    xs = (int)olRoomXSectors(r);
    zs = (int)olRoomZSectors(r);
    if (!xs) return 0xFFFF;
    if (!zs) return 0xFFFF;
    d  = x - olRoomX(r);
    sx = (d < 0) ? 0 : (int)((unsigned int)d >> 10);
    if (sx >= xs) sx = xs - 1;

    d  = z - olRoomZ(r);
    sz = (d < 0) ? 0 : (int)((unsigned int)d >> 10);
    if (sz >= zs) sz = zs - 1;

    return olRoomSectorOff(r) + (unsigned int)(sx * zs) + (unsigned int)sz;
}
#endif

#if defined(OL_FX3) && !defined(OL_C_MOTION)
void Room_waterCold(unsigned int room, int x, int z, int* level, int* depth)
#else
void Room_water(unsigned int room, int x, int z, int* level, int* depth)
#endif
{
    RoomQuery q;
    unsigned int s, next;
    int c;

    *level = WALL;
    *depth = WALL;

    if (room >= olRoomCount) return;
    if (olRoomXSectors(room) == 0 || olRoomZSectors(room) == 0) return;

    s = Room_sectorAt(room, x, z);

    if (olRoomWater(room))
    {
        /* up to the air */
        for (;;) {
            next = olLinkAbove(olSectorLink(s));
            if (next == OL_NO_ROOM) break;
            if (!olRoomWater(next)) break;
            room = next;
            s = Room_sectorAt(room, x, z);
        }
    }
    else
    {
        /* down to the water */
        for (;;) {
            next = olLinkBelow(olSectorLink(s));
            if (next == OL_NO_ROOM) return;
            room = next;
            s = Room_sectorAt(room, x, z);
            if (olRoomWater(room)) break;
        }
    }

    c = olSectorCeiling(s) << 8;
    *level = c;

    if (!Room_query(room, x, c + 1, z, &q)) return;

    if (olLinkAbove(olSectorLink(s)) == OL_NO_ROOM && q.ceiling != WALL) {
        *level = q.ceiling;
    }
    if (q.floor != OL_NO_FLOOR) {
        *depth = q.floor - c;
    }
}
