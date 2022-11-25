#ifndef H_ITEM
#define H_ITEM

#include "common.h"
#include "camera.h"
#include "room.h"

EWRAM_DATA AABBs tmpBox;

#define GRAVITY      6

int32 alignOffset(int32 a, int32 b)
{
    int32 ca = a >> 10;
    int32 cb = b >> 10;

    if (ca == cb) {
        return 0;
    }

    a &= 1023;

    if (ca < cb) {
        return 1025 - a;
    }

    return -(a + 1);
}

void* soundPlay(int16 id, const vec3i* pos)
{
#if defined(__32X__) || defined(__WIN32__) // TODO
    return NULL;
#endif

    if (!gSettings.audio_sfx)
        return NULL;

    if (id < 0)
        return NULL;

    // TODO gym
    // 0 -> 200
    // 4 -> 204

    int16 a = level.soundMap[id];

    if (a == -1)
        return NULL;

    const SoundInfo* b = level.soundsInfo + a;

    if (b->chance && b->chance < rand_draw())
        return NULL;

    int32 volume = b->volume;

    if (pos)
    {
        vec3i d = *pos - playersExtra[0].camera.target.pos; // TODO find nearest camera for coop

        if (abs(d.x) >= SND_MAX_DIST || abs(d.y) >= SND_MAX_DIST || abs(d.z) >= SND_MAX_DIST)
            return NULL;

        volume -= (phd_sqrt(dot(d, d)) << 2);
    }

    if (SI_GAIN(b->flags)) {
        volume -= rand_draw() >> 2;
    }

    volume = X_MIN(volume, 0x7FFF) >> 9;

    if (volume <= 0)
        return NULL;

    volume += 1; // 63 to 64 (1 << SND_VOL_SHIFT) for 100% vol samples

    int32 pitch = 128;

    if (SI_PITCH(b->flags)) {
        pitch += ((rand_draw() * 13) >> 14) - 13;
    }

    int32 index = b->index;
    if (SI_COUNT(b->flags) > 1) {
        index += (rand_draw() * SI_COUNT(b->flags)) >> 15;
    }

    return sndPlaySample(index, volume, pitch, SI_MODE(b->flags));
}

void soundStop(int16 id)
{
#if defined(__32X__) || defined(__WIN32__) // TODO
    return;
#endif

    int16 a = level.soundMap[id];

    if (a == -1)
        return;

    const SoundInfo* b = level.soundsInfo + a;

    for (int32 i = 0; i < SI_COUNT(b->flags); i++)
    {
        sndStopSample(b->index + i);
    }
}

int32 ItemObj::getFrames(const AnimFrame* &frameA, const AnimFrame* &frameB, int32 &animFrameRate) const
{
    const Anim* anim = level.anims + animIndex;

    if (anim->frameBegin == anim->frameEnd)
    {
        frameA = frameB = (AnimFrame*)(level.animFrames + (anim->frameOffset >> 1));
        animFrameRate = 1;
        return 0;
    }

    animFrameRate = anim->frameRate;

    int32 frameSize = (sizeof(AnimFrame) >> 1) + (level.models[type].count << 1);

    int32 frame = frameIndex - anim->frameBegin;

//    int32 d = FixedInvU(animFrameRate);
//    int32 indexA = frame * d >> 16;

    int32 indexA, indexB;
    int32 frameDelta;

    if (animFrameRate == 1)
    {
        indexA = frame;
        frameDelta = frame - indexA;
    }
    else if (animFrameRate == 2)
    {
        indexA = frame >> 1;
        frameDelta = frame - (indexA << 1);
    }
    else if (animFrameRate == 4)
    {
        indexA = frame >> 2;
        frameDelta = frame - (indexA << 2);
    }
    else
    {
        indexA = frame / animFrameRate;
        frameDelta = frame - (indexA * animFrameRate);
    }

    indexB = indexA + 1;

    if (indexB * animFrameRate >= anim->frameEnd)
    {
        indexB = indexA;
    }

    frameA = (AnimFrame*)(level.animFrames + (anim->frameOffset >> 1) + indexA * frameSize);
    frameB = (AnimFrame*)(level.animFrames + (anim->frameOffset >> 1) + indexB * frameSize);

    if (!frameDelta || frameA == frameB)
        return 0;

    indexB *= animFrameRate;
    if (indexB > anim->frameEnd) {
        animFrameRate -= indexB - anim->frameEnd;
    }

    return frameDelta;
}

const AnimFrame* ItemObj::getFrame() const
{
    const AnimFrame *frameA, *frameB;

    int32 frameRate;
    int32 frameDelta = getFrames(frameA, frameB, frameRate);

    return (frameDelta <= (frameRate >> 1)) ? frameA : frameB;
}

const AABBs& ItemObj::getBoundingBox(bool lerp) const
{
    if (!lerp)
        return getFrame()->box;

    const AnimFrame *frameA, *frameB;

    int32 frameRate;
    int32 frameDelta = getFrames(frameA, frameB, frameRate);

    if (!frameDelta)
        return frameA->box;

    int32 d = GET_FRAME_T(frameDelta, frameRate);

    #define COMP_LERP(COMP) tmpBox.COMP = frameA->box.COMP + ((frameB->box.COMP - frameA->box.COMP) * d >> 16);

    COMP_LERP(minX);
    COMP_LERP(maxX);
    COMP_LERP(minY);
    COMP_LERP(maxY);
    COMP_LERP(minZ);
    COMP_LERP(maxZ);

    #undef COMP_LERP

    return tmpBox;
}

void ItemObj::move()
{
    const Anim* anim = level.anims + animIndex;

    int32 sp = anim->speed;

    if (flags & ITEM_FLAG_GRAVITY)
    {
        sp += anim->accel * (frameIndex - anim->frameBegin - 1);
        hSpeed -= sp >> 16;
        sp += anim->accel;
        hSpeed += sp >> 16;

        vSpeed += (vSpeed < 128) ? GRAVITY : 1;

        pos.y += vSpeed;
    } else {
        sp += anim->accel * (frameIndex - anim->frameBegin);
    
        hSpeed = sp >> 16;
    }

    int16 realAngle = (type == ITEM_LARA) ? extraL->moveAngle : angle.y;

    int32 s, c;
    sincos(realAngle, s, c);

    pos.x += s * hSpeed >> FIXED_SHIFT;
    pos.z += c * hSpeed >> FIXED_SHIFT;
}

const Anim* ItemObj::animSet(int32 newAnimIndex, bool resetState, int32 frameOffset)
{
    const Anim* anim = level.anims + newAnimIndex;

    animIndex   = newAnimIndex;
    frameIndex  = anim->frameBegin + frameOffset;

    if (resetState) {
        state = goalState = uint8(anim->state);
    }

    return anim;
}

