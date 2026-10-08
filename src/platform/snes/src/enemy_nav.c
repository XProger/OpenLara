/* FX3: src/fixed/enemy.h's three-context admission, moods and Nav targets.
 * Creature state graphs still own steering and animation. World coordinates
 * are reconstructed before box/corridor decisions, not compared as int16. */
#include "enemy.h"
#include "nav_fx3.h"
#include "camera.h"

/* One CPU target call at a time; NMI never enters creature logic. These
   fields also bridge the native target-info shell to the C mood rules. */
NavFx3* olEnemyNav;
ItemObj* olNavEnemy;
ItemObj* olNavLara;
NavPoint olEnemyPos, olTargetPos;
unsigned int olEnemyBox, olTargetBox;
static NavPoint viewPos;
#define enemyNav olEnemyNav
#define navEnemy olNavEnemy
#define navLara olNavLara
#define enemyPos olEnemyPos
#define targetPos olTargetPos
#define enemyBox olEnemyBox
#define targetBox olTargetBox

static uint32 Enemy_viewDistance(ItemObj* item)
{
    NavPoint p;
    int32 dx, dy, dz;
    uint32 d;
    Nav_point(&p, item->room, item->x, item->y, item->z);
    dx = (p.x - viewPos.x) >> 8;
    dy = (p.y - viewPos.y) >> 8;
    dz = (p.z - viewPos.z) >> 8;
    d = dx * dx;
    d += dy * dy;
    d += dz * dz;
    return d;
}

#ifdef OL_C_MOTION
NavFx3* Enemy_navAcquire(ItemObj* item, int forced)
#else
/* The native entry returns an already owned context without a C frame or
   repeated 8240-byte-stride multiplication. Keep admission/eviction here. */
NavFx3* Enemy_navAcquireCold(ItemObj* item, int forced)
#endif
{
    unsigned int owner = (unsigned int)(item - items), i, box;
    int slot = -1;
    NavFx3* n;
    uint32 farthest;
    for (i = 0; i < OL_NAV_SLOTS; i++) {
        if (olNav[i].owner == owner) return olNav + i;
    }
    box = navBoxAt(item->room, item->x, item->z);
    if (box >= olBoxCount) return 0;
    for (i = 0; i < OL_NAV_SLOTS; i++) {
        unsigned int other = olNav[i].owner;
        if (other == NO_BOX) { slot = i; break; }
        if (ItemObj_status(items + other) == ITEM_STATUS_INACTIVE) { slot = i; break; }
        if (!(items[other].flags & ITEM_FLAG_ACTIVE)) { slot = i; break; }
    }
    if (slot < 0) {
        Nav_point(&viewPos, camera.room, camera.x, camera.y, camera.z);
        farthest = forced ? 0 : Enemy_viewDistance(item);
        for (i = 0; i < OL_NAV_SLOTS; i++) {
            uint32 dist = Enemy_viewDistance(items + olNav[i].owner);
            if (dist > farthest) { farthest = dist; slot = i; }
        }
    }
    if (slot < 0) return 0;
    n = olNav + slot;
    if (n->owner != NO_BOX) {
        ItemObj* old = items + n->owner;
        if (ItemObj_status(old) != ITEM_STATUS_INACTIVE) {
            ItemObj_setStatus(old, ITEM_STATUS_INVISIBLE);
            ItemObj_touch(old);
        }
    }
    n->owner = owner;
    n->stepHeight = 256;
    n->dropHeight = (item->type == 7 || (item->type >= 12 && item->type <= 14)) ? -1024 : -256;
    n->vSpeed = 0;
    if (item->type == 9) {
        n->stepHeight = 20480; n->dropHeight = -20480; n->vSpeed = 16;
    }
    olEnemyTurn[owner] = 0;
    Nav_init(n, box);
    return n;
}

void Enemy_navRelease(ItemObj* item)
{
    unsigned int i, owner = (unsigned int)(item - items);
    for (i = 0; i < OL_NAV_SLOTS; i++) {
        if (olNav[i].owner == owner) olNav[i].owner = NO_BOX;
    }
}

