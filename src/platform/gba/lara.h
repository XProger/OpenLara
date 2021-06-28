#ifndef H_LARA
#define H_LARA

#include "common.h"
#include "item.h"
#include "collision.h"
#include "camera.h"

// -------------
// TODO list - GBA demo (GYM, LEVEL1, LEVEL2)
// -------------
// fix portals flickering
// darts damage
// swing blade damage
// multi-pickups
// sprite effects (splash, smoke, ricochet)
// animation lerp for Lara (enemies?)
// inventory
// camera look
// lookat
// weapons
// enemies (Bat, Wolf, Bear)
// main menu
// save game
// ADPCM sounds and tracks
// gameflow
// -------------

#define LARA_STATES(E) \
    E( STATE_WALK         ) \
    E( STATE_RUN          ) \
    E( STATE_STOP         ) \
    E( STATE_JUMP         ) \
    E( STATE_POSE         ) \
    E( STATE_BACK_FAST    ) \
    E( STATE_TURN_RIGHT   ) \
    E( STATE_TURN_LEFT    ) \
    E( STATE_DEATH        ) \
    E( STATE_FALL         ) \
    E( STATE_HANG         ) \
    E( STATE_REACH        ) \
    E( STATE_SPLAT        ) \
    E( STATE_UW_TREAD     ) \
    E( STATE_LAND         ) \
    E( STATE_COMPRESS     ) \
    E( STATE_BACK         ) \
    E( STATE_UW_SWIM      ) \
    E( STATE_UW_GLIDE     ) \
    E( STATE_HANG_UP      ) \
    E( STATE_TURN_FAST    ) \
    E( STATE_STEP_RIGHT   ) \
    E( STATE_STEP_LEFT    ) \
    E( STATE_ROLL_END     ) \
    E( STATE_SLIDE        ) \
    E( STATE_JUMP_BACK    ) \
    E( STATE_JUMP_RIGHT   ) \
    E( STATE_JUMP_LEFT    ) \
    E( STATE_JUMP_UP      ) \
    E( STATE_FALL_BACK    ) \
    E( STATE_HANG_LEFT    ) \
    E( STATE_HANG_RIGHT   ) \
    E( STATE_SLIDE_BACK   ) \
    E( STATE_SURF_TREAD   ) \
    E( STATE_SURF_SWIM    ) \
    E( STATE_UW_DIVE      ) \
    E( STATE_BLOCK_PUSH   ) \
    E( STATE_BLOCK_PULL   ) \
    E( STATE_BLOCK_READY  ) \
    E( STATE_PICKUP       ) \
    E( STATE_SWITCH_DOWN  ) \
    E( STATE_SWITCH_UP    ) \
    E( STATE_USE_KEY      ) \
    E( STATE_USE_PUZZLE   ) \
    E( STATE_DEATH_UW     ) \
    E( STATE_ROLL_START   ) \
    E( STATE_SPECIAL      ) \
    E( STATE_SURF_BACK    ) \
    E( STATE_SURF_LEFT    ) \
    E( STATE_SURF_RIGHT   ) \
    E( STATE_USE_MIDAS    ) \
    E( STATE_DEATH_MIDAS  ) \
    E( STATE_SWAN_DIVE    ) \
    E( STATE_FAST_DIVE    ) \
    E( STATE_HANDSTAND    ) \
    E( STATE_WATER_OUT    ) \
    E( STATE_CLIMB_START  ) \
    E( STATE_CLIMB_UP     ) \
    E( STATE_CLIMB_LEFT   ) \
    E( STATE_CLIMB_END    ) \
    E( STATE_CLIMB_RIGHT  ) \
    E( STATE_CLIMB_DOWN   ) \
    E( STATE_UNUSED_1     ) \
    E( STATE_UNUSED_2     ) \
    E( STATE_UNUSED_3     ) \
    E( STATE_WADE         ) \
    E( STATE_ROLL_UW      ) \
    E( STATE_PICKUP_FLARE ) \
    E( STATE_ROLL_AIR     ) \
    E( STATE_UNUSED_4     ) \
    E( STATE_ZIPLINE      )

#define DECL_ENUM(v) v,
#define DECL_S_HANDLER(v) &Lara::s_##v,
#define DECL_C_HANDLER(v) &Lara::c_##v,
#define S_HANDLER(state) void s_##state()
#define C_HANDLER(state) void c_##state()

#define LARA_HANG_SLANT         60
#define LARA_HANG_OFFSET        724
#define LARA_HEIGHT             762
#define LARA_HEIGHT_JUMP        870 // LARA_HEIGHT + hands up
#define LARA_HEIGHT_UW          400
#define LARA_HEIGHT_SURF        700
#define LARA_RADIUS             100
#define LARA_RADIUS_WATER       300
#define LARA_RADIUS_CLIMB       220
#define LARA_MAX_HEALTH         1000
#define LARA_MAX_OXYGEN         1800    // TODO +30 sec for TR5
#define LARA_HEIGHT             762
#define LARA_TURN_ACCEL         (2 * DEG2SHORT + DEG2SHORT / 4)
#define LARA_TURN_JUMP          (3 * DEG2SHORT)
#define LARA_TURN_VERY_SLOW     (2 * DEG2SHORT)
#define LARA_TURN_SLOW          (4 * DEG2SHORT)
#define LARA_TURN_MED           (6 * DEG2SHORT)
#define LARA_TURN_FAST          (8 * DEG2SHORT)
#define LARA_TILT_ACCEL         (DEG2SHORT + DEG2SHORT / 2)
#define LARA_TILT_MAX           (11 * DEG2SHORT)
#define LARA_STEP_HEIGHT        384
#define LARA_SMASH_HEIGHT       640
#define LARA_FLOAT_UP_SPEED     5
#define LARA_SWIM_FRICTION      6
#define LARA_SWIM_ACCEL         8
#define LARA_SWIM_SPEED_MIN     133
#define LARA_SWIM_SPEED_MAX     200
#define LARA_SWIM_TIMER         10  // 1/3 sec
#define LARA_SURF_FRICTION      4
#define LARA_SURF_ACCEL         8
#define LARA_SURF_SPEED_MAX     60
#define LARA_DIVE_SPEED         80
#define LARA_WADE_MIN_DEPTH     384
#define LARA_WADE_MAX_DEPTH     730
#define LARA_SWIM_MIN_DEPTH     512

enum {
    JOINT_HIPS = 0,
    JOINT_LEG_L1,
    JOINT_LEG_L2,
    JOINT_LEG_L3,
    JOINT_LEG_R1,
    JOINT_LEG_R2,
    JOINT_LEG_R3,
    JOINT_CHEST,
    JOINT_ARM_R1,
    JOINT_ARM_R2,
    JOINT_ARM_R3,
    JOINT_ARM_L1,
    JOINT_ARM_L2,
    JOINT_ARM_L3,
    JOINT_HEAD,
    JOINT_MAX
};

enum {
    JOINT_MASK_HIPS       = 1 << JOINT_HIPS,
    JOINT_MASK_LEG_L1     = 1 << JOINT_LEG_L1,
    JOINT_MASK_LEG_L2     = 1 << JOINT_LEG_L2,
    JOINT_MASK_LEG_L3     = 1 << JOINT_LEG_L3,
    JOINT_MASK_LEG_R1     = 1 << JOINT_LEG_R1,
    JOINT_MASK_LEG_R2     = 1 << JOINT_LEG_R2,
    JOINT_MASK_LEG_R3     = 1 << JOINT_LEG_R3,
    JOINT_MASK_CHEST      = 1 << JOINT_CHEST,
    JOINT_MASK_ARM_R1     = 1 << JOINT_ARM_R1,
    JOINT_MASK_ARM_R2     = 1 << JOINT_ARM_R2,
    JOINT_MASK_ARM_R3     = 1 << JOINT_ARM_R3,
    JOINT_MASK_ARM_L1     = 1 << JOINT_ARM_L1,
    JOINT_MASK_ARM_L2     = 1 << JOINT_ARM_L2,
    JOINT_MASK_ARM_L3     = 1 << JOINT_ARM_L3,
    JOINT_MASK_HEAD       = 1 << JOINT_HEAD,
    JOINT_MASK_ARM_L      = JOINT_MASK_ARM_L1 | JOINT_MASK_ARM_L2 | JOINT_MASK_ARM_L3,
    JOINT_MASK_ARM_R      = JOINT_MASK_ARM_R1 | JOINT_MASK_ARM_R2 | JOINT_MASK_ARM_R3,
    JOINT_MASK_LEG_L      = JOINT_MASK_LEG_L1 | JOINT_MASK_LEG_L2 | JOINT_MASK_LEG_L3,
    JOINT_MASK_LEG_R      = JOINT_MASK_LEG_R1 | JOINT_MASK_LEG_R2 | JOINT_MASK_LEG_R3,
    JOINT_MASK_UPPER      = JOINT_MASK_CHEST  | JOINT_MASK_ARM_L  | JOINT_MASK_ARM_R,       // without head
    JOINT_MASK_LOWER      = JOINT_MASK_HIPS   | JOINT_MASK_LEG_L  | JOINT_MASK_LEG_R,
    JOINT_MASK_BRAID      = JOINT_MASK_HEAD   | JOINT_MASK_CHEST  | JOINT_MASK_ARM_L1 | JOINT_MASK_ARM_L2 | JOINT_MASK_ARM_R1 | JOINT_MASK_ARM_R2,
};

int32 swimTimer;

struct Lara : Item
{
    enum State {
        LARA_STATES(DECL_ENUM)
        X_MAX
    };

    enum {
        ANIM_RUN                = 0,

        ANIM_STAND_LEFT         = 2,
        ANIM_STAND_RIGHT        = 3,

        ANIM_RUN_START          = 6,

        ANIM_STAND              = 11,

        ANIM_LANDING            = 24,

        ANIM_CLIMB_JUMP         = 26,

        ANIM_FALL_HANG          = 28,

        ANIM_SMASH_JUMP         = 32,

        ANIM_FALL_FORTH         = 34,

        ANIM_BACK               = 41,
        ANIM_CLIMB_3            = 42,

        ANIM_CLIMB_2            = 50,

        ANIM_SMASH_RUN_LEFT     = 53,
        ANIM_SMASH_RUN_RIGHT    = 54,
        ANIM_RUN_ASCEND_LEFT    = 55,
        ANIM_RUN_ASCEND_RIGHT   = 56,
        ANIM_WALK_ASCEND_LEFT   = 57,
        ANIM_WALK_ASCEND_RIGHT  = 58,
        ANIM_WALK_DESCEND_RIGHT = 59,
        ANIM_WALK_DESCEND_LEFT  = 60,
        ANIM_BACK_DESCEND_LEFT  = 61,
        ANIM_BACK_DESCEND_RIGHT = 62,

