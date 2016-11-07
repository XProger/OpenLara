#ifndef H_CONTROLLER
#define H_CONTROLLER

#include "format.h"

#define GRAVITY     6.0f
#define NO_OVERLAP  0x7FFFFFFF

#define SPRITE_FPS  10.0f

struct Controller {
    TR::Level   *level;
    int         entity;

    enum Stand { 
        STAND_AIR, STAND_GROUND, STAND_SLIDE, STAND_HANG, STAND_UNDERWATER, STAND_ONWATER
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

    int     *meshes;
    int     mCount;

    struct ActionCommand {
        TR::Action      action;
        int             value;
        float           timer;
        ActionCommand   *next;

        ActionCommand() {}
        ActionCommand(TR::Action action, int value, float timer, ActionCommand *next = NULL) : action(action), value(value), timer(timer), next(next) {}
    } *actionCommand;

    Controller(TR::Level *level, int entity) : level(level), entity(entity), velocity(0.0f), animTime(0.0f), animPrevFrame(0), health(100), turnTime(0.0f), actionCommand(NULL) {
        TR::Entity &e = getEntity();
        pos       = vec3((float)e.x, (float)e.y, (float)e.z);
        angle     = vec3(0.0f, e.rotation, 0.0f);
        stand     = STAND_GROUND;
        animIndex = e.modelIndex > 0 ? getModel().animation : 0;
        state     = level->anims[animIndex].state;
        TR::Model &model = getModel();
        mCount    = model.mCount;
        meshes    = mCount ? new int[mCount] : NULL;
        for (int i = 0; i < mCount; i++)
            meshes[i] = model.mStart + i;
    }

    virtual ~Controller() {
        delete[] meshes;
    }

    void meshSwap(TR::Model &model, int mask) {
        for (int i = 0; i < model.mCount; i++) {
            int index = model.mStart + i;
            if (((1 << i) & mask) && level->meshOffsets[index])
                meshes[i] = index;
        }
    }

    void updateEntity() {
        TR::Entity &e = getEntity();
        e.x = int(pos.x);
        e.y = int(pos.y);
        e.z = int(pos.z);
        while (angle.y < 0.0f)   angle.y += 2 * PI;
        while (angle.y > 2 * PI) angle.y -= 2 * PI;
        e.rotation = angle.y;
    }

    bool insideRoom(const vec3 &pos, int room) const {
        TR::Room &r = level->rooms[room];
        vec3 min = vec3(r.info.x, r.info.yTop, r.info.z);
        vec3 max = min + vec3(r.xSectors * 1024, r.info.yBottom - r.info.yTop, r.zSectors * 1024);

        return  pos.x >= min.x && pos.x <= max.x &&
                pos.y >= min.y && pos.y <= max.y &&
                pos.z >= min.z && pos.z <= max.z;
    }

    TR::Model& getModel() const {
        return level->models[getEntity().modelIndex - 1];
    }

    TR::Entity& getEntity() const {
        return level->entities[entity];
    }

    TR::Room& getRoom() const {
        int index = getRoomIndex();
        ASSERT(index >= 0 && index < level->roomsCount);
        return level->rooms[index];
    }

    virtual int getRoomIndex() const {
        return getEntity().room;
    }

    int setAnimation(int index, int frame = 0) {
        animIndex = index;
        TR::Animation &anim = level->anims[animIndex];
        animTime  = (frame <= 0 ? -frame : (frame - anim.frameStart)) / 30.0f;
        ASSERT(anim.frameStart <= anim.frameEnd);
        animPrevFrame = int(animTime * 30.0f) - 1;
        return state = anim.state;
    }

