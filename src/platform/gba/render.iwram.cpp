#include "common.h"

#define DIV_TABLE_SIZE 512

uint16 divTable[DIV_TABLE_SIZE];

#ifdef _WIN32
    uint8 fb[WIDTH * HEIGHT * 2];
#else
    uint32 fb = VRAM;
#endif

#define FixedInvS(x) ((x < 0) ? -divTable[abs(x)] : divTable[x])
#define FixedInvU(x) divTable[x]

#if defined(USE_MODE_5) || defined(_WIN32)
    extern uint16 palette[256];
#endif

extern uint8            lightmap[256 * 32];
extern const uint8*     tiles[15];
extern const Texture*   textures;

extern Rect clip;
extern int32 camSinY;
extern int32 camCosY;

uint32 gVerticesCount = 0;
EWRAM_DATA Vertex gVertices[MAX_VERTICES];

int32 gFacesCount = 0;
Face gFaces[MAX_FACES];
Face* gFacesSorted[MAX_FACES];

const uint8* curTile;
uint16 mipMask;

int32 clamp(int32 x, int32 a, int32 b) {
    return x < a ? a : (x > b ? b : x);
}

template <class T>
inline void swap(T &a, T &b) {
    T tmp = a;
    a = b;
    b = tmp;
}

bool checkBackface(const Vertex *a, const Vertex *b, const Vertex *c) {
    return (b->x - a->x) * (c->y - a->y) -
           (c->x - a->x) * (b->y - a->y) <= 0;
}

INLINE void sortVertices(Vertex *&t, Vertex *&m, Vertex *&b) {
    if (t->y > m->y) swap(t, m);
    if (t->y > b->y) swap(t, b);
    if (m->y > b->y) swap(m, b);
}

INLINE void sortVertices(Vertex *&t, Vertex *&m, Vertex *&o, Vertex *&b) {
    if (t->y > m->y) swap(t, m);
    if (o->y > b->y) swap(o, b);
    if (t->y > o->y) swap(t, o);
    if (m->y > b->y) swap(m, b);
    if (m->y > o->y) swap(m, o);
}

int32 classify(const Vertex* v) {
    return (v->x < clip.x0 ? 1 : 0) |
           (v->x > clip.x1 ? 2 : 0) |
           (v->y < clip.y0 ? 4 : 0) |
           (v->y > clip.y1 ? 8 : 0);
}

void transform(int32 vx, int32 vy, int32 vz, int32 vg, int32 x, int32 y, int32 z) {
#ifdef _WIN32
    if (gVerticesCount >= MAX_VERTICES) {
        DebugBreak();
        return;
    }
#endif

    Vertex &res = gVertices[gVerticesCount++];

    int32 px = vx + x;
    int32 pz = vz + z;

    int32 cz = px * camSinY + pz * camCosY;
    cz >>= 16;

    // znear / zfar clip
    if (cz < 32 || cz > MAX_DIST) {
        res.z = -1;
        return;
    }

    int32 py = vy + y;

#if 0
    int32 cx = (px * camCosY - pz * camSinY) >> 16;
    int32 cy = py;

    uint32 czInv = MyDiv(1 << 16, uint32(cz));

    cx = cx * czInv;
    cy = cy * czInv;
#else
    int32 cx = px * camCosY - pz * camSinY;
    int32 cy = py << 16;

    cx = MyDiv(cx, cz);
    cy = MyDiv(cy, cz);
#endif
    cy = cy * FRAME_WIDTH / FRAME_HEIGHT;

    cx = clamp(cx, -2 << 16, 2 << 16);
    cy = clamp(cy, -2 << 16, 2 << 16);

    res.x = ( ( (1 << 16) + cx ) * (FRAME_WIDTH / 2)  ) >> 16;
    res.y = ( ( (1 << 16) + cy ) * (FRAME_HEIGHT / 2) ) >> 16;
    res.z = cz;
    res.clip = classify(&res);

    int32 fog = vg - ((cz * cz) >> 15);
    if (fog < 0) {
        fog = 0;
    }

    res.g = uint32(255 - (fog >> 5)) >> 3;
}

#define FETCH_T()               curTile[(t & 0xFF00) | (t >> 24)]
#define FETCH_T_MIP()           curTile[(t & 0xFF00) | (t >> 24) & mipMask]
#define FETCH_GT()              lightmap[(g & 0x1F00) | FETCH_T()]
#define FETCH_G(palIndex)       lightmap[(g & 0x1F00) | palIndex]
#define FETCH_GT_PAL()          palette[FETCH_GT()]
#define FETCH_G_PAL(palIndex)   palette[FETCH_G(palIndex)]

