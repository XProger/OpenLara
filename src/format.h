#ifndef H_FORMAT
#define H_FORMAT

#include "utils.h"

#define TR1_DEMO

namespace TR {
    #define ROOM_FLAG_VISIBLE   0x8000

    #define DATA_PORTAL     0x01
    #define DATA_FLOOR      0x02
    #define DATA_CEILING    0x03


    #define ENTITY_FLAG_CLEAR   0x0080
    #define ENTITY_FLAG_VISIBLE 0x0100
    #define ENTITY_FLAG_MASK    0x3E00

    #define ENTITY_LARA                     0

    #define ENTITY_ENEMY_TWIN               6
    #define ENTITY_ENEMY_WOLF               7
    #define ENTITY_ENEMY_BEAR               8
    #define ENTITY_ENEMY_BAT                9
    #define ENTITY_ENEMY_CROCODILE_LAND     10
    #define ENTITY_ENEMY_CROCODILE_WATER    11
    #define ENTITY_ENEMY_LION_MALE          12
    #define ENTITY_ENEMY_LION_FEMALE        13
    #define ENTITY_ENEMY_PUMA               14
    #define ENTITY_ENEMY_GORILLA            15
    #define ENTITY_ENEMY_RAT_LAND           16
    #define ENTITY_ENEMY_RAT_WATER          17
    #define ENTITY_ENEMY_REX                18
    #define ENTITY_ENEMY_RAPTOR             19
    #define ENTITY_ENEMY_MUTANT             20

    #define ENTITY_ENEMY_CENTAUR    23
    #define ENTITY_ENEMY_MUMMY      24
    #define ENTITY_ENEMY_LARSON     27

    #define ENTITY_CRYSTAL          83

    #define ENTITY_MEDIKIT_SMALL    93
    #define ENTITY_MEDIKIT_BIG      94

    #define ENTITY_VIEW_TARGET      169

    #define ENTITY_TRAP_FLOOR       35
    #define ENTITY_TRAP_SPIKES      37
    #define ENTITY_TRAP_STONE       38
    #define ENTITY_TRAP_DART        40

    #define ENTITY_SWITCH           55

    #define ENTITY_GUN_SHOTGUN      85

    #define ENTITY_AMMO_UZI         91
    #define ENTITY_AMMO_SHOTGUN     89
    #define ENTITY_AMMO_MAGNUM      90


    enum LaraState {
        STATE_WALK,
        STATE_RUN,
        STATE_STOP,
        STATE_FORWARD_JUMP,
        STATE_FAST_TURN,
        STATE_FAST_BACK,
        STATE_TURN_RIGHT,
        STATE_TURN_LEFT,
        STATE_DEATH,
        STATE_FAST_FALL,
        STATE_HANG,
        STATE_REACH,
        STATE_SPLAT,
        STATE_TREAD,
        STATE_FAST_TURN_14,
        STATE_COMPRESS,
        STATE_BACK,
        STATE_SWIM,
        STATE_GLIDE,
        STATE_NULL_19,
        STATE_FAST_TURN_20,
        STATE_STEP_RIGHT,
        STATE_STEP_LEFT,
        STATE_ROLL,
        STATE_SLIDE,
        STATE_BACK_JUMP,
        STATE_RIGHT_JUMP,
        STATE_LEFT_JUMP,
        STATE_UP_JUMP,
        STATE_FALL_BACK,
        STATE_HANG_LEFT,
        STATE_HANG_RIGHT,
        STATE_SLIDE_BACK,
        STATE_SURF_TREAD,
        STATE_SURF_SWIM,
        STATE_DIVE,
        STATE_PUSH_BLOCK,
        STATE_PULL_BLOCK,
        STATE_PUSH_PULL_READY,
        STATE_PICK_UP,
        STATE_SWITCH_ON,
        STATE_SWITCH_OFF,
        STATE_USE_KEY,
        STATE_USE_PUZZLE,
        STATE_UNDERWATER_DEATH,
        STATE_ROLL_45,
        STATE_SPECIAL,
        STATE_SURF_BACK,
        STATE_SURF_LEFT,
        STATE_SURF_RIGHT,
        STATE_NULL_50,
        STATE_NULL_51,
        STATE_SWAN_DIVE,
        STATE_FAST_DIVE,
        STATE_HANDSTAND,
        STATE_WATER_OUT,
        STATE_CLIMB_START_AND_STANDING,
        STATE_CLIMB_UP,
        STATE_CLIMB_LEFT,
        STATE_CLIMB_END,
        STATE_CLIMB_RIGHT,
        STATE_CLIMB_DOWN,
        STATE_NULL_62,
        STATE_NULL_63,
        STATE_NULL_64,
        STATE_WADE,
        STATE_WATER_ROLL,
        STATE_PICK_UP_FLARE,
        STATE_NULL_68,
        STATE_NULL_69,
        STATE_DEATH_SLIDE,
        STATE_DUCK,
        STATE_DUCK_72,
        STATE_DASH,
        STATE_DASH_DIVE,
        STATE_MAX };

