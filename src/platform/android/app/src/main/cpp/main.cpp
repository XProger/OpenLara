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

InputKey keyToInputKey(int code) {
    int codes[] = {
        21, 22, 19, 20, 62, 66, 111, 59, 113, 57,
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
            case -3 : Input::setPos(ikJoyL, vec2(DeadZone(x), DeadZone(y))); break;
            case -4 : Input::setPos(ikJoyR, vec2(DeadZone(x), DeadZone(y))); break;
            case -5 : Input::setPos(ikJoyPOV, vec2(float(getPOV(sign(x), sign(y))), 0.0f)); break;
            default : {
                int btn = int(x);
                InputKey key = btn <= 0 ? InputKey(ikJoyA - btn) : keyToInputKey(btn);
                Input::setDown(key, state != -1);
            }
        }
        return;
    }

    if (id == -100) {
    	switch (state) {
			case 0 : Input::head.basis.rot.x = x; Input::head.basis.rot.y = y; break;
			case 1 : Input::head.basis.rot.z = x; Input::head.basis.rot.w = y; Input::head.set(); break;
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

JNI_METHOD(void, nativeSoundFill)(JNIEnv* env, jobject obj, jshortArray buffer) {
    jshort *frames = env->GetShortArrayElements(buffer, NULL);
    jsize count = env->GetArrayLength(buffer) / 2;
    Sound::fill((Sound::Frame*)frames, count);
    env->ReleaseShortArrayElements(buffer, frames, 0);
}

}