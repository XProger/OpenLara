#ifndef H_MESH
#define H_MESH

#include "core.h"
#include "format.h"

typedef unsigned short Index;

struct Vertex {
    short4  coord;      // xyz  - position, w - unused
    short4  texCoord;   // xy   - texture coordinates, z - anim tex range index, w - anim tex frame index
    short4  normal;     // xyz  - vertex normal, w - disable lighting (0, 1)
    ubyte4  color;      // xyz  - color, w - intensity
};

struct MeshRange {
    int iStart;
    int iCount;
    int vStart;
    int aIndex;

    MeshRange() : aIndex(-1) {}

    void setup() const {
        Vertex *v = (Vertex*)(vStart * sizeof(Vertex));
        glVertexAttribPointer(aCoord,    4, GL_SHORT,         false, sizeof(Vertex), &v->coord);
        glVertexAttribPointer(aTexCoord, 4, GL_SHORT,         false, sizeof(Vertex), &v->texCoord);
        glVertexAttribPointer(aNormal,   4, GL_SHORT,         false, sizeof(Vertex), &v->normal);
        glVertexAttribPointer(aColor,    4, GL_UNSIGNED_BYTE, true,  sizeof(Vertex), &v->color);
    }

    void bind(GLuint *VAO) const {
        if (aIndex > -1)
            glBindVertexArray(VAO[aIndex]);
        else
            setup();        
    }
};

struct Mesh {
    GLuint  ID[2];
    GLuint  *VAO;
    int     iCount;
    int     vCount;
    int     aCount;
    int     aIndex;

    Mesh(Index *indices, int iCount, Vertex *vertices, int vCount, int aCount) : VAO(NULL), iCount(iCount), vCount(vCount), aCount(aCount), aIndex(0) {
        glGenBuffers(2, ID);
        bind();
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, iCount * sizeof(Index), indices, GL_STATIC_DRAW);
        glBufferData(GL_ARRAY_BUFFER, vCount * sizeof(Vertex), vertices, GL_STATIC_DRAW);

        if (Core::support.VAO && aCount) {
            VAO = new GLuint[aCount];
            glGenVertexArrays(aCount, VAO);
        }
    }

    virtual ~Mesh() {
        if (VAO) {
            glDeleteVertexArrays(aCount, VAO);
            delete[] VAO;
        }
        glDeleteBuffers(2, ID);
    }

    void initRange(MeshRange &range) {
        if (Core::support.VAO) {
            range.aIndex = aIndex++;
            range.bind(VAO);
            bind();
            range.setup();
        } else
            range.aIndex = -1;
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
        range.bind(VAO);
        glDrawElements(GL_TRIANGLES, range.iCount, GL_UNSIGNED_SHORT, (GLvoid*)(range.iStart * sizeof(Index)));

        Core::stats.dips++;
        Core::stats.tris += range.iCount / 3;
    }
};


#define CHECK_NORMAL(n) \
        if (!(n.x | n.y | n.z)) {\
            vec3 o(mVertices[f.vertices[0]]);\
            vec3 a = o - mVertices[f.vertices[1]];\
            vec3 b = o - mVertices[f.vertices[2]];\
            o = b.cross(a).normal() * 16300.0f;\
            n.x = (int)o.x;\
            n.y = (int)o.y;\
            n.z = (int)o.z;\
        }\

#define CHECK_ROOM_NORMAL(n) \
            vec3 o(d.vertices[f.vertices[0]].vertex);\
            vec3 a = o - d.vertices[f.vertices[1]].vertex;\
            vec3 b = o - d.vertices[f.vertices[2]].vertex;\
            o = b.cross(a).normal() * 16300.0f;\
            n.x = (int)o.x;\
            n.y = (int)o.y;\
            n.z = (int)o.z;

float intensityf(int lighting) {
    if (lighting < 0) return 1.0f;
    float lum = 1.0f - (lighting >> 5) / 255.0f;
    return lum * lum; // gamma to linear space
}

