#include <stdio.h>
#include <string.h>
#include <math.h>
#include <windows.h>

#include "libimagequant.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"

#define ASSERT(x) { if (!(x)) { DebugBreak(); } }

typedef signed char        int8;
typedef signed short       int16;
typedef signed int         int32;
typedef signed long long   int64;
typedef unsigned char      uint8;
typedef unsigned short     uint16;
typedef unsigned int       uint32;
typedef unsigned long long uint64;

inline uint16 swap16(uint16 x) {
    return ((x & 0x00FF) << 8) | ((x & 0xFF00) >> 8);
}

inline uint32 swap32(uint32 x) {
    return ((x & 0x000000FF) << 24) | ((x & 0x0000FF00) << 8) | ((x & 0x00FF0000) >> 8) | ((x & 0xFF000000) >> 24);
}

template <class T>
inline void swap(T &a, T &b) {
    T tmp = a;
    a = b;
    b = tmp;
}

void launchApp(const char* cmdline)
{
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;

    memset(&si, 0, sizeof(si));
    memset(&pi, 0, sizeof(pi));
    si.cb = sizeof(si);

    CreateProcess(
        NULL,
        (char*)cmdline,
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        NULL,
        &si,
        &pi
    );

    WaitForSingleObject(pi.hProcess, INFINITE);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

#define ITEM_TYPES(E) \
    E( LARA                  ) \
    E( LARA_PISTOLS          ) \
    E( LARA_SHOTGUN          ) \
    E( LARA_MAGNUMS          ) \
    E( LARA_UZIS             ) \
    E( LARA_SPEC             ) \
    E( DOPPELGANGER          ) \
    E( WOLF                  ) \
    E( BEAR                  ) \
    E( BAT                   ) \
    E( CROCODILE_LAND        ) \
    E( CROCODILE_WATER       ) \
    E( LION_MALE             ) \
    E( LION_FEMALE           ) \
    E( PUMA                  ) \
    E( GORILLA               ) \
    E( RAT_LAND              ) \
    E( RAT_WATER             ) \
    E( REX                   ) \
    E( RAPTOR                ) \
    E( MUTANT_1              ) \
    E( MUTANT_2              ) \
    E( MUTANT_3              ) \
    E( CENTAUR               ) \
    E( MUMMY                 ) \
    E( UNUSED_1              ) \
    E( UNUSED_2              ) \
    E( LARSON                ) \
    E( PIERRE                ) \
    E( SKATEBOARD            ) \
    E( SKATER                ) \
    E( COWBOY                ) \
    E( MR_T                  ) \
    E( NATLA                 ) \
    E( ADAM                  ) \
    E( TRAP_FLOOR            ) \
    E( TRAP_SWING_BLADE      ) \
    E( TRAP_SPIKES           ) \
    E( TRAP_BOULDER          ) \
    E( DART                  ) \
    E( TRAP_DART_EMITTER     ) \
    E( DRAWBRIDGE            ) \
    E( TRAP_SLAM             ) \
    E( TRAP_SWORD            ) \
    E( HAMMER_HANDLE         ) \
    E( HAMMER_BLOCK          ) \
    E( LIGHTNING             ) \
    E( MOVING_OBJECT         ) \
    E( BLOCK_1               ) \
    E( BLOCK_2               ) \
    E( BLOCK_3               ) \
    E( BLOCK_4               ) \
    E( MOVING_BLOCK          ) \
    E( TRAP_CEILING          ) \
    E( TRAP_FLOOR_LOD        ) \
    E( SWITCH                ) \
    E( SWITCH_WATER          ) \
    E( DOOR_1                ) \
    E( DOOR_2                ) \
    E( DOOR_3                ) \
    E( DOOR_4                ) \
    E( DOOR_5                ) \
    E( DOOR_6                ) \
    E( DOOR_7                ) \
    E( DOOR_8                ) \
    E( TRAP_DOOR_1           ) \
    E( TRAP_DOOR_2           ) \
    E( TRAP_DOOR_LOD         ) \
    E( BRIDGE_FLAT           ) \
    E( BRIDGE_TILT_1         ) \
    E( BRIDGE_TILT_2         ) \
    E( INV_PASSPORT          ) \
    E( INV_COMPASS           ) \
    E( INV_HOME              ) \
    E( GEARS_1               ) \
    E( GEARS_2               ) \
    E( GEARS_3               ) \
    E( CUT_1                 ) \
    E( CUT_2                 ) \
    E( CUT_3                 ) \
    E( CUT_4                 ) \
    E( INV_PASSPORT_CLOSED   ) \
    E( INV_MAP               ) \
    E( CRYSTAL               ) \
    E( PISTOLS               ) \
    E( SHOTGUN               ) \
    E( MAGNUMS               ) \
    E( UZIS                  ) \
    E( AMMO_PISTOLS          ) \
    E( AMMO_SHOTGUN          ) \
    E( AMMO_MAGNUMS          ) \
    E( AMMO_UZIS             ) \
    E( EXPLOSIVE             ) \
    E( MEDIKIT_SMALL         ) \
    E( MEDIKIT_BIG           ) \
    E( INV_DETAIL            ) \
    E( INV_SOUND             ) \
    E( INV_CONTROLS          ) \
    E( INV_GAMMA             ) \
    E( INV_PISTOLS           ) \
    E( INV_SHOTGUN           ) \
    E( INV_MAGNUMS           ) \
    E( INV_UZIS              ) \
    E( INV_AMMO_PISTOLS      ) \
    E( INV_AMMO_SHOTGUN      ) \
    E( INV_AMMO_MAGNUMS      ) \
    E( INV_AMMO_UZIS         ) \
    E( INV_EXPLOSIVE         ) \
    E( INV_MEDIKIT_SMALL     ) \
    E( INV_MEDIKIT_BIG       ) \
    E( PUZZLE_1              ) \
    E( PUZZLE_2              ) \
    E( PUZZLE_3              ) \
    E( PUZZLE_4              ) \
    E( INV_PUZZLE_1          ) \
    E( INV_PUZZLE_2          ) \
    E( INV_PUZZLE_3          ) \
    E( INV_PUZZLE_4          ) \
    E( PUZZLEHOLE_1          ) \
    E( PUZZLEHOLE_2          ) \
    E( PUZZLEHOLE_3          ) \
    E( PUZZLEHOLE_4          ) \
    E( PUZZLEHOLE_DONE_1     ) \
    E( PUZZLEHOLE_DONE_2     ) \
    E( PUZZLEHOLE_DONE_3     ) \
    E( PUZZLEHOLE_DONE_4     ) \
    E( LEADBAR               ) \
    E( INV_LEADBAR           ) \
    E( MIDAS_HAND            ) \
    E( KEY_ITEM_1            ) \
    E( KEY_ITEM_2            ) \
    E( KEY_ITEM_3            ) \
    E( KEY_ITEM_4            ) \
    E( INV_KEY_ITEM_1        ) \
    E( INV_KEY_ITEM_2        ) \
    E( INV_KEY_ITEM_3        ) \
    E( INV_KEY_ITEM_4        ) \
    E( KEYHOLE_1             ) \
    E( KEYHOLE_2             ) \
    E( KEYHOLE_3             ) \
    E( KEYHOLE_4             ) \
    E( UNUSED_5              ) \
    E( UNUSED_6              ) \
    E( SCION_PICKUP_QUALOPEC ) \
    E( SCION_PICKUP_DROP     ) \
    E( SCION_TARGET          ) \
    E( SCION_PICKUP_HOLDER   ) \
    E( SCION_HOLDER          ) \
    E( UNUSED_7              ) \
    E( UNUSED_8              ) \
    E( INV_SCION             ) \
    E( EXPLOSION             ) \
    E( UNUSED_9              ) \
    E( SPLASH                ) \
    E( UNUSED_10             ) \
    E( BUBBLE                ) \
    E( UNUSED_11             ) \
    E( UNUSED_12             ) \
    E( BLOOD                 ) \
    E( UNUSED_13             ) \
    E( SMOKE                 ) \
    E( CENTAUR_STATUE        ) \
    E( CABIN                 ) \
    E( MUTANT_EGG_SMALL      ) \
    E( RICOCHET              ) \
    E( SPARKLES              ) \
    E( MUZZLE_FLASH          ) \
    E( UNUSED_14             ) \
    E( UNUSED_15             ) \
    E( VIEW_TARGET           ) \
    E( WATERFALL             ) \
    E( NATLA_BULLET          ) \
    E( MUTANT_BULLET         ) \
    E( CENTAUR_BULLET        ) \
    E( UNUSED_16             ) \
    E( UNUSED_17             ) \
    E( LAVA_PARTICLE         ) \
    E( LAVA_EMITTER          ) \
    E( FLAME                 ) \
    E( FLAME_EMITTER         ) \
    E( TRAP_LAVA             ) \
    E( MUTANT_EGG_BIG        ) \
    E( BOAT                  ) \
    E( EARTHQUAKE            ) \
    E( UNUSED_18             ) \
    E( UNUSED_19             ) \
    E( UNUSED_20             ) \
    E( UNUSED_21             ) \
    E( UNUSED_22             ) \
    E( LARA_BRAID            ) \
    E( GLYPHS                )

#define DECL_ENUM(v) ITEM_##v,

enum ItemType {
    ITEM_TYPES(DECL_ENUM)
    TR1_ITEM_MAX,
    ITEM_MAX = TR1_ITEM_MAX
};

#undef DECL_ENUM

struct vec3s
{
    int16 x, y, z;

    vec3s() {}
    vec3s(int16 x, int16 y, int16 z) : x(x), y(y), z(z) {}
};

struct vec3i
{
    int32 x, y, z;
};

struct Tile
{
    uint8 indices[256 * 256];
};

struct Palette
{
    uint8 colors[256 * 3];
};

struct FileStream
{
    FILE* f;

    bool bigEndian;

    FileStream(const char* fileName, bool write) : bigEndian(false)
    {
        f = fopen(fileName, write ? "wb" : "rb");
    }

    ~FileStream()
    {
        if (f) fclose(f);
    }

    bool isValid()
    {
        return f != NULL;
    }

    void seek(int32 offset)
    {
        fseek(f, offset, SEEK_CUR);
    }

    uint32 getPos()
    {
        return ftell(f);
    }

    void setPos(uint32 pos)
    {
        fseek(f, pos, SEEK_SET);
    }

    uint32 align4()
    {
        uint32 pos = getPos();
        uint32 aligned = (pos + 3) & ~3;

        if (aligned != pos) {
            static const uint32 zero = 0;
            fwrite(&zero, 1, aligned - pos, f);
        }

        return aligned;
    }

    uint32 align16()
    {
        uint32 pos = getPos();
        uint32 aligned = (pos + 15) & ~15;

        if (aligned != pos) {
            static const uint32 zero = 0;
            fwrite(&zero, 1, aligned - pos, f);
        }

        return aligned;
    }

    template <typename T>
    void read(T &result)
    {
        fread(&result, sizeof(result), 1, f);
    }

    template <typename T, typename C>
    void read(T* &elements, C count)
    {
        if (count) {
            elements = new T[count];
            fread(&elements[0], sizeof(elements[0]), count, f);
        } else {
            elements = NULL;
        }
    }

    template <typename T, typename C>
    void readArray(T* &elements, C &count)
    {
        read(count);
        read(elements, count);
    }

    void write(int8 value)
    {
        writeRaw(value);
    }

    void write(uint8 value)
    {
        writeRaw(value);
    }

    void write(int16 value)
    {
        if (bigEndian) {
            value = (int16)swap16((uint16)value);
        }
        writeRaw(value);
    }

    void write(uint16 value)
    {
        if (bigEndian) {
            value = swap16(value);
        }
        writeRaw(value);
    }

    void write(int32 value)
    {
        if (bigEndian) {
            value = (int32)swap32((uint32)value);
        }
        writeRaw(value);
    }

    void write(uint32 value)
    {
        if (bigEndian) {
            value = swap32(value);
        }
        writeRaw(value);
    }

    template <typename T, typename C>
    void write(const T* elements, C count)
    {
        if (!elements || !count)
            return;

        for (int32 i = 0; i < count; i++)
        {
            write(elements[i]);
        }
    }

    template <typename T, typename C>
    void writeObj(const T* elements, C count)
    {
        if (!elements || !count)
            return;

        for (int32 i = 0; i < count; i++)
        {
            elements[i].write(*this);
        }
    }

    template <typename T>
    void writeRaw(const T &result)
    {
        fwrite(&result, sizeof(result), 1, f);
    }

private:
    template <typename T, typename C>
    void writeArray(const T* elements, C count)
    {
        write(count);
        write(elements, count);
    }
};

template <typename T>
struct Array
{
    int32 count;
    T** items;

    Array() : count(0), items(NULL) {}

    ~Array()
    {
        delete[] items;
    }

    T* operator [] (int index) const { return items[index]; }

    int32 add(T* item)
    {
        count++;
        T** tmp = new T*[count];
        memcpy(tmp, items, (count - 1) * sizeof(T*));
        tmp[count - 1] = item;
        delete[] items;
        items = tmp;
        return count - 1;
    }

    int32 find(T* item)
    {
        for (int32 i = 0; i < count; i++)
        {
            if (items[i]->isEqual(item))
                return i;
        }
        return -1;
    }

    void qsort(T** v, int L, int R) {
        int i = L;
        int j = R;
        const T* m = v[(L + R) / 2];

        while (i <= j) {
            while (T::cmp(v[i], m) < 0) i++;
            while (T::cmp(m, v[j]) < 0) j--;

            if (i <= j)
            {
                T* tmp = v[i];
                v[i] = v[j];
                v[j] = tmp;
                i++;
                j--;
            }
        }

        if (L < j) qsort(v, L, j);
        if (i < R) qsort(v, i, R);
    }

    void sort() {
        if (count) {
            qsort(items, 0, count - 1);
        }
    }
};

struct _BITMAPFILEHEADER {
    uint32  bfSize;
    uint16  bfReserved1;
    uint16  bfReserved2;
    uint32  bfOffBits;
};

struct _BITMAPINFOHEADER {
    uint32 biSize;
    uint32 biWidth;
    uint32 biHeight;
    uint16 biPlanes;
    uint16 biBitCount;
    uint32 biCompression;
    uint32 biSizeImage;
    uint32 biXPelsPerMeter;
    uint32 biYPelsPerMeter;
    uint32 biClrUsed;
    uint32 biClrImportant;
};

void saveBitmap(const char* fileName, uint8* data, int32 width, int32 height, int32 bpp = 24)
{
    _BITMAPFILEHEADER fhdr;
    _BITMAPINFOHEADER ihdr;

    memset(&fhdr, 0, sizeof(fhdr));
    memset(&ihdr, 0, sizeof(ihdr));

    ihdr.biSize      = sizeof(ihdr);
    ihdr.biWidth     = width;
    ihdr.biHeight    = height;
    ihdr.biPlanes    = 1;
    ihdr.biBitCount  = bpp;
    ihdr.biSizeImage = width * height * bpp / 8;

    fhdr.bfOffBits   = 2 + sizeof(fhdr) + ihdr.biSize;
    fhdr.bfSize      = fhdr.bfOffBits + ihdr.biSizeImage;

    FILE *f = fopen(fileName, "wb");
    if (f) {
        uint16 type = 'B' + ('M' << 8);
        fwrite(&type, sizeof(type), 1, f);
        fwrite(&fhdr, sizeof(fhdr), 1, f);
        fwrite(&ihdr, sizeof(ihdr), 1, f);
        for (int32 i = 0; i < height; i++)
        {
            fwrite(data + (height - i - 1) * width * bpp / 8, bpp / 8, width, f);
        }
        fclose(f);
    }
}

uint8* loadBitmap(const char* fileName, int32* width, int32* height, int32* bpp)
{
    _BITMAPFILEHEADER fhdr;
    _BITMAPINFOHEADER ihdr;

    FILE *f = fopen(fileName, "rb");
    if (!f) return NULL;

    uint16 type;
    fread(&type, sizeof(type), 1, f);
    fread(&fhdr, sizeof(fhdr), 1, f);
    fread(&ihdr, sizeof(ihdr), 1, f);

    *width = ihdr.biWidth;
    *height = ihdr.biHeight;
    *bpp = ihdr.biBitCount;

    uint8* data = new uint8[ihdr.biWidth * ihdr.biHeight * ihdr.biBitCount / 8];
    data += ihdr.biWidth * ihdr.biHeight * ihdr.biBitCount / 8;

    for (int32 i = 0; i < ihdr.biHeight; i++)
    {
        data -= ihdr.biWidth * ihdr.biBitCount / 8;
        fread(data, ihdr.biWidth * ihdr.biBitCount / 8, 1, f);
    }

    fclose(f);

    return data;
}

#define X_CLAMP(x, a, b) ((x) < (a) ? (a) : ((x) > (b) ? (b) : (x)))

void fixLightmap(const uint8* lightmap, uint16* palette, int32 palIndex)
{
    uint16 color = palette[palIndex];

    int32 r = 0x1F & (color);
    int32 g = 0x1F & (color >> 5);
    int32 b = 0x1F & (color >> 10);

    for (int32 i = 0; i < 32; i++)
    {
        int32 lum = 31 - i;

        int32 lumR = X_CLAMP((r * lum) / 14, 0, 31);
        int32 lumG = X_CLAMP((g * lum) / 14, 0, 31);
        int32 lumB = X_CLAMP((b * lum) / 14, 0, 31);

        palette[lightmap[i * 256 + palIndex]] = lumR | (lumG << 5) | (lumB << 10);
    }
}

void fixTexCoord(uint32 uv0, uint32 &uv1)
{
    int32 u0 = uv0 >> 24;
    int32 u1 = uv1 >> 24;
    int32 v0 = (uv0 >> 8) & 0xFF;
    int32 v1 = (uv1 >> 8) & 0xFF;

    if (abs(u1 - u0) > 127) {
        if (u1 > u0) {
            u1 = u0 + 127;
        } else {
            u1 = u0 - 127;
        }
    }

    if (abs(v1 - v0) > 127) {
        if (v1 > v0) {
            v1 = v0 + 127;
        } else {
            v1 = v0 - 127;
        }
    }

    uv1 = (u1 << 24) | (v1 << 8);
}

#define MAX_ROOMS           256
#define MAX_ROOM_VERTICES   0xFFFF
#define MAX_ROOM_QUADS      2048
#define MAX_ROOM_TRIANGLES  128
#define MAX_ROOM_SPRITES    64
#define MAX_ROOM_PORTALS    16
#define MAX_ROOM_SECTORS    (20*20)
#define MAX_FLOORS          (9 * 1024)
#define MAX_BOXES           1024
#define MAX_OVERLAPS        (6 * 1024)
#define MAX_ITEMS           240
#define MAX_NODES           32
#define MAX_TEXTURES        1536 * 2

#define TEX_FLIP_X      1
#define TEX_FLIP_Y      2
#define TEX_ATTR_AKILL  0x0001
#define TEX_ATTR_MIPS   0x8000

#define FACE_TEXTURE    0x07FF

// 3DO face flags
// 1:ccw, 1:opaque, 8:intensity, 11:mipTexIndex, 11:texIndex
#define FACE_MIP_SHIFT  11
#define FACE_OPAQUE     (1 << 30)
#define FACE_CCW        (1 << 31)

// GBA
#define FACE_COLORED    (1 << 14)
#define FACE_TRIANGLE   (1 << 15)

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
    short shortOne, shortTwo;
    long i, j;

    lastEstimateL = lastEstimateR = 0L;
    stepSizeL = stepSizeR = 7L;
    stepIndexL = stepIndexR = 0L;
        
    for(i = j = 0; i < numSamples; i += 2, j++)
    {
        buffer[j] = ADDVIEncode(samples[i + 0], samples[i + 1], channels);
    }
}

