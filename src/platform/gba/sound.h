#ifndef H_SOUND
#define H_SOUND

#include "common.h"

void decodeIMA(IMA_STATE &state, const uint8* data, int32* buffer, int32 size);

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
        IMA_STATE     state;

        void fill(int32* buffer, int32 count)
        {
            int32 len = X_MIN(size - pos, count >> 1);

            decodeIMA(state, data + pos, buffer, len);

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
        const uint8* data;
        int32        size;
        int32        pos;
        int32        inc;
        int32        volume;

        void fill(int32* buffer, int32 count)
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
        sample->volume = volume;

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

    void playMusic(const void* data, int32 size)
    {
        music.data = (uint8*)data;
        music.size = size;
        music.pos = 0;
        //music.volume = (1 << SND_VOL_SHIFT);
        music.state.smp = 0;
        music.state.idx = 0;
    }

    void stopMusic()
    {
        music.data = NULL;
    }

    void init()
    {
        channelsCount = 0;
        music.data = NULL;
    }

    void fill(uint8* bufferA, uint8* bufferB, int32 count)
    {
    #ifdef PROFILE_SOUNDTIME
        PROFILE_CLEAR();
        PROFILE(CNT_SOUND);
    #endif

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
        #ifdef USE_9BIT_SOUND
            int32 samp = tmp[i] >> (SND_VOL_SHIFT - 1);
            //samp += (rand_draw() & 1) - 1;
            samp = X_CLAMP(samp, SND_MIN, SND_MAX);

            bufferA[i] = SND_ENCODE((samp >> 1));
            bufferB[i] = SND_ENCODE((samp >> 1) + (samp & 1));
        #else
            int32 samp = X_CLAMP(tmp[i] >> SND_VOL_SHIFT, SND_MIN, SND_MAX);

            bufferA[i] = SND_ENCODE(samp);
        #endif
        }
    }
};

Mixer mixer;

#endif
