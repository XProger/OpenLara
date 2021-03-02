#ifndef H_LEVEL
#define H_LEVEL

#include "common.h"

// level file data -------------------
//int32               tilesCount;
extern const uint8* tiles;

extern uint16    palette[256];
extern uint8     lightmap[256 * 32];
const uint16*    floors;

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
// -----------------------------------

int16           roomsCount;
EWRAM_DATA Room rooms[64];

int32 firstActive = NO_ITEM;

int32 visRoomsCount;
int32 visRooms[16];

#define ROOM_VISIBLE (1 << 15)

void roomItemAdd(int32 roomIndex, int32 itemIndex)
{
    Item* item = items + itemIndex;
    Room* room = rooms + roomIndex;

    ASSERT(item->nextItem == NO_ITEM);

    item->room = roomIndex;
    item->nextItem = room->firstItem;
    room->firstItem = itemIndex;
}

void roomItemRemove(int32 itemIndex)
{
    Item* item = items + itemIndex;
    Room* room = rooms + item->room;
    item->room = NO_ROOM;

    int32 prevIndex = NO_ITEM;
    int32 index = room->firstItem;

    while (index != NO_ITEM)
    {
        int32 next = items[index].nextItem;

        if (index == itemIndex)
        {
            items[index].nextItem = NO_ITEM;

            if (prevIndex == NO_ITEM) {
                room->firstItem = next;
            } else {
                items[prevIndex].nextItem = next;
            }

            break;
        }

        prevIndex = index;
        index = next;
    }
}

void activateItem(int32 itemIndex)
{
    items[itemIndex].nextActive = firstActive;
    firstActive = itemIndex;
}

void deactivateItem(int32 itemIndex)
{
    int32 prevIndex = NO_ITEM;
    int32 index = firstActive;

    while (index != NO_ITEM)
    {
        int32 next = items[index].nextActive;

        if (index == itemIndex)
        {
            items[index].nextItem = NO_ITEM;

            if (prevIndex == NO_ITEM) {
                firstActive = next;
            } else {
                items[prevIndex].nextActive = next;
            }

            break;
        }

        prevIndex = index;
        index = next;
    }
}

void fixLightmap(int32 palIndex)
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

