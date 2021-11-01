#include "common.h"

Item sndMixer;
Item testSample;

struct Channel
{
    Item gainL;
    Item gainR;
    Item sample;
    Item attach;
    Item frequency;
    Item amplitude;

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

        channels[i].sample = LoadInstrument("varmono8.dsp", 0, 100);
        channels[i].frequency = GrabKnob(channels[i].sample, "Frequency");
        channels[i].amplitude = GrabKnob(channels[i].sample, "Amplitude");
        ConnectInstruments(channels[i].sample, "Output", sndMixer, InputName);

        channels[i].setVolume(0x7FFF);
        channels[i].setPitch(0x8000 / 10);
    }

    StartInstrument(sndMixer, NULL);
}

void sndInitSamples()
{
    for (int32 i = 0; i < level.soundOffsetsCount; i++)
    {
        uint8* data = (uint8*)level.soundData + level.soundOffsets[i];

        uint32 size;
        memcpy(&size, data + 4, sizeof(size));
        size += 8;

        samples[i] = CreateSampleVA(
            AF_TAG_IMAGE_ADDRESS,  data,
            AF_TAG_IMAGE_LENGTH,   size,
            AF_TAG_LEAVE_IN_PLACE, TRUE,
            TAG_END
        );
    }
}

int32 idx = 0;

void* sndPlaySample(int32 index, int32 volume, int32 pitch, int32 mode)
{
    idx = (idx + 1) % SND_CHANNELS;

    StopInstrument(channels[idx].sample, NULL);
    if (channels[idx].attach) {
        DetachSample(channels[idx].attach);
    }

    if (samples[index] < 0) {
        printf("fuck!\n");
    }

    channels[idx].attach = AttachSample(channels[idx].sample, samples[index], NULL);
    StartInstrument(channels[idx].sample, NULL);

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