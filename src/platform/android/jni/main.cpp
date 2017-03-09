#include <jni.h>
#include <android/log.h>
#include <sys/time.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

#include "game.h"

int getTime() {
    timeval time;
    gettimeofday(&time, NULL);
    return (time.tv_sec * 1000) + (time.tv_usec / 1000);
}

extern "C" {

int lastTime, fpsTime, fps;

char Stream::cacheDir[255];
char Stream::contentDir[255];

JNIEXPORT void JNICALL Java_org_xproger_openlara_Wrapper_nativeInit(JNIEnv* env, jobject obj, jstring packName, jstring cacheDir, jint levelOffset, jint musicOffset) {
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

JNIEXPORT void JNICALL Java_org_xproger_openlara_Wrapper_nativeFree(JNIEnv* env) {
    Game::free();
}

JNIEXPORT void JNICALL Java_org_xproger_openlara_Wrapper_nativeReset(JNIEnv* env) {
//  core->reset();
}

JNIEXPORT void JNICALL Java_org_xproger_openlara_Wrapper_nativeUpdate(JNIEnv* env) {
    int time = getTime();
    if (time == lastTime)
        return;

    float delta = min(1.0f, (time - lastTime) * 0.001f);
    while (delta > EPS) {
        Core::deltaTime = min(delta, 1.0f / 30.0f);
        Game::update();
        delta -= Core::deltaTime;
    }
    lastTime = time;
}

JNIEXPORT void JNICALL Java_org_xproger_openlara_Wrapper_nativeRender(JNIEnv* env) {
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

JNIEXPORT void JNICALL Java_org_xproger_openlara_Wrapper_nativeResize(JNIEnv* env, jobject obj, jint w, jint h) {
    Core::width  = w;
    Core::height = h;
}

float DeadZone(float x) {
    return x = fabsf(x) < 0.2f ? 0.0f : x;
}

JNIEXPORT void JNICALL Java_org_xproger_openlara_Wrapper_nativeTouch(JNIEnv* env, jobject obj, jint id, jint state, jfloat x, jfloat y) {
    if (id > 1) return;
/* gamepad
    if (state < 0) {
        state = -state;
        Input::Joystick &joy = Input::joy;

        switch (state) {
            case 3 :
                joy.L.x = DeadZone(x);
                joy.L.y = DeadZone(y);
                break;
            case 4 :
                joy.R.x = DeadZone(x);
                joy.R.y = DeadZone(y);
                break;
            default:
                joy.down[(int)x] = state != 1;
        }
        return;
    }
*/
    if (state == 3 && x < Core::width / 2) {
        vec2 center(Core::width * 0.25f, Core::height * 0.6f);
        vec2 pos(x, y);
        vec2 d = pos - center;

        float angle = atan2(d.x, -d.y) + PI * 0.125f;
        if (angle < 0.0f) angle += PI2;
        int pov = int(angle / (PI * 0.25f));

        Input::setPos(ikJoyPOV, vec2(float(1 + pov), 0.0f));
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
        Input::setPos(ikJoyPOV, vec2(0.0f));
        Input::setDown(ikJoyA, false);
        Input::setDown(ikJoyX, false);
        Input::setDown(ikJoyY, false);
    }
}

JNIEXPORT void JNICALL Java_org_xproger_openlara_Wrapper_nativeSoundFill(JNIEnv* env, jobject obj, jshortArray buffer) {
    jshort *frames = env->GetShortArrayElements(buffer, NULL);
    jsize count = env->GetArrayLength(buffer) / 2;
    Sound::fill((Sound::Frame*)frames, count);
    env->ReleaseShortArrayElements(buffer, frames, 0);
}

}


