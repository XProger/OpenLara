#ifndef H_LEVEL
#define H_LEVEL

#include "core.h"
#include "utils.h"
#include "format.h"
#include "cache.h"
#include "enemy.h"
#include "camera.h"
#include "lara.h"
#include "trigger.h"
#include "inventory.h"
#include "savegame.h"
#include "network.h"

#if defined(_DEBUG) && defined(_GAPI_GL) && !defined(_GAPI_GLES)
    #define DEBUG_RENDER
#endif

#ifdef DEBUG_RENDER
    #include "debug.h"
#endif

#define ANIM_TEX_TIMESTEP (10.0f / 30.0f)
#define SKY_TIME_PERIOD   (1.0f / 0.005f)

extern void loadLevelAsync(Stream *stream, void *userData);

extern Array<SaveSlot> saveSlots;
extern SaveResult saveResult;
extern int loadSlot;

struct Level : IGame {

    TR::Level   level;
    Texture     *atlas;
    MeshBuilder *mesh;

    Lara        *players[2], *player;
    Camera      *camera;
    Texture     *shadow;

    struct Params {
        float   time;
        float   waterHeight;
        float   clipSign;
        float   clipHeight;
    } *params;

    ZoneCache    *zoneCache;
    AmbientCache *ambientCache;
    WaterCache   *waterCache;

    Sound::Sample *sndTrack, *sndWater;
    bool waitTrack;

    bool lastTitle;
    bool isEnded;
    bool needRedrawTitleBG;
    bool needRedrawReflections;
    bool needRenderGame;
    bool showStats;
    bool skyIsVisible;

    TR::LevelID nextLevel;

    TR::Effect::Type effect;
    float      effectTimer;
    int        effectIdx;
    float      cutsceneWaitTimer;
    float      animTexTimer;
    float      statsTimeDelta;

    vec3 underwaterColor;
    vec4 underwaterFogParams;
    vec4 levelFogParams;

// IGame implementation ========
    virtual void loadLevel(TR::LevelID id) {
        sndWater = sndTrack = NULL;
        Sound::stopAll();
        nextLevel = id;
    }

    virtual void loadNextLevel() {
        if (nextLevel != TR::LVL_MAX) return;

        TR::LevelID id = TR::LVL_MAX;
    #ifdef _OS_WEB
        if (level.id == TR::LVL_TR1_2 && level.version != TR::VER_TR1_PC)
            id = TR::LVL_TR1_TITLE;
        else
    #endif
        id = (level.isEnd() || level.isHome()) ? level.getTitleId() : TR::LevelID(level.id + 1);

        if (!level.isTitle() && loadSlot == -1) {
        // update statistics info for current level
            if (!TR::isCutsceneLevel(level.id) && !level.isHome())
                saveGame(level.id, false, true);
        // save next level
            if (!TR::isCutsceneLevel(id) && !TR::isTitleLevel(id)) {
                saveGame(id, false, false);
                loadSlot  = getSaveSlot(id, false);
                showStats = true;
            }
        }
        loadLevel(id);
    }

    virtual void invShow(int playerIndex, int page, int itemIndex = -1) {
        if (itemIndex != -1 || page == Inventory::PAGE_SAVEGAME)
            inventory->pageItemIndex[page] = itemIndex;
        inventory->toggle(playerIndex, Inventory::Page(page));
    }

    SaveSlot createSaveSlot(TR::LevelID id, bool checkpoint, bool dummy = false) {
        SaveSlot slot;

        // allocate oversized data for save slot
        slot.data  = new uint8[sizeof(SaveStats) + sizeof(SaveItem) * inventory->itemsCount + // for every save
                               sizeof(SaveState) + sizeof(SaveEntity) * level.entitiesCount]; // only for checkpoints

        uint8 *ptr = slot.data;

    // level progress stats
        SaveStats *stats = (SaveStats*)ptr;
        if (!checkpoint)
            memset(stats, 0, sizeof(*stats));
        else
            *stats = saveStats;
        stats->level      = id;
        stats->checkpoint = checkpoint;
        ptr += sizeof(*stats);

    // inventory items
        int32 *itemsCount = (int32*)ptr;
        ptr += sizeof(*itemsCount);

        *itemsCount = 0;
        if (dummy) {
            SaveItem *item = (SaveItem*)ptr;
            ptr += sizeof(*item);

            item->type  = TR::Entity::INV_PISTOLS;
            item->count = UNLIMITED_AMMO;

            *itemsCount = 1;
        } else {
            for (int i = 0; i < inventory->itemsCount; i++) {
                Inventory::Item *invItem = inventory->items[i];

                if (!TR::Entity::isPickup(TR::Level::convFromInv(invItem->type)))
                    continue;

                if (!checkpoint) {
                    if (!TR::Entity::isCrossLevelItem(TR::Level::convFromInv(invItem->type)))
                        continue;
                    if (TR::isEmptyLevel(id))
                        continue;
                }

                SaveItem *item = (SaveItem*)ptr;
                ptr += sizeof(*item);

                item->type  = invItem->type;
                item->count = invItem->count;

                (*itemsCount)++;
            }
        }

        if (checkpoint) {
        // level state
            SaveState *state = (SaveState*)ptr;
            ptr += sizeof(*state);
            *state = level.state;

        // level entities
            int32 *entitiesCount = (int32*)ptr;
            ptr += sizeof(*entitiesCount);

            *entitiesCount = 0;
            for (int i = 0; i < level.entitiesCount; i++) {
                Controller *controller = (Controller*)level.entities[i].controller;
                SaveEntity *entity = (SaveEntity*)ptr;
                if (!controller || !controller->getSaveData(*entity)) continue;
                ptr += (sizeof(SaveEntity) - sizeof(SaveEntity::Extra)) + entity->extraSize;
                (*entitiesCount)++;
            }
        }

        slot.size = int32(ptr - slot.data);

        return slot;
    }

    void parseSaveSlot(const SaveSlot &slot) {
        clearInventory();

        uint8 *data = slot.data;
        uint8 *ptr  = data;

    // level progress stats
        if (slot.isCheckpoint())
            saveStats = *(SaveStats*)ptr; // start level current position

        ptr += sizeof(saveStats);

    // inventory items
        int32 itemsCount = *(int32*)ptr;
        ptr += sizeof(itemsCount);

        for (int i = 0; i < itemsCount; i++) {
            SaveItem *item = (SaveItem*)ptr;
            inventory->add(TR::Entity::Type(item->type), item->count, false);
            ptr += sizeof(*item);
        }

        if (slot.isCheckpoint()) {
            clearEntities();

        // level state
            level.state = *(SaveState*)ptr;
            ptr += sizeof(level.state);

        // level entities
            int32 entitiesCount = *(int32*)ptr;
            ptr += sizeof(entitiesCount);

            for (int i = 0; i < entitiesCount; i++) {
                SaveEntity *entity = (SaveEntity*)ptr;

                Controller *controller;
                if (i >= level.entitiesBaseCount)
                    controller = addEntity(TR::Entity::Type(entity->type), entity->room, vec3(float(entity->x), float(entity->y), float(entity->z)), TR::angle(entity->rotation));
                else
                    controller = (Controller*)level.entities[i].controller;

                controller->setSaveData(*entity);
                if (Controller::first != controller && controller->flags.state != TR::Entity::asNone) {
                    controller->next = Controller::first;
                    Controller::first = controller;
                }

                if (controller->getEntity().isLara()) {
                    Lara *lara = (Lara*)controller;
                    if (lara->camera)
                        lara->camera->reset();
                }

                ptr += (sizeof(SaveEntity) - sizeof(SaveEntity::Extra)) + entity->extraSize;
            }

            if (level.state.flags.flipped) {
                flipMap();
                level.state.flags.flipped = true;
            }

            uint8 track = level.state.flags.track;
            level.state.flags.track = 0;
            playTrack(track);
        }

        statsTimeDelta = 0.0f;
    }

    static void saveGameWriteAsync(Stream *stream, void *userData) {
        if (stream != NULL) {
            delete[] stream->data;
            delete stream;
            saveResult = SAVE_RESULT_SUCCESS;
            UI::showHint(STR_HINT_SAVING_DONE, 1.0f);
        } else {
            saveResult = SAVE_RESULT_ERROR;
            UI::showHint(STR_HINT_SAVING_ERROR, 3.0f);
        }
    }

    virtual void saveGame(TR::LevelID id, bool checkpoint, bool updateStats) {
        ASSERT(saveResult != SAVE_RESULT_WAIT);

        if (saveResult == SAVE_RESULT_WAIT)
            return;

        LOG("Save Game...\n");

        SaveSlot slot;
        if (updateStats) {
            removeSaveSlot(id, true);
            int index = getSaveSlot(id, false);
            if (index == -1) {
                slot = createSaveSlot(id, false, true);
                saveSlots.push(slot);
            } else
                slot = saveSlots[index];
            SaveStats *stats = (SaveStats*)slot.data;
            stats->level      = level.id;
            stats->checkpoint = checkpoint;
        } else {
            removeSaveSlot(id, checkpoint); // remove checkpoints and level saves
            saveSlots.push(createSaveSlot(id, checkpoint));
        }

        saveSlots.sort();

        if (!updateStats) {
            saveResult = SAVE_RESULT_WAIT;
            UI::showHint(STR_HINT_SAVING, 60.0f);

            int size;
            uint8 *data = writeSaveSlots(size);
            osWriteSlot(new Stream(SAVE_FILENAME, (const char*)data, size, saveGameWriteAsync, this));
        }
    }

    virtual void loadGame(int slot) {
        LOG("Load Game...\n");
        loadSlot = slot;
    }

    void clearInventory() {
        int i = inventory->itemsCount;

        while (i--) {
            if (TR::Entity::isPickup(TR::Level::convFromInv(inventory->items[i]->type)))
                inventory->remove(i);
        }
    }

    void clearEntities() {
        Controller::first = NULL;
        for (int i = 0; i < level.entitiesCount; i++) {
            TR::Entity &e = level.entities[i];
            Controller *controller = (Controller*)e.controller;
            if (controller) {
                controller->next = NULL;
                controller->flags.state = TR::Entity::asNone;
                if (i >= level.entitiesBaseCount) {
                    delete controller;
                    e.controller = NULL;
                }
            }
        }
    }

    void initShadow() {
        delete shadow;
        if (Core::settings.detail.shadows > Core::Settings::LOW) {
            if (level.isTitle())
                shadow = new Texture(32, 32, 1, FMT_SHADOW); // init dummy shadow map
            else
                shadow = new Texture(SHADOW_TEX_SIZE, SHADOW_TEX_SIZE, 1, FMT_SHADOW, OPT_TARGET);
        } else
            shadow = NULL;
    }

    virtual void applySettings(const Core::Settings &settings) {
        if (settings.detail.filter != Core::settings.detail.filter)
            atlas->setFilterQuality(settings.detail.filter);

        bool rebuildMesh    = settings.detail.water    != Core::settings.detail.water;
        bool rebuildAmbient = settings.detail.lighting != Core::settings.detail.lighting;
        bool rebuildShadows = settings.detail.shadows  != Core::settings.detail.shadows;
        bool rebuildWater   = settings.detail.water    != Core::settings.detail.water;
        bool switchModels   = settings.detail.simple   != Core::settings.detail.simple;

        bool rebuildShaders = rebuildWater || rebuildAmbient || rebuildShadows;

        bool redraw = memcmp(&settings.detail, &Core::settings.detail, sizeof(settings.detail)) != 0;

    #ifdef _OS_ANDROID
        if ((settings.detail.stereo == Core::Settings::STEREO_VR) ^ (Core::settings.detail.stereo == Core::Settings::STEREO_VR))
            osToggleVR(settings.detail.stereo == Core::Settings::STEREO_VR);
    #endif

        Core::settings = settings;

        Core::setVSync(Core::settings.detail.vsync != 0);

        Stream::cacheWrite("settings", (char*)&settings, sizeof(settings));

        if (rebuildShaders) {
        #if !defined(_GAPI_D3D9) && !defined(_GAPI_GXM)
            delete shaderCache;
            shaderCache = new ShaderCache();
        #endif
        }

        if (rebuildMesh) {
            delete mesh;
            mesh = new MeshBuilder(&level, atlas);
        }

        if (rebuildAmbient) {
            delete ambientCache;
            ambientCache = Core::settings.detail.lighting > Core::Settings::MEDIUM ? new AmbientCache(this) : NULL;
        }

        if (rebuildShadows)
            initShadow();
            
        if (rebuildWater) {
            delete waterCache;
            waterCache = Core::settings.detail.water > Core::Settings::LOW ? new WaterCache(this) : NULL;
        }

        if (redraw && inventory->active && !level.isTitle())
            needRedrawTitleBG = true;

        if (switchModels)
            resetModels();
    }

    virtual TR::Level* getLevel() {
        return &level;
    }

    virtual MeshBuilder* getMesh() {
        return mesh;
    }

    virtual Texture* getAtlas() {
        return atlas;
    }

    virtual ICamera* getCamera(int index = -1) {
        if (index == -1)
            return camera;
        if (players[index])
            return players[index]->camera;
        return NULL;
    }

    virtual Controller* getLara(int index = 0) {
        return players[index];
    }

