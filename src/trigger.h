#ifndef H_TRIGGER
#define H_TRIGGER

#include "core.h"
#include "controller.h"
#include "character.h"
#include "sprite.h"

struct Switch : Controller {
    enum {
        STATE_DOWN,
        STATE_UP,
    };

    Switch(IGame *game, int entity) : Controller(game, entity) {}
    
    bool setTimer(float t) {
        if (activeState == asInactive) {
            if (state == STATE_DOWN && t > 0.0f) {
                timer = t;
                activeState = asActive;
            } else
                deactivate(true);
           return true;
        }
        return false;
    }
    
    virtual bool activate() {
        if (Controller::activate()) {
            animation.setState(state == STATE_UP ? STATE_DOWN : STATE_UP);
            return true;
        }
        return false;
    }

    virtual void update() {
        updateAnimation(true);
        getEntity().flags.active = TR::ACTIVE;
        if (!isActive())
            animation.setState(STATE_UP);
    }
};

struct Gear : Controller {
    enum {
        STATE_STATIC,
        STATE_ROTATE,
    };

    Gear(IGame *game, int entity) : Controller(game, entity) {}

    virtual void update() {
        updateAnimation(true);
        animation.setState(isActive() ? STATE_ROTATE : STATE_STATIC);
    }
};

#define DART_DAMAGE 50

struct Dart : Controller {
    vec3 velocity;
    vec3 dir;
    bool armed;

    Dart(IGame *game, int entity) : Controller(game, entity), armed(true) {
        dir = vec3(sinf(angle.y), 0, cosf(angle.y));
        activate();
    }

    virtual void update() {
        velocity = dir * animation.getSpeed();
        pos = pos + velocity * (Core::deltaTime * 30.0f);
        updateEntity();

        Controller *lara = (Controller*)level->laraController;
        if (armed && collide(lara)) {
            Sprite::add(game, TR::Entity::BLOOD, getRoomIndex(), (int)pos.x, (int)pos.y, (int)pos.z, Sprite::FRAME_ANIMATED);
            lara->hit(DART_DAMAGE, this);
            armed = false;
        }

        TR::Level::FloorInfo info;
        level->getFloorInfo(getRoomIndex(), (int)pos.x, (int)pos.y, (int)pos.z, info);
        if (pos.y > info.floor || pos.y < info.ceiling || !insideRoom(pos, getRoomIndex())) {
            TR::Entity &e = getEntity();
                
            vec3 p = pos - dir * 64.0f; // wall offset = 64
            Sprite::add(game, TR::Entity::RICOCHET, e.room, (int)p.x, (int)p.y, (int)p.z, Sprite::FRAME_RANDOM);

            level->entityRemove(entity);
            delete this;
        }
    }
};

struct TrapDartEmitter : Controller {
    enum {
        STATE_IDLE,
        STATE_FIRE
    };

    TrapDartEmitter(IGame *game, int entity) : Controller(game, entity) {}
    
    void virtual update() {
        if (state == STATE_IDLE || state == STATE_FIRE)
            animation.setState(isActive() ? STATE_FIRE : STATE_IDLE);
        else
            ASSERT(false);

        if (state == STATE_FIRE && animation.framePrev == -1) {
            TR::Entity &entity = getEntity();

            vec3 p = pos + vec3(0.0f, -512.0f, 256.0f).rotateY(PI - entity.rotation);

            game->addEntity(TR::Entity::DART, getRoomIndex(), p, entity.rotation);
            Sprite::add(game, TR::Entity::SMOKE, entity.room, (int)p.x, (int)p.y, (int)p.z);
            game->playSound(TR::SND_DART, p, Sound::Flags::PAN);
        }

        updateAnimation(true);
    }
};

#define FLAME_HEAT_DAMAGE 90
#define FLAME_BURN_DAMAGE 150

struct Flame : Sprite {

    static Flame* add(IGame *game, Controller *controller, int jointIndex) {
        Flame *flame = NULL;

        TR::Level *level = game->getLevel();
        int  roomIndex   = controller->getRoomIndex();
        vec3 pos         = controller->pos;

        int index = level->entityAdd(TR::Entity::FLAME, roomIndex, int(pos.x), int(pos.y), int(pos.z), 0, 0);
        if (index > -1) {
            flame = new Flame(game, index, jointIndex);
            level->entities[index].controller = flame; 
        }
        return flame;
    }

    int jointIndex;
    float sleep;

    Flame(IGame *game, int entity, int jointIndex) : Sprite(game, entity, false, Sprite::FRAME_ANIMATED), jointIndex(jointIndex), sleep(0.0f) {
        time = randf() * 3.0f;
        activate();
    }

    virtual void update() {
        Sprite::update();
        game->playSound(TR::SND_FLAME, pos, Sound::PAN);

        Character *lara = (Character*)level->laraController;

        if (jointIndex > -1) {
            if (lara->stand == Character::STAND_UNDERWATER) {
                level->entityRemove(entity);
                delete this;
                return;
            }

            pos = lara->animation.getJoints(lara->getMatrix(), jointIndex).pos;
            if (jointIndex == 0)
                pos.y += 100.0f;

            lara->hit(FLAME_BURN_DAMAGE * Core::deltaTime, this);
        } else 
            if (lara->health > 0.0f) {
                if (sleep > 0.0f)
                    sleep = max(0.0f, sleep - Core::deltaTime);

                if (sleep == 0.0f && lara->collide(Sphere(pos, 600.0f))) {
                    lara->hit(FLAME_HEAT_DAMAGE * Core::deltaTime, this);

                    if (lara->collide(Sphere(pos, 300.0f))) {
                        Flame::add(game, lara, 0);
                        sleep = 3.0f; // stay inactive for 3 seconds
                    }
                }
            }
    }
};

