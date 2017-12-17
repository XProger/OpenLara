#ifndef H_GAME
#define H_GAME

#include "core.h"
#include "format.h"
#include "cache.h"
#include "level.h"
#include "ui.h"

ShaderCache *shaderCache;

namespace Game {
    Level  *level;
    Stream *nextLevel;
}

void loadAsync(Stream *stream, void *userData) {
    if (!stream) {
        if (Game::level) Game::level->isEnded = false;
        return;
    }
    Game::nextLevel = stream;
}

namespace Game {
    void startLevel(Stream *lvl) {
        delete level;
        level = new Level(*lvl);
        UI::game = level;
        delete lvl;
    }

    void stopChannel(Sound::Sample *channel) {
        if (level) level->stopChannel(channel);
    }

    void init(Stream *lvl) {
        nextLevel = NULL;

        Core::init();

        shaderCache = new ShaderCache();

        UI::init(level);

        Sound::callback = stopChannel;

        level = NULL;
        startLevel(lvl);
    }

    void init(char *lvlName = NULL, char *sndName = NULL) {
        char fileName[255];

        TR::Version version = TR::getGameVersion();
        if (!lvlName && version != TR::VER_UNKNOWN) {
            lvlName = fileName;
            TR::getGameLevelFile(lvlName, version, TR::getTitleId(version));
        }

        if (!lvlName) {
            lvlName = fileName;
            strcpy(lvlName, "level/1/TITLE.PSX");
        }

        init(new Stream(lvlName));
    }

    void deinit() {
        delete level;
        UI::deinit();
        delete shaderCache;
        Core::deinit();
    }

    void updateTick() {
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

        Input::update();

        if (level->camera) {
            if (Input::down[ikV]) { // third <-> first person view
                level->camera->changeView(!level->camera->firstPerson);
                Input::down[ikV] = false;
            }
        }

        if (Input::down[ikS]) {
            if (level->lara->canSaveGame())
                level->saveGame(0);
            Input::down[ikS] = false;
        }

        if (Input::down[ikL]) {
            level->loadGame(0);
            Input::down[ikL] = false;
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

    void render() {
        PROFILE_MARKER("RENDER");
        PROFILE_TIMING(Core::stats.tFrame);
        Core::beginFrame();
        level->render();
        UI::renderTouch();

        #ifdef _DEBUG
            level->renderDebug();
        #endif

        Core::endFrame();
    }
}

#endif