const Anim* ItemObj::animChange(const Anim* anim)
{
    if (goalState == state || !anim->statesCount)
        return anim;

    const AnimState* animState = level.animStates + anim->statesStart;

    for (int32 i = 0; i < anim->statesCount; i++)
    {
        if (goalState == animState->state)
        {
            const AnimRange* animRange = level.animRanges + animState->rangesStart;

            for (int32 j = 0; j < animState->rangesCount; j++)
            {
                if ((frameIndex >= animRange->frameBegin) && (frameIndex <= animRange->frameEnd))
                {
                    frameIndex = animRange->nextFrameIndex;
                    animIndex = animRange->nextAnimIndex;
                    anim = level.anims + animRange->nextAnimIndex;
                    state = uint8(anim->state);
                    return anim;
                }
                animRange++;
            }
        }
        animState++;
    }

    return anim;
}

void ItemObj::animCmd(bool fx, const Anim* anim)
{
    if (!anim->commandsCount) return;

    const int16 *ptr = level.animCommands + anim->commandsStart;

    for (int32 i = 0; i < anim->commandsCount; i++)
    {
        int32 cmd = *ptr++;

        switch (cmd)
        {
            case ANIM_CMD_NONE:
                break;

            case ANIM_CMD_OFFSET:
            {
                if (!fx)
                {
                    int32 s, c;
                    sincos(angle.y, s, c);
                    int32 x = ptr[0];
                    int32 z = ptr[2];
                    pos.x += X_ROTX(x, z, -s, c);
                    pos.y += ptr[1];
                    pos.z += X_ROTY(x, z, -s, c);
                }
                ptr += 3;
                break;
            }

            case ANIM_CMD_JUMP:
            {
                if (!fx)
                {
                    if (type == ITEM_LARA && extraL->vSpeedHack) {
                        vSpeed = -extraL->vSpeedHack;
                        extraL->vSpeedHack = 0;
                    } else {
                        vSpeed = ptr[0];
                    }
                    hSpeed = ptr[1];
                    flags |= ITEM_FLAG_GRAVITY;
                }
                ptr += 2;
                break;
            }

            case ANIM_CMD_EMPTY:
            {
                if (!fx) {
                    ASSERT(type == ITEM_LARA);
                    extraL->weaponState = WEAPON_STATE_FREE;
                }
                break;
            }

            case ANIM_CMD_KILL:
            {
                if (!fx) {
                    flags &= ~ITEM_FLAG_STATUS;
                    flags |= ITEM_FLAG_STATUS_INACTIVE;
                }
                break;
            }

            case ANIM_CMD_SOUND:
            {
                if (fx && frameIndex == ptr[0]) {
                    soundPlay(ptr[1] & 0x03FFF, &pos);
                }
                ptr += 2;
                break;
            }

            case ANIM_CMD_EFFECT:
            {
                if (fx && frameIndex == ptr[0])
                {
                    switch (ptr[1]) {
                        case FX_ROTATE_180 :
                        {
                            angle.y += ANGLE_180;
                            break;
                        }

                    /*
                        case FX_FLOOR_SHAKE    : ASSERT(false);
                    */

                        case FX_LARA_NORMAL :
                        {
                            ASSERT(type == ITEM_LARA);
                            animSet(11, true); // Lara::ANIM_STAND
                            break;
                        }

                        case FX_LARA_BUBBLES :
                        {
                            fxBubbles(room, JOINT_HEAD, _vec3i(0, 0, 50));
                            break;
                        }

                        case FX_LARA_HANDSFREE :
                        {
                            ASSERT(type == ITEM_LARA && extraL);
                            extraL->weaponState = WEAPON_STATE_FREE;
                            break;
                        }
                    /*
                        case FX_DRAW_RIGHTGUN  : drawGun(true); break;
                        case FX_DRAW_LEFTGUN   : drawGun(false); break;
                        case FX_SHOT_RIGHTGUN  : game->addMuzzleFlash(this, LARA_RGUN_JOINT, LARA_RGUN_OFFSET, 1 + camera->cameraIndex); break;
                        case FX_SHOT_LEFTGUN   : game->addMuzzleFlash(this, LARA_LGUN_JOINT, LARA_LGUN_OFFSET, 1 + camera->cameraIndex); break;
                        case FX_MESH_SWAP_1    : 
                        case FX_MESH_SWAP_2    : 
                        case FX_MESH_SWAP_3    : Character::cmdEffect(fx);
                        case 26 : break; // TODO TR2 reset_hair
                        case 32 : break; // TODO TR3 footprint
                        default : LOG("unknown effect command %d (anim %d)\n", fx, animation.index); ASSERT(false);
                    */
                        default : ;
                    }
                }
                ptr += 2;
                break;
            }
        }
    }
}

void ItemObj::animSkip(int32 stateBefore, int32 stateAfter, bool advance)
{
    goalState = stateBefore;

    vec3i p = pos;

    while (state != goalState)
    {
        animProcess(false);
    }

    if (advance) {
        animProcess();
    }

    pos = p;
    vSpeed = 0;
    hSpeed = 0;

    goalState = stateAfter;
}

#define ANIM_MOVE_LERP_POS_SHIFT    4
#define ANIM_MOVE_LERP_POS          (1 << ANIM_MOVE_LERP_POS_SHIFT)
#define ANIM_MOVE_LERP_ROT          ANGLE(2)

void ItemObj::animProcess(bool movement)
{
    ASSERT(level.models[type].count > 0);

    const Anim* anim = level.anims + animIndex;

#ifndef STATIC_ITEMS
    frameIndex++;
#endif

    anim = animChange(anim);

    if ((type != ITEM_LARA) && (nextState == state)) {
        nextState = 0;
    }

    if (frameIndex > anim->frameEnd)
    {
        animCmd(false, anim);

        frameIndex = anim->nextFrameIndex;
        animIndex = anim->nextAnimIndex;
        anim = level.anims + anim->nextAnimIndex;
        state = uint8(anim->state);

        if (type != ITEM_LARA)
        {
            goalState = state;
            if (nextState == state) {
                nextState = 0;
            }
        }
    }

    animCmd(true, anim);

#ifndef STATIC_ITEMS
    if (movement) {
        move();
    }
#endif
}

bool ItemObj::animIsEnd(int32 offset) const
{
    return frameIndex == level.anims[animIndex].frameEnd - offset;
}

void ItemObj::animHit(int32 dirX, int32 dirZ, int32 hitTimer)
{
    ASSERT(type == ITEM_LARA);
    ASSERT(extraL != NULL);

    extraL->hitQuadrant = uint16(angle.y - phd_atan(dirZ, dirX) + ANGLE_180 + ANGLE_45) >> ANGLE_SHIFT_90;
    extraL->hitTimer = hitTimer;
}

