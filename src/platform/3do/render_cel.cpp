#include "common.h"

struct Vertex
{
    int32 x, y, z;
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
uint8  gClip[MAX_VERTICES];

Face* gFaces; // MAX_FACES
Face* otFacesHead[OT_SIZE];
Face* otFacesTail[OT_SIZE];

int32 otMin = OT_SIZE - 1;
int32 otMax = 0;

RectMinMax viewportRel;

bool enableAlphaTest;
bool enableClipping;
bool secondPalette;

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
    SHADE_24 = DUP16( PPMPC_MF_8 | PPMPC_SF_16 | PPMPC_2S_PDC ), // 1 + 8/16
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

X_INLINE int32 classify(const Vertex* v, int32 x0, int32 y0, int32 x1, int32 y1)
{
    return (v->x < x0 ? 1 : 0) |
           (v->x > x1 ? 2 : 0) |
           (v->y < y0 ? 4 : 0) |
           (v->y > y1 ? 8 : 0);
}

void transform(vec3i* points, int32 count)
{
    mmv3m33d desc;
    desc.dest  =
    desc.src   = (vec3f16*)points;
    desc.mat   = (mat33f16*)&matrixGet();
    desc.n     = 1 << 8;
    desc.count = count;

    MulManyVec3Mat33DivZ_F16(&desc);
}

bool transformBoxRect(const AABBs* box, RectMinMax* rect)
{
    Matrix &m = matrixGet();

    if ((m.e23 < VIEW_MIN_F) || (m.e23 >= VIEW_MAX_F)) {
        return false;
    }

    int32 cx = Dot3_F16(*(vec3f16*)&m.e03, *(vec3f16*)&m.e00) >> DOT_SHIFT; // dot(pos, right)
    int32 cy = Dot3_F16(*(vec3f16*)&m.e03, *(vec3f16*)&m.e01) >> DOT_SHIFT; // dot(pos, up)
    int32 cz = Dot3_F16(*(vec3f16*)&m.e03, *(vec3f16*)&m.e02) >> DOT_SHIFT; // dot(pos, dir)

    AABBi b;
    b.minX = (box->minX << F16_SHIFT) + cx;
    b.maxX = (box->maxX << F16_SHIFT) + cx;
    b.minY = (box->minY << F16_SHIFT) + cy;
    b.maxY = (box->maxY << F16_SHIFT) + cy;
    b.minZ = (box->minZ << F16_SHIFT) + cz;
    b.maxZ = (box->maxZ << F16_SHIFT) + cz;

    vec3i v[8] = {
        { b.minX, b.minY, b.minZ },
        { b.maxX, b.minY, b.minZ },
        { b.minX, b.maxY, b.minZ },
        { b.maxX, b.maxY, b.minZ },
        { b.minX, b.minY, b.maxZ },
        { b.maxX, b.minY, b.maxZ },
        { b.minX, b.maxY, b.maxZ },
        { b.maxX, b.maxY, b.maxZ }
    };

    transform(v, 8);

    *rect = RectMinMax( INT_MAX, INT_MAX, INT_MIN, INT_MIN );

    for (int32 i = 0; i < 8; i++)
    {
        if (v[i].z <= 0)
            return false;

        if (v[i].z >= (VIEW_MAX_F >> FIXED_SHIFT))
            continue;

        if (v[i].x < rect->x0) rect->x0 = v[i].x;
        if (v[i].x > rect->x1) rect->x1 = v[i].x;
        if (v[i].y < rect->y0) rect->y0 = v[i].y;
        if (v[i].y > rect->y1) rect->y1 = v[i].y;
    }

    rect->x0 += (FRAME_WIDTH  / 2);
    rect->y0 += (FRAME_HEIGHT / 2);
    rect->x1 += (FRAME_WIDTH  / 2);
    rect->y1 += (FRAME_HEIGHT / 2);

    return true;
}

const RoomVertex* gRoomVertices;

void transformRoom(const RoomVertex* vertices, int32 vCount, bool underwater)
{
    gRoomVertices = vertices;

    int32 cx = cameraViewOffset.x << F16_SHIFT;
    int32 cy = cameraViewOffset.y << F16_SHIFT;
    int32 cz = cameraViewOffset.z << F16_SHIFT;

    Vertex* res = &gVertices[gVerticesCount];

    // use one aligned LDR instead of LDRB x3
    uint32 *v32 = (uint32*)vertices;

    for (int32 i = 0; i < vCount; i++, res++)
    {
        uint32 p = *v32++;
        res->x = cx + ((p >> 24)         << (10 + F16_SHIFT));
        res->y = cy + (int8(p >> 16)     << ( 8 + F16_SHIFT));
        res->z = cz + (((p >> 8) & 0xFF) << (10 + F16_SHIFT));
    }

    transform((vec3i*)&gVertices[gVerticesCount], vCount);

    int32 x0 = viewportRel.x0;
    int32 y0 = viewportRel.y0;
    int32 x1 = viewportRel.x1;
    int32 y1 = viewportRel.y1;

    res = &gVertices[gVerticesCount];
    uint8* clip = &gClip[gVerticesCount];

    for (int32 i = 0; i < vCount; i++, res++, clip++)
    {
        if (res->z < (VIEW_MIN_F >> FIXED_SHIFT)) {
            res->z = (VIEW_MIN_F >> FIXED_SHIFT);
            *clip = 32;
        } else if (res->z >= (VIEW_MAX_F >> FIXED_SHIFT)) {
            res->z = (VIEW_MAX_F >> FIXED_SHIFT);
            *clip = 16;
        } else {
            *clip = classify(res, x0, y0, x1, y1);
        }
    }

    if (viewport.x0 == 0 && viewport.y0 == 0)
        return;

    gVerticesCount += vCount;
}

void transformMesh(const MeshVertex* vertices, int32 vCount, const uint16* vIntensity, const vec3s* vNormal)
{
    Matrix &m = matrixGet();

    // TODO_3DO normalize 3x3 interpolated matrix or get cameraViewOffset for the general case somehow
    // TODO_3DO MulVec3Mat33_F16 (transposed?)
    int32 cx = Dot3_F16(*(vec3f16*)&m.e03, *(vec3f16*)&m.e00) >> DOT_SHIFT; // dot(pos, right)
    int32 cy = Dot3_F16(*(vec3f16*)&m.e03, *(vec3f16*)&m.e01) >> DOT_SHIFT; // dot(pos, up)
    int32 cz = Dot3_F16(*(vec3f16*)&m.e03, *(vec3f16*)&m.e02) >> DOT_SHIFT; // dot(pos, dir)

    Vertex* res = &gVertices[gVerticesCount];

    uint32 *v32 = (uint32*)vertices;

    for (int32 i = 0; i < vCount; i++, res++)
    {
        // << F16_SHIFT should be already applied
        uint32 p = *v32++;
        //p += cxy;
        res->x = cx + int16(p);
        res->y = cy + int16(p >> 16);
        p = *v32++;
        res->z = cz + int16(p);
    }

    transform((vec3i*)&gVertices[gVerticesCount], vCount);

    int32 x0 = viewportRel.x0;
    int32 y0 = viewportRel.y0;
    int32 x1 = viewportRel.x1;
    int32 y1 = viewportRel.y1;
/*
    res = &gVertices[gVerticesCount];
    uint8* clip = &gClip[gVerticesCount];

    for (int32 i = 0; i < vCount; i++, res++, clip++)
    {
        if (res->z < (VIEW_MIN_F >> FIXED_SHIFT) || res->z >= (VIEW_MAX_F >> FIXED_SHIFT)) {
            *clip = 32;
        } else {
            *clip = enableClipping ? classify(res, x0, y0, x1, y1) : 0;
        }
    }
*/
    gVerticesCount += vCount;
}

#define DEPTH_T_AVG() ((v0->z + v1->z + v2->z + v2->z) >> (2 + OT_SHIFT))
#define DEPTH_T_MAX() (X_MAX(v0->z, X_MAX(v1->z, v2->z)) >> OT_SHIFT)
#define DEPTH_Q_AVG() ((v0->z + v1->z + v2->z + v3->z) >> (2 + OT_SHIFT))
#define DEPTH_Q_MAX() (X_MAX(v0->z, X_MAX(v1->z, X_MAX(v2->z, v3->z))) >> OT_SHIFT)

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

X_INLINE void faceAddRoomQuad(uint32 flags, const Index* indices, int32 startVertex32)
{
    uint32 i01 = startVertex32 + ((uint32*)indices)[0];
    uint32 i23 = startVertex32 + ((uint32*)indices)[1];

    uint32 i0 = (i01 >> 16);
    uint32 i1 = (i01 & 0xFFFF);
    uint32 i2 = (i23 >> 16);
    uint32 i3 = (i23 & 0xFFFF);

    uint8 c0 = gClip[i0];
    uint8 c1 = gClip[i1];
    uint8 c2 = gClip[i2];
    uint8 c3 = gClip[i3];

    if (c0 & c1 & c2 & c3)
        return;

    if (c0 == 32 || c1 == 32 || c2 == 32 || c3 == 32)
        return;

    const Vertex* v0 = gVertices + i0;
    const Vertex* v1 = gVertices + i1;
    const Vertex* v2 = gVertices + i2;
    const Vertex* v3 = gVertices + i3;

    if (checkBackface(v0, v1, v2) == !(flags & FACE_CCW))
        return;

    int32 depth = DEPTH_Q_MAX();

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

    int32 x0 = v0->x;
    int32 y0 = v0->y;
    int32 x1 = v1->x;
    int32 y1 = v1->y;
    int32 x2 = v2->x;
    int32 y2 = v2->y;
    int32 x3 = v3->x;
    int32 y3 = v3->y;

    int32 ws = texture->wShift;
    int32 hs = texture->hShift;

    int32 hdx0 = (x1 - x0) << ws;
    int32 hdy0 = (y1 - y0) << ws;
    int32 hdx1 = (x2 - x3) << ws;
    int32 hdy1 = (y2 - y3) << ws;
    int32 vdx0 = (x3 - x0) << hs;
    int32 vdy0 = (y3 - y0) << hs;

    f->ccb_HDX = hdx0;
    f->ccb_HDY = hdy0;
    f->ccb_VDX = vdx0;
    f->ccb_VDY = vdy0;

    f->ccb_XPos = (x0 + (FRAME_WIDTH  >> 1)) << 16;
    f->ccb_YPos = (y0 + (FRAME_HEIGHT >> 1)) << 16;

    hs = 16 - hs;
    f->ccb_HDDX = (hdx1 - hdx0) >> hs;
    f->ccb_HDDY = (hdy1 - hdy0) >> hs;
}

X_INLINE void faceAddRoomTriangle(uint32 flags, const Index* indices, int32 startVertex32)
{
    uint32 i01 = ((uint32*)indices)[0] + startVertex32;
    uint32 i23 = ((uint32*)indices)[1] + startVertex32;

    uint32 i0 = (i01 >> 16);
    uint32 i1 = (i01 & 0xFFFF);
    uint32 i2 = (i23 >> 16);

    uint8 c0 = gClip[i0];
    uint8 c1 = gClip[i1];
    uint8 c2 = gClip[i2];

    if (c0 & c1 & c2)
        return;

    if (c0 == 32 || c1 == 32 || c2 == 32)
        return;

    const Vertex* v0 = gVertices + i0;
    const Vertex* v1 = gVertices + i1;
    const Vertex* v2 = gVertices + i2;

    if (checkBackface(v0, v1, v2))
        return;

    int32 depth = DEPTH_T_MAX();

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

    int32 x0 = v0->x;
    int32 y0 = v0->y;
    int32 x1 = v1->x;
    int32 y1 = v1->y;
    int32 x2 = v2->x;
    int32 y2 = v2->y;

    int32 ws = texture->wShift;
    int32 hs = texture->hShift;

    int32 hdx0 = (x1 - x0) << ws;
    int32 hdy0 = (y1 - y0) << ws;
    int32 vdx0 = (x2 - x0) << hs;
    int32 vdy0 = (y2 - y0) << hs;

    f->ccb_HDX = hdx0;
    f->ccb_HDY = hdy0;
    f->ccb_VDX = vdx0;
    f->ccb_VDY = vdy0;

    f->ccb_XPos = (x0 + (FRAME_WIDTH  >> 1)) << 16;
    f->ccb_YPos = (y0 + (FRAME_HEIGHT >> 1)) << 16;

    hs = 16 - hs;
    f->ccb_HDDX = -hdx0 >> hs;
    f->ccb_HDDY = -hdy0 >> hs;
}

X_INLINE void faceAddMeshQuad(uint32 flags, const Index* indices, int32 startVertex32, uint32 shade)
{
    uint32 i01 = startVertex32 + ((uint32*)indices)[0];
    uint32 i23 = startVertex32 + ((uint32*)indices)[1];

    uint32 i0 = (i01 >> 16);
    uint32 i1 = (i01 & 0xFFFF);
    uint32 i2 = (i23 >> 16);
    uint32 i3 = (i23 & 0xFFFF);

    const Vertex* v0 = gVertices + i0;
    const Vertex* v1 = gVertices + i1;
    const Vertex* v2 = gVertices + i2;
    const Vertex* v3 = gVertices + i3;

    if (checkBackface(v0, v1, v2) == !(flags & FACE_CCW))
        return;

    int32 depth = DEPTH_Q_AVG();

    Face* f = faceAdd(depth);
    f->ccb_PIXC = shade;

    const Texture* texture = level.textures + (flags & FACE_TEXTURE);
    ccbSetTexture(flags, f, texture);

    int32 x0 = v0->x;
    int32 y0 = v0->y;
    int32 x1 = v1->x;
    int32 y1 = v1->y;
    int32 x2 = v2->x;
    int32 y2 = v2->y;
    int32 x3 = v3->x;
    int32 y3 = v3->y;

    int32 ws = texture->wShift;
    int32 hs = texture->hShift;

    int32 hdx0 = (x1 - x0) << ws;
    int32 hdy0 = (y1 - y0) << ws;
    int32 hdx1 = (x2 - x3) << ws;
    int32 hdy1 = (y2 - y3) << ws;
    int32 vdx0 = (x3 - x0) << hs;
    int32 vdy0 = (y3 - y0) << hs;

    f->ccb_HDX = hdx0;
    f->ccb_HDY = hdy0;
    f->ccb_VDX = vdx0;
    f->ccb_VDY = vdy0;

    f->ccb_XPos = (x0 + (FRAME_WIDTH  >> 1)) << 16;
    f->ccb_YPos = (y0 + (FRAME_HEIGHT >> 1)) << 16;

    hs = 16 - hs;
    f->ccb_HDDX = (hdx1 - hdx0) >> hs;
    f->ccb_HDDY = (hdy1 - hdy0) >> hs;
}

X_INLINE void faceAddMeshTriangle(uint32 flags, const Index* indices, int32 startVertex32, uint32 shade)
{
    uint32 i01 = ((uint32*)indices)[0] + startVertex32;
    uint32 i23 = ((uint32*)indices)[1] + startVertex32;

    uint32 i0 = (i01 >> 16);
    uint32 i1 = (i01 & 0xFFFF);
    uint32 i2 = (i23 >> 16);

    const Vertex* v0 = gVertices + i0;
    const Vertex* v1 = gVertices + i1;
    const Vertex* v2 = gVertices + i2;

    if (checkBackface(v0, v1, v2))
        return;

    int32 depth = DEPTH_T_AVG();

    Face* f = faceAdd(depth);
    f->ccb_PIXC = shade;

    const Texture* texture = level.textures + (flags & FACE_TEXTURE);
    ccbSetTexture(flags, f, texture);

    int32 x0 = v0->x;
    int32 y0 = v0->y;
    int32 x1 = v1->x;
    int32 y1 = v1->y;
    int32 x2 = v2->x;
    int32 y2 = v2->y;

    int32 ws = texture->wShift;
    int32 hs = texture->hShift;

    int32 hdx0 = (x1 - x0) << ws;
    int32 hdy0 = (y1 - y0) << ws;
    int32 vdx0 = (x2 - x0) << hs;
    int32 vdy0 = (y2 - y0) << hs;

    f->ccb_HDX = hdx0;
    f->ccb_HDY = hdy0;
    f->ccb_VDX = vdx0;
    f->ccb_VDY = vdy0;

    f->ccb_XPos = (x0 + (FRAME_WIDTH  >> 1)) << 16;
    f->ccb_YPos = (y0 + (FRAME_HEIGHT >> 1)) << 16;

    hs = 16 - hs;
    f->ccb_HDDX = -hdx0 >> hs;
    f->ccb_HDDY = -hdy0 >> hs;
}

X_INLINE void faceAddMeshQuadFlat(uint32 flags, const Index* indices, int32 startVertex32, uint32 shade)
{
    uint32 i01 = startVertex32 + ((uint32*)indices)[0];
    uint32 i23 = startVertex32 + ((uint32*)indices)[1];

    uint32 i0 = (i01 >> 16);
    uint32 i1 = (i01 & 0xFFFF);
    uint32 i2 = (i23 >> 16);
    uint32 i3 = (i23 & 0xFFFF);

    const Vertex* v0 = gVertices + i0;
    const Vertex* v1 = gVertices + i1;
    const Vertex* v2 = gVertices + i2;
    const Vertex* v3 = gVertices + i3;

    if (checkBackface(v0, v1, v2))
        return;

    int32 depth = DEPTH_Q_AVG();

    Face* f = faceAdd(depth);
    f->ccb_PIXC = shade;

    ccbSetColor(flags, f);

    int32 x0 = v0->x;
    int32 y0 = v0->y;
    int32 x1 = v1->x;
    int32 y1 = v1->y;
    int32 x2 = v2->x;
    int32 y2 = v2->y;
    int32 x3 = v3->x;
    int32 y3 = v3->y;

    int32 hdx0 = (x1 - x0) << 20;
    int32 hdy0 = (y1 - y0) << 20;
    int32 hdx1 = (x2 - x3) << 20;
    int32 hdy1 = (y2 - y3) << 20;
    int32 vdx0 = (x3 - x0) << 16;
    int32 vdy0 = (y3 - y0) << 16;

    f->ccb_HDX = hdx0;
    f->ccb_HDY = hdy0;
    f->ccb_VDX = vdx0;
    f->ccb_VDY = vdy0;

    f->ccb_XPos = (x0 + (FRAME_WIDTH  >> 1)) << 16;
    f->ccb_YPos = (y0 + (FRAME_HEIGHT >> 1)) << 16;

    f->ccb_HDDX = (hdx1 - hdx0);
    f->ccb_HDDY = (hdy1 - hdy0);
}

X_INLINE void faceAddMeshTriangleFlat(uint32 flags, const Index* indices, int32 startVertex32, uint32 shade)
{
    uint32 i01 = ((uint32*)indices)[0] + startVertex32;
    uint32 i23 = ((uint32*)indices)[1] + startVertex32;

    uint32 i0 = (i01 >> 16);
    uint32 i1 = (i01 & 0xFFFF);
    uint32 i2 = (i23 >> 16);

    const Vertex* v0 = gVertices + i0;
    const Vertex* v1 = gVertices + i1;
    const Vertex* v2 = gVertices + i2;

    if (checkBackface(v0, v1, v2))
        return;

    int32 depth = DEPTH_T_AVG();

    Face* f = faceAdd(depth);
    f->ccb_PIXC = shade;

    ccbSetColor(flags, f);

    int32 x0 = v0->x;
    int32 y0 = v0->y;
    int32 x1 = v1->x;
    int32 y1 = v1->y;
    int32 x2 = v2->x;
    int32 y2 = v2->y;

    int32 hdx0 = (x1 - x0) << 20;
    int32 hdy0 = (y1 - y0) << 20;
    int32 vdx0 = (x2 - x0) << 16;
    int32 vdy0 = (y2 - y0) << 16;

    f->ccb_HDX = hdx0;
    f->ccb_HDY = hdy0;
    f->ccb_VDX = vdx0;
    f->ccb_VDY = vdy0;

    f->ccb_XPos = (x0 + (FRAME_WIDTH  >> 1)) << 16;
    f->ccb_YPos = (y0 + (FRAME_HEIGHT >> 1)) << 16;

    f->ccb_HDDX = -hdx0;
    f->ccb_HDDY = -hdy0;
}

void faceAddShadow(int32 x, int32 z, int32 sx, int32 sz)
{
    x <<= F16_SHIFT;
    z <<= F16_SHIFT;
    sx <<= F16_SHIFT;
    sz <<= F16_SHIFT;

    int32 sx2 = sx << 1;
    int32 sz2 = sz << 1;

    MeshVertex v[8] = { // y, x, _unused, z
        { 0, x - sx,  0, z + sz2 }, // 0
        { 0, x + sx,  0, z + sz2 }, // 1
        { 0, x + sx2, 0, z + sz  }, // 2
        { 0, x + sx2, 0, z - sz  }, // 3
        { 0, x + sx,  0, z - sz2 }, // 4
        { 0, x - sx,  0, z - sz2 }, // 5
        { 0, x - sx2, 0, z - sz  }, // 6
        { 0, x - sx2, 0, z + sz  }  // 7
    };

    int32 startVertex32 = gVerticesCount | (gVerticesCount << 16);

    transformMesh(v, 8, NULL, NULL);

    static const Index indices[] = {
        0, 1, 2, 7,
        7, 2, 3, 6,
        6, 3, 4, 5
    };

    faceAddMeshQuadFlat(0, indices + 0, startVertex32, SHADE_SHADOW);
    faceAddMeshQuadFlat(0, indices + 4, startVertex32, SHADE_SHADOW);
    faceAddMeshQuadFlat(0, indices + 8, startVertex32, SHADE_SHADOW);
}

void faceAddSprite(int32 vx, int32 vy, int32 vz, int32 vg, int32 index)
{
    if (gFacesCount >= MAX_FACES)
        return;

    const Matrix &m = matrixGet();

    vec3i v;
    v.x = (vx - cameraViewPos.x) << F16_SHIFT;
    v.y = (vy - cameraViewPos.y) << F16_SHIFT;
    v.z = (vz - cameraViewPos.z) << F16_SHIFT;

    MulVec3Mat33DivZ_F16(*(vec3f16*)&v, *(vec3f16*)&v, *(mat33f16*)&m, 1 << 8);

    if (v.z < (VIEW_MIN_F >> FIXED_SHIFT) || v.z >= (VIEW_MAX_F >> FIXED_SHIFT))
        return;

    const Sprite* sprite = level.sprites + index;

    int32 d = (1 << 20) / v.z;
    int32 x0 = sprite->l * d >> 12;
    int32 y0 = sprite->t * d >> 12;
    int32 x1 = sprite->r * d >> 12;
    int32 y1 = sprite->b * d >> 12;

    if (x0 == x1) return;
    if (y0 == y1) return;

    x0 += v.x;
    x1 += v.x;
    y0 += v.y;
    y1 += v.y;

    if (x0 >= viewportRel.x1) return;
    if (y0 >= viewportRel.y1) return;
    if (x1 <= viewportRel.x0) return;
    if (y1 <= viewportRel.y0) return;

    int32 depth = X_MAX(0, v.z - 128) >> OT_SHIFT; // depth hack

    Face* f = faceAdd(depth);

    if (depth > (FOG_MIN >> OT_SHIFT)) {
        vg += (depth - (FOG_MIN >> OT_SHIFT)) << 4;
        vg = X_MIN(vg, 8191);
    }

    vg >>= 8;
    f->ccb_PIXC = shadeTable[vg];

    Texture* texture = level.textures + sprite->texture;
    ccbSetTexture(0, f, texture);

    f->ccb_HDX = (x1 - x0) << texture->wShift;
    f->ccb_HDY = 0;
    f->ccb_VDX = 0;
    f->ccb_VDY = (y1 - y0) << texture->hShift;

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

    f->ccb_HDX = (sprite->r - sprite->l) << texture->wShift;
    f->ccb_HDY = 0;
    f->ccb_VDX = 0;
    f->ccb_VDY = (sprite->b - sprite->t) << texture->hShift;

    f->ccb_XPos = (vx + sprite->l) << 16;
    f->ccb_YPos = (vy + sprite->t) << 16;

    f->ccb_HDDX = 0;
    f->ccb_HDDY = 0;
}

void faceAddRoom(const Quad* quads, int32 qCount, const Triangle* triangles, int32 tCount, int32 startVertex)
{
    startVertex |= startVertex << 16;

    for (int32 i = 0; i < qCount; i++, quads++) {
        faceAddRoomQuad(quads->flags, quads->indices, startVertex);
    }

    for (int32 i = 0; i < tCount; i++, triangles++) {
        faceAddRoomTriangle(triangles->flags, triangles->indices, startVertex);
    }
}

void faceAddMesh(const Quad* rFaces, const Quad* crFaces, const Triangle* tFaces, const Triangle* ctFaces, int32 rCount, int32 crCount, int32 tCount, int32 ctCount, int32 startVertex)
{
    startVertex |= startVertex << 16;

    uint32 shade;
    if (lightAmbient > 4096) {
        shade = shadeTable[lightAmbient >> 8];
    } else {
        shade = SHADE_16;
    }

    for (int32 i = 0; i < rCount; i++) {
        faceAddMeshQuad(rFaces[i].flags, rFaces[i].indices, startVertex, shade);
    }

    for (int32 i = 0; i < tCount; i++) {
        faceAddMeshTriangle(tFaces[i].flags, tFaces[i].indices, startVertex, shade);
    }

    for (int32 i = 0; i < crCount; i++) {
        faceAddMeshQuadFlat(crFaces[i].flags, crFaces[i].indices, startVertex, shade);
    }

    for (int32 i = 0; i < ctCount; i++) {
        faceAddMeshTriangleFlat(ctFaces[i].flags, ctFaces[i].indices, startVertex, shade);
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
            DrawCels(screenItem, facesHead);
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
