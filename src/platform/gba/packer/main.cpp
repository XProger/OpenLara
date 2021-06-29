#include <stdio.h>
#include <string.h>
#include <math.h>
#include <windows.h>


#define ASSERT(x) { if (!(x)) { DebugBreak(); } }

typedef signed char        int8;
typedef signed short       int16;
typedef signed int         int32;
typedef signed long long   int64;
typedef unsigned char      uint8;
typedef unsigned short     uint16;
typedef unsigned int       uint32;
typedef unsigned long long uint64;

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

struct vec3s
{
    int16 x, y, z;
};

struct vec3i
{
    int32 x, y, z;
};

struct Tile
{
    uint8 indices[256 * 256];
};

struct Palette
{
    uint8 colors[256 * 3];
};

struct FileStream
{
    FILE* f;

    FileStream(const char* fileName, bool write)
    {
        f = fopen(fileName, write ? "wb" : "rb");
    }

    ~FileStream()
    {
        if (f) fclose(f);
    }

    bool isValid()
    {
        return f != NULL;
    }

    void seek(int32 offset)
    {
        fseek(f, offset, SEEK_CUR);
    }

    uint32 getPos()
    {
        return ftell(f);
    }

    void setPos(uint32 pos)
    {
        fseek(f, pos, SEEK_SET);
    }

    uint32 align4()
    {
        uint32 pos = getPos();
        uint32 aligned = (pos + 3) & ~3;

        if (aligned != pos) {
            static const uint32 zero = 0;
            fwrite(&zero, 1, aligned - pos, f);
        }

        return aligned;
    }

    template <typename T>
    void read(T &result)
    {
        fread(&result, sizeof(result), 1, f);
    }

    template <typename T, typename C>
    void read(T* &elements, C count)
    {
        if (count) {
            elements = new T[count];
            fread(&elements[0], sizeof(elements[0]), count, f);
        } else {
            elements = NULL;
        }
    }

    template <typename T, typename C>
    void readArray(T* &elements, C &count)
    {
        read(count);
        read(elements, count);
    }

    template <typename T>
    void write(const T &result)
    {
        fwrite(&result, sizeof(result), 1, f);
    }

    template <typename T, typename C>
    void write(const T* elements, C count)
    {
        if (!elements || !count) return;
        fwrite(&elements[0], sizeof(elements[0]), count, f);
    }

    template <typename T, typename C>
    void writeArray(const T* elements, C count)
    {
        write(count);
        write(elements, count);
    }
};

template <typename T>
struct Array
{
    int32 count;
    T** items;

    Array() : count(0), items(NULL) {}

    ~Array()
    {
        delete[] items;
    }

    T* operator [] (int index) const { return items[index]; }

    int32 add(T* item)
    {
        count++;
        T** tmp = new T*[count];
        memcpy(tmp, items, (count - 1) * sizeof(T*));
        tmp[count - 1] = item;
        delete[] items;
        items = tmp;
        return count - 1;
    }

    int32 find(T* item)
    {
        for (int32 i = 0; i < count; i++)
        {
            if (items[i]->isEqual(item))
                return i;
        }
        return -1;
    }

    void qsort(T** v, int L, int R) {
        int i = L;
        int j = R;
        const T* m = v[(L + R) / 2];

        while (i <= j) {
            while (T::cmp(v[i], m) < 0) i++;
            while (T::cmp(m, v[j]) < 0) j--;

            if (i <= j)
            {
                T* tmp = v[i];
                v[i] = v[j];
                v[j] = tmp;
                i++;
                j--;
            }
        }

        if (L < j) qsort(v, L, j);
        if (i < R) qsort(v, i, R);
    }

    void sort() {
        if (count) {
            qsort(items, 0, count - 1);
        }
    }
};

