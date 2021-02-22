#include "common.h"

#if defined(_WIN32)
    uint16 fb[WIDTH * HEIGHT];
#elif defined(__GBA__)
    uint32 fb = VRAM;
#elif defined(__TNS__)
    uint16 fb[WIDTH * HEIGHT];
#endif

#define PAL_COLOR_TRANSP    0x0000
#define PAL_COLOR_BLACK     0x0421

uint16 palette[256];       // IWRAM 0.5 kb
uint8  lightmap[256 * 32]; // IWRAM 8 kb
const uint8* ft_lightmap;

const Texture* textures;
const uint8*   tiles;
const uint8*   tile;

uint32 gVerticesCount = 0;
int32 gFacesCount = 0;

EWRAM_DATA Vertex gVertices[MAX_VERTICES]; // EWRAM 16 kb
EWRAM_DATA Face* gFacesSorted[MAX_FACES];  // EWRAM 2 kb
EWRAM_DATA Face gFaces[MAX_FACES];         // EWRAM 5 kb

//uint16 mipMask;

Rect clip;

template <class T>
INLINE void swap(T &a, T &b) {
    T tmp = a;
    a = b;
    b = tmp;
}

INLINE bool checkBackface(const Vertex *a, const Vertex *b, const Vertex *c) {
    return (b->x - a->x) * (c->y - a->y) <= (c->x - a->x) * (b->y - a->y);
}

INLINE int32 classify(const Vertex* v) {
    return (v->x < clip.x0 ? 1 : 0) |
           (v->x > clip.x1 ? 2 : 0) |
           (v->y < clip.y0 ? 4 : 0) |
           (v->y > clip.y1 ? 8 : 0);
}

bool boxIsVisible(const Box* box)
{
    const Matrix &m = matrixGet();

    if (m[2][3] >= VIEW_MAX_F) {
        return false;
    }

    if (m[2][3] < VIEW_MIN_F) { // TODO large objects
        return false;
    }

    const vec3i v[8] {
        { box->minX, box->minY, box->minZ },
        { box->maxX, box->minY, box->minZ },
        { box->minX, box->maxY, box->minZ },
        { box->maxX, box->maxY, box->minZ },
        { box->minX, box->minY, box->maxZ },
        { box->maxX, box->minY, box->maxZ },
        { box->minX, box->maxY, box->maxZ },
        { box->maxX, box->maxY, box->maxZ }
    };

    Rect rect = { INT_MAX, INT_MAX, INT_MIN, INT_MIN };

    for (int32 i = 0; i < 8; i++) {
        int32 z = DP43(m[2], v[i]);

        if (z < VIEW_MIN_F || z >= VIEW_MAX_F) { // TODO znear clip
            continue;
        }

        int32 x = DP43(m[0], v[i]);
        int32 y = DP43(m[1], v[i]);

        z >>= FOV_SHIFT;

        x = (x / z);
        y = (y / z);

        if (x < rect.x0) rect.x0 = x;
        if (x > rect.x1) rect.x1 = x;
        if (y < rect.y0) rect.y0 = y;
        if (y > rect.y1) rect.y1 = y;
    }

    rect.x0 += (FRAME_WIDTH  / 2);
    rect.y0 += (FRAME_HEIGHT / 2);
    rect.x1 += (FRAME_WIDTH  / 2);
    rect.y1 += (FRAME_HEIGHT / 2);

    return !(rect.x0 > rect.x1 ||
             rect.x0 > clip.x1 ||
             rect.x1 < clip.x0 ||
             rect.y0 > clip.y1 ||
             rect.y1 < clip.y0);
}

void transform(const vec3s &v, int32 vg) {
    ASSERT(gVerticesCount < MAX_VERTICES);

    const Matrix &m = matrixGet();

    Vertex &res = gVertices[gVerticesCount++];

    // TODO https://mikro.naprvyraz.sk/docs/Coding/1/3D-ROTAT.TXT
    int32 z = DP43(m[2], v);

    if (z < VIEW_MIN_F || z >= VIEW_MAX_F) { // TODO znear clip
        res.clip = 16;
        res.z = -1;
        return;
    }

    int32 x = DP43(m[0], v);
    int32 y = DP43(m[1], v);

    int32 fogZ = z >> FIXED_SHIFT;
    if (fogZ > FOG_MAX) {
        vg = 8191;
    } else if (fogZ > FOG_MIN) {
        vg += (fogZ - FOG_MIN) << FOG_SHIFT;
        if (vg > 8191) {
            vg = 8191;
        }
    }
    res.g = vg >> 8;

    z >>= FOV_SHIFT;

    x = (x / z);
    y = (y / z);

    res.x = x + (FRAME_WIDTH  / 2);
    res.y = y + (FRAME_HEIGHT / 2);
    res.z = fogZ;
    res.clip = classify(&res);
}

