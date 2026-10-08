/* OpenLara - SNES / Super FX target
 * The playable slice of src/fixed/lara.h's state machine: dry land, the
 * hang and the climb, and the water. See lara.h for what is deliberately
 * not here yet.
 */

#include "lara.h"
#include "object.h"
#include "level.h"
#include "object.h"
#include "room.h"
#include "collision.h"
#ifdef OL_FX3
#include "enemy.h"
#endif
#include "sound.h"
#include "camera.h"
#ifdef OL_FX3
#include "fx.h"
#endif

int olLaraTurn;
int olLaraVSpeedHack;
uint16 olLaraMoveAngle;
unsigned int olLaraWaterState;
int olLaraAngleX;
#ifdef OL_FX3
int olLaraAngleZ;           /* upstream's angle.z, her lean: GS_ROLL (main.asm) */
#endif
int olLaraSwimTimer;
int olLaraOxygen;
unsigned int olLaraWeaponState;
RoomQuery olLaraQuery;
int       olLaraQueryOk;

/* Upstream's updateRoom(offset). On land the query is at her feet rather
   than upstream's mid-body: that is where every observer of hers already
   looked, and it is one query rather than two. In the water the offset is
   upstream's -- her radius down at the surface, so the room she is in is
   the water she floats on, and the water state can tell the surface from
   the air above it. A sector with no floor leaves roomFloor at
   OL_NO_FLOOR, which is the badPos test below. */
static void Lara_updateRoom(ItemObj* lara, int offset)
{
    olLaraQueryOk = Room_query(lara->room, lara->x, lara->y + offset, lara->z,
                               &olLaraQuery);
    if (olLaraQueryOk) {
        lara->room      = (uint8)olLaraQuery.room;
        lara->roomFloor = (int16)olLaraQuery.floor;
    } else {
        lara->roomFloor = OL_NO_FLOOR;
    }
}

/* ---------------------------------------------------------------------------
 * The helpers every state handler shares.
 * ------------------------------------------------------------------------ */

/* upstream's angleDec: toward zero by `d`, never through it. */
static int angleDec(int v, int d)
{
    if (v > 0)  { v -= d; if (v < 0) v = 0; }
    else if (v < 0) { v += d; if (v > 0) v = 0; }
    return v;
}

#define X_ABS(v)            ((v) < 0 ? -(v) : (v))
#define LARA_ANGLE(deg)     ((deg) * ANGLE_1)
#ifdef OL_FX3
#define LARA_TILT_ACCEL     OL_SPEED(LARA_ANGLE(3) / 2)
#define LARA_TILT_MAX       LARA_ANGLE(11)
#endif

/* bool Lara::s_checkDeath(int32 deathState) */
static int s_checkDeath(ItemObj* lara, int deathState)
{
    if (lara->health <= 0) {
        lara->goalState = (uint8)deathState;
        return 1;
    }
    return 0;
}

/* bool Lara::s_checkFront(int16 angleDelta, int32 radius)
 *
 * Upstream's: a throwaway CollisionInfo run through collideRoom at her
 * radius, asking whether the way `angleDelta` from her facing is a wall.
 * That lives in collision.c now that Room_query answers arbitrary points. */
static int s_checkFront(ItemObj* lara, unsigned int input, int angleDelta,
                        int radius)
{
    return Lara_checkFront(lara, input, angleDelta, radius);
}

/* bool Lara::s_getFront(int16 rot)
 *
 * The cheaper question COMPRESS asks before choosing a jump: is the floor
 * 256 units that way within a step of this one. */
static int s_getFront(ItemObj* lara, int rot)
{
    RoomQuery q;
    uint16 a = (uint16)(lara->angleY + rot);
    int x = lara->x + (int)(phd_sin((int32)a) >> (FIXED_SHIFT - 8));
    int y = lara->y - LARA_HEIGHT;
    int z = lara->z + (int)(phd_cos((int32)a) >> (FIXED_SHIFT - 8));
    int floor;

    if (!Room_query(lara->room, x, y, z, &q)) return 0;
    if (q.floor == OL_NO_FLOOR) return 0;
    floor = q.floor - lara->y;
    return floor >= -LARA_STEP_HEIGHT;
}

/* bool Lara::s_checkRoll()
 *
 * Above water, Up+Down starts TR's standing roll animation at frame two.
 * Its own EFFECT command turns Lara around during ROLL_END. */
static int s_checkRoll(ItemObj* lara, unsigned int input)
{
    /* Upstream answers no from the surface, and under water it would start
       TR2's roll -- so here it answers no there too. */
    if (olLaraWaterState != WATER_STATE_ABOVE) return 0;
    if ((input & (IN_UP | IN_DOWN)) != (IN_UP | IN_DOWN)) return 0;
    if (lara->state != STATE_RUN && lara->state != STATE_STOP) return 0;

    ItemObj_animSetFrame(lara,
        olModelFirstAnim(lara->model) + LARA_ANIM_ROLL, 2);
    lara->goalState = STATE_STOP;
    return 1;
}

/* void Lara::s_rotate(int32 maxSpeed, int32 tilt)
 *
 * The tilt is upstream's angle.z, and only the run asks for one: FX3 leans
 * her there (s_STATE_RUN, olLaraAngleZ, drawn by the GSU's ItemTilt).
 * GSU-2 has neither the bytes nor the renderer for it. */
static void s_rotate(unsigned int input, int maxSpeed)
{
    if (input & IN_LEFT) {
        olLaraTurn -= LARA_TURN_ACCEL;
        if (olLaraTurn < -maxSpeed) olLaraTurn = -maxSpeed;
    } else if (input & IN_RIGHT) {
        olLaraTurn += LARA_TURN_ACCEL;
        if (olLaraTurn > maxSpeed) olLaraTurn = maxSpeed;
    }
}

/* void Lara::s_checkWalk(int32 stopState)
 *
 * Written out rather than short-circuited: 816-tcc gets `a && f()` wrong when
 * the right side is a call. See "Toolchain traps" in the README. */