struct Edge {
    int32  h;
    int32  x;
    int32  g;
    uint32 t;
    int32  dx;
    int32  dg;
    uint32 dt;

    int32   index;
    Vertex* vert[8];

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

        Vertex* v1 = vert[index--];
        Vertex* v2 = vert[index];

        h = v2->y - v1->y;
        x = v1->x << 16;
        g = v1->g << 16;

        if (h > 1) {
            uint32 d = FixedInvU(h);

            dx = d * (v2->x - v1->x);
            dg = d * (v2->g - v1->g);
        }

        return true;
    }

    INLINE bool nextGT() {
        if (index == 0) {
            return false;
        }

        Vertex* v1 = vert[index--];
        Vertex* v2 = vert[index];

        h = v2->y - v1->y;
        x = v1->x << 16;
        g = v1->g << 16;
        t = (v1->u << 24) | (v1->v << 8);

        if (h > 1) {
            uint32 d = FixedInvU(h);

            dx = d * (v2->x - v1->x);
            dg = d * (v2->g - v1->g);

            int32 du = d * (v2->u - v1->u);
            int32 dv = d * (v2->v - v1->v);

            dt = ((du << 8) & 0xFFFF0000) | int16(dv >> 8);
        }

        return true;
    }

    void build(int32 start, int32 count, int32 t, int32 b, int32 incr) {
        vert[index = 0] = gVertices + start + b;

        for (int i = 1; i < count; i++) {
            b = (b + incr) % count;

            Vertex *v = gVertices + start + b;

            if (vert[index]->x != v->x || vert[index]->y != v->y) {
                vert[++index] = v;
            }

            if (b == t) {
                break;
            }
        }
    }
};

INLINE void scanlineG(uint16* buffer, int32 x1, int32 x2, uint8 palIndex, uint32 g, int32 dgdx) {
    #ifdef USE_MODE_5
        uint16* pixel = buffer + x1;
        int32 width = (x2 - x1);

        while (width--)
        {
            *pixel++ = FETCH_G_PAL(palIndex);
            g += dgdx;
        }
    #else
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

            if (p == 0xFFFFFFFF) return;
        }

        if (x2 & 1)
        {
            *pixel = (*pixel & 0xFF00) | FETCH_G(palIndex);
        }
    #endif
}

INLINE void scanlineGT(uint16* buffer, int32 x1, int32 x2, uint32 g, uint32 t, int32 dgdx, uint32 dtdx) {
    #ifdef USE_MODE_5
        uint16* pixel = buffer + x1;
        int32 width = (x2 - x1);

        while (width--)
        {
            *pixel++ = FETCH_GT_PAL();
            t += dtdx;
            g += dgdx;
        }
    #else
        if (x1 & 1)
        {
            uint16 &p = *(uint16*)((uint8*)buffer + x1 - 1);
            p = (p & 0x00FF) | (FETCH_GT() << 8);
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

            if (p == 0xFFFFFFFF) return;
        }

        if (x2 & 1)
        {
            *pixel = (*pixel & 0xFF00) | FETCH_GT();
        }
    #endif
}

void rasterizeG(uint16* buffer, int32 palIndex, Edge &L, Edge &R)
{
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

            if (x2 > x1)
            {
                int32 d = FixedInvU(x2 - x1);

                int32 dgdx = d * ((R.g - L.g) >> 8) >> 16;

                scanlineG(buffer, x1, x2, palIndex, L.g >> 8, dgdx);
            }

            buffer += WIDTH / PIXEL_SIZE;

            L.stepG();
            R.stepG();
        }
    }
}

void rasterizeGT(uint16* buffer, Edge &L, Edge &R)
{
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

            if (x2 > x1)
            {
                uint32 d = FixedInvU(x2 - x1);

                int32  dgdx = d * ((R.g - L.g) >> 8) >> 16;

                uint32 u = d * ((R.t >> 16) - (L.t >> 16));
                uint32 v = d * ((R.t & 0xFFFF) - (L.t & 0xFFFF));
                uint32 dtdx = (u & 0xFFFF0000) | (v >> 16);

                scanlineGT(buffer, x1, x2, L.g >> 8, L.t, dgdx, dtdx);
            }

            buffer += WIDTH / PIXEL_SIZE;

            L.stepGT();
            R.stepGT();
        }
    }
}

