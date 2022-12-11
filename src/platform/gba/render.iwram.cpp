#include "common.h"

struct Vertex
{
    int16 x;
    int16 y;
    int16 z;
    uint8 g;
    uint8 clip;
};

struct VertexLink
{
    Vertex v;
    TexCoord t;
    int8 prev;
    int8 next;
    uint16 padding;
};

struct ViewportRel {
    int32 minXY;
    int32 maxXY;
};

ViewportRel viewportRel;

#if defined(__GBA_WIN__)
    uint16 fb[FRAME_WIDTH * FRAME_HEIGHT];
#elif defined(__GBA__)
    uint32 fb = MEM_VRAM;
#elif defined(__TNS__)
    uint16 fb[FRAME_WIDTH * FRAME_HEIGHT];
#elif defined(__DOS__)
    uint16 fb[FRAME_WIDTH * FRAME_HEIGHT];
#endif

enum FaceType {
    FACE_TYPE_SHADOW,
    FACE_TYPE_F,
    FACE_TYPE_FT,
    FACE_TYPE_FTA,
    FACE_TYPE_GT,
    FACE_TYPE_GTA,
    FACE_TYPE_SPRITE,
    FACE_TYPE_FILL_S,
    FACE_TYPE_LINE_H,
    FACE_TYPE_LINE_V,
    FACE_TYPE_MAX
};

#define FACE_TRIANGLE   (1 << 19)
#define FACE_CLIPPED    (1 << 18)
#define FACE_TYPE_SHIFT 14
#define FACE_TYPE_MASK  15
#define FACE_GOURAUD    (2 << FACE_TYPE_SHIFT)
#define FACE_TEXTURE    0x3FFF

#include "rasterizer.h"

extern Level level;

const uint8* gTile;

EWRAM_DATA uint8 gBackgroundCopy[FRAME_WIDTH * FRAME_HEIGHT];   // EWRAM 37.5k
EWRAM_DATA ALIGN8 Vertex gVertices[MAX_VERTICES];               // EWRAM 16k
EWRAM_DATA Face gFaces[MAX_FACES];                              // EWRAM 30k
Face* gOT[OT_SIZE];                                             // IWRAM 2.5k

Vertex* gVerticesBase = gVertices;
Face* gFacesBase = gFaces;

#if defined(USE_VRAM_MESH) || defined(USE_VRAM_ROOM)
uint8* vramPtr;
#endif

enum ClipFlags {
    CLIP_LEFT    = 1 << 0,
    CLIP_RIGHT   = 1 << 1,
    CLIP_TOP     = 1 << 2,
    CLIP_BOTTOM  = 1 << 3,
    CLIP_PLANE   = 1 << 4,
    CLIP_FRAME   = 1 << 5,
    CLIP_DISCARD = (CLIP_LEFT | CLIP_RIGHT | CLIP_TOP | CLIP_BOTTOM | CLIP_PLANE),
};

const MeshQuad gShadowQuads[] = {
    { {0, 1, 1, 5}, (FACE_TYPE_SHADOW << FACE_TYPE_SHIFT) },    // abs idx {0, 1, 2, 7}
    { {0,-5, 1, 3}, (FACE_TYPE_SHADOW << FACE_TYPE_SHIFT) },    // abs idx {7, 2, 3, 6}
    { {0,-3, 1, 1}, (FACE_TYPE_SHADOW << FACE_TYPE_SHIFT) }     // abs idx {6, 3, 4, 5}
};

void setViewport(const RectMinMax &vp)
{
    viewport = vp;

    int32 minX = vp.x0 - (FRAME_WIDTH  >> 1);
    int32 minY = vp.y0 - (FRAME_HEIGHT >> 1);
    int32 maxX = vp.x1 - (FRAME_WIDTH  >> 1);
    int32 maxY = vp.y1 - (FRAME_HEIGHT >> 1);

    viewportRel.minXY = (minX << 16) | (minY & 0xFFFF);
    viewportRel.maxXY = (maxX << 16) | (maxY & 0xFFFF);
}

void setPaletteIndex(int32 index)
{
    // TODO
}

X_INLINE Face* faceAdd(int32 depth)
{
    ASSERT(depth >= 0 && depth < OT_SIZE);

    Face* face = gFacesBase++;
    face->next = gOT[depth];
    gOT[depth] = face;

    return face;
}

extern "C" {
    X_NOINLINE void drawPoly(uint32 flags, VertexLink* v);
}