    #pragma pack(push, 1)

    struct fixed {
        uint16  L;
        int16   H;
        float toFloat() {
            return H + L / 65535.0f;
        }
    };

    struct RGB {
        uint8 r, g, b;
    };

    struct RGBA {
        uint8 r, g, b, a;
    };

    struct Vertex {
        int16 x, y, z;

        operator vec3() const { return vec3((float)x, (float)y, (float)z); };
    };

    struct Rectangle {
        uint16 vertices[4];
        uint16 texture;     // 15 bit - double-sided
    };

    struct Triangle {
        uint16 vertices[3];
        uint16 texture;
    };

    struct Tile8 {
        uint8 index[256 * 256];
    };

    struct Room {

        struct Info {
            int32 x, z;
            int32 yBottom, yTop;
        } info;

        struct Data {
            uint32      size;       // Number of data words (uint16_t's)

            int16       vCount;
            int16       rCount;
            int16       tCount;
            int16       sCount;

            struct Vertex {
                TR::Vertex  vertex;
                int16       lighting;   // 0 (bright) .. 0x1FFF (dark)
            } *vertices;

            Rectangle   *rectangles;
            Triangle    *triangles;

            struct Sprite {
                int16       vertex;
                int16       texture;
            } *sprites;
        } data;

        uint16  portalsCount;
        uint16  zSectors;
        uint16  xSectors;
        uint16  ambient;    // 0 (bright) .. 0x1FFF (dark)
        uint16  lightsCount;
        uint16  meshesCount;
        int16   alternateRoom;
        int16   flags;

        struct Portal {
            uint16  roomIndex;
            Vertex  normal;
            Vertex  vertices[4];
        } *portals;

        struct Sector {
            uint16  floorIndex; // Index into FloorData[]
            uint16  boxIndex;   // Index into Boxes[] (-1 if none)
            uint8   roomBelow;  // 255 is none
            int8    floor;      // Absolute height of floor * 256
            uint8   roomAbove;  // 255 if none
            int8    ceiling;    // Absolute height of ceiling * 256
        } *sectors;

        struct Light {
            int32   x, y, z;
            uint16  align;          // ! not exists in file !
            uint16  intensity;
            uint32  attenuation;
        } *lights;

        struct Mesh {
            int32   x, y, z;
            uint16  rotation;
            uint16  intensity;
            uint16  meshID;
            uint16  align;          // ! not exists in file !
        } *meshes;
    };

    struct Mesh {
        Vertex  center;
        int32   radius;

        int16   vCount;
        Vertex  *vertices;  // List of vertices (relative coordinates)

        int16   nCount;
        union {
            Vertex  *normals;
            int16   *lights;    // if nCount < 0 -> (abs(nCount))
        };

        uint16      rCount;
        Rectangle   *rectangles;

        uint16      tCount;
        Triangle    *triangles;

        uint16      crCount;
        Rectangle   *crectangles;

        uint16      ctCount;
        Triangle    *ctriangles;
    };

