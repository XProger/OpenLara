const void* TRACKS_IMA;
void* levelData;

#include "game.h"

Game game;

Item irqVBL;
Item irqVRAM;
Item irqTimer;

ScreenContext screen;
int32 screenPage;
Item  screenItem;
int32 fps;
int32 g_timer;

const int chars[10][20] = {
    {
        0,1,1,0,
        1,0,0,1,
        1,0,0,1,
        1,0,0,1,
        0,1,1,0
    }, {
        0,0,1,0,
        0,1,1,0,
        0,0,1,0,
        0,0,1,0,
        0,0,1,0
    }, {
        0,1,1,0,
        1,0,0,1,
        0,0,1,1,
        0,1,0,0,
        1,1,1,1
    }, {
        1,1,1,0,
        0,0,0,1,
        0,0,1,0,
        1,0,0,1,
        0,1,1,0
    }, {
        0,0,0,1,
        0,0,1,1,
        0,1,0,1,
        1,1,1,1,
        0,0,0,1
    }, {
        1,1,1,1,
        1,0,0,0,
        1,1,1,0,
        0,0,0,1,
        1,1,1,0
    }, {
        0,1,1,0,
        1,0,0,0,
        1,1,1,0,
        1,0,0,1,
        0,1,1,0
    }, {
        1,1,1,1,
        0,0,0,1,
        0,0,1,0,
        0,1,0,0,
        0,1,0,0
    }, {
        0,1,1,0,
        1,0,0,1,
        0,1,1,0,
        1,0,0,1,
        0,1,1,0
    }, {
        0,1,1,0,
        1,0,0,1,
        0,1,1,1,
        0,0,0,1,
        0,1,1,0
    }
};

void drawChar(int32 x, int32 y, int32 c)
{
    uint16* ptr = (uint16*)screen.sc_Bitmaps[screenPage]->bm_Buffer;

    ptr += y / 2 * 2 * FRAME_WIDTH;
    ptr += x * 2;
    ptr += y & 1;

    for (int32 i = 0; i < 5; i++)
    {
        for (int32 j = 0; j < 4; j++)
        {
            if (chars[c][i * 4 + j])
            {
                ptr[j * 2] = 0xFFFF;
            }
        }

        if ((y + i) & 1) {
            ptr += FRAME_WIDTH * 2 - 1;
        } else {
            ptr += 1;
        }
    }
}

void drawInt(int32 x, int32 y, int32 number)
{
    while (number)
    {
        drawChar(x, y, number % 10);
        number /= 10;
        x -= 6;
    }
}

void osSetPalette(const uint16* palette)
{
    //
}

int32 osGetSystemTimeMS()
{
    return GetMSecTime(irqTimer);
}

void osJoyVibrate(int32 index, int32 L, int32 R)
{
    //
}

IOInfo clearInfo = {
    FLASHWRITE_CMD, IO_QUICK, 0, 0, -1, 0, 0,
    { NULL, 0 },
    { NULL, FRAME_WIDTH * FRAME_HEIGHT * 2 }
};

X_INLINE void clearFast(int32 page)
{
    clearInfo.ioi_Recv.iob_Buffer = screen.sc_Bitmaps[page]->bm_Buffer;
    SendIO(irqVRAM, &clearInfo);
}

uint32 joyMask;

void updateInput()
{
    ControlPadEventData event;
    event.cped_ButtonBits = 0;
    if (GetControlPad(1, 0, &event)) {
        joyMask = event.cped_ButtonBits;
    }

    keys = 0;

    if (joyMask & ControlUp)         keys |= IK_UP;
    if (joyMask & ControlRight)      keys |= IK_RIGHT;
    if (joyMask & ControlDown)       keys |= IK_DOWN;
    if (joyMask & ControlLeft)       keys |= IK_LEFT;
    if (joyMask & ControlA)          keys |= IK_A;
    if (joyMask & ControlB)          keys |= IK_B;
    if (joyMask & ControlC)          keys |= IK_C;
    if (joyMask & ControlLeftShift)  keys |= IK_L;
    if (joyMask & ControlRightShift) keys |= IK_R;
    if (joyMask & ControlStart)      keys |= IK_START;
    if (joyMask & ControlX)          keys |= IK_SELECT;
}

