#ifndef H_MESH
#define H_MESH

#include "core.h"
#include "format.h"

typedef unsigned short Index;

struct Vertex {
    short4  coord;      // xyz  - position
    short4  texCoord;   // xy   - texture coordinates, z - anim tex range index, w - anim tex frame index
    short4  normal;     // xyz  - vertex normal, w - disable lighting (0, 1)
    ubyte4  color;      // rgba - color
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
        glVertexAttribPointer(aCoord,    4, GL_SHORT,         false, sizeof(Vertex), &v->coord);
        glVertexAttribPointer(aTexCoord, 4, GL_SHORT,         false, sizeof(Vertex), &v->texCoord);
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
        int          offset;
        TR::Vertex   center;
        TR::Collider collider;
    }   *meshInfo;
    int mCount;

// sprite sequences
    MeshRange *sequenceRanges;

// indexed mesh
    Mesh *mesh;

    vec2 *animTexRanges;
    vec2 *animTexOffsets;

    int animTexRangesCount;
    int animTexOffsetsCount;

    TR::Level *level;

    MeshBuilder(TR::Level &level) : level(&level) {
        initAnimTextures(level);

    // create dummy white object textures for non-textured (colored) geometry
        TR::ObjectTexture whiteTileQuad;
        whiteTileQuad.attribute = 0;
        whiteTileQuad.tile.index = 15;
        whiteTileQuad.tile.triangle = 0;
        whiteTileQuad.vertices[0] = 
        whiteTileQuad.vertices[1] = 
        whiteTileQuad.vertices[2] = 
        whiteTileQuad.vertices[3] = { 0, 253, 0, 253 };

        TR::ObjectTexture whiteTileTri = whiteTileQuad;
        whiteTileTri.tile.triangle = 1;

    // allocate room geometry ranges
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
        while ( ((intptr_t)ptr - (intptr_t)level.meshData) < level.meshDataSize * 2 ) {
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
            ptr = (TR::Mesh*)(((intptr_t)ptr + 3) & -4);
        }
        meshInfo = new MeshInfo[mCount];
        
    // get size of mesh for sprite sequences
        sequenceRanges = new MeshRange[level.spriteSequencesCount];
        for (int i = 0; i < level.spriteSequencesCount; i++) {
        // TODO: sequences not only first frame
            sequenceRanges[i].vStart = vCount;
            sequenceRanges[i].iStart = iCount;
            sequenceRanges[i].iCount = level.spriteSequences[i].sCount * 6;
            iCount += level.spriteSequences[i].sCount * 6;
            vCount += level.spriteSequences[i].sCount * 4;
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
                TR::Rectangle     &f = d.rectangles[j];
                TR::ObjectTexture &t = level.objectTextures[f.texture];

                addQuad(indices, iCount, vCount, vStart, vertices, &t);

                for (int k = 0; k < 4; k++) {
                    TR::Room::Data::Vertex &v = d.vertices[f.vertices[k]];
                    uint8 a = 255 - (v.lighting >> 5);

                    vertices[vCount].coord      = { v.vertex.x, v.vertex.y, v.vertex.z };
                    vertices[vCount].color      = { a, a, a, 255 };
                    vertices[vCount].normal     = { 0, 0, 0, 1 };
                    vCount++;
                }
            }

            for (int j = 0; j < d.tCount; j++) {
                TR::Triangle      &f = d.triangles[j];
                TR::ObjectTexture &t = level.objectTextures[f.texture];                

                addTriangle(indices, iCount, vCount, vStart, vertices, &t);

                for (int k = 0; k < 3; k++) {
                    auto &v = d.vertices[f.vertices[k]];
                    uint8 a = 255 - (v.lighting >> 5);

                    vertices[vCount].coord      = { v.vertex.x, v.vertex.y, v.vertex.z };
                    vertices[vCount].color      = { a, a, a, 255 };
                    vertices[vCount].normal     = { 0, 0, 0, 1 };
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
        ptr = (TR::Mesh*)level.meshData;
        for (int i = 0; i < mCount; i++) {
            MeshInfo &info = meshInfo[i];
            info.offset   = (intptr_t)ptr - (intptr_t)level.meshData;
            info.vStart   = vCount;
            info.iStart   = iCount;
            info.center   = ptr->center;
            info.collider = ptr->collider;

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
                TR::Rectangle     &f = ((TR::Rectangle*)&ptr->rectangles)[j];
                TR::ObjectTexture &t = level.objectTextures[f.texture];

                addQuad(indices, iCount, vCount, vStart, vertices, &t);

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
                    vCount++;
                }
            }
            OFFSET(ptr->rCount * sizeof(TR::Rectangle));

        // triangles
            for (int j = 0; j < ptr->tCount; j++) {
                TR::Triangle      &f = ((TR::Triangle*)&ptr->triangles)[j];
                TR::ObjectTexture &t = level.objectTextures[f.texture];

                addTriangle(indices, iCount, vCount, vStart, vertices, &t);

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
                    vCount++;
                }
            }
            OFFSET(ptr->tCount * sizeof(TR::Triangle));

        // color rectangles
            for (int j = 0; j < ptr->crCount; j++) {
                TR::Rectangle &f = ((TR::Rectangle*)&ptr->crectangles)[j];
                TR::RGB       &c = level.palette[f.texture & 0xFF];

                addQuad(indices, iCount, vCount, vStart, vertices, &whiteTileQuad);

                for (int k = 0; k < 4; k++) {
                    auto &v = mVertices[f.vertices[k]];

                    vertices[vCount].coord      = { v.x, v.y, v.z, 0 };

                    if (nCount > 0) {
                        TR::Vertex &n = normals[f.vertices[k]];
                        vertices[vCount].normal = { n.x, n.y, n.z, 0 };
                        vertices[vCount].color  = { c.r, c.g, c.b, 255 };
                    } else {
                        uint8 a = 255 - (lights[f.vertices[k]] >> 5);
                        vertices[vCount].normal = { 0, 0, 0, 1   };
                        vertices[vCount].color  = { a, a, a, 255 };
                    }
                    vCount++;
                }
            }
            OFFSET(ptr->crCount * sizeof(TR::Rectangle));

        // color triangles
            for (int j = 0; j < ptr->ctCount; j++) {
                TR::Triangle &f = ((TR::Triangle*)&ptr->ctriangles)[j];
                TR::RGB      &c = level.palette[f.texture & 0xFF];

                addTriangle(indices, iCount, vCount, vStart, vertices, &whiteTileTri);

                for (int k = 0; k < 3; k++) {
                    auto &v = mVertices[f.vertices[k]];

                    vertices[vCount].coord      = { v.x, v.y, v.z, 0 };

                    if (nCount > 0) {
                        TR::Vertex &n = normals[f.vertices[k]];
                        vertices[vCount].normal = { n.x, n.y, n.z, 0   };
                        vertices[vCount].color  = { c.r, c.g, c.b, 255 };
                    } else {
                        uint8 a = 255 - (lights[f.vertices[k]] >> 5);
                        vertices[vCount].normal = { 0, 0, 0, 1   };
                        vertices[vCount].color  = { a, a, a, 255 };
                    }
                    vCount++;
                }
            }
            info.iCount = iCount - info.iStart;

            OFFSET(ptr->ctCount * sizeof(TR::Triangle) + sizeof(TR::Mesh));
            ptr = (TR::Mesh*)(((intptr_t)ptr + 3) & -4);
        }

    // build sprite sequences
        for (int i = 0; i < level.spriteSequencesCount; i++) 
            for (int j = 0; j < level.spriteSequences[i].sCount; j++) {
                TR::SpriteTexture &sprite = level.spriteTextures[level.spriteSequences[i].sStart + j];
                addSprite(indices, vertices, iCount, vCount, vCount, 0, 0, 0, sprite, 255);
            }

        mesh = new Mesh(indices, iCount, vertices, vCount);
        delete[] indices;
        delete[] vertices;
    }

    ~MeshBuilder() {
        delete[] animTexRanges;
        delete[] animTexOffsets;
        delete[] roomRanges;
        delete[] meshInfo;
        delete[] sequenceRanges;
        delete mesh;
    }

    vec2 getTexCoord(const TR::ObjectTexture &tex) {
        int  tile = tex.tile.index;
        int  tx = (tile % 4) * 256;
        int  ty = (tile / 4) * 256;
        return vec2( (float)(((tx + tex.vertices[0].Xpixel) << 5) + 16),
                     (float)(((ty + tex.vertices[0].Ypixel) << 5) + 16) );
    }

    void initAnimTextures(TR::Level &level) {
        ASSERT(level.animTexturesDataSize);

        uint16 *ptr = &level.animTexturesData[0];
        animTexRangesCount = *ptr++ + 1;
        animTexRanges = new vec2[animTexRangesCount];
        animTexRanges[0] = vec2(0.0f, 1.0f);
        animTexOffsetsCount = 1;
        for (int i = 1; i < animTexRangesCount; i++) {
            TR::AnimTexture *animTex = (TR::AnimTexture*)ptr;
            
            int start = animTexOffsetsCount;
            animTexOffsetsCount += animTex->count + 1;
            animTexRanges[i] = vec2((float)start, (float)(animTexOffsetsCount - start));

            ptr += (sizeof(TR::AnimTexture) + sizeof(animTex->textures[0]) * (animTex->count + 1)) / sizeof(uint16);
        }
        animTexOffsets = new vec2[animTexOffsetsCount];
        animTexOffsets[0] = vec2(0.0f);
        animTexOffsetsCount = 1;

        ptr = &level.animTexturesData[1];
        for (int i = 1; i < animTexRangesCount; i++) {
            int start = animTexOffsetsCount;
            TR::AnimTexture *animTex = (TR::AnimTexture*)ptr;            

            vec2 first = getTexCoord(level.objectTextures[animTex->textures[0]]);
            animTexOffsets[animTexOffsetsCount++] = vec2(0.0f); // first - first for first frame %)

            for (int j = 1; j <= animTex->count; j++)
                animTexOffsets[animTexOffsetsCount++] = getTexCoord(level.objectTextures[animTex->textures[j]]) - first;

            ptr += (sizeof(TR::AnimTexture) + sizeof(animTex->textures[0]) * (animTex->count + 1)) / sizeof(uint16);
        }
    }

    TR::ObjectTexture* getAnimTexture(TR::ObjectTexture *tex, int &range, int &frame) {
        range = frame = 0;
        if (!level->animTexturesDataSize)
            return tex;

        uint16 *ptr = &level->animTexturesData[1];
        for (int i = 1; i < animTexRangesCount; i++) {
            TR::AnimTexture *animTex = (TR::AnimTexture*)ptr;            

            for (int j = 0; j <= animTex->count; j++)
                if (tex == &level->objectTextures[animTex->textures[j]]) {
                    range = i;
                    frame = j;
                    return &level->objectTextures[animTex->textures[0]];
                }
            
            ptr += (sizeof(TR::AnimTexture) + sizeof(animTex->textures[0]) * (animTex->count + 1)) / sizeof(uint16);
        }
        
        return tex;
    }

    void addTexCoord(Vertex *vertices, int vCount, TR::ObjectTexture *tex) {
        int range, frame;
        tex = getAnimTexture(tex, range, frame);

        int  tile = tex->tile.index;
        int  tx = (tile % 4) * 256;
        int  ty = (tile / 4) * 256;

        int count = tex->tile.triangle ? 3 : 4;
        for (int i = 0; i < count; i++) {
            Vertex &v = vertices[vCount + i];
            v.texCoord.x = ((tx + tex->vertices[i].Xpixel) << 5) + 16;
            v.texCoord.y = ((ty + tex->vertices[i].Ypixel) << 5) + 16;
            v.texCoord.z = range;
            v.texCoord.w = frame;
        }
    }

    void addTriangle(Index *indices, int &iCount, int vCount, int vStart, Vertex *vertices, TR::ObjectTexture *tex) {
        int  vIndex = vCount - vStart;

        indices[iCount + 0] = vIndex + 0;
        indices[iCount + 1] = vIndex + 1;
        indices[iCount + 2] = vIndex + 2;

        iCount += 3;

        if (tex) addTexCoord(vertices, vCount, tex);
    }

    void addQuad(Index *indices, int &iCount, int vCount, int vStart, Vertex *vertices, TR::ObjectTexture *tex) {
        int  vIndex = vCount - vStart;

        indices[iCount + 0] = vIndex + 0;
        indices[iCount + 1] = vIndex + 1;
        indices[iCount + 2] = vIndex + 2;

        indices[iCount + 3] = vIndex + 0;
        indices[iCount + 4] = vIndex + 2;
        indices[iCount + 5] = vIndex + 3;

        iCount += 6;

        if (tex) addTexCoord(vertices, vCount, tex);
    }

    void addSprite(Index *indices, Vertex *vertices, int &iCount, int &vCount, int vStart, int16 x, int16 y, int16 z, const TR::SpriteTexture &sprite, uint8 intensity) {
        addQuad(indices, iCount, vCount, vStart, NULL, NULL);

        Vertex *quad = &vertices[vCount];

        quad[0].coord  = quad[1].coord  = quad[2].coord  = quad[3].coord  = { x, y, z, 0 };
        quad[0].normal = quad[1].normal = quad[2].normal = quad[3].normal = { 0, 0, 0, 0 };
        quad[0].color  = quad[1].color  = quad[2].color  = quad[3].color  = { intensity, intensity, intensity, 255 };

        int  tx = (sprite.tile % 4) * 256;
        int  ty = (sprite.tile / 4) * 256;

        int16 u0 = ((tx + sprite.u) << 5) + 16;
        int16 v0 = ((ty + sprite.v) << 5) + 16;
        int16 u1 = u0 + (sprite.w >> 3);
        int16 v1 = v0 + (sprite.h >> 3);

        quad[0].texCoord = { u0, v0, sprite.r, sprite.t };
        quad[1].texCoord = { u1, v0, sprite.l, sprite.t };
        quad[2].texCoord = { u1, v1, sprite.l, sprite.b };
        quad[3].texCoord = { u0, v1, sprite.r, sprite.b };

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

    void renderMesh(MeshInfo *meshInfo) {
        mesh->render(*meshInfo);
    }

    void renderMesh(int meshIndex) {
        renderMesh(&meshInfo[meshIndex]);
    }

    void renderSprite(int sequenceIndex, int frame) {
        MeshRange range = sequenceRanges[sequenceIndex];
        range.iCount = 6;
        range.iStart += frame * 6;
        range.vStart += frame * 4;
        mesh->render(range);
    }
};

#endif