/* OpenLara - SNES / Super FX target
 * The port of src/fixed/object.h's collide() methods: what happens when she
 * stands in front of a thing and presses Action.
 *
 * Upstream gives each class a virtual collide(lara, cinfo) and calls it for
 * every entity with ITEM_FLAG_COLLISION in the rooms around her; the class
 * decides whether she is close enough (a limit box in the thing's own frame
 * and an angle window), moves her onto the spot, and plays the interaction
 * through animSkip. This is the same, dispatched on the class, over a list
 * of the entities that can be used at all.
 *
 * Not here: the inventory screen. TR opens it for a keyhole and lets you
 * choose; this side carries counts of what she picked up and offers the
 * matching key when she presses Action at the hole. The sound of a key that
 * does not fit (SND_NO) is played through sound.c.
 */

#include "object.h"
#include "lara.h"
#include "collision.h"
#include "room.h"
#include "level.h"
#include "sound.h"

/* Upstream's Limit tables, upstream's numbers: the box she has to be in, in
   the thing's frame -- minX maxX minY maxY minZ maxZ -- and how far off its
   facing she may be about each axis. rodata: 816-tcc keeps a const table
   in the code image. */
#define LIM_ANGLE(deg)  ((deg) * ANGLE_1)

static const int LIMIT_SWITCH[9] = {
    -200, 200, 0, 0, 312, 512, LIM_ANGLE(10), LIM_ANGLE(30), LIM_ANGLE(10)
};
static const int LIMIT_SWITCH_UW[9] = {
    -1024, 1024, -1024, 1024, -1024, 1024,
    LIM_ANGLE(80), LIM_ANGLE(80), LIM_ANGLE(80)
};
static const int LIMIT_PICKUP[9] = {
    -256, 256, -100, 100, -256, 100, LIM_ANGLE(10), 0, 0
};
static const int LIMIT_PICKUP_UW[9] = {
    -512, 512, -512, 512, -512, 512,
    LIM_ANGLE(45), LIM_ANGLE(45), LIM_ANGLE(45)
};
static const int LIMIT_HOLE[9] = {
    -200, 200, 0, 0, 312, 512, LIM_ANGLE(10), LIM_ANGLE(30), LIM_ANGLE(10)
};
const int LIMIT_BLOCK[9] = {
    -300, 300, 0, 0, -692, -512, LIM_ANGLE(10), LIM_ANGLE(30), LIM_ANGLE(10)
};

/* ---------------------------------------------------------------------------
 * bool Object::checkLimit(Lara* lara, const int16* limitData)
 *
 * Her offset from the thing, turned into the thing's frame -- the inverse
 * of the rotation the OFFSET command applies, which is collideBounds's --
 * against the box; her angles against the windows. Nothing here rolls, and
 * the only pitch is hers under water, so the X window is her pitch against
 * the thing's (the underwater pickup wants her nosed down 25 degrees).
 * ------------------------------------------------------------------------ */
#define X_ROTX(x, y, s, c)  ((int)((((int32)(x) * (c)) - ((int32)(y) * (s))) >> FIXED_SHIFT))
#define X_ROTY(x, y, s, c)  ((int)((((int32)(y) * (c)) + ((int32)(x) * (s))) >> FIXED_SHIFT))

int Object_checkLimit(ItemObj* item, ItemObj* L, const int* lim, int pitch)
{
    int a, dx, dy, dz, px, pz;
    int32 s, c;

    a = (int)(int16)(uint16)(L->angleY - item->angleY);
    /* int is 16-bit on the cartridge: abs(-32768) stays negative and
       would accept a half turn. Compare the signed window directly. */
    if (a < -lim[7] || a > lim[7]) return 0;

    a = olLaraAngleX - pitch;
    if (a < -lim[6] || a > lim[6]) return 0;

    dx = L->x - item->x;
    dy = L->y - item->y;
    dz = L->z - item->z;

    if (dy < lim[2] || dy > lim[3]) return 0;

    s = phd_sin((int32)item->angleY);
    c = phd_cos((int32)item->angleY);
    px = X_ROTX(dx, dz, s, c);
    pz = X_ROTY(dx, dz, s, c);

    return px >= lim[0] && px <= lim[1] && pz >= lim[4] && pz <= lim[5];
}

