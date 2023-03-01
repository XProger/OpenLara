#ifndef H_COMMON
#define H_COMMON
//#define STATIC_ITEMS
//#define PROFILING
#ifdef PROFILING
    #define STATIC_ITEMS
    #define PROFILE_FRAMETIME
//    #define PROFILE_SOUNDTIME
#endif

// supported formats
// level
#define LVL_FMT_PHD     (1 << 0)
#define LVL_FMT_PSX     (1 << 1)
#define LVL_FMT_SAT     (1 << 2)
#define LVL_FMT_TR2     (1 << 3)
#define LVL_FMT_TR4     (1 << 4)
#define LVL_FMT_PKD     (1 << 5)
// video
#define FMV_FMT_RPL     (1 << 6)
#define FMV_FMT_STR     (1 << 7)
#define FMV_FMT_MP1     (1 << 8)
// audio
#define SND_FMT_PCM     (1 << 9)
#define SND_FMT_ADPCM   (1 << 10)
#define SND_FMT_IMA     (1 << 11)
#define SND_FMT_VAG     (1 << 12)
#define SND_FMT_XA      (1 << 13)
#define SND_FMT_OGG     (1 << 14)
#define SND_FMT_MP3     (1 << 15)

#define FIXED_SHIFT     14

#if defined(__WIN32__)
    #define USE_DIV_TABLE
    #define MODEHW
    #define GAPI_GL1

    extern int FRAME_WIDTH;
    extern int FRAME_HEIGHT;
    extern float FRAME_PERSP;

    #define USE_FMT     (LVL_FMT_PHD)

    #define _CRT_SECURE_NO_WARNINGS
    #include <windows.h>
#elif defined(__GBA_WIN__)
    #define USE_DIV_TABLE

    #define MODE4
    #define FRAME_WIDTH  240
    #define FRAME_HEIGHT 160

    #define USE_FMT     (LVL_FMT_PKD)

    #define _CRT_SECURE_NO_WARNINGS
    #include <windows.h>
#elif defined(__GBA__)
    #define USE_DIV_TABLE
    #define ROM_READ
    #define USE_ASM
    #define ALIGNED_LIGHTMAP

    #define MODE4
    #define FRAME_WIDTH  240
    #define FRAME_HEIGHT 160

    #define USE_FMT     (LVL_FMT_PKD)
    #define USE_VRAM_MESH // experimental
    //#define USE_VRAM_ROOM // experimental

    #include <tonc.h>
#elif defined(__NDS__)
    #define USE_DIV_TABLE

    #define MODEHW
    #define FRAME_WIDTH  256
    #define FRAME_HEIGHT 192

    #define USE_FMT     (LVL_FMT_PSX)

    #include <nds.h>
    #include <fat.h>
    #include <filesystem.h>
#elif defined(__TNS__)
    #define USE_DIV_TABLE

    #define MODE13
    #define FRAME_WIDTH  320
    #define FRAME_HEIGHT 240

    #define USE_FMT     (LVL_FMT_PKD)

    #include <os.h>
#elif defined(__DOS__)
    #define USE_DIV_TABLE

    #define MODE13
    #define FRAME_WIDTH  320
    #define FRAME_HEIGHT 200

    #define USE_FMT     (LVL_FMT_PKD)

    #include <stdlib.h>
    #include <stddef.h>
    #include <conio.h>
    #include <dos.h>
    #include <mem.h>
#elif defined(__3DO__)
    #define USE_DIV_TABLE   // 4k of DRAM
    #define CPU_BIG_ENDIAN
    #define USE_ASM
    #define F16_SHIFT 2

    #define MODEHW
    #define FRAME_WIDTH  320
    #define FRAME_HEIGHT 240

    #define USE_FMT     (LVL_FMT_PKD)

    #include <displayutils.h>
    #include <debug.h>
    #include <nodes.h>
    #include <kernelnodes.h>
    #include <list.h>
    #include <folio.h>
    #include <audio.h>
    #include <task.h>
    #include <kernel.h>
    #include <mem.h>
    #include <operamath.h>
    #include <semaphore.h>
    #include <io.h>
    #include <event.h>
    #include <controlpad.h>
    #include <graphics.h>
    #include <celutils.h>
    #include <timerutils.h>
    #include <hardware.h>
#elif defined(__32X__)
    #define USE_DIV_TABLE
    #define CPU_BIG_ENDIAN
    #define ROM_READ
    #define TEX_2PX
    #define ALIGNED_LIGHTMAP
    #define USE_MATRIX_INT16
    #define F16_SHIFT 2

    #define MODE13
    #define FRAME_WIDTH  320
    #define FRAME_HEIGHT 224

    #define USE_FMT     (LVL_FMT_PKD)

    #include "32x.h"

    enum MarsCmd {
        MARS_CMD_NONE = 0,
        MARS_CMD_CLEAR,
        MARS_CMD_FLUSH
    };
#else
    #error unsupported platform
#endif

#ifdef _DEBUG
    #if defined(__WIN32__)
        #include <stdio.h>
        inline void LOG(const char* format, ...)
        {
            char str[1024];
            va_list arglist;
            va_start(arglist, format);
            _vsnprintf(str, 1024, format, arglist);
            va_end(arglist);
            OutputDebugStringA(str);
        }
    #else
        #define LOG(...)    printf(__VA_ARGS__)
    #endif
#else
    #define LOG()
#endif

#if !defined(__3DO__)
    #include <string.h>
#endif

#include "stdio.h" // TODO_3DO armcpp bug?

#include <math.h>
#include <limits.h>

#ifndef USE_FMT
    #define USE_FMT (LVL_FMT_PHD | LVL_FMT_PSX | LVL_FMT_SAT | LVL_FMT_TR2 | LVL_FMT_TR4)
#endif

// Optimization flags =========================================================
#ifdef __GBA__
// hide dead enemies after a while to reduce the number of polygons on the screen
    #define HIDE_CORPSES (30*10) // 10 sec
// replace trap flor geometry by two flat quads in the static state
    #define LOD_TRAP_FLOOR
// disable some plants environment to reduce overdraw of transparent geometry
    #define NO_STATIC_MESH_PLANTS
// the maximum of active enemies
    #define MAX_ENEMIES 3
// visibility distance
    #define VIEW_DIST (10 << 10)
// skip collideSpheres for enemies
    #define FAST_HITMASK
#endif

#ifdef __3DO__
// hide dead enemies after a while to reduce the number of polygons on the screen
    #define HIDE_CORPSES (30*10) // 10 sec
// replace trap flor geometry by two flat quads in the static state
    #define LOD_TRAP_FLOOR
// disable matrix interpolation
    //#define NO_ANIM_LERP
// the maximum navigation iterations per simulation tick
    #define NAV_STEPS 1
// the maximum of active enemies
    #define MAX_ENEMIES 3
// set the maximum number of simultaneously played channels
    #define SND_CHANNELS 4
// visibility distance
    #define VIEW_DIST (10 << 10)
// skip collideSpheres for enemies
    #define FAST_HITMASK
#endif

#ifdef __32X__
// hide dead enemies after a while to reduce the number of polygons on the screen
    #define HIDE_CORPSES (30*10) // 10 sec
// replace trap flor geometry by two flat quads in the static state
    #define LOD_TRAP_FLOOR
// disable some plants environment to reduce overdraw of transparent geometry
    #define NO_STATIC_MESH_PLANTS
// the maximum of active enemies
    #define MAX_ENEMIES 3
// visibility distance
    #define VIEW_DIST (10 << 10)
// skip collideSpheres for enemies
    #define FAST_HITMASK
#endif

#ifndef NAV_STEPS
    #define NAV_STEPS 5
#endif

#ifndef MAX_ENEMIES
    #define MAX_ENEMIES 8
#endif

// ============================================================================

#if defined(_MSC_VER)
    #define X_INLINE    inline
    #define X_NOINLINE  __declspec(noinline)
    #define ALIGN4      __declspec(align(4))
    #define ALIGN8      __declspec(align(8))
    #define ALIGN16     __declspec(align(16))
#elif defined(__WATCOMC__) || defined(__3DO__)
    #define X_INLINE    inline
    #define X_NOINLINE
    #define ALIGN4
    #define ALIGN8
    #define ALIGN16
#else
    #define X_INLINE    __attribute__((always_inline)) inline
    #define X_NOINLINE  __attribute__((noinline))
    #define ALIGN4      __attribute__((aligned(4)))
    #define ALIGN8      __attribute__((aligned(8)))
    #define ALIGN16     __attribute__((aligned(16)))
#endif

#if defined(__3DO__)
typedef size_t             intptr_t;
typedef uint32             divTableInt;
#else
typedef signed char        int8;
typedef signed short       int16;
#if !defined(__NDS__)
typedef signed int         int32;
#endif
typedef unsigned char      uint8;
typedef unsigned short     uint16;
#if !defined(__NDS__)
typedef unsigned int       uint32;
#endif
typedef uint16             divTableInt;
#endif

#if defined(__32X__)
    typedef int8 ColorIndex;
#else
    typedef uint8 ColorIndex;
#endif

#define ADDR_ALIGN4(x)  ((uint8*)x += ((intptr_t(x) + 3) & ~3) - intptr_t(x))

//#include <new>
inline void* operator new(size_t, void *ptr)
{
    return ptr;
}

#if defined(__3DO__) || defined(__32X__)
X_INLINE int32 abs(int32 x) {
    return (x >= 0) ? x : -x;
}
#endif

#if defined(__GBA__) || defined(__NDS__) || defined(__32X__)
    #define int2str(x,str) itoa(x, str, 10)
#elif defined(__3DO__)
    #define int2str(x,str) sprintf(str, "%d", x)
#elif defined(__TNS__)
    #define int2str(x,str) __itoa(x, str, 10)
#else
    #define int2str(x,str) _itoa(x, str, 10)
#endif

#ifdef __GBA__
    #define ARM_CODE __attribute__((target("arm")))
#else
    #define ARM_CODE
    #define THUMB_CODE
    #define IWRAM_DATA
    #define EWRAM_DATA
    #define EWRAM_BSS
    #define IWRAM_CODE
    #define EWRAM_CODE
#endif

#if defined(__WIN32__) || defined(__GBA_WIN__)
    #define ASSERT(x) { if (!(x)) { DebugBreak(); } }
    #define STATIC_ASSERT(x) typedef char static_assert_##__COUNTER__[(x) ? 1 : -1]
#else
    #define ASSERT(x)
    #define STATIC_ASSERT(x)
#endif

#if defined(__GBA_WIN__)
    extern uint16 fb[FRAME_WIDTH * FRAME_HEIGHT];
#elif defined(__GBA__)
    extern uint32 fb;
#elif defined(__TNS__)
    extern uint16 fb[FRAME_WIDTH * FRAME_HEIGHT];
#elif defined(__DOS__)
    extern uint16 fb[FRAME_WIDTH * FRAME_HEIGHT];