#ifdef USE_ASM
    #define transformRoom           transformRoom_asm
    #define transformRoomUW         transformRoomUW_asm
    #define transformMesh           transformMesh_asm
    #define faceAddRoomQuads        faceAddRoomQuads_asm
    #define faceAddRoomTriangles    faceAddRoomTriangles_asm
    #define faceAddMeshQuads        faceAddMeshQuads_asm
    #define faceAddMeshTriangles    faceAddMeshTriangles_asm
    #define rasterize               rasterize_asm
    #define clearFB                 clearFB_asm

    extern "C" {
        void transformRoom_asm(const RoomVertex* vertices, int32 count);
        void transformRoomUW_asm(const RoomVertex* vertices, int32 count);
        void transformMesh_asm(const MeshVertex* vertices, int32 count, int32 intensity);
        void faceAddRoomQuads_asm(const RoomQuad* polys, int32 count);
        void faceAddRoomTriangles_asm(const RoomTriangle* polys, int32 count);
        void faceAddMeshQuads_asm(const MeshQuad* polys, int32 count);
        void faceAddMeshTriangles_asm(const MeshTriangle* polys, int32 count);
        void rasterize_asm(uint32 flags, VertexLink* top);
        void clearFB_asm(void* fb);
    }
#else
    #define transformRoom           transformRoom_c
    #define transformRoomUW         transformRoomUW_c
    #define transformMesh           transformMesh_c
    #define faceAddRoomQuads        faceAddRoomQuads_c
    #define faceAddRoomTriangles    faceAddRoomTriangles_c
    #define faceAddMeshQuads        faceAddMeshQuads_c
    #define faceAddMeshTriangles    faceAddMeshTriangles_c
    #define rasterize               rasterize_c
    #define clearFB(fb)             dmaFill(fb, 0, FRAME_WIDTH * FRAME_HEIGHT)

X_INLINE bool checkBackface(const Vertex* a, const Vertex* b, const Vertex* c)
{
    return (b->x - a->x) * (c->y - a->y) <= (c->x - a->x) * (b->y - a->y);
}

void transformRoom_c(const RoomVertex* vertices, int32 count)
{
    Vertex* res = gVerticesBase;

    for (int32 i = 0; i < count; i++, res++)
    {
        uint32 value = *(uint32*)(vertices++);

        int32 vx = (value & (0xFF)) << 8;
        int32 vy = (value & (0xFF << 8));
        int32 vz = (value & (0xFF << 16)) >> 8;
        int32 vg = (value & (0xFF << 24)) >> (24 + 3);

        const Matrix &m = matrixGet();
        int32 x = DP43(m.e00, m.e01, m.e02, m.e03, vx, vy, vz);
        int32 y = DP43(m.e10, m.e11, m.e12, m.e13, vx, vy, vz);
        int32 z = DP43(m.e20, m.e21, m.e22, m.e23, vx, vy, vz);

        uint32 clip = 0;

        if (z <= VIEW_MIN_F) {
            clip = CLIP_PLANE;
            z = VIEW_MIN_F;
        }

        if (z >= VIEW_MAX_F) {
            clip = CLIP_PLANE;
            z = VIEW_MAX_F;
        }

        x >>= FIXED_SHIFT;
        y >>= FIXED_SHIFT;
        z >>= FIXED_SHIFT;

        int32 fog = z - FOG_MIN;
        if (fog > 0)
        {
            vg += fog >> (FOG_SHIFT + 3);
            if (vg > 31)
            {
                vg = 31;
            }
        }

        PERSPECTIVE(x, y, z);

        // use this in case of overflow
        //x = X_CLAMP(x, -512, 512);
        //y = X_CLAMP(y, -512, 512);

        x += (FRAME_WIDTH  >> 1);
        y += (FRAME_HEIGHT >> 1);

        if ((x < 0 || x > FRAME_WIDTH) || (y < 0 || y > FRAME_HEIGHT)) {
            clip |= CLIP_FRAME;
        }

        if (x < viewport.x0) clip |= CLIP_LEFT;
        if (x > viewport.x1) clip |= CLIP_RIGHT;
        if (y < viewport.y0) clip |= CLIP_TOP;
        if (y > viewport.y1) clip |= CLIP_BOTTOM;

        res->x = x;
        res->y = y;
        res->z = z;
        res->g = vg >> 8;
        res->clip = clip;
    }
}

