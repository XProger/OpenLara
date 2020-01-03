#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <stdio.h>
#include <math.h>
#include <pthread.h>
#include <cstring>

#include <jni.h>
#include <android/log.h>
#include <EGL/egl.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include "android_native_app_glue.h"

#include "game.h"

JNIEnv *env;

// timing
time_t startTime;

int osGetTimeMS() {
    timeval t;
    gettimeofday(&t, NULL);
    return int((t.tv_sec - startTime) * 1000 + t.tv_usec / 1000);
}

// sound
// 1176 - 26 ms latency
#define          SND_FRAMES 1176

Sound::Frame     sndBuf[2][SND_FRAMES];
int              sndBufIndex;

SLObjectItf      sndEngine;
SLObjectItf      sndOutput;
SLObjectItf      sndPlayer;
SLBufferQueueItf sndQueue = NULL;
SLPlayItf        sndPlay  = NULL;

void sndFill(SLBufferQueueItf bq, void *context) {
    if (!sndQueue) return;
    Sound::fill(sndBuf[sndBufIndex ^= 1], SND_FRAMES);
    (*sndQueue)->Enqueue(sndQueue, sndBuf[sndBufIndex], SND_FRAMES * sizeof(Sound::Frame));
}

void sndSetState(bool active) {
    if (!sndPlay) return;
    (*sndPlay)->SetPlayState(sndPlay, active ? SL_PLAYSTATE_PLAYING : SL_PLAYSTATE_PAUSED);
}

void sndInit() {
    slCreateEngine(&sndEngine, 0, NULL, 0, NULL, NULL);
    (*sndEngine)->Realize(sndEngine, SL_BOOLEAN_FALSE);

    SLEngineItf engine;

    (*sndEngine)->GetInterface(sndEngine, SL_IID_ENGINE, &engine);
    (*engine)->CreateOutputMix(engine, &sndOutput, 0, NULL, NULL);
    (*sndOutput)->Realize(sndOutput, SL_BOOLEAN_FALSE);

    SLDataFormat_PCM bufFormat;
    bufFormat.formatType    = SL_DATAFORMAT_PCM;
    bufFormat.numChannels   = 2;
    bufFormat.samplesPerSec = SL_SAMPLINGRATE_44_1;
    bufFormat.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16;
    bufFormat.containerSize = SL_PCMSAMPLEFORMAT_FIXED_16;
    bufFormat.channelMask   = SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT ;
    bufFormat.endianness    = SL_BYTEORDER_LITTLEENDIAN;

    SLDataLocator_AndroidSimpleBufferQueue bufLocator;
    bufLocator.locatorType = SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE;
    bufLocator.numBuffers  = 2;
    
    SLDataLocator_OutputMix snkLocator;
    snkLocator.locatorType = SL_DATALOCATOR_OUTPUTMIX;
    snkLocator.outputMix   = sndOutput;

    SLDataSource audioSrc;
    audioSrc.pLocator = &bufLocator;
    audioSrc.pFormat  = &bufFormat;

    SLDataSink audioSnk;
    audioSnk.pLocator = &snkLocator;
    audioSnk.pFormat  = NULL;

    SLInterfaceID audioInt[] = { SL_IID_BUFFERQUEUE, SL_IID_PLAY  };
    SLboolean     audioReq[] = { SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE };

    (*engine)->CreateAudioPlayer(engine, &sndPlayer, &audioSrc, &audioSnk, 2, audioInt, audioReq);
    (*sndPlayer)->Realize(sndPlayer, SL_BOOLEAN_FALSE);
    (*sndPlayer)->GetInterface(sndPlayer, SL_IID_BUFFERQUEUE, &sndQueue);
    (*sndPlayer)->GetInterface(sndPlayer, SL_IID_PLAY, &sndPlay);
    (*sndQueue)->RegisterCallback(sndQueue, sndFill, NULL);

    sndBufIndex = 1;
    sndFill(sndQueue, NULL);
    sndFill(sndQueue, NULL);
}

void sndFree() {
    if (sndPlayer) (*sndPlayer)->Destroy(sndPlayer);
    if (sndOutput) (*sndOutput)->Destroy(sndOutput);
    if (sndEngine) (*sndEngine)->Destroy(sndEngine);
    sndPlayer = sndOutput = sndEngine = NULL;
    sndQueue = NULL;
    sndPlay  = NULL;
}

// joystick
int joyIndex[INPUT_JOY_COUNT];
int joyCount = 0;

