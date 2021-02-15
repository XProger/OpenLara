#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "game.h"

// multi-threading
void* osMutexInit() {
    Handle *mutex = new Handle();
    svcCreateMutex(mutex, false);
    return mutex;
}

void osMutexFree(void *obj) {
    svcCloseHandle(*(Handle*)obj);
    delete (Handle*)obj;
}

void osMutexLock(void *obj) {
    svcWaitSynchronization(*(Handle*)obj, U64_MAX);
}

void osMutexUnlock(void *obj) {
    svcReleaseMutex(*(Handle*)obj);
}

// timing
u64 osStartTime = 0;

int osGetTimeMS() {
    return int(osGetTime() - osStartTime);
}

// backlight
bool bottomScreenOn = false;

void setBottomScreen(bool enable) {
    gspLcdInit();
    enable ? GSPLCD_PowerOnBacklight(GSPLCD_SCREEN_BOTTOM) : GSPLCD_PowerOffBacklight(GSPLCD_SCREEN_BOTTOM);
    gspLcdExit();
}

aptHookCookie(cookie);

void checkAptHook(APT_HookType hook, void *param) {
    if (!bottomScreenOn) {
        switch(hook) {
            case APTHOOK_ONSUSPEND : setBottomScreen(true);
                break;
            case APTHOOK_ONRESTORE :
            case APTHOOK_ONWAKEUP  : setBottomScreen(false);
                break;
            default:
                break;
        }
    }
}

// input
bool osJoyReady(int index) {
    return index == 0;
}

void osJoyVibrate(int index, float L, float R) {
    //
}

void inputInit() {
    aptHook(&cookie, checkAptHook, NULL);
    hidInit();
}

void inputUpdate() {
    const static u64 keys[jkMAX] = { 0,
        KEY_B, KEY_A, KEY_Y, KEY_X, KEY_L, KEY_R, KEY_SELECT, KEY_START, 
        0, 0, KEY_ZL, KEY_ZR,
        KEY_DLEFT, KEY_DRIGHT, KEY_DUP, KEY_DDOWN,
    };

    hidScanInput();

	u64 down = hidKeysDown();
    u64 mask = down | hidKeysHeld();

    for (int i = 1; i < jkMAX; i++) {
        Input::setJoyDown(0, JoyKey(jkNone + i), (mask & keys[i]) != 0);
    }

    circlePosition circlePos;
    hidCircleRead(&circlePos);

    vec2 stickL = vec2(float(circlePos.dx), float(-circlePos.dy)) / 160.0f;

    if (fabsf(stickL.x) < 0.3f && fabsf(stickL.y) < 0.3f) stickL = vec2(0.0f);
    Input::setJoyPos(0, jkL, stickL);

    if (down & KEY_TOUCH) {
        bottomScreenOn = !bottomScreenOn;
        bottomScreenOn ? setBottomScreen(true) : setBottomScreen(false);
    }
}

void inputFree() {
    if (!bottomScreenOn)
        setBottomScreen(true);
    hidExit();
}

// sound
#define SND_FRAMES (4704/2)

ndspWaveBuf   sndWaveBuf[2];
Sound::Frame *sndBuffer;
Thread        sndThread;
int           sndBufIndex;
bool          sndReady;

void sndFill(void *arg) {
    memset(sndWaveBuf, 0, sizeof(sndWaveBuf));
    sndWaveBuf[0].data_vaddr = sndBuffer + 0;
    sndWaveBuf[0].nsamples   = SND_FRAMES;
    sndWaveBuf[1].data_vaddr = sndBuffer + SND_FRAMES;
    sndWaveBuf[1].nsamples   = SND_FRAMES;

    Sound::fill(sndBuffer, SND_FRAMES * 2);

    sndBufIndex = 0;
    ndspChnWaveBufAdd(0, sndWaveBuf + 0);
    ndspChnWaveBufAdd(0, sndWaveBuf + 1);

    while (sndReady) {
        ndspWaveBuf &buf = sndWaveBuf[sndBufIndex];

        if (buf.status == NDSP_WBUF_DONE) {
            Sound::fill((Sound::Frame*)buf.data_pcm16, buf.nsamples);
            DSP_FlushDataCache(buf.data_pcm16, buf.nsamples);
            ndspChnWaveBufAdd(0, &buf);
            sndBufIndex = !sndBufIndex;
        }

        svcSleepThread(1000000ULL);
    }
}

