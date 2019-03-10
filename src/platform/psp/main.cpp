#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <pspkernel.h>
#include <psppower.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <psprtc.h>
#include <pspaudiolib.h>
#include <pspaudio.h>

PSP_MODULE_INFO("OpenLara", 0, 1, 1);
PSP_HEAP_SIZE_KB(20480);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER | PSP_THREAD_ATTR_VFPU | PSP_THREAD_ATTR_NO_FILLSTACK);

#include "game.h"

#define BUF_WIDTH   (512)
#define SCR_WIDTH   (480)
#define SCR_HEIGHT  (272)

int exitCallback(int arg1, int arg2, void *common) {
    Core::quit();
    return 0;
}

int callbackThread(SceSize args, void *argp) {
    int cbid = sceKernelCreateCallback("Exit Callback", exitCallback, NULL);
    sceKernelRegisterExitCallback(cbid);
    sceKernelSleepThreadCB();
    return 0;
}

int setupCallbacks(void) {
    int thid = sceKernelCreateThread("update_thread", callbackThread, 0x11, 0xFA0, 0, 0);
    sceKernelStartThread(thid, 0, 0);
    return thid;
}


// multi-threading
void* osMutexInit() {
    SceUID *mutex = new SceUID();
    *mutex = sceKernelCreateSema(NULL, 0, 1, 1, 0);
    return mutex;
}

void osMutexFree(void *obj) {
    sceKernelDeleteSema(*(SceUID*)obj);
    delete (SceUID*)obj;
}

void osMutexLock(void *obj) {
    sceKernelWaitSema(*(SceUID*)obj, 1, NULL);
}

void osMutexUnlock(void *obj) {
    sceKernelSignalSema(*(SceUID*)obj, 1);
}

void* osRWLockInit() {
    return osMutexInit();
}

void osRWLockFree(void *obj) {
    osMutexFree(obj);
}

void osRWLockRead(void *obj) {
    osMutexLock(obj);
}

void osRWUnlockRead(void *obj) {
    osMutexUnlock(obj);
}

void osRWLockWrite(void *obj) {
    osMutexLock(obj);
}

void osRWUnlockWrite(void *obj) {
    osMutexUnlock(obj);
}

// timing
int osStartTime = 0;
int osTimerFreq;

int osGetTimeMS() {
    u64 time;
    sceRtcGetCurrentTick(&time);
    return int(time * 1000 / osTimerFreq - osStartTime);
}

// input
bool osJoyReady(int index) {
    return index == 0;
}

void osJoyVibrate(int index, float L, float R) {
    //
}

void joyInit() {
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
}

void joyUpdate() {
    SceCtrlData pad;
    sceCtrlReadBufferPositive(&pad, 1);

    Input::setJoyDown(0, jkUp,     (pad.Buttons & PSP_CTRL_UP));
    Input::setJoyDown(0, jkDown,   (pad.Buttons & PSP_CTRL_DOWN));
    Input::setJoyDown(0, jkLeft,   (pad.Buttons & PSP_CTRL_LEFT));
    Input::setJoyDown(0, jkRight,  (pad.Buttons & PSP_CTRL_RIGHT));
    Input::setJoyDown(0, jkA,      (pad.Buttons & PSP_CTRL_CROSS));
    Input::setJoyDown(0, jkB,      (pad.Buttons & PSP_CTRL_CIRCLE));
    Input::setJoyDown(0, jkX,      (pad.Buttons & PSP_CTRL_SQUARE));
    Input::setJoyDown(0, jkY,      (pad.Buttons & PSP_CTRL_TRIANGLE));
    Input::setJoyDown(0, jkLB,     (pad.Buttons & PSP_CTRL_LTRIGGER));
    Input::setJoyDown(0, jkRB,     (pad.Buttons & PSP_CTRL_RTRIGGER));
    Input::setJoyDown(0, jkStart,  (pad.Buttons & PSP_CTRL_START));
    Input::setJoyDown(0, jkSelect, (pad.Buttons & PSP_CTRL_SELECT));

    vec2 stick = vec2(float(pad.Lx), float(pad.Ly)) / 128.0f - 1.0f;
    if (fabsf(stick.x) < 0.2f && fabsf(stick.y) < 0.2f)
        stick = vec2(0.0f);
    Input::setJoyPos(0, jkL, stick);
}

void sndFill(void* buf, unsigned int length, void *userdata) {
    Sound::fill((Sound::Frame*)buf, length);
}

void sndInit() {
    pspAudioInit();
    pspAudioSetChannelCallback(0, sndFill, NULL);
}

int main() {
    cacheDir[0] = saveDir[0] = contentDir[0] = 0;

    scePowerSetClockFrequency(333, 333, 166);
    setupCallbacks();

    sceGuInit();

    GAPI::beginCmdBuf();

    sceGuDrawBuffer(GU_PSM_5650, (void*)0, BUF_WIDTH);
    sceGuDispBuffer(SCR_WIDTH, SCR_HEIGHT, (void*)(BUF_WIDTH * SCR_HEIGHT * 2), BUF_WIDTH);
    sceGuDepthBuffer((void*)(BUF_WIDTH * SCR_HEIGHT * 2 * 2), BUF_WIDTH);

    sceGuScissor(0, 0, SCR_WIDTH, SCR_HEIGHT);
    sceGuEnable(GU_SCISSOR_TEST);

    sndInit();
    joyInit();

    osTimerFreq = sceRtcGetTickResolution();
    osStartTime = Core::getTime();

    Game::init();

    GAPI::submitCmdBuf();

    sceDisplayWaitVblankStart();
    sceGuDisplay(GU_TRUE);

    GAPI::curBackBuffer = 0;

    while (!Core::isQuit) {
        GAPI::beginCmdBuf();

        joyUpdate();
        Game::update();
        Game::render();
        GAPI::submitCmdBuf();
        Core::waitVBlank();
        GAPI::curBackBuffer = sceGuSwapBuffers();
    }

    Game::deinit();

    sceGuTerm();
    sceKernelExitGame();

    return 0;
}