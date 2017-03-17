#include <jni.h>
#include <android/log.h>
//#include "vr/gvr/capi/include/gvr.h"
#include <sys/time.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

#include "game.h"

#define JNI_METHOD(return_type, method_name) \
  JNIEXPORT return_type JNICALL              \
      Java_org_xproger_openlara_Wrapper_##method_name

int getTime() {
    timeval time;
    gettimeofday(&time, NULL);
    return (time.tv_sec * 1000) + (time.tv_usec / 1000);
}

extern "C" {

int lastTime, fpsTime, fps;

char Stream::cacheDir[255];
char Stream::contentDir[255];

JNI_METHOD(void, nativeInit)(JNIEnv* env, jobject obj, jstring packName, jstring cacheDir, jint levelOffset, jint musicOffset) {
    const char* str;

    Stream::contentDir[0] = Stream::cacheDir[0] = 0;
    str = env->GetStringUTFChars(cacheDir, NULL);
    strcat(Stream::cacheDir, str);
    env->ReleaseStringUTFChars(cacheDir, str);

    str = env->GetStringUTFChars(packName, NULL);
    Stream *level = new Stream(str);
    Stream *music = new Stream(str);
    env->ReleaseStringUTFChars(packName, str);

    level->seek(levelOffset);
    music->seek(musicOffset);

    Game::init(level, music);

    lastTime    = getTime();
    fpsTime     = lastTime + 1000;
    fps         = 0;
}

JNI_METHOD(void, nativeFree)(JNIEnv* env) {
    Game::free();
}

JNI_METHOD(void, nativeReset)(JNIEnv* env) {
//  core->reset();
}

JNI_METHOD(void, nativeUpdate)(JNIEnv* env) {
    int time = getTime();
    if (time == lastTime)
        return;
    Game::update((time - lastTime) * 0.001f);
    lastTime = time;
}

JNI_METHOD(void, nativeRender)(JNIEnv* env) {
    Core::stats.dips = 0;
    Core::stats.tris = 0;
    Game::render();
    if (fpsTime < getTime()) {
        LOG("FPS: %d DIP: %d TRI: %d\n", fps, Core::stats.dips, Core::stats.tris);
        fps = 0;
        fpsTime = getTime() + 1000;
    } else
        fps++;
}

JNI_METHOD(void, nativeResize)(JNIEnv* env, jobject obj, jint w, jint h) {
    Core::width  = w;
    Core::height = h;
}

float DeadZone(float x) {
    return x = fabsf(x) < 0.2f ? 0.0f : x;
}

int getPOV(int x, int y) {
    switch (x) {
        case -1 : {
            if (y == -1) return 8;
            if (y ==  0) return 7;
            if (y == +1) return 6;
        }
        case 0 : {
            if (y == -1) return 1;
            if (y ==  0) return 0;
            if (y == +1) return 5;
        }
        case +1 : {
            if (y == -1) return 2;
            if (y ==  0) return 3;
            if (y == +1) return 4;
        }
    }
    return 0;
}

JNI_METHOD(void, nativeTouch)(JNIEnv* env, jobject obj, jint id, jint state, jfloat x, jfloat y) {
    if (id > 1) return;
// gamepad
    if (state < 0) {
        switch (state) {
            case -3 : Input::setPos(ikJoyL, vec2(DeadZone(x), DeadZone(y))); break;
            case -4 : Input::setPos(ikJoyR, vec2(DeadZone(x), DeadZone(y))); break;
            case -5 : Input::setPos(ikJoyPOV, vec2(float(getPOV(sign(x), sign(y))), 0.0f)); break;
            default : Input::setDown(InputKey(ikJoyA + (int)x), state != -1);
        }
        return;
    }

    if (state == 3 && x < Core::width / 2) {
        vec2 center(Core::width * 0.25f, Core::height * 0.6f);
        vec2 pos(x, y);
        vec2 d = pos - center;
        Input::setPos(ikJoyL, d.normal());
    };

    if (state == 2 && x > Core::width / 2) {
        int i = y / (Core::height / 3);
        InputKey key;
        if (i == 0)
            key = ikJoyX;
        else if (i == 1)
            key = ikJoyA;
        else
            key = ikJoyY;
        Input::setDown(key, true);
    }

    if (state == 1) {
        Input::setPos(ikJoyL, vec2(0.0f));
        Input::setDown(ikJoyA, false);
        Input::setDown(ikJoyX, false);
        Input::setDown(ikJoyY, false);
    }
}

JNI_METHOD(void, nativeSoundFill)(JNIEnv* env, jobject obj, jshortArray buffer) {
    jshort *frames = env->GetShortArrayElements(buffer, NULL);
    jsize count = env->GetArrayLength(buffer) / 2;
    Sound::fill((Sound::Frame*)frames, count);
    env->ReleaseShortArrayElements(buffer, frames, 0);
}

}


