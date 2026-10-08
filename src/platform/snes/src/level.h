/* OpenLara - SNES / Super FX target
 * The port of src/fixed/level.h's role: where the level's tables are.
 *
 * Upstream keeps a `level` struct of pointers and writes `level.anims[i].
 * frameEnd`. GSU-2 owns cartridge ROM while rendering, so it stages one
 * blob in SRAM. FX3 permits concurrent access: its independent table
 * pointers address FastROM, except for writable sector floors in WRAM.
 *
 * So a table lookup is a function rather than a field, and the offsets are
 * bound once at load. That is the only structural difference; what the
 * tables hold is TR's, resolved into this side's terms by the packer.
 */

#ifndef H_OL_LEVEL
#define H_OL_LEVEL

#include "fixed.h"

/* GSU-2 stages one aggregate blob in SRAM. FX3 binds independent, full
   pointers into CPU FastROM, except for the writable SECTOR table in $7F.
   No read-only FX3 table requires a WRAM copy or bus-ownership handoff. */
#define olBlob          ((unsigned char *)0x710000LL)

/* One 16-bit load, not two byte loads and a shift: the 65816 has no
   alignment rule and the blob is little-endian. Halves the cost of every
   word read out of the level. */
#define olBlob16(o)     (*(uint16 *)(olBlob + (o)))
/* Keep the host oracle's sign extension identical to 816-tcc's 16-bit int. */
#define olBlobS16(o)    ((int)(int16)olBlob16(o))
#define olBlobS32(o)    (((int32)(int16)olBlob16((o) + 2) << 16) \
                         | (int32)(uint16)olBlob16(o))
#define olData16(p, o)  (*(uint16 *)((p) + (o)))
#define olDataS16(p, o) ((int)(int16)olData16(p, o))
#define olDataS32(p, o) (((int32)(int16)olData16(p, (o) + 2) << 16) \
                         | (int32)(uint16)olData16(p, o))

/* A table stride on this compiler is a call to tcc__mul unless it is spelt
   as shifts -- and the profiler put that helper at 13% of a tick, called
   from exactly these macros. So the strides are spelt out. */
#define olMul6(x)   (((x) << 2) + ((x) << 1))
#define olMul8(x)   ((x) << 3)
#define olMul10(x)  (((x) << 3) + ((x) << 1))
#define olMul12(x)  (((x) << 3) + ((x) << 2))
#define olMul14(x)  (((x) << 4) - ((x) << 1))
#define olMul16(x)  ((x) << 4)
#define olMul20(x)  (((x) << 4) + ((x) << 2))

/* GSU-2: offsets set by olLevelInit. FX3: low words of bound far pointers. */
extern unsigned int olRoomBase, olSectorBase, olLinkBase, olModelBase;
extern unsigned int olAnimBoxBase, olAnimSpeedBase;
extern unsigned int olAnimBase, olTransBase;
extern unsigned int olCmdBase, olTriggerBase, olItemBase;
extern unsigned int olBoxBase, olOverlapBase, olZoneBase;
extern unsigned int olStaticBase, olStaticFirstBase;
extern unsigned int olAnimAimBase;
#ifdef OL_FX3
#include "olhost.h"
/* The level this boot plays: its entry of the cartridge's level table
   (tools/mkbanks.py, levels.inc), copied by main.asm's SelectLevel before
   anything reads the level's data. Every place a level's data lives is
   here rather than a build-time constant, so one program serves every
   level a cartridge carries. Words only: main.asm reads it by offset
   (OLD_* in levels.inc), and 816-tcc lays a struct of words out plainly. */
