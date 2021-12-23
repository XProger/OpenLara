#include "common.h"

struct Vertex
{
    int16 x, y, z;
    uint8 g, clip;
};

struct VertexLink
{
    Vertex v;
    TexCoord t;
    VertexLink* prev;
    VertexLink* next;
};

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
extern int32 lightAmbient;
extern int32 randTable[MAX_RAND_TABLE];
extern int32 caustics[MAX_CAUSTICS];
extern int32 causticsFrame;

const uint8* tile;

int32 gVerticesCount;
int32 gFacesCount;

Vertex* gVerticesBase;

EWRAM_DATA Vertex gVertices[MAX_VERTICES]; // EWRAM 16k
EWRAM_DATA Face gFaces[MAX_FACES];         // EWRAM 5k
EWRAM_DATA Face* otFaces[OT_SIZE];

int32 otMin = OT_SIZE - 1;
int32 otMax = 0;

bool enableAlphaTest;
bool enableClipping;
bool enableMaxSort;

void setViewport(const RectMinMax &vp)
{
    viewport = vp;
}

void setPaletteIndex(int32 index)
{
    // TODO
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

X_INLINE int32 classify(const Vertex &v, const RectMinMax &clip)
{
    return (v.x < clip.x0 ? 1 : 0) |
           (v.x > clip.x1 ? 2 : 0) |
           (v.y < clip.y0 ? 4 : 0) |
           (v.y > clip.y1 ? 8 : 0);
}

void transformRoomVertex(Vertex* res, const RoomVertex* v, int32 caustics)
{
    int32 vx = v->x << 10;
    int32 vz = v->z << 10;

#if defined(MODE4) // TODO for all modes
    if (vz < frustumAABB.minZ || vz > frustumAABB.maxZ ||
        vx < frustumAABB.minX || vx > frustumAABB.maxX)
    {
        res->clip = 32;
        return;
    }
#endif

    int32 vy = v->y << 8;

    const Matrix &m = matrixGet();

    int32 z = DP43(m.e20, m.e21, m.e22, m.e23, vx, vy, vz);

    if (z <= 0)
    {
        res->clip = 32;
        return;
    }

    if (z < VIEW_MIN_F || z >= VIEW_MAX_F)
    {
        if (z < VIEW_MIN_F) z = VIEW_MIN_F;
        if (z >= VIEW_MAX_F) z = VIEW_MAX_F;
        res->clip = 16;
    } else {
        res->clip = 0;
    }

    int32 fogZ = z >> FIXED_SHIFT;
    res->z = fogZ;

    int32 vg = v->g << 5;

    if (caustics) {
        vg = X_CLAMP(vg + caustics, 0, 8191);
    }

    if (fogZ > FOG_MIN)
    {
        vg += (fogZ - FOG_MIN) << FOG_SHIFT;
        if (vg > 8191) {
            vg = 8191;
        }
    }

    res->g = vg >> 8;

    int32 x = DP43(m.e00, m.e01, m.e02, m.e03, vx, vy, vz);
    int32 y = DP43(m.e10, m.e11, m.e12, m.e13, vx, vy, vz);

    x >>= FIXED_SHIFT;
    y >>= FIXED_SHIFT;
    z >>= FIXED_SHIFT;

    PERSPECTIVE(x, y, z);

    x = X_CLAMP(x, -GUARD_BAND, GUARD_BAND);
    y = X_CLAMP(y, -GUARD_BAND, GUARD_BAND);

    res->x = x + (FRAME_WIDTH  >> 1);
    res->y = y + (FRAME_HEIGHT >> 1);

    res->clip |= classify(*res, viewport);
}

void transformMeshVertex(Vertex* res, int32 vx, int32 vy, int32 vz, int32 vg)
{
    const Matrix &m = matrixGet();

    int32 z = DP43(m.e20, m.e21, m.e22, m.e23, vx, vy, vz);

    if (z < VIEW_MIN_F || z >= VIEW_MAX_F)
    {
        res->clip = 32;
        return;
    }

    int32 x = DP43(m.e00, m.e01, m.e02, m.e03, vx, vy, vz);
    int32 y = DP43(m.e10, m.e11, m.e12, m.e13, vx, vy, vz);

    x >>= FIXED_SHIFT;
    y >>= FIXED_SHIFT;
    z >>= FIXED_SHIFT;

    res->z = z;
    res->g = vg >> 8;

    PERSPECTIVE(x, y, z);

    x = X_CLAMP(x, -GUARD_BAND, GUARD_BAND);
    y = X_CLAMP(y, -GUARD_BAND, GUARD_BAND);

    res->x = x + (FRAME_WIDTH  >> 1);
    res->y = y + (FRAME_HEIGHT >> 1);

    res->clip = classify(*res, viewport); // enableClipping ? classify(res, viewport) : 0; TODO fix clip boxes for static meshes
}

void transformRoom(const Room* room)
{
    int32 vCount = room->info->verticesCount;
    if (vCount <= 0)
        return;

    gVerticesBase = gVertices + gVerticesCount;
    gVerticesCount += vCount;
    ASSERT(gVerticesCount <= MAX_VERTICES);

    const RoomVertex* vertices = room->data.vertices;

    bool underwater = ROOM_FLAG_WATER(room->info->flags);

    int32 causticsValue = 0;

    Vertex* res = gVerticesBase;

    for (int32 i = 0; i < vCount; i++, res++)
    {
        if (underwater) {
            causticsValue = caustics[(randTable[i & (MAX_RAND_TABLE - 1)] + causticsFrame) & (MAX_CAUSTICS - 1)];
        }

        transformRoomVertex(res, vertices, causticsValue);
        vertices++;
    }
}

void transformMesh(const MeshVertex* vertices, int32 vCount, const uint16* vIntensity, const vec3s* vNormal)
{
    gVerticesBase = gVertices + gVerticesCount;

    gVerticesCount += vCount;
    ASSERT(gVerticesCount <= MAX_VERTICES);

    Vertex* res = gVerticesBase;

    // TODO calc lighting for vNormal
    for (int32 i = 0; i < vCount; i++, res++)
    {
        ASSERT(!vIntensity || (vIntensity[i] + lightAmbient >= 0)); // ohhh, use X_CLAMP...

        transformMeshVertex(res, vertices->x, vertices->y, vertices->z, vIntensity ? X_MIN(vIntensity[i] + lightAmbient, 8191) : lightAmbient);
        vertices++;
    }
}

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
}

