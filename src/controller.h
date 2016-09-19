#ifndef H_CONTROLLER
#define H_CONTROLLER

#include "format.h"

#define GRAVITY     6.0f
#define NO_OVERLAP  0x7FFFFFFF

#define SND_SECRET  13

struct Controller {
    TR::Level   *level;
    int         entity;

    enum Stand { 
        STAND_AIR, STAND_GROUND, STAND_UNDERWATER, STAND_ONWATER 
    }       stand;
    int     state;
    int     mask;

    enum {  LEFT        = 1 << 1, 
            RIGHT       = 1 << 2, 
            FORTH       = 1 << 3, 
            BACK        = 1 << 4, 
            JUMP        = 1 << 5,
            WALK        = 1 << 6,
            ACTION      = 1 << 7,
            WEAPON      = 1 << 8,
            DEATH       = 1 << 9 };

    float   animTime;
    int     animIndex;
    int     animPrevFrame;

    vec3    pos, velocity;
    vec3    angle;

    float   angleExt;

    int     health; 

    float   turnTime;

    struct Action {
        TR::Action  action;
        int         value;

        Action(TR::Action action, int value) : action(action), value(value) {}
    } nextAction;

    Controller(TR::Level *level, int entity) : level(level), entity(entity), velocity(0.0f), animTime(0.0f), animPrevFrame(0), health(100), turnTime(0.0f), nextAction(TR::Action::NONE, 0) {
        TR::Entity &e = getEntity();
        pos       = vec3((float)e.x, (float)e.y, (float)e.z);
        angle     = vec3(0.0f, e.rotation / 16384.0f * PI * 0.5f, 0.0f);
        stand     = STAND_GROUND;
        animIndex = getModel().animation;
        state     = level->anims[animIndex].state;
    }

    void updateEntity() {
        TR::Entity &e = getEntity();
        e.x = int(pos.x);
        e.y = int(pos.y);
        e.z = int(pos.z);
        e.rotation = int(angle.y / (PI * 0.5f) * 16384.0f);
    }

    bool insideRoom(const vec3 &pos, int room) const {
        TR::Room &r = level->rooms[room];
        vec3 min = vec3(r.info.x, r.info.yTop, r.info.z);
        vec3 max = min + vec3(r.xSectors * 1024, r.info.yBottom - r.info.yTop, r.zSectors * 1024);

        return  pos.x >= min.x && pos.x <= max.x &&
                pos.y >= min.y && pos.y <= max.y &&
                pos.z >= min.z && pos.z <= max.z;
    }

    TR::Entity& getEntity() const {
        return level->entities[entity];
    }

    TR::Model& getModel() const {
        TR::Entity &entity = getEntity();
        for (int i = 0; i < level->modelsCount; i++)
            if (entity.id == level->models[i].id)
                return level->models[i];
        ASSERT(false);
        return level->models[0];
    }

    TR::Room& getRoom() const {
        int index = getRoomIndex();
        ASSERT(index >= 0 && index < level->roomsCount);
        return level->rooms[index];
    }

    virtual int getRoomIndex() const {
        return getEntity().room;
    }

    int setAnimation(int index, int frame = -1) {
        animIndex = index;
        TR::Animation &anim = level->anims[animIndex];
        animTime  = frame == -1 ? 0.0f : ((frame - anim.frameStart) / 30.0f);
        animPrevFrame = -1;
        return state = anim.state;
    }

    bool setState(int state) {
        TR::Animation *anim = &level->anims[animIndex];

        if (state == anim->state)
            return true;

        int fIndex = int(animTime * 30.0f);

        bool exists = false;

        for (int i = 0; i < anim->scCount; i++) {
            TR::AnimState &s = level->states[anim->scOffset + i];
            if (s.state == state) {
                exists = true;
                for (int j = 0; j < s.rangesCount; j++) {
                    TR::AnimRange &range = level->ranges[s.rangesOffset + j];
                    if (anim->frameStart + fIndex >= range.low && anim->frameStart + fIndex <= range.high) {
                        setAnimation(range.nextAnimation, range.nextFrame);
                        break;
                    }
                }
            }
        }

        return exists;
    }