#pragma pack(1)
struct LevelPC
{
    struct RoomQuad3DO
    {
        uint16 indices[4];
        uint32 flags;

        void write(FileStream &f) const
        {
            f.write(flags);
            f.write(indices[0]);
            f.write(indices[1]);
            f.write(indices[2]);
            f.write(indices[3]);
        }
    };

    struct MeshQuad3DO
    {
        uint8  indices[4];
        uint32 flags;

        void write(FileStream &f) const
        {
            f.write(flags);
            f.write(indices[3]);
            f.write(indices[2]);
            f.write(indices[1]);
            f.write(indices[0]);
        }
    };

    struct Quad
    {
        uint16 indices[4];
        uint16 flags;

        void write(FileStream &f) const
        {
            f.write(indices[0]);
            f.write(indices[1]);
            f.write(indices[2]);
            f.write(indices[3]);
            f.write(flags);
        }
    };

    struct RoomTriangle3DO
    {
        uint16 indices[3];
        uint16 _unused;
        uint32 flags;

        void write(FileStream &f) const
        {
            f.write(flags);
            f.write(indices[0]);
            f.write(indices[1]);
            f.write(indices[2]);
            f.write(_unused);
        }
    };

    struct MeshTriangle3DO
    {
        uint8  indices[3];
        uint8  _unused;
        uint32 flags;

        void write(FileStream &f) const
        {
            f.write(flags);
            uint8 unused = 0;
            f.write(unused);
            f.write(indices[2]);
            f.write(indices[1]);
            f.write(indices[0]);
        }
    };

    struct Triangle
    {
        uint16 indices[3];
        uint16 flags;

        void write(FileStream &f) const
        {
            f.write(indices[0]);
            f.write(indices[1]);
            f.write(indices[2]);
            f.write(flags);
        }
    };

    struct MeshQuadGBA
    {
        uint16 flags;
        uint8 indices[4];
        
        MeshQuadGBA(const Quad &q)
        {
            flags = q.flags;
            indices[0] = q.indices[0];
            indices[1] = q.indices[1];
            indices[2] = q.indices[2];
            indices[3] = q.indices[3];
        }

        void write(FileStream &f) const
        {
            f.write(flags);
            f.write(indices[0]);
            f.write(indices[1]);
            f.write(indices[2]);
            f.write(indices[3]);
        }
    };

    struct MeshTriangleGBA
    {
        uint16 flags;
        uint8 indices[4];
        
        MeshTriangleGBA(const Triangle &t)
        {
            flags = t.flags | FACE_TRIANGLE;
            indices[0] = t.indices[0];
            indices[1] = t.indices[1];
            indices[2] = t.indices[2];
            indices[3] = 0;
        }

        void write(FileStream &f) const
        {
            f.write(flags);
            f.write(indices[0]);
            f.write(indices[1]);
            f.write(indices[2]);
            f.write(indices[3]);
        }
    };

    struct RoomQuadGBA
    {
        uint16 flags;
        uint16 indices[4];
        
        RoomQuadGBA(const Quad &q)
        {
            flags = q.flags;
            indices[0] = q.indices[0];
            indices[1] = q.indices[1];
            indices[2] = q.indices[2];
            indices[3] = q.indices[3];
        }

        void write(FileStream &f) const
        {
            f.write(flags);
            f.write(indices[0]);
            f.write(indices[1]);
            f.write(indices[2]);
            f.write(indices[3]);
        }
    };

    struct RoomTriangleGBA
    {
        uint16 flags;
        uint16 indices[3];
        
        RoomTriangleGBA(const Triangle &t)
        {
            flags = t.flags | FACE_TRIANGLE;
            indices[0] = t.indices[0];
            indices[1] = t.indices[1];
            indices[2] = t.indices[2];
        }

        void write(FileStream &f) const
        {
            f.write(flags);
            f.write(indices[0]);
            f.write(indices[1]);
            f.write(indices[2]);
        }
    };

    struct Room
    {
        struct Info
        {
            int32 x;
            int32 z;
            int32 yBottom;
            int32 yTop;
            int32 dataSize;
        };

        struct InfoComp
        {
            int16 x;
            int16 z;
            int16 yBottom;
            int16 yTop;

            uint16 quadsCount;
            uint16 trianglesCount;

            uint16 verticesCount;
            uint16 spritesCount;

            uint8 portalsCount;
            uint8 lightsCount;
            uint8 meshesCount;
            uint8 ambient;

            uint8 xSectors;
            uint8 zSectors;
            uint8 alternateRoom;
            uint8 flags;

            uint32 quads;
            uint32 triangles;
            uint32 vertices;
            uint32 sprites;
            uint32 portals;
            uint32 sectors;
            uint32 lights;
            uint32 meshes;

            void write(FileStream &f) const
            {
                f.write(x);
                f.write(z);
                f.write(yBottom);
                f.write(yTop);
                f.write(quadsCount);
                f.write(trianglesCount);
                f.write(verticesCount);
                f.write(spritesCount);
                f.write(portalsCount);
                f.write(lightsCount);
                f.write(meshesCount);
                f.write(ambient);
                f.write(xSectors);
                f.write(zSectors);
                f.write(alternateRoom);
                f.write(flags);
                f.write(quads);
                f.write(triangles);
                f.write(vertices);
                f.write(sprites);
                f.write(portals);
                f.write(sectors);
                f.write(lights);
                f.write(meshes);
            }
        };

        struct Vertex
        {
            vec3s pos;
            int16 lighting;

            bool isEqual(const Vertex *v)
            {
                return pos.x == v->pos.x && pos.y == v->pos.y && pos.z == v->pos.z && lighting == v->lighting;
            }
        };

        struct VertexComp
        {
            int8  x, y, z; 
            uint8 g;

            void write(FileStream &f) const
            {
                f.write(x);
                f.write(y);
                f.write(z);
                f.write(g);
            }
        };

        struct Sprite
        {
            uint16 index;
            uint16 texture;
        };

        struct SpriteComp
        {
            int16 x, y, z;
            uint8 g;
            uint8 index;

            void write(FileStream &f) const
            {
                f.write(x);
                f.write(y);
                f.write(z);
                f.write(g);
                f.write(index);
            }
        };
        /*
        struct MeshComp {
            vec3s center;
            int16 radius;

            int16 vCount;
            int16 nCount;

            int16 rCount;
            int16 tCount;

            vec3s* vertices;
            Quad* quads;
            Triangle* triangles;
        };*/

        struct Portal
        {
            int16 roomIndex;
            vec3s normal;
            vec3s vertices[4];

            void write(FileStream &f) const
            {
                f.write(roomIndex);
                f.write(normal.x);
                f.write(normal.y);
                f.write(normal.z);
                for (int32 i = 0; i < 4; i++)
                {
                    f.write(vertices[i].x);
                    f.write(vertices[i].y);
                    f.write(vertices[i].z);
                }
            }
        };

        struct PortalComp
        {
            uint32 roomIndex;
            uint32 normalMask;
            vec3i vertices[4];

            void write(FileStream &f) const
            {
                f.write(roomIndex);
                f.write(normalMask);
                for (int32 i = 0; i < 4; i++)
                {
                    f.write(vertices[i].x);
                    f.write(vertices[i].y);
                    f.write(vertices[i].z);
                }
            }
        };

        struct Sector
        {
            uint16 floorIndex;
            uint16 boxIndex;
            uint8 roomBelow;
            int8 floor;
            uint8 roomAbove;
            int8 ceiling;

            void write(FileStream &f) const
            {
                f.write(floorIndex);
                f.write(boxIndex);
                f.write(roomBelow);
                f.write(floor);
                f.write(roomAbove);
                f.write(ceiling);
            }
        };

        struct Light
        {
            vec3i pos;
            uint16 intensity;
            int32 radius;
        };

        struct LightComp
        {
            vec3s pos;
            uint8 radius;
            uint8 intensity;

            void write(FileStream &f) const
            {
                f.write(pos.x);
                f.write(pos.y);
                f.write(pos.z);
                f.write(radius);
                f.write(intensity);
            }
        };

        struct Mesh
        {
            vec3i pos;
            int16 angleY;
            uint16 intensity;
            uint16 id;
        };

        struct MeshComp
        {
            vec3s pos;
            uint8 intensity;
            uint8 flags;

            void write(FileStream &f) const
            {
                uint32 xy = (pos.x << 16) | uint16(pos.y);
                uint32 zf = (pos.z << 16) | (intensity << 8) | flags;
                f.write(xy);
                f.write(zf);
            }
        };

        Info info;

        int16 vCount;
        Vertex* vertices;

        int16 qCount;
        Quad* quads;

        int16 tCount;
        Triangle* triangles;

        int16 sCount;
        Sprite* sprites;

        int16 pCount;
        Portal* portals;

        uint16 zSectors;
        uint16 xSectors;
        Sector* sectors;

        uint16 ambient;

        uint16 lCount;
        Light* lights;

        uint16 mCount;
        Mesh* meshes;

        int16 alternateRoom;
        uint16 flags;
    };

    struct FloorData
    {
        uint16 value;

        void write(FileStream &f) const
        {
            f.write(value);
        }
    };

    struct Animation
    {
        uint32 frameOffset;

        uint8 frameRate;
        uint8 frameSize;
        uint16 state;

        uint32 speed;
        uint32 accel;

        uint16 frameBegin;
        uint16 frameEnd;

        uint16 nextAnimIndex;
        uint16 nextFrameIndex;

        uint16 statesCount;
        uint16 statesStart;

        uint16 commandsCount;
        uint16 commandsStart;

        void write(FileStream &f) const
        {
            f.write(frameOffset);
            f.write(frameRate);
            f.write(frameSize);
            f.write(state);
            f.write(speed);
            f.write(accel);
            f.write(frameBegin);
            f.write(frameEnd);
            f.write(nextAnimIndex);
            f.write(nextFrameIndex);
            f.write(statesCount);
            f.write(statesStart);
            f.write(commandsCount);
            f.write(commandsStart);
        }
    };

    struct AnimState
    {
        uint16 state;
        uint16 rangesCount;
        uint16 rangesStart;
    };

    struct AnimStateComp
    {
        uint8  state;
        uint8  rangesCount;
        uint16 rangesStart;

        void write(FileStream &f) const
        {
            f.write(state);
            f.write(rangesCount);
            f.write(rangesStart);
        }
    };

    struct AnimRange
    {
        int16 frameBegin;
        int16 frameEnd;
        int16 nextAnimIndex;
        int16 nextFrameIndex;

        void write(FileStream &f) const
        {
            f.write(frameBegin);
            f.write(frameEnd);
            f.write(nextAnimIndex);
            f.write(nextFrameIndex);
        }
    };

    struct Model
    {
        uint32 type;
        uint16 count;
        uint16 start;
        uint32 nodeIndex;
        uint32 frameIndex;
        uint16 animIndex;
    };

    struct ModelComp
    {
        uint8  type;
        uint8  count;
        uint16 start;
        uint16 nodeIndex;
        uint16 animIndex;

        void write(FileStream &f) const
        {
            f.write(type);
            f.write(count);
            f.write(start);
            f.write(nodeIndex);
            f.write(animIndex);
        }
    };

    struct MinMax
    {
        int16 minX, maxX;
        int16 minY, maxY;
        int16 minZ, maxZ;

        void write(FileStream &f) const
        {
            f.write(minX); f.write(maxX);
            f.write(minY); f.write(maxY);
            f.write(minZ); f.write(maxZ);
        }
    };

    struct Sphere16
    {
        int16 x, y, z, radius;

        void write(FileStream &f) const
        {
            uint32 xy = (x << 16) | uint16(y);
            uint32 zr = (z << 16) | uint16(radius);
            f.write(xy);
            f.write(zr);
        }
    };
   
    struct StaticMesh
    {
        uint32 id;
        uint16 meshIndex;
        MinMax vbox;
        MinMax cbox;
        uint16 flags;
    };

    struct StaticMeshComp
    {
        uint16 id;
        uint16 meshIndex;
        uint32 flags;
        //Sphere16 vs;
        MinMax vbox;
        MinMax cbox;

        void write(FileStream &f) const
        {
            Sphere16 vs;
            vs.x = (vbox.maxX + vbox.minX) >> 1;
            vs.y = (vbox.maxY + vbox.minY) >> 1;
            vs.z = (vbox.maxZ + vbox.minZ) >> 1;

            int32 dx = (vbox.maxX - vbox.minX) >> 1;
            int32 dy = (vbox.maxY - vbox.minY) >> 1;
            int32 dz = (vbox.maxZ - vbox.minZ) >> 1;

            vs.radius = int32(sqrtf(float(dx * dx + dy * dy + dz * dz)));

            f.write(id);
            f.write(meshIndex);
            f.write(flags);
            vs.write(f);
            vbox.write(f);
            cbox.write(f);
        }
    };

    struct ObjectTexture
    {
        uint16 attribute;
        uint16 tile;
        union {
            struct { uint8 xh0, x0, yh0, y0; };
            uint32 uv0;
        };
        
        union {
            struct { uint8 xh1, x1, yh1, y1; };
            uint32 uv1;
        };

        union {
            struct { uint8 xh2, x2, yh2, y2; };
            uint32 uv2;
        };

        union {
            struct { uint8 xh3, x3, yh3, y3; };
            uint32 isQuad;
            uint32 uv3;
        };
    };

    struct ObjectTextureComp
    {
        uint16 attribute;
        uint16 tile;
        uint32 uv0;
        uint32 uv1;
        uint32 uv2;
        uint32 uv3;

        void write(FileStream &f) const
        {
            f.write(attribute);
            f.write(tile);
            f.write(uv0);
            f.write(uv1);
            f.write(uv2);
            f.write(uv3);
/*
            union TexCoord {
                struct { uint16 v, u; };
                uint32 uv;
            } t;


            t.uv = uv0;
            f.write(t.v);
            f.write(t.u);

            t.uv = uv1;
            f.write(t.v);
            f.write(t.u);

            t.uv = uv2;
            f.write(t.v);
            f.write(t.u);

            t.uv = uv3;
            f.write(t.v);
            f.write(t.u);
*/
        }
    };

    struct SpriteTexture
    {
        uint16 tile;
        uint8 u, v;
        uint16 w, h;
        int16 l, t, r, b;

        void write(FileStream &f) const
        {
            f.write(tile);
            f.write(u);
            f.write(v);
            f.write(w);
            f.write(h);
            f.write(l);
            f.write(t);
            f.write(r);
            f.write(b);
        }
    };

    struct SpriteTextureComp
    {
        uint16 tile;
        uint8 u, v;
        uint8 w, h;
        int16 l, t, r, b;

        void write(FileStream &f) const
        {
            f.write(tile);
            f.write(u);
            f.write(v);
            f.write(w);
            f.write(h);
            f.write(l);
            f.write(t);
            f.write(r);
            f.write(b);
        }
    };

    struct SpriteTexture3DO
    {
        uint32 texture;
        int32 l, t, r, b;

        void write(FileStream &f) const
        {
            f.write(texture);
            f.write(l);
            f.write(t);
            f.write(r);
            f.write(b);
        }
    };

    struct SpriteSequence
    {
        uint16 type;
        uint16 unused;
        int16 count;
        int16 start;

        void write(FileStream &f) const
        {
            f.write(type);
            f.write(unused);
            f.write(count);
            f.write(start);
        }
    };

    struct Camera
    {
        vec3i pos;
        int16 roomIndex;
        uint16 flags;

        void write(FileStream &f) const
        {
            f.write(pos.x);
            f.write(pos.y);
            f.write(pos.z);
            f.write(roomIndex);
            f.write(flags);
        }
    };

    struct SoundSource
    {
        vec3i pos;
        uint16 id;
        uint16 flags;

        void write(FileStream &f) const
        {
            f.write(pos.x);
            f.write(pos.y);
            f.write(pos.z);
            f.write(id);
            f.write(flags);
        }
    };

    struct Box
    {
        int32 minZ, maxZ;
        int32 minX, maxX;
        int16 floor;
        int16 overlap;
    };

    struct BoxComp
    {
        uint8 minZ, maxZ;
        uint8 minX, maxX;
        int16 floor;
        int16 overlap;

        void write(FileStream &f) const
        {
            f.write(minZ);
            f.write(maxZ);
            f.write(minX);
            f.write(maxX);
            f.write(floor);
            f.write(overlap);
        }
    };

    struct Zone
    {
        uint16* ground1;
        uint16* ground2;
        uint16* fly;
    };

    struct Item
    {
        uint16 type;
        int16 roomIndex;
        vec3i pos;
        int16 angleY;
        int16 intensity;
        uint16 flags;
    };

    struct ItemComp
    {
        uint8 type;
        uint8 roomIndex;
        vec3s pos;
        int16 intensity;
        uint16 flags;

        void write(FileStream &f) const
        {
            f.write(type);
            f.write(roomIndex);
            f.write(pos.x);
            f.write(pos.y);
            f.write(pos.z);
            f.write(intensity);
            f.write(flags);
        }
    };

    struct CameraFrame
    {
        vec3s target;
        vec3s pos;
        int16 fov;
        int16 roll;

        void write(FileStream &f) const
        {
            f.write(target.x);
            f.write(target.y);
            f.write(target.z);
            f.write(pos.x);
            f.write(pos.y);
            f.write(pos.z);
            f.write(fov);
            f.write(roll);
        }
    };

    struct SoundInfo
    {
        uint16 index;
        uint16 volume;
        uint16 chance;

        union {
            struct {
                uint16 mode:2, count:4, unused:6, camera:1, pitch:1, gain:1, :1;
            };

            uint16 value;
        } flags;

        void write(FileStream &f) const
        {
            f.write(index);
            f.write(volume);
            f.write(chance);
            f.write(flags.value);
        }
    };

    enum NodeFlag
    {
        NODE_FLAG_POP  = (1 << 0),
        NODE_FLAG_PUSH = (1 << 1),
        NODE_FLAG_ROTX = (1 << 2),
        NODE_FLAG_ROTY = (1 << 3),
        NODE_FLAG_ROTZ = (1 << 4),
    };

    struct Node
    {
        uint32 flags;
        vec3i pos;
    };

    struct NodeComp
    {
        vec3s pos;
        uint16 flags;

        void write(FileStream &f)
        {
            f.write(pos.x);
            f.write(pos.y);
            f.write(pos.z);
            f.write(flags);
        }
    };

    int32 tilesCount;
    Tile* tiles;

    int16 roomsCount;
    Room* rooms;

    int32 floorsCount;
    FloorData* floors;

    int32 meshDataSize;
    uint16* meshData;

    int32 meshOffsetsCount;
    uint32* meshOffsets;

    int32 animsCount;
    Animation* anims;

    int32 statesCount;
    AnimState* states;

    int32 rangesCount;
    AnimRange* ranges;

    int32 commandsCount;
    int16* commands;

    int32 nodesDataSize;
    uint32* nodesData;

    int32 frameDataSize;
    uint16* frameData;

    int32 modelsCount;
    Model* models;

    int32 staticMeshesCount;
    StaticMesh* staticMeshes;

    int32 objectTexturesCount;
    ObjectTexture* objectTextures;

    int32 spriteTexturesCount;
    SpriteTexture* spriteTextures;

    int32 spriteSequencesCount;
    SpriteSequence* spriteSequences;

    int32 camerasCount;
    Camera* cameras;

    int32 soundSourcesCount;
    SoundSource* soundSources;

    int32 boxesCount;
    Box* boxes;

    int32 overlapsCount;
    uint16* overlaps;

    Zone zones[2];

    int32 animTexDataSize;
    uint16* animTexData;

    int32 itemsCount;
    Item* items;

    uint8 lightmap[32 * 256];
    Palette palette;

