#ifndef H_SOUND
#define H_SOUND

#include "common.h"

int16 IMA_INDEX[] = { // IWRAM !
    -1, -1, -1, -1, 2, 4, 6, 8,
};

int16 IMA_STEP[] = { // IWRAM !
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

struct Mixer
{
    enum SoundMode {
        UNIQUE,
        REPLAY,
        LOOP,
    };

    struct Music
    {
        const uint8*  data;
        int32         size;
        int32         pos;
        int32         smp;
        int32         idx;

        X_INLINE int32 getSample(uint32 n)
        {
            int32 step = IMA_STEP[idx];
            int32 index = n & 7;

            idx = X_CLAMP(idx + IMA_INDEX[index], 0, 88);

            int32 diff = (2 * index + 1) * step >> 3;

            if (n & 8) {
                smp = X_MAX(smp - diff, -32768);
            } else {
                smp = X_MIN(smp + diff, 32767);
            }

            return smp >> 1;
        }

        X_INLINE void fill(int32* buffer, int32 count)
        {
            for (int32 i = 0; i < count; i += 2)
            {
                if (pos >= size)
                {
                    data = NULL;
                    memset(buffer, 0, (count - i) * sizeof(buffer[0]));
                    return;
                }

                uint32 n = data[pos++];
                *buffer++ = getSample(n);
                *buffer++ = getSample(n >> 4);
            }
        }
    };

    struct Sample
    {
        const uint8* data;
        int32        size;
        int32        pos;
        int32        inc;
        int32        volume;

        X_INLINE void fill(int32* buffer, int32 count)
        {
            for (int32 i = 0; i < count; i++)
            {
                buffer[i] += SND_DECODE(data[pos >> SND_FIXED_SHIFT]) * volume;

                pos += inc;
                if (pos >= size)
                {
                    // TODO LOOP
                    data = NULL;
                    return;
                }
            }
        }
    };

    Music  music;
    Sample channels[SND_CHANNELS];
    int32  channelsCount;

    void fill(uint8* bufferA, uint8* bufferB, int32 count)
    {
        if ((channelsCount == 0) && !music.data)
        {
            dmaFill(bufferA, SND_ENCODE(0), count);
        #ifdef USE_9BIT_SOUND
            dmaFill(bufferB, SND_ENCODE(0), count);
        #endif
            return;
        }

        int32 tmp[SND_SAMPLES];

        if (music.data) {
            music.fill(tmp, count);
        } else {
            dmaFill(tmp, 0, sizeof(tmp));
        }

        int32 ch = channelsCount;
        while (ch--)
        {
            Sample* sample = channels + ch;

            sample->fill(tmp, count);

            if (!sample->data) {
                channels[ch] = channels[--channelsCount];
            }
        }

        for (int32 i = 0; i < count; i++)
        {
            int32 samp = X_CLAMP(tmp[i] >> SND_VOL_SHIFT, SND_MIN, SND_MAX);

        #if defined(_WIN32)
            bufferA[i] = SND_ENCODE(samp);
        #elif defined(__GBA__)
            #ifdef USE_9BIT_SOUND
                bufferA[i] = (samp >> 1);
                bufferB[i] = (samp >> 1) + (samp & 1); // TODO
            #else
                bufferA[i] = samp;
            #endif
        #endif
        }
    }

    #define CALC_INC (((SND_SAMPLE_FREQ << SND_FIXED_SHIFT) / SND_OUTPUT_FREQ) * pitch >> SND_PITCH_SHIFT)

    Sample* playSample(const uint8* data, int32 size, int32 volume, int32 pitch, int32 mode)
    {
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

    #ifdef USE_9BIT_SOUND
        // expand 8 to 9-bit
        volume <<= 1;
    #endif

        Sample* sample = channels + channelsCount++;
        sample->data = data;
        sample->size = size << SND_FIXED_SHIFT;
        sample->pos  = 0;
        sample->inc  = CALC_INC;
        sample->volume = volume + 1;

        return sample;
    }

    void stopSample(const uint8* data)
    {
        int32 i = channelsCount;

        while (--i >= 0)
        {
            if (channels[i].data == data)
            {
                channels[i] = channels[--channelsCount];
            }
        }
    }

    void playMusic(const void* data)
    {
        music.data   = (uint8*)data + 16;
        music.size   = *((int32*)data + 2);
        music.pos    = 0;
        //music.volume = (1 << SND_VOL_SHIFT);
        music.smp    = 0;
        music.idx    = 0;
    }

    void init()
    {
        channelsCount = 0;
        music.data = NULL;
    }
};

Mixer mixer;

#endif