static const int joyCodes[] = {
        0, AKEYCODE_BUTTON_A, AKEYCODE_BUTTON_B, AKEYCODE_BUTTON_X, AKEYCODE_BUTTON_Y, AKEYCODE_BUTTON_L1, AKEYCODE_BUTTON_R1, 
        AKEYCODE_BUTTON_SELECT, AKEYCODE_BUTTON_START, AKEYCODE_BUTTON_THUMBL, AKEYCODE_BUTTON_THUMBR, AKEYCODE_BUTTON_L2, AKEYCODE_BUTTON_R2,
        AKEYCODE_DPAD_LEFT, AKEYCODE_DPAD_RIGHT, AKEYCODE_DPAD_UP, AKEYCODE_DPAD_DOWN };

static const int keyCodes[] {
        0, 21, 22, 19, 20, 62, 61, 66, 111, 59, 113, 57,
        7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
        29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41,
        42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54 };

int getItemIndex(int value, const int *items, int count) {
    for (int i = 0; i < count; i++)
        if (items[i] == value)
            return i;
    return -1;
}

int getJoyIndex(int id) {
    int index = getItemIndex(id, joyIndex, joyCount);
    if (index == -1 && joyCount < COUNT(joyIndex)) {
        joyIndex[joyCount++] = id;
        return joyCount - 1;
    }
    return index;
}

float joyDeadZone(float x) {
    return x = fabsf(x) < 0.2f ? 0.0f : x;
}

int32_t onInputEvent(android_app *app, AInputEvent *event) {
    int  source    = AInputEvent_getSource(event);
    bool isGamepad = (source & (AINPUT_SOURCE_GAMEPAD | AINPUT_SOURCE_JOYSTICK | AINPUT_SOURCE_DPAD)) != 0;

    switch (AInputEvent_getType(event)) {
        case AINPUT_EVENT_TYPE_KEY : {
            bool isDown = AKeyEvent_getAction(event) == AKEY_EVENT_ACTION_DOWN;
            int keyCode = AKeyEvent_getKeyCode(event);
            int keyIndex;

            if (keyCode == AKEYCODE_BACK)
                keyCode = AKEYCODE_ESCAPE;

            if (isGamepad && (keyIndex = getItemIndex(keyCode, joyCodes, COUNT(joyCodes))) != -1) {
                int index = getJoyIndex(AInputEvent_getDeviceId(event));
                if (index == -1)
                    return 0;
                Input::setJoyDown(index, JoyKey(keyIndex), isDown);
                return 1;
            } else if ((keyIndex = getItemIndex(keyCode, keyCodes, COUNT(keyCodes))) != -1) {
                Input::setDown(InputKey(keyIndex), isDown);
                return 1;
            }
            break;
        }
        case AINPUT_EVENT_TYPE_MOTION : {
            int action = AMotionEvent_getAction(event);
            int flags  = action & AMOTION_EVENT_ACTION_MASK;

            if (isGamepad) {
                int index = getJoyIndex(AInputEvent_getDeviceId(event));
                if (index == -1)
                    return 0;
                
                Input::setJoyPos(index, jkL, vec2(joyDeadZone(AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_X, 0)),
                                                  joyDeadZone(AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_Y, 0))));
                Input::setJoyPos(index, jkR, vec2(joyDeadZone(AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_Z, 0)),
                                                  joyDeadZone(AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_RZ, 0))));

                if (!(source & AINPUT_SOURCE_DPAD)) {
                    float dx = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_HAT_X, 0);
                    float dy = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_HAT_Y, 0);

                    Input::setJoyDown(index, jkLeft,  dx < -0.9);
                    Input::setJoyDown(index, jkRight, dx >  0.9);
                    Input::setJoyDown(index, jkUp,    dy < -0.9);
                    Input::setJoyDown(index, jkDown,  dy >  0.9);
                }

                return 1;
            }

            switch (flags) {
                case AMOTION_EVENT_ACTION_DOWN :
                case AMOTION_EVENT_ACTION_UP   :
                case AMOTION_EVENT_ACTION_MOVE :
                    for (int i = 0; i < AMotionEvent_getPointerCount(event); i++) {
                        InputKey key = Input::getTouch(AMotionEvent_getPointerId(event, i));
                        if (key == ikNone) continue;
                        Input::setPos(key, vec2(AMotionEvent_getX(event, i), AMotionEvent_getY(event, i)));
                        if (flags == AMOTION_EVENT_ACTION_DOWN || flags == AMOTION_EVENT_ACTION_UP)
                            Input::setDown(key, flags == AMOTION_EVENT_ACTION_DOWN);
                    }
                    return 1;

                case AMOTION_EVENT_ACTION_POINTER_DOWN :
                case AMOTION_EVENT_ACTION_POINTER_UP   :
                    int i = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
                    InputKey key = Input::getTouch(AMotionEvent_getPointerId(event, i));
                    if (key == ikNone) break;
                    Input::setPos(key, vec2(AMotionEvent_getX(event, i), AMotionEvent_getY(event, i)));
                    Input::setDown(key, flags == AMOTION_EVENT_ACTION_POINTER_DOWN);
                    return 1;
            }

            break;
        }
    }

    return 0;
}

