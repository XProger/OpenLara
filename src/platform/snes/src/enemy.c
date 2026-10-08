/* OpenLara - SNES / Super FX target
 * The port of src/fixed/enemy.h, as far as it goes.
 *
 * Upstream is 40 KB of one class per creature: each with its own states,
 * distances, damage, hit mask and aggression, and a Nav of its own. What is
 * here is their shared mood and path search, with separate wolf/bear/bat
 * controllers plus FX3's raptor and inherited Lion controllers. The table identifies each species,
 * navigation zone and health.
 *
 * The state numbers are upstream's own enums, not guesses. A creature's states
 * are its own and nothing in the level data says which is which, so a row is
 * what makes a creature exist.
 */

#include "level.h"
#include "item.h"
#include "nav.h"
#include "enemy.h"

unsigned int olEnemyInZone;

/* ITEM_ types, from common.h's enum. */
#define ITEM_WOLF   7
#define ITEM_BEAR   8
#define ITEM_BAT    9
#define ITEM_LION_MALE   12
#define ITEM_LION_FEMALE 13
#define ITEM_PUMA        14
#define ITEM_RAPTOR 19

/* Simplified creature rates, now expressed per 30 Hz game tick. */
#define ENEMY_NOTICE    (1024 * 5)
#define DIST_ATTACK     3072        /* enemy.h's own threshold */
#define DIST_STALK      (1024 * 2)  /* how far a stalker hangs back */

/* State, speed and death data now belong to the original species controllers.
   Only shared navigation kind and initial health remain in this table. */
#define CR_ZONE     0
#define CR_HEALTH   1
#define CR_COLS     2
#ifdef OL_FX3
#define CREATURES   7
#else
#define CREATURES   3
#endif

static const unsigned char creature[CREATURES * CR_COLS] = {
    /* zone health: wolf, bear, bat */
    0, 6,  0, 20,  1, 1
#ifdef OL_FX3
    , 0, 20  /* raptor: ground-1, health 20 */
    , 0, 30, 0, 25, 0, 40 /* lion male/female/puma: ground-1 */
#endif
};

int Enemy_row(unsigned int type)
{
    /* A switch, not a scan: ItemObj_class asks this for every creature-range
       type, and the scan multiplied its way through the table each time. */
    switch (type)
    {
        case ITEM_WOLF: return 0;
        case ITEM_BEAR: return CR_COLS;
        case ITEM_BAT:  return CR_COLS * 2;
#ifdef OL_FX3
        case ITEM_RAPTOR: return CR_COLS * 3;
        case ITEM_LION_MALE: return CR_COLS * 4;
        case ITEM_LION_FEMALE: return CR_COLS * 5;
        case ITEM_PUMA: return CR_COLS * 6;
#endif
        default:        return -1;
    }
}

int Enemy_health(unsigned int type)
{
    int k = Enemy_row(type);
    return (k < 0) ? 0 : creature[k + CR_HEALTH];
}

/* ---------------------------------------------------------------------------
 * uint32 Enemy::getMoodNormal()
 *
 * Upstream's, thresholds and rolls included. The rolls come out of upstream's
 * own PRNG so a creature that flinches does it for the same reason on every
 * run -- the sequence is behaviour, not noise.
 *
 * The rolls are kept out of the conditions rather than short-circuited into
 * them: this compiler gets `a && f()` wrong when f() is a call, and the
 * ATTACK arm below returned "escape" with `hurt` provably zero until it was
 * written out. See "Toolchain traps" in the README.
 * ------------------------------------------------------------------------ */
#if !defined(OL_FX3) && defined(OL_C_MOTION)
static unsigned int Enemy_getMoodNormal(ItemObj* item, int inZone, int dist)
{
    unsigned int m = item->mood;
    int hurt = (item->flags2 & ITEM_FLAG_INJURED);

    if (m == MOOD_SLEEP || m == MOOD_STALK)
    {
        if (hurt) {
            if (!inZone) return MOOD_ESCAPE;
            if (rand_logic() < 0x800) return MOOD_ESCAPE;
        }
        if (inZone) return (dist < DIST_ATTACK) ? MOOD_ATTACK : MOOD_STALK;
        return m;
    }

    if (m == MOOD_ATTACK)
    {
        if (hurt) {
            if (rand_logic() < 0x800) return MOOD_ESCAPE;
        }
        return inZone ? MOOD_ATTACK : MOOD_SLEEP;
    }

    if (inZone) {
        if (rand_logic() < 0x100) return MOOD_STALK;
    }
    return MOOD_ESCAPE;
}

/* Shared, cached navigation. The species owns its state graph and movement;
   this only chooses a mood and a direction in the appropriate zone. */