void transformRoomUW_c(const RoomVertex* vertices, int32 count)
{
    Vertex* res = gVerticesBase;

    for (int32 i = 0; i < count; i++, res++)
    {
        uint32 value = *(uint32*)(vertices++);

        int32 vx = (value & (0xFF)) << 8;
        int32 vy = (value & (0xFF << 8));
        int32 vz = (value & (0xFF << 16)) >> 8;
        int32 vg = (value & (0xFF << 24)) >> (24 - 5);

        const Matrix &m = matrixGet();
        int32 x = DP43(m.e00, m.e01, m.e02, m.e03, vx, vy, vz);
        int32 y = DP43(m.e10, m.e11, m.e12, m.e13, vx, vy, vz);
        int32 z = DP43(m.e20, m.e21, m.e22, m.e23, vx, vy, vz);

        uint32 clip = 0;

        if (z <= VIEW_MIN_F) {
            clip = CLIP_PLANE;
            z = VIEW_MIN_F;
        }

        if (z >= VIEW_MAX_F) {
            clip = CLIP_PLANE;
            z = VIEW_MAX_F;
        }

        int32 causticsValue = gCaustics[(gRandTable[i & (MAX_RAND_TABLE - 1)] + gCausticsFrame) & (MAX_CAUSTICS - 1)];
        vg = X_CLAMP(vg + causticsValue, 0, 8191);

        x >>= FIXED_SHIFT;
        y >>= FIXED_SHIFT;
        z >>= FIXED_SHIFT;

        if (z > FOG_MIN)
        {
            vg += (z - FOG_MIN) << FOG_SHIFT;
            if (vg > 8191) {
                vg = 8191;
            }
        }

        PERSPECTIVE(x, y, z);

        x += (FRAME_WIDTH  >> 1);
        y += (FRAME_HEIGHT >> 1);

        if ((x < 0 || x > FRAME_WIDTH) || (y < 0 || y > FRAME_HEIGHT)) {
            clip |= CLIP_FRAME;
        }

        if (x < viewport.x0) clip |= CLIP_LEFT;
        if (x > viewport.x1) clip |= CLIP_RIGHT;
        if (y < viewport.y0) clip |= CLIP_TOP;
        if (y > viewport.y1) clip |= CLIP_BOTTOM;

        res->x = x;
        res->y = y;
        res->z = z;
        res->g = vg >> 8;
        res->clip = clip;
    }
}

void transformMesh_c(const MeshVertex* vertices, int32 count, int32 intensity)
{
    Vertex* res = gVerticesBase;

    int32 vg = X_CLAMP((intensity + gLightAmbient) >> 8, 0, 31);

    for (int32 i = 0; i < count; i++, res++)
    {
        int32 vx = vertices->x << 2;
        int32 vy = vertices->y << 2;
        int32 vz = vertices->z << 2;
        vertices++;

        const Matrix &m = matrixGet();
        int32 x = DP43(m.e00, m.e01, m.e02, m.e03, vx, vy, vz);
        int32 y = DP43(m.e10, m.e11, m.e12, m.e13, vx, vy, vz);
        int32 z = DP43(m.e20, m.e21, m.e22, m.e23, vx, vy, vz);
 
        uint32 clip = 0;

        if (z <= VIEW_MIN_F) {
            clip = CLIP_PLANE;
            z = VIEW_MIN_F;
        }

        if (z >= VIEW_MAX_F) {
            clip = CLIP_PLANE;
            z = VIEW_MAX_F;
        }

        x >>= FIXED_SHIFT;
        y >>= FIXED_SHIFT;
        z >>= FIXED_SHIFT;

        PERSPECTIVE(x, y, z);

        x += (FRAME_WIDTH  >> 1);
        y += (FRAME_HEIGHT >> 1);

        if ((x < 0 || x > FRAME_WIDTH) || (y < 0 || y > FRAME_HEIGHT)) {
            clip |= CLIP_FRAME;
        }

        res->x = x;
        res->y = y;
        res->z = z;
        res->g = vg;
        res->clip = clip;
    }
}

void faceAddRoomQuads_c(const RoomQuad* polys, int32 count)
{
    const Vertex* v0;
    const Vertex* v1;
    const Vertex* v2;
    const Vertex* v3 = gVerticesBase;

    for (int32 i = 0; i < count; i++, polys++)
    {
        uint32 flags = polys->flags;
        v0 = v3 + polys->indices[0];
        v1 = v0 + polys->indices[1];
        v2 = v1 + polys->indices[2];
        v3 = v2 + polys->indices[3];

        uint32 c0 = v0->clip;
        uint32 c1 = v1->clip;
        uint32 c2 = v2->clip;
        uint32 c3 = v3->clip;

        if (c0 & c1 & c2 & c3 & CLIP_DISCARD)
            continue;

        if ((c0 | c1 | c2 | c3) & CLIP_FRAME) {
            flags |= FACE_CLIPPED;
        }

        uint32 g0 = v0->g;
        uint32 g1 = v1->g;
        uint32 g2 = v2->g;
        uint32 g3 = v3->g;

        if (g0 != g1 || g0 != g2 || g0 != g3) {
            flags += FACE_GOURAUD;
        }

        if (checkBackface(v0, v1, v2))
            continue;

        int32 depth = X_MAX(v0->z, X_MAX(v1->z, X_MAX(v2->z, v3->z))) >> OT_SHIFT;

        Face* f = faceAdd(depth);
        f->flags = flags;
        f->indices[0] = v0 - gVertices;
        f->indices[1] = v1 - gVertices;
        f->indices[2] = v2 - gVertices;
        f->indices[3] = v3 - gVertices;
    }
}

