#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <linux/input.h>
#include <X11/Xutil.h>
#include <linux/joystick.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <pwd.h>
#include <pthread.h>
#include <pulse/pulseaudio.h>
#include <pulse/simple.h>

#include "game.h"

#define WND_TITLE       "OpenLara"

// timing
unsigned int startTime;

int osGetTimeMS() {
    timeval t;
    gettimeofday(&t, NULL);
    return int((t.tv_sec - startTime) * 1000 + t.tv_usec / 1000);
}

// sound
#define SND_FRAME_SIZE  4
#define SND_DATA_SIZE   (2352 * SND_FRAME_SIZE)

pa_simple *sndOut;
pthread_t sndThread;

Sound::Frame *sndData;

void* sndFill(void *arg) {
    while (1) {
        Sound::fill(sndData, SND_DATA_SIZE / SND_FRAME_SIZE);
        pa_simple_write(sndOut, sndData, SND_DATA_SIZE, NULL);
    }
    return NULL;
}

void sndInit() {
    static const pa_sample_spec spec = {
        .format   = PA_SAMPLE_S16LE,
        .rate     = 44100,
        .channels = 2
    };

    static const pa_buffer_attr attr = {
        .maxlength  = SND_DATA_SIZE * 4,
        .tlength    = 0xFFFFFFFF,
        .prebuf     = 0xFFFFFFFF,
        .minreq     = SND_DATA_SIZE,
        .fragsize   = 0xFFFFFFFF,
    };

    int error;
    if (!(sndOut = pa_simple_new(NULL, WND_TITLE, PA_STREAM_PLAYBACK, NULL, "game", &spec, NULL, &attr, &error))) {
        LOG("pa_simple_new() failed: %s\n", pa_strerror(error));
        sndData = NULL;
        return;
    }

    sndData = new Sound::Frame[SND_DATA_SIZE / SND_FRAME_SIZE];
    pthread_create(&sndThread, NULL, sndFill, NULL);
}

void sndFree() {
    if (sndOut) {
        pthread_cancel(sndThread);
    //    pa_simple_flush(sndOut, NULL);
    //    pa_simple_free(sndOut);
        delete[] sndData;
    }
}

// Input
InputKey keyToInputKey(Display *dpy, XKeyEvent event) {
    KeySym code = XLookupKeysym(&event, 0);

    if (code == XK_Shift_R)   code = XK_Shift_L;
    if (code == XK_Control_R) code = XK_Control_L;
    if (code == XK_Alt_R)     code = XK_Alt_L;

    KeySym codes[] = {
        XK_Left, XK_Right, XK_Up, XK_Down, XK_space, XK_Tab, XK_Return, XK_Escape, XK_Shift_L, XK_Control_L, XK_Alt_L,
        XK_0, XK_1, XK_2, XK_3, XK_4, XK_5, XK_6, XK_7, XK_8, XK_9,
        XK_a, XK_b, XK_c, XK_d, XK_e, XK_f, XK_g, XK_h, XK_i, XK_j, XK_k, XK_l, XK_m,
        XK_n, XK_o, XK_p, XK_q, XK_r, XK_s, XK_t, XK_u, XK_v, XK_w, XK_x, XK_y, XK_z,
        XK_KP_0, XK_KP_1, XK_KP_2, XK_KP_3, XK_KP_4, XK_KP_5, XK_KP_6, XK_KP_7, XK_KP_8, XK_KP_9, XK_KP_Add, XK_KP_Subtract, XK_KP_Multiply, XK_KP_Divide, XK_KP_Separator,
        XK_F1, XK_F2, XK_F3, XK_F4, XK_F5, XK_F6, XK_F7, XK_F8, XK_F9, XK_F10, XK_F11, XK_F12,
        XK_minus, XK_equal, XK_bracketleft, XK_bracketright, XK_slash, XK_backslash, XK_comma, XK_period, XK_grave, XK_semicolon, XK_apostrophe, XK_Page_Up, XK_Page_Down, XK_Home, XK_End, XK_Delete, XK_Insert, XK_BackSpace
    };

    for (int i = 0; i < COUNT(codes); i++) {
        if (XKeysymToKeycode(dpy, codes[i]) == event.keycode) {
            return (InputKey)(ikLeft + i);
        }
    }
    return ikNone;
}

