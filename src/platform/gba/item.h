#ifndef H_ITEM
#define H_ITEM

#include "common.h"
#include "sound.h"
#include "camera.h"

int32 curItemIndex;

#define GRAVITY      6

int16 angleDec(int16 angle, int32 value) {
    if (angle < -value) {
        return angle + value;
    } else if (angle > value) {
        return angle - value;
    }
    return 0;
}

AnimFrame* getFrame(const Item* item, const Model* model)
{
    const Anim* anim = anims + item->animIndex;

    int32 frameSize = sizeof(AnimFrame) / 2 + model->mCount * 2;
    int32 frameIndex = (item->frameIndex - anim->frameBegin) / anim->frameRate; // TODO fixed div? check the range

    return (AnimFrame*)(animFrames + anim->frameOffset / 2 + frameIndex * frameSize);
}

const Box& getBoundingBox(const Item* item)
{
    const Model* model = models + modelsMap[item->type];
    AnimFrame* frame = getFrame(item, model);
    return frame->box;
}

Sound::Sample* soundPlay(int16 id, const vec3i &pos)
{
    int16 a = soundMap[id];

    if (a == -1) {
        return NULL;
    }

    const SoundInfo* b = soundInfos + a;

    if (b->chance && b->chance < xRand()) {
        return NULL;
    }

    vec3i d = pos - camera.targetPos;

    if (abs(d.x) >= SND_MAX_DIST || abs(d.y) >= SND_MAX_DIST || abs(d.z) >= SND_MAX_DIST) {
        return NULL;
    }

    int32 volume = b->volume - (phd_sqrt(dot(d, d)) << 2);

    if (b->flags.gain) {
        volume -= xRand() >> 2;
    }

    if (volume <= 0) {
        return NULL;
    }

    volume = X_MIN(volume, 0x7FFF);

    int32 pitch = 128;

    if (b->flags.pitch) {
        pitch += ((xRand() * 13) >> 14) - 13;
    }

    int32 index = b->index;
    if (b->flags.count > 1) {
        index += (xRand() * b->flags.count) >> 15;
    }

    const uint8 *data = soundData + soundOffsets[index];

    int32 size;
    memcpy(&size, data + 40, 4); // TODO preprocess and remove wave header
    data += 44;

    return sound.play(data, size, volume, pitch, b->flags.mode);
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

    item->pos.x += phd_sin(item->moveAngle) * item->hSpeed >> FIXED_SHIFT;
    item->pos.z += phd_cos(item->moveAngle) * item->hSpeed >> FIXED_SHIFT;
}

const Anim* animSet(Item* item, int32 animIndex, bool resetState, int32 frameOffset = 0) {
    const Anim* anim = anims + animIndex;

    item->animIndex   = animIndex;
    item->frameIndex  = anim->frameBegin + frameOffset;

    if (resetState) {
        item->state = item->goalState = anim->state;
    }

    return anim;
}

const Anim* animChange(Item* item, const Anim* anim)
{
    if (!anim->statesCount || item->goalState == item->state)
        return anim;

    const AnimState* animState = animStates + anim->statesStart;

    for (int32 i = 0; i < anim->statesCount; i++)
    {
        if (item->goalState == animState->state)
        {
            const AnimRange* animRange = animRanges + animState->rangesStart;

            for (int32 j = 0; j < animState->rangesCount; j++)
            {
                if ((item->frameIndex >= animRange->frameBegin) && (item->frameIndex <= animRange->frameEnd))
                {
                    if ((item->type != ITEM_LARA) && (item->nextState == animState->state)) {
                        item->nextState = 0;
                    }

                    item->frameIndex = animRange->nextFrameIndex;
                    item->animIndex = animRange->nextAnimIndex;
                    anim = anims + animRange->nextAnimIndex;
                    item->state = anim->state;
                    return anim;
                }
                animRange++;
            }
        }
        animState++;
    }

    return anim;
}