void transformRoom(const RoomInfo::Vertex* vertex, int32 vCount)
{
    for (int32 i = 0; i < vCount; i++)
    {
        transform(vertex->pos, vertex->lighting);
        vertex++;
    }
}

void transformMesh(const vec3s* vertices, int32 vCount, uint16 intensity)
{
    for (int32 i = 0; i < vCount; i++) {
        transform(*vertices++, intensity);
    }
}

#if 0 // TODO
void clipZ(int32 znear, VertexUV *output, int32 &count, const VertexUV *a, const VertexUV *b) {
    #define LERP2(a,b,t) int32((b) + (((a) - (b)) * t))

    float t = (znear - b->v.sz) / float(a->v.sz - b->v.sz);
    VertexUV* v = output + count++;
/*
    int32 ax = (a->v.x - (FRAME_WIDTH  / 2)) * a->v.z;
    int32 ay = (a->v.y - (FRAME_HEIGHT / 2)) * a->v.z;
    int32 bx = (b->v.x - (FRAME_WIDTH  / 2)) * b->v.z;
    int32 by = (b->v.y - (FRAME_HEIGHT / 2)) * b->v.z;
    int32 x = LERP2(ax, bx, t);
    int32 y = LERP2(ay, by, t);
*/
    int32 x = LERP2(a->v.sx, b->v.sx, t);
    int32 y = LERP2(a->v.sy, b->v.sy, t);
    int32 z = LERP2(a->v.sz, b->v.sz, t);
    v->v.x = (x / znear) + (FRAME_WIDTH  / 2);
    v->v.y = (y / znear) + (FRAME_HEIGHT / 2);
    v->v.g = LERP2(a->v.g, b->v.g, t);
    v->uv = (LERP2(a->uv & 0xFFFF, b->uv & 0xFFFF, t)) | (LERP2(a->uv >> 16, b->uv >> 16, t) << 16);
}
#endif
VertexUV* clipPoly(VertexUV* poly, VertexUV* tmp, int32 &pCount) {
    #define LERP(a,b,t) ((b) + (((a) - (b)) * t >> 16))

    #define CLIP_AXIS(X, Y, edge, output) {\
        uint32 t = ((edge - b->v.X) << 16) / (a->v.X - b->v.X);\
        VertexUV* v = output + count++;\
        v->v.X = edge;\
        v->v.Y = LERP(a->v.Y, b->v.Y, t);\
        v->v.z = LERP(a->v.z, b->v.z, t);\
        v->v.g = LERP(a->v.g, b->v.g, t);\
        v->t.u = LERP(a->t.u, b->t.u, t);\
        v->t.v = LERP(a->t.v, b->t.v, t);\
    }

