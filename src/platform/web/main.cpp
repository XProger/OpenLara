#include <stdio.h>
#include <cstring>
#include <pthread.h>

#include "game.h"

int lastJoy = -1;
EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context;
int WEBGL_VERSION;

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
        stream->callback(new Stream(stream->name, stream->data, stream->size), stream->userData);
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
void osReadSlot(Stream *stream) {
    return osCacheRead(stream);
}

void osWriteSlot(Stream *stream) {
    return osCacheWrite(stream);
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
    }
}

bool initGL() {
    EmscriptenWebGLContextAttributes attrs;
    emscripten_webgl_init_context_attributes(&attrs);
    attrs.alpha                     = true;
    attrs.depth                     = true;
    attrs.enableExtensionsByDefault = true;
    attrs.antialias                 = false;
    attrs.premultipliedAlpha        = false;
    attrs.majorVersion              = 2;

    context = emscripten_webgl_create_context(0, &attrs);
    if (!context) {
        attrs.majorVersion = 1;
        context = emscripten_webgl_create_context(0, &attrs);
    }

    if (!context) {
        LOG("! can't initialize WebGL !\n");
        return false;
    }
    WEBGL_VERSION = attrs.majorVersion;

    emscripten_webgl_make_context_current(context);

    char *ext = (char*)glGetString(GL_EXTENSIONS);
    if (ext != NULL) {
        char buf[255];
        int len = strlen(ext);
        int start = 0;
        for (int i = 0; i < len; i++)
            if (ext[i] == ' ' || (i == len - 1)) {
                memcpy(buf, &ext[start], i - start);
                buf[i - start] = 0;
                emscripten_webgl_enable_extension(context, buf);
                //LOG("enable: %s\n", buf);
                start = i + 1;
            }
    }

    return true;
}

void freeGL() {
    emscripten_webgl_destroy_context(context);
}

EM_BOOL resize() {
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

int main() {
    if (!initGL()) {
        return 0;
    }
    cacheDir[0] = saveDir[0] = contentDir[0] = 0;

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