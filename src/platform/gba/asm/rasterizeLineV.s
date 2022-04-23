#include "common_asm.inc"

pixel   .req r0   // arg
L       .req r1   // arg
R       .req r2   // arg
tmp     .req r12
index   .req L
height  .req R

.global rasterizeLineV_asm
rasterizeLineV_asm:
    add R, #VERTEX_SIZEOF
    ldrsh tmp, [L, #VERTEX_X]
    ldrb index, [L, #VERTEX_G]
    ldrsh height, [R, #VERTEX_Y]
    add pixel, tmp

    tst pixel, #1
      beq .right

    sub pixel, #1
.left:
    ldrb tmp, [pixel]
    orr tmp, index, lsl #8
    strh tmp, [pixel], #FRAME_WIDTH
    subs height, #1
      bne .left
    mov pc, lr

.right:
    ldrb tmp, [pixel, #1]
    orr tmp, index, tmp, lsl #8
    strh tmp, [pixel], #FRAME_WIDTH
    subs height, #1
      bne .right
    mov pc, lr