    struct Entity {
        int16   id;             // Object Identifier (matched in Models[], or SpriteSequences[], as appropriate)
        int16   room;           // which room contains this item
        int32   x, y, z;        // world coords
        int16   rotation;       // ((0xc000 >> 14) * 90) degrees
        int16   intensity;      // (constant lighting; -1 means use mesh lighting)
        uint16  flags;          // 0x0100 indicates "initially invisible", 0x3e00 is Activation Mask
                                // 0x3e00 indicates "open" or "activated";  these can be XORed with
                                // related FloorData::FDlist fields (e.g. for switches)
        uint16  align;          // ! not exists in file !
    };

    struct Animation {
        uint32  frameOffset;    // Byte offset into Frames[] (divide by 2 for Frames[i])
        uint8   frameRate;      // Engine ticks per frame
        uint8   frameSize;      // Number of int16_t's in Frames[] used by this animation

        uint16  state;

        fixed   speed;
        fixed   accel;

        uint16  frameStart;     // First frame in this animation
        uint16  frameEnd;       // Last frame in this animation
        uint16  nextAnimation;
        uint16  nextFrame;

        uint16  scCount;
        uint16  scOffset;       // Offset into StateChanges[]

        uint16  acCount;        // How many of them to use.
        uint16  animCommand;    // Offset into AnimCommand[]
    };

    struct AnimState {
        uint16  state;
        uint16  rangesCount;    // number of ranges
        uint16  rangesOffset;   // Offset into animRanges[]
    };

    struct AnimRange {
        int16   low;            // Lowest frame that uses this range
        int16   high;           // Highest frame that uses this range
        int16   nextAnimation;  // Animation to dispatch to
        int16   nextFrame;      // Frame offset to dispatch to
    };

    struct AnimFrame {
        int16   minX, minY, minZ;   // Bounding box (low)
        int16   maxX, maxY, maxZ;   // Bounding box (high)
        int16   x, y, z;            // Starting offset for this model
        int16   aCount;
        uint16  angles[0];          // angle frames in YXZ order
    };

    struct AnimTexture {
        int16   tCount;             // Actually, this is the number of texture ID's - 1
        int16   textures[0];         // offsets into ObjectTextures[], in animation order
    };

    struct Node {
        uint32  flags;
        int32   x, y, z;
    };

    struct Model {
        uint32  id;         // Item Identifier (matched in Entities[])
        uint16  mCount;     // number of meshes in this object
        uint16  mStart;     // stating mesh (offset into MeshPointers[])
        uint32  node;       // offset into MeshTree[]
        uint32  frame;      // byte offset into Frames[] (divide by 2 for Frames[i])
        uint16  animation;  // offset into Animations[]
        uint16  align;      // ! not exists in file !
    };

    struct StaticMesh {
        uint32  id;             // Static Mesh Identifier
        uint16  mesh;           // Mesh (offset into MeshPointers[])
        Vertex  vBox[2];
        Vertex  cBox[2];
        uint16  flags;
    };

    struct ObjectTexture  {
        uint16  attribute;  // 0 - opaque, 1 - transparent, 2 - blend additive
        uint16  tileAndFlag;    // 0..14 - tile, 15 - is triangle
        struct {
            uint8   Xcoordinate; // 1 if Xpixel is the low value, 255 if Xpixel is the high value in the object texture
            uint8   Xpixel;
            uint8   Ycoordinate; // 1 if Ypixel is the low value, 255 if Ypixel is the high value in the object texture
            uint8   Ypixel;
        } vertices[4]; // The four corners of the texture
    };

    struct SpriteTexture {
        uint16  tile;
        uint8   u, v;
        uint16  w, h;
        int16   l, t, r, b;
    };

    struct SpriteSequence {
        int32   id;         // Sprite identifier
        int16   sCount;     // Negative of ``how many sprites are in this sequence''
        int16   sStart;     // Where (in sprite texture list) this sequence starts
    };

    struct Camera {
        int32   x, y, z;
        int16   room;
        uint16  flags;
    };

    struct CameraFrame {
        int16   rotY;
        int16   rotZ;
        int16   unused1;
        int16   posZ;
        int16   posY;
        int16   posX;
        int16   unknown;
        int16   rotX;
    };

    struct SoundSource {
        int32   x, y, z;    // absolute position of sound source (world coordinates)
        uint16  id;         // internal sample index
        uint16  flags;      // 0x40, 0x80, or 0xC0
    };

