#ifndef H_LEVEL
#define H_LEVEL

#include "core.h"
#include "utils.h"
#include "format.h"
#include "cache.h"
#include "lara.h"
#include "enemy.h"
#include "camera.h"
#include "trigger.h"

#ifdef _DEBUG
    #include "debug.h"
#endif

struct Level : IGame {
    TR::Level   level;
    Texture     *atlas;
    Texture     *cube;
    MeshBuilder *mesh;

    Lara        *lara;
    Camera      *camera;
    Texture     *shadow;

    struct Params {
        float   time;
        float   waterHeight;
        float   clipSign;
        float   clipHeight;
    } *params = (Params*)&Core::params;

    ShaderCache  *shaderCache;
    AmbientCache *ambientCache;
    WaterCache   *waterCache;

    Sound::Sample *sndAmbient;
    Sound::Sample *sndUnderwater;

// IGame implementation ========
    virtual TR::Level* getLevel() {
        return &level;
    }

    virtual MeshBuilder* getMesh()  {
        return mesh;
    }

    virtual Controller* getCamera() {
        return camera;    
    }

    virtual void setClipParams(float clipSign, float clipHeight) {
        params->clipSign   = clipSign;
        params->clipHeight = clipHeight;
    }

    virtual void setWaterParams(float height) {
        params->waterHeight = height;
    }

    virtual void updateParams() {
        Core::active.shader->setParam(uParam, Core::params);
    }

    virtual void waterDrop(const vec3 &pos, float radius, float strength) {
        if (waterCache)
            waterCache->addDrop(pos, radius, strength);
    }

    virtual void setShader(Core::Pass pass, Shader::Type type, bool underwater = false, bool alphaTest = false) {
        shaderCache->bind(pass, type, (underwater ? ShaderCache::FX_UNDERWATER : 0) | (alphaTest ? ShaderCache::FX_ALPHA_TEST : 0) | (params->clipHeight != NO_CLIP_PLANE ? ShaderCache::FX_CLIP_PLANE : 0));
    }

    virtual void renderEnvironment(int roomIndex, const vec3 &pos, Texture **targets, int stride = 0) {
        PROFILE_MARKER("ENVIRONMENT");
    // first pass render level into cube faces
        for (int i = 0; i < 6; i++) {
            setupCubeCamera(pos, i);
            Core::pass = Core::passAmbient;
            Texture *target = targets[0]->cube ? targets[0] : targets[i * stride];
            Core::setTarget(target, true, i);
            renderScene(roomIndex);
            Core::invalidateTarget(false, true);
        }
    }

    virtual void renderCompose(int roomIndex) {
        PROFILE_MARKER("PASS_COMPOSE");
        Core::pass = Core::passCompose;

        Core::setDepthTest(true);
        Core::setDepthWrite(true);
        if (shadow)
            shadow->bind(sShadow);
        renderScene(roomIndex);
    }
//==============================

