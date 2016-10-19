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

#define LARA_HANG_OFFSET    735

#define PICKUP_FRAME_GROUND     40
#define PICKUP_FRAME_UNDERWATER 16

#define MAX_TRIGGER_ACTIONS 64

#define DESCENT_SPEED       2048.0f

struct Lara : Controller {

    ActionCommand actionList[MAX_TRIGGER_ACTIONS];
    int lastPickUp;

    // http://www.tombraiderforums.com/showthread.php?t=148859
    enum {
        ANIM_STAND              = 11,
        
        ANIM_CLIMB_JUMP         = 26,

        ANIM_HANG_FALL          = 28,
        ANIM_HANG_WALL          = 29,

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

        ANIM_HANG               = 96,

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

        ANIM_HANG_NOWALL        = 150,
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
        STATE_ROLL_1,
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
        STATE_ROLL_2,
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
    #ifdef _DEBUG
    /*
    // gym 
        pos = vec3(43182, 2473, 51556);
        angle = vec3(0.0f, PI * 0.5f, 0.0f);
        getEntity().room = 12;
    */
    
    // level 2 (pool)
        pos = vec3(70067, -256, 29104);
        angle = vec3(0.0f, -0.68f, 0.0f);
        getEntity().room = 15;
    
    /*
    // level 2 (wolf)
        pos = vec3(75671, -1024, 22862);
        angle = vec3(0.0f, -PI * 0.25f, 0.0f);
        getEntity().room = 13;
    */

