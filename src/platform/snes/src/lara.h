/* OpenLara - SNES / Super FX target
 * The port of src/fixed/lara.h, as far as it goes.
 *
 * Upstream is 108 KB: 71 states with a handler and collision handler each,
 * handler each, weapons, water, climbing, the inventory and the camera. What
 * is here is the first dry-land movement slice -- standing, walking, running,
 * turning, backing, sidestepping, rolling, directional jumping, falling,
 * landing and death -- and the thing underneath all of them, which is that
 * TR's Lara moves at the speed her current animation says.
 *
 * That last point is the whole reason this exists. What stood here before was
 * `items[0].x += sin(angle) * TURN_SPEED`: a constant nobody chose, applied
 * whatever she was doing. Upstream never writes a speed down. Every animation
 * carries one, `ItemObj::move` reads it, and a walk differs from a run
 * because the animations differ. Getting that right is what makes the state
 * machine mean anything -- without it every state moves identically and the
 * graph is decoration.
 *
 * Not here, and each is its own slice: the remaining weapons; wall climbing of TR2
 * and after; wading, which is TR2's as well. Hanging and the ledge climbs
 * are in, with `updateCollision` and the c_ handlers, and so is the water
 * -- the fall in, the surface, the swim and the climb out -- on the room
 * flag and the water column the blob and room.c carry.
 */

#ifndef H_OL_LARA
#define H_OL_LARA

#include "item.h"

/* Upstream's Lara::State, in upstream's order, which is TR's numbering: the
   level data indexes it, so these are facts about the file rather than
   choices. Only the ones this port can reach are named. */
#define STATE_WALK          0
#define STATE_RUN           1
#define STATE_STOP          2
#define STATE_JUMP          3
#define STATE_POSE          4
#define STATE_BACK_FAST     5
#define STATE_TURN_RIGHT    6
#define STATE_TURN_LEFT     7
#define STATE_DEATH         8
#define STATE_FALL          9
#define STATE_HANG          10
#define STATE_REACH         11
#define STATE_SPLAT         12
#define STATE_UW_TREAD      13
#define STATE_LAND          14
#define STATE_COMPRESS      15
#define STATE_BACK          16
#define STATE_UW_SWIM       17
#define STATE_UW_GLIDE      18
#define STATE_HANG_UP       19
#define STATE_TURN_FAST     20
#define STATE_STEP_RIGHT    21
#define STATE_STEP_LEFT     22
#define STATE_ROLL_END      23
#define STATE_SLIDE         24
#define STATE_JUMP_BACK     25
#define STATE_JUMP_RIGHT    26
#define STATE_JUMP_LEFT     27
#define STATE_JUMP_UP       28
#define STATE_FALL_BACK     29
#define STATE_HANG_LEFT     30
#define STATE_HANG_RIGHT    31
#define STATE_SLIDE_BACK    32
#define STATE_SURF_TREAD    33
#define STATE_SURF_SWIM     34
#define STATE_UW_DIVE       35
#define STATE_BLOCK_PUSH    36
#define STATE_BLOCK_PULL    37
#define STATE_BLOCK_READY   38
#define STATE_PICKUP        39
#define STATE_SWITCH_DOWN   40
#define STATE_SWITCH_UP     41
#define STATE_USE_KEY       42
#define STATE_USE_PUZZLE    43
#define STATE_DEATH_UW      44
#define STATE_ROLL_START    45
#define STATE_SURF_BACK     47
#define STATE_SURF_LEFT     48
#define STATE_SURF_RIGHT    49
#define STATE_SWAN_DIVE     52
#define STATE_FAST_DIVE     53
#define STATE_HANDSTAND     54
#define STATE_WATER_OUT     55
#define STATE_WADE          65      /* TR2's; upstream reaches it, TR1 has no animation for it */

/* Upstream's WaterState, in upstream's order. */
#define WATER_STATE_ABOVE   0
#define WATER_STATE_WADE    1
#define WATER_STATE_SURFACE 2
#define WATER_STATE_UNDER   3

/* Upstream's constants, converted by OL_SPEED and OL_ACCEL -- see fixed.h for
   why those are different multipliers. turnSpeed is an angular speed, so its
   caps take OL_SPEED. */