InputKey mouseToInputKey(int btn) {
    switch (btn) {
        case 1 : return ikMouseL;
        case 2 : return ikMouseM;
        case 3 : return ikMouseR;
    }
    return ikNone;
}

#define JOY_DEAD_ZONE_STICK      8192
#define JOY_DEAD_ZONE_TRIGGER    8192
#define JOY_MIN_UPDATE_FX_TIME   50.0f

struct JoyDevice {
    int   fd;     // device file descriptor
    int   fe;     // event file descriptor
    vec2  L, R;   // left/right stick axes values
    float vL, vR; // current value for left/right motor vibration
    float oL, oR; // last applied value
    int   time;   // time when we can send effect update
    ff_effect fx; // effect structure
    uint8_t axismap[ABS_CNT]; // axis mapping
} joyDevice[INPUT_JOY_COUNT];

bool osJoyReady(int index) {
    return joyDevice[index].fd != -1;
}

void osJoyVibrate(int index, float L, float R) {
    if (!osJoyReady(index)) return;
    joyDevice[index].vL = L;
    joyDevice[index].vR = R;
}

void joyInit() {
    LOG("init gamepads:\n");
    char name[128];
    for (int i = 0; i < INPUT_JOY_COUNT; i++) {
        JoyDevice &joy = joyDevice[i];
    // open device
        sprintf(name, "/dev/input/js%d", i);
        joy.fd = open(name, O_RDONLY | O_NONBLOCK);
        if (joy.fd == -1)
            continue;
    // skip init messages
        js_event event;
        while (read(joy.fd, &event, sizeof(event)) != -1 && (event.type & JS_EVENT_INIT));
    // get gamepad info
        int8 axes, buttons;
        ioctl(joy.fd, JSIOCGAXES,    &axes);
        ioctl(joy.fd, JSIOCGBUTTONS, &buttons);
        ioctl(joy.fd, JSIOCGAXMAP, joy.axismap);

        if (axes < 4 || buttons < 11) { // is it really a gamepad?
            close(joy.fd);
            joy.fd = -1;
            continue;
        }

        if (ioctl(joy.fd, JSIOCGNAME(sizeof(name)), name) < 0)
            strcpy(name, "Unknown");

        LOG("gamepad %d\n", i + 1);
        LOG(" name : %s\n", name);
        LOG(" btns : %d\n", int(buttons));
        LOG(" axes : %d\n", int(axes));

        joy.fe = -1;
        for (int j = 0; j < 99; j++) {
            sprintf(name, "/sys/class/input/js%d/device/event%d", i, j);
            DIR *dir = opendir(name);
            if (!dir) continue;
            closedir(dir);
            sprintf(name, "/dev/input/event%d", j);
            joy.fe = open(name, O_RDWR);
            break;
        }

        uint32 features[4];
        if (joy.fe > -1 && (ioctl(joy.fe, EVIOCGBIT(EV_FF, sizeof(features)), features) == -1 || !TEST_BIT(features, FF_RUMBLE))) {
            close(joy.fe);
            joy.fe = -1;
        }

        if (joy.fe > -1) {
            int n_effects;
            if (ioctl(joy.fe, EVIOCGEFFECTS, &n_effects) == -1) {
                perror("Ioctl query");
            }
            LOG(" vibration feature %d\n", n_effects);
            joy.fx.id           = -1;
            joy.fx.type         = FF_RUMBLE;
            joy.fx.replay.delay = 0;
            joy.vL = joy.oL = joy.vR = joy.oR = 0.0f;
            joy.time  = Core::getTime();
        }
    }
}