static void s_checkWalk(ItemObj* lara, unsigned int input, int stopState)
{
    int go = 0;

    if (input & IN_UP) {
        if (s_checkFront(lara, input, ANGLE_0, LARA_RADIUS + 4)) go = 1;
    }

    if (go) {
        lara->goalState = (uint8)((input & IN_WALK) ? STATE_WALK : STATE_RUN);
    } else {
        lara->goalState = (uint8)stopState;
    }
}

/* bool Lara::s_checkFall()
 *
 * Upstream's: past a downward speed she stops jumping and starts falling. The
 * Swan dives progress to the head-first fast dive instead of a feet-first fall. */
static int s_checkFall(ItemObj* lara)
{
    if (lara->vSpeed > LARA_FALL_SPEED) {
        lara->goalState = (lara->state == STATE_SWAN_DIVE) ? STATE_FAST_DIVE : STATE_FALL;
        return 1;
    }
    return 0;
}

/* ---------------------------------------------------------------------------
 * The state handlers, upstream's S_HANDLERs.
 * ------------------------------------------------------------------------ */

static void s_STATE_WALK(ItemObj* lara, unsigned int input)
{
    if (s_checkDeath(lara, STATE_STOP)) return;

    s_rotate(input, LARA_TURN_SLOW);
    s_checkWalk(lara, input, STATE_STOP);
}

static void s_STATE_RUN(ItemObj* lara, unsigned int input)
{
    if (s_checkDeath(lara, STATE_DEATH)) return;
    if (s_checkRoll(lara, input)) return;

    s_rotate(input, LARA_TURN_FAST);
#ifdef OL_FX3
    /* s_rotate(LARA_TURN_FAST, 1): the lean into the turn */
    if (input & IN_LEFT) {
        olLaraAngleZ -= LARA_TILT_ACCEL;
        if (olLaraAngleZ < -LARA_TILT_MAX) olLaraAngleZ = -LARA_TILT_MAX;
    } else if (input & IN_RIGHT) {
        olLaraAngleZ += LARA_TILT_ACCEL;
        if (olLaraAngleZ > LARA_TILT_MAX) olLaraAngleZ = LARA_TILT_MAX;
    }
#endif

    if ((input & IN_JUMP) && !(lara->flags2 & ITEM_FLAG_GRAVITY)) {
        lara->goalState = STATE_JUMP;
    } else {
        s_checkWalk(lara, input, STATE_STOP);
    }
}

/* bool s_checkLook(): Look held puts the camera in LOOK and the state holds
   still (the d-pad is the head's this tick -- see Lara_manualLook). game.c
   reads the flag when it picks the camera's mode. */
unsigned int olLaraLooking;

static int s_checkLook(unsigned int input)
{
    if (input & IN_LOOK) {
        olLaraLooking = 1;
        return 1;
    }
    return 0;
}

/* updateLook's manual branch: no target, Look held. The d-pad turns her head
   two degrees a tick, clamped, the torso with it; and it is spent here, so
   her state machine sees none of it. Everything else in updateLook -- death,
   tracking a target, the decay -- is weapon_anim.asm's Lara_updateLook. */
#define LARA_LOOK_ANGLE_MAX     (22 * ANGLE_1)
#define LARA_LOOK_ANGLE_MIN     (-42 * ANGLE_1)
#define LARA_LOOK_ANGLE_Y       (44 * ANGLE_1)
#define LARA_LOOK_TURN_SPEED    (2 * ANGLE_1)

unsigned int Lara_manualLook(unsigned int input)
{
    int x = olLaraLook.headX, y = olLaraLook.headY;

    if (input & IN_UP)    x -= LARA_LOOK_TURN_SPEED;
    if (input & IN_DOWN)  x += LARA_LOOK_TURN_SPEED;
    if (input & IN_LEFT)  y -= LARA_LOOK_TURN_SPEED;
    if (input & IN_RIGHT) y += LARA_LOOK_TURN_SPEED;
    if (x < LARA_LOOK_ANGLE_MIN) x = LARA_LOOK_ANGLE_MIN;
    if (x > LARA_LOOK_ANGLE_MAX) x = LARA_LOOK_ANGLE_MAX;
    if (y < -LARA_LOOK_ANGLE_Y) y = -LARA_LOOK_ANGLE_Y;
    if (y >  LARA_LOOK_ANGLE_Y) y =  LARA_LOOK_ANGLE_Y;
    olLaraLook.headX = olLaraLook.torsoX = (int16)x;
    olLaraLook.headY = olLaraLook.torsoY = (int16)y;

    return input & ~(IN_RIGHT | IN_LEFT | IN_UP | IN_DOWN);
}

static void s_STATE_STOP(ItemObj* lara, unsigned int input)
{
    if (s_checkDeath(lara, STATE_DEATH)) return;
    if (s_checkRoll(lara, input)) return;

    lara->goalState = STATE_STOP;

    if (s_checkLook(input)) return;

    if (input & IN_WALK) {
        /* Nested, not `&&` with the call on the right -- the port's rule
           since the mood-code miscompile -- and upstream's own shape. */
        if (input & IN_LEFT) {
            if (s_checkFront(lara, input, -ANGLE_90, LARA_RADIUS + 16)) {
                lara->goalState = STATE_STEP_LEFT;
            }
        } else if (input & IN_RIGHT) {
            if (s_checkFront(lara, input, ANGLE_90, LARA_RADIUS + 16)) {
                lara->goalState = STATE_STEP_RIGHT;
            }
        }
    } else {
        if (input & IN_LEFT) {
            lara->goalState = STATE_TURN_LEFT;
        } else if (input & IN_RIGHT) {
            lara->goalState = STATE_TURN_RIGHT;
        }
    }

    if (input & IN_JUMP) {
        lara->goalState = STATE_COMPRESS;
    } else if (input & IN_UP) {
        if (s_checkFront(lara, input, ANGLE_0, LARA_RADIUS + 4)) {
            lara->goalState = (uint8)((input & IN_WALK) ? STATE_WALK : STATE_RUN);
        }
    } else if (input & IN_DOWN) {
        if (s_checkFront(lara, input, (int)ANGLE_180, LARA_RADIUS + 4)) {
            if (input & IN_WALK) {
                lara->goalState = STATE_BACK;
                s_rotate(input, LARA_TURN_SLOW);
            } else {
                lara->goalState = STATE_BACK_FAST;
            }
        }
    }
}