bool ItemObj::moveTo(const vec3i &point, ItemObj* item, bool lerp)
{
    // lerp position
    vec3i p = item->getRelative(point);

    if (!lerp)
    {
        pos = p;
        angle = item->angle;
        return true;
    }

    vec3i posDelta = p - pos;

    int32 dist = X_SQR(posDelta.x) + X_SQR(posDelta.y) + X_SQR(posDelta.z);

    if (dist > ANIM_MOVE_LERP_POS * ANIM_MOVE_LERP_POS)
    {
        dist = phd_sqrt(dist) >> 1;
        ASSERT(dist < DIV_TABLE_SIZE);
        dist = FixedInvU(dist);

        pos.x += posDelta.x * dist >> (16 + 1 - ANIM_MOVE_LERP_POS_SHIFT);
        pos.y += posDelta.y * dist >> (16 + 1 - ANIM_MOVE_LERP_POS_SHIFT);
        pos.z += posDelta.z * dist >> (16 + 1 - ANIM_MOVE_LERP_POS_SHIFT);
    }
    else
    {
        pos = p;
    }

    // lerp rotation
    angle.x = angleLerp(angle.x, item->angle.x, ANIM_MOVE_LERP_ROT);
    angle.y = angleLerp(angle.y, item->angle.y, ANIM_MOVE_LERP_ROT);
    angle.z = angleLerp(angle.z, item->angle.z, ANIM_MOVE_LERP_ROT);

    return (pos == p && angle == item->angle);
}

ItemObj* ItemObj::add(ItemType type, Room* room, const vec3i &pos, int32 angleY)
{
    if (!ItemObj::sFirstFree) {
        ASSERT(false);
        return NULL;
    }

    ItemObj* item = ItemObj::sFirstFree;
    ItemObj::sFirstFree = item->nextItem;

    item->type = type;
    item->pos = pos;
    item->angle.y = angleY;
    item->intensity = 128;

    item->init(room);

    return item;
}

void ItemObj::remove()
{
    deactivate();
    room->remove(this);

    for (int32 i = 0; i < MAX_PLAYERS; i++)
    {
        if (playersExtra[i].armR.target == this) playersExtra[i].armR.target = NULL;
        if (playersExtra[i].armL.target == this) playersExtra[i].armL.target = NULL;
    }

    nextItem = ItemObj::sFirstFree;
    ItemObj::sFirstFree = this;
}

void ItemObj::activate()
{
    //ASSERT(!flags.active) TODO check LEVEL3B

    flags |= ITEM_FLAG_ACTIVE;

    nextActive = ItemObj::sFirstActive;
    ItemObj::sFirstActive = this;
}

void ItemObj::deactivate()
{
    ItemObj* prev = NULL;
    ItemObj* curr = ItemObj::sFirstActive;

    while (curr)
    {
        ItemObj* next = curr->nextActive;

        if (curr == this)
        {
            flags &= ~ITEM_FLAG_ACTIVE;
            nextActive = NULL;

            if (prev) {
                prev->nextActive = next;
            } else {
                ItemObj::sFirstActive = next;
            }

            break;
        }

        prev = curr;
        curr = next;
    }
}

void ItemObj::hit(int32 damage, const vec3i &point, int32 soundId)
{
    //
}

void ItemObj::fxBubbles(Room *fxRoom, int32 fxJoint, const vec3i &fxOffset)
{
    int32 count = rand_draw() % 3;

    if (!count)
        return;

    vec3i fxPos = pos + getJoint(fxJoint, fxOffset);

    for (int32 i = 0; i < count; i++)
    {
        ItemObj::add(ITEM_BUBBLE, fxRoom, fxPos, 0);
    }
}

void ItemObj::fxRicochet(Room *fxRoom, const vec3i &fxPos, bool fxSound)
{
    ItemObj* ricochet = ItemObj::add(ITEM_RICOCHET, fxRoom, fxPos, 0);

    if (!ricochet)
        return;

    ricochet->timer = 4;
    ricochet->frameIndex = rand_draw() % (-level.models[ricochet->type].count);

    if (fxSound) {
        soundPlay(SND_RICOCHET, &ricochet->pos);
    }
}

void ItemObj::fxBlood(const vec3i &fxPos, int16 fxAngleY, int16 fxSpeed)
{
    ItemObj* blood = ItemObj::add(ITEM_BLOOD, room, fxPos, fxAngleY);
    
    if (!blood)
        return;

    blood->hSpeed = fxSpeed;
    blood->timer = 4;
    blood->flags |= ITEM_FLAG_ANIMATED;
}

void ItemObj::fxSmoke(const vec3i &fxPos)
{
    ItemObj* smoke = ItemObj::add(ITEM_SMOKE, room, fxPos, 0);
    
    if (!smoke)
        return;

    smoke->timer = 3;
    smoke->flags |= ITEM_FLAG_ANIMATED;
}

void ItemObj::fxSplash()
{
    vec3i fxPos = pos;
    fxPos.y = getWaterLevel();

    // TODO TR3+
    for (int32 i = 0; i < 10; i++)
    {
        ItemObj* splash = ItemObj::add(ITEM_SPLASH, room, fxPos, int16(rand_draw() - ANGLE_90) << 1);
    
        if (!splash)
            return;

        splash->hSpeed = int16(rand_draw() >> 8);
        splash->flags |= ITEM_FLAG_ANIMATED;
    }
}

void ItemObj::updateRoom(int32 offset)
{
    Room* nextRoom = room->getRoom(pos.x, pos.y + offset, pos.z);
        
    if (room != nextRoom)
    {
        room->remove(this);
        nextRoom->add(this);
    }

    const Sector* sector = room->getSector(pos.x, pos.z);
    roomFloor = sector->getFloor(pos.x, pos.y, pos.z);
}

bool ItemObj::isKeyHit(InputState state) const
{
    return (input & state) && !(extraL->lastInput & state);
}

vec3i ItemObj::getRelative(const vec3i &point) const
{
    matrixPush();

    Matrix &m = matrixGet();

    matrixSetIdentity();
    matrixRotateYXZ(angle.x, angle.y, angle.z);

    vec3i p;
    p.x = pos.x + (DP33(m.e00, m.e01, m.e02, point.x, point.y, point.z) >> FIXED_SHIFT);
    p.y = pos.y + (DP33(m.e10, m.e11, m.e12, point.x, point.y, point.z) >> FIXED_SHIFT);
    p.z = pos.z + (DP33(m.e20, m.e21, m.e22, point.x, point.y, point.z) >> FIXED_SHIFT);

    matrixPop();

    return p;
}

int32 ItemObj::getWaterLevel() const
{
    const Sector* sector = room->getWaterSector(pos.x, pos.z);
    if (sector) {
        if (sector->roomAbove == NO_ROOM) {
            return sector->getCeiling(pos.x, pos.y, pos.z);
        } else {
            return sector->ceiling << 8;
        }
    }

    return WALL;
}

