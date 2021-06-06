#ifndef H_CONTROLLER
#define H_CONTROLLER

#include "format.h"
#include "frustum.h"
#include "mesh.h"
#include "animation.h"

#define GRAVITY     6.0f
#define SPRITE_FPS  10.0f

#define MAX_LAYERS  4

#define UNLIMITED_AMMO  10000

struct Controller;

struct ICamera {
    enum Mode {
        MODE_FOLLOW,
        MODE_STATIC,
        MODE_LOOK,
        MODE_COMBAT,
        MODE_CUTSCENE,
        MODE_HEAVY,
    } mode;

    int          cameraIndex;
    vec4         *reflectPlane;
    vec3         angle;
    float        shake;
    bool         firstPerson;
    bool         centerView;
    TR::Location eye, target;

    ICamera() : cameraIndex(0), reflectPlane(NULL), angle(0.0f), shake(0.0f), centerView(false) {}

    virtual void setup(bool calcMatrices) {}
    virtual int  getRoomIndex() const { return TR::NO_ROOM; }
    virtual void doCutscene(const vec3 &pos, float rotation) {}
    virtual Controller* getOwner() { return NULL; }

    void setAngle(float x, float y) {
        angle.x = x * DEG2RAD;
        angle.y = y * DEG2RAD;
    }
};

struct RoomDesc {
    int32 index;
    vec4  portal;

    RoomDesc() {}
    RoomDesc(int32 index, const vec4 &portal) : index(index), portal(portal) {}
};

struct IGame {
    virtual ~IGame() {}
    virtual void         loadLevel(TR::LevelID id) {}
    virtual void         loadNextLevel() {}
    virtual void         saveGame(TR::LevelID id, bool checkpoint, bool updateStats) {}
    virtual void         loadGame(int slot) {}
    virtual void         applySettings(const Core::Settings &settings)  {}

    virtual TR::Level*   getLevel()     { return NULL; }
    virtual MeshBuilder* getMesh()      { return NULL; }
    virtual ICamera*     getCamera(int index = -1)  { return NULL; }
    virtual Controller*  getLara(int index = 0)     { return NULL; }
    virtual Controller*  getLara(const vec3 &pos)   { return NULL; }
    virtual bool         isCutscene()   { return false; }
    virtual uint16       getRandomBox(uint16 zone, uint16 *zones) { return 0; }
    virtual uint16       findPath(int ascend, int descend, bool big, int boxStart, int boxEnd, uint16 *zones, uint16 **boxes) { return 0; }
    virtual void         flipMap(bool water = true) {}
    virtual void setWaterParams(float height) {}
    virtual void waterDrop(const vec3 &pos, float radius, float strength) {}
    virtual void setShader(Core::Pass pass, Shader::Type type, bool underwater = false, bool alphaTest = false) {}
    virtual void setRoomParams(int roomIndex, Shader::Type type, float diffuse, float ambient, float specular, float alpha, bool alphaTest = false) {}
    virtual void setupBinding() {}
    virtual void getVisibleRooms(RoomDesc *roomsList, int &roomsCount, int from, int to, const vec4 &viewPort, bool water, int count = 0) {}
    virtual void renderEnvironment(int roomIndex, const vec3 &pos, Texture **targets, int stride = 0, Core::Pass pass = Core::passAmbient) {}
    virtual void renderModelFull(int modelIndex, bool underwater, Basis *joints) {}
    virtual void renderCompose(int roomIndex) {}
    virtual void renderView(int roomIndex, bool water, bool showUI, int roomsCount = 0, RoomDesc *roomsList = NULL) {}
    virtual void renderGame(bool showUI, bool invBG) {}
    virtual void setEffect(Controller *controller, TR::Effect::Type effect) {}

    virtual vec4 projectPoint(const vec4 &p) { return vec4(0.0f); }
    virtual void checkTrigger(Controller *controller, bool heavy) {}
    virtual void shakeCamera(float value, bool add = false) {}

    virtual Controller* addEntity(TR::Entity::Type type, int room, const vec3 &pos, float angle = 0.0f) { return NULL; }
    virtual void removeEntity(Controller *controller) {}

    virtual void addMuzzleFlash(Controller *owner, int joint, const vec3 &offset, int lightIndex) {}

    virtual void invShow(int playerIndex, int page, int itemIndex = -1) {}
    virtual bool invUse(int playerIndex, TR::Entity::Type type) { return false; }
    virtual void invAdd(TR::Entity::Type type, int count = 1) {}
    virtual int* invCount(TR::Entity::Type type) { return NULL; }
    virtual bool invChooseKey(int playerIndex, TR::Entity::Type hole) { return false; }

    virtual Sound::Sample* playSound(int id, const vec3 &pos = vec3(0.0f), int flags = 0) const { return NULL; }
    virtual void playTrack(uint8 track, bool background = false) {}
    virtual void stopTrack()                                     {}
};

struct Controller {

    static Controller *first;
    Controller  *next;

    IGame       *game;
    TR::Level   *level;
    int         entity;
    
    Animation   animation;
    int         &state;

    vec3    pos;
    vec3    angle;

    int16             roomIndex;
    TR::Entity::Flags flags;

    Basis   *joints;
    int     jointsFrame;

    vec4    ambient[6];
    float   specular;
    float   intensity;

    float   timer;

    TR::Room::Light *targetLight;
    vec3 mainLightPos;
    vec4 mainLightColor;
    bool mainLightFlip;
    bool invertAim;
    bool lockMatrix;

    struct MeshLayer {
        uint32   model;
        uint32   mask;
    } *layers;

    uint32 visibleMask;
    uint32 explodeMask;
    struct ExplodePart {
        Basis basis;
        vec3  velocity;
        float damage;
        int16 roomIndex;
    } *explodeParts;

    vec3 lastPos;
    mat4 matrix;

    float waterLevel, waterDepth;

