#include "common.h"

uint8 ADPCM4_ADAPT[] = { // IWRAM !
    192,192,136,136,128,128,128,128, // -8..-1
    112,128,128,128,128,136,136,192, //  0..+7
};

#if defined(__GBA__) && defined(USE_ASM)
    extern const uint8_t TRACKS_AD4[];
#else
    extern const void* TRACKS_AD4;
#endif

int8 soundBuffer[2 * SND_SAMPLES + 32]; // 32 bytes of silence for DMA overrun while interrupt

#ifdef USE_ASM
    #define sndADPCM4_fill sndADPCM4_fill_asm
    #define sndPCM_fill    sndPCM_fill_asm
    #define sndPCM_mix     sndPCM_mix_asm
    #define sndClear       sndClear_asm

    extern "C" {
        void sndClear_asm(int8* buffer);
        void sndADPCM4_fill_asm(ADPCM4_STATE &state, int8* buffer, const uint8* data, int32 size);
        int32 sndPCM_fill_asm(int32 pos, int32 inc, int32 size, int32 volume, const uint8* data, int8* buffer);
        int32 sndPCM_mix_asm(int32 pos, int32 inc, int32 size, int32 volume, const uint8* data, int8* buffer);
    }
#else
    #define sndADPCM4_fill sndADPCM4_c
    #define sndPCM_fill    sndPCM_c
    #define sndPCM_mix     sndPCM_c
    #define sndClear(b)    dmaFill(b, SND_ENCODE(0), SND_SAMPLES * sizeof(b[0]))

#define DECODE_ADPCM4(n)\
    tap = zM2 + tap - (tap >> 3);\
    *buffer++ = SND_ENCODE(X_CLAMP(tap >> 8, SND_MIN, SND_MAX));\
    res = ((n&0xF) ^ 8) - 8;\
    out = res*quant + (zM1 - zM2);\
    zM2 = zM1;\
    zM1 = out;\
    quant = (quant*(int32)ADPCM4_ADAPT[res+8] + 127) >> 7;\

void sndADPCM4_c(ADPCM4_STATE &state, int8* buffer, const uint8* data, int32 size)
{
    int32 zM1   = state.zM1;
    int32 zM2   = state.zM2;
    int32 tap   = state.tap;
    int32 quant = state.quant;
    int32 res, out;
    
    for (int32 i=0; i < size; i++)
    {
        uint32 n = *data++;
        DECODE_ADPCM4(n);
        n >>= 4;
        DECODE_ADPCM4(n);
    }
    
    state.zM1   = zM1;
    state.zM2   = zM2;
    state.tap   = tap;
    state.quant = quant;
}

int32 sndPCM_c(int32 pos, int32 inc, int32 size, int32 volume, const uint8* data, int8* buffer)
{
    int32 last = pos + SND_SAMPLES * inc;
    if (last > size) {
        last = size;
    }

    while (pos < last)
    {
        int32 amp = SND_DECODE(*(uint8*)buffer) + ((SND_DECODE(data[pos >> SND_FIXED_SHIFT]) * volume) >> SND_VOL_SHIFT);
        *buffer++ = SND_ENCODE(X_CLAMP(amp, SND_MIN, SND_MAX));
        pos += inc;
    }

    return pos;
}
#endif

struct Music
{
    const uint8*  data;
    int32         size;
    int32         pos;
    ADPCM4_STATE  state;

    void fill(int8* buffer)
    {
        int32 len = X_MIN(size - pos, SND_SAMPLES >> 1);

        sndADPCM4_fill(state, buffer, data + pos, len);

        pos += len;

        if (pos >= size)
        {
            data = NULL;
            memset(buffer + (len << 1), 0, (SND_SAMPLES - (len << 1)) * sizeof(buffer[0]));
        }
    }
};

struct Sample
{
    int32        pos;
    int32        inc;
    int32        size;
    int32        volume;
    const uint8* data;

