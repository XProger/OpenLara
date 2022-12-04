#include "common_asm.inc"

pixel   .req r0   // arg
L       .req r1   // arg
R       .req r2   // arg
LMAP    .req r3
Lh      .req r4
Rh      .req r5
Lx      .req r6
Rx      .req r7
// FIQ regs
Ldx     .req r8
Rdx     .req r9
N       .req r10
tmp     .req r11
pair    .req r12
width   .req r13
indexA  .req r14

h       .req N
Rxy     .req tmp
Ry2     .req Rh
Lxy     .req tmp
Ly2     .req Lh
indexB  .req pair

.global rasterizeS_asm
rasterizeS_asm:
    stmfd sp!, {r4-r7}
    fiq_on

    mov LMAP, #LMAP_ADDR
    add LMAP, #0x1A00

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
        mulne Ldx, tmp, Ldx         // Ldx = tmp * (N->v.x - Lx)
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

    add tmp, pixel, tmp             // tmp = pixel + x1

    // 2 bytes alignment (VRAM write requirement)
.align_left:
    tst tmp, #1
      beq .align_right

    ldrh pair, [tmp, #-1]!
    ldrb indexA, [LMAP, pair, lsr #8]
    and pair, #0xFF
    orr pair, indexA, lsl #8
    strh pair, [tmp], #2

    subs width, #1              // width--
      beq .scanline_end

.align_right:
    tst width, #1
      beq .scanline
    subs width, #1              // width--
    ldrh pair, [tmp, width]
    and indexA, pair, #0xFF
    ldrb indexA, [LMAP, indexA]
    and pair, #0xFF00
    orr pair, indexA
    strh pair, [tmp, width]
      beq .scanline_end         // width == 0

.scanline:
    ldrh pair, [tmp]
    ldrb indexA, [LMAP, pair, lsr #8]
    and pair, #0xFF
    ldrb indexB, [LMAP, pair]
    orr pair, indexB, indexA, lsl #8
    strh pair, [tmp], #2

    subs width, #2
      bne .scanline

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
    ldmfd sp!, {r4-r7}
    bx lr