        ANIM_SLIDE_FORTH        = 70,

        ANIM_UW_GLIDE           = 87,

        ANIM_FALL_BACK          = 93,

        ANIM_HANG               = 96,

        ANIM_STAND_NORMAL       = 103,

        ANIM_SLIDE_BACK         = 104,

        ANIM_UNDERWATER         = 108,

        ANIM_WATER_OUT          = 111,
        ANIM_WATER_FALL         = 112,
        ANIM_SURF               = 114,
        ANIM_SURF_SWIM          = 116,
        ANIM_SURF_DIVE          = 119,

        ANIM_HIT_FRONT          = 125,
        ANIM_HIT_BACK           = 126,
        ANIM_HIT_LEFT           = 127,
        ANIM_HIT_RIGHT          = 128,

        ANIM_DEATH_BOULDER      = 139,
        ANIM_SURF_BACK          = 140,

        ANIM_SURF_LEFT          = 143,
        ANIM_SURF_RIGHT         = 144,

        ANIM_STAND_ROLL_BEGIN   = 146,
        ANIM_STAND_ROLL_END     = 147,

        ANIM_DEATH_SPIKES       = 149,
        ANIM_HANG_SWING         = 150,

        ANIM_CLIMB_START        = 164,

        ANIM_WADE_SWIM          = 176,
        ANIM_WADE               = 177,
        ANIM_WADE_RUN_LEFT      = 178,
        ANIM_WADE_RUN_RIGHT     = 179,
        ANIM_WADE_STAND         = 186,
        ANIM_WADE_ASCEND        = 190,
        ANIM_SURF_OUT           = 191,
        ANIM_SWIM_STAND         = 192,
        ANIM_SURF_STAND         = 193,

        ANIM_SWITCH_BIG_DOWN    = 195,
        ANIM_SWITCH_BIG_UP      = 196,
        ANIM_PUSH_BUTTON        = 197,

        ANIM_UW_ROLL            = 203,
    };

    typedef void (Lara::*Handler)();

    static const Handler sHandlers[X_MAX];
    static const Handler cHandlers[X_MAX];

    void updateState()
    {
        (this->*sHandlers[state])();
    }

    void updateCollision()
    {
        Room** adjRoom = room->getAdjRooms();
        while (*adjRoom)
        {
            Item* item = (*adjRoom++)->firstItem;
            
            while (item)
            {
                if (item->flags.status != ITEM_FLAGS_STATUS_INVISIBLE)
                {
                    if (item->flags.collision)
                    {
                        vec3i d = pos - item->pos;

                        if (abs(d.x) < 4096 && abs(d.y) < 4096 && abs(d.z) < 4096)
                        {
                            item->collide(this, &cinfo);
                        }
                    }
                }
                item = item->nextItem;
            }
        }

        (this->*cHandlers[state])();
    }

    void startScreaming()
    {
        soundPlay(SND_SCREAM, pos);
    }

    void stopScreaming()
    {
        soundStop(SND_SCREAM);
    }

// common
    bool alignAngle(int16 &angle, int16 threshold)
    {
        if (angle >= -threshold && angle <= threshold) {
            angle = 0;
        } else if (angle >= ANGLE_90 - threshold && angle <= ANGLE_90 + threshold) {
            angle = ANGLE_90;
        } else if (angle >= -ANGLE_90 - threshold && angle <= -ANGLE_90 + threshold) {
            angle = -ANGLE_90;
        } else if (angle >= -(ANGLE_180 + 1 + threshold) || angle <= (ANGLE_180 + 1 + threshold)) {
            angle = ANGLE_180;
        }

        return (angle & (ANGLE_90 - 1)) > 0;
    }

    void alignWall(int32 radius)
    {
        int x = pos.x & ~1023;
        int z = pos.z & ~1023;

        switch (angle.y)
        {
            case  ANGLE_0   : pos.z = z + 1024 - radius; break;
            case  ANGLE_90  : pos.x = x + 1024 - radius; break;
            case -ANGLE_90  : pos.x = x + radius; break;
            case  ANGLE_180 : pos.z = z + radius; break;
            default         : ASSERT(false);
        }
    }

    int16 getFront(int16 rot)
    {
        rot += angle.y;

        int32 x = pos.x + (phd_sin(rot) >> (FIXED_SHIFT - 8));
        int32 y = pos.y - LARA_HEIGHT;
        int32 z = pos.z + (phd_cos(rot) >> (FIXED_SHIFT - 8));

        Room* roomFront = room->getRoom(x, y, z);
        const Sector* sector = roomFront->getSector(x, z);
        int32 floor = sector->getFloor(x, y, z);

        if (floor != WALL) {
            floor -= pos.y;
        }

        return floor;
    }

    const Bounds& getBounds()
    {
        const Model* model = models + type;

        AnimFrame* frame = getFrame(model);

        return frame->box;
    }

    bool checkDeath(State deathState)
    {
        if (health <= 0) {
            goalState = deathState;
            return true;
        }
        return false;
    }

// state control
    bool s_checkFront(int16 angleDelta) 
    {
        CollisionInfo tmpInfo = cinfo;
        int16 tmpAngle = moveAngle;

        c_angle(angleDelta);
        cinfo.radius = LARA_RADIUS + 4;

        cinfo.gapPos      = -WALL;
        cinfo.gapNeg      = -LARA_STEP_HEIGHT;
        cinfo.stopOnSlant = true;
        cinfo.gapCeiling  = 0;

        if ((angleDelta == ANGLE_180) && ((input & IN_WALK) || (waterState == WATER_STATE_WADE))) {
            cinfo.gapPos = LARA_STEP_HEIGHT;
        }

        collideRoom(this, LARA_HEIGHT, 0);

        bool collide = (cinfo.type == CT_FRONT) || (cinfo.type == CT_FRONT_CEILING);

        cinfo = tmpInfo;
        moveAngle = tmpAngle;

        return !collide;
    }

    void s_ignoreEnemy()
    {
        cinfo.enemyPush = false;
        cinfo.enemyHit  = false;
    }

    void s_rotate(int32 maxSpeed, int32 tilt)
    {
        tilt *= LARA_TILT_ACCEL;

        if (input & IN_LEFT) {
            turnSpeed = X_MAX(turnSpeed - LARA_TURN_ACCEL, -maxSpeed);
            angle.z = X_MAX(angle.z - tilt, -LARA_TILT_MAX);
        } else if (input & IN_RIGHT) {
            turnSpeed = X_MIN(turnSpeed + LARA_TURN_ACCEL, maxSpeed);
            angle.z = X_MIN(angle.z + tilt, LARA_TILT_MAX);
        }
    }

    bool s_checkFall()
    {
        if (vSpeed > 131)
        {
            if (state == STATE_SWAN_DIVE) {
                goalState = STATE_FAST_DIVE;
            } else {
                goalState = STATE_FALL;
            }
            return true;
        }
        return false;
    }

    void s_checkWalk(int32 stopState)
    {
        if ((input & IN_UP) && s_checkFront(ANGLE_0)) {
            if (input & IN_WALK) {
                goalState = STATE_WALK;
            } else {
                goalState = STATE_RUN;
            }
        } else {
            goalState = stopState;
        }
    }

    bool s_checkRoll()
    {
        if ((waterState != WATER_STATE_ABOVE) && (waterState != WATER_STATE_UNDER)) {
            return false;
        }

        bool roll = (input & (IN_UP | IN_DOWN)) == (IN_UP | IN_DOWN);

        if ((waterState == WATER_STATE_ABOVE) && roll)
        {
            if ((state == STATE_RUN) || (state == STATE_STOP))
            {
                animSet(ANIM_STAND_ROLL_BEGIN, true, 2);
                goalState = STATE_STOP;
                return true;
            }
        }

        return false;
    }

    void s_turnUW()
    {
        if (input & IN_UP) {
            angle.x -= 2 * DEG2SHORT;
        } else if (input & IN_DOWN) {
            angle.x += 2 * DEG2SHORT;
        }

        if (input & IN_LEFT) {
            turnSpeed = max(turnSpeed - LARA_TURN_ACCEL, -LARA_TURN_MED);
            angle.z -= LARA_TILT_ACCEL * 2;
        } else if (input & IN_RIGHT) {
            turnSpeed = min(turnSpeed + LARA_TURN_ACCEL,  LARA_TURN_MED);
            angle.z += LARA_TILT_ACCEL * 2;
        }
    }

    void s_dive()
    {
        animSet(ANIM_SURF_DIVE, true);
        angle.x = -45 * DEG2SHORT;
        vSpeed = LARA_DIVE_SPEED;
        waterState = WATER_STATE_UNDER;
    }

    S_HANDLER( STATE_WALK )
    {
        if (checkDeath(STATE_STOP))
            return;

        s_rotate(LARA_TURN_SLOW, 0);

        s_checkWalk(STATE_STOP);
    }

    S_HANDLER( STATE_RUN )
    {
        if (checkDeath(STATE_DEATH))
            return;

        if (s_checkRoll())
            return;

        s_rotate(LARA_TURN_FAST, 1);

        if ((input & IN_JUMP) && !flags.gravity) {
            goalState = STATE_JUMP;
        } else {
            s_checkWalk(STATE_STOP);
        }
    }

    S_HANDLER( STATE_STOP )
    {
        if (checkDeath(STATE_DEATH))
            return;

        if (s_checkRoll())
            return;

        goalState = STATE_STOP;

        if ((input & (IN_UP | IN_ACTION)) == (IN_UP | IN_ACTION))
        {
            c_angle(ANGLE_0);
            cinfo.radius = LARA_RADIUS + 4;
            c_default();
            cinfo.radius = LARA_RADIUS;

            if (c_checkClimbUp())
                return;
        }

        if (input & IN_WALK) {
            if ((input & IN_LEFT) && s_checkFront(-ANGLE_90)) {
                goalState = STATE_STEP_LEFT;
            } else if ((input & IN_RIGHT) && s_checkFront(ANGLE_90)) {
                goalState = STATE_STEP_RIGHT;
            }
        } else {
            if (input & IN_LEFT) {
                goalState = STATE_TURN_LEFT;
            } else if (input & IN_RIGHT) {
                goalState = STATE_TURN_RIGHT;
            }
        }

        if (input & IN_JUMP) {
            goalState = STATE_COMPRESS;
        } else if ((input & IN_UP) && s_checkFront(ANGLE_0)) {
            if (input & IN_WALK) {
                s_STATE_WALK();
            } else {
                s_STATE_RUN();
            }
        } else if ((input & IN_DOWN) && s_checkFront(ANGLE_180)) {
            if (input & IN_WALK) {
                s_STATE_BACK();
            } else {
                goalState = STATE_BACK_FAST;
            }
        }
    }


