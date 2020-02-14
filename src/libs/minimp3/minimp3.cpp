/*
 * MPEG Audio Layer III decoder
 * Copyright (c) 2001, 2002 Fabrice Bellard,
 *           (c) 2007 Martin J. Fiedler
 *
 * This file is a stripped-down version of the MPEG Audio decoder from
 * the FFmpeg libavcodec library.
 *
 * FFmpeg and minimp3 are free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg and minimp3 are distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef _MSC_VER
    #define INLINE __forceinline
    #define FASTCALL __fastcall
    #ifdef NOLIBC
        #ifdef MAIN_PROGRAM
            int _fltused=0;
        #endif
    #endif
#else
    #define INLINE inline
    #define FASTCALL __attribute__((fastcall))
    #include <stdint.h>
#endif

typedef unsigned char       uint8;
typedef   signed char        int8;
typedef unsigned short      uint16;
typedef   signed short       int16;
typedef unsigned int        uint32;
typedef   signed int         int32;
typedef unsigned long long  uint64;
typedef   signed long long   int64;

#define PI 3.14159265358979

#define libc_malloc  malloc
#define libc_calloc  calloc
#define libc_realloc realloc
#define libc_free    free

#define libc_memset  memset
#define libc_memcpy  memcpy
#define libc_memmove memmove

#define libc_exp     exp
#define libc_pow     pow

#if defined(_MSC_VER) && !defined(_DEBUG) 
static INLINE double libc_frexp(double x, int *e) {
    double res = -9999.999;
    unsigned __int64 i = *(unsigned __int64*)(&x);
    if (!(i & 0x7F00000000000000UL)) {
        *e = 0;
        return x;
    }
    *e = ((i << 1) >> 53) - 1022;
    i &= 0x800FFFFFFFFFFFFFUL;
    i |= 0x3FF0000000000000UL;
    return *(double*)(&i) * 0.5;
}
#else
    #define libc_frexp  frexp
#endif

#include "minimp3.h"

#define MP3_FRAME_SIZE 1152
#define MP3_MAX_CODED_FRAME_SIZE 1792
#define MP3_MAX_CHANNELS 2
#define SBLIMIT 32

#define MP3_STEREO  0
#define MP3_JSTEREO 1
#define MP3_DUAL    2
#define MP3_MONO    3

#define SAME_HEADER_MASK \
   (0xffe00000 | (3 << 17) | (0xf << 12) | (3 << 10) | (3 << 19))

#define FRAC_BITS   15
#define WFRAC_BITS  14

#define OUT_MAX (32767)
#define OUT_MIN (-32768)
#define OUT_SHIFT (WFRAC_BITS + FRAC_BITS - 15)

#define MODE_EXT_MS_STEREO 2
#define MODE_EXT_I_STEREO  1

#define FRAC_ONE    (1 << FRAC_BITS)
#define FIX(a)   ((int)((a) * FRAC_ONE))
#define FIXR(a)   ((int)((a) * FRAC_ONE + 0.5))
#define FRAC_RND(a) (((a) + (FRAC_ONE/2)) >> FRAC_BITS)
#define FIXHR(a) ((int)((a) * (1LL<<32) + 0.5))

#define MULL(a,b) int32(((int64)(a) * (int64)(b)) >> FRAC_BITS)
#define MULH(a,b) int32(((int64)(a) * (int64)(b)) >> 32)
#define MULS(ra, rb) ((ra) * (rb))

#define ISQRT2 FIXR(0.70710678118654752440)

#define HEADER_SIZE 4
#define BACKSTEP_SIZE 512
#define EXTRABYTES 24

#define VLC_TYPE int16

////////////////////////////////////////////////////////////////////////////////

struct _granule;

typedef struct _bitstream {
    const uint8 *buffer, *buffer_end;
    int index;
    int size_in_bits;
} bitstream_t;

typedef struct _vlc {
    int bits;
    VLC_TYPE (*table)[2]; ///< code, bits
    int table_size, table_allocated;
} vlc_t;

typedef struct _mp3_context {
    uint8 last_buf[2*BACKSTEP_SIZE + EXTRABYTES];
    int last_buf_size;
    int frame_size;
    uint32 free_format_next_header;
    int error_protection;
    int sample_rate;
    int sample_rate_index;
    int bit_rate;
    bitstream_t gb;
    bitstream_t in_gb;
    int nb_channels;
    int mode;
    int mode_ext;
    int lsf;
    int16 synth_buf[MP3_MAX_CHANNELS][512 * 2];
    int synth_buf_offset[MP3_MAX_CHANNELS];
    int32 sb_samples[MP3_MAX_CHANNELS][36][SBLIMIT];
    int32 mdct_buf[MP3_MAX_CHANNELS][SBLIMIT * 18];
    int dither_state;
} mp3_context_t;

typedef struct _granule {
    uint8 scfsi;
    int part2_3_length;
    int big_values;
    int global_gain;
    int scalefac_compress;
    uint8 block_type;
    uint8 switch_point;
    int table_select[3];
    int subblock_gain[3];
    uint8 scalefac_scale;
    uint8 count1table_select;
    int region_size[3];
    int preflag;
    int short_start, long_end;
    uint8 scale_factors[40];
    int32 sb_hybrid[SBLIMIT * 18];
} granule_t;

typedef struct _huff_table {
    int xsize;
    const uint8 *bits;
    const uint16 *codes;
} huff_table_t;

static vlc_t huff_vlc[16];
static vlc_t huff_quad_vlc[2];
static uint16 band_index_long[9][23];
#define TABLE_4_3_SIZE (8191 + 16)*4
static int8  *table_4_3_exp;
static uint32 *table_4_3_value;
static uint32 exp_table[512];
static uint32 expval_table[512][16];
static int32 is_table[2][16];
static int32 is_table_lsf[2][2][16];
static int32 csa_table[8][4];
static float csa_table_float[8][4];
static int32 mdct_win[8][36];
static int16 window[512];

////////////////////////////////////////////////////////////////////////////////

static const uint16 mp3_bitrate_tab[2][15] = {
    {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320 },
    {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160}
};

static const uint16 mp3_freq_tab[3] = { 44100, 48000, 32000 };

static const int32 mp3_enwindow[257] = {
     0,    -1,    -1,    -1,    -1,    -1,    -1,    -2,
    -2,    -2,    -2,    -3,    -3,    -4,    -4,    -5,
    -5,    -6,    -7,    -7,    -8,    -9,   -10,   -11,
   -13,   -14,   -16,   -17,   -19,   -21,   -24,   -26,
   -29,   -31,   -35,   -38,   -41,   -45,   -49,   -53,
   -58,   -63,   -68,   -73,   -79,   -85,   -91,   -97,
  -104,  -111,  -117,  -125,  -132,  -139,  -147,  -154,
  -161,  -169,  -176,  -183,  -190,  -196,  -202,  -208,
   213,   218,   222,   225,   227,   228,   228,   227,
   224,   221,   215,   208,   200,   189,   177,   163,
   146,   127,   106,    83,    57,    29,    -2,   -36,
   -72,  -111,  -153,  -197,  -244,  -294,  -347,  -401,
  -459,  -519,  -581,  -645,  -711,  -779,  -848,  -919,
  -991, -1064, -1137, -1210, -1283, -1356, -1428, -1498,
 -1567, -1634, -1698, -1759, -1817, -1870, -1919, -1962,
 -2001, -2032, -2057, -2075, -2085, -2087, -2080, -2063,
  2037,  2000,  1952,  1893,  1822,  1739,  1644,  1535,
  1414,  1280,  1131,   970,   794,   605,   402,   185,
   -45,  -288,  -545,  -814, -1095, -1388, -1692, -2006,
 -2330, -2663, -3004, -3351, -3705, -4063, -4425, -4788,
 -5153, -5517, -5879, -6237, -6589, -6935, -7271, -7597,
 -7910, -8209, -8491, -8755, -8998, -9219, -9416, -9585,
 -9727, -9838, -9916, -9959, -9966, -9935, -9863, -9750,
 -9592, -9389, -9139, -8840, -8492, -8092, -7640, -7134,
  6574,  5959,  5288,  4561,  3776,  2935,  2037,  1082,
    70,  -998, -2122, -3300, -4533, -5818, -7154, -8540,
 -9975,-11455,-12980,-14548,-16155,-17799,-19478,-21189,
-22929,-24694,-26482,-28289,-30112,-31947,-33791,-35640,
-37489,-39336,-41176,-43006,-44821,-46617,-48390,-50137,
-51853,-53534,-55178,-56778,-58333,-59838,-61289,-62684,
-64019,-65290,-66494,-67629,-68692,-69679,-70590,-71420,
-72169,-72835,-73415,-73908,-74313,-74630,-74856,-74992,
 75038,
};

static const uint8 slen_table[2][16] = {
    { 0, 0, 0, 0, 3, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4 },
    { 0, 1, 2, 3, 0, 1, 2, 3, 1, 2, 3, 1, 2, 3, 2, 3 },
};

static const uint8 lsf_nsf_table[6][3][4] = {
    { {  6,  5,  5, 5 }, {  9,  9,  9, 9 }, {  6,  9,  9, 9 } },
    { {  6,  5,  7, 3 }, {  9,  9, 12, 6 }, {  6,  9, 12, 6 } },
    { { 11, 10,  0, 0 }, { 18, 18,  0, 0 }, { 15, 18,  0, 0 } },
    { {  7,  7,  7, 0 }, { 12, 12, 12, 0 }, {  6, 15, 12, 0 } },
    { {  6,  6,  6, 3 }, { 12,  9,  9, 6 }, {  6, 12,  9, 6 } },
    { {  8,  8,  5, 0 }, { 15, 12,  9, 0 }, {  6, 18,  9, 0 } },
};

static const uint16 mp3_huffcodes_1[4] = {
 0x0001, 0x0001, 0x0001, 0x0000,
};

static const uint8 mp3_huffbits_1[4] = {
  1,  3,  2,  3,
};

static const uint16 mp3_huffcodes_2[9] = {
 0x0001, 0x0002, 0x0001, 0x0003, 0x0001, 0x0001, 0x0003, 0x0002,
 0x0000,
};

static const uint8 mp3_huffbits_2[9] = {
  1,  3,  6,  3,  3,  5,  5,  5,
  6,
};

static const uint16 mp3_huffcodes_3[9] = {
 0x0003, 0x0002, 0x0001, 0x0001, 0x0001, 0x0001, 0x0003, 0x0002,
 0x0000,
};

static const uint8 mp3_huffbits_3[9] = {
  2,  2,  6,  3,  2,  5,  5,  5,
  6,
};

static const uint16 mp3_huffcodes_5[16] = {
 0x0001, 0x0002, 0x0006, 0x0005, 0x0003, 0x0001, 0x0004, 0x0004,
 0x0007, 0x0005, 0x0007, 0x0001, 0x0006, 0x0001, 0x0001, 0x0000,
};

static const uint8 mp3_huffbits_5[16] = {
  1,  3,  6,  7,  3,  3,  6,  7,
  6,  6,  7,  8,  7,  6,  7,  8,
};

static const uint16 mp3_huffcodes_6[16] = {
 0x0007, 0x0003, 0x0005, 0x0001, 0x0006, 0x0002, 0x0003, 0x0002,
 0x0005, 0x0004, 0x0004, 0x0001, 0x0003, 0x0003, 0x0002, 0x0000,
};

static const uint8 mp3_huffbits_6[16] = {
  3,  3,  5,  7,  3,  2,  4,  5,
  4,  4,  5,  6,  6,  5,  6,  7,
};

static const uint16 mp3_huffcodes_7[36] = {
 0x0001, 0x0002, 0x000a, 0x0013, 0x0010, 0x000a, 0x0003, 0x0003,
 0x0007, 0x000a, 0x0005, 0x0003, 0x000b, 0x0004, 0x000d, 0x0011,
 0x0008, 0x0004, 0x000c, 0x000b, 0x0012, 0x000f, 0x000b, 0x0002,
 0x0007, 0x0006, 0x0009, 0x000e, 0x0003, 0x0001, 0x0006, 0x0004,
 0x0005, 0x0003, 0x0002, 0x0000,
};

static const uint8 mp3_huffbits_7[36] = {
  1,  3,  6,  8,  8,  9,  3,  4,
  6,  7,  7,  8,  6,  5,  7,  8,
  8,  9,  7,  7,  8,  9,  9,  9,
  7,  7,  8,  9,  9, 10,  8,  8,
  9, 10, 10, 10,
};

static const uint16 mp3_huffcodes_8[36] = {
 0x0003, 0x0004, 0x0006, 0x0012, 0x000c, 0x0005, 0x0005, 0x0001,
 0x0002, 0x0010, 0x0009, 0x0003, 0x0007, 0x0003, 0x0005, 0x000e,
 0x0007, 0x0003, 0x0013, 0x0011, 0x000f, 0x000d, 0x000a, 0x0004,
 0x000d, 0x0005, 0x0008, 0x000b, 0x0005, 0x0001, 0x000c, 0x0004,
 0x0004, 0x0001, 0x0001, 0x0000,
};

static const uint8 mp3_huffbits_8[36] = {
  2,  3,  6,  8,  8,  9,  3,  2,
  4,  8,  8,  8,  6,  4,  6,  8,
  8,  9,  8,  8,  8,  9,  9, 10,
  8,  7,  8,  9, 10, 10,  9,  8,
  9,  9, 11, 11,
};

static const uint16 mp3_huffcodes_9[36] = {
 0x0007, 0x0005, 0x0009, 0x000e, 0x000f, 0x0007, 0x0006, 0x0004,
 0x0005, 0x0005, 0x0006, 0x0007, 0x0007, 0x0006, 0x0008, 0x0008,
 0x0008, 0x0005, 0x000f, 0x0006, 0x0009, 0x000a, 0x0005, 0x0001,
 0x000b, 0x0007, 0x0009, 0x0006, 0x0004, 0x0001, 0x000e, 0x0004,
 0x0006, 0x0002, 0x0006, 0x0000,
};

static const uint8 mp3_huffbits_9[36] = {
  3,  3,  5,  6,  8,  9,  3,  3,
  4,  5,  6,  8,  4,  4,  5,  6,
  7,  8,  6,  5,  6,  7,  7,  8,
  7,  6,  7,  7,  8,  9,  8,  7,
  8,  8,  9,  9,
};

static const uint16 mp3_huffcodes_10[64] = {
 0x0001, 0x0002, 0x000a, 0x0017, 0x0023, 0x001e, 0x000c, 0x0011,
 0x0003, 0x0003, 0x0008, 0x000c, 0x0012, 0x0015, 0x000c, 0x0007,
 0x000b, 0x0009, 0x000f, 0x0015, 0x0020, 0x0028, 0x0013, 0x0006,
 0x000e, 0x000d, 0x0016, 0x0022, 0x002e, 0x0017, 0x0012, 0x0007,
 0x0014, 0x0013, 0x0021, 0x002f, 0x001b, 0x0016, 0x0009, 0x0003,
 0x001f, 0x0016, 0x0029, 0x001a, 0x0015, 0x0014, 0x0005, 0x0003,
 0x000e, 0x000d, 0x000a, 0x000b, 0x0010, 0x0006, 0x0005, 0x0001,
 0x0009, 0x0008, 0x0007, 0x0008, 0x0004, 0x0004, 0x0002, 0x0000,
};

static const uint8 mp3_huffbits_10[64] = {
  1,  3,  6,  8,  9,  9,  9, 10,
  3,  4,  6,  7,  8,  9,  8,  8,
  6,  6,  7,  8,  9, 10,  9,  9,
  7,  7,  8,  9, 10, 10,  9, 10,
  8,  8,  9, 10, 10, 10, 10, 10,
  9,  9, 10, 10, 11, 11, 10, 11,
  8,  8,  9, 10, 10, 10, 11, 11,
  9,  8,  9, 10, 10, 11, 11, 11,
};

static const uint16 mp3_huffcodes_11[64] = {
 0x0003, 0x0004, 0x000a, 0x0018, 0x0022, 0x0021, 0x0015, 0x000f,
 0x0005, 0x0003, 0x0004, 0x000a, 0x0020, 0x0011, 0x000b, 0x000a,
 0x000b, 0x0007, 0x000d, 0x0012, 0x001e, 0x001f, 0x0014, 0x0005,
 0x0019, 0x000b, 0x0013, 0x003b, 0x001b, 0x0012, 0x000c, 0x0005,
 0x0023, 0x0021, 0x001f, 0x003a, 0x001e, 0x0010, 0x0007, 0x0005,
 0x001c, 0x001a, 0x0020, 0x0013, 0x0011, 0x000f, 0x0008, 0x000e,
 0x000e, 0x000c, 0x0009, 0x000d, 0x000e, 0x0009, 0x0004, 0x0001,
 0x000b, 0x0004, 0x0006, 0x0006, 0x0006, 0x0003, 0x0002, 0x0000,
};

static const uint8 mp3_huffbits_11[64] = {
  2,  3,  5,  7,  8,  9,  8,  9,
  3,  3,  4,  6,  8,  8,  7,  8,
  5,  5,  6,  7,  8,  9,  8,  8,
  7,  6,  7,  9,  8, 10,  8,  9,
  8,  8,  8,  9,  9, 10,  9, 10,
  8,  8,  9, 10, 10, 11, 10, 11,
  8,  7,  7,  8,  9, 10, 10, 10,
  8,  7,  8,  9, 10, 10, 10, 10,
};

static const uint16 mp3_huffcodes_12[64] = {
 0x0009, 0x0006, 0x0010, 0x0021, 0x0029, 0x0027, 0x0026, 0x001a,
 0x0007, 0x0005, 0x0006, 0x0009, 0x0017, 0x0010, 0x001a, 0x000b,
 0x0011, 0x0007, 0x000b, 0x000e, 0x0015, 0x001e, 0x000a, 0x0007,
 0x0011, 0x000a, 0x000f, 0x000c, 0x0012, 0x001c, 0x000e, 0x0005,
 0x0020, 0x000d, 0x0016, 0x0013, 0x0012, 0x0010, 0x0009, 0x0005,
 0x0028, 0x0011, 0x001f, 0x001d, 0x0011, 0x000d, 0x0004, 0x0002,
 0x001b, 0x000c, 0x000b, 0x000f, 0x000a, 0x0007, 0x0004, 0x0001,
 0x001b, 0x000c, 0x0008, 0x000c, 0x0006, 0x0003, 0x0001, 0x0000,
};

static const uint8 mp3_huffbits_12[64] = {
  4,  3,  5,  7,  8,  9,  9,  9,
  3,  3,  4,  5,  7,  7,  8,  8,
  5,  4,  5,  6,  7,  8,  7,  8,
  6,  5,  6,  6,  7,  8,  8,  8,
  7,  6,  7,  7,  8,  8,  8,  9,
  8,  7,  8,  8,  8,  9,  8,  9,
  8,  7,  7,  8,  8,  9,  9, 10,
  9,  8,  8,  9,  9,  9,  9, 10,
};

static const uint16 mp3_huffcodes_13[256] = {
 0x0001, 0x0005, 0x000e, 0x0015, 0x0022, 0x0033, 0x002e, 0x0047,
 0x002a, 0x0034, 0x0044, 0x0034, 0x0043, 0x002c, 0x002b, 0x0013,
 0x0003, 0x0004, 0x000c, 0x0013, 0x001f, 0x001a, 0x002c, 0x0021,
 0x001f, 0x0018, 0x0020, 0x0018, 0x001f, 0x0023, 0x0016, 0x000e,
 0x000f, 0x000d, 0x0017, 0x0024, 0x003b, 0x0031, 0x004d, 0x0041,
 0x001d, 0x0028, 0x001e, 0x0028, 0x001b, 0x0021, 0x002a, 0x0010,
 0x0016, 0x0014, 0x0025, 0x003d, 0x0038, 0x004f, 0x0049, 0x0040,
 0x002b, 0x004c, 0x0038, 0x0025, 0x001a, 0x001f, 0x0019, 0x000e,
 0x0023, 0x0010, 0x003c, 0x0039, 0x0061, 0x004b, 0x0072, 0x005b,
 0x0036, 0x0049, 0x0037, 0x0029, 0x0030, 0x0035, 0x0017, 0x0018,
 0x003a, 0x001b, 0x0032, 0x0060, 0x004c, 0x0046, 0x005d, 0x0054,
 0x004d, 0x003a, 0x004f, 0x001d, 0x004a, 0x0031, 0x0029, 0x0011,
 0x002f, 0x002d, 0x004e, 0x004a, 0x0073, 0x005e, 0x005a, 0x004f,
 0x0045, 0x0053, 0x0047, 0x0032, 0x003b, 0x0026, 0x0024, 0x000f,
 0x0048, 0x0022, 0x0038, 0x005f, 0x005c, 0x0055, 0x005b, 0x005a,
 0x0056, 0x0049, 0x004d, 0x0041, 0x0033, 0x002c, 0x002b, 0x002a,
 0x002b, 0x0014, 0x001e, 0x002c, 0x0037, 0x004e, 0x0048, 0x0057,
 0x004e, 0x003d, 0x002e, 0x0036, 0x0025, 0x001e, 0x0014, 0x0010,
 0x0035, 0x0019, 0x0029, 0x0025, 0x002c, 0x003b, 0x0036, 0x0051,
 0x0042, 0x004c, 0x0039, 0x0036, 0x0025, 0x0012, 0x0027, 0x000b,
 0x0023, 0x0021, 0x001f, 0x0039, 0x002a, 0x0052, 0x0048, 0x0050,
 0x002f, 0x003a, 0x0037, 0x0015, 0x0016, 0x001a, 0x0026, 0x0016,
 0x0035, 0x0019, 0x0017, 0x0026, 0x0046, 0x003c, 0x0033, 0x0024,
 0x0037, 0x001a, 0x0022, 0x0017, 0x001b, 0x000e, 0x0009, 0x0007,
 0x0022, 0x0020, 0x001c, 0x0027, 0x0031, 0x004b, 0x001e, 0x0034,
 0x0030, 0x0028, 0x0034, 0x001c, 0x0012, 0x0011, 0x0009, 0x0005,
 0x002d, 0x0015, 0x0022, 0x0040, 0x0038, 0x0032, 0x0031, 0x002d,
 0x001f, 0x0013, 0x000c, 0x000f, 0x000a, 0x0007, 0x0006, 0x0003,
 0x0030, 0x0017, 0x0014, 0x0027, 0x0024, 0x0023, 0x0035, 0x0015,
 0x0010, 0x0017, 0x000d, 0x000a, 0x0006, 0x0001, 0x0004, 0x0002,
 0x0010, 0x000f, 0x0011, 0x001b, 0x0019, 0x0014, 0x001d, 0x000b,
 0x0011, 0x000c, 0x0010, 0x0008, 0x0001, 0x0001, 0x0000, 0x0001,
};

static const uint8 mp3_huffbits_13[256] = {
  1,  4,  6,  7,  8,  9,  9, 10,
  9, 10, 11, 11, 12, 12, 13, 13,
  3,  4,  6,  7,  8,  8,  9,  9,
  9,  9, 10, 10, 11, 12, 12, 12,
  6,  6,  7,  8,  9,  9, 10, 10,
  9, 10, 10, 11, 11, 12, 13, 13,
  7,  7,  8,  9,  9, 10, 10, 10,
 10, 11, 11, 11, 11, 12, 13, 13,
  8,  7,  9,  9, 10, 10, 11, 11,
 10, 11, 11, 12, 12, 13, 13, 14,
  9,  8,  9, 10, 10, 10, 11, 11,
 11, 11, 12, 11, 13, 13, 14, 14,
  9,  9, 10, 10, 11, 11, 11, 11,
 11, 12, 12, 12, 13, 13, 14, 14,
 10,  9, 10, 11, 11, 11, 12, 12,
 12, 12, 13, 13, 13, 14, 16, 16,
  9,  8,  9, 10, 10, 11, 11, 12,
 12, 12, 12, 13, 13, 14, 15, 15,
 10,  9, 10, 10, 11, 11, 11, 13,
 12, 13, 13, 14, 14, 14, 16, 15,
 10, 10, 10, 11, 11, 12, 12, 13,
 12, 13, 14, 13, 14, 15, 16, 17,
 11, 10, 10, 11, 12, 12, 12, 12,
 13, 13, 13, 14, 15, 15, 15, 16,
 11, 11, 11, 12, 12, 13, 12, 13,
 14, 14, 15, 15, 15, 16, 16, 16,
 12, 11, 12, 13, 13, 13, 14, 14,
 14, 14, 14, 15, 16, 15, 16, 16,
 13, 12, 12, 13, 13, 13, 15, 14,
 14, 17, 15, 15, 15, 17, 16, 16,
 12, 12, 13, 14, 14, 14, 15, 14,
 15, 15, 16, 16, 19, 18, 19, 16,
};

static const uint16 mp3_huffcodes_15[256] = {
 0x0007, 0x000c, 0x0012, 0x0035, 0x002f, 0x004c, 0x007c, 0x006c,
 0x0059, 0x007b, 0x006c, 0x0077, 0x006b, 0x0051, 0x007a, 0x003f,
 0x000d, 0x0005, 0x0010, 0x001b, 0x002e, 0x0024, 0x003d, 0x0033,
 0x002a, 0x0046, 0x0034, 0x0053, 0x0041, 0x0029, 0x003b, 0x0024,
 0x0013, 0x0011, 0x000f, 0x0018, 0x0029, 0x0022, 0x003b, 0x0030,
 0x0028, 0x0040, 0x0032, 0x004e, 0x003e, 0x0050, 0x0038, 0x0021,
 0x001d, 0x001c, 0x0019, 0x002b, 0x0027, 0x003f, 0x0037, 0x005d,
 0x004c, 0x003b, 0x005d, 0x0048, 0x0036, 0x004b, 0x0032, 0x001d,
 0x0034, 0x0016, 0x002a, 0x0028, 0x0043, 0x0039, 0x005f, 0x004f,
 0x0048, 0x0039, 0x0059, 0x0045, 0x0031, 0x0042, 0x002e, 0x001b,
 0x004d, 0x0025, 0x0023, 0x0042, 0x003a, 0x0034, 0x005b, 0x004a,
 0x003e, 0x0030, 0x004f, 0x003f, 0x005a, 0x003e, 0x0028, 0x0026,
 0x007d, 0x0020, 0x003c, 0x0038, 0x0032, 0x005c, 0x004e, 0x0041,
 0x0037, 0x0057, 0x0047, 0x0033, 0x0049, 0x0033, 0x0046, 0x001e,
 0x006d, 0x0035, 0x0031, 0x005e, 0x0058, 0x004b, 0x0042, 0x007a,
 0x005b, 0x0049, 0x0038, 0x002a, 0x0040, 0x002c, 0x0015, 0x0019,
 0x005a, 0x002b, 0x0029, 0x004d, 0x0049, 0x003f, 0x0038, 0x005c,
 0x004d, 0x0042, 0x002f, 0x0043, 0x0030, 0x0035, 0x0024, 0x0014,
 0x0047, 0x0022, 0x0043, 0x003c, 0x003a, 0x0031, 0x0058, 0x004c,
 0x0043, 0x006a, 0x0047, 0x0036, 0x0026, 0x0027, 0x0017, 0x000f,
 0x006d, 0x0035, 0x0033, 0x002f, 0x005a, 0x0052, 0x003a, 0x0039,
 0x0030, 0x0048, 0x0039, 0x0029, 0x0017, 0x001b, 0x003e, 0x0009,
 0x0056, 0x002a, 0x0028, 0x0025, 0x0046, 0x0040, 0x0034, 0x002b,
 0x0046, 0x0037, 0x002a, 0x0019, 0x001d, 0x0012, 0x000b, 0x000b,
 0x0076, 0x0044, 0x001e, 0x0037, 0x0032, 0x002e, 0x004a, 0x0041,
 0x0031, 0x0027, 0x0018, 0x0010, 0x0016, 0x000d, 0x000e, 0x0007,
 0x005b, 0x002c, 0x0027, 0x0026, 0x0022, 0x003f, 0x0034, 0x002d,
 0x001f, 0x0034, 0x001c, 0x0013, 0x000e, 0x0008, 0x0009, 0x0003,
 0x007b, 0x003c, 0x003a, 0x0035, 0x002f, 0x002b, 0x0020, 0x0016,
 0x0025, 0x0018, 0x0011, 0x000c, 0x000f, 0x000a, 0x0002, 0x0001,
 0x0047, 0x0025, 0x0022, 0x001e, 0x001c, 0x0014, 0x0011, 0x001a,
 0x0015, 0x0010, 0x000a, 0x0006, 0x0008, 0x0006, 0x0002, 0x0000,
};

static const uint8 mp3_huffbits_15[256] = {
  3,  4,  5,  7,  7,  8,  9,  9,
  9, 10, 10, 11, 11, 11, 12, 13,
  4,  3,  5,  6,  7,  7,  8,  8,
  8,  9,  9, 10, 10, 10, 11, 11,
  5,  5,  5,  6,  7,  7,  8,  8,
  8,  9,  9, 10, 10, 11, 11, 11,
  6,  6,  6,  7,  7,  8,  8,  9,
  9,  9, 10, 10, 10, 11, 11, 11,
  7,  6,  7,  7,  8,  8,  9,  9,
  9,  9, 10, 10, 10, 11, 11, 11,
  8,  7,  7,  8,  8,  8,  9,  9,
  9,  9, 10, 10, 11, 11, 11, 12,
  9,  7,  8,  8,  8,  9,  9,  9,
  9, 10, 10, 10, 11, 11, 12, 12,
  9,  8,  8,  9,  9,  9,  9, 10,
 10, 10, 10, 10, 11, 11, 11, 12,
  9,  8,  8,  9,  9,  9,  9, 10,
 10, 10, 10, 11, 11, 12, 12, 12,
  9,  8,  9,  9,  9,  9, 10, 10,
 10, 11, 11, 11, 11, 12, 12, 12,
 10,  9,  9,  9, 10, 10, 10, 10,
 10, 11, 11, 11, 11, 12, 13, 12,
 10,  9,  9,  9, 10, 10, 10, 10,
 11, 11, 11, 11, 12, 12, 12, 13,
 11, 10,  9, 10, 10, 10, 11, 11,
 11, 11, 11, 11, 12, 12, 13, 13,
 11, 10, 10, 10, 10, 11, 11, 11,
 11, 12, 12, 12, 12, 12, 13, 13,
 12, 11, 11, 11, 11, 11, 11, 11,
 12, 12, 12, 12, 13, 13, 12, 13,
 12, 11, 11, 11, 11, 11, 11, 12,
 12, 12, 12, 12, 13, 13, 13, 13,
};

static const uint16 mp3_huffcodes_16[256] = {
 0x0001, 0x0005, 0x000e, 0x002c, 0x004a, 0x003f, 0x006e, 0x005d,
 0x00ac, 0x0095, 0x008a, 0x00f2, 0x00e1, 0x00c3, 0x0178, 0x0011,
 0x0003, 0x0004, 0x000c, 0x0014, 0x0023, 0x003e, 0x0035, 0x002f,
 0x0053, 0x004b, 0x0044, 0x0077, 0x00c9, 0x006b, 0x00cf, 0x0009,
 0x000f, 0x000d, 0x0017, 0x0026, 0x0043, 0x003a, 0x0067, 0x005a,
 0x00a1, 0x0048, 0x007f, 0x0075, 0x006e, 0x00d1, 0x00ce, 0x0010,
 0x002d, 0x0015, 0x0027, 0x0045, 0x0040, 0x0072, 0x0063, 0x0057,
 0x009e, 0x008c, 0x00fc, 0x00d4, 0x00c7, 0x0183, 0x016d, 0x001a,
 0x004b, 0x0024, 0x0044, 0x0041, 0x0073, 0x0065, 0x00b3, 0x00a4,
 0x009b, 0x0108, 0x00f6, 0x00e2, 0x018b, 0x017e, 0x016a, 0x0009,
 0x0042, 0x001e, 0x003b, 0x0038, 0x0066, 0x00b9, 0x00ad, 0x0109,
 0x008e, 0x00fd, 0x00e8, 0x0190, 0x0184, 0x017a, 0x01bd, 0x0010,
 0x006f, 0x0036, 0x0034, 0x0064, 0x00b8, 0x00b2, 0x00a0, 0x0085,
 0x0101, 0x00f4, 0x00e4, 0x00d9, 0x0181, 0x016e, 0x02cb, 0x000a,
 0x0062, 0x0030, 0x005b, 0x0058, 0x00a5, 0x009d, 0x0094, 0x0105,
 0x00f8, 0x0197, 0x018d, 0x0174, 0x017c, 0x0379, 0x0374, 0x0008,
 0x0055, 0x0054, 0x0051, 0x009f, 0x009c, 0x008f, 0x0104, 0x00f9,
 0x01ab, 0x0191, 0x0188, 0x017f, 0x02d7, 0x02c9, 0x02c4, 0x0007,
 0x009a, 0x004c, 0x0049, 0x008d, 0x0083, 0x0100, 0x00f5, 0x01aa,
 0x0196, 0x018a, 0x0180, 0x02df, 0x0167, 0x02c6, 0x0160, 0x000b,
 0x008b, 0x0081, 0x0043, 0x007d, 0x00f7, 0x00e9, 0x00e5, 0x00db,
 0x0189, 0x02e7, 0x02e1, 0x02d0, 0x0375, 0x0372, 0x01b7, 0x0004,
 0x00f3, 0x0078, 0x0076, 0x0073, 0x00e3, 0x00df, 0x018c, 0x02ea,
 0x02e6, 0x02e0, 0x02d1, 0x02c8, 0x02c2, 0x00df, 0x01b4, 0x0006,
 0x00ca, 0x00e0, 0x00de, 0x00da, 0x00d8, 0x0185, 0x0182, 0x017d,
 0x016c, 0x0378, 0x01bb, 0x02c3, 0x01b8, 0x01b5, 0x06c0, 0x0004,
 0x02eb, 0x00d3, 0x00d2, 0x00d0, 0x0172, 0x017b, 0x02de, 0x02d3,
 0x02ca, 0x06c7, 0x0373, 0x036d, 0x036c, 0x0d83, 0x0361, 0x0002,
 0x0179, 0x0171, 0x0066, 0x00bb, 0x02d6, 0x02d2, 0x0166, 0x02c7,
 0x02c5, 0x0362, 0x06c6, 0x0367, 0x0d82, 0x0366, 0x01b2, 0x0000,
 0x000c, 0x000a, 0x0007, 0x000b, 0x000a, 0x0011, 0x000b, 0x0009,
 0x000d, 0x000c, 0x000a, 0x0007, 0x0005, 0x0003, 0x0001, 0x0003,
};

static const uint8 mp3_huffbits_16[256] = {
  1,  4,  6,  8,  9,  9, 10, 10,
 11, 11, 11, 12, 12, 12, 13,  9,
  3,  4,  6,  7,  8,  9,  9,  9,
 10, 10, 10, 11, 12, 11, 12,  8,
  6,  6,  7,  8,  9,  9, 10, 10,
 11, 10, 11, 11, 11, 12, 12,  9,
  8,  7,  8,  9,  9, 10, 10, 10,
 11, 11, 12, 12, 12, 13, 13, 10,
  9,  8,  9,  9, 10, 10, 11, 11,
 11, 12, 12, 12, 13, 13, 13,  9,
  9,  8,  9,  9, 10, 11, 11, 12,
 11, 12, 12, 13, 13, 13, 14, 10,
 10,  9,  9, 10, 11, 11, 11, 11,
 12, 12, 12, 12, 13, 13, 14, 10,
 10,  9, 10, 10, 11, 11, 11, 12,
 12, 13, 13, 13, 13, 15, 15, 10,
 10, 10, 10, 11, 11, 11, 12, 12,
 13, 13, 13, 13, 14, 14, 14, 10,
 11, 10, 10, 11, 11, 12, 12, 13,
 13, 13, 13, 14, 13, 14, 13, 11,
 11, 11, 10, 11, 12, 12, 12, 12,
 13, 14, 14, 14, 15, 15, 14, 10,
 12, 11, 11, 11, 12, 12, 13, 14,
 14, 14, 14, 14, 14, 13, 14, 11,
 12, 12, 12, 12, 12, 13, 13, 13,
 13, 15, 14, 14, 14, 14, 16, 11,
 14, 12, 12, 12, 13, 13, 14, 14,
 14, 16, 15, 15, 15, 17, 15, 11,
 13, 13, 11, 12, 14, 14, 13, 14,
 14, 15, 16, 15, 17, 15, 14, 11,
  9,  8,  8,  9,  9, 10, 10, 10,
 11, 11, 11, 11, 11, 11, 11,  8,
};

static const uint16 mp3_huffcodes_24[256] = {
 0x000f, 0x000d, 0x002e, 0x0050, 0x0092, 0x0106, 0x00f8, 0x01b2,
 0x01aa, 0x029d, 0x028d, 0x0289, 0x026d, 0x0205, 0x0408, 0x0058,
 0x000e, 0x000c, 0x0015, 0x0026, 0x0047, 0x0082, 0x007a, 0x00d8,
 0x00d1, 0x00c6, 0x0147, 0x0159, 0x013f, 0x0129, 0x0117, 0x002a,
 0x002f, 0x0016, 0x0029, 0x004a, 0x0044, 0x0080, 0x0078, 0x00dd,
 0x00cf, 0x00c2, 0x00b6, 0x0154, 0x013b, 0x0127, 0x021d, 0x0012,
 0x0051, 0x0027, 0x004b, 0x0046, 0x0086, 0x007d, 0x0074, 0x00dc,
 0x00cc, 0x00be, 0x00b2, 0x0145, 0x0137, 0x0125, 0x010f, 0x0010,
 0x0093, 0x0048, 0x0045, 0x0087, 0x007f, 0x0076, 0x0070, 0x00d2,
 0x00c8, 0x00bc, 0x0160, 0x0143, 0x0132, 0x011d, 0x021c, 0x000e,
 0x0107, 0x0042, 0x0081, 0x007e, 0x0077, 0x0072, 0x00d6, 0x00ca,
 0x00c0, 0x00b4, 0x0155, 0x013d, 0x012d, 0x0119, 0x0106, 0x000c,
 0x00f9, 0x007b, 0x0079, 0x0075, 0x0071, 0x00d7, 0x00ce, 0x00c3,
 0x00b9, 0x015b, 0x014a, 0x0134, 0x0123, 0x0110, 0x0208, 0x000a,
 0x01b3, 0x0073, 0x006f, 0x006d, 0x00d3, 0x00cb, 0x00c4, 0x00bb,
 0x0161, 0x014c, 0x0139, 0x012a, 0x011b, 0x0213, 0x017d, 0x0011,
 0x01ab, 0x00d4, 0x00d0, 0x00cd, 0x00c9, 0x00c1, 0x00ba, 0x00b1,
 0x00a9, 0x0140, 0x012f, 0x011e, 0x010c, 0x0202, 0x0179, 0x0010,
 0x014f, 0x00c7, 0x00c5, 0x00bf, 0x00bd, 0x00b5, 0x00ae, 0x014d,
 0x0141, 0x0131, 0x0121, 0x0113, 0x0209, 0x017b, 0x0173, 0x000b,
 0x029c, 0x00b8, 0x00b7, 0x00b3, 0x00af, 0x0158, 0x014b, 0x013a,
 0x0130, 0x0122, 0x0115, 0x0212, 0x017f, 0x0175, 0x016e, 0x000a,
 0x028c, 0x015a, 0x00ab, 0x00a8, 0x00a4, 0x013e, 0x0135, 0x012b,
 0x011f, 0x0114, 0x0107, 0x0201, 0x0177, 0x0170, 0x016a, 0x0006,
 0x0288, 0x0142, 0x013c, 0x0138, 0x0133, 0x012e, 0x0124, 0x011c,
 0x010d, 0x0105, 0x0200, 0x0178, 0x0172, 0x016c, 0x0167, 0x0004,
 0x026c, 0x012c, 0x0128, 0x0126, 0x0120, 0x011a, 0x0111, 0x010a,
 0x0203, 0x017c, 0x0176, 0x0171, 0x016d, 0x0169, 0x0165, 0x0002,
 0x0409, 0x0118, 0x0116, 0x0112, 0x010b, 0x0108, 0x0103, 0x017e,
 0x017a, 0x0174, 0x016f, 0x016b, 0x0168, 0x0166, 0x0164, 0x0000,
 0x002b, 0x0014, 0x0013, 0x0011, 0x000f, 0x000d, 0x000b, 0x0009,
 0x0007, 0x0006, 0x0004, 0x0007, 0x0005, 0x0003, 0x0001, 0x0003,
};

static const uint8 mp3_huffbits_24[256] = {
  4,  4,  6,  7,  8,  9,  9, 10,
 10, 11, 11, 11, 11, 11, 12,  9,
  4,  4,  5,  6,  7,  8,  8,  9,
  9,  9, 10, 10, 10, 10, 10,  8,
  6,  5,  6,  7,  7,  8,  8,  9,
  9,  9,  9, 10, 10, 10, 11,  7,
  7,  6,  7,  7,  8,  8,  8,  9,
  9,  9,  9, 10, 10, 10, 10,  7,
  8,  7,  7,  8,  8,  8,  8,  9,
  9,  9, 10, 10, 10, 10, 11,  7,
  9,  7,  8,  8,  8,  8,  9,  9,
  9,  9, 10, 10, 10, 10, 10,  7,
  9,  8,  8,  8,  8,  9,  9,  9,
  9, 10, 10, 10, 10, 10, 11,  7,
 10,  8,  8,  8,  9,  9,  9,  9,
 10, 10, 10, 10, 10, 11, 11,  8,
 10,  9,  9,  9,  9,  9,  9,  9,
  9, 10, 10, 10, 10, 11, 11,  8,
 10,  9,  9,  9,  9,  9,  9, 10,
 10, 10, 10, 10, 11, 11, 11,  8,
 11,  9,  9,  9,  9, 10, 10, 10,
 10, 10, 10, 11, 11, 11, 11,  8,
 11, 10,  9,  9,  9, 10, 10, 10,
 10, 10, 10, 11, 11, 11, 11,  8,
 11, 10, 10, 10, 10, 10, 10, 10,
 10, 10, 11, 11, 11, 11, 11,  8,
 11, 10, 10, 10, 10, 10, 10, 10,
 11, 11, 11, 11, 11, 11, 11,  8,
 12, 10, 10, 10, 10, 10, 10, 11,
 11, 11, 11, 11, 11, 11, 11,  8,
  8,  7,  7,  7,  7,  7,  7,  7,
  7,  7,  7,  8,  8,  8,  8,  4,
};

static const huff_table_t mp3_huff_tables[16] = {
{ 1, NULL, NULL },
{ 2, mp3_huffbits_1, mp3_huffcodes_1 },
{ 3, mp3_huffbits_2, mp3_huffcodes_2 },
{ 3, mp3_huffbits_3, mp3_huffcodes_3 },
{ 4, mp3_huffbits_5, mp3_huffcodes_5 },
{ 4, mp3_huffbits_6, mp3_huffcodes_6 },
{ 6, mp3_huffbits_7, mp3_huffcodes_7 },
{ 6, mp3_huffbits_8, mp3_huffcodes_8 },
{ 6, mp3_huffbits_9, mp3_huffcodes_9 },
{ 8, mp3_huffbits_10, mp3_huffcodes_10 },
{ 8, mp3_huffbits_11, mp3_huffcodes_11 },
{ 8, mp3_huffbits_12, mp3_huffcodes_12 },
{ 16, mp3_huffbits_13, mp3_huffcodes_13 },
{ 16, mp3_huffbits_15, mp3_huffcodes_15 },
{ 16, mp3_huffbits_16, mp3_huffcodes_16 },
{ 16, mp3_huffbits_24, mp3_huffcodes_24 },
};

static const uint8 mp3_huff_data[32][2] = {
{ 0, 0 },
{ 1, 0 },
{ 2, 0 },
{ 3, 0 },
{ 0, 0 },
{ 4, 0 },
{ 5, 0 },
{ 6, 0 },
{ 7, 0 },
{ 8, 0 },
{ 9, 0 },
{ 10, 0 },
{ 11, 0 },
{ 12, 0 },
{ 0, 0 },
{ 13, 0 },
{ 14, 1 },
{ 14, 2 },
{ 14, 3 },
{ 14, 4 },
{ 14, 6 },
{ 14, 8 },
{ 14, 10 },
{ 14, 13 },
{ 15, 4 },
{ 15, 5 },
{ 15, 6 },
{ 15, 7 },
{ 15, 8 },
{ 15, 9 },
{ 15, 11 },
{ 15, 13 },
};

static const uint8 mp3_quad_codes[2][16] = {
    {  1,  5,  4,  5,  6,  5,  4,  4, 7,  3,  6,  0,  7,  2,  3,  1, },
    { 15, 14, 13, 12, 11, 10,  9,  8, 7,  6,  5,  4,  3,  2,  1,  0, },
};

static const uint8 mp3_quad_bits[2][16] = {
    { 1, 4, 4, 5, 4, 6, 5, 6, 4, 5, 5, 6, 5, 6, 6, 6, },
    { 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, },
};

static const uint8 band_size_long[9][22] = {
{ 4, 4, 4, 4, 4, 4, 6, 6, 8, 8, 10,
  12, 16, 20, 24, 28, 34, 42, 50, 54, 76, 158, }, /* 44100 */
{ 4, 4, 4, 4, 4, 4, 6, 6, 6, 8, 10,
  12, 16, 18, 22, 28, 34, 40, 46, 54, 54, 192, }, /* 48000 */
{ 4, 4, 4, 4, 4, 4, 6, 6, 8, 10, 12,
  16, 20, 24, 30, 38, 46, 56, 68, 84, 102, 26, }, /* 32000 */
{ 6, 6, 6, 6, 6, 6, 8, 10, 12, 14, 16,
  20, 24, 28, 32, 38, 46, 52, 60, 68, 58, 54, }, /* 22050 */
{ 6, 6, 6, 6, 6, 6, 8, 10, 12, 14, 16,
  18, 22, 26, 32, 38, 46, 52, 64, 70, 76, 36, }, /* 24000 */
{ 6, 6, 6, 6, 6, 6, 8, 10, 12, 14, 16,
  20, 24, 28, 32, 38, 46, 52, 60, 68, 58, 54, }, /* 16000 */
{ 6, 6, 6, 6, 6, 6, 8, 10, 12, 14, 16,
  20, 24, 28, 32, 38, 46, 52, 60, 68, 58, 54, }, /* 11025 */
{ 6, 6, 6, 6, 6, 6, 8, 10, 12, 14, 16,
  20, 24, 28, 32, 38, 46, 52, 60, 68, 58, 54, }, /* 12000 */
{ 12, 12, 12, 12, 12, 12, 16, 20, 24, 28, 32,
  40, 48, 56, 64, 76, 90, 2, 2, 2, 2, 2, }, /* 8000 */
};

