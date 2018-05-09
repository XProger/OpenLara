#include <stdio.h>
#include <cstring>
#include <pthread.h>
#include <EGL/egl.h>

#include "game.h"

int lastJoy = -1;
EGLDisplay display;
EGLSurface surface;
EGLContext context;

// timing
int osGetTime() {
    return (int)emscripten_get_now();
}

// storage and data downloading
const char *IDB = "db";

void onError(void *arg) {
    Stream *stream = (Stream*)arg;
    LOG("! IDB error for %s\n", stream->name);
    if (stream->callback)
        stream->callback(NULL, stream->userData);
    delete stream;
}

void onLoad(void *arg, void *data, int size) {
    Stream *stream = (Stream*)arg;

    FILE *f = fopen(stream->name, "wb");
    fwrite(data, size, 1, f);
    fclose(f);

    stream->callback(new Stream(stream->name), stream->userData);   
    delete stream;
}

void onLoadAndStore(void *arg, void *data, int size) {
    emscripten_idb_async_store(IDB, ((Stream*)arg)->name, data, size, NULL, NULL, onError);
    onLoad(arg, data, size);
}

void onExists(void *arg, int exists) {
    if (exists)
        emscripten_idb_async_load(IDB, ((Stream*)arg)->name, arg, onLoad, onError);
    else
        emscripten_async_wget_data(((Stream*)arg)->name, arg, onLoadAndStore, onError);
}

void osDownload(Stream *stream) {
    emscripten_idb_async_exists(IDB, stream->name, stream, onExists, onError);
}

void onCacheStore(void *arg) {
    Stream *stream = (Stream*)arg;
    LOG("cache stored: %s\n", stream->name);
    if (stream->callback)    
        stream->callback(new Stream(stream->name, NULL, 0), stream->userData);
    delete stream;
}

void onCacheLoad(void *arg, void *data, int size) {
    Stream *stream = (Stream*)arg;
    LOG("cache loaded: %s\n", stream->name);
    if (stream->callback)
        stream->callback(new Stream(stream->name, data, size), stream->userData);   
    delete stream;
}

void onCacheError(void *arg) {
    Stream *stream = (Stream*)arg;
    LOG("! cache error: %s\n", stream->name);
    if (stream->callback)
        stream->callback(NULL, stream->userData);
    delete stream;
}

void osCacheWrite(Stream *stream) {
    emscripten_idb_async_store(IDB, stream->name, stream->data, stream->size, stream, onCacheStore, onCacheError);
}

void osCacheRead(Stream *stream) {
    emscripten_idb_async_load(IDB, stream->name, stream, onCacheLoad, onCacheError);
}

// memory card
void osSaveGame(Stream *stream) {
    return osCacheWrite(stream);
}

void osLoadGame(Stream *stream) {
    return osCacheRead(stream);
}

JoyKey joyToInputKey(int code) {
    static const JoyKey keys[16] = {
        jkA, jkB, jkX, jkY, jkLB, jkRB, jkLT, jkRT, jkSelect, jkStart, jkL, jkR, jkUp, jkDown, jkLeft, jkRight,
    };

    if (code >= 0 && code < 16)
        return keys[code];
    
    return jkNone;
}

#define JOY_DEAD_ZONE_STICK      0.3f
#define JOY_DEAD_ZONE_TRIGGER    0.01f

vec2 joyAxis(float x, float y) {
    if (fabsf(x) > JOY_DEAD_ZONE_STICK || fabsf(y) > JOY_DEAD_ZONE_STICK)
        return vec2(x, y);
    return vec2(0.0f);
}

vec2 joyTrigger(float x) {
    return vec2(x > JOY_DEAD_ZONE_TRIGGER ? x : 0.0f, 0.0f);
}

bool joyReady[INPUT_JOY_COUNT] = { 0 };

bool osJoyReady(int index) {
    return joyReady[index];
}

void osJoyVibrate(int index, float L, float R) {
    //
}

