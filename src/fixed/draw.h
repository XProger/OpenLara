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

    int32 fogZ = m.e23 >> FIXED_SHIFT;
    if (fogZ > FOG_MIN) {
        lightAmbient += (fogZ - FOG_MIN) << FOG_SHIFT;
        lightAmbient = X_MIN(lightAmbient, 8191);
    }
}

void calcLightingStatic(int32 intensity)
{
    lightAmbient = intensity - 4096;

    Matrix &m = matrixGet();

    int32 fogZ = m.e23 >> FIXED_SHIFT;
    if (fogZ > FOG_MIN) {
        lightAmbient += (fogZ - FOG_MIN) << FOG_SHIFT;
    }
}

void drawNumber(int32 number, int32 x, int32 y)
{
#ifdef __3DO__
    return;
#endif
    static const int32 widths[] = {
        12, 8, 10, 10, 10, 10, 10, 10, 10, 10
    };

    const Sprite* glyphSprites = level.sprites + level.models[ITEM_GLYPHS].start;

    while (number > 0)
    {
        x -= widths[number % 10];
        drawGlyph(glyphSprites + 52 + (number % 10), x, y);
        number /= 10;
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
    while ((c = *text++))
    {
        if (c == ' ') {
            w += 6;
            continue;
        }
        w += char_width[charRemap(c)];
    }

    return w;
}

enum TextAlign {
    TEXT_ALIGN_LEFT,
    TEXT_ALIGN_RIGHT,
    TEXT_ALIGN_CENTER
};

void drawText(int32 x, int32 y, const char* text, TextAlign align)
{
    if (align == TEXT_ALIGN_CENTER) {
        x += (FRAME_WIDTH - getTextWidth(text)) >> 1;
    }

    char c;
    while ((c = *text++))
    {
        if (c == ' ') {
            x += 6;
            continue;
        }
        int32 index = charRemap(c);
        faceAddGlyph(x, y, level.models[ITEM_GLYPHS].start + index);
        x += char_width[index];
    }
}

void drawMesh(int32 meshIndex)
{
    const uint8* ptr = (uint8*)level.meshes[meshIndex];

    const Mesh* mesh = (Mesh*)ptr; ptr += sizeof(Mesh);

    int16 vCount = mesh->vCount;

    bool hasNormals = true;
    if (vCount < 0) {
        hasNormals = false;
        vCount = -vCount;
    }

    const MeshVertex* vertices = (MeshVertex*)ptr;
    ptr += vCount * sizeof(vertices[0]);

    Quad* rFaces = (Quad*)ptr;
    ptr += mesh->rCount * sizeof(Quad);

    Triangle* tFaces = (Triangle*)ptr;
    ptr += mesh->tCount * sizeof(Triangle);

    Quad* crFaces = (Quad*)ptr;
    ptr += mesh->crCount * sizeof(Quad);

    Triangle* ctFaces = (Triangle*)ptr;
    ptr += mesh->ctCount * sizeof(Triangle);

    const uint16* vIntensity = NULL;
    const vec3s* vNormal = NULL;

    //const int16* normals = (int16*)ptr;
    if (hasNormals) { // normals
        vNormal = (vec3s*)ptr;
        ptr += vCount * sizeof(vec3s);
    } else { // intensity
        vIntensity = (uint16*)ptr;
        ptr += vCount * sizeof(uint16);
    }

    int32 startVertex = gVerticesCount;

    if (MAX_VERTICES - gVerticesCount < vCount)
        return;

    if (MAX_FACES - gFacesCount < mesh->rCount + mesh->tCount + mesh->crCount + mesh->ctCount)
        return;

    {
        PROFILE(CNT_TRANSFORM);
        transformMesh(vertices, vCount, vIntensity, vNormal);
    }

    {
        PROFILE(CNT_ADD);
        faceAddMesh(rFaces, crFaces, tFaces, ctFaces, mesh->rCount, mesh->crCount, mesh->tCount, mesh->ctCount, startVertex);
    }
}

void drawShadow(const ItemObj* item, int32 size)
{
    const Sector* sector = item->room->getSector(item->pos.x, item->pos.z);
    int32 floor = sector->getFloor(item->pos.x, item->pos.y, item->pos.z);

    if (floor == WALL)
        return;

    enableClipping = true;

    const AABBs& box = item->getBoundingBox(true);
    int32 x = (box.maxX + box.minX) >> 1;
    int32 z = (box.maxZ + box.minZ) >> 1;
    int32 sx = (box.maxX - box.minX) * size >> 10;
    int32 sz = (box.maxZ - box.minZ) * size >> 10;

    matrixPush();
    matrixTranslateAbs(item->pos.x, floor, item->pos.z);
    matrixRotateY(item->angle.y);

#ifdef __3DO__
    faceAddShadow(x, z, sx, sz);
#else
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

    int32 startVertex32 = gVerticesCount;

    transformMesh(v, 8, NULL, NULL);

    static const Index indices[] = { 
        0, 1, 2, 7,
        7, 2, 3, 6,
        6, 3, 4, 5
    };

    faceAddQuad(FACE_SHADOW, indices + 0, startVertex32);
    faceAddQuad(FACE_SHADOW, indices + 4, startVertex32);
    faceAddQuad(FACE_SHADOW, indices + 8, startVertex32);
#endif

    matrixPop();
}

void drawSprite(const ItemObj* item)
{
    faceAddSprite(item->pos.x, item->pos.y, item->pos.z, item->intensity << 5, level.models[item->type].start + item->frameIndex);
}

void drawFlash(const ExtraInfoLara::Arm::Flash &flash)
{
    matrixPush();
    matrixTranslate(0, flash.offset, 55);
    matrixRotateYXZ(-ANGLE_90, 0, flash.angle);

    int32 tmp = lightAmbient;
    calcLightingStatic(flash.intensity);

    drawMesh(level.models[ITEM_MUZZLE_FLASH].start);

    lightAmbient = tmp;

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

    matrixFrame(frameA->pos, angles);
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

        matrixFrame(node->pos, angles);

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

    int32 t = FixedInvU(frameRate) * frameDelta;

    vec3s posLerp;
    posLerp.x = frameA->pos.x + ((frameB->pos.x - frameA->pos.x) * t >> 16);
    posLerp.y = frameA->pos.y + ((frameB->pos.y - frameA->pos.y) * t >> 16);
    posLerp.z = frameA->pos.z + ((frameB->pos.z - frameA->pos.z) * t >> 16);

    matrixFrameLerp(posLerp, anglesA, anglesB, frameDelta, frameRate);
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

        matrixFrameLerp(node->pos, anglesA, anglesB, frameDelta, frameRate);

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

#define DEF_TORSO_ANGLE _vec3s(1216, -832, -192)

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
                #ifndef __3DO__ // TODO_3DO
                    matrixSetBasis(matrixGet(), basis);
                #endif
                    matrixRotateYXZ(arm->angle.x, arm->angle.y, arm->angle.z);
                }
                matrixFrame(_vec3s(0, 0, 0), anglesArm[i]++);
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
                #ifndef __3DO__ // TODO_3DO
                    matrixSetBasis(matrixGet(), basis);
                #endif
                    matrixRotateYXZ(arm->angle.x, arm->angle.y, arm->angle.z);
                }

                bool useLerp = frameRateArm[i] > 1; // armed hands always use frameRate == 1 (i.e. useLerp == false)

                if (useLerp) {
                    matrixFrameLerp(_vec3s(0, 0, 0), anglesArmA[i]++, anglesArmB[i]++, frameDelta, frameRate);
                } else {
                    matrixFrame(_vec3s(0, 0, 0), anglesArmA[i]++);
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

void drawModel(const ItemObj* item)
{
    const AnimFrame *frameA, *frameB;
    
    int32 frameRate;
    int32 frameDelta = item->getFrames(frameA, frameB, frameRate);

#ifdef NO_ANIM_LERP
    frameDelta = 0;
#endif

    matrixPush();
    matrixTranslateAbs(item->pos.x, item->pos.y, item->pos.z);
    matrixRotateYXZ(item->angle.x, item->angle.y, item->angle.z);

    int32 vis;
    RectMinMax rect;
    if (transformBoxRect(&frameA->box, &rect)) {
        vis = rectIsVisible(&rect);
    } else {
        vis = 0;
    }

    if (vis)
    {
    #ifndef NO_ANIM_LERP
        #ifdef LOD_ANIM
            if ((item->type != ITEM_LARA) && (item->type != ITEM_CRYSTAL))
            {
                int32 d = X_MAX(rect.x1 - rect.x0, rect.y1 - rect.y0);
                if (d < LOD_ANIM) {
                    frameDelta = 0; // don't use matrix interpolation for small objects on the screen
                }
            }
        #endif
    #endif

        enableClipping = vis < 0;

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
    }

    matrixPop();

// shadow
    if (vis && item->flags.shadow) {
        drawShadow(item, 160);  // TODO per item shadow size
    }
}

void drawItem(const ItemObj* item)
{
    if (level.models[item->type].count > 0) {
        drawModel(item);
    } else {
        drawSprite(item);
    }
}

void drawRoom(const Room* room, Camera* camera)
{
    setViewport(room->clip);

    int32 startVertex = gVerticesCount;

    const RoomInfo* info = room->info;
    const RoomData& data = room->data;

    if (MAX_VERTICES - gVerticesCount < info->verticesCount)
        return;

    if (MAX_FACES - gFacesCount < info->quadsCount + info->trianglesCount)
        return;

    int32 rx = info->x << 8;
    int32 rz = info->z << 8;

    matrixPush();
    matrixTranslateAbs(info->x << 8, 0, info->z << 8);

    camera->updateFrustum(info->x << 8, 0, info->z << 8);

    setPaletteIndex(ROOM_FLAG_WATER(info->flags) ? 1 : 0);

    enableClipping = true;

/* // show portals
    for (int32 i = 0; i < info->portalsCount; i++)
    {
        RoomVertex pv[4];
        for (int32 j = 0; j < 4; j++)
        {
            pv[j].x = (data.portals[i].v[j].x + 1) >> 10;
            pv[j].y = (data.portals[i].v[j].y + 1) >> 8;
            pv[j].z = (data.portals[i].v[j].z + 1) >> 10;
        }
        Quad q;
        q.flags = 171;
        q.indices[0] = i * 4 + 0;
        q.indices[1] = i * 4 + 1;
        q.indices[2] = i * 4 + 2;
        q.indices[3] = i * 4 + 3;

        transformRoom(pv, 4, 0);
        faceAddRoom(&q, 1, NULL, 0, startVertex);
    }
    startVertex = gVerticesCount;
*/

    {
        PROFILE(CNT_TRANSFORM);
        transformRoom(data.vertices, info->verticesCount, ROOM_FLAG_WATER(info->flags));
    }

    {
        PROFILE(CNT_ADD);
        faceAddRoom(data.quads, info->quadsCount, data.triangles, info->trianglesCount, startVertex);
    }

    matrixPop();

    for (int32 i = 0; i < info->spritesCount; i++)
    {
        const RoomSprite* sprite = data.sprites + i;
        faceAddSprite(sprite->pos.x + rx, sprite->pos.y, sprite->pos.z + rz, sprite->g << 5, sprite->index);
    }

    for (int32 i = 0; i < info->meshesCount; i++)
    {
        const RoomMesh* mesh = data.meshes + i;

    #ifdef NO_STATIC_MESH_PLANTS
        if (STATIC_MESH_ID(mesh->flags) < 10) continue;
    #endif

        const StaticMesh* staticMesh = level.staticMeshes + STATIC_MESH_ID(mesh->flags);

        if (!(staticMesh->flags & STATIC_MESH_FLAG_VISIBLE)) continue; // invisible

        vec3i pos;
        pos.x = mesh->pos.x + (info->x << 8);
        pos.y = mesh->pos.y;
        pos.z = mesh->pos.z + (info->z << 8);

        matrixPush();
        matrixTranslateAbs(pos.x, pos.y, pos.z);
        matrixRotateY(STATIC_MESH_ROT(mesh->flags));

        int32 vis = boxIsVisible(&staticMesh->vbox);
        if (vis != 0) {
            enableClipping =vis < 0; // TODO wrong visibility BBox?

            calcLightingStatic(mesh->intensity << 5);
            drawMesh(staticMesh->meshIndex);
        }

        matrixPop();
    }

    ItemObj* item = room->firstItem;
    while (item)
    {
        if (item->flags.status != ITEM_FLAGS_STATUS_INVISIBLE) {
            item->draw();
        }
        item = item->nextItem;
    }
}

void drawRooms(Camera* camera)
{
    camera->view.room->clip = viewport;

    Room** visRoom = camera->view.room->getVisibleRooms();

    // draw Lara first
    for (int32 i = 0; i < MAX_PLAYERS; i++)
    {
        Lara* lara = players[i];
        if (lara)
        {
            lara->flags.status = ITEM_FLAGS_STATUS_NONE;
            setPaletteIndex(ROOM_FLAG_WATER(lara->room->info->flags) ? 1 : 0);
            lara->draw();
            lara->flags.status = ITEM_FLAGS_STATUS_INVISIBLE; // skip drawing in the general pass
        }
    }

    // draw rooms and objects
    while (*visRoom)
    {
        Room* room = *visRoom++;
        drawRoom(room, camera);
        room->reset();
    }

    // reset visibility flags for Lara
    for (int32 i = 0; i < MAX_PLAYERS; i++)
    {
        Lara* lara = players[i];
        if (lara)
        {
            lara->flags.status = ITEM_FLAGS_STATUS_NONE;
        }
    }

    flush();

    setPaletteIndex(0);
    setViewport(camera->view.room->clip);
}

#endif
