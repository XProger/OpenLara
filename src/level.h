#ifndef H_LEVEL
#define H_LEVEL

#include "core.h"
#include "utils.h"
#include "format.h"
#include "controller.h"
#include "camera.h"

#ifdef _DEBUG
    #include "debug.h"
#endif

const char SHADER[] =
    #include "shader.glsl"
;

struct Level {
    enum { shStatic, shCaustics, shSprite, shMAX };

    TR::Level   level;
    Shader      *shaders[shMAX];
    Texture     *atlas;
    MeshBuilder *mesh;

    Controller  *lara;

    float       time;
    Camera      camera;

    Level(Stream &stream) : level{stream}, time(0.0f) {
        mesh = new MeshBuilder(level);
        
        initAtlas();
        initShaders();

        int entity = 0;
        for (int i = 0; i < level.entitiesCount; i++)
            if (level.entities[i].id == ENTITY_LARA) {
                entity = i;
                break;
            }

        lara = new Lara(&level, entity);

        camera.fov          = 75.0f;
        camera.znear        = 0.1f * 2048.0f;
        camera.zfar         = 1000.0f * 2048.0f;
        camera.offset       = vec3(0, 0, 1024);
        camera.deltaPos     = vec3(0.0f, 768.0f, 0.0f);
        camera.deltaAngle   = vec3(0.0f, PI, 0.0f);
        camera.angle        = vec3(0.0f);
    }

    ~Level() {
        for (int i = 0; i < shMAX; i++)
            delete shaders[i];
        delete atlas;
        delete mesh;

        delete lara;
    }

    void initAtlas() {
        if (!level.tilesCount) {
            atlas = NULL;
            return;
        }

        TR::RGBA *data = new TR::RGBA[1024 * 1024];
        for (int i = 0; i < level.tilesCount; i++) {
            int tx = (i % 4) * 256;
            int ty = (i / 4) * 256;

            TR::RGBA *ptr = &data[ty * 1024 + tx];
            for (int y = 0; y < 256; y++) {
                for (int x = 0; x < 256; x++) {
                    int index = level.tiles[i].index[y * 256 + x];
                    auto p = level.palette[index];
                    ptr[x].r = p.r;
                    ptr[x].g = p.g;
                    ptr[x].b = p.b;
                    ptr[x].a = index == 0 ? 0 : 255;
                }
                ptr += 1024;
            }
        }

        for (int y = 1020; y < 1024; y++)
            for (int x = 1020; x < 1024; x++) {
                int i = y * 1024 + x;
                data[i].r = data[i].g = data[i].b = data[i].a = 255;    // white texel for colored triangles
            }

        atlas = new Texture(1024, 1024, 0, data);
        delete[] data;
    }

    void initShaders() {
        char def[255], ext[255];
        sprintf(def, "#define MAX_RANGES %d\n#define MAX_OFFSETS %d\n", mesh->animTexRangesCount, mesh->animTexOffsetsCount);
        shaders[shStatic]   = new Shader(SHADER, def);
        sprintf(ext, "%s#define CAUSTICS\n", def);
        shaders[shCaustics] = new Shader(SHADER, ext);
        sprintf(ext, "%s#define SPRITE\n", def);
        shaders[shSprite]   = new Shader(SHADER, ext);
    }


    TR::StaticMesh* getMeshByID(int id) {
        for (int i = 0; i < level.staticMeshesCount; i++)
            if (level.staticMeshes[i].id == id)
                return &level.staticMeshes[i];
        return NULL;
    }

    Shader *setRoomShader(const TR::Room &room, float intensity) {
        if (room.flags & TR::ROOM_FLAG_WATER) {
            Core::color = vec4(0.6f * intensity, 0.9f * intensity, 0.9f * intensity, 1.0f);
            return shaders[shCaustics];
        } else {
            Core::color = vec4(intensity, intensity, intensity, 1.0f);
            return shaders[shStatic];
        }
    }

