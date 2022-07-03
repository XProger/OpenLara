#ifndef H_PHD
#define H_PHD

#include "common.h"
#include "stream.h"

bool read_PHD(DataStream &f)
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
        info->meshesCount = 0; // TODO

        info->alternateRoom = uint8(f.read16s());
        uint16 flags = f.read16u();

        info->flags = 0;
        if (flags & 1) info->flags |= 1; // TODO 1?
        if (flags & 256) info->flags |= 2;  // TODO 2?

    // prepare room
        Room* room = rooms + i;
        room->info = info;
        room->data = info->data;
        room->sectors = info->data.sectors;
        room->firstItem = NULL;
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
        uint32 meshDataCount = f.read32u();
        int32 meshDataPos = f.getPos();
        f.seek(meshDataCount * sizeof(uint16));

        level.meshesCount = f.read32u();
        level.meshOffsets = (int32*)ptr;
        for (uint32 i = 0; i < level.meshesCount; i++)
        {
            ((int32*)ptr)[i] = f.read32s();
        }
        ptr += level.meshesCount * sizeof(int32);

        int32 endPos = f.getPos();

        level.meshes = (const Mesh**)ADDR_ALIGN4(ptr);
        for (int32 i = 0; i < level.meshesCount; i++)
        {
            if ((level.meshOffsets[i] == 0) && (i > 0))
                continue;
            f.setPos(meshDataPos + level.meshOffsets[i]);

            *(int32*)&level.meshOffsets[i] = ptr - (uint8*)level.meshes;

            Mesh* mesh = (Mesh*)ptr;
            ptr += sizeof(Mesh);

            mesh->center.x = f.read16s();
            mesh->center.y = f.read16s();
            mesh->center.z = f.read16s();
            mesh->radius   = f.read16s();
            mesh->intensity = 4096;
            f.seek(2); // skip flags
            
            { // mesh vertices
                mesh->vCount = uint8(f.read16s());
                for (int32 j = 0; j < mesh->vCount; j++)
                {
                    MeshVertex* v = (MeshVertex*)ptr;
                    ptr += sizeof(MeshVertex);
                    v->x = f.read16s();
                    v->y = f.read16s();
                    v->z = f.read16s();
                }
            }

            { // mesh lighting
                mesh->hasNormals = (f.read16s() > 0);

                if (mesh->hasNormals)
                { // normals
                    for (int32 j = 0; j < mesh->vCount; j++)
                    {
                        vec3s* v = (vec3s*)ptr;
                        ptr += sizeof(vec3s);
                        v->x = f.read16s();
                        v->y = f.read16s();
                        v->z = f.read16s();
                    }
                }
                else
                { // intensity
                    for (int32 j = 0; j < mesh->vCount; j++)
                    {
                        *(uint16*)ptr++ = f.read16u();
                    }
                }
            }

            { // mesh rects
                mesh->rCount = f.read16s();
                for (int32 j = 0; j < mesh->rCount; j++)
                {
                    MeshQuad* v = (MeshQuad*)ptr;
                    ptr += sizeof(MeshQuad);
                    v->indices[0] = uint8(f.read16u());
                    v->indices[1] = uint8(f.read16u());
                    v->indices[2] = uint8(f.read16u());
                    v->indices[3] = uint8(f.read16u());
                    v->flags      = f.read16u();
                }
            }

            { // mesh triangles
                mesh->tCount = f.read16s();
                for (int32 j = 0; j < mesh->tCount; j++)
                {
                    MeshTriangle* v = (MeshTriangle*)ptr;
                    ptr += sizeof(MeshTriangle);
                    v->indices[0] = uint8(f.read16u());
                    v->indices[1] = uint8(f.read16u());
                    v->indices[2] = uint8(f.read16u());
                    v->indices[3] = 0;
                    v->flags      = f.read16u();
                }
            }

            { // mesh colored rects
                mesh->crCount = f.read16s();
                for (int32 j = 0; j < mesh->crCount; j++)
                {
                    MeshQuad* v = (MeshQuad*)ptr;
                    ptr += sizeof(MeshQuad);
                    v->indices[0] = uint8(f.read16u());
                    v->indices[1] = uint8(f.read16u());
                    v->indices[2] = uint8(f.read16u());
                    v->indices[3] = uint8(f.read16u());
                    v->flags      = f.read16u();
                }
            }

            { // mesh colored triangles
                mesh->ctCount = f.read16s();
                for (int32 j = 0; j < mesh->ctCount; j++)
                {
                    MeshTriangle* v = (MeshTriangle*)ptr;
                    ptr += sizeof(MeshTriangle);
                    v->indices[0] = uint8(f.read16u());
                    v->indices[1] = uint8(f.read16u());
                    v->indices[2] = uint8(f.read16u());
                    v->indices[3] = 0;
                    v->flags      = f.read16u();
                }
            }
        }

        f.setPos(endPos);
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

    { // anim states
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

    { // anim ranges
        uint32 animRangesCount = f.read32u();
        level.animRanges = (AnimRange*)ADDR_ALIGN4(ptr);
        for (uint32 i = 0; i < animRangesCount; i++)
        {
            AnimRange* animRange = (AnimRange*)ptr;
            ptr += sizeof(AnimRange);

            animRange->frameBegin     = f.read16u();
            animRange->frameEnd       = f.read16u();
            animRange->nextAnimIndex  = f.read16u();
            animRange->nextFrameIndex = f.read16u();
        }
    }

    { // anim commands
        uint32 animCommandsCount = f.read32u();
        level.animCommands = (int16*)ADDR_ALIGN4(ptr);
        for (uint32 i = 0; i < animCommandsCount; i++)
        {
            ((int16*)ptr)[i] = f.read16s();
        }
        ptr += animCommandsCount * sizeof(int16);
    }

    { // nodes
        uint32 nodesSize = f.read32u();
        level.nodes = (ModelNode*)ADDR_ALIGN4(ptr);
        for (uint32 i = 0; i < nodesSize / 4; i++)
        {
            ModelNode* node = (ModelNode*)ptr;
            ptr += sizeof(ModelNode);

            node->flags = uint16(f.read32u());
            node->pos.x = int16(f.read32s());
            node->pos.y = int16(f.read32s());
            node->pos.z = int16(f.read32s());
        }
    }

    { // anim frames
        uint32 animFramesCount = f.read32u();
        level.animFrames = (uint16*)ADDR_ALIGN4(ptr);
        for (uint32 i = 0; i < animFramesCount; i++)
        {
            ((uint16*)ptr)[i] = f.read16u();
        }
        ptr += animFramesCount * sizeof(uint16);
    }

    { // models
        level.modelsCount = f.read32u();
        level.models = (Model*)ADDR_ALIGN4(ptr);
        for (uint32 i = 0; i < level.modelsCount; i++)
        {
            Model* model = (Model*)ptr;
            ptr += sizeof(Model);

            model->type      = uint8(f.read32u());
            model->count     = uint8(f.read16u());
            model->start     = uint16(f.read16u());
            model->nodeIndex = uint16(f.read32u() / 4);
            f.seek(4); // skip frameIndex
            model->animIndex = uint16(f.read16u());
        }
    }

    { // static meshes        
        level.staticMeshesCount = f.read32u();
        level.staticMeshes = (StaticMesh*)ADDR_ALIGN4(ptr);
        for (uint32 i = 0; i < level.staticMeshesCount; i++)
        {
            StaticMesh* staticMesh = (StaticMesh*)ptr;
            ptr += sizeof(StaticMesh);

            staticMesh->id        = uint16(f.read32u());
            staticMesh->meshIndex = f.read16u();
            staticMesh->vbox.minX = f.read16s();
            staticMesh->vbox.maxX = f.read16s();
            staticMesh->vbox.minY = f.read16s();
            staticMesh->vbox.maxY = f.read16s();
            staticMesh->vbox.minZ = f.read16s();
            staticMesh->vbox.maxZ = f.read16s();
            staticMesh->cbox.minX = f.read16s();
            staticMesh->cbox.maxX = f.read16s();
            staticMesh->cbox.minY = f.read16s();
            staticMesh->cbox.maxY = f.read16s();
            staticMesh->cbox.minZ = f.read16s();
            staticMesh->cbox.maxZ = f.read16s();
            staticMesh->flags     = f.read16u();

            // TODO calc Sphere16
        }
    }

    { // textures     
        level.texturesCount = f.read32u();
        level.textures = (Texture*)ADDR_ALIGN4(ptr);
        for (uint32 i = 0; i < level.texturesCount; i++)
        {
            Texture* texture = (Texture*)ptr;
            ptr += sizeof(Texture);

            f.seek(2); // skip attribute

            texture->tile = f.read16u();

            uint32 uv0 = f.read32u() & 0xFF00FF00;
            uint32 uv1 = f.read32u() & 0xFF00FF00;
            uint32 uv2 = f.read32u() & 0xFF00FF00;
            uint32 uv3 = f.read32u() & 0xFF00FF00;

            texture->uv01 = uv0 | (uv1 >> 8);
            texture->uv23 = uv2 | (uv3 >> 8);
        }
    }

    { // sprite
        level.spritesCount = f.read32u();
        level.sprites = (Sprite*)ADDR_ALIGN4(ptr);
        for (uint32 i = 0; i < level.spritesCount; i++)
        {
            Sprite* sprite = (Sprite*)ptr;
            ptr += sizeof(Sprite);

            sprite->tile = f.read16u();
            uint32 u     = f.read8u();
            uint32 v     = f.read8u();
            uint32 w     = (f.read16u() + 255) >> 8;
            uint32 h     = (f.read16u() + 255) >> 8;
            sprite->uwvh = (u << 24) | (w << 16) | (v << 8) | h;
            sprite->l    = f.read16s();
            sprite->t    = f.read16s();
            sprite->r    = f.read16s();
            sprite->b    = f.read16s();
        }
    }

    { // sprite sequences
        level.spriteSequencesCount = f.read32u();
        level.spriteSequences = (SpriteSeq*)ADDR_ALIGN4(ptr);
        for (uint32 i = 0; i < level.spriteSequencesCount; i++)
        {
            SpriteSeq* spriteSeq = (SpriteSeq*)ptr;
            ptr += sizeof(SpriteSeq);

            spriteSeq->type  = f.read16u();
            f.seek(2);
            spriteSeq->unused = 0;
            spriteSeq->count  = f.read16s();
            spriteSeq->start  = f.read16u();
        }
    }

    { // cameras
        level.camerasCount = f.read32u();
        level.cameras = (FixedCamera*)ADDR_ALIGN4(ptr);
        for (uint32 i = 0; i < level.camerasCount; i++)
        {
            FixedCamera* camera = (FixedCamera*)ptr;
            ptr += sizeof(FixedCamera);

            camera->pos.x     = f.read32s();
            camera->pos.y     = f.read32s();
            camera->pos.z     = f.read32s();
            camera->roomIndex = f.read16s();
            camera->flags     = f.read16u();
        }
    }

    { // sound sources
        level.soundSourcesCount = f.read32u();
        level.soundSources = (SoundSource*)ADDR_ALIGN4(ptr);
        for (uint32 i = 0; i < level.soundSourcesCount; i++)
        {
            SoundSource* soundSrc = (SoundSource*)ptr;
            ptr += sizeof(SoundSource);

            soundSrc->pos.x = f.read32s();
            soundSrc->pos.y = f.read32s();
            soundSrc->pos.z = f.read32s();
            soundSrc->id    = f.read16u();
            soundSrc->flags = f.read16u();
        }
    }

    { // boxes
        level.boxesCount = f.read32u();
        level.boxes = (Box*)ADDR_ALIGN4(ptr);
        for (uint32 i = 0; i < level.boxesCount; i++)
        {
            Box* box = (Box*)ptr;
            ptr += sizeof(Box);

            box->minZ    = f.read32s() >> 10;
            box->maxZ    = (f.read32s() + 1) >> 10;
            box->minX    = f.read32s() >> 10;
            box->maxX    = (f.read32s() + 1) >> 10;
            box->floor   = f.read16s();
            box->overlap = f.read16s();
        }
    }

    { // overlaps
        uint32 overlapsCount = f.read32u();
        level.overlaps = (uint16*)ADDR_ALIGN4(ptr);
        for (uint32 i = 0; i < overlapsCount; i++)
        {
            ((uint16*)ptr)[i] = f.read16u();
        }
        ptr += overlapsCount * sizeof(uint16);
    }

    { // zones
        for (int32 k = 0; k < 2; k++)
        {
            for (int32 j = 0; j < ZONE_MAX; j++)
            {
                level.zones[k][j] = (uint16*)ADDR_ALIGN4(ptr);
                for (uint32 i = 0; i < level.boxesCount; i++)
                {
                    ((uint16*)ptr)[i] = f.read16u();
                }
                ptr += level.boxesCount * sizeof(uint16);
            }
        }
    }

    { // anim textures
        uint32 animTexDataCount = f.read32u();
        level.animTexData = (uint16*)ADDR_ALIGN4(ptr);
        for (uint32 i = 0; i < animTexDataCount; i++)
        {
            ((uint16*)ptr)[i] = f.read16s();
        }
        ptr += animTexDataCount * sizeof(uint16);
    }

    { // items
        level.itemsCount = f.read32u();
        level.itemsInfo = (ItemObjInfo*)ADDR_ALIGN4(ptr);
        for (uint32 i = 0; i < level.itemsCount; i++)
        {
            ItemObjInfo* item = (ItemObjInfo*)ptr;
            ptr += sizeof(ItemObjInfo);

            item->type      = uint8(f.read16u());
            item->roomIndex = uint8(f.read16u());

            const RoomInfo* info = level.roomsInfo + item->roomIndex;
            item->pos.x     = int16(f.read32s() - (info->x << 8));
            item->pos.y     = int16(f.read32s());
            item->pos.z     = int16(f.read32s() - (info->z << 8));
                
            int16 angleY    = f.read16s();
            int16 intensity = f.read16s();
            item->intensity = intensity < 0 ? 0 : (intensity >> 5);
            item->flags     = f.read16u() | ((angleY / ANGLE_90 + 2) << 14);
        }
    }

    // lightmap
    f.seek(256 * 32); // skip

    { // palette
        level.palette = (uint16*)ADDR_ALIGN4(ptr);
        for (int32 i = 0; i < 256; i++)
        {
            *ptr++ = f.read8u() << 2;
            *ptr++ = f.read8u() << 2;
            *ptr++ = f.read8u() << 2;
        }
    }

    // TODO
    // cameraFrames
    // demoData
    // soundMap
    // soundInfo
    // soundData
    // soundOffsets

    { // fix head mask
        #define SET_ROT(joint, mask) (((ModelNode*)level.nodes)[level.models[i].nodeIndex + joint]).flags |= mask;

        for (int32 i = 0; i < level.modelsCount; i++)
        {
            switch (level.models[i].type)
            {
                case ITEM_WOLF            : SET_ROT(2, NODE_FLAG_ROTY); break;
                case ITEM_BEAR            : SET_ROT(13, NODE_FLAG_ROTY); break;
                //case ITEM_BAT             : break;
                case ITEM_CROCODILE_LAND  : SET_ROT(7, NODE_FLAG_ROTY); break;
                case ITEM_CROCODILE_WATER : SET_ROT(7, NODE_FLAG_ROTY); break;
                case ITEM_LION_MALE       : SET_ROT(19, NODE_FLAG_ROTY); break;
                case ITEM_LION_FEMALE     : SET_ROT(19, NODE_FLAG_ROTY); break;
                case ITEM_PUMA            : SET_ROT(19, NODE_FLAG_ROTY); break;
                case ITEM_GORILLA         : SET_ROT(13, NODE_FLAG_ROTY); break;
                case ITEM_RAT_LAND        : SET_ROT(1, NODE_FLAG_ROTY); break;
                case ITEM_RAT_WATER       : SET_ROT(1, NODE_FLAG_ROTY); break;
                case ITEM_REX             : SET_ROT(10, NODE_FLAG_ROTY); SET_ROT(11, NODE_FLAG_ROTY); break;
                case ITEM_RAPTOR          : SET_ROT(21, NODE_FLAG_ROTY); break;
                case ITEM_MUTANT_1        : SET_ROT(0, NODE_FLAG_ROTY); SET_ROT(2, NODE_FLAG_ROTY); break;
                case ITEM_MUTANT_2        : SET_ROT(0, NODE_FLAG_ROTY); SET_ROT(2, NODE_FLAG_ROTY); break;
                case ITEM_MUTANT_3        : SET_ROT(0, NODE_FLAG_ROTY); SET_ROT(2, NODE_FLAG_ROTY); break;
                case ITEM_CENTAUR         : SET_ROT(10, NODE_FLAG_ROTX | NODE_FLAG_ROTY); break;
                case ITEM_MUMMY           : SET_ROT(2, NODE_FLAG_ROTY); break;
                case ITEM_LARSON          : SET_ROT(6, NODE_FLAG_ROTY); break;
                case ITEM_PIERRE          : SET_ROT(6, NODE_FLAG_ROTY); break;
                case ITEM_SKATER          : SET_ROT(0, NODE_FLAG_ROTY); break;
                case ITEM_COWBOY          : SET_ROT(0, NODE_FLAG_ROTY); break;
                case ITEM_MR_T            : SET_ROT(0, NODE_FLAG_ROTY); break;
                case ITEM_NATLA           : SET_ROT(2, NODE_FLAG_ROTX | NODE_FLAG_ROTZ); break;
                case ITEM_ADAM            : SET_ROT(1, NODE_FLAG_ROTY); break;
                default                   : break;
            }
        }

        #undef SET_ROT
    }

    return true;
}
#endif
