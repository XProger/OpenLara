/* OpenLara - SNES / Super FX target
 * The port of src/fixed/object.h's Block: the pushable cube.
 *
 * A block is a floor. Upstream's Block::updateFloor rewrites the sector it
 * stands on so the room's floor is its top, and rewrites it back when it
 * moves; TR's floor data is level data upstream can modify because it has
 * copied it into RAM. Here the host's sector table lives in game pak RAM
 * bank 1 as well, so the same rewrite works on the same bytes -- the first
 * time this port changes the world.
 *
 * FX3 fires a heavy trigger when the block lands, as checkTrigger(fd, NULL)
 * does upstream. Not here: the nav boxes' blocked bit (upstream's TODO too).
 */

#include "object.h"
#include "lara.h"
#include "collision.h"
#include "room.h"
#include "level.h"

extern const int LIMIT_BLOCK[9];

#define BLOCK_STATE_NONE    0
#define BLOCK_STATE_READY   1
#define BLOCK_STATE_PUSH    2
#define BLOCK_STATE_PULL    3

/* vec3i getBlockOffset(int16 angleY, int32 offset): a sector along the
   block's facing. */
static void Block_offset(uint16 angleY, int offset, int* ox, int* oz)
{
    *ox = 0; *oz = 0;
    if (angleY == ANGLE_0)                 *oz = -offset;
    else if (angleY == (uint16)ANGLE_180)  *oz = offset;
    else if (angleY == ANGLE_90)           *ox = -offset;
    else                                   *ox = offset;
}

/* void Block::updateFloor(int32 offset): the sector's floor byte, in the
   host table, moved by the block's height. Room_reset forgets what room.asm
   remembered about the sector. */
void Block_updateFloor(ItemObj* item, int offset)
{
    unsigned int s = Room_sectorAt(item->room, item->x, item->z);
    int floor = olSectorFloor(s);
    int ceiling = olSectorCeiling(s);

    if (floor == -127) {
        floor = ceiling + (offset >> 8);
    } else {
        floor += (offset >> 8);
        if (floor == ceiling) floor = -127;
    }
    olTable8(Sector, (s << 2)) = (unsigned char)(int8)floor;
    Room_reset();
}

/* bool Block::checkBlocking(): it sits on its own sector's floor. */
static int Block_checkBlocking(ItemObj* item)
{
    unsigned int s = Room_sectorAt(item->room, item->x, item->z);
    int floor = olSectorFloor(s);
    return floor == -127 || (floor << 8) + 1024 == item->y;
}

/* bool Block::checkObstacles(x, z, height): the sector there has its floor
   exactly at the block's base and its ceiling clear of the block's top. */
static int Block_checkObstacles(ItemObj* item, int x, int z, int height)
{
    RoomQuery q;
    int floor = item->y;
    int ceiling = item->y - height;

    if (!Room_query(item->room, x, floor, z, &q)) return 0;
    if (q.floor != floor) return 0;
    if (!Room_query(q.room, x, ceiling, z, &q)) return 0;
    if (q.ceiling > ceiling) return 0;
    return 1;
}

static int Block_checkPush(ItemObj* item)
{
    int ox, oz;
    if (!Block_checkBlocking(item)) return 0;
    Block_offset(item->angleY, -1024, &ox, &oz);
    return Block_checkObstacles(item, item->x + ox, item->z + oz, 1024);
}

static int Block_checkPull(ItemObj* item, ItemObj* L)
{
    int ox, oz;
    if (!Block_checkBlocking(item)) return 0;
    Block_offset(item->angleY, 1024, &ox, &oz);
    if (!Block_checkObstacles(item, item->x + ox, item->z + oz, 1024)) return 0;
    return Block_checkObstacles(item, L->x + ox, L->z + oz, LARA_HEIGHT);
}

/* ---------------------------------------------------------------------------
 * void Block::collide(Lara* lara, CollisionInfo* cinfo)
 *
 * Upstream's. Standing at it with Action, facing it square, she takes hold
 * (BLOCK_READY, aligned to the wall of it); Up or Down from there, if the
 * way is clear, starts the push or the pull, hers and the block's together.
 * ------------------------------------------------------------------------ */
