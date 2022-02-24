const void* TRACKS_IMA;
const void* TITLE_SCR;

extern void* LEVEL1_PKD;

#include "game.h"

int32 fps;

void osSetPalette(const uint16* palette)
{
    void *dst = (void*)&MARS_CRAM;
    memcpy(dst, palette, 256 * 2);
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

void osJoyVibrate(int32 index, int32 L, int32 R)
{
    // nope
}

void updateInput()
{
    keys = 0;

    uint16 mask = MARS_SYS_COMM8;
    if (mask & SEGA_CTRL_UP)      keys |= IK_UP;
    if (mask & SEGA_CTRL_RIGHT)   keys |= IK_RIGHT;
    if (mask & SEGA_CTRL_DOWN)    keys |= IK_DOWN;
    if (mask & SEGA_CTRL_LEFT)    keys |= IK_LEFT;
    if (mask & SEGA_CTRL_A)       keys |= IK_A;
    if (mask & SEGA_CTRL_B)       keys |= IK_B;
    if (mask & SEGA_CTRL_C)       keys |= IK_C;
    if (mask & SEGA_CTRL_X)       keys |= IK_X;
    if (mask & SEGA_CTRL_Y)       keys |= IK_Y;
    if (mask & SEGA_CTRL_Z)       keys |= IK_Z;
    if (mask & SEGA_CTRL_START)   keys |= IK_START;
    if (mask & SEGA_CTRL_MODE)    keys |= IK_SELECT;
}

void* osLoadLevel(const char* name)
{
    return (void*)LEVEL1_PKD;
}

uint16 pageIndex = 0;

void pageWait()
{
    while ((MARS_VDP_FBCTL & MARS_VDP_FS) != pageIndex);
}

void pageFlip()
{
    pageIndex ^= 1;
    MARS_VDP_FBCTL = pageIndex;
}

extern "C" void slave(void)
{
    // TODO
}

int main()
{
    while ((MARS_SYS_INTMSK & MARS_SH2_ACCESS_VDP) == 0);
    MARS_VDP_DISPMODE = MARS_224_LINES | MARS_VDP_MODE_256;

    for (int32 page = 0; page < 2; page++)
    {
        pageFlip();
        pageWait();

        volatile uint16* lineTable = &MARS_FRAMEBUFFER;
        uint16 wordOffset = 0x100;
        for (int32 i = 0; i < 224; i++, wordOffset += FRAME_WIDTH / 2)
        {
            lineTable[i] = wordOffset;
        }

        uint8* fb = (uint8*)(lineTable + 0x100);
        memset(fb, 0, 0x10000 - 0x200);
    }

    gameInit(gLevelInfo[gLevelID].name);

    int32 lastFrame = (MARS_SYS_COMM12 >> 1) - 1;
    int32 fpsCounter = 0;
    int32 fpsFrame = MARS_SYS_COMM12;
    int32 vsyncRate = (MARS_VDP_DISPMODE & MARS_NTSC_FORMAT) ? 60 : 50;

    while (1)
    {
        int32 frame = MARS_SYS_COMM12;

        if (frame - fpsFrame >= vsyncRate)
        {
            fpsFrame += vsyncRate;
            fps = fpsCounter;
            fpsCounter = 0;
        }

        if (vsyncRate == 50)
        {
            frame = frame * 6 / 5;  // PAL fix
        }

        frame >>= 1;

        int32 delta = frame - lastFrame;

        if (!delta)
            continue;

        lastFrame = frame;

        updateInput();

        gameUpdate(delta);

        pageWait();

        gameRender();

        pageFlip();

        fpsCounter++;
    }

    return 0;
}