    /*
    // level 2 (room 1)
        pos = vec3(31400, -2560, 25200);
        angle = vec3(0.0f, PI, 0.0f);
        getEntity().room = 43;   
        
    // level 2 (medikit)
        pos = vec3(30800, -7936, 22131);
        angle = vec3(0.0f, 0.0f, 0.0f);
        getEntity().room = 58;   
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
        updateEntity();
    #endif
    }

    bool waterOut(int &outState) {
        // TODO: playSound 36
        vec3 dst = pos + getDir() * 32.0f;

        TR::Level::FloorInfo infoCur, infoDst;
        level->getFloorInfo(getEntity().room,  (int)pos.x, (int)pos.z, infoCur),
        level->getFloorInfo(infoCur.roomAbove, (int)dst.x, (int)dst.z, infoDst);

        int h = int(pos.y - infoDst.floor);

        if (h > 0 && h <= 256) { // possibility check
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

    bool doPickUp() {
        int room = getRoomIndex();
        TR::Entity &e = getEntity();

        for (int i = 0; i < level->entitiesCount; i++) {
            TR::Entity &item = level->entities[i];
            if (item.room == room && !item.flags.invisible) {
                if (abs(item.x - e.x) > 256 || abs(item.z - e.z) > 256)
                    continue;

                switch (item.type) {
                    case TR::Entity::WEAPON_PISTOLS :
                    case TR::Entity::WEAPON_SHOTGUN :
                    case TR::Entity::WEAPON_MAGNUMS :
                    case TR::Entity::WEAPON_UZIS    :
                    case TR::Entity::AMMO_SHOTGUN   :
                    case TR::Entity::AMMO_MAGNUMS   :
                    case TR::Entity::AMMO_UZIS      :
                    case TR::Entity::MEDIKIT_SMALL  :
                    case TR::Entity::MEDIKIT_BIG    :
                    case TR::Entity::PUZZLE_1       :
                    case TR::Entity::PUZZLE_2       :
                    case TR::Entity::PUZZLE_3       :
                    case TR::Entity::PUZZLE_4       :
                    case TR::Entity::PICKUP         :
                    case TR::Entity::KEY_1          :
                    case TR::Entity::KEY_2          :
                    case TR::Entity::KEY_3          :
                    case TR::Entity::KEY_4          :
                    case TR::Entity::ARTIFACT       :
                        lastPickUp = i;
                        angle.x = 0.0f;
                        pos.x = item.x;
                        pos.y = item.y;
                        pos.z = item.z;
                        if (stand == STAND_UNDERWATER) { // TODO: lerp to pos/angle
                            pos = pos - getDir() * 256.0f;
                            pos.y -= 256;
                        }
                        updateEntity();                       
                        return true;
                    default : ;
                }
            }
        }
        return false;
    }

    void doTrigger() {
        if (actionCommand) return;

        TR::Entity &e = getEntity();
        TR::Level::FloorInfo info;
        level->getFloorInfo(e.room, e.x, e.z, info);

        if (!info.trigCmdCount) return; // has no trigger
        bool isActive = (level->entities[info.trigCmd[0].args].flags.active);
        if (info.trigInfo.once == 1 && isActive) return; // once trigger is already activated

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
                LOG("! next activation entity %d has no controller\n", level->entities[info.trigCmd[i].args].type);
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
        if (state == STATE_HANG || state == STATE_HANG_LEFT || state == STATE_HANG_RIGHT) {
            if (mask & ACTION)
                return STAND_HANG;
            setAnimation(ANIM_HANG_FALL);
            velocity = vec3(0.0f);
            pos.y += 128.0f;
            updateEntity();
            return STAND_AIR;
        }

        if (state == STATE_HANDSTAND || state == STATE_HANG_UP)
            return STAND_HANG;

        if (stand == STAND_ONWATER && state != STATE_DIVE && state != STATE_STOP)
            return stand;

        if (getRoom().flags.water)
            return STAND_UNDERWATER; // TODO: ONWATER

        TR::Entity &e = getEntity();
        TR::Level::FloorInfo info;
        level->getFloorInfo(e.room, e.x, e.z, info, true);

        if (stand == STAND_SLIDE || (stand == STAND_AIR && velocity.y > 0) || stand == STAND_GROUND) {
            if (e.y + 8 >= info.floor && (abs(info.slantX) > 2 || abs(info.slantZ) > 2)) {
                if (stand == STAND_AIR)
                    playSound(TR::SND_LANDING);
                pos.y = info.floor;
                updateEntity();

                if (stand == STAND_GROUND || stand == STAND_AIR)
                    slideStart();
                
                return STAND_SLIDE;
            }
        }

        int extra = stand != STAND_AIR ? 256 : 0;

        if (e.y + extra >= info.floor) {
            if (stand != STAND_GROUND) {
                pos.y = info.floor;
                updateEntity();
            }
            return STAND_GROUND;
        }

        return STAND_AIR;
    }

    virtual int getHeight() {
        if (stand == STAND_GROUND || stand == STAND_AIR)
            return 768;
        return 0;
    }

    virtual int getStateAir() {
        angle.x = 0.0f;

        if (state == STATE_REACH && getDir().dot(vec3(velocity.x, 0.0f, velocity.z)) < 0)
            velocity.x = velocity.z = 0.0f;

        if ((state == STATE_REACH || state == STATE_UP_JUMP) && (mask & ACTION)) {

            if (state == STATE_REACH && velocity.y < 0.0f)
                return state;

            vec3 p = pos + getDir() * 128.0f;
            TR::Level::FloorInfo info;
            
            info.roomAbove = getRoomIndex();
            level->getFloorInfo(info.roomAbove, (int)pos.x, (int)pos.z, info);
            if (info.roomAbove == 0xFF)
                info.roomAbove = getRoomIndex();

            do {
                level->getFloorInfo(info.roomAbove, (int)p.x, (int)p.z, info, true);
            } while (info.ceiling > p.y - LARA_HANG_OFFSET && info.roomAbove != 0xFF);

            if (abs(int(info.floor - (p.y - LARA_HANG_OFFSET))) < 16) {
                turnToWall();
                pos = pos - getDir() * 96.0f; // TODO: collision wall offset
                pos.y = info.floor + LARA_HANG_OFFSET;
                updateEntity();

                stand = STAND_HANG;
                if (state == STATE_REACH) {
                    return STATE_HANG; // TODO: ANIM_HANG_WALL / ANIM_HANG_NOWALL
                } else
                    return setAnimation(ANIM_HANG, -15);
            }
        }

        if (state == STATE_FORWARD_JUMP) {
            if (mask & ACTION) return STATE_REACH;
            if ((mask & (FORTH | WALK)) == (FORTH | WALK)) return STATE_SWAN_DIVE;
        } else 
            if (state != STATE_SWAN_DIVE && state != STATE_REACH && state != STATE_FALL && state != STATE_UP_JUMP && state != STATE_BACK_JUMP && state != STATE_LEFT_JUMP && state != STATE_RIGHT_JUMP)
                return setAnimation(ANIM_FALL);

        return state;
    }

    virtual int getStateGround() {
        angle.x = 0.0f;

        if (mask == ACTION && doPickUp())
            return STATE_PICK_UP;

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
            level->getFloorInfo(getRoomIndex(), (int)p.x, (int)p.z, info, true);
            int h = (int)pos.y - info.floor;
            
            int aIndex = animIndex;
            if (h < 256 + 128) {
                ; // do nothing
            } else if (h <= 2 * 256 + 128) {
                aIndex = ANIM_CLIMB_2;
                pos.y  = info.floor + 512;
            } else if (h <= 3 * 256 + 128) {
                aIndex = ANIM_CLIMB_3;
                pos.y  = info.floor + 768;
            } else if (h <= 7 * 256 + 128)
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

    void slideStart() {
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
            setAnimation(aIndex);
        }
    }

    virtual int getStateSlide() {
        if (mask & JUMP)
            return state == STATE_SLIDE ? STATE_FORWARD_JUMP : STATE_BACK_JUMP;
        return state;
    }

    virtual int getStateHang() {
        if (mask & LEFT)  return STATE_HANG_LEFT;
        if (mask & RIGHT) return STATE_HANG_RIGHT;
        if (mask & FORTH) {
            // possibility check
            TR::Level::FloorInfo info;
            vec3 p = pos + getDir() * 128.0f;
            level->getFloorInfo(getRoomIndex(), (int)p.x, (int)p.z, info, true);
            if (info.floor - info.ceiling >= 768) {
                LOG("%d %d %d\n", info.floor, info.ceiling, info.floor - info.ceiling);
                return (mask & WALK) ? STATE_HANDSTAND : STATE_HANG_UP;
            }
        }
        return STATE_HANG;
    }

    virtual int getStateUnderwater() {
        if (mask == ACTION && doPickUp())
            return STATE_PICK_UP;

        if (state == STATE_FORWARD_JUMP || state == STATE_UP_JUMP || state == STATE_BACK_JUMP || state == STATE_LEFT_JUMP || state == STATE_RIGHT_JUMP || state == STATE_FALL || state == STATE_REACH) {
            addSprite(level, TR::Entity::Type::WATER_SPLASH, getRoomIndex(), (int)pos.x, (int)pos.y, (int)pos.z);
            return setAnimation(ANIM_WATER_FALL);
        }
        
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
        if (state == STATE_DIVE || (state == STATE_RUN && (mask & JUMP)) ) return state;
        switch (stand) {
            case STAND_GROUND     : return STATE_STOP;
            case STAND_HANG       : return STATE_HANG;
            case STAND_ONWATER    : return STATE_SURF_TREAD;
            case STAND_UNDERWATER : return STATE_TREAD;
            default : ;
        }
        return STATE_FALL;
    }

    virtual int getInputMask() {
        mask = 0;
        int &p = Input::joy.POV;
        if (Input::down[ikW] || p == 8 || p == 1 || p == 2)                     mask |= FORTH;
        if (Input::down[ikD] || p == 2 || p == 3 || p == 4)                     mask |= RIGHT;
        if (Input::down[ikS] || p == 4 || p == 5 || p == 6)                     mask |= BACK;
        if (Input::down[ikA] || p == 6 || p == 7 || p == 8)                     mask |= LEFT;
        if (Input::down[ikJoyB])                                                mask  = FORTH | BACK; // roll
        if (Input::down[ikJoyRT])                                               mask  = WALK | RIGHT; // step right
        if (Input::down[ikJoyLT])                                               mask  = WALK | LEFT;  // step left
        if (Input::down[ikSpace] || Input::down[ikJoyX])                        mask |= JUMP;
        if (Input::down[ikShift] || Input::down[ikJoyLB])                       mask |= WALK;
        if (Input::down[ikE] || Input::down[ikMouseL] || Input::down[ikJoyA])   mask |= ACTION;
        if (Input::down[ikQ] || Input::down[ikMouseR] || Input::down[ikJoyY])   mask |= WEAPON;
        if (health <= 0)                                                        mask  = DEATH;
        return mask;
    }

    virtual void updateState() {
        doTrigger();

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
                /*playSound(snd_id);
                */
                //setAnimation(snd_id);
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

    
    virtual void doCustomCommand(int curFrame, int prevFrame) {
        if (state == STATE_PICK_UP) {
            if (!level->entities[lastPickUp].flags.invisible) {
                int pickupFrame = stand == STAND_GROUND ? PICKUP_FRAME_GROUND : PICKUP_FRAME_UNDERWATER;                
                if (curFrame >= pickupFrame)
                    level->entities[lastPickUp].flags.invisible = true; // TODO: add to inventory
            }
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
                    if (stand == STAND_HANG) {
                        vec3 p = pos + getDir() * 128.0f;
                        level->getFloorInfo(e.room, (int)p.x, (int)p.z, info);
                        if (info.roomAbove != 0xFF && info.floor >= e.y - LARA_HANG_OFFSET)
                            level->getFloorInfo(info.roomAbove, (int)p.x, (int)p.z, info);
                    } else
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

    virtual void checkRoom() {
        TR::Level::FloorInfo info;
        TR::Entity &e = getEntity();
        level->getFloorInfo(e.room, e.x, e.z, info);

        if (info.roomNext != 0xFF)
            e.room = info.roomNext;        

        if (info.roomBelow != 0xFF && e.y > info.floor)
            e.room = info.roomBelow;       

        if (info.roomAbove != 0xFF && e.y <= info.ceiling) {
            if (stand == STAND_UNDERWATER && !level->rooms[info.roomAbove].flags.water) {
                stand = STAND_ONWATER;
                velocity.y = 0;
                pos.y = info.ceiling;
                updateEntity();
            } else
                if (stand != STAND_ONWATER)
                    e.room = info.roomAbove;
        }
    }

    virtual void move() { // TODO: sphere / bbox collision
        if (velocity.length() < 0.001f) return;

        vec3 offset = velocity * Core::deltaTime * 30.0f;

        vec3 p = pos;
        pos = pos + offset;

        TR::Level::FloorInfo info;
        level->getFloorInfo(getEntity().room, (int)pos.x, (int)pos.z, info, true);

    // get frame to get height
        TR::Animation *anim  = &level->anims[animIndex];

        bool canPassGap = (info.floor - info.ceiling) >= (stand == STAND_GROUND ? 768 : 512);
        float f = info.floor - pos.y;
        float c = pos.y - info.ceiling;

        if (canPassGap)
            switch (stand) {
                case STAND_AIR        : {
                    int fSize = sizeof(TR::AnimFrame) + getModel().mCount * sizeof(uint16) * 2;
                    TR::AnimFrame *frame = (TR::AnimFrame*)&level->frameData[((anim->frameOffset + (int(animTime * 30.0f / anim->frameRate) * fSize)) >> 1)];

                    f = info.floor   - (p.y + frame->box.maxY);
                    c = (p.y + frame->box.minY) - info.ceiling;
                    canPassGap = f >= -256 && c >= (state == STATE_UP_JUMP ? 0.0f : -256);  
                    break;
                }
                case STAND_GROUND     : {                    
                    if (state == STATE_WALK || state == STATE_BACK) 
                        canPassGap = fabsf(f) <= (256.0f + 128.0f);
                    else
                        if (state == STATE_STEP_LEFT || state == STATE_STEP_RIGHT)
                            canPassGap = fabsf(f) <= (128.0f + 64.0f);
                        else
                            canPassGap = f >= -(256 + 128);
                    break;
                }
                case STAND_HANG : {
                    canPassGap = f >= 220.0f; // check dist to floor
                    if (canPassGap) { // check end of hang layer
                        vec3 d = pos + getDir() * 128.0f;
                        level->getFloorInfo(info.roomAbove != 0xFF ? info.roomAbove : getRoomIndex(), (int)d.x, (int)d.z, info, true);
                        canPassGap = fabsf((pos.y - LARA_HANG_OFFSET) - info.floor) < 64.0f;
                    }
                    break;
                }
                case STAND_UNDERWATER : 
                    canPassGap = f > 0.0f && c > 0.0f;
                    break;
                case STAND_ONWATER :
                    canPassGap = (info.floor - p.y) >= 1.0f && c >= 1.0f;
                    break;
                default : ;
            }

        bool isLeftFoot = (anim->frameEnd - anim->frameStart) / 2 > int(animTime * 30.0f);

        if (!canPassGap) {
            pos = p; // TODO: use smart ejection

        // hit the wall
            switch (stand) {
                case STAND_AIR :
                    if (state == STATE_UP_JUMP || state == STATE_REACH) {
                        velocity.x = velocity.z = 0.0f;
                        if (c <= 0 && offset.y < 0.0f) offset.y = velocity.y = 0.0f;
                    }

                    if (velocity.x != 0.0f || velocity.z != 0.0f) {
                        setAnimation(ANIM_SMASH_JUMP);
                        velocity.x = -velocity.x * 0.5f;
                        velocity.z = -velocity.z * 0.5f;
                        if (offset.y < 0.0f)
                            offset.y = velocity.y = 0.0f;
                    }

                    pos.y = p.y + offset.y;

                    break;
                case STAND_GROUND :
                case STAND_HANG   :
                    if (f <= -(256 * 3 - 128) && state == STATE_RUN)
                        setAnimation(isLeftFoot ? ANIM_SMASH_RUN_LEFT : ANIM_SMASH_RUN_RIGHT);
                    else if (stand == STAND_HANG)
                        setAnimation(ANIM_HANG, -21);
                    else if (state != STATE_ROLL_1 && state != STATE_ROLL_2)
                        setAnimation(ANIM_STAND);
                    velocity.x = velocity.z = 0.0f;
                    break;
                default : ;// no smash animation
            }                     
        } else
            if (stand == STAND_GROUND) {
                int h = int(info.floor - pos.y);

                if (h >= 256 && state == STATE_FAST_BACK) {
                    stand = STAND_AIR;
                    setAnimation(ANIM_FALL);
                } else if (h >= 128 && (state == STATE_WALK || state == STATE_BACK)) { // descend
                    if (state == STATE_WALK) setAnimation(isLeftFoot ? ANIM_WALK_DESCEND_LEFT : ANIM_WALK_DESCEND_RIGHT);
                    if (state == STATE_BACK) setAnimation(isLeftFoot ? ANIM_BACK_DESCEND_LEFT : ANIM_BACK_DESCEND_RIGHT);
                    pos.y = info.floor;
                } else if (h > -1.0f) {
                    pos.y = min((float)info.floor, pos.y += DESCENT_SPEED * Core::deltaTime);
                } else if (h > -128) {
                    pos.y = info.floor;
                } else if (h >= -(256 + 128) && (state == STATE_RUN || state == STATE_WALK)) { // ascend
                    if (state == STATE_RUN)  setAnimation(isLeftFoot ? ANIM_RUN_ASCEND_LEFT  : ANIM_RUN_ASCEND_RIGHT);
                    if (state == STATE_WALK) setAnimation(isLeftFoot ? ANIM_WALK_ASCEND_LEFT : ANIM_WALK_ASCEND_RIGHT);
                    pos.y = info.floor;
                } else
                    pos.y = info.floor;
            }

        updateEntity();
        checkRoom();
    }
};

#endif