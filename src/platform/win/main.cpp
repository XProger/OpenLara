#ifdef _DEBUG
    #define _CRTDBG_MAP_ALLOC
    #include "crtdbg.h"
    #define DEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
    #define new DEBUG_NEW
#endif

#ifdef MINIMAL
    #if _MSC_VER >= 1900 // VS2015 (1900) VS2017 (1910)
        #define _NO_CRT_STDIO_INLINE
        #include <malloc.h>
        void __cdecl operator delete(void *ptr, unsigned int size) { free(ptr); }
        // add "/d2noftol3" to compiler additional options
    #endif
#endif

//#define VR_SUPPORT
// TODO: fix depth precision
// TODO: fix water surface rendering
// TODO: fix clipping
// TODO: add MSAA support for render targets
// TODO: add IK for arms
// TODO: controls (WIP)

// hint to the driver to use discrete GPU
extern "C" {
// NVIDIA
    __declspec(dllexport) int NvOptimusEnablement = 1;
// AMD
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

#ifdef VR_SUPPORT
   #include "libs/openvr/openvr.h"
#endif

#include "game.h"


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
int osStartTime = 0;

int osGetTime() {
#ifdef DEBUG
    LARGE_INTEGER Freq, Count;
    QueryPerformanceFrequency(&Freq);
    QueryPerformanceCounter(&Count);
    return int(Count.QuadPart * 1000L / Freq.QuadPart);
#else
    timeBeginPeriod(0);
    return int(timeGetTime()) - osStartTime;
#endif
}

// common input functions
InputKey keyToInputKey(int code) {
    static const int codes[] = {
        VK_LEFT, VK_RIGHT, VK_UP, VK_DOWN, VK_SPACE, VK_TAB, VK_RETURN, VK_ESCAPE, VK_SHIFT, VK_CONTROL, VK_MENU,
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
        'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
    };

    for (int i = 0; i < sizeof(codes) / sizeof(codes[0]); i++)
        if (codes[i] == code)
            return (InputKey)(ikLeft + i);
    return ikNone;
}

InputKey mouseToInputKey(int msg) {
    return (msg >= WM_LBUTTONDOWN && msg <= WM_LBUTTONDBLCLK) ? ikMouseL :
           (msg >= WM_RBUTTONDOWN && msg <= WM_RBUTTONDBLCLK) ? ikMouseR : ikMouseM;
}

// joystick
typedef struct _XINPUT_GAMEPAD {
    WORD                                wButtons;
    BYTE                                bLeftTrigger;
    BYTE                                bRightTrigger;
    SHORT                               sThumbLX;
    SHORT                               sThumbLY;
    SHORT                               sThumbRX;
    SHORT                               sThumbRY;
} XINPUT_GAMEPAD, *PXINPUT_GAMEPAD;

typedef struct _XINPUT_STATE {
    DWORD                               dwPacketNumber;
    XINPUT_GAMEPAD                      Gamepad;
} XINPUT_STATE, *PXINPUT_STATE;

typedef struct _XINPUT_VIBRATION {
    WORD                                wLeftMotorSpeed;
    WORD                                wRightMotorSpeed;
} XINPUT_VIBRATION, *PXINPUT_VIBRATION;

#define XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE  7849
#define XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE 8689
#define XINPUT_GAMEPAD_TRIGGER_THRESHOLD    30

DWORD (WINAPI *XInputGetState) (DWORD dwUserIndex, XINPUT_STATE* pState) = NULL;
DWORD (WINAPI *XInputSetState) (DWORD dwUserIndex, XINPUT_VIBRATION* pVibration) = NULL;
void  (WINAPI *XInputEnable)   (BOOL enable) = NULL;
#define XInputGetProc(x) (x = (decltype(x))GetProcAddress(h, #x))

#define JOY_DEAD_ZONE_STICK      0.3f
#define JOY_DEAD_ZONE_TRIGGER    0.01f
#define JOY_MIN_UPDATE_FX_TIME   50

struct JoyDevice {
    float vL, vR; // current value for left/right motor vibration
    float oL, oR; // last applied value
    int   time;   // time when we can send vibration update
    bool  ready;
} joyDevice[INPUT_JOY_COUNT];

bool osJoyReady(int index) {
    return joyDevice[index].ready;
}

void osJoyVibrate(int index, float L, float R) {
    joyDevice[index].vL = L;
    joyDevice[index].vR = R;
}

void joyRumble(int index) {
    JoyDevice &joy = joyDevice[index];
    if (XInputSetState && joy.ready && (joy.vL != joy.oL || joy.vR != joy.oR) && osGetTime() >= joy.time) {
        XINPUT_VIBRATION vibration;
        vibration.wLeftMotorSpeed  = int(joy.vL * 65535.0f);
        vibration.wRightMotorSpeed = int(joy.vR * 65535.0f);
        XInputSetState(index, &vibration);
        joy.oL = joy.vL;
        joy.oR = joy.vR;
        joy.time = osGetTime() + JOY_MIN_UPDATE_FX_TIME;
    }
}

void joyInit() {
    memset(joyDevice, 0, sizeof(joyDevice));

    HMODULE h = LoadLibrary("xinput1_3.dll");
    if (h == NULL)
        h = LoadLibrary("xinput9_1_0.dll");

    XInputGetProc(XInputGetState);
    XInputGetProc(XInputSetState);
    XInputGetProc(XInputEnable);

    for (int j = 0; j < INPUT_JOY_COUNT; j++) {
        if (XInputGetState) { // XInput
            XINPUT_STATE state;
            int res = XInputGetState(j, &state);
            joyDevice[j].ready = (XInputGetState(j, &state) == ERROR_SUCCESS);
        } else { // mmSystem (legacy)
            JOYINFOEX info;
            info.dwSize  = sizeof(info);
            info.dwFlags = JOY_RETURNALL;
            joyDevice[j].ready = (joyGetPosEx(j, &info) == JOYERR_NOERROR);
        }

        if (joyDevice[j].ready)
            LOG("Gamepad %d is ready\n", j + 1);
    }
}

void joyFree() {
    memset(joyDevice, 0, sizeof(joyDevice));
    Input::reset();
}

float joyAxis(int x, int xMin, int xMax) {
    return ((x - xMin) / (float)(xMax - xMin)) * 2.0f - 1.0f;
}

vec2 joyDir(float ax, float ay) {
    vec2 dir = vec2(ax, ay);
    float dist = min(1.0f, dir.length());
    if (dist < JOY_DEAD_ZONE_STICK) dist = 0.0f;

    return dir.normal() * dist;
}

int joyDeadZone(int value, int zone) {
    return (value < -zone || value > zone) ? value : 0;
}

void joyUpdate() {
    for (int j = 0; j < INPUT_JOY_COUNT; j++) {
        if (!joyDevice[j].ready) continue;

        joyRumble(j);

        if (XInputGetState) { // XInput
            XINPUT_STATE state;
            if (XInputGetState(j, &state) == ERROR_SUCCESS) {
                //osJoyVibrate(j, state.Gamepad.bLeftTrigger / 255.0f, state.Gamepad.bRightTrigger / 255.0f); // vibration test

                Input::setJoyPos(j, jkL,   joyDir(joyAxis(joyDeadZone( state.Gamepad.sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE),  -32768, 32767),
                                                  joyAxis(joyDeadZone(-state.Gamepad.sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE),  -32768, 32767)));
                Input::setJoyPos(j, jkR,   joyDir(joyAxis(joyDeadZone( state.Gamepad.sThumbRX, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE), -32768, 32767),
                                                  joyAxis(joyDeadZone(-state.Gamepad.sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE), -32768, 32767)));
                Input::setJoyPos(j, jkLT,  vec2(joyDeadZone(state.Gamepad.bLeftTrigger,  XINPUT_GAMEPAD_TRIGGER_THRESHOLD) / 255.0f, 0.0f));
                Input::setJoyPos(j, jkRT,  vec2(joyDeadZone(state.Gamepad.bRightTrigger, XINPUT_GAMEPAD_TRIGGER_THRESHOLD) / 255.0f, 0.0f));

                static const JoyKey keys[] = { jkUp, jkDown, jkLeft, jkRight, jkStart, jkSelect, jkL, jkR, jkLB, jkRB, jkNone, jkNone, jkA, jkB, jkX, jkY };
                for (int i = 0; i < 16; i++)
                    Input::setJoyDown(j, keys[i], (state.Gamepad.wButtons & (1 << i)) != 0);
            } else {
                joyFree();
                joyInit();
                break;
            }
        } else { // mmSystem (legacy)
            JOYINFOEX info;
            info.dwSize  = sizeof(info);
            info.dwFlags = JOY_RETURNALL;

            if (joyGetPosEx(j, &info) == JOYERR_NOERROR) {
                JOYCAPS caps;
                joyGetDevCaps(j, &caps, sizeof(caps));

                if (caps.wNumAxes > 0) {
                    Input::setJoyPos(j, jkL, joyDir(joyAxis(info.dwXpos, caps.wXmin, caps.wXmax),
                                                    joyAxis(info.dwYpos, caps.wYmin, caps.wYmax)));

                    if ((caps.wCaps & JOYCAPS_HASR) && (caps.wCaps & JOYCAPS_HASU))
                        Input::setJoyPos(j, jkR, joyDir(joyAxis(info.dwUpos, caps.wUmin, caps.wUmax),
                                                        joyAxis(info.dwRpos, caps.wRmin, caps.wRmax)));

                    if (caps.wCaps & JOYCAPS_HASZ) {
                        float z = joyAxis(info.dwZpos, caps.wZmin, caps.wZmax);
                        Input::setJoyPos(j, jkLT, vec2(0.0f));
                        Input::setJoyPos(j, jkRT, vec2(0.0f));

                        JoyKey key = z > JOY_DEAD_ZONE_TRIGGER ? jkLT : (z < -JOY_DEAD_ZONE_TRIGGER ? jkRT : jkNone);
                        if (key != jkNone)
                            Input::setJoyPos(j, key, vec2(fabsf(z), 0.0f));
                    }
                }

                int p = ((caps.wCaps & JOYCAPS_HASPOV) && (info.dwPOV != JOY_POVCENTERED)) ? (1 + info.dwPOV / 4500) : 0;
                Input::setJoyDown(j, jkUp,    p == 8 || p == 1 || p == 2);
                Input::setJoyDown(j, jkRight, p == 2 || p == 3 || p == 4);
                Input::setJoyDown(j, jkDown,  p == 4 || p == 5 || p == 6);
                Input::setJoyDown(j, jkLeft,  p == 6 || p == 7 || p == 8);

                for (int i = 0; i < 10; i++)
                    Input::setJoyDown(j, JoyKey(jkA + i), (info.dwButtons & (1 << i)) > 0);

            } else {
                joyFree();
                joyInit();
                break;
            }
        }
    }
}

// touch
BOOL (WINAPI *RegisterTouchWindowX)(HWND, ULONG);
BOOL (WINAPI *GetTouchInputInfoX)(HTOUCHINPUT, UINT, PTOUCHINPUT, int);
BOOL (WINAPI *CloseTouchInputHandleX)(HTOUCHINPUT);

#define MAX_TOUCH_COUNT 6

void touchInit(HWND hWnd) {
    int value = GetSystemMetrics(SM_DIGITIZER);
    if (value) {
        HMODULE hUser32 = LoadLibrary("user32.dll");
        RegisterTouchWindowX     = (decltype(RegisterTouchWindowX))   GetProcAddress(hUser32, "RegisterTouchWindow");
        GetTouchInputInfoX       = (decltype(GetTouchInputInfoX))     GetProcAddress(hUser32, "GetTouchInputInfo");
        CloseTouchInputHandleX   = (decltype(CloseTouchInputHandleX)) GetProcAddress(hUser32, "CloseTouchInputHandle");
        if (RegisterTouchWindowX && GetTouchInputInfoX && CloseTouchInputHandleX)
            RegisterTouchWindowX(hWnd, 0);
    }
}

void touchUpdate(HWND hWnd, HTOUCHINPUT hTouch, int count) {
    TOUCHINPUT touch[MAX_TOUCH_COUNT];
    count = min(count, MAX_TOUCH_COUNT);

    if (!GetTouchInputInfoX(hTouch, count, touch, sizeof(TOUCHINPUT)))
        return;

    for (int i = 0; i < count; i++) {
        InputKey key = Input::getTouch(touch[i].dwID);
        if (key == ikNone) continue;
        POINT pos = { touch[i].x / 100, touch[i].y / 100 };
        ScreenToClient(hWnd, &pos);
        Input::setPos(key, vec2(float(pos.x), float(pos.y)));

        if (touch[i].dwFlags & (TOUCHEVENTF_DOWN | TOUCHEVENTF_UP))
            Input::setDown(key, (touch[i].dwFlags & TOUCHEVENTF_DOWN) != 0);
    }

    CloseTouchInputHandleX(hTouch);
}

// sound
#define SND_SIZE 4704*2

bool sndReady;
char *sndData;
HWAVEOUT waveOut;
WAVEFORMATEX waveFmt = { WAVE_FORMAT_PCM, 2, 44100, 44100 * 4, 4, 16, sizeof(waveFmt) };
WAVEHDR waveBuf[2];
HANDLE  sndThread;
HANDLE  sndSema;

void sndFree() {
    if (!sndReady) return;
    sndReady = false;
    ReleaseSemaphore(sndSema, 1, NULL);
    WaitForSingleObject(sndThread, INFINITE);
    CloseHandle(sndThread);
    CloseHandle(sndSema);
    waveOutUnprepareHeader(waveOut, &waveBuf[0], sizeof(WAVEHDR));
    waveOutUnprepareHeader(waveOut, &waveBuf[1], sizeof(WAVEHDR));
    waveOutReset(waveOut);
    waveOutClose(waveOut);
    delete[] sndData;
}

DWORD WINAPI sndPrep(void* arg) {
    int idx = 0;
    while (1) {
        WaitForSingleObject(sndSema, INFINITE);
        if (!sndReady) break;

        WAVEHDR *hdr = waveBuf + idx;
        waveOutUnprepareHeader(waveOut, hdr, sizeof(WAVEHDR));
        Sound::fill((Sound::Frame*)hdr->lpData, SND_SIZE / 4);
        waveOutPrepareHeader(waveOut, hdr, sizeof(WAVEHDR));
        waveOutWrite(waveOut, hdr, sizeof(WAVEHDR));

        idx ^= 1;
    }
    return 0;
}

void sndFill(HWAVEOUT waveOut, LPWAVEHDR waveBufPrev) {
    if (!sndReady) return;
    ReleaseSemaphore(sndSema, 1, NULL);
}

void sndInit(HWND hwnd) {
    if (waveOutOpen(&waveOut, WAVE_MAPPER, &waveFmt, (INT_PTR)hwnd, 0, CALLBACK_WINDOW) == MMSYSERR_NOERROR) {
        sndReady = true;
        sndData  = new char[SND_SIZE * 2];
        memset(sndData, 0, SND_SIZE * 2);
        memset(&waveBuf, 0, sizeof(waveBuf));
        for (int i = 0; i < 2; i++) {
            WAVEHDR *hdr = waveBuf + i;
            hdr->dwBufferLength = SND_SIZE;
            hdr->lpData = sndData + SND_SIZE * i;
            waveOutPrepareHeader(waveOut, hdr, sizeof(WAVEHDR));
            waveOutWrite(waveOut, hdr, sizeof(WAVEHDR));
        }
        sndSema   = CreateSemaphore(NULL, 0, 2, NULL);
        sndThread = CreateThread(NULL, 0, sndPrep, NULL, 0, NULL);
    } else {
        sndReady = false;
        sndData  = NULL;
    }
}

HWND hWnd;

#ifdef _GAPI_GL
    HDC   hDC;
    HGLRC hRC;

    void ContextCreate() {
        hDC = GetDC(hWnd);

        PIXELFORMATDESCRIPTOR pfd;
        memset(&pfd, 0, sizeof(pfd));
        pfd.nSize        = sizeof(pfd);
        pfd.nVersion     = 1;
        pfd.dwFlags      = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
        pfd.cColorBits   = 32;
        pfd.cRedBits     = 8;
        pfd.cGreenBits   = 8;
        pfd.cBlueBits    = 8;
        pfd.cAlphaBits   = 8;
        pfd.cDepthBits   = 24;
        pfd.cStencilBits = 8;

        int format = ChoosePixelFormat(hDC, &pfd);
        SetPixelFormat(hDC, format, &pfd);
        hRC = wglCreateContext(hDC);
        wglMakeCurrent(hDC, hRC);
    }

    void ContextDelete() {
        wglMakeCurrent(0, 0);
        wglDeleteContext(hRC);
        ReleaseDC(hWnd, hDC);
    }

    void ContextResize() {}

    void ContextSwap() {
        SwapBuffers(hDC);
    }
#else
    LPDIRECT3D9           D3D;
    LPDIRECT3DDEVICE9     device;
    D3DPRESENT_PARAMETERS d3dpp;

    void ContextCreate() {
        memset(&d3dpp, 0, sizeof(d3dpp));
        d3dpp.Windowed                  = TRUE;
        d3dpp.BackBufferCount           = 1;
        d3dpp.BackBufferFormat          = D3DFMT_A8R8G8B8;
        d3dpp.SwapEffect                = D3DSWAPEFFECT_DISCARD;
        d3dpp.hDeviceWindow             = hWnd;
        d3dpp.EnableAutoDepthStencil    = TRUE;
        d3dpp.AutoDepthStencilFormat    = D3DFMT_D24S8;
        d3dpp.PresentationInterval      = D3DPRESENT_INTERVAL_IMMEDIATE;

        if (!(D3D = Direct3DCreate9(D3D_SDK_VERSION))) {
            LOG("! cant't initialize DirectX\n");
            return;
        }

        if (!SUCCEEDED(D3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &device)))
            LOG("! can't create device\n");
    }

    void ContextDelete() {
        GAPI::deinit();
        if (device) device->Release();
        if (D3D)    D3D->Release();
    }

    void ContextResize() {
        if (Core::width <= 0 || Core::height <= 0)
            return;
        d3dpp.BackBufferWidth  = Core::width;
        d3dpp.BackBufferHeight = Core::height;
        GAPI::resetDevice();
    }

    void ContextSwap() {
        if (device->Present(NULL, NULL, NULL, NULL) == D3DERR_DEVICELOST)
            GAPI::resetDevice();
    }
