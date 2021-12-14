#include "common.h"
#include <soundplayer.h>

Item sndMixer;

struct Channel
{
    Item gainL;
    Item gainR;
    Item sampler;
    Item attach;
    Item frequency;
    Item amplitude;

    int32 index;
    bool playing;

    void setPitch(uint32 value)
    {
        if (frequency >= 0) {
            TweakKnob(frequency, value);
        }
    }

    void setVolume(uint32 value)
    {
        if (amplitude >= 0) {
            TweakKnob(amplitude, value);
        }
    }
};

struct SampleData
{
    Item data;
    int32 size;
};

Channel channels[SND_CHANNELS]; // [sample, sample, sample, music]
SampleData samples[MAX_SAMPLES];

#define MUSIC_CHANNEL (SND_CHANNELS - 1)

Item musicThread;
SPPlayer* musicPlayer;
SPSound* music;
uint32 musicSignal;
int32 musicTrack;

void musicProc()
{
    OpenAudioFolio();

    musicSignal = AllocSignal(0);

    void* buffers[SND_BUFFERS];
    for (int32 i = 0; i < SND_BUFFERS; i++)
    {
        buffers[i] = (uint8*)RAM_SND + i * SND_BUFFER_SIZE;
    }

    spCreatePlayer(&musicPlayer, channels[MUSIC_CHANNEL].sampler, SND_BUFFERS, SND_BUFFER_SIZE, buffers);

    int32 signalMask = spGetPlayerSignalMask(musicPlayer);

    while (1)
    {
        WaitSignal(musicSignal);

        channels[MUSIC_CHANNEL].playing = true;

        int32 track = musicTrack;
        {
            char path[32];
            sprintf(path, "audio/%d.aifc", track);

            spAddSoundFile(&music, musicPlayer, path);
        }

        spStartReading(music, SP_MARKER_NAME_BEGIN);

        spStartPlayingVA(musicPlayer, 
            AF_TAG_AMPLITUDE, 0x7FFF,
            TAG_END);

        while (spGetPlayerStatus(musicPlayer) & SP_STATUS_F_BUFFER_ACTIVE)
        {
            int32 signal = WaitSignal(signalMask);
            if (spService(musicPlayer, signal) < 0)
                break;

            if (track != musicTrack) {
                spStop(musicPlayer);
            }
        }

        spRemoveSound(music);
        channels[MUSIC_CHANNEL].playing = false;
    }
}


void sndInit()
{
    #if SND_CHANNELS != 4
        #error change the sound mixer or channels count
    #endif

    sndMixer = LoadInstrument("mixer4x2.dsp", 0, 100);

    LoadInstrument("decodeadpcm.dsp", 0, 100);

    char* LGainName = "LeftGain0";
    char* RGainName = "RightGain0";
    char* InputName = "Input0";

    for (int32 i = 0; i < SND_CHANNELS; i++)
    {
        Channel* ch = channels + i;

        LGainName[8] = '0' + i;
        RGainName[9] = '0' + i;
        InputName[5] = '0' + i;

        ch->gainL = GrabKnob(sndMixer, LGainName);
        ch->gainR = GrabKnob(sndMixer, RGainName);

        if (i == MUSIC_CHANNEL) {
            ch->sampler = LoadInstrument("dcsqxdhalfmono.dsp", 0, 100);
            ConnectInstruments(ch->sampler, "Output", sndMixer, InputName);
        } else {
            ch->sampler = LoadInstrument("adpcmvarmono.dsp", 0, 100);
            ConnectInstruments(ch->sampler, "Output", sndMixer, InputName);
        }

        ch->frequency = GrabKnob(ch->sampler, "Frequency");
        ch->amplitude = GrabKnob(ch->sampler, "Amplitude");
        ch->setVolume(0x7FFF);
        ch->setPitch(0x2000);

        TweakKnob(ch->gainL, 255 << 6);
        TweakKnob(ch->gainR, 255 << 6);

        ch->index = -1;
        ch->playing = false;
    }

    StartInstrument(sndMixer, NULL);

    musicThread = CreateThread("music", 180, musicProc, 2048);
}