struct TrapFlameEmitter : Controller {
    Flame *flame;

    TrapFlameEmitter(IGame *game, int entity) : Controller(game, entity), flame(NULL) {}

    void virtual update() {
        if (!isActive()) {
            if (flame) {
                level->entityRemove(flame->entity);
                delete flame;
                flame = NULL;
                Sound::stop(TR::SND_FLAME);
            }
            return;
        }

        if (flame) return;

        flame = Flame::add(game, this, -1);
    }
};


#define LAVA_PARTICLE_DAMAGE 10
#define LAVA_V_SPEED         -165
#define LAVA_H_SPEED         32
#define LAVA_EMITTER_RANGE   (1024 * 10)

struct LavaParticle : Sprite {
    int  bounces;

    LavaParticle(IGame *game, int entity) : Sprite(game, entity, false, Sprite::FRAME_RANDOM), bounces(0) {
        float speed = randf() * LAVA_H_SPEED;
        velocity = vec3(cosf(angle.y) * speed, randf() * LAVA_V_SPEED, sinf(angle.y) * speed);
        blendMode = bmAdd;
        activate();
    }

    virtual void update() {
        applyGravity(velocity.y);
        Sprite::update();

        TR::Level::FloorInfo info;
        level->getFloorInfo(getRoomIndex(), int(pos.x), int(pos.y), int(pos.z), info);

        bool hit = false;
        if (!bounces) {
            Controller *lara = (Controller*)game->getLara();
            if ((hit = lara->collide(Sphere(pos, 0.0f))))
                lara->hit(LAVA_PARTICLE_DAMAGE, this);
        }

        if (hit || pos.y > info.floor || pos.y < info.ceiling) {
            if (hit || ++bounces > 4) {
                level->entityRemove(entity);
                delete this;
                return;
            }
            getEntity().intensity = bounces * 2048 - 1;

            if (pos.y > info.floor)   pos.y = float(info.floor);
            if (pos.y < info.ceiling) pos.y = float(info.ceiling);

            velocity = velocity.reflect(vec3(0, 1, 0)) * 0.5f;
        }
    }
};

struct TrapLavaEmitter : Controller {
    float timer;

    TrapLavaEmitter(IGame *game, int entity) : Controller(game, entity), timer(0.0f) {}

    void virtual update() {
        vec3 d = (game->getLara()->pos - pos).abs();

        if (!isActive() || max(d.x, d.y, d.z) > LAVA_EMITTER_RANGE) return;

        if (timer <= 0.0f) {
            game->addEntity(TR::Entity::LAVA_PARTICLE, getRoomIndex(), pos, PI * 2.0f * randf());
            game->playSound(TR::SND_LAVA, pos, Sound::PAN);
            timer += 1.0f / 30.0f;
        } else
            timer -= Core::deltaTime;
    }
};



#define BOULDER_DAMAGE_GROUND 1000
#define BOULDER_DAMAGE_AIR    100

struct TrapBoulder : Controller {
    enum {
        STATE_FALL,
        STATE_ROLL,
    };

    vec3 velocity;

    TrapBoulder(IGame *game, int entity) : Controller(game, entity), velocity(0) {}

    virtual void update() {
        if (activeState != asActive) return;

        TR::Level::FloorInfo info;
        level->getFloorInfo(getRoomIndex(), int(pos.x), int(pos.y), int(pos.z), info);

        vec3 dir = getDir();

        if (pos.y >= info.floor - 256) {
            pos.y = float(info.floor);
            velocity = dir * animation.getSpeed();
            if (state != STATE_ROLL)
                animation.setState(STATE_ROLL);
        } else {
            if (velocity.y == 0.0f)
                velocity.y = 10.0f;
            velocity.y += GRAVITY * Core::deltaTime;
            animation.setState(STATE_FALL);
        }

        vec3 p = pos;
        pos += velocity * (30.0f * Core::deltaTime);

        if (info.roomNext != TR::NO_ROOM)
            getEntity().room = info.roomNext;

        game->checkTrigger(this, true);

        vec3 v = pos + getDir() * 512.0f;
        level->getFloorInfo(getRoomIndex(), int(v.x), int(v.y), int(v.z), info);
        if (pos.y > info.floor) {
            pos = p;
            deactivate();
            return;
        }

        Character *lara = (Character*)level->laraController;
        if (lara->health > 0.0f && collide(lara)) {
            if (lara->stand == Character::STAND_GROUND)
                lara->hit(BOULDER_DAMAGE_GROUND, this, TR::HIT_BOULDER);
            if (lara->stand == Character::STAND_AIR)
                lara->hit(BOULDER_DAMAGE_AIR * 30.0f * Core::deltaTime, this);
        }

        updateAnimation(true);
        updateEntity();
    }
};

// not a trigger
struct Block : Controller {
    enum {
        STATE_STAND = 1,
        STATE_PUSH,
        STATE_PULL,
    };

    float velocity;

    Block(IGame *game, int entity) : Controller(game, entity), velocity(0.0f) {
        updateFloor(true);
    }

    void updateFloor(bool rise) {
        TR::Entity &e = getEntity();
        TR::Level::FloorInfo info;
        level->getFloorInfo(e.room, e.x, e.y, e.z, info);
        if (info.roomNext != 0xFF)
            e.room = info.roomNext;
        int dx, dz;
        TR::Room::Sector &s = level->getSector(e.room, e.x, e.z, dx, dz);
        s.floor += rise ? -4 : 4;
    }