    Level(Stream &stream, Stream *snd, bool demo, bool home) : level(stream, demo), lara(NULL) {
        params->time = 0.0f;

        #ifdef _DEBUG
            Debug::init();
        #endif
        mesh = new MeshBuilder(level);
        
        initTextures();
        shaderCache = new ShaderCache(this);
        initOverrides();

        for (int i = 0; i < level.entitiesBaseCount; i++) {
            TR::Entity &entity = level.entities[i];
            switch (entity.type) {
                case TR::Entity::LARA                  : 
                case TR::Entity::CUT_1                 :
                    entity.controller = (lara = new Lara(this, i, home));
                    break;
                case TR::Entity::ENEMY_WOLF            :   
                    entity.controller = new Wolf(this, i);
                    break;
                case TR::Entity::ENEMY_BEAR            : 
                    entity.controller = new Bear(this, i);
                    break;
                case TR::Entity::ENEMY_BAT             :   
                    entity.controller = new Bat(this, i);
                    break;
                case TR::Entity::ENEMY_TWIN            :   
                case TR::Entity::ENEMY_CROCODILE_LAND  :   
                case TR::Entity::ENEMY_CROCODILE_WATER :   
                case TR::Entity::ENEMY_LION_MALE       :   
                case TR::Entity::ENEMY_LION_FEMALE     :   
                case TR::Entity::ENEMY_PUMA            :   
                case TR::Entity::ENEMY_GORILLA         :   
                case TR::Entity::ENEMY_RAT_LAND        :   
                case TR::Entity::ENEMY_RAT_WATER       :   
                case TR::Entity::ENEMY_REX             :   
                case TR::Entity::ENEMY_RAPTOR          :   
                case TR::Entity::ENEMY_MUTANT          :   
                case TR::Entity::ENEMY_CENTAUR         :   
                case TR::Entity::ENEMY_MUMMY           :   
                case TR::Entity::ENEMY_LARSON          :
                    entity.controller = new Enemy(this, i, 100);
                    break;
                case TR::Entity::DOOR_1                :
                case TR::Entity::DOOR_2                :
                case TR::Entity::DOOR_3                :
                case TR::Entity::DOOR_4                :
                case TR::Entity::DOOR_5                :
                case TR::Entity::DOOR_6                :
                case TR::Entity::DOOR_BIG_1            :
                case TR::Entity::DOOR_BIG_2            :
                    entity.controller = new Door(this, i);
                    break;
                case TR::Entity::TRAP_DOOR_1           :
                case TR::Entity::TRAP_DOOR_2           :
                    entity.controller = new TrapDoor(this, i);
                    break;
                case TR::Entity::BRIDGE_0              :
                case TR::Entity::BRIDGE_1              :
                case TR::Entity::BRIDGE_2              :
                    entity.controller = new Bridge(this, i);
                    break;
                case TR::Entity::GEARS_1               :
                case TR::Entity::GEARS_2               :
                case TR::Entity::GEARS_3               :
                    entity.controller = new Boulder(this, i);
                    break;
                case TR::Entity::TRAP_FLOOR            :
                    entity.controller = new TrapFloor(this, i);
                    break;
                case TR::Entity::CRYSTAL               :
                    entity.controller = new Crystal(this, i);
                    break;
                case TR::Entity::TRAP_BLADE            :
                case TR::Entity::TRAP_SPIKES           :
                    entity.controller = new Trigger(this, i, true);
                    break;
                case TR::Entity::TRAP_BOULDER          :
                    entity.controller = new Boulder(this, i);
                    break;
                case TR::Entity::TRAP_DARTGUN          :
                    entity.controller = new Dartgun(this, i);
                    break;
                case TR::Entity::BLOCK_1               :
                case TR::Entity::BLOCK_2               :
                    entity.controller = new Block(this, i);
                    break;
                case TR::Entity::SWITCH                :
                case TR::Entity::SWITCH_WATER          :
                case TR::Entity::HOLE_PUZZLE           :
                case TR::Entity::HOLE_KEY              :
                    entity.controller = new Trigger(this, i, false);
                    break;
                case TR::Entity::WATERFALL             :
                    entity.controller = new Waterfall(this, i);
                    break;
                default                                : 
                    if (entity.modelIndex > 0)
                        entity.controller = new Controller(this, i);
                    else
                        entity.controller = new Sprite(this, i, 0);
            }
        }

        ASSERT(lara != NULL);
        camera = new Camera(this, lara);

        level.cameraController = camera;

        ambientCache = Core::settings.ambient ? new AmbientCache(this) : NULL;
        waterCache   = Core::settings.water   ? new WaterCache(this)   : NULL;
        shadow       = Core::settings.shadows ? new Texture(1024, 1024, Texture::SHADOW, false) : NULL;

        initReflections();

        // init sounds
        //Sound::play(Sound::openWAD("05_Lara's_Themes.wav"), 1, 1, 0);
        sndAmbient = Sound::play(snd, vec3(0.0f), 1, 1, Sound::Flags::LOOP);

        sndUnderwater = lara->playSound(TR::SND_UNDERWATER, vec3(0.0f), Sound::LOOP);
        if (sndUnderwater)
            sndUnderwater->volume = 0.0f;

        for (int i = 0; i < level.soundSourcesCount; i++) {
            TR::SoundSource &src = level.soundSources[i];
            lara->playSound(src.id, vec3(float(src.x), float(src.y), float(src.z)), Sound::PAN | Sound::LOOP | Sound::STATIC);
        }
    }

    virtual ~Level() {
        #ifdef _DEBUG
            Debug::free();
        #endif
        for (int i = 0; i < level.entitiesCount; i++)
            delete (Controller*)level.entities[i].controller;

        delete shaderCache;

        delete shadow;
        delete ambientCache;
        delete waterCache;

        delete atlas;
        delete cube;
        delete mesh;

        delete camera;
        Sound::stopAll();
    }