    uint16 cameraFramesCount;
    CameraFrame* cameraFrames;

    uint16 demoDataSize;
    uint8* demoData;

    int16 soundMap[256];
    int32 soundInfoCount;
    SoundInfo* soundInfo;

    int32 soundDataSize;
    uint8* soundData;

    int32 soundOffsetsCount;
    uint32* soundOffsets;

    LevelPC(const char* fileName)
    {
        tiles = NULL;

        FileStream f(fileName, false);
        
        if (!f.isValid()) return;

        uint32 magic;
        f.read(magic);

        if (magic != 0x00000020)
        {
            printf("Unsupported level format\n");
            return;
        }

        f.readArray(tiles, tilesCount);
        f.seek(4);

        f.read(roomsCount);
        rooms = new Room[roomsCount];
        
        for (int32 i = 0; i < roomsCount; i++)
        {
            Room* room = rooms + i;

            f.read(room->info);
            f.readArray(room->vertices, room->vCount);
            f.readArray(room->quads, room->qCount);
            f.readArray(room->triangles, room->tCount);
            f.readArray(room->sprites, room->sCount);
            f.readArray(room->portals, room->pCount);
            f.read(room->zSectors);
            f.read(room->xSectors);
            f.read(room->sectors, room->zSectors * room->xSectors);
            f.read(room->ambient);
            f.readArray(room->lights, room->lCount);
            f.readArray(room->meshes, room->mCount);
            f.read(room->alternateRoom);
            f.read(room->flags); 
        }

        f.readArray(floors, floorsCount);
        
        f.readArray(meshData, meshDataSize);
        f.readArray(meshOffsets, meshOffsetsCount);
        f.readArray(anims, animsCount);
        f.readArray(states, statesCount);
        f.readArray(ranges, rangesCount);
        f.readArray(commands, commandsCount);
        f.readArray(nodesData, nodesDataSize);
        f.readArray(frameData, frameDataSize);
        f.readArray(models, modelsCount);
        f.readArray(staticMeshes, staticMeshesCount);
        f.readArray(objectTextures, objectTexturesCount);
        f.readArray(spriteTextures, spriteTexturesCount);
        f.readArray(spriteSequences, spriteSequencesCount);

        f.readArray(cameras, camerasCount);
        f.readArray(soundSources, soundSourcesCount);
        f.readArray(boxes, boxesCount);
        f.readArray(overlaps, overlapsCount);

        for (int32 i = 0; i < 2; i++)
        {
            f.read(zones[i].ground1, boxesCount);
            f.read(zones[i].ground2, boxesCount);
            f.read(zones[i].fly, boxesCount);      
        }

        f.readArray(animTexData, animTexDataSize);
        f.readArray(items, itemsCount);
        f.read(lightmap);
        f.read(palette);
        f.readArray(cameraFrames, cameraFramesCount);
        f.readArray(demoData, demoDataSize);

        f.read(soundMap);
        f.readArray(soundInfo, soundInfoCount);
        f.readArray(soundData, soundDataSize);
        f.readArray(soundOffsets, soundOffsetsCount);

        markRoomTextures();
    }

    ~LevelPC()
    {
        delete[] tiles;
        
        for (int32 i = 0; i < roomsCount; i++)
        {
            Room* room = rooms + i;
            delete[] room->vertices;
            delete[] room->quads;
            delete[] room->triangles;
            delete[] room->sprites;
            delete[] room->portals;
            delete[] room->sectors;
            delete[] room->lights;
            delete[] room->meshes;
        }

        delete[] rooms;
        delete[] floors;
        delete[] meshData;
        delete[] meshOffsets;
        delete[] anims;
        delete[] states;
        delete[] ranges;
        delete[] commands;
        delete[] nodesData;
        delete[] frameData;
        delete[] models;
        delete[] staticMeshes;
        delete[] objectTextures;
        delete[] spriteTextures;
        delete[] spriteSequences;
        delete[] cameras;
        delete[] soundSources;
        delete[] boxes;
        delete[] overlaps;

        for (int32 i = 0; i < 2; i++)
        {
            delete[] zones[i].ground1;
            delete[] zones[i].ground2;
            delete[] zones[i].fly;
        }

        delete[] animTexData;
        delete[] items;
        delete[] cameraFrames;
        delete[] demoData;
        delete[] soundInfo;
        delete[] soundData;
        delete[] soundOffsets;
    }

    void markRoomTextures()
    {
        for (int32 i = 0; i < roomsCount; i++)
        {
            Room* room = rooms + i;

            for (int32 j = 0; j < room->qCount; j++)
            {
                Quad* q = room->quads + j;
                objectTextures[q->flags & FACE_TEXTURE].attribute |= TEX_ATTR_MIPS;
            }

            for (int32 j = 0; j < room->tCount; j++)
            {
                Triangle* t = room->triangles + j;
                objectTextures[t->flags & FACE_TEXTURE].attribute |= TEX_ATTR_MIPS;
            }
        }
    }

    struct Header
    {
        uint32 magic;

        uint16 tilesCount;
        uint16 roomsCount;
        uint16 modelsCount;
        uint16 meshesCount;
        uint16 staticMeshesCount;
        uint16 spriteSequencesCount;
        uint16 soundSourcesCount;
        uint16 boxesCount;
        uint16 texturesCount;
        uint16 itemsCount;
        uint16 camerasCount;
        uint16 cameraFramesCount;
        uint16 soundOffsetsCount;
        uint16 _reserved;

        uint32 palette;
        uint32 lightmap;
        uint32 tiles;
        uint32 rooms;
        uint32 floors;
        uint32 meshData;
        uint32 meshOffsets;
        uint32 anims;
        uint32 states;
        uint32 ranges;
        uint32 commands;
        uint32 nodes;
        uint32 frameData;
        uint32 models;
        uint32 staticMeshes;
        uint32 objectTextures;
        uint32 spriteTextures;
        uint32 spriteSequences;
        uint32 cameras;
        uint32 soundSources;
        uint32 boxes;
        uint32 overlaps;
        uint32 zones[2][3];
        uint32 animTexData;
        uint32 items;
        uint32 cameraFrames;
        uint32 soundMap;
        uint32 soundInfos;
        uint32 soundData;
        uint32 soundOffsets;

        void write(FileStream &f) const
        {
            f.write(magic);
            f.write(tilesCount);
            f.write(roomsCount);
            f.write(modelsCount);
            f.write(meshesCount);
            f.write(staticMeshesCount);
            f.write(spriteSequencesCount);
            f.write(soundSourcesCount);
            f.write(boxesCount);
            f.write(texturesCount);
            f.write(itemsCount);
            f.write(camerasCount);
            f.write(cameraFramesCount);
            f.write(soundOffsetsCount);
            f.write(_reserved);

            f.write(palette);
            f.write(lightmap);
            f.write(tiles);
            f.write(rooms);
            f.write(floors);
            f.write(meshData);
            f.write(meshOffsets);
            f.write(anims);
            f.write(states);
            f.write(ranges);
            f.write(commands);
            f.write(nodes);
            f.write(frameData);
            f.write(models);
            f.write(staticMeshes);
            f.write(objectTextures);
            f.write(spriteTextures);
            f.write(spriteSequences);
            f.write(cameras);
            f.write(soundSources);
            f.write(boxes);
            f.write(overlaps);

            for (int32 i = 0; i < 2; i++)
            {
                for (int32 j = 0; j < 3; j++)
                {
                    f.write(zones[i][j]);
                }
            }

            f.write(animTexData);
            f.write(items);
            f.write(cameraFrames);
            f.write(soundMap);
            f.write(soundInfos);
            f.write(soundData);
            f.write(soundOffsets);
        }
    };

    Room::VertexComp roomVertices[MAX_ROOM_VERTICES];
    int32 roomVerticesCount;

    int32 addRoomVertex(int32 yOffset, const Room::Vertex &v, bool ignoreG = false)
    {
        Room::VertexComp comp;
        int32 px = v.pos.x >> 10;
        int32 py = (v.pos.y - yOffset) >> 8;
        int32 pz = v.pos.z >> 10;

        ASSERT(py >= 0);
        ASSERT(px < 32);
        ASSERT(py < 64);
        ASSERT(pz < 32);

        comp.x = px;
        comp.y = py;
        comp.z = pz;
        comp.g = ignoreG ? 0 : (v.lighting >> 5);

        for (int32 i = 0; i < roomVerticesCount; i++)
        {
            if (memcmp(roomVertices + i, &comp, sizeof(comp)) == 0)
            {
                return i;
            }
        }

        roomVertices[roomVerticesCount] = comp;

        return roomVerticesCount++;
    }

    void fixObjectTexture(ObjectTextureComp &tex, int32 idx)
    {
        fixTexCoord(tex.uv0, tex.uv1);
        fixTexCoord(tex.uv0, tex.uv3);
        fixTexCoord(tex.uv1, tex.uv2);
        /*
        if (tex.attribute == 0)
            return;

        int32 u0 = tex.uv0 >> 24;
        int32 u1 = tex.uv1 >> 24;
        int32 u2 = tex.uv2 >> 24;
        int32 u3 = tex.uv3 >> 24;

        int32 v0 = (tex.uv0 >> 8) & 0xFF;
        int32 v1 = (tex.uv1 >> 8) & 0xFF;
        int32 v2 = (tex.uv2 >> 8) & 0xFF;
        int32 v3 = (tex.uv3 >> 8) & 0xFF;

        int32 minU = MIN(u0, MIN(u1, u2));
        int32 maxU = MAX(u0, MAX(u1, u2));
        int32 minV = MIN(v0, MIN(v1, v2));
        int32 maxV = MAX(v0, MAX(v1, v2));

        if (tex.uv3 != 0)
        {
            minU = MIN(minU, u3);
            maxU = MAX(maxU, u3);
            minV = MIN(minV, v3);
            maxV = MAX(maxV, v3);
        }

        uint8* tile = tiles[tex.tile].indices;

        int32 w = (maxU - minU) + 1;
        int32 h = (maxV - minV) + 1;

        uint8* data = new uint8[w * h * 4];
        uint8* ptr = data;

        bool flag = false;

        palette.colors[0] = 63;
        palette.colors[1] = 0;
        palette.colors[2] = 63;

        for (int32 y = minV; y <= maxV; y++)
        {
            for (int32 x = minU; x <= maxU; x++)
            {
                int32 index = tile[y * 256 + x];

                if (index == 0) {
                    flag = true;
                }

                *ptr++ = palette.colors[index * 3 + 2] << 2;
                *ptr++ = palette.colors[index * 3 + 1] << 2;
                *ptr++ = palette.colors[index * 3 + 0] << 2;
            }
        }

        if (flag) {
            char buf[256];
            sprintf(buf, "test/tex_%d.bmp", idx);
            saveBitmap(buf, data, w, h);
        }

        delete[] data;


        if (!flag) {
            tex.attribute = 0;
        }
        */
    }

    void fixHeadMask()
    {
        #define SET_ROT(joint, mask) (((Node*)nodesData)[models[i].nodeIndex / 4 + joint]).flags |= mask;

        for (int32 i = 0; i < modelsCount; i++)
        {
            switch (models[i].type)
            {
                case ITEM_WOLF            : SET_ROT(2, NODE_FLAG_ROTY); break;
                case ITEM_BEAR            : SET_ROT(13, NODE_FLAG_ROTY); break;
                //case ITEM_BAT             : break;
                case ITEM_CROCODILE_LAND  : SET_ROT(7, NODE_FLAG_ROTY); break;
                case ITEM_CROCODILE_WATER : SET_ROT(7, NODE_FLAG_ROTY); break;
                case ITEM_LION_MALE       : SET_ROT(19, NODE_FLAG_ROTY); break;
                case ITEM_LION_FEMALE     : SET_ROT(19, NODE_FLAG_ROTY); break;
                case ITEM_PUMA            : SET_ROT(19, NODE_FLAG_ROTY); break;
                case ITEM_GORILLA         : SET_ROT(13, NODE_FLAG_ROTY); break;
                case ITEM_RAT_LAND        : SET_ROT(1, NODE_FLAG_ROTY); break;
                case ITEM_RAT_WATER       : SET_ROT(1, NODE_FLAG_ROTY); break;
                case ITEM_REX             : SET_ROT(10, NODE_FLAG_ROTY); SET_ROT(11, NODE_FLAG_ROTY); break;
                case ITEM_RAPTOR          : SET_ROT(21, NODE_FLAG_ROTY); break;
                case ITEM_MUTANT_1        : SET_ROT(0, NODE_FLAG_ROTY); SET_ROT(2, NODE_FLAG_ROTY); break;
                case ITEM_MUTANT_2        : SET_ROT(0, NODE_FLAG_ROTY); SET_ROT(2, NODE_FLAG_ROTY); break;
                case ITEM_MUTANT_3        : SET_ROT(0, NODE_FLAG_ROTY); SET_ROT(2, NODE_FLAG_ROTY); break;
                case ITEM_CENTAUR         : SET_ROT(10, NODE_FLAG_ROTX | NODE_FLAG_ROTY); break;
                case ITEM_MUMMY           : SET_ROT(2, NODE_FLAG_ROTY); break;
                case ITEM_LARSON          : SET_ROT(6, NODE_FLAG_ROTY); break;
                case ITEM_PIERRE          : SET_ROT(6, NODE_FLAG_ROTY); break;
                case ITEM_SKATER          : SET_ROT(0, NODE_FLAG_ROTY); break;
                case ITEM_COWBOY          : SET_ROT(0, NODE_FLAG_ROTY); break;
                case ITEM_MR_T            : SET_ROT(0, NODE_FLAG_ROTY); break;
                case ITEM_NATLA           : SET_ROT(2, NODE_FLAG_ROTX | NODE_FLAG_ROTZ); break;
                case ITEM_ADAM            : SET_ROT(1, NODE_FLAG_ROTY); break;
                default                   : break;
            }
        }

        #undef SET_ROT
    }

    int32 getModelIndex(int32 type)
    {
        for (int32 i = 0; i < modelsCount; i++)
        {
            if (models[i].type == type) {
                return i;
            }
        }
        return -1;
    }

    template <typename T>
    T* addElements(T* &a, int32 &count, int32 size)
    {
        T* ptr = new T[count + size];
        memcpy(ptr, a, sizeof(a[0]) * count);
        delete[] a;
        a = ptr;
        count += size;
        return &a[count - size];
    }

    int32 getMeshTexture(uint16* meshPtr)
    {
        meshPtr += 3 + 1 + 1; // skip center, radius, flags
        int16 vCount = *(int16*)meshPtr;
        meshPtr += 1; // skip vCount
        meshPtr += vCount * 3; // skip vertices
        int16 nCount = *(int16*)meshPtr;
        meshPtr += 1; // skip nCount
        if (nCount > 0) {
            meshPtr += nCount * 3; // skip normals
        } else {
            meshPtr -= nCount; // skip intensity
        }
        int16 rCount = *(int16*)meshPtr;
        meshPtr += 1; // skip rCount
        if (rCount > 0) {
            meshPtr += 4; // skip indices
            return (*(uint16*)meshPtr) & 0x07FF;
        }
        int16 tCount = *(int16*)meshPtr;
        meshPtr += 1; // skip tCount
        if (tCount > 0)
        {
            meshPtr += 3; // skip indices
            return (*(uint16*)meshPtr) & 0x07FF;
        }
        // no textured quads or triangles
        ASSERT(false);
        return -1;
    }

    int32 getMaxTexture(int32 tile, int32 x, int32 y, int32 &minU, int32 &minV, int32 &maxU, int32 &maxV)
    {
        int32 index = -1;
        int32 maxW = 0;
        int32 maxH = 0;

        for (int32 i = 0; i < objectTexturesCount; i++)
        {
            ObjectTexture *tex = objectTextures + i;

            //if (!tex->isQuad)
            //    continue;

            if (tex->tile != tile)
                continue;

            int32 minX = MIN(MIN(tex->x0, tex->x1), tex->x2);
            int32 minY = MIN(MIN(tex->y0, tex->y1), tex->y2);
            int32 maxX = MAX(MAX(tex->x0, tex->x1), tex->x2);
            int32 maxY = MAX(MAX(tex->y0, tex->y1), tex->y2);

            if (tex->isQuad)
            {
                minX = MIN(minX, tex->x3);
                minY = MIN(minY, tex->y3);
                maxX = MAX(maxX, tex->x3);
                maxY = MAX(maxY, tex->y3);
            }

            if (x >= minX && x <= maxX && y >= minY && y <= maxY)
            {
                int32 w = maxX - minX;
                int32 h = maxY - minY;

                if (w >= maxW && h >= maxH)
                {
                    index = i;
                    maxW = w;
                    maxH = h;

                    minU = minX;
                    minV = minY;
                    maxU = maxX;
                    maxV = maxY;
                }
            }
        }

        ASSERT(index >= 0);

        return index;
    }

    void generateLODs()
    {
        struct Quad {
            uint16 indices[4];
            uint16 flags;
        };

        struct Mesh {
            vec3s center;
            int16 radius;
            uint16 flags;
            int16 vCount;
            vec3s vertices[4];
            int16 nCount;
            int16 intensity[4];
            int16 rCount;
            Quad rFaces[2];
            int16 tCount;
            int16 crCount;
            int16 ctCount;
        };

        struct AnimFrame {
            MinMax box;
            vec3s pos;
            uint16 angles[2];
        };

        AnimFrame meshPlaneFrame;
        meshPlaneFrame.box.minX = -512;
        meshPlaneFrame.box.maxX = 512;
        meshPlaneFrame.box.minY = -512;
        meshPlaneFrame.box.maxY = -512;
        meshPlaneFrame.box.minZ = -512;
        meshPlaneFrame.box.maxZ = 512;

        meshPlaneFrame.pos = vec3s(0, -512, 0);
        meshPlaneFrame.angles[0] = meshPlaneFrame.angles[1] = 0;

        Mesh meshPlane;
        meshPlane.center = vec3s(-512, 0, 512);
        meshPlane.radius = 727;
        meshPlane.flags = 1;
        meshPlane.vCount = 4;
        meshPlane.vertices[0] = vec3s(-512, 0, -512);
        meshPlane.vertices[1] = vec3s( 512, 0, -512);
        meshPlane.vertices[2] = vec3s( 512, 0,  512);
        meshPlane.vertices[3] = vec3s(-512, 0,  512);
        meshPlane.nCount = -4;
        meshPlane.intensity[0] = 3800;
        meshPlane.intensity[1] = 3800;
        meshPlane.intensity[2] = 3800;
        meshPlane.intensity[3] = 3800;
        meshPlane.rCount = 2;
        meshPlane.rFaces[0].indices[0] = 3;
        meshPlane.rFaces[0].indices[1] = 2;
        meshPlane.rFaces[0].indices[2] = 1;
        meshPlane.rFaces[0].indices[3] = 0;
        meshPlane.rFaces[1].indices[0] = 0;
        meshPlane.rFaces[1].indices[1] = 1;
        meshPlane.rFaces[1].indices[2] = 2;
        meshPlane.rFaces[1].indices[3] = 3;
        meshPlane.tCount = 0;
        meshPlane.crCount = 0;
        meshPlane.ctCount = 0;

        // trap floor lod
        int32 index = getModelIndex(ITEM_TRAP_FLOOR);
        
        if (index > -1)
        {
            Model* model = addElements(models, modelsCount, 1);
            *model = models[index];
            model->type = ITEM_TRAP_FLOOR_LOD;

            int32 texture = getMeshTexture((uint16*)((uint8*)meshData + meshOffsets[model->start]));
            ObjectTexture* objTex = objectTextures + texture;

            int32 minU, minV, maxU, maxV;

            texture = getMaxTexture(objTex->tile, (objTex->x0 + objTex->x1 + objTex->x2) / 3, (objTex->y0 + objTex->y1 + objTex->y2) / 3, minU, minV, maxU, maxV);

            objTex = addElements(objectTextures, objectTexturesCount, 1);
            *objTex = objectTextures[texture];
            objTex->x0 = minU;
            objTex->y0 = minV;
            objTex->x1 = maxU;
            objTex->y1 = minV;
            objTex->x2 = maxU;
            objTex->y2 = maxV;
            objTex->x3 = minU;
            objTex->y3 = maxV;

            meshPlane.rFaces[0].flags = objectTexturesCount - 1;
            meshPlane.rFaces[1].flags = objectTexturesCount - 1;

            uint32 *meshOffset = addElements(meshOffsets, meshOffsetsCount, 1);

            uint16* mesh = addElements(meshData, meshDataSize, sizeof(meshPlane) / sizeof(uint16));
            memcpy(mesh, &meshPlane, sizeof(meshPlane));

            *meshOffset = (mesh - meshData) * sizeof(uint16);

            uint16* frame = addElements(frameData, frameDataSize, sizeof(meshPlaneFrame) / sizeof(uint16));
            memcpy(frame, &meshPlaneFrame, sizeof(meshPlaneFrame));

            Animation* anim = addElements(anims, animsCount, 1);
            memset(anim, 0, sizeof(anim[0]));
            anim->frameRate = 1;
            anim->frameOffset = (frame - frameData) << 1;

            Node* node = (Node*)addElements(nodesData, nodesDataSize, sizeof(Node) / sizeof(uint32));
            node->flags = 0;
            node->pos.x = 0;
            node->pos.y = 0;
            node->pos.z = 0;

            model->count = 1;
            model->start = meshOffsetsCount - 1;
            model->animIndex = animsCount - 1;
            model->nodeIndex = (uint32*)node - nodesData;
        }
    }