void faceAddRoomTriangles_c(const RoomTriangle* polys, int32 count)
{
    const Vertex* v = gVerticesBase;

    for (int32 i = 0; i < count; i++, polys++)
    {
        uint32 flags = polys->flags;
        const Vertex* v0 = v + polys->indices[0];
        const Vertex* v1 = v + polys->indices[1];
        const Vertex* v2 = v + polys->indices[2];

        uint32 c0 = v0->clip;
        uint32 c1 = v1->clip;
        uint32 c2 = v2->clip;

        if (c0 & c1 & c2 & CLIP_DISCARD)
            continue;

        if ((c0 | c1 | c2) & CLIP_FRAME) {
            flags |= FACE_CLIPPED;
        }

        uint32 g0 = v0->g;
        uint32 g1 = v1->g;
        uint32 g2 = v2->g;

        if (g0 != g1 || g0 != g2) {
            flags += FACE_GOURAUD;
        }
        flags |= FACE_TRIANGLE;

        if (checkBackface(v0, v1, v2))
            continue;

        int32 depth = X_MAX(v0->z, X_MAX(v1->z, v2->z)) >> OT_SHIFT;

        Face* f = faceAdd(depth);
        f->flags = flags;
        f->indices[0] = v0 - gVertices;
        f->indices[1] = v1 - gVertices;
        f->indices[2] = v2 - gVertices;
    }
}

void faceAddMeshQuads_c(const MeshQuad* polys, int32 count)
{
    const Vertex* v0;
    const Vertex* v1;
    const Vertex* v2;
    const Vertex* v3 = gVerticesBase;

    for (int32 i = 0; i < count; i++, polys++)
    {
        v0 = v3 + polys->indices[0];
        v1 = v0 + polys->indices[1];
        v2 = v1 + polys->indices[2];
        v3 = v2 + polys->indices[3];

        if (checkBackface(v0, v1, v2))
            continue;

        uint32 c0 = v0->clip;
        uint32 c1 = v1->clip;
        uint32 c2 = v2->clip;
        uint32 c3 = v3->clip;

        if (c0 & c1 & c2 & c3 & CLIP_DISCARD)
            continue;

        uint32 flags = polys->flags;
        if ((c0 | c1 | c2 | c3) & CLIP_FRAME) {
            flags |= FACE_CLIPPED;
        }

        int32 depth = (v0->z + v1->z + v2->z + v3->z) >> (2 + OT_SHIFT);

        Face* f = faceAdd(depth);
        f->flags = flags;
        f->indices[0] = v0 - gVertices;
        f->indices[1] = v1 - gVertices;
        f->indices[2] = v2 - gVertices;
        f->indices[3] = v3 - gVertices;
    }
}

void faceAddMeshTriangles_c(const MeshTriangle* polys, int32 count)
{
    const Vertex* v0;
    const Vertex* v1;
    const Vertex* v2 = gVerticesBase;

    for (int32 i = 0; i < count; i++, polys++)
    {
        v0 = v2 + polys->indices[0];
        v1 = v0 + polys->indices[1];
        v2 = v1 + polys->indices[3];

        if (checkBackface(v0, v1, v2))
            continue;

        uint32 c0 = v0->clip;
        uint32 c1 = v1->clip;
        uint32 c2 = v2->clip;

        if (c0 & c1 & c2 & CLIP_DISCARD)
            continue;

        uint32 flags = polys->flags;
        if ((c0 | c1 | c2) & CLIP_FRAME) {
            flags |= FACE_CLIPPED;
        }
        flags |= FACE_TRIANGLE;

        int32 depth = (v0->z + v1->z + v2->z + v2->z) >> (2 + OT_SHIFT);

        Face* f = faceAdd(depth);
        f->flags = flags;
        f->indices[0] = v0 - gVertices;
        f->indices[1] = v1 - gVertices;
        f->indices[2] = v2 - gVertices;
    }
}

int32 sphereIsVisible_c(int32 sx, int32 sy, int32 sz, int32 r)
{
    Matrix &m = matrixGet();

    if (abs(sx) < r && abs(sy) < r && abs(sz) < r)
        return 1;

    int32 z = DP33(m.e20, m.e21, m.e22, sx, sy, sz);

    if (z < 0)
        return 0;

    int32 x = DP33(m.e00, m.e01, m.e02, sx, sy, sz);
    int32 y = DP33(m.e10, m.e11, m.e12, sx, sy, sz);

    x >>= FIXED_SHIFT;
    y >>= FIXED_SHIFT;
    z >>= FIXED_SHIFT;

    z = PERSPECTIVE_DZ(z);
    if (z >= DIV_TABLE_SIZE) z = DIV_TABLE_SIZE - 1;
    int32 d = FixedInvU(z);
    x = (x * d) >> 12;
    y = (y * d) >> 12;
    r = (r * d) >> 12;

    x += (FRAME_WIDTH  / 2);
    y += (FRAME_HEIGHT / 2);

    int32 rMinX = x - r;
    int32 rMinY = y - r;
    int32 rMaxX = x + r;
    int32 rMaxY = y + r;

    if (rMinX > viewport.x1 ||
        rMaxX < viewport.x0 ||
        rMinY > viewport.y1 ||
        rMaxY < viewport.y0) return 0; // not visible

    return 1;
}

typedef void (*RasterProc)(uint16* pixel, const VertexLink* L, const VertexLink* R);