void drawTriangle(uint16 flags, int32 start, const int8* indices)
{
    Vertex *v1, *v2, *v3;

    bool clipped = indices[0] == indices[1];

    if (clipped) {
        v1 = gVertices + start;
        v2 = v1 + 1;
        v3 = v1 + 2;
    } else {
        v1 = gVertices + start;
        v2 = v1 + indices[1];
        v3 = v1 + indices[2];
    }

    uint16 palIndex = flags & FACE_TEXTURE;

    if (!(flags & FACE_COLORED)) {
        const Texture &tex = textures[palIndex];
        palIndex = 0xFFFF;
        curTile = tiles[tex.tile];
        if (!clipped) {
            v1->u = tex.x0;
            v1->v = tex.y0;
            v2->u = tex.x1;
            v2->v = tex.y1;
            v3->u = tex.x2;
            v3->v = tex.y2;
        }
    }

    sortVertices(v1, v2, v3);

    int32 temp = (v2->y - v1->y) * FixedInvU(v3->y - v1->y);

    int32 longest = ((temp * (v3->x - v1->x)) >> 16) + (v1->x - v2->x);
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

    if (palIndex != 0xFFFF) {
        rasterizeG((uint16*)fb + v1->y * (WIDTH / PIXEL_SIZE), palIndex, L, R);
    } else {
        rasterizeGT((uint16*)fb + v1->y * (WIDTH / PIXEL_SIZE), L, R);
    }
}

void drawQuad(uint16 flags, int32 start, const int8* indices) {
    Vertex *v1, *v2, *v3, *v4;
    bool clipped = indices[0] == indices[1];

    if (clipped) {
        v1 = gVertices + start;
        v2 = v1 + 1;
        v3 = v1 + 2;
        v4 = v1 + 3;
    } else {
        v1 = gVertices + start;
        v2 = v1 + indices[1];
        v3 = v1 + indices[2];
        v4 = v1 + indices[3];
    }

    uint16 palIndex = flags & FACE_TEXTURE;

    if (!(flags & FACE_COLORED)) {
        const Texture &tex = textures[palIndex];
        palIndex = 0xFFFF;
        curTile = tiles[tex.tile];
        if (!clipped) {
            v1->u = tex.x0;
            v1->v = tex.y0;
            v2->u = tex.x1;
            v2->v = tex.y1;
            v3->u = tex.x2;
            v3->v = tex.y2;
            v4->u = tex.x3;
            v4->v = tex.y3;
        }
    }

    sortVertices(v1, v2, v3, v4);

    int32 temp = (v2->y - v1->y) * FixedInvU(v4->y - v1->y);

    int32 longest = ((temp * (v4->x - v1->x)) >> 16) + (v1->x - v2->x);
    if (longest == 0)
    {
        return;
    }

    Edge L, R;

    if (checkBackface(v1, v4, v2) == checkBackface(v1, v4, v3))
    {
        if (longest < 0)
        {
            L.vert[0] = v4;
            L.vert[1] = v1;
            L.index   = 1;
            R.vert[0] = v4;
            R.vert[1] = v3;
            R.vert[2] = v2;
            R.vert[3] = v1;
            R.index   = 3;
        }
        else
        {
            R.vert[0] = v4;
            R.vert[1] = v1;
            R.index   = 1;
            L.vert[0] = v4;
            L.vert[1] = v3;
            L.vert[2] = v2;
            L.vert[3] = v1;
            L.index   = 3;
        }
    }
    else
    {
        R.vert[0] = v4;
        R.vert[1] = v3;
        R.vert[2] = v1;
        R.index   = 2;
        L.vert[0] = v4;
        L.vert[1] = v2;
        L.vert[2] = v1;
        L.index   = 2;

        if (longest < 0)
        {
            swap(L.vert[1], R.vert[1]);
        }
    }

    if (palIndex != 0xFFFF) {
        rasterizeG((uint16*)fb + v1->y * (WIDTH / PIXEL_SIZE), palIndex, L, R);
    } else {
        rasterizeGT((uint16*)fb + v1->y * (WIDTH / PIXEL_SIZE), L, R);
    }
}

