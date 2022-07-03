#include "common.h"

//#define DEBUG_CLIPPING
//#define CHECK_LIMITS

struct Vertex
{
    int32 x, y, z; // for rooms z = (depth << CLIP_SHIFT) | ClipFlags
};

uint16* gPalette; // offset to the default or underwater PLUTs

extern Level level;

int32 gVerticesCount;
int32 gFacesCount;

Vertex gVertices[MAX_VERTICES];

Face* gFaces; // MAX_FACES
Face* gFacesBase;

struct ListOT {
    Face* head;
    Face* tail;
};

ListOT gOT[OT_SIZE];

struct ViewportRel {
    int32 minXY;
    int32 maxXY;
};

ViewportRel viewportRel;

#define SHADOW_OPACITY  3   // 50%
#define MIP_DIST        (1024 * 5)

extern Item screenItem;

#define FACE_CCW        (1 << 31)
#define FACE_MIP_SHIFT  11
#define FACE_TEXTURE    0x07FF

#define DUP16(value) value | (value << 16)

enum ShadeValue
{
    SHADE_SHADOW = PPMPC_1S_CFBD | PPMPC_SF_8 | PPMPC_2S_PDC | (SHADOW_OPACITY << PPMPC_MF_SHIFT), // transparent
    SHADE_1  = DUP16( PPMPC_MF_1 | PPMPC_SF_8 | PPMPC_2D_2 ), // 1/16
    SHADE_2  = DUP16( PPMPC_MF_2 | PPMPC_SF_8 | PPMPC_2D_2 ), // 2/16
    SHADE_3  = DUP16( PPMPC_MF_3 | PPMPC_SF_8 | PPMPC_2D_2 ), // 3/16
    SHADE_4  = DUP16( PPMPC_MF_4 | PPMPC_SF_8 | PPMPC_2D_2 ), // 4/16
    SHADE_5  = DUP16( PPMPC_MF_5 | PPMPC_SF_8 | PPMPC_2D_2 ), // 5/16
    SHADE_6  = DUP16( PPMPC_MF_6 | PPMPC_SF_8 | PPMPC_2D_2 ), // 6/16
    SHADE_7  = DUP16( PPMPC_MF_7 | PPMPC_SF_8 | PPMPC_2D_2 ), // 7/16
    SHADE_8  = DUP16( PPMPC_MF_8 | PPMPC_SF_8 | PPMPC_2D_2 ), // 8/16
    SHADE_9  = DUP16( PPMPC_MF_1 | PPMPC_SF_8 | PPMPC_2D_2 | PPMPC_2S_PDC ), // 9/16
    SHADE_10 = DUP16( PPMPC_MF_2 | PPMPC_SF_8 | PPMPC_2D_2 | PPMPC_2S_PDC ), // 10/16
    SHADE_11 = DUP16( PPMPC_MF_3 | PPMPC_SF_8 | PPMPC_2D_2 | PPMPC_2S_PDC ), // 11/16
    SHADE_12 = DUP16( PPMPC_MF_4 | PPMPC_SF_8 | PPMPC_2D_2 | PPMPC_2S_PDC ), // 12/16
    SHADE_13 = DUP16( PPMPC_MF_5 | PPMPC_SF_8 | PPMPC_2D_2 | PPMPC_2S_PDC ), // 13/16
    SHADE_14 = DUP16( PPMPC_MF_6 | PPMPC_SF_8 | PPMPC_2D_2 | PPMPC_2S_PDC ), // 14/16
    SHADE_15 = DUP16( PPMPC_MF_7 | PPMPC_SF_8 | PPMPC_2D_2 | PPMPC_2S_PDC ), // 15/16
    SHADE_16 = DUP16( PPMPC_MF_8 | PPMPC_SF_8 ), // 1
    SHADE_17 = DUP16( PPMPC_MF_1 | PPMPC_SF_16 | PPMPC_2S_PDC ), // 1 + 1/16
    SHADE_18 = DUP16( PPMPC_MF_2 | PPMPC_SF_16 | PPMPC_2S_PDC ), // 1 + 2/16
    SHADE_19 = DUP16( PPMPC_MF_3 | PPMPC_SF_16 | PPMPC_2S_PDC ), // 1 + 3/16
    SHADE_20 = DUP16( PPMPC_MF_4 | PPMPC_SF_16 | PPMPC_2S_PDC ), // 1 + 4/16
    SHADE_21 = DUP16( PPMPC_MF_5 | PPMPC_SF_16 | PPMPC_2S_PDC ), // 1 + 5/16
    SHADE_22 = DUP16( PPMPC_MF_6 | PPMPC_SF_16 | PPMPC_2S_PDC ), // 1 + 6/16
    SHADE_23 = DUP16( PPMPC_MF_7 | PPMPC_SF_16 | PPMPC_2S_PDC ), // 1 + 7/16
    SHADE_24 = DUP16( PPMPC_MF_8 | PPMPC_SF_16 | PPMPC_2S_PDC )  // 1 + 8/16
};