RasterProc gRasterProc[FACE_TYPE_MAX] = { // IWRAM
    rasterizeS,
    rasterizeF,
    rasterizeFT,
    rasterizeFTA,
    rasterizeGT,
    rasterizeGTA,
    rasterizeSprite,
    rasterizeFillS,
    rasterizeLineH,
    rasterizeLineV
};

X_NOINLINE void rasterize_c(uint32 flags, VertexLink* top)
{
    uint8* pixel = (uint8*)fb + top->v.y * FRAME_WIDTH;

    uint32 type = (flags >> FACE_TYPE_SHIFT) & FACE_TYPE_MASK;

    VertexLink* R = (type == FACE_TYPE_F) ? (VertexLink*)(flags & 0xFF) : top;

    gRasterProc[type]((uint16*)pixel, top, R);
}

void flush_c()
{
#ifdef PROFILING
    #if !defined(PROFILE_FRAMETIME) && !defined(PROFILE_SOUNDTIME)
        gCounters[CNT_VERT] += gVerticesBase - gVertices;
        gCounters[CNT_POLY] += gFacesBase - gFaces;
    #endif
#endif

    gVerticesBase = gVertices;

    if (gFacesBase == gFaces)
        return;

    gFacesBase = gFaces;

    VertexLink v[8];
    VertexLink* q = v;
    VertexLink* t = v + 4;
    // quad
    q[0].prev = 3;
    q[0].next = 1;
    q[1].prev = -1;
    q[1].next = 1;
    q[2].prev = -1;
    q[2].next = 1;
    q[3].prev = -1;
    q[3].next = -3;
    // triangle
    t[0].prev = 2;
    t[0].next = 1;
    t[1].prev = -1;
    t[1].next = 1;
    t[2].prev = -1;
    t[2].next = -2;
    // t[3] dummy

    PROFILE(CNT_FLUSH);

    for (int32 i = OT_SIZE - 1; i >= 0; i--)
    {
        if (!gOT[i]) continue;

        Face *face = gOT[i];
        gOT[i] = NULL;

        do {
            uint32 flags = face->flags;

            uint32 type = (flags >> FACE_TYPE_SHIFT) & FACE_TYPE_MASK;

            if (type <= FACE_TYPE_GTA)
            {
                VertexLink* ptr = (flags & FACE_TRIANGLE) ? t : q;

                if (type > FACE_TYPE_F)
                {
                    const Texture &tex = level.textures[flags & FACE_TEXTURE];
                    gTile = (uint8*)tex.tile;

                    ptr[0].t.t = 0xFF00FF00 & (tex.uv01);
                    ptr[1].t.t = 0xFF00FF00 & (tex.uv01 << 8);
                    ptr[2].t.t = 0xFF00FF00 & (tex.uv23);
                    ptr[3].t.t = 0xFF00FF00 & (tex.uv23 << 8);
                }

                ptr[0].v = gVertices[face->indices[0]];
                ptr[1].v = gVertices[face->indices[1]];
                ptr[2].v = gVertices[face->indices[2]];
                if (!(flags & FACE_TRIANGLE)) {
                    ptr[3].v = gVertices[face->indices[3]];
                }

                if (flags & FACE_CLIPPED) {
                    drawPoly(flags, ptr);
                } else {
                    // get top vertex for tri or quad
                    VertexLink* top = ptr;
                    if (top->v.y > ptr[1].v.y) top = ptr + 1;
                    if (top->v.y > ptr[2].v.y) top = ptr + 2;
                    if (!(flags & FACE_TRIANGLE))
                    {
                        if (top->v.y > v[3].v.y) top = ptr + 3;
                    }
                    rasterize(flags, top);
                }
            }
            else
            {
                const Vertex *vert = gVertices + face->indices[0];
                v[0].v = vert[0];
                v[1].v = vert[1];

                if (type == FACE_TYPE_SPRITE)
                {
                    const Sprite &sprite = level.sprites[flags & FACE_TEXTURE];
                    gTile = (uint8*)sprite.tile;
                    v[0].t.t = (sprite.uwvh) & (0xFF00FF00);
                    v[1].t.t = (sprite.uwvh) & (0xFF00FF00 >> 8);
                }

                rasterize(flags, v);
            }

            face = face->next;

        } while (face);
    }
}
#endif