#endif

#define STATIC_MESH_FLAG_NO_COLLISION   1
#define STATIC_MESH_FLAG_VISIBLE        2
#define MAX_STATIC_MESH_RADIUS          (5 * 1024)

extern int32 fps;

#if defined(USE_VRAM_MESH) || defined(USE_VRAM_ROOM)
extern uint8* vramPtr;
#endif

#ifndef F16_SHIFT
    #define F16_SHIFT 0
#endif

#ifdef USE_MATRIX_INT16
    #define MATRIX_FIXED_SHIFT FIXED_SHIFT
#endif

#ifndef MATRIX_FIXED_SHIFT
   #define MATRIX_FIXED_SHIFT 0
#endif

#define SND_MAX_DIST    (8 * 1024)

#ifndef SND_CHANNELS
    #define SND_CHANNELS 6
#endif

#define SND_FIXED_SHIFT 8
#define SND_VOL_SHIFT   6
#define SND_PITCH_SHIFT 7

#if defined(__WIN32__)
    #define SND_SAMPLES      1024
    #define SND_OUTPUT_FREQ  22050
    #define SND_SAMPLE_FREQ  22050
    #define SND_ENCODE(x)    ((x) + 128)
    #define SND_DECODE(x)    ((x) - 128)
    #define SND_MIN          -128
    #define SND_MAX          127
#elif defined(__GBA_WIN__)
    #define SND_SAMPLES      1024
    #define SND_OUTPUT_FREQ  22050
    #define SND_SAMPLE_FREQ  22050
    #define SND_ENCODE(x)    ((x) + 128)
    #define SND_DECODE(x)    ((x) - 128)
    #define SND_MIN          -128
    #define SND_MAX          127
#elif defined(__GBA__)
    #define SND_SAMPLES      176
    #define SND_OUTPUT_FREQ  10512
    #define SND_SAMPLE_FREQ  22050
    #define SND_ENCODE(x)    (x)
    #define SND_DECODE(x)    ((x) - 128)
    #define SND_MIN          -128
    #define SND_MAX          127
#elif defined(__DOS__)
    #define SND_SAMPLES      1024
    #define SND_OUTPUT_FREQ  11025
    #define SND_SAMPLE_FREQ  11025
    #define SND_ENCODE(x)    ((x) + 128)
    #define SND_DECODE(x)    ((x) - 128)
    #define SND_MIN          -128
    #define SND_MAX          127
#elif defined(__3DO__)
    #define SND_SAMPLES      1024
    #define SND_OUTPUT_FREQ  11025
    #define SND_SAMPLE_FREQ  11025
    #define SND_ENCODE(x)    ((x) + 128)
    #define SND_DECODE(x)    ((x) - 128)
    #define SND_MIN          -128
    #define SND_MAX          127
#endif

#if defined(__3DO__)
    #define MAX_VERTICES (1024 + 32) // for mesh (max = LEVEL10A room:58)
#elif defined(__GBA__)
    #define MAX_VERTICES (5*1024) // for frame (max is 8191 - check the assumption in flush.s)
#else
    #define MAX_VERTICES (5*1024) // for frame
#endif

#define MAX_UPDATE_FRAMES   10
#define MAX_PLAYERS         1 // TODO 2 players for non-potato platforms
#define MAX_SPHERES         32
#define MAX_MATRICES        8
#define MAX_ROOMS           139 // LEVEL7A
#define MAX_ITEMS           256
#define MAX_MODELS          ITEM_MAX
#define MAX_MESHES          512
#define MAX_STATIC_MESHES   50
#define MAX_CAMERAS         16
#define MAX_BOXES           1024
#define MAX_TEXTURES        1536
#define MAX_SPRITES         180
#define MAX_FACES           1920
#define MAX_ROOM_LIST       32
#define MAX_PORTALS         16
#define MAX_CAUSTICS        32
#define MAX_RAND_TABLE      32
#define MAX_DYN_SECTORS     (1024*3)
#define MAX_SAMPLES         180

#ifndef VIEW_DIST
    #define VIEW_DIST (1024 * 10)
#endif

#define FOV_SHIFT       3
#define FOG_SHIFT       1
#define FOG_MAX         VIEW_DIST
#define FOG_MIN         (FOG_MAX - 2048)
#define VIEW_MIN_F      (64 << FIXED_SHIFT)
#define VIEW_MAX_F      (VIEW_DIST << FIXED_SHIFT)

#define MESH_SHIFT      2

#define TEX_ATTR_AKILL  1

#define NOT_ENEMY       -0x4000     // default hp for non enemies
#define NO_ROOM         0xFF
#define NO_MODEL        0xFF
#define NO_BOX          0xFFFF
#define NO_FLOOR        -127
#define WALL            (NO_FLOOR * 256)

#define ANGLE_360       0x10000
#define ANGLE_0         0
#define ANGLE_1         (ANGLE_360 / 360)
#define ANGLE_45        (ANGLE_360 / 8)      // != 45 * ANGLE_1 !!!
#define ANGLE_90        (ANGLE_360 / 4)      // != 90 * ANGLE_1 !!!
#define ANGLE_180      -(ANGLE_360 / 2)      // INT16_MIN
#define ANGLE(x)        ((x) * ANGLE_1)
#define ANGLE_SHIFT_45  13
#define ANGLE_SHIFT_90  14
#define ANGLE_SHIFT_180 15

#define LARA_MAX_HEALTH         1000
#define LARA_MAX_OXYGEN         1800    // TODO +30 sec for TR5

#define X_CLAMP(x, a, b) ((x) < (a) ? (a) : ((x) > (b) ? (b) : (x)))
#define X_MIN(a,b)       ((a) < (b) ? (a) : (b))
#define X_MAX(a,b)       ((a) > (b) ? (a) : (b))
#define X_SQR(x)         ((x) * (x))
#define X_COUNT(x)       int32(sizeof(x) / sizeof(x[0]))

#define X_ROTX(x,y,s,c)  (((x) * (c) - (y) * (s)) >> FIXED_SHIFT)
#define X_ROTY(x,y,s,c)  (((y) * (c) + (x) * (s)) >> FIXED_SHIFT)
#define X_ROTXY(x,y,s,c) {\
    int32 _x = X_ROTX(x,y,s,c);\
    int32 _y = X_ROTY(x,y,s,c);\
    x = _x;\
    y = _y;\
}

#define DP43(ax,ay,az,aw,bx,by,bz)  (ax * bx + ay * by + az * bz + (aw << MATRIX_FIXED_SHIFT))
#define DP33(ax,ay,az,bx,by,bz)     (ax * bx + ay * by + az * bz)

#ifdef USE_DIV_TABLE
    #define DIV_TABLE_SIZE   1025 // to compare with #1024 without extra LDR
    #define FixedInvS(x)     ((x < 0) ? -divTable[abs(x)] : divTable[x])
    #define FixedInvU(x)     divTable[x]
    extern divTableInt divTable[DIV_TABLE_SIZE];

    #define GET_FRAME_T(x,n) (FixedInvU(n) * x)
#else
    #define GET_FRAME_T(x,n) ((x << 16) / n)
#endif

#define OT_SHIFT        4
#define OT_SIZE         ((VIEW_MAX_F >> (FIXED_SHIFT + OT_SHIFT)) + 1)

// system keys (keys)
enum InputKey {
    IK_NONE     = 0,
    IK_UP       = (1 << 0),
    IK_RIGHT    = (1 << 1),
    IK_DOWN     = (1 << 2),
    IK_LEFT     = (1 << 3),
    IK_A        = (1 << 4),
    IK_B        = (1 << 5),
    IK_C        = (1 << 6),
    IK_X        = (1 << 7),
    IK_Y        = (1 << 8),
    IK_Z        = (1 << 9),
    IK_L        = (1 << 10),
    IK_R        = (1 << 11),
    IK_LT       = (1 << 12),
    IK_RT       = (1 << 13),
    IK_START    = (1 << 14),
    IK_SELECT   = (1 << 15)
};

// action keys (ItemObj::input)
enum InputState {
    IN_LEFT   = (1 << 1),
    IN_RIGHT  = (1 << 2),
    IN_UP     = (1 << 3),
    IN_DOWN   = (1 << 4),
    IN_JUMP   = (1 << 5),
    IN_WALK   = (1 << 6),
    IN_ACTION = (1 << 7),
    IN_WEAPON = (1 << 8),
    IN_LOOK   = (1 << 9),
    IN_START  = (1 << 10),
    IN_SELECT = (1 << 11)
};

struct vec3s {
    int16 x, y, z;

    X_INLINE static vec3s create(int16 x, int16 y, int16 z) {
        vec3s r;
        r.x = x;
        r.y = y;
        r.z = z;
        return r;
    }

    X_INLINE vec3s  operator + (const vec3s &v) const { return create(x + v.x, y + v.y, z + v.z); }
    X_INLINE vec3s  operator - (const vec3s &v) const { return create(x - v.x, y - v.y, z - v.z); }
    X_INLINE bool   operator == (const vec3s &v) { return x == v.x && y == v.y && z == v.z; }
    X_INLINE bool   operator != (const vec3s &v) { return x != v.x || y != v.y || z != v.z; }
    X_INLINE vec3s& operator += (const vec3s &v) { x += v.x; y += v.y; z += v.z; return *this; }
    X_INLINE vec3s& operator -= (const vec3s &v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
};

#define _vec3s(x,y,z) vec3s::create(x, y, z)

struct vec4s {
    int16 x, y, z, w;
};

struct vec3i {
    int32 x, y, z;

    X_INLINE static vec3i create(int32 x, int32 y, int32 z) {
        vec3i r;
        r.x = x;
        r.y = y;
        r.z = z;
        return r;
    }

    X_INLINE vec3i  operator + (const vec3i &v) const { return create(x + v.x, y + v.y, z + v.z); }
    X_INLINE vec3i  operator - (const vec3i &v) const { return create(x - v.x, y - v.y, z - v.z); }
    X_INLINE vec3i  operator * (int32 s) const { return create(x * s, y * s, z * s); }
    X_INLINE bool   operator == (const vec3i &v) const { return x == v.x && y == v.y && z == v.z; }
    X_INLINE bool   operator != (const vec3i &v) const { return x != v.x || y != v.y || z != v.z; }
    X_INLINE vec3i& operator += (const vec3i &v) { x += v.x; y += v.y; z += v.z; return *this; }
    X_INLINE vec3i& operator -= (const vec3i &v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
    X_INLINE vec3i& operator *= (int32 s) { x *= s; y *= s; z *= s; return *this; }
};

#define _vec3i(x,y,z) vec3i::create(x, y, z)

struct vec4i {
    int32 x, y, z, w;

