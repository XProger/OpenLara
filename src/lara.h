#ifndef H_LARA
#define H_LARA
/*****************************************/
/*     Desine sperare qui hic intras     */
/*****************************************/
#include "character.h"
#include "trigger.h"
#include "sprite.h"
#include "inventory.h"

#define TURN_FAST           PI
#define TURN_FAST_BACK      PI * 3.0f / 4.0f
#define TURN_NORMAL         PI / 2.0f
#define TURN_SLOW           PI / 3.0f
#define TURN_WATER_FAST     PI * 3.0f / 4.0f
#define TURN_WATER_SLOW     PI * 2.0f / 3.0f

#define TILT_MAX            (PI / 18.0f)
#define TILT_SPEED          TILT_MAX

#define GLIDE_SPEED         35.0f
#define SWIM_SPEED          45.0f

#define LARA_HANG_OFFSET    735.0f

#define LARA_WET_SPECULAR   0.5f
#define LARA_WET_TIMER      (LARA_WET_SPECULAR / 16.0f)   // 4 sec

#define PICKUP_FRAME_GROUND     40
#define PICKUP_FRAME_UNDERWATER 16
#define PUZZLE_FRAME            80

#define MAX_TRIGGER_ACTIONS 64

#define DESCENT_SPEED       2048.0f
#define MUZZLE_FLASH_TIME   0.1f
#define FLASH_LIGHT_COLOR   vec4(0.8f, 0.7f, 0.3f, 3072 * 3072)
#define TARGET_MAX_DIST     (8.0f * 1024.0f)

struct Lara : Character {

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

        ANIM_UNDERWATER         = 108,

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
    
    enum : int {
        BODY_HIP        = 0x0001,
        BODY_LEG_L1     = 0x0002,
        BODY_LEG_L2     = 0x0004,
        BODY_LEG_L3     = 0x0008,
        BODY_LEG_R1     = 0x0010,
        BODY_LEG_R2     = 0x0020,
        BODY_LEG_R3     = 0x0040,
        BODY_CHEST      = 0x0080,
        BODY_ARM_R1     = 0x0100,
        BODY_ARM_R2     = 0x0200,
        BODY_ARM_R3     = 0x0400,
        BODY_ARM_L1     = 0x0800,
        BODY_ARM_L2     = 0x1000,
        BODY_ARM_L3     = 0x2000,
        BODY_HEAD       = 0x4000,
        BODY_ARM_L      = BODY_ARM_L1 | BODY_ARM_L2 | BODY_ARM_L3,
        BODY_ARM_R      = BODY_ARM_R1 | BODY_ARM_R2 | BODY_ARM_R3,
        BODY_LEG_L      = BODY_LEG_L1 | BODY_LEG_L2 | BODY_LEG_L3,
        BODY_LEG_R      = BODY_LEG_R1 | BODY_LEG_R2 | BODY_LEG_R3,
        BODY_UPPER      = BODY_CHEST  | BODY_ARM_L  | BODY_ARM_R,       // without head
        BODY_LOWER      = BODY_HIP    | BODY_LEG_L  | BODY_LEG_R,
    };

    bool home;

    struct Weapon {
        enum Type  { EMPTY, PISTOLS, SHOTGUN, MAGNUMS, UZIS, MAX };
        enum State { IS_HIDDEN, IS_ARMED, IS_FIRING };
        enum Anim  { NONE, PREPARE, UNHOLSTER, HOLSTER, HOLD, AIM, FIRE };

        int ammo;   // if -1 weapon is not available
    } weapons[Weapon::MAX];

    Weapon::Type    wpnCurrent;
    Weapon::Type    wpnNext;
    Weapon::State   wpnState;
    vec3            chestOffset;

    struct Arm {
        int             target;
        float           shotTimer;
        quat            rot, rotAbs;
        Weapon::Anim    anim;
        Animation       animation;
    } arms[2];

    ActionCommand actionList[MAX_TRIGGER_ACTIONS];
    
    Inventory inventory;
    int lastPickUp;
    int viewTarget;

    Lara(TR::Level *level, int entity, bool home) : Character(level, entity, 1000), home(home), wpnCurrent(Weapon::EMPTY), wpnNext(Weapon::EMPTY), chestOffset(pos), viewTarget(-1) {
        animation.setAnim(ANIM_STAND);
        getEntity().flags.active = 1;
        initMeshOverrides();
        for (int i = 0; i < 2; i++) {
            arms[i].shotTimer = MUZZLE_FLASH_TIME + 1.0f;
            arms[i].rot       = quat(0, 0, 0, 1);
            arms[i].rotAbs    = quat(0, 0, 0, 1);
        }

        memset(weapons, -1, sizeof(weapons));
        if (!home) {
            weapons[Weapon::PISTOLS].ammo = 0;
            weapons[Weapon::SHOTGUN].ammo = 9000;
            weapons[Weapon::MAGNUMS].ammo = 9000;
            weapons[Weapon::UZIS   ].ammo = 9000;
            wpnSet(Weapon::PISTOLS);
        } else
            meshSwap(1, TR::MODEL_LARA_SPEC, BODY_UPPER | BODY_LOWER);
    #ifdef _DEBUG
/*
    // gym 
        pos = vec3(43182, 2473, 51556);
        angle = vec3(0.0f, PI * 0.5f, 0.0f);
        getEntity().room = 12;
        
    // level 2 (pool)
        pos = vec3(70067, -256, 29104);
        angle = vec3(0.0f, -0.68f, 0.0f);
        getEntity().room = 15;
        
    // level 2 (blade)
        pos = vec3(27221, -1024, 29205);
        angle = vec3(0.0f, PI * 0.5f, 0.0f);
        getEntity().room = 61;
        
    // level 2 (wolf)
        pos = vec3(75671, -1024, 22862);
        angle = vec3(0.0f, -PI * 0.25f, 0.0f);
        getEntity().room = 13;
        
    // level 2 (room 1)
        pos = vec3(31400, -2560, 25200);
        angle = vec3(0.0f, PI, 0.0f);
        getEntity().room = 43;   
        
    // level 2 (medikit)
        pos = vec3(30800, -7936, 22131);
        angle = vec3(0.0f, 0.0f, 0.0f);
        getEntity().room = 58;   
        
    // level 2 (block)
        pos = vec3(60843, 1024, 30557);
        angle = vec3(0.0f, PI, 0.0f);
        getEntity().room = 19;   
        
    // level 2 (bat trigger)
        pos = vec3(64108, -512, 16514);
        angle = vec3(0.0f, -PI * 0.5f, 0.0f);
        getEntity().room = 7;   
        
    // level 2 (bear)
        pos = vec3(70082, -512, 26935);
        angle = vec3(0.0f, PI * 0.5f, 0.0f);
        getEntity().room = 15;   
        
    // level 2 (trap floor)
        pos = vec3(31390, -2048, 33472);
        angle = vec3(0.0f, 0.0f, 0.0f);
        getEntity().room = 63;
        
    // level 2 (trap door)
        pos = vec3(21987, -1024, 29144);
        angle = vec3(0.0f, PI * 3.0f * 0.5f, 0.0f);
        getEntity().room = 61;
        
    // level 3a (t-rex)
        pos = vec3(41015, 3584, 34494);
        angle = vec3(0.0f, -PI, 0.0f);
        getEntity().room = 51;
        
    // level 3a (gears)
        pos = vec3(38643, -3072, 92370);
        angle = vec3(0.0f, PI * 0.5f, 0.0f);
        getEntity().room = 5;
        
    // level 1
        pos = vec3(20215, 6656, 52942);
        angle = vec3(0.0f, PI, 0.0f);
        getEntity().room = 14;

    // level 8c
        pos = vec3(40913, -1012, 42252);
        angle = vec3(0.0f, PI, 0.0f);
        getEntity().room = 0;
*/
        updateEntity();
    #endif
        chestOffset = animation.getJoints(getMatrix(), 7).pos;
        if (getRoom().flags.water)
            animation.setAnim(ANIM_UNDERWATER);
    }
    
