#ifndef H_FORMAT
#define H_FORMAT

#include "utils.h"

#define MAX_RESERVED_ENTITIES 128
#define MAX_SECRETS_COUNT     16
#define MAX_TRIGGER_COMMANDS  32
#define MAX_MESHES            512

namespace TR {

    enum {
        FLOOR_BLOCK = -127,
        NO_ROOM     = 0xFF,
    };

    enum {
        ANIM_CMD_NONE       ,
        ANIM_CMD_OFFSET     ,
        ANIM_CMD_JUMP       ,
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
        SND_NO              = 2,

        SND_LANDING         = 4,
        
        SND_UNHOLSTER       = 6,
        SND_HOLSTER         = 7,
        SND_PISTOLS_SHOT    = 8,
        SND_SHOTGUN_RELOAD  = 9,
        SND_RICOCHET        = 10,
        
        SND_SCREAM          = 30,
        SND_HIT             = 31,
        
        SND_BUBBLE          = 37,
         
        SND_UZIS_SHOT       = 43,
        SND_MAGNUMS_SHOT    = 44,
        SND_SHOTGUN_SHOT    = 45,
        
        SND_UNDERWATER      = 60,
        
        SND_MENU_SPIN       = 108,
        SND_MENU_HOME       = 109,
        SND_MENU_CONTROLS   = 110,
        SND_MENU_SHOW       = 111,
        SND_MENU_HIDE       = 112,
        SND_MENU_COMPASS    = 113,
        SND_MENU_WEAPON     = 114,
        SND_MENU_PAGE       = 115,
        SND_HEALTH          = 116,
        
        SND_DART            = 151,
        
        SND_SECRET          = 173,
    };

    enum {
        MODEL_LARA          = 0,
        MODEL_PISTOLS       = 1,
        MODEL_SHOTGUN       = 2,
        MODEL_MAGNUMS       = 3,
        MODEL_UZIS          = 4,
        MODEL_LARA_SPEC     = 5,
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

    struct Color32 {
        uint8 r, g, b, a;

        Color32() {}
        Color32(uint8 r, uint8 g, uint8 b, uint8 a) : r(r), g(g), b(b), a(a) {}
    };


    struct Color24 {
        uint8 r, g, b;

        Color24() {}
        Color24(uint8 r, uint8 g, uint8 b) : r(r), g(g), b(b) {}
    };

    struct Color16 {
        uint16 r:5, g:5, b:5, a:1;

        operator Color24() const { return Color24(r << 3, g << 3, b << 3); }
        operator Color32() const { return Color32(r << 3, g << 3, b << 3, -a); }
    };

    struct Vertex {
        int16 x, y, z;

        operator vec3() const { return vec3((float)x, (float)y, (float)z); };
    };

    struct Rectangle {
        uint16 vertices[4];
        uint16 texture;
    };

    struct Triangle {
        uint16 vertices[3];
        uint16 texture;
    };

    struct Tile4 {
        struct {
            uint8 a:4, b:4;
        } index[256 * 256 / 2];
    };

    struct Tile8 {
        uint8 index[256 * 256];
    };

    struct Tile32 {
        Color32 color[256 * 256];
    };

    struct CLUT {
        Color16 color[16];
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

            vec3 getCenter() const {
                return vec3(float( (int(vertices[0].x) + int(vertices[1].x) + int(vertices[2].x) + int(vertices[3].x)) / 4 ),
                            float( (int(vertices[0].y) + int(vertices[1].y) + int(vertices[2].y) + int(vertices[3].y)) / 4 ),
                            float( (int(vertices[0].z) + int(vertices[1].z) + int(vertices[2].z) + int(vertices[3].z)) / 4 ));
            }

            vec3 getSize() const {
                return vec3(float( abs(int(vertices[0].x) - int(vertices[2].x)) / 2 ),
                            float( abs(int(vertices[0].y) - int(vertices[2].y)) / 2 ),
                            float( abs(int(vertices[0].z) - int(vertices[2].z)) / 2 ));
            }
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
            uint32  radius;
        } *lights;

        struct Mesh {
            int32   x, y, z;
            angle   rotation;
            int16   intensity;
            uint16  meshID;
            uint16  align; // PSX
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
            uint16 value;
            struct {
                uint16 args:10;
                Action action:5;
                uint16 end:1;
            };
            struct {
                uint16 delay:8, once:1, timer:7;
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

    //struct Collider {
    //    uint16 radius:10, info:6;
    //    uint16 flags:16;
    //};

    // internal mesh structure
    struct Mesh {

        struct Vertex {
            short4 coord;
            short4 normal;
        };

        TR::Vertex  center;
        uint16      radius;
        uint16      flags;
        int16       vCount;
        int16       rCount;
        int16       tCount;

