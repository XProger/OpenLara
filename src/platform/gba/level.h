#ifndef H_LEVEL
#define H_LEVEL

#include "common.h"

Level level;

const Texture* textures;
const uint8* tiles;

#ifndef MODE_PAL
extern uint16 palette[256];
#endif

IWRAM_DATA uint8 lightmap[256 * 32]; // IWRAM 8k

EWRAM_DATA Item items[MAX_ITEMS];

#define MAX_DYN_SECTORS     1024
int32                       dynSectorsCount;
EWRAM_DATA Sector dynSectors[MAX_DYN_SECTORS];   // EWRAM 8k

EWRAM_DATA Room rooms[MAX_ROOMS];
EWRAM_DATA Model models[MAX_MODELS];
EWRAM_DATA StaticMesh staticMeshes[MAX_STATIC_MESHES];

Item* Item::sFirstActive;
Item* Item::sFirstFree;

Room* roomsList[MAX_ROOM_LIST];

void fixLightmap(uint16* palette, int32 palIndex)
{
    uint16 color = palette[palIndex];

    int32 r = 0x1F & (color);
    int32 g = 0x1F & (color >> 5);
    int32 b = 0x1F & (color >> 10);

    for (int32 i = 0; i < 32; i++)
    {
        int32 lum = 31 - i;

        int32 lumR = X_CLAMP((r * lum) / 14, 0, 31);
        int32 lumG = X_CLAMP((g * lum) / 14, 0, 31);
        int32 lumB = X_CLAMP((b * lum) / 14, 0, 31);

        palette[lightmap[i * 256 + palIndex]] = lumR | (lumG << 5) | (lumB << 10);
    }
}

void readLevel(const uint8* data)
{
    Item::sFirstActive = NULL;
    Item::sFirstFree = NULL;

    dynSectorsCount = 0;

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

    // initialize global pointers
#ifdef MODE_PAL
    paletteSet(level.palette);
#endif

    memcpy(lightmap, level.lightmap, sizeof(lightmap));

    tiles = level.tiles;

    textures = level.textures;

    // prepare models // TODO prerocess
    memset(models, 0, sizeof(models));
    for (int32 i = 0; i < level.modelsCount; i++)
    {
        const Model* model = level.models + i;
        models[model->type] = *model;
    }

    //  prepare static meshes // TODO preprocess
    memset(staticMeshes, 0, sizeof(staticMeshes));
    for (int32 i = 0; i < level.staticMeshesCount; i++)
    {
        const StaticMesh* staticMesh = level.staticMeshes + i;

        ASSERT(staticMesh->id < MAX_STATIC_MESHES);
        staticMeshes[staticMesh->id] = *staticMesh;
    }

    // prepare sprites // TODO preprocess
    for (int32 i = 0; i < level.spriteSequencesCount; i++)
    {
        const SpriteSeq* spriteSeq = level.spriteSequences + i;

        if (spriteSeq->type >= TR1_ITEM_MAX) // WTF?
            continue;

        Model *m = models + spriteSeq->type;
        m->count = spriteSeq->count;
        m->start = spriteSeq->start;
    }

    // prepare free list
    for (int32 i = MAX_ITEMS - 1; i >= level.itemsCount; i--)
    {
        items[i].nextItem = items + i + 1;
    }
    Item::sFirstFree = items + level.itemsCount;
}

#endif
