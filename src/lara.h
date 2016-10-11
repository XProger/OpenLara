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


#define MAX_TRIGGER_ACTIONS 64

#define DESCENT_SPEED       2048.0f

struct Lara : Controller {

    ActionCommand actionList[MAX_TRIGGER_ACTIONS];

    // http://www.tombraiderforums.com/showthread.php?t=148859
    enum {
        ANIM_STAND              = 11,
        
        ANIM_CLIMB_JUMP         = 26,

        ANIM_HANG_UP            = 29,

        ANIM_FALL               = 34,
        ANIM_SMASH_JUMP         = 32,

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

        ANIM_HANG_FORTH         = 96,

        ANIM_STAND_NORMAL       = 103,

        ANIM_SLIDE_BACK         = 105,

        ANIM_WATER_FALL         = 112,
        ANIM_TO_ONWATER         = 114,
        ANIM_TO_UNDERWATER      = 119,
        ANIM_HIT_FRONT          = 125,
        ANIM_HIT_BACK           = 126,
        ANIM_HIT_LEFT           = 127,
        ANIM_HIT_RIGHT          = 128,
        ANIM_STAND_ROLL_BEGIN   = 146,
        ANIM_STAND_ROLL_END     = 147,
    };

    // http://www.tombraiderforums.com/showthread.php?t=211681
    enum {
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
        STATE_HANG_UP,
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
        STATE_SWITCH_DOWN,
        STATE_SWITCH_UP,
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