    void renderRoom(int index) {
        ASSERT(index >= 0 && index < level.roomsCount);

        TR::Room &room = level.rooms[index];

        if (room.flags & TR::ROOM_FLAG_VISIBLE) return; // already rendered
        room.flags |= TR::ROOM_FLAG_VISIBLE;

        vec3 offset = vec3(room.info.x, 0.0f, room.info.z);

        mat4 m = Core::mModel;
        Core::mModel.translate(offset);
        Core::ambient       = vec3(1.0f);
        Core::lightColor    = vec4(0.0f, 0.0f, 0.0f, 1.0f);

        Shader *sh = setRoomShader(room, 1.0f);

        sh->bind();
        sh->setParam(uModel, Core::mModel);
        sh->setParam(uColor, Core::color);
        sh->setParam(uAmbient, Core::ambient);
        sh->setParam(uLightColor, Core::lightColor);

    // render room geometry
        mesh->renderRoomGeometry(index);

    // render room sprites
        if (mesh->hasRoomSprites(index)) {
            sh = shaders[shSprite];
            sh->bind();
            sh->setParam(uModel, Core::mModel);
            sh->setParam(uColor, Core::color);
            mesh->renderRoomSprites(index);
        }

        Core::mModel = m;

        // meshes
        for (int i = 0; i < room.meshesCount; i++) {
            TR::Room::Mesh &rMesh = room.meshes[i];
            TR::StaticMesh *sMesh = getMeshByID(rMesh.meshID);
            ASSERT(sMesh != NULL);

            mat4 m = Core::mModel;
            Core::mModel.translate(vec3((float)rMesh.x, (float)rMesh.y, (float)rMesh.z));
            Core::mModel.rotateY(rMesh.rotation / 16384.0f * PI * 0.5f);

            // TODO: check visibility for sMesh.vBox

            getLight(vec3(rMesh.x, rMesh.y, rMesh.z), index);

            renderMesh(sMesh->mesh);

            Core::mModel = m;
        }
        
        Camera::Frustum *camFrustum = camera.frustum;   // push camera frustum
        Camera::Frustum frustum = *camFrustum;
        camera.frustum = &frustum;

        for (int i = 0; i < room.portalsCount; i++) {
            TR::Room::Portal &p = room.portals[i];

            vec3 v[4] = {
                offset + p.vertices[0],
                offset + p.vertices[1],
                offset + p.vertices[2],
                offset + p.vertices[3],
            };

            if (frustum.clipByPortal(v, p.normal)) {
                renderRoom(p.roomIndex);
                frustum = *camFrustum;
            }
        }
        camera.frustum = camFrustum;    // pop camera frustum
    }

    void renderMesh(uint32 meshOffset) {
        if (!level.meshOffsets[meshOffset] && meshOffset)
            return;

        for (int i = 0; i < mesh->mCount; i++)
            if (mesh->meshInfo[i].offset == level.meshOffsets[meshOffset]) {
                MeshBuilder::MeshInfo &m = mesh->meshInfo[i];

                if (camera.frustum->isVisible(Core::mModel * m.center, m.radius)) {
                    Core::active.shader->setParam(uModel, Core::mModel);
                    mesh->renderMesh(i);
                }
                break;
            }
    }

    vec3 getAngle(TR::AnimFrame *frame, int index) {
        #define ANGLE_SCALE (2.0f * PI / 1024.0f)

        uint16 b = frame->angles[index * 2 + 0];
        uint16 a = frame->angles[index * 2 + 1];

        return vec3((a & 0x3FF0) >> 4, ( ((a & 0x000F) << 6) | ((b & 0xFC00) >> 10)), b & 0x03FF) * ANGLE_SCALE;
    }

    float lerpAngle(float a, float b, float t) {
        float d = b - a;
        if (d >= PI)
            a += PI * 2.0f;
        else
            if (d <= -PI)
                a -= PI * 2.0f;
        return a + (b - a) * t;
    }

    quat lerpAngle(const vec3 &a, const vec3 &b, float t) {
    //  return vec3(lerpAngle(a.x, b.x, t),
    //              lerpAngle(a.y, b.y, t),
    //              lerpAngle(a.z, b.z, t));

        mat4 ma, mb;
        ma.identity();
        mb.identity();

        ma.rotateY(a.y);
        ma.rotateX(a.x);
        ma.rotateZ(a.z);

        mb.rotateY(b.y);
        mb.rotateX(b.x);
        mb.rotateZ(b.z);

        return ma.getRot().slerp(mb.getRot(), t).normal();
    }