    X_INLINE int32& operator [] (int32 index) const {
        ASSERT(index >= 0 && index <= 3);
        return ((int32*)this)[index];
    }
};

struct Matrix
{
#if defined(__3DO__)
    int32 e00, e10, e20;
    int32 e01, e11, e21;
    int32 e02, e12, e22;
    int32 e03, e13, e23;
#elif defined(USE_MATRIX_INT16)
    int16 e00, e01, e02;
    int16 e10, e11, e12;
    int16 e20, e21, e22;
    int16 e03, e13, e23;
#else
    int32 e00, e01, e02, e03;
    int32 e10, e11, e12, e13;
    int32 e20, e21, e22, e23;
#endif
};

struct RoomQuad
{
#if defined(__3DO__)
    uint32 flags;
    uint16 indices[4];
#elif defined(__32X__)
    uint32 flags;
    int8 indices[4];
#else
    int8 indices[4];
    uint16 flags;
    uint16 padding;
#endif
};

struct RoomTriangle
{
#if defined(__3DO__)
    uint32 flags;
    uint16 indices[4];
#elif defined(__32X__)
    uint16 flags;
    uint16 indices[3];
#else
    uint16 indices[3];
    uint16 flags;
#endif
};

struct MeshQuad
{
#if defined(__3DO__)
    uint32 flags;
    uint32 indices;
#elif defined(__32X__)
    uint16 flags;
    uint8  indices[4];
#else
    int8 indices[4];
    uint16 flags;
    uint16 padding;
#endif
};

struct MeshTriangle
{
#if defined(__3DO__)
    uint32 flags;
    uint32 indices;
#elif defined(__32X__)
    uint16 flags;
    uint8  indices[4];
#else
    int8 indices[4];
    uint16 flags;
    uint16 padding;
#endif
};

struct RectMinMax
{
    int32 x0;
    int32 y0;
    int32 x1;
    int32 y1;

    RectMinMax() {}
    RectMinMax(int32 x0, int32 y0, int32 x1, int32 y1) : x0(x0), y0(y0), x1(x1), y1(y1) {}
};

union TexCoord
{
    struct { uint16 v, u; } uv;
    uint32 t;
};

#ifdef __3DO__
struct Face
{
    uint32   ccb_Flags;
    Face*    ccb_NextPtr;
    CelData* ccb_SourcePtr;
    void*    ccb_PLUTPtr;
    Coord    ccb_XPos;
    Coord    ccb_YPos;
    int32    ccb_HDX;
    int32    ccb_HDY;
    int32    ccb_VDX;
    int32    ccb_VDY;
    int32    ccb_HDDX;
    int32    ccb_HDDY;
    uint32   ccb_PIXC;
    // TODO use 1x1 textures instead of colored faces to remove preamble words (8 bytes per face - 15k)
    uint32   ccb_PRE0;
    uint32   ccb_PRE1;
    //int32    ccb_Width;
    //int32    ccb_Height;
};

    #define BLOCK_SIZE_DRAM     (32 * 1024)
    #define BLOCK_SIZE_VRAM     (16 * 1024)
    #define BLOCK_SIZE_CD       (2 * 1024)

    #define SND_BUFFER_SIZE     (4 * BLOCK_SIZE_CD)
    #define SND_BUFFERS         4

    #define MAX_RAM_LVL         (BLOCK_SIZE_DRAM * 29) // 34 for LEVEL10C! >_<
    #define MAX_RAM_TEX         (BLOCK_SIZE_VRAM * 44)
    #define MAX_RAM_CEL         (MAX_FACES * sizeof(Face))
    #define MAX_RAM_SND         (SND_BUFFERS * SND_BUFFER_SIZE)

    extern void* RAM_LVL;
    extern void* RAM_TEX;
    extern void* RAM_CEL;
    extern void* RAM_SND;
#else
struct Face
{
    uint32 flags;
    Face* next;
    int16 indices[4];
};
#endif

struct AABBs
{
    int16 minX;
    int16 maxX;
    int16 minY;
    int16 maxY;
    int16 minZ;
    int16 maxZ;

    X_INLINE AABBs() {}
    X_INLINE AABBs(int16 minX, int16 maxX, int16 minY, int16 maxY, int16 minZ, int16 maxZ) :
        minX(minX), maxX(maxX), minY(minY), maxY(maxY), minZ(minZ), maxZ(maxZ) {}

    X_INLINE vec3i getCenter() const {
        return _vec3i((maxX + minX) >> 1, (maxY + minY) >> 1, (maxZ + minZ) >> 1);
    }
};

struct AABBi
{
    int32 minX;
    int32 maxX;
    int32 minY;
    int32 maxY;
    int32 minZ;
    int32 maxZ;

    X_INLINE AABBi() {}
    X_INLINE AABBi(const AABBs &b) :
        minX(b.minX), maxX(b.maxX), minY(b.minY), maxY(b.maxY), minZ(b.minZ), maxZ(b.maxZ) {}
    X_INLINE AABBi(int32 minX, int32 maxX, int32 minY, int32 maxY, int32 minZ, int32 maxZ) :
        minX(minX), maxX(maxX), minY(minY), maxY(maxY), minZ(minZ), maxZ(maxZ) {}

    X_INLINE vec3i getCenter() const {
        return _vec3i((maxX + minX) >> 1, (maxY + minY) >> 1, (maxZ + minZ) >> 1);
    }
};

struct Sphere
{
    vec3i center;
    int32 radius;
};

struct Room;

struct RoomVertex
{
#if defined(__3DO__)
    uint16 xyz565;
#elif defined(__GBA__) || defined(__GBA_WIN__) || defined(__32X__)
    uint8 x, y, z, g;
#else
    uint8 x, y, z, g;
#endif
};

struct RoomSprite
{
    vec3s pos;
    uint8 g;
    uint8 index;
};

struct MeshVertex
{
    int16 x, y, z;
};

struct Portal
{
    uint16 roomIndex;
    vec3s n;
    vec3s v[4];
};

struct Sector
{
    uint16 floorIndex;
    uint16 boxIndex;
    uint8  roomBelow;
    int8   floor;
    uint8  roomAbove;
    int8   ceiling;

    const Sector* getSectorBelow(int32 posX, int32 posZ) const;
    const Sector* getSectorAbove(int32 posX, int32 posZ) const;
    int32 getFloor(int32 x, int32 y, int32 z) const;
    int32 getCeiling(int32 x, int32 y, int32 z) const;
    Room* getNextRoom() const;
    void getTriggerFloorCeiling(int32 x, int32 y, int32 z, int32* floor, int32* ceiling) const;
};

struct Light
{
    vec3s pos;
    uint8 radius;
    uint8 intensity;
};

#define STATIC_MESH_ID(flags)           ((flags) & 0x3F)
#define STATIC_MESH_QUADRANT(flags)     (((flags) >> 6) & 3)
#define STATIC_MESH_ROT(flags)          ((STATIC_MESH_QUADRANT(flags) - 2) * ANGLE_90)
#define STATIC_MESH_INTENSITY(flags)    ((((flags) >> 8) & 0xFF) << 5)

struct RoomMesh
{
    uint32 xy; // (x << 16) | y
    uint32 zf; // (z << 16) | (intensity << 8) | flags
};

struct ItemObj;

struct RoomData
{
    const RoomQuad* quads;
    const RoomTriangle* triangles;
    const RoomVertex* vertices;
    const RoomSprite* sprites;
    const Portal* portals;
    const Sector* sectors;
    const Light* lights;
    const RoomMesh* meshes;
};

#define ROOM_FLAG_WATER(x) ((x) & 1)

struct RoomInfo
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

    RoomData data;
};

struct CollisionInfo;

struct Room {
    ItemObj* firstItem;
    const RoomInfo* info;
    const Sector* sectors; // == info->sectors (const) by default (see roomModify) 

    RoomData data;

    RectMinMax clip;
    uint8 _reserved;
    bool visible;

    void modify();
    void reset();

    void add(ItemObj* item);
    void remove(ItemObj* item);

    const Sector* getSector(int32 x, int32 z) const;
    const Sector* getWaterSector(int32 x, int32 z) const;
    Room* getRoom(int32 x, int32 y, int32 z);
    bool collideStatic(CollisionInfo &cinfo, const vec3i &p, int32 height);
    bool checkPortal(const Portal* portal);

    Room** addVisibleRoom(Room** list);
    Room** addNearRoom(Room** list, int32 x, int32 y, int32 z);
    Room** getNearRooms(const vec3i &pos, int32 radius, int32 height);
    Room** getAdjRooms();
    Room** getVisibleRooms();
};

enum NodeFlag {
    NODE_FLAG_POP  = (1 << 0),
    NODE_FLAG_PUSH = (1 << 1),
    NODE_FLAG_ROTX = (1 << 2),
    NODE_FLAG_ROTY = (1 << 3),
    NODE_FLAG_ROTZ = (1 << 4)
};

struct ModelNode {
    vec3s pos;
    uint16 flags;
};

struct Model {
    uint8 type;
    int8 count;
    uint16 start;
    uint16 nodeIndex;
    uint16 animIndex;
};

#define FILE_MODEL_SIZE (sizeof(Model) - 2) // -padding

struct Mesh {
    vec3s center;
    int16 radius;
    uint16 intensity;
    uint8 vCount;
    uint8 hasNormals;
    int16 rCount;
    int16 tCount;
    int16 crCount;
    int16 ctCount;
    // data...
};

struct Sphere16 {
    uint32 xy;
    uint32 zr;
};

struct StaticMesh {
    uint16 id;
    uint16 meshIndex;
    uint32 flags;
    Sphere16 vs;
    AABBs vbox;
    AABBs cbox;
};

#define SI_MODE(x)      (x & 3)
#define SI_COUNT(x)     ((x >> 2) & 15)
#define SI_CAMERA(x)    ((x >> 12) & 1)
#define SI_PITCH(x)     ((x >> 13) & 1)
#define SI_GAIN(x)      ((x >> 14) & 1)

struct SoundInfo
{
    uint16 index;
    uint16 volume;
    uint16 chance;
    uint16 flags;
};

struct SoundSource
{
    vec3i pos;
    uint16 id;
    uint16 flags;
};

struct Anim {
    uint32 frameOffset;
    uint8 frameRate;
    uint8 frameSize;
    uint16 state;
    int32 speed;
    int32 accel;
    uint16 frameBegin;
    uint16 frameEnd;
    uint16 nextAnimIndex;
    uint16 nextFrameIndex;
    uint16 statesCount;
    uint16 statesStart;
    uint16 commandsCount;
    uint16 commandsStart;
};

struct AnimState {
    uint8 state;
    uint8 rangesCount;
    uint16 rangesStart;
};

struct AnimRange {
    uint16 frameBegin;
    uint16 frameEnd;
    uint16 nextAnimIndex;
    uint16 nextFrameIndex;
};

struct AnimFrame {
    AABBs box;
    vec3s pos;
    uint16 angles[1];
};

struct Texture
{
#ifdef __3DO__
    uint8* data;
    uint32 shift;
#else
    uint32 tile;
    uint32 uv01;
    uint32 uv23;
#endif
};

struct Sprite
{
#ifdef __3DO__
    uint32 texture;
    int32 l, t, r, b;
#else
    uint32 tile;
    uint32 uwvh;
    int16 l, t, r, b;
#endif
};

struct SpriteSeq {
    uint16 type;
    uint16 unused;
    int16 count;
    uint16 start;
};


#define FIXED_CAMERA_FLAG_TIMER         0xFF
#define FIXED_CAMERA_FLAG_ONCE          (1 << 8)
#define FIXED_CAMERA_FLAG_SPEED_SHIFT   9
#define FIXED_CAMERA_FLAG_SPEED         (31 << FIXED_CAMERA_FLAG_SPEED_SHIFT)

struct FixedCamera {
    vec3i pos;
    int16 roomIndex;
    uint16 flags;
};

#define ITEM_FLAGS_STATUS_SHIFT     3
#define ITEM_FLAGS_MASK_SHIFT       9
#define ITEM_FLAGS_MASK_ALL         31

#define ITEM_FLAG_GRAVITY           (1 << 1)
#define ITEM_FLAG_ACTIVE            (1 << 2)
#define ITEM_FLAG_STATUS            (3 << ITEM_FLAGS_STATUS_SHIFT)
#define ITEM_FLAG_STATUS_ACTIVE         (1 << ITEM_FLAGS_STATUS_SHIFT)
#define ITEM_FLAG_STATUS_INACTIVE       (2 << ITEM_FLAGS_STATUS_SHIFT)
#define ITEM_FLAG_STATUS_INVISIBLE      (3 << ITEM_FLAGS_STATUS_SHIFT)
#define ITEM_FLAG_COLLISION         (1 << 5)
#define ITEM_FLAG_INJURED           (1 << 6)
#define ITEM_FLAG_ANIMATED          (1 << 7)
#define ITEM_FLAG_ONCE              (1 << 8)
#define ITEM_FLAG_MASK              (ITEM_FLAGS_MASK_ALL << ITEM_FLAGS_MASK_SHIFT)
#define ITEM_FLAG_REVERSE           (1 << 14)
#define ITEM_FLAG_SHADOW            (1 << 15)

struct ItemObjInfo {
    uint8 type;
    uint8 roomIndex;
    vec3s pos;
    uint16 intensity;
    uint16 flags;
};

#define FILE_ITEM_SIZE (sizeof(ItemObjInfo) - 2)

struct CollisionInfo;
struct Lara;

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

struct Location {
    Room* room;
    vec3i pos;
};

enum CameraMode {
    CAMERA_MODE_FREE,
    CAMERA_MODE_FOLLOW,
    CAMERA_MODE_COMBAT,
    CAMERA_MODE_LOOK,
    CAMERA_MODE_FIXED,
    CAMERA_MODE_OBJECT,
    CAMERA_MODE_CUTSCENE
};

struct Camera {
    Location view;
    Location target;

