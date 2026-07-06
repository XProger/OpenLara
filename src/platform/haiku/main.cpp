#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <SupportDefs.h>
#include <storage/FindDirectory.h>
#include <kernel/fs_info.h>
#include <media/MediaDefs.h>
#include <media/SoundPlayer.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include "game.h"
#include "App.h"


unsigned int startTime;

#define SND_FRAME_SIZE  4
#define SND_FRAMES      1024

Sound::Frame *sndData;
media_raw_audio_format sndFormat;

BSoundPlayer *sndPlayer;

int osGetTimeMS() {
    timeval t;
    gettimeofday(&t, NULL);
    return int((t.tv_sec - startTime) * 1000 + t.tv_usec / 1000);
}

void sndFill(void *cookie, void *buffer, size_t size, const media_raw_audio_format &format) {
    Sound::fill(sndData, SND_FRAMES);
    memcpy(buffer, sndData, SND_FRAMES * SND_FRAME_SIZE);
}

void sndInit() {
    sndFormat.buffer_size = SND_FRAMES * SND_FRAME_SIZE;
    sndFormat.format = media_raw_audio_format::B_AUDIO_SHORT;
    sndFormat.channel_count = 2;
    sndFormat.frame_rate = 44100.0;
    sndFormat.byte_order = B_MEDIA_LITTLE_ENDIAN;

    sndPlayer = new BSoundPlayer(&sndFormat, "sndPlayer", sndFill, NULL, NULL);

    sndData = new Sound::Frame[SND_FRAMES * SND_FRAME_SIZE];

    sndPlayer->Start();
    sndPlayer->SetHasData(true);
}

void sndFree() {
    sndPlayer->Stop();
    delete sndPlayer;
}

void osJoyVibrate(int index, float L, float R) {
}

InputKey codeToInputKey(const char* code) {
    switch (code[0]) {
    // keyboard
        case B_LEFT_ARROW            : return ikLeft;
        case B_RIGHT_ARROW           : return ikRight;
        case B_UP_ARROW              : return ikUp;
        case B_DOWN_ARROW            : return ikDown;
        case B_SPACE                 : return ikSpace;
        case B_TAB                   : return ikTab;
        case B_RETURN                : return ikEnter;
        case B_ESCAPE                : return ikEscape;
        //case B_LEFT_SHIFT_KEY        :
        //case B_RIGHT_SHIFT_KEY       : return ikShift;
        //case B_LEFT_CONTROL_KEY      :
        //case B_RIGHT_CONTROL_KEY     : return ikCtrl;
        //case B_LEFT_COMMAND_KEY      :
        //case B_RIGHT_COMMAND_KEY     : return ikAlt;
        case '0'          : return ik0;
        case '1'          : return ik1;
        case '2'          : return ik2;
        case '3'          : return ik3;
        case '4'          : return ik4;
        case '5'          : return ik5;
        case '6'          : return ik6;
        case '7'          : return ik7;
        case '8'          : return ik8;
        case '9'          : return ik9;
        case 'a'          : return ikA;
        case 'b'          : return ikB;
        case 'c'          : return ikC;
        case 'd'          : return ikD;
        case 'e'          : return ikE;
        case 'f'          : return ikF;
        case 'g'          : return ikG;
        case 'h'          : return ikH;
        case 'i'          : return ikI;
        case 'j'          : return ikJ;
        case 'k'          : return ikK;
        case 'l'          : return ikL;
        case 'm'          : return ikM;
        case 'n'          : return ikN;
        case 'o'          : return ikO;
        case 'p'          : return ikP;
        case 'q'          : return ikQ;
        case 'r'          : return ikR;
        case 's'          : return ikS;
        case 't'          : return ikT;
        case 'u'          : return ikU;
        case 'v'          : return ikV;
        case 'w'          : return ikW;
        case 'x'          : return ikX;
        case 'y'          : return ikY;
        case 'z'          : return ikZ;
        //case SDL_SCANCODE_AC_HOME    : return ikEscape;
    }

    return ikNone;
}

void handleKeyPress(const char* keyBytes, int index) {
    Input::setDown(codeToInputKey(keyBytes), index);
}

void handleModifierKeyPress(int32 keys, int index) {
    if (keys & B_SHIFT_KEY) {
        Input::setDown(ikShift, 1);
    } else {
        Input::setDown(ikShift, 0);
    }

    if (keys & B_CONTROL_KEY) {
        Input::setDown(ikCtrl, 1);
    } else {
        Input::setDown(ikCtrl, 0);
    }

    if (keys & B_COMMAND_KEY) {
        Input::setDown(ikAlt, 1);
    } else {
        Input::setDown(ikAlt, 0);
    }
}

int main() {
	cacheDir[0] = saveDir[0] = contentDir[0] = 0;

    dev_t bootVolume = dev_for_path("/boot");

    if (find_directory(B_USER_CACHE_DIRECTORY, bootVolume, false, cacheDir, 255) != B_OK) {
        fputs("Cannot get B_USER_CACHE_DIRECTORY\n", stderr);
        return 1;
    }

    if (find_directory(B_USER_SETTINGS_DIRECTORY, bootVolume, false, saveDir, 255) != B_OK) {
        fputs("Cannot get B_USER_SETTINGS_DIRECTORY\n", stderr);
        return 1;
    }

    if (find_directory(B_USER_NONPACKAGED_DATA_DIRECTORY, bootVolume, false, contentDir, 255) != B_OK) {
        fputs("Cannot get B_USER_NONPACKAGED_DATA_DIRECTORY\n", stderr);
        return 1;
    }

    strcat(cacheDir, "/openlara/");
    strcat(saveDir, "/openlara/");
    strcat(contentDir, "/openlara/");

    struct stat st = {0};

    if (stat(cacheDir, &st) == -1 && mkdir(cacheDir, 0777) == -1) {
        fprintf(stderr, "Cannot create cache dir %s\n", cacheDir);
        return 1;
    }

    if (stat(saveDir, &st) == -1 && mkdir(saveDir, 0777) == -1) {
        fprintf(stderr, "Cannot create save dir %s\n", saveDir);
        return 1;
    }

    if (stat(contentDir, &st) == -1 && mkdir(contentDir, 0777) == -1) {
        fprintf(stderr, "Cannot create content dir %s\n", contentDir);
        return 1;
    }

    timeval t;
    gettimeofday(&t, NULL);
    startTime = t.tv_sec;

    App* app = new App();
    app->Run();
    delete app;
    return 0;
}