    virtual Controller* getLara(const vec3 &pos) {
        if (!players[1])
            return players[0];
        if (players[0]->health <= 0.0f)
            return players[1];
        if (players[1]->health <= 0.0f)
            return players[0];
        return (players[0]->pos - pos).length2() < (players[1]->pos - pos).length2() ? players[0] : players[1];
    }

    virtual bool isCutscene() {
        if (level.isTitle()) return false;
        return camera->mode == Camera::MODE_CUTSCENE;
    }

    virtual uint16 getRandomBox(uint16 zone, uint16 *zones) { 
        ZoneCache::Item *item = zoneCache->getBoxes(zone, zones);
        return item->boxes[int(randf() * item->count)];
    }
    
    virtual uint16 findPath(int ascend, int descend, bool big, int boxStart, int boxEnd, uint16 *zones, uint16 **boxes) {
        return zoneCache->findPath(ascend, descend, big, boxStart, boxEnd, zones, boxes);
    }

    void updateBlocks(bool rise) {
        for (int i = 0; i < level.entitiesBaseCount; i++) {
            Controller *controller = (Controller*)level.entities[i].controller;
            switch (level.entities[i].type) {
                case TR::Entity::BLOCK_1 :
                case TR::Entity::BLOCK_2 :
                case TR::Entity::BLOCK_3 :
                case TR::Entity::BLOCK_4 :
                    ((Block*)controller)->updateFloor(rise);
                    break;
                case TR::Entity::MOVING_BLOCK :
                    ((MovingBlock*)controller)->updateFloor(rise);
                    break;
                default : ;
            }
        }
    }

    virtual void flipMap(bool water = true) {
        updateBlocks(false);
        if (water && waterCache) waterCache->flipMap();
        mesh->flipMap();
        level.flipMap();
        updateBlocks(true);
    }

    virtual void setClipParams(float clipSign, float clipHeight) {
        params->clipSign   = clipSign;
        params->clipHeight = clipHeight;
    }

    virtual void setWaterParams(float height) {
        params->waterHeight = height;
    }

    virtual void waterDrop(const vec3 &pos, float radius, float strength) {
        if (waterCache)
            waterCache->addDrop(pos, radius, strength);
    }

    virtual void setShader(Core::Pass pass, Shader::Type type, bool underwater = false, bool alphaTest = false) {
        shaderCache->bind(pass, type, (underwater ? ShaderCache::FX_UNDERWATER : 0) | (alphaTest ? ShaderCache::FX_ALPHA_TEST : 0) | ((params->clipHeight != NO_CLIP_PLANE && pass == Core::passCompose) ? ShaderCache::FX_CLIP_PLANE : 0));
    }

    virtual void setRoomParams(int roomIndex, Shader::Type type, float diffuse, float ambient, float specular, float alpha, bool alphaTest = false) {
        if (Core::pass == Core::passShadow) {
            setShader(Core::pass, type, false, alphaTest);
            return;
        }

        TR::Room &room = level.rooms[roomIndex];

        if (room.dynLightsCount) {
            Core::lightPos[3]   = room.dynLights[0].pos;
            Core::lightColor[3] = room.dynLights[0].color;
        } else {
            Core::lightPos[3]   = vec4(0);
            Core::lightColor[3] = vec4(0, 0, 0, 1);
        }

        if (type == Shader::SPRITE) {
            alphaTest = true;
        }

    #ifdef FFP
        switch (type) {
            case Shader::SPRITE :
            case Shader::ROOM   :
                ambient = 1.0f;
                Core::lightColor[0].w = 1.0f;
                break;
            case Shader::FLASH  :
            case Shader::MIRROR :
                ambient = 1.0f;
                Core::lightColor[0].w = 
                Core::lightColor[1].w =
                Core::lightColor[2].w =
                Core::lightColor[3].w = 1.0f;
                break;
            default : ;
        }
    #endif

        vec4 material;

        if (Core::pass == Core::passAmbient) {
            if (room.flags.water) {
                Core::fogParams = underwaterFogParams;
                material = vec4(underwaterColor, 1.0f);
            } else {
                Core::fogParams = levelFogParams;
                material = vec4(1.0f);
            }
        } else {
            Core::fogParams = levelFogParams;
            material = vec4(diffuse, ambient, specular, alpha);
        }
        
        setShader(Core::pass, type, (Core::pass == Core::passAmbient) ? false : room.flags.water, alphaTest);

        Core::setMaterial(material.x, material.y, material.z, material.w);

        if (room.flags.water) {
            if (waterCache) {
                waterCache->bindCaustics(roomIndex);
            }
            setWaterParams(float(room.waterLevel[level.state.flags.flipped]));
        } else {
            setWaterParams(NO_CLIP_PLANE);
        }

        Core::active.shader->setParam(uParam, Core::params);

        Core::updateLights();

        if (Core::settings.detail.shadows > Core::Settings::MEDIUM)
            Core::active.shader->setParam(uContacts, Core::contacts[0], MAX_CONTACTS);
    }

    virtual void setupBinding() {
        atlas->bind(sDiffuse);
        Core::whiteTex->bind(sNormal);
        Core::whiteTex->bind(sMask);
        Core::whiteTex->bind(sReflect);
        Core::whiteCube->bind(sEnvironment);
        if (shadow) shadow->bind(sShadow);
        Core::basis.identity();
    }

    virtual void renderEnvironment(int roomIndex, const vec3 &pos, Texture **targets, int stride = 0, Core::Pass pass = Core::passAmbient) {
        PROFILE_MARKER("ENVIRONMENT");
        setupBinding();
        float      tmpEye  = Core::eye;
        Core::Pass tmpPass = Core::pass;
        Core::eye = 0.0f;

        int16 rIndex = roomIndex;
        level.getSector(rIndex, pos); // fix room index for overlapped blocks

    // render level into cube faces or texture images
        for (int i = 0; i < 6; i++) {
            setupCubeCamera(pos, i);
            Core::pass = pass;
            Texture *target = (targets[0]->opt & OPT_CUBEMAP) ? targets[0] : targets[i * stride];
            Core::setTarget(target, NULL, RT_CLEAR_COLOR | RT_CLEAR_DEPTH | RT_STORE_COLOR, i);
            renderView(rIndex, false);
        }

        Core::pass = tmpPass;
        Core::eye  = tmpEye;
    }
    
    virtual void setEffect(Controller *controller, TR::Effect::Type effect) {
        this->effect      = effect;
        this->effectTimer = 0.0f;
        this->effectIdx   = 0;

        switch (effect) {
            case TR::Effect::FLOOR_SHAKE :
                for (int i = 0; i < 2; i++)
                    if (players[i] && players[i]->camera)
                        players[i]->camera->shake = 0.5f * max(0.0f, 1.0f - (controller->pos - players[i]->camera->eye.pos).length2() / (15 * 1024 * 15 * 1024));
                return;
            case TR::Effect::FLOOD : {
                Sound::Sample *sample = playSound(TR::SND_FLOOD);
                if (sample)
                    sample->setVolume(0.0f, 4.0f);
                break;
            }
            case TR::Effect::STAIRS2SLOPE :
                playSound(TR::SND_STAIRS2SLOPE);
                break;
            case TR::Effect::EXPLOSION :
                playSound(TR::SND_TNT);
                shakeCamera(1.0f);
                break;
            default : ;
        }
    }

    virtual void checkTrigger(Controller *controller, bool heavy) {
        players[0]->checkTrigger(controller, heavy);
    }

    virtual void shakeCamera(float value, bool add = false) {
        for (int i = 0; i < 2; i++)
            if (players[i] && players[i]->camera) {
                if (add)
                    players[i]->camera->shake += value;
                else
                    players[i]->camera->shake  = value;
            }
    }


    virtual Controller* addEntity(TR::Entity::Type type, int room, const vec3 &pos, float angle) {
        int index;
        for (index = level.entitiesBaseCount; index < level.entitiesCount; index++) {
            TR::Entity &e = level.entities[index];
            if (!e.controller) {
                e.type          = type;
                e.room          = room;
                e.x             = int(pos.x);
                e.y             = int(pos.y);
                e.z             = int(pos.z);
                e.rotation      = TR::angle(normalizeAngle(angle));
                e.intensity     = -1;
                e.flags.value   = 0;
                e.flags.smooth  = true;
                e.modelIndex    = level.getModelIndex(e.type);
                break;
            }
        }

        if (index == level.entitiesCount)
            return NULL;

        TR::Entity &e = level.entities[index];
        if (e.isPickup())
            e.intensity = 4096;
        else
            if (e.isSprite()) {
                if (e.type == TR::Entity::LAVA_PARTICLE || e.type == TR::Entity::FLAME)
                    e.intensity = 0; // emissive
                else
                    e.intensity = 0x1FFF - level.rooms[room].ambient;
            }

        Controller *controller = initController(index);
        e.controller = controller;

        if (e.isEnemy() || e.isSprite()) {
            controller->flags.active = TR::ACTIVE;
            controller->activate();
        }

        return controller;
    }

    virtual void removeEntity(Controller *controller) {
        level.entities[controller->entity].controller = NULL;
        delete controller;
    }

    virtual void addMuzzleFlash(Controller *owner, int joint, const vec3 &offset, int lightIndex) {
        MuzzleFlash *mf = (MuzzleFlash*)addEntity(TR::Entity::MUZZLE_FLASH, owner->getRoomIndex(), offset, 0);
        if (mf) {
            mf->owner      = owner;
            mf->joint      = joint;
            mf->lightIndex = lightIndex;
        }
    }

    virtual bool invUse(int playerIndex, TR::Entity::Type type) {
        if (!players[playerIndex]->useItem(type))
            return inventory->use(type);
        return true;
    }

    virtual void invAdd(TR::Entity::Type type, int count) {
        inventory->add(type, count);
    }

    virtual int* invCount(TR::Entity::Type type) { 
        return inventory->getCountPtr(type);
    }

    virtual bool invChooseKey(int playerIndex, TR::Entity::Type hole) {
        return inventory->chooseKey(playerIndex, hole);
    }

    virtual Sound::Sample* playSound(int id, const vec3 &pos = vec3(0.0f), int flags = 0) const {
        if (level.version == TR::VER_TR1_PSX && id == TR::SND_SECRET)
            return NULL;

        int16 a = level.soundsMap[id];
        if (a == -1) return NULL;

        TR::SoundInfo &b = level.soundsInfo[a];
        if (b.chance == 0 || randf() <= b.chance) {
            int   index  = b.index + rand() % b.flags.count;
            float volume = b.volume;
            float pitch  = 1.0f + (b.flags.pitch ? ((randf() - 0.5f) * b.pitch) : 0.0f);

            if (level.version == TR::VER_TR2_PSX) // fix for 8 kHz VAG in PSX TR2
                pitch *= 8000.0f / 11025.0f;

            if (!(flags & Sound::MUSIC)) {
                switch (b.flags.mode) {
                    case 0 : if (level.version & TR::VER_TR1) flags |= Sound::UNIQUE; break;
                    case 1 : flags |= (level.version & TR::VER_TR1) ? Sound::REPLAY : Sound::UNIQUE; break;
                    case 2 : if (level.version & TR::VER_TR1)    flags |= Sound::LOOP; break;
                    case 3 : if (!(level.version & TR::VER_TR1)) flags |= Sound::LOOP | Sound::UNIQUE; break;
                }
            }
            if (b.flags.gain) volume = max(0.0f, volume - randf() * 0.25f);
            //if (b.flags.camera) flags &= ~Sound::PAN;
            return Sound::play(level.getSampleStream(index), &pos, volume, pitch, flags, id);
        }
        return NULL;
    }

    void stopChannel(Sound::Sample *channel) {
        if (channel == sndTrack) {
            sndTrack = NULL;
            if (level.state.flags.track != TR::LEVEL_INFO[level.id].track && TR::LEVEL_INFO[level.id].track != TR::NO_TRACK) // play ambient track
                playTrack(0);
        }
    }

    struct TrackRequest {
        Level *level;
        int   flags;

        TrackRequest(Level *level, int flags) : level(level), flags(flags) {}
    };

    static void playAsync(Stream *stream, void *userData) {
        TrackRequest *req = (TrackRequest*)userData;
        Level *level = req->level;
        level->waitTrack = false;
        if (stream) {
            level->sndTrack = Sound::play(stream, NULL, 0.01f, 1.0f, req->flags);
            if (level->sndTrack) {
                if (level->level.isCutsceneLevel()) {
                    Core::resetTime();
                }
                level->sndTrack->volume = level->sndTrack->volumeTarget = 0.0f;
            }
        }

        delete req;
    }

    static void playAsyncBG(Stream *stream, void *userData) {
        TrackRequest *req = (TrackRequest*)userData;
        if (stream)
            Sound::play(stream, NULL, 1.0f, 1.0f, req->flags);
        delete req;
    }

