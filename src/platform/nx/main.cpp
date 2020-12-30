#include <string.h>
#include <cstdlib>
#include <malloc.h>

#include <switch.h>
#include "game.h"

// multi-threading
void* osMutexInit() {
    RMutex *mutex = new RMutex();
    rmutexInit(mutex);
    return mutex;
}

void osMutexFree(void *obj) {
    delete ((RMutex*)obj);
}

void osMutexLock(void *obj) {
    rmutexLock((RMutex*)obj);
}

void osMutexUnlock(void *obj) {
    rmutexUnlock((RMutex*)obj);
}

// timing
u64 startTick;

int osGetTimeMS() {
    u64 tick = armGetSystemTick();
    return int(((tick - startTick) * 625 / 12) / 1000000);
}

// sound
#define SND_RESAMPLE    (44100.0f / 48000.0f)
#define SND_FRAMES_FILL 2352
#define SND_FRAMES_OUT  int(SND_FRAMES_FILL / SND_RESAMPLE)

Thread          sndThread;
AudioOutBuffer  sndOutBuffer[2];
Sound::Frame    *sndFrames;

void sndFill(void *args) {
    Sound::Frame *frames = new Sound::Frame[SND_FRAMES_FILL]; // some decoders 

    while (!Core::isQuit) {
        AudioOutBuffer *chunk;
        audoutWaitPlayFinish(&chunk, NULL, UINT64_MAX);

        Sound::fill(frames, SND_FRAMES_FILL);

        for (int j = 0; j < SND_FRAMES_OUT; j++) { // resample 44100 -> 48000
            ((Sound::Frame*)chunk->buffer)[j] = frames[int(j * SND_RESAMPLE)];
        }

        audoutAppendAudioOutBuffer(chunk);
    }

    delete[] frames;
}

bool sndInit() {
    audoutInitialize();

    memset(sndOutBuffer, 0, sizeof(sndOutBuffer));

    for (int i = 0; i < COUNT(sndOutBuffer); i++) {
        AudioOutBuffer &chunk = sndOutBuffer[i];
        chunk.data_size   = SND_FRAMES_OUT * sizeof(Sound::Frame);
        chunk.buffer_size = (chunk.data_size + 0xFFF) & ~0xFFF;
        chunk.buffer      = memalign(0x1000, chunk.buffer_size);
        memset(chunk.buffer, 0, chunk.buffer_size);
        audoutAppendAudioOutBuffer(&chunk);
    }

    audoutStartAudioOut();

    threadCreate(&sndThread, sndFill, NULL, NULL, 0x4000, 0x2B, 2);
    threadStart(&sndThread);

    return true;
}

void sndFree() {
    threadWaitForExit(&sndThread);
    threadClose(&sndThread);
    audoutStopAudioOut();
    audoutExit();
}

// Window
EGLDisplay display;
EGLSurface surface;
EGLContext context;
NWindow    *window;

void configureResolution() {
    if (appletGetOperationMode() == AppletOperationMode_Console) {
        Core::width = 1920;
        Core::height = 1080;
    }
    else {
        Core::width = 1280;
        Core::height = 720;
    }

    int offset = 1080 - Core::height;
    nwindowSetCrop(window, 0, offset, Core::width, Core::height + offset);
}

bool eglInit() {
    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display == EGL_NO_DISPLAY)
        return false;

    if (eglInitialize(display, NULL, NULL) == EGL_FALSE)
        return false;

    static const EGLint eglAttr[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
        EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
        EGL_RED_SIZE,        8,
        EGL_GREEN_SIZE,      8,
        EGL_BLUE_SIZE,       8,
        EGL_ALPHA_SIZE,      8,
        EGL_DEPTH_SIZE,      24,
        EGL_NONE
    };

    static const EGLint ctxAttr[] = {
        EGL_CONTEXT_MAJOR_VERSION, 2,
        EGL_CONTEXT_MINOR_VERSION, 0,
        EGL_NONE
    };

    EGLConfig config;
    EGLint configCount = 0;

    if (eglChooseConfig(display, eglAttr, &config, 1, &configCount) == EGL_FALSE)
        return false;

    window = nwindowGetDefault();
    nwindowSetDimensions(window, 1920, 1080);

    surface = eglCreateWindowSurface(display, config, (EGLNativeWindowType)window, NULL);

    if (surface == EGL_NO_SURFACE)
        return false;

    if (eglBindAPI(EGL_OPENGL_API) == EGL_FALSE)
        return false;

    context = eglCreateContext(display, config, EGL_NO_CONTEXT, ctxAttr);
    if (context == EGL_NO_CONTEXT)
        return false;

    if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE)
        return false;

    configureResolution();
    glClear(GL_COLOR_BUFFER_BIT);
    eglSwapBuffers(display, surface);

    return true;
}

void eglFree() {
    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroySurface(display, surface);
    eglDestroyContext(display, context);
    eglTerminate(display);
    eglReleaseThread();
}

// Input
bool osJoyReady(int index) {
    return index == 0;
}

void osJoyVibrate(int index, float L, float R) {
    // TODO
}

bool joyIsSplit;
int  joySplitTime;
PadState pads[2];