    bool doMove(bool push) {
    // check floor height of next floor
        vec3 dir = getDir() * (push ? 1024.0f : -1024.0f);
        TR::Entity &e = getEntity();
        TR::Level::FloorInfo info;

        int px = e.x + (int)dir.x;
        int pz = e.z + (int)dir.z;
        level->getFloorInfo(e.room, px, e.y, pz, info);

        if ((info.slantX | info.slantZ) || info.floor != e.y || info.floor - info.ceiling < 1024)
            return false;

        // check for trapdoor
        px /= 1024;
        pz /= 1024;
        for (int i = 0; i < info.trigCmdCount; i++)
            if (info.trigCmd[i].action == TR::Action::ACTIVATE) {
                TR::Entity &obj = level->entities[info.trigCmd[i].args];
                if ((obj.type == TR::Entity::TRAP_DOOR_1 || obj.type == TR::Entity::TRAP_DOOR_2) && px == obj.x / 1024 && pz == obj.z / 1024)
                    return false;
            }

        // check Laras destination position
        if (!push) {
            dir = getDir() * (-2048.0f);
            px = e.x + (int)dir.x;
            pz = e.z + (int)dir.z;
            level->getFloorInfo(e.room, px, e.y, pz, info);
            if ((info.slantX | info.slantZ) || info.floor != e.y || info.floor - info.ceiling < 1024)
                return false;
        }

        if (!animation.setState(push ? STATE_PUSH : STATE_PULL))
            return false;
        updateFloor(false);
        activate();
        return true;
    }

    virtual void update() {
        TR::Entity &e = getEntity();
        TR::Level::FloorInfo info;        
        level->getFloorInfo(e.room, e.x, e.y, e.z, info);

        if (pos.y < info.floor) {
            if (info.roomBelow != TR::NO_ROOM)
                e.room = info.roomBelow;

            velocity += Core::deltaTime * GRAVITY;
            pos.y    += Core::deltaTime * velocity * 30.0f;
            if (pos.y >= info.floor) {
                velocity = 0.0f;
                pos.y    = float(info.floor);
                game->setEffect(this, TR::Effect::FLOOR_SHAKE);
                game->playSound(TR::SND_BOULDER, pos, Sound::PAN);
                deactivate(true);
                updateFloor(true);
            }
            updateEntity();
        } else {
            if (state == STATE_STAND) return;
            updateAnimation(true);
            if (state == STATE_STAND) {
                updateEntity();
                updateFloor(true);
                deactivate();
                game->checkTrigger(this, true);
            }
        }
        updateLights();
    }
};


struct MovingBlock : Controller {
    enum {
        STATE_BEGIN,
        STATE_END,
        STATE_MOVE,
    };

    MovingBlock(IGame *game, int entity) : Controller(game, entity) {
        if (!getEntity().flags.invisible)
            updateFloor(true);
    }

    void updateFloor(bool rise) {
        updateEntity();
        TR::Entity &e = getEntity();
        TR::Level::FloorInfo info;
        level->getFloorInfo(e.room, e.x, e.y, e.z, info);
        if (info.roomNext != 0xFF)
            e.room = info.roomNext;
        int dx, dz;
        TR::Room::Sector &s = level->getSector(e.room, e.x, e.z, dx, dz);
        s.floor += rise ? -8 : 8;
    }

    virtual void update() {
        updateAnimation(true);

        if (isActive()) {
            if (state == STATE_BEGIN) {
                updateFloor(false);
                animation.setState(STATE_END);
            }
        } else {
            if (state == STATE_END) {
                updateFloor(false);
                animation.setState(STATE_BEGIN);
            }
        }

        if (activeState == asInactive) {
            if (getEntity().flags.active == TR::ACTIVE)
                activeState = asActive; // stay in active items list
            pos.x = int(pos.x / 1024.0f) * 1024.0f + 512.0f;
            pos.z = int(pos.z / 1024.0f) * 1024.0f + 512.0f;
            updateFloor(true);
            game->checkTrigger(this, true);
            return;
        }

        pos += getDir() * (animation.getSpeed() * Core::deltaTime * 30.0f);
    }
};


struct Door : Controller {
    enum {
        STATE_CLOSE,
        STATE_OPEN,
    };

    struct BlockInfo {
        int              roomIndex[2];
        int              sectorIndex[2];
        TR::Room::Sector sectors[2];

        BlockInfo() {}
        BlockInfo(TR::Level *level, int room, int nx, int nz, int x, int z, bool flip) {
        // front
            roomIndex[0] = room;
            roomIndex[1] = TR::NO_ROOM;

            if (roomIndex[0] == TR::NO_ROOM)
                return;
            if (flip && level->rooms[roomIndex[0]].alternateRoom != -1)
                roomIndex[0] = level->rooms[roomIndex[0]].alternateRoom;

            sectors[0] = level->getSector(roomIndex[0], x, z, sectorIndex[0]);

        // behind
            roomIndex[1] = level->getNextRoom(sectors[0].floorIndex);

            if (roomIndex[1] == TR::NO_ROOM)
                return;
            if (flip && level->rooms[roomIndex[1]].alternateRoom != -1)
                roomIndex[1] = level->rooms[roomIndex[1]].alternateRoom;

            sectors[1] = level->getSector(roomIndex[1], nx, nz, sectorIndex[1]);
        }

