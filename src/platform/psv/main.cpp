#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "debugScreen.h"

#include <psp2/rtc.h>
#include <psp2/ctrl.h>
#include <psp2/audioout.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/power.h>
#include <psp2/touch.h>
#include <malloc.h>

#include "game.h"


// multi-threading
void* osMutexInit() {
    SceUID *mutex = new SceUID();
    *mutex = sceKernelCreateMutex(NULL, 0, 0, NULL);
}

void osMutexFree(void *obj) {
    sceKernelDeleteMutex(*(SceUID*)obj);
    delete (SceUID*)obj;
}

void osMutexLock(void *obj) {
    sceKernelLockMutex(*(SceUID*)obj, 1, NULL);
}

void osMutexUnlock(void *obj) {
    sceKernelUnlockMutex(*(SceUID*)obj, 1);
}

// timing
int osStartTime = 0;
int osTimerFreq;

int osGetTimeMS() {
    SceRtcTick current;
    sceRtcGetCurrentTick(&current);
    return int(current.tick * 1000 / osTimerFreq - osStartTime);
}

// input
bool osJoyReady(int index) {
    return index == 0;
}

void osJoyVibrate(int index, float L, float R) {
    //
}

void inputInit() {
    sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
    sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, SCE_TOUCH_SAMPLING_STATE_START);
    sceTouchEnableTouchForce(SCE_TOUCH_PORT_FRONT);
}

void inputUpdate() {
// gamepad
    SceCtrlData pad;
    sceCtrlReadBufferPositive(0, &pad, 1);

    Input::setJoyDown(0, jkUp,     (pad.buttons & SCE_CTRL_UP));
    Input::setJoyDown(0, jkDown,   (pad.buttons & SCE_CTRL_DOWN));
    Input::setJoyDown(0, jkLeft,   (pad.buttons & SCE_CTRL_LEFT));
    Input::setJoyDown(0, jkRight,  (pad.buttons & SCE_CTRL_RIGHT));
    Input::setJoyDown(0, jkA,      (pad.buttons & SCE_CTRL_CROSS));
    Input::setJoyDown(0, jkB,      (pad.buttons & SCE_CTRL_CIRCLE));
    Input::setJoyDown(0, jkX,      (pad.buttons & SCE_CTRL_SQUARE));
    Input::setJoyDown(0, jkY,      (pad.buttons & SCE_CTRL_TRIANGLE));
    Input::setJoyDown(0, jkLB,     (pad.buttons & SCE_CTRL_LTRIGGER));
    Input::setJoyDown(0, jkRB,     (pad.buttons & SCE_CTRL_RTRIGGER));
    Input::setJoyDown(0, jkStart,  (pad.buttons & SCE_CTRL_START));
    Input::setJoyDown(0, jkSelect, (pad.buttons & SCE_CTRL_SELECT));

    vec2 stickL = vec2(float(pad.lx), float(pad.ly)) / 128.0f - 1.0f;
    vec2 stickR = vec2(float(pad.rx), float(pad.ry)) / 128.0f - 1.0f;
    if (fabsf(stickL.x) < 0.3f && fabsf(stickL.y) < 0.3f) stickL = vec2(0.0f);
    if (fabsf(stickR.x) < 0.3f && fabsf(stickR.y) < 0.3f) stickR = vec2(0.0f);
    Input::setJoyPos(0, jkL, stickL);
    Input::setJoyPos(0, jkR, stickR);

// touch
    SceTouchData touch;
    sceTouchPeek(SCE_TOUCH_PORT_FRONT, &touch, 1);

    bool touchState[COUNT(Input::touch)];
    for (int i = 0; i < COUNT(Input::touch); i++)
        touchState[i] = Input::down[ikTouchA + i];

    for (int i = 0; i < touch.reportNum; i++) {
        SceTouchReport &t = touch.report[i];

        InputKey key = Input::getTouch(t.id);
        if (key == ikNone) continue;

        Input::setPos(key, vec2(t.x * (DISPLAY_WIDTH / 1920.0f), t.y * (DISPLAY_HEIGHT / 1080.0f)));
        Input::setDown(key, true);

        touchState[key - ikTouchA] = false;
    }

    for (int i = 0; i < COUNT(Input::touch); i++)
        if (touchState[i])
            Input::setDown(InputKey(ikTouchA + i), false);
}

