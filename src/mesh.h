#ifndef H_MESH
#define H_MESH

#include "core.h"
#include "format.h"

TR::TextureInfo barTile[5 /* UI::BAR_MAX */];
TR::TextureInfo &whiteTile = barTile[4]; // BAR_WHITE

#define PLANE_DETAIL 48
#define CIRCLE_SEGS  16

#define DYN_MESH_FACES     2048
#define DOUBLE_SIDED       2

#define WATER_VOLUME_HEIGHT (768 * 2)
#define WATER_VOLUME_OFFSET 4

const Color32 COLOR_WHITE( 255, 255, 255, 255 );

struct Mesh : GAPI::Mesh {
    int aIndex;

    Mesh(Index *indices, int iCount, Vertex *vertices, int vCount, int aCount, bool dynamic) : GAPI::Mesh(dynamic), aIndex(0) {
        init(indices, iCount, vertices, vCount, aCount);
    }

    virtual ~Mesh() {
        deinit();
    }

    void initRange(MeshRange &range) {
        initNextRange(range, aIndex);
    }

    void render(const MeshRange &range) {
        Core::DIP(this, range);
    }
};

#define CHECK_ROOM_NORMAL(f) \
            vec3 o = d.vertices[f.vertices[0]].pos;\
            vec3 a = o - d.vertices[f.vertices[1]].pos;\
            vec3 b = o - d.vertices[f.vertices[2]].pos;\
            o = b.cross(a).normal() * 32767.0f;\
            f.normal.x = (int)o.x;\
            f.normal.y = (int)o.y;\
            f.normal.z = (int)o.z;

#define ADD_ROOM_FACE(_indices, iCount, vCount, vStart, _vertices, f, t) \
            addFace(_indices, iCount, vCount, vStart, _vertices, f, &t,\
                    d.vertices[f.vertices[0]].pos,\
                    d.vertices[f.vertices[1]].pos,\
                    d.vertices[f.vertices[2]].pos,\
                    d.vertices[f.vertices[3]].pos);\
            for (int k = 0; k < (f.triangle ? 3 : 4); k++) {\
                TR::Room::Data::Vertex &v = d.vertices[f.vertices[k]];\
                Vertex &rv = _vertices[vCount++];\
                rv.coord  = short4( v.pos.x,    v.pos.y,    v.pos.z,    0 );\
                rv.normal = short4( f.normal.x, f.normal.y, f.normal.z, 0 );\
                rv.color  = ubyte4( 255, 255, 255, 255 );\
                rv.light  = ubyte4( v.color.r, v.color.g, v.color.b, 255 );\
            }


float intensityf(uint16 lighting) {
    ASSERT(lighting >= 0 && lighting <= 0x1FFF);
    return (0x1FFF - lighting) / float(0x1FFF);
}

uint8 intensity(int lighting) {
    return uint8(intensityf(lighting) * 255);
}

struct MeshBuilder {
    Index     dynIndices[DYN_MESH_FACES * 3];
    Vertex    dynVertices[DYN_MESH_FACES * 3];
    MeshRange dynRange;
    int       dynICount;
    int       dynVCount;
    Mesh      *dynMesh;

    Mesh      *mesh;
    Texture   *atlas;
    TR::Level *level;

// level
    struct Geometry {
        int       count;
        MeshRange ranges[100];

        Geometry() : count(0) {}

        void finish(int iCount) {
            MeshRange *range = count ? &ranges[count - 1] : NULL;

            if (range) {
                range->iCount = iCount - range->iStart;
                if (!range->iCount)
                    count--;
            }
        }

        bool validForTile(uint16 tile, uint16 clut) {
        #ifdef SPLIT_BY_TILE
            if (!count) return false;
            MeshRange &range = ranges[count - 1];

            return (tile == range.tile
                #ifdef SPLIT_BY_CLUT
                    && clut == range.clut
                #endif
                    );
        #else
            return count != 0;
        #endif
        }

        MeshRange* getNextRange(int vStart, int iCount, uint16 tile, uint16 clut) {
            MeshRange *range = count ? &ranges[count - 1] : NULL;

            if (range)
                range->iCount = iCount - range->iStart;
    
            if (!range || range->iCount) {
                ASSERT(count < COUNT(ranges));
                range = &ranges[count++];
            }

            range->vStart = vStart;
            range->iStart = iCount;
            range->tile   = tile;
            range->clut   = clut;

            return range;
        }
    };

    struct Dynamic {
        uint16 count;
        uint16 *faces;
    };

    struct RoomRange {
        Geometry  geometry[3]; // opaque, double-side alpha, additive
        Dynamic   dynamic[3];  // lists of dynamic polygons (with animated textures) like lava, waterfalls etc.
        MeshRange sprites;
        MeshRange waterVolume;
        int       split;
    } *rooms;

    struct ModelRange {
        int      parts[3][32];
        Geometry geometry[3];
    } *models;

// procedured
    MeshRange shadowBlob;
    MeshRange quad, circle, box;
    MeshRange plane;

    int transparent;

    #ifdef SPLIT_BY_TILE
        uint16 curTile, curClut;
    #endif

    enum {
        BLEND_NONE  = 1,
        BLEND_ALPHA = 2,
        BLEND_ADD   = 4,
    };

