#include "common.h"

Item sndMixer;

struct Channel
{
    Item gainL;
    Item gainR;
    Item sample;
    Item attach;
    Item frequency;
    Item amplitude;

    Item playing; // TODO

    void setPitch(uint32 value)
    {
        TweakKnob(frequency, value);
    }

    void setVolume(uint32 value)
    {
        TweakKnob(amplitude, value);
    }
};

Channel channels[SND_CHANNELS];

Item samples[SND_SAMPLES];

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
        LGainName[8] = '0' + i;
        RGainName[9] = '0' + i;
        InputName[5] = '0' + i;

        channels[i].gainL = GrabKnob(sndMixer, LGainName);
        channels[i].gainR = GrabKnob(sndMixer, RGainName);

        TweakKnob(channels[i].gainL, 255 << 6);
        TweakKnob(channels[i].gainR, 255 << 6);

        channels[i].sample = LoadInstrument("adpcmvarmono.dsp", 0, 100);
        channels[i].frequency = GrabKnob(channels[i].sample, "Frequency");
        channels[i].amplitude = GrabKnob(channels[i].sample, "Amplitude");
        ConnectInstruments(channels[i].sample, "Output", sndMixer, InputName);

        channels[i].setVolume(0x7FFF);
        channels[i].setPitch(0x2000);
    }

    StartInstrument(sndMixer, NULL);
}

void sndInitSamples()
{
    for (int32 i = 0; i < SND_CHANNELS; i++)
    {
        StopInstrument(channels[i].sample, NULL);
        if (channels[i].attach) {
            DetachSample(channels[i].attach);
        }
        channels[i].playing = NULL;
    }

    for (int32 i = 0; i < level.soundOffsetsCount; i++)
    {
        uint8* data = (uint8*)level.soundData + level.soundOffsets[i];

        samples[i] = CreateSampleVA(
            AF_TAG_FRAMES, *(uint32*)data,
            AF_TAG_ADDRESS, (uint8*)data + 4,
            AF_TAG_CHANNELS, 1,
            AF_TAG_WIDTH, 2,
            AF_TAG_COMPRESSIONTYPE, ID_ADP4,
            AF_TAG_COMPRESSIONRATIO, 4,
            TAG_END
        );
    }
}

int32 idx = 0;

void* sndPlaySample(int32 index, int32 volume, int32 pitch, int32 mode)
{
    if (mode == UNIQUE || mode == REPLAY)
    {
        for (int32 i = 0; i < SND_CHANNELS; i++)
        {
            if (channels[i].playing != samples[index])
                continue;

            channels[i].setVolume(0x7FFF * volume >> SND_VOL_SHIFT);
            channels[i].setPitch(0x2000 * pitch >> SND_PITCH_SHIFT);

            //if (mode == REPLAY) TODO
            {
                StopInstrument(channels[i].sample, NULL);
                StartInstrument(channels[i].sample, NULL);
            }

            return (void*)channels[i].sample;
        }
    }
 
    // TODO
    idx = (idx + 1) % SND_CHANNELS;

    StopInstrument(channels[idx].sample, NULL);
    if (channels[idx].attach) {
        DetachSample(channels[idx].attach);
    }

    channels[idx].attach = AttachSample(channels[idx].sample, samples[index], NULL);
    channels[idx].playing = samples[index];

    StartInstrument(channels[idx].sample, NULL);

    channels[idx].setVolume(0x7FFF * volume >> SND_VOL_SHIFT);
    channels[idx].setPitch(0x2000 * pitch >> SND_PITCH_SHIFT);

    return (void*)channels[idx].sample;
}

void sndPlayTrack(int32 track)
{

}

void sndStopTrack()
{

}

void sndStopSample(int32 index)
{

}

void sndStop()
{

}