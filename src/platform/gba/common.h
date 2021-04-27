#ifndef H_COMMON
#define H_COMMON

//#define TEST
//#define PROFILE
#ifdef PROFILE
    #define STATIC_ITEMS
#endif


#if defined(_WIN32)
    #define MODE4
    //#define MODE5
    //#define MODE13
#elif defined(__GBA__)
    #define MODE4
    //#define MODE5

    //#define USE_9BIT_SOUND
#elif defined(__TNS__)
    #define MODE13
#elif defined(__DOS__)
    #define MODE13
#else
    #error unsupported platform
#endif

#if defined(MODE5)
    #define ROTATE90_MODE
    //#define DEBUG_OVERDRAW
#endif

#define NO_STATIC_MESHES

#if defined(_WIN32)
    #define _CRT_SECURE_NO_WARNINGS
    #include <windows.h>
#elif defined(__GBA__)
    #include <tonc.h>
#elif defined(__TNS__)
    #include <os.h>
#elif defined(__DOS__)
    #include <stdlib.h>
    #include <stddef.h>
    #include <conio.h>
    #include <dos.h>
    #include <mem.h>
#endif

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <new>

//#define DEBUG_FACES

#if defined(MODE4)
    #define MODE_PAL
    #define VRAM_WIDTH   120 // in shorts (240 bytes)
    #define FRAME_WIDTH  240
    #define FRAME_HEIGHT 160
#elif defined(MODE5)
    #define VRAM_WIDTH   160

    #ifdef ROTATE90_MODE
        #define FRAME_WIDTH  160
        #define FRAME_HEIGHT 120
    #else
        #define FRAME_WIDTH  120
        #define FRAME_HEIGHT 80
    #endif
#elif defined(MODE13)
    #define MODE_PAL
    #define VRAM_WIDTH   160 // in shorts (320 bytes)
    #define FRAME_WIDTH  320

    #if defined(__TNS__)
        #define FRAME_HEIGHT 240 // MODE X?
    #else
        #define FRAME_HEIGHT 200
    #endif
#endif

#if defined(_MSC_VER)
    #define X_INLINE    inline
    #define X_NOINLINE  __declspec(noinline)
    #define ALIGN4      __declspec(align(4))
    #define ALIGN16     __declspec(align(16))
#elif defined(__WATCOMC__)
    #define X_INLINE    inline
    #define X_NOINLINE  __declspec(noinline)
    #define ALIGN4
    #define ALIGN16
#else
    #define X_INLINE    __attribute__((always_inline)) inline
    #define X_NOINLINE  __attribute__((noinline))
    #define ALIGN4      __attribute__((aligned(4)))
    #define ALIGN16     __attribute__((aligned(16)))
#endif

#define UNUSED(x) (void)(x)

typedef signed char        int8;
typedef signed short       int16;
typedef signed int         int32;
typedef signed long long   int64;
typedef unsigned char      uint8;
typedef unsigned short     uint16;
typedef unsigned int       uint32;
typedef unsigned long long uint64;
typedef int16              Index;

#ifndef __GBA__
    #define ARM_CODE
    #define THUMB_CODE
    #define IWRAM_DATA
    #define EWRAM_DATA
    #define EWRAM_BSS
    #define IWRAM_CODE
    #define EWRAM_CODE

    #define dmaCopy(src,dst,size) memcpy(dst,src,size)
#endif

#if defined(_WIN32)
    #define ASSERT(x) { if (!(x)) { DebugBreak(); } }
#else
    #define ASSERT(x)
#endif

#if defined(__GBA__)
    #define IME_DISABLE() u16 origIME = REG_IME; REG_IME = 0
    #define IME_ENABLE()  REG_IME = origIME;
#else
    #define IME_DISABLE()
    #define IME_ENABLE()
#endif

#if defined(_WIN32)
    extern uint16 fb[VRAM_WIDTH * FRAME_HEIGHT];
    #ifdef MODE_PAL
        extern uint16 MEM_PAL_BG[256];
    #endif
#elif defined(__GBA__)
    extern uint32 fb;
#elif defined(__TNS__)
    extern uint16 fb[VRAM_WIDTH * FRAME_HEIGHT];
#elif defined(__DOS__)
    extern uint16 fb[VRAM_WIDTH * FRAME_HEIGHT];
#endif

#ifdef PROFILE
    #if defined(_WIN32)

        extern LARGE_INTEGER g_timer;
        extern LARGE_INTEGER g_current;

        #define PROFILE_START() {\
            QueryPerformanceCounter(&g_timer);\
        }

        #define PROFILE_STOP(value) {\
            QueryPerformanceCounter(&g_current);\
            value += (g_current.QuadPart - g_timer.QuadPart);\
        }

    #elif defined(__GBA__)
        #ifdef TEST
            #define TIMER_FREQ_DIV 1
        #else
            #define TIMER_FREQ_DIV 3
        #endif
    
        #define PROFILE_START() {\
            REG_TM2CNT_L = 0;\
            REG_TM2CNT_H = (1 << 7) | TIMER_FREQ_DIV;\
        }

        #define PROFILE_STOP(value) {\
            value += REG_TM2CNT_L;\
            REG_TM2CNT_H = 0;\
        }

    #else
        #define PROFILE_START()
        #define PROFILE_STOP(value)
    #endif