int32 ItemObj::getWaterDepth() const
{
    const Sector* sector = room->getWaterSector(pos.x, pos.z);

    if (sector)
        return sector->getFloor(pos.x, pos.y, pos.z) - (sector->ceiling * 256);

    return WALL;
}

int32 ItemObj::getBridgeFloor(int32 x, int32 z) const
{
    if (type == ITEM_BRIDGE_FLAT)
        return pos.y;

    int32 h;
    if (angle.y == ANGLE_0) {
        h = 1024 - x;
    } else if (angle.y == ANGLE_180) {
        h = x;
    } else if (angle.y == ANGLE_90) {
        h = z;
    } else {
        h = 1024 - z;
    }

    h &= 1023;

    return pos.y + ((type == ITEM_BRIDGE_TILT_1) ? (h >> 2) : (h >> 1));
}

int32 ItemObj::getTrapDoorFloor(int32 x, int32 z) const
{
    int32 dx = (pos.x >> 10) - (x >> 10);
    int32 dz = (pos.z >> 10) - (z >> 10);

    if (((dx ==  0) && (dz ==  0)) ||
        ((dx ==  0) && (dz ==  1) && (angle.y ==  ANGLE_0))   ||
        ((dx ==  0) && (dz == -1) && (angle.y ==  ANGLE_180)) ||
        ((dx ==  1) && (dz ==  0) && (angle.y ==  ANGLE_90))  ||
        ((dx == -1) && (dz ==  0) && (angle.y == -ANGLE_90)))
    {
        return pos.y;
    }

    return WALL;
}

int32 ItemObj::getDrawBridgeFloor(int32 x, int32 z) const
{
    int32 dx = (pos.x >> 10) - (x >> 10);
    int32 dz = (pos.z >> 10) - (z >> 10);

    if (((dx == 0) && ((dz == -1) || (dz == -2)) && (angle.y ==  ANGLE_0))   ||
        ((dx == 0) && ((dz ==  1) || (dz ==  2)) && (angle.y ==  ANGLE_180)) ||
        ((dz == 0) && ((dx == -1) || (dz == -2)) && (angle.y ==  ANGLE_90))  ||
        ((dz == 0) && ((dx ==  1) || (dz ==  2)) && (angle.y == -ANGLE_90)))
    {
        return pos.y;
    }

    return WALL;
}

void ItemObj::getItemFloorCeiling(int32 x, int32 y, int32 z, int32* floor, int32* ceiling) const
{
    int32 h = WALL;

    switch (type)
    {
        case ITEM_TRAP_FLOOR:
        {
            if (state == 0 || state == 1) {
                h = pos.y - 512;
            }
            break;
        }
        case ITEM_DRAWBRIDGE:
        {
            if (state == 1) {
                h = getDrawBridgeFloor(x, z);
            }
            break;
        }
        case ITEM_BRIDGE_FLAT:
        case ITEM_BRIDGE_TILT_1:
        case ITEM_BRIDGE_TILT_2:
        {
            h = getBridgeFloor(x, z);
            break;
        }
        case ITEM_TRAP_DOOR_1:
        case ITEM_TRAP_DOOR_2:
        {
            if (state != 0)
                return;

            h = getTrapDoorFloor(x, z);

            if ((floor && (h >= *floor)) || (ceiling && (h <= *ceiling)))
                return;
        }
    }

    if (h == WALL)
        return;

    if (floor && (y <= h))
    {
        *floor = h;
    }

    if (ceiling && (y > h))
    {
        *ceiling = h + 256;
    }
}

vec3i ItemObj::getJoint(int32 jointIndex, const vec3i &offset) const
{
    const Model* model = level.models + type;

    const AnimFrame* frame = getFrame();

    const uint32* frameAngles = (uint32*)(frame->angles + 1);

    Matrix* oldMatrixPtr = gMatrixPtr;

    matrixPush();
    matrixSetIdentity();
    matrixRotateYXZ(angle.x, angle.y, angle.z);

    const ModelNode* node = level.nodes + model->nodeIndex;

    matrixFrame(&frame->pos, frameAngles);

    ASSERT(jointIndex < model->count);

    for (int32 i = 0; i < jointIndex; i++)
    {
        if (node->flags & 1) matrixPop();
        if (node->flags & 2) matrixPush();

        matrixFrame(&node->pos, ++frameAngles);

        // TODO extra rotations

        node++;
    }

    matrixTranslateRel(offset.x, offset.y, offset.z);

    Matrix &m = matrixGet();
    vec3i result = _vec3i(m.e03 >> (FIXED_SHIFT - MATRIX_FIXED_SHIFT),
                          m.e13 >> (FIXED_SHIFT - MATRIX_FIXED_SHIFT),
                          m.e23 >> (FIXED_SHIFT - MATRIX_FIXED_SHIFT));

    gMatrixPtr = oldMatrixPtr;

    return result;
}

int32 ItemObj::getSpheres(Sphere* spheres, bool flag) const
{
    const Model* model = level.models + type;

    const AnimFrame* frame = getFrame();
    const uint32* frameAngles = (uint32*)(frame->angles + 1);

    const Mesh** meshPtr = level.meshes + model->start;

    int32 x, y, z;

    if (flag) {
        x = pos.x;
        y = pos.y;
        z = pos.z;
        matrixPush();
        matrixSetIdentity();
    } else {
        x = y = z = 0;
        matrixPush();
        matrixTranslateAbs(pos.x, pos.y, pos.z);
    }

    matrixRotateYXZ(angle.x, angle.y, angle.z);

    const ModelNode* node = level.nodes + model->nodeIndex;

    matrixFrame(&frame->pos, frameAngles);

    Sphere* sphere = spheres;

    matrixPush();
    {
        const Mesh* mesh = *meshPtr;
        matrixTranslateRel(mesh->center.x, mesh->center.y, mesh->center.z);
        Matrix &m = matrixGet();
        sphere->center.x = x + (m.e03 >> (FIXED_SHIFT - MATRIX_FIXED_SHIFT));
        sphere->center.y = y + (m.e13 >> (FIXED_SHIFT - MATRIX_FIXED_SHIFT));
        sphere->center.z = z + (m.e23 >> (FIXED_SHIFT - MATRIX_FIXED_SHIFT));
        sphere->radius = mesh->radius;
        sphere++;
        meshPtr++;
    }
    matrixPop();
    
    for (int32 i = 1; i < model->count; i++)
    {
        if (node->flags & 1) matrixPop();
        if (node->flags & 2) matrixPush();

        matrixFrame(&node->pos, ++frameAngles);

        // TODO extra rotations

        matrixPush();
        {
            const Mesh* mesh = *meshPtr;
            matrixTranslateRel(mesh->center.x, mesh->center.y, mesh->center.z);
            Matrix &m = matrixGet();
            sphere->center.x = x + (m.e03 >> (FIXED_SHIFT - MATRIX_FIXED_SHIFT));
            sphere->center.y = y + (m.e13 >> (FIXED_SHIFT - MATRIX_FIXED_SHIFT));
            sphere->center.z = z + (m.e23 >> (FIXED_SHIFT - MATRIX_FIXED_SHIFT));
            sphere->radius = mesh->radius;
            sphere++;
            meshPtr++;
        }
        matrixPop();

        node++;
    }

    matrixPop();

    return sphere - spheres;
}

