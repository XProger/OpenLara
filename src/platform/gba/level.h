#ifndef H_LEVEL
#define H_LEVEL

#include "common.h"
#include "camera.h"

// level file data -------------------
uint32              tilesCount;
extern const uint8* tiles[15];

#if defined(USE_MODE_5) || defined(_WIN32)
    ALIGN4 uint16 palette[256];
#endif

extern uint8     lightmap[256 * 32];

uint16           roomsCount;

const uint16*    floors;

uint32                texturesCount;
extern const Texture* textures;

const Sprite*    sprites;

uint32           spritesSeqCount;
const SpriteSeq* spritesSeq;

const uint8*     meshData;
const uint32*    meshOffsets;

const int32*     nodes;

uint32           modelsCount;
EWRAM_DATA Model models[MAX_MODELS];
EWRAM_DATA int16 modelsMap[MAX_ENTITY];

uint32           entitiesCount;
const Entity*    entities;
// -----------------------------------

struct RoomDesc {
    Rect                clip;
    bool                visible;
    int32               x, z;
    uint16              vCount;
    uint16              qCount;
    uint16              tCount;
    uint16              pCount;
    uint16              zSectors;
    uint16              xSectors;
    const Room::Vertex* vertices;
    const Quad*         quads;
    const Triangle*     triangles;
    const Room::Portal* portals;
    const Room::Sector* sectors;

    INLINE void reset() {
        visible = false;
        clip = { FRAME_WIDTH, FRAME_HEIGHT, 0, 0 };
    }
};

EWRAM_DATA RoomDesc rooms[64];

int32 visRoomsCount;
int32 visRooms[16];

#define ROOM_VISIBLE (1 << 15)

#define ENTITY_LARA  0

#define SEQ_GLYPH    190

enum FloorType {
    FLOOR_TYPE_NONE,
    FLOOR_TYPE_PORTAL,
    FLOOR_TYPE_FLOOR,
    FLOOR_TYPE_CEILING,
};

int32 seqGlyphs;
int32 entityLara;

extern uint32 gVerticesCount;
extern Rect   clip;