void joyFree() {
    for (int i = 0; i < INPUT_JOY_COUNT; i++) {
        JoyDevice &joy = joyDevice[i];
        if (joy.fd == -1) continue;
        close(joy.fd);
        if (joy.fe == -1) continue;
    }
}

float joyAxisValue(int value) {
    if (value > -JOY_DEAD_ZONE_STICK && value < JOY_DEAD_ZONE_STICK)
        return 0.0f;
    return value / 32767.0f;
}

float joyTrigger(int value) {
    if (value + 32767 < JOY_DEAD_ZONE_TRIGGER)
        return 0.0f;
    return min(1.0f, (value + 32767) / 65536.0f);
}

vec2 joyDir(const vec2 &value) {
    float dist = min(1.0f, value.length());
    return value.normal() * dist;
}

void joyRumble(JoyDevice &joy) {
    if (joy.fe == -1) return;

    if (joy.oL == 0.0f && joy.vL == 0.0f && joy.oR == 0.0f && joy.vR == 0.0f)
        return;

    if (Core::getTime() <= joy.time)
        return;

    input_event event;
    event.type = EV_FF;

    if (joy.vL != 0.0f || joy.vR != 0.0f) {
    // update effect
        joy.fx.u.rumble.strong_magnitude = int(joy.vL * 65535);
        joy.fx.u.rumble.weak_magnitude   = int(joy.vR * 65535);
        joy.fx.replay.length             = int(max(JOY_MIN_UPDATE_FX_TIME, 1000.0f / Core::stats.fps));

        if (ioctl(joy.fe, EVIOCSFF, &joy.fx) == -1) {
            LOG("! joy update fx\n");
        }

    // play effect
        event.value = 1;
        event.code  = joy.fx.id;
        if (write(joy.fe, &event, sizeof(event)) == -1)
            LOG("! joy play fx\n");
    } else
        if (joy.oL != 0.0f || joy.oR != 0.0f) {
        // stop effect
            event.value = 0;
            event.code  = joy.fx.id;
            if (write(joy.fe, &event, sizeof(event)) == -1)
                LOG("! joy stop fx\n");
        }

    joy.oL = joy.vL;
    joy.oR = joy.vR;

    joy.time = Core::getTime() + joy.fx.replay.length;
}

void joyUpdate() {
    static const JoyKey keys[] = { jkA, jkB, jkX, jkY, jkLB, jkRB, jkSelect, jkStart, jkNone /*jkHome*/, jkL, jkR };

    for (int i = 0; i < INPUT_JOY_COUNT; i++) {
        JoyDevice &joy = joyDevice[i];

        if (joy.fd == -1)
            continue;

        joyRumble(joy);

        js_event event;
        while (read(joy.fd, &event, sizeof(event)) != -1) {
        // buttons
            if (event.type & JS_EVENT_BUTTON)
                Input::setJoyDown(i, event.number >= COUNT(keys) ? jkNone : keys[event.number], event.value == 1);
        // axes
            if (event.type & JS_EVENT_AXIS) {

                switch (joy.axismap[event.number]) {
                // Left stick
                    case ABS_X  : joy.L.x = joyAxisValue(event.value); break;
                    case ABS_Y  : joy.L.y = joyAxisValue(event.value); break;
                // Right stick
                    case ABS_RX : joy.R.x = joyAxisValue(event.value); break;
                    case ABS_RY : joy.R.y = joyAxisValue(event.value); break;
                // Left trigger
                    case ABS_Z  : Input::setJoyPos(i, jkLT, joyTrigger(event.value)); break;
                // Right trigger
                    case ABS_RZ : Input::setJoyPos(i, jkRT, joyTrigger(event.value)); break;
                // D-PAD
                    case ABS_HAT0X    :
                    case ABS_THROTTLE :
                        Input::setJoyDown(i, jkLeft,  event.value < -0x4000);
                        Input::setJoyDown(i, jkRight, event.value >  0x4000);
                        break;
                    case ABS_HAT0Y    :
                    case ABS_RUDDER   :
                        Input::setJoyDown(i, jkUp,    event.value < -0x4000);
                        Input::setJoyDown(i, jkDown,  event.value >  0x4000);
                        break;
                }

                Input::setJoyPos(i, jkL, joyDir(joy.L));
                Input::setJoyPos(i, jkR, joyDir(joy.R));
            }
        }
    }
}