#endif

#ifdef _NAPI_SOCKET
char command[256];

void parseCommand(char *cmd) {
    NAPI::Peer peer;
    int pos = 0;
    for (int i = 0; i < strlen(cmd); i++)
        if (cmd[i] == ':') {
            cmd[i] = 0;
            pos = i + 1;
            break;
        }
    peer.ip = inet_addr(cmd);
    peer.port = htons(atoi(&cmd[pos]));
    cmd[pos - 1] = ':';

    LOG("join %s:%d\n", inet_ntoa(*(in_addr*)&peer.ip), ntohs(peer.port));
    Network::joinGame(peer);
}
#endif

static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        // window
        case WM_ACTIVATE :
            if (XInputEnable)
                XInputEnable(wParam != WA_INACTIVE);
            Input::reset();
            break;
        case WM_SIZE:
            Core::width  = LOWORD(lParam);
            Core::height = HIWORD(lParam);
            ContextResize();
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        // keyboard
        case WM_CHAR       :
        case WM_SYSCHAR    :
            #ifdef _NAPI_SOCKET
            if (wParam == VK_RETURN) {
                parseCommand(command);
                //command[0] = 0;
            } else if ((wParam >= '0' && wParam <= '9') || wParam == ':' || wParam == '.') {
                int len = strlen(command);
                command[len] = wParam;
                command[len + 1] = 0;
            } else if (wParam == 8) {
                int len = strlen(command);
                if (len > 0)
                    command[len - 1] = 0;
            }
            #endif
            break;
        case WM_KEYDOWN    :
        case WM_KEYUP      :
        case WM_SYSKEYDOWN :
        case WM_SYSKEYUP   :
            if (msg == WM_SYSKEYDOWN && wParam == VK_RETURN) { // Alt + Enter - switch to fullscreen or window
                static WINDOWPLACEMENT pLast;
                DWORD style = GetWindowLong(hWnd, GWL_STYLE);
                if (style & WS_OVERLAPPEDWINDOW) {
                    MONITORINFO mInfo = { sizeof(mInfo) };
                    if (GetWindowPlacement(hWnd, &pLast) && GetMonitorInfo(MonitorFromWindow(hWnd, MONITOR_DEFAULTTOPRIMARY), &mInfo)) {
                        RECT &r = mInfo.rcMonitor;
                        SetWindowLong(hWnd, GWL_STYLE, style & ~WS_OVERLAPPEDWINDOW);
                        MoveWindow(hWnd, r.left, r.top, r.right - r.left, r.bottom - r.top, FALSE);
                    }
                } else {
                    SetWindowLong(hWnd, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
                    SetWindowPlacement(hWnd, &pLast);
                }
                break;
            }
            if (msg == WM_SYSKEYDOWN && wParam == VK_F4) { // Alt + F4 - close application
                Core::quit();
                break;
            }
            Input::setDown(keyToInputKey(wParam), msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);
            break;
        // mouse
        case WM_LBUTTONDOWN   :
        case WM_LBUTTONUP     :
        case WM_LBUTTONDBLCLK :
        case WM_RBUTTONDOWN   :
        case WM_RBUTTONUP     :
        case WM_RBUTTONDBLCLK :
        case WM_MBUTTONDOWN   :
        case WM_MBUTTONUP     :
        case WM_MBUTTONDBLCLK : {
            if ((GetMessageExtraInfo() & 0xFFFFFF00) == 0xFF515700) break;
            InputKey key = mouseToInputKey(msg);
            Input::setPos(key, vec2((float)(short)LOWORD(lParam), (float)(short)HIWORD(lParam)));
            bool down = msg != WM_LBUTTONUP && msg != WM_RBUTTONUP && msg != WM_MBUTTONUP;
            Input::setDown(key, down);
            if (down)
                SetCapture(hWnd);
            else
                ReleaseCapture();
            break;
        }
        case WM_MOUSEMOVE :
            if ((GetMessageExtraInfo() & 0xFFFFFF00) == 0xFF515700) break;
            Input::setPos(ikMouseL, vec2((float)(short)LOWORD(lParam), (float)(short)HIWORD(lParam)));
            break;
        // joystick
        case WM_DEVICECHANGE :
            joyFree();
            joyInit();
            return 1;
        // touch
        case WM_TOUCH :
            touchUpdate(hWnd, (HTOUCHINPUT)lParam, wParam);
            break;
        // sound
        case MM_WOM_DONE :
            sndFill((HWAVEOUT)wParam, (WAVEHDR*)lParam);
            break;
        default :
            return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}

