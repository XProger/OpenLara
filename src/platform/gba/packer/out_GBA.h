#ifndef H_OUT_GBA
#define H_OUT_GBA

#include "common.h"
#include "TR1_PC.h"
#include "TR1_PSX.h"

struct out_GBA
{
    enum FaceType { // 2 high bits of face flags
        FACE_TYPE_SHADOW,
        FACE_TYPE_F,
        FACE_TYPE_FT,
        FACE_TYPE_FTA
    };

    enum {
        FACE_TYPE_SHIFT = 14,
        FACE_TRIANGLE   = (1 << 13)
    };

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
        uint16 spritesCount;
        uint16 itemsCount;
        uint16 camerasCount;
        uint16 cameraFramesCount;
        uint16 soundOffsetsCount;

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

        void write(FileStream &f) const
        {
            f.write(magic);
            f.write(tilesCount);
            f.write(roomsCount);
            f.write(modelsCount);
            f.write(meshesCount);
            f.write(staticMeshesCount);
            f.write(spriteSequencesCount);
            f.write(soundSourcesCount);
            f.write(boxesCount);
            f.write(texturesCount);
            f.write(spritesCount);
            f.write(itemsCount);
            f.write(camerasCount);
            f.write(cameraFramesCount);
            f.write(soundOffsetsCount);

            f.write(palette);
            f.write(lightmap);
            f.write(tiles);
            f.write(rooms);
            f.write(floors);
            f.write(meshData);
            f.write(meshOffsets);
            f.write(anims);
            f.write(states);
            f.write(ranges);
            f.write(commands);
            f.write(nodes);
            f.write(frameData);
            f.write(models);
            f.write(staticMeshes);
            f.write(objectTextures);
            f.write(spriteTextures);
            f.write(spriteSequences);
            f.write(cameras);
            f.write(soundSources);
            f.write(boxes);
            f.write(overlaps);

            for (int32 i = 0; i < 2; i++)
            {
                for (int32 j = 0; j < 3; j++)
                {
                    f.write(zones[i][j]);
                }
            }

            f.write(animTexData);
            f.write(items);
            f.write(cameraFrames);
            f.write(soundMap);
            f.write(soundInfos);
            f.write(soundData);
            f.write(soundOffsets);
        }
    };

    struct Info
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

        void write(FileStream &f) const
        {
            f.write(x);
            f.write(z);
            f.write(yBottom);
            f.write(yTop);
            f.write(quadsCount);
            f.write(trianglesCount);
            f.write(verticesCount);
            f.write(spritesCount);
            f.write(portalsCount);
            f.write(lightsCount);
            f.write(meshesCount);
            f.write(ambient);
            f.write(xSectors);
            f.write(zSectors);
            f.write(alternateRoom);
            f.write(flags);
            f.write(quads);
            f.write(triangles);
            f.write(vertices);
            f.write(sprites);
            f.write(portals);
            f.write(sectors);
            f.write(lights);
            f.write(meshes);
        }
    };

    struct RoomVertex
    {
        uint8 x, y, z, g;

        void write(FileStream &f) const
        {
            f.write(x);
            f.write(y);
            f.write(z);
            f.write(g);
        }
    };

    struct RoomQuad
    {
        uint16 flags;
        uint16 indices[4];

        void write(FileStream &f) const
        {
            f.write(flags);
            f.write(indices[0]);
            f.write(indices[1]);
            f.write(indices[2]);
            f.write(indices[3]);
        }
    };

    struct RoomTriangle
    {
        uint16 flags;
        uint16 indices[3];
        
        void write(FileStream &f) const
        {
            f.write(flags);
            f.write(indices[0]);
            f.write(indices[1]);
            f.write(indices[2]);
        }
    };

    struct MeshQuad
    {
        uint16 flags;
        uint8 indices[4];

        void write(FileStream &f) const
        {
            f.write(flags);
            f.write(indices[0]);
            f.write(indices[1]);
            f.write(indices[2]);
            f.write(indices[3]);
        }
    };

    struct MeshTriangle
    {
        uint16 flags;
        uint8 indices[4];
 
        void write(FileStream &f) const
        {
            f.write(flags);
            f.write(indices[0]);
            f.write(indices[1]);
            f.write(indices[2]);
            f.write(indices[3]);
        }
    };

    struct AnimState
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

    struct Node
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

    struct Sprite
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

    struct Portal
    {
        uint8 roomIndex;
        uint8 normalIndex;
        uint16 x, y, z;
        uint8 a, b;

        Portal(const TR1_PC::Room::Portal &portal)
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

            int32 sx = (maxX - minX) / 256 >> 1;
            int32 sy = (maxY - minY) / 256 >> 1;
            int32 sz = (maxZ - minZ) / 256 >> 1;

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

        void write(FileStream &f) const
        {
            f.write(roomIndex);
            f.write(normalIndex);
            f.write(x);
            f.write(y);
            f.write(z);
            f.write(a);
            f.write(b);
        }
    };

    struct Light
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

    struct RoomMesh
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

    struct StaticMesh
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
        uint32 tile;
        uint32 uv01;
        uint32 uv23;

        void write(FileStream &f) const
        {
            f.write(tile);
            f.write(uv01);
            f.write(uv23);
        }
    };

    struct SpriteTexture
    {
        uint32 tile;
        uint32 uwvh;
        int16 l, t, r, b;

        void write(FileStream &f) const
        {
            f.write(tile);
            f.write(uwvh);
            f.write(l);
            f.write(t);
            f.write(r);
            f.write(b);
        }
    };

    struct Box
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

    struct Item
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

    struct Mesh
    {
        int32 offset;
        TR1_PC* level;
        vec3s center;
        int16 radius;
        uint16 intensity;
        int16 vCount;
        int16 rCount;
        int16 crCount;
        int16 tCount;
        int16 ctCount;
        const vec3s* vertices;
        const TR1_PC::Quad* rFaces;
        const TR1_PC::Quad* crFaces;
        const TR1_PC::Triangle* tFaces;
        const TR1_PC::Triangle* ctFaces;

        void init(TR1_PC* level, const uint8* ptr)
        {
            this->level = level;

            center = *(vec3s*)ptr; ptr += sizeof(center);
            radius = *(int16*)ptr; ptr += sizeof(radius);
            uint16 flags = *(uint16*)ptr; ptr += sizeof(flags);

            vCount = *(int16*)ptr; ptr += 2;
            vertices = (vec3s*)ptr;
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

            rCount = *(int16*)ptr; ptr += 2;
            rFaces = (TR1_PC::Quad*)ptr; ptr += rCount * sizeof(TR1_PC::Quad);

            tCount = *(int16*)ptr; ptr += 2;
            tFaces = (TR1_PC::Triangle*)ptr; ptr += tCount * sizeof(TR1_PC::Triangle);

            crCount = *(int16*)ptr; ptr += 2;
            crFaces = (TR1_PC::Quad*)ptr; ptr += crCount * sizeof(TR1_PC::Quad);

            ctCount = *(int16*)ptr; ptr += 2;
            ctFaces = (TR1_PC::Triangle*)ptr; ptr += ctCount * sizeof(TR1_PC::Triangle);

            intensity = 0;

            if (vIntensity)
            {
                uint32 sum = 0;
                for (int32 i = 0; i < vCount; i++)
                {
                    sum += vIntensity[i];
                }
                intensity = sum / vCount;
            }
        }

        void write(FileStream &f) const
        {
            f.write(center.x);
            f.write(center.y);
            f.write(center.z);
            f.write(radius);
            f.write(intensity);
            f.write(vCount);
            f.write(int16(rCount + crCount));
            f.write(int16(tCount + ctCount));
            f.write(int16(0));
            f.write(int16(0));

            for (int32 j = 0; j < vCount; j++)
            {
                struct MeshVertexGBA {
                    int16 x, y, z;
                } v;

                v.x = vertices[j].x;
                v.y = vertices[j].y;
                v.z = vertices[j].z;

                f.write(v.x);
                f.write(v.y);
                f.write(v.z);
            }

            for (int32 j = 0; j < rCount; j++)
            {
                TR1_PC::Quad q = rFaces[j];

                MeshQuad comp;
                comp.indices[0] = q.indices[0];
                comp.indices[1] = q.indices[1];
                comp.indices[2] = q.indices[2];
                comp.indices[3] = q.indices[3];
                comp.flags = q.flags & FACE_TEXTURE;
                if (level->objectTextures[comp.flags].attribute & TEX_ATTR_AKILL) {
                    comp.flags |= (FACE_TYPE_FTA << FACE_TYPE_SHIFT);
                } else {
                    comp.flags |= (FACE_TYPE_FT << FACE_TYPE_SHIFT);
                }

                comp.write(f);
            }

            for (int32 j = 0; j < crCount; j++)
            {
                TR1_PC::Quad q = crFaces[j];

                MeshQuad comp;
                comp.indices[0] = q.indices[0];
                comp.indices[1] = q.indices[1];
                comp.indices[2] = q.indices[2];
                comp.indices[3] = q.indices[3];
                comp.flags = q.flags & FACE_TEXTURE;
                comp.flags |= (FACE_TYPE_F << FACE_TYPE_SHIFT);

                comp.write(f);
            }

            for (int32 j = 0; j < tCount; j++)
            {
                TR1_PC::Triangle t = tFaces[j];

                MeshTriangle comp;
                comp.indices[0] = t.indices[0];
                comp.indices[1] = t.indices[1];
                comp.indices[2] = t.indices[2];
                comp.indices[3] = 0;
                comp.flags = t.flags & FACE_TEXTURE;
                if (level->objectTextures[comp.flags].attribute & TEX_ATTR_AKILL) {
                    comp.flags |= (FACE_TYPE_FTA << FACE_TYPE_SHIFT);
                } else {
                    comp.flags |= (FACE_TYPE_FT << FACE_TYPE_SHIFT);
                }
                comp.flags |= FACE_TRIANGLE;

                comp.write(f);
            }

            for (int32 j = 0; j < ctCount; j++)
            {
                TR1_PC::Triangle t = ctFaces[j];

                MeshTriangle comp;
                comp.indices[0] = t.indices[0];
                comp.indices[1] = t.indices[1];
                comp.indices[2] = t.indices[2];
                comp.indices[3] = 0;
                comp.flags = t.flags & FACE_TEXTURE;
                comp.flags |= (FACE_TYPE_F << FACE_TYPE_SHIFT);
                comp.flags |= FACE_TRIANGLE;

                comp.write(f);
            }
        }
    };

    struct Model
    {
        TR1_PC* level;
        int32 objTexIndex;
        uint8 type;
        uint8 count;
        uint16 start;
        uint16 nodeIndex;
        uint16 animIndex;

        void init(TR1_PC* level, const TR1_PC::Model &model)
        {
            this->level = level;
        }

        void write(FileStream &f) const
        {
            f.write(type);
            f.write(count);
            f.write(start);
            f.write(nodeIndex);
            f.write(animIndex);
        }
    };

    struct AnimFrames
    {
        const uint8* data;
        uint32 size;

        void init(const uint8* startPtr, const uint8* endPtr)
        {
            data = startPtr;
            size = endPtr - startPtr;
        }

        void write(FileStream &f) const
        {
            f.write(data, size);
        }
    };


    struct Texture
    {
        int32 texId;
        LevelID levelId;
        uint8* data;
        int32 width;
        int32 height;
        int32 akill;
        bool isSprite;
        uint8* indices;
        uint32 indexSum;

        Texture(const TR1_PC::Tile* tile, const TR1_PC::Palette* palette, bool transp, int32 minX, int32 minY, int32 maxX, int32 maxY)
        {
            width = maxX - minX + 1;
            height = maxY - minY + 1;
            akill = 0;
            indexSum = 0;

            indices = new uint8[width * height];

            data = new uint8[width * height * 3];

            const uint8* src = tile->indices + 256 * minY;
            uint8* dst = data;
            uint8* idx = indices;

            for (int32 y = minY; y <= maxY; y++)
            {
                for (int32 x = minX; x <= maxX; x++)
                {
                    int32 index = src[x] * 3;
                    *idx++ = index;
                    indexSum += index;

                    if ((index == 0) && transp) {
                        dst[0] = 255;
                        dst[1] = 0;
                        dst[2] = 255;
                        akill++;
                    } else {
                        dst[2] = palette->colors[index + 0] << 2;
                        dst[1] = palette->colors[index + 1] << 2;
                        dst[0] = palette->colors[index + 2] << 2;
                    }
                    dst += 3;
                }
                src += 256;
            }
        }

        ~Texture()
        {
            delete[] indices;
            delete[] data;
        }

        bool isEqual(Texture* tex)
        {
            if (width != tex->width || height != tex->height)
                return false;

        //#define TEX_COMPARE_COLORS

        #ifdef TEX_COMPARE_COLORS
            #define COLOR_THRESHOLD_SQ (8 * 8)

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
        #else
            if (indexSum != tex->indexSum)
                return false;
            return memcmp(indices, tex->indices, width * height) == 0;
        #endif
        }

        void save(const char* fileName)
        {
            saveBitmap(fileName, data, width, height);
        }

        static int cmp(const Texture* a, const Texture* b)
        {
            int32 max1 = MAX(a->width, a->height);
            int32 max2 = MAX(b->width, b->height);            
            int32 i = max2 - max1;

            if (i == 0) {
                i = a->akill - b->akill;
            }

            if (i == 0) {
                i = a->levelId - b->levelId;
            }

            if (i == 0) {
                i = a->texId - b->texId;
            }

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

            void fill24(uint8* data)
            {
                if (childs[0] != NULL && childs[1] != NULL)
                {
                    childs[0]->fill24(data);
                    childs[1]->fill24(data);
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

            void fill8(uint8* data)
            {
                if (childs[0] != NULL && childs[1] != NULL)
                {
                    childs[0]->fill8(data);
                    childs[1]->fill8(data);
                }

                if (!tex) {
                    return;
                }

                // fill code
                for (int32 y = 0; y < tex->height; y++)
                {
                    memcpy(data + ((t + y) * 256 + l), tex->indices + y * tex->width, tex->width);
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

        void fill24(uint8* data)
        {
            root->fill24(data);
        }

        void fill8(uint8* data)
        {
            root->fill8(data);
        }

        static int cmp(const Tile24* a, const Tile24* b)
        {
            return 0;
        }
    };

    RoomVertex* roomVertices;
    int32 roomVerticesCount;

    Mesh* meshes;
    int32 meshesCount;
    int32 meshesRemap[LVL_MAX][MAX_MESHES];

    Model* models;
    int32 modelsCount;
    int32 modelsRemap[LVL_MAX][MAX_MODELS];

    AnimFrames* animFrames;
    int32 animFramesCount;
    int32 animFramesRemap[LVL_MAX][MAX_ANIMS];

    Array<Texture> textures;
    Array<Tile24> tiles;

    void packTiles(FileStream &f)
    {
        textures.sort();

        for (int32 i = 0; i < textures.count; i++)
        {
            Texture* tex = textures[i];

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

    // save bitmap (debug)
        for (int32 i = 0; i < 256 * 256 * tiles.count; i++)
        {
            data[i * 3 + 0] = 255;
            data[i * 3 + 1] = 0;
            data[i * 3 + 2] = 255;
        }

        for (int32 i = 0; i < tiles.count; i++)
        {
            tiles[i]->fill24(data + i * 256 * 256 * 3);
        }
        saveBitmap("tiles.bmp", data, 256, 256 * tiles.count);


        memset(data, 0, 256 * 256 * tiles.count);

        for (int32 i = 0; i < tiles.count; i++)
        {
            tiles[i]->fill8(data + i * 256 * 256);
        }

        f.write(data,  256 * 256 * tiles.count);

        delete[] data;
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

    void addTextures(const TR1_PC* level)
    {
        static int32 texId = 0;

    // textures
        for (int32 i = 0; i < level->objectTexturesCount; i++)
        {
            TR1_PC::ObjectTexture* objTex = level->objectTextures + i;
            TR1_PC::Tile *tile = level->tiles + (objTex->tile & 0x3FFF);

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

            Texture* tex = new Texture(tile, &level->palette, objTex->attribute & TEX_ATTR_AKILL, minX, minY, maxX, maxY);
            tex->texId = texId++;
            tex->levelId = level->id;
            tex->isSprite = false;
            if (!(objTex->attribute & TEX_ATTR_AKILL))
                tex->akill = 0;
            addTexture(tex);
        }

    // sprites
        for (int32 i = 0; i < level->spriteTexturesCount; i++)
        {
            TR1_PC::SpriteTexture* sprTex = level->spriteTextures + i;
            TR1_PC::Tile *tile = level->tiles + (sprTex->tile & 0x3FFF);

            Texture* tex = new Texture(tile, &level->palette, true, sprTex->u, sprTex->v, sprTex->u + (sprTex->w >> 8), sprTex->v + (sprTex->h >> 8));
            tex->texId = texId++;
            tex->levelId = level->id;
            tex->isSprite = true;
            addTexture(tex);
        }
    }

    int32 addRoomVertex(int32 yOffset, const TR1_PC::Room::Vertex &v)
    {
        RoomVertex comp;
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
        comp.g = v.lighting >> 5;

        for (int32 i = 0; i < roomVerticesCount; i++)
        {
            if (memcmp(roomVertices + i, &comp, sizeof(comp)) == 0)
            {
                return i;
            }
        }

        ASSERT(roomVerticesCount < 0xFFFF);

        roomVertices[roomVerticesCount] = comp;
        return roomVerticesCount++;
    }

    int32 addMesh(const Mesh &mesh)
    {
        for (int32 i = 0; i < meshesCount; i++)
        {
            const Mesh &m = meshes[i];
            if (m.center.x != mesh.center.x ||
                m.center.y != mesh.center.y ||
                m.center.z != mesh.center.z ||
                m.radius != mesh.radius ||
                m.intensity != mesh.intensity || // TODO move to static mesh to save 5k
                m.vCount < mesh.vCount ||
                m.rCount < mesh.rCount ||
                m.crCount < mesh.crCount ||
                m.tCount < mesh.tCount ||
                m.ctCount < mesh.ctCount) continue;

            if (memcmp(m.vertices, mesh.vertices, mesh.vCount * sizeof(mesh.vertices[0])) != 0)
                continue;

            return i;
        }

        meshes[meshesCount] = mesh;
        return meshesCount++;
    }

    int32 addModel(const Model &model)
    {
        return 0;
    }

    int32 addAnimFrames(const AnimFrames &anim)
    {
        for (int32 i = 0; i < animFramesCount; i++)
        {
            const AnimFrames &af = animFrames[i];

            if (af.size < anim.size)
                continue;

            if (memcmp(af.data, anim.data, anim.size) != 0)
                continue;

            return i;
        }

        animFrames[animFramesCount] = anim;
        return animFramesCount++;
    }

    void fixObjectTexture(ObjectTexture &tex, int32 idx)
    {
        uint32 uv0 = tex.uv01 & 0xFF00FF00;
        uint32 uv1 = (tex.uv01 << 8) & 0xFF00FF00;
        uint32 uv2 = tex.uv23 & 0xFF00FF00;
        uint32 uv3 = (tex.uv23 << 8) & 0xFF00FF00;

        fixTexCoord(uv0, uv1);
        fixTexCoord(uv0, uv3);
        fixTexCoord(uv1, uv2);

        tex.uv01 = uv0 | (uv1 >> 8);
        tex.uv23 = uv2 | (uv3 >> 8);
    }

    void writePalette(FileStream &f, Header &header, TR1_PC* level)
    {
        header.palette = f.align4();

        uint16 pal[256];

        for (int32 i = 0; i < 256; i++)
        {
            uint8 r = level->palette.colors[i * 3 + 0];
            uint8 g = level->palette.colors[i * 3 + 1];
            uint8 b = level->palette.colors[i * 3 + 2];

            pal[i] = (r >> 1) | ((g >> 1) << 5) | ((b >> 1) << 10);
        }

        pal[0] = 0;

        f.write(pal, 256);
    }

    void writeLightmap(FileStream &f, Header &header, TR1_PC* level)
    {
        header.lightmap = f.align4();

        for (int32 i = 0; i < 32; i++) {
            level->lightmap[i * 256] = 0;
        }
        f.write(level->lightmap, 32 * 256);
    }

    void writeRooms(FileStream &f, Header &header, TR1_PC* level)
    {
        header.roomsCount = level->roomsCount;
        header.rooms = f.align4();
        {
            f.seek(sizeof(Info) * level->roomsCount);

            Info infos[255];

            for (int32 i = 0; i < level->roomsCount; i++)
            {
                const TR1_PC::Room* room = level->rooms + i;

                Info &info = infos[i];

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
                    TR1_PC::Room::Vertex &v = room->vertices[j];
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
                    TR1_PC::Quad q = room->quads[i];
                    RoomQuad comp;
                    comp.indices[0] = addRoomVertex(info.yTop, room->vertices[q.indices[0]]);
                    comp.indices[1] = addRoomVertex(info.yTop, room->vertices[q.indices[1]]);
                    comp.indices[2] = addRoomVertex(info.yTop, room->vertices[q.indices[2]]);
                    comp.indices[3] = addRoomVertex(info.yTop, room->vertices[q.indices[3]]);

                    comp.flags = q.flags & FACE_TEXTURE;
                    if (level->objectTextures[comp.flags].attribute & TEX_ATTR_AKILL) {
                        comp.flags |= (FACE_TYPE_FTA << FACE_TYPE_SHIFT);
                    } else {
                        comp.flags |= (FACE_TYPE_FT << FACE_TYPE_SHIFT);
                    }

                    comp.write(f);
                }

                info.triangles = f.align4();
                for (int32 i = 0; i < room->tCount; i++)
                {
                    TR1_PC::Triangle t = room->triangles[i];
                    RoomTriangle comp;

                    comp.indices[0] = addRoomVertex(info.yTop, room->vertices[t.indices[0]]);
                    comp.indices[1] = addRoomVertex(info.yTop, room->vertices[t.indices[1]]);
                    comp.indices[2] = addRoomVertex(info.yTop, room->vertices[t.indices[2]]);
                    
                    comp.flags = t.flags & FACE_TEXTURE;
                    if (level->objectTextures[comp.flags].attribute & TEX_ATTR_AKILL) {
                        comp.flags |= (FACE_TYPE_FTA << FACE_TYPE_SHIFT);
                    } else {
                        comp.flags |= (FACE_TYPE_FT << FACE_TYPE_SHIFT);
                    }
                    comp.flags |= FACE_TRIANGLE;

                    comp.write(f);
                }

                info.vertices = f.align4();
                info.verticesCount = roomVerticesCount;
                for (int32 i = 0; i < roomVerticesCount; i++)
                {
                    roomVertices[i].write(f);
                }

                info.sprites = f.align4();
                for (int32 i = 0; i < room->sCount; i++)
                {
                    const TR1_PC::Room::Sprite* sprite = room->sprites + i;
                    const TR1_PC::Room::Vertex* v = room->vertices + sprite->index;

                    Sprite comp;
                    comp.x = v->pos.x;
                    comp.y = v->pos.y;
                    comp.z = v->pos.z;
                    comp.g = v->lighting >> 5;
                    comp.index = uint8(sprite->texture);

                    ASSERT(sprite->texture <= 255);

                    comp.write(f);
                }

                info.portals = f.align4();
            #if 0 // -72k
                for (int32 i = 0; i < room->pCount; i++)
                {
                    Portal p = room->portals[i];
                    p.write(f);
                }
            #else
                f.writeObj(room->portals, room->pCount);
            #endif

                info.sectors = f.align4();
                f.writeObj(room->sectors, room->zSectors * room->xSectors);
            
                info.lights = f.align4();
                for (int32 i = 0; i < room->lCount; i++)
                {
                    const TR1_PC::Room::Light* light = room->lights + i;

                    Light comp;
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
                    const TR1_PC::Room::Mesh* mesh = room->meshes + i;

                    RoomMesh comp;
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
            f.writeObj(infos, header.roomsCount);
            f.setPos(pos);
        }
    }

    void writeFloors(FileStream &f, Header &header, TR1_PC* level)
    {
        header.floors = f.align4();
        f.writeObj(level->floors, level->floorsCount);
    }

    void writeStaticMeshes(FileStream &f, Header &header, TR1_PC* level)
    {
        header.staticMeshes = f.align4();
        for (int32 i = 0; i < level->staticMeshesCount; i++)
        {
            const TR1_PC::StaticMesh* staticMesh = level->staticMeshes + i;

            StaticMesh comp;
            comp.id = staticMesh->id;
            comp.meshIndex = staticMesh->meshIndex;
            comp.flags = staticMesh->flags;

            comp.vbox.minX = staticMesh->vbox.minX;
            comp.vbox.maxX = staticMesh->vbox.maxX;
            comp.vbox.minY = staticMesh->vbox.minY;
            comp.vbox.maxY = staticMesh->vbox.maxY;
            comp.vbox.minZ = staticMesh->vbox.minZ;
            comp.vbox.maxZ = staticMesh->vbox.maxZ;

            comp.cbox.minX = staticMesh->cbox.minX;
            comp.cbox.maxX = staticMesh->cbox.maxX;
            comp.cbox.minY = staticMesh->cbox.minY;
            comp.cbox.maxY = staticMesh->cbox.maxY;
            comp.cbox.minZ = staticMesh->cbox.minZ;
            comp.cbox.maxZ = staticMesh->cbox.maxZ;

            comp.write(f);
        }
    }

    void writeCameras(FileStream &f, Header &header, TR1_PC* level)
    {
        header.cameras = f.align4();
        f.writeObj(level->cameras, level->camerasCount);
    }

    void writeSoundSources(FileStream &f, Header &header, TR1_PC* level)
    {
        header.soundSources = f.align4();
        f.writeObj(level->soundSources, level->soundSourcesCount);
    }

    void writeBoxes(FileStream &f, Header &header, TR1_PC* level)
    {
        header.boxes = f.align4();
        header.boxesCount = level->boxesCount;

        for (int32 i = 0; i < level->boxesCount; i++)
        {
            const TR1_PC::Box* box = level->boxes + i;

            Box comp;
            comp.minX = box->minX / 1024;
            comp.minZ = box->minZ / 1024;
            comp.maxX = (box->maxX + 1) / 1024;
            comp.maxZ = (box->maxZ + 1) / 1024;
            comp.floor = box->floor;
            comp.overlap = box->overlap;

            comp.write(f);
        }
    }

    void writeOverlaps(FileStream &f, Header &header, TR1_PC* level)
    {
        header.overlaps = f.align4();
        f.write(level->overlaps, level->overlapsCount);
    }

    void writeZones(FileStream &f, Header &header, TR1_PC* level)
    {
        for (int32 i = 0; i < 2; i++)
        {
            header.zones[i][0] = f.align4();
            f.write(level->zones[i].ground1, level->boxesCount);

            header.zones[i][1] = f.align4();
            f.write(level->zones[i].ground2, level->boxesCount);

            header.zones[i][2] = f.align4();
            f.write(level->zones[i].fly, level->boxesCount);
        }
    }

    void writeAnimTex(FileStream &f, Header &header, TR1_PC* level)
    {
        header.animTexData = f.align4();
        f.write(level->animTexData, level->animTexDataSize);
    }

    void writeItems(FileStream &f, Header &header, TR1_PC* level)
    {
        header.items = f.align4();
        for (int32 i = 0; i < level->itemsCount; i++)
        {
            const TR1_PC::Item* item = level->items + i;
            const TR1_PC::Room* room = level->rooms + item->roomIndex;

            Item comp;
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
    }

    void writeCameraFrames(FileStream &f, Header &header, TR1_PC* level)
    {
        header.cameraFrames = f.align4();
        f.writeObj(level->cameraFrames, level->cameraFramesCount);
    }

    void convertGBA(FileStream &f, TR1_PC* pc)
    {
        pc->fixHeadMask();

        Header header;
        f.seek(sizeof(Header)); // will be rewritten at the end

        header.magic = 0x20414247;
        header.tilesCount = pc->tilesCount;
        header.roomsCount = pc->roomsCount;
        header.modelsCount = pc->modelsCount;
        header.meshesCount = pc->meshOffsetsCount;
        header.staticMeshesCount = pc->staticMeshesCount;
        header.spriteSequencesCount = pc->spriteSequencesCount;
        header.soundSourcesCount = pc->soundSourcesCount;
        header.boxesCount = pc->boxesCount;
        header.texturesCount = pc->objectTexturesCount;
        header.spritesCount = pc->spriteTexturesCount;
        header.itemsCount = pc->itemsCount;
        header.camerasCount = pc->camerasCount;
        header.cameraFramesCount = pc->cameraFramesCount;
        header.soundOffsetsCount = pc->soundOffsetsCount;

        writePalette(f, header, pc);
        writeLightmap(f, header, pc);

        header.tiles = f.align4();
        f.write((uint8*)pc->tiles, header.tilesCount * 256 * 256);

        writeRooms(f, header, pc);
        writeFloors(f, header, pc);

        header.meshData = f.align4();

        int32 mOffsets[2048];
        for (int32 i = 0; i < 2048; i++) {
            mOffsets[i] = -1;
        }

        for (int32 i = 0; i < pc->meshOffsetsCount; i++)
        {
            if (mOffsets[i] != -1)
                continue;

            mOffsets[i] = f.align4() - header.meshData;

            const uint8* ptr = (uint8*)pc->meshData + pc->meshOffsets[i];

            Mesh mesh;
            mesh.init(pc, ptr);
            mesh.write(f);

            for (int32 j = i + 1; j < pc->meshOffsetsCount; j++)
            {
                if (pc->meshOffsets[i] == pc->meshOffsets[j])
                {
                    mOffsets[j] = mOffsets[i];
                }
            }
        }

        header.meshOffsets = f.align4();
        f.write(mOffsets, pc->meshOffsetsCount);

        header.anims = f.align4();
        f.writeObj(pc->anims, pc->animsCount);

        header.states = f.align4();
        for (int32 i = 0; i < pc->statesCount; i++)
        {
            const TR1_PC::AnimState* state = pc->states + i;

            AnimState comp;
            comp.state = uint8(state->state);
            comp.rangesCount = uint8(state->rangesCount);
            comp.rangesStart = state->rangesStart;

            comp.write(f);
        }

        header.ranges = f.align4();
        f.writeObj(pc->ranges, pc->rangesCount);

        header.commands = f.align4();
        f.write(pc->commands, pc->commandsCount);

        header.nodes = f.align4();
        for (int32 i = 0; i < pc->nodesDataSize / 4; i++)
        {
            const TR1_PC::Node* node = (TR1_PC::Node*)(pc->nodesData + i * 4);

            ASSERT(node->pos.x > -32768);
            ASSERT(node->pos.x <  32767);
            ASSERT(node->pos.y > -32768);
            ASSERT(node->pos.y <  32767);
            ASSERT(node->pos.z > -32768);
            ASSERT(node->pos.z <  32767);
            ASSERT(node->flags < 0xFFFF);

            Node comp;
            comp.flags = uint16(node->flags);
            comp.pos.x = int16(node->pos.x);
            comp.pos.y = int16(node->pos.y);
            comp.pos.z = int16(node->pos.z);

            comp.write(f);
        }

        header.frameData = f.align4();
        f.write(pc->frameData, pc->frameDataSize);

        header.models = f.align4();
        for (int32 i = 0; i < pc->modelsCount; i++)
        {
            const TR1_PC::Model* model = pc->models + i;

            Model comp;
            comp.type = uint8(model->type);
            comp.count = uint8(model->count);
            comp.start = model->start;
            comp.nodeIndex = model->nodeIndex / 4;
            comp.animIndex = model->animIndex;

            comp.write(f);
        }

        writeStaticMeshes(f, header, pc);

        header.objectTextures = f.align4();
        for (int32 i = 0; i < pc->objectTexturesCount; i++)
        {
            const TR1_PC::ObjectTexture* objectTexture = pc->objectTextures + i;

            ObjectTexture comp;
            comp.tile = (objectTexture->tile & 0x3FFF) << 16;
            uint32 uv0 = ((objectTexture->uv0 << 16) | (objectTexture->uv0 >> 16)) & 0xFF00FF00;
            uint32 uv1 = ((objectTexture->uv1 << 16) | (objectTexture->uv1 >> 16)) & 0xFF00FF00;
            uint32 uv2 = ((objectTexture->uv2 << 16) | (objectTexture->uv2 >> 16)) & 0xFF00FF00;
            uint32 uv3 = ((objectTexture->uv3 << 16) | (objectTexture->uv3 >> 16)) & 0xFF00FF00;
            comp.uv01 = uv0 | (uv1 >> 8);
            comp.uv23 = uv2 | (uv3 >> 8);

            // GBA rasterizer doesn't support UV deltas over 127
            fixObjectTexture(comp, i);

            comp.write(f);
        }

        header.spriteTextures = f.align4();
        for (int32 i = 0; i < pc->spriteTexturesCount; i++)
        {
            const TR1_PC::SpriteTexture* spriteTexture = pc->spriteTextures + i;

            SpriteTexture comp;
            comp.tile = spriteTexture->tile << 16;
            uint32 u = spriteTexture->u;
            uint32 v = spriteTexture->v;
            uint32 w = (spriteTexture->w + 255) >> 8;
            uint32 h = (spriteTexture->h + 255) >> 8;
            comp.uwvh = (u << 24) | (w << 16) | (v << 8) | h;
            comp.l = spriteTexture->l;
            comp.t = spriteTexture->t;
            comp.r = spriteTexture->r;
            comp.b = spriteTexture->b;

            comp.write(f);
        }

        header.spriteSequences = f.align4();
        f.writeObj(pc->spriteSequences, pc->spriteSequencesCount);

        writeCameras(f, header, pc);
        writeSoundSources(f, header, pc);
        writeBoxes(f, header, pc);
        writeOverlaps(f, header, pc);
        writeZones(f, header, pc);
        writeAnimTex(f, header, pc);
        writeItems(f, header, pc);
        writeCameraFrames(f, header, pc);

        //f.writeArray(demoData, demoDataSize);

        for (int32 i = 0; i < pc->soundOffsetsCount; i++)
        {
            uint8* ptr = pc->soundData + pc->soundOffsets[i];
            int32 size = *(int32*)(ptr + 40);
            uint8* src = ptr + 44;
            uint8* dst = ptr;

            while ((dst - pc->soundData) % 4 != 0) {
                dst++;
            }
            dst += 4;

            for (int32 j = 0; j < size; j++)
            {
                dst[j] = src[j];
            }

            while ((size % 4) != 0)
            {
                dst[size] = dst[size - 1];
                size++;
            }

            dst -= 4;
            *(int32*)dst = size;

            pc->soundOffsets[i] = dst - pc->soundData;
        }

        header.soundMap = f.align4();
        f.write(pc->soundMap, 256);

        header.soundInfos = f.align4();
        f.writeObj(pc->soundInfo, pc->soundInfoCount);

        header.soundData = f.align4();
        f.write(pc->soundData, pc->soundDataSize);

        header.soundOffsets = f.align4();
        f.write(pc->soundOffsets, pc->soundOffsetsCount);

        f.setPos(0);
        header.write(f);
    }

    void convertTracks(FileStream &f, const char* from)
    {
        char buf[256];
        sprintf(buf, "%s/*.ima", from);

        WIN32_FIND_DATA fd;
        HANDLE h = FindFirstFile(buf, &fd);

        if (h == INVALID_HANDLE_VALUE)
            return;

        struct Track {
            int32 size;
            char* data;
        };
        Track tracks[MAX_TRACKS];
        memset(tracks, 0, sizeof(tracks));

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
                    strcpy(buf, from);
                    strcat(buf, "/");
                    strcat(buf, fd.cFileName);

                    FILE* f = fopen(buf, "rb");

                    if (!f)
                        continue;

                    fseek(f, 0, SEEK_END);
                    int32 size = ftell(f);
                    fseek(f, 0, SEEK_SET);
                    tracks[index].data = new char[size + 4];
                    fread(tracks[index].data, 1, size, f);
                    fclose(f);

                    tracks[index].size = ALIGN(*((int32*)tracks[index].data + 2), 4) - 4;

                    ASSERT(tracks[index].size % 4 == 0);
                }
            }
        }
        while (FindNextFile(h, &fd));
        FindClose(h);

        int32 offset = MAX_TRACKS * (4 + 4);

        for (int32 i = 0; i < MAX_TRACKS; i++)
        {
            if (tracks[i].size == 0) {
                int32 zero = 0;
                f.write(zero);
            } else {
                f.write(offset);
            }
            f.write(tracks[i].size);
            offset += tracks[i].size;
        }

        for (int32 i = 0; i < MAX_TRACKS; i++)
        {
            if (tracks[i].size == 0)
                continue;
            f.write((uint8*)tracks[i].data + 16, tracks[i].size);
            delete[] tracks[i].data;
        }
    }

    void convertScreen(const char* dir, const char* name, const TR1_PC::Palette &pal)
    {
        char path[256];
        sprintf(path, "screens/%s.bmp", name);

        int32 width, height, bpp;
        uint32* data = (uint32*)loadBitmap(path, &width, &height, &bpp);

        ASSERT(data);
        ASSERT(width == 240 && height == 160 && bpp == 32);

        uint32* uniqueColors = new uint32[width * height];
        uint32 count = 0;

        for (int32 i = 0; i < width * height; i++)
        {
            uint32 c = data[i];

            int32 index = -1;

            for (int32 j = 0; j < count; j++)
            {
                if (uniqueColors[j] == c) {
                    index = j;
                    break;
                }
            }

            if (index == -1) {
                index = count++;
                uniqueColors[index] = c;
            }

            data[i] = index;
        }

        for (int32 i = 0; i < count; i++)
        {
            uint32 c = uniqueColors[i];

            int32 cr = (c >> 16) & 0xFF;
            int32 cg = (c >> 8) & 0xFF;
            int32 cb = c & 0xFF;

            float dist = 256 * 256 * 256;
            int32 index = 0;

            for (int32 j = 0; j < 256; j++)
            {
                int32 r = pal.colors[j * 3 + 0] << 2;
                int32 g = pal.colors[j * 3 + 1] << 2;
                int32 b = pal.colors[j * 3 + 2] << 2;

                float d = sqrtf(float(SQR(cr - r) + SQR(cg - g) + SQR(cb - b)));
                if (d < dist)
                {
                    dist = d;
                    index = j;
                }
            }
    
            uniqueColors[i] = index;
        }

        uint8* indices = new uint8[width * height];
        for (int32 i = 0; i < width * height; i++)
        {
            indices[i] = uniqueColors[data[i]];
        }

        sprintf(path, "%s/%s.SCR", dir, name);

        FILE *f = fopen(path, "wb");
        fwrite(indices, 1, width * height, f);
        fclose(f);

        delete[] data;
        delete[] uniqueColors;
        delete[] indices;
    }

    void convertWAD(FileStream &f, TR1_PC** pc, TR1_PSX** psx)
    {
        TR1_PC* level;
        int32 id;

        #define LEVELS_LOOP() for (id = 0, level = pc[id]; id < LVL_MAX; id++, level = pc[id])

        Header headers[LVL_MAX];
        f.seek(sizeof(headers));

    // audio tracks
        convertTracks(f, "tracks/conv");

    // collect unique textures
        LEVELS_LOOP()
        {
            addTextures(level);
        }
        packTiles(f);

        printf("textures: %d\n", textures.count);

    // collect unique meshes
        LEVELS_LOOP()
        {
            ASSERT(level->meshOffsetsCount < MAX_MESHES);

            for (int32 i = 0; i < level->meshOffsetsCount; i++)
            {
                const uint8* ptr = (uint8*)level->meshData + level->meshOffsets[i];

                Mesh mesh;
                mesh.init(level, ptr);
                meshesRemap[id][i] = addMesh(mesh);
            }
        }

    // collect unique animations
        LEVELS_LOOP()
        {
            ASSERT(level->animsCount < MAX_ANIMS);

            for (int32 i = 0; i < level->modelsCount; i++)
            {
                const TR1_PC::Model &curModel = level->models[i];
                if (curModel.animIndex == 0xFFFF)
                    continue;

                uint8* startPtr = (uint8*)&level->frameData[level->anims[curModel.animIndex].frameOffset >> 1];
                uint8* endPtr = (uint8*)&level->frameData[level->frameDataSize];
                
                for (int32 j = i + 1; j < level->modelsCount; j++)
                {
                    const TR1_PC::Model &nextModel = level->models[j];
                    if (nextModel.animIndex == 0xFFFF)
                        continue;

                    endPtr = (uint8*)&level->frameData[level->anims[nextModel.animIndex].frameOffset >> 1];
                    break;
                }

                AnimFrames af;
                af.init(startPtr, endPtr);
                animFramesRemap[id][i] = addAnimFrames(af);
            }
        }

    // collect unique models
        LEVELS_LOOP()
        {
            for (int32 i = 0; i < level->modelsCount; i++)
            {
                Model model;
                model.init(level, level->models[i]);
                modelsRemap[id][i] = addModel(model);
            }
        }

    // collect unique object textures
        LEVELS_LOOP()
        {
            // TODO
        }

        printf("Meshes: %d\n", meshesCount);
        printf("Models: %d\n", modelsCount);
        printf("Animations: %d\n", animFramesCount);

    // write meshes
        //header.meshes = f.align4();
        for (int32 i = 0; i < meshesCount; i++)
        {
            meshes[i].offset = f.align4();
            meshes[i].write(f);
        }

    // write models
        //header.models = f.align4();
        for (int32 i = 0; i < modelsCount; i++)
        {
            models[i].write(f);
        }

    // write anims
        //header.frames = f.align4();
        for (int32 i = 0; i < animFramesCount; i++)
        {
            animFrames[i].write(f);
        }

    // palette
        LEVELS_LOOP()
        {
            headers[id].palette = f.align4();
            writePalette(f, headers[id], level);
        }

    // lightmaps
        LEVELS_LOOP()
        {
            headers[id].lightmap = f.align4();
            writeLightmap(f, headers[id], level);
        }

    // rooms
        LEVELS_LOOP()
        {
            writeRooms(f, headers[id], level);
        }

    // floors data
        LEVELS_LOOP()
        {
            writeFloors(f, headers[id], level);
        }

    // mesh offsets
        LEVELS_LOOP()
        {
            headers[id].meshesCount = level->meshOffsetsCount;
            headers[id].meshOffsets = f.align4();
            for (int32 i = 0; i < level->meshOffsetsCount; i++)
            {
                f.write(meshes[meshesRemap[id][i]].offset);
            }
        }

/*
        LEVELS_LOOP()
        {
            //header.anims = f.align4();
            f.writeObj(level->anims, level->animsCount);

            //header.states = f.align4();
            for (int32 i = 0; i < level->statesCount; i++)
            {
                const TR1_PC::AnimState* state = level->states + i;

                AnimState comp;
                comp.state = uint8(state->state);
                comp.rangesCount = uint8(state->rangesCount);
                comp.rangesStart = state->rangesStart;

                comp.write(f);
            }

            //header.ranges = f.align4();
            f.writeObj(level->ranges, level->rangesCount);

            //header.commands = f.align4();
            f.write(level->commands, level->commandsCount);

            //header.nodes = f.align4();
            for (int32 i = 0; i < level->nodesDataSize / 4; i++)
            {
                const TR1_PC::Node* node = (TR1_PC::Node*)(level->nodesData + i * 4);

                ASSERT(node->pos.x > -32768);
                ASSERT(node->pos.x <  32767);
                ASSERT(node->pos.y > -32768);
                ASSERT(node->pos.y <  32767);
                ASSERT(node->pos.z > -32768);
                ASSERT(node->pos.z <  32767);
                ASSERT(node->flags < 0xFFFF);

                Node comp;
                comp.flags = uint16(node->flags);
                comp.pos.x = int16(node->pos.x);
                comp.pos.y = int16(node->pos.y);
                comp.pos.z = int16(node->pos.z);

                comp.write(f);
            }

            //header.frameData = f.align4();
            f.write(level->frameData, level->frameDataSize);

            //header.models = f.align4();
            for (int32 i = 0; i < level->modelsCount; i++)
            {
                const TR1_PC::Model* model = level->models + i;

                Model comp;
                comp.type = uint8(model->type);
                comp.count = uint8(model->count);
                comp.start = model->start;
                comp.nodeIndex = model->nodeIndex / 4;
                comp.animIndex = model->animIndex;

                comp.write(f);
            }
        }
*/
        LEVELS_LOOP()
        {
            writeStaticMeshes(f, headers[id], level);
        }
    /*
        header.objectTextures = f.align4();
        for (int32 i = 0; i < pc->objectTexturesCount; i++)
        {
            const TR1_PC::ObjectTexture* objectTexture = pc->objectTextures + i;

            ObjectTexture comp;
            comp.attribute = objectTexture->attribute;
            comp.tile = objectTexture->tile & 0x3FFF;
            comp.uv0 = ((objectTexture->uv0 << 16) | (objectTexture->uv0 >> 16)) & 0xFF00FF00;
            comp.uv1 = ((objectTexture->uv1 << 16) | (objectTexture->uv1 >> 16)) & 0xFF00FF00;
            comp.uv2 = ((objectTexture->uv2 << 16) | (objectTexture->uv2 >> 16)) & 0xFF00FF00;
            comp.uv3 = ((objectTexture->uv3 << 16) | (objectTexture->uv3 >> 16)) & 0xFF00FF00;

            // GBA rasterizer doesn't support UV deltas over 127, due performance reason, so we clamp it
            fixObjectTexture(comp, i);

            comp.write(f);
        }

        header.spriteTextures = f.align4();
        for (int32 i = 0; i < pc->spriteTexturesCount; i++)
        {
            const TR1_PC::SpriteTexture* spriteTexture = pc->spriteTextures + i;

            SpriteTexture comp;
            comp.tile = spriteTexture->tile;
            comp.u = spriteTexture->u;
            comp.v = spriteTexture->v;
            comp.w = spriteTexture->w >> 8;
            comp.h = spriteTexture->h >> 8;
            comp.l = spriteTexture->l;
            comp.t = spriteTexture->t;
            comp.r = spriteTexture->r;
            comp.b = spriteTexture->b;

            comp.write(f);
        }

        header.spriteSequences = f.align4();
        f.writeObj(pc->spriteSequences, pc->spriteSequencesCount);
        */

    // fixed cameras
        LEVELS_LOOP()
        {
            writeCameras(f, headers[id], level);
        }
        
    // static sounds
        LEVELS_LOOP()
        {
            writeSoundSources(f, headers[id], level);
        }

    // boxes
        LEVELS_LOOP()
        {
            writeBoxes(f, headers[id], level);
        }

    // overlaps
        LEVELS_LOOP()
        {
            writeOverlaps(f, headers[id], level);
        }

    // zones
        LEVELS_LOOP()
        {
            writeZones(f, headers[id], level);
        }

    // animated textures
        LEVELS_LOOP()
        {
            writeAnimTex(f, headers[id], level);
        }

    // items
        LEVELS_LOOP()
        {
            writeItems(f, headers[id], level);
        }

    // animated camera frames
        LEVELS_LOOP()
        {
            writeCameraFrames(f, headers[id], level);
        }

    /*
        for (int32 i = 0; i < pc->soundOffsetsCount; i++)
        {
            uint8* ptr = pc->soundData + pc->soundOffsets[i];
            int32 size = *(int32*)(ptr + 40);
            uint8* src = ptr + 44;
            uint8* dst = ptr;

            while ((dst - pc->soundData) % 4 != 0) {
                dst++;
            }
            dst += 4;

            for (int32 j = 0; j < size; j++)
            {
                dst[j] = src[j];
            }

            while ((size % 4) != 0)
            {
                dst[size] = dst[size - 1];
                size++;
            }

            dst -= 4;
            *(int32*)dst = size;

            pc->soundOffsets[i] = dst - pc->soundData;
        }

        header.soundMap = f.align4();
        f.write(pc->soundMap, 256);

        header.soundInfos = f.align4();
        f.writeObj(pc->soundInfo, pc->soundInfoCount);

        header.soundData = f.align4();
        f.write(pc->soundData, pc->soundDataSize);

        header.soundOffsets = f.align4();
        f.write(pc->soundOffsets, pc->soundOffsetsCount);

        f.setPos(0);
        header.write(f);
    */
        f.writeRaw(headers);
    }

    //#define GBA_WAD

    void process(const char* dir, TR1_PC** pc, TR1_PSX** psx)
    {
        roomVerticesCount = 0;
        roomVertices = new RoomVertex[MAX_ROOM_VERTICES];

        char buf[256];
    #ifdef GBA_WAD
        sprintf(buf, "%s/TR1.WAD", dir, pc, psx);
        FileStream f(buf, true);
            
        if (!f.isValid()) {
            printf("can't save \"%s\"\n", buf);
            return;
        }

        meshes = new Mesh[LVL_MAX * MAX_MESHES];
        meshesCount = 0;
        models = new Model[LVL_MAX * MAX_MODELS];
        modelsCount = 0;
        animFrames = new AnimFrames[LVL_MAX * MAX_ANIMS];
        animFramesCount = 0;

        convertWAD(f, pc, psx);

        delete[] meshes;
        delete[] models;
        delete[] animFrames;
    #else
        for (int32 i = 0; i < LVL_MAX; i++)
        {
            sprintf(buf, "%s/%s.PKD", dir, levelNames[i]);
            FileStream f(buf, true);
            
            if (!f.isValid()) {
                printf("can't save \"%s\"\n", buf);
                continue;
            }

            convertGBA(f, pc[i]);
        }

        // title screen
        convertScreen(dir, "TITLE", pc[LVL_TR1_TITLE]->palette);

        // audio tracks
        {
            sprintf(buf, "%s/TRACKS.IMA", dir);
            FileStream f(buf, true);
            convertTracks(f, "tracks/conv_demo");
        }
    #endif

        delete[] roomVertices;
    }
};

#endif