static void s_STATE_TURN_RIGHT(ItemObj* lara, unsigned int input)
{
    if (s_checkDeath(lara, STATE_STOP)) return;

    olLaraTurn += LARA_TURN_ACCEL;

    if (olLaraTurn > LARA_TURN_SLOW || olLaraWeaponState == WEAPON_STATE_READY) {
        if (!(input & IN_WALK) && olLaraWaterState != WATER_STATE_WADE) {
            lara->goalState = STATE_TURN_FAST;
        }
    }

    if (lara->goalState == lara->state) {
        if (olLaraTurn > LARA_TURN_SLOW) olLaraTurn = LARA_TURN_SLOW;
    }

    s_checkWalk(lara, input,
                (input & IN_RIGHT) ? lara->goalState : STATE_STOP);
}

static void s_STATE_TURN_LEFT(ItemObj* lara, unsigned int input)
{
    if (s_checkDeath(lara, STATE_STOP)) return;

    olLaraTurn -= LARA_TURN_ACCEL;

    if (olLaraTurn < -LARA_TURN_SLOW || olLaraWeaponState == WEAPON_STATE_READY) {
        if (!(input & IN_WALK) && olLaraWaterState != WATER_STATE_WADE) {
            lara->goalState = STATE_TURN_FAST;
        }
    }

    if (lara->goalState == lara->state) {
        if (olLaraTurn < -LARA_TURN_SLOW) olLaraTurn = -LARA_TURN_SLOW;
    }

    s_checkWalk(lara, input,
                (input & IN_LEFT) ? lara->goalState : STATE_STOP);
}

static void s_STATE_TURN_FAST(ItemObj* lara, unsigned int input)
{
    if (s_checkDeath(lara, STATE_STOP)) return;

    if (olLaraTurn < 0) {
        olLaraTurn = -LARA_TURN_FAST;
        if (!(input & IN_LEFT)) lara->goalState = STATE_STOP;
    } else {
        olLaraTurn = LARA_TURN_FAST;
        if (!(input & IN_RIGHT)) lara->goalState = STATE_STOP;
    }
}

static void s_STATE_BACK_FAST(ItemObj* lara, unsigned int input)
{
    /* Upstream's, whole: it always asks for STOP, which is what makes the
       backward hop a hop rather than a walk -- the animation plays out and
       she stops, and holding down again starts another. */
    s_rotate(input, LARA_TURN_MED);
    lara->goalState = STATE_STOP;
}

/* ---------------------------------------------------------------------------
 * The air. She leaves the ground because an animation command says so --
 * ANIM_CMD_JUMP carries the push, so nothing here decides how high she goes --
 * and gravity owns her Y from then until the floor takes it back.
 * ------------------------------------------------------------------------ */

static void s_STATE_COMPRESS(ItemObj* lara, unsigned int input)
{
    if (input & IN_UP) {
        if (s_getFront(lara, ANGLE_0)) lara->goalState = STATE_JUMP;
    } else if (input & IN_LEFT) {
        if (s_getFront(lara, -ANGLE_90)) lara->goalState = STATE_JUMP_LEFT;
    } else if (input & IN_RIGHT) {
        if (s_getFront(lara, ANGLE_90)) lara->goalState = STATE_JUMP_RIGHT;
    } else if (input & IN_DOWN) {
        if (s_getFront(lara, (int)ANGLE_180)) lara->goalState = STATE_JUMP_BACK;
    }

    s_checkFall(lara);
}

/* void Lara::s_ignoreEnemy() */
static void s_ignoreEnemy(void)
{
    cinfo.enemyPush = 0;
    cinfo.enemyHit  = 0;
}

static void s_STATE_JUMP(ItemObj* lara, unsigned int input)
{
    if (lara->goalState == STATE_SWAN_DIVE || lara->goalState == STATE_REACH)
        lara->goalState = STATE_JUMP;

    if (lara->goalState != STATE_DEATH &&
        lara->goalState != STATE_STOP &&
        lara->goalState != STATE_RUN)
    {
        /* Walk takes priority over Action when both are held, upstream's
           order. Both manoeuvres need free hands. */
        if (olLaraWeaponState == WEAPON_STATE_FREE) {
            if (input & IN_ACTION) lara->goalState = STATE_REACH;
            if (input & IN_WALK) lara->goalState = STATE_SWAN_DIVE;
        }

        s_checkRoll(lara, input);
        s_checkFall(lara);
    }

    s_rotate(input, LARA_TURN_JUMP);
}

static void s_STATE_HANG(ItemObj* lara, unsigned int input)
{
    camera.askAngleX = -60 * ANGLE_1;
    CAM_ASKED();
    s_ignoreEnemy();
    if (input & IN_LEFT) {
        lara->goalState = STATE_HANG_LEFT;
    } else if (input & IN_RIGHT) {
        lara->goalState = STATE_HANG_RIGHT;
    }
}

static void s_STATE_REACH(ItemObj* lara, unsigned int input)
{
    camera.askAngleY = 85 * ANGLE_1;
    CAM_ASKED();
    s_checkFall(lara);
}

static void s_STATE_HANG_UP(ItemObj* lara, unsigned int input)
{
    s_ignoreEnemy();
}

static void s_STATE_HANG_LEFT(ItemObj* lara, unsigned int input)
{
    camera.askAngleX = -60 * ANGLE_1;
    CAM_ASKED();
    s_ignoreEnemy();
    if (!(input & IN_LEFT)) lara->goalState = STATE_HANG;
}

static void s_STATE_HANG_RIGHT(ItemObj* lara, unsigned int input)
{
    camera.askAngleX = -60 * ANGLE_1;
    CAM_ASKED();
    s_ignoreEnemy();
    if (!(input & IN_RIGHT)) lara->goalState = STATE_HANG;
}

