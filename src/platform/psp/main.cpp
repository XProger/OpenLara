#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <pspkernel.h>
#include <pspdisplay.h>
//#include <pspdebug.h>
#include <pspctrl.h>
#include <psprtc.h>
#include <pspaudiolib.h>
#include <pspaudio.h>

PSP_MODULE_INFO("OpenLara", 0, 1, 1);
PSP_HEAP_SIZE_KB(20480);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_VFPU);

//#define printf pspDebugScreenPrintf
//#define printf Kprintf

#include "game.h"

#define BUF_WIDTH	(512)
#define SCR_WIDTH	(480)
#define SCR_HEIGHT	(272)

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

int osStartTime = 0;
int osTimerFreq;
    
int osGetTime() {
    u64 time; 
    sceRtcGetCurrentTick(&time);
    return int(time * 1000 / osTimerFreq - osStartTime);
}

bool osSave(const char *name, const void *data, int size) {
    return false;
}

int joyGetPOV(int mask) {
    switch (mask) {
        case 0b0001 : return 1;
        case 0b1001 : return 2;
        case 0b1000 : return 3;
        case 0b1010 : return 4;
        case 0b0010 : return 5;
        case 0b0110 : return 6;
        case 0b0100 : return 7;
        case 0b0101 : return 8;
    }
    return 0;
}

void joyInit() {
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
}

void joyUpdate() {
    SceCtrlData pad;
    sceCtrlReadBufferPositive(&pad, 1);
    
    Input::setDown(ikJoyA,      (pad.Buttons & PSP_CTRL_CROSS));
    Input::setDown(ikJoyB,      (pad.Buttons & PSP_CTRL_CIRCLE));
    Input::setDown(ikJoyX,      (pad.Buttons & PSP_CTRL_SQUARE));
    Input::setDown(ikJoyY,      (pad.Buttons & PSP_CTRL_TRIANGLE));
    Input::setDown(ikJoyLB,     (pad.Buttons & PSP_CTRL_LTRIGGER));
    Input::setDown(ikJoyRB,     (pad.Buttons & PSP_CTRL_RTRIGGER));
    Input::setDown(ikJoyStart,  (pad.Buttons & PSP_CTRL_START));
    Input::setDown(ikJoySelect, (pad.Buttons & PSP_CTRL_SELECT));
    
    int pov = joyGetPOV( ((pad.Buttons & PSP_CTRL_UP)    != 0) | 
                        (((pad.Buttons & PSP_CTRL_DOWN)  != 0) << 1) | 
                        (((pad.Buttons & PSP_CTRL_LEFT)  != 0) << 2) | 
                        (((pad.Buttons & PSP_CTRL_RIGHT) != 0) << 3));
    Input::setPos(ikJoyPOV, vec2(float(pov), 0.0f));
    
    vec2 stick = vec2(float(pad.Lx), float(pad.Ly)) / 128.0f - 1.0f;
    if (fabsf(stick.x) < 0.2f && fabsf(stick.y) < 0.2f)
        stick = vec2(0.0f);
    Input::setPos(ikJoyR, stick);
}

void sndFill(void* buf, unsigned int length, void *userdata) {
    Sound::fill((Sound::Frame*)buf, length);
}

void sndInit() {
	pspAudioInit();
	pspAudioSetChannelCallback(0, sndFill, NULL);
}

char Stream::cacheDir[255];
char Stream::contentDir[255];

int main() {
    //pspDebugScreenInit();
	setupCallbacks();

	sceGuInit();

    Core::beginCmdBuf();
    
	sceGuDrawBuffer(GU_PSM_5650, (void*)0, BUF_WIDTH);
	sceGuDispBuffer(SCR_WIDTH, SCR_HEIGHT, (void*)(BUF_WIDTH * SCR_HEIGHT * 2), BUF_WIDTH);
	sceGuDepthBuffer((void*)(BUF_WIDTH * SCR_HEIGHT * 2 * 2), BUF_WIDTH);

	sceGuScissor(0, 0, SCR_WIDTH, SCR_HEIGHT);
	sceGuEnable(GU_SCISSOR_TEST);

    sndInit();
    joyInit();

    osTimerFreq = sceRtcGetTickResolution();
    osStartTime = osGetTime();

    Game::init("PSXDATA/LEVEL2.PSX");

    Core::submitCmdBuf();

	sceDisplayWaitVblankStart();
	sceGuDisplay(GU_TRUE);

    Core::curBackBuffer = 0;
    
	while (!Core::isQuit) {
        //pspDebugScreenSetOffset((int)frameOffset);
        //pspDebugScreenSetXY(0, 0);

        Core::beginCmdBuf();

        joyUpdate();
        Game::update();
        Game::render();

        Core::submitCmdBuf();
        
		//sceDisplayWaitVblankStart();
		Core::curBackBuffer = sceGuSwapBuffers();
    }

    Game::deinit();

	sceGuTerm();
	sceKernelExitGame();
    
	return 0;
}