    int32 targetDist;
    vec3s targetAngle;

    vec3s angle;

    ItemObj* laraItem;
    ItemObj* lastItem;
    ItemObj* lookAtItem;

    int32 speed;
    int32 timer;
    int32 index;
    int32 lastIndex;

    CameraMode mode;

    bool lastFixed;
    bool center;

    void initCinematic();
    void updateCinematic();

    void init(ItemObj* lara);
    Location getLocationForAngle(int32 angle, int32 distH, int32 distV);
    void clip(Location &loc);
    Location getBestLocation(bool clip);
    void move(Location &to, int32 speed);
    void updateFree();
    void updateFollow(ItemObj* item);
    void updateCombat(ItemObj* item);
    void updateLook(ItemObj* item);
    void updateFixed();
    void lookAt(int32 offset);
    void update();
    void toCombat();
};

enum ZoneType
{
    ZONE_GROUND_1,
    ZONE_GROUND_2,
    ZONE_FLY,
    ZONE_MAX
};

struct Nav
{
    struct Cell
    {
        uint16 boxIndex;
        uint16 weight;
        uint16 end;
        uint16 next;
    };

    Cell cells[MAX_BOXES];
    uint32 cellsCount;

    uint32 zoneType;
    uint32 weight;

    uint32 endBox;
    uint32 nextBox;
    uint32 headBox;
    uint32 tailBox;
    int32 stepHeight;
    int32 dropHeight;
    int32 vSpeed;
    uint32 mask;

    vec3i pos;

    void init(uint32 boxIndex);
    void search(uint16 zone, const uint16* zones);
    vec3i getWaypoint(uint32 boxIndex, const vec3i &from);
};

enum WeaponState
{
    WEAPON_STATE_FREE,
    WEAPON_STATE_BUSY,
    WEAPON_STATE_DRAW,
    WEAPON_STATE_HOLSTER,
    WEAPON_STATE_READY
};

enum Weapon
{
    WEAPON_PISTOLS,
    WEAPON_MAGNUMS,
    WEAPON_UZIS,
    WEAPON_SHOTGUN,
    // WEAPON_DESERT_EAGLE,
    // WEAPON_REVOLVER,
    // WEAPON_M16
    // WEAPON_MP5
    // WEAPON_HK
    // WEAPON_ROCKET
    // WEAPON_GRENADE
    // WEAPON_HARPOON
    // WEAPON_CROSSBOW
    // WEAPON_GRAPPLING
    // WEAPON_FLARE
    WEAPON_NONE,
    WEAPON_MAX
};

struct WeaponParams
{
    ItemType modelType;
    ItemType animType;
    uint16 damage;
    uint16 spread;
    uint16 range;
    int16 height;
    int16 soundId;
    uint8 reloadTimer;
    uint8 flashOffset;
    uint8 flashTimer;
    uint8 flashIntensity;
    int16 aimX;
    int16 aimY;
    int16 armX;
    int16 armMinY;
    int16 armMaxY;
};

enum LaraArm
{
    LARA_ARM_R,
    LARA_ARM_L,
    LARA_ARM_MAX
};

enum LaraJoint
{
    JOINT_HIPS = 0,
    JOINT_LEG_L1,
    JOINT_LEG_L2,
    JOINT_LEG_L3,
    JOINT_LEG_R1,
    JOINT_LEG_R2,
    JOINT_LEG_R3,
    JOINT_TORSO,
    JOINT_ARM_R1,
    JOINT_ARM_R2,
    JOINT_ARM_R3,
    JOINT_ARM_L1,
    JOINT_ARM_L2,
    JOINT_ARM_L3,
    JOINT_HEAD,
    JOINT_MAX
};

struct ExtraInfoLara
{
    int16 swimTimer;
    uint8 weaponState;
    uint8 vSpeedHack;
    
    int16 moveAngle;
    int16 hitFrame;

    int8  hitTimer;
    int8  hitQuadrant;

    uint8 weapon;
    uint8 goalWeapon;

    struct Head {
        vec3s angle;
    } head;

    struct Torso {
        vec3s angle;
    } torso;

    struct Arm
    {
        vec3s angle;
        vec3s angleAim;

        uint16 animIndex;
        uint16 frameIndex;

        struct Flash {
            int16 timer;
            int16 angle;
            int16 offset;
            int16 intensity;
        } flash;

        ItemObj* target;

        bool aim;
        bool useBasis;
    };
    
    Arm armR;
    Arm armL;

    Camera camera;

    uint16 meshes[JOINT_MAX];

    int16 ammo[WEAPON_MAX]; // TODO make global

    Nav nav;

    uint16 lastInput;
    int8 healthTimer;

    bool dozy;
};

extern ExtraInfoLara playersExtra[MAX_PLAYERS];

#define gCinematicCamera playersExtra[0].camera

struct Enemy;

enum EnemyMood
{
    MOOD_SLEEP,
    MOOD_STALK,
    MOOD_ATTACK,
    MOOD_ESCAPE
};

struct ExtraInfoEnemy
{
    int16 rotHead;
    int16 rotNeck;

    int16 maxTurn;
    int16 _reserved;

    Enemy* enemy;

    Nav nav;
};

struct ItemObj
{
    Room* room;

    vec3i pos;
    vec3s angle;

    uint16 flags;

    int16 vSpeed;
    int16 hSpeed;

    union {
        uint16 animIndex;
        int16 tick; // effects only
    };

    uint16 frameIndex;

    uint8 state;
    uint8 nextState; // enemies only
    uint8 goalState;
    uint8 waterState;

    uint16 headOffset; // enemies only
    union {
        uint16 gymTimer;   // lara only
        uint16 aggression; // enemies only
    };

    int16 health;
    union {
        int16 timer;
        int16 oxygen; // Lara only
        int16 radius; // enemies only TODO
    };

    union {
        uint16 input; // Lara only
        uint16 mood; // enemies only
        int16 corpseTimer; // enemies only
    };
    int16 turnSpeed;

    uint8 type;
    uint8 intensity;
    int16 roomFloor;

    uint32 hitMask;
    uint32 visibleMask;

    union {
        uint8* extra;
        ExtraInfoLara* extraL;
        ExtraInfoEnemy* extraE;
    };

    ItemObj* nextItem;
    ItemObj* nextActive;

    static ItemObj* sFirstActive;
    static ItemObj* sFirstFree;

    static ItemObj* add(ItemType type, Room* room, const vec3i &pos, int32 angleY);
    void remove();

    void fxBubbles(Room *fxRoom, int32 fxJoint, const vec3i &fxOffset);
    void fxRicochet(Room *fxRoom, const vec3i &fxPos, bool fxSound);
    void fxBlood(const vec3i &fxPos, int16 fxAngleY, int16 fxSpeed);
    void fxSmoke(const vec3i &fxPos);
    void fxSplash();

    int32 getFrames(const AnimFrame* &frameA, const AnimFrame* &frameB, int32 &animFrameRate) const;
    const AnimFrame* getFrame() const;
    const AABBs& getBoundingBox(bool lerp) const;
    void move();
    const Anim* animSet(int32 newAnimIndex, bool resetState, int32 frameOffset = 0);
    const Anim* animChange(const Anim* anim);
    void animCmd(bool fx, const Anim* anim);
    void animSkip(int32 stateBefore, int32 stateAfter, bool advance = false);
    void animProcess(bool movement = true);
    bool animIsEnd(int32 offset) const;
    void animHit(int32 dirX, int32 dirZ, int32 hitTimer);
    bool moveTo(const vec3i &point, ItemObj* item, bool lerp);

    void updateRoom(int32 offset = 0);

    bool isKeyHit(InputState state) const; // Lara only

    vec3i getRelative(const vec3i &point) const;

    int32 getWaterLevel() const;
    int32 getWaterDepth() const;
    int32 getBridgeFloor(int32 x, int32 z) const;
    int32 getTrapDoorFloor(int32 x, int32 z) const;
    int32 getDrawBridgeFloor(int32 x, int32 z) const;
    void getItemFloorCeiling(int32 x, int32 y, int32 z, int32* floor, int32* ceiling) const;

