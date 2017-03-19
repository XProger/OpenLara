#ifndef H_CACHE
#define H_CACHE

#include "core.h"
#include "format.h"
#include "controller.h"
#include "camera.h"

#define NO_CLIP_PLANE  1000000.0f

#define FOG_DIST       (18 * 1024)
#define WATER_FOG_DIST (8 * 1024)
//#define WATER_USE_GRID
#define UNDERWATER_COLOR "#define UNDERWATER_COLOR vec3(0.6, 0.9, 0.9)\n"

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

struct ShaderCache {
    enum Effect { FX_NONE = 0, FX_UNDERWATER = 1, FX_ALPHA_TEST = 2, FX_CLIP_PLANE = 4 };

    IGame  *game;
    Shader *shaders[Core::passMAX][Shader::MAX][(FX_UNDERWATER | FX_ALPHA_TEST | FX_CLIP_PLANE) + 1];

    ShaderCache(IGame *game) : game(game) {
        memset(shaders, 0, sizeof(shaders));

        LOG("shader: cache warm up...\n");
        if (Core::settings.shadows)
            compile(Core::passShadow, Shader::ENTITY, FX_NONE);

        if (Core::settings.ambient) {
            compile(Core::passAmbient, Shader::ROOM,   FX_NONE);
            compile(Core::passAmbient, Shader::ROOM,   FX_ALPHA_TEST);
            compile(Core::passAmbient, Shader::ROOM,   FX_CLIP_PLANE);
            compile(Core::passAmbient, Shader::ROOM,   FX_ALPHA_TEST | FX_CLIP_PLANE);
            compile(Core::passAmbient, Shader::ROOM,   FX_UNDERWATER);
            compile(Core::passAmbient, Shader::ROOM,   FX_UNDERWATER | FX_CLIP_PLANE);
            compile(Core::passAmbient, Shader::SPRITE, FX_ALPHA_TEST);
            compile(Core::passAmbient, Shader::SPRITE, FX_ALPHA_TEST | FX_CLIP_PLANE);
        }

        if (Core::settings.water) {
            compile(Core::passWater, Shader::WATER_MASK,     FX_NONE);
            compile(Core::passWater, Shader::WATER_STEP,     FX_NONE);
            compile(Core::passWater, Shader::WATER_CAUSTICS, FX_NONE);
            compile(Core::passWater, Shader::WATER_COMPOSE,  FX_NONE);
            compile(Core::passWater, Shader::WATER_DROP,     FX_NONE);
        }

        compile(Core::passFilter, Shader::FILTER_DOWNSAMPLE, FX_NONE);

        compile(Core::passCompose, Shader::ROOM,   FX_NONE);
        compile(Core::passCompose, Shader::ROOM,   FX_ALPHA_TEST);
        compile(Core::passCompose, Shader::ROOM,   FX_ALPHA_TEST | FX_CLIP_PLANE);
        compile(Core::passCompose, Shader::ROOM,   FX_CLIP_PLANE);
        compile(Core::passCompose, Shader::ROOM,   FX_UNDERWATER);
        compile(Core::passCompose, Shader::ROOM,   FX_UNDERWATER | FX_CLIP_PLANE);
        compile(Core::passCompose, Shader::ENTITY, FX_NONE);
        compile(Core::passCompose, Shader::ENTITY, FX_CLIP_PLANE);
        compile(Core::passCompose, Shader::ENTITY, FX_UNDERWATER);
        compile(Core::passCompose, Shader::ENTITY, FX_UNDERWATER | FX_CLIP_PLANE);
        compile(Core::passCompose, Shader::ENTITY, FX_ALPHA_TEST);
        compile(Core::passCompose, Shader::ENTITY, FX_ALPHA_TEST | FX_CLIP_PLANE);
        compile(Core::passCompose, Shader::SPRITE, FX_ALPHA_TEST);
        compile(Core::passCompose, Shader::SPRITE, FX_ALPHA_TEST | FX_CLIP_PLANE);
        compile(Core::passCompose, Shader::SPRITE, FX_UNDERWATER | FX_ALPHA_TEST);
        compile(Core::passCompose, Shader::SPRITE, FX_UNDERWATER | FX_ALPHA_TEST | FX_CLIP_PLANE);
        compile(Core::passCompose, Shader::FLASH,  FX_ALPHA_TEST);
        compile(Core::passCompose, Shader::MIRROR, FX_NONE);

        LOG("shader: cache is ready\n");
    }

