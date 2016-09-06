#ifndef H_CONTROLLER
#define H_CONTROLLER

#include "format.h"

#define GRAVITY     7.0f
#define NO_OVERLAP  0x7FFFFFFF

struct Controller {
    TR::Level   *level;
    int         entity;

    float       fTime;
    int         lastFrame;

    vec3        pos, velocity;
    vec3        angle;

    int         health; 

    float       turnTime;

    bool        onGround;
    bool        inWater;

    Controller(TR::Level *level, int entity) : level(level), entity(entity), velocity(0.0f), fTime(0.0f), lastFrame(0), health(100), turnTime(0.0f) {
        TR::Entity &e = getEntity();
        pos   = vec3((float)e.x, (float)e.y, (float)e.z);
        angle = vec3(0.0f, e.rotation / 16384.0f * PI * 0.5f, 0.0f);
        onGround = inWater = false;
    }

    virtual void update() {}

    void updateEntity() {
        TR::Entity &e = getEntity();
        e.x = int(pos.x);
        e.y = int(pos.y);
        e.z = int(pos.z);
        e.rotation = int(angle.y / (PI * 0.5f) * 16384.0f);
    }

    bool insideRoom(const vec3 &pos, int room) {
        TR::Room &r = level->rooms[room];
        vec3 min = vec3(r.info.x, r.info.yTop, r.info.z);
        vec3 max = min + vec3(r.xSectors * 1024, r.info.yBottom - r.info.yTop, r.zSectors * 1024);

        return  pos.x >= min.x && pos.x <= max.x &&
                pos.y >= min.y && pos.y <= max.y &&
                pos.z >= min.z && pos.z <= max.z;
    }

    TR::Entity& getEntity() {
        return level->entities[entity];
    }

    TR::Model& getModel() {
        TR::Entity &entity = getEntity();
        for (int i = 0; i < level->modelsCount; i++)
            if (entity.id == level->models[i].id)
                return level->models[i];
        ASSERT(false);
        return level->models[0];
    }

    TR::Room& getRoom() {
        return level->rooms[getEntity().room];
    }

    TR::Room::Sector& getSector(int x, int z, int &dx, int &dz) {
        TR::Room &room = getRoom();
        
        int sx = x - room.info.x;
        int sz = z - room.info.z;
        dx = sx & 1023; // mod 1024
        dz = sz & 1023;
        sx >>= 10;      // div 1024
        sz >>= 10;

        return room.sectors[sx * room.zSectors + sz];
    }

    TR::Room::Sector& getSector(int &dx, int &dz) {
        TR::Entity &entity = getEntity();
        return getSector(entity.x, entity.z, dx, dz);
    }

    bool changeState(int state) {
        TR::Model     &model = getModel();
        TR::Animation *anim  = &level->anims[model.animation];

        if (state == anim->state)
            return true;

        int fIndex = int(fTime * 30.0f);

        bool exists = false;

        for (int i = 0; i < anim->scCount; i++) {
            TR::AnimState &s = level->states[anim->scOffset + i];
            if (s.state == state) {
                exists = true;
                for (int j = 0; j < s.rangesCount; j++) {
                    TR::AnimRange &range = level->ranges[s.rangesOffset + j];
                    if (anim->frameStart + fIndex >= range.low && anim->frameStart + fIndex <= range.high) {
                        model.animation = range.nextAnimation;
                        fTime = (range.nextFrame - level->anims[model.animation].frameStart) / 30.0f;
                        break;
                    }
                }
            }
        }

        return exists;
    }

    struct FloorInfo {
        int floor, ceiling;
        int roomNext, roomBelow, roomAbove;
    };

    int getOverlap(int fromX, int fromY, int fromZ, int toX, int toZ) { 
        int dx, dz;
        TR::Room::Sector &s = getSector(fromX, fromZ, dx, dz);

        if (s.boxIndex == 0xFFFF) return NO_OVERLAP;

        TR::Box &b = level->boxes[s.boxIndex];
        if (b.contains(toX, toZ))
            return b.floor;

        int floor = NO_OVERLAP;
        int delta = 0x7FFFFFFF;

        TR::Overlap *o = &level->overlaps[b.overlap & 0x7FFF];
        do {
            TR::Box &b = level->boxes[o->boxIndex];
            if (b.contains(toX, toZ)) { // get min delta
                int d = abs(fromY - b.floor);
                if (d < delta) {
                    floor = b.floor;
                    delta = d;
                }
            }
        } while (!(o++)->end);

        return floor;
    }