extern "C" const uint32 shadeTable[32] = {
    SHADE_24,
    SHADE_24,
    SHADE_23,
    SHADE_23,
    SHADE_22,
    SHADE_22,
    SHADE_21,
    SHADE_21,
    SHADE_20,
    SHADE_20,
    SHADE_19,
    SHADE_19,
    SHADE_18,
    SHADE_18,
    SHADE_17,
    SHADE_17,
    SHADE_16,
    SHADE_15,
    SHADE_14,
    SHADE_13,
    SHADE_12,
    SHADE_11,
    SHADE_10,
    SHADE_9,
    SHADE_8,
    SHADE_7,
    SHADE_6,
    SHADE_5,
    SHADE_4,
    SHADE_3,
    SHADE_2,
    SHADE_1
};

extern "C" const MeshQuad gShadowQuads[] = {
    0, (0 | (1 << 8) | (2 << 16) | (7 << 24)),
    0, (7 | (2 << 8) | (3 << 16) | (6 << 24)),
    0, (6 | (3 << 8) | (4 << 16) | (5 << 24)),
};

void renderInit()
{
    gPalette = (uint16*)RAM_TEX;
    gFaces = (Face*)RAM_CEL;
    gFacesBase = gFaces;

    Face* face = gFaces;
    for (int32 i = 0; i < MAX_FACES; i++, face++)
    {
        // set preamble for CCB_CCBPRE flag
        face->ccb_PRE0 = PRE0_BGND | PRE0_LINEAR | PRE0_BPP_16;
        face->ccb_PRE1 = PRE1_TLLSB_PDC0;
    }
}

void renderFree()
{
}

void renderLevelInit()
{
}

void renderLevelFree()
{
}

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
    uint32 paletteOffset = *(uint32*)RAM_TEX;
    gPalette = (uint16*)(intptr_t(RAM_TEX) + paletteOffset + index * level.tilesCount * sizeof(uint16) * 16);
}

X_INLINE int32 cross(const Vertex *a, const Vertex *b, const Vertex *c)
{
    return (b->x - a->x) * (c->y - a->y) -
           (c->x - a->x) * (b->y - a->y);
}

enum ClipFlags {
    CLIP_SHIFT  = 8,
    CLIP_MASK   = (1 << CLIP_SHIFT) - 1,
    CLIP_LEFT   = 1 << 0,
    CLIP_RIGHT  = 1 << 1,
    CLIP_TOP    = 1 << 2,
    CLIP_BOTTOM = 1 << 3,
    CLIP_FAR    = 1 << 4,
    CLIP_NEAR   = 1 << 5
};

X_INLINE Face* faceAdd(int32 depth)
{
    Face* face = gFacesBase++;

    if (gOT[depth].head) {
        face->ccb_NextPtr = gOT[depth].head;
    } else {
        gOT[depth].tail = face;
    }

    gOT[depth].head = face;

    return face;
}

X_INLINE void ccbSetTexture(uint32 flags, Face* face, const Texture* texture)
{
    face->ccb_Flags =
        CCB_NPABS  |
        CCB_SPABS  |
        CCB_PPABS  |
        CCB_LDSIZE |
        CCB_LDPRS  |
        CCB_LDPPMP |
        CCB_LDPLUT |
        CCB_YOXY   |
        CCB_ACSC   | CCB_ALSC |
        CCB_ACW    | CCB_ACCW |
        CCB_ACE    |
        CCB_NOBLK  |
        (flags >> (8 + FACE_MIP_SHIFT + FACE_MIP_SHIFT) << 5); // set CCB_BGND (0x20 == 1 << 5)

    face->ccb_SourcePtr = (CelData*)texture->data;
    face->ccb_PLUTPtr = (uint8*)gPalette + (texture->shift >> 16);
}

X_INLINE void ccbSetColor(uint32 flags, Face* face)
{
    face->ccb_Flags =
        CCB_NPABS  |
        CCB_SPABS  |
        CCB_PPABS  |
        CCB_LDSIZE |
        CCB_LDPRS  |
        CCB_LDPPMP |
        CCB_CCBPRE | // use the preamble words set in renderInit
        CCB_YOXY   |
        CCB_ACSC   | CCB_ALSC |
        CCB_ACW    | CCB_ACCW |
        CCB_ACE    |
        CCB_NOBLK  |
        CCB_BGND;

    face->ccb_SourcePtr = (CelData*)&gPalette[flags & 0xFF];
}