void saveBitmap(const char* fileName, uint8* data, int32 width, int32 height)
{
    struct BITMAPFILEHEADER {
        uint32  bfSize;
        uint16  bfReserved1;
        uint16  bfReserved2;
        uint32  bfOffBits;
    } fhdr;

    struct BITMAPINFOHEADER{
        uint32 biSize;
        uint32 biWidth;
        uint32 biHeight;
        uint16 biPlanes;
        uint16 biBitCount;
        uint32 biCompression;
        uint32 biSizeImage;
        uint32 biXPelsPerMeter;
        uint32 biYPelsPerMeter;
        uint32 biClrUsed;
        uint32 biClrImportant;
    } ihdr;

    memset(&fhdr, 0, sizeof(fhdr));
    memset(&ihdr, 0, sizeof(ihdr));

    ihdr.biSize      = sizeof(ihdr);
    ihdr.biWidth     = width;
    ihdr.biHeight    = height;
    ihdr.biPlanes    = 1;
    ihdr.biBitCount  = 24;
    ihdr.biSizeImage = width * height * 3;

    fhdr.bfOffBits   = 2 + sizeof(fhdr) + ihdr.biSize;
    fhdr.bfSize      = fhdr.bfOffBits + ihdr.biSizeImage;

    FILE *f = fopen(fileName, "wb");
    if (f) {
        uint16 type = 'B' + ('M' << 8);
        fwrite(&type, sizeof(type), 1, f);
        fwrite(&fhdr, sizeof(fhdr), 1, f);
        fwrite(&ihdr, sizeof(ihdr), 1, f);
        for (int32 i = 0; i < height; i++)
        {
            fwrite(data + (height - i - 1) * width * 3, 3, width, f);
        }
        fclose(f);
    }
}

#define X_CLAMP(x, a, b) ((x) < (a) ? (a) : ((x) > (b) ? (b) : (x)))

void fixLightmap(const uint8* lightmap, uint16* palette, int32 palIndex)
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

void fixTexCoord(uint32 uv0, uint32 &uv1)
{
    int32 u0 = uv0 >> 24;
    int32 u1 = uv1 >> 24;
    int32 v0 = (uv0 >> 8) & 0xFF;
    int32 v1 = (uv1 >> 8) & 0xFF;

    if (abs(u1 - u0) > 127) {
        if (u1 > u0) {
            u1 = u0 + 127;
        } else {
            u1 = u0 - 127;
        }
    }

    if (abs(v1 - v0) > 127) {
        if (v1 > v0) {
            v1 = v0 + 127;
        } else {
            v1 = v0 - 127;
        }
    }

    uv1 = (u1 << 24) | (v1 << 8);
}

#define MAX_ROOMS           256
#define MAX_ROOM_VERTICES   0xFFFF
#define MAX_ROOM_QUADS      2048
#define MAX_ROOM_TRIANGLES  128
#define MAX_ROOM_SPRITES    64
#define MAX_ROOM_PORTALS    16
#define MAX_ROOM_SECTORS    (20*20)
#define MAX_FLOORS          (9 * 1024)
#define MAX_BOXES           1024
#define MAX_OVERLAPS        (6 * 1024)
#define MAX_ITEMS           240
#define MAX_NODES           32

#pragma pack(1)
struct LevelPC
{
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

        struct InfoComp
        {
            int16 x;
            int16 z;
            int16 yBottom;
            int16 yTop;

            uint16 quadsCount;
            uint16 trianglesCount;

            uint16 verticesCount;
            uint16 spritesCount;

            uint8 portalsCount;
            uint8 lightsCount;
            uint8 meshesCount;
            uint8 ambient;

            uint8 xSectors;
            uint8 zSectors;
            uint8 alternateRoom;
            uint8 flags;

            uint32 quads;
            uint32 triangles;
            uint32 vertices;
            uint32 sprites;
            uint32 portals;
            uint32 sectors;
            uint32 lights;
            uint32 meshes;
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
        };

        struct Quad
        {
            uint16 indices[4];
            uint16 flags;
        };

        struct Triangle
        {
            uint16 indices[3];
            uint16 flags;
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
    };

    struct AnimRange
    {
        int16 frameBegin;
        int16 frameEnd;
        int16 nextAnimIndex;
        int16 nextFrameIndex;
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

    struct StaticMeshComp
    {
        uint16 id;
        uint16 meshIndex;
        uint16 flags;
        MinMax vbox;
        MinMax cbox;
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
    };

    struct SpriteTexture
    {
        uint16 tile;
        uint8 u, v;
        uint16 w, h;
        int16 l, t, r, b;
    };

    struct SpriteTextureComp
    {
        uint16 tile;
        uint8 u, v;
        uint8 w, h;
        int16 l, t, r, b;
    };

    struct SpriteSequence
    {
        uint16 type;
        uint16 unused;
        int16 count;
        int16 start;
    };

    struct Camera
    {
        vec3i pos;
        int16 roomIndex;
        uint16 flags;
    };

