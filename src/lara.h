#ifndef H_LARA
#define H_LARA

#include "controller.h"

#define FAST_TURN_TIME      1.0f

#define TURN_FAST           PI
#define TURN_FAST_BACK      PI * 3.0f / 4.0f
#define TURN_NORMAL         PI / 2.0f
#define TURN_SLOW           PI / 3.0f
#define TURN_TILT           PI / 18.0f
#define TURN_WATER_FAST     PI * 3.0f / 4.0f
#define TURN_WATER_SLOW     PI * 2.0f / 3.0f
#define GLIDE_SPEED         50.0f

struct Lara : Controller {

    // http://www.tombraiderforums.com/showthread.php?t=148859&highlight=Explanation+left
    enum LaraAnim : int32 {
        ANIM_STAND              = 11,
        ANIM_FALL               = 34,
        ANIM_SMASH_JUMP         = 32,
        ANIM_SMASH_RUN_LEFT     = 53,
        ANIM_SMASH_RUN_RIGHT    = 54,
        ANIM_WATER_FALL         = 112,
        ANIM_TO_ONWATER         = 114,
        ANIM_STAND_ROLL_BEGIN   = 146,
        ANIM_STAND_ROLL_END     = 147,        
        ANIM_TO_UNDERWATER      = 119,
        ANIM_HIT_FRONT          = 125,
        ANIM_HIT_BACK           = 126,
        ANIM_HIT_LEFT           = 127,
        ANIM_HIT_RIGHT          = 128,
    };

    // http://www.tombraiderforums.com/showthread.php?t=211681
    enum LaraState : int32 {
        STATE_WALK,
        STATE_RUN,
        STATE_STOP,
        STATE_FORWARD_JUMP,
        STATE_4,
        STATE_FAST_BACK,
        STATE_TURN_RIGHT,
        STATE_TURN_LEFT,
        STATE_DEATH,
        STATE_FALL,
        STATE_HANG,
        STATE_REACH,
        STATE_SPLAT,
        STATE_TREAD,
        STATE_FAST_TURN_14,
        STATE_COMPRESS,
        STATE_BACK,
        STATE_SWIM,
        STATE_GLIDE,
        STATE_NULL_19,
        STATE_FAST_TURN,
        STATE_STEP_RIGHT,
        STATE_STEP_LEFT,
        STATE_ROLL,
        STATE_SLIDE,
        STATE_BACK_JUMP,
        STATE_RIGHT_JUMP,
        STATE_LEFT_JUMP,
        STATE_UP_JUMP,
        STATE_FALL_BACK,
        STATE_HANG_LEFT,
        STATE_HANG_RIGHT,
        STATE_SLIDE_BACK,
        STATE_SURF_TREAD,
        STATE_SURF_SWIM,
        STATE_DIVE,
        STATE_PUSH_BLOCK,
        STATE_PULL_BLOCK,
        STATE_PUSH_PULL_READY,
        STATE_PICK_UP,
        STATE_SWITCH_ON,
        STATE_SWITCH_OFF,
        STATE_USE_KEY,
        STATE_USE_PUZZLE,
        STATE_UNDERWATER_DEATH,
        STATE_ROLL_45,
        STATE_SPECIAL,
        STATE_SURF_BACK,
        STATE_SURF_LEFT,
        STATE_SURF_RIGHT,
        STATE_NULL_50,
        STATE_NULL_51,
        STATE_SWAN_DIVE,
        STATE_FAST_DIVE,
        STATE_HANDSTAND,
        STATE_WATER_OUT,
        STATE_MAX };

