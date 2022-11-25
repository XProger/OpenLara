#ifndef H_LARA
#define H_LARA

#include "common.h"
#include "item.h"
#include "camera.h"
#include "inventory.h"

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
#define LARA_HEIGHT             762
#define LARA_TURN_ACCEL         (ANGLE(9) / 4)
#define LARA_TURN_JUMP          ANGLE(3)
#define LARA_TURN_VERY_SLOW     ANGLE(2)
#define LARA_TURN_SLOW          ANGLE(4)
#define LARA_TURN_MED           ANGLE(6)
#define LARA_TURN_FAST          ANGLE(8)
#define LARA_TILT_ACCEL         (ANGLE(3) / 2)
#define LARA_TILT_MAX           ANGLE(11)
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
    JOINT_MASK_HIPS       = 1 << JOINT_HIPS,
    JOINT_MASK_LEG_L1     = 1 << JOINT_LEG_L1,
    JOINT_MASK_LEG_L2     = 1 << JOINT_LEG_L2,
    JOINT_MASK_LEG_L3     = 1 << JOINT_LEG_L3,
    JOINT_MASK_LEG_R1     = 1 << JOINT_LEG_R1,
    JOINT_MASK_LEG_R2     = 1 << JOINT_LEG_R2,
    JOINT_MASK_LEG_R3     = 1 << JOINT_LEG_R3,
    JOINT_MASK_TORSO      = 1 << JOINT_TORSO,
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
    JOINT_MASK_UPPER      = JOINT_MASK_TORSO  | JOINT_MASK_ARM_L  | JOINT_MASK_ARM_R,       // without head
    JOINT_MASK_LOWER      = JOINT_MASK_HIPS   | JOINT_MASK_LEG_L  | JOINT_MASK_LEG_R,
    JOINT_MASK_BRAID      = JOINT_MASK_HEAD   | JOINT_MASK_TORSO  | JOINT_MASK_ARM_L1 | JOINT_MASK_ARM_L2 | JOINT_MASK_ARM_R1 | JOINT_MASK_ARM_R2
};

EWRAM_DATA Lara* players[MAX_PLAYERS];
EWRAM_DATA CollisionInfo cinfo;

const WeaponParams weaponParams[WEAPON_MAX] = {
    { // WEAPON_PISTOLS
        ITEM_LARA_PISTOLS,  // modelType
        ITEM_LARA_PISTOLS,  // animType
        1,                  // damage
        ANGLE(8),           // spread
        8192,               // range
        650,                // height
        SND_PISTOLS_SHOT,   // soundId
        9,                  // reloadTimer
        155,                // flashOffset
        3,                  // flashTimer
        20,                 // flashIntensity
        ANGLE(60),          // aimX
        ANGLE(60),          // aimY
        ANGLE(80),          // armX
        ANGLE(-60),         // armMinY
        ANGLE(170),         // armMaxY
    },
    { // WEAPON_MAGNUMS
        ITEM_LARA_MAGNUMS,  // modelType
        ITEM_LARA_PISTOLS,  // animType
        2,                  // damage
        ANGLE(8),           // spread
        8192,               // range
        650,                // height
        SND_MAGNUMS_SHOT,   // soundId
        9,                  // reloadTimer
        155,                // flashOffset
        3,                  // flashTimer
        16,                 // flashIntensity
        ANGLE(60),          // aimX
        ANGLE(60),          // aimY
        ANGLE(80),          // armX
        ANGLE(-60),         // armMinY
        ANGLE(170),         // armMaxY
    },
    { // WEAPON_UZIS
        ITEM_LARA_UZIS,     // modelType
        ITEM_LARA_PISTOLS,  // animType
        1,                  // damage
        ANGLE(8),           // spread
        8192,               // range
        650,                // height
        SND_UZIS_SHOT,      // soundId
        3,                  // reloadTimer
        180,                // flashOffset
        2,                  // flashTimer
        10,                 // flashIntensity
        ANGLE(60),          // aimX
        ANGLE(60),          // aimY
        ANGLE(80),          // armX
        ANGLE(-60),         // armMinY
        ANGLE(170),         // armMaxY
    },
    { // WEAPON_SHOTGUN
        ITEM_LARA_SHOTGUN,  // modelType
        ITEM_LARA_SHOTGUN,  // animType
        4,                  // damage
        ANGLE(20),          // spread
        8192,               // range
        500,                // height
        SND_SHOTGUN_SHOT,   // soundId
        26,                 // reloadTimer
        0,                  // flashOffset
        0,                  // flashTimer
        0,                  // flashIntensity
        ANGLE(55),          // aimX
        ANGLE(60),          // aimY
        ANGLE(65),          // armX
        ANGLE(-80),         // armMinY
        ANGLE(80),          // armMaxY
    },
};

struct Lara : ItemObj
{
    enum State {
        LARA_STATES(DECL_ENUM)
        X_MAX
    };

    enum {
        ANIM_PISTOLS_AIM = 0,
        ANIM_PISTOLS_PICK,
        ANIM_PISTOLS_DRAW,
        ANIM_PISTOLS_FIRE,

        ANIM_SHOTGUN_AIM = 0,
        ANIM_SHOTGUN_DRAW,
        ANIM_SHOTGUN_FIRE
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
        ANIM_BLOCK_READY        = 120,

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

        ANIM_UW_ROLL            = 203
    };

    typedef void (Lara::*Handler)();

    static const Handler sHandlers[X_MAX];
    static const Handler cHandlers[X_MAX];

    void updateState()
    {
        (this->*sHandlers[state])();
    }