/* ---------------------------------------------------------------------------
 * The inventory, as far as a keyhole needs one: how many of each key and
 * puzzle piece she carries, and the medipacks and the lead bar for the day
 * there is a screen to use them from. Everything else she picks up is
 * counted in olPickups and nothing more.
 * ------------------------------------------------------------------------ */
#define ITEM_MEDIKIT_SMALL  93
#define ITEM_MEDIKIT_BIG    94
#define ITEM_PUZZLE_1       110
#define ITEM_PUZZLEHOLE_1   118
#define ITEM_LEADBAR        126
#define ITEM_KEY_ITEM_1     129
#define ITEM_KEY_HOLE_1     137

#define INV_KEY             0
#define INV_PUZZLE          4
#define INV_MEDI_SMALL      8
#define INV_MEDI_BIG        9
#define INV_LEADBAR         10
#define INV_SLOTS           11

uint8 olInventory[INV_SLOTS];

#ifdef OL_FX3
#include "inventory.h"
/* FX3 has the inventory itself (inventory.c): its counts are the truth. */
void Inventory_add(unsigned int type)
{
    Inventory_addType(type);
}
#else
static int Inventory_slot(unsigned int type)
{
    if (type >= ITEM_KEY_ITEM_1 && type < ITEM_KEY_ITEM_1 + 4) return INV_KEY + (int)(type - ITEM_KEY_ITEM_1);
    if (type >= ITEM_PUZZLE_1  && type < ITEM_PUZZLE_1 + 4)  return INV_PUZZLE + (int)(type - ITEM_PUZZLE_1);
    if (type == ITEM_MEDIKIT_SMALL) return INV_MEDI_SMALL;
    if (type == ITEM_MEDIKIT_BIG)   return INV_MEDI_BIG;
    if (type == ITEM_LEADBAR)       return INV_LEADBAR;
    return -1;
}

void Inventory_add(unsigned int type)
{
    int k = Inventory_slot(type);
    if (k >= 0) olInventory[k]++;
}

/* Inventory::applyItem for a hole: the key or piece that fits it, if she has
   one, spent. */
static int Inventory_apply(unsigned int holeType)
{
    int k;
    if (holeType >= ITEM_KEY_HOLE_1 && holeType < ITEM_KEY_HOLE_1 + 4) {
        k = INV_KEY + (int)(holeType - ITEM_KEY_HOLE_1);
    } else if (holeType >= ITEM_PUZZLEHOLE_1 && holeType < ITEM_PUZZLEHOLE_1 + 4) {
        k = INV_PUZZLE + (int)(holeType - ITEM_PUZZLEHOLE_1);
    } else {
        return 0;
    }
    if (olInventory[k] == 0) return 0;
    olInventory[k]--;
    return 1;
}
#endif

/* ---------------------------------------------------------------------------
 * void Switch::collide(Lara* lara, CollisionInfo* cinfo)
 * void SwitchWater::collide(Lara* lara, CollisionInfo* cinfo)
 *
 * Upstream's. She has to be standing still, empty-handed, in the limit box
 * and pressing Action; she is turned to face the lever, the lever is asked
 * for the other state, and her pull is entered outright. The lever then
 * plays, ends on its KILL, and its own trigger -- the SWITCH type under her
 * feet -- fires what it is wired to (Switch_use). The water one is the same
 * from a tread, after she has swum onto the spot.
 * ------------------------------------------------------------------------ */
#define ITEM_SWITCH_WATER   56

static void Switch_collide(ItemObj* item, ItemObj* L, unsigned int input)
{
    int down;

    if (!(input & IN_ACTION)) return;
    if (ItemObj_status(item) != ITEM_STATUS_NONE) return;

    if (item->type == ITEM_SWITCH_WATER)
    {
        if (L->state != STATE_UW_TREAD) return;
        if (!Object_checkLimit(item, L, LIMIT_SWITCH_UW, 0)) return;
        if (!ItemObj_moveTo(L, 0, 0, 108, item, 1)) return;

        L->vSpeed = 0;
        item->goalState = (uint8)((item->state == SWITCH_STATE_UP) ? SWITCH_STATE_DOWN
                                                                   : SWITCH_STATE_UP);
        ItemObj_animSkip(L, STATE_SWITCH_DOWN, STATE_UW_TREAD, 1);
    }
    else
    {
        if (olLaraWeaponState != WEAPON_STATE_FREE) return;
        if (L->state != STATE_STOP) return;
        if (!Object_checkLimit(item, L, LIMIT_SWITCH, 0)) return;

        L->angleY = item->angleY;
        down = (item->state == SWITCH_STATE_DOWN);
        item->goalState = (uint8)(down ? SWITCH_STATE_UP : SWITCH_STATE_DOWN);
        ItemObj_animSkip(L, down ? STATE_SWITCH_DOWN : STATE_SWITCH_UP,
                         STATE_STOP, 1);
        olLaraWeaponState = WEAPON_STATE_BUSY;
    }

    ItemObj_activate(item);
    ItemObj_setStatus(item, ITEM_STATUS_ACTIVE);
}