    virtual void playTrack(uint8 track, bool background = false) {
        if (background) {
            TR::getGameTrack(level.version, track, playAsyncBG, new TrackRequest(this, Sound::MUSIC));
            return;
        }

        if (track == 0) {
            if (sndTrack) return;
            track = TR::LEVEL_INFO[level.id].track;
        }

        if (level.state.flags.track == track) {
        //    if (sndTrack) {
        //        sndTrack->replay();
        //        sndTrack->setVolume(1.0f, 0.2f);
        //    }
            return;
        }

        level.state.flags.track = track;

        if (sndTrack) {
            sndTrack->setVolume(-1.0f, 0.2f);
            sndTrack = NULL;
        }

        if (track == 0xFF) return;

        int flags = Sound::MUSIC;
        if (track == TR::LEVEL_INFO[level.id].track)
            flags |= Sound::LOOP;

        waitTrack = true;
        TR::getGameTrack(level.version, track, playAsync, new TrackRequest(this, flags));

        UI::showSubs(UI::getSubs(track));
    }

    virtual void stopTrack() {
        playTrack(0xFF);
    }
//==============================

    Level(Stream &stream) : level(stream), waitTrack(false), isEnded(false), cutsceneWaitTimer(0.0f), animTexTimer(0.0f), statsTimeDelta(0.0f) {
        level.simpleItems = Core::settings.detail.simple == 1;
        level.initModelIndices();

    #ifdef _OS_PSP
        GAPI::freeEDRAM();
    #endif
        nextLevel = TR::LVL_MAX;
        showStats = false;

        params = (Params*)&Core::params;
        params->time = 0.0f;

        memset(players, 0, sizeof(players));
        player = NULL;

        underwaterColor     = vec3(0.6f, 0.9f, 0.9f);
        underwaterFogParams = vec4(underwaterColor * 0.2f, 1.0f / (6 * 1024));
        levelFogParams      = TR::getFogParams(level.id);

        inventory->game = this;

        if (!level.isCutsceneLevel()) {
            inventory->reset();
            memset(&saveStats, 0, sizeof(saveStats));
            saveStats.level = level.id;
        }

        initTextures();
        mesh = new MeshBuilder(&level, atlas);
        initEntities();

        shadow       = NULL;
        camera       = NULL;
        ambientCache = NULL;
        waterCache   = NULL;
        zoneCache    = NULL;

        needRedrawTitleBG = false;
        needRedrawReflections = true;

        initShadow();

        if (!(lastTitle = level.isTitle())) {
            ASSERT(players[0] != NULL);
            player = players[0];
            camera = player->camera;

            zoneCache    = new ZoneCache(this);
            ambientCache = Core::settings.detail.lighting > Core::Settings::MEDIUM ? new AmbientCache(this) : NULL;
            waterCache   = Core::settings.detail.water    > Core::Settings::LOW    ? new WaterCache(this)   : NULL;

            if (ambientCache) { // at first calculate ambient cube for Lara
                AmbientCache::Cube cube;
                ambientCache->getAmbient(players[0]->getRoomIndex(), players[0]->pos, cube); // add to queue
            }

            for (int i = 0; i < level.soundSourcesCount; i++) {
                TR::SoundSource &src = level.soundSources[i];
                int flags = Sound::PAN;
                if (src.flags & 64)  flags |= Sound::FLIPPED;
                if (src.flags & 128) flags |= Sound::UNFLIPPED;
                playSound(src.id, vec3(float(src.x), float(src.y), float(src.z)), flags);
            }

        }

        setClipParams(1.0f, NO_CLIP_PLANE);

        effect  = TR::Effect::NONE;

        sndWater = sndTrack = NULL;

        /*
        if (level.id == TR::LVL_TR2_RIG) {
            lara->animation.setAnim(level.models[level.extra.laraSpec].animation);
            camera->doCutscene(lara->pos, lara->angle.y);
        }
        */

        saveResult = SAVE_RESULT_SUCCESS;
        if (loadSlot != -1 && saveSlots[loadSlot].getLevelID() == level.id) {
            parseSaveSlot(saveSlots[loadSlot]);
            loadSlot = -1;
        }

        Network::start(this);

        Core::resetTime();
    }

    virtual ~Level() {
        Network::stop();

        for (int i = 0; i < level.entitiesCount; i++)
            delete (Controller*)level.entities[i].controller;

        delete shadow;
        delete ambientCache;
        delete waterCache;
        delete zoneCache;

        delete atlas;
        delete mesh;

        Sound::stopAll();
    }

    void init(bool playLogo, bool playVideo) {
        inventory->init(playLogo, playVideo);
    }

    void initEntities() {
        for (int i = 0; i < level.entitiesBaseCount; i++) {
            TR::Entity &e = level.entities[i];
            e.controller = initController(i);
            if (e.type == TR::Entity::LARA || ((level.version & TR::VER_TR1) && e.type == TR::Entity::CUT_1))
                players[0] = (Lara*)e.controller;
        }

        Sound::listenersCount = 1;
    }

    void resetModels() {
        level.simpleItems = Core::settings.detail.simple == 1;
        level.initModelIndices();
        for (int i = 0; i < level.entitiesCount; i++) {
            TR::Entity &e = level.entities[i];
            if (!e.controller) continue;
            Controller *controller = (Controller*)e.controller;
            controller->updateModel();
        }
    }

    void addPlayer(int index) {
        if (level.isCutsceneLevel()) return;

        if (!players[index]) {
            players[index] = (Lara*)addEntity(TR::Entity::LARA, 0, vec3(0.0f), 0.0f);
            players[index]->camera->cameraIndex = index;
            Sound::listenersCount = 2;
        } else if (index == 1) {
            removePlayer(index);
            Sound::listenersCount = 1;
            return;
        }

        Lara *lead = players[index ^ 1];
        if (!lead) return;

        Controller *c = Controller::first;
        while (c) {
            Controller *next = c->next;
            if (c->getEntity().type == TR::Entity::FLAME && ((Flame*)c)->owner == players[index])
                removeEntity(c);
            c = next;
        }

        players[index]->reset(lead->getRoomIndex(), lead->pos, lead->angle.y, lead->stand);
    }

    void removePlayer(int index) {
        for (int i = 0; i < level.entitiesCount; i++) {
            if (level.entities[i].controller && level.entities[i].isEnemy()) {
                Enemy *e = (Enemy*)level.entities[i].controller;
                if (e->target == players[index]) {
                    e->target = NULL;
                }
            }
        }
        removeEntity(players[index]);
        players[index] = NULL;
    }

    Controller* initController(int index) {
        if (level.entities[index].type == TR::Entity::CUT_1 && (level.version & TR::VER_TR1))
            return new Lara(this, index);

        switch (level.entities[index].type) {
            case TR::Entity::LARA                  : return new Lara(this, index);
            case TR::Entity::ENEMY_DOPPELGANGER    : return new Doppelganger(this, index);
            case TR::Entity::ENEMY_WOLF            : return new Wolf(this, index);
            case TR::Entity::ENEMY_BEAR            : return new Bear(this, index);
            case TR::Entity::ENEMY_BAT             : return new Bat(this, index);
            case TR::Entity::ENEMY_PUMA            :
            case TR::Entity::ENEMY_LION_MALE       :
            case TR::Entity::ENEMY_LION_FEMALE     : return new Lion(this, index);
            case TR::Entity::ENEMY_RAT_LAND        :
            case TR::Entity::ENEMY_RAT_WATER       : return new Rat(this, index);
            case TR::Entity::ENEMY_REX             : return new Rex(this, index);
            case TR::Entity::ENEMY_RAPTOR          : return new Raptor(this, index);
            case TR::Entity::ENEMY_MUTANT_1        :
            case TR::Entity::ENEMY_MUTANT_2        :
            case TR::Entity::ENEMY_MUTANT_3        : return new Mutant(this, index);
            case TR::Entity::ENEMY_CENTAUR         : return new Centaur(this, index);
            case TR::Entity::ENEMY_MUMMY           : return new Mummy(this, index);
            case TR::Entity::ENEMY_CROCODILE_LAND  :
            case TR::Entity::ENEMY_CROCODILE_WATER : return new Crocodile(this, index);
            case TR::Entity::ENEMY_GORILLA         : return new Gorilla(this, index);
            case TR::Entity::ENEMY_LARSON          : return new Larson(this, index);
            case TR::Entity::ENEMY_PIERRE          : return new Pierre(this, index);
            case TR::Entity::ENEMY_SKATEBOY        : return new SkaterBoy(this, index);
            case TR::Entity::ENEMY_COWBOY          : return new Cowboy(this, index);
            case TR::Entity::ENEMY_MR_T            : return new MrT(this, index);
            case TR::Entity::ENEMY_NATLA           : return new Natla(this, index);
            case TR::Entity::ENEMY_GIANT_MUTANT    : return new GiantMutant(this, index);
            case TR::Entity::DOOR_1                :
            case TR::Entity::DOOR_2                :
            case TR::Entity::DOOR_3                :
            case TR::Entity::DOOR_4                :
            case TR::Entity::DOOR_5                :
            case TR::Entity::DOOR_6                :
            case TR::Entity::DOOR_7                :
            case TR::Entity::DOOR_8                : return new Door(this, index);
            case TR::Entity::TRAP_DOOR_1           :
            case TR::Entity::TRAP_DOOR_2           : return new TrapDoor(this, index);
            case TR::Entity::BRIDGE_1              :
            case TR::Entity::BRIDGE_2              :
            case TR::Entity::BRIDGE_3              : return new Bridge(this, index);
            case TR::Entity::GEARS_1               :
            case TR::Entity::GEARS_2               :
            case TR::Entity::GEARS_3               : return new Gear(this, index);
            case TR::Entity::INV_KEY_ITEM_1        :
            case TR::Entity::INV_KEY_ITEM_2        :
            case TR::Entity::INV_KEY_ITEM_3        :
            case TR::Entity::INV_KEY_ITEM_4        : return new KeyItemInv(this, index);
            case TR::Entity::TRAP_FLOOR            : return new TrapFloor(this, index);
            case TR::Entity::CRYSTAL               : return new Crystal(this, index);
            case TR::Entity::TRAP_SWING_BLADE      : return new TrapSwingBlade(this, index);
            case TR::Entity::TRAP_SPIKES           : return new TrapSpikes(this, index);
            case TR::Entity::TRAP_BOULDER          : 
            case TR::Entity::TRAP_BOULDERS         : return new TrapBoulder(this, index);
            case TR::Entity::DART                  : return new Dart(this, index);
            case TR::Entity::TRAP_DART_EMITTER     : return new TrapDartEmitter(this, index);
            case TR::Entity::DRAWBRIDGE            : return new Drawbridge(this, index);
            case TR::Entity::BLOCK_1               :
            case TR::Entity::BLOCK_2               :
            case TR::Entity::BLOCK_3               :
            case TR::Entity::BLOCK_4               : return new Block(this, index);
            case TR::Entity::MOVING_BLOCK          : return new MovingBlock(this, index);
            case TR::Entity::TRAP_CEILING_1        :
            case TR::Entity::TRAP_CEILING_2        : return new TrapCeiling(this, index);
            case TR::Entity::TRAP_SLAM             : return new TrapSlam(this, index);
            case TR::Entity::TRAP_SWORD            : return new TrapSword(this, index);
            case TR::Entity::HAMMER_HANDLE         : return new ThorHammer(this, index);
            case TR::Entity::HAMMER_BLOCK          : return new ThorHammerBlock(this, index);
            case TR::Entity::LIGHTNING             : return new Lightning(this, index);
            case TR::Entity::MOVING_OBJECT         : return new MovingObject(this, index);
            case TR::Entity::SWITCH                :
            case TR::Entity::SWITCH_WATER          :
            case TR::Entity::SWITCH_BUTTON         : 
            case TR::Entity::SWITCH_BIG            : return new Switch(this, index);
            case TR::Entity::PUZZLE_HOLE_1         :
            case TR::Entity::PUZZLE_HOLE_2         :
            case TR::Entity::PUZZLE_HOLE_3         :
            case TR::Entity::PUZZLE_HOLE_4         :
            case TR::Entity::KEY_HOLE_1            :
            case TR::Entity::KEY_HOLE_2            :
            case TR::Entity::KEY_HOLE_3            :
            case TR::Entity::KEY_HOLE_4            : return new KeyHole(this, index);
            case TR::Entity::MIDAS_HAND            : return new MidasHand(this, index);
            case TR::Entity::SCION_TARGET          : return new ScionTarget(this, index);
            case TR::Entity::WATERFALL             : return new Waterfall(this, index);
            case TR::Entity::NATLA_BULLET          :
            case TR::Entity::MUTANT_BULLET         :
            case TR::Entity::CENTAUR_BULLET        : return new Bullet(this, index);
            case TR::Entity::TRAP_LAVA             : return new TrapLava(this, index);
            case TR::Entity::BUBBLE                : return new Bubble(this, index);
            case TR::Entity::EXPLOSION             : return new Explosion(this, index);
            case TR::Entity::WATER_SPLASH          :
            case TR::Entity::BLOOD                 :
            case TR::Entity::SMOKE                 :
            case TR::Entity::SPARKLES              : return new Sprite(this, index, true, Sprite::FRAME_ANIMATED);
            case TR::Entity::RICOCHET              : return new Sprite(this, index, true, Sprite::FRAME_RANDOM);
            case TR::Entity::CENTAUR_STATUE        : return new CentaurStatue(this, index);
            case TR::Entity::CABIN                 : return new Cabin(this, index);
            case TR::Entity::MUZZLE_FLASH          : return new MuzzleFlash(this, index);
            case TR::Entity::LAVA_PARTICLE         : ASSERT(false); return NULL;
            case TR::Entity::TRAP_LAVA_EMITTER     : return new TrapLavaEmitter(this, index);
            case TR::Entity::FLAME                 : return new Flame(this, index);
            case TR::Entity::TRAP_FLAME_EMITTER    : return new TrapFlameEmitter(this, index);
            case TR::Entity::BOAT                  : return new Boat(this, index);
            case TR::Entity::EARTHQUAKE            : return new Earthquake(this, index);
            case TR::Entity::MUTANT_EGG_SMALL      :
            case TR::Entity::MUTANT_EGG_BIG        : return new MutantEgg(this, index);

            case TR::Entity::ENEMY_DOG              : return new Dog(this, index);
            case TR::Entity::ENEMY_TIGER            : return new Tiger(this, index);
            case TR::Entity::ENEMY_GOON_MASK_1      :
            case TR::Entity::ENEMY_GOON_MASK_2      :
            case TR::Entity::ENEMY_GOON_MASK_3      :
            case TR::Entity::ENEMY_GOON_KNIFE       :
            case TR::Entity::ENEMY_GOON_SHOTGUN     :
            case TR::Entity::ENEMY_RAT              :
            case TR::Entity::ENEMY_DRAGON_FRONT     :
            case TR::Entity::ENEMY_DRAGON_BACK      :
            case TR::Entity::ENEMY_SHARK            :
            case TR::Entity::ENEMY_MORAY_1          :
            case TR::Entity::ENEMY_MORAY_2          :
            case TR::Entity::ENEMY_BARACUDA         :
            case TR::Entity::ENEMY_DIVER            :
            case TR::Entity::ENEMY_GUNMAN_1         :
            case TR::Entity::ENEMY_GUNMAN_2         :
            case TR::Entity::ENEMY_GOON_STICK_1     :
            case TR::Entity::ENEMY_GOON_STICK_2     :
            case TR::Entity::ENEMY_GOON_FLAME       :
            case TR::Entity::UNUSED_23              :
            case TR::Entity::ENEMY_SPIDER           :
            case TR::Entity::ENEMY_SPIDER_GIANT     :
            case TR::Entity::ENEMY_CROW             :
            case TR::Entity::ENEMY_MARCO            :
            case TR::Entity::ENEMY_GUARD_SPEAR        :
            case TR::Entity::ENEMY_GUARD_SPEAR_STATUE :
            case TR::Entity::ENEMY_GUARD_SWORD        :
            case TR::Entity::ENEMY_GUARD_SWORD_STATUE :
            case TR::Entity::ENEMY_YETI             :
            case TR::Entity::ENEMY_BIRD_MONSTER     :
            case TR::Entity::ENEMY_EAGLE            :
            case TR::Entity::ENEMY_MERCENARY_1      :
            case TR::Entity::ENEMY_MERCENARY_2      :
            case TR::Entity::ENEMY_MERCENARY_3      :
            case TR::Entity::ENEMY_MERCENARY_SNOWMOBILE :
            case TR::Entity::ENEMY_MONK_1           :
            case TR::Entity::ENEMY_MONK_2           : return new Enemy(this, index, 100, 10, 0.0f, 0.0f);
            case TR::Entity::ENEMY_WINSTON          : return new Winston(this, index);

            case TR::Entity::CRYSTAL_PICKUP         : return new CrystalPickup(this, index);
            case TR::Entity::STONE_ITEM_1           :
            case TR::Entity::STONE_ITEM_2           :
            case TR::Entity::STONE_ITEM_3           :
            case TR::Entity::STONE_ITEM_4           : return new StoneItem(this, index);

            case TR::Entity::WINDOW_1               :
            case TR::Entity::WINDOW_2               : return new BreakableWindow(this, index);

            case TR::Entity::HELICOPTER_FLYING      : return new HelicopterFlying(this, index);

            default                                 : return new Controller(this, index);
        }
    }

