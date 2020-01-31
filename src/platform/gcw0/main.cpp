#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <linux/input.h>
#include <linux/fb.h>
#include <linux/vt.h>
#include <unistd.h>
#include <pwd.h>
#include <pthread.h>
#include <EGL/egl.h>
#include <fcntl.h>
#include <linux/input.h>
#include <alsa/asoundlib.h>

#include "game.h"

#define WND_TITLE    "OpenLara"

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
    
    // In the perfect world ReedPlayer-Clover process 
    // will release ALSA device before app running, but...
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

// Window
struct FrameBuffer {
    unsigned short width;
    unsigned short height;
} fb;

EGLDisplay display;
EGLSurface surface;
EGLContext context;

bool eglInit() {
    LOG("EGL init context...\n");

    fb_var_screeninfo vinfo;
    int fd = open("/dev/fb0", O_RDWR);
    if (ioctl(fd, FBIOGET_VSCREENINFO, &vinfo) < 0) {
        LOG("! can't get framebuffer size\n");
        return false;
    }
    close(fd);

    fb.width  = vinfo.xres;
    fb.height = vinfo.yres;

    const EGLint eglAttr[] = {
        EGL_RENDERABLE_TYPE,        EGL_OPENGL_ES2_BIT,
        EGL_SURFACE_TYPE,           EGL_WINDOW_BIT,
        EGL_BLUE_SIZE,              5,
        EGL_GREEN_SIZE,             6,
        EGL_RED_SIZE,               5,
        EGL_DEPTH_SIZE,             16,
        EGL_NONE
    };

    const EGLint ctxAttr[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display == EGL_NO_DISPLAY) {
        LOG("eglGetDisplay = EGL_NO_DISPLAY\n");
        return false;
    }

    if (eglInitialize(display, NULL, NULL) == EGL_FALSE) {
        LOG("eglInitialize = EGL_FALSE\n");
        return false;
    }

    EGLConfig config;
    EGLint configCount;

    if (eglChooseConfig(display, eglAttr, &config, 1, &configCount) == EGL_FALSE || configCount == 0) {
        LOG("eglChooseConfig = EGL_FALSE\n");
        return false;
    }

    surface = eglCreateWindowSurface(display, config, 0, NULL);
    if (surface == EGL_NO_SURFACE) {
        LOG("eglCreateWindowSurface = EGL_NO_SURFACE\n");
        return false;
    }

    context = eglCreateContext(display, config, EGL_NO_CONTEXT, ctxAttr);
    if (context == EGL_NO_CONTEXT) {
        LOG("eglCreateContext = EGL_NO_CONTEXT\n");
        return false;
    }

    if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE) {
        LOG("eglMakeCurrent = EGL_FALSE\n");
        return false;
    }

    return true;
}

void eglFree() {
    LOG("EGL release context\n");
    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroySurface(display, surface);
    eglDestroyContext(display, context);
    eglTerminate(display);
}

// Input
int ev_buttons;
int ev_haptic;
ff_effect joy_ff;

#define JOY_RUMBLE_TIMER  50
#define JOY_RUMBLE_GAIN   0xFFFF

vec2 joyL, joyR;
float joyVGain, joyVGainOld;
int   joyVTime;

bool osJoyReady(int index) {
    return index == 0;
}

void osJoyVibrate(int index, float L, float R) {
    if (ev_haptic == -1) return;
    joyVGain = (L + R) * 0.5f;
}

float joyAxisValue(int value) {
    return value / 1536.0f - 1.0f;
}

vec2 joyDir(const vec2 &value) {
    float dist = min(1.0f, value.length());
    return value.normal() * dist;
}

void joyRumble() {
    if (joy_ff.id == -1 || joyVGain == joyVGainOld || osGetTimeMS() < joyVTime)
        return;

    joy_ff.u.rumble.strong_magnitude = int(JOY_RUMBLE_GAIN * joyVGain);
    ioctl(ev_haptic, EVIOCSFF, &joy_ff);

    joyVGainOld = joyVGain;
    joyVTime    = osGetTimeMS() + JOY_RUMBLE_TIMER;
}

JoyKey codeToJoyKey(int code) {
    switch (code) {
    // gamepad
        case KEY_LEFT       : return jkLeft;
        case KEY_RIGHT      : return jkRight;
        case KEY_UP         : return jkUp;
        case KEY_DOWN       : return jkDown;
        case KEY_LEFTCTRL   : return jkB;
        case KEY_LEFTALT    : return jkA;
        case KEY_SPACE      : return jkY;
        case KEY_LEFTSHIFT  : return jkX;
        case KEY_TAB        : return jkLB;
        case KEY_BACKSPACE  : return jkRB;
        case KEY_ESC        : return jkSelect;
        case KEY_ENTER      : return jkStart;
        case KEY_KPSLASH    : return jkL;
        case KEY_KPDOT      : return jkR;
        case KEY_PAGEUP     : return jkLT;
        case KEY_PAGEDOWN   : return jkRT;
        case KEY_POWER      : {
            Game::quickSave();
            Core::quit();
        }
    }
    return jkNone;
}

