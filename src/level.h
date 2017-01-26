#ifndef H_LEVEL
#define H_LEVEL

#include "core.h"
#include "utils.h"
#include "format.h"
#include "lara.h"
#include "enemy.h"
#include "camera.h"
#include "trigger.h"

#ifdef _DEBUG
    #include "debug.h"
#endif

const char SHADER[] =
    #include "shader.glsl"
;

const char FILTER[] =
    #include "filter.glsl"
;

const char WATER[] =
    #include "water.glsl"
;

const char GUI[] =
    #include "gui.glsl"
;

struct Level {
    enum { shCompose, shShadow, shAmbient, shFilter, shWater, shGUI, shMAX };

    TR::Level   level;
    Shader      *shaders[shMAX];
    Texture     *atlas;
    Texture     *cube;
    MeshBuilder *mesh;

    Lara        *lara;
    Camera      *camera;
    Texture     *shadow;

    Texture     *water[2];

    Texture     *waterRefract;
    Texture     *waterReflect;
    Texture     *waterCaustics;
    float       waterTimer;

    float   time;
    float   clipHeight;
    float   clipSign;

    struct AmbientCache {
        Level *level;

        struct Cube {
            enum int32 {
                BLANK, WAIT, READY
            }    status;
            vec3 colors[6];
        } *items;
        int *offsets;

        struct Task {
            int  room;
            int  sector;
            Cube *cube;
        } tasks[32];
        int tasksCount;

        Texture *textures[6 * 4]; // 64, 16, 4, 1 

        AmbientCache(Level *level) : level(level), tasksCount(0) {
            items   = NULL;
            offsets = new int[level->level.roomsCount];
            int sectors = 0;
            for (int i = 0; i < level->level.roomsCount; i++) {
                TR::Room &r = level->level.rooms[i];
                offsets[i] = sectors;
                sectors += r.xSectors * r.zSectors;
            }
        // init cache buffer
            items = new Cube[sectors];
            memset(items, 0, sizeof(Cube) * sectors);
        // init downsample textures
            for (int j = 0; j < 6; j++)
                for (int i = 0; i < 4; i++)
                    textures[j * 4 + i] = new Texture(64 >> (i << 1), 64 >> (i << 1), Texture::RGBA, false);
        }

        ~AmbientCache() {
            delete[] items;
            delete[] offsets;
            for (int i = 0; i < 6 * 4; i++)
                delete textures[i];
        }

        void addTask(int room, int sector) {
            if (tasksCount >= 32) return;

            Task &task  = tasks[tasksCount++];
            task.room   = room;
            task.sector = sector;
            task.cube   = &items[offsets[room] + sector];
            task.cube->status = Cube::WAIT;
        }

        void renderAmbient(int room, int sector, vec3 *colors) {
            PROFILE_MARKER("PASS_AMBIENT");
                
            TR::Room &r = level->level.rooms[room];
            TR::Room::Sector &s = r.sectors[sector];
            
            vec3 pos = vec3(float((sector / r.zSectors) * 1024 + 512 + r.info.x), 
                            float(max((s.floor - 2) * 256, (s.floor + s.ceiling) * 256 / 2)),
                            float((sector % r.zSectors) * 1024 + 512 + r.info.z));

            // first pass - render environment from position (room geometry & static meshes)
            level->renderEnvironment(room, pos, textures, 4);

            // second pass - downsample it
            glDisable(GL_DEPTH_TEST);
//            glDisable(GL_CULL_FACE);
            level->setPassShader(Core::passFilter);
            Core::active.shader->setParam(uType, Shader::FILTER_DOWNSAMPLE);

            for (int i = 1; i < 4; i++) {
                int size = 64 >> (i << 1);

                Core::active.shader->setParam(uParam, vec4(float(size << 2), 0.0f, 0.0f, 0.0f));

                for (int j = 0; j < 6; j++) {
                    Texture *src = textures[j * 4 + i - 1];
                    Texture *dst = textures[j * 4 + i];
                    Core::setTarget(dst);
                    src->bind(sDiffuse);
                    level->mesh->renderQuad();
                }
            }

            // get result color from 1x1 textures
            for (int j = 0; j < 6; j++) {
                Core::setTarget(textures[j * 4 + 3]);

                TR::Color32 color;
                glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &color);
                colors[j] = vec3(powf(color.r / 255.0f, 2.2f), powf(color.g / 255.0f, 2.2f), powf(color.b / 255.0f, 2.2f)); // to linear space
            }
            Core::setTarget(NULL);