/* TODO
    #define CLIP_NEAR(znear, output) {\
        //clipZ(znear, output, count, a, b);\
        uint32 t = ((znear - b->v.z) << 16) / (a->v.z - b->v.z);\
        VertexUV* v = output + count++;\
        int32 ax = (a->v.x - (FRAME_WIDTH  / 2)) * a->v.z;\
        int32 ay = (a->v.y - (FRAME_HEIGHT / 2)) * a->v.z;\
        int32 bx = (b->v.x - (FRAME_WIDTH  / 2)) * b->v.z;\
        int32 by = (b->v.y - (FRAME_HEIGHT / 2)) * b->v.z;\
        int32 x = LERP(ax, bx, t);\
        int32 y = LERP(ay, by, t);\
        v->v.x = (x / znear) + (FRAME_WIDTH  / 2);\
        v->v.y = (y / znear) + (FRAME_HEIGHT / 2);\
        v->v.z = LERP(a->v.z, b->v.z, t);\
        v->v.g = LERP(a->v.g, b->v.g, t);\
        v->uv = (LERP(a->uv & 0xFFFF, b->uv & 0xFFFF, t)) | (LERP(a->uv >> 16, b->uv >> 16, t) << 16);\
    }
*/

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

    #define ZNEAR (VIEW_MIN_F << FIXED_SHIFT >> FOV_SHIFT)

    #define CLIP_Z(input, output) {\
        const VertexUV *a, *b = input + pCount - 1;\
        for (int32 i = 0; i < pCount; i++) {\
            a = b;\
            b = input + i;\
            if (a->v.z < ZNEAR) {\
                if (b->v.z < ZNEAR) continue;\
                CLIP_NEAR(ZNEAR, output);\
            }\
            if (b->v.z < ZNEAR) {\
                CLIP_NEAR(ZNEAR, output);\
            } else {\
                output[count++] = *b;\
            }\
        }\
        if (count < 3) return NULL;\
    }

    int32 count = 0;

    VertexUV *in = poly;
    VertexUV *out = tmp;
/*
    uint32 clipFlags = poly[0].v.clip | poly[1].v.clip | poly[2].v.clip;
    if (pCount > 3) {
        clipFlags |= poly[3].v.clip;
    }

    if (clipFlags & 16) {
        CLIP_Z(in, out);
        swap(in, out);
        pCount = count;
        count = 0;
    }
*/
    {//if (clipFlags & (1 | 2 | 4 | 8)) {
    // clip x
        CLIP_XY(x, y, clip.x0, clip.x1, in, out);

        pCount = count;
        count = 0;

    // clip y
        CLIP_XY(y, x, clip.y0, clip.y1, out, in);
        pCount = count;
    }

    return in;
}

#ifdef DEBUG_OVERDRAW
#define FETCH_GT()              32
#define FETCH_G(palIndex)       32
#define FETCH_GT_PAL()          32
#define FETCH_G_PAL(palIndex)   32
#else

#define FETCH_T()      tile[(t & 0xFF00) | (t >> 24)]
#define FETCH_G()      lightmap[(g & 0x1F00) | palIndex]
#define FETCH_GT()     lightmap[(g & 0x1F00) | FETCH_T()]
#define FETCH_FT()     ft_lightmap[FETCH_T()]

#define PUT_PIXEL_GT() { uint16 p = palette[FETCH_GT()]; if (p) *pixel = p; }
#define PUT_PIXEL_FT() { uint16 p = palette[FETCH_FT()]; if (p) *pixel = p; }
#define PUT_PIXEL_G()  { *pixel = palette[FETCH_G()]; }
#endif

struct Edge {
    int32  h;
    int32  x;
    int32  g;
    UV     t;
    int32  dx;
    int32  dg;
    uint32 dt;

    int32     index;
    VertexUV* vert[8];

    Edge() : h(0) {}

    INLINE void stepG() {
        x += dx;
        g += dg;
    }

    INLINE void stepGT() {
        x += dx;
        g += dg;
        t.uv += dt;
    }

    INLINE void stepFT() {
        x += dx;
        t.uv += dt;
    }

    INLINE bool nextG() {
        if (index == 0) {
            return false;
        }

        VertexUV* v1 = vert[index--];
        VertexUV* v2 = vert[index];

        h = v2->v.y - v1->v.y;
        x = v1->v.x << 16;
        g = v1->v.g << 16;

        if (h > 1) {
            uint32 d = FixedInvU(h);

            dx = d * (v2->v.x - v1->v.x);
            dg = d * (v2->v.g - v1->v.g);
        }

        return true;
    }

    INLINE bool nextGT() {
        if (index == 0) {
            return false;
        }

        VertexUV* v1 = vert[index--];
        VertexUV* v2 = vert[index];

        h = v2->v.y - v1->v.y;
        x = v1->v.x << 16;
        g = v1->v.g << 16;
        t.v = v1->t.u;
        t.u = v1->t.v;

        if (h > 1) {
            uint32 d = FixedInvU(h);

            dx = d * (v2->v.x - v1->v.x);
            dg = d * (v2->v.g - v1->v.g);

            int32 du = d * (v2->t.v - v1->t.v);
            int32 dv = d * (v2->t.u - v1->t.u);

            dt = (du & 0xFFFF0000) | int16(dv >> 16);
        }

        return true;
    }

