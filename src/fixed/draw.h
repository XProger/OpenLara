#ifndef H_DRAW
#define H_DRAW

#include "common.h"
#include "item.h"

//#define TEST_ROOM_CACHE

#ifdef TEST_ROOM_CACHE
RoomVertex roomVert[512];
EWRAM_DATA RoomQuad roomQuads[512];
EWRAM_DATA RoomTriangle roomTri[64];
#endif

void drawInit()
{
    renderInit();

    for (int32 i = 0; i < MAX_RAND_TABLE; i++)
    {
        gRandTable[i] = (rand_draw() >> 5) - 511;
    }

    for (int32 i = 0; i < MAX_CAUSTICS; i++)
    {
        int16 rot = i * (ANGLE_90 * 4) / MAX_CAUSTICS;
        gCaustics[i] = sin(rot) * 768 >> FIXED_SHIFT;
    }
}

void drawFree()
{
    renderFree();
}

void drawLevelInit()
{
    renderLevelInit();

#ifdef TEST_ROOM_CACHE
    Room &room = rooms[14];

    memcpy(roomVert, room.data.vertices, sizeof(RoomVertex) * room.info->verticesCount);
    memcpy(roomQuads, room.data.quads, sizeof(RoomQuad) * room.info->quadsCount);
    memcpy(roomTri, room.data.triangles, sizeof(RoomTriangle) * room.info->trianglesCount);

    room.data.vertices = roomVert;
    room.data.quads = roomQuads;
    room.data.triangles = roomTri;
#endif
}

void drawLevelFree()
{
    renderLevelFree();
}

void calcLightingDynamic(const Room* room, const vec3i &point)
{
    const RoomInfo* info = room->info;

    gLightAmbient = (info->ambient << 5);

    if (!info->lightsCount)
        return;

    gLightAmbient = 8191 - gLightAmbient;
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

        int32 lum = (intensity * att) / (dist + att) + gLightAmbient;

        if (lum > maxLum) {
            maxLum = lum;
        }
    }

    gLightAmbient = 8191 - ((maxLum + gLightAmbient) >> 1);

    Matrix &m = matrixGet();

    int32 fogZ = m.e23 >> (FIXED_SHIFT - MATRIX_FIXED_SHIFT);
    if (fogZ > FOG_MIN) {
        gLightAmbient += (fogZ - FOG_MIN) << FOG_SHIFT;
        gLightAmbient = X_MIN(gLightAmbient, 8191);
    }
}

void calcLightingStatic(int32 intensity)
{
    gLightAmbient = intensity - 4096;

    Matrix &m = matrixGet();

    int32 fogZ = m.e23 >> (FIXED_SHIFT - MATRIX_FIXED_SHIFT);
    if (fogZ > FOG_MIN) {
        gLightAmbient += (fogZ - FOG_MIN) << FOG_SHIFT;
        gLightAmbient = X_MIN(gLightAmbient, 8191);
    }
}

const static uint8 char_width[110] = {
    14, 11, 11, 11, 11, 11, 11, 13, 8, 11, 12, 11, 13, 13, 12, 11, 12, 12, 11, 12, 13, 13, 13, 12, 12, 11, // A-Z
    9, 9, 9, 9, 9, 9, 9, 9, 5, 9, 9, 5, 12, 10, 9, 9, 9, 8, 9, 8, 9, 9, 11, 9, 9, 9, // a-z
    12, 8, 10, 10, 10, 10, 10, 9, 10, 10, // 0-9
    5, 5, 5, 11, 9, 7, 8, 6, 0, 7, 7, 3, 8, 8, 13, 7, 9, 4, 12, 12, 
    7, 5, 7, 7, 7, 7, 7, 7, 7, 7, 16, 14, 14, 14, 16, 16, 16, 16, 16, 12, 14, 8, 8, 8, 8, 8, 8, 8
};