#else
    #define PROFILE_START()
    #define PROFILE_STOP(value)
#endif

#ifdef __TNS__
    void paletteSet(unsigned short* palette);
#endif

// system keys (keys)
enum InputKey {
    IK_NONE     = 0,
    IK_UP       = (1 << 0),
    IK_RIGHT    = (1 << 1),
    IK_DOWN     = (1 << 2),
    IK_LEFT     = (1 << 3),
    IK_A        = (1 << 4),
    IK_B        = (1 << 5),
    IK_L        = (1 << 6),
    IK_R        = (1 << 7),
    IK_START    = (1 << 8),
    IK_SELECT   = (1 << 9),
};

// action keys (Item::input)
enum {
    IN_LEFT   = (1 << 1),
    IN_RIGHT  = (1 << 2),
    IN_UP     = (1 << 3),
    IN_DOWN   = (1 << 4),
    IN_JUMP   = (1 << 5),
    IN_WALK   = (1 << 6),
    IN_ACTION = (1 << 7),
    IN_WEAPON = (1 << 8),
    IN_LOOK   = (1 << 9),
};


struct vec3s {
    int16 x, y, z;

    vec3s() {}
    X_INLINE vec3s(int16 x, int16 y, int16 z) : x(x), y(y), z(z) {}
};

struct vec3i {
    int32 x, y, z;

    vec3i() {};
    X_INLINE vec3i(int32 s) : x(s), y(s), z(s) {}
    X_INLINE vec3i(int32 x, int32 y, int32 z) : x(x), y(y), z(z) {}
    X_INLINE vec3i(vec3s &v) : x(v.x), y(v.y), z(v.z) {}
    X_INLINE vec3i  operator + (const vec3i &v) const { return vec3i(x + v.x, y + v.y, z + v.z); }
    X_INLINE vec3i  operator - (const vec3i &v) const { return vec3i(x - v.x, y - v.y, z - v.z); }
    X_INLINE vec3i& operator += (const vec3i &v) { x += v.x; y += v.y; z += v.z; return *this; }
    X_INLINE vec3i& operator -= (const vec3i &v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
};

struct vec4i {
    int32 x, y, z, w;

    X_INLINE int32& operator [] (int32 index) const {
        ASSERT(index >= 0 && index <= 3);
        return ((int32*)this)[index];
    }
};

typedef vec4i Matrix[3];

struct Quad {
    Index  indices[4];
    uint16 flags;
};

struct Triangle {
    Index  indices[3];
    uint16 flags;
};

struct Rect {
    int32 x0;
    int32 y0;
    int32 x1;
    int32 y1;

    Rect() {}
    Rect(int32 x0, int32 y0, int32 x1, int32 y1) : x0(x0), y0(y0), x1(x1), y1(y1) {}
};

struct Vertex { // 8
    int16 x, y, z;
    uint8 g, clip;
};

union UV {
    struct { uint16 v, u; };
    uint32 uv;
};

struct VertexUV {
    Vertex v;
    UV     t;
    VertexUV *prev;
    VertexUV *next;
};

struct Face { // 12
    Face*  next;
    uint16 flags;
    int16  start;
    int8   indices[4];
};

struct Box {
    int16 minX;
    int16 maxX;
    int16 minY;
    int16 maxY;
    int16 minZ;
    int16 maxZ;

    X_INLINE Box() {}
    X_INLINE Box(int16 minX, int16 maxX, int16 minY, int16 maxY, int16 minZ, int16 maxZ) :
        minX(minX), maxX(maxX), minY(minY), maxY(maxY), minZ(minZ), maxZ(maxZ) {}
};

struct AABB {
    int32 minX;
    int32 maxX;
    int32 minY;
    int32 maxY;
    int32 minZ;
    int32 maxZ;
};

struct Room;

struct RoomInfo {
    struct Vertex {
        vec3s  pos;
        uint16 lighting;
    };

    struct Sprite {
        Index  index;
        uint16 texture;
    };
    
    struct Portal {
        uint16 roomIndex;
        vec3s  n;
        vec3s  v[4];
    };

    struct Sector {
        uint16 floorIndex;
        uint16 boxIndex;
        uint8  roomBelow;
        int8   floor;
        uint8  roomAbove;
        int8   ceiling;

        const Sector* getSectorBelow(int32 posX, int32 posZ) const;
        const Sector* getSectorAbove(int32 posX, int32 posZ) const;
        int32 getFloor(int32 x, int32 y, int32 z) const;
        int32 getCeiling(int32 x, int32 y, int32 z) const;
        Room* getNextRoom() const;
        void getTriggerFloorCeiling(int32 x, int32 y, int32 z, int32* floor, int32* ceiling) const;
    };

    struct Light { // TODO align struct
        uint8 pos[12];
        int16 intensity;
        uint8 radius[4];
    };

    struct Mesh {
        int16  pos[6]; // TODO align struct (int32 x, y, z)
        int16  rotation;
        uint16 intensity;
        uint16 staticMeshId;
    };

    int32 x;
    int32 z;
    int32 yBottom;
    int32 yTop;

    uint32 dataSize;
/*
    uint16    vCount;
    Vertex*   vertices;

    uint16    qCount;
    Quad*     quads;
    
    uint16    tCount;
    Triangle* triangles;
    
    uint16    sCount;
    Sprite*   sprites;
*/
};

struct Item;

struct Room {
    Item*               firstItem;