static void s_STATE_FALL_BACK(ItemObj* lara, unsigned int input)
{
    s_checkFall(lara);
    if ((input & IN_ACTION) && olLaraWeaponState == WEAPON_STATE_FREE)
        lara->goalState = STATE_REACH;
}

static void s_STATE_FALL(ItemObj* lara, unsigned int input)
{
    /* Upstream's: air drag, five percent of her forward speed a step, and
       the scream once she is falling fast enough to die of it. The sound is
       UNIQUE, so calling it every step keeps it going rather than restarting. */
    lara->hSpeed = (int16)(((int32)lara->hSpeed * 95) / 100);
    if (lara->vSpeed >= 154) {
        Sound_playItem(SND_SCREAM, lara);
    }
}

static void s_STATE_DIVE(ItemObj* lara, unsigned int input)
{
    cinfo.enemyPush = 1;
    cinfo.enemyHit = 0;
    if (lara->state == STATE_SWAN_DIVE) {
        s_checkFall(lara);
    } else {
        s_STATE_FALL(lara, input);
        /* s_checkRoll only accepts STOP/RUN in TR1; a fast dive cannot
           turn into the standing roll. */
    }
}

static void s_STATE_LAND(ItemObj* lara, unsigned int input)
{
    /* Upstream's is empty: landing is the animation, and the graph leads out
       of it on its own. */
}

static void s_STATE_DEATH(ItemObj* lara, unsigned int input)
{
    lara->goalState = STATE_DEATH;
}

static void s_STATE_BACK(ItemObj* lara, unsigned int input)
{
    if (s_checkDeath(lara, STATE_STOP)) return;

    if ((input & (IN_WALK | IN_DOWN)) != (IN_WALK | IN_DOWN)) {
        lara->goalState = STATE_STOP;
    } else {
        lara->goalState = STATE_BACK;
    }
    s_rotate(input, LARA_TURN_SLOW);
}

static void s_STATE_STEP_RIGHT(ItemObj* lara, unsigned int input)
{
    if (s_checkDeath(lara, STATE_STOP)) return;
    if ((input & (IN_WALK | IN_RIGHT)) != (IN_WALK | IN_RIGHT))
        lara->goalState = STATE_STOP;
}

static void s_STATE_STEP_LEFT(ItemObj* lara, unsigned int input)
{
    if (s_checkDeath(lara, STATE_STOP)) return;
    if ((input & (IN_WALK | IN_LEFT)) != (IN_WALK | IN_LEFT))
        lara->goalState = STATE_STOP;
}

static void s_STATE_JUMP_BACK(ItemObj* lara, unsigned int input)
{
    camera.askAngleY = 135 * ANGLE_1;
    CAM_ASKED();
    if (s_checkFall(lara)) return;
    if (lara->goalState == STATE_RUN) lara->goalState = STATE_STOP;
}

static void s_STATE_JUMP_SIDE(ItemObj* lara)
{
    s_checkFall(lara);
}

/* The states only the collision handlers reach: a slide is started by
   c_checkSlide on a steep floor, a splat by running into a wall. */
static void s_STATE_SLIDE(ItemObj* lara, unsigned int input)
{
    camera.askAngleX = -45 * ANGLE_1;
    CAM_ASKED();
    if (input & IN_JUMP) lara->goalState = STATE_JUMP;
}

static void s_STATE_SLIDE_BACK(ItemObj* lara, unsigned int input)
{
    if (input & IN_JUMP) lara->goalState = STATE_JUMP_BACK;
}

/* ---------------------------------------------------------------------------
 * The water: upstream's s_turnUW and s_dive, and the handlers of the
 * eleven states she swims, floats, drowns and climbs out in. vSpeed is her
 * swimming speed here -- TR reuses the field -- decayed by a friction and
 * pushed by an acceleration, never below zero. Her pitch is olLaraAngleX,
 * TR's way round: Up puts her nose down. Her lean (FX3) twice the run's
 * rate, clamped at 22 degrees by updateUnder.
 * ------------------------------------------------------------------------ */

/* void Lara::s_turnUW() */
static void s_turnUW(unsigned int input)
{
    if (input & IN_UP) {
        olLaraAngleX -= LARA_UW_PITCH;
    } else if (input & IN_DOWN) {
        olLaraAngleX += LARA_UW_PITCH;
    }

    if (input & IN_LEFT) {
        olLaraTurn -= LARA_TURN_ACCEL;
        if (olLaraTurn < -LARA_TURN_MED) olLaraTurn = -LARA_TURN_MED;
#ifdef OL_FX3
        olLaraAngleZ -= LARA_TILT_ACCEL * 2;
#endif
    } else if (input & IN_RIGHT) {
        olLaraTurn += LARA_TURN_ACCEL;
        if (olLaraTurn > LARA_TURN_MED) olLaraTurn = LARA_TURN_MED;
#ifdef OL_FX3
        olLaraAngleZ += LARA_TILT_ACCEL * 2;
#endif
    }
}

/* void Lara::s_dive() */
void Lara_dive(ItemObj* lara)
{
    ItemObj_animSet(lara, olModelFirstAnim(lara->model) + LARA_ANIM_SURF_DIVE);
    olLaraAngleX     = LARA_ANGLE(-45);
    lara->vSpeed     = LARA_DIVE_SPEED;
    olLaraWaterState = WATER_STATE_UNDER;
}

static void s_swimDecay(ItemObj* lara, int friction)
{
    int v = lara->vSpeed - friction;
    if (v < 0) v = 0;
    lara->vSpeed = (int16)v;
}

static void s_swimPush(ItemObj* lara, int accel, int cap)
{
    int v = lara->vSpeed + accel;
    if (v > cap) v = cap;
    lara->vSpeed = (int16)v;
}

static void s_STATE_UW_TREAD(ItemObj* lara, unsigned int input)
{
    if (s_checkDeath(lara, STATE_DEATH_UW)) return;
    if (s_checkRoll(lara, input)) return;

    s_turnUW(input);

    if (input & IN_JUMP) lara->goalState = STATE_UW_SWIM;

    s_swimDecay(lara, LARA_SWIM_FRICTION);
}

