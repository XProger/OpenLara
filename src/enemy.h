#ifndef H_ENEMY
#define H_ENEMY

#include "controller.h"

struct Enemy : Controller {
    int target;
    quat rotHead, rotChest;
    int baseAnim;

    Enemy(TR::Level *level, int entity) : Controller(level, entity), target(-1) {
        initAnimOverrides();
        rotHead  = rotChest = quat(0, 0, 0, 1);
        baseAnim = animIndex;
    }

    virtual Stand getStand() {
        return STAND_GROUND;
    }

    virtual void hit(int damage) {
        health -= damage;
    };

    virtual bool activate(ActionCommand *cmd) {
        Controller::activate(cmd);

        getEntity().flags.active = true;        
        activateNext();

        for (int i = 0; i < level->entitiesCount; i++)
            if (level->entities[i].type == TR::Entity::LARA) {
                target = i;
                break;
            }

        return true;
    }

    virtual void updateVelocity() {
        TR::Animation *anim = &level->anims[animIndex];
        float speed = anim->speed + anim->accel * (animTime * 30.0f);
        velocity = getDir() * speed;
    }

    virtual void move() {
        if (!getEntity().flags.active) return;
        vec3 p = pos;
        pos += velocity * Core::deltaTime * 30.0f;
        TR::Level::FloorInfo info;
        level->getFloorInfo(getRoomIndex(), (int)pos.x, (int)pos.z, info);
        if (pos.y - info.floor > 1024) {
            pos = p;
            return;
        }
        if (stand == STAND_GROUND)
            pos.y = info.floor;
        updateEntity();
        checkRoom();
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

    void setOverrides(bool active, int chest, int head) {
        int mask = 0;
        if (head  > -1) mask |= (1 << head);
        if (chest > -1) mask |= (1 << chest);

        if (active)
            animOverrideMask |= mask;
        else
            animOverrideMask &= ~mask;

        TR::AnimFrame *frameA, *frameB;
        float t;

        getFrames(&frameA, &frameB, t, animIndex, animTime, true);
        animOverrides[chest] = lerpFrames(frameA, frameB, t, chest);
        animOverrides[head]  = lerpFrames(frameA, frameB, t, head);
    }

    void lookAt(int target, int chest, int head) {
        float speed = 8.0f * Core::deltaTime;
        quat rot;

        if (chest > -1) {
            if (aim(target, chest, vec4(-PI * 0.4f, PI * 0.4f, -PI * 0.75f, PI * 0.75f), rot))
                rotChest = rotChest.slerp(quat(0, 0, 0, 1).slerp(rot, 0.5f), speed);
            else 
                rotChest = rotChest.slerp(quat(0, 0, 0, 1), speed);
            animOverrides[chest] = rotChest * animOverrides[chest];
        }

        if (head > -1) {
            if (aim(target, head, vec4(-PI * 0.25f, PI * 0.25f, -PI * 0.5f, PI * 0.5f), rot))
                rotHead = rotHead.slerp(rot, speed);
            else
                rotHead = rotHead.slerp(quat(0, 0, 0, 1), speed);
            animOverrides[head] = rotHead * animOverrides[head];
        }
    }

    virtual int getInputMask() {
        if (target > -1) {
            vec3 v = (((Controller*)level->entities[target].controller)->pos - pos).normal();
            float d = atan2(v.x, v.z) - angle.y;
            if (fabsf(d) > 0.01f)
                return d < 0 ? LEFT : RIGHT;
        }
        return 0;
    }
};


#define WOLF_TURN_FAST   PI
#define WOLF_TURN_SLOW   (PI / 3.0f)

struct Wolf : Enemy {

    enum {
        ANIM_DEATH      = 20,
        ANIM_DEATH_RUN  = 21,
        ANIM_DEATH_JUMP = 22,
    };