bool osJoyReady(int index) {
    return index < joyCount;
}

void osJoyVibrate(int index, float L, float R) {
    //
}

// display
android_app *app;

EGLDisplay display = EGL_NO_DISPLAY;
EGLSurface surface = EGL_NO_SURFACE;
EGLContext context = EGL_NO_CONTEXT;
EGLint     format;
EGLConfig  config;

bool isActive = false;

void osToggleVR(bool enable) {
    Core::settings.detail.stereo = Core::Settings::STEREO_OFF;
}

void eglInitSurface() {
    ANativeWindow_setBuffersGeometry(app->window, 0, 0, format);
    surface = eglCreateWindowSurface(display, config, app->window, NULL);

    eglMakeCurrent(display, surface, surface, context);
    eglQuerySurface(display, surface, EGL_WIDTH,  &Core::width);
    eglQuerySurface(display, surface, EGL_HEIGHT, &Core::height);
}

void eglFreeSurface() {
    if (display == EGL_NO_DISPLAY || surface == EGL_NO_SURFACE) return;

    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroySurface(display, surface);

    surface = EGL_NO_SURFACE;
}

void eglInit() {
    static const EGLint eglAttr[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
        EGL_RED_SIZE,        8,
        EGL_GREEN_SIZE,      8,
        EGL_BLUE_SIZE,       8,
        EGL_ALPHA_SIZE,      8,
        EGL_DEPTH_SIZE,      24,
        EGL_SAMPLES,         0,
        EGL_NONE
    };

    static const EGLint ctxAttr[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    EGLint numConfigs;

    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    eglInitialize(display, 0, 0);
    eglChooseConfig(display, eglAttr, &config, 1, &numConfigs);
    eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);

    context = eglCreateContext(display, config, EGL_NO_CONTEXT, ctxAttr);

    eglInitSurface();
}

void eglFree() {
    if (display == EGL_NO_DISPLAY) return;

    eglFreeSurface();

    if (context != EGL_NO_CONTEXT)
        eglDestroyContext(display, context);

    eglTerminate(display);
    display = EGL_NO_DISPLAY;
    context = EGL_NO_CONTEXT;
}

void onAppCmd(android_app *app, int32_t cmd) {
    LOG("android_app_cmd %d\n", cmd);


    switch (cmd) {
        case APP_CMD_SAVE_STATE:
            //app->savedStateSize = ...;
            //app->savedState = malloc(app->savedStateSize);
            //TODO: fill save state
            break;

        case APP_CMD_INIT_WINDOW:
            if (app->window == NULL) return;

            LOG("---- init context\n");

            if (context == EGL_NO_CONTEXT) {
                LOG("---- init gl\n");
                eglInit();
                LOG("---- init game\n");
                Game::init();
                LOG("---- init gound\n");
                sndInit();
            } else
                eglInitSurface();

            LOG("---- init done!\n");

            break;

        case APP_CMD_TERM_WINDOW:
            if (app->window == NULL) return;
            eglFreeSurface();
            break;

        case APP_CMD_GAINED_FOCUS :
        case APP_CMD_LOST_FOCUS   :
            isActive = cmd == APP_CMD_GAINED_FOCUS;
            sndSetState(isActive);
            Core::resetTime();
            break;
    }
}

void android_main(android_app *state) {
    LOG("android_main\n");

    cacheDir[0] = saveDir[0] = contentDir[0] = 0;

    state->activity->vm->AttachCurrentThread(&env, 0);

    state->onAppCmd     = onAppCmd;
    state->onInputEvent = onInputEvent;
    app = state;


    if (state->savedState != NULL) {
        // TODO: load level from save state
    }

    timeval t;
    gettimeofday(&t, NULL);
    startTime = t.tv_sec;

    LOG("get content dir: ");

    strcpy(contentDir, getenv("EXTERNAL_STORAGE"));
    strcat(contentDir, "/OpenLara/"); // skip /Android/data/com.OpenLara/files
    strcpy(cacheDir, contentDir);
    strcat(cacheDir, "cache/");
    strcpy(saveDir, contentDir);

    LOG("%s\n", contentDir);

    Core::isQuit = false;

    while (!state->destroyRequested) {
        android_poll_source *source;

        while (ALooper_pollAll(isActive ? 0 : -1, NULL, NULL, (void**)&source) >= 0) {
            if (source != NULL)
                source->process(state, source);

            if (state->destroyRequested != 0)
                break;
        }

        if (display != EGL_NO_DISPLAY && isActive && Game::update()) {
            Game::render();
            eglSwapBuffers(display, surface);
        }

        if (Core::isQuit)
            ANativeActivity_finish(state->activity);
    }

    Game::deinit();
    eglFree();
    sndFree();

    state->activity->vm->DetachCurrentThread();
}
