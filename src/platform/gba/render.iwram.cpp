#include "common.h"

#if defined(_WIN32)
    uint16 fb[FRAME_WIDTH * FRAME_HEIGHT];
#elif defined(__GBA__)
    uint32 fb = MEM_VRAM;
#elif defined(__TNS__)
    uint16 fb[FRAME_WIDTH * FRAME_HEIGHT];
#endif

#define PAL_COLOR_TRANSP    0x0000
#define PAL_COLOR_BLACK     0x0421

uint16 palette[256];       // IWRAM 0.5 kb
uint8 lightmap[256 * 32]; // IWRAM 8 kb
const uint8* ft_lightmap;

const Texture* textures;
const uint8* tiles;
const uint8* tile;

uint32 gVerticesCount = 0;
int32 gFacesCount = 0; // 1 is reserved as OT terminator

EWRAM_DATA Vertex gVertices[MAX_VERTICES]; // EWRAM 16 kb
EWRAM_DATA Face gFaces[MAX_FACES];         // EWRAM 5 kb
EWRAM_DATA Face* otFaces[OT_SIZE] = { 0 };
int32 otMin = OT_SIZE - 1;
int32 otMax = 0;
bool alphaKill;

X_INLINE bool checkBackface(const Vertex *a, const Vertex *b, const Vertex *c) {
    return (b->x - a->x) * (c->y - a->y) <= (c->x - a->x) * (b->y - a->y);
}