    vec3i getJoint(int32 jointIndex, const vec3i &offset) const;
    int32 getSpheres(Sphere* spheres, bool flag) const;

    uint32 collideSpheres(Lara* lara) const;
    bool collideBounds(Lara* lara, CollisionInfo* cinfo) const;
    void collidePush(Lara* lara, CollisionInfo* cinfo, bool enemyHit) const;
    void collideRoom(int32 height, int32 yOffset) const;

    uint32 updateHitMask(Lara* lara, CollisionInfo* cinfo);

    void meshSwap(ItemType type, uint32 mask);

    ItemObj* init(Room* room);

    X_INLINE ItemObj() {}
    ItemObj(Room* room);
    virtual void activate();
    virtual void deactivate();
    virtual void hit(int32 damage, const vec3i &point, int32 soundId);
    virtual void collide(Lara* lara, CollisionInfo* cinfo);
    virtual void update();
    virtual void draw();
    virtual uint8* save(uint8* data);
    virtual uint8* load(uint8* data);
};

#define TRACK_FLAG_ONCE     32
#define TRACK_FLAG_MASK     31

#define SAVEGAME_VER    2
#define SAVEGAME_SIZE   (8 * 1024)  // 8k EWRAM

struct SaveGame
{
    uint32 version;
    uint32 dataSize;

    uint8  level;
    int8   track;
    uint8  secrets;
    uint8  pickups;
    uint32 time;
    uint32 distance;
    uint32 randSeedLogic;
    uint32 randSeedDraw;
    uint16 mediUsed;
    uint16 ammoUsed;
    uint16 kills;
    uint16 flipped;
    uint8  tracks[64];
    uint16 invSlots[64];
};

#define SETTINGS_VER    3
#define SETTINGS_SIZE   128

struct Settings
{
    uint8 version;
    uint8 controls_vibration:1;
    uint8 controls_swap:1;
    uint8 audio_sfx:1;
    uint8 audio_music:1;
    uint8 video_gamma:5;
    uint8 video_fps:1;
    uint8 video_vsync:1;
};

#define FD_SET_END(x,end)   ((x) |= ((end) << 15))
#define FD_END(x)           ((x) >> 15)
#define FD_FLOOR_TYPE(x)    ((x) & 0x1F)
#define FD_TRIGGER_TYPE(x)  (((x) >> 8) & 0x7F)
#define FD_TIMER(x)         ((x) & 0xFF)
#define FD_ONCE(x)          (((x) >> 8) & 1)
#define FD_SPEED(x)         (((x) >> 9) & 0x1F)
#define FD_MASK(x)          (((x) >> 9) & 0x1F)
#define FD_ACTION(x)        (((x) >> 10) & 0x1F)
#define FD_ARGS(x)          ((x) & 0x03FF)
#define FD_SLANT_X(x)       int8((x) & 0xFF)
#define FD_SLANT_Z(x)       int8((x) >> 8)

typedef uint16 FloorData;

enum FloorType
{
    FLOOR_TYPE_NONE,
    FLOOR_TYPE_PORTAL,
    FLOOR_TYPE_FLOOR,
    FLOOR_TYPE_CEILING,
    FLOOR_TYPE_TRIGGER,
    FLOOR_TYPE_LAVA
};

enum TriggerType
{
    TRIGGER_TYPE_ACTIVATE,
    TRIGGER_TYPE_PAD,
    TRIGGER_TYPE_SWITCH,
    TRIGGER_TYPE_KEY,
    TRIGGER_TYPE_PICKUP,
    TRIGGER_TYPE_OBJECT,
    TRIGGER_TYPE_ANTIPAD,
    TRIGGER_TYPE_COMBAT,
    TRIGGER_TYPE_DUMMY
};

enum TriggerAction
{
    TRIGGER_ACTION_ACTIVATE_OBJECT,
    TRIGGER_ACTION_ACTIVATE_CAMERA,
    TRIGGER_ACTION_FLOW,
    TRIGGER_ACTION_FLIP,
    TRIGGER_ACTION_FLIP_ON,
    TRIGGER_ACTION_FLIP_OFF,
    TRIGGER_ACTION_CAMERA_TARGET,
    TRIGGER_ACTION_END,
    TRIGGER_ACTION_SOUNDTRACK,
    TRIGGER_ACTION_EFFECT,
    TRIGGER_ACTION_SECRET,
    TRIGGER_ACTION_CLEAR_BODIES,
    TRIGGER_ACTION_FLYBY,
    TRIGGER_ACTION_CUTSCENE
};

enum SlantType
{
    SLANT_NONE,
    SLANT_LOW,
    SLANT_HIGH
};

enum WaterState
{
    WATER_STATE_ABOVE,
    WATER_STATE_WADE,
    WATER_STATE_SURFACE,
    WATER_STATE_UNDER
};

enum AnimCommand
{
    ANIM_CMD_NONE,
    ANIM_CMD_OFFSET,
    ANIM_CMD_JUMP,
    ANIM_CMD_EMPTY,
    ANIM_CMD_KILL,
    ANIM_CMD_SOUND,
    ANIM_CMD_EFFECT
};

enum EffectType
{
    FX_NONE           = -1,
    FX_ROTATE_180     ,
    FX_FLOOR_SHAKE    ,
    FX_LARA_NORMAL    ,
    FX_LARA_BUBBLES   ,
    FX_FINISH_LEVEL   ,
    FX_EARTHQUAKE     ,
    FX_FLOOD          ,
    FX_UNK1           ,
    FX_STAIRS2SLOPE   ,
    FX_UNK3           ,
    FX_UNK4           ,
    FX_EXPLOSION      ,
    FX_LARA_HANDSFREE ,
    FX_FLIP_MAP       ,
    FX_DRAW_RIGHTGUN  ,
    FX_DRAW_LEFTGUN   ,
    FX_SHOT_RIGHTGUN  ,
    FX_SHOT_LEFTGUN   ,
    FX_MESH_SWAP_1    ,
    FX_MESH_SWAP_2    ,
    FX_MESH_SWAP_3    ,
    FX_INV_ON         ,
    FX_INV_OFF        ,
    FX_DYN_ON         ,
    FX_DYN_OFF        ,
    FX_STATUE_FX      ,
    FX_RESET_HAIR     ,
    FX_BOILER_FX      ,
    FX_ASSAULT_RESET  ,
    FX_ASSAULT_STOP   ,
    FX_ASSAULT_START  ,
    FX_ASSAULT_FINISH ,
    FX_FOOTPRINT      ,
// specific
    FX_TR1_FLICKER    = 16
};

enum SoundMode {
    UNIQUE,
    REPLAY,
    LOOP
};

enum SoundID
{
    SND_NO              = 2,

    SND_LANDING         = 4,
        
    SND_DRAW            = 6,
    SND_HOLSTER         = 7,
    SND_PISTOLS_SHOT    = 8,
    SND_SHOTGUN_RELOAD  = 9,
    SND_RICOCHET        = 10,
        
    SND_HIT_BEAR        = 16,
    SND_HIT_WOLF        = 20,

    SND_SCREAM          = 30,
    SND_HIT             = 27,
    SND_DAMAGE          = 31,

    SND_SPLASH          = 33,
        
    SND_BUBBLE          = 37,
         
    SND_UZIS_SHOT       = 43,
    SND_MAGNUMS_SHOT    = 44,
    SND_SHOTGUN_SHOT    = 45,
    SND_EMPTY           = 48,
    SND_HIT_UNDERWATER  = 50,
        
    SND_UNDERWATER      = 60,

    SND_BOULDER         = 70,

    SND_FLOOD           = 81,

    SND_HIT_LION        = 85,

    SND_HIT_RAT         = 95,
        
    SND_LIGHTNING       = 98,
    SND_ROCK            = 99,

    SND_SWORD           = 103,
    SND_EXPLOSION       = 104,

    SND_INV_SPIN        = 108,
    SND_INV_HOME        = 109,
    SND_INV_CONTROLS    = 110,
    SND_INV_SHOW        = 111,
    SND_INV_HIDE        = 112,
    SND_INV_COMPASS     = 113,
    SND_INV_WEAPON      = 114,
    SND_INV_PAGE        = 115,
    SND_HEALTH          = 116,
        
    SND_STAIRS2SLOPE    = 119,

    SND_NATLA_SHOT      = 123,

    SND_HIT_SKATER      = 132,

    SND_HIT_ADAM        = 142,
    SND_STOMP           = 147,
        
    SND_LAVA            = 149,
    SND_FLAME           = 150,
    SND_DART            = 151,
        
    SND_TNT             = 170,
    SND_MUTANT_DEATH    = 171,
    SND_SECRET          = 173,

    SND_HELICOPTER      = 297,

    SND_WINSTON_SCARED  = 344,
    SND_WINSTON_WALK    = 345,
    SND_WINSTON_PUSH    = 346,
    SND_WINSTON_TRAY    = 347
};

#define LARA_LOOK_ANGLE_MAX     ANGLE(22)
#define LARA_LOOK_ANGLE_MIN     ANGLE(-42)
#define LARA_LOOK_ANGLE_Y       ANGLE(44)
#define LARA_LOOK_TURN_SPEED    ANGLE(2)

enum CollisionType
{
    CT_NONE          = 0,
    CT_FRONT         = (1 << 0),
    CT_LEFT          = (1 << 1),
    CT_RIGHT         = (1 << 2),
    CT_CEILING       = (1 << 3),
    CT_FRONT_CEILING = (1 << 4),
    CT_FLOOR_CEILING = (1 << 5)
};

struct CollisionInfo
{
    enum SideType
    {
        ST_MIDDLE,
        ST_FRONT,
        ST_LEFT,
        ST_RIGHT,
        ST_MAX
    };

    struct Side
    {
        int32     floor;
        int32     ceiling;
        SlantType slantType;
    };

    const FloorData* trigger;

    Side m;
    Side f;
    Side l;
    Side r;

    int32 radius;

    int32 gapPos;
    int32 gapNeg;
    int32 gapCeiling;

    vec3i offset;
    vec3i pos;

    int16 angle;
    uint16 quadrant;

    CollisionType type;

    int8 slantX;
    int8 slantZ;

    bool enemyPush;
    bool enemyHit;
    bool staticHit;
    bool stopOnSlant;
    bool stopOnLava;

    void setSide(SideType st, int32 floor, int32 ceiling);

    X_INLINE void setAngle(int16 value)
    {
        angle = value;
        quadrant = uint16(value + ANGLE_45) >> ANGLE_SHIFT_90;
    }
};

struct Box
{
    uint8 minZ, maxZ;
    uint8 minX, maxX;
    int16 floor;
    uint16 overlap;
};

struct CameraFrame
{
    vec3s  target;
    vec3s  pos;
    int16  fov;
    int16  roll;
};

enum Version
{
    VER_TR1_GBA,
    VER_TR1_3DO,
    VER_TR1_32X,
    VER_TR1_PC
};

struct Level
{
    uint32 version;

