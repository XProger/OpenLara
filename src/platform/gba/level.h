#ifndef H_LEVEL
#define H_LEVEL

#include "common.h"

// level file data -------------------
//int32               tilesCount;
extern const uint8* tiles;

extern uint16    palette[256];
extern uint8     lightmap[256 * 32];

const FloorData* floors;

int32                 texturesCount;
extern const Texture* textures;

const Sprite*    sprites;

int32            spritesSeqCount;
const SpriteSeq* spritesSeq;

const uint8*     meshData;
const int32*     meshOffsets;

const int32*     nodesPtr;

//int32            animsCount;
const Anim*      anims;

//int32            animStatesCount;
const AnimState* animStates;

//int32            animRangesCount;
const AnimRange* animRanges;

//int32            animCommandsCount;
const int16*     animCommands;

//int32            animFramesCount;
const uint16*    animFrames;

int32            modelsCount;
EWRAM_DATA Model models[MAX_MODELS];
EWRAM_DATA uint8 modelsMap[MAX_ITEMS];
EWRAM_DATA uint8 staticMeshesMap[MAX_MESHES];

int32             staticMeshesCount;
const StaticMesh* staticMeshes;

int32           itemsCount;
EWRAM_DATA Item items[MAX_ITEMS];

const uint16*    soundMap;

//int32            soundInfosCount;
const SoundInfo* soundInfos;

//int32           soundDataSize;
const uint8*    soundData;

//int32           soundOffsetsCount;
const uint32*   soundOffsets;

#define MAX_DYN_SECTORS     1024
int32                       dynSectorsCount;
EWRAM_DATA RoomInfo::Sector dynSectors[MAX_DYN_SECTORS];   // EWRAM 8k
// -----------------------------------

int16           roomsCount;
EWRAM_DATA Room rooms[64];

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

