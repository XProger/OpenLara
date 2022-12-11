#ifndef H_TR1_PC
#define H_TR1_PC

#include "common.h"

#pragma pack(1)
struct TR1_PC
{
    struct Tile
    {
        uint8 indices[256 * 256];
    };

    struct Palette
    {
        uint8 colors[256 * 3];
    };

    struct Quad
    {
        uint16 indices[4];
        uint16 flags;

        static int cmp(const Quad* a, const Quad* b)
        {
            return a->indices[0] - b->indices[0];
        }
    };

    struct Triangle
    {
        uint16 indices[3];
        uint16 flags;
    };

    struct Room
    {
        struct Info
        {
            int32 x;
            int32 z;
            int32 yBottom;
            int32 yTop;
            int32 dataSize;
        };

        struct Vertex
        {
            vec3s pos;
            int16 lighting;
        };

        struct Sprite
        {
            uint16 index;
            uint16 texture;
        };

        struct Portal
        {
            int16 roomIndex;
            vec3s normal;
            vec3s vertices[4];

            void write(FileStream &f) const
            {
                f.write(roomIndex);
                f.write(normal.x);
                f.write(normal.y);
                f.write(normal.z);
                for (int32 i = 0; i < 4; i++)
                {
                    f.write(vertices[i].x);
                    f.write(vertices[i].y);
                    f.write(vertices[i].z);
                }
            }
        };

        struct Sector
        {
            uint16 floorIndex;
            uint16 boxIndex;
            uint8 roomBelow;
            int8 floor;
            uint8 roomAbove;
            int8 ceiling;

            void write(FileStream &f) const
            {
                f.write(floorIndex);
                f.write(boxIndex);
                f.write(roomBelow);
                f.write(floor);
                f.write(roomAbove);
                f.write(ceiling);
            }
        };

        struct Light
        {
            vec3i pos;
            uint16 intensity;
            int32 radius;
        };

        struct Mesh
        {
            vec3i pos;
            int16 angleY;
            uint16 intensity;
            uint16 id;
        };

        Info info;

        int16 vCount;
        Vertex* vertices;

        int16 qCount;
        Quad* quads;

        int16 tCount;
        Triangle* triangles;

        int16 sCount;
        Sprite* sprites;

        int16 pCount;
        Portal* portals;

        uint16 zSectors;
        uint16 xSectors;
        Sector* sectors;

        uint16 ambient;

        uint16 lCount;
        Light* lights;

        uint16 mCount;
        Mesh* meshes;

        int16 alternateRoom;
        uint16 flags;
    };

    struct FloorData
    {
        uint16 value;

        void write(FileStream &f) const
        {
            f.write(value);
        }
    };

    struct Animation
    {
        uint32 frameOffset;

        uint8 frameRate;
        uint8 frameSize;
        uint16 state;

        uint32 speed;
        uint32 accel;

        uint16 frameBegin;
        uint16 frameEnd;

        uint16 nextAnimIndex;
        uint16 nextFrameIndex;

        uint16 statesCount;
        uint16 statesStart;

        uint16 commandsCount;
        uint16 commandsStart;

        void write(FileStream &f) const
        {
            f.write(frameOffset);
            f.write(frameRate);
            f.write(frameSize);
            f.write(state);
            f.write(speed);
            f.write(accel);
            f.write(frameBegin);
            f.write(frameEnd);
            f.write(nextAnimIndex);
            f.write(nextFrameIndex);
            f.write(statesCount);
            f.write(statesStart);
            f.write(commandsCount);
            f.write(commandsStart);
        }
    };

    struct AnimState
    {
        uint16 state;
        uint16 rangesCount;
        uint16 rangesStart;
    };

    struct AnimRange
    {
        int16 frameBegin;
        int16 frameEnd;
        int16 nextAnimIndex;
        int16 nextFrameIndex;

        void write(FileStream &f) const
        {
            f.write(frameBegin);
            f.write(frameEnd);
            f.write(nextAnimIndex);
            f.write(nextFrameIndex);
        }
    };

    struct Model
    {
        uint32 type;
        uint16 count;
        uint16 start;
        uint32 nodeIndex;
        uint32 frameIndex;
        uint16 animIndex;
    };

