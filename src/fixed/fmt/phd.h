#ifndef H_PHD
#define H_PHD

#include "common.h"

struct PHD
{
    struct RoomVertex
    {
        int16 x, y, z;
        uint16 lighting;
    };

    struct RoomQuad
    {
        uint16 indices[4];
        uint16 flags;
    };

    struct RoomTriangle
    {
        uint16 indices[3];
        uint16 flags;
    };

    struct RoomSprite
    {
        uint16 index;
        uint16 texture;
    };

    struct RoomPortal
    {
        int16 roomIndex;
        vec3s normal;
        vec3s vertices[4];
    };
};

RoomInfo roomsInfo_phd[MAX_ROOMS];

bool read_PHD(const uint8* data)
{
/* TODO
    Stream stream(data, 0);

    uint32 magic = stream.read32u();
    if (magic != 0x00000020)
    {
        LOG("Unsupported level format\n");
        return;
    }

    level.version = VER_TR1_PC;
    
    level.tilesCount = stream.read32u();
    level.tiles = (uint8*)stream.getPtr();
    stream.pos += 4;

    level.roomsCount = stream.read16u();
    level.roomsInfo = phd_roomsInfo;

    for (uint32 i = 0; i < level.roomsCount; i++)
    {
        RoomInfo* info = phd_roomsInfo + i;

        info->x = stream.read32s();
        info->z = stream.read32s();
        info->yBottom = stream.read32s();
        info->yTop = stream.read32s();

        info->verticesCount = stream.read16u();
        info->data.vertices = (RoomVertex*)stream.getPtr();
        stream.pos += sizeof(PHD::RoomVertex) * info->verticesCount;

        info->quadsCount = stream.read16u();
        info->data.quads = (RoomQuad*)stream.getPtr();
        stream.pos += sizeof(PHD::RoomQuad) * info->quadsCount;

        info->trianglesCount = stream.read16u();
        info->data.triangles = (RoomTriangle*)stream.getPtr();
        stream.pos += sizeof(PHD::RoomTriangle) * info->trianglesCount;

        info->spritesCount = stream.read16u();
        info->data.sprites = (RoomSprite*)stream.getPtr();
        stream.pos += sizeof(PHD::RoomSprite) * info->spritesCount;

        info->portalsCount = stream.read16u();
        info->data.portals = (RoomPortal*)stream.getPtr();
        stream.pos += sizeof(PHD::RoomPortal) * info->portalsCount;
    }
*/
    return false;
}
#endif