    void hideRoom(int32 roomIndex)
    {
        Room &room = rooms[roomIndex];
        room.vCount = 0;
        room.qCount = 0;
        room.tCount = 0;
        room.sCount = 0;
        room.pCount = 0;
        room.lCount = 0;
        room.mCount = 0;
        room.zSectors = 0;
        room.xSectors = 0;
        room.alternateRoom = -1;

        for (int32 i = 0; i < roomsCount; i++)
        {
            Room &room = rooms[i];

            int32 j = room.pCount - 1;
            while (j >= 0)
            {
                if (room.portals[j].roomIndex == roomIndex)
                {
                    room.pCount--;
                    room.portals[j] = room.portals[room.pCount];
                }
                j--;
            }
        }
    }

    void cutData(const char* name)
    {
        if (strcmp(name, "GYM") == 0)
        {
            hideRoom(0);
            hideRoom(1);
            hideRoom(2);
            hideRoom(3);
            hideRoom(4);
            hideRoom(5);
            hideRoom(6);
            hideRoom(15);
            hideRoom(16);
            hideRoom(17);
            hideRoom(18);

            // disable alpha-test
            objectTextures[93].attribute =
            objectTextures[167].attribute =
            objectTextures[175].attribute =
            objectTextures[190].attribute =
            objectTextures[191].attribute =
            objectTextures[211].attribute =
            objectTextures[220].attribute =
            objectTextures[221].attribute =
            objectTextures[580].attribute =
            objectTextures[581].attribute = 0;
        }

        if (strcmp(name, "LEVEL1") == 0)
        {
            objectTextures[271].attribute =
            objectTextures[272].attribute =
            objectTextures[331].attribute =
            objectTextures[333].attribute =
            objectTextures[334].attribute =
            objectTextures[335].attribute =
            objectTextures[517].attribute =
            objectTextures[518].attribute =
            objectTextures[569].attribute =
            objectTextures[571].attribute =
            objectTextures[685].attribute =
            objectTextures[686].attribute = 0;
        }

        if (strcmp(name, "LEVEL2") == 0)
        {
            objectTextures[247].attribute =
            objectTextures[248].attribute =
            objectTextures[307].attribute =
            objectTextures[309].attribute =
            objectTextures[310].attribute =
            objectTextures[311].attribute =
            objectTextures[547].attribute =
            objectTextures[661].attribute =
            objectTextures[662].attribute =
            objectTextures[688].attribute =
            objectTextures[905].attribute =
            objectTextures[906].attribute =
            objectTextures[923].attribute = 0;
        }

        // TODO remove unused textures & models
    }

    void convertGBA(const char* fileName)
    {
        FileStream f(fileName, true);

        if (!f.isValid()) return;

        Header header;
        f.seek(sizeof(Header)); // will be rewritten at the end

        header.magic = 0x20414247;
        header.tilesCount = tilesCount;
        header.roomsCount = roomsCount;
        header.modelsCount = modelsCount;
        header.meshesCount = meshOffsetsCount;
        header.staticMeshesCount = staticMeshesCount;
        header.spriteSequencesCount = spriteSequencesCount;
        header.soundSourcesCount = soundSourcesCount;
        header.boxesCount = boxesCount;
        header.texturesCount = objectTexturesCount;
        header.itemsCount = itemsCount;
        header.camerasCount = camerasCount;
        header.cameraFramesCount = cameraFramesCount;
        header.soundOffsetsCount = soundOffsetsCount;
        header._reserved = 0;

        header.palette = f.align4();

        { // convert palette to 15-bit and fix some color gradients
            uint16 pal[256];

            for (int32 i = 0; i < 256; i++)
            {
                uint8 r = palette.colors[i * 3 + 0];
                uint8 g = palette.colors[i * 3 + 1];
                uint8 b = palette.colors[i * 3 + 2];

                pal[i] = (r >> 1) | ((g >> 1) << 5) | ((b >> 1) << 10);
            }

            pal[0] = 0;
            //fixLightmap(lightmap, pal, 6); // boots
            //fixLightmap(lightmap, pal, 14); // skin

            f.write(pal, 256);
        }

        for (int32 i = 0; i < 32; i++) {
            lightmap[i * 256] = 0;
        }

        fixHeadMask();

        header.lightmap = f.align4();
        f.write(lightmap, 32 * 256);

        header.tiles = f.align4();
        f.write((uint8*)tiles, tilesCount * 256 * 256);

        header.rooms = f.align4();
        {
            f.seek(sizeof(Room::InfoComp) * roomsCount);

            Room::InfoComp infoComp[255];

            for (int32 i = 0; i < roomsCount; i++)
            {
                const LevelPC::Room* room = rooms + i;

                Room::InfoComp &info = infoComp[i];

                ASSERT(room->info.x % 256 == 0);
                ASSERT(room->info.z % 256 == 0);
                ASSERT(room->info.yBottom >= -32768 && room->info.yBottom <= 32767);
                ASSERT(room->info.yTop >= -32768 && room->info.yTop <= 32767);
                info.x = room->info.x / 256;
                info.z = room->info.z / 256;
                info.yBottom = -32768;
                info.yTop = 32767;

                for (int32 j = 0; j < room->vCount; j++)
                {
                    Room::Vertex &v = room->vertices[j];
                    if (v.pos.y < info.yTop) {
                        info.yTop = v.pos.y;
                    }
                    if (v.pos.y > info.yBottom) {
                        info.yBottom = v.pos.y;
                    }
                }

                info.spritesCount = room->sCount;
                info.quadsCount = room->qCount;
                info.trianglesCount = room->tCount;
                info.portalsCount = uint8(room->pCount);
                info.lightsCount = uint8(room->lCount);
                info.meshesCount = uint8(room->mCount);
                info.ambient = room->ambient >> 5;
                info.xSectors = uint8(room->xSectors);
                info.zSectors = uint8(room->zSectors);
                info.alternateRoom = uint8(room->alternateRoom);

                info.flags = 0;
                if (room->flags & 1) info.flags |= 1;
                if (room->flags & 256) info.flags |= 2;

                ASSERT((room->flags & ~257) == 0);
                ASSERT(info.portalsCount == room->pCount);
                ASSERT(info.lightsCount == room->lCount);
                ASSERT(info.meshesCount == room->mCount);
                ASSERT(info.xSectors == room->xSectors);
                ASSERT(info.zSectors == room->zSectors);

                roomVerticesCount = 0;

                info.quads = f.align4();
                for (int32 i = 0; i < room->qCount; i++)
                {
                    Quad q = room->quads[i];
                    q.indices[0] = addRoomVertex(info.yTop, room->vertices[q.indices[0]]);
                    q.indices[1] = addRoomVertex(info.yTop, room->vertices[q.indices[1]]);
                    q.indices[2] = addRoomVertex(info.yTop, room->vertices[q.indices[2]]);
                    q.indices[3] = addRoomVertex(info.yTop, room->vertices[q.indices[3]]);

                    RoomQuadGBA comp(q);
                    comp.write(f);
                }

                info.triangles = f.align4();
                for (int32 i = 0; i < room->tCount; i++)
                {
                    Triangle t = room->triangles[i];
                    t.indices[0] = addRoomVertex(info.yTop, room->vertices[t.indices[0]]);
                    t.indices[1] = addRoomVertex(info.yTop, room->vertices[t.indices[1]]);
                    t.indices[2] = addRoomVertex(info.yTop, room->vertices[t.indices[2]]);
                    
                    RoomTriangleGBA comp(t);
                    comp.write(f);
                }

                info.vertices = f.align4();
                info.verticesCount = roomVerticesCount;
                for (int32 i = 0; i < roomVerticesCount; i++)
                {
                    roomVertices[i].write(f);
                }

                info.sprites = f.align4();
                for (int32 i = 0; i < room->sCount; i++)
                {
                    const Room::Sprite* sprite = room->sprites + i;
                    const Room::Vertex* v = room->vertices + sprite->index;

                    Room::SpriteComp comp;
                    comp.x = v->pos.x;
                    comp.y = v->pos.y;
                    comp.z = v->pos.z;
                    comp.g = v->lighting >> 5;
                    comp.index = uint8(sprite->texture);

                    ASSERT(sprite->texture <= 255);

                    comp.write(f);
                }

                info.portals = f.align4();
                f.writeObj(room->portals, room->pCount);

                info.sectors = f.align4();
                f.writeObj(room->sectors, room->zSectors * room->xSectors);
            
                info.lights = f.align4();
                for (int32 i = 0; i < room->lCount; i++)
                {
                    const Room::Light* light = room->lights + i;

                    Room::LightComp comp;
                    comp.pos.x = light->pos.x - room->info.x;
                    comp.pos.y = light->pos.y;
                    comp.pos.z = light->pos.z - room->info.z;
                    comp.radius = light->radius >> 8;
                    comp.intensity = light->intensity >> 5;

                    comp.write(f);
                }

                info.meshes = f.align4();
                for (int32 i = 0; i < room->mCount; i++)
                {
                    const Room::Mesh* mesh = room->meshes + i;

                    Room::MeshComp comp;
                    comp.pos.x = mesh->pos.x - room->info.x;
                    comp.pos.y = mesh->pos.y;
                    comp.pos.z = mesh->pos.z - room->info.z;
                    comp.intensity = mesh->intensity >> 5;
                    comp.flags = ((mesh->angleY / 0x4000 + 2) << 6) | mesh->id;

                    ASSERT(mesh->id <= 63);
                    ASSERT(mesh->angleY % 0x4000 == 0);
                    ASSERT(mesh->angleY / 0x4000 + 2 >= 0);

                    comp.write(f);
                }
            }

            int32 pos = f.getPos();
            f.setPos(header.rooms);
            f.writeObj(infoComp, roomsCount);
            f.setPos(pos);
        }

        header.floors = f.align4();
        f.writeObj(floors, floorsCount);

        header.meshData = f.align4();

        int32 mOffsets[2048];
        for (int32 i = 0; i < 2048; i++) {
            mOffsets[i] = -1;
        }

        for (int32 i = 0; i < meshOffsetsCount; i++)
        {
            if (mOffsets[i] != -1)
                continue;

            mOffsets[i] = f.align4() - header.meshData;

            const uint8* ptr = (uint8*)meshData + meshOffsets[i];

            vec3s center = *(vec3s*)ptr; ptr += sizeof(center);
            int16 radius = *(int16*)ptr; ptr += sizeof(radius);
            uint16 flags = *(uint16*)ptr; ptr += sizeof(flags);

            int16 vCount = *(int16*)ptr; ptr += 2;
            const vec3s* vertices = (vec3s*)ptr;
            ptr += vCount * sizeof(vec3s);

            const uint16* vIntensity = NULL;
            const vec3s* vNormal = NULL;

            int16 nCount = *(int16*)ptr; ptr += 2;
            //const int16* normals = (int16*)ptr;
            if (nCount > 0) { // normals
                vNormal = (vec3s*)ptr;
                ptr += nCount * 3 * sizeof(int16);
            } else { // intensity
                vIntensity = (uint16*)ptr;
                ptr += vCount * sizeof(uint16);
            }

            int16     rCount = *(int16*)ptr; ptr += 2;
            Quad*     rFaces = (Quad*)ptr; ptr += rCount * sizeof(Quad);

            int16     tCount = *(int16*)ptr; ptr += 2;
            Triangle* tFaces = (Triangle*)ptr; ptr += tCount * sizeof(Triangle);

            int16     crCount = *(int16*)ptr; ptr += 2;
            Quad*     crFaces = (Quad*)ptr; ptr += crCount * sizeof(Quad);

            int16     ctCount = *(int16*)ptr; ptr += 2;
            Triangle* ctFaces = (Triangle*)ptr; ptr += ctCount * sizeof(Triangle);

            uint16 intensity = 0;

            if (vIntensity)
            {
                uint32 sum = 0;
                for (int32 i = 0; i < vCount; i++)
                {
                    sum += vIntensity[i];
                }
                intensity = sum / vCount;
            }

            f.write(center.x);
            f.write(center.y);
            f.write(center.z);
            f.write(radius);
            f.write(intensity);
            f.write(vCount);
            f.write(int16(rCount + crCount));
            f.write(int16(tCount + ctCount));
            f.write(int16(0));
            f.write(int16(0));

            for (int32 j = 0; j < vCount; j++)
            {
                struct MeshVertexGBA {
                    int16 x, y, z;
                } v;

                v.x = vertices[j].x;
                v.y = vertices[j].y;
                v.z = vertices[j].z;

                f.write(v.x);
                f.write(v.y);
                f.write(v.z);
            }

            for (int32 j = 0; j < rCount; j++)
            {
                MeshQuadGBA comp(rFaces[j]);
                comp.write(f);
            }

            for (int32 j = 0; j < crCount; j++)
            {
                MeshQuadGBA comp(crFaces[j]);
                comp.flags |= FACE_COLORED;
                comp.write(f);
            }

            for (int32 j = 0; j < tCount; j++)
            {
                MeshTriangleGBA comp(tFaces[j]);
                comp.write(f);
            }

            for (int32 j = 0; j < ctCount; j++)
            {
                MeshTriangleGBA comp(ctFaces[j]);
                comp.flags |= FACE_COLORED;
                comp.write(f);
            }

            for (int32 j = i + 1; j < meshOffsetsCount; j++)
            {
                if (meshOffsets[i] == meshOffsets[j])
                {
                    mOffsets[j] = mOffsets[i];
                }
            }
        }

        header.meshOffsets = f.align4();
        f.write(mOffsets, meshOffsetsCount);

        header.anims = f.align4();
        f.writeObj(anims, animsCount);

        header.states = f.align4();
        for (int32 i = 0; i < statesCount; i++)
        {
            const LevelPC::AnimState* state = states + i;

            LevelPC::AnimStateComp comp;
            comp.state = uint8(state->state);
            comp.rangesCount = uint8(state->rangesCount);
            comp.rangesStart = state->rangesStart;

            comp.write(f);
        }

        header.ranges = f.align4();
        f.writeObj(ranges, rangesCount);

        header.commands = f.align4();
        f.write(commands, commandsCount);

        header.nodes = f.align4();
        for (int32 i = 0; i < nodesDataSize / 4; i++)
        {
            const Node* node = (Node*)(nodesData + i * 4);

            ASSERT(node->pos.x > -32768);
            ASSERT(node->pos.x <  32767);
            ASSERT(node->pos.y > -32768);
            ASSERT(node->pos.y <  32767);
            ASSERT(node->pos.z > -32768);
            ASSERT(node->pos.z <  32767);
            ASSERT(node->flags < 0xFFFF);

            LevelPC::NodeComp comp;
            comp.flags = uint16(node->flags);
            comp.pos.x = int16(node->pos.x);
            comp.pos.y = int16(node->pos.y);
            comp.pos.z = int16(node->pos.z);

            comp.write(f);
        }
        //f.write(nodesData, nodesDataSize);

        header.frameData = f.align4();
        f.write(frameData, frameDataSize);

        header.models = f.align4();
        for (int32 i = 0; i < modelsCount; i++)
        {
            const LevelPC::Model* model = models + i;

            LevelPC::ModelComp comp;
            comp.type = uint8(model->type);
            comp.count = uint8(model->count);
            comp.start = model->start;
            comp.nodeIndex = model->nodeIndex / 4;
            comp.animIndex = model->animIndex;

            comp.write(f);
        }

        header.staticMeshes = f.align4();
        for (int32 i = 0; i < staticMeshesCount; i++)
        {
            const LevelPC::StaticMesh* staticMesh = staticMeshes + i;

            LevelPC::StaticMeshComp comp;
            comp.id = staticMesh->id;
            comp.meshIndex = staticMesh->meshIndex;
            comp.flags = staticMesh->flags;
            comp.vbox = staticMesh->vbox;
            comp.cbox = staticMesh->cbox;

            comp.write(f);
        }

        header.objectTextures = f.align4();
        for (int32 i = 0; i < objectTexturesCount; i++)
        {
            const LevelPC::ObjectTexture* objectTexture = objectTextures + i;

            ObjectTextureComp comp;
            comp.attribute = objectTexture->attribute;
            comp.tile = objectTexture->tile & 0x3FFF;
            comp.uv0 = ((objectTexture->uv0 << 16) | (objectTexture->uv0 >> 16)) & 0xFF00FF00;
            comp.uv1 = ((objectTexture->uv1 << 16) | (objectTexture->uv1 >> 16)) & 0xFF00FF00;
            comp.uv2 = ((objectTexture->uv2 << 16) | (objectTexture->uv2 >> 16)) & 0xFF00FF00;
            comp.uv3 = ((objectTexture->uv3 << 16) | (objectTexture->uv3 >> 16)) & 0xFF00FF00;

        #define FIX_TEXCOORD // GBA rasterizer doesn't support UV deltas over 127, due performance reason, so we clamp it

        #ifdef FIX_TEXCOORD
            fixObjectTexture(comp, i);
        #endif

            comp.write(f);
        }

        header.spriteTextures = f.align4();
        for (int32 i = 0; i < spriteTexturesCount; i++)
        {
            const LevelPC::SpriteTexture* spriteTexture = spriteTextures + i;

            SpriteTextureComp comp;
            comp.tile = spriteTexture->tile;
            comp.u = spriteTexture->u;
            comp.v = spriteTexture->v;
            comp.w = spriteTexture->w >> 8;
            comp.h = spriteTexture->h >> 8;
            comp.l = spriteTexture->l;
            comp.t = spriteTexture->t;
            comp.r = spriteTexture->r;
            comp.b = spriteTexture->b;

            comp.write(f);
        }

        f.writeObj(spriteTextures, spriteTexturesCount);

        header.spriteSequences = f.align4();
        f.writeObj(spriteSequences, spriteSequencesCount);

        header.cameras = f.align4();
        f.writeObj(cameras, camerasCount);

        header.soundSources = f.align4();
        f.writeObj(soundSources, soundSourcesCount);

        header.boxes = f.align4();
        for (int32 i = 0; i < boxesCount; i++)
        {
            const LevelPC::Box* box = boxes + i;

            BoxComp comp;
            comp.minX = box->minX / 1024;
            comp.minZ = box->minZ / 1024;
            comp.maxX = (box->maxX + 1) / 1024;
            comp.maxZ = (box->maxZ + 1) / 1024;
            comp.floor = box->floor;
            comp.overlap = box->overlap;

            comp.write(f);
        }

        header.overlaps = f.align4();
        f.write(overlaps, overlapsCount);

        for (int32 i = 0; i < 2; i++)
        {
            header.zones[i][0] = f.align4();
            f.write(zones[i].ground1, boxesCount);

            header.zones[i][1] = f.align4();
            f.write(zones[i].ground2, boxesCount);

            header.zones[i][2] = f.align4();
            f.write(zones[i].fly, boxesCount);
        }

        header.animTexData = f.align4();
        f.write(animTexData, animTexDataSize);

        header.items = f.align4();
        for (int32 i = 0; i < itemsCount; i++)
        {
            const LevelPC::Item* item = items + i;
            const LevelPC::Room* room = rooms + item->roomIndex;

            ItemComp comp;
            comp.type = uint8(item->type);
            comp.roomIndex = uint8(item->roomIndex);
            comp.pos.x = int16(item->pos.x - room->info.x);
            comp.pos.y = int16(item->pos.y);
            comp.pos.z = int16(item->pos.z - room->info.z);
            comp.intensity = item->intensity < 0 ? 0 : (item->intensity >> 5);
            comp.flags = item->flags | ((item->angleY / 0x4000 + 2) << 14);

            ASSERT((item->flags & ~(0x3F1F)) == 0);

            comp.write(f);
        }

        header.cameraFrames = f.align4();
        f.writeObj(cameraFrames, cameraFramesCount);

        //f.writeArray(demoData, demoDataSize);

        for (int32 i = 0; i < soundOffsetsCount; i++)
        {
            uint8* ptr = soundData + soundOffsets[i];
            int32 size = *(int32*)(ptr + 40);
            uint8* src = ptr + 44;
            uint8* dst = ptr;

            while ((dst - soundData) % 4 != 0) {
                dst++;
            }
            dst += 4;

            for (int32 j = 0; j < size; j++)
            {
                dst[j] = src[j];
            }

            while ((size % 4) != 0)
            {
                dst[size] = dst[size - 1];
                size++;
            }

            dst -= 4;
            *(int32*)dst = size;

            soundOffsets[i] = dst - soundData;
        }

        header.soundMap = f.align4();
        f.write(soundMap, 256);

        header.soundInfos = f.align4();
        f.writeObj(soundInfo, soundInfoCount);

        header.soundData = f.align4();
        f.write(soundData, soundDataSize);

        header.soundOffsets = f.align4();
        f.write(soundOffsets, soundOffsetsCount);

        f.setPos(0);
        header.write(f);
    }

// 3DO ========================================================================
    struct PLUT {
        uint16 colors[16];
    } PLUTs[MAX_TEXTURES];
    int32 plutsCount;

