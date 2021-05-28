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

#ifdef __TNS__
	.equ VRAM_STRIDE,   320
#else
	.equ VRAM_STRIDE,   240
#endif

#define TEX_2PX
