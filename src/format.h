#ifndef H_FORMAT
#define H_FORMAT

#include "utils.h"

#define TR1_DEMO

#define MAX_RESERVED_ENTITIES 64
#define MAX_SECRETS_COUNT     16

namespace TR {

    enum {
        ANIM_CMD_NONE       ,
        ANIM_CMD_MOVE       ,
        ANIM_CMD_SPEED      ,
        ANIM_CMD_EMPTY      ,
        ANIM_CMD_KILL       ,
        ANIM_CMD_SOUND      ,
        ANIM_CMD_EFFECT     ,
    };

    // https://dl.dropboxusercontent.com/u/62482708/Secret/TR4%26TR5%20PSX%20Stuff.zip
    enum {
        EFFECT_ROTATE_180       ,
        EFFECT_FLOOR_SHAKE      ,
        EFFECT_LARA_NORMAL      ,
        EFFECT_LARA_BUBBLES     ,
        EFFECT_FINISH_LEVEL     ,
        EFFECT_ACTIVATE_CAMERA  ,
        EFFECT_ACTIVATE_KEY     ,
        EFFECT_RUBBLEFX         ,
        EFFECT_CROWBAR          ,
        EFFECT_CURTAINFX        ,
        EFFECT_SETCHANGEFX      ,
        EFFECT_EXPLOSION_FX     ,
        EFFECT_LARA_HANDSFREE   ,
        EFFECT_FLIP_MAP         ,
        EFFECT_DRAW_RIGHTGUN    ,
        EFFECT_DRAW_LEFTGUN     ,
        EFFECT_SHOOT_RIGHTGUN   ,
        EFFECT_SHOOT_LEFTGUN    ,
        EFFECT_MESH_SWAP1       ,
        EFFECT_MESH_SWAP2       ,
        EFFECT_MESH_SWAP3       ,
        EFFECT_INV_ON           ,
        EFFECT_INV_OFF          ,
        EFFECT_DYN_ON           ,
        EFFECT_DYN_OFF          ,
        EFFECT_STATUEFX         ,
        EFFECT_RESET_HAIR       ,
        EFFECT_BOILERFX         ,
        EFFECT_SETFOG           ,
        EFFECT_GHOSTTRAP        ,
        EFFECT_LARALOCATION     ,
        EFFECT_CLEARSCARABS     ,
        EFFECT_FOOTPRINT_FX     ,
        EFFECT_FLIP_MAP0        ,
        EFFECT_FLIP_MAP1        ,
        EFFECT_FLIP_MAP2        ,
        EFFECT_FLIP_MAP3        ,
        EFFECT_FLIP_MAP4        ,
        EFFECT_FLIP_MAP5        ,
        EFFECT_FLIP_MAP6        ,
        EFFECT_FLIP_MAP7        ,
        EFFECT_FLIP_MAP8        ,
        EFFECT_FLIP_MAP9        ,
        EFFECT_POURSWAP1        ,
        EFFECT_POURSWAP2        ,
        EFFECT_LARALOCATIONPAD  ,
        EFFECT_KILLACTIVEBADDIES,
	};

    enum {
        SND_NO          = 2,
        SND_LANDING     = 4,
        SND_BUBBLE      = 37,
        SND_DART        = 151,
        SND_SECRET      = 173,
    };

    enum Action : uint16 {
        ACTIVATE        ,   // activate item
        CAMERA_SWITCH   ,   // switch to camera
        FLOW            ,   // underwater flow
        FLIP_MAP        ,   // flip map
        FLIP_ON         ,   // flip on
        FLIP_OFF        ,   // flip off
        CAMERA_TARGET   ,   // look at item
        END             ,   // end level
        SOUNDTRACK      ,   // play soundtrack
        HARDCODE        ,   // special hadrdcode trigger
        SECRET          ,   // secret found
        CLEAR           ,   // clear bodies
        CAMERA_FLYBY    ,   // flyby camera sequence
        CUTSCENE        ,   // play cutscene
    };

    #pragma pack(push, 1)

    struct fixed {
        uint16  L;
        int16   H;
        operator float() const {
            return H + L / 65535.0f;
        }
    };

    struct angle {
        uint16 value;