    S_HANDLER( STATE_JUMP )
    {
        if (goalState == STATE_SWAN_DIVE ||
            goalState == STATE_REACH)
        {
            goalState = STATE_JUMP;
        }

        if (goalState != STATE_DEATH &&
            goalState != STATE_STOP &&
            goalState != STATE_RUN)
        {
            if (input & IN_ACTION)
            {
                goalState = STATE_REACH;
            }
        
            if (input & IN_WALK)
            {
                goalState = STATE_SWAN_DIVE;
            }
        
            s_checkRoll();
            s_checkFall();
        }

        s_rotate(LARA_TURN_JUMP, 0);
    }

    S_HANDLER( STATE_POSE )
    {
        // empty
    }

    S_HANDLER( STATE_BACK_FAST )
    {
        s_rotate(LARA_TURN_MED, 0);
        goalState = STATE_STOP;
    }

    S_HANDLER( STATE_TURN_RIGHT )
    {
        if (checkDeath(STATE_STOP))
            return;

        if (input & IN_LOOK)
        {
            goalState = STATE_STOP;
            return;
        }

        turnSpeed += LARA_TURN_ACCEL;

        if (turnSpeed > LARA_TURN_SLOW)
        {
            goalState = STATE_TURN_FAST;
        }

        s_checkWalk((input & IN_RIGHT) ? goalState : STATE_STOP);
    }

    S_HANDLER( STATE_TURN_LEFT )
    {
        if (checkDeath(STATE_STOP))
            return;
            
        if (input & IN_LOOK)
        {
            goalState = STATE_STOP;
            return;
        }

        turnSpeed -= LARA_TURN_ACCEL;

        if (turnSpeed < -LARA_TURN_SLOW)
        {
            goalState = STATE_TURN_FAST;
        }

        s_checkWalk((input & IN_LEFT) ? goalState : STATE_STOP);
    }

    S_HANDLER( STATE_DEATH )
    {
        s_ignoreEnemy();
    }

    S_HANDLER( STATE_FALL )
    {
        hSpeed = (hSpeed * 95) / 100;

        if (vSpeed >= 154) {
            startScreaming();
        }
    }

    S_HANDLER( STATE_HANG )
    {
        camera.targetAngleX = -60 * DEG2SHORT;

        s_ignoreEnemy();
        if (input & IN_LEFT) {
            goalState = STATE_HANG_LEFT;
        } else if (input & IN_RIGHT) {
            goalState = STATE_HANG_RIGHT;
        }
    }

    S_HANDLER( STATE_REACH )
    {
        camera.targetAngleY = 85 * DEG2SHORT;

        s_checkFall();
    }

    S_HANDLER( STATE_SPLAT )
    {
        // empty
    }

    S_HANDLER( STATE_UW_TREAD )
    {
        if (checkDeath(STATE_DEATH_UW))
            return;

        if (s_checkRoll())
            return;

        s_turnUW();

        if (input & IN_JUMP) {
            goalState = STATE_UW_SWIM;
        }

        vSpeed = max(vSpeed - LARA_SWIM_FRICTION, 0);
    }

    S_HANDLER( STATE_LAND )
    {
        // empty
    }

    S_HANDLER( STATE_COMPRESS )
    {
        if ((input & IN_UP) && getFront(ANGLE_0) >= -LARA_STEP_HEIGHT) {
            goalState = STATE_JUMP;
        } else if ((input & IN_LEFT) && getFront(-ANGLE_90) >= -LARA_STEP_HEIGHT) {
            goalState = STATE_JUMP_LEFT;
        } else if ((input & IN_RIGHT) && getFront(ANGLE_90) >= -LARA_STEP_HEIGHT) {
            goalState = STATE_JUMP_RIGHT;
        } else if ((input & IN_DOWN) && getFront(ANGLE_180) >= -LARA_STEP_HEIGHT) {
            goalState = STATE_JUMP_BACK;
        }
        s_checkFall();
    }

    S_HANDLER( STATE_BACK )
    {
        if (checkDeath(STATE_STOP))
            return;

        if ((input & (IN_WALK | IN_DOWN)) != (IN_WALK | IN_DOWN)) {
            goalState = STATE_STOP;
        } else {
            goalState = STATE_BACK;
        }

        s_rotate(LARA_TURN_SLOW, 0);
    }

    S_HANDLER( STATE_UW_SWIM )
    {
        if (checkDeath(STATE_DEATH_UW))
            return;

        if (s_checkRoll())
            return;

        s_turnUW();

        vSpeed = min(vSpeed + LARA_SWIM_ACCEL, LARA_SWIM_SPEED_MAX);

        if (!(input & IN_JUMP)) {
            goalState = STATE_UW_GLIDE;
        }
    }

    S_HANDLER( STATE_UW_GLIDE )
    {
        if (checkDeath(STATE_DEATH_UW))
            return;

        if (s_checkRoll())
            return;

        s_turnUW();

        if (input & IN_JUMP) {
            goalState = STATE_UW_SWIM;
        }

        vSpeed = max(vSpeed - LARA_SWIM_FRICTION, 0);

        if (vSpeed <= LARA_SWIM_SPEED_MIN) {
            goalState = STATE_UW_TREAD;
        }
    }

    S_HANDLER( STATE_HANG_UP )
    {
        s_ignoreEnemy();
    }

    S_HANDLER( STATE_TURN_FAST )
    {
        if (checkDeath(STATE_STOP))
            return;
            
        if (input & IN_LOOK)
        {
            goalState = STATE_STOP;
            return;
        }

        if (turnSpeed < 0) {
            turnSpeed = -LARA_TURN_FAST;
            if (!(input & IN_LEFT)) {
                goalState = STATE_STOP;
            }
        } else {
            turnSpeed = LARA_TURN_FAST;
            if (!(input & IN_RIGHT)) {
                goalState = STATE_STOP;
            }
        }
    }

    S_HANDLER( STATE_STEP_RIGHT )
    {
        if (checkDeath(STATE_STOP))
            return;
            
        if ((input & (IN_WALK | IN_RIGHT)) != (IN_WALK | IN_RIGHT))
        {
            goalState = STATE_STOP;
        }
    }

    S_HANDLER( STATE_STEP_LEFT )
    {
        if (checkDeath(STATE_STOP))
            return;
            
        if ((input & (IN_WALK | IN_LEFT)) != (IN_WALK | IN_LEFT))
        {
            goalState = STATE_STOP;
        }
    }

    S_HANDLER( STATE_ROLL_END )
    {
        // empty
    }

    S_HANDLER( STATE_SLIDE )
    {
        camera.targetAngleX = -45 * DEG2SHORT;

        if (input & IN_JUMP)
        {
            goalState = STATE_JUMP;
        }
    }

    S_HANDLER( STATE_JUMP_BACK )
    {
        camera.targetAngleY = 135 * DEG2SHORT;

        if (s_checkFall())
            return;

        if (goalState == STATE_RUN) {
            goalState = STATE_STOP;
        }
    }

    S_HANDLER( STATE_JUMP_RIGHT )
    {
        s_checkFall();
    }

    S_HANDLER( STATE_JUMP_LEFT )
    {
        s_checkFall();
    }

    S_HANDLER( STATE_JUMP_UP )
    {
        s_checkFall();
    }

    S_HANDLER( STATE_FALL_BACK )
    {
        s_checkFall();
    
        if (input & IN_ACTION) 
        {
            goalState = STATE_REACH;
        }
    }

    S_HANDLER( STATE_HANG_LEFT )
    {
        camera.targetAngleX = -60 * DEG2SHORT;

        s_ignoreEnemy();

        if (!(input & IN_LEFT))
        {
            goalState = STATE_HANG;
        }
    }

    S_HANDLER( STATE_HANG_RIGHT )
    {
        camera.targetAngleX = -60 * DEG2SHORT;

        s_ignoreEnemy();

        if (!(input & IN_RIGHT))
        {
            goalState = STATE_HANG;
        }
    }

    S_HANDLER( STATE_SLIDE_BACK )
    {
        if (input & IN_JUMP)
        {
            goalState = STATE_JUMP_BACK;
        }
    }

    S_HANDLER( STATE_SURF_TREAD )
    {
        vSpeed = max(vSpeed - LARA_SURF_FRICTION, 0);

        if (input & IN_LEFT) {
            angle.y -= LARA_TURN_SLOW;
        } else if (input & IN_RIGHT) {
            angle.y += LARA_TURN_SLOW;
        }

        if (input & IN_UP) {
            goalState = STATE_SURF_SWIM;
        } else if (input & IN_DOWN) {
            goalState = STATE_SURF_BACK;
        } else if ((input & (IN_WALK | IN_LEFT)) == (IN_WALK | IN_LEFT)) {
            goalState = STATE_SURF_LEFT;
        } else if ((input & (IN_WALK | IN_RIGHT)) == (IN_WALK | IN_RIGHT)) {
            goalState = STATE_SURF_RIGHT;
        }

        if (input & IN_JUMP) {
            swimTimer++;
            if (swimTimer == LARA_SWIM_TIMER) {
                s_dive();
            }
        } else {
            swimTimer = 0;
        }
    }

    S_HANDLER( STATE_SURF_SWIM )
    {
        if (checkDeath(STATE_DEATH_UW))
            return;

        swimTimer = 0;

        if (input & IN_LEFT) {
            angle.y -= LARA_TURN_SLOW;
        } else if (input & IN_RIGHT) {
            angle.y += LARA_TURN_SLOW;
        }

        if (!(input & IN_UP) || (input & IN_JUMP)) {
            goalState = STATE_SURF_TREAD;
        }

        vSpeed = min(vSpeed + LARA_SURF_ACCEL, LARA_SURF_SPEED_MAX);
    }

    S_HANDLER( STATE_UW_DIVE )
    {
        if (input & IN_UP) {
            angle.x -= ANGLE_1;
        }
    }

    S_HANDLER( STATE_BLOCK_PUSH )
    {
        camera.targetAngleX = -25 * DEG2SHORT;
        camera.targetAngleY = 35 * DEG2SHORT;
        camera.center = true;

        s_ignoreEnemy();
    }

