#ifndef H_COMMON
#define H_COMMON

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#else
#include <gba_console.h>
#include <gba_video.h>
#include <gba_timers.h>
#include <gba_interrupt.h>
#include <gba_systemcalls.h>
#include <gba_input.h>
#include <gba_dma.h>
#include <gba_affine.h>
#include <fade.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

//#define USE_MODE_5
//#define DEBUG_OVERDRAW

#define SCALE   1

#ifdef USE_MODE_5
    #define WIDTH        160
    #define HEIGHT       128
    #define FRAME_WIDTH  160
    #define FRAME_HEIGHT 128
    #define PIXEL_SIZE   1
#else // MODE_4
    #define WIDTH        240
    #define HEIGHT       160
    #define FRAME_WIDTH  (240/SCALE)
    #define FRAME_HEIGHT (160/SCALE)
    #define PIXEL_SIZE   2
#endif

#ifdef _WIN32
    #define INLINE inline
#else
    #define INLINE __attribute__((always_inline)) inline
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

#ifdef _WIN32
    #define IWRAM_CODE
    #define EWRAM_DATA

    #define dmaCopy(src,dst,size) memcpy(dst,src,size)
    #define ALIGN4
#else
    #define ALIGN4 __attribute__ ((aligned (4)))
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

struct Room {
    struct Info {
        int32 x, z;
        int32 yBottom, yTop;
    };

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
        // int32  x, y, z;
        // uint16 rotation;
        // uint16 intensity;
        // uint16 meshID;
        uint8 dummy[18];
    };

    Info   info;
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
    uint32 node;
    uint32 frame;
    uint16 animation;
    uint16 paddding;
};

#define FILE_MODEL_SIZE (sizeof(Model) - 2) // -padding

struct Entity {
    uint16 type;
    uint16 room;
    vec3i  pos;
    int16  rotation;
    uint16 flags;
};

struct EntityDesc { // 32 bytes
    uint16 type;
    uint16 flags;

    vec3i  pos;

    vec3s  rot;
    uint8  state;
    uint8  targetState;

    uint8  vSpeed;
    uint8  hSpeed;
    uint8  room;
    uint8  modelIndex;

    uint16 animIndex;
    uint16 frameIndex;
};

struct Texture {
    uint16  attribute;
    uint16  tile:14, :2;
    uint8   xh0, x0, yh0, y0;
    uint8   xh1, x1, yh1, y1;
    uint8   xh2, x2, yh2, y2;
    uint8   xh3, x3, yh3, y3;
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
    uint8 u, v;
};

struct Face {
    uint16 flags;
    int16  depth;
    int16  start;
    int8   indices[4];
};

#define FIXED_SHIFT     14
#define FOV_SHIFT       (7 - (SCALE - 1))

#define MAX_MATRICES    8
#define MAX_MODELS      64
#define MAX_ENTITY      190
#define MAX_VERTICES    1024
#define MAX_FACES       384
#define FOG_SHIFT       2
#define FOG_MAX         (10 * 1024)
#define FOG_MIN         (FOG_MAX - (8192 >> FOG_SHIFT))
#define VIEW_MIN_F      (32 << FIXED_SHIFT)
#define VIEW_MAX_F      (FOG_MAX << FIXED_SHIFT)

#define FACE_TRIANGLE   0x8000
#define FACE_COLORED    0x4000
#define FACE_TEXTURE    0x3FFF

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define SQR(x)   ((x) * (x))

#define DP43(a,b)  ((a).x * (b).x + (a).y * (b).y + (a).z * (b).z + (a).w)
#define DP33(a,b)  ((a).x * (b).x + (a).y * (b).y + (a).z * (b).z)

int32 clamp(int32 x, int32 a, int32 b);
int32 phd_sin(int32 x);
int32 phd_cos(int32 x);

Matrix& matrixGet();
void matrixPush();
void matrixPop();
void matrixTranslate(const vec3i &offset);
void matrixTranslateAbs(const vec3i &offset);
void matrixRotate(int16 rotX, int16 rotY, int16 rotZ);
void matrixSetView(const vec3i &pos, int16 rotX, int16 rotY);

void drawGlyph(const Sprite *sprite, int32 x, int32 y);

void clear();
void transform(const vec3s &v, int32 vg);
void faceAddTriangle(uint16 flags, const Index* indices, int32 startVertex);
void faceAddQuad(uint16 flags, const Index* indices, int32 startVertex);
void flush();
void initRender();

void readLevel(const uint8 *data);
const Room::Sector* getSector(int32 roomIndex, int32 x, int32 z);
int32 getRoomIndex(int32 roomIndex, const vec3i &pos);

#endif