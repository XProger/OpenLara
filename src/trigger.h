#ifndef H_TRIGGER
#define H_TRIGGER

#include "controller.h"
#include "sprite.h"

struct Trigger : Controller {

    bool  immediate;
    float timer;
    int   baseState;

    Trigger(TR::Level *level, int entity, bool immediate) : Controller(level, entity), immediate(immediate), timer(0.0f) {
        baseState = state;
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

        if (!inState())
            animation.setState(state != baseState ? baseState : (entity.type == TR::Entity::TRAP_BLADE ? 2 : (baseState ^ 1)));

        updateAnimation(true);
        updateEntity();
    }
};

struct Dart : Controller {
    vec3 velocity;
    vec3 dir;
    bool inWall;    // dart starts from wall

    Dart(TR::Level *level, int entity) : Controller(level, entity), inWall(true) {
        dir = vec3(sinf(angle.y), 0, cosf(angle.y));
    }

    virtual void update() {
        TR::Animation *anim = animation;
        velocity = dir * animation.getSpeed();
        pos = pos + velocity * (Core::deltaTime * 30.0f);
        updateEntity();
        TR::Level::FloorInfo info;
        level->getFloorInfo(getRoomIndex(), (int)pos.x, (int)pos.z, info);
        if (pos.y > info.floor || pos.y < info.ceiling || !insideRoom(pos, getRoomIndex())) {
            if (!inWall) {
                TR::Entity &e = getEntity();
                
                vec3 p = pos - dir * 64.0f; // wall offset = 64
                Sprite::add(level, TR::Entity::SPARK, e.room, (int)p.x, (int)p.y, (int)p.z, Sprite::FRAME_RANDOM);

                level->entityRemove(entity);
                delete this;
            }
        } else
            inWall = false;
    }
};

struct Dartgun : Trigger {
    vec3 origin;

    Dartgun(TR::Level *level, int entity) : Trigger(level, entity, true), origin(pos) {}

    virtual bool activate(ActionCommand *cmd) {
        if (!Trigger::activate(cmd))
            return false;
        
        // add dart (bullet)
        TR::Entity &entity = getEntity();

        vec3 p = pos + vec3(0.0f, -512.0f, 256.0f).rotateY(PI - entity.rotation);

        int dartIndex = level->entityAdd(TR::Entity::TRAP_DART, entity.room, (int)p.x, (int)p.y, (int)p.z, entity.rotation, entity.intensity);
        if (dartIndex > -1)
            level->entities[dartIndex].controller = new Dart(level, dartIndex);

        Sprite::add(level, TR::Entity::SMOKE, entity.room, (int)p.x, (int)p.y, (int)p.z);

        playSound(TR::SND_DART, pos, Sound::Flags::PAN);

        return true;
    }

};

struct Boulder : Trigger {

    Boulder(TR::Level *level, int entity) : Trigger(level, entity, true) {}

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

    Block(TR::Level *level, int entity) : Controller(level, entity) {
        updateFloor(true);
    }

    void updateFloor(bool rise) {
        TR::Entity &e = getEntity();
        TR::Level::FloorInfo info;
        level->getFloorInfo(e.room, e.x, e.z, info);
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
        level->getFloorInfo(e.room, e.x + (int)dir.x, e.z + (int)dir.z, info, true);
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

#endif