    uint16 tilesCount;
    uint16 roomsCount;
    uint16 modelsCount;
    uint16 meshesCount;
    uint16 staticMeshesCount;
    uint16 spriteSequencesCount;
    uint16 soundSourcesCount;
    uint16 boxesCount;
    uint16 texturesCount;
    uint16 spritesCount;
    uint16 itemsCount;
    uint16 camerasCount;
    uint16 cameraFramesCount;
    uint16 soundOffsetsCount;

    const uint16* palette;
    const uint8* lightmap;
    const uint8* tiles;
    const RoomInfo* roomsInfo;
    const FloorData* floors;
    const Mesh** meshes;
    const int32* meshOffsets;
    const Anim* anims;
    const AnimState* animStates;
    const AnimRange* animRanges;
    const int16* animCommands;
    const ModelNode* nodes;
    const uint16* animFrames;
    const Model* models;
    const StaticMesh* staticMeshes;
    Texture* textures;
    Sprite* sprites;
    const SpriteSeq* spriteSequences;
    FixedCamera* cameras;
    const SoundSource* soundSources;
    Box* boxes;
    const uint16* overlaps;
    const uint16* zones[2][ZONE_MAX];
    const uint16* animTexData;
    const ItemObjInfo* itemsInfo;
    const CameraFrame* cameraFrames;
    const uint16* soundMap;
    const SoundInfo* soundsInfo;
    const uint8* soundData;
    const int32* soundOffsets;
};

// used by enemies
struct TargetInfo
{
    ItemObj* target;
    vec3i waypoint;
    vec3i pos;
    int16 angle;
    int16 rotHead;
    int16 tilt;
    int16 turn;
    uint32 dist;
    uint16 boxIndex;
    uint16 boxIndexTarget;
    uint16 zoneIndex;
    uint16 zoneIndexTarget;
    bool front;
    bool behind;
    bool canAttack;
};

extern TargetInfo tinfo;

extern Level level;

struct IMA_STATE
{
    int32 smp;
    int32 idx;
};

// Currently only used for GBA
struct ADPCM4_STATE
{
    int32 zM1, zM2;
    int32 tap;
    int32 quant;
};

#if defined(GAPI_GL1)
    #define PERSPECTIVE_DZ(z) z

    #define PERSPECTIVE(x, y, z) {\
        float invZ = FRAME_PERSP / PERSPECTIVE_DZ(z);\
        x = int32(x * (FRAME_HEIGHT >> 1) * invZ);\
        y = int32(y * (FRAME_HEIGHT >> 1) * invZ);\
    }
#elif defined(MODEHW) || defined(MODE13)
    #define PROJ_SHIFT 4

    #define PERSPECTIVE_DZ(z) (z >> PROJ_SHIFT)

    #define PERSPECTIVE(x, y, z) {\
        int32 dz = PERSPECTIVE_DZ(z);\
        if (dz >= DIV_TABLE_SIZE) dz = DIV_TABLE_SIZE - 1;\
        int32 d = FixedInvU(dz);\
        x = (x * d) >> (16 - PROJ_SHIFT);\
        y = (y * d) >> (16 - PROJ_SHIFT);\
    }
#elif defined(MODE4)
    #define PROJ_SHIFT 4

    #define PERSPECTIVE_DZ(z) ((z >> 4) + (z >> 6))

    #define PERSPECTIVE(x, y, z) {\
        int32 dz = PERSPECTIVE_DZ(z);\
        if (dz >= DIV_TABLE_SIZE) dz = DIV_TABLE_SIZE - 1;\
        int32 d = FixedInvU(dz);\
        x = (x * d) >> (16 - PROJ_SHIFT);\
        y = (y * d) >> (16 - PROJ_SHIFT);\
    }
#else
    #define PERSPECTIVE(x, y, z) {\
        int32 dz = (z >> (FIXED_SHIFT + FOV_SHIFT - 1)) / 3;\
        if (dz >= DIV_TABLE_SIZE) dz = DIV_TABLE_SIZE - 1;\
        int32 d = FixedInvU(dz);\
        x = d * (x >> FIXED_SHIFT) >> 13;\
        y = d * (y >> FIXED_SHIFT) >> 13;\
    }
#endif

#define STR_LANGUAGES \
      "English"       \
    , "Fran|cais"     \
    , "Deutsch"

#define STR_SCALE "25", "50", "75", "100"

enum StringID {
      STR_EMPTY
    , STR_ALPHA_END_1
    , STR_ALPHA_END_2
    , STR_ALPHA_END_3
    , STR_ALPHA_END_4
    , STR_ALPHA_END_5
    , STR_ALPHA_END_6
    , STR_GBA_SAVE_WARNING_1
    , STR_GBA_SAVE_WARNING_2
    , STR_GBA_SAVE_WARNING_3
// common
    , STR_LOADING
    , STR_LEVEL_STATS
    , STR_HINT_SAVING
    , STR_HINT_SAVING_DONE
    , STR_HINT_SAVING_ERROR
    , STR_YES
    , STR_NO
    , STR_OFF
    , STR_ON
    , STR_OK
    , STR_SBS
    , STR_ANAGLYPH
    , STR_SPLIT
    , STR_VR
    , STR_QUALITY_LOW
    , STR_QUALITY_MEDIUM
    , STR_QUALITY_HIGH
    , STR_LANG_EN
    , STR_LANG_FR
    , STR_LANG_DE
//    , STR_LANG_ES
//    , STR_LANG_IT
//    , STR_LANG_PL
//    , STR_LANG_PT
//    , STR_LANG_RU
//    , STR_LANG_JA
//    , STR_LANG_GR
//    , STR_LANG_FI
//    , STR_LANG_CZ
//    , STR_LANG_CN
//    , STR_LANG_HU
//    , STR_LANG_SV
    , STR_APPLY
    , STR_GAMEPAD_1
    , STR_GAMEPAD_2
    , STR_GAMEPAD_3
    , STR_GAMEPAD_4
    , STR_NOT_READY
    , STR_PLAYER_1
    , STR_PLAYER_2
    , STR_PRESS_ANY_KEY
    , STR_HELP_SELECT
    , STR_HELP_BACK
// inventory pages
    , STR_INV_TITLE_OPTIONS
    , STR_INV_TITLE_MAIN
    , STR_INV_TITLE_KEYS
// save game page
    , STR_SAVEGAME
    , STR_CURRENT_POSITION
// inventory option
    , STR_GAME
    , STR_MAP
    , STR_COMPASS
    , STR_STOPWATCH
    , STR_HOME
    , STR_DETAIL
    , STR_SOUND
    , STR_CONTROLS
    , STR_GAMMA
// passport menu
    , STR_LOAD_GAME
    , STR_SAVE_GAME
    , STR_START_GAME
    , STR_RESTART_LEVEL
    , STR_EXIT_TO_TITLE
    , STR_EXIT_GAME
    , STR_SELECT_LEVEL
// detail options
    , STR_SELECT_DETAIL
    , STR_OPT_DETAIL_GAMMA
    , STR_OPT_DETAIL_FPS
    , STR_OPT_DETAIL_FILTER
    , STR_OPT_DETAIL_LIGHTING
    , STR_OPT_DETAIL_SHADOWS
    , STR_OPT_DETAIL_WATER
    , STR_OPT_DETAIL_VSYNC
    , STR_OPT_DETAIL_STEREO
    , STR_OPT_SIMPLE_ITEMS
    , STR_OPT_RESOLUTION
    , STR_SCALE_25
    , STR_SCALE_50
    , STR_SCALE_75
    , STR_SCALE_100
// sound options
    , STR_SET_VOLUMES
    , STR_REVERBERATION
    , STR_OPT_SUBTITLES
    , STR_OPT_LANGUAGE
    , STR_OPT_SOUND_SFX
    , STR_OPT_SOUND_MUSIC
// controls options
    , STR_SET_CONTROLS
    , STR_OPT_CONTROLS_KEYBOARD
    , STR_OPT_CONTROLS_GAMEPAD
    , STR_OPT_CONTROLS_VIBRATION
    , STR_OPT_CONTROLS_RETARGET
    , STR_OPT_CONTROLS_MULTIAIM
    , STR_OPT_CONTROLS_SWAP
    // controls
    , STR_CTRL_RUN
    , STR_CTRL_BACK
    , STR_CTRL_RIGHT
    , STR_CTRL_LEFT
    , STR_CTRL_WALK
    , STR_CTRL_JUMP
    , STR_CTRL_ACTION
    , STR_CTRL_WEAPON
    , STR_CTRL_LOOK
    , STR_CTRL_ROLL
    , STR_CTRL_INVENTORY
    , STR_CTRL_PAUSE
    // control keys
    , STR_KEY_UP
    , STR_KEY_DOWN
    , STR_KEY_RIGHT
    , STR_KEY_LEFT
    , STR_KEY_A
    , STR_KEY_B
    , STR_KEY_L
    , STR_KEY_R
    , STR_KEY_SELECT
    , STR_KEY_START
    , STR_KEY_L_R
    , STR_KEY_L_A
    , STR_KEY_L_B
// inventory items
    , STR_UNKNOWN
    , STR_EXPLOSIVE
    , STR_PISTOLS
    , STR_SHOTGUN
    , STR_MAGNUMS
    , STR_UZIS
    , STR_AMMO_PISTOLS
    , STR_AMMO_SHOTGUN
    , STR_AMMO_MAGNUMS
    , STR_AMMO_UZIS
    , STR_MEDI_SMALL
    , STR_MEDI_BIG
    , STR_LEAD_BAR
    , STR_SCION
// keys
    , STR_KEY
    , STR_KEY_SILVER
    , STR_KEY_RUSTY
    , STR_KEY_GOLD
    , STR_KEY_SAPPHIRE
    , STR_KEY_NEPTUNE
    , STR_KEY_ATLAS
    , STR_KEY_DAMOCLES
    , STR_KEY_THOR
    , STR_KEY_ORNATE
// puzzles
    , STR_PUZZLE
    , STR_PUZZLE_GOLD_IDOL
    , STR_PUZZLE_GOLD_BAR
    , STR_PUZZLE_COG
    , STR_PUZZLE_FUSE
    , STR_PUZZLE_ANKH
    , STR_PUZZLE_HORUS
    , STR_PUZZLE_ANUBIS
    , STR_PUZZLE_SCARAB
    , STR_PUZZLE_PYRAMID
#ifdef USE_SUBTITLES
// TR1 subtitles
    , STR_TR1_SUB_CAFE
    , STR_TR1_SUB_LIFT
    , STR_TR1_SUB_CANYON
    , STR_TR1_SUB_PRISON
    , STR_TR1_SUB_22 // CUT4
    , STR_TR1_SUB_23 // CUT1
    , STR_TR1_SUB_24
    , STR_TR1_SUB_25 // CUT3
    , STR_TR1_SUB_26
    , STR_TR1_SUB_27
    , STR_TR1_SUB_28
    , STR_TR1_SUB_29
    , STR_TR1_SUB_30
    , STR_TR1_SUB_31
    , STR_TR1_SUB_32
    , STR_TR1_SUB_33
    , STR_TR1_SUB_34
    , STR_TR1_SUB_35
    , STR_TR1_SUB_36
    , STR_TR1_SUB_37
    , STR_TR1_SUB_38
    , STR_TR1_SUB_39
    , STR_TR1_SUB_40
    , STR_TR1_SUB_41
    , STR_TR1_SUB_42
    , STR_TR1_SUB_43
    , STR_TR1_SUB_44
    , STR_TR1_SUB_45
    , STR_TR1_SUB_46
    , STR_TR1_SUB_47
    , STR_TR1_SUB_48
    , STR_TR1_SUB_49
    , STR_TR1_SUB_50
    , STR_TR1_SUB_51
    , STR_TR1_SUB_52
    , STR_TR1_SUB_53
    , STR_TR1_SUB_54
    , STR_TR1_SUB_55
    , STR_TR1_SUB_56
#endif
// TR1 levels
    , STR_TR1_GYM
    , STR_TR1_LEVEL1
    , STR_TR1_LEVEL2
    , STR_TR1_LEVEL3A
    , STR_TR1_LEVEL3B
    , STR_TR1_LEVEL4
    , STR_TR1_LEVEL5
    , STR_TR1_LEVEL6
    , STR_TR1_LEVEL7A
    , STR_TR1_LEVEL7B
    , STR_TR1_LEVEL8A
    , STR_TR1_LEVEL8B
    , STR_TR1_LEVEL8C
    , STR_TR1_LEVEL10A
    , STR_TR1_LEVEL10B
    , STR_TR1_LEVEL10C
    , STR_TR1_EGYPT
    , STR_TR1_CAT
    , STR_TR1_END
    , STR_TR1_END2
// TR2 levels
    , STR_TR2_ASSAULT
    , STR_TR2_WALL
    , STR_TR2_BOAT
    , STR_TR2_VENICE
    , STR_TR2_OPERA
    , STR_TR2_RIG
    , STR_TR2_PLATFORM
    , STR_TR2_UNWATER
    , STR_TR2_KEEL
    , STR_TR2_LIVING
    , STR_TR2_DECK
    , STR_TR2_SKIDOO
    , STR_TR2_MONASTRY
    , STR_TR2_CATACOMB
    , STR_TR2_ICECAVE
    , STR_TR2_EMPRTOMB
    , STR_TR2_FLOATING
    , STR_TR2_XIAN
    , STR_TR2_HOUSE
// TR3 levels
    , STR_TR3_HOUSE
    , STR_TR3_JUNGLE
    , STR_TR3_TEMPLE
    , STR_TR3_QUADCHAS
    , STR_TR3_TONYBOSS
    , STR_TR3_SHORE
    , STR_TR3_CRASH
    , STR_TR3_RAPIDS
    , STR_TR3_TRIBOSS
    , STR_TR3_ROOFS
    , STR_TR3_SEWER
    , STR_TR3_TOWER
    , STR_TR3_OFFICE
    , STR_TR3_NEVADA
    , STR_TR3_COMPOUND
    , STR_TR3_AREA51
    , STR_TR3_ANTARC
    , STR_TR3_MINES
    , STR_TR3_CITY
    , STR_TR3_CHAMBER
    , STR_TR3_STPAUL