static const uint8 band_size_short[9][13] = {
{ 4, 4, 4, 4, 6, 8, 10, 12, 14, 18, 22, 30, 56, }, /* 44100 */
{ 4, 4, 4, 4, 6, 6, 10, 12, 14, 16, 20, 26, 66, }, /* 48000 */
{ 4, 4, 4, 4, 6, 8, 12, 16, 20, 26, 34, 42, 12, }, /* 32000 */
{ 4, 4, 4, 6, 6, 8, 10, 14, 18, 26, 32, 42, 18, }, /* 22050 */
{ 4, 4, 4, 6, 8, 10, 12, 14, 18, 24, 32, 44, 12, }, /* 24000 */
{ 4, 4, 4, 6, 8, 10, 12, 14, 18, 24, 30, 40, 18, }, /* 16000 */
{ 4, 4, 4, 6, 8, 10, 12, 14, 18, 24, 30, 40, 18, }, /* 11025 */
{ 4, 4, 4, 6, 8, 10, 12, 14, 18, 24, 30, 40, 18, }, /* 12000 */
{ 8, 8, 8, 12, 16, 20, 24, 28, 36, 2, 2, 2, 26, }, /* 8000 */
};

static const uint8 mp3_pretab[2][22] = {
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 3, 3, 3, 2, 0 },
};

