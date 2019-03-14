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
        if (flags.state == TR::Entity::asInactive) {
            if (state == STATE_DOWN && t > 0.0f) {
                timer = t;
                Controller::activate();
            } else
                Controller::deactivate(true);
            return true;
        }
        return false;
    }

    virtual void deactivate(bool removeFromList) {
        Controller::deactivate(true);
        flags.state = TR::Entity::asInactive;
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
        flags.active = TR::ACTIVE;
        if (!isActive()) {
            animation.setState(STATE_UP);
            timer = 0.0f;
        }
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

struct KeyItemInv : Controller {
    KeyItemInv(IGame *game, int entity) : Controller(game, entity) {
        angle.x = PI * 0.5f;
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

        Controller *lara = game->getLara(pos);
        if (armed && collide(lara)) {
            lara->hit(DART_DAMAGE, this, TR::HIT_DART);
            armed = false;
        }

        TR::Level::FloorInfo info;
        getFloorInfo(getRoomIndex(), pos, info);
        if (pos.y > info.floor || pos.y < info.ceiling || !insideRoom(pos, getRoomIndex())) {
            game->addEntity(TR::Entity::RICOCHET, getRoomIndex(), pos - dir * 64.0f); // with wall offset
            game->removeEntity(this);
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
            vec3 p = pos + vec3(0.0f, -512.0f, 256.0f).rotateY(PI - angle.y);

            game->addEntity(TR::Entity::DART, getRoomIndex(), p, angle.y);
            if (level->extra.smoke != -1)
                game->addEntity(TR::Entity::SMOKE, getRoomIndex(), p);
            game->playSound(TR::SND_DART, p, Sound::PAN);
        }

        updateAnimation(true);
    }
};

#define FLAME_HEAT_DAMAGE 90
#define FLAME_BURN_DAMAGE 150

struct Flame : Sprite {

    static Flame* add(IGame *game, Controller *owner, int jointIndex) {
        ASSERT(owner);

        if (owner && owner->getEntity().isLara()) {
            ((Character*)owner)->burn = true;
        }

        Flame *flame = (Flame*)game->addEntity(TR::Entity::FLAME, owner->getRoomIndex(), owner->pos);

        int jCount = owner->getModel()->mCount;

        if (flame) {
            flame->owner = owner;
            flame->jointIndex = jCount > 1 ? jointIndex : -1;
            ASSERT(flame->jointIndex < jCount);
        }
        return flame;
    }

    Controller *owner;
    int32 jointIndex;
    float sleep;

    Flame(IGame *game, int entity) : Sprite(game, entity, false, Sprite::FRAME_ANIMATED), owner(NULL), jointIndex(0), sleep(0.0f) {
        time = randf() * 3.0f;
        activate();
    }

    virtual bool getSaveData(SaveEntity &data) {
        return false;
    }

    virtual void update() {
        Sprite::update();
        game->playSound(TR::SND_FLAME, pos, Sound::PAN);

        Character *lara = (Character*)((owner && owner->getEntity().isLara()) ? owner : game->getLara(pos));

        if (jointIndex > -1) {
            if (!lara->burn) {
                game->removeEntity(this);
                return;
            }

            pos = lara->getJoint(jointIndex).pos;
            if (jointIndex == 0)
                pos.y += 100.0f;

            lara->hit(FLAME_BURN_DAMAGE * Core::deltaTime, this);
        } else 
            if (lara->health > 0.0f) {
                if (sleep > 0.0f)
                    sleep = max(0.0f, sleep - Core::deltaTime);

                if (sleep == 0.0f && !lara->burn && lara->collide(Sphere(pos, 600.0f))) {
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
                Sound::stop(TR::SND_FLAME);
                game->removeEntity(flame);
                flame = NULL;
            }
            return;
        }

        if (flame) return;

        flame = Flame::add(game, this, -1);
    }
};


#define MUZZLE_FLASH_TIME   0.1f
#define FLASH_LIGHT_COLOR   vec4(0.6f, 0.5f, 0.1f, 1.0f / 3072.0f)

struct MuzzleFlash : Controller {
    Controller *owner;
    int        joint;
    int        lightIndex;

    MuzzleFlash(IGame *game, int entity) : Controller(game, entity), owner(NULL), joint(0), lightIndex(-1) {
        pos.z += (level->version & (TR::VER_TR2 | TR::VER_TR3)) ? 180.0f : 150.0f;
        activate();
        timer = 0.0f;
    }

    virtual void update() {
        timer += Core::deltaTime;
        if (timer < MUZZLE_FLASH_TIME) {
            float intensity = clamp((MUZZLE_FLASH_TIME - timer) * 20.0f, EPS, 1.0f);

            vec4 lightPos   = vec4(owner->getJoint(joint).pos, 0);
            vec4 lightColor = FLASH_LIGHT_COLOR * vec4(intensity, intensity, intensity, 1.0f / sqrtf(intensity));
            if (lightIndex > -1) {
                ASSERT(lightIndex + 1 < MAX_LIGHTS);
                Core::lightPos[lightIndex]   = lightPos;
                Core::lightColor[lightIndex] = lightColor;
            } else
                getRoom().addDynLight(owner->entity, lightPos, lightColor, true);
        } else {
            if (lightIndex > -1) {
                ASSERT(lightIndex < MAX_LIGHTS);
                Core::lightPos[lightIndex]   = vec4(0);
                Core::lightColor[lightIndex] = vec4(0, 0, 0, 1);
            } else
                getRoom().removeDynLight(owner->entity);
            game->removeEntity(this);
        }
    }

    virtual void render(Frustum *frustum, MeshBuilder *mesh, Shader::Type type, bool caustics) {
        ASSERT(level->extra.muzzleFlash);
        ASSERT(owner);

        float alpha = min(1.0f, (0.1f - timer) * 20.0f);
        float lum   = 3.0f;
        Basis b = owner->getJoint(joint);
        b.w = 1.0f;
        b.rotate(quat(vec3(1, 0, 0), -PI * 0.5f));
        b.translate(pos);
        if (level->version & (TR::VER_TR2 | TR::VER_TR3))
            lum = alpha;

        game->setShader(Core::pass, Shader::FLASH, false, false);
        Core::setMaterial(lum * alpha, 0.0f, 0.0f, alpha);
        Core::setBasis(&b, 1);
        Core::setDepthWrite(false);
        mesh->renderModel(level->extra.muzzleFlash);
        Core::setDepthWrite(true);
    }
};

#define LAVA_PARTICLE_DAMAGE  10
#define LAVA_V_SPEED          -165
#define LAVA_H_SPEED          32
#define LAVA_EMITTER_RANGE    (1024 * 10)

struct TrapLavaEmitter : Controller {

    struct Particle {
        vec3  pos;
        vec3  velocity;
        int16 roomIndex;
        int8  frame;

        void update(TR::Level *level, Controller *lara) {
            Controller::applyGravity(velocity.y);
            vec3 opos = pos;
            pos += velocity * (30.0f * Core::deltaTime);

            if (lara->collide(Sphere(pos, 0.0f))) {
                lara->hit(LAVA_PARTICLE_DAMAGE);
                frame = -1;
                return;
            }

            if (level->rooms[roomIndex].flags.water) {
                frame = -1;
                return;
            }

            TR::Room::Sector *sector = level->getSector(roomIndex, pos);
            float floor = level->getFloor(sector, pos);
            float ceiling = level->getCeiling(sector, pos);

            if (pos.y > floor || pos.y < ceiling) {
                vec3 n;

                if (pos.y - floor > 128) {
                    int ix = int(pos.x);
                    int iz = int(pos.z);
                    ix -= ix / 1024 * 1024 + 512;
                    iz -= iz / 1024 * 1024 + 512;

                    if (abs(ix) > abs(iz)) {
                        n = vec3(ix < 0 ? -1.0f : 1.0f, 0, 0);
                    } else {
                        n = vec3(0, 0, iz < 0 ? -1.0f : 1.0f);
                    }

                } else if (pos.y > floor) {
                    n = vec3(0, -1, 0);
                } else if (pos.y < ceiling) {
                    n = vec3(0,  1, 0);
                }

                velocity = velocity.reflect(n) * 0.5f;
                pos      = opos;
                frame--;
            }
        }
    };

    Array<Particle> particles;
    int             spriteIndex;

    TrapLavaEmitter(IGame *game, int entity) : Controller(game, entity) {
        spriteIndex = level->getModelIndex(TR::Entity::LAVA_PARTICLE);
        if (spriteIndex) {
            level->spriteSequences[-(spriteIndex + 1)].transp = 2; // fix blending mode to additive
        }
        particles.capacity = 128;
    }

    void virtual update() {
        if (!spriteIndex)
            return;

        Controller *lara = game->getLara(pos);

        vec3 d = (lara->pos - pos).abs();

        if (isActive() && max(d.x, d.z) < LAVA_EMITTER_RANGE) {
            if (timer <= 0.0f) {
                vec2 d;
                sincos(PI * 2.0f * randf(), &d.x, &d.y);
                d *= randf() * LAVA_H_SPEED;

                Particle part;
                part.pos       = pos;
                part.velocity  = vec3(d.x, randf() * LAVA_V_SPEED, d.y);
                part.roomIndex = getRoomIndex();
                part.frame     = rand() % level->spriteSequences[-(spriteIndex + 1)].sCount;
                particles.push(part);

                game->playSound(TR::SND_LAVA, pos, Sound::PAN);
                timer += 1.0f / 30.0f;

            } else {
                timer -= Core::deltaTime;
            }
        }

        for (int i = 0; i < particles.length; i++) {
            particles[i].update(level, lara);
            if (particles[i].frame < 0) {
                particles.removeFast(i);
                i--;
            }
        }
    }

    virtual void render(Frustum *frustum, MeshBuilder *mesh, Shader::Type type, bool caustics) {
        for (int i = 0; i < particles.length; i++) {
            Particle &part = particles[i];

            uint8 intensity = clamp(int(part.frame * 0.18f * 255.0f), 0, 255);
            Color32 color(intensity, intensity, intensity, 255);

            vec3 p = part.pos - Core::viewPos.xyz();

            mesh->addDynSprite(level->spriteSequences[-(spriteIndex + 1)].sStart + part.frame, short3(int16(p.x), int16(p.y), int16(p.z)), false, false, color, color);
        }
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
        TR::Level::FloorInfo info;
        getFloorInfo(getRoomIndex(), pos, info);

        vec3 dir = getDir();

        bool onGround = false;

        if (pos.y >= info.floor - 256) {
            onGround = true;
            pos.y = info.floor;
            velocity = dir * animation.getSpeed();
            if (state != STATE_ROLL)
                animation.setState(STATE_ROLL);
        } else {
            if (velocity.y == 0.0f)
                velocity.y = 10.0f;
            applyGravity(velocity.y);
            animation.setState(STATE_FALL);
        }

        vec3 p = pos;
        pos += velocity * (30.0f * Core::deltaTime);

        if (info.roomNext != TR::NO_ROOM)
            roomIndex = info.roomNext;

        if (onGround) {
            game->checkTrigger(this, true);
        }

        vec3 v = pos + getDir() * 512.0f;
        getFloorInfo(getRoomIndex(), v, info);
        if (pos.y > info.floor) {
            if (onGround) {
                pos = p;
                deactivate(true);
                game->checkTrigger(this, true);
                return;
            } else {
                pos.x = p.x;
                pos.z = p.z;
                velocity.x = velocity.z = 0.0f;
            }
        }

        Character *lara = (Character*)game->getLara(pos);
        if (lara->health > 0.0f && collide(lara)) {
            if (lara->stand == Character::STAND_GROUND)
                lara->hit(BOULDER_DAMAGE_GROUND, this, TR::HIT_BOULDER);
            if (lara->stand == Character::STAND_AIR)
                lara->hit(BOULDER_DAMAGE_AIR * 30.0f * Core::deltaTime, this);
        }

        updateAnimation(true);
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

    virtual void setSaveData(const SaveEntity &data) {
        updateFloor(false);
        Controller::setSaveData(data);
        if (state == STATE_STAND)
            updateFloor(true);
    }

    void updateFloor(bool rise) {
        TR::Level::FloorInfo info;
        getFloorInfo(getRoomIndex(), pos, info);
        if (info.roomNext != 0xFF)
            roomIndex = info.roomNext;
        int dx, dz;
        TR::Room::Sector &s = level->getSector(getRoomIndex(), int(pos.x), int(pos.z), dx, dz);
        s.floor += rise ? -4 : 4;
    }

    bool doMove(bool push) {
    // check floor height of next floor
        vec3 dir = getDir() * (push ? 1024.0f : -1024.0f);
        TR::Level::FloorInfo info;

        vec3 p = pos + dir;
        getFloorInfo(getRoomIndex(), p, info);

        if ((info.slantX | info.slantZ) || info.floor != pos.y || info.floor - info.ceiling < 1024)
            return false;

        // check for trapdoor
        int px = int(p.x) / 1024;
        int pz = int(p.z) / 1024;
        for (int i = 0; i < info.trigCmdCount; i++) {
            if (info.trigCmd[i].action == TR::Action::ACTIVATE) {
                TR::Entity &e = level->entities[info.trigCmd[i].args];
                vec3 objPos = ((Controller*)e.controller)->pos;
                if ((e.type == TR::Entity::TRAP_DOOR_1 || e.type == TR::Entity::TRAP_DOOR_2) && px == int(objPos.x) / 1024 && pz == int(objPos.z) / 1024)
                    return false;
            } else if (info.trigCmd[i].action == TR::Action::CAMERA_SWITCH)
                i++; // skip camera switch delay info
        }

        // check Laras destination position
        if (!push) {
            dir = getDir() * (-2048.0f);
            getFloorInfo(getRoomIndex(), pos + dir, info);
            if ((info.slantX | info.slantZ) || info.floor != pos.y || info.floor - info.ceiling < 1024)
                return false;
        }

        if (!animation.setState(push ? STATE_PUSH : STATE_PULL))
            return false;
        updateFloor(false);
        activate();
        return true;
    }

    virtual void update() {
        TR::Level::FloorInfo info;        
        getFloorInfo(getRoomIndex(), pos, info);

        if (pos.y < info.floor) {
            if (info.roomBelow != TR::NO_ROOM)
                roomIndex = info.roomBelow;

            applyGravity(velocity);
            pos.y += velocity * (30.0f * Core::deltaTime);

            if (pos.y >= info.floor) {
                velocity = 0.0f;
                pos.y    = info.floor;
                game->setEffect(this, TR::Effect::FLOOR_SHAKE);
                game->playSound(TR::SND_BOULDER, pos, Sound::PAN);
                deactivate(true);
                updateFloor(true);
            }
        } else {
            if (state == STATE_STAND) return;
            updateAnimation(true);
            if (state == STATE_STAND) {
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
        if (!flags.invisible)
            updateFloor(true);
    }

    virtual void setSaveData(const SaveEntity &data) {
        updateFloor(false);
        Controller::setSaveData(data);
        if (state != STATE_MOVE)
            updateFloor(true);
    }

    void updateFloor(bool rise) {
        TR::Level::FloorInfo info;
        getFloorInfo(getRoomIndex(), pos, info);
        if (info.roomNext != 0xFF)
            roomIndex = info.roomNext;
        int dx, dz;
        TR::Room::Sector &s = level->getSector(getRoomIndex(), int(pos.x), int(pos.z), dx, dz);
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

        if (flags.state == TR::Entity::asInactive) {
            if (flags.active == TR::ACTIVE)
                flags.state = TR::Entity::asActive; // stay in active items list
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
            roomIndex[1] = level->getNextRoom(&sectors[0]);

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
        vec3 p = pos - getDir() * 1024.0f;
        block[0] = BlockInfo(level, getRoomIndex(), int(pos.x), int(pos.z), int(p.x), int(p.z), false);
        block[1] = BlockInfo(level, getRoomIndex(), int(pos.x), int(pos.z), int(p.x), int(p.z), true);
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

    TrapDoor(IGame *game, int entity) : Controller(game, entity) {}
    
    virtual bool isCollider() {
        int targetState = isActive(false) ? STATE_OPEN : STATE_CLOSE;
        return state == targetState && state == STATE_CLOSE;
    }

    virtual void update() {
        updateAnimation(true);
        int targetState = isActive() ? STATE_OPEN : STATE_CLOSE;

        if (state != targetState)
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

    float velocity;

    TrapFloor(IGame *game, int entity) : Controller(game, entity), velocity(0) {}

    virtual bool isCollider() {
        return state == STATE_STATIC || state == STATE_SHAKE;
    }

    virtual bool activate() {
        if (state != STATE_STATIC) return false;
        vec3 &p = game->getLara(pos)->pos;
        if (fabsf(p.y - (pos.y - 512.0f)) <= 8 && Controller::activate()) {
            animation.setState(STATE_SHAKE);
            return true;
        }
        return false;
    }

    virtual void update() {
        updateAnimation(true);
        if (state == STATE_FALL) {
            applyGravity(velocity);
            pos.y += velocity * (30.0f * Core::deltaTime);

            TR::Level::FloorInfo info;
            getFloorInfo(getRoomIndex(), pos, info);

            if (pos.y > info.roomFloor && info.roomBelow != 0xFF)
                roomIndex = info.roomBelow;

            if (pos.y > info.floor) {
                pos.y = (float)info.floor;
                animation.setState(STATE_DOWN);
            }
        }
    }
};

struct Bridge : Controller {
    Bridge(IGame *game, int entity) : Controller(game, entity) {}
};

struct Drawbridge : Controller {
    enum {
        STATE_UP,
        STATE_DOWN,
    };

    Drawbridge(IGame *game, int entity) : Controller(game, entity) {}

    virtual bool isCollider() {
        return flags.active != TR::ACTIVE;
    }

    virtual void update() {
        updateAnimation(true);
        animation.setState(isActive() ? STATE_DOWN : STATE_UP);
    }
};


#define CRYSTAL_LIGHT_RADIUS 1024.0f
#define CRYSTAL_LIGHT_COLOR  vec4(0.1f, 0.1f, 3.0f, 1.0f / CRYSTAL_LIGHT_RADIUS)

struct Crystal : Controller {
    Texture *environment;

    Crystal(IGame *game, int entity) : Controller(game, entity) {
        environment = new Texture(64, 64, 1, FMT_RGBA, OPT_CUBEMAP | OPT_MIPMAPS | OPT_TARGET);
        activate();
    }

    virtual ~Crystal() {
        delete environment;
    }

    virtual bool isCollider() {
        return !flags.invisible;
    }

    virtual void deactivate(bool removeFromList = false) {
        flags.invisible = true;
        Controller::deactivate(removeFromList);
        getRoom().removeDynLight(entity);
    }

    virtual void update() {
        updateAnimation(false);
        vec3 lightPos = getJoint(0).pos - vec3(0, 256, 0);
        getRoom().addDynLight(entity, vec4(lightPos, 0.0f), CRYSTAL_LIGHT_COLOR);
    }

    virtual void render(Frustum *frustum, MeshBuilder *mesh, Shader::Type type, bool caustics) {
        Core::setMaterial(0.5, 0.5, 3.0, 1.0f);
        environment->bind(sEnvironment);
        Controller::render(frustum, mesh, type, caustics);
    }
};


#define CRYSTAL_PICKUP_LIGHT_COLOR  vec4(0.1f, 0.5f, 0.1f, 1.0f / CRYSTAL_LIGHT_RADIUS)

struct CrystalPickup : Controller {

    CrystalPickup(IGame *game, int entity) : Controller(game, entity) {
        activate();
    }

    virtual void deactivate(bool removeFromList = false) {
        Controller::deactivate(removeFromList);
        getRoom().removeDynLight(entity);
    }

    virtual void update() {
        updateAnimation(false);
        vec3 lightPos = getJoint(0).pos;
        getRoom().addDynLight(entity, vec4(lightPos, 0.0f), CRYSTAL_PICKUP_LIGHT_COLOR);
    }
};

#define BLADE_DAMAGE    100
#define BLADE_RANGE     1024

struct TrapSwingBlade : Controller {
    enum {
        STATE_STATIC = 0,
        STATE_SWING  = 2,
    };

    TrapSwingBlade(IGame *game, int entity) : Controller(game, entity) {}

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

        Character* lara = (Character*)game->getLara(pos);
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
        if (getRoom().flags.water)
            return;

        Character *lara = (Character*)game->getLara(pos);
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

    float velocity;

    TrapCeiling(IGame *game, int entity) : Controller(game, entity), velocity(0) {}

    virtual void update() {
        updateAnimation(true);

        if (state == STATE_STATIC)
            animation.setState(STATE_FALL);
       
        if (state == STATE_FALL) {
            applyGravity(velocity);
            pos.y += velocity * (30.0f * Core::deltaTime);

            TR::Level::FloorInfo info;
            getFloorInfo(getRoomIndex(), pos, info);

            if (pos.y > info.roomFloor && info.roomBelow != 0xFF)
                roomIndex = info.roomBelow;

            if (pos.y > info.floor) {
                pos.y = (float)info.floor;
                animation.setState(STATE_DOWN);
            }

            Controller *lara = game->getLara(pos);
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

            Character *lara = (Character*)game->getLara(pos);
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
        getFloorInfo(getRoomIndex(), pos, info);

        Controller *lara = game->getLara(pos);

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
            if (pos.y > info.floor) {
                pos.y = info.floor;
                game->playSound(TR::SND_SWORD, pos, Sound::PAN);
                deactivate(true);
            }

            if (collide(lara))
                lara->hit(SWORD_DAMAGE * 30.0f * Core::deltaTime, this, TR::HIT_SWORD); // TODO: push lara
        }
    }
};


struct ThorHammerBlock : Controller {
    Controller *handle;

    ThorHammerBlock(IGame *game, int entity) : Controller(game, entity), handle(NULL) {}

    virtual bool isCollider() {
        return handle ? handle->isCollider() : false;
    }

    virtual bool getSaveData(SaveEntity &data) {
        return false;
    }
};

struct ThorHammer : Controller {
    enum {
        STATE_IDLE,
        STATE_START,
        STATE_FALL,
        STATE_DOWN,
    };

    ThorHammerBlock *block;

    ThorHammer(IGame *game, int entity) : Controller(game, entity) {
        initBlock();
    }

    void initBlock() {
        block = (ThorHammerBlock*)game->addEntity(TR::Entity::HAMMER_BLOCK, getRoomIndex(), pos, angle.y);
        ASSERT(block);
        if (block)
            block->handle = this;
    }

    void updateBlock() {
        block->animation.frameA = animation.frameA;
        block->animation.frameB = animation.frameB;
        block->animation.delta  = animation.delta;
    }

    virtual void setSaveData(const SaveEntity &data) {
        Controller::setSaveData(data);
        initBlock();
        updateBlock();
    }

    virtual bool isCollider() {
        return state == STATE_DOWN;
    }

    virtual void update() {
        switch (state) {
            case STATE_IDLE  : if (isActive()) animation.setState(STATE_START); break;
            case STATE_START : animation.setState(isActive() ? STATE_FALL : STATE_IDLE); break;
            case STATE_FALL  : {
                Character *lara = (Character*)game->getLara(pos);
                if (animation.frameIndex > 30 && lara->health > 0.0f) {
                    vec3 d = pos + getDir() * (3.0f * 1024.0f) - lara->pos;
                    if (fabsf(d.x) < 520.0f && fabsf(d.z) < 520.0f)
                        lara->hit(1001, this, TR::HIT_BOULDER);
                }
                break;
            }
            case STATE_DOWN : {
                game->checkTrigger(this, 1); 
                deactivate(true);
                break;
            }
        }

        updateAnimation(true);
        if (block)
            updateBlock();
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

            Character *lara = (Character*)game->getLara(pos);

            if (timer <= 0.0f) {
                if (flash) {
                    if (level->state.flags.flipped)
                        game->flipMap();
                    flash = false;
                    armed = true;
                    timer = (35.0f + randf() * 45.0f) / 30.0f;                    
                } else {
                    if (!level->state.flags.flipped)
                        game->flipMap();
                    flash = true;
                    timer = 20.0f / 30.0f;

                    bool hasTargets = getModel()->mCount > 1; // LEVEL4 has, LEVEL10C not

                    if ((lara->pos - pos).length() < (hasTargets ? 2560.0f : 1024.0f)) {
                        lara->hit(LIGHTNING_DAMAGE, this, TR::HIT_LIGHTNING);
                        armed = false;
                    } else if (!hasTargets) {
                        target = pos + vec3(0.0f, 1024.0f, 0.0f);
                    } else
                        target = getJoint(1 + int(randf() * 5)).pos;
                }
                game->playSound(TR::SND_LIGHTNING, pos, Sound::PAN);
            }
        } else {
            timer = 0.0f;
            flash = false;
            if (level->state.flags.flipped)
                game->flipMap();
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
        TR::TextureInfo &tex = CommonTex[CTEX_FLASH];
        v.coord     = toCoord(coord, joint);
        v.normal    = short4( 0, -1, 0, 0 );
        v.texCoord  = short4( tex.texCoordAtlas[idx].x, tex.texCoordAtlas[idx].y, 32767, 32767 );
        v.color     = ubyte4( 255, 255, 255, 255 );
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
        vec3 dir = Core::mViewInv.dir().xyz();

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
            target = game->getLara(pos)->pos;

        Basis b = getJoint(0);
        b.rot = quat(0, 0, 0, 1);

        game->setShader(Core::pass, Shader::FLASH, false, false);
        Core::setMaterial(0.0f, 0.0f, 0.0f, 1.0f);
        Core::setBasis(&b, 1);

        Core::setCullMode(cmNone);
        Core::setBlendMode(bmAdd);
        Core::setDepthWrite(false);

        renderPolyline(vec3(0.0f), target - b.pos, 64.0f, 512.0f, 1);

        Core::setDepthWrite(true);
        Core::setBlendMode(bmNone);
        Core::setCullMode(cmFront);
    }
};

struct MidasHand : Controller {
    TR::Entity::Type invItem;
    bool interaction;

    MidasHand(IGame *game, int entity) : Controller(game, entity), invItem(TR::Entity::NONE), interaction(false) {
        activate();
    }

    virtual void update() {
        Character *lara = (Character*)game->getLara(pos);
        
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
            } else if (Input::state[0][cAction] && !game->invChooseKey(0, getEntity().type)) // TODO: add callback for useItem // TODO: player[1]
                game->playSound(TR::SND_NO, pos, Sound::PAN); // no compatible items in inventory
        }
    }
};

struct TrapLava : Controller {
    TrapLava(IGame *game, int entity) : Controller(game, entity) {}

    virtual void update() {
        Character *lara = (Character*)game->getLara(pos);
        if (lara->health > 0.0f && collide(lara))
            lara->hit(1001.0f, this, TR::HIT_LAVA);

        vec3 dir = getDir();

        roomIndex = getRoomIndex();
        TR::Room::Sector *s = level->getSector(roomIndex, pos + dir * 2048.0f);
        if (!s || s->floor * 256 != int(pos.y))
            return;

        pos += dir * (25.0f * 30.0f * Core::deltaTime);
    }
};


struct MovingObject : Controller {
    enum {
        STATE_CLOSE,
        STATE_OPEN,
    };

    MovingObject(IGame *game, int entity) : Controller(game, entity) {}

    virtual bool isCollider() {
        return true;
    }

    virtual void update() {
        updateAnimation(true);
        animation.setState(isActive() ? STATE_OPEN : STATE_CLOSE);
        pos += getDir() * (animation.getSpeed() * Core::deltaTime * 30.0f);

        TR::Level::FloorInfo info;
        getFloorInfo(getRoomIndex(), pos, info);
        if (info.roomNext != TR::NO_ROOM)
            roomIndex = info.roomNext;
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

        if ((pos - game->getLara(pos)->pos).length() < CENTAUR_STATUE_RANGE) {
            explode(0xFFFFFFFF, 0.0f);
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
        if (flags.active == TR::ACTIVE) {
            if (state >= STATE_UP && state <= STATE_DOWN_2)
                animation.setState(state + 1);
            flags.active = 0;
        }

        if (state == STATE_GROUND) {
            flags.invisible        = true;
            level->state.flipmaps[3].active = TR::ACTIVE;
            game->flipMap();
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
            case STATE_STOP : deactivate(true); flags.invisible = true; break;
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

        switch (flags.active) {
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
            if ( flags.once || getEntity().type == TR::Entity::MUTANT_EGG_BIG || box.contains(((game->getLara(pos))->pos)) ) {
                animation.setState(STATE_EXPLOSION);
                layers[0].mask = 0xffffffff & ~(1 << 24);
                explode(0x00fffe00, 0.0f);
                game->addEntity(enemy, getRoomIndex(), pos, angle.y);
            }
        }
        Controller::update();
    }

    virtual void setSaveData(const SaveEntity &data) {
        Controller::setSaveData(data);
        visibleMask = (state == STATE_IDLE) ? 0xff0001ff : (0xffffffff & ~(1 << 24));
    }
};


struct KeyHole : Controller {
    KeyHole(IGame *game, int entity) : Controller(game, entity) {}

    virtual bool activate() {
        if (!Controller::activate()) return false;
        flags.active = TR::ACTIVE;
        deactivate();
        return true;
    }

    virtual const TR::Model* getModel() {
        if (getEntity().isPuzzleHole() && flags.active == TR::ACTIVE) {
            int doneIdx = TR::Level::convToInv(TR::Entity::getItemForHole(getEntity().type)) - TR::Entity::INV_PUZZLE_1;
            return &level->models[level->extra.puzzleDone[doneIdx]];
        }
        return Controller::getModel();
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
            game->shakeCamera(1.0f);
        } else if (p < 0.04f) {
            game->playSound(TR::SND_BOULDER);
            game->shakeCamera(0.3f);
        }

        timer = 0.0f;
    }
};


struct Waterfall : Controller {
    #define SPLASH_TIMESTEP (1.0f / 30.0f)

    float timer;

    Waterfall(IGame *game, int entity) : Controller(game, entity), timer(0.0f) {}

    virtual void update() {
        if (getRoom().alternateRoom != -1 && level->state.flags.flipped) // room is flipped
            return;

        vec3 delta = (game->getLara(pos)->pos - pos) * (1.0f / 1024.0f);
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
        if (level->extra.waterSplash > -1)
            game->addEntity(TR::Entity::WATER_SPLASH, getRoomIndex(), dropPos);
    } 

    #undef SPLASH_TIMESTEP
};

struct Bubble : Sprite {
    float speed;

    Bubble(IGame *game, int entity) : Sprite(game, entity, true, Sprite::FRAME_RANDOM) {
        speed = (10.0f + randf() * 6.0f) * 30.0f;
    // get water height => bubble life time
        int dx, dz;
        int room = getRoomIndex();
        int h = int(pos.y);
        while (room != TR::NO_ROOM && level->rooms[room].flags.water) {
            TR::Room::Sector &s = level->getSector(room, int(pos.x), int(pos.z), dx, dz);
            h = s.ceiling * 256;
            room = s.roomAbove;
        }
        if (pos.y < h)
            time = 1.0f / SPRITE_FPS;
        else
            time -= (pos.y - h) / speed - (1.0f / SPRITE_FPS);
        activate();
    }

    virtual ~Bubble() {
        game->waterDrop(pos, 64.0f, 0.01f);
    }

    virtual bool getSaveData(SaveEntity &data) {
        return false;
    }

    virtual void update() {
        pos.y -= speed * Core::deltaTime;
        angle.x += 30.0f * 13.0f * DEG2RAD * Core::deltaTime;
        angle.y += 30.0f *  9.0f * DEG2RAD * Core::deltaTime;
        pos.x += sinf(angle.y) * (11.0f * 30.0f * Core::deltaTime);
        pos.z += cosf(angle.x) * (8.0f  * 30.0f * Core::deltaTime);
        Sprite::update();
    }
};


struct Explosion : Sprite {

    Explosion(IGame *game, int entity) : Sprite(game, entity, true, Sprite::FRAME_ANIMATED) {
        game->playSound(TR::SND_EXPLOSION, pos, Sound::PAN);
        level->spriteSequences[-(getEntity().modelIndex + 1)].transp = 2; // fix blending mode to additive
    }

    virtual bool getSaveData(SaveEntity &data) {
        return false;
    }
};


struct BreakableWindow : Controller {

    BreakableWindow(IGame *game, int entity) : Controller(game, entity) {
        initMeshOverrides();
        layers[0].mask = 0x00000001;
    }
};


#define HELICOPTER_SPEED 3000
#define HELICOPTER_RANGE (1024 * 30)

struct HelicopterFlying : Controller {

    HelicopterFlying(IGame *game, int entity) : Controller(game, entity) {}

    virtual void update() {
        pos.z += HELICOPTER_SPEED * Core::deltaTime;
        updateAnimation(false);
        updateRoom();

        Controller *lara = game->getLara(pos);

        float dist = pos.z - lara->pos.z;

        Sound::Sample *sample = game->playSound(TR::SND_HELICOPTER, vec3(0.0), 0);
        if (sample) {
            sample->volume = (1.0f - dist / HELICOPTER_RANGE) * 0.8f;
        }

        if (fabsf(dist) > HELICOPTER_RANGE) {
            Sound::stop(TR::SND_HELICOPTER);
            flags.invisible = true;
            deactivate(true);
        }
    }
};


#define STONE_ITEM_LIGHT_RADIUS 2048.0f

struct StoneItem : Controller {
    float phase;

    StoneItem(IGame *game, int entity) : Controller(game, entity), phase(0) {
        activate();
    }

    virtual void deactivate(bool removeFromList = false) {
        Controller::deactivate(removeFromList);
        getRoom().removeDynLight(entity);
    }

    virtual void update() {
        updateAnimation(false);
        
        angle.y += Core::deltaTime * 2.0f;
        phase += Core::deltaTime;
        float s = 0.3f + (sinf(phase * PI2) * 0.5f + 0.5f) * 0.7f;

        vec4 lightColor(0.1f * s, 1.0f * s, 1.0f * s, 1.0f / STONE_ITEM_LIGHT_RADIUS);
        vec3 lightPos = getJoint(0).pos;

        getRoom().addDynLight(entity, vec4(lightPos, 0.0f), lightColor);
    }
};

#define CENTAUR_BULLET_DIST   SQR(1024.0f)
#define CENTAUR_BULLET_DAMAGE 100.0f
#define MUTANT_BULLET_DAMAGE  30.0f

struct Bullet : Controller {
    vec3 velocity;

    Bullet(IGame *game, int entity) : Controller(game, entity) {
        velocity  = vec3(200.0f) * 30.0f;
        intensity = intensityf(4096);
        activate();
    }

    virtual bool getSaveData(SaveEntity &data) {
        return false;
    }

    void setAngle(const vec3 &ang) {
        angle = ang;
        float speed;
        switch (getEntity().type) {
            case TR::Entity::NATLA_BULLET   : speed = 220.0f; break;
            case TR::Entity::MUTANT_BULLET  : speed = 250.0f; break;
            case TR::Entity::CENTAUR_BULLET : speed = 220.0f; break;
            default : speed = 200.0f;
        }
        velocity = getDir() * (speed * 30.0f);
    }

    virtual void update() {
        //getRoom().removeDynLight(entity);
        pos = pos + velocity * Core::deltaTime;

        level->getSector(roomIndex, pos);

        Controller::update();
        //getRoom().addDynLight(entity, pos, vec4(1, 1, 0, 1.0f / 1024.0f));

        bool directHit = false;

        Character *lara = (Character*)game->getLara(pos);
        if (!collide(lara)) {
            TR::Level::FloorInfo info;
            getFloorInfo(getRoomIndex(), pos, info);
            if (!(pos.y > info.floor || pos.y < info.ceiling || !insideRoom(pos, getRoomIndex())))
                return;
        } else
            directHit = true;

        //getRoom().removeDynLight(entity);
        switch (getEntity().type) {
            case TR::Entity::CENTAUR_BULLET :
                if (directHit) {
                    lara->hit(CENTAUR_BULLET_DAMAGE);
                } else {
                    for (int i = 0; i < 2; i++) {
                        Controller *lara = game->getLara(i);
                        if (!lara) continue;

                        float dist = (pos - lara->pos).length2();
                        if (dist < CENTAUR_BULLET_DIST)
                            lara->hit(CENTAUR_BULLET_DAMAGE * (CENTAUR_BULLET_DIST - dist) / CENTAUR_BULLET_DIST);
                    }
                }

                game->addEntity(TR::Entity::EXPLOSION, getRoomIndex(), pos);
                break;
            case TR::Entity::MUTANT_BULLET  :
                if (directHit)
                    lara->hit(MUTANT_BULLET_DAMAGE);
                else
                    game->getLara()->addRicochet(pos - getDir() * 64.0f, true);
                break;
            default : ;
        }

        if (directHit)
            game->playSound(lara->stand == Character::STAND_UNDERWATER ? TR::SND_HIT_UNDERWATER : TR::SND_HIT, lara->pos, Sound::PAN);

        game->removeEntity(this);
    }
};

#endif