void Block_collide(ItemObj* item, ItemObj* L, unsigned int input)
{
    unsigned int quadrant;

    if (!(input & IN_ACTION)) return;
    if (ItemObj_status(item) == ITEM_STATUS_ACTIVE) return;
    if (L->y != item->y) return;

    quadrant = olQuadrant(L->angleY);

    if (L->animIndex == olModelFirstAnim(L->model) + LARA_ANIM_BLOCK_READY &&
        (input & (IN_UP | IN_DOWN)))
    {
        if (!ItemObj_animIsEnd(L, 0)) return;
        if (!Object_checkLimit(item, L, LIMIT_BLOCK, 0)) return;

        if (input & IN_UP) {
            if (!Block_checkPush(item)) return;
            L->goalState = STATE_BLOCK_PUSH;
            item->goalState = BLOCK_STATE_PUSH;
        } else {
            if (!Block_checkPull(item, L)) return;
            L->goalState = STATE_BLOCK_PULL;
            item->goalState = BLOCK_STATE_PULL;
        }

        Block_updateFloor(item, 1024);

        ItemObj_activate(item);
        ItemObj_setStatus(item, ITEM_STATUS_ACTIVE);

        ItemObj_animProcess(item);
        ItemObj_animProcess(L);
    }

    if (L->state == STATE_STOP)
    {
        if (olLaraWeaponState != WEAPON_STATE_FREE) return;
        if (input & (IN_UP | IN_DOWN)) return;

        item->angleY = (uint16)(quadrant << 14);

        if (!Object_checkLimit(item, L, LIMIT_BLOCK, 0)) return;

        L->goalState = STATE_BLOCK_READY;
        L->angleY = item->angleY;
        Lara_alignWall(L, LARA_RADIUS);
        ItemObj_animProcess(L);
        if (L->state == STATE_BLOCK_READY) {
            olLaraWeaponState = WEAPON_STATE_BUSY;
        }
    }
}

/* ---------------------------------------------------------------------------
 * void Block::update()
 *
 * Upstream's: the animation carries it a sector, gravity takes it down a
 * hole if there is one, and when it comes to rest it is a floor again.
 * ------------------------------------------------------------------------ */
void Block_update(ItemObj* item)
{
    if (item->flags & ITEM_FLAG_ONCE)
    {
        Block_updateFloor(item, 1024);
        ItemObj_deactivate(item);
        ItemObj_setStatus(item, ITEM_STATUS_INVISIBLE);
        return;
    }

    ItemObj_animProcess(item);
    ItemObj_move(item);
    ItemObj_updateRoom(item, 0);

    /* Y grows downwards. Stay active while above the floor, even if the
       push animation has already issued KILL; becoming a floor in mid-air
       would strand the block. A missing floor is not a landing height. */
    if (item->roomFloor == OL_NO_FLOOR) return;
    if (item->y < item->roomFloor)
    {
        item->flags2 |= ITEM_FLAG_GRAVITY;
        return;
    }
    else if (item->flags2 & ITEM_FLAG_GRAVITY)
    {
        item->flags2 &= (uint8)~ITEM_FLAG_GRAVITY;
        ItemObj_setStatus(item, ITEM_STATUS_INACTIVE);
        item->y = item->roomFloor;
        item->vSpeed = 0;
    }

    if (ItemObj_status(item) == ITEM_STATUS_INACTIVE)
    {
        ItemObj_deactivate(item);
        ItemObj_setStatus(item, ITEM_STATUS_NONE);
#ifdef OL_FX3
        {
            RoomQuery q;
            /* Upstream saved gLastFloorData in updateRoom before changing
               the floor, but only fires it after the block becomes a floor. */
            int queried = Room_query(item->room, item->x, item->y, item->z, &q);
            Block_updateFloor(item, -1024);
            if (queried) checkTriggerObject(q.trigger);
        }
#else
        Block_updateFloor(item, -1024);
#endif
    }
}
