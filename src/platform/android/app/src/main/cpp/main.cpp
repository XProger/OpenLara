#include <jni.h>
#include <android/log.h>
#include <sys/time.h>
#include <stdio.h>
#include <math.h>
#include <pthread.h>
#include <cstring>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

#include "game.h"

JavaVM *jvm;

#define JNI_METHOD(return_type, method_name) \
  JNIEXPORT return_type JNICALL              \
      Java_org_xproger_openlara_Wrapper_##method_name

// timing
time_t startTime;

int osGetTime() {
    timeval t;
    gettimeofday(&t, NULL);
    return int((t.tv_sec - startTime) * 1000 + t.tv_usec / 1000);
}

// sound
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
bool osJoyReady(int index) {
    return index == 0;
}

void osJoyVibrate(int index, float L, float R) {
    //
}

void osToggleVR(bool enable) {
    JNIEnv *jniEnv;
    jvm->AttachCurrentThread(&jniEnv, NULL);

    jboolean  v = enable;
    jclass    c = jniEnv->FindClass("org/xproger/openlara/MainActivity");
    jmethodID m = jniEnv->GetStaticMethodID(c, "toggleVR", "(Z)V");
    jniEnv->CallStaticVoidMethod(c, m, v);
}

extern "C" {

JNI_METHOD(void, nativeInit)(JNIEnv* env, jobject obj, jstring jcontentDir, jstring jcacheDir, jint langId) {
    env->GetJavaVM(&jvm);

    timeval t;
    gettimeofday(&t, NULL);
    startTime = t.tv_sec;	

    const char* str;

    cacheDir[0] = saveDir[0] = contentDir[0] = 0;
    Core::defLang = langId;

    str = env->GetStringUTFChars(jcontentDir, NULL);
    strcat(contentDir, str);
    env->ReleaseStringUTFChars(jcontentDir, str);

    str = env->GetStringUTFChars(jcacheDir, NULL);
    strcat(cacheDir, str);
    env->ReleaseStringUTFChars(jcacheDir, str);

    strcpy(saveDir, contentDir);

    sndInit();

    glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint*)&GAPI::defaultFBO);
    Game::init();
}

JNI_METHOD(void, nativeFree)(JNIEnv* env) {
    Game::deinit();
    sndFree();
}

JNI_METHOD(void, nativeReset)(JNIEnv* env) {
//  core->reset();
}

JNI_METHOD(void, nativeUpdate)(JNIEnv* env) {
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint*)&GAPI::defaultFBO);

    Game::update();
}

JNI_METHOD(void, nativeFrameBegin)(JNIEnv* env) {
    Game::frameBegin();
}

JNI_METHOD(void, nativeFrameEnd)(JNIEnv* env) {
    Core::setBlendMode(bmNone);
    Core::validateRenderState();

    Game::frameEnd();
    Core::reset();
}

JNI_METHOD(void, nativeFrameRender)(JNIEnv* env) {
    Game::frameRender();
    Core::reset();
}

JNI_METHOD(void, nativeResize)(JNIEnv* env, jobject obj, jint x, jint y, jint w, jint h) {
    Core::viewportDef = Viewport(x, y, w, h);
    Core::x      = x;
    Core::y      = y;
    Core::width  = w;
    Core::height = h;
}

float DeadZone(float x) {
    return x = fabsf(x) < 0.2f ? 0.0f : x;
}

InputKey keyToInputKey(int code) {
    int codes[] = {
        21, 22, 19, 20, 62, 61, 66, 111, 59, 113, 57,
        7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
        29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41,
        42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54,
    };

    for (int i = 0; i < sizeof(codes) / sizeof(codes[0]); i++)
        if (codes[i] == code)
            return (InputKey)(ikLeft + i);
    return ikNone;
}

JNI_METHOD(void, nativeTouch)(JNIEnv* env, jobject obj, jint id, jint state, jfloat x, jfloat y) {
// gamepad / keyboard
    if (state < 0) {
        switch (state) {
            case -3 : Input::setJoyPos(id, jkL, vec2(DeadZone(x), DeadZone(y))); break;
            case -4 : Input::setJoyPos(id, jkR, vec2(DeadZone(x), DeadZone(y))); break;
            default : {
                int btn = int(x);
                if (btn < 0)
                    Input::setJoyDown(id, JoyKey(jkNone - btn), state != -1);
                else
                    Input::setDown(keyToInputKey(btn), state != -1);
            }
        }
        return;
    }

// touch
    InputKey key = Input::getTouch(id);
    if (key == ikNone) return;
    Input::setPos(key, vec2(x, y));
    if (state == 1 || state == 2)
        Input::setDown(key, state == 2);
}

JNI_METHOD(void, nativeSetVR)(JNIEnv* env, jobject obj, jboolean enabled) {
    Core::Settings settings = Core::settings;
    settings.detail.stereo = enabled ? Core::Settings::STEREO_VR : Core::Settings::STEREO_OFF;
    Game::level->applySettings(settings);
}

JNI_METHOD(void, nativeSetHead)(JNIEnv* env, jobject obj, jfloatArray head) {
    jfloat *mHead = env->GetFloatArrayElements(head, NULL);
    memcpy(&Input::hmd.head, mHead, sizeof(float) * 16);
    Input::hmd.head = Input::hmd.head.inverseOrtho();
    env->ReleaseFloatArrayElements(head, mHead, 0);
}

JNI_METHOD(void, nativeSetEye)(JNIEnv* env, jobject obj, jint eye, jfloatArray proj, jfloatArray view) {
    Core::eye = float(eye);
    if (eye == 0) return;
    eye = eye == -1 ? 0 : 1;

    jfloat *mProj = env->GetFloatArrayElements(proj, NULL);
    jfloat *mView = env->GetFloatArrayElements(view, NULL);

    memcpy(&Input::hmd.proj[eye], mProj, sizeof(float) * 16);
    memcpy(&Input::hmd.eye[eye],  mView, sizeof(float) * 16);

    Input::hmd.eye[eye].setPos(Input::hmd.eye[eye].getPos() * ONE_METER);
    Input::hmd.eye[eye] = Input::hmd.eye[eye].inverseOrtho();

    env->ReleaseFloatArrayElements(proj, mProj, 0);
    env->ReleaseFloatArrayElements(view, mView, 0);
}

JNI_METHOD(void, nativeSoundState)(JNIEnv* env, jobject obj, jboolean active) {
    sndSetState(active);
}

}