    int getOverlap(int fromX, int fromY, int fromZ, int toX, int toZ, int &delta) const { 
        int dx, dz;
        TR::Room::Sector &s = level->getSector(getEntity().room, fromX, fromZ, dx, dz);

        if (s.boxIndex == 0xFFFF) return NO_OVERLAP;

        TR::Box &b = level->boxes[s.boxIndex];
        if (b.contains(toX, toZ)) {
            delta = 0;
            return b.floor;
        }

        int floor = NO_OVERLAP;
        delta = floor;
       
        TR::Overlap *o = &level->overlaps[b.overlap & 0x7FFF];
        do {
            TR::Box &ob = level->boxes[o->boxIndex];
            if (ob.contains(toX, toZ)) { // get min delta
                int d = abs(ob.floor - fromY);
                if (d < delta) {
                    floor = ob.floor;
                    delta = d;
                }
            }
        } while (!(o++)->end);

        delta = floor - b.floor;
        return floor;
    }

    void playSound(int id) const {
    //    LOG("play sound %d\n", id);

        int16 a = level->soundsMap[id];
        TR::SoundInfo &b = level->soundsInfo[a];
        if (b.chance == 0 || (rand() & 0x7fff) <= b.chance) {
            uint32 c = level->soundOffsets[b.offset + rand() % ((b.flags & 0xFF) >> 2)];
            void *p = &level->soundData[c];
        #ifdef WIN32
            PlaySound((LPSTR)p, NULL, SND_ASYNC | SND_MEMORY);
        #endif
        }
    }

    vec3 getDir() const {
        return vec3(angle.x, angle.y);
    }

    void turnToWall() {
        float fx = pos.x / 1024.0f;
        float fz = pos.z / 1024.0f;
        fx -= (int)fx;
        fz -= (int)fz;

        float k;
        if (fx > 1.0f - fz)
            k = fx < fz ? 0 : 1;
        else
            k = fx < fz ? 3 : 2;

        angle.y = k * PI * 0.5f;  // clamp angle to n*PI/2
    }

    void collide() {
        TR::Entity &entity = getEntity();

        TR::Level::FloorInfo info;
        level->getFloorInfo(entity.room, entity.x, entity.z, info);

        if (info.roomNext != 0xFF)
            entity.room = info.roomNext;

        if (entity.y >= info.floor) {
            if (info.roomBelow == 0xFF) {
                entity.y = info.floor;
                pos.y = entity.y;
                velocity.y = 0.0f;
            } else
                entity.room = info.roomBelow;
        }
         
        int height = getHeight();
        if (entity.y - getHeight() < info.ceiling) {
            if (info.roomAbove == 0xFF) {
                pos.y = entity.y = info.ceiling + height;
                velocity.y = fabsf(velocity.y);
            } else {
                if (stand == STAND_UNDERWATER && !(level->rooms[info.roomAbove].flags & TR::ROOM_FLAG_WATER)) {
                    stand = STAND_ONWATER;
                    velocity.y = 0;
                    pos.y = info.ceiling;
                } else
                    if (stand != STAND_ONWATER && entity.y < info.ceiling)
                        entity.room = info.roomAbove;
            }
        }
    }

    void activateNext() { // activate next entity (for triggers)
        if (nextAction.action == TR::Action::NONE) return; 
        
        Controller *controller = (Controller*)level->entities[nextAction.value].controller;
        nextAction.action = TR::Action::NONE;
        if (controller)
            controller->activate();
    }

    virtual void  activate()           {} 
    virtual void  updateVelocity()     {}
    virtual void  move() {}
    virtual Stand getStand()           { return STAND_AIR; }
    virtual int   getHeight()          { return 0; }
    virtual int   getStateAir()        { return state; }
    virtual int   getStateGround()     { return state; }
    virtual int   getStateUnderwater() { return state; }
    virtual int   getStateOnwater()    { return state; }
    virtual int   getStateDeath()      { return state; }
    virtual int   getStateDefault()    { return state; }
    virtual int   getInputMask()       { return 0; }