    struct SoundSource
    {
        vec3i pos;
        uint16 id;
        uint16 flags;
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
        int8 minZ, maxZ;
        int8 minX, maxX;
        int16 floor;
        int16 overlap;
    };

    struct Overlap
    {
        uint16 value;
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
    };

    struct CameraFrame
    {
        vec3s target;
        vec3s pos;
        int16 fov;
        int16 roll;
    };

    struct SoundInfo
    {
        uint16 index;
        uint16 volume;
        uint16 chance;
        uint16 flags;
    };

    struct Node
    {
        uint32 flags;
        vec3i pos;
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
    Overlap* overlaps;

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

    struct Header
    {
        uint32 magic;

        uint16 tilesCount;
        uint16 roomsCount;
        uint16 modelsCount;
        uint16 meshesCount;
        uint16 staticMeshesCount;
        uint16 spriteSequencesCount;
        uint16 soundSourcesCount;
        uint16 boxesCount;
        uint16 texturesCount;
        uint16 itemsCount;
        uint16 camerasCount;
        uint16 cameraFramesCount;

        uint32 palette;
        uint32 lightmap;
        uint32 tiles;
        uint32 rooms;
        uint32 floors;
        uint32 meshData;
        uint32 meshOffsets;
        uint32 anims;
        uint32 states;
        uint32 ranges;
        uint32 commands;
        uint32 nodes;
        uint32 frameData;
        uint32 models;
        uint32 staticMeshes;
        uint32 objectTextures;
        uint32 spriteTextures;
        uint32 spriteSequences;
        uint32 cameras;
        uint32 soundSources;
        uint32 boxes;
        uint32 overlaps;
        uint32 zones[2][3];
        uint32 animTexData;
        uint32 items;
        uint32 cameraFrames;
        uint32 soundMap;
        uint32 soundInfos;
        uint32 soundData;
        uint32 soundOffsets;
    };

    Room::VertexComp roomVertices[MAX_ROOM_VERTICES];
    int32 roomVerticesCount;