static const uint8 char_map[102] = {
    0, 64, 66, 78, 77, 74, 78, 79, 69, 70, 92, 72, 63, 71, 62, 68, 52, 53, 54, 55, 56, 57, 58, 59, 
    60, 61, 73, 73, 66, 74, 75, 65, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 
    18, 19, 20, 21, 22, 23, 24, 25, 80, 76, 81, 97, 98, 77, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 
    37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 100, 101, 102, 67, 0, 0, 0, 0, 0, 0, 0
};

X_INLINE int32 charRemap(char c)
{
    if (c < 11)
        return c + 81;
    if (c < 16)
        return c + 91;
    return char_map[c - 32];
}

int32 getTextWidth(const char* text)
{
    int32 w = 0;

    char c;
    while ((c = *text++) != 0)
    {
        if (c == ' ') {
            w += 6;
            continue;
        }
        w += char_width[charRemap(c)] + 1;
    }

    return w;
}

void drawText(int32 x, int32 y, const char* text, TextAlign align)
{
    if (!text || !*text)
        return;

    if (align == TEXT_ALIGN_CENTER) {
        x += (FRAME_WIDTH - getTextWidth(text)) >> 1;
    }

    if (align == TEXT_ALIGN_RIGHT) {
        x += FRAME_WIDTH - getTextWidth(text);
    }

    int32 index;
    char c;
    while ((c = *text++) != 0)
    {
        if (c == ' ') {
            x += 6;
            continue;
        }

        if (c == '$') { // special char
            index = *text++;
        } else {
            index = charRemap(c);
        }

        int32 iy = y;

        if (c == 'p') { // TODO investigate!
            iy--;
        }

        renderGlyph(x, iy, level.models[ITEM_GLYPHS].start + index);
        x += char_width[index] + 1;
    }
}

void drawMesh(int32 meshIndex)
{
    renderMesh(level.meshes[meshIndex]);
}

void drawShadow(const ItemObj* item, int32 size)
{
    const Sector* sector = item->room->getSector(item->pos.x, item->pos.z);
    int32 floor = sector->getFloor(item->pos.x, item->pos.y, item->pos.z);

    if (floor == WALL)
        return;

    const AABBs& box = item->getBoundingBox(true);
    int32 x = (box.maxX + box.minX) >> 1;
    int32 z = (box.maxZ + box.minZ) >> 1;
    int32 sx = (box.maxX - box.minX) * size >> 10;
    int32 sz = (box.maxZ - box.minZ) * size >> 10;

    matrixPush();
    matrixTranslateAbs(item->pos.x, floor, item->pos.z);
    matrixRotateY(item->angle.y);

    renderShadow(x, z, sx, sz);

    matrixPop();
}

void drawSprite(const ItemObj* item)
{
    renderSprite(item->pos.x, item->pos.y, item->pos.z, item->intensity << 5, level.models[item->type].start + item->frameIndex);
}

void drawFlash(const ExtraInfoLara::Arm::Flash &flash)
{
    matrixPush();
    matrixTranslateRel(0, flash.offset, 55);
    matrixRotateYXZ(-ANGLE_90, 0, flash.angle);

    int32 tmp = gLightAmbient;
    calcLightingStatic(flash.intensity);

    drawMesh(level.models[ITEM_MUZZLE_FLASH].start);

    gLightAmbient = tmp;

    matrixPop();
}

void drawNodes(const ItemObj* item, const AnimFrame* frameA)
{
    const Model* model = level.models + item->type;
    const ModelNode* node = level.nodes + model->nodeIndex;
    int32 meshIndex = model->start;
    int32 meshCount = model->count;
    uint32 visibleMask = item->visibleMask;

    const uint32* angles = (uint32*)(frameA->angles + 1);
    const int16* extraAngles = (int16*)item->extra;

    matrixFrame(&frameA->pos, angles);
    if (visibleMask & 1) {
        drawMesh(meshIndex);
    }

    while (meshCount > 1)
    {
        meshIndex++;
        visibleMask >>= 1;
        angles++;

        if (node->flags & NODE_FLAG_POP)  matrixPop();
        if (node->flags & NODE_FLAG_PUSH) matrixPush();

        matrixFrame(&node->pos, angles);

        if (extraAngles)
        {
            if (node->flags & NODE_FLAG_ROTY) matrixRotateY(*extraAngles++);
            if (node->flags & NODE_FLAG_ROTX) matrixRotateX(*extraAngles++);
            if (node->flags & NODE_FLAG_ROTZ) matrixRotateZ(*extraAngles++);
        }

        if (visibleMask & 1) {
            drawMesh(meshIndex);
        }

        meshCount--;
        node++;
    }
}

