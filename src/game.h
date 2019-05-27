#ifndef H_GAME
#define H_GAME

#include "core.h"
#include "format.h"
#include "cache.h"
#include "inventory.h"
#include "level.h"
#include "ui.h"
#include "savegame.h"

#define MAX_CHEAT_SEQUENCE 8

namespace Game {
    Level      *level;
    Stream     *nextLevel;
    ControlKey cheatSeq[MAX_CHEAT_SEQUENCE];

    void cheatControl(ControlKey key) {
        if (key == cMAX || !level || level->level.isTitle() || level->level.isCutsceneLevel()) return;
        const ControlKey CHEAT_ALL_WEAPONS[] = { cLook, cWeapon, cDash, cDuck, cDuck, cDash, cRoll, cLook };
        const ControlKey CHEAT_SKIP_LEVEL[]  = { cDuck, cDash, cLook, cRoll, cWeapon, cLook, cDash, cDuck };

        for (int i = 0; i < MAX_CHEAT_SEQUENCE - 1; i++)
            cheatSeq[i] = cheatSeq[i + 1];
        cheatSeq[MAX_CHEAT_SEQUENCE - 1] = key;

    // add all weapons
        if (!memcmp(&cheatSeq[MAX_CHEAT_SEQUENCE - COUNT(CHEAT_ALL_WEAPONS)], CHEAT_ALL_WEAPONS, sizeof(CHEAT_ALL_WEAPONS))) {
            inventory->addWeapons();
            level->playSound(TR::SND_SCREAM);
        }
    // skip level
        if (!memcmp(&cheatSeq[MAX_CHEAT_SEQUENCE - COUNT(CHEAT_SKIP_LEVEL)], CHEAT_SKIP_LEVEL, sizeof(CHEAT_SKIP_LEVEL)))
            level->loadNextLevel();
    }

    void startLevel(Stream *lvl) {
        TR::LevelID id = TR::LVL_MAX;
        if (level)
            id = level->level.id;

        Input::stopJoyVibration();

        bool playVideo = true;
        if (loadSlot != -1)
            playVideo = !saveSlots[loadSlot].isCheckpoint();

        delete level;
        level = new Level(*lvl);

        bool playLogo = level->level.isTitle() && id == TR::LVL_MAX;
        playVideo = playVideo && (id != level->level.id);

        if (level->level.isTitle() && id != TR::LVL_MAX && !TR::isGameEnded)
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

        memset(cheatSeq, 0, sizeof(cheatSeq));

        Core::init();
        Sound::callback = stopChannel;

        if (lvl->size == -1) {
            delete lvl;
            Core::quit();
            return;
        }

        Core::settings.version = SETTINGS_READING;
        Stream::cacheRead("settings", loadSettings, lvl);
        readSlots();
    }

    void init(const char *lvlName = NULL) {
        #ifdef DEBUG_RENDER
            Debug::init();
        #endif
        char fileName[255];

        TR::isGameEnded = false;

        TR::Version version = TR::getGameVersion();
        if (!lvlName)
            TR::getGameLevelFile(fileName, version, TR::getTitleId(version));
        else
            strcpy(fileName, lvlName);

        inventory = new Inventory();

        init(new Stream(fileName));
    }

    void deinit() {
        freeSaveSlots();

        #ifdef DEBUG_RENDER
            Debug::deinit();
        #endif
        delete inventory;
        delete level;
        UI::deinit();
        delete shaderCache;
        Core::deinit();
    }

    void updateTick() {
        Input::update();
        Network::update();

        cheatControl(Input::lastState[0]); 

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

    #ifdef _DEBUG
        if (Input::down[ikF]) {
            level->flipMap();
            Input::down[ikF] = false;
        }
    #endif

    #ifdef _DEBUG_SHADERS
        if (Input::down[ikCtrl] && Input::down[ik1]) {
            delete shaderCache;
            shaderCache = new ShaderCache();
            Input::down[ik1] = false;
        }
    #endif

        if (Input::down[ik5] && !inventory->isActive()) {
            if (level->players[0]->canSaveGame())
                level->saveGame(level->level.id, true, false);
            Input::down[ik5] = false;
        }

        if (Input::down[ik9] && !inventory->isActive()) {
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