    TR::Tile32 *tileData;

    static void fillCallback(int id, int tileX, int tileY, int atlasWidth, int atlasHeight, Atlas::Tile &tile, void *userData, void *data) {
        static const uint32 barColor[UI::BAR_MAX][25] = {
            // flash bar
                { 0x00000000, 0xFFA20058, 0xFFFFFFFF, 0xFFA20058, 0x00000000 },
            // health bar
                { 0xFF2C5D71, 0xFF5E81AE, 0xFF2C5D71, 0xFF1B4557, 0xFF16304F },
            // oxygen bar
                { 0xFF647464, 0xFFA47848, 0xFF647464, 0xFF4C504C, 0xFF303030 },
            // option bar
                { 0x00000000, 0x20202020, 0x20202020, 0x20202020, 0x00000000,
                  0x00000000, 0x60606060, 0x60606060, 0x60606060, 0x00000000,
                  0x00000000, 0x80808080, 0x80808080, 0x80808080, 0x00000000,
                  0x00000000, 0x60606060, 0x60606060, 0x60606060, 0x00000000,
                  0x00000000, 0x20202020, 0x20202020, 0x20202020, 0x00000000 },
            // white bar (white tile)
                { 0xFFFFFFFF },
            };

        int stride = 256, uvCount;
        short2 *uv = NULL;

        Level *owner = (Level*)userData;
        TR::Level *level = &owner->level;

        Color32 *src, *dst = (Color32*)data;
        short4 mm;

        bool isSprite = false;

        if (id < level->objectTexturesCount) { // textures
            TR::TextureInfo &t = level->objectTextures[id];
            mm      = t.getMinMax();
            src     = owner->tileData->color;
            uv      = t.texCoordAtlas;
            uvCount = 4;
            if (data) {
                level->fillObjectTexture(owner->tileData, tile.uv, tile.tex);
            }
        } else {
            id -= level->objectTexturesCount;

            if (id < level->spriteTexturesCount) { // sprites
                TR::TextureInfo &t = level->spriteTextures[id];
                mm       = t.getMinMax();
                src      = owner->tileData->color;
                uv       = t.texCoordAtlas;
                uvCount  = 2;
                isSprite = true;
                if (data) {
                    level->fillObjectTexture(owner->tileData, tile.uv, tile.tex);
                }
            } else { // common (generated) textures
                id -= level->spriteTexturesCount;

                TR::TextureInfo *tex;
                mm.x = mm.y = mm.z = mm.w = 0;
                stride  = 1;
                uvCount = 4;

                switch (id) {
                    case UI::BAR_FLASH    :
                    case UI::BAR_HEALTH   :
                    case UI::BAR_OXYGEN   : 
                    case UI::BAR_OPTION   :
                    case UI::BAR_WHITE    :
                        src  = (Color32*)&barColor[id][0];
                        tex  = &barTile[id];
                        if (id != UI::BAR_WHITE) {
                            mm.w = 4; // height - 1
                            if (id == UI::BAR_OPTION) {
                                stride = 5;
                                mm.z   = 4;
                            }
                        }
                        break;
                    default : return;
                }

                memset(tex, 0, sizeof(*tex));
                uv = tex->texCoordAtlas;
                uv[2].y += mm.w;
                uv[3].y += mm.w;
                uv[1].x += mm.z;
                uv[2].x += mm.z;
            }
        }

        int cx = -mm.x, cy = -mm.y;

        if (data) {
            int w = mm.z - mm.x + 1;
            int h = mm.w - mm.y + 1;
            dst += tileY * atlasWidth + tileX;
            for (int y = -ATLAS_BORDER; y < h + ATLAS_BORDER; y++) {
                for (int x = -ATLAS_BORDER; x < w + ATLAS_BORDER; x++) {
                    Color32 *p = &src[mm.y * stride + mm.x];
                    ASSERT((x + ATLAS_BORDER + tileX) >= 0 && (x + ATLAS_BORDER + tileX) < atlasWidth);
                    ASSERT((y + ATLAS_BORDER + tileY) >= 0 && (y + ATLAS_BORDER + tileY) < atlasHeight);
                    p += clamp(x, 0, w - 1);
                    p += clamp(y, 0, h - 1) * stride;

                    if (isSprite && (y < 0 || y >= h || x < 0 || x >= w)) {
                        dst[x + ATLAS_BORDER] = Color32(0, 0, 0, 0);
                    } else {
                        dst[x + ATLAS_BORDER] = *p;
                    }
                }
                dst += atlasWidth;
            }

            cx += tileX + ATLAS_BORDER;
            cy += tileY + ATLAS_BORDER;
        }

        for (int i = 0; i < uvCount; i++) {
            if (uv[i].x == mm.z) uv[i].x++;
            if (uv[i].y == mm.w) uv[i].y++;
            uv[i].x += cx;
            uv[i].y += cy;

            uv[i].x = int32(uv[i].x) * 32767 / atlasWidth;
            uv[i].y = int32(uv[i].y) * 32767 / atlasHeight;
        }

    // apply ref for instanced tile
        if (data) return;

        int ref = tileX;
        if (ref < level->objectTexturesCount) { // textures
            mm = level->objectTextures[ref].getMinMaxAtlas();
        } else {
            ref -= level->objectTexturesCount;
            if (ref < level->spriteTexturesCount) // sprites
                mm = level->spriteTextures[ref].getMinMaxAtlas();
            else
                ASSERT(false); // only object textures and sprites may be instanced
        }

        for (int i = 0; i < uvCount; i++) {
            uv[i].x += mm.x;
            uv[i].y += mm.y;
        }
    }
/*
    void dumpGlyphs() {
        TR::SpriteSequence &seq = level.spriteSequences[level.extra.glyphs];
        short2 size = short2(0, 0);
        for (int i = 0; i < seq.sCount; i++) {
            TR::TextureInfo &sprite = level.spriteTextures[seq.sStart + i];
            short w = sprite.texCoord[1].x - sprite.texCoord[0].x + 1;
            short h = sprite.texCoord[1].y - sprite.texCoord[0].y + 1;
            size.y += h + 1;
            size.x = max(size.x, w);
        }
        size.x += 1;
        size.y += 1;
        size.x = (size.x + 3) / 4 * 4;
        Color32 *data = new Color32[int(size.x) * int(size.y)];
        memset(data, 128, int(size.x) * int(size.y) * 4);
        
        short2 pos = short2(1, 1);
        for (int i = 0; i < seq.sCount; i++) {
            TR::TextureInfo &sprite = level.spriteTextures[seq.sStart + i];
            short w = sprite.texCoord[1].x - sprite.texCoord[0].x + 1;
            short h = sprite.texCoord[1].y - sprite.texCoord[0].y + 1;

            for (int y = 0; y < h; y++)
                for (int x = 0; x < w; x++) {
                    TR::Tile32 &tile = level.tiles[sprite.tile];

                    data[pos.x + x + (pos.y + y) * size.x] = tile.color[sprite.texCoord[0].x + x + (sprite.texCoord[0].y + y) * 256];
                }
            pos.y += h + 1;
        }

        Texture::SaveBMP("psx_glyph.bmp", (char*)data, size.x, size.y);
        delete[] data;
    }
*/
    void initTextures() {
    #ifndef SPLIT_BY_TILE

        #ifdef _OS_PSP
            #error atlas packing is not allowed for this platform
        #endif

        //dumpGlyphs();

    // repack texture tiles
        Atlas *tiles = new Atlas(level.objectTexturesCount + level.spriteTexturesCount + UI::BAR_MAX, this, fillCallback);
        // add textures
        for (int i = 0; i < level.objectTexturesCount; i++) {
            TR::TextureInfo &t = level.objectTextures[i];
            if (t.tile == 0xFFFF) continue;

            short4 uv;
            uv.x = min(min(t.texCoord[0].x, t.texCoord[1].x), t.texCoord[2].x);
            uv.y = min(min(t.texCoord[0].y, t.texCoord[1].y), t.texCoord[2].y);
            uv.z = max(max(t.texCoord[0].x, t.texCoord[1].x), t.texCoord[2].x) + 1;
            uv.w = max(max(t.texCoord[0].y, t.texCoord[1].y), t.texCoord[2].y) + 1;

            tiles->add(i, uv, &t);
        }
        // add sprites
        for (int i = 0; i < level.spriteTexturesCount; i++) {
            TR::TextureInfo &t = level.spriteTextures[i];
            if (t.tile == 0xFFFF) continue;

            short4 uv;
            uv.x = t.texCoord[0].x;
            uv.y = t.texCoord[0].y;
            uv.z = t.texCoord[1].x + 1;
            uv.w = t.texCoord[1].y + 1;

            tiles->add(level.objectTexturesCount + i, uv, &t);
        }
        // add common textures
        const short2 bar[UI::BAR_MAX] = { short2(0, 4), short2(0, 4), short2(0, 4), short2(4, 4), short2(0, 0) };
        for (int i = 0; i < UI::BAR_MAX; i++) {
            barTile[i].type = TR::TEX_TYPE_SPRITE;
            tiles->add(level.objectTexturesCount + level.spriteTexturesCount + i, short4(i * 32, 4096, i * 32 + bar[i].x, 4096 + bar[i].y), &barTile[i]);
        }

        // get result texture
        tileData = new TR::Tile32();
        
        atlas = tiles->pack();
        delete[] tileData;
        tileData = NULL;

        atlas->setFilterQuality(Core::settings.detail.filter);

        delete tiles;

        LOG("atlas: %d x %d\n", atlas->width, atlas->height);
        PROFILE_LABEL(TEXTURE, atlas->ID, "atlas");

    #else
        ASSERT(level.tilesCount);

        #ifdef _OS_PSP
            atlas = new Texture(level.tiles4, level.tilesCount, level.cluts, level.clutsCount);
        #else
            Texture::Tile *tiles = new Texture::Tile[level.tilesCount];
            for (int i = 0; i < level.tilesCount; i++) {
                tiles[i].width = tiles[i].height = 256;
                tiles[i].data = new uint32[256 * 256];
            }

            for (int i = 0; i < level.objectTexturesCount; i++) {
                TR::TextureInfo &t = level.objectTextures[i];
                short4 uv = t.getMinMax();
                uv.z++;
                uv.w++;
                level.fillObjectTexture((TR::Tile32*)tiles[t.tile].data, uv, &t);
            }

            for (int i = 0; i < level.spriteTexturesCount; i++) {
                TR::TextureInfo &t = level.spriteTextures[i];
                short4 uv = t.getMinMax();
                uv.z++;
                uv.w++;
                level.fillObjectTexture((TR::Tile32*)tiles[t.tile].data, uv, &t);
            }

            for (int i = 0; i < level.tilesCount; i++) {
                char buf[256];
                sprintf(buf, "texture/%s_%d.png", TR::LEVEL_INFO[level.id].name, i);
                if (Stream::exists(buf)) {
                    delete[] tiles[i].data;
                    tiles[i].data = (uint32*)Texture::LoadPNG(Stream(buf), tiles[i].width, tiles[i].height);
                }
            }

            atlas = new Texture(tiles, level.tilesCount);

            for (int i = 0; i < level.tilesCount; i++)
                delete[] tiles[i].data;
            delete[] tiles;
        #endif

        for (int i = 0; i < level.objectTexturesCount; i++) {
            TR::TextureInfo &t = level.objectTextures[i];

            t.texCoordAtlas[0].x <<= 7;
            t.texCoordAtlas[0].y <<= 7;
            t.texCoordAtlas[1].x <<= 7;
            t.texCoordAtlas[1].y <<= 7;
            t.texCoordAtlas[2].x <<= 7;
            t.texCoordAtlas[2].y <<= 7;
            t.texCoordAtlas[3].x <<= 7;
            t.texCoordAtlas[3].y <<= 7;

            t.texCoordAtlas[0].x += 64;
            t.texCoordAtlas[0].y += 64;
            t.texCoordAtlas[1].x += 64;
            t.texCoordAtlas[1].y += 64;
            t.texCoordAtlas[2].x += 64;
            t.texCoordAtlas[2].y += 64;
            t.texCoordAtlas[3].x += 64;
            t.texCoordAtlas[3].y += 64;
        }

        for (int i = 0; i < level.spriteTexturesCount; i++) {
            TR::TextureInfo &t = level.spriteTextures[i];

            t.texCoordAtlas[0].x <<= 7;
            t.texCoordAtlas[0].y <<= 7;
            t.texCoordAtlas[1].x <<= 7;
            t.texCoordAtlas[1].y <<= 7;
            /*
            t.texCoord[0].x += 16;
            t.texCoord[0].y += 16;
            t.texCoord[1].x += 16;
            t.texCoord[1].y += 16;
            */
        }
    #endif
    }