    void initTextures() {
        if (!level.tilesCount) {
            atlas = NULL;
            return;
        }

        // merge all tiles into one 1024x1024 32bpp 
        TR::Color32 *data = new TR::Color32[1024 * 1024];
        for (int i = 0; i < level.tilesCount; i++) {
            int tx = (i % 4) * 256;
            int ty = (i / 4) * 256;

            TR::Color32 *ptr = &data[ty * 1024 + tx];
            for (int y = 0; y < 256; y++) {
                memcpy(ptr, &level.tiles[i].color[y * 256], 256 * sizeof(TR::Color32));
                ptr += 1024;
            }
        }

        // white texture
        for (int y = 1020; y < 1024; y++)
            for (int x = 1020; x < 1024; x++) {
                int i = y * 1024 + x;
                data[i].r = data[i].g = data[i].b = data[i].a = 255;    // white texel for colored triangles
            }
/*
        FILE *f = fopen("atlas.raw", "wb");
        fwrite(data, 1024 * 1024 * 4, 1, f);
        fclose(f);
*/

        atlas = new Texture(1024, 1024, Texture::RGBA, false, data);
        PROFILE_LABEL(TEXTURE, atlas->ID, "atlas");

        uint32 whitePix = 0xFFFFFFFF;
        cube = new Texture(1, 1, Texture::RGBA, true, &whitePix);

        delete[] data;
        delete[] level.tiles;
        level.tiles = NULL;
    }

    void initOverrides() {
    /*
        for (int i = 0; i < level.entitiesCount; i++) {
            int16 &id = level.entities[i].id;
            switch (id) {
            // weapon
                case 84 : id =  99; break; // pistols
                case 85 : id = 100; break; // shotgun
                case 86 : id = 101; break; // magnums
                case 87 : id = 102; break; // uzis
            // ammo
                case 88 : id = 103; break; // for pistols
                case 89 : id = 104; break; // for shotgun
                case 90 : id = 105; break; // for magnums
                case 91 : id = 106; break; // for uzis
            // medikit
                case 93 : id = 108; break; // big
                case 94 : id = 109; break; // small
            // keys
                case 110 : id = 114; break; 
                case 111 : id = 115; break; 
                case 112 : id = 116; break; 
                case 113 : id = 117; break; 
                case 126 : id = 127; break; 
                case 129 : id = 133; break; 
                case 130 : id = 134; break; 
                case 131 : id = 135; break; 
                case 132 : id = 136; break; 
                case 141 : id = 145; break; 
                case 142 : id = 146; break; 
                case 143 : id = 150; break; 
                case 144 : id = 150; break;
            }
        }
    */
    }

    void initReflections() {
        Core::resetStates();        
        for (int i = 0; i < level.entitiesBaseCount; i++) {
            TR::Entity &e = level.entities[i];
            if (e.type == TR::Entity::CRYSTAL) {
                Crystal *c = (Crystal*)e.controller;
                renderEnvironment(c->getRoomIndex(), c->pos - vec3(0, 512, 0), &c->environment);
            }
        }
    }

    void setRoomParams(int roomIndex, Shader::Type type, float diffuse, float ambient, float specular, float alpha, bool alphaTest = false) {
        if (Core::pass == Core::passShadow) {
            setShader(Core::pass, type);
            return;
        }

        TR::Room &room = level.rooms[roomIndex];

        if (room.flags.water)
            setWaterParams(float(room.info.yTop));
        else
            setWaterParams(NO_CLIP_PLANE);

        setShader(Core::pass, type, room.flags.water, alphaTest);

        if (room.flags.water) {
            if (waterCache)
                waterCache->bindCaustics(roomIndex);
            setWaterParams(float(room.info.yTop));
        }

        Core::active.shader->setParam(uMaterial, vec4(diffuse, ambient, specular, alpha));
    }

