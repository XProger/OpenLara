.section .iwram
.arm

.equ VERTEX_X,      0
.equ VERTEX_Y,      2
.equ VERTEX_Z,      4
.equ VERTEX_G,      6
.equ VERTEX_CLIP,   7
.equ VERTEX_T,      8
.equ VERTEX_PREV,   12
.equ VERTEX_NEXT,   16
.equ EWRAM_START,   0x2000000
.equ IWRAM_START,   0x3000000

.equ DIVLUT_ADDR,   EWRAM_START
.equ LMAP_ADDR,     IWRAM_START

#ifdef __TNS__
    .equ VRAM_STRIDE,   320
#else
    .equ VRAM_STRIDE,   240
#endif

#define TEX_2PX

CLIP_LEFT       = ((1 << 0) << 8)
CLIP_RIGHT      = ((1 << 1) << 8)
CLIP_TOP        = ((1 << 2) << 8)
CLIP_BOTTOM     = ((1 << 3) << 8)
CLIP_FAR        = ((1 << 4) << 8)
CLIP_NEAR       = ((1 << 5) << 8)
CLIP_MASK_VP    = (CLIP_LEFT + CLIP_RIGHT + CLIP_TOP + CLIP_BOTTOM)
CLIP_MASK       = (CLIP_MASK_VP + CLIP_FAR + CLIP_NEAR)

FACE_FLAT       = (1 << 12)
FACE_CLIPPED    = (1 << 13)

FRAME_WIDTH     = 240
FRAME_HEIGHT    = 160

FIXED_SHIFT     = 14
PROJ_SHIFT      = 4
OT_SHIFT        = 4

VIEW_DIST       = (1024 * 10)   // max = DIV_TABLE_END << PROJ_SHIFT
FOG_SHIFT       = 1
FOG_MAX         = VIEW_DIST
FOG_MIN         = (FOG_MAX - (8192 >> FOG_SHIFT))
VIEW_MIN        = (64)
VIEW_MAX        = (VIEW_DIST)
VIEW_OFF        = 2048

VIEW_MIN_F      = (VIEW_MIN << FIXED_SHIFT)
VIEW_MAX_F      = (VIEW_MAX << FIXED_SHIFT)
VIEW_OFF_F      = (VIEW_OFF << FIXED_SHIFT)

MIN_INT32       = 0x80000000
MAX_INT32       = 0x7FFFFFFF

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
    bgt \skip
.endm