    S_HANDLER( STATE_BLOCK_PULL )
    {
        camera.targetAngleX = -25 * DEG2SHORT;
        camera.targetAngleY = 35 * DEG2SHORT;
        camera.center = true;

        s_ignoreEnemy();
    }

    S_HANDLER( STATE_BLOCK_READY )
    {
        camera.targetAngleY = 75 * DEG2SHORT;

        s_ignoreEnemy();

        if (!(input & IN_ACTION))
        {
            goalState = STATE_STOP;
        }
    }

    S_HANDLER( STATE_PICKUP )
    {
        camera.targetAngleX = -15 * DEG2SHORT;
        camera.targetAngleY = -130 * DEG2SHORT;
        camera.targetDist = 1024;

        s_ignoreEnemy();
    }

    S_HANDLER( STATE_SWITCH_DOWN )
    {
        camera.targetAngleX = -25 * DEG2SHORT;
        camera.targetAngleY = 80 * DEG2SHORT;
        camera.targetDist = 1024;

        s_ignoreEnemy();
    }

    S_HANDLER( STATE_SWITCH_UP )
    {
        camera.targetAngleX = -25 * DEG2SHORT;
        camera.targetAngleY = 80 * DEG2SHORT;
        camera.targetDist = 1024;

        s_ignoreEnemy();
    }

    S_HANDLER( STATE_USE_KEY )
    {
        camera.targetAngleX = -25 * DEG2SHORT;
        camera.targetAngleY = -80 * DEG2SHORT;
        camera.targetDist = 1024;

        s_ignoreEnemy();
    }

    S_HANDLER( STATE_USE_PUZZLE )
    {
        camera.targetAngleX = -25 * DEG2SHORT;
        camera.targetAngleY = -80 * DEG2SHORT;
        camera.targetDist = 1024;

        s_ignoreEnemy();
    }

    S_HANDLER( STATE_DEATH_UW )
    {
        vSpeed = X_MAX(vSpeed - LARA_SWIM_ACCEL, 0);
        angle.x = angleDec(angle.x, 2 * DEG2SHORT);
    }

    S_HANDLER( STATE_ROLL_START )
    {
        // empty
    }

    S_HANDLER( STATE_SPECIAL )
    {
        camera.targetAngleX = -25 * DEG2SHORT;
        camera.targetAngleY = 170 * DEG2SHORT;
        camera.center = true;
    }

    S_HANDLER( STATE_SURF_BACK )
    {
        if (checkDeath(STATE_DEATH_UW))
            return;

        swimTimer = 0;

        if (input & IN_LEFT) {
            angle.y -= LARA_TURN_VERY_SLOW;
        } else if (input & IN_RIGHT) {
            angle.y += LARA_TURN_VERY_SLOW;
        }

        if (!(input & IN_DOWN)) {
            goalState = STATE_SURF_TREAD;
        }

        vSpeed = min(vSpeed + LARA_SURF_ACCEL, LARA_SURF_SPEED_MAX);    
    }

    S_HANDLER( STATE_SURF_LEFT )
    {
        if (checkDeath(STATE_DEATH_UW))
            return;

        swimTimer = 0;

        if ((input & (IN_WALK | IN_LEFT)) != (IN_WALK | IN_LEFT)) {
            goalState = STATE_SURF_TREAD;
        }

        vSpeed = min(vSpeed + LARA_SURF_ACCEL, LARA_SURF_SPEED_MAX);
    }

    S_HANDLER( STATE_SURF_RIGHT )
    {
        if (checkDeath(STATE_DEATH_UW))
            return;

        swimTimer = 0;

        if ((input & (IN_WALK | IN_RIGHT)) != (IN_WALK | IN_RIGHT)) {
            goalState = STATE_SURF_TREAD;
        }

        vSpeed = min(vSpeed + LARA_SURF_ACCEL, LARA_SURF_SPEED_MAX);    
    }

    S_HANDLER( STATE_USE_MIDAS )
    {
        s_ignoreEnemy();
    }

    S_HANDLER( STATE_DEATH_MIDAS )
    {
        s_ignoreEnemy();
        flags.gravity = false;
    }

    S_HANDLER( STATE_SWAN_DIVE )
    {
        cinfo.enemyPush = true;
        cinfo.enemyHit  = false;

        s_checkFall();
    }

    S_HANDLER( STATE_FAST_DIVE )
    {
        cinfo.enemyPush = true;
        cinfo.enemyHit  = false;
        hSpeed = (hSpeed * 95) / 100;

        s_checkRoll();
    }

    S_HANDLER( STATE_HANDSTAND )
    {
        s_ignoreEnemy();
    }

    S_HANDLER( STATE_WATER_OUT )
    {
        s_ignoreEnemy();
        camera.center = true;
    }

    S_HANDLER( STATE_CLIMB_START  ) {}
    S_HANDLER( STATE_CLIMB_UP     ) {}
    S_HANDLER( STATE_CLIMB_LEFT   ) {}
    S_HANDLER( STATE_CLIMB_END    ) {}
    S_HANDLER( STATE_CLIMB_RIGHT  ) {}
    S_HANDLER( STATE_CLIMB_DOWN   ) {}
    S_HANDLER( STATE_UNUSED_1     ) {}
    S_HANDLER( STATE_UNUSED_2     ) {}
    S_HANDLER( STATE_UNUSED_3     ) {}
    S_HANDLER( STATE_WADE         ) {}
    S_HANDLER( STATE_ROLL_UW      ) {}
    S_HANDLER( STATE_PICKUP_FLARE ) {}
    S_HANDLER( STATE_ROLL_AIR     ) {}
    S_HANDLER( STATE_UNUSED_4     ) {}
    S_HANDLER( STATE_ZIPLINE      ) {}


// collision control
    void c_applyOffset()
    {
        pos += cinfo.offset;
        cinfo.offset = vec3i(0, 0, 0);
    }

    void c_angle(int32 angleDelta)
    {
        moveAngle = angle.y + angleDelta;

        cinfo.angle    = moveAngle;
        cinfo.quadrant = uint16(cinfo.angle + ANGLE_45) / ANGLE_90;
    }

    bool c_checkCeiling()
    {
        if (cinfo.type != CT_CEILING && cinfo.type != CT_FLOOR_CEILING) {
            return false;
        }

        animSet(ANIM_STAND, true);
        goalState = state;
        hSpeed = 0;
        vSpeed = 0;
        flags.gravity = false;
        pos = cinfo.pos;

        return true;
    }

    bool c_checkWall()
    {
        if (cinfo.type == CT_FRONT || cinfo.type == CT_FRONT_CEILING)
        {
            c_applyOffset();
            goalState = STATE_STOP;
            hSpeed = 0;
            flags.gravity = false;
            return true;
        }

        if (cinfo.type == CT_LEFT) {
            c_applyOffset();
            angle.y += 5 * DEG2SHORT;
            angle.z = angleDec(angle.z, 2 * DEG2SHORT);
        } else if (cinfo.type == CT_RIGHT) {
            c_applyOffset();
            angle.y -= 5 * DEG2SHORT;
            angle.z = angleDec(angle.z, 2 * DEG2SHORT);
        }

        return false;
    }

    bool c_checkWallUW()
    {
        if (cinfo.type == CT_FRONT) {
            if (angle.x > 35 * DEG2SHORT) {
                angle.x += 2 * DEG2SHORT;
            } else if (angle.x < -35 * DEG2SHORT) {
                angle.x -= 2 * DEG2SHORT;
            } else {
                vSpeed = 0;
            }
        } else if (cinfo.type == CT_CEILING) {
            if (angle.x >= -45 * DEG2SHORT) {
                angle.x -= 2 * DEG2SHORT;
            }
        } else if (cinfo.type == CT_FRONT_CEILING) {
            vSpeed = 0;
        } else if (cinfo.type == CT_LEFT) {
            angle.y += 5 * DEG2SHORT;
        } else if (cinfo.type == CT_RIGHT) {
            angle.y -= 5 * DEG2SHORT;
        } else if (cinfo.type == CT_FLOOR_CEILING) {
            pos = cinfo.pos;
            vSpeed = 0;
            return true;
        }

        if (cinfo.m.floor < 0) {
            pos.y += cinfo.m.floor;
            angle.x += 2 * DEG2SHORT;
        }

        int32 waterDepth = getWaterDepth();

        if (waterDepth == WALL) {
            vSpeed = 0;
            pos = cinfo.pos;
        } else if (waterDepth <= 512) {
            waterState = WATER_STATE_WADE;

            animSet(ANIM_SWIM_STAND, true);
            goalState = STATE_STOP;

            angle.x = 0;
            angle.z = 0;
            hSpeed = 0;
            vSpeed = 0;
            flags.gravity = false;
        }

        return false;
    }

    bool c_checkWallSurf()
    {
        if ((cinfo.m.floor < 0 && cinfo.m.slantType == SLANT_HIGH) || (cinfo.type & (CT_FRONT | CT_CEILING | CT_FRONT_CEILING | CT_FLOOR_CEILING))) {
            pos  = cinfo.pos;
            vSpeed = 0;
        } else if (cinfo.type == CT_LEFT) {
            angle.y += 5 * DEG2SHORT;
        } else if (cinfo.type == CT_RIGHT) {
            angle.y -= 5 * DEG2SHORT;
        }

        return true;
    }

    bool c_checkSlide()
    {
        if (waterState == WATER_STATE_WADE) {
            return false;
        }

        if (cinfo.m.slantType != SLANT_HIGH) {
            return false;
        }

        c_applyOffset();

        int16 realAngle;

        if (cinfo.slantX > 2) {
            realAngle = -ANGLE_90;
        } else if (cinfo.slantX < -2) {
            realAngle =  ANGLE_90;
        } else if (cinfo.slantZ > 2) {
            realAngle = ANGLE_180;
        } else {
            realAngle = 0;
        }

        if (abs(realAngle - angle.y) <= ANGLE_90) {
            if (state != STATE_SLIDE) {
                animSet(ANIM_SLIDE_FORTH, true);
            }
            moveAngle = realAngle;
            angle.y = realAngle;
        } else {
            if (state != STATE_SLIDE_BACK) {
                animSet(ANIM_SLIDE_BACK, true);
            }
            moveAngle = realAngle;
            angle.y = realAngle + ANGLE_180;
        }

        return true;
    }

    bool c_checkFall(int32 height, int32 fallAnimIndex = ANIM_FALL_FORTH)
    {
        if (waterState == WATER_STATE_WADE) {
            return false;
        }

        if (cinfo.m.floor <= height) {
            return false;
        }

        animSet(fallAnimIndex, true);

        vSpeed = 0;
        flags.gravity = true;

        return true;
    }