    enum {
        STATE_STOP     = 1,
        STATE_WALK     = 2,
        STATE_RUN      = 3,
        STATE_STALKING = 5,
        STATE_JUMP     = 6,
        STATE_HOWL     = 7,
        STATE_SLEEP    = 8,
        STATE_GROWL    = 9,
        STATE_10       = 10, // WTF?
        STATE_DEATH    = 11,
        STATE_ATTACK   = 12,
    };

    enum {
        JOINT_CHEST    = 2,
        JOINT_HEAD     = 3
    };

    Wolf(TR::Level *level, int entity) : Enemy(level, entity) {}

    virtual int getStateGround() {
        // STATE_SLEEP     -> STATE_STOP
        // STATE_STOP      -> STATE_WALK, STATE_HOWL, STATE_SLEEP, STATE_GROWL
        // STATE_WALK      -> NULL
        // STATE_RUN       -> STaTE_JUMP, STATe_GROWL, STATE_10
        // STATE_STALKING  -> STATE_RUN, STATE_GROWL, STATE_BITING
        // STATE_JUMP      -> STATE_RUN
        // STATE_GROWL     -> STATE_STOP, STATE_RUN, STATE_STALKING, STATE_HOWL, STATE_ATTACK 
        // STATE_BITING    -> NULL
        if (state == STATE_DEATH) return state;

        if (health <= 0) {
            switch (state) {
                case STATE_RUN  : return setAnimation(baseAnim + ANIM_DEATH_RUN);
                case STATE_JUMP : return setAnimation(baseAnim + ANIM_DEATH_JUMP);
                default         : return setAnimation(baseAnim + ANIM_DEATH);
            }
        }

        TR::Entity &e = getEntity();
        if (!e.flags.active)
            return (state == STATE_STOP || state == STATE_SLEEP) ? STATE_SLEEP : STATE_STOP;

        switch (state) {
            case STATE_SLEEP    : return STATE_STOP;
            case STATE_STOP     : return STATE_HOWL;
            case STATE_GROWL    : return STATE_STALKING;
            case STATE_STALKING : if (health < 70) return STATE_RUN; break;
        }

        if (target > -1 && (state == STATE_STALKING || state == STATE_RUN)) {
            vec3 v = ((Controller*)level->entities[target].controller)->pos - pos;
            float d = v.length();
            if (state == STATE_STALKING && d < 512)
                return STATE_ATTACK;
            if (state == STATE_RUN && d > 512 && d < 1024)
                return STATE_JUMP;
        }

        if (state == STATE_JUMP)
            return STATE_RUN;

        return state;
    }

    virtual void updateState() {
        Enemy::updateState();
        float w = 0.0f;
        if (state == STATE_RUN || state == STATE_STALKING) {
            w = state == STATE_RUN ? WOLF_TURN_FAST : WOLF_TURN_SLOW;
            if (mask & LEFT) w = -w;
                
            if (w != 0.0f) {
                w *= Core::deltaTime;
                angle.y += w; 
                velocity = velocity.rotateY(-w);
            }
        } else
            velocity = vec3(0.0f);
    }

    virtual void move() {
        if (state == STATE_DEATH) {
            animOverrideMask = 0;
            return;
        }
        Enemy::move();
        setOverrides(state == STATE_STALKING || state == STATE_RUN, JOINT_CHEST, JOINT_HEAD);
        lookAt(target, JOINT_CHEST, JOINT_HEAD);
    }
};


struct Bear : Enemy {

    enum {
        STATE_STOP   = 1,
    };

    Bear(TR::Level *level, int entity) : Enemy(level, entity) {}

    virtual int getStateGround() {
        return state;
    }
};


struct Bat : Enemy {

    enum {
        STATE_AWAKE  = 1,
        STATE_FLY    = 2,
    };

    Bat(TR::Level *level, int entity) : Enemy(level, entity) {}

    virtual Stand getStand() {
        return STAND_AIR;
    }

    virtual int getStateAir() {
        animTime = 0.0f;
        return STATE_AWAKE;
    }
};

#endif