static const float ci_table[8] = {
    -0.6f, -0.535f, -0.33f, -0.185f, -0.095f, -0.041f, -0.0142f, -0.0037f,
};

#define C1 FIXHR(0.98480775301220805936/2)
#define C2 FIXHR(0.93969262078590838405/2)
#define C3 FIXHR(0.86602540378443864676/2)
#define C4 FIXHR(0.76604444311897803520/2)
#define C5 FIXHR(0.64278760968653932632/2)
#define C6 FIXHR(0.5/2)
#define C7 FIXHR(0.34202014332566873304/2)
#define C8 FIXHR(0.17364817766693034885/2)

static const int icos36[9] = {
    FIXR(0.50190991877167369479),
    FIXR(0.51763809020504152469), //0
    FIXR(0.55168895948124587824),
    FIXR(0.61038729438072803416),
    FIXR(0.70710678118654752439), //1
    FIXR(0.87172339781054900991),
    FIXR(1.18310079157624925896),
    FIXR(1.93185165257813657349), //2
    FIXR(5.73685662283492756461),
};

static const int icos36h[9] = {
    FIXHR(0.50190991877167369479/2),
    FIXHR(0.51763809020504152469/2), //0
    FIXHR(0.55168895948124587824/2),
    FIXHR(0.61038729438072803416/2),
    FIXHR(0.70710678118654752439/2), //1
    FIXHR(0.87172339781054900991/2),
    FIXHR(1.18310079157624925896/4),
    FIXHR(1.93185165257813657349/4), //2
//    FIXHR(5.73685662283492756461),
};