        int32       offset;
        Vertex      *vertices;
        Rectangle   *rectangles;
        Triangle    *triangles;

        Mesh() : vertices(0), rectangles(0), triangles(0) {}
        ~Mesh() {
            delete[] vertices;
            delete[] rectangles;
            delete[] triangles;
        }
    };

    struct Entity {
        enum Type : int16 {
            NONE                     = -1,
            LARA                     = 0,
            LARA_PISTOLS             = 1,
            LARA_SHOTGUN             = 2,
            LARA_MAGNUMS             = 3,
            LARA_UZIS                = 4,

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

            BLOCK_1                  = 48,
            BLOCK_2                  = 49,

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
            TRAP_DOOR_1              = 65,
            TRAP_DOOR_2              = 66,

            BRIDGE_0                 = 68,
            BRIDGE_1                 = 69,
            BRIDGE_2                 = 70,

            GEARS_1                  = 74,
            GEARS_2                  = 75,
            GEARS_3                  = 76,

            CUT_1                    = 77,
            CUT_2                    = 78,
            CUT_3                    = 79,
            CUT_4                    = 79,

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
            HOLE_PUZZLE_SET          = 122,

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

            MUZZLE_FLASH             = 166,

            VIEW_TARGET              = 169,      // invisible
            WATERFALL                = 170,      // invisible (water splash generator)

            BRAID                    = 189,      // Lara's ponytail
            GLYPH                    = 190,      // sprite

        }       type;
        int16   room;
        int32   x, y, z;
        angle   rotation;
        int16   intensity;
        union {
            struct { uint16 unused:7, clear:1, invisible:1, active:5, collision:1, rendered:1; };
            uint16 value;
        } flags;
    // not exists in file
        uint16  align;
        int16   modelIndex;     // index of representation in models (index + 1) or spriteSequences (-(index + 1)) arrays
        void    *controller;    // Controller implementation or NULL 

        bool isEnemy() {
            return type >= ENEMY_TWIN && type <= ENEMY_LARSON;
        }

        int isItem() {
            return (type >= WEAPON_PISTOLS && type <= AMMO_UZIS) ||
                   (type >= PUZZLE_1 && type <= PUZZLE_4) ||
                   (type >= KEY_1 && type <= KEY_4) ||
                   (type == MEDIKIT_SMALL || type == MEDIKIT_BIG || type == ARTIFACT || type == PICKUP);
        }

        bool isBlock() {
            return type >= TR::Entity::BLOCK_1 && type <= TR::Entity::BLOCK_2;
        }
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

    #pragma warning( push )
    #pragma warning( disable : 4200 ) // zero-sized array warning
    struct AnimFrame {
        MinMax  box;
        Vertex  pos;
        int16   aCount;
        uint16  angles[0];  // angle frames in YXZ order

        vec3 getAngle(int index) {
            #define ANGLE_SCALE (2.0f * PI / 1024.0f)

            uint16 b = angles[index * 2 + 0];
            uint16 a = angles[index * 2 + 1];

            return vec3(float((a & 0x3FF0) >> 4), float( ((a & 0x000F) << 6) | ((b & 0xFC00) >> 10)), float(b & 0x03FF)) * ANGLE_SCALE;
        }
    };

    struct AnimTexture {
        int16   count;        // number of texture offsets - 1 in group
        int16   textures[0];  // offsets into objectTextures[]
    };
    #pragma warning( push )

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
        uint16 index:14, undefined:1, triangle:1;
    };

    struct ObjectTexture {
        uint16  clut;
        Tile    tile;           // tile or palette index
        uint16  attribute;      // 0 - opaque, 1 - transparent, 2 - blend additive 
        ubyte2  texCoord[4];
    };

    struct SpriteTexture {
        uint16  clut;
        uint16  tile;
        int16   l, t, r, b;
        ubyte2  texCoord[2];
    };

    struct SpriteSequence {
        Entity::Type    type;
        uint16          unused;
        int16           sCount;    
        int16           sStart;    
    };

    struct Camera {
        int32   x, y, z;
        union {
            int16   room;   // for camera
            int16   speed;  // for sink (underwater current)
        };
        uint16  flags;
    };

    struct CameraFrame {
        Vertex  target;
        Vertex  pos;
        int16   fov;
        int16   roll;
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
        union {
            struct { uint16 mode:2, count:4, unused:6, fixed:1, pitch:1, gain:1, :1; };
            uint16 value;
        } flags;
    };

    #pragma pack(pop)

    struct Level {
        enum : uint32 {
            VER_TR1_PC  = 0x00000020,
            VER_TR1_PSX = 0x56414270,
        }               version;