void drawNodesLerp(const ItemObj* item, const AnimFrame* frameA, const AnimFrame* frameB, int32 frameDelta, int32 frameRate)
{
    if (frameDelta == 0)
    {
        drawNodes(item, frameA);
        return;
    }

    const Model* model = level.models + item->type;
    const ModelNode* node = level.nodes + model->nodeIndex;
    int32 meshIndex = model->start;
    int32 meshCount = model->count;
    uint32 visibleMask = item->visibleMask;

    const uint32* anglesA = (uint32*)(frameA->angles + 1);
    const uint32* anglesB = (uint32*)(frameB->angles + 1);
    const int16* extraAngles = (int16*)item->extra;

    int32 t = GET_FRAME_T(frameDelta, frameRate);

    vec4s posLerp;
    posLerp.x = frameA->pos.x + ((frameB->pos.x - frameA->pos.x) * t >> 16);
    posLerp.y = frameA->pos.y + ((frameB->pos.y - frameA->pos.y) * t >> 16);
    posLerp.z = frameA->pos.z + ((frameB->pos.z - frameA->pos.z) * t >> 16);

    matrixFrameLerp(&posLerp, anglesA, anglesB, frameDelta, frameRate);
    if (visibleMask & 1) {
        drawMesh(meshIndex);
    }

    while (meshCount > 1)
    {
        meshIndex++;
        visibleMask >>= 1;
        anglesA++;
        anglesB++;

        if (node->flags & NODE_FLAG_POP)  matrixPop();
        if (node->flags & NODE_FLAG_PUSH) matrixPush();

        matrixFrameLerp(&node->pos, anglesA, anglesB, frameDelta, frameRate);

        if (extraAngles)
        {
            if (node->flags & NODE_FLAG_ROTY) matrixRotateY(*extraAngles++);
            if (node->flags & NODE_FLAG_ROTX) matrixRotateX(*extraAngles++);
            if (node->flags & NODE_FLAG_ROTZ) matrixRotateZ(*extraAngles++);
        }

        if (visibleMask & 1) {
            drawMesh(meshIndex);
        }

        meshCount--;
        node++;
    }
}

#define DEF_TORSO_ANGLE_X 1216
#define DEF_TORSO_ANGLE_Y -832
#define DEF_TORSO_ANGLE_Z -192

uint32 ZERO_POS[2] = { 0, 0 };