    void updateObjectsCollision()
    {
        if (health <= 0)
        {
            extraL->hitQuadrant = -1;
            return;
        }

        Room** adjRoom = room->getAdjRooms();
        while (*adjRoom)
        {
            ItemObj* item = (*adjRoom++)->firstItem;
            
            while (item)
            {
                if ((item->flags & ITEM_FLAG_STATUS) != ITEM_FLAG_STATUS_INVISIBLE)
                {
                    if (item->flags & ITEM_FLAG_COLLISION)
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
    }

    void updateCollision()
    {
        #ifndef __NDS__ // TODO
            updateObjectsCollision();
        #endif

        (this->*cHandlers[state])();

        // control hit animation
        if (extraL->hitTimer <= 0)
            return;

        if (!extraL->hitFrame) {
            soundPlay(SND_HIT, &pos);
        }

        extraL->hitFrame++;
        if (extraL->hitFrame > 34) {
            extraL->hitFrame = 34;
        }

        extraL->hitTimer--;
        if (extraL->hitTimer == 0)
        {
            extraL->hitQuadrant = -1;
            extraL->hitFrame = 0;
        }
    }

    void startScreaming()
    {
        soundPlay(SND_SCREAM, &pos);
    }

    void stopScreaming()
    {
        soundStop(SND_SCREAM);
    }

    void restore()
    {
        if (health > 0)
            return;

        health = LARA_MAX_HEALTH;
        oxygen = LARA_MAX_OXYGEN;
        animSet(ROOM_FLAG_WATER(room->info->flags) ? Lara::ANIM_UNDERWATER : Lara::ANIM_STAND, true, 0);
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

    bool s_getFront(int16 rot)
    {
        rot += angle.y;

        int32 s, c;
        sincos(rot, s, c);

        int32 x = pos.x + (s >> (FIXED_SHIFT - 8));
        int32 y = pos.y - LARA_HEIGHT;
        int32 z = pos.z + (c >> (FIXED_SHIFT - 8));

        Room* roomFront = room->getRoom(x, y, z);
        const Sector* sector = roomFront->getSector(x, z);
        int32 floor = sector->getFloor(x, y, z);

        if (floor != WALL) {
            floor -= pos.y;
        }

        return floor >= -LARA_STEP_HEIGHT;
    }

    bool s_checkDeath(int32 deathState)
    {
        if (health <= 0) {
            goalState = deathState;
            return true;
        }
        return false;
    }

// state control
    bool s_checkFront(int16 angleDelta, int32 radius) 
    {
        CollisionInfo tmpInfo = cinfo;
        int16 tmpAngle = extraL->moveAngle;

        c_angle(angleDelta);
        cinfo.radius      = radius;
        cinfo.type        = CT_NONE;
        cinfo.gapPos      = -WALL;
        cinfo.gapNeg      = -LARA_STEP_HEIGHT;
        cinfo.stopOnSlant = true;
        cinfo.gapCeiling  = 0;

        if ((angleDelta == ANGLE_180) && ((input & IN_WALK) || (waterState == WATER_STATE_WADE))) {
            cinfo.gapPos = LARA_STEP_HEIGHT;
            cinfo.stopOnLava = true;
        }

        collideRoom(LARA_HEIGHT, 0);

        bool collide = (cinfo.type == CT_FRONT) || (cinfo.type == CT_FRONT_CEILING);

        cinfo = tmpInfo;
        extraL->moveAngle = tmpAngle;

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
        if ((input & IN_UP) && s_checkFront(ANGLE_0, LARA_RADIUS + 4)) {
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
            angle.x -= ANGLE(2);
        } else if (input & IN_DOWN) {
            angle.x += ANGLE(2);
        }

        if (input & IN_LEFT) {
            turnSpeed = X_MAX(turnSpeed - LARA_TURN_ACCEL, -LARA_TURN_MED);
            angle.z -= LARA_TILT_ACCEL * 2;
        } else if (input & IN_RIGHT) {
            turnSpeed = X_MIN(turnSpeed + LARA_TURN_ACCEL,  LARA_TURN_MED);
            angle.z += LARA_TILT_ACCEL * 2;
        }
    }

    void s_dive()
    {
        animSet(ANIM_SURF_DIVE, true);
        angle.x = ANGLE(-45);
        vSpeed = LARA_DIVE_SPEED;
        waterState = WATER_STATE_UNDER;
    }

    bool s_checkLook()
    {
        if (input & IN_LOOK) {
            extraL->camera.mode = CAMERA_MODE_LOOK;
            return true;
        }

        if (extraL->camera.mode == CAMERA_MODE_LOOK) {
            extraL->camera.mode = CAMERA_MODE_FOLLOW;
        }

        return false;
    }

    S_HANDLER( STATE_WALK )
    {
        if (s_checkDeath(STATE_STOP))
            return;

        s_rotate(LARA_TURN_SLOW, 0);

        s_checkWalk(STATE_STOP);
    }

    S_HANDLER( STATE_RUN )
    {
        if (s_checkDeath(STATE_DEATH))
            return;

        if (s_checkRoll())
            return;

        s_rotate(LARA_TURN_FAST, 1);

        if ((input & IN_JUMP) && !(flags & ITEM_FLAG_GRAVITY)) {
            goalState = STATE_JUMP;
        } else {
            s_checkWalk(STATE_STOP);
        }
    }

    S_HANDLER( STATE_STOP )
    {
        if (s_checkDeath(STATE_DEATH))
            return;

        if (s_checkRoll())
            return;

        goalState = STATE_STOP;

        if (s_checkLook())
            return;

        if (input & IN_WALK) {
            if ((input & IN_LEFT) && s_checkFront(-ANGLE_90, LARA_RADIUS + 16)) {
                goalState = STATE_STEP_LEFT;
            } else if ((input & IN_RIGHT) && s_checkFront(ANGLE_90, LARA_RADIUS + 16)) {
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
        } else if ((input & IN_UP) && s_checkFront(ANGLE_0, LARA_RADIUS + 4)) {
            if (input & IN_WALK) {
                s_STATE_WALK();
            } else {
                s_STATE_RUN();
            }
        } else if ((input & IN_DOWN) && s_checkFront(ANGLE_180, LARA_RADIUS + 4)) {
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
            if (extraL->weaponState == WEAPON_STATE_FREE)
            {
                if (input & IN_ACTION)
                {
                    goalState = STATE_REACH;
                }
        
                if (input & IN_WALK)
                {
                    goalState = STATE_SWAN_DIVE;
                }
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
        if (s_checkDeath(STATE_STOP))
            return;

        if (input & IN_LOOK)
        {
            goalState = STATE_STOP;
            return;
        }

        turnSpeed += LARA_TURN_ACCEL;

        if ((turnSpeed > LARA_TURN_SLOW || extraL->weaponState == WEAPON_STATE_READY) && (waterState != WATER_STATE_WADE) && !(input & IN_WALK))
        {
            goalState = STATE_TURN_FAST;
        }

        if (goalState == state) {
            turnSpeed = X_MIN(turnSpeed, LARA_TURN_SLOW);
        }

        s_checkWalk((input & IN_RIGHT) ? goalState : STATE_STOP);
    }

    S_HANDLER( STATE_TURN_LEFT )
    {
        if (s_checkDeath(STATE_STOP))
            return;
            
        if (input & IN_LOOK)
        {
            goalState = STATE_STOP;
            return;
        }

        turnSpeed -= LARA_TURN_ACCEL;

        if ((turnSpeed < -LARA_TURN_SLOW || extraL->weaponState == WEAPON_STATE_READY) && (waterState != WATER_STATE_WADE) && !(input & IN_WALK))
        {
            goalState = STATE_TURN_FAST;
        }

        if (goalState == state) {
            turnSpeed = X_MAX(turnSpeed, -LARA_TURN_SLOW);
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
        extraL->camera.targetAngle.x = ANGLE(-60);

        s_ignoreEnemy();
        if (input & IN_LEFT) {
            goalState = STATE_HANG_LEFT;
        } else if (input & IN_RIGHT) {
            goalState = STATE_HANG_RIGHT;
        }
    }

    S_HANDLER( STATE_REACH )
    {
        extraL->camera.targetAngle.y = ANGLE(85);

        s_checkFall();
    }

    S_HANDLER( STATE_SPLAT )
    {
        // empty
    }

    S_HANDLER( STATE_UW_TREAD )
    {
        if (s_checkDeath(STATE_DEATH_UW))
            return;

        if (s_checkRoll())
            return;

        s_turnUW();

        if (input & IN_JUMP) {
            goalState = STATE_UW_SWIM;
        }

        vSpeed = X_MAX(vSpeed - LARA_SWIM_FRICTION, 0);
    }

    S_HANDLER( STATE_LAND )
    {
        // empty
    }

    S_HANDLER( STATE_COMPRESS )
    {
        if ((input & IN_UP) && s_getFront(ANGLE_0)) {
            goalState = STATE_JUMP;
        } else if ((input & IN_LEFT) && s_getFront(-ANGLE_90)) {
            goalState = STATE_JUMP_LEFT;
        } else if ((input & IN_RIGHT) && s_getFront(ANGLE_90)) {
            goalState = STATE_JUMP_RIGHT;
        } else if ((input & IN_DOWN) && s_getFront(ANGLE_180)) {
            goalState = STATE_JUMP_BACK;
        }
        s_checkFall();
    }

    S_HANDLER( STATE_BACK )
    {
        if (s_checkDeath(STATE_STOP))
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
        if (s_checkDeath(STATE_DEATH_UW))
            return;

        if (s_checkRoll())
            return;

        s_turnUW();

        vSpeed = X_MIN(vSpeed + LARA_SWIM_ACCEL, LARA_SWIM_SPEED_MAX);

        if (!(input & IN_JUMP)) {
            goalState = STATE_UW_GLIDE;
        }
    }

    S_HANDLER( STATE_UW_GLIDE )
    {
        if (s_checkDeath(STATE_DEATH_UW))
            return;

        if (s_checkRoll())
            return;

        s_turnUW();

        if (input & IN_JUMP) {
            goalState = STATE_UW_SWIM;
        }

        vSpeed = X_MAX(vSpeed - LARA_SWIM_FRICTION, 0);

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
        if (s_checkDeath(STATE_STOP))
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
        if (s_checkDeath(STATE_STOP))
            return;
            
        if ((input & (IN_WALK | IN_RIGHT)) != (IN_WALK | IN_RIGHT))
        {
            goalState = STATE_STOP;
        }
    }

    S_HANDLER( STATE_STEP_LEFT )
    {
        if (s_checkDeath(STATE_STOP))
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
        extraL->camera.targetAngle.x = ANGLE(-45);

        if (input & IN_JUMP) {
            goalState = STATE_JUMP;
        }
    }

    S_HANDLER( STATE_JUMP_BACK )
    {
        extraL->camera.targetAngle.y = ANGLE(135);

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
    
        if ((input & IN_ACTION) && (extraL->weaponState == WEAPON_STATE_FREE)) {
            goalState = STATE_REACH;
        }
    }

    S_HANDLER( STATE_HANG_LEFT )
    {
        extraL->camera.targetAngle.x = ANGLE(-60);

        s_ignoreEnemy();

        if (!(input & IN_LEFT)) {
            goalState = STATE_HANG;
        }
    }

    S_HANDLER( STATE_HANG_RIGHT )
    {
        extraL->camera.targetAngle.x = ANGLE(-60);

        s_ignoreEnemy();

        if (!(input & IN_RIGHT)) {
            goalState = STATE_HANG;
        }
    }

    S_HANDLER( STATE_SLIDE_BACK )
    {
        if (input & IN_JUMP) {
            goalState = STATE_JUMP_BACK;
        }
    }

    S_HANDLER( STATE_SURF_TREAD )
    {
        vSpeed = X_MAX(vSpeed - LARA_SURF_FRICTION, 0);

        if (s_checkDeath(STATE_DEATH_UW))
            return;

        if (s_checkLook())
            return;

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
            extraL->swimTimer++;
            if (extraL->swimTimer == LARA_SWIM_TIMER) {
                s_dive();
            }
        } else {
            extraL->swimTimer = 0;
        }
    }

    S_HANDLER( STATE_SURF_SWIM )
    {
        if (s_checkDeath(STATE_DEATH_UW))
            return;

        extraL->swimTimer = 0;

        if (input & IN_LEFT) {
            angle.y -= LARA_TURN_SLOW;
        } else if (input & IN_RIGHT) {
            angle.y += LARA_TURN_SLOW;
        }

        if (!(input & IN_UP) || (input & IN_JUMP)) {
            goalState = STATE_SURF_TREAD;
        }

        vSpeed = X_MIN(vSpeed + LARA_SURF_ACCEL, LARA_SURF_SPEED_MAX);
    }

    S_HANDLER( STATE_UW_DIVE )
    {
        if (input & IN_UP) {
            angle.x -= ANGLE_1;
        }
    }

    S_HANDLER( STATE_BLOCK_PUSH )
    {
        extraL->camera.targetAngle.x = ANGLE(-25);
        extraL->camera.targetAngle.y = ANGLE(35);
        extraL->camera.center = true;

        s_ignoreEnemy();
    }

    S_HANDLER( STATE_BLOCK_PULL )
    {
        extraL->camera.targetAngle.x = ANGLE(-25);
        extraL->camera.targetAngle.y = ANGLE(35);
        extraL->camera.center = true;

        s_ignoreEnemy();
    }

    S_HANDLER( STATE_BLOCK_READY )
    {
        extraL->camera.targetAngle.y = ANGLE(75);

        s_ignoreEnemy();

        if (!(input & IN_ACTION)) {
            goalState = STATE_STOP;
        }
    }

    S_HANDLER( STATE_PICKUP )
    {
        extraL->camera.targetAngle.x = ANGLE(-15);
        extraL->camera.targetAngle.y = ANGLE(-130);
        extraL->camera.targetDist = 1024;

        s_ignoreEnemy();
    }

    S_HANDLER( STATE_SWITCH_DOWN )
    {
        extraL->camera.targetAngle.x = ANGLE(-25);
        extraL->camera.targetAngle.y = ANGLE(80);
        extraL->camera.targetDist = 1024;

        s_ignoreEnemy();
    }

    S_HANDLER( STATE_SWITCH_UP )
    {
        extraL->camera.targetAngle.x = ANGLE(-25);
        extraL->camera.targetAngle.y = ANGLE(80);
        extraL->camera.targetDist = 1024;

        s_ignoreEnemy();
    }

    S_HANDLER( STATE_USE_KEY )
    {
        extraL->camera.targetAngle.x = ANGLE(-25);
        extraL->camera.targetAngle.y = ANGLE(-80);
        extraL->camera.targetDist = 1024;

        s_ignoreEnemy();
    }

    S_HANDLER( STATE_USE_PUZZLE )
    {
        extraL->camera.targetAngle.x = ANGLE(-25);
        extraL->camera.targetAngle.y = ANGLE(-80);
        extraL->camera.targetDist = 1024;

        s_ignoreEnemy();
    }

    S_HANDLER( STATE_DEATH_UW )
    {
        vSpeed = X_MAX(vSpeed - LARA_SWIM_ACCEL, 0);
        angle.x = angleDec(angle.x, ANGLE(2));
    }

    S_HANDLER( STATE_ROLL_START )
    {
        // empty
    }

    S_HANDLER( STATE_SPECIAL )
    {
        extraL->camera.targetAngle.x = ANGLE(-25);
        extraL->camera.targetAngle.y = ANGLE(170);
        extraL->camera.center = true;
    }

    S_HANDLER( STATE_SURF_BACK )
    {
        if (s_checkDeath(STATE_DEATH_UW))
            return;

        extraL->swimTimer = 0;

        if (input & IN_LEFT) {
            angle.y -= LARA_TURN_VERY_SLOW;
        } else if (input & IN_RIGHT) {
            angle.y += LARA_TURN_VERY_SLOW;
        }

        if (!(input & IN_DOWN)) {
            goalState = STATE_SURF_TREAD;
        }

        vSpeed = X_MIN(vSpeed + LARA_SURF_ACCEL, LARA_SURF_SPEED_MAX);    
    }

    S_HANDLER( STATE_SURF_LEFT )
    {
        if (s_checkDeath(STATE_DEATH_UW))
            return;

        extraL->swimTimer = 0;

        if ((input & (IN_WALK | IN_LEFT)) != (IN_WALK | IN_LEFT)) {
            goalState = STATE_SURF_TREAD;
        }

        vSpeed = X_MIN(vSpeed + LARA_SURF_ACCEL, LARA_SURF_SPEED_MAX);
    }

    S_HANDLER( STATE_SURF_RIGHT )
    {
        if (s_checkDeath(STATE_DEATH_UW))
            return;

        extraL->swimTimer = 0;

        if ((input & (IN_WALK | IN_RIGHT)) != (IN_WALK | IN_RIGHT)) {
            goalState = STATE_SURF_TREAD;
        }

        vSpeed = X_MIN(vSpeed + LARA_SURF_ACCEL, LARA_SURF_SPEED_MAX);    
    }

    S_HANDLER( STATE_USE_MIDAS )
    {
        s_ignoreEnemy();
    }

    S_HANDLER( STATE_DEATH_MIDAS )
    {
        s_ignoreEnemy();
        flags &= ~ITEM_FLAG_GRAVITY;
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
        extraL->camera.center = true;
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
        cinfo.offset = _vec3i(0, 0, 0);
    }

    void c_angle(int16 angleDelta)
    {
        angleDelta += angle.y;

        extraL->moveAngle = angleDelta;
        cinfo.setAngle(angleDelta);
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
        flags &= ~ITEM_FLAG_GRAVITY;
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
            flags &= ~ITEM_FLAG_GRAVITY;
            return true;
        }

        if (cinfo.type == CT_LEFT) {
            c_applyOffset();
            angle.y += ANGLE(5);
            angle.z = angleDec(angle.z, ANGLE(2));
        } else if (cinfo.type == CT_RIGHT) {
            c_applyOffset();
            angle.y -= ANGLE(5);
            angle.z = angleDec(angle.z, ANGLE(2));
        }

        return false;
    }

    bool c_checkWallUW()
    {
        if (cinfo.type == CT_FRONT) {
            if (angle.x > ANGLE(35)) {
                angle.x += ANGLE(2);
            } else if (angle.x < ANGLE(-35)) {
                angle.x -= ANGLE(2);
            } else {
                vSpeed = 0;
            }
        } else if (cinfo.type == CT_CEILING) {
            if (angle.x >= ANGLE(-45)) {
                angle.x -= ANGLE(2);
            }
        } else if (cinfo.type == CT_FRONT_CEILING) {
            vSpeed = 0;
        } else if (cinfo.type == CT_LEFT) {
            angle.y += ANGLE(5);
        } else if (cinfo.type == CT_RIGHT) {
            angle.y -= ANGLE(5);
        } else if (cinfo.type == CT_FLOOR_CEILING) {
            pos = cinfo.pos;
            vSpeed = 0;
            return true;
        }

        if (cinfo.m.floor < 0) {
            pos.y += cinfo.m.floor;
            angle.x += ANGLE(2);
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
            flags &= ~ITEM_FLAG_GRAVITY;
        }

        return false;
    }

    bool c_checkWallSurf()
    {
        if ((cinfo.m.floor < 0 && cinfo.m.slantType == SLANT_HIGH) || (cinfo.type & (CT_FRONT | CT_CEILING | CT_FRONT_CEILING | CT_FLOOR_CEILING))) {
            pos  = cinfo.pos;
            vSpeed = 0;
        } else if (cinfo.type == CT_LEFT) {
            angle.y += ANGLE(5);
        } else if (cinfo.type == CT_RIGHT) {
            angle.y -= ANGLE(5);
        }

        return true;
    }

    bool c_checkSlide()
    {
        if (waterState == WATER_STATE_WADE)
            return false;

        if (cinfo.m.slantType != SLANT_HIGH)
            return false;

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
            extraL->moveAngle = realAngle;
            angle.y = realAngle;
        } else {
            if (state != STATE_SLIDE_BACK) {
                animSet(ANIM_SLIDE_BACK, true);
            }
            extraL->moveAngle = realAngle;
            angle.y = realAngle + ANGLE_180;
        }

        return true;
    }

    bool c_checkFall(int32 height, int32 fallAnimIndex = ANIM_FALL_FORTH)
    {
        if (waterState == WATER_STATE_WADE)
            return false;

        if (cinfo.m.floor <= height)
            return false;

        animSet(fallAnimIndex, true);

        vSpeed = 0;
        flags |= ITEM_FLAG_GRAVITY;

        return true;
    }

    bool c_checkLanding()
    {
        if ((state == STATE_FAST_DIVE || state == STATE_ROLL_AIR) && vSpeed > 133)
        {
            hit(LARA_MAX_HEALTH, pos, 0);
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
            hit(LARA_MAX_HEALTH, pos, 0);
        } else {
            hit((X_SQR(vSpeed - 140) * LARA_MAX_HEALTH) / 196, pos, 0);
        }

        return health <= 0;
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

        if (floor != WALL)
        {
            int32 ceiling = sector->getCeiling(x, y, z);

            floor   -= y;
            ceiling -= y;

            if (floor > 0 && ceiling < -400)
                return true;
        }

        return false;
    }

    bool c_checkGrab()
    {
        return (extraL->weaponState != WEAPON_STATE_FREE) || !(input & IN_ACTION) || (cinfo.type != CT_FRONT) || (abs(cinfo.r.floor - cinfo.l.floor) >= LARA_HANG_SLANT);
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

        if (c_checkGrab())
            return false;

        int16 realAngle = angle.y;
        if (alignAngle(realAngle, ANGLE(30)))
            return false;

        if (cinfo.f.floor >= -640 && cinfo.f.floor <= -384) {
            if (c_checkSpace())
                return false;

            setWeaponState(WEAPON_STATE_BUSY);
            animSet(ANIM_CLIMB_2, true);
            state = STATE_HANG_UP;

            pos.y += 512 + cinfo.f.floor;
        } else if (cinfo.f.floor >= -896 && cinfo.f.floor <= -640) {
            if (c_checkSpace())
                return false;

            setWeaponState(WEAPON_STATE_BUSY);
            animSet(ANIM_CLIMB_3, true);
            state = STATE_HANG_UP;

            pos.y += 768 + cinfo.f.floor;
        } else if (cinfo.f.floor >= -1920 && cinfo.f.floor <= -896) {
            animSet(ANIM_STAND, true);
            goalState = STATE_JUMP_UP;
            extraL->vSpeedHack = int32(phd_sqrt(-2 * GRAVITY * (cinfo.f.floor + 800)) + 3);
            animProcess();
        /*} TODO climb 
          else if ((waterState != WATER_STATE_WADE) && (cinfo.f.floor <= -1920) && (cinfo.l.floor <= -1920) && (cinfo.r.floor <= -1920) && (cinfo.m.ceiling <= -1158)) {
            animSet(ANIM_STAND, true);
            goalState = STATE_JUMP_UP;
            vSpeedHack = 116;
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
        if (c_checkGrab())
            return false;

        if ((cinfo.f.ceiling > 0) ||
            (cinfo.m.ceiling > -LARA_STEP_HEIGHT) ||
            (cinfo.m.floor < 200 && state == STATE_REACH))
        {
            return false;
        }

        int32 h = cinfo.f.floor - getBoundingBox(true).minY;
        int32 v = h + vSpeed;

        if ((h < 0 && v < 0) || (h > 0 && v > 0))
            return false;

        if (alignAngle(angle.y, ANGLE(35)))
            return false;

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

        setWeaponState(WEAPON_STATE_BUSY);
        cinfo.offset.y = cinfo.f.floor - getBoundingBox(true).minY;

        c_applyOffset();

        flags &= ~ITEM_FLAG_GRAVITY;
        hSpeed = 0;
        vSpeed = 0;

        return true;
    }

    bool c_checkDrop()
    {
        // TODO getTrigger here

        if ((health > 0) && (input & IN_ACTION))
        {
            flags &= ~ITEM_FLAG_GRAVITY;
            vSpeed = 0;
            return false;
        }

        flags |= ITEM_FLAG_GRAVITY;
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

        if (h <= -512 || h > 316)
            return false;

        if (alignAngle(angle.y, ANGLE(35)))
            return false;

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
        setWeaponState(WEAPON_STATE_BUSY);

        waterState = WATER_STATE_ABOVE;
        goalState = STATE_STOP;
        angle.x = 0;
        angle.z = 0;
        hSpeed = 0;
        vSpeed = 0;
        flags &= ~ITEM_FLAG_GRAVITY;

        return true;
    }

    void c_default()
    {
        cinfo.gapPos      = LARA_STEP_HEIGHT;
        cinfo.gapNeg      = -LARA_STEP_HEIGHT;
        cinfo.gapCeiling  = 0;
        cinfo.stopOnSlant = true;

        collideRoom(LARA_HEIGHT, 0);
    }

    void c_step()
    {
        cinfo.gapPos      = (waterState == WATER_STATE_WADE) ? -WALL : 128;
        cinfo.gapNeg      = -128;
        cinfo.gapCeiling  = 0;
        cinfo.stopOnSlant = true;

        collideRoom(LARA_HEIGHT, 0);

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
        flags &= ~ITEM_FLAG_GRAVITY;

        if (state == STATE_JUMP) {
            animProcess();
        }
    }

    void c_jump()
    {
        cinfo.gapPos = -WALL;
        cinfo.gapNeg = (state == STATE_REACH) ? 0 : -LARA_STEP_HEIGHT;
        cinfo.gapCeiling = 192;

        collideRoom(state == STATE_JUMP_UP ? LARA_HEIGHT_JUMP : LARA_HEIGHT, 0);

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
            osJoyVibrate(0, 0xFF, 0xFF);
            animSet(ANIM_SMASH_JUMP, true, 1);
            extraL->moveAngle += ANGLE_180;
            hSpeed >>= 2;
            if (vSpeed <= 0) {
                vSpeed = 1;
            }
        } else if (cinfo.type == CT_FLOOR_CEILING) {
            int32 s, c;
            sincos(cinfo.angle, s, c);
            pos.x -= (s * LARA_RADIUS) >> FIXED_SHIFT;
            pos.z -= (c * LARA_RADIUS) >> FIXED_SHIFT;
            cinfo.m.floor = 0;
            hSpeed = 0;
            if (vSpeed <= 0) {
                vSpeed = 16;
            }
        } else if (cinfo.type == CT_LEFT) {
            angle.y += ANGLE(5);
        } else if (cinfo.type == CT_RIGHT) {
            angle.y -= ANGLE(5);
        }

        c_fall();
    }

    void c_slide()
    {
        cinfo.gapPos      = -WALL;
        cinfo.gapNeg      = -512;
        cinfo.gapCeiling  = 0;
        cinfo.stopOnSlant = true;

        collideRoom(LARA_HEIGHT, 0);

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
        flags &= ~ITEM_FLAG_GRAVITY;

        cinfo.gapPos      = -WALL;
        cinfo.gapNeg      = -LARA_STEP_HEIGHT;
        cinfo.gapCeiling  = 0;
        cinfo.stopOnSlant = true;

        collideRoom(LARA_HEIGHT, 0);

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
        collideRoom(LARA_HEIGHT, 0);

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

        collideRoom(LARA_HEIGHT, 0);

        extraL->moveAngle = angle.y + angleDelta;

        if (health <= 0 || !(input & IN_ACTION))
        {
            animSet(ANIM_FALL_HANG, true, 9);

            cinfo.offset.y = cinfo.f.floor - getBoundingBox(true).minY + 2;
            c_applyOffset();

            hSpeed = 2;
            vSpeed = 1;
            flags |= ITEM_FLAG_GRAVITY;
            setWeaponState(WEAPON_STATE_FREE);
            return;
        }

        vSpeed  = 0;
        flags &= ~ITEM_FLAG_GRAVITY;

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

        int32 h = cinfo.f.floor - getBoundingBox(true).minY;
        if (abs(h) <= 256) {
            pos.y += h;
        }
    }

    enum ClimbState {
        CLIMB_HANG,
        CLIMB_COLLIDE,
        CLIMB_OK
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

    void c_swim()
    {
        c_angle(ANGLE_0);

        collideRoom(LARA_HEIGHT_UW, LARA_HEIGHT_UW / 2);

        c_applyOffset();

        if (c_checkWallUW())
            return;
    }

    void c_surf()
    {
        collideRoom(LARA_HEIGHT_SURF + 100, LARA_HEIGHT_SURF);

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
        flags &= ~ITEM_FLAG_GRAVITY;
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

        collideRoom(LARA_HEIGHT, 0);

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
        flags &= ~ITEM_FLAG_GRAVITY;

        c_angle(ANGLE_0);

        if (input == (IN_UP | IN_ACTION)) // to check front climb up from STOP state
        {
            int32 s, c;
            sincos(cinfo.angle, s, c);
            pos.x += (s * 4) >> FIXED_SHIFT;
            pos.z += (c * 4) >> FIXED_SHIFT;
        }

        c_default();

        if (c_checkClimbUp())
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
        flags &= ~ITEM_FLAG_GRAVITY;

        c_angle(ANGLE_180);

        cinfo.gapPos      = -WALL;
        cinfo.gapNeg      = -LARA_STEP_HEIGHT;
        cinfo.gapCeiling  = 0;
        cinfo.stopOnSlant = true;

        collideRoom(LARA_HEIGHT, 0);

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
        flags |= ITEM_FLAG_GRAVITY;
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
        flags |= ITEM_FLAG_GRAVITY;
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
        flags &= ~ITEM_FLAG_GRAVITY;

        cinfo.gapPos      = -WALL;
        cinfo.gapNeg      = WALL;
        cinfo.gapCeiling  = 0;

        collideRoom(LARA_HEIGHT, 0);

        if (cinfo.m.ceiling > -100)
        {
            animSet(ANIM_STAND, true);
            pos = cinfo.pos;
            hSpeed = 0;
            vSpeed = 0;
        }
    }

    C_HANDLER( STATE_BACK )
    {
        vSpeed  = 0;
        flags &= ~ITEM_FLAG_GRAVITY;

        c_angle(ANGLE_180);

        cinfo.gapPos      = (waterState == WATER_STATE_WADE) ? -WALL : LARA_STEP_HEIGHT;
        cinfo.gapNeg      = -LARA_STEP_HEIGHT;
        cinfo.gapCeiling  = 0;
        cinfo.stopOnSlant = true;

        collideRoom(LARA_HEIGHT, 0);

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

    Lara(Room* room) : ItemObj(room)
    {
        int32 playerIndex = -1;

        for (int32 i = 0; i < X_COUNT(players); i++)
        {
            if (players[i] == this) {
                playerIndex = i;
                break;
            }
        }
        
        ASSERT(playerIndex != -1);

        extraL = &playersExtra[playerIndex];
        memset(extraL, 0, sizeof(*extraL));
        extraL->hitQuadrant = -1;

        extraL->weapon = extraL->goalWeapon = WEAPON_PISTOLS; // TODO LEVEL10A
        setWeaponState(WEAPON_STATE_FREE);
        meshSwap(ITEM_LARA, 0xFFFFFFFF);

        bool isHome = gLevelID == LVL_TR1_GYM;

        if (isHome) {
            meshSwap(ITEM_LARA_SPEC, JOINT_MASK_UPPER | JOINT_MASK_LOWER);
            extraL->ammo[WEAPON_PISTOLS] = 0;
        } else {
            extraL->ammo[WEAPON_PISTOLS] = -1;
            extraL->ammo[WEAPON_MAGNUMS] = -1;
            extraL->ammo[WEAPON_UZIS] = -1;
            extraL->ammo[WEAPON_SHOTGUN] = -1;

            if (extraL->weapon != WEAPON_MAX)
            {
                meshSwapPistols(JOINT_MASK_LEG_R1 | JOINT_MASK_LEG_L1, JOINT_MASK_ARM_R3 | JOINT_MASK_ARM_L3);
                // TODO check if shotgun on back
                meshSwapShotgun(false);
            }

            //extraL->weapon = extraL->goalWeapon = WEAPON_SHOTGUN;
        }

        animSet(ANIM_STAND, true, 0);

        health = LARA_MAX_HEALTH;
        oxygen = LARA_MAX_OXYGEN;
        flags |= ITEM_FLAG_SHADOW;

        extraL->camera.init(this);
        extraL->healthTimer = 100;
    }

// update control
    void updateInput()
    {
        extraL->lastInput = input;

        input = 0;

    #if defined(__3DO__)
        if (keys & IK_A) input |= IN_JUMP;
        if (keys & IK_B) input |= IN_ACTION;
        if (keys & IK_C) input |= IN_WEAPON;

        if ((keys & (IK_L | IK_R)) == (IK_L | IK_R)) {
            input |= IN_UP | IN_DOWN;
        } else {
            if (keys & IK_L) input |= IN_LOOK;
            if (keys & IK_R) input |= IN_WALK;
        }
    #elif defined(__32X__)
        // 6 buttons
        if (keys & IK_A) input |= IN_ACTION;
        if (keys & IK_B) input |= IN_JUMP;
        if (keys & IK_C) input |= IN_WEAPON;
        if (keys & IK_X) input |= IN_WALK;
        if (keys & IK_Y) input |= IN_UP | IN_DOWN;
        if (keys & IK_Z) input |= IN_LOOK;
    #elif defined(__GBA__) || defined(__GBA_WIN__)
        int32 ikA, ikB;

        if (gSettings.controls_swap) {
            ikA = IK_B;
            ikB = IK_A;
        } else {
            ikA = IK_A;
            ikB = IK_B;
        }

        if (keys & ikA)
        {
            if (keys & IK_L) {
                if (extraL->weaponState != WEAPON_STATE_BUSY) {
                    input |= IN_WEAPON;
                } else {
                    input |= IN_ACTION;
                }
            } else {
                input |= IN_ACTION;
            }
        }

        if (keys & ikB)
        {
            if (keys & IK_L) {
                input |= IN_UP | IN_DOWN;
            } else {
                input |= IN_JUMP;
            }
        }

        if (keys & IK_R)
        {
            if (keys & IK_L) {
                input |= IN_LOOK;
            } else {
                input |= IN_WALK;
            }
        }
    #elif defined(__NDS__)
        if (keys & IK_A) input |= IN_UP | IN_DOWN;
        if (keys & IK_B) input |= IN_ACTION;
        if (keys & IK_X) input |= IN_WEAPON;
        if (keys & IK_Y) input |= IN_JUMP;
        if (keys & IK_L) input |= IN_LOOK;
        if (keys & IK_R) input |= IN_WALK;
    #elif defined(__WIN32__)
        if (keys & IK_A) input |= IN_ACTION;
        if (keys & IK_B) input |= IN_UP | IN_DOWN;
        if (keys & IK_Y) input |= IN_WEAPON;
        if (keys & IK_X) input |= IN_JUMP;
        if (keys & IK_L) input |= IN_LOOK;
        if (keys & IK_R) input |= IN_WALK;
    #endif

        if (keys & IK_LEFT)    input |= IN_LEFT;
        if (keys & IK_RIGHT)   input |= IN_RIGHT;
        if (keys & IK_UP)      input |= IN_UP;
        if (keys & IK_DOWN)    input |= IN_DOWN;
        if (keys & IK_SELECT)  input |= IN_SELECT;

        if (extraL->camera.mode == CAMERA_MODE_FREE) {
            input = 0;
        }

        if (keys & IK_START)   input |= IN_START;

        if (isKeyHit(IN_START) && (inventory.state == INV_STATE_NONE))
        {
            if (extraL->camera.mode != CAMERA_MODE_FREE) {
                extraL->camera.mode = CAMERA_MODE_FREE;
            } else {
                extraL->camera.mode = CAMERA_MODE_FOLLOW;
            }
        }
    }

    void updateLook()
    {
        ExtraInfoLara::Arm &R = extraL->armR;
        ExtraInfoLara::Arm &L = extraL->armL;
        vec3s &H = extraL->head.angle;
        vec3s &T = extraL->torso.angle;

        if (health <= 0) {
            H = T = _vec3s(0, 0, 0);
            return;
        }

        if (R.target || L.target)
        {
            if (extraL->weapon < WEAPON_SHOTGUN)
            {
                int32 aX = R.angle.x + L.angle.x;
                int32 aY = R.angle.y + L.angle.y;

                if (R.aim && L.aim) {
                    H.x = T.x = aX >> 2;
                    H.y = T.y = aY >> 2;
                } else {
                    H.x = T.x = aX >> 1;
                    H.y = T.y = aY >> 1;
                }
            } else {
                T.x = R.angle.x;
                T.y = R.angle.y;
                H.x = H.y = 0;
            }
            return;
        }

        if ((input & IN_LOOK) && extraL->camera.mode != CAMERA_MODE_FIXED)
        {
            extraL->camera.lookAtItem = NULL;

            if (input & IN_UP) {
                H.x -= LARA_LOOK_TURN_SPEED;
            }

            if (input & IN_DOWN) {
                H.x += LARA_LOOK_TURN_SPEED;
            }

            if (input & IN_LEFT) {
                H.y -= LARA_LOOK_TURN_SPEED;
            }

            if (input & IN_RIGHT) {
                H.y += LARA_LOOK_TURN_SPEED;
            }

            H.x = T.x = X_CLAMP(H.x, LARA_LOOK_ANGLE_MIN, LARA_LOOK_ANGLE_MAX);
            H.y = T.y = X_CLAMP(H.y, -LARA_LOOK_ANGLE_Y, LARA_LOOK_ANGLE_Y);

            input &= ~(IN_RIGHT | IN_LEFT | IN_UP | IN_DOWN);
            return;
        }

        if (extraL->camera.lastItem != NULL)
            return;

        H.x = T.x = angleDec(H.x, abs(H.x) >> 3);
        H.y = T.y = angleDec(H.y, abs(H.y) >> 3);
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
                    if (!ROOM_FLAG_WATER(room->info->flags)) // go dive
                        break;

                    waterState = WATER_STATE_UNDER;
                    flags &= ~ITEM_FLAG_GRAVITY;
                    oxygen = LARA_MAX_OXYGEN;

                    pos.y += 100;
                    updateRoom(0);
                    stopScreaming();

                    if (state == STATE_SWAN_DIVE) {
                        angle.x = ANGLE(-45);
                        goalState = STATE_UW_DIVE;
                        animProcess();
                        vSpeed *= 2;
                        //game->waterDrop(pos, 128.0f, 0.2f);
                    } else if (state == STATE_FAST_DIVE) {
                        angle.x = ANGLE(-85);
                        goalState = STATE_UW_DIVE;
                        animProcess();
                        vSpeed *= 2;
                        //game->waterDrop(pos, 128.0f, 0.2f);
                    } else {
                        angle.x = ANGLE(-45);
                        animSet(ANIM_WATER_FALL, true);
                        state = STATE_UW_DIVE; // TODO check necessary
                        goalState = STATE_UW_SWIM;
                        vSpeed = vSpeed * 3 / 2;
                        //game->waterDrop(pos, 256.0f, 0.2f);
                    }

                    fxSplash();
                } else if (waterDist > LARA_WADE_MIN_DEPTH) {
                    waterState = WATER_STATE_WADE;
                    if (!(flags & ITEM_FLAG_GRAVITY)) {
                        goalState = STATE_STOP;
                    }
                }
                break;
            }

            case WATER_STATE_SURFACE:
            {
                if (ROOM_FLAG_WATER(room->info->flags))
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
                    flags |= ITEM_FLAG_GRAVITY;
                }

                vSpeed = 0;
                angle.x = angle.z = 0;
                break;
            }

            case WATER_STATE_UNDER:
            {
                if (ROOM_FLAG_WATER(room->info->flags) || extraL->dozy)
                    break;

                if ((getWaterDepth() != WALL) && abs(waterDist) < 256) {
                    waterState = WATER_STATE_SURFACE;
                    pos.y -= (waterDist - 1);
                    animSet(ANIM_SURF, true);
                    vSpeed = 0;
                    extraL->swimTimer = LARA_SWIM_TIMER + 1; // block dive before we press jump button again
                    updateRoom(-LARA_HEIGHT / 2);
                    //game->playSound(TR::SND_BREATH, pos, Sound::PAN | Sound::UNIQUE);
                } else {
                    waterState = WATER_STATE_ABOVE;
                    animSet(ANIM_FALL_FORTH, true);
                    hSpeed = vSpeed / 4;
                    vSpeed = 0;
                    flags |= ITEM_FLAG_GRAVITY;
                }

                angle.x = angle.z = 0;
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

                    extraL->swimTimer = 0;
                    vSpeed = 0;
                    flags &= ~ITEM_FLAG_GRAVITY;
                    angle.x = angle.z = 0;
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

        angle.z = angleDec(angle.z, ANGLE(1));
        turnSpeed = angleDec(turnSpeed, ANGLE(2));
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

        angle.z = angleDec(angle.z, ANGLE(2));

        int32 s, c;
        sincos(extraL->moveAngle, s, c);

        pos.x += (s * vSpeed) >> 16;
        pos.z += (c * vSpeed) >> 16;

        extraL->camera.targetAngle.x = ANGLE(-22);
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

        angle.z = angleDec(angle.z, ANGLE(2));
        turnSpeed = angleDec(turnSpeed, ANGLE(2));
        angle.y += turnSpeed;

        angle.x = X_CLAMP(angle.x, ANGLE(-85), ANGLE(85));
        angle.z = X_CLAMP(angle.z, ANGLE(-22), ANGLE(22));

        int32 sx, cx;
        int32 sy, cy;
        sincos(angle.x, sx, cx);
        sincos(angle.y, sy, cy);

        pos.y -= (sx * vSpeed) >> 16;
        pos.x += (cx * ((sy * vSpeed) >> 16)) >> FIXED_SHIFT;
        pos.z += (cx * ((cy * vSpeed) >> 16)) >> FIXED_SHIFT;
    }

    bool weaponFire(const ExtraInfoLara::Arm* arm)
    {
        int16 ammo = extraL->ammo[extraL->weapon];

        if (!ammo) {
            soundPlay(SND_EMPTY, &pos);
            extraL->goalWeapon = WEAPON_PISTOLS;
            return false;
        }

        if (ammo > 0) {
            ammo--;
        }

        const WeaponParams &params = weaponParams[extraL->weapon]; 

        Location from;
        from.pos.x = pos.x;
        from.pos.y = pos.y - params.height;
        from.pos.z = pos.z;
        from.room = room;

        int32 count = (extraL->weapon == WEAPON_SHOTGUN) ? 6 : 1;

        for (int32 i = 0; i < count; i++)
        {
            int32 aimX = int32(rand_logic() - 0x4000) * params.spread >> 16;
            int32 aimY = int32(rand_logic() - 0x4000) * params.spread >> 16;

            aimX += arm->angle.x;
            aimY += arm->angle.y;
            aimY += angle.y;

            matrixSetView(from.pos, aimX, aimY);

            int32 minDist = INT_MAX;

            if (arm->target && arm->target->health > 0)
            {
                Sphere* spheres = gSpheres[0];
                int32 spheresCount = arm->target->getSpheres(spheres, false);

                for (int32 i = 0; i < spheresCount; i++)
                {
                    const Sphere &s = spheres[i];

                    if (abs(s.center.x) >= s.radius)
                        continue;

                    if (abs(s.center.y) >= s.radius)
                        continue;

                    if (s.center.z <= s.radius)
                        continue;

                    if (fastLength(s.center.x, s.center.y) > s.radius)
                        continue;

                    int32 dist = s.center.z - s.radius;

                    if (dist < minDist) {
                        minDist = dist;
                    }
                }
            }

            vec3i dir = matrixGetDir(matrixGet());

            Location to = from;

            if (minDist != INT_MAX)
            {
                dir *= minDist;
                to.pos.x += dir.x >> FIXED_SHIFT;
                to.pos.y += dir.y >> FIXED_SHIFT;
                to.pos.z += dir.z >> FIXED_SHIFT;

                arm->target->hit(params.damage, to.pos, 0);
            } else {
                to.pos += dir;

                trace(from, to, true);
                fxRicochet(to.room, to.pos, true);
            }
        }

        soundPlay(params.soundId, &pos);

        return true;
    }

    void setWeaponState(WeaponState weaponState)
    {
        if (weaponState == extraL->weaponState)
            return;
        extraL->weaponState = weaponState;

        ExtraInfoLara::Arm &R = extraL->armR;
        ExtraInfoLara::Arm &L = extraL->armL;

        if (weaponState == WEAPON_STATE_DRAW)
        {
            const WeaponParams &params = weaponParams[extraL->weapon];
            int32 anim = (extraL->weapon == WEAPON_SHOTGUN) ? ANIM_SHOTGUN_DRAW : ANIM_PISTOLS_PICK;
            R.animIndex = L.animIndex = level.models[params.animType].animIndex + anim;
            R.frameIndex = L.frameIndex = 0;
        }

        if (weaponState == WEAPON_STATE_HOLSTER)
        {
            R.target = L.target = NULL;
        }

        if (weaponState == WEAPON_STATE_FREE)
        {
            R.useBasis = L.useBasis = false;
            R.animIndex = L.animIndex = 0;
            R.frameIndex = L.frameIndex = 0;
        #ifdef __3DO__
            extraL->goalWeapon = extraL->weapon = (extraL->weapon + 1) % WEAPON_MAX;
        #endif
        }
    }

    void weaponAim(ExtraInfoLara::Arm &arm)
    {
        if (arm.aim) {
            arm.angle.x = angleLerp(arm.angle.x, arm.angleAim.x, ANGLE(10));
            arm.angle.y = angleLerp(arm.angle.y, arm.angleAim.y, ANGLE(10));
        } else {
            arm.angle.x = angleLerp(arm.angle.x, 0, ANGLE(10));
            arm.angle.y = angleLerp(arm.angle.y, 0, ANGLE(10));
        }
    }

    void weaponDrawPistols()
    {
        const WeaponParams &params = weaponParams[extraL->weapon];

        ExtraInfoLara::Arm* arm = &extraL->armR;

        const Anim* animPtr = level.anims + arm->animIndex;
        int32 animLength = animPtr->frameEnd - animPtr->frameBegin;
        int32 frame = arm->frameIndex + 1;
        int32 anim = arm->animIndex - level.models[params.animType].animIndex;

        if (frame > animLength)
        {
            anim++;

            if (anim == ANIM_PISTOLS_DRAW) {
                meshSwapPistols(JOINT_MASK_ARM_R3 | JOINT_MASK_ARM_L3, JOINT_MASK_LEG_R1 | JOINT_MASK_LEG_L1);
                soundPlay(SND_DRAW, &pos);
            } else if (anim == ANIM_PISTOLS_FIRE) {
                anim = ANIM_PISTOLS_AIM;
                setWeaponState(WEAPON_STATE_READY);
            }

            frame = 0;
        }

        extraL->armR.angle = extraL->armL.angle = _vec3s(0, 0, 0);
        extraL->armR.animIndex = extraL->armL.animIndex = anim + level.models[params.animType].animIndex;
        extraL->armR.frameIndex = extraL->armL.frameIndex = frame;
    }

    void weaponHolsterPistols()
    {
        const WeaponParams &params = weaponParams[extraL->weapon];

        for (int32 i = 0; i < LARA_ARM_MAX; i++)
        {
            ExtraInfoLara::Arm* arm = &extraL->armR + i;

            if (!arm->animIndex)
                continue;

            int32 frame = arm->frameIndex;
            int32 anim = arm->animIndex - level.models[params.animType].animIndex;

            if (frame)
            {
                if (anim == ANIM_PISTOLS_AIM) {
                    arm->angle.x -= arm->angle.x / frame; // @DIV
                    arm->angle.y -= arm->angle.y / frame; // @DIV
                }

                if (anim == ANIM_PISTOLS_FIRE) {
                    frame = 0;
                } else {
                    frame--;
                }
            } else {
                if (anim == ANIM_PISTOLS_AIM) {
                    anim = ANIM_PISTOLS_DRAW;
                } else if (anim == ANIM_PISTOLS_PICK) {
                    arm->animIndex = 0;
                    continue;
                } else if (anim == ANIM_PISTOLS_DRAW) {
                    anim = ANIM_PISTOLS_PICK;
                    if (i == LARA_ARM_R) {
                        meshSwapPistols(JOINT_MASK_LEG_R1, JOINT_MASK_ARM_R3);
                    } else {
                        meshSwapPistols(JOINT_MASK_LEG_L1, JOINT_MASK_ARM_L3);
                    }
                    soundPlay(SND_HOLSTER, &pos);
                } else if (anim == ANIM_PISTOLS_FIRE) {
                    anim = ANIM_PISTOLS_AIM;
                }

                arm->animIndex = anim + level.models[params.animType].animIndex;
                frame = level.anims[arm->animIndex].frameEnd - level.anims[arm->animIndex].frameBegin;
            }

            arm->frameIndex = frame;
        }

        if (!extraL->armR.animIndex && !extraL->armL.animIndex) {
            setWeaponState(WEAPON_STATE_FREE);
        }
    }

    void weaponDrawShotgun()
    {
        const WeaponParams &params = weaponParams[extraL->weapon];

        ExtraInfoLara::Arm &arm = extraL->armR;

        const Anim* animPtr = level.anims + arm.animIndex;
        int32 animLength = animPtr->frameEnd - animPtr->frameBegin;
        int32 frame = arm.frameIndex + 1;
        int32 anim = arm.animIndex - level.models[params.animType].animIndex;

        ASSERT(anim == ANIM_SHOTGUN_DRAW);

        if (frame == 10) {
            meshSwapShotgun(true);
            soundPlay(SND_DRAW, &pos);
        }

        if (frame == animLength) {
            setWeaponState(WEAPON_STATE_READY);
        }

        extraL->armR.angle = extraL->armL.angle = _vec3s(0, 0, 0);
        extraL->armR.animIndex = extraL->armL.animIndex = anim + level.models[params.animType].animIndex;
        extraL->armR.frameIndex = extraL->armL.frameIndex = frame;
    }

    void weaponHolsterShotgun()
    {
        const WeaponParams &params = weaponParams[extraL->weapon];

        ExtraInfoLara::Arm &arm = extraL->armR;

        int32 frame = arm.frameIndex;
        int32 anim = arm.animIndex - level.models[params.animType].animIndex;

        if (anim == ANIM_SHOTGUN_AIM) {
            if (frame == 0) {
                anim = ANIM_SHOTGUN_DRAW;
                const Anim* animPtr = level.anims + level.models[params.animType].animIndex + anim;
                frame = animPtr->frameEnd - animPtr->frameBegin;
            } else {
                frame--;
            }
        } else if (anim == ANIM_SHOTGUN_FIRE) {
            frame++;
            if (frame > 12) {
                anim = ANIM_SHOTGUN_DRAW;
                const Anim* animPtr = level.anims + level.models[params.animType].animIndex + anim;
                frame = animPtr->frameEnd - animPtr->frameBegin;
            }
        } else if (anim == ANIM_SHOTGUN_DRAW) {
            if (frame == 0) {
                setWeaponState(WEAPON_STATE_FREE);
                return;
            } else {
                if (frame == 10) {
                    meshSwapShotgun(false);
                    soundPlay(SND_HOLSTER, &pos);
                }
                frame--;
            }
        }

        extraL->armR.angle = extraL->armL.angle = _vec3s(0, 0, 0);
        extraL->armR.animIndex = extraL->armL.animIndex = anim + level.models[params.animType].animIndex;
        extraL->armR.frameIndex = extraL->armL.frameIndex = frame;
    }

    void weaponDraw()
    {
        switch (extraL->weapon)
        {
            case WEAPON_PISTOLS:
            case WEAPON_MAGNUMS:
            case WEAPON_UZIS:
                weaponDrawPistols();
                break;
            case WEAPON_SHOTGUN:
                weaponDrawShotgun();
                break;
            default: ASSERT(false);
        }
    }

    void weaponHolster()
    {
        meshSwap(ITEM_LARA, JOINT_MASK_HEAD);

        switch (extraL->weapon)
        {
            case WEAPON_PISTOLS:
            case WEAPON_MAGNUMS:
            case WEAPON_UZIS:
                weaponHolsterPistols();
                break;
            case WEAPON_SHOTGUN:
                weaponHolsterShotgun();
                break;
            default: ASSERT(false);
        }
    }

    void weaponUpdatePistols()
    {
        ExtraInfoLara::Arm &R = extraL->armR;
        ExtraInfoLara::Arm &L = extraL->armL;

        weaponAim(R);
        weaponAim(L);

        const WeaponParams &params = weaponParams[extraL->weapon];

        for (int32 i = 0; i < LARA_ARM_MAX; i++)
        {
            ExtraInfoLara::Arm* arm = &extraL->armR + i;

            const Anim* animPtr = level.anims + arm->animIndex;
            int32 animLength = animPtr->frameEnd - animPtr->frameBegin;
            int32 frame = arm->frameIndex;
            int32 anim = arm->animIndex - level.models[params.animType].animIndex;

            if (((input & IN_ACTION) && !arm->target) || arm->aim)
            {
                if (anim == ANIM_PISTOLS_AIM)
                {
                    if (frame == animLength)
                    {
                        if ((input & IN_ACTION) && weaponFire(arm))
                        {
                            anim = ANIM_PISTOLS_FIRE;
                            frame = 0;

                            arm->flash.timer = params.flashTimer;
                            arm->flash.angle = int16(rand_draw() << 1);
                            arm->flash.offset = params.flashOffset;
                            arm->flash.intensity = params.flashIntensity << 8;
                        }
                    } else {
                        frame++;
                    }
                } else { // ANIM_DUAL_FIRE
                    frame++;
                    if (frame == params.reloadTimer)
                    {
                        anim = ANIM_PISTOLS_AIM;
                        const Anim* animPtr = level.anims + anim + level.models[params.animType].animIndex;
                        frame = animPtr->frameEnd - animPtr->frameBegin;
                    }
                }
            } else {
                if (anim == ANIM_PISTOLS_FIRE)
                {
                    anim = ANIM_PISTOLS_AIM;
                    const Anim* animPtr = level.anims + anim + level.models[params.animType].animIndex;
                    frame = animPtr->frameEnd - animPtr->frameBegin;
                } else if (frame) {
                    frame--;
                };
            }

            arm->animIndex = anim + level.models[params.animType].animIndex;
            arm->frameIndex = frame;
            arm->useBasis = (anim == ANIM_PISTOLS_AIM && frame) || (anim == ANIM_PISTOLS_FIRE);
        }
    }

    void weaponUpdateShotgun()
    {
        ExtraInfoLara::Arm &R = extraL->armR;
        ExtraInfoLara::Arm &L = extraL->armL;

        weaponAim(R);

        const WeaponParams &params = weaponParams[extraL->weapon];

        ExtraInfoLara::Arm* arm = &extraL->armR;

        const Anim* animPtr = level.anims + arm->animIndex;
        int32 animLength = animPtr->frameEnd - animPtr->frameBegin;
        int32 frame = arm->frameIndex;
        int32 anim = arm->animIndex - level.models[params.animType].animIndex;

        bool aim = ((input & IN_ACTION) && !arm->target) || arm->aim;

        switch (anim)
        {
            case ANIM_SHOTGUN_FIRE:
            {
                frame++;
                if (frame == 10) {
                    soundPlay(SND_SHOTGUN_RELOAD, &pos);
                } else if (frame == params.reloadTimer) {
                    anim = ANIM_SHOTGUN_AIM;
                    animPtr = level.anims + level.models[params.animType].animIndex + anim;
                    frame = animPtr->frameEnd - animPtr->frameBegin;
                } else if ((animLength - frame < 10) && !aim) {
                    anim = ANIM_SHOTGUN_AIM;
                    frame = animLength - frame; // how many frames left for fire animation
                    animPtr = level.anims + level.models[params.animType].animIndex + anim;
                    frame = animPtr->frameEnd - animPtr->frameBegin - frame; // offset aim frames from the end
                }
                break;
            }
            case ANIM_SHOTGUN_DRAW:
            {
                if (aim)
                {
                    anim = ANIM_SHOTGUN_AIM;
                    frame = 1;
                }
                break;
            }
            case ANIM_SHOTGUN_AIM:
            {
                if (aim)
                {
                    if (frame == animLength)
                    {
                        if ((input & IN_ACTION) && weaponFire(arm))
                        {
                            frame = 1;
                            anim = ANIM_SHOTGUN_FIRE;
                        }
                    } else {
                        frame++;
                    }
                } else {
                    if (frame == 0) {
                        anim = ANIM_SHOTGUN_DRAW;
                        animPtr = level.anims + level.models[params.animType].animIndex + anim;
                        animLength = animPtr->frameEnd - animPtr->frameBegin;
                        frame = animLength;
                    } else {
                        frame--;
                    }
                }
                break;
            }
        }

        R.useBasis = L.useBasis = false;
        R.animIndex = L.animIndex = anim + level.models[params.animType].animIndex;
        R.frameIndex = L.frameIndex = frame;
    }

    void weaponUpdateState()
    {
        bool change = false;
        if (waterState == WATER_STATE_ABOVE || waterState == WATER_STATE_WADE)
        {
            if (extraL->weapon != extraL->goalWeapon)
            {
                if (extraL->weaponState == WEAPON_STATE_FREE) {
                    extraL->weapon = extraL->goalWeapon;
                    change = true;
                } else if (extraL->weaponState == WEAPON_STATE_READY) {
                    change = true;
                }
            } else if (input & IN_WEAPON) {
                change = true;
            }
        } else if (extraL->weaponState == WEAPON_STATE_READY) {
            change = true;
        }

        if (!change)
            return;

        if (extraL->weaponState == WEAPON_STATE_FREE)
        {
            if (extraL->ammo[WEAPON_PISTOLS] != 0)
            {
                setWeaponState(WEAPON_STATE_DRAW);
            }
        }
            
        if (extraL->weaponState == WEAPON_STATE_READY)
        {
            setWeaponState(WEAPON_STATE_HOLSTER);
        }
    }

    void weaponGetAimPoint(ItemObj* target, Location &point)
    {
        const AABBs &box = target->getBoundingBox(false);
        vec3i p;
        p.x = (box.minX + box.maxX) >> 1;
        p.y = box.minY + (box.maxY - box.minY) / 3; // @DIV
        p.z = (box.minZ + box.maxZ) >> 1;
        int32 s, c;
        sincos(target->angle.y, s, c);
        X_ROTXY(p.x, p.z, -s, c);

        point.pos = target->pos + p;
        point.room = target->room;
    }

    void weaponTrackTargets()
    {
        ExtraInfoLara::Arm &arm = extraL->armR;

        if (arm.target && arm.target->health <= 0)
        {
            arm.target = NULL;
        }

        if (!arm.target)
        {
            extraL->armR.aim = extraL->armL.aim = false;
            return;
        }

        const WeaponParams &params = weaponParams[extraL->weapon];

        Location from;
        from.pos.x = pos.x;
        from.pos.y = pos.y - params.height;
        from.pos.z = pos.z;
        from.room = room;

        Location to;
        weaponGetAimPoint(arm.target, to);

        vec3i dir = to.pos - from.pos;
        vec3s angleAim;

        anglesFromVector(dir.x, dir.y, dir.z, angleAim.x, angleAim.y);

        angleAim.x -= angle.x;
        angleAim.y -= angle.y;

        if (trace(from, to, false))
        {
            if (abs(angleAim.x) <= params.aimX && abs(angleAim.y) <= params.aimY) {
                extraL->armR.aim = extraL->armL.aim = true;
            } else {
                extraL->armR.aim = extraL->armR.aim && (abs(angleAim.x) <= params.armX) && (angleAim.y >=  params.armMinY) && (angleAim.y <=  params.armMaxY);
                extraL->armL.aim = extraL->armL.aim && (abs(angleAim.x) <= params.armX) && (angleAim.y >= -params.armMaxY) && (angleAim.y <= -params.armMinY);
            }
        } else {
            extraL->armR.aim = extraL->armL.aim = false;
        }

        extraL->armR.angleAim = extraL->armL.angleAim = angleAim;
    }

    void weaponFindTargets()
    {
        if (!ItemObj::sFirstActive)
            return;

        const WeaponParams &params = weaponParams[extraL->weapon];
        int32 range = params.range;
        int32 rangeQ = X_SQR(range);
        int32 minAimY = params.aimY;

        Location from;
        from.pos.x = pos.x;
        from.pos.y = pos.y - params.height;
        from.pos.z = pos.z;
        from.room = room;

        ItemObj* item = ItemObj::sFirstActive;
        do
        {
            if (item->health <= 0)
                continue;

            if ((item->flags & ITEM_FLAG_STATUS) != ITEM_FLAG_STATUS_ACTIVE)
                continue;

            vec3i d = item->pos - pos;
            int32 distQ = X_SQR(d.x) + X_SQR(d.y) + X_SQR(d.z);

            if (distQ > rangeQ)
                continue;

            Location to;
            weaponGetAimPoint(item, to);

            if (!trace(from, to, false))
                continue;

            vec3i dir = to.pos - from.pos;
            vec3s angleAim;

            anglesFromVector(dir.x, dir.y, dir.z, angleAim.x, angleAim.y);

            angleAim.x -= angle.x + extraL->torso.angle.x;
            angleAim.y -= angle.y + extraL->torso.angle.y;

            angleAim.x = abs(angleAim.x);
            angleAim.y = abs(angleAim.y);

            if (angleAim.x > params.aimX || angleAim.y > params.aimY || angleAim.y > minAimY)
                continue;

            minAimY = angleAim.y;
            extraL->armR.target = item;
        } while ((item = item->nextActive) != NULL);
    }

    void weaponUpdateTargets()
    {
        if (input & IN_ACTION) {
            meshSwap(ITEM_LARA_UZIS, JOINT_MASK_HEAD);
        } else {
            meshSwap(ITEM_LARA, JOINT_MASK_HEAD);
            extraL->armR.target = NULL;
        }

        if (extraL->armR.target == NULL) {
            weaponFindTargets();
        }

        weaponTrackTargets();

        extraL->armL.target = extraL->armR.target;
    }

    void updateWeapon()
    {
        if (extraL->armR.flash.timer) {
            extraL->armR.flash.timer--;
        }

        if (extraL->armL.flash.timer) {
            extraL->armL.flash.timer--;
        }

        if (extraL->weapon == WEAPON_MAX)
            return;

        if (health <= 0)
        {
            extraL->armR.animIndex = extraL->armL.animIndex = 0;
            extraL->armR.useBasis = extraL->armL.useBasis = false;
            return;
        }

        weaponUpdateState();

        switch (extraL->weaponState)
        {
            case WEAPON_STATE_DRAW:
            {
                extraL->camera.toCombat();
                weaponDraw();
                break;
            }

            case WEAPON_STATE_HOLSTER:
            {
                weaponHolster();
                break;
            }

            case WEAPON_STATE_READY:
            {
                extraL->camera.toCombat();
                weaponUpdateTargets();

                if (extraL->weapon < WEAPON_SHOTGUN) {
                    weaponUpdatePistols();
                } else {
                    weaponUpdateShotgun();
                }
            }

            default: ;
        }
    }

    void changeWeapon(Weapon weapon)
    {
        extraL->goalWeapon = weapon;
        if ((extraL->weaponState == WEAPON_STATE_FREE) && (extraL->goalWeapon == extraL->weapon))
        {
            extraL->weapon = WEAPON_NONE;
        }
    }

    bool useItem(InvSlot slot)
    {
        switch (slot)
        {
            case SLOT_PISTOLS:
                changeWeapon(WEAPON_PISTOLS);
                break;
            case SLOT_SHOTGUN:
                changeWeapon(WEAPON_SHOTGUN);
                break;
            case SLOT_MAGNUMS:
                changeWeapon(WEAPON_MAGNUMS);
                break;
            case SLOT_UZIS:
                changeWeapon(WEAPON_UZIS);
                break;
            case SLOT_MEDIKIT_BIG:
            case SLOT_MEDIKIT_SMALL:
                if (health < LARA_MAX_HEALTH)
                {
                    health += (slot == SLOT_MEDIKIT_BIG) ? LARA_MAX_HEALTH : (LARA_MAX_HEALTH >> 1);
                    if (health > LARA_MAX_HEALTH) {
                        health = LARA_MAX_HEALTH;
                    }
                    inventory.remove(slot, 1);
                    extraL->healthTimer = 40;
                    soundPlay(SND_HEALTH, &pos);
                }
                break;
            default: return false;
        }
        return true;
    }

    virtual void hit(int32 damage, const vec3i &point, int32 soundId)
    {
        if (health <= 0 || damage <= 0)
            return;

        osJoyVibrate(0, 0xFF, 0xFF);
        extraL->healthTimer = 40;
        health = X_MAX(0, health - damage);
    }

    virtual void update()
    {
        vec3i oldPos = pos;

        updateInput();

        if ((input & (IN_JUMP | IN_WEAPON)) == (IN_JUMP | IN_WEAPON))
        {
            restore();
        }

        if (isKeyHit(IN_SELECT) && (gBrightness == 0))
        {
            inventory.open(this, (health > 0) ? INV_PAGE_MAIN : INV_PAGE_DEATH);
        }

        updateLook();

        updateWaterState();

        if (health > 0)
        {
            if (waterState == WATER_STATE_UNDER)
            {
                if (oxygen > 0) {
                    oxygen--;
                } else {
                    hit(5, pos, 0);
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

        const Sector* sector = room->getSector(pos.x, pos.z);
        bool badPos = (sector->floor == NO_FLOOR);

        //if (!badPos) {
        //    int32 h = pos.y - roomFloor;
        //    badPos = (h > cinfo.gapPos) || (h < cinfo.gapNeg);
        //}

        if (badPos)
        {
            pos = oldPos;
            updateRoom(offset);
        }

        updateWeapon();

        checkTrigger(cinfo.trigger, this);

        extraL->camera.update();

        if (health > 0 && extraL->healthTimer > 0) {
            extraL->healthTimer--;
        }
    }

    void meshSwapPistols(uint32 weaponMask, uint32 bodyMask)
    {
        const WeaponParams &params = weaponParams[extraL->weapon];

        meshSwap(ITEM_LARA, bodyMask);
        meshSwap(params.modelType, weaponMask);
    }

    void meshSwapShotgun(bool armed)
    {
        const WeaponParams &params = weaponParams[WEAPON_SHOTGUN];

        if (armed) {
            meshSwap(ITEM_LARA, JOINT_MASK_TORSO);
            meshSwap(params.modelType, JOINT_MASK_ARM_R3 | JOINT_MASK_ARM_L3);
        } else {
            meshSwap(ITEM_LARA, JOINT_MASK_ARM_R3 | JOINT_MASK_ARM_L3);
            meshSwap(params.modelType, JOINT_MASK_TORSO);
        }
    }

    virtual void draw()
    {
        int32 tmpAnimIndex = animIndex;
        int32 tmpFrameIndex = frameIndex;

        if (extraL->hitQuadrant != -1)
        {
            switch (extraL->hitQuadrant)
            {
                case 0 : animIndex = ANIM_HIT_FRONT; break;
                case 1 : animIndex = ANIM_HIT_LEFT; break;
                case 2 : animIndex = ANIM_HIT_BACK; break;
                case 3 : animIndex = ANIM_HIT_RIGHT; break;
                default : ASSERT(false);
            }
            frameIndex = level.anims[animIndex].frameBegin + extraL->hitFrame;
        }

        drawModel(this);

        animIndex = tmpAnimIndex;
        frameIndex = tmpFrameIndex;
    }

    struct LaraSave {
        int16 vSpeed;
        int16 hSpeed;
        int16 health; // oxygen already saved as alias of ItemObj::timer

        uint8 weaponState;
        uint8 weapon;
        uint8 goalWeapon;
        uint8 waterState;

        struct Arm {
            uint16 animIndex;
            uint16 frameIndex;
        };

        Arm armR;
        Arm armL;

        uint8 cameraRoom;
        uint8 cameraLastIndex;

        int16 cameraViewX;
        int16 cameraViewY;
        int16 cameraViewZ;

        uint16 meshes[JOINT_MAX];
    };

    virtual uint8* save(uint8* data)
    {
        data = ItemObj::save(data);

        LaraSave* sg = (LaraSave*)data;

        sg->vSpeed = vSpeed;
        sg->hSpeed = hSpeed;
        sg->health = health;
        sg->weaponState = extraL->weaponState;
        sg->weapon = extraL->weapon;
        sg->goalWeapon = extraL->goalWeapon;
        sg->waterState = waterState;

        sg->armR.animIndex  = extraL->armR.animIndex;
        sg->armR.frameIndex = extraL->armR.frameIndex;
        sg->armL.animIndex  = extraL->armL.animIndex;
        sg->armL.frameIndex = extraL->armL.frameIndex;

        const Room* camRoom = extraL->camera.view.room;
        sg->cameraRoom = camRoom - rooms;
        sg->cameraLastIndex = extraL->camera.lastIndex;
        sg->cameraViewX = extraL->camera.view.pos.x - (camRoom->info->x << 8);
        sg->cameraViewY = extraL->camera.view.pos.y - (camRoom->info->yTop);
        sg->cameraViewZ = extraL->camera.view.pos.z - (camRoom->info->z << 8);

        ASSERT(sizeof(sg->meshes) == sizeof(extraL->meshes));
        memcpy(sg->meshes, extraL->meshes, sizeof(extraL->meshes));

        return data + sizeof(LaraSave);
    }

    virtual uint8* load(uint8* data)
    {
        data = ItemObj::load(data);

        LaraSave* sg = (LaraSave*)data;

        vSpeed = sg->vSpeed;
        hSpeed = sg->hSpeed;
        health = sg->health;
        extraL->weaponState = sg->weaponState;
        extraL->weapon = sg->weapon;
        extraL->goalWeapon = sg->goalWeapon;
        waterState = sg->waterState;

        extraL->armR.animIndex  = sg->armR.animIndex;
        extraL->armR.frameIndex = sg->armR.frameIndex;
        extraL->armL.animIndex  = sg->armL.animIndex;
        extraL->armL.frameIndex = sg->armL.frameIndex;

        extraL->camera.init(this);

        Room* camRoom = rooms + sg->cameraRoom;
        extraL->camera.view.room = camRoom;
        extraL->camera.lastIndex = sg->cameraLastIndex;
        extraL->camera.view.pos.x = sg->cameraViewX + (camRoom->info->x << 8);
        extraL->camera.view.pos.y = sg->cameraViewY + (camRoom->info->yTop);
        extraL->camera.view.pos.z = sg->cameraViewZ + (camRoom->info->z << 8);

        ASSERT(sizeof(sg->meshes) == sizeof(extraL->meshes));
        memcpy(extraL->meshes, sg->meshes, sizeof(extraL->meshes));

        return data + sizeof(LaraSave);
    }
};

const Lara::Handler Lara::sHandlers[X_MAX] = { LARA_STATES(DECL_S_HANDLER) };
const Lara::Handler Lara::cHandlers[X_MAX] = { LARA_STATES(DECL_C_HANDLER) };

#undef DECL_ENUM
#undef DECL_S_HANDLER
#undef DECL_C_HANDLER
#undef S_HANDLER
#undef C_HANDLER

int32 doTutorial(ItemObj* lara, int32 track)
{
    if (!lara)
        return track;

    switch (track)
    {
        case 28 :
            if ((gSaveGame.tracks[track] & TRACK_FLAG_ONCE) && lara->state == Lara::STATE_JUMP_UP) {
                track = 29;
            }
            break;

        case 37 : 
        case 41 :
            if (lara->state != Lara::STATE_HANG) {
                track = 0;
            }
            break;

        case 42 :
            if ((gSaveGame.tracks[track] & TRACK_FLAG_ONCE) && lara->state == Lara::STATE_HANG) {
                track = 43;
            }
            break;

        case 49 :
            if (lara->state != Lara::STATE_SURF_TREAD) {
                track = 0;
            }
            break;

        case 50 : // end of GYM
            if (gSaveGame.tracks[track] & TRACK_FLAG_ONCE) {
                lara->gymTimer++;
                if (lara->gymTimer > 90)
                {
                    nextLevel(LVL_TR1_TITLE);
                }
            } else {
                if (lara->state != Lara::STATE_WATER_OUT)
                    track = 0;
                lara->gymTimer = 0;
            }
            break;
    }

    return track;
}

Lara* getLara(const vec3i &pos)
{
    return players[0]; // TODO find nearest player
}

#endif
