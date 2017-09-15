#ifndef H_CONTROLLER
#define H_CONTROLLER

#include "format.h"
#include "frustum.h"
#include "mesh.h"
#include "animation.h"
#include "collision.h"

#define GRAVITY     (6.0f * 30.0f)
#define SPRITE_FPS  10.0f

#define MAX_LAYERS  4

#define UNLIMITED_AMMO  10000

struct Controller;

struct ICamera {
    vec4 *reflectPlane;
    vec3 pos;

    ICamera() : reflectPlane(NULL) {}

    virtual void setup(bool calcMatrices) {}
    virtual int  getRoomIndex() const { return TR::NO_ROOM; }
};

struct IGame {
    virtual ~IGame() {}
    virtual void         loadLevel(TR::LevelID id) {}
    virtual TR::Level*   getLevel()     { return NULL; }
    virtual MeshBuilder* getMesh()      { return NULL; }
    virtual ICamera*     getCamera()    { return NULL; }
    virtual Controller*  getLara()      { return NULL; }
    virtual bool         isCutscene()   { return false; }
    virtual uint16       getRandomBox(uint16 zone, uint16 *zones) { return 0; }
    virtual uint16       findPath(int ascend, int descend, bool big, int boxStart, int boxEnd, uint16 *zones, uint16 **boxes) { return 0; }
    virtual void setClipParams(float clipSign, float clipHeight) {}
    virtual void setWaterParams(float height) {}
    virtual void updateParams() {}
    virtual void waterDrop(const vec3 &pos, float radius, float strength) {}
    virtual void setShader(Core::Pass pass, Shader::Type type, bool underwater = false, bool alphaTest = false) {}
    virtual void setupBinding() {}
    virtual void renderEnvironment(int roomIndex, const vec3 &pos, Texture **targets, int stride = 0) {}
    virtual void renderCompose(int roomIndex) {}
    virtual void renderView(int roomIndex, bool water) {}
    virtual void setEffect(TR::Effect effect, float param) {}

    virtual bool invUse(TR::Entity::Type type) { return false; }
    virtual void invAdd(TR::Entity::Type type, int count = 1) {}
    virtual int* invCount(TR::Entity::Type type) { return NULL; }
    virtual bool invChooseKey(TR::Entity::Type hole) { return false; }

    virtual Sound::Sample* playSound(int id, const vec3 &pos, int flags, int group = -1) const { return NULL; }
    virtual void playTrack(int track, bool restart = false) {}
    virtual void stopTrack()                                {}
};

struct Controller {
    static Controller *first;
    Controller  *next;
    enum ActiveState { asNone, asActive, asInactive } activeState;

    IGame       *game;
    TR::Level   *level;
    int         entity;
    
    Animation   animation;
    int         &state;

    vec3    pos;
    vec3    angle;

    Basis   *joints;
    int     frameIndex;

    vec3    ambient[6];
    float   specular;

    float   timer;

    TR::Room::Light *targetLight;
    vec3 mainLightPos;
    vec4 mainLightColor;

    struct MeshLayer {
        uint32   model;
        uint32   mask;
    } *layers;

    Controller(IGame *game, int entity) : next(NULL), activeState(asNone), game(game), level(game->getLevel()), entity(entity), animation(level, getModel()), state(animation.state), layers(NULL) {
        TR::Entity &e = getEntity();
        pos        = vec3(float(e.x), float(e.y), float(e.z));
        angle      = vec3(0.0f, e.rotation, 0.0f);
        TR::Model  *m = getModel();
        joints     = m ? new Basis[m->mCount] : NULL;
        frameIndex = -1;
        specular   = 0.0f;
        timer      = 0.0f;
        ambient[0] = ambient[1] = ambient[2] = ambient[3] = ambient[4] = ambient[5] = vec3(intensityf(getRoom().ambient));
        targetLight = NULL;
        updateLights(false);

        if (e.flags.once) {
            e.flags.invisible = true;
            e.flags.once      = false;
        }

        if (e.flags.active == TR::ACTIVE) {
            e.flags.active  = 0;
            e.flags.reverse = true;
            activate();
        }

        if (e.isLara() || e.isActor()) // Lara and cutscene entities is active by default
            activate();
    }