    bool waterOut(int &outState) {
        // TODO: playSound 36
        vec3 dst = pos + getDir() * 32.0f;

        TR::Level::FloorInfo infoCur, infoDst;
        level->getFloorInfo(getEntity().room,  (int)pos.x, (int)pos.z, infoCur),
        level->getFloorInfo(infoCur.roomAbove, (int)dst.x, (int)dst.z, infoDst);

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

    void performTrigger() {
        if (actionCommand) return;

        TR::Entity &e = getEntity();
        TR::Level::FloorInfo info;
        level->getFloorInfo(e.room, e.x, e.z, info);

        if (!info.trigCmdCount) return; // has no trigger
        bool isActive = (level->entities[info.trigCmd[0].args].flags & ENTITY_FLAG_ACTIVE);
        if (info.trigInfo.once == 1 && (level->entities[info.trigCmd[0].args].flags & ENTITY_FLAG_ACTIVE)) return; // once trigger is already activated

        int actionState = state;
        switch (info.trigger) {
            case TR::Level::Trigger::ACTIVATE :
                if (isActive) return;
                break;
            case TR::Level::Trigger::PAD :
                if (stand != STAND_GROUND || isActive) return;
                break;
            case TR::Level::Trigger::SWITCH :
                actionState = (isActive && stand == STAND_GROUND) ? STATE_SWITCH_UP : STATE_SWITCH_DOWN;
                if ((mask & ACTION) == 0 || state == actionState)
                    return;
                if (fabsf(shortAngle(level->entities[info.trigCmd[0].args].rotation, e.rotation)) > PI * 0.25f) // TODO clamp angles
                    return;
                break;
            case TR::Level::Trigger::KEY :
                actionState = STATE_USE_KEY;
                if (isActive || (mask & ACTION) == 0 || state == actionState)   // TODO: STATE_USE_PUZZLE
                    return;
                if (fabsf(shortAngle(level->entities[info.trigCmd[0].args].rotation, e.rotation)) > PI * 0.25f) // TODO clamp angles
                    return;
                break;
            case TR::Level::Trigger::PICKUP :
                if (!isActive)  // check if item is not picked up
                    return;
                break;
            default :
                LOG("unsupported trigger type %d\n", info.trigger);
                return;
        }

        // try to activate Lara state
        if (!setState(actionState)) return;

        if (info.trigger == TR::Level::Trigger::SWITCH || info.trigger == TR::Level::Trigger::KEY) {
            TR::Entity &p = level->entities[info.trigCmd[0].args];
            angle.y = p.rotation;
            angle.x = 0;
            pos = vec3(p.x, p.y, p.z) + vec3(sinf(angle.y), 0, cosf(angle.y)) * (stand == STAND_GROUND ? 384 : 128);
            velocity = vec3(0.0f);
            updateEntity();
        }

        // build trigger activation chain
        ActionCommand *actionItem = &actionList[1];

        Controller *controller = this;
        for (int i = 0; i < info.trigCmdCount; i++) {
            if (!controller) {
                LOG("! next activation entity %d has no controller\n", level->entities[info.trigCmd[i].args].id);
                playSound(TR::SND_NO);
                return;
            }

            if (info.trigger == TR::Level::Trigger::KEY && i == 0) continue; // skip keyhole

            TR::FloorData::TriggerCommand &cmd = info.trigCmd[i];
            switch (cmd.action) {
                case TR::Action::CAMERA_SWITCH :
                    *actionItem = ActionCommand(cmd.action, cmd.args, (float)info.trigCmd[++i].delay);    // camera switch uses next command for delay timer
                    break;
                default :
                    *actionItem = ActionCommand(cmd.action, cmd.args, info.trigInfo.timer);
            }

            actionItem->next = (i < info.trigCmdCount - 1) ? actionItem + 1 : NULL;
            actionItem++;
        }

        LOG("perform\n");
        actionList[0].next = &actionList[1];
        actionCommand = &actionList[0];

        if (info.trigger != TR::Level::Trigger::KEY)
            activateNext();
    }

    vec3 getViewOffset() {
        TR::Animation *anim = &level->anims[animIndex];
        TR::Model &model = getModel();

        float k = animTime * 30.0f / anim->frameRate;
        int fIndex = (int)k;
        int fCount = (anim->frameEnd - anim->frameStart) / anim->frameRate + 1;

        int fSize = sizeof(TR::AnimFrame) + model.mCount * sizeof(uint16) * 2;
        k = k - fIndex;

        int fIndexA = fIndex % fCount, fIndexB = (fIndex + 1) % fCount;
        TR::AnimFrame *frameA = (TR::AnimFrame*)&level->frameData[(anim->frameOffset + fIndexA * fSize) >> 1];

        TR::Animation *nextAnim = NULL;

        vec3 move(0.0f);
        if (fIndexB == 0) {
            move     = getAnimMove();
            nextAnim = &level->anims[anim->nextAnimation];
            fIndexB  = (anim->nextFrame - nextAnim->frameStart) / nextAnim->frameRate;
        } else
            nextAnim = anim;

        TR::AnimFrame *frameB = (TR::AnimFrame*)&level->frameData[(nextAnim->frameOffset + fIndexB * fSize) >> 1];

        float h = ((vec3)frameA->pos).lerp(move + frameB->pos, k).y;

        switch (stand) {
            case Controller::STAND_AIR    :
            case Controller::STAND_GROUND :
            case Controller::STAND_SLIDE  :
            case Controller::STAND_HANG   :
                h -= 256.0f;
                break;
            case Controller::STAND_UNDERWATER :
            case Controller::STAND_ONWATER    :
                h -= 128.0f;
                break;
        }

        return vec3(0.0f, h, 0.0f);
    }

    virtual Stand getStand() {
        if (stand == STAND_HANG && (mask & ACTION))
            return stand;

        if (stand == STAND_ONWATER && state != STATE_DIVE && state != STATE_STOP)
            return stand;

        if (getRoom().flags & TR::ROOM_FLAG_WATER)
            return STAND_UNDERWATER; // TODO: ONWATER

        TR::Entity &e = getEntity();
        TR::Level::FloorInfo info;
        level->getFloorInfo(e.room, e.x, e.z, info);

        if (stand == STAND_SLIDE || (stand == STAND_AIR && velocity.y > 0) || stand == STAND_GROUND) {
            if (e.y + 8 >= info.floor && (abs(info.slantX) > 2 || abs(info.slantZ) > 2)) {
                if (stand == STAND_AIR)
                    playSound(TR::SND_LANDING);
                return STAND_SLIDE;
            }
        }

        int extra = stand != STAND_AIR ? 256 : 0;

        if (stand != STAND_HANG && info.roomBelow == 0xFF && e.y + extra >= info.floor)
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

        if ((state == STATE_REACH || state == STATE_UP_JUMP) && (mask & ACTION)) {
            vec3 p = pos + getDir() * 128.0f;
            TR::Level::FloorInfo info;
            level->getFloorInfo(getRoomIndex(), (int)p.x, (int)p.z, info);

            if (abs(info.floor - (p.y - 768.0f + 64.0f)) < 32) {
                turnToWall();
                pos = pos - getDir() * 128.0f; // TODO: collision wall offset
                pos.y = info.floor + 768.0f - 64.0f;
                stand = STAND_HANG;
                updateEntity();
                return setAnimation(state == STATE_HANG ? ANIM_HANG_UP : ANIM_HANG_FORTH);
            }
        }

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
            if (state == STATE_RUN)
                return STATE_FORWARD_JUMP;
            if (animIndex == ANIM_SLIDE_BACK)
                return STATE_SLIDE_BACK;                
            return STATE_COMPRESS;
        }
        
        // walk button is pressed
        if (mask & WALK) {      
            if (mask & FORTH) return STATE_WALK;
            if (mask & BACK)  return STATE_BACK;
            if (mask & LEFT)  return STATE_STEP_LEFT;
            if (mask & RIGHT) return STATE_STEP_RIGHT;
            return STATE_STOP;
        } 

        if ( (mask & (FORTH | ACTION)) == (FORTH | ACTION) && (animIndex == ANIM_STAND || animIndex == ANIM_STAND_NORMAL) ) {
            vec3 p = pos + getDir() * 64.0f;
            TR::Level::FloorInfo info;
            level->getFloorInfo(getRoomIndex(), (int)p.x, (int)p.z, info);
            int h = (int)pos.y - info.floor;
            
            int aIndex = animIndex;
            if (h < 2 * 256 - 16)
                ; // do nothing
            else if (h <= 2 * 256 + 16)
                aIndex = ANIM_CLIMB_2;
            else if (h <= 3 * 256 + 16)
                aIndex = ANIM_CLIMB_3;
            else if (h <= 7 * 256 + 16)
                aIndex = ANIM_CLIMB_JUMP;

            if (aIndex != animIndex) {
                turnToWall();
                updateEntity();
                return setAnimation(aIndex);
            }
        }

        // only dpad buttons pressed
        if (mask & FORTH) return STATE_RUN;
        if (mask & BACK)  return STATE_FAST_BACK;
        if (mask & LEFT)  return turnTime < FAST_TURN_TIME ? STATE_TURN_LEFT  : STATE_FAST_TURN;
        if (mask & RIGHT) return turnTime < FAST_TURN_TIME ? STATE_TURN_RIGHT : STATE_FAST_TURN;
        return STATE_STOP;
    }

