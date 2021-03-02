#ifndef H_DRAW
#define H_DRAW

#include "common.h"
#include "item.h"

int32 seqGlyphs;

void drawNumber(int32 number, int32 x, int32 y)
{
    const int32 widths[] = {
        12, 8, 10, 10, 10, 10, 10, 10, 10, 10
    };

    const Sprite *glyphSprites = sprites + spritesSeq[seqGlyphs].sStart;

    while (number > 0)
    {
        x -= widths[number % 10];
        drawGlyph(glyphSprites + 52 + (number % 10), x, y);
        number /= 10;
    }
}

void drawMesh(int16 meshIndex, uint16 intensity)
{
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

void drawShadow(const Item* item)
{
    const RoomInfo::Sector* sector = getSector(item->room, item->pos.x, item->pos.z);
    int32 floor = getFloor(sector, item->pos.x, item->pos.y, item->pos.z);

    if (floor == WALL)
        return;

    int32 shadowSize = 160; // TODO per item

    const Box& box = getBoundingBox(item);
    int32 x = (box.maxX + box.minX) >> 1;
    int32 z = (box.maxZ + box.minZ) >> 1;
    int32 sx = (box.maxX - box.minX) * shadowSize >> 10;
    int32 sz = (box.maxZ - box.minZ) * shadowSize >> 10;
    int32 sx2 = sx << 1;
    int32 sz2 = sz << 1;

    int32 startVertex = gVerticesCount;

    int32 y = floor - item->pos.y;

    transform(vec3s(x - sx,  y, z + sz2), 4096);
    transform(vec3s(x + sx,  y, z + sz2), 4096);
    transform(vec3s(x + sx2, y, z + sz),  4096);
    transform(vec3s(x + sx2, y, z - sz),  4096);

    transform(vec3s(x + sx,  y, z - sz2), 4096);
    transform(vec3s(x - sx,  y, z - sz2), 4096);
    transform(vec3s(x - sx2, y, z - sz),  4096);
    transform(vec3s(x - sx2, y, z + sz),  4096);

    static const Index indices[] = { 
        0, 1, 2, 7,
        7, 2, 3, 6,
        6, 3, 4, 5
    };

    faceAddQuad(FACE_COLORED | FACE_FLAT | FACE_SHADOW, indices + 0, startVertex);
    faceAddQuad(FACE_COLORED | FACE_FLAT | FACE_SHADOW, indices + 4, startVertex);
    faceAddQuad(FACE_COLORED | FACE_FLAT | FACE_SHADOW, indices + 8, startVertex);
}

void drawItem(const Item* item)
{
    int32 modelIndex = modelsMap[item->type];
    if (modelIndex == NO_MODEL) {
        return; // TODO sprite items
    }

    const Model* model = models + modelIndex;

    if (model->mCount == 1 && meshOffsets[model->mStart] == 0)
        return;

    AnimFrame* frame = getFrame(item, model);
    uint16* frameAngles = frame->angles + 1;

    matrixPush();
    matrixTranslateAbs(item->pos);
    matrixRotateYXZ(item->angleX, item->angleY, item->angleZ);

    int32 intensity = item->intensity;

    if (intensity == 0xFFFF) {
        intensity = itemCalcLighting(item, frame->box);
    }

    bool isVisible = boxIsVisible(&frame->box);
    if (isVisible)
    {
        // non-aligned access (TODO)
        uint32 nodeIndex;
        memcpy(&nodeIndex, &model->nodeIndex,  sizeof(nodeIndex));
        Node nodes[32];
        memcpy(nodes, nodesPtr + nodeIndex, (model->mCount - 1) * sizeof(Node));

        const Node* node = nodes;

        matrixFrame(frame->pos, frameAngles);

        drawMesh(model->mStart, intensity);

        for (int32 i = 1; i < model->mCount; i++)
        {
            if (node->flags & 1) matrixPop();
            if (node->flags & 2) matrixPush();

            frameAngles += 2;
            matrixFrame(node->pos, frameAngles);

            drawMesh(model->mStart + i, intensity);

            node++;
        }
    }

    matrixPop();

// shadow
    if (isVisible & item->flags.shadow) {
        matrixPush();
        matrixTranslateAbs(item->pos);
        matrixRotateY(item->angleY);

        drawShadow(item);

        matrixPop();
    }
}

void drawRoom(const Room* room)
{
    clip = room->clip;

    int32 startVertex = gVerticesCount;

    matrixPush();
    matrixTranslateAbs(vec3i(room->x, 0, room->z));

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
        matrixTranslateAbs(pos);
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

    flush();
}

bool checkPortal(int32 roomIndex, const RoomInfo::Portal* portal)
{
    Room &room = rooms[roomIndex];

    vec3i d;
    d.x = portal->v[0].x - camera.viewPos.x + room.x;
    d.y = portal->v[0].y - camera.viewPos.y;
    d.z = portal->v[0].z - camera.viewPos.z + room.z;

    if (DP33(portal->n, d) >= 0) {
        return false;
    }

    int32 x0 = room.clip.x1;
    int32 y0 = room.clip.y1;
    int32 x1 = room.clip.x0;
    int32 y1 = room.clip.y0;

    int32 znear = 0, zfar = 0;

    Matrix &m = matrixGet();

    vec3i pv[4];

    for (int32 i = 0; i < 4; i++)
    {
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
            PERSPECTIVE(x, y, z);

            x += FRAME_WIDTH  >> 1;
            y += FRAME_HEIGHT >> 1;
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
    matrixTranslateAbs(vec3i(room->x, 0, room->z));

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

void roomReset(Room* room)
{
    room->visible = false;
    room->clip = { FRAME_WIDTH, FRAME_HEIGHT, 0, 0 };
}

void drawRooms()
{
    rooms[camera.room].clip = { 0, 0, FRAME_WIDTH, FRAME_HEIGHT };
    visRoomsCount = 0;
    visRooms[visRoomsCount++] = camera.room;

    getVisibleRooms(camera.room);

    while (visRoomsCount--)
    {
        Room* room = rooms + visRooms[visRoomsCount];
        drawRoom(room);
        roomReset(room);
    }
}

#ifdef TEST
void faceAddQuad(uint32 flags, const Index* indices, int32 startVertex);

extern Vertex gVertices[MAX_VERTICES];

void drawTest() {
    static Rect testClip = { 0, 0, FRAME_WIDTH, FRAME_HEIGHT };
    static int32 testTile = 10; // 707 // 712

#ifdef _WIN32
    Sleep(16);
#endif

    int dx = 0;
    int dy = 0;

    if (keys & IK_LEFT) dy++;
    if (keys & IK_RIGHT) dy--;
    if (keys & IK_UP) dx--;
    if (keys & IK_DOWN) dx++;

    if (keys & IK_L) {
        testClip.x0 += dx;
        testClip.y0 += dy;
    }

    if (keys & IK_R) {
        testClip.x1 += dx;
        testClip.y1 += dy;
    }

    if (keys & IK_A) {
        testTile++;
        //Sleep(100);
    }

    if (keys & IK_B) {
        testTile--;
        //Sleep(100);
    }

    testTile = (testTile + texturesCount) % texturesCount;

    clip = testClip;

    static int vidx = 0;

    if (keys & IK_SELECT) {
        vidx++;
        //Sleep(100);
    }

    gVertices[(vidx + 0) % 4].x = -25;
    gVertices[(vidx + 0) % 4].y = -25;

    gVertices[(vidx + 1) % 4].x = 25;
    gVertices[(vidx + 1) % 4].y = -25;

    gVertices[(vidx + 2) % 4].x = 50;
    gVertices[(vidx + 2) % 4].y = 25;

    gVertices[(vidx + 3) % 4].x = -50;
    gVertices[(vidx + 3) % 4].y = 25;

    for (int i = 0; i < 4; i++) {
        gVertices[i].x += FRAME_WIDTH/2;
        gVertices[i].y += FRAME_HEIGHT/2;
        gVertices[i].z = 100;
        gVertices[i].g = 16;
        gVertices[i].clip = classify(gVertices + i, clip);
    }
    gVerticesCount = 4;

    Index indices[] = { 0, 1, 2, 3, 0, 2, 3 };

    faceAddQuad(testTile, indices, 0);

#ifdef _WIN32
    for (int y = 0; y < FRAME_HEIGHT; y++) {
        for (int x = 0; x < FRAME_WIDTH; x++) {
            if (x == clip.x0 || x == clip.x1 - 1 || y == clip.y0 || y == clip.y1 - 1)
                ((uint16*)fb)[y * FRAME_WIDTH + x] = 255;
        }
    }
#endif

    flush();
}
#endif

#endif
