#include <stdio.h>
#include <sys/time.h>
#include <alsa/asoundlib.h>
#include <SDL/SDL.h>

#include "game.h"

SDL_Surface *screen = NULL;

#define SCREEN_WIDTH    (SCREEN_HEIGHT/3)*4
#define SCREEN_HEIGHT   240
#ifdef __MIYOO__
#define BTN_A           SDLK_LALT
#define BTN_B           SDLK_LCTRL
#define BTN_X           SDLK_LSHIFT
#define BTN_Y           SDLK_SPACE
#define BTN_L1          SDLK_BACKSPACE
#define BTN_R1          SDLK_TAB
#define BTN_L2          SDLK_RSHIFT
#define BTN_R2          SDLK_RALT
#else
#define BTN_B           SDLK_SPACE
#define BTN_A           SDLK_LCTRL
#define BTN_TA          SDLK_LALT
#define BTN_TB          SDLK_LSHIFT
#endif
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

// sound
snd_pcm_uframes_t   SND_FRAMES = 512;
snd_pcm_t           *sndOut;
Sound::Frame        *sndData;
pthread_t           sndThread;

void* sndFill(void *arg) {
    while (sndOut) {
        Sound::fill(sndData, SND_FRAMES);

        int count = SND_FRAMES;
        while (count > 0) {
            int frames = snd_pcm_writei(sndOut, &sndData[SND_FRAMES - count], count);
            if (frames < 0) {
                frames = snd_pcm_recover(sndOut, frames, 0);
                if (frames == -EAGAIN) {
                    LOG("snd_pcm_writei try again\n");
                    sleep(1);
                    continue;
                }
                if (frames < 0) {
                    LOG("snd_pcm_writei failed: %s\n", snd_strerror(frames));
                    sndOut = NULL;
                    return NULL;
                }
            }
            count -= frames;
        }

        snd_pcm_prepare(sndOut);
    }
    return NULL;
}

bool sndInit() {
    unsigned int freq = 44100;

    int err;

    for (int i = 0; i < 20; i++) { // 20 * 0.1 = 2 secs
        sndOut = NULL;
        if ((err = snd_pcm_open(&sndOut, "default", SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
            LOG("sound: try to snd_pcm_open #%d...\n", i);
            usleep(100000); // wait for 100 ms
            continue;
        }
        break;
    }

    // I've bad news for you
    if (!sndOut) {
        LOG("! sound: snd_pcm_open %s\n", snd_strerror(err));
        return false;
    }

    snd_pcm_hw_params_t *params;

    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(sndOut, params);
    snd_pcm_hw_params_set_access(sndOut, params, SND_PCM_ACCESS_RW_INTERLEAVED);

    snd_pcm_hw_params_set_channels(sndOut, params, 2);
    snd_pcm_hw_params_set_format(sndOut, params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_rate_near(sndOut, params, &freq, NULL);

    snd_pcm_hw_params_set_periods(sndOut, params, 4, 0);
    snd_pcm_hw_params_set_period_size_near(sndOut, params, &SND_FRAMES, NULL);
    snd_pcm_hw_params_get_period_size(params, &SND_FRAMES, 0);

    snd_pcm_hw_params(sndOut, params);
    snd_pcm_prepare(sndOut);

    sndData = new Sound::Frame[SND_FRAMES];
    memset(sndData, 0, SND_FRAMES * sizeof(Sound::Frame));
    if ((err = snd_pcm_writei(sndOut, sndData, SND_FRAMES)) < 0) {
        LOG("! sound: write %s\n", snd_strerror(err));
        sndOut = NULL;
    }

    snd_pcm_start(sndOut);
    pthread_create(&sndThread, NULL, sndFill, NULL);

    return true;
}

void sndFree() {
    pthread_cancel(sndThread);
    snd_pcm_drop(sndOut);
    snd_pcm_drain(sndOut);
    snd_pcm_close(sndOut);
    delete[] sndData;
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
#ifdef __MIYOO__
        case BTN_A      : return jkA;
        case BTN_B      : return jkB;
        case BTN_X      : return jkX;
        case BTN_Y      : return jkY;
        case BTN_L1     : return jkLB;
        case BTN_R1     : return jkRB;
        case BTN_L2     : return jkLT;
        case BTN_R2     : return jkRT;
#else
        case BTN_B      : return jkX;
        case BTN_A      : return jkA;
        case BTN_TA     : return jkRB;
        case BTN_TB     : return jkY;
#endif
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

    Game::init((const char *)NULL);

    GAPI::resize();
    GAPI::swColor = (uint16*)screen->pixels;

    sndInit();

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

    sndFree();

    return 0;
}