    Rect                clip;
    uint8               _reserved;
    bool                visible;

    // TODO leave in ROM
    int32               x, z;
    uint16              vCount;
    uint16              qCount;
    uint16              tCount;
    uint16              pCount;
    uint16              lCount;
    uint16              mCount;
    uint16              zSectors;
    uint16              xSectors;
    uint16              ambient;

    const RoomInfo::Vertex* vertices;
    const Quad*             quads;
    const Triangle*         triangles;
    const RoomInfo::Portal* portals;
    const RoomInfo::Light*  lights;
    const RoomInfo::Mesh*   meshes;
    const RoomInfo::Sector* sectors; // == sectorsOrig (const) by default (see roomModify) 
    const RoomInfo::Sector* sectorsOrig;

    void modify();
    void reset();

    void add(Item* item);
    void remove(Item* item);

    const RoomInfo::Sector* getSector(int32 posX, int32 posZ) const;
    Room* getRoom(int32 x, int32 y, int32 z);
    bool checkPortal(const RoomInfo::Portal* portal);
    Room** addVisibleRoom(Room** list);
    Room** addNearRoom(Room** list, int32 x, int32 y, int32 z);
    Room** getNearRooms(const vec3i &pos, int32 radius, int32 height);
    Room** getAdjRooms();
    Room** getVisibleRooms();

    int32 getWaterLevel();
    int32 getWaterDepth();
};

struct Node {
    uint32 flags;
    vec3i  pos;
};

struct Model {
    uint32 type;
    uint16 mCount;
    uint16 mStart;
    uint32 nodeIndex;
    uint32 frameIndex;
    uint16 animIndex;
    uint16 _padding;
};

#define FILE_MODEL_SIZE (sizeof(Model) - 2) // -padding

struct StaticMesh {
    int32   id;
    uint16  meshIndex;
    Box     vbox;
    Box     cbox;
    uint16  flags;
};

struct SoundInfo
{
    uint16 index;
    uint16 volume;
    uint16 chance;
    union {
        struct { uint16 mode:2, count:4, unused:6, camera:1, pitch:1, gain:1, :1; };
        uint16 value;
    } flags;
};

struct Anim {
    uint32  frameOffset;

    uint8   frameRate;
    uint8   frameSize;
    uint16  state;

    int32   speed;

    int32   accel;

    uint16  frameBegin;
    uint16  frameEnd;

    uint16  nextAnimIndex;
    uint16  nextFrameIndex;

    uint16  statesCount;
    uint16  statesStart;

    uint16  commandsCount;
    uint16  commandsStart;
};

struct AnimState {
    uint16  state;
    uint16  rangesCount;
    uint16  rangesStart;
};

struct AnimRange {
    uint16  frameBegin;
    uint16  frameEnd;
    uint16  nextAnimIndex;
    uint16  nextFrameIndex;
};

struct AnimFrame {
    Box    box;
    vec3s  pos;
    uint16 angles[1];
};

struct Texture {
    uint16  attribute;
    uint16  tile:14, :2;
    uint32  uv0;
    uint32  uv1;
    uint32  uv2;
    uint32  uv3;
};

struct Sprite {
    uint16  tile;
    uint8   u, v;
    uint16  w, h;
    int16   l, t, r, b;
};

struct SpriteSeq {
    uint16  type;
    uint16  unused;
    int16   sCount;
    int16   sStart;
};

struct ItemInfo { // 24
    uint16 type;
    int16  startRoomIndex;
    vec3i  pos;
    int16  angleY;
    uint16 intensity;

    union {
        struct {
            uint16 save:1, gravity:1, active:1, status:2, collision:1, custom:2, once:1, mask:5, reverse:1, shadow:1; // TODO
        };
        uint16 value;
    } flags;

