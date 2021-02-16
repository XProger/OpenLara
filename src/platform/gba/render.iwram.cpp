#include "common.h"

#define DIV_TABLE_SIZE 256

uint16 divTable[DIV_TABLE_SIZE];    // IWRAM 0.5 kb

#if defined(_WIN32)
    uint8 fb[WIDTH * HEIGHT * 2];
#elif defined(__GBA__)
    uint32 fb = VRAM;
#elif defined(__TNS__)
    uint8 fb[WIDTH * HEIGHT];
#endif

#define FixedInvS(x) ((x < 0) ? -divTable[abs(x)] : divTable[x])
#define FixedInvU(x) divTable[x]

#if defined(USE_MODE_5) || defined(_WIN32)
    uint16 palette[256];
#endif

uint8 lightmap[256 * 32]; // IWRAM 8 kb

const uint8* tiles;
const uint8* tile;

const Texture* textures;

uint32 gVerticesCount = 0;
int32 gFacesCount = 0;

EWRAM_DATA Vertex gVertices[MAX_VERTICES]; // EWRAM 8 kb
EWRAM_DATA Face* gFacesSorted[MAX_FACES];  // EWRAM 2 kb
EWRAM_DATA Face gFaces[MAX_FACES];         // EWRAM 5 kb

//uint16 mipMask;

Rect clip;

Matrix matrixStack[MAX_MATRICES];
int32  matrixStackIndex = 0;

template <class T>
INLINE void swap(T &a, T &b) {
    T tmp = a;
    a = b;
    b = tmp;
}

INLINE bool checkBackface(const Vertex *a, const Vertex *b, const Vertex *c) {
    return (b->x - a->x) * (c->y - a->y) <= (c->x - a->x) * (b->y - a->y);
}

INLINE void sortVertices(VertexUV *&t, VertexUV *&m, VertexUV *&b) {
    if (t->v.y > m->v.y) swap(t, m);
    if (t->v.y > b->v.y) swap(t, b);
    if (m->v.y > b->v.y) swap(m, b);
}

INLINE int32 classify(const Vertex* v) {
    return (v->x < clip.x0 ? 1 : 0) |
           (v->x > clip.x1 ? 2 : 0) |
           (v->y < clip.y0 ? 4 : 0) |
           (v->y > clip.y1 ? 8 : 0);
}