/* ---------------------------------------------------------------------------
 * void Pickup::collide(Lara* lara, CollisionInfo* cinfo)
 *
 * Upstream's, with the inventory reduced to a count. The thing turns to
 * face her (it is a billboard here, so that is only what the limit box is
 * measured against); standing still with Action she steps a hundred units
 * short of it and bends; twenty-three frames before the bend ends it is
 * hers. Under water the same from a tread, swimming onto it, and earlier
 * in a shorter animation.
 * ------------------------------------------------------------------------ */
static void Pickup_take(ItemObj* item)
{
    Inventory_add(item->type);
    olPickups++;
    ItemObj_setStatus(item, ITEM_STATUS_INVISIBLE);
    ItemObj_touch(item);
}

static void Pickup_collide(ItemObj* item, ItemObj* L, unsigned int input)
{
    item->angleY = L->angleY;

    if (olLaraWaterState == WATER_STATE_ABOVE || olLaraWaterState == WATER_STATE_WADE)
    {
        if (!Object_checkLimit(item, L, LIMIT_PICKUP, 0)) return;

        if (L->state == STATE_PICKUP)
        {
            if (!ItemObj_animIsEnd(L, 23)) return;
            Pickup_take(item);
        }
        else if (L->state == STATE_STOP)
        {
            if (olLaraWeaponState != WEAPON_STATE_FREE) return;
            if (!(input & IN_ACTION)) return;
            ItemObj_moveTo(L, 0, 0, -100, item, 0);
            ItemObj_animSkip(L, STATE_PICKUP, STATE_STOP, 0);
            olLaraWeaponState = WEAPON_STATE_BUSY;
        }
        return;
    }

    if (olLaraWaterState == WATER_STATE_UNDER)
    {
        if (!Object_checkLimit(item, L, LIMIT_PICKUP_UW, LIM_ANGLE(-25))) return;

        if (L->state == STATE_PICKUP)
        {
            if (!ItemObj_animIsEnd(L, 14)) return;
            Pickup_take(item);
        }
        else if (L->state == STATE_UW_TREAD)
        {
            if (!(input & IN_ACTION)) return;
            if (!ItemObj_moveTo(L, 0, -200, -350, item, 1)) return;
            ItemObj_animSkip(L, STATE_PICKUP, STATE_UW_TREAD, 0);
        }
    }
}

/* ---------------------------------------------------------------------------
 * void Hole::apply(int32 offset, Lara* lara, Lara::State stateUse)
 * KeyHole::collide, PuzzleHole::collide
 *
 * Upstream's: Action at the hole with any key or piece in her pockets
 * opens the ring on its USE page, the hole's own first (FX3); what she
 * chooses there comes back as the inventory's useSlot, and the next tick
 * this spends it if it fits -- else SND_NO -- walks her onto the spot and
 * plays the use. GSU-2 has no ring: the matching one is spent outright. The hole is ACTIVE from then, which is what its own
 * trigger -- the KEY type in the sector -- asks for through Key_use once
 * her hands are free again. A puzzle hole becomes its DONE type when the
 * piece is in, as upstream's does; the piece is not drawn in it, since the
 * renderer's model for an entity is the packer's and fixed.
 * ------------------------------------------------------------------------ */