typedef struct OlLevelDesc {
    uint16 hostBank;        /* HOST.BIN, bank-aligned: its directory at hostBank:0000 */
    uint16 levelBank;       /* LEVEL.BIN's first bank, as the CPU sees it ($80 + GSU's) */
    uint16 gsuEntry;        /* R15 for the level's GSU program, at PBR $41 */
    uint16 levelId;         /* upstream's LevelID */
    uint16 cameraAt, cameraBank, cameraSize;   /* CAMERA.BIN */
    uint16 invAt, invBank, invSize;            /* INV.BIN */
    uint16 fontAt, fontBank, fontSize;         /* FONT.BIN */
    uint16 paletteAt, paletteBank;             /* PALETTE.BIN */
    uint16 hudAt, hudBank;                     /* HUD.BIN */
    uint16 sndtabAt, sndtabBank;               /* SNDTAB.BIN */
    uint16 soundAt, soundBank, soundSize;      /* SOUND.BIN, the APU image */
    uint16 sphereBank;                         /* SPHERE.BIN, 0 for none */
    uint16 reserved;
    uint16 fxAt, fxBank, fxSize;               /* FX.BIN: effects, joints */
    uint16 gunAt, gunBank, gunSize;            /* GUNSND.BIN: the guns' sounds */
    uint16 spare[2];                           /* to 64 bytes: a shift */
} OlLevelDesc;
extern OlLevelDesc olLevel;
extern unsigned int olLevelIndex, olLevelCount; /* which of the cartridge's levels, of how many */
#define OL_LEVEL_MAX 4
extern uint16 olLevelHostBanks[OL_LEVEL_MAX];  /* every level's hostBank and */
extern uint16 olLevelIds[OL_LEVEL_MAX];        /* LevelID: the passport, a save */
extern unsigned int olBootKind;             /* the restart that brought this boot (save.c) */
extern unsigned char olHostDirectory[256];  /* the level's host directory, copied at bind */
extern unsigned char *olRoomData, *olSectorData, *olLinkData, *olModelData;
extern unsigned char *olAnimBoxData, *olAnimSpeedData, *olAnimData, *olTransData;
extern unsigned char *olCmdData, *olTriggerData, *olItemData;
extern unsigned char *olBoxData, *olOverlapData, *olZoneData;
extern unsigned char *olStaticData, *olStaticFirstData, *olAnimAimData, *olNavMapData;
extern unsigned int olHostTablesReady;
#define olHeader16(o) olData16(olHostDirectory, o)
#define olTable8(t, o)  ol##t##Data[o]
#define olTable16(t, o) olData16(ol##t##Data, o)
#define olTableS16(t, o) olDataS16(ol##t##Data, o)
#define olTableS32(t, o) olDataS32(ol##t##Data, o)
#else
/* Call sites supply already-grouped strides. Preserve the legacy expression
   order too: changing it needlessly churns GSU-2's tightly packed code. */
#define olTable8(t, o)  olBlob[ol##t##Base + o]
#define olTable16(t, o) olBlob16(ol##t##Base + o)
#define olTableS16(t, o) olBlobS16(ol##t##Base + o)
#define olTableS32(t, o) olBlobS32(ol##t##Base + o)
#define olRoomData (olBlob + olRoomBase)
#define olSectorData (olBlob + olSectorBase)
#define olLinkData (olBlob + olLinkBase)
#define olModelData (olBlob + olModelBase)
#define olAnimBoxData (olBlob + olAnimBoxBase)
#define olAnimSpeedData (olBlob + olAnimSpeedBase)
#define olAnimData (olBlob + olAnimBase)
#define olTransData (olBlob + olTransBase)
#define olCmdData (olBlob + olCmdBase)
#define olTriggerData (olBlob + olTriggerBase)
#define olItemData (olBlob + olItemBase)
#define olBoxData (olBlob + olBoxBase)
#define olOverlapData (olBlob + olOverlapBase)
#define olZoneData (olBlob + olZoneBase)
#define olStaticData (olBlob + olStaticBase)
#define olStaticFirstData (olBlob + olStaticFirstBase)
#define olAnimAimData (olBlob + olAnimAimBase)
#endif
extern unsigned int olRoomCount, olSectorCount;
extern unsigned int olBoxCount, olTriggerCount, olStaticCount;

/* Whether host tables are ready: validated bindings on FX3, staged SRAM
   length plus the GSU completion flag on GSU-2. */
int  olLevelReady(void);
void olLevelInit(void);

/* --- animations ---------------------------------------------------------- */
/* The box an animation occupies, which is upstream's per-frame box unioned
   over the animation. See the note in packer/out_SNES.h for why. */
#define olAnimBox(a, k)     olTableS16(AnimBox, olMul12(a) + ((k) << 1))

/* Sparse current-frame aim points follow the static room index: animation
   count, relative record offsets, then tick maps to shared XYZ points.
   A zero record offset retains the animation-union aim point. */

/* The speed and acceleration TR gives an animation, in 16.16 -- which is
   where movement comes from: `ItemObj::move` reads them for the horizontal
   speed, so a walk and a run differ because their animations do. Read in two
   halves because there is no 32-bit load here. */
#define olAnimSpeed(a)      olTableS32(AnimSpeed, olMul8(a))
#define olAnimAccel(a)      olTableS32(AnimSpeed, olMul8(a) + 4)