    struct MinMax
    {
        int16 minX, maxX;
        int16 minY, maxY;
        int16 minZ, maxZ;
    };

    struct StaticMesh
    {
        uint32 id;
        uint16 meshIndex;
        MinMax vbox;
        MinMax cbox;
        uint16 flags;
    };

    struct ObjectTexture
    {
        uint16 attribute;
        uint16 tile;
        union {
            struct { uint8 xh0, x0, yh0, y0; };
            uint32 uv0;
        };
        
        union {
            struct { uint8 xh1, x1, yh1, y1; };
            uint32 uv1;
        };

        union {
            struct { uint8 xh2, x2, yh2, y2; };
            uint32 uv2;
        };

        union {
            struct { uint8 xh3, x3, yh3, y3; };
            uint32 isQuad;
            uint32 uv3;
        };
    };

    struct SpriteTexture
    {
        uint16 tile;
        uint8 u, v;
        uint16 w, h;
        int16 l, t, r, b;
    };

    struct SpriteSequence
    {
        uint16 type;
        uint16 unused;
        int16 count;
        uint16 start;

        void write(FileStream &f) const
        {
            f.write(type);
            f.write(unused);
            f.write(count);
            f.write(start);
        }
    };

    struct Camera
    {
        vec3i pos;
        int16 roomIndex;
        uint16 flags;

        void write(FileStream &f) const
        {
            f.write(pos.x);
            f.write(pos.y);
            f.write(pos.z);
            f.write(roomIndex);
            f.write(flags);
        }
    };

    struct SoundSource
    {
        vec3i pos;
        uint16 id;
        uint16 flags;

        void write(FileStream &f) const
        {
            f.write(pos.x);
            f.write(pos.y);
            f.write(pos.z);
            f.write(id);
            f.write(flags);
        }
    };

    struct Box
    {
        int32 minZ, maxZ;
        int32 minX, maxX;
        int16 floor;
        int16 overlap;
    };

    struct Zone
    {
        uint16* ground1;
        uint16* ground2;
        uint16* fly;
    };

    struct Item
    {
        uint16 type;
        int16 roomIndex;
        vec3i pos;
        int16 angleY;
        int16 intensity;
        uint16 flags;
    };

    struct CameraFrame
    {
        vec3s target;
        vec3s pos;
        int16 fov;
        int16 roll;

        void write(FileStream &f) const
        {
            f.write(target.x);
            f.write(target.y);
            f.write(target.z);
            f.write(pos.x);
            f.write(pos.y);
            f.write(pos.z);
            f.write(fov);
            f.write(roll);
        }
    };

    struct SoundInfo
    {
        uint16 index;
        uint16 volume;
        uint16 chance;

        union {
            struct {
                uint16 mode:2, count:4, unused:6, camera:1, pitch:1, gain:1, :1;
            };

            uint16 value;
        } flags;

        void write(FileStream &f) const
        {
            f.write(index);
            f.write(volume);
            f.write(chance);
            f.write(flags.value);
        }
    };

    enum NodeFlag
    {
        NODE_FLAG_POP  = (1 << 0),
        NODE_FLAG_PUSH = (1 << 1),
        NODE_FLAG_ROTX = (1 << 2),
        NODE_FLAG_ROTY = (1 << 3),
        NODE_FLAG_ROTZ = (1 << 4),
    };

    struct Node
    {
        uint32 flags;
        vec3i pos;
    };

    LevelID id;

    int32 tilesCount;
    Tile* tiles;

    int16 roomsCount;
    Room* rooms;

    int32 floorsCount;
    FloorData* floors;

    int32 meshDataSize;
    uint16* meshData;

    int32 meshOffsetsCount;
    uint32* meshOffsets;

    int32 animsCount;
    Animation* anims;

    int32 statesCount;
    AnimState* states;

    int32 rangesCount;
    AnimRange* ranges;

    int32 commandsCount;
    int16* commands;

    int32 nodesDataSize;
    uint32* nodesData;

    int32 frameDataSize;
    uint16* frameData;

    int32 modelsCount;
    Model* models;

    int32 staticMeshesCount;
    StaticMesh* staticMeshes;

    int32 objectTexturesCount;
    ObjectTexture* objectTextures;

    int32 spriteTexturesCount;
    SpriteTexture* spriteTextures;