    struct Texture3DO {
        int32 data;
        int32 plut;

        uint8 wShift;
        uint8 hShift;
        uint16 color;

        uint32 pre0;
        uint32 pre1;
        uint8* src;
        int32 w;
        int32 h;
        uint16 flip;
        int16 mip;
        uint8* image;

        void write(FileStream &f) const
        {
            ASSERT(plut * sizeof(PLUT) < 0xFFFF);
            uint32 shift = wShift | (hShift << 8) | ((plut * sizeof(PLUT)) << 16);
            f.write(data);
            f.write(shift);
        }

        bool cmp(const Texture3DO &t)
        {
            if (wShift != t.wShift || hShift != t.hShift || plut != t.plut)
                return false;

            return memcmp(image, t.image, (1 << (20 - wShift)) * (1 << (16 - hShift)) / 2) == 0;
        }

    } textures3DO[MAX_TEXTURES];

    int32 spritesBaseIndex;

    uint32 nextPow2(uint32 x) {
        x--;
        x |= x >> 1;
        x |= x >> 2;
        x |= x >> 4;
        x |= x >> 8;
        x |= x >> 16;
        x++;
        return x;
    }

    uint32 shiftPow2(int32 x)
    {
        int32 count = 0;
        while (x >>= 1) {
            count++;
        }
        return count;
    }

    int32 addPalette(const PLUT &p)
    {
        for (int32 i = 0; i < plutsCount; i++)
        {
            if (memcmp(&PLUTs[i], &p, sizeof(PLUT)) == 0)
            {
                return i;
            }
        }

        PLUTs[plutsCount] = p;

        return plutsCount++;
    }

    template <typename T>
    void calcQuadFlip(T &q)
    {
        Texture3DO* tex = textures3DO + (q.flags & FACE_TEXTURE);
        bool flip = false;

        if (tex->flip & TEX_FLIP_X) {
            swap(q.indices[0], q.indices[1]);
            swap(q.indices[3], q.indices[2]);
            flip = !flip;
        }

        if (tex->flip & TEX_FLIP_Y) {
            swap(q.indices[0], q.indices[3]);
            swap(q.indices[1], q.indices[2]);
            flip = !flip;
        }

        if (flip) {
            q.flags |= FACE_CCW;
        }
    }

    int32 convertTextures3DO(const char* fileName)
    {
        #define PRE1_WOFFSET_PREFETCH   2
        #define PRE0_VCNT_PREFETCH      1
        #define PRE0_VCNT_SHIFT         6
        #define PRE0_BPP_4              3
        #define PRE1_TLHPCNT_PREFETCH   1
        #define PRE1_TLHPCNT_SHIFT      0
        #define PRE1_TLLSB_PDC0         0x00001000
        #define PRE1_WOFFSET8_SHIFT     24
        #define PRE0_BGND               0x40000000
        #define PRE0_LINEAR             0x00000010
        #define PRE0_BPP_16             0x00000006

        ASSERT(objectTexturesCount + spriteTexturesCount < MAX_TEXTURES);

        plutsCount = 0;

        FileStream f(fileName, true);

        if (!f.isValid()) return 0;

        f.bigEndian = true;

    // reserve 4 bytes for the PLUTs offset
        f.seek(4);

    // convert palette to 15-bit and fix some color gradients
        uint16 pal[256];

        for (int32 i = 0; i < 256; i++)
        {
            uint8 b = palette.colors[i * 3 + 0];
            uint8 g = palette.colors[i * 3 + 1];
            uint8 r = palette.colors[i * 3 + 2];

            pal[i] = (r >> 1) | ((g >> 1) << 5) | ((b >> 1) << 10);
        }

        pal[0] = 0;

    // convert palette to 16 x PLUTs
        {
            for (int32 i = 0; i < 16; i++)
            {
                memcpy(PLUTs[i].colors, &pal[i * 16], 16 * sizeof(uint16));
            }
            plutsCount = 16;
        }

    // convert palette to 32-bit
        uint32 pal32[256];
        for (int32 i = 0; i < 256; i++)
        {
            uint16 p = pal[i];

            uint8 r = (p & 31) << 3;
            uint8 g = ((p >> 5) & 31) << 3;
            uint8 b = ((p >> 10) & 31) << 3;

            pal32[i] = r | (g << 8) | (b << 16);

            if (pal32[i]) {
                pal32[i] |= 0xFF000000;
            }
        }
        pal32[0] = 0;

        uint32* bitmap32 = new uint32[256 * 256];
        uint32* bitmap32_tmp = new uint32[256 * 256];
        uint8* bitmap8 = new uint8[256 * 256];
        uint8* bitmap8_tmp = new uint8[256 * 256];

        spritesBaseIndex = objectTexturesCount;

        {
            LevelPC::ObjectTexture* tmp = new LevelPC::ObjectTexture[objectTexturesCount + spriteTexturesCount];
            memcpy(tmp, objectTextures, sizeof(LevelPC::ObjectTexture) * objectTexturesCount);

            for (int32 i = 0; i < spriteTexturesCount; i++)
            {
                LevelPC::SpriteTexture* spriteTexture = spriteTextures + i; 
                LevelPC::ObjectTexture* objectTexture = tmp + objectTexturesCount + i;

                int32 w = spriteTexture->w >> 8;
                int32 h = spriteTexture->h >> 8;

                objectTexture->attribute = TEX_ATTR_AKILL;
                objectTexture->tile = spriteTexture->tile;
                objectTexture->uv0 = 0;
                objectTexture->uv1 = 0;
                objectTexture->uv2 = 0;
                objectTexture->uv3 = 0;
                objectTexture->x0 = spriteTexture->u;
                objectTexture->y0 = spriteTexture->v;
                objectTexture->x1 = spriteTexture->u + w;
                objectTexture->y1 = spriteTexture->v;
                objectTexture->x2 = spriteTexture->u + w;
                objectTexture->y2 = spriteTexture->v + h;
                objectTexture->x3 = spriteTexture->u;
                objectTexture->y3 = spriteTexture->v + h;
            }

            delete[] objectTextures;
            objectTextures = tmp;

            objectTexturesCount += spriteTexturesCount;
        }

        int32 mipIndex = objectTexturesCount;

        int32 dupSize = 0;

        for (int32 i = 0; i < objectTexturesCount; i++)
        {
            const LevelPC::ObjectTexture* objectTexture = objectTextures + i;

            int32 x0 = MIN(MIN(objectTexture->x0, objectTexture->x1), objectTexture->x2);
            int32 y0 = MIN(MIN(objectTexture->y0, objectTexture->y1), objectTexture->y2);
            int32 x1 = MAX(MAX(objectTexture->x0, objectTexture->x1), objectTexture->x2);
            int32 y1 = MAX(MAX(objectTexture->y0, objectTexture->y1), objectTexture->y2);

            textures3DO[i].flip = 0;

            if (objectTexture->isQuad)
            {
                if (objectTexture->x0 > objectTexture->x1) textures3DO[i].flip |= TEX_FLIP_X;
                if (objectTexture->y0 > objectTexture->y2) textures3DO[i].flip |= TEX_FLIP_Y;
            }

            int32 w = x1 - x0 + 1;
            int32 h = y1 - y0 + 1;

            textures3DO[i].src = tiles[objectTexture->tile & 0x3FFF].indices + 256 * y0 + x0;
            textures3DO[i].w = w;
            textures3DO[i].h = h;

            { // check if the texture is already converted
                int32 index = -1;

                if (objectTextures[i].isQuad)
                {
                    for (int32 j = 0; j < i; j++)
                    {
                        if (objectTextures[j].isQuad && textures3DO[j].src == textures3DO[i].src)
                        {
                            // TODO can we reuse textures with the same src and width but smaller height?
                            if ((textures3DO[j].w == textures3DO[i].w) && (textures3DO[j].h == textures3DO[i].h))
                            {
                                index = j;
                                break;
                            }
                        }
                    }
                }

                if (index != -1)
                {
                    uint8 flip = textures3DO[i].flip;
                    textures3DO[i] = textures3DO[index];
                    textures3DO[i].flip = flip; // flip flags may differ
                    continue; // skip texture conversion
                }
            }


            { // copy tile to 32-bit image and calculate average tile color
                uint8* src = textures3DO[i].src;
                uint32* dst = bitmap32;

                uint32 avgR = 0;
                uint32 avgG = 0;
                uint32 avgB = 0;

                for (int32 y = 0; y < h; y++)
                {
                    for (int32 x = 0; x < w; x++)
                    {
                        if (!objectTexture->isQuad)
                        {
                            float u = float(x) / float(w - 1);
                            float v = float(y) / float(h - 1);

                            float px0 = objectTexture->x0;
                            float py0 = objectTexture->y0;
                            float px1 = objectTexture->x1;
                            float py1 = objectTexture->y1;
                            float px2 = objectTexture->x2;
                            float py2 = objectTexture->y2;
                            float px3 = objectTexture->x2;
                            float py3 = objectTexture->y2;

                            float px = (1.0f - u) * (1.0f - v) * px0 + u * (1.0f - v) * px1 + (1 - u) * v * px2 + u * v * px3;
                            float py = (1.0f - u) * (1.0f - v) * py0 + u * (1.0f - v) * py1 + (1 - u) * v * py2 + u * v * py3;

                            int32 ix = int32(px + 0.5) - x0;
                            int32 iy = int32(py + 0.5) - y0;

                            ASSERT(!(ix < 0 || ix >= w || iy < 0 || iy >= h));

                            src = textures3DO[i].src + iy * 256 + ix;
                        }

                        uint32 p = pal32[*src++];
                        *dst++ = p;

                        uint32 A = p >> 24;
                        if (A)
                        {
                            avgR += (p >> 16) & 0xFF;
                            avgG += (p >> 8) & 0xFF;
                            avgB += (p) & 0xFF;
                        }
                    }
                    src += 256 - w;
                }

                avgR /= w * h;
                avgG /= w * h;
                avgB /= w * h;

                textures3DO[i].color = (avgB >> 3) | ((avgG >> 3) << 5) | ((avgR >> 3) << 10);
            }

            { // resize to POT
                int32 wp = nextPow2(w);
                int32 hp = nextPow2(h);

                if (wp != w) {
                    wp /= 2;
                }

                if (hp != h) {
                    hp /= 2;
                }

                if (wp > 64) {
                    wp = 64;
                }

                if (hp > 64) {
                    hp = 64;
                }

                ASSERT(wp != 0 && hp != 0);

                if (w != wp || h != hp)
                {
                    stbir_resize_uint8_generic((uint8*)bitmap32, w, h, 0, (uint8*)bitmap32_tmp, wp, hp, 0, 4, 3, 0,
                                                STBIR_EDGE_CLAMP, STBIR_FILTER_BOX, STBIR_COLORSPACE_LINEAR, NULL);
                    swap(bitmap32, bitmap32_tmp);

                    w = wp;
                    h = hp;
                }
            }

            /*{
                char buf[128];
                sprintf(buf, "tex%d.bmp", i);
                saveBitmap(buf, (uint8*)bitmap32, w, h, 32);
            }*/

            int32 rowBytes = (((w * 4) + 31) >> 5) << 2;
            if (rowBytes < 8) {
                rowBytes = 8;
            }
            int32 rowWOFFSET = (rowBytes >> 2) - PRE1_WOFFSET_PREFETCH;

            textures3DO[i].pre0 = ((h - PRE0_VCNT_PREFETCH) << PRE0_VCNT_SHIFT) | PRE0_BPP_4;
            textures3DO[i].pre1 = ((w - PRE1_TLHPCNT_PREFETCH) << PRE1_TLHPCNT_SHIFT) | PRE1_TLLSB_PDC0 | (rowWOFFSET << PRE1_WOFFSET8_SHIFT);
            textures3DO[i].wShift = 20 - shiftPow2(w);
            textures3DO[i].hShift = 16 - shiftPow2(h);

            if (!(objectTexture->attribute & TEX_ATTR_AKILL)) {
                textures3DO[i].pre0 |= PRE0_BGND;
            }

            { // quantize to 16 colors
                liq_attr *attr = liq_attr_create();
                liq_image *image = liq_image_create_rgba(attr, bitmap32, w, h, 0);
                liq_set_max_colors(attr, 16);

                liq_result *res;
                liq_image_quantize(image, attr, &res);

                liq_write_remapped_image(res, image, bitmap8, 256 * 256);
                const liq_palette *pal8 = liq_get_palette(res);

                PLUT plut;

                memset(&plut, 0, sizeof(plut));
                for(int32 j = 0; j < pal8->count; j++)
                {
                    liq_color c = pal8->entries[j];
                    if (c.a < 128) {
                        plut.colors[j] = 0;
                    } else {
                        plut.colors[j] = (c.r >> 3) | ((c.g >> 3) << 5) | ((c.b >> 3) << 10);
                    }
                }

                textures3DO[i].plut = addPalette(plut);

                liq_result_destroy(res);
                liq_image_destroy(image);
                liq_attr_destroy(attr);
            }

            if (rowBytes * 2 != w) // adjust row pitch
            {
                uint8* src = bitmap8;
                uint8* dst = bitmap8_tmp;
                memset(dst, 0, (rowBytes * 2) * h);

                for (int32 y = 0; y < h; y++) {
                    memcpy(dst, src, w);
                    dst += rowBytes * 2;
                    src += w;
                }

                swap(bitmap8, bitmap8_tmp);
            }

            { // encode to 4-bit image
                textures3DO[i].image = new uint8[rowBytes * h];

                uint8* src = bitmap8;
                uint8* dst = textures3DO[i].image;
                for (int32 y = 0; y < h; y++)
                {
                    for (int32 x = 0; x < rowBytes; x++, src += 2)
                    {
                        *dst++ = (src[0] << 4) | src[1];
                    }
                }

                textures3DO[i].data = 0;
/*
                for (int32 j = 0; j < i; j++)
                {
                    if (textures3DO[i].cmp(textures3DO[j]))
                    {
                        textures3DO[i].data = textures3DO[j].data;

                        //ASSERT((objectTextures[i].attribute & TEX_ATTR_MIPS) == (objectTextures[j].attribute & TEX_ATTR_MIPS));

                        dupSize += rowBytes * h;
                        break;
                    }
                }
                */
            // write image
                if (!textures3DO[i].data) {
                    textures3DO[i].data = f.getPos();

                    f.write(textures3DO[i].pre0);
                    f.write(textures3DO[i].pre1);
                    f.write(textures3DO[i].image, rowBytes * h);
                }
            }

        // generate mip level
            if (!(objectTexture->attribute & TEX_ATTR_MIPS)) {
                textures3DO[i].mip = -1;
                continue;
            }

            textures3DO[i].mip = mipIndex;

            Texture3DO* mip = &textures3DO[mipIndex++];
            *mip = textures3DO[i];

            w >>= 1;
            h >>= 1;
            ASSERT(w > 0);
            ASSERT(h > 0);

            {
                stbir_resize_uint8_generic((uint8*)bitmap32, w << 1, h << 1, 0, (uint8*)bitmap32_tmp, w, h, 0, 4, 3, 0,
                                            STBIR_EDGE_CLAMP, STBIR_FILTER_BOX, STBIR_COLORSPACE_LINEAR, NULL);
                swap(bitmap32, bitmap32_tmp);
            }

            rowBytes = (((w * 4) + 31) >> 5) << 2;
            if (rowBytes < 8) {
                rowBytes = 8;
            }
            rowWOFFSET = (rowBytes >> 2) - PRE1_WOFFSET_PREFETCH;

            mip->pre0 = ((h - PRE0_VCNT_PREFETCH) << PRE0_VCNT_SHIFT) | PRE0_BPP_4;
            mip->pre1 = ((w - PRE1_TLHPCNT_PREFETCH) << PRE1_TLHPCNT_SHIFT) | PRE1_TLLSB_PDC0 | (rowWOFFSET << PRE1_WOFFSET8_SHIFT);
            mip->wShift = 20 - shiftPow2(w);
            mip->hShift = 16 - shiftPow2(h);

            if (!(objectTexture->attribute & TEX_ATTR_AKILL)) {
                mip->pre0 |= PRE0_BGND;
            }

            { // quantize to 16 colors
                liq_attr *attr = liq_attr_create();
                liq_image *image = liq_image_create_rgba(attr, bitmap32, w, h, 0);
                liq_set_max_colors(attr, 16);

                liq_result *res;
                liq_image_quantize(image, attr, &res);

                liq_write_remapped_image(res, image, bitmap8, 256 * 256);
                const liq_palette *pal8 = liq_get_palette(res);

                PLUT plut;

                memset(&plut, 0, sizeof(plut));
                for(int32 j = 0; j < pal8->count; j++)
                {
                    liq_color c = pal8->entries[j];
                    if (c.a < 128) {
                        plut.colors[j] = 0;
                    } else {
                        plut.colors[j] = (c.r >> 3) | ((c.g >> 3) << 5) | ((c.b >> 3) << 10);
                    }
                }

                mip->plut = addPalette(plut);

                liq_result_destroy(res);
                liq_image_destroy(image);
                liq_attr_destroy(attr);
            }

            if (rowBytes * 2 != w) // adjust row pitch
            {
                uint8* src = bitmap8;
                uint8* dst = bitmap8_tmp;
                memset(dst, 0, (rowBytes * 2) * h);

                for (int32 y = 0; y < h; y++) {
                    memcpy(dst, src, w);
                    dst += rowBytes * 2;
                    src += w;
                }

                swap(bitmap8, bitmap8_tmp);
            }

            { // encode to 4-bit image
                uint8* src = bitmap8;
                uint8* dst = bitmap8_tmp;
                for (int32 y = 0; y < h; y++)
                {
                    for (int32 x = 0; x < rowBytes; x++, src += 2)
                    {
                        *dst++ = (src[0] << 4) | src[1];
                    }
                }

            // write image
                mip->data = f.getPos();
                f.write(mip->pre0);
                f.write(mip->pre1);
                f.write(bitmap8_tmp, rowBytes * h);
            }
        }

        objectTexturesCount = mipIndex;

        printf("duplicate size: %d\n", dupSize);

        uint32 paletteOffset = f.align4();

    // write PLUTs
        f.write((uint16*)PLUTs, sizeof(PLUT) / 2 * plutsCount);

    // calculate underwater PLUTs (blue tint = (0.5, 0.8, 0.8))
        {
            uint16* src = PLUTs[0].colors;
            for (int32 i = 0; i < plutsCount * 16; i++)
            {
                uint16 p = *src;

                uint32 b = (p & 31) << 3;
                uint32 g = ((p >> 5) & 31) << 3;
                uint32 r = ((p >> 10) & 31) << 3;

                r = int32(r * 0.5f);
                g = int32(g * 0.8f);
                b = int32(b * 0.8f);

                *src++ = (b >> 3) | ((g >> 3) << 5) | ((r >> 3) << 10);
            }
        }
        f.write((uint16*)PLUTs, sizeof(PLUT) / 2 * plutsCount);

        int32 texFileSize = f.getPos();

    // write palette offset at the file start
        f.setPos(0);
        f.write(paletteOffset);

        delete[] bitmap32;
        delete[] bitmap32_tmp;
        delete[] bitmap8;
        delete[] bitmap8_tmp;

        return texFileSize;
    }