void readLevel(const uint8* data) // TODO non-hardcode level loader, added *_OFF alignment bytes
{
    Item::sFirstActive = NULL;
    Item::sFirstFree = NULL;

    dynSectorsCount = 0;

//    tilesCount = *((int32*)(data + 4));
    tiles = data + 8;

    #define MDL_OFF 2
    #define ITM_OFF 2

    roomsCount = *((int16*)(data + 720908));
    const RoomInfo* roomsPtr = (RoomInfo*)(data + 720908 + 2);

    floors = (FloorData*)(data + 899492 + 4);

    meshData = data + 908172 + 4;
    meshOffsets = (int32*)(data + 975724 + 4);

//    animsCount = *((int32*)(data + 976596));
    anims = (Anim*)(data + 976596 + 4);
    ASSERT((intptr_t)anims % 4 == 0);

//    animStatesCount = *((int32*)(data + 985464));
    animStates = (AnimState*)(data + 985464 + 4);
    ASSERT((intptr_t)animStates % 2 == 0);

//    animRangesCount = *((int32*)(data + 986872));
    animRanges = (AnimRange*)(data + 986872 + 4);
    ASSERT((intptr_t)animRanges % 2 == 0);

//    animCommandsCount = *((int32*)(data + 988868));
    animCommands = (int16*)(data + 988868 + 4);
    ASSERT((intptr_t)animCommands % 2 == 0);

//    animFramesCount = *((int32*)(data + 992990));
    animFrames = (uint16*)(data + 992990 + 4);
    ASSERT((intptr_t)animFrames % 2 == 0);

    nodesPtr = (int32*)(data + 990318);

    modelsCount = *((int32*)(data + 1270666 + MDL_OFF));
    const uint8* modelsPtr = (uint8*)(data + 1270666 + 4 + MDL_OFF);
    ASSERT((intptr_t)modelsPtr % 4 == 0);

    staticMeshesCount = *((int32*)(data + 1271426 + MDL_OFF));
    staticMeshes = (StaticMesh*)(data + 1271426 + 4 + MDL_OFF);
    ASSERT((intptr_t)staticMeshes % 4 == 0);

    texturesCount = *((int32*)(data + 1271686 + MDL_OFF));
    textures = (Texture*)(data + 1271686 + 4 + MDL_OFF);

    sprites = (Sprite*)(data + 1289634 + MDL_OFF);

    spritesSeqCount = *((int32*)(data + 1292130 + MDL_OFF));
    spritesSeq = (SpriteSeq*)(data + 1292130 + 4 + MDL_OFF);

    itemsCount = *((int32*)(data + 1319252 + MDL_OFF + ITM_OFF));
    const uint8* itemsPtr = (data + 1319252 + 4 + MDL_OFF + ITM_OFF);

    soundMap = (uint16*)(data + 1329540 + MDL_OFF + ITM_OFF);

//    soundInfosCount = *((int32*)(data + 1330052 + MDL_OFF + ITM_OFF));
    soundInfos = (SoundInfo*)(data + 1330052 + 4 + MDL_OFF + ITM_OFF);

//    int32 soundDataSize = *((int32*)(data + 1330624 + MDL_OFF + ITM_OFF));
    soundData = (uint8*)(data + 1330624 + 4 + MDL_OFF + ITM_OFF);

//    soundOffsetsCount = *((int32*)(data + 2533294 + MDL_OFF + ITM_OFF));
    soundOffsets = (uint32*)(data + 2533294 + 4 + MDL_OFF + ITM_OFF);

    memset(items, 0, sizeof(items));
    for (int32 i = 0; i < itemsCount; i++) {
        memcpy(&items[i].type, itemsPtr, FILE_ITEM_SIZE);
        itemsPtr += FILE_ITEM_SIZE;
    }

// prepare free list
    for (int32 i = MAX_ITEMS - 1; i >= itemsCount; i--)
    {
        items[i].nextItem = items + i + 1;
    }
    Item::sFirstFree = items + itemsCount;

// prepare lightmap
    const uint8* f_lightmap = data + 1320576 + MDL_OFF + ITM_OFF;
    memcpy(lightmap, f_lightmap, sizeof(lightmap));
    
    // TODO preprocess
    for (int i = 0; i < 32; i++) {
        lightmap[i * 256] = 0;
    }

// prepare palette
    const uint8* f_palette = data + 1328768 + MDL_OFF + ITM_OFF;

    const uint8* p = f_palette;

#ifdef MODE_PAL
    uint16 palette[256];
#endif

    for (int i = 0; i < 256; i++) // TODO preprocess
    {
    #if defined(_WIN32) || defined(__GBA__) || defined(__DOS__)
        // grayscale palette
        //uint8 c = ((p[0] + p[1] + p[2]) / 3) >> 1;
        //palette[i] = c | (c << 5) | (c << 10);

        palette[i] = (p[0] >> 1) | ((p[1] >> 1) << 5) | ((p[2] >> 1) << 10);
    #elif defined(__TNS__)
        palette[i] = (p[2] >> 1) | ((p[1] >> 1) << 5) | ((p[0] >> 1) << 10);
    #endif
        p += 3;
    }
    palette[0] = 0; // black or transparent

    // TODO preprocess fix Laras palette
    fixLightmap(palette, 6);  // boots
    fixLightmap(palette, 14); // skin

#ifdef MODE_PAL
    paletteSet(palette);
#endif

// prepare rooms
    uint8* ptr = (uint8*)roomsPtr;

    for (int32 roomIndex = 0; roomIndex < roomsCount; roomIndex++)
    {
        const RoomInfo* room = (RoomInfo*)ptr;
        ptr += sizeof(RoomInfo);

        uint32 dataSize;
        memcpy(&dataSize, &room->dataSize, sizeof(dataSize));
        uint8* skipPtr = ptr + dataSize * 2;

        Room &desc = rooms[roomIndex];

        desc.firstItem = NULL;

        // offset
        memcpy(&desc.x, &room->x, sizeof(room->x));
        memcpy(&desc.z, &room->z, sizeof(room->z));

        // vertices
        desc.vCount = *((uint16*)ptr);
        ptr += 2;
        desc.vertices = (RoomInfo::Vertex*)ptr;
        ptr += sizeof(RoomInfo::Vertex) * desc.vCount;

        // quads
        desc.qCount = *((uint16*)ptr);
        ptr += 2;
        desc.quads = (Quad*)ptr;
        ptr += sizeof(Quad) * desc.qCount;

        // triangles
        desc.tCount = *((uint16*)ptr);
        ptr += 2;
        desc.triangles = (Triangle*)ptr;
        ptr += sizeof(Triangle) * desc.tCount;

        ptr = skipPtr;

        // portals
        desc.pCount = *((uint16*)ptr);
        ptr += 2;
        desc.portals = (RoomInfo::Portal*)ptr;
        ptr += sizeof(RoomInfo::Portal) * desc.pCount;

        desc.zSectors = *((uint16*)ptr);
        ptr += 2;
        desc.xSectors = *((uint16*)ptr);
        ptr += 2;
        desc.sectors = (RoomInfo::Sector*)ptr;
        desc.sectorsOrig = desc.sectors;
        ptr += sizeof(RoomInfo::Sector) * desc.zSectors * desc.xSectors;

        desc.ambient = *((uint16*)ptr);
        ptr += 2;

        desc.lCount = *((uint16*)ptr);
        ptr += 2;
        desc.lights = (RoomInfo::Light*)ptr;
        ptr += sizeof(RoomInfo::Light) * desc.lCount;

        desc.mCount = *((uint16*)ptr);
        ptr += 2;
        desc.meshes = (RoomInfo::Mesh*)ptr;
        ptr += sizeof(RoomInfo::Mesh) * desc.mCount;

        ptr += 2 + 2; // skip alternateRoom and flags
    }

// prepare models
    memset(modelsMap, 0xFF, sizeof(modelsMap));
    for (int32 i = 0; i < modelsCount; i++)
    {
        memcpy(models + i, modelsPtr, sizeof(Model)); // sizeof(Model) is faster than FILE_MODEL_SIZE
        modelsPtr += FILE_MODEL_SIZE;
        modelsMap[models[i].type] = i;
    }

// prepare static meshes
    memset(staticMeshesMap, 0xFF, sizeof(staticMeshesMap));
    for (int32 i = 0; i < staticMeshesCount; i++)
    {
        staticMeshesMap[staticMeshes[i].id] = i;
    }
}

#endif