    Controller(IGame *game, int entity) : next(NULL), game(game), level(game->getLevel()), entity(entity), animation(level, getModel(), level->entities[entity].flags.smooth), state(animation.state), invertAim(false), layers(0), explodeMask(0), explodeParts(0), lastPos(0) {
        const TR::Entity &e = getEntity();
        lockMatrix  = false;
        matrix.identity();

        waterLevel = waterDepth = 0.0f;

        pos         = vec3(float(e.x), float(e.y), float(e.z));
        angle       = vec3(0.0f, e.rotation, 0.0f);
        roomIndex   = e.room;
        flags       = e.flags;
        flags.state = TR::Entity::asNone;

        const TR::Model *m = getModel();
        joints      = m ? new Basis[m->mCount] : NULL;
        jointsFrame = -1;

        specular   = 0.0f;
        intensity  = e.intensity == -1 ? -1.0f : intensityf(e.intensity);
        timer      = 0.0f;
        ambient[0] = ambient[1] = ambient[2] = ambient[3] = ambient[4] = ambient[5] = vec4(intensityf(getRoom().ambient));
        targetLight = NULL;
        mainLightFlip = false;
        updateLights(false);
        visibleMask = 0xFFFFFFFF;

        if (flags.once) {
            flags.invisible = true;
            flags.once      = false;
        }

        if (flags.active == TR::ACTIVE) {
            flags.active  = 0;
            flags.reverse = true;
            activate();
        }

        level->entities[entity].flags = flags;

        if (e.isLara() || e.isActor()) // Lara and cutscene entities is active by default
            activate();
    }

    virtual ~Controller() {
        delete[] joints;
        delete[] layers;
        delete[] explodeParts;
        deactivate(true);
    }

    void updateModel() {
        const TR::Model *model = getModel();

        if (!model || model == animation.model)
            return;
        animation.setModel(model);
        delete[] joints;
        joints = new Basis[model->mCount];
    }

    bool fixRoomIndex() { // TODO: remove this and fix braid
        vec3 p = getPos();
        if (insideRoom(p, roomIndex))
            return true;
        for (int i = 0; i < level->roomsCount; i++)
            if (insideRoom(p, i)) {
                roomIndex = i;
                return true;
            }
        return false;
    }

    void getFloorInfo(int roomIndex, const vec3 &pos, TR::Level::FloorInfo &info) const {
        int x = int(pos.x);
        int y = int(pos.y);
        int z = int(pos.z);

        int dx, dz;
        TR::Room::Sector &s = level->getSector(roomIndex, x, z, dx, dz);

        info.roomFloor    = float(256 * s.floor);
        info.roomCeiling  = float(256 * s.ceiling);
        info.floor        = info.roomFloor;
        info.ceiling      = info.roomCeiling;
        info.slantX       = 0;
        info.slantZ       = 0;
        info.roomNext     = TR::NO_ROOM;
        info.roomBelow    = s.roomBelow;
        info.roomAbove    = s.roomAbove;
        info.floorIndex   = s.floorIndex;
        info.boxIndex     = s.boxIndex;
        info.lava         = false;
        info.climb        = 0;
        info.trigger      = TR::Level::Trigger::ACTIVATE;
        info.trigCmdCount = 0;

        //if (s.floor == TR::NO_FLOOR) 
        //    return;

        TR::Room::Sector *sBelow = &s;
        while (sBelow->roomBelow != TR::NO_ROOM) sBelow = &level->getSector(sBelow->roomBelow, x, z, dx, dz);
        info.floor = float(256 * sBelow->floor);

        parseFloorData(info, sBelow->floorIndex, dx, dz);

        if (info.roomNext == TR::NO_ROOM) {
            TR::Room::Sector *sAbove = &s;
            while (sAbove->roomAbove != TR::NO_ROOM) sAbove = &level->getSector(sAbove->roomAbove, x, z, dx, dz);
            if (sAbove != sBelow) {
                TR::Level::FloorInfo tmpInfo;
                tmpInfo.ceiling = float(256 * sAbove->ceiling);
                parseFloorData(tmpInfo, sAbove->floorIndex, dx, dz);
                info.ceiling = tmpInfo.ceiling;
            }
        } else {
            int tmp = info.roomNext;
            getFloorInfo(tmp, pos, info);
            info.roomNext = tmp;
        }

    // entities collide
        if (info.trigCmdCount) {
            int sx = x / 1024;
            int sz = z / 1024;
            int dx = x % 1024;
            int dz = z % 1024;

            for (int i = 0; i < info.trigCmdCount; i++) {
                TR::FloorData::TriggerCommand cmd = info.trigCmd[i];
                if (cmd.action == TR::Action::CAMERA_SWITCH) {
                    i++;
                    continue;
                }
                if (cmd.action != TR::Action::ACTIVATE) continue;
                    
                TR::Entity &e = level->entities[cmd.args];
                Controller *controller = (Controller*)e.controller;
                if (!controller) continue; // Block UpdateFloor issue while entities initialization

                switch (e.type) {
                    case TR::Entity::TRAP_DOOR_1 :
                    case TR::Entity::TRAP_DOOR_2 : {
                        if (!controller->isCollider()) continue;

                        int dirX, dirZ;
                        e.getAxis(dirX, dirZ);

                        int ex = int(controller->pos.x) / 1024;
                        int ey = int(controller->pos.y);
                        int ez = int(controller->pos.z) / 1024;
                        if ((ex == sx && ez == sz) || (ex + dirX == sx && ez + dirZ == sz)) {
                            if (ey >= y - 128 && controller->pos.y < info.floor) {
                                info.floor  = controller->pos.y;
                                info.slantX = info.slantZ = 0;
                                info.lava   = false;
                            }
                            if (ey < y - 128 && controller->pos.y > info.ceiling)
                                info.ceiling = controller->pos.y + 256;
                        }
                        break;
                    }
                    case TR::Entity::TRAP_FLOOR  : {
                        if (!controller->isCollider()) continue;

                        if (sx != int(controller->pos.x) / 1024 || sz != int(controller->pos.z) / 1024) 
                            break;
                        int ey = int(controller->pos.y) - 512;
                        if (ey >= y - 128 && ey < info.floor) {
                            info.floor  = float(ey);
                            info.slantX = info.slantZ = 0;
                            info.lava   = false;
                        }
                        if (ey < y - 128 && ey > info.ceiling)
                            info.ceiling = float(ey);
                        break;
                    }
                    case TR::Entity::DRAWBRIDGE  : {
                        if (controller->isCollider()) continue; // drawbridge is collidable in inactive state, but it works as floor only when active

                        if (controller->flags.active != TR::ACTIVE) continue;
                        int dirX, dirZ;
                        e.getAxis(dirX, dirZ);
                        int ex = int(controller->pos.x) / 1024;
                        int ez = int(controller->pos.z) / 1024;

                        if ((ex - dirX * 1 == sx && ez - dirZ * 1 == sz) ||
                            (ex - dirX * 2 == sx && ez - dirZ * 2 == sz)) {
                            int ey = int(controller->pos.y);
                            if (ey >= y - 128 && controller->pos.y < info.floor) {
                                info.floor  = controller->pos.y;
                                info.slantX = info.slantZ = 0;
                                info.lava   = false;
                            }
                            if (ey < y - 128 && controller->pos.y > info.ceiling)
                                info.ceiling = controller->pos.y + 256;
                        }
                        break;
                    }
                    case TR::Entity::HAMMER_HANDLE : {
                        if (!controller->isCollider()) continue;

                        int dirX, dirZ;
                        e.getAxis(dirX, dirZ);
                        if (abs(int(controller->pos.x) + dirX * 1024 * 3 - x) < 512 && abs(int(controller->pos.z) + dirZ * 1024 * 3 - z) < 512) {
                            info.floor -= 1024 * 3;
                            info.trigCmdCount = 0;
                        }
                        break;
                    }
                    case TR::Entity::BRIDGE_1    : 
                    case TR::Entity::BRIDGE_2    : 
                    case TR::Entity::BRIDGE_3    : {
                        if (sx != int(controller->pos.x) / 1024 || sz != int(controller->pos.z) / 1024) 
                            break;

                        int s = (e.type == TR::Entity::BRIDGE_2) ? 1 :
                                (e.type == TR::Entity::BRIDGE_3) ? 2 : 0;

                        int ey = int(controller->pos.y), sx = 0, sz = 0; 

                        if (s > 0) {
                            switch (e.rotation.value / 0x4000) { // get slantXZ by direction
                                case 0 : sx =  s; break;
                                case 1 : sz = -s; break;
                                case 2 : sx = -s; break;
                                case 3 : sz =  s; break;
                            }

                            ey -= sx * (sx > 0 ? (dx - 1024) : dx) >> 2;
                            ey -= sz * (sz > 0 ? (dz - 1024) : dz) >> 2;
                        }

                        if (y - 128 <= ey) {
                            info.floor  = float(ey);
                            info.slantX = sx;
                            info.slantZ = sz;
                            info.lava   = false;
                        }
                        if (ey  < y - 128)
                            info.ceiling = float(ey + 64);
                        break;
                    }

                    default : ;
                }
            }
        }
    }