    Lara(TR::Level *level, int entity) : Controller(level, entity) {
    /*
    // level 2 (pool)
        pos = vec3(70067, -256, 29104);
        angle = vec3(0.0f, -0.68f, 0.0f);
        getEntity().room = 15;        
    */
    /*
    // level 2 (wolf)
        pos = vec3(75671, -1024, 22862);
        angle = vec3(0.0f, -PI * 0.25f, 0.0f);
        getEntity().room = 13;
    */
    /*
    // level 3a
        pos = vec3(41015, 3584, 34494);
        angle = vec3(0.0f, -PI, 0.0f);
        getEntity().room = 51;
    */
    /*
    // level 1
        pos = vec3(20215, 6656, 52942);
        angle = vec3(0.0f, PI, 0.0f);
        getEntity().room = 14;
    */
    //    updateEntity();
    }

    bool isMovingState(int state) {
        return state == STATE_RUN || state == STATE_FAST_BACK || state == STATE_ROLL || state == STATE_WALK || state == STATE_STEP_LEFT || state == STATE_STEP_RIGHT;
    }

    bool waterOut(int &outState) {
        vec3 dst = pos + getDir() * 32.0f;

        FloorInfo infoCur = getFloorInfo((int)pos.x, (int)pos.z),
                  infoDst = getFloorInfo(infoCur.roomAbove, (int)dst.x, (int)dst.z);

        if (infoDst.roomBelow == 0xFF && pos.y - infoDst.floor <= 256) { // possibility check
            if (!setState(STATE_STOP)) { // can't set water out state
                outState = STATE_STOP;
                return true;
            }
            outState = state;

            turnToWall();
            dst.y   -= pos.y - infoDst.floor;
            pos     = dst;  // set new position

            getEntity().room = infoCur.roomAbove;
            updateEntity();
            return true;
        }

        return false;
    }

    virtual Stand getStand() {
        if (stand == STAND_ONWATER && state != STATE_DIVE && state != STATE_STOP)
            return stand;

        if (getRoom().flags & TR::ROOM_FLAG_WATER)
            return STAND_UNDERWATER; // TODO: ONWATER

        int extra = isMovingState(state) ? 256 : 0;

        TR::Entity &e = getEntity();
        FloorInfo info = getFloorInfo(e.x, e.z);
        if (info.roomBelow == 0xFF && e.y + extra >= info.floor)
            return STAND_GROUND;

        return STAND_AIR;
    }

    virtual int getHeight() {
        if (stand == STAND_GROUND || stand == STAND_AIR)
            return 768;
        return 0;
    }

    virtual int getStateAir() {
        angle.x = 0.0f;

        if (state == STATE_FORWARD_JUMP) {
            if (mask & ACTION) return STATE_REACH;
            if ((mask & (FORTH | WALK)) == (FORTH | WALK)) return STATE_SWAN_DIVE;
        } else 
            if (state != STATE_SWAN_DIVE && state != STATE_REACH && state != STATE_FALL && state != STATE_UP_JUMP && state != STATE_BACK_JUMP && state != STATE_LEFT_JUMP && state != STATE_RIGHT_JUMP && state != STATE_ROLL)
                return setAnimation(ANIM_FALL);

        return state;
    }

    virtual int getStateGround() {
        angle.x = 0.0f;
    /*
    // hit test
        if (animIndex != ANIM_HIT_FRONT)
            for (int i = 0; i < level->entitiesCount; i++) {
                TR::Entity &e = level->entities[i];
                if (e.id != ENTITY_ENEMY_WOLF) continue;
                vec3 v = vec3(e.x, e.y, e.z) - pos;
                if (v.length2() < 128 * 128) {
                    return setAnimation(ANIM_HIT_FRONT);
                }
            }
    */
        if ( (mask & (FORTH | BACK)) == (FORTH | BACK) && (state == STATE_STOP || state == STATE_RUN) )
            return setAnimation(ANIM_STAND_ROLL_BEGIN);

        // ready to jump
        if (state == STATE_COMPRESS) {
            switch (mask & (RIGHT | LEFT | FORTH | BACK)) {
                case RIGHT  : return STATE_RIGHT_JUMP;
                case LEFT   : return STATE_LEFT_JUMP;
                case FORTH  : return STATE_FORWARD_JUMP;
                case BACK   : return STATE_BACK_JUMP;
                default     : return STATE_UP_JUMP;
            }
        }

        // jump button is pressed
        if (mask & JUMP) {   
            if ((mask & FORTH) && state == STATE_FORWARD_JUMP)
                return STATE_RUN;
            return state == STATE_RUN ? STATE_FORWARD_JUMP : STATE_COMPRESS;
        }
        
        // walk button is pressed
        if (mask & WALK) {      
            if (mask & FORTH) return STATE_WALK;
            if (mask & BACK)  return STATE_BACK;
            if (mask & LEFT)  return STATE_STEP_LEFT;
            if (mask & RIGHT) return STATE_STEP_RIGHT;
            return STATE_STOP;
         } 
        
         // only dpad buttons pressed
         if (mask & FORTH) return STATE_RUN;
         if (mask & BACK)  return STATE_FAST_BACK;
         if (mask & LEFT)  return turnTime < FAST_TURN_TIME ? STATE_TURN_LEFT  : STATE_FAST_TURN;
         if (mask & RIGHT) return turnTime < FAST_TURN_TIME ? STATE_TURN_RIGHT : STATE_FAST_TURN;         
         return STATE_STOP;
    }

