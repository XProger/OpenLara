#ifndef H_COMMON
#define H_COMMON

//#define TEST
//#define PROFILE

#if defined(_WIN32)
    #define _CRT_SECURE_NO_WARNINGS
    #include <windows.h>
#elif defined(__GBA__)
    #include <gba_console.h>
    #include <gba_video.h>
    #include <gba_timers.h>
    #include <gba_interrupt.h>
    #include <gba_systemcalls.h>
    #include <gba_input.h>
    #include <gba_dma.h>
    #include <gba_affine.h>
    #include <fade.h>
#elif defined(__TNS__)
    #include <os.h>
#endif

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <limits.h>

//#define DEBUG_OVERDRAW
//#define DEBUG_FACES

#if defined(__TNS__)
    #define WIDTH        SCREEN_WIDTH
    #define HEIGHT       SCREEN_HEIGHT
    #define FRAME_WIDTH  WIDTH
    #define FRAME_HEIGHT HEIGHT
    #define FOV_SHIFT    8
#else
    #define WIDTH        160
    #define HEIGHT       128
    #define FRAME_WIDTH  160
    #define FRAME_HEIGHT 128
    #define FOV_SHIFT    7
#endif

#if defined(_WIN32)
    #define INLINE    inline
    #define NOINLINE  __declspec(noinline)
#elif defined(__GBA__) || defined(__TNS__)
    #define INLINE    __attribute__((always_inline)) inline
    #define NOINLINE  __attribute__((noinline))
#endif

typedef signed char        int8;
typedef signed short       int16;
typedef signed int         int32;
typedef signed long long   int64;
typedef unsigned char      uint8;
typedef unsigned short     uint16;
typedef unsigned int       uint32;
typedef int16              Index;

#define PI      3.14159265358979323846f
#define PIH     (PI * 0.5f)
#define PI2     (PI * 2.0f)
#define DEG2RAD (PI / 180.0f)
#define RAD2DEG (180.0f / PI)

#ifdef __GBA__
    #define ARM_CODE    __attribute__((target("arm")))
    #define THUMB_CODE  __attribute__((target("thumb")))
    #define IWRAM_DATA  __attribute__((section(".iwram")))
    #define EWRAM_DATA  __attribute__((section(".ewram")))
    #define EWRAM_BSS   __attribute__((section(".sbss")))
    #define IWRAM_CODE  __attribute__((section(".iwram"), long_call))
    #define EWRAM_CODE  __attribute__((section(".ewram"), long_call))
#else
    #define ARM_CODE
    #define THUMB_CODE
    #define IWRAM_DATA
    #define EWRAM_DATA
    #define EWRAM_BSS
    #define IWRAM_CODE
    #define EWRAM_CODE

    #define dmaCopy(src,dst,size) memcpy(dst,src,size)
#endif

#if defined(_WIN32)
    #define ALIGN4      __declspec(align(4))
#elif defined(__GBA__)
    #define ALIGN4      __attribute__((aligned(4)))
#elif defined(__TNS__)
    #define ALIGN4      __attribute__((aligned(4)))
#endif

#if defined(_WIN32)
    #define ASSERT(x) { if (!(x)) { DebugBreak(); } }
#else
    #define ASSERT(x)
#endif

#ifdef PROFILE
    #if defined(_WIN32)

        extern LARGE_INTEGER g_timer;
        extern LARGE_INTEGER g_current;

        #define PROFILE_START() {\
            QueryPerformanceCounter(&g_timer);\
        }

        #define PROFILE_STOP(value) {\
            QueryPerformanceCounter(&g_current);\
            value += (g_current.QuadPart - g_timer.QuadPart);\
        }

    #elif defined(__GBA__)

        #ifdef TEST
            #define TIMER_FREQ_DIV 1
        #else
            #define TIMER_FREQ_DIV 3
        #endif

        #define PROFILE_START() {\
            REG_TM0CNT_L = 0;\
            REG_TM0CNT_H = (1 << 7) | TIMER_FREQ_DIV;\
        }

        #define PROFILE_STOP(value) {\
            value += REG_TM0CNT_L;\
            REG_TM0CNT_H = 0;\
        }

    #else
        #define PROFILE_START()
        #define PROFILE_STOP(value)
    #endif
#else
    #define PROFILE_START()
    #define PROFILE_STOP(value)