    void renderRoom(int roomIndex, int from = TR::NO_ROOM) {
        ASSERT(roomIndex >= 0 && roomIndex < level.roomsCount);
        PROFILE_MARKER("ROOM");

        TR::Room &room = level.rooms[roomIndex];
        vec3 offset = vec3(float(room.info.x), 0.0f, float(room.info.z));

    // room geometry & sprites
        if (!room.flags.rendered) { // skip if already rendered
            if (waterCache && room.flags.water)
                waterCache->setVisible(roomIndex);

            room.flags.rendered = true;

            if (Core::pass != Core::passShadow) {

                Basis qTemp = Core::basis;
                Core::basis.translate(offset);

                MeshBuilder::RoomRange &range = mesh->rooms[roomIndex];

                for (int transp = 0; transp < 2; transp++) {
                    if (!range.geometry[transp].iCount)
                        continue;

                    Core::setBlending(transp ? bmAlpha : bmNone);

                    setRoomParams(roomIndex, Shader::ROOM, 1.0f, intensityf(room.ambient), 0.0f, 1.0f, transp > 0);
                    Shader *sh = Core::active.shader;
                    sh->setParam(uLightColor, Core::lightColor[0], MAX_LIGHTS);
                    sh->setParam(uLightPos,   Core::lightPos[0],   MAX_LIGHTS);
                    sh->setParam(uBasis,      Core::basis);

                // render room geometry
                    mesh->renderRoomGeometry(roomIndex, transp > 0);
                }

            // render room sprites
                if (range.sprites.iCount) {
                    Core::setBlending(bmAlpha);
                    setRoomParams(roomIndex, Shader::SPRITE, 1.0f, 1.0f, 0.0f, 1.0f, true);
                    Shader *sh = Core::active.shader;
                    sh->setParam(uLightColor, Core::lightColor[0], MAX_LIGHTS);
                    sh->setParam(uLightPos,   Core::lightPos[0],   MAX_LIGHTS);
                    sh->setParam(uBasis, Core::basis);
                    mesh->renderRoomSprites(roomIndex);
                }

                Core::basis = qTemp;
            }
            Core::setBlending(bmNone);
        }

    #ifdef LEVEL_EDITOR
        return;
    #endif

    // render rooms through portals recursively
        Frustum *camFrustum = camera->frustum;   // push camera frustum
        Frustum frustum;
        camera->frustum = &frustum;

        for (int i = 0; i < room.portalsCount; i++) {
            TR::Room::Portal &p = room.portals[i];

            if (p.roomIndex == from) continue;

            vec3 v[] = {
                offset + p.vertices[0],
                offset + p.vertices[1],
                offset + p.vertices[2],
                offset + p.vertices[3],
            };

            frustum = *camFrustum;
            if (frustum.clipByPortal(v, 4, p.normal)) {
                if (waterCache &&(level.rooms[roomIndex].flags.water ^ level.rooms[p.roomIndex].flags.water) && v[0].y == v[1].y && v[0].y == v[2].y)
                    waterCache->setVisible(roomIndex, p.roomIndex);
                renderRoom(p.roomIndex, roomIndex);
            }
        }
        camera->frustum = camFrustum;    // pop camera frustum
    }

    int getLightIndex(const vec3 &pos, int &room, float maxAtt = -1.0f, int depth = 0) {
        int idx = -1;

        TR::Room &r = level.rooms[room];

        for (int i = 0; i < r.lightsCount; i++) {
            TR::Room::Light &light = r.lights[i];
            if (light.intensity > 0x1FFF) continue;
            float att = max(0.0f, 1.0f - (pos - vec3(float(light.x), float(light.y), float(light.z))).length2() / ((float)light.radius * (float)light.radius));
            if (att > maxAtt) {
                maxAtt = att;
                idx    = i;
            }
        }

        if (depth > 0) 
            for (int i = 0; i < r.portalsCount; i++) {
                int nextRoom = r.portals[i].roomIndex;
                int nextLight = getLightIndex(pos, nextRoom, maxAtt, depth - 1);
                if (nextLight > -1) {
                    room = nextRoom;
                    idx  = nextLight;
                }
            }

        return idx;
    }

    void getLight(const vec3 &pos, int roomIndex) {
        int room = roomIndex;
        int idx = getLightIndex(pos, room);

        if (idx > -1) {
            TR::Room::Light &light = level.rooms[room].lights[idx];
            float c = 1.0f - intensityf(level.rooms[room].lights[idx].intensity);
            Core::lightPos[0]   = vec3(float(light.x), float(light.y), float(light.z));
            Core::lightColor[0] = vec4(c, c, c, (float)light.radius * (float)light.radius);
        } else {
            Core::lightPos[0]   = vec3(0);
            Core::lightColor[0] = vec4(0, 0, 0, 1);
        }
    }