int Enemy_target(ItemObj* item, ItemObj* lara, int* tx, int* tz)
{
    int k = Enemy_row(item->type);
    int dx, dz;

    olEnemyInZone = 0;

    dx = lara->x - item->x;
    dz = lara->z - item->z;

    if (dx > ENEMY_NOTICE || dx < -ENEMY_NOTICE ||
        dz > ENEMY_NOTICE || dz < -ENEMY_NOTICE) {
        item->mood = MOOD_SLEEP;
        return 0;
    }

    {
        int flying = creature[k + CR_ZONE];
        int csx = olSectorX(item->room, item->x);
        int csz = olSectorZ(item->room, item->z);
        unsigned int mine = navBoxAt(item->room, item->x, item->z);
        unsigned int hers = navBoxAt(lara->room, lara->x, lara->z);
        int inZone, dist, adx, adz;
        unsigned int mood;

        if (mine == NO_BOX || hers == NO_BOX) {
            item->mood = MOOD_SLEEP;
            return 0;
        }

        inZone = (olZone(mine, flying) == olZone(hers, flying));
        olEnemyInZone = inZone;

        adx = (dx < 0) ? -dx : dx;
        adz = (dz < 0) ? -dz : dz;
        dist = (adx > adz) ? adx : adz;

        mood = lara->health <= 0 ? MOOD_SLEEP : Enemy_getMoodNormal(item, inZone, dist);
        item->mood = (uint8)mood;
        item->flags2 &= (uint8)~ITEM_FLAG_INJURED;  /* the flinch is spent
                                                       on one decision */

        if (mood == MOOD_SLEEP) {
            return 0;
        }

        if (mood == MOOD_ESCAPE) {
            dx = -dx;           /* away needs no path: anywhere will do */
            dz = -dz;
        }
        else
        {
            if (!flying && mood == MOOD_STALK && dist < DIST_STALK) {
                return 0;       /* close enough to watch her from */
            }

            if (mine != hers)
            {
                unsigned int next;

                if (navFrom[flying] != hers) navFlood(flying, hers);
                next = (mine < OL_BOX_MAX) ? navNext[flying][mine] : NO_BOX;

                if (next == NO_BOX) {   /* no way through to her at all */
                    return 0;
                }
                /* A direction, in sectors, scaled back to world units. Both
                   ends are absolute, so the difference is right wherever it
                   is in the level. */
                {
                    int sx = boxCentreX(next) - csx;
                    int sz = boxCentreZ(next) - csz;
                    /* Only the direction matters, and 32 sectors << 10 is
                       past sixteen bits. */
                    if (sx > 31) sx = 31; else if (sx < -31) sx = -31;
                    if (sz > 31) sz = 31; else if (sz < -31) sz = -31;
                    dx = sx << 10;
                    dz = sz << 10;
                }
            }
        }
    }

    *tx = dx;
    *tz = dz;
    return 1;
}
#endif /* Native GSU-2 uses enemy_target_gsu2.asm; FX3 uses enemy_nav.c. */

#if !defined(OL_FX3) || defined(OL_C_MOTION)
/* Native FX3 dispatch preserves this order and tail-calls the species. */
void Enemy_update(ItemObj* item, ItemObj* lara)
{
#ifdef OL_FX3
    if (item->health <= 0) {
        if (Enemy_corpseTick(item)) return;
    }
#endif
    if (ItemObj_status(item) == ITEM_STATUS_INACTIVE) {
#ifdef OL_FX3
        Enemy_navRelease(item);
#endif
        ItemObj_deactivate(item);
        return;
    }
#ifdef OL_FX3
    if (!Enemy_navAcquire(item, ItemObj_status(item) == ITEM_STATUS_NONE)) {
        ItemObj_setStatus(item, ITEM_STATUS_INVISIBLE);
        ItemObj_touch(item);
        return;
    }
    if (ItemObj_status(item) == ITEM_STATUS_INVISIBLE) {
        ItemObj_setStatus(item, ITEM_STATUS_ACTIVE);
        ItemObj_touch(item);
    }
#endif
    if (item->type == ITEM_WOLF) {
        Wolf_update(item, lara);
    } else if (item->type == ITEM_BAT) {
        Bat_update(item, lara);
#ifdef OL_FX3
    } else if (item->type == ITEM_RAPTOR) {
        Raptor_update(item, lara);
    } else if (item->type == ITEM_LION_MALE || item->type == ITEM_LION_FEMALE || item->type == ITEM_PUMA) {
        Lion_update(item, lara);
#endif
    } else {
        Bear_update(item, lara);
    }
}
#endif