    int32 addRoomVertex(const Room::Vertex &v)
    {
        Room::VertexComp comp;
        comp.x = v.pos.x / 1024;
        comp.y = v.pos.y / 256;
        comp.z = v.pos.z / 1024;
        comp.g = v.lighting >> 5;

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

    void fixObjectTexture(ObjectTextureComp &tex, int32 idx)
    {
        fixTexCoord(tex.uv0, tex.uv1);
        fixTexCoord(tex.uv0, tex.uv3);
        fixTexCoord(tex.uv1, tex.uv2);
        /*
        if (tex.attribute == 0)
            return;

        int32 u0 = tex.uv0 >> 24;
        int32 u1 = tex.uv1 >> 24;
        int32 u2 = tex.uv2 >> 24;
        int32 u3 = tex.uv3 >> 24;

        int32 v0 = (tex.uv0 >> 8) & 0xFF;
        int32 v1 = (tex.uv1 >> 8) & 0xFF;
        int32 v2 = (tex.uv2 >> 8) & 0xFF;
        int32 v3 = (tex.uv3 >> 8) & 0xFF;

        int32 minU = MIN(u0, MIN(u1, u2));
        int32 maxU = MAX(u0, MAX(u1, u2));
        int32 minV = MIN(v0, MIN(v1, v2));
        int32 maxV = MAX(v0, MAX(v1, v2));

        if (tex.uv3 != 0)
        {
            minU = MIN(minU, u3);
            maxU = MAX(maxU, u3);
            minV = MIN(minV, v3);
            maxV = MAX(maxV, v3);
        }

        uint8* tile = tiles[tex.tile].indices;

        int32 w = (maxU - minU) + 1;
        int32 h = (maxV - minV) + 1;

        uint8* data = new uint8[w * h * 4];
        uint8* ptr = data;

        bool flag = false;

        palette.colors[0] = 63;
        palette.colors[1] = 0;
        palette.colors[2] = 63;

        for (int32 y = minV; y <= maxV; y++)
        {
            for (int32 x = minU; x <= maxU; x++)
            {
                int32 index = tile[y * 256 + x];

                if (index == 0) {
                    flag = true;
                }

                *ptr++ = palette.colors[index * 3 + 2] << 2;
                *ptr++ = palette.colors[index * 3 + 1] << 2;
                *ptr++ = palette.colors[index * 3 + 0] << 2;
            }
        }

        if (flag) {
            char buf[256];
            sprintf(buf, "test/tex_%d.bmp", idx);
            saveBitmap(buf, data, w, h);
        }

        delete[] data;


        if (!flag) {
            tex.attribute = 0;
        }
        */
    }

    void convertGBA(const char* fileName)
    {
        FileStream f(fileName, true);
        
        if (!f.isValid()) return;

        Header header;
        f.seek(sizeof(Header)); // will be rewritten at the end

        header.magic = 0x31414247;
        header.tilesCount = tilesCount;
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

        header.palette = f.align4();

        { // convert palette to 15-bit and fix some color gradients
            uint16 pal[256];

            for (int32 i = 0; i < 256; i++)
            {
                uint8 r = palette.colors[i * 3 + 0];
                uint8 g = palette.colors[i * 3 + 1];
                uint8 b = palette.colors[i * 3 + 2];

                pal[i] = (r >> 1) | ((g >> 1) << 5) | ((b >> 1) << 10);
            }

            pal[0] = 0;
            //fixLightmap(lightmap, pal, 6); // boots
            //fixLightmap(lightmap, pal, 14); // skin

            f.write(pal);
        }

        for (int32 i = 0; i < 32; i++) {
            lightmap[i * 256] = 0;
        }

        header.lightmap = f.align4();
        f.write(lightmap);

        header.tiles = f.align4();
        f.write(tiles, tilesCount);

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
                info.yBottom = room->info.yBottom;
                info.yTop = room->info.yTop;

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

                info.quads = f.getPos();
                for (int32 i = 0; i < room->qCount; i++)
                {
                    Room::Quad q = room->quads[i];
                    q.indices[0] = addRoomVertex(room->vertices[q.indices[0]]);
                    q.indices[1] = addRoomVertex(room->vertices[q.indices[1]]);
                    q.indices[2] = addRoomVertex(room->vertices[q.indices[2]]);
                    q.indices[3] = addRoomVertex(room->vertices[q.indices[3]]);
                    f.write(q);
                }

                info.triangles = f.getPos();
                for (int32 i = 0; i < room->tCount; i++)
                {
                    Room::Triangle t = room->triangles[i];
                    t.indices[0] = addRoomVertex(room->vertices[t.indices[0]]);
                    t.indices[1] = addRoomVertex(room->vertices[t.indices[1]]);
                    t.indices[2] = addRoomVertex(room->vertices[t.indices[2]]);
                    f.write(t);
                }

                info.vertices = f.getPos();
                info.verticesCount = roomVerticesCount;
                f.write(roomVertices, roomVerticesCount);

                info.sprites = f.getPos();
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

                    f.write(comp);
                }

                info.portals = f.getPos();
                f.write(room->portals, room->pCount);

                info.sectors = f.getPos();
                f.write(room->sectors, room->zSectors * room->xSectors);
            
                info.lights = f.getPos();
                for (int32 i = 0; i < room->lCount; i++)
                {
                    const Room::Light* light = room->lights + i;

                    Room::LightComp comp;
                    comp.pos.x = light->pos.x - room->info.x;
                    comp.pos.y = light->pos.y;
                    comp.pos.z = light->pos.z - room->info.z;
                    comp.radius = light->radius >> 8;
                    comp.intensity = light->intensity >> 5;

                    f.write(comp);
                }

                info.meshes = f.getPos();
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

                    f.write(comp);
                }
            }

            int32 pos = f.getPos();
            f.setPos(header.rooms);
            f.write(infoComp, roomsCount);
            f.setPos(pos);
        }

        header.floors = f.align4();
        f.write(floors, floorsCount);

        header.meshData = f.align4();
        f.write(meshData, meshDataSize);

        header.meshOffsets = f.align4();
        f.write(meshOffsets, meshOffsetsCount);

        header.anims = f.align4();
        f.write(anims, animsCount);

        header.states = f.align4();
        for (int32 i = 0; i < statesCount; i++)
        {
            const LevelPC::AnimState* state = states + i;

            LevelPC::AnimStateComp comp;
            comp.state = uint8(state->state);
            comp.rangesCount = uint8(state->rangesCount);
            comp.rangesStart = state->rangesStart;

            f.write(comp);
        }

        header.ranges = f.align4();
        f.write(ranges, rangesCount);

        header.commands = f.align4();
        f.write(commands, commandsCount);

        header.nodes = f.align4();
        f.write(nodesData, nodesDataSize);

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

            f.write(comp);
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