    INLINE bool nextFT() {
        if (index == 0) {
            return false;
        }

        VertexUV* v1 = vert[index--];
        VertexUV* v2 = vert[index];

        h = v2->v.y - v1->v.y;
        x = v1->v.x << 16;
        t.u = v1->t.v;
        t.v = v1->t.u;

        if (h > 1) {
            uint32 d = FixedInvU(h);

            dx = d * (v2->v.x - v1->v.x);

            int32 du = d * (v2->t.v - v1->t.v);
            int32 dv = d * (v2->t.u - v1->t.u);

            dt = (du & 0xFFFF0000) | int16(dv >> 16);
        }

        return true;
    }

    void build(VertexUV *vertices, int32 count, int32 t, int32 b, int32 incr) {
        vert[index = 0] = vertices + b;

        for (int32 i = 1; i < count; i++) {
            b = (b + incr) % count;

            VertexUV* v = vertices + b;

            if (vert[index]->v.x != v->v.x || vert[index]->v.y != v->v.y) {
                vert[++index] = v;
            }

            if (b == t) {
                break;
            }
        }
    }
};

INLINE void scanlineG(uint16* buffer, int32 x1, int32 x2, uint8 palIndex, uint32 g, uint32 dgdx) {
    uint16* pixel = buffer + x1;

    int32 width = x2 - x1;

    dgdx <<= 1;

    while (width--) {
        PUT_PIXEL_G();
        pixel++;

        if (width) {
            width--;
            PUT_PIXEL_G();
            pixel++;
        }

        g += dgdx;
    }
}

INLINE void scanlineGT(uint16* buffer, int32 x1, int32 x2, uint32 g, uint32 t, uint32 dgdx, uint32 dtdx) {
    uint16* pixel = buffer + x1;

    int32 width = x2 - x1;

    dgdx <<= 1;

    while (width--) {
        PUT_PIXEL_GT();
        pixel++;
        t += dtdx;

        if (width) {
            width--;
            PUT_PIXEL_GT();
            pixel++;
            t += dtdx;
        }

        g += dgdx;
    }
}

INLINE void scanlineFT(uint16* buffer, int32 x1, int32 x2, uint32 t, uint32 dtdx) {
    uint16* pixel = buffer + x1;

    int32 width = x2 - x1;

    while (width--) {
        PUT_PIXEL_FT();
        pixel++;
        t += dtdx;

        if (width) {
            width--;
            PUT_PIXEL_FT();
            pixel++;
            t += dtdx;
        }
    }
}

void rasterizeG(int16 y, int32 palIndex, Edge &L, Edge &R) {
    uint16 *buffer = (uint16*)fb + y * WIDTH;

    while (1)
    {
        while (!L.h)
        {
            if (!L.nextG())
            {
                return;
            }
        }

        while (!R.h)
        {
            if (!R.nextG())
            {
                return;
            }
        }

        int32 h = MIN(L.h, R.h);
        L.h -= h;
        R.h -= h;

        while (h--) {
            int32 x1 = L.x >> 16;
            int32 x2 = R.x >> 16;

            int32 width = x2 - x1;
            if (width > 0)
            {
                uint32 d = FixedInvU(width);

                uint32 dgdx = d * ((R.g - L.g) >> 8) >> 16;

                scanlineG(buffer, x1, x2, palIndex, L.g >> 8, dgdx);
            }

            buffer += WIDTH;

            L.stepG();
            R.stepG();
        }
    }
}

void rasterizeGT(int16 y, Edge &L, Edge &R) {
    uint16 *buffer = (uint16*)fb + y * WIDTH;

    while (1)
    {
        while (!L.h)
        {
            if (!L.nextGT())
            {
                return;
            }
        }

        while (!R.h)
        {
            if (!R.nextGT())
            {
                return;
            }
        }

        int32 h = MIN(L.h, R.h);
        L.h -= h;
        R.h -= h;

        while (h--) {
            int32 x1 = L.x >> 16;
            int32 x2 = R.x >> 16;

            int32 width = x2 - x1;
            if (width > 0)
            {
                uint32 d = FixedInvU(width);

                uint32 dgdx = d * ((R.g - L.g) >> 8) >> 16;

                uint32 u = d * (R.t.u - L.t.u);
                uint32 v = d * (R.t.v - L.t.v);
                uint32 dtdx = (u & 0xFFFF0000) | (v >> 16);

                scanlineGT(buffer, x1, x2, L.g >> 8, L.t.uv, dgdx, dtdx);
            };

            buffer += WIDTH;

            L.stepGT();
            R.stepGT();
        }
    }
}

