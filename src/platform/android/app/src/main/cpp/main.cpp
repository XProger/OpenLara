#include <jni.h>
#include <android/log.h>
#include <sys/time.h>
#include <stdio.h>
#include <math.h>
#include <pthread.h>

#include "game.h"

#define JNI_METHOD(return_type, method_name) \
  JNIEXPORT return_type JNICALL              \
      Java_org_xproger_openlara_Wrapper_##method_name

// multi-threading
void* osMutexInit() {
    pthread_mutex_t *mutex = new pthread_mutex_t();
    pthread_mutex_init(mutex, NULL);
    return mutex;
}

void osMutexFree(void *obj) {
    pthread_mutex_destroy((pthread_mutex_t*)obj);
    delete (pthread_mutex_t*)obj;
}

void osMutexLock(void *obj) {
    pthread_mutex_lock((pthread_mutex_t*)obj);
}

void osMutexUnlock(void *obj) {
    pthread_mutex_unlock((pthread_mutex_t*)obj);
}

void* osRWLockInit() {
    pthread_rwlock_t *lock = new pthread_rwlock_t();
    pthread_rwlock_init(lock, NULL);
    return lock;
}

void osRWLockFree(void *obj) {
    pthread_rwlock_destroy((pthread_rwlock_t*)obj);
    delete (pthread_rwlock_t*)obj;
}

void osRWLockRead(void *obj) {
    pthread_rwlock_rdlock((pthread_rwlock_t*)obj);
}

void osRWUnlockRead(void *obj) {
    pthread_rwlock_unlock((pthread_rwlock_t*)obj);
}

void osRWLockWrite(void *obj) {
    pthread_rwlock_wrlock((pthread_rwlock_t*)obj);
}

void osRWUnlockWrite(void *obj) {
    pthread_rwlock_unlock((pthread_rwlock_t*)obj);
}


// timing
time_t startTime;

int osGetTime() {
    timeval t;
    gettimeofday(&t, NULL);
    return int((t.tv_sec - startTime) * 1000 + t.tv_usec / 1000);
}

bool osSave(const char *name, const void *data, int size) {
    return false;
}

extern "C" {

char Stream::cacheDir[255];
char Stream::contentDir[255];

JNI_METHOD(void, nativeInit)(JNIEnv* env, jobject obj, jstring contentDir, jstring cacheDir, jstring packName, jint levelOffset) {
    timeval t;
    gettimeofday(&t, NULL);
    startTime = t.tv_sec;	

    const char* str;

    Stream::contentDir[0] = Stream::cacheDir[0] = 0;

    str = env->GetStringUTFChars(packName, NULL);
/*
    Stream *level = new Stream(str);
    env->ReleaseStringUTFChars(packName, str);
    level->seek(levelOffset);
*/
    str = env->GetStringUTFChars(contentDir, NULL);
    strcat(Stream::contentDir, str);
    env->ReleaseStringUTFChars(contentDir, str);

    str = env->GetStringUTFChars(cacheDir, NULL);
    strcat(Stream::cacheDir, str);
    env->ReleaseStringUTFChars(cacheDir, str);

    Game::init();
}

JNI_METHOD(void, nativeFree)(JNIEnv* env) {
    Game::deinit();
}

JNI_METHOD(void, nativeReset)(JNIEnv* env) {
//  core->reset();
}

JNI_METHOD(void, nativeUpdate)(JNIEnv* env) {
    Game::update();
}

JNI_METHOD(void, nativeRender)(JNIEnv* env) {
    Core::stats.dips = 0;
    Core::stats.tris = 0;
    Game::render();
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
            case -3 : Input::setJoyPos(0, jkL, vec2(DeadZone(x), DeadZone(y))); break;
            case -4 : Input::setJoyPos(0, jkR, vec2(DeadZone(x), DeadZone(y))); break;
            //case -5 : Input::setJoyPos(ikJoyPOV, vec2(float(getPOV(sign(x), sign(y))), 0.0f)); break;
            default : {
                int btn = int(x);
                if (btn <= 0)
                    Input::setJoyDown(0, JoyKey(jkA - btn), state != -1);
                else
                    Input::setDown(keyToInputKey(btn), state != -1);
            }
        }
        return;
    }

    if (id == -100) {
        /*
        switch (state) {
            case 0 : Input::head.basis.rot.x = x; Input::head.basis.rot.y = y; break;
            case 1 : Input::head.basis.rot.z = x; Input::head.basis.rot.w = y; Input::head.set(); break;
        }
        */
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