    virtual int getStateUnderwater() {
        if (state == STATE_FORWARD_JUMP || state == STATE_UP_JUMP || state == STATE_BACK_JUMP || state == STATE_LEFT_JUMP || state == STATE_RIGHT_JUMP || state == STATE_FALL || state == STATE_REACH)
            return setAnimation(ANIM_WATER_FALL);
        
        if (state == STATE_SWAN_DIVE) {
            angle.x = -PI * 0.5f;
            return STATE_DIVE;
        }
            
        if (mask & JUMP) return STATE_SWIM;
        return (state == STATE_SWIM || velocity.y > GLIDE_SPEED) ? STATE_GLIDE : STATE_TREAD;
    }

    virtual int getStateOnwater() {
        angle.x = 0.0f;  

        if (state == STATE_WATER_OUT) return state;

        if (state != STATE_SURF_TREAD && state != STATE_SURF_LEFT && state != STATE_SURF_RIGHT && state != STATE_SURF_SWIM && state != STATE_SURF_BACK && state != STATE_STOP)
            return setAnimation(ANIM_TO_ONWATER);

        if (mask & FORTH) {
            if (mask & JUMP) { 
                angle.x = -PI * 0.25f;
                return setAnimation(ANIM_TO_UNDERWATER);
            }

            if ((mask & ACTION) && waterOut(state)) return state;

            return STATE_SURF_SWIM;
        }
        
        if (mask & BACK)  return STATE_SURF_BACK;
        if (mask & WALK) {
            if (mask & LEFT)  return STATE_SURF_LEFT;
            if (mask & RIGHT) return STATE_SURF_RIGHT;         
        }
        return STATE_SURF_TREAD;
    }

    virtual int getStateDeath() {
        return STATE_DEATH;
    }

    virtual int getStateDefault() {
        if (state == STATE_DIVE)        return state;
        if (stand == STAND_ONWATER)     return STATE_SURF_TREAD;
        if (stand == STAND_UNDERWATER)  return STATE_TREAD;
        if (stand == STAND_GROUND)      return STATE_STOP;
        return STATE_FALL;
    }

    virtual int getInputMask() {
        mask = 0;
        if (Input::down[ikW] || Input::joy.L.y < 0)                             mask |= FORTH;
        if (Input::down[ikS] || Input::joy.L.y > 0)                             mask |= BACK;
        if (Input::down[ikA] || Input::joy.L.x < 0)                             mask |= LEFT;
        if (Input::down[ikD] || Input::joy.L.x > 0)                             mask |= RIGHT;
        if (Input::down[ikSpace] || Input::down[ikJoyX])                        mask |= JUMP;
        if (Input::down[ikShift] || Input::down[ikJoyLT])                       mask |= WALK;
        if (Input::down[ikE] || Input::down[ikMouseL] || Input::down[ikJoyA])   mask |= ACTION;
        if (Input::down[ikQ] || Input::down[ikMouseR] || Input::down[ikJoyY])   mask |= WEAPON;
        if (health <= 0)                                                        mask  = DEATH;
        return mask;
    }

