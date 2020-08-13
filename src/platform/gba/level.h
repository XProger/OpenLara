#ifndef H_LEVEL
#define H_LEVEL

#include "common.h"
#include "camera.h"

// level file data -------------------
uint32           tilesCount;
const uint8*     tiles[15];

#if defined(USE_MODE_5) || defined(_WIN32)
    ALIGN4 uint16 palette[256];
#endif

ALIGN4 uint8     lightmap[256 * 32];

uint16           roomsCount;
const Room*      rooms;

uint32           texturesCount;
const Texture*   textures;

const Sprite*    sprites;

uint32           spritesSeqCount;
const SpriteSeq* spritesSeq;

const uint8*     meshData;
const uint32*    meshOffsets;

const int32*     nodes;
const Model*     models;
// -----------------------------------

struct RoomDesc {
    int32               x, z;
    uint16              vCount;
    uint16              qCount;
    uint16              tCount;
    uint16              pCount;
    const Room::Vertex* vertices;
    const Quad*         quads;
    const Triangle*     triangles;
    const Room::Portal* portals;
};

EWRAM_DATA RoomDesc roomDescs[64];

#define SEQ_GLYPH_ID    190
int32 seqGlyphs;

extern uint32 gVerticesCount;

void readLevel(const uint8 *data) { // TODO non-hardcode level loader
    tilesCount = *((uint32*)(data + 4));
    for (uint32 i = 0; i < tilesCount; i++) {
        tiles[i] = data + 8 + 256 * 256 * i;
    }

    roomsCount = *((uint16*)(data + 720908));
    rooms = (Room*)(data + 720908 + 2);

    texturesCount = *((uint32*)(data + 1271686));
    textures = (Texture*)(data + 1271686 + 4);

    sprites = (Sprite*)(data + 1289634);

    spritesSeqCount = *((uint32*)(data + 1292130));
    spritesSeq = (SpriteSeq*)(data + 1292130 + 4);

    meshData = data + 908172 + 4;
    meshOffsets = (uint32*)(data + 975724 + 4);

    nodes = (int32*)(data + 990318);

    models = (Model*)(data + 1270670);

// prepare lightmap
    const uint8* f_lightmap = data + 1320576;
    memcpy(lightmap, f_lightmap, sizeof(lightmap));

// prepare palette
#if !(defined(USE_MODE_5) || defined(_WIN32))
    uint16 palette[256];
#endif
    const uint8* f_palette = data + 1328768;

    const uint8* p = f_palette;

    for (int i = 0; i < 256; i++) {
        palette[i] = (p[0] >> 1) | ((p[1] >> 1) << 5) | ((p[2] >> 1) << 10);
        p += 3;
    }
   
#ifndef _WIN32
    #ifndef USE_MODE_5
        SetPalette(palette);
    #endif
#endif

// prepare glyphs
    for (uint32 i = 0; i < spritesSeqCount; i++) {
        if (spritesSeq[i].type == SEQ_GLYPH_ID) {
            seqGlyphs = i;
            break;
        }
    }

// prepare rooms
    uint8 *ptr = (uint8*)rooms;

    for (uint16 roomIndex = 0; roomIndex < roomsCount; roomIndex++) {
        const Room *room = (Room*)ptr;
        ptr += sizeof(Room);

        uint32 dataSize;
        memcpy(&dataSize, &room->dataSize, sizeof(dataSize));
        uint8* skipPtr = ptr + dataSize * 2;

        RoomDesc &desc = roomDescs[roomIndex];

        // offset
        memcpy(&desc.x, &room->info.x, sizeof(room->info.x));
        memcpy(&desc.z, &room->info.z, sizeof(room->info.z));

        // vertices
        desc.vCount = *((uint16*)ptr);
        ptr += 2;
        desc.vertices = (Room::Vertex*)ptr;
        ptr += sizeof(Room::Vertex) * desc.vCount;

        // quads
        desc.qCount = *((uint16*)ptr);
        ptr += 2;
        desc.quads = (Quad*)ptr;
        ptr += sizeof(Quad) * desc.qCount;

        // triangles
        desc.tCount = *((uint16*)ptr);
        ptr += 2;
        desc.triangles = (Triangle*)ptr;
        ptr += sizeof(Triangle) * desc.tCount;

        ptr = skipPtr;

        // portals
        desc.pCount = *((uint16*)ptr);
        ptr += 2;
        desc.portals = (Room::Portal*)ptr;
        ptr += sizeof(Room::Portal) * desc.pCount;

        uint16 zSectors = *((uint16*)ptr);
        ptr += 2;
        uint16 xSectors = *((uint16*)ptr);
        ptr += 2;
        //sectors = (Room::Sector*)sectors;
        ptr += sizeof(Room::Sector) * zSectors * xSectors;

        //ambient = *((uint16*)ptr);
        ptr += 2;

        uint16 lightsCount = *((uint16*)ptr);
        ptr += 2;
        //lights = (Room::Light*)ptr;
        ptr += sizeof(Room::Light) * lightsCount;

        uint16 meshesCount = *((uint16*)ptr);
        ptr += 2;
        //meshes = (Room::Mesh*)ptr;
        ptr += sizeof(Room::Mesh) * meshesCount;

        ptr += 2 + 2; // skip alternateRoom and flags
    }
}