#ifdef OL_C_MOTION
static int Enemy_checkZone(unsigned int box)
{
    if (box >= olBoxCount) return 0;
    if (olZone(box, enemyNav->zoneType) != olZone(enemyBox, enemyNav->zoneType)) return 0;
    if (enemyPos.x <= ((int32)olBoxMinX(box) << 10)) return 1;
    if (enemyPos.x >= ((int32)olBoxMaxX(box) << 10) - 1) return 1;
    if (enemyPos.z <= ((int32)olBoxMinZ(box) << 10)) return 1;
    if (enemyPos.z >= ((int32)olBoxMaxZ(box) << 10) - 1) return 1;
    return 0;
}
#else
int Enemy_checkZone(unsigned int box);
#endif

#ifdef OL_C_MOTION
static int Enemy_quadrant(int32 dx, int32 dz)
{
    if (dz > 0) return dx > 0 ? 2 : 1;
    return dx > 0 ? 3 : 0;
}

static int Enemy_checkStalk(unsigned int box)
{
    int32 dx = ((int32)(olBoxMinX(box) + olBoxMaxX(box)) << 9) - targetPos.x;
    int32 dz = ((int32)(olBoxMinZ(box) + olBoxMaxZ(box)) << 9) - targetPos.z;
    int qLara, qBox, qEnemy, diff;
    if (dx < -3072) return 0;
    if (dx > 3072) return 0;
    if (dz < -3072) return 0;
    if (dz > 3072) return 0;
    qLara = ((int16)navLara->angleY >> 14) + 2;
    qBox = Enemy_quadrant(dx, dz);
    if (qLara == qBox) return 0;
    diff = qLara - qBox;
    if (diff != 2 && diff != -2) return 1;
    qEnemy = Enemy_quadrant(enemyPos.x - targetPos.x, enemyPos.z - targetPos.z);
    return qLara != qEnemy;
}
#else
int Enemy_checkStalk(unsigned int box);
#endif

static int Enemy_checkEscape(unsigned int box)
{
    int32 dx = ((int32)(olBoxMinX(box) + olBoxMaxX(box)) << 9) - targetPos.x;
    int32 dz = ((int32)(olBoxMinZ(box) + olBoxMaxZ(box)) << 9) - targetPos.z;
    int oppositeX, oppositeZ;
    if (dx > -5120) {
        if (dx < 5120) {
            if (dz > -5120) {
                if (dz < 5120) return 0;
            }
        }
    }
    oppositeX = (dx < 0) != (enemyPos.x < targetPos.x);
    oppositeZ = (dz < 0) != (enemyPos.z < targetPos.z);
    return !(oppositeX && oppositeZ);
}

static void Enemy_setNextBox(unsigned int box)
{
    int32 low, size;
    box &= 0x7FFF;
    if (box >= olBoxCount) return;
    enemyNav->nextBox = box;
    low = ((int32)olBoxMinX(box) << 10) + 512;
    size = ((int32)(olBoxMaxX(box) - olBoxMinX(box)) << 10) - 1024;
    enemyNav->pos.x = low + ((rand_logic() * size) >> 15);
    low = ((int32)olBoxMinZ(box) << 10) + 512;
    size = ((int32)(olBoxMaxZ(box) - olBoxMinZ(box)) << 10) - 1024;
    enemyNav->pos.z = low + ((rand_logic() * size) >> 15);
    enemyNav->pos.y = olBoxFloor(box);
    if (enemyNav->zoneType != OL_ZONE_FLY) enemyNav->pos.y -= 384;
}

#ifdef OL_C_MOTION
static unsigned int Enemy_randomBox(void)
{
    unsigned int index = RAND_LOGIC(enemyNav->cellsCount);
    return enemyNav->cells[index].boxIndex;
}

