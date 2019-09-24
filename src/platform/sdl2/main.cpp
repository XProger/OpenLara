#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>

#include <SDL2/SDL.h>

#include "game.h"

#define WND_TITLE    "OpenLara"

// timing
unsigned int startTime;

int osGetTimeMS() {
    timeval t;
    gettimeofday(&t, NULL);
    return int((t.tv_sec - startTime) * 1000 + t.tv_usec / 1000);
}

// sound
#define SND_FRAME_SIZE  4
#define SND_FRAMES      1024

// A Frame is a struct containing: int16 L, int16 R.
Sound::Frame        *sndData;
SDL_AudioDeviceID sdl_audiodev;

void sndFill(void *udata, Uint8 *stream, int len) {
        // Let's milk the audio subsystem for SND_FRAMES frames!
        Sound::fill(sndData, SND_FRAMES);
        // We have the number of samples, but each sample is 4 bytes long (16bit stereo sound),
        // and memcpy copies a number of bytes.
        memcpy (stream, sndData, SND_FRAMES * SND_FRAME_SIZE);
}

bool sndInit() {
    int FREQ = 44100;

    SDL_AudioSpec desired, obtained;

    desired.freq     = FREQ;
    desired.format   = AUDIO_S16SYS;
    desired.channels = 2;
    desired.samples  = SND_FRAMES;
    desired.callback = sndFill;
    desired.userdata = NULL;

    sdl_audiodev = SDL_OpenAudioDevice(NULL, 0, &desired, &obtained, /*SDL_AUDIO_ALLOW_FORMAT_CHANGE*/0);
    if (sdl_audiodev == 0)
    {
        LOG ("SDL2: error opening audio device: %s\n", SDL_GetError());
        return false;
    }

    if (desired.samples != obtained.samples) {
        LOG ("SDL2: number of samples not supported by device. Watch out for buggy audio drivers!\n");
        return false;
    }

    // Initialize audio buffer and fill it with zeros
    sndData = new Sound::Frame[SND_FRAMES];
    memset(sndData, 0, SND_FRAMES * SND_FRAME_SIZE);

    SDL_PauseAudioDevice(sdl_audiodev,0);

    return true;
}

void sndFree() {
    SDL_PauseAudioDevice(sdl_audiodev,1);
    SDL_CloseAudioDevice(sdl_audiodev);

    // Delete the audio buffer
    delete[] sndData;
}

// Input

#define MAX_JOYS 4
#define JOY_DEAD_ZONE_STICK      8192
#define WIN_W 640
#define WIN_H 480

struct sdl_input *sdl_inputs;
int sdl_numjoysticks, sdl_numcontrollers;
SDL_Joystick *sdl_joysticks[MAX_JOYS];
SDL_GameController *sdl_controllers[MAX_JOYS];
SDL_Window *sdl_window;
SDL_Renderer *sdl_renderer;
SDL_DisplayMode sdl_displaymode;

bool fullscreen;

vec2 joyL, joyR;

bool osJoyReady(int index) {
    return index == 0; // TODO
}

void osJoyVibrate(int index, float L, float R) {
    // TODO
}

bool isKeyPressed (SDL_Scancode scancode) {
    const Uint8 *state = SDL_GetKeyboardState(NULL);
    if (state[scancode]) {
        return true;
    }
    return false;
}

#ifndef _GAPI_GLES 
void toggleFullscreen () {

    Uint32 flags = 0;

    fullscreen = !fullscreen;

    flags = fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0;

    SDL_SetWindowFullscreen (sdl_window, flags);

    // Tell the engine we have changed display size!
    Core::width  = fullscreen ? sdl_displaymode.w : WIN_W;
    Core::height = fullscreen ? sdl_displaymode.h : WIN_H;
}
#endif

