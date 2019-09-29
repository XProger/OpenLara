#include <stdio.h>
#include <sys/time.h>
#include <SDL/SDL.h>

#include "game.h"

SDL_Surface *screen = NULL;

#define SCREEN_WIDTH    (SCREEN_HEIGHT/3)*4
#define SCREEN_HEIGHT   240
#define BTN_B           SDLK_SPACE
#define BTN_A           SDLK_LCTRL
#define BTN_TA          SDLK_LALT
#define BTN_TB          SDLK_LSHIFT
#define BTN_START       SDLK_RETURN
#define BTN_SELECT      SDLK_ESCAPE
#define BTN_R           SDLK_RCTRL
#define BTN_UP          SDLK_UP
#define BTN_DOWN        SDLK_DOWN
#define BTN_LEFT        SDLK_LEFT
#define BTN_RIGHT       SDLK_RIGHT

// timing
unsigned int startTime;

int osGetTimeMS() {
    timeval t;
    gettimeofday(&t, NULL);
    return int((t.tv_sec - startTime) * 1000 + t.tv_usec / 1000);
}

// input
bool osJoyReady(int index) {
    return index == 0;
}

void osJoyVibrate(int index, float L, float R) {
    //
}

JoyKey getJoyKey(int key) {
    switch (key) {
        case BTN_B      : return jkX;
        case BTN_A      : return jkA;
        case BTN_TA     : return jkRB;
        case BTN_TB     : return jkY;
        case BTN_START  : return jkStart;
        case BTN_SELECT : return jkSelect;
        case BTN_UP     : return jkUp;
        case BTN_DOWN   : return jkDown;
        case BTN_LEFT   : return jkLeft;
        case BTN_RIGHT  : return jkRight;
        default         : return jkNone;
    }
}

int main() {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE);
    SDL_ShowCursor(0);
    screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, 16, SDL_SWSURFACE | SDL_NOFRAME);

    timeval t;
    gettimeofday(&t, NULL);
    startTime = t.tv_sec;

    Core::width  = SCREEN_WIDTH;
    Core::height = SCREEN_HEIGHT;

    Core::defLang = 0;

    Game::init("/mnt/games/OpenLara/LEVEL2.PSX");

    GAPI::resize();
    GAPI::swColor = (uint16*)screen->pixels;

    bool isQuit = false;

    while (!isQuit) {

        SDL_Event event;
        if (SDL_PollEvent(&event)) {

            if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
                Input::setJoyDown(0, getJoyKey(event.key.keysym.sym), event.type == SDL_KEYDOWN);
            }

            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == BTN_R) {
                isQuit = true;
            }
        } else {
            if (Game::update()) {
                Game::render();
                SDL_Flip(screen);
            }
        }

    }

    Game::deinit();

    return 0;
}
