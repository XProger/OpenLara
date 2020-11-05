#ifndef H_CHARACTER
#define H_CHARACTER

#include "controller.h"
#include "collision.h"
#include "sprite.h"

struct Character : Controller {
    float   health;
    float   tilt;
    quat    rotHead, rotChest;

    enum Stand { 
        STAND_AIR,
        STAND_GROUND,
        STAND_SLIDE,
        STAND_HANG,
        STAND_UNDERWATER,
        STAND_ONWATER,
        STAND_WADE
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
        LOOK        = 1 << 9,
        DEATH       = 1 << 10
    };

    Controller  *viewTarget;
    int         jointChest;
    int         jointHead;
    vec4        rangeChest;
    vec4        rangeHead;

    vec3    velocity;
    float   angleExt;
    float   speed;
    float   lookAtSpeed;
    int     stepHeight;
    int     dropHeight;

    int     zone;
    int     box;

    bool    burn;
    bool    flying;
    bool    fullChestRotation;

    Collision collision;

    Character(IGame *game, int entity, float health) : Controller(game, entity), health(health), tilt(0.0f), stand(STAND_GROUND), lastInput(0), viewTarget(NULL), jointChest(-1), jointHead(-1), velocity(0.0f), angleExt(0.0f), speed(0.0f) {
        stepHeight =  256;
        dropHeight = -256;

        lookAtSpeed = 8.0f;

        rangeChest = vec4(-0.80f, 0.80f, -0.75f, 0.75f) * PI;
        rangeHead  = vec4(-0.25f, 0.25f, -0.50f, 0.50f) * PI;
        animation.initOverrides();

        rotHead  = rotChest = quat(0, 0, 0, 1);

        burn   = false;
        flying = getEntity().type == TR::Entity::ENEMY_BAT;
        fullChestRotation = false;
        updateZone();
    }

    bool isActiveTarget() {
        return flags.state == TR::Entity::asActive && !flags.invisible && health > 0.0f;
    }

    virtual int getRoomIndex() const {
        int index = Controller::getRoomIndex();
        
        if (level->isCutsceneLevel())
            return index;
        return index;
    }

    virtual TR::Room& getLightRoom() {
        if (stand == STAND_ONWATER) {
            int16 rIndex = getRoomIndex();
            TR::Room::Sector *sector = level->getSector(rIndex, pos);
            if (sector && sector->roomAbove != TR::NO_ROOM)
                return level->rooms[sector->roomAbove];
        }
        return Controller::getLightRoom();
    }

    bool updateZone() {
        if (level->isCutsceneLevel())
            return false;

        int dx, dz;
        TR::Room::Sector &s = level->getSector(getRoomIndex(), int(pos.x), int(pos.z), dx, dz);
        if (s.boxIndex == TR::NO_BOX)
            return false;
        box  = s.boxIndex;
        zone = getZones()[box];
        return true;
    }

    uint16* getZones() {
        TR::Zone &zones = level->zones[level->state.flags.flipped];
        return (flying || stand == STAND_UNDERWATER || stand == STAND_ONWATER) ? zones.fly : (stepHeight == 256 ? zones.ground1 : zones.ground2);
    }

    void rotateY(float delta) {
        angle.y = clampAngle(angle.y + delta);
        velocity = velocity.rotateY(-delta);
    }

    void rotateX(float delta) {
        angle.x = clamp(angle.x + delta, -PI * 0.49f, PI * 0.49f);
    }

