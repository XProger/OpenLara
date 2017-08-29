#ifndef H_CHARACTER
#define H_CHARACTER

#include "controller.h"
#include "trigger.h"

struct Character : Controller {
    float   health;
    float   tilt;
    quat    rotHead, rotChest;

    enum Stand { 
        STAND_AIR, STAND_GROUND, STAND_SLIDE, STAND_HANG, STAND_UNDERWATER, STAND_ONWATER
    }       stand;
    int     input, lastInput;

    enum Key {  
        LEFT        = 1 << 1, 
        RIGHT       = 1 << 2, 
        FORTH       = 1 << 3, 
        BACK        = 1 << 4, 
        JUMP        = 1 << 5,
        WALK        = 1 << 6,
        ACTION      = 1 << 7,
        WEAPON      = 1 << 8,
        DEATH       = 1 << 9
    };

    vec3    velocity;
    float   angleExt;
    float   speed;
    int     stepHeight;
    int     dropHeight;

    int     zone;
    int     box;

    bool    flying;

    Collision collision;

    Character(IGame *game, int entity, float health) : Controller(game, entity), health(health), tilt(0.0f), stand(STAND_GROUND), lastInput(0), velocity(0.0f), angleExt(0.0f) {
        stepHeight =  256;
        dropHeight = -256;

        animation.initOverrides();
        rotHead  = rotChest = quat(0, 0, 0, 1);

        flying = getEntity().type == TR::Entity::ENEMY_BAT;
        updateZone();
    }

    bool updateZone() {
        int dx, dz;
        TR::Room::Sector &s = level->getSector(getRoomIndex(), int(pos.x), int(pos.z), dx, dz);
        if (s.boxIndex == 0xFFFF)
            return false;
        box  = s.boxIndex;
        zone = getZones()[box];
        return true;
    }

    uint16* getZones() {
        return flying ? level->zones[level->isFlipped].fly : (stepHeight == 256 ? level->zones[level->isFlipped].ground1 : level->zones[level->isFlipped].ground2);
    }

    void rotateY(float delta) {
        angle.y += delta; 
        velocity = velocity.rotateY(-delta);
    }

    void rotateX(float delta) {
        angle.x = clamp(angle.x + delta, -PI * 0.49f, PI * 0.49f);
    }

    virtual void hit(float damage, Controller *enemy = NULL) {
        health = max(0.0f, health - damage);
    }

    virtual void checkRoom() {
        TR::Level::FloorInfo info;
        TR::Entity &e = getEntity();
        level->getFloorInfo(e.room, e.x, e.y, e.z, info);

        if (info.roomNext != TR::NO_ROOM)
            e.room = info.roomNext;        

        if (info.roomBelow != TR::NO_ROOM && e.y > info.roomFloor)
            e.room = info.roomBelow;

        if (info.roomAbove != TR::NO_ROOM && e.y <= info.roomCeiling) {
            if (stand == STAND_UNDERWATER && !level->rooms[info.roomAbove].flags.water) {
                stand = STAND_ONWATER;
                velocity.y = 0;
                pos.y = float(info.roomCeiling);
                updateEntity();
            } else
                if (stand != STAND_ONWATER)
                    e.room = info.roomAbove;
        }
    }

    virtual void  updateVelocity()      {}
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
    virtual int   getInput()            { return health <= 0 ? DEATH : 0; }

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

    virtual void updateTilt(float value, float tiltSpeed, float tiltMax) {
        value = clamp(value, -tiltMax, +tiltMax);
        decrease(value - angle.z, angle.z, tiltSpeed);
    }

    virtual void updateTilt(bool active, float tiltSpeed, float tiltMax) {
    // calculate turning tilt
        if (active && (input & (LEFT | RIGHT)) && (tilt == 0.0f || (tilt < 0.0f && (input & LEFT)) || (tilt > 0.0f && (input & RIGHT)))) {
            if (input & LEFT)  tilt -= tiltSpeed;
            if (input & RIGHT) tilt += tiltSpeed;
            tilt = clamp(tilt, -tiltMax, +tiltMax);
        } else {
            if (tilt > 0.0f) tilt = max(0.0f, tilt - tiltSpeed);
            if (tilt < 0.0f) tilt = min(0.0f, tilt + tiltSpeed);
        }
        angle.z = tilt;
    }

    bool isPressed(Key key) {
        return (input & key) && !(lastInput & key);
    }

    virtual void update() {
        vec3 p = pos;
        lastInput = input;
        input = getInput();
        stand = getStand();
        updateState();
        Controller::update();

        if (getEntity().flags.active) {
            updateVelocity();
            updatePosition();
            if (p != pos) {
                if (updateZone())
                    updateLights();
                else
                    pos = p;
            }
        }
    }

    virtual void cmdJump(const vec3 &vel) {
        velocity.x = sinf(angleExt) * vel.z;
        velocity.y = vel.y;
        velocity.z = cosf(angleExt) * vel.z;
        stand = STAND_AIR;
    }

    virtual void doBubbles() {
        int count = rand() % 3;
        if (!count) return;
        playSound(TR::SND_BUBBLE, pos, Sound::Flags::PAN);
        vec3 head = animation.getJoints(getMatrix(), 14, true) * vec3(0.0f, 0.0f, 50.0f);
        for (int i = 0; i < count; i++) {
            int index = Sprite::add(game, TR::Entity::BUBBLE, getRoomIndex(), int(head.x), int(head.y), int(head.z), Sprite::FRAME_RANDOM, true);
            if (index > -1)
                level->entities[index].controller = new Bubble(game, index);
        }
    }
};

#endif