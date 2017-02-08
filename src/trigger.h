#ifndef H_TRIGGER
#define H_TRIGGER

#include "core.h"
#include "controller.h"
#include "sprite.h"

struct Trigger : Controller {

    bool  immediate;
    float timer;
    int   baseState;

    Trigger(IGame *game, int entity, bool immediate) : Controller(game, entity), immediate(immediate), timer(0.0f) {
        baseState = state;
        getEntity().flags.collision = false;
    }

    bool inState() {
        return (state != baseState) == (getEntity().flags.active != 0);
    }

    virtual bool activate(ActionCommand *cmd) {
        if (this->timer != 0.0f || !inState() || actionCommand) return false;
        Controller::activate(cmd);
        this->timer = cmd->timer;

        getEntity().flags.active ^= 0x1F;
        
        if (immediate)
            activateNext();

        return true;
    }

    virtual void update() {
        TR::Entity &entity = getEntity();

        if (timer > 0.0f) {
            timer -= Core::deltaTime;
            if (timer <= 0.0f) {
                timer = 0.0f;
                entity.flags.active ^= 0x1F;
            }
        }

        if (timer < 0.0f) {
            timer += Core::deltaTime;
            if (timer >= 0.0f) {
                timer = 0.0f;
                entity.flags.active ^= 0x1F;
            }
        }

        if (!inState() && entity.type != TR::Entity::HOLE_KEY && entity.type != TR::Entity::HOLE_PUZZLE)
            animation.setState(state != baseState ? baseState : (entity.type == TR::Entity::TRAP_BLADE ? 2 : (baseState ^ 1)));

        updateAnimation(true);
        updateEntity();
    }
};

struct Dart : Controller {
    vec3 velocity;
    vec3 dir;
    bool inWall;    // dart starts from wall

    Dart(IGame *game, int entity) : Controller(game, entity), inWall(true) {
        dir = vec3(sinf(angle.y), 0, cosf(angle.y));
    }

    virtual void update() {
        velocity = dir * animation.getSpeed();
        pos = pos + velocity * (Core::deltaTime * 30.0f);
        updateEntity();
        TR::Level::FloorInfo info;
        level->getFloorInfo(getRoomIndex(), (int)pos.x, (int)pos.y, (int)pos.z, info);
        if (pos.y > info.floor || pos.y < info.ceiling || !insideRoom(pos, getRoomIndex())) {
            if (!inWall) {
                TR::Entity &e = getEntity();
                
                vec3 p = pos - dir * 64.0f; // wall offset = 64
                Sprite::add(game, TR::Entity::SPARK, e.room, (int)p.x, (int)p.y, (int)p.z, Sprite::FRAME_RANDOM);

                level->entityRemove(entity);
                delete this;
            }
        } else
            inWall = false;
    }
};

struct Dartgun : Trigger {
    vec3 origin;

    Dartgun(IGame *game, int entity) : Trigger(game, entity, true), origin(pos) {}

    virtual bool activate(ActionCommand *cmd) {
        if (!Trigger::activate(cmd))
            return false;
        
        // add dart (bullet)
        TR::Entity &entity = getEntity();

        vec3 p = pos + vec3(0.0f, -512.0f, 256.0f).rotateY(PI - entity.rotation);

        int dartIndex = level->entityAdd(TR::Entity::TRAP_DART, entity.room, (int)p.x, (int)p.y, (int)p.z, entity.rotation, entity.intensity);
        if (dartIndex > -1)
            level->entities[dartIndex].controller = new Dart(game, dartIndex);

        Sprite::add(game, TR::Entity::SMOKE, entity.room, (int)p.x, (int)p.y, (int)p.z);

        playSound(TR::SND_DART, pos, Sound::Flags::PAN);

        return true;
    }

};

struct Boulder : Trigger {

    Boulder(IGame *game, int entity) : Trigger(game, entity, true) {}

    virtual void update() {
        if (getEntity().flags.active) {
            updateAnimation(true);
            updateEntity();
        }
    }
};

// not a trigger
struct Block : Controller {

    enum {
        STATE_STAND = 1,
        STATE_PUSH,
        STATE_PULL,
    };

    Block(IGame *game, int entity) : Controller(game, entity) {
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
        vec3 dir = getDir() * (push ? 1024.0f : -2048.0f);
        TR::Entity &e = getEntity();
        TR::Level::FloorInfo info;
        level->getFloorInfo(e.room, e.x + (int)dir.x, e.y, e.z + (int)dir.z, info);
        if ((info.slantX | info.slantZ) || info.floor != e.y)
            return false;        
        if (!animation.setState(push ? STATE_PUSH : STATE_PULL))
            return false;
        updateFloor(false);
        return true;
    }

    virtual void update() {
        if (state == STATE_STAND) return;
        updateAnimation(true);
        if (state == STATE_STAND) {
            updateEntity();
            updateFloor(true);
        }
    }
};


struct Door : Trigger {
    int8 *floor[2], orig[2];

    Door(IGame *game, int entity) : Trigger(game, entity, true) {
        TR::Entity &e = getEntity();
        TR::Level::FloorInfo info;
        vec3 p = pos - getDir() * 1024.0f;

        level->getFloorInfo(e.room, (int)p.x, (int)p.y, (int)p.z, info);
        int dx, dz;
        TR::Room::Sector *s = &level->getSector(e.room, (int)p.x, (int)p.z, dx, dz);

        orig[0] = *(floor[0] = &s->floor);

        if (info.roomNext != 0xFF) {
            s = &level->getSector(info.roomNext, e.x, e.z, dx, dz);
            orig[1] = *(floor[1] = &s->floor);
        } else
            floor[1] = NULL;

        updateBlock();
    }

