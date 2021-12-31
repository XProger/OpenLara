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
    VertexLink* prev;
    VertexLink* next;
};

struct ViewportRel {
    int32 minXY;
    int32 maxXY;
};

ViewportRel viewportRel;

#if defined(_WIN32)
    uint16 fb[VRAM_WIDTH * FRAME_HEIGHT];
#elif defined(__GBA__)
    uint32 fb = MEM_VRAM;
#elif defined(__TNS__)
    uint16 fb[VRAM_WIDTH * FRAME_HEIGHT];
#elif defined(__DOS__)
    uint16 fb[VRAM_WIDTH * FRAME_HEIGHT];
#endif

#define GUARD_BAND 512

#define PAL_COLOR_TRANSP    0x0000
#define PAL_COLOR_BLACK     0x0421

#define FACE_SPRITE     (1 << 11)
#define FACE_FLAT       (1 << 12)
#define FACE_CLIPPED    (1 << 13)
#define FACE_COLORED    (1 << 14)
#define FACE_TRIANGLE   (1 << 15)
#define FACE_SHADOW     (FACE_COLORED | FACE_FLAT | FACE_SPRITE)
#define FACE_TEXTURE    0x07FF

#if defined(__GBA__)
    #define ALIGNED_LIGHTMAP
#endif

#if defined(MODE4)
    #include "rasterizer_mode4.h"
#elif defined(MODE13)
    #include "rasterizer_mode13.h"
#else
    #error no supported video mode set
#endif

extern uint8 lightmap[256 * 32];
extern Level level;

const uint8* tile;

Vertex* gVerticesBase;
Face* gFacesBase;

EWRAM_DATA Vertex gVertices[MAX_VERTICES]; // EWRAM 16k
EWRAM_DATA Face gFaces[MAX_FACES];         // EWRAM 5k
EWRAM_DATA Face* otFaces[OT_SIZE];

bool enableAlphaTest;

enum ClipFlags {
    CLIP_LEFT   = 1 << 0,
    CLIP_RIGHT  = 1 << 1,
    CLIP_TOP    = 1 << 2,
    CLIP_BOTTOM = 1 << 3,
    CLIP_FAR    = 1 << 4,
    CLIP_NEAR   = 1 << 5,
    CLIP_MASK_VP  = (CLIP_LEFT | CLIP_RIGHT | CLIP_TOP | CLIP_BOTTOM),
};

