#ifndef H_MESH
#define H_MESH

#include "core.h"
#include "format.h"

typedef unsigned short Index;

struct Vertex {
    short3  coord;
    short2  texCoord;
    short4  normal;
    ubyte4  color;
};

struct MeshRange {
    int iStart;
    int iCount;
    int vStart;
};

struct Mesh {
    GLuint  ID[2];
    int     iCount;
    int     vCount;

    Mesh(Index *indices, int iCount, Vertex *vertices, int vCount) : iCount(iCount), vCount(vCount) {
        glGenBuffers(2, ID);
        bind();
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, iCount * sizeof(Index), indices, GL_STATIC_DRAW);
        glBufferData(GL_ARRAY_BUFFER, vCount * sizeof(Vertex), vertices, GL_STATIC_DRAW);
    }

    virtual ~Mesh() {
        glDeleteBuffers(2, ID);
    }

    void bind() {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ID[0]);
        glBindBuffer(GL_ARRAY_BUFFER, ID[1]);

        glEnableVertexAttribArray(aCoord);
        glEnableVertexAttribArray(aTexCoord);
        glEnableVertexAttribArray(aNormal);
        glEnableVertexAttribArray(aColor);
    }

    void render(const MeshRange &range) {
        Vertex *v = (Vertex*)(range.vStart * sizeof(Vertex));
        glVertexAttribPointer(aCoord,    3, GL_SHORT,         false, sizeof(Vertex), &v->coord);
        glVertexAttribPointer(aTexCoord, 2, GL_SHORT,         true,  sizeof(Vertex), &v->texCoord);
        glVertexAttribPointer(aNormal,   4, GL_SHORT,         false, sizeof(Vertex), &v->normal);
        glVertexAttribPointer(aColor,    4, GL_UNSIGNED_BYTE, true,  sizeof(Vertex), &v->color);
        glDrawElements(GL_TRIANGLES, range.iCount, GL_UNSIGNED_SHORT, (GLvoid*)(range.iStart * sizeof(Index)));

        Core::stats.dips++;
        Core::stats.tris += range.iCount / 3;
    }
};

struct MeshBuilder {
// rooms
    struct RoomRange {
        MeshRange geometry;
        MeshRange sprites;     
    } *roomRanges;

// objects meshes
    struct MeshInfo : MeshRange {
        int         offset;
        TR::Vertex  center;
        int32       radius;
    }   *meshInfo;
    int mCount;

// sprite sequences
    MeshRange *spriteRanges;

// indexed mesh
    Mesh *mesh;