InputKey codeToInputKey(int code) {
    switch (code) {
    // keyboard
        case SDL_SCANCODE_LEFT       : return ikLeft;
        case SDL_SCANCODE_RIGHT      : return ikRight;
        case SDL_SCANCODE_UP         : return ikUp;
        case SDL_SCANCODE_DOWN       : return ikDown;
        case SDL_SCANCODE_SPACE      : return ikSpace;
        case SDL_SCANCODE_TAB        : return ikTab;
        case SDL_SCANCODE_RETURN     : return ikEnter;
        case SDL_SCANCODE_ESCAPE     : return ikEscape;
        case SDL_SCANCODE_LSHIFT     :
        case SDL_SCANCODE_RSHIFT     : return ikShift;
        case SDL_SCANCODE_LCTRL      :
        case SDL_SCANCODE_RCTRL      : return ikCtrl;
        case SDL_SCANCODE_LALT       :
        case SDL_SCANCODE_RALT       : return ikAlt;
        case SDL_SCANCODE_0          : return ik0;
        case SDL_SCANCODE_1          : return ik1;
        case SDL_SCANCODE_2          : return ik2;
        case SDL_SCANCODE_3          : return ik3;
        case SDL_SCANCODE_4          : return ik4;
        case SDL_SCANCODE_5          : return ik5;
        case SDL_SCANCODE_6          : return ik6;
        case SDL_SCANCODE_7          : return ik7;
        case SDL_SCANCODE_8          : return ik8;
        case SDL_SCANCODE_9          : return ik9;
        case SDL_SCANCODE_A          : return ikA;
        case SDL_SCANCODE_B          : return ikB;
        case SDL_SCANCODE_C          : return ikC;
        case SDL_SCANCODE_D          : return ikD;
        case SDL_SCANCODE_E          : return ikE;
        case SDL_SCANCODE_F          : return ikF;
        case SDL_SCANCODE_G          : return ikG;
        case SDL_SCANCODE_H          : return ikH;
        case SDL_SCANCODE_I          : return ikI;
        case SDL_SCANCODE_J          : return ikJ;
        case SDL_SCANCODE_K          : return ikK;
        case SDL_SCANCODE_L          : return ikL;
        case SDL_SCANCODE_M          : return ikM;
        case SDL_SCANCODE_N          : return ikN;
        case SDL_SCANCODE_O          : return ikO;
        case SDL_SCANCODE_P          : return ikP;
        case SDL_SCANCODE_Q          : return ikQ;
        case SDL_SCANCODE_R          : return ikR;
        case SDL_SCANCODE_S          : return ikS;
        case SDL_SCANCODE_T          : return ikT;
        case SDL_SCANCODE_U          : return ikU;
        case SDL_SCANCODE_V          : return ikV;
        case SDL_SCANCODE_W          : return ikW;
        case SDL_SCANCODE_X          : return ikX;
        case SDL_SCANCODE_Y          : return ikY;
        case SDL_SCANCODE_Z          : return ikZ;
        case SDL_SCANCODE_AC_HOME    : return ikEscape;
    }
    return ikNone;
}

JoyKey controllerCodeToJoyKey(int code) {
// joystick using the modern SDL GameController interface
    switch (code) {
        case SDL_CONTROLLER_BUTTON_A                    : return jkA;
        case SDL_CONTROLLER_BUTTON_B                    : return jkB;
        case SDL_CONTROLLER_BUTTON_X                    : return jkX;
        case SDL_CONTROLLER_BUTTON_Y                    : return jkY;
        case SDL_CONTROLLER_BUTTON_LEFTSHOULDER         : return jkLB;
        case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER        : return jkRB;
        case SDL_CONTROLLER_BUTTON_BACK                 : return jkSelect;
        case SDL_CONTROLLER_BUTTON_START                : return jkStart;
        case SDL_CONTROLLER_BUTTON_LEFTSTICK            : return jkL;
        case SDL_CONTROLLER_BUTTON_RIGHTSTICK           : return jkR;
    }
    return jkNone;
}

JoyKey joyCodeToJoyKey(int buttonNumber) {
// joystick using the classic SDL Joystick interface
    switch (buttonNumber) {
        case 0 : return jkY;
        case 1 : return jkB;
        case 2 : return jkA;
        case 3 : return jkX;
        case 4 : return jkL;
        case 5 : return jkR;
        case 6 : return jkLB;
        case 7 : return jkRB;
        case 8 : return jkSelect;
        case 9 : return jkStart;
    }
    return jkNone;
}

int joyGetIndex(SDL_JoystickID id) {
    int i;
    for (i=0 ; i < sdl_numjoysticks; i++) {
        if (SDL_JoystickInstanceID(sdl_joysticks[i]) == id) {
            return i;
        }
    }
    return -1;
}

// To know if it's a gamecontroller when we only have the instanceID
bool joyIsController (Sint32 instanceID) {
    int i;
    bool ret = false;
    
    // We can't use SDL_IsGameController after we have physically disconnected a joystick, so we use this workaround.
    for (i = 0; i < sdl_numcontrollers; i++) {
        if (SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(sdl_controllers[i])) == instanceID) {
            ret = true;
            break;
        }
    }
    return ret;
}