uint8 intensity(int lighting) {
    return uint8(intensityf(lighting) * 255);
}

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
    MeshInfo **meshMap;  // meshInfo by meshOffsetIndex

    MeshRange *spriteSequences;
    MeshRange shadowBlob;
    MeshRange bar;

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
        whiteTileQuad.texCoord[0] = 
        whiteTileQuad.texCoord[1] = 
        whiteTileQuad.texCoord[2] = 
        whiteTileQuad.texCoord[3] = { 253, 253 };

        TR::ObjectTexture whiteTileTri = whiteTileQuad;
        whiteTileTri.tile.triangle = 1;

    // allocate room geometry ranges
        roomRanges = new RoomRange[level.roomsCount];

        int iCount = 0, vCount = 0, aCount = 0;

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
            if (r.sprites.iCount)
                aCount++;
        }
        aCount += level.roomsCount;

    // get objects mesh info
        #define OFFSET(bytes) (ptr = (TR::Mesh*)((char*)ptr + (bytes) - sizeof(char*)))

        for (int i = 0; i < level.meshesCount; i++) {
            TR::Mesh &mesh = level.meshes[i];
            iCount += mesh.rCount * 6 + mesh.tCount * 3;
            vCount += mesh.rCount * 4 + mesh.tCount * 3;
        }
        aCount += level.meshesCount;
        meshInfo = new MeshInfo[level.meshesCount];
        meshMap  = new MeshInfo*[level.meshOffsetsCount];
        memset(meshMap, 0, sizeof(meshMap[0]) * level.meshOffsetsCount); 
        
    // get size of mesh for sprite sequences
        spriteSequences = new MeshRange[level.spriteSequencesCount];
        for (int i = 0; i < level.spriteSequencesCount; i++) {
        // TODO: sequences not only first frame
            spriteSequences[i].vStart = vCount;
            spriteSequences[i].iStart = iCount;
            spriteSequences[i].iCount = level.spriteSequences[i].sCount * 6;
            iCount += level.spriteSequences[i].sCount * 6;
            vCount += level.spriteSequences[i].sCount * 4;
        }
        aCount += level.spriteSequencesCount;

    // get size of simple shadow spot mesh (8 triangles, 8 vertices)
        shadowBlob.vStart = vCount;
        shadowBlob.iStart = iCount;
        shadowBlob.iCount = 8 * 3;
        aCount++;
        iCount += shadowBlob.iCount;
        vCount += 8;

    // bar (health, oxygen)
        bar.vStart = vCount;
        bar.iStart = iCount;
        bar.iCount = 2 * 3;
        aCount++;
        iCount += bar.iCount;
        vCount += 4;

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

                TR::Vertex n;
                CHECK_ROOM_NORMAL(n);

                for (int k = 0; k < 4; k++) {
                    TR::Room::Data::Vertex &v = d.vertices[f.vertices[k]];
                    vertices[vCount].coord  = { v.vertex.x, v.vertex.y, v.vertex.z, 0 };
                    vertices[vCount].normal = { n.x, n.y, n.z, 0 };
                    vertices[vCount].color  = { 255, 255, 255, intensity(v.lighting) };
                    vCount++;
                }
            }

            for (int j = 0; j < d.tCount; j++) {
                TR::Triangle      &f = d.triangles[j];
                TR::ObjectTexture &t = level.objectTextures[f.texture];

                addTriangle(indices, iCount, vCount, vStart, vertices, &t);

                TR::Vertex n;
                CHECK_ROOM_NORMAL(n);

                for (int k = 0; k < 3; k++) {
                    auto &v = d.vertices[f.vertices[k]];
                    vertices[vCount].coord  = { v.vertex.x, v.vertex.y, v.vertex.z, 0 };
                    vertices[vCount].normal = { n.x, n.y, n.z, 0 };
                    vertices[vCount].color  = { 255, 255, 255, intensity(v.lighting) };
                    vCount++;
                }
            }

        // rooms sprites
            vStart = vCount;
            for (int j = 0; j < d.sCount; j++) {
                TR::Room::Data::Sprite &f = d.sprites[j];
                TR::Room::Data::Vertex &v = d.vertices[f.vertex];
                TR::SpriteTexture &sprite = level.spriteTextures[f.texture];

                addSprite(indices, vertices, iCount, vCount, vStart, v.vertex.x, v.vertex.y, v.vertex.z, sprite, intensity(v.lighting));
            }
        }

    // build objects geometry
        TR::Color24 COLOR_WHITE = { 255, 255, 255 };

        for (int i = 0; i < level.meshesCount; i++) {
            TR::Mesh &mesh = level.meshes[i];

            MeshInfo &info = meshInfo[i];
            info.offset   = mesh.offset;
            info.vStart   = vCount;
            info.iStart   = iCount;
            info.center   = mesh.center;
            info.collider = mesh.collider;

            if (!info.offset)
                meshMap[0] = &info;
            else
                for (int j = 0; j < level.meshOffsetsCount; j++)
                    if (info.offset == level.meshOffsets[j])
                        meshMap[j] = &info;

            int vStart = vCount;
            for (int j = 0; j < mesh.rCount; j++) {
                TR::Rectangle &f = mesh.rectangles[j];
                bool textured = !(f.texture & 0x8000);
                TR::ObjectTexture &t = textured ? level.objectTextures[f.texture] : whiteTileQuad;
                TR::Color24 c = textured ? COLOR_WHITE : level.getColor(f.texture);

                addQuad(indices, iCount, vCount, vStart, vertices, &t);

                for (int k = 0; k < 4; k++) {
                    TR::Mesh::Vertex &v  = mesh.vertices[f.vertices[k]];

                    vertices[vCount].coord  = v.coord;
                    vertices[vCount].normal = v.normal;
                    vertices[vCount].color  = { c.r, c.g, c.b, intensity(v.coord.w) };

                    vCount++;
                }
            }

            for (int j = 0; j < mesh.tCount; j++) {
                TR::Triangle &f = mesh.triangles[j];
                bool textured = !(f.texture & 0x8000);
                TR::ObjectTexture &t = textured ? level.objectTextures[f.texture] : whiteTileQuad;
                TR::Color24 c = textured ? COLOR_WHITE : level.getColor(f.texture);

                addTriangle(indices, iCount, vCount, vStart, vertices, &t);

                for (int k = 0; k < 3; k++) {
                    TR::Mesh::Vertex &v  = mesh.vertices[f.vertices[k]];

                    vertices[vCount].coord  = v.coord;
                    vertices[vCount].normal = v.normal;
                    vertices[vCount].color  = { c.r, c.g, c.b, intensity(v.coord.w) };

                    vCount++;
                }
            }
            info.iCount = iCount - info.iStart;
        }

    // build sprite sequences
        for (int i = 0; i < level.spriteSequencesCount; i++) 
            for (int j = 0; j < level.spriteSequences[i].sCount; j++) {
                TR::SpriteTexture &sprite = level.spriteTextures[level.spriteSequences[i].sStart + j];
                addSprite(indices, vertices, iCount, vCount, spriteSequences[i].vStart, 0, 0, 0, sprite, 255);
            }

    // build shadow spot
        for (int i = 0; i < 8; i++) {
            Vertex &v = vertices[vCount + i];
            v.normal    = { 0, -1, 0, 1 };
            v.color     = { 255, 255, 255, 0 };
            v.texCoord  = { 32688, 32688, 0, 0 };

            float a = i * (PI / 4.0f) + (PI / 8.0f);
            short c = short(cosf(a) * 512.0f);
            short s = short(sinf(a) * 512.0f);
            v.coord = { c, 0, s, 0 };

            int idx = iCount + i * 3;
            indices[idx + 0] = i;
            indices[idx + 1] = 0;
            indices[idx + 2] = (i + 1) % 8;
        }
        iCount += shadowBlob.iCount;
        vCount += 8;

    // white bar
        addQuad(indices, iCount, vCount, bar.vStart, vertices, &whiteTileQuad);
        vertices[vCount + 0].coord = { 0, 0, 0, 0 };
        vertices[vCount + 1].coord = { 1, 0, 0, 0 };
        vertices[vCount + 2].coord = { 1, 1, 0, 0 };
        vertices[vCount + 3].coord = { 0, 1, 0, 0 };

        for (int i = 0; i < 4; i++) {
            Vertex &v = vertices[vCount + i];
            v.normal    = { 0, 0, 0, 0 };
            v.color     = { 255, 255, 255, 255 };
            v.texCoord  = { 32688, 32688, 0, 0 };
        }
        iCount += bar.iCount;
        vCount += 8;

    // compile buffer and ranges
        mesh = new Mesh(indices, iCount, vertices, vCount, aCount);
        delete[] indices;
        delete[] vertices;

        PROFILE_LABEL(BUFFER, mesh->ID[0], "Geometry indices");
        PROFILE_LABEL(BUFFER, mesh->ID[1], "Geometry vertices");

        // initialize Vertex Arrays
        for (int i = 0; i < level.roomsCount; i++) {
            RoomRange &r = roomRanges[i];
            mesh->initRange(r.geometry);
            if (r.sprites.iCount)
                mesh->initRange(r.sprites);
        }

        for (int i = 0; i < level.spriteSequencesCount; i++)
            mesh->initRange(spriteSequences[i]);       
        for (int i = 0; i < level.meshesCount; i++)
            mesh->initRange(meshInfo[i]);
        mesh->initRange(shadowBlob);
    }

    ~MeshBuilder() {
        delete[] animTexRanges;
        delete[] animTexOffsets;
        delete[] roomRanges;
        delete[] meshInfo;
        delete[] meshMap;
        delete[] spriteSequences;
        delete mesh;
    }

    vec2 getTexCoord(const TR::ObjectTexture &tex) {
        int  tile = tex.tile.index;
        int  tx = (tile % 4) * 256;
        int  ty = (tile / 4) * 256;
        return vec2( (float)(((tx + tex.texCoord[0].x) << 5) + 16),
                     (float)(((ty + tex.texCoord[0].y) << 5) + 16) );
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

    void addTexCoord(Vertex *vertices, int vCount, TR::ObjectTexture *tex, bool triangle) {
        int range, frame;
        tex = getAnimTexture(tex, range, frame);

        int  tile = tex->tile.index;
        int  tx = (tile % 4) * 256;
        int  ty = (tile / 4) * 256;

        int count = triangle ? 3 : 4;
        for (int i = 0; i < count; i++) {
            Vertex &v = vertices[vCount + i];
            v.texCoord.x = ((tx + tex->texCoord[i].x) << 5) + 8;
            v.texCoord.y = ((ty + tex->texCoord[i].y) << 5) + 8;
            v.texCoord.z = range;
            v.texCoord.w = frame;
        }

        if (level->version == TR::Level::VER_TR1_PSX && !triangle)
            swap(vertices[vCount + 2].texCoord, vertices[vCount + 3].texCoord);
    }

    void addTriangle(Index *indices, int &iCount, int vCount, int vStart, Vertex *vertices, TR::ObjectTexture *tex) {
        int  vIndex = vCount - vStart;

        indices[iCount + 0] = vIndex + 0;
        indices[iCount + 1] = vIndex + 1;
        indices[iCount + 2] = vIndex + 2;

        iCount += 3;

        if (tex) addTexCoord(vertices, vCount, tex, true);
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

        if (tex) addTexCoord(vertices, vCount, tex, false);
    }

    void addSprite(Index *indices, Vertex *vertices, int &iCount, int &vCount, int vStart, int16 x, int16 y, int16 z, const TR::SpriteTexture &sprite, uint8 intensity) {
        addQuad(indices, iCount, vCount, vStart, NULL, NULL);

        Vertex *quad = &vertices[vCount];

        quad[0].coord  = quad[1].coord  = quad[2].coord  = quad[3].coord  = { x, y, z, 0 };
        quad[0].normal = quad[1].normal = quad[2].normal = quad[3].normal = { 0, 0, 0, 0 };
        quad[0].color  = quad[1].color  = quad[2].color  = quad[3].color  = { 255, 255, 255, intensity };

        int  tx = (sprite.tile % 4) * 256;
        int  ty = (sprite.tile / 4) * 256;

        int16 u0 = ((tx + sprite.texCoord[0].x) << 5) + 8;
        int16 v0 = ((ty + sprite.texCoord[0].y) << 5) + 8;
        int16 u1 = ((tx + sprite.texCoord[1].x) << 5) + 8;
        int16 v1 = ((ty + sprite.texCoord[1].y) << 5) + 8;

        quad[0].texCoord = { u0, v0, sprite.l, sprite.t };
        quad[1].texCoord = { u1, v0, sprite.r, sprite.t };
        quad[2].texCoord = { u1, v1, sprite.r, sprite.b };
        quad[3].texCoord = { u0, v1, sprite.l, sprite.b };

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
        MeshRange range = spriteSequences[sequenceIndex];
        range.iCount  = 6;
        range.iStart += frame * 6;
        mesh->render(range);
    }

    void renderShadowSpot() {
        mesh->render(shadowBlob);
    }

    void renderLine(const vec2 &pos, const vec2 &size, uint32 color) {

    }

    void renderBar(const vec2 &size, float value) {
        /*
        float w = size.y / 9.0f;
        // health bar
        enum Colors {
            clBlack = 0xFF000000,
            clGrayL = 0xFF748474,
            clGrayD = 0xFF4C504C,
            clRed1  = 0xFF705C2C,
            clRed2  = 0xFFA47848,
            clRed3  = 0xFF705C2C,
            clRed4  = 0xFF584400,
            clRed5  = 0xFF503014,
        };

        uint32 *d = (uint32*)&data[0];
        d[0] = clGrayD; d[1] = clGrayD; d[2] = clGrayD; d[3] = clGrayD; d[4] = clGrayL; d+= 1024;
        d[0] = clGrayD; d[1] = clBlack; d[2] = clBlack; d[3] = clBlack; d[4] = clGrayL; d+= 1024;
        d[0] = clGrayD; d[1] = clBlack; d[2] = clRed1;  d[3] = clBlack; d[4] = clGrayL; d+= 1024;
        d[0] = clGrayD; d[1] = clBlack; d[2] = clRed2;  d[3] = clBlack; d[4] = clGrayL; d+= 1024;
        d[0] = clGrayD; d[1] = clBlack; d[2] = clRed3;  d[3] = clBlack; d[4] = clGrayL; d+= 1024;
        d[0] = clGrayD; d[1] = clBlack; d[2] = clRed4;  d[3] = clBlack; d[4] = clGrayL; d+= 1024;
        d[0] = clGrayD; d[1] = clBlack; d[2] = clRed5;  d[3] = clBlack; d[4] = clGrayL; d+= 1024;
        d[0] = clGrayD; d[1] = clBlack; d[2] = clBlack; d[3] = clBlack; d[4] = clGrayL; d+= 1024;
        d[0] = clGrayD; d[1] = clGrayL; d[2] = clGrayL; d[3] = clGrayL; d[4] = clGrayL; d+= 1024;
        */
    }

    void textOut(const vec2 &pos, const vec4 &color, char *text) {
        const static uint8 char_width[110] = {
            14, 11, 11, 11, 11, 11, 11, 13, 8, 11, 12, 11, 13, 13, 12, 11, 12, 12, 11, 12, 13, 13, 13, 12, 
            12, 11, 9, 9, 9, 9, 9, 9, 9, 9, 5, 9, 9, 5, 12, 10, 9, 9, 9, 8, 9, 8, 9, 9, 11, 9, 9, 9, 12, 8,
            10, 10, 10, 10, 10, 9, 10, 10, 5, 5, 5, 11, 9, 10, 8, 6, 6, 7, 7, 3, 11, 8, 13, 16, 9, 4, 12, 12, 
            7, 5, 7, 7, 7, 7, 7, 7, 7, 7, 16, 14, 14, 14, 16, 16, 16, 16, 16, 12, 14, 8, 8, 8, 8, 8, 8, 8 }; 
        
        static const uint8 char_map[102] = {
            0, 64, 66, 78, 77, 74, 78, 79, 69, 70, 92, 72, 63, 71, 62, 68, 52, 53, 54, 55, 56, 57, 58, 59, 
            60, 61, 73, 73, 66, 74, 75, 65, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 
            18, 19, 20, 21, 22, 23, 24, 25, 80, 76, 81, 97, 98, 77, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 
            37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 100, 101, 102, 67, 0, 0, 0, 0, 0, 0, 0 };
        
        if (!text) return;
        
        Core::active.shader->setParam(uColor, color);
        //text = "a: b";
        mat4 m;
        m.identity();
        m.translate(vec3(pos.x, pos.y, 0.0f));
       // text = "A";
        while (char c = *text++) {
            
            int frame = c > 10 ? (c > 15 ? char_map[c - 32] : c + 91) : c + 81; 


            //int frame = T_remapASCII[c - 32];
            /*
            if (c >= 'A' && c <= 'Z')
                frame = c - 'A';
            else if (c >= 'a' && c <= 'z')
                frame = 26 + c - 'a';
            else if (c >= '0' && c <= '9')
                frame = 52 + c - '0';
            else {
                if (c == ' ')
                    m.translate(vec3(16, 0.0f, 0.0f));
                continue;
            }
            */
            if (c == ' ' || c == '_') {
                m.translate(vec3(char_width[0], 0.0f, 0.0f));
                continue;
            }

            Core::active.shader->setParam(uModel, m);
            renderSprite(15, frame);
            m.translate(vec3(char_width[frame], 0.0f, 0.0f));
        }
    }
};

#endif