    void getSample(const char* base, const char* prefix, int32 id, int32 sub, uint8* buffer, int32 &size)
    {
        // 57 == 38?
        size = 0;

        char path[256];
        sprintf(path, "%s\\%s%03d_%d.wav", base, prefix, id, sub);

        FILE* f = fopen(path, "rb");

        if (!f)
        {
            if (prefix[0] == '_') // try to open file without the prefix
            {
                sprintf(path, "%s\\%s%03d_%d.wav", base, "", id, sub);
                f = fopen(path, "rb");
                if (!f)
                {
                    printf("%s not found!\n", path);
                    return;
                }
            } else {
                printf("%s not found!\n", path);
                return;
            }
        }

        fseek(f, 12, SEEK_SET); // skip RIFF header

        struct {
            unsigned int id;
            unsigned int size;
        } chunk;

        while (1)
        {
            fread(&chunk, sizeof(chunk), 1, f);
            if (chunk.id == 0x61746164) // data
            {
                int numSamples = chunk.size / (1 * sizeof(short));
                size = numSamples / 2; // 4 bits per sample

                short* data = new short[chunk.size / sizeof(short)];

                fread(data, 1, chunk.size, f);

                BlockADDVIEncode(buffer, data, numSamples, 1); // mono block

                delete[] data;

                break;
            } else {
                fseek(f, chunk.size, SEEK_CUR);
            }
        }

        fclose(f);
    }

    bool getSoundID(int32 index, int32 &id, int32 &sub)
    {
        for (int32 i = 0; i < 256; i++)
        {
            SoundInfo &s = soundInfo[soundMap[i]];
            if (s.index <= index && s.index + s.flags.count > index)
            {
                id = i;
                sub = index - s.index;
                return true;
            }        
        }

        return false;
    }

    void convert3DO(const char* name)
    {
        char path[256];
        sprintf(path, "../../3do/CD/data/%s.V", name);
        int32 texFileSize = convertTextures3DO(path);

        sprintf(path, "../../3do/CD/data/%s.D", name);

        FileStream f(path, true);

        if (!f.isValid()) return;

        f.bigEndian = true;

        Header header;
        f.seek(sizeof(Header)); // will be rewritten at the end

        header.magic = 0x33444F20;
        header.tilesCount = plutsCount;
        header.roomsCount = roomsCount;
        header.modelsCount = modelsCount;
        header.meshesCount = meshOffsetsCount;
        header.staticMeshesCount = staticMeshesCount;
        header.spriteSequencesCount = spriteSequencesCount;
        header.soundSourcesCount = soundSourcesCount;
        header.boxesCount = boxesCount;
        header.texturesCount = objectTexturesCount;
        header.itemsCount = itemsCount;
        header.camerasCount = camerasCount;
        header.cameraFramesCount = cameraFramesCount;
        header.soundOffsetsCount = soundOffsetsCount;
        header._reserved = 0;

        header.palette = 0;
        header.lightmap = 0;
        
        fixHeadMask();

        header.rooms = f.align4();
        {
            f.seek(sizeof(Room::InfoComp) * roomsCount);

            Room::InfoComp infoComp[255];

            for (int32 i = 0; i < roomsCount; i++)
            {
                const LevelPC::Room* room = rooms + i;

                Room::InfoComp &info = infoComp[i];

                ASSERT(room->info.x % 256 == 0);
                ASSERT(room->info.z % 256 == 0);
                ASSERT(room->info.yBottom >= -32768 && room->info.yBottom <= 32767);
                ASSERT(room->info.yTop >= -32768 && room->info.yTop <= 32767);
                info.x = room->info.x / 256;
                info.z = room->info.z / 256;
                info.yBottom = -32768;
                info.yTop = 32767;

                for (int32 j = 0; j < room->vCount; j++)
                {
                    Room::Vertex &v = room->vertices[j];
                    if (v.pos.y < info.yTop) {
                        info.yTop = v.pos.y;
                    }
                    if (v.pos.y > info.yBottom) {
                        info.yBottom = v.pos.y;
                    }
                }

                info.spritesCount = room->sCount;
                info.quadsCount = room->qCount;
                info.trianglesCount = room->tCount;
                info.portalsCount = uint8(room->pCount);
                info.lightsCount = uint8(room->lCount);
                info.meshesCount = uint8(room->mCount);
                info.ambient = room->ambient >> 5;
                info.xSectors = uint8(room->xSectors);
                info.zSectors = uint8(room->zSectors);
                info.alternateRoom = uint8(room->alternateRoom);

                info.flags = 0;
                if (room->flags & 1) info.flags |= 1;
                if (room->flags & 256) info.flags |= 2;

                ASSERT((room->flags & ~257) == 0);
                ASSERT(info.portalsCount == room->pCount);
                ASSERT(info.lightsCount == room->lCount);
                ASSERT(info.meshesCount == room->mCount);
                ASSERT(info.xSectors == room->xSectors);
                ASSERT(info.zSectors == room->zSectors);

                roomVerticesCount = 0;

                info.quads = f.align4();
                for (int32 i = 0; i < room->qCount; i++)
                {
                    Quad q = room->quads[i];

                // get intensity
                    const Room::Vertex &v0 = room->vertices[q.indices[0]];
                    const Room::Vertex &v1 = room->vertices[q.indices[1]];
                    const Room::Vertex &v2 = room->vertices[q.indices[2]];
                    const Room::Vertex &v3 = room->vertices[q.indices[3]];

                    uint32 intensity = ((v0.lighting + v1.lighting + v2.lighting + v3.lighting) / 4) >> 5;
                    ASSERT(intensity <= 255);
                    
                    q.indices[0] = addRoomVertex(info.yTop, v0, true);
                    q.indices[1] = addRoomVertex(info.yTop, v1, true);
                    q.indices[2] = addRoomVertex(info.yTop, v2, true);
                    q.indices[3] = addRoomVertex(info.yTop, v3, true);

                    ASSERT((int32)q.indices[0] * 12 < 0xFFFF);
                    ASSERT((int32)q.indices[1] * 12 < 0xFFFF);
                    ASSERT((int32)q.indices[2] * 12 < 0xFFFF);
                    ASSERT((int32)q.indices[3] * 12 < 0xFFFF);

                    RoomQuad3DO comp;
                    comp.indices[0] = q.indices[0] * 12;
                    comp.indices[1] = q.indices[1] * 12;
                    comp.indices[2] = q.indices[2] * 12;
                    comp.indices[3] = q.indices[3] * 12;
                    comp.flags = q.flags;
                // add ccw flag and swap indices
                    calcQuadFlip(comp);
                    ASSERT((comp.flags & FACE_CCW) == 0);
                // add intensity
                    comp.flags |= (intensity << (FACE_MIP_SHIFT + FACE_MIP_SHIFT));
                    if (textures3DO[comp.flags & FACE_TEXTURE].pre0 & PRE0_BGND) {
                        comp.flags |= FACE_OPAQUE; // set opaque flag
                    }
                // add mip level
                    Texture3DO* tex = textures3DO + (comp.flags & FACE_TEXTURE);
                    if (tex->mip != -1) {
                        comp.flags |= (tex->mip << FACE_MIP_SHIFT);
                    }

                    comp.write(f);
                }

                info.triangles = f.align4();
                for (int32 i = 0; i < room->tCount; i++)
                {
                    Triangle t = room->triangles[i];

                // get intensity
                    const Room::Vertex &v0 = room->vertices[t.indices[0]];
                    const Room::Vertex &v1 = room->vertices[t.indices[1]];
                    const Room::Vertex &v2 = room->vertices[t.indices[2]];

                    uint32 intensity = ((v0.lighting + v1.lighting + v2.lighting) / 3) >> 5;
                    ASSERT(intensity <= 255);
                    
                    t.indices[0] = addRoomVertex(info.yTop, v0, true);
                    t.indices[1] = addRoomVertex(info.yTop, v1, true);
                    t.indices[2] = addRoomVertex(info.yTop, v2, true);

                    ASSERT((int32)t.indices[0] * 12 < 0xFFFF);
                    ASSERT((int32)t.indices[1] * 12 < 0xFFFF);
                    ASSERT((int32)t.indices[2] * 12 < 0xFFFF);

                    RoomTriangle3DO comp;
                    comp.indices[0] = t.indices[0] * 12;
                    comp.indices[1] = t.indices[1] * 12;
                    comp.indices[2] = t.indices[2] * 12;
                    comp._unused = 0;
                    comp.flags = t.flags;
                // add intensity
                    comp.flags |= (intensity << (FACE_MIP_SHIFT + FACE_MIP_SHIFT));
                    if (textures3DO[comp.flags & FACE_TEXTURE].pre0 & PRE0_BGND) {
                        comp.flags |= FACE_OPAQUE; // set opaque flag
                    }
                // add mip level
                    Texture3DO* tex = textures3DO + (comp.flags & FACE_TEXTURE);
                    if (tex->mip != -1) {
                        comp.flags |= (tex->mip << FACE_MIP_SHIFT);
                    }
                    comp.write(f);
                }

                info.vertices = f.align4();
                info.verticesCount = roomVerticesCount;
                for (int32 i = 0; i < roomVerticesCount; i += 4)
                {
                    Room::VertexComp v[4];

                    for (int32 j = 0; j < 4; j++)
                    {
                        if (i + j < roomVerticesCount) {
                            v[j] = roomVertices[i + j];
                        } else {
                            memset(&v[j], 0, sizeof(v[j]));
                        }
                    }

                    {
                        uint32 value = v[0].x | (v[0].y << 5) | (v[0].z << 11);
                        value |= (v[1].x | (v[1].y << 5) | (v[1].z << 11)) << 16;
                        f.write(value);
                    }

                    {
                        uint32 value = v[2].x | (v[2].y << 5) | (v[2].z << 11);
                        value |= (v[3].x | (v[3].y << 5) | (v[3].z << 11)) << 16;
                        f.write(value);
                    }
                }

                info.sprites = f.align4();
                for (int32 i = 0; i < room->sCount; i++)
                {
                    const Room::Sprite* sprite = room->sprites + i;
                    const Room::Vertex* v = room->vertices + sprite->index;

                    Room::SpriteComp comp;
                    comp.x = v->pos.x;
                    comp.y = v->pos.y;
                    comp.z = v->pos.z;
                    comp.g = v->lighting >> 5;
                    comp.index = uint8(sprite->texture);

                    ASSERT(sprite->texture <= 255);

                    comp.write(f);
                }

                info.portals = f.align4();
                f.writeObj(room->portals, room->pCount);
                /*
                for (int32 i = 0; i < room->pCount; i++)
                {
                    const Room::Portal* portal = room->portals + i;

                    Room::PortalComp comp;
                    
                    comp.roomIndex = portal->roomIndex;

                    static const struct {
                        int32 x, y, z;
                        int32 mask;
                    } normals[9] = {
                        { -1,  0,  0,  2 << 0 },
                        {  1,  0,  0,  1 << 0 },
                        {  0, -1,  0,  2 << 2 },
                        {  0,  1,  0,  1 << 2 },
                        {  0,  0, -1,  2 << 4 },
                        {  0,  0,  1,  1 << 4 }
                    };

                    comp.normalMask = 255;
                    for (int32 i = 0; i < 9; i++)
                    {
                        if (portal->normal.x == normals[i].x &&
                            portal->normal.y == normals[i].y &&
                            portal->normal.z == normals[i].z)
                        {
                            comp.normalMask = normals[i].mask;
                            break;
                        }
                    }

                    ASSERT(comp.normalMask != 255);

                    for (int32 i = 0; i < 4; i++)
                    {
                        comp.vertices[i].x = portal->vertices[i].x;
                        comp.vertices[i].y = portal->vertices[i].y;
                        comp.vertices[i].z = portal->vertices[i].z;
                    }

                    comp.write(f);
                }*/

                info.sectors = f.align4();
                f.writeObj(room->sectors, room->zSectors * room->xSectors);
            
                info.lights = f.align4();
                for (int32 i = 0; i < room->lCount; i++)
                {
                    const Room::Light* light = room->lights + i;

                    Room::LightComp comp;
                    comp.pos.x = light->pos.x - room->info.x;
                    comp.pos.y = light->pos.y;
                    comp.pos.z = light->pos.z - room->info.z;
                    comp.radius = light->radius >> 8;
                    comp.intensity = light->intensity >> 5;

                    comp.write(f);
                }

                info.meshes = f.align4();
                for (int32 i = 0; i < room->mCount; i++)
                {
                    const Room::Mesh* mesh = room->meshes + i;

                    Room::MeshComp comp;
                    comp.pos.x = mesh->pos.x - room->info.x;
                    comp.pos.y = mesh->pos.y;
                    comp.pos.z = mesh->pos.z - room->info.z;
                    comp.intensity = mesh->intensity >> 5;
                    comp.flags = ((mesh->angleY / 0x4000 + 2) << 6) | mesh->id;

                    ASSERT(mesh->id <= 63);
                    ASSERT(mesh->angleY % 0x4000 == 0);
                    ASSERT(mesh->angleY / 0x4000 + 2 >= 0);

                    comp.write(f);
                }
            }

            int32 pos = f.getPos();
            f.setPos(header.rooms);
            f.writeObj(infoComp, roomsCount);
            f.setPos(pos);
        }

        header.floors = f.align4();
        f.writeObj(floors, floorsCount);

        header.meshData = f.align4();

        int32 mOffsets[2048];
        for (int32 i = 0; i < 2048; i++) {
            mOffsets[i] = -1;
        }

        for (int32 i = 0; i < meshOffsetsCount; i++)
        {
            if (mOffsets[i] != -1)
                continue;

            mOffsets[i] = f.align4() - header.meshData;

            const uint8* ptr = (uint8*)meshData + meshOffsets[i];

            vec3s center = *(vec3s*)ptr; ptr += sizeof(center);
            int16 radius = *(int16*)ptr; ptr += sizeof(radius);
            uint16 flags = *(uint16*)ptr; ptr += sizeof(flags);

            int16 vCount = *(int16*)ptr; ptr += 2;
            const vec3s* vertices = (vec3s*)ptr;
            ptr += vCount * sizeof(vec3s);

            const uint16* vIntensity = NULL;
            const vec3s* vNormal = NULL;

            int16 nCount = *(int16*)ptr; ptr += 2;
            //const int16* normals = (int16*)ptr;
            if (nCount > 0) { // normals
                vNormal = (vec3s*)ptr;
                ptr += nCount * 3 * sizeof(int16);
            } else { // intensity
                vIntensity = (uint16*)ptr;
                ptr += vCount * sizeof(uint16);
            }

            int16     rCount = *(int16*)ptr; ptr += 2;
            Quad*     rFaces = (Quad*)ptr; ptr += rCount * sizeof(Quad);

            int16     tCount = *(int16*)ptr; ptr += 2;
            Triangle* tFaces = (Triangle*)ptr; ptr += tCount * sizeof(Triangle);

            int16     crCount = *(int16*)ptr; ptr += 2;
            Quad*     crFaces = (Quad*)ptr; ptr += crCount * sizeof(Quad);

            int16     ctCount = *(int16*)ptr; ptr += 2;
            Triangle* ctFaces = (Triangle*)ptr; ptr += ctCount * sizeof(Triangle);

            uint16 intensity = 0;

            if (vIntensity)
            {
                uint32 sum = 0;
                for (int32 i = 0; i < vCount; i++)
                {
                    sum += vIntensity[i];
                }
                intensity = sum / vCount;
            }

            f.write(center.x);
            f.write(center.y);
            f.write(center.z);
            f.write(radius);
            f.write(intensity);
            f.write(vCount);
            f.write(rCount);
            f.write(tCount);
            f.write(crCount);
            f.write(ctCount);

            for (int32 j = 0; j < vCount; j++)
            {
                struct MeshVertex3DO {
                    int16 x, y, z;
                } v;

                v.x = vertices[j].x << 2;
                v.y = vertices[j].y << 2;
                v.z = vertices[j].z << 2;

                f.write(v.x);
                f.write(v.y);
                f.write(v.z);
            }

            if (vCount % 2) { // add one vertex for the data alignment
                int16 zero = 0;
                f.write(zero);
                f.write(zero);
                f.write(zero);
            }

            for (int32 j = 0; j < rCount; j++)
            {
                Quad q = rFaces[j];

                ASSERT(q.indices[0] < 256);
                ASSERT(q.indices[1] < 256);
                ASSERT(q.indices[2] < 256);
                ASSERT(q.indices[3] < 256);

                MeshQuad3DO comp;
                comp.indices[0] = q.indices[0];
                comp.indices[1] = q.indices[1];
                comp.indices[2] = q.indices[2];
                comp.indices[3] = q.indices[3];
                comp.flags = q.flags;
                if (textures3DO[comp.flags & FACE_TEXTURE].pre0 & PRE0_BGND) {
                    comp.flags |= FACE_OPAQUE; // set opaque flag
                }
                calcQuadFlip(comp);
                comp.write(f);
            }

            for (int32 j = 0; j < tCount; j++)
            {
                Triangle t = tFaces[j];

                ASSERT(t.indices[0] < 256);
                ASSERT(t.indices[1] < 256);
                ASSERT(t.indices[2] < 256);

                MeshTriangle3DO comp;
                comp.indices[0] = t.indices[0];
                comp.indices[1] = t.indices[1];
                comp.indices[2] = t.indices[2];
                comp._unused = 0;
                comp.flags = t.flags;
                if (textures3DO[comp.flags & FACE_TEXTURE].pre0 & PRE0_BGND) {
                    comp.flags |= FACE_OPAQUE; // set opaque flag
                }
                comp.write(f);
            }

            for (int32 j = 0; j < crCount; j++)
            {
                Quad q = crFaces[j];

                ASSERT(q.indices[0] < 256);
                ASSERT(q.indices[1] < 256);
                ASSERT(q.indices[2] < 256);
                ASSERT(q.indices[3] < 256);

                MeshQuad3DO comp;
                comp.indices[0] = q.indices[0];
                comp.indices[1] = q.indices[1];
                comp.indices[2] = q.indices[2];
                comp.indices[3] = q.indices[3];
                comp.flags = q.flags;
                comp.write(f);
            }

            for (int32 j = 0; j < ctCount; j++)
            {
                Triangle t = ctFaces[j];

                ASSERT(t.indices[0] < 256);
                ASSERT(t.indices[1] < 256);
                ASSERT(t.indices[2] < 256);

                MeshTriangle3DO comp;
                comp.indices[0] = t.indices[0];
                comp.indices[1] = t.indices[1];
                comp.indices[2] = t.indices[2];
                comp._unused = 0;
                comp.flags = t.flags;
                comp.write(f);
            }


            for (int32 j = i + 1; j < meshOffsetsCount; j++)
            {
                if (meshOffsets[i] == meshOffsets[j])
                {
                    mOffsets[j] = mOffsets[i];
                }
            }
        }

        header.meshOffsets = f.align4();
        f.write(mOffsets, meshOffsetsCount);

        header.anims = f.align4();
        f.writeObj(anims, animsCount);

        header.states = f.align4();
        for (int32 i = 0; i < statesCount; i++)
        {
            const LevelPC::AnimState* state = states + i;

            LevelPC::AnimStateComp comp;
            comp.state = uint8(state->state);
            comp.rangesCount = uint8(state->rangesCount);
            comp.rangesStart = state->rangesStart;

            comp.write(f);
        }

        header.ranges = f.align4();
        f.writeObj(ranges, rangesCount);

        header.commands = f.align4();
        f.write(commands, commandsCount);

        header.nodes = f.align4();
        for (int32 i = 0; i < nodesDataSize / 4; i++)
        {
            const Node* node = (Node*)(nodesData + i * 4);

            ASSERT(node->pos.x > -32768);
            ASSERT(node->pos.x <  32767);
            ASSERT(node->pos.y > -32768);
            ASSERT(node->pos.y <  32767);
            ASSERT(node->pos.z > -32768);
            ASSERT(node->pos.z <  32767);
            ASSERT(node->flags < 0xFFFF);

            LevelPC::NodeComp comp;
            comp.flags = uint16(node->flags);
            comp.pos.x = int16(node->pos.x);
            comp.pos.y = int16(node->pos.y);
            comp.pos.z = int16(node->pos.z);

            comp.write(f);
        }
        //f.write(nodesData, nodesDataSize);

        header.frameData = f.align4();
        f.write(frameData, frameDataSize);

        header.models = f.align4();
        for (int32 i = 0; i < modelsCount; i++)
        {
            const LevelPC::Model* model = models + i;

            LevelPC::ModelComp comp;
            comp.type = uint8(model->type);
            comp.count = uint8(model->count);
            comp.start = model->start;
            comp.nodeIndex = model->nodeIndex / 4;
            comp.animIndex = model->animIndex;

            comp.write(f);
        }

        header.staticMeshes = f.align4();
        for (int32 i = 0; i < staticMeshesCount; i++)
        {
            const LevelPC::StaticMesh* staticMesh = staticMeshes + i;

            LevelPC::StaticMeshComp comp;
            comp.id = staticMesh->id;
            comp.meshIndex = staticMesh->meshIndex;
            comp.flags = staticMesh->flags;
            comp.vbox = staticMesh->vbox;
            comp.cbox = staticMesh->cbox;

            comp.write(f);
        }

        header.objectTextures = f.align4();
        for (int32 i = 0; i < objectTexturesCount; i++)
        {
            textures3DO[i].write(f);
        }

        header.spriteTextures = f.align4();
        for (int32 i = 0; i < spriteTexturesCount; i++)
        {
            const LevelPC::SpriteTexture* spriteTexture = spriteTextures + i;

            SpriteTexture3DO comp;
            comp.texture = spritesBaseIndex + i;
            comp.l = spriteTexture->l;
            comp.t = spriteTexture->t;
            comp.r = spriteTexture->r;
            comp.b = spriteTexture->b;

            comp.write(f);
        }

        f.writeObj(spriteTextures, spriteTexturesCount);

        header.spriteSequences = f.align4();
        f.writeObj(spriteSequences, spriteSequencesCount);

        header.cameras = f.align4();
        f.writeObj(cameras, camerasCount);

        header.soundSources = f.align4();
        f.writeObj(soundSources, soundSourcesCount);

        header.boxes = f.align4();
        for (int32 i = 0; i < boxesCount; i++)
        {
            const LevelPC::Box* box = boxes + i;

            BoxComp comp;
            comp.minX = box->minX >> 10;
            comp.minZ = box->minZ >> 10;
            comp.maxX = (box->maxX + 1) >> 10;
            comp.maxZ = (box->maxZ + 1) >> 10;
            comp.floor = box->floor;
            comp.overlap = box->overlap;

            comp.write(f);
        }

        header.overlaps = f.align4();
        f.write(overlaps, overlapsCount);

        for (int32 i = 0; i < 2; i++)
        {
            header.zones[i][0] = f.align4();
            f.write(zones[i].ground1, boxesCount);

            header.zones[i][1] = f.align4();
            f.write(zones[i].ground2, boxesCount);

            header.zones[i][2] = f.align4();
            f.write(zones[i].fly, boxesCount);
        }

        header.animTexData = f.align4();
        {
            int32 lastPos = f.getPos();

            uint16 rangesCount = *animTexData++;
            struct TexAnimRange
            {
                uint16 count;
                uint16 indices[256];
            } ranges[64];
            ASSERT(rangesCount <= 64);

            int32 newRangesCount = rangesCount;

            for (int32 i = 0; i < rangesCount; i++)
            {
                bool mips = true;

                TexAnimRange &range = ranges[i];
                range.count = *animTexData++;
                for (int32 j = 0; j <= range.count; j++)
                {
                    range.indices[j] = *animTexData++;

                    if (textures3DO[range.indices[j]].mip < 0) {
                        mips = false;
                    }
                }

            // add the new anim range for mip textures
                if (mips)
                {
                    TexAnimRange &mipRange = ranges[newRangesCount++];
                    mipRange.count = range.count;
                    for (int32 j = 0; j <= range.count; j++)
                    {
                        mipRange.indices[j] = textures3DO[range.indices[j]].mip;
                    }
                }
            }
            rangesCount = newRangesCount;

            f.write(rangesCount);
            for (int32 i = 0; i < rangesCount; i++)
            {
                f.write(ranges[i].count);
                f.write(ranges[i].indices, ranges[i].count + 1);
            }

            lastPos = f.getPos() - lastPos;
        }

        header.items = f.align4();
        for (int32 i = 0; i < itemsCount; i++)
        {
            const LevelPC::Item* item = items + i;
            const LevelPC::Room* room = rooms + item->roomIndex;

            ItemComp comp;
            comp.type = uint8(item->type);
            comp.roomIndex = uint8(item->roomIndex);
            comp.pos.x = int16(item->pos.x - room->info.x);
            comp.pos.y = int16(item->pos.y);
            comp.pos.z = int16(item->pos.z - room->info.z);
            comp.intensity = item->intensity < 0 ? 0 : (item->intensity >> 5);
            comp.flags = item->flags | ((item->angleY / 0x4000 + 2) << 14);

            ASSERT((item->flags & ~(0x3F1F)) == 0);

            comp.write(f);
        }

        header.cameraFrames = f.align4();
        f.writeObj(cameraFrames, cameraFramesCount);

        //f.writeArray(demoData, demoDataSize);

        header.soundMap = f.align4();
        f.write(soundMap, 256);

        header.soundInfos = f.align4();
        f.writeObj(soundInfo, soundInfoCount);

        header.soundData = f.align4();

        uint8* soundBuf = new uint8[2 * 1024 * 1024];

        bool isHome = strcmp(name, "GYM") == 0;

        for (int32 i = 0; i < soundOffsetsCount; i++)
        {
            soundOffsets[i] = f.align4() - header.soundData;

            int32 id, sub, size;
            if (getSoundID(i, id, sub))
            {
                getSample("C:\\Projects\\OpenLara\\src\\platform\\gba\\packer\\sounds\\conv_3do", isHome ? "_" : "", id, sub, soundBuf, size);
            } else {
                ASSERT(false);
            }

            int32 numSamples = size * 2;
            f.write(numSamples);

            if (size) {
                f.write(soundBuf, size);
            }
        }

        delete[] soundBuf;

        header.soundOffsets = f.align4();
        f.write(soundOffsets, soundOffsetsCount);

        f.setPos(0);
        header.write(f);
    }

};