////////////////////////////////////////////////////////////////////////////////

static INLINE int unaligned32_be(const uint8 *p)
{
        return (((p[0]<<8) | p[1])<<16) | (p[2]<<8) | (p[3]);
}

#define MIN_CACHE_BITS 25

#define NEG_SSR32(a,s) ((( int32)(a))>>(32-(s)))
#define NEG_USR32(a,s) (((uint32)(a))>>(32-(s)))

#define OPEN_READER(name, gb) \
        int name##_index= (gb)->index;\
        int name##_cache= 0;\

#define CLOSE_READER(name, gb)\
        (gb)->index= name##_index;\

#define UPDATE_CACHE(name, gb)\
        name##_cache= unaligned32_be(&((gb)->buffer[name##_index>>3])) << (name##_index&0x07); \

#define SKIP_CACHE(name, gb, num)\
        name##_cache <<= (num);

#define SKIP_COUNTER(name, gb, num)\
        name##_index += (num);\

#define SKIP_BITS(name, gb, num)\
        {\
            SKIP_CACHE(name, gb, num)\
            SKIP_COUNTER(name, gb, num)\
        }\

#define LAST_SKIP_BITS(name, gb, num) SKIP_COUNTER(name, gb, num)
#define LAST_SKIP_CACHE(name, gb, num) ;