//VR Support
#ifdef VR_SUPPORT
vr::IVRSystem *hmd; // vrContext
vr::IVRRenderModels* rm; // not currently in use
vr::TrackedDevicePose_t tPose[vr::k_unMaxTrackedDeviceCount];
//eye textures(eventually)

//action handles
vr::VRActionHandle_t VRcLeft      = vr::k_ulInvalidActionHandle;
vr::VRActionHandle_t VRcRight     = vr::k_ulInvalidActionHandle;
vr::VRActionHandle_t VRcUp        = vr::k_ulInvalidActionHandle;
vr::VRActionHandle_t VRcDown      = vr::k_ulInvalidActionHandle;

vr::VRActionHandle_t VRcJump      = vr::k_ulInvalidActionHandle;
vr::VRActionHandle_t VRcWalk      = vr::k_ulInvalidActionHandle;
vr::VRActionHandle_t VRcAction    = vr::k_ulInvalidActionHandle;
vr::VRActionHandle_t VRcWeapon    = vr::k_ulInvalidActionHandle;
vr::VRActionHandle_t VRcRoll      = vr::k_ulInvalidActionHandle;
vr::VRActionHandle_t VRcLook      = vr::k_ulInvalidActionHandle;
vr::VRActionHandle_t VRcInventory = vr::k_ulInvalidActionHandle;
vr::VRActionHandle_t VRcStart     = vr::k_ulInvalidActionHandle;

