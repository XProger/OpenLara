#include "game.h"

LPDIRECT3D8           D3D;
LPDIRECT3DDEVICE8     device;
D3DPRESENT_PARAMETERS d3dpp;

// multi-threading
void* osMutexInit() {
    CRITICAL_SECTION *CS = new CRITICAL_SECTION();
    InitializeCriticalSection(CS);
    return CS;
}

void osMutexFree(void *obj) {
    DeleteCriticalSection((CRITICAL_SECTION*)obj);
    delete (CRITICAL_SECTION*)obj;
}

void osMutexLock(void *obj) {
    EnterCriticalSection((CRITICAL_SECTION*)obj);
}

void osMutexUnlock(void *obj) {
    LeaveCriticalSection((CRITICAL_SECTION*)obj);
}

// timing
LARGE_INTEGER timerFreq;
LARGE_INTEGER timerStart;

int osGetTimeMS() {
    LARGE_INTEGER time;
    QueryPerformanceCounter(&time);
    return int32((time.QuadPart - timerStart.QuadPart) * 1000L / timerFreq.QuadPart);
}


// input
struct JoyDevice {
    HANDLE            handle;
    int               time;
    float             vL, vR;
    float             oL, oR;
    XINPUT_FEEDBACK   feedback;
} joyDevice[INPUT_JOY_COUNT];

#define JOY_MIN_UPDATE_FX_TIME  50
#define JOY_TRIGGER_THRESHOLD   30

void osJoyVibrate(int index, float L, float R) {
    joyDevice[index].vL = L;
    joyDevice[index].vR = R;
}

void joyInit() {
    XInitDevices(0, NULL);
}

float joyTrigger(int32 value) {
    return clamp(float(value - JOY_TRIGGER_THRESHOLD) / (255 - JOY_TRIGGER_THRESHOLD), 0.0f, 1.0f);
}

void joyUpdate() {
    static XINPUT_POLLING_PARAMETERS params = { TRUE, TRUE, 0, 8, 8, 0, };

    DWORD dwInsertions, dwRemovals;
    XGetDeviceChanges(XDEVICE_TYPE_GAMEPAD, &dwInsertions, &dwRemovals);

    for (DWORD i = 0; i < XGetPortCount(); i++) {
        JoyDevice &joy = joyDevice[i];

        if (dwInsertions & (1 << i)) {
            memset(&joy, 0, sizeof(joy));
            joy.handle = XInputOpen(XDEVICE_TYPE_GAMEPAD, i, XDEVICE_NO_SLOT, &params);
            LOG("joy insert: %d\n", i);
        }

        if (dwRemovals & (1 << i)) {
            XInputClose(joy.handle);
            joy.handle = NULL;
            LOG("joy remove: %d\n", i);
        }

        if (joy.handle) {
            XINPUT_STATE state;
            XInputGetState(joy.handle, &state);

            Input::setJoyPos(i, jkL, vec2(state.Gamepad.sThumbLX + 0.5f, -(state.Gamepad.sThumbLY + 0.5f)) / 32767.5f);
            Input::setJoyPos(i, jkR, vec2(state.Gamepad.sThumbRX + 0.5f, -(state.Gamepad.sThumbRY + 0.5f)) / 32767.5f);
            Input::setJoyPos(i, jkLT, vec2(state.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_WHITE], 0.0f));
            Input::setJoyPos(i, jkRT, vec2(state.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_BLACK], 0.0f));

            Input::setJoyDown(i, jkA,      state.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_A] > 0);
            Input::setJoyDown(i, jkB,      state.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_B] > 0);
            Input::setJoyDown(i, jkX,      state.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_X] > 0);
            Input::setJoyDown(i, jkY,      state.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_Y] > 0);
            Input::setJoyDown(i, jkLB,     joyTrigger(state.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_LEFT_TRIGGER]) > 0);
            Input::setJoyDown(i, jkRB,     joyTrigger(state.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_RIGHT_TRIGGER]) > 0);
            Input::setJoyDown(i, jkUp,     (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) > 0);
            Input::setJoyDown(i, jkDown,   (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) > 0);
            Input::setJoyDown(i, jkLeft,   (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) > 0);
            Input::setJoyDown(i, jkRight,  (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) > 0);
            Input::setJoyDown(i, jkStart,  (state.Gamepad.wButtons & XINPUT_GAMEPAD_START) > 0);
            Input::setJoyDown(i, jkSelect, (state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK) > 0);
            Input::setJoyDown(i, jkL,      (state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) > 0);
            Input::setJoyDown(i, jkR,      (state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) > 0);

            if ((joy.vL != joy.oL || joy.vR != joy.oR) && Core::getTime() >= joy.time) {
                if (joy.feedback.Header.dwStatus != ERROR_IO_PENDING) {
                    joy.feedback.Rumble.wLeftMotorSpeed  = WORD(joy.vL * 65535.0f);
                    joy.feedback.Rumble.wRightMotorSpeed = WORD(joy.vR * 65535.0f);
                    joy.oL = joy.vL;
                    joy.oR = joy.vR;
                    joy.time = Core::getTime() + JOY_MIN_UPDATE_FX_TIME;
                    XInputSetState(joy.handle, &joy.feedback);
                }
            }

        }
    }
}


