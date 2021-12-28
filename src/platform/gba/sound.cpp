#include "common.h"

int16 IMA_STEP[89] = { // IWRAM !
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

extern void decodeIMA(IMA_STATE &state, const uint8* data, int32* buffer, int32 size);

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

#ifdef __GBA__
extern const uint8_t TRACKS_IMA[];
#else
extern const void* TRACKS_IMA;
#endif

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
    const uint8 *data = level.soundData + level.soundOffsets[index];

    int32 size;
    memcpy(&size, data + 40, 4); // TODO preprocess and remove wave header
    data += 44;

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
    const uint8 *data = level.soundData + level.soundOffsets[index] + 44;

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
        buffer[i] = SND_ENCODE(samp);
    }
}
