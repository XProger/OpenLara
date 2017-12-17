#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <pthread.h>
#include <bcm_host.h>
#include <EGL/egl.h>
#include <fcntl.h>
#include <linux/input.h>
#include <libudev.h>
#include <alsa/asoundlib.h>

#include "game.h"

#define WND_TITLE    "OpenLara"

// Time
unsigned int startTime;

int osGetTime() {
    timeval t;
    gettimeofday(&t, NULL);
    return int((t.tv_sec - startTime) * 1000 + t.tv_usec / 1000);
}

bool osSave(const char *name, const void *data, int size) {
    FILE *f = fopen(name, "wb");
    if (!f) return false;
    fwrite(data, size, 1, f);
    fclose(f);
    return true;
}

// Sound
snd_pcm_uframes_t   SND_FRAMES = 512;
snd_pcm_t           *sndOut;
Sound::Frame        *sndData;
pthread_t           sndThread;
pthread_mutex_t     sndMutex;

void* sndFill(void *arg) {
    while (sndOut) {
        pthread_mutex_lock(&sndMutex);
        Sound::fill(sndData, SND_FRAMES);
        pthread_mutex_unlock(&sndMutex);

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

    pthread_mutex_init(&sndMutex, NULL);

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
    pthread_mutex_lock(&sndMutex);
    snd_pcm_drop(sndOut);
    snd_pcm_drain(sndOut);
    snd_pcm_close(sndOut);
    pthread_mutex_unlock(&sndMutex);
    pthread_mutex_destroy(&sndMutex);
    delete[] sndData;
}

// Window
bool wndInit(DISPMANX_DISPLAY_HANDLE_T &display, EGL_DISPMANX_WINDOW_T &window) {
    if (graphics_get_display_size(0, (uint32_t*)&window.width, (uint32_t*)&window.height) < 0) {
        LOG("! can't get display size\n");
        return false;
    }

    int scale = 1;
    window.width  /= scale;
    window.height /= scale;

    VC_RECT_T dstRect, srcRect;
    vc_dispmanx_rect_set(&dstRect, 0, 0, window.width, window.height);
    vc_dispmanx_rect_set(&srcRect, 0, 0, window.width << 16, window.height << 16);
    VC_DISPMANX_ALPHA_T alpha = { DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS, 0xFF, 0 };

    display = vc_dispmanx_display_open(0);

    DISPMANX_UPDATE_HANDLE_T update = vc_dispmanx_update_start(0);

    window.element = vc_dispmanx_element_add(
        update, display, 0, &dstRect, 0, &srcRect,
        DISPMANX_PROTECTION_NONE, &alpha, NULL, DISPMANX_NO_ROTATE);

    vc_dispmanx_update_submit_sync(update);

    return true;
}

void wndFree(DISPMANX_DISPLAY_HANDLE_T &display, EGL_DISPMANX_WINDOW_T &window) {
    DISPMANX_UPDATE_HANDLE_T update = vc_dispmanx_update_start(0);
    vc_dispmanx_element_remove(update, window.element);
    vc_dispmanx_update_submit_sync(update);
    vc_dispmanx_display_close(display);
}

bool eglInit(EGL_DISPMANX_WINDOW_T &window, EGLDisplay &display, EGLSurface &surface, EGLContext &context) {
    static const EGLint eglAttr[] = {
        EGL_RED_SIZE,       8,
        EGL_GREEN_SIZE,     8,
        EGL_BLUE_SIZE,      8,
        EGL_ALPHA_SIZE,     8,
        EGL_DEPTH_SIZE,     24,
        EGL_SAMPLES,        0,
        EGL_SURFACE_TYPE,   EGL_WINDOW_BIT,
        EGL_NONE
    };

    static const EGLint ctxAttr[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display == EGL_NO_DISPLAY)
        return false;

    if (eglInitialize(display, NULL, NULL) == EGL_FALSE)
        return false;

    EGLConfig config;
    EGLint configCount;

    if (eglChooseConfig(display, eglAttr, &config, 1, &configCount) == EGL_FALSE)
        return false;

    context = eglCreateContext(display, config, EGL_NO_CONTEXT, ctxAttr);
    if (context == EGL_NO_CONTEXT)
        return false;

    surface = eglCreateWindowSurface(display, config, &window, NULL);
    if (surface == EGL_NO_SURFACE)
        return false;

    if (eglSurfaceAttrib(display, surface, EGL_SWAP_BEHAVIOR, EGL_BUFFER_DESTROYED) == EGL_FALSE)
        return false;

    if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE)
        return false;

    //eglSwapInterval(display, 0); // turn off vsync

    return true;
}

