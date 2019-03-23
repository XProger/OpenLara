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

void main_loop(void *data, wl_callback *callback, uint32_t time);

bool configured = false;

void configure_callback(void *data, wl_callback *callback, uint32_t  time) {
    wl_callback_destroy(callback);
    configured = true;
    main_loop(data, NULL, time);
}

wl_callback_listener configure_callback_listener = {
    configure_callback,
};

bool eglInit() {
    LOG("EGL init context...\n");

    wlDisplay = wl_display_connect(NULL);
    wl_registry* registry = wl_display_get_registry(wlDisplay);
    wl_registry_add_listener(registry, &wlRegistryListener, NULL);
    wl_display_dispatch(wlDisplay);
    
    static const EGLint eglAttr[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
        EGL_BLUE_SIZE,       8,
        EGL_GREEN_SIZE,      8,
        EGL_RED_SIZE,        8,
        EGL_ALPHA_SIZE,      8,
        EGL_DEPTH_SIZE,      24,
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

    eglBindAPI(EGL_OPENGL_ES_API);

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

    wl_shell_surface_set_fullscreen(wlShellSurface, WL_SHELL_SURFACE_FULLSCREEN_METHOD_DEFAULT, 0, NULL);

    wl_callback *callback = wl_display_sync(wlDisplay);
    wl_callback_add_listener(callback, &configure_callback_listener, NULL);

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

JoyKey codeToJoyKey(int code) {
    switch (code) {
    // gamepad
        case BTN_B   : return jkB;
        case BTN_C   : return jkA;
        case BTN_A   : return jkY;
        case BTN_X   : return jkX;
        case BTN_TL  : return jkLB;
        case BTN_TR  : return jkRB;
        case BTN_TL2 : return jkSelect;
        case BTN_TR2 : return jkStart;
        case BTN_Y   : return jkLT;
        case BTN_Z   : return jkRT;
    }
    return jkNone;
}

void inputUpdate() {
// get input events
    input_event events[16];

    for (int i = 0; i < MAX_INPUT_DEVICES; i++) {
        if (inputDevices[i].fd == -1) continue;
        int rb = read(inputDevices[i].fd, events, sizeof(events));

        int joyIndex = inputDevices[i].joyIndex;
        if (joyIndex == -1) continue;

        input_event *e = events;
        while (rb > 0) {
            switch (e->type) {
                case EV_KEY : {
                    JoyKey key = codeToJoyKey(e->code);
                    Input::setJoyDown(joyIndex, key, e->value != 0);
                    break;
                }
                case EV_ABS : {
                    switch (e->code) {
                        case ABS_X : {
                            if (e->value == 1) {
                                Input::setJoyDown(joyIndex, jkLeft,  false);
                                Input::setJoyDown(joyIndex, jkRight, false);
                            } else {
                                Input::setJoyDown(joyIndex, e->value ? jkRight : jkLeft, true);
                            }
                            break;
                        }
                        case ABS_Y : {
                            if (e->value == 1) {
                                Input::setJoyDown(joyIndex, jkUp,   false);
                                Input::setJoyDown(joyIndex, jkDown, false);
                            } else {
                                Input::setJoyDown(joyIndex, e->value ? jkDown : jkUp, true);
                            }
                            break;
                        }
                    }
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

wl_callback_listener frame_listener = {
    main_loop
};

void main_loop(void *data, wl_callback *callback, uint32_t time) {
    if (!configured)
        return;

    if (callback) {
        wl_callback_destroy(callback);
    }

    inputUpdate();

    Game::update();
    Game::render();
    Core::waitVBlank();

    wl_region *region = wl_compositor_create_region(wlCompositor);
    wl_region_add(region, 0, 0, Core::width, Core::height);
    wl_surface_set_opaque_region(wlSurface, region);
    wl_region_destroy(region);

    {
        wl_callback *callback = wl_surface_frame(wlSurface);
        wl_callback_add_listener(callback, &frame_listener, NULL);
    }
            
    eglSwapBuffers(display, surface);
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

    while (!Core::isQuit && wl_display_dispatch(wlDisplay) != -1);

    inputFree();

    Game::deinit();
    eglFree();

    sndFree();

    return 0;
}
