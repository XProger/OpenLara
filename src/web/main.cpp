#include <stdio.h>
#include <EGL/egl.h>

#include "game.h"

int lastTime, fpsTime, fps;
EGLDisplay display;
EGLSurface surface;
EGLContext context;

int getTime() {
    return (int)emscripten_get_now();
}

extern "C" {
    void EMSCRIPTEN_KEEPALIVE snd_fill(Sound::Frame *frames, int count) {
		Sound::fill(frames, count);		
    }
}

void main_loop() {
    int time = getTime();

    if (time - lastTime <= 0)
        return;

    float delta = (time - lastTime) * 0.001f;
    while (delta > EPS) {
        Core::deltaTime = min(delta, 1.0f / 30.0f);
        Game::update();
        delta -= Core::deltaTime;
    }
    lastTime = time;

    int f;
    emscripten_get_canvas_size(&Core::width, &Core::height, &f);

    Core::stats.dips = 0;
    Core::stats.tris = 0;
    Game::render();
    eglSwapBuffers(display, surface);

    if (fpsTime < getTime()) {
        LOG("FPS: %d DIP: %d TRI: %d\n", fps, Core::stats.dips, Core::stats.tris);
        fps = 0;
        fpsTime = getTime() + 1000;
    } else
        fps++;
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

InputKey keyToInputKey(int code) {
    int codes[] = {
        0x25, 0x27, 0x26, 0x28, 0x20, 0x0D, 0x1B, 0x10, 0x11, 0x12,
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
            Input::setDown(keyToInputKey(e->keyCode), eventType == EMSCRIPTEN_EVENT_KEYDOWN);
            break;
    }
    return 1;
}

EM_BOOL resizeCallback(int eventType, const EmscriptenUiEvent *e, void *userData) {
//    Core::width = e->documentBodyClientWidth;
//    Core::height = e->documentBodyClientHeight;
    int f;
    emscripten_get_canvas_size(&Core::width, &Core::height, &f);
    LOG("resize %d x %d\n", Core::width, Core::height);
    return 1;
}

EM_BOOL touchCallback(int eventType, const EmscriptenTouchEvent *e, void *userData) {
    bool flag = false;
    /*
    for (int i = 0; i < e->numTouches; i++) {
        long x = e->touches[i].canvasX;
        long y = e->touches[i].canvasY;
        if (x < 0 || y < 0 || x >= Core::width || y >= Core::height) continue;
        flag = true;

        switch (eventType) {
            case EMSCRIPTEN_EVENT_TOUCHSTART :
                if (x > Core::width / 2)
                    Input::joy.down[(y > Core::height / 2) ? 1 : 4] = true;
                break;
            case EMSCRIPTEN_EVENT_TOUCHMOVE :
                if (x < Core::width / 2) {
                    vec2 center(Core::width * 0.25f, Core::height * 0.6f);
                    vec2 pos(x, y);
                    vec2 d = pos - center;

                    Input::Joystick &joy = Input::joy;

                    joy.L.x = d.x;
                    joy.L.y = d.y;
                    if (joy.L != vec2(0.0f))
                        joy.L.normalize();
                }
                break;
            case EMSCRIPTEN_EVENT_TOUCHEND :
            case EMSCRIPTEN_EVENT_TOUCHCANCEL : {
                Input::joy.L = vec2(0.0f);
                Input::joy.down[1] = false;
                Input::joy.down[4] = false;
                break;
            }
        }
    }
    */
    return flag;
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
    initGL();

    emscripten_set_canvas_size(Core::width = 854, Core::height = 480);

    emscripten_set_keydown_callback(0, 0, 1, keyCallback);
    emscripten_set_keyup_callback(0, 0, 1, keyCallback);
    emscripten_set_resize_callback(0, 0, 1, resizeCallback);

    emscripten_set_touchstart_callback(0, 0, 1, touchCallback);
    emscripten_set_touchend_callback(0, 0, 1, touchCallback);
    emscripten_set_touchmove_callback(0, 0, 1, touchCallback);
    emscripten_set_touchcancel_callback(0, 0, 1, touchCallback);

    emscripten_set_mousedown_callback(0, 0, 1, mouseCallback);
    emscripten_set_mouseup_callback(0, 0, 1, mouseCallback);
    emscripten_set_mousemove_callback(0, 0, 1, mouseCallback);

    Game::init();

    emscripten_run_script("snd_init()");

    lastTime = getTime();
    fpsTime  = lastTime + 1000;
    fps      = 0;

    emscripten_set_main_loop(main_loop, 0, true);

    Game::free();
    freeGL();

    return 0;
}