        void set(TR::Level *level) {
            for (int i = 0; i < 2; i++)
                if (roomIndex[i] != TR::NO_ROOM) {
                    TR::Room::Sector &s = level->rooms[roomIndex[i]].sectors[sectorIndex[i]];
                    s.floorIndex = 0;
                    s.boxIndex   = TR::NO_BOX;
                    s.roomBelow  = TR::NO_ROOM;
                    s.floor      = TR::NO_FLOOR;
                    s.roomAbove  = TR::NO_ROOM;
                    s.ceiling    = TR::NO_FLOOR;

                    if (sectors[i].boxIndex != TR::NO_BOX) {
                        ASSERT(sectors[i].boxIndex < level->boxesCount);
                        TR::Box &box = level->boxes[sectors[i].boxIndex];
                        if (box.overlap.blockable)
                            box.overlap.block = true;
                    }
                }
        }

        void reset(TR::Level *level) {
            for (int i = 0; i < 2; i++)
                if (roomIndex[i] != TR::NO_ROOM) {
                    level->rooms[roomIndex[i]].sectors[sectorIndex[i]] = sectors[i];
                    if (sectors[i].boxIndex != TR::NO_BOX) {
                        TR::Box &box = level->boxes[sectors[i].boxIndex];
                        if (box.overlap.blockable)
                            box.overlap.block = false;
                    }
                }
        }

    } block[2];

    Door(IGame *game, int entity) : Controller(game, entity) {
        TR::Entity &e = getEntity();
        vec3 p = pos - getDir() * 1024.0f;
        block[0] = BlockInfo(level, e.room, e.x, e.z, int(p.x), int(p.z), false);
        block[1] = BlockInfo(level, e.room, e.x, e.z, int(p.x), int(p.z), true);
        updateBlock(false);
    }

    void updateBlock(bool open) {
        if (open) {
            block[0].reset(level);
            block[1].reset(level);
        } else {
            block[0].set(level);
            block[1].set(level);
        }
    }
    
    virtual void update() {
        updateAnimation(true);
        int targetState = isActive() ? STATE_OPEN : STATE_CLOSE;

        if (state == targetState)
            updateBlock(targetState == STATE_OPEN);
        else
            animation.setState(targetState);
    }
};

struct TrapDoor : Controller {
    enum {
        STATE_CLOSE,
        STATE_OPEN,
    };

    TrapDoor(IGame *game, int entity) : Controller(game, entity) {
        getEntity().flags.collision = true;
    }
    
    virtual void update() {
        updateAnimation(true);
        int targetState = isActive() ? STATE_OPEN : STATE_CLOSE;

        if (state == targetState)
            getEntity().flags.collision = targetState == STATE_CLOSE;
        else
            animation.setState(targetState);
    }
};


struct TrapFloor : Controller {
    enum {
        STATE_STATIC,
        STATE_SHAKE,
        STATE_FALL,
        STATE_DOWN,
    };
    float speed;

    TrapFloor(IGame *game, int entity) : Controller(game, entity), speed(0) {
        getEntity().flags.collision = true;
    }

    virtual bool activate() {
        if (state != STATE_STATIC) return false;
        TR::Entity &e = ((Controller*)level->laraController)->getEntity();
        int ey = getEntity().y - 512; // real floor object position
        if (abs(e.y - ey) <= 8 && Controller::activate()) {
            animation.setState(STATE_SHAKE);
            return true;
        }
        return false;
    }

    virtual void update() {
        updateAnimation(true);
        if (state == STATE_FALL) {
            getEntity().flags.collision = false;
            speed += GRAVITY * 30 * Core::deltaTime;
            pos.y += speed * Core::deltaTime;

            TR::Level::FloorInfo info;
            level->getFloorInfo(getRoomIndex(), int(pos.x), int(pos.y), int(pos.z), info);

            if (pos.y > info.roomFloor && info.roomBelow != 0xFF)
                getEntity().room = info.roomBelow;

            if (pos.y > info.floor) {
                pos.y = (float)info.floor;
                animation.setState(STATE_DOWN);
            }
            updateEntity();
        }
    }
};

struct Bridge : Controller {
    Bridge(IGame *game, int entity) : Controller(game, entity) {
        getEntity().flags.collision = true;
    }
};

struct Drawbridge : Controller {
    enum {
        STATE_UP,
        STATE_DOWN,
    };

    Drawbridge(IGame *game, int entity) : Controller(game, entity) {
        getEntity().flags.collision = true;
    }

    virtual void update() {
        updateAnimation(true);
        animation.setState(isActive() ? STATE_DOWN : STATE_UP);
    }
};

struct Crystal : Controller {
    Texture *environment;

    Crystal(IGame *game, int entity) : Controller(game, entity) {
        environment = new Texture(64, 64, Texture::RGBA, true, NULL, true, true);
        activate();
    }

    virtual ~Crystal() {
        delete environment;
    }

    virtual void update() {
        updateAnimation(false);
    }

    virtual void render(Frustum *frustum, MeshBuilder *mesh, Shader::Type type, bool caustics) {
        Core::active.shader->setParam(uMaterial, vec4(0.5, 0.5, 2.0, 1.0f)); // blue color dodge for crystal
        environment->bind(sEnvironment);
        Controller::render(frustum, mesh, type, caustics);
    }
};

#define BLADE_DAMAGE    100
#define BLADE_RANGE     1024

struct TrapBlade : Controller {
    enum {
        STATE_STATIC = 0,
        STATE_SWING  = 2,
    };

    TrapBlade(IGame *game, int entity) : Controller(game, entity) {}