#ifdef USE_ASM
    #define unpackRoom                  unpackRoom_asm                  // -53%
    #define unpackMesh                  unpackMesh_asm                  // -48%
    #define projectVertices             projectVertices_asm             // -18%
    #define faceAddRoomQuads            faceAddRoomQuads_asm            // -46%
    #define faceAddRoomTriangles        faceAddRoomTriangles_asm        // -30%
    #define faceAddMeshQuads            faceAddMeshQuads_asm            // -36%
    #define faceAddMeshTriangles        faceAddMeshTriangles_asm        // -38%
    #define faceAddMeshQuadsFlat        faceAddMeshQuadsFlat_asm        // -28%
    #define faceAddMeshTrianglesFlat    faceAddMeshTrianglesFlat_asm    // -35%

    extern "C" {
        void unpackRoom_asm(const RoomVertex* vertices, int32 vCount);
        void unpackMesh_asm(const MeshVertex* vertices, int32 vCount);
        void projectVertices_asm(int32 vCount);
        void faceAddRoomQuads_asm(const RoomQuad* polys, int32 count);
        void faceAddRoomTriangles_asm(const RoomTriangle* polys, int32 count);
        void faceAddMeshQuads_asm(const MeshQuad* polys, int32 count, uint32 shade);
        void faceAddMeshTriangles_asm(const MeshTriangle* polys, int32 count, uint32 shade);
        void faceAddMeshQuadsFlat_asm(const MeshQuad* polys, int32 count, uint32 shade);
        void faceAddMeshTrianglesFlat_asm(const MeshTriangle* polys, int32 count, uint32 shade);
    }
#else
    #define unpackRoom                  unpackRoom_c
    #define unpackMesh                  unpackMesh_c
    #define projectVertices             projectVertices_c
    #define faceAddRoomQuads            faceAddRoomQuads_c
    #define faceAddRoomTriangles        faceAddRoomTriangles_c
    #define faceAddMeshQuads            faceAddMeshQuads_c
    #define faceAddMeshTriangles        faceAddMeshTriangles_c
    #define faceAddMeshQuadsFlat        faceAddMeshQuadsFlat_c
    #define faceAddMeshTrianglesFlat    faceAddMeshTrianglesFlat_c

void unpackRoom_c(const RoomVertex* vertices, int32 vCount)
{
    Vertex* res = gVertices;

    uint32 *v32 = (uint32*)vertices;

    for (int32 i = 0; i < vCount; i += 4)
    {
        uint32 n0 = *v32++;
        uint32 n1 = *v32++;

        res->x = (n0 << 12) & 0x1F000;
        res->y = (n0 << 5) & 0xFC00;
        res->z = (n0 << 1) & 0x1F000;
        res++;

        res->x = (n0 >> 4) & 0x1F000;
        res->y = (n0 >> 11) & 0xFC00;
        res->z = (n0 >> 15) & 0x1F000;
        res++;

        res->x = (n1 << 12) & 0x1F000;
        res->y = (n1 << 5) & 0xFC00;
        res->z = (n1 << 1) & 0x1F000;
        res++;

        res->x = (n1 >> 4) & 0x1F000;
        res->y = (n1 >> 11) & 0xFC00;
        res->z = (n1 >> 15) & 0x1F000;
        res++;
    }
}

void unpackMesh_c(const MeshVertex* vertices, int32 vCount)
{
    uint32 *v32 = (uint32*)vertices;

    Vertex* res = gVertices;
    for (int32 i = 0; i < vCount; i += 2)
    {
        uint32 n0 = *v32++;
        uint32 n1 = *v32++;
        uint32 n2 = *v32++;

        res->x = int16(n0 >> 16);
        res->y = int16(n0);
        res->z = int16(n1 >> 16);
        res++;

        res->x = int16(n1);
        res->y = int16(n2 >> 16);
        res->z = int16(n2);
        res++;
    }
}

void projectVertices_c(int32 vCount)
{
    Vertex* v = gVertices;
    Vertex* last = gVertices + vCount;

    Matrix &m = matrixGet();
    int32 mx = m.e03 >> FIXED_SHIFT;
    int32 my = m.e13 >> FIXED_SHIFT;
    int32 mz = m.e23 >> FIXED_SHIFT;

    MulManyVec3Mat33_F16((vec3f16*)v, (vec3f16*)v, *(mat33f16*)&m, vCount);

    do
    {
        int32 x = v->x + mx;
        int32 y = v->y + my;
        int32 z = v->z + mz;

        int32 clip = 0;

        if (z <= (VIEW_MIN_F >> FIXED_SHIFT)) {
            z = (VIEW_MIN_F >> FIXED_SHIFT);
            clip = CLIP_NEAR;
        } else if (z >= (VIEW_MAX_F >> FIXED_SHIFT)) {
            z = (VIEW_MAX_F >> FIXED_SHIFT);
            clip = CLIP_FAR;
        }

        PERSPECTIVE(x, y, z);

    // Y is shifted left by 16
        y <<= 16;

        int32 vMinXY = viewportRel.minXY;
        int32 vMaxXY = viewportRel.maxXY;

        if (x < (vMinXY >> 16)) clip |= CLIP_LEFT;
        if (y < (vMinXY << 16)) clip |= CLIP_TOP;
        if (x > (vMaxXY >> 16)) clip |= CLIP_RIGHT;
        if (y > (vMaxXY << 16)) clip |= CLIP_BOTTOM;

        v->x = x;
        v->y = y >> 16;
        v->z = (z << CLIP_SHIFT) | clip;
        v++;
    } while (v < last);
}

