#ifndef H_GAME
#define H_GAME

#include "core.h"
#include "format.h"
#include "cache.h"
#include "level.h"
#include "ui.h"
#include "savegame.h"

ShaderCache *shaderCache;

namespace Game {
    Level  *level;
    Stream *nextLevel;

    void startLevel(Stream *lvl) {
        TR::LevelID id = TR::LVL_MAX;
        if (level)
            id = level->level.id;

        Input::stopJoyVibration();

        bool playVideo = true;
        if (loadSlot != -1)
            playVideo = (saveSlots[loadSlot].level & LVL_FLAG_CHECKPOINT) == 0;

        delete level;
        level = new Level(*lvl);

        bool playLogo = level->level.isTitle() && id == TR::LVL_MAX;
        playVideo = playVideo && (id != level->level.id);

        if (level->level.isTitle() && id != TR::LVL_MAX)
            playVideo = false;

        level->init(playLogo, playVideo);

        UI::game = level;
        #if !defined(_OS_PSP) && !defined(_OS_CLOVER)
            UI::helpTipTime = 5.0f;
        #endif
        delete lvl;
    }
}

void loadLevelAsync(Stream *stream, void *userData) {
    if (!stream) {
        if (Game::level) Game::level->isEnded = false;
        return;
    }
    Game::nextLevel = stream;
}

void loadSettings(Stream *stream, void *userData) {
    if (stream) {
        uint8 version;
        stream->read(version);
        if (version == SETTINGS_VERSION && stream->size == sizeof(Core::Settings))
            stream->raw((char*)&Core::settings + 1, stream->size - 1); // read settings data right after version number
        delete stream;
    }
    
    #ifdef _OS_ANDROID
        if (Core::settings.detail.stereo == Core::Settings::STEREO_VR)
            osToggleVR(true);
    #endif

    Core::settings.version = SETTINGS_VERSION;
    Core::setVSync(Core::settings.detail.vsync != 0);

    shaderCache = new ShaderCache();
    Game::startLevel((Stream*)userData);
    UI::init(Game::level);
}

static void readSlotAsync(Stream *stream, void *userData) {
    if (!stream) {
        saveResult = SAVE_RESULT_ERROR;
        return;
    }

    readSaveSlots(stream);
    delete stream;

    saveResult = SAVE_RESULT_SUCCESS;
}

void readSlots() {
    ASSERT(saveResult != SAVE_RESULT_WAIT);

    if (saveResult == SAVE_RESULT_WAIT)
        return;

    LOG("Read Slots...\n");
    saveResult = SAVE_RESULT_WAIT;

    osReadSlot(new Stream(SAVE_FILENAME, NULL, 0, readSlotAsync, NULL));
}

namespace Game {

    void stopChannel(Sound::Sample *channel) {
        if (level) level->stopChannel(channel);
    }

    void init(Stream *lvl) {
        loadSlot    = -1;
        nextLevel   = NULL;
        shaderCache = NULL;
        level       = NULL;

        Core::init();
        Sound::callback = stopChannel;

        Core::settings.version = SETTINGS_READING;
        Stream::cacheRead("settings", loadSettings, lvl);
        readSlots();
    }

    void init(const char *lvlName = NULL) {
        #ifdef DEBUG_RENDER
            Debug::init();
        #endif
        char fileName[255];

        TR::Version version = TR::getGameVersion();
        if (!lvlName)
            TR::getGameLevelFile(fileName, version, TR::getTitleId(version));
        else
            strcpy(fileName, lvlName);

        init(new Stream(fileName));
    }

    void deinit() {
        freeSaveSlots();

        #ifdef DEBUG_RENDER
            Debug::deinit();
        #endif
        delete level;
        UI::deinit();
        delete shaderCache;
        Core::deinit();
    }

    void updateTick() {
        Input::update();
        
        if (!level->level.isTitle()) {
            if (Input::lastState[0] == cStart) level->addPlayer(0);
            if (Input::lastState[1] == cStart) level->addPlayer(1);
        }

        float dt = Core::deltaTime;
        if (Input::down[ikR]) // slow motion (for animation debugging)
            Core::deltaTime /= 10.0f;

        if (Input::down[ikT]) // fast motion
            for (int i = 0; i < 9; i++)
                level->update();

        level->update();

        Core::deltaTime = dt;
    }

    bool update() {
    // async load for settings
        if (Core::settings.version == SETTINGS_READING)
            return true;

        PROFILE_MARKER("UPDATE");

        if (!Core::update())
            return false;

        float delta = Core::deltaTime;

        if (nextLevel) {
            startLevel(nextLevel);
            nextLevel = NULL;
        }

        if (level->isEnded)
            return true;

        if (Input::down[ik0] && !level->inventory->isActive()) {
            level->inventory->toggle(0, Inventory::PAGE_LEVEL_STATS);
            Input::down[ik0] = false;
        }

        if (Input::down[ik5] && !level->inventory->isActive()) {
            if (level->players[0]->canSaveGame())
                level->saveGame(true, false);
            Input::down[ik5] = false;
        }

        if (Input::down[ik9] && !level->inventory->isActive()) {
            int slot = getSaveSlot(level->level.id, true);
            if (slot == -1)
                slot = getSaveSlot(level->level.id, false);
            if (slot > -1)
                level->loadGame(slot);
            Input::down[ik9] = false;
        }

        if (!level->level.isCutsceneLevel())
            delta = min(0.2f, delta);

        while (delta > EPS) {
            Core::deltaTime = min(delta, 1.0f / 30.0f);
            Game::updateTick();
            delta -= Core::deltaTime;
            if (Core::resetState) // resetTime was called
                break;
        }

        return true;
    }

    bool frameBegin() {
        if (Core::settings.version == SETTINGS_READING) return false;
        Core::reset();
        if (Core::beginFrame()) {
            level->renderPrepare();
            return true;
        }
        return false;
    }

    void frameRender() {
        if (Core::settings.version == SETTINGS_READING) return;

        PROFILE_MARKER("RENDER");
        PROFILE_TIMING(Core::stats.tFrame);

        level->render();
        #ifdef DEBUG_RENDER
            level->renderDebug();
        #endif    
    }

    void frameEnd() {
        if (Core::settings.version == SETTINGS_READING) return;

        UI::renderTouch();
        Core::endFrame();
    }

    bool render() {
        if (frameBegin()) {
            frameRender();
            frameEnd();
            return true;
        }
        return false;
    }
}

#endif