    virtual void update() {
        updateAnimation(true);

        if (isActive()) {
            if (state == STATE_STATIC)
                animation.setState(STATE_SWING);
        } else {
            if (state == STATE_SWING)
                animation.setState(STATE_STATIC);
        }

        if (state != STATE_SWING)
            return;

        int f = animation.frameIndex;
        if ((f <= 8 || f >= 20) && (f <= 42 || f >= 57))
            return;

        Character* lara = (Character*)level->laraController;
        if (!checkRange(lara, BLADE_RANGE) || !collide(lara))
            return;

        lara->hit(BLADE_DAMAGE * 30.0f * Core::deltaTime, this, TR::HIT_BLADE);
    }
};

#define SPIKES_DAMAGE_FALL      1000
#define SPIKES_DAMAGE_RUN       15
#define SPIKES_RANGE            1024

struct TrapSpikes : Controller {
    TrapSpikes(IGame *game, int entity) : Controller(game, entity) {
        activate();
    }

    virtual void update() {
        Character *lara = (Character*)level->laraController;
        if (lara->health <= 0.0f) return;

        if (!checkRange(lara, SPIKES_RANGE) || !collide(lara))
            return;

        if (lara->stand != Character::STAND_AIR || lara->velocity.y <= 0.0f || (pos.y - lara->pos.y) > 256.0f) {
            if (lara->speed < 30.0f)
                return;
            lara->hit(SPIKES_DAMAGE_RUN * 30.0f * Core::deltaTime, this, TR::HIT_SPIKES);
        } else
            lara->hit(SPIKES_DAMAGE_FALL, this, TR::HIT_SPIKES);
    }
};

struct TrapCeiling : Controller {
    enum {
        STATE_STATIC,
        STATE_FALL,
        STATE_DOWN,
    };

    float speed;

    TrapCeiling(IGame *game, int entity) : Controller(game, entity), speed(0) {}

    virtual void update() {
        updateAnimation(true);

        if (state == STATE_STATIC)
            animation.setState(STATE_FALL);
       
        if (state == STATE_FALL) {
            speed += GRAVITY * 30 * Core::deltaTime;
            pos.y += speed * Core::deltaTime;

            TR::Level::FloorInfo info;
            level->getFloorInfo(getRoomIndex(), int(pos.x), int(pos.y), int(pos.z), info);

            if (pos.y > info.roomFloor && info.roomBelow != 0xFF)
                getEntity().room = info.roomBelow;

            if (pos.y > info.floor) {
                pos.y = (float)info.floor;
                animation.setState(STATE_DOWN);
            }
            updateEntity();

            Controller *lara = (Controller*)level->laraController;
            if (collide(lara))
                lara->hit(1000);
        }
    }
};

#define SLAM_DAMAGE 400

struct TrapSlam : Controller {
    enum {
        STATE_OPEN,
        STATE_SLAM,
    };
    
    bool bite;

    TrapSlam(IGame *game, int entity) : Controller(game, entity), bite(false) {}

    virtual void update() {
        if (isActive()) {
            animation.setState(STATE_SLAM);

            if (animation.frameIndex >= 20)
                bite = false;

            Character *lara = (Character*)level->laraController;
            if (animation.state == STATE_SLAM && !bite && collide(lara)) {
                lara->hit(SLAM_DAMAGE, this, TR::HIT_SLAM);
                bite = true;
            }

        } else
            animation.setState(STATE_OPEN);

        updateAnimation(true);
    }
};

#define SWORD_DAMAGE 100.0f
#define SWORD_RANGE  1536.0f

struct TrapSword : Controller {
    vec3  dir;
    float rot;

    TrapSword(IGame *game, int entity) : Controller(game, entity), dir(0) {
        rot = (randf() * 2.0f - 1.0f) * PI;
    }

    virtual void update() {
        TR::Level::FloorInfo info;
        level->getFloorInfo(getRoomIndex(), int(pos.x), int(pos.y), int(pos.z), info);

        Controller *lara = game->getLara();

        if (dir.y == 0.0f) {
            dir = lara->pos - pos;
            if (dir.y > 0 && dir.y < 3072.0f && fabsf(dir.x) < SWORD_RANGE && fabsf(dir.z) < SWORD_RANGE) {
                dir /= 32.0f;
                dir.y = 50.0f;
            } else
                dir.y = 0.0f;
        } else {
            angle.y += rot * Core::deltaTime;
            applyGravity(dir.y);
            pos += dir * (30.0f * Core::deltaTime);
            if (pos.y > float(info.floor)) {
                pos.y = float(info.floor);
                game->playSound(TR::SND_SWORD, pos, Sound::PAN);
                deactivate(true);
            }
            updateEntity();

            if (collide(lara))
                lara->hit(SWORD_DAMAGE * 30.0f * Core::deltaTime, this, TR::HIT_SWORD); // TODO: push lara
        }
    }
};


struct ThorHammer : Controller {
    enum {
        STATE_IDLE,
        STATE_START,
        STATE_FALL,
        STATE_DOWN,
    };

    Controller *block;

    ThorHammer(IGame *game, int entity) : Controller(game, entity) {
        TR::Entity &e = getEntity();
        int index = level->entityAdd(TR::Entity::HAMMER_BLOCK, getRoomIndex(), e.x, e.y, e.z, e.rotation, -1);
        if (index > -1) {
            block = new Controller(game, index);
            level->entities[index].controller = block;
        }
        e.flags.collision = block->getEntity().flags.collision = false;
    }

