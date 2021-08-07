#include "common.h"

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

#ifndef MODE_PAL
uint16 palette[256];      // IWRAM 0.5k
#endif

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
extern Texture textures[MAX_TEXTURES];
extern const Sprite* sprites;
extern const uint8* tiles;
extern int32 lightAmbient;
extern int32 randTable[MAX_RAND_TABLE];
extern int32 caustics[MAX_CAUSTICS];
extern int32 causticsFrame;

const uint8* tile;
const Sprite* sprite;

int32 gVerticesCount = 0;
int32 gFacesCount = 0; // 1 is reserved as OT terminator

EWRAM_DATA Vertex gVertices[MAX_VERTICES]; // EWRAM 16k
EWRAM_DATA Face gFaces[MAX_FACES];         // EWRAM 5k
EWRAM_DATA Face* otFaces[OT_SIZE] = { 0 };
int32 otMin = OT_SIZE - 1;
int32 otMax = 0;

bool enableAlphaTest;
bool enableClipping;

bool transformBoxRect(const Bounds* box, Rect* rect)
{
    const Matrix &m = matrixGet();

    if ((m[2][3] < VIEW_MIN_F) || (m[2][3] >= VIEW_MAX_F)) {
        return false;
    }

    const vec3i v[8] = {
        vec3i( box->minX, box->minY, box->minZ ),
        vec3i( box->maxX, box->minY, box->minZ ),
        vec3i( box->minX, box->maxY, box->minZ ),
        vec3i( box->maxX, box->maxY, box->minZ ),
        vec3i( box->minX, box->minY, box->maxZ ),
        vec3i( box->maxX, box->minY, box->maxZ ),
        vec3i( box->minX, box->maxY, box->maxZ ),
        vec3i( box->maxX, box->maxY, box->maxZ )
    };

    *rect = Rect( INT_MAX, INT_MAX, INT_MIN, INT_MIN );

    for (int32 i = 0; i < 8; i++)
    {
        int32 z = DP43(m[2], v[i]);

        if (z < VIEW_MIN_F || z >= VIEW_MAX_F) {
            continue;
        }

        int32 x = DP43(m[0], v[i]);
        int32 y = DP43(m[1], v[i]);

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

int32 rectIsVisible(const Rect* rect)
{
    if (rect->x0 > rect->x1 ||
        rect->x0 > viewport.x1  ||
        rect->x1 < viewport.x0  ||
        rect->y0 > viewport.y1  ||
        rect->y1 < viewport.y0) return 0; // not visible

    if (rect->x0 < viewport.x0 ||
        rect->x1 > viewport.x1 ||
        rect->y0 < viewport.y0 ||
        rect->y1 > viewport.y1) return -1; // clipped

    return 1; // fully visible
}

int32 boxIsVisible(const Bounds* box)
{
    Rect rect;
    if (!transformBoxRect(box, &rect))
        return 0; // not visible
    return rectIsVisible(&rect);
}

X_INLINE int32 classify(const Vertex &v, const Rect &clip) {
    return (v.x < clip.x0 ? 1 : 0) |
           (v.x > clip.x1 ? 2 : 0) |
           (v.y < clip.y0 ? 4 : 0) |
           (v.y > clip.y1 ? 8 : 0);
}

void transform(int32 vx, int32 vy, int32 vz, int32 vg)
{
    ASSERT(gVerticesCount < MAX_VERTICES);

    Vertex &res = gVertices[gVerticesCount++];

    const Matrix &m = matrixGet();

    int32 z = DP43c(m[2], vx, vy, vz);

    if (z < VIEW_MIN_F || z >= VIEW_MAX_F)
    {
        res.clip = 32;
        return;
    }

    int32 x = DP43c(m[0], vx, vy, vz);
    int32 y = DP43c(m[1], vx, vy, vz);

    res.z = z >> FIXED_SHIFT;
    res.g = vg >> 8;

    PERSPECTIVE(x, y, z);

    x = X_CLAMP(x, -GUARD_BAND, GUARD_BAND);
    y = X_CLAMP(y, -GUARD_BAND, GUARD_BAND);

    res.x = x + (FRAME_WIDTH  >> 1);
    res.y = y + (FRAME_HEIGHT >> 1);

    res.clip = classify(res, viewport); // enableClipping ? classify(res, viewport) : 0; TODO fix clip boxes for static meshes
}

void transformRoomVertex(const RoomVertex* v, int32 caustics)
{
    int32 vx = v->x << 10;
    int32 vz = v->z << 10;

    ASSERT(gVerticesCount < MAX_VERTICES);

    Vertex &res = gVertices[gVerticesCount++];

#if defined(MODE4) // TODO for all modes
    if (vz < frustumAABB.minZ || vz > frustumAABB.maxZ ||
        vx < frustumAABB.minX || vx > frustumAABB.maxX)
    {
        res.clip = 32;
        return;
    }
#endif

    int32 vy = v->y << 8;

    const Matrix &m = matrixGet();

    int32 z = DP43c(m[2], vx, vy, vz);

    if (z < VIEW_MIN_F || z >= VIEW_MAX_F)
    {
        if (z < VIEW_MIN_F) z = VIEW_MIN_F;
        if (z >= VIEW_MAX_F) z = VIEW_MAX_F;
        res.clip = 16;
    } else {
        res.clip = 0;
    }

    int32 fogZ = z >> FIXED_SHIFT;
    res.z = fogZ;

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

    res.g = vg >> 8;

    int32 x = DP43c(m[0], vx, vy, vz);
    int32 y = DP43c(m[1], vx, vy, vz);

    PERSPECTIVE(x, y, z);

    x = X_CLAMP(x, -GUARD_BAND, GUARD_BAND);
    y = X_CLAMP(y, -GUARD_BAND, GUARD_BAND);

    res.x = x + (FRAME_WIDTH  >> 1);
    res.y = y + (FRAME_HEIGHT >> 1);

    res.clip |= classify(res, viewport);
}

void transformRoom(const RoomVertex* vertices, int32 vCount, bool applyCaustics)
{
    int32 causticsValue = 0;

    for (int32 i = 0; i < vCount; i++)
    {
        if (applyCaustics) {
            causticsValue = caustics[(randTable[i & (MAX_RAND_TABLE - 1)] + causticsFrame) & (MAX_CAUSTICS - 1)];
        }

        transformRoomVertex(vertices, causticsValue);
        vertices++;
    }
}

void transformMesh(const vec3s* vertices, int32 vCount, const uint16* vIntensity, const vec3s* vNormal)
{
    // TODO calc lighting for vNormal
    for (int32 i = 0; i < vCount; i++)
    {
        ASSERT(!vIntensity || (vIntensity[i] + lightAmbient >= 0)); // ohhh, use X_CLAMP...

        transform(vertices->x, vertices->y, vertices->z, vIntensity ? X_MIN(vIntensity[i] + lightAmbient, 8191) : lightAmbient);
        vertices++;
    }
}

VertexUV* clipPoly(VertexUV* poly, VertexUV* tmp, int32 &pCount)
{
    #define LERP(a,b,t)         (b + ((a - b) * t >> 12))
    #define LERP2(a,b,ta,tb)    (b + (((a - b) * ta / tb) >> 12) )

    #define CLIP_AXIS(X, Y, edge, output) {\
        int32 ta = (edge - b->v.X) << 12;\
        int32 tb = (a->v.X - b->v.X);\
        ASSERT(tb != 0);\
        int32 t = ta / tb;\
        VertexUV* v = output + count++;\
        v->v.X = edge;\
        v->v.Y = LERP2(a->v.Y, b->v.Y, ta, tb);\
        v->v.g = LERP(a->v.g, b->v.g, t);\
        v->t.u = LERP(a->t.u, b->t.u, t);\
        v->t.v = LERP(a->t.v, b->t.v, t);\
    }

    #define CLIP_XY(X, Y, X0, X1, input, output) {\
        const VertexUV *a, *b = input + pCount - 1;\
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

    VertexUV *in = poly;
    VertexUV *out = tmp;

    // clip x
    CLIP_XY(x, y, viewport.x0, viewport.x1, in, out);

    pCount = count;
    count = 0;

    // clip y
    CLIP_XY(y, x, viewport.y0, viewport.y1, out, in);
    pCount = count;

    return in;
}

void rasterize(const Face* face, const VertexUV *top)
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

void drawTriangle(const Face* face, VertexUV *v)
{
    VertexUV *v1 = v + 0,
             *v2 = v + 1,
             *v3 = v + 2;

    v1->next = v2;
    v2->next = v3;
    v3->next = v1;
    v1->prev = v3;
    v2->prev = v1;
    v3->prev = v2;

    const VertexUV* top = v1;
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

void drawQuad(const Face* face, VertexUV *v)
{
    VertexUV *v1 = v + 0,
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

    VertexUV* top;

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

void drawPoly(Face* face, VertexUV* v)
{
    VertexUV tmp[16];

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

    VertexUV* top = v;
    top->next = v + 1;
    top->prev = v + count - 1;

    bool skip = true;

    for (int32 i = 1; i < count; i++)
    {
        VertexUV *p = v + i;

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

void drawSprite(Face* face, VertexUV* v)
{
    sprite = sprites + face->indices[1];
    tile = tiles + (sprite->tile << 16);

    rasterize(face, v);
}

void drawGlyph(const Sprite *sprite, int32 x, int32 y)
{
    int32 w = sprite->r - sprite->l;
    int32 h = sprite->b - sprite->t;

    w = (w >> 1) << 1; // make it even

    int32 ix = x + sprite->l;
    int32 iy = y + sprite->t;

#if defined(MODE_PAL)
    uint16* pixel = (uint16*)fb + iy * VRAM_WIDTH + (ix >> 1);
#else
    uint16* pixel = (uint16*)fb + iy * VRAM_WIDTH + ix;
#endif

    const uint8* glyphData = tiles + (sprite->tile << 16) + 256 * sprite->v + sprite->u;

    while (h--)
    {
    #if defined(MODE_PAL)
        const uint8* p = glyphData;

        for (int32 i = 0; i < (w / 2); i++)
        {
            if (p[0] || p[1])
            {
                uint16 d = pixel[i];

                if (p[0]) d = (d & 0xFF00) | p[0];
                if (p[1]) d = (d & 0x00FF) | (p[1] << 8);

                pixel[i] = d;
            }

            p += 2;
        }
    #else
        for (int32 i = 0; i < w; i++)
        {
            if (glyphData[i] == 0) continue;
            pixel[i] = palette[glyphData[i]];
        }
    #endif

        pixel += VRAM_WIDTH;

        glyphData += 256;
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

void faceAddQuad(uint32 flags, const Index* indices, int32 startVertex)
{
    ASSERT(gFacesCount < MAX_FACES);

    const Vertex* v = gVertices + startVertex;
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

    int32 depth = X_MAX(v1->z, X_MAX(v2->z, X_MAX(v3->z, v4->z))) >> OT_SHIFT;

    Face* f = faceAdd(depth);
    f->flags = uint16(flags);
    f->indices[0] = v1 - gVertices;
    f->indices[1] = v2 - gVertices;
    f->indices[2] = v3 - gVertices;
    f->indices[3] = v4 - gVertices;
}

void faceAddTriangle(uint32 flags, const Index* indices, int32 startVertex)
{
    ASSERT(gFacesCount < MAX_FACES);

    const Vertex* v = gVertices + startVertex;
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

    int32 depth = X_MAX(v1->z, X_MAX(v2->z, v3->z)) >> OT_SHIFT;

    Face* f = faceAdd(depth);
    f->flags = uint16(flags | FACE_TRIANGLE);
    f->indices[0] = v1 - gVertices;
    f->indices[1] = v2 - gVertices;
    f->indices[2] = v3 - gVertices;
}

void faceAddSprite(int32 vx, int32 vy, int32 vz, int32 vg, int32 index)
{
    const Matrix &m = matrixGet();

    int32 z = DP43c(m[2], vx, vy, vz);

    if (z < VIEW_MIN_F || z >= VIEW_MAX_F)
    {
        return;
    }

    ASSERT(gFacesCount < MAX_FACES);

    int32 x = DP43c(m[0], vx, vy, vz);
    int32 y = DP43c(m[1], vx, vy, vz);

    const Sprite* sprite = sprites + index;

    int32 l = x + (sprite->l << FIXED_SHIFT);
    int32 r = x + (sprite->r << FIXED_SHIFT);
    int32 t = y + (sprite->t << FIXED_SHIFT);
    int32 b = y + (sprite->b << FIXED_SHIFT);

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

    int32 fogZ = z >> FIXED_SHIFT;
    if (fogZ > FOG_MIN)
    {
        vg += (fogZ - FOG_MIN) << FOG_SHIFT;
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

    int32 depth = X_MAX(0, fogZ - 128); // depth hack

    Face* f = faceAdd(depth >> OT_SHIFT);
    f->flags = uint16(FACE_SPRITE);
    f->indices[0] = gVerticesCount - 2;
    f->indices[1] = index;
}

void faceAddRoom(const Quad* quads, int32 qCount, const Triangle* triangles, int32 tCount, int32 startVertex)
{
    for (uint16 i = 0; i < qCount; i++) {
        faceAddQuad(quads[i].flags, quads[i].indices, startVertex);
    }

    for (uint16 i = 0; i < tCount; i++) {
        faceAddTriangle(triangles[i].flags, triangles[i].indices, startVertex);
    }
}

void faceAddMesh(const Quad* rFaces, const Quad* crFaces, const Triangle* tFaces, const Triangle* ctFaces, int32 rCount, int32 crCount, int32 tCount, int32 ctCount, int32 startVertex)
{
    for (int i = 0; i < rCount; i++) {
        faceAddQuad(rFaces[i].flags, rFaces[i].indices, startVertex);
    }

    for (int i = 0; i < crCount; i++) {
        faceAddQuad(crFaces[i].flags | FACE_COLORED, crFaces[i].indices, startVertex);
    }

    for (int i = 0; i < tCount; i++) {
        faceAddTriangle(tFaces[i].flags, tFaces[i].indices, startVertex);
    }

    for (int i = 0; i < ctCount; i++) {
        faceAddTriangle(ctFaces[i].flags | FACE_COLORED, ctFaces[i].indices, startVertex);
    }
}

#ifdef DEBUG_FACES
    int32 gFacesCountMax, gVerticesCountMax;
#endif

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
                VertexUV v[16];

                uint32 flags = face->flags;

                if (flags == FACE_SPRITE) {
                    const Sprite &sprite = sprites[face->indices[1]];

                    v[0].v = gVertices[face->indices[0] + 0];
                    v[0].t.u = sprite.u;
                    v[0].t.v = sprite.v;
                    v[1].v = gVertices[face->indices[0] + 1];
                    v[1].t.u = sprite.w;
                    v[1].t.v = sprite.h;

                    ASSERT(v[0].v.x <= v[1].v.x);
                    ASSERT(v[0].v.y <= v[1].v.y);

                    drawSprite(face, v);
                } else {
                    if (!(flags & FACE_COLORED))
                    {
                        const Texture &tex = textures[flags & FACE_TEXTURE];
                        tile = tiles + (tex.tile << 16);

                        v[0].t.uv = tex.uv0;
                        v[1].t.uv = tex.uv1;
                        v[2].t.uv = tex.uv2;
                        v[3].t.uv = tex.uv3;

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
                    };
                }

                face = face->next;
            } while (face);
        }

        otMin = OT_SIZE - 1;
        otMax = 0;
    }

#ifdef DEBUG_FACES
    if ((gFacesCount > gFacesCountMax) || (gVerticesCount > gVerticesCountMax))
    {
        if (gFacesCount > gFacesCountMax) gFacesCountMax = gFacesCount;
        if (gVerticesCount > gVerticesCountMax) gVerticesCountMax = gVerticesCount;
        printf("v: %d f: %d\n", gVerticesCountMax, gFacesCountMax);
    }
#endif

#ifdef PROFILING
    #if !defined(PROFILE_FRAMETIME) && !defined(PROFILE_SOUNDTIME)
        gCounters[CNT_VERT] += gVerticesCount;
        gCounters[CNT_POLY] += gFacesCount;
    #endif
#endif

    gVerticesCount = 0;
    gFacesCount = 0;
}

void clear()
{
    dmaFill((void*)fb, 0, VRAM_WIDTH * FRAME_HEIGHT * 2);
}

#ifdef IWRAM_MATRIX_LERP
void matrixLerp(const Matrix &n, int32 multiplier, int32 divider)
{
    Matrix &m = matrixGet();

    if ((divider == 2) || ((divider == 4) && (multiplier == 2))) {
        LERP_MATRIX(LERP_1_2);
    } else if (divider == 4) {

        if (multiplier == 1) {
            LERP_MATRIX(LERP_1_4);
        } else {
            LERP_MATRIX(LERP_3_4);
        }

    } else {
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