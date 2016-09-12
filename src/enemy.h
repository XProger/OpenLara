#ifndef H_ENEMY
#define H_ENEMY

#include "controller.h"

struct Enemy : Controller {
    Enemy(TR::Level *level, int entity) : Controller(level, entity) {}

    virtual Stand getStand() {
        return STAND_GROUND;
    }
};


struct Wolf : Enemy {

    enum {
        STATE_STOP     = 1,
        STATE_WALK     = 2,
        STATE_RUN      = 3,
        STATE_STALKING = 5,
        STATE_JUMP     = 6,
        STATE_HOWL     = 7,
        STATE_SLEEP    = 8,
        STATE_GROWL    = 9,
        STATE_10       = 10, // WTF?
        STATE_ATTACK   = 12,
    };

    Wolf(TR::Level *level, int entity) : Enemy(level, entity) {}

    virtual int getStateGround() {
        // STATE_SLEEP     -> STATE_STOP
        // STATE_STOP      -> STATE_WALK, STATE_HOWL, STATE_SLEEP, STATE_GROWL
        // STATE_WALK      -> NULL
        // STATE_RUN       -> STaTE_JUMP, STATe_GROWL, STATE_10
        // STATE_STALKING  -> STATE_RUN, STATE_GROWL, STATE_BITING
        // STATE_JUMP      -> STATE_RUN
        // STATE_GROWL     -> STATE_STOP, STATE_RUN, STATE_STALKING, STATE_HOWL, STATE_ATTACK 
        // STATE_BITING    -> NULL

//        if (state == STATE_SLEEP) return STATE_STOP;
//        if (state == STATE_STOP)  return STATE_GROWL;
//        if (state == STATE_GROWL) return STATE_ATTACK;
//        if (state == STATE_RUN)   return STATE_10;
        return state;
    }
};


struct Bear : Enemy {

    enum {
        STATE_STOP   = 1,
    };

    Bear(TR::Level *level, int entity) : Enemy(level, entity) {}

    virtual int getStateGround() {
        return state;
    }
};


struct Bat : Enemy {

    enum {
        STATE_AWAKE  = 1,
        STATE_FLY    = 2,
    };

    Bat(TR::Level *level, int entity) : Enemy(level, entity) {}

    virtual Stand getStand() {
        return STAND_AIR;
    }

    virtual int getStateAir() {
        animTime = 0.0f;
        return STATE_AWAKE;
    }
};

#endif
