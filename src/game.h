#ifndef H_GAME_TR
#define H_GAME_TR

//#define FREE_CAMERA

#include "core.h"
#include "format.h"
#include "level.h"

namespace Game {
    Level *level;

    void init() {
        Core::init();
        Stream stream("LEVEL2_DEMO.PHD");
        level = new Level(stream, true);

        #ifndef __EMSCRIPTEN__    
            //Sound::play(Sound::openWAD("05_Lara's_Themes.wav"), 1, 1, 0);
            Sound::play(new Stream("05.ogg"), vec3(0.0f), 1, 1, Sound::Flags::LOOP);
            //Sound::play(new Stream("03.mp3"), 1, 1, 0);
        #endif
    }

    void free() {
        delete level;

        Core::free();
    }

    void update() {
        level->update();
    }

    void render() {
        Core::clear(vec4(0.0f));
        Core::setViewport(0, 0, Core::width, Core::height);
        Core::setBlending(bmAlpha);
        level->render();
    }
}

#endif