    bool c_checkLanding()
    {
        if ((state == STATE_FAST_DIVE || state == STATE_ROLL_AIR) && vSpeed > 133)
        {
            health = 0;
            return true;
        }

        int32 y = pos.y;
        pos.y += cinfo.m.floor;
        roomFloor = pos.y;

        checkTrigger(cinfo.trigger, this);

        pos.y = y;

        if (vSpeed <= 140)
            return false;

        if (vSpeed > 154) {
            health = 0;
        } else {
            health -= (X_SQR(vSpeed - 140) * LARA_MAX_HEALTH) / 196;
        }

        return checkDeath((State)state);
    }

    bool c_checkSwing()
    {
        int32 x = pos.x;
        int32 y = pos.y;
        int32 z = pos.z;

        switch (angle.y) {
            case  ANGLE_0   : z += 256; break;
            case  ANGLE_90  : x += 256; break;
            case -ANGLE_90  : x -= 256; break;
            case  ANGLE_180 : z -= 256; break;
        }

        Room* roomBelow = room->getRoom(x, y, z);
        const Sector* sector = roomBelow->getSector(x, z);
        int32 floor = sector->getFloor(x, y, z);

        if (floor != WALL) {
            int32 ceiling = sector->getCeiling(x, y, z);

            floor   -= y;
            ceiling -= y;

            if (floor > 0 && ceiling < -400) {
                return true;
            }
        }

        return false;
    }

    bool c_checkGrab()
    {
        return !(input & IN_ACTION) || (cinfo.type != CT_FRONT) || (abs(cinfo.r.floor - cinfo.l.floor) >= LARA_HANG_SLANT);
    }

    bool c_checkSpace()
    {
        return (cinfo.f.floor < cinfo.f.ceiling ||
                cinfo.l.floor < cinfo.l.ceiling ||
                cinfo.r.floor < cinfo.r.ceiling);
    }

    bool c_checkClimbStart()
    {
        return false;
    }

    bool c_checkClimbUp()
    {
        if (cinfo.f.floor == WALL)
            return false;

        if (c_checkGrab()) {
            return false;
        }

        int16 realAngle = angle.y;
        if (alignAngle(realAngle, 30 * DEG2SHORT)) {
            return false;
        }

        if (cinfo.f.floor >= -640 && cinfo.f.floor <= -384) {
            if (c_checkSpace()) return false;

            animSet(ANIM_CLIMB_2, true);
            state = STATE_HANG_UP;

            pos.y += 512 + cinfo.f.floor;
        } else if (cinfo.f.floor >= -896 && cinfo.f.floor <= -640) {
            if (c_checkSpace()) return false;

            animSet(ANIM_CLIMB_3, true);
            state = STATE_HANG_UP;

            pos.y += 768 + cinfo.f.floor;
        } else if (cinfo.f.floor >= -1920 && cinfo.f.floor <= -896) {
            animSet(ANIM_STAND, true);
            goalState = STATE_JUMP_UP;
            vSpeedHack = -int32(phd_sqrt(-2 * GRAVITY * (cinfo.f.floor + 800)) + 3);
            animProcess();
        /*} TODO climb 
          else if ((waterState != WATER_STATE_WADE) && (cinfo.f.floor <= -1920) && (cinfo.l.floor <= -1920) && (cinfo.r.floor <= -1920) && (cinfo.m.ceiling <= -1158)) {
            animSet(ANIM_STAND, true);
            goalState = STATE_JUMP_UP;
            vSpeedHack = -116;
            animProcess();
        } else if (((cinfo.f.floor < -1024) && (cinfo.f.ceiling >= 506)) || ((cinfo.m.ceiling <= -518) && c_checkClimbStart())) {
            animSet(ANIM_STAND, true);
            goalState = STATE_CLIMB_START;
            animProcess();*/
        } else {
            return false;
        }

        angle.y = realAngle;
        c_applyOffset();

        return true;
    }

    bool c_checkHang()
    {
        if (c_checkGrab()) {
            return false;
        }

        if ((cinfo.f.ceiling > 0) ||
            (cinfo.m.ceiling > -LARA_STEP_HEIGHT) ||
            (cinfo.m.floor < 200 && state == STATE_REACH))
        {
            return false;
        }

        int32 h = cinfo.f.floor - getBounds().minY;
        int32 v = h + vSpeed;

        if ((h < 0 && v < 0) || (h > 0 && v > 0)) {
            return false;
        }

        if (alignAngle(angle.y, 35 * DEG2SHORT)) {
            return false;
        }

        if (state == STATE_REACH)
        {
            if (c_checkSwing()) {
                animSet(ANIM_HANG_SWING, true);
            } else {
                animSet(ANIM_HANG, true);
            }
        } else {
            animSet(ANIM_HANG, true, 12);
        }

        cinfo.offset.y = cinfo.f.floor - getBounds().minY;

        c_applyOffset();

        flags.gravity = false;
        hSpeed = 0;
        vSpeed = 0;

        return true;
    }

    bool c_checkDrop()
    {
        // TODO getTrigger here

        if ((health > 0) && (input & IN_ACTION))
        {
            flags.gravity = false;
            vSpeed = 0;
            return false;
        }

        flags.gravity = true;
        hSpeed = 2;
        vSpeed = 1;

        animSet(ANIM_FALL_FORTH, true);

        return true;
    }

    bool c_checkWaterOut()
    {
        if (!(input & IN_ACTION) ||
            (cinfo.type != CT_FRONT) ||
            (cinfo.f.ceiling > 0) ||
            (cinfo.m.ceiling > -LARA_STEP_HEIGHT) ||
            (abs(cinfo.r.floor - cinfo.l.floor) >= LARA_HANG_SLANT))
        {
            return false;
        }

        int32 h = cinfo.f.floor + LARA_HEIGHT_SURF;

        if (h <= -512 || h > 316) {
            return false;
        }

        if (alignAngle(angle.y, 35 * DEG2SHORT)) {
            return false;
        }

        pos.y += h - 5;

        updateRoom(-LARA_HEIGHT / 2);

        alignWall(-LARA_RADIUS);
        
        if ((h < -128)) { // TODO || (level->version & TR::VER_TR1)) {
            animSet(ANIM_WATER_OUT, true);
        //    specular = LARA_WET_SPECULAR;
        } else if (h < 128) {
            animSet(ANIM_SURF_OUT, true);
        } else {
            animSet(ANIM_SURF_STAND, true);
        }

        //game->waterDrop(pos, 128.0f, 0.2f);

        animSet(ANIM_WATER_OUT, true);

        waterState = WATER_STATE_ABOVE;
        goalState = STATE_STOP;
        angle.x = 0;
        angle.z = 0;
        hSpeed = 0;
        vSpeed = 0;
        flags.gravity = false;

        return true;
    }

    void c_default()
    {
        cinfo.gapPos      = LARA_STEP_HEIGHT;
        cinfo.gapNeg      = -LARA_STEP_HEIGHT;
        cinfo.gapCeiling  = 0;
        cinfo.stopOnSlant = true;

        collideRoom(this, LARA_HEIGHT, 0);
    }

    void c_step()
    {
        cinfo.gapPos      = (waterState == WATER_STATE_WADE) ? -WALL : 128;
        cinfo.gapNeg      = -128;
        cinfo.gapCeiling  = 0;
        cinfo.stopOnSlant = true;

        collideRoom(this, LARA_HEIGHT, 0);

        if (c_checkCeiling())
            return;

        if (c_checkWall()) {
            animSet(ANIM_STAND, true);
        }

        if (c_checkSlide())
            return;

        pos.y += cinfo.m.floor;
    }

    void c_fall()
    {
        if (vSpeed <= 0 || cinfo.m.floor > 0)
            return;

        if (c_checkLanding()) {
            goalState = STATE_DEATH;
        } else if (state == STATE_JUMP && (input & IN_UP) && !(input & IN_WALK)) {
            goalState = STATE_RUN;
        } else if (state == STATE_FALL) {
            animSet(ANIM_LANDING, true);
        } else {
            goalState = STATE_STOP;
        }

        stopScreaming();

        pos.y += cinfo.m.floor;
        vSpeed = 0;
        flags.gravity = false;

        if (state == STATE_JUMP) {
            animProcess();
        }
    }

    void c_jump()
    {
        cinfo.gapPos = -WALL;
        cinfo.gapNeg = (state == STATE_REACH) ? 0 : -LARA_STEP_HEIGHT;
        cinfo.gapCeiling = 192;

        collideRoom(this, state == STATE_JUMP_UP ? LARA_HEIGHT_JUMP : LARA_HEIGHT, 0);

        if ((state == STATE_REACH || state == STATE_JUMP_UP) && c_checkHang())
            return;

        c_applyOffset();

        // TODO long up jump
        // TODO can't side jump near walls
        bool slide = (state == STATE_FALL) || (state == STATE_REACH) || (state == STATE_JUMP_UP);

        if ((cinfo.type == CT_CEILING) || (slide && (cinfo.type == CT_FRONT_CEILING))) {
            if (vSpeed <= 0) {
                vSpeed = 1;
            }
        } else if (!slide && ((cinfo.type == CT_FRONT) || (cinfo.type == CT_FRONT_CEILING))) {
            animSet(ANIM_SMASH_JUMP, true, 1);
            moveAngle += ANGLE_180;
            hSpeed /= 4;
            if (vSpeed <= 0) {
                vSpeed = 1;
            }
        } else if (cinfo.type == CT_FLOOR_CEILING) {
            pos.x -= (phd_sin(cinfo.angle) * LARA_RADIUS) >> FIXED_SHIFT;
            pos.z -= (phd_cos(cinfo.angle) * LARA_RADIUS) >> FIXED_SHIFT;
            cinfo.m.floor = 0;
            hSpeed = 0;
            if (vSpeed <= 0) {
                vSpeed = 16;
            }
        } else if (cinfo.type == CT_LEFT) {
            angle.y += 5 * DEG2SHORT;
        } else if (cinfo.type == CT_RIGHT) {
            angle.y -= 5 * DEG2SHORT;
        }

        c_fall();
    }

    void c_slide()
    {
        cinfo.gapPos      = -WALL;
        cinfo.gapNeg      = -512;
        cinfo.gapCeiling  = 0;
        cinfo.stopOnSlant = true;

        collideRoom(this, LARA_HEIGHT, 0);

        if (c_checkCeiling())
            return;

        c_checkWall();

        if (c_checkFall(200, state == STATE_SLIDE ? ANIM_FALL_FORTH : ANIM_FALL_BACK))
            return;

        c_checkSlide();

        pos.y += cinfo.m.floor;

        if (cinfo.m.slantType != SLANT_HIGH) {
            goalState = STATE_STOP;
        }
    }