    , STR_MAX
};

extern const char* const* STR;

enum TrackID
{
    TRACK_NONE          = -1,
// TR1
    TRACK_TR1_TITLE     = 4,    // TODO 2
    TRACK_TR1_CAVES     = 5,
    TRACK_TR1_SECRET    = 13,
    TRACK_TR1_CISTERN   = 57,
    TRACK_TR1_WIND      = 58,
    TRACK_TR1_PYRAMID   = 59,
    TRACK_TR1_CUT_1     = 23,
    TRACK_TR1_CUT_2     = 25,
    TRACK_TR1_CUT_3     = 24,
    TRACK_TR1_CUT_4     = 22
};

struct LevelInfo
{
    const void*  data;
    StringID     title;
    TrackID      track;
    uint8        secrets;
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
    LVL_TR1_EGYPT,
    LVL_TR1_CAT,
    LVL_TR1_END,
    LVL_TR1_END2,
    LVL_LOAD,
    LVL_MAX
};

extern const LevelInfo gLevelInfo[LVL_MAX];
extern LevelID gLevelID;

enum BarType {
    BAR_HEALTH,
    BAR_OXYGEN,
    BAR_DASH,
    BAR_OPTION,
    BAR_MAX
};

enum TextAlign {
    TEXT_ALIGN_LEFT,
    TEXT_ALIGN_RIGHT,
    TEXT_ALIGN_CENTER
};

// renderer internal
extern uint32 keys;
extern RectMinMax viewport;
extern vec3i gCameraViewPos;
extern Matrix* gMatrixPtr;
extern Matrix gMatrixStack[MAX_MATRICES];
extern const uint32 gSinCosTable[4096];

extern Sphere gSpheres[2][MAX_SPHERES];

extern Settings gSettings;
extern SaveGame gSaveGame;
extern uint8 gSaveData[SAVEGAME_SIZE - sizeof(SaveGame)];

extern int32 gCurTrack;
extern int32 gAnimTexFrame;
extern int32 gBrightness;
extern int32 gLightAmbient;
extern int32 gRandTable[MAX_RAND_TABLE];
extern int32 gCaustics[MAX_CAUSTICS];
extern int32 gCausticsFrame;

extern const FloorData* gLastFloorData;
extern FloorData gLastFloorSlant;

extern Room rooms[MAX_ROOMS];
extern ItemObj items[MAX_ITEMS];

template <class T>
X_INLINE void swap(T &a, T &b) {
    T tmp = a;
    a = b;
    b = tmp;
}

X_INLINE uint16 swap16(uint16 x) {
    return ((x & 0x00FF) << 8) | ((x & 0xFF00) >> 8);
}

X_INLINE uint32 swap32(uint32 x) {
    return ((x & 0x000000FF) << 24) | ((x & 0x0000FF00) << 8) | ((x & 0x00FF0000) >> 8) | ((x & 0xFF000000) >> 24);
}


extern int32 gRandSeedLogic;
extern int32 gRandSeedDraw;

int32 rand_logic();
int32 rand_draw();

#define RAND_LOGIC(r) (rand_logic() * (r) >> 15)
#define RAND_DRAW(r)  (rand_draw() * (r) >> 15)

#define sincos(x,s,c) {\
    uint32 sc = gSinCosTable[uint32(x << 16) >> 20];\
    s = int32(sc) >> 16;\
    c = int32(sc) << 16 >> 16;\
}

#define sin(x) (int32(gSinCosTable[uint32(x << 16) >> 20]) >> 16)

int32 phd_atan(int32 x, int32 y);
uint32 phd_sqrt(uint32 x);

