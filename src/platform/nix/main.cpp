#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <pthread.h>
#include <pulse/pulseaudio.h>
#include <pulse/simple.h>

#include "game.h"

#define WND_TITLE       "OpenLara"

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
#define SND_FRAME_SIZE  4
#define SND_DATA_SIZE   (1024 * SND_FRAME_SIZE)

pa_simple *sndOut;
pthread_t sndThread;
pthread_mutex_t sndMutex;

Sound::Frame *sndData;

void* sndFill(void *arg) {
    while (1) {
        pthread_mutex_lock(&sndMutex);
        Sound::fill(sndData, SND_DATA_SIZE / SND_FRAME_SIZE);
        pthread_mutex_unlock(&sndMutex);
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

    pthread_mutex_init(&sndMutex, NULL);

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
        pthread_mutex_lock(&sndMutex);
    //    pa_simple_flush(sndOut, NULL);
    //    pa_simple_free(sndOut);
        pthread_mutex_unlock(&sndMutex);
        delete[] sndData;
    }
    pthread_mutex_destroy(&sndMutex);
}

// Input
InputKey keyToInputKey(int code) {
    int codes[] = {
        113, 114, 111, 116, 65, 23, 36, 9, 50, 37, 64,
        19, 10, 11, 12, 13, 14, 15, 16, 17, 18,
        38, 56, 54, 40, 26, 41, 42, 43, 31, 44, 45, 46, 58,
        57, 32, 33, 24, 27, 39, 28, 30, 55, 25, 53, 29, 52,
    };

    for (int i = 0; i < sizeof(codes) / sizeof(codes[0]); i++)
        if (codes[i] == code)
            return (InputKey)(ikLeft + i);
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

void WndProc(const XEvent &e,Display*dpy,Window wnd) {
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
            Input::setDown(keyToInputKey(e.xkey.keycode), e.type == KeyPress);
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

char Stream::cacheDir[255];
char Stream::contentDir[255];

int main(int argc, char **argv) {
    Stream::contentDir[0] = Stream::cacheDir[0] = 0;

    const char *home;
    if (!(home = getenv("HOME")))
        home = getpwuid(getuid())->pw_dir;
    strcat(Stream::cacheDir, home);
    strcat(Stream::cacheDir, "/.OpenLara/");
    
    struct stat st = {0};    
    if (stat(Stream::cacheDir, &st) == -1 && mkdir(Stream::cacheDir, 0777) == -1)
        Stream::cacheDir[0] = 0;

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

    sndInit();
    Game::init(argc > 1 ? argv[1] : NULL);

    while (1) {
        if (XPending(dpy)) {
            XEvent event;
            XNextEvent(dpy, &event);
            if (event.type == ClientMessage && *event.xclient.data.l == WM_DELETE_WINDOW)
                break;
            WndProc(event,dpy,wnd);
        } else {
            pthread_mutex_lock(&sndMutex);
			bool updated = Game::update();
            pthread_mutex_unlock(&sndMutex);
            if (updated) {
				Game::render();
				glXSwapBuffers(dpy, wnd);
			}
        }
    };

    sndFree();
    Game::deinit();

    glXMakeCurrent(dpy, 0, 0);
    XCloseDisplay(dpy);
    return 0;
}