void joyUpdate() {
    int count = min(emscripten_get_num_gamepads(), INPUT_JOY_COUNT);
    for (int j = 0; j < count; j++) {
        EmscriptenGamepadEvent state;

        bool wasReady = joyReady[j];
        joyReady[j] = emscripten_get_gamepad_status(j, &state) == EMSCRIPTEN_RESULT_SUCCESS;
        if (!joyReady[j]) {
            if (wasReady)
                Input::reset();
            continue;
        }

        for (int i = 0; i < state.numButtons; i++) {
            JoyKey key = joyToInputKey(i);
            Input::setJoyDown(j, key, state.digitalButton[i]);
            if (key == jkLT || key == jkRT)
                Input::setJoyPos(j, key, joyTrigger(state.analogButton[i]));
        }

        Input::setJoyPos(j, jkL, joyAxis(state.axis[0], state.axis[1]));
        Input::setJoyPos(j, jkR, joyAxis(state.axis[2], state.axis[3]));
    }
}

void main_loop() {
    joyUpdate();

    if (Game::update()) {
        Game::render();
        
    // clear backbuffer alpha by 1.0f to make opaque canvas layer
        glColorMask(false, false, false, true);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glColorMask(true, true, true, true);
        
        eglSwapBuffers(display, surface);
    }
}

bool initGL() {
    EGLint vMajor, vMinor, cfgCount;
    EGLConfig cfg;

    EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE, EGL_NONE };
    EGLint attribList[] = {
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 8,
        EGL_STENCIL_SIZE, EGL_DONT_CARE,
        EGL_SAMPLE_BUFFERS, EGL_DONT_CARE,
        EGL_NONE
    };

    display = eglGetDisplay((EGLNativeDisplayType)EGL_DEFAULT_DISPLAY);
    eglInitialize(display, &vMajor, &vMinor);
    eglGetConfigs(display, NULL, 0, &cfgCount);
    eglChooseConfig(display, attribList, &cfg, 1, &cfgCount);
    surface = eglCreateWindowSurface(display, cfg, NULL, NULL);
    context = eglCreateContext(display, cfg, EGL_NO_CONTEXT, contextAttribs);
    eglMakeCurrent(display, surface, surface, context);
    return true;
}

void freeGL() {
    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(display, context);
    eglDestroySurface(display, surface);
    eglTerminate(display);
}

EM_BOOL resize() {
    //int f;
    //emscripten_get_canvas_size(&Core::width, &Core::height, &f);
    double w, h;
    emscripten_get_element_css_size(NULL, &w, &h);
    Core::width  = int(w);
    Core::height = int(h);
    emscripten_set_canvas_size(Core::width, Core::height);
    LOG("resize %d %d\n", Core::width, Core::height);
    return 1;
}

EM_BOOL resizeCallback(int eventType, const EmscriptenUiEvent *e, void *userData) {
    return resize();
}

EM_BOOL fullscreenCallback(int eventType, const void *reserved, void *userData) {
    return resize();
}

bool isFullScreen() {
    EmscriptenFullscreenChangeEvent status;
    emscripten_get_fullscreen_status(&status);
    return status.isFullscreen;
}

extern "C" {
    void EMSCRIPTEN_KEEPALIVE snd_fill(Sound::Frame *frames, int count) {
        Sound::fill(frames, count);
    }
    
    void EMSCRIPTEN_KEEPALIVE game_level_load(char *data, int size) {
        Game::startLevel(new Stream(NULL, data, size));
    }
    
    void EMSCRIPTEN_KEEPALIVE change_fs_mode() {
        EM_ASM(JSEvents.inEventHandler = true);
        EM_ASM(JSEvents.currentEventHandler = {allowsDeferredCalls:true});
        
        if (!isFullScreen()) {
            EmscriptenFullscreenStrategy s;
            s.scaleMode                 = EMSCRIPTEN_FULLSCREEN_SCALE_STRETCH;
            s.canvasResolutionScaleMode = EMSCRIPTEN_FULLSCREEN_CANVAS_SCALE_HIDEF;
            s.filteringMode             = EMSCRIPTEN_FULLSCREEN_FILTERING_DEFAULT;
            s.canvasResizedCallback     = fullscreenCallback;
            emscripten_request_fullscreen_strategy(NULL, 0, &s);
        } else {
            LOG("exit fs\n");
            emscripten_exit_fullscreen();
        }
    }
}

