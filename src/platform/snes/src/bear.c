/* src/fixed/enemy.h: Bear::updateState and the ground Enemy update order.
 * The original graph owns speed, pending states, attacks and both deaths.
 * FX3 consumes pre-push current-frame FAST_HITMASK contact; GSU-2 keeps
 * rotated animation-union bounds. Blood and head rendering remain separate.
 */
#include "enemy.h"
#include "room.h"
#ifdef OL_FX3
#include "fx.h"
#endif

#define B_WALK      0
#define B_STOP      1
#define B_HIND      2
#define B_RUN       3
#define B_HOWL      4
#define B_GROWL     5
#define B_BITE      6
#define B_ATTACK    7
#define B_EAT       8
#define B_DEATH     9

/* Same target-info bit positions as Wolf_state. */
#define B_FRONT     1
#define B_CAN_BITE  4
#define B_TOUCH     16

#ifdef OL_C_MOTION
static void Bear_hit(int damage)
{
    if (items[0].health > 0) {
        items[0].health -= damage;
        if (items[0].health < 0) items[0].health = 0;
    }
}

unsigned int Bear_state(ItemObj* item, unsigned int info, int dist)
{
    unsigned int slot = (unsigned int)(item - items);
    unsigned int next = olEnemyNext[slot], mood = item->mood;

    if (item->health <= 0) {
        switch (item->state) {
            case B_HIND: return B_HOWL;
            case B_WALK:
            case B_RUN: return B_STOP;
            case B_STOP:
                item->flags2 &= (uint8)~ITEM_FLAG_REVERSE;
                return B_DEATH;
            case B_HOWL:
                item->flags2 |= ITEM_FLAG_REVERSE;
                return B_DEATH;
            case B_DEATH:
                if (item->flags2 & ITEM_FLAG_REVERSE) {
                    if (info & B_TOUCH) {
                        Bear_hit(200);
                        item->flags2 &= (uint8)~ITEM_FLAG_REVERSE;
                    }
                }
                return B_DEATH;
        }
        return item->goalState;
    }

    /* Keep the original ordering: updateMood consumed INJURED before this
       state method. REVERSE, when set, restores it for the next update. */
    if (item->flags2 & ITEM_FLAG_INJURED) {
        item->flags2 |= ITEM_FLAG_REVERSE;
    } else if (item->flags2 & ITEM_FLAG_REVERSE) {
        item->flags2 |= ITEM_FLAG_INJURED;
    }

    switch (item->state) {
        case B_WALK:
            olEnemyTurn[slot] = 2;
            if (items[0].health <= 0 && (info & B_FRONT) && (info & B_TOUCH)) {
                olEnemyNext[slot] = B_STOP;
                return B_STOP;
            }
            if (mood != MOOD_SLEEP) {
                if (mood == MOOD_ESCAPE) olEnemyNext[slot] = 0;
                return B_STOP;
            }
            if (rand_logic() < 0x50) {
                olEnemyNext[slot] = B_GROWL;
                return B_STOP;
            }
            break;
        case B_STOP:
            if (items[0].health <= 0)
                return (info & B_CAN_BITE) && dist < 768 ? B_EAT : B_WALK;
            if (next) return next;
            return mood == MOOD_SLEEP ? B_WALK : B_RUN;
        case B_HIND:
            if (item->flags2 & ITEM_FLAG_INJURED) {
                olEnemyNext[slot] = 0;
                return B_HOWL;
            }
            if ((info & B_FRONT) && (info & B_TOUCH)) return B_HOWL;
            if (mood == MOOD_ESCAPE) {
                olEnemyNext[slot] = 0;
                return B_HOWL;
            }
            /* Split call-containing conditions for 816-tcc without changing
               the short-circuit order or consuming an extra random roll. */
            if (mood == MOOD_SLEEP) {
                olEnemyNext[slot] = B_GROWL;
                return B_HOWL;
            }
            if (rand_logic() < 0x50) {
                olEnemyNext[slot] = B_GROWL;
                return B_HOWL;
            }
            if (dist > 2048) {
                olEnemyNext[slot] = B_STOP;
                return B_HOWL;
            }
            if (rand_logic() < 0x600) olEnemyNext[slot] = B_STOP;
            return B_HOWL;
        case B_RUN:
            olEnemyTurn[slot] = 5;
            if (info & B_TOUCH) {
                Bear_hit(3);
                olEnemyNext[slot] = next = 0;
            }
            if (items[0].health <= 0 || mood == MOOD_SLEEP) return B_STOP;
            if (!next && (info & B_FRONT) && dist < 2048) {
                if (!(item->flags2 & ITEM_FLAG_INJURED)) {
                    if (rand_logic() < 0x300) {
                        olEnemyNext[slot] = B_HOWL;
                        return B_STOP;
                    }
                }
                if (dist < 1024) return B_BITE;
            }
            break;
        case B_HOWL:
            if (item->flags2 & ITEM_FLAG_INJURED) {
                olEnemyNext[slot] = 0;
                return B_STOP;
            }
            if (next) return next;
            if (mood == MOOD_SLEEP || mood == MOOD_ESCAPE) return B_STOP;
            if ((info & B_CAN_BITE) && dist < 600) return B_ATTACK;
            return B_HIND;
        case B_BITE:
            if (!next && (info & B_TOUCH)) {
#ifdef OL_FX3
                Fx_bite(item, FX_SITE_BEAR);
#endif
                Bear_hit(200);
                olEnemyNext[slot] = B_STOP;
            }
            break;
        case B_ATTACK:
            if (!next && (info & B_TOUCH)) {
                Bear_hit(400);
                olEnemyNext[slot] = B_HOWL;
            }
            break;
    }
    return item->goalState;
}
void Bear_update(ItemObj* item, ItemObj* lara)
{
    unsigned int slot = (unsigned int)(item - items), info = 0;
    int dx, dz, dist = 16384, turn = 0, limit;

    /* updateTargetInfo leaves the default distance/front flags for a dead
       bear or target. A dying upright bear can still make fall contact. */
    if (item->health <= 0 || lara->health > 0) {
#ifdef OL_FX3
        /* A dead bear retains captured contact even if Lara died after
           the collision phase; living bears ignore a now-dead target. */
        if (olEnemyTouch[slot]) info |= B_TOUCH;
#else
        if (ItemObj_collideBounds(item, lara, 100, -olAnimBox(lara->animIndex, 2))) info |= B_TOUCH;
#endif
    }
    if (item->health > 0) {
        if (lara->health > 0) {
            dx = lara->x - item->x - (int)((phd_sin((int32)item->angleY) * 500) >> FIXED_SHIFT);
            dz = lara->z - item->z - (int)((phd_cos((int32)item->angleY) * 500) >> FIXED_SHIFT);
            dist = Enemy_distance(dx, dz);
            limit = (int)(unsigned int)(phd_atan((int32)dz, (int32)dx) - item->angleY);
            if (limit > -ANGLE_90 && limit < ANGLE_90) {
                info |= B_FRONT;
                limit = lara->y - item->y;
                if (limit > -256 && limit < 256) info |= B_CAN_BITE;
            }
        }
        if (Enemy_target(item, lara, &dx, &dz)) {
            limit = olEnemyTurn[slot] * ANGLE_1;
            if (item->hSpeed && limit) {
                turn = (int)(unsigned int)(phd_atan((int32)dz, (int32)dx) - item->angleY);
                if (turn <= -ANGLE_90 || turn >= ANGLE_90) {
                    if ((int32)Enemy_distance(dx, dz) < ((int32)item->hSpeed << FIXED_SHIFT) / limit) limit >>= 1;
                }
                if (turn > limit) turn = limit;
                else if (turn < -limit) turn = -limit;
            }
        }
    }
    item->goalState = (uint8)Bear_state(item, info, dist);
    Enemy_moveGround(item, turn);
}
#else
/* Preserve the infrequent close/reverse test's signed 32-bit division. */
int Bear_closeTurn(int dist, int speed, int limit)
{
    return (int32)dist < ((int32)speed << FIXED_SHIFT) / limit;
}
#endif