    virtual void update() {
        Character *lara = (Character*)game->getLara();

        switch (state) {
            case STATE_IDLE  : if (isActive()) animation.setState(STATE_START); break;
            case STATE_START : animation.setState(isActive() ? STATE_FALL : STATE_IDLE); break;
            case STATE_FALL  : {
                if (animation.frameIndex > 30 && lara->health > 0.0f) {
                    vec3 d = pos + getDir() * (3.0f * 1024.0f) - lara->pos;
                    if (fabsf(d.x) < 520.0f && fabsf(d.z) < 520.0f)
                        lara->hit(1001, this, TR::HIT_BOULDER);
                }
                break;
            }
            case STATE_DOWN : {
                game->checkTrigger(this, 1); 
                if (lara->health > 0.0f)
                    getEntity().flags.collision = block->getEntity().flags.collision = true;
                deactivate(true);
                break;
            }
        }

        updateAnimation(true);
        block->animation.frameA = animation.frameA;
        block->animation.frameB = animation.frameB;
        block->animation.delta  = animation.delta;
    }
};


#define LIGHTNING_DAMAGE 400

struct Lightning : Controller {
    vec3  target;
    float timer;
    bool  flash;
    bool  armed;

    Lightning(IGame *game, int entity) : Controller(game, entity), timer(0), flash(false) {}

    virtual void update() {
        if (isActive()) {
            timer -= Core::deltaTime;

            Character *lara = (Character*)level->laraController;

            if (timer <= 0.0f) {
                if (flash) {
                    level->isFlipped = false;
                    flash = false;
                    armed = true;
                    timer = (35.0f + randf() * 45.0f) / 30.0f;                    
                } else {
                    level->isFlipped = true;
                    flash = true;
                    timer = 20.0f / 30.0f;

                    bool hasTargets = getModel()->mCount > 1; // LEVEL4 has, LEVEL10C not

                    if ((lara->pos - pos).length() < (hasTargets ? 2560.0f : 1024.0f)) {
                        lara->hit(LIGHTNING_DAMAGE, this, TR::HIT_LIGHTNING);
                        armed = false;
                    } else if (!hasTargets) {
                        target = pos + vec3(0.0f, 1024.0f, 0.0f);
                    } else
                        target = animation.getJoints(getMatrix(), 1 + int(randf() * 5)).pos;
                }
                game->playSound(TR::SND_LIGHTNING, pos, Sound::PAN);
            }
        } else {
            timer = 0.0f;
            flash = false;
            level->isFlipped = false;
            deactivate(true);
        }
    }

    void divide(vec3 *points, int L, int R, float spread) {
        int M = (L + R) / 2;
        if (M == L || M == R) return;
        points[M] = (points[L] + points[R]) * 0.5f + (vec3(randf(), randf(), randf()) - 0.5f) * spread;
        spread *= 0.5f;
        divide(points, L, M, spread);
        divide(points, M, R, spread);
    }

    short4 toCoord(const vec3 &v, int16 joint) {
        return short4(int16(v.x), int16(v.y), int16(v.z), joint);
    }

    void setVertex(Vertex &v, const vec3 &coord, int16 joint, int idx) {
        v.coord     = toCoord(coord, joint);
        v.normal    = { 0, -1, 0, 0 };
        v.texCoord  = { barTile[0].texCoord[idx].x, barTile[0].texCoord[idx].y, 32767, 32767 };
        v.param     = { 0, 0, 0, 0 };
        v.color     = { 255, 255, 255, 255 };
    }

    void renderPolyline(const vec3 &start, const vec3 &end, float width, float spread, int depth) {
        vec3 points[9];
        points[0] = start;
        points[8] = end;
        divide(points, 0, 8, spread);

        Index  indices[(COUNT(points) - 1) * 6];
        Vertex vertices[COUNT(points) * 2];
        
        int iCount = 0;
        int vCount = 0;
        int count = COUNT(points);
    // build indices
        for (int i = 0; i < count - 1; i++) {
            indices[iCount++] = vCount;
            indices[iCount++] = vCount + 1;
            indices[iCount++] = vCount + 2;
            indices[iCount++] = vCount + 1;
            indices[iCount++] = vCount + 3;
            indices[iCount++] = vCount + 2;
            vCount += 2;
        }
        vCount += 2;
        ASSERT(iCount == (count - 1) * 6);
        ASSERT(vCount == count * 2);

    // build vertices
        vec3 dir = Core::mViewInv.dir.xyz;

        vCount = 0;
        vec3 n;
        for (int i = 0; i < count; i++) {
            if (i < count - 1)
                n = dir.cross(points[i + 1] - points[i]).normal() * width;
            setVertex(vertices[vCount++], points[i] - n, 0, 0);
            setVertex(vertices[vCount++], points[i] + n, 0, 3);
        }
        ASSERT(vCount == count * 2);

        game->getMesh()->renderBuffer(indices, iCount, vertices, vCount);

        if (depth > 0) {
            for (int i = 0; i < 2; i++) {
                vec3 a = points[int(randf() * (count - 1))];
                vec3 b = a;
                b.x += (randf() - 0.5f) * spread;
                b.y  = points[count - 1].y;
                b.z += (randf() - 0.5f) * spread;

                renderPolyline(a, b, width * 0.75f, spread * 0.5f, depth - 1);
            }
        }
    }


