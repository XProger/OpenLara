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
        return (state != baseState) == (getEntity().flags & ENTITY_FLAG_ACTIVE) > 0;
    }

    virtual bool activate(float timer) {
        if (this->timer != 0.0f || !inState()) return false;
        this->timer = timer;

        getEntity().flags ^= ENTITY_FLAG_ACTIVE;
        
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
                entity.flags &= ~ENTITY_FLAG_ACTIVE;
            }
        }

        if (timer < 0.0f) {
            timer += Core::deltaTime;
            if (timer >= 0.0f) {
                timer = 0.0f;
                entity.flags |= ENTITY_FLAG_ACTIVE;
            }
        }

        if (!inState())
            setState(state != baseState ? baseState : (entity.id == TR::Entity::TRAP_BLADE ? 2 : (baseState ^ 1)));

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
                int sparkIndex = level->entityAdd(TR::Entity::SPARK, e.room, (int)p.x, (int)p.y, (int)p.z, e.rotation, -1);
                if (sparkIndex > -1)
                    level->entities[sparkIndex].controller = new SpriteController(level, sparkIndex, true, SpriteController::FRAME_RANDOM);

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

    virtual bool activate(float timer) {
        if (!Trigger::activate(timer))
            return false;
        
        // add dart (bullet)
        TR::Entity &entity = getEntity();

        vec3 pos = vec3(0.0f, -512.0f, 256.0f).rotateY(PI - entity.rotation);
        pos = pos + vec3(entity.x, entity.y, entity.z);

        int dartIndex = level->entityAdd(TR::Entity::TRAP_DART, entity.room, (int)pos.x, (int)pos.y, (int)pos.z, entity.rotation, entity.intensity);
        if (dartIndex > -1)
            level->entities[dartIndex].controller = new Dart(level, dartIndex);

        int smokeIndex = level->entityAdd(TR::Entity::SMOKE, entity.room, (int)pos.x, (int)pos.y, (int)pos.z, entity.rotation, -1);
        if (smokeIndex > -1)
            level->entities[smokeIndex].controller = new SpriteController(level, smokeIndex);

        playSound(151);

        return true;
    }

};

#endif