            f.write(comp);
        }

        header.objectTextures = f.align4();
        for (int32 i = 0; i < objectTexturesCount; i++)
        {
            const LevelPC::ObjectTexture* objectTexture = objectTextures + i;

            ObjectTextureComp comp;
            comp.attribute = objectTexture->attribute;
            comp.tile = objectTexture->tile;
            comp.uv0 = ((objectTexture->uv0 << 16) | (objectTexture->uv0 >> 16)) & 0xFF00FF00;
            comp.uv1 = ((objectTexture->uv1 << 16) | (objectTexture->uv1 >> 16)) & 0xFF00FF00;
            comp.uv2 = ((objectTexture->uv2 << 16) | (objectTexture->uv2 >> 16)) & 0xFF00FF00;
            comp.uv3 = ((objectTexture->uv3 << 16) | (objectTexture->uv3 >> 16)) & 0xFF00FF00;

        #define FIX_TEXCOORD // GBA rasterizer doesn't support UV deltas over 127, due performance reason, so we clamp it

        #ifdef FIX_TEXCOORD
            fixObjectTexture(comp, i);
        #endif

            f.write(comp);
        }

        header.spriteTextures = f.align4();
        for (int32 i = 0; i < spriteTexturesCount; i++)
        {
            const LevelPC::SpriteTexture* spriteTexture = spriteTextures + i;

            SpriteTextureComp comp;
            comp.tile = spriteTexture->tile;
            comp.u = spriteTexture->u;
            comp.v = spriteTexture->v;
            comp.w = spriteTexture->w >> 8;
            comp.h = spriteTexture->h >> 8;
            comp.l = spriteTexture->l;
            comp.t = spriteTexture->t;
            comp.r = spriteTexture->r;
            comp.b = spriteTexture->b;

            f.write(comp);
        }

        f.write(spriteTextures, spriteTexturesCount);

        header.spriteSequences = f.align4();
        f.write(spriteSequences, spriteSequencesCount);

        header.cameras = f.align4();
        f.write(cameras, camerasCount);

        header.soundSources = f.align4();
        f.write(soundSources, soundSourcesCount);

        header.boxes = f.align4();
        for (int32 i = 0; i < boxesCount; i++)
        {
            const LevelPC::Box* box = boxes + i;

            BoxComp comp;
            comp.minX = box->minX / 1024;
            comp.minZ = box->minZ / 1024;
            comp.maxX = (box->maxX + 1) / 1024;
            comp.maxZ = (box->maxZ + 1) / 1024;
            comp.floor = box->floor;
            comp.overlap = box->overlap;

            f.write(comp);
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
        f.write(animTexData, animTexDataSize);

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

            f.write(comp);
        }

        header.cameraFrames = f.align4();
        f.write(cameraFrames, cameraFramesCount);

        //f.writeArray(demoData, demoDataSize);

        header.soundMap = f.align4();
        f.write(soundMap);

        header.soundInfos = f.align4();
        f.write(soundInfo, soundInfoCount);

        header.soundData = f.align4();
        f.write(soundData, soundDataSize);

        header.soundOffsets = f.align4();
        f.write(soundOffsets, soundOffsetsCount);

        f.setPos(0);
        f.write(header);
    }
};


#define COLOR_THRESHOLD_SQ (8 * 8)

const char* levelNames[] = {
#if 0
    "LEVEL1"
#else
    "TITLE",
    "GYM",
    "LEVEL1",
    "LEVEL2",
    "LEVEL3A",
    "LEVEL3B",
    "CUT1",
    "LEVEL4",
    "LEVEL5",
    "LEVEL6",
    "LEVEL7A",
    "LEVEL7B",
    "CUT2",
    "LEVEL8A",
    "LEVEL8B",
    "LEVEL8C",
    "LEVEL10A",
    "CUT3",
    "LEVEL10B",
    "CUT4",
    "LEVEL10C"
#endif
};


#define MAX_LEVELS sizeof(levelNames) / sizeof(levelNames[0])

LevelPC* levels[MAX_LEVELS];

struct WAD
{
    struct Texture
    {
        int32 id;
        uint8* data;
        int32 width;
        int32 height;