vr::VRActionSetHandle_t m_actionsetDemo = vr::k_ulInvalidActionSetHandle;

vr::VRInputValueHandle_t m_leftHand  = vr::k_ulInvalidInputValueHandle;
vr::VRInputValueHandle_t m_rightHand = vr::k_ulInvalidInputValueHandle;

//only in select TR games
vr::VRActionHandle_t VRcDuck = vr::k_ulInvalidActionHandle;
vr::VRActionHandle_t VRcDash = vr::k_ulInvalidActionHandle;
//

vr::VRActionSetHandle_t m_actionsetTR = vr::k_ulInvalidActionSetHandle;

void vrInit() {
    vr::EVRInitError eError = vr::VRInitError_None;
    hmd = vr::VR_Init(&eError, vr::VRApplication_Scene);
    //rm = vr::VRRenderModels(); // initialize render models interface

    if (eError != vr::VRInitError_None) {
        hmd = NULL;
        LOG("! unable to init VR runtime: %s\n", vr::VR_GetVRInitErrorAsEnglishDescription(eError));
        return;
    }

    if (!vr::VRCompositor()) {
        vr::VR_Shutdown();
        LOG("! compositor initialization failed\n");
        return;
    }

    //set manifest
    vr::VRInput()->SetActionManifestPath("C:/Users/Austin/Desktop/OpenLaraGitTest2/OpenLara/bin/TombRaidervr_actions.json"); // needs absolutepath
        // get action handles
    vr::VRInput()->GetActionHandle("/actions/demo/in/Left",      &VRcLeft);
    vr::VRInput()->GetActionHandle("/actions/demo/in/Right",     &VRcRight);
    vr::VRInput()->GetActionHandle("/actions/demo/in/Up",        &VRcUp);
    vr::VRInput()->GetActionHandle("/actions/demo/in/Down",      &VRcDown);
    vr::VRInput()->GetActionHandle("/actions/demo/in/Jump",      &VRcJump);
    vr::VRInput()->GetActionHandle("/actions/demo/in/Walk",      &VRcWalk);
    vr::VRInput()->GetActionHandle("/actions/demo/in/Action",    &VRcAction);
    vr::VRInput()->GetActionHandle("/actions/demo/in/Weapon",    &VRcWeapon);
    vr::VRInput()->GetActionHandle("/actions/demo/in/Roll",      &VRcRoll);
    vr::VRInput()->GetActionHandle("/actions/demo/in/Inventory", &VRcInventory);
    vr::VRInput()->GetActionHandle("/actions/demo/in/Start",     &VRcStart);
    //get actionsethandle
    vr::VRInput()->GetActionSetHandle("/actions/demo",           &m_actionsetDemo);
    //get input source handles
    vr::VRInput()->GetInputSourceHandle("/user/hand/left",       &m_leftHand);
    vr::VRInput()->GetInputSourceHandle("/user/hand/right",      &m_rightHand);
    // aren't using right now
    //vr::VRInput()->GetActionHandle("/actions/demo/in/TriggerHaptic", &m_actionTriggerHaptic);
    //vr::VRInput()->GetActionHandle("/actions/demo/in/AnalogInput", &m_actionAnalongInput);
}

