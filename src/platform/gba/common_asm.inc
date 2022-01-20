.section .iwram
.arm

#define TEX_2PX     // lazy texturing, comment out for per-pixel

.equ VERTEX_X,      0
.equ VERTEX_Y,      2
.equ VERTEX_Z,      4
.equ VERTEX_G,      6
.equ VERTEX_CLIP,   7
.equ VERTEX_T,      8
.equ VERTEX_PREV,   12
.equ VERTEX_NEXT,   16
.equ VERTEX_SIZEOF, 20

.equ EWRAM_START,   0x2000000
.equ IWRAM_START,   0x3000000
.equ VRAM_START,    0x6000000
.equ VRAM_PAGE,     0x000A000

.equ FRAME_WIDTH,   240
.equ FRAME_HEIGHT,  160

.equ DIVLUT_ADDR,   EWRAM_START
.equ LMAP_ADDR,     IWRAM_START

.equ CLIP_LEFT,     ((1 << 0) << 8)
.equ CLIP_RIGHT,    ((1 << 1) << 8)
.equ CLIP_TOP,      ((1 << 2) << 8)
.equ CLIP_BOTTOM,   ((1 << 3) << 8)
.equ CLIP_FAR,      ((1 << 4) << 8)
.equ CLIP_NEAR,     ((1 << 5) << 8)
.equ CLIP_MASK_VP,  (CLIP_LEFT + CLIP_RIGHT + CLIP_TOP + CLIP_BOTTOM)
.equ CLIP_MASK,     (CLIP_MASK_VP + CLIP_FAR + CLIP_NEAR)

.equ FACE_TYPE_SHIFT, 11
.equ FACE_FLAT_ADD,   (2 << FACE_TYPE_SHIFT)
.equ FACE_CLIPPED,    (1 << 16)

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

.equ VIEW_MIN_F,    (VIEW_MIN << FIXED_SHIFT)
.equ VIEW_MAX_F,    (VIEW_MAX << FIXED_SHIFT)
.equ VIEW_OFF_F,    (VIEW_OFF << FIXED_SHIFT)

.equ MAX_CAUSTICS,   32
.equ MAX_RAND_TABLE, 32

.equ MIN_INT32,     0x80000000
.equ MAX_INT32,     0x7FFFFFFF

.macro CCW skip
    ldrsh vx0, [vp0, #VERTEX_X]
    ldrsh vy0, [vp0, #VERTEX_Y]
    ldrsh vx2, [vp2, #VERTEX_X]
    ldrsh vy1, [vp1, #VERTEX_Y]
    rsb vx2, vx2, vx0   // reverse order for mla
    sub vy1, vy1, vy0
    mul tmp, vx2, vy1
    ldrsh vx1, [vp1, #VERTEX_X]
    ldrsh vy2, [vp2, #VERTEX_Y]
    sub vx1, vx1, vx0
    sub vy2, vy2, vy0
    mlas tmp, vx1, vy2, tmp
    ble \skip
.endm

.macro tex index, uv
    and \index, \uv, #0xFF00
    orr \index, \uv, lsr #24        // index = v * 256 + u
    ldrb \index, [TILE, \index]
.endm

.macro lit index
    ldrb \index, [LMAP, \index]
.endm
