#ifndef H_LEVEL
#define H_LEVEL

#include "common.h"
#include "camera.h"

#define GRAVITY    6

// level file data -------------------
int32               tilesCount;
extern const uint8* tiles;

extern uint16    palette[256];
extern uint8     lightmap[256 * 32];
const uint16*    floors;

int32                 texturesCount;
extern const Texture* textures;

const Sprite*    sprites;

int32            spritesSeqCount;
const SpriteSeq* spritesSeq;

const uint8*     meshData;
const int32*     meshOffsets;

const int32*     nodesPtr;

int32            animsCount;
const Anim*      anims;

int32            framesCount;
const uint16*    frames;

int32            modelsCount;
EWRAM_DATA Model models[MAX_MODELS];
EWRAM_DATA uint8 modelsMap[MAX_ITEMS];
EWRAM_DATA uint8 staticMeshesMap[MAX_MESHES];

int32             staticMeshesCount;
const StaticMesh* staticMeshes;

int32           itemsCount;
EWRAM_DATA Item items[MAX_ITEMS];
// -----------------------------------

struct Room {
    Rect                clip;
    uint8               firstItem;
    bool                visible;

    // TODO leave in ROM
    int32               x, z;
    uint16              vCount;
    uint16              qCount;
    uint16              tCount;
    uint16              pCount;
    uint16              lCount;
    uint16              mCount;
    uint16              zSectors;
    uint16              xSectors;
    uint16              ambient;

    const RoomInfo::Vertex* vertices;
    const Quad*             quads;
    const Triangle*         triangles;
    const RoomInfo::Portal* portals;
    const RoomInfo::Sector* sectors;
    const RoomInfo::Light*  lights;
    const RoomInfo::Mesh*   meshes;
};

int16           roomsCount;
EWRAM_DATA Room rooms[64];

int32 firstActive = NO_ITEM;

int32 visRoomsCount;
int32 visRooms[16];

#define ROOM_VISIBLE (1 << 15)
#define SEQ_GLYPH    190

enum FloorType {
    FLOOR_TYPE_NONE,
    FLOOR_TYPE_PORTAL,
    FLOOR_TYPE_FLOOR,
    FLOOR_TYPE_CEILING,
};

int32 seqGlyphs;

extern uint32 gVerticesCount;
extern Rect   clip;

void roomReset(int32 roomIndex)
{
    Room* room = rooms + roomIndex;

    room->visible = false;
    room->clip = { FRAME_WIDTH, FRAME_HEIGHT, 0, 0 };
}

void roomItemAdd(int32 roomIndex, int32 itemIndex)
{
    ASSERT(items[itemIndex].nextItem == NO_ITEM);

    Room* room = rooms + roomIndex;

    items[itemIndex].nextItem = room->firstItem;
    room->firstItem = itemIndex;
}

void roomItemRemove(int32 roomIndex, int32 itemIndex)
{
    Room* room = rooms + roomIndex;

    int32 prevIndex = NO_ITEM;
    int32 index = room->firstItem;

    while (index != NO_ITEM)
    {
        int32 next = items[index].nextItem;

        if (index == itemIndex)
        {
            items[index].nextItem = NO_ITEM;

            if (prevIndex == NO_ITEM) {
                room->firstItem = next;
            } else {
                items[prevIndex].nextItem = next;
            }

            break;
        }

        prevIndex = index;
        index = next;
    }
}

void activateItem(int32 itemIndex)
{
    items[itemIndex].nextActive = firstActive;
    firstActive = itemIndex;
}

void deactivateItem(int32 itemIndex)
{
    int32 prevIndex = NO_ITEM;
    int32 index = firstActive;

    while (index != NO_ITEM)
    {
        int32 next = items[index].nextActive;

        if (index == itemIndex)
        {
            items[index].nextItem = NO_ITEM;

            if (prevIndex == NO_ITEM) {
                firstActive = next;
            } else {
                items[prevIndex].nextActive = next;
            }

            break;
        }

        prevIndex = index;
        index = next;
    }
}