    MeshBuilder(const TR::Level &level) {
        roomRanges = new RoomRange[level.roomsCount];

        int iCount = 0, vCount = 0;

    // get size of mesh for rooms (geometry & sprites)
        for (int i = 0; i < level.roomsCount; i++) {
            TR::Room::Data &d = level.rooms[i].data;
            RoomRange &r = roomRanges[i];

            r.geometry.vStart = vCount;
            r.geometry.iStart = iCount;
            iCount += d.rCount * 6 + d.tCount * 3;
            vCount += d.rCount * 4 + d.tCount * 3;
            r.geometry.iCount = iCount - r.geometry.iStart;

            r.sprites.vStart = vCount;
            r.sprites.iStart = iCount;
            iCount += d.sCount * 6;
            vCount += d.sCount * 4;
            r.sprites.iCount = iCount - r.sprites.iStart;
        }

    // get objects mesh info
        #define OFFSET(bytes) (ptr = (TR::Mesh*)((char*)ptr + (bytes) - sizeof(char*)))

        mCount = 0;
        TR::Mesh *ptr = (TR::Mesh*)level.meshData;
        while ( ((int)ptr - (int)level.meshData) < level.meshDataSize * 2 ) {
            mCount++;

            OFFSET(ptr->vCount * sizeof(TR::Vertex));
            if (ptr->nCount > 0)
                OFFSET(ptr->nCount * sizeof(TR::Vertex));
            else
                OFFSET(-ptr->nCount * sizeof(int16));

            iCount += ptr->rCount * 6;
            vCount += ptr->rCount * 4;
            OFFSET(ptr->rCount * sizeof(TR::Rectangle));

            iCount += ptr->tCount * 3;
            vCount += ptr->tCount * 3;
            OFFSET(ptr->tCount * sizeof(TR::Triangle));

            iCount += ptr->crCount * 6;
            vCount += ptr->crCount * 4;
            OFFSET(ptr->crCount * sizeof(TR::Rectangle));

            iCount += ptr->ctCount * 3;
            vCount += ptr->ctCount * 3;
            OFFSET(ptr->ctCount * sizeof(TR::Triangle) + sizeof(TR::Mesh));
            ptr = (TR::Mesh*)(((int)ptr + 3) & -4);
        }
        meshInfo = new MeshInfo[mCount];
        
    // get size of mesh for sprite sequences
        spriteRanges = new MeshRange[level.spriteSequencesCount];
        for (int i = 0; i < level.spriteSequencesCount; i++) {
        // TODO: sequences not only first frame
            spriteRanges[i].vStart = vCount;
            spriteRanges[i].iStart = iCount;
            spriteRanges[i].iCount = 6;
            iCount += 6;
            vCount += 4;
        }

    // make meshes buffer (single vertex buffer object for all geometry & sprites on level)
        Index  *indices  = new Index[iCount];
        Vertex *vertices = new Vertex[vCount];
        iCount = vCount = 0;

    // build rooms
        for (int i = 0; i < level.roomsCount; i++) {
            TR::Room::Data &d = level.rooms[i].data;

        // rooms geometry
            int vStart = vCount;
            for (int j = 0; j < d.rCount; j++) {
                auto &f = d.rectangles[j];
                auto &t = level.objectTextures[f.texture];

                int  tile = t.tileAndFlag & 0x7FFF;
                int  tx = (tile % 4) * 256;
                int  ty = (tile / 4) * 256;

                addQuad(indices, iCount, vCount, vStart);

                for (int k = 0; k < 4; k++) {
                    auto &v = d.vertices[f.vertices[k]];
                    uint8 a = 255 - (v.lighting >> 5);

                    vertices[vCount].coord      = { v.vertex.x, v.vertex.y, v.vertex.z };
                    vertices[vCount].color      = { a, a, a, 255 };
                    vertices[vCount].normal     = { 0, 0, 0, 1 };
                    vertices[vCount].texCoord.x = ((tx + t.vertices[k].Xpixel) << 5) + 16;
                    vertices[vCount].texCoord.y = ((ty + t.vertices[k].Ypixel) << 5) + 16;
                    vCount++;
                }
            }

            for (int j = 0; j < d.tCount; j++) {
                auto &f = d.triangles[j];
                auto &t = level.objectTextures[f.texture];

                int  tile = t.tileAndFlag & 0x7FFF;
                int  tx = (tile % 4) * 256;
                int  ty = (tile / 4) * 256;

                addTriangle(indices, iCount, vCount, vStart);

                for (int k = 0; k < 3; k++) {
                    auto &v = d.vertices[f.vertices[k]];
                    uint8 a = 255 - (v.lighting >> 5);

                    vertices[vCount].coord      = { v.vertex.x, v.vertex.y, v.vertex.z };
                    vertices[vCount].color      = { a, a, a, 255 };
                    vertices[vCount].normal     = { 0, 0, 0, 1 };
                    vertices[vCount].texCoord.x = ((tx + t.vertices[k].Xpixel) << 5) + 16;
                    vertices[vCount].texCoord.y = ((ty + t.vertices[k].Ypixel) << 5) + 16;
                    vCount++;
                }
            }

        // rooms sprites
            TR::Room::Info &info = level.rooms[i].info;
            vStart = vCount;
            for (int j = 0; j < d.sCount; j++) {
                TR::Room::Data::Sprite &f = d.sprites[j];                   
                TR::Room::Data::Vertex &v = d.vertices[f.vertex];
                TR::SpriteTexture &sprite = level.spriteTextures[f.texture];
                uint8 intensity = 255 - (v.lighting >> 5);
                addSprite(indices, vertices, iCount, vCount, vStart, v.vertex.x, v.vertex.y, v.vertex.z, sprite, intensity);
            }
        }

    // build objects geometry
        mCount = 0;
        ptr = (TR::Mesh*)level.meshData;
        while ( ((int)ptr - (int)level.meshData) < level.meshDataSize * sizeof(uint16) ) {
            MeshInfo &info = meshInfo[mCount++];
            info.offset = (int)ptr - (int)level.meshData;
            info.vStart = vCount;
            info.iStart = iCount;
            info.center = ptr->center;
            info.radius = ptr->radius;

            TR::Vertex *mVertices = (TR::Vertex*)&ptr->vertices;

            OFFSET(ptr->vCount * sizeof(TR::Vertex));

            TR::Vertex  *normals = NULL;
            int16       *lights  = NULL;
            int         nCount   = ptr->nCount;

            if (ptr->nCount > 0) {
                normals = (TR::Vertex*)&ptr->normals;
                OFFSET(ptr->nCount * sizeof(TR::Vertex));
            } else {
                lights = (int16*)&ptr->lights;
                OFFSET(-ptr->nCount * sizeof(int16));
            }

            int vStart = vCount;
        // rectangles
            for (int j = 0; j < ptr->rCount; j++) {
                auto &f = ((TR::Rectangle*)&ptr->rectangles)[j];
                auto &t = level.objectTextures[f.texture];

                int  tile = t.tileAndFlag & 0x7FFF;
                int  tx = (tile % 4) * 256;
                int  ty = (tile / 4) * 256;

                addQuad(indices, iCount, vCount, vStart);

                for (int k = 0; k < 4; k++) {
                    auto &v = mVertices[f.vertices[k]];

                    vertices[vCount].coord      = { v.x, v.y, v.z };

                    if (nCount > 0) {
                        TR::Vertex &n = normals[f.vertices[k]];
                        vertices[vCount].normal = { n.x, n.y, n.z, 0   };
                        vertices[vCount].color  = { 255, 255, 255, 255 };
                    } else {
                        uint8 a = 255 - (lights[f.vertices[k]] >> 5);
                        vertices[vCount].normal = { 0, 0, 0, 1   };
                        vertices[vCount].color  = { a, a, a, 255 };
                    }
                    vertices[vCount].texCoord.x = ((tx + t.vertices[k].Xpixel) << 5) + 16;
                    vertices[vCount].texCoord.y = ((ty + t.vertices[k].Ypixel) << 5) + 16;
                    vCount++;
                }
            }
            OFFSET(ptr->rCount * sizeof(TR::Rectangle));

        // triangles
            for (int j = 0; j < ptr->tCount; j++) {
                auto &f = ((TR::Triangle*)&ptr->triangles)[j];
                auto &t = level.objectTextures[f.texture];

                int  tile = t.tileAndFlag & 0x7FFF;
                int  tx = (tile % 4) * 256;
                int  ty = (tile / 4) * 256;

                addTriangle(indices, iCount, vCount, vStart);

                for (int k = 0; k < 3; k++) {
                    auto &v = mVertices[f.vertices[k]];
                    vertices[vCount].coord      = { v.x, v.y, v.z };

                    if (nCount > 0) {
                        TR::Vertex &n = normals[f.vertices[k]];
                        vertices[vCount].normal = { n.x, n.y, n.z, 0   };
                        vertices[vCount].color  = { 255, 255, 255, 255 };
                    } else {
                        uint8 a = 255 - (lights[f.vertices[k]] >> 5);
                        vertices[vCount].normal = { 0, 0, 0, 1   };
                        vertices[vCount].color  = { a, a, a, 255 };
                    }
                    vertices[vCount].texCoord.x = ((tx + t.vertices[k].Xpixel) << 5) + 16;
                    vertices[vCount].texCoord.y = ((ty + t.vertices[k].Ypixel) << 5) + 16;
                    vCount++;
                }
            }
            OFFSET(ptr->tCount * sizeof(TR::Triangle));

        // color rectangles
            for (int j = 0; j < ptr->crCount; j++) {
                auto &f = ((TR::Rectangle*)&ptr->crectangles)[j];
                auto &c = level.palette[f.texture & 0xFF];

                addQuad(indices, iCount, vCount, vStart);

                for (int k = 0; k < 4; k++) {
                    auto &v = mVertices[f.vertices[k]];

                    vertices[vCount].coord      = { v.x, v.y, v.z };

                    if (nCount > 0) {
                        TR::Vertex &n = normals[f.vertices[k]];
                        vertices[vCount].normal = { n.x, n.y, n.z, 0 };
                        vertices[vCount].color  = { c.r, c.g, c.b, 255 };
                    } else {
                        uint8 a = 255 - (lights[f.vertices[k]] >> 5);
                        vertices[vCount].normal = { 0, 0, 0, 1   };
                        vertices[vCount].color  = { a, a, a, 255 }; // TODO: apply color
                    }
                    vertices[vCount].texCoord   = { 1022 << 5, 1022 << 5 };
                    vCount++;
                }
            }
            OFFSET(ptr->crCount * sizeof(TR::Rectangle));

        // color triangles
            for (int j = 0; j < ptr->ctCount; j++) {
                auto &f = ((TR::Triangle*)&ptr->ctriangles)[j];
                auto &c = level.palette[f.texture & 0xFF];

                addTriangle(indices, iCount, vCount, vStart);

                for (int k = 0; k < 3; k++) {
                    auto &v = mVertices[f.vertices[k]];

                    vertices[vCount].coord      = { v.x, v.y, v.z };

                    if (nCount > 0) {
                        TR::Vertex &n = normals[f.vertices[k]];
                        vertices[vCount].normal = { n.x, n.y, n.z, 0   };
                        vertices[vCount].color  = { c.r, c.g, c.b, 255 };
                    } else {
                        uint8 a = 255 - (lights[f.vertices[k]] >> 5);
                        vertices[vCount].normal = { 0, 0, 0, 1   };
                        vertices[vCount].color  = { a, a, a, 255 }; // TODO: apply color
                    }
                    vertices[vCount].texCoord   = { 1022 << 5, 1022 << 5 };
                    vCount++;
                }
            }
            OFFSET(ptr->ctCount * sizeof(TR::Triangle) + sizeof(TR::Mesh));

            ptr = (TR::Mesh*)(((int)ptr + 3) & -4);

            info.iCount = iCount - info.iStart;
        }

    // build sprite sequences
        for (int i = 0; i < level.spriteSequencesCount; i++) {
            TR::SpriteTexture &sprite = level.spriteTextures[level.spriteSequences[i].sStart];
            addSprite(indices, vertices, iCount, vCount, vCount, 0, -16, 0, sprite, 255);
        }

        mesh = new Mesh(indices, iCount, vertices, vCount);
        delete[] indices;
        delete[] vertices;
    }