void rasterize(const Face* face, const VertexLink *top)
{
    uint16* pixel = (uint16*)fb + top->v.y * VRAM_WIDTH;
#ifdef DEBUG_OVERDRAW
    rasterize_overdraw(pixel, top, top);
#else
    #if 0
        rasterizeG(pixel, top, top, (face->flags & FACE_TEXTURE) + 10);
        //rasterize_dummy(pixel, top, top);
        //rasterizeGT(pixel, top, top);
    #else
        if (face->flags & FACE_COLORED) {
            if (face->flags & FACE_FLAT) {
                if (face->flags & FACE_SPRITE) {
                    rasterizeS(pixel, top, top);
                } else {
                    rasterizeF(pixel, top, top, face->flags & FACE_TEXTURE);
                }
            } else {
                rasterizeG(pixel, top, top, face->flags & FACE_TEXTURE);
            }
        } else {
            if (face->flags & FACE_SPRITE) {
                rasterizeSprite(pixel, top, top + 1);
            } else if (enableAlphaTest) {
                if (face->flags & FACE_FLAT) {
                    rasterizeFTA(pixel, top, top);
                } else {
                    rasterizeGTA(pixel, top, top);
                }
            } else {
                #if 0
                    rasterizeFT(pixel, top, top);
                #else
                if (face->flags & FACE_FLAT) {
                    rasterizeFT(pixel, top, top);
                } else {
                    rasterizeGT(pixel, top, top);
                }
                #endif
            }
        }
    #endif
#endif
}

