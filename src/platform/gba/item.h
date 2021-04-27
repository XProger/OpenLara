#ifndef H_ITEM
#define H_ITEM

#include "common.h"
#include "sound.h"
#include "camera.h"
#include "draw.h"

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

Mixer::Sample* soundPlay(int16 id, const vec3i &pos)
{
    int16 a = soundMap[id];

    if (a == -1) {
        return NULL;
    }

    const SoundInfo* b = soundInfos + a;

    if (b->chance && b->chance < xRand()) {
        return NULL;
    }

    vec3i d = pos - camera.target.pos;

    if (abs(d.x) >= SND_MAX_DIST || abs(d.y) >= SND_MAX_DIST || abs(d.z) >= SND_MAX_DIST) {
        return NULL;
    }

    int32 volume = b->volume - (phd_sqrt(dot(d, d)) << 2);

    if (b->flags.gain) {
        volume -= xRand() >> 2;
    }

    volume = X_MIN(volume, 0x7FFF) >> 9;

    if (volume <= 0) {
        return NULL;
    }

    int32 pitch = 128;

    if (b->flags.pitch) {
        pitch += ((xRand() * 13) >> 14) - 13;
    }

    int32 index = b->index;
    if (b->flags.count > 1) {
        index += (xRand() * b->flags.count) >> 15;
    }

    int32 offset;
    memcpy(&offset, soundOffsets + index, 4); // TODO preprocess and remove wave header

    const uint8 *data = soundData + offset;//soundOffsets[index];

    int32 size;
    memcpy(&size, data + 40, 4); // TODO preprocess and remove wave header
    data += 44;

    return mixer.playSample(data, size, volume, pitch, b->flags.mode);
}

AnimFrame* Item::getFrame(const Model* model) const
{
    const Anim* anim = anims + animIndex;

    int32 frameSize = sizeof(AnimFrame) / 2 + model->mCount * 2;
    int32 keyFrame = (frameIndex - anim->frameBegin) / anim->frameRate; // TODO fixed div? check the range

    return (AnimFrame*)(animFrames + anim->frameOffset / 2 + keyFrame * frameSize);
}

const Box& Item::getBoundingBox() const
{
    const Model* model = models + modelsMap[type];
    AnimFrame* frame = getFrame(model);
    return frame->box;
}

void Item::move()
{
    const Anim* anim = anims + animIndex;

    int32 s = anim->speed;

    if (flags.gravity)
    {
        s += anim->accel * (frameIndex - anim->frameBegin - 1);
        hSpeed -= s >> 16;
        s += anim->accel;
        hSpeed += s >> 16;

        vSpeed += (vSpeed < 128) ? GRAVITY : 1;

        pos.y += vSpeed;
    } else {
        s += anim->accel * (frameIndex - anim->frameBegin);
    
        hSpeed = s >> 16;
    }

    int16 angle = (type == ITEM_LARA) ? moveAngle : angleY;

    pos.x += phd_sin(angle) * hSpeed >> FIXED_SHIFT;
    pos.z += phd_cos(angle) * hSpeed >> FIXED_SHIFT;
}

const Anim* Item::animSet(int32 newAnimIndex, bool resetState, int32 frameOffset)
{
    const Anim* anim = anims + newAnimIndex;

    animIndex   = newAnimIndex;
    frameIndex  = anim->frameBegin + frameOffset;

    if (resetState) {
        state = goalState = uint8(anim->state);
    }

    return anim;
}