    void wpnSet(Weapon::Type wType) {
        wpnCurrent = wType;
        wpnState   = Weapon::IS_FIRING;

        arms[0].animation = arms[1].animation = Animation(level, &level->models[wType == Weapon::SHOTGUN ? TR::MODEL_SHOTGUN : TR::MODEL_PISTOLS]);

        wpnSetAnim(arms[0], Weapon::IS_HIDDEN, Weapon::Anim::NONE, 0.0f, 0.0f);
        wpnSetAnim(arms[1], Weapon::IS_HIDDEN, Weapon::Anim::NONE, 0.0f, 0.0f);
    }

    void wpnSetAnim(Arm &arm, Weapon::State wState, Weapon::Anim wAnim, float wAnimTime, float wAnimDir, bool playing = true) {
        arm.animation.setAnim(wpnGetAnimIndex(wAnim), 0, wAnim == Weapon::Anim::FIRE);
        arm.animation.dir = playing ? wAnimDir : 0.0f;

        if (arm.anim != wAnim)
            arm.animation.frameIndex = 0xFFFF;

        arm.anim = wAnim;

        if (wAnimDir > 0.0f) 
            arm.animation.time = wAnimTime;
        else 
            if (wAnimDir < 0.0f)
                arm.animation.time = arm.animation.timeMax + wAnimTime;
        arm.animation.updateInfo();

        wpnSetState(wState);
    }

    int wpnGetDamage() {
        switch (wpnCurrent) {
            case Weapon::PISTOLS : return 1;
            case Weapon::SHOTGUN : return 1;
            case Weapon::MAGNUMS : return 2;
            case Weapon::UZIS    : return 1;
            default : ;
        }
        return 0;
    }

    void wpnSetState(Weapon::State wState) {
        if (wpnState == wState) return;

        int mask = 0;
        switch (wpnCurrent) {
            case Weapon::EMPTY   : break;
            case Weapon::PISTOLS : 
            case Weapon::MAGNUMS :
            case Weapon::UZIS    :
                switch (wState) {
                    case Weapon::IS_HIDDEN : mask = BODY_LEG_L1 | BODY_LEG_R1;              break;
                    case Weapon::IS_ARMED  : mask = BODY_ARM_L3 | BODY_ARM_R3;              break;
                    case Weapon::IS_FIRING : mask = BODY_ARM_L3 | BODY_ARM_R3 | BODY_HEAD;  break;
                }
                break;
            case Weapon::SHOTGUN :
                switch (wState) {
                    case Weapon::IS_HIDDEN : mask = BODY_CHEST;                             break;
                    case Weapon::IS_ARMED  : mask = BODY_ARM_L3 | BODY_ARM_R3;              break;
                    case Weapon::IS_FIRING : mask = BODY_ARM_L3 | BODY_ARM_R3 | BODY_HEAD;  break;
                }
                break;
            default : ;
        }

        if (wpnState == Weapon::IS_HIDDEN && wState == Weapon::IS_ARMED)  playSound(TR::SND_UNHOLSTER, pos, Sound::Flags::PAN);
        if (wpnState == Weapon::IS_ARMED  && wState == Weapon::IS_HIDDEN) playSound(TR::SND_HOLSTER,   pos, Sound::Flags::PAN);

    // swap layers
    // 0 - body (full)
    // 1 - legs (hands, legs)
    // 2 - shotgun (hands, chest)
    // 3 - angry (head)

        // swap weapon parts        
        if (wpnCurrent != Weapon::SHOTGUN) {
            meshSwap(1, wpnCurrent, mask);
            // have a shotgun in inventory place it on the back if another weapon is in use
            meshSwap(2, Weapon::SHOTGUN, (weapons[Weapon::SHOTGUN].ammo != -1) ? BODY_CHEST : 0);
        } else {
            meshSwap(2, wpnCurrent, mask);
        }

        // mesh swap to angry Lara's head while firing (from uzis model)
        meshSwap(3, Weapon::UZIS, (wState == Weapon::IS_FIRING) ? BODY_HEAD : 0);
            
        wpnState = wState;
    }

    bool emptyHands() {
        return wpnCurrent == Weapon::EMPTY || arms[0].anim == Weapon::Anim::NONE;
    }

    bool canLookAt() {
        return (stand == STAND_GROUND || stand == STAND_SLIDE)  
               && state != STATE_REACH
               && state != STATE_PUSH_BLOCK
               && state != STATE_PULL_BLOCK
               && state != STATE_PUSH_PULL_READY
               && state != STATE_PICK_UP;
    }

    bool canDrawWeapon() {
        return wpnCurrent != Weapon::EMPTY
               && emptyHands()
               && state != STATE_DEATH 
               && state != STATE_HANG
               && state != STATE_REACH
               && state != STATE_TREAD
               && state != STATE_SWIM
               && state != STATE_GLIDE
               && state != STATE_HANG_UP
               && state != STATE_FALL_BACK
               && state != STATE_HANG_LEFT
               && state != STATE_HANG_RIGHT
               && state != STATE_SURF_TREAD
               && state != STATE_SURF_SWIM
               && state != STATE_DIVE
               && state != STATE_PUSH_BLOCK
               && state != STATE_PULL_BLOCK
               && state != STATE_PUSH_PULL_READY
               && state != STATE_PICK_UP
               && state != STATE_SWITCH_DOWN
               && state != STATE_SWITCH_UP
               && state != STATE_USE_KEY
               && state != STATE_USE_PUZZLE
               && state != STATE_UNDERWATER_DEATH
               && state != STATE_SPECIAL
               && state != STATE_SURF_BACK
               && state != STATE_SURF_LEFT
               && state != STATE_SURF_RIGHT
               && state != STATE_SWAN_DIVE
               && state != STATE_FAST_DIVE
               && state != STATE_HANDSTAND
               && state != STATE_WATER_OUT;
    }

    bool wpnReady() {
        return arms[0].anim != Weapon::Anim::PREPARE && arms[0].anim != Weapon::Anim::UNHOLSTER && arms[0].anim != Weapon::Anim::HOLSTER;
    }

    void wpnDraw() {
        if (!canDrawWeapon()) return;

        if (wpnReady() && emptyHands()) {
            if (wpnCurrent != Weapon::SHOTGUN) {
                wpnSetAnim(arms[0], wpnState, Weapon::Anim::PREPARE, 0.0f, 1.0f);
                wpnSetAnim(arms[1], wpnState, Weapon::Anim::PREPARE, 0.0f, 1.0f);
            } else
                wpnSetAnim(arms[0], wpnState, Weapon::Anim::UNHOLSTER, 0.0f, 1.0f);            
        }
    }

    void wpnHide() {
        if (wpnReady() && !emptyHands()) {
            if (wpnCurrent != Weapon::SHOTGUN) {
                wpnSetAnim(arms[0], wpnState, Weapon::Anim::UNHOLSTER, 0.0f, -1.0f);
                wpnSetAnim(arms[1], wpnState, Weapon::Anim::UNHOLSTER, 0.0f, -1.0f);
            } else
                wpnSetAnim(arms[0], wpnState, Weapon::Anim::HOLSTER, 0.0f, 1.0f);
        }
    }

    void wpnChange(Weapon::Type wType) {
        if (wpnCurrent == wType || home) return;
        wpnNext = wType;
        wpnHide();
    }