        int32           tilesCount;
        Tile32          *tiles;

        uint32          unused;

        uint16          roomsCount;
        Room            *rooms;

        int32           floorsCount;
        FloorData       *floors;

        int16           meshesCount;
        Mesh            meshes[MAX_MESHES];

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

        Color24         *palette;
        CLUT            *cluts;
        Tile4           *tiles4;

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
        uint32          *soundSize;

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
            int roomFloor, roomCeiling;
            int roomNext, roomBelow, roomAbove;
            int floor, ceiling;
            int slantX, slantZ;
            int floorIndex;
            int kill;
            int trigCmdCount;
            Trigger trigger;
            FloorData::TriggerInfo trigInfo;
            FloorData::TriggerCommand trigCmd[MAX_TRIGGER_COMMANDS];

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

        int     cutEntity;
        mat4    cutMatrix;

        struct {
            int16 muzzleFlash;
            int16 puzzleSet;
            int16 weapons[4];
            int16 braid;
        } extra;

        Level(Stream &stream, bool demo) {
            memset(this, 0, sizeof(*this));
            cutEntity = -1;
            Tile8 *tiles8 = NULL;

            int soundOffset = 0;

            stream.read(version);
            if (version != VER_TR1_PC) {
                soundOffset = version;
                stream.read(version);
            }

            if (version && version != VER_TR1_PC && version != VER_TR1_PSX) {
                LOG("unsupported level format\n"); 
                ASSERT(false); 
                memset(this, 0, sizeof(*this)); 
                return;
            }

            if (version == VER_TR1_PSX) {
                uint32 offsetTexTiles;

                stream.seek(8);
                stream.read(offsetTexTiles);
            // sound offsets
                uint16 numSounds;
                stream.setPos(22);
                stream.read(numSounds);
                stream.setPos(2086 + numSounds * 512);
                soundOffsetsCount = numSounds;
                soundOffsets = new uint32[soundOffsetsCount];
                soundSize    = new uint32[soundOffsetsCount];
                uint32 soundDataSize = 0;
                for (int i = 0; i < soundOffsetsCount; i++) {
                    soundOffsets[i] = soundDataSize;
                    uint16 size;
                    stream.read(size);
                    soundDataSize += soundSize[i] = size * 8;
                }           
            // sound data
                stream.setPos(2600 + numSounds * 512);
                stream.read(soundData, soundDataSize);
                stream.setPos(offsetTexTiles + 8);
            } else if (version == VER_TR1_PC) {
            // tiles
                stream.read(tiles8, stream.read(tilesCount));
            }

            if (!version /*PSX cutscene*/ || version == VER_TR1_PSX) {
                version = VER_TR1_PSX;
            // tiles
                stream.read(tiles4, tilesCount = 13);
                stream.read(cluts, 512);                
                stream.seek(0x4000);
            }
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
                if (version == VER_TR1_PSX) stream.seek(2);
                stream.read(d.vertices, stream.read(d.vCount));

                if (version == VER_TR1_PSX)
                    for (int j = 0; j < d.vCount; j++) // convert vertex luminance from PSX to PC format
                        d.vertices[j].lighting = 0x1FFF - (d.vertices[j].lighting << 5);               

                stream.read(d.rectangles, stream.read(d.rCount));
                
                if (version == VER_TR1_PSX)
                    for (int j = 0; j < d.rCount; j++) // swap indices (quad strip -> quad list)
                        swap(d.rectangles[j].vertices[2], d.rectangles[j].vertices[3]);
                        
                stream.read(d.triangles,    stream.read(d.tCount));
                stream.read(d.sprites,      stream.read(d.sCount));
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
                    if (version == VER_TR1_PSX) {
                        uint32 intensity;
                        light.intensity = stream.read(intensity);
                    } else
                        stream.read(light.intensity);
                    stream.read(light.radius);

                    light.radius *= 2;
                }
            // meshes
                stream.read(r.meshesCount);
                r.meshes = r.meshesCount ? new Room::Mesh[r.meshesCount] : NULL;
                for (int i = 0; i < r.meshesCount; i++)
                    stream.raw(&r.meshes[i], sizeof(r.meshes[i]) - (version == VER_TR1_PC ? sizeof(r.meshes[i].align) : 0));
            // misc flags
                stream.read(r.alternateRoom);
                stream.read(r.flags);
            }