const Anim* Item::animChange(const Anim* anim)
{
    if (!anim->statesCount || goalState == state)
        return anim;

    const AnimState* animState = animStates + anim->statesStart;

    for (int32 i = 0; i < anim->statesCount; i++)
    {
        if (goalState == animState->state)
        {
            const AnimRange* animRange = animRanges + animState->rangesStart;

            for (int32 j = 0; j < animState->rangesCount; j++)
            {
                if ((frameIndex >= animRange->frameBegin) && (frameIndex <= animRange->frameEnd))
                {
                    if ((type != ITEM_LARA) && (nextState == animState->state)) {
                        nextState = 0;
                    }

                    frameIndex = animRange->nextFrameIndex;
                    animIndex = animRange->nextAnimIndex;
                    anim = anims + animRange->nextAnimIndex;
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

void Item::animCmd(bool fx, const Anim* anim)
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
                    int32 s = phd_sin(moveAngle);
                    int32 c = phd_cos(moveAngle);
                    int32 x = ptr[0];
                    int32 y = ptr[1];
                    int32 z = ptr[2];
                    pos.x += (c * x + s * z) >> FIXED_SHIFT;
                    pos.y += y;
                    pos.z += (c * z - s * x) >> FIXED_SHIFT;
                }
                ptr += 3;
                break;
            }
            case ANIM_CMD_JUMP:
                if (!fx)
                {
                    if (vSpeedHack) {
                        vSpeed = vSpeedHack;
                        vSpeedHack = 0;
                    } else {
                        vSpeed = ptr[0];
                    }
                    hSpeed = ptr[1];
                    flags.gravity = true;
                }
                ptr += 2;
                break;
            case ANIM_CMD_EMPTY:
                break;
            case ANIM_CMD_KILL:
                if (!fx)
                {
                    flags.status = ITEM_FLAGS_STATUS_INACTIVE;
                }
                break;
            case ANIM_CMD_SOUND:
                if (fx && frameIndex == ptr[0])
                {
                    soundPlay(ptr[1] & 0x03FFF, pos);
                }
                ptr += 2;
                break;
            case ANIM_CMD_EFFECT:
                if (fx && frameIndex == ptr[0]) {
                    switch (ptr[1]) {
                        case FX_ROTATE_180     : angleY += ANGLE_180; break;
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

void Item::skipAnim()
{
    vec3i p = pos;

    while (state != goalState)
    {
        updateAnim(false);
    }

    pos = p;
    vSpeed = 0;
    hSpeed = 0;
}

void Item::updateAnim(bool movement)
{
    if (modelsMap[type] == NO_MODEL)
        return;

    const Anim* anim = anims + animIndex;

#ifndef STATIC_ITEMS
    frameIndex++;
#endif

    anim = animChange(anim);

    if (frameIndex > anim->frameEnd)
    {
        animCmd(false, anim);

        frameIndex = anim->nextFrameIndex;
        animIndex = anim->nextAnimIndex;
        anim = anims + anim->nextAnimIndex;
        state = uint8(anim->state);
    }

    animCmd(true, anim);

#ifndef STATIC_ITEMS
    if (movement) {
        move();
    }
#endif
}

Item* Item::add(ItemType type, Room* room, const vec3i &pos, int32 angleY)
{
    if (!Item::sFirstFree) {
        ASSERT(false);
        return NULL;
    }

    Item* item = Item::sFirstFree;
    Item::sFirstFree = item->nextItem;

    item->type = type;
    item->pos = pos;
    item->angleY = angleY;
    item->intensity = -1;

    item->init(room);

    return item;
}

void Item::remove()
{
    deactivate();
    room->remove(this);

    nextItem = Item::sFirstFree;
    Item::sFirstFree = this;
}

void Item::activate()
{
    ASSERT(!flags.active)

    flags.active = true;

    nextActive = sFirstActive;
    sFirstActive = this;
}

void Item::deactivate()
{
    Item* prev = NULL;
    Item* curr = sFirstActive;

    while (curr)
    {
        Item* next = curr->nextActive;

        if (curr == this)
        {
            flags.active = false;
            nextActive = NULL;

            if (prev) {
                prev->nextActive = next;
            } else {
                sFirstActive = next;
            }

            break;
        }

        prev = curr;
        curr = next;
    }
}

void Item::updateRoom(int32 offset)
{
    Room* nextRoom = room->getRoom(pos.x, pos.y + offset, pos.z);
        
    if (room != nextRoom)
    {
        room->remove(this);
        nextRoom->add(this);
    }

    const RoomInfo::Sector* sector = room->getSector(pos.x, pos.z);
    floor = sector->getFloor(pos.x, pos.y, pos.z);
}

int32 Item::calcLighting(const Box& box) const
{
    matrixPush();
    Matrix &m = matrixGet();
    m[0][3] = m[1][3] = m[2][3] = 0;
    matrixRotateYXZ(angleX, angleY, angleZ);

    vec3i p((box.maxX + box.minX) >> 1,
            (box.maxY + box.minY) >> 1,
            (box.maxZ + box.minZ) >> 1);

    matrixTranslate(p);

    p = vec3i(m[0][3] >> FIXED_SHIFT,
              m[1][3] >> FIXED_SHIFT,
              m[2][3] >> FIXED_SHIFT) + pos;
    matrixPop();

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

        vec3i d = p - lpos;
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

#include "lara.h"
#include "enemy.h"
#include "object.h"

Item::Item(Room* room) 
{
    angleX      = 0;
    angleZ      = 0;
    vSpeed      = 0;
    hSpeed      = 0;
    nextItem    = NULL;
    nextActive  = NULL;
    animIndex   = models[modelsMap[type]].animIndex;
    frameIndex  = anims[animIndex].frameBegin;
    state       = uint8(anims[animIndex].state);
    nextState   = state;
    goalState   = state;

    flags.save = true;
    flags.gravity = false;
    flags.active = false;
    flags.status = ITEM_FLAGS_STATUS_NONE;
    flags.collision = true;
    flags.custom = 0;
    flags.shadow = false;

    if (flags.once) // once -> invisible
    {
        flags.status = ITEM_FLAGS_STATUS_INVISIBLE;
        flags.once = false;
    }

    if (flags.mask == ITEM_FLAGS_MASK_ALL) // full set of mask -> reverse
    {
        flags.mask = 0;
        flags.active = true;
        flags.reverse = true;
        activate();
    }

    ASSERT(type <= ITEM_MAX);

    ASSERT(room);

    room->add(this);
}

void Item::update()
{
    //
}

void Item::draw()
{
    drawItem(this);
}

void Item::collide(Lara* lara, CollisionInfo* cinfo)
{
    UNUSED(lara);
    UNUSED(cinfo);
}

Item* Item::init(Room* room)
{
    #define INIT_ITEM(type, className) case ITEM_##type : return new (this) className(room)

    switch (type)
    {
        INIT_ITEM( LARA                  , Lara );
        // INIT_ITEM( DOPPELGANGER          , ??? );
        INIT_ITEM( WOLF                  , Wolf );
        INIT_ITEM( BEAR                  , Bear );
        INIT_ITEM( BAT                   , Bat );
        // INIT_ITEM( CROCODILE_LAND        , ??? );
        // INIT_ITEM( CROCODILE_WATER       , ??? );
        // INIT_ITEM( LION_MALE             , ??? );
        // INIT_ITEM( LION_FEMALE           , ??? );
        // INIT_ITEM( PUMA                  , ??? );
        // INIT_ITEM( GORILLA               , ??? );
        // INIT_ITEM( RAT_LAND              , ??? );
        // INIT_ITEM( RAT_WATER             , ??? );
        // INIT_ITEM( REX                   , ??? );
        // INIT_ITEM( RAPTOR                , ??? );
        // INIT_ITEM( MUTANT_1              , ??? );
        // INIT_ITEM( MUTANT_2              , ??? );
        // INIT_ITEM( MUTANT_3              , ??? );
        // INIT_ITEM( CENTAUR               , ??? );
        // INIT_ITEM( MUMMY                 , ??? );
        // INIT_ITEM( LARSON                , ??? );
        // INIT_ITEM( PIERRE                , ??? );
        // INIT_ITEM( SKATEBOARD            , ??? );
        // INIT_ITEM( SKATER                , ??? );
        // INIT_ITEM( COWBOY                , ??? );
        // INIT_ITEM( MR_T                  , ??? );
        // INIT_ITEM( NATLA                 , ??? );
        // INIT_ITEM( GIANT_MUTANT          , ??? );
        INIT_ITEM( TRAP_FLOOR            , TrapFloor );
        // INIT_ITEM( TRAP_SWING_BLADE      , ??? );
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
        // INIT_ITEM( BLOCK_1               , ??? );
        // INIT_ITEM( BLOCK_2               , ??? );
        // INIT_ITEM( BLOCK_3               , ??? );
        // INIT_ITEM( BLOCK_4               , ??? );
        // INIT_ITEM( MOVING_BLOCK          , ??? );
        // INIT_ITEM( TRAP_CEILING_1        , ??? );
        // INIT_ITEM( TRAP_CEILING_2        , ??? );
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
        // INIT_ITEM( TRAP_DOOR_1           , ??? );
        // INIT_ITEM( TRAP_DOOR_2           , ??? );
        // INIT_ITEM( BRIDGE_FLAT           , ??? );
        // INIT_ITEM( BRIDGE_TILT1          , ??? );
        // INIT_ITEM( BRIDGE_TILT2          , ??? );
        // INIT_ITEM( INV_PASSPORT          , ??? );
        // INIT_ITEM( INV_COMPASS           , ??? );
        // INIT_ITEM( INV_HOME              , ??? );
        // INIT_ITEM( GEARS_1               , ??? );
        // INIT_ITEM( GEARS_2               , ??? );
        // INIT_ITEM( GEARS_3               , ??? );
        // INIT_ITEM( CUT_1                 , ??? );
        // INIT_ITEM( CUT_2                 , ??? );
        // INIT_ITEM( CUT_3                 , ??? );
        // INIT_ITEM( CUT_4                 , ??? );
        // INIT_ITEM( INV_PASSPORT_CLOSED   , ??? );
        // INIT_ITEM( INV_MAP               , ??? );
        // INIT_ITEM( CRYSTAL               , ??? );
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
        // INIT_ITEM( PUZZLE_HOLE_1         , ??? );
        // INIT_ITEM( PUZZLE_HOLE_2         , ??? );
        // INIT_ITEM( PUZZLE_HOLE_3         , ??? );
        // INIT_ITEM( PUZZLE_HOLE_4         , ??? );
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
        // INIT_ITEM( KEY_HOLE_1            , ??? );
        // INIT_ITEM( KEY_HOLE_2            , ??? );
        // INIT_ITEM( KEY_HOLE_3            , ??? );
        // INIT_ITEM( KEY_HOLE_4            , ??? );
        // INIT_ITEM( SCION_PICKUP_QUALOPEC , ??? );
        INIT_ITEM( SCION_PICKUP_DROP     , Pickup );
        INIT_ITEM( SCION_TARGET          , ViewTarget );
        // INIT_ITEM( SCION_PICKUP_HOLDER   , ??? );
        // INIT_ITEM( SCION_HOLDER          , ??? );
        // INIT_ITEM( INV_SCION             , ??? );
        // INIT_ITEM( EXPLOSION             , ??? );
        // INIT_ITEM( WATER_SPLASH          , ??? );
        // INIT_ITEM( BUBBLE                , ??? );
        // INIT_ITEM( BLOOD                 , ??? );
        // INIT_ITEM( SMOKE                 , ??? );
        // INIT_ITEM( CENTAUR_STATUE        , ??? );
        // INIT_ITEM( CABIN                 , ??? );
        // INIT_ITEM( MUTANT_EGG_SMALL      , ??? );
        // INIT_ITEM( RICOCHET              , ??? );
        // INIT_ITEM( SPARKLES              , ??? );
        // INIT_ITEM( MUZZLE_FLASH          , ??? );
        INIT_ITEM( VIEW_TARGET           , ViewTarget );
        // INIT_ITEM( WATERFALL             , ??? );
        // INIT_ITEM( NATLA_BULLET          , ??? );
        // INIT_ITEM( MUTANT_BULLET         , ??? );
        // INIT_ITEM( CENTAUR_BULLET        , ??? );
        // INIT_ITEM( LAVA_PARTICLE         , ??? );
        // INIT_ITEM( TRAP_LAVA_EMITTER     , ??? );
        // INIT_ITEM( FLAME                 , ??? );
        // INIT_ITEM( TRAP_FLAME_EMITTER    , ??? );
        // INIT_ITEM( TRAP_LAVA             , ??? );
        // INIT_ITEM( MUTANT_EGG_BIG        , ??? );
        // INIT_ITEM( BOAT                  , ??? );
        // INIT_ITEM( EARTHQUAKE            , ??? );
    }

    return new (this) Item(room);
}

#endif