    virtual void render(Frustum *frustum, MeshBuilder *mesh, Shader::Type type, bool caustics) {
        Controller::render(frustum, mesh, type, caustics);
        if (!flash) return;

        if (!armed)
            target = game->getLara()->pos;

        Basis b = animation.getJoints(getMatrix(), 0);
        b.rot = quat(0, 0, 0, 1);

        game->setShader(Core::pass, Shader::FLASH, false, false);
        Core::active.shader->setParam(uMaterial, vec4(0.0f, 0.0f, 0.0f, 1.0f));
        Core::active.shader->setParam(uBasis, b);

        Core::setCulling(cfNone);
        Core::setBlending(bmAdd);
        Core::setDepthWrite(false);

        renderPolyline(vec3(0.0f), target - b.pos, 64.0f, 512.0f, 1);

        Core::setDepthWrite(true);
        Core::setBlending(bmNone);
        Core::setCulling(cfFront);
    }
};

struct MidasHand : Controller {
    TR::Entity::Type invItem;
    bool interaction;

    MidasHand(IGame *game, int entity) : Controller(game, entity), invItem(TR::Entity::NONE), interaction(false) {
        activate();
    }

    virtual void update() {
        Character *lara = (Character*)level->laraController;
        
        if (lara->health <= 0.0f || lara->stand != Character::STAND_GROUND || lara->getRoomIndex() != getRoomIndex())
            return;

        vec3 d = (pos - lara->pos).abs();

        if (d.x < 512.0f && d.z < 512.0f) { // check for same sector
            lara->hit(1001.0f, this, TR::HIT_MIDAS);
            deactivate(true);
            return;
        }

        interaction = (d.x < 700.0f && d.z < 700.0f) && lara->state == 2; // 2 = Lara::STATE_STOP

        if (interaction) {
            if (invItem != TR::Entity::NONE) {
                if (invItem == TR::Entity::INV_LEADBAR) {
                    lara->angle.y = PI * 0.5f;
                    lara->pos.x   = pos.x - 612.0f;
                    lara->animation.setAnim(level->models[TR::MODEL_LARA_SPEC].animation);
                    game->invAdd(TR::Entity::PUZZLE_1);
                } else
                    game->playSound(TR::SND_NO, pos, Sound::PAN); // uncompatible item
                invItem = TR::Entity::NONE;
            } else if (Input::state[cAction] && !game->invChooseKey(getEntity().type)) // TODO: add callback for useItem
                game->playSound(TR::SND_NO, pos, Sound::PAN); // no compatible items in inventory
        }
    }
};

struct TrapLava : Controller {
    bool done;

    TrapLava(IGame *game, int entity) : Controller(game, entity), done(false) {}

    virtual void update() {
        Character *lara = (Character*)level->laraController;
        if (lara->health > 0.0f && collide(lara))
            lara->hit(1001.0f, this, TR::HIT_LAVA);

        if (done) {
            deactivate();
            return;
        }

        vec3 dir = getDir();
        pos += dir * (25.0f * 30.0f * Core::deltaTime);

        updateEntity();
        int roomIndex = getRoomIndex();
        TR::Room::Sector *s = level->getSector(roomIndex, int(pos.x + dir.x * 2048.0f), int(pos.y), int(pos.z + dir.z * 2048.0f));
        if (!s || s->floor * 256 != int(pos.y))
            done = true;
        getEntity().room = roomIndex;
    }
};


struct MovingObject : Controller {
    enum {
        STATE_CLOSE,
        STATE_OPEN,
    };

    MovingObject(IGame *game, int entity) : Controller(game, entity) {
        getEntity().flags.collision = true;
    }

    virtual void update() {
        updateAnimation(true);
        animation.setState(isActive() ? STATE_OPEN : STATE_CLOSE);
        pos += getDir() * (animation.getSpeed() * Core::deltaTime * 30.0f);

        TR::Level::FloorInfo info;
        level->getFloorInfo(getRoomIndex(), int(pos.x), int(pos.y), int(pos.z), info);
        if (info.roomNext != TR::NO_ROOM)
            getEntity().room = info.roomNext;
        updateEntity();
    }
};


#define CENTAUR_STATUE_RANGE (3072 + 512)

struct CentaurStatue : Controller {

    CentaurStatue(IGame *game, int entity) : Controller(game, entity) {}

    virtual void update() {
        if (explodeMask) {
            Controller::update();
            if (!explodeMask)
                deactivate(true);
            return;
        }

        if ((pos - game->getLara()->pos).length() < CENTAUR_STATUE_RANGE) {
            explode(0xFFFFFFFF);
            game->playSound(TR::SND_EXPLOSION, pos, Sound::PAN);
            Controller *enemy = game->addEntity(TR::Entity::ENEMY_CENTAUR, getRoomIndex(), pos, angle.y);
            if (enemy)
                enemy->animation.setAnim(7, -36);
        }
    }
};


struct Cabin : Controller {
    enum {
        STATE_UP,
        STATE_DOWN_1,
        STATE_DOWN_2,
        STATE_DOWN_3,
        STATE_GROUND,
    };

    Cabin(IGame *game, int entity) : Controller(game, entity) {}

    virtual void update() {
        TR::Entity &e = getEntity();

        if (e.flags.active == TR::ACTIVE) {
            if (state >= STATE_UP && state <= STATE_DOWN_2)
                animation.setState(state + 1);
            e.flags.active = 0;
        }

        if (state == STATE_GROUND) {
            e.flags.invisible        = true;
            level->flipmap[3].active = TR::ACTIVE;
            level->isFlipped         = !level->isFlipped;
            deactivate(true);
        }

        updateAnimation(true);
    }
};


struct Boat : Controller {
    enum {
        STATE_IDLE = 1,
        STATE_MOVE,
        STATE_STOP,
    };

    Boat(IGame *game, int entity) : Controller(game, entity) {}