void sndInitSamples()
{
    for (int32 i = 0; i < level.soundOffsetsCount; i++)
    {
        uint8* data = (uint8*)level.soundData + level.soundOffsets[i];
        int32 frames = *(uint32*)data;

        samples[i].size = frames >> 1;
        samples[i].data = CreateSampleVA(
            AF_TAG_FRAMES, frames,
            AF_TAG_ADDRESS, (uint8*)data + 4,
            AF_TAG_CHANNELS, 1,
            AF_TAG_WIDTH, 2,
            AF_TAG_COMPRESSIONTYPE, ID_ADP4,
            AF_TAG_COMPRESSIONRATIO, 4,
            TAG_END);
    }
}

void sndFreeSamples()
{
    if (!level.soundOffsetsCount)
        return;

    for (int32 i = 0; i < SND_CHANNELS - 1; i++)
    {
        Channel* ch = channels + i;

        if (ch->index < 0)
            continue;

        StopInstrument(ch->sampler, NULL);
        DetachSample(ch->attach);

        ch->index = -1;
        ch->playing = false;
    }

    for (int32 i = 0; i < level.soundOffsetsCount; i++)
    {
        UnloadSample(samples[i].data);
    }
}

void* sndPlaySample(int32 index, int32 volume, int32 pitch, int32 mode)
{
    volume = volume * 0x7FFF >> SND_VOL_SHIFT;
    pitch = pitch * 0x2000 >> SND_PITCH_SHIFT;

// update playing status
    int32 maxPos = -2;
    int32 maxPosIndex = -1;

    for (int32 i = 0; i < SND_CHANNELS; i++)
    {
        Channel* ch = channels + i;
        if (!ch->playing)
            continue;

        int32 pos = WhereAttachment(ch->attach);
        if (pos == -1 || pos >= samples[ch->index].size)
        {
            ch->playing = false;
        }

        if (maxPos < pos) {
            maxPos = pos;
            maxPosIndex = i;
        }
    }

// get existing channel
    if (mode == UNIQUE || mode == REPLAY)
    {
        for (int32 i = 0; i < SND_CHANNELS - 1; i++)
        {
            Channel* ch = channels + i;

            if (ch->index != index)
                continue;

            ch->setVolume(volume);
            ch->setPitch(pitch);

            if (!ch->playing || mode == REPLAY) 
            {
                ch->playing = true;
                StartInstrument(ch->sampler, NULL);
            }

            return (void*)ch->sampler;
        }
    }

// get free channel
    for (int32 i = 0; i < SND_CHANNELS - 1; i++)
    {
        Channel* ch = channels + i;

        if (ch->playing)
            continue;

        if (ch->index >= 0)
        {
            StopInstrument(ch->sampler, NULL);
            DetachSample(ch->attach);
        }

        ch->setVolume(volume);
        ch->setPitch(pitch);
        ch->attach = AttachSample(ch->sampler, samples[index].data, NULL);
        ch->index = index;
        ch->playing = true;

        StartInstrument(ch->sampler, NULL);

        return (void*)ch->sampler;
    }

// stop a longest playing sample
    if (maxPosIndex != -1)
    {
        sndStopSample(maxPosIndex);

        Channel* ch = channels + maxPosIndex;

        ch->setVolume(volume);
        ch->setPitch(pitch);
        ch->attach = AttachSample(ch->sampler, samples[index].data, NULL);
        ch->index = index;
        ch->playing = true;

        StartInstrument(ch->sampler, NULL);

        return (void*)ch->sampler;
    }

    return NULL;
}

void sndPlayTrack(int32 track)
{
    musicTrack = track;

    if (track >= 0) {
        SendSignal(musicThread, musicSignal);
    }
}

void sndStopTrack()
{
    sndPlayTrack(-1);
}

bool sndTrackIsPlaying()
{
    return channels[MUSIC_CHANNEL].playing;
}

void sndStopSample(int32 index)
{
    Channel* ch = channels + index;
    if (!ch->playing)
        return;

    StopInstrument(ch->sampler, NULL);
    DetachSample(ch->attach);
    ch->index = -1;
    ch->playing = false;
}

void sndStop()
{
    //sndStopTrack(); // TODO wait for signal?
    for (int32 i = 0; i < SND_CHANNELS - 1; i++)
    {
        sndStopSample(i);
    }
}