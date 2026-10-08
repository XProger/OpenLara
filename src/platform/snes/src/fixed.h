/* OpenLara - SNES / Super FX target
 * The port of src/fixed: types, conventions, and the fixed-point primitives.
 *
 * src/fixed is C++ and 816-tcc is not a C++ compiler, so this target cannot
 * recompile the engine; it has to come across as C. That is a large job, and
 * what makes it tractable is that every file here compiles two ways:
 *
 *   for the cartridge   816-tcc, 16-bit int, no 32-bit int at all
 *   for the desktop     the host compiler, with OL_HOST_BUILD defined
 *
 * so tools/fixedcheck can run the *shipped* code against an independent model
 * and diff them. Nothing is ported without that. See `make fixedcheck`.
 *
 * ---------------------------------------------------------------------------
 * Conventions, which are all forced by the compiler rather than chosen:
 *
 *   int32          `long long`. 816-tcc's int is 16 bits and there is no
 *                  32-bit int, so every 32-bit operation is a library call --
 *                  arithmetic-heavy work belongs on the GSU, not here.
 *   member fns     become free functions taking the struct first, named for
 *                  their type: `ItemObj_update(item)`.
 *   operators      become named functions. There is no overloading.
 *   references     become pointers.
 *   names          functions ported from src/fixed keep their upstream names,
 *                  so the two can be read side by side. Anything that exists
 *                  only on this target is prefixed `ol`.
 *   tables         live in the compiled image, which is copied to WRAM at
 *                  boot, and so come out of the same 32 KB the code does.
 *                  Anything big has to move to .bss and be filled from ROM
 *                  before the GSU starts.
 * ---------------------------------------------------------------------------
 */

#ifndef H_OL_FIXED
#define H_OL_FIXED

#ifdef OL_HOST_BUILD
    typedef signed char        int8;
    typedef short              int16;
    typedef int                int32;
    typedef unsigned char      uint8;
    typedef unsigned short     uint16;
    typedef unsigned int       uint32;
#else
    /* 816-tcc: char 1, short 2, int 2, long 2, long long 4, pointer 4. */
    typedef signed char        int8;
    typedef int                int16;
    typedef long long          int32;
    typedef unsigned char      uint8;
    typedef unsigned int       uint16;
    typedef unsigned long long uint32;
#endif

/* Input bits, matching InputKey in src/fixed/common.h. Upstream's IN_* are
   the same set under other names; the mapping onto this pad is here so both
   game.c and lara.c read one list. */
#define IK_UP       0x0001
#define IK_RIGHT    0x0002
#define IK_DOWN     0x0004
#define IK_LEFT     0x0008
#define IK_A        0x0010
#define IK_B        0x0020
#define IK_L        0x0040
#define IK_R        0x0080
#define IK_Y        0x0400      /* SNES Y; START and SELECT are 0x100/0x200 */
#define IK_SELECT   0x0200

#define IN_UP       IK_UP
#define IN_DOWN     IK_DOWN
#define IN_LEFT     IK_LEFT
#define IN_RIGHT    IK_RIGHT
#define IN_JUMP     IK_A
#define IN_ACTION   IK_B
#define IN_WALK     IK_L        /* the shoulder, as TR uses shift */
#define IN_WEAPON   IK_R
#define IN_LOOK     IK_Y        /* upstream's look: the head, then the camera */

/* Angles are 16 bit, 65536 to the turn, as everywhere in TR. */
#define ANGLE_0         0
#define ANGLE_1         182     /* 65536 / 360, which is upstream's ANGLE(1) */
#define ANGLE_45        0x2000
#define ANGLE_90        0x4000
#define ANGLE_180       0x8000

/* Game logic now runs at TR's native thirty ticks a second, queued by NMI and
   independent of the renderer. Keep these names at the conversion sites so a
   future PAL scheduler or slow-motion mode still has one rate boundary. */
#define OL_FRAME_TICKS  1
#define OL_SPEED(v)     ((v) * OL_FRAME_TICKS)
#define OL_ACCEL(v)     ((v) * OL_FRAME_TICKS * OL_FRAME_TICKS)

/* One sector, and the unit floor and ceiling heights are stored in. */
#define SECTOR_SIZE     1024
#define HEIGHT_UNIT     256

/* sin and cos come back in 1.14: 16384 is 1.0, which is what TR uses and what
   src/fixed's gSinCosTable holds. */
#define FIXED_SHIFT     14
#define FIXED_ONE       (1 << FIXED_SHIFT)

int32 phd_sin(int32 angle);
int32 phd_cos(int32 angle);

/* The angle of the vector (x, y), TR's argument order: x along the screen,
   y into it. Matches src/fixed's phd_atan, octant reduction and all. */
int32 phd_atan(int32 x, int32 y);

uint32 phd_sqrt(uint32 x);
void olMathCheck(void);

/* The logic PRNG: the one the game consults, as against the one the renderer
   would. Its sequence is part of the game's behaviour, so it is checked
   against upstream's first values by olMathCheck. */
void  rand_reset(void);
int32 rand_logic(void);
#define RAND_LOGIC(r)   ((int)((rand_logic() * (r)) >> 15))

/* How many entities the host holds a slot for, matching SNES_ITEM_MAX in the
   packer and GS_ITEM_MAX in gsu.inc. The packer drops entities past it rather
   than letting two share a slot. */
#define OL_ITEM_MAX     128

/* What SectorAt reports for a wall, or for a point off the sector grid. */
#define OL_NO_FLOOR     32767

/* How far apart consecutive entities are held in their animation, so a room
   full of one model does not step in time with itself. The oracles model this
   -- tools/gsusim.py stages it and tools/typesnes_dump.ts pins it -- because
   it is part of what the renderer sees. */
#define OL_PHASE_SPREAD 5

/* How many models a level may have, and how big the blob the GSU stages is --
   matching GS_ITEMS_MAX in snes.inc. Both are caps, not counts. */
#define OL_MODEL_MAX    64
#define OL_ANIM_MAX     320
#define OL_TRANS_MAX    512
#define OL_CMD_MAX      320
#define OL_TRIGGER_MAX  255
#define OL_BOX_MAX      1024
#define OL_TRIGGER_ITEMS 6
#define OL_ITEM_BLOB    65534

#endif