void drawTriangle(const Face* face, VertexLink *v)
{
    VertexLink *v1 = v + 0,
             *v2 = v + 1,
             *v3 = v + 2;

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

    rasterize(face, top);
}

void drawQuad(const Face* face, VertexLink *v)
{
    VertexLink *v1 = v + 0,
             *v2 = v + 1,
             *v3 = v + 2,
             *v4 = v + 3;

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

    rasterize(face, top);
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

    rasterize(face, top);
}

void drawSprite(Face* face, VertexLink* v)
{
    rasterize(face, v);
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

X_INLINE Face* faceAdd(int32 depth)
{
    ASSERT(depth >= 0 && depth < OT_SIZE);

    Face* face = gFaces + gFacesCount++;
    face->next = otFaces[depth];
    otFaces[depth] = face;

    if (depth < otMin) otMin = depth;
    if (depth > otMax) otMax = depth;

    return face;
}

void faceAddQuad(uint32 flags, const Index* indices)
{
    ASSERT(gFacesCount < MAX_FACES);

    const Vertex* v = gVerticesBase;
    const Vertex* v1 = v + indices[0];
    const Vertex* v2 = v + indices[1];
    const Vertex* v3 = v + indices[2];
    const Vertex* v4 = v + indices[3];

    if (v1->clip & v2->clip & v3->clip & v4->clip)
        return;

    int32 clip = (v1->clip | v2->clip | v3->clip | v4->clip);
    if (clip & 32)
        return;

    if (clip) {
        flags |= FACE_CLIPPED;
    }

    if (v1->g == v2->g && v1->g == v3->g && v1->g == v4->g) {
        flags |= FACE_FLAT;
    }

    int32 depth;
    if (enableMaxSort) {
        depth = X_MAX(v1->z, X_MAX(v2->z, X_MAX(v3->z, v4->z))) >> OT_SHIFT;
    } else {
        depth = (v1->z + v2->z + v3->z + v4->z) >> (2 + OT_SHIFT);
    }

    Face* f = faceAdd(depth);
    f->flags = uint16(flags);
    f->indices[0] = v1 - gVertices;
    f->indices[1] = v2 - gVertices;
    f->indices[2] = v3 - gVertices;
    f->indices[3] = v4 - gVertices;
}

void faceAddTriangle(uint32 flags, const Index* indices)
{
    ASSERT(gFacesCount < MAX_FACES);

    const Vertex* v = gVerticesBase;
    const Vertex* v1 = v + indices[0];
    const Vertex* v2 = v + indices[1];
    const Vertex* v3 = v + indices[2];

    if (v1->clip & v2->clip & v3->clip)
        return;

    int32 clip = (v1->clip | v2->clip | v3->clip);
    if (clip & 32)
        return;

    if (clip) {
        flags |= FACE_CLIPPED;
    }

    if (v1->g == v2->g && v1->g == v3->g) {
        flags |= FACE_FLAT;
    }

    int32 depth;
    if (enableMaxSort) {
        depth = X_MAX(v1->z, X_MAX(v2->z, v3->z)) >> OT_SHIFT;
    } else {
        depth = (v1->z + v2->z + v3->z + v3->z) >> (2 + OT_SHIFT);
    }

    Face* f = faceAdd(depth);
    f->flags = uint16(flags | FACE_TRIANGLE);
    f->indices[0] = v1 - gVertices;
    f->indices[1] = v2 - gVertices;
    f->indices[2] = v3 - gVertices;
}

void faceAddSprite(int32 vx, int32 vy, int32 vz, int32 vg, int32 index)
{
    const Matrix &m = matrixGet();

    vx -= cameraViewPos.x;
    vy -= cameraViewPos.y;
    vz -= cameraViewPos.z;

    int32 z = DP33(m.e20, m.e21, m.e22, vx, vy, vz);

    if (z < VIEW_MIN_F || z >= VIEW_MAX_F)
    {
        return;
    }

    ASSERT(gFacesCount < MAX_FACES);

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

    ASSERT(gVerticesCount + 1 < MAX_VERTICES);

    Vertex &v1 = gVertices[gVerticesCount++];
    v1.x = l;
    v1.y = t;
    //v1.z = z;
    v1.g = vg;

    Vertex &v2 = gVertices[gVerticesCount++];
    v2.x = r;
    v2.y = b;
    //v2.z = z;
    //v2.g = vg;

    ASSERT(v2.x >= v1.x);
    ASSERT(v2.y >= v1.y);

    int32 depth = X_MAX(0, z - 128); // depth hack

    Face* f = faceAdd(depth >> OT_SHIFT);
    f->flags = uint16(FACE_SPRITE);
    f->indices[0] = gVerticesCount - 2;
    f->indices[1] = index;
}

void faceAddGlyph(int32 vx, int32 vy, int32 index)
{
    //
}

void faceAddRoom(const Room* room)
{
    enableMaxSort = true;

    const RoomQuad* quads = room->data.quads;
    const RoomTriangle* triangles = room->data.triangles;

    for (int32 i = 0; i < room->info->quadsCount; i++) {
        faceAddQuad(quads[i].flags, quads[i].indices);
    }

    for (int32 i = 0; i < room->info->trianglesCount; i++) {
        faceAddTriangle(triangles[i].flags, triangles[i].indices);
    }

    enableMaxSort = false;
}

void faceAddMesh(const MeshQuad* rFaces, const MeshQuad* crFaces, const MeshTriangle* tFaces, const MeshTriangle* ctFaces, int32 rCount, int32 crCount, int32 tCount, int32 ctCount)
{
    for (int32 i = 0; i < rCount; i++) {
        faceAddQuad(rFaces[i].flags, rFaces[i].indices);
    }

    for (int32 i = 0; i < crCount; i++) {
        faceAddQuad(crFaces[i].flags | FACE_COLORED, crFaces[i].indices);
    }

    for (int32 i = 0; i < tCount; i++) {
        faceAddTriangle(tFaces[i].flags, tFaces[i].indices);
    }

    for (int32 i = 0; i < ctCount; i++) {
        faceAddTriangle(ctFaces[i].flags | FACE_COLORED, ctFaces[i].indices);
    }
}

void flush()
{
    if (gFacesCount)
    {
        PROFILE(CNT_FLUSH);

        for (int32 i = otMax; i >= otMin; i--)
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

                        enableAlphaTest = (tex.attribute == 1);
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

        otMin = OT_SIZE - 1;
        otMax = 0;
    }

#ifdef PROFILING
    #if !defined(PROFILE_FRAMETIME) && !defined(PROFILE_SOUNDTIME)
        gCounters[CNT_VERT] += gVerticesCount;
        gCounters[CNT_POLY] += gFacesCount;
    #endif
#endif

    gVerticesBase = gVertices;
    gVerticesCount = 0;
    gFacesCount = 0;
}

void clear()
{
    dmaFill((void*)fb, 0, VRAM_WIDTH * FRAME_HEIGHT * 2);
}

#ifdef IWRAM_MATRIX_LERP
#define LERP_1_2(a, b)   a = (b + a) >> 1
#define LERP_1_3(a, b)   a = a + (b - a) / 3
#define LERP_2_3(a, b)   a = b - (b - a) / 3
#define LERP_1_4(a, b)   a = a + ((b - a) >> 2)
#define LERP_3_4(a, b)   a = b - ((b - a) >> 2)
#define LERP_1_5(a, b)   a = a + (b - a) / 5
#define LERP_2_5(a, b)   a = a + ((b - a) << 1) / 5
#define LERP_3_5(a, b)   a = b - ((b - a) << 1) / 5
#define LERP_4_5(a, b)   a = b - (b - a) / 5
#define LERP_SLOW(a, b)  a = a + ((b - a) * t >> 8)

#define LERP_ROW(lerp_func, a, b, row) \
    lerp_func(a.e##row##0, b.e##row##0); \
    lerp_func(a.e##row##1, b.e##row##1); \
    lerp_func(a.e##row##2, b.e##row##2); \
    lerp_func(a.e##row##3, b.e##row##3);

#define LERP_MATRIX(lerp_func) \
    LERP_ROW(lerp_func, m, n, 0); \
    LERP_ROW(lerp_func, m, n, 1); \
    LERP_ROW(lerp_func, m, n, 2);

void matrixLerp_c(const Matrix &n, int32 pmul, int32 pdiv)
{
    Matrix &m = matrixGet();

    if ((pdiv == 2) || ((pdiv == 4) && (pmul == 2))) {
        LERP_MATRIX(LERP_1_2);
    } else if (pdiv == 4) {

        if (pmul == 1) {
            LERP_MATRIX(LERP_1_4);
        } else {
            LERP_MATRIX(LERP_3_4);
        }

    } else {
        int32 t = pmul * FixedInvU(pdiv) >> 8;
        LERP_MATRIX(LERP_SLOW);
    }
}
#endif

// TODO move to sound.iwram.cpp?
int16 IMA_STEP[] = { // IWRAM !
    7,     8,     9,     10,    11,    12,    13,    14,
    16,    17,    19,    21,    23,    25,    28,    31,
    34,    37,    41,    45,    50,    55,    60,    66,
    73,    80,    88,    97,    107,   118,   130,   143,
    157,   173,   190,   209,   230,   253,   279,   307,
    337,   371,   408,   449,   494,   544,   598,   658,
    724,   796,   876,   963,   1060,  1166,  1282,  1411,
    1552,  1707,  1878,  2066,  2272,  2499,  2749,  3024,
    3327,  3660,  4026,  4428,  4871,  5358,  5894,  6484,
    7132,  7845,  8630,  9493,  10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794,
    32767
};

#define DECODE_IMA_4(n)\
    step = IMA_STEP[idx];\
    index = n & 7;\
    step += index * step << 1;\
    if (index < 4) {\
        idx = X_MAX(idx - 1, 0);\
    } else {\
        idx = X_MIN(idx + ((index - 3) << 1), X_COUNT(IMA_STEP) - 1);\
    }\
    if (n & 8) {\
        smp -= step >> 3;\
    } else {\
        smp += step >> 3;\
    }\
    *buffer++ = smp >> (16 - (8 + SND_VOL_SHIFT));

void decodeIMA(IMA_STATE &state, const uint8* data, int32* buffer, int32 size)
{
    uint32 step, index;

    int32 idx = state.idx;
    int32 smp = state.smp;

    for (int32 i = 0; i < size; i++)
    {
        uint32 n = *data++;
        DECODE_IMA_4(n);
        n >>= 4;
        DECODE_IMA_4(n);
    }

    state.idx = idx;
    state.smp = smp;
}

/* TODO OUT OF IWRAM!
#define DECODE_IMA_4_sample(n)\
    step = IMA_STEP[idx];\
    index = n & 7;\
    step += index * step << 1;\
    if (index < 4) {\
        idx = X_MAX(idx - 1, 0);\
    } else {\
        idx = X_MIN(idx + ((index - 3) << 1), X_COUNT(IMA_STEP) - 1);\
    }\
    if (n & 8) {\
        smp -= step >> 3;\
    } else {\
        smp += step >> 3;\
    }\
    *buffer++ += smp * volume >> (16 - (8 + SND_VOL_SHIFT));

void decodeIMA_sample(IMA_STATE &state, const uint8* data, int32* buffer, int32 size, int32 inc, int32 volume)
{
    uint32 step, index;

    int32 idx = state.idx;
    int32 smp = state.smp;

    for (int32 i = 0; i < size; i++)
    {
        uint32 n = *data;
        DECODE_IMA_4_sample(n);
        n >>= 4;
        DECODE_IMA_4_sample(n);

        data += inc;
    }

    state.idx = idx;
    state.smp = smp;
}
*/