    virtual void updateState() {
        TR::Animation *anim  = &level->anims[animIndex];

        int fCount = anim->frameEnd - anim->frameStart;
        int fIndex = int(animTime * 30.0f);

        float rot = 0.0f;

#ifdef _DEBUG
        // show state transitions for current animation
        static bool lState = false;
        if (Input::down[ikEnter]) {
            if (!lState) {
                lState = true;
                static int snd_id = 0;//160;
                playSound(snd_id);
            //    setAnimation(snd_id);
                LOG("sound: %d\n", snd_id++);

                LOG("state: %d\n", anim->state);
                for (int i = 0; i < anim->scCount; i++) {
                    auto &sc = level->states[anim->scOffset + i];
                    LOG("-> %d : ", (int)sc.state);
                    for (int j = 0; j < sc.rangesCount; j++) {
                        TR::AnimRange &range = level->ranges[sc.rangesOffset + j];
                        LOG("%d ", range.nextAnimation);
                    }
                    LOG("\n");
                }

            }
            
        } else 
            lState = false;
#endif

    // calculate turn tilt
        if (state == STATE_RUN && (mask & (LEFT | RIGHT))) {
            if (mask & LEFT)  angle.z -= Core::deltaTime * TURN_TILT;
            if (mask & RIGHT) angle.z += Core::deltaTime * TURN_TILT;
            angle.z = clamp(angle.z, -TURN_TILT, TURN_TILT);
        } else
            angle.z -= angle.z * min(Core::deltaTime * 8.0f, 1.0f);

        if (state == STATE_TURN_LEFT || state == STATE_TURN_RIGHT || state == STATE_FAST_TURN)
            turnTime += Core::deltaTime;
        else
            turnTime = 0.0f;

    // get turning angle
        float w = 0.0f;
       
        if (state == STATE_SWIM || state == STATE_GLIDE)
            w = TURN_WATER_FAST;
        else if (state == STATE_TREAD || state == STATE_SURF_TREAD || state == STATE_SURF_SWIM || state == STATE_SURF_BACK)
            w = TURN_WATER_SLOW;
        else if (state == STATE_RUN || state == STATE_FAST_TURN)
            w = TURN_FAST;
        else if (state == STATE_FAST_BACK)
            w = TURN_FAST_BACK;
        else if (state == STATE_TURN_LEFT || state == STATE_TURN_RIGHT || state == STATE_WALK)
            w = TURN_NORMAL;   
        else if (state == STATE_FORWARD_JUMP || state == STATE_BACK)
            w = TURN_SLOW;

        if (w != 0.0f) {
            w *= Core::deltaTime;
        // yaw
            if (mask & LEFT)  { angle.y -= w; velocity = velocity.rotateY(+w); }
            if (mask & RIGHT) { angle.y += w; velocity = velocity.rotateY(-w); }
        // pitch (underwater only)
            if (stand == STAND_UNDERWATER && (mask & (FORTH | BACK)) ) {
                angle.x += ((mask & FORTH) ? -w : w) * 0.5f;
                angle.x = clamp(angle.x, -PI * 0.5f, PI * 0.5f);
            }
        }

    // get animation direction
        angleExt = angle.y;
        switch (state) {
            case STATE_BACK :
            case STATE_SURF_BACK  :
            case STATE_BACK_JUMP  :
            case STATE_FAST_BACK  :
                angleExt += PI;
                break;
            case STATE_LEFT_JUMP  :
            case STATE_STEP_LEFT  :
            case STATE_SURF_LEFT  :
                angleExt -= PI * 0.5f;
                break;
            case STATE_RIGHT_JUMP :
            case STATE_STEP_RIGHT :
            case STATE_SURF_RIGHT :
                angleExt +=  PI * 0.5f;
                break;
        }  
    }

