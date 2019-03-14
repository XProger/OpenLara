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
#include <libudev.h>
#include <alsa/asoundlib.h>

#include <wayland-client.h>
#include <wayland-egl.h>

#include "game.h"

#define WND_TITLE    "OpenLara"

#define WND_WIDTH    1280
#define WND_HEIGHT   720

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
wl_display       *wlDisplay;
wl_compositor    *wlCompositor = NULL;
wl_shell         *wlShell = NULL;
wl_surface       *wlSurface;
wl_shell_surface *wlShellSurface;
wl_egl_window    *wlEGLWindow;

EGLDisplay display;
EGLSurface surface;
EGLContext context;

// Wayland Listeners
void wlEventObjectAdd(void* data, wl_registry* registry, uint32_t name, const char* interface, uint32_t version) {
    if (!strcmp(interface, "wl_compositor")) {
        wlCompositor = (wl_compositor*)wl_registry_bind(registry, name, &wl_compositor_interface, 1);
    } else if (!strcmp(interface, "wl_shell")) {
        wlShell = (wl_shell*)wl_registry_bind(registry, name, &wl_shell_interface, 1);
    }
}

void wlEventObjectRemove(void* data, wl_registry* registry, uint32_t name) {
    //
}

wl_registry_listener wlRegistryListener = {
    &wlEventObjectAdd,
    &wlEventObjectRemove
};

void wlEventSurfacePing(void* data, wl_shell_surface* shell_surface, uint32_t serial) {
    wl_shell_surface_pong(shell_surface, serial);
}

void wlEventSurfaceConfig(void* data, wl_shell_surface* shell_surface, uint32_t edges, int32_t width, int32_t height) {
    wl_egl_window_resize(wlEGLWindow, width, height, 0, 0);
    Core::width  = width;
    Core::height = height;
}

void wlEnentSurfacePopup(void* data, wl_shell_surface* shell_surface) {
    //
}

wl_shell_surface_listener wlSurfaceListener = {
    &wlEventSurfacePing,
    &wlEventSurfaceConfig,
    &wlEnentSurfacePopup
};

bool eglInit() {
    LOG("EGL init context...\n");

    wlDisplay = wl_display_connect(NULL);
    wl_registry* registry = wl_display_get_registry(wlDisplay);
    wl_registry_add_listener(registry, &wlRegistryListener, NULL);
    wl_display_roundtrip(wlDisplay);
    
    static const EGLint eglAttr[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
        EGL_BLUE_SIZE,       8,
        EGL_GREEN_SIZE,      8,
        EGL_RED_SIZE,        8,
        EGL_ALPHA_SIZE,      8,
        EGL_NONE
    };

    static const EGLint ctxAttr[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    display = eglGetDisplay((EGLNativeDisplayType)wlDisplay);;
    if (display == EGL_NO_DISPLAY) {
        LOG("eglGetDisplay = EGL_NO_DISPLAY\n");
        return false;
    }

    if (eglInitialize(display, NULL, NULL) == EGL_FALSE) {
        LOG("eglInitialize = EGL_FALSE\n");
        return false;
    }

    eglBindAPI(EGL_OPENGL_API);

    EGLConfig config;
    EGLint configCount;

    if (eglChooseConfig(display, eglAttr, &config, 1, &configCount) == EGL_FALSE) {
        LOG("eglChooseConfig = EGL_FALSE\n");
        return false;
    }

    context = eglCreateContext(display, config, EGL_NO_CONTEXT, ctxAttr);
    if (context == EGL_NO_CONTEXT) {
        LOG("eglCreateContext = EGL_NO_CONTEXT\n");
        return false;
    }

    wlSurface = wl_compositor_create_surface(wlCompositor);
    wlShellSurface = wl_shell_get_shell_surface(wlShell, wlSurface);
    wl_shell_surface_add_listener(wlShellSurface, &wlSurfaceListener, NULL);
    wl_shell_surface_set_toplevel(wlShellSurface);

    wlEGLWindow = wl_egl_window_create(wlSurface, WND_WIDTH, WND_HEIGHT);

    surface = eglCreateWindowSurface(display, config, wlEGLWindow, NULL);
    if (surface == EGL_NO_SURFACE) {
        LOG("eglCreateWindowSurface = EGL_NO_SURFACE\n");
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
    wl_egl_window_destroy(wlEGLWindow);
    wl_shell_surface_destroy(wlShellSurface);
    wl_surface_destroy(wlSurface);
    eglDestroyContext(display, context);
    eglTerminate(display);
    wl_display_disconnect(wlDisplay);
}

// Input
#define MAX_INPUT_DEVICES 16
struct InputDevice {
    int fd;
    int joyIndex;
} inputDevices[MAX_INPUT_DEVICES];

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
    // system keys
        case KEY_VOLUMEUP   :
        case BTN_MODE       :
            Core::quit();
            return ikNone;
    }
    return ikNone;
}