static int Enemy_moodDistance(void)
{
    int32 dx = targetPos.x - enemyPos.x, dz = targetPos.z - enemyPos.z, d;
    int offset = navEnemy->type == 7 ? 375 : navEnemy->type == 8 ? 500 :
        (navEnemy->type == 19 || (navEnemy->type >= 12 && navEnemy->type <= 14)) ? 400 : 0;
    if (offset) {
        int32 h = (phd_sin((int32)navEnemy->angleY) * offset) >> FIXED_SHIFT;
        dx = dx - h;
        h = (phd_cos((int32)navEnemy->angleY) * offset) >> FIXED_SHIFT;
        dz = dz - h;
    }
    if (dx < 0) dx = -dx;
    if (dz < 0) dz = -dz;
    d = dx > dz ? dx + (dz >> 1) : dz + (dx >> 1);
    return d > 32767 ? 32767 : (int)d;
}
#else
unsigned int Enemy_randomBox(void);
int Enemy_moodDistance(void);
#endif

static unsigned int Enemy_moodNormal(void)
{
    unsigned int m = navEnemy->mood;
    int hurt = navEnemy->flags2 & ITEM_FLAG_INJURED;
    if (m == MOOD_SLEEP || m == MOOD_STALK) {
        if (hurt) {
            int roll = rand_logic();
            if (roll < 0x800) return MOOD_ESCAPE;
            if (!olEnemyInZone) return MOOD_ESCAPE;
        }
        if (olEnemyInZone) {
            if (Enemy_moodDistance() < 3072) return MOOD_ATTACK;
            if (m == MOOD_STALK && enemyNav->nextBox == NO_BOX) return MOOD_ATTACK;
            return MOOD_STALK;
        }
        return m;
    }
    if (m == MOOD_ATTACK) {
        if (hurt) {
            if (rand_logic() < 0x800) return MOOD_ESCAPE;
        }
        return olEnemyInZone ? m : MOOD_SLEEP;
    }
    if (olEnemyInZone) {
        if (rand_logic() < 0x100) return MOOD_STALK;
    }
    return m;
}

#ifdef OL_C_MOTION
static void Enemy_moodChoose(void)
#else
void Enemy_moodChoose(void);
/* Native attack selection bypasses this complete fallback. Keeping the
   other cases together preserves their transition/PRNG order. */
void Enemy_moodChooseCold(void)
#endif
{
    unsigned int box;
    switch (navEnemy->mood) {
        case MOOD_ATTACK:
            if (rand_logic() >= (navEnemy->type == 12 ? 0x7FFF :
                (navEnemy->type == 7 || navEnemy->type == 13 || navEnemy->type == 14) ? 0x2000 :
                (navEnemy->type == 8 || navEnemy->type == 19) ? 0x4000 : 0x400)) return;
            enemyNav->pos.x = targetPos.x;
            enemyNav->pos.y = targetPos.y;
            enemyNav->pos.z = targetPos.z;
            enemyNav->nextBox = targetBox;
            if (enemyNav->zoneType == OL_ZONE_FLY) enemyNav->pos.y += olAnimBox(navLara->animIndex, 2);
            return;
        case MOOD_STALK:
            if (enemyNav->nextBox != NO_BOX) {
                if (Enemy_checkStalk(enemyNav->nextBox)) return;
            }
            box = Enemy_randomBox();
            if (!Enemy_checkZone(box)) return;
            if (Enemy_checkStalk(box)) { Enemy_setNextBox(box); return; }
            if (enemyNav->nextBox != NO_BOX) return;
            Enemy_setNextBox(box);
            if (!olEnemyInZone) navEnemy->mood = MOOD_SLEEP;
            return;
        case MOOD_ESCAPE:
            box = Enemy_randomBox();
            if (!Enemy_checkZone(box)) return;
            if (enemyNav->nextBox != NO_BOX) return;
            if (Enemy_checkEscape(box)) { Enemy_setNextBox(box); return; }
            if (!olEnemyInZone) return;
            if (!Enemy_checkStalk(box)) return;
            navEnemy->mood = MOOD_STALK;
            Enemy_setNextBox(box);
            return;
        default:
            box = Enemy_randomBox();
            if (!Enemy_checkZone(box)) return;
            if (Enemy_checkStalk(box)) {
                navEnemy->mood = MOOD_STALK; Enemy_setNextBox(box); return;
            }
            if (enemyNav->nextBox == NO_BOX) Enemy_setNextBox(box);
    }
}