#include "lara.h"
#include "enemy.h"
#include "object.h"

#ifdef __GNUC__
// ItemObj ctor is called on existing and pre-filled memory
#pragma GCC diagnostic ignored "-Wuninitialized"
#endif

ItemObj::ItemObj(Room* room) 
{
    angle.x     = 0;
    angle.z     = 0;
    vSpeed      = 0;
    hSpeed      = 0;
    nextItem    = NULL;
    nextActive  = NULL;
    animIndex   = level.models[type].animIndex;
    frameIndex  = level.anims[animIndex].frameBegin;
    state       = uint8(level.anims[animIndex].state);
    nextState   = state;
    goalState   = state;
    extra       = NULL;
    health      = NOT_ENEMY;
    hitMask     = 0;
    visibleMask = 0xFFFFFFFF;

    flags &= (ITEM_FLAG_ONCE | ITEM_FLAG_MASK);

    if ((type == ITEM_BRIDGE_FLAT)   || 
        (type == ITEM_BRIDGE_TILT_1) || 
        (type == ITEM_BRIDGE_TILT_2) ||
        (type == ITEM_TRAP_FLOOR))
    {
        // no collision
    } else {
        flags |= ITEM_FLAG_COLLISION;
    }

    if (flags & ITEM_FLAG_ONCE) // once -> invisible
    {
        flags &= ~ITEM_FLAG_ONCE;
        flags |= ITEM_FLAG_STATUS_INVISIBLE;
    }

    if ((flags & ITEM_FLAG_MASK) == ITEM_FLAG_MASK) // full set of mask -> reverse
    {
        flags &= ~ITEM_FLAG_MASK;
        flags |= ITEM_FLAG_REVERSE;
        activate();
    }

    ASSERT(type <= ITEM_MAX);

    ASSERT(room);

    room->add(this);
}

#ifdef __GNUC__
#pragma GCC diagnostic warning "-Wuninitialized"
#endif

void ItemObj::update()
{
    //
}

void ItemObj::draw()
{
    if (level.models[type].count > 0) {
        drawModel(this);
    } else {
        drawSprite(this);
    }
}

struct ItemSave {
    int16 x;
    int16 y;
    int16 z; 
    int16 ax;
    int16 ay;
    uint16 animIndex;
    uint16 frameIndex;
    uint16 flags;
    uint16 timer;
    uint8 state;
    uint8 nextState;
    uint8 goalState;
    uint8 roomIndex;
};

uint8* ItemObj::save(uint8* data)
{
    ItemSave* sg = (ItemSave*)data;

    sg->x          = pos.x - (room->info->x << 8);
    sg->y          = pos.y - (room->info->yTop);
    sg->z          = pos.z - (room->info->z << 8);
    sg->ax         = angle.x;
    sg->ay         = angle.y;
    sg->animIndex  = animIndex;
    sg->frameIndex = frameIndex;
    sg->flags      = flags;
    sg->timer      = timer;
    sg->state      = state;
    sg->nextState  = nextState;
    sg->goalState  = goalState;
    sg->roomIndex  = room - rooms;

    return data + sizeof(ItemSave);
}

uint8* ItemObj::load(uint8* data)
{
    ItemSave* sg = (ItemSave*)data;

    if (room) {
        room->remove(this);
    }

    room = rooms + sg->roomIndex;
    room->add(this);

    pos.x       = sg->x + (room->info->x << 8);
    pos.y       = sg->y + (room->info->yTop);
    pos.z       = sg->z + (room->info->z << 8);
    angle.x     = sg->ax;
    angle.y     = sg->ay;
    animIndex   = sg->animIndex;
    frameIndex  = sg->frameIndex;
    flags       = sg->flags;
    timer       = sg->timer;
    state       = sg->state;
    nextState   = sg->nextState;
    goalState   = sg->goalState;

    return data + sizeof(ItemSave);
}

void ItemObj::collide(Lara* lara, CollisionInfo* cinfo)
{
    // empty
}

uint32 ItemObj::collideSpheres(Lara* lara) const
{
#ifdef FAST_HITMASK
    if (type != ITEM_TRAP_SWING_BLADE)
        return 0xFFFFFFFF;
#endif

    Sphere *a = gSpheres[0];
    Sphere *b = gSpheres[1];

    int32 aCount = getSpheres(a, true);
    int32 bCount = lara->getSpheres(b, true);

    uint32 mask = 0;

    for (int32 i = 0; i < aCount; i++)
    {
        if (a[i].radius <= 0)
            continue;

        for (int32 j = 0; j < bCount; j++)
        {
            if (b[j].radius <= 0)
                continue;

            vec3i d = b[j].center - a[i].center;
            int32 r = b[j].radius + a[i].radius;

            if (X_SQR(d.x) + X_SQR(d.y) + X_SQR(d.z) < X_SQR(r))
            {
                mask |= (1 << i);
            }
        }
    }

    return mask;
}

bool ItemObj::collideBounds(Lara* lara, CollisionInfo* cinfo) const
{
    const AABBs &a = getBoundingBox(false);
    const AABBs &b = lara->getBoundingBox(false);

    int32 dy = lara->pos.y - pos.y;

    if ((a.maxY - b.minY <= dy) ||
        (a.minY - b.maxY >= dy))
        return false;

    int32 dx = lara->pos.x - pos.x;
    int32 dz = lara->pos.z - pos.z;

    int32 s, c;
    sincos(angle.y, s, c);

    int32 px = X_ROTX(dx, dz, s, c);
    int32 pz = X_ROTY(dx, dz, s, c);

    int32 r = cinfo->radius;

    return (px >= a.minX - r) &&
           (px <= a.maxX + r) &&
           (pz >= a.minZ - r) &&
           (pz <= a.maxZ + r);
}

