#include "common_asm.inc"

flags  .req r0
top    .req r1
y      .req r2
width  .req r3
pixel  .req flags
type   .req r12

.extern rasterizeS_asm
.extern rasterizeF_asm
.extern rasterizeFT_asm
.extern rasterizeFTA_asm
.extern rasterizeGT_asm
.extern rasterizeGTA_asm
.extern rasterizeSprite_c
.extern rasterizeFillS_asm
.extern rasterizeLineH_asm
.extern rasterizeLineV_asm
.extern rasterize_dummy

.global rasterize_asm
rasterize_asm:
    and type, flags, #FACE_TYPE_MASK
    cmp type, #FACE_TYPE_F
    streqb flags, [top, #VERTEX_CLIP]

    ldr pixel, =fb
    ldr pixel, [pixel]
    ldrsh y, [top, #VERTEX_Y]

#if (FRAME_WIDTH == 240)    // pixel += (y * 16 - y) * 16
    rsb y, y, y, lsl #4
    add pixel, pixel, y, lsl #4
#else
    mov width, #FRAME_WIDTH
    mla pixel, y, width, pixel
#endif

    mov r2, top
    add pc, type, lsr #(FACE_TYPE_SHIFT - 2)
    nop
    b rasterizeS_asm
    b rasterizeF_asm
    b rasterizeFT_asm
    b rasterizeFTA_asm
    b rasterizeGT_asm
    b rasterizeGTA_asm
    b rasterizeSprite_c
    b rasterizeFillS_asm
    b rasterizeLineH_asm
    b rasterizeLineV_asm