    void initReflections() {
        for (int i = 0; i < level.entitiesBaseCount; i++) {
            TR::Entity &e = level.entities[i];
            if (e.type == TR::Entity::CRYSTAL) {
                Crystal *c = (Crystal*)e.controller;
                renderEnvironment(c->getRoomIndex(), c->pos - vec3(0, 512, 0), &c->environment);
                c->environment->generateMipMap();
            }
        }
    }

    void setMainLight(Controller *controller) {
        Core::lightPos[0]   = controller->mainLightPos;
        Core::lightColor[0] = vec4(controller->mainLightColor.xyz(), 1.0f / controller->mainLightColor.w);
    }

    void renderSky() {
        ASSERT(mesh->transparent == 0);

        Shader::Type type;
        TR::SkyParams skyParams;

        if (level.version & TR::VER_TR1) {
            if (Core::settings.detail.lighting < Core::Settings::HIGH || !Core::support.tex3D || !TR::getSkyParams(level.id, skyParams))
                return;
            type = Shader::SKY_CLOUDS_AZURE;
        } else { // TR2, TR3
            if (level.extra.sky == -1)
                return;

            if (Core::settings.detail.lighting < Core::Settings::HIGH || !Core::support.tex3D) {
                type = Shader::DEFAULT;
            } else {
                type = Shader::SKY_CLOUDS;
                if (!TR::getSkyParams(level.id, skyParams)) {
                    type = Shader::DEFAULT;
                }
            }
        }

        if (type != Shader::DEFAULT && !Core::perlinTex) {
            type = Shader::DEFAULT;
            if (level.version & TR::VER_TR1) {
                return;
            }
        }

        Core::Pass pass = Core::pass;
        mat4 mView = Core::mView;
        mat4 mProj = Core::mProj;

        Core::mView.setPos(vec3(0));
        Core::setViewProj(Core::mView, Core::mProj);

        setShader(Core::passSky, type, false, false);

        if (type != Shader::DEFAULT) {
            float time = Core::params.x;
            if (time > SKY_TIME_PERIOD) {
                time /= SKY_TIME_PERIOD;
                time = (time - int(time)) * SKY_TIME_PERIOD;
            }

            Core::active.shader->setParam(uParam,     vec4(skyParams.wind * time, 1.0));
            Core::active.shader->setParam(uLightProj, *(mat4*)&skyParams);
            Core::active.shader->setParam(uPosScale,  skyParams.cloudDownColor, 2);

            Core::perlinTex->bind(sNormal);
            Core::ditherTex->bind(sMask);
        }

        if (level.version & TR::VER_TR1) {
            Core::setCullMode(cmNone);
            mesh->renderBox();
            Core::setCullMode(cmFront);
        } else {
            Basis b;
            Core::setBasis(&b, 1); // unused
            mesh->renderModel(level.extra.sky);
        }

        Core::setViewProj(mView, mProj);
        Core::pass = pass;
    }

    void prepareRooms(int *roomsList, int roomsCount) {
        skyIsVisible = (level.version & TR::VER_TR1);

        for (int i = 0; i < level.roomsCount; i++)
            level.rooms[i].flags.visible = false;

        for (int i = 0; i < roomsCount; i++) {
            TR::Room &r = level.rooms[roomsList[i]];
            skyIsVisible |= r.flags.sky;
            r.flags.visible = true;
        }

        if (Core::pass == Core::passShadow)
            return;

        if (Core::settings.detail.shadows > Core::Settings::MEDIUM) {
            Sphere spheres[MAX_CONTACTS];
            int count = player->getSpheres(spheres);

            for (int i = 0; i < MAX_CONTACTS; i++)
                if (i < count)
                    Core::contacts[i] = vec4(spheres[i].center, PI * spheres[i].radius * spheres[i].radius * 0.25f);
                else
                    Core::contacts[i] = vec4(0.0f);
        }

        setMainLight(player);
    }

    void renderRooms(int *roomsList, int roomsCount, int transp) {
        PROFILE_MARKER("ROOMS");

        if (Core::pass == Core::passShadow)
            return;

        Basis basis;
        basis.identity();

        Core::mModel.identity();

        switch (transp) {
            case 0 : Core::setBlendMode(bmNone);    break;
            case 1 : Core::setBlendMode(bmPremult); break;
            case 2 : Core::setBlendMode(bmAdd);   Core::setDepthWrite(false); break;
        }

        int i     = 0;
        int end   = roomsCount;
        int dir   = 1;

        if (transp) {
            i   = roomsCount - 1;
            end = -1;
            dir = -1;
        }

        while (i != end) {
            int roomIndex = roomsList[i];
            MeshBuilder::RoomRange &range = mesh->rooms[roomIndex];

            if (!range.geometry[transp].count && !range.dynamic[transp].count) {
                i += dir;
                continue;
            }

            const TR::Room &room = level.rooms[roomIndex];

            vec3 center = room.getCenter();
            int ambient = room.getAmbient(int(center.x), int(center.y), int(center.z));

            setRoomParams(roomIndex, Shader::ROOM, 1.0f, intensityf(ambient), 0.0f, 1.0f, transp == 1);

            basis.pos = room.getOffset();
            Core::setBasis(&basis, 1);

            Core::mModel.setPos(basis.pos);

            mesh->transparent = transp;
            mesh->renderRoomGeometry(roomIndex);

            i += dir;
        }

        Core::setDepthWrite(true);

        if (transp == 1) {
            Core::setBlendMode(bmPremult);

            #ifdef MERGE_SPRITES
                basis.rot = Core::mViewInv.getRot();
            #else
                basis.rot = quat(0, 0, 0, 1);
            #endif

            for (int i = 0; i < roomsCount; i++) {
                level.rooms[roomsList[i]].flags.visible = true;

                int roomIndex = roomsList[i];
                MeshBuilder::RoomRange &range = mesh->rooms[roomIndex];

                if (!range.sprites.iCount)
                    continue;

                setRoomParams(roomIndex, Shader::SPRITE, 1.0f, 1.0f, 0.0f, 1.0f, true);

                basis.pos = level.rooms[roomIndex].getOffset();
                Core::setBasis(&basis, 1);

                mesh->renderRoomSprites(roomIndex);
            }
        }

        Core::setBlendMode(bmNone);
    }

    void renderEntity(const TR::Entity &entity) {
        //if (entity.room != lara->getRoomIndex()) return;
        if (Core::pass == Core::passShadow && !entity.castShadow()) return;

        Controller *controller = (Controller*)entity.controller;
        int roomIndex = controller->getRoomIndex();
        TR::Room &room = level.rooms[roomIndex];

        if (controller->flags.invisible)
            return;

        if (!entity.isLara() && !entity.isActor() && !room.flags.visible)
            return;

        bool isModel;

        if (entity.type != TR::Entity::TRAP_LAVA_EMITTER) {
            isModel = entity.modelIndex > 0;
            if (isModel) {
                if (!mesh->models[controller->getModel()->index].geometry[mesh->transparent].count) return;
            } else {
                if (level.spriteSequences[-(entity.modelIndex + 1)].transp != mesh->transparent) return;
            }
        } else {
            if (mesh->transparent != 2) {
                return;
            }
            isModel = false;
        }

        Shader::Type type = isModel ? Shader::ENTITY : Shader::SPRITE;
        if (entity.type == TR::Entity::CRYSTAL)
            type = Shader::MIRROR;

        if (isModel) { // model
            ASSERT(controller->intensity >= 0.0f);

            setMainLight(controller);
            setRoomParams(roomIndex, type, 1.0f, controller->intensity, controller->specular, 1.0f, mesh->transparent == 1);

            vec3 pos = controller->getPos();
            if (ambientCache) {
                if (!entity.isDoor() && !entity.isBlock()) { // no advanced ambient lighting for secret (all) doors and blocks
                    AmbientCache::Cube cube;
                    ambientCache->getAmbient(roomIndex, pos, cube);
                    if (cube.status == AmbientCache::Cube::READY)
                        memcpy(controller->ambient, cube.colors, sizeof(cube.colors)); // store last calculated ambient into controller
                } else {
                    controller->ambient[0] =
                    controller->ambient[1] =
                    controller->ambient[2] =
                    controller->ambient[3] =
                    controller->ambient[4] =
                    controller->ambient[5] = vec4(Core::active.material.y);
                }
                Core::active.shader->setParam(uAmbient, controller->ambient[0], 6);
            }
        }

        controller->render(camera->frustum, mesh, type, room.flags.water);
    }

