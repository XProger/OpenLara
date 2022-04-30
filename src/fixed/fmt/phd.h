#ifndef H_PHD
#define H_PHD

#include "common.h"
#include "stream.h"

bool read_PHD(Stream &f)
{
    uint8* ptr = gLevelData;

    uint32 magic = f.read32u();
    if (magic != 0x00000020)
        return false;

    level.version = VER_TR1_PC;
    
    level.tilesCount = f.read32u();
    level.tiles = (uint8*)f.getPtr();

    f.seek(level.tilesCount * 256 * 256 + 4);

    level.roomsCount = f.read16u();
    level.roomsInfo = (RoomInfo*)ptr;
    ptr += level.roomsCount * sizeof(RoomInfo);

    for (uint32 i = 0; i < level.roomsCount; i++)
    {
        RoomInfo* info = (RoomInfo*)level.roomsInfo + i;

        info->x = f.read32s() >> 8;
        info->z = f.read32s() >> 8;
        f.seek(4 + 4 + 4);

        info->verticesCount = f.read16u();
        info->data.vertices = (RoomVertex*)ptr;

        int32 vertDataPos = f.getPos();

        int32 yb = -32768;
        int32 yt = 32767;

        f.seek(2); // skip x
        for (uint32 j = 0; j < info->verticesCount; j++)
        {
            int32 y = f.read16s(); // read y

            if (y < yt) {
                yt = y;
            }
            if (y > yb) {
                yb = y;
            }

            f.seek(2 + 2 + 2); // skip z, g, x
        }

        info->yBottom = yb;
        info->yTop = yt;

        f.setPos(vertDataPos);

        for (uint32 j = 0; j < info->verticesCount; j++)
        {
            RoomVertex *v = (RoomVertex*)ADDR_ALIGN4(ptr);
            ptr += sizeof(RoomVertex);

            v->x = f.read16s() >> 8;
            v->y = (f.read16s() - yt) >> 8;
            v->z = f.read16s() >> 8;
            v->g = f.read16u() >> 3;
        }

        info->quadsCount = f.read16u();
        info->data.quads = (RoomQuad*)ADDR_ALIGN4(ptr);
        for (uint32 j = 0; j < info->quadsCount; j++)
        {
            RoomQuad *q = (RoomQuad*)ptr;
            ptr += sizeof(RoomQuad);

            q->indices[0] = f.read16u();
            q->indices[1] = f.read16u();
            q->indices[2] = f.read16u();
            q->indices[3] = f.read16u();
            q->flags = f.read16u();
        }

        info->trianglesCount = f.read16u();
        info->data.triangles = (RoomTriangle*)ADDR_ALIGN4(ptr);
        for (uint32 j = 0; j < info->trianglesCount; j++)
        {
            RoomTriangle *t = (RoomTriangle*)ptr;
            ptr += sizeof(RoomTriangle);

            t->indices[0] = f.read16u();
            t->indices[1] = f.read16u();
            t->indices[2] = f.read16u();
            t->flags = f.read16u();
        }

        info->spritesCount = f.read16u();
        info->data.sprites = (RoomSprite*)ADDR_ALIGN4(ptr);
        for (uint32 j = 0; j < info->spritesCount; j++)
        {
            RoomSprite *s = (RoomSprite*)ptr;
            ptr += sizeof(RoomSprite);

            int32 idx = f.read16u();
            s->index = f.read16u() * 0xFF;

            int32 pos = f.getPos();
            f.setPos(vertDataPos + 8 * idx);

            s->pos.x = f.read16u();
            s->pos.y = f.read16u();
            s->pos.z = f.read16u();
            s->g = f.read16u() >> 3;

            f.setPos(pos);
        }

        info->portalsCount = uint8(f.read16u());
        info->data.portals = (Portal*)ADDR_ALIGN4(ptr);
        for (uint32 j = 0; j < info->portalsCount; j++)
        {
            Portal *p = (Portal*)ptr;
            ptr += sizeof(Portal);

            p->roomIndex = f.read16s();
            p->n.x = f.read16s();
            p->n.y = f.read16s();
            p->n.z = f.read16s();
            for (int32 k = 0; k < 4; k++)
            {
                p->v[k].x = f.read16s();
                p->v[k].y = f.read16s();
                p->v[k].z = f.read16s();
            }
        }

        info->zSectors = uint8(f.read16u());
        info->xSectors = uint8(f.read16u());
        info->data.sectors = (Sector*)ADDR_ALIGN4(ptr);
        for (uint32 j = 0; j < uint32(info->zSectors) * uint32(info->xSectors); j++)
        {
            Sector *s = (Sector*)ptr;
            ptr += sizeof(Sector);

            s->floorIndex = f.read16u();
            s->boxIndex   = f.read16u();
            s->roomBelow  = f.read8u();
            s->floor      = f.read8s();
            s->roomAbove  = f.read8u();
            s->ceiling    = f.read8s();
        }

        info->ambient = f.read16u() >> 5; // TODO 3?

        info->lightsCount = uint8(f.read16u());
        info->data.lights = (Light*)ADDR_ALIGN4(ptr);
        for (uint32 j = 0; j < info->lightsCount; j++)
        {
            Light *l = (Light*)ptr;
            ptr += sizeof(Light);

            l->pos.x = f.read32s() - (info->x << 8);
            l->pos.y = f.read32s();
            l->pos.z = f.read32s() - (info->z << 8);
            l->intensity = f.read16s() >> 5; // TODO 3?
            l->radius = f.read32s() >> 8;
        }

        info->meshesCount = uint8(f.read16u());
        info->data.meshes = (RoomMesh*)ADDR_ALIGN4(ptr);
        for (uint32 j = 0; j < info->meshesCount; j++)
        {
            RoomMesh *m = (RoomMesh*)ptr;
            ptr += sizeof(RoomMesh);

            vec3s pos;
            uint8 intensity;
            uint8 flags;

            pos.x = f.read32s() - (info->x << 8);
            pos.y = f.read32s();
            pos.z = f.read32s() - (info->z << 8);
            flags = ((f.read16s() / 0x4000 + 2) << 6);
            intensity = f.read16u() >> 5; // TODO 3?
            flags |= f.read16u();

            m->xy = (pos.x << 16) | uint16(pos.y);
            m->zf = (pos.z << 16) | (intensity << 8) | flags;
        }

        info->alternateRoom = uint8(f.read16s());
        uint16 flags = f.read16u();

        info->flags = 0;
        if (flags & 1) info->flags |= 1; // TODO 1?
        if (flags & 256) info->flags |= 2;  // TODO 2?
    }

    { // floors data
        uint32 floorsCount = f.read32u();
        level.floors = (FloorData*)ADDR_ALIGN4(ptr);
        FloorData *fd = (FloorData*)ptr;
        for (uint32 i = 0; i < floorsCount; i++)
        {
            fd[i] = f.read16u();
        }
        ptr += sizeof(FloorData) * floorsCount;
    }

    { // mesh data
        uint32 meshDataSize = f.read32u();
        level.meshes = (const Mesh**)ADDR_ALIGN4(ptr);

        int32 meshDataPos = f.getPos();
        f.seek(meshDataSize * sizeof(uint16));

        level.meshesCount = f.read32u();
        f.seek(level.meshesCount * sizeof(uint32));

        // TODO
        ptr += meshDataSize * sizeof(uint32);
    }

    { // anims
        uint32 animsCount = f.read32u();
        level.anims = (Anim*)ADDR_ALIGN4(ptr);
        for (uint32 i = 0; i < animsCount; i++)
        {
            Anim* anim = (Anim*)ptr;
            ptr += sizeof(Anim);

            anim->frameOffset    = f.read32u();
            anim->frameRate      = f.read8u();
            anim->frameSize      = f.read8u();
            anim->state          = f.read16u();
            anim->speed          = f.read32u();
            anim->accel          = f.read32u();
            anim->frameBegin     = f.read16u();
            anim->frameEnd       = f.read16u();
            anim->nextAnimIndex  = f.read16u();
            anim->nextFrameIndex = f.read16u();
            anim->statesCount    = f.read16u();
            anim->statesStart    = f.read16u();
            anim->commandsCount  = f.read16u();
            anim->commandsStart  = f.read16u();
        }
    }

    { // states
        uint32 animStatesCount = f.read32u();
        level.animStates = (AnimState*)ADDR_ALIGN4(ptr);
        for (uint32 i = 0; i < animStatesCount; i++)
        {
            AnimState* animState = (AnimState*)ptr;
            ptr += sizeof(AnimState);

            animState->state       = uint8(f.read16u());
            animState->rangesCount = uint8(f.read16u());
            animState->rangesStart = f.read16u();
        }
    }

    // TODO

    return true;
}
#endif
