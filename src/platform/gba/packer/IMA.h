#ifndef H_IMA
#define H_IMA

#include "common.h"

#define CLIP(x,lo,hi) \
    if ( x < lo ) \
    { \
        x = lo; \
    } \
    else if ( x > hi ) \
    { \
        x = hi; \
    }

// ADPCM.h - Common ADPCM definitions
static short gIndexDeltas[16] = {
    -1,-1,-1,-1, 2, 4, 6, 8,
    -1,-1,-1,-1, 2, 4, 6, 8
};

/*  DVI ADPCM step table */
static short gStepSizes[89] = {
    7,     8,     9,    10,    11,    12,    13,    14,    16,     17,    19,
    21,    23,    25,    28,    31,    34,    37,    41,    45,     50,    55,
    60,    66,    73,    80,    88,    97,   107,   118,   130,    143,   157,
    173,   190,   209,   230,   253,   279,   307,   337,   371,    408,   449,
    494,   544,   598,   658,   724,   796,   876,   963,  1060,   1166,  1282,
    1411,  1552,  1707,  1878,  2066,  2272,  2499,  2749,  3024,   3327,  3660,
    4026,  4428,  4871,  5358,  5894,  6484,  7132,  7845,  8630,   9493, 10442,
11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623,  27086, 29794,
32767  };

// Intel DVI ADPCM (ADP4) encoder based on the original SoundHack code https://github.com/tomerbe/SoundHack
long lastEstimateL, stepSizeL, stepIndexL;
long lastEstimateR, stepSizeR, stepIndexR;

char EncodeDelta( long stepSize, long delta )
{
    char encodedSample = 0;
    
    if ( delta < 0L )
    {
        encodedSample = 8;
        delta = -delta;
    }
    if ( delta >= stepSize )
    {
        encodedSample |= 4;
        delta -= stepSize;
    }
    stepSize = stepSize >> 1;
    if ( delta >= stepSize )
    {
        encodedSample |= 2;
        delta -= stepSize;
    }
    stepSize = stepSize >> 1;
    if ( delta >= stepSize ) encodedSample |= 1;
    
    return ( encodedSample );
}

long DecodeDelta( long stepSize, char encodedSample )
{
    long delta = 0;
    
    if( encodedSample & 4) delta = stepSize;
    if( encodedSample & 2) delta += (stepSize >> 1);
    if( encodedSample & 1) delta += (stepSize >> 2);
    delta += (stepSize >> 3);
    if (encodedSample & 8) delta = -delta;
    
    return( delta );
}

char ADDVIEncode(short shortOne, short shortTwo, long channels)
{
    long            delta;
    unsigned char    encodedSample, outputByte;

    outputByte = 0;
    
/* First sample or left sample to be packed in first nibble */
/* calculate delta */
    delta = shortOne - lastEstimateL;
    CLIP(delta, -32768L, 32767L);

/* encode delta relative to the current stepsize */
    encodedSample = EncodeDelta(stepSizeL, delta);

/* pack first nibble */
    outputByte = 0x00F0 & (encodedSample<<4);

/* decode ADPCM code value to reproduce delta and generate an estimated InputSample */
    lastEstimateL += DecodeDelta(stepSizeL, encodedSample);
    CLIP(lastEstimateL, -32768L, 32767L);

/* adapt stepsize */
    stepIndexL += gIndexDeltas[encodedSample];
    CLIP(stepIndexL, 0, 88);
    stepSizeL = gStepSizes[stepIndexL];
    
    if(channels == 2L)
    {
/* calculate delta for second sample */
        delta = shortTwo - lastEstimateR;
        CLIP(delta, -32768L, 32767L);

/* encode delta relative to the current stepsize */
        encodedSample = EncodeDelta(stepSizeR, delta);

/* pack second nibble */
        outputByte |= 0x000F & encodedSample;

/* decode ADPCM code value to reproduce delta and generate an estimated InputSample */
        lastEstimateR += DecodeDelta(stepSizeR, encodedSample);
        CLIP(lastEstimateR, -32768L, 32767L);

/* adapt stepsize */
        stepIndexR += gIndexDeltas[encodedSample];
        CLIP(stepIndexR, 0, 88);
        stepSizeR = gStepSizes[stepIndexR];
    }
    else
    {
/* calculate delta for second sample */
        delta = shortTwo - lastEstimateL;
        CLIP(delta, -32768L, 32767L);

/* encode delta relative to the current stepsize */
        encodedSample = EncodeDelta(stepSizeL, delta);

/* pack second nibble */
        outputByte |= 0x000F & encodedSample;

/* decode ADPCM code value to reproduce delta and generate an estimated InputSample */
        lastEstimateL += DecodeDelta(stepSizeL, encodedSample);
        CLIP(lastEstimateL, -32768L, 32767L);

/* adapt stepsize */
        stepIndexL += gIndexDeltas[encodedSample];
        CLIP(stepIndexL, 0, 88);
        stepSizeL = gStepSizes[stepIndexL];
    }
    return(outputByte);
}

void BlockADDVIEncode(uint8 *buffer, short *samples, long numSamples, long channels)
{
    long i, j;

    lastEstimateL = lastEstimateR = 0L;
    stepSizeL = stepSizeR = 7L;
    stepIndexL = stepIndexR = 0L;
        
    for(i = j = 0; i < numSamples; i += 2, j++)
    {
        buffer[j] = ADDVIEncode(samples[i + 0], samples[i + 1], channels);
    }
}

#endif