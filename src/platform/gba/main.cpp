#ifndef _WIN32
#include <gba_console.h>
#include <gba_video.h>
#include <gba_timers.h>
#include <gba_interrupt.h>
#include <gba_systemcalls.h>
#include <gba_input.h>
#include <gba_dma.h>
#include <gba_affine.h>
#include <fade.h>

#include "LEVEL1_PHD.h"
#endif

//#define PROFILE

#include "common.h"
#include "level.h"

#ifdef _WIN32
    uint8* LEVEL1_PHD;

    uint32 SCREEN[WIDTH * HEIGHT];

    extern uint8 fb[WIDTH * HEIGHT * 2];

    #define WND_SCALE 4
#else
    extern uint32 fb;
#endif

bool keys[IK_MAX] = {};

int32 fps;
int32 frameIndex = 0;
int32 fpsCounter = 0;

void update(int32 frames) {
    for (int32 i = 0; i < frames; i++) {
        updateCamera();
    }
}

void render() {
    clear();

    drawRoom(6);
    flush();

    drawRoom(0);
    drawModel(0, 75162, 3072 - 512, 5000 + 1024);
    flush();

    drawNumber(fps, WIDTH, 16);
}

#ifdef _WIN32
HDC hDC;

void VBlankIntrWait() {
    #ifdef USE_MODE_5
        for (int i = 0; i < WIDTH * HEIGHT; i++) {
            uint16 c = ((uint16*)fb)[i];
            SCREEN[i] = (((c << 3) & 0xFF) << 16) | ((((c >> 5) << 3) & 0xFF) << 8) | ((c >> 10 << 3) & 0xFF) | 0xFF000000;
        }
    #else
        for (int i = 0; i < WIDTH * HEIGHT; i++) {
            uint16 c = palette[((uint8*)fb)[i]];
            SCREEN[i] = (((c << 3) & 0xFF) << 16) | ((((c >> 5) << 3) & 0xFF) << 8) | ((c >> 10 << 3) & 0xFF) | 0xFF000000;
        }
    #endif

    const BITMAPINFO bmi = { sizeof(BITMAPINFOHEADER), WIDTH, -HEIGHT, 1, 32, BI_RGB, 0, 0, 0, 0, 0 };
    StretchDIBits(hDC, 0, 0, 240 * WND_SCALE, 160 * WND_SCALE, 0, 0, WIDTH, HEIGHT, SCREEN, &bmi, DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK wndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_DESTROY :
            PostQuitMessage(0);
            break;
        case WM_KEYDOWN :
        case WM_KEYUP   : {
            InputKey key = IK_MAX;
            switch (wParam) {
                case VK_UP    : key = IK_UP;    break;
                case VK_RIGHT : key = IK_RIGHT; break;
                case VK_DOWN  : key = IK_DOWN;  break;
                case VK_LEFT  : key = IK_LEFT;  break;
                case 'Z'      : key = IK_A;     break;
                case 'X'      : key = IK_B;     break;
                case 'A'      : key = IK_L;     break;
                case 'S'      : key = IK_R;     break;
            }
            if (key != IK_MAX) {
                keys[key] = msg != WM_KEYUP;
            }
            break;
        }
        default :
            return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}
#endif

void vblank() {
    frameIndex++;
}

int main(void) {
#ifdef _WIN32
    {
        FILE *f = fopen("C:/Projects/TR/TR1_ANDROID/LEVEL1.PHD", "rb");
        fseek(f, 0, SEEK_END);
        int32 size = ftell(f);
        fseek(f, 0, SEEK_SET);
        LEVEL1_PHD = new uint8[size];
        fread(LEVEL1_PHD, 1, size, f);
        fclose(f);
    }
#else
    // set low latency mode via WAITCNT register (thanks to GValiente)
    #define BIT_SET(y, flag)    (y |= (flag))
    #define REG_WAITCNT_NV      *(u16*)(REG_BASE + 0x0204)

    BIT_SET(REG_WAITCNT_NV, 0x0008 | 0x0010 | 0x4000);
#endif

    initRender();

    readLevel(LEVEL1_PHD);

#ifdef _WIN32
    RECT r = { 0, 0, 240 * WND_SCALE, 160 * WND_SCALE };

    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, false);
    int wx = (GetSystemMetrics(SM_CXSCREEN) - (r.right - r.left)) / 2;
    int wy = (GetSystemMetrics(SM_CYSCREEN) - (r.bottom - r.top)) / 2;

    HWND hWnd = CreateWindow("static", "OpenLara GBA", WS_OVERLAPPEDWINDOW, wx + r.left, wy + r.top, r.right - r.left, r.bottom - r.top, 0, 0, 0, 0);
    hDC = GetDC(hWnd);

    SetWindowLong(hWnd, GWL_WNDPROC, (LONG)&wndProc);
    ShowWindow(hWnd, SW_SHOWDEFAULT);

    MSG msg;

    do {
        if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {

            update(1);
            render();

            VBlankIntrWait();
        }
    } while (msg.message != WM_QUIT);

#else
    irqInit();
    irqSet(IRQ_VBLANK, vblank);
    irqEnable(IRQ_VBLANK);

    uint16 mode = BG2_ON | BACKBUFFER;

    #ifdef USE_MODE_5
        mode |= MODE_5;

        REG_BG2PA = 256 - 64 - 16 - 4 - 1;
        REG_BG2PD = 256 - 48 - 2;
    #else
        mode |= MODE_4;
    #endif

    int32 lastFrameIndex = -1;

    #ifdef PROFILE
        int counter = 0;
    #endif

    while (1) {
        //VBlankIntrWait();

    #ifdef PROFILE
        if (counter++ >= 10) return 0;
    #endif

        SetMode(mode ^= BACKBUFFER);
        fb ^= 0xA000;

        scanKeys();
        uint16 key = keysDown() | keysHeld();
        keys[IK_UP]    = (key & KEY_UP);
        keys[IK_RIGHT] = (key & KEY_RIGHT);
        keys[IK_DOWN]  = (key & KEY_DOWN);
        keys[IK_LEFT]  = (key & KEY_LEFT);
        keys[IK_A]     = (key & KEY_A);
        keys[IK_B]     = (key & KEY_B);
        keys[IK_L]     = (key & KEY_L);
        keys[IK_R]     = (key & KEY_R);

        int32 frame = frameIndex;
        update(frame - lastFrameIndex);
        lastFrameIndex = frame;

        render();

        fpsCounter++;
        if (frameIndex >= 60) {
            frameIndex -= 60;
            lastFrameIndex -= 60;

            fps = fpsCounter;

            fpsCounter = 0;
        }

    }
#endif
}