#endif

#ifdef __TNS__
    void SetPalette(unsigned short* palette);
#endif

enum InputKey {
    IK_UP,
    IK_RIGHT,
    IK_DOWN,
    IK_LEFT,
    IK_A,
    IK_B,
    IK_L,
    IK_R,
    IK_MAX
};

extern bool keys[IK_MAX];

struct vec3i {
    int32 x, y, z;

    vec3i() = default;
    INLINE vec3i(int32 x, int32 y, int32 z) : x(x), y(y), z(z) {}
};

struct vec3s {
    int16 x, y, z;
};

struct vec4i {
    int32 x, y, z, w;

    INLINE int32& operator [] (int32 index) const {
        ASSERT(index >= 0 && index <= 3);
        return ((int32*)this)[index];
    }
};

typedef vec4i Matrix[3];

struct Quad {
    Index  indices[4];
    uint16 flags;
};

struct Triangle {
    Index  indices[3];
    uint16 flags;
};

struct Box {
    int16 minX;
    int16 maxX;
    int16 minY;
    int16 maxY;
    int16 minZ;
    int16 maxZ;
};

struct RoomInfo {
    struct Vertex {
        vec3s  pos;
        uint16 lighting;
    };

    struct Sprite {
        Index  index;
        uint16 texture;
    };
    
    struct Portal {
        uint16 roomIndex;
        vec3s  n;
        vec3s  v[4];
    };

    struct Sector {
        uint16 floorIndex;
        uint16 boxIndex;
        uint8  roomBelow;
        int8   floor;
        uint8  roomAbove;
        int8   ceiling;
    };

    struct Light {
        // int32  x, y, z;
        // uint16 intensity;
        // uint32 radius;
        uint8 dummy[18];
    };

    struct Mesh {
        int16  pos[6]; // TODO align struct (int32 x, y, z)
        int16  rotation;
        uint16 intensity;
        uint16 staticMeshId;
    };

    int32 x;
    int32 z;
    int32 yBottom;
    int32 yTop;

    uint32 dataSize;
/*
    uint16    vCount;
    Vertex*   vertices;

    uint16    qCount;
    Quad*     quads;
    
    uint16    tCount;
    Triangle* triangles;
    
    uint16    sCount;
    Sprite*   sprites;
*/
};

struct Node {
    uint32 flags;
    vec3i  pos;
};

struct Model {
    uint32 type;
    uint16 mCount;
    uint16 mStart;
    uint32 nodeIndex;
    uint32 frameIndex;
    uint16 animIndex;
    uint16 _padding;
};

#define FILE_MODEL_SIZE (sizeof(Model) - 2) // -padding

struct StaticMesh {
    int32   id;
    uint16  meshIndex;
    Box     vbox;
    Box     cbox;
    uint16  flags;
};

struct ItemInfo { // 24
    uint16 type;
    int16  room;
    vec3i  pos;
    int16  angleY;
    uint16 intensity;

    union {
        struct {
            uint16 gravity:1; // TODO
        };
        uint16 value;
    } flags;

    uint16 _padding;
};

#define FILE_ITEM_SIZE (sizeof(ItemInfo) - 2)

struct Item : ItemInfo { // 24 + 20 = 44
    int16  angleX;
    int16  angleZ;

    uint16 vSpeed;
    uint16 hSpeed;

    uint16 animIndex;
    uint16 frameIndex;

    uint8  state;
    uint8  nextState;
    uint8  goalState;
    uint8  timer;

    uint16 health;
    uint8  nextItem;
    uint8  nextActive;
};

struct Anim {
    uint32  frameOffset;

    uint8   frameRate;
    uint8   frameSize;
    uint16  state;

    int32   speed;

    int32   accel;

    uint16  frameBegin;
    uint16  frameEnd;

    uint16  nextAnimIndex;
    uint16  nextFrameIndex;

    uint16  scCount;
    uint16  scOffset;

    uint16  acCount;
    uint16  animCommand;
};

struct Frame {
    Box    box;
    vec3s  pos;
    uint16 angles[1];
};

struct Texture {
    uint16  attribute;
    uint16  tile:14, :2;
    uint32  uv0;
    uint32  uv1;
    uint32  uv2;
    uint32  uv3;
};

