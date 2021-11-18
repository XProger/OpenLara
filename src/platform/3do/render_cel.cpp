#include "common.h"

//#define DEBUG_CLIPPING

struct Vertex
{
    int32 x, y, z; // for rooms z = (depth << CLIP_SHIFT) | ClipFlags
};

uint16* gPalette;
int32 gPaletteOffset; // offset to the default or underwater PLUTs

extern Level level;
extern int32 lightAmbient;
extern int32 randTable[MAX_RAND_TABLE];
extern int32 caustics[MAX_CAUSTICS];
extern int32 causticsFrame;

int32 gVerticesCount;
int32 gFacesCount;

Vertex gVertices[MAX_VERTICES];

Face* gFaces; // MAX_FACES
Face* otFacesHead[OT_SIZE];
Face* otFacesTail[OT_SIZE];

int32 otMin = OT_SIZE - 1;
int32 otMax = 0;

RectMinMax viewportRel;

bool enableClipping;

#define SHADOW_OPACITY  3   // 50%
#define MIP_DIST        (1024 * 5)

extern Item screenItem;

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

static const uint32 shadeTable[32] = {
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

void renderInit()
{
    gPalette = (uint16*)RAM_TEX;
    gFaces = (Face*)RAM_CEL;

    Face* face = gFaces;
    for (int32 i = 0; i < MAX_FACES; i++, face++)
    {
        // set preamble for CCB_CCBPRE flag
        face->ccb_PRE0 = PRE0_BGND | PRE0_LINEAR | PRE0_BPP_16;
        face->ccb_PRE1 = PRE1_TLLSB_PDC0;
    }
}

void setViewport(const RectMinMax &vp)
{
    viewport = vp;

    viewportRel.x0 = vp.x0 - (FRAME_WIDTH  >> 1);
    viewportRel.y0 = vp.y0 - (FRAME_HEIGHT >> 1);
    viewportRel.x1 = vp.x1 - (FRAME_WIDTH  >> 1);
    viewportRel.y1 = vp.y1 - (FRAME_HEIGHT >> 1);
}

void setPaletteIndex(int32 index)
{
    gPaletteOffset = index * level.tilesCount * sizeof(uint16) * 16;
    gPalette = (uint16*)((uint8*)RAM_TEX + (*(uint32*)RAM_TEX) + gPaletteOffset);
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

int32 boxIsVisible(const AABBs* box)
{
    RectMinMax rect;
    if (!transformBoxRect(box, &rect))
        return 0; // not visible
    return rectIsVisible(&rect);
}

X_INLINE bool checkBackface(const Vertex *a, const Vertex *b, const Vertex *c)
{
    return (b->x - a->x) * (c->y - a->y) -
           (c->x - a->x) * (b->y - a->y) <= 0;
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

#define USE_ASM

#ifdef USE_ASM
    #define unpackRoom unpackRoom_asm
    #define unpackMesh unpackMesh_asm
    #define projectVertices projectVertices_asm

    extern "C" void unpackRoom_asm(const RoomVertex* vertices, int32 vCount);
    extern "C" void unpackMesh_asm(const MeshVertex* vertices, int32 vCount);
    extern "C" void projectVertices_asm(int32 vCount);
#else
    #define unpackRoom unpackRoom_c
    #define unpackMesh unpackMesh_c
    #define projectVertices projectVertices_c

void unpackRoom_c(const RoomVertex* vertices, int32 vCount)
{
    Vertex* res = gVertices;

    uint32 *v32 = (uint32*)vertices;

    for (int32 i = 0; i < vCount; i += 4)
    {
        uint32 n0 = *v32++;
        uint32 n1 = *v32++;

        res->x = (n0 << 10) & 0x7C00;
        res->y = (n0 << 3) & 0x3F00;
        res->z = (n0 >> 1) & 0x7C00;
        res++;

        res->x = (n0 >> 6) & 0x7C00;
        res->y = (n0 >> 13) & 0x3F00;
        res->z = (n0 >> 17) & 0x7C00;
        res++;

        res->x = (n1 << 10) & 0x7C00;
        res->y = (n1 << 3) & 0x3F00;
        res->z = (n1 >> 1) & 0x7C00;
        res++;

        res->x = (n1 >> 6) & 0x7C00;
        res->y = (n1 >> 13) & 0x3F00;
        res->z = (n1 >> 17) & 0x7C00;
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
    Matrix& m = matrixGet();

    Vertex* v = gVertices;

    int32 minX = viewportRel.x0;
    int32 minY = viewportRel.y0;
    int32 maxX = viewportRel.x1;
    int32 maxY = viewportRel.y1;

    for (int32 i = 0; i < vCount; i++)
    {
        int32 vx = v->x;
        int32 vy = v->y;
        int32 vz = v->z;

        int32 x = DP43(m.e00, m.e01, m.e02, m.e03, vx, vy, vz);
        int32 y = DP43(m.e10, m.e11, m.e12, m.e13, vx, vy, vz);
        int32 z = DP43(m.e20, m.e21, m.e22, m.e23, vx, vy, vz);

        int32 clip = 0;

        if (z < VIEW_MIN_F) {
            z = VIEW_MIN_F;
            clip = CLIP_NEAR;
        } else if (z > VIEW_MAX_F) {
            z = VIEW_MAX_F;
            clip = CLIP_FAR;
        }

        x >>= FIXED_SHIFT;
        y >>= FIXED_SHIFT;
        z >>= FIXED_SHIFT;

        PERSPECTIVE(x, y, z);

        if (x < minX) clip |= CLIP_LEFT;
        if (y < minY) clip |= CLIP_TOP;
        if (x > maxX) clip |= CLIP_RIGHT;
        if (y > maxY) clip |= CLIP_BOTTOM;

        v->x = x;
        v->y = y;
        v->z = (z << CLIP_SHIFT) | clip;
        v++;
    }
}
#endif

bool transformBoxRect(const AABBs* box, RectMinMax* rect)
{
    Matrix &m = matrixGet();

    if ((m.e23 < VIEW_MIN_F) || (m.e23 >= VIEW_MAX_F)) {
        return false;
    }

    int32 minX = box->minX;
    int32 maxX = box->maxX;
    int32 minY = box->minY;
    int32 maxY = box->maxY;
    int32 minZ = box->minZ;
    int32 maxZ = box->maxZ;

    gVertices[0].x = minX; gVertices[0].y = minY; gVertices[0].z = minZ;
    gVertices[1].x = maxX; gVertices[1].y = minY; gVertices[1].z = minZ;
    gVertices[2].x = minX; gVertices[2].y = maxY; gVertices[2].z = minZ;
    gVertices[3].x = maxX; gVertices[3].y = maxY; gVertices[3].z = minZ;
    gVertices[4].x = minX; gVertices[4].y = minY; gVertices[4].z = maxZ;
    gVertices[5].x = maxX; gVertices[5].y = minY; gVertices[5].z = maxZ;
    gVertices[6].x = minX; gVertices[6].y = maxY; gVertices[6].z = maxZ;
    gVertices[7].x = maxX; gVertices[7].y = maxY; gVertices[7].z = maxZ;

    projectVertices(8);

    *rect = RectMinMax( INT_MAX, INT_MAX, INT_MIN, INT_MIN );

    Vertex* v = gVertices;

    for (int32 i = 0; i < 8; i++, v++)
    {
        int32 x = v->x;
        int32 y = v->y;
        int32 z = v->z;

        if ((z & CLIP_MASK) & (CLIP_NEAR | CLIP_FAR))
            continue;

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

void transformRoom(const Room* room)
{
    int32 vCount = room->info->verticesCount;
    if (vCount <= 0)
        return;

    unpackRoom(room->data.vertices, vCount);

    projectVertices(vCount);

    gVerticesCount += vCount;
}

void transformMesh(const MeshVertex* vertices, int32 vCount, const uint16* vIntensity, const vec3s* vNormal)
{
    unpackMesh(vertices, vCount);

    projectVertices(vCount);

    gVerticesCount += vCount;
}

#define DEPTH_T_AVG(z0,z1,z2)    ((z0 + z1 + z2 + z2) >> (2 + CLIP_SHIFT + OT_SHIFT))
#define DEPTH_Q_AVG(z0,z1,z2,z3) ((z0 + z1 + z2 + z3) >> (2 + CLIP_SHIFT + OT_SHIFT))
#define DEPTH_T_MAX(z0,z1,z2)    (X_MAX(z0, X_MAX(z1, z2)) >> (CLIP_SHIFT + OT_SHIFT))
#define DEPTH_Q_MAX(z0,z1,z2,z3) (X_MAX(z0, X_MAX(z1, X_MAX(z2, z3))) >> (CLIP_SHIFT + OT_SHIFT))

X_INLINE Face* faceAdd(int32 depth)
{
    if (depth < 0) depth = 0;
    if (depth > OT_SIZE - 1) depth = OT_SIZE - 1;

    if (depth < otMin) otMin = depth;
    if (depth > otMax) otMax = depth;

    Face* face = gFaces + gFacesCount++;

    if (otFacesHead[depth]) {
        face->ccb_NextPtr = otFacesHead[depth];
    } else {
        face->ccb_NextPtr = NULL;
        otFacesTail[depth] = face;
    }

    otFacesHead[depth] = face;

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
    face->ccb_PLUTPtr = texture->plut + gPaletteOffset;
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
    uint32 hs = shift >> 8;

    int32 hdx0 = (x1 - x0) << ws;
    int32 hdy0 = (y1 - y0) << ws;
    int32 hdx1 = (x2 - x3) << ws;
    int32 hdy1 = (y2 - y3) << ws;
    int32 vdx0 = (x3 - x0) << hs;
    int32 vdy0 = (y3 - y0) << hs;

    hs = 16 - hs;
    int32 hddx = (hdx1 - hdx0) >> hs;
    int32 hddy = (hdy1 - hdy0) >> hs;

    f->ccb_XPos = (x0 << 16) + (((FRAME_WIDTH  >> 1) << 16) + 32768);
    f->ccb_YPos = (y0 << 16) + (((FRAME_HEIGHT >> 1) << 16) + 32768);
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
    uint32 hs = shift >> 8;

    int32 hdx0 = (x1 - x0) << ws;
    int32 hdy0 = (y1 - y0) << ws;
    int32 vdx0 = (x2 - x0) << hs;
    int32 vdy0 = (y2 - y0) << hs;

    f->ccb_XPos = (x0 << 16) + (((FRAME_WIDTH  >> 1) << 16) + 32768);
    f->ccb_YPos = (y0 << 16) + (((FRAME_HEIGHT >> 1) << 16) + 32768);
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

X_INLINE void faceAddRoomQuad(uint32 flags, const Index* indices)
{
    uint32 i01 = ((uint32*)indices)[0];
    uint32 i23 = ((uint32*)indices)[1];

    uint32 i0 = (i01 >> 16);
    uint32 i1 = (i01 & 0xFFFF);
    uint32 i2 = (i23 >> 16);
    uint32 i3 = (i23 & 0xFFFF);

    const Vertex* v0 = gVertices + i0;
    const Vertex* v1 = gVertices + i1;
    const Vertex* v2 = gVertices + i2;
    const Vertex* v3 = gVertices + i3;

    uint32 c0 = v0->z;
    uint32 c1 = v1->z;
    uint32 c2 = v2->z;
    uint32 c3 = v3->z;

    if ((c0 & c1 & c2 & c3) & CLIP_MASK)
        return;

    int32 depth = DEPTH_Q_MAX(c0, c1, c2, c3);

    if (checkBackface(v0, v1, v3) == !(flags & FACE_CCW))
        return;

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

    ccbMap4(f, v0, v1, v2, v3, texture->shift);
}

X_INLINE void faceAddRoomTriangle(uint32 flags, const Index* indices)
{
    uint32 i01 = ((uint32*)indices)[0];
    uint32 i23 = ((uint32*)indices)[1];

    uint32 i0 = (i01 >> 16);
    uint32 i1 = (i01 & 0xFFFF);
    uint32 i2 = (i23 >> 16);

    const Vertex* v0 = gVertices + i0;
    const Vertex* v1 = gVertices + i1;
    const Vertex* v2 = gVertices + i2;

    uint32 c0 = v0->z;
    uint32 c1 = v1->z;
    uint32 c2 = v2->z;

    if ((c0 & c1 & c2) & CLIP_MASK)
        return;

    int32 depth = DEPTH_T_MAX(c0, c1, c2);

    if (checkBackface(v0, v1, v2) == !(flags & FACE_CCW))
        return;

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

X_INLINE void faceAddMeshQuad(uint32 flags, uint32 indices, uint32 shade)
{
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
        return;

    if (checkBackface(v0, v1, v3) == !(flags & FACE_CCW)) // TODO (hdx0 * vdy0 - vdx0 * hdy0) <= 0
        return;

    int32 depth = DEPTH_Q_AVG(v0->z, v1->z, v2->z, v3->z);

    Face* f = faceAdd(depth);
    f->ccb_PIXC = shade;

    const Texture* texture = level.textures + (flags & FACE_TEXTURE);
    ccbSetTexture(flags, f, texture);

    ccbMap4(f, v0, v1, v2, v3, texture->shift);
}

X_INLINE void faceAddMeshTriangle(uint32 flags, uint32 indices, uint32 shade)
{
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
        return;

    if (checkBackface(v0, v1, v2))
        return;

    int32 depth = DEPTH_T_AVG(v0->z, v1->z, v2->z);

    Face* f = faceAdd(depth);
    f->ccb_PIXC = shade;

    const Texture* texture = level.textures + (flags & FACE_TEXTURE);
    ccbSetTexture(flags, f, texture);

    ccbMap3(f, v0, v1, v2, texture->shift);
}

X_INLINE void faceAddMeshQuadFlat(uint32 flags, uint32 indices, uint32 shade)
{
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
        return;

    if (checkBackface(v0, v1, v3))
        return;

    int32 depth = DEPTH_Q_AVG(v0->z, v1->z, v2->z, v3->z);

    Face* f = faceAdd(depth);
    f->ccb_PIXC = shade;

    ccbSetColor(flags, f);

    ccbMap4(f, v0, v1, v2, v3, 20 | (16 << 8));
}

X_INLINE void faceAddMeshTriangleFlat(uint32 flags, uint32 indices, uint32 shade)
{
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
        return;

    if (checkBackface(v0, v1, v2))
        return;

    int32 depth = DEPTH_T_AVG(v0->z, v1->z, v2->z);

    Face* f = faceAdd(depth);
    f->ccb_PIXC = shade;

    ccbSetColor(flags, f);

    ccbMap3(f, v0, v1, v2, 20 | (16 << 8));
}

void faceAddShadow(int32 x, int32 z, int32 sx, int32 sz)
{
    int32 sx2 = sx << 1;
    int32 sz2 = sz << 1;

    MeshVertex v[8] = {
        { x - sx,  0, z + sz2 }, // 0
        { x + sx,  0, z + sz2 }, // 1
        { x + sx2, 0, z + sz  }, // 2
        { x + sx2, 0, z - sz  }, // 3
        { x + sx,  0, z - sz2 }, // 4
        { x - sx,  0, z - sz2 }, // 5
        { x - sx2, 0, z - sz  }, // 6
        { x - sx2, 0, z + sz  }  // 7
    };

    transformMesh(v, 8, NULL, NULL);

    faceAddMeshQuadFlat(0, (0 | (1 << 8) | (2 << 16) | (7 << 24)), SHADE_SHADOW);
    faceAddMeshQuadFlat(0, (7 | (2 << 8) | (3 << 16) | (6 << 24)), SHADE_SHADOW);
    faceAddMeshQuadFlat(0, (6 | (3 << 8) | (4 << 16) | (5 << 24)), SHADE_SHADOW);
}

void faceAddSprite(int32 vx, int32 vy, int32 vz, int32 vg, int32 index)
{
    if (gFacesCount >= MAX_FACES)
        return;

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

    if (x0 >= viewportRel.x1) return;
    if (y0 >= viewportRel.y1) return;
    if (x1 <= viewportRel.x0) return;
    if (y1 <= viewportRel.y0) return;

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

void faceAddGlyph(int32 vx, int32 vy, int32 index)
{
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

void faceAddRoom(const RoomQuad* quads, int32 qCount, const RoomTriangle* triangles, int32 tCount)
{
    for (int32 i = 0; i < qCount; i++, quads++) {
        faceAddRoomQuad(quads->flags, quads->indices);
    }

    for (int32 i = 0; i < tCount; i++, triangles++) {
        faceAddRoomTriangle(triangles->flags, triangles->indices);
    }
}

void faceAddMesh(const MeshQuad* rFaces, const MeshQuad* crFaces, const MeshTriangle* tFaces, const MeshTriangle* ctFaces, int32 rCount, int32 crCount, int32 tCount, int32 ctCount)
{
    uint32 shade;
    if (lightAmbient > 4096) {
        shade = shadeTable[lightAmbient >> 8];
    } else {
        shade = SHADE_16;
    }

    for (int32 i = 0; i < rCount; i++) {
        faceAddMeshQuad(rFaces[i].flags, rFaces[i].indices, shade);
    }

    for (int32 i = 0; i < tCount; i++) {
        faceAddMeshTriangle(tFaces[i].flags, tFaces[i].indices, shade);
    }

    for (int32 i = 0; i < crCount; i++) {
        faceAddMeshQuadFlat(crFaces[i].flags, crFaces[i].indices, shade);
    }

    for (int32 i = 0; i < ctCount; i++) {
        faceAddMeshTriangleFlat(ctFaces[i].flags, ctFaces[i].indices, shade);
    }
}

void flush()
{
    Face* facesHead = NULL;
    Face* facesTail = NULL;

    if (gFacesCount)
    {
        PROFILE(CNT_FLUSH);

        for (int32 i = otMax; i >= otMin; i--)
        {
            if (!otFacesHead[i]) continue;

            Face *face = otFacesHead[i];
            otFacesHead[i] = NULL;

            if (face)
            {
                if (facesTail)
                {
                    facesTail->ccb_NextPtr = face;
                } else if (!facesHead) {
                    facesHead = face;
                }

                facesTail = otFacesTail[i];
            }
        }

        otMin = OT_SIZE - 1;
        otMax = 0;

        if (facesHead)
        {
            LAST_CEL(facesTail);
            DrawScreenCels(screenItem, facesHead);
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
}

void clear()
{
    // we use fast clear via SPORT on vblank
}