    void update() {
        if (isEnded) return;

        bool invRing = inventory->phaseRing != 0.0f && inventory->phaseRing != 1.0f;
        if (inventory->video || inventory->titleTimer >= 1.0f || level.isCutsceneLevel() || invRing) {
            memset(Input::btnEnable, 0, sizeof(Input::btnEnable));
            Input::btnEnable[Input::bInventory] = !invRing;
        } else
            memset(Input::btnEnable, 1, sizeof(Input::btnEnable));

        Input::btnEnable[Input::bWeapon] &= players[0] && players[0]->canDrawWeapon();

        if (inventory->isActive())
            Input::btnEnable[Input::bWalk] = Input::btnEnable[Input::bJump] = Input::btnEnable[Input::bWeapon] = false;

        if (inventory->video) {
            inventory->update();
            return;
        }

        if (level.isCutsceneLevel() && waitTrack) {
            if (!sndTrack && TR::LEVEL_INFO[level.id].track != TR::NO_TRACK) {
                if (camera->timer > 0.0f) // for the case that audio stops before animation ends
                    loadNextLevel();
                return;
            }

            if (cutsceneWaitTimer > 0.0f) {
                cutsceneWaitTimer -= Core::deltaTime;
                if (cutsceneWaitTimer > 0.0f)
                    return;
                if (sndTrack)
                    sndTrack->setVolume(1.0f, 0.0f);
                cutsceneWaitTimer = 0.0f;
                Core::resetTime();
                LOG("reset timer - %d\n", Core::getTime());
                return;
            }
        }

        if ((Input::lastState[0] == cInventory || Input::lastState[1] == cInventory) && !level.isTitle() && inventory->titleTimer < 1.0f && !inventory->active) {
            int playerIndex = (Input::lastState[0] == cInventory) ? 0 : 1;

            if (getLara(playerIndex)) {
                if (level.isCutsceneLevel()) { // skip cutscene level
                    loadNextLevel();
                    return;
                }

                if (player->health <= 0.0f)
                    inventory->toggle(playerIndex, Inventory::PAGE_OPTION, TR::Entity::INV_PASSPORT);
                else
                    inventory->toggle(playerIndex);
            }
        }

        bool invActive = inventory->isActive();

        inventory->update();

        if (inventory->titleTimer > 1.0f)
            return;

        if (nextLevel != TR::LVL_MAX && !inventory->isActive()) {
            if (showStats) {
                inventory->toggle(0, Inventory::PAGE_LEVEL_STATS);
                showStats = false;
                return;
            }
            isEnded = true;
            char buf[64];
            TR::getGameLevelFile(buf, level.version, nextLevel);
            nextLevel = TR::LVL_MAX;
            new Stream(buf, loadLevelAsync);
            return;
        }

        if (loadSlot > -1 && nextLevel == TR::LVL_MAX) {
            if (inventory->isActive())
                return;

            loadLevel(saveSlots[loadSlot].getLevelID());
            return;
        }

        if (!inventory->isActive()) {
            UI::update();
        }

        float volWater, volTrack;

        if (invActive || level.isTitle()) {
            Sound::reverb.setRoomSize(vec3(1.0f));
            volWater = 0.0f;
            volTrack = level.isTitle() ? 0.9f : 0.0f;
        } else {
            
            if (!level.isCutsceneLevel()) {
                statsTimeDelta += Core::deltaTime;
                while (statsTimeDelta >= 1.0f) {
                    statsTimeDelta -= 1.0f;
                    saveStats.time++;
                }
            }

            params->time += Core::deltaTime;
            animTexTimer += Core::deltaTime;

            float timeStep = ANIM_TEX_TIMESTEP;
            if (level.version & TR::VER_TR1)
                timeStep *= 0.5f;

            if (animTexTimer > timeStep) {
                level.shiftAnimTex();
                animTexTimer -= timeStep;
            }

            updateEffect();

            Controller *c = Controller::first;
            while (c) {
                Controller *next = c->next;
                c->update();
                c = next;
            }

            if (waterCache) 
                waterCache->update();

            Controller::clearInactive();

        // underwater ambient sound volume control
            if (camera->isUnderwater()) {
                if (!sndWater && !level.isCutsceneLevel()) {
                    sndWater = playSound(TR::SND_UNDERWATER, vec3(0.0f), Sound::LOOP | Sound::MUSIC);
                    if (sndWater)
                        sndWater->volume = sndWater->volumeTarget = 0.0f;
                }
                volWater = 1.0f;
            } else 
                volWater = 0.0f;

            volTrack = 1.0f;
        }

        if (sndWater && sndWater->volumeTarget != volWater)
            sndWater->setVolume(volWater, 0.2f);
        if (sndTrack && sndTrack->volumeTarget != volTrack)
            sndTrack->setVolume(volTrack, 0.2f);

    #ifdef _DEBUG
    #ifdef _NAPI_SOCKET
        if (Input::down[ikJ]) {
            //Network::sayHello();
            NAPI::Peer peer;
            peer.ip = ((93 << 0) | (92 << 8) | (200 << 16) | (163 << 24));
            peer.port = htons(10994);
            LOG("join %s:%d\n", inet_ntoa(*(in_addr*)&peer.ip), ntohs(peer.port));
            Network::joinGame(peer);
            Input::down[ikJ] = false;
        }

        if (Input::down[ikY]) {
            NAPI::requestAddress();
            Input::down[ikY] = false;
        }
    #endif        
    #endif
    }

    void updateEffect() {
        if (effect == TR::Effect::NONE)
            return;

        effectTimer += Core::deltaTime;

        switch (effect) {
            case TR::Effect::TR1_FLICKER : {
                int idx = effectIdx;
                switch (effectIdx) {
                    case 0 : if (effectTimer > 3.0f) effectIdx++; break;
                    case 1 : if (effectTimer > 3.1f) effectIdx++; break;
                    case 2 : if (effectTimer > 3.5f) effectIdx++; break;
                    case 3 : if (effectTimer > 3.6f) effectIdx++; break;
                    case 4 : if (effectTimer > 4.1f) { effectIdx++; effect = TR::Effect::NONE; } break;
                }
                if (idx != effectIdx)
                    flipMap();
                break;
            }
            case TR::Effect::EARTHQUAKE : {
                switch (effectIdx) {
                    case 0 : if (effectTimer > 0.0f) { playSound(TR::SND_ROCK);     effectIdx++; shakeCamera(1.0f); } break;
                    case 1 : if (effectTimer > 0.1f) { playSound(TR::SND_STOMP);    effectIdx++; } break;
                    case 2 : if (effectTimer > 0.6f) { playSound(TR::SND_BOULDER);  effectIdx++; shakeCamera(0.5f, true); } break;
                    case 3 : if (effectTimer > 1.1f) { playSound(TR::SND_ROCK);     effectIdx++; } break;
                    case 4 : if (effectTimer > 1.6f) { playSound(TR::SND_BOULDER);  effectIdx++; shakeCamera(0.5f, true); } break;
                    case 5 : if (effectTimer > 2.3f) { playSound(TR::SND_BOULDER);  shakeCamera(0.5f, true); effect = TR::Effect::NONE; } break;
                }
                break;
            }
            default : effect = TR::Effect::NONE; return;
        }
    }

    void setup() {
        camera->setup(Core::pass == Core::passCompose);
        setupBinding();
    }

    void renderEntitiesTransp(int transp) {
        mesh->dynBegin();
        mesh->transparent = transp;
        for (int i = 0; i < level.entitiesCount; i++) {
            TR::Entity &e = level.entities[i];
            if (!e.controller || e.modelIndex == 0) continue;
            renderEntity(e);
        }

        {
            PROFILE_MARKER("ENTITY_SPRITES");

            if (mesh->dynICount) {
                Core::lightPos[0]   = vec4(0, 0, 0, 0);
                Core::lightColor[0] = vec4(0, 0, 0, 1);
                setRoomParams(getLara()->getRoomIndex(), Shader::SPRITE, 1.0f, 1.0f, 0.0f, 1.0f, mesh->transparent == 1);

                Basis b;
                b.w   = 1.0f;
                b.pos = Core::viewPos.xyz();
                #ifdef MERGE_SPRITES
                    b.rot = Core::mViewInv.getRot();
                #else
                    b.rot = quat(0, 0, 0, 1);
                #endif
                Core::setBasis(&b, 1);
            }

            mesh->dynEnd();
        }
    }

    void renderEntities(int transp) {
        if (Core::pass == Core::passAmbient) // TODO allow static entities
            return;

        PROFILE_MARKER("ENTITIES");

        if (transp == 0) {
            Core::setBlendMode(bmNone);
            renderEntitiesTransp(transp);
        }

        if (transp == 1) {
            Core::setBlendMode(bmPremult);
            renderEntitiesTransp(transp);

            #ifdef FFP
                Core::whiteTex->bind(0);
            #endif

            Core::setBlendMode(bmMult);
            for (int i = 0; i < level.entitiesCount; i++) {
                TR::Entity &entity = level.entities[i];
                Controller *controller = (Controller*)entity.controller;
                if (controller && controller->flags.rendered && controller->getEntity().castShadow())
                    controller->renderShadow(mesh);
            }
            Core::setBlendMode(bmNone);

            #ifdef FFP
                atlas->bind(0);
            #endif
        }

        if (transp == 2) {
            Core::setDepthWrite(false);
            Core::setBlendMode(bmAdd);
            renderEntitiesTransp(transp);
            Core::setDepthWrite(true);
        }
    }

    bool checkPortal(const TR::Room &room, const TR::Room::Portal &portal, const vec4 &viewPort, vec4 &clipPort) {
        vec3 n = portal.normal;
        vec3 v = Core::viewPos.xyz() - (room.getOffset() + portal.vertices[0]);

        if (n.dot(v) <= 0.0f)
            return false;

        int  zClip = 0;
        vec4 p[4];

        clipPort = vec4(INF, INF, -INF, -INF);

        for (int i = 0; i < 4; i++) {
            p[i] = Core::mViewProj * vec4(vec3(portal.vertices[i]) + room.getOffset(), 1.0f);

            if (p[i].w > 0.0f) {
                p[i].xyz() *= (1.0f / p[i].w);

                clipPort.x = min(clipPort.x, p[i].x);
                clipPort.y = min(clipPort.y, p[i].y);
                clipPort.z = max(clipPort.z, p[i].x);
                clipPort.w = max(clipPort.w, p[i].y);
            } else
                zClip++;
        }

        if (zClip == 4)
            return false;

        if (zClip > 0) {
            for (int i = 0; i < 4; i++) {
                vec4 &a = p[i];
                vec4 &b = p[(i + 1) % 4];

                if ((a.w > 0.0f) ^ (b.w > 0.0f)) {

                    if (a.x < 0.0f && b.x < 0.0f)
                        clipPort.x = -1.0f;
                    else
                        if (a.x > 0.0f && b.x > 0.0f)
                            clipPort.z = 1.0f;
                        else {
                            clipPort.x = -1.0f;
                            clipPort.z =  1.0f;
                        }

                    if (a.y < 0.0f && b.y < 0.0f)
                        clipPort.y = -1.0f;
                    else
                        if (a.y > 0.0f && b.y > 0.0f)
                            clipPort.w = 1.0f;
                        else {
                            clipPort.y = -1.0f;
                            clipPort.w =  1.0f;
                        }

                }
            }
        }

        if (clipPort.x > viewPort.z || clipPort.y > viewPort.w || clipPort.z < viewPort.x || clipPort.w < viewPort.y)
            return false;

        clipPort.x = max(clipPort.x, viewPort.x);
        clipPort.y = max(clipPort.y, viewPort.y);
        clipPort.z = min(clipPort.z, viewPort.z);
        clipPort.w = min(clipPort.w, viewPort.w);

        return true;
    }

    virtual void getVisibleRooms(int *roomsList, int &roomsCount, int from, int to, const vec4 &viewPort, bool water, int count = 0) {
        if (count > 16) {
            //ASSERT(false);
            return;
        }

        TR::Room &room = level.rooms[to];

        if (!room.flags.visible) {
            if (Core::pass == Core::passCompose && water && waterCache && from != TR::NO_ROOM && (level.rooms[from].flags.water ^ level.rooms[to].flags.water))
                waterCache->setVisible(from, to);

            room.flags.visible = true;
            roomsList[roomsCount++] = to;
        }

        vec4 clipPort;
        for (int i = 0; i < room.portalsCount; i++) {
            TR::Room::Portal &p = room.portals[i];

            if (Core::pass == Core::passCompose && water && waterCache && (level.rooms[to].flags.water ^ level.rooms[p.roomIndex].flags.water))
                waterCache->setVisible(to, p.roomIndex);

            if (from != room.portals[i].roomIndex && checkPortal(room, p, viewPort, clipPort))
                getVisibleRooms(roomsList, roomsCount, to, p.roomIndex, clipPort, water, count + 1);
        }
    }

    void renderOpaque(int *roomsList, int roomsCount) {
        renderRooms(roomsList, roomsCount, 0);
        renderEntities(0);
        if (Core::pass != Core::passShadow && skyIsVisible) {
            renderSky();
        }
    }

    void renderTransparent(int *roomsList, int roomsCount) {
        renderRooms(roomsList, roomsCount, 1);
        renderEntities(1);
    }

    void renderAdditive(int *roomsList, int roomsCount) {
        vec4 oldFog = Core::fogParams;
        Core::fogParams = FOG_BLACK; // don't apply fog for additive 
        renderRooms(roomsList, roomsCount, 2);
        renderEntities(2);
        Core::fogParams = oldFog;
    }