void drawRoom(int16 roomIndex) {
    RoomDesc &room = roomDescs[roomIndex];

    int32 dx = -camX + room.x;
    int32 dy = -camY;
    int32 dz = -camZ + room.z;

    int32 startVertex = gVerticesCount;

    const Room::Vertex* vertices = room.vertices;
    for (uint16 i = 0; i < room.vCount; i++) {
        const Room::Vertex &v = vertices[i];
        transform(v.x, v.y, v.z, v.lighting, dx, dy, dz);
    }

    const Quad* quads = room.quads;
    for (uint16 i = 0; i < room.qCount; i++) {
        faceAddQuad(quads[i].flags, quads[i].indices, startVertex);
    }

    const Triangle* triangles = room.triangles;
    for (uint16 i = 0; i < room.tCount; i++) {
        faceAddTriangle(triangles[i].flags, triangles[i].indices, startVertex);
    }
}

void drawMesh(int16 meshIndex, int32 x, int32 y, int32 z) {
    uint32 offset = meshOffsets[meshIndex];
    const uint8* ptr = meshData + offset;

    ptr += 2 * 5; // skip [cx, cy, cz, radius, flags]

    int16 vCount = *(int16*)ptr; ptr += 2;
    const int16* vertices = (int16*)ptr;
    ptr += vCount * 3 * sizeof(int16);

    int16 nCount = *(int16*)ptr; ptr += 2;
    //const int16* normals = (int16*)ptr;
    if (nCount > 0) { // normals
        ptr += nCount * 3 * sizeof(int16);
    } else { // intensity
        ptr += vCount * sizeof(int16);
    }

    int16     rCount = *(int16*)ptr; ptr += 2;
    Quad*     rFaces = (Quad*)ptr; ptr += rCount * sizeof(Quad);

    int16     tCount = *(int16*)ptr; ptr += 2;
    Triangle* tFaces = (Triangle*)ptr; ptr += tCount * sizeof(Triangle);

    int16     crCount = *(int16*)ptr; ptr += 2;
    Quad*     crFaces = (Quad*)ptr; ptr += crCount * sizeof(Quad);

    int16     ctCount = *(int16*)ptr; ptr += 2;
    Triangle* ctFaces = (Triangle*)ptr; ptr += ctCount * sizeof(Triangle);

    int32 startVertex = gVerticesCount;

    int32 dx = x - camX;
    int32 dy = y - camY;
    int32 dz = z - camZ;

    const int16* v = vertices;
    for (uint16 i = 0; i < vCount; i++) {
        transform(v[0], v[1], v[2], 4096, dx, dy, dz);
        v += 3;
    }

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

void drawModel(int32 modelIndex, int32 x, int32 y, int32 z) {
    const Model* model = models + modelIndex;

    // non-aligned access
    uint32 node, frame;
    memcpy(&node,  &model->node,  sizeof(node));
    memcpy(&frame, &model->frame, sizeof(frame));

    Node bones[32];
    memcpy(bones, nodes + node, (model->mCount - 1) * sizeof(Node));

    const Node* n = bones;

    struct StackItem {
        int32 x, y, z;
    } stack[4];
    StackItem *s = stack;

    drawMesh(model->mStart, x, y, z);

    for (int i = 1; i < model->mCount; i++) {
        if (n->flags & 1) {
            s--;
            x = s->x;
            y = s->y;
            z = s->z;
        }

        if (n->flags & 2) {
            s->x = x;
            s->y = y;
            s->z = z;
            s++;
        }

        x += n->x;
        y += n->y;
        z += n->z;
        n++;

        drawMesh(model->mStart + i, x, y, z);
    }
}

void drawNumber(int32 number, int32 x, int32 y) {
    const int32 widths[] = { 12, 8, 10, 10, 10, 10, 10, 10, 10, 10 };

    const Sprite *glyphSprites = sprites + spritesSeq[seqGlyphs].sStart;

    while (number > 0) {
        x -= widths[number % 10];
        drawGlyph(glyphSprites + 52 + (number % 10), x, y);
        number /= 10;
    }
}

#endif
