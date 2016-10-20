#ifndef H_TRIGGER
#define H_TRIGGER

#include "controller.h"

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
            setState(state != baseState ? baseState : (entity.type == TR::Entity::TRAP_BLADE ? 2 : (baseState ^ 1)));

        updateAnimation(true);
        updateEntity();
    }
};

struct Dart : Controller {

    vec3 dir;
    bool inWall;    // dart starts from wall

    Dart(TR::Level *level, int entity) : Controller(level, entity), inWall(true) {
        dir = vec3(sinf(angle.y), 0, cosf(angle.y));
    }

    virtual void update() {
        velocity = dir * level->anims[animIndex].speed;
        pos = pos + velocity * (Core::deltaTime * 30.0f);
        updateEntity();
        TR::Level::FloorInfo info;
        level->getFloorInfo(getRoomIndex(), (int)pos.x, (int)pos.z, info);
        if (pos.y > info.floor || pos.y < info.ceiling || !insideRoom(pos, getRoomIndex())) {
            if (!inWall) {
                TR::Entity &e = getEntity();
                
                vec3 p = pos - dir * 64.0f; // wall offset = 64
                addSprite(level, TR::Entity::SPARK, e.room, (int)p.x, (int)p.y, (int)p.z, SpriteController::FRAME_RANDOM);

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

        vec3 pos = vec3(0.0f, -512.0f, 256.0f).rotateY(PI - entity.rotation);
        pos = pos + vec3(entity.x, entity.y, entity.z);

        int dartIndex = level->entityAdd(TR::Entity::TRAP_DART, entity.room, (int)pos.x, (int)pos.y, (int)pos.z, entity.rotation, entity.intensity);
        if (dartIndex > -1)
            level->entities[dartIndex].controller = new Dart(level, dartIndex);

        addSprite(level, TR::Entity::SMOKE, entity.room, (int)pos.x, (int)pos.y, (int)pos.z);

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

#endif