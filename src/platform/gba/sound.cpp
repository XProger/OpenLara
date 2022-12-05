#include "common.h"

int32 IMA_STEP[] = { // IWRAM !
    7,     8,     9,     10,    11,    12,    13,    14,
    16,    17,    19,    21,    23,    25,    28,    31,
    34,    37,    41,    45,    50,    55,    60,    66,
    73,    80,    88,    97,    107,   118,   130,   143,
    157,   173,   190,   209,   230,   253,   279,   307,
    337,   371,   408,   449,   494,   544,   598,   658,
    724,   796,   876,   963,   1060,  1166,  1282,  1411,
    1552,  1707,  1878,  2066,  2272,  2499,  2749,  3024,
    3327,  3660,  4026,  4428,  4871,  5358,  5894,  6484,
    7132,  7845,  8630,  9493,  10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794,
    32767
};

#if defined(__GBA__) && defined(USE_ASM)
    extern const uint8_t TRACKS_IMA[];
#else
    extern const void* TRACKS_IMA;
#endif

int8 soundBuffer[2 * SND_SAMPLES + 32]; // 32 bytes of silence for DMA overrun while interrupt

#ifdef USE_ASM
    #define sndIMA_fill sndIMA_fill_asm
    #define sndPCM_fill sndPCM_fill_asm
    #define sndPCM_mix  sndPCM_mix_asm
    #define sndClear    sndClear_asm

    extern "C" {
        void sndClear_asm(int8* buffer);
        void sndIMA_fill_asm(IMA_STATE &state, int8* buffer, const uint8* data, int32 size);
        int32 sndPCM_fill_asm(int32 pos, int32 inc, int32 size, int32 volume, const uint8* data, int8* buffer);
        int32 sndPCM_mix_asm(int32 pos, int32 inc, int32 size, int32 volume, const uint8* data, int8* buffer);
    }
#else
    #define sndIMA_fill sndIMA_c
    #define sndPCM_fill sndPCM_c
    #define sndPCM_mix  sndPCM_c
    #define sndClear(b) dmaFill(b, SND_ENCODE(0), SND_SAMPLES * sizeof(b[0]))

#define DECODE_IMA_4(n)\
    step = IMA_STEP[idx];\
    index = n & 7;\
    step += index * step << 1;\
    if (index < 4) {\
        idx = X_MAX(idx - 1, 0);\
    } else {\
        idx = X_MIN(idx + ((index - 3) << 1), X_COUNT(IMA_STEP) - 1);\
    }\
    if (n & 8) {\
        smp -= step >> 3;\
    } else {\
        smp += step >> 3;\
    }\
    amp = smp >> 8;\
    *buffer++ = SND_ENCODE(X_CLAMP(amp, SND_MIN, SND_MAX));

void sndIMA_c(IMA_STATE &state, int8* buffer, const uint8* data, int32 size)
{
    uint32 step, index;

    int32 smp = state.smp;
    int32 idx = state.idx;
    int32 amp;

    for (int32 i = 0; i < size; i++)
    {
        uint32 n = *data++;
        DECODE_IMA_4(n);
        n >>= 4;
        DECODE_IMA_4(n);
    }

    state.smp = smp;
    state.idx = idx;
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
    IMA_STATE     state;

    void fill(int8* buffer)
    {
        int32 len = X_MIN(size - pos, SND_SAMPLES >> 1);

        sndIMA_fill(state, buffer, data + pos, len);

        pos += len;

        if (pos >= size)
        {
            data = NULL;
            memset(buffer, 0, (SND_SAMPLES - (len << 1)) * sizeof(buffer[0]));
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
    
    const TrackInfo* info = (const TrackInfo*)TRACKS_IMA + track;

    if (!info->size)
        return;

    music.data = (uint8*)TRACKS_IMA + info->offset;
    music.size = info->size;
    music.pos = 0;
    //music.volume = (1 << SND_VOL_SHIFT);
    music.state.smp = 0;
    music.state.idx = 0;
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
