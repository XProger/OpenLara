#if defined(_WIN32)
    void* LEVEL1_PHD;
#elif defined(__GBA__)
    #include "LEVEL1_PHD.h"
#elif defined(__TNS__)
    void* LEVEL1_PHD;
#endif

#include "game.h"

Game game;

#if defined(_WIN32)
    uint32 SCREEN[FRAME_WIDTH * FRAME_HEIGHT];

    HWND hWnd;

    LARGE_INTEGER g_timer;
    LARGE_INTEGER g_current;

    #define WND_WIDTH   240*4
    #define WND_HEIGHT  160*4
#elif defined(__GBA__)
    //
#elif defined(__TNS__)
    unsigned int osTime;
    volatile unsigned int *timerBUS;
    volatile unsigned int *timerCLK;
    volatile unsigned int *timerCTR;
    volatile unsigned int *timerDIV;

    void timerInit()
    {
        timerBUS = (unsigned int*)0x900B0018;
        timerCLK = (unsigned int*)0x900C0004;
        timerCTR = (unsigned int*)0x900C0008;
        timerDIV = (unsigned int*)0x900C0080;

        *timerBUS &= ~(1 << 11);
        *timerDIV = 0x0A;
        *timerCTR = 0x82;

        osTime = *timerCLK;
    }

    int GetTickCount()
    {
        return (osTime - *timerCLK) / 33;
    }

    void SetPalette(unsigned short* palette)
    {
        unsigned short *palReg = (unsigned short*)0xC0000200;
        memcpy(palReg, palette, 256 * 2);
    }

    touchpad_info_t*  touchInfo;
    touchpad_report_t touchReport;
    uint8 inputData[0x20];

    void inputInit()
    {
        touchInfo = is_touchpad ? touchpad_getinfo() : NULL;
    }

    void inputUpdate()
    {
        if (touchInfo)
        {
            touchpad_scan(&touchReport);
        }

        memcpy(inputData, (void*)0x900E0000, 0x20);
    }

    bool keyDown(const t_key &key)
    {
        return (*(short*)(inputData + key.tpad_row)) & key.tpad_col;
    }
#endif

int32 fps;
int32 frameIndex = 0;
int32 fpsCounter = 0;

#ifdef PROFILE
    uint32 dbg_transform;
    uint32 dbg_poly;
    uint32 dbg_flush;
    uint32 dbg_vert_count;
    uint32 dbg_poly_count;
#endif

EWRAM_DATA ALIGN16 uint8 soundBuffer[2 * SND_SAMPLES + 32]; // 32 bytes of silence for DMA overrun while interrupt
uint32 curSoundBuffer = 0;

#if defined(_WIN32)
HWAVEOUT waveOut;
WAVEFORMATEX waveFmt = { WAVE_FORMAT_PCM, 1, SND_OUTPUT_FREQ, SND_OUTPUT_FREQ, 1, 8, sizeof(waveFmt) };
WAVEHDR waveBuf[2];

void soundInit() {
    sound.init();

    if (waveOutOpen(&waveOut, WAVE_MAPPER, &waveFmt, (INT_PTR)hWnd, 0, CALLBACK_WINDOW) != MMSYSERR_NOERROR) {
        return;
    }

    memset(&waveBuf, 0, sizeof(waveBuf));
    for (int i = 0; i < 2; i++) {
        WAVEHDR *waveHdr = waveBuf + i;
        waveHdr->dwBufferLength = SND_SAMPLES;
        waveHdr->lpData = (LPSTR)(soundBuffer + i * SND_SAMPLES);
        waveOutPrepareHeader(waveOut, waveHdr, sizeof(WAVEHDR));
        waveOutWrite(waveOut, waveHdr, sizeof(WAVEHDR));
    }
}

void soundFill() {
    WAVEHDR *waveHdr = waveBuf + curSoundBuffer;
    waveOutUnprepareHeader(waveOut, waveHdr, sizeof(WAVEHDR));
    sound.fill((uint8*)waveHdr->lpData, SND_SAMPLES);
    waveOutPrepareHeader(waveOut, waveHdr, sizeof(WAVEHDR));
    waveOutWrite(waveOut, waveHdr, sizeof(WAVEHDR));
    curSoundBuffer ^= 1;
}
#elif defined(__GBA__)
void soundInit()
{
    sound.init();

    REG_SOUNDCNT_X = SSTAT_ENABLE;
    REG_SOUNDCNT_H = SDS_ATMR0 | SDS_A100 | SDS_AL | SDS_AR | SDS_ARESET;
    REG_TM0D = 65536 - (16777216 / SND_OUTPUT_FREQ);
    REG_TM0CNT = TM_ENABLE;
    REG_DMA1DAD = (u32)&REG_FIFO_A;
}