    void c_roll()
    {
        vSpeed = 0;
        flags.gravity = false;

        cinfo.gapPos      = -WALL;
        cinfo.gapNeg      = -LARA_STEP_HEIGHT;
        cinfo.gapCeiling  = 0;
        cinfo.stopOnSlant = true;

        collideRoom(this, LARA_HEIGHT, 0);

        if (c_checkCeiling())
            return;

        if (c_checkSlide())
            return;

        if (c_checkFall(200, state == STATE_ROLL_START ? ANIM_FALL_FORTH : ANIM_FALL_BACK))
            return;

        c_applyOffset();

        pos.y += cinfo.m.floor;
    }

    void c_hang(int32 angleDelta)
    {
        c_angle(angleDelta);
        cinfo.gapPos     = -WALL;
        cinfo.gapNeg     = WALL;
        cinfo.gapCeiling = 0;
        collideRoom(this, LARA_HEIGHT, 0);

        bool noFloor = cinfo.f.floor < 200;

        c_angle(ANGLE_0);
        cinfo.gapPos     = -WALL;
        cinfo.gapNeg     = -LARA_STEP_HEIGHT;
        cinfo.gapCeiling = 0;

        switch (cinfo.quadrant)
        {
            case 0 : pos.z += 2; break;
            case 1 : pos.x += 2; break;
            case 2 : pos.z -= 2; break;
            case 3 : pos.x -= 2; break;
        }

        collideRoom(this, LARA_HEIGHT, 0);

        moveAngle = angle.y + angleDelta;

        if (health <= 0 || !(input & IN_ACTION))
        {
            animSet(ANIM_FALL_HANG, true, 9);

            cinfo.offset.y = cinfo.f.floor - getBounds().minY + 2;
            c_applyOffset();

            hSpeed = 2;
            vSpeed = 1;
            flags.gravity = true;
            return;
        }

        vSpeed  = 0;
        flags.gravity = false;

        if (noFloor || (cinfo.type != CT_FRONT) || (cinfo.m.ceiling >= 0) || abs(cinfo.r.floor - cinfo.l.floor) >= LARA_HANG_SLANT)
        {
            if (state != STATE_HANG) {
                animSet(ANIM_HANG, true, 21);
            }
            pos = cinfo.pos;
            return;
        }

        if (cinfo.quadrant & 1) {
            pos.x += cinfo.offset.x;
        } else {
            pos.z += cinfo.offset.z;
        }

        int32 h = cinfo.f.floor - getBounds().minY;
        if (abs(h) <= 256) {
            pos.y += h;
        }
    }

    enum ClimbState {
        CLIMB_HANG,
        CLIMB_COLLIDE,
        CLIMB_OK,
    };

    ClimbState c_climbCollide(int32 width)
    {
        /* TODO
        int32 dx = 0, dz = 0;

        int32 x = pos.x;
        int32 y = pos.y - 512;
        int32 z = pos.z;

        switch (cinfo.quadrant) {
            case 0 :
                x += width;
                z += cinfo.radius;
                dz = 4;
                break;
            case 1 :
                x += cinfo.radius;
                z -= width;
                dx = 4;
                break;
            case 2 :
                x -= width;
                z -= cinfo.radius;
                dz = -4;
                break;
            case 3 :
                x -= cinfo.radius;
                z += width;
                dx = -4;
                break;
        }

        // TODO
        cinfo.offset.y = 0;
        */
        return CLIMB_OK;
    }

    void c_climbSide(int32 width)
    {
        if (c_checkDrop())
            return;

        switch (c_climbCollide(width))
        {
            case CLIMB_HANG:
            {
                pos.x = cinfo.pos.x;
                pos.z = cinfo.pos.z;
                goalState = STATE_HANG;
                break;
            }

            case CLIMB_COLLIDE:
            {
                pos.x = cinfo.pos.x;
                pos.z = cinfo.pos.z;
                animSet(ANIM_CLIMB_START, true);
                break;
            }

            case CLIMB_OK:
            {
                if (input & IN_LEFT) {
                    goalState = STATE_CLIMB_LEFT;
                } else if (input & IN_RIGHT) {
                    goalState = STATE_CLIMB_RIGHT;
                } else {
                    goalState = STATE_CLIMB_START;
                }
                pos.y += cinfo.offset.y;
                break;
            }
        }
    }

    void c_swim() {
        c_angle(ANGLE_0);

        collideRoom(this, LARA_HEIGHT_UW, LARA_HEIGHT_UW / 2);

        c_applyOffset();

        if (c_checkWallUW())
            return;
    }

    void c_surf()
    {
        collideRoom(this, LARA_HEIGHT_SURF + 100, LARA_HEIGHT_SURF);

        c_applyOffset();

        c_checkWallSurf();

        if (state == STATE_SURF_TREAD) {
            if (frameIndex == 0) {
                //game->waterDrop(getJoint(jointHead).pos, 96.0f, 0.03f);
            }
        } else {
            if (frameIndex % 4 == 0) {
                //game->waterDrop(getJoint(jointHead).pos, 96.0f, 0.02f);
            }
        }

        int32 waterLevel = getWaterLevel();
        if (waterLevel - pos.y <= -100) {
            s_dive();
            return;
        }

        /* TODO
        if (level->version & TR::VER_TR1) {
            return;
        }

        if ((cinfo.type == CT_FRONT) || (cinfo.m.slantType == TR::SLANT_HIGH) || (cinfo.m.floor >= 0)) {
            return;
        }

        if (cinfo.m.floor >= -128) {
            if (targetState == STATE_SURF_LEFT) {
                targetState = STATE_STEP_LEFT;
            } else if (targetState == STATE_SURF_RIGHT) {
                targetState = STATE_STEP_RIGHT;
            } else {
                setAnimV2(ANIM_WADE, true);
            }
        } else {
            setAnimV2(ANIM_WADE_ASCEND, true);
            targetState = STATE_STOP;
        }

        v2pos.y += cinfo.f.floor + LARA_HEIGHT - 5;
        updateRoomV2(-LARA_HEIGHT / 2);

        gravity = false;
        v2rot.x = 0;
        v2rot.z = 0;
        hSpeed  = 0;
        vSpeed  = 0;
        waterState = WATER_STATE_WADE;
        */
    }

    C_HANDLER( STATE_WALK )
    {
        vSpeed = 0;
        flags.gravity = false;
        cinfo.stopOnLava = true;

        c_angle(ANGLE_0);
        c_default();

        if (c_checkCeiling())
            return;
        
        if (c_checkClimbUp())
            return;

        if (c_checkWall())
        {
            if (frameIndex >= 29 && frameIndex <= 47) {
                animSet(ANIM_STAND_RIGHT, false);
            } else if ((frameIndex >= 22 && frameIndex <= 28) || (frameIndex >= 48 && frameIndex <= 57)) {
                animSet(ANIM_STAND_LEFT, false);
            } else {
                animSet(ANIM_STAND, false);
            }
        }

        if (c_checkFall(LARA_STEP_HEIGHT))
            return;

        // descend
        if (cinfo.m.floor > 128)
        {
            if (frameIndex >= 28 && frameIndex <= 45) {
                animSet(ANIM_WALK_DESCEND_RIGHT, false);
            } else {
                animSet(ANIM_WALK_DESCEND_LEFT, false);
            }
        }

        // ascend
        if (cinfo.m.floor >= -LARA_STEP_HEIGHT && cinfo.m.floor < -128)
        {
            if (frameIndex >= 27 && frameIndex <= 44) {
                animSet(ANIM_WALK_ASCEND_RIGHT, false);
            } else {
                animSet(ANIM_WALK_ASCEND_LEFT, false);
            }
        }

        if (c_checkSlide())
            return;

        pos.y += cinfo.m.floor;
    }

    C_HANDLER( STATE_RUN )
    {
        c_angle(ANGLE_0);

        cinfo.gapPos      = -WALL;
        cinfo.gapNeg      = -LARA_STEP_HEIGHT;
        cinfo.gapCeiling  = 0;
        cinfo.stopOnSlant = true;

        collideRoom(this, LARA_HEIGHT, 0);

        if (c_checkCeiling())
            return;

        if (c_checkClimbUp())
            return;

        if (c_checkWall()) {
            angle.z = 0;

            if (cinfo.f.slantType == SLANT_NONE && cinfo.f.floor < -LARA_SMASH_HEIGHT && frameIndex < 22)
            {
                animSet(frameIndex < 10 ? ANIM_SMASH_RUN_LEFT : ANIM_SMASH_RUN_RIGHT, false);
                state = STATE_SPLAT;
                return;
            }

            animSet(ANIM_STAND, true);
        }

        if (c_checkFall(LARA_STEP_HEIGHT))
            return;

        // ascend
        if (cinfo.m.floor >= -LARA_STEP_HEIGHT && cinfo.m.floor < -128)
        {
            if (frameIndex >= 3 && frameIndex <= 14) {
                animSet(ANIM_RUN_ASCEND_RIGHT, false);
            } else {
                animSet(ANIM_RUN_ASCEND_LEFT, false);
            }
        }

        if (c_checkSlide())
            return;

        if (cinfo.m.floor >= 50)
        {
            pos.y += 50;
            return;
        }

        pos.y += cinfo.m.floor;
    }

    C_HANDLER( STATE_STOP )
    {
        vSpeed = 0;
        flags.gravity = false;

        c_angle(ANGLE_0);
        c_default();

        if (c_checkCeiling())
            return;

        if (c_checkFall(100))
            return;

        if (c_checkSlide())
            return;

        c_applyOffset();

        pos.y += cinfo.m.floor;
    }

    C_HANDLER( STATE_JUMP )
    {
        c_angle(ANGLE_0);
        c_jump();
    }

    C_HANDLER( STATE_POSE )
    {
        c_STATE_STOP();
    }

    C_HANDLER( STATE_BACK_FAST )
    {
        vSpeed = 0;
        flags.gravity = false;

        c_angle(ANGLE_180);

        cinfo.gapPos      = -WALL;
        cinfo.gapNeg      = -LARA_STEP_HEIGHT;
        cinfo.gapCeiling  = 0;
        cinfo.stopOnSlant = true;

        collideRoom(this, LARA_HEIGHT, 0);

        if (c_checkCeiling())
            return;

        if (c_checkFall(200, ANIM_FALL_BACK))
            return;

        if (c_checkWall()) {
            animSet(ANIM_STAND, false);
        }

        pos.y += cinfo.m.floor;
    }