        Texture(const Tile* tile, const Palette* palette, int32 minX, int32 minY, int32 maxX, int32 maxY)
        {
            width = maxX - minX + 1;
            height = maxY - minY + 1;

            data = new uint8[width * height * 3];

            const uint8* src = tile->indices + 256 * minY;
            uint8* dst = data;

            for (int32 y = minY; y <= maxY; y++)
            {
                for (int32 x = minX; x <= maxX; x++)
                {
                    int32 idx = src[x] * 3;
                    if (idx == 0) {
                        dst[0] = 255;
                        dst[1] = 0;
                        dst[2] = 255;
                    } else {
                        dst[2] = palette->colors[idx + 0] << 2;
                        dst[1] = palette->colors[idx + 1] << 2;
                        dst[0] = palette->colors[idx + 2] << 2;
                    }
                    dst += 3;
                }
                src += 256;
            }
        }

        ~Texture()
        {
            delete[] data;
        }

        bool isEqual(Texture* tex)
        {
            if (width != tex->width || height != tex->height)
                return false;

            uint8* src = data;
            uint8* dst = tex->data;

            for (int32 i = 0; i < width * height; i++)
            {
                int32 dR = *src++ - *dst++;
                int32 dG = *src++ - *dst++;
                int32 dB = *src++ - *dst++;

                if (dR * dR + dG * dG + dB * dB > COLOR_THRESHOLD_SQ)
                    return false;
            }

            return true;
        }

        void save(const char* fileName)
        {
            saveBitmap(fileName, data, width, height);
        }

        static int cmp(const Texture* a, const Texture* b)
        {
            int32 p1 = MIN(a->width, a->height);
            int32 p2 = MIN(b->width, b->height);
            int32 i = p2 - p1;
            if (i == 0)
                i = a->id - b->id;
            return i;
        }
    };

    struct Tile24
    {
        struct Node
        {
            Node* childs[2];
            Texture* tex;
            int32 l, t, r, b;

            Node(short l, short t, short r, short b) : l(l), t(t), r(r), b(b), tex(NULL) { 
                childs[0] = childs[1] = NULL; 
            }

            ~Node() {
                delete childs[0];
                delete childs[1];
            }

            Node* insert(Texture* tex)
            {
                if (childs[0] != NULL && childs[1] != NULL)
                {
                    Node* node = childs[0]->insert(tex);
                    if (node != NULL)
                        return node;
                    return childs[1]->insert(tex);
                }

                if (this->tex != NULL)
                    return NULL;

                int16 nw  = r - l;
                int16 nh  = b - t;
                int16 tw = tex->width;
                int16 th = tex->height;

                if (nw < tw || nh < th)
                    return NULL;

                if (nw == tw && nh == th) {
                    this->tex = tex;
                    return this;
                }

                int16 dx = nw - tw;
                int16 dy = nh - th;

                if (dx > dy) {
                    childs[0] = new Node(l, t, l + tw, b);
                    childs[1] = new Node(l + tw, t, r, b);
                } else {
                    childs[0] = new Node(l, t, r, t + th);
                    childs[1] = new Node(l, t + th, r, b);
                }

                return childs[0]->insert(tex);
            }

            void fill(uint8* data)
            {
                if (childs[0] != NULL && childs[1] != NULL)
                {
                    childs[0]->fill(data);
                    childs[1]->fill(data);
                }

                if (!tex) {
                    return;
                }

                // fill code
                for (int32 y = 0; y < tex->height; y++)
                {
                    memcpy(data + ((t + y) * 256 + l) * 3, tex->data + y * tex->width * 3, tex->width * 3);
                }
            }
        };

        Node* root;

        Tile24()
        {
            root = new Node(0, 0, 256, 256);
        }

        ~Tile24()
        {
            delete root;
        }

        void fill(uint8* data)
        {
            for (int32 i = 0; i < 256 * 256; i++)
            {
                data[i * 3 + 0] = 255;
                data[i * 3 + 1] = 0;
                data[i * 3 + 2] = 255;
            }

            root->fill(data);
        }

        static int cmp(const Tile24* a, const Tile24* b)
        {
            return 0;
        }
    };

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

            void addQuad(const LevelPC::Room::Quad &q, const LevelPC::Room::Vertex* verts)
            {
                Quad n;
                n.flags = q.flags;
                n.indices[0] = addVertex(verts[q.indices[0]]);
                n.indices[1] = addVertex(verts[q.indices[1]]);
                n.indices[2] = addVertex(verts[q.indices[2]]);
                n.indices[3] = addVertex(verts[q.indices[3]]);
                quads[qCount++] = n;
            }

            void addTriangle(const LevelPC::Room::Triangle &t, const LevelPC::Room::Vertex* verts)
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

