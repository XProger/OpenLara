#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <pwd.h>
#include <pthread.h>
#include <alsa/asoundlib.h>

// SDL2 include stuff
#include <SDL2/SDL.h>
#define GL_GLEXT_PROTOTYPES 1
#include <SDL2/SDL_opengles2.h>
//

#include "game.h"

#define WND_TITLE    "OpenLara"

// timing
unsigned int startTime;

int osGetTime() {
    timeval t;
    gettimeofday(&t, NULL);
    return int((t.tv_sec - startTime) * 1000 + t.tv_usec / 1000);
}

// sound
snd_pcm_uframes_t   SND_FRAMES = 512;
snd_pcm_t           *sndOut;
Sound::Frame        *sndData;
pthread_t           sndThread;

void* sndFill(void *arg) {
    while (sndOut) {
        Sound::fill(sndData, SND_FRAMES);

        int err = snd_pcm_writei(sndOut, sndData, SND_FRAMES);
        if (err < 0) {
            LOG("! sound: write %s\n", snd_strerror(err));;
            if (err != -EPIPE)
                break;

            err = snd_pcm_recover(sndOut, err, 0);
            if (err < 0) {
                LOG("! sound: failed to recover\n");
                break;
            }
            snd_pcm_prepare(sndOut);
        }
    }
    return NULL;
}

bool sndInit() {
    unsigned int freq = 44100;

    int err;
    if ((err = snd_pcm_open(&sndOut, "default", SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        LOG("! sound: open %s\n", snd_strerror(err));\
        sndOut = NULL;
        return false;
    }

    snd_pcm_hw_params_t *params;

    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(sndOut, params);
    snd_pcm_hw_params_set_access(sndOut, params, SND_PCM_ACCESS_RW_INTERLEAVED);

    snd_pcm_hw_params_set_channels(sndOut, params, 2);
    snd_pcm_hw_params_set_format(sndOut, params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_rate_near(sndOut, params, &freq, NULL);

    snd_pcm_hw_params_set_periods(sndOut, params, 4, 0);
    snd_pcm_hw_params_set_period_size_near(sndOut, params, &SND_FRAMES, NULL);
    snd_pcm_hw_params_get_period_size(params, &SND_FRAMES, 0);

    snd_pcm_hw_params(sndOut, params);
    snd_pcm_prepare(sndOut);

    sndData = new Sound::Frame[SND_FRAMES];
    memset(sndData, 0, SND_FRAMES * sizeof(Sound::Frame));
    if ((err = snd_pcm_writei(sndOut, sndData, SND_FRAMES)) < 0) {
        LOG("! sound: write %s\n", snd_strerror(err));\
        sndOut = NULL;
    }

    snd_pcm_start(sndOut);
    pthread_create(&sndThread, NULL, sndFill, NULL);

    return true;
}

void sndFree() {
    pthread_cancel(sndThread);
    snd_pcm_drop(sndOut);
    snd_pcm_drain(sndOut);
    snd_pcm_close(sndOut);
    delete[] sndData;
}

// Input
SDL_GameController *sdl_gamecontroller;
SDL_Joystick *sdl_joystick;
bool using_old_joystick_interface;

vec2 joyL, joyR;

bool osJoyReady(int index) {
    return index == 0; // TODO
}

void osJoyVibrate(int index, float L, float R) {
    // TODO
}

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

JoyKey codeToJoyKey(int code) {
    switch (code) {
    // gamepad
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

bool inputInit() {
    sdl_gamecontroller = NULL;
    sdl_joystick = NULL;
    if (SDL_NumJoysticks() > 0) {
	if(SDL_IsGameController(0)) {
            sdl_gamecontroller = SDL_GameControllerOpen(0);
        }
        else {
            sdl_joystick = SDL_JoystickOpen(0);
            using_old_joystick_interface = true;
        }
    }
    return true;
}

void inputFree() {
    if (sdl_gamecontroller != NULL) {
        SDL_GameControllerClose(sdl_gamecontroller);
        sdl_gamecontroller = NULL;
    }
    if (sdl_joystick != NULL) {
        SDL_JoystickClose(sdl_joystick);
        sdl_joystick = NULL;
    }
}

#define JOY_DEAD_ZONE_STICK      8192

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

    int joyIndex = 0; // TODO: joy index

    SDL_Event event;
    while (SDL_PollEvent(&event) == 1) { // while there are still events to be processed
        switch (event.type) {
            case SDL_KEYDOWN: {
		int scancode = event.key.keysym.scancode;
                InputKey key = codeToInputKey(scancode);
		if (key != ikNone) {
		    Input::setDown(key, 1);
		}
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
             case SDL_CONTROLLERBUTTONDOWN: {
                        JoyKey key = codeToJoyKey(event.cbutton.button);
                        Input::setJoyDown(joyIndex, key, 1);
                        break;
             }
             case SDL_CONTROLLERBUTTONUP: {
                        JoyKey key = codeToJoyKey(event.cbutton.button);
                        Input::setJoyDown(joyIndex, key, 0);
                        break;
             }
             case SDL_CONTROLLERAXISMOTION: {
                 switch (event.caxis.axis) {
                     case SDL_CONTROLLER_AXIS_LEFTX:
		            if (event.caxis.value < 0) {
			        Input::setJoyDown(joyIndex, jkLeft,  1);
			        Input::setJoyDown(joyIndex, jkRight, 0);
                            }
		            if (event.caxis.value > 0) {
			        Input::setJoyDown(joyIndex, jkRight, 1);
			        Input::setJoyDown(joyIndex, jkLeft,  0);
                            } 
                            if (event.caxis.value == 0) {
			        Input::setJoyDown(joyIndex, jkRight, 0);
			        Input::setJoyDown(joyIndex, jkLeft,  0);
                            }
		            break;
                      case SDL_CONTROLLER_AXIS_LEFTY:
		            if (event.caxis.value < 0) {
			        Input::setJoyDown(joyIndex, jkUp,  1);
			        Input::setJoyDown(joyIndex, jkDown, 0);
                            }
		            if (event.caxis.value > 0) {
			        Input::setJoyDown(joyIndex, jkUp, 0);
			        Input::setJoyDown(joyIndex, jkDown,  1);
                            } 
                            if (event.caxis.value == 0) {
			        Input::setJoyDown(joyIndex, jkUp, 0);
			        Input::setJoyDown(joyIndex, jkDown,  0);
                            }
		            break;
                 }
                 break;
             }
        }

        Input::setJoyPos(joyIndex, jkL, joyDir(joyL));
        Input::setJoyPos(joyIndex, jkR, joyDir(joyR));
     }
}

int main(int argc, char **argv) {

    int w, h;
    SDL_DisplayMode current;


    SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS|SDL_INIT_GAMECONTROLLER);

    SDL_GetCurrentDisplayMode(0, &current);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    // We start in fullscreen mode using the vide mode currently in use, to avoid video mode changes.
    SDL_Window *window = SDL_CreateWindow(WND_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        //current.w, current.h, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN_DESKTOP);
	      640, 480, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
   
    // We try to use the current video mode, but we inform the core of whatever mode SDL2 gave us in the end. 
    SDL_GetWindowSize(window, &w, &h);

    Core::width  = w;
    Core::height = h;

    SDL_GLContext context = SDL_GL_CreateContext(window);
    SDL_GL_SetSwapInterval(0);

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1,
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
	    SDL_GL_SwapWindow(window);
        }
    };

    sndFree();
    Game::deinit();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
