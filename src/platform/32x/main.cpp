const void* TRACKS_IMA;
const void* TITLE_SCR;

extern void* LEVEL1_PKD;

#include "game.h"

volatile unsigned mars_frt_ovf_count = 0;

extern "C" {
    volatile uint32 mars_pwdt_ovf_count = 0;
    volatile uint32 mars_swdt_ovf_count = 0;
}

int32 gFrameIndex = 0;

#define SH2_WDT_RTCSR       (*(volatile unsigned char *)0xFFFFFE80)
#define SH2_WDT_RTCNT       (*(volatile unsigned char *)0xFFFFFE81)
#define SH2_WDT_RRSTCSR     (*(volatile unsigned char *)0xFFFFFE83)
#define SH2_WDT_WTCSR_TCNT  (*(volatile unsigned short *)0xFFFFFE80)
#define SH2_WDT_WRWOVF_RST  (*(volatile unsigned short *)0xFFFFFE82)
#define SH2_WDT_VCR         (*(volatile unsigned short *)0xFFFFFEE4)

int32 g_timer;
int32 fps;

void osSetPalette(const uint16* palette)
{
    void *dst = (void*)&MARS_CRAM;
    memcpy(dst, palette, 256 * 2);
}

int32 osGetSystemTimeMS()
{
    return int32((mars_pwdt_ovf_count << 8) | SH2_WDT_RTCNT);
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

const void* osLoadScreen(LevelID id)
{
    return TITLE_SCR;
}

const void* osLoadLevel(LevelID id)
{
    return (void*)LEVEL1_PKD; // TODO
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

void pageClear()
{
    dmaFill((uint8*)&MARS_FRAMEBUFFER + 0x200, 0, FRAME_WIDTH * FRAME_HEIGHT);
}

extern "C" void pri_vbi_handler()
{
    gFrameIndex++;
}

extern void flush_ot(int32 bit);

extern "C" void secondary()
{
    // init DMA
    SH2_DMA_SAR0 = 0;
    SH2_DMA_DAR0 = 0;
    SH2_DMA_TCR0 = 0;
    SH2_DMA_CHCR0 = 0;
    SH2_DMA_DRCR0 = 0;
    SH2_DMA_SAR1 = 0;
    SH2_DMA_DAR1 = 0;
    SH2_DMA_TCR1 = 0;
    SH2_DMA_CHCR1 = 0;
    SH2_DMA_DRCR1 = 0;
    SH2_DMA_DMAOR = 1;  // enable DMA

    SH2_DMA_VCR1 = 66;  // set exception vector for DMA channel 1
    SH2_INT_IPRA = (SH2_INT_IPRA & 0xF0FF) | 0x0400;    // set DMA INT to priority 4

    while (1)
    {
        int cmd;
        while ((cmd = MARS_SYS_COMM4) == 0);

        switch (cmd)
        {            
            case MARS_CMD_CLEAR:
                pageClear();
                break;
            case MARS_CMD_FLUSH:
                flush_ot(1);
                break;
        }

        MARS_SYS_COMM4 = 0;
    }
}

extern "C" void sec_dma1_handler()
{
    SH2_DMA_CHCR1; // read TE
    SH2_DMA_CHCR1 = 0; // clear TE
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

        for (int32 i = 0; i < 256; i++)
        {
            lineTable[i] = wordOffset;

            if (i < FRAME_HEIGHT - 1) {
                wordOffset += FRAME_WIDTH / 2;
            }
        }

        pageClear();
    }

    SH2_WDT_VCR = (65<<8) | (SH2_WDT_VCR & 0x00FF); // set exception vector for WDT
    SH2_INT_IPRA = (SH2_INT_IPRA & 0xFF0F) | 0x0020; // set WDT INT to priority 2

    SH2_WDT_WTCSR_TCNT = 0x5A00;
    SH2_WDT_WTCSR_TCNT = 0xA53E;

    MARS_SYS_COMM4 = 0;

    gameInit();

    int32 lastFrame = (gFrameIndex >> 1) - 1;
    int32 fpsCounter = 0;
    int32 fpsFrame = gFrameIndex;
    int32 vsyncRate = (MARS_VDP_DISPMODE & MARS_NTSC_FORMAT) ? 60 : 50;

    while (1)
    {
        int32 frame = *(volatile int32 *)&gFrameIndex;

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

