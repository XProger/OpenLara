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
    };

    struct Remap {
        int32 meshes[MAX_MESHES];
        int32 models[MAX_MODELS];
        int32 animFrames[MAX_ANIMS];
        int16 textures[MAX_TEXTURES];
        int16 sprites[MAX_TEXTURES];
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
        int8 indices[4];
        uint16 flags;
        uint16 unused;

        void write(FileStream &f) const
        {
            f.write(indices[0]);
            f.write(indices[1]);
            f.write(indices[2]);
            f.write(indices[3]);
            f.write(flags);

            uint16 padding = 0;
            f.write(padding);
        }
    };

    struct RoomTriangle
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

    struct MeshQuad
    {
        int8 indices[4];
        uint16 flags;
        uint16 unused;

        void write(FileStream &f) const
        {
            f.write(indices[0]);
            f.write(indices[1]);
            f.write(indices[2]);
            f.write(indices[3]);
            f.write(flags);

            uint16 padding = 0;
            f.write(padding);
        }
    };

    struct MeshTriangle
    {
        int8 indices[4];
        uint16 flags;
        uint16 unused;

        void write(FileStream &f) const
        {
            f.write(indices[0]);
            f.write(indices[1]);
            f.write(indices[2]);
            f.write(indices[3]);
            f.write(flags);

            uint16 padding = 0;
            f.write(padding);
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

        ObjectTexture(const TR1_PC::ObjectTexture* tex)
        {
            tile = (tex->tile & 0x3FFF) << 16;
            uint32 uv0 = ((tex->uv0 << 16) | (tex->uv0 >> 16)) & 0xFF00FF00;
            uint32 uv1 = ((tex->uv1 << 16) | (tex->uv1 >> 16)) & 0xFF00FF00;
            uint32 uv2 = ((tex->uv2 << 16) | (tex->uv2 >> 16)) & 0xFF00FF00;
            uint32 uv3 = ((tex->uv3 << 16) | (tex->uv3 >> 16)) & 0xFF00FF00;

            fixTexCoord(uv0, uv1);
            fixTexCoord(uv0, uv3);
            fixTexCoord(uv1, uv2);

            uv01 = uv0 | (uv1 >> 8);
            uv23 = uv2 | (uv3 >> 8);
        }

        bool isEqual(const ObjectTexture* tex)
        {
            return memcmp(this, tex, sizeof(*tex)) == 0;
        }

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

        SpriteTexture(const TR1_PC::SpriteTexture* spr)
        {
            uint32 u = spr->u;
            uint32 v = spr->v;
            uint32 w = (spr->w + 255) >> 8;
            uint32 h = (spr->h + 255) >> 8;
            tile = spr->tile << 16;
            uwvh = (u << 24) | (w << 16) | (v << 8) | h;
            l = spr->l;
            t = spr->t;
            r = spr->r;
            b = spr->b;
        }

        bool isEqual(const SpriteTexture* spr)
        {
            return memcmp(this, spr, sizeof(*spr)) == 0;
        }

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
        uint8 vCount;
        uint8 hasNormals;
        int16 rCount;
        int16 crCount;
        int16 tCount;
        int16 ctCount;
        const vec3s* vertices;
        const TR1_PC::Quad* rFaces;
        const TR1_PC::Quad* crFaces;
        const TR1_PC::Triangle* tFaces;
        const TR1_PC::Triangle* ctFaces;

        Mesh(TR1_PC* level, const uint8* ptr)
        {
            this->level = level;

            center = *(vec3s*)ptr; ptr += sizeof(center);
            radius = *(int16*)ptr; ptr += sizeof(radius);
            uint16 flags = *(uint16*)ptr; ptr += sizeof(flags);

            vCount = (uint8)*(int16*)ptr; ptr += 2;
            vertices = (vec3s*)ptr;
            ptr += vCount * sizeof(vec3s);

            const uint16* vIntensity = NULL;
            const vec3s* vNormal = NULL;

            int16 nCount = *(int16*)ptr; ptr += 2;
            //const int16* normals = (int16*)ptr;
            if (nCount > 0) { // normals
                vNormal = (vec3s*)ptr;
                ptr += nCount * 3 * sizeof(int16);
                hasNormals = 1;
            } else { // intensity
                vIntensity = (uint16*)ptr;
                ptr += vCount * sizeof(uint16);
                hasNormals = 0;
            }

            hasNormals = 0; // don't use dynamic per-vertex lighting on GBA

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

        void write(FileStream &f, const Remap *remap) const
        {
            ASSERT(vCount < 256);

        #if 0 // TODO proper center/radius calc?
            vec3s bmin = vertices[0];
            vec3s bmax = bmin;

            for (int32 j = 0; j < vCount; j++)
            {
                const vec3s* v = vertices + j;
                if (v->x < bmin.x) bmin.x = v->x;
                if (v->y < bmin.y) bmin.y = v->y;
                if (v->z < bmin.z) bmin.z = v->z;
                if (v->x > bmax.x) bmax.x = v->x;
                if (v->y > bmax.y) bmax.y = v->y;
                if (v->z > bmax.z) bmax.z = v->z;
            }
        #endif

            f.write(center.x);
            f.write(center.y);
            f.write(center.z);
            f.write(radius);
            f.write(intensity);
            f.write(vCount);
            f.write(hasNormals);
            f.write(int16(rCount + crCount));
            f.write(int16(tCount + ctCount));
            f.write(int16(0));
            f.write(int16(0));


            Array<const TR1_PC::Quad> sortedQuads;

            for (int32 j = 0; j < rCount; j++)
            {
                sortedQuads.add(rFaces + j);
            }

            for (int32 j = 0; j < crCount; j++)
            {
                sortedQuads.add(crFaces + j);
            }

            sortedQuads.sort(); // sort quads by indices for consistent deltas

            int32 prev = 0;

            for (int32 j = 0; j < sortedQuads.count; j++)
            {
                const TR1_PC::Quad* q = sortedQuads[j];

                int32 i0 = q->indices[0];
                int32 i1 = q->indices[1];
                int32 i2 = q->indices[2];
                int32 i3 = q->indices[3];

                int32 p0 = i0 - prev;
                int32 p1 = i1 - i0;
                int32 p2 = i2 - i1;
                int32 p3 = i3 - i2;
                prev = i3;

                ASSERT(p0 >= -128 && p0 <= 127);
                ASSERT(p1 >= -128 && p1 <= 127);
                ASSERT(p2 >= -128 && p2 <= 127);
                ASSERT(p3 >= -128 && p3 <= 127);

                MeshQuad comp;
                comp.indices[0] = p0;
                comp.indices[1] = p1;
                comp.indices[2] = p2;
                comp.indices[3] = p3;

                bool textured = (q >= rFaces) && (q < (rFaces + rCount));

                if (textured)
                {
                    uint16 texIndex = q->flags & FACE_TEXTURE;
                    comp.flags = remap ? remap->textures[texIndex] : texIndex;
                    if (level->objectTextures[texIndex].attribute & TEX_ATTR_AKILL)
                    {
                        comp.flags |= (FACE_TYPE_FTA << FACE_TYPE_SHIFT);
                    }
                    else
                    {
                        comp.flags |= (FACE_TYPE_FT << FACE_TYPE_SHIFT);
                    }
                }
                else
                {
                    comp.flags = q->flags & FACE_TEXTURE;
                    comp.flags |= (FACE_TYPE_F << FACE_TYPE_SHIFT);
                }

                comp.write(f);
            }

            prev = 0;
            for (int32 j = 0; j < tCount; j++)
            {
                TR1_PC::Triangle t = tFaces[j];
                uint16 texIndex = t.flags & FACE_TEXTURE;

                int32 i0 = t.indices[0];
                int32 i1 = t.indices[1];
                int32 i2 = t.indices[2];

                int32 p0 = i0 - prev;
                int32 p1 = i1 - i0;
                int32 p2 = i2 - i1;
                prev = i2;

                ASSERT(p0 >= -128 && p0 <= 127);
                ASSERT(p1 >= -128 && p1 <= 127);
                ASSERT(p2 >= -128 && p2 <= 127);

                MeshTriangle comp;
                comp.indices[0] = p0;
                comp.indices[1] = p1;
                comp.indices[2] = 0;    // asr hack
                comp.indices[3] = p2;

                comp.flags = remap ? remap->textures[texIndex] : texIndex;
                if (level->objectTextures[texIndex].attribute & TEX_ATTR_AKILL) {
                    comp.flags |= (FACE_TYPE_FTA << FACE_TYPE_SHIFT);
                } else {
                    comp.flags |= (FACE_TYPE_FT << FACE_TYPE_SHIFT);
                }

                comp.write(f);
            }

            for (int32 j = 0; j < ctCount; j++)
            {
                TR1_PC::Triangle t = ctFaces[j];

                int32 i0 = t.indices[0];
                int32 i1 = t.indices[1];
                int32 i2 = t.indices[2];

                int32 p0 = i0 - prev;
                int32 p1 = i1 - i0;
                int32 p2 = i2 - i1;
                prev = i2;

                ASSERT(p0 >= -128 && p0 <= 127);
                ASSERT(p1 >= -128 && p1 <= 127);
                ASSERT(p2 >= -128 && p2 <= 127);

                MeshTriangle comp;
                comp.indices[0] = p0;
                comp.indices[1] = p1;
                comp.indices[2] = 0;    // asr hack
                comp.indices[3] = p2;

                comp.flags = t.flags & FACE_TEXTURE;
                comp.flags |= (FACE_TYPE_F << FACE_TYPE_SHIFT);

                comp.write(f);
            }

            for (int32 j = 0; j < vCount; j++)
            {
                struct MeshVertexGBA {
                    int16 x, y, z;
                } v;

                v.x = vertices[j].x >> 2;
                v.y = vertices[j].y >> 2;
                v.z = vertices[j].z >> 2;

                f.write(v.x);
                f.write(v.y);
                f.write(v.z);
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

    struct Animation {
        TR1_PC::Animation anim;

    };

    struct AnimFrames
    {
        const uint8* data;
        uint32 size;

        AnimFrames(const uint8* startPtr, const uint8* endPtr)
        {
            data = startPtr;
            size = uint32(endPtr - startPtr);
        }

        void write(FileStream &f) const
        {
            f.write(data, size);
        }
    };

    struct Texture
    {
        const TR1_PC* level;
        TR1_PC::ObjectTexture* tex;
        TR1_PC::SpriteTexture* spr;
        Texture* link;
        int32 uid;
        int32 width;
        int32 height;
        int32 akill;
        uint8* indices;
        uint32 indexSum;
        int32 tile;
        int32 x, y;
        int32 minX, minY, maxX, maxY;

        Texture(const TR1_PC* level, TR1_PC::ObjectTexture* tex, int32 uid)
        {
            this->level = level;
            this->tex = tex;
            this->spr = NULL;
            this->link = NULL;
            this->uid = uid;
            this->tile = -1;

            TR1_PC::Tile *tile = level->tiles + (tex->tile & 0x3FFF);

            minX = MIN(MIN(tex->x0, tex->x1), tex->x2);
            minY = MIN(MIN(tex->y0, tex->y1), tex->y2);
            maxX = MAX(MAX(tex->x0, tex->x1), tex->x2);
            maxY = MAX(MAX(tex->y0, tex->y1), tex->y2);

            if (tex->isQuad)
            {
                minX = MIN(minX, tex->x3);
                minY = MIN(minY, tex->y3);
                maxX = MAX(maxX, tex->x3);
                maxY = MAX(maxY, tex->y3);
            }

            width = maxX - minX + 1;
            height = maxY - minY + 1;
            akill = 0;

            bool transp = (tex->attribute & TEX_ATTR_AKILL);

            indices = new uint8[width * height];
            const uint8* src = tile->indices + 256 * minY;
            uint8* dst = indices;

            for (int32 y = minY; y <= maxY; y++)
            {
                for (int32 x = minX; x <= maxX; x++)
                {
                    uint8 index = src[x];
                    *dst++ = index;

                    if ((index == 0) && transp) {
                        akill++;
                    }
                }
                src += 256;
            }
        }

        Texture(const TR1_PC* level, TR1_PC::SpriteTexture* spr, int32 uid)
        {
            this->level = level;
            this->uid = uid;
            this->tex = NULL;
            this->spr = spr;
            this->link = NULL;
            this->tile = -1;

            TR1_PC::Tile *tile = level->tiles + (spr->tile & 0x3FF);

            minX = spr->u;
            minY = spr->v;
            maxX = spr->u + (spr->w >> 8);
            maxY = spr->v + (spr->h >> 8);

            width = maxX - minX + 1;
            height = maxY - minY + 1;
            akill = 0;
            indexSum = 0;

            indices = new uint8[width * height];
            const uint8* src = tile->indices + 256 * minY;
            uint8* dst = indices;

            for (int32 y = minY; y <= maxY; y++)
            {
                for (int32 x = minX; x <= maxX; x++)
                {
                    uint8 index = src[x];
                    *dst++ = index;

                    indexSum += index;

                    if (index == 0) {
                        akill++;
                    }
                }
                src += 256;
            }
        }

        ~Texture()
        {
            delete[] indices;
        }

        static int cmp(const Texture* a, const Texture* b)
        {
            int32 animA = a->tex ? a->tex->attribute & TEX_ATTR_ANIM : 0;
            int32 animB = b->tex ? b->tex->attribute & TEX_ATTR_ANIM : 0;
            int32 i = animB - animA;

            if (i == 0)
            {
                i = b->akill - a->akill;
            }

            if (i == 0)
            {
                int32 max1 = MAX(a->width, a->height);
                int32 max2 = MAX(b->width, b->height);
                i = max2 - max1;
            }

            if (i == 0) {
                i = int32(a->level - b->level);
            }

            if (i == 0) {
                i = a->uid - b->uid;
            }

            return i;
        }
    };

    struct Atlas
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

                if (nw == tw && nh == th)
                {
                    this->tex = tex;
                    tex->x = l;
                    tex->y = t;
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
                const TR1_PC::Palette &pal = tex->level->palette;

                uint8* index = tex->indices;

                uint8* dst = data + (t * 256 + l) * 3;

                for (int32 y = 0; y < tex->height; y++)
                {
                    for (int32 x = 0; x < tex->width; x++)
                    {
                        if ((*index == 0) && tex->akill) {
                            dst[x * 3 + 0] = 255;
                            dst[x * 3 + 1] = 0;
                            dst[x * 3 + 2] = 255;
                        } else {
                            dst[x * 3 + 0] = pal.colors[*index * 3 + 2] << 2;
                            dst[x * 3 + 1] = pal.colors[*index * 3 + 1] << 2;
                            dst[x * 3 + 2] = pal.colors[*index * 3 + 0] << 2;
                        }
                        *index++;
                    }
                    dst += 256 * 3;
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

        Atlas()
        {
            root = new Node(0, 0, 256, 256);
        }

        ~Atlas()
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

        static int cmp(const Atlas* a, const Atlas* b)
        {
            return 0;
        }
    };

    RoomVertex* roomVertices;
    int32 roomVerticesCount;

    Remap remaps[LVL_MAX];

    Array<Mesh> meshes;
    Array<Model> models;
    Array<AnimFrames> animFrames;
    Array<ObjectTexture> objectTextures;
    Array<SpriteTexture> spriteTextures;

    Array<Texture> textures;
    Array<Atlas> tiles;

    void addTextures(TR1_PC* level)
    {
    // check object textures usage
        bool* used = new bool[level->objectTexturesCount];
        memset(used, 0, sizeof(bool) * level->objectTexturesCount);

        for (int32 i = 0; i < level->roomsCount; i++)
        {
            TR1_PC::Room &room = level->rooms[i];

            for (int32 j = 0; j < room.qCount; j++)
            {
                used[room.quads[j].flags & FACE_TEXTURE] = true;
            }

            for (int32 j = 0; j < room.tCount; j++)
            {
                used[room.triangles[j].flags & FACE_TEXTURE] = true;
            }
        }

        for (int32 i = 0; i < level->meshOffsetsCount; i++)
        {
            const uint8* ptr = (uint8*)level->meshData + level->meshOffsets[i];

            Mesh mesh(level, ptr);

            for (int32 j = 0; j < mesh.rCount; j++)
            {
                used[mesh.rFaces[j].flags & FACE_TEXTURE] = true;
            }

            for (int32 j = 0; j < mesh.tCount; j++)
            {
                used[mesh.tFaces[j].flags & FACE_TEXTURE] = true;
            }
        }

        static int32 texUID = 0;

    // textures
        for (int32 i = 0; i < level->objectTexturesCount; i++)
        {
            if (!used[i]) continue;
            Texture* tex = new Texture(level, level->objectTextures + i, texUID++);
            textures.add(tex);
        }

    // sprites
        for (int32 i = 0; i < level->spriteTexturesCount; i++)
        {
            Texture* tex = new Texture(level, level->spriteTextures + i, texUID++);
            textures.add(tex);
        }

        static int32 maxSprites = 0;
        maxSprites += level->spriteTexturesCount;
        printf("%d\n", maxSprites);

        delete[] used;
    }

    void linkTextures()
    {
        textures.sort();

        for (int32 i = 0; i < textures.count; i++)
        {
            Texture* src = textures[i];
            ASSERT(src->link == NULL);

            for (int32 j = 0; j < i; j++)
            {
                Texture* dst = textures[j];

                if (dst->link) {
                    dst = dst->link;
                    ASSERT(dst->link == NULL);
                }

                if (src->tex) // is ObjectTexture
                {
                    if (src->width != dst->width || src->height > dst->height)
                        continue;

                    int32 dy = dst->height - src->height;

                    for (int32 y = 0; y <= dy; y++)
                    {
                        if (memcmp(dst->indices + dst->width * y, src->indices, src->width * src->height) == 0)
                        {
                            src->link = dst;
                            break;
                        }
                    }
                }
                else // is SpriteTexture
                {
                    if (src->width != dst->width || src->height != dst->height || src->indexSum != dst->indexSum)
                        continue;

                    if (memcmp(dst->indices, src->indices, src->width * src->height) == 0)
                    {
                        src->link = dst;
                        break;
                    }
                }
            }
        }
    }

    void packTiles(FileStream &f)
    {
        int32 texPacked = 0;

        for (int32 i = 0; i < textures.count; i++)
        {
            Texture* tex = textures[i];

            if (tex->link)
                continue;

            bool placed = false;

            for (int32 j = 0; j < tiles.count; j++)
            {
                if (tiles[j]->root->insert(tex))
                {
                    placed = true;
                    tex->tile = j;
                    break;
                }
            }

            if (!placed)
            {
                Atlas* tile = new Atlas();
                tex->tile = tiles.add(tile);
                placed = tile->root->insert(tex);
            }

            if (!placed)
            {
                tex->tile = -1;
                printf("Can't pack texture %d x %d", tex->width, tex->height);
                break;
            }

            if (placed) {
                texPacked++;
            }
        }

        printf("textures packed: %d\n", texPacked);

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

        f.write(data, 256 * 256 * tiles.count);

        delete[] data;
    }

    void remapTextures()
    {
        int32 animated = -1;
        int32 maxAnimatedTex = 0;

        for (int32 i = 0; i < textures.count; i++)
        {
            const Texture* texture = textures[i];
            const Texture* instance = texture->link ? texture->link : texture;

            int32 dx = instance->x - instance->minX;
            int32 dy = instance->y - instance->minY;

            ASSERT(instance->tile >= 0);

            Remap &remap = remaps[texture->level->id];

            if (texture->tex) // is ObjectTexture
            {
                if (texture->tex->attribute & TEX_ATTR_ANIM)
                {
                    ASSERT(animated == -1 || animated == 1);
                    animated = 1;
                } else {

                    // add dummy textures as padding between animated and static textures
                    int32 padding = MAX_ANIM_TEX - objectTextures.count;
                    for (int32 j = 0; j < padding; j++)
                    {
                        TR1_PC::ObjectTexture tmp;
                        memset(&tmp, 0, sizeof(tmp));
                        objectTextures.add(new ObjectTexture(&tmp));
                    }

                    animated = 0;
                }

                TR1_PC::ObjectTexture tmp;
                memset(&tmp, 0, sizeof(tmp));
                tmp.attribute = texture->tex->attribute; // old attribute
                tmp.tile = instance->tile; // new tile index
                tmp.x0 = texture->tex->x0 + dx;
                tmp.y0 = texture->tex->y0 + dy;
                tmp.x1 = texture->tex->x1 + dx;
                tmp.y1 = texture->tex->y1 + dy;
                tmp.x2 = texture->tex->x2 + dx;
                tmp.y2 = texture->tex->y2 + dy;
                if (texture->tex->isQuad)
                {
                    tmp.x3 = texture->tex->x3 + dx;
                    tmp.y3 = texture->tex->y3 + dy;
                }

                ObjectTexture* comp = new ObjectTexture(&tmp);

                int32 texIndex = int32(texture->tex - texture->level->objectTextures);

                int32 index = objectTextures.find(comp);
                if (index <= 0) {
                    index = objectTextures.add(comp);
                } else {
                    delete comp;
                }
                remap.textures[texIndex] = index;

                if (texture->tex->attribute & TEX_ATTR_ANIM) {
                    maxAnimatedTex = i;
                }
            }
            else // is SpriteTexture
            {
                ASSERT(animated == 0);

                TR1_PC::SpriteTexture tmp;
                memset(&tmp, 0, sizeof(tmp));
                tmp.tile = instance->spr->tile;
                tmp.u = texture->spr->u + dx;
                tmp.v = texture->spr->v + dy;
                tmp.w = texture->spr->w;
                tmp.h = texture->spr->h;
                tmp.l = texture->spr->l;
                tmp.t = texture->spr->t;
                tmp.r = texture->spr->r;
                tmp.b = texture->spr->b;

                SpriteTexture* comp = new SpriteTexture(&tmp);

                int32 sprIndex = int32(texture->spr - texture->level->spriteTextures);

                int32 index = spriteTextures.find(comp);
                if (index <= 0) {
                    index = spriteTextures.add(comp);
                } else {
                    delete comp;
                }
                remap.sprites[sprIndex] = index;
            }
        }
        
        printf("animated textures: %d\n", maxAnimatedTex + 1);

        ASSERT(maxAnimatedTex < MAX_ANIM_TEX);
    }

    int32 addRoomVertex(int32 yOffset, const TR1_PC::Room::Vertex &v)
    {
        RoomVertex comp;
        int32 px = v.pos.x >> 8;
        int32 py = (v.pos.y - yOffset) >> 8;
        int32 pz = v.pos.z >> 8;

        ASSERT(py >= 0);
        ASSERT(px < (32 << 2));
        ASSERT(py < 64);
        ASSERT(pz < (32 << 2));

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

    int32 addMesh(Mesh* mesh)
    {
        for (int32 i = 0; i < meshes.count; i++)
        {
            const Mesh* m = meshes[i];
            if (m->center.x != mesh->center.x ||
                m->center.y != mesh->center.y ||
                m->center.z != mesh->center.z ||
                m->radius != mesh->radius ||
                m->intensity != mesh->intensity || // TODO move to static mesh to save 5k
                m->vCount < mesh->vCount ||
                m->rCount < mesh->rCount ||
                m->crCount < mesh->crCount ||
                m->tCount < mesh->tCount ||
                m->ctCount < mesh->ctCount) continue;

            if (memcmp(m->vertices, mesh->vertices, mesh->vCount * sizeof(mesh->vertices[0])) != 0)
                continue;

            delete mesh;

            return i;
        }

        return meshes.add(mesh);
    }

    int32 addModel(const Model &model)
    {
        return 0;
    }

    int32 addAnimFrames(AnimFrames* anim)
    {
        for (int32 i = 0; i < animFrames.count; i++)
        {
            const AnimFrames* af = animFrames[i];

            if (af->size < anim->size)
                continue;

            if (memcmp(af->data, anim->data, anim->size) != 0)
                continue;

            delete anim;

            return i;
        }

        return animFrames.add(anim);
    }

    uint32 writePalette(FileStream &f, TR1_PC* level)
    {
        uint32 offset = f.align4();

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

        return offset;
    }

    uint32 writeLightmap(FileStream &f, TR1_PC* level)
    {
        uint32 offset = f.align4();
    #if 1
        for (int32 i = 0; i < 32; i++) {
            level->lightmap[i * 256] = 0;
        }
        f.write(level->lightmap, 256 * 32);
    #else
        uint8 lmap[256 * 32];
        for (int32 i = 0; i < 32; i++)
        {
            for (int32 j = 0; j < 256; j++)
            {
                lmap[j * 32 + i] = level->lightmap[i * 256 + j];
            }
        }
        f.write(lmap, 256 * 32);
    #endif
        return offset;
    }

    uint32 writeRooms(FileStream &f, TR1_PC* level, const Remap* remap)
    {
        uint32 offset = f.align4();

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

            int32 prev = 0;

            for (int32 i = 0; i < room->qCount; i++)
            {
                TR1_PC::Quad q = room->quads[i];
                uint16 texIndex = q.flags & FACE_TEXTURE;

                int32 i0 = addRoomVertex(info.yTop, room->vertices[q.indices[0]]);
                int32 i1 = addRoomVertex(info.yTop, room->vertices[q.indices[1]]);
                int32 i2 = addRoomVertex(info.yTop, room->vertices[q.indices[2]]);
                int32 i3 = addRoomVertex(info.yTop, room->vertices[q.indices[3]]);

                int32 p0 = i0 - prev;
                int32 p1 = i1 - i0;
                int32 p2 = i2 - i1;
                int32 p3 = i3 - i2;
                prev = i3;

                ASSERT(p0 >= -128 && p0 <= 127);
                ASSERT(p1 >= -128 && p1 <= 127);
                ASSERT(p2 >= -128 && p2 <= 127);
                ASSERT(p3 >= -128 && p3 <= 127);

                RoomQuad comp;
                comp.indices[0] = p0;
                comp.indices[1] = p1;
                comp.indices[2] = p2;
                comp.indices[3] = p3;

                comp.flags = remap ? remap->textures[texIndex] : texIndex;
                if (level->objectTextures[texIndex].attribute & TEX_ATTR_AKILL) {
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
                uint16 texIndex = t.flags & FACE_TEXTURE;

                RoomTriangle comp;
                comp.indices[0] = addRoomVertex(info.yTop, room->vertices[t.indices[0]]);
                comp.indices[1] = addRoomVertex(info.yTop, room->vertices[t.indices[1]]);
                comp.indices[2] = addRoomVertex(info.yTop, room->vertices[t.indices[2]]);
                comp.flags = remap ? remap->textures[texIndex] : texIndex;

                if (level->objectTextures[texIndex].attribute & TEX_ATTR_AKILL) {
                    comp.flags |= (FACE_TYPE_FTA << FACE_TYPE_SHIFT);
                } else {
                    comp.flags |= (FACE_TYPE_FT << FACE_TYPE_SHIFT);
                }

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
        f.setPos(offset);
        f.writeObj(infos, level->roomsCount);
        f.setPos(pos);

        return offset;
    }

    uint32 writeFloors(FileStream &f, TR1_PC* level)
    {
        uint32 offset = f.align4();
        
        f.writeObj(level->floors, level->floorsCount);

        return offset;
    }

    uint32 writeStaticMeshes(FileStream &f, TR1_PC* level)
    {
        uint32 offset = f.align4();

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

        return offset;
    }

    uint32 writeCameras(FileStream &f, TR1_PC* level)
    {
        uint32 offset = f.align4();

        f.writeObj(level->cameras, level->camerasCount);

        return offset;
    }

    uint32 writeSoundSources(FileStream &f, TR1_PC* level)
    {
        uint32 offset = f.align4();

        f.writeObj(level->soundSources, level->soundSourcesCount);

        return offset;
    }

    uint32 writeBoxes(FileStream &f, TR1_PC* level)
    {
        uint32 offset = f.align4();

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

        return offset;
    }

    uint32 writeOverlaps(FileStream &f, TR1_PC* level)
    {
        uint32 offset = f.align4();

        f.write(level->overlaps, level->overlapsCount);

        return offset;
    }

    void writeZones(FileStream &f, TR1_PC* level, uint32* zoneOffset)
    {
        for (int32 i = 0; i < 2; i++)
        {
            *zoneOffset++ = f.align4();
            f.write(level->zones[i].ground1, level->boxesCount);

            *zoneOffset++ = f.align4();
            f.write(level->zones[i].ground2, level->boxesCount);

            *zoneOffset++ = f.align4();
            f.write(level->zones[i].fly, level->boxesCount);
        }
    }

    uint32 writeAnimTex(FileStream &f, TR1_PC* level, const Remap* remap)
    {
        uint32 offset = f.align4();

        const uint16* data = level->animTexData;

        int16 rangesCount = *data++;

        f.write(rangesCount);

        for (int32 i = 0; i < rangesCount; i++)
        {
            int16 texCount = *data++;

            f.write(texCount);

            for (int32 j = 0; j <= texCount; j++)
            {
                uint16 texIndex = *data++;
                
                if (remap)
                {
                    texIndex = remap->textures[texIndex];
                }

                f.write(texIndex);
            }
        }

        return offset;
    }

    uint32 writeItems(FileStream &f, TR1_PC* level)
    {
        uint32 offset = f.align4();

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

        return offset;
    }

    uint32 writeCameraFrames(FileStream &f, TR1_PC* level)
    {
        uint32 offset = f.align4();

        f.writeObj(level->cameraFrames, level->cameraFramesCount);

        return offset;
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

        header.palette = writePalette(f, pc);
        header.lightmap = writeLightmap(f, pc);

        header.tiles = f.align4();
        f.write((uint8*)pc->tiles, header.tilesCount * 256 * 256);

        header.rooms = writeRooms(f, pc, NULL);
        header.floors = writeFloors(f, pc);

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

            Mesh* mesh = new Mesh(pc, ptr);
            mesh->write(f, NULL);
            delete[] mesh;

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

        header.staticMeshes = writeStaticMeshes(f, pc);

        header.objectTextures = f.align4();
        for (int32 i = 0; i < pc->objectTexturesCount; i++)
        {
            ObjectTexture comp(pc->objectTextures + i);
            comp.write(f);
        }

        header.spriteTextures = f.align4();
        for (int32 i = 0; i < pc->spriteTexturesCount; i++)
        {
            SpriteTexture comp(pc->spriteTextures + i);
            comp.write(f);
        }

        header.spriteSequences = f.align4();
        f.writeObj(pc->spriteSequences, pc->spriteSequencesCount);

        header.cameras = writeCameras(f, pc);
        header.soundSources = writeSoundSources(f, pc);
        header.boxes = writeBoxes(f, pc);
        header.overlaps = writeOverlaps(f, pc);
        writeZones(f, pc, header.zones[0]);
        header.animTexData = writeAnimTex(f, pc, NULL);
        header.items = writeItems(f, pc);
        header.cameraFrames = writeCameraFrames(f, pc);

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

            pc->soundOffsets[i] = uint32(dst - pc->soundData);
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
        sprintf(buf, "%s/*.ad4", from);

        WIN32_FIND_DATA fd;
        HANDLE h = FindFirstFile(buf, &fd);

        if (h == INVALID_HANDLE_VALUE)
            return;

        struct Track {
            int32 size;
            uint8* data;
        };
        Track tracks[MAX_TRACKS];
        memset(tracks, 0, sizeof(tracks));

        do
        {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                const char* src = fd.cFileName;
                const char* srcEnd = strrchr(src, '.');
                char* dst = buf;

                while (src < srcEnd)
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
                    tracks[index].data = new uint8[size];
                    fread(tracks[index].data, 1, size, f);
                    fclose(f);

                    tracks[index].size = size; // ad4 tool encodes 32-bit chunks, so no need to align
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
            f.write(tracks[i].data, tracks[i].size);
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
        int32 count = 0;

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

    void markAnimatedTextures(TR1_PC* level)
    {
        const uint16* data = level->animTexData;

        int16 rangesCount = *data++;

        for (int32 i = 0; i < rangesCount; i++)
        {
            int16 texCount = *data++;

            for (int32 j = 0; j <= texCount; j++)
            {
                level->objectTextures[*data++].attribute |= TEX_ATTR_ANIM;
            }
        }    
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
            markAnimatedTextures(level);
            addTextures(level);
        }

        linkTextures();

        packTiles(f);

        remapTextures();

        printf("textures: %d\n", objectTextures.count);
        printf("sprites: %d\n", spriteTextures.count);

        ASSERT(objectTextures.count < (1 << FACE_TYPE_SHIFT));

    // collect unique meshes
        LEVELS_LOOP()
        {
            ASSERT(level->meshOffsetsCount < MAX_MESHES);

            for (int32 i = 0; i < level->meshOffsetsCount; i++)
            {
                const uint8* ptr = (uint8*)level->meshData + level->meshOffsets[i];

                Mesh* mesh = new Mesh(level, ptr);
                remaps[id].meshes[i] = addMesh(mesh);
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

                AnimFrames* af = new AnimFrames(startPtr, endPtr);
                remaps[id].animFrames[i] = addAnimFrames(af);
            }
        }

    // collect unique models
        LEVELS_LOOP()
        {
            printf("anims: %d\n", level->animsCount);

            for (int32 i = 0; i < level->modelsCount; i++)
            {
                Model model;
                model.init(level, level->models[i]);
                remaps[id].models[i] = addModel(model);
            }
        }

        printf("Meshes: %d\n", meshes.count);
        printf("Models: %d\n", models.count);
        printf("Animations: %d\n", animFrames.count);

    // write objectTextures
        headers[0].objectTextures = f.align4();
        for (int32 i = 0; i < objectTextures.count; i++)
        {
            objectTextures[i]->write(f);
        }

    // write spriteTextures
        headers[0].spriteTextures = f.align4();
        for (int32 i = 0; i < spriteTextures.count; i++)
        {
            spriteTextures[i]->write(f);
        }

    // write meshes
        headers[0].meshData = f.align4();
        for (int32 i = 0; i < meshes.count; i++)
        {
            meshes[i]->offset = f.align4();
            meshes[i]->write(f, &remaps[meshes[i]->level->id]);
        }

    // write models
        headers[0].models = f.align4();
        for (int32 i = 0; i < models.count; i++)
        {
            models[i]->write(f);
        }

    // write anims
        headers[0].frameData = f.align4();
        for (int32 i = 0; i < animFrames.count; i++)
        {
            animFrames[i]->write(f);
        }

    // set global array pointers
        LEVELS_LOOP()
        {
            headers[id].objectTextures  = headers[0].objectTextures;
            headers[id].spriteTextures  = headers[0].spriteTextures;
            headers[id].meshData        = headers[0].meshData;
            headers[id].models          = headers[0].models;
            headers[id].frameData       = headers[0].frameData;
        }

    // palette
        LEVELS_LOOP()
        {
            headers[id].palette = writePalette(f, level);
        }

    // lightmaps
        LEVELS_LOOP()
        {
            headers[id].lightmap = writeLightmap(f, level);
        }

    // rooms
        LEVELS_LOOP()
        {
            headers[id].rooms = writeRooms(f, level, &remaps[id]);
        }

    // floors data
        LEVELS_LOOP()
        {
            headers[id].floors = writeFloors(f, level);
        }

    // mesh offsets
        LEVELS_LOOP()
        {
            headers[id].meshesCount = level->meshOffsetsCount;
            headers[id].meshOffsets = f.align4();
            for (int32 i = 0; i < level->meshOffsetsCount; i++)
            {
                f.write(meshes[remaps[id].meshes[i]]->offset);
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
            headers[id].staticMeshes = writeStaticMeshes(f, level);
        }

/*
        header.spriteSequences = f.align4();
        f.writeObj(pc->spriteSequences, pc->spriteSequencesCount);
*/

    // fixed cameras
        LEVELS_LOOP()
        {
            headers[id].cameras = writeCameras(f, level);
        }
        
    // static sounds
        LEVELS_LOOP()
        {
            headers[id].soundSources = writeSoundSources(f, level);
        }

    // boxes
        LEVELS_LOOP()
        {
            headers[id].boxes = writeBoxes(f, level);
        }

    // overlaps
        LEVELS_LOOP()
        {
            headers[id].overlaps = writeOverlaps(f, level);
        }

    // zones
        LEVELS_LOOP()
        {
            writeZones(f, level, headers[id].zones[0]);
        }

    // animated textures
        LEVELS_LOOP()
        {
            headers[id].animTexData = writeAnimTex(f, level, &remaps[id]);
        }

    // items
        LEVELS_LOOP()
        {
            headers[id].items = writeItems(f, level);
        }

    // animated camera frames
        LEVELS_LOOP()
        {
            headers[id].cameraFrames = writeCameraFrames(f, level);
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
        sprintf(buf, "%s/TR1.WAD", dir);
        FileStream f(buf, true);
            
        if (!f.isValid()) {
            printf("can't save \"%s\"\n", buf);
            return;
        }

        convertWAD(f, pc, psx);
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
            sprintf(buf, "%s/TRACKS.AD4", dir);
            FileStream f(buf, true);
            convertTracks(f, "tracks/conv_demo");
        }
    #endif

        delete[] roomVertices;
    }
};

#endif