    virtual void updateVelocity() {
    // calculate moving speed
        float dt = Core::deltaTime * 30.0f;
        
        TR::Animation *anim = &level->anims[animIndex];

        //if (anim->speed != 0.0f || anim->accel != 0.0f)
        //    LOG("speed: %f accel: %f\n", (float)anim->speed, (float)anim->accel);

        switch (stand) {
            case STAND_AIR :
                velocity.y += GRAVITY * dt;
                break;
            case STAND_GROUND :
            case STAND_ONWATER : {

                float speed = 0.0f;
                switch (state) {
                    case STATE_SURF_SWIM  :
                    case STATE_SURF_BACK  :
                    case STATE_SURF_LEFT  :
                    case STATE_SURF_RIGHT :
                        speed = 15.0f;
                        break;
                    default :
                        speed = anim->speed + anim->accel * (animTime * 30.0f);
                        if (animIndex == ANIM_STAND_ROLL_END) 
                            speed = -speed;
                }

                velocity.x = sinf(angleExt) * speed;
                velocity.z = cosf(angleExt) * speed;
                if (stand == STAND_GROUND)
                    velocity.y += GRAVITY * dt; 
                else
                    velocity.y = 0.0f;
                break;
            }
            case STAND_UNDERWATER : {
                float speed = 0.0f;
                if (animIndex == ANIM_TO_UNDERWATER)
                    speed = 15.0f;
                if (state == STATE_SWIM)
                    speed = 35.0f;

                if (speed != 0.0f)
                    velocity = vec3(angle.x, angle.y) * speed;
                else 
                    velocity = velocity - velocity * min(1.0f, Core::deltaTime * 2.0f);
                // TODO: apply flow velocity
                break;
            }
        }
    }

    virtual void move() { // TORO: as part of Controller
        if (velocity.length() < EPS) return;

        vec3 offset = velocity * Core::deltaTime * 30.0f;

        vec3 p = pos;
        pos = pos + offset;
        
        FloorInfo info = getFloorInfo((int)pos.x, (int)pos.z);

        int delta;
        int d = getOverlap((int)p.x, (int)p.y, (int)p.z, (int)pos.x, (int)pos.z, delta);

        int height = getHeight();
        bool canPassGap;

        switch (stand) {
            case STAND_AIR : 
                canPassGap = ((int)p.y - d) <= 512 && (info.roomAbove != 0xFF || (pos.y - height - info.ceiling > -256));
                break;
            case STAND_UNDERWATER : 
                canPassGap = ((int)p.y - d) <  128; 
                break;
            case STAND_ONWATER : {
                canPassGap = info.roomAbove != 0xFF;
                break;
            }
            default : // TODO: height
                if (state == STATE_WALK || state == STATE_BACK || state == STATE_STEP_LEFT || state == STATE_STEP_RIGHT) 
                    canPassGap = delta <=  256;
                else
                    canPassGap = delta >= -256 - 16; 
        }

        if (d == NO_OVERLAP || !canPassGap) {
            pos = p; // TODO: use smart ejection

        // hit the wall
            switch (stand) {
                case STAND_AIR :
                    if (state != STATE_UP_JUMP && state != STATE_REACH) {
                        setAnimation(ANIM_SMASH_JUMP);
                        velocity.x = -velocity.x * 0.5f;
                        velocity.z = -velocity.z * 0.5f;
                        velocity.y = 0.0f;
                    } else
                        velocity.x = velocity.z = 0.0f;
                    break;
                case STAND_GROUND :
                    if (delta <= -256 * 4 && state == STATE_RUN)
                        setAnimation(ANIM_SMASH_RUN_LEFT);  // TODO: RIGHT
                    else
                        setAnimation(ANIM_STAND);
                    velocity.x = velocity.z = 0.0f;
                    break;
                case STAND_UNDERWATER :
                case STAND_ONWATER    :
                    break;
            }                     
        } else 
            updateEntity();
    }
};

#endif