X_INLINE void ccbMap4(Face* f, const Vertex* v0, const Vertex* v1, const Vertex* v2, const Vertex* v3, uint32 shift)
{
    int32 x1 = v1->x;
    int32 y1 = v1->y;
    int32 x3 = v3->x;
    int32 y3 = v3->y;
    int32 x2 = v2->x;
    int32 y2 = v2->y;
    int32 x0 = v0->x;
    int32 y0 = v0->y;

    uint32 ws = shift & 0xFF;
    uint32 hs = (shift >> 8) & 0xFF;

    int32 hdx0 = (x1 - x0) << ws;
    int32 hdy0 = (y1 - y0) << ws;
    int32 hdx1 = (x2 - x3) << ws;
    int32 hdy1 = (y2 - y3) << ws;
    int32 vdx0 = (x3 - x0) << hs;
    int32 vdy0 = (y3 - y0) << hs;

    hs = 16 - hs;
    int32 hddx = (hdx1 - hdx0) >> hs;
    int32 hddy = (hdy1 - hdy0) >> hs;

    f->ccb_XPos = (x0 + (FRAME_WIDTH  >> 1)) << 16;
    f->ccb_YPos = (y0 + (FRAME_HEIGHT >> 1)) << 16;
    f->ccb_HDX = hdx0;
    f->ccb_HDY = hdy0;
    f->ccb_VDX = vdx0;
    f->ccb_VDY = vdy0;
    f->ccb_HDDX = hddx;
    f->ccb_HDDY = hddy;

#ifdef DEBUG_CLIPPING
    f->ccb_PIXC = SHADE_SHADOW;
#endif
}

X_INLINE void ccbMap3(Face* f, const Vertex* v0, const Vertex* v1, const Vertex* v2, uint32 shift)
{
    int32 x0 = v0->x;
    int32 y0 = v0->y;
    int32 x1 = v1->x;
    int32 y1 = v1->y;
    int32 x2 = v2->x;
    int32 y2 = v2->y;

    uint32 ws = shift & 0xFF;
    uint32 hs = (shift >> 8) & 0xFF;

    int32 hdx0 = (x1 - x0) << ws;
    int32 hdy0 = (y1 - y0) << ws;
    int32 vdx0 = (x2 - x0) << hs;
    int32 vdy0 = (y2 - y0) << hs;

    f->ccb_XPos = (x0 + (FRAME_WIDTH  >> 1)) << 16;
    f->ccb_YPos = (y0 + (FRAME_HEIGHT >> 1)) << 16;
    f->ccb_HDX = hdx0;
    f->ccb_HDY = hdy0;
    f->ccb_VDX = vdx0;
    f->ccb_VDY = vdy0;

    hs = 16 - hs;
    f->ccb_HDDX = -hdx0 >> hs;
    f->ccb_HDDY = -hdy0 >> hs;

#ifdef DEBUG_CLIPPING
    f->ccb_PIXC = SHADE_SHADOW;
#endif
}

#define DEPTH_Q_AVG(z0,z1,z2,z3) ((z0 + z1 + z2 + z3) >> (2 + CLIP_SHIFT + OT_SHIFT))
#define DEPTH_T_AVG(z0,z1,z2)    ((z0 + z1 + z2 + z2) >> (2 + CLIP_SHIFT + OT_SHIFT))
#define DEPTH_Q_MAX(z0,z1,z2,z3) (X_MAX(z0, X_MAX(z1, X_MAX(z2, z3))) >> (CLIP_SHIFT + OT_SHIFT))
#define DEPTH_T_MAX(z0,z1,z2)    (X_MAX(z0, X_MAX(z1, z2)) >> (CLIP_SHIFT + OT_SHIFT))