// sound
#define SND_PACKETS 2
#define SND_SAMPLES 2352
#define SND_SIZE    (SND_SAMPLES * sizeof(int16) * 2)

LPDIRECTSOUND       DSound;
LPDIRECTSOUNDSTREAM sndStream;
uint8*              sndBuffer;

void CALLBACK sndFill(VOID* pStreamContext, VOID* pPacketContext, DWORD dwStatus) {
    if (Core::isQuit) return;

    if (dwStatus != XMEDIAPACKET_STATUS_SUCCESS)
        return;

    int32 index = (int32)pPacketContext;

    XMEDIAPACKET packet = {0};
    packet.dwMaxSize = SND_SIZE;
    packet.pvBuffer  = sndBuffer + index * SND_SIZE;
    packet.pContext  = pPacketContext;

    Sound::fill((Sound::Frame*)packet.pvBuffer, SND_SIZE / 4);

    sndStream->Process(&packet, NULL);
}

void sndInit() {
    if (FAILED(DirectSoundCreate(NULL, &DSound, NULL)))
        return;

    WAVEFORMATEX waveFmt = { WAVE_FORMAT_PCM, 2, 44100, 44100 * 4, 4, 16, sizeof(waveFmt) };

    DSSTREAMDESC dssd = {0};
    dssd.dwMaxAttachedPackets = SND_PACKETS;
    dssd.lpwfxFormat          = &waveFmt;
    dssd.lpfnCallback         = sndFill;

    sndBuffer = (uint8*)XPhysicalAlloc(SND_SIZE * SND_PACKETS, MAXULONG_PTR, 0, PAGE_READWRITE | PAGE_NOCACHE);
    if (!sndBuffer)
        return;
    memset(sndBuffer, 0, SND_SIZE * SND_PACKETS);

    if (FAILED(DirectSoundCreateStream(&dssd, &sndStream)))
        return;

    XMEDIAPACKET packet = {0};
    packet.dwMaxSize = SND_SIZE;
    for (int i = 0; i < SND_PACKETS; i++) {
        packet.pvBuffer  = sndBuffer + i * SND_SIZE;
        packet.pContext  = (VOID*)i;
        sndStream->Process(&packet, NULL);
    }
}

void sndFree() {
    if (sndStream) {
        sndStream->Pause(DSSTREAMPAUSE_PAUSE);
        sndStream->Discontinuity();
        sndStream->Flush();
        sndStream->Release();
    }
    if (sndBuffer) {
        XPhysicalFree(sndBuffer);
    }
}

void sndUpdate() {
    DirectSoundDoWork();
}


// context
struct {
    int  width;
    int  height;
    bool prog;
    bool wide;

    bool support(DWORD videoFlags) {
        if (wide && !(videoFlags & XC_VIDEO_FLAGS_WIDESCREEN))
            return false;
        if ((height == 480) && wide && !(videoFlags & XC_VIDEO_FLAGS_WIDESCREEN))
            return false;
        if ((height == 480) && prog && !(videoFlags & XC_VIDEO_FLAGS_HDTV_480p))
            return false;
        if ((height == 720) && !(videoFlags & XC_VIDEO_FLAGS_HDTV_720p))
            return false;
        if ((height == 1080) && !(videoFlags & XC_VIDEO_FLAGS_HDTV_1080i))
            return false;
        return true;
    }
} displayModes[] = {
    {  1920,   1080,    false,  true  },    // 1920x1080 interlaced 16x9
    {  1280,    720,    true,   true  },    // 1280x720 progressive 16x9
    {   720,    480,    true,   true  },    // 720x480 progressive 16x9
    {   720,    480,    false,  true  },    // 720x480 interlaced 16x9
    {   640,    480,    true,   true  },    // 640x480 progressive 16x9
    {   640,    480,    false,  true  },    // 640x480 interlaced 16x9
    {   720,    480,    true,   false },    // 720x480 progressive 4x3
    {   720,    480,    false,  false },    // 720x480 interlaced 4x3
    {   640,    480,    true,   false },    // 640x480 progressive 4x3
    {   640,    480,    false,  false },    // 640x480 interlaced 4x3
};