    void parseFloorData(TR::Level::FloorInfo &info, int floorIndex, int dx, int dz) const {
        if (!floorIndex) return;

        TR::FloorData *fd = &level->floors[floorIndex];
        TR::FloorData::Command cmd;

        do {
            cmd = (*fd++).cmd;
                
            switch (cmd.func) {

                case TR::FloorData::PORTAL  :
                    info.roomNext = (*fd++).value;
                    break;

                case TR::FloorData::FLOOR                 :
                case TR::FloorData::FLOOR_NW_SE_SOLID     : 
                case TR::FloorData::FLOOR_NE_SW_SOLID     : 
                case TR::FloorData::FLOOR_NW_SE_PORTAL_SE :
                case TR::FloorData::FLOOR_NW_SE_PORTAL_NW :
                case TR::FloorData::FLOOR_NE_SW_PORTAL_SW :
                case TR::FloorData::FLOOR_NE_SW_PORTAL_NE : {
                    int sx, sz;

                    if (cmd.func == TR::FloorData::FLOOR) {
                        sx = fd->slantX;
                        sz = fd->slantZ;
                    } else {
                        if (cmd.func == TR::FloorData::FLOOR_NW_SE_SOLID     || 
                            cmd.func == TR::FloorData::FLOOR_NW_SE_PORTAL_SE ||
                            cmd.func == TR::FloorData::FLOOR_NW_SE_PORTAL_NW) {
                            if (dx <= 1024 - dz) {
                                info.floor += cmd.triangle.b * 256;
                                sx = fd->a - fd->b;
                                sz = fd->c - fd->b;
                            } else {
                                info.floor += cmd.triangle.a * 256;
                                sx = fd->d - fd->c;
                                sz = fd->d - fd->a;
                            }
                        } else {
                            if (dx <= dz) {
                                info.floor += cmd.triangle.b * 256;
                                sx = fd->d - fd->c;
                                sz = fd->c - fd->b;
                            } else {
                                info.floor += cmd.triangle.a * 256;
                                sx = fd->a - fd->b;
                                sz = fd->d - fd->a;
                            }
                        }
                    }
                    fd++;

                    info.slantX = sx;
                    info.slantZ = sz;
                    info.floor -= sx * (sx > 0 ? (dx - 1023) : dx) >> 2;
                    info.floor -= sz * (sz > 0 ? (dz - 1023) : dz) >> 2;
                    break;
                }

                case TR::FloorData::CEILING                 :
                case TR::FloorData::CEILING_NE_SW_SOLID     :
                case TR::FloorData::CEILING_NW_SE_SOLID     :
                case TR::FloorData::CEILING_NE_SW_PORTAL_SW :
                case TR::FloorData::CEILING_NE_SW_PORTAL_NE :
                case TR::FloorData::CEILING_NW_SE_PORTAL_SE :
                case TR::FloorData::CEILING_NW_SE_PORTAL_NW : { 
                    int sx, sz;

                    if (cmd.func == TR::FloorData::CEILING) {
                        sx = fd->slantX;
                        sz = fd->slantZ;
                    } else {
                        if (cmd.func == TR::FloorData::CEILING_NW_SE_SOLID     || 
                            cmd.func == TR::FloorData::CEILING_NW_SE_PORTAL_SE ||
                            cmd.func == TR::FloorData::CEILING_NW_SE_PORTAL_NW) {
                            if (dx <= 1024 - dz) {
                                info.ceiling += cmd.triangle.b * 256;
                                sx = fd->c - fd->d;
                                sz = fd->b - fd->c;
                            } else {
                                info.ceiling += cmd.triangle.a * 256;
                                sx = fd->b - fd->a;
                                sz = fd->a - fd->d;
                            }
                        } else {
                            if (dx <= dz) {
                                info.ceiling += cmd.triangle.b * 256;
                                sx = fd->b - fd->a;
                                sz = fd->b - fd->c;
                            } else {
                                info.ceiling += cmd.triangle.a * 256;
                                sx = fd->c - fd->d;
                                sz = fd->a - fd->d;
                            }
                        }
                    }
                    fd++;

                    info.ceiling -= sx * (sx < 0 ? (dx - 1023) : dx) >> 2; 
                    info.ceiling += sz * (sz > 0 ? (dz - 1023) : dz) >> 2; 
                    break;
                }

                case TR::FloorData::TRIGGER :  {
                    bool skip = info.trigCmdCount > 0;

                    if (!skip) {
                        info.trigger      = (TR::Level::Trigger::Type)cmd.sub;
                        info.trigCmdCount = 0;
                        info.trigInfo     = (*fd++).triggerInfo;
                    } else
                        fd++;

                    TR::FloorData::TriggerCommand trigCmd;
                    do {
                        trigCmd = (*fd++).triggerCmd; // trigger action
                        if (!skip) {
                            ASSERT(info.trigCmdCount < MAX_TRIGGER_COMMANDS);
                            info.trigCmd[info.trigCmdCount++] = trigCmd;
                        }
                    } while (!trigCmd.end);
                    break;
                }

                case TR::FloorData::LAVA :
                    info.lava = true;
                    break;

                case TR::FloorData::CLIMB :
                    info.climb = cmd.sub; // climb mask
                    break;

                case TR::FloorData::MONKEY : break;
                case TR::FloorData::MINECART_LEFT  : 
                case TR::FloorData::MINECART_RIGHT : break;

                default : LOG("unknown func: %d\n", cmd.func);
            }

        } while (!cmd.end);
    }