    uint16 input; // used by Lara only
};

#define ITEM_FLAGS_MASK_ALL         0x1F

#define ITEM_FLAGS_STATUS_NONE      0
#define ITEM_FLAGS_STATUS_ACTIVE    1
#define ITEM_FLAGS_STATUS_INACTIVE  2
#define ITEM_FLAGS_STATUS_INVISIBLE 3

#define FILE_ITEM_SIZE (sizeof(ItemInfo) - 2)

struct CollisionInfo;
struct Lara;

#define ITEM_TYPES(E) \
    E( LARA                  ) \
    E( LARA_PISTOLS          ) \
    E( LARA_SHOTGUN          ) \
    E( LARA_MAGNUMS          ) \
    E( LARA_UZIS             ) \
    E( LARA_SPEC             ) \
    E( DOPPELGANGER          ) \
    E( WOLF                  ) \
    E( BEAR                  ) \
    E( BAT                   ) \
    E( CROCODILE_LAND        ) \
    E( CROCODILE_WATER       ) \
    E( LION_MALE             ) \
    E( LION_FEMALE           ) \
    E( PUMA                  ) \
    E( GORILLA               ) \
    E( RAT_LAND              ) \
    E( RAT_WATER             ) \
    E( REX                   ) \
    E( RAPTOR                ) \
    E( MUTANT_1              ) \
    E( MUTANT_2              ) \
    E( MUTANT_3              ) \
    E( CENTAUR               ) \
    E( MUMMY                 ) \
    E( UNUSED_1              ) \
    E( UNUSED_2              ) \
    E( LARSON                ) \
    E( PIERRE                ) \
    E( SKATEBOARD            ) \
    E( SKATER                ) \
    E( COWBOY                ) \
    E( MR_T                  ) \
    E( NATLA                 ) \
    E( GIANT_MUTANT          ) \
    E( TRAP_FLOOR            ) \
    E( TRAP_SWING_BLADE      ) \
    E( TRAP_SPIKES           ) \
    E( TRAP_BOULDER          ) \
    E( DART                  ) \
    E( TRAP_DART_EMITTER     ) \
    E( DRAWBRIDGE            ) \
    E( TRAP_SLAM             ) \
    E( TRAP_SWORD            ) \
    E( HAMMER_HANDLE         ) \
    E( HAMMER_BLOCK          ) \
    E( LIGHTNING             ) \
    E( MOVING_OBJECT         ) \
    E( BLOCK_1               ) \
    E( BLOCK_2               ) \
    E( BLOCK_3               ) \
    E( BLOCK_4               ) \
    E( MOVING_BLOCK          ) \
    E( TRAP_CEILING_1        ) \
    E( TRAP_CEILING_2        ) \
    E( SWITCH                ) \
    E( SWITCH_WATER          ) \
    E( DOOR_1                ) \
    E( DOOR_2                ) \
    E( DOOR_3                ) \
    E( DOOR_4                ) \
    E( DOOR_5                ) \
    E( DOOR_6                ) \
    E( DOOR_7                ) \
    E( DOOR_8                ) \
    E( TRAP_DOOR_1           ) \
    E( TRAP_DOOR_2           ) \
    E( UNUSED_3              ) \
    E( BRIDGE_FLAT           ) \
    E( BRIDGE_TILT1          ) \
    E( BRIDGE_TILT2          ) \
    E( INV_PASSPORT          ) \
    E( INV_COMPASS           ) \
    E( INV_HOME              ) \
    E( GEARS_1               ) \
    E( GEARS_2               ) \
    E( GEARS_3               ) \
    E( CUT_1                 ) \
    E( CUT_2                 ) \
    E( CUT_3                 ) \
    E( CUT_4                 ) \
    E( INV_PASSPORT_CLOSED   ) \
    E( INV_MAP               ) \
    E( CRYSTAL               ) \
    E( PISTOLS               ) \
    E( SHOTGUN               ) \
    E( MAGNUMS               ) \
    E( UZIS                  ) \
    E( AMMO_PISTOLS          ) \
    E( AMMO_SHOTGUN          ) \
    E( AMMO_MAGNUMS          ) \
    E( AMMO_UZIS             ) \
    E( EXPLOSIVE             ) \
    E( MEDIKIT_SMALL         ) \
    E( MEDIKIT_BIG           ) \
    E( INV_DETAIL            ) \
    E( INV_SOUND             ) \
    E( INV_CONTROLS          ) \
    E( INV_GAMMA             ) \
    E( INV_PISTOLS           ) \
    E( INV_SHOTGUN           ) \
    E( INV_MAGNUMS           ) \
    E( INV_UZIS              ) \
    E( INV_AMMO_PISTOLS      ) \
    E( INV_AMMO_SHOTGUN      ) \
    E( INV_AMMO_MAGNUMS      ) \
    E( INV_AMMO_UZIS         ) \
    E( INV_EXPLOSIVE         ) \
    E( INV_MEDIKIT_SMALL     ) \
    E( INV_MEDIKIT_BIG       ) \
    E( PUZZLE_1              ) \
    E( PUZZLE_2              ) \
    E( PUZZLE_3              ) \
    E( PUZZLE_4              ) \
    E( INV_PUZZLE_1          ) \
    E( INV_PUZZLE_2          ) \
    E( INV_PUZZLE_3          ) \
    E( INV_PUZZLE_4          ) \
    E( PUZZLE_HOLE_1         ) \
    E( PUZZLE_HOLE_2         ) \
    E( PUZZLE_HOLE_3         ) \
    E( PUZZLE_HOLE_4         ) \
    E( PUZZLE_DONE_1         ) \
    E( PUZZLE_DONE_2         ) \
    E( PUZZLE_DONE_3         ) \
    E( PUZZLE_DONE_4         ) \
    E( LEADBAR               ) \
    E( INV_LEADBAR           ) \
    E( MIDAS_HAND            ) \
    E( KEY_ITEM_1            ) \
    E( KEY_ITEM_2            ) \
    E( KEY_ITEM_3            ) \
    E( KEY_ITEM_4            ) \
    E( INV_KEY_ITEM_1        ) \
    E( INV_KEY_ITEM_2        ) \
    E( INV_KEY_ITEM_3        ) \
    E( INV_KEY_ITEM_4        ) \
    E( KEY_HOLE_1            ) \
    E( KEY_HOLE_2            ) \
    E( KEY_HOLE_3            ) \
    E( KEY_HOLE_4            ) \
    E( UNUSED_4              ) \
    E( UNUSED_5              ) \
    E( SCION_PICKUP_QUALOPEC ) \
    E( SCION_PICKUP_DROP     ) \
    E( SCION_TARGET          ) \
    E( SCION_PICKUP_HOLDER   ) \
    E( SCION_HOLDER          ) \
    E( UNUSED_6              ) \
    E( UNUSED_7              ) \
    E( INV_SCION             ) \
    E( EXPLOSION             ) \
    E( UNUSED_8              ) \
    E( WATER_SPLASH          ) \
    E( UNUSED_9              ) \
    E( BUBBLE                ) \
    E( UNUSED_10             ) \
    E( UNUSED_11             ) \
    E( BLOOD                 ) \
    E( UNUSED_12             ) \
    E( SMOKE                 ) \
    E( CENTAUR_STATUE        ) \
    E( CABIN                 ) \
    E( MUTANT_EGG_SMALL      ) \
    E( RICOCHET              ) \
    E( SPARKLES              ) \
    E( MUZZLE_FLASH          ) \
    E( UNUSED_13             ) \
    E( UNUSED_14             ) \
    E( VIEW_TARGET           ) \
    E( WATERFALL             ) \
    E( NATLA_BULLET          ) \
    E( MUTANT_BULLET         ) \
    E( CENTAUR_BULLET        ) \
    E( UNUSED_16             ) \
    E( UNUSED_17             ) \
    E( LAVA_PARTICLE         ) \
    E( TRAP_LAVA_EMITTER     ) \
    E( FLAME                 ) \
    E( TRAP_FLAME_EMITTER    ) \
    E( TRAP_LAVA             ) \
    E( MUTANT_EGG_BIG        ) \
    E( BOAT                  ) \
    E( EARTHQUAKE            ) \
    E( UNUSED_18             ) \
    E( UNUSED_19             ) \
    E( UNUSED_20             ) \
    E( UNUSED_21             ) \
    E( UNUSED_22             ) \
    E( LARA_BRAID            ) \
    E( GLYPHS                )

#define DECL_ENUM(v) ITEM_##v,

enum ItemType {
    ITEM_TYPES(DECL_ENUM)
    ITEM_MAX
};

#undef DECL_ENUM

struct Item : ItemInfo { // 24 + 28 = 52
    Room*  room;