    virtual int getStateSlide() {

        TR::Entity &e = getEntity();

        if (state != STATE_SLIDE && state != STATE_SLIDE_BACK) {
            TR::Level::FloorInfo info;
            level->getFloorInfo(e.room, e.x, e.z, info);

            int sx = abs(info.slantX), sz = abs(info.slantZ);
            // get direction
            float dir;
            if (sx > sz)
                dir = info.slantX > 0 ? 3.0f : 1.0f;
            else
                dir = info.slantZ > 0 ? 2.0f : 0.0f;
            dir *= PI * 0.5f;

            int aIndex = ANIM_SLIDE_FORTH;
            if (fabsf(shortAngle(dir, angle.y)) > PI * 0.5f) {
                aIndex = ANIM_SLIDE_BACK;
                dir += PI;
            }

            angle.y = dir;
            updateEntity();
            return setAnimation(aIndex);
        }

        if (mask & JUMP) {
            stand    = STAND_GROUND;
            pos.y   -= 16;
            updateEntity();
            return state == STATE_SLIDE ? STATE_FORWARD_JUMP : STATE_BACK_JUMP;
        }

        return state;
    }

    virtual int getStateHang() {
        if (mask & LEFT)  return STATE_HANG_LEFT;
        if (mask & RIGHT) return STATE_HANG_RIGHT;
        if (mask & FORTH) return (mask & WALK) ? STATE_HANDSTAND : STATE_HANG_UP;
        return STATE_HANG;
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
        if (state == STATE_DIVE) return state;
        switch (stand) {
            case STAND_GROUND     : return STATE_STOP;
            case STAND_HANG       : return STATE_HANG;
            case STAND_ONWATER    : return STATE_SURF_TREAD;
            case STAND_UNDERWATER : return STATE_TREAD;
        }
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
        performTrigger();

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
                /*
                static int snd_id = 0;//160;
                playSound(snd_id);
                */
            //    setAnimation(snd_id);
                //LOG("sound: %d\n", snd_id++);
                
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
            case STATE_SLIDE_BACK :
                angleExt += PI;
                break;
            case STATE_LEFT_JUMP  :
            case STATE_STEP_LEFT  :
            case STATE_SURF_LEFT  :
            case STATE_HANG_LEFT  :
                angleExt -= PI * 0.5f;
                break;
            case STATE_RIGHT_JUMP :
            case STATE_STEP_RIGHT :
            case STATE_SURF_RIGHT :
            case STATE_HANG_RIGHT :
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
            case STAND_GROUND  :
            case STAND_SLIDE   :
            case STAND_HANG    :
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

                if (stand == STAND_ONWATER) {
                    velocity.x = sinf(angleExt) * speed;
                    velocity.z = cosf(angleExt) * speed;
                    velocity.y = 0.0f;
                } else {
                    TR::Entity &e = getEntity();
                    TR::Level::FloorInfo info;
                    level->getFloorInfo(e.room, e.x, e.z, info);

                    vec3 v(sinf(angleExt), 0.0f, cosf(angleExt));
                    velocity = info.getSlant(v) * speed;
                }
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

        TR::Level::FloorInfo info;
        level->getFloorInfo(getEntity().room, (int)pos.x, (int)pos.z, info);

        int delta;
        int d = getOverlap((int)p.x, (int)p.y, (int)p.z, (int)pos.x, (int)pos.z, delta);

        int height = getHeight();
        bool canPassGap;

        float h = info.floor - pos.y;

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
                if (state == STATE_WALK || state == STATE_BACK) 
                    canPassGap = h >= -256 && h <=  256;
                else
                    if (state == STATE_STEP_LEFT || state == STATE_STEP_RIGHT)
                        canPassGap = h >= -128 && h <= 128;
                    else
                        canPassGap = h >= -256 - 16; 
        }