    virtual ~Controller() {
        delete[] joints;
        delete[] layers;
        deactivate(true);
    }

    bool isActive() {
        TR::Entity &e = getEntity();

        if (e.flags.active != TR::ACTIVE)
            return e.flags.reverse;

        if (timer == 0.0f)
            return !e.flags.reverse;

        if (timer == -1.0f)
            return e.flags.reverse;

        timer = max(0.0f, timer - Core::deltaTime);

        if (timer == 0.0f)
            timer = -1.0f;

        return !e.flags.reverse;
    }

    virtual bool activate() {
        if (activeState != asNone)
            return false;
        getEntity().flags.invisible = false;
        activeState = asActive;
        next = first;
        first = this;
        return true;
    }

    virtual void deactivate(bool removeFromList = false) {
        activeState = asInactive;
        if (removeFromList) {
            Controller *prev = NULL;
            Controller *c = first;
            while (c) {
                if (c == this) {
                    if (prev)
                        prev->next = c->next;
                    else
                        first = c->next;
                    c->activeState = asNone;
                    break;
                } else
                    prev = c;
                c = c->next;
            }
        }
    }

    static void clearInactive() {
        Controller *prev = NULL;
        Controller *c = first;
        while (c) {
            if (c->activeState == asInactive) {
                if (prev)
                    prev->next = c->next;
                else
                    first = c->next;
                c->activeState = asNone;
            } else
                prev = c;
            c = c->next;
        }
    }

