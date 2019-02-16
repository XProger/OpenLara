#ifndef H_ENEMY
#define H_ENEMY

#include "character.h"
#include "trigger.h"

#define STALK_BOX       (1024 * 3)
#define ESCAPE_BOX      (1024 * 5)
#define ATTACK_BOX      STALK_BOX

#define MAX_SHOT_DIST   (64 * 1024)

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

    uint16 targetBox;
    vec3   waypoint;

    float thinkTime;
    float length;       // dist from center to head (jaws)
    float aggression;
    int   radius;
    int   hitSound;

    Character *target;
    Path      *path;

    float targetDist;
    float targetAngle;
    bool  targetDead;
    bool  targetInView;     // target in enemy view zone
    bool  targetFromView;   // enemy in target view zone
    bool  targetCanAttack;

    Enemy(IGame *game, int entity, float health, int radius, float length, float aggression) : Character(game, entity, health), ai(AI_RANDOM), mood(MOOD_SLEEP), wound(false), nextState(0), targetBox(TR::NO_BOX), thinkTime(1.0f / 30.0f), length(length), aggression(aggression), radius(radius), hitSound(-1), target(NULL), path(NULL) {
        targetDist   = +INF;
        targetInView = targetFromView = targetCanAttack = false;
        waypoint     = pos;
    }

    virtual ~Enemy() {
        delete path;
    }

    virtual bool getSaveData(SaveEntity &data) {
        Character::getSaveData(data);
        data.extraSize = sizeof(data.extra.enemy);
        data.extra.enemy.health    = health;
        data.extra.enemy.spec.mood = mood;
        data.extra.enemy.targetBox = targetBox;
        return true;
    }

    virtual void setSaveData(const SaveEntity &data) {
        Character::setSaveData(data);
        health    = data.extra.enemy.health;
        mood      = Mood(data.extra.enemy.spec.mood);
        targetBox = data.extra.enemy.targetBox;
        updateZone();
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

    void collideEnemies() {
        if (getEntity().isBigEnemy())
            return;

        Controller *c = Controller::first;
        while (c) {
            if (c != this && c->getEntity().isEnemy()) {
                Enemy *enemy = (Enemy*)c;
                if (enemy->health > 0.0f) {
                    vec3 dir = vec3(enemy->pos.x - pos.x, 0.0f, enemy->pos.z - pos.z);
                    float D = dir.length2();
                    float R = float((enemy->radius + radius) / 2);
                    if (D < R * R) {
                        D = sqrtf(D);
                        pos -= dir.normal() * (R - D);
                    }
                }
            }
            c = c->next;
        }
    }

    virtual void updatePosition() {
        if (!flags.active) return;

        vec3 p = pos;
        pos += velocity * (30.0f * Core::deltaTime);

        collideEnemies();

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

        updateRoom();
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

    bool targetIsVisible(float maxDist) {
        if (targetInView && targetDist < maxDist && target->health > 0.0f) {
            TR::Location from, to;
            from.room = getRoomIndex();
            from.pos  = pos;
            to.pos    = target->pos;

        // vertical offset to ~gun/head height
            from.pos.y -= 768.0f;
            if (target->stand != STAND_UNDERWATER && target->stand != STAND_ONWATER)
                to.pos.y -= 768.0f;

            return trace(from, to);
        }
        return false;
    }

    virtual void lookAt(Controller *target) {
        Character::lookAt(targetInView ? target : NULL);
    }

    void turn(bool tilt, float w) {
        float speed = animation.getSpeed();

        if (!target || speed == 0.0f || w == 0.0f) {
            angle.z = lerp(angle.z, 0.0f, 4.0f * Core::deltaTime);
            return;
        }

        vec3  d = waypoint - pos;
        float a = clampAngle(normalizeAngle(PIH - d.angleY() - angle.y));

        w /= 30.0f;

        float minDist = speed * PIH / w;

        if ( (a > PIH || a < -PIH) && (SQR(d.x) + SQR(d.z) < SQR(minDist)) )
            w *= 0.5f;

        a = clamp(a, -w, w);

        angle.y += a * 30.0f * Core::deltaTime;
        angle.z = lerp(angle.z, tilt ? a * 2.0f : 0.0f, 4.0f * Core::deltaTime);
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
        if (hitSound > -1 && health > 0.0f) 
            game->playSound(hitSound, pos, Sound::PAN);
        Character::hit(damage, enemy, hitType);
        wound = true;
    };

    void bite(int joint, const vec3 &offset, float damage) {
        ASSERT(target);
        target->hit(damage, this);
        if (joint >= 0)
            game->addEntity(TR::Entity::BLOOD, target->getRoomIndex(), getJoint(joint) * offset);
    }

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
                return ((dx <= ATTACK_BOX && dz <= ATTACK_BOX) || (mood == MOOD_STALK && targetBox == TR::NO_BOX)) ? MOOD_ATTACK : MOOD_STALK;
            }
            return mood;
        }

        if (mood == MOOD_ATTACK)
            return (wound && !brave) ? MOOD_ESCAPE : (!inZone ? MOOD_SLEEP : mood);

        return brave ? MOOD_STALK : mood;
    }
    
    bool think(bool fixedLogic) {
        thinkTime += Core::deltaTime;
        if (thinkTime < 1.0f / 30.0f)
            return false;
        thinkTime -= 1.0f / 30.0f;

        int zoneOld = zone;
        updateZone();

        target = (Character*)game->getLara(pos);

        vec3 targetVec  = target->pos - pos - getDir() * length;
        targetDist      = targetVec.length();
        targetAngle     = clampAngle(atan2f(targetVec.x, targetVec.z) - angle.y);
        targetDead      = target->health <= 0;
        targetInView    = targetVec.dot(getDir()) > 0;
        targetFromView  = targetVec.dot(target->getDir()) < 0;
        targetCanAttack = targetInView && fabsf(targetVec.y) <= 256.0f;

        int targetBoxOld = targetBox;

        bool inZone = zone == target->zone;

        if (target->health <= 0.0f || !inZone)
            targetBox = TR::NO_BOX;

    // update mood
        if (mood != MOOD_ATTACK && targetBox != TR::NO_BOX && !checkBox(targetBox)) {
            if (!inZone)
                mood = MOOD_SLEEP;
            targetBox = TR::NO_BOX;
        }

        mood = target->health <= 0 ? MOOD_SLEEP : (ai == AI_FIXED ? getMoodFixed() : getMoodRandom());

    // set behavior and target
        int box;

        switch (mood) {
            case MOOD_SLEEP :
                if (targetBox == TR::NO_BOX && checkBox(box = getRandomZoneBox()) && isStalkBox(box)) {
                    mood = MOOD_STALK;
                    gotoBox(box);
                }
                break;
            case MOOD_STALK :
                if ((targetBox == TR::NO_BOX || !isStalkBox(targetBox)) && checkBox(box = getRandomZoneBox())) {
                    if (isStalkBox(box))
                        gotoBox(box);
                    else
                        if (targetBox == TR::NO_BOX) {
                            if (!inZone)
                                mood = MOOD_SLEEP;
                            gotoBox(box);
                        }
                }
                break;
            case MOOD_ATTACK :
                if (randf() > aggression)
                    break;
                targetBox = TR::NO_BOX;
                break;
            case MOOD_ESCAPE :
                if (targetBox == TR::NO_BOX && checkBox(box = getRandomZoneBox())) {
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

        if (targetBox == TR::NO_BOX)
            gotoBox(target->box);

        if (path && this->box != path->boxes[path->index - 1] && this->box != path->boxes[path->index])
            targetBoxOld = TR::NO_BOX;

        if (zoneOld != zone)
            targetBoxOld = TR::NO_BOX;

        if (targetBoxOld != targetBox) {
            if (findPath(stepHeight, dropHeight, getEntity().isBigEnemy()))
                nextWaypoint();
            else
                targetBox = TR::NO_BOX;
        }

        if (targetBox != TR::NO_BOX && path) {
            vec3 d = pos - waypoint;

            if (fabsf(d.x) < 512 && fabsf(d.y) < 512 && fabsf(d.z) < 512)
                nextWaypoint();
        }

        return true;
    }

    void nextWaypoint() {
        if (!path->getNextPoint(level, waypoint))
            waypoint = target->pos;
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
        uint16   type = getEntity().type;

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

    void shot(TR::Entity::Type type, int joint, const vec3 &offset, float rx, float ry) {
        vec3 from = getJoint(joint) * offset;
        vec3 to   = target->getBoundingBox().center();

        Bullet *bullet = (Bullet*)game->addEntity(type, getRoomIndex(), from);
        if (bullet) {
            vec3 dir = to - from;
            vec3 ang = vec3(-atan2f(dir.y, sqrtf(dir.x * dir.x + dir.z * dir.z)), atan2f(dir.x, dir.z), 0.0f);
            ang += vec3(rx, ry, 0.0f);
            bullet->setAngle(ang);
        }
    }

    void shot(TR::Entity::Type type, int joint, const vec3 &offset) {
        shot(type, joint, offset, (randf() * 2.0f - 1.0f) * (1.5f * DEG2RAD), (randf() * 2.0f - 1.0f) * (1.5f * DEG2RAD));
    }

    bool isVisible() {
        for (int i = 0; i < 2; i++) {
            ICamera *camera = game->getCamera(i);
            if (!camera) continue;

            TR::Location eye = camera->eye;
            TR::Location loc;
            loc.room = getRoomIndex();
            loc.box  = box;
            loc.pos  = pos;
            loc.pos.y -= 1024;

            if (trace(eye, loc))
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
        if (!think(false))
            return state;

        if (nextState == state)
            nextState = STATE_NONE;

        switch (state) {
            case STATE_SLEEP    :
                if (mood == MOOD_ESCAPE || target->zone == zone)
                    nextState = STATE_GROWL;
                else if (rand() < 32)
                    nextState = STATE_WALK;
                else
                    break;
                return STATE_STOP;
            case STATE_STOP     : return nextState != STATE_NONE ? nextState : STATE_WALK;
            case STATE_WALK     : 
                if (mood != MOOD_SLEEP) {
                    nextState = STATE_NONE;
                    return STATE_STALK;
                }
                if (rand() < 32) {
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
                if (rand() < 384) {
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
                    bite(6, vec3(0.0f, -14.0f, 174.0f), state == STATE_ATTACK ? 50.0f : 100.0f);
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
        turn(state == STATE_RUN || state == STATE_WALK || state == STATE_STALK, state == STATE_RUN ? WOLF_TURN_FAST : WOLF_TURN_SLOW);

        if (state == STATE_DEATH) {
            animation.overrideMask = 0;
            return;
        }

        Enemy::updatePosition();
        setOverrides(state != STATE_DEATH, jointChest, jointHead);
        lookAt(target);
    }
};


#define LION_DIST_ATTACK 1024
#define LION_TURN_FAST   (DEG2RAD * 150)
#define LION_TURN_SLOW   (DEG2RAD * 60)

struct Lion : Enemy {

    enum {
        HIT_MASK = 0x380066,
    };

    enum {
        ANIM_DEATH_LION = 7,
        ANIM_DEATH_PUMA = 4,
    };

    enum {
        STATE_NONE   ,
        STATE_STOP   ,
        STATE_WALK   ,
        STATE_RUN    ,
        STATE_ATTACK ,
        STATE_DEATH  ,
        STATE_ROAR   ,
        STATE_BITE   ,
    };

    Lion(IGame *game, int entity) : Enemy(game, entity, 6, 341, 400.0f, 0.25f) {
        dropHeight = -1024;
        jointChest = 19;
        jointHead  = 20;
        switch (getEntity().type) {
            case TR::Entity::ENEMY_LION_MALE :
                hitSound   = TR::SND_HIT_LION;
                health     = 30.0f;
                aggression = 1.0f;
                break;
            case TR::Entity::ENEMY_LION_FEMALE :
                hitSound = TR::SND_HIT_LION;
                health   = 25.0f;
                break;
            case TR::Entity::ENEMY_PUMA :
                health   = 45.0f;
                break;
            default : ;
        }
    }

    virtual int getStateGround() {
        if (!think(true))
            return state;

        if (nextState == state)
            nextState = STATE_NONE;

        switch (state) {
            case STATE_STOP    :
                if (nextState != STATE_NONE)
                    return nextState;
                if (mood == MOOD_SLEEP)
                    return STATE_WALK;
                if (targetInView && (collide(target) & HIT_MASK))
                    return STATE_BITE;
                if (targetInView && targetDist < LION_DIST_ATTACK)
                    return STATE_ATTACK;
                return STATE_RUN;
            case STATE_WALK     : 
                if (mood != MOOD_SLEEP)
                    return STATE_STOP;
                if (randf() < 0.0004f) {
                    nextState = STATE_ROAR;
                    return STATE_STOP;
                }
                break;
            case STATE_RUN      :
                if ((mood == MOOD_SLEEP) ||
                    (targetInView && targetDist < LION_DIST_ATTACK) ||
                    (targetInView && (collide(target) & HIT_MASK)))
                    return STATE_STOP;
                if (mood == MOOD_ESCAPE && randf() < 0.0004f) {
                    nextState = STATE_ROAR;
                    return STATE_STOP;
                }
                break;
            case STATE_ATTACK :
            case STATE_BITE   :
                if (nextState == STATE_NONE && (collide(target) & HIT_MASK)) {
                    bite(21, vec3(-2.0f, -10.0f, 132.0f), state == STATE_ATTACK ? 150.0f : 250.0f);
                    nextState = STATE_STOP;
                }
        }

        return state;
    }

    virtual int getStateDeath() {
        if (state == STATE_DEATH)
            return state;
        int deathAnim = (getEntity().type == TR::Entity::ENEMY_PUMA) ? ANIM_DEATH_PUMA : ANIM_DEATH_LION;
        return animation.setAnim(deathAnim + rand() % 2);
    }

    virtual void updatePosition() {
        turn(state == STATE_RUN || state == STATE_WALK || state == STATE_ROAR, state == STATE_RUN ? LION_TURN_FAST : LION_TURN_SLOW);

        if (state == STATE_DEATH) {
            animation.overrideMask = 0;
            return;
        }

        Enemy::updatePosition();
        setOverrides(true, jointChest, jointHead);
        lookAt(target);
    }
};


#define GORILLA_DIST_ATTACK     430
#define GORILLA_DIST_AGGRESSION 2048
#define GORILLA_TURN_FAST       (DEG2RAD * 150)

struct Gorilla : Enemy {

    enum {
        HIT_MASK = 0x00FF00,
    };

    enum {
        ANIM_DEATH = 7,
        ANIM_CLIMB = 19,
    };

    enum {
        STATE_NONE   ,
        STATE_STOP   ,
        STATE_UNUSED ,
        STATE_RUN    ,
        STATE_ATTACK ,
        STATE_DEATH  ,
        STATE_IDLE1  ,
        STATE_IDLE2  ,
        STATE_LEFT   ,
        STATE_RIGHT  ,
        STATE_JUMP   ,
        STATE_CLIMB  ,
    };

    enum {
        FLAG_ATTACK = 1,
        FLAG_LEFT   = 2,
        FLAG_RIGHT  = 4,
    };

    Gorilla(IGame *game, int entity) : Enemy(game, entity, 22, 341, 250.0f, 1.0f) {
        dropHeight = -1024;
        stepHeight =  1024;
        jointChest = -1;//7;
        jointHead  = 14;
        flags.unused = 0;
    }

    virtual int getStateGround() {
        if (!think(true))
            return state;
        
        if (nextState == state)
            nextState = STATE_NONE;

        if (targetDist < GORILLA_DIST_AGGRESSION)
            flags.unused |= FLAG_ATTACK;

        switch (state) {
            case STATE_STOP    :
                if (nextState != STATE_NONE)
                    return nextState;
                if (targetCanAttack && targetDist < GORILLA_DIST_ATTACK)
                    return STATE_ATTACK;
                if (!(flags.unused & FLAG_ATTACK) && zone == target->zone && targetInView) {
                    int r = rand() % 512;
                    if (r < 120) return STATE_JUMP;
                    if (r < 240) return STATE_IDLE1;
                    if (r < 360) return STATE_IDLE2;
                    if (r < 480) return STATE_RUN;
                    return (r % 2) ? STATE_LEFT : STATE_RIGHT;
                }
                return STATE_RUN;
            case STATE_RUN      :
                if (!flags.unused && targetInView)
                    return STATE_STOP;
                if (targetInView && (collide(target) & HIT_MASK)) {
                    nextState = STATE_ATTACK;
                    return STATE_STOP;
                }
                if (mood != MOOD_ESCAPE) {
                    int r = rand();
                    if (r < 160)
                        nextState = STATE_JUMP;
                    else if (r < 320)
                        nextState = STATE_IDLE1;
                    else if (r < 480)
                        nextState = STATE_IDLE2;
                    else
                        break;
                    return STATE_STOP;
                }
                break;
            case STATE_ATTACK :
                if (nextState == STATE_NONE && (collide(target) & HIT_MASK)) {
                    bite(15, vec3(0.0f, -19.0f, 75.0f), 200.0f);
                    nextState = STATE_STOP;
                }
                break;
            case STATE_LEFT  :
            case STATE_RIGHT :
                return STATE_STOP;
            default : ;
        }

        return state;
    }

    virtual void hit(float damage, Controller *enemy = NULL, TR::HitType hitType = TR::HIT_DEFAULT) {
        Enemy::hit(damage, enemy, hitType);
        flags.unused |= FLAG_ATTACK;
    };

    virtual int getStateDeath() {
        if (state == STATE_DEATH)
            return state;
        return animation.setAnim(ANIM_DEATH + rand() % 2);
    }

    void strafe(int dir) {
        switch (state) {
            case STATE_STOP :
                if (flags.unused & FLAG_LEFT)  angle.y += PI * 0.5f;
                if (flags.unused & FLAG_RIGHT) angle.y -= PI * 0.5f;
                flags.unused &= ~(FLAG_LEFT | FLAG_RIGHT);
                break;
            case STATE_LEFT :
                if (!(flags.unused & FLAG_LEFT)) {
                    flags.unused |= FLAG_LEFT;
                    angle.y -= PI * 0.5f;
                }
                break;
            case STATE_RIGHT :
                if (!(flags.unused & FLAG_RIGHT)) {
                    flags.unused |= FLAG_RIGHT;
                    angle.y += PI * 0.5f;
                }
                break;
            default : ;
        }
    }

    virtual void updateAnimation(bool commands) {
        Enemy::updateAnimation(commands);

        strafe(state);

        if ((state == STATE_LEFT || state == STATE_RIGHT) && animation.isPrepareToNext && animation.anims[animation.next].state == STATE_STOP)
            animation.rot = (state == STATE_LEFT ? -PI : PI) * 0.5f;
    }

    virtual void updatePosition() {
        turn(state == STATE_RUN, GORILLA_TURN_FAST);

        if (state == STATE_DEATH) {
            animation.overrideMask = 0;
            return;
        }

        vec3 old = pos;

        TR::Level::FloorInfo infoA, infoB;
        getFloorInfo(getRoomIndex(), old, infoA);
        old.y = infoA.floor;

        Enemy::updatePosition();

        getFloorInfo(getRoomIndex(), pos, infoB);

        if (infoB.floor < old.y - 384)
            climb(old);

        setOverrides(true, jointChest, jointHead);
        lookAt(target);
    }

    void climb(const vec3 &old) {
        int ox = int(old.x) / 1024;
        int oz = int(old.z) / 1024;

        int cx = int(pos.x) / 1024;
        int cz = int(pos.z) / 1024;

        if ((ox == cx) == (oz == cz))
            return;

        strafe(STATE_STOP);

        if (oz == cz) {
            if (ox < cx) {
                pos.x   = float(cx * 1024 - 75);
                angle.y = PI * 0.5f;
            } else {
                pos.x   = float(ox * 1024 + 75);
                angle.y = -PI * 0.5f;
            }
        } else {
            if (oz < cz) {
                pos.z   = float(cz * 1024 - 75);
                angle.y = 0.0f;
            } else {
                pos.z   = float(oz * 1024 + 75);
                angle.y = -PI;
            }
        }

        pos.y = old.y;
        animation.setAnim(ANIM_CLIMB);
    }
};


#define RAT_TURN_SLOW   (DEG2RAD * 90)
#define RAT_TURN_FAST   (DEG2RAD * 180)
#define RAT_DIST_BITE   341.0f
#define RAT_DIST_ATTACK 1536.0f
#define RAT_WAIT        0.01f
#define RAT_DAMAGE      20

struct Rat : Enemy {

    enum {
        HIT_MASK = 0x0300018F,
    };

    enum {
        ANIM_DEATH_LAND  = 8,
        ANIM_DEATH_WATER = 2,
    };

    enum {
    // land
        STATE_NONE   ,
        STATE_STOP   ,
        STATE_ATTACK ,
        STATE_RUN    ,
        STATE_BITE   ,
        STATE_DEATH  ,
        STATE_WAIT   ,
    // water
        STATE_WATER_SWIM = 1,
        STATE_WATER_BITE    ,
        STATE_WATER_DEATH   ,
    };

    int modelLand, modelWater;

    Rat(IGame *game, int entity) : Enemy(game, entity, 5, 204, 200.0f, 0.25f) {
        hitSound   = TR::SND_HIT_RAT;
        jointChest = 1;
        jointHead  = 2;

        modelLand  = level->getModelIndex(TR::Entity::ENEMY_RAT_LAND)  - 1;
        modelWater = level->getModelIndex(TR::Entity::ENEMY_RAT_WATER) - 1;
    }
    
    const virtual TR::Model* getModel() {
        bool water = getRoom().flags.water;
        int modelIndex = water ? modelWater : modelLand;

        if (modelIndex == -1) {
            water = modelWater != -1;
            modelIndex = water ? modelWater : modelLand;
        }

        ASSERT(modelIndex > -1);
        const TR::Model *model = &level->models[modelIndex];
        if (animation.model != model) {
            targetBox = TR::NO_BOX;
            animation.setModel(model);
            stand = water ? STAND_ONWATER : STAND_GROUND;

            int16 rIndex = getRoomIndex();
            if (water) {
                TR::Room::Sector *sector = level->getWaterLevelSector(rIndex, pos);
                if (sector) {
                    pos.y = float(sector->ceiling * 256);
                    roomIndex = rIndex;
                }
            } else {
                int16 rIndex = getRoomIndex();
                TR::Room::Sector *sector = level->getSector(rIndex, pos);
                if (sector) {
                    pos.y = float(sector->floor * 256);
                    roomIndex = rIndex;
                }
            }

            nextState = STATE_NONE;
            state     = STATE_NONE;

            if (health <= 0.0f) {
                getStateDeath();
                animation.goEnd(false);
            }

            updateZone();
        }
        return animation.model;
    }

    virtual int getStateGround() {
        if (!think(false))
            return state;

        if (nextState == state)
            nextState = STATE_NONE;

        switch (state) {
            case STATE_STOP :
                if (nextState != STATE_NONE)
                    return nextState;
                if (targetCanAttack && targetDist < RAT_DIST_BITE)
                    return STATE_BITE;
                return STATE_RUN;
            case STATE_RUN :
                if (targetInView && (collide(target) & HIT_MASK))
                    return STATE_STOP;
                if (targetCanAttack && targetDist < RAT_DIST_ATTACK)
                    return STATE_ATTACK;
                if (targetInView && randf() < RAT_WAIT) {
                    nextState = STATE_WAIT;
                    return STATE_STOP;
                }
                break;
            case STATE_ATTACK :
            case STATE_BITE   :
                if (nextState == STATE_NONE && targetInView && (collide(target) & HIT_MASK)) {
                    bite(3, vec3(0.0f, -11.0f, 108.0f), RAT_DAMAGE);
                    nextState = state == STATE_ATTACK ? STATE_RUN : STATE_STOP;
                }
                break;
            case STATE_WAIT :
                if (mood == MOOD_SLEEP || randf() < RAT_WAIT)
                    return STATE_STOP;
            default : ;
        }

        return state;
    }

    virtual int getStateOnwater() {
        if (!think(false))
            return state;

        if (nextState == state)
            nextState = STATE_NONE;

        if (animation.frameIndex % 4 == 0)
            game->waterDrop(getJoint(jointHead).pos, 96.0f, 0.02f);

        switch (state) {
            case STATE_WATER_SWIM :
                if (targetInView && (collide(target) & HIT_MASK))
                    return STATE_WATER_BITE;
                break;
            case STATE_WATER_BITE :
                if (nextState == STATE_NONE && targetInView && (collide(target) & HIT_MASK)) {
                    game->waterDrop(getJoint(jointHead).pos, 256.0f, 0.2f);
                    bite(3, vec3(0.0f, -11.0f, 108.0f), RAT_DAMAGE);
                    nextState = STATE_WATER_SWIM;
                }
                return STATE_NONE;
            default : ;
        }

        return state;
    }

    virtual void updatePosition() {
        turn((stand == STAND_GROUND && state == STATE_RUN) || (stand == STAND_ONWATER && state == STATE_WATER_SWIM), RAT_TURN_FAST);

        if (state == STATE_DEATH) {
            animation.overrideMask = 0;
            return;
        }

        Enemy::updatePosition();
        setOverrides(state != STATE_DEATH, jointChest, jointHead);
        lookAt(target);
    }

    virtual int getStateDeath() {
        bool water = getRoom().flags.water;
        if ((water && state == STATE_WATER_DEATH) || (!water && state == STATE_DEATH))
            return state;
        return animation.setAnim(water ? ANIM_DEATH_WATER : ANIM_DEATH_LAND);
    }
};


#define CROCODILE_TURN_SLOW   (DEG2RAD * 90)
#define CROCODILE_TURN_FAST   (DEG2RAD * 180)
#define CROCODILE_DIST_BITE   435.0f
#define CROCODILE_DIST_TURN   (1024 * 3)
#define CROCODILE_LIFT_SPEED  960.0f
#define CROCODILE_DAMAGE      25

struct Crocodile : Enemy {

    enum {
        HIT_MASK = 0x000003FC,
    };

    enum {
        ANIM_DEATH_LAND  = 11,
        ANIM_DEATH_WATER = 4,
    };

    enum {
    // land
        STATE_NONE   ,
        STATE_STOP   ,
        STATE_RUN    ,
        STATE_WALK   ,
        STATE_TURN   ,
        STATE_BITE   ,
        STATE_UNUSED ,
        STATE_DEATH  ,
    // water
        STATE_WATER_SWIM = 1,
        STATE_WATER_BITE    ,
        STATE_WATER_DEATH   ,
    };

    int modelLand, modelWater;

    Crocodile(IGame *game, int entity) : Enemy(game, entity, 20, 341, 600.0f, 0.25f) {
        jointChest = 1;
        jointHead  = 8;

        modelLand  = level->getModelIndex(TR::Entity::ENEMY_CROCODILE_LAND)  - 1;
        modelWater = level->getModelIndex(TR::Entity::ENEMY_CROCODILE_WATER) - 1;

        bool water = getRoom().flags.water;
        flying     = water;
        stand      = water ? STAND_UNDERWATER : STAND_GROUND;
    }

    const virtual TR::Model* getModel() {
        bool water = getRoom().flags.water;
        int modelIndex = water ? modelWater : modelLand;

        if (modelIndex == -1) {
            water = modelWater != -1;
            modelIndex = water ? modelWater : modelLand;
        }

        ASSERT(modelIndex > -1);
        const TR::Model *model = &level->models[modelIndex];
        if (animation.model != model) {
            targetBox = TR::NO_BOX;
            animation.setModel(model);
            stand  = water ? STAND_UNDERWATER : STAND_GROUND;
            flying = water;

            int16 rIndex = getRoomIndex();
            if (water) {
                TR::Room::Sector *sector = level->getWaterLevelSector(rIndex, pos);
                if (sector) {
                    pos.y = float(sector->ceiling * 256);
                    roomIndex = rIndex;
                }
            } else {
                int16 rIndex = getRoomIndex();
                TR::Room::Sector *sector = level->getSector(rIndex, pos);
                if (sector) {
                    pos.y = float(sector->floor * 256);
                    roomIndex = rIndex;
                }
            }

            nextState = STATE_NONE;
            state     = STATE_NONE;

            if (health <= 0.0f) {
                getStateDeath();
                animation.goEnd(false);
            }

            updateZone();
        }
        return animation.model;
    }

    virtual int getStateGround() {
        if (!think(true))
            return state;

        if (nextState == state)
            nextState = STATE_NONE;

        switch (state) {
            case STATE_STOP :
                if (targetCanAttack && targetDist < CROCODILE_DIST_BITE)
                    return STATE_BITE;
                switch (mood) {
                    case MOOD_ESCAPE : return STATE_RUN;
                    case MOOD_ATTACK : return (targetInView || targetDist < CROCODILE_DIST_TURN) ? STATE_RUN : STATE_TURN;
                    case MOOD_STALK  : return STATE_WALK;
                    default          : return state;
                }
            case STATE_RUN  :
                if (targetInView && (collide(target) & HIT_MASK))
                    return STATE_STOP;
                switch (mood) {
                    case MOOD_SLEEP  : return STATE_STOP;
                    case MOOD_STALK  : return STATE_WALK;
                    case MOOD_ATTACK : if (targetDist > CROCODILE_DIST_TURN && !targetInView) return STATE_STOP;
                    default          : return state;
                }
            case STATE_WALK :
                if (targetInView && (collide(target) & HIT_MASK))
                    return STATE_STOP;
                switch (mood) {
                    case MOOD_SLEEP  : return STATE_STOP;
                    case MOOD_ATTACK :
                    case MOOD_ESCAPE : return STATE_RUN;
                    default          : return state;
                }
            case STATE_TURN :
                return targetInView ? STATE_WALK : state;
            case STATE_BITE   :
                if (nextState == STATE_NONE) {
                    bite(9, vec3(5.0f, -21.0f, 467.0f), CROCODILE_DAMAGE);
                    nextState = STATE_STOP;
                }
                break;
            default : ;
        }

        return state;
    }

    virtual int getStateUnderwater() {
        if (!think(false))
            return state;

        if (nextState == state)
            nextState = STATE_NONE;

        if (animation.frameIndex % 4 == 0)
            game->waterDrop(getJoint(jointHead).pos, 96.0f, 0.02f);

        switch (state) {
            case STATE_WATER_SWIM :
                if (targetInView && collide(target))
                    return STATE_WATER_BITE;
                break;
            case STATE_WATER_BITE :
                if (collide(target)) {
                    if (nextState != STATE_NONE)
                        return state;
                    bite(9, vec3(5.0f, -21.0f, 467.0f), CROCODILE_DAMAGE);
                    nextState = STATE_WATER_SWIM;
                }
                return STATE_WATER_SWIM;
            default : ;
        }

        return state;
    }

    virtual void updatePosition() {
        if (state == STATE_TURN)
            angle.y += CROCODILE_TURN_FAST * Core::deltaTime;
        else
            turn((stand == STAND_GROUND && (state == STATE_RUN || state == STATE_WALK)) || (stand == STAND_UNDERWATER && state == STATE_WATER_SWIM), CROCODILE_TURN_FAST);
        angle.z = 0.0f;

        if (state == STATE_DEATH) {
            animation.overrideMask = 0;
            return;
        }

        if (flying) {
            lift(waypoint.y - pos.y, CROCODILE_LIFT_SPEED);
            int16 rIndex = getRoomIndex();
            TR::Room::Sector *sector = level->getWaterLevelSector(rIndex, pos);
            if (sector) {
                float waterLevel = float(sector->ceiling * 256) + 256;
                if (pos.y < waterLevel)
                    pos.y = waterLevel;
            }
        }

        Enemy::updatePosition();
        setOverrides(state != STATE_DEATH, jointChest, jointHead);
        lookAt(target);
    }

    virtual int getStateDeath() {
        bool water = getRoom().flags.water;
        if ((water && state == STATE_WATER_DEATH) || (!water && state == STATE_DEATH))
            return state;
        return animation.setAnim(water ? ANIM_DEATH_WATER : ANIM_DEATH_LAND);
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
                    bite(14, vec3(0.0f, 96.0f, 335.0f), state == STATE_BITE ? 200.0f : 400.0f);
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
        turn(state == STATE_RUN || state == STATE_WALK || state == STATE_HIND, state == STATE_RUN ? BEAR_TURN_FAST : BEAR_TURN_SLOW);

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
                    bite(4, vec3(0.0f, 16.0f, 45.0f), 2);
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
        turn(state == STATE_FLY || state == STATE_ATTACK, BAT_TURN_SPEED);

        if (flying) {
            float wy = waypoint.y - (target->stand != STAND_ONWATER ? 765.0f : 64.0f);
            lift(wy - pos.y, BAT_LIFT_SPEED);
        }
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
#define REX_DAMAGE_WALK     1
#define REX_DAMAGE_RUN      10

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
                if (mask) target->hit(REX_DAMAGE_WALK, this);
                if (mood != MOOD_SLEEP && !walk)    return STATE_STOP;
                if (targetInView && randf() < 0.015f) {
                    nextState = STATE_BAWL;
                    return STATE_STOP;
                }
                break;
            case STATE_RUN :
                if (mask) target->hit(REX_DAMAGE_RUN, this);
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

        turn(state == STATE_RUN || state == STATE_WALK, state == STATE_RUN ? REX_TURN_FAST : REX_TURN_SLOW);

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
                    bite(22, vec3(0.0f, 66.0f, 318.0f), 100);                    
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

        turn(state == STATE_RUN || state == STATE_WALK, state == STATE_RUN ? RAPTOR_TURN_FAST : RAPTOR_TURN_SLOW);
        
        Enemy::updatePosition();
        setOverrides(true, jointChest, jointHead);
        lookAt(target);
    }
};


#define MUTANT_TURN_FAST        (DEG2RAD * 180)
#define MUTANT_TURN_SLOW        (DEG2RAD * 60)
#define MUTANT_LIFT_SPEED       512.0f
#define MUTANT_DIST_ATTACK_1    600
#define MUTANT_DIST_ATTACK_2    (2048 + 512)
#define MUTANT_DIST_ATTACK_3    300
#define MUTANT_DIST_SHOT        3840
#define MUTANT_DIST_STALK       (4096 + 512)
#define MUTANT_PART_DAMAGE      100

struct Mutant : Enemy {

    enum {
        HIT_MASK = 0x0678,
    };

    enum {
        STATE_NONE,
        STATE_STOP,
        STATE_WALK,
        STATE_RUN,
        STATE_ATTACK_1,
        STATE_DEATH,
        STATE_LOOKING,
        STATE_ATTACK_2,
        STATE_ATTACK_3,
        STATE_AIM_1,
        STATE_AIM_2,
        STATE_FIRE,
        STATE_IDLE,
        STATE_FLY,
    };

    enum {
        FLAG_FLY      = 1,
        FLAG_BULLET_1 = 2,
        FLAG_BULLET_2 = 4,
    };

    Mutant(IGame *game, int entity) : Enemy(game, entity, 50, 341, 150.0f, 1.0f) {
        if (getEntity().type != TR::Entity::ENEMY_MUTANT_1) {
            initMeshOverrides();
            layers[0].mask = 0xffe07fff;
            aggression     = 0.25f;
        }

        flags.unused = 0;
        jointChest   = 1;
        jointHead    = 2;
    }

    virtual void setSaveData(const SaveEntity &data) {
        Character::setSaveData(data);
        if (flags.invisible)
            deactivate(true);
    }

    virtual void update() {
        bool exploded = explodeMask != 0;

        if (health <= 0.0f && !exploded) {
            game->playSound(TR::SND_MUTANT_DEATH, pos, Sound::PAN);
            explode(0xffffffff, MUTANT_PART_DAMAGE);
        }

        Enemy::update();

        if (exploded && !explodeMask) {
            deactivate(true);
            flags.invisible = true;
        }
    }

    virtual int getStateGround() {
        if (state == STATE_FLY) {
            stand  = STAND_AIR;
            flying = true;
            updateZone();
            return getStateAir();
        }

        stepHeight = 256;
        dropHeight = -stepHeight;

        if (!think(true))
            return state;

        if (nextState == state)
            nextState = STATE_NONE;

        if (getEntity().type != TR::Entity::ENEMY_MUTANT_3) {
            if (flags.unused & (FLAG_BULLET_1 | FLAG_BULLET_2)) {
                if (targetAngle > PI * 0.25f)
                    flags.unused &= ~(FLAG_BULLET_1 | FLAG_BULLET_2);
            } else {
                if (targetAngle < PI * 0.25f && state != STATE_FIRE && (targetDist > MUTANT_DIST_SHOT || zone != target->zone) && targetIsVisible(MAX_SHOT_DIST))
                    flags.unused |= (rand() % 2) ? FLAG_BULLET_1 : FLAG_BULLET_2;
            }
        }

        if (getEntity().type == TR::Entity::ENEMY_MUTANT_1) { // flying mutant
            if (mood == MOOD_ESCAPE || (zone != target->zone && !(flags.unused & (FLAG_BULLET_1 | FLAG_BULLET_2)))) {
                flags.unused |= FLAG_FLY;
            }
        }

        int mask = collide(target);

        switch (state) {
            case STATE_STOP :
                if (flags.unused & FLAG_FLY)
                    return STATE_FLY;
                if ((targetCanAttack && targetDist < MUTANT_DIST_ATTACK_3) || (mask & HIT_MASK))
                    return STATE_ATTACK_3;
                if ((targetCanAttack && targetDist < MUTANT_DIST_ATTACK_1))
                    return STATE_ATTACK_1;
                if (flags.unused & FLAG_BULLET_1)
                    return STATE_AIM_1;
                if (flags.unused & FLAG_BULLET_2)
                    return STATE_AIM_2;
                if (mood == MOOD_SLEEP || (mood == MOOD_STALK && targetDist < MUTANT_DIST_STALK))
                    return STATE_LOOKING;
                return STATE_RUN;
            case STATE_WALK :
                if (flags.unused)
                    return STATE_STOP;
                if (mood == MOOD_ATTACK || mood == MOOD_ESCAPE)
                    return STATE_STOP;
                if (mood == MOOD_SLEEP || (mood == MOOD_STALK && target->zone != zone)) {
                    if (rand() < 50)
                        return STATE_LOOKING;
                } else if (mood == MOOD_STALK && targetDist > MUTANT_DIST_STALK)
                    return STATE_STOP;
                break;
            case STATE_RUN :
                if (flags.unused & FLAG_FLY)
                    return STATE_STOP;
                if (mask & HIT_MASK)
                    return STATE_STOP;
                if (targetCanAttack && targetDist < MUTANT_DIST_ATTACK_1)
                    return STATE_STOP;
                if (targetInView && targetDist < MUTANT_DIST_ATTACK_2)
                    return STATE_ATTACK_2;
                if (flags.unused & (FLAG_BULLET_1 | FLAG_BULLET_2))
                    return STATE_STOP;
                if (mood == MOOD_SLEEP || (mood == MOOD_STALK && targetDist < MUTANT_DIST_STALK))
                    return STATE_STOP;
                break;
            case STATE_LOOKING :
                if (flags.unused)
                    return STATE_STOP;
                switch (mood) {
                    case MOOD_SLEEP :
                        if (rand() < 256)
                            return STATE_WALK;
                        break;
                    case MOOD_STALK :
                        if (targetDist < MUTANT_DIST_STALK) {
                            if (target->zone == zone && rand() < 256)
                                return STATE_WALK;
                        } else
                            return STATE_STOP;
                        break;
                    case MOOD_ATTACK :
                    case MOOD_ESCAPE :
                        return STATE_STOP;
                }
            case STATE_ATTACK_1 :
            case STATE_ATTACK_2 :
            case STATE_ATTACK_3 :
                if (nextState == STATE_NONE && (mask & HIT_MASK)) {
                    float damage = state == STATE_ATTACK_1 ? 150.0f : (state == STATE_ATTACK_2 ? 100.0f : 200.0f);
                    bite(10, vec3(-27.0f, 98.0f, 0.0f), damage);
                    nextState = STATE_STOP;
                }
                break;
            case STATE_AIM_1 :
                return (flags.unused & FLAG_BULLET_1) ? STATE_FIRE : STATE_STOP;
            case STATE_AIM_2 :
                return (flags.unused & FLAG_BULLET_2) ? STATE_FIRE : STATE_STOP;
            case STATE_FIRE :
                if (flags.unused & FLAG_BULLET_1)
                    shot(TR::Entity::MUTANT_BULLET, 9, vec3(-35.0f, 269.0f, 0.0f));
                if (flags.unused & FLAG_BULLET_2)
                    shot(TR::Entity::CENTAUR_BULLET, 14, vec3(51.0f, 213.0f, 0.0f));
                flags.unused &= ~(FLAG_BULLET_1 | FLAG_BULLET_2);
                break;
            case STATE_IDLE :
                return STATE_STOP;
            default : ;
        }

        return state;
    }

    virtual int getStateAir() {
        if (state != STATE_FLY) {
            stand  = STAND_GROUND;
            flying = false;
            updateZone();
            return getStateGround();
        }

        stepHeight = 30 * 1024;
        dropHeight = -stepHeight;

        if (!think(true))
            return state;

        if ((flags.unused & FLAG_FLY) && mood != MOOD_ESCAPE && zone == target->zone)
            flags.unused &= ~FLAG_FLY;

        if (!(flags.unused & FLAG_FLY)) {
            int16 roomIndex = getRoomIndex();
            TR::Room::Sector *sector = level->getSector(roomIndex, pos);
            float floor = level->getFloor(sector, pos) - 128.0f;
            if (pos.y >= floor)
                return STATE_STOP;
        }

        return STATE_FLY;
    }

    virtual void updatePosition() {
        turn(state == STATE_RUN || state == STATE_WALK || state == STATE_FLY, (state == STATE_RUN || state == STATE_FLY) ? MUTANT_TURN_FAST : MUTANT_TURN_SLOW);

        if (flying)
            lift(target->pos.y - pos.y, MUTANT_LIFT_SPEED);

        Enemy::updatePosition();
        setOverrides(true, jointChest, jointHead);
        lookAt(target);
    }
};

#define GIANT_MUTANT_TURN_SLOW    (DEG2RAD * 90)
#define GIANT_MUTANT_MIN_ANGLE    (DEG2RAD * 10)
#define GIANT_MUTANT_MAX_ANGLE    (DEG2RAD * 45)
#define GIANT_MUTANT_DAMAGE       500
#define GIANT_MUTANT_DAMAGE_WALK  5
#define GIANT_MUTANT_DAMAGE_FATAL 1000
#define GIANT_MUTANT_DIST_ATTACK  2600
#define GIANT_MUTANT_DIST_FATAL   2250
#define GIANT_MUTANT_PART_DAMAGE  250

struct GiantMutant : Enemy {

    enum {
        HIT_MASK_HAND  = 0x3FF8000,
        HIT_MASK_HANDS = 0x3FFFFF0,
    };

    enum {
        ANIM_DEATH = 13,
    };

    enum {
        STATE_NONE,
        STATE_STOP,
        STATE_TURN_LEFT,
        STATE_TURN_RIGHT,
        STATE_ATTACK_1,
        STATE_ATTACK_2,
        STATE_ATTACK_3,
        STATE_WALK,
        STATE_BORN,
        STATE_FALL,
        STATE_UNUSED,
        STATE_FATAL,
    };

    GiantMutant(IGame *game, int entity) : Enemy(game, entity, 500, 341, 375.0f, 1.0f) {
        hitSound   = TR::SND_HIT_MUTANT;
        stand      = STAND_AIR;
        jointChest = -1;
        jointHead  = -1; // 3; TODO: fix head orientation
        rangeHead  = vec4(-0.5f, 0.5f, -0.5f, 0.5f) * PI;
        invertAim  = true;
    }

    virtual void setSaveData(const SaveEntity &data) {
        Character::setSaveData(data);
        if (flags.invisible)
            deactivate(true);
    }

    void update() {
        bool exploded = explodeMask != 0;

        Enemy::update();

        if (health <= 0.0f && !exploded && animation.index == ANIM_DEATH && flags.state == TR::Entity::asInactive) {
            flags.state = TR::Entity::asActive;
            game->playSound(TR::SND_MUTANT_DEATH, pos, Sound::PAN);
            explode(0xffffffff, GIANT_MUTANT_PART_DAMAGE);
            game->checkTrigger(this, true);
        }

        setOverrides(true, jointChest, jointHead);
        lookAt(target);

        if (exploded && !explodeMask) {
            deactivate(true);
            flags.invisible = true;
        }
    }

    virtual int getStateAir() {
        if (state == STATE_BORN)
            return STATE_FALL;
        return state;
    }

    virtual int getStateGround() {
        if (health <= 0)
            return state;

        if (!think(true))
            return state;

        if (!target || target->health <= 0.0f)
            return STATE_STOP;

        int mask = collide(target);

        if (mask) target->hit(GIANT_MUTANT_DAMAGE_WALK, this);

        switch (state) {
            case STATE_FALL :
                animation.setState(STATE_STOP);
                game->shakeCamera(5.0f);
                break;
            case STATE_STOP :
                flags.unused = false;
                if (targetAngle >  GIANT_MUTANT_MAX_ANGLE) return STATE_TURN_RIGHT;
                if (targetAngle < -GIANT_MUTANT_MAX_ANGLE) return STATE_TURN_LEFT;
                if (targetDist < GIANT_MUTANT_DIST_ATTACK) {
                    if (target->health <= GIANT_MUTANT_DAMAGE) {
                        if (targetDist < GIANT_MUTANT_DIST_FATAL)
                            return STATE_ATTACK_3;
                    } else
                        return ((rand() % 2) ? STATE_ATTACK_1 : STATE_ATTACK_2);
                }
                return STATE_WALK;
            case STATE_WALK :
                if (targetDist  <  GIANT_MUTANT_DIST_ATTACK ||
                    targetAngle >  GIANT_MUTANT_MAX_ANGLE   ||
                    targetAngle < -GIANT_MUTANT_MAX_ANGLE)
                    return STATE_STOP;
                break;
            case STATE_TURN_RIGHT :
                if (targetAngle < GIANT_MUTANT_MAX_ANGLE)
                    return STATE_STOP;
                break;
            case STATE_TURN_LEFT :
                if (targetAngle > -GIANT_MUTANT_MAX_ANGLE)
                    return STATE_STOP;
                break;
            case STATE_ATTACK_1 :
            case STATE_ATTACK_2 :
                if (!flags.unused && (
                    (state == STATE_ATTACK_1 && (mask & HIT_MASK_HAND)) ||
                    (state == STATE_ATTACK_2 && (mask & HIT_MASK_HANDS)))) {
                    target->hit(GIANT_MUTANT_DAMAGE, this);
                    flags.unused = true;
                }
                break;
            case STATE_ATTACK_3 :
                if (target->stand != STAND_HANG) {
                    target->hit(GIANT_MUTANT_DAMAGE_FATAL, this, TR::HIT_GIANT_MUTANT);
                    return STATE_FATAL;
                }
                break;
            default : ;
        }
        
        return state;
    }

    virtual int getStateDeath() {
        if (animation.index != ANIM_DEATH)
            return animation.setAnim(ANIM_DEATH);
        return state;
    }

    virtual void updatePosition() {
        turn(false, GIANT_MUTANT_TURN_SLOW);

        Enemy::updatePosition();
        //setOverrides(true, jointChest, jointHead);
        //lookAt(target);
    }
};

#define CENTAUR_TURN_FAST   (DEG2RAD * 120)
#define CENTAUR_DIST_RUN    (1024 + 512)
#define CENTAUR_PART_DAMAGE 100

struct Centaur : Enemy {

    enum {
        HIT_MASK = 0x030199,
    };

    enum {
        ANIM_DEATH = 8,
    };

    enum {
        STATE_NONE,
        STATE_STOP,
        STATE_FIRE,
        STATE_RUN,
        STATE_AIM,
        STATE_DEATH,
        STATE_IDLE,
    };

    Centaur(IGame *game, int entity) : Enemy(game, entity, 120, 341, 400.0f, 1.0f) {
        jointChest = 10;
        jointHead  = 17;
    }

    virtual void setSaveData(const SaveEntity &data) {
        Character::setSaveData(data);
        if (flags.invisible)
            deactivate(true);
    }

    virtual int getStateGround() {
        if (!think(true))
            return state;

        if (nextState == state)
            nextState = STATE_NONE;

        switch (state) {
            case STATE_STOP :
                if (nextState != STATE_NONE)
                    return nextState;
                if (targetCanAttack && targetDist < CENTAUR_DIST_RUN)
                    return STATE_RUN;
                if (targetIsVisible(MAX_SHOT_DIST))
                    return STATE_AIM;
                return STATE_RUN;
            case STATE_RUN :
                if (targetCanAttack && targetDist < CENTAUR_DIST_RUN) {
                    nextState = STATE_IDLE;
                    return STATE_STOP;
                }
                if (targetIsVisible(MAX_SHOT_DIST)) {
                    nextState = STATE_AIM;
                    return STATE_STOP;
                }
                if (rand() < 96) {
                    nextState = STATE_IDLE;
                    return STATE_STOP;
                }
                break;
            case STATE_AIM :
                if (nextState != STATE_NONE)
                    return nextState;
                if (targetIsVisible(MAX_SHOT_DIST))
                    return STATE_FIRE;
                return STATE_STOP;
            case STATE_FIRE :
                if (nextState != STATE_NONE)
                    break;
                nextState = STATE_AIM;
                shot(TR::Entity::CENTAUR_BULLET, 13, vec3(11.0f, 415.0f, 41.0f));
                break;
            case STATE_IDLE :
                if (nextState == STATE_NONE && (collide(target) & HIT_MASK)) {
                    bite(5, vec3(50.0f, 30.0f, 0.0f), 200);
                    nextState = STATE_STOP;
                }
                break;
        }

        return state;
    }

    virtual int getStateDeath() {
        if (state == STATE_DEATH) return state;
        return animation.setAnim(ANIM_DEATH);
    }

    virtual void updatePosition() {
        turn(state == STATE_RUN, CENTAUR_TURN_FAST);

        Enemy::updatePosition();
        setOverrides(true, jointChest, jointHead);
        lookAt(target);
    }

    virtual void deactivate(bool removeFromList = false) {
        if (!removeFromList) {
            if (!explodeMask)
                explode(0xffffffff, CENTAUR_PART_DAMAGE);
            return;
        }
        Enemy::deactivate(removeFromList);
    }

    virtual void update() {
        bool exploded = explodeMask != 0;

        Enemy::update();

        if (exploded && !explodeMask) {
            deactivate(true);
            flags.invisible = true;
        }
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
        if (state == STATE_IDLE && (health <= 0.0f || collide(game->getLara(pos)))) {
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
        enemy->hit(damage * 10, this);
    };

    virtual void update() {
        if (!target)
            target = (Character*)game->getLara(pos);

        if (stand != STAND_AIR) {
            pos      = DOPPELGANGER_ROOM_CENTER * 2.0f - target->pos;
            pos.y    = target->pos.y;
            angle    = target->angle;
            angle.y -= PI;
        }

        Enemy::updateRoom();

        TR::Level::FloorInfo info;
        getFloorInfo(getRoomIndex(), target->pos, info);
        float laraHeight = info.floor - target->pos.y;

        getFloorInfo(getRoomIndex(), pos, info);
        float selfHeight = info.floor - pos.y;

        if (stand != STAND_AIR && target->stand == Character::STAND_GROUND && selfHeight > 1024 && laraHeight < 256) {
            animation = Animation(level, target->getModel());
            animation.setAnim(ANIM_FALL, 1);
            stand = STAND_AIR;
            velocity.x = velocity.y = 0.0f;
        }

        if (stand == STAND_AIR) {
            if (selfHeight < 128.0f) {
                game->checkTrigger(this, true);
                flags.invisible = true;
                deactivate(true);
            } else {
                updateAnimation(true);
                applyGravity(velocity.y);
                pos += velocity * (30.0f * Core::deltaTime);
            }
        } else {
            animation.frameA = target->animation.frameA;
            animation.frameB = target->animation.frameB;
            animation.delta  = target->animation.delta;
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
    ScionTarget(IGame *game, int entity) : Enemy(game, entity, 5, 0, 0, 0) {}

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
                    game->shakeCamera(0.5f);
                }

                if (timer < 0.0f) 
                    deactivate(true);
            }
        }
    }
};

#define HUMAN_WAIT       0.01f
#define HUMAN_DIST_WALK  (1024 * 3)
#define HUMAN_DIST_SHOT  (1024 * 7)
#define HUMAN_TURN_SLOW  (DEG2RAD * 90)
#define HUMAN_TURN_FAST  (DEG2RAD * 180)

struct Human : Enemy {
    enum {
        STATE_NONE,
        STATE_STOP,
        STATE_WALK,
        STATE_RUN,
        STATE_AIM,
        STATE_DEATH,
        STATE_WAIT, // == STATE_FIRE for MrT and Cowboy
        STATE_FIRE
    };

    int jointGun;
    int animDeath;

    Human(IGame *game, int entity, float health) : Enemy(game, entity, health, 100, 375.0f, 1.0f), animDeath(-1) {
        jointGun   = 0;
        jointChest = 7;
        jointHead  = 8;
    }

    virtual void deactivate(bool removeFromList = false) {
        if (health <= 0.0f)
            onDead();
        Enemy::deactivate(removeFromList);
        getRoom().removeDynLight(entity);
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
        turn(state == STATE_RUN || state == STATE_WALK, state == STATE_RUN ? HUMAN_TURN_FAST : HUMAN_TURN_SLOW);
        
        Enemy::updatePosition();
        setOverrides(true, jointChest, jointHead);
        lookAt(target);
    }

    virtual void onDead() {}

    bool doShot(float damage, const vec3 &muzzleOffset) {
        game->addMuzzleFlash(this, jointGun, muzzleOffset, -1);

        if (targetDist < HUMAN_DIST_SHOT && randf() < ((HUMAN_DIST_SHOT - targetDist) / HUMAN_DIST_SHOT - 0.25f)) {
            bite(-1, vec3(0.0f), damage);
            game->addEntity(TR::Entity::BLOOD, target->getRoomIndex(), target->getJoint(rand() % target->getModel()->mCount).pos);
            game->playSound(target->stand == STAND_UNDERWATER ? TR::SND_HIT_UNDERWATER : TR::SND_HIT, target->pos, Sound::PAN);
            return true;
        }

        int16 roomIndex = getRoomIndex();
        TR::Room::Sector *sector = level->getSector(roomIndex, target->pos);
        float floor = level->getFloor(sector, target->pos) - 64.0f;
        vec3 p = vec3(target->pos.x + randf() * 512.0f - 256.0f, floor, target->pos.z + randf() * 512.0f - 256.0f);

        target->addRicochet(p, true);
        return false;
    }
};


#define LARSON_DAMAGE 50

struct Larson : Human {

    Larson(IGame *game, int entity) : Human(game, entity, 50) {
        animDeath = 15;
        jointGun  = 14;
    }

    virtual int getStateGround() {
        if (!think(false))
            return state;

        fullChestRotation = state == STATE_FIRE || state == STATE_AIM;

        if (nextState == state)
            nextState = STATE_NONE;

        switch (state) {
            case STATE_STOP :
                if (nextState != STATE_NONE)
                    return nextState;
                if (mood == MOOD_SLEEP)
                    return randf() < HUMAN_WAIT ? STATE_WAIT : STATE_WALK;
                if (mood == MOOD_ESCAPE)
                    return STATE_RUN;
                return STATE_WALK;
            case STATE_WAIT : 
                if (mood != MOOD_SLEEP)
                    return STATE_STOP;
                if (randf() < HUMAN_WAIT) {
                    nextState = STATE_WALK;
                    return STATE_STOP;
                }
                break;
            case STATE_WALK :
                if (mood == MOOD_SLEEP && randf() < HUMAN_WAIT)
                    nextState = STATE_WAIT;
                else if (mood == MOOD_ESCAPE)
                    nextState = STATE_RUN;
                else if (targetIsVisible(HUMAN_DIST_SHOT))
                    nextState = STATE_AIM;
                else if (!targetInView || targetDist > HUMAN_DIST_WALK)
                    nextState = STATE_RUN;
                else
                    break;
                return STATE_STOP;
            case STATE_RUN :
                if (mood == MOOD_SLEEP && randf() < HUMAN_WAIT)
                    nextState = STATE_WAIT;
                else if (targetIsVisible(HUMAN_DIST_SHOT))
                    nextState = STATE_AIM;
                else if (targetInView && targetDist < HUMAN_DIST_WALK)
                    nextState = STATE_WALK;
                else
                    break;
                return STATE_STOP;
            case STATE_AIM :
                if (nextState != STATE_NONE)
                    return nextState;
                if (targetIsVisible(HUMAN_DIST_SHOT))
                    return STATE_FIRE;
                return STATE_STOP;
            case STATE_FIRE :
                if (nextState == STATE_NONE) {
                    doShot(LARSON_DAMAGE, vec3(-50, 0, 20));
                    nextState = STATE_AIM;
                }
                if (mood == MOOD_ESCAPE || target->health <= 0.0f)
                    nextState = STATE_STOP;
                break;
        }

        return state;
    }
};


#define PIERRE_MIN_HEALTH   40
#define PIERRE_DAMAGE       25

struct Pierre : Human {

    Pierre(IGame *game, int entity) : Human(game, entity, 70) {
        animDeath = 12;
    }

    virtual void onDead() {
        game->addEntity(TR::Entity::MAGNUMS,           getRoomIndex(), pos, 0);
        game->addEntity(TR::Entity::SCION_PICKUP_DROP, getRoomIndex(), pos, 0);
        game->addEntity(TR::Entity::KEY_ITEM_1,        getRoomIndex(), pos, 0);
    }

    virtual int getStateGround() {
        if (!think(false))
            return state;

        if (!flags.once && health <= PIERRE_MIN_HEALTH) {
            health = PIERRE_MIN_HEALTH;
            timer += Core::deltaTime;
        }

        if (timer > 0.0f && isVisible()) // time to run away!
            timer = 0.0f;

        if (getRoom().flags.water)
            timer = 1.0f;

        if (timer > 0.4f) {
            flags.invisible = true;
            deactivate(true);
        }

        fullChestRotation = state == STATE_FIRE || state == STATE_AIM;

        if (nextState == state)
            nextState = STATE_NONE;

        switch (state) {
            case STATE_STOP :
                if (nextState != STATE_NONE)
                    return nextState;
                if (mood == MOOD_SLEEP)
                    return randf() < HUMAN_WAIT ? STATE_WAIT : STATE_WALK;
                if (mood == MOOD_ESCAPE)
                    return STATE_RUN;
                return STATE_WALK;
            case STATE_WAIT : 
                if (mood != MOOD_SLEEP)
                    return STATE_STOP;
                if (randf() < HUMAN_WAIT) {
                    nextState = STATE_WALK;
                    return STATE_STOP;
                }
                break;
            case STATE_WALK :
                if (mood == MOOD_SLEEP && randf() < HUMAN_WAIT)
                    nextState = STATE_WAIT;
                else if (mood == MOOD_ESCAPE)
                    nextState = STATE_RUN;
                else if (targetIsVisible(HUMAN_DIST_SHOT))
                    nextState = STATE_AIM;
                else if (!targetInView || targetDist > HUMAN_DIST_WALK)
                    nextState = STATE_RUN;
                else
                    break;
                return STATE_STOP;
            case STATE_RUN :
                if (mood == MOOD_SLEEP && randf() < HUMAN_WAIT)
                    nextState = STATE_WAIT;
                else if (targetIsVisible(HUMAN_DIST_SHOT))
                    nextState = STATE_AIM;
                else if (targetInView && targetDist < HUMAN_DIST_WALK)
                    nextState = STATE_WALK;
                else
                    break;
                return STATE_STOP;
            case STATE_AIM :
                if (nextState != STATE_NONE)
                    return nextState;
                if (targetIsVisible(HUMAN_DIST_SHOT))
                    return STATE_FIRE;
                return STATE_STOP;
            case STATE_FIRE :
                if (nextState == STATE_NONE) {
                    jointGun = 11; doShot(PIERRE_DAMAGE, vec3(60, 0, 50));
                    jointGun = 14; doShot(PIERRE_DAMAGE, vec3(-60, 0, 50));
                    nextState = STATE_AIM;
                }
                if (mood == MOOD_ESCAPE && (rand() % 2))
                    nextState = STATE_STOP;
                break;
        }

        return state;
    }
};


#define SKATERBOY_DIST_MIN      2560
#define SKATERBOY_DIST_MAX      4096
#define SKATERBOY_TURN_FAST     (120 * DEG2RAD)
#define SKATERBOY_DAMAGE_STAND  50.0f
#define SKATERBOY_DAMAGE_MOVE   40.0f

struct SkaterBoy : Human {

    enum {
        STATE_STOP,
        STATE_STAND_FIRE,
        STATE_MOVE,
        STATE_STEP,
        STATE_MOVE_FIRE,
        STATE_DEATH
    };

    Controller *board;

    SkaterBoy(IGame *game, int entity) : Human(game, entity, 125) {
        animDeath  = 13;
        jointChest = 1;

        board = game->addEntity(TR::Entity::ENEMY_SKATEBOARD, getRoomIndex(), pos, 0.0f);
        board->activate();
    }

    virtual void onDead() {
        game->addEntity(TR::Entity::UZIS, getRoomIndex(), pos, 0);
    }

    virtual void hit(float damage, Controller *enemy = NULL, TR::HitType hitType = TR::HIT_DEFAULT) {
        bool flag = health >= 120;
        Human::hit(damage, enemy, hitType);
        if (flag && health < 120) {
            game->playTrack(56, true);
        }
    };
    
    virtual int getStateGround() {
        if (!think(false))
            return state;

        fullChestRotation = state == STATE_STAND_FIRE || state == STATE_MOVE_FIRE;

        switch (state) {
            case STATE_STOP :
                flags.unused = 0;
                if (targetIsVisible(HUMAN_DIST_SHOT))
                    return STATE_STAND_FIRE;
                return STATE_MOVE;
            case STATE_MOVE :
                flags.unused = 0;
                if (rand() < 512)
                    return STATE_STEP;
                if (targetIsVisible(HUMAN_DIST_SHOT))
                    return (mood != MOOD_ESCAPE && targetDist > SKATERBOY_DIST_MIN && targetDist < SKATERBOY_DIST_MAX) ? STATE_STOP : STATE_MOVE_FIRE;
                break;
            case STATE_STEP :
                if (rand() < 1024)
                    return STATE_MOVE;
                break;
            case STATE_STAND_FIRE :
            case STATE_MOVE_FIRE  :
                if (!flags.unused && targetIsVisible(HUMAN_DIST_SHOT)) {
                    float damage = state == STATE_STAND_FIRE ? SKATERBOY_DAMAGE_STAND : SKATERBOY_DAMAGE_MOVE;
                    jointGun = 7; doShot(damage, vec3(0, -32, 0));
                    jointGun = 4; doShot(damage, vec3(0, -32, 0));
                    flags.unused = 1;
                }

                if (mood == MOOD_ESCAPE || targetDist < 1024)
                    return STATE_RUN;
                break;
        }

        return state;
    }

    virtual void update() {
        Human::update();
        board->pos   = pos;
        board->angle = angle;
        if (board->animation.index != animation.index)
            board->animation.setAnim(animation.index);
        board->animation.time = animation.time - Core::deltaTime;
    }

    virtual void deactivate(bool removeFromList) {
        board->deactivate(removeFromList);
        Human::deactivate(removeFromList);
    }
};


#define COWBOY_DIST_WALK (3 * 1024)
#define COWBOY_DAMAGE    70

struct Cowboy : Human {

    Cowboy(IGame *game, int entity) : Human(game, entity, 150) {
        animDeath  = 7;
        jointChest = 1;
        jointHead  = 2;
    }

    virtual void onDead() {
        game->addEntity(TR::Entity::MAGNUMS, getRoomIndex(), pos, 0);
    }

    virtual int getStateGround() {
        if (!think(false))
            return state;

        fullChestRotation = state == STATE_WAIT || state == STATE_AIM;

        if (nextState == state)
            nextState = STATE_NONE;

        switch (state) {
            case STATE_STOP :
                if (nextState != STATE_NONE)
                    return nextState;
                if (targetIsVisible(HUMAN_DIST_SHOT))
                    return STATE_AIM;
                if (mood == MOOD_SLEEP)
                    return STATE_WALK;
                return STATE_RUN;
            case STATE_WALK : 
                if (mood == MOOD_ESCAPE || !targetInView)
                    nextState = STATE_RUN;
                else if (targetIsVisible(HUMAN_DIST_SHOT))
                    nextState = STATE_AIM;
                else if (targetDist > COWBOY_DIST_WALK)
                    nextState = STATE_RUN;
                else
                    break;
                return STATE_STOP;
            case STATE_RUN :
                if (mood == MOOD_ESCAPE || !targetInView)
                    break;
                if (targetIsVisible(HUMAN_DIST_SHOT))
                    nextState = STATE_AIM;
                else if (targetDist < COWBOY_DIST_WALK && targetInView)
                    nextState = STATE_WALK;
                else
                    break;
                return STATE_STOP;
            case STATE_AIM :
                flags.unused = 7;
                if (nextState != STATE_NONE)
                    return STATE_STOP;
                if (targetIsVisible(HUMAN_DIST_SHOT))
                    return STATE_WAIT; // STATE_FIRE
                return STATE_STOP;
            case STATE_WAIT : // STATE_FIRE
                if (animation.frameIndex != flags.unused && (animation.frameIndex == 0 || animation.frameIndex == 4)) {
                    jointGun = (flags.unused == 7) ? 8 : 5;
                    doShot(COWBOY_DAMAGE, vec3(0, -40, 40));
                    flags.unused = animation.frameIndex;
                }

                if (mood == MOOD_ESCAPE)
                    nextState = STATE_RUN;
                break;
        }

        return state;
    }
};


#define MRT_DIST_WALK (4 * 1024)
#define MRT_DAMAGE    150

struct MrT : Human {

    MrT(IGame *game, int entity) : Human(game, entity, 200) {
        animDeath  = 14;
        jointGun   = 9;
        jointChest = 1;
        jointHead  = 2;
        state = STATE_RUN;
    }

    virtual void onDead() {
        game->addEntity(TR::Entity::SHOTGUN, getRoomIndex(), pos, 0);
    }

    virtual int getStateGround() {
        if (!think(false))
            return state;

        fullChestRotation = state == STATE_WAIT || state == STATE_AIM;

        if (nextState == state)
            nextState = STATE_NONE;

        switch (state) {
            case STATE_STOP :
                if (nextState != STATE_NONE)
                    return nextState;
                if (targetIsVisible(HUMAN_DIST_SHOT))
                    return STATE_AIM;
                if (mood == MOOD_SLEEP)
                    return STATE_WALK;
                return STATE_RUN;
            case STATE_WALK : 
                if (mood == MOOD_ESCAPE || !targetInView)
                    nextState = STATE_RUN;
                else if (targetIsVisible(HUMAN_DIST_SHOT))
                    nextState = STATE_AIM;
                else if (targetDist > MRT_DIST_WALK)
                    nextState = STATE_RUN;
                else
                    break;
                return STATE_STOP;
            case STATE_RUN :
                if (mood == MOOD_ESCAPE || !targetInView)
                    break;
                if (targetIsVisible(HUMAN_DIST_SHOT))
                    nextState = STATE_AIM;
                else if (targetDist < MRT_DIST_WALK && targetInView)
                    nextState = STATE_WALK;
                else
                    break;
                return STATE_STOP;
            case STATE_AIM :
                flags.unused = false;
                if (nextState != STATE_NONE)
                    return STATE_STOP;
                if (targetIsVisible(HUMAN_DIST_SHOT))
                    return STATE_WAIT; // STATE_FIRE
                return STATE_STOP;
            case STATE_WAIT : // STATE_FIRE
                if (!flags.unused) {
                    doShot(MRT_DAMAGE, vec3(-20, -20, 300));
                    flags.unused = true;
                }
                if (mood == MOOD_ESCAPE)
                    nextState = STATE_RUN;
                break;
        }

        return state;
    }
};

#define NATLA_FIRE_ANGLE    (30 * DEG2RAD)
#define NATLA_FAINT_TIME    16.0f
#define NATLA_HALF_HEALTH   200.0f
#define NATLA_FAINT_HEALTH  -8192.0f
#define NATLA_LIFT_SPEED    MUTANT_LIFT_SPEED

struct Natla : Human {

    enum {
        STATE_NONE,
        STATE_STOP,
        STATE_FLY,
        STATE_RUN,
        STATE_AIM,
        STATE_FAINT,
        STATE_FIRE,
        STATE_FALL,
        STATE_STAND,
        STATE_DEATH,
    };

    enum {
        FLAG_FLY = 1,
    };

    Natla(IGame *game, int entity) : Human(game, entity, 400) {
        jointChest = 1;
        jointHead  = 2;
    }

    int stage1() {
        flying = (flags.unused & FLAG_FLY);

        stepHeight =  256;
        dropHeight = -256;

        if (flying) {
            stepHeight *= 80;
            dropHeight *= 80;
        }

        if (!think(false))
            return state;

        timer += Core::deltaTime;
        bool canShot = target && target->health > 0.0f && targetIsVisible(HUMAN_DIST_SHOT);

        if (canShot && state == STATE_FLY && flying && rand() < 256)
            flags.unused &= ~FLAG_FLY;
        else if (!canShot)
            flags.unused |= FLAG_FLY;

        flying = (flags.unused & FLAG_FLY);

        if (state == nextState)
            nextState = STATE_NONE;

        switch (state) {
            case STATE_STOP :
                timer = 0.0f;
                return flying ? STATE_FLY : STATE_AIM;
            case STATE_FLY : {
                if (timer >= 1.0f) {
                    { //if (canShot) { ???
                        game->playSound(TR::SND_NATLA_SHOT, pos, Sound::PAN);
                        shot(TR::Entity::CENTAUR_BULLET, 4, vec3(5.0f, 220.0f, 7.0f));
                    }
                    timer -= 1.0f;
                }

                int16 roomIndex = getRoomIndex();
                TR::Room::Sector *sector = level->getSector(roomIndex, pos);
                float floor = level->getFloor(sector, pos) - 128.0f;

                if (!flying && pos.y >= floor)
                    return STATE_STOP;

                break;
            }
            case STATE_AIM :
                if (nextState != STATE_NONE)
                    return nextState;
                return canShot ? STATE_FIRE : STATE_STOP;
            case STATE_FIRE :
                if (nextState != STATE_NONE)
                    return state;

                game->playSound(TR::SND_NATLA_SHOT, pos, Sound::PAN);
                shot(TR::Entity::CENTAUR_BULLET, 4, vec3(5.0f, 220.0f, 7.0f));
                shot(TR::Entity::CENTAUR_BULLET, 4, vec3(5.0f, 220.0f, 7.0f), 0.0f, (randf() * 2.0f - 1.0f) * (25.0f * DEG2RAD));
                shot(TR::Entity::CENTAUR_BULLET, 4, vec3(5.0f, 220.0f, 7.0f), 0.0f, (randf() * 2.0f - 1.0f) * (25.0f * DEG2RAD));

                nextState = STATE_STOP;
                break;
        }

        return state;
    }

    int stage2() {
        stepHeight =  256;
        dropHeight = -256;
        flying     = false;

        if (!think(true))
            return state;

        timer += Core::deltaTime;
        bool canShot = target && target->health > 0.0f && fabsf(targetAngle) < NATLA_FIRE_ANGLE && targetIsVisible(HUMAN_DIST_SHOT);

        switch (state) {
            case STATE_RUN   :
            case STATE_STAND :
                if (timer >= 20.0f / 30.0f) {
                    { // if (canShot) { ???
                        game->playSound(TR::SND_NATLA_SHOT, pos, Sound::PAN);
                        shot(TR::Entity::MUTANT_BULLET, 4, vec3(5.0f, 220.0f, 7.0f));
                    }
                    timer -= 20.0f / 30.0f;
                }
                return canShot ? STATE_STAND : STATE_RUN;
            case STATE_FLY  :
                health = NATLA_FAINT_HEALTH;
                return STATE_FALL;
            case STATE_STOP :
            case STATE_AIM  :
            case STATE_FIRE :
                health = NATLA_FAINT_HEALTH;
                return STATE_FAINT;
        }
        return state;
    }

    virtual int getStateGround() {
        if (health > NATLA_HALF_HEALTH)
            return stage1();
        return stage2();
    }

    virtual int getStateDeath() {
        switch (state) {
            case STATE_FALL : {
                int16 roomIndex = getRoomIndex();
                TR::Room::Sector *sector = level->getSector(roomIndex, pos);
                float floor = level->getFloor(sector, pos);
                if (pos.y >= floor) {
                    pos.y = floor;
                    timer = 0.0f;
                    return STATE_FAINT;
                }
                return state;
            }
            case STATE_FAINT : {
                timer += Core::deltaTime;
                if (timer >= NATLA_FAINT_TIME) {
                    health = NATLA_HALF_HEALTH;
                    timer  = 0.0f;
                    flags.unused = 0;
                    game->playTrack(54, true);
                    return STATE_STAND;
                }
                return state;
            }
        }
        return STATE_DEATH;
    }

    virtual void updateVelocity() {
        if (state == STATE_FLY) {
            angle.y += targetAngle;
            updateJoints();
            angle.y -= targetAngle;
        }
        Enemy::updateVelocity();
    }

    virtual void updatePosition() {
        if (flying) {
            stand = STAND_AIR;
            lift(target->pos.y - pos.y, NATLA_LIFT_SPEED);
        }

        turn(true, state == STATE_RUN ? HUMAN_TURN_FAST : HUMAN_TURN_SLOW);
        
        Enemy::updatePosition();
        setOverrides(state != STATE_FAINT, jointChest, jointHead);
        lookAt(target);

        stand = STAND_GROUND;
    }
};

struct Dog : Enemy {

    enum {
        ANIM_SLEEP = 5,
        ANIM_DEATH = 13,
    };

    enum {
        STATE_DEATH = 10,
    };

    Dog(IGame *game, int entity) : Enemy(game, entity, 6, 10, 0.0f, 0.0f) {
        jointChest = 19;
        jointHead  = 20;
        animation.setAnim(ANIM_SLEEP);
    }

    virtual int getStateDeath() {
        if (state != STATE_DEATH)
            return animation.setAnim(ANIM_DEATH);
        return state;
    }
};

#define TIGER_WALK           1120
#define TIGER_ROAR           96
#define TIGER_DIST_ATTACK_1  341
#define TIGER_DIST_ATTACK_2  1536
#define TIGER_DIST_ATTACK_3  1024
#define TIGER_DAMAGE         100
#define TIGER_TURN_FAST      (DEG2RAD * 180)
#define TIGER_TURN_SLOW      (DEG2RAD * 90)

struct Tiger : Enemy {

    enum {
        HIT_MASK = 0x7FDC000,
    };

    enum {
        ANIM_DEATH = 11,
    };

    enum {
        STATE_NONE = -1,
        STATE_DEATH    ,
        STATE_STOP     ,
        STATE_WALK     ,
        STATE_RUN      ,
        STATE_IDLE     ,
        STATE_ROAR     ,
        STATE_ATTACK_1 ,
        STATE_ATTACK_2 ,
        STATE_ATTACK_3 ,
    };

    Tiger(IGame *game, int entity) : Enemy(game, entity, 20, 341, 200.0f, 0.25f) {
        dropHeight = -1024;
        jointChest = -1;//21;
        jointHead  = -1;//22;
        nextState  = STATE_NONE;
    }

    virtual int getStateGround() {
        if (!think(true))
            return state;

        if (nextState == state)
            nextState = STATE_NONE;

        switch (state) {
            case STATE_STOP    :
                if (mood == MOOD_ESCAPE)
                    return STATE_RUN;
                if (mood == MOOD_SLEEP) {
                    int r = rand();
                    if (r < TIGER_ROAR) return STATE_ROAR;
                    if (r < TIGER_WALK) return STATE_WALK;
                    return state;
                }
                if (targetInView && targetDist < TIGER_DIST_ATTACK_1)
                    return STATE_ATTACK_1;
                if (targetInView && targetDist < TIGER_DIST_ATTACK_3)
                    return STATE_ATTACK_3;
                if (nextState != STATE_NONE)
                    return nextState;
                if (mood != MOOD_ATTACK && rand() < TIGER_ROAR)
                    return STATE_ROAR;
                return STATE_RUN;
            case STATE_WALK     : 
                if (mood == MOOD_ATTACK || mood == MOOD_ESCAPE)
                    return STATE_RUN;
                if (rand() < TIGER_ROAR) {
                    nextState = STATE_ROAR;
                    return STATE_STOP;
                }
                break;
            case STATE_RUN      : {
                bool melee = flags.unused != 0;
                flags.unused = 0;

                if (mood == MOOD_SLEEP)
                    return STATE_STOP;
                if (targetInView && melee)
                    return STATE_STOP;
                if (targetInView && targetDist < TIGER_DIST_ATTACK_2) {
                    if (target->velocity.length2() < SQR(16.0f))
                        return STATE_STOP;
                    else
                        return STATE_ATTACK_2;
                }
                if (mood != MOOD_ATTACK && rand() < TIGER_ROAR) {
                    nextState = STATE_ROAR;
                    return STATE_STOP;
                }
                break;
            }
            case STATE_ATTACK_1 :
            case STATE_ATTACK_2 :
            case STATE_ATTACK_3 :
                if (flags.unused == 0 && (collide(target) & HIT_MASK)) {
                    bite(26, vec3(19.0f, -13.0f, 3.0f), TIGER_DAMAGE);
                    flags.unused = 1;
                }
        }
        return state;
    }

    virtual int getStateDeath() {
        if (state == STATE_DEATH)
            return state;
        return animation.setAnim(ANIM_DEATH);
    }

    virtual void updatePosition() {
        turn(state == STATE_RUN || state == STATE_WALK || state == STATE_ROAR, state == STATE_RUN ? TIGER_TURN_FAST : TIGER_TURN_SLOW);

        if (state == STATE_DEATH) {
            animation.overrideMask = 0;
            return;
        }

        Enemy::updatePosition();
        setOverrides(true, jointChest, jointHead);
        lookAt(target);
    }
};


#define WINSTON_DIST        1536.0f
#define WINSTON_TURN_SLOW   (DEG2RAD * 60)
#define WINSTON_FREEZE_TIME 60.0f

struct Winston : Enemy {

    enum {
        STATE_NONE ,
        STATE_STOP ,
        STATE_WALK ,
    };

    Texture *environment;

    Winston(IGame *game, int entity) : Enemy(game, entity, 20, 341, 200.0f, 0.25f), environment(NULL) {
        dropHeight = -1024;
        jointChest = 11;
        jointHead  = 25;
        nextState  = STATE_NONE;
        lookAtSpeed = 1.0f;
        timer = 0.0f;
    }

    virtual ~Winston() {
        delete environment;
    }

    virtual int getStateGround() {
        if (getRoomIndex() == 94) {
            int doorIndex = (level->version & TR::VER_TR2) ? 38 : 68;

            Controller *door = (Controller*)game->getLevel()->entities[doorIndex].controller;
            if (!door->isActive()) {
                if (timer > WINSTON_FREEZE_TIME) {
                    flags.unused |= 4;
                    if (!environment) {
                        bakeEnvironment(environment);
                    }
                } else {
                    timer += Core::deltaTime;
                }
                return STATE_STOP;
            } else {
                if (!(flags.unused & 4)) {
                    timer = 0.0f;
                }
            }
        }

        if (flags.unused & 4) {
            timer -= Core::deltaTime;
            if (timer < 0.0f) {
                timer = 0.0f;
                flags.unused &= ~4;
            }
            return STATE_STOP;
        }

        if (!think(false))
            return state;

        if (nextState == state)
            nextState = STATE_NONE;

        if (nextState != STATE_NONE)
            return nextState;

        switch (state) {
            case STATE_STOP    :
                if ((targetDist > WINSTON_DIST || !targetInView) && nextState != STATE_WALK) {
                    nextState = STATE_WALK;
                    game->playSound(TR::SND_WINSTON_WALK, pos, Sound::PAN);
                }
            case STATE_WALK     :
                if (targetDist < WINSTON_DIST) {
                    if (targetInView) {
                        nextState = STATE_STOP;
                        flags.unused &= ~1;
                    } else if (!(flags.unused & 1)) {
                        game->playSound(TR::SND_WINSTON_SCARED, pos, Sound::PAN);
                        game->playSound(TR::SND_WINSTON_TRAY, pos, Sound::PAN);
                        flags.unused |= 1;
                    }
                }
        }

        bool touch = collide(target) != 0;
        bool push  = (flags.unused & 2) != 0;

        if (!push && touch) {
            game->playSound(TR::SND_WINSTON_PUSH, pos, Sound::PAN);
            game->playSound(TR::SND_WINSTON_TRAY, pos, Sound::PAN);
            flags.unused |= 2;
        }

        if (push && !touch) {
            flags.unused &= ~2;
        }

        if (rand() < 0x100) {
            game->playSound(TR::SND_WINSTON_TRAY, pos, Sound::PAN);
        }

        return state;
    }

    virtual void updatePosition() {
        if (flags.unused & 4) {
            animation.time = 0.0f;
            animation.updateInfo();
            return;
        }

        turn(state == STATE_WALK, WINSTON_TURN_SLOW);
        angle.z = 0.0f;

        Enemy::updatePosition();
        setOverrides(true, jointChest, jointHead);
        lookAt(target);
    }

    virtual void render(Frustum *frustum, MeshBuilder *mesh, Shader::Type type, bool caustics) {
        if (environment && (flags.unused & 4)) {
            game->setRoomParams(getRoomIndex(), Shader::MIRROR, 1.5f, 2.0f, 2.5f, 1.0f, false);
            environment->bind(sEnvironment);
            Controller::render(frustum, mesh, type, caustics);
        } else {
            Enemy::render(frustum, mesh, type, caustics);
        }
    }
};

#endif