    int16  angleX;
    int16  angleZ;

    int16  vSpeed;
    int16  hSpeed;

    uint16 animIndex;
    uint16 frameIndex;

    uint8  state;
    uint8  nextState;
    uint8  goalState;
    uint8  waterState;

    int16  health;
    union {
        int16 timer;
        int16 oxygen;
    };

    int16  moveAngle;
    int16  floor;

    int16  turnSpeed;
    int16  vSpeedHack;

    Item*  nextItem;
    Item*  nextActive;

    static Item* sFirstActive;
    static Item* sFirstFree;

    static Item* add(ItemType type, Room* room, const vec3i &pos, int32 angleY);
    void remove();
    void activate();
    void deactivate();

    AnimFrame* getFrame(const Model* model) const;
    const Box& getBoundingBox() const;
    void move();
    const Anim* animSet(int32 newAnimIndex, bool resetState, int32 frameOffset = 0);
    const Anim* animChange(const Anim* anim);
    void animCmd(bool fx, const Anim* anim);

    void skipAnim();
    void updateAnim(bool movement = true);
    void updateRoom(int32 offset = 0);
    int32 calcLighting(const Box& box) const;

    Item* init(Room* room);

    X_INLINE Item() {}
    Item(Room* room);
    virtual void update();
    virtual void draw();
    virtual void collide(Lara* lara, CollisionInfo* cinfo);
};

struct SaveGame
{
    uint8 secrets;
};

union FloorData { // 2 bytes
    uint16 value;

    union Command {
        struct {
            uint16 func:5, tri:3, type:7, end:1;
        };
        struct {
            int16 :5, a:5, b:5, :1;
        } triangle;
    } cmd;

    struct {
        int8 slantX;
        int8 slantZ;
    };

    struct {
        uint16 a:4, b:4, c:4, d:4;
    };

    struct TriggerInfo {
        uint16  timer:8, once:1, mask:5, :2;
    } triggerInfo;