        // floors
            stream.read(floors,         stream.read(floorsCount));
        // meshes
            readMeshes(stream);
        // animations
            stream.read(anims,          stream.read(animsCount));
            stream.read(states,         stream.read(statesCount));
            stream.read(ranges,         stream.read(rangesCount));
            stream.read(commands,       stream.read(commandsCount));
            stream.read(nodesData,      stream.read(nodesDataSize));
            stream.read(frameData,      stream.read(frameDataSize));
        // models
            stream.read(modelsCount);
            models = modelsCount ? new Model[modelsCount] : NULL;
            for (int i = 0; i < modelsCount; i++)
                stream.raw(&models[i], sizeof(models[i]) - (version == VER_TR1_PC ? sizeof(models[i].align) : 0));
            stream.read(staticMeshes, stream.read(staticMeshesCount));
        // textures & UV
            readObjectTex(stream);
            readSpriteTex(stream);
        // palette for demo levels
            if (version == VER_TR1_PC && demo) stream.read(palette, 256);
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
                if (e.type == Entity::CUT_1)
                    cutEntity = i;
            }
            for (int i = entitiesBaseCount; i < entitiesCount; i++) {
                entities[i].type = Entity::NONE;
                entities[i].controller = NULL;
            }

            if (version == VER_TR1_PC) {
                stream.seek(32 * 256);
            // palette for release levels
                if (!demo) 
                    stream.read(palette, 256);
            // cinematic frames for cameras (PC)
                stream.read(cameraFrames,   stream.read(cameraFramesCount));
            // demo data
                stream.read(demoData,       stream.read(demoDataSize));
            }

        // sounds
            stream.read(soundsMap,      256);
            stream.read(soundsInfo,     stream.read(soundsInfoCount));
            if (version == VER_TR1_PC) {
                stream.read(soundData,      stream.read(soundDataSize));
                stream.read(soundOffsets,   stream.read(soundOffsetsCount));
            }
        // cinematic frames for cameras (PSX)
            if (version == VER_TR1_PSX) {
                stream.seek(4); // unknown 4 bytes always == 6
                stream.read(cameraFrames, stream.read(cameraFramesCount));
            }

            initTiles(tiles4, tiles8, palette, cluts);

            //delete[] tiles4;   tiles4 = NULL;
            delete[] tiles8;   tiles8 = NULL;

        // init secrets states
            memset(secrets, 0, MAX_SECRETS_COUNT * sizeof(secrets[0]));
        // get special models indices
            memset(&extra, 0, sizeof(extra));
            for (int i = 0; i < 4; i++)
                extra.weapons[i] = -1;
            extra.braid = -1;

            for (int i = 0; i < modelsCount; i++)
                switch (models[i].type) {
                    case Entity::MUZZLE_FLASH    : extra.muzzleFlash = i; break;
                    case Entity::HOLE_PUZZLE_SET : extra.puzzleSet   = i; break;
                    case Entity::LARA_PISTOLS    : extra.weapons[0]  = i; break;
                    case Entity::LARA_SHOTGUN    : extra.weapons[1]  = i; break;
                    case Entity::LARA_MAGNUMS    : extra.weapons[2]  = i; break;
                    case Entity::LARA_UZIS       : extra.weapons[3]  = i; break;
                    case Entity::BRAID           : extra.braid       = i; break;
                    default : ;
                }
        // init cutscene transform
            cutMatrix.identity();
            if (cutEntity > -1) {
                Entity &e = entities[cutEntity];
                switch (cameraFramesCount) { // HACK to detect cutscene level number
                    case 1600 : // CUT1
                        cutMatrix.translate(vec3(36668, float(e.y), 63180));
                        cutMatrix.rotateY(-23312.0f / float(0x4000) * PI * 0.5f);
                        break;
                    case 1000 : // CUT2
                        cutMatrix.translate(vec3(51962, float(e.y), 53760));
                        cutMatrix.rotateY(16380.0f / float(0x4000) * PI * 0.5f);
                        break;
                    case 400  : // CUT3
                    case 1890 : // CUT4
                        cutMatrix.translate(vec3(float(e.x), float(e.y), float(e.z)));
                        cutMatrix.rotateY(PI * 0.5f);
                        break;
                }
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
            delete[] cluts;
            delete[] tiles4;
            delete[] cameraFrames;
            delete[] demoData;
            delete[] soundsMap;
            delete[] soundsInfo;
            delete[] soundData;
            delete[] soundOffsets;
            delete[] soundSize;
        }

