const void* TRACKS_IMA;
const void* TITLE_SCR;
unsigned char* levelData;

#include "game.h"

int32 fps;
int32 frameIndex = 0;
int32 fpsCounter = 0;
uint32 curSoundBuffer = 0;

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
        if (ty < -0.5f) keys |= IK_DOWN;
    }

    if (keyDown(KEY_NSPIRE_2)) keys |= IK_A;
    if (keyDown(KEY_NSPIRE_3)) keys |= IK_B;
    if (keyDown(KEY_NSPIRE_7)) keys |= IK_L;
    if (keyDown(KEY_NSPIRE_9)) keys |= IK_R;
    if (keyDown(KEY_NSPIRE_ENTER)) keys |= IK_START;
    if (keyDown(KEY_NSPIRE_SPACE)) keys |= IK_SELECT;
}

void* osLoadLevel(const char* name)
{
// level1
    char buf[32];

    delete[] levelData;

    sprintf(buf, "/documents/OpenLara/%s.PKD.tns", name);

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

    return (void*)levelData;
}

int main(void)
{
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

    return 0;
}