extern "C" X_NOINLINE void drawPoly(uint32 flags, VertexLink* v)
{
    #define LERP_SHIFT          6
    #define LERP(a,b,t)         (b + ((a - b) * t >> LERP_SHIFT))
    //#define LERP2(a,b,ta,tb)    LERP(a,b,t)
    #define LERP2(a,b,ta,tb)    (b + (((a - b) * ta / tb) >> LERP_SHIFT) ) // less gaps between clipped polys, but slow // @DIV

    #define CLIP_AXIS(X, Y, edge, output) {\
        int32 ta = (edge - b->v.X) << LERP_SHIFT;\
        int32 tb = (a->v.X - b->v.X);\
        ASSERT(tb != 0);\
        int32 t = ta / tb;\
        ASSERT(count < 8);\
        VertexLink* p = output + count++;\
        p->v.X = edge;\
        p->v.Y = LERP2(a->v.Y, b->v.Y, ta, tb);\
        p->v.g = LERP(a->v.g, b->v.g, t);\
        p->t.uv.u = LERP(a->t.uv.u, b->t.uv.u, t);\
        p->t.uv.v = LERP(a->t.uv.v, b->t.uv.v, t);\
    }

    #define CLIP_XY(X, Y, X0, X1, input, output) {\
        const VertexLink *a, *b = input + pCount - 1;\
        for (int32 i = 0; i < pCount; i++) {\
            a = b;\
            b = input + i;\
            if (a->v.X < X0) {\
                if (b->v.X < X0) continue;\
                CLIP_AXIS(X, Y, X0, output);\
            } else if (a->v.X > X1) {\
                if (b->v.X > X1) continue;\
                CLIP_AXIS(X, Y, X1, output);\
            }\
            if (b->v.X < X0) {\
                CLIP_AXIS(X, Y, X0, output);\
            } else if (b->v.X > X1) {\
                CLIP_AXIS(X, Y, X1, output);\
            } else {\
                ASSERT(count < 8);\
                output[count++] = *b;\
            }\
        }\
        if (count < 3) return;\
    }

    VertexLink tmp[8];
    VertexLink out[8];

    int32 pCount = (flags & FACE_TRIANGLE) ? 3 : 4;
    int32 count = 0;

    // clip x
    CLIP_XY(x, y, 0, FRAME_WIDTH, v, tmp);

    pCount = count;
    count = 0;

    // clip y
    CLIP_XY(y, x, 0, FRAME_HEIGHT, tmp, out);

    VertexLink* first = out;
    VertexLink* last = out + count - 1;

    bool skip = (first->v.y == last->v.y);

    VertexLink* top = (first->v.y < last->v.y) ? first : last;
    first->prev = count - 1;
    first->next = 1;
    last->prev = -1;
    last->next = 1 - count;

    for (int32 i = 1; i < count - 1; i++)
    {
        VertexLink* p = out + i;

        if (p->v.y != top->v.y)
        {
            if (p->v.y < top->v.y)
            {
                top = p;
            }
            skip = false;
        }

        p->prev = -1;
        p->next = 1;
    }

    if (skip)
        return;

    rasterize(flags, top);
}

void faceAddRoom(const Room* room)
{
    if (room->info->quadsCount > 0) {
        faceAddRoomQuads(room->data.quads, room->info->quadsCount);
    }

    if (room->info->trianglesCount > 0) {
        faceAddRoomTriangles(room->data.triangles, room->info->trianglesCount);
    }
}

void faceAddMesh(const MeshQuad* quads, const MeshTriangle* triangles, int32 qCount, int32 tCount)
{
    if (qCount > 0) {
        faceAddMeshQuads(quads, qCount);
    }

    if (tCount > 0) {
        faceAddMeshTriangles(triangles, tCount);
    }
}

void clear()
{
    clearFB((void*)fb);
}

void renderRoom(Room* room)
{
    int32 vCount = room->info->verticesCount;
    if (vCount <= 0)
        return;

    if ((gVerticesBase - gVertices) + vCount > MAX_VERTICES)
    {
        ASSERT(false);
        return;
    }

#ifdef USE_VRAM_ROOM
    if (playersExtra[0].camera.view.room == room && !((uint32)room->data.vertices & 0x06000000))
    {
        memcpy(vramPtr, room->data.quads, room->info->quadsCount * sizeof(RoomQuad));
        room->data.quads = (RoomQuad*)vramPtr;
        vramPtr += room->info->quadsCount * sizeof(RoomQuad);

        if ((uint32)vramPtr & 3) vramPtr += 2;

        memcpy(vramPtr, room->data.triangles, room->info->trianglesCount * sizeof(RoomTriangle));
        room->data.triangles = (RoomTriangle*)vramPtr;
        vramPtr += room->info->trianglesCount * sizeof(RoomTriangle);

        if ((uint32)vramPtr & 3) vramPtr += 2;

        memcpy(vramPtr, room->data.vertices, room->info->verticesCount * sizeof(RoomVertex));
        room->data.vertices = (RoomVertex*)vramPtr;
        vramPtr += room->info->verticesCount * sizeof(RoomVertex);
    }
#endif

    {
        PROFILE(CNT_TRANSFORM);
        if (ROOM_FLAG_WATER(room->info->flags)) {
            transformRoomUW(room->data.vertices, vCount);
        } else {
            transformRoom(room->data.vertices, vCount);
        }
    }

    {
        PROFILE(CNT_ADD);
        faceAddRoom(room);
    }

    gVerticesBase += vCount;
}