    void renderEntity(const TR::Entity &entity) {
        //if (entity.room != lara->getRoomIndex()) return;
        if (entity.type == TR::Entity::NONE || !entity.modelIndex) return;
        if (Core::pass == Core::passShadow && !TR::castShadow(entity.type)) return;

        ASSERT(entity.controller);

        bool isModel = entity.modelIndex > 0;

        Controller *controller = (Controller*)entity.controller;


        TR::Room &room = level.rooms[entity.room];
        if (!room.flags.rendered || entity.flags.invisible || entity.flags.rendered)
            return;

        int16 lum = entity.intensity == -1 ? room.ambient : entity.intensity;

        Shader::Type type = isModel ? Shader::ENTITY : Shader::SPRITE;
        if (entity.type == TR::Entity::CRYSTAL)
            type = Shader::MIRROR;

        int roomIndex = entity.room;
//        if (entity.type == TR::Entity::LARA && ((Lara*)entity.controller)->state == Lara::STATE_WATER_OUT)
//            roomIndex = ((Lara*)entity.controller)->roomPrev;

        setRoomParams(roomIndex, type, 1.0f, intensityf(lum), controller->specular, 1.0f, isModel ? mesh->models[entity.modelIndex].opaque : true);

        if (isModel) { // model
            vec3 pos = controller->getPos();
            if (Core::settings.ambient) {
                AmbientCache::Cube cube;
                if (Core::frameIndex != controller->frameIndex) {
                    ambientCache->getAmbient(entity.room, pos, cube);
                    if (cube.status == AmbientCache::Cube::READY)
                        memcpy(controller->ambient, cube.colors, sizeof(cube.colors)); // store last calculated ambient into controller
                }
                Core::active.shader->setParam(uAmbient, controller->ambient[0], 6);
            }
            getLight(pos, entity.room);
        } else { // sprite
            Core::lightPos[0]   = vec3(0);
            Core::lightColor[0] = vec4(0, 0, 0, 1);
        }        
        
        Core::active.shader->setParam(uLightPos,   Core::lightPos[0],   MAX_LIGHTS);
        Core::active.shader->setParam(uLightColor, Core::lightColor[0], MAX_LIGHTS);

        controller->render(camera->frustum, mesh, type, room.flags.water);
    }

    void update() {
        params->time += Core::deltaTime;

        for (int i = 0; i < level.entitiesCount; i++) {
            TR::Entity &e = level.entities[i];
            if (e.type != TR::Entity::NONE) {
                Controller *controller = (Controller*)e.controller;
                if (controller) {
                    controller->update();

                    if (waterCache && e.type == TR::Entity::WATERFALL && ((Waterfall*)controller)->drop) { // add water drops for waterfalls
                        Waterfall *w = (Waterfall*)controller;
                        waterCache->addDrop(w->dropPos, w->dropRadius, w->dropStrength);
                    }
                }
            }
        }

        camera->update();
        float ambientVolume =  camera->isUnderwater() ? 0.0f : 1.0f;
        if (sndAmbient) sndAmbient->volume = ambientVolume;
        if (sndUnderwater) sndUnderwater->volume = 1.0f - ambientVolume;

        if (waterCache) 
            waterCache->update();
    }

    void setup() {
        PROFILE_MARKER("SETUP");

        camera->setup(Core::pass == Core::passCompose);

        atlas->bind(sDiffuse);
        Core::whiteTex->bind(sNormal);
        Core::whiteTex->bind(sMask);
        Core::whiteTex->bind(sReflect);        
        cube->bind(sEnvironment);

        if (!Core::support.VAO)
            mesh->bind();
        //Core::mViewProj = Core::mLightProj;
        Core::basis.identity();

        // clear visibility flag for rooms
        for (int i = 0; i < level.roomsCount; i++)
            level.rooms[i].flags.rendered = false;
        
        if (Core::pass != Core::passAmbient)
            for (int i = 0; i < level.entitiesCount; i++)
                level.entities[i].flags.rendered = false;
    }

    void renderRooms(int roomIndex) {
        PROFILE_MARKER("ROOMS");

        getLight(lara->pos, lara->getRoomIndex());

    #ifdef LEVEL_EDITOR
        for (int i = 0; i < level.roomsCount; i++)
            renderRoom(i);
    #else
        if (!camera->cutscene)
            renderRoom(roomIndex);
        else // TODO: use brain
            for (int i = 0; i < level.roomsCount; i++)
                renderRoom(i);
    #endif
    }

    void renderEntities() {
        PROFILE_MARKER("ENTITIES");
        for (int i = 0; i < level.entitiesCount; i++)
            renderEntity(level.entities[i]);
    }

    void renderScene(int roomIndex) {
        PROFILE_MARKER("SCENE");
        setup();
        renderRooms(roomIndex);
        if (Core::pass != Core::passAmbient)
            renderEntities();
    }

