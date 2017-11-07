#ifndef H_ENEMY
#define H_ENEMY

#include "character.h"

struct Enemy : Character {

    struct Path {
        int16       index;
        int16       count;
        uint16      *boxes;
        TR::Level   *level;

        Path(TR::Level *level, uint16 *boxes, int count) : index(0), count(count), boxes(new uint16[count]), level(level) {
            memcpy(this->boxes, boxes, count * sizeof(boxes[0]));
        }

        ~Path() {
            delete[] boxes;
        }

        bool getNextPoint(TR::Level *level, vec3 &point) {
            if (index >= count - 1)
                return false;

            TR::Box &a = level->boxes[boxes[index++]];
            TR::Box &b = level->boxes[boxes[index]];

            int minX = max(a.minX, b.minX);
            int minZ = max(a.minZ, b.minZ);
            int maxX = min(a.maxX, b.maxX);
            int maxZ = min(a.maxZ, b.maxZ);

            point.x = float(minX + 512) + randf() * (maxX - minX - 1024);
            point.y = float((a.floor + b.floor) / 2);
            point.z = float(minZ + 512) + randf() * (maxZ - minZ - 1024);

            return true;
        }
    };

    enum AI {
        AI_FIXED, AI_RANDOM
    } ai;

    enum Mood {
        MOOD_SLEEP, MOOD_STALK, MOOD_ATTACK, MOOD_ESCAPE 
    } mood;

    bool  wound;
    int   nextState;

    int   targetBox;
    vec3  waypoint;

    float thinkTime;
    float length;       // dist from center to head (jaws)
    float aggression;
    int   radius;
    int   hitSound;

    Character *target;
    Path      *path;

    float targetDist;
    bool  targetDead;
    bool  targetInView;     // target in enemy view zone
    bool  targetFromView;   // enemy in target view zone
    bool  targetCanAttack;

    Enemy(IGame *game, int entity, float health, int radius, float length, float aggression) : Character(game, entity, health), ai(AI_RANDOM), mood(MOOD_SLEEP), wound(false), nextState(0), targetBox(-1), thinkTime(1.0f / 30.0f), length(length), aggression(aggression), radius(radius), hitSound(-1), target(NULL), path(NULL) {
        targetDist   = +INF;
        targetInView = targetFromView = targetCanAttack = false;
    }

    virtual ~Enemy() {
        delete path;
    }

    virtual void getSaveData(SaveData &data) {
        Character::getSaveData(data);
        data.extraSize = sizeof(data.extra.enemy);
        data.extra.enemy.health = uint16(health);
        data.extra.enemy.mood   = uint16(mood);
    }

    virtual void setSaveData(const SaveData &data) {
        Character::setSaveData(data);
        health = float(data.extra.enemy.health);
        mood   = Mood(data.extra.enemy.mood);
    }

    virtual bool activate() {
        return health > 0.0f && Character::activate();
    }

    virtual void updateVelocity() {
        if (stand == STAND_AIR && (!flying || health <= 0.0f))
            applyGravity(velocity.y);
        else
            velocity = getDir() * animation.getSpeed();

        if (health <= 0.0f) 
            velocity.x = velocity.z = 0.0f;
    }

    bool checkPoint(int x, int z) {
        TR::Box &a = level->boxes[box];
        if (a.contains(x, z))
            return true;

        bool big = getEntity().isBigEnemy();
        TR::Overlap *o = &level->overlaps[a.overlap.index];
        do {
            TR::Box &b = level->boxes[o->boxIndex];
            if (!b.contains(x, z))
                continue;
            if (big && b.overlap.blockable)
                continue;
            if (getZones()[o->boxIndex] == zone) {
                int d = a.floor - b.floor;
                if (d <= stepHeight && d >= dropHeight)
                    return true;
            }
        } while (!(o++)->end);

        return false;
    }

    void clipByBox(vec3 &pos) {
        int px = int(pos.x);
        int pz = int(pos.z);
        int nx = px;
        int nz = pz;

        TR::Box &a = level->boxes[box];
            
        if (!checkPoint(px - radius, pz)) nx = a.minX + radius;
        if (!checkPoint(px + radius, pz)) nx = a.maxX - radius;
        if (!checkPoint(px, pz - radius)) nz = a.minZ + radius;
        if (!checkPoint(px, pz + radius)) nz = a.maxZ - radius;

        if (px != nx) pos.x = float(nx);
        if (pz != nz) pos.z = float(nz);
    }