bool transformBoxRect(const Box* box, Rect* rect)
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

    *rect = { INT_MAX, INT_MAX, INT_MIN, INT_MIN };

    for (int32 i = 0; i < 8; i++) {
        int32 z = DP43(m[2], v[i]);

        if (z < VIEW_MIN_F || z >= VIEW_MAX_F) { // TODO znear clip
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

bool rectIsVisible(const Rect* rect)
{
    return !(rect->x0 > rect->x1 ||
             rect->x0 > clip.x1  ||
             rect->x1 < clip.x0  ||
             rect->y0 > clip.y1  ||
             rect->y1 < clip.y0);
}

bool boxIsVisible(const Box* box)
{
    Rect rect;
    return transformBoxRect(box, &rect) && rectIsVisible(&rect);
}

void transform(const vec3s &v, int32 vg)
{
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

    PERSPECTIVE(x, y, z)

    //x = X_CLAMP(x, -512, 511);
    //y = X_CLAMP(y, -512, 511);

    res.x = x + (FRAME_WIDTH  >> 1);
    res.y = y + (FRAME_HEIGHT >> 1);
    res.z = fogZ;
    res.clip = classify(&res, clip);
}

void transformRoom(const RoomInfo::Vertex* vertex, int32 vCount)
{
    // TODO per-sector clipping?
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
        /*v->v.z = LERP(a->v.z, b->v.z, t);*/\
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
#define INC_PIXEL()     { *pixel += 0b001000010000100; }
#define PUT_PIXEL_F()   INC_PIXEL()
#define PUT_PIXEL_G()   INC_PIXEL()
#define PUT_PIXEL_FT()  INC_PIXEL()
#define PUT_PIXEL_GT()  INC_PIXEL()
#define PUT_PIXEL_FTA() INC_PIXEL()
#define PUT_PIXEL_GTA() INC_PIXEL()
#define SHADE_PIXEL2()  INC_PIXEL()
#else
#define FETCH_T()      tile[(t & 0xFF00) | (t >> 24)]
#define FETCH_G()      ft_lightmap[g & 0x1F00]
#define FETCH_GT()     lightmap[(g & 0x1F00) | FETCH_T()]
#define FETCH_FT()     ft_lightmap[FETCH_T()]

#define PUT_PIXEL_F()   { *pixel = color; }
#define PUT_PIXEL_G()   { *pixel = palette[FETCH_G()]; }
#define PUT_PIXEL_FT()  { uint16 p = palette[FETCH_FT()]; *pixel = p; }
#define PUT_PIXEL_GT()  { uint16 p = palette[FETCH_GT()]; *pixel = p; }
#define PUT_PIXEL_FTA() { uint16 p = palette[FETCH_FT()]; if (p) *pixel = p; }
#define PUT_PIXEL_GTA() { uint16 p = palette[FETCH_GT()]; if (p) *pixel = p; }
#define SHADE_PIXEL2()  { *pixel = (*pixel >> 1) & 0b11110111101111; }
#endif

struct Edge {
    int32  h;
    uint32 x;
    int32  g;
    UV     t;
    int32  dx;
    int32  dg;
    uint32 dt;

    const VertexUV* top;

    Edge(const VertexUV* top) : h(0), top(top) {}

    X_INLINE void stepF() {
        x += dx;
    }

    X_INLINE void stepG() {
        x += dx;
        g += dg;
    }

    X_INLINE void stepGT() {
        x += dx;
        g += dg;
        t.uv += dt;
    }

    X_INLINE void stepFT() {
        x += dx;
        t.uv += dt;
    }

    X_INLINE bool calcF(const VertexUV* next) {
        const VertexUV* v1 = top;
        const VertexUV* v2 = next;
        top = next;

        if (v2->v.y < v1->v.y) return false;

        h = v2->v.y - v1->v.y;
        x = v1->v.x << 16;

        if (h > 1) {
            uint32 d = FixedInvU(h);

            dx = d * (v2->v.x - v1->v.x);
        }

        return true;
    }

    X_INLINE bool calcG(const VertexUV* next) {
        const VertexUV* v1 = top;
        const VertexUV* v2 = next;
        top = next;

        if (v2->v.y < v1->v.y) return false;

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

    X_INLINE bool calcFT(const VertexUV* next) {
        const VertexUV* v1 = top;
        const VertexUV* v2 = next;
        top = next;

        if (v2->v.y < v1->v.y) return false;

        h = v2->v.y - v1->v.y;
        x = v1->v.x << 16;
        t.u = v1->t.v; // TODO preprocess swap
        t.v = v1->t.u; // TODO preprocess swap

        if (h > 1) {
            uint32 d = FixedInvU(h);

            dx = d * (v2->v.x - v1->v.x);

            uint32 du = d * (v2->t.v - v1->t.v); // TODO preprocess swap
            uint32 dv = d * (v2->t.u - v1->t.u); // TODO preprocess swap
            dt = (du & 0xFFFF0000) | (dv >> 16);
        }

        return true;
    }

    X_INLINE bool calcGT(const VertexUV* next) {
        const VertexUV* v1 = top;
        const VertexUV* v2 = next;
        top = next;

        if (v2->v.y < v1->v.y) return false;

        h = v2->v.y - v1->v.y;
        x = v1->v.x << 16;
        g = v1->v.g << 16;
        t.v = v1->t.u; // TODO preprocess swap
        t.u = v1->t.v; // TODO preprocess swap

        if (h > 1) {
            uint32 d = FixedInvU(h);

            dx = d * (v2->v.x - v1->v.x);
            dg = d * (v2->v.g - v1->v.g);

            uint32 du = d * (v2->t.v - v1->t.v); // TODO preprocess swap
            uint32 dv = d * (v2->t.u - v1->t.u); // TODO preprocess swap
            dt = (du & 0xFFFF0000) | (dv >> 16);
        }

        return true;
    }
};

X_INLINE void scanlineF_c(uint16* pixel, int32 width, uint32 color)
{
    while (1)
    {
        PUT_PIXEL_F()
        if (!--width) break;
        pixel++;

        PUT_PIXEL_F()
        if (!--width) break;
        pixel++;
    }
}

X_INLINE void scanlineG_c(uint16* pixel, int32 width, uint32 g, uint32 dgdx)
{
    while (1)
    {
        PUT_PIXEL_G();
        if (!--width) break;
        pixel++;
        
        PUT_PIXEL_G();
        if (!--width) break;
        pixel++;

        g += dgdx;
    }
}

X_INLINE void scanlineGT_c(uint16* pixel, int32 width, uint32 g, uint32 t, uint32 dgdx, uint32 dtdx)
{
    while (1) {
        PUT_PIXEL_GT();
        if (!--width) break;
        pixel++;
        t += dtdx;
        
        PUT_PIXEL_GT();
        if (!--width) break;
        pixel++;
        t += dtdx;

        g += dgdx;
    }
}

X_INLINE void scanlineFT_c(uint16* pixel, int32 width, uint32 t, uint32 dtdx)
{
    while (1)
    {
        PUT_PIXEL_FT();
        if (!--width) break;
        pixel++;
        t += dtdx;

        PUT_PIXEL_FT();
        if (!--width) break;
        pixel++;
        t += dtdx;
    }
}

X_INLINE void scanlineGTA_c(uint16* pixel, int32 width, uint32 g, uint32 t, uint32 dgdx, uint32 dtdx)
{
    while (1) {
        PUT_PIXEL_GTA();
        if (!--width) break;
        pixel++;
        t += dtdx;
        
        PUT_PIXEL_GTA();
        if (!--width) break;
        pixel++;
        t += dtdx;

        g += dgdx;
    }
}

X_INLINE void scanlineFTA_c(uint16* pixel, int32 width, uint32 t, uint32 dtdx)
{
    while (1)
    {
        PUT_PIXEL_FTA();
        if (!--width) break;
        pixel++;
        t += dtdx;

        PUT_PIXEL_FTA();
        if (!--width) break;
        pixel++;
        t += dtdx;
    }
}

X_INLINE void scanlineS_c(uint16* pixel, int32 width)
{
    while (1)
    {
        SHADE_PIXEL2();
        if (!--width) break;
        pixel++;

        SHADE_PIXEL2();
        if (!--width) break;
        pixel++;
    }
}

#if defined(_WIN32)
#define scanlineS scanlineS_c
#define scanlineF scanlineF_c
#define scanlineG scanlineG_c
#define scanlineFT scanlineFT_c
#define scanlineGT scanlineGT_c
#define scanlineGTA scanlineGTA_c
#define scanlineFTA scanlineFTA_c
#define rasterizeF rasterizeF_c
#define rasterizeF_inner rasterizeF_inner_c
#else
extern "C" {
    void scanlineF_asm(uint16* pixel, int32 width, uint16 color);
    void scanlineG_asm(uint16* pixel, int32 width, uint32 g, uint32 dgdx);
    void scanlineFT_asm(uint16* pixel, int32 width, uint32 t, uint32 dtdx);
    void scanlineGT_asm(uint16* pixel, int32 width, uint32 g, uint32 t, uint32 dgdx, uint32 dtdx);
    void scanlineFTA_asm(uint16* pixel, int32 width, uint32 t, uint32 dtdx);
    void scanlineGTA_asm(uint16* pixel, int32 width, uint32 g, uint32 t, uint32 dgdx, uint32 dtdx);
    void rasterizeF_asm(uint16* pixel, Edge &L, Edge &R, int32 palIndex);
    void rasterizeF_asm(uint16* pixel, Edge &L, Edge &R, int32 palIndex);
    uint16* rasterizeF_inner_asm(uint16* pixel, Edge &L, Edge &R, uint32 color);
}

#define scanlineS scanlineS_c
#define scanlineF scanlineF_asm
#define scanlineG scanlineG_asm
#define scanlineFT scanlineFT_asm
#define scanlineGT scanlineGT_asm
#define scanlineGTA scanlineGTA_asm
#define scanlineFTA scanlineFTA_asm
#define rasterizeF rasterizeF_c
#define rasterizeF_inner rasterizeF_inner_asm
#endif

X_INLINE uint16* rasterizeF_inner_c(uint16* pixel, Edge &L, Edge &R, uint32 color)
{
    int32 h = X_MIN(L.h, R.h);
    L.h -= h;
    R.h -= h;
    
    while (h--)
    {
        int32 x1 = L.x >> 16;
        int32 x2 = R.x >> 16;

        int32 width = x2 - x1;

        if (width > 0)
        {
            scanlineF(pixel + x1, width, color);
        }

        pixel += FRAME_WIDTH;

        L.stepF();
        R.stepF();
    }

    return pixel;
}

void rasterizeF_c(uint16* pixel, Edge &L, Edge &R, int32 palIndex)
{
    uint32 color = palette[lightmap[(L.top->v.g << 8) | palIndex]];

    while (1)
    {
        while (!L.h) if (!L.calcF(L.top->prev)) return;
        while (!R.h) if (!R.calcF(R.top->next)) return;

        pixel = rasterizeF_inner(pixel, L, R, color);
    }
}

void rasterizeG(uint16* pixel, Edge &L, Edge &R, int32 palIndex)
{
    ft_lightmap = lightmap + palIndex;

    while (1)
    {
        while (!L.h) if (!L.calcG(L.top->prev)) return;
        while (!R.h) if (!R.calcG(R.top->next)) return;

        int32 h = X_MIN(L.h, R.h);
        L.h -= h;
        R.h -= h;

        while (h--)
        {
            int32 x1 = L.x >> 16;
            int32 x2 = R.x >> 16;

            int32 width = x2 - x1;
            if (width > 0)
            {
                uint32 d = FixedInvU(width);

                uint32 dgdx = d * ((R.g - L.g) >> 8) >> 15;

                scanlineG(pixel + x1, width, L.g >> 8, dgdx);
            }

            pixel += FRAME_WIDTH;

            L.stepG();
            R.stepG();
        }
    }
}

void rasterizeFT(uint16* pixel, Edge &L, Edge &R)
{
    ft_lightmap = &lightmap[L.top->v.g << 8];

    while (1)
    {
        while (!L.h) if (!L.calcFT(L.top->prev)) return;
        while (!R.h) if (!R.calcFT(R.top->next)) return;

        int32 h = X_MIN(L.h, R.h);
        L.h -= h;
        R.h -= h;

        while (h--)
        {
            int32 x1 = L.x >> 16;
            int32 x2 = R.x >> 16;

            int32 width = x2 - x1;
            if (width > 0)
            {
                uint32 d = FixedInvU(width);

                uint32 u = d * (R.t.u - L.t.u);
                uint32 v = d * (R.t.v - L.t.v);
                uint32 dtdx = (u & 0xFFFF0000) | (v >> 16);

                scanlineFT(pixel + x1, width, L.t.uv, dtdx);
            };

            pixel += FRAME_WIDTH;

            L.stepFT();
            R.stepFT();
        }
    }
}

void rasterizeGT(uint16* pixel, Edge &L, Edge &R)
{
    while (1)
    {
        while (!L.h) if (!L.calcGT(L.top->prev)) return;
        while (!R.h) if (!R.calcGT(R.top->next)) return;

        int32 h = X_MIN(L.h, R.h);
        L.h -= h;
        R.h -= h;

        while (h--)
        {
            int32 x1 = L.x >> 16;
            int32 x2 = R.x >> 16;

            int32 width = x2 - x1;
            if (width > 0)
            {
                uint32 d = FixedInvU(width);

                uint32 dgdx = d * ((R.g - L.g) >> 8) >> 15;

                uint32 u = d * (R.t.u - L.t.u);
                uint32 v = d * (R.t.v - L.t.v);
                uint32 dtdx = (u & 0xFFFF0000) | (v >> 16);

                scanlineGT(pixel + x1, width, L.g >> 8, L.t.uv, dgdx, dtdx);
            };

            pixel += FRAME_WIDTH;

            L.stepGT();
            R.stepGT();
        }
    }
}

void rasterizeFTA(uint16* pixel, Edge &L, Edge &R)
{
    ft_lightmap = &lightmap[L.top->v.g << 8];

    while (1)
    {
        while (!L.h) if (!L.calcFT(L.top->prev)) return;
        while (!R.h) if (!R.calcFT(R.top->next)) return;

        int32 h = X_MIN(L.h, R.h);
        L.h -= h;
        R.h -= h;

        while (h--)
        {
            int32 x1 = L.x >> 16;
            int32 x2 = R.x >> 16;

            int32 width = x2 - x1;
            if (width > 0)
            {
                uint32 d = FixedInvU(width);

                uint32 u = d * (R.t.u - L.t.u);
                uint32 v = d * (R.t.v - L.t.v);
                uint32 dtdx = (u & 0xFFFF0000) | (v >> 16);

                scanlineFTA(pixel + x1, width, L.t.uv, dtdx);
            };

            pixel += FRAME_WIDTH;

            L.stepFT();
            R.stepFT();
        }
    }
}

void rasterizeGTA(uint16* pixel, Edge &L, Edge &R)
{
    while (1)
    {
        while (!L.h) if (!L.calcGT(L.top->prev)) return;
        while (!R.h) if (!R.calcGT(R.top->next)) return;

        int32 h = X_MIN(L.h, R.h);
        L.h -= h;
        R.h -= h;

        while (h--)
        {
            int32 x1 = L.x >> 16;
            int32 x2 = R.x >> 16;

            int32 width = x2 - x1;
            if (width > 0)
            {
                uint32 d = FixedInvU(width);

                uint32 dgdx = d * ((R.g - L.g) >> 8) >> 15;

                uint32 u = d * (R.t.u - L.t.u);
                uint32 v = d * (R.t.v - L.t.v);
                uint32 dtdx = (u & 0xFFFF0000) | (v >> 16);

                scanlineGTA(pixel + x1, width, L.g >> 8, L.t.uv, dgdx, dtdx);
            };

            pixel += FRAME_WIDTH;

            L.stepGT();
            R.stepGT();
        }
    }
}

void rasterizeS(uint16* pixel, Edge &L, Edge &R)
{
    while (1)
    {
        while (!L.h) if (!L.calcF(L.top->prev)) return;
        while (!R.h) if (!R.calcF(R.top->next)) return;

        int32 h = X_MIN(L.h, R.h);
        L.h -= h;
        R.h -= h;

        while (h--)
        {
            int32 x1 = L.x >> 16;
            int32 x2 = R.x >> 16;

            int32 width = x2 - x1;
            if (width > 0)
            {
                scanlineS(pixel + x1, width);
            }

            pixel += FRAME_WIDTH;

            L.stepF();
            R.stepF();
        }
    }
}

void rasterize(Edge &L, Edge &R, const Face* face)
{
    uint16* pixel = (uint16*)fb + L.top->v.y * FRAME_WIDTH;
#if 0
    rasterizeF(pixel, L, R, (face->flags & FACE_TEXTURE) + 10);
#else
    if (face->flags & FACE_COLORED) {
        if (face->flags & FACE_FLAT) {
            if (face->flags & FACE_SHADOW) {
                rasterizeS(pixel, L, R);
            } else {
                rasterizeF(pixel, L, R, face->flags & FACE_TEXTURE);
            }
        } else {
            rasterizeG(pixel, L, R, face->flags & FACE_TEXTURE);
        }
    } else {
        if (alphaKill) {
            if (face->flags & FACE_FLAT) {
                rasterizeFTA(pixel, L, R);
            } else {
               rasterizeGTA(pixel, L, R);
            }
        } else {
            if (face->flags & FACE_FLAT) {
                rasterizeFT(pixel, L, R);
            } else {
                rasterizeGT(pixel, L, R);
            }
        }
    }
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

    Edge L(top), R(top);
    rasterize(L, R, face);
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

    Edge L(top), R(top);
    rasterize(L, R, face);
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

        if (p->v.y != top->v.y) {
            if (p->v.y < top->v.y) {
                top = p;
            }
            skip = false;
        }
    }

    if (skip) {
        return; // zero height poly
    }

    Edge L(top), R(top);
    rasterize(L, R, face);
}

void drawGlyph(const Sprite *sprite, int32 x, int32 y)
{
    int32 w = sprite->r - sprite->l;
    int32 h = sprite->b - sprite->t;

    w = (w >> 1) << 1; // make it even

    int32 ix = x + sprite->l;
    int32 iy = y + sprite->t;

#ifdef ROTATE90_MODE
    uint16* pixel = (uint16*)fb + iy;
#else
    uint16* pixel = (uint16*)fb + iy * FRAME_WIDTH + ix;
#endif

    const uint8* glyphData = tiles + (sprite->tile << 16) + 256 * sprite->v + sprite->u;

    while (h--)
    {
        for (int32 i = 0; i < w; i++)
        {
            if (glyphData[i] == 0) continue;
        #ifdef ROTATE90_MODE
            pixel[(FRAME_HEIGHT - (ix + i) - 1) * FRAME_WIDTH] = palette[glyphData[i]];
        #else
            pixel[i] = palette[glyphData[i]];
        #endif
        }

    #ifdef ROTATE90_MODE
        pixel += 1;
    #else
        pixel += FRAME_WIDTH;
    #endif
        glyphData += 256;
    }
}

X_INLINE void faceAddToOTable(Face* face, int32 depth)
{
    ASSERT(depth < OT_SIZE);
    face->next = otFaces[depth];
    otFaces[depth] = face;

    if (depth < otMin) otMin = depth;
    if (depth > otMax) otMax = depth;
}

void faceAddQuad(uint32 flags, const Index* indices, int32 startVertex)
{
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

    if (v1->clip | v2->clip | v3->clip | v4->clip) {
        flags |= FACE_CLIPPED;
    }

    if (v1->g == v2->g && v1->g == v3->g && v1->g == v4->g) {
        flags |= FACE_FLAT;
    }

    Face *f = gFaces + gFacesCount++;

    int32 depth = X_MAX(v1->z, X_MAX(v2->z, X_MAX(v3->z, v4->z))) >> OT_SHIFT;

    // z-bias hack for the shadow plane
    if (flags & FACE_SHADOW) {
        depth = X_MAX(0, depth - 8);
    }

    faceAddToOTable(f, depth);

    f->flags      = flags;
    f->start      = startVertex + indices[0];
    f->indices[0] = 0;
    f->indices[1] = indices[1] - indices[0];
    f->indices[2] = indices[2] - indices[0];
    f->indices[3] = indices[3] - indices[0];
}

void faceAddTriangle(uint32 flags, const Index* indices, int32 startVertex)
{
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

    if (v1->clip | v2->clip | v3->clip) {
        flags |= FACE_CLIPPED;
    }

    if (v1->g == v2->g && v1->g == v3->g) {
        flags |= FACE_FLAT;
    }

    Face *f = gFaces + gFacesCount++;

    int32 depth = X_MAX(v1->z, X_MAX(v2->z, v3->z)) >> OT_SHIFT;
    faceAddToOTable(f, depth);

    f->flags      = flags | FACE_TRIANGLE;
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

#ifdef DEBUG_FACES
    int32 gFacesCountMax, gVerticesCountMax;
#endif

void flush()
{
    if (gFacesCount)
    {
        PROFILE_START();
        for (int32 i = otMax; i >= otMin; i--)
        {
            if (!otFaces[i]) continue;

            Face *face = otFaces[i];
            otFaces[i] = NULL;

            do {
                VertexUV v[16];

                uint32 flags = face->flags;

                if (!(flags & FACE_COLORED)) {
                    const Texture &tex = textures[face->flags & FACE_TEXTURE];
                    tile = tiles + (tex.tile << 16);
                    v[0].t.uv = tex.uv0 & 0xFF00FF00; // TODO preprocess
                    v[1].t.uv = tex.uv1 & 0xFF00FF00; // TODO preprocess
                    v[2].t.uv = tex.uv2 & 0xFF00FF00; // TODO preprocess
                    v[3].t.uv = tex.uv3 & 0xFF00FF00; // TODO preprocess
                    alphaKill = (tex.attribute == 1);
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

                face = face->next;
            } while (face);
        }

        PROFILE_STOP(dbg_flush);

        otMin = OT_SIZE - 1;
        otMax = 0;
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

void clear()
{
    dmaFill((void*)fb, 0, FRAME_WIDTH * FRAME_HEIGHT * 2);
}