static void Hole_apply(ItemObj* item, ItemObj* L, unsigned int input,
                       int offset, unsigned int stateUse)
{
    if (olLaraWeaponState != WEAPON_STATE_FREE) return;
    if (ItemObj_status(item) != ITEM_STATUS_NONE) return;
    if (L->state != STATE_STOP) return;
    if (L->animIndex != olModelFirstAnim(L->model) + LARA_ANIM_STAND_NORMAL) return;
#ifdef OL_FX3
    if (!(input & IN_ACTION) && olInvUseSlot == SLOT_MAX) return;
    if (!Object_checkLimit(item, L, LIMIT_HOLE, 0)) return;
    if (olInvUseSlot == SLOT_MAX) {
        if (olInvNumKeys > 0) {
            Inventory_open(INV_PAGE_USE, item->type, input, input);
            return;
        }
    } else {
        if (Inventory_applyItem(item->type)) goto fits;
        olInvUseSlot = SLOT_MAX;
    }
    Sound_playItem(SND_NO, L);          /* Hole::collide: nothing fits */
    return;
fits:
#else
    if (!(input & IN_ACTION)) return;
    if (!Object_checkLimit(item, L, LIMIT_HOLE, 0)) return;
    if (!Inventory_apply(item->type)) {
        Sound_playItem(SND_NO, L);      /* Hole::collide: nothing fits */
        return;
    }
#endif

    ItemObj_moveTo(L, 0, 0, offset, item, 0);
    ItemObj_animSkip(L, stateUse, STATE_STOP, 0);
    olLaraWeaponState = WEAPON_STATE_BUSY;
    ItemObj_setStatus(item, ITEM_STATUS_ACTIVE);
}

static void Hole_collide(ItemObj* item, ItemObj* L, unsigned int input)
{
    if (item->type >= ITEM_KEY_HOLE_1) {
        Hole_apply(item, L, input, 362, STATE_USE_KEY);
        return;
    }

    if (L->state == STATE_USE_PUZZLE)
    {
        if (!Object_checkLimit(item, L, LIMIT_HOLE, 0)) return;
        if (!ItemObj_animIsEnd(L, 28)) return;
        item->type += 4;        /* PUZZLEHOLE_n -> PUZZLEHOLE_DONE_n */
        return;
    }

    Hole_apply(item, L, input, 327, STATE_USE_PUZZLE);
}

/* ---------------------------------------------------------------------------
 * void Lara::updateObjectsCollision()
 *
 * Upstream walks every entity in the rooms around her that has
 * ITEM_FLAG_COLLISION and is within four sectors, and calls its collide.
 * This walks the list olLevelInit made of the ones that have a collide to
 * call -- switches, pickups, holes, blocks -- with the same reach. Doors
 * are Lara_collideItems's, and creatures push in enemy.c's own way.
 * ------------------------------------------------------------------------ */
#define COLLIDE_REACH   4096

void Lara_collideObjects(ItemObj* L, unsigned int input)
{
    unsigned int n;

    if (L->health <= 0) return;

    /* Action also fires drawn pistols. On land every usable object's
       entry path requires free hands; walking that list while shooting
       spent the GSU's remaining time on interactions that cannot start.
       Keep pending pickup/puzzle completion and underwater use intact. */
    if (olLaraWeaponState == WEAPON_STATE_READY &&
        olLaraWaterState <= WATER_STATE_WADE &&
        L->state != STATE_PICKUP && L->state != STATE_USE_PUZZLE) return;

    /* Every collide below does nothing without Action, except the two
       that are waiting for a frame of an animation already playing. The
       walk over the list was 4.7 K cycles a tick when it ran every tick --
       a far pointer per field read, on this compiler -- and a tick has
       1.5 fields to live in. */
    if (!(input & IN_ACTION) && L->state != STATE_PICKUP &&
        L->state != STATE_USE_PUZZLE
#ifdef OL_FX3
        && olInvUseSlot == SLOT_MAX         /* chosen on the ring for a hole */
#endif
        ) return;

    for (n = 0; n < olUsableCount; n++)
    {
        ItemObj* it = items + olUsables[n];
        int d;

        if (ItemObj_status(it) == ITEM_STATUS_INVISIBLE) continue;

        d = it->x - L->x;
        if (d >= COLLIDE_REACH || d <= -COLLIDE_REACH) continue;
        d = it->z - L->z;
        if (d >= COLLIDE_REACH || d <= -COLLIDE_REACH) continue;
        d = it->y - L->y;
        if (d >= COLLIDE_REACH || d <= -COLLIDE_REACH) continue;

        switch (ItemObj_class(it->type))
        {
            case CLASS_SWITCH:  Switch_collide(it, L, input); break;
            case CLASS_PICKUP:  Pickup_collide(it, L, input); break;
            case CLASS_HOLE:    Hole_collide(it, L, input);   break;
            case CLASS_BLOCK:   Block_collide(it, L, input);  break;
            default: break;
        }
    }
}