        void readMeshes(Stream &stream) {
            uint32 meshDataSize;
            stream.read(meshDataSize);
            int32 start = stream.pos;
            int32 end   = stream.pos + meshDataSize * 2;
            meshesCount = 0;
            while (stream.pos < end) {
                Mesh &mesh = meshes[meshesCount++];
                mesh.offset = stream.pos - start;

                stream.read(mesh.center);
                stream.read(mesh.radius);
                stream.read(mesh.flags);
                stream.read(mesh.vCount);

                switch (version) {
                    case VER_TR1_PC : {
                    /*  struct {
                            short3      center;
                            short2      collider;
                            short       vCount;
                            short3      vertices[vCount];
                            short       nCount;
                            short3      normals[max(0, nCount)];
                            ushort      luminance[-min(0, nCount)];
                            short       rCount;
                            Rectangle   rectangles[rCount];
                            short       tCount;
                            Triangle    triangles[tCount];
                            short       crCount;
                            Rectangle   crectangles[crCount];
                            short       ctCount;
                            Triangle    ctriangles[ctCount];
                        }; */
                        mesh.vertices = new Mesh::Vertex[mesh.vCount];
                        for (int i = 0; i < mesh.vCount; i++) {
                            short4 &c = mesh.vertices[i].coord;
                            stream.read(c.x);
                            stream.read(c.y);
                            stream.read(c.z);
                        }
                        int16 nCount;
                        stream.read(nCount);
                        ASSERT(mesh.vCount == abs(nCount));
                        for (int i = 0; i < mesh.vCount; i++) {
                            short4 &c = mesh.vertices[i].coord;
                            short4 &n = mesh.vertices[i].normal;
                            if (nCount > 0) { // normal
                                stream.read(n.x);
                                stream.read(n.y);
                                stream.read(n.z);
                                n.w = 1;
                                c.w = 0x1FFF;
                            } else { // intensity
                                stream.read(c.w);
                                n = { 0, 0, 0, 0 };
                            }
                        }

                        uint16 rCount, crCount, tCount, ctCount;

                        int tmp = stream.pos;
                        stream.seek(stream.read(rCount)  * sizeof(Rectangle));
                        stream.seek(stream.read(tCount)  * sizeof(Triangle));
                        stream.seek(stream.read(crCount) * sizeof(Rectangle));
                        stream.seek(stream.read(ctCount) * sizeof(Triangle));
                        stream.setPos(tmp);

                        mesh.rCount = rCount + crCount;
                        mesh.tCount = tCount + ctCount;

                        mesh.rectangles = mesh.rCount ? new Rectangle[mesh.rCount] : NULL;
                        mesh.triangles  = mesh.tCount ? new Triangle[mesh.tCount]  : NULL;

                        stream.seek(sizeof(uint16)); stream.raw(&mesh.rectangles[0],      rCount  * sizeof(Rectangle));
                        stream.seek(sizeof(uint16)); stream.raw(&mesh.triangles[0],       tCount  * sizeof(Triangle));
                        stream.seek(sizeof(uint16)); stream.raw(&mesh.rectangles[rCount], crCount * sizeof(Rectangle));
                        stream.seek(sizeof(uint16)); stream.raw(&mesh.triangles[tCount],  ctCount * sizeof(Triangle));
                    // add "use palette color" flags
                        for (int i = rCount; i < mesh.rCount; i++) mesh.rectangles[i].texture |= 0x8000;
                        for (int i = tCount; i < mesh.tCount; i++) mesh.triangles[i].texture  |= 0x8000;
                        break;
                    }
                    case VER_TR1_PSX : {
                    /*  struct {
                            short3      center;
                            short2      collider;
                            short       vCount;
                            short4      vertices[abs(vCount)];
                            short4      normals[max(0, vCount)];
                            ushort      luminance[-min(0, vCount)];
                            short       rCount;
                            Rectangle   rectangles[rCount];
                            short       tCount;
                            Triangle    triangles[tCount];
                        }; */
                        int nCount = mesh.vCount;
                        mesh.vCount = abs(mesh.vCount);
                        mesh.vertices = new Mesh::Vertex[mesh.vCount];

                        for (int i = 0; i < mesh.vCount; i++)
                            stream.read(mesh.vertices[i].coord);

                        for (int i = 0; i < mesh.vCount; i++) {
                            short4 &c = mesh.vertices[i].coord;
                            short4 &n = mesh.vertices[i].normal;
                            if (nCount > 0) { // normal
                                stream.read(n);
                                n.w = 1;
                                c.w = 0x1FFF;
                            } else { // intensity
                                stream.read(c.w);
                                n = { 0, 0, 0, 0 };
                            }
                        }

                        stream.read(mesh.rectangles, stream.read(mesh.rCount));
                        stream.read(mesh.triangles,  stream.read(mesh.tCount));   
                        for (int i = 0; i < mesh.rCount; i++) if (mesh.rectangles[i].texture < 300) mesh.rectangles[i].texture |= 0x8000;
                        for (int i = 0; i < mesh.tCount; i++) if (mesh.triangles[i].texture  < 300) mesh.triangles[i].texture  |= 0x8000;
                        break;
                    }
                }

                #define RECALC_ZERO_NORMALS(mesh, face, count)\
                    int fn = -1;\
                    for (int j = 0; j < count; j++) {\
                        Mesh::Vertex &v = mesh.vertices[face.vertices[j]];\
                        short4 &n = v.normal;\
                        if (!(n.x | n.y | n.z)) {\
                            if (fn > -1) {\
                                n = mesh.vertices[face.vertices[fn]].normal;\
                                continue;\
                            }\
                            vec3 o(mesh.vertices[face.vertices[0]].coord);\
                            vec3 a = o - mesh.vertices[face.vertices[1]].coord;\
                            vec3 b = o - mesh.vertices[face.vertices[2]].coord;\
                            o = b.cross(a).normal() * 16300.0f;\
                            n.x = (int)o.x;\
                            n.y = (int)o.y;\
                            n.z = (int)o.z;\
                            fn = j;\
                        }\
                    }

            // recalc zero normals
                for (int i = 0; i < mesh.rCount; i++) {
                    Rectangle &f = mesh.rectangles[i];
                    RECALC_ZERO_NORMALS(mesh, f, 4);
                }

                for (int i = 0; i < mesh.tCount; i++) {
                    Triangle &f = mesh.triangles[i];
                    RECALC_ZERO_NORMALS(mesh, f, 3);
                }

                #undef RECALC_ZERO_NORMALS

                int32 align = (stream.pos - start - mesh.offset) % 4;
                if (align) stream.seek(4 - align);
            }
            ASSERT(stream.pos - start == meshDataSize * 2);

            stream.read(meshOffsets, stream.read(meshOffsetsCount));

        // remap offsets to mesh index
            for (int i = 0; i < meshOffsetsCount; i++) {
                int index = -1;
//                if (!meshOffsets[i] && i)
//                    index = -1;
//                else
                for (int j = 0; j < meshesCount; j++)
                    if (meshes[j].offset == meshOffsets[i]) {                       
                        index = j;
                        break;
                    }
                meshOffsets[i] = index;
            }
        }

