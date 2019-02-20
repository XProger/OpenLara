#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <linux/input.h>
#include <unistd.h>
#include <pwd.h>
#include <pthread.h>
//#include <EGL/egl.h>
#include <fcntl.h>
#include <linux/input.h>
#include <libudev.h>
#include <alsa/asoundlib.h>

// SDL2 include stuff
#include <SDL2/SDL.h>
#define GL_GLEXT_PROTOTYPES 1
#include <SDL2/SDL_opengles2.h>
//

#include "game.h"

#define WND_TITLE    "OpenLara"

// timing
unsigned int startTime;

int osGetTime() {
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

        int err = snd_pcm_writei(sndOut, sndData, SND_FRAMES);
        if (err < 0) {
            LOG("! sound: write %s\n", snd_strerror(err));;
            if (err != -EPIPE)
                break;

            err = snd_pcm_recover(sndOut, err, 0);
            if (err < 0) {
                LOG("! sound: failed to recover\n");
                break;
            }
            snd_pcm_prepare(sndOut);
        }
    }
    return NULL;
}

bool sndInit() {
    unsigned int freq = 44100;

    int err;
    if ((err = snd_pcm_open(&sndOut, "default", SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        LOG("! sound: open %s\n", snd_strerror(err));\
        sndOut = NULL;
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
        LOG("! sound: write %s\n", snd_strerror(err));\
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

// Input
#define MAX_INPUT_DEVICES 16
int inputDevices[MAX_INPUT_DEVICES];

udev *udevObj;
udev_monitor *udevMon;
int udevMon_fd;

vec2 joyL, joyR;

bool osJoyReady(int index) {
    return index == 0; // TODO
}

void osJoyVibrate(int index, float L, float R) {
    // TODO
}

InputKey codeToInputKey(int code) {
    switch (code) {
    // keyboard
        case KEY_LEFT       : return ikLeft;
        case KEY_RIGHT      : return ikRight;
        case KEY_UP         : return ikUp;
        case KEY_DOWN       : return ikDown;
        case KEY_SPACE      : return ikSpace;
        case KEY_TAB        : return ikTab;
        case KEY_ENTER      : return ikEnter;
        case KEY_ESC        : return ikEscape;
        case KEY_LEFTSHIFT  :
        case KEY_RIGHTSHIFT : return ikShift;
        case KEY_LEFTCTRL   :
        case KEY_RIGHTCTRL  : return ikCtrl;
        case KEY_LEFTALT    :
        case KEY_RIGHTALT   : return ikAlt;
        case KEY_0          : return ik0;
        case KEY_1          : return ik1;
        case KEY_2          : return ik2;
        case KEY_3          : return ik3;
        case KEY_4          : return ik4;
        case KEY_5          : return ik5;
        case KEY_6          : return ik6;
        case KEY_7          : return ik7;
        case KEY_8          : return ik8;
        case KEY_9          : return ik9;
        case KEY_A          : return ikA;
        case KEY_B          : return ikB;
        case KEY_C          : return ikC;
        case KEY_D          : return ikD;
        case KEY_E          : return ikE;
        case KEY_F          : return ikF;
        case KEY_G          : return ikG;
        case KEY_H          : return ikH;
        case KEY_I          : return ikI;
        case KEY_J          : return ikJ;
        case KEY_K          : return ikK;
        case KEY_L          : return ikL;
        case KEY_M          : return ikM;
        case KEY_N          : return ikN;
        case KEY_O          : return ikO;
        case KEY_P          : return ikP;
        case KEY_Q          : return ikQ;
        case KEY_R          : return ikR;
        case KEY_S          : return ikS;
        case KEY_T          : return ikT;
        case KEY_U          : return ikU;
        case KEY_V          : return ikV;
        case KEY_W          : return ikW;
        case KEY_X          : return ikX;
        case KEY_Y          : return ikY;
        case KEY_Z          : return ikZ;
        case KEY_HOMEPAGE   : return ikEscape;
    // mouse
        case BTN_LEFT       : return ikMouseL;
        case BTN_RIGHT      : return ikMouseR;
        case BTN_MIDDLE     : return ikMouseM;
    }
    return ikNone;
}

JoyKey codeToJoyKey(int code) {
    switch (code) {
    // gamepad
        case BTN_A          : return jkA;
        case BTN_B          : return jkB;
        case BTN_X          : return jkX;
        case BTN_Y          : return jkY;
        case BTN_TL         : return jkLB;
        case BTN_TR         : return jkRB;
        case BTN_SELECT     : return jkSelect;
        case BTN_START      : return jkStart;
        case BTN_THUMBL     : return jkL;
        case BTN_THUMBR     : return jkR;
        case BTN_TL2        : return jkLT;
        case BTN_TR2        : return jkRT;
    }
    return jkNone;
}

int inputDevIndex(const char *node) {
    const char *str = strstr(node, "/event");
    if (str)
        return atoi(str + 6);
    return -1;
}

void inputDevAdd(const char *node) {
    int index = inputDevIndex(node);
    if (index != -1) {
        inputDevices[index] = open(node, O_RDONLY | O_NONBLOCK);
        ioctl(inputDevices[index], EVIOCGRAB, 1);
        //LOG("input: add %s\n", node);
    }
}

void inputDevRemove(const char *node) {
    int index = inputDevIndex(node);
    if (index != -1 && inputDevices[index] != -1) {
        close(inputDevices[index]);
        //LOG("input: remove %s\n", node);
    }
}

bool inputInit() {
    joyL = joyR = vec2(0);

    for (int i = 0; i < MAX_INPUT_DEVICES; i++)
        inputDevices[i] = -1;

    udevObj = udev_new();
    if (!udevObj)
        return false;

    udevMon = udev_monitor_new_from_netlink(udevObj, "udev");
    udev_monitor_filter_add_match_subsystem_devtype(udevMon, "input", NULL);
    udev_monitor_enable_receiving(udevMon);
    udevMon_fd = udev_monitor_get_fd(udevMon);

    udev_enumerate *e = udev_enumerate_new(udevObj);
    udev_enumerate_add_match_subsystem(e, "input");
    udev_enumerate_scan_devices(e);
    udev_list_entry *devices = udev_enumerate_get_list_entry(e);

    udev_list_entry *entry;
    udev_list_entry_foreach(entry, devices) {
        const char *path, *node;
        udev_device *device;

        path   = udev_list_entry_get_name(entry);
        device = udev_device_new_from_syspath(udevObj, path);
        node   = udev_device_get_devnode(device);

        if (node)
            inputDevAdd(node);
    }
    udev_enumerate_unref(e);

    return true;
}

void inputFree() {
    for (int i = 0; i < MAX_INPUT_DEVICES; i++)
        if (inputDevices[i] != -1)
            close(inputDevices[i]);
    udev_monitor_unref(udevMon);
    udev_unref(udevObj);
}

#define JOY_DEAD_ZONE_STICK      8192

float joyAxisValue(int value) {
    if (value > -JOY_DEAD_ZONE_STICK && value < JOY_DEAD_ZONE_STICK)
        return 0.0f;
    return value / 32767.0f;
}

float joyTrigger(int value) {
    return min(1.0f, value / 255.0f);
}

vec2 joyDir(const vec2 &value) {
    float dist = min(1.0f, value.length());
    return value.normal() * dist;
}

void inputUpdate() {
// get input events
    input_event events[16];

    for (int i = 0; i < MAX_INPUT_DEVICES; i++) {
        if (inputDevices[i] == -1) continue;
        int rb = read(inputDevices[i], events, sizeof(events));

        int joyIndex = 0; // TODO: joy index

        input_event *e = events;
        while (rb > 0) {
            switch (e->type) {
                case EV_KEY : {
                    InputKey key = codeToInputKey(e->code);
                    if (key != ikNone) {
                        if (key == ikMouseL || key == ikMouseR || key == ikMouseM)
                            Input::setPos(key, Input::mouse.pos);
                        Input::setDown(key, e->value != 0);
                    } else {
                        JoyKey key = codeToJoyKey(e->code);
                        Input::setJoyDown(joyIndex, key, e->value != 0);
                    }
                    break;
                }
                case EV_REL : {
                    vec2 delta(0);
                    delta[e->code] = float(e->value);
                    Input::setPos(ikMouseL, Input::mouse.pos + delta);
                    break;
                }
                case EV_ABS : {
                    switch (e->code) {
                    // Left stick
                        case ABS_X  : joyL.x = joyAxisValue(e->value); break;
                        case ABS_Y  : joyL.y = joyAxisValue(e->value); break;
                    // Right stick
                        case ABS_RX : joyR.x = joyAxisValue(e->value); break;
                        case ABS_RY : joyR.y = joyAxisValue(e->value); break;
                    // Left trigger
                        case ABS_Z  : Input::setJoyPos(joyIndex, jkLT, joyTrigger(e->value)); break;
                    // Right trigger
                        case ABS_RZ : Input::setJoyPos(joyIndex, jkRT, joyTrigger(e->value)); break;
                    // D-PAD
                        case ABS_HAT0X    :
                        case ABS_THROTTLE :
                            Input::setJoyDown(joyIndex, jkLeft,  e->value < 0);
                            Input::setJoyDown(joyIndex, jkRight, e->value > 0);
                            break;
                        case ABS_HAT0Y    :
                        case ABS_RUDDER   :
                            Input::setJoyDown(joyIndex, jkUp,    e->value < 0);
                            Input::setJoyDown(joyIndex, jkDown,  e->value > 0);
                            break;
                    }

                    Input::setJoyPos(joyIndex, jkL, joyDir(joyL));
                    Input::setJoyPos(joyIndex, jkR, joyDir(joyR));
                }
            }
            //LOG("input: type = %d, code = %d, value = %d\n", int(e->type), int(e->code), int(e->value));
            e++;
            rb -= sizeof(events[0]);
        }
    }

// monitoring plug and unplug input devices
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(udevMon_fd, &fds);

    timeval tv;
    tv.tv_sec  = 0;
    tv.tv_usec = 0;

    if (select(udevMon_fd + 1, &fds, NULL, NULL, &tv) > 0 && FD_ISSET(udevMon_fd, &fds)) {
        udev_device *device = udev_monitor_receive_device(udevMon);
        if (device) {
            const char *node = udev_device_get_devnode(device);
            if (node) {
                const char *action = udev_device_get_action(device);
                if (!strcmp(action, "add"))
                    inputDevAdd(node);
                if (!strcmp(action, "remove"))
                    inputDevRemove(node);
            }
            udev_device_unref(device);
        } else
            LOG("! input: receive_device\n");
    }
}

int main(int argc, char **argv) {

    int w, h;
    SDL_DisplayMode current;

    SDL_Init(SDL_INIT_VIDEO);

    SDL_GetCurrentDisplayMode(0, &current);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    // We start in fullscreen mode using the vide mode currently in use, to avoid video mode changes.
    SDL_Window *window = SDL_CreateWindow(WND_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
	current.w, current.h, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN_DESKTOP);
   
    // We try to use the current video mode, but we inform the core of whatever mode SDL2 gave us in the end. 
    SDL_GetWindowSize(window, &w, &h);

    Core::width  = w;
    Core::height = h;

    SDL_GLContext context = SDL_GL_CreateContext(window);
    SDL_GL_SetSwapInterval(0);

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1,
	  SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);

    SDL_ShowCursor(SDL_DISABLE);

    cacheDir[0] = saveDir[0] = contentDir[0] = 0;

    const char *home;
    if (!(home = getenv("HOME")))
        home = getpwuid(getuid())->pw_dir;
    strcat(cacheDir, home);
    strcat(cacheDir, "/.openlara/");

    struct stat st = {0};
    if (stat(cacheDir, &st) == -1 && mkdir(cacheDir, 0777) == -1)
        cacheDir[0] = 0;
    strcpy(saveDir, cacheDir);

    timeval t;
    gettimeofday(&t, NULL);
    startTime = t.tv_sec;

    sndInit();

    char *lvlName = argc > 1 ? argv[1] : NULL;

    Game::init(lvlName);

    inputInit(); // initialize and grab input devices

    while (!Core::isQuit) {
        inputUpdate();

        if (Game::update()) {
            Game::render();
            Core::waitVBlank();
	    SDL_GL_SwapWindow(window);
        }
    };

    inputFree();

    sndFree();
    Game::deinit();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