    FloorInfo getFloorInfo(int x, int z) {
        int dx, dz;
        TR::Room::Sector &s = getSector(x, z, dx, dz);

        FloorInfo info;
        info.floor      = 256 * (int)s.floor;
        info.ceiling    = 256 * (int)s.ceiling;
        info.roomNext   = 255;
        info.roomBelow  = s.roomBelow;
        info.roomAbove  = s.roomAbove;

        if (!s.floorIndex) return info;

        TR::FloorData *fd = &level->floors[s.floorIndex];
        TR::FloorData::Command cmd;

        do {
            cmd = (*fd++).cmd;
                
            switch (cmd.func) {

                case TR::FD_PORTAL  :
                    info.roomNext = (*fd++).data;
                    break;

                case TR::FD_FLOOR   : // floor & ceiling
                case TR::FD_CEILING : { 
                    TR::FloorData::Slant slant = (*fd++).slant;
                    int sx = (int)slant.x;
                    int sz = (int)slant.z;
                    if (cmd.func == TR::FD_FLOOR) {
                        info.floor -= sx * (sx > 0 ? (dx - 1024) : dx) >> 2;
                        info.floor -= sz * (sz > 0 ? (dz - 1024) : dz) >> 2;
                    } else {
                        info.ceiling -= sx * (sx < 0 ? (dx - 1024) : dx) >> 2; 
                        info.ceiling += sz * (sz > 0 ? (dz - 1024) : dz) >> 2; 
                    }
                    break;
                }

                case TR::FD_TRIGGER :  {
                    TR::FloorData::TriggerInfo info = (*fd++).triggerInfo;
                    TR::FloorData::TriggerCommand trigCmd;
                    do {
                        trigCmd = (*fd++).triggerCmd; // trigger action
                        switch (trigCmd.func) {
                            case  0 : break; // activate item
                            case  1 : break; // switch to camera
                            case  2 : break; // camera delay
                            case  3 : break; // flip map
                            case  4 : break; // flip on
                            case  5 : break; // flip off
                            case  6 : break; // look at item
                            case  7 : break; // end level
                            case  8 : break; // play soundtrack
                            case  9 : break; // special hadrdcode trigger
                            case 10 : break; // secret found
                            case 11 : break; // clear bodies
                            case 12 : break; // flyby camera sequence
                            case 13 : break; // play cutscene
                        }
                        // ..
                    } while (!trigCmd.end);                       
                    break;
                }

                default : LOG("unknown func: %d\n", cmd.func);
            }

        } while (!cmd.end);

        return info;
    }

};


#define FAST_TURN_TIME      1.0f

#define TURN_FAST           PI
#define TURN_FAST_BACK      PI * 3.0f / 4.0f
#define TURN_NORMAL         PI / 2.0f
#define TURN_SLOW           PI / 3.0f
#define TURN_TILT           PI / 18.0f
#define TURN_WATER_FAST     PI * 3.0f / 4.0f
#define TURN_WATER_SLOW     PI * 2.0f / 3.0f
#define GLIDE_SPEED         50.0f

struct Lara : Controller {
    int sc;
    bool lState;

    Lara(TR::Level *level, int entity) : Controller(level, entity) {
        pos = vec3(70067, -256, 29104);
        angle = vec3(0.0f, -0.68f, 0.0f);
        getEntity().room = 15;
    }