#define olAnimFrames(a)     ((unsigned int)olTable8(Anim, olMul14(a)))
#define olAnimFrameRate(a)  ((unsigned int)olTable8(Anim, olMul14(a) + 1))
#define olAnimNext(a)       olTable16(Anim, olMul14(a) + 2)
#define olAnimNextFrame(a)  ((unsigned int)olTable8(Anim, olMul14(a) + 4))
#define olAnimTransStart(a) olTable16(Anim, olMul14(a) + 6)
#define olAnimTransCount(a) ((unsigned int)olTable8(Anim, olMul14(a) + 8))
#define olAnimState(a)      ((unsigned int)olTable8(Anim, olMul14(a) + 9))
#define olAnimCmdStart(a)   olTable16(Anim, olMul14(a) + 10)
#define olAnimCmdCount(a)   ((unsigned int)olTable8(Anim, olMul14(a) + 12))

/* --- the transitions out of one ------------------------------------------ */
#define olTransGoal(t)      ((unsigned int)olTable8(Trans, olMul6(t)))
#define olTransBegin(t)     ((unsigned int)olTable8(Trans, olMul6(t) + 1))
#define olTransEnd(t)       ((unsigned int)olTable8(Trans, olMul6(t) + 2))
#define olTransFrame(t)     ((unsigned int)olTable8(Trans, olMul6(t) + 3))
#define olTransAnim(t)      olTable16(Trans, olMul6(t) + 4)

/* --- the commands it carries ---------------------------------------------- */
#define olCmdOp(c)          ((unsigned int)olTable8(Cmd, olMul8(c)))
#define olCmdA0(c)          olTableS16(Cmd, olMul8(c) + 2)
#define olCmdA1(c)          olTableS16(Cmd, olMul8(c) + 4)
#define olCmdA2(c)          olTableS16(Cmd, olMul8(c) + 6)

/* --- triggers, resolved out of TR's FloorData by the packer --------------- */
#define olTrigType(t)       ((unsigned int)olTable8(Trigger, olMul16(t)))
#define olTrigItemCount(t)  ((unsigned int)olTable8(Trigger, olMul16(t) + 1))
#define olTrigMask(t)       ((unsigned int)olTable8(Trigger, olMul16(t) + 2))
#define olTrigOnce(t)       ((unsigned int)olTable8(Trigger, olMul16(t) + 3))
#define olTrigTimer(t)      ((unsigned int)olTable8(Trigger, olMul16(t) + 4))
/* The actions besides activating entities the host has anything to do
   about, as bits, and the secret's index. */
#define olTrigActions(t)    ((unsigned int)olTable8(Trigger, olMul16(t) + 5))
#define olTrigSecret(t)     ((unsigned int)olTable8(Trigger, olMul16(t) + 6))
#define OL_TRIGGER_END      1
#define OL_TRIGGER_SECRET   2
#define OL_TRIGGER_CAMERA   4   /* FX3: its camera actions are in CAMERA.BIN */
#define OL_TRIGGER_TARGET   8
#define olTrigItem(t, j)    ((unsigned int)olTable8(Trigger, olMul16(t) + 8 + (j)))

/* --- models --------------------------------------------------------------- */
#define olModelFirstAnim(m) olTable16(Model, olMul16(m))
#define olModelBox(m, k)    olTableS16(Model, olMul16(m) + 4 + ((k) << 1))

/* --- navigation ----------------------------------------------------------- */
#define olBoxMinZ(b)        ((int)olTable8(Box, olMul8(b)))
#define olBoxMaxZ(b)        ((int)olTable8(Box, olMul8(b) + 1))
#define olBoxMinX(b)        ((int)olTable8(Box, olMul8(b) + 2))
#define olBoxMaxX(b)        ((int)olTable8(Box, olMul8(b) + 3))
#define olBoxFloor(b)       olTableS16(Box, olMul8(b) + 4)
#define olBoxOverlap(b)     olTable16(Box, olMul8(b) + 6)
#define olOverlap(p)        olTable16(Overlap, ((p) << 1))
#ifdef OL_FX3
#define OL_ZONE_GROUND_1 0
#define OL_ZONE_GROUND_2 1
#define OL_ZONE_FLY      2
extern unsigned int olZoneOffsets[3], olFlipped;
#define olZone(b, kind)     olTable16(Zone, olZoneOffsets[kind] + ((b) << 1))
#else
#define olZone(b, fly)      olTable16(Zone, ((fly) ? (olBoxCount << 1) : 0) + ((b) << 1))
#endif