    C_HANDLER( STATE_TURN_RIGHT )
    {
        c_angle(ANGLE_0);
        c_default();

        if (c_checkFall(100))
            return;

        if (c_checkSlide())
            return;

        pos.y += cinfo.m.floor;
    }

    C_HANDLER( STATE_TURN_LEFT )
    {
        c_STATE_TURN_RIGHT();
    }

    C_HANDLER( STATE_DEATH )
    {
        cinfo.radius = LARA_RADIUS * 4;
        
        c_angle(ANGLE_0);
        c_default();

        c_applyOffset();
        pos.y += cinfo.m.floor;
    }

    C_HANDLER( STATE_FALL )
    {
        flags.gravity = true;
        c_jump();
    }

    C_HANDLER( STATE_HANG )
    {
        c_hang(0);

        if ((input & IN_UP) && goalState == STATE_HANG)
        {
            if (abs(cinfo.f.floor) >= 850)
                return;

            if (c_checkSpace() || cinfo.staticHit)
                return;

            if (input & IN_WALK) {
                goalState = STATE_HANDSTAND;
            } else {
                goalState = STATE_HANG_UP;
            }
        }
    }

    C_HANDLER( STATE_REACH )
    {
        flags.gravity = true;
        c_angle(ANGLE_0);
        c_jump();
    }

    C_HANDLER( STATE_SPLAT )
    {
        c_angle(ANGLE_0);
        c_default();
        c_applyOffset();
    }

    C_HANDLER( STATE_UW_TREAD )
    {
        c_swim();
    }

    C_HANDLER( STATE_LAND )
    {
        c_STATE_STOP();
    }

    C_HANDLER( STATE_COMPRESS )
    {
        vSpeed  = 0;
        flags.gravity = false;

        cinfo.gapPos      = -WALL;
        cinfo.gapNeg      = WALL;
        cinfo.gapCeiling  = 0;

        collideRoom(this, LARA_HEIGHT, 0);

        if (cinfo.m.ceiling > -100)
        {
            animSet(ANIM_STAND, true);
            pos = cinfo.pos;
            hSpeed = 0;
            vSpeed = 0;
            flags.gravity = false;
        }
    }

    C_HANDLER( STATE_BACK )
    {
        vSpeed  = 0;
        flags.gravity = false;

        c_angle(ANGLE_180);

        cinfo.gapPos      = (waterState == WATER_STATE_WADE) ? -WALL : LARA_STEP_HEIGHT;
        cinfo.gapNeg      = -LARA_STEP_HEIGHT;
        cinfo.gapCeiling  = 0;
        cinfo.stopOnSlant = true;

        collideRoom(this, LARA_HEIGHT, 0);

        if (c_checkCeiling())
            return;

        if (c_checkWall())
        {
            animSet(ANIM_STAND, true);
        }

        if (cinfo.m.floor > 128 && cinfo.m.floor < LARA_STEP_HEIGHT)
        {
            if (frameIndex < 568) {
                animSet(ANIM_BACK_DESCEND_LEFT, false);
            } else {
                animSet(ANIM_BACK_DESCEND_RIGHT, false);
            }
        }

        if (c_checkSlide())
            return;

        pos.y += cinfo.m.floor;
    }

    C_HANDLER( STATE_UW_SWIM )
    {
        c_swim();
    }

    C_HANDLER( STATE_UW_GLIDE )
    {
        c_swim();
    }

    C_HANDLER( STATE_HANG_UP )
    {
        c_angle(ANGLE_0);
        c_default();
    }

    C_HANDLER( STATE_TURN_FAST )
    {
        c_STATE_STOP();
    }

    C_HANDLER( STATE_STEP_RIGHT )
    {
        c_angle(+ANGLE_90);
        c_step();
    }

    C_HANDLER( STATE_STEP_LEFT )
    {
        c_angle(-ANGLE_90);
        c_step();
    }

    C_HANDLER( STATE_ROLL_END )
    {
        c_angle(ANGLE_180);
        c_roll();
    }

    C_HANDLER( STATE_SLIDE )
    {
        c_angle(ANGLE_0);
        c_slide();
    }

    C_HANDLER( STATE_JUMP_BACK )
    {
        c_angle(ANGLE_180);
        c_jump();
    }

    C_HANDLER( STATE_JUMP_RIGHT )
    {
        c_angle(ANGLE_90);
        c_jump();
    }

    C_HANDLER( STATE_JUMP_LEFT )
    {
        c_angle(-ANGLE_90);
        c_jump();
    }

    C_HANDLER( STATE_JUMP_UP )
    {
        c_angle(ANGLE_0);
        c_jump();
    }

    C_HANDLER( STATE_FALL_BACK )
    {
        c_angle(ANGLE_180);
        c_jump();
    }

    C_HANDLER( STATE_HANG_LEFT )
    {
        c_hang(-ANGLE_90);
    }

    C_HANDLER( STATE_HANG_RIGHT )
    {
        c_hang(ANGLE_90);
    }

    C_HANDLER( STATE_SLIDE_BACK )
    {
        c_angle(ANGLE_180);
        c_slide();
    }

    C_HANDLER( STATE_SURF_TREAD )
    {
        c_angle(ANGLE_0);
        c_surf();
    }

    C_HANDLER( STATE_SURF_SWIM )
    {
        cinfo.gapNeg = -LARA_STEP_HEIGHT;

        c_angle(ANGLE_0);
        c_surf();
        c_checkWaterOut();
    }

    C_HANDLER( STATE_UW_DIVE )
    {
        c_swim();
    }

    C_HANDLER( STATE_BLOCK_PUSH )
    {
        c_angle(ANGLE_0);
        c_default();
    }

    C_HANDLER( STATE_BLOCK_PULL )
    {
        c_angle(ANGLE_0);
        c_default();
    }

    C_HANDLER( STATE_BLOCK_READY )
    {
        c_angle(ANGLE_0);
        c_default();
    }

    C_HANDLER( STATE_PICKUP )
    {
        c_angle(ANGLE_0);
        c_default();
    }

    C_HANDLER( STATE_SWITCH_DOWN )
    {
        c_angle(ANGLE_0);
        c_default();
    }

    C_HANDLER( STATE_SWITCH_UP )
    {
        c_angle(ANGLE_0);
        c_default();
    }

    C_HANDLER( STATE_USE_KEY )
    {
        c_angle(ANGLE_0);
        c_default();
    }

    C_HANDLER( STATE_USE_PUZZLE )
    {
        c_angle(ANGLE_0);
        c_default();
    }

    C_HANDLER( STATE_DEATH_UW )
    {
        health = 0;
        oxygen = 0;

        int16 waterLevel = getWaterLevel();
        if (waterLevel != WALL && waterLevel < pos.y - LARA_RADIUS) {
            pos.y -= LARA_FLOAT_UP_SPEED;
        }

        c_swim();
    }

    C_HANDLER( STATE_ROLL_START )
    {
        c_angle(ANGLE_0);
        c_roll();
    }

    C_HANDLER( STATE_SPECIAL )
    {
        // empty
    }

    C_HANDLER( STATE_SURF_BACK )
    {
        c_angle(ANGLE_180);
        c_surf();
    }

    C_HANDLER( STATE_SURF_LEFT )
    {
        c_angle(-ANGLE_90);
        c_surf();
    }

    C_HANDLER( STATE_SURF_RIGHT )
    {
        c_angle(ANGLE_90);
        c_surf();
    }

    C_HANDLER( STATE_USE_MIDAS )
    {
        c_angle(ANGLE_0);
        c_default();
    }

    C_HANDLER( STATE_DEATH_MIDAS )
    {
        c_angle(ANGLE_0);
        c_default();
    }

    C_HANDLER( STATE_SWAN_DIVE )
    {
        c_angle(ANGLE_0);
        c_jump();
    }

    C_HANDLER( STATE_FAST_DIVE )
    {
        c_angle(ANGLE_0);
        c_jump();
    }

    C_HANDLER( STATE_HANDSTAND )
    {
        c_angle(ANGLE_0);
        c_default();
    }

    C_HANDLER( STATE_WATER_OUT )
    {
        c_angle(ANGLE_0);
        c_default();
    }

    C_HANDLER( STATE_CLIMB_START  ) {}
    C_HANDLER( STATE_CLIMB_UP     ) {}
    C_HANDLER( STATE_CLIMB_LEFT   ) {}
    C_HANDLER( STATE_CLIMB_END    ) {}
    C_HANDLER( STATE_CLIMB_RIGHT  ) {}
    C_HANDLER( STATE_CLIMB_DOWN   ) {}
    C_HANDLER( STATE_UNUSED_1     ) {}
    C_HANDLER( STATE_UNUSED_2     ) {}
    C_HANDLER( STATE_UNUSED_3     ) {}
    C_HANDLER( STATE_WADE         ) {}
    C_HANDLER( STATE_ROLL_UW      ) {}
    C_HANDLER( STATE_PICKUP_FLARE ) {}
    C_HANDLER( STATE_ROLL_AIR     ) {}
    C_HANDLER( STATE_UNUSED_4     ) {}
    C_HANDLER( STATE_ZIPLINE      ) {}

    Lara(Room* room) : Item(room)
    {
        health = LARA_MAX_HEALTH;
        oxygen = LARA_MAX_OXYGEN;
        flags.shadow = true;

        activate();

        animSet(ANIM_STAND, true, 0);
    }

// update control
    void updateInput()
    {
        input = 0;

        if (camera.mode == CAMERA_MODE_FREE)
            return;

        if (keys & IK_LEFT)  input |= IN_LEFT;
        if (keys & IK_RIGHT) input |= IN_RIGHT;
        if (keys & IK_UP)    input |= IN_UP;
        if (keys & IK_DOWN)  input |= IN_DOWN;
        if (keys & IK_R)     input |= IN_WALK;
        if (keys & IK_A)     input |= IN_ACTION;
        if (keys & IK_B)     input |= IN_JUMP;
        if ((keys & (IK_L | IK_A)) == (IK_L | IK_A)) input |= IN_WEAPON;
        if ((keys & (IK_L | IK_B)) == (IK_L | IK_B)) input |= IN_UP | IN_DOWN;
        if ((keys & (IK_L | IK_R)) == (IK_L | IK_R)) input |= IN_LOOK;
    }