void vrInitTargets() {
    if (!hmd) return;
    uint32_t width, height;
    hmd->GetRecommendedRenderTargetSize(&width, &height);
    Core::eyeTex[0] = new Texture(width, height, 1, TexFormat::FMT_RGBA);
    Core::eyeTex[1] = new Texture(width, height, 1, TexFormat::FMT_RGBA);
}

void vrFree() {
    if (!hmd) return;
    vr::VR_Shutdown();
}

mat4 convToMat4(const vr::HmdMatrix44_t &m) {
    return mat4(m.m[0][0], m.m[1][0], m.m[2][0], m.m[3][0],
                m.m[0][1], m.m[1][1], m.m[2][1], m.m[3][1],
                m.m[0][2], m.m[1][2], m.m[2][2], m.m[3][2],
                m.m[0][3], m.m[1][3], m.m[2][3], m.m[3][3]);
}

mat4 convToMat4(const vr::HmdMatrix34_t &m) {
    return mat4(m.m[0][0], m.m[1][0], m.m[2][0], 0.0f,
                m.m[0][1], m.m[1][1], m.m[2][1], 0.0f,
                m.m[0][2], m.m[1][2], m.m[2][2], 0.0f,
                m.m[0][3], m.m[1][3], m.m[2][3], 1.0f);
}
//utility function for reading digital state
bool GetDigitalActionState(vr::VRActionHandle_t action, vr::VRInputValueHandle_t *pDevicePath = nullptr)
{
    vr::InputDigitalActionData_t actionData;
    vr::VRInput()->GetDigitalActionData(action, &actionData, sizeof(actionData), vr::k_ulInvalidInputValueHandle);
    if (pDevicePath) {
        *pDevicePath = vr::k_ulInvalidInputValueHandle;
        if (actionData.bActive) {
            vr::InputOriginInfo_t originInfo;
            if (vr::VRInputError_None == vr::VRInput()->GetOriginTrackedDeviceInfo(actionData.activeOrigin, &originInfo, sizeof(originInfo))) {
                *pDevicePath = originInfo.devicePath;
            }
        }
    }
    return actionData.bActive && actionData.bState;
}