static void s_STATE_UW_SWIM(ItemObj* lara, unsigned int input)
{
    if (s_checkDeath(lara, STATE_DEATH_UW)) return;
    if (s_checkRoll(lara, input)) return;

    s_turnUW(input);

    s_swimPush(lara, LARA_SWIM_ACCEL, LARA_SWIM_SPEED_MAX);

    if (!(input & IN_JUMP)) lara->goalState = STATE_UW_GLIDE;
}

static void s_STATE_UW_GLIDE(ItemObj* lara, unsigned int input)
{
    if (s_checkDeath(lara, STATE_DEATH_UW)) return;
    if (s_checkRoll(lara, input)) return;

    s_turnUW(input);

    if (input & IN_JUMP) lara->goalState = STATE_UW_SWIM;

    s_swimDecay(lara, LARA_SWIM_FRICTION);

    if (lara->vSpeed <= LARA_SWIM_SPEED_MIN) lara->goalState = STATE_UW_TREAD;
}

static void s_STATE_UW_DIVE(unsigned int input)
{
    if (input & IN_UP) olLaraAngleX -= ANGLE_1;
}

static void s_STATE_DEATH_UW(ItemObj* lara)
{
    s_swimDecay(lara, LARA_SWIM_ACCEL);
    olLaraAngleX = angleDec(olLaraAngleX, LARA_ANGLE(2));
}

/* At the surface she turns her heading directly, with nothing to decay --
   upstream's, and why updateSurface has no turn in its tail. */
static void s_surfTurn(ItemObj* lara, unsigned int input, int rate)
{
    if (input & IN_LEFT) {
        lara->angleY = (uint16)(lara->angleY - rate);
    } else if (input & IN_RIGHT) {
        lara->angleY = (uint16)(lara->angleY + rate);
    }
}

static void s_STATE_SURF_TREAD(ItemObj* lara, unsigned int input)
{
    s_swimDecay(lara, LARA_SURF_FRICTION);

    if (s_checkDeath(lara, STATE_DEATH_UW)) return;

    if (s_checkLook(input)) return;

    s_surfTurn(lara, input, LARA_TURN_SLOW);

    if (input & IN_UP) {
        lara->goalState = STATE_SURF_SWIM;
    } else if (input & IN_DOWN) {
        lara->goalState = STATE_SURF_BACK;
    } else if ((input & (IN_WALK | IN_LEFT)) == (IN_WALK | IN_LEFT)) {
        lara->goalState = STATE_SURF_LEFT;
    } else if ((input & (IN_WALK | IN_RIGHT)) == (IN_WALK | IN_RIGHT)) {
        lara->goalState = STATE_SURF_RIGHT;
    }

    /* A third of a second of Jump dives; a tap does not. */
    if (input & IN_JUMP) {
        olLaraSwimTimer++;
        if (olLaraSwimTimer == LARA_SWIM_TIMER) Lara_dive(lara);
    } else {
        olLaraSwimTimer = 0;
    }
}

static void s_STATE_SURF_SWIM(ItemObj* lara, unsigned int input)
{
    if (s_checkDeath(lara, STATE_DEATH_UW)) return;

    olLaraSwimTimer = 0;

    s_surfTurn(lara, input, LARA_TURN_SLOW);

    if (!(input & IN_UP) || (input & IN_JUMP)) lara->goalState = STATE_SURF_TREAD;

    s_swimPush(lara, LARA_SURF_ACCEL, LARA_SURF_SPEED_MAX);
}

static void s_STATE_SURF_BACK(ItemObj* lara, unsigned int input)
{
    if (s_checkDeath(lara, STATE_DEATH_UW)) return;

    olLaraSwimTimer = 0;

    s_surfTurn(lara, input, LARA_TURN_VERY_SLOW);

    if (!(input & IN_DOWN)) lara->goalState = STATE_SURF_TREAD;

    s_swimPush(lara, LARA_SURF_ACCEL, LARA_SURF_SPEED_MAX);
}

/* SURF_LEFT and SURF_RIGHT, which differ only in the direction held. */
static void s_STATE_SURF_SIDE(ItemObj* lara, unsigned int input, unsigned int held)
{
    if (s_checkDeath(lara, STATE_DEATH_UW)) return;

    olLaraSwimTimer = 0;

    if ((input & (IN_WALK | held)) != (IN_WALK | held)) {
        lara->goalState = STATE_SURF_TREAD;
    }

    s_swimPush(lara, LARA_SURF_ACCEL, LARA_SURF_SPEED_MAX);
}

static void s_STATE_WATER_OUT(void)
{
    s_ignoreEnemy();
    camera.center = 1;
    CAM_ASKED();
}

/* ---------------------------------------------------------------------------
 * void Lara::updateState()
 *
 * Upstream dispatches through a table of member-function pointers, one per
 * state, built by the same macro that declares the enum. There is no table
 * here because half the states have no handler yet: the switch has an arm
 * for each that exists and does nothing for the rest, which is what
 * upstream's unimplemented handlers do too.
 * ------------------------------------------------------------------------ */
