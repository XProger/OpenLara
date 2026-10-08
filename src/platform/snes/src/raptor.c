/* src/fixed/enemy.h: Raptor's state graph, lethal-hit animation selection
 * and ground update order. FX3 only until GSU-2 has a resident-code plan.
 * Contact is latched before Lara's current-frame collision push, matching
 * the GBA/32X FAST_HITMASK path. Rendered tilt/head and blood remain.
 */
#include "enemy.h"
#include "room.h"
#include "fx.h"

#define R_DEATH      0
#define R_STOP       1
#define R_WALK       2
#define R_RUN        3
#define R_ATTACK_1   4
#define R_ROAR       6
#define R_ATTACK_2   7
#define R_BITE       8

#define R_FRONT      1
#define R_CAN_BITE   4
#define R_TOUCH      16

void Raptor_die(ItemObj* item)
{
    ItemObj_animSet(item, olModelFirstAnim(item->model) + 9 + RAND_LOGIC(2));
    /* Original animSet(resetState=true) resets state/goal, not nextState. */
}

void Raptor_hit(ItemObj* item, int damage)
{
    if (item->health <= 0 || damage <= 0) return;
    item->health -= damage;
    item->flags2 |= ITEM_FLAG_INJURED;
    if (item->health <= 0) {
        item->health = 0;
        item->timer = OL_CORPSE_TICKS;
        Raptor_die(item);
    }
}

#ifdef OL_C_MOTION
unsigned int Raptor_state(ItemObj* item, unsigned int info, int dist)
{
    unsigned int slot = (unsigned int)(item - items);
    unsigned int next = olEnemyNext[slot], mood = item->mood;
    if (item->health <= 0) return item->goalState;
    switch (item->state) {
        case R_STOP:
            if (next) return next;
            if (info & R_TOUCH) return R_BITE;
            if ((info & R_CAN_BITE) && dist < 680) return R_BITE;
            if ((info & R_CAN_BITE) && dist < 1536) return R_ATTACK_1;
            return mood == MOOD_SLEEP ? R_WALK : R_RUN;
        case R_WALK:
            olEnemyTurn[slot] = 1;
            if (mood != MOOD_SLEEP) return R_STOP;
            if (rand_logic() < 0x100) {
                olEnemyNext[slot] = R_ROAR;
                return R_STOP;
            }
            break;
        case R_RUN:
            olEnemyTurn[slot] = 4;
            if (info & R_TOUCH) return R_STOP;
            if ((info & R_CAN_BITE) && dist < 1536) {
                if (item->goalState != R_RUN) break;
                return rand_logic() < 0x2000 ? R_STOP : R_ATTACK_2;
            }
            /* Keep short-circuit random consumption explicit for 816-tcc. */
            if ((info & R_FRONT) && mood != MOOD_ESCAPE) {
                if (rand_logic() < 0x100) {
                    olEnemyNext[slot] = R_ROAR;
                    return R_STOP;
                }
            }
            if (mood == MOOD_SLEEP) return R_STOP;
            break;
        case R_ATTACK_1:
        case R_ATTACK_2:
        case R_BITE:
            if (!next && (info & R_FRONT) && (info & R_TOUCH)) {
                Fx_bite(item, FX_SITE_RAPTOR);
                if (items[0].health > 0) {
                    items[0].health -= 100;
                    if (items[0].health < 0) items[0].health = 0;
                }
                olEnemyNext[slot] = item->state == R_ATTACK_2 ? R_RUN : R_STOP;
            }
            break;
    }
    return item->goalState;
}

void Raptor_update(ItemObj* item, ItemObj* lara)
{
    unsigned int slot = (unsigned int)(item - items), info = 0;
    int dx, dz, dist = 16384, turn = 0, limit;

    if (item->health <= 0) {
        /* Weapon hits select the animation immediately. This also handles
           externally killed/loaded actors, once, without corpse rerolls. */
        if (item->state != R_DEATH) Raptor_die(item);
    } else {
        if (lara->health > 0) {
            if (olEnemyTouch[slot]) info |= R_TOUCH;
            dx = lara->x - item->x - (int)((phd_sin((int32)item->angleY) * 400) >> FIXED_SHIFT);
            dz = lara->z - item->z - (int)((phd_cos((int32)item->angleY) * 400) >> FIXED_SHIFT);
            dist = Enemy_distance(dx, dz);
            limit = (int)(unsigned int)(phd_atan((int32)dz, (int32)dx) - item->angleY);
            if (limit > -ANGLE_90 && limit < ANGLE_90) {
                info |= R_FRONT;
                limit = lara->y - item->y;
                if (limit > -256 && limit < 256) info |= R_CAN_BITE;
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
        item->goalState = (uint8)Raptor_state(item, info, dist);
    }
    olEnemyTouch[slot] = 0;
    Enemy_moveGround(item, turn);
}
#endif