/* --- sectors -------------------------------------------------------------- */
/* Read by src/room.asm rather than from here: a sector is four bytes,
       int8 floor, int8 ceiling, uint16 link
   in 256-unit heights, and the link names an eight-byte record shared by
   every sector that links the same way and slopes the same way:
       uint8 roomBelow, roomAbove, portal, trigger
       uint16 floorSlants, ceilingSlants    two signed bytes on FX3;
                                          signed five-bit values on GSU-2
   Link 0 links nowhere and is flat. LEVEL1 has 583 of them for 4,026
   sectors, which is the 13 KB that keeps the blob inside its bank. */

/* --- rooms, for turning a position into an absolute sector ---------------- */
#define olRoomSectorX(r)    ((int)olTable8(Room, olMul10(r)))
#define olRoomSectorZ(r)    ((int)olTable8(Room, olMul10(r) + 1))
#define olRoomX(r)          olTableS16(Room, olMul10(r) + 2)
#define olRoomZ(r)          olTableS16(Room, olMul10(r) + 4)
#define olRoomSectorOff(r)  olTable16(Room, olMul10(r) + 6)
/* Bit 7 of the width byte is TR's ROOM_FLAG_WATER; the width has the low
   seven, and room.asm masks it the same way. A byte of its own would have
   moved every table behind the room records for one bit. */
#define olRoomXSectors(r)   ((unsigned int)olTable8(Room, olMul10(r) + 8) & 0x7F)
#define olRoomZSectors(r)   ((unsigned int)olTable8(Room, olMul10(r) + 9))
#define olRoomWater(r)      (olTable8(Room, olMul10(r) + 8) & 0x80)

/* The sector and link records themselves, for the C that walks them --
   the water column (Room_water) -- rather than the point resolution,
   which is room.asm's. A sector is four bytes, a link eight; see below. */
#define OL_NO_ROOM          0xFF
#define olSectorFloor(s)    ((int)(int8)olTable8(Sector, ((s) << 2)))
#define olSectorCeiling(s)  ((int)(int8)olTable8(Sector, ((s) << 2) + 1))
#define olSectorLink(s)     olTable16(Sector, ((s) << 2) + 2)
#define olLinkBelow(l)      ((unsigned int)olTable8(Link, ((l) << 3)))
#define olLinkAbove(l)      ((unsigned int)olTable8(Link, ((l) << 3) + 1))

int olSectorX(unsigned int room, int x);
int olSectorZ(unsigned int room, int z);

/* --- the static meshes she can walk into --------------------------------- */
/* The packer walked the placements, dropped the plants and the ones flagged
   as having no collision, and turned each box to its quadrant; what is left
   is the box where it stands, in room order. k: minX, maxX, minY, maxY,
   minZ, maxZ. */
#define olStaticRoom(i)     ((unsigned int)olTable8(Static, olMul20(i)))
#define olStaticX(i)        olTableS16(Static, olMul20(i) + 2)
#define olStaticY(i)        olTableS16(Static, olMul20(i) + 4)
#define olStaticZ(i)        olTableS16(Static, olMul20(i) + 6)
#define olStaticBox(i, k)   olTableS16(Static, olMul20(i) + 8 + ((k) << 1))
/* Where room r's run starts; room r + 1's is where it ends. */
#define olStaticFirst(r)    olTable16(StaticFirst, ((r) << 1))
/* The scan itself, from src/room.asm: the first placement in the room whose
   box, translated to the point, meets her cylinder -- written to `box`,
   returning 1 -- or 0. The macros above are one far read each and the
   compiler makes a far read a hundred cycles; the scan runs per placement,
   per probe, per tick. */
struct AABBi;
int olStaticHit(unsigned int room, int px, int py, int pz, int radius,
                int height, struct AABBi* box);

/* --- the entities by class, indexed once at load ------------------------- */
/* Three loops over all sixty entities ran every tick to find the handful of
   doors, bridges and creatures among them, and the profiler put them at a
   third of the tick. These are the handfuls. */
#define OL_LIST_MAX     32
#define OL_ROOM_MAX     64
extern uint8        olDoors[OL_LIST_MAX], olBridges[OL_LIST_MAX], olCreatures[OL_LIST_MAX];
extern uint8        olUsables[OL_LIST_MAX];
extern unsigned int olDoorCount, olBridgeCount, olCreatureCount, olUsableCount;
/* The sector box every bridge stands in, so a probe outside it -- nearly
   all of them -- never walks the bridge list. In sectors: minX, maxX,
   minZ, maxZ. */
extern int          olBridgeBox[4];

#endif