// filesystem
#define MAX_FILES 4096
char* gFiles[MAX_FILES];
int32 gFilesCount;

void addDir(char* path)
{
    char* fileName;
    struct dirent* e;
    DIR* dir = opendir(path);

    int32 pathLen = strlen(path);
    path[pathLen] = '/';

    while ((e = readdir(dir)))
    {
        if (e->d_type == DT_DIR)
        {
            if (e->d_name[0] != '.')
            {
                strcpy(path + 1 + pathLen, e->d_name);
                addDir(path);
            }
        }
        else
        {
            ASSERT(gFilesCount < MAX_FILES);
            if (gFilesCount < MAX_FILES)
            {
                strcpy(path + 1 + pathLen, e->d_name);
                fileName = (char*)malloc(strlen(path) + 1 - 2);
                gFiles[gFilesCount++] = strcpy(fileName, path + 2);
            }
        }
    }

    path[pathLen] = '\0';
}

void fsInit()
{
    char path[1024];
    strcpy(path, ".");
    addDir(path);
    LOG("scan %d files\n", gFilesCount);
}

void fsFree()
{
    int32 i;
    for (i = 0; i < gFilesCount; i++)
    {
        free(gFiles[i]);
    }
}

const char* osFixFileName(const char* fileName)
{
    int32 i;
    for (i = 0; i < gFilesCount; i++)
    {
        if (!strcasecmp(fileName, gFiles[i]))
        {
            return gFiles[i];
        }
    }
    return NULL;
}

// system
void toggle_fullscreen(Display* dpy, Window win) {
    const size_t _NET_WM_STATE_TOGGLE=2;

    XEvent xev;
    Atom wm_state  =  XInternAtom(dpy, "_NET_WM_STATE", False);
    Atom scr_full  =  XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);

    memset(&xev, 0, sizeof(xev));
    xev.type = ClientMessage;
    xev.xclient.window = win;
    xev.xclient.message_type = wm_state;
    xev.xclient.format = 32;
    xev.xclient.data.l[0] = _NET_WM_STATE_TOGGLE;
    xev.xclient.data.l[1] = scr_full;

    XSendEvent(dpy, DefaultRootWindow(dpy), False, SubstructureNotifyMask, &xev);
}

void WndProc(const XEvent &e, Display* dpy, Window wnd) {
    switch (e.type) {
        case ConfigureNotify :
            Core::width  = e.xconfigure.width;
            Core::height = e.xconfigure.height;
            break;
        case KeyPress   :
        case KeyRelease :
            if (e.type == KeyPress && (e.xkey.state & Mod1Mask) && e.xkey.keycode == 36) {
                toggle_fullscreen(dpy,wnd);
                break;
            }
            Input::setDown(keyToInputKey(dpy, e.xkey), e.type == KeyPress);
            break;
        case ButtonPress :
        case ButtonRelease : {
            InputKey key = mouseToInputKey(e.xbutton.button);
            Input::setPos(key, Input::mouse.pos);
            Input::setDown(key, e.type == ButtonPress);
            break;
        }
        case MotionNotify :
            Input::setPos(ikMouseL, vec2((float)e.xmotion.x, (float)e.xmotion.y));
            break;
    }
}

