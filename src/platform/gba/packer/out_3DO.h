#ifndef H_OUT_3DO
#define H_OUT_3DO

#include "common.h"
#include "TR1_PC.h"
#include "TR1_PSX.h"

// TODO use PSX format as source
struct out_3DO
{
    void process(const char* dir, TR1_PC** pc, TR1_PSX** psx)
    {
        //
    }
};


#if 0

#pragma pack(1)
struct LevelPC
{
    struct RoomQuad3DO
    {
        uint16 indices[4];
        uint32 flags;

        void write(FileStream &f) const
        {
            f.write(flags);
            f.write(indices[0]);
            f.write(indices[1]);
            f.write(indices[2]);
            f.write(indices[3]);
        }
    };

    struct MeshQuad3DO
    {
        uint8  indices[4];
        uint32 flags;

        void write(FileStream &f) const
        {
            f.write(flags);
            f.write(indices[3]);
            f.write(indices[2]);
            f.write(indices[1]);
            f.write(indices[0]);
        }
    };

    struct Quad
    {
        uint16 indices[4];
        uint16 flags;

        void write(FileStream &f) const
        {
            f.write(indices[0]);
            f.write(indices[1]);
            f.write(indices[2]);
            f.write(indices[3]);
            f.write(flags);
        }
    };

    struct RoomTriangle3DO
    {
        uint16 indices[3];
        uint16 _unused;
        uint32 flags;

        void write(FileStream &f) const
        {
            f.write(flags);
            f.write(indices[0]);
            f.write(indices[1]);
            f.write(indices[2]);
            f.write(_unused);
        }
    };

    struct MeshTriangle3DO
    {
        uint8  indices[3];
        uint8  _unused;
        uint32 flags;

        void write(FileStream &f) const
        {
            f.write(flags);
            uint8 unused = 0;
            f.write(unused);
            f.write(indices[2]);
            f.write(indices[1]);
            f.write(indices[0]);
        }
    };

    struct Triangle
    {
        uint16 indices[3];
        uint16 flags;

        void write(FileStream &f) const
        {
            f.write(indices[0]);
            f.write(indices[1]);
            f.write(indices[2]);
            f.write(flags);
        }
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

            bool isEqual(const Vertex *v)
            {
                return pos.x == v->pos.x && pos.y == v->pos.y && pos.z == v->pos.z && lighting == v->lighting;
            }
        };

        struct VertexComp
        {
            int8  x, y, z; 
            uint8 g;

            void write(FileStream &f) const
            {
                f.write(x);
                f.write(y);
                f.write(z);
                f.write(g);
            }
        };

        struct Sprite
        {
            uint16 index;
            uint16 texture;
        };

        struct SpriteComp
        {
            int16 x, y, z;
            uint8 g;
            uint8 index;

            void write(FileStream &f) const
            {
                f.write(x);
                f.write(y);
                f.write(z);
                f.write(g);
                f.write(index);
            }
        };
        /*
        struct MeshComp {
            vec3s center;
            int16 radius;

            int16 vCount;
            int16 nCount;

            int16 rCount;
            int16 tCount;

            vec3s* vertices;
            Quad* quads;
            Triangle* triangles;
        };*/

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

        struct PortalComp
        {
            uint32 roomIndex;
            uint32 normalMask;
            vec3i vertices[4];