void Lara_updateState(ItemObj* lara, unsigned int input)
{
    switch (lara->state)
    {
        case STATE_WALK:        s_STATE_WALK(lara, input); break;
        case STATE_RUN:         s_STATE_RUN(lara, input); break;
        case STATE_STOP:        s_STATE_STOP(lara, input); break;
        case STATE_TURN_RIGHT:  s_STATE_TURN_RIGHT(lara, input); break;
        case STATE_TURN_LEFT:   s_STATE_TURN_LEFT(lara, input); break;
        case STATE_TURN_FAST:   s_STATE_TURN_FAST(lara, input); break;
        case STATE_BACK_FAST:   s_STATE_BACK_FAST(lara, input); break;
        case STATE_COMPRESS:    s_STATE_COMPRESS(lara, input); break;
        case STATE_JUMP:        s_STATE_JUMP(lara, input); break;
        case STATE_FALL:        s_STATE_FALL(lara, input); break;
        case STATE_LAND:        s_STATE_LAND(lara, input); break;
        case STATE_DEATH:       s_STATE_DEATH(lara, input); break;
        case STATE_BACK:        s_STATE_BACK(lara, input); break;
        case STATE_STEP_RIGHT:  s_STATE_STEP_RIGHT(lara, input); break;
        case STATE_STEP_LEFT:   s_STATE_STEP_LEFT(lara, input); break;
        case STATE_JUMP_BACK:   s_STATE_JUMP_BACK(lara, input); break;
        case STATE_JUMP_RIGHT:  s_STATE_JUMP_SIDE(lara); break;
        case STATE_JUMP_LEFT:   s_STATE_JUMP_SIDE(lara); break;
        case STATE_JUMP_UP:     s_STATE_JUMP_SIDE(lara); break;
        case STATE_FALL_BACK:   s_STATE_FALL_BACK(lara, input); break;
        case STATE_HANG:        s_STATE_HANG(lara, input); break;
        case STATE_REACH:       s_STATE_REACH(lara, input); break;
        case STATE_HANDSTAND:
        case STATE_HANG_UP:     s_STATE_HANG_UP(lara, input); break;
        case STATE_SWAN_DIVE:
        case STATE_FAST_DIVE:   s_STATE_DIVE(lara, input); break;
        case STATE_HANG_LEFT:   s_STATE_HANG_LEFT(lara, input); break;
        case STATE_HANG_RIGHT:  s_STATE_HANG_RIGHT(lara, input); break;
        case STATE_SLIDE:       s_STATE_SLIDE(lara, input); break;
        case STATE_SLIDE_BACK:  s_STATE_SLIDE_BACK(lara, input); break;
        case STATE_UW_TREAD:    s_STATE_UW_TREAD(lara, input); break;
        case STATE_UW_SWIM:     s_STATE_UW_SWIM(lara, input); break;
        case STATE_UW_GLIDE:    s_STATE_UW_GLIDE(lara, input); break;
        case STATE_UW_DIVE:     s_STATE_UW_DIVE(input); break;
        case STATE_DEATH_UW:    s_STATE_DEATH_UW(lara); break;
        case STATE_SURF_TREAD:  s_STATE_SURF_TREAD(lara, input); break;
        case STATE_SURF_SWIM:   s_STATE_SURF_SWIM(lara, input); break;
        case STATE_SURF_BACK:   s_STATE_SURF_BACK(lara, input); break;
        case STATE_SURF_LEFT:   s_STATE_SURF_SIDE(lara, input, IN_LEFT); break;
        case STATE_SURF_RIGHT:  s_STATE_SURF_SIDE(lara, input, IN_RIGHT); break;
        case STATE_WATER_OUT:   s_STATE_WATER_OUT(); break;
        /* The interactions: upstream's handlers point the camera -- to her
           side, closer -- and hold the enemies off. BLOCK_READY lets go of
           the block when Action does. */
        case STATE_BLOCK_READY:
            camera.askAngleY = 75 * ANGLE_1;
            CAM_ASKED();
            s_ignoreEnemy();
            if (!(input & IN_ACTION)) lara->goalState = STATE_STOP;
            break;
        case STATE_BLOCK_PUSH:
        case STATE_BLOCK_PULL:
            camera.askAngleX = -25 * ANGLE_1;
            camera.askAngleY = 35 * ANGLE_1;
            camera.center = 1;
            CAM_ASKED();
            s_ignoreEnemy();
            break;
        case STATE_PICKUP:
            camera.askAngleX = -15 * ANGLE_1;
            camera.askAngleY = -130 * ANGLE_1;
            camera.askDist = 1024;
            CAM_ASKED();
            s_ignoreEnemy();
            break;
        case STATE_SWITCH_DOWN:
        case STATE_SWITCH_UP:
            camera.askAngleX = -25 * ANGLE_1;
            camera.askAngleY = 80 * ANGLE_1;
            camera.askDist = 1024;
            CAM_ASKED();
            s_ignoreEnemy();
            break;
        case STATE_USE_KEY:
        case STATE_USE_PUZZLE:
            camera.askAngleX = -25 * ANGLE_1;
            camera.askAngleY = -80 * ANGLE_1;
            camera.askDist = 1024;
            CAM_ASKED();
            s_ignoreEnemy();
            break;
        case STATE_SPLAT:       break;      /* upstream's is empty */
        case STATE_ROLL_START:  break;
        case STATE_ROLL_END:    break;
        default:                break;
    }
}

/* ---------------------------------------------------------------------------
 * void Lara::updateWaterState()
 *
 * Which of the four ways of being in the world she is in, decided from the
 * water column under her: the surface of the water below or around her,
 * and how far the floor lies under that surface. Room_water answers both in
 * one walk. A change of state here sets the animation outright, as
 * upstream does: the graph has no transition from falling to diving.
 *
 * TR1's Lara has no wading animations -- ANIM_WADE and its kin are TR2's --
 * so the arms that would play them play what TR1 has, standing, and TR1's
 * own behaviour in the shallows is to swim. WADE remains a state of the
 * water column that the collision handlers read, because upstream reaches
 * it from a shallow pool whether or not the level can animate it.
 * ------------------------------------------------------------------------ */