void readLevel(const uint8 *data) { // TODO non-hardcode level loader, added *_OFF alignment bytes
    tilesCount = *((int32*)(data + 4));
    tiles = data + 8;

    #define MDL_OFF 2
    #define ITM_OFF 2

    roomsCount = *((int16*)(data + 720908));
    const RoomInfo* roomsPtr = (RoomInfo*)(data + 720908 + 2);

    floors = (uint16*)(data + 899492 + 4);

    meshData = data + 908172 + 4;
    meshOffsets = (int32*)(data + 975724 + 4);

    animsCount = *((int32*)(data + 976596));
    anims = (Anim*)(data + 976596 + 4);
    ASSERT((intptr_t)anims % 4 == 0);

    framesCount = *((int32*)(data + 992990));
    frames = (uint16*)(data + 992990 + 4);
    ASSERT((intptr_t)frames % 2 == 0);

    nodesPtr = (int32*)(data + 990318);

    modelsCount = *((int32*)(data + 1270666 + MDL_OFF));
    const uint8* modelsPtr = (uint8*)(data + 1270666 + 4 + MDL_OFF);
    ASSERT((intptr_t)modelsPtr % 4 == 0);

    staticMeshesCount = *((int32*)(data + 1271426 + MDL_OFF));
    staticMeshes = (StaticMesh*)(data + 1271426 + 4 + MDL_OFF);
    ASSERT((intptr_t)staticMeshes % 4 == 0);

    texturesCount = *((int32*)(data + 1271686 + MDL_OFF));
    textures = (Texture*)(data + 1271686 + 4 + MDL_OFF);

    sprites = (Sprite*)(data + 1289634 + MDL_OFF);

    spritesSeqCount = *((int32*)(data + 1292130 + MDL_OFF));
    spritesSeq = (SpriteSeq*)(data + 1292130 + 4 + MDL_OFF);

    itemsCount = *((int32*)(data + 1319252 + MDL_OFF + ITM_OFF));
    const uint8* itemsPtr = (data + 1319252 + 4 + MDL_OFF + ITM_OFF);

    for (int32 i = 0; i < itemsCount; i++) {
        memcpy(items + i, itemsPtr, FILE_ITEM_SIZE);
        itemsPtr += FILE_ITEM_SIZE;
    }

// prepare lightmap
    const uint8* f_lightmap = data + 1320576 + MDL_OFF + ITM_OFF;
    memcpy(lightmap, f_lightmap, sizeof(lightmap));

// prepare palette
    const uint8* f_palette = data + 1328768 + MDL_OFF + ITM_OFF;

    const uint8* p = f_palette;

    for (int i = 0; i < 256; i++)
    {
    #if defined(_WIN32) || defined(__GBA__)
        palette[i] = (p[0] >> 1) | ((p[1] >> 1) << 5) | ((p[2] >> 1) << 10);
    #elif defined(__TNS__)
        palette[i] = (p[2] >> 1) | ((p[1] >> 1) << 5) | ((p[0] >> 1) << 10);
    #endif
        p += 3;
    }

// prepare rooms
    uint8 *ptr = (uint8*)roomsPtr;

    for (int32 roomIndex = 0; roomIndex < roomsCount; roomIndex++)
    {
        const RoomInfo *room = (RoomInfo*)ptr;
        ptr += sizeof(RoomInfo);

        uint32 dataSize;
        memcpy(&dataSize, &room->dataSize, sizeof(dataSize));
        uint8* skipPtr = ptr + dataSize * 2;

        Room &desc = rooms[roomIndex];
        roomReset(roomIndex);

        desc.firstItem = NO_ITEM;

        // offset
        memcpy(&desc.x, &room->x, sizeof(room->x));
        memcpy(&desc.z, &room->z, sizeof(room->z));

        // vertices
        desc.vCount = *((uint16*)ptr);
        ptr += 2;
        desc.vertices = (RoomInfo::Vertex*)ptr;
        ptr += sizeof(RoomInfo::Vertex) * desc.vCount;

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
        desc.portals = (RoomInfo::Portal*)ptr;
        ptr += sizeof(RoomInfo::Portal) * desc.pCount;

        desc.zSectors = *((uint16*)ptr);
        ptr += 2;
        desc.xSectors = *((uint16*)ptr);
        ptr += 2;
        desc.sectors = (RoomInfo::Sector*)ptr;
        ptr += sizeof(RoomInfo::Sector) * desc.zSectors * desc.xSectors;

        desc.ambient = *((uint16*)ptr);
        ptr += 2;

        desc.lCount = *((uint16*)ptr);
        ptr += 2;
        desc.lights = (RoomInfo::Light*)ptr;
        ptr += sizeof(RoomInfo::Light) * desc.lCount;

        desc.mCount = *((uint16*)ptr);
        ptr += 2;
        desc.meshes = (RoomInfo::Mesh*)ptr;
        ptr += sizeof(RoomInfo::Mesh) * desc.mCount;

        ptr += 2 + 2; // skip alternateRoom and flags
    }

// prepare models
    memset(modelsMap, 0xFF, sizeof(modelsMap));
    for (int32 i = 0; i < modelsCount; i++)
    {
        memcpy(models + i, modelsPtr, sizeof(Model)); // sizeof(Model) is faster than FILE_MODEL_SIZE
        modelsPtr += FILE_MODEL_SIZE;
        modelsMap[models[i].type] = i;
    }

// prepare static meshes
    memset(staticMeshesMap, 0xFF, sizeof(staticMeshesMap));
    for (int32 i = 0; i < staticMeshesCount; i++)
    {
        staticMeshesMap[staticMeshes[i].id] = i;
    }

// prepare items
    for (int32 i = 0; i < itemsCount; i++) {
        Item* item = items + i;

        item->angleX      = 0;
        item->angleZ      = 0;
        item->vSpeed      = 0;
        item->hSpeed      = 0;
        item->nextItem    = NO_ITEM;
        item->nextActive  = NO_ITEM;
        item->animIndex   = models[modelsMap[item->type]].animIndex;
        item->frameIndex  = anims[item->animIndex].frameBegin;
        item->state       = anims[item->animIndex].state;
        item->nextState   = item->state;
        item->goalState   = item->state;
        item->intensity   = 4096; // TODO lighting

        item->flags.gravity = 0; 

        if (item->room > -1) {
            roomItemAdd(item->room, i);
        }

        if (item->type == ITEM_LARA) {
            activateItem(i);
        }

        // TODO remove
        if (item->type == ITEM_WOLF ||
            item->type == ITEM_BEAR ||
            item->type == ITEM_BAT  ||
            item->type == ITEM_CRYSTAL)
        {
            activateItem(i);
        }
    }

// prepare glyphs
    for (int32 i = 0; i < spritesSeqCount; i++) {
        if (spritesSeq[i].type == SEQ_GLYPH) {
            seqGlyphs = i;
            break;
        }
    }

    camera.init();
    camera.room = 0;
}

