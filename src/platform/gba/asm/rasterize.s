#include "common_asm.inc"

flags  .req r0      // arg
L      .req r1      // arg
R      .req r2
y      .req r3
type   .req r12
pixel  .req flags

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
    andeq R, flags, #0xFF   // R = face color for FACE_TYPE_F
    movne R, L              // R = L otherwise

    ldr pixel, =fb
    ldr pixel, [pixel]
    ldrsh y, [L, #VERTEX_Y]

    // pixel += y * 240 -> (y * 16 - y) * 16
    rsb y, y, y, lsl #4
    add pixel, pixel, y, lsl #4

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