        TR::Animation *anim = &level->anims[animIndex];
        int frame = int(animTime * 30.0f);
        bool left = (anim->frameEnd - anim->frameStart) / 2 > frame;

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
                    } else {
                        velocity.x = velocity.z = 0.0f;
                        pos.y = p.y + offset.y;
                        updateEntity();
                    }
                    break;
                case STAND_GROUND :
                    if (delta <= -256 * 4 && state == STATE_RUN)
                        setAnimation(left ? ANIM_SMASH_RUN_LEFT : ANIM_SMASH_RUN_RIGHT);
                    else
                        setAnimation(ANIM_STAND);
                    velocity.x = velocity.z = 0.0f;
                    break;
                default : ;// no smash animation
            }                     
        } else {
            if (state == STATE_RUN || state == STATE_WALK || state == STATE_BACK || state == STATE_ROLL) {
                if (h <= -128 && h >= -256) { // ascend
                    if (state == STATE_RUN)  setAnimation(left ? ANIM_RUN_ASCEND_LEFT  : ANIM_RUN_ASCEND_RIGHT);
                    if (state == STATE_WALK) setAnimation(left ? ANIM_WALK_ASCEND_LEFT : ANIM_WALK_ASCEND_RIGHT);
                    pos.y = info.floor;
                }

                if (h >= 128 && h <= 256 && (state == STATE_WALK || state == STATE_BACK)) { // descend
                    if (state == STATE_WALK) setAnimation(left ? ANIM_WALK_DESCEND_LEFT : ANIM_WALK_DESCEND_RIGHT);
                    if (state == STATE_BACK) setAnimation(left ? ANIM_BACK_DESCEND_LEFT : ANIM_BACK_DESCEND_RIGHT);
                    pos.y = info.floor;
                }

                if (h > 0 && (state == STATE_RUN || state == STATE_ROLL)) {
                    pos.y += DESCENT_SPEED * Core::deltaTime;
                }
            }

            if (state == STATE_FAST_BACK) {
                if (h >= 255) {
                    stand = STAND_AIR;
                    setAnimation(ANIM_FALL);
                } else
                    pos.y += DESCENT_SPEED * Core::deltaTime;
            }


            updateEntity();
        }
    }
};

#endif