    union TriggerCommand {
        struct {
            uint16 args:10, action:5, end:1;
        };
        struct {
            uint16 timer:8, once:1, speed:5, :2;
        };
    } triggerCmd;
};

enum FloorType {
    FLOOR_TYPE_NONE,
    FLOOR_TYPE_PORTAL,
    FLOOR_TYPE_FLOOR,
    FLOOR_TYPE_CEILING,
    FLOOR_TYPE_TRIGGER,
    FLOOR_TYPE_LAVA
};

enum TriggerType {
    TRIGGER_TYPE_ACTIVATE,
    TRIGGER_TYPE_PAD,
    TRIGGER_TYPE_SWITCH,
    TRIGGER_TYPE_KEY,
    TRIGGER_TYPE_PICKUP,
    TRIGGER_TYPE_OBJECT,
    TRIGGER_TYPE_ANTIPAD,
    TRIGGER_TYPE_COMBAT,
    TRIGGER_TYPE_DUMMY,
};

enum TriggerAction {
    TRIGGER_ACTION_ACTIVATE_OBJECT,
    TRIGGER_ACTION_ACTIVATE_CAMERA,
    TRIGGER_ACTION_FLOW,
    TRIGGER_ACTION_FLIP,
    TRIGGER_ACTION_FLIP_ON,
    TRIGGER_ACTION_FLIP_OFF,
    TRIGGER_ACTION_CAMERA_TARGET,
    TRIGGER_ACTION_END,
    TRIGGER_ACTION_SOUNDTRACK,
    TRIGGER_ACTION_EFFECT,
    TRIGGER_ACTION_SECRET,
    TRIGGER_ACTION_CLEAR_BODIES,
    TRIGGER_ACTION_FLYBY,
    TRIGGER_ACTION_CUTSCENE
};

enum SlantType {
    SLANT_NONE,
    SLANT_LOW,
    SLANT_HIGH
};

enum WaterState {
    WATER_STATE_ABOVE,
    WATER_STATE_SURFACE,
    WATER_STATE_UNDER,
    WATER_STATE_WADE,
};

enum AnimCommand {
    ANIM_CMD_NONE,
    ANIM_CMD_OFFSET,
    ANIM_CMD_JUMP,
    ANIM_CMD_EMPTY,
    ANIM_CMD_KILL,
    ANIM_CMD_SOUND,
    ANIM_CMD_EFFECT,
};

enum EffectType {
    FX_NONE           = -1,
    FX_ROTATE_180     ,
    FX_FLOOR_SHAKE    ,
    FX_LARA_NORMAL    ,
    FX_LARA_BUBBLES   ,
    FX_FINISH_LEVEL   ,
    FX_EARTHQUAKE     ,
    FX_FLOOD          ,
    FX_UNK1           ,
    FX_STAIRS2SLOPE   ,
    FX_UNK3           ,
    FX_UNK4           ,
    FX_EXPLOSION      ,
    FX_LARA_HANDSFREE ,
    FX_FLIP_MAP       ,
    FX_DRAW_RIGHTGUN  ,
    FX_DRAW_LEFTGUN   ,
    FX_SHOT_RIGHTGUN  ,
    FX_SHOT_LEFTGUN   ,
    FX_MESH_SWAP_1    ,
    FX_MESH_SWAP_2    ,
    FX_MESH_SWAP_3    ,
    FX_INV_ON         ,
    FX_INV_OFF        ,
    FX_DYN_ON         ,
    FX_DYN_OFF        ,
    FX_STATUE_FX      ,
    FX_RESET_HAIR     ,
    FX_BOILER_FX      ,
    FX_ASSAULT_RESET  ,
    FX_ASSAULT_STOP   ,
    FX_ASSAULT_START  ,
    FX_ASSAULT_FINISH ,
    FX_FOOTPRINT      ,
// specific
    FX_TR1_FLICKER    = 16,
};

enum {
    SND_NO              = 2,

    SND_LANDING         = 4,
        
    SND_UNHOLSTER       = 6,
    SND_HOLSTER         = 7,
    SND_PISTOLS_SHOT    = 8,
    SND_SHOTGUN_RELOAD  = 9,
    SND_RICOCHET        = 10,
        
    SND_HIT_BEAR        = 16,
    SND_HIT_WOLF        = 20,

    SND_SCREAM          = 30,
    SND_HIT             = 31,

    SND_WATER_SPLASH    = 33,
        
    SND_BUBBLE          = 37,
         
    SND_UZIS_SHOT       = 43,
    SND_MAGNUMS_SHOT    = 44,
    SND_SHOTGUN_SHOT    = 45,
    SND_EMPTY           = 48,
    SND_HIT_UNDERWATER  = 50,
        
    SND_UNDERWATER      = 60,

    SND_BOULDER         = 70,

    SND_FLOOD           = 81,

    SND_HIT_LION        = 85,

    SND_HIT_RAT         = 95,
        
    SND_LIGHTNING       = 98,
    SND_ROCK            = 99,

    SND_SWORD           = 103,
    SND_EXPLOSION       = 104,

    SND_INV_SPIN        = 108,
    SND_INV_HOME        = 109,
    SND_INV_CONTROLS    = 110,
    SND_INV_SHOW        = 111,
    SND_INV_HIDE        = 112,
    SND_INV_COMPASS     = 113,
    SND_INV_WEAPON      = 114,
    SND_INV_PAGE        = 115,
    SND_HEALTH          = 116,
        
    SND_STAIRS2SLOPE    = 119,

    SND_NATLA_SHOT      = 123,

    SND_HIT_SKATEBOY    = 132,

    SND_HIT_MUTANT      = 142,
    SND_STOMP           = 147,
        
    SND_LAVA            = 149,
    SND_FLAME           = 150,
    SND_DART            = 151,
        
    SND_TNT             = 170,
    SND_MUTANT_DEATH    = 171,
    SND_SECRET          = 173,

    SND_HELICOPTER      = 297,