struct Sprite {
    uint16  tile;
    uint8   u, v;
    uint16  w, h;
    int16   l, t, r, b;
};

struct SpriteSeq {
    uint16  type;
    uint16  unused;
    int16   sCount;
    int16   sStart;
};

struct Rect {
    int32 x0;
    int32 y0;
    int32 x1;
    int32 y1;
};

struct Vertex {
    int16 x, y, z;
    uint8 g, clip;
};

union UV {
    struct { uint16 v, u; };
    uint32 uv;
};

struct VertexUV {
    Vertex v;
    UV     t;
};

struct Face {
    uint16 flags;
    int16  depth;
    int16  start;
    int8   indices[4];
};

#ifdef PROFILE
    extern uint32 dbg_transform;
    extern uint32 dbg_poly;
    extern uint32 dbg_sort;
    extern uint32 dbg_flush;
    extern uint32 dbg_vert_count;
    extern uint32 dbg_poly_count;
#endif

#define DIV_TABLE_SIZE  256
#define FixedInvS(x)    ((x < 0) ? -divTable[abs(x)] : divTable[x])
#define FixedInvU(x)    divTable[x]

extern uint16 divTable[DIV_TABLE_SIZE];

#define FIXED_SHIFT     14

#define MAX_MATRICES    8
#define MAX_MODELS      64
#define MAX_ITEMS       256
#define MAX_MESHES      50
#define MAX_VERTICES    2048
#define MAX_FACES       512
#define FOG_SHIFT       1
#define FOG_MAX         (10 * 1024)
#define FOG_MIN         (FOG_MAX - (8192 >> FOG_SHIFT))
#define VIEW_MIN_F      (32 << FIXED_SHIFT)
#define VIEW_MAX_F      ((FOG_MAX - 1024) << FIXED_SHIFT)

#define FACE_TRIANGLE   0x8000
#define FACE_COLORED    0x4000
#define FACE_CLIPPED    0x2000
#define FACE_FLAT       0x1000
#define FACE_TEXTURE    0x0FFF

#define NO_ROOM         0xFF
#define NO_ITEM         0xFF
#define NO_MODEL        0xFF

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define SQR(x)   ((x) * (x))

#define DP43(a,b)  ((a).x * (b).x + (a).y * (b).y + (a).z * (b).z + (a).w)
#define DP33(a,b)  ((a).x * (b).x + (a).y * (b).y + (a).z * (b).z)

#define ITEM_LARA    0
#define ITEM_WOLF    7
#define ITEM_BEAR    8
#define ITEM_BAT     9
#define ITEM_CRYSTAL 83

extern vec3i  viewPos;
extern Matrix matrixStack[MAX_MATRICES];
extern int32  matrixStackIndex;

int32 clamp(int32 x, int32 a, int32 b);
int32 phd_sin(int32 x);
int32 phd_cos(int32 x);

void initLUT();

Matrix& matrixGet();
void matrixPush();
void matrixPop();
void matrixTranslate(int32 x, int32 y, int32 z);
void matrixTranslateAbs(int32 x, int32 y, int32 z);
void matrixRotateX(int32 angle);
void matrixRotateY(int32 angle);
void matrixRotateZ(int32 angle);
void matrixRotateYXZ(int32 angleX, int32 angleY, int32 angleZ);
void matrixFrame(int32 x, int32 y, int32 z, uint16* angles);
void matrixSetView(int32 x, int32 y, int32 z, int32 angleX, int32 angleY);

void drawGlyph(const Sprite *sprite, int32 x, int32 y);

void clear();
bool boxIsVisible(const Box* box);
void transformRoom(const RoomInfo::Vertex* vertices, int32 vCount);
void transformMesh(const vec3s* vertices, int32 vCount, uint16 intensity);
void faceAddRoom(const Quad* quads, int32 qCount, const Triangle* triangles, int32 tCount, int32 startVertex);
void faceAddMesh(const Quad* rFaces, const Quad* crFaces, const Triangle* tFaces, const Triangle* ctFaces, int32 rCount, int32 crCount, int32 tCount, int32 ctCount, int32 startVertex);

void flush();

void readLevel(const uint8 *data);
const RoomInfo::Sector* getSector(int32 roomIndex, int32 x, int32 z);
int32 getRoomIndex(int32 roomIndex, const vec3i* pos);

#endif
