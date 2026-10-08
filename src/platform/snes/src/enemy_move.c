/* Ground branch of src/fixed/enemy.h: Enemy::updateLocation. Wolves and the
 * Lion family drop at most 1024; bears and raptors use 256. All step up 256
 * and store radius 170 (full diameter 340). */
#include "enemy.h"
#include "room.h"
#include "nav.h"

#ifdef OL_C_MOTION
/* Native builds use enemy_move.asm; retain the C implementation as its oracle. */
void Enemy_moveGround(ItemObj* item, int turn)
{
    RoomQuery q;
    int x = item->x, y = item->y, z = item->z, h;
    unsigned int room = item->room, oldBox, newBox, i;
    oldBox = navBoxAt(room, x, z);
    ItemObj_animProcess(item);
    ItemObj_move(item);
    if (!Room_query(room, item->x, item->y, item->z, &q)) goto groundBlocked;
    if (q.floor == OL_NO_FLOOR || q.ceiling == WALL || q.ceiling >= q.floor) goto groundBlocked;
    h = item->y - q.floor;
#ifdef OL_FX3
    if (h > 256 || h < ((item->type == 7 || (item->type >= 12 && item->type <= 14)) ? -1024 : -256)) goto groundBlocked;
#else
    if (h > 256 || h < (item->type == 8 ? -256 : -1024)) goto groundBlocked;
#endif
    if (h < -64) item->y += 64;
    else item->y = q.floor;
    for (i = 0; i < olCreatureCount; i++) {
        ItemObj* other = items + olCreatures[i];
        if (other != item && other->room == q.room && other->health > 0 &&
            ItemObj_status(other) == ITEM_STATUS_ACTIVE && item->hSpeed)
        {
            if (Enemy_distance(other->x - item->x, other->z - item->z) < 340) {
                turn = ANGLE_1 * 8;
                goto groundBlocked;
            }
        }
    }
    newBox = navBoxAt(q.room, item->x, item->z);
    if (oldBox == NO_BOX || newBox == NO_BOX) goto groundBlocked;
    if (olZone(oldBox, 0) != olZone(newBox, 0)) goto groundBlocked;
    item->room = (uint8)q.room;
    item->roomFloor = q.floor;
    item->angleY += turn;
    return;
groundBlocked:
    item->x = x; item->y = y; item->z = z;
    item->angleY += turn + turn;
}
#endif
