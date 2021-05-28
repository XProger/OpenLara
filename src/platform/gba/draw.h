#ifndef H_DRAW
#define H_DRAW

#include "common.h"
#include "item.h"

extern AABB fastClipAABB;

void drawNumber(int32 number, int32 x, int32 y)
{
    static const int32 widths[] = {
        12, 8, 10, 10, 10, 10, 10, 10, 10, 10
    };

    const Sprite* glyphSprites = level.sprites + models[ITEM_GLYPHS].start;

    while (number > 0)
    {
        x -= widths[number % 10];
        drawGlyph(glyphSprites + 52 + (number % 10), x, y);
        number /= 10;
    }
}

void drawMesh(int16 meshIndex, uint16 intensity)
{
    int32 offset = level.meshOffsets[meshIndex];
    const uint8* ptr = level.meshData + offset;

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

void drawShadow(const Item* item, int32 size)
{
    const Sector* sector = item->room->getSector(item->pos.x, item->pos.z);
    int32 floor = sector->getFloor(item->pos.x, item->pos.y, item->pos.z);

    if (floor == WALL)
        return;

    enableClipping = true;

    const Bounds& box = item->getBoundingBox();
    int32 x = (box.maxX + box.minX) >> 1;
    int32 z = (box.maxZ + box.minZ) >> 1;
    int32 sx = (box.maxX - box.minX) * size >> 10;
    int32 sz = (box.maxZ - box.minZ) * size >> 10;
    int32 sx2 = sx << 1;
    int32 sz2 = sz << 1;

    int32 startVertex = gVerticesCount;

    int32 y = floor - item->pos.y;

    matrixPush();
    matrixTranslateAbs(item->pos);
    matrixRotateY(item->angle.y);

    transform(x - sx,  y, z + sz2, 4096);
    transform(x + sx,  y, z + sz2, 4096);
    transform(x + sx2, y, z + sz,  4096);
    transform(x + sx2, y, z - sz,  4096);

    transform(x + sx,  y, z - sz2, 4096);
    transform(x - sx,  y, z - sz2, 4096);
    transform(x - sx2, y, z - sz,  4096);
    transform(x - sx2, y, z + sz,  4096);

    static const Index indices[] = { 
        0, 1, 2, 7,
        7, 2, 3, 6,
        6, 3, 4, 5
    };

    faceAddQuad(FACE_COLORED | FACE_FLAT | FACE_SHADOW, indices + 0, startVertex);
    faceAddQuad(FACE_COLORED | FACE_FLAT | FACE_SHADOW, indices + 4, startVertex);
    faceAddQuad(FACE_COLORED | FACE_FLAT | FACE_SHADOW, indices + 8, startVertex);

    matrixPop();
}

void drawSprite(const Item* item)
{
    // TODO
}

void drawModel(const Item* item, uint16* meshOverrides)
{
    const Model* model = models + item->type;

    AnimFrame* frame = item->getFrame(model);
    uint16* frameAngles = frame->angles + 1;

    camera.updateFrustum(item->pos.x, item->pos.y, item->pos.z);

    matrixPush();
    matrixTranslateAbs(item->pos);
    matrixRotateYXZ(item->angle.x, item->angle.y, item->angle.z);

    int32 intensity = item->intensity << 5;

    if (intensity == 0) {
        intensity = item->calcLighting(frame->box);
    }

    int32 vis = boxIsVisible(&frame->box);
    if (vis != 0)
    {
        enableClipping = vis < 0;

        if (item->type == ITEM_BAT ||
            item->type == ITEM_TRAP_FLOOR) // some objects have the wrong AABB // TODO preprocess
        {
            enableClipping = true;
        }

        // skip rooms portal clipping for objects
        Rect oldViewport = viewport;
        viewport = Rect( 0, 0, FRAME_WIDTH, FRAME_HEIGHT );

        const Node* node = level.nodes + model->nodeIndex;

        matrixFrame(frame->pos, frameAngles);

        drawMesh(meshOverrides ? meshOverrides[0] : model->start, intensity);

        for (int32 i = 1; i < model->count; i++)
        {
            if (node->flags & 1) matrixPop();
            if (node->flags & 2) matrixPush();

            frameAngles += 2;
            matrixFrame(node->pos, frameAngles);

            drawMesh(meshOverrides ? meshOverrides[i] : (model->start + i), intensity);

            node++;
        }

        viewport = oldViewport;
    }

    matrixPop();

// shadow
    if (vis != 0 && item->flags.shadow)
    {
        drawShadow(item, 160);  // TODO per item shadow size
    }
}

void drawItem(const Item* item)
{
    if (models[item->type].count > 0) {
        drawModel(item, NULL);
    } else {
        drawSprite(item);
    }
}

void drawRoom(const Room* room)
{
    viewport = room->clip;

    int32 startVertex = gVerticesCount;

    const RoomInfo* info = room->info;
    const RoomData& data = room->data;

    matrixPush();
    matrixTranslateAbs(vec3i(info->x << 8, 0, info->z << 8));

    camera.updateFrustum(info->x << 8, 0, info->z << 8);

    enableClipping = true;

    PROFILE_START();
    transformRoom(data.vertices, info->verticesCount);
    PROFILE_STOP(dbg_transform);

    matrixPop();

    PROFILE_START();
    faceAddRoom(data.quads, info->quadsCount, data.triangles, info->trianglesCount, startVertex);
    PROFILE_STOP(dbg_poly);

    for (int32 i = 0; i < info->meshesCount; i++)
    {
        const RoomMesh* mesh = data.meshes + i;

    #ifdef NO_STATIC_MESHES
        if (mesh->id != STATIC_MESH_GATE) continue;
    #endif

        const StaticMesh* staticMesh = staticMeshes + mesh->id;

        if (!(staticMesh->flags & STATIC_MESH_FLAG_VISIBLE)) continue; // invisible

        vec3i pos;
        pos.x = mesh->pos.x + (info->x << 8);
        pos.y = mesh->pos.y;
        pos.z = mesh->pos.z + (info->z << 8);

        camera.updateFrustum(pos.x, pos.y, pos.z);

        matrixPush();
        matrixTranslateAbs(pos);
        matrixRotateY((mesh->rot - 2) * 0x4000);

        int32 vis = boxIsVisible(&staticMesh->vbox);
        if (vis != 0) {
            enableClipping = vis < 0;
            drawMesh(staticMesh->meshIndex, mesh->intensity << 5);
        }

        matrixPop();
    }

    Item* item = room->firstItem;
    while (item)
    {
        if (item->flags.status != ITEM_FLAGS_STATUS_INVISIBLE) {
            item->draw();
        }
        item = item->nextItem;
    }
}

void drawRooms()
{
    camera.view.room->clip = Rect( 0, 0, FRAME_WIDTH, FRAME_HEIGHT );

    Room** visRoom = camera.view.room->getVisibleRooms();

    while (*visRoom)
    {
        Room* room = *visRoom++;

        drawRoom(room);
        room->reset();
    }

    flush();
}

#ifdef TEST
void faceAddQuad(uint32 flags, const Index* indices, int32 startVertex);

extern Vertex gVertices[MAX_VERTICES];

Rect testClip = { 0, 0, FRAME_WIDTH, FRAME_HEIGHT };
int32 testTile = 10; // 707 // 712

void drawTest() {
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

    //testTile = (testTile + texturesCount) % texturesCount;

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

    for (int i = 0; i < 4; i++)
    {
        gVertices[i].x += FRAME_WIDTH/2;
        gVertices[i].y += FRAME_HEIGHT/2;
        gVertices[i].z = 100;
        gVertices[i].g = 16;
        gVertices[i].clip = classify(gVertices[i], clip);
    }
    gVerticesCount = 4;

    Index indices[] = { 0, 1, 2, 3, 0, 2, 3 };

    faceAddQuad(testTile, indices, 0);

#ifdef _WIN32
    for (int y = 0; y < FRAME_HEIGHT; y++)
    {
        for (int x = 0; x < FRAME_WIDTH; x++)
        {
            if (x == clip.x0 || x == clip.x1 - 1 || y == clip.y0 || y == clip.y1 - 1)
            {
            #ifdef MODE4
                ((uint8*)fb)[y * FRAME_WIDTH + x] = 255;
            #else
                ((uint16*)fb)[y * FRAME_WIDTH + x] = 255;
            #endif
            }
        }
    }
#endif

    flush();
}
#endif

#endif