#define SHOW_UBITS(name, gb, num)\
        NEG_USR32(name##_cache, num)

#define SHOW_SBITS(name, gb, num)\
        NEG_SSR32(name##_cache, num)

#define GET_CACHE(name, gb)\
        ((uint32)name##_cache)

static INLINE int get_bits_count(bitstream_t *s){
    return s->index;
}

static INLINE void skip_bits_long(bitstream_t *s, int n){
    s->index += n;
}
#define skip_bits skip_bits_long

static void init_get_bits(bitstream_t *s, const uint8 *buffer, int bit_size) {
    int buffer_size= (bit_size+7)>>3;
    if(buffer_size < 0 || bit_size < 0) {
        buffer_size = bit_size = 0;
        buffer = NULL;
    }
    s->buffer= buffer;
    s->size_in_bits= bit_size;
    s->buffer_end= buffer + buffer_size;
    s->index=0;
}

static INLINE unsigned int get_bits(bitstream_t *s, int n){
    int tmp;
    OPEN_READER(re, s)
    UPDATE_CACHE(re, s)
    tmp= SHOW_UBITS(re, s, n);
    LAST_SKIP_BITS(re, s, n)
    CLOSE_READER(re, s)
    return tmp;
}

static INLINE int get_bitsz(bitstream_t *s, int n)
{
    if (n == 0)
        return 0;
    else
        return get_bits(s, n);
}

static INLINE unsigned int get_bits1(bitstream_t *s){
    int index= s->index;
    uint8 result= s->buffer[ index>>3 ];
    result<<= (index&0x07);
    result>>= 8 - 1;
    index++;
    s->index= index;
    return result;
}

static INLINE void align_get_bits(bitstream_t *s)
{
    int n= (-get_bits_count(s)) & 7;
    if(n) skip_bits(s, n);
}

#define GET_DATA(v, table, i, wrap, size) \
{\
    const uint8 *ptr = (const uint8 *)table + i * wrap;\
    switch(size) {\
    case 1:\
        v = *(const uint8 *)ptr;\
        break;\
    case 2:\
        v = *(const uint16 *)ptr;\
        break;\
    default:\
        v = *(const uint32 *)ptr;\
        break;\
    }\
}

static INLINE int alloc_table(vlc_t *vlc, int size) {
    int index;
    index = vlc->table_size;
    vlc->table_size += size;
    if (vlc->table_size > vlc->table_allocated) {
        vlc->table_allocated += (1 << vlc->bits);
        vlc->table = (VLC_TYPE(*)[2])libc_realloc(vlc->table, sizeof(VLC_TYPE) * 2 * vlc->table_allocated);
        if (!vlc->table)
            return -1;
    }
    return index;
}

static int build_table(
    vlc_t *vlc, int table_nb_bits,
    int nb_codes,
    const void *bits, int bits_wrap, int bits_size,
    const void *codes, int codes_wrap, int codes_size,
    uint32 code_prefix, int n_prefix
) {
    int i, j, k, n, table_size, table_index, nb, n1, index, code_prefix2;
    uint32 code;
    VLC_TYPE (*table)[2];

    table_size = 1 << table_nb_bits;
    table_index = alloc_table(vlc, table_size);
    if (table_index < 0)
        return -1;
    table = &vlc->table[table_index];

    for(i=0;i<table_size;i++) {
        table[i][1] = 0; //bits
        table[i][0] = -1; //codes
    }

    for(i=0;i<nb_codes;i++) {
        GET_DATA(n, bits, i, bits_wrap, bits_size);
        GET_DATA(code, codes, i, codes_wrap, codes_size);
        if (n <= 0)
            continue;
        n -= n_prefix;
        code_prefix2= code >> n;
        if (n > 0 && code_prefix2 == code_prefix) {
            if (n <= table_nb_bits) {
                j = (code << (table_nb_bits - n)) & (table_size - 1);
                nb = 1 << (table_nb_bits - n);
                for(k=0;k<nb;k++) {
                    if (table[j][1] /*bits*/ != 0) {
                        return -1;
                    }
                    table[j][1] = n; //bits
                    table[j][0] = i; //code
                    j++;
                }
            } else {
                n -= table_nb_bits;
                j = (code >> n) & ((1 << table_nb_bits) - 1);
                n1 = -table[j][1]; //bits
                if (n > n1)
                    n1 = n;
                table[j][1] = -n1; //bits
            }
        }
    }
    for(i=0;i<table_size;i++) {
        n = table[i][1]; //bits
        if (n < 0) {
            n = -n;
            if (n > table_nb_bits) {
                n = table_nb_bits;
                table[i][1] = -n; //bits
            }
            index = build_table(vlc, n, nb_codes,
                                bits, bits_wrap, bits_size,
                                codes, codes_wrap, codes_size,
                                (code_prefix << table_nb_bits) | i,
                                n_prefix + table_nb_bits);
            if (index < 0)
                return -1;
            table = &vlc->table[table_index];
            table[i][0] = index; //code
        }
    }
    return table_index;
}

static INLINE int init_vlc(
    vlc_t *vlc, int nb_bits, int nb_codes,
    const void *bits, int bits_wrap, int bits_size,
    const void *codes, int codes_wrap, int codes_size
) {
    vlc->bits = nb_bits;
    if (build_table(vlc, nb_bits, nb_codes,
                    bits, bits_wrap, bits_size,
                    codes, codes_wrap, codes_size,
                    0, 0) < 0) {
        libc_free(vlc->table);
        return -1;
    }
    return 0;
}

#define GET_VLC(code, name, gb, table, bits, max_depth)\
{\
    int n, index, nb_bits;\
\
    index= SHOW_UBITS(name, gb, bits);\
    code = table[index][0];\
    n    = table[index][1];\
\
    if(max_depth > 1 && n < 0){\
        LAST_SKIP_BITS(name, gb, bits)\
        UPDATE_CACHE(name, gb)\
\
        nb_bits = -n;\
\
        index= SHOW_UBITS(name, gb, nb_bits) + code;\
        code = table[index][0];\
        n    = table[index][1];\
        if(max_depth > 2 && n < 0){\
            LAST_SKIP_BITS(name, gb, nb_bits)\
            UPDATE_CACHE(name, gb)\
\
            nb_bits = -n;\
\
            index= SHOW_UBITS(name, gb, nb_bits) + code;\
            code = table[index][0];\
            n    = table[index][1];\
        }\
    }\
    SKIP_BITS(name, gb, n)\
}

static INLINE int get_vlc2(bitstream_t *s, VLC_TYPE (*table)[2], int bits, int max_depth) {
    int code;

    OPEN_READER(re, s)
    UPDATE_CACHE(re, s)

    GET_VLC(code, re, s, table, bits, max_depth)

    CLOSE_READER(re, s)
    return code;
}

static void switch_buffer(mp3_context_t *s, int *pos, int *end_pos, int *end_pos2) {
    if(s->in_gb.buffer && *pos >= s->gb.size_in_bits){
        s->gb= s->in_gb;
        s->in_gb.buffer=NULL;
        skip_bits_long(&s->gb, *pos - *end_pos);
        *end_pos2=
        *end_pos= *end_pos2 + get_bits_count(&s->gb) - *pos;
        *pos= get_bits_count(&s->gb);
    }
}

////////////////////////////////////////////////////////////////////////////////

static INLINE int mp3_check_header(uint32 header){
    /* header */
    if ((header & 0xffe00000) != 0xffe00000)
        return -1;
    /* layer check */
    if ((header & (3<<17)) != (1 << 17))
        return -1;
    /* bit rate */
    if ((header & (0xf<<12)) == 0xf<<12)
        return -1;
    /* frequency */
    if ((header & (3<<10)) == 3<<10)
        return -1;
    return 0;
}


static void lsf_sf_expand(
    int *slen, int sf, int n1, int n2, int n3
) {
    if (n3) {
        slen[3] = sf % n3;
        sf /= n3;
    } else {
        slen[3] = 0;
    }
    if (n2) {
        slen[2] = sf % n2;
        sf /= n2;
    } else {
        slen[2] = 0;
    }
    slen[1] = sf % n1;
    sf /= n1;
    slen[0] = sf;
}

static INLINE int l3_unscale(int value, int exponent)
{
    unsigned int m;
    int e;

    e = table_4_3_exp  [4*value + (exponent&3)];
    m = table_4_3_value[4*value + (exponent&3)];
    e -= (exponent >> 2);
    if (e > 31)
        return 0;
    m = (m + (1 << (e-1))) >> e;

    return m;
}

static INLINE int round_sample(int *sum) {
    int sum1;
    sum1 = (*sum) >> OUT_SHIFT;
    *sum &= (1<<OUT_SHIFT)-1;
    if (sum1 < OUT_MIN)
        sum1 = OUT_MIN;
    else if (sum1 > OUT_MAX)
        sum1 = OUT_MAX;
    return sum1;
}

static void exponents_from_scale_factors(
    mp3_context_t *s, granule_t *g, int16 *exponents
) {
    const uint8 *bstab, *pretab;
    int len, i, j, k, l, v0, shift, gain, gains[3];
    int16 *exp_ptr;

    exp_ptr = exponents;
    gain = g->global_gain - 210;
    shift = g->scalefac_scale + 1;

    bstab = band_size_long[s->sample_rate_index];
    pretab = mp3_pretab[g->preflag];
    for(i=0;i<g->long_end;i++) {
        v0 = gain - ((g->scale_factors[i] + pretab[i]) << shift) + 400;
        len = bstab[i];
        for(j=len;j>0;j--)
            *exp_ptr++ = v0;
    }

    if (g->short_start < 13) {
        bstab = band_size_short[s->sample_rate_index];
        gains[0] = gain - (g->subblock_gain[0] << 3);
        gains[1] = gain - (g->subblock_gain[1] << 3);
        gains[2] = gain - (g->subblock_gain[2] << 3);
        k = g->long_end;
        for(i=g->short_start;i<13;i++) {
            len = bstab[i];
            for(l=0;l<3;l++) {
                v0 = gains[l] - (g->scale_factors[k++] << shift) + 400;
                for(j=len;j>0;j--)
                *exp_ptr++ = v0;
            }
        }
    }
}

static void reorder_block(mp3_context_t *s, granule_t *g)
{
    int i, j, len;
    int32 *ptr, *dst, *ptr1;
    int32 tmp[576];

    if (g->block_type != 2)
        return;

    if (g->switch_point) {
        if (s->sample_rate_index != 8) {
            ptr = g->sb_hybrid + 36;
        } else {
            ptr = g->sb_hybrid + 48;
        }
    } else {
        ptr = g->sb_hybrid;
    }

    for(i=g->short_start;i<13;i++) {
        len = band_size_short[s->sample_rate_index][i];
        ptr1 = ptr;
        dst = tmp;
        for(j=len;j>0;j--) {
            *dst++ = ptr[0*len];
            *dst++ = ptr[1*len];
            *dst++ = ptr[2*len];
            ptr++;
        }
        ptr+=2*len;
        libc_memcpy(ptr1, tmp, len * 3 * sizeof(*ptr1));
    }
}

static void compute_antialias(mp3_context_t *s, granule_t *g) {
    int32 *ptr, *csa;
    int n, i;

    /* we antialias only "long" bands */
    if (g->block_type == 2) {
        if (!g->switch_point)
            return;
        /* XXX: check this for 8000Hz case */
        n = 1;
    } else {
        n = SBLIMIT - 1;
    }

    ptr = g->sb_hybrid + 18;
    for(i = n;i > 0;i--) {
        int tmp0, tmp1, tmp2;
        csa = &csa_table[0][0];
#define INT_AA(j) \
            tmp0 = ptr[-1-j];\
            tmp1 = ptr[   j];\
            tmp2= MULH(tmp0 + tmp1, csa[0+4*j]);\
            ptr[-1-j] = 4*(tmp2 - MULH(tmp1, csa[2+4*j]));\
            ptr[   j] = 4*(tmp2 + MULH(tmp0, csa[3+4*j]));

        INT_AA(0)
        INT_AA(1)
        INT_AA(2)
        INT_AA(3)
        INT_AA(4)
        INT_AA(5)
        INT_AA(6)
        INT_AA(7)

        ptr += 18;
    }
}

static void compute_stereo(
    mp3_context_t *s, granule_t *g0, granule_t *g1
) {
    int i, j, k, l;
    int32 v1, v2;
    int sf_max, tmp0, tmp1, sf, len, non_zero_found;
    int32 (*is_tab)[16];
    int32 *tab0, *tab1;
    int non_zero_found_short[3];

    if (s->mode_ext & MODE_EXT_I_STEREO) {
        if (!s->lsf) {
            is_tab = is_table;
            sf_max = 7;
        } else {
            is_tab = is_table_lsf[g1->scalefac_compress & 1];
            sf_max = 16;
        }

        tab0 = g0->sb_hybrid + 576;
        tab1 = g1->sb_hybrid + 576;

        non_zero_found_short[0] = 0;
        non_zero_found_short[1] = 0;
        non_zero_found_short[2] = 0;
        k = (13 - g1->short_start) * 3 + g1->long_end - 3;
        for(i = 12;i >= g1->short_start;i--) {
            /* for last band, use previous scale factor */
            if (i != 11)
                k -= 3;
            len = band_size_short[s->sample_rate_index][i];
            for(l=2;l>=0;l--) {
                tab0 -= len;
                tab1 -= len;
                if (!non_zero_found_short[l]) {
                    /* test if non zero band. if so, stop doing i-stereo */
                    for(j=0;j<len;j++) {
                        if (tab1[j] != 0) {
                            non_zero_found_short[l] = 1;
                            goto found1;
                        }
                    }
                    sf = g1->scale_factors[k + l];
                    if (sf >= sf_max)
                        goto found1;

                    v1 = is_tab[0][sf];
                    v2 = is_tab[1][sf];
                    for(j=0;j<len;j++) {
                        tmp0 = tab0[j];
                        tab0[j] = MULL(tmp0, v1);
                        tab1[j] = MULL(tmp0, v2);
                    }
                } else {
                found1:
                    if (s->mode_ext & MODE_EXT_MS_STEREO) {
                        /* lower part of the spectrum : do ms stereo
                           if enabled */
                        for(j=0;j<len;j++) {
                            tmp0 = tab0[j];
                            tmp1 = tab1[j];
                            tab0[j] = MULL(tmp0 + tmp1, ISQRT2);
                            tab1[j] = MULL(tmp0 - tmp1, ISQRT2);
                        }
                    }
                }
            }
        }

        non_zero_found = non_zero_found_short[0] |
            non_zero_found_short[1] |
            non_zero_found_short[2];

        for(i = g1->long_end - 1;i >= 0;i--) {
            len = band_size_long[s->sample_rate_index][i];
            tab0 -= len;
            tab1 -= len;
            /* test if non zero band. if so, stop doing i-stereo */
            if (!non_zero_found) {
                for(j=0;j<len;j++) {
                    if (tab1[j] != 0) {
                        non_zero_found = 1;
                        goto found2;
                    }
                }
                /* for last band, use previous scale factor */
                k = (i == 21) ? 20 : i;
                sf = g1->scale_factors[k];
                if (sf >= sf_max)
                    goto found2;
                v1 = is_tab[0][sf];
                v2 = is_tab[1][sf];
                for(j=0;j<len;j++) {
                    tmp0 = tab0[j];
                    tab0[j] = MULL(tmp0, v1);
                    tab1[j] = MULL(tmp0, v2);
                }
            } else {
            found2:
                if (s->mode_ext & MODE_EXT_MS_STEREO) {
                    /* lower part of the spectrum : do ms stereo
                       if enabled */
                    for(j=0;j<len;j++) {
                        tmp0 = tab0[j];
                        tmp1 = tab1[j];
                        tab0[j] = MULL(tmp0 + tmp1, ISQRT2);
                        tab1[j] = MULL(tmp0 - tmp1, ISQRT2);
                    }
                }
            }
        }
    } else if (s->mode_ext & MODE_EXT_MS_STEREO) {
        /* ms stereo ONLY */
        /* NOTE: the 1/sqrt(2) normalization factor is included in the
           global gain */
        tab0 = g0->sb_hybrid;
        tab1 = g1->sb_hybrid;
        for(i=0;i<576;i++) {
            tmp0 = tab0[i];
            tmp1 = tab1[i];
            tab0[i] = tmp0 + tmp1;
            tab1[i] = tmp0 - tmp1;
        }
    }
}

static int huffman_decode(
    mp3_context_t *s, granule_t *g, int16 *exponents, int end_pos2
) {
    int s_index;
    int i;
    int last_pos, bits_left;
    vlc_t *vlc;
    int end_pos= s->gb.size_in_bits;
    if (end_pos2 < end_pos) end_pos = end_pos2;

    /* low frequencies (called big values) */
    s_index = 0;
    for(i=0;i<3;i++) {
        int j, k, l, linbits;
        j = g->region_size[i];
        if (j == 0)
            continue;
        /* select vlc table */
        k = g->table_select[i];
        l = mp3_huff_data[k][0];
        linbits = mp3_huff_data[k][1];
        vlc = &huff_vlc[l];

        if(!l){
            libc_memset(&g->sb_hybrid[s_index], 0, sizeof(*g->sb_hybrid)*2*j);
            s_index += 2*j;
            continue;
        }

        /* read huffcode and compute each couple */
        for(;j>0;j--) {
            int exponent, x, y, v;
            int pos= get_bits_count(&s->gb);

            if (pos >= end_pos){
                switch_buffer(s, &pos, &end_pos, &end_pos2);
                if(pos >= end_pos)
                    break;
            }
            y = get_vlc2(&s->gb, vlc->table, 7, 3);

            if(!y){
                g->sb_hybrid[s_index  ] =
                g->sb_hybrid[s_index+1] = 0;
                s_index += 2;
                continue;
            }

            exponent= exponents[s_index];

            if(y&16){
                x = y >> 5;
                y = y & 0x0f;
                if (x < 15){
                    v = expval_table[ exponent ][ x ];
                }else{
                    x += get_bitsz(&s->gb, linbits);
                    v = l3_unscale(x, exponent);
                }
                if (get_bits1(&s->gb))
                    v = -v;
                g->sb_hybrid[s_index] = v;
                if (y < 15){
                    v = expval_table[ exponent ][ y ];
                }else{
                    y += get_bitsz(&s->gb, linbits);
                    v = l3_unscale(y, exponent);
                }
                if (get_bits1(&s->gb))
                    v = -v;
                g->sb_hybrid[s_index+1] = v;
            }else{
                x = y >> 5;
                y = y & 0x0f;
                x += y;
                if (x < 15){
                    v = expval_table[ exponent ][ x ];
                }else{
                    x += get_bitsz(&s->gb, linbits);
                    v = l3_unscale(x, exponent);
                }
                if (get_bits1(&s->gb))
                    v = -v;
                g->sb_hybrid[s_index+!!y] = v;
                g->sb_hybrid[s_index+ !y] = 0;
            }
            s_index+=2;
        }
    }

    /* high frequencies */
    vlc = &huff_quad_vlc[g->count1table_select];
    last_pos=0;
    while (s_index <= 572) {
        int pos, code;
        pos = get_bits_count(&s->gb);
        if (pos >= end_pos) {
            if (pos > end_pos2 && last_pos){
                /* some encoders generate an incorrect size for this
                   part. We must go back into the data */
                s_index -= 4;
                skip_bits_long(&s->gb, last_pos - pos);
                break;
            }
            switch_buffer(s, &pos, &end_pos, &end_pos2);
            if(pos >= end_pos)
                break;
        }
        last_pos= pos;

        code = get_vlc2(&s->gb, vlc->table, vlc->bits, 1);
        g->sb_hybrid[s_index+0]=
        g->sb_hybrid[s_index+1]=
        g->sb_hybrid[s_index+2]=
        g->sb_hybrid[s_index+3]= 0;
        while(code){
            const static int idxtab[16]={3,3,2,2,1,1,1,1,0,0,0,0,0,0,0,0};
            int v;
            int pos= s_index+idxtab[code];
            code ^= 8>>idxtab[code];
            v = exp_table[ exponents[pos] ];
            if(get_bits1(&s->gb))
                v = -v;
            g->sb_hybrid[pos] = v;
        }
        s_index+=4;
    }
    libc_memset(&g->sb_hybrid[s_index], 0, sizeof(*g->sb_hybrid)*(576 - s_index));

    /* skip extension bits */
    bits_left = end_pos2 - get_bits_count(&s->gb);
    if (bits_left < 0) {
        return -1;
    }
    skip_bits_long(&s->gb, bits_left);

    i= get_bits_count(&s->gb);
    switch_buffer(s, &i, &end_pos, &end_pos2);

    return 0;
}

////////////////////////////////////////////////////////////////////////////////

static void imdct12(int *out, int *in)
{
    int in0, in1, in2, in3, in4, in5, t1, t2;

    in0= in[0*3];
    in1= in[1*3] + in[0*3];
    in2= in[2*3] + in[1*3];
    in3= in[3*3] + in[2*3];
    in4= in[4*3] + in[3*3];
    in5= in[5*3] + in[4*3];
    in5 += in3;
    in3 += in1;

    in2= MULH(2*in2, C3);
    in3= MULH(4*in3, C3);

    t1 = in0 - in4;
    t2 = MULH(2*(in1 - in5), icos36h[4]);

    out[ 7]=
    out[10]= t1 + t2;
    out[ 1]=
    out[ 4]= t1 - t2;

    in0 += in4>>1;
    in4 = in0 + in2;
    in5 += 2*in1;
    in1 = MULH(in5 + in3, icos36h[1]);
    out[ 8]=
    out[ 9]= in4 + in1;
    out[ 2]=
    out[ 3]= in4 - in1;

    in0 -= in2;
    in5 = MULH(2*(in5 - in3), icos36h[7]);
    out[ 0]=
    out[ 5]= in0 - in5;
    out[ 6]=
    out[11]= in0 + in5;
}

static void imdct36(int *out, int *buf, int *in, int *win)
{
    int i, j, t0, t1, t2, t3, s0, s1, s2, s3;
    int tmp[18], *tmp1, *in1;

    for(i=17;i>=1;i--)
        in[i] += in[i-1];
    for(i=17;i>=3;i-=2)
        in[i] += in[i-2];

    for(j=0;j<2;j++) {
        tmp1 = tmp + j;
        in1 = in + j;
        t2 = in1[2*4] + in1[2*8] - in1[2*2];

        t3 = in1[2*0] + (in1[2*6]>>1);
        t1 = in1[2*0] - in1[2*6];
        tmp1[ 6] = t1 - (t2>>1);
        tmp1[16] = t1 + t2;

        t0 = MULH(2*(in1[2*2] + in1[2*4]),    C2);
        t1 = MULH(   in1[2*4] - in1[2*8] , -2*C8);
        t2 = MULH(2*(in1[2*2] + in1[2*8]),   -C4);

        tmp1[10] = t3 - t0 - t2;
        tmp1[ 2] = t3 + t0 + t1;
        tmp1[14] = t3 + t2 - t1;

        tmp1[ 4] = MULH(2*(in1[2*5] + in1[2*7] - in1[2*1]), -C3);
        t2 = MULH(2*(in1[2*1] + in1[2*5]),    C1);
        t3 = MULH(   in1[2*5] - in1[2*7] , -2*C7);
        t0 = MULH(2*in1[2*3], C3);

        t1 = MULH(2*(in1[2*1] + in1[2*7]),   -C5);

        tmp1[ 0] = t2 + t3 + t0;
        tmp1[12] = t2 + t1 - t0;
        tmp1[ 8] = t3 - t1 - t0;
    }

    i = 0;
    for(j=0;j<4;j++) {
        t0 = tmp[i];
        t1 = tmp[i + 2];
        s0 = t1 + t0;
        s2 = t1 - t0;

        t2 = tmp[i + 1];
        t3 = tmp[i + 3];
        s1 = MULH(2*(t3 + t2), icos36h[j]);
        s3 = MULL(t3 - t2, icos36[8 - j]);

        t0 = s0 + s1;
        t1 = s0 - s1;
        out[(9 + j)*SBLIMIT] =  MULH(t1, win[9 + j]) + buf[9 + j];
        out[(8 - j)*SBLIMIT] =  MULH(t1, win[8 - j]) + buf[8 - j];
        buf[9 + j] = MULH(t0, win[18 + 9 + j]);
        buf[8 - j] = MULH(t0, win[18 + 8 - j]);

        t0 = s2 + s3;
        t1 = s2 - s3;
        out[(9 + 8 - j)*SBLIMIT] =  MULH(t1, win[9 + 8 - j]) + buf[9 + 8 - j];
        out[(        j)*SBLIMIT] =  MULH(t1, win[        j]) + buf[        j];
        buf[9 + 8 - j] = MULH(t0, win[18 + 9 + 8 - j]);
        buf[      + j] = MULH(t0, win[18         + j]);
        i += 4;
    }

    s0 = tmp[16];
    s1 = MULH(2*tmp[17], icos36h[4]);
    t0 = s0 + s1;
    t1 = s0 - s1;
    out[(9 + 4)*SBLIMIT] =  MULH(t1, win[9 + 4]) + buf[9 + 4];
    out[(8 - 4)*SBLIMIT] =  MULH(t1, win[8 - 4]) + buf[8 - 4];
    buf[9 + 4] = MULH(t0, win[18 + 9 + 4]);
    buf[8 - 4] = MULH(t0, win[18 + 8 - 4]);
}

static void compute_imdct(
    mp3_context_t *s, granule_t *g, int32 *sb_samples, int32 *mdct_buf
) {
    int32 *ptr, *win, *win1, *buf, *out_ptr, *ptr1;
    int32 out2[12];
    int i, j, mdct_long_end, v, sblimit;

    /* find last non zero block */
    ptr = g->sb_hybrid + 576;
    ptr1 = g->sb_hybrid + 2 * 18;
    while (ptr >= ptr1) {
        ptr -= 6;
        v = ptr[0] | ptr[1] | ptr[2] | ptr[3] | ptr[4] | ptr[5];
        if (v != 0)
            break;
    }
    sblimit = int((ptr - g->sb_hybrid) / 18) + 1;

    if (g->block_type == 2) {
        /* XXX: check for 8000 Hz */
        if (g->switch_point)
            mdct_long_end = 2;
        else
            mdct_long_end = 0;
    } else {
        mdct_long_end = sblimit;
    }

    buf = mdct_buf;
    ptr = g->sb_hybrid;
    for(j=0;j<mdct_long_end;j++) {
        /* apply window & overlap with previous buffer */
        out_ptr = sb_samples + j;
        /* select window */
        if (g->switch_point && j < 2)
            win1 = mdct_win[0];
        else
            win1 = mdct_win[g->block_type];
        /* select frequency inversion */
        win = win1 + ((4 * 36) & -(j & 1));
        imdct36(out_ptr, buf, ptr, win);
        out_ptr += 18*SBLIMIT;
        ptr += 18;
        buf += 18;
    }
    for(j=mdct_long_end;j<sblimit;j++) {
        /* select frequency inversion */
        win = mdct_win[2] + ((4 * 36) & -(j & 1));
        out_ptr = sb_samples + j;

        for(i=0; i<6; i++){
            *out_ptr = buf[i];
            out_ptr += SBLIMIT;
        }
        imdct12(out2, ptr + 0);
        for(i=0;i<6;i++) {
            *out_ptr = MULH(out2[i], win[i]) + buf[i + 6*1];
            buf[i + 6*2] = MULH(out2[i + 6], win[i + 6]);
            out_ptr += SBLIMIT;
        }
        imdct12(out2, ptr + 1);
        for(i=0;i<6;i++) {
            *out_ptr = MULH(out2[i], win[i]) + buf[i + 6*2];
            buf[i + 6*0] = MULH(out2[i + 6], win[i + 6]);
            out_ptr += SBLIMIT;
        }
        imdct12(out2, ptr + 2);
        for(i=0;i<6;i++) {
            buf[i + 6*0] = MULH(out2[i], win[i]) + buf[i + 6*0];
            buf[i + 6*1] = MULH(out2[i + 6], win[i + 6]);
            buf[i + 6*2] = 0;
        }
        ptr += 18;
        buf += 18;
    }
    /* zero bands */
    for(j=sblimit;j<SBLIMIT;j++) {
        /* overlap */
        out_ptr = sb_samples + j;
        for(i=0;i<18;i++) {
            *out_ptr = buf[i];
            buf[i] = 0;
            out_ptr += SBLIMIT;
        }
        buf += 18;
    }
}

#define SUM8(sum, op, w, p) \
{                                               \
    sum op MULS((w)[0 * 64], p[0 * 64]);\
    sum op MULS((w)[1 * 64], p[1 * 64]);\
    sum op MULS((w)[2 * 64], p[2 * 64]);\
    sum op MULS((w)[3 * 64], p[3 * 64]);\
    sum op MULS((w)[4 * 64], p[4 * 64]);\
    sum op MULS((w)[5 * 64], p[5 * 64]);\
    sum op MULS((w)[6 * 64], p[6 * 64]);\
    sum op MULS((w)[7 * 64], p[7 * 64]);\
}

#define SUM8P2(sum1, op1, sum2, op2, w1, w2, p) \
{                                               \
    int tmp;\
    tmp = p[0 * 64];\
    sum1 op1 MULS((w1)[0 * 64], tmp);\
    sum2 op2 MULS((w2)[0 * 64], tmp);\
    tmp = p[1 * 64];\
    sum1 op1 MULS((w1)[1 * 64], tmp);\
    sum2 op2 MULS((w2)[1 * 64], tmp);\
    tmp = p[2 * 64];\
    sum1 op1 MULS((w1)[2 * 64], tmp);\
    sum2 op2 MULS((w2)[2 * 64], tmp);\
    tmp = p[3 * 64];\
    sum1 op1 MULS((w1)[3 * 64], tmp);\
    sum2 op2 MULS((w2)[3 * 64], tmp);\
    tmp = p[4 * 64];\
    sum1 op1 MULS((w1)[4 * 64], tmp);\
    sum2 op2 MULS((w2)[4 * 64], tmp);\
    tmp = p[5 * 64];\
    sum1 op1 MULS((w1)[5 * 64], tmp);\
    sum2 op2 MULS((w2)[5 * 64], tmp);\
    tmp = p[6 * 64];\
    sum1 op1 MULS((w1)[6 * 64], tmp);\
    sum2 op2 MULS((w2)[6 * 64], tmp);\
    tmp = p[7 * 64];\
    sum1 op1 MULS((w1)[7 * 64], tmp);\
    sum2 op2 MULS((w2)[7 * 64], tmp);\
}

#define COS0_0  FIXHR(0.50060299823519630134/2)
#define COS0_1  FIXHR(0.50547095989754365998/2)
#define COS0_2  FIXHR(0.51544730992262454697/2)
#define COS0_3  FIXHR(0.53104259108978417447/2)
#define COS0_4  FIXHR(0.55310389603444452782/2)
#define COS0_5  FIXHR(0.58293496820613387367/2)
#define COS0_6  FIXHR(0.62250412303566481615/2)
#define COS0_7  FIXHR(0.67480834145500574602/2)
#define COS0_8  FIXHR(0.74453627100229844977/2)
#define COS0_9  FIXHR(0.83934964541552703873/2)
#define COS0_10 FIXHR(0.97256823786196069369/2)
#define COS0_11 FIXHR(1.16943993343288495515/4)
#define COS0_12 FIXHR(1.48416461631416627724/4)
#define COS0_13 FIXHR(2.05778100995341155085/8)
#define COS0_14 FIXHR(3.40760841846871878570/8)
#define COS0_15 FIXHR(10.19000812354805681150/32)

#define COS1_0 FIXHR(0.50241928618815570551/2)
#define COS1_1 FIXHR(0.52249861493968888062/2)
#define COS1_2 FIXHR(0.56694403481635770368/2)
#define COS1_3 FIXHR(0.64682178335999012954/2)
#define COS1_4 FIXHR(0.78815462345125022473/2)
#define COS1_5 FIXHR(1.06067768599034747134/4)
#define COS1_6 FIXHR(1.72244709823833392782/4)
#define COS1_7 FIXHR(5.10114861868916385802/16)

#define COS2_0 FIXHR(0.50979557910415916894/2)
#define COS2_1 FIXHR(0.60134488693504528054/2)
#define COS2_2 FIXHR(0.89997622313641570463/2)
#define COS2_3 FIXHR(2.56291544774150617881/8)

#define COS3_0 FIXHR(0.54119610014619698439/2)
#define COS3_1 FIXHR(1.30656296487637652785/4)

#define COS4_0 FIXHR(0.70710678118654752439/2)

#define BF(a, b, c, s)\
{\
    tmp0 = tab[a] + tab[b];\
    tmp1 = tab[a] - tab[b];\
    tab[a] = tmp0;\
    tab[b] = MULH(tmp1<<(s), c);\
}

#define BF1(a, b, c, d)\
{\
    BF(a, b, COS4_0, 1);\
    BF(c, d,-COS4_0, 1);\
    tab[c] += tab[d];\
}

#define BF2(a, b, c, d)\
{\
    BF(a, b, COS4_0, 1);\
    BF(c, d,-COS4_0, 1);\
    tab[c] += tab[d];\
    tab[a] += tab[c];\
    tab[c] += tab[b];\
    tab[b] += tab[d];\
}

#define ADD(a, b) tab[a] += tab[b]

static void dct32(int32 *out, int32 *tab)
{
    int tmp0, tmp1;

    /* pass 1 */
    BF( 0, 31, COS0_0 , 1);
    BF(15, 16, COS0_15, 5);
    /* pass 2 */
    BF( 0, 15, COS1_0 , 1);
    BF(16, 31,-COS1_0 , 1);
    /* pass 1 */
    BF( 7, 24, COS0_7 , 1);
    BF( 8, 23, COS0_8 , 1);
    /* pass 2 */
    BF( 7,  8, COS1_7 , 4);
    BF(23, 24,-COS1_7 , 4);
    /* pass 3 */
    BF( 0,  7, COS2_0 , 1);
    BF( 8, 15,-COS2_0 , 1);
    BF(16, 23, COS2_0 , 1);
    BF(24, 31,-COS2_0 , 1);
    /* pass 1 */
    BF( 3, 28, COS0_3 , 1);
    BF(12, 19, COS0_12, 2);
    /* pass 2 */
    BF( 3, 12, COS1_3 , 1);
    BF(19, 28,-COS1_3 , 1);
    /* pass 1 */
    BF( 4, 27, COS0_4 , 1);
    BF(11, 20, COS0_11, 2);
    /* pass 2 */
    BF( 4, 11, COS1_4 , 1);
    BF(20, 27,-COS1_4 , 1);
    /* pass 3 */
    BF( 3,  4, COS2_3 , 3);
    BF(11, 12,-COS2_3 , 3);
    BF(19, 20, COS2_3 , 3);
    BF(27, 28,-COS2_3 , 3);
    /* pass 4 */
    BF( 0,  3, COS3_0 , 1);
    BF( 4,  7,-COS3_0 , 1);
    BF( 8, 11, COS3_0 , 1);
    BF(12, 15,-COS3_0 , 1);
    BF(16, 19, COS3_0 , 1);
    BF(20, 23,-COS3_0 , 1);
    BF(24, 27, COS3_0 , 1);
    BF(28, 31,-COS3_0 , 1);



    /* pass 1 */
    BF( 1, 30, COS0_1 , 1);
    BF(14, 17, COS0_14, 3);
    /* pass 2 */
    BF( 1, 14, COS1_1 , 1);
    BF(17, 30,-COS1_1 , 1);
    /* pass 1 */
    BF( 6, 25, COS0_6 , 1);
    BF( 9, 22, COS0_9 , 1);
    /* pass 2 */
    BF( 6,  9, COS1_6 , 2);
    BF(22, 25,-COS1_6 , 2);
    /* pass 3 */
    BF( 1,  6, COS2_1 , 1);
    BF( 9, 14,-COS2_1 , 1);
    BF(17, 22, COS2_1 , 1);
    BF(25, 30,-COS2_1 , 1);

    /* pass 1 */
    BF( 2, 29, COS0_2 , 1);
    BF(13, 18, COS0_13, 3);
    /* pass 2 */
    BF( 2, 13, COS1_2 , 1);
    BF(18, 29,-COS1_2 , 1);
    /* pass 1 */
    BF( 5, 26, COS0_5 , 1);
    BF(10, 21, COS0_10, 1);
    /* pass 2 */
    BF( 5, 10, COS1_5 , 2);
    BF(21, 26,-COS1_5 , 2);
    /* pass 3 */
    BF( 2,  5, COS2_2 , 1);
    BF(10, 13,-COS2_2 , 1);
    BF(18, 21, COS2_2 , 1);
    BF(26, 29,-COS2_2 , 1);
    /* pass 4 */
    BF( 1,  2, COS3_1 , 2);
    BF( 5,  6,-COS3_1 , 2);
    BF( 9, 10, COS3_1 , 2);
    BF(13, 14,-COS3_1 , 2);
    BF(17, 18, COS3_1 , 2);
    BF(21, 22,-COS3_1 , 2);
    BF(25, 26, COS3_1 , 2);
    BF(29, 30,-COS3_1 , 2);

    /* pass 5 */
    BF1( 0,  1,  2,  3);
    BF2( 4,  5,  6,  7);
    BF1( 8,  9, 10, 11);
    BF2(12, 13, 14, 15);
    BF1(16, 17, 18, 19);
    BF2(20, 21, 22, 23);
    BF1(24, 25, 26, 27);
    BF2(28, 29, 30, 31);

    /* pass 6 */

    ADD( 8, 12);
    ADD(12, 10);
    ADD(10, 14);
    ADD(14,  9);
    ADD( 9, 13);
    ADD(13, 11);
    ADD(11, 15);

    out[ 0] = tab[0];
    out[16] = tab[1];
    out[ 8] = tab[2];
    out[24] = tab[3];
    out[ 4] = tab[4];
    out[20] = tab[5];
    out[12] = tab[6];
    out[28] = tab[7];
    out[ 2] = tab[8];
    out[18] = tab[9];
    out[10] = tab[10];
    out[26] = tab[11];
    out[ 6] = tab[12];
    out[22] = tab[13];
    out[14] = tab[14];
    out[30] = tab[15];

    ADD(24, 28);
    ADD(28, 26);
    ADD(26, 30);
    ADD(30, 25);
    ADD(25, 29);
    ADD(29, 27);
    ADD(27, 31);

    out[ 1] = tab[16] + tab[24];
    out[17] = tab[17] + tab[25];
    out[ 9] = tab[18] + tab[26];
    out[25] = tab[19] + tab[27];
    out[ 5] = tab[20] + tab[28];
    out[21] = tab[21] + tab[29];
    out[13] = tab[22] + tab[30];
    out[29] = tab[23] + tab[31];
    out[ 3] = tab[24] + tab[20];
    out[19] = tab[25] + tab[21];
    out[11] = tab[26] + tab[22];
    out[27] = tab[27] + tab[23];
    out[ 7] = tab[28] + tab[18];
    out[23] = tab[29] + tab[19];
    out[15] = tab[30] + tab[17];
    out[31] = tab[31];
}

static void mp3_synth_filter(
    int16 *synth_buf_ptr, int *synth_buf_offset,
    int16 *window, int *dither_state,
    int16 *samples, int incr,
    int32 sb_samples[SBLIMIT]
) {
    int32 tmp[32];
    int16 *synth_buf;
    const int16 *w, *w2, *p;
    int j, offset, v;
    int16 *samples2;
    int sum, sum2;

    dct32(tmp, sb_samples);

    offset = *synth_buf_offset;
    synth_buf = synth_buf_ptr + offset;

    for(j=0;j<32;j++) {
        v = tmp[j];
        /* NOTE: can cause a loss in precision if very high amplitude
           sound */
        if (v > 32767)
            v = 32767;
        else if (v < -32768)
            v = -32768;
        synth_buf[j] = v;
    }
    /* copy to avoid wrap */
    libc_memcpy(synth_buf + 512, synth_buf, 32 * sizeof(int16));

    samples2 = samples + 31 * incr;
    w = window;
    w2 = window + 31;

    sum = *dither_state;
    p = synth_buf + 16;
    SUM8(sum, +=, w, p);
    p = synth_buf + 48;
    SUM8(sum, -=, w + 32, p);
    *samples = round_sample(&sum);
    samples += incr;
    w++;

    /* we calculate two samples at the same time to avoid one memory
       access per two sample */
    for(j=1;j<16;j++) {
        sum2 = 0;
        p = synth_buf + 16 + j;
        SUM8P2(sum, +=, sum2, -=, w, w2, p);
        p = synth_buf + 48 - j;
        SUM8P2(sum, -=, sum2, -=, w + 32, w2 + 32, p);

        *samples = round_sample(&sum);
        samples += incr;
        sum += sum2;
        *samples2 = round_sample(&sum);
        samples2 -= incr;
        w++;
        w2--;
    }

    p = synth_buf + 32;
    SUM8(sum, -=, w + 32, p);
    *samples = round_sample(&sum);
    *dither_state= sum;

    offset = (offset - 32) & 511;
    *synth_buf_offset = offset;
}

////////////////////////////////////////////////////////////////////////////////

static int decode_header(mp3_context_t *s, uint32 header) {
    int sample_rate, frame_size, mpeg25, padding;
    int sample_rate_index, bitrate_index;
    if (header & (1<<20)) {
        s->lsf = (header & (1<<19)) ? 0 : 1;
        mpeg25 = 0;
    } else {
        s->lsf = 1;
        mpeg25 = 1;
    }

    sample_rate_index = (header >> 10) & 3;
    sample_rate = mp3_freq_tab[sample_rate_index] >> (s->lsf + mpeg25);
    sample_rate_index += 3 * (s->lsf + mpeg25);
    s->sample_rate_index = sample_rate_index;
    s->error_protection = ((header >> 16) & 1) ^ 1;
    s->sample_rate = sample_rate;

    bitrate_index = (header >> 12) & 0xf;
    padding = (header >> 9) & 1;
    s->mode = (header >> 6) & 3;
    s->mode_ext = (header >> 4) & 3;
    s->nb_channels = (s->mode == MP3_MONO) ? 1 : 2;

    if (bitrate_index != 0) {
        frame_size = mp3_bitrate_tab[s->lsf][bitrate_index];
        s->bit_rate = frame_size * 1000;
        s->frame_size = (frame_size * 144000) / (sample_rate << s->lsf) + padding;
    } else {
        /* if no frame size computed, signal it */
        return 1;
    }
    return 0;
}

static int mp_decode_layer3(mp3_context_t *s) {
    int nb_granules, main_data_begin, private_bits;
    int gr, ch, blocksplit_flag, i, j, k, n, bits_pos;
    granule_t *g;
    static granule_t granules[2][2];
    static int16 exponents[576];
    const uint8 *ptr;

    if (s->lsf) {
        main_data_begin = get_bits(&s->gb, 8);
        private_bits = get_bits(&s->gb, s->nb_channels);
        nb_granules = 1;
    } else {
        main_data_begin = get_bits(&s->gb, 9);
        if (s->nb_channels == 2)
            private_bits = get_bits(&s->gb, 3);
        else
            private_bits = get_bits(&s->gb, 5);
        nb_granules = 2;
        for(ch=0;ch<s->nb_channels;ch++) {
            granules[ch][0].scfsi = 0; /* all scale factors are transmitted */
            granules[ch][1].scfsi = get_bits(&s->gb, 4);
        }
    }

    for(gr=0;gr<nb_granules;gr++) {
        for(ch=0;ch<s->nb_channels;ch++) {
            g = &granules[ch][gr];
            g->part2_3_length = get_bits(&s->gb, 12);
            g->big_values = get_bits(&s->gb, 9);
            g->global_gain = get_bits(&s->gb, 8);
            /* if MS stereo only is selected, we precompute the
               1/sqrt(2) renormalization factor */
            if ((s->mode_ext & (MODE_EXT_MS_STEREO | MODE_EXT_I_STEREO)) ==
                MODE_EXT_MS_STEREO)
                g->global_gain -= 2;
            if (s->lsf)
                g->scalefac_compress = get_bits(&s->gb, 9);
            else
                g->scalefac_compress = get_bits(&s->gb, 4);
            blocksplit_flag = get_bits(&s->gb, 1);
            if (blocksplit_flag) {
                g->block_type = get_bits(&s->gb, 2);
                if (g->block_type == 0)
                    return -1;
                g->switch_point = get_bits(&s->gb, 1);
                for(i=0;i<2;i++)
                    g->table_select[i] = get_bits(&s->gb, 5);
                for(i=0;i<3;i++)
                    g->subblock_gain[i] = get_bits(&s->gb, 3);
                /* compute huffman coded region sizes */
                if (g->block_type == 2)
                    g->region_size[0] = (36 / 2);
                else {
                    if (s->sample_rate_index <= 2)
                        g->region_size[0] = (36 / 2);
                    else if (s->sample_rate_index != 8)
                        g->region_size[0] = (54 / 2);
                    else
                        g->region_size[0] = (108 / 2);
                }
                g->region_size[1] = (576 / 2);
            } else {
                int region_address1, region_address2, l;
                g->block_type = 0;
                g->switch_point = 0;
                for(i=0;i<3;i++)
                    g->table_select[i] = get_bits(&s->gb, 5);
                /* compute huffman coded region sizes */
                region_address1 = get_bits(&s->gb, 4);
                region_address2 = get_bits(&s->gb, 3);
                g->region_size[0] =
                    band_index_long[s->sample_rate_index][region_address1 + 1] >> 1;
                l = region_address1 + region_address2 + 2;
                /* should not overflow */
                if (l > 22)
                    l = 22;
                g->region_size[1] =
                    band_index_long[s->sample_rate_index][l] >> 1;
            }
            /* convert region offsets to region sizes and truncate
               size to big_values */
            g->region_size[2] = (576 / 2);
            j = 0;
            for(i=0;i<3;i++) {
                k = g->region_size[i];
                if (g->big_values < k) k = g->big_values;
                g->region_size[i] = k - j;
                j = k;
            }

            /* compute band indexes */
            if (g->block_type == 2) {
                if (g->switch_point) {
                    /* if switched mode, we handle the 36 first samples as
                       long blocks.  For 8000Hz, we handle the 48 first
                       exponents as long blocks (XXX: check this!) */
                    if (s->sample_rate_index <= 2)
                        g->long_end = 8;
                    else if (s->sample_rate_index != 8)
                        g->long_end = 6;
                    else
                        g->long_end = 4; /* 8000 Hz */

                    g->short_start = 2 + (s->sample_rate_index != 8);
                } else {
                    g->long_end = 0;
                    g->short_start = 0;
                }
            } else {
                g->short_start = 13;
                g->long_end = 22;
            }

            g->preflag = 0;
            if (!s->lsf)
                g->preflag = get_bits(&s->gb, 1);
            g->scalefac_scale = get_bits(&s->gb, 1);
            g->count1table_select = get_bits(&s->gb, 1);
        }
    }

    ptr = s->gb.buffer + (get_bits_count(&s->gb)>>3);
    /* now we get bits from the main_data_begin offset */
    if(main_data_begin > s->last_buf_size){
        s->last_buf_size= main_data_begin;
      }

    memcpy(s->last_buf + s->last_buf_size, ptr, EXTRABYTES);
    s->in_gb= s->gb;
    init_get_bits(&s->gb, s->last_buf + s->last_buf_size - main_data_begin, main_data_begin*8);

    for(gr=0;gr<nb_granules;gr++) {
        for(ch=0;ch<s->nb_channels;ch++) {
            g = &granules[ch][gr];

            bits_pos = get_bits_count(&s->gb);

            if (!s->lsf) {
                uint8 *sc;
                int slen, slen1, slen2;

                /* MPEG1 scale factors */
                slen1 = slen_table[0][g->scalefac_compress];
                slen2 = slen_table[1][g->scalefac_compress];
                if (g->block_type == 2) {
                    n = g->switch_point ? 17 : 18;
                    j = 0;
                    if(slen1){
                        for(i=0;i<n;i++)
                            g->scale_factors[j++] = get_bits(&s->gb, slen1);
                    }else{
                        libc_memset((void*) &g->scale_factors[j], 0, n);
                        j += n;
//                        for(i=0;i<n;i++)
//                            g->scale_factors[j++] = 0;
                    }
                    if(slen2){
                        for(i=0;i<18;i++)
                            g->scale_factors[j++] = get_bits(&s->gb, slen2);
                        for(i=0;i<3;i++)
                            g->scale_factors[j++] = 0;
                    }else{
                        for(i=0;i<21;i++)
                            g->scale_factors[j++] = 0;
                    }
                } else {
                    sc = granules[ch][0].scale_factors;
                    j = 0;
                    for(k=0;k<4;k++) {
                        n = (k == 0 ? 6 : 5);
                        if ((g->scfsi & (0x8 >> k)) == 0) {
                            slen = (k < 2) ? slen1 : slen2;
                            if(slen){
                                for(i=0;i<n;i++)
                                    g->scale_factors[j++] = get_bits(&s->gb, slen);
                            }else{
                                libc_memset((void*) &g->scale_factors[j], 0, n);
                                j += n;
//                                for(i=0;i<n;i++)
//                                    g->scale_factors[j++] = 0;
                            }
                        } else {
                            /* simply copy from last granule */
                            for(i=0;i<n;i++) {
                                g->scale_factors[j] = sc[j];
                                j++;
                            }
                        }
                    }
                    g->scale_factors[j++] = 0;
                }
            } else {
                int tindex, tindex2, slen[4], sl, sf;

                /* LSF scale factors */
                if (g->block_type == 2) {
                    tindex = g->switch_point ? 2 : 1;
                } else {
                    tindex = 0;
                }
                sf = g->scalefac_compress;
                if ((s->mode_ext & MODE_EXT_I_STEREO) && ch == 1) {
                    /* intensity stereo case */
                    sf >>= 1;
                    if (sf < 180) {
                        lsf_sf_expand(slen, sf, 6, 6, 0);
                        tindex2 = 3;
                    } else if (sf < 244) {
                        lsf_sf_expand(slen, sf - 180, 4, 4, 0);
                        tindex2 = 4;
                    } else {
                        lsf_sf_expand(slen, sf - 244, 3, 0, 0);
                        tindex2 = 5;
                    }
                } else {
                    /* normal case */
                    if (sf < 400) {
                        lsf_sf_expand(slen, sf, 5, 4, 4);
                        tindex2 = 0;
                    } else if (sf < 500) {
                        lsf_sf_expand(slen, sf - 400, 5, 4, 0);
                        tindex2 = 1;
                    } else {
                        lsf_sf_expand(slen, sf - 500, 3, 0, 0);
                        tindex2 = 2;
                        g->preflag = 1;
                    }
                }

                j = 0;
                for(k=0;k<4;k++) {
                    n = lsf_nsf_table[tindex2][tindex][k];
                    sl = slen[k];
                    if(sl){
                        for(i=0;i<n;i++)
                            g->scale_factors[j++] = get_bits(&s->gb, sl);
                    }else{
                        libc_memset((void*) &g->scale_factors[j], 0, n);
                        j += n;                        
//                        for(i=0;i<n;i++)
//                            g->scale_factors[j++] = 0;
                    }
                }
                /* XXX: should compute exact size */
                libc_memset((void*) &g->scale_factors[j], 0, 40 - j);
//                for(;j<40;j++)
//                    g->scale_factors[j] = 0;
            }

            exponents_from_scale_factors(s, g, exponents);

            /* read Huffman coded residue */
            if (huffman_decode(s, g, exponents,
                               bits_pos + g->part2_3_length) < 0)
                return -1;
        } /* ch */

        if (s->nb_channels == 2)
            compute_stereo(s, &granules[0][gr], &granules[1][gr]);

        for(ch=0;ch<s->nb_channels;ch++) {
            g = &granules[ch][gr];
            reorder_block(s, g);
            compute_antialias(s, g);
            compute_imdct(s, g, &s->sb_samples[ch][18 * gr][0], s->mdct_buf[ch]);
        }
    } /* gr */
    return nb_granules * 18;
}

static int mp3_decode_main(
    mp3_context_t *s,
    int16 *samples, const uint8 *buf, int buf_size
) {
    int i, nb_frames, ch;
    int16 *samples_ptr;

    init_get_bits(&s->gb, buf + HEADER_SIZE, (buf_size - HEADER_SIZE)*8);

    if (s->error_protection)
        get_bits(&s->gb, 16);

        nb_frames = mp_decode_layer3(s);

        s->last_buf_size=0;
        if(s->in_gb.buffer){
            align_get_bits(&s->gb);
            i= (s->gb.size_in_bits - get_bits_count(&s->gb))>>3;
            if(i >= 0 && i <= BACKSTEP_SIZE){
                libc_memmove(s->last_buf, s->gb.buffer + (get_bits_count(&s->gb)>>3), i);
                s->last_buf_size=i;
            }
            s->gb= s->in_gb;
        }

        align_get_bits(&s->gb);
        i= (s->gb.size_in_bits - get_bits_count(&s->gb))>>3;

        if(i<0 || i > BACKSTEP_SIZE || nb_frames<0){
            i = buf_size - HEADER_SIZE;
            if (BACKSTEP_SIZE < i) i = BACKSTEP_SIZE;
        }
        libc_memcpy(s->last_buf + s->last_buf_size, s->gb.buffer + buf_size - HEADER_SIZE - i, i);
        s->last_buf_size += i;

    /* apply the synthesis filter */
    for(ch=0;ch<s->nb_channels;ch++) {
        samples_ptr = samples + ch;
        for(i=0;i<nb_frames;i++) {
            mp3_synth_filter(
                s->synth_buf[ch], &(s->synth_buf_offset[ch]),
                window, &s->dither_state,
                samples_ptr, s->nb_channels,
                s->sb_samples[ch][i]
            );
            samples_ptr += 32 * s->nb_channels;
        }
    }
    return nb_frames * 32 * sizeof(uint16) * s->nb_channels;
}

////////////////////////////////////////////////////////////////////////////////

int mp3_decode_init() {
    static int init=0;
    int i, j, k;

    if (!init) {
        /* synth init */
        for(i=0;i<257;i++) {
            int v;
            v = mp3_enwindow[i];
            #if WFRAC_BITS < 16
                v = (v + (1 << (16 - WFRAC_BITS - 1))) >> (16 - WFRAC_BITS);
            #endif
            window[i] = v;
            if ((i & 63) != 0)
                v = -v;
            if (i != 0)
                window[512 - i] = v;
        }

        /* huffman decode tables */
        for(i=1;i<16;i++) {
            const huff_table_t *h = &mp3_huff_tables[i];
            int xsize, x, y;
            unsigned int n;
            uint8  tmp_bits [512];
            uint16 tmp_codes[512];

            libc_memset(tmp_bits , 0, sizeof(tmp_bits ));
            libc_memset(tmp_codes, 0, sizeof(tmp_codes));

            xsize = h->xsize;
            n = xsize * xsize;

            j = 0;
            for(x=0;x<xsize;x++) {
                for(y=0;y<xsize;y++){
                    tmp_bits [(x << 5) | y | ((x&&y)<<4)]= h->bits [j  ];
                    tmp_codes[(x << 5) | y | ((x&&y)<<4)]= h->codes[j++];
                }
            }

            init_vlc(&huff_vlc[i], 7, 512,
                     tmp_bits, 1, 1, tmp_codes, 2, 2);
        }
        for(i=0;i<2;i++) {
            init_vlc(&huff_quad_vlc[i], i == 0 ? 7 : 4, 16,
                     mp3_quad_bits[i], 1, 1, mp3_quad_codes[i], 1, 1);
        }

        for(i=0;i<9;i++) {
            k = 0;
            for(j=0;j<22;j++) {
                band_index_long[i][j] = k;
                k += band_size_long[i][j];
            }
            band_index_long[i][22] = k;
        }

        /* compute n ^ (4/3) and store it in mantissa/exp format */
        table_4_3_exp = (int8*)libc_malloc(TABLE_4_3_SIZE * sizeof(table_4_3_exp[0]));
        if(!table_4_3_exp)
            return -1;
        table_4_3_value = (uint32*)libc_malloc(TABLE_4_3_SIZE * sizeof(table_4_3_value[0]));
        if(!table_4_3_value)
            return -1;

        for(i=1;i<TABLE_4_3_SIZE;i++) {
            double f, fm;
            int e, m;
            f = libc_pow((double)(i/4), 4.0 / 3.0) * libc_pow(2, (i&3)*0.25);
            fm = libc_frexp(f, &e);
            m = (uint32)(fm*(1LL<<31) + 0.5);
            e+= FRAC_BITS - 31 + 5 - 100;
            table_4_3_value[i] = m;
            table_4_3_exp[i] = -e;
        }
        for(i=0; i<512*16; i++){
            int exponent= (i>>4);
            double f= libc_pow(i&15, 4.0 / 3.0) * libc_pow(2, (exponent-400)*0.25 + FRAC_BITS + 5);
            expval_table[exponent][i&15]= uint32(f);
            if((i&15)==1)
                exp_table[exponent]= uint32(f);
        }

        for(i=0;i<7;i++) {
            float f;
            int v;
            if (i != 6) {
                f = float(tan((double)i * PI / 12.0));
                v = FIXR(f / (1.0 + f));
            } else {
                v = FIXR(1.0);
            }
            is_table[0][i] = v;
            is_table[1][6 - i] = v;
        }
        for(i=7;i<16;i++)
            is_table[0][i] = is_table[1][i] = 0;

        for(i=0;i<16;i++) {
            double f;
            int e, k;

            for(j=0;j<2;j++) {
                e = -(j + 1) * ((i + 1) >> 1);
                f = libc_pow(2.0, e / 4.0);
                k = i & 1;
                is_table_lsf[j][k ^ 1][i] = FIXR(f);
                is_table_lsf[j][k][i] = FIXR(1.0);
            }
        }

        for(i=0;i<8;i++) {
            float ci, cs, ca;
            ci = ci_table[i];
            cs = float(1.0 / sqrt(1.0 + ci * ci));
            ca = cs * ci;
            csa_table[i][0] = FIXHR(cs/4);
            csa_table[i][1] = FIXHR(ca/4);
            csa_table[i][2] = FIXHR(ca/4) + FIXHR(cs/4);
            csa_table[i][3] = FIXHR(ca/4) - FIXHR(cs/4);
            csa_table_float[i][0] = cs;
            csa_table_float[i][1] = ca;
            csa_table_float[i][2] = ca + cs;
            csa_table_float[i][3] = ca - cs;
        }

        /* compute mdct windows */
        for(i=0;i<36;i++) {
            for(j=0; j<4; j++){
                double d;

                if(j==2 && i%3 != 1)
                    continue;

                d= sin(PI * (i + 0.5) / 36.0);
                if(j==1){
                    if     (i>=30) d= 0;
                    else if(i>=24) d= sin(PI * (i - 18 + 0.5) / 12.0);
                    else if(i>=18) d= 1;
                }else if(j==3){
                    if     (i<  6) d= 0;
                    else if(i< 12) d= sin(PI * (i -  6 + 0.5) / 12.0);
                    else if(i< 18) d= 1;
                }
                d*= 0.5 / cos(PI*(2*i + 19)/72);
                if(j==2)
                    mdct_win[j][i/3] = FIXHR((d / (1<<5)));
                else
                    mdct_win[j][i  ] = FIXHR((d / (1<<5)));
            }
        }
        for(j=0;j<4;j++) {
            for(i=0;i<36;i+=2) {
                mdct_win[j + 4][i] = mdct_win[j][i];
                mdct_win[j + 4][i + 1] = -mdct_win[j][i + 1];
            }
        }
        init = 1;
    }
    return 0;
}

void mp3_decode_free() {
    libc_free(table_4_3_exp);
    libc_free(table_4_3_value);

    for (int i = 0; i < 16; i++) libc_free(huff_vlc[i].table);
    for (int i = 0; i <  2; i++) libc_free(huff_quad_vlc[i].table);
}

static int mp3_decode_frame(
    mp3_context_t *s,
    int16 *out_samples, int *data_size,
    uint8 *buf, int buf_size
) {
    uint32 header;
    int out_size;
    int extra_bytes = 0;

retry:
    if(buf_size < HEADER_SIZE)
        return -1;

    header = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
    if(mp3_check_header(header) < 0){
        buf++;
        buf_size--;
        extra_bytes++;
        goto retry;
    }

    if (decode_header(s, header) == 1) {
        s->frame_size = -1;
        return -1;
    }

    if(s->frame_size<=0 || s->frame_size > buf_size){
        return -1;  // incomplete frame
    }
    if(s->frame_size < buf_size) {
        buf_size = s->frame_size;
    }

    out_size = mp3_decode_main(s, out_samples, buf, buf_size);
    if(out_size>=0)
        *data_size = out_size;
    // else: Error while decoding MPEG audio frame.
    s->frame_size += extra_bytes;
    return buf_size;
}

////////////////////////////////////////////////////////////////////////////////

mp3_decoder_t mp3_create(void) {
    void *dec = libc_calloc(sizeof(mp3_context_t), 1);
    return (mp3_decoder_t) dec;
}

void mp3_done(mp3_decoder_t dec) {
    if (dec) {
        libc_free(dec);
    }
}

int mp3_decode(mp3_decoder_t dec, void *buf, int bytes, signed short *out, mp3_info_t *info) {
    int res, size = -1;
    mp3_context_t *s = (mp3_context_t*) dec;
    if (!s) return 0;
    res = mp3_decode_frame(s, (int16*) out, &size, (uint8*)buf, bytes);
    if (res < 0) return 0;
    if (info) {
        info->sample_rate = s->sample_rate;
        info->channels = s->nb_channels;
        info->audio_bytes = size;
    }
    return s->frame_size;
}
