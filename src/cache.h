#ifndef H_CACHE
#define H_CACHE

#include "core.h"
#include "format.h"
#include "controller.h"
#include "camera.h"

#define NO_WATER_HEIGHT  1000000.0f

#if defined(_OS_IOS) || defined(_GAPI_D3D9) || defined(_GAPI_D3D11) || defined(_GAPI_GXM)
    #define USE_SCREEN_TEX
#endif

struct AmbientCache {
    IGame     *game;
    TR::Level *level;

    struct Cube {
        enum int32 {
            BLANK, WAIT, READY
        }    status;
        vec4 colors[6];
    } *items;
    int *offsets;

    struct Task {
        int  room;
        int  flip;
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
            sectors += r.xSectors * r.zSectors * (r.alternateRoom > -1 ? 2 : 1); // x2 for flipped rooms
        }
    // init cache buffer
        items = new Cube[sectors];
        memset(items, 0, sizeof(Cube) * sectors);
    // init downsample textures
        for (int j = 0; j < 6; j++)
            for (int i = 0; i < 4; i++)
                textures[j * 4 + i] = new Texture(64 >> (i << 1), 64 >> (i << 1), 1, FMT_RGBA, OPT_TARGET | OPT_NEAREST);
    }

    ~AmbientCache() {
        delete[] items;
        delete[] offsets;
        for (int i = 0; i < 6 * 4; i++)
            delete textures[i];
    }

    void addTask(int room, int sector) {
        if (tasksCount >= COUNT(tasks)) return;

        Task &task  = tasks[tasksCount++];
        task.room   = room;
        task.flip   = level->state.flags.flipped && level->rooms[room].alternateRoom > -1;
        task.sector = sector;
        task.cube   = &items[offsets[room] + sector];
        task.cube->status = Cube::WAIT;
    }

    void renderAmbient(int room, int sector, vec4 *colors) {
        PROFILE_MARKER("Ambient");
                
        TR::Room &r = level->rooms[room];
        TR::Room::Sector &s = r.sectors[sector];
            
        vec3 pos = vec3(float((sector / r.zSectors) * 1024 + 512 + r.info.x), 
                        float(max((s.floor - 2) * 256, (s.floor + s.ceiling) * 256 / 2)),
                        float((sector % r.zSectors) * 1024 + 512 + r.info.z));

        Core::setClearColor(vec4(0, 0, 0, 1));

        // first pass - render environment from position (room geometry & static meshes)
        game->renderEnvironment(room, pos, textures, 4);

        // second pass - downsample it
        mat4 mProj, mView;
        mView.identity();
        mProj = GAPI::ortho(-1, +1, -1, +1, 0, 1);
        mProj.scale(vec3(1.0f / 32767.0f));
        Core::setViewProj(mView, mProj);

        for (int i = 1; i < 4; i++) {
            int size = 64 >> (i << 1);

            for (int j = 0; j < 6; j++) {
                Texture *src = textures[j * 4 + i - 1];
                Texture *dst = textures[j * 4 + i];

                Core::beginRenderPass(RP_FILTER, RenderTarget(dst, 0, 0), RenderTarget(NULL, 0, 0));
                Core::setPipelineState(PS_FILTER_DOWNSAMPLE);
                Core::active.shader->setParam(uParam, vec4(1.0f / (size << 2), 0.0f, 0.0f, 0.0f));

                src->bind(sDiffuse);
                game->getMesh()->renderQuad();

                Core::endRenderPass();
            }
        }

        // get result color from 1x1 textures
        for (int j = 0; j < 6; j++) {
            colors[j] = Core::copyPixel(textures[j * 4 + 3], 0, 0);
        }

        Core::setClearColor(vec4(0, 0, 0, 0));
    }

    void processQueue() {
        game->setupBinding();

        for (int i = 0; i < tasksCount; i++) {
            Task &task = tasks[i];
            
            bool needFlip = task.flip != level->state.flags.flipped;
           
            if (needFlip) game->flipMap(false);

            int sector = task.sector;
            if (task.flip) {
                TR::Room &r = level->rooms[task.room];
                sector -= r.xSectors * r.zSectors;
            }

            renderAmbient(task.room, sector, &task.cube->colors[0]);
            if (needFlip) game->flipMap(false);

            task.cube->status = Cube::READY;
        }
        tasksCount = 0;
    }

    Cube* getAmbient(int roomIndex, int x, int z) {
        TR::Room &r = level->rooms[roomIndex];

        int sx = clamp(x / 1024, 0, r.xSectors - 1);
        int sz = clamp(z / 1024, 0, r.zSectors - 1);
        int sector = sx * r.zSectors + sz;

        if (r.sectors[sector].floor == TR::NO_FLOOR)
            return NULL;

        if (level->state.flags.flipped && r.alternateRoom > -1)
            sector += r.xSectors * r.zSectors;

        Cube *cube = &items[offsets[roomIndex] + sector];
        if (cube->status == Cube::BLANK)
            addTask(roomIndex, sector);

        return cube->status == Cube::READY ? cube : NULL;
    }

    void lerpCubes(Cube &result, const Cube *a, const Cube *b, float t) {
        ASSERT(a != NULL && b != NULL);
        result.colors[0] = a->colors[0].lerp(b->colors[0], t);
        result.colors[1] = a->colors[1].lerp(b->colors[1], t);
        result.colors[2] = a->colors[2].lerp(b->colors[2], t);
        result.colors[3] = a->colors[3].lerp(b->colors[3], t);
        result.colors[4] = a->colors[4].lerp(b->colors[4], t);
        result.colors[5] = a->colors[5].lerp(b->colors[5], t);
    }

    void getAmbient(int room, const vec3 &pos, Cube &value) {
        TR::Room &r = level->rooms[room];
        
        int x = int(pos.x) - r.info.x;
        int z = int(pos.z) - r.info.z;

        // cc cx
        // cz cd

        Cube *cc = getAmbient(room, x, z);
        if (cc && cc->status == Cube::READY) {
            Cube *cx = NULL, *cz = NULL, *cd = NULL;

            int sx = (x / 1024) * 1024 + 512;
            int sz = (z / 1024) * 1024 + 512;

            int ox = sx + sign(x - sx) * 1024;
            int oz = sz + sign(z - sz) * 1024;

            float tx, tz;
            tx = abs(x - sx) / 1024.0f;
            tz = abs(z - sz) / 1024.0f;

            cx = getAmbient(room, ox, sz);
            cz = getAmbient(room, sx, oz);
            cd = getAmbient(room, ox, oz);

            if (cx != NULL && cx->status != Cube::READY) cx = cc;
            if (cz != NULL && cz->status != Cube::READY) cz = cc;
            if (cd != NULL && cd->status != Cube::READY) cd = cc;

            Cube lx, lz;
            if (cd != NULL && cx != NULL && cz != NULL) {
                lerpCubes(lx, cc, cx, tx);
                lerpCubes(lz, cz, cd, tx);
                lerpCubes(value, &lx, &lz, tz);
            } else if (cd != NULL && cx != NULL) {
                lerpCubes(lx, cc, cx, tx);
                lerpCubes(value, &lx, cd, tz);
            } else if (cd != NULL && cz != NULL) {
                lerpCubes(lz, cc, cz, tz);
                lerpCubes(value, &lz, cd, tx);
            } else if (cx != NULL && cz != NULL) {
                lerpCubes(lx, cc, cx, tx);
                lerpCubes(value, &lx, cz, tz);
            } else if (cx != NULL) {
                lerpCubes(value, cc, cx, tx);
            } else if (cz != NULL) {
                lerpCubes(value, cc, cz, tz);
            } else
                value = *cc;

            value.status = cc->status;
        } else
            value.status = Cube::BLANK;
    }
};