void readLevel(const uint8* data) { // TODO non-hardcode level loader, added *_OFF alignment bytes
//    tilesCount = *((int32*)(data + 4));
    tiles = data + 8;

    #define MDL_OFF 2
    #define ITM_OFF 2

    roomsCount = *((int16*)(data + 720908));
    const RoomInfo* roomsPtr = (RoomInfo*)(data + 720908 + 2);

    floors = (uint16*)(data + 899492 + 4);

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

//    soundDataSize = *((int32*)(data + 1330624 + MDL_OFF + ITM_OFF));
    soundData = (uint8*)(data + 1330624 + 4 + MDL_OFF + ITM_OFF);

//    soundOffsetsCount = *((int32*)(data + 2533294 + MDL_OFF + ITM_OFF));
    soundOffsets = (uint32*)(data + 2533294 + 4 + MDL_OFF + ITM_OFF);

    memset(items, 0, sizeof(items));
    for (int32 i = 0; i < itemsCount; i++) {
        memcpy(items + i, itemsPtr, FILE_ITEM_SIZE);
        itemsPtr += FILE_ITEM_SIZE;
    }

// prepare lightmap
    const uint8* f_lightmap = data + 1320576 + MDL_OFF + ITM_OFF;
    memcpy(lightmap, f_lightmap, sizeof(lightmap));

// prepare palette
    const uint8* f_palette = data + 1328768 + MDL_OFF + ITM_OFF;

    const uint8* p = f_palette;

    for (int i = 0; i < 256; i++)
    {
    #if defined(_WIN32) || defined(__GBA__)
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
    fixLightmap(6);  // boots
    fixLightmap(14); // skin

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

        desc.firstItem = NO_ITEM;

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

int32 getBridgeFloor(const Item* item, int32 x, int32 z)
{
    if (item->type == ITEM_BRIDGE_1) {
        return item->pos.y;
    }

    int32 h;
    if (item->angleY == ANGLE_0) {
        h = 1024 - x;
    } else if (item->angleY == -ANGLE_180) {
        h = x;
    } else if (item->angleY == ANGLE_90) {
        h = z;
    } else {
        h = 1024 - z;
    }

    h &= 1023;

    return item->pos.y + ((item->type == ITEM_BRIDGE_2) ? (h >> 2) : (h >> 1));
}

int32 getTrapDoorFloor(const Item* item, int32 x, int32 z)
{
    int32 dx = (item->pos.x >> 10) - (x >> 10);
    int32 dz = (item->pos.z >> 10) - (z >> 10);

    if (((dx ==  0) && (dz ==  0)) ||
        ((dx ==  0) && (dz ==  1) && (item->angleY ==  ANGLE_0))   ||
        ((dx ==  0) && (dz == -1) && (item->angleY == -ANGLE_180)) ||
        ((dx ==  1) && (dz ==  0) && (item->angleY ==  ANGLE_90))  ||
        ((dx == -1) && (dz ==  0) && (item->angleY == -ANGLE_90)))
    {
        return item->pos.y;
    }

    return WALL;
}

int32 getDrawBridgeFloor(const Item* item, int32 x, int32 z)
{
    int32 dx = (item->pos.x >> 10) - (x >> 10);
    int32 dz = (item->pos.z >> 10) - (z >> 10);

    if (((dx == 0) && ((dz == -1) || (dz == -2)) && (item->angleY ==  ANGLE_0))   ||
        ((dx == 0) && ((dz ==  1) || (dz ==  2)) && (item->angleY == -ANGLE_180)) ||
        ((dz == 0) && ((dx == -1) || (dz == -2)) && (item->angleY ==  ANGLE_90))  ||
        ((dz == 0) && ((dx ==  1) || (dz ==  2)) && (item->angleY == -ANGLE_90)))
    {
        return item->pos.y;
    }

    return WALL;
}

void getItemFloorData(const Item* item, int32 x, int32 y, int32 z, int32* floor, int32* ceiling)
{
    int32 h = WALL;

    switch (item->type)
    {
        case ITEM_TRAP_FLOOR:
        {
            if (item->state == 0 || item->state == 1) {
                h = item->pos.y - 512;
            }
            break;
        }
        case ITEM_DRAWBRIDGE:
        {
            if (item->state == 1) {
                h = getDrawBridgeFloor(item, x, z);
            }
            break;
        }
        case ITEM_BRIDGE_1:
        case ITEM_BRIDGE_2:
        case ITEM_BRIDGE_3:
        {
            h = getBridgeFloor(item, x, z);
            break;
        }
        case ITEM_TRAP_DOOR_1:
        case ITEM_TRAP_DOOR_2:
        {
            if (item->state == 0) {
                h = getTrapDoorFloor(item, x, z);
            }

            if ((floor && (h >= *floor)) || (ceiling && (h <= *ceiling)))
            {
                h = WALL;
            }
        }
        default : return;
    }

    if (floor && (y <= h))
    {
        *floor = h;
    }

    if (ceiling && (y > h))
    {
        *ceiling = h + 256;
    }
}

const RoomInfo::Sector* getSector(int32 roomIndex, int32 x, int32 z)
{
    Room &room = rooms[roomIndex];

    int32 sx = X_CLAMP((x - room.x) >> 10, 0, room.xSectors - 1);
    int32 sz = X_CLAMP((z - room.z) >> 10, 0, room.zSectors - 1);

    return room.sectors + sx * room.zSectors + sz;
}

const RoomInfo::Sector* getSectorBelow(const RoomInfo::Sector* sector, int32 x, int32 z)
{
    while (sector->roomBelow != NO_ROOM)
    {
        Room* room = rooms + sector->roomBelow;
        int32 sx = (x - room->x) >> 10;
        int32 sz = (z - room->z) >> 10;
        sector = room->sectors + sx * room->zSectors + sz;
    }
    return sector;
}

const RoomInfo::Sector* getSectorAbove(const RoomInfo::Sector* sector, int32 x, int32 z)
{
    while (sector->roomAbove != NO_ROOM)
    {
        Room* room = rooms + sector->roomAbove;
        int32 sx = (x - room->x) >> 10;
        int32 sz = (z - room->z) >> 10;
        sector = room->sectors + sx * room->zSectors + sz;
    }
    return sector;
}

int32 getRoomIndex(int32 roomIndex, int32 x, int32 y, int32 z)
{
    const RoomInfo::Sector* sector = getSector(roomIndex, x, z);

    while (1)
    {
        if (!sector->floorIndex)
            break;

        // always in this order
        // - floor
        // - ceiling
        // - portal
        // - other

        FloorData* fd = (FloorData*)(floors + sector->floorIndex);
        FloorData::Command cmd = (fd++)->cmd;

        if (cmd.func == FLOOR_TYPE_FLOOR)  // skip floor
        {
            if (cmd.end) break;
            fd++;
            cmd = (fd++)->cmd;
        }

        if (cmd.func == FLOOR_TYPE_CEILING) // skip ceiling
        {
            if (cmd.end) break;
            fd++;
            cmd = (fd++)->cmd;
        }

        if (cmd.func != FLOOR_TYPE_PORTAL) // no portal
            break;

        roomIndex = fd->value;
        sector = getSector(roomIndex, x, z);
    };

    while (sector->roomAbove != NO_ROOM && y < (sector->ceiling << 8))
    {
        roomIndex = sector->roomAbove;
        sector = getSector(roomIndex, x, z);
    }

    while (sector->roomBelow != NO_ROOM && y >= (sector->floor << 8))
    {
        roomIndex = sector->roomBelow;
        sector = getSector(roomIndex, x, z);
    }

    return roomIndex;
}

void getTriggerFloorData(const RoomInfo::Sector* sector, int32 x, int32 y, int32 z, int32* floor, int32* ceiling)
{
    if (!sector->floorIndex)
        return;

    FloorData::Command cmd;
    FloorData* fd = (FloorData*)(floors + sector->floorIndex);

    do {
        cmd = (fd++)->cmd;
            
        switch (cmd.func)
        {
            case FLOOR_TYPE_PORTAL:
            case FLOOR_TYPE_FLOOR:
            case FLOOR_TYPE_CEILING:
            {
                fd++;
                break;
            }
            
            case FLOOR_TYPE_TRIGGER:
            {
                fd++;
                FloorData::TriggerCommand trigger;

                do {
                    trigger = (fd++)->triggerCmd;

                    if (trigger.action == TRIGGER_ACTION_ACTIVATE)
                    {
                        getItemFloorData(items + trigger.args, x, y, z, floor, ceiling);
                    }

                    if (trigger.action == TRIGGER_ACTION_CAMERA_SWITCH)
                    {
                        trigger = (fd++)->triggerCmd; // skip camera index
                    }

                } while (!trigger.end);

                break;
            }

            case FLOOR_TYPE_LAVA:
                break;
        }

    } while (!cmd.end);
}

FloorData floorSlant;

int32 getFloor(const RoomInfo::Sector* sector, int32 x, int32 y, int32 z)
{
    const RoomInfo::Sector* lowerSector = getSectorBelow(sector, x, z);

    int32 floor = lowerSector->floor << 8;

    floorSlant.value = 0;

    if (lowerSector->floorIndex)
    {
        FloorData* fd = (FloorData*)(floors + lowerSector->floorIndex);
        FloorData::Command cmd = (fd++)->cmd;

        if (cmd.func == FLOOR_TYPE_FLOOR) // found floor
        {
            floorSlant = *fd;
            int32 sx = fd->slantX;
            int32 sz = fd->slantZ;
            int32 dx = x & 1023;
            int32 dz = z & 1023;
            floor -= sx * (sx < 0 ? dx : (dx - 1023)) >> 2;
            floor -= sz * (sz < 0 ? dz : (dz - 1023)) >> 2;
        }
    }

    getTriggerFloorData(lowerSector, x, y, z, &floor, NULL);

    return floor;
}

int32 getCeiling(const RoomInfo::Sector* sector, int32 x, int32 y, int32 z)
{
    const RoomInfo::Sector* upperSector = getSectorAbove(sector, x, z);

    int32 ceiling = upperSector->ceiling << 8;

    if (upperSector->floorIndex)
    {
        FloorData* fd = (FloorData*)(floors + upperSector->floorIndex);
        FloorData::Command cmd = (fd++)->cmd;

        if (cmd.func == FLOOR_TYPE_FLOOR) // skip floor
        {
            fd++;
            cmd = (fd++)->cmd;
        }

        if (cmd.func == FLOOR_TYPE_CEILING) // found ceiling
        {
            int32 sx = fd->slantX;
            int32 sz = fd->slantZ;
            int32 dx = x & 1023;
            int32 dz = z & 1023;
            ceiling -= sx * (sx < 0 ? (dx - 1023) : dx) >> 2;
            ceiling += sz * (sz < 0 ? dz : (dz - 1023)) >> 2;
        }
    }

    const RoomInfo::Sector* lowerSector = getSectorBelow(sector, x, z);

    getTriggerFloorData(lowerSector, x, y, z, NULL, &ceiling);

    return ceiling;
}


#endif