void faceAddRoomQuads_c(const RoomQuad* polys, int32 count)
{
    for (int32 i = 0; i < count; i++, polys++)
    {
        uint32 flags = polys->flags;
        uint32* indices = (uint32*)polys->indices;

        uint32 i01 = indices[0];
        uint32 i23 = indices[1];

        uint32 i0 = (i01 >> 16);
        uint32 i1 = (i01 & 0xFFFF);
        uint32 i2 = (i23 >> 16);
        uint32 i3 = (i23 & 0xFFFF);

        const Vertex* v0 = (Vertex*)((uint8*)gVertices + i0);
        const Vertex* v1 = (Vertex*)((uint8*)gVertices + i1);
        const Vertex* v2 = (Vertex*)((uint8*)gVertices + i2);
        const Vertex* v3 = (Vertex*)((uint8*)gVertices + i3);

        uint32 c0 = v0->z;
        uint32 c1 = v1->z;
        uint32 c2 = v2->z;
        uint32 c3 = v3->z;

        if ((c0 & c1 & c2 & c3) & CLIP_MASK)
            continue;

        int32 depth = DEPTH_Q_MAX(c0, c1, c2, c3);

        if (cross(v0, v1, v3) <= 0)
            continue;

        Face* f = faceAdd(depth);

        int32 fog = X_MAX(0, (depth - (FOG_MIN >> OT_SHIFT)) >> 1);
        uint32 intensity = X_MIN(255, fog + ((flags >> (FACE_MIP_SHIFT + FACE_MIP_SHIFT)) & 0xFF));

        f->ccb_PIXC = shadeTable[intensity >> 3];

        uint32 texIndex = flags;
        if (depth > (MIP_DIST >> OT_SHIFT)) {
            texIndex >>= FACE_MIP_SHIFT;
        }
        const Texture* texture = level.textures + (texIndex & FACE_TEXTURE);
        ccbSetTexture(flags, f, texture);

        ccbMap4(f, v0, v1, v2, v3, texture->shift);
    }
}

void faceAddRoomTriangles_c(const RoomTriangle* polys, int32 count)
{
    for (int32 i = 0; i < count; i++, polys++)
    {
        uint32 flags = polys->flags;
        uint32* indices = (uint32*)polys->indices;

        uint32 i01 = indices[0];
        uint32 i23 = indices[1];

        uint32 i0 = (i01 >> 16);
        uint32 i1 = (i01 & 0xFFFF);
        uint32 i2 = (i23 >> 16);

        const Vertex* v0 = (Vertex*)((uint8*)gVertices + i0);
        const Vertex* v1 = (Vertex*)((uint8*)gVertices + i1);
        const Vertex* v2 = (Vertex*)((uint8*)gVertices + i2);

        uint32 c0 = v0->z;
        uint32 c1 = v1->z;
        uint32 c2 = v2->z;

        if ((c0 & c1 & c2) & CLIP_MASK)
            continue;

        int32 depth = DEPTH_T_MAX(c0, c1, c2);

        if (cross(v0, v1, v2) <= 0)
            continue;

        Face* f = faceAdd(depth);
    
        uint32 intensity = (flags >> (FACE_MIP_SHIFT + FACE_MIP_SHIFT)) & 0xFF;
        if (depth > (FOG_MIN >> OT_SHIFT)) {
            intensity += (depth - (FOG_MIN >> OT_SHIFT)) >> 1;
            intensity = X_MIN(intensity, 255);
        }

        f->ccb_PIXC = shadeTable[intensity >> 3];

        uint32 texIndex = flags;
        if (depth > (MIP_DIST >> OT_SHIFT)) {
            texIndex >>= FACE_MIP_SHIFT;
        }
        const Texture* texture = level.textures + (texIndex & FACE_TEXTURE);
        ccbSetTexture(flags, f, texture);

        ccbMap3(f, v0, v1, v2, texture->shift);
    }
}

void faceAddMeshQuads_c(const MeshQuad* polys, int32 count, uint32 shade)
{
    for (int32 i = 0; i < count; i++, polys++)
    {
        uint32 indices = polys->indices;
        uint32 flags = polys->flags;

        uint32 i0 = indices & 0xFF; indices >>= 8;
        uint32 i1 = indices & 0xFF; indices >>= 8;
        uint32 i2 = indices & 0xFF; indices >>= 8;
        uint32 i3 = indices;

        const Vertex* v0 = gVertices + i0;
        const Vertex* v1 = gVertices + i1;
        const Vertex* v2 = gVertices + i2;
        const Vertex* v3 = gVertices + i3;

        uint32 c0 = v0->z;
        uint32 c1 = v1->z;
        uint32 c2 = v2->z;
        uint32 c3 = v3->z;

        if ((c0 & c1 & c2 & c3) & CLIP_MASK)
            continue;

        int32 depth = DEPTH_Q_AVG(c0, c1, c2, c3);

        if ((cross(v0, v1, v3) ^ flags) & FACE_CCW)
            continue;

        Face* f = faceAdd(depth);
        f->ccb_PIXC = shade;

        const Texture* texture = level.textures + (flags & FACE_TEXTURE);
        ccbSetTexture(flags, f, texture);

        ccbMap4(f, v0, v1, v2, v3, texture->shift);
    }
}

