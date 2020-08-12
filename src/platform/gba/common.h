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

#ifdef USE_MODE_5
    #define WIDTH        160
    #define HEIGHT       128
    #define FRAME_WIDTH  160
    #define FRAME_HEIGHT 128
    #define PIXEL_SIZE   1
#else // MODE_4
    #define WIDTH        240
    #define HEIGHT       160
    #define FRAME_WIDTH  240
    #define FRAME_HEIGHT 160
    #define PIXEL_SIZE   2
#endif

#define WND_SCALE   4

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
    extern uint8* LEVEL1_PHD;

    extern uint32  VRAM[WIDTH * HEIGHT];

    #ifdef USE_MODE_5
        extern uint16  fb[WIDTH * HEIGHT];
    #else
        extern uint8   fb[WIDTH * HEIGHT];
    #endif

    #define IWRAM_CODE
    #define EWRAM_DATA

    #define dmaCopy(src,dst,size) memcpy(dst,src,size)
    #define ALIGN4

    struct ObjAffineSource {
         int16 sX;
         int16 sY;
         uint16 theta;
    };

    struct ObjAffineDest {
         int16 pa;
         int16 pb;
         int16 pc;
         int16 pd;
    };

    static void ObjAffineSet(ObjAffineSource *source, ObjAffineDest *dest, int32 num, int32 offset) {
        float ang = (source->theta >> 8) * PI / 128.0f;

        int32 c = int32(cosf(ang) * 16384.0f);
        int32 s = int32(sinf(ang) * 16384.0f);

        dest->pa = ( source->sX * c) >> 14;
        dest->pb = (-source->sX * s) >> 14;
        dest->pc = ( source->sY * s) >> 14;
        dest->pd = ( source->sY * c) >> 14;
    }

#else
    #define ALIGN4 __attribute__ ((aligned (4)))

    extern uint32 fb;
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
        int16 x, y, z;
        uint16 lighting;
    };

    struct Sprite {
        Index  index;
        uint16 texture;
    };
    
    struct Portal {
        struct Vertex {
            int16 x, y, z;
        };

        uint16 roomIndex;
        Vertex n;
        Vertex v[4];
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
    int32 x, y, z;
};

struct Model {
    uint16 type;
    uint16 index;
    uint16 mCount;
    uint16 mStart;
    uint32 node;
    uint32 frame;
    uint16 animation;
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

struct SpriteSequence {
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
    int16 x, y;
    int16 z;
    uint8 u, v, g, clip;
};

struct Face {
    uint16 flags;
    int16  depth;
    int16  start;
    int8   indices[4];
};

#define MAX_VERTICES    1024
#define MAX_FACES       384
#define MAX_DIST        (16 * 1024)

#define FACE_TRIANGLE   0x8000
#define FACE_COLORED    0x4000
#define FACE_TEXTURE    0x3FFF

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

void drawGlyph(int32 index, int32 x, int32 y);
void clear();

#endif