bool sndTerm;
int sndPort;
int sndPartIndex;
SceUID sndSema;
SceUID sndPrepTID, sndOutTID;
Sound::Frame *sndBuffer;

#define SND_FRAMES 2048

int sndPrepThread(SceSize args, void *argp) {
    while (!sndTerm) {
        sceKernelWaitSema(sndSema, 1, NULL);
        sndPartIndex ^= 1;
        Sound::Frame *part = sndBuffer + SND_FRAMES * sndPartIndex;
        Sound::fill(part, SND_FRAMES);
    }
    return 0;
}

int sndOutThread(SceSize args, void *argp) {
    while (!sndTerm) {
        Sound::Frame *part = sndBuffer + SND_FRAMES * sndPartIndex;
        sceKernelSignalSema(sndSema, 1);
        sceAudioOutOutput(sndPort, part);
    }
    return 0;
}

void sndInit() {
    sndTerm = false;
    sndPartIndex = 0;

    sndBuffer = (Sound::Frame*)memalign(64, sizeof(Sound::Frame) * SND_FRAMES * 2);

    sndPort = sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_BGM, SND_FRAMES, 44100, SCE_AUDIO_OUT_MODE_STEREO); // SCE_AUDIO_OUT_PARAM_FORMAT_S16_STEREO
    int volume[] = { SCE_AUDIO_VOLUME_0DB, SCE_AUDIO_VOLUME_0DB };
    sceAudioOutSetVolume(sndPort, SceAudioOutChannelFlag(SCE_AUDIO_VOLUME_FLAG_L_CH | SCE_AUDIO_VOLUME_FLAG_R_CH), volume);

    Sound::fill(sndBuffer, SND_FRAMES); // fill first part

    sndSema = sceKernelCreateSema("SoundSema", 0, 0, 1, NULL);

    sndPrepTID = sceKernelCreateThread("SoundPrep", sndPrepThread, 0x10000100, 0x10000, 0, 0x00070000, NULL);
    sndOutTID  = sceKernelCreateThread("SoundOut",  sndOutThread,  0x10000100, 0x10000, 0, 0x00070000, NULL);
    sceKernelStartThread(sndPrepTID, 0, NULL);
    sceKernelStartThread(sndOutTID,  0, NULL);
}

void sndFree() {
    sndTerm = true;

    sceKernelSignalSema(sndSema, 1);
    sceKernelWaitThreadEnd(sndPrepTID, NULL, NULL);
    sceKernelWaitThreadEnd(sndOutTID, NULL, NULL);
    sceKernelDeleteThread(sndPrepTID);
    sceKernelDeleteThread(sndOutTID);
    sceKernelDeleteSema(sndSema);

    sceAudioOutReleasePort(sndPort);

    free(sndBuffer);
}


int main() {
    psvDebugScreenInit();

    scePowerSetArmClockFrequency(444);
    scePowerSetBusClockFrequency(222);
    scePowerSetGpuClockFrequency(222);
    scePowerSetGpuXbarClockFrequency(166);

    cacheDir[0] = saveDir[0] = contentDir[0] = 0;
    strcpy(cacheDir,    "ux0:data/OpenLara/");
    strcpy(saveDir,     "ux0:data/OpenLara/");
    strcpy(contentDir,  "ux0:data/OpenLara/");

    sndInit();
    inputInit();

    osTimerFreq = sceRtcGetTickResolution();
    osStartTime = Core::getTime();

    Game::init("PSXDATA/LEVEL2.PSX");
//    sceRazorGpuCaptureSetTrigger(100, "ux0:data/OpenLara/capture.sgx");

    while (!Core::isQuit) {
        inputUpdate();

        if (Input::joy[0].down[jkStart])
            Core::quit();

        Game::update();
        Game::render();

        GAPI::present();
    }

    sndFree();
    Game::deinit();

    return 0;
}