void sndInit() {
    sndBuffer = (Sound::Frame*)linearAlloc(SND_FRAMES * sizeof(Sound::Frame) * 2);

    ndspInit();
    ndspSetOutputMode (NDSP_OUTPUT_STEREO);
    ndspChnSetFormat  (0, NDSP_FORMAT_STEREO_PCM16);
    ndspChnSetInterp  (0, NDSP_INTERP_LINEAR);
    ndspChnSetRate    (0, 44100);

    float mix[12];
    memset(mix, 0, sizeof(mix));
    mix[0] = 1.0;
    mix[1] = 1.0;
    ndspChnSetMix(0, mix);

    sndReady  = true;

    s32 priority = 0;
    svcGetThreadPriority(&priority, CUR_THREAD_HANDLE);

    sndThread = threadCreate(sndFill, NULL, 64 * 1024, priority - 1, -1, false);
}

void sndFree() {
    sndReady = false;
    threadJoin(sndThread, U64_MAX);
    threadFree(sndThread);

    ndspExit();
    linearFree((uint32*)sndBuffer);
}

int checkLanguage()
{
    uint8 id;
    CFGU_GetSystemLanguage(&id);

    int str = STR_LANG_EN;
    switch (id)
    {
        case CFG_LANGUAGE_EN : str = STR_LANG_EN; break;
        case CFG_LANGUAGE_FR : str = STR_LANG_FR; break;
        case CFG_LANGUAGE_DE : str = STR_LANG_DE; break;
        case CFG_LANGUAGE_ES : str = STR_LANG_ES; break;
        case CFG_LANGUAGE_IT : str = STR_LANG_IT; break;
        case CFG_LANGUAGE_PT : str = STR_LANG_PT; break;
        case CFG_LANGUAGE_RU : str = STR_LANG_RU; break;
        case CFG_LANGUAGE_JP : str = STR_LANG_JA; break;
        case CFG_LANGUAGE_ZH : str = STR_LANG_CN; break;
    }
    return str - STR_LANG_EN;
}

int main() {
    cfguInit();
    
    setBottomScreen(false);

    {
        bool isNew3DS;
        APT_CheckNew3DS(&isNew3DS);
        if (isNew3DS) {
            osSetSpeedupEnable(true);
        }
    }

    strcpy(cacheDir,   "sdmc:/3ds/OpenLara/");
    strcpy(saveDir,    "sdmc:/3ds/OpenLara/");
    strcpy(contentDir, "sdmc:/3ds/OpenLara/");

    Stream::init();
    
    Core::defLang = checkLanguage();

    sndInit();
    inputInit();

    osStartTime = Core::getTime();

    Game::init();

    if (Core::isQuit) {
        bottomScreenOn = true;
        setBottomScreen(true);
        consoleClear();
        LOG("\n\nCopy the original game content to:\n\n    %s\n\nPress A to exit", contentDir);
        while (aptMainLoop()) {
            hidScanInput();
            u64 mask = hidKeysDown() | hidKeysHeld();
            if (mask & KEY_A) {
                break;
            }

            gfxFlushBuffers();
            gfxSwapBuffers();
            gspWaitForVBlank();
        }
    } else {
        while (aptMainLoop() && !Core::isQuit) {
            inputUpdate();

            Game::update();
            Game::render();

            GAPI::present();
        }
    }

    inputFree();
    sndFree();
    Game::deinit();

    return 0;
}