    virtual bool getSaveData(SaveEntity &data) {
        const TR::Entity &e = getEntity();
        const TR::Model  *m = getModel();
        if (entity < level->entitiesBaseCount) {
            data.x        = e.x ^ int32(pos.x);
            data.y        = e.y ^ int32(pos.y);
            data.z        = e.z ^ int32(pos.z);
            data.rotation = e.rotation.value ^ TR::angle(normalizeAngle(angle.y)).value;
            data.type     = 0;
            data.room     = e.room ^ roomIndex;
        } else {
            data.x        = int32(pos.x);
            data.y        = int32(pos.y);
            data.z        = int32(pos.z);
            data.rotation = TR::angle(normalizeAngle(angle.y)).value;
            data.type     = int16(e.type);
            data.room     = uint8(roomIndex);
        }

        data.flags      = e.flags.value ^ flags.value;
        data.timer      = timer == 0.0f ? 0 : (timer < 0.0f ? -1 : int16(timer * 30.0f));
    // animation
        data.animIndex  = m ? animation.index : 0;
        data.animFrame  = m ? animation.frameIndex : 0;

        data.extraSize  = 0;

        return true;
    }

    virtual void setSaveData(const SaveEntity &data) {
        const TR::Entity &e = getEntity();
        const TR::Model  *m = getModel();
        if (entity < level->entitiesBaseCount) {
            pos.x       = float(e.x ^ data.x);
            pos.y       = float(e.y ^ data.y);
            pos.z       = float(e.z ^ data.z);
            angle.y     = TR::angle(uint16(e.rotation.value ^ data.rotation));
            roomIndex   = e.room ^ data.room;
        } else {
            pos.x       = float(data.x);
            pos.y       = float(data.y);
            pos.z       = float(data.z);
            angle.y     = TR::angle(uint16(data.rotation));
            flags.value = data.flags;
            roomIndex   = data.room;
        }
        flags.value = e.flags.value ^ data.flags;
        timer       = data.timer == -1 ? -1.0f : (data.timer / 30.0f);
    // animation
        if (m) animation.setAnim(data.animIndex, -data.animFrame);
        updateLights(false);
    }

    bool isActive(bool timing = true) {
        if (flags.active != TR::ACTIVE)
            return flags.reverse;

        if (timer == 0.0f)
            return !flags.reverse;

        if (timer == -1.0f)
            return flags.reverse;

        if (timing) {
            timer = max(0.0f, timer - Core::deltaTime);

            if (timer == 0.0f)
                timer = -1.0f;
        }

        return !flags.reverse;
    }

    virtual bool isCollider() {
        const TR::Entity &e = getEntity();
        return e.isEnemy() ||
               e.isVehicle() ||
               e.isDoor() ||
               e.type == TR::Entity::SCION_HOLDER ||
               e.type == TR::Entity::TRAP_BOULDER ||
               e.type == TR::Entity::MUTANT_EGG_SMALL;
    }

    virtual bool activate() {
        if (flags.state == TR::Entity::asActive || next)
            return false;
        flags.invisible = false;
        flags.state = TR::Entity::asActive;
        next = first;
        first = this;
        return true;
    }

    virtual void deactivate(bool removeFromList = false) {
        flags.state = TR::Entity::asInactive;
        if (removeFromList) {
            flags.state = TR::Entity::asNone;
            Controller *prev = NULL;
            Controller *c = first;
            while (c) {
                if (c == this) {
                    if (prev)
                        prev->next = c->next;
                    else
                        first = c->next;
                    break;
                } else
                    prev = c;
                c = c->next;
            }
            next = NULL;
        }
    }

    static void clearInactive() {
        Controller *prev = NULL;
        Controller *c = first;
        while (c) {
            Controller *next = c->next;
            if (c->flags.state == TR::Entity::asInactive) {
                if (prev)
                    prev->next = c->next;
                else
                    first = c->next;
                c->flags.state = TR::Entity::asNone;
                c->next = NULL;
            } else
                prev = c;
            c = next;
        }
    }

    void initMeshOverrides() {
        if (layers) return;
        layers = new MeshLayer[MAX_LAYERS];
        memset(layers, 0, sizeof(MeshLayer) * MAX_LAYERS);
        layers[0].model = getEntity().modelIndex - 1;
        layers[0].mask  = 0xFFFFFFFF;
    }

    void meshSwap(int layer, int16 model, uint32 mask = 0xFFFFFFFF) {
        if (model < 0) return;

        if (!layers) initMeshOverrides();

        TR::Model &m = level->models[model];
        for (int i = 0; i < m.mCount; i++) {
            if (((1 << i) & mask) && !level->meshOffsets[m.mStart + i] && m.mStart + i > 0)
                mask &= ~(1 << i);
        }

        layers[layer].model = model;
        layers[layer].mask  = mask;
    }

    bool aim(const vec3 &t, int joint, const vec4 &angleRange, quat &rot, quat *rotAbs = NULL) {
        Basis b = animation.getJoints(getMatrix(), joint);
        vec3 delta = (b.inverse() * t).normal();
        if (invertAim)
            delta = -delta;

        float angleY = clampAngle(atan2f(delta.x, delta.z));
        float angleX = clampAngle(asinf(delta.y));

        if (angleX > angleRange.x && angleX <= angleRange.y &&
            angleY > angleRange.z && angleY <= angleRange.w) {

            quat ax(vec3(1, 0, 0), -angleX);
            quat ay(vec3(0, 1, 0), angleY);

            rot = ay * ax;
            if (rotAbs)
                *rotAbs = b.rot * rot;

            return true;
        }

        if (rotAbs)
            *rotAbs = rotYXZ(angle);

        return false;
    }

