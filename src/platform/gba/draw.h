#ifndef H_DRAW
#define H_DRAW

#include "common.h"
#include "item.h"

int32 lightAmbient;

int32 randTable[MAX_RAND_TABLE];
int32 caustics[MAX_CAUSTICS];
int32 causticsFrame;

void drawInit()
{
    for (int32 i = 0; i < MAX_RAND_TABLE; i++)
    {
        randTable[i] = (rand_draw() >> 5) - 511;
    }

    for (int32 i = 0; i < MAX_CAUSTICS; i++)
    {
        int16 rot = i * (ANGLE_90 * 4) / MAX_CAUSTICS;
        caustics[i] = phd_sin(rot) * 768 >> FIXED_SHIFT;
    }
}

void drawFree()
{
    //
}

void calcLightingDynamic(const Room* room, const vec3i &point)
{
    const RoomInfo* info = room->info;

    lightAmbient = (info->ambient << 5);

    if (!info->lightsCount)
        return;

    lightAmbient = 8191 - lightAmbient;
    int32 maxLum = 0;

    for (int i = 0; i < info->lightsCount; i++)
    {
        const Light* light = room->data.lights + i;

        vec3i pos;
        pos.x = light->pos.x + (info->x << 8);
        pos.y = light->pos.y;
        pos.z = light->pos.z + (info->z << 8);

        int32 radius = light->radius << 8;
        int32 intensity = light->intensity << 5;

        vec3i d = point - pos;
        int32 dist = dot(d, d) >> 12;
        int32 att = X_SQR(radius) >> 12;

        int32 lum = (intensity * att) / (dist + att) + lightAmbient;

        if (lum > maxLum) {
            maxLum = lum;
        }
    }

    lightAmbient = 8191 - ((maxLum + lightAmbient) >> 1);

    Matrix &m = matrixGet();

    int32 fogZ = m[2].w >> FIXED_SHIFT;
    if (fogZ > FOG_MIN) {
        lightAmbient += (fogZ - FOG_MIN) << FOG_SHIFT;
        lightAmbient = X_MIN(lightAmbient, 8191);
    }
}

void calcLightingStatic(int32 intensity)
{
    lightAmbient = intensity - 4096;

    Matrix &m = matrixGet();

    int32 fogZ = m[2].w >> FIXED_SHIFT;
    if (fogZ > FOG_MIN) {
        lightAmbient += (fogZ - FOG_MIN) << FOG_SHIFT;
    }
}

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