static void Lara_updateWaterState(ItemObj* lara)
{
    int level, depth, dist;
    int inWater = olRoomWater(lara->room) ? 1 : 0;
    unsigned int base = olModelFirstAnim(lara->model);

    Room_water(lara->room, lara->x, lara->z, &level, &depth);

    dist = WALL;
    if (level != WALL) dist = lara->y - level;

    switch (olLaraWaterState)
    {
        case WATER_STATE_ABOVE:
            if (dist == WALL || dist < LARA_WADE_MIN_DEPTH) break;

            if (depth > LARA_WADE_MAX_DEPTH - 256)
            {
                if (!inWater) break;        /* still falling toward it */

                olLaraWaterState = WATER_STATE_UNDER;
                lara->flags2 &= (uint8)~ITEM_FLAG_GRAVITY;
                olLaraOxygen = LARA_MAX_OXYGEN;
                Sound_stop(SND_SCREAM);

                lara->y = (int16)(lara->y + 100);
                Lara_updateRoom(lara, 0);

                olLaraAngleX = LARA_ANGLE(-45);
                if (lara->state == STATE_SWAN_DIVE || lara->state == STATE_FAST_DIVE) {
                    if (lara->state == STATE_FAST_DIVE) olLaraAngleX = LARA_ANGLE(-85);
                    lara->goalState = STATE_UW_DIVE;
                    /* Upstream animProcess includes movement; this target
                       keeps the two operations separate. Preserve both. */
                    ItemObj_animProcess(lara);
                    ItemObj_move(lara);
                    lara->vSpeed = (int16)(lara->vSpeed * 2);
                } else {
                    ItemObj_animSet(lara, base + LARA_ANIM_WATER_FALL);
                    lara->state     = STATE_UW_DIVE;
                    lara->goalState = STATE_UW_SWIM;
                    lara->vSpeed    = (int16)(((int32)lara->vSpeed * 3) / 2);
                }
#ifdef OL_FX3
                Fx_splash(lara);
#endif
            }
            else if (dist > LARA_WADE_MIN_DEPTH)
            {
                olLaraWaterState = WATER_STATE_WADE;
                if (!(lara->flags2 & ITEM_FLAG_GRAVITY)) {
                    lara->goalState = STATE_STOP;
                }
            }
            break;

        case WATER_STATE_SURFACE:
            if (inWater) break;

            if (dist > LARA_WADE_MIN_DEPTH) {
                /* upstream: ANIM_STAND_NORMAL toward STATE_WADE, TR2's */
                olLaraWaterState = WATER_STATE_WADE;
                ItemObj_animSet(lara, base + LARA_ANIM_STAND_NORMAL);
            } else {
                olLaraWaterState = WATER_STATE_ABOVE;
                ItemObj_animSet(lara, base + LARA_ANIM_FALL_FORTH);
                lara->hSpeed = (int16)(lara->vSpeed / 4);
                lara->flags2 |= ITEM_FLAG_GRAVITY;
            }

            lara->vSpeed = 0;
            olLaraAngleX = 0;
#ifdef OL_FX3
            olLaraAngleZ = 0;
#endif
            break;

        case WATER_STATE_UNDER:
            if (inWater) break;

            if (depth != WALL && X_ABS(dist) < 256) {
                olLaraWaterState = WATER_STATE_SURFACE;
                lara->y = (int16)(lara->y - (dist - 1));
                ItemObj_animSet(lara, base + LARA_ANIM_SURF);
                lara->vSpeed = 0;
                /* no dive until Jump is pressed afresh */
                olLaraSwimTimer = LARA_SWIM_TIMER + 1;
                Lara_updateRoom(lara, -LARA_HEIGHT / 2);
            } else {
                olLaraWaterState = WATER_STATE_ABOVE;
                ItemObj_animSet(lara, base + LARA_ANIM_FALL_FORTH);
                lara->hSpeed = (int16)(lara->vSpeed / 4);
                lara->vSpeed = 0;
                lara->flags2 |= ITEM_FLAG_GRAVITY;
            }

            olLaraAngleX = 0;
#ifdef OL_FX3
            olLaraAngleZ = 0;
#endif
            break;

        case WATER_STATE_WADE:
            if (dist < LARA_WADE_MIN_DEPTH)
            {
                /* upstream sends a STATE_WADE on to RUN; TR1 never stood
                   in one */
                olLaraWaterState = WATER_STATE_ABOVE;
            }
            else if (dist > LARA_WADE_MAX_DEPTH)
            {
                olLaraWaterState = WATER_STATE_SURFACE;
                lara->y = (int16)(lara->y - (dist - 1));

                if (lara->state == STATE_BACK) {
                    ItemObj_animSet(lara, base + LARA_ANIM_SURF_BACK);
                } else if (lara->state == STATE_STEP_RIGHT) {
                    ItemObj_animSet(lara, base + LARA_ANIM_SURF_RIGHT);
                } else if (lara->state == STATE_STEP_LEFT) {
                    ItemObj_animSet(lara, base + LARA_ANIM_SURF_LEFT);
                } else {
                    ItemObj_animSet(lara, base + LARA_ANIM_SURF_SWIM);
                }

                olLaraSwimTimer = 0;
                lara->vSpeed = 0;
                lara->flags2 &= (uint8)~ITEM_FLAG_GRAVITY;
                olLaraAngleX = 0;
#ifdef OL_FX3
                olLaraAngleZ = 0;
#endif
                Lara_updateRoom(lara, 0);
            }
            break;

        default:
            break;
    }
}

/* ---------------------------------------------------------------------------
 * Lara::update, in upstream's order: the water state and her air; then one
 * of updateAbove, updateSurface and updateUnder -- the tick's collision
 * defaults, the state handler, and the turn or the drift the water state
 * gives her; animProcess and move; updateCollision (the c_ handler of the
 * state she is now in); updateRoom; and the trigger under the point
 * collision left her on.
 *
 * The order matters. The state handler asks for a goal; animProcess grants
 * it when the graph offers a transition; move reads the animation that
 * resulted; collision then puts her where the world allows -- and sets
 * moveAngle for the *next* tick's move, which is upstream's timing.
 * ------------------------------------------------------------------------ */
