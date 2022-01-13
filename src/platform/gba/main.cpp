#if defined(_WIN32) || defined(__DOS__)
    const void* TRACKS_IMA;
    const void* TITLE_SCR;
    const void* levelData;
#endif

#include "game.h"

EWRAM_DATA int32 fps;
EWRAM_DATA int32 frameIndex = 0;
EWRAM_DATA int32 fpsCounter = 0;

#if defined(_WIN32)
    uint32 SCREEN[FRAME_WIDTH * FRAME_HEIGHT];

    HWND hWnd;

    LARGE_INTEGER g_timer;
    LARGE_INTEGER g_current;

    #define WND_WIDTH   240*4
    #define WND_HEIGHT  160*4

    uint16 MEM_PAL_BG[256];

    void osSetPalette(const uint16* palette)
    {
        memcpy(MEM_PAL_BG, palette, 256 * 2);
    }

    int32 osGetSystemTimeMS()
    {
        return GetTickCount();
    }

    bool osSaveSettings()
    {
        FILE* f = fopen("settings.dat", "wb");
        if (!f) return false;
        fwrite(&gSettings, sizeof(gSettings), 1, f);
        fclose(f);
        return true;
    }

    bool osLoadSettings()
    {
        FILE* f = fopen("settings.dat", "rb");
        if (!f) return false;
        uint8 version;
        fread(&version, 1, 1, f);
        if (version != gSettings.version) {
            fclose(f);
            return false;
        }
        fread((uint8*)&gSettings + 1, sizeof(gSettings) - 1, 1, f);
        fclose(f);
        return true;
    }

    bool osCheckSave()
    {
        FILE* f = fopen("savegame.dat", "rb");
        if (!f) return false;
        fclose(f);
        return true;
    }

    bool osSaveGame()
    {
        FILE* f = fopen("savegame.dat", "wb");
        if (!f) return false;
        fwrite(&gSaveGame, sizeof(gSaveGame), 1, f);
        fwrite(&gSaveData, gSaveGame.dataSize, 1, f);
        fclose(f);
        return true;
    }

    bool osLoadGame()
    {
        FILE* f = fopen("savegame.dat", "rb");
        if (!f) return false;

        uint32 version;
        fread(&version, sizeof(version), 1, f);

        if (SAVEGAME_VER != version)
        {
            fclose(f);
            return false;
        }

        fread(&gSaveGame.dataSize, sizeof(gSaveGame) - sizeof(version), 1, f);
        fread(&gSaveData, gSaveGame.dataSize, 1, f);
        fclose(f);
        return true;
    }

    void osJoyVibrate(int32 index, int32 L, int32 R) {}