        void readObjectTex(Stream &stream) {
            #define SET_PARAMS(t, d, c) {\
                    t.clut        = c;\
                    t.tile        = d.tile;\
                    t.attribute   = d.attribute;\
                    t.texCoord[0] = { d.x0, d.y0 };\
                    t.texCoord[1] = { d.x1, d.y1 };\
                    t.texCoord[2] = { d.x2, d.y2 };\
                    t.texCoord[3] = { d.x3, d.y3 };\
                }

            stream.read(objectTexturesCount);
            objectTextures = new ObjectTexture[objectTexturesCount];
            for (int i = 0; i < objectTexturesCount; i++) {
                ObjectTexture &t = objectTextures[i];
                switch (version) {
                    case VER_TR1_PC : {                   
                        struct {
                            uint16  attribute;
                            Tile    tile;       
                            uint8   xh0, x0, yh0, y0;
                            uint8   xh1, x1, yh1, y1;
                            uint8   xh2, x2, yh2, y2;
                            uint8   xh3, x3, yh3, y3;
                        } d;
                        stream.read(d);
                        SET_PARAMS(t, d, 0);
                        break;
                    }
                    case VER_TR1_PSX : {
                        struct {
                            uint8   x0, y0;
                            uint16  clut;
                            uint8   x1, y1;
                            Tile    tile;       
                            uint8   x2, y2;
                            uint16  unknown;
                            uint8   x3, y3;
                            uint16  attribute; 
                        } d;
                        stream.read(d);
                        SET_PARAMS(t, d, d.clut);
                        break;
                    }
                }
            }

            #undef SET_PARAMS
        }

        void readSpriteTex(Stream &stream) {
            #define SET_PARAMS(t, d, c) {\
                    t.clut = c;\
                    t.tile = d.tile;\
                    t.l    = d.l;\
                    t.t    = d.t;\
                    t.r    = d.r;\
                    t.b    = d.b;\
                }

            stream.read(spriteTexturesCount);
            spriteTextures = new SpriteTexture[spriteTexturesCount];
            for (int i = 0; i < spriteTexturesCount; i++) {
                SpriteTexture &t = spriteTextures[i];
                switch (version) {
                    case VER_TR1_PC : {                        
                        struct {
                            uint16  tile;
                            uint8   u, v;
                            uint16  w, h;
                            int16   l, t, r, b;
                        } d;
                        stream.read(d);
                        SET_PARAMS(t, d, 0);
                        t.texCoord[0] = { d.u,                       d.v                       };
                        t.texCoord[1] = { (uint8)(d.u + (d.w >> 8)), (uint8)(d.v + (d.h >> 8)) };
                        break;
                    }
                    case VER_TR1_PSX : {
                        struct {
                            int16   l, t, r, b;
                            uint16  clut;
                            uint16  tile;
                            uint8   u0, v0;
                            uint8   u1, v1;
                        } d;
                        stream.read(d);
                        SET_PARAMS(t, d, d.clut);
                        t.texCoord[0] = { d.u0, d.v0 };
                        t.texCoord[1] = { d.u1, d.v1 };
                        break;
                    }
                }
            }

