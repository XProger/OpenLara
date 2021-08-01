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