#elif defined(__GBA__)
    void osSetPalette(const uint16* palette)
    {
        memcpy((uint16*)MEM_PAL_BG, palette, 256 * 2);
    }

    int32 osGetSystemTimeMS()
    {
        return 0; // TODO
    }

    const uint8 SRAM_MAGIC[4] = { 14, 02, 19, 68 }; 

    int32 byteCopy(volatile uint8* dst, const volatile uint8* src, uint32 count)
    {
        for (uint32 i = 0; i < count; i++)
        {
            *dst++ = *src++;
        }
        return count;
    }

    bool checkSRAM(volatile uint8* src)
    {
        for (uint32 i = 0; i < sizeof(SRAM_MAGIC); i++)
        {
            if (SRAM_MAGIC[i] != *src++)
                return false;
        }
        return true;
    }

    bool osSaveSettings()
    {
        volatile uint8* ptr = (uint8*)MEM_SRAM;

        byteCopy(ptr, SRAM_MAGIC, 4);
        if (!checkSRAM(ptr))
            return false;
        ptr += 4;

        volatile uint8* data = (uint8*)&gSettings;
        byteCopy(ptr, data, sizeof(gSettings));

        return true;
    }

    bool osLoadSettings()
    {
        volatile uint8* ptr = (uint8*)MEM_SRAM;
        
        if (!checkSRAM(ptr))
            return false;
        ptr += 4;

        if (SETTINGS_VER != *ptr)
            return false;

        volatile uint8* data = (uint8*)&gSettings;
        byteCopy(data, ptr, sizeof(gSettings));

        return true;
    }

    bool osCheckSave()
    {
        volatile uint8* ptr = (uint8*)MEM_SRAM + SETTINGS_SIZE;

        if (!checkSRAM(ptr))
            return false;
        ptr += 4;

        uint32 version;
        byteCopy((uint8*)&version, ptr, sizeof(version));

        return (SAVEGAME_VER == version);
    }

    bool osSaveGame()
    {
        volatile uint8* ptr = (uint8*)MEM_SRAM + SETTINGS_SIZE;

        byteCopy(ptr, SRAM_MAGIC, 4);
        if (!checkSRAM(ptr))
            return false;

        ptr += 4;
        ptr += byteCopy(ptr, (uint8*)&gSaveGame, sizeof(gSaveGame));
        byteCopy(ptr, (uint8*)&gSaveData, gSaveGame.dataSize);
        return true;
    }

    bool osLoadGame()
    {
        if (!osCheckSave())
            return false;

        volatile uint8* ptr = (uint8*)MEM_SRAM + SETTINGS_SIZE + 4; // skip magic

        ptr += byteCopy((uint8*)&gSaveGame, ptr, sizeof(gSaveGame));
        byteCopy((uint8*)&gSaveData, ptr, gSaveGame.dataSize);

        return true;
    }

    #define GPIO_RUMBLE_DATA      (*(vu16*)0x80000C4)
    #define GPIO_RUMBLE_DIRECTION (*(vu16*)0x80000C6)
    #define GPIO_RUMBLE_CONTROL   (*(vu16*)0x80000C8)
    #define GPIO_RUMBLE_MASK      (1 << 3)

    #define CART_RUMBLE_TICKS     6

    EWRAM_DATA int32 cartRumbleTick = 0;

    void rumbleInit()
    {
        GPIO_RUMBLE_DIRECTION = GPIO_RUMBLE_MASK;
        GPIO_RUMBLE_CONTROL = 1;
    }

    void rumbleSet(bool enable)
    {
        if (enable) {
            GPIO_RUMBLE_DATA |= GPIO_RUMBLE_MASK;
            cartRumbleTick = CART_RUMBLE_TICKS;
        } else {
            GPIO_RUMBLE_DATA &= ~GPIO_RUMBLE_MASK;
            cartRumbleTick = 0;
        }
    }

    void rumbleUpdate(int32 frames)
    {
        if (!cartRumbleTick)
            return;

        cartRumbleTick -= frames;

        if (cartRumbleTick <= 0) {
            rumbleSet(false);
        }
    }

    void osJoyVibrate(int32 index, int32 L, int32 R)
    {
        if (!gSettings.controls_vibration)
            return;
        rumbleSet(X_MAX(L, R) > 0);
    }
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

    int32 GetTickCount()
    {
        return (osTime - *timerCLK) / 33;
    }

    int32 osGetSystemTimeMS()
    {
        return *timerCLK / 33;
    }

    bool osSaveSettings()
    {
        return false;
    }

    bool osLoadSettings()
    {
        return false;
    }

    bool osCheckSave()
    {
        return false;
    }

    bool osSaveGame()
    {
        return false;
    }

    bool osLoadGame()
    {
        return false;
    }

    void osJoyVibrate(int32 index, int32 L, int32 R) {}

    void osSetPalette(const uint16* palette)
    {
        memcpy((uint16*)0xC0000200, palette, 256 * 2);
    }

    touchpad_info_t*  touchInfo;
    touchpad_report_t touchReport;
    uint8 inputData[0x20];

    bool keyDown(const t_key &key)
    {
        return (*(short*)(inputData + key.tpad_row)) & key.tpad_col;
    }

    void inputInit()
    {
        touchInfo = is_touchpad ? touchpad_getinfo() : NULL;
    }

    void inputUpdate()
    {
        keys = 0;

        if (touchInfo)
        {
            touchpad_scan(&touchReport);
        }

        memcpy(inputData, (void*)0x900E0000, 0x20);

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
#elif defined(__DOS__)
    #define KB_ESC      1
    #define KB_A        30
    #define KB_S        31
    #define KB_Z        44
    #define KB_X        45
    #define KB_UP       72
    #define KB_LEFT     75
    #define KB_RIGHT    77
    #define KB_DOWN     80
    #define KB_ENTER    20
    #define KB_TAB      15

    #define DOS_ISR __interrupt __far

    #define PIT_TIMER    0x08
    #define PIT_KEYBOARD 0x09


    void (DOS_ISR *old_timerISR)();
    void (DOS_ISR *old_keyISR)();

    bool keyState[128];

    void setVideoMode();
    #pragma aux setVideoMode  = \
          "mov ax,13h"        \
          "int 10h";

    void setTextMode();
    #pragma aux setTextMode = \
        "mov ax,03h"          \
        "int 10h";

    void osSetPalette(const uint16* palette)
    {
        outp(0x03C8, 0);
        for (int32 i = 0; i < 256; i++)
        {
            uint16 c = *palette++;
            outp(0x03C9, (c & 0x1F) << 1);
            outp(0x03C9, ((c >> 5) & 0x1F) << 1);
            outp(0x03C9, ((c >> 10) & 0x1F) << 1);
        }
    }

    void DOS_ISR timerISR()
    {
        frameIndex++;

        outp(0x20, 0x20);
    }

    void videoAcquire()
    {
        setVideoMode();

        old_timerISR = _dos_getvect(PIT_TIMER);
        _dos_setvect(PIT_TIMER, timerISR);

        uint32 divisor = 1193182 / 60;
        outp(0x43, 0x36);
        outp(0x40, divisor & 0xFF);
        outp(0x40, divisor >> 8);
    }

    void videoRelease()
    {
        _dos_setvect(PIT_TIMER, old_timerISR);
        setTextMode();
    }

    void waitVBlank()
    {
        while  ((inp(0x03DA) & 0x08));
        while (!(inp(0x03DA) & 0x08));
    }

    void blit()
    {
        memcpy((uint8*)0xA0000, fb, VRAM_WIDTH * FRAME_HEIGHT * 2);
    }

    void DOS_ISR keyISR()
    {
        uint32 scancode = inp(0x60);

        if (scancode != 0xE0) {
            keyState[scancode & 0x7F] = ((scancode & 0x80) == 0);
        }

        outp(0x20, 0x20);
    }

    void inputAcquire()
    {
        old_keyISR = _dos_getvect(PIT_KEYBOARD);
        _dos_setvect(PIT_KEYBOARD, keyISR);
    }

    void inputRelease()
    {
        _dos_setvect(PIT_KEYBOARD, old_keyISR);
    }

    void inputUpdate()
    {
        keys = 0;
        if (keyState[KB_UP])      keys |= IK_UP;
        if (keyState[KB_RIGHT])   keys |= IK_RIGHT;
        if (keyState[KB_DOWN])    keys |= IK_DOWN;
        if (keyState[KB_LEFT])    keys |= IK_LEFT;
        if (keyState[KB_X])       keys |= IK_A;
        if (keyState[KB_Z])       keys |= IK_B;
        if (keyState[KB_A])       keys |= IK_L;
        if (keyState[KB_S])       keys |= IK_R;
        if (keyState[KB_ENTER])   keys |= IK_START;
        if (keyState[KB_TAB])     keys |= IK_SELECT;
    }

    int32 osGetSystemTimeMS()
    {
        return 0;
    }

    bool osSaveSettings()
    {
        return false;
    }

    bool osLoadSettings()
    {
        return false;
    }

    bool osCheckSave()
    {
        return false;
    }

    bool osSaveGame()
    {
        return false;
    }

    bool osLoadGame()
    {
        return false;
    }

    void osJoyVibrate(int32 index, int32 L, int32 R) {}
#endif

//#ifdef __GBA__
//    uint8* soundBuffer = (uint8*)MEM_VRAM + FRAME_WIDTH * FRAME_HEIGHT; // use 2k of VRAM after the first frame buffer as sound buffer
//#else
    EWRAM_DATA ALIGN16 uint8 soundBuffer[2 * SND_SAMPLES + 32]; // 32 bytes of silence for DMA overrun while interrupt
//#endif

EWRAM_DATA uint32 curSoundBuffer = 0;

#if defined(_WIN32)
HWAVEOUT waveOut;
WAVEFORMATEX waveFmt = { WAVE_FORMAT_PCM, 1, SND_OUTPUT_FREQ, SND_OUTPUT_FREQ, 1, 8, sizeof(waveFmt) };
WAVEHDR waveBuf[2];

void soundInit()
{
    sndInit();

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

void soundFill()
{
    WAVEHDR *waveHdr = waveBuf + curSoundBuffer;
    waveOutUnprepareHeader(waveOut, waveHdr, sizeof(WAVEHDR));
    sndFill((uint8*)waveHdr->lpData, SND_SAMPLES);
    waveOutPrepareHeader(waveOut, waveHdr, sizeof(WAVEHDR));
    waveOutWrite(waveOut, waveHdr, sizeof(WAVEHDR));
    curSoundBuffer ^= 1;
}
#elif defined(__GBA__)
void soundInit()
{
    sndInit();

    REG_SOUNDCNT_X = SSTAT_ENABLE;
    REG_SOUNDCNT_H = SDS_ATMR0 | SDS_AL | SDS_AR | SDS_ARESET | SDS_A100;
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

    sndFill(soundBuffer + curSoundBuffer * SND_SAMPLES, SND_SAMPLES);
    curSoundBuffer ^= 1;
}
#endif

#if defined(_WIN32)
HDC hDC;

void blit()
{
    for (int i = 0; i < FRAME_WIDTH * FRAME_HEIGHT; i++)
    {
        uint16 c = MEM_PAL_BG[((uint8*)fb)[i]];
        SCREEN[i] = (((c << 3) & 0xFF) << 16) | ((((c >> 5) << 3) & 0xFF) << 8) | ((c >> 10 << 3) & 0xFF) | 0xFF000000;
    }
    const BITMAPINFO bmi = { sizeof(BITMAPINFOHEADER), FRAME_WIDTH, -FRAME_HEIGHT, 1, 32, BI_RGB, 0, 0, 0, 0, 0 };
    StretchDIBits(hDC, 0, 0, WND_WIDTH, WND_HEIGHT, 0, 0, FRAME_WIDTH, FRAME_HEIGHT, SCREEN, &bmi, DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK wndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_ACTIVATE:
        {
            keys = 0;
            break;
        }

        case WM_DESTROY :
        {
            PostQuitMessage(0);
            break;
        }

        case WM_KEYDOWN    :
        case WM_KEYUP      :
        case WM_SYSKEYUP   :
        case WM_SYSKEYDOWN :
        {
            InputKey key = IK_NONE;
            switch (wParam) {
                case VK_UP     : key = IK_UP;     break;
                case VK_RIGHT  : key = IK_RIGHT;  break;
                case VK_DOWN   : key = IK_DOWN;   break;
                case VK_LEFT   : key = IK_LEFT;   break;
                case 'A'       : key = IK_B;      break;
                case 'S'       : key = IK_A;      break;
                case 'Q'       : key = IK_L;      break;
                case 'W'       : key = IK_R;      break;
                case VK_RETURN : key = IK_START;  break;
                case VK_SPACE  : key = IK_SELECT; break;
            }

            if (wParam == '1') players[0]->extraL->goalWeapon = WEAPON_PISTOLS;
            if (wParam == '2') players[0]->extraL->goalWeapon = WEAPON_MAGNUMS;
            if (wParam == '3') players[0]->extraL->goalWeapon = WEAPON_UZIS;
            if (wParam == '4') players[0]->extraL->goalWeapon = WEAPON_SHOTGUN;

            if (msg != WM_KEYUP && msg != WM_SYSKEYUP) {
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

void* osLoadLevel(const char* name)
{
    sndStop();

#if defined(_WIN32) || defined(__TNS__) || defined(__DOS__)
    {
    // level1
        char buf[32];

        delete[] levelData;

        #if defined(_WIN32) || defined(__DOS__)
            sprintf(buf, "data/%s.PKD", name);
        #elif defined(__TNS__)
            sprintf(buf, "/documents/OpenLara/%s.PKD.tns", name);
        #else
            #error
        #endif

        FILE *f = fopen(buf, "rb");

        if (!f)
            return NULL;

        {
            fseek(f, 0, SEEK_END);
            int32 size = ftell(f);
            fseek(f, 0, SEEK_SET);
            uint8* data = new uint8[size];
            fread(data, 1, size, f);
            fclose(f);

            levelData = data;
        }

    // track 13
        #if defined(_WIN32) || defined(__DOS__)
        if (!TRACKS_IMA)
        {
            FILE *f = fopen("data/TRACKS.IMA", "rb");
            if (!f)
                return NULL;

            fseek(f, 0, SEEK_END);
            int32 size = ftell(f);
            fseek(f, 0, SEEK_SET);
            uint8* data = new uint8[size];
            fread(data, 1, size, f);
            fclose(f);

            TRACKS_IMA = data;
        }

        if (!TITLE_SCR)
        {
            FILE *f = fopen("data/TITLE.SCR", "rb");
            if (!f)
                return NULL;

            fseek(f, 0, SEEK_END);
            int32 size = ftell(f);
            fseek(f, 0, SEEK_SET);
            uint8* data = new uint8[size];
            fread(data, 1, size, f);
            fclose(f);

            TITLE_SCR = data;
        }
        #endif
    }
    
    return (void*)levelData;
#elif defined(__GBA__)
    for (int32 i = 0; i < LVL_MAX; i++)
    {
        if (strcmp(name, gLevelInfo[i].name) == 0)
            return (void*)gLevelInfo[i].data;
    }

    gLevelID = LVL_TR1_TITLE;
    return (void*)gLevelInfo[gLevelID].data;
#endif
}

int main(void) {
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

    gameInit(gLevelInfo[gLevelID].name);

    MSG msg;

    int32 startTime = GetTickCount() - 33;
    int32 lastFrame = 0;

    do {
        if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
            int32 frame = (GetTickCount() - startTime) / 33;
            if (GetAsyncKeyState('R')) frame /= 10;

            int32 count = frame - lastFrame;
            if (GetAsyncKeyState('T')) count *= 10;
            gameUpdate(count);
            lastFrame = frame;

            gameRender();

            blit();
        }
    } while (msg.message != WM_QUIT);

#elif defined(__GBA__)
    if (intptr_t(divTable) != MEM_EWRAM) return 0;
    if (intptr_t(lightmap) != MEM_IWRAM) return 0;

    irq_init(NULL);
    irq_add(II_VBLANK, vblank);
    irq_enable(II_VBLANK);

    // Undocumented - Green Swap trick for fake-AA feature (thanks to GValiente)
    // https://mgba-emu.github.io/gbatek/#4000002h---undocumented---green-swap-rw
    //uint16 &GreenSwap = *(uint16*)0x4000002;
    //GreenSwap = 1;

    // set low latency mode via WAITCNT register (thanks to GValiente)
    REG_WSCNT = WS_ROM0_N2 | WS_ROM0_S1 | WS_PREFETCH; // fast ROM
    
    // Undocumented - Internal Memory Control (R/W)
    #define MEM_CHECK_MAGIC 14021968
    vu32& fastAccessReg = *(vu32*)(REG_BASE+0x0800);

    fastAccessReg = 0x0E000020; // fast EWRAM

    vu32* fastAccessMem = (vu32*)soundBuffer; // check EWRAM access
    // write
    for (int32 i = 0; i < 16; i++)
    {
        fastAccessMem[i] = MEM_CHECK_MAGIC + i;
    }
    // read
    for (int32 i = 0; i < 16; i++)
    {
        if (fastAccessMem[i] != vu32(MEM_CHECK_MAGIC + i))
        {
            fastAccessReg = 0x0D000020; // device doesn't support this feature, revert reg value
        }
    }

    rumbleInit();
    soundInit();

    gameInit(gLevelInfo[gLevelID].name);

    uint16 mode = DCNT_BG2 | DCNT_PAGE;

    mode |= DCNT_MODE4;
    REG_BG2PA = (1 << 8);
    REG_BG2PD = (1 << 8);

    int32 lastFrameIndex = -1;

    while (1)
    {
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
        int32 delta = frame - lastFrameIndex;

        if (!delta) {
            continue;
        }

        rumbleUpdate(delta);

        lastFrameIndex = frame;

        gameUpdate(delta);

        #ifdef PROFILING
            VBlankIntrWait();
        #endif
        REG_DISPCNT = (mode ^= DCNT_PAGE);
        fb ^= 0xA000;

        gameRender();

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

    gameInit(gLevelInfo[gLevelID].name);

    int startTime = GetTickCount();
    int lastTime = -16;
    int fpsTime = startTime;

    while (1)
    {
        inputUpdate();

        if (keyDown(KEY_NSPIRE_ESC))
        {
            break;
        }

        int time = GetTickCount() - startTime;
        gameUpdate((time - lastTime) / 16);
        lastTime = time;

        gameRender();

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
#elif defined(__DOS__)
    videoAcquire();
    inputAcquire();

    gameInit(gLevelInfo[gLevelID].name);

    int32 lastFrameIndex = -1;

    //int extraFrame = 0;

    while (1)
    {
        inputUpdate();

        if (keyState[KB_ESC])
            break;

        int32 frame = frameIndex / 2;
        gameUpdate(frame - lastFrameIndex);
        lastFrameIndex = frame;

        gameRender();

        fpsCounter++;
        if (frameIndex >= 60) {
            frameIndex -= 60;
            lastFrameIndex -= 30;

            fps = fpsCounter;

            fpsCounter = 0;
        }

        blit();
    }

    inputRelease();
    videoRelease();
#endif
    return 0;
}