void drawPoly(uint16 flags, int32 start, int32 count) {
    uint16 palIndex = flags & FACE_TEXTURE;
    
    if (!(flags & FACE_COLORED)) {
        const Texture &tex = textures[palIndex];
        palIndex = 0xFFFF;
        curTile = tiles[tex.tile];
    }

    int32 minY =  0x7FFF;
    int32 maxY = -0x7FFF;
    int32 t = start, b = start;

    for (int i = 0; i < count; i++) {
        Vertex *v = gVertices + start + i;

        if (v->y < minY) {
            minY = v->y;
            t = i;
        }

        if (v->y > maxY) {
            maxY = v->y;
            b = i;
        }
    }

    Edge L, R;

    L.build(start, count, t, b, count + 1);
    R.build(start, count, t, b, count - 1);

    Vertex *v1 = gVertices + start + t;

    if (palIndex != 0xFFFF) {
        rasterizeG((uint16*)fb + v1->y * (WIDTH / PIXEL_SIZE), palIndex, L, R);
    } else {
        rasterizeGT((uint16*)fb + v1->y * (WIDTH / PIXEL_SIZE), L, R);
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

    const uint8* glyphData = tiles[sprite->tile] + 256 * sprite->v + sprite->u;

    while (h--)
    {
    #ifdef USE_MODE_5
        for (int i = 0; i < w; i++) {
            if (glyphData[i] == 0) continue;

            ptr[i] = palette[glyphData[i]];
        }
    #else
        const uint8* p = glyphData;

        for (int i = 0; i < (w / 2); i++) {

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

void faceAddPolyClip(uint16 flags, Vertex** poly, int32 pCount) {
    #define LERP(a,b,t) (b + ((a - b) * t >> 16))

    #define CLIP_AXIS(x, y, edge, output) {\
        uint32 t = MyDiv((edge - b->x) << 16, a->x - b->x);\
        Vertex* v = output + count++;\
        v->x = edge;\
        v->y = LERP(a->y, b->y, t);\
        v->z = LERP(a->z, b->z, t);\
        v->u = LERP(a->u, b->u, t);\
        v->v = LERP(a->v, b->v, t);\
        v->g = LERP(a->g, b->g, t);\
    }

    #define CLIP_VERTEX(x, y, x0, x1, input, output) {\
        const Vertex *a, *b = input[pCount - 1];\
        for (int32 i = 0; i < pCount; i++) {\
            a = b;\
            b = input[i];\
            if (a->x < x0) {\
                if (b->x < x0) continue;\
                CLIP_AXIS(x, y, x0, output);\
            } else if (a->x > x1) {\
                if (b->x > x1) continue;\
                CLIP_AXIS(x, y, x1, output);\
            }\
            if (b->x < x0) {\
                CLIP_AXIS(x, y, x0, output);\
            } else if (b->x > x1) {\
                CLIP_AXIS(x, y, x1, output);\
            } else {\
                output[count++] = *b;\
            }\
        }\
        if (count < 3) return;\
    }

    if (!(flags & FACE_COLORED)) {
        const Texture &tex = textures[flags & FACE_TEXTURE];
        curTile = tiles[tex.tile];
        poly[0]->u = tex.x0;
        poly[0]->v = tex.y0;
        poly[1]->u = tex.x1;
        poly[1]->v = tex.y1;
        poly[2]->u = tex.x2;
        poly[2]->v = tex.y2;
        if (pCount == 4) {
            poly[3]->u = tex.x3;
            poly[3]->v = tex.y3;
        }
    }

    Vertex tmp[8];
    int32 count = 0;

// clip x
    int32 x0 = clip.x0;
    int32 x1 = clip.x1;
    CLIP_VERTEX(x, y, x0, x1, poly, tmp);

    pCount = count;
    count = 0;

    Vertex* output = gVertices + gVerticesCount;

// clip y
    int32 y0 = clip.y0;
    int32 y1 = clip.y1;
    CLIP_VERTEX(y, x, y0, y1, &tmp, output);

    Face *f = gFaces + gFacesCount;
    gFacesSorted[gFacesCount++] = f;
    f->flags      = flags;
    f->start      = gVerticesCount;
    f->indices[0] = count;
    f->indices[1] = count;

    if (count == 3) {
        f->flags |= FACE_TRIANGLE;
        f->depth = (output[0].z + output[1].z + output[2].z) / 3;
    } else if (count == 4) {
        f->depth = (output[0].z + output[1].z + output[2].z + output[3].z) >> 2;
    } else {
        int32 depth = output[0].z;
        for (int32 i = 1; i < count; i++) {
            depth = (depth + output[i].z) >> 1;
        }
        f->depth = depth;
    }

    gVerticesCount += count;
}

void faceAddQuad(uint16 flags, const Index* indices, int32 startVertex) {
#ifdef _WIN32
    if (gFacesCount >= MAX_FACES) {
        DebugBreak();
    }
#endif
    Vertex* v1 = gVertices + startVertex + indices[0];
    Vertex* v2 = gVertices + startVertex + indices[1];
    Vertex* v3 = gVertices + startVertex + indices[2];
    Vertex* v4 = gVertices + startVertex + indices[3];

    if (v1->z < 0 || v2->z < 0 || v3->z < 0 || v4->z < 0)
        return;

    if (checkBackface(v1, v2, v3))
        return;

    if (v1->clip & v2->clip & v3->clip & v4->clip)
        return;

    if (v1->clip | v2->clip | v3->clip | v4->clip) {
        Vertex* poly[4] = { v1, v2, v3, v4 };
        faceAddPolyClip(flags, poly, 4);
    } else {
        Face *f = gFaces + gFacesCount;
        gFacesSorted[gFacesCount++] = f;
        f->flags      = flags;
        f->depth      = (v1->z + v2->z + v3->z + v4->z) >> 2;
        f->start      = startVertex + indices[0];
        f->indices[0] = 0;
        f->indices[1] = indices[1] - indices[0];
        f->indices[2] = indices[2] - indices[0];
        f->indices[3] = indices[3] - indices[0];
    }
}

void faceAddTriangle(uint16 flags, const Index* indices, int32 startVertex) {
#ifdef _WIN32
    if (gFacesCount >= MAX_FACES) {
        DebugBreak();
    }
#endif
    Vertex* v1 = gVertices + startVertex + indices[0];
    Vertex* v2 = gVertices + startVertex + indices[1];
    Vertex* v3 = gVertices + startVertex + indices[2];

    if (v1->z < 0 || v2->z < 0 || v3->z < 0)
        return;

    if (checkBackface(v1, v2, v3))
        return;

    if (v1->clip & v2->clip & v3->clip)
        return;

    if (v1->clip | v2->clip | v3->clip) {
        Vertex* poly[3] = { v1, v2, v3 };
        faceAddPolyClip(flags, poly, 3);
    } else {
        Face *f = gFaces + gFacesCount;
        gFacesSorted[gFacesCount++] = f;
        f->flags      = flags | FACE_TRIANGLE;
        f->depth      = (v1->z + v2->z + v3->z) / 3;
        f->start      = startVertex + indices[0];
        f->indices[0] = 0;
        f->indices[1] = indices[1] - indices[0];
        f->indices[2] = indices[2] - indices[0];
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

//int32 gFacesCountMax, gVerticesCountMax;

void flush() {
    if (gFacesCount) {
        faceSort(gFacesSorted, 0, gFacesCount - 1);

        //const uint16 mips[] = { 0xFFFF, 0xFEFE, 0xFCFC, 0xF8F8 };

        for (int32 i = 0; i < gFacesCount; i++) {
            const Face *f = gFacesSorted[i];

            // TODO
            //mipMask = mips[MIN(3, f.depth / 2048)];

            if (f->flags & FACE_TRIANGLE) {
                drawTriangle(f->flags, f->start, f->indices);
            } else {
                if (f->indices[0] == f->indices[1] /* && f.indices[0] > 4 */) {
                    drawPoly(f->flags, f->start, f->indices[0]);
                } else {
                    drawQuad(f->flags, f->start, f->indices);
                }
            }
        }
    }

    //if (gFacesCount > gFacesCountMax) gFacesCountMax = gFacesCount;
    //if (gVerticesCount > gVerticesCountMax) gVerticesCountMax = gVerticesCount;
    //printf("%d %d\n", gFacesCountMax, gVerticesCountMax);

    gVerticesCount = 0;
    gFacesCount = 0;
}

void initRender() {
    divTable[0] = 0;
    for (uint32 i = 1; i < DIV_TABLE_SIZE; i++) {
        divTable[i] = MyDiv(1 << 16, i);
    }
}

void clear() {
    uint32* dst = (uint32*)fb;

    #ifdef USE_MODE_5
        uint32* end = dst + (WIDTH * HEIGHT >> 1);
    #else
        uint32* end = dst + (WIDTH * HEIGHT >> 2);
    #endif

    while (dst < end) {
        *dst++ = 0;
        *dst++ = 0;
        *dst++ = 0;
        *dst++ = 0;
        *dst++ = 0;
        *dst++ = 0;
        *dst++ = 0;
        *dst++ = 0;
        *dst++ = 0;
        *dst++ = 0;
        *dst++ = 0;
        *dst++ = 0;
        *dst++ = 0;
        *dst++ = 0;
        *dst++ = 0;
        *dst++ = 0;
    }
}