    ~ShaderCache() {
        for (int pass = 0; pass < Core::passMAX; pass++)
            for (int type = 0; type < Shader::MAX; type++)
                for (int fx = 0; fx < sizeof(shaders[Core::passMAX][Shader::MAX]) / sizeof(shaders[Core::passMAX][Shader::MAX][FX_NONE]); fx++)
                    delete shaders[pass][type][fx];
    }

    Shader* compile(Core::Pass pass, Shader::Type type, int fx) {
        char def[1024], ext[255];
        ext[0] = 0;
        if (Core::settings.shadows) {
		    if (Core::support.shadowSampler) {
			    #ifdef MOBILE
				    strcat(ext, "#extension GL_EXT_shadow_samplers : require\n");
			    #endif
			    strcat(ext, "#define SHADOW_SAMPLER\n");
		    } else {
			    if (Core::support.depthTexture)
				    strcat(ext, "#define SHADOW_DEPTH\n");
			    else
				    strcat(ext, "#define SHADOW_COLOR\n");
            }
        }

        const char *passNames[] = { "COMPOSE", "SHADOW", "AMBIENT", "WATER", "FILTER", "GUI" };
        const char *src = NULL;
        const char *typ = NULL;
        switch (pass) {
            case Core::passCompose :
            case Core::passShadow  :
            case Core::passAmbient : {
                static const char *typeNames[] = { "SPRITE", "FLASH", "ROOM", "ENTITY", "MIRROR" };
                src = SHADER;
                typ = typeNames[type];
                int animTexRangesCount  = game->getMesh()->animTexRangesCount;
                int animTexOffsetsCount = game->getMesh()->animTexOffsetsCount;
                sprintf(def, "%s#define PASS_%s\n#define TYPE_%s\n#define MAX_LIGHTS %d\n#define MAX_RANGES %d\n#define MAX_OFFSETS %d\n#define FOG_DIST (1.0/%d.0)\n#define WATER_FOG_DIST (1.0/%d.0)\n", ext, passNames[pass], typ, MAX_LIGHTS, animTexRangesCount, animTexOffsetsCount, FOG_DIST, WATER_FOG_DIST);
                if (fx & FX_UNDERWATER) strcat(def, "#define UNDERWATER\n" UNDERWATER_COLOR);
                if (fx & FX_ALPHA_TEST) strcat(def, "#define ALPHA_TEST\n");
                if (fx & FX_CLIP_PLANE) strcat(def, "#define CLIP_PLANE\n");
                if (Core::settings.ambient)  strcat(def, "#define OPT_AMBIENT\n");
                if (Core::settings.lighting) strcat(def, "#define OPT_LIGHTING\n");
                if (Core::settings.shadows)  strcat(def, "#define OPT_SHADOW\n");
                if (Core::settings.water)    strcat(def, "#define OPT_WATER\n");
                break;
            }
            case Core::passWater   : {
                static const char *typeNames[] = { "DROP", "STEP", "CAUSTICS", "MASK", "COMPOSE" };
                src = WATER;
                typ = typeNames[type];
                sprintf(def, "%s#define PASS_%s\n#define WATER_%s\n#define WATER_FOG_DIST (1.0/%d.0)\n" UNDERWATER_COLOR, ext, passNames[pass], typ, WATER_FOG_DIST);
                #ifdef WATER_USE_GRID
                    strcat(def, "#define WATER_USE_GRID\n");
                #endif
                break;
            }
            case Core::passFilter  : {
                static const char *typeNames[] = { "DOWNSAMPLE" };
                src = FILTER;
                typ = typeNames[type];
                sprintf(def, "%s#define PASS_%s\n#define FILTER_%s\n", ext, passNames[pass], typ);
                break;
            }
            case Core::passGUI : {
                static const char *typeNames[] = { "DEFAULT" };
                src = GUI;
                typ = typeNames[type];
                sprintf(def, "%s#define PASS_%s\n", ext, passNames[pass]);
                break;
            }
            default : ASSERT(false);
        }
        LOG("shader: compile %s -> %s %s%s%s\n", passNames[pass], typ, (fx & FX_UNDERWATER) ? "underwater " : "", (fx & FX_ALPHA_TEST) ? "alphaTest " : "", (fx & FX_CLIP_PLANE) ? "clipPlane" : "");
        return shaders[pass][type][fx] = new Shader(src, def);
    }