            glEnable(GL_DEPTH_TEST);
//            glEnable(GL_CULL_FACE);
        }

        void precessQueue() {
            for (int i = 0; i < tasksCount; i++) {
                Task &task = tasks[i];
                renderAmbient(task.room, task.sector, &task.cube->colors[0]);
                task.cube->status = Cube::READY;
            }
            tasksCount = 0;
        }

        Cube* getAmbient(int room, int sector) {
            Cube *cube = &items[offsets[room] + sector];
            if (cube->status == Cube::BLANK)
                addTask(room, sector);            
            return cube->status == Cube::READY ? cube : NULL;
        }

        void getAmbient(int room, const vec3 &pos, Cube &value) {
            TR::Room &r = level->level.rooms[room];
            
            int sx = clamp((int(pos.x) - r.info.x) / 1024, 0, r.xSectors - 1);
            int sz = clamp((int(pos.z) - r.info.z) / 1024, 0, r.zSectors - 1);
            
            int sector = sx * r.zSectors + sz;
            Cube *a = getAmbient(room, sector);
            if (a)
                value = *a;
            else
                value.status = Cube::BLANK;
        }
    } *ambientCache;

    /*
    struct LightCache {

        struct Item {
            int     room;
            int     index;
            float   intensity;
        } items[MAX_CACHED_LIGHTS];

        void updateLightCache(const TR::Level &level, const vec3 &pos, int room) {
        // update intensity
            for (int i = 0; i < MAX_CACHED_LIGHTS; i++) {
                Item &item = items[i];
                if (c.intensity < 0.0f) continue;
                TR::Room::Light &light = level.rooms[c.room].lights[i];
                c.intensity = max(0.0f, 1.0f - (pos - vec3(float(light.x), float(light.y), float(light.z))).length2() / ((float)light.radius * (float)light.radius));
            }

        // check for new lights
            int index = getLightIndex(pos, room);

            if (index >= 0 && (items[0].room != room || items[0].index != index)) [
                TR::Room::Light &light = level.rooms[room].lights[index];
                float intensity = max(0.0f, 1.0f - (lara->pos - vec3(float(light.x), float(light.y), float(light.z))).length2() / ((float)light.radius * (float)light.radius));

                int i = 0;
                while (i < MAX_CACHED_LIGHTS && lightCache[i].intensity > intensity) // get sorted place
                    i++;
                if (i < MAX_CACHED_LIGHTS) { // insert light
                    for (int j = MAX_CACHED_LIGHTS - 1; j > i; j--)
                         lightCache[j] = lightCache[j - 1];
                    lightCache[i].room      = room;
                    lightCache[i].index     = index;
                    lightCache[i].intensity = intensity;
                }

            }
        }
    } lightCache;
    */
    Level(Stream &stream, bool demo, bool home) : level(stream, demo), lara(NULL), time(0.0f) {
        #ifdef _DEBUG
            Debug::init();
        #endif
        mesh = new MeshBuilder(level);
        
        initTextures();
        initShaders();
        initOverrides();

        for (int i = 0; i < level.entitiesBaseCount; i++) {
            TR::Entity &entity = level.entities[i];
            switch (entity.type) {
                case TR::Entity::LARA                  : 
                case TR::Entity::CUT_1                 :
                    entity.controller = (lara = new Lara(&level, i, home));
                    break;
                case TR::Entity::ENEMY_WOLF            :   
                    entity.controller = new Wolf(&level, i);
                    break;
                case TR::Entity::ENEMY_BEAR            : 
                    entity.controller = new Bear(&level, i);
                    break;
                case TR::Entity::ENEMY_BAT             :   
                    entity.controller = new Bat(&level, i);
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
                    entity.controller = new Enemy(&level, i, 100);
                    break;
                case TR::Entity::DOOR_1                :
                case TR::Entity::DOOR_2                :
                case TR::Entity::DOOR_3                :
                case TR::Entity::DOOR_4                :
                case TR::Entity::DOOR_5                :
                case TR::Entity::DOOR_6                :
                case TR::Entity::DOOR_BIG_1            :
                case TR::Entity::DOOR_BIG_2            :
                    entity.controller = new Door(&level, i);
                    break;
                case TR::Entity::TRAP_DOOR_1           :
                case TR::Entity::TRAP_DOOR_2           :
                    entity.controller = new TrapDoor(&level, i);
                    break;
                case TR::Entity::BRIDGE_0              :
                case TR::Entity::BRIDGE_1              :
                case TR::Entity::BRIDGE_2              :
                    entity.controller = new Bridge(&level, i);
                    break;
                case TR::Entity::GEARS_1               :
                case TR::Entity::GEARS_2               :
                case TR::Entity::GEARS_3               :
                    entity.controller = new Boulder(&level, i);
                    break;
                case TR::Entity::TRAP_FLOOR            :
                    entity.controller = new TrapFloor(&level, i);
                    break;
                case TR::Entity::CRYSTAL               :
                    entity.controller = new Crystal(&level, i);
                    break;
                case TR::Entity::TRAP_BLADE            :
                case TR::Entity::TRAP_SPIKES           :
                    entity.controller = new Trigger(&level, i, true);
                    break;
                case TR::Entity::TRAP_BOULDER          :
                    entity.controller = new Boulder(&level, i);
                    break;
                case TR::Entity::TRAP_DARTGUN          :
                    entity.controller = new Dartgun(&level, i);
                    break;
                case TR::Entity::BLOCK_1               :
                case TR::Entity::BLOCK_2               :
                    entity.controller = new Block(&level, i);
                    break;
                case TR::Entity::SWITCH                :
                case TR::Entity::SWITCH_WATER          :
                case TR::Entity::HOLE_PUZZLE           :
                case TR::Entity::HOLE_KEY              :
                    entity.controller = new Trigger(&level, i, false);
                    break;
                default                                : 
                    if (entity.modelIndex > 0)
                        entity.controller = new Controller(&level, i);
                    else
                        entity.controller = new Sprite(&level, i, 0);
            }
        }

        ASSERT(lara != NULL);
        camera = new Camera(&level, lara);

        level.cameraController = camera;

        initReflections();

        ambientCache = new AmbientCache(this);
    }

    ~Level() {
        #ifdef _DEBUG
            Debug::free();
        #endif
        for (int i = 0; i < level.entitiesCount; i++)
            delete (Controller*)level.entities[i].controller;

        for (int i = 0; i < shMAX; i++)
            delete shaders[i];

        delete shadow;
        delete ambientCache;

        delete water[0];
        delete water[1];

        delete waterRefract;
        delete waterReflect;
        delete waterCaustics;

        delete atlas;
        delete cube;
        delete mesh;

        delete camera;        
    }

    #define WATER_SIMULATE_TIMESTEP (1.0f / 40.0f)

    void initTextures() {
        shadow = new Texture(1024, 1024, Texture::SHADOW, false);

        water[0] = new Texture(256, 256, Texture::RGBA_HALF, false);
        water[1] = new Texture(256, 256, Texture::RGBA_HALF, false);

        waterRefract  = NULL;
        waterReflect  = new Texture( 512, 512, Texture::RGBA, false);
        waterCaustics = new Texture( 256, 256, Texture::RGBA, false);

        waterTimer = WATER_SIMULATE_TIMESTEP;

        Core::setTarget(waterRefract); // clear refract mask (frame borders)
        Core::clear(vec4(0.0f));
        Core::setTarget(NULL);

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

    void checkRefractTexture() {
        if (waterRefract && Core::width <= waterRefract->width && Core::height <= waterRefract->height) return;
        delete waterRefract;
        waterRefract = new Texture(nextPow2(Core::width), nextPow2(Core::height), Texture::RGBA, false);
    }

    void initShaders() {
        char def[255], ext[255];

        ext[0] = 0;
		if (Core::support.shadowSampler) {
			#ifdef MOBILE
				strcat(ext, "#extension GL_EXT_shadow_samplers : require\n");
			#endif
			strcat(ext, "#define SHADOW_SAMPLER\n");
		} else
			if (Core::support.depthTexture)
				strcat(ext, "#define SHADOW_DEPTH\n");
			else
				strcat(ext, "#define SHADOW_COLOR\n");

        sprintf(def, "%s#define PASS_COMPOSE\n#define MAX_LIGHTS %d\n#define MAX_RANGES %d\n#define MAX_OFFSETS %d\n", ext, MAX_LIGHTS, mesh->animTexRangesCount, mesh->animTexOffsetsCount);
        shaders[shCompose]  = new Shader(SHADER, def);
        sprintf(def, "%s#define PASS_SHADOW\n", ext);
        shaders[shShadow]   = new Shader(SHADER, def);
        sprintf(def, "%s#define PASS_AMBIENT\n", ext);
        shaders[shAmbient]  = new Shader(SHADER, def);
        shaders[shFilter]   = new Shader(FILTER, "");
        shaders[shWater]    = new Shader(WATER, "");
        shaders[shGUI]      = new Shader(GUI, "");
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

#ifdef LEVEL_EDITOR
    struct Light {
        vec3    pos;
        float   radius;
        float   intensity;
    } lights[255];
    int lightsCount    = 0;
    int lightIntensity = 0.5f;
    float lightRadius  = 4096.0f;

    int findLight(const vec3 &pos, float maxDist) const {
        int index = 0;
        for (int i = 0; i < lightsCount; i++) {
            float d = (lights[i].pos - pos).length();
            if (d < maxDist) {
                maxDist = d;
                index   = i;
            }
        }
        return index;
    }

    void addLight(const vec3 &pos, float radius) {
        int index = findLight(pos, 1024.0f);
        if (index > -1) {
            lightRadius    = lights[index].radius;
            lightIntensity = lights[index].intensity;
        } else
            index = lightsCount++;
        
        lights[index].pos    = pos;
        lights[index].radius = radius;
    }

    void removeLight(const vec3 &pos) {
        int index = findLight(pos, 1024.0f);
        if (index > -1)
            lights[index] = lights[--lightsCount];
    }

    void updateLight(const vec3 &pos, float addRadius, float addIntensity) {
        int index = findLight(pos, 1024.0f);
        if (index > -1) {            
            lights[index].radius    = max(lights[index].radius + addRadius, 256.0f);
            lights[index].intensity = clamp(lights[index].intensity + addIntensity, 0.0f, 1.0f);
            lightRadius    = lights[index].radius;
            lightIntensity = lights[index].intensity;
        }
    }

    void updateEditor() {


    }

    void renderEditor() {
    #ifdef _DEBUG
        Debug::begin();

        Debug::end();
    #endif
    }
#endif

    void setRoomParams(const TR::Room &room, float intensity) {
        if (Core::pass == Core::passShadow)
            return;

        if (room.flags.water) {
            Core::color = vec4(0.6f, 0.9f, 0.9f, intensity);
            Core::active.shader->setParam(uCaustics, 1);
        } else {
            Core::color = vec4(1.0f, 1.0f, 1.0f, intensity);
            Core::active.shader->setParam(uCaustics, 0);
        }
        Core::active.shader->setParam(uColor, Core::color);

        waterCaustics->bind(sReflect);
        vec4 roomSize = vec4(room.info.x, room.info.z, room.info.x + room.xSectors * 1024, room.info.z + room.zSectors * 1024);
        Core::active.shader->setParam(uRoomSize, roomSize);
    }

    void renderRoom(int roomIndex, int from = -1) {
        ASSERT(roomIndex >= 0 && roomIndex < level.roomsCount);
        PROFILE_MARKER("ROOM");

        TR::Room &room = level.rooms[roomIndex];
        vec3 offset = vec3(float(room.info.x), 0.0f, float(room.info.z));

    // room geometry & sprites
        if (!room.flags.rendered) { // skip if already rendered
            room.flags.rendered = true;

            if (Core::pass != Core::passShadow) {
                setRoomParams(room, intensityf(room.ambient));

                Basis qTemp = Core::basis;
                Core::basis.translate(offset);

                Shader *sh = Core::active.shader;
                sh->setParam(uType, Shader::ROOM);
                sh->setParam(uBasis, Core::basis);

            // render room geometry
                mesh->renderRoomGeometry(roomIndex);

            // render room sprites
                if (mesh->hasRoomSprites(roomIndex)) {
                    Core::color.w = 1.0;
                    sh->setParam(uType,  Shader::SPRITE);
                    sh->setParam(uColor, Core::color);
                    mesh->renderRoomSprites(roomIndex);
                }

                Core::basis = qTemp;
            }
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
            if (frustum.clipByPortal(v, 4, p.normal))
                renderRoom(p.roomIndex, roomIndex);
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
        setRoomParams(room, isModel ? controller->specular : intensityf(lum));

        if (isModel) { // model
            vec3 pos = controller->getPos();
            AmbientCache::Cube cube;
            if (Core::frameIndex != controller->frameIndex) {
                ambientCache->getAmbient(entity.room, pos, cube);
                if (cube.status == AmbientCache::Cube::READY)
                    memcpy(controller->ambient, cube.colors, sizeof(cube.colors)); // store last calculated ambient into controller
            }
            getLight(pos, entity.room);
            Core::active.shader->setParam(uType, Shader::ENTITY);
            Core::active.shader->setParam(uAmbient, controller->ambient[0], 6);
        } else { // sprite
            Core::active.shader->setParam(uType, Shader::SPRITE);
            Core::lightPos[0]   = vec3(0);
            Core::lightColor[0] = vec4(0, 0, 0, 1);
        }        
        
        Core::active.shader->setParam(uLightPos,   Core::lightPos[0],   MAX_LIGHTS);
        Core::active.shader->setParam(uLightColor, Core::lightColor[0], MAX_LIGHTS);

        controller->render(camera->frustum, mesh);
    }

    void update() {
        time += Core::deltaTime;
        waterTimer += Core::deltaTime;

        for (int i = 0; i < level.entitiesCount; i++) 
            if (level.entities[i].type != TR::Entity::NONE) {
                Controller *controller = (Controller*)level.entities[i].controller;
                if (controller) 
                    controller->update();
            }
        
        camera->update();
    }

    void setup() {
        PROFILE_MARKER("SETUP");

        camera->setup(Core::pass == Core::passCompose);

        atlas->bind(sDiffuse);
        cube->bind(sEnvironment);

        if (!Core::support.VAO)
            mesh->bind();

        // set frame constants for all shaders
        Shader *sh = Core::active.shader;
        sh->bind();
        sh->setParam(uViewProj,         Core::mViewProj);
        sh->setParam(uLightProj,        Core::mLightProj);
        sh->setParam(uViewInv,          Core::mViewInv);
        sh->setParam(uViewPos,          Core::viewPos);
        sh->setParam(uParam,            vec4(time, 0.0f, clipSign, clipHeight));
        sh->setParam(uLightsCount,      3);
        sh->setParam(uAnimTexRanges,    mesh->animTexRanges[0],     mesh->animTexRangesCount);
        sh->setParam(uAnimTexOffsets,   mesh->animTexOffsets[0],    mesh->animTexOffsetsCount);

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
        Core::active.shader->setParam(uLightColor, Core::lightColor[0], MAX_LIGHTS);
        Core::active.shader->setParam(uLightPos,   Core::lightPos[0],   MAX_LIGHTS);

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

    void setPassShader(Core::Pass pass) {
        Core::pass = pass;
        Shader *sh = NULL;
        switch (pass) {
            case Core::passCompose : sh = shaders[shCompose]; break;
            case Core::passShadow  : sh = shaders[shShadow];  break;
            case Core::passAmbient : sh = shaders[shAmbient]; break;
            case Core::passFilter  : sh = shaders[shFilter];  break;
            case Core::passWater   : sh = shaders[shWater];   break;
        }
        ASSERT(sh);
        sh->bind();
    }

    void renderEnvironment(int roomIndex, const vec3 &pos, Texture **targets, int stride = 0) {
        PROFILE_MARKER("ENVIRONMENT");
    // first pass render level into cube faces
        for (int i = 0; i < 6; i++) {
            setupCubeCamera(pos, i);
            setPassShader(Core::passAmbient);
            Core::setBlending(bmAlpha);
            Texture *target = targets[0]->cube ? targets[0] : targets[i * stride];
            Core::setTarget(target, i);
            Core::clear(vec4(0, 0, 0, 1));
            renderScene(roomIndex);
        }
        Core::setTarget(NULL);
    }

    void renderShadows(int roomIndex) {
        PROFILE_MARKER("PASS_SHADOW");
        if (!setupLightCamera()) return;
        shadow->unbind(sShadow);
        setPassShader(Core::passShadow);
        Core::setBlending(bmNone);
	    Core::setTarget(shadow);
	    Core::clear(vec4(1.0));
        Core::setCulling(cfBack);
	    renderScene(roomIndex);
        Core::setCulling(cfFront);
	    Core::setTarget(NULL);
    }

    void renderCompose(int roomIndex) {
        PROFILE_MARKER("PASS_COMPOSE");
        setPassShader(Core::passCompose);

        Core::setBlending(bmAlpha);
        Core::clear(vec4(0.0f));
        shadow->bind(sShadow);
        renderScene(roomIndex);
    }

    void waterSimulate() {
        if (waterTimer < WATER_SIMULATE_TIMESTEP) return;

        Core::active.shader->setParam(uParam, vec4(1.0f / water[0]->width, 1.0f / water[0]->height, 0.0f, 0.0f));
        Core::active.shader->setParam(uType, Shader::WATER_STEP);

        while (waterTimer >= WATER_SIMULATE_TIMESTEP) {
        // water step
            water[0]->bind(sDiffuse);
            Core::setTarget(water[1]);
            mesh->renderQuad();
            swap(water[0], water[1]);
            waterTimer -= WATER_SIMULATE_TIMESTEP;
        }
        
    // calc normals
        //water[0]->bind(sDiffuse);
        //Core::setTarget(water[1]);
        //Core::active.shader->setParam(uType, Shader::WATER_NORMAL);
        //mesh->renderQuad();
        //swap(water[0], water[1]);
        
    // calc caustics
        Core::active.shader->setParam(uScale, 1.0f / PLANE_DETAIL);
        water[0]->bind(sNormal);
        waterCaustics->unbind(sReflect);
        Core::setTarget(waterCaustics);
        Core::active.shader->setParam(uType, Shader::WATER_CAUSTICS);
        mesh->renderPlane();
        Core::active.shader->setParam(uScale, 1.0f);
    }

    void waterDrop() {
        static vec3 lastPos = vec3(0.0);
        vec3 head = lara->animation.getJoints(lara->getMatrix(), 14).pos;
        bool flag = (head - lastPos).length() > 16.0f;
        bool fall = (lara->animation.index == Lara::ANIM_WATER_FALL || lara->animation.index == 152) && lara->animation.frameIndex == 0;
        bool flag2 = lara->animation.frameIndex == 20 || fall;
        flag  &= lara->stand == Lara::STAND_ONWATER || fall;
        flag2 &= lara->stand == Lara::STAND_ONWATER || fall;

        if (Input::down[ikU] || flag || flag2) {
            vec2 p(randf(), randf());
            if (flag || flag2) {
                vec3 c(40448, 0.0, 60928);
                p.x = (head.x - c.x) / 5120.0f + 0.5;
                p.y = (head.z - c.z) / 5120.0f + 0.5;
                lastPos = head;
            }
            float radius = (flag2 ? 1.0f : randf() + 0.2f);
            float strength = flag2 ? 0.01f : randf() * 0.03f;

            if (fall) {
                radius = 1.5f;
                strength = 0.05f;
            }

            water[0]->bind(sDiffuse);
            Core::setTarget(water[1]);
            Core::active.shader->setParam(uType, Shader::WATER_DROP);
            Core::active.shader->setParam(uParam, vec4(p.x, p.y, 128.0f / 5120.0f * radius, strength));
            mesh->renderQuad();
            swap(water[0], water[1]);
            Input::down[ikU] = false;
        }
    }

    void renderWater() {
        bool underwater = level.rooms[camera->getRoomIndex()].flags.water;

    // mask underwater geometry by zero alpha
        setPassShader(Core::passWater);
        atlas->bind(sNormal);
        atlas->bind(sReflect);

        Core::active.shader->setParam(uType,     Shader::WATER_MASK);
        Core::active.shader->setParam(uViewProj, Core::mViewProj);
        Core::active.shader->setParam(uScale,    1.0f);
        Core::setBlending(bmNone);
        Core::setCulling(cfNone);
        glDepthMask(false);
        glColorMask(false, false, false, true);
        mesh->renderQuad();
        glColorMask(true, true, true, true);
        glDepthMask(true);

    // get refraction texture
        checkRefractTexture();
        waterRefract->bind(sDiffuse);
        glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, Core::width, Core::height);

    // render mirror reflection
        Core::setTarget(waterReflect);
        vec3 p = vec3(40448, 3584, 60928);
        vec3 n = vec3(0, 1, 0);

        vec4 reflectPlane = vec4(n.x, n.y, n.z, -n.dot(p));

        camera->reflectPlane = &reflectPlane;
        Core::setCulling(cfBack);
        clipSign   = underwater ? -1.0f : 1.0f;
        clipHeight = 3584.0 * clipSign;
        renderCompose(underwater ? 13 : lara->getRoomIndex());
        clipHeight = 1000000.0f;
        clipSign   = 1.0f;
        Core::setCulling(cfFront);
        camera->reflectPlane = NULL;


    // simulate water
        glDisable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);

        setPassShader(Core::passWater);
        waterDrop();
        waterSimulate();
        Core::setTarget(NULL);

        glEnable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);

    // render water plane
        int dx, dz;
        TR::Room::Sector &s = level.getSector(lara->getRoomIndex(), int(lara->pos.x), int(lara->pos.z), dx, dz);
        if (s.roomAbove != 0xFF && level.rooms[s.roomAbove].lightsCount) {
            TR::Room::Light &light = level.rooms[s.roomAbove].lights[0];
            Core::lightPos[0] = vec3(float(light.x), float(light.y), float(light.z));
            float lum = intensityf(light.intensity);
            Core::lightColor[0] = vec4(lum, lum, lum, float(light.radius) * float(light.radius));
        }

        Core::active.shader->setParam(uType,        Shader::WATER_COMPOSE);
        Core::active.shader->setParam(uViewProj,    Core::mViewProj);
        Core::active.shader->setParam(uViewPos,     Core::viewPos);
        Core::active.shader->setParam(uLightPos,    Core::lightPos[0],   1);
        Core::active.shader->setParam(uLightColor,  Core::lightColor[0], 1);
        Core::active.shader->setParam(uParam, vec4(float(Core::width) / waterRefract->width, float(Core::height) / waterRefract->height, 0.075f, 0.02f));
//        Core::active.shader->setParam(uParam, vec4(1.0f, 1.0f, 0.075f, 0.02f));
        Core::active.shader->setParam(uScale, 1.0f);

        waterRefract->bind(sDiffuse);
        waterReflect->bind(sReflect);
        water[0]->bind(sNormal);
        Core::setCulling(cfNone);
        mesh->renderQuad();
        Core::setCulling(cfFront);
    }

    void render() {
        clipHeight = 1000000.0f;
        clipSign   = 1.0f;
        Core::resetStates();
        
        ambientCache->precessQueue();
        renderShadows(lara->getRoomIndex());
        Core::setViewport(0, 0, Core::width, Core::height);
        renderCompose(camera->getRoomIndex());

        renderWater();

//        Core::mViewInv = camera->mViewInv;
//        Core::mView = Core::mViewInv.inverse();

    #ifdef _DEBUG
        camera->setup(true);
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
                level.entities[lastEntity].controller = new Controller(&level, lastEntity);
            }
        } else
            lastStateK = false;

        if (lastEntity > -1)
            renderEntity(level.entities[lastEntity]);
