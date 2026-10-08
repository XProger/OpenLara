/* OpenLara - SNES / Super FX target
 * src/fixed/enemy.h: Bat::updateState and flying Enemy::updateLocation.
 * Uses per-creature Nav on FX3, the shared zone flood on GSU-2, and the
 * original bat states, animation speed,
 * 16-unit vertical pursuit, 20-degree turn and two-point contact attack.
 * FX3 uses pre-push current-frame FAST_HITMASK contact; GSU-2 retains
 * rotated animation unions. Neither is TR's joint-sphere implementation;
 * blood effects and rendered pitch remain unported.
 */
#include "enemy.h"
#include "room.h"
#ifdef OL_FX3
#include "fx.h"
#endif

#define BAT_AWAKE       1
#define BAT_FLY         2
#define BAT_ATTACK      3
#define BAT_CIRCLING    4
#define BAT_DEATH       5
#define BAT_TURN        (ANGLE_1 * 20)
#define BAT_VSPEED      16

void Bat_update(ItemObj* item, ItemObj* lara)
{
    RoomQuery q;
    int x = item->x, z = item->z, y = item->y;
    unsigned int room = item->room;
    int dx, dz, dy, turn, contact;

    if (!Room_query(room, x, y, z, &q)) return;
    if (q.floor == OL_NO_FLOOR || q.ceiling == WALL || q.ceiling >= q.floor) return;
    room = q.room;
    item->room = (uint8)room;
    item->roomFloor = (int16)q.floor;
    if (item->health <= 0)
    {
        item->hSpeed = 0;
        if (item->roomFloor != OL_NO_FLOOR && y < item->roomFloor) {
            item->flags2 |= ITEM_FLAG_GRAVITY;
            item->goalState = BAT_CIRCLING;
        } else {
            item->flags2 &= (uint8)~ITEM_FLAG_GRAVITY;
            item->goalState = BAT_DEATH;
            if (item->roomFloor != OL_NO_FLOOR) item->y = item->roomFloor;
        }
    }
    else
    {
        if (Enemy_target(item, lara, &dx, &dz)) {
            turn = (int)(unsigned int)(phd_atan((int32)dz, (int32)dx) - item->angleY);
            if (turn > BAT_TURN) turn = BAT_TURN;
            else if (turn < -BAT_TURN) turn = -BAT_TURN;
            if (item->hSpeed) item->angleY += turn;
        }

        contact = 0;
        if (lara->health > 0) {
#ifdef OL_FX3
            contact = olEnemyTouch[(unsigned int)(item - items)];
#else
            contact = ItemObj_collideBounds(item, lara, 100,
                                            -olAnimBox(lara->animIndex, 2));
#endif
        }
        switch (item->state)
        {
            case BAT_AWAKE:
                item->goalState = BAT_FLY;
                break;
            case BAT_FLY:
                if (contact) item->goalState = BAT_ATTACK;
                break;
            case BAT_ATTACK:
                if (!contact) {
                    item->mood = MOOD_SLEEP;
                    item->goalState = BAT_FLY;
                } else {
#ifdef OL_FX3
                    Fx_bite(item, FX_SITE_BAT);     /* Enemy::bite */
#endif
                    lara->health -= 2;
                    if (lara->health < 0) lara->health = 0;
                }
                break;
        }
    }

    ItemObj_animProcess(item);
    ItemObj_move(item);
    if (item->health <= 0) {
        /* Gravity must not inherit forward motion from a living animation
           while its transition to CIRCLING/DEATH is still pending. */
        item->x = x;
        item->z = z;
        item->hSpeed = 0;
#ifdef OL_FX3
    } else {
        dy = Enemy_waypointY(item) - item->y;
#else
    } else if (item->state != BAT_AWAKE && item->mood == MOOD_ATTACK) {
        dy = lara->y + olAnimBox(lara->animIndex, 2) - item->y;
#endif
        if (dy > BAT_VSPEED) dy = BAT_VSPEED;
        else if (dy < -BAT_VSPEED) dy = -BAT_VSPEED;
        item->y += dy;
    }

    /* Resolve portals after moving, then clamp flight to the column. A wall
       or a step whose column doesn't contain the previous height rejects
       horizontal movement; it must not teleport the bat onto the new floor.
       These point bounds mirror upstream's flying updateLocation. */
    if (!Room_query(room, item->x, item->y, item->z, &q)) goto blocked;
    if (q.floor == OL_NO_FLOOR || q.ceiling == WALL || q.ceiling >= q.floor ||
        y < q.ceiling || y > q.floor)
    {
        item->x = x;
        item->z = z;
        if (!Room_query(room, x, item->y, z, &q)) goto blocked;
    }
    if (q.floor == OL_NO_FLOOR || q.ceiling == WALL || q.ceiling >= q.floor) goto blocked;
    if (item->y > q.floor) item->y = q.floor;
    if (item->y < q.ceiling) item->y = q.ceiling;
    item->room = (uint8)q.room;
    item->roomFloor = (int16)q.floor;
    return;

blocked:
    item->x = x;
    item->y = y;
    item->z = z;
}