void eglFree(EGLDisplay display, EGLSurface surface, EGLContext context) {
    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroySurface(display, surface);
    eglDestroyContext(display, context);
    eglTerminate(display);
}

// Input
#define MAX_INPUT_DEVICES 16
int inputDevices[MAX_INPUT_DEVICES];

udev *udevObj;
udev_monitor *udevMon;
int udevMon_fd;

vec2 joyL, joyR;

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
    // mouse
        case BTN_LEFT       : return ikMouseL;
        case BTN_RIGHT      : return ikMouseR;
        case BTN_MIDDLE     : return ikMouseM;
    // gamepad
        case KEY_HOMEPAGE   : return ikEscape;
        case BTN_A          : return ikJoyA;
        case BTN_B          : return ikJoyB;
        case BTN_X          : return ikJoyX;
        case BTN_Y          : return ikJoyY;
        case BTN_TL         : return ikJoyLB;
        case BTN_TR         : return ikJoyRB;
        case BTN_SELECT     : return ikJoySelect;
        case BTN_START      : return ikJoyStart;
        case BTN_THUMBL     : return ikJoyL;
        case BTN_THUMBR     : return ikJoyR;
        case BTN_TL2        : return ikJoyLT;
        case BTN_TR2        : return ikJoyRT;
    }
    return ikNone;
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

void inputUpdate() {
// get input events
    input_event events[16];

    for (int i = 0; i < MAX_INPUT_DEVICES; i++) {
        if (inputDevices[i] == -1) continue;
        int rb = read(inputDevices[i], events, sizeof(events));

        input_event *e = events;
        while (rb > 0) {
            switch (e->type) {
                case EV_KEY : {
                    InputKey key = codeToInputKey(e->code);
                    if (key == ikMouseL || key == ikMouseR || key == ikMouseM)
                        Input::setPos(key, Input::mouse.pos);
                    Input::setDown(key, e->value != 0);
                    break;
                }
                case EV_REL : {
                    vec2 delta(0);
                    delta[e->code] = float(e->value);
                    Input::setPos(ikMouseL, Input::mouse.pos + delta);
                    break;
                }
                case EV_ABS : {
                    float v = float(e->value) / 128.0f - 1.0f;
                    switch (e->code) {
                        case ABS_X  : joyL.x = v; break;
                        case ABS_Y  : joyL.y = v; break;
                        case ABS_Z  : joyR.x = v; break;
                        case ABS_RZ : joyR.y = v; break;
                    }
                }
            }
            //LOG("input: type = %d, code = %d, value = %d\n", int(e->type), int(e->code), int(e->value));
            e++;
            rb -= sizeof(events[0]);
        }
    }
    Input::setPos(ikJoyL, joyL);
    Input::setPos(ikJoyR, joyR);
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

char Stream::cacheDir[255];
char Stream::contentDir[255];

int main(int argc, char **argv) {
    bcm_host_init();

    DISPMANX_DISPLAY_HANDLE_T dmDisplay;
    EGL_DISPMANX_WINDOW_T     dmWindow;
    if (!wndInit(dmDisplay, dmWindow))
        return 1;
    Core::width  = dmWindow.width;
    Core::height = dmWindow.height;

    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    if (!eglInit(dmWindow, display, context, surface)) {
        LOG("! can't initialize EGL context\n");
        return 1;
    }

    Stream::contentDir[0] = Stream::cacheDir[0] = 0;

    const char *home;
    if (!(home = getenv("HOME")))
        home = getpwuid(getuid())->pw_dir;
    strcat(Stream::cacheDir, home);
    strcat(Stream::cacheDir, "/.OpenLara/");

    struct stat st = {0};
    if (stat(Stream::cacheDir, &st) == -1 && mkdir(Stream::cacheDir, 0777) == -1)
        Stream::cacheDir[0] = 0;

    timeval t;
    gettimeofday(&t, NULL);
    startTime = t.tv_sec;

    sndInit();

    char *lvlName = argc > 1 ? argv[1] : NULL;

    Game::init(lvlName);

    inputInit(); // initialize and grab input devices

    while (!Core::isQuit) {
        inputUpdate();

        pthread_mutex_lock(&sndMutex);
        bool updated = Game::update();
        pthread_mutex_unlock(&sndMutex);
		if (updated) {
			Game::render();
			eglSwapBuffers(display, surface);
		}
    };

    sndFree();
    Game::deinit();

    inputFree();
    eglFree(display, surface, context);
    wndFree(dmDisplay, dmWindow);

    return 0;
}
