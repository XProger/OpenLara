#ifndef H_ENEMY
#define H_ENEMY

#include "character.h"

struct Enemy : Character {
    bool bitten;

    Enemy(IGame *game, int entity, int health) : Character(game, entity, health), bitten(false) {}

    virtual bool activate(ActionCommand *cmd) {
    #ifdef LEVEL_EDITOR
        return true;
    #endif

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
        velocity = getDir() * animation.getSpeed();
    }

    virtual void updatePosition() {
        if (!getEntity().flags.active) return;
        vec3 p = pos;
        pos += velocity * Core::deltaTime * 30.0f;
        TR::Level::FloorInfo info;
        level->getFloorInfo(getRoomIndex(), (int)pos.x, (int)pos.y, (int)pos.z, info);
        if (pos.y - info.floor > 1024) {
            pos = p;
            return;
        }

        switch (stand) {
            case STAND_GROUND : pos.y = float(info.floor); break;
            case STAND_AIR    : pos.y = clamp(pos.y, float(info.ceiling), float(info.floor)); break;
            default : ;
        }

        updateEntity();
        checkRoom();
    }

    void setOverrides(bool active, int chest, int head) {
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

    bool getTargetInfo(int height, vec3 *pos, float *angleX, float *angleY, float *dist) {
        if (target == -1) return false;
        Character *character  = (Character*)level->entities[target].controller;
        if (character->health <= 0) return false;

        vec3 p = character->pos;
        p.y -= height;
        if (pos) *pos = p;
        vec3 a = p - this->pos;
        if (dist) *dist = a.length();

        if (angleX || angleY) {
            a = a.normal();
            vec3 b = getDir();
            vec3 n = vec3(0, 1, 0);
            if (angleX) *angleX = 0.0f;
            if (angleY) *angleY = atan2f(b.cross(a).dot(n), a.dot(b));
        }
        return true;
    }

    int turn(float delta, float speed) {
        speed *= Core::deltaTime;
        decrease(delta, angle.y, speed);
        if (speed != 0.0f) {
            velocity = velocity.rotateY(-speed);
            return speed < 0 ? LEFT : RIGHT;
        }
        return 0;
    }

    int lift(float delta, float speed) {
        speed *= Core::deltaTime;
        decrease(delta, pos.y, speed);
        if (speed != 0.0f) {
            updateEntity();
            return speed < 0 ? FORTH : BACK;
        }
        return 0;
    }

    void bite(const vec3 &pos, int damage) {
        if (bitten) return;
        bitten = true;
        ASSERT(target > -1);
        Character *c = (Character*)level->entities[target].controller;
        c->hit(damage, this);
        Sprite::add(game, TR::Entity::BLOOD, c->getRoomIndex(), (int)pos.x, (int)pos.y, (int)pos.z, Sprite::FRAME_ANIMATED);
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
        STATE_TURN     = 10,
        STATE_DEATH    = 11,
        STATE_BITE     = 12,
    };

    enum {
        JOINT_CHEST    = 2,
        JOINT_HEAD     = 3
    };

    Wolf(IGame *game, int entity) : Enemy(game, entity, 6) {}

    virtual int getStateGround() {
        TR::Entity &e = getEntity();
        if (!e.flags.active)
            return (state == STATE_STOP || state == STATE_SLEEP) ? STATE_SLEEP : STATE_STOP;

        switch (state) {
            case STATE_SLEEP    : return target > -1 ? STATE_STOP : state;
            case STATE_STOP     : return target > -1 ? STATE_HOWL : STATE_SLEEP;
            case STATE_HOWL     : return state;
            case STATE_GROWL    : return target > -1 ? (randf() > 0.5f ? STATE_STALKING : STATE_RUN) : STATE_STOP;
            case STATE_RUN      :
            case STATE_STALKING : {
                if (state == STATE_STALKING && health < 6) return STATE_RUN;

                float angleY, dist;
                if (getTargetInfo(0, NULL, NULL, &angleY, &dist)) {
                    float w = state == STATE_RUN ? WOLF_TURN_FAST : WOLF_TURN_SLOW;
                    input = turn(angleY, w);   // also set input mask (left, right) for tilt control

                    if ((state == STATE_STALKING && dist < 512)) {
                        bitten = false;
                        return STATE_BITE;
                    }
                    if ((state == STATE_RUN && dist > 512 && dist < 1024)) {
                        bitten = false;
                        return STATE_JUMP;
                    }
                } else {
                    target = -1;
                    return STATE_GROWL;
                }
                break;
            }
        }

        if ((state == STATE_JUMP || state == STATE_BITE) && !bitten) {
            float dist;
            if (getTargetInfo(0, NULL, NULL, NULL, &dist) && dist < 256.0f)
                bite(animation.getJoints(getMatrix(), JOINT_HEAD, true).pos, state == STATE_BITE ? 100 : 50);                
        }

        if (state == STATE_JUMP)
            return STATE_RUN;

        return state;
    }

    virtual int getStateDeath() {
        switch (state) {
            case STATE_DEATH : return state;
            case STATE_RUN   : return animation.setAnim(ANIM_DEATH_RUN);
            case STATE_JUMP  : return animation.setAnim(ANIM_DEATH_JUMP);
            default          : return animation.setAnim(ANIM_DEATH);
        }
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
        ANIM_DEATH_HIND = 19,
        ANIM_DEATH      = 20,
    };

    enum {
        STATE_WALK   = 0,
        STATE_STOP   = 1,
        STATE_HIND   = 2,
        STATE_RUN    = 3,
        STATE_HOWL   = 4,
        STATE_GROWL  = 5,
        STATE_BITE   = 6,
        STATE_ATTACK = 7,
        STATE_EAT    = 8,
        STATE_DEATH  = 9,
    };

    enum {
        JOINT_CHEST    = 2,
        JOINT_HEAD     = 3
    };

    Bear(IGame *game, int entity) : Enemy(game, entity, 20) {}

    virtual int getStateGround() {
        switch (state) {
            case STATE_STOP     : return STATE_RUN;
            case STATE_GROWL    : return state;
            case STATE_WALK     : 
            case STATE_RUN      :
            case STATE_HIND     : {
                if (state == STATE_HIND && health < 6) return STATE_RUN;

                float angleY, dist;
                if (getTargetInfo(0, NULL, NULL, &angleY, &dist)) {
                    float w = state == STATE_RUN ? WOLF_TURN_FAST : WOLF_TURN_SLOW;
                    input = turn(angleY, w);   // also set input mask (left, right) for tilt control

                    if ((state == STATE_HIND && dist < 512)) {
                        bitten = false;
                        return STATE_ATTACK;
                    }
                    if ((state == STATE_RUN && dist > 512 && dist < 1024)) {
                        bitten = false;
                        return STATE_BITE;
                    }
                }
                break;
            }
        }

        if ((state == STATE_ATTACK || state == STATE_BITE) && !bitten) {
            float dist;
            if (getTargetInfo(0, NULL, NULL, NULL, &dist) && dist < 256.0f)
                bite(animation.getJoints(getMatrix(), JOINT_HEAD, true).pos, state == STATE_BITE ? 100 : 50);                
        }

        return state;
    }

    virtual int getStateDeath() {
        return state == STATE_DEATH ? state : animation.setAnim(ANIM_DEATH);
    }

    virtual void updatePosition() {
        updateTilt(state == STATE_RUN, WOLF_TILT_SPEED, WOLF_TILT_MAX);
        Enemy::updatePosition();
        /*
        if (state == STATE_DEATH) {
            animation.overrideMask = 0;
            return;
        }
        Enemy::updatePosition();
        setOverrides(state == STATE_STALKING || state == STATE_RUN, JOINT_CHEST, JOINT_HEAD);
        lookAt(target, JOINT_CHEST, JOINT_HEAD);
        */
    }
};


#define BAT_TURN_SPEED  PI
#define BAT_LIFT_SPEED  512.0f

struct Bat : Enemy {