void drawLaraNodes(const ItemObj* lara, const AnimFrame* frameA)
{
    const Model* model = level.models + lara->type;
    const ModelNode* node = level.nodes + model->nodeIndex;
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
                int32 aX, aY, aZ;
                DECODE_ANGLES(*((uint32*)(frame->angles + 1) + JOINT_TORSO), aX, aY, aZ);
                torsoAngle.x = torsoAngle.x + aX - DEF_TORSO_ANGLE_X;
                torsoAngle.y = torsoAngle.y + aY - DEF_TORSO_ANGLE_Y;
                torsoAngle.z = torsoAngle.z + aZ - DEF_TORSO_ANGLE_Z;
            }
        }
    }

    anglesArm[LARA_ARM_R] += JOINT_ARM_R1;
    anglesArm[LARA_ARM_L] += JOINT_ARM_L1;

    const Matrix& basis = matrixGet();

    matrixPush();
    { // JOINT_HIPS
        matrixFrame(&frameA->pos, angles++);
        drawMesh(*mesh++);

        for (int32 i = 0; i < 2; i++) // draw Left & Right legs
        {
            matrixPush();
            { // JOINT_LEG_1
                matrixFrame(&(node++)->pos, angles++);
                drawMesh(*mesh++);

                { // JOINT_LEG_2
                    matrixFrame(&(node++)->pos, angles++);
                    drawMesh(*mesh++);

                    { // JOINT_LEG_3
                        matrixFrame(&(node++)->pos, angles++);
                        drawMesh(*mesh++);
                    }
                }
            }
            matrixPop();
        }

        { // JOINT_TORSO
            matrixFrame(&(node++)->pos, angles++);
            matrixRotateYXZ(torsoAngle.x, torsoAngle.y, torsoAngle.z);
            drawMesh(*mesh++);

            for (int32 i = 0; i < LARA_ARM_MAX; i++) // draw Right & Left arms
            {
                const ExtraInfoLara::Arm* arm = &extraL->armR + i;

                matrixPush();
                // JOINT_ARM_1
                matrixTranslateRel(node->pos.x, node->pos.y, node->pos.z);
                node++;
                if (arm->useBasis) { // hands are rotated relative to the basis
                    matrixSetBasis(matrixGet(), basis);
                    matrixRotateYXZ(arm->angle.x, arm->angle.y, arm->angle.z);
                }
                matrixFrame(ZERO_POS, anglesArm[i]++);
                drawMesh(*mesh++);

                { // JOINT_ARM_2
                    matrixFrame(&(node++)->pos, anglesArm[i]++);
                    drawMesh(*mesh++);

                    { // JOINT_ARM_3
                        matrixFrame(&(node++)->pos, anglesArm[i]);
                        drawMesh(*mesh++);

                        if (arm->flash.timer) { // muzzle flash
                            drawFlash(arm->flash);
                        }
                    }
                }
                matrixPop();
            }

            { // JOINT_HEAD
                matrixFrame(&(node++)->pos, angles + 3 * LARA_ARM_MAX);
                matrixRotateYXZ(extraL->head.angle.x, extraL->head.angle.y, extraL->head.angle.z);
                drawMesh(*mesh++);
            }
        }
    }
    matrixPop();
}