void drawMesh(int16 meshIndex, uint16 intensity) {
    int32 offset = meshOffsets[meshIndex];
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

    PROFILE_START();
    transformMesh(vertices, vCount, intensity);
    PROFILE_STOP(dbg_transform);

    PROFILE_START();
    faceAddMesh(rFaces, crFaces, tFaces, ctFaces, rCount, crCount, tCount, ctCount, startVertex);
    PROFILE_STOP(dbg_poly);
}

Frame* getFrame(const Item* item, const Model* model)
{
    const Anim* anim = anims + item->animIndex;

    int32 frameSize = sizeof(Frame) / 2 + model->mCount * 2;
    int32 frameIndex = (item->frameIndex - anim->frameBegin) / anim->frameRate;//* FixedInvU(anim->frameRate) >> 16;

    return (Frame*)(frames + anim->frameOffset / 2 + frameIndex * frameSize);
}

void drawItem(const Item* item) {
    int32 modelIndex = modelsMap[item->type];
    if (modelIndex == NO_MODEL) {
        return; // TODO sprite items
    }

    const Model* model = models + modelIndex;

    if (model->mCount == 1 && meshOffsets[model->mStart] == 0) return;

    Frame* frame = getFrame(item, model);
    uint16* frameAngles = frame->angles + 1;

    matrixPush();
    matrixTranslateAbs(item->pos.x, item->pos.y, item->pos.z);
    matrixRotateYXZ(item->angleX, item->angleY, item->angleZ);

    if (boxIsVisible(&frame->box)) {
        // non-aligned access (TODO)
        uint32 nodeIndex;
        memcpy(&nodeIndex, &model->nodeIndex,  sizeof(nodeIndex));
        Node nodes[32];
        memcpy(nodes, nodesPtr + nodeIndex, (model->mCount - 1) * sizeof(Node));

        const Node* node = nodes;

        matrixFrame(frame->pos.x, frame->pos.y, frame->pos.z, frameAngles);

        drawMesh(model->mStart, item->intensity);

        for (int32 i = 1; i < model->mCount; i++)
        {
            if (node->flags & 1) matrixPop();
            if (node->flags & 2) matrixPush();

            frameAngles += 2;
            matrixFrame(node->pos.x, node->pos.y, node->pos.z, frameAngles);

            drawMesh(model->mStart + i, item->intensity);

            node++;
        }
    }

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

void drawRoom(int32 roomIndex) {
    const Room* room = rooms + roomIndex;

    clip = room->clip;

    int32 startVertex = gVerticesCount;

    matrixPush();
    matrixTranslateAbs(room->x, 0, room->z);

    PROFILE_START();
    transformRoom(room->vertices, room->vCount);
    PROFILE_STOP(dbg_transform);

    matrixPop();

    PROFILE_START();
    faceAddRoom(room->quads, room->qCount, room->triangles, room->tCount, startVertex);

    for (int32 i = 0; i < room->mCount; i++)
    {
        const RoomInfo::Mesh* mesh = room->meshes + i;
        const StaticMesh* staticMesh = staticMeshes + staticMeshesMap[mesh->staticMeshId];

        if (!(staticMesh->flags & 2)) continue; // invisible
        
        // TODO align RoomInfo::Mesh (room relative int16?)
        vec3i pos;
        memcpy(&pos, &mesh->pos, sizeof(pos));

        matrixPush();
        matrixTranslateAbs(pos.x, pos.y, pos.z);
        matrixRotateY(mesh->rotation);

        if (boxIsVisible(&staticMesh->vbox)) {
            drawMesh(staticMesh->meshIndex, mesh->intensity);
        }

        matrixPop();
    }

    int32 itemIndex = room->firstItem;
    while (itemIndex != NO_ITEM)
    {
        drawItem(items + itemIndex);
        itemIndex = items[itemIndex].nextItem;
    }
    PROFILE_STOP(dbg_poly);

    roomReset(roomIndex);

    flush();
}

const RoomInfo::Sector* getSector(int32 roomIndex, int32 x, int32 z) {
    Room &room = rooms[roomIndex];

    int32 sx = clamp((x - room.x) >> 10, 0, room.xSectors - 1);
    int32 sz = clamp((z - room.z) >> 10, 0, room.zSectors - 1);

    return room.sectors + sx * room.zSectors + sz;
}

int32 getRoomIndex(int32 roomIndex, const vec3i* pos) {
    const RoomInfo::Sector *sector = getSector(roomIndex, pos->x, pos->z);

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

    while (sector->roomAbove != NO_ROOM && pos->y < (sector->ceiling << 8)) {
        roomIndex = sector->roomAbove;
        sector = getSector(roomIndex, pos->x, pos->z);
    }

    while (sector->roomBelow != 0xFF && pos->y >= (sector->floor << 8)) {
        roomIndex = sector->roomBelow;
        sector = getSector(roomIndex, pos->x, pos->z);
    }

    return roomIndex;
}

bool checkPortal(int32 roomIndex, const RoomInfo::Portal* portal) {
    Room &room = rooms[roomIndex];

    vec3i d;
    d.x = portal->v[0].x - camera.pos.x + room.x;
    d.y = portal->v[0].y - camera.pos.y;
    d.z = portal->v[0].z - camera.pos.z + room.z;

    if (DP33(portal->n, d) >= 0) {
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
        const vec3s &v = portal->v[i];

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

    Room &nextRoom = rooms[portal->roomIndex];

    if (x0 < nextRoom.clip.x0) nextRoom.clip.x0 = x0;
    if (x1 > nextRoom.clip.x1) nextRoom.clip.x1 = x1;
    if (y0 < nextRoom.clip.y0) nextRoom.clip.y0 = y0;
    if (y1 > nextRoom.clip.y1) nextRoom.clip.y1 = y1;

    if (!nextRoom.visible) {
        nextRoom.visible = true;
        visRooms[visRoomsCount++] = portal->roomIndex;
    }

    return true;
}

void getVisibleRooms(int32 roomIndex)
{
    const Room* room = rooms + roomIndex;

    matrixPush();
    matrixTranslateAbs(room->x, 0, room->z);

    for (int32 i = 0; i < room->pCount; i++)
    {
        const RoomInfo::Portal* portal = room->portals + i;

        if (checkPortal(roomIndex, portal))
        {
            getVisibleRooms(portal->roomIndex);
        }
    }

    matrixPop();
}

void drawRooms()
{
    rooms[camera.room].clip = { 0, 0, FRAME_WIDTH, FRAME_HEIGHT };
    visRoomsCount = 0;
    visRooms[visRoomsCount++] = camera.room;

    getVisibleRooms(camera.room);

    while (visRoomsCount--)
    {
        drawRoom(visRooms[visRoomsCount]);
    }
}

void move(Item* item, const Anim* anim)
{
    int32 speed = anim->speed;

    if (item->flags.gravity)
    {
        speed += anim->accel * (item->frameIndex - anim->frameBegin - 1);
        item->hSpeed -= speed >> 16;
        speed += anim->accel;
        item->hSpeed += speed >> 16;

        item->vSpeed += (item->vSpeed < 128) ? GRAVITY : 1;

        item->pos.y += item->vSpeed;
    } else {
        speed += anim->accel * (item->frameIndex - anim->frameBegin);
    
        item->hSpeed = speed >> 16;
    }

    item->pos.x += phd_sin(item->angleY) * item->hSpeed >> FIXED_SHIFT;
    item->pos.z += phd_cos(item->angleY) * item->hSpeed >> FIXED_SHIFT;
}

void animChange(Item* item, const Anim* anim)
{
    if (!anim->scCount) return;
    // check state change
}

void animCommand(bool fx, Item* item, const Anim* anim)
{
    if (!anim->acCount) return;
    // check animation command
}

const Anim* animSet(Item* item, int32 animIndex, int32 frameIndex)
{
    item->animIndex = animIndex;
    item->frameIndex = frameIndex;
    item->state = anims[animIndex].state;

    return anims + animIndex;
}

void animUpdate(Item* item)
{
    const Anim* anim = anims + item->animIndex;

    item->frameIndex++;

    animChange(item, anim);

    if (item->frameIndex > anim->frameEnd)
    {
        animCommand(false, item, anim);
        anim = animSet(item, anim->nextAnimIndex, anim->nextFrameIndex);
    }

    animCommand(true, item, anim);

    //move(item, anim);
}

void updateItems()
{
    int32 itemIndex = firstActive;
    while (itemIndex != NO_ITEM)
    {
        Item* item = items + itemIndex;

        if (modelsMap[item->type] != NO_MODEL) {
            animUpdate(item);
        }

        itemIndex = item->nextActive;
    }
}

#endif