void inputInit() {
    joyL = joyR = vec2(0.0f);
    joyVGain    = 0.0f;
    joyVGainOld = 0.0f;
    joyVTime    = osGetTimeMS();

    memset(&joy_ff, 0, sizeof(joy_ff));
    joy_ff.id = -1;

    // TODO find compatible device instead of hardcode
    ev_buttons = open("/dev/input/event3", O_NONBLOCK | O_RDONLY);
    ev_haptic  = open("/dev/input/event1", O_RDWR);

    if (ev_buttons == -1) {
        LOG("! input device was not found\n");
    }

    if (ev_haptic == -1) {
        LOG("! haptic device was not found\n");
    } else {
        joy_ff.type                      = FF_RUMBLE;
        joy_ff.id                        = -1;
        joy_ff.replay.length             = 0;
        joy_ff.replay.delay              = 0;
        joy_ff.u.rumble.strong_magnitude = 0;
        joy_ff.u.rumble.weak_magnitude   = 0;

        if (ioctl(ev_haptic, EVIOCSFF, &joy_ff) != -1) {
            input_event gain;
            gain.type  = EV_FF;
            gain.code  = FF_GAIN;
            gain.value = JOY_RUMBLE_GAIN;
            write(ev_haptic, &gain, sizeof(gain));

            input_event state;
            state.type  = EV_FF;
            state.code  = joy_ff.id;
            state.value = 1; // play
            write(ev_haptic, &state, sizeof(state));
        } else {
            LOG("! can't initialize vibration\n");
            close(ev_haptic);
            ev_haptic = -1;
        }
    }
}

void inputFree() {
    if (ev_buttons != -1) close(ev_buttons);
    if (ev_haptic  != -1) close(ev_haptic);
}

void inputUpdate() {
    joyRumble();

    if (ev_buttons == -1) return;

    input_event events[64];

    int rb = read(ev_buttons, events, sizeof(events));

    input_event *e = events;
    while (rb > 0) {
        switch (e->type) {
            case EV_KEY : {
                JoyKey key = codeToJoyKey(e->code);
                Input::setJoyDown(0, key, e->value != 0);
                break;
            }
            case EV_ABS : {
                switch (e->code) {
                // Left stick
                    case ABS_X  : joyL.x = -joyAxisValue(e->value); break;
                    case ABS_Y  : joyL.y = -joyAxisValue(e->value); break;
                // Right stick
                    case ABS_RX : joyR.x = joyAxisValue(e->value); break;
                    case ABS_RY : joyR.y = joyAxisValue(e->value); break;
                }

                Input::setJoyPos(0, jkL, joyDir(joyL));
                Input::setJoyPos(0, jkR, joyDir(joyR));
            }
        }
        e++;
        rb -= sizeof(events[0]);
    }
}

int main(int argc, char **argv) {
    if (!eglInit()) {
        LOG("! can't initialize EGL context\n");
        return -1;
    }
    
    Core::width  = fb.width;
    Core::height = fb.height;

    cacheDir[0] = saveDir[0] = contentDir[0] = 0;

    const char *home;
    if (!(home = getenv("HOME")))
        home = getpwuid(getuid())->pw_dir;
    strcpy(contentDir, home);
    strcat(contentDir, "/.openlara/");

    struct stat st = {0};

    if (stat(contentDir, &st) == -1) {
        LOG("no data directory found, please copy the original game content into %s\n", contentDir);
        return -1;
    }

    strcpy(saveDir, contentDir);

    strcpy(cacheDir, contentDir);
    strcat(cacheDir, "cache/");

    if (stat(cacheDir, &st) == -1 && mkdir(cacheDir, 0777) == -1) {
        cacheDir[0] = 0;
        LOG("can't create /home/.openlara/cache/\n");
    }

    timeval t;
    gettimeofday(&t, NULL);
    startTime = t.tv_sec;

    Game::init();

    Game::quickLoad(true);

    inputInit();
    sndInit();

    while (!Core::isQuit) {
        inputUpdate();

        if (Game::update()) {
            Game::render();
            Core::waitVBlank();
            eglSwapBuffers(display, surface);
        } else
            usleep(9000);
    };

    inputFree();

    Game::deinit();
    eglFree();

    sndFree();

    return 0;
}