    bool canSetState(int state) {
        TR::Animation *anim = &level->anims[animIndex];

        if (state == anim->state)
            return true;

        int fIndex = int(animTime * 30.0f);

        for (int i = 0; i < anim->scCount; i++) {
            TR::AnimState &s = level->states[anim->scOffset + i];
            if (s.state == state)
                for (int j = 0; j < s.rangesCount; j++) {
                    TR::AnimRange &range = level->ranges[s.rangesOffset + j];
                    if (anim->frameStart + fIndex >= range.low && anim->frameStart + fIndex <= range.high)
                        return true;
                }
        }

        return false;
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

    int getOverlap(int fromX, int fromY, int fromZ, int toX, int toZ) const {
        int dx, dz;
        TR::Room::Sector &s = level->getSector(getEntity().room, fromX, fromZ, dx, dz);
        
        if (s.boxIndex == 0xFFFF)
            return NO_OVERLAP;      

        TR::Box &b = level->boxes[s.boxIndex];
        if (b.contains(toX, toZ))
            return 0;

        int floor = NO_OVERLAP;
        int delta = NO_OVERLAP;

        TR::Overlap *o = &level->overlaps[b.overlap & 0x7FFF];
        do {
            TR::Box &ob = level->boxes[o->boxIndex];
            if (ob.contains(toX, toZ)) { // get min delta
                int d = abs(b.floor - ob.floor);
                if (d < delta) {
                    floor = ob.floor;
                    delta = d;
                }
            }
        } while (!(o++)->end);

        if (floor == NO_OVERLAP)
            return NO_OVERLAP;

        return b.floor - floor;
    }

    void playSound(int id, const vec3 &pos, int flags) const {
    //    LOG("play sound %d\n", id);

        int16 a = level->soundsMap[id];
        if (a == -1) return;
        TR::SoundInfo &b = level->soundsInfo[a];
        if (b.chance == 0 || (rand() & 0x7fff) <= b.chance) {
            uint32 c = level->soundOffsets[b.offset + rand() % ((b.flags & 0xFF) >> 2)];
            void *p = &level->soundData[c];
            Sound::play(new Stream(p, 1024 * 1024), pos, (float)b.volume / 0xFFFF, 0.0f, flags);
        }
    }

    vec3 getDir() const {
        return vec3(angle.x, angle.y);
    }

    void alignToWall(float offset = 0.0f) {
        float fx = pos.x / 1024.0f;
        float fz = pos.z / 1024.0f;
        fx -= (int)fx;
        fz -= (int)fz;

        int k;
        if (fx > 1.0f - fz)
            k = fx < fz ? 0 : 1;
        else
            k = fx < fz ? 3 : 2;

        angle.y = k * PI * 0.5f;  // clamp angle to n*PI/2

        if (offset != 0.0f) {
            vec3 dir = getDir() * (512.0f - offset);
            if (k % 2)
                pos.x = int(pos.x / 1024.0f) * 1024.0f + 512.0f + dir.x;
            else
                pos.z = int(pos.z / 1024.0f) * 1024.0f + 512.0f + dir.z;
        }
        updateEntity();
    }

    virtual Box getBoundingBox() {
        TR::Animation *anim  = &level->anims[animIndex];
        TR::Model     &model = getModel();

        float k = animTime * 30.0f / anim->frameRate;
        int fIndex = (int)k;
        int fCount = (anim->frameEnd - anim->frameStart) / anim->frameRate + 1;

        int fSize = sizeof(TR::AnimFrame) + model.mCount * sizeof(uint16) * 2;
        k = k - fIndex;

        int fIndexA = fIndex % fCount, fIndexB = (fIndex + 1) % fCount;
        TR::AnimFrame *fA = (TR::AnimFrame*)&level->frameData[(anim->frameOffset + fIndexA * fSize) >> 1];
        TR::AnimFrame *fB = (TR::AnimFrame*)&level->frameData[(anim->frameOffset + fIndexB * fSize) >> 1];
        Box box(fA->box.min().lerp(fB->box.min(), k), fA->box.max().lerp(fB->box.max(), k));
        box.rotate90(getEntity().rotation.value / 0x4000);
        box.min += pos;
        box.max += pos;
        return box;
    }

    void collide() {
        TR::Entity &entity = getEntity();

        TR::Level::FloorInfo info;
        level->getFloorInfo(entity.room, entity.x, entity.z, info);

        if (info.roomNext != 0xFF)
            entity.room = info.roomNext;

        if (entity.y > info.floor) {
            if (info.roomBelow == 0xFF) {
                if (entity.y > info.floor) {
                    entity.y = info.floor;
                    pos.y = entity.y;
                    velocity.y = 0.0f;
                }
            } else
                entity.room = info.roomBelow;
        }
         
        int height = getHeight();
        if (entity.y - height < info.ceiling) {
            if (info.roomAbove == 0xFF) {
                pos.y = entity.y = info.ceiling + height;
                if (velocity.y < 0.0f)
                    velocity.y = GRAVITY;
            } else {
                if (stand == STAND_UNDERWATER && !level->rooms[info.roomAbove].flags.water) {
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
        if (!actionCommand || !actionCommand->next) {
            actionCommand = NULL;
            return;
        }
        ActionCommand *next = actionCommand->next;

        Controller *controller = NULL;
        switch (next->action) {
            case TR::Action::ACTIVATE        :
                controller = (Controller*)level->entities[next->value].controller;
                break;
            case TR::Action::CAMERA_SWITCH   :
            case TR::Action::CAMERA_TARGET   :
                controller = (Controller*)level->cameraController;
                break;
            case TR::Action::SECRET          :
                if (!level->secrets[next->value]) {
                    level->secrets[next->value] = true;
                    playSound(TR::SND_SECRET, pos, 0);
                }
                actionCommand = next;
                activateNext();
                return;
            case TR::Action::FLOW            :
            case TR::Action::FLIP_MAP        :
            case TR::Action::FLIP_ON         :
            case TR::Action::FLIP_OFF        :
            case TR::Action::END             :
            case TR::Action::SOUNDTRACK      :
            case TR::Action::HARDCODE        :
            case TR::Action::CLEAR           :
            case TR::Action::CAMERA_FLYBY    :
            case TR::Action::CUTSCENE        :
                break;
        }

        if (controller) {
            if (controller->activate(next))
                actionCommand = NULL;
        } else
            actionCommand = NULL;
    }

    virtual bool  activate(ActionCommand *cmd) { actionCommand = cmd; return true; } 
    virtual void  doCustomCommand       (int curFrame, int prevFrame) {}
    virtual void  updateVelocity()      {}
    virtual void  checkRoom()           {}
    virtual void  move()                {}
    virtual Stand getStand()            { return STAND_AIR; }
    virtual int   getHeight()           { return 0; }
    virtual int   getStateAir()         { return state; }
    virtual int   getStateGround()      { return state; }
    virtual int   getStateSlide()       { return state; }
    virtual int   getStateHang()        { return state; }
    virtual int   getStateUnderwater()  { return state; }
    virtual int   getStateOnwater()     { return state; }
    virtual int   getStateDeath()       { return state; }
    virtual int   getStateDefault()     { return state; }
    virtual int   getInputMask()        { return 0; }

    virtual int getState(Stand stand) {
        TR::Animation *anim  = &level->anims[animIndex];

        int state = anim->state;

        if (mask & DEATH)
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
        if (!setState(state))
            setState(getStateDefault());

        return level->anims[animIndex].state;
    }

    virtual void updateBegin() {
        mask  = getInputMask();
        state = getState(stand = getStand());
    }

    virtual void updateEnd() {
        TR::Entity &e = getEntity();
        move();
        updateEntity();
    }

    virtual void updateState() {}

    virtual vec3 getAnimMove() {
        TR::Animation *anim = &level->anims[animIndex];
        int16 *ptr = &level->commands[anim->animCommand];

        for (int i = 0; i < anim->acCount; i++) {
            int cmd = *ptr++; 
            switch (cmd) {
                case TR::ANIM_CMD_MOVE : { // cmd position
                    int16 sx = *ptr++;
                    int16 sy = *ptr++;
                    int16 sz = *ptr++;
                    return vec3((float)sx, (float)sy, (float)sz);
                    break;
                }
                case TR::ANIM_CMD_SPEED  : // cmd jump speed
                case TR::ANIM_CMD_SOUND  : // play sound
                case TR::ANIM_CMD_EFFECT : // special commands
                    ptr += 2;
                    break;
            }
        }
        return vec3(0.0f);
    }

    virtual void updateAnimation(bool commands) {
        int frameIndex = int((animTime += Core::deltaTime) * 30.0f);
        TR::Animation *anim = &level->anims[animIndex];
        bool endFrame = frameIndex > anim->frameEnd - anim->frameStart;
 
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
                            pos = pos + vec3(sx, sy, sz).rotateY(-angle.y);
                            updateEntity();
                            checkRoom();
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
                    case TR::ANIM_CMD_SOUND  : // play sound
                    case TR::ANIM_CMD_EFFECT : { // special commands
                        int frame = (*ptr++);
                        int id    = (*ptr++) & 0x3FFF;
                        int idx   = frame - anim->frameStart;

                        if (idx > animPrevFrame && idx <= frameIndex) {
                            if (cmd == TR::ANIM_CMD_EFFECT) {
                                switch (id) {
                                    case TR::EFFECT_ROTATE_180     : angle.y = angle.y + PI;    break;
                                    case TR::EFFECT_LARA_BUBBLES   : if (rand() % 10 > 6) playSound(TR::SND_BUBBLE, pos, Sound::Flags::PAN); break;
                                    case TR::EFFECT_LARA_HANDSFREE : break;
                                    default : LOG("unknown special cmd %d (anim %d)\n", id, animIndex);
                                }
                            } else
                                playSound(id, pos, Sound::Flags::PAN);
                        }
                        break;
                    }
                }
            }
        }

        doCustomCommand(frameIndex, animPrevFrame);

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


struct SpriteController : Controller {

    enum {
        FRAME_ANIMATED = -1,
        FRAME_RANDOM   = -2,
    };

    int frame;
    bool instant, animated;

    SpriteController(TR::Level *level, int entity, bool instant = true, int frame = FRAME_ANIMATED) : Controller(level, entity), instant(instant), animated(frame == FRAME_ANIMATED) {
        if (frame >= 0) { // specific frame
            this->frame = frame;
        } else if (frame == FRAME_RANDOM) { // random frame
            this->frame = rand() % getSequence().sCount;
        } else if (frame == FRAME_ANIMATED) { // animated
            this->frame = 0;
        }
    }

    TR::SpriteSequence& getSequence() {
        return level->spriteSequences[-(getEntity().modelIndex + 1)];
    }

    void update() {
        bool remove = false;
        animTime += Core::deltaTime;

        if (animated) {
            frame = int(animTime * SPRITE_FPS);
            TR::SpriteSequence &seq = getSequence();
            if (instant && frame >= seq.sCount)
                remove = true;
            else
                frame %= seq.sCount;
        } else
            if (instant && animTime >= (1.0f / SPRITE_FPS))
                remove = true;

        if (remove) {
            level->entityRemove(entity);
            delete this;
        }
    }
};

void addSprite(TR::Level *level, TR::Entity::Type type, int room, int x, int y, int z, int frame = -1) {
    int index = level->entityAdd(type, room, x, y, z, 0, -1);
    if (index > -1) {
        level->entities[index].intensity  = 0x1FFF - level->rooms[room].ambient;
        level->entities[index].controller = new SpriteController(level, index, true, frame);
    }        
}

#endif