void rasterizeFT(int16 y, Edge &L, Edge &R) {
    uint16 *buffer = (uint16*)fb + y * WIDTH;

    ft_lightmap = &lightmap[(L.vert[0]->v.g << 8) & 0x1F00];

    while (1)
    {
        while (!L.h)
        {
            if (!L.nextFT())
            {
                return;
            }
        }

        while (!R.h)
        {
            if (!R.nextFT())
            {
                return;
            }
        }

        int32 h = MIN(L.h, R.h);
        L.h -= h;
        R.h -= h;

        while (h--) {
            int32 x1 = L.x >> 16;
            int32 x2 = R.x >> 16;

            int32 width = x2 - x1;
            if (width > 0)
            {
                uint32 d = FixedInvU(width);

                uint32 u = d * (R.t.u - L.t.u);
                uint32 v = d * (R.t.v - L.t.v);
                uint32 dtdx = (u & 0xFFFF0000) | (v >> 16);

                scanlineFT(buffer, x1, x2, L.t.uv, dtdx);
            };

            buffer += WIDTH;

            L.stepFT();
            R.stepFT();
        }
    }
}

void drawTriangle(const Face* face, VertexUV *v) {
    VertexUV *v1 = v + 0,
             *v2 = v + 1,
             *v3 = v + 2;

    if (v1->v.y > v2->v.y) swap(v1, v2);
    if (v1->v.y > v3->v.y) swap(v1, v3);
    if (v2->v.y > v3->v.y) swap(v2, v3);

    int32 temp = (v2->v.y - v1->v.y) * FixedInvU(v3->v.y - v1->v.y);

    int32 longest = ((temp * (v3->v.x - v1->v.x)) >> 16) + (v1->v.x - v2->v.x);
    if (longest == 0)
    {
        return;
    }

    Edge L, R;

    if (longest < 0)
    {
        R.vert[0] = v3;
        R.vert[1] = v2;
        R.vert[2] = v1;
        R.index   = 2;
        L.vert[0] = v3;
        L.vert[1] = v1;
        L.index   = 1;
    } else {
        L.vert[0] = v3;
        L.vert[1] = v2;
        L.vert[2] = v1;
        L.index   = 2;
        R.vert[0] = v3;
        R.vert[1] = v1;
        R.index   = 1;
    }

    if (face->flags & FACE_COLORED) {
        rasterizeG(v1->v.y, face->flags & FACE_TEXTURE, L, R);
    } else {
        if (face->flags & FACE_FLAT) {
            rasterizeFT(v1->v.y, L, R);
        } else {
            rasterizeGT(v1->v.y, L, R);
        }
    }
}

void drawQuad(const Face* face, VertexUV *v) {
    VertexUV *v1 = v + 0,
             *v2 = v + 1,
             *v3 = v + 2,
             *v4 = v + 3;

    VertexUV* poly[8] = { v1, v2, v3, v4, v1, v2, v3, v4 };

    int32 t, b;

    if (v1->v.y < v2->v.y) {
        if (v1->v.y < v3->v.y) {
            t = (v1->v.y < v4->v.y) ? 0 : 3;
        } else {
            t = (v3->v.y < v4->v.y) ? 2 : 3;
        }
    } else {
        if (v2->v.y < v3->v.y) {
            t = (v2->v.y < v4->v.y) ? 1 : 3;
        } else {
            t = (v3->v.y < v4->v.y) ? 2 : 3;
        }
    }

    if (v1->v.y > v2->v.y) {
        if (v1->v.y > v3->v.y) {
            b = (v1->v.y > v4->v.y) ? 0 : 3;
        } else {
            b = (v3->v.y > v4->v.y) ? 2 : 3;
        }
    } else {
        if (v2->v.y > v3->v.y) {
            b = (v2->v.y > v4->v.y) ? 1 : 3;
        } else {
            b = (v3->v.y > v4->v.y) ? 2 : 3;
        }
    }

    Edge L, R;

    v1 = poly[t];

    L.vert[L.index = 0] = poly[b];
    R.vert[R.index = 0] = poly[b];

    int32 ib = b;
    do {
        L.vert[++L.index] = poly[++b];
    } while (poly[b] != v1);

    b = ib + 4;
    do {
        R.vert[++R.index] = poly[--b];
    } while (poly[b] != v1);

    if (face->flags & FACE_COLORED) {
        rasterizeG(v1->v.y, face->flags & FACE_TEXTURE, L, R);
    } else {
        if (face->flags & FACE_FLAT) {
            rasterizeFT(v1->v.y, L, R);
        } else {
            rasterizeGT(v1->v.y, L, R);
        }
    }
}

