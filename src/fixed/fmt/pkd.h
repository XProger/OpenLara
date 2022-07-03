#ifndef H_PKD
#define H_PKD

#include "common.h"
#include "stream.h"

bool read_PKD(DataStream &f)
{
    const uint8* data = f.getPtr();

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

    return true;
}

#endif
