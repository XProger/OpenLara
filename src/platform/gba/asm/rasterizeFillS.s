#include "common_asm.inc"

pixel   .req r0   // arg
L       .req r1   // arg
R       .req r2   // arg
p       .req r3
// FIQ regs
w       .req r8
indexA  .req r9
indexB  .req r10
shade   .req r11

width   .req L
height  .req R
LMAP    .req shade

.global rasterizeFillS_asm
rasterizeFillS_asm:
    fiq_on

    add R, #VERTEX_SIZEOF
    ldrsh p, [L, #VERTEX_X]
    add pixel, p
    ldrb shade, [L, #VERTEX_G]
    ldrsh width, [R, #VERTEX_X]
    ldrsh height, [R, #VERTEX_Y]
    lsl shade, #8
    add LMAP, shade, #LMAP_ADDR

.loop:
    mov p, pixel
    mov w, width

    // 2 bytes alignment (VRAM write requirement)
.align_left:
    tst p, #1
      beq .align_right
    ldrh indexA, [p, #-1]!
    ldrb indexB, [LMAP, indexA, lsr #8]
    and indexA, indexA, #0xFF
    orr indexA, indexB, lsl #8
    strh indexA, [p], #2
    subs w, #1
      beq .scanline_end

.align_right:
    tst w, #1
      beq .scanline_block_2px
    subs w, #1
    ldrh indexA, [p, w]
    and indexB, indexA, #0xFF
    ldrb indexB, [LMAP, indexB]
    and indexA, indexA, #0xFF00
    orr indexA, indexA, indexB
    strh indexA, [p, w]
      beq .scanline_end

.scanline_block_2px:
    ldrh indexA, [p]
    and indexB, indexA, #0xFF
    ldrb indexA, [LMAP, indexA, lsr #8]
    ldrb indexB, [LMAP, indexB]
    orr indexA, indexB, indexA, lsl #8
    strh indexA, [p], #2
    subs w, #2
      bne .scanline_block_2px

.scanline_end:
    add pixel, #FRAME_WIDTH
    subs height, #1
      bne .loop

    fiq_off
    bx lr