void soundFill()
{
    if (curSoundBuffer == 1) {
        REG_DMA1CNT = 0;
        REG_DMA1SAD = (u32)soundBuffer;
        REG_DMA1CNT = DMA_DST_FIXED | DMA_REPEAT | DMA_16 | DMA_AT_FIFO | DMA_ENABLE;
    }

    sound.fill(soundBuffer + curSoundBuffer * SND_SAMPLES, SND_SAMPLES);
    curSoundBuffer ^= 1;
}
#endif

#if defined(_WIN32)
HDC hDC;

void blit() {
#ifdef ROTATE90_MODE
    for (int i = 0; i < FRAME_WIDTH * FRAME_HEIGHT; i++) {
        int32 x = FRAME_HEIGHT - (i % FRAME_HEIGHT) - 1;
        int32 y = i / FRAME_HEIGHT;
        uint16 c = ((uint16*)fb)[x * FRAME_WIDTH + y];
        SCREEN[i] = (((c << 3) & 0xFF) << 16) | ((((c >> 5) << 3) & 0xFF) << 8) | ((c >> 10 << 3) & 0xFF) | 0xFF000000;
    }
    const BITMAPINFO bmi = { sizeof(BITMAPINFOHEADER), FRAME_HEIGHT, -FRAME_WIDTH, 1, 32, BI_RGB, 0, 0, 0, 0, 0 };
    StretchDIBits(hDC, 0, 0, WND_WIDTH, WND_HEIGHT, 0, 0, FRAME_HEIGHT, FRAME_WIDTH, SCREEN, &bmi, DIB_RGB_COLORS, SRCCOPY);
#else
    for (int i = 0; i < FRAME_WIDTH * FRAME_HEIGHT; i++) {
        uint16 c = ((uint16*)fb)[i];
        SCREEN[i] = (((c << 3) & 0xFF) << 16) | ((((c >> 5) << 3) & 0xFF) << 8) | ((c >> 10 << 3) & 0xFF) | 0xFF000000;
    }
    const BITMAPINFO bmi = { sizeof(BITMAPINFOHEADER), FRAME_WIDTH, -FRAME_HEIGHT, 1, 32, BI_RGB, 0, 0, 0, 0, 0 };
    StretchDIBits(hDC, 0, 0, WND_WIDTH, WND_HEIGHT, 0, 0, FRAME_WIDTH, FRAME_HEIGHT, SCREEN, &bmi, DIB_RGB_COLORS, SRCCOPY);
#endif
}

LRESULT CALLBACK wndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_DESTROY :
        {
            PostQuitMessage(0);
            break;
        }

        case WM_KEYDOWN :
        case WM_KEYUP   :
        {
            InputKey key = IK_NONE;
            switch (wParam) {
                case VK_UP     : key = IK_UP;     break;
                case VK_RIGHT  : key = IK_RIGHT;  break;
                case VK_DOWN   : key = IK_DOWN;   break;
                case VK_LEFT   : key = IK_LEFT;   break;
                case 'Z'       : key = IK_B;      break;
                case 'X'       : key = IK_A;      break;
                case 'A'       : key = IK_L;      break;
                case 'S'       : key = IK_R;      break;
                case VK_RETURN : key = IK_START;  break;
                case VK_SPACE  : key = IK_SELECT; break;
            }

            if (msg != WM_KEYUP) {
                keys |= key;
            } else {
                keys &= ~key;
            }
            break;
        }

        case MM_WOM_DONE :
        {
            soundFill();
            break;
        }

        default :
            return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}

#elif defined(__GBA__)

void vblank() {
    frameIndex++;
    soundFill();
}

#endif

int main(void) {
#if defined(_WIN32) || defined(__TNS__)
    {
        #if defined(_WIN32)
            FILE *f = fopen("data/LEVEL1.PHD", "rb");
        #elif defined(__TNS__)
            FILE *f = fopen("/documents/OpenLara/LEVEL1.PHD.tns", "rb");
        #else
            #error
        #endif

        if (!f) {
            return 0;
        }

        fseek(f, 0, SEEK_END);
        int32 size = ftell(f);
        fseek(f, 0, SEEK_SET);
        LEVEL1_PHD = new uint8[size];
        fread(LEVEL1_PHD, 1, size, f);
        fclose(f);
    }
#elif defined(__GBA__)
    // set low latency mode via WAITCNT register (thanks to GValiente)
    REG_WSCNT = WS_ROM0_N2 | WS_ROM0_S1 | WS_PREFETCH;
#endif

    game.init();

#if defined(_WIN32)
    RECT r = { 0, 0, WND_WIDTH, WND_HEIGHT };

    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, false);
    int wx = (GetSystemMetrics(SM_CXSCREEN) - (r.right - r.left)) / 2;
    int wy = (GetSystemMetrics(SM_CYSCREEN) - (r.bottom - r.top)) / 2;

    hWnd = CreateWindow("static", "OpenLara GBA", WS_OVERLAPPEDWINDOW, wx + r.left, wy + r.top, r.right - r.left, r.bottom - r.top, 0, 0, 0, 0);
    hDC = GetDC(hWnd);

    SetWindowLong(hWnd, GWL_WNDPROC, (LONG)&wndProc);
    ShowWindow(hWnd, SW_SHOWDEFAULT);

    soundInit();

    MSG msg;

    int startTime = GetTickCount() - 33;
    int lastFrame = 0;

    do {
        if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {

            int frame = (GetTickCount() - startTime) / 33; 
            game.update(frame - lastFrame);
            lastFrame = frame;

            game.render();

            blit();
        }
    } while (msg.message != WM_QUIT);