#define LARA_TURN_VERY_SLOW OL_SPEED(ANGLE_1 * 2)
#define LARA_TURN_SLOW      OL_SPEED(ANGLE_1 * 4)
#define LARA_TURN_MED       OL_SPEED(ANGLE_1 * 6)
#define LARA_TURN_FAST      OL_SPEED(ANGLE_1 * 8)
#define LARA_TURN_JUMP      OL_SPEED(ANGLE_1 * 3)

/* upstream's s_checkFall threshold: a downward speed of 131 a tick is when
   she stops jumping and starts falling. A speed, so it converts as one. */
#define LARA_FALL_SPEED     OL_SPEED(131)

#define LARA_TURN_ACCEL     OL_ACCEL((ANGLE_1 * 9) / 4)
#define LARA_TURN_DECAY     OL_ACCEL(ANGLE_1 * 2)

#define LARA_RADIUS         100
#define LARA_RADIUS_WATER   300
#define LARA_HEIGHT         762
#define LARA_HEIGHT_JUMP    870
#define LARA_HEIGHT_UW      400
#define LARA_HEIGHT_SURF    700
#define LARA_STEP_HEIGHT    384
#define LARA_SMASH_HEIGHT   640
#define LARA_HANG_SLANT     60
#define LARA_MAX_HEALTH     1000
#define LARA_MAX_OXYGEN     1800    /* a minute of ticks */
#define LARA_GRAVITY        6       /* upstream's GRAVITY, per tick */

/* The water. vSpeed is her swimming speed under water and at the surface
   -- TR reuses the field -- so the caps are speeds and the friction and
   acceleration are accelerations. The pitch step is an angular speed. */
#define LARA_FLOAT_UP_SPEED OL_SPEED(5)
#define LARA_SWIM_FRICTION  OL_ACCEL(6)
#define LARA_SWIM_ACCEL     OL_ACCEL(8)
#define LARA_SWIM_SPEED_MIN OL_SPEED(133)
#define LARA_SWIM_SPEED_MAX OL_SPEED(200)
#define LARA_SWIM_TIMER     10      /* a third of a second of Jump, to dive */
#define LARA_SURF_FRICTION  OL_ACCEL(4)
#define LARA_SURF_ACCEL     OL_ACCEL(8)
#define LARA_SURF_SPEED_MAX OL_SPEED(60)
#define LARA_DIVE_SPEED     OL_SPEED(80)
#define LARA_UW_PITCH       OL_SPEED(ANGLE_1 * 2)
#define LARA_WADE_MIN_DEPTH 384
#define LARA_WADE_MAX_DEPTH 730
#define LARA_SWIM_MIN_DEPTH 512

/* Upstream's ANIM_ indices into her own model's run. */
#define LARA_ANIM_RUN                 0
#define LARA_ANIM_WALK                1
#define LARA_ANIM_STAND_LEFT          2
#define LARA_ANIM_STAND_RIGHT         3
#define LARA_ANIM_STAND               11
#define LARA_ANIM_LANDING             24
#define LARA_ANIM_FALL_HANG           28
#define LARA_ANIM_SMASH_JUMP          32
#define LARA_ANIM_FALL_FORTH          34
#define LARA_ANIM_BACK                41
#define LARA_ANIM_CLIMB_3             42
#define LARA_ANIM_CLIMB_2             50
#define LARA_ANIM_SMASH_RUN_LEFT      53
#define LARA_ANIM_SMASH_RUN_RIGHT     54
#define LARA_ANIM_RUN_ASCEND_LEFT     55
#define LARA_ANIM_RUN_ASCEND_RIGHT    56
#define LARA_ANIM_WALK_ASCEND_LEFT    57
#define LARA_ANIM_WALK_ASCEND_RIGHT   58
#define LARA_ANIM_WALK_DESCEND_RIGHT  59
#define LARA_ANIM_WALK_DESCEND_LEFT   60
#define LARA_ANIM_BACK_DESCEND_LEFT   61
#define LARA_ANIM_BACK_DESCEND_RIGHT  62
#define LARA_ANIM_SLIDE_FORTH         70
#define LARA_ANIM_UW_GLIDE            87
#define LARA_ANIM_FALL_BACK           93
#define LARA_ANIM_HANG                96
#define LARA_ANIM_STAND_NORMAL        103
#define LARA_ANIM_BLOCK_READY         120
#define LARA_ANIM_SLIDE_BACK          104
#define LARA_ANIM_UNDERWATER          108
#define LARA_ANIM_WATER_OUT           111
#define LARA_ANIM_WATER_FALL          112
#define LARA_ANIM_SURF                114
#define LARA_ANIM_SURF_SWIM           116
#define LARA_ANIM_SURF_DIVE           119
#define LARA_ANIM_SURF_BACK           140
#define LARA_ANIM_SURF_LEFT           143
#define LARA_ANIM_SURF_RIGHT          144
/* TR1's Lara has 160 animations. Upstream's ANIM_WADE, ANIM_SURF_OUT,
   ANIM_SWIM_STAND, ANIM_SURF_STAND and ANIM_UW_ROLL are past that -- TR2's
   -- so the arms that would play them are not here. */