    bool aim(Controller *target, int joint, const vec4 &angleRange, quat &rot, quat *rotAbs = NULL) {
        if (target) {
            Box box = target->getBoundingBox();
            vec3 t = (box.min + box.max) * 0.5f;
            return aim(t, joint, angleRange, rot, rotAbs);
        }

        if (rotAbs)
            *rotAbs = rotYXZ(angle);

        return false;
    }


    bool insideRoom(const vec3 &pos, int roomIndex) const {
        TR::Room &r = level->rooms[roomIndex];
        vec3 min = vec3((float)r.info.x + 1024, (float)r.info.yTop, (float)r.info.z + 1024);
        vec3 max = min + vec3(float((r.xSectors - 2) * 1024), float(r.info.yBottom - r.info.yTop), float((r.zSectors - 2) * 1024));

        return  pos.x >= min.x && pos.x <= max.x &&
                pos.y >= min.y && pos.y <= max.y &&
                pos.z >= min.z && pos.z <= max.z;
    }

    virtual const TR::Model* getModel() {
        int index = getEntity().modelIndex;
        return index > 0 ? &level->models[index - 1] : NULL;
    }

    const TR::Entity& getEntity() const {
        return level->entities[entity];
    }

    TR::Room& getRoom() {
        int index = getRoomIndex();
        ASSERT(index >= 0 && index < level->roomsCount);
        return level->rooms[index];
    }

    virtual int getRoomIndex() const {
        return roomIndex;
    }

    virtual vec3 getPos() {
        return getEntity().isActor() ? getJoint(0).pos : pos;
    }

    vec3 getDir() const {
        return vec3(angle.x, angle.y);
    }

    static inline void applyGravity(float &speed) {
        speed += (speed < 128.0f ? GRAVITY : 1.0f) * (30.0f * Core::deltaTime);
    }

    bool alignToWall(float offset = 0.0f, int quadrant = -1, int maxDist = 0, int maxWidth = 0) {
        int q  = angleQuadrant(angle.y, 0.25f);
        int ix = int(pos.x);
        int iz = int(pos.z);
        int x  = ix & ~1023;
        int z  = iz & ~1023;

        if (quadrant > -1 && quadrant != q)
            return false;

        if (maxDist) { // check dist
            int dist = 0;
            switch (q) {
                case 0 : dist = z + 1024 - iz; break;
                case 1 : dist = x + 1024 - ix; break;
                case 2 : dist = iz - z;        break;
                case 3 : dist = ix - x;        break;
            }
            if (dist > maxDist)
                return false;
        }

        if (maxWidth) {
            int width = abs( ((q % 2) ? (iz - z) : (ix - x)) - 512);
            if (width > maxWidth)
                return false;
        }

        switch (q) {
            case 0 : pos.z = z + 1024 + offset; break;
            case 1 : pos.x = x + 1024 + offset; break;
            case 2 : pos.z = z - offset;        break;
            case 3 : pos.x = x - offset;        break;
        }

        angle.y = q * (PI * 0.5f);
        return true;
    }

    Box getBoundingBox() {
        return getBoundingBoxLocal() * getMatrix();
    }

    Box getBoundingBoxLocal(bool oriented = false) {
        return animation.getBoundingBox(vec3(0, 0, 0), oriented ? getEntity().rotation.value / 0x4000 : 0);
    }

    int getSpheres(Sphere *spheres) {
        const TR::Model *m = getModel();
        ASSERT(m->mCount <= MAX_JOINTS);

        int jFrame = jointsFrame;
        updateJoints();
        jointsFrame = jFrame;

        int count = 0;
        for (int i = 0; i < m->mCount; i++) {
            TR::Mesh &aMesh = level->meshes[level->meshOffsets[m->mStart + i]];
            if (aMesh.radius <= 0) continue;
            vec3 center = joints[i] * aMesh.center;
            spheres[count++] = Sphere(center, aMesh.radius);
        }
        return count;
    }

    Box getSpheresBox(bool local = false) {
        Sphere spheres[MAX_JOINTS];
        int count = getSpheres(spheres);
        if (count) {

            if (local) {
                mat4 m = getMatrix().inverseOrtho();
                for (int i = 0; i < count; i++)
                    spheres[i].center = m * spheres[i].center;
            }

            Box box(spheres[0].center - vec3(spheres[0].radius), spheres[0].center + vec3(spheres[0].radius));
            for (int i = 1; i < count; i++)
                box += Box(spheres[i].center - vec3(spheres[i].radius), spheres[i].center + vec3(spheres[i].radius));

            return box;
        } else
            return local ? getBoundingBoxLocal() : getBoundingBox();
    }


    int collide(Controller *controller, bool checkBoxes = true) {
        const TR::Model *a = getModel();
        const TR::Model *b = controller->getModel();
        if (!a || !b) 
            return 0;

        if (checkBoxes && !getBoundingBox().intersect(controller->getBoundingBox()))
            return 0;

        ASSERT(a->mCount <= 34);
        ASSERT(b->mCount <= 34);

        Sphere aSpheres[MAX_JOINTS];
        Sphere bSpheres[MAX_JOINTS];

        int aCount = getSpheres(aSpheres);
        int bCount = controller->getSpheres(bSpheres);

        int mask = 0;
        for (int i = 0; i < aCount; i++) 
            for (int j = 0; j < bCount; j++)
                if (bSpheres[j].intersect(aSpheres[i])) {
                    mask |= (1 << i);
                    break;
                }
        return mask;
    }

    bool collide(const Sphere &sphere) {
        return getBoundingBoxLocal().intersect(Sphere(getMatrix().inverseOrtho() * sphere.center, sphere.radius));
    }

    vec3 trace(int fromRoom, const vec3 &from, const vec3 &to, int &room, bool isCamera) { // TODO: use Bresenham
        room = fromRoom;

        vec3 pos = from, dir = to - from;
        int px = (int)pos.x, py = (int)pos.y, pz = (int)pos.z;

        float dist = dir.length();
        dir = dir * (1.0f / dist);

        int lr = -1, lx = -1, lz = -1;
        TR::Level::FloorInfo info;
        while (dist > 1.0f) {
            int sx = px / 1024 * 1024 + 512,
                sz = pz / 1024 * 1024 + 512;

            if (lr != room || lx != sx || lz != sz) {
                getFloorInfo(room, vec3(float(sx), float(py), float(sz)), info);
                if (info.roomNext != TR::NO_ROOM) {
                    room = info.roomNext;
                    getFloorInfo(room, vec3(float(sx), float(py), float(sz)), info);
                }
                lr = room;
                lx = sx;
                lz = sz;
            }

            if (isCamera) {
                if (py > info.floor && info.roomBelow != TR::NO_ROOM)
                    room = info.roomBelow;
                else if (py < info.ceiling && info.roomAbove != TR::NO_ROOM)
                    room = info.roomAbove;
                else if (py > info.floor || py < info.ceiling) {
                    int minX = px / 1024 * 1024;
                    int minZ = pz / 1024 * 1024;
                    int maxX = minX + 1024;
                    int maxZ = minZ + 1024;

                    pos = vec3(float(clamp(px, minX, maxX)), pos.y, float(clamp(pz, minZ, maxZ))) + boxNormal(px, pz) * 256.0f;
                    dir = (pos - from).normal();
                }
            } else {
                if (py > info.floor) {
                    if (info.roomBelow != TR::NO_ROOM) 
                        room = info.roomBelow;
                    else
                        break;
                }

                if (py < info.ceiling) {
                    if (info.roomAbove != TR::NO_ROOM)
                        room = info.roomAbove;
                    else
                        break;
                }
            }

            float d = min(dist, 32.0f);    // STEP = 32
            dist -= d;
            pos = pos + dir * d;

            px = (int)pos.x;
            py = (int)pos.y;
            pz = (int)pos.z;
        }

        return pos;
    }