#define COLOR_THRESHOLD_SQ (8 * 8)

const char* levelNames[] = {
#if 0
    "TITLE",
    "GYM",
    "LEVEL1",
    "LEVEL2",
#else
    "TITLE",
    "GYM",
    "LEVEL1",
    "LEVEL2",
    "LEVEL3A",
    "LEVEL3B",
    "CUT1",
    "LEVEL4",
    "LEVEL5",
    "LEVEL6",
    "LEVEL7A",
    "LEVEL7B",
    "CUT2",
    "LEVEL8A",
    "LEVEL8B",
    "LEVEL8C",
    "LEVEL10A",
    "CUT3",
    "LEVEL10B",
    "CUT4",
    "LEVEL10C"
#endif
};


#define MAX_LEVELS sizeof(levelNames) / sizeof(levelNames[0])

LevelPC* levels[MAX_LEVELS];

struct WAD
{
    struct Texture
    {
        int32 id;
        uint8* data;
        int32 width;
        int32 height;

        Texture(const Tile* tile, const Palette* palette, int32 minX, int32 minY, int32 maxX, int32 maxY)
        {
            width = maxX - minX + 1;
            height = maxY - minY + 1;

            data = new uint8[width * height * 3];

            const uint8* src = tile->indices + 256 * minY;
            uint8* dst = data;

            for (int32 y = minY; y <= maxY; y++)
            {
                for (int32 x = minX; x <= maxX; x++)
                {
                    int32 idx = src[x] * 3;
                    if (idx == 0) {
                        dst[0] = 255;
                        dst[1] = 0;
                        dst[2] = 255;
                    } else {
                        dst[2] = palette->colors[idx + 0] << 2;
                        dst[1] = palette->colors[idx + 1] << 2;
                        dst[0] = palette->colors[idx + 2] << 2;
                    }
                    dst += 3;
                }
                src += 256;
            }
        }

        ~Texture()
        {
            delete[] data;
        }

        bool isEqual(Texture* tex)
        {
            if (width != tex->width || height != tex->height)
                return false;

            uint8* src = data;
            uint8* dst = tex->data;

            for (int32 i = 0; i < width * height; i++)
            {
                int32 dR = *src++ - *dst++;
                int32 dG = *src++ - *dst++;
                int32 dB = *src++ - *dst++;

                if (dR * dR + dG * dG + dB * dB > COLOR_THRESHOLD_SQ)
                    return false;
            }

            return true;
        }

        void save(const char* fileName)
        {
            saveBitmap(fileName, data, width, height);
        }

        static int cmp(const Texture* a, const Texture* b)
        {
            int32 p1 = MIN(a->width, a->height);
            int32 p2 = MIN(b->width, b->height);
            int32 i = p2 - p1;
            if (i == 0)
                i = a->id - b->id;
            return i;
        }
    };

    struct Tile24
    {
        struct Node
        {
            Node* childs[2];
            Texture* tex;
            int32 l, t, r, b;

            Node(short l, short t, short r, short b) : l(l), t(t), r(r), b(b), tex(NULL) { 
                childs[0] = childs[1] = NULL; 
            }

            ~Node() {
                delete childs[0];
                delete childs[1];
            }

            Node* insert(Texture* tex)
            {
                if (childs[0] != NULL && childs[1] != NULL)
                {
                    Node* node = childs[0]->insert(tex);
                    if (node != NULL)
                        return node;
                    return childs[1]->insert(tex);
                }

                if (this->tex != NULL)
                    return NULL;

                int16 nw  = r - l;
                int16 nh  = b - t;
                int16 tw = tex->width;
                int16 th = tex->height;

                if (nw < tw || nh < th)
                    return NULL;

                if (nw == tw && nh == th) {
                    this->tex = tex;
                    return this;
                }

                int16 dx = nw - tw;
                int16 dy = nh - th;

                if (dx > dy) {
                    childs[0] = new Node(l, t, l + tw, b);
                    childs[1] = new Node(l + tw, t, r, b);
                } else {
                    childs[0] = new Node(l, t, r, t + th);
                    childs[1] = new Node(l, t + th, r, b);
                }

                return childs[0]->insert(tex);
            }

            void fill(uint8* data)
            {
                if (childs[0] != NULL && childs[1] != NULL)
                {
                    childs[0]->fill(data);
                    childs[1]->fill(data);
                }

                if (!tex) {
                    return;
                }

                // fill code
                for (int32 y = 0; y < tex->height; y++)
                {
                    memcpy(data + ((t + y) * 256 + l) * 3, tex->data + y * tex->width * 3, tex->width * 3);
                }
            }
        };

        Node* root;

        Tile24()
        {
            root = new Node(0, 0, 256, 256);
        }

        ~Tile24()
        {
            delete root;
        }

        void fill(uint8* data)
        {
            for (int32 i = 0; i < 256 * 256; i++)
            {
                data[i * 3 + 0] = 255;
                data[i * 3 + 1] = 0;
                data[i * 3 + 2] = 255;
            }

            root->fill(data);
        }

        static int cmp(const Tile24* a, const Tile24* b)
        {
            return 0;
        }
    };

    struct LevelWAD
    {
        struct Room
        {
            struct Vertex
            {
                int8  x, y, z; 
                uint8 lighting;
            };

            struct Quad
            {
                uint16 flags;
                uint16 indices[4];
            };

            struct Triangle
            {
                uint16 flags;
                uint16 indices[3];
            };

            struct Sprite
            {
                int16  x, y, z;
                uint16 texture;
            };

            struct Portal
            {
                uint8 roomIndex;
                uint8 normalIndex;
                uint16 x, y, z;
                uint8 a, b;

                Portal() {}
                Portal(const LevelPC::Room::Portal &portal)
                {

                    const vec3s &v0 = portal.vertices[0];
                    const vec3s &v1 = portal.vertices[1];
                    const vec3s &v2 = portal.vertices[2];
                    const vec3s &v3 = portal.vertices[3];
                    const vec3s &n = portal.normal;

                    normalIndex = 0xFF;

                    if (n.x == -1) normalIndex = 0;
                    if (n.x ==  1) normalIndex = 1;
                    if (n.y == -1) normalIndex = 2;
                    if (n.y ==  1) normalIndex = 3;
                    if (n.z == -1) normalIndex = 4;
                    if (n.z ==  1) normalIndex = 5;

                    ASSERT(normalIndex != 0xFF);

                    int32 minX = MIN(MIN(MIN(v0.x, v1.x), v2.x), v3.x);
                    int32 minY = MIN(MIN(MIN(v0.y, v1.y), v2.y), v3.y);
                    int32 minZ = MIN(MIN(MIN(v0.z, v1.z), v2.z), v3.z);
                    int32 maxX = MAX(MAX(MAX(v0.x, v1.x), v2.x), v3.x);
                    int32 maxY = MAX(MAX(MAX(v0.y, v1.y), v2.y), v3.y);
                    int32 maxZ = MAX(MAX(MAX(v0.z, v1.z), v2.z), v3.z);

                    x = (maxX + minX) / 2;
                    y = (maxY + minY) / 2;
                    z = (maxZ + minZ) / 2;

                    int32 sx = (maxX - minX) / 256;
                    int32 sy = (maxY - minY) / 256;
                    int32 sz = (maxZ - minZ) / 256;

                    switch (normalIndex / 2)
                    {
                        case 0 : // x
                            a = sy;
                            b = sz;
                            break;
                        case 1 : // y
                            a = sx;
                            b = sz;
                            break;
                        case 2 : // z
                            a = sx;
                            b = sy;
                            break;
                    }
                }
            };

            struct Sector
            {
                uint16 floorIndex;
                uint16 boxIndex;
                uint8 roomBelow;
                int8 floor;
                uint8 roomAbove;
                int8 ceiling;
            };

            int16 vCount;
            int16 qCount;
            int16 tCount;
            uint8 sCount;
            uint8 pCount;
            uint8 zSectors;
            uint8 xSectors;

            Vertex vertices[MAX_ROOM_VERTICES];
            Quad quads[MAX_ROOM_QUADS];
            Triangle triangles[MAX_ROOM_TRIANGLES];
            Sprite sprites[MAX_ROOM_SPRITES];
            Portal portals[MAX_ROOM_PORTALS];
            Sector sectors[MAX_ROOM_SECTORS];

            Room() {}

            Room(const LevelPC::Room &room) : vCount(0), qCount(0), tCount(0), sCount(0)
            {
                for (int32 i = 0; i < room.qCount; i++)
                {
                    addQuad(room.quads[i], room.vertices);
                }

                for (int32 i = 0; i < room.tCount; i++)
                {
                    addTriangle(room.triangles[i], room.vertices);
                }

                for (int32 i = 0; i < room.sCount; i++)
                {
                    addSprite(room.sprites[i], room.vertices);
                }

                pCount = uint8(room.pCount);
                for (int32 i = 0; i < pCount; i++)
                {
                    portals[i] = room.portals[i];
                }

                zSectors = uint8(room.zSectors);
                xSectors = uint8(room.xSectors);
                for (int32 i = 0; i < zSectors * xSectors; i++)
                {
                    const LevelPC::Room::Sector &src = room.sectors[i];
                    Sector &dst = sectors[i];
                    dst.floorIndex = src.floorIndex;
                    dst.boxIndex   = src.boxIndex;
                    dst.roomBelow  = src.roomBelow;
                    dst.floor      = src.floor;
                    dst.roomAbove  = src.roomAbove;
                    dst.ceiling    = src.ceiling;
                }
            }

            int32 addVertex(const LevelPC::Room::Vertex &v)
            {
                Vertex n;
                n.x = v.pos.x / 1024;
                n.y = v.pos.y / 256;
                n.z = v.pos.z / 1024;
                n.lighting = v.lighting >> 5;

                for (int32 i = 0; i < vCount; i++)
                {
                    if (vertices[i].x == n.x &&
                        vertices[i].y == n.y &&
                        vertices[i].z == n.z &&
                        vertices[i].lighting == n.lighting)
                    {
                        return i;
                    }
                }

                vertices[vCount++] = n;

                return vCount - 1;
            }

            void addQuad(const LevelPC::Quad &q, const LevelPC::Room::Vertex* verts)
            {
                Quad n;
                n.flags = q.flags;
                n.indices[0] = addVertex(verts[q.indices[0]]);
                n.indices[1] = addVertex(verts[q.indices[1]]);
                n.indices[2] = addVertex(verts[q.indices[2]]);
                n.indices[3] = addVertex(verts[q.indices[3]]);
                quads[qCount++] = n;
            }