void renderMesh(const Mesh* mesh)
{
    int32 vCount = mesh->vCount;
    if (vCount <= 0)
        return;

    if ((gVerticesBase - gVertices) + vCount > MAX_VERTICES)
    {
        ASSERT(false);
        return;
    }

    int32 fCount = mesh->rCount + mesh->tCount;
    if ((gFacesBase - gFaces) + fCount > MAX_FACES)
    {
        ASSERT(false);
        return;
    }

    const uint8* ptr = (uint8*)mesh + sizeof(Mesh);

    MeshQuad* quads = (MeshQuad*)ptr;
    ptr += mesh->rCount * sizeof(MeshQuad);

    MeshTriangle* triangles = (MeshTriangle*)ptr;
    ptr += mesh->tCount * sizeof(MeshTriangle);

    const MeshVertex* vertices = (MeshVertex*)ptr;

    {
        PROFILE(CNT_TRANSFORM);
        transformMesh(vertices, vCount, mesh->intensity);
    }

    {
        PROFILE(CNT_ADD);
        faceAddMesh(quads, triangles, mesh->rCount, mesh->tCount);
    }

    gVerticesBase += vCount;
}

void renderShadow(int32 x, int32 z, int32 sx, int32 sz)
{
    if (gVerticesBase - gVertices + 8 > MAX_VERTICES) {
        ASSERT(false);
        return;
    }

    if (gFacesBase - gFaces + 3 > MAX_FACES) {
        ASSERT(false);
        return;
    }

    x >>= MESH_SHIFT;
    z >>= MESH_SHIFT;
    sx >>= MESH_SHIFT;
    sz >>= MESH_SHIFT;

    int16 xns1 = x - sx;
    int16 xps1 = x + sx;
    int16 xns2 = xns1 - sx;
    int16 xps2 = xps1 + sx;

    int16 zns1 = z - sz;
    int16 zps1 = z + sz;
    int16 zns2 = zns1 - sz;
    int16 zps2 = zps1 + sz;

    MeshVertex v[8];
    v[0].x = xns1; v[0].y = 0; v[0].z = zps2;
    v[1].x = xps1; v[1].y = 0; v[1].z = zps2;
    v[2].x = xps2; v[2].y = 0; v[2].z = zps1;
    v[3].x = xps2; v[3].y = 0; v[3].z = zns1;
    v[4].x = xps1; v[4].y = 0; v[4].z = zns2;
    v[5].x = xns1; v[5].y = 0; v[5].z = zns2;
    v[6].x = xns2; v[6].y = 0; v[6].z = zns1;
    v[7].x = xns2; v[7].y = 0; v[7].z = zps1;

    transformMesh(v, 8, 0);
    faceAddMeshQuads(gShadowQuads, 3);

    gVerticesBase += 8;
}

X_NOINLINE void renderFill(int32 x, int32 y, int32 width, int32 height, int32 shade, int32 z)
{
    if (gVerticesBase - gVertices + 2 > MAX_VERTICES) {
        ASSERT(false);
        return;
    }

    if (gFacesBase - gFaces + 1 > MAX_FACES) {
        ASSERT(false);
        return;
    }

    gVerticesBase[0].x = x;
    gVerticesBase[0].y = y;
    gVerticesBase[0].g = shade;

    gVerticesBase[1].x = width;
    gVerticesBase[1].y = height;

    Face* f = faceAdd(z);
    f->flags = (FACE_TYPE_FILL_S << FACE_TYPE_SHIFT);
    f->indices[0] = gVerticesBase - gVertices;

    gVerticesBase += 2;
}

X_NOINLINE void renderLine(int32 x, int32 y, int32 width, int32 height, int32 index, int32 z)
{
    if (gVerticesBase - gVertices + 2 > MAX_VERTICES) {
        ASSERT(false);
        return;
    }

    if (gFacesBase - gFaces + 1 > MAX_FACES) {
        ASSERT(false);
        return;
    }

    ASSERT(width == 1 || height == 1);
    ASSERT(width > 0);
    ASSERT(height > 0);

    gVerticesBase[0].x = x;
    gVerticesBase[0].y = y;
    gVerticesBase[0].g = index;

    gVerticesBase[1].x = width;
    gVerticesBase[1].y = height;

    int32 idx = gVerticesBase - gVertices;

    Face* f = faceAdd(z);
    f->flags = (height == 1) ? (FACE_TYPE_LINE_H << FACE_TYPE_SHIFT) : (FACE_TYPE_LINE_V << FACE_TYPE_SHIFT);
    f->indices[0] = idx;

    gVerticesBase += 2;
}

