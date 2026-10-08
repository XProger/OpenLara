/* src/fixed/enemy.h: Wolf::updateState and the ground Enemy update order.
 * The original animation graph owns speed and attacks. FX3 has per-creature
 * Nav; GSU-2 uses the shared flood. Contact uses rotated animation-union bounds
 * until joint hit spheres are available. Head/tilt rendering and blood are
 * not implemented on this target yet.
 */
#include "enemy.h"
#include "room.h"
#include "nav.h"
#ifdef OL_FX3
#include "fx.h"
#endif

#define W_STOP      1
#define W_WALK      2
#define W_RUN       3
#define W_STALK     5
#define W_ATTACK    6
#define W_HOWL      7
#define W_SLEEP     8
#define W_GROWL     9
#define W_DEATH     11
#define W_BITE      12

/* These bits describe TR's target info, not a replacement hit-mask format. */
#define W_FRONT     1
#define W_BEHIND    2
#define W_CAN_BITE  4
#define W_IN_ZONE   8
#define W_TOUCH     16

#ifdef OL_C_MOTION
/* Executable reference for wolf_state.asm, included in the CPU-only oracle. */
unsigned int Wolf_state(ItemObj* item, unsigned int info, int dist)
{
    unsigned int slot = (unsigned int)(item - items);
    unsigned int next = olEnemyNext[slot];
    unsigned int mood = item->mood;
    int damage;
    if (item->health <= 0) return item->goalState;
    switch (item->state)
    {
        case W_STOP:
            return next ? next : W_WALK;
        case W_WALK:
            olEnemyTurn[slot] = 2;
            if (mood != MOOD_SLEEP) {
                olEnemyNext[slot] = 0;
                return W_STALK;
            }
            if (rand_logic() < 0x20) {
                olEnemyNext[slot] = W_SLEEP;
                return W_STOP;
            }
            break;
        case W_RUN:
            olEnemyTurn[slot] = 5;
            if ((info & W_FRONT) && dist < 1536) {
                if (dist <= 768 && !(info & W_BEHIND)) {
                    olEnemyNext[slot] = 0;
                    return W_ATTACK;
                }
                olEnemyNext[slot] = W_STALK;
                return W_GROWL;
            }
            if (mood == MOOD_STALK && dist < 3069) {
                olEnemyNext[slot] = W_STALK;
                return W_GROWL;
            }
            if (mood == MOOD_SLEEP) return W_GROWL;
            break;
        case W_STALK:
            olEnemyTurn[slot] = 2;
            if (mood == MOOD_ESCAPE) return W_RUN;
            if (dist < 345 && (info & W_CAN_BITE) && items[0].health > 0) return W_BITE;
            if (dist > 3069) return W_RUN;
            if (mood == MOOD_ATTACK) {
                if (!(info & W_FRONT) || dist > 1536 || (info & W_BEHIND)) return W_RUN;
                break;
            }
            if (rand_logic() < 0x180) {
                olEnemyNext[slot] = W_HOWL;
                return W_GROWL;
            }
            if (mood == MOOD_SLEEP) return W_GROWL;
            break;
        case W_SLEEP:
            if (mood == MOOD_ESCAPE || (info & W_IN_ZONE)) {
                olEnemyNext[slot] = W_GROWL;
            } else {
                if (rand_logic() >= 0x20) break;
                olEnemyNext[slot] = W_WALK;
            }
            return W_STOP;
        case W_GROWL:
            if (next) return next;
            if (mood == MOOD_ESCAPE) return W_RUN;
            if (dist < 345 && (info & W_CAN_BITE) && items[0].health > 0) return W_BITE;
            if (mood == MOOD_STALK) return W_STALK;
            if (mood == MOOD_SLEEP) return W_STOP;
            return W_RUN;
        case W_ATTACK:
        case W_BITE:
            if (!next && (info & W_TOUCH)) {
#ifdef OL_FX3
                Fx_bite(item, FX_SITE_WOLF);
#endif
                damage = item->state == W_ATTACK ? 50 : 100;
                if (items[0].health > 0) {
                    items[0].health -= damage;
                    if (items[0].health < 0) items[0].health = 0;
                }
                olEnemyNext[slot] = item->state == W_ATTACK ? W_RUN : W_GROWL;
            }
            return W_RUN;
    }
    return item->goalState;
}
#endif