    virtual void update() {
        TR::Model     &model = getModel();
        TR::Animation *anim  = &level->anims[model.animation];

        fTime += Core::deltaTime;

        float rot = 0.0f;

        enum {  LEFT        = 1 << 1, 
                RIGHT       = 1 << 2, 
                FORTH       = 1 << 3, 
                BACK        = 1 << 4, 
                JUMP        = 1 << 5,
                WALK        = 1 << 6,
                ACTION      = 1 << 7,
                WEAPON      = 1 << 8,
                GROUND      = 1 << 9,
                WATER       = 1 << 10,
                DEATH       = 1 << 11 };

        inWater = (getRoom().flags & TR::ROOM_FLAG_WATER);

        int mask = 0;

        if (Input::down[ikW] || Input::joy.L.y < 0)                             mask |= FORTH;
        if (Input::down[ikS] || Input::joy.L.y > 0)                             mask |= BACK;
        if (Input::down[ikA] || Input::joy.L.x < 0)                             mask |= LEFT;
        if (Input::down[ikD] || Input::joy.L.x > 0)                             mask |= RIGHT;
        if (Input::down[ikSpace] || Input::down[ikJoyX])                        mask |= JUMP;
        if (Input::down[ikShift] || Input::down[ikJoyLT])                       mask |= WALK;
        if (Input::down[ikE] || Input::down[ikMouseL] || Input::down[ikJoyA])   mask |= ACTION;
        if (Input::down[ikQ] || Input::down[ikMouseR] || Input::down[ikJoyY])   mask |= WEAPON;
        if (health <= 0)                                                        mask |= DEATH;
        if (onGround)                                                           mask |= GROUND;
        if (inWater)                                                            mask |= WATER;

        int state = anim->state;

        if ((mask & (GROUND | WATER)) == (GROUND | WATER)) {   // on water surface
            angle.x = 0.0f;
            
            state = TR::STATE_SURF_TREAD;

        } else if (mask & GROUND) {
            angle.x = 0.0f;

            if (state == TR::STATE_COMPRESS) {
                switch (mask & (RIGHT | LEFT | FORTH | BACK)) {
                    case RIGHT  : state = TR::STATE_RIGHT_JUMP;     break;
                    case LEFT   : state = TR::STATE_LEFT_JUMP;      break;
                    case FORTH  : state = TR::STATE_FORWARD_JUMP;   break;
                    case BACK   : state = TR::STATE_BACK_JUMP;      break;
                    default     : state = TR::STATE_UP_JUMP;        break;
                }
            } else
                if (mask & JUMP) {   // jump button is pressed
                    if ((mask & FORTH) && state == TR::STATE_FORWARD_JUMP)
                        state = TR::STATE_RUN;
                    else
                        state = state == TR::STATE_RUN ? TR::STATE_FORWARD_JUMP : TR::STATE_COMPRESS;
                } else
                    if (mask & WALK) {      // walk button is pressed
                        if (mask & FORTH)
                            state = TR::STATE_WALK;
                        else if (mask & BACK)
                            state = TR::STATE_BACK;
                        else if (mask & LEFT)
                            state = TR::STATE_STEP_LEFT;
                        else if (mask & RIGHT)
                            state = TR::STATE_STEP_RIGHT;
                        else
                            state = TR::STATE_STOP;
                    } else {                // only dpad buttons pressed
                        if (mask & FORTH)
                            state = TR::STATE_RUN;
                        else if (mask & BACK)
                            state = TR::STATE_FAST_BACK;
                        else if (mask & LEFT)
                            state = turnTime < FAST_TURN_TIME ? TR::STATE_TURN_LEFT  : TR::STATE_FAST_TURN;
                        else if (mask & RIGHT)
                            state = turnTime < FAST_TURN_TIME ? TR::STATE_TURN_RIGHT : TR::STATE_FAST_TURN;
                        else
                            state = TR::STATE_STOP;
                    }

        } else if (mask & WATER) {  // underwater

            if (state == TR::STATE_FORWARD_JUMP || state == TR::STATE_UP_JUMP || state == TR::STATE_BACK_JUMP || state == TR::STATE_LEFT_JUMP || state == TR::STATE_RIGHT_JUMP || state == TR::STATE_FALL || state == TR::STATE_REACH) {
                model.animation = TR::ANIM_WATER_FALL;
                fTime = 0.0f;
                state = level->anims[model.animation].state;
            } else if (state == TR::STATE_SWAN_DIVE) {
                state = TR::STATE_DIVE;
                angle.x = -PI * 0.5f;
            } else
                if (mask & JUMP)
                    state = TR::STATE_SWIM;
                else
                    state = (state == TR::STATE_SWIM || velocity.y > GLIDE_SPEED) ? TR::STATE_GLIDE : TR::STATE_TREAD;

        } else {    // in the air
            angle.x = 0.0f;

            if (state == TR::STATE_FORWARD_JUMP) {
                if (mask & ACTION)
                    state = TR::STATE_REACH;
                else if ((mask & (FORTH | WALK)) == (FORTH | WALK))
                    state = TR::STATE_SWAN_DIVE;
            } else if (state != TR::STATE_SWAN_DIVE && state != TR::STATE_REACH && state != TR::STATE_FALL && state != TR::STATE_UP_JUMP && state != TR::STATE_BACK_JUMP && state != TR::STATE_LEFT_JUMP && state != TR::STATE_RIGHT_JUMP) {
                model.animation = TR::ANIM_FALL;
                state = level->anims[model.animation].state;
            }
             //   state = TR::STATE_FALL;
       //     LOG("- speed: %f\n", velocity.length());

        }

    // try to set new state
        if (!changeState(state)) {
            int stopState = TR::STATE_FALL;

            if (state == TR::STATE_DIVE)
                stopState = state;
            else if ((mask & (GROUND | WATER)) == (GROUND | WATER))
                stopState = TR::STATE_SURF_TREAD;
            else if (mask & WATER)
                stopState = TR::STATE_TREAD;
            else if (mask & GROUND)
                stopState = TR::STATE_STOP;

            changeState(stopState);
            /*
            if (state == stopState || !changeState(stopState)) {
                int stopAnim = -1;
                switch (stopState) {
                    case TR::STATE_FALL : stopAnim = TR::ANIM_FALL; break;
                }

                if (stopAnim > -1) {
                    model.animation = stopAnim;
                    fTime = 0.0f;
                }
            }*/
        }

        anim  = &level->anims[model.animation]; // get new animation and state (if it has been changed)
        state = anim->state;

        int fCount = anim->frameEnd - anim->frameStart + 1;
        int fIndex = int(fTime * 30.0f);

#ifdef _DEBUG
        // show state transitions for current animation
        if (Input::down[ikEnter]) {
            LOG("state: %d\n", anim->state);
            for (int i = 0; i < anim->scCount; i++) {
                auto &sc = level->states[anim->scOffset + i];
                LOG("-> %d : ", (int)sc.state);
                for (int j = 0; j < sc.rangesCount; j++) {
                    TR::AnimRange &range = level->ranges[sc.rangesOffset + j];
                    LOG("%d ", range.nextAnimation);
                }
                LOG("\n");
            }
        }
#endif

    // calculate turn tilt
        if (state == TR::STATE_RUN && (mask & (GROUND | WATER)) == GROUND && (mask & (LEFT | RIGHT))) {
            if (mask & LEFT)  angle.z -= Core::deltaTime * TURN_TILT;
            if (mask & RIGHT) angle.z += Core::deltaTime * TURN_TILT;
            angle.z = clamp(angle.z, -TURN_TILT, TURN_TILT);
        } else
            angle.z -= angle.z * min(Core::deltaTime * 8.0f, 1.0f);

        if (state == TR::STATE_TURN_LEFT || state == TR::STATE_TURN_RIGHT || state == TR::STATE_FAST_TURN)
            turnTime += Core::deltaTime;
        else
            turnTime = 0.0f;

    // get turning angle
        float w = 0.0f;
       
        if (state == TR::STATE_SWIM || state == TR::STATE_GLIDE)
            w = TURN_WATER_FAST;
        else if (state == TR::STATE_TREAD)
            w = TURN_WATER_SLOW;
        else if (state == TR::STATE_RUN || state == TR::STATE_FAST_TURN)
            w = TURN_FAST;
        else if (state == TR::STATE_FAST_BACK)
            w = TURN_FAST_BACK;
        else if (state == TR::STATE_TURN_LEFT || state == TR::STATE_TURN_RIGHT || state == TR::STATE_WALK)
            w = TURN_NORMAL;   
        else if (state == TR::STATE_FORWARD_JUMP || state == TR::STATE_BACK)
            w = TURN_SLOW;

        if (w != 0.0f) {
            w *= Core::deltaTime;
        // yaw
            if (mask & LEFT)  { angle.y -= w; velocity = velocity.rotateY(+w); }
            if (mask & RIGHT) { angle.y += w; velocity = velocity.rotateY(-w); }
        // pitch (underwater only)
            if ( ((mask & (GROUND | WATER)) == WATER) && (mask & (FORTH | BACK)) ) {
                angle.x += ((mask & FORTH) ? -w : w) * 0.5f;
                angle.x = clamp(angle.x, -PI * 0.5f, PI * 0.5f);
            }
        }

    // get animation direction
        float d = 0.0f;
        switch (state) {
            case TR::STATE_BACK :
            case TR::STATE_BACK_JUMP :
            case TR::STATE_FAST_BACK :
                d = PI;
                break;
            case TR::STATE_STEP_LEFT :
            case TR::STATE_LEFT_JUMP :
                d = -PI * 0.5f;
                break;
            case TR::STATE_STEP_RIGHT :
            case TR::STATE_RIGHT_JUMP :
                d =  PI * 0.5f;
                break;
        }
        d += angle.y;

        bool endFrame = fIndex >= fCount;

    // calculate moving speed
        float dt = Core::deltaTime * 30.0f;

        if (mask & (GROUND | WATER)) {
            
            if ((mask & (GROUND | WATER)) == (GROUND | WATER)) {    // on water

            } else if (mask & WATER) {  // underwater

                if (state == TR::STATE_SWIM) {
                    velocity = vec3(angle.x, angle.y) * 35.0f;
                } else 
                    velocity = velocity - velocity * min(1.0f, Core::deltaTime * 2.0f);

                // TODO: apply flow velocity
            } else {    // on ground
                float speed = anim->speed + anim->accel * (fTime * 30.0f);
 
                velocity.x = sinf(d) * speed;
                velocity.z = cosf(d) * speed;
                velocity.y += GRAVITY * dt;
 
            }
        } else
            velocity.y += GRAVITY * dt;

    // apply animation commands
        int16 *ptr = &level->commands[anim->animCommand];

        for (int i = 0; i < anim->acCount; i++) {
            switch (*ptr++) {
                case 0x01 : { // cmd position
                    int16 sx = *ptr++;
                    int16 sy = *ptr++;
                    int16 sz = *ptr++;
                    LOG("move: %d %d %d\n", (int)sx, (int)sy, (int)sz);
                    break;
                }
                case 0x02 : { // cmd jump speed
                    int16 sy = *ptr++;
                    int16 sz = *ptr++;
                    if (endFrame) {
                        LOG("jump: %d %d\n", (int)sy, (int)sz);
                        velocity.x = sinf(d) * sz;
                        velocity.y = sy;
                        velocity.z = cosf(d) * sz;
                        LOG("speed: %f\n", velocity.length());
                        onGround = false;
                    }
                    break;
                }
                case 0x03 : // empty hands
                    break;
                case 0x04 : // kill
                    break;
                case 0x05 : { // play sound
                    int frame = (*ptr++);
                    int id    = (*ptr++) & 0x3FFF;
                    int idx   = frame - anim->frameStart;
                     
//                    if (fIndex != lastFrame)
//                        LOG("play sound at %d current %d last %d (%d)\n", idx, fIndex, lastFrame, (int)id);

                    if (idx > lastFrame && idx <= fIndex) {
                        int16 a = level->soundsMap[id];
                        TR::SoundInfo &b = level->soundsInfo[a];
                        if (b.chance == 0 || (rand() & 0x7fff) <= b.chance) {
                            uint32 c = level->soundOffsets[b.offset + rand() % ((b.flags & 0xFF) >> 2)];

                            void *p = &level->soundData[c];
                        #ifdef WIN32
//                            LOG("play\n");
                            PlaySound((LPSTR)p, NULL, SND_ASYNC | SND_MEMORY);
                        #endif
                        }
                    }
                    break;
                }
                case 0x06 :
                    if (fIndex != lastFrame && fIndex + anim->frameStart == ptr[0]) {
                        if (ptr[1] == 0) {
                            angle = angle + PI;
                        }
                    }
                    ptr += 2;
                    break;
                default :
                    LOG("unknown animation command\n");
            }
        }

    // check for next animation
        if (endFrame) {
            model.animation = anim->nextAnimation;
            TR::Animation *nextAnim = &level->anims[anim->nextAnimation];
            fIndex = anim->nextFrame - nextAnim->frameStart;
            fTime = fIndex / 30.0f;
        }



        move(velocity * dt);
        collide();

        lastFrame = fIndex;
    }