    void bind(Core::Pass pass, Shader::Type type, int fx) {
        Core::pass = pass;
        Shader *shader = shaders[pass][type][fx];
        if (!shader)
            shader = compile(pass, type, fx);
        ASSERT(shader != NULL);
        shader->bind();
        // TODO: bindable uniform block
        shader->setParam(uViewProj,       Core::mViewProj);
        shader->setParam(uLightProj,      Core::mLightProj);
        shader->setParam(uViewInv,        Core::mViewInv);
        shader->setParam(uViewPos,        Core::viewPos);
        shader->setParam(uParam,          Core::params);
        MeshBuilder *mesh = game->getMesh();
        shader->setParam(uAnimTexRanges,  mesh->animTexRanges[0],  mesh->animTexRangesCount);
        shader->setParam(uAnimTexOffsets, mesh->animTexOffsets[0], mesh->animTexOffsetsCount);
    }

};

struct AmbientCache {
    IGame     *game;
    TR::Level *level;

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

    AmbientCache(IGame *game) : game(game), level(game->getLevel()), tasksCount(0) {
        items   = NULL;
        offsets = new int[level->roomsCount];
        int sectors = 0;
        for (int i = 0; i < level->roomsCount; i++) {
            TR::Room &r = level->rooms[i];
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
                
        TR::Room &r = level->rooms[room];
        TR::Room::Sector &s = r.sectors[sector];
            
        vec3 pos = vec3(float((sector / r.zSectors) * 1024 + 512 + r.info.x), 
                        float(max((s.floor - 2) * 256, (s.floor + s.ceiling) * 256 / 2)),
                        float((sector % r.zSectors) * 1024 + 512 + r.info.z));

        // first pass - render environment from position (room geometry & static meshes)
        game->renderEnvironment(room, pos, textures, 4);

        // second pass - downsample it
        Core::setDepthTest(false);

        game->setShader(Core::passFilter, Shader::FILTER_DOWNSAMPLE);

        for (int i = 1; i < 4; i++) {
            int size = 64 >> (i << 1);

            Core::active.shader->setParam(uParam, vec4(float(size << 2), 0.0f, 0.0f, 0.0f));

            for (int j = 0; j < 6; j++) {
                Texture *src = textures[j * 4 + i - 1];
                Texture *dst = textures[j * 4 + i];
                Core::setTarget(dst, true);
                src->bind(sDiffuse);
                game->getMesh()->renderQuad();
            }
        }

        // get result color from 1x1 textures
        for (int j = 0; j < 6; j++) {
            Core::setTarget(textures[j * 4 + 3]);

            TR::Color32 color;
            glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &color);
            colors[j] = vec3(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f);
            colors[j] *= colors[j]; // to "linear" space
        }

        Core::setDepthTest(true);
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
        TR::Room &r = level->rooms[room];
            
        int sx = clamp((int(pos.x) - r.info.x) / 1024, 0, r.xSectors - 1);
        int sz = clamp((int(pos.z) - r.info.z) / 1024, 0, r.zSectors - 1);
            
        int sector = sx * r.zSectors + sz;
        Cube *a = getAmbient(room, sector);
        if (a)
            value = *a;
        else
            value.status = Cube::BLANK;
    }
};

struct WaterCache {
    #define MAX_SURFACES       8
    #define MAX_INVISIBLE_TIME 5.0f
    #define SIMULATE_TIMESTEP  (1.0f / 40.0f)
    #define DETAIL             (64.0f / 1024.0f)
    #define MAX_DROPS          32

    IGame     *game;
    TR::Level *level;
    Texture   *refract;
    Texture   *reflect;

    struct Item {
        int     from, to, caust;
        float   timer;
        bool    visible;
        bool    blank;
        vec3    pos, size;
        Texture *mask;
        Texture *caustics;
        Texture *data[2];

        Item() {
            mask = caustics = data[0] = data[1] = NULL;
        }

        Item(int from, int to) : from(from), to(to), caust(to), timer(SIMULATE_TIMESTEP), visible(true), blank(true) {
            mask = caustics = data[0] = data[1] = NULL;
        }

