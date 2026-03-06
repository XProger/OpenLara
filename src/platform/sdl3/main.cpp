#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>

#define SDL_MAIN_USE_CALLBACKS 1 /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "game.h"

#define WND_TITLE    "OpenLara"
#define SDL_WINDOW_WIDTH           800
#define SDL_WINDOW_HEIGHT          600

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Surface *surface;
} AppState;

// timing
unsigned int startTime;

int osGetTimeMS() {

    timeval t;
    gettimeofday(&t, NULL);
    return int((t.tv_sec - startTime) * 1000 + t.tv_usec / 1000);
    
}

void osJoyVibrate(int index, float L, float R) {

}

// sound
#define SND_FRAME_SIZE  4
#define SND_FRAMES      256
#define FREQ		44100

void sndFill(void *userdata, SDL_AudioStream *stream, int additional_amount, int total_amount) {

	int count = 0; 
	if (!additional_amount) {
		return;
	}
	while (additional_amount > 0) {
		count = (additional_amount / SND_FRAME_SIZE > SND_FRAMES) ? SND_FRAMES : additional_amount / SND_FRAME_SIZE;
		Sound::fill((Sound::Frame*) userdata, count);
		if (!SDL_PutAudioStreamData(stream, userdata, count * SND_FRAME_SIZE)) {
			LOG("Couldn't flush audio stream: %s", SDL_GetError());
			return;
		}
		additional_amount -= count * SND_FRAME_SIZE;
	}
	
}

bool sndInit() {

	SDL_AudioSpec desired;
	SDL_AudioStream *stream;
	Sound::Frame        *sndData;

	// Initialize audio buffer and fill it with zeros
	sndData = new Sound::Frame[SND_FRAMES];
	memset(sndData, 0, SND_FRAMES * SND_FRAME_SIZE);

	desired.freq     = FREQ;
	desired.format   = SDL_AUDIO_S16;
	desired.channels = 2;

	stream = SDL_OpenAudioDeviceStream( SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK , &desired, sndFill, sndData );
	
	if (!stream) {
		LOG("Couldn't create audio stream: %s\n", SDL_GetError());
		return false;
	}
	
        SDL_ResumeAudioStreamDevice(stream);
	return true;
	
};

//input 
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

void print_help(int argc, char **argv) {
    
    printf("%s [OPTION]\nA open source re-implementation of the classic Tomb Raider engine.\n",
           argc ? argv[0] : "OpenLara");
    puts("-d [DIRECTORY]  directory where data files are");
    puts("-l [FILE]       load a specific level file");
    puts("-h              print this help");
    
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {

    cacheDir[0] = saveDir[0] = contentDir[0] = 0;
    char *lvlName = nullptr;

    int option;
    while ((option = getopt(argc, argv, "hl:d:")) != -1) {
        switch(option) {
            case 'h':
                print_help(argc, argv);
                return SDL_APP_SUCCESS;
            case 'l':
               lvlName = optarg;
               break;
            case 'd':
               strncpy(contentDir, optarg, 254);
               break;
            case ':':
                LOG("option %c needs a value\n", optopt);
                print_help(argc, argv);
                return SDL_APP_FAILURE;
            case '?':
                LOG("unknown option: %c\n", optopt);
                print_help(argc, argv);
                return SDL_APP_FAILURE;
            default:
                break;
        }
    }

    size_t contentDirLen = strlen(contentDir);

    if (contentDirLen > 0 &&
        contentDir[contentDirLen-1] != '/' && contentDir[contentDirLen-1] != '\\' &&
        contentDirLen < 254) {
        contentDir[contentDirLen] = '/';
        contentDir[contentDirLen+1] = '\0';
    }

    timeval t;
    gettimeofday(&t, NULL);
    startTime = t.tv_sec;

    SDL_AddGamepadMappingsFromFile("gamecontrollerdb.txt");
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD)) {
	LOG("Couldn't init SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    AppState *as = (AppState *)SDL_calloc(1, sizeof(AppState));
    if (!as) {
        LOG("Couldn't init app state: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    *appstate = as;
   
    if (!SDL_CreateWindowAndRenderer(WND_TITLE, SDL_WINDOW_WIDTH, SDL_WINDOW_HEIGHT, 0, &as->window, &as->renderer)) {
        LOG("Couldn't create window: %s", SDL_GetError());   
        return SDL_APP_FAILURE;
    }

    Core::width  = SDL_WINDOW_WIDTH;
    Core::height = SDL_WINDOW_HEIGHT;

    as->surface = SDL_GetWindowSurface(as->window);
    if (as->surface == NULL) {
        LOG("Couldn't get surface: %s", SDL_GetError());   
	return SDL_APP_FAILURE;
    }
    SDL_LockSurface(as->surface);

    GAPI::swColor = (unsigned int*)as->surface->pixels;
    GAPI::resize();

    if (!sndInit()) {
	//return SDL_APP_FAILURE;
    }

    Game::init(lvlName);

    return SDL_APP_CONTINUE;
    
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {

    InputKey key = ikNone;

    switch (event->type) {
	case SDL_EVENT_QUIT:
		return SDL_APP_SUCCESS;    
	case SDL_EVENT_KEY_DOWN:
		key = codeToInputKey(event->key.scancode);
		if (key != ikNone) {
                    Input::setDown(key, 1);
                }
		break;
	case SDL_EVENT_KEY_UP:
		key = codeToInputKey(event->key.scancode);
		if (key != ikNone) {
                    Input::setDown(key, 0);
                }
		break;		
    }
    return SDL_APP_CONTINUE;
    
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {

    LOG("Exiting\n");
    if (appstate != NULL) {
        AppState *as = (AppState *)appstate;
        SDL_DestroyRenderer(as->renderer);
        SDL_DestroyWindow(as->window);
	SDL_DestroySurface(as->surface);
        SDL_free(as);
    }
    
}

SDL_AppResult SDL_AppIterate(void *appstate) {

	AppState *as = (AppState *)appstate;
	
	if(Core::isQuit) {
		return SDL_APP_SUCCESS;
	}
	if (Game::update()) {
            Game::render();
            SDL_UpdateWindowSurface(as->window);
        }

	return SDL_APP_CONTINUE;
	
}