    MeshBuilder(TR::Level *level, Texture *atlas) : atlas(atlas), level(level) {
        dynMesh = new Mesh(NULL, COUNT(dynIndices), NULL, COUNT(dynVertices), 1, true);
        dynRange.vStart = 0;
        dynRange.iStart = 0;
        dynMesh->initRange(dynRange);

    // allocate room geometry ranges
        rooms = new RoomRange[level->roomsCount];

        int iCount = 0, vCount = 0;

    // sort room faces by material
        for (int i = 0; i < level->roomsCount; i++) {
            TR::Room::Data &data = level->rooms[i].data;
            sort(data.faces, data.fCount);
        // sort room sprites by material
            sort(data.sprites, data.sCount);
        }

    // sort mesh faces by material
        for (int i = 0; i < level->meshesCount; i++) {
            TR::Mesh &mesh = level->meshes[i];
            sort(mesh.faces, mesh.fCount);
        }

    // get size of mesh for rooms (geometry & sprites)
        int vStartRoom = vCount;

    // pre-calculate water level for rooms
        for (int i = 0; i < level->roomsCount; i++) {
            TR::Room &room = level->rooms[i];
            room.waterLevel[0] = room.waterLevel[1] = room.waterLevelSurface = TR::NO_WATER;
        }

        for (int j = 0; j < 2; j++) {
            for (int i = 0; i < level->roomsCount; i++)
                calcWaterLevel(i, level->state.flags.flipped);
            level->flipMap();
        }

    // get reooms geometry info
        for (int i = 0; i < level->roomsCount; i++) {
            TR::Room       &r = level->rooms[i];
            TR::Room::Data &d = r.data;

            int vStartCount = vCount;

            iCount += (d.rCount * 6 + d.tCount * 3) * DOUBLE_SIDED;
            vCount += (d.rCount * 4 + d.tCount * 3);

            roomRemoveWaterSurfaces(r, iCount, vCount);

            for (int j = 0; j < r.meshesCount; j++) {
                TR::Room::Mesh &m = r.meshes[j];
                TR::StaticMesh *s = &level->staticMeshes[m.meshIndex];
                if (!level->meshOffsets[s->mesh]) continue;
                TR::Mesh &mesh = level->meshes[level->meshOffsets[s->mesh]];

                iCount += (mesh.rCount * 6 + mesh.tCount * 3) * DOUBLE_SIDED;
                vCount += (mesh.rCount * 4 + mesh.tCount * 3);
            }

        #ifdef MERGE_SPRITES
            iCount += d.sCount * 6;
            vCount += d.sCount * 4;
        #endif

            if (vCount - vStartRoom > 0xFFFF) {
                vStartRoom = vStartCount;
                rooms[i].split = true;
            } else
                rooms[i].split = false;
        }

    // get models info
        models = new ModelRange[level->modelsCount];
        for (int i = 0; i < level->modelsCount; i++) {
            TR::Model &model = level->models[i];
            for (int j = 0; j < model.mCount; j++) {
                int index = level->meshOffsets[model.mStart + j];
                if (!index && model.mStart + j > 0) 
                    continue;
                TR::Mesh &mesh = level->meshes[index];
                iCount += (mesh.rCount * 6 + mesh.tCount * 3) * DOUBLE_SIDED;
                vCount += (mesh.rCount * 4 + mesh.tCount * 3);
            }
        }

    // shadow blob mesh (8 triangles, 8 vertices)
        iCount += 8 * 3 * 3;
        vCount += 8 * 2 + 1;

    // quad (post effect filter)
        iCount += 6;
        vCount += 4;

    // circle
        iCount += CIRCLE_SEGS * 3;
        vCount += CIRCLE_SEGS + 1;

    // box
        const Index boxIndices[] = {
            2,  1,  0,  3,  2,  0,
            4,  5,  6,  4,  6,  7,
            8,  9,  10, 8,  10, 11,
            14, 13, 12, 15, 14, 12,
            16, 17, 18, 16, 18, 19,
            22, 21, 20, 23, 22, 20,
        };

        const short4 boxCoords[] = {
            {-1, -1,  1, 0}, { 1, -1,  1, 0}, { 1,  1,  1, 0}, {-1,  1,  1, 0},
            { 1,  1,  1, 0}, { 1,  1, -1, 0}, { 1, -1, -1, 0}, { 1, -1,  1, 0},
            {-1, -1, -1, 0}, { 1, -1, -1, 0}, { 1,  1, -1, 0}, {-1,  1, -1, 0},
            {-1, -1, -1, 0}, {-1, -1,  1, 0}, {-1,  1,  1, 0}, {-1,  1, -1, 0},
            { 1,  1,  1, 0}, {-1,  1,  1, 0}, {-1,  1, -1, 0}, { 1,  1, -1, 0},
            {-1, -1, -1, 0}, { 1, -1, -1, 0}, { 1, -1,  1, 0}, {-1, -1,  1, 0},
        };

        iCount += COUNT(boxIndices);
        vCount += COUNT(boxCoords);

    // detailed plane
    #ifdef GENERATE_WATER_PLANE
        iCount += SQR(PLANE_DETAIL * 2) * 6;
        vCount += SQR(PLANE_DETAIL * 2 + 1);
    #endif

    // make meshes buffer (single vertex buffer object for all geometry & sprites on level)
        Index  *indices  = new Index[iCount];
        Vertex *vertices = new Vertex[vCount];
        iCount = vCount = 0;
        int aCount = 0;

    // build rooms
        vStartRoom = vCount;
        aCount++;

        for (int i = 0; i < level->roomsCount; i++) {
            TR::Room &room = level->rooms[i];
            TR::Room::Data &d = room.data;
            RoomRange &range = rooms[i];

            if (range.split) {
                vStartRoom = vCount;
                aCount++;
            }

            range.waterVolume.iCount = 0;
            if (Core::settings.detail.water > Core::Settings::MEDIUM)
                buildWaterVolume(i, indices, vertices, iCount, vCount, vStartRoom);

            for (int transp = 0; transp < 3; transp++) { // opaque, opacity
                int blendMask = getBlendMask(transp);

                Geometry &geom = range.geometry[transp];

            // rooms geometry
                buildRoom(geom, range.dynamic[transp], blendMask, room, level, indices, vertices, iCount, vCount, vStartRoom);

            // static meshes
                for (int j = 0; j < room.meshesCount; j++) {
                    TR::Room::Mesh &m = room.meshes[j];
                    TR::StaticMesh *s = &level->staticMeshes[m.meshIndex];
                    if (!level->meshOffsets[s->mesh]) continue;
                    TR::Mesh &mesh = level->meshes[level->meshOffsets[s->mesh]];

                    int x = m.x - room.info.x;
                    int y = m.y;
                    int z = m.z - room.info.z;
                    int d = m.rotation.value / 0x4000;
                    buildMesh(geom, blendMask, mesh, level, indices, vertices, iCount, vCount, vStartRoom, 0, x, y, z, d, m.color);
                }

                geom.finish(iCount);
            }

        // rooms sprites
        #ifdef MERGE_SPRITES
            range.sprites.vStart = vStartRoom;
            range.sprites.iStart = iCount;
            for (int j = 0; j < d.sCount; j++) {
                TR::Room::Data::Sprite &f = d.sprites[j];
                TR::Room::Data::Vertex &v = d.vertices[f.vertexIndex];
                TR::TextureInfo &sprite = level->spriteTextures[f.texture];

                addSprite(indices, vertices, iCount, vCount, vStartRoom, v.pos.x, v.pos.y, v.pos.z, false, sprite, v.color, v.color);
            }
            range.sprites.iCount = iCount - range.sprites.iStart;
        #else
            range.sprites.iCount = d.sCount * 6;
        #endif
        }
        ASSERT(vCount - vStartRoom <= 0xFFFF);

    // build models geometry
        int vStartModel = vCount;
        aCount++;

        for (int i = 0; i < level->modelsCount; i++) {
            TR::Model &model = level->models[i];

            int vCountStart = vCount;

            for (int transp = 0; transp < 3; transp++) {
                Geometry &geom = models[i].geometry[transp];

                int blendMask = getBlendMask(transp);

                for (int j = 0; j < model.mCount; j++) {
                    #ifndef MERGE_MODELS
                        models[i].parts[transp][j] = geom.count;
                    #endif

                    int index = level->meshOffsets[model.mStart + j];
                    if (index || model.mStart + j <= 0) {
                        TR::Mesh &mesh = level->meshes[index];
                        #ifndef MERGE_MODELS
                            geom.getNextRange(vStartModel, iCount, 0xFFFF, 0xFFFF);
                        #endif
                        buildMesh(geom, blendMask, mesh, level, indices, vertices, iCount, vCount, vStartModel, j, 0, 0, 0, 0, COLOR_WHITE);
                    }

                    #ifndef MERGE_MODELS
                        geom.finish(iCount);
                        models[i].parts[transp][j] = geom.count - models[i].parts[transp][j];
                    #endif
                }

                #ifdef MERGE_MODELS
                    geom.finish(iCount);
                    models[i].parts[transp][0] = geom.count;
                #endif
            }

            //int transp = TR::Entity::fixTransp(model.type);

            if (model.type == TR::Entity::SKY) {
                ModelRange &m = models[i];
                m.geometry[0].ranges[0].iCount = iCount - models[i].geometry[0].ranges[0].iStart;
                m.geometry[1].ranges[0].iCount = 0;
                m.geometry[2].ranges[0].iCount = 0;
            // remove bottom triangles from skybox
                //if (m.geometry[0].ranges[0].iCount && ((level.version & TR::VER_TR3)))
                //    m.geometry[0].ranges[0].iCount -= 16 * 3;
            // rotate TR2 skybox
                if (level->version & TR::VER_TR2) {
                    for (int j = vCountStart; j < vCount; j++) {
                        short4 &c = vertices[j].coord;
                        c = short4(c.x, -c.z, c.y, c.w);
                    }
                }
            }
        }
        ASSERT(vCount - vStartModel <= 0xFFFF);

    // build common primitives
        int vStartCommon = vCount;
        aCount++;

        shadowBlob.vStart = vStartCommon;
        shadowBlob.iStart = iCount;
        shadowBlob.iCount = 8 * 3 * 3;
        for (int i = 0; i < 9; i++) {
            Vertex &v0 = vertices[vCount + i * 2 + 0];
            v0.normal    = short4( 0, -1, 0, 32767 );
            v0.texCoord  = short4( whiteTile.texCoordAtlas[0].x, whiteTile.texCoordAtlas[0].y, 32767, 32767 );
            v0.color     = v0.light = ubyte4( 0, 0, 0, 0 );

            if (i == 8) {
                v0.coord = short4( 0, 0, 0, 0 );
                break;
            }

            float a = i * (PI / 4.0f) + (PI / 8.0f);
            float c = cosf(a);
            float s = sinf(a);
            short c0 = short(c * 256.0f);
            short s0 = short(s * 256.0f);
            short c1 = short(c * 512.0f);
            short s1 = short(s * 512.0f);
            v0.coord = short4( c0, 0, s0, 0 );

            Vertex &v1 = vertices[vCount + i * 2 + 1];
            v1 = v0;
            v1.coord = short4( c1, 0, s1, 0 );
            v1.color = v1.light = ubyte4( 255, 255, 255, 0 );

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

    // quad
        quad.vStart = vStartCommon;
        quad.iStart = iCount;
        quad.iCount = 2 * 3;

        addQuad(indices, iCount, vCount, vStartCommon, vertices, &whiteTile, false, false);
        vertices[vCount + 0].coord = short4( -32767,  32767, 0, 1 );
        vertices[vCount + 1].coord = short4(  32767,  32767, 1, 1 );
        vertices[vCount + 2].coord = short4(  32767, -32767, 1, 0 );
        vertices[vCount + 3].coord = short4( -32767, -32767, 0, 0 );

        vertices[vCount + 0].texCoord = short4(     0,  32767, 0, 0 );
        vertices[vCount + 1].texCoord = short4( 32767,  32767, 0, 0 );
        vertices[vCount + 2].texCoord = short4( 32767,      0, 0, 0 );
        vertices[vCount + 3].texCoord = short4(     0,      0, 0, 0 );

        for (int i = 0; i < 4; i++) {
            Vertex &v = vertices[vCount++];
            v.normal  = short4( 0, 0, 0, 0 );
            v.color   = ubyte4( 255, 255, 255, 255 );
            v.light   = ubyte4( 255, 255, 255, 255 );
        }

    // circle
        circle.vStart = vStartCommon;
        circle.iStart = iCount;
        circle.iCount = CIRCLE_SEGS * 3;

        vec2 pos(32767.0f, 0.0f);
        vec2 cs(cosf(PI2 / CIRCLE_SEGS), sinf(PI2 / CIRCLE_SEGS));

        int baseIdx = vCount - vStartCommon;
        for (int i = 0; i < CIRCLE_SEGS; i++) {
            Vertex &v = vertices[vCount + i];
            pos.rotate(cs);
            v.coord     = short4( short(pos.x), short(pos.y), 0, 0 );
            v.normal    = short4( 0, 0, 0, 32767 );
            v.texCoord  = short4( whiteTile.texCoordAtlas[0].x, whiteTile.texCoordAtlas[0].y, 32767, 32767 );
            v.color     = ubyte4( 255, 255, 255, 255 );
            v.light     = ubyte4( 255, 255, 255, 255 );

            indices[iCount++] = baseIdx + i;
            indices[iCount++] = baseIdx + (i + 1) % CIRCLE_SEGS;
            indices[iCount++] = baseIdx + CIRCLE_SEGS;
        }
        vertices[vCount + CIRCLE_SEGS] = vertices[vCount];
        vertices[vCount + CIRCLE_SEGS].coord = short4( 0, 0, 0, 0 );
        vCount += CIRCLE_SEGS + 1;

    // box
        box.vStart = vStartCommon;
        box.iStart = iCount;
        box.iCount = COUNT(boxIndices);

        baseIdx = vCount - vStartCommon;

        for (int i = 0; i < COUNT(boxIndices); i++)
            indices[iCount++] = baseIdx + boxIndices[i];

        for (int i = 0; i < COUNT(boxCoords); i++) {
            Vertex &v = vertices[vCount++];
            v.coord    = boxCoords[i];
            v.normal   = short4(0, 0, 0, 32767);
            v.texCoord = short4(0, 0, 0, 0);
            v.color    = ubyte4(255, 255, 255, 255);
            v.light    = ubyte4(255, 255, 255, 255);
        }

    // plane
    #ifdef GENERATE_WATER_PLANE
        plane.vStart = vStartCommon;
        plane.iStart = iCount;
        plane.iCount = SQR(PLANE_DETAIL * 2) * 6;

        baseIdx = vCount - vStartCommon;
        for (int16 j = -PLANE_DETAIL; j <= PLANE_DETAIL; j++)
            for (int16 i = -PLANE_DETAIL; i <= PLANE_DETAIL; i++) {
                vertices[vCount++].coord = short4( i, j, 0, 0 );
                if (j < PLANE_DETAIL && i < PLANE_DETAIL) {
                    int idx = baseIdx + (j + PLANE_DETAIL) * (PLANE_DETAIL * 2 + 1) + i + PLANE_DETAIL;
                    indices[iCount + 0] = idx + PLANE_DETAIL * 2 + 1;
                    indices[iCount + 1] = idx + 1;
                    indices[iCount + 2] = idx;
                    indices[iCount + 3] = idx + PLANE_DETAIL * 2 + 2;
                    indices[iCount + 4] = idx + 1;
                    indices[iCount + 5] = idx + PLANE_DETAIL * 2 + 1;
                    iCount += 6;
                }
            }
        ASSERT(vCount - vStartCommon <= 0xFFFF);
    #else
        plane.iCount = 0;
    #endif

        LOG("MegaMesh (i:%d v:%d a:%d, size:%d)\n", iCount, vCount, aCount, int(iCount * sizeof(Index) + vCount * sizeof(GAPI::Vertex)));

    // compile buffer and ranges
        mesh = new Mesh(indices, iCount, vertices, vCount, aCount, false);
        delete[] indices;
        delete[] vertices;

        PROFILE_LABEL(BUFFER, mesh->ID[0], "Geometry indices");
        PROFILE_LABEL(BUFFER, mesh->ID[1], "Geometry vertices");

        // initialize Vertex Arrays
        MeshRange rangeRoom;
        rangeRoom.vStart = 0;
        mesh->initRange(rangeRoom);
        for (int i = 0; i < level->roomsCount; i++) {
            
            if (rooms[i].split) {
                ASSERT(rooms[i].geometry[0].count);
                rangeRoom.vStart = rooms[i].geometry[0].ranges[0].vStart;
                mesh->initRange(rangeRoom);
            }

            RoomRange &r = rooms[i];
            for (int j = 0; j < 3; j++)
                for (int k = 0; k < r.geometry[j].count; k++)
                    r.geometry[j].ranges[k].aIndex = rangeRoom.aIndex;

            r.sprites.aIndex = rangeRoom.aIndex;
            r.waterVolume.aIndex = rangeRoom.aIndex;
        }

        MeshRange rangeModel;
        rangeModel.vStart = vStartModel;
        mesh->initRange(rangeModel);
        for (int i = 0; i < level->modelsCount; i++)
            for (int j = 0; j < 3; j++) {
                Geometry &geom = models[i].geometry[j];
                for (int k = 0; k < geom.count; k++)
                    geom.ranges[k].aIndex = rangeModel.aIndex;
            }

        MeshRange rangeCommon;
        rangeCommon.vStart = vStartCommon;
        mesh->initRange(rangeCommon);
        shadowBlob.aIndex = rangeCommon.aIndex;
        quad.aIndex       = rangeCommon.aIndex;
        circle.aIndex     = rangeCommon.aIndex;
        plane.aIndex      = rangeCommon.aIndex;
        box.aIndex        = rangeCommon.aIndex;
    }

    ~MeshBuilder() {
        for (int i = 0; i < level->roomsCount; i++)
            for (int j = 0; j < COUNT(rooms[i].dynamic); j++)
                delete[] rooms[i].dynamic[j].faces;

        delete[] rooms;
        delete[] models;
        delete mesh;
        delete dynMesh;
    }

    void flipMap() {
        for (int i = 0; i < level->roomsCount; i++)
            if (level->rooms[i].alternateRoom > -1)
                swap(rooms[i], rooms[level->rooms[i].alternateRoom]);
    }

    inline short4 rotate(const short4 &v, int dir) {
        if (dir == 0) return v;
        short4 res = v;

        switch (dir) {
            case 1  : res.x =  v.z; res.z = -v.x; break;
            case 2  : res.x = -v.x; res.z = -v.z; break;
            case 3  : res.x = -v.z; res.z =  v.x; break;
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

    bool isWaterSurface(int delta, int roomIndex, bool fromWater) {
        if (roomIndex != TR::NO_ROOM && delta == 0) {
            TR::Room &r = level->rooms[roomIndex];
            if (r.flags.water ^ fromWater)
                return true;
            if (r.alternateRoom > -1 && level->rooms[r.alternateRoom].flags.water ^ fromWater)
                return true;
        }
        return false;
    }

    int calcWaterLevel(int16 roomIndex, bool flip) {
        TR::Room &room = level->rooms[roomIndex];

        int32 &value = room.waterLevel[flip];

        if (value != TR::NO_WATER)
            return value;

        if (room.flags.water) {
            value = room.info.yBottom;

            for (int z = 0; z < room.zSectors; z++)
                for (int x = 0; x < room.xSectors; x++) {
                    TR::Room::Sector &s = room.sectors[x * room.zSectors + z];
                    if (level->getNextRoom(&s) != TR::NO_ROOM)
                        continue;
                    if (s.ceiling != TR::NO_FLOOR)
                        value = min( value, s.ceiling * 256 );
                    if (s.roomAbove != TR::NO_ROOM)
                        value = min( value, calcWaterLevel(s.roomAbove, flip) );
                }

            return value;
        }
        
        return room.info.yBottom;
    }

    void roomRemoveWaterSurfaces(TR::Room &room, int &iCount, int &vCount) {
        for (int i = 0; i < room.data.fCount; i++)
            room.data.faces[i].water = false;

        if (Core::settings.detail.water == Core::Settings::LOW)
            return;

        for (int i = 0; i < room.data.fCount; i++) {
            TR::Face &f = room.data.faces[i];

            short3 &a = room.data.vertices[f.vertices[0]].pos;
            short3 &b = room.data.vertices[f.vertices[1]].pos;
            short3 &c = room.data.vertices[f.vertices[2]].pos;

            if (a.y != b.y || a.y != c.y) // skip non-horizontal or non-portal plane primitive
                continue;
            
            int sx = (int(a.x) + int(b.x) + int(c.x)) / 3 / 1024;
            int sz = (int(a.z) + int(b.z) + int(c.z)) / 3 / 1024;

            TR::Room::Sector &s = room.sectors[sx * room.zSectors + sz];

            int yt = abs(a.y - s.ceiling * 256);
            int yb = abs(s.floor * 256 - a.y);

            if (yt > 0 && yb > 0) continue;

            if (isWaterSurface(yt, s.roomAbove, room.flags.water) ||
                isWaterSurface(yb, s.roomBelow, room.flags.water)) {
                f.water = true;

                room.waterLevelSurface = a.y;
                if (f.triangle) {
                    iCount -= 3;
                    vCount -= 3;
                } else {
                    iCount -= 6;
                    vCount -= 4;
                }

            // preserve indices & vertices for water volume
                if (room.flags.water && Core::settings.detail.water > Core::Settings::MEDIUM) {
                // water volume caps
                    iCount += (f.triangle ? 3 : 6) * 2;
                    vCount += (f.triangle ? 3 : 4) * 2;
                // water volume bounds (reserved)
                    iCount += (f.triangle ? 3 : 4) * 6;
                    vCount += (f.triangle ? 3 : 4) * 4;
                }
            }
        }
    }

    Index addUniqueVertex(Array<short3> &vertices, short3 &v) {
        for (int i = 0; i < vertices.length; i++) {
            short3 &o = vertices[i];
            if (o.x == v.x && o.y == v.y && o.z == v.z)
                return i;
        }
        return vertices.push(v);
    }

    void addUniqueEdge(Array<Edge> &edges, Index a, Index b) {
        for (int i = 0; i < edges.length; i++) {
            Edge &e = edges[i];
            if ((e.a == a && e.b == b) || (e.a == b && e.b == a)) {
                edges.remove(i);
                return;
            }
        }
        edges.push(Edge(a, b));
    }

    void buildWaterVolume(int roomIndex, Index *indices, Vertex *vertices, int &iCount, int &vCount, int vStart) {
        TR::Room &room = level->rooms[roomIndex];
        if (!room.flags.water) return;
        MeshRange &range = rooms[roomIndex].waterVolume;

        Array<Edge>   wEdges(128);
        Array<Index>  wIndices(128);
        Array<short3> wVertices(128);

        for (int i = 0; i < room.data.fCount; i++) {
            TR::Face &f = room.data.faces[i];
            if (!f.water) continue;

            Index idx[4];

            idx[0] = addUniqueVertex(wVertices, room.data.vertices[f.vertices[0]].pos);
            idx[1] = addUniqueVertex(wVertices, room.data.vertices[f.vertices[1]].pos);
            idx[2] = addUniqueVertex(wVertices, room.data.vertices[f.vertices[2]].pos);

            if (!f.triangle) {
                idx[3] = addUniqueVertex(wVertices, room.data.vertices[f.vertices[3]].pos);

                wIndices.push(idx[0]);
                wIndices.push(idx[1]);
                wIndices.push(idx[3]);

                wIndices.push(idx[3]);
                wIndices.push(idx[1]);
                wIndices.push(idx[2]);

                addUniqueEdge(wEdges, idx[0], idx[1]);
                addUniqueEdge(wEdges, idx[1], idx[2]);
                addUniqueEdge(wEdges, idx[2], idx[3]);
                addUniqueEdge(wEdges, idx[3], idx[0]);
            } else {
                wIndices.push(idx[0]);
                wIndices.push(idx[1]);
                wIndices.push(idx[2]);

                addUniqueEdge(wEdges, idx[0], idx[1]);
                addUniqueEdge(wEdges, idx[1], idx[2]);
                addUniqueEdge(wEdges, idx[2], idx[0]);
            }
        }

        if (!wEdges.length) return;

        Array<short3> wOffsets(wVertices.length);

        for (int i = 0; i < wVertices.length; i++)
            wOffsets.push(short3(0, WATER_VOLUME_OFFSET, 0));

        for (int i = 0; i < wEdges.length; i++) {
            Edge &e = wEdges[i];
            short3 &a = wVertices[e.a];
            short3 &b = wVertices[e.b];
            int16 dx = a.z - b.z;
            int16 dz = b.x - a.x;

            short3 &ao = wOffsets[e.a];
            ao.x = clamp(ao.x + dx, -WATER_VOLUME_OFFSET, WATER_VOLUME_OFFSET);
            ao.z = clamp(ao.z + dz, -WATER_VOLUME_OFFSET, WATER_VOLUME_OFFSET);

            short3 &bo = wOffsets[e.b];
            bo.x = clamp(bo.x + dx, -WATER_VOLUME_OFFSET, WATER_VOLUME_OFFSET);
            bo.z = clamp(bo.z + dz, -WATER_VOLUME_OFFSET, WATER_VOLUME_OFFSET);
        }

        range.vStart = vStart;
        range.iCount = wIndices.length * 2 + wEdges.length * 6;
        range.iStart = iCount;

        for (int i = 0; i < wIndices.length; i += 3) {
            indices[iCount++] = vCount + wIndices[i + 2];
            indices[iCount++] = vCount + wIndices[i + 1];
            indices[iCount++] = vCount + wIndices[i + 0];
        }

        for (int i = 0; i < wIndices.length; i++)
            indices[iCount++] = vCount + wIndices[i] + wVertices.length;

        for (int i = 0; i < wEdges.length; i++) {
            Index a = wEdges[i].a;
            Index b = wEdges[i].b;

            indices[iCount++] = vCount + a;
            indices[iCount++] = vCount + b;
            indices[iCount++] = vCount + a + wVertices.length;

            indices[iCount++] = vCount + b;
            indices[iCount++] = vCount + b + wVertices.length;
            indices[iCount++] = vCount + a + wVertices.length;
        }

        for (int i = 0; i < wVertices.length; i++) {
            short3 &v = wVertices[i];
            short3 &o = wOffsets[i];

            v.x += o.x;
            v.y += o.y;
            v.z += o.z;

            vertices[vCount++].coord = short4(v.x, v.y, v.z, 0);
        }

        for (int i = 0; i < wVertices.length; i++) {
            short3 &v = wVertices[i];

            int16 base = v.y;

            v.y += WATER_VOLUME_HEIGHT - WATER_VOLUME_OFFSET - WATER_VOLUME_OFFSET;

            const vec3 sectorOffsets[] = {
                vec3(-8, 0, -8),
                vec3( 8, 0, -8),
                vec3( 8, 0,  8),
                vec3(-8, 0,  8),
            };

            int16 floor = 32000;
            for (int j = 0; j < 4; j++) {
                vec3 pos = room.getOffset() + vec3(v.x, v.y, v.z) + sectorOffsets[j];
                int16 rIndex = roomIndex;
                TR::Room::Sector *sector = level->getSector(rIndex, pos);
                if (sector->floor == TR::NO_FLOOR || !level->rooms[rIndex].flags.water) continue;
                floor = min(floor, int16(level->getFloor(sector, pos)));
            }

            floor -= WATER_VOLUME_OFFSET * 3;

            if (floor > base) {
                v.y = min(v.y, floor);
            }

            vertices[vCount++].coord = short4(v.x, v.y, v.z, 0);
        }
    }

    inline int getBlendMask(int texAttribute) {
        ASSERT(texAttribute < 3);
        return 1 << texAttribute;
    }

    void buildRoom(Geometry &geom, Dynamic &dyn, int blendMask, const TR::Room &room, TR::Level *level, Index *indices, Vertex *vertices, int &iCount, int &vCount, int vStart) {
        const TR::Room::Data &d = room.data;

        dyn.count = 0;
        dyn.faces = NULL;

        for (int j = 0; j < d.fCount; j++) {
            TR::Face &f = d.faces[j];
            ASSERT(!f.colored);
            ASSERT(f.flags.texture < level->objectTexturesCount);
            TR::TextureInfo &t = level->objectTextures[f.flags.texture];

            if (f.water) continue;

            CHECK_ROOM_NORMAL(f);

            if (!(blendMask & getBlendMask(t.attribute)))
                continue;

            if (t.animated) {
                ASSERT(dyn.count < 0xFFFF);
                dyn.count++;
                continue;
            }

            if (!geom.validForTile(t.tile, t.clut))
                geom.getNextRange(vStart, iCount, t.tile, t.clut);

            ADD_ROOM_FACE(indices, iCount, vCount, vStart, vertices, f, t);
        }

    // if room has non-static polygons, fill the list of dynamic faces
        if (dyn.count) {
            dyn.faces = new uint16[dyn.count];
            dyn.count = 0;
            for (int j = 0; j < d.fCount; j++) {
                TR::Face        &f = d.faces[j];
                TR::TextureInfo &t = level->objectTextures[f.flags.texture];

                if (f.water) continue;

                if (!(blendMask & getBlendMask(t.attribute)))
                    continue;

                if (t.animated)
                    dyn.faces[dyn.count++] = j;
            }
        }
    }

    bool buildMesh(Geometry &geom, int blendMask, const TR::Mesh &mesh, TR::Level *level, Index *indices, Vertex *vertices, int &iCount, int &vCount, int vStart, int16 joint, int x, int y, int z, int dir, const Color32 &light) {
        bool isOpaque = true;

        for (int j = 0; j < mesh.fCount; j++) {
            TR::Face &f = mesh.faces[j];
            ASSERT(f.colored || f.flags.texture < level->objectTexturesCount);
            TR::TextureInfo &t = f.colored ? whiteTile : level->objectTextures[f.flags.texture];

            if (t.attribute != 0)
                isOpaque = false;

            if (!(blendMask & getBlendMask(t.attribute)))
                continue;

            if (!geom.validForTile(t.tile, t.clut))
                geom.getNextRange(vStart, iCount, t.tile, t.clut);

            Color32 c = f.colored ? level->getColor(f.flags.value) : COLOR_WHITE;

            addFace(indices, iCount, vCount, vStart, vertices, f, &t, 
                    mesh.vertices[f.vertices[0]].coord,
                    mesh.vertices[f.vertices[1]].coord,
                    mesh.vertices[f.vertices[2]].coord,
                    mesh.vertices[f.vertices[3]].coord);

            for (int k = 0; k < (f.triangle ? 3 : 4); k++) {
                TR::Mesh::Vertex &v = mesh.vertices[f.vertices[k]];

                vertices[vCount].coord  = transform(v.coord, joint, x, y, z, dir);
                vec3 n = vec3(v.normal.x, v.normal.y, v.normal.z).normal() * 32767.0f;
                v.normal = short4(short(n.x), short(n.y), short(n.z), 0);
                vertices[vCount].normal = rotate(v.normal, dir);
                vertices[vCount].color  = ubyte4( c.r, c.g, c.b, 255 );
                vertices[vCount].light  = ubyte4( light.r, light.g, light.b, 255 );

                vCount++;
            }
        }

        return isOpaque;
    }

    void addTexCoord(Vertex *vertices, int vCount, TR::TextureInfo *tex, bool triangle, uint8 flip) {
        int count = triangle ? 3 : 4;
        for (int i = 0; i < count; i++) {
            Vertex &v = vertices[vCount + i];
            v.texCoord = short4( tex->texCoordAtlas[i].x, tex->texCoordAtlas[i].y, 32767, 32767 );
        }

        if (((level->version & TR::VER_PSX)) && !triangle) // TODO: swap vertices instead of rectangle indices and vertices.texCoords (WRONG lighting in TR2!)
            swap(vertices[vCount + 2].texCoord, vertices[vCount + 3].texCoord);

        if ((level->version & TR::VER_SAT)) {
            if (triangle) {
            /*  transform Saturn's triangle texCoords by flip code
                                    |\
                flip 2, 6,  8, 12 - |_\
                                     _
                flip 0, 4, 10, 14 - | /
                                    |/
            */
                if (flip == 2 || flip == 6 || flip == 8 || flip == 12)
                    vertices[vCount + 1].texCoord = vertices[vCount + 2].texCoord;

                vertices[vCount + 2].texCoord.x = vertices[vCount + 0].texCoord.x;

                if (flip == 10 || flip == 14) // flip diagonal
                    swap(vertices[vCount + 1].texCoord, vertices[vCount + 2].texCoord);

                if (flip == 2 || flip == 6) { // rotate
                    swap(vertices[vCount + 0].texCoord, vertices[vCount + 2].texCoord);
                    swap(vertices[vCount + 2].texCoord, vertices[vCount + 1].texCoord);
                }

                if (flip == 8 || flip == 12) // flip vertical
                    swap(vertices[vCount + 0].texCoord, vertices[vCount + 2].texCoord);

            } else {

                if (flip) { // flip horizontal
                    swap(vertices[vCount + 0].texCoord, vertices[vCount + 1].texCoord);
                    swap(vertices[vCount + 3].texCoord, vertices[vCount + 2].texCoord);
                }

            }
        }
    }

    void addTriangle(Index *indices, int &iCount, int vCount, int vStart, Vertex *vertices, TR::TextureInfo *tex, bool doubleSided, uint8 flip) {
        int vIndex = vCount - vStart;

        indices[iCount + 0] = vIndex + 0;
        indices[iCount + 1] = vIndex + 1;
        indices[iCount + 2] = vIndex + 2;

        iCount += 3;

        if (doubleSided) {
            indices[iCount + 0] = vIndex + 2;
            indices[iCount + 1] = vIndex + 1;
            indices[iCount + 2] = vIndex + 0;
            iCount += 3;
        }

        if (tex) addTexCoord(vertices, vCount, tex, true, flip);
    }

    void addQuad(Index *indices, int &iCount, int vCount, int vStart, Vertex *vertices, TR::TextureInfo *tex, bool doubleSided, uint8 flip) {
        int vIndex = vCount - vStart;

        indices[iCount + 0] = vIndex + 0;
        indices[iCount + 1] = vIndex + 1;
        indices[iCount + 2] = vIndex + 2;

        indices[iCount + 3] = vIndex + 0;
        indices[iCount + 4] = vIndex + 2;
        indices[iCount + 5] = vIndex + 3;

        iCount += 6;

        if (doubleSided) {
            indices[iCount + 0] = vIndex + 2;
            indices[iCount + 1] = vIndex + 1;
            indices[iCount + 2] = vIndex + 0;

            indices[iCount + 3] = vIndex + 3;
            indices[iCount + 4] = vIndex + 2;
            indices[iCount + 5] = vIndex + 0;

            iCount += 6;
        }

        if (tex) addTexCoord(vertices, vCount, tex, false, flip);
    }

    void addQuad(Index *indices, int &iCount, int &vCount, int vStart, Vertex *vertices, TR::TextureInfo *tex, bool doubleSided, uint8 flip,
                 const short3 &c0, const short3 &c1, const short3 &c2, const short3 &c3) {
        addQuad(indices, iCount, vCount, vStart, vertices, tex, doubleSided, flip);

        // TODO: pre-calculate trapezoid ratio for room geometry
        vec3 a = c0 - c1;
        vec3 b = c3 - c2;
        vec3 c = c0 - c3;
        vec3 d = c1 - c2;

        float aL = a.length();
        float bL = b.length();
        float cL = c.length();
        float dL = d.length();

        float ab = a.dot(b) / (aL * bL);
        float cd = c.dot(d) / (cL * dL);

        int16 tx = abs(vertices[vCount + 0].texCoord.x - vertices[vCount + 3].texCoord.x);
        int16 ty = abs(vertices[vCount + 0].texCoord.y - vertices[vCount + 3].texCoord.y);

        if (ab > cd) {
            int k = (tx > ty) ? 3 : 2;

            if (aL > bL)
                vertices[vCount + 2].texCoord[k] = vertices[vCount + 3].texCoord[k] = int16(bL / aL * 32767.0f);
            else
                vertices[vCount + 0].texCoord[k] = vertices[vCount + 1].texCoord[k] = int16(aL / bL * 32767.0f);
        } else {
            int k = (tx > ty) ? 2 : 3;

            if (cL > dL) {
                vertices[vCount + 1].texCoord[k] = vertices[vCount + 2].texCoord[k] = int16(dL / cL * 32767.0f);
            } else
                vertices[vCount + 0].texCoord[k] = vertices[vCount + 3].texCoord[k] = int16(cL / dL * 32767.0f);
        }
    }


    void addFace(Index *indices, int &iCount, int &vCount, int vStart, Vertex *vertices, const TR::Face &f, TR::TextureInfo *tex, const short3 &a, const short3 &b, const short3 &c, const short3 &d) {
        if (f.triangle)
            addTriangle(indices, iCount, vCount, vStart, vertices, tex, f.flags.doubleSided, f.flip);
        else
            addQuad(indices, iCount, vCount, vStart, vertices, tex, f.flags.doubleSided, f.flip, a, b, c, d);
    }


    short4 coordTransform(const vec3 &center, const vec3 &offset) {
        mat4 m = Core::mViewInv;
        m.setPos(center);

        vec3 coord = m * offset;
        return short4(int16(coord.x), int16(coord.y), int16(coord.z), 0);
    }

    void addSprite(Index *indices, Vertex *vertices, int &iCount, int &vCount, int vStart, int16 x, int16 y, int16 z, bool invert, const TR::TextureInfo &sprite, const Color32 &tColor, const Color32 &bColor, bool expand = false) {
        addQuad(indices, iCount, vCount, vStart, NULL, NULL, false, false);

        Vertex *quad = &vertices[vCount];

        int16 x0, y0, x1, y1;

        if (expand) {
            if (invert) {
                x0 = x - int16(sprite.l);
                y0 = y - int16(sprite.t);
                x1 = x - int16(sprite.r);
                y1 = y - int16(sprite.b);
            } else {
                x0 = x + int16(sprite.l);
                y0 = y + int16(sprite.t);
                x1 = x + int16(sprite.r);
                y1 = y + int16(sprite.b);
            }
        } else {
            x0 = x1 = x;
            y0 = y1 = y;
        }

    #ifndef MERGE_SPRITES
        if (!expand) {
            vec3 pos = vec3(float(x), float(y), float(z));
            quad[0].coord = coordTransform(pos, vec3( float(sprite.l), float(-sprite.t), 0 ));
            quad[1].coord = coordTransform(pos, vec3( float(sprite.r), float(-sprite.t), 0 ));
            quad[2].coord = coordTransform(pos, vec3( float(sprite.r), float(-sprite.b), 0 ));
            quad[3].coord = coordTransform(pos, vec3( float(sprite.l), float(-sprite.b), 0 ));
        } else
    #endif
        {
            quad[0].coord = short4( x0, y0, z, 0 );
            quad[1].coord = short4( x1, y0, z, 0 );
            quad[2].coord = short4( x1, y1, z, 0 );
            quad[3].coord = short4( x0, y1, z, 0 );
        }

        quad[0].normal = quad[1].normal = quad[2].normal = quad[3].normal = short4( 0, 0, 0, 0 );
        quad[0].color  = quad[1].color  = 
        quad[2].color  = quad[3].color  = ubyte4( 255, 255, 255, 255 );
        quad[0].light  = quad[1].light  = ubyte4( tColor.r, tColor.g, tColor.b, tColor.a );
        quad[2].light  = quad[3].light  = ubyte4( bColor.r, bColor.g, bColor.b, bColor.a );

        quad[0].texCoord = short4( sprite.texCoordAtlas[0].x, sprite.texCoordAtlas[0].y, sprite.l, -sprite.t );
        quad[1].texCoord = short4( sprite.texCoordAtlas[1].x, sprite.texCoordAtlas[0].y, sprite.r, -sprite.t );
        quad[2].texCoord = short4( sprite.texCoordAtlas[1].x, sprite.texCoordAtlas[1].y, sprite.r, -sprite.b );
        quad[3].texCoord = short4( sprite.texCoordAtlas[0].x, sprite.texCoordAtlas[1].y, sprite.l, -sprite.b );

        vCount += 4;
    }

    void addDynBar(const TR::TextureInfo &tile, const vec2 &pos, const vec2 &size, uint32 color, uint32 color2 = 0) {
        dynCheck(1 * 6);

        Index *indices   = dynIndices;
        Vertex *vertices = dynVertices;
        int &iCount      = dynICount;
        int &vCount      = dynVCount;

        addQuad(indices, iCount, vCount, 0, vertices, NULL, false, false);

        int16 minX = int16(pos.x);
        int16 minY = int16(pos.y);
        int16 maxX = int16(size.x) + minX;
        int16 maxY = int16(size.y) + minY;

        vertices[vCount + 0].coord = short4( minX, minY, 0, 0 );
        vertices[vCount + 1].coord = short4( maxX, minY, 0, 0 );
        vertices[vCount + 2].coord = short4( maxX, maxY, 0, 0 );
        vertices[vCount + 3].coord = short4( minX, maxY, 0, 0 );

        for (int i = 0; i < 4; i++) {
            Vertex &v = vertices[vCount + i];
            v.normal  = short4( 0, 0, 0, 0 );
            if (color2 != 0 && (i == 0 || i == 3))
                v.light.value = color2;
            else
                v.light.value = color;

            short2 uv = tile.texCoordAtlas[i];

            v.texCoord = short4( uv.x, uv.y, 32767, 32767 );
        }

        vCount += 4;
    }

    void addDynFrame(const vec2 &pos, const vec2 &size, uint32 color1, uint32 color2) {
        dynCheck(4 * 6);

        Index *indices   = dynIndices;
        Vertex *vertices = dynVertices;
        int &iCount      = dynICount;
        int &vCount      = dynVCount;

        short4 uv = short4( whiteTile.texCoordAtlas[0].x, whiteTile.texCoordAtlas[0].y, 32767, 32767 );

        int16 minX = int16(pos.x);
        int16 minY = int16(pos.y);
        int16 maxX = int16(size.x) + minX;
        int16 maxY = int16(size.y) + minY;

        vertices[vCount + 0].coord = short4( minX, minY, 0, 0 );
        vertices[vCount + 1].coord = short4( maxX, minY, 0, 0 );
        vertices[vCount + 2].coord = short4( maxX, int16(minY + 1), 0, 0 );
        vertices[vCount + 3].coord = short4( minX, int16(minY + 1), 0, 0 );

        vertices[vCount + 4].coord = short4( minX, minY, 0, 0 );
        vertices[vCount + 5].coord = short4( int16(minX + 1), minY, 0, 0 );
        vertices[vCount + 6].coord = short4( int16(minX + 1), maxY, 0, 0 );
        vertices[vCount + 7].coord = short4( minX, maxY, 0, 0 );

        for (int i = 0; i < 8; i++) {
            Vertex &v = vertices[vCount + i];
            v.normal      = short4( 0, 0, 0, 0 );
            v.light.value = color1;
            v.texCoord    = uv;
        }

        addQuad(indices, iCount, vCount, 0, vertices, NULL, false, false); vCount += 4;
        addQuad(indices, iCount, vCount, 0, vertices, NULL, false, false); vCount += 4;

        vertices[vCount + 0].coord = short4( minX, int16(maxY - 1), 0, 0 );
        vertices[vCount + 1].coord = short4( maxX, int16(maxY - 1), 0, 0 );
        vertices[vCount + 2].coord = short4( maxX, maxY, 0, 0 );
        vertices[vCount + 3].coord = short4( minX, maxY, 0, 0 );

        vertices[vCount + 4].coord = short4( int16(maxX - 1), minY, 0, 0 );
        vertices[vCount + 5].coord = short4( maxX, minY, 0, 0 );
        vertices[vCount + 6].coord = short4( maxX, maxY, 0, 0 );
        vertices[vCount + 7].coord = short4( int16(maxX - 1), maxY, 0, 0 );

        for (int i = 0; i < 8; i++) {
            Vertex &v = vertices[vCount + i];
            v.normal      = short4( 0, 0, 0, 0 );
            v.light.value = color2;
            v.texCoord    = uv;
        }

        addQuad(indices, iCount, vCount, 0, vertices, NULL, false, false); vCount += 4;
        addQuad(indices, iCount, vCount, 0, vertices, NULL, false, false); vCount += 4;
    }
    
    void renderBuffer(Index *indices, int iCount, Vertex *vertices, int vCount) {
        if (!iCount) return;
        ASSERT(vCount > 0);

        dynRange.iStart = 0;
        dynRange.iCount = iCount;

        dynMesh->update(indices, iCount, vertices, vCount);
        dynMesh->render(dynRange);
    }

    void renderRoomGeometry(int roomIndex) {
        Geometry &geom = rooms[roomIndex].geometry[transparent];
        for (int i = 0; i < geom.count; i++) {
            MeshRange &range = geom.ranges[i];

        #ifdef SPLIT_BY_TILE
            int clutOffset = level->rooms[roomIndex].flags.water ? 512 : 0;
            atlas->bindTile(range.tile, range.clut + clutOffset);
        #endif

            mesh->render(range);
        }

        Dynamic &dyn = rooms[roomIndex].dynamic[transparent];
        if (dyn.count) {
        #ifdef SPLIT_BY_TILE
            uint16 tile = 0xFFFF, clut = 0xFFFF;
        #endif
            int iCount = 0, vCount = 0, vStart = 0;

            const TR::Room::Data &d = level->rooms[roomIndex].data;
            for (int i = 0; i < dyn.count; i++) {
                TR::Face        &f = d.faces[dyn.faces[i]];
                TR::TextureInfo &t = level->objectTextures[f.flags.texture];

            #ifdef SPLIT_BY_TILE
                if (iCount) {
                    if (tile != t.tile
                    #ifdef SPLIT_BY_CLUT
                        || clut != t.clut
                    #endif
                        ) {
                        atlas->bindTile(tile, clut);
                        renderBuffer(dynIndices, iCount, dynVertices, vCount);
                        tile = t.tile;
                        clut = t.clut;
                        iCount = 0;
                        vCount = 0;
                    }
                } else {
                    tile = t.tile;
                    clut = t.clut;
                }
            #endif

                ADD_ROOM_FACE(dynIndices, iCount, vCount, vStart, dynVertices, f, t);
            }

            if (iCount) {
            #ifdef SPLIT_BY_TILE
                atlas->bindTile(tile, clut);
            #endif
                renderBuffer(dynIndices, iCount, dynVertices, vCount);
            }
        }
    }

    void dynBegin() {
        dynICount = 0;
        dynVCount = 0;

        #ifdef SPLIT_BY_TILE
            curTile = 0xFFFF;
            curClut = 0xFFFF;
        #endif
    }

    void dynEnd() {
        if (dynICount) {
            renderBuffer(dynIndices, dynICount, dynVertices, dynVCount);
        }
    }

    void dynCheck(int freeIndicesCount) {
        if (dynICount + freeIndicesCount > COUNT(dynIndices)) {
            dynEnd();
            dynBegin();
        }
    }

    void addDynSprite(int spriteIndex, const short3 &center, bool invert, const Color32 &tColor, const Color32 &bColor, bool expand = false) {
        dynCheck(1 * 6);

        TR::TextureInfo &sprite = level->spriteTextures[spriteIndex];

        #ifdef SPLIT_BY_TILE
            if (sprite.tile != curTile
                #ifdef SPLIT_BY_CLUT
                    || sprite.clut != curClut
                #endif
            ) {
                dynEnd();
                dynBegin();
                curTile = sprite.tile;
                curClut = sprite.clut;
                atlas->bindTile(curTile, curClut);
            }
        #endif

        addSprite(dynIndices, dynVertices, dynICount, dynVCount, 0, center.x, center.y, center.z, invert, sprite, tColor, bColor, expand);
    }

    void renderRoomSprites(int roomIndex) {
    #ifndef MERGE_SPRITES
        Core::mModel.identity();
        Core::mModel.setPos(Core::active.basis[0].pos);

        dynBegin();

        TR::Room::Data &d = level->rooms[roomIndex].data;
        for (int j = 0; j < d.sCount; j++) {
            TR::Room::Data::Sprite &f = d.sprites[j];
            addDynSprite(f.texture, d.vertices[f.vertexIndex].pos, COLOR_WHITE, COLOR_WHITE);
        }

        dynEnd();
    #else
        mesh->render(rooms[roomIndex].sprites);
    #endif
    }

    void renderMesh(const MeshRange &range) {
        mesh->render(range);
    }

    void renderModel(int modelIndex, bool underwater = false) {
        ASSERT((Core::pass != Core::passCompose && Core::pass != Core::passShadow && Core::pass != Core::passAmbient) || 
               level->models[modelIndex].mCount == Core::active.basisCount);

        int part = 0;

        Geometry &geom = models[modelIndex].geometry[transparent];
    #ifdef MERGE_MODELS
        int i = 0;
    #else
        for (int i = 0; i < level->models[modelIndex].mCount; i++)
    #endif
        {
            #ifndef MERGE_MODELS
                if (models[modelIndex].parts[transparent][i] == 0)
                    continue;

                Basis &basis = Core::active.basis[i];
                if (basis.w == 0.0f) {
                    part += models[modelIndex].parts[transparent][i];
                    continue;
                }
                #ifdef FFP
                    Core::mModel.identity();
                    Core::mModel.setRot(basis.rot);
                    Core::mModel.setPos(basis.pos);
                #endif
            #endif

            for (int j = 0; j < models[modelIndex].parts[transparent][i]; j++) {
                MeshRange &range = geom.ranges[part++];

                ASSERT(range.iCount);

                #ifdef SPLIT_BY_TILE
                    int clutOffset = underwater ? 512 : 0;
                    atlas->bindTile(range.tile, range.clut + clutOffset);
                #endif

                mesh->render(range);
            }
        }
    }

    void renderModelFull(int modelIndex, bool underwater = false) {
        Core::setBlendMode(bmPremult);
        transparent = 0;
        renderModel(modelIndex, underwater);
        transparent = 1;
        renderModel(modelIndex, underwater);
        Core::setBlendMode(bmAdd);
        Core::setDepthWrite(false);
        transparent = 2;
        renderModel(modelIndex, underwater);
        Core::setDepthWrite(true);
        Core::setBlendMode(bmNone);
        transparent = 0;
    }

    void renderShadowBlob() {
        mesh->render(shadowBlob);
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

    void renderBox() {
        mesh->render(box);
    }

    void renderWaterVolume(int roomIndex) {
        MeshRange &range = rooms[roomIndex].waterVolume;
        if (range.iCount)
            mesh->render(range);
    }
};

#endif
