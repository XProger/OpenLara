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
.equ CLIP_FAR,      ((1 << 4) << 8)
.equ CLIP_NEAR,     ((1 << 5) << 8)
.equ CLIP_FRAME,    ((1 << 6) << 8)
.equ CLIP_DISCARD,  (CLIP_LEFT + CLIP_RIGHT + CLIP_TOP + CLIP_BOTTOM + CLIP_FAR + CLIP_NEAR)
.equ CLIP_MASK,     0xFF00

.equ FACE_TEXTURE_BITS, 14
.equ FACE_TEXTURE,      ((1 << FACE_TEXTURE_BITS) - 1)
.equ FACE_GOURAUD,      (2 << FACE_TYPE_SHIFT)
.equ FACE_CLIPPED,      (1 << 18)
.equ FACE_TRIANGLE,     (1 << 19)

.equ FACE_FLAGS,      0
.equ FACE_NEXT,       4
.equ FACE_INDICES,    8

.equ FACE_TYPE_SHIFT, 14
.equ FACE_TYPE_MASK,  (15 << FACE_TYPE_SHIFT)

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
.equ OT_SHIFT,      4

.equ VIEW_DIST,     (1024 * 10)   // max = DIV_TABLE_END << PROJ_SHIFT
.equ FOG_SHIFT,     1
.equ FOG_MAX,       VIEW_DIST
.equ FOG_MIN,       (FOG_MAX - (8192 >> FOG_SHIFT))
.equ VIEW_MIN,      (64)
.equ VIEW_MAX,      (VIEW_DIST)
.equ VIEW_OFF,      4096

.equ OT_SIZE,       641

.equ VIEW_MIN_F,    (VIEW_MIN << FIXED_SHIFT)
.equ VIEW_MAX_F,    (VIEW_MAX << FIXED_SHIFT)
.equ VIEW_OFF_F,    (VIEW_OFF << FIXED_SHIFT)

.equ MAX_CAUSTICS,   32
.equ MAX_RAND_TABLE, 32
.equ MAX_ANIM_TEX,   128

.equ MIN_INT32,     0x80000000
.equ MAX_INT32,     0x7FFFFFFF

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
    ldrsh vx0, [vp0, #VERTEX_X]
    ldrsh vy0, [vp0, #VERTEX_Y]
    ldrsh vx2, [vp2, #VERTEX_X]
    ldrsh vy1, [vp1, #VERTEX_Y]
    rsb vx2, vx2, vx0 // reverse order for mla
    sub vy1, vy1, vy0
    mul vy1, vx2, vy1
    ldrsh vx1, [vp1, #VERTEX_X]
    sub vx0, vx1, vx0
    ldrsh vy2, [vp2, #VERTEX_Y]
    sub vy0, vy2, vy0
    mlas vy1, vx0, vy0, vy1
    ble \skip
.endm

.macro scaleUV uv, u, v, f
    asr \u, \uv, #16
    mul \u, \f                // u = f * int16(uv >> 16)
    lsl \v, \uv, #16
    asr \v, #16
    mul \v, \f                // v = f * int16(uv)
    lsr \u, #16
    lsl \u, #16
    orr \uv, \u, \v, lsr #16  // uv = (u & 0xFFFF0000) | (v >> 16)
.endm

.macro tex index, uv
    and \index, \uv, #0xFF00
    orr \index, \uv, lsr #24        // index = v * 256 + u
    ldrb \index, [TILE, \index]
.endm

.macro lit index
    ldrb \index, [LMAP, \index]
.endm