void drawPoly(Face* face, VertexUV* v) {
    VertexUV tmp[16];

    int32 count = (face->flags & FACE_TRIANGLE) ? 3 : 4;

    v = clipPoly(v, tmp, count);

    if (!v) return;

    if (count <= 4) {
        face->indices[0] = 0;
        face->indices[1] = 1;
        face->indices[2] = 2;
        face->indices[3] = 3;

        if (count == 3) {
            drawTriangle(face, v);
        } else {
            drawQuad(face, v);
        }
        return;
    }

    int32 minY =  0x7FFF;
    int32 maxY = -0x7FFF;
    int32 t = 0, b = 0;

    for (int32 i = 0; i < count; i++) {
        VertexUV *p = v + i;

        if (p->v.y < minY) {
            minY = p->v.y;
            t = i;
        }

        if (p->v.y > maxY) {
            maxY = p->v.y;
            b = i;
        }
    }

    Edge L, R;

    L.build(v, count, t, b, count + 1);
    R.build(v, count, t, b, count - 1);

    if (face->flags & FACE_COLORED) {
        rasterizeG(v[t].v.y, face->flags & FACE_TEXTURE, L, R);
    } else {
        if (face->flags & FACE_FLAT) {
            rasterizeFT(v[t].v.y, L, R);
        } else {
            rasterizeGT(v[t].v.y, L, R);
        }
    }
}

void drawGlyph(const Sprite *sprite, int32 x, int32 y) {
    int32 w = sprite->r - sprite->l;
    int32 h = sprite->b - sprite->t;

    w = (w >> 1) << 1; // make it even

    int32 ix = x + sprite->l;
    int32 iy = y + sprite->t;

    uint16* ptr = (uint16*)fb + iy * WIDTH;

    ptr += ix;

    const uint8* glyphData = tiles + (sprite->tile << 16) + 256 * sprite->v + sprite->u;

    while (h--)
    {
        for (int32 i = 0; i < w; i++) {
            if (glyphData[i] == 0) continue;

            ptr[i] = palette[glyphData[i]];
        }

        ptr += WIDTH;
        glyphData += 256;
    }
}

void faceAddQuad(uint32 flags, const Index* indices, int32 startVertex) {
    ASSERT(gFacesCount < MAX_FACES);

    Vertex* v = gVertices + startVertex;
    Vertex* v1 = v + indices[0];
    Vertex* v2 = v + indices[1];
    Vertex* v3 = v + indices[2];
    Vertex* v4 = v + indices[3];

    if ((v1->clip | v2->clip | v3->clip | v4->clip) & 16) // TODO znear clip
        return;

    if (checkBackface(v1, v2, v3))
        return;

    if (v1->clip & v2->clip & v3->clip & v4->clip)
        return;

    int32 depth = (v1->z + v2->z + v3->z + v4->z) >> 2;

    if (v1->clip | v2->clip | v3->clip | v4->clip) {
        flags |= FACE_CLIPPED;
    }

    if (v1->g == v2->g && v1->g == v3->g && v1->g == v4->g) {
        flags |= FACE_FLAT;
    }

    Face *f = gFaces + gFacesCount;
    gFacesSorted[gFacesCount++] = f;
    f->flags      = flags;
    f->depth      = depth;
    f->start      = startVertex + indices[0];
    f->indices[0] = 0;
    f->indices[1] = indices[1] - indices[0];
    f->indices[2] = indices[2] - indices[0];
    f->indices[3] = indices[3] - indices[0];
}