//        renderModel(level.models[modelIndex], level.entities[4]);

        Debug::begin();
            
            glMatrixMode(GL_MODELVIEW);
            glPushMatrix();
            glLoadIdentity();
            glMatrixMode(GL_PROJECTION);
            glPushMatrix();
            glLoadIdentity();
            glOrtho(0, Core::width, 0, Core::height, 0, 1);

            waterCaustics->bind(sDiffuse);
            glEnable(GL_TEXTURE_2D);
            glDisable(GL_CULL_FACE);
            glDisable(GL_DEPTH_TEST);

            glColor3f(1, 1, 1);
            glBegin(GL_QUADS);
                glTexCoord2f(0, 0); glVertex2f(0, 0);
                glTexCoord2f(1, 0); glVertex2f(256, 0);
                glTexCoord2f(1, 1); glVertex2f(256, 256);
                glTexCoord2f(0, 1); glVertex2f(0, 256);
            glEnd();

            glDisable(GL_TEXTURE_2D);
            glEnable(GL_CULL_FACE);
            glEnable(GL_DEPTH_TEST);

            glMatrixMode(GL_PROJECTION);
            glPopMatrix();
            glMatrixMode(GL_MODELVIEW);
            glPopMatrix();
            

        //    Debug::Level::rooms(level, lara->pos, lara->getEntity().room);
        //    Debug::Level::lights(level, lara->getRoomIndex());
        //    Debug::Level::sectors(level, lara->getRoomIndex(), (int)lara->pos.y);
            Debug::Level::portals(level);
        //    Debug::Level::meshes(level);
        //    Debug::Level::entities(level);
        /*
            static int dbg_ambient = 0;
            dbg_ambient = int(time * 2) % 4;

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