            #undef SET_PARAMS

            stream.read(spriteSequences, stream.read(spriteSequencesCount));
            for (int i = 0; i < spriteSequencesCount; i++)
                spriteSequences[i].sCount = -spriteSequences[i].sCount;
        }

        void initTiles(Tile4 *tiles4, Tile8 *tiles8, Color24 *palette, CLUT *cluts) {
            tiles = new Tile32[tilesCount];

            switch (version) {
                case VER_TR1_PC : {
                    ASSERT(tiles8);
                    ASSERT(palette);

                    for (int j = 0; j < 256; j++) { // Amiga -> PC color palette
                        Color24 &c = palette[j];
                        c.r <<= 2;
                        c.g <<= 2;
                        c.b <<= 2;
                    }

                    for (int i = 0; i < tilesCount; i++) {
                        Color32 *ptr = &tiles[i].color[0];
                        for (int y = 0; y < 256; y++) {
                            for (int x = 0; x < 256; x++) {
                                uint8 index = tiles8[i].index[y * 256 + x];
                                Color24 &p = palette[index];
                                ptr[x].r = p.r;
                                ptr[x].g = p.g;
                                ptr[x].b = p.b;
                                ptr[x].a = index == 0 ? 0 : 255;
                            }
                            ptr += 256;
                        }
                    }
                    break;
                }
                case VER_TR1_PSX : {
                    ASSERT(tiles4);
                    ASSERT(cluts);

                    for (int i = 0; i < objectTexturesCount; i++) {
                        ObjectTexture &t = objectTextures[i];
                        CLUT   &clut = cluts[t.clut];
                        Tile32 &dst  = tiles[t.tile.index];
                        Tile4  &src  = tiles4[t.tile.index];

                        int minX = min(min(t.texCoord[0].x, t.texCoord[1].x), t.texCoord[2].x);
                        int maxX = max(max(t.texCoord[0].x, t.texCoord[1].x), t.texCoord[2].x);
                        int minY = min(min(t.texCoord[0].y, t.texCoord[1].y), t.texCoord[2].y);
                        int maxY = max(max(t.texCoord[0].y, t.texCoord[1].y), t.texCoord[2].y);

                        for (int y = minY; y <= maxY; y++)
                            for (int x = minX; x <= maxX; x++)                          
                                dst.color[y * 256 + x] = clut.color[(x % 2) ? src.index[(y * 256 + x) / 2].b : src.index[(y * 256 + x) / 2].a];
                    }

                    for (int i = 0; i < spriteTexturesCount; i++) {
                        SpriteTexture &t = spriteTextures[i];
                        CLUT   &clut = cluts[t.clut];
                        Tile32 &dst  = tiles[t.tile];
                        Tile4  &src  = tiles4[t.tile];
                        
                        for (int y = t.texCoord[0].y; y < t.texCoord[1].y; y++)
                            for (int x = t.texCoord[0].x; x < t.texCoord[1].x; x += 2) {                           
                                dst.color[y * 256 + x + 0] = clut.color[src.index[(y * 256 + x) / 2].a];
                                dst.color[y * 256 + x + 1] = clut.color[src.index[(y * 256 + x) / 2].b];
                            }
                    }

                    break;
                }
            }
        }

    // common methods
        Color24 getColor(int texture) const {
            switch (version) {
                case VER_TR1_PC  : return palette[texture & 0xFF];
                case VER_TR1_PSX : {
                    ObjectTexture &t = objectTextures[texture & 0x7FFF];
                    int idx  = (t.texCoord[0].y * 256 + t.texCoord[0].x) / 2;
                    int part = t.texCoord[0].x % 2;
                    Tile4 &tile = tiles4[t.tile.index];
                    CLUT  &clut = cluts[t.clut];
                    return clut.color[part ? tile.index[idx].b : tile.index[idx].a];
                }
            }
            return Color24(255, 0, 255);
        }

        Stream* getSampleStream(int index) {
            uint8 *data = &soundData[soundOffsets[index]];
            uint32 size = 0;
            switch (version) {
                case VER_TR1_PC  : size = ((uint32*)data)[1] + 8; break; // read size from wave header
                case VER_TR1_PSX : size = soundSize[index]; break;
            }
            return new Stream(data, size);
        }

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

            sx = clamp(sx, 0, room.xSectors * 1024 - 1);
            sz = clamp(sz, 0, room.zSectors * 1024 - 1);