    enum {
        ANIM_DEATH     = 4,
    };

    enum {
        STATE_AWAKE    = 1,
        STATE_FLY      = 2,
        STATE_ATTACK   = 3,
        STATE_CIRCLING = 4,
        STATE_DEATH    = 5,
    };

    Bat(IGame *game, int entity) : Enemy(game, entity, 1) { stand = STAND_AIR; }

    virtual int getStateAir() {
        if (!getEntity().flags.active) {
            animation.time = 0.0f;
            animation.dir = 0.0f;
            return STATE_AWAKE;
        }

        switch (state) {
            case STATE_AWAKE  : return STATE_FLY;
            case STATE_ATTACK : 
            case STATE_FLY    : {
                vec3 p;
                float angleY, dist;
                if (getTargetInfo(765, &p, NULL, &angleY, &dist)) {
                    turn(angleY, BAT_TURN_SPEED);
                    lift(p.y - pos.y, BAT_LIFT_SPEED);

                    if (dist < 128) {                        
                        if (state == STATE_ATTACK && !(animation.frameIndex % 15))
                            bite(pos, 2); // TODO: bite position
                        else
                            bitten = false;
                        return STATE_ATTACK;
                    } else
                        return STATE_FLY;
                } else {
                    turn(PI, BAT_TURN_SPEED); // circling
                    return STATE_FLY;
                }
            }                
        }

        return state;
    }

    virtual int getStateDeath() {
        return state == STATE_DEATH ? state : animation.setAnim(ANIM_DEATH);
    }

    virtual void updateVelocity() {
        if (state != STATE_DEATH)
            Enemy::updateVelocity();
        else
            velocity = vec3(0.0f, velocity.y + GRAVITY * Core::deltaTime, 0.0f);
    }

};

#endif