    void renderModel(const TR::Model &model, vec3 angle) {
        TR::Animation *anim = &level.anims[model.animation];

        float fTime = time;

        if (model.id == ENTITY_LARA) {
            fTime = lara->fTime;
            angle = lara->angle;
        }

        if (angle.y != 0.0f) Core::mModel.rotateY(angle.y);
        if (angle.x != 0.0f) Core::mModel.rotateX(angle.x);
        if (angle.z != 0.0f) Core::mModel.rotateZ(angle.z);

        float k = fTime * 30.0f / anim->frameRate;
        int fIndex = (int)k;
        int fCount = (anim->frameEnd - anim->frameStart) / anim->frameRate + 1;

        int fSize = sizeof(TR::AnimFrame) + model.mCount * sizeof(uint16) * 2;
        k = k - fIndex;


        int fIndexA = fIndex % fCount, fIndexB = (fIndex + 1) % fCount;
        TR::AnimFrame *frameA = (TR::AnimFrame*)&level.frameData[(anim->frameOffset + fIndexA * fSize) >> 1];

        TR::Animation *nextAnim = NULL;

        if (fIndexB == 0) {
            nextAnim = &level.anims[anim->nextAnimation];
            fIndexB = (anim->nextFrame - nextAnim->frameStart) / anim->frameRate;
        } else
            nextAnim = anim;

//      LOG("%d %f\n", fIndexA, fTime);


        TR::AnimFrame *frameB = (TR::AnimFrame*)&level.frameData[(nextAnim->frameOffset + fIndexB * fSize) >> 1];



//      ASSERT(fpSize == fSize);
//      fSize = fpSize;

    //  LOG("%d\n", fIndex % fCount);
        //if (fCount > 1) LOG("%d %d\n", model->id, fCount);
    //  LOG("%d\n", fIndex % fCount);


//      Debug::Draw::box(Box(vec3(frameA->minX, frameA->minY, frameA->minZ), vec3(frameA->maxX, frameA->maxY, frameA->maxZ)));

        TR::Node *node = (int)model.node < level.nodesDataSize ? (TR::Node*)&level.nodesData[model.node] : NULL;

        int sIndex = 0;
        mat4 stack[20];

        mat4 m;
        m.identity();
        m.translate(vec3(frameA->x, frameA->y, frameA->z).lerp(vec3(frameB->x, frameB->y, frameB->z), k));

        for (int i = 0; i < model.mCount; i++) {

            if (i > 0 && node) {
                TR::Node &t = node[i - 1];

                if (t.flags & 0x01) m = stack[--sIndex];
                if (t.flags & 0x02) stack[sIndex++] = m;

                ASSERT(sIndex >= 0);
                ASSERT(sIndex < 20);

                m.translate(vec3(t.x, t.y, t.z));
            }

            quat q = lerpAngle(getAngle(frameA, i), getAngle(frameB, i), k);
            m = m * mat4(q, vec3(0.0f));


        //  vec3 angle = lerpAngle(getAngle(frameA, i), getAngle(frameB, i), k);
        //  m.rotateY(angle.y);
        //  m.rotateX(angle.x);
        //  m.rotateZ(angle.z);

            mat4 tmp = Core::mModel;
            Core::mModel = Core::mModel * m;
            renderMesh(model.mStart + i);
            Core::mModel = tmp;
        }
    }

    int getLightIndex(const vec3 &pos, int &room) {
        int idx = -1;
        float dist;
      //  for (int j = 0; j < level.roomsCount; j++)
        int j = room;
            for (int i = 0; i < level.rooms[j].lightsCount; i++) {
                TR::Room::Light &light = level.rooms[j].lights[i];
                float d = (pos - vec3(light.x, light.y, light.z)).length2();
                if (idx == -1 || d < dist) {
                    idx = i;
                    dist = d;
                //    room = j;
                }
            }
        return idx;
    }

    void getLight(const vec3 &pos, int roomIndex) {
        int room = roomIndex;
        int idx = getLightIndex(pos, room);
        if (idx > -1) {
            TR::Room::Light &light = level.rooms[room].lights[idx];
            float c = level.rooms[room].lights[idx].intensity / 8191.0f;
            Core::lightPos   = vec3(light.x, light.y, light.z);
            Core::lightColor = vec4(c, c, c, (float)light.attenuation * (float)light.attenuation);
        } else {
            Core::lightPos   = vec3(0);
            Core::lightColor = vec4(0, 0, 0, 1);
        }
        Core::ambient = vec3(1.0f - level.rooms[roomIndex].ambient / 8191.0f);
        Core::active.shader->setParam(uAmbient, Core::ambient);
        Core::active.shader->setParam(uLightPos, Core::lightPos);
        Core::active.shader->setParam(uLightColor, Core::lightColor);
    }

