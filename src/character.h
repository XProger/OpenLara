#ifndef H_CHARACTER
#define H_CHARACTER

#include "controller.h"

struct Character : Controller {
    int     target;
    int     health;
    float   tilt;
    quat    rotHead, rotChest;

    enum Stand { 
        STAND_AIR, STAND_GROUND, STAND_SLIDE, STAND_HANG, STAND_UNDERWATER, STAND_ONWATER
    }       stand;
    int     input;

    enum {  LEFT        = 1 << 1, 
            RIGHT       = 1 << 2, 
            FORTH       = 1 << 3, 
            BACK        = 1 << 4, 
            JUMP        = 1 << 5,
            WALK        = 1 << 6,
            ACTION      = 1 << 7,
            WEAPON      = 1 << 8,
            DEATH       = 1 << 9 };

    vec3    velocity;
    float   angleExt;

    Character(TR::Level *level, int entity, int health) : Controller(level, entity), target(-1), health(100), tilt(0.0f), stand(STAND_GROUND), velocity(0.0f) {
        animation.initOverrides();
        rotHead  = rotChest = quat(0, 0, 0, 1);
    }

    virtual void hit(int damage) {
        health -= damage;
    };

    virtual void checkRoom() {
        TR::Level::FloorInfo info;
        TR::Entity &e = getEntity();
        level->getFloorInfo(e.room, e.x, e.z, info);

        if (info.roomNext != 0xFF)
            e.room = info.roomNext;        

        if (info.roomBelow != 0xFF && e.y > info.floor)
            e.room = info.roomBelow;       

        if (info.roomAbove != 0xFF && e.y <= info.ceiling) {
            if (stand == STAND_UNDERWATER && !level->rooms[info.roomAbove].flags.water) {
                stand = STAND_ONWATER;
                velocity.y = 0;
                pos.y = float(info.ceiling);
                updateEntity();
            } else
                if (stand != STAND_ONWATER)
                    e.room = info.roomAbove;
        }
    }

    virtual void cmdKill() {
        health = 0;
    }

    virtual void  updatePosition()      {}
    virtual Stand getStand()            { return stand; }
    virtual int   getHeight()           { return 0; }
    virtual int   getStateAir()         { return state; }
    virtual int   getStateGround()      { return state; }
    virtual int   getStateSlide()       { return state; }
    virtual int   getStateHang()        { return state; }
    virtual int   getStateUnderwater()  { return state; }
    virtual int   getStateOnwater()     { return state; }
    virtual int   getStateDeath()       { return state; }
    virtual int   getStateDefault()     { return state; }
    virtual int   getInput()            { return 0; }

    virtual void updateState() {
        int state = animation.state;

        if (input & DEATH)
            state = getStateDeath();        
        else if (stand == STAND_GROUND)
            state = getStateGround();
        else if (stand == STAND_SLIDE)
            state = getStateSlide();
        else if (stand == STAND_HANG)
            state = getStateHang();
        else if (stand == STAND_AIR)
            state = getStateAir();
        else if (stand == STAND_UNDERWATER)
            state = getStateUnderwater();
        else
            state = getStateOnwater();            

        // try to set new state
        if (!animation.setState(state))
            animation.setState(getStateDefault());
    }

    virtual void updateTilt(bool active, float tiltSpeed, float tiltMax) {
    // calculate turning tilt
        if (active && (input & (LEFT | RIGHT)) && (tilt == 0.0f || (tilt < 0.0f && (input & LEFT)) || (tilt > 0.0f && (input & RIGHT)))) {
            if (input & LEFT)  tilt -= tiltSpeed * Core::deltaTime;
            if (input & RIGHT) tilt += tiltSpeed * Core::deltaTime;
        } else
            if (fabsf(tilt) > 0.01f) {
                if (tilt > 0.0f)
                    tilt -= min(tilt,  tiltSpeed * 4.0f * Core::deltaTime);
                else
                    tilt -= max(tilt, -tiltSpeed * 4.0f * Core::deltaTime);
            } else
                tilt = 0.0f;
        tilt = clamp(tilt, -tiltMax, tiltMax);
        
        angle.z = tilt;
    }

    virtual void update() {
        input = getInput();
        stand = getStand();
        updateState();
        Controller::update();
        updateVelocity();
        updatePosition();
    }

    virtual void cmdJump(const vec3 &vel) {
        velocity.x = sinf(angleExt) * vel.z;
        velocity.y = vel.y;
        velocity.z = cosf(angleExt) * vel.z;
        stand = STAND_AIR;
    }
};

#endif