/* fastLength on the port's wrapped 16-bit coordinates. Saturate distances
 * beyond signed range: they are far away, never negative/within bite range. */
#ifdef OL_C_MOTION
int Enemy_distance(int dx, int dz)
{
    unsigned int x = dx < 0 ? -(unsigned int)dx : (unsigned int)dx;
    unsigned int z = dz < 0 ? -(unsigned int)dz : (unsigned int)dz;
    unsigned int d = x > z ? x + (z >> 1) : z + (x >> 1);
    return d > 32767 ? 32767 : (int)d;
}

void Wolf_update(ItemObj* item, ItemObj* lara)
{
    unsigned int slot = (unsigned int)(item - items), info = 0;
    int dx, dz, dist, turn = 0, limit, angle;

    if (item->health <= 0) {
        if (item->state != W_DEATH) {
            ItemObj_animSet(item, olModelFirstAnim(item->model) + 20 + RAND_LOGIC(3));
            olEnemyNext[slot] = 0;
        }
    } else {
        /* Navigation uses the previous tick's max turn, just as Enemy does
           before Wolf::updateState changes it for WALK/STALK/RUN. */
        if (Enemy_target(item, lara, &dx, &dz)) {
            limit = olEnemyTurn[slot] * ANGLE_1;
            if (item->hSpeed && limit) {
                turn = (int)(unsigned int)(phd_atan((int32)dz, (int32)dx) - item->angleY);
                if (turn <= -ANGLE_90 || turn >= ANGLE_90) {
                    dist = Enemy_distance(dx, dz);
                    if ((int32)dist < ((int32)item->hSpeed << FIXED_SHIFT) / limit) limit >>= 1;
                }
                if (turn > limit) turn = limit;
                else if (turn < -limit) turn = -limit;
            }
        }
        if (olEnemyInZone) info |= W_IN_ZONE;
        dx = lara->x - item->x - (int)((phd_sin((int32)item->angleY) * 375) >> FIXED_SHIFT);
        dz = lara->z - item->z - (int)((phd_cos((int32)item->angleY) * 375) >> FIXED_SHIFT);
        dist = Enemy_distance(dx, dz);
        angle = (int)phd_atan((int32)dz, (int32)dx);
        limit = (int)(unsigned int)(angle - item->angleY);
        if (limit > -ANGLE_90 && limit < ANGLE_90) {
            info |= W_FRONT;
            limit = lara->y - item->y;
            if (limit > -256 && limit < 256) info |= W_CAN_BITE;
        }
        limit = (int)(unsigned int)(angle - lara->angleY - ANGLE_180);
        if (limit > -ANGLE_90 && limit < ANGLE_90) info |= W_BEHIND;
        if (lara->health > 0 && (item->state == W_ATTACK || item->state == W_BITE)) {
#ifdef OL_FX3
            /* Lara's current-frame contact precedes the enemy push/update.
               Do not recompute an animation-wide box after she has moved. */
            if (olEnemyTouch[slot]) info |= W_TOUCH;
#else
            if (ItemObj_collideBounds(item, lara, 100, -olAnimBox(lara->animIndex, 2))) info |= W_TOUCH;
#endif
        }
        item->goalState = (uint8)Wolf_state(item, info, dist);
    }
    Enemy_moveGround(item, turn);
}
#else
/* Infrequent close/reverse steering division. The native update keeps
   the original signed 32-bit expression here, including truncation to zero. */
int Wolf_closeTurn(int dist, int speed, int limit)
{
    return (int32)dist < ((int32)speed << FIXED_SHIFT) / limit;
}
#endif
