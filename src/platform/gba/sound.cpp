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
    // the sound mixer works during VBlank, this is a great opportunity for exclusive access to VRAM without any perf penalties
    // so we use part of offscreen VRAM as sound buffers (704 + 384 = 1088 bytes)
    int32* mixerBuffer = (int32*)(MEM_VRAM + VRAM_PAGE_SIZE + FRAME_WIDTH * FRAME_HEIGHT);
    uint8* soundBuffer = (uint8*)(MEM_VRAM + VRAM_PAGE_SIZE + FRAME_WIDTH * FRAME_HEIGHT + SND_SAMPLES * sizeof(int32)); // use 2k of VRAM after the first frame buffer as sound buffer
#else
    extern const void* TRACKS_IMA;
    int32 mixerBuffer[SND_SAMPLES];
    uint8 soundBuffer[2 * SND_SAMPLES + 32]; // 32 bytes of silence for DMA overrun while interrupt
#endif

#ifdef USE_ASM
    #define sndIMA      sndIMA_asm
    #define sndPCM      sndPCM_asm
    #define sndWrite    sndWrite_asm

    extern "C" {
        void sndIMA_asm(IMA_STATE &state, int32* buffer, const uint8* data, int32 size);
        int32 sndPCM_asm(int32 pos, int32 inc, int32 size, int32 volume, const uint8* data, int32* buffer, int32 count);
        void sndWrite_asm(uint8* buffer, int32 count, int32 *data);
    }
#else
    #define sndIMA      sndIMA_c
    #define sndPCM      sndPCM_c
    #define sndWrite    sndWrite_c

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
    *buffer++ = smp >> (16 - (8 + SND_VOL_SHIFT));

void sndIMA_c(IMA_STATE &state, int32* buffer, const uint8* data, int32 size)
{
    uint32 step, index;

    int32 smp = state.smp;
    int32 idx = state.idx;

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

int32 sndPCM_c(int32 pos, int32 inc, int32 size, int32 volume, const uint8* data, int32* buffer, int32 count)
{
    int32 last = pos + count * inc;
    if (last > size) {
        last = size;
    }

    while (pos < last)
    {
        *buffer++ += SND_DECODE(data[pos >> SND_FIXED_SHIFT]) * volume;
        pos += inc;
    }

    return pos;
}

void sndWrite_c(uint8* buffer, int32 count, int32 *data)
{
    for (int32 i = 0; i < count; i++)
    {
        int32 samp = X_CLAMP(data[i] >> SND_VOL_SHIFT, SND_MIN, SND_MAX);
        buffer[i] = SND_ENCODE(samp);
    }
}
#endif


struct Music
{
    const uint8*  data;
    int32         size;
    int32         pos;
    IMA_STATE     state;

    void fill(int32* buffer, int32 count)
    {
        int32 len = X_MIN(size - pos, count >> 1);

        sndIMA(state, buffer, data + pos, len);

        pos += len;

        if (pos >= size)
        {
            data = NULL;
            memset(buffer, 0, (count - (len << 1)) * sizeof(buffer[0]));
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

    void fill(int32* buffer, int32 count)
    {
        pos = sndPCM(pos, inc, size, volume, data, buffer, count);

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

void sndFill(uint8* buffer, int32 count)
{
#ifdef PROFILE_SOUNDTIME
    PROFILE_CLEAR();
    PROFILE(CNT_SOUND);
#endif

    if ((channelsCount == 0) && !music.data)
    {
        dmaFill(buffer, SND_ENCODE(0), count);
        return;
    }

    if (music.data) {
        music.fill(mixerBuffer, count);
    } else {
        dmaFill(mixerBuffer, 0, SND_SAMPLES * sizeof(int32));
    }

    int32 ch = channelsCount;
    while (ch--)
    {
        Sample* sample = channels + ch;

        sample->fill(mixerBuffer, count);

        if (!sample->data) {
            channels[ch] = channels[--channelsCount];
        }
    }

    sndWrite(buffer, count, mixerBuffer);
}