    virtual void updatePosition() {
        if (!flags.active) return;

        vec3 p = pos;
        pos += velocity * (30.0f * Core::deltaTime);

        clipByBox(pos);

        TR::Level::FloorInfo info;
        getFloorInfo(getRoomIndex(), pos, info);
        if (stand == STAND_AIR && !flying && info.floor < pos.y) {
            stand = STAND_GROUND;
            pos.y = info.floor;
        }

        if (info.boxIndex != 0xFFFF && zone == getZones()[info.boxIndex] && !level->boxes[info.boxIndex].overlap.block) {
            switch (stand) {
                case STAND_GROUND : {
                    float fallSpeed = 2048.0f * Core::deltaTime;
                    decrease(info.floor - pos.y, pos.y, fallSpeed);
                    break;
                }
                case STAND_AIR    : 
                    pos.y = clamp(pos.y, info.ceiling, info.floor);
                    break;
                default : ;
            }
        } else
            pos = p;

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

    bool getTargetInfo(int height, vec3 *pos, float *angleX, float *angleY, float *dist) {
        vec3 p = waypoint;
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

    virtual void lookAt(Controller *target) {
        Character::lookAt(targetInView ? target : NULL);
    }

    int turn(float delta, float speed) {
        float w = speed * Core::deltaTime;

        updateTilt(delta, w, speed * 0.1f);

        if (delta != 0.0f) {
            decrease(delta, angle.y, w);
            if (speed != 0.0f) {
                velocity = velocity.rotateY(-w);
                return speed < 0 ? LEFT : RIGHT;
            }
        }
        return 0;
    }

    int lift(float delta, float speed) {
        speed *= Core::deltaTime;
        decrease(delta, pos.y, speed);
        if (speed != 0.0f) {
            return speed < 0 ? FORTH : BACK;
        }
        return 0;
    }

    virtual void hit(float damage, Controller *enemy = NULL, TR::HitType hitType = TR::HIT_DEFAULT) {
        Character::hit(damage, enemy, hitType);
        wound = true;
        if (hitSound > -1) 
            game->playSound(hitSound, pos, Sound::PAN);
    };

    void bite(const vec3 &pos, float damage) {
        ASSERT(target);
        target->hit(damage, this);
        game->addEntity(TR::Entity::BLOOD, target->getRoomIndex(), pos);
    }

    #define STALK_BOX       (1024 * 3)
    #define ESCAPE_BOX      (1024 * 5)
    #define ATTACK_BOX      STALK_BOX
    
    Mood getMoodFixed() {
        bool inZone = zone == target->zone;

        if (mood == MOOD_SLEEP || mood == MOOD_STALK)
            return inZone ? MOOD_ATTACK : (wound ? MOOD_ESCAPE : mood);
        
        if (mood == MOOD_ATTACK)
            return inZone ? mood : MOOD_SLEEP;

        return inZone ? MOOD_ATTACK : mood;
    }

    Mood getMoodRandom() {
        bool inZone = zone == target->zone;
        bool brave  = rand() < (mood != MOOD_ESCAPE ? 0x7800 : 0x0100) && inZone;
            
        if (mood == MOOD_SLEEP || mood == MOOD_STALK) {
            if (wound && !brave)
                return MOOD_ESCAPE;
            if (inZone) {
                int dx = abs(int(pos.x - target->pos.x));
                int dz = abs(int(pos.z - target->pos.z));
                return ((dx <= ATTACK_BOX && dz <= ATTACK_BOX) || (mood == MOOD_STALK && targetBox == -1)) ? MOOD_ATTACK : MOOD_STALK;
            }
            return mood;
        }

        if (mood == MOOD_ATTACK)
            return (wound && !brave) ? MOOD_ESCAPE : (!inZone ? MOOD_SLEEP : mood);

        return brave ? MOOD_STALK : mood;
    }
    
    bool think(bool fixedLogic) {
        if (!target)
            target = (Character*)game->getLara();

        thinkTime += Core::deltaTime;
        if (thinkTime < 1.0f / 30.0f)
            return false;
        thinkTime -= 1.0f / 30.0f;

        if (!target) {
            mood = MOOD_SLEEP;
            targetDist  = +INF;
            targetInView = targetFromView = targetCanAttack = false;
            return true;
        }

        vec3 targetVec  = target->pos - pos - getDir() * length;
        targetDist      = targetVec.length();
        targetDead      = target->health <= 0;
        targetInView    = targetVec.dot(getDir()) > 0;
        targetFromView  = targetVec.dot(target->getDir()) < 0;
        targetCanAttack = targetInView && fabsf(targetVec.y) <= 256.0f;

        int targetBoxOld = targetBox;

    // update mood
        bool inZone = zone == target->zone;

        if (mood != MOOD_ATTACK && targetBox > -1 && !checkBox(targetBox)) {
            if (!inZone)
                mood = MOOD_SLEEP;
            targetBox = -1;
        }

        mood = target->health <= 0 ? MOOD_SLEEP : (ai == AI_FIXED ? getMoodFixed() : getMoodRandom());

    // set behavior and target
        int box;

        switch (mood) {
            case MOOD_SLEEP :
                if (targetBox == -1 && checkBox(box = getRandomZoneBox()) && isStalkBox(box)) {
                    mood = MOOD_STALK;
                    gotoBox(box);
                }
                break;
            case MOOD_STALK :
                if ((targetBox == -1 || !isStalkBox(targetBox)) && checkBox(box = getRandomZoneBox())) {
                    if (isStalkBox(box))
                        gotoBox(box);
                    else
                        if (targetBox == -1) {
                            if (!inZone)
                                mood = MOOD_SLEEP;
                            gotoBox(box);
                        }
                }
                break;
            case MOOD_ATTACK :
                if (randf() > aggression)
                    break;
                targetBox = -1;
                break;
            case MOOD_ESCAPE :
                if (targetBox == -1 && checkBox(box = getRandomZoneBox())) {
                    if (isEscapeBox(box))
                        gotoBox(box);
                    else
                        if (inZone && isStalkBox(box)) {
                            mood = MOOD_STALK;
                            gotoBox(box);
                        }                    
                }
                break;
        }

        if (targetBox == -1)
            gotoBox(target->box);

        if (path && this->box != path->boxes[path->index - 1] && this->box != path->boxes[path->index])
            targetBoxOld = -1;

        if (targetBoxOld != targetBox) {
            if (findPath(stepHeight, dropHeight, getEntity().isBigEnemy()))
                nextWaypoint();
            else
                targetBox = -1;
        }

        if (targetBox != -1 && path) {
            vec3 d = pos - waypoint;

            if (fabsf(d.x) < 512 && fabsf(d.y) < 512 && fabsf(d.z) < 512)
                nextWaypoint();
        }

        return true;
    }

    void nextWaypoint() {
        if (!path->getNextPoint(level, waypoint))
            waypoint = target->pos;
        if (flying) {
            if (target->stand != STAND_ONWATER)
                waypoint.y -= 765.0f;
            else
                waypoint.y -= 64.0f;
        }
    }

    uint16 getRandomZoneBox() {
        return game->getRandomBox(zone, getZones());
    }

    void gotoBox(int box) {
        targetBox = box;
    }

    bool checkBox(int box) {
        if (zone != getZones()[box])
            return false;

        TR::Box    &b = game->getLevel()->boxes[box];
        TR::Entity::Type type = getEntity().type;

        if (b.overlap.block)
            return false;

        if (type == TR::Entity::ENEMY_REX || type == TR::Entity::ENEMY_MUTANT_1 || type == TR::Entity::ENEMY_CENTAUR) {
            if (b.overlap.blockable)
                return false;
        } else
            if (b.overlap.block)
                return false;

        return int(pos.x) < int(b.minX) || int(pos.x) > int(b.maxX) || int(pos.z) < int(b.minZ) || int(pos.z) > int(b.maxZ);
    }

    bool isStalkBox(int box) {
        TR::Box    &b = game->getLevel()->boxes[box];

        int x = (b.minX + b.maxX) / 2 - int(target->pos.x);
        if (abs(x) > STALK_BOX) return false;

        int z = (b.minZ + b.maxZ) / 2 - int(target->pos.z);
        if (abs(z) > STALK_BOX) return false;

        // TODO: check for some quadrant shit

        return true;
    }

    bool isEscapeBox(int box) {
        TR::Box    &b = game->getLevel()->boxes[box];

        int x = (b.minX + b.maxX) / 2 - int(target->pos.x);
        if (abs(x) < ESCAPE_BOX) return false;

        int z = (b.minZ + b.maxZ) / 2 - int(target->pos.z);
        if (abs(z) < ESCAPE_BOX) return false;

        return !((pos.x > target->pos.x) ^ (x > 0)) || !((pos.z > target->pos.z) ^ (z > 0));
    }

    bool findPath(int ascend, int descend, bool big) {
        delete path;
        path = NULL;

        uint16 *boxes;
        uint16 count = game->findPath(ascend, descend, big, box, targetBox, getZones(), &boxes);
        if (count) {
            path = new Path(level, boxes, count);
            return true;
        }

        return false;
    }
};

#define WOLF_TURN_FAST   (DEG2RAD * 150)
#define WOLF_TURN_SLOW   (DEG2RAD * 60)

#define WOLF_DIST_STALK  STALK_BOX
#define WOLF_DIST_BITE   345
#define WOLF_DIST_ATTACK (1024 + 512)


struct Wolf : Enemy {

    enum {
        HIT_MASK = 0x774F,  // body, head, front legs
    };

    enum {
        ANIM_DEATH      = 20,
        ANIM_DEATH_RUN  = 21,
        ANIM_DEATH_JUMP = 22,
    };

    enum {
        STATE_NONE   ,
        STATE_STOP   ,
        STATE_WALK   ,
        STATE_RUN    ,
        STATE_JUMP   , // unused
        STATE_STALK  ,
        STATE_ATTACK ,
        STATE_HOWL   ,
        STATE_SLEEP  ,
        STATE_GROWL  ,
        STATE_TURN   , // unused
        STATE_DEATH  ,
        STATE_BITE   ,
    };

    Wolf(IGame *game, int entity) : Enemy(game, entity, 6, 341, 375.0f, 0.25f) {
        dropHeight = -1024;
        jointChest = 2;
        jointHead  = 3;
        hitSound   = TR::SND_HIT_WOLF;
        nextState  = STATE_NONE;
        animation.time = animation.timeMax;
        updateAnimation(false);
    }

    virtual int getStateGround() {
        if (!flags.active)
            return (state == STATE_STOP || state == STATE_SLEEP) ? STATE_SLEEP : STATE_STOP;

        if (!think(false))
            return state;

        float angle;
        getTargetInfo(0, NULL, NULL, &angle, NULL);

        bool inZone = target ? target->zone == zone : false;

        if (nextState == state)
            nextState = STATE_NONE;

        switch (state) {
            case STATE_SLEEP    :
                if (mood == MOOD_ESCAPE || inZone) {
                    nextState = STATE_GROWL;
                    return STATE_STOP;
                }
                if (randf() < 0.0001f) {
                    nextState = STATE_WALK;
                    return STATE_STOP;
                }
                break;
            case STATE_STOP     : return nextState != STATE_NONE ? nextState : STATE_WALK;
            case STATE_WALK     : 
                if (mood != MOOD_SLEEP) {
                    nextState = STATE_NONE;
                    return STATE_STALK;
                }
                if (randf() < 0.0001f) {
                    nextState = STATE_SLEEP;
                    return STATE_STOP;
                }
                break;
            case STATE_GROWL    :
                if (nextState != STATE_NONE) return nextState;
                if (mood == MOOD_ESCAPE)     return STATE_RUN;
                if (targetDist < WOLF_DIST_BITE && targetCanAttack) return STATE_BITE;
                if (mood == MOOD_STALK)      return STATE_STALK;
                if (mood == MOOD_SLEEP)      return STATE_STOP;
                return STATE_RUN;
            case STATE_STALK    : 
                if (mood == MOOD_ESCAPE)            return STATE_RUN;
                if (targetDist < WOLF_DIST_BITE && targetCanAttack) return STATE_BITE;
                if (targetDist > WOLF_DIST_STALK)   return STATE_RUN;
                if (mood == MOOD_ATTACK) {
                    if (!targetInView || targetFromView || targetDist > WOLF_DIST_ATTACK)
                        return STATE_RUN;
                }
                if (randf() < 0.012f) {
                    nextState = STATE_HOWL;
                    return STATE_GROWL;
                }
                if (mood == MOOD_SLEEP)     return STATE_GROWL;
                break;
            case STATE_RUN      :
                if (targetDist < WOLF_DIST_ATTACK && targetInView) {
                    if (targetDist < WOLF_DIST_ATTACK * 0.5f && targetFromView) {
                        nextState = STATE_NONE;
                        return STATE_ATTACK;
                    }
                    nextState = STATE_STALK;
                    return STATE_GROWL;
                }
                if (mood == MOOD_STALK && targetDist < WOLF_DIST_STALK) {
                    nextState = STATE_STALK;
                    return STATE_GROWL;
                }
                if (mood == MOOD_SLEEP) return STATE_GROWL;
                break;
            case STATE_ATTACK :
            case STATE_BITE   :
                if (nextState == STATE_NONE && targetInView && (collide(target) & HIT_MASK)) {
                    bite(animation.getJoints(getMatrix(), jointHead, true).pos, state == STATE_ATTACK ? 50.0f : 100.0f);
                    nextState = state == STATE_ATTACK ? STATE_RUN : STATE_GROWL;
                }
                return state == STATE_ATTACK ? STATE_RUN : state;
        }

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
        float angleY = 0.0f;

        if (state == STATE_RUN || state == STATE_WALK || state == STATE_STALK)
            getTargetInfo(0, NULL, NULL, &angleY, NULL);

        turn(angleY, state == STATE_RUN ? WOLF_TURN_FAST : WOLF_TURN_SLOW);

        if (state == STATE_DEATH) {
            animation.overrideMask = 0;
            return;
        }

        Enemy::updatePosition();
        setOverrides(state != STATE_DEATH, jointChest, jointHead);
        lookAt(target);
    }
};

struct Lion : Enemy {
    Lion(IGame *game, int entity) : Enemy(game, entity, 6, 341, 375.0f, 0.25f) {
        hitSound = TR::SND_HIT_LION;
    }
};

struct Rat : Enemy {
    Rat(IGame *game, int entity) : Enemy(game, entity, 6, 341, 375.0f, 0.25f) {
        hitSound = TR::SND_HIT_RAT;
    }
};

#define BEAR_DIST_EAT    768
#define BEAR_DIST_HOWL   2048
#define BEAR_DIST_BITE   1024
#define BEAR_DIST_ATTACK 600

#define BEAR_TURN_FAST   (DEG2RAD * 150)
#define BEAR_TURN_SLOW   (DEG2RAD * 60)

struct Bear : Enemy {

    enum {
        HIT_MASK = 0x2406C, // front legs and head
    };

    enum {
        ANIM_DEATH_HIND = 19,
        ANIM_DEATH      = 20,
    };

    enum {
        STATE_NONE   = -1,
        STATE_WALK   ,
        STATE_STOP   ,
        STATE_HIND   ,
        STATE_RUN    ,
        STATE_HOWL   ,
        STATE_GROWL  ,
        STATE_BITE   ,
        STATE_ATTACK ,
        STATE_EAT    ,
        STATE_DEATH  ,
    };

    Bear(IGame *game, int entity) : Enemy(game, entity, 20, 341, 500.0f, 0.5f) {
        jointChest = 13;
        jointHead  = 14;
        hitSound   = TR::SND_HIT_BEAR;
        nextState  = STATE_NONE;
    }

    virtual int getStateGround() {
        if (!flags.active)
            return state;

        if (!think(true))
            return state;

        if (nextState == state)
            nextState = STATE_NONE;

        switch (state) {
            case STATE_WALK     :
                if (nextState != STATE_NONE) return STATE_STOP;
                if (targetDead && targetInView && (collide(target) & HIT_MASK))
                    return nextState = STATE_STOP; // eat lara! >:E
                if (mood != MOOD_SLEEP) {
                    if (mood == MOOD_ESCAPE)
                        nextState = STATE_NONE;
                    return STATE_STOP;
                } else if (randf() < 0.003f) {
                    nextState = STATE_GROWL;
                    return STATE_STOP;
                }
                break;
            case STATE_STOP     :
                if (targetDead)
                    return (targetDist < BEAR_DIST_EAT && targetCanAttack) ? STATE_EAT : STATE_WALK;
                else
                    return nextState != STATE_NONE ? nextState : (mood == MOOD_SLEEP ? STATE_WALK : STATE_RUN);
            case STATE_HIND     :
                if (wound) {
                    nextState = STATE_NONE;
                    return STATE_HOWL;
                }

                if (targetInView && (collide(target) & HIT_MASK)) return STATE_HOWL;

                if (mood == MOOD_ESCAPE)
                    nextState = STATE_NONE;
                else if (mood == MOOD_SLEEP || randf() < 0.003f)
                    nextState = STATE_GROWL;
                else if (targetDist > BEAR_DIST_HOWL || randf() < 0.05f)
                    nextState = STATE_STOP;

                return STATE_HOWL;
                break;
            case STATE_RUN      :
                if (collide(target) & HIT_MASK)
                    target->hit(3, this);                
                if (targetDead || mood == MOOD_SLEEP)
                    return STATE_STOP;
                if (nextState != STATE_NONE) return STATE_STOP;
                if (targetInView) {
                    if (!wound && targetDist < BEAR_DIST_HOWL && randf() < 0.025f) {
                        nextState = STATE_HOWL;
                        return STATE_STOP;
                    } 
                    if (targetDist < BEAR_DIST_BITE) return STATE_BITE;
                }
                break;
            case STATE_HOWL     :
                if (wound) {
                    nextState = STATE_NONE;
                    return STATE_STOP;
                }
                if (nextState != STATE_NONE) return nextState;
                if (mood == MOOD_SLEEP || mood == MOOD_ESCAPE) return STATE_STOP;
                if (targetDist < BEAR_DIST_ATTACK) return STATE_ATTACK;
                return STATE_HIND;
            case STATE_BITE     :
            case STATE_ATTACK   :
                if (nextState == STATE_NONE && (collide(target) & HIT_MASK)) {
                    bite(animation.getJoints(getMatrix(), jointHead, true).pos, state == STATE_BITE ? 200.0f : 400.0f);
                    nextState = state == STATE_BITE ? STATE_STOP : STATE_HOWL;
                }
                break;
        }

        return state;
    }

    virtual int getStateDeath() {
        switch (state) {
            case STATE_HIND : return STATE_HOWL;
            case STATE_RUN  :
            case STATE_WALK : return STATE_STOP;
            case STATE_HOWL :
            case STATE_STOP : return STATE_DEATH;
        }
        return state;// == STATE_DEATH ? state : animation.setAnim(ANIM_DEATH);
    }

    virtual void updatePosition() {
        float angleY = 0.0f;
        if (state == STATE_RUN || state == STATE_WALK || state == STATE_HIND)
            getTargetInfo(0, NULL, NULL, &angleY, NULL);

        turn(angleY, state == STATE_RUN ? BEAR_TURN_FAST : BEAR_TURN_SLOW);

        if (state == STATE_DEATH) {
            animation.overrideMask = 0;
            return;
        }

        Enemy::updatePosition();
        setOverrides(state == STATE_RUN || state == STATE_WALK || state == STATE_HIND, jointChest, jointHead);
        lookAt(target);
    }
};


#define BAT_TURN_SPEED  (DEG2RAD * 300)
#define BAT_LIFT_SPEED  512.0f

struct Bat : Enemy {

    enum {
        ANIM_DEATH = 4,
    };

    enum {
        STATE_NONE,
        STATE_AWAKE,
        STATE_FLY,
        STATE_ATTACK,
        STATE_CIRCLING,
        STATE_DEATH,
    };

    Bat(IGame *game, int entity) : Enemy(game, entity, 1, 102, 0.0f, 0.03f) {
        stand = STAND_AIR;
        stepHeight =  20 * 1024;
        dropHeight = -20 * 1024;
        jointHead  = 4;
    }

    virtual int getStateAir() {
        if (!flags.active) {
            animation.time = 0.0f;
            animation.dir  = 0.0f;
            return STATE_AWAKE;
        }

        if (!think(false))
            return state;

        switch (state) {
            case STATE_AWAKE  : return STATE_FLY;
            case STATE_ATTACK : 
                if (!collide(target)) {
                    mood = MOOD_SLEEP;
                    return STATE_FLY;
                } else
                    bite(animation.getJoints(getMatrix(), jointHead, true).pos, 2);
                break;
            case STATE_FLY    : 
                if (collide(target)) {
                    mood = MOOD_ATTACK;
                    return STATE_ATTACK;
                }
                break;
        }

        return state;
    }

    virtual int getStateDeath() {
        return state == STATE_DEATH ? state : animation.setAnim(ANIM_DEATH);
    }

    virtual void updatePosition() {
        float angleY = 0.0f;
        if (state == STATE_FLY || state == STATE_ATTACK)
            getTargetInfo(0, NULL, NULL, &angleY, NULL);
        turn(angleY, BAT_TURN_SPEED);
        if (flying)
            lift(waypoint.y - pos.y, BAT_LIFT_SPEED);
        Enemy::updatePosition();
    }

    virtual void deactivate(bool removeFromList = false) {
        if (health <= 0.0f) {
            TR::Level::FloorInfo info;
            getFloorInfo(getRoomIndex(), pos, info);
            if (info.floor > pos.y)
                return;
            pos.y = info.floor;
        }
        Enemy::deactivate(removeFromList);
    }
};


#define REX_DIST_BITE       1500
#define REX_DIST_BITE_MAX   4096
#define REX_DIST_WALK       5120
#define REX_TURN_FAST       (DEG2RAD * 120)
#define REX_TURN_SLOW       (DEG2RAD * 60)
#define REX_DAMAGE          1000

struct Rex : Enemy {

    enum {
        HIT_MASK = (1 << 12) | (1 << 13),  // head
    };

    enum {
        STATE_NONE,
        STATE_STOP,
        STATE_WALK,
        STATE_RUN,
        STATE_UNUSED,
        STATE_DEATH,
        STATE_BAWL,
        STATE_BITE,
        STATE_FATAL,
    };

    Rex(IGame *game, int entity) : Enemy(game, entity, 100, 341, 2000.0f, 1.0f) {
        jointChest = 10;
        jointHead  = 12;    
        nextState  = STATE_NONE;
    }

    virtual int getStateGround() {
        if (!flags.active)
            return state;

        if (!think(true))
            return state;

        if (nextState == state)
            nextState = STATE_NONE;

        if (targetDead) {
            return (state == STATE_STOP || state == STATE_WALK) ? STATE_WALK : STATE_STOP;
            if (state != STATE_STOP) return STATE_STOP;
            return STATE_WALK;
        }

        int mask = collide(target);

        // if Lara is behind and watching Rex we need to rotate
        bool walk = targetFromView && !targetInView && mood != MOOD_ESCAPE;
        if (!walk && targetCanAttack && targetDist > REX_DIST_BITE && targetDist < REX_DIST_BITE_MAX)
            walk = true;

        switch (state) {
            case STATE_STOP : 
                if (nextState != STATE_NONE)                        return nextState;
                if (targetCanAttack && targetDist < REX_DIST_BITE)  return STATE_BITE;
                if (mood == MOOD_SLEEP || walk)                     return STATE_WALK;
                return STATE_RUN;
            case STATE_WALK :
                if (mask) target->hit(1, this);
                if (mood != MOOD_SLEEP && !walk)    return STATE_STOP;
                if (targetInView && randf() < 0.015f) {
                    nextState = STATE_BAWL;
                    return STATE_STOP;
                }
                break;
            case STATE_RUN :
                if (mask) target->hit(10, this);
                if ((targetCanAttack && targetDist < REX_DIST_WALK) || walk)
                    return STATE_STOP;
                if (targetInView && mood != MOOD_ESCAPE && randf() < 0.015f) {
                    nextState = STATE_BAWL;
                    return STATE_STOP;
                }
                if (mood == MOOD_SLEEP)
                    return STATE_STOP;
                break;
            case STATE_BITE :
                if (mask & HIT_MASK) {
                    target->hit(REX_DAMAGE, this, TR::HIT_REX);
                    return STATE_FATAL;
                }
                nextState = STATE_WALK;
                break;
        }

        return state;
    }

    virtual int getStateDeath() {
        return state == STATE_STOP ? STATE_DEATH : STATE_STOP;
    }

    virtual void updatePosition() {
        if (state == STATE_DEATH) {
            animation.overrideMask = 0;
            angle.z = 0.0f;
            return;
        }

        float angleY = 0.0f;
        getTargetInfo(0, NULL, NULL, &angleY, NULL);

        if (state == STATE_RUN || state == STATE_WALK)
            turn(angleY, state == STATE_RUN ? REX_TURN_FAST : REX_TURN_SLOW);

        Enemy::updatePosition();
        setOverrides(true, jointChest, jointHead);
        lookAt(target);
    }
};

#define RAPTOR_DIST_BITE        680
#define RAPTOR_DIST_ATTACK      (1024 + 512)

#define RAPTOR_TURN_FAST        (DEG2RAD * 120)
#define RAPTOR_TURN_SLOW        (DEG2RAD * 30)

struct Raptor : Enemy {

    enum {
        HIT_MASK = 0xFF7C00,  // hands and head
    };

    enum {
        ANIM_DEATH_1 = 9,
        ANIM_DEATH_2 = 10,
    };

    enum {
        STATE_NONE = -1,
        STATE_DEATH,
        STATE_STOP,
        STATE_WALK,
        STATE_RUN,
        STATE_ATTACK_1,
        STATE_UNUSED,
        STATE_BAWL,
        STATE_ATTACK_2,
        STATE_BITE,
    };

    Raptor(IGame *game, int entity) : Enemy(game, entity, 20, 341, 400.0f, 0.5f) {
        jointChest = -1;
        jointHead  = 21;    
        nextState  = STATE_NONE;
    }

    virtual int getStateGround() {
        if (!flags.active)
            return state;

        if (!think(true))
            return state;

        if (nextState == state)
            nextState = STATE_NONE;

        if (targetDead) {
            return (state == STATE_STOP || state == STATE_WALK) ? STATE_WALK : STATE_STOP;
            if (state != STATE_STOP) return STATE_STOP;
            return STATE_WALK;
        }

        int mask = collide(target);

        switch (state) {
            case STATE_STOP : 
                if (nextState != STATE_NONE) return nextState;
                if ((mask & HIT_MASK) || (targetCanAttack && targetDist < RAPTOR_DIST_BITE)) return STATE_BITE;
                if (targetCanAttack && targetDist < RAPTOR_DIST_ATTACK) return STATE_ATTACK_1;
                if (mood == MOOD_SLEEP) return STATE_WALK;
                return STATE_RUN;
            case STATE_WALK :
                if (nextState != STATE_NONE) return STATE_STOP;
                if (mood != MOOD_SLEEP) return STATE_STOP;
                if (targetInView && randf() < 0.01f) {
                    nextState = STATE_BAWL;
                    return STATE_STOP;
                }
                break;
            case STATE_RUN :
                if (nextState != STATE_NONE) return STATE_STOP;
                if (mask & HIT_MASK) return STATE_STOP;
                if (targetCanAttack && targetDist < RAPTOR_DIST_ATTACK)
                    return (randf() < 0.25) ? STATE_STOP : STATE_ATTACK_2;
                if (mood == MOOD_ESCAPE && targetInView) {
                    nextState = STATE_BAWL;
                    return STATE_STOP;
                }
                if (mood == MOOD_SLEEP)
                    return STATE_STOP;
                break;
            case STATE_ATTACK_1 :
            case STATE_ATTACK_2 :
            case STATE_BITE     :
                if (nextState == STATE_NONE && targetInView && (mask & HIT_MASK)) {
                    bite(animation.getJoints(getMatrix(), jointHead, true).pos, 100);                    
                    nextState = state == STATE_ATTACK_2 ? STATE_RUN : STATE_STOP;
                }
                break;
        }

        return state;
    }

    virtual int getStateDeath() {
        if (state == STATE_DEATH) return state;
        return animation.setAnim((rand() % 2) ? ANIM_DEATH_1 : ANIM_DEATH_2);
    }

    virtual void updatePosition() {
        if (state == STATE_DEATH) {
            animation.overrideMask = 0;
            angle.z = 0.0f;
            return;
        }

        float angleY = 0.0f;
        getTargetInfo(0, NULL, NULL, &angleY, NULL);

        if (state == STATE_RUN || state == STATE_WALK)
            turn(angleY, state == STATE_RUN ? RAPTOR_TURN_FAST : RAPTOR_TURN_SLOW);
        
        Enemy::updatePosition();
        setOverrides(true, jointChest, jointHead);
        lookAt(target);
    }
};

struct Mutant : Enemy {
    Mutant(IGame *game, int entity) : Enemy(game, entity, 50, 341, 150.0f, 1.0f) {
        if (getEntity().type != TR::Entity::ENEMY_MUTANT_1) {
            initMeshOverrides();
            layers[0].mask = 0xffe07fff;
            aggression     = 0.25f;
        }

        jointChest = 1;
        jointHead  = 2;    
        nextState  = 0;
    }

    virtual void update() {
        bool exploded = explodeMask != 0;

        if (health <= 0.0f && !exploded) {
            game->playSound(TR::SND_MUTANT_DEATH, pos, 0);
            explode(0xffffffff);
        }

        Enemy::update();

        if (exploded && !explodeMask) {
            deactivate(true);
            flags.invisible = true;
        }
    }

    virtual int getStateGround() {
        if (!think(true))
            return state;
        return state;
    }

    virtual void updatePosition() {
        Enemy::updatePosition();
        setOverrides(true, jointChest, jointHead);
        lookAt(target);
    }
};

struct GiantMutant : Enemy {
    enum {
        STATE_STOP = 1,
        STATE_BORN = 8,
        STATE_FALL = 9,
    };

    GiantMutant(IGame *game, int entity) : Enemy(game, entity, 500, 341, 375.0f, 1.0f) {
        hitSound = TR::SND_HIT_MUTANT;
        stand = STAND_AIR;
        jointChest = -1;
        jointHead  = 3;
        rangeHead  = vec4(-0.5f, 0.5f, -0.5f, 0.5f) * PI;
        invertAim  = true;
    }

    virtual int getStateGround() {
        if (!think(true))
            return state;
        return state;
    }

    void update() {
        bool exploded = explodeMask != 0;

        if (health <= 0.0f && !exploded) {
            game->playSound(TR::SND_MUTANT_DEATH, pos, 0);
            explode(0xffffffff);
        }

        switch (state) {
            case STATE_BORN : animation.setState(STATE_FALL); break;
            case STATE_FALL : 
                if (stand == STAND_GROUND) {
                    animation.setState(STATE_STOP);
                    game->getCamera()->shake = 5.0f;
                }
                break;
        }

        Enemy::update();

        setOverrides(true, jointChest, jointHead);
        lookAt(target);

        if (exploded && !explodeMask) {
            game->checkTrigger(this, true);
            deactivate(true);
            flags.invisible = true;
        }
    }
};


struct Centaur : Enemy {
    Centaur(IGame *game, int entity) : Enemy(game, entity, 20, 341, 400.0f, 0.5f) {
        jointChest = 10;
        jointHead  = 17;
    }

    virtual int getStateGround() {
        if (!think(true))
            return state;
        return state;
    }

    virtual void updatePosition() {
        Enemy::updatePosition();
        setOverrides(true, jointChest, jointHead);
        lookAt(target);
    }
};


struct Mummy : Enemy {
    enum {
        STATE_NONE,
        STATE_IDLE,
        STATE_FALL,
    };

    Mummy(IGame *game, int entity) : Enemy(game, entity, 18, 341, 150.0f, 0.0f) {
        jointHead = 2;
    }

    virtual void update() {
        if (state == STATE_IDLE && (health <= 0.0f || collide((Controller*)level->laraController))) {
            animation.setState(STATE_FALL);
            health = 0.0f;
        }
        Enemy::update();
    }

    virtual int getStateGround() {
        if (!think(true))
            return state;
        return state;
    }

    virtual void updatePosition() {
        Enemy::updatePosition();
        setOverrides(true, jointChest, jointHead);
        lookAt(target);
    }
};


#define DOPPELGANGER_ROOM_CENTER (vec3(36, 0, 60) * 1024.0f)

struct Doppelganger : Enemy {
    enum {
        ANIM_FALL = 34,
    };

    Doppelganger(IGame *game, int entity) : Enemy(game, entity, 1000, 341, 150.0f, 0.0f) {
        jointChest = 7;
        jointHead  = 14;
    }

    virtual void hit(float damage, Controller *enemy = NULL, TR::HitType hitType = TR::HIT_DEFAULT) {
        Character *lara = (Character*)game->getLara();
        lara->hit(damage * 10, this);
    };

    virtual void update() {
        Character *lara = (Character*)game->getLara();

        if (stand != STAND_AIR) {
            pos      = DOPPELGANGER_ROOM_CENTER * 2.0f - lara->pos;
            pos.y    = lara->pos.y;
            angle    = lara->angle;
            angle.y -= PI;
        }

        Enemy::checkRoom();

        TR::Level::FloorInfo info;
        getFloorInfo(getRoomIndex(), pos, info);

        if (stand != STAND_AIR && lara->stand == Character::STAND_GROUND && pos.y < info.floor - 1024) {
            animation = Animation(level, lara->getModel());
            animation.setAnim(ANIM_FALL, 1);
            stand = STAND_AIR;
            velocity.x = velocity.y = 0.0f;
        }

        if (stand == STAND_AIR) {
            if (pos.y > info.floor) {
                game->checkTrigger(this, true);
                flags.invisible = true;
                deactivate(true);
            } else {
                updateAnimation(true);
                applyGravity(velocity.y);
                pos += velocity * (30.0f * Core::deltaTime);
            }
        } else {
            animation.frameA = lara->animation.frameA;
            animation.frameB = lara->animation.frameB;
            animation.delta  = lara->animation.delta;
        }
    }

    virtual int getStateGround() {
        if (!think(true))
            return state;
        return state;
    }

    virtual void updatePosition() {
        Enemy::updatePosition();
        setOverrides(true, jointChest, jointHead);
        lookAt(target);
    }
};


struct ScionTarget : Enemy {
    float timer;

    ScionTarget(IGame *game, int entity) : Enemy(game, entity, 5, 0, 0, 0), timer(0.0f) {}

    virtual void update() {
        Controller::update();

        if (health <= 0.0f) {
            if (timer == 0.0f) {
                flags.invisible = true;
                game->checkTrigger(this, true);
                timer = 3.0f;
            }

            if (timer > 0.0f) {
                int index = int(timer / 0.3f);
                timer -= Core::deltaTime;

                if (index != int(timer / 0.3f)) {
                    vec3 p = pos + vec3((randf() * 2.0f - 1.0f) * 512.0f, (randf() * 2.0f - 1.0f) * 64.0f - 500.0f, (randf() * 2.0f - 1.0f) * 512.0f);
                    game->addEntity(TR::Entity::EXPLOSION, getRoomIndex(), p);
                    game->getCamera()->shake = 0.5f;
                }

                if (timer < 0.0f) 
                    deactivate(true);
            }
        }
    }
};


struct Human : Enemy {
    enum {
        STATE_NONE,
        STATE_STOP,
        STATE_WALK,
        STATE_RUN,
        STATE_AIM,
        STATE_DEATH,
        STATE_UNKNOWN,
        STATE_FIRE
    };

    int animDeath;

    Human(IGame *game, int entity, float health) : Enemy(game, entity, health, 100, 375.0f, 1.0f), animDeath(-1) {
        jointChest = 7;
        jointHead  = 8;
    }

    virtual void deactivate(bool removeFromList = false) {
        if (health <= 0.0f)
            onDead();
        Enemy::deactivate(removeFromList);
    }

    virtual int getStateDeath() {
        return (animDeath == -1 || state == STATE_DEATH || state == STATE_NONE) ? STATE_DEATH : animation.setAnim(animDeath);
    }

    virtual int getStateGround() {
        if (!think(true))
            return state;
        return state;
    }

    virtual void updatePosition() {
        Enemy::updatePosition();
        setOverrides(true, jointChest, jointHead);
        lookAt(target);
    }

    virtual void onDead() {}
};


struct Larson : Human {

    Larson(IGame *game, int entity) : Human(game, entity, 50) {
        animDeath = 15;
    }
};


struct Pierre : Human {

    Pierre(IGame *game, int entity) : Human(game, entity, 70) {
        animDeath = 12;
    }

    virtual void onDead() {
        if (level->id == TR::LEVEL_7B) {
            game->addEntity(TR::Entity::MAGNUMS,           getRoomIndex(), pos, 0);
            game->addEntity(TR::Entity::SCION_PICKUP_DROP, getRoomIndex(), pos, 0);
            game->addEntity(TR::Entity::KEY_ITEM_1,        getRoomIndex(), pos, 0);
        }
    }
};


struct SkaterBoy : Human {

    SkaterBoy(IGame *game, int entity) : Human(game, entity, 125) {
        animDeath = 13;
    }

    virtual void onDead() {
        game->addEntity(TR::Entity::UZIS, getRoomIndex(), pos, 0);
    }
};


struct Cowboy : Human {

    Cowboy(IGame *game, int entity) : Human(game, entity, 150) {
        animDeath = 7;
    }

    virtual void onDead() {
        game->addEntity(TR::Entity::MAGNUMS, getRoomIndex(), pos, 0);
    }
};


struct MrT : Human {

    MrT(IGame *game, int entity) : Human(game, entity, 200) {
        animDeath = 14;
    }

    virtual void onDead() {
        game->addEntity(TR::Entity::SHOTGUN, getRoomIndex(), pos, 0);
    }
};


struct Natla : Human {

    Natla(IGame *game, int entity) : Human(game, entity, 400) {}
};

#endif