    virtual void renderView(int roomIndex, bool water, int roomsCount = 0, int *roomsList = NULL) {
        PROFILE_MARKER("VIEW");

        if (water && waterCache)
            waterCache->reset();

        int rList[256];

        if (!roomsList) {
            roomsList = rList;

            // mark all rooms as invisible
            for (int i = 0; i < level.roomsCount; i++)
                level.rooms[i].flags.visible = false;

            if (level.isCutsceneLevel()) { // render all rooms except flipped
            // hide alternative rooms from getVisibleRooms
                for (int i = 0; i < level.roomsCount; i++) {
                    int flipIndex = level.rooms[i].alternateRoom;
                    if (flipIndex > -1)
                        level.rooms[flipIndex].flags.visible = true;
                }

            // get room list through portals (it will change room visible flag)
                getVisibleRooms(roomsList, roomsCount, TR::NO_ROOM, roomIndex, vec4(-1.0f, -1.0f, 1.0f, 1.0f), water);

            // add other non-alternative rooms
                for (int i = 0; i < level.roomsCount; i++)
                    if (!level.rooms[i].flags.visible)
                        roomsList[roomsCount++] = i;

            // refresh visible flag
                for (int i = 0; i < level.roomsCount; i++)
                    level.rooms[i].flags.visible = false;

                for (int i = 0; i < roomsCount; i++)
                    level.rooms[roomsList[i]].flags.visible = true;
            } else
                getVisibleRooms(roomsList, roomsCount, TR::NO_ROOM, roomIndex, vec4(-1.0f, -1.0f, 1.0f, 1.0f), water);
        }

        if (water && waterCache) {
            for (int i = 0; i < roomsCount; i++)
                waterCache->setVisible(roomsList[i]);

            waterCache->renderReflection();

            Core::Pass pass = Core::pass;
            waterCache->simulate();
            Core::pass = pass;
        }

        // clear entity rendered flag (used for blob shadows)
        if (Core::pass != Core::passAmbient)
            for (int i = 0; i < level.entitiesCount; i++) {
                Controller *controller = (Controller*)level.entities[i].controller;
                if (controller)
                    controller->flags.rendered = false;
            }

        Texture *screen = NULL;
        if (water) {
            screen = (waterCache && waterCache->visible) ? waterCache->getScreenTex() : NULL;
            Core::setTarget(screen, NULL, RT_CLEAR_COLOR | RT_CLEAR_DEPTH | RT_STORE_COLOR | (screen ? RT_STORE_DEPTH : 0)); // render to screen texture (FUCK YOU iOS!) or back buffer
            setupBinding();
        }

        prepareRooms(roomsList, roomsCount);

        renderOpaque(roomsList, roomsCount);
        renderTransparent(roomsList, roomsCount);

        if (camera->isUnderwater())
            renderAdditive(roomsList, roomsCount);

        Core::setBlendMode(bmNone);
        if (water && waterCache && waterCache->visible) {
            Core::Pass pass = Core::pass;
            if (!camera->isUnderwater())
                waterCache->renderRays();
            waterCache->renderMask();
            waterCache->copyScreenToRefraction();
            setMainLight(player);
            waterCache->compose();
            if (camera->isUnderwater())
                waterCache->renderRays();

            Core::pass = pass;
            setupBinding();
        }

        if (!camera->isUnderwater())
            renderAdditive(roomsList, roomsCount);
    
        Core::setBlendMode(bmNone);

        Core::Pass pass = Core::pass;

        if (water && waterCache && waterCache->visible && screen) {
            Core::setTarget(NULL, NULL, RT_STORE_COLOR);
            waterCache->blitTexture(screen);
        }

        Core::pass = pass;
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

        Core::mViewInv  = mat4(pos, pos + dir, up);
        Core::mView     = Core::mViewInv.inverseOrtho();
        Core::mProj     = GAPI::perspective(90, 1.0f, camera->znear, camera->zfar);
        Core::mViewProj = Core::mProj * Core::mView;
        Core::viewPos   = Core::mViewInv.offset().xyz();

        camera->setup(false);
    }

    void renderShadowView(int roomIndex) {
        vec3 pos = player->getBoundingBox().center();

        float znear = camera->znear;
        float zfar  = player->mainLightColor.w * 1.5f;

        Core::mViewInv = mat4(player->mainLightPos, pos, vec3(0, -1, 0));
        Core::mView    = Core::mViewInv.inverseOrtho();
        Core::mProj    = GAPI::perspective(90.0f, 1.0f, znear, zfar);

        Core::mLightProj = Core::mProj * Core::mView;

        mat4 bias;
        bias.identity();
        bias.e03 = bias.e13 = bias.e23 = bias.e00 = bias.e11 = bias.e22 = 0.5f;
    #if defined(_GAPI_D3D9) || defined(_GAPI_GXM)
        bias.e11 = -bias.e11;
    #endif
        Core::mLightProj = bias * Core::mLightProj;

        camera->frustum->pos = Core::viewPos.xyz();
        camera->frustum->calcPlanes(Core::mViewProj);

        setup();
        renderView(roomIndex, false);
    }
/*
    void renderShadowEntity(int index, Controller *controller, Controller *player) {
        Box box = controller->getSpheresBox(true);
        mat4 m = controller->getMatrix();

        vec3 pos = m * box.center();
        Core::mViewInv = mat4(player->mainLightPos, pos, vec3(0, -1, 0));
        Core::mView    = Core::mViewInv.inverseOrtho();
        Core::mProj    = mat4(90.0f, 1.0f, 1.0f, 2.0f);

        Box crop = box * (Core::mProj * Core::mView * m);
        crop.min.z = max(0.0f, crop.min.z);

        float sx =  2.0f / (crop.max.x - crop.min.x);
        float sy =  2.0f / (crop.max.y - crop.min.y);
        float sz =  2.0f / (crop.max.z - crop.min.z);
        float ox = -0.5f * (crop.max.x + crop.min.x) * sx;
        float oy = -0.5f * (crop.max.y + crop.min.y) * sy;
        float oz = -0.5f * (crop.max.z + crop.min.z) * sz;

        Core::mProj = mat4(sx,  0,    0,  0,
                            0,  sy,   0,  0,
                            0,   0,  sz,  0,
                            ox,  oy,  oz,  1) * Core::mProj;

        Core::setViewProj(Core::mView, Core::mProj);

        mat4 bias;
        bias.identity();
        bias.e00 = bias.e11 = bias.e22 = bias.e03 = bias.e13 = bias.e23 = 0.5f;
        Core::mLightProj[index] = bias * (Core::mProj * Core::mView);
        
        Core::setBlendMode(bmNone);

        mesh->transparent = 0;
        if (mesh->models[controller->getEntity().modelIndex - 1].geometry[mesh->transparent].count) {
            setShader(Core::pass, Shader::ENTITY, false, false);
            controller->render(NULL, mesh, Shader::ENTITY, false);
        }
        mesh->transparent = 1;
        if (mesh->models[controller->getEntity().modelIndex - 1].geometry[mesh->transparent].count) {
            setShader(Core::pass, Shader::ENTITY, false, true);
            controller->render(NULL, mesh, Shader::ENTITY, false);
        }
        mesh->transparent = 0;
    }

    struct NearObj {
        int   player;
        int   index;
        float dist;
        NearObj() {}
        NearObj(int player, int index, float dist) : player(player), index(index), dist(dist) {}
    };

    int getNearObjects(NearObj *nearObj, int maxCount) {
        int count = 0;

        nearObj[count++] = NearObj(0, players[0]->entity, 0.0f);
        if (players[1])
            nearObj[count++] = NearObj(1, players[1]->entity, 0.0f);
        int base = count;

        for (int i = 0; i < level.entitiesCount; i++) {
            TR::Entity &e = level.entities[i];
            Controller *controller = (Controller*)e.controller;
            if (controller && e.castShadow() && controller != players[0] && controller != players[1]) {
                int pIndex = 0;
                float dist = (players[0]->getPos() - controller->getPos()).length2();
                if (players[1]) {
                    float dist2 = (players[1]->getPos() - controller->getPos()).length2();
                    if (dist2 < dist) {
                        dist = dist2;
                        pIndex = 1;
                    }
                }

                if (dist > 8192 * 8192) continue;

            // get index to insert
                int index = base;
                while (index < count) {
                    if (dist < nearObj[index].dist)
                        break;
                    index++;
                }
            // insertion
                if (index < maxCount) {
                    if (count < maxCount)
                        count++;
                    for (int j = count - 1; j > index; j--)
                        nearObj[j] = nearObj[j - 1];
                    nearObj[index] = NearObj(pIndex, controller->entity, dist);
                }
            }
        }

        return count;
    }
*/
    void renderShadows(int roomIndex) {
        PROFILE_MARKER("PASS_SHADOW");

        if (Core::settings.detail.shadows == Core::Settings::LOW)
            return;
        ASSERT(shadow);

    // render to shadow map
        float oldEye = Core::eye;
        Core::eye = 0.0f;

        Core::pass = Core::passShadow;
        shadow->unbind(sShadow);
        bool colorShadow = shadow->fmt == FMT_RGBA ? true : false;
        if (colorShadow)
            Core::setClearColor(vec4(1.0f));
        Core::setTarget(shadow, NULL, RT_CLEAR_DEPTH | (colorShadow ? (RT_CLEAR_COLOR | RT_STORE_COLOR) : RT_STORE_DEPTH));
        //Core::setCullMode(cmBack);
        Core::validateRenderState();

        /*
        if (Core::settings.detail.shadows > Core::Settings::MEDIUM) { // per-object shadow map (atlas)
            NearObj nearObj[SHADOW_OBJ_MAX];
            int nearCount = getNearObjects(nearObj, SHADOW_OBJ_MAX);

            for (int i = 0; i < nearCount; i++) {
                Core::setViewport((i % SHADOW_OBJ_COLS) * SHADOW_TEX_TILE, (i / SHADOW_OBJ_COLS) * SHADOW_TEX_TILE, SHADOW_TEX_TILE, SHADOW_TEX_TILE);
                renderShadowEntity(i, (Controller*)level.entities[nearObj[i].index].controller, players[nearObj[i].player]);
            }

            for (int i = nearCount; i < SHADOW_OBJ_MAX; i++)
                Core::mLightProj[i].identity();
        } else // all-in-one shadow map
        */
        renderShadowView(roomIndex);

        //Core::setCullMode(cmFront);
        if (colorShadow)
            Core::setClearColor(vec4(0.0f));

        Core::eye = oldEye;
    }

    #ifdef DEBUG_RENDER
    void renderDebug() {
        if (level.isTitle() || inventory->titleTimer > 1.0f) return;

        Core::setViewport(Core::x, Core::y, Core::width, Core::height);
        camera->setup(true);

        Debug::begin();
        /*
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
        */

        /*
            glMatrixMode(GL_MODELVIEW);
            glPushMatrix();
            glLoadIdentity();
            glMatrixMode(GL_PROJECTION);
            glPushMatrix();
            glLoadIdentity();
            glOrtho(0, Core::width, 0, Core::height, 0, 1);

            if (shadow)
                shadow->bind(sDiffuse);
            else
                atlas->bind(sDiffuse);
            glEnable(GL_TEXTURE_2D);
            Core::setCullMode(cmNone);
            Core::setDepthTest(false);
            Core::setBlendMode(bmNone);
            Core::validateRenderState();

            glColor3f(10, 10, 10);
            float w = float(Core::active.textures[sDiffuse]->width);
            float h = float(Core::active.textures[sDiffuse]->height);
            glBegin(GL_QUADS);
                glTexCoord2f(0, 0); glVertex2f(0, 0);
                glTexCoord2f(1, 0); glVertex2f(w, 0);
                glTexCoord2f(1, 1); glVertex2f(w, h);
                glTexCoord2f(0, 1); glVertex2f(0, h);
            glEnd();
            glColor3f(1, 1, 1);

            glDisable(GL_TEXTURE_2D);

            glMatrixMode(GL_PROJECTION);
            glPopMatrix();
            glMatrixMode(GL_MODELVIEW);
            glPopMatrix();
        */

        /*
        Core::setDepthTest(false);
        glBegin(GL_LINES);
            glColor3f(1, 1, 1);
            glVertex3fv((GLfloat*)&lara->pos);
            glVertex3fv((GLfloat*)&lara->mainLightPos);
        glEnd();
        Core::setDepthTest(true);
        */
        //    Debug::Draw::sphere(lara->mainLightPos, lara->mainLightColor.w, vec4(1, 1, 0, 1));

        //    Box bbox = lara->getBoundingBox();
        //    Debug::Draw::box(bbox.min, bbox.max, vec4(1, 0, 1, 1));

            Core::setBlendMode(bmAlpha);
            Core::setDepthTest(false);
            Core::validateRenderState();
        //    Debug::Level::rooms(level, lara->pos, lara->getEntity().room);
        //     Debug::Level::lights(level, player->getRoomIndex(), player);
        //    Debug::Level::sectors(this, players[0]->getRoomIndex(), (int)players[0]->pos.y);
        //    Core::setDepthTest(false);
        //    Debug::Level::portals(level);
        //    Core::setDepthTest(true);
        //    Debug::Level::meshes(level);
        //    Debug::Level::entities(level);
        //    Debug::Level::zones(level, lara);
        //    Debug::Level::blocks(level);
        //    Debug::Level::path(level, (Enemy*)level.entities[105].controller);
        //    Debug::Level::debugOverlaps(level, lara->box);
        //    Debug::Level::debugBoxes(level, lara->dbgBoxes, lara->dbgBoxesCount);
            Core::setDepthTest(true);
            Core::setBlendMode(bmNone);
        /*// render ambient cube
            Core::validateRenderState();

            static int dbg_ambient = 0;
            dbg_ambient = int(params->time * 2) % 4;

            shadow->unbind(sShadow);
            Core::whiteCube->unbind(sEnvironment);

            glActiveTexture(GL_TEXTURE0);
            glEnable(GL_TEXTURE_2D);
            glDisable(GL_CULL_FACE);
            glColor3f(1, 1, 1);
            for (int j = 0; j < 6; j++) {
                glPushMatrix();
                glTranslatef(player->pos.x, player->pos.y - 1024, player->pos.z);
                switch (j) {
                    case 0 : glRotatef( 90, 0, 1, 0); break;
                    case 1 : glRotatef(-90, 0, 1, 0); break;
                    case 2 : glRotatef(-90, 1, 0, 0); break;
                    case 3 : glRotatef( 90, 1, 0, 0); break;
                    case 4 : glRotatef(  0, 0, 1, 0); break;
                    case 5 : glRotatef(180, 0, 1, 0); break;
                }
                glTranslatef(0, 0, 256);
                
                ambientCache->textures[j * 4 + dbg_ambient]->bind(sDiffuse);
                glBegin(GL_QUADS);
                    glTexCoord2f(0, 0); glVertex3f(-256,  256, 0);
                    glTexCoord2f(1, 0); glVertex3f( 256,  256, 0);
                    glTexCoord2f(1, 1); glVertex3f( 256, -256, 0);
                    glTexCoord2f(0, 1); glVertex3f(-256, -256, 0);
                glEnd();
                glPopMatrix();
            }
            glEnable(GL_CULL_FACE);
            glDisable(GL_TEXTURE_2D);


            glLineWidth(4);
            glBegin(GL_LINES);
            float S = 64.0f;
            for (int i = 0; i < level.roomsCount; i++) {
                if (i != players[0]->getRoomIndex()) continue;
                TR::Room &r = level.rooms[i];                
                for (int j = 0; j < r.xSectors * r.zSectors; j++) {
                    TR::Room::Sector &s = r.sectors[j];
                    vec3 p = vec3(float((j / r.zSectors) * 1024 + 512 + r.info.x),
                                  float(max((s.floor - 2) * 256, (s.floor + s.ceiling) * 256 / 2)),
                                  float((j % r.zSectors) * 1024 + 512 + r.info.z));

                    AmbientCache::Cube &cube = ambientCache->items[ambientCache->offsets[i] + j];
                    if (cube.status == AmbientCache::Cube::READY) {
                        glColor3fv((GLfloat*)&cube.colors[0]);
                        glVertex3f(p.x + 0, p.y + 0, p.z + 0);
                        glVertex3f(p.x + S, p.y + 0, p.z + 0);

                        glColor3fv((GLfloat*)&cube.colors[1]);
                        glVertex3f(p.x + 0, p.y + 0, p.z + 0);
                        glVertex3f(p.x - S, p.y + 0, p.z + 0);

                        glColor3fv((GLfloat*)&cube.colors[2]);
                        glVertex3f(p.x + 0, p.y + 0, p.z + 0);
                        glVertex3f(p.x + 0, p.y + S, p.z + 0);

                        glColor3fv((GLfloat*)&cube.colors[3]);
                        glVertex3f(p.x + 0, p.y + 0, p.z + 0);
                        glVertex3f(p.x + 0, p.y - S, p.z + 0);

                        glColor3fv((GLfloat*)&cube.colors[4]);
                        glVertex3f(p.x + 0, p.y + 0, p.z + 0);
                        glVertex3f(p.x + 0, p.y + 0, p.z + S);

                        glColor3fv((GLfloat*)&cube.colors[5]);
                        glVertex3f(p.x + 0, p.y + 0, p.z + 0);
                        glVertex3f(p.x + 0, p.y + 0, p.z - S);
                    }
                }
            }
            glEnd();
            glLineWidth(1);           
        */

            Debug::Level::info(this, player, player->animation);


        Debug::end();
    }
    #endif