void faceAddMeshTriangles_c(const MeshTriangle* polys, int32 count, uint32 shade)
{
    for (int32 i = 0; i < count; i++, polys++)
    {
        uint32 indices = polys->indices;
        uint32 flags = polys->flags;

        uint32 i0 = indices & 0xFF; indices >>= 8;
        uint32 i1 = indices & 0xFF; indices >>= 8;
        uint32 i2 = indices;

        const Vertex* v0 = gVertices + i0;
        const Vertex* v1 = gVertices + i1;
        const Vertex* v2 = gVertices + i2;

        uint32 c0 = v0->z;
        uint32 c1 = v1->z;
        uint32 c2 = v2->z;

        if ((c0 & c1 & c2) & CLIP_MASK)
            continue;

        int32 depth = DEPTH_T_AVG(v0->z, v1->z, v2->z);

        if (cross(v0, v1, v2) <= 0)
            continue;

        Face* f = faceAdd(depth);
        f->ccb_PIXC = shade;

        const Texture* texture = level.textures + (flags & FACE_TEXTURE);
        ccbSetTexture(flags, f, texture);

        ccbMap3(f, v0, v1, v2, texture->shift);
    }
}

void faceAddMeshQuadsFlat_c(const MeshQuad* polys, int32 count, uint32 shade)
{
    for (int32 i = 0; i < count; i++, polys++)
    {
        uint32 indices = polys->indices;
        uint32 flags = polys->flags;

        uint32 i0 = indices & 0xFF; indices >>= 8;
        uint32 i1 = indices & 0xFF; indices >>= 8;
        uint32 i2 = indices & 0xFF; indices >>= 8;
        uint32 i3 = indices;

        const Vertex* v0 = gVertices + i0;
        const Vertex* v1 = gVertices + i1;
        const Vertex* v2 = gVertices + i2;
        const Vertex* v3 = gVertices + i3;

        uint32 c0 = v0->z;
        uint32 c1 = v1->z;
        uint32 c2 = v2->z;
        uint32 c3 = v3->z;

        if ((c0 & c1 & c2 & c3) & CLIP_MASK)
            continue;

        int32 depth = DEPTH_Q_AVG(v0->z, v1->z, v2->z, v3->z);

        if (cross(v0, v1, v3) <= 0)
            continue;

        Face* f = faceAdd(depth);
        f->ccb_PIXC = shade;

        ccbSetColor(flags, f);

        ccbMap4(f, v0, v1, v2, v3, 20 | (16 << 8));
    }
}

void faceAddMeshTrianglesFlat_c(const MeshTriangle* polys, int32 count, uint32 shade)
{
    for (int32 i = 0; i < count; i++, polys++)
    {
        uint32 indices = polys->indices;
        uint32 flags = polys->flags;

        uint32 i0 = indices & 0xFF; indices >>= 8;
        uint32 i1 = indices & 0xFF; indices >>= 8;
        uint32 i2 = indices;

        const Vertex* v0 = gVertices + i0;
        const Vertex* v1 = gVertices + i1;
        const Vertex* v2 = gVertices + i2;

        uint32 c0 = v0->z;
        uint32 c1 = v1->z;
        uint32 c2 = v2->z;

        if ((c0 & c1 & c2) & CLIP_MASK)
            continue;

        int32 depth = DEPTH_T_AVG(v0->z, v1->z, v2->z);

        if (cross(v0, v1, v2) <= 0)
            continue;

        Face* f = faceAdd(depth);
        f->ccb_PIXC = shade;

        ccbSetColor(flags, f);

        ccbMap3(f, v0, v1, v2, 20 | (16 << 8));
    }
}

int32 sphereIsVisible_c(int32 x, int32 y, int32 z, int32 r)
{
    Matrix &m = matrixGet();

    //if (abs(x) < r && abs(y) < r && abs(z) < r)
    //    return 1;

    int32 vx = DP33(m.e00, m.e01, m.e02, x, y, z);
    int32 vy = DP33(m.e10, m.e11, m.e12, x, y, z);
    int32 vz = DP33(m.e20, m.e21, m.e22, x, y, z);

    if (vz < 0 || vz > VIEW_MAX_F)
        return 0;

    x = vx >> FIXED_SHIFT;
    y = vy >> FIXED_SHIFT;
    z = vz >> FIXED_SHIFT;

    z = PERSPECTIVE_DZ(z);
    int32 d = FixedInvU(z);
    x = (x * d) >> (16 - PROJ_SHIFT);
    y = (y * d) << PROJ_SHIFT;
    r = (r * d);

    int32 rMinX = x - (r >> (16 - PROJ_SHIFT));
    int32 rMaxX = x + (r >> (16 - PROJ_SHIFT));
    int32 rMinY = y - (r << PROJ_SHIFT);
    int32 rMaxY = y + (r << PROJ_SHIFT);

    int32 vMinXY = viewportRel.minXY;
    int32 vMaxXY = viewportRel.maxXY;

    if (rMaxX < (vMinXY >> 16) ||
        rMaxY < (vMinXY << 16) ||
        rMinX > (vMaxXY >> 16) ||
        rMinY > (vMaxXY << 16)) return 0;

    return 1;
}
#endif