    virtual int getState(Stand stand) {
        TR::Animation *anim  = &level->anims[animIndex];

        int state = anim->state;

        if (mask & DEATH)
            state = getStateDeath();        
        else if (stand == STAND_GROUND)
            state = getStateGround();
        else if (stand == STAND_AIR)
            state = getStateAir();
        else if (stand == STAND_UNDERWATER)
            state = getStateUnderwater();
        else
            state = getStateOnwater();            

        // try to set new state
        if (!setState(state))
            setState(getStateDefault());

        return level->anims[animIndex].state;
    }

    virtual void updateBegin() {
        mask  = getInputMask();
        state = getState(stand = getStand());
    }

    virtual void updateEnd() {
        move();
        collide();
        updateEntity();
    }

    virtual void updateState() {}

    virtual void updateAnimation(bool commands) {
        int frameIndex = int((animTime += Core::deltaTime) * 30.0f);
        TR::Animation *anim = &level->anims[animIndex];
        bool endFrame = frameIndex >  anim->frameEnd - anim->frameStart;
 
    // apply animation commands
        if (commands) {
            int16 *ptr = &level->commands[anim->animCommand];

            for (int i = 0; i < anim->acCount; i++) {
                int cmd = *ptr++; 
                switch (cmd) {
                    case TR::ANIM_CMD_MOVE : { // cmd position
                        int16 sx = *ptr++;
                        int16 sy = *ptr++;
                        int16 sz = *ptr++;
                        if (endFrame) {
                            pos = pos + vec3(sx, sy, sz).rotateY(angle.y);
                            updateEntity();
                            LOG("move: %d %d %d\n", (int)sx, (int)sy, (int)sz);
                        }
                        break;
                    }
                    case TR::ANIM_CMD_SPEED : { // cmd jump speed
                        int16 sy = *ptr++;
                        int16 sz = *ptr++;
                        if (endFrame) {
                            LOG("jump: %d %d\n", (int)sy, (int)sz);
                            velocity.x = sinf(angleExt) * sz;
                            velocity.y = sy;
                            velocity.z = cosf(angleExt) * sz;
                            stand = STAND_AIR;
                        }
                        break;
                    }
                    case TR::ANIM_CMD_EMPTY : // empty hands
                        break;
                    case TR::ANIM_CMD_KILL : // kill
                        break;
                    case TR::ANIM_CMD_SOUND : { // play sound
                        int frame = (*ptr++);
                        int id    = (*ptr++) & 0x3FFF;
                        int idx   = frame - anim->frameStart;
                  
                        if (idx > animPrevFrame && idx <= frameIndex) {
                            if (getEntity().id != ENTITY_ENEMY_BAT) // temporary mute the bat
                                playSound(id);
                        }
                        break;
                    }
                    case TR::ANIM_CMD_SPECIAL : // special commands
                        if (frameIndex != animPrevFrame && frameIndex + anim->frameStart == ptr[0]) {
                            switch (ptr[1]) {
                                case TR::ANIM_CMD_SPECIAL_FLIP   : angle.y = angle.y + PI;   break;
                                case TR::ANIM_CMD_SPECIAL_BUBBLE : /* playSound(TR::SND_BUBBLE); */ break;
                                case TR::ANIM_CMD_SPECIAL_CTRL   : LOG("water out ?\n");      break;
                                default : LOG("unknown special cmd %d\n", (int)ptr[1]);
                            }
                        }
                        ptr += 2;
                        break;
                    default :
                        LOG("unknown animation command %d\n", cmd);
                }
            }
        }

        if (endFrame) { // if animation is end - switch to next
            setAnimation(anim->nextAnimation, anim->nextFrame);    
            activateNext();
        } else
            animPrevFrame = frameIndex;
    }
    
    virtual void update() {
        updateBegin();
        updateState();
        updateAnimation(true);        
        updateVelocity();
        updateEnd();
    }
};

#endif