    SND_WINSTON_SCARED  = 344,
    SND_WINSTON_WALK    = 345,
    SND_WINSTON_PUSH    = 346,
    SND_WINSTON_TRAY    = 347,
};


#define STATIC_MESH_GATE    10

extern int32 fps;

#ifdef PROFILE
    extern uint32 dbg_transform;
    extern uint32 dbg_poly;
    extern uint32 dbg_flush;
    extern uint32 dbg_vert_count;
    extern uint32 dbg_poly_count;
#endif

#define FIXED_SHIFT     14

#define SND_MAX_DIST    (8 * 1024)
#define SND_SHIFT       2
#define SND_CHANNELS    (1 << SND_SHIFT)
#define SND_FIXED_SHIFT 8
#define SND_VOL_SHIFT   6
#define SND_PITCH_SHIFT 7

#if defined(_WIN32)
    #define SND_SAMPLES      1024
    #define SND_OUTPUT_FREQ  22050
    #define SND_SAMPLE_FREQ  22050
    #define SND_ENCODE(x)    ((x) + 128)
    #define SND_DECODE(x)    ((x) - 128)
    #define SND_MIN          -128
    #define SND_MAX          127
#elif defined(__GBA__)
    #define SND_SAMPLES      176
    #define SND_OUTPUT_FREQ  10512
    #define SND_SAMPLE_FREQ  22050
    #define SND_ENCODE(x)    (x)
    #define SND_DECODE(x)    (((x) - 128) << 1)
    
    #ifdef USE_9BIT_SOUND        
        #define SND_MIN          -255
        #define SND_MAX          254
    #else
        #define SND_MIN          -128
        #define SND_MAX          127
    #endif
#elif defined(__DOS__)
    #define SND_SAMPLES      1024
    #define SND_OUTPUT_FREQ  11025
    #define SND_SAMPLE_FREQ  11025
    #define SND_ENCODE(x)    ((x) + 128)
    #define SND_DECODE(x)    ((x) - 128)
    #define SND_MIN          -128
    #define SND_MAX          127
#endif

#define MAX_UPDATE_FRAMES 10

#define FOV_SHIFT       3
#define MAX_MATRICES    8
#define MAX_MODELS      64
#define MAX_ITEMS       256
#define MAX_MESHES      50
#define MAX_VERTICES    3072
#define MAX_FACES       1024
#define MAX_ROOM_LIST   16
#define FOG_SHIFT       1
#define FOG_MAX         (10 * 1024)
#define FOG_MIN         (FOG_MAX - (8192 >> FOG_SHIFT))
#define VIEW_MIN_F      (32 << FIXED_SHIFT)
#define VIEW_MAX_F      (FOG_MAX << FIXED_SHIFT)

#define FRUSTUM_FAR_X   5 << 10
#define FRUSTUM_FAR_Y   3 << 10
#define FRUSTUM_FAR_Z   9 << 10

#define FACE_TRIANGLE   0x8000
#define FACE_COLORED    0x4000
#define FACE_CLIPPED    0x2000
#define FACE_FLAT       0x1000
#define FACE_SHADOW     0x0800
#define FACE_TEXTURE    0x07FF

#define NO_ROOM         0xFF
#define NO_MODEL        0xFF
#define NO_BOX          0xFFFF
#define NO_FLOOR        -127
#define WALL            (NO_FLOOR * 256)

#define DEG2SHORT       (0x10000 / 360)
#define ANGLE_0         0
#define ANGLE_1         DEG2SHORT
#define ANGLE_45        0x2000      // != 45 * DEG2SHORT !!!
#define ANGLE_90        0x4000
#define ANGLE_180       -0x8000

#define X_CLAMP(x, a, b) ((x) < (a) ? (a) : ((x) > (b) ? (b) : (x)))
#define X_MIN(a,b)       ((a) < (b) ? (a) : (b))
#define X_MAX(a,b)       ((a) > (b) ? (a) : (b))
#define X_SQR(x)         ((x) * (x))

#define DP43(a,b)  ((a).x * (b).x + (a).y * (b).y + (a).z * (b).z + (a).w)
#define DP33(a,b)  ((a).x * (b).x + (a).y * (b).y + (a).z * (b).z)

#define DIV_TABLE_SIZE  1024
#define FixedInvS(x)    ((x < 0) ? -divTable[abs(x)] : divTable[x])
#define FixedInvU(x)    divTable[x]

#define OT_SHIFT        4
#define OT_SIZE         ((VIEW_MAX_F >> (FIXED_SHIFT + OT_SHIFT)) + 1)

/*
#define PERSPECTIVE(x, y, z) {\
    x = (x / (z >> 7));\
    y = (y / (z >> 6));\
}
*/
#if defined(MODE13)
    #define PERSPECTIVE(x, y, z) {\
        int32 dz = (z >> (FIXED_SHIFT + FOV_SHIFT - 1)) / 3;\
        if (dz >= DIV_TABLE_SIZE) dz = DIV_TABLE_SIZE - 1;\
        int32 d = FixedInvU(dz);\
        x = d * (x >> FIXED_SHIFT) >> 12;\
        y = d * (y >> FIXED_SHIFT) >> 12;\
    }
#elif defined(MODE4)
    #define PERSPECTIVE(x, y, z) {\
        int32 dz = (z >> (FIXED_SHIFT + FOV_SHIFT - 1)) / 3;\
        if (dz >= DIV_TABLE_SIZE) dz = DIV_TABLE_SIZE - 1;\
        int32 d = FixedInvU(dz);\
        x = d * (x >> FIXED_SHIFT) >> 12;\
        y = d * (y >> FIXED_SHIFT) >> 12;\
    }
#elif defined(ROTATE90_MODE)
    #define PERSPECTIVE(x, y, z) {\
        int32 dz = (z >> (FIXED_SHIFT + FOV_SHIFT - 1)) / 3;\
        if (dz >= DIV_TABLE_SIZE) dz = DIV_TABLE_SIZE - 1;\
        int32 d = FixedInvU(dz);\
        x = d * (x >> FIXED_SHIFT) >> 12;\
        y = d * (y >> FIXED_SHIFT) >> 13;\
    }