void transformRoom(const Room* room, int32 vCount)
{
    unpackRoom(room->data.vertices, vCount);
    projectVertices(vCount);

#ifdef CHECK_LIMITS
    gVerticesCount += vCount;
#endif
}

void transformMesh(const MeshVertex* vertices, int32 vCount)
{
    if (vCount <= 0)
        return;

    unpackMesh(vertices, vCount);
    projectVertices(vCount);

#ifdef CHECK_LIMITS
    gVerticesCount += vCount;
#endif
}

void renderShadow(int32 x, int32 z, int32 sx, int32 sz)
{
#ifdef CHECK_LIMITS
    if (gFacesCount + 3 > MAX_FACES)
        return;
    gFacesCount += 3;
#endif
    x <<= F16_SHIFT;
    z <<= F16_SHIFT;
    sx <<= F16_SHIFT;
    sz <<= F16_SHIFT;

    int32 xns1 = x - sx;
    int32 xps1 = x + sx;
    int32 xns2 = xns1 - sx;
    int32 xps2 = xps1 + sx;

    int32 zns1 = z - sz;
    int32 zps1 = z + sz;
    int32 zns2 = zns1 - sz;
    int32 zps2 = zps1 + sz;

    gVertices[0].x = xns1; gVertices[0].y = 0; gVertices[0].z = zps2;
    gVertices[1].x = xps1; gVertices[1].y = 0; gVertices[1].z = zps2;
    gVertices[2].x = xps2; gVertices[2].y = 0; gVertices[2].z = zps1;
    gVertices[3].x = xps2; gVertices[3].y = 0; gVertices[3].z = zns1;
    gVertices[4].x = xps1; gVertices[4].y = 0; gVertices[4].z = zns2;
    gVertices[5].x = xns1; gVertices[5].y = 0; gVertices[5].z = zns2;
    gVertices[6].x = xns2; gVertices[6].y = 0; gVertices[6].z = zns1;
    gVertices[7].x = xns2; gVertices[7].y = 0; gVertices[7].z = zps1;

    projectVertices(8);
    faceAddMeshQuadsFlat(gShadowQuads, 3, SHADE_SHADOW);
}

void renderSprite(int32 vx, int32 vy, int32 vz, int32 vg, int32 index)
{
#ifdef CHECK_LIMITS
    if (gFacesCount >= MAX_FACES)
        return;
    gFacesCount++;
#endif

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

    PERSPECTIVE(x, y, z);

    const Sprite* sprite = level.sprites + index;

    int32 d = (1 << 20) / z;
    int32 x0 = sprite->l * d >> 12;
    int32 x1 = sprite->r * d >> 12;
    if (x0 == x1) return;

    int32 y0 = sprite->t * d >> 12;
    int32 y1 = sprite->b * d >> 12;
    if (y0 == y1) return;

    x0 += x;
    x1 += x;
    y0 += y;
    y1 += y;

    int32 vMinXY = viewportRel.minXY;
    int32 vMaxXY = viewportRel.maxXY;

    if (x0 > (vMaxXY >> 16) ||
        x1 < (vMinXY >> 16) ||
        y0 > (vMaxXY << 16 >> 16) ||
        y1 < (vMinXY << 16 >> 16)) return;

    int32 depth = X_MAX(0, z - 128) >> OT_SHIFT; // depth hack

    Face* f = faceAdd(depth);

    if (depth > (FOG_MIN >> OT_SHIFT)) {
        vg += (depth - (FOG_MIN >> OT_SHIFT)) << 4;
        vg = X_MIN(vg, 8191);
    }

    vg >>= 8;
    f->ccb_PIXC = shadeTable[vg];

    Texture* texture = level.textures + sprite->texture;
    ccbSetTexture(0, f, texture);

    uint32 shift = texture->shift;
    uint32 ws = shift & 0xFF;
    uint32 hs = shift >> 8;

    f->ccb_HDX = (x1 - x0) << ws;
    f->ccb_HDY = 0;
    f->ccb_VDX = 0;
    f->ccb_VDY = (y1 - y0) << hs;

    f->ccb_XPos = (x0 + (FRAME_WIDTH  >> 1)) << 16;
    f->ccb_YPos = (y0 + (FRAME_HEIGHT >> 1)) << 16;

    f->ccb_HDDX = 0;
    f->ccb_HDDY = 0;
}