struct WaterCache {
    #define MAX_SURFACES       16
    #define MAX_INVISIBLE_TIME 5.0f
    #define SIMULATE_TIMESTEP  (1.0f / 40.0f)
    #define WATER_TILE_SIZE    64
    #define DETAIL             (WATER_TILE_SIZE / 1024.0f)
    #define MAX_DROPS          32

    IGame     *game;
    TR::Level *level;
    Texture   *screen;
    Texture   *refract;
    Texture   *reflect;

    struct Item {
        int     from, to, caust;
        float   timer;
        bool    flip;
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
            ASSERT(r.flags.water);
            int minX = r.xSectors, minZ = r.zSectors, maxX = 0, maxZ = 0;
            
            int posY = level->rooms[to].waterLevelSurface;
            if (posY == TR::NO_WATER)
                posY = level->rooms[from].waterLevelSurface;

            ASSERT(posY != -1); // underwater room without reaching the surface
            
            int caustY = posY;
            
            for (int z = 0; z < r.zSectors; z++)
                for (int x = 0; x < r.xSectors; x++) {
                    TR::Room::Sector &s = r.sectors[x * r.zSectors + z];
                    if (s.roomAbove != TR::NO_ROOM && !level->rooms[s.roomAbove].flags.water) {
                        minX = min(minX, x);
                        minZ = min(minZ, z);
                        maxX = max(maxX, x);
                        maxZ = max(maxZ, z);
                        if (s.roomBelow != TR::NO_ROOM) {
                            int16 caustRoom = s.roomBelow;
                            int floor = int(level->getFloor(&s, vec3(float(r.info.x + x * 1024), float(posY), float(r.info.z + z * 1024)), &caustRoom));
                            if (floor > caustY) {
                                caustY = floor;
                                caust  = caustRoom;
                            }
                        }
                    }
                }
            maxX++;
            maxZ++;