    void move(const vec3 &offset) {
        vec3 p = pos;
        pos = pos + offset;

        int d = getOverlap((int)p.x, (int)p.y, (int)p.z, (int)pos.x, (int)pos.z); 

        int state = level->anims[getModel().animation].state;
        bool stop = false;

        if ((d == NO_OVERLAP) ||
           ((state == TR::STATE_WALK || state == TR::STATE_BACK || state == TR::STATE_STEP_LEFT || state == TR::STATE_STEP_RIGHT) && (int)p.y - d < -256) ||
           ((int)p.y - d > 256)) {

            pos = p;

            TR::Model    &model = getModel();
            TR::Animation *anim = &level->anims[model.animation];

        // smashes
            if (onGround) { // onGround
                if (d >= 256 * 4 && anim->state == TR::STATE_RUN)
                    model.animation = TR::ANIM_SMASH_RUN_LEFT;  // TODO: RIGHT
                else
                    model.animation = TR::ANIM_STAND;
                velocity.x = velocity.z = 0.0f;
                fTime = 0;
            } else if (inWater) {   // in water
                // do nothing
                //velocity.x = velocity.z = 0.0f;
            } else  { // in the air
                model.animation = TR::ANIM_SMASH_JUMP;
                velocity.x = -velocity.x * 0.5f;
                velocity.z = -velocity.z * 0.5f;
                velocity.y = 0.0f;
                fTime = 0;
            }
                     
        } else {
            TR::Entity &entity = getEntity();
            entity.x = (int)pos.x;
            entity.y = (int)pos.y;
            entity.z = (int)pos.z;
        }
    }