void joySplit(bool split) {
    joyIsSplit   = split;
    joySplitTime = Core::getTime();

    if (split) {
        hidSetNpadJoyAssignmentModeSingle(HidNpadIdType_No1, HidNpadJoyDeviceType_Left);
        hidSetNpadJoyAssignmentModeSingle(HidNpadIdType_No2, HidNpadJoyDeviceType_Right);
    } else {
        hidSetNpadJoyAssignmentModeDual(HidNpadIdType_No1);
        hidSetNpadJoyAssignmentModeDual(HidNpadIdType_No2);
        hidMergeSingleJoyAsDualJoy(HidNpadIdType_No1, HidNpadIdType_No2);

        if (Game::level && Game::level->players[1]) {
            Game::level->addPlayer(1); // add existing player == remove player
        }
    }
}

void joyInit() {
    hidSetNpadJoyHoldType(HidNpadJoyHoldType_Horizontal);

    padConfigureInput(2, HidNpadStyleSet_NpadStandard);
    padInitialize(&pads[0], HidNpadIdType_No1, HidNpadIdType_Handheld);
    padInitialize(&pads[1], HidNpadIdType_No2, HidNpadIdType_Handheld);

    joySplit(false);
}

void joyUpdate() {
    const static u64 keys[jkMAX] = { 0,
        KEY_B, KEY_A, KEY_Y, KEY_X, KEY_L, KEY_R, KEY_PLUS, KEY_MINUS, 
        KEY_LSTICK, KEY_RSTICK, KEY_ZL, KEY_ZR,
        KEY_DLEFT, KEY_DRIGHT, KEY_DUP, KEY_DDOWN,
    };

    bool waitForSplit = false;

    for (int i = 0; i < (joyIsSplit ? 2 : 1); i++) {
        padUpdate(&pads[i]);

        u64 mask = padGetButtonsDown(&pads[i]) | padGetButtons(&pads[i]);

        for (int j = 1; j < jkMAX; j++) {
            if (j != jkSelect && j != jkStart) {
                Input::setJoyDown(i, JoyKey(jkNone + j), (mask & keys[j]) != 0);
            }
        }

        Input::setJoyDown(i, jkSelect, (mask & (KEY_MINUS  | KEY_PLUS))   != 0);
        Input::setJoyDown(i, jkStart,  (mask & (KEY_LSTICK | KEY_RSTICK)) != 0);

        HidAnalogStickState sL = padGetStickPos(&pads[i], 0);
        HidAnalogStickState sR = padGetStickPos(&pads[i], 1);

        Input::setJoyPos(i, jkL, vec2(float(sL.x), float(-sL.y)) / 32767.0f);
        Input::setJoyPos(i, jkR, vec2(float(sR.x), float(-sR.y)) / 32767.0f);

        if ((mask & (KEY_L | KEY_R)) == (KEY_L | KEY_R)) { // hold L and R to split/merge joy-con's
            if (Core::getTime() - joySplitTime > 1000) { // 1 sec timer
                joySplit(!joyIsSplit);
            }
            waitForSplit = true;
        } 
    }

    if (!waitForSplit) {
        joySplitTime = Core::getTime();
    }
}

void touchUpdate() {
    HidTouchScreenState state;
    hidGetTouchScreenStates(&state, 1);

    bool touchState[COUNT(Input::touch)];

    for (int i = 0; i < COUNT(Input::touch); i++) {
        touchState[i] = Input::down[ikTouchA + i];
    }

    for (int i = 0; i < state.count; i++) {
        InputKey key = Input::getTouch(state.touches[i].finger_id);
        if (key == ikNone) continue;

        Input::setPos(key, vec2(float(state.touches[i].x), float(state.touches[i].y)));
        Input::setDown(key, true);

        touchState[key - ikTouchA] = false;
    }

    for (int i = 0; i < COUNT(Input::touch); i++) {
        if (touchState[i]) {
            Input::setDown(InputKey(ikTouchA + i), false);
        }
    }
}

void makeCacheDir(char *elfPath) {
    char buf[255];
    int len = strlen(elfPath);
    int start = 0;
    strcpy(buf, elfPath);

    // skip volume id
    for (int i = 0; i < len; i++) {
        if (buf[i] == ':') {
            start = i + 1;
            break;
        }
    }

    // skip executable name
    for (int i = len - 1; i >= 0; i--) {
        if (buf[i] == '/') {
            buf[i] = 0;
            break;
        }
    }

    // make directory by full path
    strcpy(cacheDir, buf + start);
    strcat(cacheDir, "/cache/");
    fsFsCreateDirectory(fsdevGetDeviceFileSystem("sdmc"), cacheDir);
}

int main(int argc, char* argv[]) {
    Core::width  = 1280;
    Core::height = 720;

    if (!eglInit()) {
        LOG("! can't initialize EGL context\n");
        return EXIT_FAILURE;
    }

    cacheDir[0] = saveDir[0] = contentDir[0] = 0;
    makeCacheDir(argv[0]);

    startTick = armGetSystemTick();

    sndInit();
    joyInit();

    Game::init();

    while (appletMainLoop() && !Core::isQuit) {
        joyUpdate();
        touchUpdate();

        if (Game::update()) {
            configureResolution();
            Game::render();
            Core::waitVBlank();
            eglSwapBuffers(display, surface);
        }
    };

    sndFree();
    Game::deinit();

    eglFree();

    return EXIT_SUCCESS;
}