    int wpnGetAnimIndex(Weapon::Anim wAnim) {
        if (wpnCurrent == Weapon::SHOTGUN) {
            switch (wAnim) {
                case Weapon::Anim::PREPARE   : ASSERT(false); break;    // rifle has no prepare animation
                case Weapon::Anim::UNHOLSTER : return 1;
                case Weapon::Anim::HOLSTER   : return 3;
                case Weapon::Anim::HOLD      : 
                case Weapon::Anim::AIM       : return 0;
                case Weapon::Anim::FIRE      : return 2;
                default : ;
            }
        } else
            switch (wAnim) {
                case Weapon::Anim::PREPARE   : return 1;
                case Weapon::Anim::UNHOLSTER : return 2;
                case Weapon::Anim::HOLSTER   : ASSERT(false); break;    // pistols has no holster animation (it's reversed unholster)
                case Weapon::Anim::HOLD      : 
                case Weapon::Anim::AIM       : return 0;
                case Weapon::Anim::FIRE      : return 3;
                default : ;
            }
        return 0;
    }

    int wpnGetSound() {
        switch (wpnCurrent) {
            case Weapon::PISTOLS : return TR::SND_PISTOLS_SHOT;
            case Weapon::SHOTGUN : return TR::SND_SHOTGUN_SHOT;
            case Weapon::MAGNUMS : return TR::SND_MAGNUMS_SHOT;
            case Weapon::UZIS    : return TR::SND_UZIS_SHOT;
            default              : return TR::SND_NO;
        }
    }

    void wpnFire() {
        bool armShot[2] = { false, false };
        for (int i = 0; i < 2; i++) {
            if (arms[i].anim == Weapon::Anim::FIRE) {
                Animation &anim = arms[i].animation;
                //int realFrameIndex = int(arms[i].animation.time * 30.0f / anim->frameRate) % ((anim->frameEnd - anim->frameStart) / anim->frameRate + 1);
                if (anim.frameIndex != anim.framePrev) {
                    if (anim.frameIndex == 0) { //realFrameIndex < arms[i].animation.framePrev) {
                        if ((input & ACTION) && (target == -1 || (target > -1 && arms[i].target > -1))) {
                            armShot[i] = true;
                        } else
                            wpnSetAnim(arms[i], Weapon::IS_ARMED, Weapon::Anim::AIM, 0.0f, -1.0f, target == -1);
                    }
                // shotgun reload sound
                    if (wpnCurrent == Weapon::SHOTGUN) { 
                        if (anim.frameIndex == 10)
                            playSound(TR::SND_SHOTGUN_RELOAD, pos, Sound::Flags::PAN);
                    }
                }

            }
            arms[i].animation.framePrev = arms[i].animation.frameIndex;

            if (wpnCurrent == Weapon::SHOTGUN) break;
        }

        if (armShot[0] || armShot[1])
            doShot(armShot[0], armShot[1]);
    }

    void doShot(bool rightHand, bool leftHand) {
        int count = wpnCurrent == Weapon::SHOTGUN ? 6 : 2;
        
        float nearDist = 32.0f * 1024.0f;
        vec3  nearPos;
        bool  hasShot = false;

        for (int i = 0; i < count; i++) {            
            int armIndex;
            if (wpnCurrent == Weapon::SHOTGUN) {
                if (!rightHand) continue;
                armIndex = 0;
            } else {
                if (!(i ? leftHand : rightHand)) continue;
                armIndex = i;
            }
            Arm *arm = &arms[armIndex];
            
            arm->shotTimer = 0.0f;
            hasShot = true;

            int joint = wpnCurrent == Weapon::SHOTGUN ? 8 : (i ? 11 : 8);

            vec3 p = animation.getJoints(getMatrix(), joint, false).pos;
            vec3 d = arm->rotAbs * vec3(0, 0, 1);
            vec3 t = p + d * (24.0f * 1024.0f) + ((vec3(randf(), randf(), randf()) * 2.0f) - vec3(1.0f)) * 1024.0f;

            int room;
            vec3 hit = trace(getRoomIndex(), p, t, room, false);
            if (target > -1 && checkHit(target, p, hit, hit)) {
                ((Character*)level->entities[target].controller)->hit(wpnGetDamage());
                hit -= d * 64.0f;
                Sprite::add(level, TR::Entity::BLOOD, room, (int)hit.x, (int)hit.y, (int)hit.z, Sprite::FRAME_ANIMATED);
            } else {
                hit -= d * 64.0f;
                Sprite::add(level, TR::Entity::SPARK, room, (int)hit.x, (int)hit.y, (int)hit.z, Sprite::FRAME_RANDOM);

                float dist = (hit - p).length();
                if (dist < nearDist) {
                    nearPos  = hit;
                    nearDist = dist;
                }
            }

            Core::lightPos[1 + armIndex]   = animation.getJoints(getMatrix(), armIndex == 0 ? 10 : 13, false).pos;
            Core::lightColor[1 + armIndex] = FLASH_LIGHT_COLOR;
        }

        if (hasShot) {
            playSound(wpnGetSound(), pos, Sound::Flags::PAN);
            playSound(TR::SND_RICOCHET, nearPos, Sound::Flags::PAN);
        }
    }

    void updateWeapon() {
        if (input & DEATH) {
            arms[0].shotTimer = arms[1].shotTimer = MUZZLE_FLASH_TIME + 1.0f;
            animation.overrideMask = 0;
            target = -1;
            return;
        }

        updateTargets();
        updateOverrides();

        if (Input::down[ik1]) wpnChange(Weapon::PISTOLS);
        if (Input::down[ik2]) wpnChange(Weapon::SHOTGUN);
        if (Input::down[ik3]) wpnChange(Weapon::MAGNUMS);
        if (Input::down[ik4]) wpnChange(Weapon::UZIS);

        if (wpnNext != Weapon::EMPTY && emptyHands()) {
            wpnSet(wpnNext);
            wpnDraw();
            wpnNext = Weapon::EMPTY;
        }

    // apply weapon state changes
        if (input & WEAPON) {
            if (emptyHands())
                wpnDraw();
            else
                wpnHide();
        }

        if (!emptyHands()) {
            bool isRifle = wpnCurrent == Weapon::SHOTGUN;

            for (int i = 0; i < 2; i++) {
                if (arms[i].target > -1 || ((input & ACTION) && target == -1)) {
                    if (arms[i].anim == Weapon::Anim::HOLD)
                        wpnSetAnim(arms[i], wpnState, Weapon::Anim::AIM, 0.0f, 1.0f);
                } else
                    if (arms[i].anim == Weapon::Anim::AIM)
                        arms[i].animation.dir = -1.0f;

                if (isRifle) break;
            }

            for (int i = 0; i < 2; i++) {
                arms[i].animation.update();
                arms[i].shotTimer += Core::deltaTime;

                float intensity = clamp((0.1f - arms[i].shotTimer) * 20.0f, 0.0f, 1.0f);
                Core::lightColor[1 + i] = FLASH_LIGHT_COLOR * vec4(intensity, intensity, intensity, sqrtf(intensity));
            }
 
            if (isRifle)
                animateShotgun();
            else
                animatePistols();
        
            wpnFire(); // make a shot
        }
    }

    void animatePistols() {
        for (int i = 0; i < 2; i++) {
            Arm &arm = arms[i];

            if (!arm.animation.isEnded) continue;                

            if (arm.animation.dir >= 0.0f)
                switch (arm.anim) {
                    case Weapon::Anim::PREPARE   : wpnSetAnim(arm, Weapon::IS_ARMED, Weapon::Anim::UNHOLSTER, arm.animation.time - arm.animation.timeMax, 1.0f); break;
                    case Weapon::Anim::UNHOLSTER : wpnSetAnim(arm, Weapon::IS_ARMED, Weapon::Anim::HOLD, 0.0f, 1.0f, false); break;
                    case Weapon::Anim::AIM       :
                    case Weapon::Anim::FIRE      :
                        if (input & ACTION)
                            wpnSetAnim(arm, Weapon::IS_FIRING, Weapon::Anim::FIRE, arm.animation.time - arm.animation.timeMax, wpnCurrent == Weapon::UZIS ? 2.0f : 1.0f);
                        else
                            wpnSetAnim(arm, Weapon::IS_ARMED, Weapon::Anim::AIM, 0.0f, -1.0f, false);
                        break;
                    default : ;
                };
        
            if (arm.animation.dir < 0.0f)
                switch (arm.anim) {
                    case Weapon::Anim::PREPARE   : wpnSetAnim(arm, Weapon::IS_HIDDEN, Weapon::Anim::NONE,    0.0f, 1.0f, false);    break;
                    case Weapon::Anim::UNHOLSTER : wpnSetAnim(arm, Weapon::IS_HIDDEN, Weapon::Anim::PREPARE, arm.animation.time, -1.0f);  break;
                    case Weapon::Anim::AIM       : wpnSetAnim(arm, Weapon::IS_ARMED,  Weapon::Anim::HOLD,    0.0f, 1.0f, false);    break;
                    default : ;
                };
        }
    }

