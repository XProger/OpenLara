#include "game.h"

EWRAM_DATA int32 fps;
EWRAM_DATA int32 frameIndex = 0;
EWRAM_DATA int32 fpsCounter = 0;
EWRAM_DATA uint32 curSoundBuffer = 0;

#ifdef __GBA_WIN__
const void* TRACKS_AD4;
const void* TITLE_SCR;
const void* levelData;

HWND hWnd;

LARGE_INTEGER g_timer;
LARGE_INTEGER g_current;

#define WND_WIDTH   240*4
#define WND_HEIGHT  160*4

uint16 MEM_PAL_BG[256];
uint32 SCREEN[FRAME_WIDTH * FRAME_HEIGHT];

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

extern int8 soundBuffer[2 * SND_SAMPLES + 32]; // 32 bytes of silence for DMA overrun while interrupt

HWAVEOUT waveOut;
WAVEFORMATEX waveFmt = { WAVE_FORMAT_PCM, 1, SND_OUTPUT_FREQ, SND_OUTPUT_FREQ, 1, 8, sizeof(waveFmt) };
WAVEHDR waveBuf[2];

void soundInit()
{
    sndInit();

    if (waveOutOpen(&waveOut, WAVE_MAPPER, &waveFmt, (INT_PTR)hWnd, 0, CALLBACK_WINDOW) != MMSYSERR_NOERROR)
        return;

    memset(&waveBuf, 0, sizeof(waveBuf));
    for (int i = 0; i < 2; i++)
    {
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
    sndFill((int8*)waveHdr->lpData);
    waveOutPrepareHeader(waveOut, waveHdr, sizeof(WAVEHDR));
    waveOutWrite(waveOut, waveHdr, sizeof(WAVEHDR));
    curSoundBuffer ^= 1;
}

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

const void* osLoadScreen(LevelID id)
{
    return TITLE_SCR;
}

const void* osLoadLevel(LevelID id)
{
    // level1
    char buf[32];

    delete[] levelData;

    sprintf(buf, "data/%s.PKD", (const char*)gLevelInfo[id].data);

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

// tracks
    if (!TRACKS_AD4)
    {
        FILE *f = fopen("data/TRACKS.AD4", "rb");
        if (!f)
            return NULL;

        fseek(f, 0, SEEK_END);
        int32 size = ftell(f);
        fseek(f, 0, SEEK_SET);
        uint8* data = new uint8[size];
        fread(data, 1, size, f);
        fclose(f);

        TRACKS_AD4 = data;
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
    
    return (void*)levelData;
}

int main(void)
{
    RECT r = { 0, 0, WND_WIDTH, WND_HEIGHT };

    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, false);
    int wx = (GetSystemMetrics(SM_CXSCREEN) - (r.right - r.left)) / 2;
    int wy = (GetSystemMetrics(SM_CYSCREEN) - (r.bottom - r.top)) / 2;

    hWnd = CreateWindow("static", "OpenLara GBA", WS_OVERLAPPEDWINDOW, wx + r.left, wy + r.top, r.right - r.left, r.bottom - r.top, 0, 0, 0, 0);
    hDC = GetDC(hWnd);

    SetWindowLong(hWnd, GWL_WNDPROC, (LONG)&wndProc);
    ShowWindow(hWnd, SW_SHOWDEFAULT);

    soundInit();

    gameInit();

    MSG msg;

    int32 startTime = GetTickCount() - 33;
    int32 lastFrame = 0;

    do {
        if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
        #ifdef _DEBUG
            Sleep(4);
        #endif
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

    return 0;
}
#else
void osSetPalette(const uint16* palette)
{
    memcpy((uint16*)MEM_PAL_BG, palette, 256 * 2);
}

int32 osGetSystemTimeMS()
{
    return 0; // TODO
}

const char* SRAM_ID = "SRAM_Vnnn\0\0";
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

void updateInput()
{
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

extern int8 soundBuffer[2 * SND_SAMPLES + 32];

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
    if (curSoundBuffer) {
        REG_DMA1CNT = 0;
        REG_DMA1SAD = (u32)soundBuffer;
        REG_DMA1CNT = DMA_DST_FIXED | DMA_REPEAT | DMA_16 | DMA_AT_FIFO | DMA_ENABLE;
    }

    sndFill(soundBuffer + curSoundBuffer);
    curSoundBuffer ^= SND_SAMPLES;
}

void vblank()
{
    frameIndex++;
    soundFill();
}

#define MEM_CHECK_MAGIC 14021968

extern bool checkROM(uint32 mask); // should be in IWRAM

const uint32 WSCNT_MASK[] = {
    WS_ROM0_N2 | WS_ROM0_S1 | WS_PREFETCH,
    WS_ROM0_N3 | WS_ROM0_S1 | WS_PREFETCH,
};

void boostROM()
{
    for (int32 i = 0; i < X_COUNT(WSCNT_MASK); i++)
    {
        if (checkROM(WSCNT_MASK[i])) {
            break;
        }
    }
}

void boostEWRAM()
{
    // Undocumented - Internal Memory Control (R/W)
    vu32& fastAccessReg = *(vu32*)(REG_BASE+0x0800);

    fastAccessReg = 0x0E000020; // fast EWRAM

    vu32* fastAccessMem = (vu32*)gSpheres; // check EWRAM access
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
}

const void* osLoadScreen(LevelID id)
{
    return TITLE_SCR;
}

const void* osLoadLevel(LevelID id)
{
    return gLevelInfo[id].data;
}

int main(void)
{
    if (intptr_t(divTable) != MEM_EWRAM) return 0;
    if (intptr_t(gLightmap) != MEM_IWRAM) return 0;

    irq_init(NULL);
    irq_add(II_VBLANK, vblank);
    irq_enable(II_VBLANK);

    boostROM();
    boostEWRAM();

    rumbleInit();
    soundInit();

    gameInit();

    uint16 mode = DCNT_BG2 | DCNT_PAGE | DCNT_MODE4;

    int32 lastFrameIndex = -1;

    while (1)
    {
        updateInput();

        int32 frame = frameIndex / 2;
        int32 delta = frame - lastFrameIndex;

        if (!delta)
            continue;
        lastFrameIndex = frame;

        rumbleUpdate(delta);

        gameUpdate(delta);

        #ifdef PROFILING
            VBlankIntrWait();
        #else
            if (gSettings.video_vsync) {
                VBlankIntrWait();
            }
        #endif

        REG_DISPCNT = (mode ^= DCNT_PAGE);
        fb ^= VRAM_PAGE_SIZE;

        gameRender();

        fpsCounter++;
        if (frameIndex >= 60)
        {
            frameIndex -= 60;
            lastFrameIndex -= 30;

            fps = fpsCounter;

            fpsCounter = 0;
        }
    }

    return 0;
}
#endif