void renderGlyph(int32 vx, int32 vy, int32 index)
{
#ifdef CHECK_LIMITS
    if (gFacesCount >= MAX_FACES)
        return;
    gFacesCount++;
#endif

    const Sprite* sprite = level.sprites + index;

    Face* f = faceAdd(0);
    f->ccb_PIXC = SHADE_16;

    Texture* texture = level.textures + sprite->texture;
    ccbSetTexture(0, f, texture);

    uint32 shift = texture->shift;
    uint32 ws = shift & 0xFF;
    uint32 hs = shift >> 8;

    f->ccb_HDX = (sprite->r - sprite->l) << ws;
    f->ccb_HDY = 0;
    f->ccb_VDX = 0;
    f->ccb_VDY = (sprite->b - sprite->t) << hs;

    f->ccb_XPos = (vx + sprite->l) << 16;
    f->ccb_YPos = (vy + sprite->t) << 16;

    f->ccb_HDDX = 0;
    f->ccb_HDDY = 0;
}

void faceAddRoom(const Room* room)
{
    if (room->info->quadsCount) {
        faceAddRoomQuads(room->data.quads, room->info->quadsCount);
    }

    if (room->info->trianglesCount) {
        faceAddRoomTriangles(room->data.triangles, room->info->trianglesCount);
    }

#ifdef CHECK_LIMITS
    gFacesCount = gFacesBase - gFaces;
#endif
}

void faceAddMesh(const MeshQuad* rFaces, const MeshQuad* crFaces, const MeshTriangle* tFaces, const MeshTriangle* ctFaces, int32 rCount, int32 crCount, int32 tCount, int32 ctCount, int32 intensity)
{
    uint32 shade = shadeTable[X_CLAMP((gLightAmbient + intensity) >> 8, 0, 31)];

    if (rCount) {
        faceAddMeshQuads(rFaces, rCount, shade);
    }

    if (tCount) {
        faceAddMeshTriangles(tFaces, tCount, shade);
    }

    if (crCount) {
        faceAddMeshQuadsFlat(crFaces, crCount, shade);
    }

    if (ctCount) {
        faceAddMeshTrianglesFlat(ctFaces, ctCount, shade);
    }

#ifdef CHECK_LIMITS
    gFacesCount = gFacesBase - gFaces;
#endif
}

void flush()
{
    Face* facesHead = NULL;
    Face* facesTail = NULL;

    if (gFaces != gFacesBase)
    {
        PROFILE(CNT_FLUSH);

        for (int32 i = OT_SIZE - 1; i >= 0; i--)
        {
            if (!gOT[i].head) continue;

            Face *face = gOT[i].head;
            gOT[i].head = NULL;

            if (face)
            {
                if (facesTail)
                {
                    facesTail->ccb_NextPtr = face;
                } else if (!facesHead) {
                    facesHead = face;
                }

                facesTail = gOT[i].tail;
            }
        }

        if (facesHead)
        {
            LAST_CEL(facesTail);
            DrawScreenCels(screenItem, (CCB*)facesHead);
            UNLAST_CEL(facesTail);
        }
    }

#ifdef PROFILING
    #if !defined(PROFILE_FRAMETIME) && !defined(PROFILE_SOUNDTIME)
        gCounters[CNT_VERT] += gVerticesCount;
        gCounters[CNT_POLY] += gFacesCount;
    #endif
#endif

    gVerticesCount = 0;
    gFacesCount = 0;
    gFacesBase = gFaces;
}

void clear()
{
    // we use fast clear via SPORT on vblank
}

void renderRoom(const Room* room)
{
    int32 vCount = room->info->verticesCount;
    if (vCount <= 0)
        return;

    transformRoom(room, vCount);
    faceAddRoom(room);
}

void renderMesh(const Mesh* mesh)
{
    int32 vCount = mesh->vCount;
    if (vCount <= 0)
        return;

    const uint8* ptr = (uint8*)mesh + sizeof(Mesh);

    const MeshVertex* vertices = (MeshVertex*)ptr;
    ptr += vCount * sizeof(vertices[0]);

    if (vCount & 1) { // data alignment
        ptr += sizeof(MeshVertex);
    }

    MeshQuad* rFaces = (MeshQuad*)ptr;
    ptr += mesh->rCount * sizeof(MeshQuad);

    MeshTriangle* tFaces = (MeshTriangle*)ptr;
    ptr += mesh->tCount * sizeof(MeshTriangle);

    MeshQuad* crFaces = (MeshQuad*)ptr;
    ptr += mesh->crCount * sizeof(MeshQuad);

    MeshTriangle* ctFaces = (MeshTriangle*)ptr;
    ptr += mesh->ctCount * sizeof(MeshTriangle);

    transformMesh(vertices, vCount);
    faceAddMesh(rFaces, crFaces, tFaces, ctFaces, mesh->rCount, mesh->crCount, mesh->tCount, mesh->ctCount, mesh->intensity);
}

void renderBorder(int32 x, int32 y, int32 width, int32 height, int32 shade, int32 color1, int32 color2, int32 z)
{
    // TODO
}

#define BAR_HEIGHT  5

void renderBar(int32 x, int32 y, int32 width, int32 value, BarType type)
{
    // TODO
}

void renderBackground(const void* background)
{
    // TODO
}

void* copyBackground()
{
    return NULL; // TODO
}