void ItemObj::collidePush(Lara* lara, CollisionInfo* cinfo, bool enemyHit) const
{
    int32 dx = lara->pos.x - pos.x;
    int32 dz = lara->pos.z - pos.z;

    int32 s, c;
    sincos(angle.y, s, c);

    int32 px = X_ROTX(dx, dz, s, c);
    int32 pz = X_ROTY(dx, dz, s, c);

    const AABBs &box = getBoundingBox(false);
    int32 minX = box.minX - cinfo->radius;
    int32 maxX = box.maxX + cinfo->radius;
    int32 minZ = box.minZ - cinfo->radius;
    int32 maxZ = box.maxZ + cinfo->radius;

    if ((px < minX) || (px > maxX) || (pz < minZ) || (pz > maxZ))
        return;

    enemyHit = enemyHit && cinfo->enemyHit && ((box.maxY - box.minY) > 256);

    int32 ax = px - minX;
    int32 bx = maxX - px;
    int32 az = pz - minZ;
    int32 bz = maxZ - pz;

    if (ax <= bx && ax <= az && ax <= bz) {
        px -= ax;
    } else if (bx <= ax && bx <= az && bx <= bz) {
        px += bx;
    } else if (az <= ax && az <= bx && az <= bz) {
        pz -= az;
    } else {
        pz += bz;
    }

    s = -s;

    lara->pos.x = pos.x + X_ROTX(px, pz, s, c);
    lara->pos.z = pos.z + X_ROTY(px, pz, s, c);

    if (enemyHit)
    {
        int32 cx = (minX + maxX) >> 1;
        int32 cz = (minZ + maxZ) >> 1;
        dx -= X_ROTX(cx, cz, s, c);
        dz -= X_ROTY(cx, cz, s, c);
        lara->animHit(dx, dz, 5);
    }

    int32 tmpAngle = cinfo->angle;

    cinfo->gapPos = -WALL;
    cinfo->gapNeg = -LARA_STEP_HEIGHT;
    cinfo->gapCeiling = 0;

    cinfo->setAngle(phd_atan(lara->pos.z - cinfo->pos.z, lara->pos.x - cinfo->pos.x));
    
    collideRoom(LARA_HEIGHT, 0);

    cinfo->setAngle(tmpAngle);

    if (cinfo->type != CT_NONE) {
        lara->pos.x = cinfo->pos.x;
        lara->pos.z = cinfo->pos.z;
    } else {
        cinfo->pos = lara->pos;
        lara->updateRoom(-10);
    }
}

void ItemObj::collideRoom(int32 height, int32 yOffset) const
{
    cinfo.type = CT_NONE;
    cinfo.offset = _vec3i(0, 0, 0);

    vec3i p = pos;
    p.y += yOffset;

    int32 y = p.y - height;

    int32 cy = y - 160;

    int32 floor, ceiling;

    Room* nextRoom = room;

    #define CHECK_HEIGHT(v) {\
        nextRoom = nextRoom->getRoom(v.x, cy, v.z);\
        const Sector* sector = nextRoom->getSector(v.x, v.z);\
        floor = sector->getFloor(v.x, cy, v.z);\
        if (floor != WALL) floor -= p.y;\
        ceiling = sector->getCeiling(v.x, cy, v.z);\
        if (ceiling != WALL) ceiling -= y;\
    }

// middle
    CHECK_HEIGHT(p);

    cinfo.trigger = gLastFloorData;
    cinfo.slantX = FD_SLANT_X(gLastFloorSlant);
    cinfo.slantZ = FD_SLANT_Z(gLastFloorSlant);

    cinfo.setSide(CollisionInfo::ST_MIDDLE, floor, ceiling);

    vec3i f, l, r;
    int32 R = cinfo.radius;

    int32 s, c;
    sincos(cinfo.angle, s, c);

    switch (cinfo.quadrant) {
        case 0 : {
            f = _vec3i((R * s) >> FIXED_SHIFT, 0, R);
            l = _vec3i(-R, 0,  R);
            r = _vec3i( R, 0,  R);
            break;
        }
        case 1 : {
            f = _vec3i( R, 0, (R * c) >> FIXED_SHIFT);
            l = _vec3i( R, 0,  R);
            r = _vec3i( R, 0, -R);
            break;
        }
        case 2 : {
            f = _vec3i((R * s) >> FIXED_SHIFT, 0, -R);
            l = _vec3i( R, 0, -R);
            r = _vec3i(-R, 0, -R);
            break;
        }
        case 3 : {
            f = _vec3i(-R, 0, (R * c) >> FIXED_SHIFT);
            l = _vec3i(-R, 0, -R);
            r = _vec3i(-R, 0,  R);
            break;
        }
        default : {
            f.x = f.y = f.z = 0;
            l.x = l.y = l.z = 0;
            r.x = r.y = r.z = 0;
            ASSERT(false);
        }
    }

    f += p;
    l += p;
    r += p;

    vec3i delta;
    delta.x = cinfo.pos.x - p.x;
    delta.y = cinfo.pos.y - p.y;
    delta.z = cinfo.pos.z - p.z;

// front
    CHECK_HEIGHT(f);
    cinfo.setSide(CollisionInfo::ST_FRONT, floor, ceiling);

// left
    CHECK_HEIGHT(l);
    cinfo.setSide(CollisionInfo::ST_LEFT, floor, ceiling);

// right
    CHECK_HEIGHT(r);
    cinfo.setSide(CollisionInfo::ST_RIGHT, floor, ceiling);

// static objects
    room->collideStatic(cinfo, p, height);

// check middle
    if (cinfo.m.floor == WALL)
    {
        cinfo.offset = delta;
        cinfo.type   = CT_FRONT;
        return;
    }

    if (cinfo.m.floor <= cinfo.m.ceiling)
    {
        cinfo.offset = delta;
        cinfo.type   = CT_FLOOR_CEILING;
        return;
    }

    if (cinfo.m.ceiling >= 0)
    {
        cinfo.offset.y = cinfo.m.ceiling;
        cinfo.type     = CT_CEILING;
    }

// front
    if (cinfo.f.floor > cinfo.gapPos || 
        cinfo.f.floor < cinfo.gapNeg ||
        cinfo.f.ceiling > cinfo.gapCeiling)
    {
        if (cinfo.quadrant & 1)
        {
            cinfo.offset.x = alignOffset(f.x, p.x);
            cinfo.offset.z = delta.z;
        } else {
            cinfo.offset.x = delta.x;
            cinfo.offset.z = alignOffset(f.z, p.z);
        }

        cinfo.type = CT_FRONT;
        return;
    }

// front ceiling
    if (cinfo.f.ceiling >= cinfo.gapCeiling)
    {
        cinfo.offset = delta;
        cinfo.type   = CT_FRONT_CEILING;
        return;
    }

// left
    if (cinfo.l.floor > cinfo.gapPos || cinfo.l.floor < cinfo.gapNeg)
    {
        if (cinfo.quadrant & 1) {
            cinfo.offset.z = alignOffset(l.z, f.z);
        } else {
            cinfo.offset.x = alignOffset(l.x, f.x);
        }
        cinfo.type = CT_LEFT;
        return;
    }