JoyKey codeToJoyKey(int code) {
    switch (code) {
    // gamepad
        case BTN_TRIGGER_HAPPY1 : return jkLeft;
        case BTN_TRIGGER_HAPPY2 : return jkRight;
        case BTN_TRIGGER_HAPPY3 : return jkUp;
        case BTN_TRIGGER_HAPPY4 : return jkDown;
        case BTN_A          : return jkB;
        case BTN_B          : return jkA;
        case BTN_X          : return jkY;
        case BTN_Y          : return jkX;
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

void inputDevAdd(const char *node, udev_device *device) {
    int index = inputDevIndex(node);
    if (index != -1) {
        InputDevice &item = inputDevices[index];
        item.fd = open(node, O_RDONLY | O_NONBLOCK);
        ioctl(item.fd, EVIOCGRAB, 1);

        item.joyIndex = -1;
        if (udev_device_get_property_value(device, "ID_INPUT_JOYSTICK")) {

            // TODO get index from /dev/input/js[N]

            for (int i = 0; i < 4; i++) { // up to 4 gamepads
                bool found = true;
                for (int j = 0; j < MAX_INPUT_DEVICES; j++) {
                    if (inputDevices[j].joyIndex == i) {
                        found = false;
                        break;
                    }
                }
                
                if (found) {
                    item.joyIndex = i;
                    break;
                }
            }
        }

        //LOG("input: add %s (%d)\n", node, item.joyIndex);
    }
}

void inputDevRemove(const char *node) {
    int index = inputDevIndex(node);
    if (index != -1 && inputDevices[index].fd != -1) {
        close(inputDevices[index].fd);
        //LOG("input: remove %s\n", node);
    }
}

bool inputInit() {
    joyL = joyR = vec2(0);

    for (int i = 0; i < MAX_INPUT_DEVICES; i++) {
        inputDevices[i].fd       = -1;
        inputDevices[i].joyIndex = -1;
    }

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
            inputDevAdd(node, device);
    }
    udev_enumerate_unref(e);

    return true;
}

void inputFree() {
    for (int i = 0; i < MAX_INPUT_DEVICES; i++)
        if (inputDevices[i].fd != -1)
            close(inputDevices[i].fd);
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
        if (inputDevices[i].fd == -1) continue;
        int rb = read(inputDevices[i].fd, events, sizeof(events));

        int joyIndex = inputDevices[i].joyIndex;

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
                        if (joyIndex == -1)
                            break;
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
                    if (joyIndex == -1)
                        break;
                        
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
                    inputDevAdd(node, device);
                if (!strcmp(action, "remove"))
                    inputDevRemove(node);
            }
            udev_device_unref(device);
        } else
            LOG("! input: receive_device\n");
    }
}

int main(int argc, char **argv) {
    if (!eglInit()) {
        LOG("! can't initialize EGL context\n");
        return -1;
    }
    
    //Core::width  = fb.width;
    //Core::height = fb.height;

    cacheDir[0] = saveDir[0] = contentDir[0] = 0;

    strcpy(contentDir, argv[0]);
    int i = strlen(contentDir);
    while (--i >= 0) {
        if (contentDir[i] == '/') {
            contentDir[i + 1] = 0;
            break;
        }
        i--;
    }

    strcpy(cacheDir, contentDir);
    strcat(cacheDir, "cache/");

    struct stat st = {0};
    if (stat(cacheDir, &st) == -1 && mkdir(cacheDir, 0777) == -1)
        cacheDir[0] = 0;

    const char *home;
    if (!(home = getenv("HOME")))
        home = getpwuid(getuid())->pw_dir;
    strcpy(saveDir, home);
    strcat(saveDir, "/.openlara/");

    if (stat(saveDir, &st) == -1 && mkdir(saveDir, 0777) == -1)
        saveDir[0] = 0;

    timeval t;
    gettimeofday(&t, NULL);
    startTime = t.tv_sec;

    Game::init();
    inputInit();
    sndInit();

    while (!Core::isQuit) {
        wl_display_dispatch_pending(wlDisplay);

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