void Lara_update(ItemObj* lara, unsigned int input)
{
    int oldX = lara->x, oldY = lara->y, oldZ = lara->z;
    int offset;
    int32 s, c, v, hx, hz;

    Lara_updateWaterState(lara);

    /* Her air: a minute of it, spent under water a tick at a time and won
       back ten at a time out of it; with none left she drowns at five a
       tick, which is upstream's hit(5). */
    if (lara->health > 0) {
        if (olLaraWaterState == WATER_STATE_UNDER) {
            if (olLaraOxygen > 0) {
                olLaraOxygen--;
            } else {
                lara->health = (int16)(lara->health - 5);
                if (lara->health < 0) lara->health = 0;
            }
        } else {
            olLaraOxygen += 10;
            if (olLaraOxygen > LARA_MAX_OXYGEN) olLaraOxygen = LARA_MAX_OXYGEN;
        }
    }

    /* The collision defaults the three share. */
    cinfo.trigger     = 0xFFFF;
    cinfo.posX        = lara->x;
    cinfo.posY        = lara->y;
    cinfo.posZ        = lara->z;
    cinfo.stopOnSlant = 0;
    cinfo.stopOnLava  = 0;

    switch (olLaraWaterState)
    {
        case WATER_STATE_SURFACE:
            /* updateSurface */
            cinfo.radius     = LARA_RADIUS;
            cinfo.gapPos     = -WALL;
            cinfo.gapNeg     = -128;
            cinfo.gapCeiling = 100;
            cinfo.enemyPush  = 0;
            cinfo.enemyHit   = 0;

            Lara_updateState(lara, input);

            /* She drifts the way the last tick's handler pointed her, at
               her swimming speed: the surface animations carry none of
               their own, so move() adds nothing to this. */
            s = phd_sin((int32)olLaraMoveAngle);
            c = phd_cos((int32)olLaraMoveAngle);
            v = (int32)lara->vSpeed;
            lara->x = (int16)(lara->x + (int)((s * v) >> 16));
            lara->z = (int16)(lara->z + (int)((c * v) >> 16));
            camera.askAngleX = -22 * ANGLE_1;
            CAM_ASKED();
#ifdef OL_FX3
            olLaraAngleZ = angleDec(olLaraAngleZ, OL_SPEED(LARA_ANGLE(2)));
#endif
            break;

        case WATER_STATE_UNDER:
            /* updateUnder */
            cinfo.radius     = LARA_RADIUS_WATER;
            cinfo.gapPos     = -WALL;
            cinfo.gapNeg     = -LARA_HEIGHT_UW;
            cinfo.gapCeiling = LARA_HEIGHT_UW;
            cinfo.enemyPush  = 0;
            cinfo.enemyHit   = 0;

            Lara_updateState(lara, input);

            olLaraTurn = angleDec(olLaraTurn, LARA_TURN_DECAY);
            lara->angleY = (uint16)(lara->angleY + olLaraTurn);

            if (olLaraAngleX < LARA_ANGLE(-85)) olLaraAngleX = LARA_ANGLE(-85);
            if (olLaraAngleX > LARA_ANGLE(85))  olLaraAngleX = LARA_ANGLE(85);
#ifdef OL_FX3
            olLaraAngleZ = angleDec(olLaraAngleZ, OL_SPEED(LARA_ANGLE(2)));
            if (olLaraAngleZ < LARA_ANGLE(-22)) olLaraAngleZ = LARA_ANGLE(-22);
            if (olLaraAngleZ > LARA_ANGLE(22))  olLaraAngleZ = LARA_ANGLE(22);
#endif

            /* Along her pitch and heading at vSpeed: TR's own arithmetic, a
               1.14 sine against the speed and sixteen bits off, so the 200
               she swims at is 50 units a tick. */
            v  = (int32)lara->vSpeed;
            s  = phd_sin((int32)(uint16)olLaraAngleX);
            c  = phd_cos((int32)(uint16)olLaraAngleX);
            hx = (phd_sin((int32)lara->angleY) * v) >> 16;
            hz = (phd_cos((int32)lara->angleY) * v) >> 16;
            lara->y = (int16)(lara->y - (int)((s * v) >> 16));
            lara->x = (int16)(lara->x + (int)((c * hx) >> FIXED_SHIFT));
            lara->z = (int16)(lara->z + (int)((c * hz) >> FIXED_SHIFT));
            break;

        default:
            /* updateAbove, which WADE shares */
            cinfo.radius     = LARA_RADIUS;
            cinfo.enemyPush  = 1;
            cinfo.enemyHit   = 1;

            Lara_updateState(lara, input);

#ifdef OL_FX3
            olLaraAngleZ = angleDec(olLaraAngleZ, OL_SPEED(LARA_ANGLE(1)));
#endif
            olLaraTurn = angleDec(olLaraTurn, LARA_TURN_DECAY);
            lara->angleY = (uint16)(lara->angleY + olLaraTurn);
            break;
    }

    ItemObj_animProcess(lara);
    ItemObj_move(lara);

    /* updateCollision: the things she can bump into and the things she can
       use, then the state's own handler. On FX3 a shut door is walls in
       the sectors (door.c), as upstream's, and has no collide of its own. */
#ifndef OL_FX3
    Lara_collideItems(lara);
#else
    Enemy_collideLara(lara);
    if (olDartsLive) Dart_collideLara(lara);
    if (olBladeCount) SwingBlade_collideLara(lara);
#endif
    Lara_collideObjects(lara, input);
    Lara_updateCollision(lara, input);
#ifdef OL_FX3
    /* A zero timer is a no-op; the call is not free under 816-tcc. */
    if (olLaraHitTimer) Lara_hitTick();
#endif

    /* Her radius down at the surface, her feet otherwise -- see
       Lara_updateRoom. */
    offset = (olLaraWaterState == WATER_STATE_SURFACE) ? LARA_RADIUS : 0;
    Lara_updateRoom(lara, offset);

    /* Upstream's badPos: a sector with no floor is nowhere to be, so she
       stays where she was. */
    if (lara->roomFloor == OL_NO_FLOOR) {
        lara->x = (int16)oldX;
        lara->y = (int16)oldY;
        lara->z = (int16)oldZ;
        Lara_updateRoom(lara, offset);
    }

#ifdef OL_FX3
    /* gameUpdate checks this probe after Weapon_update, as upstream does.
       Landing still checks immediately in collision.c with the old weapon
       state. COMBAT must observe draw/holster transitions on this tick. */
#else
    checkTrigger(cinfo.trigger);
#endif
}