extern "C" const MeshQuad gShadowQuads[] = {
    { FACE_SHADOW, {0, 1, 2, 7} },
    { FACE_SHADOW, {7, 2, 3, 6} },
    { FACE_SHADOW, {6, 3, 4, 5} }
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

X_INLINE bool checkBackface(const Vertex *a, const Vertex *b, const Vertex *c)
{
    return (b->x - a->x) * (c->y - a->y) > (c->x - a->x) * (b->y - a->y);
}

X_INLINE int32 classify(const Vertex &v, const RectMinMax &clip)
{
    return (v.x < clip.x0 ? CLIP_LEFT : 0) |
           (v.x > clip.x1 ? CLIP_RIGHT : 0) |
           (v.y < clip.y0 ? CLIP_TOP : 0) |
           (v.y > clip.y1 ? CLIP_BOTTOM : 0);
}

X_INLINE Face* faceAdd(int32 depth)
{
    ASSERT(depth >= 0 && depth < OT_SIZE);

    Face* face = gFacesBase++;
    face->next = otFaces[depth];
    otFaces[depth] = face;

    return face;
}

#ifdef USE_ASM
    #define transformRoom           transformRoom_asm
    #define transformRoomUW         transformRoomUW_asm
    #define transformMesh           transformMesh_asm
    #define faceAddRoomQuads        faceAddRoomQuads_asm
    #define faceAddRoomTriangles    faceAddRoomTriangles_asm
    #define faceAddMeshQuads        faceAddMeshQuads_asm
    #define faceAddMeshTriangles    faceAddMeshTriangles_asm

    extern "C" {
        void transformRoom_asm(const RoomVertex* vertices, int32 count);
        void transformRoomUW_asm(const RoomVertex* vertices, int32 count);
        void transformMesh_asm(const MeshVertex* vertices, int32 count, int32 intensity);
        void faceAddRoomQuads_asm(const RoomQuad* polys, int32 count);
        void faceAddRoomTriangles_asm(const RoomTriangle* polys, int32 count);
        void faceAddMeshQuads_asm(const MeshQuad* polys, int32 count);
        void faceAddMeshTriangles_asm(const MeshTriangle* polys, int32 count);
    }
#else
    #define transformRoom           transformRoom_c
    #define transformRoomUW         transformRoomUW_c
    #define transformMesh           transformMesh_c
    #define faceAddRoomQuads        faceAddRoomQuads_c
    #define faceAddRoomTriangles    faceAddRoomTriangles_c
    #define faceAddMeshQuads        faceAddMeshQuads_c
    #define faceAddMeshTriangles    faceAddMeshTriangles_c

void transformRoom_c(const RoomVertex* vertices, int32 count)
{
    Vertex* res = gVerticesBase;

    for (int32 i = 0; i < count; i++, res++)
    {
        uint32 value = *(uint32*)(vertices++);

        int32 vx = (value & (0xFF)) << 10;
        int32 vy = (value & (0xFF << 8));
        int32 vz = (value & (0xFF << 16)) >> 6;
        int32 vg = (value & (0xFF << 24)) >> (24 - 5);

        const Matrix &m = matrixGet();
        int32 x = DP43(m.e00, m.e01, m.e02, m.e03, vx, vy, vz);
        int32 y = DP43(m.e10, m.e11, m.e12, m.e13, vx, vy, vz);
        int32 z = DP43(m.e20, m.e21, m.e22, m.e23, vx, vy, vz);

        uint32 clip = 0;

        if (z <= VIEW_MIN_F) {
            clip = CLIP_NEAR;
            z = VIEW_MIN_F;
        }

        if (z >= VIEW_MAX_F) {
            clip = CLIP_FAR;
            z = VIEW_MAX_F;
        }

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

        int32 vx = (value & (0xFF)) << 10;
        int32 vy = (value & (0xFF << 8));
        int32 vz = (value & (0xFF << 16)) >> 6;
        int32 vg = (value & (0xFF << 24)) >> (24 - 5);

        const Matrix &m = matrixGet();
        int32 x = DP43(m.e00, m.e01, m.e02, m.e03, vx, vy, vz);
        int32 y = DP43(m.e10, m.e11, m.e12, m.e13, vx, vy, vz);
        int32 z = DP43(m.e20, m.e21, m.e22, m.e23, vx, vy, vz);

        uint32 clip = 0;

        if (z <= VIEW_MIN_F) {
            clip = CLIP_NEAR;
            z = VIEW_MIN_F;
        }

        if (z >= VIEW_MAX_F) {
            clip = CLIP_FAR;
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
        int32 vx = vertices->x;
        int32 vy = vertices->y;
        int32 vz = vertices->z;
        vertices++;

        const Matrix &m = matrixGet();
        int32 x = DP43(m.e00, m.e01, m.e02, m.e03, vx, vy, vz);
        int32 y = DP43(m.e10, m.e11, m.e12, m.e13, vx, vy, vz);
        int32 z = DP43(m.e20, m.e21, m.e22, m.e23, vx, vy, vz);
 
        uint32 clip = 0;

        if (z <= VIEW_MIN_F) {
            clip = CLIP_NEAR;
            z = VIEW_MIN_F;
        }

        if (z >= VIEW_MAX_F) {
            clip = CLIP_FAR;
            z = VIEW_MAX_F;
        }

        x >>= FIXED_SHIFT;
        y >>= FIXED_SHIFT;
        z >>= FIXED_SHIFT;

        PERSPECTIVE(x, y, z);

        x += (FRAME_WIDTH  >> 1);
        y += (FRAME_HEIGHT >> 1);

        if (x < viewport.x0) clip |= CLIP_LEFT;
        if (x > viewport.x1) clip |= CLIP_RIGHT;
        if (y < viewport.y0) clip |= CLIP_TOP;
        if (y > viewport.y1) clip |= CLIP_BOTTOM;

        res->x = x;
        res->y = y;
        res->z = z;
        res->g = vg;
        res->clip = clip;
    }
}

void faceAddRoomQuads_c(const RoomQuad* polys, int32 count)
{
    const Vertex* v = gVerticesBase;

    for (int32 i = 0; i < count; i++, polys++)
    {
        uint32 flags = polys->flags;
        const Vertex* v0 = v + polys->indices[0];
        const Vertex* v1 = v + polys->indices[1];
        const Vertex* v2 = v + polys->indices[2];
        const Vertex* v3 = v + polys->indices[3];

        uint32 c0 = v0->clip;
        uint32 c1 = v1->clip;
        uint32 c2 = v2->clip;
        uint32 c3 = v3->clip;

        if (c0 & c1 & c2 & c3)
            continue;

        if ((c0 | c1 | c2 | c3) & CLIP_MASK_VP) {
            flags |= FACE_CLIPPED;
        }

        uint32 g0 = v0->g;
        uint32 g1 = v1->g;
        uint32 g2 = v2->g;
        uint32 g3 = v3->g;

        if (g0 == g1 && g0 == g2 && g0 == g3) {
            flags |= FACE_FLAT;
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

        if (c0 & c1 & c2)
            continue;

        if ((c0 | c1 | c2) & CLIP_MASK_VP) {
            flags |= FACE_CLIPPED;
        }

        uint32 g0 = v0->g;
        uint32 g1 = v1->g;
        uint32 g2 = v2->g;

        if (g0 == g1 && g0 == g2) {
            flags |= FACE_FLAT;
        }

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
    const Vertex* v = gVerticesBase;

    for (int32 i = 0; i < count; i++, polys++)
    {
        uint32 flags = polys->flags;
        const Vertex* v0 = v + polys->indices[0];
        const Vertex* v1 = v + polys->indices[1];
        const Vertex* v2 = v + polys->indices[2];
        const Vertex* v3 = v + polys->indices[3];

        if (checkBackface(v0, v1, v2))
            continue;

        uint32 c0 = v0->clip;
        uint32 c1 = v1->clip;
        uint32 c2 = v2->clip;
        uint32 c3 = v3->clip;

        if (c0 & c1 & c2 & c3)
            continue;

        if ((c0 | c1 | c2 | c3) & CLIP_MASK_VP) {
            flags |= FACE_CLIPPED;
        }

        uint32 g0 = v0->g;
        uint32 g1 = v1->g;
        uint32 g2 = v2->g;
        uint32 g3 = v3->g;

        if (g0 == g1 && g0 == g2 && g0 == g3) {
            flags |= FACE_FLAT;
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
    const Vertex* v = gVerticesBase;

    for (int32 i = 0; i < count; i++, polys++)
    {
        uint32 flags = polys->flags;
        const Vertex* v0 = v + polys->indices[0];
        const Vertex* v1 = v + polys->indices[1];
        const Vertex* v2 = v + polys->indices[2];

        if (checkBackface(v0, v1, v2))
            continue;

        uint32 c0 = v0->clip;
        uint32 c1 = v1->clip;
        uint32 c2 = v2->clip;

        if (c0 & c1 & c2)
            continue;

        if ((c0 | c1 | c2) & CLIP_MASK_VP) {
            flags |= FACE_CLIPPED;
        }

        uint32 g0 = v0->g;
        uint32 g1 = v1->g;
        uint32 g2 = v2->g;

        if (g0 == g1 && g0 == g2) {
            flags |= FACE_FLAT;
        }

        int32 depth = (v0->z + v1->z + v2->z + v2->z) >> (2 + OT_SHIFT);

        Face* f = faceAdd(depth);
        f->flags = flags;
        f->indices[0] = v0 - gVertices;
        f->indices[1] = v1 - gVertices;
        f->indices[2] = v2 - gVertices;
    }
}

bool transformBoxRect(const AABBs* box, RectMinMax* rect)
{
    const Matrix &m = matrixGet();

    if ((m.e23 < VIEW_MIN_F) || (m.e23 >= VIEW_MAX_F))
        return false;

    const vec3i v[8] = {
        _vec3i( box->minX, box->minY, box->minZ ),
        _vec3i( box->maxX, box->minY, box->minZ ),
        _vec3i( box->minX, box->maxY, box->minZ ),
        _vec3i( box->maxX, box->maxY, box->minZ ),
        _vec3i( box->minX, box->minY, box->maxZ ),
        _vec3i( box->maxX, box->minY, box->maxZ ),
        _vec3i( box->minX, box->maxY, box->maxZ ),
        _vec3i( box->maxX, box->maxY, box->maxZ )
    };

    *rect = RectMinMax( INT_MAX, INT_MAX, INT_MIN, INT_MIN );

    for (int32 i = 0; i < 8; i++)
    {
        int32 z = DP43(m.e20, m.e21, m.e22, m.e23, v[i].x, v[i].y, v[i].z);

        if (z < VIEW_MIN_F || z >= VIEW_MAX_F)
            continue;

        int32 x = DP43(m.e00, m.e01, m.e02, m.e03, v[i].x, v[i].y, v[i].z);
        int32 y = DP43(m.e10, m.e11, m.e12, m.e13, v[i].x, v[i].y, v[i].z);

        x >>= FIXED_SHIFT;
        y >>= FIXED_SHIFT;
        z >>= FIXED_SHIFT;

        PERSPECTIVE(x, y, z);

        if (x < rect->x0) rect->x0 = x;
        if (x > rect->x1) rect->x1 = x;
        if (y < rect->y0) rect->y0 = y;
        if (y > rect->y1) rect->y1 = y;
    }

    rect->x0 += (FRAME_WIDTH  / 2);
    rect->y0 += (FRAME_HEIGHT / 2);
    rect->x1 += (FRAME_WIDTH  / 2);
    rect->y1 += (FRAME_HEIGHT / 2);

    return true;
}

int32 rectIsVisible(const RectMinMax* rect)
{
    if (rect->x0 > rect->x1 ||
        rect->x0 > viewport.x1 ||
        rect->x1 < viewport.x0 ||
        rect->y0 > viewport.y1 ||
        rect->y1 < viewport.y0) return 0; // not visible

    if (rect->x0 < viewport.x0 ||
        rect->x1 > viewport.x1 ||
        rect->y0 < viewport.y0 ||
        rect->y1 > viewport.y1) return -1; // clipped

    return 1; // fully visible
}

int32 boxIsVisible_c(const AABBs* box)
{
    RectMinMax rect;
    if (!transformBoxRect(box, &rect))
        return 0; // not visible
    return rectIsVisible(&rect);
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
#endif

VertexLink* clipPoly(VertexLink* poly, VertexLink* tmp, int32 &pCount)
{
    #define LERP(a,b,t)         (b + ((a - b) * t >> 12))
    #define LERP2(a,b,ta,tb)    (b + (((a - b) * ta / tb) >> 12) )

    #define CLIP_AXIS(X, Y, edge, output) {\
        int32 ta = (edge - b->v.X) << 12;\
        int32 tb = (a->v.X - b->v.X);\
        ASSERT(tb != 0);\
        int32 t = ta / tb;\
        VertexLink* v = output + count++;\
        v->v.X = edge;\
        v->v.Y = LERP2(a->v.Y, b->v.Y, ta, tb);\
        v->v.g = LERP(a->v.g, b->v.g, t);\
        v->t.uv.u = LERP(a->t.uv.u, b->t.uv.u, t);\
        v->t.uv.v = LERP(a->t.uv.v, b->t.uv.v, t);\
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
                output[count++] = *b;\
            }\
        }\
        if (count < 3) return NULL;\
    }

    int32 count = 0;

    VertexLink *in = poly;
    VertexLink *out = tmp;

    // clip x
    CLIP_XY(x, y, viewport.x0, viewport.x1, in, out);

    pCount = count;
    count = 0;

    // clip y
    CLIP_XY(y, x, viewport.y0, viewport.y1, out, in);
    pCount = count;

    return in;
}

void renderInit()
{
    gVerticesBase = gVertices;
    gFacesBase = gFaces;
}

void rasterize(uint32 flags, const VertexLink *top)
{
    uint16* pixel = (uint16*)fb + top->v.y * VRAM_WIDTH;
#ifdef DEBUG_OVERDRAW
    rasterize_overdraw(pixel, top, top);
#else
    if (flags & FACE_COLORED) {
        if (flags & FACE_FLAT) {
            if (flags & FACE_SPRITE) {
                rasterizeS(pixel, top, top);
            } else {
                rasterizeF(pixel, top, top, flags & FACE_TEXTURE);
            }
        } else {
            rasterizeG(pixel, top, top, flags & FACE_TEXTURE);
        }
    } else {
        if (flags & FACE_SPRITE) {
            rasterizeSprite(pixel, top, top + 1);
        } else if (enableAlphaTest) {
            if (flags & FACE_FLAT) {
                rasterizeFTA(pixel, top, top);
            } else {
                rasterizeGTA(pixel, top, top);
            }
        } else {
            if (flags & FACE_FLAT) {
                rasterizeFT(pixel, top, top);
            } else {
                rasterizeGT(pixel, top, top);
            }
        }
    }
#endif
}

void drawTriangle(const Face* face, VertexLink *v)
{
    VertexLink* v1 = v + 0;
    VertexLink* v2 = v + 1;
    VertexLink* v3 = v + 2;

    v1->next = v2;
    v2->next = v3;
    v3->next = v1;
    v1->prev = v3;
    v2->prev = v1;
    v3->prev = v2;

    const VertexLink* top = v1;
    if (v1->v.y < v2->v.y) {
        if (v1->v.y < v3->v.y) {
            top = v1;
        } else {
            top = v3;
        }
    } else {
        if (v2->v.y < v3->v.y) {
            top = v2;
        } else {
            top = v3;
        }
    }

    rasterize(face->flags, top);
}

void drawQuad(const Face* face, VertexLink *v)
{
    VertexLink* v1 = v + 0;
    VertexLink* v2 = v + 1;
    VertexLink* v3 = v + 2;
    VertexLink* v4 = v + 3;

    v1->next = v2;
    v2->next = v3;
    v3->next = v4;
    v4->next = v1;
    v1->prev = v4;
    v2->prev = v1;
    v3->prev = v2;
    v4->prev = v3;

    VertexLink* top;

    if (v1->v.y < v2->v.y) {
        if (v1->v.y < v3->v.y) {
            top = (v1->v.y < v4->v.y) ? v1 : v4;
        } else {
            top = (v3->v.y < v4->v.y) ? v3 : v4;
        }
    } else {
        if (v2->v.y < v3->v.y) {
            top = (v2->v.y < v4->v.y) ? v2 : v4;
        } else {
            top = (v3->v.y < v4->v.y) ? v3 : v4;
        }
    }

    rasterize(face->flags, top);
}

void drawPoly(Face* face, VertexLink* v)
{
    VertexLink tmp[16];

    int32 count = (face->flags & FACE_TRIANGLE) ? 3 : 4;

    v = clipPoly(v, tmp, count);

    if (!v) return;

    if (count <= 4)
    {
        face->indices[0] = 0;
        face->indices[1] = 1;
        face->indices[2] = 2;
        face->indices[3] = 3;

        if (count == 3) {

            if (v[0].v.y == v[1].v.y &&
                v[0].v.y == v[2].v.y)
                return;

            drawTriangle(face, v);
        } else {

            if (v[0].v.y == v[1].v.y &&
                v[0].v.y == v[2].v.y &&
                v[0].v.y == v[3].v.y)
                return;

            drawQuad(face, v);
        }
        return;
    }

    VertexLink* top = v;
    top->next = v + 1;
    top->prev = v + count - 1;

    bool skip = true;

    for (int32 i = 1; i < count; i++)
    {
        VertexLink *p = v + i;

        p->next = v + (i + 1) % count;
        p->prev = v + (i - 1 + count) % count;

        if (p->v.y != top->v.y)
        {
            if (p->v.y < top->v.y) {
                top = p;
            }
            skip = false;
        }
    }

    if (skip) {
        return; // zero height poly
    }

    rasterize(face->flags, top);
}

void drawSprite(Face* face, VertexLink* v)
{
    rasterize(face->flags, v);
}

void drawGlyph(const Sprite *sprite, int32 x, int32 y)
{
    int32 w = sprite->r - sprite->l;
    int32 h = sprite->b - sprite->t;

    w = (w >> 1) << 1; // make it even

    int32 ix = x + sprite->l;
    int32 iy = y + sprite->t;

    uint16* pixel = (uint16*)fb + iy * VRAM_WIDTH + (ix >> 1);

    const uint16* glyphData = (uint16*)(level.tiles + (sprite->tile << 16) + 256 * sprite->v + sprite->u);

    while (h--)
    {
        const uint16* p = glyphData;

        for (int32 i = 0; i < (w / 2); i++)
        {
            if (p[0])
            {
                if (p[0] & 0x00FF) {
                    if (p[0] & 0xFF00) {
                        pixel[i] = p[0];
                    } else {
                        pixel[i] = (pixel[i] & 0xFF00) | p[0];
                    }
                } else {
                    pixel[i] = (pixel[i] & 0x00FF) | p[0];
                }
            }

            p++;
        }

        pixel += VRAM_WIDTH;

        glyphData += 256 / 2;
    }
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

void flush()
{
    if (gFacesBase > gFaces)
    {
        PROFILE(CNT_FLUSH);

        for (int32 i = OT_SIZE - 1; i >= 0; i--)
        {
            if (!otFaces[i]) continue;

            Face *face = otFaces[i];
            otFaces[i] = NULL;

            do {
                uint32 flags = face->flags;

                VertexLink v[16];

                if (flags == FACE_SPRITE) {
                    const Sprite &sprite = level.sprites[face->indices[1]];

                    v[0].v = gVertices[face->indices[0] + 0];
                    v[0].t.uv.u = sprite.u;
                    v[0].t.uv.v = sprite.v;
                    v[1].v = gVertices[face->indices[0] + 1];
                    v[1].t.uv.u = sprite.w;
                    v[1].t.uv.v = sprite.h;

                    ASSERT(v[0].v.x <= v[1].v.x);
                    ASSERT(v[0].v.y <= v[1].v.y);

                    tile = level.tiles + (level.sprites[face->indices[1]].tile << 16);

                    drawSprite(face, v);
                } else {
                    if (!(flags & FACE_COLORED))
                    {
                        const Texture &tex = level.textures[flags & FACE_TEXTURE];
                        tile = level.tiles + (tex.tile << 16);

                        v[0].t.t = tex.uv0;
                        v[1].t.t = tex.uv1;
                        v[2].t.t = tex.uv2;
                        v[3].t.t = tex.uv3;

                        enableAlphaTest = (tex.attribute & TEX_ATTR_AKILL);
                    }

                    v[0].v = gVertices[face->indices[0]];
                    v[1].v = gVertices[face->indices[1]];
                    v[2].v = gVertices[face->indices[2]];
                    if (!(flags & FACE_TRIANGLE)) {
                        v[3].v = gVertices[face->indices[3]];
                    }

                    if (flags & FACE_CLIPPED) {
                        drawPoly(face, v);
                    } else {
                        if (flags & FACE_TRIANGLE) {
                            drawTriangle(face, v);
                        } else {
                            drawQuad(face, v);
                        }
                    }
                }
                face = face->next;
            } while (face);
        }
    }

#ifdef PROFILING
    #if !defined(PROFILE_FRAMETIME) && !defined(PROFILE_SOUNDTIME)
        gCounters[CNT_VERT] += gVerticesBase - gVertices;
        gCounters[CNT_POLY] += gFacesBase - gFaces;
    #endif
#endif

    gVerticesBase = gVertices;
    gFacesBase = gFaces;
}

void clear()
{
    dmaFill((void*)fb, 0, VRAM_WIDTH * FRAME_HEIGHT * 2);
}

void renderRoom(const Room* room)
{
    int32 vCount = room->info->verticesCount;
    if (vCount <= 0)
        return;

    if ((gVerticesBase - gVertices) + vCount > MAX_VERTICES)
    {
        ASSERT(false);
        return;
    }

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

    int32 fCount = mesh->rCount + mesh->crCount + mesh->tCount + mesh->ctCount;
    if ((gFacesBase - gFaces) + fCount > MAX_FACES)
    {
        ASSERT(false);
        return;
    }

    const uint8* ptr = (uint8*)mesh + sizeof(Mesh);

    const MeshVertex* vertices = (MeshVertex*)ptr;
    ptr += vCount * sizeof(vertices[0]);

    MeshQuad* quads = (MeshQuad*)ptr;
    ptr += mesh->rCount * sizeof(MeshQuad);

    MeshTriangle* triangles = (MeshTriangle*)ptr;

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

    vx -= cameraViewPos.x;
    vy -= cameraViewPos.y;
    vz -= cameraViewPos.z;

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
    f->flags = uint16(FACE_SPRITE);
    f->indices[0] = v1 - gVertices;
    f->indices[1] = index;

    gVerticesBase += 2;
}

void renderGlyph(int32 vx, int32 vy, int32 index)
{
    //
}
