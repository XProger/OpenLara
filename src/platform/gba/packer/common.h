#ifndef H_COMMON
#define H_COMMON

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <algorithm>
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

#define SQR(x) ((x) * (x))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define CLAMP(x, a, b) ((x) < (a) ? (a) : ((x) > (b) ? (b) : (x)))
#define ALIGN(x, a) (((x) + ((a) - 1)) & ~((a) - 1))

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

        for (int32 i = 0; i < int32(count); i++)
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

enum LevelID
{
    LVL_TR1_TITLE,
    LVL_TR1_GYM,
    LVL_TR1_1,
    LVL_TR1_2,
    LVL_TR1_3A,
    LVL_TR1_3B,
    LVL_TR1_CUT_1,
    LVL_TR1_4,
    LVL_TR1_5,
    LVL_TR1_6,
    LVL_TR1_7A,
    LVL_TR1_7B,
    LVL_TR1_CUT_2,
    LVL_TR1_8A,
    LVL_TR1_8B,
    LVL_TR1_8C,
    LVL_TR1_10A,
    LVL_TR1_CUT_3,
    LVL_TR1_10B,
    LVL_TR1_CUT_4,
    LVL_TR1_10C,
    LVL_MAX
};

const char* levelNames[LVL_MAX] = {
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
};

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

#define MAX_TRACKS          256
#define MAX_ROOMS           256
#define MAX_MESHES          512
#define MAX_MODELS          256
#define MAX_ANIMS           512
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
#define MAX_ANIM_TEX        128

#define TEX_ATTR_AKILL      0x0001
#define TEX_ATTR_ANIM       0x0002
#define FACE_TEXTURE        0x3FFF

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

    for (uint32 i = 0; i < ihdr.biHeight; i++)
    {
        data -= ihdr.biWidth * ihdr.biBitCount / 8;
        fread(data, ihdr.biWidth * ihdr.biBitCount / 8, 1, f);
    }

    fclose(f);

    return data;
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

#endif