    int traceX(const TR::Location &from, TR::Location &to) {
        vec3 d = to.pos - from.pos;
        if (fabsf(d.x) < EPS) return 1;

        d.yz() *= 1024 / d.x;

        vec3 p = from.pos;

        p.x = float(int(p.x) / 1024 * 1024);
        if (d.x > 0) p.x += 1023;

        p.yz() += d.yz() * ((p.x - from.pos.x) / 1024);

        float s = float(sign(d.x));
        d.x = 1024;
        d *= s;

        int16 roomIndex = from.room;
        while ((p.x - to.pos.x) * s < 0) {
            if (level->isBlocked(roomIndex, p)) {
                to.pos  = p;
                to.room = roomIndex;
                return -1;
            }

            to.room = roomIndex;
            if (level->isBlocked(roomIndex, vec3(p.x + s, p.y, p.z))) {
                to.pos = p;
                return 0;
            }

            p += d;
        }

        to.room = roomIndex;
        return 1;
    }

    int traceZ(const TR::Location &from, TR::Location &to) {
        vec3 d = to.pos - from.pos;
        if (fabsf(d.z) < EPS) return 1;

        d.xy() *= 1024 / d.z;

        vec3 p = from.pos;
        
        p.z = float(int(p.z) / 1024 * 1024);
        if (d.z > 0) p.z += 1023;

        p.xy() += d.xy() * ((p.z - from.pos.z) / 1024);

        float s = float(sign(d.z));
        d.z = 1024;
        d *= s;

        int16 roomIndex = from.room;
        while ((p.z - to.pos.z) * s < 0) {
            if (level->isBlocked(roomIndex, p)) {
                to.pos  = p;
                to.room = roomIndex;
                return -1;
            }

            to.room = roomIndex;
            if (level->isBlocked(roomIndex, vec3(p.x, p.y, p.z + s))) {
                to.pos = p;
                return 0;
            }

            p += d;
        }

        to.room = roomIndex;
        return 1;
    }

    bool trace(const TR::Location &from, TR::Location &to) {
        int rx, rz;

        if (fabsf(to.pos.x - from.pos.x) > fabsf(to.pos.z - from.pos.z)) {
            rz = traceZ(from, to);
            rx = traceX(from, to);
            if (!rx) return false;
        } else {
            rx = traceX(from, to);
            rz = traceZ(from, to);
            if (!rz) return false;
        }
        TR::Room::Sector *sector = level->getSector(to.room, to.pos);
        return !(!clipHeight(from, to, sector) || rx != 1 || rz != 1);
    }

    bool clipHeight(const TR::Location &from, TR::Location &to, TR::Room::Sector *sector) {
        vec3 dir = to.pos - from.pos;

        float y = level->getFloor(sector, to.pos);
        if (to.pos.y <= y || from.pos.y >= y) {
            y = level->getCeiling(sector, to.pos);
            if (to.pos.y >= y || from.pos.y <= y)
                return true;
        }

        ASSERT(dir.y != 0.0f);

        float d = (y - from.pos.y) / dir.y;
        to.pos.y = y;
        to.pos.x = from.pos.x + dir.x * d;
        to.pos.z = from.pos.z + dir.z * d;
        return false;
    }


    bool checkRange(Controller *target, float range) {
        vec3 d = target->pos - pos;
        return fabsf(d.x) < range && fabsf(d.z) < range && fabsf(d.y) < range;
    }

    void updateRoom() {
        level->getSector(roomIndex, pos);
        level->getWaterInfo(getRoomIndex(), pos, waterLevel, waterDepth);
    }

    virtual void hit(float damage, Controller *enemy = NULL, TR::HitType hitType = TR::HIT_DEFAULT) {}
    virtual void  doCustomCommand               (int curFrame, int prevFrame) {}
    virtual void  applyFlow(TR::Camera &sink)   {}

    virtual void  cmdOffset(const vec3 &offset) {
        pos = pos + offset.rotateY(-angle.y);
        updateRoom();
    }

    virtual void  cmdJump(const vec3 &vel)      {}
    virtual void  cmdEmpty()                    {}

    virtual void  cmdEffect(int fx) { 
        switch (fx) {
            case TR::Effect::MESH_SWAP_1    : 
            case TR::Effect::MESH_SWAP_2    : 
            case TR::Effect::MESH_SWAP_3    : {
                if (!layers) initMeshOverrides();
                uint32 mask = (layers[1].mask == 0xFFFFFFFF) ? 0 : 0xFFFFFFFF;
                meshSwap(1, level->extra.meshSwap[fx - TR::Effect::MESH_SWAP_1], mask);
                break;
            }
            case TR::Effect::INV_ON         : flags.invisible = true;  break;
            case TR::Effect::INV_OFF        : flags.invisible = false; break;
            case TR::Effect::DYN_ON         :
                Core::lightColor[1] = vec4(0.6f, 0.5f, 0.1f, 1.0f / 4096.0f);
                Core::lightPos[1]   = getPos();
                break;
            case TR::Effect::DYN_OFF        :
                Core::lightColor[1] = vec4(0, 0, 0, 1);
                break;
            case TR::Effect::FOOTPRINT      : break; // TODO TR3
            default : ASSERT(false);
        }
    }