void ProcessVREvent(const vr::VREvent_t &event) {
    char buffer[1024] = "test";
    switch (event.eventType) {
    case vr::VREvent_TrackedDeviceActivated:
        //SetupRenderModelForTrackedDevice( event.trackedDeviceIndex );
        vr::RenderModel_t ** controllerRender;
        hmd->GetStringTrackedDeviceProperty(event.trackedDeviceIndex, vr::ETrackedDeviceProperty::Prop_RenderModelName_String, buffer, 1024); // can be filled with an error,but I can't find the right type
        //rm->LoadRenderModel_Async(buffer, controllerRender);
        // need to process render model?
        LOG("Device %u attached. Setting up render model\n", event.trackedDeviceIndex);
        break;
    case vr::VREvent_TrackedDeviceDeactivated:
        LOG("Device %u detached.\n", event.trackedDeviceIndex);
        Input::reset();
        break;
    case vr::VREvent_TrackedDeviceUpdated: //not sure what to do here
        LOG("Device %u updated.\n", event.trackedDeviceIndex);
        break;
    }
}

void vrUpdateInput() {
    if (!hmd) return;
    vr::VREvent_t event;

    while (hmd->PollNextEvent(&event, sizeof(event))) {
        ProcessVREvent(event);
    }

    vr::VRActiveActionSet_t actionSet = { 0 };
    actionSet.ulActionSet = m_actionsetDemo;
    vr::VRInput()->UpdateActionState(&actionSet, sizeof(actionSet), 1);

    Input::hmd.state[cUp]        = GetDigitalActionState(VRcUp);
    Input::hmd.state[cDown]      = GetDigitalActionState(VRcDown);
    Input::hmd.state[cRight]     = GetDigitalActionState(VRcRight);
    Input::hmd.state[cLeft]      = GetDigitalActionState(VRcLeft);
    Input::hmd.state[cJump]      = GetDigitalActionState(VRcJump);
    Input::hmd.state[cWalk]      = GetDigitalActionState(VRcWalk);
    Input::hmd.state[cAction]    = GetDigitalActionState(VRcAction);
    Input::hmd.state[cWeapon]    = GetDigitalActionState(VRcWeapon);
    Input::hmd.state[cRoll]      = GetDigitalActionState(VRcRoll);
    Input::hmd.state[cStart]     = GetDigitalActionState(VRcStart);
    Input::hmd.state[cInventory] = GetDigitalActionState(VRcInventory);
}