void joyAdd(int index) {
    if(SDL_IsGameController(index)) {
        SDL_GameController *controller = SDL_GameControllerOpen(index);
        sdl_controllers[index] = controller;
        sdl_joysticks[index] = SDL_GameControllerGetJoystick(controller);
        sdl_numcontrollers++;
    }
    else {
        sdl_joysticks[index] = SDL_JoystickOpen(index);
    }
    // Update number of joysticks
    sdl_numjoysticks = SDL_NumJoysticks();
    sdl_numjoysticks = (sdl_numjoysticks < MAX_JOYS )? sdl_numjoysticks : MAX_JOYS;
}

void joyRemove(Sint32 instanceID) {
    int i;

    // Closing game controller
    if (joyIsController(instanceID)) {
        for (i = 0; i < sdl_numcontrollers; i++) {
            if (SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(sdl_controllers[i])) == instanceID) {
	        SDL_GameControllerClose(sdl_controllers[i]);
                sdl_controllers[i] = NULL;
	        sdl_numcontrollers--;
	        sdl_numjoysticks--;
            }
        }   
    }
    // Closing joystick
    else {
        i = joyGetIndex(instanceID);    
        if (i >= 0) {
            SDL_JoystickClose(sdl_joysticks[i]);
            sdl_numjoysticks--;
        }
    }
}

bool inputInit() {
    int index;
    joyL = joyR = vec2(0);
    sdl_numjoysticks = SDL_NumJoysticks();
    sdl_numjoysticks = (sdl_numjoysticks < MAX_JOYS )? sdl_numjoysticks : MAX_JOYS;

    for (index = 0; index < MAX_JOYS; index++)
        sdl_joysticks[index] = NULL;

    for (index = 0; index < sdl_numjoysticks; index++) {
        joyAdd(index);
    }
    return true;
}

void inputFree() {
    int i;
    Sint32 instanceID;

    for (i = 0; i < sdl_numjoysticks; i++) {
        instanceID = SDL_JoystickInstanceID(sdl_joysticks[i]);
        joyRemove(instanceID);
    }
}

float joyAxisValue(int value) {
    if (value > -JOY_DEAD_ZONE_STICK && value < JOY_DEAD_ZONE_STICK)
        return 0.0f;
    return value / 32767.0f;
}

float joyTrigger(int value) {
    return min(1.0f, value / 255.0f);
}

vec2 joyDir(const vec2 &value) {
    float dist = min(1.0f, value.length());
    return value.normal() * dist;
}