void renderSprite(int32 vx, int32 vy, int32 vz, int32 vg, int32 index)
{
    if (gVerticesBase - gVertices + 2 > MAX_VERTICES) {
        ASSERT(false);
        return;
    }

    if (gFacesBase - gFaces + 1 > MAX_FACES) {
        ASSERT(false);
        return;
    }

    const Matrix &m = matrixGet();

    vx -= gCameraViewPos.x;
    vy -= gCameraViewPos.y;
    vz -= gCameraViewPos.z;

    int32 z = DP33(m.e20, m.e21, m.e22, vx, vy, vz);

    if (z < VIEW_MIN_F || z >= VIEW_MAX_F)
    {
        return;
    }

    int32 x = DP33(m.e00, m.e01, m.e02, vx, vy, vz);
    int32 y = DP33(m.e10, m.e11, m.e12, vx, vy, vz);

    x >>= FIXED_SHIFT;
    y >>= FIXED_SHIFT;
    z >>= FIXED_SHIFT;

    const Sprite* sprite = level.sprites + index;

    int32 l = x + sprite->l;
    int32 r = x + sprite->r;
    int32 t = y + sprite->t;
    int32 b = y + sprite->b;

    // TODO one projection
    PERSPECTIVE(l, t, z);

    l += (FRAME_WIDTH >> 1);
    if (l >= viewport.x1) return;

    t += (FRAME_HEIGHT >> 1);
    if (t >= viewport.y1) return;

    PERSPECTIVE(r, b, z);

    r += (FRAME_WIDTH >> 1);
    if (r < viewport.x0) return;

    b += (FRAME_HEIGHT >> 1);
    if (b < viewport.y0) return;

    if (l == r) return;
    if (t == b) return;

    if (z > FOG_MIN)
    {
        vg += (z - FOG_MIN) << FOG_SHIFT;
        if (vg > 8191) {
            vg = 8191;
        }
    }
    vg >>= 8;

    Vertex* v1 = gVerticesBase++;
    v1->x = l;
    v1->y = t;
    //v1->z = z;
    v1->g = vg;

    Vertex* v2 = gVerticesBase++;
    v2->x = r;
    v2->y = b;
    //v2->z = z;
    //v2->g = vg;

    int32 depth = X_MAX(0, z - 128); // depth hack

    Face* f = faceAdd(depth >> OT_SHIFT);
    f->flags = (FACE_TYPE_SPRITE << FACE_TYPE_SHIFT) | index;
    f->indices[0] = v1 - gVertices;

    gVerticesBase += 2;
}

void renderGlyph(int32 vx, int32 vy, int32 index)
{
    if (gVerticesBase - gVertices + 2 > MAX_VERTICES) {
        ASSERT(false);
        return;
    }

    if (gFacesBase - gFaces + 1 > MAX_FACES) {
        ASSERT(false);
        return;
    }

    const Sprite* sprite = level.sprites + index;

    int32 l = vx + sprite->l;
    int32 r = vx + sprite->r;
    int32 t = vy + sprite->t;
    int32 b = vy + sprite->b;

    if (l == r) return;
    if (t == b) return;
    if (r < 0) return;
    if (b < 0) return;
    if (l >= FRAME_WIDTH) return;
    if (t >= FRAME_HEIGHT) return;

    Vertex* v1 = gVerticesBase++;
    v1->x = l;
    v1->y = t;
    v1->g = 16;

    Vertex* v2 = gVerticesBase++;
    v2->x = r;
    v2->y = b;

    Face* f = faceAdd(0);
    f->flags = (FACE_TYPE_SPRITE << FACE_TYPE_SHIFT) | index;
    f->indices[0] = v1 - gVertices;
}

#define BAR_HEIGHT  5

const int32 BAR_COLORS[BAR_MAX][5] = {
    {  8, 11,  8,  6, 24 },
    { 32, 41, 32, 19, 21 },
    { 28, 29, 28, 26, 27 },
    { 43, 44, 43, 42, 41 },
};

X_NOINLINE void renderBorder(int32 x, int32 y, int32 width, int32 height, int32 color1, int32 color2, int32 z)
{
    renderLine(x + 1, y, width - 2, 1, color1, z);
    renderLine(x + 1, y + height - 1, width - 2, 1, color2, z);
    renderLine(x, y, 1, height, color1, z);
    renderLine(x + width - 1, y, 1, height, color2, z);
}

void renderBar(int32 x, int32 y, int32 width, int32 value, BarType type)
{
    // colored bar
    int32 ix = x + 1;
    int32 iy = y + 1;
    int32 w = value* width >> 8;

    if (w > 0)
    {
        for (int32 i = 0; i < 5; i++)
        {
            renderLine(ix, iy++, w, 1, BAR_COLORS[type][i], 0);
        }
    }

    if (w < width)
    {
        renderFill(x + 1 + w, y + 1, width - w, BAR_HEIGHT, 27, 0);
    }

    renderBorder(x, y, width + 2, BAR_HEIGHT + 2, 19, 17, 0);
}

void renderBackground(const void* background)
{
    dmaCopy(background, (void*)fb, FRAME_WIDTH * FRAME_HEIGHT);
}

void* copyBackground()
{
    dmaCopy((void*)fb, gBackgroundCopy, FRAME_WIDTH * FRAME_HEIGHT);

    palGrayRemap(gBackgroundCopy, FRAME_WIDTH * FRAME_HEIGHT);

    return gBackgroundCopy;
}