// right
    if (cinfo.r.floor > cinfo.gapPos || cinfo.r.floor < cinfo.gapNeg)
    {
        if (cinfo.quadrant & 1) {
            cinfo.offset.z = alignOffset(r.z, f.z);
        } else {
            cinfo.offset.x = alignOffset(r.x, f.x);
        }
        cinfo.type = CT_RIGHT;
        return;
    }
}

uint32 ItemObj::updateHitMask(Lara* lara, CollisionInfo* cinfo)
{
    hitMask = 0;

    if (!collideBounds(lara, cinfo)) // check bound box intersection
        return false;

    hitMask = collideSpheres(lara); // get hitMask = spheres collision mask

    return hitMask;
}

void ItemObj::meshSwap(ItemType type, uint32 mask)
{
    int32 start = level.models[type].start;

    for (int32 i = 0; i < JOINT_MAX && mask; i++, mask >>= 1)
    {
        if (mask & 1) {
            extraL->meshes[i] = start + i;
        }
    }
}

ItemObj* ItemObj::init(Room* room)
{
    #define INIT_ITEM(type, className) case ITEM_##type : return new (this) className(room)

    switch (type)
    {
        INIT_ITEM( LARA                  , Lara );
        INIT_ITEM( DOPPELGANGER          , Doppelganger );
        INIT_ITEM( WOLF                  , Wolf );
        INIT_ITEM( BEAR                  , Bear );
        INIT_ITEM( BAT                   , Bat );
        INIT_ITEM( CROCODILE_LAND        , Crocodile );
        INIT_ITEM( CROCODILE_WATER       , Crocodile );
        INIT_ITEM( LION_MALE             , Lion );
        INIT_ITEM( LION_FEMALE           , Lion );
        INIT_ITEM( PUMA                  , Lion );
        INIT_ITEM( GORILLA               , Gorilla );
        INIT_ITEM( RAT_LAND              , Rat );
        INIT_ITEM( RAT_WATER             , Rat );
        INIT_ITEM( REX                   , Rex );
        INIT_ITEM( RAPTOR                , Raptor );
        INIT_ITEM( MUTANT_1              , Mutant );
        INIT_ITEM( MUTANT_2              , Mutant );
        INIT_ITEM( MUTANT_3              , Mutant );
        INIT_ITEM( CENTAUR               , Centaur );
        INIT_ITEM( MUMMY                 , Mummy );
        // INIT_ITEM( UNUSED_1              , ??? );
        // INIT_ITEM( UNUSED_2              , ??? );
        INIT_ITEM( LARSON                , Larson );
        INIT_ITEM( PIERRE                , Pierre );
        // INIT_ITEM( SKATEBOARD            , ??? );
        INIT_ITEM( SKATER                , Skater );
        INIT_ITEM( COWBOY                , Cowboy );
        INIT_ITEM( MR_T                  , MrT );
        INIT_ITEM( NATLA                 , Natla );
        INIT_ITEM( ADAM                  , Adam );
        INIT_ITEM( TRAP_FLOOR            , TrapFloor );
        INIT_ITEM( TRAP_SWING_BLADE      , TrapSwingBlade );
        // INIT_ITEM( TRAP_SPIKES           , ??? );
        // INIT_ITEM( TRAP_BOULDER          , ??? );
        INIT_ITEM( DART                  , Dart );
        INIT_ITEM( TRAP_DART_EMITTER     , TrapDartEmitter );
        // INIT_ITEM( DRAWBRIDGE            , ??? );
        // INIT_ITEM( TRAP_SLAM             , ??? );
        // INIT_ITEM( TRAP_SWORD            , ??? );
        // INIT_ITEM( HAMMER_HANDLE         , ??? );
        // INIT_ITEM( HAMMER_BLOCK          , ??? );
        // INIT_ITEM( LIGHTNING             , ??? );
        // INIT_ITEM( MOVING_OBJECT         , ??? );
        INIT_ITEM( BLOCK_1               , Block );
        INIT_ITEM( BLOCK_2               , Block );
        INIT_ITEM( BLOCK_3               , Block );
        INIT_ITEM( BLOCK_4               , Block );
        // INIT_ITEM( MOVING_BLOCK          , ??? );
        // INIT_ITEM( TRAP_CEILING          , ??? );
        // INIT_ITEM( TRAP_FLOOR_LOD        , ??? );
        INIT_ITEM( SWITCH                , Switch );
        INIT_ITEM( SWITCH_WATER          , SwitchWater );
        INIT_ITEM( DOOR_1                , Door );
        INIT_ITEM( DOOR_2                , Door );
        INIT_ITEM( DOOR_3                , Door );
        INIT_ITEM( DOOR_4                , Door );
        INIT_ITEM( DOOR_5                , Door );
        INIT_ITEM( DOOR_6                , Door );
        INIT_ITEM( DOOR_7                , Door );
        INIT_ITEM( DOOR_8                , Door );
        INIT_ITEM( TRAP_DOOR_1           , TrapDoor );
        INIT_ITEM( TRAP_DOOR_2           , TrapDoor );
        // INIT_ITEM( TRAP_DOOR_LOD         , ??? );
        // INIT_ITEM( BRIDGE_FLAT           , ??? );
        // INIT_ITEM( BRIDGE_TILT_1         , ??? );
        // INIT_ITEM( BRIDGE_TILT_2         , ??? );
        // INIT_ITEM( INV_PASSPORT          , ??? );
        // INIT_ITEM( INV_COMPASS           , ??? );
        // INIT_ITEM( INV_HOME              , ??? );
        INIT_ITEM( GEARS_1               , Gears );
        INIT_ITEM( GEARS_2               , Gears );
        INIT_ITEM( GEARS_3               , Gears );
        INIT_ITEM( CUT_1                 , CinematicObject );
        INIT_ITEM( CUT_2                 , CinematicObject );
        INIT_ITEM( CUT_3                 , CinematicObject );
        INIT_ITEM( CUT_4                 , CinematicObject );
        // INIT_ITEM( INV_PASSPORT_CLOSED   , ??? );
        // INIT_ITEM( INV_MAP               , ??? );
        INIT_ITEM( CRYSTAL               , Crystal );
        INIT_ITEM( PISTOLS               , Pickup );
        INIT_ITEM( SHOTGUN               , Pickup );
        INIT_ITEM( MAGNUMS               , Pickup );
        INIT_ITEM( UZIS                  , Pickup );
        INIT_ITEM( AMMO_PISTOLS          , Pickup );
        INIT_ITEM( AMMO_SHOTGUN          , Pickup );
        INIT_ITEM( AMMO_MAGNUMS          , Pickup );
        INIT_ITEM( AMMO_UZIS             , Pickup );
        INIT_ITEM( EXPLOSIVE             , Pickup );
        INIT_ITEM( MEDIKIT_SMALL         , Pickup );
        INIT_ITEM( MEDIKIT_BIG           , Pickup );
        // INIT_ITEM( INV_DETAIL            , ??? );
        // INIT_ITEM( INV_SOUND             , ??? );
        // INIT_ITEM( INV_CONTROLS          , ??? );
        // INIT_ITEM( INV_GAMMA             , ??? );
        // INIT_ITEM( INV_PISTOLS           , ??? );
        // INIT_ITEM( INV_SHOTGUN           , ??? );
        // INIT_ITEM( INV_MAGNUMS           , ??? );
        // INIT_ITEM( INV_UZIS              , ??? );
        // INIT_ITEM( INV_AMMO_PISTOLS      , ??? );
        // INIT_ITEM( INV_AMMO_SHOTGUN      , ??? );
        // INIT_ITEM( INV_AMMO_MAGNUMS      , ??? );
        // INIT_ITEM( INV_AMMO_UZIS         , ??? );
        // INIT_ITEM( INV_EXPLOSIVE         , ??? );
        // INIT_ITEM( INV_MEDIKIT_SMALL     , ??? );
        // INIT_ITEM( INV_MEDIKIT_BIG       , ??? );
        INIT_ITEM( PUZZLE_1              , Pickup );
        INIT_ITEM( PUZZLE_2              , Pickup );
        INIT_ITEM( PUZZLE_3              , Pickup );
        INIT_ITEM( PUZZLE_4              , Pickup );
        // INIT_ITEM( INV_PUZZLE_1          , ??? );
        // INIT_ITEM( INV_PUZZLE_2          , ??? );
        // INIT_ITEM( INV_PUZZLE_3          , ??? );
        // INIT_ITEM( INV_PUZZLE_4          , ??? );
        INIT_ITEM( PUZZLEHOLE_1          , PuzzleHole );
        INIT_ITEM( PUZZLEHOLE_2          , PuzzleHole );
        INIT_ITEM( PUZZLEHOLE_3          , PuzzleHole );
        INIT_ITEM( PUZZLEHOLE_4          , PuzzleHole );
        // INIT_ITEM( PUZZLE_DONE_1         , ??? );
        // INIT_ITEM( PUZZLE_DONE_2         , ??? );
        // INIT_ITEM( PUZZLE_DONE_3         , ??? );
        // INIT_ITEM( PUZZLE_DONE_4         , ??? );
        INIT_ITEM( LEADBAR               , Pickup );
        // INIT_ITEM( INV_LEADBAR           , ??? );
        // INIT_ITEM( MIDAS_HAND            , ??? );
        INIT_ITEM( KEY_ITEM_1            , Pickup );
        INIT_ITEM( KEY_ITEM_2            , Pickup );
        INIT_ITEM( KEY_ITEM_3            , Pickup );
        INIT_ITEM( KEY_ITEM_4            , Pickup );
        // INIT_ITEM( INV_KEY_ITEM_1        , ??? );
        // INIT_ITEM( INV_KEY_ITEM_2        , ??? );
        // INIT_ITEM( INV_KEY_ITEM_3        , ??? );
        // INIT_ITEM( INV_KEY_ITEM_4        , ??? );
        INIT_ITEM( KEYHOLE_1             , KeyHole );
        INIT_ITEM( KEYHOLE_2             , KeyHole );
        INIT_ITEM( KEYHOLE_3             , KeyHole );
        INIT_ITEM( KEYHOLE_4             , KeyHole );
        // INIT_ITEM( UNUSED_4              , ??? );
        // INIT_ITEM( UNUSED_5              , ??? );
        // INIT_ITEM( SCION_PICKUP_QUALOPEC , ??? );
        INIT_ITEM( SCION_PICKUP_DROP     , Pickup );
        INIT_ITEM( SCION_TARGET          , ViewTarget );
        // INIT_ITEM( SCION_PICKUP_HOLDER   , ??? );
        // INIT_ITEM( SCION_HOLDER          , ??? );
        // INIT_ITEM( UNUSED_6              , ??? );
        // INIT_ITEM( UNUSED_7              , ??? );
        // INIT_ITEM( INV_SCION             , ??? );
        // INIT_ITEM( EXPLOSION             , ??? );
        // INIT_ITEM( UNUSED_8              , ??? );
        INIT_ITEM( SPLASH                , SpriteEffect );
        // INIT_ITEM( UNUSED_9              , ??? );
        INIT_ITEM( BUBBLE                , Bubble );
        // INIT_ITEM( UNUSED_10             , ??? );
        // INIT_ITEM( UNUSED_11             , ??? );
        INIT_ITEM( BLOOD                 , SpriteEffect );
        // INIT_ITEM( UNUSED_12             , ??? );
        INIT_ITEM( SMOKE                 , SpriteEffect );
        // INIT_ITEM( CENTAUR_STATUE        , ??? );
        // INIT_ITEM( CABIN                 , ??? );
        // INIT_ITEM( MUTANT_EGG_SMALL      , ??? );
        INIT_ITEM( RICOCHET              , SpriteEffect );
        INIT_ITEM( SPARKLES              , SpriteEffect );
        // INIT_ITEM( MUZZLE_FLASH          , ??? );
        // INIT_ITEM( UNUSED_13             , ??? );
        // INIT_ITEM( UNUSED_14             , ??? );
        INIT_ITEM( VIEW_TARGET           , ViewTarget );
        INIT_ITEM( WATERFALL             , Waterfall );
        // INIT_ITEM( NATLA_BULLET          , ??? );
        // INIT_ITEM( MUTANT_BULLET         , ??? );
        // INIT_ITEM( CENTAUR_BULLET        , ??? );
        // INIT_ITEM( UNUSED_15             , ??? );
        // INIT_ITEM( UNUSED_16             , ??? );
        // INIT_ITEM( LAVA_PARTICLE         , ??? );
        INIT_ITEM( LAVA_EMITTER          , LavaEmitter );
        // INIT_ITEM( FLAME                 , ??? );
        // INIT_ITEM( FLAME_EMITTER         , ??? );
        // INIT_ITEM( TRAP_LAVA             , ??? );
        // INIT_ITEM( MUTANT_EGG_BIG        , ??? );
        // INIT_ITEM( BOAT                  , ??? );
        // INIT_ITEM( EARTHQUAKE            , ??? );
        // INIT_ITEM( UNUSED_17             , ??? );
        // INIT_ITEM( UNUSED_18             , ??? );
        // INIT_ITEM( UNUSED_19             , ??? );
        // INIT_ITEM( UNUSED_20             , ??? );
        // INIT_ITEM( UNUSED_21             , ??? );
        // INIT_ITEM( LARA_BRAID            , ??? );
        // INIT_ITEM( GLYPHS                , ??? );
    }

    return new (this) ItemObj(room);
}

#endif