            void addTriangle(const LevelPC::Triangle &t, const LevelPC::Room::Vertex* verts)
            {
                Triangle n;
                n.flags = t.flags;
                n.indices[0] = addVertex(verts[t.indices[0]]);
                n.indices[1] = addVertex(verts[t.indices[1]]);
                n.indices[2] = addVertex(verts[t.indices[2]]);
                triangles[tCount++] = n;
            }

            void addSprite(const LevelPC::Room::Sprite &s, const LevelPC::Room::Vertex* verts)
            {
                Sprite n;
                n.texture = s.texture;
                n.x = verts[s.index].pos.x;
                n.y = verts[s.index].pos.y;
                n.z = verts[s.index].pos.z;
                // lighting?
                sprites[sCount++] = n;
            }
        };

        struct FloorData
        {
            uint16 value;
        };

        struct Box
        {
            int8 minZ, maxZ;
            int8 minX, maxX;
            int16 floor;
            int16 overlap;

            Box() {}

            Box(const LevelPC::Box &b)
            {
                minX = b.minX / 1024;
                minZ = b.minZ / 1024;
                maxX = (b.maxX + 1) / 1024;
                maxZ = (b.maxZ + 1) / 1024;

                floor = int16(b.floor);
                overlap = int16(b.overlap);

                ASSERT(b.minX == minX * 1024);
                ASSERT(b.minZ == minZ * 1024);
                ASSERT(b.maxX == maxX * 1024 - 1);
                ASSERT(b.maxZ == maxZ * 1024 - 1);
            }
        };

        struct Overlap
        {
            uint16 value;
        };

        struct Zone
        {
            uint16 ground1[MAX_BOXES];
            uint16 ground2[MAX_BOXES];
            uint16 fly[MAX_BOXES];
        };

        struct Item
        {
            uint8 type;
            uint8 roomIndex;
            vec3s pos;
            int8 angleY;
            uint8 intensity;
            uint16 flags;

            Item() {}
            
            Item(const LevelPC::Item &item)
            {
                type = uint8(item.type);
                roomIndex = uint8(item.roomIndex);
                pos.x = int16(item.pos.x);
                pos.y = int16(item.pos.y);
                pos.z = int16(item.pos.z);
                angleY = item.angleY / 0x4000;
                intensity = item.intensity >> 5;
                flags = item.flags;
            }
        };

        int16 roomsCount;
        int16 floorsCount;
        int16 boxesCount;
        int16 overlapsCount;
        int16 itemsCount;
         
        Room rooms[MAX_ROOMS];
        FloorData floors[MAX_FLOORS];
        Box boxes[MAX_BOXES];
        Overlap overlaps[MAX_OVERLAPS];
        Zone zones[2];
        Item items[MAX_ITEMS];

        LevelWAD() {}

        LevelWAD(const LevelPC &level)
        {
            roomsCount = int16(level.roomsCount);
            for (int32 i = 0; i < level.roomsCount; i++)
            {
                rooms[i] = level.rooms[i];
            }

            floorsCount = int16(level.floorsCount);
            memcpy(floors, level.floors, floorsCount * sizeof(floors[0]));

            boxesCount = int16(level.boxesCount);
            for (int32 i = 0; i < level.boxesCount; i++)
            {
                boxes[i] = level.boxes[i];
            }

            overlapsCount = int16(level.overlapsCount);
            memcpy(overlaps, level.overlaps, overlapsCount * sizeof(overlaps[0]));

            for (int32 i = 0; i < 2; i++)
            {
                memcpy(zones[i].ground1, level.zones[i].ground1, level.boxesCount * sizeof(uint16));
                memcpy(zones[i].ground2, level.zones[i].ground2, level.boxesCount * sizeof(uint16));
                memcpy(zones[i].fly, level.zones[i].fly, level.boxesCount * sizeof(uint16));
            }

            itemsCount = int16(level.itemsCount);
            for (int32 i = 0; i < itemsCount; i++)
            {
                LevelPC::Item item = level.items[i];
                const LevelPC::Room &room = level.rooms[item.roomIndex];

                item.pos.x -= room.info.x;
                item.pos.z -= room.info.z;

                items[i] = item;
            }
        }
    };

    struct Node
    {
        uint32 flags;
        vec3i pos;
    };

    struct NodeList
    {
        int32 count;
        Node nodes[MAX_NODES];
    };

    struct Model
    {
        uint16 count;
        uint16 start;
        uint16 nodeIndex;
        uint16 frameIndex;
        uint16 animIndex;
    };

    Array<Texture> textures;
    Array<Tile24> tiles;
    Array<NodeList> nodeLists;


    Model* models[MAX_ITEMS];

    bool itemsUsed[MAX_ITEMS];

    LevelWAD* levels[MAX_LEVELS];

    WAD()
    {
        memset(models, 0, sizeof(models));
        memset(itemsUsed, 0, sizeof(itemsUsed));
    }

    int32 addTexture(Texture* tex)
    {
        int32 index = textures.find(tex);
        if (index != -1) {
            delete tex;
            return index;
        }
        return textures.add(tex);
    }

    void addLevel(int32 index, const LevelPC &level)
    {
        int32 texId = 0;

    // textures
        for (int32 i = 0; i < level.objectTexturesCount; i++)
        {
            LevelPC::ObjectTexture* objTex = level.objectTextures + i;
            Tile *tile = level.tiles + (objTex->tile & 0x3FFF);

            int32 minX = MIN(MIN(objTex->x0, objTex->x1), objTex->x2);
            int32 minY = MIN(MIN(objTex->y0, objTex->y1), objTex->y2);
            int32 maxX = MAX(MAX(objTex->x0, objTex->x1), objTex->x2);
            int32 maxY = MAX(MAX(objTex->y0, objTex->y1), objTex->y2);

            if (objTex->isQuad)
            {
                minX = MIN(minX, objTex->x3);
                minY = MIN(minY, objTex->y3);
                maxX = MAX(maxX, objTex->x3);
                maxY = MAX(maxY, objTex->y3);
            }

            Texture* tex = new Texture(tile, &level.palette, minX, minY, maxX, maxY);
            tex->id = texId++;
            addTexture(tex);
        }

    // sprites
        for (int32 i = 0; i < level.spriteTexturesCount; i++)
        {
            LevelPC::SpriteTexture* sprTex = level.spriteTextures + i;
            Tile *tile = level.tiles + (sprTex->tile & 0x3FFF);

            Texture* tex = new Texture(tile, &level.palette, sprTex->u, sprTex->v, sprTex->u + sprTex->w, sprTex->v + sprTex->h);
            tex->id = texId++;
            addTexture(tex);
        }

        levels[index] = new LevelWAD(level);

        for (int32 i = 0; i < level.modelsCount; i++)
        {
            const LevelPC::Model &model = level.models[i];

            if (models[model.type] == NULL)
            {
                models[model.type] = new Model();
                models[model.type]->nodeIndex = 0;
                models[model.type]->count = 0;
            }

            Model *m = models[model.type];

            int32 newIndex = addNodes((LevelPC::Node*)(level.nodesData + model.nodeIndex), model.count - 1);
            if (newIndex != 0) {
                models[model.type]->nodeIndex = newIndex;
            }

            if (!(m->count == 0 || m->count == model.count))
                printf("aaa");

            m->count = model.count;
            m->start = 0; // TODO
            m->frameIndex = 0; // TODO
            m->animIndex = 0; // TODO


            // TODO cutscenes and some other objects have variable nodes between levels
        }

        for (int32 i = 0; i < level.itemsCount; i++)
        {
            itemsUsed[level.items[i].type] = true;
        }

        for (int32 i = 0; i < level.spriteSequencesCount; i++)
        {
            const LevelPC::SpriteSequence &s = level.spriteSequences[i];
            itemsUsed[s.type] = true;
        }

        for (int32 i = 0; i < level.staticMeshesCount; i++)
        {
            const LevelPC::StaticMesh &m = level.staticMeshes[i];
            // TODO collect unique static meshes
        }
    }

    int32 addNodes(const LevelPC::Node *nodes, int32 count)
    {
        if (count == 0)
            return 0;

        int32 index = 0;

        for (int32 i = 0; i < nodeLists.count; i++)
        {
            if (nodeLists[i]->count == count)
            {
                bool equal = true;

                for (int32 j = 0; j < nodeLists[i]->count; j++)
                {
                    const Node &n = nodeLists[i]->nodes[j];
                    if (n.flags != nodes[j].flags ||
                        n.pos.x != nodes[j].pos.x ||
                        n.pos.y != nodes[j].pos.y ||
                        n.pos.z != nodes[j].pos.z)
                    {
                        equal = false;
                        break;
                    }
                }

                if (equal) {
                    return index;
                }
            }

            index += nodeLists[i]->count;
        }

        NodeList* list = new NodeList();
        list->count = count;
        for (int32 i = 0; i < count; i++)
        {
            Node &n = list->nodes[i];
            n.flags = nodes[i].flags;
            n.pos.x = nodes[i].pos.x;
            n.pos.y = nodes[i].pos.y;
            n.pos.z = nodes[i].pos.z;
        }

        nodeLists.add(list);

        return index;
    }

    void packTiles()
    {
        textures.sort();

        for (int32 i = 0; i < textures.count; i++)
        {
            WAD::Texture* tex = textures[i];

            bool placed = false;

            for (int32 j = 0; j < tiles.count; j++)
            {
                if (tiles[j]->root->insert(tex)) {
                    placed = true;
                    break;
                }
            }

            if (!placed)
            {
                Tile24* tile = new Tile24();
                tiles.add(tile);
                if (!tile->root->insert(tex))
                {
                    printf("Can't pack texture %d x %d", tex->width, tex->height);
                    break;
                }
            }
        }

        uint8* data = new uint8[256 * 256 * 3 * tiles.count];

        for (int32 i = 0; i < tiles.count; i++)
        {
            tiles[i]->fill(data + i * 256 * 256 * 3);
        }
        saveBitmap("tiles.bmp", data, 256, 256 * tiles.count);

        delete[] data;
    }
};

#define MAX_TRACKS 256

#define ALIGN(x, a) (((x) + ((a) - 1)) & ~((a) - 1))

void pack_tracks(const char* dir)
{
    WIN32_FIND_DATA fd;
    HANDLE h = FindFirstFile(dir, &fd);

    if (h == INVALID_HANDLE_VALUE)
        return;

    struct Track {
        int32 size;
        char* data;
    };
    Track tracks[MAX_TRACKS];
    memset(tracks, 0, sizeof(tracks));

    char buf[256];

    do
    {
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            const char* src = fd.cFileName;
            char* dst = buf;

            while (*src)
            {
                if (*src >= '0' && *src <= '9')
                {
                    *dst++ = *src;
                }
                src++;
            }
            *dst++ = 0;

            int32 index = atoi(buf);

            if (index != 0)
            {
                strcpy(buf, dir);
                buf[strlen(buf) - 5] = 0;
                strcat(buf, fd.cFileName);

                FILE* f = fopen(buf, "rb");

                if (!f)
                    continue;

                fseek(f, 0, SEEK_END);
                int32 size = ftell(f);
                fseek(f, 0, SEEK_SET);
                tracks[index].data = new char[size + 4];
                fread(tracks[index].data, 1, size, f);
                fclose(f);

                tracks[index].size = ALIGN(*((int32*)tracks[index].data + 2), 4) - 4;

                ASSERT(tracks[index].size % 4 == 0);
            }
        }
    }
    while (FindNextFile(h, &fd));
    FindClose(h);

    FILE* f = fopen("../data/TRACKS.IMA", "wb");

    int32 offset = MAX_TRACKS * (4 + 4);

    for (int32 i = 0; i < MAX_TRACKS; i++)
    {
        if (tracks[i].size == 0) {
            int32 zero = 0;
            fwrite(&zero, 4, 1, f);
        } else {
            fwrite(&offset, 4, 1, f);
        }
        fwrite(&tracks[i].size, 4, 1, f);
        offset += tracks[i].size;
    }

    for (int32 i = 0; i < MAX_TRACKS; i++)
    {
        if (tracks[i].size == 0)
            continue;
        fwrite((uint8*)tracks[i].data + 16, 1, tracks[i].size, f);
        delete[] tracks[i].data;
    }

    fclose(f);
}

void convertTracks3DO(const char* inDir, const char* outDir)
{
    WIN32_FIND_DATA fd;
    HANDLE h = FindFirstFile(inDir, &fd);

    if (h == INVALID_HANDLE_VALUE)
        return;

    char buf[256];

    do
    {
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            const char* src = fd.cFileName;
            char* dst = buf;

            while (*src)
            {
                if (*src >= '0' && *src <= '9')
                {
                    *dst++ = *src;
                }
                src++;
            }
            *dst++ = 0;

            int32 index = atoi(buf);

            if (index != 0)
            {
                strcpy(buf, inDir);
                buf[strlen(buf) - 1] = 0;
                strcat(buf, fd.cFileName);

                char cmdline[256];
                sprintf(cmdline, "C:\\Program Files\\ffmpeg\\ffmpeg.exe -y -i \"%s\" -ac 1 -ar 22050 -acodec pcm_s16be %s\\%d.aiff", buf, outDir, index);

                launchApp(cmdline);
                /* TODO SDXC encoder
                FILE* f = fopen("tmp.wav", "rb");
                ASSERT(f);

                fseek(f, 0, SEEK_END);
                int32 size = ftell(f);
                fseek(f, 0, SEEK_SET);
                uint8* samples = new uint8[size];
                fread(samples, 1, size, f);
                fclose(f);

                int32 numSamples = size / sizeof(short);

                int32 outputSize = (size + 3) / 4;
                uint8* output = new uint8[outputSize];
                memset(output, 0, outputSize);

                BlockADDVIEncode(output, (short*)samples, numSamples, 1); // mono block

                sprintf(buf, "%s\\%d.S", outDir, index);
                f = fopen(buf, "wb");
                ASSERT(f);
                fwrite(&numSamples, sizeof(numSamples), 1, f);
                fwrite(output, 1, outputSize, f);
                fclose(f);

                delete[] output;
                delete[] samples;
                */
            }
        }
    }
    while (FindNextFile(h, &fd));
    FindClose(h);
}

#define SQR(x) ((x) * (x))

void convertScreen(const char* name, const Palette &pal)
{
    char path[256];
    sprintf(path, "screens/%s.bmp", name);

    int32 width, height, bpp;
    uint32* data = (uint32*)loadBitmap(path, &width, &height, &bpp);

    ASSERT(data);
    ASSERT(width == 240 && height == 160 && bpp == 32);

    uint32* uniqueColors = new uint32[width * height];
    uint32 count = 0;

    for (int32 i = 0; i < width * height; i++)
    {
        uint32 c = data[i];

        int32 index = -1;

        for (int32 j = 0; j < count; j++)
        {
            if (uniqueColors[j] == c) {
                index = j;
                break;
            }
        }

        if (index == -1) {
            index = count++;
            uniqueColors[index] = c;
        }

        data[i] = index;
    }

    for (int32 i = 0; i < count; i++)
    {
        uint32 c = uniqueColors[i];

        int32 cr = (c >> 16) & 0xFF;
        int32 cg = (c >> 8) & 0xFF;
        int32 cb = c & 0xFF;

        float dist = 256 * 256 * 256;
        int32 index = 0;

        for (int32 j = 0; j < 256; j++)
        {
            int32 r = pal.colors[j * 3 + 0] << 2;
            int32 g = pal.colors[j * 3 + 1] << 2;
            int32 b = pal.colors[j * 3 + 2] << 2;

            float d = sqrtf(float(SQR(cr - r) + SQR(cg - g) + SQR(cb - b)));
            if (d < dist)
            {
                dist = d;
                index = j;
            }
        }
    
        uniqueColors[i] = index;
    }

    uint8* indices = new uint8[width * height];
    for (int32 i = 0; i < width * height; i++)
    {
        indices[i] = uniqueColors[data[i]];
    }

    sprintf(path, "../data/%s.SCR", name);

    FILE *f = fopen(path, "wb");
    fwrite(indices, 1, width * height, f);
    fclose(f);

    delete[] data;
    delete[] uniqueColors;
    delete[] indices;
}

uint32 palDump[32][256];

int main()
{
    memset(palDump, 0, sizeof(palDump));

    //pack_tracks("tracks/conv_demo/*.ima"); return 0;

    for (int32 i = 0; i < MAX_LEVELS; i++)
    {
        char path[64];
        sprintf(path, "levels/%s.PHD", levelNames[i]);
        levels[i] = new LevelPC(path);

        if (strcmp(levelNames[i], "TITLE") == 0) {
            convertScreen("TITLE", levels[i]->palette);
        }

        for (int32 j = 0; j < 256; j++)
        {
            int32 r = levels[i]->palette.colors[j * 3 + 0] << 2;
            int32 g = levels[i]->palette.colors[j * 3 + 1] << 2;
            int32 b = levels[i]->palette.colors[j * 3 + 2] << 2;
            palDump[i][j] = b | (g << 8) | (r << 16) | (0xFF << 24);
        }

        levels[i]->generateLODs();
        levels[i]->cutData(levelNames[i]);

        sprintf(path, "../data/%s.PKD", levelNames[i]);
        levels[i]->convertGBA(path);

        levels[i]->convert3DO(levelNames[i]);
    }

//    saveBitmap("pal.bmp", (uint8*)palDump, 256, 32, 32);

//    convertTracks3DO("C:\\Projects\\OpenLara\\src\\platform\\gba\\packer\\tracks\\orig\\*", "C:\\Projects\\OpenLara\\src\\platform\\3do\\tracks");

    return 0;
    WAD* wad = new WAD();

    int32 size = 0;
    int32 maxItems = 0;

    int32 maxVertices = 0;

    for (int32 i = 0; i < MAX_LEVELS; i++)
    {
        LevelPC* level = levels[i];

        wad->addLevel(i, *level);

        for (int32 j = 0; j < level->roomsCount; j++)
        {
            WAD::LevelWAD::Room &room = wad->levels[i]->rooms[j];

            size += room.vCount * sizeof(WAD::LevelWAD::Room::Vertex);
            size += room.qCount * sizeof(WAD::LevelWAD::Room::Quad);
            size += room.tCount * sizeof(WAD::LevelWAD::Room::Triangle);
            size += room.sCount * sizeof(WAD::LevelWAD::Room::Sprite);
            size += room.pCount * sizeof(WAD::LevelWAD::Room::Portal);
            size += room.xSectors * room.zSectors * sizeof(room.sectors[0]);
        }

        size += wad->levels[i]->floorsCount * sizeof(WAD::LevelWAD::FloorData);
        size += wad->levels[i]->boxesCount * sizeof(WAD::LevelWAD::Box);
        size += wad->levels[i]->overlapsCount * sizeof(WAD::LevelWAD::Overlap);
        size += wad->levels[i]->boxesCount * sizeof(uint16) * 3 * 2; // zones
        size += wad->levels[i]->itemsCount * sizeof(WAD::LevelWAD::Item);

        size += level->frameDataSize * 2;
    }

    int32 nodes = 0;
    for (int32 i = 0; i < wad->nodeLists.count; i++)
    {
        nodes += wad->nodeLists[i]->count;
    }

    printf("roomsSize: %d bytes %d\n", size, nodes);

    wad->packTiles();

    printf("tiles: %d (%d bytes)\n", wad->tiles.count, wad->tiles.count * 256 * 256);

/*
    int32 texSize = 0;
    for (int32 i = 0; i < wad.textures.count; i++)
    {
        texSize += wad.textures[i]->width * wad.textures[i]->height;

        char texName[64];
        sprintf(texName, "textures/%d.bmp", i);
        wad.textures[i]->save(texName);
    }
*/
    for (int32 i = 0; i < MAX_LEVELS; i++)
    {
        delete levels[i];
    }

    return 0;
}