    virtual void hit(float damage, Controller *enemy = NULL, TR::HitType hitType = TR::HIT_DEFAULT) {
        if (getEntity().isEnemy() && health > 0.0f && health <= damage)
            saveStats.kills++;
        health = max(0.0f, health - damage);
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
    virtual int   getStateWade()        { return state; }
    virtual int   getStateDeath()       { return state; }
    virtual int   getStateDefault()     { return state; }
    virtual int   getInput()            { return health <= 0 ? DEATH : 0; }
    virtual bool  useHeadAnimation()    { return false; }

    int getNextState() {
        if (input & DEATH) {
            int deathState = getStateDeath();
            if (state == deathState || animation.canSetState(deathState)) {
                if (stand != STAND_AIR)
                    velocity = vec3(0.0f);
                return deathState;
            }
        }

        switch (stand) {
            case STAND_AIR        : return getStateAir();
            case STAND_GROUND     : return getStateGround();
            case STAND_SLIDE      : return getStateSlide();
            case STAND_HANG       : return getStateHang();
            case STAND_UNDERWATER : return getStateUnderwater();
            case STAND_ONWATER    : return getStateOnwater();
            case STAND_WADE       : return getStateWade();
        }
        return animation.state;
    }

    virtual void updateState() {
        if (stand == STAND_UNDERWATER || stand == STAND_ONWATER)
            burn = false;

        int state = getNextState();
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
        updateRoom();

        vec3 p = pos;
        lastInput = input;
        input = getInput();
        stand = getStand();
        updateState();
        Controller::update();

        if (flags.active) {
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

    vec3 getViewPoint() { // TOOD: remove this
        return getJoint(jointChest).pos;
    }

    virtual void lookAt(Controller *target) {
        if (health <= 0.0f)
            target = NULL;

        vec3 t(0.0f);
        if (target) {
            Box box = target->getBoundingBox();
            t = (box.min + box.max) * 0.5f;
        }

        lookAtPos(target ? &t : NULL);
    }


    void lookAtPos(const vec3 *t = NULL) {
        float speed = lookAtSpeed * Core::deltaTime;
        quat rot;

        if (jointChest > -1) {
            if (t && aim(*t, jointChest, rangeChest, rot)) {
                if (fullChestRotation)
                    rotChest = rotChest.slerp(rot, speed);
                else
                    rotChest = rotChest.slerp(quat(0, 0, 0, 1).slerp(rot, 0.5f), speed);
            } else 
                rotChest = rotChest.slerp(quat(0, 0, 0, 1), speed);
            animation.overrides[jointChest] = rotChest * animation.overrides[jointChest];
        }

        if (jointHead > -1) {
            if (t && aim(*t, jointHead, rangeHead, rot))
                rotHead = rotHead.slerp(rot, speed);
            else
                rotHead = rotHead.slerp(quat(0, 0, 0, 1), speed);
            animation.overrides[jointHead] = rotHead * animation.overrides[jointHead];
        }
    }

    void addSparks(uint32 mask) {
        Sphere spheres[MAX_JOINTS];
        int count = getSpheres(spheres);
        for (int i = 0; i < count; i++)
            if (mask & (1 << i)) {
                vec3 sprPos = spheres[i].center + (vec3(randf(), randf(), randf()) * 2.0f - 1.0f) * spheres[i].radius;
                game->addEntity(TR::Entity::SPARKLES, getRoomIndex(), sprPos);
            }
    }

    void addBlood(const vec3 &sprPos, const vec3 &sprVel) {
        Sprite *sprite = (Sprite*)game->addEntity(TR::Entity::BLOOD, getRoomIndex(), sprPos, 0);
        if (sprite)
            sprite->velocity = sprVel;
    }

    void addBlood(float radius, float height, const vec3 &sprVel) {
        vec3 p = pos + vec3((randf() * 2.0f - 1.0f) * radius, -randf() * height, (randf() * 2.0f - 1.0f) * radius);
        addBlood(p, sprVel);
    }

    void addBloodSpikes() {
        float ang = randf() * PI * 2.0f;
        addBlood(64.0f,  512.0f, vec3(sinf(ang), 0.0f, cosf(ang)) * 20.0f);
    }

    void addBloodBlade() {
        float ang = angle.y + (randf() - 0.5f) * 30.0f * DEG2RAD;
        addBlood(64.0f, 744.0f, vec3(sinf(ang), 0.0f, cosf(ang)) * speed);
    }

    void addBloodSlam(Controller *trapSlam) {
        for (int i = 0; i < 6; i++)
            addBloodSpikes();
    }

    void bakeEnvironment(Texture *&environment) {
        Core::beginFrame();

        flags.invisible = true;
        if (!environment) {
            uint32 opt = OPT_CUBEMAP | OPT_TARGET;
            #ifdef USE_CUBEMAP_MIPS
                opt |= OPT_MIPMAPS;
            #endif
            environment = new Texture(256, 256, 1, FMT_RGB16, opt);
        }
        game->renderEnvironment(getRoomIndex(), pos - vec3(0.0f, 384.0f, 0.0f), &environment);
        #ifdef USE_CUBEMAP_MIPS
            environment->generateMipMap();
        #endif
        flags.invisible = false;

        Core::endFrame();
    }
};

#endif