#define LARA_ANIM_ROLL                146
#define LARA_ANIM_HANG_SWING          150

/* upstream's Lara::turnSpeed, which lives in extraL rather than in ItemObj --
   so it lives here rather than costing every entity two bytes. Signed. */
extern int olLaraTurn;
extern uint16 olLaraMoveAngle;
/* upstream's extraL->vSpeedHack: the launch speed c_checkClimbUp works out
   for a jump up at a ledge too high to climb, spent by the next JUMP
   command in place of the animation's own. */
extern int olLaraVSpeedHack;

/* upstream's Lara::waterState, and the three fields of extraL the water
   reads: angle.x, her pitch under water, which ItemObj has no room for and
   the renderer does not draw (DrawRoomItems turns an entity about Y only,
   so she swims level in the picture while moving along her pitch);
   swimTimer, how long Jump has been held at the surface; and oxygen, which
   upstream aliases to ItemObj::timer -- the active list reads that here.
   TR's convention for the pitch: Up puts her nose down. */
extern unsigned int olLaraWaterState;
extern int olLaraAngleX;
#ifdef OL_FX3
extern int olLaraAngleZ;
#endif
extern int olLaraSwimTimer;
extern int olLaraOxygen;

/* upstream's extraL->weaponState: FREE is
   hands empty and idle, BUSY is an interaction playing -- a switch, a
   pickup, a key -- which the animation's EMPTY command ends. DRAW,
   HOLSTER and READY belong to the pistol controller. */
#define WEAPON_STATE_FREE   0
#define WEAPON_STATE_BUSY   1
#define WEAPON_STATE_DRAW   2
#define WEAPON_STATE_HOLSTER 3
#define WEAPON_STATE_READY  4
extern unsigned int olLaraWeaponState;

/* Additional local joint rotations, applied after the animation's pose.
   Pistol tracking sets head and torso alike; the renderer keeps them
   separate for the look camera and other weapons when those are ported. */
typedef struct {
    int16 torsoX, torsoY;
    int16 headX, headY;
} LaraLook;
extern LaraLook olLaraLook;
void Lara_resetLook(void);
void Lara_updateLook(void);
unsigned int Lara_manualLook(unsigned int input);
extern unsigned int olLaraLooking;  /* s_checkLook, this tick */

/* void Lara::s_dive(): from the surface into the water, nose down at 45
   degrees. c_surf starts it too, when the water has closed over her. */
void Lara_dive(ItemObj* lara);


/* void Lara::updateState() and the tail of updateAbove(), which is where the
   turn she asked for is spent. */
void Lara_updateState(ItemObj* lara, unsigned int input);

/* Lara::update's ground path: state, turn, animation, movement, and then
   collision -- the c_ handlers in collision.c -- room relink and trigger. */
void Lara_update(ItemObj* lara, unsigned int input);

/* The room query that relinked her at the end of Lara_update, kept for
   whoever wants the floor under her feet this tick -- the camera, the
   probe, the runner -- so nobody asks the same question twice. */
#include "room.h"
extern RoomQuery olLaraQuery;
extern int       olLaraQueryOk;

#endif
