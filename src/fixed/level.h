#ifndef H_LEVEL
#define H_LEVEL

#include "common.h"
#include "camera.h"

Level level;

#ifdef __32X__
    extern uint8 gLightmap[256 * 32]; // SDRAM 8k at 0x6000000
#else
    #ifndef MODEHW
        IWRAM_DATA uint8 gLightmap[256 * 32]; // IWRAM 8k
    #endif
#endif

EWRAM_DATA ItemObj items[MAX_ITEMS];

#ifdef ROM_READ
EWRAM_DATA Texture textures[MAX_TEXTURES]; // animated textures require memory swap
EWRAM_DATA Sprite sprites[MAX_SPRITES];
EWRAM_DATA FixedCamera cameras[MAX_CAMERAS];
EWRAM_DATA Box boxes[MAX_BOXES];
#endif

EWRAM_DATA Room rooms[MAX_ROOMS];
EWRAM_DATA Model models[MAX_MODELS];
EWRAM_DATA const Mesh* meshes[MAX_MESHES];
EWRAM_DATA StaticMesh staticMeshes[MAX_STATIC_MESHES];

EWRAM_DATA ItemObj* ItemObj::sFirstActive;
EWRAM_DATA ItemObj* ItemObj::sFirstFree;

EWRAM_DATA int32 gBrightness;

void readLevel_GBA(const uint8* data)
{
    memcpy(&level, data, sizeof(level));

    { // fix level data offsets
        uint32* ptr = (uint32*)&level.palette;
        while (ptr <= (uint32*)&level.soundOffsets)
        {
            *ptr++ += (uint32)data;
        }
    }

    { // prepare rooms
        for (int32 i = 0; i < level.roomsCount; i++)
        {
            Room* room = rooms + i;
            room->info = level.roomsInfo + i;
            room->data = room->info->data;

            for (uint32 j = 0; j < sizeof(room->data) / 4; j++)
            {
                int32* x = (int32*)&room->data + j;
                *x += (int32)data;
            }

            room->sectors = room->data.sectors;
            room->firstItem = NULL;
        }
    }

#ifndef MODEHW
    // initialize global pointers
    gBrightness = -128;
    palSet(level.palette, gSettings.video_gamma << 4, gBrightness);
    memcpy(gLightmap, level.lightmap, sizeof(gLightmap));
#endif

    // prepare models // TODO prerocess
    memset(models, 0, sizeof(models));
    for (int32 i = 0; i < level.modelsCount; i++)
    {
        const Model* model = level.models + i;
        ASSERT(model->type < MAX_MODELS);
        models[model->type] = *model;
    }
    level.models = models;

    // prepare meshes
    for (int32 i = 0; i < level.meshesCount; i++)
    {
        meshes[i] = (Mesh*)((uint8*)level.meshes + level.meshOffsets[i]);
    }
    level.meshes = meshes;

    //  prepare static meshes // TODO preprocess
    memset(staticMeshes, 0, sizeof(staticMeshes));
    for (int32 i = 0; i < level.staticMeshesCount; i++)
    {
        const StaticMesh* staticMesh = level.staticMeshes + i;

        ASSERT(staticMesh->id < MAX_STATIC_MESHES);
        staticMeshes[staticMesh->id] = *staticMesh;
    }
    level.staticMeshes = staticMeshes;

    // prepare sprites // TODO preprocess
    for (int32 i = 0; i < level.spriteSequencesCount; i++)
    {
        const SpriteSeq* spriteSeq = level.spriteSequences + i;

        if (spriteSeq->type >= TR1_ITEM_MAX) // WTF?
            continue;

        Model* m = models + spriteSeq->type;
        m->count = int8(spriteSeq->count);
        m->start = spriteSeq->start;
    }

#ifdef ROM_READ
    // prepare textures (required by anim tex logic)
    memcpy(textures, level.textures, level.texturesCount * sizeof(Texture));
    level.textures = textures;

    // prepare sprites (TODO preprocess tile address in packer)
    memcpy(sprites, level.sprites, level.spritesCount * sizeof(Sprite));
    level.sprites = sprites;

    // prepare boxes
    memcpy(boxes, level.boxes, level.boxesCount * sizeof(Box));
    level.boxes = boxes;

    // prepare fixed cameras
    memcpy(cameras, level.cameras, level.camerasCount * sizeof(FixedCamera));
    level.cameras = cameras;
#endif

#ifdef __3DO__
    for (int32 i = 0; i < level.texturesCount; i++)
    {
        Texture* tex = level.textures + i;
        tex->data += intptr_t(RAM_TEX);
    }
#else
    // TODO preprocess in packer
    for (int32 i = 0; i < level.texturesCount; i++)
    {
        level.textures[i].tile += (uint32)level.tiles;
    }

    for (int32 i = 0; i < level.spritesCount; i++)
    {
        level.sprites[i].tile += (uint32)level.tiles;
    }
#endif
}

void readLevel(const uint8* data)
{
//#ifdef ROM_READ
    dynSectorsCount = 0;
//#endif

    readLevel_GBA(data);

    gAnimTexFrame = 0;
}

void animTexturesShift()
{
    const int16* data = level.animTexData;

    int16 texRangesCount = *data++;

    for (int32 i = 0; i < texRangesCount; i++)
    {
        int16 count = *data++;

        Texture tmp = level.textures[*data];
        while (count > 0)
        {
            level.textures[data[0]] = level.textures[data[1]];
            data++;
            count--;
        }
        level.textures[*data++] = tmp;
    }
}

#define FADING_RATE_SHIFT 4

void updateFading(int32 frames)
{
    if (gBrightness == 0)
        return;

    frames <<= FADING_RATE_SHIFT;

    if (gBrightness < 0)
    {
        gBrightness += frames;
        if (gBrightness > 0) {
            gBrightness = 0;
        }
    }

    if (gBrightness > 0)
    {
        gBrightness -= frames;
        if (gBrightness < 0) {
            gBrightness = 0;
        }
    }

    palSet(level.palette, gSettings.video_gamma << 4, gBrightness);
}

void updateLevel(int32 frames)
{
    updateFading(frames);

    gCausticsFrame += frames;

    gAnimTexFrame += frames;
    while (gAnimTexFrame > 5)
    {
        animTexturesShift();
        gAnimTexFrame -= 5;
    }
}

int32 getAmbientTrack()
{
    return gLevelInfo[gLevelID].track;
}

bool isCutsceneLevel()
{
    return (gLevelID == LVL_TR1_CUT_1) ||
           (gLevelID == LVL_TR1_CUT_2) ||
           (gLevelID == LVL_TR1_CUT_3) ||
           (gLevelID == LVL_TR1_CUT_4);
}

#endif