void drawMesh(int16 meshIndex)
{
    const uint8* ptr = (uint8*)meshes[meshIndex] + sizeof(Mesh);

    int16 vCount = *(int16*)ptr; ptr += 2;
    const vec3s* vertices = (vec3s*)ptr;
    ptr += vCount * 3 * sizeof(int16);

    const uint16* vIntensity = NULL;
    const vec3s* vNormal = NULL;

    int16 nCount = *(int16*)ptr; ptr += 2;
    //const int16* normals = (int16*)ptr;
    if (nCount > 0) { // normals
        vNormal = (vec3s*)ptr;
        ptr += nCount * 3 * sizeof(int16);
    } else { // intensity
        vIntensity = (uint16*)ptr;
        ptr += vCount * sizeof(uint16);
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

    {
        PROFILE(CNT_TRANSFORM);
        transformMesh(vertices, vCount, vIntensity, vNormal);
    }

    {
        PROFILE(CNT_ADD);
        faceAddMesh(rFaces, crFaces, tFaces, ctFaces, rCount, crCount, tCount, ctCount, startVertex);
    }
}

void drawShadow(const Item* item, int32 size)
{
    const Sector* sector = item->room->getSector(item->pos.x, item->pos.z);
    int32 floor = sector->getFloor(item->pos.x, item->pos.y, item->pos.z);

    if (floor == WALL)
        return;

    enableClipping = true;

    const Bounds& box = item->getBoundingBox(true);
    int32 x = (box.maxX + box.minX) >> 1;
    int32 z = (box.maxZ + box.minZ) >> 1;
    int32 sx = (box.maxX - box.minX) * size >> 10;
    int32 sz = (box.maxZ - box.minZ) * size >> 10;
    int32 sx2 = sx << 1;
    int32 sz2 = sz << 1;

    int32 startVertex = gVerticesCount;

    int32 y = floor - item->pos.y;

    matrixPush();
    matrixTranslateAbs(item->pos.x, item->pos.y, item->pos.z);
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

    faceAddQuad(FACE_SHADOW, indices + 0, startVertex);
    faceAddQuad(FACE_SHADOW, indices + 4, startVertex);
    faceAddQuad(FACE_SHADOW, indices + 8, startVertex);

    matrixPop();
}

void drawSprite(const Item* item)
{
    vec3i d = item->pos - cameraViewPos;
    faceAddSprite(d.x, d.y, d.z, item->intensity << 5, models[item->type].start + item->frameIndex);
}

void drawFlash(const ExtraInfoLara::Arm::Flash &flash)
{
    matrixPush();
    matrixTranslate(0, flash.offset, 55);
    matrixRotateYXZ(-ANGLE_90, 0, flash.angle);

    int32 tmp = lightAmbient;
    calcLightingStatic(flash.intensity);

    drawMesh(models[ITEM_MUZZLE_FLASH].start);

    lightAmbient = tmp;

    matrixPop();
}

void drawNodes(const Item* item, const AnimFrame* frameA)
{
    const Model* model = models + item->type;
    const Node* node = level.nodes + model->nodeIndex;

    const uint32* angles = (uint32*)(frameA->angles + 1);

    matrixFrame(frameA->pos, angles);

    drawMesh(model->start);

    for (int32 i = 1; i < model->count; i++)
    {
        if (node->flags & 1) matrixPop();
        if (node->flags & 2) matrixPush();

        matrixFrame(node->pos, ++angles);

        drawMesh(model->start + i);

        node++;
    }
}

void drawNodesLerp(const Item* item, const AnimFrame* frameA, const AnimFrame* frameB, int32 frameDelta, int32 frameRate)
{
    if (frameDelta == 0)
    {
        drawNodes(item, frameA);
        return;
    }

    const Model* model = models + item->type;
    const Node* node = level.nodes + model->nodeIndex;

    const uint32* anglesA = (uint32*)(frameA->angles + 1);
    const uint32* anglesB = (uint32*)(frameB->angles + 1);

    int32 t = FixedInvU(frameRate) * frameDelta;

    vec3s posLerp;
    posLerp.x = frameA->pos.x + ((frameB->pos.x - frameA->pos.x) * t >> 16);
    posLerp.y = frameA->pos.y + ((frameB->pos.y - frameA->pos.y) * t >> 16);
    posLerp.z = frameA->pos.z + ((frameB->pos.z - frameA->pos.z) * t >> 16);

    matrixFrameLerp(posLerp, anglesA, anglesB, frameDelta, frameRate);

    drawMesh(model->start);

    for (int32 i = 1; i < model->count; i++)
    {
        if (node->flags & 1) matrixPop();
        if (node->flags & 2) matrixPush();

        matrixFrameLerp(node->pos, ++anglesA, ++anglesB, frameDelta, frameRate);

        drawMesh(model->start + i);

        node++;
    }
}

#define DEF_TORSO_ANGLE vec3s(1216, -832, -192)

void drawLaraNodes(const Item* lara, const AnimFrame* frameA)
{
    const Model* model = models + lara->type;
    const Node* node = level.nodes + model->nodeIndex;
    const ExtraInfoLara* extraL = lara->extraL;

    const uint16* mesh = extraL->meshes;

    const uint32* anglesArm[LARA_ARM_MAX];
    const uint32* angles = anglesArm[LARA_ARM_R] = anglesArm[LARA_ARM_L] = (uint32*)(frameA->angles + 1);
    int32 frameSize = (sizeof(AnimFrame) >> 1) + (model->count << 1);

    vec3s torsoAngle = extraL->torso.angle;

    for (int32 i = 0; i < LARA_ARM_MAX; i++)
    {
        const ExtraInfoLara::Arm* arm = &extraL->armR + i;

        if (arm->animIndex)
        {
            const Anim* anim = level.anims + arm->animIndex;
            const AnimFrame* frame = (AnimFrame*)(level.animFrames + (anim->frameOffset >> 1) + arm->frameIndex * frameSize);
            anglesArm[i] = (uint32*)(frame->angles + 1);

            // additional torso animation for shotgun
            if (extraL->weapon == WEAPON_SHOTGUN && i == LARA_ARM_R)
            {
                vec3s ang;
                DECODE_ANGLES((uint32*)(frame->angles + 1) + JOINT_TORSO, ang.x, ang.y, ang.z);
                torsoAngle += ang - DEF_TORSO_ANGLE;
            }
        }
    }

    anglesArm[LARA_ARM_R] += JOINT_ARM_R1;
    anglesArm[LARA_ARM_L] += JOINT_ARM_L1;

    const Matrix& basis = matrixGet();

    matrixPush();
    { // JOINT_HIPS
        matrixFrame(frameA->pos, angles++);
        drawMesh(*mesh++);

        for (int32 i = 0; i < 2; i++) // draw Left & Right legs
        {
            matrixPush();
            { // JOINT_LEG_1
                matrixFrame((node++)->pos, angles++);
                drawMesh(*mesh++);

                { // JOINT_LEG_2
                    matrixFrame((node++)->pos, angles++);
                    drawMesh(*mesh++);

                    { // JOINT_LEG_3
                        matrixFrame((node++)->pos, angles++);
                        drawMesh(*mesh++);
                    }
                }
            }
            matrixPop();
        }

        { // JOINT_TORSO
            matrixFrame((node++)->pos, angles++);
            matrixRotateYXZ(torsoAngle.x, torsoAngle.y, torsoAngle.z);
            drawMesh(*mesh++);

            for (int32 i = 0; i < LARA_ARM_MAX; i++) // draw Right & Left arms
            {
                const ExtraInfoLara::Arm* arm = &extraL->armR + i;

                matrixPush();
                // JOINT_ARM_1
                matrixTranslate(node->pos.x, node->pos.y, node->pos.z);
                node++;
                if (arm->useBasis) { // hands are rotated relative to the basis
                    matrixSetBasis(matrixGet(), basis);
                    matrixRotateYXZ(arm->angle.x, arm->angle.y, arm->angle.z);
                }
                matrixFrame(vec3s(0, 0, 0), anglesArm[i]++);
                drawMesh(*mesh++);

                { // JOINT_ARM_2
                    matrixFrame((node++)->pos, anglesArm[i]++);
                    drawMesh(*mesh++);

                    { // JOINT_ARM_3
                        matrixFrame((node++)->pos, anglesArm[i]);
                        drawMesh(*mesh++);

                        if (arm->flash.timer) { // muzzle flash
                            drawFlash(arm->flash);
                        }
                    }
                }
                matrixPop();
            }

            { // JOINT_HEAD
                matrixFrame((node++)->pos, angles + 3 * LARA_ARM_MAX);
                matrixRotateYXZ(extraL->head.angle.x, extraL->head.angle.y, extraL->head.angle.z);
                drawMesh(*mesh++);
            }
        }
    }
    matrixPop();
}

void drawLaraNodesLerp(const Item* lara, const AnimFrame* frameA, const AnimFrame* frameB, int32 frameDelta, int32 frameRate)
{
    if (frameDelta == 0)
    {
        drawLaraNodes(lara, frameA);
        return;
    }

    const Model* model = models + lara->type;
    const Node* node = level.nodes + model->nodeIndex;
    const ExtraInfoLara* extraL = lara->extraL;

    const uint16* mesh = extraL->meshes;

    const uint32* anglesArmA[LARA_ARM_MAX];
    const uint32* anglesArmB[LARA_ARM_MAX];
    const uint32* anglesA = anglesArmA[LARA_ARM_R] = anglesArmA[LARA_ARM_L] = (uint32*)(frameA->angles + 1);
    const uint32* anglesB = anglesArmB[LARA_ARM_R] = anglesArmB[LARA_ARM_L] = (uint32*)(frameB->angles + 1);
    int32 frameRateArm[2];
    frameRateArm[0] = frameRateArm[1] = frameRate;

    int32 frameSize = (sizeof(AnimFrame) >> 1) + (model->count << 1);

    vec3s torsoAngle = extraL->torso.angle;

    for (int32 i = 0; i < LARA_ARM_MAX; i++)
    {
        const ExtraInfoLara::Arm* arm = &extraL->armR + i;

        if (arm->animIndex)
        {
            const Anim* anim = level.anims + arm->animIndex;
            const AnimFrame* frame = (AnimFrame*)(level.animFrames + (anim->frameOffset >> 1) + arm->frameIndex * frameSize);
            anglesArmA[i] = anglesArmB[i] = (uint32*)(frame->angles + 1); // no lerp for armed hands (frameRate == 1)
            ASSERT(anim->frameRate == 1);
            frameRateArm[i] = anim->frameRate;

            // additional torso animation for shotgun
            if (extraL->weapon == WEAPON_SHOTGUN && i == LARA_ARM_R)
            {
                vec3s ang;
                DECODE_ANGLES((uint32*)(frame->angles + 1) + JOINT_TORSO, ang.x, ang.y, ang.z);
                torsoAngle += ang - DEF_TORSO_ANGLE;
            }
        }
    }

    anglesArmA[LARA_ARM_R] += JOINT_ARM_R1;
    anglesArmB[LARA_ARM_R] += JOINT_ARM_R1;
    anglesArmA[LARA_ARM_L] += JOINT_ARM_L1;
    anglesArmB[LARA_ARM_L] += JOINT_ARM_L1;

    const Matrix& basis = matrixGet();

    matrixPush();
    { // JOINT_HIPS
        int32 t = FixedInvU(frameRate) * frameDelta;

        vec3s posLerp;
        posLerp.x = frameA->pos.x + ((frameB->pos.x - frameA->pos.x) * t >> 16);
        posLerp.y = frameA->pos.y + ((frameB->pos.y - frameA->pos.y) * t >> 16);
        posLerp.z = frameA->pos.z + ((frameB->pos.z - frameA->pos.z) * t >> 16);

        matrixFrameLerp(posLerp, anglesA++, anglesB++, frameDelta, frameRate);
        drawMesh(*mesh++);

        for (int32 i = 0; i < 2; i++) // draw Left & Right legs
        {
            matrixPush();
            { // JOINT_LEG_1
                matrixFrameLerp((node++)->pos, anglesA++, anglesB++, frameDelta, frameRate);
                drawMesh(*mesh++);

                { // JOINT_LEG_2
                    matrixFrameLerp((node++)->pos, anglesA++, anglesB++, frameDelta, frameRate);
                    drawMesh(*mesh++);

                    { // JOINT_LEG_3
                        matrixFrameLerp((node++)->pos, anglesA++, anglesB++, frameDelta, frameRate);
                        drawMesh(*mesh++);
                    }
                }
            }
            matrixPop();
        }

        { // JOINT_TORSO
            matrixFrameLerp((node++)->pos, anglesA++, anglesB++, frameDelta, frameRate);
            matrixRotateYXZ(torsoAngle.x, torsoAngle.y, torsoAngle.z);
            drawMesh(*mesh++);

            for (int32 i = 0; i < LARA_ARM_MAX; i++) // draw Right & Left arms
            {
                const ExtraInfoLara::Arm* arm = &extraL->armR + i;

                matrixPush();
                // JOINT_ARM_1
                matrixTranslate(node->pos.x, node->pos.y, node->pos.z);
                node++;
                if (arm->useBasis) { // hands are rotated relative to the basis
                    matrixSetBasis(matrixGet(), basis);
                    matrixRotateYXZ(arm->angle.x, arm->angle.y, arm->angle.z);
                }

                bool useLerp = frameRateArm[i] > 1; // armed hands always use frameRate == 1 (i.e. useLerp == false)

                if (useLerp) {
                    matrixFrameLerp(vec3s(0, 0, 0), anglesArmA[i]++, anglesArmB[i]++, frameDelta, frameRate);
                } else {
                    matrixFrame(vec3s(0, 0, 0), anglesArmA[i]++);
                }
                drawMesh(*mesh++);

                { // JOINT_ARM_2
                    if (useLerp) {
                        matrixFrameLerp((node++)->pos, anglesArmA[i]++, anglesArmB[i]++, frameDelta, frameRate);
                    } else {
                        matrixFrame((node++)->pos, anglesArmA[i]++);
                    }
                    drawMesh(*mesh++);

                    { // JOINT_ARM_3
                        if (useLerp) {
                            matrixFrameLerp((node++)->pos, anglesArmA[i], anglesArmB[i], frameDelta, frameRate);
                        } else {
                            matrixFrame((node++)->pos, anglesArmA[i]);
                        }
                        drawMesh(*mesh++);

                        if (arm->flash.timer) { // muzzle flash
                            drawFlash(arm->flash);
                        }
                    }
                }
                matrixPop();
            }

            { // JOINT_HEAD
                matrixFrameLerp((node++)->pos, anglesA + 3 * LARA_ARM_MAX, anglesB + 3 * LARA_ARM_MAX, frameDelta, frameRate);
                matrixRotateYXZ(extraL->head.angle.x, extraL->head.angle.y, extraL->head.angle.z);
                drawMesh(*mesh++);
            }
        }
    }
    matrixPop();
}

void drawModel(const Item* item)
{
    const AnimFrame *frameA, *frameB;
    
    int32 frameRate;
    int32 frameDelta = item->getFrames(frameA, frameB, frameRate); // TODO lerp

    matrixPush();
    matrixTranslateAbs(item->pos.x, item->pos.y, item->pos.z);
    matrixRotateYXZ(item->angle.x, item->angle.y, item->angle.z);

    int32 vis = boxIsVisible(&frameA->box);
    if (vis != 0)
    {
        enableClipping = vis < 0;

        int32 intensity = item->intensity << 5;

        if (intensity == 0) {
            vec3i point = item->getRelative(frameA->box.getCenter());
            calcLightingDynamic(item->room, point);
        } else {
            calcLightingStatic(intensity);
        }

        // skip rooms portal clipping for objects
        Rect oldViewport = viewport;
        viewport = Rect( 0, 0, FRAME_WIDTH, FRAME_HEIGHT );

        if (item->type == ITEM_LARA) {
            drawLaraNodesLerp(item, frameA, frameB, frameDelta, frameRate);
        } else {
            drawNodesLerp(item, frameA, frameB, frameDelta, frameRate);
        }

        viewport = oldViewport;
    }

    matrixPop();

// shadow
    if (vis != 0 && item->flags.shadow) {
        drawShadow(item, 160);  // TODO per item shadow size
    }
}

void drawItem(const Item* item)
{
    if (models[item->type].count > 0) {
        drawModel(item);
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
    matrixTranslateAbs(info->x << 8, 0, info->z << 8);

    gCamera->updateFrustum(info->x << 8, 0, info->z << 8);

    enableClipping = true;

    {
        PROFILE(CNT_TRANSFORM);
        transformRoom(data.vertices, info->verticesCount, info->flags.water);
    }

    {
        PROFILE(CNT_ADD);
        faceAddRoom(data.quads, info->quadsCount, data.triangles, info->trianglesCount, startVertex);
    }

    for (int32 i = 0; i < info->spritesCount; i++)
    {
        const RoomSprite* sprite = data.sprites + i;
        faceAddSprite(sprite->pos.x, sprite->pos.y, sprite->pos.z, sprite->g << 5, sprite->index);
    }

    matrixPop();

    for (int32 i = 0; i < info->meshesCount; i++)
    {
        const RoomMesh* mesh = data.meshes + i;

    #ifdef NO_STATIC_MESH_PLANTS
        if (mesh->id < 10) continue;
    #endif

        const StaticMesh* staticMesh = staticMeshes + mesh->id;

        if (!(staticMesh->flags & STATIC_MESH_FLAG_VISIBLE)) continue; // invisible

        vec3i pos;
        pos.x = mesh->pos.x + (info->x << 8);
        pos.y = mesh->pos.y;
        pos.z = mesh->pos.z + (info->z << 8);

        matrixPush();
        matrixTranslateAbs(pos.x, pos.y, pos.z);
        matrixRotateY((mesh->rot - 2) * ANGLE_90);

        int32 vis = boxIsVisible(&staticMesh->vbox);
        if (vis != 0) {
            enableClipping = true;//vis < 0; // TODO wrong visibility BBox?

            calcLightingStatic(mesh->intensity << 5);
            drawMesh(staticMesh->meshIndex);
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
    gCamera->view.room->clip = Rect( 0, 0, FRAME_WIDTH, FRAME_HEIGHT );

    Room** visRoom = gCamera->view.room->getVisibleRooms();

    while (*visRoom)
    {
        Room* room = *visRoom++;

        drawRoom(room);
        room->reset();
    }

    flush();
}

#endif