HRESULT ContextCreate() {
    D3D = Direct3DCreate8(D3D_SDK_VERSION);

    if (!D3D) return E_FAIL;

    memset(&d3dpp, 0, sizeof(d3dpp));
    d3dpp.BackBufferWidth  = 640;
    d3dpp.BackBufferHeight = 480;

    DWORD videoFlags = XGetVideoFlags();

    for (int i = 0; i < COUNT(displayModes); i++) {
        if (displayModes[i].support(videoFlags)) {
            d3dpp.BackBufferWidth  = displayModes[i].width;
            d3dpp.BackBufferHeight = displayModes[i].height;
            if (displayModes[i].wide) {
                d3dpp.Flags |= D3DPRESENTFLAG_WIDESCREEN;
            }
            if (displayModes[i].prog) {
                d3dpp.Flags |= D3DPRESENTFLAG_PROGRESSIVE;
            }
            break;
        }
    }

    float f = float(d3dpp.BackBufferWidth) / float(d3dpp.BackBufferHeight);
    if (d3dpp.Flags & D3DPRESENTFLAG_WIDESCREEN) {
        Core::aspectFix = (16.0f / 9.0f) / f;
    } else {
        Core::aspectFix = (4.0f / 3.0f) / f;
    }

    d3dpp.BackBufferFormat       = D3DFMT_LIN_X8R8G8B8;
    d3dpp.BackBufferCount        = 1;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
    d3dpp.SwapEffect             = D3DSWAPEFFECT_DISCARD;
    d3dpp.FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_ONE;

    if (d3dpp.BackBufferHeight == 480) {
        d3dpp.MultiSampleType = D3DMULTISAMPLE_4_SAMPLES_MULTISAMPLE_GAUSSIAN;
    } else if (d3dpp.BackBufferHeight == 720) {
        d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE; //D3DMULTISAMPLE_2_SAMPLES_MULTISAMPLE_QUINCUNX; // unstable 60 fps
    } else if (d3dpp.BackBufferHeight == 1080) {
        d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;
    }

    if (FAILED(D3D->CreateDevice(0, D3DDEVTYPE_HAL, NULL, D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &device)))
        return E_FAIL;

    Core::width = d3dpp.BackBufferWidth;
    Core::height = d3dpp.BackBufferHeight;

    return S_OK;
}

void ContextSwap() {
    device->Present(NULL, NULL, NULL, NULL);
}


int checkLanguage() {
    int str = STR_LANG_EN;
    switch (XGetLanguage()) {
        case XC_LANGUAGE_ENGLISH    : str = STR_LANG_EN; break;
        case XC_LANGUAGE_FRENCH     : str = STR_LANG_FR; break;
        case XC_LANGUAGE_GERMAN     : str = STR_LANG_DE; break;
        case XC_LANGUAGE_SPANISH    : str = STR_LANG_ES; break;
        case XC_LANGUAGE_ITALIAN    : str = STR_LANG_IT; break;
        case XC_LANGUAGE_PORTUGUESE : str = STR_LANG_PT; break;
        case XC_LANGUAGE_JAPANESE   : str = STR_LANG_JA; break;
        case XC_LANGUAGE_TCHINESE   : str = STR_LANG_CN; break;
    }
    return str - STR_LANG_EN;
}

void main()
{
    strcpy(contentDir, "D:\\");

    saveDir[0] = cacheDir[0] = 0;
    if (XCreateSaveGame("U:\\", L"OpenLara", OPEN_ALWAYS, 0, saveDir, 256) == ERROR_SUCCESS) {
        strcpy(cacheDir, saveDir);
    }

    if (FAILED(ContextCreate()))
        return;

    QueryPerformanceFrequency(&timerFreq);
    QueryPerformanceCounter(&timerStart);

    Sound::channelsCount = 0;

    joyInit();
    sndInit();

    Core::defLang = checkLanguage();

    Game::init((char*)NULL);//"PSXDATA\\LEVEL1.PSX");

    while (!Core::isQuit) {
        joyUpdate();
        sndUpdate();

        if (Game::update()) {
            Game::render();
            ContextSwap();
        }
    };

    sndFree();
    Game::deinit();

    LD_LAUNCH_DASHBOARD LaunchData = { XLD_LAUNCH_DASHBOARD_MAIN_MENU };
    XLaunchNewImage(NULL, (LAUNCH_DATA*)&LaunchData);
}