    void renderEntity(const TR::Entity &entity) {
    //  if (!(entity.flags & ENTITY_FLAG_VISIBLE))
    //      return;

        TR::Room &room = level.rooms[entity.room];
        if (!(room.flags & TR::ROOM_FLAG_VISIBLE)) // check for room visibility
            return;

        mat4 m = Core::mModel;
        Core::mModel.translate(vec3(entity.x, entity.y, entity.z));

        float c = (entity.intensity > -1) ? (1.0f - entity.intensity / (float)0x1FFF) : 1.0f;
        float l = 1.0f;

    // set shader
        setRoomShader(room, c)->bind();
        Core::active.shader->setParam(uColor, Core::color);

    // get light parameters for entity
        getLight(vec3(entity.x, entity.y, entity.z), entity.room);

    // render entity models (TODO: remapping or consider model and entity id's)
        bool isModel = false;

        for (int i = 0; i < level.modelsCount; i++)
            if (entity.id == level.models[i].id) {
                isModel = true;
                renderModel(level.models[i], vec3(0, entity.rotation / 16384.0f * PI * 0.5f, 0));
                break;
            }
    
    // if entity is billboard
        if (!isModel) {
            Core::color = vec4(c, c, c, 1.0f);
            shaders[shSprite]->bind();
            Core::active.shader->setParam(uModel, Core::mModel);
            Core::active.shader->setParam(uColor, Core::color);
            for (int i = 0; i < level.spriteSequencesCount; i++)
                if (entity.id == level.spriteSequences[i].id) {
                    mesh->renderSprite(i);
                    break;
                }
        }
    
        Core::mModel = m;
    }

    float tickTextureAnimation = 0.0f;

    void update() {
        time += Core::deltaTime;
        lara->update();

    #ifndef FREE_CAMERA
        camera.pos = vec3(-lara->pos.x, -lara->pos.y, lara->pos.z);
    #endif
        camera.targetDeltaPos = lara->inWater ? vec3(0.0f, -256.0f, 0.0f) : vec3(0.0f, -768.0f, 0.0f);
        camera.targetAngle = vec3(lara->angle.x, -lara->angle.y, 0.0f); //-lara->angle.z);
        camera.update();
    }

    int getCameraRoomIndex() {
        for (int i = 0; i < level.roomsCount; i++)
            if (lara->insideRoom(Core::viewPos, i))
                return i;
        return lara->getEntity().room;
    }

    void render() {
    //    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        camera.setup();;

        atlas->bind(0);
        mesh->bind();

        // set frame constants for all shaders 
        for (int i = 0; i < shMAX; i++) {
            shaders[i]->bind();
            shaders[i]->setParam(uViewProj, Core::mViewProj);
            shaders[i]->setParam(uViewInv, Core::mViewInv);
            shaders[i]->setParam(uViewPos, Core::viewPos);
            shaders[i]->setParam(uParam, vec4(time, 0, 0, 0));
            shaders[i]->setParam(uAnimTexRanges, mesh->animTexRanges[0], mesh->animTexRangesCount); 
            shaders[i]->setParam(uAnimTexOffsets, mesh->animTexOffsets[0], mesh->animTexOffsetsCount); 
        }
        glEnable(GL_DEPTH_TEST);

        Core::setCulling(cfFront);

        Core::mModel.identity();

        for (int i = 0; i < level.roomsCount; i++)
            level.rooms[i].flags &= ~TR::ROOM_FLAG_VISIBLE;    // clear visible flag

    // TODO: collision detection for camera
        renderRoom(getCameraRoomIndex());
        renderRoom(lara->getEntity().room);

        shaders[shStatic]->bind();
        for (int i = 0; i < level.entitiesCount; i++)
            renderEntity(level.entities[i]);

    #ifdef _DEBUG
        Debug::begin();
        Debug::Level::rooms(level, lara->pos, lara->getEntity().room);
    //    Debug::Level::lights(level);
        Debug::Level::portals(level);
        Debug::end();
    #endif
    }
};

#endif