    int32 spriteSequencesCount;
    SpriteSequence* spriteSequences;

    int32 camerasCount;
    Camera* cameras;

    int32 soundSourcesCount;
    SoundSource* soundSources;

    int32 boxesCount;
    Box* boxes;

    int32 overlapsCount;
    uint16* overlaps;

    Zone zones[2];

    int32 animTexDataSize;
    uint16* animTexData;

    int32 itemsCount;
    Item* items;

    uint8 lightmap[32 * 256];
    Palette palette;

    uint16 cameraFramesCount;
    CameraFrame* cameraFrames;

    uint16 demoDataSize;
    uint8* demoData;

    int16 soundMap[256];
    int32 soundInfoCount;
    SoundInfo* soundInfo;

    int32 soundDataSize;
    uint8* soundData;

    int32 soundOffsetsCount;
    uint32* soundOffsets;

    TR1_PC(FileStream &f, LevelID id)
    {
        this->id = id;
        tiles = NULL;

        uint32 magic;
        f.read(magic);

        if (magic != 0x00000020)
        {
            printf("Unsupported level format\n");
            return;
        }

        f.readArray(tiles, tilesCount);
        f.seek(4);

        f.read(roomsCount);
        rooms = new Room[roomsCount];
        
        for (int32 i = 0; i < roomsCount; i++)
        {
            Room* room = rooms + i;

            f.read(room->info);
            f.readArray(room->vertices, room->vCount);
            f.readArray(room->quads, room->qCount);
            f.readArray(room->triangles, room->tCount);
            f.readArray(room->sprites, room->sCount);
            f.readArray(room->portals, room->pCount);
            f.read(room->zSectors);
            f.read(room->xSectors);
            f.read(room->sectors, room->zSectors * room->xSectors);
            f.read(room->ambient);
            f.readArray(room->lights, room->lCount);
            f.readArray(room->meshes, room->mCount);
            f.read(room->alternateRoom);
            f.read(room->flags); 
        }

        f.readArray(floors, floorsCount);
        
        f.readArray(meshData, meshDataSize);
        f.readArray(meshOffsets, meshOffsetsCount);
        f.readArray(anims, animsCount);
        f.readArray(states, statesCount);
        f.readArray(ranges, rangesCount);
        f.readArray(commands, commandsCount);
        f.readArray(nodesData, nodesDataSize);
        f.readArray(frameData, frameDataSize);
        f.readArray(models, modelsCount);
        f.readArray(staticMeshes, staticMeshesCount);
        f.readArray(objectTextures, objectTexturesCount);
        f.readArray(spriteTextures, spriteTexturesCount);
        f.readArray(spriteSequences, spriteSequencesCount);

        f.readArray(cameras, camerasCount);
        f.readArray(soundSources, soundSourcesCount);
        f.readArray(boxes, boxesCount);
        f.readArray(overlaps, overlapsCount);

        for (int32 i = 0; i < 2; i++)
        {
            f.read(zones[i].ground1, boxesCount);
            f.read(zones[i].ground2, boxesCount);
            f.read(zones[i].fly, boxesCount);      
        }

        f.readArray(animTexData, animTexDataSize);
        f.readArray(items, itemsCount);
        f.read(lightmap);
        f.read(palette);
        f.readArray(cameraFrames, cameraFramesCount);
        f.readArray(demoData, demoDataSize);

        f.read(soundMap);
        f.readArray(soundInfo, soundInfoCount);
        f.readArray(soundData, soundDataSize);
        f.readArray(soundOffsets, soundOffsetsCount);

        palette.colors[0] = 0;
        palette.colors[1] = 0;
        palette.colors[2] = 0;
    }

