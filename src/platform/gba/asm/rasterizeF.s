#include "common_asm.inc"

pixel   .req r0   // arg
L       .req r1   // arg
color   .req r2   // arg
index   .req r3
Lh      .req r4
Rh      .req r5
Lx      .req r6

// FIQ regs
Rx      .req r8
Ldx     .req r9
Rdx     .req r10
N       .req r11
tmp     .req r12
pair    .req r13
width   .req r14

R       .req color
h       .req N
Rxy     .req tmp
Ry2     .req Rh
Lxy     .req tmp
Ly2     .req Lh
LMAP    .req Lx
ptr     .req tmp

.global rasterizeF_asm
rasterizeF_asm:
    stmfd sp!, {r4-r6}
    fiq_on

    add LMAP, color, #LMAP_ADDR
    ldrb tmp, [L, #VERTEX_G]
    ldrb index, [LMAP, tmp, lsl #8] // index = lightmap[color + L->v.g * 256]

    mov R, L

    mov Rh, #0                      // Rh = 0

    .calc_left_start:
        ldr Lxy, [L, #VERTEX_X]     // Lxy = (L->v.y << 16) | (L->v.x)
        ldrsb N, [L, #VERTEX_PREV]  // N = L + L->prev
        add L, L, N, lsl #VERTEX_SIZEOF_SHIFT
        ldrsh Ly2, [L, #VERTEX_Y]   // Ly2 = N->v.y

        subs Lh, Ly2, Lxy, asr #16  // Lh = N->v.y - L->v.y
          blt .exit                 // if (Lh < 0) return
          beq .calc_left_start

        lsl Lx, Lxy, #16            // Lx = L->v.x << 16
        cmp Lh, #1                  // if (Lh == 1) skip Ldx calc
          beq .calc_left_end

        divLUT tmp, Lh              // tmp = FixedInvU(Lh)

        ldrsh Ldx, [L, #VERTEX_X]
        subs Ldx, Lx, asr #16
        mulne Ldx, tmp, Ldx         // Ldx = tmp * (N->v.x - L->v.x)
    .calc_left_end:

    cmp Rh, #0
      bne .calc_right_end       // if (Rh != 0) end with right

    .calc_right_start:
        ldr Rxy, [R, #VERTEX_X]     // Rxy = (R->v.y << 16) | (R->v.x)
        ldrsb N, [R, #VERTEX_NEXT]  // N = R + R->next
        add R, R, N, lsl #VERTEX_SIZEOF_SHIFT
        ldrsh Ry2, [R, #VERTEX_Y]   // Ry2 = N->v.y

        subs Rh, Ry2, Rxy, asr #16  // Rh = N->v.y - R->v.y
          blt .exit                 // if (Rh < 0) return
          beq .calc_right_start

        lsl Rx, Rxy, #16            // Rx = R->v.x << 16
        cmp Rh, #1                  // if (Rh == 1) skip Rdx calc
          beq .calc_right_end

        divLUT tmp, Rh              // tmp = FixedInvU(Rh)

        ldrsh Rdx, [R, #VERTEX_X]
        subs Rdx, Rx, asr #16
        mulne Rdx, tmp, Rdx         // Rdx = tmp * (N->v.x - Rx)
    .calc_right_end:

    cmp Rh, Lh              // if (Rh < Lh)
      movlt h, Rh           //      h = Rh
      movge h, Lh           // else h = Lh
    sub Lh, h               // Lh -= h
    sub Rh, h               // Rh -= h

.scanline_start:
    asr tmp, Lx, #16                // x1 = (Lx >> 16)
    rsbs width, tmp, Rx, asr #16    // width = (Rx >> 16) - x1
      ble .scanline_end             // if (width <= 0) go next scanline

    add ptr, pixel, tmp             // ptr = pixel + x1

    // 2 bytes alignment (VRAM write requirement)
.align_left:
    tst ptr, #1                 // if (ptr & 1)
      beq .align_right
    ldrb pair, [ptr, #-1]!      //   *ptr++ = (*ptr & 0x00FF) | (index << 8)
    orr pair, index, lsl #8
    strh pair, [ptr], #2
    subs width, #1              // width--
      beq .scanline_end         // if (width == 0)

.align_right:
    tst width, #1
      beq .scanline_block_2px
    ldrb pair, [ptr, width]
    subs width, #1              // width--
    orr pair, index, pair, lsl #8
    strh pair, [ptr, width]
      beq .scanline_end         // if (width == 0)

.scanline_block_2px:
    strb index, [ptr], #2       // VRAM one as two bytes write hack
    subs width, #2
      bne .scanline_block_2px

.scanline_end:
    add Lx, Ldx                 // Lx += Ldx
    add Rx, Rdx                 // Rx += Rdx
    add pixel, #FRAME_WIDTH     // pixel += FRAME_WIDTH (240)

    subs h, #1
      bne .scanline_start

    cmp Lh, #0
      bne .calc_right_start
      b .calc_left_start

.exit:
    fiq_off
    ldmfd sp!, {r4-r6}
    bx lr