void faceAddTriangle(uint32 flags, const Index* indices, int32 startVertex) {
    ASSERT(gFacesCount < MAX_FACES);

    Vertex* v = gVertices + startVertex;
    Vertex* v1 = v + indices[0];
    Vertex* v2 = v + indices[1];
    Vertex* v3 = v + indices[2];

    if ((v1->clip | v2->clip | v3->clip) & 16) // TODO znear clip
        return;

    if (checkBackface(v1, v2, v3))
        return;

    if (v1->clip & v2->clip & v3->clip)
        return;

    int32 depth = (v1->z + v2->z + v3->z + v3->z) >> 2; // not entirely correct but fast

    if (v1->clip | v2->clip | v3->clip) {
        flags |= FACE_CLIPPED;
    }

    if (v1->g == v2->g && v1->g == v3->g) {
        flags |= FACE_FLAT;
    }

    Face *f = gFaces + gFacesCount;
    gFacesSorted[gFacesCount++] = f;
    f->flags      = flags | FACE_TRIANGLE;
    f->depth      = depth;
    f->start      = startVertex + indices[0];
    f->indices[0] = 0;
    f->indices[1] = indices[1] - indices[0];
    f->indices[2] = indices[2] - indices[0];
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

void faceSort(Face** faces, int32 L, int32 R) {
    int32 i = L;
    int32 j = R;
    int16 depth = faces[(i + j) >> 1]->depth;

    while (i <= j) {
        while (faces[i]->depth > depth) i++;
        while (faces[j]->depth < depth) j--;

        if (i <= j) {
            swap(faces[i++], faces[j--]);
        }
    };

    if (L < j) faceSort(faces, L, j);
    if (R > i) faceSort(faces, i, R);
}

#ifdef DEBUG_FACES
    int32 gFacesCountMax, gVerticesCountMax;
#endif

void flush() {
    if (gFacesCount) {
        PROFILE_START();
        faceSort(gFacesSorted, 0, gFacesCount - 1);
        PROFILE_STOP(dbg_sort);

        PROFILE_START();
        for (int32 i = 0; i < gFacesCount; i++) {
            Face *face = gFacesSorted[i];

            VertexUV v[16];

            uint32 flags = face->flags;

            if (!(flags & FACE_COLORED)) {
                const Texture &tex = textures[face->flags & FACE_TEXTURE];
                tile = tiles + (tex.tile << 16);
                v[0].t.uv = tex.uv0;
                v[1].t.uv = tex.uv1;
                v[2].t.uv = tex.uv2;
                v[3].t.uv = tex.uv3;
                palette[0] = (tex.attribute == 1) ? PAL_COLOR_TRANSP : PAL_COLOR_BLACK;
            } else {
                palette[0] = PAL_COLOR_BLACK;
            }

            Vertex *p = gVertices + face->start;
            v[0].v = p[0];
            v[1].v = p[face->indices[1]];
            v[2].v = p[face->indices[2]];
            if (!(flags & FACE_TRIANGLE)) {
                v[3].v = p[face->indices[3]];
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
        PROFILE_STOP(dbg_flush);
    }

#ifdef DEBUG_FACES
    if (gFacesCount > gFacesCountMax) gFacesCountMax = gFacesCount;
    if (gVerticesCount > gVerticesCountMax) gVerticesCountMax = gVerticesCount;
    printf("f: %d v: %d\n", gFacesCountMax, gVerticesCountMax);
#endif

#ifdef PROFILE
    dbg_vert_count += gVerticesCount;
    dbg_poly_count += gFacesCount;
#endif

    gVerticesCount = 0;
    gFacesCount = 0;
}

void dmaClear(uint32 *dst, uint32 count) {
#ifdef __GBA__
    vu32 value = 0;
    REG_DMA3SAD	= (vu32)&value;
    REG_DMA3DAD	= (vu32)dst;
    REG_DMA3CNT	= count | (DMA_ENABLE | DMA32 | DMA_SRC_FIXED | DMA_DST_INC);
#else
    memset(dst, 0, count * 4);
#endif
}

void clear() {
    dmaClear((uint32*)fb, (WIDTH * HEIGHT) >> 1);
}