    virtual void updateAnimation(bool commands) {
        animation.update();

        if (level->isCutsceneLevel() && getEntity().isActor()) {
            vec3 p = getPos();
            if ((p - lastPos).length2() > 256 * 256) {
                game->waterDrop(p, 96.0, 0.1f);
                lastPos = p;
            }
        }

        TR::Animation *anim = animation;

    // apply animation commands
        if (commands) {
            int16 *ptr = &level->commands[anim->animCommand];

            for (int i = 0; i < anim->acCount; i++) {
                int cmd = *ptr++; 
                switch (cmd) {
                    case TR::ANIM_CMD_OFFSET : ptr += 3;   break;
                    case TR::ANIM_CMD_JUMP   : ptr += 2;   break;      
                    case TR::ANIM_CMD_EMPTY  : cmdEmpty(); break;
                    case TR::ANIM_CMD_KILL   : 
                        if (animation.isEnded)
                            deactivate();
                        break;
                    case TR::ANIM_CMD_SOUND  :
                    case TR::ANIM_CMD_EFFECT : {
                        int frame = (*ptr++) - anim->frameStart;
                        int16 sfx = (*ptr++);
                        int fx    = sfx & 0x3FFF;
                        if (animation.isFrameActive(frame)) {
                            if (cmd == TR::ANIM_CMD_EFFECT) {
                                switch (fx) {
                                    case TR::Effect::ROTATE_180   : angle.x = -angle.x; angle.y = angle.y + PI; break;
                                    case TR::Effect::FLOOR_SHAKE  : game->setEffect(this, TR::Effect::Type(fx)); break;
                                    case TR::Effect::FINISH_LEVEL : game->loadNextLevel(); break;
                                    case TR::Effect::FLIP_MAP     : game->flipMap(); break;
                                    default                       : cmdEffect(fx); break;
                                }
                            } else {
                                if (!(level->version & TR::VER_TR1)) {
                                    if ((sfx & 0x8000) && waterDepth <= 0.0f)
                                        break;
                                    if ((sfx & 0x4000) && waterDepth > 0.0f)
                                        break;
                                }
                                game->playSound(fx, pos, Sound::PAN);
                            }
                        }
                        break;
                    }
                }
            }
        }

        if (animation.frameIndex != animation.framePrev)
            doCustomCommand(animation.frameIndex, animation.framePrev);

        if (animation.isEnded) { // if animation is end - switch to next
            if (animation.offset != 0.0f) cmdOffset(animation.offset);
            if (animation.jump   != 0.0f) cmdJump(animation.jump);
            animation.playNext();
        } else
            animation.framePrev = animation.frameIndex;
    }

    bool checkNear(const vec3 &p, float dist) {
        vec3 d = p - pos;
        if (d.x < -dist || d.x > dist || d.z < -dist || d.z > dist || d.y < -3072.0f || d.y > 3072.0f || (SQR(d.x) + SQR(d.z) > SQR(dist)))
            return false;
        Box box = getBoundingBoxLocal();
        if (d.y < box.min.y || (d.y - 100.0f) > box.max.y)
            return false;
        return true;
    }

    void updateExplosion() {
        if (!explodeMask) return;
        const TR::Model *model = getModel();
        for (int i = 0; i < model->mCount; i++)
            if (explodeMask & (1 << i)) {
                ExplodePart &part = explodeParts[i];
                applyGravity(part.velocity.y);

                quat q = quat(vec3(1, 0, 0), PI * Core::deltaTime) * quat(vec3(0, 0, 1), PI * 2.0f * Core::deltaTime);
                part.basis = Basis(part.basis.rot * q, part.basis.pos + part.velocity * (30.0f * Core::deltaTime));

                vec3 p = part.basis.pos;

                TR::Room::Sector *sector = level->getSector(part.roomIndex, p);
                float ceiling = level->getCeiling(sector, p);
                float floor   = level->getFloor(sector, p);

                bool explode = false;

                if (p.y < ceiling) {
                    p.y = ceiling;
                    part.velocity.y = -part.velocity.y;
                }

                if (p.y >= floor) {
                    p.y = floor;
                    part.damage = 0.0f;
                    explode = true;
                }

                if (part.damage > 0.0f) {
                    Controller *lara = game->getLara();
                    if (lara->checkNear(p, part.damage * 2.0f)) {
                        lara->hit(part.damage);
                        explode = true;
                    }
                }

                if (explode) {
                    explodeMask &= ~(1 << i);
                    game->addEntity(TR::Entity::EXPLOSION, part.roomIndex, p);
                }
            }

        if (!explodeMask) {
            delete[] explodeParts;
            explodeParts = NULL;
        }
    }

    virtual void update() {
        if (getEntity().modelIndex <= 0)
            return;

        if (explodeMask)
            updateExplosion();
        else
            updateAnimation(true);
        updateLights(true);
    }
    
    virtual TR::Room& getLightRoom() {
        return getRoom();
    }

    #define LIGHT_DIST 8192.0f

    void updateLights(bool lerp = true) {
        const TR::Room &room = getLightRoom();

        targetLight = NULL;

        if (getEntity().intensity == -1) {
            int ambient = room.ambient;

            if (room.lightsCount && getModel()) {
                vec3 center = getBoundingBox().center();

                int x = int(center.x);
                int y = int(center.y);
                int z = int(center.z);

                ambient = room.getAmbient(x, y, z, &targetLight);
            }

            intensity = intensityf(ambient);
        }

        if (targetLight == NULL) {
            mainLightPos   = vec3(0);
            mainLightColor = vec4(0, 0, 0, 1);
            return;
        }

        vec3 tpos   = vec3(float(targetLight->x), float(targetLight->y), float(targetLight->z));
        vec4 tcolor = vec4(vec3(targetLight->color.r, targetLight->color.g, targetLight->color.b) * (1.0f / 255.0f), float(targetLight->radius));

        if (mainLightFlip != level->state.flags.flipped) {
            lerp = false;
            mainLightFlip = level->state.flags.flipped;
        }

        if (lerp) {
            float t = Core::deltaTime * 2.0f;
            mainLightPos   = mainLightPos.lerp(tpos, t);
            mainLightColor = mainLightColor.lerp(tcolor, t);
        } else {
            mainLightPos   = tpos;
            mainLightColor = tcolor;
        }

    // fix position and radius
        mainLightColor.w = min(LIGHT_DIST * 1.5f, mainLightColor.w);
        vec3 dir = mainLightPos - pos;
        float dist = dir.length();
        if (dist > LIGHT_DIST) {
            dir *= (LIGHT_DIST / dist);
            mainLightPos = pos + dir;
        }
    }

