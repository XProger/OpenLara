#ifdef _DEBUG
    #define _CRTDBG_MAP_ALLOC
    #include "crtdbg.h"
    #define DEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
    #define new DEBUG_NEW
#endif

#ifdef MINIMAL
    #if _MSC_VER >= 1900 // VS2015 (1900) VS2017 (1910)
        #include <malloc.h>
        void __cdecl operator delete(void *ptr, unsigned int size) { free(ptr); }
        // add "/d2noftol3" to compiler additional options
        // add define _NO_CRT_STDIO_INLINE
    #endif
#endif

#include "game.h"

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

bool osSave(const char *name, const void *data, int size) {
    FILE *f = fopen(name, "wb");
    if (!f) return false;
    fwrite(data, size, 1, f);
    fclose(f);
    return true;
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
#define JOY_DEAD_ZONE_STICK      0.3f
#define JOY_DEAD_ZONE_TRIGGER    0.01f

bool joyReady;

void joyInit() {
    JOYINFOEX info;
    info.dwSize  = sizeof(info);
    info.dwFlags = JOY_RETURNALL;
    joyReady = joyGetPosEx(0, &info) == JOYERR_NOERROR;
}

void joyFree() {
    joyReady = false;
    memset(&Input::joy, 0, sizeof(Input::joy));
    for (int ik = ikJoyA; ik <= ikJoyPOV; ik++)
        Input::down[ik] = false;
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

void joyUpdate() {
    if (!joyReady) return;

    JOYINFOEX info;
    info.dwSize  = sizeof(info);
    info.dwFlags = JOY_RETURNALL;

    if (joyGetPosEx(0, &info) == JOYERR_NOERROR) {
        JOYCAPS caps;
        joyGetDevCaps(0, &caps, sizeof(caps));

        Input::setPos(ikJoyL, joyDir(joyAxis(info.dwXpos, caps.wXmin, caps.wXmax),
                                     joyAxis(info.dwYpos, caps.wYmin, caps.wYmax)));

        if ((caps.wCaps & JOYCAPS_HASR) && (caps.wCaps & JOYCAPS_HASU))
            Input::setPos(ikJoyR, joyDir(joyAxis(info.dwUpos, caps.wUmin, caps.wUmax),
                                         joyAxis(info.dwRpos, caps.wRmin, caps.wRmax)));

        if (caps.wCaps & JOYCAPS_HASZ) {
            float z  = joyAxis(info.dwZpos, caps.wZmin, caps.wZmax);
            InputKey key = z > JOY_DEAD_ZONE_TRIGGER ? ikJoyLT : (z < -JOY_DEAD_ZONE_TRIGGER ? ikJoyRT : ikNone);
            if (key != ikNone) {
                Input::setPos(key, vec2(fabsf(z), 0.0f));
                Input::setPos(key == ikJoyLT ? ikJoyRT : ikJoyLT, vec2(0.0f)); // release opposite trigger
            } else {
                Input::setPos(ikJoyLT, vec2(0.0f));
                Input::setPos(ikJoyRT, vec2(0.0f));
            }
        }

        if (caps.wCaps & JOYCAPS_HASPOV)
            if (info.dwPOV == JOY_POVCENTERED)
                Input::setPos(ikJoyPOV, vec2(0.0f));
            else
                Input::setPos(ikJoyPOV, vec2(float(1 + info.dwPOV / 4500), 0.0f));

        for (int i = 0; i < 10; i++)
            Input::setDown((InputKey)(ikJoyA + i), (info.dwButtons & (1 << i)) > 0);
    } else
        joyFree();
}

// touch
typedef BOOL (__stdcall *PREGISTERTOUCHWINDOW)(HWND, ULONG);
typedef BOOL (__stdcall *PGETTOUCHINPUTINFO)(HTOUCHINPUT, UINT, PTOUCHINPUT, int);
typedef BOOL (__stdcall *PCLOSETOUCHINPUTHANDLE)(HTOUCHINPUT);

PREGISTERTOUCHWINDOW    RegisterTouchWindowX;
PGETTOUCHINPUTINFO      GetTouchInputInfoX;
PCLOSETOUCHINPUTHANDLE  CloseTouchInputHandleX;

#define MAX_TOUCH_COUNT 6

void touchInit(HWND hWnd) {
    int value = GetSystemMetrics(SM_DIGITIZER);
    if (value) {
        HMODULE hUser32 = LoadLibrary("user32.dll");
        RegisterTouchWindowX     =   (PREGISTERTOUCHWINDOW)GetProcAddress(hUser32, "RegisterTouchWindow");
        GetTouchInputInfoX       =     (PGETTOUCHINPUTINFO)GetProcAddress(hUser32, "GetTouchInputInfo");
        CloseTouchInputHandleX   = (PCLOSETOUCHINPUTHANDLE)GetProcAddress(hUser32, "CloseTouchInputHandle");
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

void sndFree() {
    if (!sndReady) return;
    sndReady = false;
    waveOutUnprepareHeader(waveOut, &waveBuf[0], sizeof(WAVEHDR));
    waveOutUnprepareHeader(waveOut, &waveBuf[1], sizeof(WAVEHDR));
    waveOutReset(waveOut);
    waveOutClose(waveOut);
    delete[] sndData;
}

void sndFill(HWAVEOUT waveOut, LPWAVEHDR waveBuf) {
    if (!sndReady) return;
    waveOutUnprepareHeader(waveOut, waveBuf, sizeof(WAVEHDR));
    Sound::fill((Sound::Frame*)waveBuf->lpData, SND_SIZE / 4);
    waveOutPrepareHeader(waveOut, waveBuf, sizeof(WAVEHDR));
    waveOutWrite(waveOut, waveBuf, sizeof(WAVEHDR));
}

void sndInit(HWND hwnd) {
    if (waveOutOpen(&waveOut, WAVE_MAPPER, &waveFmt, (INT_PTR)hwnd, 0, CALLBACK_WINDOW) == MMSYSERR_NOERROR) {
        sndReady = true;
        sndData  = new char[SND_SIZE * 2];
        memset(&waveBuf, 0, sizeof(waveBuf));
        for (int i = 0; i < 2; i++) {
            waveBuf[i].dwBufferLength = SND_SIZE;
            waveBuf[i].lpData = sndData + SND_SIZE * i;
            sndFill(waveOut, &waveBuf[i]);
        }
    } else {
        sndReady = false;
        sndData  = NULL;
    }
}


static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        // window
        case WM_ACTIVATE :
            Input::reset();
            break;
        case WM_SIZE:
            Core::width  = LOWORD(lParam);
            Core::height = HIWORD(lParam);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        // keyboard
        case WM_CHAR       :
        case WM_SYSCHAR    : 
            break;
        case WM_KEYDOWN    :
        case WM_KEYUP      :
        case WM_SYSKEYDOWN :
        case WM_SYSKEYUP   :
            if (msg == WM_SYSKEYDOWN && wParam == VK_RETURN) { // switch to fullscreen or window
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

HGLRC initGL(HDC hDC) {
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
    HGLRC hRC = wglCreateContext(hDC);
    wglMakeCurrent(hDC, hRC);
    return hRC;
}

void freeGL(HGLRC hRC) {
    wglMakeCurrent(0, 0);
    wglDeleteContext(hRC);
}

char Stream::cacheDir[255];
char Stream::contentDir[255];

#ifdef _DEBUG
int main(int argc, char** argv) {
    _CrtMemState _msBegin, _msEnd, _msDiff;
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
    _CrtMemCheckpoint(&_msBegin);
//#elif PROFILE
#else
int main(int argc, char** argv) {
//#else
//int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
#endif
    Stream::contentDir[0] = Stream::cacheDir[0] = 0;

    strcat(Stream::cacheDir, getenv("APPDATA"));
    strcat(Stream::cacheDir, "\\OpenLara\\");
    CreateDirectory(Stream::cacheDir, NULL);

    RECT r = { 0, 0, 1280, 720 };
    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, false);

    HWND hWnd = CreateWindow("static", "OpenLara", WS_OVERLAPPEDWINDOW, 0, 0, r.right - r.left, r.bottom - r.top, 0, 0, 0, 0);

    HDC hDC = GetDC(hWnd);
    HGLRC hRC = initGL(hDC);
    
    Sound::channelsCount = 0;

    osStartTime = osGetTime();

    touchInit(hWnd);
    joyInit();
    sndInit(hWnd);

    char *lvlName = argc > 1 ? argv[1] : NULL;
    char *sndName = argc > 2 ? argv[2] : NULL;

    Game::init(lvlName, sndName);

    SetWindowLong(hWnd, GWL_WNDPROC, (LONG)&WndProc);
    ShowWindow(hWnd, SW_SHOWDEFAULT);

    MSG msg;

    while (!Core::isQuit) {
        if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                Core::quit();
        } else {
            joyUpdate();
            if (Game::update()) {
                Game::render();
                SwapBuffers(hDC);
            }
            #ifdef _DEBUG
                Sleep(20);
            #endif
        }
    };

    sndFree();
    Game::deinit();

    freeGL(hRC);
    ReleaseDC(hWnd, hDC);

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