void drawLaraNodesLerp(const ItemObj* lara, const AnimFrame* frameA, const AnimFrame* frameB, int32 frameDelta, int32 frameRate)
{
    if (frameDelta == 0)
    {
        drawLaraNodes(lara, frameA);
        return;
    }

    const Model* model = level.models + lara->type;
    const ModelNode* node = level.nodes + model->nodeIndex;
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
                int32 aX, aY, aZ;
                DECODE_ANGLES(*((uint32*)(frame->angles + 1) + JOINT_TORSO), aX, aY, aZ);
                torsoAngle.x = torsoAngle.x + aX - DEF_TORSO_ANGLE_X;
                torsoAngle.y = torsoAngle.y + aY - DEF_TORSO_ANGLE_Y;
                torsoAngle.z = torsoAngle.z + aZ - DEF_TORSO_ANGLE_Z;
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
        int32 t = GET_FRAME_T(frameDelta, frameRate);

        vec4s posLerp;
        posLerp.x = frameA->pos.x + ((frameB->pos.x - frameA->pos.x) * t >> 16);
        posLerp.y = frameA->pos.y + ((frameB->pos.y - frameA->pos.y) * t >> 16);
        posLerp.z = frameA->pos.z + ((frameB->pos.z - frameA->pos.z) * t >> 16);

        matrixFrameLerp(&posLerp, anglesA++, anglesB++, frameDelta, frameRate);
        drawMesh(*mesh++);

        for (int32 i = 0; i < 2; i++) // draw Left & Right legs
        {
            matrixPush();
            { // JOINT_LEG_1
                matrixFrameLerp(&(node++)->pos, anglesA++, anglesB++, frameDelta, frameRate);
                drawMesh(*mesh++);

                { // JOINT_LEG_2
                    matrixFrameLerp(&(node++)->pos, anglesA++, anglesB++, frameDelta, frameRate);
                    drawMesh(*mesh++);

                    { // JOINT_LEG_3
                        matrixFrameLerp(&(node++)->pos, anglesA++, anglesB++, frameDelta, frameRate);
                        drawMesh(*mesh++);
                    }
                }
            }
            matrixPop();
        }

        { // JOINT_TORSO
            matrixFrameLerp(&(node++)->pos, anglesA++, anglesB++, frameDelta, frameRate);
            matrixRotateYXZ(torsoAngle.x, torsoAngle.y, torsoAngle.z);
            drawMesh(*mesh++);

            for (int32 i = 0; i < LARA_ARM_MAX; i++) // draw Right & Left arms
            {
                const ExtraInfoLara::Arm* arm = &extraL->armR + i;

                matrixPush();
                // JOINT_ARM_1
                matrixTranslateRel(node->pos.x, node->pos.y, node->pos.z);
                node++;
                if (arm->useBasis) { // hands are rotated relative to the basis
                    matrixSetBasis(matrixGet(), basis);
                    matrixRotateYXZ(arm->angle.x, arm->angle.y, arm->angle.z);
                }

                bool useLerp = frameRateArm[i] > 1; // armed hands always use frameRate == 1 (i.e. useLerp == false)

                if (useLerp) {
                    matrixFrameLerp(ZERO_POS, anglesArmA[i]++, anglesArmB[i]++, frameDelta, frameRate);
                } else {
                    matrixFrame(ZERO_POS, anglesArmA[i]++);
                }
                drawMesh(*mesh++);

                { // JOINT_ARM_2
                    if (useLerp) {
                        matrixFrameLerp(&(node++)->pos, anglesArmA[i]++, anglesArmB[i]++, frameDelta, frameRate);
                    } else {
                        matrixFrame(&(node++)->pos, anglesArmA[i]++);
                    }
                    drawMesh(*mesh++);

                    { // JOINT_ARM_3
                        if (useLerp) {
                            matrixFrameLerp(&(node++)->pos, anglesArmA[i], anglesArmB[i], frameDelta, frameRate);
                        } else {
                            matrixFrame(&(node++)->pos, anglesArmA[i]);
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
                matrixFrameLerp(&(node++)->pos, anglesA + 3 * LARA_ARM_MAX, anglesB + 3 * LARA_ARM_MAX, frameDelta, frameRate);
                matrixRotateYXZ(extraL->head.angle.x, extraL->head.angle.y, extraL->head.angle.z);
                drawMesh(*mesh++);
            }
        }
    }
    matrixPop();
}

void drawModel(const ItemObj* item)
{
    const AnimFrame *frameA, *frameB;
    
    int32 frameRate;
    int32 frameDelta = item->getFrames(frameA, frameB, frameRate);

#ifdef NO_ANIM_LERP
    frameDelta = 0;
#endif

    int32 sx, sy, sz, sr, smin, smax;
    smin = frameA->box.minX;
    smax = frameA->box.maxX;
    sx = (smax + smin) >> 1;
    sr = (smax - smin);

    smin = frameA->box.minY;
    smax = frameA->box.maxY;
    sy = (smax + smin) >> 1;
    sr = X_MAX(sr, (smax - smin));

    smin = frameA->box.minZ;
    smax = frameA->box.maxZ;
    sz = (smax + smin) >> 1;
    sr = X_MAX(sr, (smax - smin));

    // approx. radius (TODO more precise)
    sr = (sr >> 1) + (sr >> 2);

    // rotate sphere center by quadrant
    uint16 quadrant = uint16(item->angle.y + ANGLE_45) >> ANGLE_SHIFT_90;

    if (quadrant != 0)
    {
        int32 ix = sx;
        int32 iz = sz;

        switch (quadrant)
        {
            case 1:
                sx = iz;
                sz = -ix;
                break;
            case 2:
                sx = -ix;
                sz = -iz;
                break;
            case 3: 
                sx = -iz;
                sz = ix;
                break;
        }
    }

    sx += item->pos.x - gCameraViewPos.x;
    sy += item->pos.y - gCameraViewPos.y;
    sz += item->pos.z - gCameraViewPos.z;

    if (!sphereIsVisible(sx, sy, sz, sr))
        return;

    matrixPush();
    matrixTranslateAbs(item->pos.x, item->pos.y, item->pos.z);
    matrixRotateYXZ(item->angle.x, item->angle.y, item->angle.z);

    int32 intensity = item->intensity << 5;

    if (intensity == 0) {
        vec3i point = item->getRelative(frameA->box.getCenter());
        calcLightingDynamic(item->room, point);
    } else {
        calcLightingStatic(intensity);
    }

    // skip rooms portal clipping for objects
    if (item->type == ITEM_LARA) {
        drawLaraNodesLerp(item, frameA, frameB, frameDelta, frameRate);
    } else {
        drawNodesLerp(item, frameA, frameB, frameDelta, frameRate);
    }

    matrixPop();

// shadow
    if (item->flags & ITEM_FLAG_SHADOW) {
        drawShadow(item, 160);  // TODO per item shadow size
    }
}

void drawRoom(Room* room)
{
    setViewport(room->clip);

    const RoomInfo* info = room->info;
    const RoomData& data = room->data;

    int32 rx = info->x << 8;
    int32 ry = info->yTop;
    int32 rz = info->z << 8;

    matrixPush();
    matrixTranslateAbs(rx, ry, rz);

    setPaletteIndex(ROOM_FLAG_WATER(info->flags) ? 1 : 0);

    renderRoom(room);

    matrixPop();

    for (int32 i = 0; i < info->spritesCount; i++)
    {
        const RoomSprite* sprite = data.sprites + i;
        renderSprite(sprite->pos.x + rx, sprite->pos.y, sprite->pos.z + rz, sprite->g << 5, sprite->index);
    }

    rx -= gCameraViewPos.x;
    ry = -gCameraViewPos.y;
    rz -= gCameraViewPos.z;

    for (int32 i = 0; i < info->meshesCount; i++)
    {
        const RoomMesh* mesh = data.meshes + i;

    #ifdef NO_STATIC_MESH_PLANTS
        if (STATIC_MESH_ID(mesh->zf) < 10) continue;
    #endif

        const StaticMesh* staticMesh = level.staticMeshes + STATIC_MESH_ID(mesh->zf);

        if (!(staticMesh->flags & STATIC_MESH_FLAG_VISIBLE)) continue; // invisible

        int32 px = rx + (int32(mesh->xy) >> 16);
        int32 py = ry + (int32(mesh->xy) << 16 >> 16);
        int32 pz = rz + (int32(mesh->zf) >> 16);

        int32 sx = (int32(staticMesh->vs.xy) >> 16);
        int32 sy = (int32(staticMesh->vs.xy) << 16 >> 16);
        int32 sz = (int32(staticMesh->vs.zr) >> 16);
        int32 sr = (int32(staticMesh->vs.zr) << 16 >> 16);

        // rotate visible sphere offset
        int32 q = STATIC_MESH_QUADRANT(mesh->zf);
        if (q == 0) {
            sx = -sx;
            sz = -sz;
        } else if (q == 1) {
            int32 t = sx;
            sx = -sz;
            sz = t;
        } else if (q == 3) {
            int32 t = sz;
            sz = -sx;
            sx = t;
        }

        sx += px;
        sy += py;
        sz += pz;

        if (!sphereIsVisible(sx, sy, sz, sr))
            continue;

        matrixPush();
        matrixTranslateSet(px, py, pz);
        matrixRotateYQ(q);

        calcLightingStatic(STATIC_MESH_INTENSITY(mesh->zf));
        drawMesh(staticMesh->meshIndex);

        matrixPop();
    }

    ItemObj* item = room->firstItem;
    while (item)
    {
        if ((item->flags & ITEM_FLAG_STATUS) != ITEM_FLAG_STATUS_INVISIBLE) {
            item->draw();
        }
        item = item->nextItem;
    }
}

void drawRooms(Camera* camera)
{
    RectMinMax vp = viewport;

    camera->view.room->clip = viewport;

    Room** visRoom = camera->view.room->getVisibleRooms();

#ifdef DRAW_LARA_FIRST
    for (int32 i = 0; i < MAX_PLAYERS; i++)
    {
        Lara* lara = players[i];
        if (lara)
        {
            lara->flags &= ~ITEM_FLAG_STATUS;
            setPaletteIndex(ROOM_FLAG_WATER(lara->room->info->flags) ? 1 : 0);
            lara->draw();
            lara->flags |= ITEM_FLAG_STATUS_INVISIBLE; // skip drawing in the general pass
        }
    }
#endif
    // draw rooms and objects
    while (*visRoom)
    {
        Room* room = *visRoom++;
        drawRoom(room);
        room->reset();
    }

#ifdef DRAW_LARA_FIRST
    // reset visibility flags for Lara
    for (int32 i = 0; i < MAX_PLAYERS; i++)
    {
        Lara* lara = players[i];
        if (lara)
        {
            lara->flags &= ~ITEM_FLAG_STATUS;
        }
    }
#endif

    setPaletteIndex(0);
    setViewport(vp);
}

void drawCinematicRooms()
{
    RectMinMax vp = viewport;
#if 1
    gCinematicCamera.view.room->clip = viewport;

    Room** visRoom = gCinematicCamera.view.room->getVisibleRooms();

    // draw rooms and objects
    while (*visRoom)
    {
        Room* room = *visRoom++;
        drawRoom(room);
        room->reset();
    }
#else
    for (int32 i = 0; i < level.roomsCount; i++)
    {
        rooms[i].clip = vp;
    }

    for (int32 i = 0; i < level.roomsCount; i++)
    {
        if (rooms[i].info->alternateRoom != NO_ROOM) {
            rooms[rooms[i].info->alternateRoom].clip.x0 = 0xFFFF;
        }
    }

    for (int32 i = 0; i < level.roomsCount; i++)
    {
        const Room* room = rooms + i;
        if (room->clip.x0 == 0xFFFF)
            continue;

        drawRoom(room);
    }
#endif

    setPaletteIndex(0);
    setViewport(vp);
}

void drawHUD(Lara* lara)
{
    int32 x = (FRAME_WIDTH - (100 + 2 + 2)) - 4;
    int32 y = 4;

    if (lara->waterState == WATER_STATE_SURFACE || lara->waterState == WATER_STATE_UNDER)
    {
        int32 v = (lara->oxygen << 8) / LARA_MAX_OXYGEN;
        renderBar(x, y, 100, v, BAR_OXYGEN);
        y += 10;
    }

    if (lara->extraL->healthTimer || lara->extraL->weaponState == WEAPON_STATE_READY)
    {
        int32 v = (lara->health << 8) / LARA_MAX_HEALTH;
        renderBar(x, y, 100, v, BAR_HEALTH);
    }
}

void drawFPS()
{
#ifdef __GBA__
    if (gLevelID == LVL_TR1_TITLE)
    {
        if (REG_WSCNT != (WS_ROM0_N2 | WS_ROM0_S1 | WS_PREFETCH))
        {
            drawText(0, 15, "! slow cartridge !", TEXT_ALIGN_CENTER);
        }
    }
#endif

    if (!gSettings.video_fps)
        return;

    char buf[32];
    int2str(fps, buf);
    drawText(2, 16, buf, TEXT_ALIGN_LEFT);
}

#ifdef PROFILING
void drawProfiling()
{
    for (int32 i = 0; i < CNT_MAX; i++)
    {
        char buf[32];
        int2str(gCounters[i], buf);
        drawText(2, 16 + 32 + i * 16, buf, TEXT_ALIGN_LEFT);
    }
    flush();
}
#endif

#endif