        angle() {}
        angle(float value) : value(uint16(value / (PI * 0.5f) * 16384.0f)) {}
        operator float() const { return value / 16384.0f * PI * 0.5f; };
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
        struct {
            uint16 water:1, unused:14, rendered:1;
        } flags;

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
            angle   rotation;
            uint16  intensity;
            uint16  meshID;
            struct { // ! not exists in file !
                uint16 unused:15, rendered:1;    
            } flags;
        } *meshes;
    };

    union FloorData {
        uint16 data;
        struct Command {
            uint16 func:8, sub:7, end:1;
        } cmd;
        struct Slant {
            int8 x:8, z:8;
        } slant;
        struct TriggerInfo {
            uint16  timer:8, once:1, mask:5, :2;
        } triggerInfo;
        union TriggerCommand {
            struct {
                uint16 args:10;
                Action action:5;
                uint16 end:1;
            };
            struct {
                uint16 delay:8, once:1; 
            };
        } triggerCmd;

        enum {
            NONE    ,
            PORTAL  ,
            FLOOR   ,
            CEILING ,
            TRIGGER ,
            KILL    ,
        };
    };

    struct Overlap {
        uint16 boxIndex:15, end:1;
    };

    struct Collider {
        uint16 radius:10, info:6;
        uint16 flags:16;
    };

    struct Mesh {
        Vertex      center;
        Collider    collider;

        uint16      vCount;
        Vertex      *vertices;  // List of vertices (relative coordinates)
 
        int16       nCount;
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
        enum Type : int16 {
            NONE                     = -1,
            LARA                     = 0,

            ENEMY_TWIN               = 6,
            ENEMY_WOLF               = 7,
            ENEMY_BEAR               = 8,
            ENEMY_BAT                = 9,
            ENEMY_CROCODILE_LAND     = 10,
            ENEMY_CROCODILE_WATER    = 11,
            ENEMY_LION_MALE          = 12,
            ENEMY_LION_FEMALE        = 13,
            ENEMY_PUMA               = 14,
            ENEMY_GORILLA            = 15,
            ENEMY_RAT_LAND           = 16,
            ENEMY_RAT_WATER          = 17,
            ENEMY_REX                = 18,
            ENEMY_RAPTOR             = 19,
            ENEMY_MUTANT             = 20,

            ENEMY_CENTAUR            = 23,
            ENEMY_MUMMY              = 24,
            ENEMY_LARSON             = 27,

            TRAP_FLOOR               = 35,
            TRAP_BLADE               = 36,
            TRAP_SPIKES              = 37,
            TRAP_BOULDER             = 38,
            TRAP_DART                = 39,
            TRAP_DARTGUN             = 40,

            SWITCH                   = 55,
            SWITCH_WATER             = 56,
            DOOR_1                   = 57,
            DOOR_2                   = 58,
            DOOR_3                   = 59,
            DOOR_4                   = 60,
            DOOR_BIG_1               = 61,
            DOOR_BIG_2               = 62,
            DOOR_5                   = 63,
            DOOR_6                   = 64,
            DOOR_FLOOR_1             = 65,
            DOOR_FLOOR_2             = 66,

            LARA_CUT                 = 77,

            CRYSTAL                  = 83,       // sprite
            WEAPON_PISTOLS           = 84,       // sprite
            WEAPON_SHOTGUN           = 85,       // sprite
            WEAPON_MAGNUMS           = 86,       // sprite
            WEAPON_UZIS              = 87,       // sprite
            AMMO_SHOTGUN             = 89,       // sprite
            AMMO_MAGNUMS             = 90,       // sprite
            AMMO_UZIS                = 91,       // sprite

            MEDIKIT_SMALL            = 93,       // sprite
            MEDIKIT_BIG              = 94,       // sprite

            PUZZLE_1                 = 110,      // sprite
            PUZZLE_2                 = 111,      // sprite
            PUZZLE_3                 = 112,      // sprite
            PUZZLE_4                 = 113,      // sprite

            HOLE_PUZZLE              = 118,

            PICKUP                   = 126,      // sprite

            KEY_1                    = 129,      // sprite
            KEY_2                    = 130,      // sprite
            KEY_3                    = 131,      // sprite
            KEY_4                    = 132,      // sprite

            HOLE_KEY                 = 137,

            ARTIFACT                 = 143,      // sprite

            WATER_SPLASH             = 153,      // sprite

            BUBBLE                   = 155,      // sprite

            BLOOD                    = 158,      // sprite

            SMOKE                    = 160,      // sprite

            SPARK                    = 164,      // sprite

            VIEW_TARGET              = 169,
   
            GLYPH                    = 190,      // sprite
        }       type;
        int16   room;
        int32   x, y, z;
        angle   rotation;
        int16   intensity;
        union {
            struct { uint16 unused:7, clear:1, invisible:1, active:5, unused2:1, rendered:1; };
            uint16 value;
        } flags;
    // not exists in file
        uint16  align;
        int16   modelIndex;     // index of representation in models (index + 1) or spriteSequences (-(index + 1)) arrays
        void    *controller;    // Controller implementation or NULL 
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

    struct MinMax {
        int16 minX, maxX, minY, maxY, minZ, maxZ;

        vec3 min() const { return vec3((float)minX, (float)minY, (float)minZ); }
        vec3 max() const { return vec3((float)maxX, (float)maxY, (float)maxZ); }
    };

    struct AnimFrame {
        MinMax  box;
        Vertex  pos;   // Starting offset for this model
        int16   aCount;
        uint16  angles[0];          // angle frames in YXZ order

        vec3 getAngle(int index) {
            #define ANGLE_SCALE (2.0f * PI / 1024.0f)

            uint16 b = angles[index * 2 + 0];
            uint16 a = angles[index * 2 + 1];

            return vec3((a & 0x3FF0) >> 4, ( ((a & 0x000F) << 6) | ((b & 0xFC00) >> 10)), b & 0x03FF) * ANGLE_SCALE;
        }
    };

    struct AnimTexture {
        int16   count;        // number of texture offsets - 1 in group
        int16   textures[0];  // offsets into objectTextures[]
    };

    struct Node {
        uint32  flags;
        int32   x, y, z;
    };

    struct Model {
        Entity::Type    type;
        uint16          unused;
        uint16          mCount;    
        uint16          mStart;    
        uint32          node;      
        uint32          frame;     
        uint16          animation; 
        uint16          align;     
    };

    struct StaticMesh {
        uint32  id;    
        uint16  mesh;  
        MinMax  vbox;
        MinMax  cbox;
        uint16  flags;

        void getBox(bool collision, angle rotation, ::Box &box) {
            int k = rotation.value / 0x4000;

            MinMax &m = collision ? cbox : vbox;

            ASSERT(m.minX <= m.maxX && m.minY <= m.maxY && m.minZ <= m.maxZ);

            box = ::Box(m.min(), m.max());
            box.rotate90(k);

            ASSERT(box.min.x <= box.max.x && box.min.y <= box.max.y && box.min.z <= box.max.z);
        }
    };

    struct Tile {
        uint16 index:14, undefined:1, triangle:1;  // undefined - need check is animated, animated - is animated
    };

    struct ObjectTexture  {
        uint16  attribute;  // 0 - opaque, 1 - transparent, 2 - blend additive
        Tile    tile;       // 0..14 - tile, 15 - is triangle
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
        uint16  w, h;   // (ActualValue  * 256) + 255
        int16   l, t, r, b;
    };

    struct SpriteSequence {
        Entity::Type    type;
        uint16          unused;
        int16           sCount;    
        int16           sStart;    
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
        uint32  minZ, maxZ; // Horizontal dimensions in global units
        uint32  minX, maxX;
        int16   floor;      // Height value in global units
        uint16  overlap;    // Index into Overlaps[].

        bool contains(uint32 x, uint32 z) {
            return x >= minX && x <= maxX && z >= minZ && z <= maxZ;
        }
    };

    struct Zone {
        struct {
            uint16  groundZone1;
            uint16  groundZone2;
            uint16  flyZone;
        } normal, alternate;
    };

    struct SoundInfo {
       uint16 offset;
       uint16 volume;
       uint16 chance;   // If !=0 and ((rand()&0x7fff) > Chance), this sound is not played
       uint16 flags;    // Bits 0-1: Looped flag, bits 2-5: num samples, bits 6-7: UNUSED
    };

    #pragma pack(pop)

    struct Level {
        uint32          version;    // version (4 bytes)

        int32           tilesCount;
        Tile8           *tiles;     // 8-bit (palettized) textiles 256x256

        uint32          unused;     // 32-bit unused value (4 bytes)

        uint16          roomsCount;
        Room            *rooms;

        int32           floorsCount;
        FloorData       *floors;

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
        Camera          *cameras;

        int32           soundSourcesCount;
        SoundSource     *soundSources;

        int32           boxesCount;
        Box             *boxes;
        int32           overlapsCount;
        Overlap         *overlaps;
        Zone            *zones;

        int32           animTexturesDataSize;
        uint16          *animTexturesData;

        int32           entitiesBaseCount;
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

   // common
        enum Trigger : uint32 {
            ACTIVATE    ,
            PAD         ,
            SWITCH      ,
            KEY         ,
            PICKUP      ,
            HEAVY       ,
            ANTIPAD     ,
            COMBAT      ,
            DUMMY       ,
            ANTI        ,
        };
    
        struct FloorInfo {
            int floor, ceiling;
            int slantX, slantZ;
            int roomNext, roomBelow, roomAbove;
            int floorIndex;
            int kill;
            int trigCmdCount;
            Trigger trigger;
            FloorData::TriggerInfo trigInfo;
            FloorData::TriggerCommand trigCmd[16];

            vec3 getNormal() {
                return vec3((float)-slantX, -4.0f, (float)-slantZ).normal();
            }

            vec3 getSlant(const vec3 &dir) {
                // project floor normal into plane(dir, up) 
                vec3 r = vec3(dir.z, 0.0f, -dir.x); // up(0, 1, 0).cross(dir)
                vec3 n = getNormal();
                n = n - r * r.dot(n);
                // project dir into plane(dir, n)
                return n.cross(dir.cross(n)).normal();
            }
        };

        bool    secrets[MAX_SECRETS_COUNT];
        void    *cameraController;

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
                    Room::Light &light = r.lights[i];
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
                    stream.raw(&r.meshes[i], sizeof(r.meshes[i]) - sizeof(r.meshes[i].flags));
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
            stream.read(staticMeshes,   stream.read(staticMeshesCount));
        // textures & UV
            stream.read(objectTextures,     stream.read(objectTexturesCount));
            stream.read(spriteTextures,     stream.read(spriteTexturesCount));
            stream.read(spriteSequences,    stream.read(spriteSequencesCount));
            for (int i = 0; i < spriteSequencesCount; i++)
                spriteSequences[i].sCount = -spriteSequences[i].sCount;

        #ifdef TR1_DEMO
            stream.read(palette,        256);
        #endif

        // cameras
            stream.read(cameras,        stream.read(camerasCount));
        // sound sources
            stream.read(soundSources,   stream.read(soundSourcesCount));
        // AI
            stream.read(boxes,          stream.read(boxesCount));
            stream.read(overlaps,       stream.read(overlapsCount));
            stream.read(zones,          boxesCount);
        // animated textures
            stream.read(animTexturesData,   stream.read(animTexturesDataSize));
        // entities (enemies, items, lara etc.)
            entitiesCount = stream.read(entitiesBaseCount) + MAX_RESERVED_ENTITIES;
            entities = new Entity[entitiesCount];
            for (int i = 0; i < entitiesBaseCount; i++) {
                Entity &e = entities[i];
                stream.raw(&e, sizeof(e) - sizeof(e.align) - sizeof(e.controller) - sizeof(e.modelIndex));
                e.align = 0;
                e.controller = NULL;
                e.modelIndex = getModelIndex(e.type);
            }
            for (int i = entitiesBaseCount; i < entitiesCount; i++) {
                entities[i].type = Entity::NONE;
                entities[i].controller = NULL;
            }
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

            memset(secrets, 0, MAX_SECRETS_COUNT * sizeof(secrets[0]));
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
            delete[] cameras;
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

    // common methods

        StaticMesh* getMeshByID(int id) const { // TODO: map this
            for (int i = 0; i < staticMeshesCount; i++)
                if (staticMeshes[i].id == id)
                    return &staticMeshes[i];
            return NULL;
        }

        int16 getModelIndex(Entity::Type type) const {
            for (int i = 0; i < modelsCount; i++)
                if (type == models[i].type)
                    return i + 1;
    
            for (int i = 0; i < spriteSequencesCount; i++)
                if (type == spriteSequences[i].type)
                    return -(i + 1);

            ASSERT(false);
            return 0;
        }

        int entityAdd(TR::Entity::Type type, int16 room, int32 x, int32 y, int32 z, angle rotation, int16 intensity) {
            int entityIndex = -1;
            for (int i = entitiesBaseCount; i < entitiesCount; i++) 
                if (entities[i].type == Entity::NONE) {
                    Entity &e = entities[i];
                    e.type          = type;
                    e.room          = room;
                    e.x             = x;
                    e.y             = y;
                    e.z             = z;
                    e.rotation      = rotation;
                    e.intensity     = intensity;
                    e.flags.value   = 0;
                    e.modelIndex    = getModelIndex(e.type);
                    e.controller    = NULL;
                    return i;
                }
            return -1;
        }

        void entityRemove(int entityIndex) {
            entities[entityIndex].type       = Entity::NONE;
            entities[entityIndex].controller = NULL;
        }

        Room::Sector& getSector(int roomIndex, int x, int z, int &dx, int &dz) const {
            ASSERT(roomIndex >= 0 && roomIndex < roomsCount);
            Room &room = rooms[roomIndex];

            int sx = x - room.info.x;
            int sz = z - room.info.z;

            sx = clamp(sx, 0, (room.xSectors - 1) * 1024);
            sz = clamp(sz, 0, (room.zSectors - 1) * 1024);

            dx = sx % 1024;
            dz = sz % 1024;
            sx /= 1024;
            sz /= 1024;

            return room.sectors[sx * room.zSectors + sz];
        }

        void getFloorInfo(int roomIndex, int x, int z, FloorInfo &info, bool actual = false, int prevRoom = 0xFF) const {
            int dx, dz;
            Room::Sector &s = getSector(roomIndex, x, z, dx, dz);

            info.floor        = 256 * (int)s.floor;
            info.ceiling      = 256 * (int)s.ceiling;
            info.slantX       = 0;
            info.slantZ       = 0;
            info.roomNext     = 255;
            info.roomBelow    = s.roomBelow;
            info.roomAbove    = s.roomAbove;
            info.floorIndex   = s.floorIndex;
            info.kill         = 0;
            info.trigger      = Trigger::ACTIVATE;
            info.trigCmdCount = 0;

            if (actual) {
                if (info.roomBelow != 0xFF && info.roomBelow != prevRoom) {
                    FloorInfo tmp;
                    getFloorInfo(info.roomBelow, x, z, tmp, true, roomIndex);
                    info.floor = tmp.floor;
                }

                if (info.roomAbove != 0xFF && info.roomAbove != prevRoom) {
                    FloorInfo tmp;
                    getFloorInfo(info.roomAbove, x, z, tmp, true, roomIndex);
                    info.ceiling = tmp.ceiling;
                }
            }

            if (!s.floorIndex) return;

            FloorData *fd = &floors[s.floorIndex];
            FloorData::Command cmd;

            do {
                cmd = (*fd++).cmd;
                
                switch (cmd.func) {

                    case FloorData::PORTAL  :
                        info.roomNext = (*fd++).data;
                        break;

                    case FloorData::FLOOR   : // floor & ceiling
                    case FloorData::CEILING : { 
                        FloorData::Slant slant = (*fd++).slant;
                        int sx = (int)slant.x;
                        int sz = (int)slant.z;
                        if (cmd.func == FloorData::FLOOR) {
                            info.slantX = sx;
                            info.slantZ = sz;
                            info.floor -= sx * (sx > 0 ? (dx - 1024) : dx) >> 2;
                            info.floor -= sz * (sz > 0 ? (dz - 1024) : dz) >> 2;
                        } else {
                            info.ceiling -= sx * (sx < 0 ? (dx - 1024) : dx) >> 2; 
                            info.ceiling += sz * (sz > 0 ? (dz - 1024) : dz) >> 2; 
                        }
                        break;
                    }

                    case FloorData::TRIGGER :  {
                        info.trigger        = (Trigger)cmd.sub;
                        info.trigCmdCount   = 0;
                        info.trigInfo       = (*fd++).triggerInfo;
                        FloorData::TriggerCommand trigCmd;
                        do {
                            trigCmd = (*fd++).triggerCmd; // trigger action
                            info.trigCmd[info.trigCmdCount++] = trigCmd;
                        } while (!trigCmd.end);                       
                        break;
                    }

                    case FloorData::KILL :
                        info.kill = 1;
                        break;

                    default : LOG("unknown func: %d\n", cmd.func);
                }

            } while (!cmd.end);

            if (actual && info.roomNext != 0xFF)
                getFloorInfo(info.roomNext, x, z, info, actual, prevRoom);
        }

    }; // struct Level

    bool castShadow(Entity::Type type) {
        return (type >= Entity::ENEMY_TWIN && type <= Entity::ENEMY_LARSON) || type == Entity::LARA || type == Entity::LARA_CUT;
    }
}

#endif