X_INLINE int32 dot(const vec3i &a, const vec3i &b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

X_INLINE int32 fastLength(int32 dx, int32 dy)
{
    dx = abs(dx);
    dy = abs(dy);
    return (dx > dy) ? (dx + (dy >> 1)) : (dy + (dx >> 1));
}

void anglesFromVector(int32 x, int32 y, int32 z, int16 &angleX, int16 &angleY);

X_INLINE int16 angleLerp(int16 a, int16 b, int32 w)
{
    int16 d = b - a;
    if (d > +w) return a + w;
    if (d < -w) return a - w;
    return b;
}

#define angleDec(angle, value) angleLerp(angle, 0, value)

bool boxIntersect(const AABBi &i, const AABBi &b);
bool boxContains(const AABBi &a, const vec3i &p);
vec3i boxPushOut(const AABBi &a, const AABBi &b);

#ifdef CPU_BIG_ENDIAN
#define DECODE_ANGLES(a,x,y,z)\
    x = (a & 0x3FF0) << 2;\
    y = (a & 0x000F) << 12 | ((a >> 16) & 0xFC00) >> 4;\
    z = ((a >> 16) & 0x03FF) << 6;
#else
#define DECODE_ANGLES(a,x,y,z)\
    x = ((a >> 16) & 0x3FF0) << 2;\
    y = ((a >> 16) & 0x000F) << 12 | (a & 0xFC00) >> 4;\
    z = (a & 0x03FF) << 6;
#endif

#define matrixGet() *gMatrixPtr

#ifdef USE_ASM
    extern "C" {
        void matrixPush_asm();
        void matrixSetIdentity_asm();
        void matrixSetBasis_asm(Matrix &dst, const Matrix &src);
        void matrixLerp_asm(const Matrix &n, int32 pmul, int32 pdiv);
        void matrixTranslateRel_asm(int32 x, int32 y, int32 z);
        void matrixTranslateAbs_asm(int32 x, int32 y, int32 z);
        void matrixTranslateSet_asm(int32 x, int32 y, int32 z);
        void matrixRotateX_asm(int32 angle);
        void matrixRotateY_asm(int32 angle);
        void matrixRotateZ_asm(int32 angle);
        void matrixRotateYQ_asm(int32 quadrant);
        void matrixRotateYXZ_asm(int32 angleX, int32 angleY, int32 angleZ);
        void matrixFrame_asm(const void* pos, const void* angles);
        void boxTranslate_asm(AABBi &box, int32 x, int32 y, int32 z);
        void boxRotateYQ_asm(AABBi &box, int32 quadrant);
        int32 sphereIsVisible_asm(int32 x, int32 y, int32 z, int32 r);
        void flush_asm();
    }

    #define matrixPush              matrixPush_asm
    #define matrixSetIdentity       matrixSetIdentity_asm
    #define matrixSetBasis          matrixSetBasis_asm
    #define matrixLerp              matrixLerp_asm
    #define matrixTranslateRel      matrixTranslateRel_asm
    #define matrixTranslateAbs      matrixTranslateAbs_asm
    #define matrixTranslateSet      matrixTranslateSet_asm
    #define matrixRotateX           matrixRotateX_asm
    #define matrixRotateY           matrixRotateY_asm
    #define matrixRotateZ           matrixRotateZ_asm
    #define matrixRotateYXZ         matrixRotateYXZ_asm
    #define matrixRotateYQ          matrixRotateYQ_asm
    #define matrixFrame             matrixFrame_asm
    #define boxTranslate            boxTranslate_asm
    #define boxRotateYQ             boxRotateYQ_asm
    #define sphereIsVisible         sphereIsVisible_asm
    #define flush                   flush_asm
#else
    #define matrixPush              matrixPush_c
    #define matrixSetIdentity       matrixSetIdentity_c
    #define matrixSetBasis          matrixSetBasis_c
    #define matrixLerp              matrixLerp_c
    #define matrixTranslateRel      matrixTranslateRel_c
    #define matrixTranslateAbs      matrixTranslateAbs_c
    #define matrixTranslateSet      matrixTranslateSet_c
    #define matrixRotateX           matrixRotateX_c
    #define matrixRotateY           matrixRotateY_c
    #define matrixRotateZ           matrixRotateZ_c
    #define matrixRotateYXZ         matrixRotateYXZ_c
    #define matrixRotateYQ          matrixRotateYQ_c
    #define matrixFrame             matrixFrame_c
    #define boxTranslate            boxTranslate_c
    #define boxRotateYQ             boxRotateYQ_c
    #define sphereIsVisible         sphereIsVisible_c
    #define flush                   flush_c

    void matrixPush_c();
    void matrixSetIdentity_c();
    void matrixSetBasis_c(Matrix &dst, const Matrix &src);
    void matrixLerp_c(const Matrix &n, int32 pmul, int32 pdiv);
    void matrixTranslateRel_c(int32 x, int32 y, int32 z);
    void matrixTranslateAbs_c(int32 x, int32 y, int32 z);
    void matrixTranslateSet_c(int32 x, int32 y, int32 z);
    void matrixRotateX_c(int32 angle);
    void matrixRotateY_c(int32 angle);
    void matrixRotateZ_c(int32 angle);
    void matrixRotateYQ_c(int32 quadrant);
    void matrixRotateYXZ_c(int32 angleX, int32 angleY, int32 angleZ);
    void matrixFrame_c(const void* pos, const void* angles);

    void boxTranslate_c(AABBi &box, int32 x, int32 y, int32 z);
    void boxRotateYQ_c(AABBi &box, int32 quadrant);
    int32 sphereIsVisible_c(int32 x, int32 y, int32 z, int32 r);
    void flush_c();
#endif

#ifdef __32X__ // TODO
    #undef matrixPush
    #undef matrixSetIdentity
    #undef matrixSetBasis
    #undef matrixLerp
    #undef matrixTranslateRel
    #undef matrixTranslateAbs
    #undef matrixTranslateSet
    #undef matrixRotateX
    #undef matrixRotateY
    #undef matrixRotateZ
    #undef matrixRotateYXZ
    #undef matrixRotateYQ
    //#undef boxTranslate
    //#undef boxRotateYQ
    //#undef sphereIsVisible
    //#undef flush

    #define matrixPush              matrixPush_asm
    #define matrixSetIdentity       matrixSetIdentity_asm
    #define matrixSetBasis          matrixSetBasis_asm
    #define matrixLerp              matrixLerp_asm
    #define matrixTranslateRel      matrixTranslateRel_asm
    #define matrixTranslateAbs      matrixTranslateAbs_asm
    #define matrixTranslateSet      matrixTranslateSet_asm
    #define matrixRotateX           matrixRotateX_asm
    #define matrixRotateY           matrixRotateY_asm
    #define matrixRotateZ           matrixRotateZ_asm
    #define matrixRotateYXZ         matrixRotateYXZ_asm
    #define matrixRotateYQ          matrixRotateYQ_asm
    //#define boxTranslate            boxTranslate_asm
    //#define boxRotateYQ             boxRotateYQ_asm
    //#define sphereIsVisible         sphereIsVisible_asm
    //#define flush                   flush_asm

    extern "C" 
    {
        void matrixPush_asm();
        void matrixSetIdentity_asm();
        void matrixSetBasis_asm(Matrix &dst, const Matrix &src);
        void matrixLerp_asm(const Matrix &n, int32 pmul, int32 pdiv);
        void matrixTranslateRel_asm(int32 x, int32 y, int32 z);
        void matrixTranslateAbs_asm(int32 x, int32 y, int32 z);
        void matrixTranslateSet_asm(int32 x, int32 y, int32 z);
        void matrixRotateX_asm(int32 angle);
        void matrixRotateY_asm(int32 angle);
        void matrixRotateZ_asm(int32 angle);
        void matrixRotateYQ_asm(int32 quadrant);
        void matrixRotateYXZ_asm(int32 angleX, int32 angleY, int32 angleZ);
        void boxTranslate_asm(AABBi &box, int32 x, int32 y, int32 z);
        void boxRotateYQ_asm(AABBi &box, int32 quadrant);
        int32 sphereIsVisible_asm(int32 x, int32 y, int32 z, int32 r);
        void flush_asm();
    }
#endif

#define matrixPop()     gMatrixPtr--; ASSERT(gMatrixPtr >= gMatrixStack);

X_INLINE vec3i matrixGetDir(const Matrix &m)
{
    return _vec3i(m.e20, m.e21, m.e22);
}

void matrixFrame_c(const void* pos, const void* angles);
void matrixFrameLerp(const void* pos, const void* anglesA, const void* anglesB, int32 delta, int32 rate);
void matrixSetView(const vec3i &pos, int32 angleX, int32 angleY);

#if defined(__GBA__) || defined(__GBA_WIN__)
#define renderInit()
#define renderFree()
#define renderSwap()
#define renderLevelInit()
#define renderLevelFree()
#else
void renderInit();
void renderFree();
void renderSwap();
void renderLevelInit();
void renderLevelFree();
#endif

void setViewport(const RectMinMax &vp);
void setPaletteIndex(int32 index);
void clear();
void renderRoom(Room* room);
void renderMesh(const Mesh* mesh);
void renderShadow(int32 x, int32 z, int32 sx, int32 sz);
void renderSprite(int32 vx, int32 vy, int32 vz, int32 vg, int32 index);
void renderGlyph(int32 vx, int32 vy, int32 index);
void renderFill(int32 x, int32 y, int32 width, int32 height, int32 shade, int32 z);
void renderBorder(int32 x, int32 y, int32 width, int32 height, int32 color1, int32 color2, int32 z);
void renderBar(int32 x, int32 y, int32 width, int32 value, BarType type);
void renderBackground(const void* background);
void* copyBackground();

int32 getTextWidth(const char* text);

void drawInit();
void drawFree();
void drawLevelInit();
void drawLevelFree();
void drawText(int32 x, int32 y, const char* text, TextAlign align);
void drawModel(const ItemObj* item);
void drawSprite(const ItemObj* item);
void drawRooms(Camera* camera);
void drawCinematicRooms();
void drawHUD(Lara* lara);
void drawNodesLerp(const ItemObj* item, const AnimFrame* frameA, const AnimFrame* frameB, int32 frameDelta, int32 frameRate);

void calcLightingDynamic(const Room* room, const vec3i &point);
void calcLightingStatic(int32 intensity);

void checkTrigger(const FloorData* fd, ItemObj* lara);
void readLevel(const uint8 *data);
bool trace(const Location &from, Location &to, bool accurate);

Lara* getLara(const vec3i &pos);

bool useSwitch(ItemObj* item, int32 timer);
bool useKey(ItemObj* item, ItemObj* lara);
bool usePickup(ItemObj* item);

void startLevel(LevelID id);
void nextLevel(LevelID next);
bool gameSave();
bool gameLoad();

int32 doTutorial(ItemObj* lara, int32 track);

void sndInit();
void sndInitSamples();
void sndFreeSamples();
void sndFill(int8* buffer);
void* sndPlaySample(int32 index, int32 volume, int32 pitch, int32 mode);
void sndPlayTrack(int32 track);
bool sndTrackIsPlaying();
void sndStopTrack();
void sndStopSample(int32 index);
void sndStop();

void palGrayRemap(uint8* data, int32 size);
void palSet(const uint16* palette, int32 gamma, int32 bright);

void updateFading(int32 frames);

void dmaFill(void* dst, uint8 value, uint32 count);
void dmaCopy(const void* src, void* dst, uint32 size);

// system
int32 osGetSystemTimeMS();
bool osSaveSettings();
bool osLoadSettings();
bool osCheckSave();
bool osSaveGame();
bool osLoadGame();
void osJoyVibrate(int32 index, int32 L, int32 R);
void osSetPalette(const uint16* palette);
const void* osLoadScreen(LevelID id);
const void* osLoadLevel(LevelID id);

#ifdef PROFILING
    #define PROFILE_FRAME\
        CNT_UPDATE,\
        CNT_RENDER

    #define PROFILE_STAGES\
        CNT_TRANSFORM,\
        CNT_ADD,\
        CNT_FLUSH,\
        CNT_VERT,\
        CNT_POLY

    #define PROFILE_SOUND\
        CNT_SOUND

    #if defined(PROFILE_FRAMETIME)
        enum ProfileCounterId {
            PROFILE_FRAME,
            CNT_MAX,
            PROFILE_STAGES,
            PROFILE_SOUND
        };
    #elif defined(PROFILE_SOUNDTIME)
        enum ProfileCounterId {
            PROFILE_SOUND,
            CNT_MAX,
            PROFILE_FRAME,
            PROFILE_STAGES
        };
    #else
        enum ProfileCounterId {
            PROFILE_STAGES,
            CNT_MAX,
            PROFILE_FRAME,
            PROFILE_SOUND
        };
    #endif

    extern uint32 gCounters[CNT_MAX];
    
    #if defined(__3DO__) || defined(__32X__) // should be first, armcpp bug (#elif)
        extern int32 g_timer;

        #define PROFILE_START() {\
            g_timer = osGetSystemTimeMS();\
        }

        #define PROFILE_STOP(value) {\
            value += (osGetSystemTimeMS() - g_timer);\
        }
    #elif defined(__WIN32__) || defined(__GBA_WIN__)
        extern LARGE_INTEGER g_timer;
        extern LARGE_INTEGER g_current;

        #define PROFILE_START() {\
            QueryPerformanceCounter(&g_timer);\
        }

        #define PROFILE_STOP(value) {\
            QueryPerformanceCounter(&g_current);\
            value += uint32(g_current.QuadPart - g_timer.QuadPart);\
        }
    #elif defined(__GBA__)
        #ifdef PROFILE_SOUNDTIME
            #define TIMER_FREQ_DIV 1
        #else
            #define TIMER_FREQ_DIV 3
        #endif

        #define PROFILE_START() {\
            REG_TM2CNT_L = 0;\
            REG_TM2CNT_H = (1 << 7) | TIMER_FREQ_DIV;\
        }

        #define PROFILE_STOP(value) {\
            value += REG_TM2CNT_L;\
            REG_TM2CNT_H = 0;\
        }
    #else
        #define PROFILE_START() aaa
        #define PROFILE_STOP(value) bbb
    #endif

    struct ProfileCounter
    {
        ProfileCounterId cnt;

        ProfileCounter(ProfileCounterId cnt) : cnt(cnt) {
            if (cnt < CNT_MAX) {
                PROFILE_START()
            }
        }

        ~ProfileCounter() {
            if (cnt < CNT_MAX) {
                PROFILE_STOP(gCounters[cnt]);
            }
        }
    };

    #define PROFILE(cnt) ProfileCounter profileCounter(cnt)
    #define PROFILE_CLEAR() memset(gCounters, 0, sizeof(gCounters));
#else
    #define PROFILE(cnt)
    #define PROFILE_CLEAR()
#endif


#endif
