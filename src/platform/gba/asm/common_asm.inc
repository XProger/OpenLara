.section .iwram
.arm

#define FRAME_WIDTH   240
#define FRAME_HEIGHT  160

.equ VERTEX_X,      0
.equ VERTEX_Y,      2
.equ VERTEX_Z,      4
.equ VERTEX_G,      6
.equ VERTEX_CLIP,   7
.equ VERTEX_T,      8
.equ VERTEX_PREV,   12
.equ VERTEX_NEXT,   13

.equ VERTEX_SIZEOF_SHIFT,   4
.equ VERTEX_SIZEOF,         (1 << VERTEX_SIZEOF_SHIFT)

.equ LEVEL_TILES,       40
.equ LEVEL_TEXTURES,    92
.equ LEVEL_SPRITES,     96

.equ SPRITE_SIZE,   16

.equ TEXTURE_TILE,  2
.equ TEXTURE_UV01,  4
.equ TEXTURE_UV23,  8

.equ EWRAM_START,   0x2000000
.equ IWRAM_START,   0x3000000
.equ VRAM_START,    0x6000000
.equ VRAM_PAGE,     0x000A000

.equ DIVLUT_ADDR,   EWRAM_START
.equ LMAP_ADDR,     IWRAM_START

.equ CLIP_LEFT,     ((1 << 0) << 8)
.equ CLIP_RIGHT,    ((1 << 1) << 8)
.equ CLIP_TOP,      ((1 << 2) << 8)
.equ CLIP_BOTTOM,   ((1 << 3) << 8)
.equ CLIP_PLANE,    ((1 << 4) << 8)
.equ CLIP_FRAME,    ((1 << 5) << 8)
.equ CLIP_DISCARD,  (CLIP_LEFT + CLIP_RIGHT + CLIP_TOP + CLIP_BOTTOM + CLIP_PLANE)
.equ CLIP_MASK,     0xFF00

.equ FACE_TYPE_SHIFT,   14
.equ FACE_TEXTURE_BITS, 14
.equ FACE_TEXTURE,      ((1 << FACE_TYPE_SHIFT) - 1)
.equ FACE_GOURAUD,      (2 << FACE_TYPE_SHIFT)
.equ FACE_CLIPPED,      (1 << 18)
.equ FACE_TRIANGLE_BIT, 19
.equ FACE_TRIANGLE,     (1 << FACE_TRIANGLE_BIT)

.equ FACE_FLAGS,      0
.equ FACE_NEXT,       4
.equ FACE_INDICES,    8

.equ FACE_TYPE_MASK,    (15 << FACE_TYPE_SHIFT)

.equ FACE_TYPE_SHADOW,  (0 << FACE_TYPE_SHIFT)
.equ FACE_TYPE_F,       (1 << FACE_TYPE_SHIFT)
.equ FACE_TYPE_FT,      (2 << FACE_TYPE_SHIFT)
.equ FACE_TYPE_FTA,     (3 << FACE_TYPE_SHIFT)
.equ FACE_TYPE_GT,      (4 << FACE_TYPE_SHIFT)
.equ FACE_TYPE_GTA,     (5 << FACE_TYPE_SHIFT)
.equ FACE_TYPE_SPRITE,  (6 << FACE_TYPE_SHIFT)
.equ FACE_TYPE_FILL_S,  (7 << FACE_TYPE_SHIFT)
.equ FACE_TYPE_LINE_H,  (8 << FACE_TYPE_SHIFT)
.equ FACE_TYPE_LINE_V,  (9 << FACE_TYPE_SHIFT)

.equ FIXED_SHIFT,   14
.equ PROJ_SHIFT,    4
.equ MESH_SHIFT,    2

.equ VIEW_MIN,      64
.equ VIEW_MAX,      (10 << 10)
.equ VIEW_OFF,      4096
.equ FOG_SHIFT,     4
.equ FOG_MIN,       (VIEW_MAX - 2048)

.equ OT_SHIFT,      4
.equ OT_SIZE,       ((VIEW_MAX >> OT_SHIFT) + 1)

.equ MAX_CAUSTICS,   32
.equ MAX_RAND_TABLE, 32
.equ MAX_ANIM_TEX,   128

.equ MIN_INT32,     0x80000000
.equ MAX_INT32,     0x7FFFFFFF

.equ SND_VOL_SHIFT,     6
.equ SND_FIXED_SHIFT,   8

// res = divTable[x] (uint16)
.macro divLUT res, x
    add \res, \x, #DIVLUT_ADDR
    ldrh \res, [\res, \x]
.endm

// vx0 - vg0
// vy0 - vg1
// vx1 - vg2
// vy1 - vg3
// vx2 - vg2
// vy2 - vg2
.macro CCW skip
    ldr vy0, [vp0, #VERTEX_X]       // yyxx
    mov vx0, vy0, lsl #16
    ldrsh vx2, [vp2, #VERTEX_X]
    ldrsh vy1, [vp1, #VERTEX_Y]
    rsb vx2, vx2, vx0, asr #16      // reverse order for mla
    subs vy1, vy1, vy0, asr #16
    mulne vy1, vx2, vy1
    ldrsh vx1, [vp1, #VERTEX_X]
    sub vx0, vx1, vx0, asr #16
    ldrsh vy2, [vp2, #VERTEX_Y]
    subs vy0, vy2, vy0, asr #16
    mulne vy0, vx0, vy0
    adds vy1, vy0
    ble \skip
.endm

.macro scaleUV uv, tmp, unused, f
    asrs \tmp, \uv, #16
    mulne \tmp, \f, \tmp            // u = f * int16(uv >> 16)
    lsrne \tmp, #16
    lslne \tmp, #16

    lsls \uv, \uv, #16
    asrne \uv, #16
    mulne \uv, \f, \uv              // v = f * int16(uv)

    orr \uv, \tmp, \uv, lsr #16     // uv = (u & 0xFFFF0000) | (v >> 16)
.endm

.macro tex index, uv
    and \index, \uv, #0xFF00
    orr \index, \uv, lsr #24        // index = v * 256 + u
    ldrb \index, [TILE, \index]
.endm

.macro lit index
    ldrb \index, [LMAP, \index]
.endm

.macro fiq_on
    msr cpsr, #0x11     // switch r8-r14 to FIQ (IRQ enabled)
.endm

.macro fiq_off
    msr cpsr, #0x1F     // restore r8-r14
.endm

.macro fiq_on_ne
    msrne cpsr, #0x11   // switch r8-r14 to FIQ (IRQ enabled)
.endm

.macro fiq_off_ne
    msrne cpsr, #0x1F   // restore r8-r14
.endm