int checkLanguage() {
    char *lang = getenv("LANG");
    if (!lang || strlen(lang) < 2) return 0;

    uint16 id;
    memcpy(&id, lang, 2);

    if (id == TWOCC("en")) return STR_LANG_EN - STR_LANG_EN;
    if (id == TWOCC("fr")) return STR_LANG_FR - STR_LANG_EN;
    if (id == TWOCC("de")) return STR_LANG_DE - STR_LANG_EN;
    if (id == TWOCC("es")) return STR_LANG_ES - STR_LANG_EN;
    if (id == TWOCC("it")) return STR_LANG_IT - STR_LANG_EN;
    if (id == TWOCC("pl")) return STR_LANG_PL - STR_LANG_EN;
    if (id == TWOCC("pt")) return STR_LANG_PT - STR_LANG_EN;
    if (id == TWOCC("uk")) return STR_LANG_RU - STR_LANG_EN;
    if (id == TWOCC("be")) return STR_LANG_RU - STR_LANG_EN;
    if (id == TWOCC("ru")) return STR_LANG_RU - STR_LANG_EN;
    if (id == TWOCC("ja")) return STR_LANG_JA - STR_LANG_EN;
    if (id == TWOCC("gr")) return STR_LANG_GR - STR_LANG_EN;
    if (id == TWOCC("fi")) return STR_LANG_FI - STR_LANG_EN;
    if (id == TWOCC("cs")) return STR_LANG_CZ - STR_LANG_EN;
    if (id == TWOCC("zh")) return STR_LANG_CN - STR_LANG_EN;
    if (id == TWOCC("hu")) return STR_LANG_HU - STR_LANG_EN;
    if (id == TWOCC("sv")) return STR_LANG_SV - STR_LANG_EN;

    return 0;
}

int main(int argc, char **argv) {
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

    static int XGLAttr[] = {
        GLX_RGBA,
        GLX_DOUBLEBUFFER,
        GLX_DEPTH_SIZE, 24,
        GLX_RED_SIZE, 8,
        GLX_GREEN_SIZE, 8,
        GLX_BLUE_SIZE, 8,
        GLX_ALPHA_SIZE, 8,
        0
    };

    Display *dpy = XOpenDisplay(NULL);
    XVisualInfo *vis = glXChooseVisual(dpy, XDefaultScreen(dpy), XGLAttr);

    XSetWindowAttributes attr;
    attr.colormap = XCreateColormap(dpy, RootWindow(dpy, vis->screen), vis->visual, AllocNone);
    attr.border_pixel = 0;
    attr.event_mask = KeyPressMask | KeyReleaseMask | StructureNotifyMask |
                      ButtonPressMask | ButtonReleaseMask |
                      ButtonMotionMask | PointerMotionMask;

    Window wnd = XCreateWindow(dpy, RootWindow(dpy, vis->screen),
                               0, 0, 1280, 720, 0,
                               vis->depth, InputOutput, vis->visual,
                               CWColormap | CWBorderPixel | CWEventMask, &attr);
    XStoreName(dpy, wnd, WND_TITLE);

    GLXContext ctx = glXCreateContext(dpy, vis, NULL, true);
    glXMakeCurrent(dpy, wnd, ctx);
    XMapWindow(dpy, wnd);

    Atom WM_DELETE_WINDOW = XInternAtom(dpy, "WM_DELETE_WINDOW", 0);
    XSetWMProtocols(dpy, wnd, &WM_DELETE_WINDOW, 1);

    timeval t;
    gettimeofday(&t, NULL);
    startTime = t.tv_sec;

    Core::defLang = checkLanguage();

    fsInit();

    joyInit();
    sndInit();
    Game::init(argc > 1 ? argv[1] : NULL);

    while (!Core::isQuit) {
        if (XPending(dpy)) {
            XEvent event;
            XNextEvent(dpy, &event);
            if (event.type == ClientMessage && *event.xclient.data.l == WM_DELETE_WINDOW)
                Core::quit();
            WndProc(event, dpy, wnd);
        } else {
            joyUpdate();
            bool updated = Game::update();
            if (updated) {
                Game::render();
                Core::waitVBlank();
                glXSwapBuffers(dpy, wnd);
            }
        }
    };

    joyFree();
    sndFree();
    Game::deinit();

    fsFree();

    glXMakeCurrent(dpy, 0, 0);
    XCloseDisplay(dpy);
    return 0;
}
