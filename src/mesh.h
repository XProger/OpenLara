#ifndef H_MESH
#define H_MESH

#include "core.h"
#include "format.h"

typedef unsigned short Index;

struct Vertex {
    short4  coord;      // xyz  - position, w - unused
    short4  texCoord;   // xy   - texture coordinates, z - anim tex range index, w - anim tex frame index
    short4  normal;     // xyz  - vertex normalá w - unused
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
        glVertexAttribPointer(aNormal,   4, GL_SHORT,         true,  sizeof(Vertex), &v->normal);
        glVertexAttribPointer(aColor,    4, GL_UNSIGNED_BYTE, true,  sizeof(Vertex), &v->color);
    }

    void bind(GLuint *VAO) const {
        if (aIndex > -1) {
            if (Core::active.VAO != VAO[aIndex]) {
                glBindVertexArray(Core::active.VAO = VAO[aIndex]);
            }
        } else
            setup();        
    }
};

#define PLANE_DETAIL 48
#define CIRCLE_SEGS  16

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

    void DIP(const MeshRange &range) {
        glDrawElements(GL_TRIANGLES, range.iCount, GL_UNSIGNED_SHORT, (GLvoid*)(range.iStart * sizeof(Index)));
        Core::stats.dips++;
        Core::stats.tris += range.iCount / 3;
    }

    void render(const MeshRange &range) {
        range.bind(VAO);

        if (Core::active.stencilTwoSide && Core::support.stencil == 0) {
            Core::setCulling(cfBack);
            glStencilOp(GL_KEEP, GL_DECR, GL_KEEP);
            DIP(range);
            Core::setCulling(cfFront);
            glStencilOp(GL_KEEP, GL_INCR, GL_KEEP);
        }

        DIP(range);
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

float intensityf(uint16 lighting) {
    if (lighting > 0x1FFF) return 1.0f;
    float lum = 1.0f - (lighting >> 5) / 255.0f;
    //return powf(lum, 2.2f); // gamma to linear space
    return lum;// * lum; // gamma to "linear" space
}

uint8 intensity(int lighting) {
    return uint8(intensityf(lighting) * 255);
}

struct MeshBuilder {
    Mesh *mesh;
// level
    struct RoomRange {
        MeshRange geometry[2]; // opaque & transparent
        MeshRange sprites;
        MeshRange **meshes;
    } *rooms;
    struct ModelRange {
        MeshRange geometry;
        bool      opaque;
    } *models;
    MeshRange *sequences;
// procedured
    MeshRange shadowBlob, shadowBox;
    MeshRange bar;
    MeshRange quad, circle;
    MeshRange plane;

    vec2 *animTexRanges;
    vec2 *animTexOffsets;

    int animTexRangesCount;
    int animTexOffsetsCount;

    TR::Level *level;
    TR::ObjectTexture whiteTile;

    MeshBuilder(TR::Level &level) : level(&level) {
        initAnimTextures(level);

    // create dummy white object textures for non-textured (colored) geometry        
        whiteTile.attribute = 0;
        whiteTile.tile.index = 15;
        whiteTile.tile.triangle = 0;
        whiteTile.texCoord[0] = 
        whiteTile.texCoord[1] = 
        whiteTile.texCoord[2] = 
        whiteTile.texCoord[3] = { 253, 253 };

    // allocate room geometry ranges
        rooms = new RoomRange[level.roomsCount];

        int iCount = 0, vCount = 0;

    // get size of mesh for rooms (geometry & sprites)
        for (int i = 0; i < level.roomsCount; i++) {
            TR::Room       &r = level.rooms[i];
            TR::Room::Data &d = r.data;

            iCount += d.rCount * 6 + d.tCount * 3;
            vCount += d.rCount * 4 + d.tCount * 3;

            if (Core::settings.water)
                roomRemoveWaterSurfaces(r, iCount, vCount);
            
            for (int j = 0; j < r.meshesCount; j++) {
                TR::Room::Mesh &m = r.meshes[j];
                TR::StaticMesh *s = level.getMeshByID(m.meshID);
                if (!level.meshOffsets[s->mesh]) continue;
                TR::Mesh &mesh = level.meshes[level.meshOffsets[s->mesh]];

                iCount += mesh.rCount * 6 + mesh.tCount * 3;
                vCount += mesh.rCount * 4 + mesh.tCount * 3;
            }

            RoomRange &range = rooms[i];
            range.sprites.vStart = vCount;
            range.sprites.iStart = iCount;
            iCount += d.sCount * 6;
            vCount += d.sCount * 4;
            range.sprites.iCount = iCount - range.sprites.iStart;

            ASSERT(vCount - range.sprites.vStart < 0xFFFF);
        }

    // get models info
        models = new ModelRange[level.modelsCount];
        for (int i = 0; i < level.modelsCount; i++) {
            TR::Model &model = level.models[i];
            for (int j = 0; j < model.mCount; j++) {
                int index = level.meshOffsets[model.mStart + j];
                if (!index && model.mStart + j > 0) 
                    continue;
                TR::Mesh &mesh = level.meshes[index];
                iCount += mesh.rCount * 6 + mesh.tCount * 3;
                vCount += mesh.rCount * 4 + mesh.tCount * 3;
            }
        }

    // get size of mesh for sprite sequences
        sequences = new MeshRange[level.spriteSequencesCount];
        for (int i = 0; i < level.spriteSequencesCount; i++) {
            sequences[i].vStart = vCount;
            sequences[i].iStart = iCount;
            sequences[i].iCount = level.spriteSequences[i].sCount * 6;
            iCount += level.spriteSequences[i].sCount * 6;
            vCount += level.spriteSequences[i].sCount * 4;
        }

    // shadow blob mesh (8 triangles, 8 vertices)
        shadowBlob.vStart = vCount;
        shadowBlob.iStart = iCount;
        shadowBlob.iCount = 8 * 3 * 3;
        iCount += shadowBlob.iCount;
        vCount += 8 * 2 + 1;

    // shadow box (for stencil shadow volumes with degenerate triangles)
        shadowBox.vStart = vCount;
        shadowBox.iStart = iCount;
        shadowBox.iCount = (3 * (2 + 4)) * 6;
        iCount += shadowBox.iCount;
        vCount += 4 * 6;

    // bar (health, oxygen)
        bar.vStart = vCount;
        bar.iStart = iCount;
        bar.iCount = 2 * 3;
        iCount += bar.iCount;
        vCount += 4;

    // quad (post effect filter)
        quad.vStart = vCount;
        quad.iStart = iCount;
        quad.iCount = 2 * 3;
        iCount += quad.iCount;
        vCount += 4;

    // circle
        circle.vStart = vCount;
        circle.iStart = iCount;
        circle.iCount = CIRCLE_SEGS * 3;
        iCount += circle.iCount;
        vCount += CIRCLE_SEGS + 1;

    // detailed plane
        plane.vStart = vCount;
        plane.iStart = iCount;
        plane.iCount = PLANE_DETAIL * 2 * PLANE_DETAIL * 2 * (2 * 3);
        iCount += plane.iCount;
        vCount += (PLANE_DETAIL * 2 + 1) * (PLANE_DETAIL * 2 + 1);

    // make meshes buffer (single vertex buffer object for all geometry & sprites on level)
        Index  *indices  = new Index[iCount];
        Vertex *vertices = new Vertex[vCount];
        iCount = vCount = 0;
        int aCount = 0;

    // build rooms
        for (int i = 0; i < level.roomsCount; i++) {
            TR::Room &room = level.rooms[i];
            TR::Room::Data &d = room.data;
            RoomRange &range = rooms[i];

            int vStart;

            for (int transp = 0; transp < 2; transp++) { // opaque, opacity
                range.geometry[transp].vStart = vCount;
                range.geometry[transp].iStart = iCount;

                vStart = vCount;
            // rooms geometry
                buildRoom(!transp, room, level, indices, vertices, iCount, vCount, vStart);

            // static meshes
                for (int j = 0; j < room.meshesCount; j++) {
                    TR::Room::Mesh &m = room.meshes[j];
                    TR::StaticMesh *s = level.getMeshByID(m.meshID);
                    if (!level.meshOffsets[s->mesh]) continue;
                    TR::Mesh &mesh = level.meshes[level.meshOffsets[s->mesh]];

                    int x = m.x - room.info.x;
                    int y = m.y;
                    int z = m.z - room.info.z;
                    int d = m.rotation.value / 0x4000;
                    buildMesh(!transp, mesh, level, indices, vertices, iCount, vCount, vStart, 0, x, y, z, d);
                }
                range.geometry[transp].iCount = iCount - range.geometry[transp].iStart;

                if (range.geometry[transp].iCount)
                    aCount++;
            }

        // rooms sprites
            vStart = vCount;
            for (int j = 0; j < d.sCount; j++) {
                TR::Room::Data::Sprite &f = d.sprites[j];
                TR::Room::Data::Vertex &v = d.vertices[f.vertex];
                TR::SpriteTexture &sprite = level.spriteTextures[f.texture];

                addSprite(indices, vertices, iCount, vCount, vStart, v.vertex.x, v.vertex.y, v.vertex.z, sprite, intensity(v.lighting));
            }

            if (d.sCount) aCount++;
        }

    // build models geometry
        for (int i = 0; i < level.modelsCount; i++) {
            TR::Model &model = level.models[i];
            ModelRange &range = models[i];
            int vStart = vCount;
            range.geometry.vStart = vStart;
            range.geometry.iStart = iCount;
            range.opaque = true;

            for (int j = 0; j < model.mCount; j++) {
                int index = level.meshOffsets[model.mStart + j];
                if (!index && model.mStart + j > 0) continue;
                aCount++;
                TR::Mesh &mesh = level.meshes[index];
                bool opaque = buildMesh(true, mesh, level, indices, vertices, iCount, vCount, vStart, j, 0, 0, 0, 0);
                if (!opaque)
                    buildMesh(false, mesh, level, indices, vertices, iCount, vCount, vStart, j, 0, 0, 0, 0);
                range.opaque &= opaque;
            }
            range.geometry.iCount = iCount - range.geometry.iStart;
        }

    // build sprite sequences
        for (int i = 0; i < level.spriteSequencesCount; i++) 
            for (int j = 0; j < level.spriteSequences[i].sCount; j++) {
                TR::SpriteTexture &sprite = level.spriteTextures[level.spriteSequences[i].sStart + j];
                addSprite(indices, vertices, iCount, vCount, sequences[i].vStart, 0, 0, 0, sprite, 255);
            }
        aCount += level.spriteSequencesCount;

    // build shadow blob
        for (int i = 0; i < 9; i++) {
            Vertex &v0 = vertices[vCount + i * 2 + 0];
            v0.normal    = { 0, -1, 0, 1 };
            v0.texCoord  = { 32688, 32688, 0, 0 };
            v0.color     = { 0, 0, 0, 0 };

            if (i == 8) {
                v0.coord = { 0, 0, 0, 0 };
                break;
            }

            float a = i * (PI / 4.0f) + (PI / 8.0f);
            float c = cosf(a);
            float s = sinf(a);
            short c0 = short(c * 256.0f);
            short s0 = short(s * 256.0f);
            short c1 = short(c * 512.0f);
            short s1 = short(s * 512.0f);
            v0.coord = { c0, 0, s0, 0 };

            Vertex &v1 = vertices[vCount + i * 2 + 1];
            v1 = v0;
            v1.coord = { c1, 0, s1, 0 };
            v1.color = { 255, 255, 255, 0 };

            int idx = iCount + i * 3 * 3;
            int j = ((i + 1) % 8) * 2;
            indices[idx++] = i * 2;
            indices[idx++] = 8 * 2;
            indices[idx++] = j;

            indices[idx++] = i * 2 + 1;
            indices[idx++] = i * 2;
            indices[idx++] = j;

            indices[idx++] = i * 2 + 1;
            indices[idx++] = j;
            indices[idx++] = j + 1;
        }
        vCount += 8 * 2 + 1;
        iCount += shadowBlob.iCount;
        aCount++;

    // build shadow box volume
        {
            static const Index tmpIndices[] = {
                0,1,2, 0,2,3, 0,7,1, 0,4,7, 
                1,11,2, 1,8,11, 2,15,3, 2,12,15,
                3,19,0, 3,16,19, 21,6,5, 21,20,6, 
                20,10,9, 20,23,10, 23,14,13, 23,22,14,
                22,18,17, 22,21,18, 20,21,22, 20,22,23,
                7,6,9, 7,9,8, 11,10,13, 11,13,12,
                15,14,17, 15,17,16, 19,5,4, 19,18,5,
                4,6,7, 4,5,6, 8,9,10, 8,10,11,
                12,14,15, 12,13,14, 16,18,19, 16,17,18
            };
            static const short4 tmpCoords[] = {
                { -1, -1, -1, 0 }, {  1, -1, -1, 0 }, {  1,  1, -1, 0 }, { -1,  1, -1, 0 },
                { -1, -1, -1, 0 }, { -1, -1,  1, 0 }, {  1, -1,  1, 0 }, {  1, -1, -1, 0 },
                {  1, -1, -1, 0 }, {  1, -1,  1, 0 }, {  1,  1,  1, 0 }, {  1,  1, -1, 0 },
                {  1,  1, -1, 0 }, {  1,  1,  1, 0 }, { -1,  1,  1, 0 }, { -1,  1, -1, 0 },
                { -1,  1, -1, 0 }, { -1,  1,  1, 0 }, { -1, -1,  1, 0 }, { -1, -1, -1, 0 },
                {  1, -1,  1, 0 }, { -1, -1,  1, 0 }, { -1,  1,  1, 0 }, {  1,  1,  1, 0 },
            };

            const short n = 32767;
            static const short4 tmpNormals[] = {
                {  0,  0, -n, 0 },
                {  0, -n,  0, 0 },
                {  n,  0,  0, 0 },
                {  0,  n,  0, 0 },
                { -n,  0,  0, 0 },
                {  0,  0,  n, 0 },
            };

            static const ubyte4 tmpColors[] = {
                {  255,   0,   0, 0 },
                {    0, 255,   0, 0 },
                {    0,   0, 255, 0 },
                {  255,   0, 255, 0 },
                {  255, 255,   0, 0 },
                {    0, 255, 255, 0 },
            };

            memcpy(&indices[iCount], &tmpIndices[0], shadowBox.iCount * sizeof(Index));
            memset(&vertices[vCount], 0, 4 * 6 * sizeof(Vertex));
            for (int i = 0; i < 4 * 6; i++) {
                vertices[vCount + i].coord  = tmpCoords[i];
                vertices[vCount + i].normal = tmpNormals[i / 4];
                vertices[vCount + i].color  = tmpColors[i / 4];
            }            
            iCount += shadowBox.iCount;
            vCount += 4 * 6;
            aCount++;
        }

    // white bar
        addQuad(indices, iCount, vCount, bar.vStart, vertices, &whiteTile);
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
        vCount += 4;
        aCount++;

    // quad
        addQuad(indices, iCount, vCount, quad.vStart, vertices, &whiteTile);
        vertices[vCount + 3].coord = { -1, -1, 0, 0 };
        vertices[vCount + 2].coord = {  1, -1, 1, 0 };
        vertices[vCount + 1].coord = {  1,  1, 1, 1 };
        vertices[vCount + 0].coord = { -1,  1, 0, 1 };

        for (int i = 0; i < 4; i++) {
            Vertex &v = vertices[vCount + i];
            v.normal    = { 0, 0, 0, 0 };
            v.color     = { 255, 255, 255, 255 };
            v.texCoord  = { 32688, 32688, 0, 0 };
        }
        vCount += 4;
        aCount++;

    // circle
        vec2 pos(32767.0f, 0.0f);
        vec2 cs(cos(PI2 / CIRCLE_SEGS), sin(PI2 / CIRCLE_SEGS));

        for (int i = 0; i < CIRCLE_SEGS; i++) {
            Vertex &v = vertices[vCount + i];
            pos.rotate(cs);
            v.coord     = { short(pos.x), short(pos.y), 0, 0 };
            v.normal    = { 0, 0, 0, 0 };
            v.color     = { 255, 255, 255, 255 };
            v.texCoord  = { 32688, 32688, 0, 0 };

            indices[iCount++] = i;
            indices[iCount++] = (i + 1) % CIRCLE_SEGS;
            indices[iCount++] = CIRCLE_SEGS;
        }
        vertices[vCount + CIRCLE_SEGS] = vertices[vCount];
        vertices[vCount + CIRCLE_SEGS].coord = { 0, 0, 0, 0 };

        vCount += CIRCLE_SEGS + 1;
        aCount++;

    // plane
        for (int16 j = -PLANE_DETAIL; j <= PLANE_DETAIL; j++)
            for (int16 i = -PLANE_DETAIL; i <= PLANE_DETAIL; i++) {
                vertices[vCount++].coord = { i, j, 0, 0 };
                if (j < PLANE_DETAIL && i < PLANE_DETAIL) {
                    int idx = (j + PLANE_DETAIL) * (PLANE_DETAIL * 2 + 1) + i + PLANE_DETAIL;
                    indices[iCount + 0] = idx + PLANE_DETAIL * 2 + 1;
                    indices[iCount + 1] = idx + 1;
                    indices[iCount + 2] = idx;
                    indices[iCount + 3] = idx + PLANE_DETAIL * 2 + 2;
                    indices[iCount + 4] = idx + 1;
                    indices[iCount + 5] = idx + PLANE_DETAIL * 2 + 1;
                    iCount += 6;
                }
            }
        aCount++;
        LOG("MegaMesh: %d %d %d\n", iCount, vCount, aCount);

    // compile buffer and ranges
        mesh = new Mesh(indices, iCount, vertices, vCount, aCount);
        delete[] indices;
        delete[] vertices;

        PROFILE_LABEL(BUFFER, mesh->ID[0], "Geometry indices");
        PROFILE_LABEL(BUFFER, mesh->ID[1], "Geometry vertices");

        // initialize Vertex Arrays
        for (int i = 0; i < level.roomsCount; i++) {
            RoomRange &r = rooms[i];
            if (r.geometry[0].iCount)
                mesh->initRange(r.geometry[0]);
            if (r.geometry[1].iCount)
                mesh->initRange(r.geometry[1]);
            if (r.sprites.iCount)
                mesh->initRange(r.sprites);
        }

        for (int i = 0; i < level.spriteSequencesCount; i++)
            mesh->initRange(sequences[i]);     
        for (int i = 0; i < level.modelsCount; i++)
            mesh->initRange(models[i].geometry);
        mesh->initRange(shadowBlob);
        mesh->initRange(shadowBox);
        mesh->initRange(bar);
        mesh->initRange(quad);
        mesh->initRange(circle);
        mesh->initRange(plane);
    }

    ~MeshBuilder() {
        delete[] animTexRanges;
        delete[] animTexOffsets;
        delete[] rooms;
        delete[] models;
        delete[] sequences;
        delete mesh;
    }

    inline short4 rotate(const short4 &v, int dir) {
        if (dir == 0) return v;
        short4 res = v;

        switch (dir) {
            case 1  : res.x =  v.z, res.z = -v.x; break;
            case 2  : res.x = -v.x, res.z = -v.z; break;
            case 3  : res.x = -v.z, res.z =  v.x; break;
            default : ASSERT(false);
        }
        return res;
    }

    inline short4 transform(const short4 &v, int joint, int x, int y, int z, int dir) {
        short4 res = rotate(v, dir);
        res.x += x;
        res.y += y;
        res.z += z;
        res.w = joint;
        return res;
    }

    bool roomCheckWaterPortal(TR::Room room) {
        for (int i = 0; i < room.portalsCount; i++)
            if (room.flags.water ^ level->rooms[room.portals[i].roomIndex].flags.water)
                return true;
        return false;
    }

    void roomRemoveWaterSurfaces(TR::Room &room, int &iCount, int &vCount) {
        if (!roomCheckWaterPortal(room)) return;

    // remove animated water polygons from room geometry
        for (int i = 0; i < room.data.rCount; i++) {
            TR::Rectangle &f = room.data.rectangles[i];
            if (f.vertices[0] == 0xFFFF) continue;

            TR::Vertex &a = room.data.vertices[f.vertices[0]].vertex;
            TR::Vertex &b = room.data.vertices[f.vertices[1]].vertex;
            TR::Vertex &c = room.data.vertices[f.vertices[2]].vertex;
            TR::Vertex &d = room.data.vertices[f.vertices[3]].vertex;

            if (a.y != b.y || a.y != c.y || a.y != d.y) // skip non-horizontal or non-portal plane primitive
                continue;
            
            int sx = (int(a.x) + int(b.x) + int(c.x) + int(d.x)) / 4 / 1024;
            int sz = (int(a.z) + int(b.z) + int(c.z) + int(d.z)) / 4 / 1024;

            TR::Room::Sector &s = room.sectors[sx * room.zSectors + sz];

            int yt = abs(a.y - s.ceiling * 256);
            int yb = abs(s.floor * 256 - a.y);

            if (yt > 0 && yb > 0) continue;

            if ((yt == 0 && s.roomAbove != TR::NO_ROOM && (level->rooms[s.roomAbove].flags.water ^ room.flags.water)) ||
                (yb == 0 && s.roomBelow != TR::NO_ROOM && (level->rooms[s.roomBelow].flags.water ^ room.flags.water))) {
                f.vertices[0] = 0xFFFF; // mark as unused
                iCount -= 6;
                vCount -= 4;
            }
        }

        for (int i = 0; i < room.data.tCount; i++) {
            TR::Triangle &f = room.data.triangles[i];
            if (f.vertices[0] == 0xFFFF) continue;

            TR::Vertex &a = room.data.vertices[f.vertices[0]].vertex;
            TR::Vertex &b = room.data.vertices[f.vertices[1]].vertex;
            TR::Vertex &c = room.data.vertices[f.vertices[2]].vertex;

            if (a.y != b.y || a.y != c.y) // skip non-horizontal or non-portal plane primitive
                continue;
            
            int sx = (int(a.x) + int(b.x) + int(c.x)) / 3 / 1024;
            int sz = (int(a.z) + int(b.z) + int(c.z)) / 3 / 1024;

            TR::Room::Sector &s = room.sectors[sx * room.zSectors + sz];

            int yt = abs(a.y - s.ceiling * 256);
            int yb = abs(s.floor * 256 - a.y);

            if (yt > 0 && yb > 0) continue;

            if ((yt <= 1 && s.roomAbove != TR::NO_ROOM && (level->rooms[s.roomAbove].flags.water ^ room.flags.water)) ||
                (yb <= 1 && s.roomBelow != TR::NO_ROOM && (level->rooms[s.roomBelow].flags.water ^ room.flags.water))) {
                f.vertices[0] = 0xFFFF; // mark as unused
                iCount -= 3;
                vCount -= 3;
            }
        }
    }

    bool buildRoom(bool opaque, const TR::Room &room, const TR::Level &level, Index *indices, Vertex *vertices, int &iCount, int &vCount, int vStart) {
        const TR::Room::Data &d = room.data;
        bool isOpaque = true;

        for (int j = 0; j < d.rCount; j++) {
            TR::Rectangle     &f = d.rectangles[j];
            TR::ObjectTexture &t = level.objectTextures[f.texture];

            if (f.vertices[0] == 0xFFFF) continue; // skip if marks as unused (removing water planes)

            if (t.attribute != 0)
                isOpaque = false;

            if (opaque != (t.attribute == 0))
                continue;

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

            if (f.vertices[0] == 0xFFFF) continue; // skip if marks as unused (removing water planes)

            if (t.attribute != 0)
                isOpaque = false;

            if (opaque != (t.attribute == 0))
                continue;

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

        return isOpaque;
    }

    bool buildMesh(bool opaque, const TR::Mesh &mesh, const TR::Level &level, Index *indices, Vertex *vertices, int &iCount, int &vCount, int vStart, int16 joint, int x, int y, int z, int dir) {
        TR::Color24 COLOR_WHITE = { 255, 255, 255 };
        bool isOpaque = true;

        for (int j = 0; j < mesh.rCount; j++) {
            TR::Rectangle &f = mesh.rectangles[j];
            bool textured = !(f.texture & 0x8000);
            TR::ObjectTexture &t = textured ? level.objectTextures[f.texture] : whiteTile;

            if (t.attribute != 0)
                isOpaque = false;

            if (opaque != (t.attribute == 0))
                continue;

            TR::Color24 c = textured ? COLOR_WHITE : level.getColor(f.texture);

            addQuad(indices, iCount, vCount, vStart, vertices, &t);

            for (int k = 0; k < 4; k++) {
                TR::Mesh::Vertex &v = mesh.vertices[f.vertices[k]];

                vertices[vCount].coord  = transform(v.coord, joint, x, y, z, dir);
                vertices[vCount].normal = rotate(v.normal, dir);
                vertices[vCount].color  = { c.r, c.g, c.b, intensity(v.coord.w) };

                vCount++;
            }
        }

        for (int j = 0; j < mesh.tCount; j++) {
            TR::Triangle &f = mesh.triangles[j];
            bool textured = !(f.texture & 0x8000);
            TR::ObjectTexture &t = textured ? level.objectTextures[f.texture] : whiteTile;

            if (t.attribute != 0)
                isOpaque = false;

            if (opaque != (t.attribute == 0))
                continue;

            TR::Color24 c = textured ? COLOR_WHITE : level.getColor(f.texture);

            addTriangle(indices, iCount, vCount, vStart, vertices, &t);

            for (int k = 0; k < 3; k++) {
                TR::Mesh::Vertex &v = mesh.vertices[f.vertices[k]];

                vertices[vCount].coord  = transform(v.coord, joint, x, y, z, dir);
                vertices[vCount].normal = rotate(v.normal, dir);
                vertices[vCount].color  = { c.r, c.g, c.b, intensity(v.coord.w) };

                vCount++;
            }
        }

        return isOpaque;
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
            v.texCoord.x = ((tx + tex->texCoord[i].x) << 5) + 16;
            v.texCoord.y = ((ty + tex->texCoord[i].y) << 5) + 16;
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

        int16 u0 = ((tx + sprite.texCoord[0].x + 1) << 5);
        int16 v0 = ((ty + sprite.texCoord[0].y + 1) << 5);
        int16 u1 = ((tx + sprite.texCoord[1].x - 1) << 5);
        int16 v1 = ((ty + sprite.texCoord[1].y - 1) << 5);

        quad[0].texCoord = { u0, v0, sprite.l, sprite.t };
        quad[1].texCoord = { u1, v0, sprite.r, sprite.t };
        quad[2].texCoord = { u1, v1, sprite.r, sprite.b };
        quad[3].texCoord = { u0, v1, sprite.l, sprite.b };

        vCount += 4;
    }

    void bind() {
        mesh->bind();
    }

    void renderRoomGeometry(int roomIndex, bool transparent) {
        ASSERT(rooms[roomIndex].geometry[transparent].iCount > 0);
        mesh->render(rooms[roomIndex].geometry[transparent]);
    }

    void renderRoomSprites(int roomIndex) {
        mesh->render(rooms[roomIndex].sprites);
    }

    void renderModel(int modelIndex) {
        mesh->render(models[modelIndex].geometry);
    }

    void renderSprite(int sequenceIndex, int frame) {
        MeshRange range = sequences[sequenceIndex];
        range.iCount  = 6;
        range.iStart += frame * 6;
        mesh->render(range);
    }

    void renderShadowBlob() {
        mesh->render(shadowBlob);
    }

    void renderShadowBox() {
        mesh->render(shadowBox);
    }

    void renderQuad() {
        mesh->render(quad);
    }

    void renderCircle() {
        mesh->render(circle);
    }

    void renderPlane() {
        mesh->render(plane);
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
        
        Core::active.shader->setParam(uMaterial, vec4(1.0, 0.0, 0.0, 1.0));
        //text = "a: b";
        Basis basis;
        basis.identity();
        basis.translate(vec3(pos.x, pos.y, 0.0f));
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
                basis.translate(vec3(char_width[0], 0.0f, 0.0f));
                continue;
            }

            Core::active.shader->setParam(uBasis, basis);
            renderSprite(15, frame);
            basis.translate(vec3(char_width[frame], 0.0f, 0.0f));
        }
    }
};

#endif