            dx = sx % 1024;
            dz = sz % 1024;
            sx /= 1024;
            sz /= 1024;

            return room.sectors[sx * room.zSectors + sz];
        }

        void getFloorInfo(int roomIndex, int x, int y, int z, FloorInfo &info) const {
            int dx, dz;
            Room::Sector &s = getSector(roomIndex, x, z, dx, dz);

            info.roomFloor    = 256 * s.floor;
            info.roomCeiling  = 256 * s.ceiling;
            info.floor        = info.roomFloor;
            info.ceiling      = info.roomCeiling;
            info.slantX       = 0;
            info.slantZ       = 0;
            info.roomNext     = 0xFF;
            info.roomBelow    = s.roomBelow;
            info.roomAbove    = s.roomAbove;
            info.floorIndex   = s.floorIndex;
            info.kill         = 0;
            info.trigger      = Trigger::ACTIVATE;
            info.trigCmdCount = 0;

            if (s.floor == -127) 
                return;

            Room::Sector *sBelow = &s;
            while (sBelow->roomBelow != TR::NO_ROOM) sBelow = &getSector(sBelow->roomBelow, x, z, dx, dz);
            info.floor = 256 * sBelow->floor;
            parseFloorData(info, sBelow->floorIndex, dx, dz);

            if (info.roomNext == TR::NO_ROOM) {
                Room::Sector *sAbove = &s;
                while (sAbove->roomAbove != TR::NO_ROOM) sAbove = &getSector(sAbove->roomAbove, x, z, dx, dz);
                if (sAbove != sBelow) {
                    info.ceiling = 256 * sAbove->ceiling;
                    parseFloorData(info, sAbove->floorIndex, dx, dz);
                }
            } else {
                int tmp = info.roomNext;
                getFloorInfo(tmp, x, y, z, info);
                info.roomNext = tmp;
            }

        // entities collide
            if (info.trigCmdCount) {
                int sx = x / 1024;
                int sz = z / 1024;
                int dx = x % 1024;
                int dz = z % 1024;

                for (int i = 0; i < info.trigCmdCount; i++) {                    
                    FloorData::TriggerCommand cmd = info.trigCmd[i];
                    if (cmd.action != Action::ACTIVATE) continue;
                    
                    Entity &e = entities[cmd.args];
                    if (!e.flags.collision) continue;

                    if (sx != e.x / 1024 || sz != e.z / 1024) continue;

                    switch (e.type) {
                        case Entity::TRAP_DOOR_1 :
                        case Entity::TRAP_DOOR_2 :
                        case Entity::TRAP_FLOOR  : {
                            int ey = e.y - (e.type == Entity::TRAP_FLOOR ? 512 : 0);
                            if (ey >= y - 128 && ey < info.floor)
                                info.floor = ey;
                            if (ey  < y - 128 && ey > info.ceiling)
                                info.ceiling = ey + (e.type == Entity::TRAP_FLOOR ? 0 : 256);
                            break;
                        }
                        case Entity::BRIDGE_0    : 
                        case Entity::BRIDGE_1    : 
                        case Entity::BRIDGE_2    : {
                            int s = (e.type == Entity::BRIDGE_1) ? 1 :
                                    (e.type == Entity::BRIDGE_2) ? 2 : 0;

                            int ey = e.y, sx = 0, sz = 0; 

                            if (s > 0) {
                                switch (e.rotation.value / 0x4000) { // get slantXZ by direction
                                    case 0 : sx =  s; break;
                                    case 1 : sz = -s; break;
                                    case 2 : sx = -s; break;
                                    case 3 : sz =  s; break;
                                }

                                ey -= sx * (sx > 0 ? (dx - 1024) : dx) >> 2;
                                ey -= sz * (sz > 0 ? (dz - 1024) : dz) >> 2;
                            }

                            if (y - 128 <= ey) {
                                info.floor  = ey;
                                info.slantX = sx;
                                info.slantZ = sz;
                            }
                            if (ey  < y - 128)
                                info.ceiling = ey + 64;
                            break;
                        }

                        default : ;
                    }
                }
            }
        }

        void parseFloorData(FloorInfo &info, int floorIndex, int dx, int dz) const {
            if (!floorIndex) return;

            FloorData *fd = &floors[floorIndex];
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
                            ASSERT(info.trigCmdCount < MAX_TRIGGER_COMMANDS);
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

        }


    }; // struct Level

    bool castShadow(Entity::Type type) {
        return (type >= Entity::ENEMY_TWIN && type <= Entity::ENEMY_LARSON) || type == Entity::LARA || (type >= Entity::CUT_1 && type <= Entity::CUT_3);
    }
}

#endif