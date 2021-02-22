#include "common.h"
#include "level.h"
#include "camera.h"

#if defined(_WIN32)
    uint8* LEVEL1_PHD;

    uint32 SCREEN[WIDTH * HEIGHT];

    extern uint16 fb[WIDTH * HEIGHT];

    LARGE_INTEGER g_timer;
    LARGE_INTEGER g_current;

    #define WND_SCALE 4
#elif defined(__GBA__)
    #include "LEVEL1_PHD.h"

    extern uint32 fb;
#elif defined(__TNS__)
    uint8* LEVEL1_PHD;

    extern uint16 fb[WIDTH * HEIGHT];

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

bool keys[IK_MAX] = {};

int32 fps;
int32 frameIndex = 0;
int32 fpsCounter = 0;

#ifdef PROFILE
    uint32 dbg_transform;
    uint32 dbg_poly;
    uint32 dbg_sort;
    uint32 dbg_flush;
    uint32 dbg_vert_count;
    uint32 dbg_poly_count;
#endif

void update(int32 frames) {
    for (int32 i = 0; i < frames; i++) {
        updateItems();
        camera.update();
    }
}

#ifdef TEST
void faceAddQuad(uint32 flags, const Index* indices, int32 startVertex);

extern Vertex gVertices[MAX_VERTICES];

INLINE int32 classify(const Vertex* v) {
    return (v->x < clip.x0 ? 1 : 0) |
           (v->x > clip.x1 ? 2 : 0) |
           (v->y < clip.y0 ? 4 : 0) |
           (v->y > clip.y1 ? 8 : 0);
}

void drawTest() {
    static Rect testClip = { 0, 0, FRAME_WIDTH, FRAME_HEIGHT };
    static int32 testTile = 707; // 712

#ifdef _WIN32
    Sleep(16);

    int dx = 0;
    int dy = 0;

    if (GetAsyncKeyState(VK_LEFT)) dx--;
    if (GetAsyncKeyState(VK_RIGHT)) dx++;
    if (GetAsyncKeyState(VK_UP)) dy--;
    if (GetAsyncKeyState(VK_DOWN)) dy++;

    if (GetAsyncKeyState('T')) {
        testClip.x0 += dx;
        testClip.y0 += dy;
    }

    if (GetAsyncKeyState('B')) {
        testClip.x1 += dx;
        testClip.y1 += dy;
    }

    if (GetAsyncKeyState('U')) {
        testTile += dx;
        if (testTile < 0) testTile = 0;
        if (testTile >= texturesCount) testTile = texturesCount - 1;
    }
#endif

    clip = testClip;

    gVertices[0].x = 50 + 50;
    gVertices[0].y = 50;

    gVertices[1].x = FRAME_WIDTH - 50 - 50;
    gVertices[1].y = 50;

    gVertices[2].x = FRAME_WIDTH - 50;
    gVertices[2].y = FRAME_HEIGHT - 50;

    gVertices[3].x = 50;
    gVertices[3].y = FRAME_HEIGHT - 50;

    for (int i = 0; i < 4; i++) {
        gVertices[i].z = 100;
        gVertices[i].g = 128;
        gVertices[i].clip = classify(gVertices + i);
    }
    gVerticesCount = 4;

    Index indices[] = { 0, 1, 2, 3, 0, 2, 3 };

    faceAddQuad(testTile, indices, 0);

#ifdef _WIN32
    for (int y = 0; y < FRAME_HEIGHT; y++) {
        for (int x = 0; x < FRAME_WIDTH; x++) {
            if (x == clip.x0 || x == clip.x1 - 1 || y == clip.y0 || y == clip.y1 - 1)
                fb[y * FRAME_WIDTH + x] = 255;
        }
    }
#endif

    flush();
}
#endif