        void init(IGame *game) {

            TR::Level *level = game->getLevel();
            TR::Room &r = level->rooms[to]; // underwater room

            int minX = r.xSectors, minZ = r.zSectors, maxX = 0, maxZ = 0, posY;

            for (int z = 0; z < r.zSectors; z++)
                for (int x = 0; x < r.xSectors; x++) {
                    TR::Room::Sector &s = r.sectors[x * r.zSectors + z];
                    if (s.roomAbove != TR::NO_ROOM && !level->rooms[s.roomAbove].flags.water) {
                        minX = min(minX, x);
                        minZ = min(minZ, z);
                        maxX = max(maxX, x);
                        maxZ = max(maxZ, z);
                        posY = s.ceiling * 256;
                        if (s.roomBelow != TR::NO_ROOM)
                            caust = s.roomBelow;
                    }
                }
            maxX++;
            maxZ++;

            int w = nextPow2(maxX - minX);
            int h = nextPow2(maxZ - minZ);

            uint16 *m = new uint16[w * h];
            memset(m, 0, w * h * sizeof(m[0]));

            for (int z = minZ; z < maxZ; z++)
                for (int x = minX; x < maxX; x++) {
                    TR::Room::Sector &s = r.sectors[x * r.zSectors + z];

                    bool hasWater = (s.roomAbove != TR::NO_ROOM && !level->rooms[s.roomAbove].flags.water);
                    bool hasFlow  = false;
                    if (hasWater) {
                        TR::Level::FloorInfo info;
                        level->getFloorInfo(to, x + r.info.x, r.info.yBottom, z + r.info.z, info);
                        if (info.trigCmdCount && info.trigger == TR::Level::Trigger::ACTIVATE)
                            for (int i = 0; i < info.trigCmdCount; i++)
                                if (info.trigCmd[i].action == TR::Action::FLOW) {
                                    hasFlow = true;
                                    break;
                                }
                    }
                        
                    m[(x - minX) + w * (z - minZ)] = hasWater ? (hasFlow ? 0xF81F : 0xF800) : 0;
                }

            size = vec3(float((maxX - minX) * 512), 1.0f, float((maxZ - minZ) * 512)); // half size
            pos  = vec3(r.info.x + minX * 1024 + size.x, float(posY), r.info.z + minZ * 1024 + size.z);

            data[0]  = new Texture(w * 64, h * 64, Texture::RGBA_HALF, false);
            data[1]  = new Texture(w * 64, h * 64, Texture::RGBA_HALF, false);
            caustics = new Texture(512, 512, Texture::RGB16, false);
            mask     = new Texture(w, h, Texture::RGB16, false, m, false);
            delete[] m;

            blank = false;

        //    Core::setTarget(data[0], true);
        //    Core::invalidateTarget(false, true);
        }

        void free() {
            delete data[0];
            delete data[1];
            delete caustics;
            delete mask;
            mask = caustics = data[0] = data[1] = NULL;
        }

    } items[MAX_SURFACES];
    int count, visible;
    bool checkVisibility;

    int dropCount;
    struct Drop {
        vec3  pos;
        float radius;
        float strength;
        Drop() {}
        Drop(const vec3 &pos, float radius, float strength) : pos(pos), radius(radius), strength(strength) {}
    } drops[MAX_DROPS];

    WaterCache(IGame *game) : game(game), level(game->getLevel()), refract(NULL), count(0), checkVisibility(false), dropCount(0) {
        reflect = new Texture(512, 512, Texture::RGB16, false);
    }

    ~WaterCache() { 
        delete refract;
        delete reflect;
        for (int i = 0; i < count; i++)
            items[i].free();
    }

    void update() {
        int i = 0;
        while (i < count) {
            Item &item = items[i];
            if (item.timer > MAX_INVISIBLE_TIME) {
                items[i].free();
                items[i] = items[--count];
                continue;
            }
            item.timer += Core::deltaTime;
            i++;
        }
    }

    void reset() {
        for (int i = 0; i < count; i++)
            items[i].visible = false;
        visible = 0;
    }

    void setVisible(int roomIndex, int nextRoom = TR::NO_ROOM) {
        if (!checkVisibility) return;

        if (nextRoom == TR::NO_ROOM) { // setVisible(underwaterRoom) for caustics update
            for (int i = 0; i < count; i++)
                if (items[i].caust == roomIndex) {
                    nextRoom = items[i].from;
                    if (!items[i].visible) {
                        items[i].visible = true;
                        visible++;
                    }
                    break;
                }
            return;
        }

        int from, to; // from surface room to underwater room
        if (level->rooms[roomIndex].flags.water) {
            from = nextRoom;
            to   = roomIndex;
        } else {
            from = roomIndex;
            to   = nextRoom;
        }

        for (int i = 0; i < count; i++) {
            Item &item = items[i];
            if (item.from == from && item.to == to) {
                if (!item.visible) {
                    visible++;
                    item.visible = true;
                }
                return;
            }
        }
        if (count == MAX_SURFACES) return;

        items[count++] = Item(from, to);
        visible++;
    }