    ~MeshBuilder() {
        delete[] roomRanges;
        delete[] meshInfo;
        delete[] spriteRanges;
        delete mesh;
    }

     void addTriangle(Index *indices, int &iCount, int vCount, int vStart) {
        int  vIndex = vCount - vStart;

        indices[iCount + 0] = vIndex + 0;
        indices[iCount + 1] = vIndex + 1;
        indices[iCount + 2] = vIndex + 2;

        iCount += 3;
     }

    void addQuad(Index *indices, int &iCount, int vCount, int vStart) {
        int  vIndex = vCount - vStart;

        indices[iCount + 0] = vIndex + 0;
        indices[iCount + 1] = vIndex + 1;
        indices[iCount + 2] = vIndex + 2;

        indices[iCount + 3] = vIndex + 0;
        indices[iCount + 4] = vIndex + 2;
        indices[iCount + 5] = vIndex + 3;

        iCount += 6;
    }

    void addSprite(Index *indices, Vertex *vertices, int &iCount, int &vCount, int vStart, int16 x, int16 y, int16 z, const TR::SpriteTexture &sprite, uint8 intensity) {
        addQuad(indices, iCount, vCount, vStart);

        Vertex *quad = &vertices[vCount];

        quad[0].coord = quad[1].coord = quad[2].coord = quad[3].coord = { x, y, z };

        int  tx = (sprite.tile % 4) * 256;
        int  ty = (sprite.tile / 4) * 256;

        int16 u0 = ((tx + sprite.u) << 5) + 16;
        int16 v0 = ((ty + sprite.v) << 5) + 16;
        int16 u1 = u0 + (sprite.w >> 3);
        int16 v1 = v0 + (sprite.h >> 3);

        quad[0].texCoord = { u0, v0 };
        quad[1].texCoord = { u1, v0 };
        quad[2].texCoord = { u1, v1 };
        quad[3].texCoord = { u0, v1 };

        quad[0].normal = { sprite.r, sprite.t, 0, 0 };
        quad[1].normal = { sprite.l, sprite.t, 0, 0 };
        quad[2].normal = { sprite.l, sprite.b, 0, 0 };
        quad[3].normal = { sprite.r, sprite.b, 0, 0 };

        quad[0].color = quad[1].color = quad[2].color = quad[3].color = { intensity, intensity, intensity, 255 };

        vCount += 4;
    }

    void bind() {
        mesh->bind();
    }

    void renderRoomGeometry(int roomIndex) {
        mesh->render(roomRanges[roomIndex].geometry);
    }

    void renderRoomSprites(int roomIndex) {
        mesh->render(roomRanges[roomIndex].sprites);
    }

    bool hasRoomSprites(int roomIndex) {
        return roomRanges[roomIndex].sprites.iCount > 0;
    }

    void renderMesh(int meshIndex) {
        mesh->render(meshInfo[meshIndex]);
    }

    void renderSprite(int spriteIndex) {
        mesh->render(spriteRanges[spriteIndex]);
    }
};

#endif