void transform(const vec3s &v, int32 vg) {
#if defined(_WIN32)
    if (gVerticesCount >= MAX_VERTICES) {
        DebugBreak();
        return;
    }
#endif
    const Matrix &m = matrixStack[matrixStackIndex];

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

void transform_room(const Room::Vertex* vertex, int32 vCount)
{
    for (int32 i = 0; i < vCount; i++)
    {
        transform(vertex->pos, vertex->lighting);
        vertex++;
    }
}

void transform_mesh(const vec3s* vertices, int32 vCount)
{
    for (int32 i = 0; i < vCount; i++) {
        transform(*vertices++, 4096);
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
        v->uv = (LERP(a->uv & 0xFFFF, b->uv & 0xFFFF, t)) | (LERP(a->uv >> 16, b->uv >> 16, t) << 16);\
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
#define FETCH_T()               tile[(t & 0xFF00) | (t >> 24)]
#define FETCH_T_MIP()           tile[(t & 0xFF00) | (t >> 24) & mipMask]
#define FETCH_GT()              lightmap[(g & 0x1F00) | FETCH_T()]

#define FETCH_T2(t)             tile[(t & 0xFF00) | (t >> 24)]

INLINE uint32 FETCH_GT2(uint32 &g, uint32 &t, uint32 dgdx, uint32 dtdx) {
#if 0
    uint32 light = g & 0x1F00;
    uint32 p = lightmap[light | FETCH_T()];
    t += dtdx;
    p |= lightmap[light | FETCH_T()] << 8;
    t += dtdx;
    g += dgdx;
    return p;
#else
    uint32 p = FETCH_GT();
    t += dtdx;
    p |= FETCH_GT() << 8;
    t += dtdx;
    g += dgdx;
    return p;
#endif
}

#define FETCH_G(palIndex)       lightmap[(g & 0x1F00) | palIndex]
#define FETCH_GT_PAL()          palette[FETCH_GT()]
#define FETCH_G_PAL(palIndex)   palette[FETCH_G(palIndex)]
#endif

struct Edge {
    int32  h;
    int32  x;
    int32  g;
    uint32 t;
    int32  dx;
    int32  dg;
    uint32 dt;

    int32     index;
    VertexUV* vert[8];

    Edge() : h(0), dx(0), dg(0), dt(0) {}

    INLINE void stepG() {
        x += dx;
        g += dg;
    }

    INLINE void stepGT() {
        x += dx;
        g += dg;
        t += dt;
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
        t = (v1->uv >> 16) | (v1->uv << 16); // TODO preprocess

        if (h > 1) {
            uint32 d = FixedInvU(h);

            dx = d * (v2->v.x - v1->v.x);
            dg = d * (v2->v.g - v1->v.g);

            int32 du = d * ((v2->uv & 0xFFFF) - (v1->uv & 0xFFFF));
            int32 dv = d * ((v2->uv >> 16) - (v1->uv >> 16));

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
    #if defined(USE_MODE_5)
        uint16* pixel = buffer + x1;

        if (x1 & 1) {
            *pixel++ = FETCH_G_PAL(palIndex);
            g += dgdx;
            x1++;

            if (x1 >= x2) {
                return;
            }
        }

        int32 width2 = (x2 - x1) >> 1;

        dgdx <<= 1;

        while (width2--) {
            uint32 p = FETCH_G_PAL(palIndex);
            g += dgdx;

            *(uint32*)pixel = p | (p << 16);
            pixel += 2;
        }

        if (x2 & 1) {
            *pixel++ = FETCH_G_PAL(palIndex);
        }
    #elif defined(USE_MODE_4)
        if (x1 & 1)
        {
            uint16 &p = *(uint16*)((uint8*)buffer + x1 - 1);
            p = (p & 0x00FF) | (FETCH_G(palIndex) << 8);
            g += dgdx;
            x1++;
        }

        int32 width = (x2 - x1) >> 1;
        uint16* pixel = (uint16*)((uint8*)buffer + x1);

        dgdx <<= 1;

        if (width && (x1 & 3))
        {
            uint16 p = FETCH_G(palIndex);
            *pixel++ = p | (FETCH_G(palIndex) << 8);

            g += dgdx;

            width--;
        }

        while (width-- > 0)
        {
            uint32 p = FETCH_G(palIndex);
            p |= (FETCH_G(palIndex) << 8);

            g += dgdx;

            if (width-- > 0)
            {
                p |= (FETCH_G(palIndex) << 16);
                p |= (FETCH_G(palIndex) << 24);

                g += dgdx;

                *(uint32*)pixel = p;
                pixel += 2;
            } else {
                *(uint16*)pixel = p;
                pixel += 1;
            }
        }

        if (x2 & 1)
        {
            *pixel = (*pixel & 0xFF00) | FETCH_G(palIndex);
        }
    #else
        if (x1 & 1)
        {
            *((uint8*)buffer + x1) = FETCH_G(palIndex);
            g += dgdx;
            x1++;
        }

        int32 width = (x2 - x1) >> 1;
        uint16* pixel = (uint16*)((uint8*)buffer + x1);

        dgdx <<= 1;

        if (width && (x1 & 3))
        {
            uint16 p = FETCH_G(palIndex);
            *pixel++ = p | (FETCH_G(palIndex) << 8);

            g += dgdx;

            width--;
        }

        while (width-- > 0)
        {
            uint32 p = FETCH_G(palIndex);
            p |= (FETCH_G(palIndex) << 8);

            g += dgdx;

            if (width-- > 0)
            {
                p |= (FETCH_G(palIndex) << 16);
                p |= (FETCH_G(palIndex) << 24);

                g += dgdx;

                *(uint32*)pixel = p;
                pixel += 2;
            } else {
                *(uint16*)pixel = p;
                pixel += 1;
            }
        }

        if (x2 & 1)
        {
            *((uint8*)pixel) = FETCH_G(palIndex);
        }
    #endif
}

INLINE void scanlineGT(uint16* buffer, int32 x1, int32 x2, uint32 g, uint32 t, uint32 dgdx, uint32 dtdx) {
    #if defined(USE_MODE_5)
        uint16* pixel = buffer + x1;

        if (x1 & 1) {
            *pixel++ = FETCH_GT_PAL();
            t += dtdx;
            g += dgdx;
            x1++;

            if (x1 >= x2) {
                return;
            }
        }

        int32 width2 = (x2 - x1) >> 1;

        dgdx <<= 1;

        while (width2--) {
            uint32 p = FETCH_GT_PAL();
            t += dtdx;
            p |= FETCH_GT_PAL() << 16;
            t += dtdx;
            g += dgdx;

            *(uint32*)pixel = p;
            pixel += 2;
        }

        if (x2 & 1) {
            *pixel++ = FETCH_GT_PAL();
        }

    #elif defined(USE_MODE_4)
        uint8* pixel = (uint8*)buffer + x1;

        // align to 2
        if (x1 & 1)
        {
            pixel--;
            *(uint16*)pixel = *pixel | (FETCH_GT() << 8);
            pixel += 2;
            t += dtdx;
            g += dgdx;
            x1++;
        }

        int32 width = (x2 - x1) >> 1;

        dgdx <<= 1;

        // align to 4
        if (width && (x1 & 3))
        {
            *(uint16*)pixel = FETCH_GT2(g, t, dgdx, dtdx);
            pixel += 2;
            width--;
        }

        // fast line
        if (width > 0)
        {
            while (width)
            {
                uint32 p = FETCH_GT2(g, t, dgdx, dtdx);
                if (width > 1) {
                    // write 4 px
                    p |= (FETCH_GT2(g, t, dgdx, dtdx) << 16);
                    *(uint32*)pixel = p;
                    pixel += 4;
                    width -= 2;
                    continue;
                }
                // write 2 px, end of fast line
                *(uint16*)pixel = p;
                pixel += 2;
                width -= 1;
            }
        }

        // write 1 px, end of scanline
        if (x2 & 1)
        {
            *(uint16*)pixel = (*(uint16*)pixel & 0xFF00) | FETCH_GT();
        }
    #else
        if (x1 & 1)
        {
            *((uint8*)buffer + x1) = FETCH_GT();
            t += dtdx;
            g += dgdx;
            x1++;
        }

        int32 width = (x2 - x1) >> 1;
        uint16* pixel = (uint16*)((uint8*)buffer + x1);

        dgdx <<= 1;

        if (width && (x1 & 3))
        {
            uint16 p = FETCH_GT();
            t += dtdx;
            *pixel++ = p | (FETCH_GT() << 8);
            t += dtdx;

            g += dgdx;

            width--;
        }

        while (width-- > 0)
        {
            uint32 p = FETCH_GT();
            t += dtdx;
            p |= (FETCH_GT() << 8);
            t += dtdx;

            g += dgdx;

            if (width-- > 0)
            {
                p |= (FETCH_GT() << 16);
                t += dtdx;
                p |= (FETCH_GT() << 24);
                t += dtdx;

                g += dgdx;

                *(uint32*)pixel = p;
                pixel += 2;
            } else {
                *(uint16*)pixel = p;
                pixel += 1;
            }
        }

        if (x2 & 1)
        {
            *((uint8*)pixel) = FETCH_GT();
        }
    #endif
}

void rasterizeG(int16 y, int32 palIndex, Edge &L, Edge &R) {
    uint16 *buffer = (uint16*)fb + y * (WIDTH / PIXEL_SIZE);

    while (1)
    {
        while (L.h <= 0)
        {
            if (!L.nextG())
            {
                return;
            }
        }

        while (R.h <= 0)
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

            buffer += WIDTH / PIXEL_SIZE;

            L.stepG();
            R.stepG();
        }
    }
}

void rasterizeGT(int16 y, Edge &L, Edge &R) {
    uint16 *buffer = (uint16*)fb + y * (WIDTH / PIXEL_SIZE);

    while (1)
    {
        while (L.h <= 0)
        {
            if (!L.nextGT())
            {
                return;
            }
        }

        while (R.h <= 0)
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

                uint32 u = d * ((R.t >> 16) - (L.t >> 16));
                uint32 v = d * ((R.t & 0xFFFF) - (L.t & 0xFFFF));
                uint32 dtdx = (u & 0xFFFF0000) | (v >> 16);

                scanlineGT(buffer, x1, x2, L.g >> 8, L.t, dgdx, dtdx);
            };

            buffer += WIDTH / PIXEL_SIZE;

            L.stepGT();
            R.stepGT();
        }
    }
}

void drawTriangle(const Face* face, VertexUV *v) {
    VertexUV *v1 = v + 0,
             *v2 = v + 1,
             *v3 = v + 2;

    sortVertices(v1, v2, v3);

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
    }
    else
    {
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
        rasterizeGT(v1->v.y, L, R);
    }
}

void drawQuad(const Face* face, VertexUV *v) {
    VertexUV *v1 = v + 0,
             *v2 = v + 1,
             *v3 = v + 2,
             *v4 = v + 3;

    int32 minY =  0x7FFF;
    int32 maxY = -0x7FFF;
    int32 t = 0, b = 0;

    VertexUV* poly[8] = { v1, v2, v3, v4, v1, v2, v3, v4 };

    for (int32 i = 0; i < 4; i++) {
        VertexUV *v = poly[i];

        if (v->v.y < minY) {
            minY = v->v.y;
            t = i;
        }

        if (v->v.y > maxY) {
            maxY = v->v.y;
            b = i;
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
        rasterizeGT(v1->v.y, L, R);
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
        rasterizeGT(v[t].v.y, L, R);
    }
}

void drawGlyph(const Sprite *sprite, int32 x, int32 y) {
    int32 w = sprite->r - sprite->l;
    int32 h = sprite->b - sprite->t;

    w = (w >> 1) << 1; // make it even

    int32 ix = x + sprite->l;
    int32 iy = y + sprite->t;

    uint16* ptr = (uint16*)fb + iy * (WIDTH / PIXEL_SIZE);

#ifdef USE_MODE_5
    ptr += ix;
#else
    ptr += ix >> 1;
#endif

    const uint8* glyphData = tiles + (sprite->tile << 16) + 256 * sprite->v + sprite->u;

    while (h--)
    {
    #ifdef USE_MODE_5
        for (int32 i = 0; i < w; i++) {
            if (glyphData[i] == 0) continue;

            ptr[i] = palette[glyphData[i]];
        }
    #else
        const uint8* p = glyphData;

        for (int32 i = 0; i < (w / 2); i++) {

            if (p[0] || p[1]) {
                uint16 d = ptr[i];

                if (p[0]) d = (d & 0xFF00) | p[0];
                if (p[1]) d = (d & 0x00FF) | (p[1] << 8);

                ptr[i] = d;
            }

            p += 2;
        }
    #endif

        ptr += WIDTH / PIXEL_SIZE;
        glyphData += 256;
    }
}

void faceAddQuad(uint32 flags, const Index* indices, int32 startVertex) {
#if defined(_WIN32)
    if (gFacesCount >= MAX_FACES) {
        DebugBreak();
    }
#endif
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
#if defined(_WIN32)
    if (gFacesCount >= MAX_FACES) {
        DebugBreak();
    }
#endif
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

    int32 depth = (v1->z + v2->z + v3->z) / 3;

    if (v1->clip | v2->clip | v3->clip) {
        flags |= FACE_CLIPPED;
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

void faceAdd_room(const Quad* quads, int32 qCount, const Triangle* triangles, int32 tCount, int32 startVertex)
{
    for (uint16 i = 0; i < qCount; i++) {
        faceAddQuad(quads[i].flags, quads[i].indices, startVertex);
    }

    for (uint16 i = 0; i < tCount; i++) {
        faceAddTriangle(triangles[i].flags, triangles[i].indices, startVertex);
    }
}

void faceAdd_mesh(const Quad* rFaces, const Quad* crFaces, const Triangle* tFaces, const Triangle* ctFaces, int32 rCount, int32 crCount, int32 tCount, int32 ctCount, int32 startVertex)
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

        //const uint16 mips[] = { 0xFFFF, 0xFEFE, 0xFCFC, 0xF8F8 };

        PROFILE_START();
        for (int32 i = 0; i < gFacesCount; i++) {
            Face *face = gFacesSorted[i];

            // TODO
            //mipMask = mips[MIN(3, f.depth / 2048)];

            VertexUV v[16];

            uint32 flags = face->flags;

            if (!(flags & FACE_COLORED)) {
                const Texture &tex = textures[face->flags & FACE_TEXTURE];
                tile = tiles + (tex.tile << 16);
                v[0].uv = tex.uv0;
                v[1].uv = tex.uv1;
                v[2].uv = tex.uv2;
                v[3].uv = tex.uv3;
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

void initRender() {
    divTable[0] = 0xFFFF;
    divTable[1] = 0xFFFF;
    for (uint32 i = 2; i < DIV_TABLE_SIZE; i++) {
        divTable[i] = (1 << 16) / i;
    }
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
    dmaClear((uint32*)fb, (WIDTH * HEIGHT) >> PIXEL_SIZE);
}
