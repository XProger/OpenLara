#ifndef H_LARA
#define H_LARA

#include "common.h"
#include "item.h"
#include "collision.h"
#include "camera.h"

#define LARA_STATES(E) \
    E( WALK             ) \
    E( RUN              ) \
    E( STOP             ) \
    E( FORWARD_JUMP     ) \
    E( POSE             ) \
    E( FAST_BACK        ) \
    E( TURN_RIGHT       ) \
    E( TURN_LEFT        ) \
    E( DEATH            ) \
    E( FALL             ) \
    E( HANG             ) \
    E( REACH            ) \
    E( SPLAT            ) \
    E( TREAD            ) \
    E( LAND             ) \
    E( COMPRESS         ) \
    E( BACK             ) \
    E( SWIM             ) \
    E( GLIDE            ) \
    E( HANG_UP          ) \
    E( FAST_TURN        ) \
    E( STEP_RIGHT       ) \
    E( STEP_LEFT        ) \
    E( ROLL_END         ) \
    E( SLIDE            ) \
    E( BACK_JUMP        ) \
    E( RIGHT_JUMP       ) \
    E( LEFT_JUMP        ) \
    E( UP_JUMP          ) \
    E( FALL_BACK        ) \
    E( HANG_LEFT        ) \
    E( HANG_RIGHT       ) \
    E( SLIDE_BACK       ) \
    E( SURF_TREAD       ) \
    E( SURF_SWIM        ) \
    E( DIVE             ) \
    E( PUSH_BLOCK       ) \
    E( PULL_BLOCK       ) \
    E( PUSH_PULL_READY  ) \
    E( PICK_UP          ) \
    E( SWITCH_DOWN      ) \
    E( SWITCH_UP        ) \
    E( USE_KEY          ) \
    E( USE_PUZZLE       ) \
    E( UW_DEATH         ) \
    E( ROLL_START       ) \
    E( SPECIAL          ) \
    E( SURF_BACK        ) \
    E( SURF_LEFT        ) \
    E( SURF_RIGHT       ) \
    E( MIDAS_USE        ) \
    E( MIDAS_DEATH      ) \
    E( SWAN_DIVE        ) \
    E( FAST_DIVE        ) \
    E( HANDSTAND        ) \
    E( WATER_OUT        )

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
#define LARA_MAX_OXYGEN         1800
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
        // TODO
    }

    void stopScreaming()
    {
        // TODO
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

    void alignWall()
    {
        int x = (~1023) & pos.x;
        int z = (~1023) & pos.z;

        switch (angleY)
        {
            case  ANGLE_0   : pos.z = z + 1024 + LARA_RADIUS; break;
            case  ANGLE_90  : pos.x = x + 1024 + LARA_RADIUS; break;
            case -ANGLE_90  : pos.x = x - LARA_RADIUS; break;
            case  ANGLE_180 : pos.z = z - LARA_RADIUS; break;
            default         : ASSERT(false);
        }
    }

    int16 getFront(int16 angle)
    {
        angle += angleY;

        int32 x = pos.x + (phd_sin(angle) >> (FIXED_SHIFT - 8));
        int32 y = pos.y - LARA_HEIGHT;
        int32 z = pos.z + (phd_cos(angle) >> (FIXED_SHIFT - 8));

        Room* roomFront = room->getRoom(x, y, z);
        const RoomInfo::Sector* sector = roomFront->getSector(x, z);
        int16 floor = sector->getFloor(x, y, z);

        if (floor != WALL) {
            floor -= pos.y;
        }

        return floor;
    }

    const Box& getBounds()
    {
        int32 modelIndex = modelsMap[type];
        ASSERT(modelIndex != NO_MODEL);

        const Model* model = models + modelIndex;

        AnimFrame* frame = getFrame(model);

        return frame->box;
    }

    bool checkDeath()
    {
        return health <= 0;
    }

// state control
    bool s_checkFront(int16 angle) 
    {
        UNUSED(angle); // TODO
        return true; // TODO
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
            angleZ = X_MAX(angleZ - tilt, -LARA_TILT_MAX);
        } else if (input & IN_RIGHT) {
            turnSpeed = X_MIN(turnSpeed + LARA_TURN_ACCEL, maxSpeed);
            angleZ = X_MIN(angleZ + tilt, LARA_TILT_MAX);
        }
    }

    bool s_checkFall()
    {
        if (vSpeed > 131)
        {
            if (state == SWAN_DIVE) {
                goalState = FAST_DIVE;
            } else {
                goalState = FALL;
            }
            return true;
        }
        return false;
    }

    void s_checkWalk(int32 stopState)
    {
        if ((input & IN_UP) && s_checkFront(ANGLE_0)) {
            if (input & IN_WALK) {
                goalState = WALK;
            } else {
                goalState = RUN;
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
            if ((state == RUN) || (state == STOP))
            {
                animSet(ANIM_STAND_ROLL_BEGIN, true, 2);
                goalState = STOP;
                return true;
            }
        }

        return false;
    }

    S_HANDLER( WALK )
    {
        if (checkDeath()) {
            goalState = STOP;
            return;
        }

        s_rotate(LARA_TURN_SLOW, 0);

        s_checkWalk(STOP);
    }

    S_HANDLER( RUN )
    {
        if (checkDeath()) {
            goalState = DEATH;
            return;
        }

        if (s_checkRoll()) {
            return;
        }

        s_rotate(LARA_TURN_FAST, 1);

        if ((input & IN_JUMP) && !flags.gravity) {
            goalState = FORWARD_JUMP;
        } else {
            s_checkWalk(STOP);
        }
    }

    S_HANDLER( STOP )
    {
        if (checkDeath())
        {
            nextState = DEATH;
            return;
        }

        if (s_checkRoll()) {
            return;
        }

        goalState = STOP;
    /*
        if ((input & (IN_UP | IN_ACTION)) == (IN_UP | IN_ACTION)) {
            c_angle(ANGLE_0);
            cinfo.radius = LARA_RADIUS + 4;
            c_default();
            cinfo.radius = LARA_RADIUS;
            if (c_checkClimbUp()) {
                return;
            }
        }
    */
        if (input & IN_WALK) {
            if ((input & IN_LEFT) && s_checkFront(-ANGLE_90)) {
                goalState = STEP_LEFT;
            } else if ((input & IN_RIGHT) && s_checkFront(ANGLE_90)) {
                goalState = STEP_RIGHT;
            }
        } else {
            if (input & IN_LEFT) {
                goalState = TURN_LEFT;
            } else if (input & IN_RIGHT) {
                goalState = TURN_RIGHT;
            }
        }

        if (input & IN_JUMP) {
            goalState = COMPRESS;
        } else if ((input & IN_UP) && s_checkFront(ANGLE_0)) {
            if (input & IN_WALK) {
                s_WALK();
            } else {
                s_RUN();
            }
        } else if ((input & IN_DOWN) && s_checkFront(ANGLE_180)) {
            if (input & IN_WALK) {
                s_BACK();
            } else {
                goalState = FAST_BACK;
            }
        }
    }


    S_HANDLER( FORWARD_JUMP )
    {
        if (goalState == SWAN_DIVE ||
            goalState == REACH)
        {
            goalState = FORWARD_JUMP;
        }

        if (goalState != DEATH &&
            goalState != STOP &&
            goalState != RUN)
        {
            if (input & IN_ACTION)
            {
                goalState = REACH;
            }
        
            if (input & IN_WALK)
            {
                goalState = SWAN_DIVE;
            }
        
            s_checkRoll();
            s_checkFall();
        }

        s_rotate(LARA_TURN_JUMP, 0);
    }

    S_HANDLER( POSE )
    {
        // empty
    }

    S_HANDLER( FAST_BACK )
    {
        s_rotate(LARA_TURN_MED, 0);
        goalState = STOP;
    }

    S_HANDLER( TURN_RIGHT )
    {
        if (checkDeath() || (input & IN_LOOK))
        {
            goalState = STOP;
            return;
        }

        turnSpeed += LARA_TURN_ACCEL;

        if (turnSpeed > LARA_TURN_SLOW)
        {
            goalState = FAST_TURN;
        }

        s_checkWalk((input & IN_RIGHT) ? goalState : STOP);
    }

    S_HANDLER( TURN_LEFT )
    {
        if (checkDeath() || (input & IN_LOOK))
        {
            goalState = STOP;
            return;
        }

        turnSpeed -= LARA_TURN_ACCEL;

        if (turnSpeed < -LARA_TURN_SLOW)
        {
            goalState = FAST_TURN;
        }

        s_checkWalk((input & IN_LEFT) ? goalState : STOP);
    }

    S_HANDLER( DEATH )
    {
        s_ignoreEnemy();
    }

    S_HANDLER( FALL )
    {
        hSpeed = (hSpeed * 95) / 100;
        if (vSpeed >= 154)
        {
            startScreaming();
        }
    }

    S_HANDLER( HANG )
    {
        camera.targetAngleX = -60 * DEG2SHORT;

        s_ignoreEnemy();
        if (input & IN_LEFT) {
            goalState = HANG_LEFT;
        } else if (input & IN_RIGHT) {
            goalState = HANG_RIGHT;
        }
    }

    S_HANDLER( REACH )
    {
        camera.targetAngleY = 85 * DEG2SHORT;

        s_checkFall();
    }

    S_HANDLER( SPLAT )
    {
        // empty
    }

    S_HANDLER( TREAD ) {} // TODO

    S_HANDLER( LAND )
    {
        // empty
    }

    S_HANDLER( COMPRESS )
    {
        if ((input & IN_UP) && getFront(ANGLE_0) >= -LARA_STEP_HEIGHT) {
            goalState = FORWARD_JUMP;
        } else if ((input & IN_LEFT) && getFront(-ANGLE_90) >= -LARA_STEP_HEIGHT) {
            goalState = LEFT_JUMP;
        } else if ((input & IN_RIGHT) && getFront(ANGLE_90) >= -LARA_STEP_HEIGHT) {
            goalState = RIGHT_JUMP;
        } else if ((input & IN_DOWN) && getFront(ANGLE_180) >= -LARA_STEP_HEIGHT) {
            goalState = BACK_JUMP;
        }
        s_checkFall();
    }

    S_HANDLER( BACK )
    {
        if (checkDeath()) {
            goalState = STOP;
            return;
        }

        if ((input & (IN_WALK | IN_DOWN)) != (IN_WALK | IN_DOWN)) {
            goalState = STOP;
        } else {
            goalState = BACK;
        }

        s_rotate(LARA_TURN_SLOW, 0);
    }

    S_HANDLER( SWIM ) {} // TODO
    S_HANDLER( GLIDE ) {} // TODO

    S_HANDLER( HANG_UP )
    {
        s_ignoreEnemy();
    }

    S_HANDLER( FAST_TURN )
    {
        if (checkDeath() || (input & IN_LOOK))
        {
            goalState = STOP;
            return;
        }

        if (turnSpeed < 0) {
            turnSpeed = -LARA_TURN_FAST;
            if (!(input & IN_LEFT)) {
                goalState = STOP;
            }
        } else {
            turnSpeed = LARA_TURN_FAST;
            if (!(input & IN_RIGHT)) {
                goalState = STOP;
            }
        }
    }

    S_HANDLER( STEP_RIGHT )
    {
        if (checkDeath() || (input & (IN_WALK | IN_RIGHT)) != (IN_WALK | IN_RIGHT))
        {
            goalState = STOP;
        }
    }

    S_HANDLER( STEP_LEFT )
    {
        if (checkDeath() || (input & (IN_WALK | IN_LEFT)) != (IN_WALK | IN_LEFT))
        {
            goalState = STOP;
        }
    }

    S_HANDLER( ROLL_END )
    {
        // empty
    }

    S_HANDLER( SLIDE )
    {
        camera.targetAngleX = -45 * DEG2SHORT;

        if (input & IN_JUMP)
        {
            goalState = FORWARD_JUMP;
        }
    }

    S_HANDLER( BACK_JUMP )
    {
        camera.targetAngleY = 135 * DEG2SHORT;

        if (s_checkFall()) return;

        if (goalState == RUN) {
            goalState = STOP;
        }
    }

    S_HANDLER( RIGHT_JUMP )
    {
        s_checkFall();
    }

    S_HANDLER( LEFT_JUMP )
    {
        s_checkFall();
    }

    S_HANDLER( UP_JUMP )
    {
        s_checkFall();
    }

    S_HANDLER( FALL_BACK )
    {
        s_checkFall();
    
        if (input & IN_ACTION) 
        {
            goalState = REACH;
        }
    }

    S_HANDLER( HANG_LEFT )
    {
        camera.targetAngleX = -60 * DEG2SHORT;

        s_ignoreEnemy();

        if (!(input & IN_LEFT))
        {
            goalState = HANG;
        }
    }

    S_HANDLER( HANG_RIGHT )
    {
        camera.targetAngleX = -60 * DEG2SHORT;

        s_ignoreEnemy();

        if (!(input & IN_RIGHT))
        {
            goalState = HANG;
        }
    }

    S_HANDLER( SLIDE_BACK )
    {
        if (input & IN_JUMP)
        {
            goalState = BACK_JUMP;
        }
    }

    S_HANDLER( SURF_TREAD ) {} // TODO
    S_HANDLER( SURF_SWIM ) {} // TODO
    S_HANDLER( DIVE ) {} // TODO

    S_HANDLER( PUSH_BLOCK )
    {
        camera.targetAngleX = -25 * DEG2SHORT;
        camera.targetAngleY = 35 * DEG2SHORT;

        s_ignoreEnemy();
    }

    S_HANDLER( PULL_BLOCK )
    {
        camera.targetAngleX = -25 * DEG2SHORT;
        camera.targetAngleY = 35 * DEG2SHORT;

        s_ignoreEnemy();
    }

    S_HANDLER( PUSH_PULL_READY )
    {
        camera.targetAngleY = 75 * DEG2SHORT;

        s_ignoreEnemy();

        if (!(input & IN_ACTION))
        {
            goalState = STOP;
        }
    }

    S_HANDLER( PICK_UP )
    {
        camera.targetAngleX = -15 * DEG2SHORT;
        camera.targetAngleY = -130 * DEG2SHORT;
        camera.targetDist = 1024;

        s_ignoreEnemy();
    }

    S_HANDLER( SWITCH_DOWN )
    {
        camera.targetAngleX = -25 * DEG2SHORT;
        camera.targetAngleY = 80 * DEG2SHORT;
        camera.targetDist = 1024;

        s_ignoreEnemy();
    }

    S_HANDLER( SWITCH_UP )
    {
        camera.targetAngleX = -25 * DEG2SHORT;
        camera.targetAngleY = 80 * DEG2SHORT;
        camera.targetDist = 1024;

        s_ignoreEnemy();
    }

    S_HANDLER( USE_KEY )
    {
        camera.targetAngleX = -25 * DEG2SHORT;
        camera.targetAngleY = -80 * DEG2SHORT;
        camera.targetDist = 1024;

        s_ignoreEnemy();
    }

    S_HANDLER( USE_PUZZLE )
    {
        camera.targetAngleX = -25 * DEG2SHORT;
        camera.targetAngleY = -80 * DEG2SHORT;
        camera.targetDist = 1024;

        s_ignoreEnemy();
    }

    S_HANDLER( UW_DEATH )
    {
        vSpeed = X_MAX(vSpeed - LARA_SWIM_ACCEL, 0);
        angleX = angleDec(angleX, 2 * DEG2SHORT);
    }

    S_HANDLER( ROLL_START )
    {
        // empty
    }

    S_HANDLER( SPECIAL )
    {
        camera.targetAngleX = -25 * DEG2SHORT;
        camera.targetAngleY = 170 * DEG2SHORT;
    }

    S_HANDLER( SURF_BACK ) {} // TODO
    S_HANDLER( SURF_LEFT ) {} // TODO
    S_HANDLER( SURF_RIGHT ) {} // TODO

    S_HANDLER( MIDAS_USE )
    {
        s_ignoreEnemy();
    }

    S_HANDLER( MIDAS_DEATH )
    {
        s_ignoreEnemy();
        flags.gravity = false;
    }

    S_HANDLER( SWAN_DIVE ) {} // TODO
    S_HANDLER( FAST_DIVE ) {} // TODO

    S_HANDLER( HANDSTAND )
    {
        s_ignoreEnemy();
    }

    S_HANDLER( WATER_OUT )
    {
        s_ignoreEnemy();
    }

// collision control
    void c_applyOffset() {
        pos += cinfo.offset;
        cinfo.offset = vec3i(0, 0, 0);
    }

    void c_angle(int32 angleDelta) {
        moveAngle = angleY + angleDelta;

        cinfo.angle    = moveAngle;
        cinfo.quadrant = uint16(cinfo.angle + ANGLE_45) / ANGLE_90;
    }

    bool c_checkCeiling() {
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

    bool c_checkWall() {
        if (cinfo.type == CT_FRONT || cinfo.type == CT_FRONT_CEILING) {
            c_applyOffset();
            goalState = STOP;
            hSpeed = 0;
            flags.gravity = false;
            return true;
        }

        if (cinfo.type == CT_LEFT) {
            c_applyOffset();
            angleY += 5 * DEG2SHORT;
            angleZ = angleDec(angleZ, 2 * DEG2SHORT);
        } else if (cinfo.type == CT_RIGHT) {
            c_applyOffset();
            angleY -= 5 * DEG2SHORT;
            angleZ = angleDec(angleZ, 2 * DEG2SHORT);
        }

        return false;
    }

    bool c_checkWallUW() {
        if (cinfo.type == CT_FRONT) {
            if (angleX > 35 * DEG2SHORT) {
                angleX += 2 * DEG2SHORT;
            } else if (angleX < -35 * DEG2SHORT) {
                angleX -= 2 * DEG2SHORT;
            } else {
                vSpeed = 0;
            }
        } else if (cinfo.type == CT_CEILING) {
            if (angleX >= -45 * DEG2SHORT) {
                angleX -= 2 * DEG2SHORT;
            }
        } else if (cinfo.type == CT_FRONT_CEILING) {
            vSpeed = 0;
        } else if (cinfo.type == CT_LEFT) {
            angleY += 5 * DEG2SHORT;
        } else if (cinfo.type == CT_RIGHT) {
            angleY -= 5 * DEG2SHORT;
        } else if (cinfo.type == CT_FLOOR_CEILING) {
            pos = cinfo.pos;
            vSpeed = 0;
            return true;
        }

        if (cinfo.m.floor < 0) {
            pos.y += cinfo.m.floor;
            angleX += 2 * DEG2SHORT;
        }

        int32 waterDepth = room->getWaterDepth();

        if (waterDepth == WALL) {
            vSpeed = 0;
            pos = cinfo.pos;
        } else if (waterDepth <= 512) {
            waterState = WATER_STATE_WADE;

            animSet(ANIM_SWIM_STAND, true);
            goalState = STOP;

            angleX = 0;
            angleZ = 0;
            hSpeed = 0;
            vSpeed = 0;
            flags.gravity = false;
        }

        return false;
    }

    bool c_checkWallSurf() {
        if ((cinfo.m.floor < 0 && cinfo.m.slantType == SLANT_HIGH) || (cinfo.type & (CT_FRONT | CT_CEILING | CT_FRONT_CEILING | CT_FLOOR_CEILING))) {
            pos  = cinfo.pos;
            vSpeed = 0;
        } else if (cinfo.type == CT_LEFT) {
            angleY += 5 * DEG2SHORT;
        } else if (cinfo.type == CT_RIGHT) {
            angleY -= 5 * DEG2SHORT;
        }

        return true;
    }

    bool c_checkSlide() {
        if (waterState == WATER_STATE_WADE) {
            return false;
        }

        if (cinfo.m.slantType != SLANT_HIGH) {
            return false;
        }

        c_applyOffset();

        int16 angle;

        if (cinfo.slantX > 2) {
            angle = -ANGLE_90;
        } else if (cinfo.slantX < -2) {
            angle =  ANGLE_90;
        } else if (cinfo.slantZ > 2) {
            angle = ANGLE_180;
        } else {
            angle = 0;
        }

        if (abs(angle - angleY) <= ANGLE_90) {
            if (state != SLIDE) {
                animSet(ANIM_SLIDE_FORTH, true);
            }
            moveAngle = angle;
            angleY = angle;
        } else {
            if (state != SLIDE_BACK) {
                animSet(ANIM_SLIDE_BACK, true);
            }
            moveAngle = angle;
            angleY = angle + ANGLE_180;
        }

        return true;
    }

    bool c_checkFall(int32 height, int32 fallAnimIndex = ANIM_FALL_FORTH) {
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

    bool c_checkLanding() {
        if ((state == FAST_DIVE /*|| state == AIR_ROLL*/) && vSpeed > 133) {
            health = 0;
            return true;
        }

        int32 y = pos.y;
        pos.y += cinfo.m.floor;
        /* TODO
        v2floor = pos.y;

        checkTrigger(cinfo.trigger, this);
        */
        pos.y = y;

        if (vSpeed <= 140) {
            return false;
        }

        if (vSpeed > 154) {
            health = 0;
        } else {
            health -= (X_SQR(vSpeed - 140) * LARA_MAX_HEALTH) / 196;
        }

        return checkDeath();
    }

    bool c_checkSwing() {
        int32 x = pos.x;
        int32 y = pos.y;
        int32 z = pos.z;

        switch (angleY) {
            case  ANGLE_0   : z += 256; break;
            case  ANGLE_90  : x += 256; break;
            case -ANGLE_90  : x -= 256; break;
            case  ANGLE_180 : z -= 256; break;
        }

        Room* roomBelow = room->getRoom(x, y, z);
        const RoomInfo::Sector* sector = roomBelow->getSector(x, z);
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

    bool c_checkGrab() {
        return !(input & IN_ACTION) || cinfo.type != CT_FRONT || abs(cinfo.r.floor - cinfo.l.floor) >= LARA_HANG_SLANT;
    }

    bool c_checkSpace() {
        return (cinfo.f.floor < cinfo.f.ceiling ||
                cinfo.l.floor < cinfo.l.ceiling ||
                cinfo.r.floor < cinfo.r.ceiling);
    }

    bool c_checkClimbStart() {
        return false;
    }

    bool c_checkClimbUp() {
        if (c_checkGrab()) {
            return false;
        }

        int16 angle = angleY;
        if (alignAngle(angle, 30 * DEG2SHORT)) {
            return false;
        }

        if (cinfo.f.floor >= -640 && cinfo.f.floor <= -384) {
            if (c_checkSpace()) return false;

            animSet(ANIM_CLIMB_2, true);
            state = HANG_UP;

            pos.y += 512 + cinfo.f.floor;
        } else if (cinfo.f.floor >= -896 && cinfo.f.floor <= -640) {
            if (c_checkSpace()) return false;

            animSet(ANIM_CLIMB_3, true);
            state = HANG_UP;

            pos.y += 768 + cinfo.f.floor;
        } else if (cinfo.f.floor >= -1920 && cinfo.f.floor <= -896) {
            animSet(ANIM_STAND, true);
            goalState = UP_JUMP;
            vSpeedHack = -int32(phd_sqrt(-2 * GRAVITY * (cinfo.f.floor + 800)) + 3);

            updateAnim();
        } /* TODO for main branch
          else if ((waterState != WATER_STATE_WADE) && (cinfo.f.floor <= -1920) && (cinfo.l.floor <= -1920) && (cinfo.r.floor <= -1920) && (cinfo.m.ceiling <= -1158)) {
            animSet(ANIM_STAND, true);
            goalState = UP_JUMP;
            vSpeedHack = -116;
            animUpdate(this);
        }
          else if (((cinfo.f.floor < -1024) && (cinfo.f.ceiling >= 506)) || (cinfo.m.ceiling <= -518) && c_checkClimbStart()) {
            animSet(ANIM_STAND, true);
            goalState = CLIMB_START;
            processAnimation();
        }*/ else {
            return false;
        }

        angleY = angle;
        c_applyOffset();

        return true;
    }

    bool c_checkHang() {
        if (c_checkGrab()) {
            return false;
        }

        if (cinfo.f.ceiling > 0 || cinfo.m.ceiling > -LARA_STEP_HEIGHT || (cinfo.m.floor < 200 && state == REACH)) {
            return false;
        }

        int32 h = cinfo.f.floor - getBounds().minY;
        int32 v = h + vSpeed;

        if ((h < 0 && v < 0) || (h > 0 && v > 0)) {
            return false;
        }

        if (alignAngle(angleY, 35 * DEG2SHORT)) {
            return false;
        }

        if (state == REACH)
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

    bool c_checkDrop() {
        // TODO getTrigger here

        if ((health > 0) && (input & IN_ACTION)) {
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

    bool c_checkWaterOut() {
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

        if (alignAngle(angleY, 35 * DEG2SHORT)) {
            return false;
        }

        angleY += h - 5;

        updateRoom(-LARA_HEIGHT / 2);

        alignWall();
        
        /* TODO for main branch
        if ((h < -128) || (level->version & TR::VER_TR1)) {
            setAnimV2(ANIM_WATER_OUT, true);
            specular = LARA_WET_SPECULAR;
        } else if (h < 128) {
            setAnimV2(ANIM_SURF_OUT, true);
        } else {
            setAnimV2(ANIM_SURF_STAND, true);
        }
        game->waterDrop(pos, 128.0f, 0.2f);
        */
        animSet(ANIM_WATER_OUT, true);

        waterState = WATER_STATE_ABOVE;
        goalState = STOP;
        angleX = 0;
        angleZ = 0;
        hSpeed = 0;
        vSpeed = 0;
        flags.gravity = false;

        return true;
    }

    void c_default() {
        cinfo.gapPos      = LARA_STEP_HEIGHT;
        cinfo.gapNeg      = -LARA_STEP_HEIGHT;
        cinfo.gapCeiling  = 0;
        cinfo.stopOnSlant = true;

        collideRoom(this, LARA_HEIGHT, 0);
    }

    void c_step() {
        cinfo.gapPos      = (waterState == WATER_STATE_WADE) ? -WALL : 128;
        cinfo.gapNeg      = -128;
        cinfo.gapCeiling  = 0;
        cinfo.stopOnSlant = true;

        collideRoom(this, LARA_HEIGHT, 0);

        if (c_checkCeiling()) return;

        if (c_checkWall()) {
            animSet(ANIM_STAND, true);
        }

        if (c_checkSlide()) return;

        pos.y += cinfo.m.floor;
    }

    void c_fall() {
        if (vSpeed <= 0 || cinfo.m.floor > 0)
            return;

        if (c_checkLanding()) {
            goalState = DEATH;
        } else if (state == FORWARD_JUMP && (input & IN_UP) && !(input & IN_WALK)) {
            goalState = RUN;
        } else if (state == FALL) {
            animSet(ANIM_LANDING, true);
        } else {
            goalState = STOP;
        }

        stopScreaming();

        pos.y += cinfo.m.floor;
        vSpeed = 0;
        flags.gravity = false;

        if (state == FORWARD_JUMP) {
            updateAnim();
        }
    }

    void c_jump() {
        cinfo.gapPos = -WALL;
        cinfo.gapNeg = (state == REACH) ? 0 : -LARA_STEP_HEIGHT;
        cinfo.gapCeiling = 192;

        collideRoom(this, state == UP_JUMP ? LARA_HEIGHT_JUMP : LARA_HEIGHT, 0);

        if ((state == REACH || state == UP_JUMP) && c_checkHang()) {
            return;
        }

        c_applyOffset();

        bool slide = (state == FALL) || (state == REACH) || (state == UP_JUMP);

        if ((cinfo.type == CT_CEILING) || ((cinfo.type == CT_FRONT_CEILING) && slide)) {
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
            pos.x -= (LARA_RADIUS * phd_sin(cinfo.angle)) >> FIXED_SHIFT;
            pos.z -= (LARA_RADIUS * phd_cos(cinfo.angle)) >> FIXED_SHIFT;
            cinfo.m.floor = 0;
            hSpeed = 0;
            if (vSpeed <= 0) {
                vSpeed = 16;
            }
        } else if (cinfo.type == CT_LEFT) {
            angleY += 5 * DEG2SHORT;
        } else if (cinfo.type == CT_RIGHT) {
            angleY -= 5 * DEG2SHORT;
        }

        c_fall();
    }

    void c_slide() {
        cinfo.gapPos      = -WALL;
        cinfo.gapNeg      = -512;
        cinfo.gapCeiling  = 0;
        cinfo.stopOnSlant = true;

        collideRoom(this, LARA_HEIGHT, 0);

        if (c_checkCeiling()) return;

        c_checkWall();

        if (c_checkFall(200, state == SLIDE ? ANIM_FALL_FORTH : ANIM_FALL_BACK)) return;

        c_checkSlide();

        pos.y += cinfo.m.floor;

        if (cinfo.m.slantType != SLANT_HIGH) {
            goalState = STOP;
        }
    }

    void c_roll() {
        vSpeed = 0;
        flags.gravity = false;

        cinfo.gapPos      = -WALL;
        cinfo.gapNeg      = -LARA_STEP_HEIGHT;
        cinfo.gapCeiling  = 0;
        cinfo.stopOnSlant = true;

        collideRoom(this, LARA_HEIGHT, 0);

        if (c_checkCeiling()) return;

        if (c_checkSlide()) return;

        if (c_checkFall(200, state == ROLL_START ? ANIM_FALL_FORTH : ANIM_FALL_BACK)) return;

        c_applyOffset();

        pos.y += cinfo.m.floor;
    }

    void c_hang(int32 angleDelta) {
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

        switch (cinfo.quadrant) {
            case 0 : pos.z += 2; break;
            case 1 : pos.x += 2; break;
            case 2 : pos.z -= 2; break;
            case 3 : pos.x -= 2; break;
        }

        collideRoom(this, LARA_HEIGHT, 0);

        moveAngle = angleY + angleDelta;

        if (health <= 0 || !(input & IN_ACTION)) {
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
            if (state != HANG) {
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

    C_HANDLER( WALK )
    {
        vSpeed = 0;
        flags.gravity = false;
        cinfo.stopOnLava = true;

        c_angle(ANGLE_0);
        c_default();

        if (c_checkCeiling()) return;
        
        if (c_checkClimbUp()) return;

        if (c_checkWall()) {
            if (frameIndex >= 29 && frameIndex <= 47) {
                animSet(ANIM_STAND_RIGHT, false);
            } else if ((frameIndex >= 22 && frameIndex <= 28) || (frameIndex >= 48 && frameIndex <= 57)) {
                animSet(ANIM_STAND_LEFT, false);
            } else {
                animSet(ANIM_STAND, false);
            }
        }

        if (c_checkFall(LARA_STEP_HEIGHT)) return;

        // descend
        if (cinfo.m.floor > 128) {
            if (frameIndex >= 28 && frameIndex <= 45) {
                animSet(ANIM_WALK_DESCEND_RIGHT, false);
            } else {
                animSet(ANIM_WALK_DESCEND_LEFT, false);
            }
        }

        // ascend
        if (cinfo.m.floor >= -LARA_STEP_HEIGHT && cinfo.m.floor < -128) {
            if (frameIndex >= 27 && frameIndex <= 44) {
                animSet(ANIM_WALK_ASCEND_RIGHT, false);
            } else {
                animSet(ANIM_WALK_ASCEND_LEFT, false);
            }
        }

        if (c_checkSlide()) return;

        pos.y += cinfo.m.floor;
    }

    C_HANDLER( RUN )
    {
        c_angle(ANGLE_0);

        cinfo.gapPos      = -WALL;
        cinfo.gapNeg      = -LARA_STEP_HEIGHT;
        cinfo.gapCeiling  = 0;
        cinfo.stopOnSlant = true;

        collideRoom(this, LARA_HEIGHT, 0);

        if (c_checkCeiling()) return;

        if (c_checkClimbUp()) return;

        if (c_checkWall()) {
            angleZ = 0;

            if (cinfo.f.slantType == SLANT_NONE && cinfo.f.floor < -LARA_SMASH_HEIGHT && frameIndex < 22) {
                animSet(frameIndex < 10 ? ANIM_SMASH_RUN_LEFT : ANIM_SMASH_RUN_RIGHT, false);
                state = SPLAT;
                return;
            }

            animSet(ANIM_STAND, true);
        }

        if (c_checkFall(LARA_STEP_HEIGHT)) return;

        // ascend
        if (cinfo.m.floor >= -LARA_STEP_HEIGHT && cinfo.m.floor < -128) {
            if (frameIndex >= 3 && frameIndex <= 14) {
                animSet(ANIM_RUN_ASCEND_RIGHT, false);
            } else {
                animSet(ANIM_RUN_ASCEND_LEFT, false);
            }
        }

        if (c_checkSlide()) return;

        if (cinfo.m.floor >= 50) {
            pos.y += 50;
            return;
        }

        pos.y += cinfo.m.floor;
    }

    C_HANDLER( STOP )
    {
        vSpeed = 0;
        flags.gravity = false;

        c_angle(ANGLE_0);
        c_default();

        if (c_checkCeiling()) return;

        if (c_checkFall(100)) return;

        if (c_checkSlide()) return;

        c_applyOffset();

        pos.y += cinfo.m.floor;
    }

    C_HANDLER( FORWARD_JUMP )
    {
        c_angle(ANGLE_0);
        c_jump();
    }

    C_HANDLER( POSE )
    {
        c_STOP();
    }

    C_HANDLER( FAST_BACK )
    {
        vSpeed = 0;
        flags.gravity = false;

        c_angle(ANGLE_180);

        cinfo.gapPos      = -WALL;
        cinfo.gapNeg      = -LARA_STEP_HEIGHT;
        cinfo.gapCeiling  = 0;
        cinfo.stopOnSlant = true;

        collideRoom(this, LARA_HEIGHT, 0);

        if (c_checkCeiling()) return;

        if (c_checkFall(200, ANIM_FALL_BACK)) return;

        if (c_checkWall()) {
            animSet(ANIM_STAND, false);
        }

        pos.y += cinfo.m.floor;
    }

    C_HANDLER( TURN_RIGHT )
    {
        c_angle(ANGLE_0);
        c_default();

        if (c_checkFall(100)) return;

        if (c_checkSlide()) return;

        pos.y += cinfo.m.floor;
    }

    C_HANDLER( TURN_LEFT )
    {
        c_TURN_RIGHT();
    }

    C_HANDLER( DEATH )
    {
        cinfo.radius = LARA_RADIUS * 4;
        
        c_angle(ANGLE_0);
        c_default();

        c_applyOffset();
        pos.y += cinfo.m.floor;
    }

    C_HANDLER( FALL )
    {
        flags.gravity = true;
        c_jump();
    }

    C_HANDLER( HANG )
    {
        c_hang(0);

        if ((input & IN_UP) && goalState == HANG)
        {
            if (cinfo.f.floor <= -850 ||
                cinfo.f.floor >= -650 ||
                c_checkSpace() || cinfo.staticHit) return;

            if (input & IN_WALK) {
                goalState = HANDSTAND;
            } else {
                goalState = HANG_UP;
            }
        }
    }

    C_HANDLER( REACH )
    {
        flags.gravity = true;
        c_angle(ANGLE_0);
        c_jump();
    }

    C_HANDLER( SPLAT )
    {
        c_angle(ANGLE_0);
        c_default();
        c_applyOffset();
    }

    C_HANDLER( TREAD ) {} // TODO

    C_HANDLER( LAND )
    {
        c_STOP();
    }

    C_HANDLER( COMPRESS )
    {
        vSpeed  = 0;
        flags.gravity = false;

        cinfo.gapPos      = -WALL;
        cinfo.gapNeg      = WALL;
        cinfo.gapCeiling  = 0;

        collideRoom(this, LARA_HEIGHT, 0);

        if (cinfo.m.ceiling > -100) {
            animSet(ANIM_STAND, true);
            pos = cinfo.pos;
            hSpeed = 0;
            vSpeed = 0;
            flags.gravity = false;
        }
    }

    C_HANDLER( BACK )
    {
        vSpeed  = 0;
        flags.gravity = false;

        c_angle(ANGLE_180);

        cinfo.gapPos      = (waterState == WATER_STATE_WADE) ? -WALL : LARA_STEP_HEIGHT;
        cinfo.gapNeg      = -LARA_STEP_HEIGHT;
        cinfo.gapCeiling  = 0;
        cinfo.stopOnSlant = true;

        collideRoom(this, LARA_HEIGHT, 0);

        if (c_checkCeiling()) return;

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

        if (c_checkSlide()) return;

        pos.y += cinfo.m.floor;
    }

    C_HANDLER( SWIM ) {} // TODO
    C_HANDLER( GLIDE ) {} // TODO

    C_HANDLER( HANG_UP )
    {
        c_angle(ANGLE_0);
        c_default();
    }

    C_HANDLER( FAST_TURN )
    {
        c_STOP();
    }

    C_HANDLER( STEP_RIGHT )
    {
        c_angle(+ANGLE_90);
        c_step();
    }

    C_HANDLER( STEP_LEFT )
    {
        c_angle(-ANGLE_90);
        c_step();
    }

    C_HANDLER( ROLL_END )
    {
        c_angle(ANGLE_180);
        c_roll();
    }

    C_HANDLER( SLIDE )
    {
        c_angle(ANGLE_0);
        c_slide();
    }

    C_HANDLER( BACK_JUMP )
    {
        c_angle(ANGLE_180);
        c_jump();
    }

    C_HANDLER( RIGHT_JUMP )
    {
        c_angle(ANGLE_90);
        c_jump();
    }

    C_HANDLER( LEFT_JUMP )
    {
        c_angle(-ANGLE_90);
        c_jump();
    }

    C_HANDLER( UP_JUMP )
    {
        c_angle(ANGLE_0);
        c_jump();
    }

    C_HANDLER( FALL_BACK )
    {
        c_angle(ANGLE_180);
        c_jump();
    }

    C_HANDLER( HANG_LEFT )
    {
        c_hang(-ANGLE_90);
    }

    C_HANDLER( HANG_RIGHT )
    {
        c_hang(ANGLE_90);
    }

    C_HANDLER( SLIDE_BACK )
    {
        c_angle(ANGLE_180);
        c_slide();
    }

    C_HANDLER( SURF_TREAD ) {} // TODO
    C_HANDLER( SURF_SWIM ) {} // TODO
    C_HANDLER( DIVE ) {} // TODO

    C_HANDLER( PUSH_BLOCK )
    {
        c_angle(ANGLE_0);
        c_default();
    }

    C_HANDLER( PULL_BLOCK )
    {
        c_angle(ANGLE_0);
        c_default();
    }

    C_HANDLER( PUSH_PULL_READY )
    {
        c_angle(ANGLE_0);
        c_default();
    }

    C_HANDLER( PICK_UP )
    {
        c_angle(ANGLE_0);
        c_default();
    }

    C_HANDLER( SWITCH_DOWN )
    {
        c_angle(ANGLE_0);
        c_default();
    }

    C_HANDLER( SWITCH_UP )
    {
        c_angle(ANGLE_0);
        c_default();
    }

    C_HANDLER( USE_KEY )
    {
        c_angle(ANGLE_0);
        c_default();
    }

    C_HANDLER( USE_PUZZLE )
    {
        c_angle(ANGLE_0);
        c_default();
    }

    C_HANDLER( UW_DEATH ) {} // TODO

    C_HANDLER( ROLL_START )
    {
        c_angle(ANGLE_0);
        c_roll();
    }

    C_HANDLER( SPECIAL )
    {
        // empty
    }

    C_HANDLER( SURF_BACK ) {} // TODO
    C_HANDLER( SURF_LEFT ) {} // TODO
    C_HANDLER( SURF_RIGHT ) {} // TODO

    C_HANDLER( MIDAS_USE )
    {
        c_angle(ANGLE_0);
        c_default();
    }

    C_HANDLER( MIDAS_DEATH )
    {
        c_angle(ANGLE_0);
        c_default();
    }

    C_HANDLER( SWAN_DIVE )
    {
        c_angle(ANGLE_0);
        c_jump();
    }

    C_HANDLER( FAST_DIVE )
    {
        c_angle(ANGLE_0);
        c_jump();
    }

    C_HANDLER( HANDSTAND )
    {
        c_angle(ANGLE_0);
        c_default();
    }

    C_HANDLER( WATER_OUT )
    {
        c_angle(ANGLE_0);
        c_default();
    }

    Lara(Room* room) : Item(room)
    {
        health = LARA_MAX_HEALTH;
        oxygen = LARA_MAX_OXYGEN;
        flags.shadow = true;

        activate();
    }

// update control
    void updateInput()
    {
        input = 0;
        if (camera.mode == CAMERA_MODE_FREE) return;

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

    virtual void update()
    {
        updateInput();

        cinfo.trigger     = NULL;
        cinfo.radius      = LARA_RADIUS;
        cinfo.pos         = pos;
        cinfo.enemyPush   = true;
        cinfo.enemyHit    = true;
        cinfo.stopOnSlant = false;
        cinfo.stopOnLava  = false;

        updateState();

        angleZ = angleDec(angleZ, 1 * DEG2SHORT);
        turnSpeed = angleDec(turnSpeed, 2 * DEG2SHORT);
        angleY += turnSpeed;

        updateAnim();

        updateCollision();

        updateRoom(-LARA_HEIGHT / 2);

        //updateWeapon();

        checkTrigger(cinfo.trigger, this);
    }
};

const Lara::Handler Lara::sHandlers[X_MAX] = { LARA_STATES(DECL_S_HANDLER) };
const Lara::Handler Lara::cHandlers[X_MAX] = { LARA_STATES(DECL_C_HANDLER) };

#undef DECL_ENUM
#undef DECL_S_HANDLER
#undef DECL_C_HANDLER
#undef S_HANDLER
#undef C_HANDLER

#endif