    void bindCaustics(int roomIndex) {
        Item *item = NULL;
        for (int i = 0; i < count; i++)
            if (items[i].caust == roomIndex) {
                item = &items[i];
                break;
            }

        if (!item || !item->caustics) {                
            Core::blackTex->bind(sReflect);
            Core::active.shader->setParam(uRoomSize, vec4(0.0f));
            game->setWaterParams(-NO_CLIP_PLANE);
        } else {
            item->caustics->bind(sReflect);
            Core::active.shader->setParam(uRoomSize, vec4(item->pos.x - item->size.x, item->pos.z - item->size.z, item->pos.x + item->size.x, item->pos.z + item->size.z));
            game->setWaterParams(item->pos.y);
        }
        game->updateParams();
    }

    void addDrop(const vec3 &pos, float radius, float strength) {
        if (dropCount >= MAX_DROPS) return;
        drops[dropCount++] = Drop(pos, radius, strength);
    }

    void drop(Item &item) { 
        if (!dropCount) return;

        game->setShader(Core::passWater, Shader::WATER_DROP);
        Core::active.shader->setParam(uTexParam, vec4(1.0f / item.data[0]->width, 1.0f / item.data[0]->height, 1.0f, 1.0f));

        vec3 rPosScale[2] = { vec3(0.0f), vec3(1.0f) };
        Core::active.shader->setParam(uPosScale, rPosScale[0], 2);
            
        for (int i = 0; i < dropCount; i++) {
            Drop &drop = drops[i];

            vec3 p;
            p.x = (drop.pos.x - (item.pos.x - item.size.x)) * DETAIL;
            p.z = (drop.pos.z - (item.pos.z - item.size.z)) * DETAIL;
            Core::active.shader->setParam(uParam, vec4(p.x, p.z, drop.radius * DETAIL, drop.strength));

            item.data[0]->bind(sDiffuse);
            Core::setTarget(item.data[1], true);
            Core::setViewport(0, 0, int(item.size.x * DETAIL * 2.0f + 0.5f), int(item.size.z * DETAIL * 2.0f + 0.5f));
            game->getMesh()->renderQuad();
            Core::invalidateTarget(false, true);
            swap(item.data[0], item.data[1]);
        }
    }
    
    void step(Item &item) {
        if (item.timer < SIMULATE_TIMESTEP) return;

        game->setShader(Core::passWater, Shader::WATER_STEP);
        Core::active.shader->setParam(uTexParam, vec4(1.0f / item.data[0]->width, 1.0f / item.data[0]->height, 1.0f, 1.0f));
        Core::active.shader->setParam(uParam, vec4(0.995f, 1.0f, 0, 0));
            
        while (item.timer >= SIMULATE_TIMESTEP) {
        // water step
            item.data[0]->bind(sDiffuse);
            Core::setTarget(item.data[1], 0, true);
            Core::setViewport(0, 0, int(item.size.x * DETAIL * 2.0f + 0.5f), int(item.size.z * DETAIL * 2.0f + 0.5f));
            game->getMesh()->renderQuad();
            Core::invalidateTarget(false, true);
            swap(item.data[0], item.data[1]);
            item.timer -= SIMULATE_TIMESTEP;
        }
        

    // calc caustics
        game->setShader(Core::passWater, Shader::WATER_CAUSTICS);
        vec3 rPosScale[2] = { vec3(0.0f), vec3(1.0f / PLANE_DETAIL) };
        Core::active.shader->setParam(uPosScale, rPosScale[0], 2);

        float sx = item.size.x * DETAIL / (item.data[0]->width  / 2);
        float sz = item.size.z * DETAIL / (item.data[0]->height / 2);

        Core::active.shader->setParam(uTexParam, vec4(1.0f, 1.0f, sx, sz));

        Core::whiteTex->bind(sReflect);
        item.data[0]->bind(sNormal);
        Core::setTarget(item.caustics, true);
        game->getMesh()->renderPlane();
        Core::invalidateTarget(false, true);
    }