    struct Node
    {
        uint32 flags;
        vec3i pos;
    };

    struct NodeList
    {
        int32 count;
        Node nodes[MAX_NODES];
    };

    struct Model
    {
        uint16 count;
        uint16 start;
        uint16 nodeIndex;
        uint16 frameIndex;
        uint16 animIndex;
    };

    Array<Texture> textures;
    Array<Tile24> tiles;
    Array<NodeList> nodeLists;


    Model* models[MAX_ITEMS];

    bool itemsUsed[MAX_ITEMS];

    LevelWAD* levels[MAX_LEVELS];

    WAD()
    {
        memset(models, 0, sizeof(models));
        memset(itemsUsed, 0, sizeof(itemsUsed));
    }

    int32 addTexture(Texture* tex)
    {
        int32 index = textures.find(tex);
        if (index != -1) {
            delete tex;
            return index;
        }
        return textures.add(tex);
    }

    void addLevel(int32 index, const LevelPC &level)
    {
        int32 texId = 0;

    // textures
        for (int32 i = 0; i < level.objectTexturesCount; i++)
        {
            LevelPC::ObjectTexture* objTex = level.objectTextures + i;
            Tile *tile = level.tiles + (objTex->tile & 0x3FFF);

            int32 minX = MIN(MIN(objTex->x0, objTex->x1), objTex->x2);
            int32 minY = MIN(MIN(objTex->y0, objTex->y1), objTex->y2);
            int32 maxX = MAX(MAX(objTex->x0, objTex->x1), objTex->x2);
            int32 maxY = MAX(MAX(objTex->y0, objTex->y1), objTex->y2);

            if (objTex->isQuad)
            {
                minX = MIN(minX, objTex->x3);
                minY = MIN(minY, objTex->y3);
                maxX = MAX(maxX, objTex->x3);
                maxY = MAX(maxY, objTex->y3);
            }

            Texture* tex = new Texture(tile, &level.palette, minX, minY, maxX, maxY);
            tex->id = texId++;
            addTexture(tex);
        }

    // sprites
        for (int32 i = 0; i < level.spriteTexturesCount; i++)
        {
            LevelPC::SpriteTexture* sprTex = level.spriteTextures + i;
            Tile *tile = level.tiles + (sprTex->tile & 0x3FFF);

            Texture* tex = new Texture(tile, &level.palette, sprTex->u, sprTex->v, sprTex->u + sprTex->w, sprTex->v + sprTex->h);
            tex->id = texId++;
            addTexture(tex);
        }

        levels[index] = new LevelWAD(level);

        for (int32 i = 0; i < level.modelsCount; i++)
        {
            const LevelPC::Model &model = level.models[i];

            if (models[model.type] == NULL)
            {
                models[model.type] = new Model();
                models[model.type]->nodeIndex = 0;
                models[model.type]->count = 0;
            }

            Model *m = models[model.type];

            int32 newIndex = addNodes((LevelPC::Node*)(level.nodesData + model.nodeIndex), model.count - 1);
            if (newIndex != 0) {
                models[model.type]->nodeIndex = newIndex;
            }

            if (!(m->count == 0 || m->count == model.count))
                printf("aaa");

            m->count = model.count;
            m->start = 0; // TODO
            m->frameIndex = 0; // TODO
            m->animIndex = 0; // TODO


            // TODO cutscenes and some other objects have variable nodes between levels
        }

        for (int32 i = 0; i < level.itemsCount; i++)
        {
            itemsUsed[level.items[i].type] = true;
        }

        for (int32 i = 0; i < level.spriteSequencesCount; i++)
        {
            const LevelPC::SpriteSequence &s = level.spriteSequences[i];
            itemsUsed[s.type] = true;
        }

        for (int32 i = 0; i < level.staticMeshesCount; i++)
        {
            const LevelPC::StaticMesh &m = level.staticMeshes[i];
            // TODO collect unique static meshes
        }
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

int main()
{
    for (int32 i = 0; i < MAX_LEVELS; i++)
    {
        char path[64];
        sprintf(path, "levels/%s.PHD", levelNames[i]);
        levels[i] = new LevelPC(path);

        sprintf(path, "../data/%s.PKD", levelNames[i]);
        levels[i]->convertGBA(path);
    }
    return 0;
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
    for (int32 i = 0; i < MAX_LEVELS; i++)
    {
        delete levels[i];
    }

    return 0;
}