    void setupCubeCamera(const vec3 &pos, int face) {
        vec3 up  = vec3(0, -1, 0);
        vec3 dir;
        switch (face) {
            case 0 : dir = vec3( 1,  0,  0); break;
            case 1 : dir = vec3(-1,  0,  0); break;
            case 2 : dir = vec3( 0,  1,  0); up = vec3(0, 0,  1); break;
            case 3 : dir = vec3( 0, -1,  0); up = vec3(0, 0, -1); break;
            case 4 : dir = vec3( 0,  0,  1); break;
            case 5 : dir = vec3( 0,  0, -1); break;
        }

        Core::mViewInv = mat4(pos, pos + dir, up);
        Core::mView    = Core::mViewInv.inverse();
        Core::mProj    = mat4(90, 1.0f, camera->znear, camera->zfar);
    }

    bool setupLightCamera() {
        vec3 pos = lara->getPos();
    
    // omni-spot light shadows
        int room = lara->getRoomIndex();
        int idx = getLightIndex(lara->pos, room);
        if (idx < 0) return false;

        TR::Room::Light &light = level.rooms[room].lights[idx];
        vec3 shadowLightPos = vec3(float(light.x), float(light.y), float(light.z)); 
        Core::mViewInv = mat4(shadowLightPos, pos - vec3(0, 256, 0), vec3(0, -1, 0));
        Core::mView    = Core::mViewInv.inverse();
        Core::mProj    = mat4(120, 1.0f, camera->znear, camera->zfar);

        mat4 bias;
        bias.identity();
        bias.e03 = bias.e13 = bias.e23 = bias.e00 = bias.e11 = bias.e22 = 0.5f;
        Core::mLightProj = bias * Core::mProj * Core::mView;

        return true;
    }

    void renderShadows(int roomIndex) {
        PROFILE_MARKER("PASS_SHADOW");
        Core::pass = Core::passShadow;
        if (!setupLightCamera()) return;
        shadow->unbind(sShadow);
        bool colorShadow = shadow->format == Texture::Format::RGBA ? true : false;
        if (colorShadow)
            Core::setClearColor(vec4(1.0f, 1.0f, 1.0f, 1.0f));
	    Core::setTarget(shadow, true);
        Core::setCulling(cfBack);
	    renderScene(roomIndex);
        Core::invalidateTarget(!colorShadow, colorShadow);
        Core::setCulling(cfFront);	    
        if (colorShadow)
            Core::setClearColor(vec4(0.0f, 0.0f, 0.0f, 0.0f));
    }