    void updateBlock() {
        int8 v[2];
        if (getEntity().flags.active) {
            v[0] = orig[0];
            v[1] = orig[1];
        } else
            v[0] = v[1] = TR::FLOOR_BLOCK;

        if (floor[0]) *floor[0] = v[0];
        if (floor[1]) *floor[1] = v[1];
    }

    virtual bool activate(ActionCommand *cmd) {
        bool res = Trigger::activate(cmd);
        updateBlock();
        return res;
    }
};

struct TrapDoor : Trigger {

    TrapDoor(IGame *game, int entity) : Trigger(game, entity, true) {
        getEntity().flags.collision = true;
    }

    virtual bool activate(ActionCommand *cmd) {
        bool res = Trigger::activate(cmd);
        getEntity().flags.collision = !getEntity().flags.active;
        return res;
    }

};


struct TrapFloor : Trigger {

    enum {
        STATE_STATIC,
        STATE_SHAKE,
        STATE_FALL,
        STATE_DOWN,
    };
    float velocity;

    TrapFloor(IGame *game, int entity) : Trigger(game, entity, true), velocity(0) {
        TR::Entity &e = getEntity();
        e.flags.collision = true;
    }

    virtual bool activate(ActionCommand *cmd) {
        TR::Entity &e = level->entities[cmd->emitter];
        if (e.type != TR::Entity::LARA) return true;
        int ey = (int)pos.y - 512; // real floor object position
        return (abs(e.y - ey) <= 8) ? Trigger::activate(cmd) : true;
    }

    virtual void update() {
        Trigger::update();
        if (state == STATE_FALL) {
            TR::Entity &e = getEntity();
            e.flags.collision = false;
            velocity += GRAVITY * 30 * Core::deltaTime;
            pos.y += velocity * Core::deltaTime;

            TR::Level::FloorInfo info;
            level->getFloorInfo(e.room, e.x, (int)pos.y, e.z, info);

            if (pos.y > info.roomFloor && info.roomBelow != 0xFF)
                e.room = info.roomBelow;

            if (pos.y > info.floor) {
                pos.y = (float)info.floor;
                animation.setState(STATE_DOWN);
            }
            updateEntity();
        }
    }
};


struct Bridge : Trigger {

    Bridge(IGame *game, int entity) : Trigger(game, entity, true) {
        getEntity().flags.collision = true;
    }
};

struct Crystal : Controller {
    Texture *environment;

    Crystal(IGame *game, int entity) : Controller(game, entity) {
        environment = new Texture(64, 64, Texture::RGBA, true);
    }

    virtual ~Crystal() {
        delete environment;
    }

    virtual void render(Frustum *frustum, MeshBuilder *mesh) {
        Shader *sh = Core::active.shader;
        sh->setParam(uType,  Shader::MIRROR);
        sh->setParam(uColor, vec4(0.4f, 0.4f, 16.0f, 1.0f)); // blue color dodge
        environment->bind(sEnvironment);
        Controller::render(frustum, mesh);
        sh->setParam(uType, Shader::ENTITY);
    }
};

struct Waterfall : Trigger {
    #define SPLASH_TIMESTEP (1.0f / 30.0f)

    float timer;
    bool  drop;
    float dropRadius;
    float dropStrength;
    vec3  dropPos;

    Waterfall(IGame *game, int entity) : Trigger(game, entity, true), timer(0.0f) {}

    virtual void update() {
        drop = false;
        Trigger::update();
        if (!getEntity().flags.active) return;

        vec3 delta = (((Controller*)level->cameraController)->pos - pos) * (1.0f / 1024.0f);
        if (delta.length2() > 100.0f)
            return;

        timer -= Core::deltaTime;
        if (timer > 0.0f) return;
        timer += SPLASH_TIMESTEP * (1.0f + randf() * 0.25f);

        drop         = true;
        dropRadius   = randf() * 128.0f + 128.0f;
        dropStrength = randf() * 0.1f + 0.05f;

        vec2 p = (vec2(randf(), randf()) * 2.0f - 1.0f) * (512.0f - dropRadius);
        dropPos = pos + vec3(p.x, 0.0f, p.y);

        Sprite::add(game, TR::Entity::WATER_SPLASH, getRoomIndex(), (int)dropPos.x, (int)dropPos.y, (int)dropPos.z);
    } 

    #undef SPLASH_TIMESTEP
};

struct Bubble : Sprite {
    float speed;

    Bubble(IGame *game, int entity) : Sprite(game, entity, true, Sprite::FRAME_RANDOM) {
        speed = (10.0f + randf() * 6.0) * 30.0f;
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
    }

    virtual ~Bubble() {
        game->waterDrop(pos, 64.0f, 0.01f);
    }

    virtual void update() {
        pos.y -= speed * Core::deltaTime;
        angle.x += 30.0f * 13.0f * DEG2RAD * Core::deltaTime;
        angle.y += 30.0f *  9.0f * DEG2RAD * Core::deltaTime;
	    pos.x += sin(angle.y) * 11.0f * 30.0f * Core::deltaTime;
	    pos.z += cos(angle.x) * 8.0f  * 30.0f * Core::deltaTime;
        updateEntity();
        Sprite::update();
    }
};

#endif