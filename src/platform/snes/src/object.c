/* OpenLara - SNES / Super FX target
 * The port of src/fixed/object.h.
 */

#include "object.h"
#include "enemy.h"
#include "room.h"
#include "lara.h"
#ifdef OL_FX3
#include "fx.h"
#endif

/* ---------------------------------------------------------------------------
 * ItemObj::init's switch, as a table.
 *
 * The type numbers are TR's own, from the ITEM_ enum in common.h. Ranges
 * rather than a case each, because they are contiguous there and a table of
 * 191 entries would be 191 bytes of image to say the same thing.
 * ------------------------------------------------------------------------ */
#define ITEM_WOLF               7
#define ITEM_ADAM               34
#define ITEM_TRAP_FLOOR         35
#define ITEM_TRAP_SWING_BLADE   36
#define ITEM_DART               39
#define ITEM_TRAP_DART_EMITTER  40
#define ITEM_DRAWBRIDGE         41
#define ITEM_BLOCK_1            48
#define ITEM_BLOCK_4            51
#define ITEM_SWITCH             55
#define ITEM_SWITCH_WATER       56
#define ITEM_DOOR_1             57
#define ITEM_DOOR_8             64
#define ITEM_TRAP_DOOR_1        65
#define ITEM_TRAP_DOOR_LOD      67
#define ITEM_BRIDGE_FLAT        68
#define ITEM_BRIDGE_TILT_2      70

/* Everything upstream's init table hands to Pickup, which is four runs of
   TR's enum rather than one: the weapons and their ammunition, the four
   puzzle pieces, the lead bar, and the four keys. The gaps between them are
   the inventory's own copies of the same things -- ITEM_INV_MEDIKIT_SMALL
   and the rest -- which are never in a level. */
#define ITEM_PISTOLS            84
#define ITEM_MEDIKIT_BIG        94
#define ITEM_PUZZLE_1           110
#define ITEM_PUZZLE_4           113
#define ITEM_PUZZLEHOLE_1       118
#define ITEM_PUZZLEHOLE_4       121
#define ITEM_LEADBAR            126
#define ITEM_KEY_ITEM_1         129
#define ITEM_KEY_ITEM_4         132
#define ITEM_KEY_HOLE_1         137
#define ITEM_KEY_HOLE_4         140
#define ITEM_SPLASH             153
#define ITEM_BUBBLE             155
#define ITEM_BLOOD              158
#define ITEM_SMOKE              160
#define ITEM_RICOCHET           164

#ifdef OL_C_MOTION
unsigned int ItemObj_class(unsigned int type)
{
    if (type == ITEM_LARA) return CLASS_LARA;

    /* WOLF through ADAM is every creature TR1 has, and Enemy_row says which
       of them this port knows the states of. */
    if (type >= ITEM_WOLF && type <= ITEM_ADAM) {
        return (Enemy_row(type) >= 0) ? CLASS_ENEMY : CLASS_NONE;
    }

    if (type == ITEM_TRAP_FLOOR) return CLASS_TRAP_FLOOR;
#ifdef OL_FX3
    if (type == ITEM_TRAP_SWING_BLADE) return CLASS_SWING_BLADE;
    if (type == ITEM_DART) return CLASS_DART;
    if (type == ITEM_TRAP_DART_EMITTER) return CLASS_DART_EMITTER;
    if (type == ITEM_DRAWBRIDGE) return CLASS_DRAWBRIDGE;
#endif
    if (type >= ITEM_BLOCK_1 && type <= ITEM_BLOCK_4) return CLASS_BLOCK;
    if (type >= ITEM_PUZZLEHOLE_1 && type <= ITEM_PUZZLEHOLE_4) return CLASS_HOLE;
    if (type >= ITEM_KEY_HOLE_1 && type <= ITEM_KEY_HOLE_4) return CLASS_HOLE;
    if (type == ITEM_SWITCH || type == ITEM_SWITCH_WATER) return CLASS_SWITCH;
    if (type >= ITEM_DOOR_1 && type <= ITEM_DOOR_8) return CLASS_DOOR;
    if (type >= ITEM_TRAP_DOOR_1 && type <= ITEM_TRAP_DOOR_LOD) return CLASS_TRAP_DOOR;
    if (type >= ITEM_BRIDGE_FLAT && type <= ITEM_BRIDGE_TILT_2) return CLASS_BRIDGE;

    if (type >= ITEM_PISTOLS   && type <= ITEM_MEDIKIT_BIG) return CLASS_PICKUP;
    if (type >= ITEM_PUZZLE_1  && type <= ITEM_PUZZLE_4)    return CLASS_PICKUP;
    if (type == ITEM_LEADBAR)                               return CLASS_PICKUP;
    if (type >= ITEM_KEY_ITEM_1 && type <= ITEM_KEY_ITEM_4) return CLASS_PICKUP;
#ifdef OL_FX3
    /* SPLASH, BUBBLE, BLOOD, SMOKE, RICOCHET: the effects pool's (fx.c) */
    if (type == ITEM_SPLASH || type == ITEM_BUBBLE || type == ITEM_BLOOD ||
        type == ITEM_SMOKE || type == ITEM_RICOCHET) return CLASS_FX;
#endif

    return CLASS_NONE;
}
#endif