#include <filefunctions.h>
void* readFile(char* fileName, void* buffer, int32 bufferSize)
{
    printf("readFile %s\n", fileName);

    Item f = OpenDiskFile(fileName);

    if (f < 0) {
        printf("failed to open file %s\n", fileName);
        return NULL;
    }

    Item req;
    IOReq* reqp;
    IOInfo params;
    DeviceStatus ds;

    req = CreateIOReq(NULL, 0, f, 0);
    reqp = (IOReq*)LookupItem(req);
    memset(&params, 0, sizeof(IOInfo));
    memset(&ds, 0, sizeof(DeviceStatus));
    params.ioi_Command = CMD_STATUS;
    params.ioi_Recv.iob_Buffer = &ds;
    params.ioi_Recv.iob_Len = sizeof(DeviceStatus);

    void* ptr = buffer;

    if (DoIO(req, &params) >= 0)
    {
        int32 size = ds.ds_DeviceBlockCount * ds.ds_DeviceBlockSize;
        if (ptr) {
            if (bufferSize < size)
            {
                printf("not enough buffer size %d bytes required!\n", size);
                ptr = NULL;
            }
        } else {
            ptr = malloc(size);
        }

        if (ptr != NULL)
        {
            memset(&params, 0, sizeof(IOInfo));
            params.ioi_Command = CMD_READ;
            params.ioi_Recv.iob_Len = size;
            params.ioi_Recv.iob_Buffer = ptr;

            if (DoIO(req, &params) < 0)
            {
                printf("can't get file content!\n");
                if (!buffer) {
                    free(ptr);
                }
                ptr = NULL;
            }
        } else {
            printf("can't allocate %d bytes!\n", size);
        }
    }

    DeleteIOReq(req);
    CloseDiskFile(f);

    return ptr;
}

void soundInit()
{
    // TODO
}

void checkRAM()
{
    for (int32 i = 1; i < 2 * 1024 * 1024; i += 16 * 1024)
    {
        void* ptr = malloc(i);
        if (ptr == NULL) {
            printf("RAM: %d kb\n", i / 1024 - 16);
            break;
        }
        free(ptr);
    }

    MemInfo memInfoVRAM;
    AvailMem(&memInfoVRAM, MEMTYPE_VRAM);
    printf("VRAM: %d\n", memInfoVRAM.minfo_SysFree);
}

int32 gLevelID = 2;

static const char* gLevelNames[] = {
    "GYM",
    "LEVEL1",
    "LEVEL2",
//    "LEVEL3A",
//    "LEVEL3B",
//    "LEVEL4",
//    "LEVEL5",
//    "LEVEL6",
//    "LEVEL7A",
//    "LEVEL7B",
//    "LEVEL8A",
//    "LEVEL8B",
//    "LEVEL8C",
//    "LEVEL10A",
//    "LEVEL10B",
//    "LEVEL10C"
};

void osLoadLevel(const char* name)
{
    char buf[32];

    sprintf(buf, "data/%s.3DO", name);
    readFile(buf, levelData, MAX_RAM);

    sprintf(buf, "data/%s.TEX", name);
    readFile(buf, VRAM_TEX, MAX_VRAM);
}

int main(int argc, char *argv[])
{
    printf("OpenLara 3DO\n");

    levelData = AllocMem(MAX_RAM, MEMTYPE_DMA);

    //checkRAM();

    uint32 lastFrame;
    uint32 frame;
    uint32 lastSec = 0;
    uint32 frameIndex = 0;

    OpenMathFolio();
    OpenGraphicsFolio();
    OpenAudioFolio();
    InitEventUtility(1, 0, LC_Observer);
    CreateBasicDisplay(&screen, DI_TYPE_NTSC, 2);

    for (int32 i = 0; i < 2; i++)
    {
        SetCEControl(screen.sc_BitmapItems[i], 0xFFFFFFFF, ASCALL);
        //DisableHAVG(screen.sc_BitmapItems[i]);
        //DisableVAVG(screen.sc_BitmapItems[i]);
    }

    irqVBL = GetVBLIOReq();
    irqVRAM = GetVRAMIOReq();
    irqTimer = GetTimerIOReq();

    soundInit();

    game.init(gLevelNames[gLevelID]);

    //checkRAM();

    GetVBLTime(irqVBL, NULL, &lastFrame);
    lastFrame /= 2;
    lastFrame--;

    while (1)
    {
        WaitVBL(irqVBL, 1);
        clearFast(screenPage);

        uint32 oldKeys = keys;

        updateInput();

        //GetVBLTime(irqVBL, NULL, &frame); // slower
        QueryGraphics(QUERYGRAF_TAG_FIELDCOUNT, &frame);

        if (frame/60 != lastSec) {
            lastSec = frame/60;
            fps = frameIndex;
            frameIndex = 0;
        }

        frame /= 2; // 30 Hz

        if ((keys & IK_SELECT) && !(oldKeys & IK_SELECT))
        {
            gLevelID = (gLevelID + 1) % (sizeof(gLevelNames) / sizeof(gLevelNames[0]));
            game.startLevel(gLevelNames[gLevelID]);
            lastFrame = frame - 1;
        }

int32 updateTime = osGetSystemTimeMS();
        game.update(frame - lastFrame);
updateTime = osGetSystemTimeMS() - updateTime;

        lastFrame = frame;

        screenItem = screen.sc_BitmapItems[screenPage];

int32 renderTime = osGetSystemTimeMS();
        game.render();
renderTime = osGetSystemTimeMS() - renderTime;

        drawInt(FRAME_WIDTH - 8, 4, fps);
        drawInt(FRAME_WIDTH - 8, 4 + 8, updateTime);
        drawInt(FRAME_WIDTH - 8, 4 + 16, renderTime);

        DisplayScreen(screen.sc_Screens[screenPage], NULL);
        screenPage ^= 1;

        frameIndex++;
    }

    return 0;
}