    void mix(int8* buffer)
    {
        pos = sndPCM_mix(pos, inc, size, volume, data, buffer);

        if (pos >= size)
        {
            data = NULL;
        }
    }

    void fill(int8* buffer)
    {
        pos = sndPCM_fill(pos, inc, size, volume, data, buffer);

        if (pos >= size)
        {
            data = NULL;
        }
    }
};

EWRAM_DATA Music  music;
EWRAM_DATA Sample channels[SND_CHANNELS];
EWRAM_DATA int32  channelsCount;

#define CALC_INC (((SND_SAMPLE_FREQ << SND_FIXED_SHIFT) / SND_OUTPUT_FREQ) * pitch >> SND_PITCH_SHIFT)

void sndInit()
{
    // initialized in main.cpp
}

void sndInitSamples()
{
    // nothing to do
}

void sndFreeSamples()
{
    // nothing to do
}

void* sndPlaySample(int32 index, int32 volume, int32 pitch, int32 mode)
{
    if (!gSettings.audio_sfx)
        return NULL;

    const uint8 *data = level.soundData + level.soundOffsets[index];
    int32 size = *(int32*)data;
    data += 4;

    if (mode == UNIQUE || mode == REPLAY)
    {
        for (int32 i = 0; i < channelsCount; i++)
        {
            Sample* sample = channels + i;

            if (sample->data != data)
                continue;

            sample->inc = CALC_INC;
            sample->volume = volume;

            if (mode == REPLAY)
            {
                sample->pos = 0;
            }

            return sample;
        }
    }

    if (channelsCount >= SND_CHANNELS)
        return NULL;

    Sample* sample = channels + channelsCount++;
    sample->data = data;
    sample->size = size << SND_FIXED_SHIFT;
    sample->pos  = 0;
    sample->inc  = CALC_INC;
    sample->volume = volume;

    return sample;
}

void sndPlayTrack(int32 track)
{
    if (!gSettings.audio_music)
        return;

    if (track == gCurTrack)
        return;

    gCurTrack = track;

    if (track == -1) {
        sndStopTrack();
        return;
    }

    struct TrackInfo {
        int32 offset;
        int32 size;
    };
    
    const TrackInfo* info = (const TrackInfo*)TRACKS_AD4 + track;

    if (!info->size)
        return;

    // Clear music.data before setup, and write it after to ensure
    // music.fill() has a consistent state at any point in time
    music.data = NULL;
    music.size = info->size;
    music.pos = 0;
    //music.volume = (1 << SND_VOL_SHIFT);
    music.state.zM1   = 0;
    music.state.zM2   = 0;
    music.state.tap   = 0;
    music.state.quant = 0x0800;
    music.data = (const uint8*)TRACKS_AD4 + info->offset;
}

void sndStopTrack()
{
    music.data = NULL;
    music.size = 0;
    music.pos = 0;
}

bool sndTrackIsPlaying()
{
    return music.data != NULL;
}

void sndStopSample(int32 index)
{
    const uint8 *data = level.soundData + level.soundOffsets[index] + 4;

    int32 i = channelsCount;

    while (--i >= 0)
    {
        if (channels[i].data == data)
        {
            channels[i] = channels[--channelsCount];
        }
    }
}

void sndStop()
{
    channelsCount = 0;
    music.data = NULL;
}

void sndFill(int8* buffer)
{
#ifdef PROFILE_SOUNDTIME
    PROFILE_CLEAR();
    PROFILE(CNT_SOUND);
#endif
    bool mix = (music.data != NULL);

    if (mix) {
        music.fill(buffer);
    } else {
        sndClear(buffer);
    }

    int32 ch = channelsCount;
    while (ch--)
    {
        Sample* sample = channels + ch;

        if (mix)
            sample->mix(buffer);
        else
            sample->fill(buffer);

        if (!sample->data) {
            channels[ch] = channels[--channelsCount];
        }

        mix = true;
    }
}