    mat4 getMatrix() {
        const TR::Entity &e = getEntity();

        if (level->isCutsceneLevel() && (e.isActor() || e.isLara()) && e.type != TR::Entity::CUT_4) 
            return level->cutMatrix;

        if (!lockMatrix) {
            matrix.identity();
            matrix.translate(pos);
            if (angle.y != 0.0f) matrix.rotateY(angle.y - (animation.anims != NULL ? (animation.rot * animation.delta) : 0.0f));
            if (angle.x != 0.0f) matrix.rotateX(angle.x);
            if (angle.z != 0.0f) matrix.rotateZ(angle.z);
        }
        return matrix;
    }

    void explode(int32 mask, float damage) {
        const TR::Model *model = getModel();

        if (!layers) initMeshOverrides();

        mask &= layers[0].mask;
        layers[0].mask &= ~mask;
        
        explodeParts = new ExplodePart[model->mCount];
        explodeMask  = 0;
       
        updateJoints();

        vec3 laraPos = game->getLara(pos)->pos;

        int roomIndex = getRoomIndex();
        for (int i = 0; i < model->mCount; i++) {
            if (!(mask & (1 << i)))
                continue;
            explodeMask |= (1 << i);
            float angle = randf() * PI * 2.0f;
            vec2  speed = vec2(randf(), -randf()) * (getEntity().type == TR::Entity::ENEMY_GIANT_MUTANT ? 256.0f : 128.0f);

            if ((rand() % 4) == 0) {
                angle = (laraPos - joints[i].pos).angleY();
            }

            ExplodePart &part = explodeParts[i];
            part.basis     = joints[i];
            part.basis.w   = 1.0f;
            part.velocity  = vec3(cosf(angle) * speed.x, speed.y, sinf(angle) * speed.x);
            part.damage    = damage;
            part.roomIndex = roomIndex;
        }
    }

    void renderShadow(MeshBuilder *mesh) {
        if (Core::pass != Core::passCompose || level->isCutsceneLevel())
            return;

        Box boxL = getBoundingBoxLocal();
        Box boxA = boxL * getMatrix();

        vec3 center = boxA.center();

        TR::Level::FloorInfo info;
        getFloorInfo(getRoomIndex(), center, info);

        const vec3 size = boxL.size() * (1.0f / 1024.0f);

        vec3 dir   = getDir();
        vec3 up    = info.getNormal();
        vec3 right = dir.cross(up).normal();
        dir = up.cross(right).normal();

        mat4 m;
        m.identity();
        m.dir()    = vec4(dir * size.z, 0.0f);
        m.up()     = vec4(up, 0.0f);
        m.right()  = vec4(right * size.x, 0.0f);
        m.offset() = vec4(center.x, info.floor - 8.0f, center.z, 1.0f);
        Core::mModel = m;

        Basis b;
        b.identity();

        game->setShader(Core::pass, Shader::FLASH, false, false);
        Core::active.shader->setParam(uViewProj, Core::mViewProj * m);
        Core::setBasis(&b, 1);

        float alpha = lerp(0.7f, 0.9f, clamp((info.floor - boxA.max.y) / 1024.0f, 0.0f, 1.0f) );
        float lum   = 1.0f - alpha;
        Core::setMaterial(lum, lum, lum, alpha);

        Core::setDepthWrite(false);
        mesh->renderShadowBlob();
        Core::setDepthWrite(true);
    }

    void updateJoints() {
        if (Core::stats.frame == jointsFrame)
            return;
        animation.getJoints(getMatrix(), -1, true, joints);
        jointsFrame = Core::stats.frame;
    }

    Basis& getJoint(int index) {
        updateJoints();

        ASSERT(getModel() && index < getModel()->mCount);
        if (getModel() && getModel()->mCount && index >= getModel()->mCount)
            return joints[0];

        return joints[index];
    }

    void renderSprite(int frame) {
        TR::Entity::Type type = getEntity().type;
        float fDiffuse = TR::Entity::isPickup(type) ? 1.0f : 0.5f;
        float fAmbient = (intensity < 0.0f ? intensityf(getRoom().ambient) : intensity) * fDiffuse;
        float fAlpha   = (type == TR::Entity::SMOKE || type == TR::Entity::WATER_SPLASH || type == TR::Entity::SPARKLES) ? 0.75f : 1.0f;

        uint8 ambient = clamp(int(fAmbient * 255.0f), 0, 255);
        uint8 alpha   = clamp(int(fAlpha   * 255.0f), 0, 255);
        Color32 color(ambient, ambient, ambient, alpha);

        vec3 p = pos - Core::viewPos.xyz();

        game->getMesh()->addDynSprite(level->spriteSequences[-(getEntity().modelIndex + 1)].sStart + frame, short3(int16(p.x), int16(p.y), int16(p.z)), false, false, color, color);
    }

    virtual void render(Frustum *frustum, MeshBuilder *mesh, Shader::Type type, bool caustics) {
        if (getEntity().modelIndex < 0) {
            renderSprite(0);
            return;
        }
        ASSERT(getEntity().modelIndex > 0);

        const TR::Model *model = getModel();

        mat4 matrix = getMatrix();

        Box box = animation.getBoundingBox(vec3(0, 0, 0), 0);
        if (!explodeMask && frustum && !frustum->isVisible(matrix, box.min, box.max))
            return;

        ASSERT(model);

        flags.rendered = true;

        updateJoints();

        Core::mModel = matrix;

        if (layers) {
            uint32 mask = 0;

            for (int i = MAX_LAYERS - 1; i >= 0; i--) {
                uint32 vmask = (layers[i].mask & ~mask) | (!i ? explodeMask : 0);
                vmask &= visibleMask;
                if (!vmask) continue;
                mask |= layers[i].mask;
            // set meshes visibility
                for (int j = 0; j < model->mCount; j++)
                    joints[j].w = (vmask & (1 << j)) ? 1.0f : 0.0f; // hide invisible parts

                if (explodeMask) {
                    ASSERT(explodeParts);
                    const TR::Model *model = getModel();
                    for (int i = 0; i < model->mCount; i++)
                        if (explodeMask & (1 << i))
                            joints[i] = explodeParts[i].basis;
                }

            // render
                Core::setBasis(joints, model->mCount);
                mesh->renderModel(layers[i].model, caustics);
            }
        } else {
            Core::setBasis(joints, model->mCount);
            mesh->renderModel(model->index, caustics);
        }
    }

    void addRicochet(const vec3 &pos, bool sound) {
        game->addEntity(TR::Entity::RICOCHET, getRoomIndex(), pos);
        if (sound)
            game->playSound(TR::SND_RICOCHET, pos, Sound::PAN);
    }
};

struct DummyController : Controller {

    DummyController(IGame *game, int entity) : Controller(game, entity) {}

    virtual void update() {}
};


Controller *Controller::first = NULL;

#endif