    struct Box {
        int32   minZ, maxZ; // Horizontal dimensions in global units
        int32   minX, maxX;
        int16   floor;      // Height value in global units
        int16   overlap;    // Index into Overlaps[].
    };

    struct Zone {
        struct {
            uint16  groundZone1;
            uint16  groundZone2;
            uint16  flyZone;
        } normal, alternate;
    };

    struct SoundInfo {
       uint16 index;    // (index into soundsIndices) -- NOT USED IN TR4-5!!!
       uint16 volume;
       uint16 chance;   // If !=0 and ((rand()&0x7fff) > Chance), this sound is not played
       uint16 flags;    // Bits 0-1: Looped flag, bits 2-5: num samples, bits 6-7: UNUSED
    };
    #pragma pack(pop)

    struct Level {
        char            *data;

        uint32          version;    // version (4 bytes)

        int32           tilesCount;
        Tile8           *tiles;     // 8-bit (palettized) textiles 256x256

        uint32          unused;     // 32-bit unused value (4 bytes)

        uint16          roomsCount;
        Room            *rooms;

        int32           floorsCount;
        uint16          *floors;

        int32           meshDataSize;
        uint16          *meshData;

        int32           meshOffsetsCount;
        uint32          *meshOffsets;

        int32           animsCount;
        Animation       *anims;

        int32           statesCount;
        AnimState       *states;

        int32           rangesCount;
        AnimRange       *ranges;

        int32           commandsCount;
        int16           *commands;

        int32           nodesDataSize;
        uint32          *nodesData;

        int32           frameDataSize;
        uint16          *frameData;

        int32           modelsCount;
        Model           *models;

        int32           staticMeshesCount;
        StaticMesh      *staticMeshes;

        int32           objectTexturesCount;
        ObjectTexture   *objectTextures;

        int32           spriteTexturesCount;
        SpriteTexture   *spriteTextures;

        int32           spriteSequencesCount;
        SpriteSequence  *spriteSequences;

        int32           camerasCount;
        Camera          *camera;

        int32           soundSourcesCount;
        SoundSource     *soundSources;

        int32           boxesCount;
        Box             *boxes;
        int32           overlapsCount;
        uint16          *overlaps;
        Zone            *zones;

        int32           animTexturesDataSize;
        uint16          *animTexturesData;

        int32           entitiesCount;
        Entity          *entities;

        RGB             *palette;

        uint16          cameraFramesCount;
        CameraFrame     *cameraFrames;

        uint16          demoDataSize;
        uint8           *demoData;

        int16           *soundsMap;

        int32           soundsInfoCount;
        SoundInfo       *soundsInfo;

        int32           soundDataSize;
        uint8           *soundData;

        int32           soundOffsetsCount;
        uint32          *soundOffsets;