void readLevel(const uint8 *data) { // TODO non-hardcode level loader, added *_OFF alignment bytes
    tilesCount = *((uint32*)(data + 4));
    for (uint32 i = 0; i < tilesCount; i++) {
        tiles[i] = data + 8 + 256 * 256 * i;
    }

    #define MDL_OFF 2
    #define ENT_OFF 2

    roomsCount = *((uint16*)(data + 720908));
    const Room* roomsPtr = (Room*)(data + 720908 + 2);

    floors = (uint16*)(data + 899492 + 4);

    meshData = data + 908172 + 4;
    meshOffsets = (uint32*)(data + 975724 + 4);

    nodes = (int32*)(data + 990318);

    modelsCount = *((uint32*)(data + 1270666 + MDL_OFF));
    const uint8* modelsPtr = (uint8*)(data + 1270666 + 4 + MDL_OFF);

    texturesCount = *((uint32*)(data + 1271686 + MDL_OFF));
    textures = (Texture*)(data + 1271686 + 4 + MDL_OFF);

    sprites = (Sprite*)(data + 1289634 + MDL_OFF);

    spritesSeqCount = *((uint32*)(data + 1292130 + MDL_OFF));
    spritesSeq = (SpriteSeq*)(data + 1292130 + 4 + MDL_OFF);

    entitiesCount = *((uint32*)(data + 1319252 + MDL_OFF + ENT_OFF));
    entities = (Entity*)(data + 1319252 + 4 + MDL_OFF + ENT_OFF);

// prepare lightmap
    const uint8* f_lightmap = data + 1320576 + MDL_OFF + ENT_OFF;
    memcpy(lightmap, f_lightmap, sizeof(lightmap));

// prepare palette
#if !(defined(USE_MODE_5) || defined(_WIN32))
    uint16 palette[256];
#endif
    const uint8* f_palette = data + 1328768 + MDL_OFF + ENT_OFF;

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
// prepare models
    for (uint32 i = 0; i < modelsCount; i++) {
        dmaCopy(modelsPtr, models + i, sizeof(Model)); // sizeof(Model) is faster than FILE_MODEL_SIZE
        modelsPtr += FILE_MODEL_SIZE;
        modelsMap[models[i].type] = i;
    }

// prepare entities
    for (uint32 i = 0; i < entitiesCount; i++) {
        if (entities[i].type == ENTITY_LARA) {
            entityLara = i;
            break;
        }
    }

// prepare glyphs
    for (uint32 i = 0; i < spritesSeqCount; i++) {
        if (spritesSeq[i].type == SEQ_GLYPH) {
            seqGlyphs = i;
            break;
        }
    }

// prepare rooms
    uint8 *ptr = (uint8*)roomsPtr;

    for (uint16 roomIndex = 0; roomIndex < roomsCount; roomIndex++) {
        const Room *room = (Room*)ptr;
        ptr += sizeof(Room);

        uint32 dataSize;
        memcpy(&dataSize, &room->dataSize, sizeof(dataSize));
        uint8* skipPtr = ptr + dataSize * 2;

        RoomDesc &desc = rooms[roomIndex];
        desc.reset();

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

        desc.zSectors = *((uint16*)ptr);
        ptr += 2;
        desc.xSectors = *((uint16*)ptr);
        ptr += 2;
        desc.sectors = (Room::Sector*)ptr;
        ptr += sizeof(Room::Sector) * desc.zSectors * desc.xSectors;

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

    camera.init();
    camera.room = entities[entityLara].room;
}

void drawMesh(int16 meshIndex) {
    uint32 offset = meshOffsets[meshIndex];
    const uint8* ptr = meshData + offset;

    ptr += 2 * 5; // skip [cx, cy, cz, radius, flags]

    int16 vCount = *(int16*)ptr; ptr += 2;
    const vec3s* vertices = (vec3s*)ptr;
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

    for (uint16 i = 0; i < vCount; i++) {
        transform(*vertices++, 4096);
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

void drawModel(int32 modelIndex) {
    const Model* model = models + modelIndex;

    // non-aligned access
    uint32 node, frame;
    memcpy(&node,  &model->node,  sizeof(node));
    memcpy(&frame, &model->frame, sizeof(frame));

    Node bones[32];
    memcpy(bones, nodes + node, (model->mCount - 1) * sizeof(Node));

    const Node* n = bones;

    drawMesh(model->mStart);

    for (int i = 1; i < model->mCount; i++) {
        if (n->flags & 1) {
            matrixPop();
        }

        if (n->flags & 2) {
            matrixPush();
        }

        matrixTranslate(n->pos);
        n++;

        drawMesh(model->mStart + i);
    }
}

void drawEntity(int32 entityIndex) {
    const Entity &e = entities[entityIndex];

    matrixPush();
    matrixTranslateAbs(vec3i(e.pos.x, e.pos.y - 512, e.pos.z)); // TODO animation

    drawModel(modelsMap[e.type]);

    matrixPop();
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

void drawRoom(int16 roomIndex) {
    RoomDesc &room = rooms[roomIndex];

    clip = room.clip;

    int32 startVertex = gVerticesCount;

    matrixPush();
    matrixTranslateAbs(vec3i(room.x, 0, room.z));

    const Room::Vertex* vertex = room.vertices;
    for (uint16 i = 0; i < room.vCount; i++) {
        transform(vertex->pos, vertex->lighting);
        vertex++;
    }

    matrixPop();

    const Quad* quads = room.quads;
    for (uint16 i = 0; i < room.qCount; i++) {
        faceAddQuad(quads[i].flags, quads[i].indices, startVertex);
    }

    const Triangle* triangles = room.triangles;
    for (uint16 i = 0; i < room.tCount; i++) {
        faceAddTriangle(triangles[i].flags, triangles[i].indices, startVertex);
    }

    if (roomIndex == entityLara) { // TODO draw all entities in the room
        drawEntity(entityLara);
    }

    room.reset();

    flush();
}

const Room::Sector* getSector(int32 roomIndex, int32 x, int32 z) {
    RoomDesc &room = rooms[roomIndex];

    int32 sx = clamp((x - room.x) >> 10, 0, room.xSectors);
    int32 sz = clamp((z - room.z) >> 10, 0, room.zSectors);

    return room.sectors + sx * room.zSectors + sz;
}

int32 getRoomIndex(int32 roomIndex, const vec3i &pos) {
    const Room::Sector *sector = getSector(roomIndex, pos.x, pos.z);

    if (sector->floorIndex) {
        const uint16 *data = floors + sector->floorIndex;
        int16 type = *data++;

        if (type == FLOOR_TYPE_FLOOR) {
            data++;
            type = *data++;
        }

        if (type == FLOOR_TYPE_CEILING) {
            data++;
            type = *data++;
        }

        if ((type & 0xFF) == FLOOR_TYPE_PORTAL) {
            roomIndex = *data;
        }
    }

    while (sector->roomAbove != 0xFF && pos.y < (sector->ceiling << 8)) {
        roomIndex = sector->roomAbove;
        sector = getSector(roomIndex, pos.x, pos.z);
    }

    while (sector->roomBelow != 0xFF && pos.y >= (sector->floor << 8)) {
        roomIndex = sector->roomBelow;
        sector = getSector(roomIndex, pos.x, pos.z);
    }

    return roomIndex;
}

bool checkPortal(int32 roomIndex, const Room::Portal &portal) {
    RoomDesc &room = rooms[roomIndex];

    vec3i d;
    d.x = portal.v[0].x - camera.pos.x + room.x;
    d.y = portal.v[0].y - camera.pos.y;
    d.z = portal.v[0].z - camera.pos.z + room.z;

    if (DP33(portal.n, d) >= 0) {
        return false;
    }

    int32 x0 = room.clip.x1;
    int32 y0 = room.clip.y1;
    int32 x1 = room.clip.x0;
    int32 y1 = room.clip.y0;

    int32 znear = 0, zfar = 0;

    Matrix &m = matrixGet();

    vec3i  pv[4];

    for (int32 i = 0; i < 4; i++) {
        const vec3s &v = portal.v[i];

        int32 x = DP43(m[0], v);
        int32 y = DP43(m[1], v);
        int32 z = DP43(m[2], v);

        pv[i].x = x;
        pv[i].y = y;
        pv[i].z = z;

        if (z <= VIEW_MIN_F) {
            znear++;
            continue;
        }

        if (z >= VIEW_MAX_F) {
            zfar++;
        }

        if (z != 0) {
            z >>= FOV_SHIFT;
            x = (x / z) + (FRAME_WIDTH  / 2);
            y = (y / z) + (FRAME_HEIGHT / 2);
        } else {
            x = (x > 0) ? clip.x1 : clip.x0;
            y = (y > 0) ? clip.y1 : clip.y0;
        }

        if (x < x0) x0 = x;
        if (x > x1) x1 = x;
        if (y < y0) y0 = y;
        if (y > y1) y1 = y;
    }

    if (znear == 4 || zfar == 4) return false;

    if (znear) {
        vec3i *a = pv;
        vec3i *b = pv + 3;
        for (int32 i = 0; i < 4; i++) {
            if ((a->z < 0) ^ (b->z < 0)) {
                if (a->x < 0 && b->x < 0) {
                    x0 = 0;
                } else if (a->x > 0 && b->x > 0) {
                    x1 = FRAME_WIDTH;
                } else {
                    x0 = 0;
                    x1 = FRAME_WIDTH;
                }

                if (a->y < 0 && b->y < 0) {
                    y0 = 0;
                } else if (a->y > 0 && b->y > 0) {
                    y1 = FRAME_HEIGHT;
                } else {
                    y0 = 0;
                    y1 = FRAME_HEIGHT;
                }
            }
            b = a;
            a++;
        }
    }

    if (x0 < room.clip.x0) x0 = room.clip.x0;
    if (x1 > room.clip.x1) x1 = room.clip.x1;
    if (y0 < room.clip.y0) y0 = room.clip.y0;
    if (y1 > room.clip.y1) y1 = room.clip.y1;

    if (x0 >= x1 || y0 >= y1) return false;

    RoomDesc &nextRoom = rooms[portal.roomIndex];

    if (x0 < nextRoom.clip.x0) nextRoom.clip.x0 = x0;
    if (x1 > nextRoom.clip.x1) nextRoom.clip.x1 = x1;
    if (y0 < nextRoom.clip.y0) nextRoom.clip.y0 = y0;
    if (y1 > nextRoom.clip.y1) nextRoom.clip.y1 = y1;

    if (!nextRoom.visible) {
        nextRoom.visible = true;
        visRooms[visRoomsCount++] = portal.roomIndex;
    }

    return true;
}

void getVisibleRooms(int32 roomIndex) {
    RoomDesc &room = rooms[roomIndex];

    matrixPush();
    matrixTranslateAbs(vec3i(room.x, 0, room.z));

    for (int32 i = 0; i < room.pCount; i++) {
        const Room::Portal &portal = room.portals[i];
        if (checkPortal(roomIndex, portal)) {
            getVisibleRooms(portal.roomIndex);
        }
    }

    matrixPop();
}

void drawRooms() {
    rooms[camera.room].clip = { 0, 0, FRAME_WIDTH, FRAME_HEIGHT };
    visRoomsCount = 0;
    visRooms[visRoomsCount++] = camera.room;

    getVisibleRooms(camera.room);

    while (visRoomsCount--) {
        drawRoom(visRooms[visRoomsCount]);
    }
}

#endif