void inputUpdate() {
// get input events

    int joyIndex;
    SDL_Event event;

    while (SDL_PollEvent(&event) == 1) { // while there are still events to be processed
        switch (event.type) {
            case SDL_KEYDOWN: {
		int scancode = event.key.keysym.scancode;
                InputKey key = codeToInputKey(scancode);
		if (key != ikNone) {
		    Input::setDown(key, 1);
		}

#ifndef _GAPI_GLES 
                if (scancode == SDL_SCANCODE_RETURN) {
                    if (isKeyPressed(SDL_SCANCODE_LALT) && isKeyPressed(SDL_SCANCODE_RETURN)) {
                        toggleFullscreen();
                    }
                }
#endif
                
		break;
             }
             case SDL_KEYUP: {
		int scancode = event.key.keysym.scancode;
                InputKey key = codeToInputKey(scancode);
		if (key != ikNone) {
		    Input::setDown(key, 0);
                }
                break;
             }
             // Joystick reading using the modern GameController interface
             case SDL_CONTROLLERBUTTONDOWN: {
                        joyIndex = joyGetIndex(event.cbutton.which);
                        JoyKey key = controllerCodeToJoyKey(event.cbutton.button);
                        Input::setJoyDown(joyIndex, key, 1);
                        break;
             }
             case SDL_CONTROLLERBUTTONUP: {
                        joyIndex = joyGetIndex(event.cbutton.which);
                        JoyKey key = controllerCodeToJoyKey(event.cbutton.button);
                        Input::setJoyDown(joyIndex, key, 0);
                        break;
             }
             case SDL_CONTROLLERAXISMOTION: {
                 joyIndex = joyGetIndex(event.caxis.which);
                 switch (event.caxis.axis) {
                     case SDL_CONTROLLER_AXIS_LEFTX:  joyL.x = joyAxisValue(event.caxis.value); break;
                     case SDL_CONTROLLER_AXIS_LEFTY:  joyL.y = joyAxisValue(event.caxis.value); break;
                     case SDL_CONTROLLER_AXIS_RIGHTX: joyR.x = joyAxisValue(event.caxis.value); break;
                     case SDL_CONTROLLER_AXIS_RIGHTY: joyR.y = joyAxisValue(event.caxis.value); break;
                 }
                 Input::setJoyPos(joyIndex, jkL, joyDir(joyL));
                 Input::setJoyPos(joyIndex, jkR, joyDir(joyR));
                 break;
             }
             // Joystick reading using the classic Joystick interface
             case SDL_JOYBUTTONDOWN: {
                        joyIndex = joyGetIndex(event.jbutton.which);
                        JoyKey key = joyCodeToJoyKey(event.jbutton.button);
                        Input::setJoyDown(joyIndex, key, 1);
                        break;
             }
             case SDL_JOYBUTTONUP: {
                        joyIndex = joyGetIndex(event.jbutton.which);
                        JoyKey key = joyCodeToJoyKey(event.jbutton.button);
                        Input::setJoyDown(joyIndex, key, 0);
                        break;
             }
             case SDL_JOYAXISMOTION: {
                 joyIndex = joyGetIndex(event.jaxis.which);
                 switch (event.jaxis.axis) {
                     // In the classic joystick interface we know what axis changed by it's number,
                     // they have no names like on the fancy GameController interface. 
                     case 0: joyL.x = joyAxisValue(event.jaxis.value); break;
                     case 1: joyL.y = joyAxisValue(event.jaxis.value); break;
                     case 2: joyR.x = joyAxisValue(event.jaxis.value); break;
                     case 3: joyR.y = joyAxisValue(event.jaxis.value); break;
                 }
                 Input::setJoyPos(joyIndex, jkL, joyDir(joyL));
                 Input::setJoyPos(joyIndex, jkR, joyDir(joyR));
                 break;
             }
             // Joystick connection or disconnection
             case SDL_JOYDEVICEADDED : {
                 // Upon connection, 'which' is the internal SDL2 joystick index,
                 // but on disconnection, 'which' is the instanceID.
                 // We store the joysticks in their corresponding position on the joysticks array,
                 // IE: joystick with index 3 will be in sdl_joysticks[3].
                 joyAdd(event.jdevice.which);
                 break;
             }
             case SDL_JOYDEVICEREMOVED : {
                 joyRemove(event.jdevice.which);
                 break;
             }
         }
     }
}

int main(int argc, char **argv) {

    int w, h;

    SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_EVENTS|SDL_INIT_GAMECONTROLLER);

    SDL_GetCurrentDisplayMode(0, &sdl_displaymode);

#ifdef _GAPI_GLES
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#endif

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    // We start in fullscreen mode using the vide mode currently in use, to avoid video mode changes.
    sdl_window = SDL_CreateWindow(WND_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        //sdl_displaymode.w, sdl_displaymode.h, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN_DESKTOP);
	WIN_W, WIN_H, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN );
  
    // We try to use the current video mode, but we inform the core of whatever mode SDL2 gave us in the end. 
    SDL_GetWindowSize(sdl_window, &w, &h);

    Core::width  = w;
    Core::height = h;

    SDL_GLContext context = SDL_GL_CreateContext(sdl_window);
    SDL_GL_SetSwapInterval(0);

    sdl_renderer = SDL_CreateRenderer(sdl_window, -1,
	  SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);

    SDL_ShowCursor(SDL_DISABLE);

    cacheDir[0] = saveDir[0] = contentDir[0] = 0;

    const char *home;
    if (!(home = getenv("HOME")))
        home = getpwuid(getuid())->pw_dir;
    strcat(cacheDir, home);
    strcat(cacheDir, "/.openlara/");

    struct stat st = {0};
    if (stat(cacheDir, &st) == -1 && mkdir(cacheDir, 0777) == -1)
        cacheDir[0] = 0;
    strcpy(saveDir, cacheDir);

    timeval t;
    gettimeofday(&t, NULL);
    startTime = t.tv_sec;

    sndInit();

    inputInit();

    char *lvlName = argc > 1 ? argv[1] : NULL;

    Game::init(lvlName);

    while (!Core::isQuit) {
        inputUpdate();

        if (Game::update()) {
            Game::render();
            Core::waitVBlank();
	    SDL_GL_SwapWindow(sdl_window);
        }
    };

    sndFree();
    Game::deinit();

    SDL_DestroyRenderer(sdl_renderer);
    SDL_DestroyWindow(sdl_window);
    SDL_Quit();

    return 0;
}