    void animateShotgun() {
        Arm &arm = arms[0];
        if (arm.animation.dir >= 0.0f) {
            if (arm.animation.isEnded) {
                switch (arm.anim) {
                    case Weapon::Anim::UNHOLSTER : wpnSetAnim(arm, Weapon::IS_ARMED,  Weapon::Anim::HOLD, 0.0f, 1.0f, false); break;
                    case Weapon::Anim::HOLSTER   : wpnSetAnim(arm, Weapon::IS_HIDDEN, Weapon::Anim::NONE, 0.0f, 1.0f, false); break;
                    case Weapon::Anim::AIM       :
                    case Weapon::Anim::FIRE      :
                        if (input & ACTION)
                            wpnSetAnim(arm, Weapon::IS_FIRING, Weapon::Anim::FIRE, arm.animation.time - arm.animation.timeMax, 1.0f);
                        else
                            wpnSetAnim(arm, Weapon::IS_ARMED,  Weapon::Anim::AIM, 0.0f, -1.0f, false);
                        break;
                    default : ;
                }
            } else 
                if (arm.animation.frameIndex != arm.animation.framePrev) {
                    float delta = arm.animation.time / arm.animation.timeMax;
                    switch (arm.anim) {
                        case Weapon::Anim::UNHOLSTER : if (delta >= 0.3f) wpnSetAnim(arm, Weapon::IS_ARMED,  arm.anim, arm.animation.time, 1.0f); break;
                        case Weapon::Anim::HOLSTER   : if (delta >= 0.7f) wpnSetAnim(arm, Weapon::IS_HIDDEN, arm.anim, arm.animation.time, 1.0f); break;
                        default : ;
                    }
                }
        } else
            if (arm.animation.isEnded && arm.anim == Weapon::Anim::AIM)
                wpnSetAnim(arm, Weapon::IS_ARMED, Weapon::Anim::HOLD, 0.0f, 1.0f, false);
    }

    void updateOverrides() {
        // head & chest        
        animation.overrideMask |= BODY_CHEST | BODY_HEAD;

        animation.overrides[ 7] = animation.getJointRot( 7);
        animation.overrides[14] = animation.getJointRot(14);        

    /* TODO: shotgun full body animation
        if (wpnCurrent == Weapon::SHOTGUN) {
            animation.frameA = arms[0].animation.frameA;
            animation.frameB = arms[0].animation.frameB;
            animation.delta  = arms[0].animation.delta;
        }
    */

        if (!emptyHands()) {
            // right arm
            Arm *arm = &arms[0];
            animation.overrides[ 8] = arm->animation.getJointRot( 8);
            animation.overrides[ 9] = arm->animation.getJointRot( 9);
            animation.overrides[10] = arm->animation.getJointRot(10);
            // left arm
            if (wpnCurrent != Weapon::SHOTGUN) arm = &arms[1];
            animation.overrides[11] = arm->animation.getJointRot(11);
            animation.overrides[12] = arm->animation.getJointRot(12);
            animation.overrides[13] = arm->animation.getJointRot(13);

            animation.overrideMask |=  (BODY_ARM_R | BODY_ARM_L);
        } else
            animation.overrideMask &= ~(BODY_ARM_R | BODY_ARM_L);


        lookAt(viewTarget);
        
        if (wpnCurrent == Weapon::SHOTGUN)
            aimShotgun();
        else
            aimPistols();
    }

    void lookAt(int target) { // TODO: character lookAt        
        float speed = 8.0f * Core::deltaTime;
        quat rot;

        bool can = canLookAt();
        // chest
        if (can && aim(target, 7, vec4(-PI * 0.4f, PI * 0.4f, -PI * 0.9f, PI * 0.9f), rot))
            rotChest = rotChest.slerp(quat(0, 0, 0, 1).slerp(rot, 0.5f), speed);
        else 
            rotChest = rotChest.slerp(quat(0, 0, 0, 1), speed);
        animation.overrides[7] = rotChest * animation.overrides[7];

        // head
        if (can && aim(target, 14, vec4(-PI * 0.25f, PI * 0.25f, -PI * 0.5f, PI * 0.5f), rot))
            rotHead = rotHead.slerp(rot, speed);
        else
            rotHead = rotHead.slerp(quat(0, 0, 0, 1), speed);
        animation.overrides[14] = rotHead * animation.overrides[14];
    }

    void aimShotgun() {
        quat rot;
        if (!aim(target, 14, vec4(-PI * 0.4f, PI * 0.4f, -PI * 0.25f, PI * 0.25f), rot, &arms[0].rotAbs))
            arms[0].target = -1;
    }

    void aimPistols() {
        float speed = 8.0f * Core::deltaTime;

        for (int i = 0; i < 2; i++) {
            int joint  = i ? 11 : 8;
            vec4 range = i ? vec4(-PI * 0.4f, PI * 0.4f, -PI * 0.5f, PI * 0.2f) : vec4(-PI * 0.4f, PI * 0.4f, -PI * 0.2f, PI * 0.5f);

            Arm &arm = arms[i];
            quat rot;
            if (!aim(target, joint, range, rot, &arm.rotAbs)) {
                arm.target = -1;
                rot = quat(0, 0, 0, 1);
            }

            float t;
            if (arm.anim == Weapon::Anim::FIRE)
                t = 1.0f;
            else if (arm.anim == Weapon::Anim::AIM)
                t = arm.animation.time / arm.animation.timeMax;
            else
                t = 0.0f;

            arm.rot = arm.rot.slerp(rot, speed);
            animation.overrides[joint] = animation.overrides[joint].slerp(arm.rot * animation.overrides[joint], t);
        }
    }

    void updateTargets() {
        if (emptyHands() || !wpnReady()) {
            target = arms[0].target = arms[1].target = -1;
            return;
        }

        if (!(input & ACTION)) {
            target = getTarget();
            arms[0].target = arms[1].target = target;
        } else
            if (target > -1) {
                TR::Entity &e = level->entities[target];
                vec3 to   = ((Controller*)e.controller)->pos;
                vec3 from = pos - vec3(0, 512, 0);
                arms[0].target = arms[1].target = checkOcclusion(from, to, (to - from).length()) ? target : -1;
            }
    }

    int getTarget() {
        vec3 dir = getDir().normal();
        float dist = TARGET_MAX_DIST;// * TARGET_MAX_DIST;

        int index = -1;
        for (int i = 0; i < level->entitiesCount; i++) {
            TR::Entity &e = level->entities[i];
            if (!e.flags.active || !e.isEnemy()) continue;
            Character *controller = (Character*)e.controller;
            if (controller->health <= 0) continue;

            vec3 p = controller->pos;
            vec3 v = p - pos;
            if (dir.dot(v.normal()) <= 0.5f) continue; // target is out of sight -60..+60 degrees

            float d = v.length();
            if (d < dist && checkOcclusion(pos - vec3(0, 512, 0), p, d) ) {
                index = i;
                dist  = d;
            }
        }

        return index;
    }

    bool checkOcclusion(const vec3 &from, const vec3 &to, float dist) {
        int room;
        vec3 d = trace(getRoomIndex(), from, to, room, false); // check occlusion
        return ((d - from).length() > (dist - 512.0f));
    }