            int w = maxX - minX;
            int h = maxZ - minZ;

            uint8 *m = new uint8[w * h];
            memset(m, 0, w * h * sizeof(m[0]));

            for (int z = minZ; z < maxZ; z++)
                for (int x = minX; x < maxX; x++) {
                    TR::Room::Sector &s = r.sectors[x * r.zSectors + z];

                    bool hasWater = s.roomAbove != TR::NO_ROOM && !level->rooms[s.roomAbove].flags.water;
                    if (hasWater) {
                        TR::Room &rt = level->rooms[s.roomAbove];
                        int xt = int(r.info.x + x * 1024 - rt.info.x) / 1024;
                        int zt = int(r.info.z + z * 1024 - rt.info.z) / 1024;
                        TR::Room::Sector &st = rt.sectors[xt * rt.zSectors + zt];
                        hasWater = s.ceiling > st.ceiling;
                        if (s.ceiling == st.ceiling) {
                            vec3 p = vec3(float(r.info.x + x * 1024 + 512), float(posY), float(r.info.z + z * 1024 + 512));
                            hasWater = (s.ceiling * 256 - level->getCeiling(&st, p)) > 8.0f;
                        }
                    }

                    m[(x - minX) + w * (z - minZ)] = hasWater ? 0xFF : 0x00; // TODO: flow map
                }
            mask = new Texture(w, h, 1, FMT_LUMINANCE, OPT_NEAREST, m);
            delete[] m;

            size = vec3(float((maxX - minX) * 512), 1.0f, float((maxZ - minZ) * 512)); // half size
            pos  = vec3(r.info.x + minX * 1024 + size.x, float(posY), r.info.z + minZ * 1024 + size.z);

            int *mf = new int[4 * w * h * SQR(WATER_TILE_SIZE)];
            memset(mf, 0, sizeof(int) * 4 * w * h * SQR(WATER_TILE_SIZE));
            data[0] = new Texture(w * WATER_TILE_SIZE, h * WATER_TILE_SIZE, 1, FMT_RG_HALF, OPT_TARGET | OPT_VERTEX, mf);
            data[1] = new Texture(w * WATER_TILE_SIZE, h * WATER_TILE_SIZE, 1, FMT_RG_HALF, OPT_TARGET | OPT_VERTEX);
            delete[] mf;

            caustics = Core::settings.detail.water > Core::Settings::MEDIUM ? new Texture(512, 512, 1, FMT_RGBA, OPT_TARGET | OPT_DEPEND) : NULL;
            
            blank = false;
        }