static unsigned int Enemy_moodWild(void)
{
    unsigned int m = navEnemy->mood;
    if (m == MOOD_SLEEP || m == MOOD_STALK) {
        if (olEnemyInZone) return MOOD_ATTACK;
        return navEnemy->flags2 & ITEM_FLAG_INJURED ? MOOD_ESCAPE : m;
    }
    if (m == MOOD_ATTACK) return olEnemyInZone ? m : MOOD_SLEEP;
    return olEnemyInZone ? MOOD_ATTACK : m;
}

/* Preserve the original transition order and PRNG consumption. The native
   entry bypasses this only for an unchanged, uninjured, same-zone attack
   with a living target and an already seeded end box. */
#ifdef OL_C_MOTION
void Enemy_targetMood(void)
#else
void Enemy_targetMoodCold(void)
#endif
{
    unsigned int mood;
    if (navEnemy->mood != MOOD_ATTACK && enemyNav->nextBox != NO_BOX) {
        if (!Enemy_checkZone(enemyNav->endBox)) {
            if (!olEnemyInZone) navEnemy->mood = MOOD_SLEEP;
            enemyNav->nextBox = NO_BOX;
        }
    }
    if (navLara->health <= 0) mood = MOOD_SLEEP;
    else if (navEnemy->flags2 & ITEM_FLAG_ANIMATED) mood = Enemy_moodWild();
    else mood = Enemy_moodNormal();
    navEnemy->flags2 &= (uint8)~ITEM_FLAG_INJURED;
    if (mood != navEnemy->mood) {
        if (navEnemy->mood == MOOD_ATTACK) Enemy_setNextBox(enemyNav->endBox);
        enemyNav->nextBox = NO_BOX;
        navEnemy->mood = (uint8)mood;
    }
    Enemy_moodChoose();
    if (enemyNav->endBox == NO_BOX) Enemy_setNextBox(enemyBox);
}

#ifdef OL_C_MOTION
static int Enemy_directionFits(int32 dx, int32 dz)
{
    if (dx > 32767) return 0;
    if (dx < -32767) return 0;
    if (dz > 32767) return 0;
    if (dz < -32767) return 0;
    return 1;
}

/* Executable reference for the native target-info/steering shell. */
int Enemy_target(ItemObj* item, ItemObj* lara, int* tx, int* tz)
{
    int32 dx, dz;
    olEnemyInZone = 0;
    enemyNav = Enemy_navAcquire(item, 0);
    if (!enemyNav) return 0;
    navEnemy = item; navLara = lara;
    enemyBox = navBoxAt(item->room, item->x, item->z);
    targetBox = navBoxAt(lara->room, lara->x, lara->z);
    if (enemyBox >= olBoxCount) return 0;
    if (targetBox >= olBoxCount) return 0;
    Nav_point(&enemyPos, item->room, item->x, item->y, item->z);
    Nav_point(&targetPos, lara->room, lara->x, lara->y, lara->z);
    olEnemyInZone = olZone(enemyBox, enemyNav->zoneType) == olZone(targetBox, enemyNav->zoneType);
    Enemy_targetMood();
    Nav_getWaypoint(enemyNav, enemyBox, &enemyPos);
    dx = enemyNav->waypoint.x - enemyPos.x;
    dz = enemyNav->waypoint.z - enemyPos.z;
    /* Steering is 16-bit; scale both axes together to preserve direction. */
    while (!Enemy_directionFits(dx, dz)) { dx >>= 1; dz >>= 1; }
    *tx = (int)dx; *tz = (int)dz;
    return 1;
}
#endif

int Enemy_waypointY(ItemObj* item)
{
    unsigned int i, owner = (unsigned int)(item - items);
    for (i = 0; i < OL_NAV_SLOTS; i++) {
        if (olNav[i].owner == owner) return (int)olNav[i].waypoint.y;
    }
    return item->y;
}