void render() {
    clear();

    #ifdef TEST
        #ifdef __GBA__
            VBlankIntrWait();
        #endif

        int32 cycles = 0;
        PROFILE_START();
        drawTest();
        PROFILE_STOP(cycles);

        drawNumber(cycles, FRAME_WIDTH, 32);
    #else
        #ifdef PROFILE
            dbg_transform = 0;
            dbg_poly = 0;
            dbg_sort = 0;
            dbg_flush = 0;
            dbg_vert_count = 0;
            dbg_poly_count = 0;
        #endif

        drawRooms();

        #ifdef PROFILE
            drawNumber(dbg_transform, FRAME_WIDTH, 32);
            drawNumber(dbg_poly, FRAME_WIDTH, 48);
            drawNumber(dbg_sort, FRAME_WIDTH, 64);
            drawNumber(dbg_flush, FRAME_WIDTH, 80);
            drawNumber(dbg_vert_count, FRAME_WIDTH, 96);
            drawNumber(dbg_poly_count, FRAME_WIDTH, 112);
        #endif

    #endif

    drawNumber(fps, FRAME_WIDTH, 16);
}

#if defined(_WIN32)
HDC hDC;

void blit() {
    for (int i = 0; i < WIDTH * HEIGHT; i++) {
        uint16 c = ((uint16*)fb)[i];
        SCREEN[i] = (((c << 3) & 0xFF) << 16) | ((((c >> 5) << 3) & 0xFF) << 8) | ((c >> 10 << 3) & 0xFF) | 0xFF000000;
    }

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
    #define REG_WAITCNT_NV  *(u16*)(REG_BASE + 0x0204)
    REG_WAITCNT_NV |= (0x0008 | 0x0010 | 0x4000);
#endif

    initLUT();

    readLevel(LEVEL1_PHD);

#if defined(_WIN32)
    RECT r = { 0, 0, 240 * WND_SCALE, 160 * WND_SCALE };

    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, false);
    int wx = (GetSystemMetrics(SM_CXSCREEN) - (r.right - r.left)) / 2;
    int wy = (GetSystemMetrics(SM_CYSCREEN) - (r.bottom - r.top)) / 2;

    HWND hWnd = CreateWindow("static", "OpenLara GBA", WS_OVERLAPPEDWINDOW, wx + r.left, wy + r.top, r.right - r.left, r.bottom - r.top, 0, 0, 0, 0);
    hDC = GetDC(hWnd);

    SetWindowLong(hWnd, GWL_WNDPROC, (LONG)&wndProc);
    ShowWindow(hWnd, SW_SHOWDEFAULT);

    MSG msg;

    int startTime = GetTickCount() - 33;
    int lastFrame = 0;

    do {
        if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {

            int frame = (GetTickCount() - startTime) / 33; 
            update(frame - lastFrame);
            lastFrame = frame;

            render();

            blit();
        }
    } while (msg.message != WM_QUIT);

#elif defined(__GBA__)
    irqInit();
    irqSet(IRQ_VBLANK, vblank);
    irqEnable(IRQ_VBLANK);

    uint16 mode = BG2_ON | BACKBUFFER;

    mode |= MODE_5;

    REG_BG2PA = 256 - 64 - 16 - 4 - 1;
    REG_BG2PD = 256 - 48 - 2;

    int32 lastFrameIndex = -1;

    while (1) {
        //VBlankIntrWait();

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

        int32 frame = frameIndex / 2;
        update(frame - lastFrameIndex);
        lastFrameIndex = frame;

        render();

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
        inputUpdate();

        if (keyDown(KEY_NSPIRE_ESC))
        {
            break;
        }

        if (touchInfo && touchReport.contact)
        {
            float tx = float(touchReport.x) / float(touchInfo->width)  * 2.0f - 1.0f;
            float ty = float(touchReport.y) / float(touchInfo->height) * 2.0f - 1.0f;

            keys[IK_LEFT]  = tx < -0.5f;
            keys[IK_RIGHT] = tx >  0.5f;
            keys[IK_UP]    = ty >  0.5f;
            keys[IK_DOWN]  = ty < -0.5f;
        } else {
            keys[IK_LEFT]  =
            keys[IK_RIGHT] =
            keys[IK_UP]    =
            keys[IK_DOWN]  = false;
        }

        keys[IK_A] = keyDown(KEY_NSPIRE_2);
        keys[IK_B] = keyDown(KEY_NSPIRE_3);
        keys[IK_L] = keyDown(KEY_NSPIRE_7);
        keys[IK_R] = keyDown(KEY_NSPIRE_9);

        int time = GetTickCount() - startTime;
        update((time - lastTime) / 16);
        lastTime = time;

        render();

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