#elif defined(__GBA__)
    irq_init(NULL);
    irq_add(II_VBLANK, vblank);
    irq_enable(II_VBLANK);

    // Undocumented - Green Swap trick for fake-AA feature (thanks to GValiente)
    // https://mgba-emu.github.io/gbatek/#4000002h---undocumented---green-swap-rw
    //uint16 &GreenSwap = *(uint16*)0x4000002;
    //GreenSwap = 1;

    soundInit();

    uint16 mode = DCNT_MODE5 | DCNT_BG2 | DCNT_PAGE;

#ifdef ROTATE90_MODE
    REG_BG2PA = 0;
    REG_BG2PB = (1 << 8);
    REG_BG2PC = -(1 << 7);
    REG_BG2PD = 0;
    REG_BG2Y = (FRAME_HEIGHT << 8) - 128;
#else
    REG_BG2PA = 256 - 64 - 16 - 4 - 1;
    REG_BG2PD = 256 - 48 - 2;
#endif

    int32 lastFrameIndex = -1;

    while (1) {
        //VBlankIntrWait();
        REG_DISPCNT = (mode ^= DCNT_PAGE);

        fb ^= 0xA000;

        { // input
            keys = 0;
            key_poll();
            if (key_is_down(KEY_UP))      keys |= IK_UP;
            if (key_is_down(KEY_RIGHT))   keys |= IK_RIGHT;
            if (key_is_down(KEY_DOWN))    keys |= IK_DOWN;
            if (key_is_down(KEY_LEFT))    keys |= IK_LEFT;
            if (key_is_down(KEY_A))       keys |= IK_A;
            if (key_is_down(KEY_B))       keys |= IK_B;
            if (key_is_down(KEY_L))       keys |= IK_L;
            if (key_is_down(KEY_R))       keys |= IK_R;
            if (key_is_down(KEY_START))   keys |= IK_START;
            if (key_is_down(KEY_SELECT))  keys |= IK_SELECT;
        }

        int32 frame = frameIndex / 2;
        game.update(frame - lastFrameIndex);
        lastFrameIndex = frame;

        game.render();

        fpsCounter++;
        if (frameIndex >= 60) {
            frameIndex -= 60;
            lastFrameIndex -= 30;

            fps = fpsCounter;

            fpsCounter = 0;
        }

    }
#elif defined(__TNS__)
    if (!has_colors)
        return 0;

    lcd_init(SCR_320x240_8);

    timerInit();
    inputInit();

    int startTime = GetTickCount();
    int lastTime = -16;
    int fpsTime = startTime;

    memset(keys, 0, sizeof(keys));

    while (1)
    {
        {
            keys = 0;

            inputUpdate();

            if (keyDown(KEY_NSPIRE_ESC))
            {
                break;
            }

            if (touchInfo && touchReport.contact)
            {
                float tx = float(touchReport.x) / float(touchInfo->width)  * 2.0f - 1.0f;
                float ty = float(touchReport.y) / float(touchInfo->height) * 2.0f - 1.0f;

                if (tx < -0.5f) keys |= IK_LEFT;
                if (tx >  0.5f) keys |= IK_RIGHT;
                if (ty >  0.5f) keys |= IK_UP;
                if (ty < -0.5f) keys |= IK_DOWN];
            }

            if (keyDown(KEY_NSPIRE_2)) keys |= IK_A;
            if (keyDown(KEY_NSPIRE_3)) keys |= IK_B;
            if (keyDown(KEY_NSPIRE_7)) keys |= IK_L;
            if (keyDown(KEY_NSPIRE_9)) keys |= IK_R;
            if (keyDown(KEY_NSPIRE_ENTER)) keys |= IK_START;
            if (keyDown(KEY_NSPIRE_SPACE)) keys |= IK_SELECT;
        }

        int time = GetTickCount() - startTime;
        game.update((time - lastTime) / 16);
        lastTime = time;

        game.render();

        lcd_blit(fb, SCR_320x240_8);
        //msleep(16);

        fpsCounter++;
        if (lastTime - fpsTime >= 1000)
        {
            fps = fpsCounter;
            fpsCounter = 0;
            fpsTime = lastTime - ((lastTime - fpsTime) - 1000);
        }
    }
#endif
}