        void deinit() {
            delete data[0];
            delete data[1];
            delete caustics;
            delete mask;
            mask = caustics = data[0] = data[1] = NULL;
        }

    } items[MAX_SURFACES];
    int count, visible;

    int dropCount;
    struct Drop {
        vec3  pos;
        float radius;
        float strength;
        Drop() {}
        Drop(const vec3 &pos, float radius, float strength) : pos(pos), radius(radius), strength(strength) {}
    } drops[MAX_DROPS];

    WaterCache(IGame *game) : game(game), level(game->getLevel()), screen(NULL), refract(NULL), count(0), dropCount(0) {
        reflect = new Texture(512, 512, 1, FMT_RGBA, OPT_TARGET);
    }

    ~WaterCache() {
        delete screen;
        delete refract;
        delete reflect;
        for (int i = 0; i < count; i++)
            items[i].deinit();
    }

    void update() {
        int i = 0;
        while (i < count) {
            Item &item = items[i];
            if (item.timer > MAX_INVISIBLE_TIME) {
                items[i].deinit();
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

    void flipMap() {
        for (int i = 0; i < level->roomsCount && count; i++)
            if (level->rooms[i].alternateRoom > -1) {
                int j = 0;
                while (j < count) {
                    if (items[j].from == i || items[j].to == i) {
                        items[j].deinit();
                        items[j] = items[--count];
                    } else
                        j++;
                }
            }
    }

    void setVisible(int roomIndex, int nextRoom = TR::NO_ROOM) {
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

        if (level->rooms[to].waterLevelSurface == TR::NO_WATER && level->rooms[from].waterLevelSurface == TR::NO_WATER) // not have water surface
            return;

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

        if (item && item->caustics) {
            item->caustics->bind(sReflect);
            Core::active.shader->setParam(uRoomSize, vec4(item->pos.x - item->size.x, item->pos.z - item->size.z, item->size.x * 2.0f, item->size.z * 2.0f));
            game->setWaterParams(item->pos.y);
        } else {
            Core::active.shader->setParam(uRoomSize, vec4(0, 0, 1, 1));
            Core::blackTex->bind(sReflect);
        }
    }

    void addDrop(const vec3 &pos, float radius, float strength) {
        if (dropCount >= MAX_DROPS) return;
        drops[dropCount++] = Drop(pos, radius, strength);
    }

    void drop(Item &item) { 
        if (!dropCount) return;

        vec2 s(item.size.x * DETAIL * 2.0f, item.size.z * DETAIL * 2.0f);
        Core::setViewport(0, 0, int(s.x + 0.5f), int(s.y + 0.5f));

        vec4 rPosScale[2] = { vec4(0.0f), vec4(1.0f) };

        for (int i = 0; i < dropCount; i++) {
            Drop &drop = drops[i];

            vec3 p;
            p.x = (drop.pos.x - (item.pos.x - item.size.x)) * DETAIL;
            p.z = (drop.pos.z - (item.pos.z - item.size.z)) * DETAIL;

            Core::beginRenderPass(RP_WATER_PROCESS, item.data[1], RenderTarget(NULL));
            Core::setPipelineState(PS_WATER_DROP);

            Core::active.shader->setParam(uPosScale, rPosScale[0], 2);
            Core::active.shader->setParam(uTexParam, vec4(1.0f / item.data[0]->width, 1.0f / item.data[0]->height, s.x / item.data[0]->width, s.y / item.data[0]->height));
            Core::active.shader->setParam(uParam, vec4(p.x, p.z, drop.radius * DETAIL, -drop.strength));

            item.data[0]->bind(sNormal);
            game->getMesh()->renderQuad();
            item.data[0]->unbind(sNormal);
            swap(item.data[0], item.data[1]);

            Core::endRenderPass();
        }
    }
    
    bool step(Item &item) {
        if (item.timer < SIMULATE_TIMESTEP) return false;

        vec2 s(item.size.x * DETAIL * 2.0f, item.size.z * DETAIL * 2.0f);
        Core::setViewport(0, 0, int(s.x + 0.5f), int(s.y + 0.5f));

        while (item.timer >= SIMULATE_TIMESTEP) {
            Core::beginRenderPass(RP_WATER_PROCESS, item.data[1], RenderTarget(NULL));
            Core::setPipelineState(PS_WATER_CALC);

            Core::active.shader->setParam(uParam, vec4(0.995f, 1.0f, randf() * 0.5f, Core::params.x));
            Core::active.shader->setParam(uTexParam, vec4(1.0f / item.data[0]->width, 1.0f / item.data[0]->height, s.x / item.data[0]->width, s.y / item.data[0]->height));
            Core::active.shader->setParam(uRoomSize, vec4(1.0f / item.mask->origWidth, 1.0f / item.mask->origHeight, float(item.mask->origWidth) / item.mask->width, float(item.mask->origHeight) / item.mask->height));

            item.data[0]->bind(sNormal);
            game->getMesh()->renderQuad();
            item.data[0]->unbind(sNormal);
            swap(item.data[0], item.data[1]);
            item.timer -= SIMULATE_TIMESTEP;

            Core::endRenderPass();
        }

        return true;
    }

    void caustics(Item &item) {
        if (Core::settings.detail.water < Core::Settings::HIGH)
            return;

        Core::beginRenderPass(RP_WATER_CAUSTICS, item.caustics, RenderTarget(NULL));
        Core::setPipelineState(PS_WATER_CAUSTICS);

        vec4 rPosScale[2] = { vec4(0.0f), vec4(32767.0f / PLANE_DETAIL) };
        Core::active.shader->setParam(uPosScale, rPosScale[0], 2);

        float sx = item.size.x * DETAIL / (item.data[0]->width  / 2);
        float sz = item.size.z * DETAIL / (item.data[0]->height / 2);

        Core::active.shader->setParam(uTexParam, vec4(1.0f / item.data[0]->width, 1.0f / item.data[0]->height, sx, sz));

        item.caustics->unbind(sReflect);
        Core::setViewport(1, 1, item.caustics->width - 1, item.caustics->width - 1); // leave 2px for black border
        Core::whiteTex->bind(sReflect);
        item.data[0]->bind(sNormal);
        game->getMesh()->renderPlane();
        item.data[0]->unbind(sNormal);

        Core::endRenderPass();
    }

    void renderRays() {
        #ifdef _OS_PSV // TODO
            return;
        #endif
        if (!visible) return;
        PROFILE_MARKER("Water (rays)");

        Core::setPipelineState(PS_WATER_RAYS);

        for (int i = 0; i < count; i++) {
            Item &item = items[i];
            if (!item.visible || !item.caustics) continue;

        // render water plane
            item.caustics->bind(sReflect);
            Core::ditherTex->bind(sMask);

            vec3 bCenter = vec3(item.pos.x, item.pos.y + WATER_VOLUME_HEIGHT / 2, item.pos.z);
            vec3 bSize   = vec3(item.size.x, WATER_VOLUME_HEIGHT / 2, item.size.z);
            Box box(bCenter - bSize, bCenter + bSize);

            vec4 rPosScale[2] = { vec4(bCenter, 0.0), vec4(bSize, 1.0) };

            Core::active.shader->setParam(uPosScale, rPosScale[0], 2);
            Core::active.shader->setParam(uParam,    vec4(level->rooms[item.to].getOffset(), 0.35f));

            game->getMesh()->renderWaterVolume(item.to);
        }
    }

    void renderMask() {
        if (!visible) return;
        PROFILE_MARKER("Water (mask)");
    // mask underwater geometry by zero alpha
        Core::setPipelineState(PS_WATER_MASK);

        Core::active.shader->setParam(uTexParam, vec4(1.0f));

        for (int i = 0; i < count; i++) {
            Item &item = items[i];
            if (!item.visible) continue;

            vec4 rPosScale[2] = { vec4(item.pos, 0.0f), vec4(item.size, 1.0) };
            Core::active.shader->setParam(uPosScale, rPosScale[0], 2);

            game->getMesh()->renderQuad();
        }
    }


    Texture* getScreenTex() {
        int w = Core::viewportDef.z;
        int h = Core::viewportDef.w;
    // get refraction texture
        if (!refract || w != refract->origWidth || h != refract->origHeight) {
            delete refract;
            refract = new Texture(w, h, 1, FMT_RGBA, OPT_TARGET);
        #ifdef USE_SCREEN_TEX
            delete screen;
            screen = new Texture(w, h, 1, FMT_RGBA, OPT_TARGET);
        #endif
        }
        return screen;
    }

    void copyScreenToRefraction() {
        PROFILE_MARKER("Water (copy)");
    // get refraction texture
        int x, y;
        if (!screen) {
            x = Core::viewportDef.x;
            y = Core::viewportDef.y;
        } else {
            x = y = 0;
        }
#if 0
        if (screen) {
            Core::setTarget(refract, NULL, RT_LOAD_DEPTH | RT_STORE_COLOR | RT_STORE_DEPTH);
            Core::validateRenderState();
            bool flip = false;
            #if defined(_GAPI_D3D9) || defined(_GAPI_D3D11) || defined(_GAPI_GXM)
                flip = true;
            #endif
            blitTexture(screen, flip);
            Core::setTarget(screen, NULL, RT_LOAD_COLOR | RT_LOAD_DEPTH | RT_STORE_COLOR);
            Core::validateRenderState();
        } else {
            Core::copyTarget(refract, 0, 0, x, y, Core::viewportDef.z, Core::viewportDef.w); // copy framebuffer into refraction texture
        }
#endif
    }

    void simulate() {
        PROFILE_MARKER("Water (simulate)");
    // simulate water
        for (int i = 0; i < count; i++) {
            Item &item = items[i];
            if (!item.visible) continue;

            if (item.timer >= SIMULATE_TIMESTEP || dropCount) {
                Core::noiseTex->bind(sDiffuse);
                item.mask->bind(sMask);

                drop(item);

                if (step(item)) {
                    caustics(item);
                }
            }
        }
    }

    void renderReflection() {
        if (!visible) return;
        PROFILE_MARKER("Water (refl)");

        for (int i = 0; i < count; i++) {
            Item &item = items[i];
            if (item.visible && item.blank)
                item.init(game);
        }
#if 0
    // render mirror reflection
        Core::setTarget(reflect, NULL, RT_CLEAR_COLOR | RT_CLEAR_DEPTH | RT_STORE_COLOR);
        Camera *camera = (Camera*)game->getCamera();
        game->setupBinding();

    // merge visible rooms for all items
        RoomDesc roomsList[256];
        int roomsCount = 0;

        for (int i = 0; i < level->roomsCount; i++)
            level->rooms[i].flags.visible = false;

        bool underwater = level->rooms[camera->getRoomIndex()].flags.water;
        vec4 reflectPlane;

        for (int i = 0; i < count; i++) {
            Item &item = items[i];
            if (!item.visible) continue;

            reflectPlane = vec4(0, 1, 0, -item.pos.y);
            camera->reflectPlane = &reflectPlane;
            camera->setup(true);

            game->getVisibleRooms(roomsList, roomsCount, TR::NO_ROOM, underwater ? item.from : item.to, vec4(-1.0f, -1.0f, 1.0f, 1.0f), false);
        }

        if (roomsCount) {
        // select optimal water plane
            float waterDist  = 10000000.0f;
            int   waterItem = 0;

            for (int i = 0; i < count; i++) {
                Item &item = items[i];
                if (!item.visible) continue;

                float d = fabsf(item.pos.x - camera->eye.pos.x) + fabsf(item.pos.z - camera->eye.pos.z);

                if (d < waterDist) {
                    waterDist = d;
                    waterItem = i;
                }
            }

            float waterLevel = items[waterItem].pos.y;
            float sign = underwater ? -1.0f : 1.0f;
            reflectPlane = vec4(0.0f, -1.0f, 0.0f, waterLevel) * sign;
            camera->reflectPlane = &reflectPlane;
            camera->setup(true);

        // render reflections frame
            game->renderView(TR::NO_ROOM, false, false, roomsCount, roomsList);
        }

        camera->reflectPlane = NULL;
        camera->setup(true);
#endif
    }

    void compose() {
        if (!visible) return;
        PROFILE_MARKER("Water");
        for (int i = 0; i < count; i++) {
            Item &item = items[i];
            if (!item.visible) continue;

        // render water plane
            Core::setPipelineState(PS_WATER_COMPOSE);
            Core::updateLights();

            Core::active.shader->setParam(uParam, vec4(float(refract->origWidth) / refract->width, float(refract->origHeight) / refract->height, 0.05f, 0.0f));

            float sx = item.size.x * DETAIL / (item.data[0]->width  / 2);
            float sz = item.size.z * DETAIL / (item.data[0]->height / 2);

            Core::active.shader->setParam(uTexParam, vec4(1.0f / item.data[0]->width, 1.0f / item.data[0]->height, sx, sz));
            Core::active.shader->setParam(uRoomSize, vec4(1.0f / item.mask->origWidth, 1.0f / item.mask->origHeight, float(item.mask->origWidth) / item.mask->width, float(item.mask->origHeight) / item.mask->height));

            refract->bind(sDiffuse);
            reflect->bind(sReflect);
            item.mask->bind(sMask);
            item.data[0]->bind(sNormal);
#if 0
            Core::setCullMode(cmNone);
            Core::setBlendMode(bmAlpha);
            #ifdef WATER_USE_GRID
                vec4 rPosScale[2] = { vec4(item.pos, 0.0f), vec4(item.size * vec3(1.0f / PLANE_DETAIL, 512.0f, 1.0f / PLANE_DETAIL), 1.0f) };
                Core::active.shader->setParam(uPosScale, rPosScale[0], 2);
                game->getMesh()->renderPlane();
            #else
                vec4 rPosScale[2] = { vec4(item.pos, 0.0f), vec4(item.size, 1.0) };
                Core::active.shader->setParam(uPosScale, rPosScale[0], 2);
                game->getMesh()->renderQuad();
            #endif
            Core::setCullMode(cmFront);
            Core::setBlendMode(bmNone);
#endif
        }
        dropCount = 0;
    }

    void blitTexture(Texture *tex, bool flip = false) {
        ASSERT(tex);

        Core::setPipelineState(PS_GUI);

        mat4 mProj = GAPI::ortho(0.0f, float(tex->origWidth), 0.0f, float(tex->origHeight), 0.0f, 1.0f);

        Core::active.shader->setParam(uViewProj, mProj);
        Core::active.shader->setParam(uMaterial, vec4(1.0f));

        tex->bind(sDiffuse);
        int w = tex->width;
        int h = tex->height;

        Index  indices[6] = { 0, 1, 2, 0, 2, 3 };
        Vertex vertices[4];
        vertices[0].coord = short4(0, h, 0, 0);
        vertices[1].coord = short4(w, h, 0, 0);
        vertices[2].coord = short4(w, 0, 0, 0);
        vertices[3].coord = short4(0, 0, 0, 0);
        vertices[0].light =
        vertices[1].light =
        vertices[2].light =
        vertices[3].light = ubyte4(255, 255, 255, 255);

    #if defined(_GAPI_D3D9) || defined(_GAPI_D3D11) || defined(_GAPI_GXM)
        flip = !flip;
    #endif

        if (flip) {
            vertices[0].texCoord = short4(    0,     0, 0, 0);
            vertices[1].texCoord = short4(32767,     0, 0, 0);
            vertices[2].texCoord = short4(32767, 32767, 0, 0);
            vertices[3].texCoord = short4(    0, 32767, 0, 0);
        } else {
            vertices[0].texCoord = short4(    0, 32767, 0, 0);
            vertices[1].texCoord = short4(32767, 32767, 0, 0);
            vertices[2].texCoord = short4(32767,     0, 0, 0);
            vertices[3].texCoord = short4(    0,     0, 0, 0);
        }
#if 0
        Core::setDepthTest(false);
        Core::setBlendMode(bmNone);

        game->getMesh()->renderBuffer(indices, COUNT(indices), vertices, COUNT(vertices));

        Core::setDepthTest(true);
#endif
        tex->unbind(sDiffuse);
    }

    #undef MAX_SURFACES
    #undef MAX_INVISIBLE_TIME
    #undef SIMULATE_TIMESTEP
    #undef DETAIL
};

struct ZoneCache {

    struct Item {
        uint16 zone;
        uint16 count;
        uint16 *zones;
        uint16 *boxes;
        Item   *next;

        Item(uint16 zone, uint16 count, uint16 *zones, uint16 *boxes, Item *next) :
            zone(zone), count(count), zones(zones), boxes(boxes), next(next) {}

        ~Item() {
            delete[] boxes;
            delete next;
        }
    } *items;

    IGame  *game;
    // dummy arrays for path search
    uint16 *nodes;
    uint16 *parents;
    uint16 *weights;

    ZoneCache(IGame *game) : items(NULL), game(game) {
        TR::Level *level = game->getLevel();
        nodes   = new uint16[level->boxesCount * 3];
        parents = nodes + level->boxesCount;
        weights = nodes + level->boxesCount * 2;
    }

    ~ZoneCache() {
        delete   items;
        delete[] nodes;
    }

    Item *getBoxes(uint16 zone, uint16 *zones) {
        Item *item = items;
        while (item) {
            if (item->zone == zone && item->zones == zones) 
                return item;
            item = item->next;
        }

        int count = 0;
        TR::Level *level = game->getLevel();
        for (int i = 0; i < level->boxesCount; i++)
            if (zones[i] == zone)
                nodes[count++] = i;

        ASSERT(count > 0);
        uint16 *boxes = new uint16[count];
        memcpy(boxes, nodes, sizeof(uint16) * count);

        return items = new Item(zone, count, zones, boxes, items);
    }

    uint16 findPath(int ascend, int descend, bool big, int boxStart, int boxEnd, uint16 *zones, uint16 **boxes) {
        if (boxStart == TR::NO_BOX || boxEnd == TR::NO_BOX)
            return 0;

        TR::Level *level = game->getLevel();
        memset(parents, 0xFF, sizeof(uint16) * level->boxesCount); // fill parents by 0xFFFF
        memset(weights, 0x00, sizeof(uint16) * level->boxesCount); // zeroes weights

        uint16 count = 0;
        nodes[count++] = boxEnd;

        uint16 zone = zones[boxStart];

        if (zone != zones[boxEnd])
            return 0;

        TR::Box &b = level->boxes[boxStart];

        int sx = (b.minX + b.maxX) >> 11; // box center / 1024
        int sz = (b.minZ + b.maxZ) >> 11;

        while (count) {
            // get min weight
            int minI  = 0;
            int minW = weights[nodes[minI]];
            for (int i = 1; i < count; i++)
                if (weights[nodes[i]] < minW) {
                    minI = i;
                    minW = weights[nodes[i]];
                }
            int cur = nodes[minI];

            // peek min weight item from array
            count--;
            for (int i = minI; i < count; i++)
                nodes[i] = nodes[i + 1];

            // check for end of path
            if (cur == boxStart) {
                count = 0;
                while (cur != boxEnd) {
                    nodes[count++] = cur;
                    cur = parents[cur];
                }
                nodes[count++] = cur;
                *boxes = nodes;
                return count;
            }

            // add overlap boxes
            TR::Box &b = game->getLevel()->boxes[cur];
            TR::Overlap *overlap = &level->overlaps[b.overlap.index];

            do {
                uint16 index = overlap->boxIndex;
                // unvisited yet
                if (parents[index] != 0xFFFF)
                    continue;
                // has same zone
                if (zones[index] != zone)
                    continue;
                // check passability
                if (big && level->boxes[index].overlap.blockable)
                    continue;
                // check blocking (doors)
                if (level->boxes[index].overlap.block)
                    continue;
                // check for height difference
                int d = level->boxes[index].floor - b.floor;
                if (d > ascend || d < descend)
                    continue;
                
                int dx = sx - ((b.minX + b.maxX) >> 11);
                int dz = sz - ((b.minZ + b.maxZ) >> 11);
                int w = abs(dx) + abs(dz);

                ASSERT(count < level->boxesCount);
                nodes[count++] = index;
                parents[index] = cur;
                weights[index] = weights[cur] + w;

            } while (!(overlap++)->end);
        }

        return 0;
    }
};

#endif