    void initMeshOverrides() {
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

    bool aim(Controller *target, int joint, const vec4 &angleRange, quat &rot, quat *rotAbs = NULL) {
        if (target) {
            Box box = target->getBoundingBox();
            vec3 t = (box.min + box.max) * 0.5f;

            Basis b = animation.getJoints(Basis(getMatrix()), joint);
            vec3 delta = (b.inverse() * t).normal();

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
        }

        if (rotAbs)
            *rotAbs = rotYXZ(angle);
        return false;
    }

    void updateEntity() {
        TR::Entity &e = getEntity();
        e.x = int(pos.x);
        e.y = int(pos.y);
        e.z = int(pos.z);
        angle.y = normalizeAngle(angle.y);
        e.rotation = angle.y;
    }

    bool insideRoom(const vec3 &pos, int roomIndex) const {
        TR::Room &r = level->rooms[roomIndex];
        vec3 min = vec3((float)r.info.x + 1024, (float)r.info.yTop, (float)r.info.z + 1024);
        vec3 max = min + vec3(float((r.xSectors - 1) * 1024), float(r.info.yBottom - r.info.yTop), float((r.zSectors - 1) * 1024));

        return  pos.x >= min.x && pos.x <= max.x &&
                pos.y >= min.y && pos.y <= max.y &&
                pos.z >= min.z && pos.z <= max.z;
    }

    TR::Model* getModel() const {
        int index = getEntity().modelIndex;
        return index > 0 ? &level->models[index - 1] : NULL;
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
        int index = getEntity().room;
        if (level->isFlipped && level->rooms[index].alternateRoom > -1)
            index = level->rooms[index].alternateRoom;
        return index;
    }

    virtual vec3& getPos() {
        return pos;
    }

    Sound::Sample* playSound(int id, const vec3 &pos, int flags) const {
        return game->playSound(id, pos, flags, entity);
    }

    vec3 getDir() const {
        return vec3(angle.x, angle.y);
    }

    bool alignToWall(float offset = 0.0f, int quadrant = -1, int maxDist = 0, int maxWidth = 0) {
        int q  = angleQuadrant(angle.y);
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
        updateEntity();
        return true;
    }

    Box getBoundingBox() {
        return getBoundingBoxLocal() * getMatrix();
    }

    Box getBoundingBoxLocal(bool oriented = false) {
        return animation.getBoundingBox(vec3(0, 0, 0), oriented ? getEntity().rotation.value / 0x4000 : 0);
    }

    void getSpheres(Sphere *spheres, int &count) {
        TR::Model *m = getModel();
        Basis basis(getMatrix());

        count = 0;
        for (int i = 0; i < m->mCount; i++) {
            TR::Mesh &aMesh = level->meshes[level->meshOffsets[m->mStart + i]];
            if (aMesh.radius <= 0) continue;
            vec3 center = animation.getJoints(basis, i, true) * aMesh.center;
            spheres[count++] = Sphere(center, aMesh.radius);
        }
    }

    int collide(Controller *controller, bool checkBoxes = true) {
        TR::Model *a = getModel();
        TR::Model *b = controller->getModel();
        if (!a || !b) 
            return 0;

        if (checkBoxes && !getBoundingBox().intersect(controller->getBoundingBox()))
            return 0;

        ASSERT(a->mCount <= 34);
        ASSERT(b->mCount <= 34);

        Sphere aSpheres[34];
        Sphere bSpheres[34];
        int aCount, bCount;

        getSpheres(aSpheres, aCount);
        controller->getSpheres(bSpheres, bCount);

        int mask = 0;
        for (int i = 0; i < aCount; i++) 
            for (int j = 0; j < bCount; j++)
                if (bSpheres[j].intersect(aSpheres[i])) {
                    mask |= (1 << i);
                    break;
                }
        return mask;
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
                level->getFloorInfo(room, sx, py, sz, info);
                if (info.roomNext != TR::NO_ROOM) {
                    room = info.roomNext;
                    level->getFloorInfo(room, sx, py, sz, info);
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

            px = (int)pos.x, py = (int)pos.y, pz = (int)pos.z;
        }

        return pos;
    }

    bool checkRange(Controller *target, float range) {
        vec3 d = target->pos - pos;
        return fabsf(d.x) < range && fabsf(d.z) < range && fabsf(d.y) < range;
    }

    virtual void hit(float damage, Controller *enemy = NULL, TR::HitType hitType = TR::HIT_DEFAULT) {}

    virtual void  doCustomCommand               (int curFrame, int prevFrame) {}
    virtual void  checkRoom()                   {}
    virtual void  applyFlow(TR::Camera &sink)   {}

    virtual void  cmdOffset(const vec3 &offset) {
        pos = pos + offset.rotateY(-angle.y);
        updateEntity();
        checkRoom();
    }

    virtual void  cmdJump(const vec3 &vel)      {}
    virtual void  cmdEmpty()                    {}
    virtual void  cmdEffect(int fx)             { ASSERT(false); } // not implemented

    virtual void updateAnimation(bool commands) {
        animation.update();
        
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
                        int fx    = (*ptr++) & 0x3FFF;
                        if (animation.isFrameActive(frame)) {
                            if (cmd == TR::ANIM_CMD_EFFECT) {
                                switch (fx) {
                                    case TR::Effect::ROTATE_180  : angle.y = angle.y + PI; break;
                                    case TR::Effect::FLOOR_SHAKE : game->setEffect(TR::Effect(fx), 0.5f * max(0.0f, 1.0f - (pos - ((ICamera*)level->cameraController)->pos).length2() / (15 * 1024 * 15 * 1024) )); break;
                                    case TR::Effect::FLIP_MAP    : level->isFlipped = !level->isFlipped; break;
                                    default                      : cmdEffect(fx); break;
                                }
                            } else
                                playSound(fx, pos, Sound::Flags::PAN);
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
    
    virtual void update() {
        updateAnimation(true);
    }

    void updateLights(bool lerp = true) {
        if (getModel()) {
            TR::Room &room = getRoom();

            vec3 center = getBoundingBox().center();
            float maxAtt = 0.0f;

            for (int i = 0; i < room.lightsCount; i++) {
                TR::Room::Light &light = room.lights[i];
                if (light.intensity > 0x1FFF) continue;

                vec3 dir = vec3(float(light.x), float(light.y), float(light.z)) - center;
                float att = max(0.0f, 1.0f - dir.length2() / float(light.radius) / float(light.radius)) * (1.0f - intensityf(light.intensity));

                if (att > maxAtt) {
                    maxAtt = att;
                    targetLight = &light;
                }
            }
        } else 
            targetLight = NULL;

        if (targetLight == NULL) {
            mainLightPos   = vec3(0);
            mainLightColor = vec4(0, 0, 0, 1);
            return;
        }

        vec3 tpos   = vec3(float(targetLight->x), float(targetLight->y), float(targetLight->z));
        vec4 tcolor = vec4(vec3(1.0f - intensityf(targetLight->intensity)), float(targetLight->radius));

        if (lerp) {
            float t = Core::deltaTime * 2.0f;
            mainLightPos   = mainLightPos.lerp(tpos, t);
            mainLightColor = mainLightColor.lerp(tcolor, t);
        } else {
            mainLightPos   = tpos;
            mainLightColor = tcolor;
        }
    }

    mat4 getMatrix() {
        mat4 matrix;

        TR::Entity &e = getEntity();
        if (!e.isActor()) {
            matrix.identity();
            matrix.translate(pos);
            if (angle.y != 0.0f) matrix.rotateY(angle.y - (animation.flip ? PI * animation.delta : 0.0f));
            if (angle.x != 0.0f) matrix.rotateX(angle.x);
            if (angle.z != 0.0f) matrix.rotateZ(angle.z);
        } else
            matrix = level->cutMatrix;

        return matrix;
    }

    void renderShadow(MeshBuilder *mesh) {
        TR::Entity &entity = getEntity();

        if (Core::pass != Core::passCompose || !entity.castShadow())
            return;

        if (entity.isActor()) // cutscene entities have no blob shadow
            return;

        Box box = animation.getBoundingBox(pos, 0);
        vec3 center = box.center();

        TR::Level::FloorInfo info;
        level->getFloorInfo(entity.room, int(center.x), int(center.y), int(center.z), info);

        const vec3 fpos = vec3(pos.x, info.floor - 16.0f, pos.z);
        const vec3 size = box.size();

        mat4 m = Core::mViewProj;
        m.translate(fpos);
        m.rotateY(angle.y);
        m.translate(vec3(center.x - pos.x, 0.0f, center.z - pos.z));
        m.scale(vec3(size.x, 0.0f, size.z) * (1.0f / 1024.0f));        

        Basis b;
        b.identity();

        game->setShader(Core::pass, Shader::FLASH, false, false);
        Core::active.shader->setParam(uViewProj, m);
        Core::active.shader->setParam(uBasis, b);
        float alpha = lerp(0.7f, 0.90f, clamp((fpos.y - box.max.y) / 1024.0f, 0.0f, 1.0f) );
        Core::active.shader->setParam(uMaterial, vec4(vec3(0.5f * (1.0f - alpha)), alpha));
        Core::active.shader->setParam(uAmbient, vec3(0.0f));

        Core::setBlending(bmMultiply);
        mesh->renderShadowBlob();
        Core::setBlending(bmNone);
    }

    virtual void render(Frustum *frustum, MeshBuilder *mesh, Shader::Type type, bool caustics) { // TODO: animation.calcJoints
        mat4 matrix = getMatrix();

        Box box = animation.getBoundingBox(vec3(0, 0, 0), 0);
        if (frustum && !frustum->isVisible(matrix, box.min, box.max))
            return;

        TR::Entity &entity = getEntity();
        TR::Model  *model  = getModel();
        ASSERT(model);

        entity.flags.rendered = true;

        if (Core::stats.frame != frameIndex)
            animation.getJoints(matrix, -1, true, joints);

        if (layers) {
            int mask = 0;

            for (int i = MAX_LAYERS - 1; i >= 0; i--) {
                int vmask = layers[i].mask & ~mask;
                if (!vmask) continue;
                mask |= layers[i].mask;
            // set meshes visibility
                for (int j = 0; j < model->mCount; j++)
                    joints[j].w = (vmask & (1 << j)) ? 1.0f : -1.0f; // AHAHA

            //    if (entity.type == TR::Entity::LARA && Core::eye != 0)
            //        joints[14].w = -1.0f;
            // render
                Core::active.shader->setParam(uBasis, joints[0], model->mCount);
                mesh->renderModel(layers[i].model);
            }
        } else {
            Core::active.shader->setParam(uBasis, joints[0], model->mCount);
            mesh->renderModel(entity.modelIndex - 1);
        }

        frameIndex = Core::stats.frame;
    }
};

Controller *Controller::first = NULL;

#endif