/* ---------------------------------------------------------------------------
 * void Door::update()
 *
 * Two lines upstream and two here. It is right for trapdoors as well, and for
 * the dart emitter, which is why it was the first class the port had.
 * ------------------------------------------------------------------------ */
static void Door_update(ItemObj* item)
{
    item->goalState = ItemObj_isActive(item) ? STATE_OPEN : STATE_CLOSE;
}

/* ---------------------------------------------------------------------------
 * void Switch::update()
 *
 * Upstream sets every mask bit each frame -- a switch is not something a
 * trigger part-activates -- and drops back to DOWN when it stops being
 * active. The timer clearing goes with it: a switch on a timer is one that
 * springs back, and clearing it here is what stops that happening twice.
 * ------------------------------------------------------------------------ */
static void Switch_update(ItemObj* item)
{
    item->flags |= ITEM_FLAG_MASK;

    if (!ItemObj_isActive(item)) {
        item->goalState = SWITCH_STATE_DOWN;
        item->timer = 0;
    }
}

/* ---------------------------------------------------------------------------
 * bool Switch::use(int32 t)
 *
 * Upstream's. The sector a switch stands in carries a trigger of the SWITCH
 * type, and standing there asks this every tick: has the lever been pulled?
 * It has when the pull's animation has run to its KILL command and left the
 * status INACTIVE, and then -- once -- the trigger's other entities fire. A
 * timed switch is left counting down instead of deactivated, and springs
 * back when Switch_update sees the timer expire.
 * ------------------------------------------------------------------------ */
int Switch_use(ItemObj* item, int t)
{
    if (ItemObj_status(item) != ITEM_STATUS_INACTIVE) return 0;

    ItemObj_setStatus(item, ITEM_STATUS_NONE);

    if (t > 0 && item->state == SWITCH_STATE_UP) {
        if (t != 1) t *= 30;
        item->timer = (int16)t;
        ItemObj_setStatus(item, ITEM_STATUS_ACTIVE);
    } else {
        ItemObj_deactivate(item);
    }
    return 1;
}

/* bool Key::use(ItemObj* lara): the keyhole's trigger, the same shape. The
   key is in it once Hole_apply has put Lara through USE_KEY and left the
   status ACTIVE; her hands must be free again, which is the animation's
   EMPTY command. */
int Key_use(ItemObj* item)
{
    if (ItemObj_status(item) != ITEM_STATUS_ACTIVE) return 0;
    if (olLaraWeaponState != WEAPON_STATE_FREE) return 0;
    ItemObj_setStatus(item, ITEM_STATUS_INACTIVE);
    return 1;
}

/* ---------------------------------------------------------------------------
 * Pickup
 *
 * The taking is Pickup::collide, in use.c: she bends for it. Where it goes
 * is a counter, upstream's gSaveGame.pickups, and the inventory's counts. A
 * medipack that healed on touch would be a rule this port invented: TR's
 * heals when you use it, and using it is a screen that does not exist yet.
 * ------------------------------------------------------------------------ */
unsigned int olPickups;

int Pickup_use(ItemObj* item)
{
    if (ItemObj_status(item) != ITEM_STATUS_INVISIBLE) return 0;

    ItemObj_setStatus(item, ITEM_STATUS_INACTIVE);
    return 1;
}

/* ---------------------------------------------------------------------------
 * void TrapFloor::update()
 *
 * Upstream's, line for line. The floor she is standing on notices her, shakes,
 * lets go, falls under gravity until it reaches the room's own floor, and
 * lies there.
 *
 * Two things are this port's rather than upstream's, both because only the
 * GSU can read the level:
 *
 *   - `updateRoom` is handed the floor rather than looking a sector up. The
 *     GSU probes one point a frame and it is hers, which is exactly the
 *     sector a falling floor is in when it falls -- she is standing on it.
 *   - the "is she on me" test is upstream's `getLara(pos)->pos.y != pos.y -
 *     512` with her sector checked as well, because upstream's `getLara`
 *     looks her up in the *room* and this side has one Lara and no such
 *     lookup. Height alone would fire every trapdoor on the level whenever
 *     she happened to stand at the right altitude somewhere else.
 *
 * It calls animProcess itself, as upstream does, and the caller must not call
 * it again -- the state has to be read *after* the animation has advanced, or
 * a floor that reached STATE_DOWN spends one more frame accelerating.
 * ------------------------------------------------------------------------ */