#else
    #define PERSPECTIVE(x, y, z) {\
        int32 dz = (z >> (FIXED_SHIFT + FOV_SHIFT - 1)) / 3;\
        if (dz >= DIV_TABLE_SIZE) dz = DIV_TABLE_SIZE - 1;\
        int32 d = FixedInvU(dz);\
        x = d * (x >> FIXED_SHIFT) >> 13;\
        y = d * (y >> FIXED_SHIFT) >> 13;\
    }
#endif

extern const uint16 divTable[DIV_TABLE_SIZE];

// renderer internal
extern uint32 keys; 
extern AABB   frustumAABB;
extern Rect   viewport;
extern vec3i  cameraViewPos;
extern Matrix matrixStack[MAX_MATRICES];
extern int32  matrixStackIndex;
extern uint32 gVerticesCount;

extern SaveGame gSaveGame;

extern const FloorData* gLastFloorData;
extern FloorData gLastFloorSlant;

// level data
extern const Anim*      anims;
extern const AnimState* animStates;
extern const AnimRange* animRanges;
extern const uint16*    animFrames;

extern int32 modelsCount;
extern Model models[MAX_MODELS];
extern uint8 modelsMap[MAX_ITEMS];
extern uint8 staticMeshesMap[MAX_MESHES];

extern bool enableClipping;

template <class T>
X_INLINE void swap(T &a, T &b) {
    T tmp = a;
    a = b;
    b = tmp;
}

X_INLINE int32 classify(const Vertex &v, const Rect &clip) {
    return (v.x < clip.x0 ? 1 : 0) |
           (v.x > clip.x1 ? 2 : 0) |
           (v.y < clip.y0 ? 4 : 0) |
           (v.y > clip.y1 ? 8 : 0);
}

int32 phd_sin(int32 x);
int32 phd_cos(int32 x);
int32 phd_atan(int32 x, int32 y);
uint32 phd_sqrt(uint32 x);

X_INLINE int32 dot(const vec3i &a, const vec3i &b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

void anglesFromVector(int32 x, int32 y, int32 z, int16 &angleX, int16 &angleY);

Box boxRotate(const Box &box, int16 angle);
void boxTranslate(Box &box, const vec3i &offset);
bool boxIntersect(const Box &a, const Box &b);
bool boxContains(const Box &a, const vec3i &p);
vec3i boxPushOut(const Box &a, const Box &b);

X_INLINE Matrix& matrixGet()
{
    return matrixStack[matrixStackIndex];
}

X_INLINE void matrixPush()
{
    ASSERT(matrixStackIndex < MAX_MATRICES - 1);

    Matrix &a = matrixStack[matrixStackIndex++];
    Matrix &b = matrixStack[matrixStackIndex];
    memcpy(b, a, sizeof(Matrix));
}

X_INLINE void matrixPop()
{
    ASSERT(matrixStackIndex > 0);
    matrixStackIndex--;
}

void matrixTranslate(const vec3i &pos);
void matrixTranslateAbs(const vec3i &pos);
void matrixRotateX(int32 angle);
void matrixRotateY(int32 angle);
void matrixRotateZ(int32 angle);
void matrixRotateYXZ(int32 angleX, int32 angleY, int32 angleZ);
void matrixRotateZXY(int32 angleX, int32 angleY, int32 angleZ);
void matrixFrame(const vec3i &pos, uint16* angles);
void matrixSetIdentity();
void matrixSetView(const vec3i &pos, int32 angleX, int32 angleY);

void paletteSet(uint16* palette);

void drawGlyph(const Sprite *sprite, int32 x, int32 y);

void clear();
int32 rectIsVisible(const Rect* rect);
int32 boxIsVisible(const Box* box);
void transform(const vec3s &v, int32 vg);
bool transformBoxRect(const Box* box, Rect* rect);
void transformRoom(const RoomInfo::Vertex* vertices, int32 vCount);
void transformMesh(const vec3s* vertices, int32 vCount, uint16 intensity);
void faceAddQuad(uint32 flags, const Index* indices, int32 startVertex);
void faceAddTriangle(uint32 flags, const Index* indices, int32 startVertex);
void faceAddRoom(const Quad* quads, int32 qCount, const Triangle* triangles, int32 tCount, int32 startVertex);
void faceAddMesh(const Quad* rFaces, const Quad* crFaces, const Triangle* tFaces, const Triangle* ctFaces, int32 rCount, int32 crCount, int32 tCount, int32 ctCount, int32 startVertex);

void flush();

void readLevel(const uint8 *data);

Lara* getLara(const vec3i &pos);

bool useSwitch(Item* item, int32 timer);
bool useKey(Item* item);
bool usePickup(Item* item);


X_INLINE void dmaFill(void *dst, uint8 value, uint32 count)
{
    ASSERT((count & 3) == 0);
#ifdef __GBA__
    vu32 v = value;
    dma3_fill(dst, v, count);
#else
    memset(dst, value, count);
#endif
}

X_INLINE int16 xRand()
{
#ifdef __GBA__
    return qran();
#else 
    return rand();
#endif
}

#endif
