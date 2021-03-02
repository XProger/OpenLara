#ifndef H_SOUND
#define H_SOUND

#include "common.h"

struct Sound
{
    enum SoundMode {
        UNIQUE,
        REPLAY,
        LOOP,
    };

    struct Sample
    {
        const uint8* data;
        int32        size;
        int32        pos;
        int32        inc;
        int32        volume;

        X_INLINE void fill(int16* buffer, int32 count)
        {
            for (int32 i = 0; i < count; i++)
            {
                buffer[i] += SND_DECODE(data[pos >> SND_FIXED_SHIFT]) * volume >> SND_VOL_SHIFT;

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

    Sample channels[SND_CHANNELS];
    int32  channelsCount;

    void fill(uint8* buffer, int32 count)
    {
        if (channelsCount == 0)
        {
            dmaFill(buffer, SND_ENCODE(0), count);
            return;
        }

        int16 tmp[SND_SAMPLES];
        dmaFill(tmp, 0, sizeof(tmp));

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
            buffer[i] = SND_ENCODE(X_CLAMP(tmp[i], -128, 127));
        }
    }

    #define CALC_INC (((SND_SAMPLE_FREQ << SND_FIXED_SHIFT) / SND_OUTPUT_FREQ) * pitch >> SND_PITCH_SHIFT)

    Sample* play(const uint8* data, int32 size, int32 volume, int32 pitch, int32 mode)
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

        if (channelsCount >= SND_CHANNELS) {
            return NULL;
        }

        Sample* sample = channels + channelsCount++;
        sample->data = data;
        sample->size = size << SND_FIXED_SHIFT;
        sample->pos  = 0;
        sample->inc  = CALC_INC;
        sample->volume = volume + 1;

        return sample;
    }

    void init()
    {
        channelsCount = 0;
    }
};

Sound sound;

#endif