            void write(FileStream &f) const
            {
                f.write(roomIndex);
                f.write(normalMask);
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

        struct LightComp
        {
            vec3s pos;
            uint8 radius;
            uint8 intensity;

            void write(FileStream &f) const
            {
                f.write(pos.x);
                f.write(pos.y);
                f.write(pos.z);
                f.write(radius);
                f.write(intensity);
            }
        };

        struct Mesh
        {
            vec3i pos;
            int16 angleY;
            uint16 intensity;
            uint16 id;
        };

        struct MeshComp
        {
            vec3s pos;
            uint8 intensity;
            uint8 flags;

            void write(FileStream &f) const
            {
                uint32 xy = (pos.x << 16) | uint16(pos.y);
                uint32 zf = (pos.z << 16) | (intensity << 8) | flags;
                f.write(xy);
                f.write(zf);
            }
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

    struct AnimStateComp
    {
        uint8  state;
        uint8  rangesCount;
        uint16 rangesStart;

        void write(FileStream &f) const
        {
            f.write(state);
            f.write(rangesCount);
            f.write(rangesStart);
        }
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

    struct ModelComp
    {
        uint8  type;
        uint8  count;
        uint16 start;
        uint16 nodeIndex;
        uint16 animIndex;

        void write(FileStream &f) const
        {
            f.write(type);
            f.write(count);
            f.write(start);
            f.write(nodeIndex);
            f.write(animIndex);
        }
    };

    struct MinMax
    {
        int16 minX, maxX;
        int16 minY, maxY;
        int16 minZ, maxZ;

        void write(FileStream &f) const
        {
            f.write(minX); f.write(maxX);
            f.write(minY); f.write(maxY);
            f.write(minZ); f.write(maxZ);
        }
    };

    struct Sphere16
    {
        int16 x, y, z, radius;

        void write(FileStream &f) const
        {
            uint32 xy = (x << 16) | uint16(y);
            uint32 zr = (z << 16) | uint16(radius);
            f.write(xy);
            f.write(zr);
        }
    };
   
    struct StaticMesh
    {
        uint32 id;
        uint16 meshIndex;
        MinMax vbox;
        MinMax cbox;
        uint16 flags;
    };

    struct StaticMeshComp
    {
        uint16 id;
        uint16 meshIndex;
        uint32 flags;
        //Sphere16 vs;
        MinMax vbox;
        MinMax cbox;

        void write(FileStream &f) const
        {
            Sphere16 vs;
            vs.x = (vbox.maxX + vbox.minX) >> 1;
            vs.y = (vbox.maxY + vbox.minY) >> 1;
            vs.z = (vbox.maxZ + vbox.minZ) >> 1;

            int32 dx = (vbox.maxX - vbox.minX) >> 1;
            int32 dy = (vbox.maxY - vbox.minY) >> 1;
            int32 dz = (vbox.maxZ - vbox.minZ) >> 1;

            vs.radius = int32(sqrtf(float(dx * dx + dy * dy + dz * dz)));

            f.write(id);
            f.write(meshIndex);
            f.write(flags);
            vs.write(f);
            vbox.write(f);
            cbox.write(f);
        }
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

    struct ObjectTextureComp
    {
        uint16 attribute;
        uint16 tile;
        uint32 uv0;
        uint32 uv1;
        uint32 uv2;
        uint32 uv3;

        void write(FileStream &f) const
        {
            f.write(attribute);
            f.write(tile);
            f.write(uv0);
            f.write(uv1);
            f.write(uv2);
            f.write(uv3);
/*
            union TexCoord {
                struct { uint16 v, u; };
                uint32 uv;
            } t;


            t.uv = uv0;
            f.write(t.v);
            f.write(t.u);

            t.uv = uv1;
            f.write(t.v);
            f.write(t.u);

            t.uv = uv2;
            f.write(t.v);
            f.write(t.u);

            t.uv = uv3;
            f.write(t.v);
            f.write(t.u);
*/
        }
    };

    struct SpriteTexture
    {
        uint16 tile;
        uint8 u, v;
        uint16 w, h;
        int16 l, t, r, b;

        void write(FileStream &f) const
        {
            f.write(tile);
            f.write(u);
            f.write(v);
            f.write(w);
            f.write(h);
            f.write(l);
            f.write(t);
            f.write(r);
            f.write(b);
        }
    };

    struct SpriteTextureComp
    {
        uint16 tile;
        uint8 u, v;
        uint8 w, h;
        int16 l, t, r, b;

        void write(FileStream &f) const
        {
            f.write(tile);
            f.write(u);
            f.write(v);
            f.write(w);
            f.write(h);
            f.write(l);
            f.write(t);
            f.write(r);
            f.write(b);
        }
    };

    struct SpriteTexture3DO
    {
        uint32 texture;
        int32 l, t, r, b;

        void write(FileStream &f) const
        {
            f.write(texture);
            f.write(l);
            f.write(t);
            f.write(r);
            f.write(b);
        }
    };

    struct SpriteSequence
    {
        uint16 type;
        uint16 unused;
        int16 count;
        int16 start;

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

    struct BoxComp
    {
        uint8 minZ, maxZ;
        uint8 minX, maxX;
        int16 floor;
        int16 overlap;

        void write(FileStream &f) const
        {
            f.write(minZ);
            f.write(maxZ);
            f.write(minX);
            f.write(maxX);
            f.write(floor);
            f.write(overlap);
        }
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

    struct ItemComp
    {
        uint8 type;
        uint8 roomIndex;
        vec3s pos;
        int16 intensity;
        uint16 flags;

        void write(FileStream &f) const
        {
            f.write(type);
            f.write(roomIndex);
            f.write(pos.x);
            f.write(pos.y);
            f.write(pos.z);
            f.write(intensity);
            f.write(flags);
        }
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

    struct NodeComp
    {
        vec3s pos;
        uint16 flags;

        void write(FileStream &f)
        {
            f.write(pos.x);
            f.write(pos.y);
            f.write(pos.z);
            f.write(flags);
        }
    };

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

    LevelPC(const char* fileName)
    {
        tiles = NULL;

        FileStream f(fileName, false);
        
        if (!f.isValid()) return;

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

        markRoomTextures();
    }

    ~LevelPC()
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

    void markRoomTextures()
    {
        for (int32 i = 0; i < roomsCount; i++)
        {
            Room* room = rooms + i;

            for (int32 j = 0; j < room->qCount; j++)
            {
                Quad* q = room->quads + j;
                objectTextures[q->flags & FACE_TEXTURE].attribute |= TEX_ATTR_MIPS;
            }

            for (int32 j = 0; j < room->tCount; j++)
            {
                Triangle* t = room->triangles + j;
                objectTextures[t->flags & FACE_TEXTURE].attribute |= TEX_ATTR_MIPS;
            }
        }
    }

    int32 addRoomVertex(int32 yOffset, const Room::Vertex &v, bool ignoreG = false)
    {
        Room::VertexComp comp;
        int32 px = v.pos.x >> 10;
        int32 py = (v.pos.y - yOffset) >> 8;
        int32 pz = v.pos.z >> 10;

        ASSERT(py >= 0);
        ASSERT(px < 32);
        ASSERT(py < 64);
        ASSERT(pz < 32);

        comp.x = px;
        comp.y = py;
        comp.z = pz;
        comp.g = ignoreG ? 0 : (v.lighting >> 5);

        for (int32 i = 0; i < roomVerticesCount; i++)
        {
            if (memcmp(roomVertices + i, &comp, sizeof(comp)) == 0)
            {
                return i;
            }
        }

        roomVertices[roomVerticesCount] = comp;

        return roomVerticesCount++;
    }

// 3DO ========================================================================
    struct PLUT {
        uint16 colors[16];
    } PLUTs[MAX_TEXTURES];
    int32 plutsCount;

    struct Texture3DO {
        int32 data;
        int32 plut;

        uint8 wShift;
        uint8 hShift;
        uint16 color;

        uint32 pre0;
        uint32 pre1;
        uint8* src;
        int32 w;
        int32 h;
        uint16 flip;
        int16 mip;
        uint8* image;

        void write(FileStream &f) const
        {
            ASSERT(plut * sizeof(PLUT) < 0xFFFF);
            uint32 shift = wShift | (hShift << 8) | ((plut * sizeof(PLUT)) << 16);
            f.write(data);
            f.write(shift);
        }

        bool cmp(const Texture3DO &t)
        {
            if (wShift != t.wShift || hShift != t.hShift || plut != t.plut)
                return false;

            return memcmp(image, t.image, (1 << (20 - wShift)) * (1 << (16 - hShift)) / 2) == 0;
        }

    } textures3DO[MAX_TEXTURES];

    int32 spritesBaseIndex;

    uint32 nextPow2(uint32 x) {
        x--;
        x |= x >> 1;
        x |= x >> 2;
        x |= x >> 4;
        x |= x >> 8;
        x |= x >> 16;
        x++;
        return x;
    }

    uint32 shiftPow2(int32 x)
    {
        int32 count = 0;
        while (x >>= 1) {
            count++;
        }
        return count;
    }

    int32 addPalette(const PLUT &p)
    {
        for (int32 i = 0; i < plutsCount; i++)
        {
            if (memcmp(&PLUTs[i], &p, sizeof(PLUT)) == 0)
            {
                return i;
            }
        }

        PLUTs[plutsCount] = p;

        return plutsCount++;
    }

    template <typename T>
    void calcQuadFlip(T &q)
    {
        Texture3DO* tex = textures3DO + (q.flags & FACE_TEXTURE);
        bool flip = false;

        if (tex->flip & TEX_FLIP_X) {
            swap(q.indices[0], q.indices[1]);
            swap(q.indices[3], q.indices[2]);
            flip = !flip;
        }

        if (tex->flip & TEX_FLIP_Y) {
            swap(q.indices[0], q.indices[3]);
            swap(q.indices[1], q.indices[2]);
            flip = !flip;
        }

        if (flip) {
            q.flags |= FACE_CCW;
        }
    }

    int32 convertTextures3DO(const char* fileName)
    {
        #define PRE1_WOFFSET_PREFETCH   2
        #define PRE0_VCNT_PREFETCH      1
        #define PRE0_VCNT_SHIFT         6
        #define PRE0_BPP_4              3
        #define PRE1_TLHPCNT_PREFETCH   1
        #define PRE1_TLHPCNT_SHIFT      0
        #define PRE1_TLLSB_PDC0         0x00001000
        #define PRE1_WOFFSET8_SHIFT     24
        #define PRE0_BGND               0x40000000
        #define PRE0_LINEAR             0x00000010
        #define PRE0_BPP_16             0x00000006

        ASSERT(objectTexturesCount + spriteTexturesCount < MAX_TEXTURES);

        plutsCount = 0;

        FileStream f(fileName, true);

        if (!f.isValid()) return 0;

        f.bigEndian = true;

    // reserve 4 bytes for the PLUTs offset
        f.seek(4);

    // convert palette to 15-bit and fix some color gradients
        uint16 pal[256];

        for (int32 i = 0; i < 256; i++)
        {
            uint8 b = palette.colors[i * 3 + 0];
            uint8 g = palette.colors[i * 3 + 1];
            uint8 r = palette.colors[i * 3 + 2];

            pal[i] = (r >> 1) | ((g >> 1) << 5) | ((b >> 1) << 10);
        }

        pal[0] = 0;

    // convert palette to 16 x PLUTs
        {
            for (int32 i = 0; i < 16; i++)
            {
                memcpy(PLUTs[i].colors, &pal[i * 16], 16 * sizeof(uint16));
            }
            plutsCount = 16;
        }

    // convert palette to 32-bit
        uint32 pal32[256];
        for (int32 i = 0; i < 256; i++)
        {
            uint16 p = pal[i];

            uint8 r = (p & 31) << 3;
            uint8 g = ((p >> 5) & 31) << 3;
            uint8 b = ((p >> 10) & 31) << 3;

            pal32[i] = r | (g << 8) | (b << 16);

            if (pal32[i]) {
                pal32[i] |= 0xFF000000;
            }
        }
        pal32[0] = 0;

        uint32* bitmap32 = new uint32[256 * 256];
        uint32* bitmap32_tmp = new uint32[256 * 256];
        uint8* bitmap8 = new uint8[256 * 256];
        uint8* bitmap8_tmp = new uint8[256 * 256];

        spritesBaseIndex = objectTexturesCount;

        {
            LevelPC::ObjectTexture* tmp = new LevelPC::ObjectTexture[objectTexturesCount + spriteTexturesCount];
            memcpy(tmp, objectTextures, sizeof(LevelPC::ObjectTexture) * objectTexturesCount);

            for (int32 i = 0; i < spriteTexturesCount; i++)
            {
                LevelPC::SpriteTexture* spriteTexture = spriteTextures + i; 
                LevelPC::ObjectTexture* objectTexture = tmp + objectTexturesCount + i;

                int32 w = spriteTexture->w >> 8;
                int32 h = spriteTexture->h >> 8;

                objectTexture->attribute = TEX_ATTR_AKILL;
                objectTexture->tile = spriteTexture->tile;
                objectTexture->uv0 = 0;
                objectTexture->uv1 = 0;
                objectTexture->uv2 = 0;
                objectTexture->uv3 = 0;
                objectTexture->x0 = spriteTexture->u;
                objectTexture->y0 = spriteTexture->v;
                objectTexture->x1 = spriteTexture->u + w;
                objectTexture->y1 = spriteTexture->v;
                objectTexture->x2 = spriteTexture->u + w;
                objectTexture->y2 = spriteTexture->v + h;
                objectTexture->x3 = spriteTexture->u;
                objectTexture->y3 = spriteTexture->v + h;
            }

            delete[] objectTextures;
            objectTextures = tmp;

            objectTexturesCount += spriteTexturesCount;
        }

        int32 mipIndex = objectTexturesCount;

        int32 dupSize = 0;

        for (int32 i = 0; i < objectTexturesCount; i++)
        {
            const LevelPC::ObjectTexture* objectTexture = objectTextures + i;

            int32 x0 = MIN(MIN(objectTexture->x0, objectTexture->x1), objectTexture->x2);
            int32 y0 = MIN(MIN(objectTexture->y0, objectTexture->y1), objectTexture->y2);
            int32 x1 = MAX(MAX(objectTexture->x0, objectTexture->x1), objectTexture->x2);
            int32 y1 = MAX(MAX(objectTexture->y0, objectTexture->y1), objectTexture->y2);

            textures3DO[i].flip = 0;

            if (objectTexture->isQuad)
            {
                if (objectTexture->x0 > objectTexture->x1) textures3DO[i].flip |= TEX_FLIP_X;
                if (objectTexture->y0 > objectTexture->y2) textures3DO[i].flip |= TEX_FLIP_Y;
            }

            int32 w = x1 - x0 + 1;
            int32 h = y1 - y0 + 1;

            textures3DO[i].src = tiles[objectTexture->tile & 0x3FFF].indices + 256 * y0 + x0;
            textures3DO[i].w = w;
            textures3DO[i].h = h;

            { // check if the texture is already converted
                int32 index = -1;

                if (objectTextures[i].isQuad)
                {
                    for (int32 j = 0; j < i; j++)
                    {
                        if (objectTextures[j].isQuad && textures3DO[j].src == textures3DO[i].src)
                        {
                            // TODO can we reuse textures with the same src and width but smaller height?
                            if ((textures3DO[j].w == textures3DO[i].w) && (textures3DO[j].h == textures3DO[i].h))
                            {
                                index = j;
                                break;
                            }
                        }
                    }
                }

                if (index != -1)
                {
                    uint8 flip = textures3DO[i].flip;
                    textures3DO[i] = textures3DO[index];
                    textures3DO[i].flip = flip; // flip flags may differ
                    continue; // skip texture conversion
                }
            }


            { // copy tile to 32-bit image and calculate average tile color
                uint8* src = textures3DO[i].src;
                uint32* dst = bitmap32;

                uint32 avgR = 0;
                uint32 avgG = 0;
                uint32 avgB = 0;

                for (int32 y = 0; y < h; y++)
                {
                    for (int32 x = 0; x < w; x++)
                    {
                        if (!objectTexture->isQuad)
                        {
                            float u = float(x) / float(w - 1);
                            float v = float(y) / float(h - 1);

                            float px0 = objectTexture->x0;
                            float py0 = objectTexture->y0;
                            float px1 = objectTexture->x1;
                            float py1 = objectTexture->y1;
                            float px2 = objectTexture->x2;
                            float py2 = objectTexture->y2;
                            float px3 = objectTexture->x2;
                            float py3 = objectTexture->y2;

                            float px = (1.0f - u) * (1.0f - v) * px0 + u * (1.0f - v) * px1 + (1 - u) * v * px2 + u * v * px3;
                            float py = (1.0f - u) * (1.0f - v) * py0 + u * (1.0f - v) * py1 + (1 - u) * v * py2 + u * v * py3;

                            int32 ix = int32(px + 0.5) - x0;
                            int32 iy = int32(py + 0.5) - y0;

                            ASSERT(!(ix < 0 || ix >= w || iy < 0 || iy >= h));

                            src = textures3DO[i].src + iy * 256 + ix;
                        }

                        uint32 p = pal32[*src++];
                        *dst++ = p;

                        uint32 A = p >> 24;
                        if (A)
                        {
                            avgR += (p >> 16) & 0xFF;
                            avgG += (p >> 8) & 0xFF;
                            avgB += (p) & 0xFF;
                        }
                    }
                    src += 256 - w;
                }

                avgR /= w * h;
                avgG /= w * h;
                avgB /= w * h;

                textures3DO[i].color = (avgB >> 3) | ((avgG >> 3) << 5) | ((avgR >> 3) << 10);
            }

            { // resize to POT
                int32 wp = nextPow2(w);
                int32 hp = nextPow2(h);

                if (wp != w) {
                    wp /= 2;
                }

                if (hp != h) {
                    hp /= 2;
                }

                if (wp > 64) {
                    wp = 64;
                }

                if (hp > 64) {
                    hp = 64;
                }

                ASSERT(wp != 0 && hp != 0);

                if (w != wp || h != hp)
                {
                    stbir_resize_uint8_generic((uint8*)bitmap32, w, h, 0, (uint8*)bitmap32_tmp, wp, hp, 0, 4, 3, 0,
                                                STBIR_EDGE_CLAMP, STBIR_FILTER_BOX, STBIR_COLORSPACE_LINEAR, NULL);
                    swap(bitmap32, bitmap32_tmp);

                    w = wp;
                    h = hp;
                }
            }

            /*{
                char buf[128];
                sprintf(buf, "tex%d.bmp", i);
                saveBitmap(buf, (uint8*)bitmap32, w, h, 32);
            }*/

            int32 rowBytes = (((w * 4) + 31) >> 5) << 2;
            if (rowBytes < 8) {
                rowBytes = 8;
            }
            int32 rowWOFFSET = (rowBytes >> 2) - PRE1_WOFFSET_PREFETCH;

            textures3DO[i].pre0 = ((h - PRE0_VCNT_PREFETCH) << PRE0_VCNT_SHIFT) | PRE0_BPP_4;
            textures3DO[i].pre1 = ((w - PRE1_TLHPCNT_PREFETCH) << PRE1_TLHPCNT_SHIFT) | PRE1_TLLSB_PDC0 | (rowWOFFSET << PRE1_WOFFSET8_SHIFT);
            textures3DO[i].wShift = 20 - shiftPow2(w);
            textures3DO[i].hShift = 16 - shiftPow2(h);

            if (!(objectTexture->attribute & TEX_ATTR_AKILL)) {
                textures3DO[i].pre0 |= PRE0_BGND;
            }

            { // quantize to 16 colors
                liq_attr *attr = liq_attr_create();
                liq_image *image = liq_image_create_rgba(attr, bitmap32, w, h, 0);
                liq_set_max_colors(attr, 16);

                liq_result *res;
                liq_image_quantize(image, attr, &res);

                liq_write_remapped_image(res, image, bitmap8, 256 * 256);
                const liq_palette *pal8 = liq_get_palette(res);

                PLUT plut;

                memset(&plut, 0, sizeof(plut));
                for(int32 j = 0; j < pal8->count; j++)
                {
                    liq_color c = pal8->entries[j];
                    if (c.a < 128) {
                        plut.colors[j] = 0;
                    } else {
                        plut.colors[j] = (c.r >> 3) | ((c.g >> 3) << 5) | ((c.b >> 3) << 10);
                    }
                }

                textures3DO[i].plut = addPalette(plut);

                liq_result_destroy(res);
                liq_image_destroy(image);
                liq_attr_destroy(attr);
            }

            if (rowBytes * 2 != w) // adjust row pitch
            {
                uint8* src = bitmap8;
                uint8* dst = bitmap8_tmp;
                memset(dst, 0, (rowBytes * 2) * h);

                for (int32 y = 0; y < h; y++) {
                    memcpy(dst, src, w);
                    dst += rowBytes * 2;
                    src += w;
                }

                swap(bitmap8, bitmap8_tmp);
            }

            { // encode to 4-bit image
                textures3DO[i].image = new uint8[rowBytes * h];

                uint8* src = bitmap8;
                uint8* dst = textures3DO[i].image;
                for (int32 y = 0; y < h; y++)
                {
                    for (int32 x = 0; x < rowBytes; x++, src += 2)
                    {
                        *dst++ = (src[0] << 4) | src[1];
                    }
                }

                textures3DO[i].data = 0;
/*
                for (int32 j = 0; j < i; j++)
                {
                    if (textures3DO[i].cmp(textures3DO[j]))
                    {
                        textures3DO[i].data = textures3DO[j].data;

                        //ASSERT((objectTextures[i].attribute & TEX_ATTR_MIPS) == (objectTextures[j].attribute & TEX_ATTR_MIPS));

                        dupSize += rowBytes * h;
                        break;
                    }
                }
                */
            // write image
                if (!textures3DO[i].data) {
                    textures3DO[i].data = f.getPos();

                    f.write(textures3DO[i].pre0);
                    f.write(textures3DO[i].pre1);
                    f.write(textures3DO[i].image, rowBytes * h);
                }
            }

        // generate mip level
            if (!(objectTexture->attribute & TEX_ATTR_MIPS)) {
                textures3DO[i].mip = -1;
                continue;
            }

            textures3DO[i].mip = mipIndex;

            Texture3DO* mip = &textures3DO[mipIndex++];
            *mip = textures3DO[i];

            w >>= 1;
            h >>= 1;
            ASSERT(w > 0);
            ASSERT(h > 0);

            {
                stbir_resize_uint8_generic((uint8*)bitmap32, w << 1, h << 1, 0, (uint8*)bitmap32_tmp, w, h, 0, 4, 3, 0,
                                            STBIR_EDGE_CLAMP, STBIR_FILTER_BOX, STBIR_COLORSPACE_LINEAR, NULL);
                swap(bitmap32, bitmap32_tmp);
            }

            rowBytes = (((w * 4) + 31) >> 5) << 2;
            if (rowBytes < 8) {
                rowBytes = 8;
            }
            rowWOFFSET = (rowBytes >> 2) - PRE1_WOFFSET_PREFETCH;

            mip->pre0 = ((h - PRE0_VCNT_PREFETCH) << PRE0_VCNT_SHIFT) | PRE0_BPP_4;
            mip->pre1 = ((w - PRE1_TLHPCNT_PREFETCH) << PRE1_TLHPCNT_SHIFT) | PRE1_TLLSB_PDC0 | (rowWOFFSET << PRE1_WOFFSET8_SHIFT);
            mip->wShift = 20 - shiftPow2(w);
            mip->hShift = 16 - shiftPow2(h);

            if (!(objectTexture->attribute & TEX_ATTR_AKILL)) {
                mip->pre0 |= PRE0_BGND;
            }

            { // quantize to 16 colors
                liq_attr *attr = liq_attr_create();
                liq_image *image = liq_image_create_rgba(attr, bitmap32, w, h, 0);
                liq_set_max_colors(attr, 16);

                liq_result *res;
                liq_image_quantize(image, attr, &res);

                liq_write_remapped_image(res, image, bitmap8, 256 * 256);
                const liq_palette *pal8 = liq_get_palette(res);

                PLUT plut;

                memset(&plut, 0, sizeof(plut));
                for(int32 j = 0; j < pal8->count; j++)
                {
                    liq_color c = pal8->entries[j];
                    if (c.a < 128) {
                        plut.colors[j] = 0;
                    } else {
                        plut.colors[j] = (c.r >> 3) | ((c.g >> 3) << 5) | ((c.b >> 3) << 10);
                    }
                }

                mip->plut = addPalette(plut);

                liq_result_destroy(res);
                liq_image_destroy(image);
                liq_attr_destroy(attr);
            }

            if (rowBytes * 2 != w) // adjust row pitch
            {
                uint8* src = bitmap8;
                uint8* dst = bitmap8_tmp;
                memset(dst, 0, (rowBytes * 2) * h);

                for (int32 y = 0; y < h; y++) {
                    memcpy(dst, src, w);
                    dst += rowBytes * 2;
                    src += w;
                }

                swap(bitmap8, bitmap8_tmp);
            }

            { // encode to 4-bit image
                uint8* src = bitmap8;
                uint8* dst = bitmap8_tmp;
                for (int32 y = 0; y < h; y++)
                {
                    for (int32 x = 0; x < rowBytes; x++, src += 2)
                    {
                        *dst++ = (src[0] << 4) | src[1];
                    }
                }

            // write image
                mip->data = f.getPos();
                f.write(mip->pre0);
                f.write(mip->pre1);
                f.write(bitmap8_tmp, rowBytes * h);
            }
        }

        objectTexturesCount = mipIndex;

        printf("duplicate size: %d\n", dupSize);

        uint32 paletteOffset = f.align4();

    // write PLUTs
        f.write((uint16*)PLUTs, sizeof(PLUT) / 2 * plutsCount);

    // calculate underwater PLUTs (blue tint = (0.5, 0.8, 0.8))
        {
            uint16* src = PLUTs[0].colors;
            for (int32 i = 0; i < plutsCount * 16; i++)
            {
                uint16 p = *src;

                uint32 b = (p & 31) << 3;
                uint32 g = ((p >> 5) & 31) << 3;
                uint32 r = ((p >> 10) & 31) << 3;

                r = int32(r * 0.5f);
                g = int32(g * 0.8f);
                b = int32(b * 0.8f);

                *src++ = (b >> 3) | ((g >> 3) << 5) | ((r >> 3) << 10);
            }
        }
        f.write((uint16*)PLUTs, sizeof(PLUT) / 2 * plutsCount);

        int32 texFileSize = f.getPos();

    // write palette offset at the file start
        f.setPos(0);
        f.write(paletteOffset);

        delete[] bitmap32;
        delete[] bitmap32_tmp;
        delete[] bitmap8;
        delete[] bitmap8_tmp;

        return texFileSize;
    }

    void getSample(const char* base, const char* prefix, int32 id, int32 sub, uint8* buffer, int32 &size)
    {
        // 57 == 38?
        size = 0;

        char path[256];
        sprintf(path, "%s\\%s%03d_%d.wav", base, prefix, id, sub);

        FILE* f = fopen(path, "rb");

        if (!f)
        {
            if (prefix[0] == '_') // try to open file without the prefix
            {
                sprintf(path, "%s\\%s%03d_%d.wav", base, "", id, sub);
                f = fopen(path, "rb");
                if (!f)
                {
                    printf("%s not found!\n", path);
                    return;
                }
            } else {
                printf("%s not found!\n", path);
                return;
            }
        }

        fseek(f, 12, SEEK_SET); // skip RIFF header

        struct {
            unsigned int id;
            unsigned int size;
        } chunk;

        while (1)
        {
            fread(&chunk, sizeof(chunk), 1, f);
            if (chunk.id == 0x61746164) // data
            {
                int numSamples = chunk.size / (1 * sizeof(short));
                size = numSamples / 2; // 4 bits per sample

                short* data = new short[chunk.size / sizeof(short)];

                fread(data, 1, chunk.size, f);

                BlockADDVIEncode(buffer, data, numSamples, 1); // mono block

                delete[] data;

                break;
            } else {
                fseek(f, chunk.size, SEEK_CUR);
            }
        }

        fclose(f);
    }

    bool getSoundID(int32 index, int32 &id, int32 &sub)
    {
        for (int32 i = 0; i < 256; i++)
        {
            SoundInfo &s = soundInfo[soundMap[i]];
            if (s.index <= index && s.index + s.flags.count > index)
            {
                id = i;
                sub = index - s.index;
                return true;
            }        
        }

        return false;
    }

    void convert3DO(const char* name)
    {
        char path[256];
        sprintf(path, "../../3do/CD/data/%s.V", name);
        int32 texFileSize = convertTextures3DO(path);

        sprintf(path, "../../3do/CD/data/%s.D", name);

        FileStream f(path, true);

        if (!f.isValid()) return;

        f.bigEndian = true;

        Header header;
        f.seek(sizeof(Header)); // will be rewritten at the end

        header.magic = 0x33444F20;
        header.tilesCount = plutsCount;
        header.roomsCount = roomsCount;
        header.modelsCount = modelsCount;
        header.meshesCount = meshOffsetsCount;
        header.staticMeshesCount = staticMeshesCount;
        header.spriteSequencesCount = spriteSequencesCount;
        header.soundSourcesCount = soundSourcesCount;
        header.boxesCount = boxesCount;
        header.texturesCount = objectTexturesCount;
        header.itemsCount = itemsCount;
        header.camerasCount = camerasCount;
        header.cameraFramesCount = cameraFramesCount;
        header.soundOffsetsCount = soundOffsetsCount;
        header._reserved = 0;

        header.palette = 0;
        header.lightmap = 0;
        
        fixHeadMask();

        header.rooms = f.align4();
        {
            f.seek(sizeof(Room::InfoComp) * roomsCount);

            Room::InfoComp infoComp[255];

            for (int32 i = 0; i < roomsCount; i++)
            {
                const LevelPC::Room* room = rooms + i;

                Room::InfoComp &info = infoComp[i];

                ASSERT(room->info.x % 256 == 0);
                ASSERT(room->info.z % 256 == 0);
                ASSERT(room->info.yBottom >= -32768 && room->info.yBottom <= 32767);
                ASSERT(room->info.yTop >= -32768 && room->info.yTop <= 32767);
                info.x = room->info.x / 256;
                info.z = room->info.z / 256;
                info.yBottom = -32768;
                info.yTop = 32767;

                for (int32 j = 0; j < room->vCount; j++)
                {
                    Room::Vertex &v = room->vertices[j];
                    if (v.pos.y < info.yTop) {
                        info.yTop = v.pos.y;
                    }
                    if (v.pos.y > info.yBottom) {
                        info.yBottom = v.pos.y;
                    }
                }

                info.spritesCount = room->sCount;
                info.quadsCount = room->qCount;
                info.trianglesCount = room->tCount;
                info.portalsCount = uint8(room->pCount);
                info.lightsCount = uint8(room->lCount);
                info.meshesCount = uint8(room->mCount);
                info.ambient = room->ambient >> 5;
                info.xSectors = uint8(room->xSectors);
                info.zSectors = uint8(room->zSectors);
                info.alternateRoom = uint8(room->alternateRoom);

                info.flags = 0;
                if (room->flags & 1) info.flags |= 1;
                if (room->flags & 256) info.flags |= 2;

                ASSERT((room->flags & ~257) == 0);
                ASSERT(info.portalsCount == room->pCount);
                ASSERT(info.lightsCount == room->lCount);
                ASSERT(info.meshesCount == room->mCount);
                ASSERT(info.xSectors == room->xSectors);
                ASSERT(info.zSectors == room->zSectors);

                roomVerticesCount = 0;

                info.quads = f.align4();
                for (int32 i = 0; i < room->qCount; i++)
                {
                    Quad q = room->quads[i];

                // get intensity
                    const Room::Vertex &v0 = room->vertices[q.indices[0]];
                    const Room::Vertex &v1 = room->vertices[q.indices[1]];
                    const Room::Vertex &v2 = room->vertices[q.indices[2]];
                    const Room::Vertex &v3 = room->vertices[q.indices[3]];

                    uint32 intensity = ((v0.lighting + v1.lighting + v2.lighting + v3.lighting) / 4) >> 5;
                    ASSERT(intensity <= 255);
                    
                    q.indices[0] = addRoomVertex(info.yTop, v0, true);
                    q.indices[1] = addRoomVertex(info.yTop, v1, true);
                    q.indices[2] = addRoomVertex(info.yTop, v2, true);
                    q.indices[3] = addRoomVertex(info.yTop, v3, true);

                    ASSERT((int32)q.indices[0] * 12 < 0xFFFF);
                    ASSERT((int32)q.indices[1] * 12 < 0xFFFF);
                    ASSERT((int32)q.indices[2] * 12 < 0xFFFF);
                    ASSERT((int32)q.indices[3] * 12 < 0xFFFF);

                    RoomQuad3DO comp;
                    comp.indices[0] = q.indices[0] * 12;
                    comp.indices[1] = q.indices[1] * 12;
                    comp.indices[2] = q.indices[2] * 12;
                    comp.indices[3] = q.indices[3] * 12;
                    comp.flags = q.flags;
                // add ccw flag and swap indices
                    calcQuadFlip(comp);
                    ASSERT((comp.flags & FACE_CCW) == 0);
                // add intensity
                    comp.flags |= (intensity << (FACE_MIP_SHIFT + FACE_MIP_SHIFT));
                    if (textures3DO[comp.flags & FACE_TEXTURE].pre0 & PRE0_BGND) {
                        comp.flags |= FACE_OPAQUE; // set opaque flag
                    }
                // add mip level
                    Texture3DO* tex = textures3DO + (comp.flags & FACE_TEXTURE);
                    if (tex->mip != -1) {
                        comp.flags |= (tex->mip << FACE_MIP_SHIFT);
                    }

                    comp.write(f);
                }

                info.triangles = f.align4();
                for (int32 i = 0; i < room->tCount; i++)
                {
                    Triangle t = room->triangles[i];

                // get intensity
                    const Room::Vertex &v0 = room->vertices[t.indices[0]];
                    const Room::Vertex &v1 = room->vertices[t.indices[1]];
                    const Room::Vertex &v2 = room->vertices[t.indices[2]];

                    uint32 intensity = ((v0.lighting + v1.lighting + v2.lighting) / 3) >> 5;
                    ASSERT(intensity <= 255);
                    
                    t.indices[0] = addRoomVertex(info.yTop, v0, true);
                    t.indices[1] = addRoomVertex(info.yTop, v1, true);
                    t.indices[2] = addRoomVertex(info.yTop, v2, true);

                    ASSERT((int32)t.indices[0] * 12 < 0xFFFF);
                    ASSERT((int32)t.indices[1] * 12 < 0xFFFF);
                    ASSERT((int32)t.indices[2] * 12 < 0xFFFF);

                    RoomTriangle3DO comp;
                    comp.indices[0] = t.indices[0] * 12;
                    comp.indices[1] = t.indices[1] * 12;
                    comp.indices[2] = t.indices[2] * 12;
                    comp._unused = 0;
                    comp.flags = t.flags;
                // add intensity
                    comp.flags |= (intensity << (FACE_MIP_SHIFT + FACE_MIP_SHIFT));
                    if (textures3DO[comp.flags & FACE_TEXTURE].pre0 & PRE0_BGND) {
                        comp.flags |= FACE_OPAQUE; // set opaque flag
                    }
                // add mip level
                    Texture3DO* tex = textures3DO + (comp.flags & FACE_TEXTURE);
                    if (tex->mip != -1) {
                        comp.flags |= (tex->mip << FACE_MIP_SHIFT);
                    }
                    comp.write(f);
                }

                info.vertices = f.align4();
                info.verticesCount = roomVerticesCount;
                for (int32 i = 0; i < roomVerticesCount; i += 4)
                {
                    Room::VertexComp v[4];

                    for (int32 j = 0; j < 4; j++)
                    {
                        if (i + j < roomVerticesCount) {
                            v[j] = roomVertices[i + j];
                        } else {
                            memset(&v[j], 0, sizeof(v[j]));
                        }
                    }

                    {
                        uint32 value = v[0].x | (v[0].y << 5) | (v[0].z << 11);
                        value |= (v[1].x | (v[1].y << 5) | (v[1].z << 11)) << 16;
                        f.write(value);
                    }

                    {
                        uint32 value = v[2].x | (v[2].y << 5) | (v[2].z << 11);
                        value |= (v[3].x | (v[3].y << 5) | (v[3].z << 11)) << 16;
                        f.write(value);
                    }
                }

                info.sprites = f.align4();
                for (int32 i = 0; i < room->sCount; i++)
                {
                    const Room::Sprite* sprite = room->sprites + i;
                    const Room::Vertex* v = room->vertices + sprite->index;

                    Room::SpriteComp comp;
                    comp.x = v->pos.x;
                    comp.y = v->pos.y;
                    comp.z = v->pos.z;
                    comp.g = v->lighting >> 5;
                    comp.index = uint8(sprite->texture);

                    ASSERT(sprite->texture <= 255);

                    comp.write(f);
                }

                info.portals = f.align4();
                f.writeObj(room->portals, room->pCount);
                /*
                for (int32 i = 0; i < room->pCount; i++)
                {
                    const Room::Portal* portal = room->portals + i;

                    Room::PortalComp comp;
                    
                    comp.roomIndex = portal->roomIndex;

                    static const struct {
                        int32 x, y, z;
                        int32 mask;
                    } normals[9] = {
                        { -1,  0,  0,  2 << 0 },
                        {  1,  0,  0,  1 << 0 },
                        {  0, -1,  0,  2 << 2 },
                        {  0,  1,  0,  1 << 2 },
                        {  0,  0, -1,  2 << 4 },
                        {  0,  0,  1,  1 << 4 }
                    };

                    comp.normalMask = 255;
                    for (int32 i = 0; i < 9; i++)
                    {
                        if (portal->normal.x == normals[i].x &&
                            portal->normal.y == normals[i].y &&
                            portal->normal.z == normals[i].z)
                        {
                            comp.normalMask = normals[i].mask;
                            break;
                        }
                    }

                    ASSERT(comp.normalMask != 255);

                    for (int32 i = 0; i < 4; i++)
                    {
                        comp.vertices[i].x = portal->vertices[i].x;
                        comp.vertices[i].y = portal->vertices[i].y;
                        comp.vertices[i].z = portal->vertices[i].z;
                    }

                    comp.write(f);
                }*/

                info.sectors = f.align4();
                f.writeObj(room->sectors, room->zSectors * room->xSectors);
            
                info.lights = f.align4();
                for (int32 i = 0; i < room->lCount; i++)
                {
                    const Room::Light* light = room->lights + i;

                    Room::LightComp comp;
                    comp.pos.x = light->pos.x - room->info.x;
                    comp.pos.y = light->pos.y;
                    comp.pos.z = light->pos.z - room->info.z;
                    comp.radius = light->radius >> 8;
                    comp.intensity = light->intensity >> 5;

                    comp.write(f);
                }

                info.meshes = f.align4();
                for (int32 i = 0; i < room->mCount; i++)
                {
                    const Room::Mesh* mesh = room->meshes + i;

                    Room::MeshComp comp;
                    comp.pos.x = mesh->pos.x - room->info.x;
                    comp.pos.y = mesh->pos.y;
                    comp.pos.z = mesh->pos.z - room->info.z;
                    comp.intensity = mesh->intensity >> 5;
                    comp.flags = ((mesh->angleY / 0x4000 + 2) << 6) | mesh->id;

                    ASSERT(mesh->id <= 63);
                    ASSERT(mesh->angleY % 0x4000 == 0);
                    ASSERT(mesh->angleY / 0x4000 + 2 >= 0);

                    comp.write(f);
                }
            }

            int32 pos = f.getPos();
            f.setPos(header.rooms);
            f.writeObj(infoComp, roomsCount);
            f.setPos(pos);
        }

        header.floors = f.align4();
        f.writeObj(floors, floorsCount);

        header.meshData = f.align4();

        int32 mOffsets[2048];
        for (int32 i = 0; i < 2048; i++) {
            mOffsets[i] = -1;
        }

        for (int32 i = 0; i < meshOffsetsCount; i++)
        {
            if (mOffsets[i] != -1)
                continue;

            mOffsets[i] = f.align4() - header.meshData;

            const uint8* ptr = (uint8*)meshData + meshOffsets[i];

            vec3s center = *(vec3s*)ptr; ptr += sizeof(center);
            int16 radius = *(int16*)ptr; ptr += sizeof(radius);
            uint16 flags = *(uint16*)ptr; ptr += sizeof(flags);

            int16 vCount = *(int16*)ptr; ptr += 2;
            const vec3s* vertices = (vec3s*)ptr;
            ptr += vCount * sizeof(vec3s);

            const uint16* vIntensity = NULL;
            const vec3s* vNormal = NULL;

            int16 nCount = *(int16*)ptr; ptr += 2;
            //const int16* normals = (int16*)ptr;
            if (nCount > 0) { // normals
                vNormal = (vec3s*)ptr;
                ptr += nCount * 3 * sizeof(int16);
            } else { // intensity
                vIntensity = (uint16*)ptr;
                ptr += vCount * sizeof(uint16);
            }

            int16     rCount = *(int16*)ptr; ptr += 2;
            Quad*     rFaces = (Quad*)ptr; ptr += rCount * sizeof(Quad);

            int16     tCount = *(int16*)ptr; ptr += 2;
            Triangle* tFaces = (Triangle*)ptr; ptr += tCount * sizeof(Triangle);

            int16     crCount = *(int16*)ptr; ptr += 2;
            Quad*     crFaces = (Quad*)ptr; ptr += crCount * sizeof(Quad);

            int16     ctCount = *(int16*)ptr; ptr += 2;
            Triangle* ctFaces = (Triangle*)ptr; ptr += ctCount * sizeof(Triangle);

            uint16 intensity = 0;

            if (vIntensity)
            {
                uint32 sum = 0;
                for (int32 i = 0; i < vCount; i++)
                {
                    sum += vIntensity[i];
                }
                intensity = sum / vCount;
            }

            f.write(center.x);
            f.write(center.y);
            f.write(center.z);
            f.write(radius);
            f.write(intensity);
            f.write(vCount);
            f.write(rCount);
            f.write(tCount);
            f.write(crCount);
            f.write(ctCount);

            for (int32 j = 0; j < vCount; j++)
            {
                struct MeshVertex3DO {
                    int16 x, y, z;
                } v;

                v.x = vertices[j].x << 2;
                v.y = vertices[j].y << 2;
                v.z = vertices[j].z << 2;

                f.write(v.x);
                f.write(v.y);
                f.write(v.z);
            }

            if (vCount % 2) { // add one vertex for the data alignment
                int16 zero = 0;
                f.write(zero);
                f.write(zero);
                f.write(zero);
            }

            for (int32 j = 0; j < rCount; j++)
            {
                Quad q = rFaces[j];

                ASSERT(q.indices[0] < 256);
                ASSERT(q.indices[1] < 256);
                ASSERT(q.indices[2] < 256);
                ASSERT(q.indices[3] < 256);

                MeshQuad3DO comp;
                comp.indices[0] = q.indices[0];
                comp.indices[1] = q.indices[1];
                comp.indices[2] = q.indices[2];
                comp.indices[3] = q.indices[3];
                comp.flags = q.flags;
                if (textures3DO[comp.flags & FACE_TEXTURE].pre0 & PRE0_BGND) {
                    comp.flags |= FACE_OPAQUE; // set opaque flag
                }
                calcQuadFlip(comp);
                comp.write(f);
            }

            for (int32 j = 0; j < tCount; j++)
            {
                Triangle t = tFaces[j];

                ASSERT(t.indices[0] < 256);
                ASSERT(t.indices[1] < 256);
                ASSERT(t.indices[2] < 256);

                MeshTriangle3DO comp;
                comp.indices[0] = t.indices[0];
                comp.indices[1] = t.indices[1];
                comp.indices[2] = t.indices[2];
                comp._unused = 0;
                comp.flags = t.flags;
                if (textures3DO[comp.flags & FACE_TEXTURE].pre0 & PRE0_BGND) {
                    comp.flags |= FACE_OPAQUE; // set opaque flag
                }
                comp.write(f);
            }

            for (int32 j = 0; j < crCount; j++)
            {
                Quad q = crFaces[j];

                ASSERT(q.indices[0] < 256);
                ASSERT(q.indices[1] < 256);
                ASSERT(q.indices[2] < 256);
                ASSERT(q.indices[3] < 256);

                MeshQuad3DO comp;
                comp.indices[0] = q.indices[0];
                comp.indices[1] = q.indices[1];
                comp.indices[2] = q.indices[2];
                comp.indices[3] = q.indices[3];
                comp.flags = q.flags;
                comp.write(f);
            }

            for (int32 j = 0; j < ctCount; j++)
            {
                Triangle t = ctFaces[j];

                ASSERT(t.indices[0] < 256);
                ASSERT(t.indices[1] < 256);
                ASSERT(t.indices[2] < 256);

                MeshTriangle3DO comp;
                comp.indices[0] = t.indices[0];
                comp.indices[1] = t.indices[1];
                comp.indices[2] = t.indices[2];
                comp._unused = 0;
                comp.flags = t.flags;
                comp.write(f);
            }


            for (int32 j = i + 1; j < meshOffsetsCount; j++)
            {
                if (meshOffsets[i] == meshOffsets[j])
                {
                    mOffsets[j] = mOffsets[i];
                }
            }
        }

        header.meshOffsets = f.align4();
        f.write(mOffsets, meshOffsetsCount);

        header.anims = f.align4();
        f.writeObj(anims, animsCount);

        header.states = f.align4();
        for (int32 i = 0; i < statesCount; i++)
        {
            const LevelPC::AnimState* state = states + i;

            LevelPC::AnimStateComp comp;
            comp.state = uint8(state->state);
            comp.rangesCount = uint8(state->rangesCount);
            comp.rangesStart = state->rangesStart;

            comp.write(f);
        }

        header.ranges = f.align4();
        f.writeObj(ranges, rangesCount);

        header.commands = f.align4();
        f.write(commands, commandsCount);

        header.nodes = f.align4();
        for (int32 i = 0; i < nodesDataSize / 4; i++)
        {
            const Node* node = (Node*)(nodesData + i * 4);

            ASSERT(node->pos.x > -32768);
            ASSERT(node->pos.x <  32767);
            ASSERT(node->pos.y > -32768);
            ASSERT(node->pos.y <  32767);
            ASSERT(node->pos.z > -32768);
            ASSERT(node->pos.z <  32767);
            ASSERT(node->flags < 0xFFFF);

            LevelPC::NodeComp comp;
            comp.flags = uint16(node->flags);
            comp.pos.x = int16(node->pos.x);
            comp.pos.y = int16(node->pos.y);
            comp.pos.z = int16(node->pos.z);

            comp.write(f);
        }
        //f.write(nodesData, nodesDataSize);

        header.frameData = f.align4();
        f.write(frameData, frameDataSize);

        header.models = f.align4();
        for (int32 i = 0; i < modelsCount; i++)
        {
            const LevelPC::Model* model = models + i;

            LevelPC::ModelComp comp;
            comp.type = uint8(model->type);
            comp.count = uint8(model->count);
            comp.start = model->start;
            comp.nodeIndex = model->nodeIndex / 4;
            comp.animIndex = model->animIndex;

            comp.write(f);
        }

        header.staticMeshes = f.align4();
        for (int32 i = 0; i < staticMeshesCount; i++)
        {
            const LevelPC::StaticMesh* staticMesh = staticMeshes + i;

            LevelPC::StaticMeshComp comp;
            comp.id = staticMesh->id;
            comp.meshIndex = staticMesh->meshIndex;
            comp.flags = staticMesh->flags;
            comp.vbox = staticMesh->vbox;
            comp.cbox = staticMesh->cbox;

            comp.write(f);
        }

        header.objectTextures = f.align4();
        for (int32 i = 0; i < objectTexturesCount; i++)
        {
            textures3DO[i].write(f);
        }

        header.spriteTextures = f.align4();
        for (int32 i = 0; i < spriteTexturesCount; i++)
        {
            const LevelPC::SpriteTexture* spriteTexture = spriteTextures + i;

            SpriteTexture3DO comp;
            comp.texture = spritesBaseIndex + i;
            comp.l = spriteTexture->l;
            comp.t = spriteTexture->t;
            comp.r = spriteTexture->r;
            comp.b = spriteTexture->b;

            comp.write(f);
        }

        f.writeObj(spriteTextures, spriteTexturesCount);

        header.spriteSequences = f.align4();
        f.writeObj(spriteSequences, spriteSequencesCount);

        header.cameras = f.align4();
        f.writeObj(cameras, camerasCount);

        header.soundSources = f.align4();
        f.writeObj(soundSources, soundSourcesCount);

        header.boxes = f.align4();
        for (int32 i = 0; i < boxesCount; i++)
        {
            const LevelPC::Box* box = boxes + i;

            BoxComp comp;
            comp.minX = box->minX >> 10;
            comp.minZ = box->minZ >> 10;
            comp.maxX = (box->maxX + 1) >> 10;
            comp.maxZ = (box->maxZ + 1) >> 10;
            comp.floor = box->floor;
            comp.overlap = box->overlap;

            comp.write(f);
        }

        header.overlaps = f.align4();
        f.write(overlaps, overlapsCount);

        for (int32 i = 0; i < 2; i++)
        {
            header.zones[i][0] = f.align4();
            f.write(zones[i].ground1, boxesCount);

            header.zones[i][1] = f.align4();
            f.write(zones[i].ground2, boxesCount);

            header.zones[i][2] = f.align4();
            f.write(zones[i].fly, boxesCount);
        }

        header.animTexData = f.align4();
        {
            int32 lastPos = f.getPos();

            uint16 rangesCount = *animTexData++;
            struct TexAnimRange
            {
                uint16 count;
                uint16 indices[256];
            } ranges[64];
            ASSERT(rangesCount <= 64);

            int32 newRangesCount = rangesCount;

            for (int32 i = 0; i < rangesCount; i++)
            {
                bool mips = true;

                TexAnimRange &range = ranges[i];
                range.count = *animTexData++;
                for (int32 j = 0; j <= range.count; j++)
                {
                    range.indices[j] = *animTexData++;

                    if (textures3DO[range.indices[j]].mip < 0) {
                        mips = false;
                    }
                }

            // add the new anim range for mip textures
                if (mips)
                {
                    TexAnimRange &mipRange = ranges[newRangesCount++];
                    mipRange.count = range.count;
                    for (int32 j = 0; j <= range.count; j++)
                    {
                        mipRange.indices[j] = textures3DO[range.indices[j]].mip;
                    }
                }
            }
            rangesCount = newRangesCount;

            f.write(rangesCount);
            for (int32 i = 0; i < rangesCount; i++)
            {
                f.write(ranges[i].count);
                f.write(ranges[i].indices, ranges[i].count + 1);
            }

            lastPos = f.getPos() - lastPos;
        }

        header.items = f.align4();
        for (int32 i = 0; i < itemsCount; i++)
        {
            const LevelPC::Item* item = items + i;
            const LevelPC::Room* room = rooms + item->roomIndex;

            ItemComp comp;
            comp.type = uint8(item->type);
            comp.roomIndex = uint8(item->roomIndex);
            comp.pos.x = int16(item->pos.x - room->info.x);
            comp.pos.y = int16(item->pos.y);
            comp.pos.z = int16(item->pos.z - room->info.z);
            comp.intensity = item->intensity < 0 ? 0 : (item->intensity >> 5);
            comp.flags = item->flags | ((item->angleY / 0x4000 + 2) << 14);

            ASSERT((item->flags & ~(0x3F1F)) == 0);

            comp.write(f);
        }

        header.cameraFrames = f.align4();
        f.writeObj(cameraFrames, cameraFramesCount);

        //f.writeArray(demoData, demoDataSize);

        header.soundMap = f.align4();
        f.write(soundMap, 256);

        header.soundInfos = f.align4();
        f.writeObj(soundInfo, soundInfoCount);

        header.soundData = f.align4();

        uint8* soundBuf = new uint8[2 * 1024 * 1024];

        bool isHome = strcmp(name, "GYM") == 0;

        for (int32 i = 0; i < soundOffsetsCount; i++)
        {
            soundOffsets[i] = f.align4() - header.soundData;

            int32 id, sub, size;
            if (getSoundID(i, id, sub))
            {
                getSample("C:\\Projects\\OpenLara\\src\\platform\\gba\\packer\\sounds\\conv_3do", isHome ? "_" : "", id, sub, soundBuf, size);
            } else {
                ASSERT(false);
            }

            int32 numSamples = size * 2;
            f.write(numSamples);

            if (size) {
                f.write(soundBuf, size);
            }
        }

        delete[] soundBuf;

        header.soundOffsets = f.align4();
        f.write(soundOffsets, soundOffsetsCount);

        f.setPos(0);
        header.write(f);
    }

};

#define COLOR_THRESHOLD_SQ (8 * 8)
#endif


#if 0
struct WAD
{


    struct LevelWAD
    {
        struct Room
        {
            struct Vertex
            {
                int8  x, y, z; 
                uint8 lighting;
            };

            struct Quad
            {
                uint16 flags;
                uint16 indices[4];
            };

            struct Triangle
            {
                uint16 flags;
                uint16 indices[3];
            };

            struct Sprite
            {
                int16  x, y, z;
                uint16 texture;
            };

            struct Portal
            {
                uint8 roomIndex;
                uint8 normalIndex;
                uint16 x, y, z;
                uint8 a, b;

                Portal() {}
                Portal(const LevelPC::Room::Portal &portal)
                {

                    const vec3s &v0 = portal.vertices[0];
                    const vec3s &v1 = portal.vertices[1];
                    const vec3s &v2 = portal.vertices[2];
                    const vec3s &v3 = portal.vertices[3];
                    const vec3s &n = portal.normal;

                    normalIndex = 0xFF;

                    if (n.x == -1) normalIndex = 0;
                    if (n.x ==  1) normalIndex = 1;
                    if (n.y == -1) normalIndex = 2;
                    if (n.y ==  1) normalIndex = 3;
                    if (n.z == -1) normalIndex = 4;
                    if (n.z ==  1) normalIndex = 5;

                    ASSERT(normalIndex != 0xFF);

                    int32 minX = MIN(MIN(MIN(v0.x, v1.x), v2.x), v3.x);
                    int32 minY = MIN(MIN(MIN(v0.y, v1.y), v2.y), v3.y);
                    int32 minZ = MIN(MIN(MIN(v0.z, v1.z), v2.z), v3.z);
                    int32 maxX = MAX(MAX(MAX(v0.x, v1.x), v2.x), v3.x);
                    int32 maxY = MAX(MAX(MAX(v0.y, v1.y), v2.y), v3.y);
                    int32 maxZ = MAX(MAX(MAX(v0.z, v1.z), v2.z), v3.z);

                    x = (maxX + minX) / 2;
                    y = (maxY + minY) / 2;
                    z = (maxZ + minZ) / 2;

                    int32 sx = (maxX - minX) / 256;
                    int32 sy = (maxY - minY) / 256;
                    int32 sz = (maxZ - minZ) / 256;

                    switch (normalIndex / 2)
                    {
                        case 0 : // x
                            a = sy;
                            b = sz;
                            break;
                        case 1 : // y
                            a = sx;
                            b = sz;
                            break;
                        case 2 : // z
                            a = sx;
                            b = sy;
                            break;
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
            };

            int16 vCount;
            int16 qCount;
            int16 tCount;
            uint8 sCount;
            uint8 pCount;
            uint8 zSectors;
            uint8 xSectors;

            Vertex vertices[MAX_ROOM_VERTICES];
            Quad quads[MAX_ROOM_QUADS];
            Triangle triangles[MAX_ROOM_TRIANGLES];
            Sprite sprites[MAX_ROOM_SPRITES];
            Portal portals[MAX_ROOM_PORTALS];
            Sector sectors[MAX_ROOM_SECTORS];

            Room() {}

            Room(const LevelPC::Room &room) : vCount(0), qCount(0), tCount(0), sCount(0)
            {
                for (int32 i = 0; i < room.qCount; i++)
                {
                    addQuad(room.quads[i], room.vertices);
                }

                for (int32 i = 0; i < room.tCount; i++)
                {
                    addTriangle(room.triangles[i], room.vertices);
                }

                for (int32 i = 0; i < room.sCount; i++)
                {
                    addSprite(room.sprites[i], room.vertices);
                }

                pCount = uint8(room.pCount);
                for (int32 i = 0; i < pCount; i++)
                {
                    portals[i] = room.portals[i];
                }

                zSectors = uint8(room.zSectors);
                xSectors = uint8(room.xSectors);
                for (int32 i = 0; i < zSectors * xSectors; i++)
                {
                    const LevelPC::Room::Sector &src = room.sectors[i];
                    Sector &dst = sectors[i];
                    dst.floorIndex = src.floorIndex;
                    dst.boxIndex   = src.boxIndex;
                    dst.roomBelow  = src.roomBelow;
                    dst.floor      = src.floor;
                    dst.roomAbove  = src.roomAbove;
                    dst.ceiling    = src.ceiling;
                }
            }

            int32 addVertex(const LevelPC::Room::Vertex &v)
            {
                Vertex n;
                n.x = v.pos.x / 1024;
                n.y = v.pos.y / 256;
                n.z = v.pos.z / 1024;
                n.lighting = v.lighting >> 5;

                for (int32 i = 0; i < vCount; i++)
                {
                    if (vertices[i].x == n.x &&
                        vertices[i].y == n.y &&
                        vertices[i].z == n.z &&
                        vertices[i].lighting == n.lighting)
                    {
                        return i;
                    }
                }

                vertices[vCount++] = n;

                return vCount - 1;
            }

            void addQuad(const LevelPC::Quad &q, const LevelPC::Room::Vertex* verts)
            {
                Quad n;
                n.flags = q.flags;
                n.indices[0] = addVertex(verts[q.indices[0]]);
                n.indices[1] = addVertex(verts[q.indices[1]]);
                n.indices[2] = addVertex(verts[q.indices[2]]);
                n.indices[3] = addVertex(verts[q.indices[3]]);
                quads[qCount++] = n;
            }

            void addTriangle(const LevelPC::Triangle &t, const LevelPC::Room::Vertex* verts)
            {
                Triangle n;
                n.flags = t.flags;
                n.indices[0] = addVertex(verts[t.indices[0]]);
                n.indices[1] = addVertex(verts[t.indices[1]]);
                n.indices[2] = addVertex(verts[t.indices[2]]);
                triangles[tCount++] = n;
            }

            void addSprite(const LevelPC::Room::Sprite &s, const LevelPC::Room::Vertex* verts)
            {
                Sprite n;
                n.texture = s.texture;
                n.x = verts[s.index].pos.x;
                n.y = verts[s.index].pos.y;
                n.z = verts[s.index].pos.z;
                // lighting?
                sprites[sCount++] = n;
            }
        };

        struct FloorData
        {
            uint16 value;
        };

        struct Box
        {
            int8 minZ, maxZ;
            int8 minX, maxX;
            int16 floor;
            int16 overlap;

            Box() {}

            Box(const LevelPC::Box &b)
            {
                minX = b.minX / 1024;
                minZ = b.minZ / 1024;
                maxX = (b.maxX + 1) / 1024;
                maxZ = (b.maxZ + 1) / 1024;

                floor = int16(b.floor);
                overlap = int16(b.overlap);

                ASSERT(b.minX == minX * 1024);
                ASSERT(b.minZ == minZ * 1024);
                ASSERT(b.maxX == maxX * 1024 - 1);
                ASSERT(b.maxZ == maxZ * 1024 - 1);
            }
        };

        struct Overlap
        {
            uint16 value;
        };

        struct Zone
        {
            uint16 ground1[MAX_BOXES];
            uint16 ground2[MAX_BOXES];
            uint16 fly[MAX_BOXES];
        };

        struct Item
        {
            uint8 type;
            uint8 roomIndex;
            vec3s pos;
            int8 angleY;
            uint8 intensity;
            uint16 flags;

            Item() {}
            
            Item(const LevelPC::Item &item)
            {
                type = uint8(item.type);
                roomIndex = uint8(item.roomIndex);
                pos.x = int16(item.pos.x);
                pos.y = int16(item.pos.y);
                pos.z = int16(item.pos.z);
                angleY = item.angleY / 0x4000;
                intensity = item.intensity >> 5;
                flags = item.flags;
            }
        };

        int16 roomsCount;
        int16 floorsCount;
        int16 boxesCount;
        int16 overlapsCount;
        int16 itemsCount;
         
        Room rooms[MAX_ROOMS];
        FloorData floors[MAX_FLOORS];
        Box boxes[MAX_BOXES];
        Overlap overlaps[MAX_OVERLAPS];
        Zone zones[2];
        Item items[MAX_ITEMS];

        LevelWAD() {}

        LevelWAD(const LevelPC &level)
        {
            roomsCount = int16(level.roomsCount);
            for (int32 i = 0; i < level.roomsCount; i++)
            {
                rooms[i] = level.rooms[i];
            }

            floorsCount = int16(level.floorsCount);
            memcpy(floors, level.floors, floorsCount * sizeof(floors[0]));

            boxesCount = int16(level.boxesCount);
            for (int32 i = 0; i < level.boxesCount; i++)
            {
                boxes[i] = level.boxes[i];
            }

            overlapsCount = int16(level.overlapsCount);
            memcpy(overlaps, level.overlaps, overlapsCount * sizeof(overlaps[0]));

            for (int32 i = 0; i < 2; i++)
            {
                memcpy(zones[i].ground1, level.zones[i].ground1, level.boxesCount * sizeof(uint16));
                memcpy(zones[i].ground2, level.zones[i].ground2, level.boxesCount * sizeof(uint16));
                memcpy(zones[i].fly, level.zones[i].fly, level.boxesCount * sizeof(uint16));
            }

            itemsCount = int16(level.itemsCount);
            for (int32 i = 0; i < itemsCount; i++)
            {
                LevelPC::Item item = level.items[i];
                const LevelPC::Room &room = level.rooms[item.roomIndex];

                item.pos.x -= room.info.x;
                item.pos.z -= room.info.z;

                items[i] = item;
            }
        }
    };



    struct Model
    {
        uint16 count;
        uint16 start;
        uint16 nodeIndex;
        uint16 frameIndex;
        uint16 animIndex;
    };


    Model* models[MAX_ITEMS];

    bool itemsUsed[MAX_ITEMS];

    LevelWAD* levels[MAX_LEVELS];

    WAD()
    {
        memset(models, 0, sizeof(models));
        memset(itemsUsed, 0, sizeof(itemsUsed));
    }



    int32 addNodes(const LevelPC::Node *nodes, int32 count)
    {
        if (count == 0)
            return 0;

        int32 index = 0;

        for (int32 i = 0; i < nodeLists.count; i++)
        {
            if (nodeLists[i]->count == count)
            {
                bool equal = true;

                for (int32 j = 0; j < nodeLists[i]->count; j++)
                {
                    const Node &n = nodeLists[i]->nodes[j];
                    if (n.flags != nodes[j].flags ||
                        n.pos.x != nodes[j].pos.x ||
                        n.pos.y != nodes[j].pos.y ||
                        n.pos.z != nodes[j].pos.z)
                    {
                        equal = false;
                        break;
                    }
                }

                if (equal) {
                    return index;
                }
            }

            index += nodeLists[i]->count;
        }

        NodeList* list = new NodeList();
        list->count = count;
        for (int32 i = 0; i < count; i++)
        {
            Node &n = list->nodes[i];
            n.flags = nodes[i].flags;
            n.pos.x = nodes[i].pos.x;
            n.pos.y = nodes[i].pos.y;
            n.pos.z = nodes[i].pos.z;
        }

        nodeLists.add(list);

        return index;
    }

    void packTiles()
    {
        textures.sort();

        for (int32 i = 0; i < textures.count; i++)
        {
            WAD::Texture* tex = textures[i];

            bool placed = false;

            for (int32 j = 0; j < tiles.count; j++)
            {
                if (tiles[j]->root->insert(tex)) {
                    placed = true;
                    break;
                }
            }

            if (!placed)
            {
                Tile24* tile = new Tile24();
                tiles.add(tile);
                if (!tile->root->insert(tex))
                {
                    printf("Can't pack texture %d x %d", tex->width, tex->height);
                    break;
                }
            }
        }

        uint8* data = new uint8[256 * 256 * 3 * tiles.count];

        for (int32 i = 0; i < tiles.count; i++)
        {
            tiles[i]->fill(data + i * 256 * 256 * 3);
        }
        saveBitmap("tiles.bmp", data, 256, 256 * tiles.count);

        delete[] data;
    }
};


void convertTracks3DO(const char* inDir, const char* outDir)
{
    WIN32_FIND_DATA fd;
    HANDLE h = FindFirstFile(inDir, &fd);

    if (h == INVALID_HANDLE_VALUE)
        return;

    char buf[256];

    do
    {
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            const char* src = fd.cFileName;
            char* dst = buf;

            while (*src)
            {
                if (*src >= '0' && *src <= '9')
                {
                    *dst++ = *src;
                }
                src++;
            }
            *dst++ = 0;

            int32 index = atoi(buf);

            if (index != 0)
            {
                strcpy(buf, inDir);
                buf[strlen(buf) - 1] = 0;
                strcat(buf, fd.cFileName);

                char cmdline[256];
                sprintf(cmdline, "C:\\Program Files\\ffmpeg\\ffmpeg.exe -y -i \"%s\" -ac 1 -ar 22050 -acodec pcm_s16be %s\\%d.aiff", buf, outDir, index);

                launchApp(cmdline);
                /* TODO SDXC encoder
                FILE* f = fopen("tmp.wav", "rb");
                ASSERT(f);

                fseek(f, 0, SEEK_END);
                int32 size = ftell(f);
                fseek(f, 0, SEEK_SET);
                uint8* samples = new uint8[size];
                fread(samples, 1, size, f);
                fclose(f);

                int32 numSamples = size / sizeof(short);

                int32 outputSize = (size + 3) / 4;
                uint8* output = new uint8[outputSize];
                memset(output, 0, outputSize);

                BlockADDVIEncode(output, (short*)samples, numSamples, 1); // mono block

                sprintf(buf, "%s\\%d.S", outDir, index);
                f = fopen(buf, "wb");
                ASSERT(f);
                fwrite(&numSamples, sizeof(numSamples), 1, f);
                fwrite(output, 1, outputSize, f);
                fclose(f);

                delete[] output;
                delete[] samples;
                */
            }
        }
    }
    while (FindNextFile(h, &fd));
    FindClose(h);
}



// 3DO face flags
// 1:ccw, 1:opaque, 8:intensity, 11:mipTexIndex, 11:texIndex
#define FACE_MIP_SHIFT  11
#define FACE_OPAQUE     (1 << 30)
#define FACE_CCW        (1 << 31)

#define TEX_FLIP_X      1
#define TEX_FLIP_Y      2
#define TEX_ATTR_MIPS   0x8000

void process3DO(const char* dir, TR1_PC* pc, TR1_PSX* psx)
{
}

// BIG TODO!
    //pack_tracks("tracks/conv_demo/*.ima"); return 0;


    /*
    uint32 palDump[32][256];
    memset(palDump, 0, sizeof(palDump));


    for (int32 i = 0; i < MAX_LEVELS; i++)
    {
        char path[64];
        sprintf(path, "levels/%s.PHD", levelNames[i]);
        levels[i] = new LevelPC(path);

        for (int32 j = 0; j < 256; j++)
        {
            int32 r = levels[i]->palette.colors[j * 3 + 0] << 2;
            int32 g = levels[i]->palette.colors[j * 3 + 1] << 2;
            int32 b = levels[i]->palette.colors[j * 3 + 2] << 2;
            palDump[i][j] = b | (g << 8) | (r << 16) | (0xFF << 24);
        }

        levels[i]->generateLODs();
        //levels[i]->cutData(levelNames[i]);

        sprintf(path, "../data/%s.PKD", levelNames[i]);
        levels[i]->convertGBA(path);

        //levels[i]->convert3DO(levelNames[i]);
    }*/

//    saveBitmap("pal.bmp", (uint8*)palDump, 256, 32, 32);

//    convertTracks3DO("C:\\Projects\\OpenLara\\src\\platform\\gba\\packer\\tracks\\orig\\*", "C:\\Projects\\OpenLara\\src\\platform\\3do\\tracks");


/*
    WAD* wad = new WAD();

    int32 size = 0;
    int32 maxItems = 0;

    int32 maxVertices = 0;

    for (int32 i = 0; i < MAX_LEVELS; i++)
    {
        LevelPC* level = levels[i];

        wad->addLevel(i, *level);

        for (int32 j = 0; j < level->roomsCount; j++)
        {
            WAD::LevelWAD::Room &room = wad->levels[i]->rooms[j];

            size += room.vCount * sizeof(WAD::LevelWAD::Room::Vertex);
            size += room.qCount * sizeof(WAD::LevelWAD::Room::Quad);
            size += room.tCount * sizeof(WAD::LevelWAD::Room::Triangle);
            size += room.sCount * sizeof(WAD::LevelWAD::Room::Sprite);
            size += room.pCount * sizeof(WAD::LevelWAD::Room::Portal);
            size += room.xSectors * room.zSectors * sizeof(room.sectors[0]);
        }

        size += wad->levels[i]->floorsCount * sizeof(WAD::LevelWAD::FloorData);
        size += wad->levels[i]->boxesCount * sizeof(WAD::LevelWAD::Box);
        size += wad->levels[i]->overlapsCount * sizeof(WAD::LevelWAD::Overlap);
        size += wad->levels[i]->boxesCount * sizeof(uint16) * 3 * 2; // zones
        size += wad->levels[i]->itemsCount * sizeof(WAD::LevelWAD::Item);

        size += level->frameDataSize * 2;
    }

    int32 nodes = 0;
    for (int32 i = 0; i < wad->nodeLists.count; i++)
    {
        nodes += wad->nodeLists[i]->count;
    }

    printf("roomsSize: %d bytes %d\n", size, nodes);

    wad->packTiles();

    printf("tiles: %d (%d bytes)\n", wad->tiles.count, wad->tiles.count * 256 * 256);

/*
    int32 texSize = 0;
    for (int32 i = 0; i < wad.textures.count; i++)
    {
        texSize += wad.textures[i]->width * wad.textures[i]->height;

        char texName[64];
        sprintf(texName, "textures/%d.bmp", i);
        wad.textures[i]->save(texName);
    }
*/

#endif


#endif