    ~TR1_PC()
    {
        delete[] tiles;
        
        for (int32 i = 0; i < roomsCount; i++)
        {
            Room* room = rooms + i;
            delete[] room->vertices;
            delete[] room->quads;
            delete[] room->triangles;
            delete[] room->sprites;
            delete[] room->portals;
            delete[] room->sectors;
            delete[] room->lights;
            delete[] room->meshes;
        }

        delete[] rooms;
        delete[] floors;
        delete[] meshData;
        delete[] meshOffsets;
        delete[] anims;
        delete[] states;
        delete[] ranges;
        delete[] commands;
        delete[] nodesData;
        delete[] frameData;
        delete[] models;
        delete[] staticMeshes;
        delete[] objectTextures;
        delete[] spriteTextures;
        delete[] spriteSequences;
        delete[] cameras;
        delete[] soundSources;
        delete[] boxes;
        delete[] overlaps;

        for (int32 i = 0; i < 2; i++)
        {
            delete[] zones[i].ground1;
            delete[] zones[i].ground2;
            delete[] zones[i].fly;
        }

        delete[] animTexData;
        delete[] items;
        delete[] cameraFrames;
        delete[] demoData;
        delete[] soundInfo;
        delete[] soundData;
        delete[] soundOffsets;
    }

    void fixHeadMask()
    {
        #define SET_ROT(joint, mask) (((Node*)nodesData)[models[i].nodeIndex / 4 + joint]).flags |= mask;

        for (int32 i = 0; i < modelsCount; i++)
        {
            switch (models[i].type)
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

    int32 getModelIndex(int32 type)
    {
        for (int32 i = 0; i < modelsCount; i++)
        {
            if (models[i].type == type) {
                return i;
            }
        }
        return -1;
    }

    template <typename T>
    T* addElements(T* &a, int32 &count, int32 size)
    {
        T* ptr = new T[count + size];
        memcpy(ptr, a, sizeof(a[0]) * count);
        delete[] a;
        a = ptr;
        count += size;
        return &a[count - size];
    }

    int32 getMeshTexture(uint16* meshPtr)
    {
        meshPtr += 3 + 1 + 1; // skip center, radius, flags
        int16 vCount = *(int16*)meshPtr;
        meshPtr += 1; // skip vCount
        meshPtr += vCount * 3; // skip vertices
        int16 nCount = *(int16*)meshPtr;
        meshPtr += 1; // skip nCount
        if (nCount > 0) {
            meshPtr += nCount * 3; // skip normals
        } else {
            meshPtr -= nCount; // skip intensity
        }
        int16 rCount = *(int16*)meshPtr;
        meshPtr += 1; // skip rCount
        if (rCount > 0) {
            meshPtr += 4; // skip indices
            return (*(uint16*)meshPtr) & 0x07FF;
        }
        int16 tCount = *(int16*)meshPtr;
        meshPtr += 1; // skip tCount
        if (tCount > 0)
        {
            meshPtr += 3; // skip indices
            return (*(uint16*)meshPtr) & 0x07FF;
        }
        // no textured quads or triangles
        ASSERT(false);
        return -1;
    }

    int32 getMaxTexture(int32 tile, int32 x, int32 y, int32 &minU, int32 &minV, int32 &maxU, int32 &maxV)
    {
        int32 index = -1;
        int32 maxW = 0;
        int32 maxH = 0;

        for (int32 i = 0; i < objectTexturesCount; i++)
        {
            ObjectTexture *tex = objectTextures + i;

            //if (!tex->isQuad)
            //    continue;

            if (tex->tile != tile)
                continue;

            int32 minX = MIN(MIN(tex->x0, tex->x1), tex->x2);
            int32 minY = MIN(MIN(tex->y0, tex->y1), tex->y2);
            int32 maxX = MAX(MAX(tex->x0, tex->x1), tex->x2);
            int32 maxY = MAX(MAX(tex->y0, tex->y1), tex->y2);

            if (tex->isQuad)
            {
                minX = MIN(minX, tex->x3);
                minY = MIN(minY, tex->y3);
                maxX = MAX(maxX, tex->x3);
                maxY = MAX(maxY, tex->y3);
            }

            if (x >= minX && x <= maxX && y >= minY && y <= maxY)
            {
                int32 w = maxX - minX;
                int32 h = maxY - minY;

                if (w >= maxW && h >= maxH)
                {
                    index = i;
                    maxW = w;
                    maxH = h;

                    minU = minX;
                    minV = minY;
                    maxU = maxX;
                    maxV = maxY;
                }
            }
        }

        ASSERT(index >= 0);

        return index;
    }

    void generateLODs()
    {
        struct Quad {
            uint16 indices[4];
            uint16 flags;
        };

        struct Mesh {
            vec3s center;
            int16 radius;
            uint16 flags;
            int16 vCount;
            vec3s vertices[4];
            int16 nCount;
            int16 intensity[4];
            int16 rCount;
            Quad rFaces[2];
            int16 tCount;
            int16 crCount;
            int16 ctCount;
        };

        struct AnimFrame {
            MinMax box;
            vec3s pos;
            uint16 angles[2];
        };

        AnimFrame meshPlaneFrame;
        meshPlaneFrame.box.minX = -512;
        meshPlaneFrame.box.maxX = 512;
        meshPlaneFrame.box.minY = -512;
        meshPlaneFrame.box.maxY = -512;
        meshPlaneFrame.box.minZ = -512;
        meshPlaneFrame.box.maxZ = 512;

        meshPlaneFrame.pos = vec3s(0, -512, 0);
        meshPlaneFrame.angles[0] = meshPlaneFrame.angles[1] = 0;

        Mesh meshPlane;
        meshPlane.center = vec3s(-512, 0, 512);
        meshPlane.radius = 727;
        meshPlane.flags = 1;
        meshPlane.vCount = 4;
        meshPlane.vertices[0] = vec3s(-512, 0, -512);
        meshPlane.vertices[1] = vec3s( 512, 0, -512);
        meshPlane.vertices[2] = vec3s( 512, 0,  512);
        meshPlane.vertices[3] = vec3s(-512, 0,  512);
        meshPlane.nCount = -4;
        meshPlane.intensity[0] = 3800;
        meshPlane.intensity[1] = 3800;
        meshPlane.intensity[2] = 3800;
        meshPlane.intensity[3] = 3800;
        meshPlane.rCount = 2;
        meshPlane.rFaces[0].indices[0] = 3;
        meshPlane.rFaces[0].indices[1] = 2;
        meshPlane.rFaces[0].indices[2] = 1;
        meshPlane.rFaces[0].indices[3] = 0;
        meshPlane.rFaces[1].indices[0] = 0;
        meshPlane.rFaces[1].indices[1] = 1;
        meshPlane.rFaces[1].indices[2] = 2;
        meshPlane.rFaces[1].indices[3] = 3;
        meshPlane.tCount = 0;
        meshPlane.crCount = 0;
        meshPlane.ctCount = 0;

        // trap floor lod
        int32 index = getModelIndex(ITEM_TRAP_FLOOR);
        
        if (index > -1)
        {
            Model* model = addElements(models, modelsCount, 1);
            *model = models[index];
            model->type = ITEM_TRAP_FLOOR_LOD;

            int32 texture = getMeshTexture((uint16*)((uint8*)meshData + meshOffsets[model->start]));
            ObjectTexture* objTex = objectTextures + texture;

            int32 minU, minV, maxU, maxV;

            texture = getMaxTexture(objTex->tile, (objTex->x0 + objTex->x1 + objTex->x2) / 3, (objTex->y0 + objTex->y1 + objTex->y2) / 3, minU, minV, maxU, maxV);

            objTex = addElements(objectTextures, objectTexturesCount, 1);
            *objTex = objectTextures[texture];
            objTex->x0 = minU;
            objTex->y0 = minV;
            objTex->x1 = maxU;
            objTex->y1 = minV;
            objTex->x2 = maxU;
            objTex->y2 = maxV;
            objTex->x3 = minU;
            objTex->y3 = maxV;

            meshPlane.rFaces[0].flags = objectTexturesCount - 1;
            meshPlane.rFaces[1].flags = objectTexturesCount - 1;

            uint32 *meshOffset = addElements(meshOffsets, meshOffsetsCount, 1);

            uint16* mesh = addElements(meshData, meshDataSize, sizeof(meshPlane) / sizeof(uint16));
            memcpy(mesh, &meshPlane, sizeof(meshPlane));

            *meshOffset = uint32((mesh - meshData) * sizeof(uint16));

            uint16* frame = addElements(frameData, frameDataSize, sizeof(meshPlaneFrame) / sizeof(uint16));
            memcpy(frame, &meshPlaneFrame, sizeof(meshPlaneFrame));

            Animation* anim = addElements(anims, animsCount, 1);
            memset(anim, 0, sizeof(anim[0]));
            anim->frameRate = 1;
            anim->frameOffset = uint32((frame - frameData) << 1);

            Node* node = (Node*)addElements(nodesData, nodesDataSize, sizeof(Node) / sizeof(uint32));
            node->flags = 0;
            node->pos.x = 0;
            node->pos.y = 0;
            node->pos.z = 0;

            model->count = 1;
            model->start = meshOffsetsCount - 1;
            model->animIndex = animsCount - 1;
            model->nodeIndex = uint32((uint32*)node - nodesData);
        }
    }

    void hideRoom(int32 roomIndex)
    {
        Room &room = rooms[roomIndex];
        room.vCount = 0;
        room.qCount = 0;
        room.tCount = 0;
        room.sCount = 0;
        room.pCount = 0;
        room.lCount = 0;
        room.mCount = 0;
        room.zSectors = 0;
        room.xSectors = 0;
        room.alternateRoom = -1;

        for (int32 i = 0; i < roomsCount; i++)
        {
            Room &room = rooms[i];

            int32 j = room.pCount - 1;
            while (j >= 0)
            {
                if (room.portals[j].roomIndex == roomIndex)
                {
                    room.pCount--;
                    room.portals[j] = room.portals[room.pCount];
                }
                j--;
            }
        }
    }

    void removeSound(int32 id)
    {
        if (soundMap[id] == -1)
            return;

        SoundInfo info = soundInfo[soundMap[id]];

        for (int32 index = info.index; index < info.index + info.flags.count; index++)
        {
            int32 offset = soundOffsets[index];

            int32 size;
            if (index == soundOffsetsCount - 1) {
                size = soundDataSize - offset;
            } else {
                size = soundOffsets[index + 1] - offset;
            }

            for (int i = index + 1; i < soundOffsetsCount; i++)
            {
                soundOffsets[i - 1] = soundOffsets[i] - size;
            }
            soundOffsetsCount--;

            for (int32 i = 0; i < soundInfoCount; i++)
            {
                if (soundInfo[i].index >= index)
                {
                    soundInfo[i].index--;
                }
            }

            uint8* data = new uint8[soundDataSize - size];
            memcpy(data, soundData, offset);
            memcpy(data + offset, soundData + offset + size, soundDataSize - offset - size);

            delete[] soundData;
            soundData = data;
            soundDataSize -= size;
        }

        soundMap[id] = -1;
    }
    
    void cutData()
    {
        removeSound(60); // underwater
        removeSound(173); // secret

        if (id == LVL_TR1_GYM)
        {
            hideRoom(0);
            hideRoom(1);
            hideRoom(2);
            hideRoom(3);
            hideRoom(4);
            hideRoom(5);
            hideRoom(6);
            hideRoom(15);
            hideRoom(16);
            hideRoom(17);
            hideRoom(18);

            for (int32 i = 174; i <= 204; i++) // remove tutorial sounds (we use sound tracks instead)
                removeSound(i);

            // disable transparency
            objectTextures[93].attribute =
            objectTextures[167].attribute =
            objectTextures[175].attribute =
            objectTextures[190].attribute =
            objectTextures[191].attribute =
            objectTextures[211].attribute =
            objectTextures[220].attribute =
            objectTextures[221].attribute =
            objectTextures[580].attribute =
            objectTextures[581].attribute = 0;
        }

        if (id == LVL_TR1_1)
        {
            objectTextures[271].attribute =
            objectTextures[272].attribute =
            objectTextures[331].attribute =
            objectTextures[333].attribute =
            objectTextures[334].attribute =
            objectTextures[335].attribute =
            objectTextures[517].attribute =
            objectTextures[518].attribute =
            objectTextures[569].attribute =
            objectTextures[571].attribute =
            objectTextures[685].attribute =
            objectTextures[686].attribute = 0;
        }

        if (id == LVL_TR1_2)
        {
            objectTextures[247].attribute =
            objectTextures[248].attribute =
            objectTextures[307].attribute =
            objectTextures[309].attribute =
            objectTextures[310].attribute =
            objectTextures[311].attribute =
            objectTextures[547].attribute =
            objectTextures[661].attribute =
            objectTextures[662].attribute =
            objectTextures[688].attribute =
            objectTextures[905].attribute =
            objectTextures[906].attribute =
            objectTextures[923].attribute = 0;
        }

        // TODO remove unused textures & models
    }
};

#endif