        Level(Stream &stream) {
        // read version
            stream.read(version);
        // tiles
            stream.read(tiles, stream.read(tilesCount));
            stream.read(unused);
        // rooms
            rooms = new Room[stream.read(roomsCount)];
            for (int i = 0; i < roomsCount; i++) {
                Room &r = rooms[i];
                Room::Data &d = r.data;
            // room info
                stream.read(r.info);
            // room data
                stream.read(d.size);
                int pos = stream.pos;
                stream.read(d.vertices,     stream.read(d.vCount));
                stream.read(d.rectangles,   stream.read(d.rCount));
                stream.read(d.triangles,    stream.read(d.tCount));
                stream.read(d.sprites,      stream.read(d.sCount));
                stream.setPos(pos + d.size * 2);
            // portals
                stream.read(r.portals,  stream.read(r.portalsCount));
            // sectors
                stream.read(r.zSectors);
                stream.read(r.xSectors);
                stream.read(r.sectors, r.zSectors * r.xSectors);
            // ambient light luminance
                stream.read(r.ambient);
            // lights
                r.lights = new Room::Light[stream.read(r.lightsCount)];
                for (int i = 0; i < r.lightsCount; i++) {
                    TR::Room::Light &light = r.lights[i];
                    stream.read(light.x);
                    stream.read(light.y);
                    stream.read(light.z);
                    stream.read(light.intensity);
                    stream.read(light.attenuation);
                }
            //    stream.read(r.lights,   stream.read(r.lightsCount));
            // meshes
                r.meshes = new Room::Mesh[stream.read(r.meshesCount)];
                for (int i = 0; i < r.meshesCount; i++)
                    stream.raw(&r.meshes[i], sizeof(r.meshes[i]) - sizeof(r.meshes[i].align));
            //    stream.read(r.meshes,   stream.read(r.meshesCount));
            // misc flags
                stream.read(r.alternateRoom);
                stream.read(r.flags);
            }

        // floors
            stream.read(floors,         stream.read(floorsCount));
        // meshes
            stream.read(meshData,       stream.read(meshDataSize));
            stream.read(meshOffsets,    stream.read(meshOffsetsCount));
        // animations
            stream.read(anims,          stream.read(animsCount));
            stream.read(states,         stream.read(statesCount));
            stream.read(ranges,         stream.read(rangesCount));
            stream.read(commands,       stream.read(commandsCount));
            stream.read(nodesData,      stream.read(nodesDataSize));
            stream.read(frameData,      stream.read(frameDataSize));
        // models
            models = new Model[stream.read(modelsCount)];
            for (int i = 0; i < modelsCount; i++)
                stream.raw(&models[i], sizeof(models[i]) - sizeof(models[i].align));
        //    stream.read(models,         stream.read(modelsCount));
            stream.read(staticMeshes,   stream.read(staticMeshesCount));
        // textures & UV
            stream.read(objectTextures,     stream.read(objectTexturesCount));
            stream.read(spriteTextures,     stream.read(spriteTexturesCount));
            stream.read(spriteSequences,    stream.read(spriteSequencesCount));

        #ifdef TR1_DEMO
            stream.read(palette,        256);
        #endif

        // cameras
            stream.read(camera,         stream.read(camerasCount));
        // sound sources
            stream.read(soundSources,   stream.read(soundSourcesCount));
        // AI
            stream.read(boxes,          stream.read(boxesCount));
            stream.read(overlaps,       stream.read(overlapsCount));
            stream.read(zones,          boxesCount);
        // animated textures
            stream.read(animTexturesData,   stream.read(animTexturesDataSize));
        // entities (enemies, items, lara etc.)
            entities = new Entity[stream.read(entitiesCount)];
            for (int i = 0; i < entitiesCount; i++)
                stream.raw(&entities[i], sizeof(entities[i]) - sizeof(entities[i].align));
        //    stream.read(entities,       stream.read(entitiesCount));
        // palette
            stream.seek(32 * 256);  // skip lightmap palette

        #ifndef TR1_DEMO
            stream.read(palette,        256);
        #endif

        // cinematic frames for cameras
            stream.read(cameraFrames,   stream.read(cameraFramesCount));
        // demo data
            stream.read(demoData,       stream.read(demoDataSize));
        // sounds
            stream.read(soundsMap,      256);
            stream.read(soundsInfo,     stream.read(soundsInfoCount));
            stream.read(soundData,      stream.read(soundDataSize));
            stream.read(soundOffsets,   stream.read(soundOffsetsCount));

        // modify palette colors from 6-bit Amiga colorspace
            int m = 0;
            for (int i = 0; i < 256; i++) {
                RGB &c = palette[i];
                c.r <<= 2;
                c.g <<= 2;
                c.b <<= 2;
            }
        }

        ~Level() {
            delete[] tiles;
        // rooms
            for (int i = 0; i < roomsCount; i++) {
                Room &r = rooms[i];
                delete[] r.data.vertices;
                delete[] r.data.rectangles;
                delete[] r.data.triangles;
                delete[] r.data.sprites;
                delete[] r.portals;
                delete[] r.sectors;
                delete[] r.lights;
                delete[] r.meshes;
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
            delete[] camera;
            delete[] soundSources;
            delete[] boxes;
            delete[] overlaps;
            delete[] zones;
            delete[] animTexturesData;
            delete[] entities;
            delete[] palette;
            delete[] cameraFrames;
            delete[] demoData;
            delete[] soundsMap;
            delete[] soundsInfo;
            delete[] soundData;
            delete[] soundOffsets;
        }
    };
}

#endif