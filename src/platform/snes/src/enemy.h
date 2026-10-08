/* OpenLara - SNES / Super FX target
 * The port of src/fixed/enemy.h's interface.
 */

#ifndef H_OL_ENEMY
#define H_OL_ENEMY

#include "item.h"

/* Upstream's moods, in upstream's order. */
#define MOOD_SLEEP      0
#define MOOD_STALK      1
#define MOOD_ATTACK     2
#define MOOD_ESCAPE     3

int  Enemy_row(unsigned int type);      /* -1 if the type is not a creature */
int  Enemy_health(unsigned int type);   /* what upstream starts it on */
void Enemy_update(ItemObj* item, ItemObj* lara);
int  Enemy_target(ItemObj* item, ItemObj* lara, int* dx, int* dz);
extern unsigned int olEnemyInZone;
int Enemy_distance(int dx, int dz);
void Enemy_moveGround(ItemObj* item, int turn);
unsigned int Wolf_state(ItemObj* item, unsigned int info, int dist);
void Wolf_update(ItemObj* item, ItemObj* lara);
unsigned int Bear_state(ItemObj* item, unsigned int info, int dist);
void Bear_update(ItemObj* item, ItemObj* lara);
void Bat_update(ItemObj* item, ItemObj* lara);
#ifdef OL_FX3
/* GBA HIDE_CORPSES: ten seconds at the original 30 Hz logic rate. Enemy
   timers have no object countdown after death, so use the saved item word. */
#define OL_CORPSE_TICKS (30 * 10)
#define OL_NOT_ENEMY (-0x4000)
int Enemy_corpseTick(ItemObj* item);
void Lion_update(ItemObj* item, ItemObj* lara);
unsigned int Raptor_state(ItemObj* item, unsigned int info, int dist);
void Raptor_update(ItemObj* item, ItemObj* lara);
void Raptor_hit(ItemObj* item, int damage);
void Raptor_die(ItemObj* item);
extern uint8 olEnemyTouch[OL_ITEM_MAX];
void Enemy_collideLara(ItemObj* lara);
extern uint8 olLaraHitQuadrant, olLaraHitTimer, olLaraHitFrame;
#define OL_HIT_PENDING 0xFF
extern int16 olLaraHitIn[6];
void Lara_enemyHit(ItemObj* item, int x, int z);
void Lara_hitResolve(void);
void Lara_hitTick(void);
void olLaraHitPublish(void);
struct NavFx3;
struct NavFx3* Enemy_navAcquire(ItemObj* item, int forced);
void Enemy_navRelease(ItemObj* item);
int Enemy_waypointY(ItemObj* item);
#endif

#endif