    void render() {
        if (!visible) return;

    // mask underwater geometry by zero alpha
        for (int i = 0; i < count; i++) {
            Item &item = items[i];
            if (item.visible && item.blank)
                item.init(game);
        }
        Core::setTarget(NULL);

        game->setShader(Core::passWater, Shader::WATER_MASK);
        Core::active.shader->setParam(uTexParam, vec4(1.0f));

        Core::setColorWrite(false, false, false, true);
        Core::setDepthWrite(false);
        Core::setCulling(cfNone);

        for (int i = 0; i < count; i++) {
            Item &item = items[i];
            if (!item.visible) continue;

            Core::active.shader->setParam(uPosScale, item.pos, 2);

            game->getMesh()->renderQuad();
        }

        Core::setColorWrite(true, true, true, true);
        Core::setDepthWrite(true);
        Core::setCulling(cfFront);

    // get refraction texture
        if (!refract || Core::width != refract->width || Core::height != refract->height) {
            delete refract;
            refract = new Texture(Core::width, Core::height, Texture::RGBA, false);
        }
        Core::copyTarget(refract, 0, 0, 0, 0, Core::width, Core::height); // copy framebuffer into refraction texture

        for (int i = 0; i < count; i++) {
            Item &item = items[i];
            if (!item.visible) continue;

        // render mirror reflection
            Core::setTarget(reflect, true);
            vec3 p = item.pos;
            vec3 n = vec3(0, 1, 0);

            vec4 reflectPlane = vec4(n.x, n.y, n.z, -n.dot(p));

            Camera *camera = (Camera*)game->getCamera();

            bool underwater = level->rooms[camera->getRoomIndex()].flags.water;

            //bool underwater = level->camera->pos.y > item.pos.y;

            camera->reflectPlane = &reflectPlane;
            float sign = underwater ? -1.0f : 1.0f;
            game->setClipParams(sign, item.pos.y * sign);
            game->updateParams();
            game->renderCompose(underwater ? item.from : item.to);
            Core::invalidateTarget(false, true);
            game->setClipParams(1.0f, NO_CLIP_PLANE);
            game->updateParams();

            camera->reflectPlane = NULL;
            camera->setup(true);

        // simulate water
            Core::setDepthTest(false);
            item.mask->bind(sMask);

            if (item.timer >= SIMULATE_TIMESTEP || dropCount) {
            // add water drops
                drop(item);                    
            // simulation step
                step(item);
            }
            Core::setTarget(NULL);

            Core::setDepthTest(true);

        // render water plane
            if (level->rooms[item.from].lightsCount) {
                TR::Room::Light &light = level->rooms[item.from].lights[0];
                Core::lightPos[0] = vec3(float(light.x), float(light.y), float(light.z));
                float lum = intensityf(light.intensity);
                Core::lightColor[0] = vec4(lum, lum, lum, float(light.radius) * float(light.radius));
            }

            game->setShader(Core::passWater, Shader::WATER_COMPOSE);
            Core::active.shader->setParam(uLightPos,    Core::lightPos[0],   1);
            Core::active.shader->setParam(uLightColor,  Core::lightColor[0], 1);
            Core::active.shader->setParam(uParam,       vec4(float(Core::width) / refract->width, float(Core::height) / refract->height, 0.05f, 0.02f));

            float sx = item.size.x * DETAIL / (item.data[0]->width  / 2);
            float sz = item.size.z * DETAIL / (item.data[0]->height / 2);

            Core::active.shader->setParam(uTexParam, vec4(1.0f, 1.0f, sx, sz));

            refract->bind(sDiffuse);
            reflect->bind(sReflect);
            item.data[0]->bind(sNormal);
            Core::setCulling(cfNone);
            #ifdef WATER_USE_GRID
                vec3 rPosScale[2] = { item.pos, item.size * vec3(1.0f / PLANE_DETAIL, 512.0f, 1.0f / PLANE_DETAIL) };
                Core::active.shader->setParam(uPosScale, rPosScale[0], 2);
                game->getMesh()->renderPlane();
            #else
                Core::active.shader->setParam(uPosScale, item.pos, 2);
                game->getMesh()->renderQuad();
            #endif
            Core::setCulling(cfFront);
        }
        dropCount = 0;
    }

    #undef MAX_WATER_SURFACES
    #undef MAX_WATER_INVISIBLE_TIME
    #undef WATER_SIMULATE_TIMESTEP
    #undef DETAIL
};

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
};
*/

#undef UNDERWATER_COLOR

#endif