InputKey keyToInputKey(int code) {
    static const int codes[] = {
        0x25, 0x27, 0x26, 0x28, 0x20, 0x09, 0x0D, 0x1B, 0x10, 0x11, 0x12,
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
        'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
    };

    for (int i = 0; i < sizeof(codes) / sizeof(codes[0]); i++)
        if (codes[i] == code)
            return (InputKey)(ikLeft + i);
    return ikNone;
}

EM_BOOL keyCallback(int eventType, const EmscriptenKeyboardEvent *e, void *userData) {
    switch(eventType) {
        case EMSCRIPTEN_EVENT_KEYDOWN:
        case EMSCRIPTEN_EVENT_KEYUP:
            if (eventType == EMSCRIPTEN_EVENT_KEYDOWN && e->altKey && e->keyCode == 0x0D) {  // Alt + Enter
                change_fs_mode();
                break;
            }
            Input::setDown(keyToInputKey(e->keyCode), eventType == EMSCRIPTEN_EVENT_KEYDOWN);
            break;
    }
    return 1;
}

EM_BOOL touchCallback(int eventType, const EmscriptenTouchEvent *e, void *userData) {
    for (int i = 0; i < e->numTouches; i++) {
        if (!e->touches[i].isChanged) continue;
        InputKey key = Input::getTouch(e->touches[i].identifier);
        if (key == ikNone) continue;
        Input::setPos(key, vec2(e->touches[i].canvasX, e->touches[i].canvasY));

        if (eventType == EMSCRIPTEN_EVENT_TOUCHSTART || eventType == EMSCRIPTEN_EVENT_TOUCHEND || eventType == EMSCRIPTEN_EVENT_TOUCHCANCEL) 
            Input::setDown(key, eventType == EMSCRIPTEN_EVENT_TOUCHSTART);
    }
    return 0;
}

EM_BOOL mouseCallback(int eventType, const EmscriptenMouseEvent *e, void *userData) {
    switch (eventType) {
        case EMSCRIPTEN_EVENT_MOUSEDOWN :
        case EMSCRIPTEN_EVENT_MOUSEUP : {
            InputKey key = e->button == 0 ? ikMouseL : (e->button == 2 ? ikMouseR : ikMouseM);
            Input::setPos(key, vec2((float)e->clientX, (float)e->clientY));
            Input::setDown(key, eventType != EMSCRIPTEN_EVENT_MOUSEUP);
            break;
        }
        case EMSCRIPTEN_EVENT_MOUSEMOVE :
            Input::setPos(ikMouseL, vec2((float)e->clientX, (float)e->clientY));
            break;
    }
    return 1;
}

char Stream::cacheDir[255];
char Stream::contentDir[255];

int main() {
    Stream::contentDir[0] = Stream::cacheDir[0] = 0;
    
    initGL(); 

    emscripten_set_keydown_callback(0, 0, 1, keyCallback);
    emscripten_set_keyup_callback(0, 0, 1, keyCallback);
    emscripten_set_resize_callback(0, 0, 0, resizeCallback);

    emscripten_set_touchstart_callback(0, 0, 0, touchCallback);
    emscripten_set_touchend_callback(0, 0, 0, touchCallback);
    emscripten_set_touchmove_callback(0, 0, 0, touchCallback);
    emscripten_set_touchcancel_callback(0, 0, 0, touchCallback);

    emscripten_set_mousedown_callback(0, 0, 1, mouseCallback);
    emscripten_set_mouseup_callback(0, 0, 1, mouseCallback);
    emscripten_set_mousemove_callback(0, 0, 1, mouseCallback);
    
    Game::init();
    emscripten_run_script("snd_init()");
    resize();

    emscripten_set_main_loop(main_loop, 0, true);

    Game::deinit();
    freeGL();

    return 0;
}