void vrUpdateView() {
    if (!hmd) return;
    vr::VRCompositor()->WaitGetPoses(tPose, vr::k_unMaxTrackedDeviceCount, NULL, 0);

    if (!tPose[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid)
        return;

    mat4 pL = convToMat4(hmd->GetProjectionMatrix(vr::Eye_Left,  8.0f, 45.0f * 1024.0f));
    mat4 pR = convToMat4(hmd->GetProjectionMatrix(vr::Eye_Right, 8.0f, 45.0f * 1024.0f));

    mat4 head = convToMat4(tPose[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking);
    if (Input::hmd.zero.x == INF)
        Input::hmd.zero = head.getPos();
    head.setPos(head.getPos() - Input::hmd.zero);

    mat4 vL = head * convToMat4(hmd->GetEyeToHeadTransform(vr::Eye_Left));
    mat4 vR = head * convToMat4(hmd->GetEyeToHeadTransform(vr::Eye_Right));

    vL.setPos(vL.getPos() * ONE_METER);
    vR.setPos(vR.getPos() * ONE_METER);
    Input::hmd.setView(pL, pR, vL, vR);

    Input::hmd.head = head;
}

void vrCompose() {
    if (!hmd) return;
    vr::Texture_t LTex = {(void*)(uintptr_t)Core::eyeTex[0]->ID, vr::TextureType_OpenGL, vr::ColorSpace_Gamma};
    vr::VRCompositor()->Submit(vr::Eye_Left, &LTex);
    vr::Texture_t RTex = {(void*)(uintptr_t)Core::eyeTex[1]->ID, vr::TextureType_OpenGL, vr::ColorSpace_Gamma};
    vr::VRCompositor()->Submit(vr::Eye_Right, &RTex);
}
#endif // #ifdef VR_SUPPORT

#ifdef _DEBUG
int main(int argc, char** argv) {
    _CrtMemState _msBegin, _msEnd, _msDiff;
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
    _CrtMemCheckpoint(&_msBegin);
//#elif PROFILE
#elif PROFILE
int main(int argc, char** argv) {
#else
int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    int argc = (lpCmdLine && strlen(lpCmdLine)) ? 2 : 1;
    char *argv[] = { "", lpCmdLine };
#endif
    cacheDir[0] = saveDir[0] = contentDir[0] = 0;

    strcat(cacheDir, getenv("APPDATA"));
    strcat(cacheDir, "\\OpenLara\\");
    strcpy(saveDir, cacheDir);
    CreateDirectory(cacheDir, NULL);

    RECT r = { 0, 0, 1280, 720 };
    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, false);

#ifndef _DEBUG
    {
        int ox = (GetSystemMetrics(SM_CXSCREEN) - (r.right - r.left)) / 2;
        int oy = (GetSystemMetrics(SM_CYSCREEN) - (r.bottom - r.top)) / 2;
        r.left   += ox;
        r.top    += oy;
        r.right  += ox;
        r.bottom += oy;
    }
#else
    r.right  -= r.left;
    r.bottom -= r.top;
    r.left = r.top = 0;
#endif

    hWnd = CreateWindow("static", "OpenLara", WS_OVERLAPPEDWINDOW, r.left, r.top, r.right - r.left, r.bottom - r.top, 0, 0, 0, 0);
    SendMessage(hWnd, WM_SETICON, 1, (LPARAM)LoadIcon(GetModuleHandle(NULL), "MAINICON"));

    ContextCreate();

#ifdef VR_SUPPORT
    vrInit();
#endif

    Sound::channelsCount = 0;

    osStartTime = osGetTime();

    touchInit(hWnd);
    joyInit();
    sndInit(hWnd);

    Game::init(argc > 1 ? argv[1] : NULL);

#ifdef VR_SUPPORT
    Input::hmd.ready = hmd != NULL;
    vrInitTargets();
#endif

    SetWindowLong(hWnd, GWL_WNDPROC, (LONG)&WndProc);

    if (Core::isQuit) {
        MessageBoxA(hWnd, "Please check the readme file first!", "Game resources not found", MB_ICONHAND);
    } else {
        ShowWindow(hWnd, SW_SHOWDEFAULT);
    }

    MSG msg;

    while (!Core::isQuit) {
        if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                Core::quit();
        } else {
            joyUpdate();
        #ifdef VR_SUPPORT
            vrUpdateInput();
        #endif
            if (Game::update()) {
            #ifdef VR_SUPPORT
                vrUpdateView();
            #endif
                Game::render();
            #ifdef VR_SUPPORT
                vrCompose();
            #endif
                Core::waitVBlank();
                ContextSwap();
            }
            #ifdef _DEBUG
                Sleep(10);
            #endif
        }
    };

    sndFree();
    Game::deinit();

#ifdef VR_SUPPORT
    vrFree();
#endif

    ContextDelete();

    DestroyWindow(hWnd);
 #ifdef _DEBUG
    _CrtMemCheckpoint(&_msEnd);

    if (_CrtMemDifference(&_msDiff, &_msBegin, &_msEnd) > 0) {
        _CrtDumpMemoryLeaks();
        system("pause");
    }
#endif

    return 0;
}
