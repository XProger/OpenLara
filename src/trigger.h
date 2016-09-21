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
            setState(state != baseState ? baseState : (entity.id == ENTITY_TRAP_BLADE ? 2 : (baseState ^ 1)));

        updateAnimation(true);
        updateEntity();
    }
};

struct Dart : Trigger {
    vec3 origin;

    Dart(TR::Level *level, int entity) : Trigger(level, entity, true), origin(pos) {}

    virtual bool activate(float timer) {
        bool res = Trigger::activate(timer);
        if (res)
            playSound(151);
        return res;
    }

    virtual void update() {
        Trigger::update();
        if (state != baseState) {
            pos = origin + vec3(angle.x, angle.y) * (animTime * 4096.0f);
            updateEntity();
        } else {
            pos = origin;
            updateEntity();
        }
    }
};

#endif