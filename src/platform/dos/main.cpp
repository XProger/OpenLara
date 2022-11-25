#include "game.h"

EWRAM_DATA int32 fps;
EWRAM_DATA int32 frameIndex = 0;
EWRAM_DATA int32 fpsCounter = 0;
EWRAM_DATA uint32 curSoundBuffer = 0;

const void* TRACKS_IMA;
const void* TITLE_SCR;
const void* levelData;

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
    
    return (void*)levelData;
}

int main(void)
{
    videoAcquire();
    inputAcquire();

    gameInit();

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

    return 0;
}