    void collide() {
        TR::Entity &entity = getEntity();

        FloorInfo info = getFloorInfo(entity.x, entity.z);
        
        /*
        float hmin = 0.0f, hmax = -768.0f;
        if (inWater) {
            hmin =  256.0f + 128.0f;
            hmax = -256.0f - 128.0f;
        }
        */

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
            
        if (entity.y <= info.ceiling) {
            if (info.roomAbove == 0xFF) {
                entity.y = info.ceiling;
                pos.y = entity.y;
                velocity.y = -velocity.y;
            } else
                entity.room = info.roomAbove;
        }

       /* 
        if (pos.y + hmin >= floor) {
            if (s.roomBelow == 0xFF) {
                pos.y = floor - hmin;
                velocity.y = 0.0f;
            } else
                entity.room = s.roomBelow;
        }
            
        if (pos.y + hmax <= ceiling) {
            if (s.roomAbove == 0xFF) {
                pos.y = ceiling - hmax;
                velocity.y = -velocity.y;
            } else
                entity.room = s.roomAbove;
        }
        */

        int state = level->anims[getModel().animation].state;

        // TODO: use a brain!
        float extra = 0;
        if (state == TR::STATE_WALK         ||
            state == TR::STATE_RUN          ||
            state == TR::STATE_STOP         ||
            state == TR::STATE_FAST_BACK    ||
            state == TR::STATE_TURN_RIGHT   ||
            state == TR::STATE_TURN_LEFT    ||
            state == TR::STATE_BACK         ||
            state == TR::STATE_FAST_TURN    ||
            state == TR::STATE_STEP_RIGHT   ||
            state == TR::STATE_STEP_LEFT    ||
            state == TR::STATE_ROLL) 
            extra = 256 + 128;

        onGround = (pos.y + extra >= info.floor) && (info.roomBelow == 0xFF) && !(getRoom().flags & TR::ROOM_FLAG_WATER);
    }

};

#endif