    void setViewport(int view, int eye, bool isUI) {
        int vX = Core::x;
        int vY = Core::y;
        int vW = Core::width;
        int vH = Core::height;

        float aspect = float(vW) / float(vH);

        if (Core::defaultTarget) {
            vX = 0;
            vY = 0;
            vW = Core::defaultTarget->width;
            vH = Core::defaultTarget->height;
        }

        Viewport &vp = Core::viewportDef;

        if (players[1] != NULL) {
            vp = Viewport(vX + vW / 2 * view, vY, vW / 2, vH);
            if (Core::settings.detail.stereo != Core::Settings::STEREO_SPLIT)
                aspect *= 0.5f;
        } else
            vp = Viewport(vX, vY, vW, vH); 
        
        if (Core::settings.detail.stereo != Core::Settings::STEREO_VR) {
            switch (eye) {
                case -1 : vp = Viewport(vX + vp.x - vp.x / 2, vY + vp.y, vp.width / 2, vp.height);   break;
                case +1 : vp = Viewport(vX + vW / 2 + vp.x / 2, vY + vp.y, vp.width / 2, vp.height); break;
            }
        }

        Core::eye = float(eye);
        Core::setViewport(vp.x, vp.y, vp.width, vp.height);

        if (isUI)
            UI::updateAspect(aspect);
        else
            camera->aspect = aspect;
    }

    void renderPrepare() {
        if (inventory->video) {
            inventory->render(1.0);
            return;
        }

        needRenderGame = !inventory->video && !level.isTitle() && ((inventory->phaseRing < 1.0f && inventory->titleTimer <= 1.0f) || needRedrawTitleBG);

        if (!needRenderGame)
            return;

        if (needRedrawReflections) {
            initReflections();
            needRedrawReflections = false;
        }

        if (ambientCache)
            ambientCache->processQueue();

        if (shadow && player)
            renderShadows(player->getRoomIndex());
    }

    void renderGame(bool showUI) {
        //if (Core::settings.detail.stereo || Core::settings.detail.splitscreen) {
        //    Core::setTarget(NULL, CLEAR_ALL);
        //    Core::validateRenderState();
        //}

/*  // catsuit test
        lara->bakeEnvironment();
        lara->visibleMask = Lara::BODY_HEAD | Lara::BODY_ARM_L3 | Lara::BODY_ARM_R3;
*/

/*
    // EQUIRECTANGULAR PROJECTION test
        if (!cube360)
            cube360 = new Texture(1024, 1024, 1, Texture::RGBA, true, NULL, true, false);
        renderEnvironment(camera->getRoomIndex(), camera->pos, &cube360, 0, Core::passCompose);
        Core::setTarget(NULL, Core::CLEAR_ALL);
        setShader(Core::passFilter, Shader::FILTER_EQUIRECTANGULAR);
        cube360->bind(sEnvironment);
        mesh->renderQuad();
        return;
*/
        Viewport vp = Core::viewportDef;

        int viewsCount = players[1] ? 2 : 1;
        for (int view = 0; view < viewsCount; view++) {
            player = players[view];
            camera = player->camera;

            setClipParams(1.0f, NO_CLIP_PLANE);
            params->waterHeight = params->clipHeight;

            if (shadow) {
                if (view > 0/* && Core::settings.detail.shadows < Core::Settings::HIGH*/)
                    renderShadows(player->getRoomIndex()); // render shadows for player2 for all-in-one shadow technique
                shadow->bind(sShadow);
            }

            Core::pass = Core::passCompose;

            if (view == 0 && Input::hmd.ready) {
                Core::settings.detail.stereo = Core::Settings::STEREO_VR;

                GAPI::Texture *oldTarget = Core::defaultTarget;
                Viewport vp = Core::viewportDef;

                Core::defaultTarget = Core::eyeTex[0];
                Core::viewportDef = Viewport(0, 0, Core::defaultTarget->width, Core::defaultTarget->height);
                Core::setTarget(NULL,Core::defaultTarget, 0); // changing to 0 and adding defaultTarget parameter
                Core::eye = -1.0f;
                setup();
                renderView(camera->getRoomIndex(), true);

                Core::defaultTarget = Core::eyeTex[1];
                Core::viewportDef = Viewport(0, 0, Core::defaultTarget->width, Core::defaultTarget->height);
                Core::setTarget(NULL, Core::defaultTarget, 0);
                Core::eye =  1.0f;
                setup();
                renderView(camera->getRoomIndex(), true);

                //Core::settings.detail.vr = false;

                Core::defaultTarget = oldTarget;
                Core::setTarget(NULL, Core::defaultTarget, 0);
                Core::viewportDef = vp;
            }

            if (Core::settings.detail.stereo == Core::Settings::STEREO_ON) { // left/right SBS stereo
                float oldEye = Core::eye;

                setViewport(view, -1, false);
                setup();
                renderView(camera->getRoomIndex(), true);

                setViewport(view,  1, false);
                setup();
                renderView(camera->getRoomIndex(), true);

                Core::eye = oldEye;
            } else {
                setViewport(view, int(Core::eye), false);
                setup();
                renderView(camera->getRoomIndex(), true);
            }
        }

        if (showUI) {
            Core::Pass pass = Core::pass;

            for (int view = 0; view < viewsCount; view++) {
                player = players[view];
                camera = player->camera;

                setClipParams(1.0f, NO_CLIP_PLANE);
                params->waterHeight = params->clipHeight;

                if (Core::settings.detail.stereo == Core::Settings::STEREO_ON) { // left/right SBS stereo
                    float oldEye = Core::eye;

                    setViewport(view, -1, false);
                    renderUI();

                    setViewport(view, 1, false);
                    renderUI();

                    Core::eye = oldEye;
                } else {
                    setViewport(view, int(Core::eye), false);
                    renderUI();
                }
            }

            Core::pass = pass;
        }

        Core::viewportDef = vp;

        player = players[0];
        camera = player->camera;

        // lara->visibleMask = 0xFFFFFFFF; // catsuit test
    }

    void renderUI() {
        if (level.isCutsceneLevel() || inventory->titleTimer > 1.0f || level.isTitle()) return;

        UI::begin();
        UI::updateAspect(camera->aspect);

        UI::renderPickups();

        Core::resetLights();

    // render health & oxygen bars
        vec2 size = vec2(180, 10);

        float health = player->health / float(LARA_MAX_HEALTH);
        float oxygen = player->oxygen / float(LARA_MAX_OXYGEN);

        if ((params->time - int(params->time)) < 0.5f) { // blinking
            if (health <= 0.2f) health = 0.0f;
            if (oxygen <= 0.2f) oxygen = 0.0f;
        }

        float eye = inventory->active ? 0.0f : UI::width * Core::eye * 0.02f;

        vec2 pos;
        if (Core::settings.detail.stereo == Core::Settings::STEREO_VR)
            pos = vec2((UI::width - size.x) * 0.5f - eye * 4.0f, 96);
        else
            pos = vec2(UI::width - 32 - size.x - eye, 32);

        if (!player->dozy && (player->stand == Lara::STAND_ONWATER || player->stand == Character::STAND_UNDERWATER)) {
            UI::renderBar(UI::BAR_OXYGEN, pos, size, oxygen);
            pos.y += 16.0f;
        }

        if ((!inventory->active && ((player->wpnReady() && !player->emptyHands()) || player->damageTime > 0.0f || health <= 0.2f))) {
            UI::renderBar(UI::BAR_HEALTH, pos, size, health);
            pos.y += 32.0f;

            if (!inventory->active && !player->emptyHands()) { // ammo
                int index = inventory->contains(player->wpnCurrent);
                if (index > -1)
                    inventory->renderItemCount(inventory->items[index], pos, size.x);
            }
        }

        UI::renderHelp();

        UI::end();
    }

    void renderInventoryEye(int eye) {
        float aspect = float(Core::width) / float(Core::height);

        if (Core::settings.detail.stereo != Core::Settings::STEREO_VR)
            switch (eye) {
                case -1 : Core::setViewport(Core::x, Core::y, Core::width / 2, Core::height); break;
                case  0 : Core::setViewport(Core::x, Core::y, Core::width, Core::height); break;
                case +1 : Core::setViewport(Core::x + Core::width / 2, Core::y, Core::width / 2, Core::height); break;
            }

        if (Core::settings.detail.stereo == Core::Settings::STEREO_SPLIT)
            eye = 0;

        Core::eye = float(eye);

        if (level.isTitle() || inventory->titleTimer > 0.0f)
            inventory->renderBackground();
        inventory->render(aspect);

        UI::begin();
        UI::updateAspect(aspect);
        inventory->renderUI();
        UI::end();
    }

    void renderInventory() {
        Core::setTarget(NULL, NULL, RT_CLEAR_DEPTH | RT_STORE_COLOR);

        Core::resetLights();

        if (!(level.isTitle() || inventory->titleTimer > 0.0f))
            inventory->renderBackground();

        float oldEye = Core::eye;

        if ((Core::settings.detail.stereo == Core::Settings::STEREO_ON) || (Core::settings.detail.stereo == Core::Settings::STEREO_SPLIT && players[1])) {
            renderInventoryEye(-1);
            renderInventoryEye(+1);
        } else
            renderInventoryEye(int(Core::eye));

        Core::setViewport(Core::x, Core::y, Core::width, Core::height);
        Core::eye = oldEye;
    }

    void render() {
        if (inventory->video)
            return;

        bool title  = inventory->isActive() || level.isTitle();
        bool copyBg = title && (lastTitle != title || needRedrawTitleBG);
        lastTitle = title;
        needRedrawTitleBG = false;

        if (isEnded) {
            Core::setTarget(NULL, NULL, RT_CLEAR_COLOR | RT_STORE_COLOR);
            UI::begin();
            UI::updateAspect(float(Core::width) / float(Core::height));
            UI::textOut(vec2(0, 480 - 16), STR_LOADING, UI::aCenter, UI::width);
            UI::end();
            return;
        }

        if (copyBg) {
            inventory->prepareBackground();
        }

        if (needRenderGame)
            renderGame(true);

        renderInventory();
    }

};

#endif