    bool checkHit(int target, const vec3 &from, const vec3 &to, vec3 &point) {
        TR::Entity &e = level->entities[target];
        Box box = ((Controller*)e.controller)->getBoundingBox();
        float t;
        vec3 v = (to - from);
        vec3 dir = v.normal();
        if (box.intersect(from, dir, t) && v.length() > t) {
            point = from + dir * t;
            return true;
        } else
            return false;
    }
    
    virtual void cmdEmpty() {
        wpnHide();
    }

    virtual void hit(int damage, Controller *enemy = NULL) {
        health -= damage;
        if (enemy && health > 0)
            playSound(TR::SND_HIT, pos, Sound::PAN);
    };

    bool waterOut() {
        // TODO: playSound 36
        vec3 dst = pos + getDir() * 32.0f;

        TR::Level::FloorInfo infoCur, infoDst;
        level->getFloorInfo(getEntity().room,  (int)pos.x, (int)pos.y, (int)pos.z, infoCur),
        level->getFloorInfo(infoCur.roomAbove, (int)dst.x, (int)dst.y, (int)dst.z, infoDst);

        int h = int(pos.y - infoDst.floor);

        if (h > 0 && h <= 256 && (state == STATE_SURF_TREAD || animation.setState(STATE_SURF_TREAD)) && animation.setState(STATE_STOP)) { // possibility check
            alignToWall();
            dst.y  -= pos.y - infoDst.floor;
            pos     = dst;  // set new position
            
            specular = LARA_WET_SPECULAR;

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

                if (item.isItem()) {
                    lastPickUp = i;
                    angle.x = 0.0f;
                    pos = ((Controller*)item.controller)->pos;
                    if (stand == STAND_UNDERWATER) { // TODO: lerp to pos/angle
                        pos -= getDir() * 256.0f;
                        pos.y -= 256;
                    }
                    updateEntity();                       
                    return true;
                }
            }
        }
        return false;
    }

    bool checkAngle(TR::angle rotation) {
        return fabsf(shortAngle(rotation, getEntity().rotation)) < PI * 0.25f;
    }

    bool useItem(TR::Entity::Type item, TR::Entity::Type slot) {
        if (item == TR::Entity::NONE) {
            switch (slot) {
                case TR::Entity::HOLE_KEY    : item = TR::Entity::KEY_1;    break;      // TODO: 1-4 
                case TR::Entity::HOLE_PUZZLE : item = TR::Entity::PUZZLE_1; break; 
                default : return false;
            }
        }

        if (inventory.getCount(item) > 0) {
            inventory.remove(item);
            return true;
        }
        return false;
    }

    void checkTrigger() {
        if (actionCommand) return;

        TR::Entity &e = getEntity();
        TR::Level::FloorInfo info;
        level->getFloorInfo(e.room, e.x, e.y, e.z, info);

        if (!info.trigCmdCount) return; // has no trigger
        bool isActive = level->entities[info.trigCmd[0].args].flags.active != 0;
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
                if (!isPressed(ACTION) || state == actionState || !emptyHands())
                    return;
                if (!checkAngle(level->entities[info.trigCmd[0].args].rotation))
                    return;
                break;
            case TR::Level::Trigger::KEY :
                if (level->entities[info.trigCmd[0].args].flags.active)
                    return;
                actionState = level->entities[info.trigCmd[0].args].type == TR::Entity::HOLE_KEY ? STATE_USE_KEY : STATE_USE_PUZZLE;
                if (isActive || !isPressed(ACTION) || state == actionState || !emptyHands())   // TODO: STATE_USE_PUZZLE
                    return;
                if (!checkAngle(level->entities[info.trigCmd[0].args].rotation))
                    return;
                if (animation.canSetState(actionState)) {
                    if (!useItem(TR::Entity::NONE, level->entities[info.trigCmd[0].args].type)) {
                        playSound(TR::SND_NO, pos, Sound::PAN);
                        return;
                    }
                } else
                    return;
                lastPickUp = info.trigCmd[0].args; // TODO: it's not pickup, it's key/puzzle hole
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
        if (!animation.setState(actionState)) return;

        if (info.trigger == TR::Level::Trigger::SWITCH || info.trigger == TR::Level::Trigger::KEY) {
            if (info.trigger == TR::Level::Trigger::KEY)
                level->entities[info.trigCmd[0].args].flags.active = true;

            TR::Entity &p = level->entities[info.trigCmd[0].args];
            angle.y = p.rotation;
            angle.x = 0;
            pos = ((Controller*)p.controller)->pos + vec3(sinf(angle.y), 0, cosf(angle.y)) * (stand == STAND_GROUND ? 384.0f : 128.0f);
            velocity = vec3(0.0f);
            updateEntity();
        }

        // build trigger activation chain
        ActionCommand *actionItem = &actionList[1];

        Controller *controller = this;
        for (int i = 0; i < info.trigCmdCount; i++) {
            if (!controller) {
                LOG("! next activation entity %d has no controller\n", level->entities[info.trigCmd[i].args].type);
                playSound(TR::SND_NO, pos, 0);
                return;
            }

            if (info.trigger == TR::Level::Trigger::KEY && i == 0) continue; // skip key שך puzzle hole

            TR::FloorData::TriggerCommand &cmd = info.trigCmd[i];
            switch (cmd.action) {
                case TR::Action::CAMERA_SWITCH :
                    *actionItem = ActionCommand(entity, cmd.action, cmd.args, (float)info.trigCmd[++i].delay);    // camera switch uses next command for delay timer
                    break;
                default :
                    *actionItem = ActionCommand(entity, cmd.action, cmd.args, info.trigInfo.timer);
            }

            actionItem->next = (i < info.trigCmdCount - 1) ? actionItem + 1 : NULL;
            actionItem++;
        }

        actionList[0].next = &actionList[1];
        actionCommand = &actionList[0];

        if (info.trigger != TR::Level::Trigger::KEY)
            activateNext();
    }

    vec3 getViewPoint() {
        vec3 offset = chestOffset = animation.getJoints(getMatrix(), 7).pos;
        if (stand != STAND_UNDERWATER)
            offset.y -= 256.0f;
        if (!emptyHands())
            offset.y -= 256.0f;

        return offset;
    }

    virtual Stand getStand() {
        if (state == STATE_HANG || state == STATE_HANG_LEFT || state == STATE_HANG_RIGHT) {
            if (input & ACTION)
                return STAND_HANG;
            animation.setAnim(ANIM_HANG_FALL);
            velocity = vec3(0.0f);
            pos.y += 128.0f;
            updateEntity();
            return STAND_AIR;
        }

        if (state == STATE_HANDSTAND || state == STATE_HANG_UP)
            return STAND_HANG;

        if (stand == STAND_ONWATER && state != STATE_DIVE && state != STATE_STOP)
            return stand;

        if (getRoom().flags.water) {
            wpnHide();
            return STAND_UNDERWATER;
        }

        TR::Entity &e = getEntity();
        TR::Level::FloorInfo info;
        level->getFloorInfo(e.room, e.x, e.y, e.z, info);

        if (stand == STAND_SLIDE || (stand == STAND_AIR && velocity.y > 0) || stand == STAND_GROUND) {
            if (e.y + 8 >= info.floor && (abs(info.slantX) > 2 || abs(info.slantZ) > 2)) {
                if (stand == STAND_AIR)
                    playSound(TR::SND_LANDING, pos, Sound::Flags::PAN);
                pos.y = float(info.floor);
                updateEntity();

                if (stand == STAND_GROUND || stand == STAND_AIR)
                    slideStart();
                
                return STAND_SLIDE;
            }
        }

        int extra = stand != STAND_AIR ? 256 : 0;

        if (e.y + extra >= info.floor && !(stand == STAND_AIR && velocity.y < 0)) {
            if (stand != STAND_GROUND) {
                pos.y = float(info.floor);
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

        if ((state == STATE_REACH || state == STATE_UP_JUMP) && (input & ACTION) && emptyHands()) {

            if (state == STATE_REACH && velocity.y < 0.0f)
                return state;

            vec3 p = pos + getDir() * 128.0f;
            TR::Level::FloorInfo info;
        // TODO: use brain
            info.roomAbove = getRoomIndex();
            level->getFloorInfo(info.roomAbove, (int)pos.x, (int)pos.y, (int)pos.z, info);
            if (info.roomAbove == 0xFF)
                info.roomAbove = getRoomIndex();

            do {
                level->getFloorInfo(info.roomAbove, (int)p.x, (int)p.y, (int)p.z, info);
            } while (info.ceiling > p.y - LARA_HANG_OFFSET && info.roomAbove != 0xFF);

            if (abs(int(info.floor - (p.y - LARA_HANG_OFFSET))) < 32) {
                alignToWall();
                pos = pos - getDir() * 96.0f; // TODO: collision wall offset
                pos.y = info.floor + LARA_HANG_OFFSET;
                updateEntity();

                stand = STAND_HANG;
                if (state == STATE_REACH) {
                    return STATE_HANG; // TODO: ANIM_HANG_WALL / ANIM_HANG_NOWALL
                } else
                    return animation.setAnim(ANIM_HANG, -15);
            }
        }

        if (state == STATE_FORWARD_JUMP) {
            if (emptyHands()) {
                if (input & ACTION) return STATE_REACH;
                if ((input & (FORTH | WALK)) == (FORTH | WALK)) return STATE_SWAN_DIVE;
            }
        } else 
            if (state != STATE_SWAN_DIVE && state != STATE_REACH && state != STATE_FALL && state != STATE_UP_JUMP && state != STATE_BACK_JUMP && state != STATE_LEFT_JUMP && state != STATE_RIGHT_JUMP)
                return animation.setAnim(ANIM_FALL);

        return state;
    }

    Block* getBlock() {
        int x = (int)pos.x;
        int y = (int)pos.y;
        int z = (int)pos.z;

        for (int i = 0; i < level->entitiesCount; i++) {
            TR::Entity &e = level->entities[i];
            if (e.isBlock() && e.y == y) {
                int dx = abs(e.x - x);
                int dz = abs(e.z - z);
                if ((dx <= (512 + 128) && dz <= (512 - 128)) ||
                    (dx <= (512 - 128) && dz <= (512 + 128))) {
                    
                    alignToWall();

                    Block *block = (Block*)e.controller;
                    block->angle.y = angle.y;
                    block->updateEntity();
                    return block;
                }
            }
        }
        return NULL;
    }

    virtual int getStateGround() {
        angle.x = 0.0f;

        if ((input & ACTION) && emptyHands() && doPickUp())
            return STATE_PICK_UP;

        if ((input & (FORTH | ACTION)) == (FORTH | ACTION) && (animation.index == ANIM_STAND || animation.index == ANIM_STAND_NORMAL) && emptyHands()) { // TODO: get rid of animation.index 
            vec3 p = pos + getDir() * 64.0f;
            TR::Level::FloorInfo info;
            level->getFloorInfo(getRoomIndex(), (int)p.x, (int)p.y, (int)p.z, info);
            int h = (int)pos.y - info.floor;
            
            int aIndex = animation.index;
            if (info.floor == info.ceiling || h < 256 + 128) {
                ; // do nothing
            } else if (h <= 2 * 256 + 128) {
                aIndex = ANIM_CLIMB_2;
                pos.y  = info.floor + 512.0f;
            } else if (h <= 3 * 256 + 128) {
                aIndex = ANIM_CLIMB_3;
                pos.y  = info.floor + 768.0f;
            } else if (h <= 7 * 256 + 128)
                aIndex = ANIM_CLIMB_JUMP;            

            if (aIndex != animation.index) {
                alignToWall();
                return animation.setAnim(aIndex);
            }
        }

        if ( (input & (FORTH | BACK)) == (FORTH | BACK) && (state == STATE_STOP || state == STATE_RUN) )
            return animation.setAnim(ANIM_STAND_ROLL_BEGIN);

        // ready to jump
        if (state == STATE_COMPRESS) {
            switch (input & (RIGHT | LEFT | FORTH | BACK)) {
                case RIGHT  : return STATE_RIGHT_JUMP;
                case LEFT   : return STATE_LEFT_JUMP;
                case FORTH  : return STATE_FORWARD_JUMP;
                case BACK   : return STATE_BACK_JUMP;
                default     : return STATE_UP_JUMP;
            }
        }

        // jump button is pressed
        if (input & JUMP) {   
            if ((input & FORTH) && state == STATE_FORWARD_JUMP)
                return STATE_RUN;
            if (state == STATE_RUN)
                return STATE_FORWARD_JUMP;
            if (animation.index == ANIM_SLIDE_BACK) // TODO: animation index? %)
                return STATE_SLIDE_BACK;                
            return STATE_COMPRESS;
        }
        
        // walk button is pressed
        if (input & WALK) {      
            if (input & FORTH) return STATE_WALK;
            if (input & BACK)  return STATE_BACK;
            if (input & LEFT)  return STATE_STEP_LEFT;
            if (input & RIGHT) return STATE_STEP_RIGHT;
            return STATE_STOP;
        } 

        if ((input & ACTION) && emptyHands()) {
            if (state == STATE_PUSH_PULL_READY && (input & (FORTH | BACK))) {
                int pushState = (input & FORTH) ? STATE_PUSH_BLOCK : STATE_PULL_BLOCK;
                Block *block = getBlock();                
                if (animation.canSetState(pushState) && block->doMove((input & FORTH) != 0)) {
                    alignToWall(128.0f);
                    return pushState;
                }
            }

            if (state == STATE_PUSH_PULL_READY || getBlock())
                return STATE_PUSH_PULL_READY;
        }

        // only dpad buttons pressed
        if (input & FORTH) return STATE_RUN;
        if (input & BACK)  return STATE_FAST_BACK;
        if (input & (LEFT | RIGHT)) {
            if (state == STATE_FAST_TURN)
                return state;

            if (input & LEFT)  return (state == STATE_TURN_LEFT  && animation.prev == animation.index) ? STATE_FAST_TURN : STATE_TURN_LEFT;
            if (input & RIGHT) return (state == STATE_TURN_RIGHT && animation.prev == animation.index) ? STATE_FAST_TURN : STATE_TURN_RIGHT;
        }

        return STATE_STOP;
    }

    void slideStart() {
        TR::Entity &e = getEntity();

        if (state != STATE_SLIDE && state != STATE_SLIDE_BACK) {
            TR::Level::FloorInfo info;
            level->getFloorInfo(e.room, e.x, e.y, e.z, info);

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
            animation.setAnim(aIndex);
        }
    }

    virtual int getStateSlide() {
        if (input & JUMP)
            return state == STATE_SLIDE ? STATE_FORWARD_JUMP : STATE_BACK_JUMP;
        return state;
    }

    virtual int getStateHang() {
        if (input & LEFT)  return STATE_HANG_LEFT;
        if (input & RIGHT) return STATE_HANG_RIGHT;
        if (input & FORTH) {
            // possibility check
            TR::Level::FloorInfo info;
            vec3 p = pos + getDir() * 128.0f;
            level->getFloorInfo(getRoomIndex(), (int)p.x, (int)p.y, (int)p.z, info);
            if (info.floor - info.ceiling >= 768)
                return (input & WALK) ? STATE_HANDSTAND : STATE_HANG_UP;          
        }
        return STATE_HANG;
    }

    virtual int getStateUnderwater() {
        if (input == ACTION && doPickUp())
            return STATE_PICK_UP;

        if (state == STATE_FORWARD_JUMP || state == STATE_UP_JUMP || state == STATE_BACK_JUMP || state == STATE_LEFT_JUMP || state == STATE_RIGHT_JUMP || state == STATE_FALL || state == STATE_REACH) {
            Sprite::add(level, TR::Entity::Type::WATER_SPLASH, getRoomIndex(), (int)pos.x, (int)pos.y, (int)pos.z);
            return animation.setAnim(ANIM_WATER_FALL); // TODO: wronng animation
        }
        
        if (state == STATE_SWAN_DIVE) {
            angle.x = -PI * 0.5f;
            return STATE_DIVE;
        }
            
        if (input & JUMP) return STATE_SWIM;

        return (state == STATE_SWIM || velocity.y > GLIDE_SPEED) ? STATE_GLIDE : STATE_TREAD;
    }

    virtual int getStateOnwater() {
        angle.x = 0.0f;  

        if (state == STATE_WATER_OUT) return state;

        if (state != STATE_SURF_TREAD && state != STATE_SURF_LEFT && state != STATE_SURF_RIGHT && state != STATE_SURF_SWIM && state != STATE_SURF_BACK && state != STATE_STOP)
            return animation.setAnim(ANIM_TO_ONWATER);

        if (input & FORTH) {
            if (input & JUMP) { 
                angle.x = -PI * 0.25f;
                return animation.setAnim(ANIM_TO_UNDERWATER);
            }

            if ((input & ACTION) && waterOut()) return state;

            return STATE_SURF_SWIM;
        }
        
        if (input & BACK)  return STATE_SURF_BACK;
        if (input & WALK) {
            if (input & LEFT)  return STATE_SURF_LEFT;
            if (input & RIGHT) return STATE_SURF_RIGHT;         
        }
        return STATE_SURF_TREAD;
    }

    virtual int getStateDeath() {
        return STATE_DEATH;
    }

    virtual int getStateDefault() {
        if (state == STATE_DIVE || (state == STATE_RUN && (input & JUMP)) ) return state;
        switch (stand) {
            case STAND_GROUND     : return STATE_STOP;
            case STAND_HANG       : return STATE_HANG;
            case STAND_ONWATER    : return STATE_SURF_TREAD;
            case STAND_UNDERWATER : return STATE_TREAD;
            default : ;
        }
        return STATE_FALL;
    }

    virtual int getInput() { // TODO: updateInput
        input = Character::getInput();
        if (input & DEATH) return input;

        int &p = Input::joy.POV;

    #ifndef LEVEL_EDITOR
        if (Input::down[ikW]) input |= FORTH;
        if (Input::down[ikD]) input |= RIGHT;
        if (Input::down[ikS]) input |= BACK;
        if (Input::down[ikA]) input |= LEFT;
    #endif

        if (Input::down[ikUp]    || p == 8 || p == 1 || p == 2)             input |= FORTH;
        if (Input::down[ikRight] || p == 2 || p == 3 || p == 4)             input |= RIGHT;
        if (Input::down[ikDown]  || p == 4 || p == 5 || p == 6)             input |= BACK;
        if (Input::down[ikLeft]  || p == 6 || p == 7 || p == 8)             input |= LEFT;
        if (Input::down[ikJoyB])                                            input  = FORTH | BACK; // roll
        if (Input::down[ikJoyRT] || Input::down[ikX])                       input  = WALK | RIGHT; // step right
        if (Input::down[ikJoyLT] || Input::down[ikZ])                       input  = WALK | LEFT;  // step left
        if (Input::down[ikSpace] || Input::down[ikJoyX])                    input |= JUMP;
        if (Input::down[ikShift] || Input::down[ikJoyLB])                   input |= WALK;
        if (Input::down[ikE] || Input::down[ikCtrl] || Input::down[ikJoyA]) input |= ACTION;
        if (Input::down[ikQ] || Input::down[ikAlt]  || Input::down[ikJoyY]) input |= WEAPON;
        return input;
    }

    virtual void doCustomCommand(int curFrame, int prevFrame) {
        switch (state) {
            case STATE_PICK_UP : {
                TR::Entity &item = level->entities[lastPickUp];
                if (!item.flags.invisible) {
                    int pickupFrame = stand == STAND_GROUND ? PICKUP_FRAME_GROUND : PICKUP_FRAME_UNDERWATER;                
                    if (animation.isFrameActive(pickupFrame)) {
                        item.flags.invisible = true;
                        inventory.add(item.type, 1);
                    }
                }
                break;
            }
            case STATE_USE_PUZZLE : {
                TR::Entity &item = level->entities[lastPickUp];
                if (animation.isFrameActive(PUZZLE_FRAME))
                    ((Controller*)item.controller)->meshSwap(0, level->extra.puzzleSet);
                break;
            }
        }
    }

    virtual void updateAnimation(bool commands) {
        Controller::updateAnimation(commands);
        updateWeapon();
        if (stand == STAND_UNDERWATER)
            specular = 0.0f;
        else
            if (specular > 0.0f)
                specular = max(0.0f, specular - LARA_WET_TIMER * Core::deltaTime);
    }

    virtual void updateVelocity() {
        checkTrigger();

    // get turning angle
        float w = (input & LEFT) ? -1.0f : ((input & RIGHT) ? 1.0f : 0.0f);

        if (state == STATE_SWIM || state == STATE_GLIDE)
            w *= TURN_WATER_FAST;
        else if (state == STATE_TREAD || state == STATE_SURF_TREAD || state == STATE_SURF_SWIM || state == STATE_SURF_BACK)
            w *= TURN_WATER_SLOW;
        else if (state == STATE_RUN)
            w  *= sign(w) != sign(tilt) ? 0.0f : w * TURN_FAST * tilt / TILT_MAX;
        else if (state == STATE_FAST_TURN)
            w *= TURN_FAST;
        else if (state == STATE_FAST_BACK)
            w *= TURN_FAST_BACK;
        else if (state == STATE_TURN_LEFT || state == STATE_TURN_RIGHT || state == STATE_WALK)
            w *= TURN_NORMAL;   
        else if (state == STATE_FORWARD_JUMP || state == STATE_BACK)
            w *= TURN_SLOW;
        else 
            w = 0.0f;

        if (w != 0.0f) {
            w *= Core::deltaTime;
            angle.y += w; 
            velocity = velocity.rotateY(-w); // in-air velocity rotation control
        }

    // pitch (underwater only)
        if (stand == STAND_UNDERWATER && (input & (FORTH | BACK)) ) {
            angle.x += ((input & FORTH) ? -TURN_WATER_SLOW : TURN_WATER_SLOW) * 0.5f * Core::deltaTime;
            angle.x = clamp(angle.x, -PI * 0.5f, PI * 0.5f);
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

        switch (stand) {
            case STAND_AIR :
                velocity.y += GRAVITY * Core::deltaTime;
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
                        speed = animation.getSpeed();
                        if (animation.index == ANIM_STAND_ROLL_END) 
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
                        level->getFloorInfo(e.room, (int)p.x, (int)p.y, (int)p.z, info);
                        if (info.roomAbove != 0xFF && info.floor >= e.y - LARA_HANG_OFFSET)
                            level->getFloorInfo(info.roomAbove, (int)p.x, (int)p.y, (int)p.z, info);
                    } else
                        level->getFloorInfo(e.room, e.x, e.y, e.z, info);

                    vec3 v(sinf(angleExt), 0.0f, cosf(angleExt));
                    velocity = info.getSlant(v) * speed;
                }
                break;
            }
            case STAND_UNDERWATER : {
                float speed = 0.0f;
                if (animation.index == ANIM_TO_UNDERWATER)
                    speed = 15.0f;
                if (state == STATE_SWIM)
                    speed = SWIM_SPEED;

                if (speed != 0.0f)
                    velocity = vec3(angle.x, angle.y) * speed;
                else 
                    velocity = velocity - velocity * min(1.0f, Core::deltaTime * 2.0f);
                // TODO: apply flow velocity
                break;
            }
        }
    }

    virtual void updatePosition() { // TODO: sphere / bbox collision
        updateTilt(state == STATE_RUN, TILT_SPEED, TILT_MAX);

        if (velocity.length() >= 0.001f) 
            move();
    }

    void move() {
        TR::Entity &e = getEntity();
        TR::Level::FloorInfo info;
        
        float f, c;
        bool canPassGap = true;
        /*
        if (velocity != 0.0f) {
            vec3 dir = velocity.normal() * 128.0f;
            vec3 p = pos + dir;
            level->getFloorInfo(e.room, (int)p.x, (int)p.y, (int)p.z, info);
            if (info.floor < p.y - (256 + 128)  || info.ceiling > p.y - 768) { // wall
                vec3 axis   = dir.axisXZ();
                vec3 normal = (p - vec3(int(p.x / 1024.0f) * 1024.0f + 512.0f, p.y, int(p.z / 1024.0f) * 1024.0f + 512.0f)).axisXZ();
                LOG("%f %f = %f %f = %f\n", axis.x, axis.z, normal.x, normal.z, abs(axis.dot(normal)));
                if (abs(axis.dot(normal)) > EPS) {
                    canPassGap = false;
                } else {
                    updateEntity();
                    checkRoom();
                    return;
                }
            }
        }
        */
        vec3 offset = velocity * Core::deltaTime * 30.0f;

        vec3 p = pos;
        pos = pos + offset;

        if (canPassGap) {
            level->getFloorInfo(e.room, (int)pos.x, (int)pos.y, (int)pos.z, info);
            canPassGap = (info.floor - info.ceiling) >= (stand == STAND_GROUND ? 768 : 512);
        }

        f = info.floor - pos.y;
        c = pos.y - info.ceiling;
        /*
        TR::Animation *anim  = animation;
        Box eBox = Box(pos - vec3(128.0f, 0.0f, 128.0f), pos + vec3(128.0, getHeight(), 128.0f)); // getBoundingBox();
        // check static meshes in the room
        if (canPassGap) {
            TR::Room &r = level->rooms[e.room];
            for (int i = 0; i < r.meshesCount; i++) {
                TR::Room::Mesh &m = r.meshes[i];
                TR::StaticMesh *sm = level->getMeshByID(m.meshID);
                if (sm->flags != 2) continue; // no have collision box

                Box  mBox;
                vec3 offset(m.x, m.y, m.z);
                sm->getBox(true, m.rotation, mBox);
                mBox.min += offset;
                mBox.max += offset;

                if (eBox.intersect(mBox)) {
                    canPassGap = false;
                    break;
                }
            }
        }
        
        // check entities in the room
        if (canPassGap)
            for (int i = 0; i < level->entitiesCount; i++) 
                if (i != entity && level->entities[i].room == e.room && level->entities[i].controller) {
                    Box mBox = ((Controller*)level->entities[i].controller)->getBoundingBox();
                    if (eBox.intersect(mBox)) {
                        canPassGap = false;
                        break;
                    }
                }
        */
        if (canPassGap)
            switch (stand) {
                case STAND_AIR : {
                    f = info.floor - (pos.y + animation.frameA->box.maxY);
                    c = (pos.y + animation.frameA->box.minY) - info.ceiling;
                    canPassGap = f >= -256;
                    if (canPassGap && c < 0) {
                        if (c > -256) { // position correction for ceiling step (less than 256)
                            pos.y -= c;
                            if (velocity.y < 0.0f) {
                                velocity.y = 0.0f;
                            }
                        } else
                            canPassGap = false;
                    }
                    break;
                }
                case STAND_GROUND : {                    
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
                        level->getFloorInfo(info.roomAbove != 0xFF ? info.roomAbove : getRoomIndex(), (int)d.x, (int)d.y, (int)d.z, info);
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

        bool isLeftFoot = animation.framesCount / 2 > animation.frameIndex;

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
                        animation.setAnim(ANIM_SMASH_JUMP);
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
                        animation.setAnim(isLeftFoot ? ANIM_SMASH_RUN_LEFT : ANIM_SMASH_RUN_RIGHT);
                    else if (stand == STAND_HANG)
                        animation.setAnim(ANIM_HANG, -21);
                    else if (state != STATE_ROLL_1 && state != STATE_ROLL_2)
                        animation.setAnim(ANIM_STAND);
                    velocity.x = velocity.z = 0.0f;
                    break;
                default : ;// no smash animation
            }                     
        } else
            if (stand == STAND_GROUND) {
                int h = int(info.floor - pos.y);

                if (h >= 256 && state == STATE_FAST_BACK) {
                    stand = STAND_AIR;
                    animation.setAnim(ANIM_FALL);
                } else if (h >= 128 && (state == STATE_WALK || state == STATE_BACK)) { // descend
                    if (state == STATE_WALK) animation.setAnim(isLeftFoot ? ANIM_WALK_DESCEND_LEFT : ANIM_WALK_DESCEND_RIGHT);
                    if (state == STATE_BACK) animation.setAnim(isLeftFoot ? ANIM_BACK_DESCEND_LEFT : ANIM_BACK_DESCEND_RIGHT);
                    pos.y = float(info.floor);
                } else if (h > -1.0f) {
                    pos.y = min((float)info.floor, pos.y += DESCENT_SPEED * Core::deltaTime);
                } else if (h > -128) {
                    pos.y = float(info.floor);
                } else if (h >= -(256 + 128) && (state == STATE_RUN || state == STATE_WALK)) { // ascend
                    if (state == STATE_RUN)  animation.setAnim(isLeftFoot ? ANIM_RUN_ASCEND_LEFT  : ANIM_RUN_ASCEND_RIGHT);
                    if (state == STATE_WALK) animation.setAnim(isLeftFoot ? ANIM_WALK_ASCEND_LEFT : ANIM_WALK_ASCEND_RIGHT);
                    pos.y = float(info.floor);
                } else
                    pos.y = float(info.floor);
            }

        updateEntity();
        checkRoom();
    }

    virtual void applyFlow(TR::Camera &sink) {
        if (stand != STAND_UNDERWATER && stand != STAND_ONWATER) return;
        vec3 v(0.0f);
        v.x = (float)sign((sink.x / 1024 - (int)pos.x / 1024));
        v.z = (float)sign((sink.z / 1024 - (int)pos.z / 1024));
        velocity = v * (sink.speed * 8.0f);
    }


    void renderMuzzleFlash(MeshBuilder *mesh, const Basis &basis, const vec3 &offset, float time) {
        ASSERT(level->extra.muzzleFlash);
        if (time > MUZZLE_FLASH_TIME) return;
        float alpha = min(1.0f, (0.1f - time) * 20.0f);
        float lum   = 3.0f;
        Basis b(basis);
        b.rotate(quat(vec3(1, 0, 0), -PI * 0.5f));
        b.translate(offset);
        
        Core::active.shader->setParam(uColor, vec4(lum, lum, lum, alpha));
        Core::active.shader->setParam(uBasis, b);
        mesh->renderModel(level->extra.muzzleFlash);
    }

    virtual void render(Frustum *frustum, MeshBuilder *mesh) {
        Controller::render(frustum, mesh);
        chestOffset = animation.getJoints(getMatrix(), 7).pos; // TODO: move to update func

        if (wpnCurrent != Weapon::SHOTGUN && Core::pass != Core::passShadow && (arms[0].shotTimer < MUZZLE_FLASH_TIME || arms[1].shotTimer < MUZZLE_FLASH_TIME)) {
            mat4 matrix = getMatrix();
            Core::active.shader->setParam(uType, Shader::FLASH);
            renderMuzzleFlash(mesh, animation.getJoints(matrix, 10, true), vec3(-10, -50, 150), arms[0].shotTimer);
            renderMuzzleFlash(mesh, animation.getJoints(matrix, 13, true), vec3( 10, -50, 150), arms[1].shotTimer);
            Core::active.shader->setParam(uType, Shader::ENTITY);
        }
    }
};

#endif