    void render() {
        Core::invalidateTarget(true, true);
        params->clipHeight  = NO_CLIP_PLANE;
        params->clipSign    = 1.0f;
        params->waterHeight = params->clipHeight;
        Core::resetStates();
        
        if (ambientCache)
            ambientCache->precessQueue();
        if (waterCache)
            waterCache->reset();
        if (shadow)
            renderShadows(lara->getRoomIndex());

        Core::setTarget(NULL, true);
        Core::setViewport(0, 0, Core::width, Core::height);

        if (waterCache)
            waterCache->checkVisibility = true;

        renderCompose(camera->getRoomIndex());

        if (waterCache) {
            waterCache->checkVisibility = false;
            waterCache->render();
        }

//        Core::mViewInv = camera->mViewInv;
//        Core::mView = Core::mViewInv.inverse();

    #ifdef _DEBUG
        camera->setup(true);

        static int snd_index = 0;
        if (Input::down[ikG]) {
            snd_index = (snd_index + 1) % level.soundsInfoCount;
            LOG("play sound: %d\n", snd_index);
            lara->playSound(snd_index, lara->pos, 0);
            Input::down[ikG] = false;
        }
        /*
        static int modelIndex = 0;
        static bool lastStateK = false;
        static int lastEntity = -1;
        if (Input::down[ikM]) {
            if (!lastStateK) {
                lastStateK = true;
                modelIndex = (modelIndex + 1) % level.modelsCount;
            //    modelIndex = (modelIndex + 1) % level.spriteSequencesCount;
                LOG("model: %d %d\n", modelIndex, level.spriteSequences[modelIndex].type);
                if (lastEntity > -1) {
                    delete level.entities[lastEntity].controller;
                    level.entityRemove(lastEntity);
                }
                vec3 p = lara->pos + lara->getDir() * 256.0f;
                lastEntity = level.entityAdd(level.models[modelIndex].type, lara->getRoomIndex(), p.x, p.y - 512, p.z, lara->getEntity().rotation, -1);
                level.entities[lastEntity].controller = new Controller(this, lastEntity);
            }
        } else
            lastStateK = false;

        if (lastEntity > -1)
            renderEntity(level.entities[lastEntity]);
//        renderModel(level.models[modelIndex], level.entities[4]);
*/
        Debug::begin();

        lara->updateEntity(); // TODO clip angle while rotating

        int q = int(normalizeAngle(lara->angleExt + PI * 0.25f) / (PI * 0.5f));
        float radius = 256.0f;

        const vec2 v[] = {
            { -radius,  radius },
            {  radius,  radius },
            {  radius, -radius },
            { -radius, -radius },
        };

        const vec2 &l = v[q],
                   &r = v[(q + 1) % 4],
                   &f = (q %= 2) ? vec2(l.x, radius * cosf(lara->angleExt)) : vec2(radius * sinf(lara->angleExt), l.y);

        vec3 F = vec3(f.x, 0.0f, f.y);
        vec3 L = vec3(l.x, 0.0f, l.y);
        vec3 R = vec3(r.x, 0.0f, r.y);

        vec3 p, n = lara->pos + vec3(0.0f, -512.0f, 0.0f);
        
        Core::setDepthTest(false);
        glBegin(GL_LINES);
            glColor3f(0, 0, 1); p = n; glVertex3fv((GLfloat*)&p); p += F; glVertex3fv((GLfloat*)&p);
            glColor3f(1, 0, 0); p = n; glVertex3fv((GLfloat*)&p); p += L; glVertex3fv((GLfloat*)&p);
            glColor3f(0, 1, 0); p = n; glVertex3fv((GLfloat*)&p); p += R; glVertex3fv((GLfloat*)&p);
            glColor3f(1, 1, 0); p = lara->pos; glVertex3fv((GLfloat*)&p); p -= vec3(0.0f, LARA_HANG_OFFSET, 0.0f); glVertex3fv((GLfloat*)&p);
        glEnd();
        Core::setDepthTest(true);

        /*
        
            glMatrixMode(GL_MODELVIEW);
            glPushMatrix();
            glLoadIdentity();
            glMatrixMode(GL_PROJECTION);
            glPushMatrix();
            glLoadIdentity();
            glOrtho(0, Core::width, 0, Core::height, 0, 1);

            if (waterCache->count)
                waterCache->refract->bind(sDiffuse);
            else
                atlas->bind(sDiffuse);
            glEnable(GL_TEXTURE_2D);
            glDisable(GL_CULL_FACE);
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_BLEND);

            glColor3f(10, 10, 10);
            int w = Core::active.textures[sDiffuse]->width / 2;
            int h = Core::active.textures[sDiffuse]->height / 2;
            glBegin(GL_QUADS);
                glTexCoord2f(0, 0); glVertex2f(0, 0);
                glTexCoord2f(1, 0); glVertex2f(w, 0);
                glTexCoord2f(1, 1); glVertex2f(w, h);
                glTexCoord2f(0, 1); glVertex2f(0, h);
            glEnd();
            glColor3f(1, 1, 1);

            glDisable(GL_TEXTURE_2D);
            glEnable(GL_CULL_FACE);
            glEnable(GL_DEPTH_TEST);
            glEnable(GL_BLEND);

            glMatrixMode(GL_PROJECTION);
            glPopMatrix();
            glMatrixMode(GL_MODELVIEW);
            glPopMatrix();
        
            
            Core::setBlending(bmAlpha);
        //    Debug::Level::rooms(level, lara->pos, lara->getEntity().room);
        //    Debug::Level::lights(level, lara->getRoomIndex());
        //    Debug::Level::sectors(level, lara->getRoomIndex(), (int)lara->pos.y);
        //    Core::setDepthTest(false);
        //    Debug::Level::portals(level);
        //    Core::setDepthTest(true);
        //    Debug::Level::meshes(level);
        //    Debug::Level::entities(level);
            Core::setBlending(bmNone);

        /*
            static int dbg_ambient = 0;
            dbg_ambient = int(params->time * 2) % 4;

            shadow->unbind(sShadow);
            atlas->bind(sDiffuse);
            glEnable(GL_TEXTURE_2D);
            glDisable(GL_CULL_FACE);
            glColor3f(1, 1, 1);
            for (int j = 0; j < 6; j++) {
                glPushMatrix();
                vec3 p = lara->pos;//getPos();
                glTranslatef(p.x, p.y - 1024, p.z);
                switch (j) {
                    case 0 : glRotatef( 90, 0, 1, 0); break;
                    case 1 : glRotatef(-90, 0, 1, 0); break;
                    case 2 : glRotatef(-90, 1, 0, 0); break;
                    case 3 : glRotatef( 90, 1, 0, 0); break;
                    case 4 : glRotatef(  0, 0, 1, 0); break;
                    case 5 : glRotatef(180, 0, 1, 0); break;
                }
                glTranslatef(0, 0, 128);
                
                ambientCache->textures[j * 4 + dbg_ambient]->bind(sDiffuse);
                
                glBegin(GL_QUADS);
                    glTexCoord2f(0, 0); glVertex3f(-128,  128, 0);
                    glTexCoord2f(1, 0); glVertex3f( 128,  128, 0);
                    glTexCoord2f(1, 1); glVertex3f( 128, -128, 0);
                    glTexCoord2f(0, 1); glVertex3f(-128, -128, 0);
                glEnd();

                glPopMatrix();
            }

            glEnable(GL_CULL_FACE);
            glDisable(GL_TEXTURE_2D);
             
            glLineWidth(4);
            glBegin(GL_LINES);
            float S = 64.0f;
            for (int i = 0; i < level.roomsCount; i++) {
                TR::Room &r = level.rooms[i];                
                for (int j = 0; j < r.xSectors * r.zSectors; j++) {
                    TR::Room::Sector &s = r.sectors[j];
                    vec3 p = vec3(float((j / r.zSectors) * 1024 + 512 + r.info.x),
                                  float(max((s.floor - 2) * 256, (s.floor + s.ceiling) * 256 / 2)),
                                  float((j % r.zSectors) * 1024 + 512 + r.info.z));

                    AmbientCache::Cube &cube = ambientCache->items[ambientCache->offsets[i] + j];
                    if (cube.status == AmbientCache::Cube::READY) {
                        glColor3f(powf(cube.colors[0].x, 1.0f / 2.2f), powf(cube.colors[0].y, 1.0f / 2.2f), powf(cube.colors[0].z, 1.0f / 2.2f));
                        glVertex3f(p.x + 0, p.y + 0, p.z + 0);
                        glVertex3f(p.x + S, p.y + 0, p.z + 0);

                        glColor3f(powf(cube.colors[1].x, 1.0f / 2.2f), powf(cube.colors[1].y, 1.0f / 2.2f), powf(cube.colors[1].z, 1.0f / 2.2f));
                        glVertex3f(p.x + 0, p.y + 0, p.z + 0);
                        glVertex3f(p.x - S, p.y + 0, p.z + 0);

                        glColor3f(powf(cube.colors[2].x, 1.0f / 2.2f), powf(cube.colors[2].y, 1.0f / 2.2f), powf(cube.colors[2].z, 1.0f / 2.2f));
                        glVertex3f(p.x + 0, p.y + 0, p.z + 0);
                        glVertex3f(p.x + 0, p.y + S, p.z + 0);

                        glColor3f(powf(cube.colors[3].x, 1.0f / 2.2f), powf(cube.colors[3].y, 1.0f / 2.2f), powf(cube.colors[3].z, 1.0f / 2.2f));
                        glVertex3f(p.x + 0, p.y + 0, p.z + 0);
                        glVertex3f(p.x + 0, p.y - S, p.z + 0);

                        glColor3f(powf(cube.colors[4].x, 1.0f / 2.2f), powf(cube.colors[4].y, 1.0f / 2.2f), powf(cube.colors[4].z, 1.0f / 2.2f));
                        glVertex3f(p.x + 0, p.y + 0, p.z + 0);
                        glVertex3f(p.x + 0, p.y + 0, p.z + S);

                        glColor3f(powf(cube.colors[5].x, 1.0f / 2.2f), powf(cube.colors[5].y, 1.0f / 2.2f), powf(cube.colors[5].z, 1.0f / 2.2f));
                        glVertex3f(p.x + 0, p.y + 0, p.z + 0);
                        glVertex3f(p.x + 0, p.y + 0, p.z - S);
                    }
                }
            }
            glEnd();
            glLineWidth(1);

            */
           /*
            shaders[shGUI]->bind();
            Core::mViewProj = mat4(0, (float)Core::width, (float)Core::height, 0, 0, 1);
            Core::active.shader->setParam(uViewProj, Core::mViewProj);            
            atlas->bind(0);

            glDisable(GL_DEPTH_TEST);
            glDisable(GL_CULL_FACE);
            
            glEnable(GL_CULL_FACE);
            glEnable(GL_DEPTH_TEST);
            */


            Debug::Level::info(level, lara->getEntity(), lara->animation);


        Debug::end();
    #endif
    }
};

#endif