static void TrapFloor_update(ItemObj* item, ItemObj* lara)
{
    switch (item->state)
    {
        case TRAP_FLOOR_STATE_STATIC:
        {
            int onIt = (lara->y == item->y - TRAP_FLOOR_HEIGHT) &&
                       ((lara->x >> 10) == (item->x >> 10)) &&
                       ((lara->z >> 10) == (item->z >> 10));
            if (!onIt) {
                ItemObj_setStatus(item, ITEM_STATUS_NONE);
                ItemObj_deactivate(item);
                return;
            }
            item->goalState = TRAP_FLOOR_STATE_SHAKE;
            break;
        }

        case TRAP_FLOOR_STATE_SHAKE:
            item->goalState = TRAP_FLOOR_STATE_FALL;
            break;

        case TRAP_FLOOR_STATE_FALL:
            if (item->goalState != TRAP_FLOOR_STATE_DOWN) {
                item->flags2 |= ITEM_FLAG_GRAVITY;
            }
            break;
    }

    ItemObj_animProcess(item);
    ItemObj_move(item);

    if (ItemObj_status(item) == ITEM_STATUS_INACTIVE) {
        ItemObj_deactivate(item);
        return;
    }

    if (item->state != TRAP_FLOOR_STATE_FALL) return;

    /* Upstream's updateRoom, which is where roomFloor comes from. */
    ItemObj_updateRoom(item, 0);

    if (item->roomFloor != OL_NO_FLOOR && item->y >= item->roomFloor)
    {
        item->y = item->roomFloor;
        item->vSpeed = 0;
        item->flags2 &= (uint8)~ITEM_FLAG_GRAVITY;
        item->goalState = TRAP_FLOOR_STATE_DOWN;
    }
}

/* ---------------------------------------------------------------------------
 * ItemObj::update, dispatched on the class rather than a vtable.
 * ------------------------------------------------------------------------ */
/* Upstream's every class ends its update() with animProcess(), and one of
   them -- TrapFloor -- has to read its state *after* the animation advanced.
   So the call lives here, per class, rather than in the caller's loop: a
   floor that reached STATE_DOWN would otherwise spend one more frame under
   gravity. */
void ItemObj_update(ItemObj* item, ItemObj* lara)
{
    switch (ItemObj_class(item->type))
    {
        case CLASS_TRAP_FLOOR:
            TrapFloor_update(item, lara);    /* calls animProcess itself */
            return;

        case CLASS_BLOCK:
            Block_update(item);              /* and this */
            return;

#ifdef OL_FX3
        case CLASS_DART_EMITTER:
            DartEmitter_update(item);        /* and these */
            return;

        case CLASS_DART:
            Dart_update(item);
            return;

        case CLASS_SWING_BLADE:
            SwingBlade_update(item);
            return;

        case CLASS_FX:
            Fx_update(item);                 /* no animation to process */
            return;
#endif

        /* Lara is updated by the host before this sweep, not in it: what she
           does depends on the floor and the walls the GSU staged, and what
           the camera does depends on her, so the order is the host's. Her
           update is Lara_update, which is upstream's override of this one and
           runs animProcess itself. */
        case CLASS_LARA:
            return;
        default:
            break;
    }

    switch (ItemObj_class(item->type))
    {
        case CLASS_ENEMY:
            Enemy_update(item, lara);
            return;                         /* species advances animation */

        case CLASS_DOOR:
#ifdef OL_FX3
            Door_update(item);
            Door_tick(item);                /* door.c: its sectors */
            break;
#endif
        case CLASS_TRAP_DOOR:
            Door_update(item);
            break;

        case CLASS_SWITCH:
            Switch_update(item);
            break;

        /* Lara is driven by the host and a bridge is a floor rather than an
           actor. Everything else stands still -- unless a trigger names it,
           in which case it gets the open-and-shut behaviour, because that is
           what the traps share and upstream gives each of them a class of its
           own that this port has not got to.

           TR1's traps are types 35 to 54 and they are not all this shape: a
           falling floor falls, a boulder rolls, a swinging blade swings. What
           they have in common is that a trigger starts them, and "open" is
           the nearest thing to starting that exists here. The dart emitter in
           room 6 of LEVEL1 really is open-and-shut, which is why it was the
           first one to work. */
        case CLASS_NONE:
            if (item->flags & ITEM_FLAG_DRIVEN) Door_update(item);
            break;

        /* A pickup has one animation of one frame and does nothing until it
           is taken, which is a collision rather than an update. Lara is
           driven by the host; a bridge is a floor rather than an actor. */
        case CLASS_BRIDGE:
        case CLASS_DRAWBRIDGE:
        case CLASS_PICKUP:
        default:
            break;
    }

    ItemObj_animProcess(item);
}
