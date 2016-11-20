#ifndef H_ENEMY
#define H_ENEMY

#include "character.h"

struct Enemy : Character {

    Enemy(TR::Level *level, int entity, int health) : Character(level, entity, health) {}

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
        TR::Animation *anim = animation;
        float speed = anim->speed + anim->accel * (animation.time * 30.0f);
        velocity = getDir() * speed;
    }

    virtual void updatePosition() {
        if (!getEntity().flags.active) return;
        vec3 p = pos;
        pos += velocity * Core::deltaTime * 30.0f;
        TR::Level::FloorInfo info;
        level->getFloorInfo(getRoomIndex(), (int)pos.x, (int)pos.z, info, true);
        if (pos.y - info.floor > 1024) {
            pos = p;
            return;
        }
        if (stand == STAND_GROUND)
            pos.y = float(info.floor);
        updateEntity();
        checkRoom();
    }

    void setOverrides(bool active, int chest, int head) {
        int mask = 0;
        if (active && head  > -1) {
            animation.overrides[head] = animation.getJointRot(head);
            animation.overrideMask |=  (1 << head);
        } else
            animation.overrideMask &= ~(1 << head);

        if (active && chest  > -1) {
            animation.overrides[chest] = animation.getJointRot(chest);
            animation.overrideMask |=  (1 << chest);
        } else
            animation.overrideMask &= ~(1 << chest);
    }

    void lookAt(int target, int chest, int head) {
        float speed = 8.0f * Core::deltaTime;
        quat rot;

        if (chest > -1) {
            if (aim(target, chest, vec4(-PI * 0.8f, PI * 0.8f, -PI * 0.75f, PI * 0.75f), rot))
                rotChest = rotChest.slerp(quat(0, 0, 0, 1).slerp(rot, 0.5f), speed);
            else 
                rotChest = rotChest.slerp(quat(0, 0, 0, 1), speed);
            animation.overrides[chest] = rotChest * animation.overrides[chest];
        }

        if (head > -1) {
            if (aim(target, head, vec4(-PI * 0.25f, PI * 0.25f, -PI * 0.5f, PI * 0.5f), rot))
                rotHead = rotHead.slerp(rot, speed);
            else
                rotHead = rotHead.slerp(quat(0, 0, 0, 1), speed);
            animation.overrides[head] = rotHead * animation.overrides[head];
        }
    }

    virtual int getInput() {
        if (target > -1) {
            vec3 a = (((Controller*)level->entities[target].controller)->pos - pos).normal();
            vec3 b = getDir();
            vec3 n = vec3(0, 1, 0);
            float d = atan2(b.cross(a).dot(n), a.dot(b));
            if (fabsf(d) > 0.01f)
                return d < 0 ? LEFT : RIGHT;
        }
        return 0;
    }
};


#define WOLF_TURN_FAST   PI
#define WOLF_TURN_SLOW   (PI / 3.0f)
#define WOLF_TILT_MAX    (PI / 6.0f)
#define WOLF_TILT_SPEED  WOLF_TILT_MAX


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

    Wolf(TR::Level *level, int entity) : Enemy(level, entity, 100) {}

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
                case STATE_RUN  : return animation.setAnim(ANIM_DEATH_RUN);
                case STATE_JUMP : return animation.setAnim(ANIM_DEATH_JUMP);
                default         : return animation.setAnim(ANIM_DEATH);
            }
        }

        TR::Entity &e = getEntity();
        if (!e.flags.active)
            return (state == STATE_STOP || state == STATE_SLEEP) ? STATE_SLEEP : STATE_STOP;

        switch (state) {
            case STATE_SLEEP    : return STATE_STOP;
            case STATE_STOP     : return STATE_HOWL;
            case STATE_GROWL    : return randf() > 0.5f ? STATE_STALKING : STATE_RUN;
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
            if (input & LEFT) w = -w;
                
            if (w != 0.0f) {
                w *= Core::deltaTime;
                angle.y += w; 
                velocity = velocity.rotateY(-w);
            }
        } else
            velocity = vec3(0.0f);
    }

    virtual void updatePosition() {
        updateTilt(state == STATE_RUN, WOLF_TILT_SPEED, WOLF_TILT_MAX);

        if (state == STATE_DEATH) {
            animation.overrideMask = 0;
            return;
        }
        Enemy::updatePosition();
        setOverrides(state == STATE_STALKING || state == STATE_RUN, JOINT_CHEST, JOINT_HEAD);
        lookAt(target, JOINT_CHEST, JOINT_HEAD);
    }
};


struct Bear : Enemy {

    enum {
        STATE_STOP   = 1,
    };

    Bear(TR::Level *level, int entity) : Enemy(level, entity, 100) {}

    virtual int getStateGround() {
        return state;
    }
};


struct Bat : Enemy {

    enum {
        STATE_AWAKE  = 1,
        STATE_FLY    = 2,
    };

    Bat(TR::Level *level, int entity) : Enemy(level, entity, 100) {}

    virtual Stand getStand() {
        return STAND_AIR;
    }

    virtual int getStateAir() {
        animation.time = 0.0f;
        return STATE_AWAKE;
    }
};

#endif
