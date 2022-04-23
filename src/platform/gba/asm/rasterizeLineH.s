#include "common_asm.inc"

pixel   .req r0   // arg
L       .req r1   // arg
R       .req r2   // arg
tmp     .req r12
index   .req L
width   .req R

.global rasterizeLineH_asm
rasterizeLineH_asm:
    add R, #VERTEX_SIZEOF
    ldrsh tmp, [L, #VERTEX_X]
    add pixel, tmp
    ldrb index, [L, #VERTEX_G]
    ldrsh width, [R, #VERTEX_X]

    // 2 bytes alignment (VRAM write requirement)
.align_left:
    tst pixel, #1
      beq .align_right
    ldrb tmp, [pixel, #-1]!
    orr tmp, index, lsl #8
    strh tmp, [pixel], #2
    subs width, #1
      beq .scanline_end

.align_right:
    tst width, #1
      beq .scanline_block_2px
    ldrb tmp, [pixel, width]
    subs width, #1
    orr tmp, index, tmp, lsl #8
    strh tmp, [pixel, width]
      beq .scanline_end

.scanline_block_2px:
    strb index, [pixel], #2     // VRAM one as two bytes write hack
    subs width, #2
      bne .scanline_block_2px

.scanline_end:
    mov pc, lr