void animCmd(bool fx, Item* item, const Anim* anim)
{
    if (!anim->commandsCount) return;

    const int16 *ptr = animCommands + anim->commandsStart;

    for (int32 i = 0; i < anim->commandsCount; i++)
    {
        int32 cmd = *ptr++;

        switch (cmd) {
            case ANIM_CMD_NONE:
                break;
            case ANIM_CMD_OFFSET:
            {
                if (!fx)
                {
                    int32 s = phd_sin(item->moveAngle);
                    int32 c = phd_cos(item->moveAngle);
                    int32 x = ptr[0];
                    int32 y = ptr[1];
                    int32 z = ptr[2];
                    item->pos.x += (c * x + s * z) >> FIXED_SHIFT;
                    item->pos.y += y;
                    item->pos.z += (c * z - s * x) >> FIXED_SHIFT;
                }
                ptr += 3;
                break;
            }
            case ANIM_CMD_JUMP:
                if (!fx)
                {
                    if (item->vSpeedHack) {
                        item->vSpeed = item->vSpeedHack;
                        item->vSpeedHack = 0;
                    } else {
                        item->vSpeed = ptr[0];
                    }
                    item->hSpeed = ptr[1];
                    item->flags.gravity = true;
                }
                ptr += 2;
                break;
            case ANIM_CMD_EMPTY:
                break;
            case ANIM_CMD_KILL:
                break;
            case ANIM_CMD_SOUND:
                if (fx && item->frameIndex == ptr[0])
                {
                    soundPlay(ptr[1] & 0x03FFF, item->pos);
                }
                ptr += 2;
                break;
            case ANIM_CMD_EFFECT:
                if (fx && item->frameIndex == ptr[0]) {
                    switch (ptr[1]) {
                        case FX_ROTATE_180     : item->angleY -= ANGLE_180; break;
                    /*
                        case FX_FLOOR_SHAKE    : ASSERT(false);
                        case FX_LARA_NORMAL    : animation.setAnim(ANIM_STAND); break;
                        case FX_LARA_BUBBLES   : doBubbles(); break;
                        case FX_LARA_HANDSFREE : break;//meshSwap(1, level->extra.weapons[wpnCurrent], BODY_LEG_L1 | BODY_LEG_R1); break;
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

void animUpdate(Item* item)
{
    if (modelsMap[item->type] == NO_MODEL)
        return;

    const Anim* anim = anims + item->animIndex;

    item->frameIndex++;

    anim = animChange(item, anim);

    if (item->frameIndex > anim->frameEnd)
    {
        animCmd(false, item, anim);

        item->frameIndex = anim->nextFrameIndex;
        item->animIndex = anim->nextAnimIndex;
        anim = anims + anim->nextAnimIndex;
        item->state = anim->state;
    }

    animCmd(true, item, anim);

    move(item, anim);
}

int32 calcLight(const vec3i &pos, int32 roomIndex)
{
    Room* room = rooms + roomIndex;

    if (!room->lCount) {
        return room->ambient;
    }

    int32 ambient = 8191 - room->ambient;
    int32 intensity = 0;

    for (int i = 0; i < room->lCount; i++)
    {
        const RoomInfo::Light* light = room->lights + i;

        // TODO preprocess align
        vec3i lpos;
        int32 lradius;
        memcpy(&lpos, &light->pos[0], sizeof(lpos));
        memcpy(&lradius, &light->radius[0], sizeof(lradius));

        vec3i d = pos - lpos;
        int32 dist = dot(d, d) >> 12;
        int32 att = X_SQR(lradius) >> 12;

        int32 lum = (light->intensity * att) / (dist + att) + ambient;

        if (lum > intensity)
        {
            intensity = lum;
        }
    }

    return 8191 - ((intensity + ambient) >> 1);
}

int32 itemCalcLighting(const Item* item, const Box& box)
{
    matrixPush();
    Matrix &m = matrixGet();
    m[0][3] = m[1][3] = m[2][3] = 0;
    matrixRotateYXZ(item->angleX, item->angleY, item->angleZ);

    vec3i v((box.maxX + box.minX) >> 1,
            (box.maxY + box.minY) >> 1,
            (box.maxZ + box.minZ) >> 1);

    matrixTranslate(v);

    v = vec3i(m[0][3] >> FIXED_SHIFT,
              m[1][3] >> FIXED_SHIFT,
              m[2][3] >> FIXED_SHIFT) + item->pos;
    matrixPop();

    return calcLight(v, item->room);
}

void itemControl(Item* item)
{
    animUpdate(item);
}

void itemInit(Item* item)
{
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
    item->flags.gravity = false;
    item->flags.shadow = true;

    switch (item->type) {
        case ITEM_LARA            : item->health = 1000; break;
        case ITEM_DOPPELGANGER    : item->health = 1000; break;
        case ITEM_WOLF            : item->health = 6;    break;
        case ITEM_BEAR            : item->health = 20;   break;
        case ITEM_BAT             : item->health = 1;    break;
        case ITEM_CROCODILE_LAND  : item->health = 20;   break;
        case ITEM_CROCODILE_WATER : item->health = 20;   break;
        case ITEM_LION_MALE       : item->health = 30;   break;
        case ITEM_LION_FEMALE     : item->health = 25;   break;
        case ITEM_PUMA            : item->health = 45;   break;
        case ITEM_GORILLA         : item->health = 22;   break;
        case ITEM_RAT_LAND        : item->health = 5;    break;
        case ITEM_RAT_WATER       : item->health = 5;    break;
        case ITEM_REX             : item->health = 100;  break;
        case ITEM_RAPTOR          : item->health = 20;   break;
        case ITEM_MUTANT_1        : item->health = 50;   break;
        case ITEM_MUTANT_2        : item->health = 50;   break;
        case ITEM_MUTANT_3        : item->health = 50;   break;
        case ITEM_CENTAUR         : item->health = 120;  break;
        case ITEM_MUMMY           : item->health = 18;   break;
        case ITEM_LARSON          : item->health = 50;   break;
        case ITEM_PIERRE          : item->health = 70;   break;
        case ITEM_SKATEBOY        : item->health = 125;  break;
        case ITEM_COWBOY          : item->health = 150;  break;
        case ITEM_MR_T            : item->health = 200;  break;
        case ITEM_NATLA           : item->health = 400;  break;
        case ITEM_GIANT_MUTANT    : item->health = 500;  break;
        default                   : 
            item->health = 0;
            item->flags.shadow = false;
    }
}

#endif