    void updateWaterState()
    {
        int32 waterLevel = getWaterLevel();
        int32 waterDist  = WALL;

        if (waterLevel != WALL) {
            waterDist = pos.y - waterLevel;
        }

    // change water state
        switch (waterState)
        {
            case WATER_STATE_ABOVE:
            {
                if (waterDist == WALL || waterDist < LARA_WADE_MIN_DEPTH) {
                    break;
                }

                int32 waterDepth = getWaterDepth();
                if (waterDepth > LARA_WADE_MAX_DEPTH - 256)
                {
                    if (!room->info->flags.water) // go dive
                        break;

                    waterState = WATER_STATE_UNDER;
                    flags.gravity = false;
                    oxygen = LARA_MAX_OXYGEN;

                    pos.y += 100;
                    updateRoom(0);
                    stopScreaming();

                    if (state == STATE_SWAN_DIVE) {
                        angle.x = -45 * DEG2SHORT;
                        goalState = STATE_UW_DIVE;
                        animProcess();
                        vSpeed *= 2;
                        //game->waterDrop(pos, 128.0f, 0.2f);
                    } else if (state == STATE_FAST_DIVE) {
                        angle.x = -85 * DEG2SHORT;
                        goalState = STATE_UW_DIVE;
                        animProcess();
                        vSpeed *= 2;
                        //game->waterDrop(pos, 128.0f, 0.2f);
                    } else {
                        angle.x = -45 * DEG2SHORT;
                        animSet(ANIM_WATER_FALL, true);
                        state = STATE_UW_DIVE; // TODO check necessary
                        goalState = STATE_UW_SWIM;
                        vSpeed = vSpeed * 3 / 2;
                        //game->waterDrop(pos, 256.0f, 0.2f);
                    }

                    //v2head.x = v2head.y = 0;
                    //v2torso.x = v2torso.y = 0;
                    //waterSplash();
                } else if (waterDist > LARA_WADE_MIN_DEPTH) {
                    waterState = WATER_STATE_WADE;
                    if (!flags.gravity) {
                        goalState = STATE_STOP;
                    }
                }
                break;
            }

            case WATER_STATE_SURFACE:
            {
                if (room->info->flags.water)
                    break;

                if (waterDist > LARA_WADE_MIN_DEPTH) {
                    waterState = WATER_STATE_WADE;
                    animSet(ANIM_STAND_NORMAL, true);
                    goalState = STATE_WADE;
                    animProcess();
                } else {
                    waterState = WATER_STATE_ABOVE;
                    animSet(ANIM_FALL_FORTH, true);
                    hSpeed = vSpeed / 4;
                    flags.gravity = true;
                }

                vSpeed = 0;
                angle.x = angle.z = 0;
                //v2head.x = v2head.y = 0;
                //v2torso.x = v2torso.y = 0;
                break;
            }

            case WATER_STATE_UNDER:
            {
                if (room->info->flags.water || flags.dozy)
                    break;

                if ((getWaterDepth() != WALL) && abs(waterDist) < 256) {
                    waterState = WATER_STATE_SURFACE;
                    pos.y -= (waterDist - 1);
                    animSet(ANIM_SURF, true);
                    vSpeed = 0;
                    swimTimer = LARA_SWIM_TIMER + 1; // block dive before we press jump button again
                    updateRoom(-LARA_HEIGHT / 2);
                    //game->playSound(TR::SND_BREATH, pos, Sound::PAN | Sound::UNIQUE);
                } else {
                    waterState = WATER_STATE_ABOVE;
                    animSet(ANIM_FALL_FORTH, true);
                    hSpeed = vSpeed / 4;
                    vSpeed = 0;
                    flags.gravity = true;
                }

                angle.x = angle.z = 0;
                //v2head.x = v2head.y = 0;
                //v2torso.x = v2torso.y = 0;
                break;
            }

            case WATER_STATE_WADE:
            {
                if (waterDist < LARA_WADE_MIN_DEPTH)
                {
                    waterState = WATER_STATE_ABOVE;
                    if (state == STATE_WADE) {
                        goalState = STATE_RUN;
                    }
                } else if (waterDist > LARA_WADE_MAX_DEPTH) {
                    waterState = WATER_STATE_SURFACE;
                    pos.y -= (waterDist - 1);

                    if (state == STATE_BACK) {
                        animSet(ANIM_SURF_BACK, true);
                    } else if (state == STATE_STEP_RIGHT) {
                        animSet(ANIM_SURF_RIGHT, true);
                    } else if (state == STATE_STEP_LEFT) {
                        animSet(ANIM_SURF_LEFT, true);
                    } else {
                        animSet(ANIM_SURF_SWIM, true);
                    }

                    swimTimer = 0;
                    vSpeed = 0;
                    flags.gravity = false;
                    angle.x = angle.z = 0;
                    //v2head.x = v2head.y = 0;
                    //v2torso.x = v2torso.y = 0;
                    updateRoom(0);
                }
                break;
            }
        }
    }

    void updateAbove()
    {
        cinfo.trigger     = NULL;
        cinfo.radius      = LARA_RADIUS;
        cinfo.pos         = pos;
        cinfo.enemyPush   = true;
        cinfo.enemyHit    = true;
        cinfo.stopOnSlant = false;
        cinfo.stopOnLava  = false;

        updateState();

        angle.z = angleDec(angle.z, 1 * DEG2SHORT);
        turnSpeed = angleDec(turnSpeed, 2 * DEG2SHORT);
        angle.y += turnSpeed;
    }

    void updateSurface()
    {
        cinfo.trigger     = NULL;
        cinfo.radius      = LARA_RADIUS;
        cinfo.gapPos      = -WALL;
        cinfo.gapNeg      = -128;
        cinfo.gapCeiling  = 100;
        cinfo.pos         = pos;
        cinfo.enemyPush   = false;
        cinfo.enemyHit    = false;
        cinfo.stopOnSlant = false;
        cinfo.stopOnLava  = false;

        updateState();

        angle.z = angleDec(angle.z, 2 * DEG2SHORT);

        pos.x += (phd_sin(moveAngle) * vSpeed) >> 16;
        pos.z += (phd_cos(moveAngle) * vSpeed) >> 16;

        camera.targetAngleX = -22 * DEG2SHORT;
    }

    void updateUnder()
    {
        cinfo.trigger     = NULL;
        cinfo.radius      = LARA_RADIUS_WATER;
        cinfo.gapPos      = -WALL;
        cinfo.gapNeg      = -LARA_HEIGHT_UW;
        cinfo.gapCeiling  = LARA_HEIGHT_UW;
        cinfo.pos         = pos;
        cinfo.enemyPush   = false;
        cinfo.enemyHit    = false;
        cinfo.stopOnSlant = false;
        cinfo.stopOnLava  = false;

        updateState();

        angle.z = angleDec(angle.z, 2 * DEG2SHORT);
        turnSpeed = angleDec(turnSpeed, 2 * DEG2SHORT);
        angle.y += turnSpeed;

        angle.x = X_CLAMP(angle.x, -85 * DEG2SHORT, 85 * DEG2SHORT);
        angle.z = X_CLAMP(angle.z, -22 * DEG2SHORT, 22 * DEG2SHORT);

        int32 c = phd_cos(angle.x);
        int32 s = phd_sin(angle.x);

        pos.y -= (s * vSpeed) >> 16;
        pos.x += (c * ((phd_sin(angle.y) * vSpeed) >> 16)) >> FIXED_SHIFT;
        pos.z += (c * ((phd_cos(angle.y) * vSpeed) >> 16)) >> FIXED_SHIFT;
    }

    void updateWeapon()
    {
        // TODO
    }

    virtual void update()
    {
        updateInput();

        updateWaterState();

        if (health > 0)
        {
            if (waterState == WATER_STATE_UNDER)
            {
                if (oxygen > 0) {
                    oxygen--;
                } else {
                    health -= 5;
                }
            } else {
                oxygen = X_MIN(oxygen + 10, LARA_MAX_OXYGEN);
            }
        }

        switch (waterState)
        {
            case WATER_STATE_ABOVE   :
            case WATER_STATE_WADE    : updateAbove(); break;
            case WATER_STATE_SURFACE : updateSurface(); break;
            case WATER_STATE_UNDER   : updateUnder(); break;
        }

        animProcess();

        updateCollision();

        int32 offset;
        if (waterState == WATER_STATE_SURFACE) {
            offset = LARA_RADIUS;
        } else if (waterState == WATER_STATE_UNDER) {
            offset = 0;
        } else {
            offset = -LARA_HEIGHT / 2;
        }
        updateRoom(offset);

        updateWeapon();

        checkTrigger(cinfo.trigger, this);
    }

    void meshSwap(uint16* meshList, uint16 start, uint32 mask)
    {
        for (int32 i = 0; i < JOINT_MAX; i++)
        {
            if ((1 << i) & mask)
            {
                meshList[i] = start + i;
            }
        }
    }

    virtual void draw()
    {
        uint16 meshList[JOINT_MAX];

        meshSwap(meshList, models[ITEM_LARA].start, 0xFFFFFFFF);

        // gym
        // t-rex death
        // midas death (gold)
        //meshSwap(meshList, models[ITEM_LARA_SPEC].start, JOINT_MASK_UPPER | JOINT_MASK_LOWER);

        drawModel(this, meshList);
    }
};

const Lara::Handler Lara::sHandlers[X_MAX] = { LARA_STATES(DECL_S_HANDLER) };
const Lara::Handler Lara::cHandlers[X_MAX] = { LARA_STATES(DECL_C_HANDLER) };

#undef DECL_ENUM
#undef DECL_S_HANDLER
#undef DECL_C_HANDLER
#undef S_HANDLER
#undef C_HANDLER

int32 doTutorial(Item* lara, int32 track)
{
    switch (track)
    {
    case 28 : if (gSaveGame.tracks[track].once && lara->state == Lara::STATE_JUMP_UP) return 29;
        case 37 : 
        case 41 : if (lara->state != Lara::STATE_HANG) return 0;
        case 42 : if (gSaveGame.tracks[track].once && lara->state == Lara::STATE_HANG) return 43;
        case 49 : if (lara->state != Lara::STATE_SURF_TREAD) return 0;
        case 50 : // end of GYM
            if (gSaveGame.tracks[track].once) {
                //timer += Core::deltaTime;
                //if (timer > 3.0f)
                //    game->loadNextLevel();
            } else {
                if (lara->state != Lara::STATE_WATER_OUT)
                    return 0;
                //timer = 0.0f;
            }
            break;
    }

    return track;
}

Lara* players[2];

Lara* getLara(const vec3i &pos)
{
    return players[0]; // TODO find nearest player
}

#endif