    virtual void update() {
        switch (state) {
            case STATE_IDLE : animation.setState(STATE_MOVE); break;
            case STATE_MOVE : animation.setState(STATE_STOP); break;
            case STATE_STOP : deactivate(true); getEntity().flags.invisible = true; break;
        }
        updateAnimation(true);
        pos = pos + getDir() * (animation.getSpeed() * Core::deltaTime * 30.0f);
    }
};


#define MUTANT_EGG_RANGE 4096

struct MutantEgg : Controller {
    enum {
        STATE_IDLE,
        STATE_EXPLOSION,
    };

    TR::Entity::Type enemy;
    
    MutantEgg(IGame *game, int entity) : Controller(game, entity) {
        initMeshOverrides();
        layers[0].mask = 0xff0001ff; // hide dynamic meshes

        switch (getEntity().flags.active) {
            case 1  : enemy = TR::Entity::ENEMY_MUTANT_2;     break;
            case 2  : enemy = TR::Entity::ENEMY_CENTAUR;      break;
            case 4  : enemy = TR::Entity::ENEMY_GIANT_MUTANT; break;
            case 8  : enemy = TR::Entity::ENEMY_MUTANT_3;     break;
            default : enemy = TR::Entity::ENEMY_MUTANT_1;
        }
    }

    virtual void update() {
        if (state != STATE_EXPLOSION) {
            Box box = Box(pos + vec3(-MUTANT_EGG_RANGE), pos + vec3(MUTANT_EGG_RANGE));
            TR::Entity &e = getEntity();
            if ( e.flags.once || e.type == TR::Entity::MUTANT_EGG_BIG || box.contains((((Controller*)level->laraController)->pos)) ) {
                animation.setState(STATE_EXPLOSION);
                layers[0].mask = 0xffffffff & ~(1 << 24);
                explode(0x00fffe00);
                game->addEntity(enemy, getRoomIndex(), pos, angle.y);
            }
        }
        Controller::update();
    }
};


struct KeyHole : Controller {
    KeyHole(IGame *game, int entity) : Controller(game, entity) {}

    virtual bool activate() {
        if (!Controller::activate()) return false;
        getEntity().flags.active = TR::ACTIVE;
        if (getEntity().isPuzzleHole()) {
            int doneIdx = TR::Entity::convToInv(TR::Entity::getItemForHole(getEntity().type)) - TR::Entity::INV_PUZZLE_1;
            meshSwap(0, level->extra.puzzleDone[doneIdx]);
        }
        deactivate();
        return true;
    }

    virtual void update() {}
};


struct Earthquake : Controller {
    float timer;

    Earthquake(IGame *game, int entity) : Controller(game, entity), timer(0.0f) {}

    virtual void update() {
        if (!isActive()) return;

        if (timer < (1.0f / 30.0f)){
            timer += Core::deltaTime;
            return;
        }

        float p = randf();
        if (p < 0.001f) {
            game->playSound(TR::SND_STOMP);
            game->getCamera()->shake = 1.0f;
        } else if (p < 0.04f) {
            game->playSound(TR::SND_BOULDER);
            game->getCamera()->shake = 0.3f;
        }

        timer = 0.0f;
    }
};


struct Waterfall : Controller {
    #define SPLASH_TIMESTEP (1.0f / 30.0f)

    float timer;

    Waterfall(IGame *game, int entity) : Controller(game, entity), timer(0.0f) {}

    virtual void update() {
        updateAnimation(true);

        vec3 delta = (((ICamera*)level->cameraController)->pos - pos) * (1.0f / 1024.0f);
        if (delta.length2() > 100.0f)
            return;

        timer -= Core::deltaTime;
        if (timer > 0.0f) return;
        timer += SPLASH_TIMESTEP * (1.0f + randf() * 0.25f);

        float dropRadius   = randf() * 128.0f + 128.0f;
        float dropStrength = randf() * 0.1f + 0.05f;

        vec2 p = (vec2(randf(), randf()) * 2.0f - 1.0f) * (512.0f - dropRadius);
        vec3 dropPos = pos + vec3(p.x, 0.0f, p.y);
        game->waterDrop(dropPos, dropRadius, dropStrength);

        Sprite::add(game, TR::Entity::WATER_SPLASH, getRoomIndex(), (int)dropPos.x, (int)dropPos.y, (int)dropPos.z);
    } 

    #undef SPLASH_TIMESTEP
};

struct Bubble : Sprite {
    float speed;

    Bubble(IGame *game, int entity) : Sprite(game, entity, true, Sprite::FRAME_RANDOM) {
        speed = (10.0f + randf() * 6.0f) * 30.0f;
    // get water height => bubble life time
        TR::Entity &e = getEntity();
        int dx, dz;
        int room = getRoomIndex();
        int h = e.y;
        while (room != TR::NO_ROOM && level->rooms[room].flags.water) {
            TR::Room::Sector &s = level->getSector(room, e.x, e.z, dx, dz);
            h = s.ceiling * 256;
            room = s.roomAbove;
        }
        time -= (e.y - h) / speed - (1.0f / SPRITE_FPS);
        activate();
    }

    virtual ~Bubble() {
        game->waterDrop(pos, 64.0f, 0.01f);
    }

    virtual void update() {
        pos.y -= speed * Core::deltaTime;
        angle.x += 30.0f * 13.0f * DEG2RAD * Core::deltaTime;
        angle.y += 30.0f *  9.0f * DEG2RAD * Core::deltaTime;
        pos.x += sinf(angle.y) * (11.0f * 30.0f * Core::deltaTime);
        pos.z += cosf(angle.x) * (8.0f  * 30.0f * Core::deltaTime);
        updateEntity();
        Sprite::update();
    }
};

#endif