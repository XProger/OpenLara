/* OpenLara - SNES / Super FX target
 * What a door does to the world: src/fixed/object.h's Door::action and
 * setDoorState. FX3 only.
 *
 * Upstream blocks a doorway by rewriting sectors, and by nothing else --
 * Door::collide is "TODO door collision". While the door is shut, the sector
 * past it in its own room and, through that sector's portal, the sector where
 * it stands in the room beyond are walls: no floor, no ceiling, no rooms, no
 * floor data, no box. Open, both are the level's again. Lara's collision, a
 * creature's step, the camera's clip and the guns' line of sight all ask
 * those sectors, so all of them see a shut door as upstream's do; until this,
 * collision.c pushed her out of a box and nothing else knew the door was
 * there.
 *
 * FX3's host keeps its sector table in WRAM ($7F:0000, copied at bind by
 * host_tables.asm) and the level's own in ROM, so "the level's again" is a
 * far read, as upstream's `room->data.sectors`. A shut sector is {-127,
 * -127, link 0}: TR's NO_FLOOR both ways, and the link that leads nowhere
 * with no trigger. Not here, and not upstream either: the boxes ("TODO
 * modify level.boxes") and flipped rooms.
 *
 * GSU-2 keeps collision.c's box: its resident C has no room for this.
 */

#include "object.h"
#include "room.h"
#include "level.h"

/* The sectors as this last wrote them, per entity: 1 open. Zero at a boot
   (the reset clears .bss), when olLevelInit shuts every door. Not saved: a
   load boots the level, which shuts them, and the doors standing open
   reopen theirs on their first update, as upstream's do after gameLoad. */
uint8 olDoorOpen[OL_ITEM_MAX];

extern int fbValid, tcValid, clipValid;

/* Sector s as the level has it: the SECTOR table's descriptor (directory
   entry 1, offset at byte 52) in the level's host bank. A table lies
   within one bank, so the offset is a 16-bit add. */
static const unsigned char* Door_level(unsigned int s)
{
    union { const unsigned char* p; uint16 w[2]; } u;
    u.w[0] = (uint16)(olHeader16(52) + (s << 2));
    u.w[1] = (uint16)(olHeader16(54) + olLevel.hostBank);
    return u.p;
}

/* setDoorState for one sector. Returns the room through its portal as the
   level has it -- upstream reads getNextRoom before a wall goes in and
   after the level's sector comes back, the same sector either way. */
static unsigned int Door_sector(unsigned int room, int x, int z, int close)
{
    unsigned int s = Room_sectorAt(room, x, z);
    const unsigned char* level = Door_level(s);
    unsigned char* live = olSectorData + (s << 2);

    if (close) {
        olData16(live, 0) = 0x8181;     /* NO_FLOOR, floor and ceiling */
        olData16(live, 2) = 0;          /* link 0: nowhere, no trigger */
    } else {
        olData16(live, 0) = olData16(level, 0);
        olData16(live, 2) = olData16(level, 2);
    }
    return olTable8(Link, (olData16(level, 2) << 3) + 2);
}

/* void Door::action(bool close). getBlockOffset(angle.y, 1): the sector
   along the door's facing, in its own room; then the door's own position
   in the room that sector's portal leads to. */
void Door_action(ItemObj* item, int close)
{
    int ox = 0, oz = 0;
    unsigned int next;

    if (item->angleY == ANGLE_0)                 oz = -1024;
    else if (item->angleY == (uint16)ANGLE_180)  oz = 1024;
    else if (item->angleY == ANGLE_90)           ox = -1024;
    else                                         ox = 1024;

    next = Door_sector(item->room, item->x + ox, item->z + oz, close);
    if (next < olRoomCount) {
        Door_sector(next, item->x, item->z, close);
    }

    olDoorOpen[item - items] = (uint8)!close;

    /* What room.asm and the camera remember about sectors is stale now. */
    Room_reset();
    fbValid = 0;
    tcValid = 0;
    clipValid = 0;
}

/* Door::update's action calls, after Door_update set the goal from
   isActive (which must not be asked twice: it counts the timer down).
   Active, a door that stands open opens its sectors; inactive, one that
   stands shut shuts them; on the way between they stay as they were.
   Upstream rewrites them every frame; this, when they change. */
void Door_tick(ItemObj* item)
{
    unsigned int open = olDoorOpen[item - items];

    if (item->goalState == STATE_OPEN) {
        if (item->state != STATE_CLOSE && !open) Door_action(item, 0);
    } else {
        if (item->state != STATE_OPEN && open) Door_action(item, 1);
    }
}
