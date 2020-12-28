#include <sys/time.h>

#include <game.h>

// multi-threading (no sound - no problem)
void* osMutexInit() { return NULL; }
void osMutexFree(void *obj) {}
void osMutexLock(void *obj) {}
void osMutexUnlock(void *obj) {}


// timing
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

int osGetTimeMS()
{
    return (osTime - *timerCLK) / 33;
}


// input
touchpad_info_t*  touchInfo;
touchpad_report_t touchReport;

void touchInit()
{
    touchInfo = is_touchpad ? touchpad_getinfo() : NULL;
}

bool osJoyReady(int index)
{
    return (index == 0);
}

void osJoyVibrate(int index, float L, float R) {}

bool inputUpdate()
{
    Input::setJoyPos(0, jkL, vec2(0.0f, 0.0f));
    Input::setJoyPos(0, jkR, vec2(0.0f, 0.0f));
    Input::setJoyPos(0, jkLT, vec2(0.0f, 0.0f));
    Input::setJoyPos(0, jkRT, vec2(0.0f, 0.0f));
    
    if (touchInfo)
    {
        touchpad_scan(&touchReport);
        if (touchReport.contact)
        {
            float tx = float(touchReport.x) / float(touchInfo->width)  * 2.0f - 1.0f;
            float ty = float(touchReport.y) / float(touchInfo->height) * 2.0f - 1.0f;
            Input::setJoyPos(0, jkL, vec2(tx, -ty));
        }
    }
    
    uint8 inputData[0x20];
    memcpy(inputData, (void*)0x900E0000, 0x20);

    #define IS_KEY_DOWN(key) ((*(short*)(inputData + key.tpad_row)) & key.tpad_col)

    Input::setJoyDown(0, jkA,      IS_KEY_DOWN(KEY_NSPIRE_2));
    Input::setJoyDown(0, jkB,      IS_KEY_DOWN(KEY_NSPIRE_3));
    Input::setJoyDown(0, jkX,      IS_KEY_DOWN(KEY_NSPIRE_5));
    Input::setJoyDown(0, jkY,      IS_KEY_DOWN(KEY_NSPIRE_6));
    Input::setJoyDown(0, jkLB,     IS_KEY_DOWN(KEY_NSPIRE_7));
    Input::setJoyDown(0, jkRB,     IS_KEY_DOWN(KEY_NSPIRE_9));
    Input::setJoyDown(0, jkL,      false);
    Input::setJoyDown(0, jkR,      false);
    Input::setJoyDown(0, jkStart,  IS_KEY_DOWN(KEY_NSPIRE_ENTER));
    Input::setJoyDown(0, jkSelect, IS_KEY_DOWN(KEY_NSPIRE_MENU));

    return !IS_KEY_DOWN(KEY_NSPIRE_ESC);
}

unsigned short* osPalette()
{
    return (unsigned short*)0xC0000200;
}

int main(void)
{
    if (!has_colors)
        return 0;

    lcd_init(SCR_320x240_565);

    timerInit();
    touchInit();

    contentDir[0] = saveDir[0] = cacheDir[0] = 0;

    strcpy(contentDir, "/documents/Games/OpenLara/");
    strcpy(saveDir, contentDir);
    strcpy(cacheDir, contentDir);

    Stream::addPack("content.tns");

    Core::width  = SCREEN_WIDTH;
    Core::height = SCREEN_HEIGHT;
    
    GAPI::swColor = new GAPI::ColorSW[Core::width * Core::height];
    GAPI::resize();
    
    Sound::channelsCount = 0;
    
    Game::init("DATA/LEVEL1.PHD");

    while (!Core::isQuit)
    {
        if (!inputUpdate())
        {
            Core::quit();
        }

        if (Game::update())
        {
            Game::render();
            
            lcd_blit(GAPI::swColor, SCR_320x240_565);
        }
    }

    delete[] GAPI::swColor;

    //Game::deinit();
    
    return 0;
}
