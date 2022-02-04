#include "common_asm.inc"

pixel   .req r0
L       .req r1
R       .req r2
index   .req r3
Lh      .req r4
Rh      .req r5
Lx      .req r6
Rx      .req r7
Ldx     .req r8
Rdx     .req r9
N       .req r10
tmp     .req r11
DIVLUT  .req r12
width   .req lr

h       .req N
Rxy     .req tmp
Ry2     .req Rh
Lxy     .req tmp
Ly2     .req Lh
LMAP    .req Lx
pair    .req DIVLUT
blocks  .req DIVLUT

.global rasterizeF_asm
rasterizeF_asm:
    stmfd sp!, {r4-r11, lr}

    mov LMAP, #LMAP_ADDR

    // TODO use ldrh, swap g and clip
    ldrb tmp, [L, #VERTEX_G]
    ldrb index, [L, #VERTEX_CLIP]
    orr tmp, index, tmp, lsl #8     // tmp = index | (L->v.g << 8)
    ldrb index, [LMAP, tmp]         // tmp = lightmap[tmp]

    mov Lh, #0                      // Lh = 0
    mov Rh, #0                      // Rh = 0

.loop:
    mov DIVLUT, #DIVLUT_ADDR

    .calc_left_start:
        cmp Lh, #0
          bne .calc_left_end        // if (Lh != 0) end with left
        ldr N, [L, #VERTEX_PREV]    // N = L->prev
        ldr Lxy, [L, #VERTEX_X]     // Lxy = (L->v.y << 16) | (L->v.x)
        ldrsh Ly2, [N, #VERTEX_Y]   // Ly2 = N->v.y
        subs Lh, Ly2, Lxy, asr #16  // Lh = N->v.y - L->v.y
          blt .exit                 // if (Lh < 0) return
        lsl Lx, Lxy, #16            // Lx = L->v.x << 16
        mov L, N                    // L = N
        cmp Lh, #1                  // if (Lh <= 1) skip Ldx calc
          ble .calc_left_start
        lsl tmp, Lh, #1
        ldrh tmp, [DIVLUT, tmp]     // tmp = FixedInvU(Lh)

        ldrsh Ldx, [L, #VERTEX_X]
        sub Ldx, Lx, asr #16
        mul Ldx, tmp                // Ldx = tmp * (N->v.x - L->v.x)
    .calc_left_end:

    .calc_right_start:
        cmp Rh, #0
          bne .calc_right_end       // if (Rh != 0) end with right
        ldr N, [R, #VERTEX_NEXT]    // N = R->next
        ldr Rxy, [R, #VERTEX_X]     // Rxy = (R->v.y << 16) | (R->v.x)
        ldrsh Ry2, [N, #VERTEX_Y]   // Ry2 = N->v.y
        subs Rh, Ry2, Rxy, asr #16  // Rh = N->v.y - R->v.y
          blt .exit                 // if (Rh < 0) return
        lsl Rx, Rxy, #16            // Rx = R->v.x << 16
        mov R, N                    // R = N
        cmp Rh, #1                  // if (Rh <= 1) skip Rdx calc
          ble .calc_right_start
        lsl tmp, Rh, #1
        ldrh tmp, [DIVLUT, tmp]     // tmp = FixedInvU(Rh)

        ldrsh Rdx, [R, #VERTEX_X]
        sub Rdx, Rx, asr #16
        mul Rdx, tmp                // Rdx = tmp * (N->v.x - Rx)
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
    tst tmp, #1                 // if (tmp & 1)
      beq .align_right
    ldrb pair, [tmp, #-1]!      //   *tmp++ = (*tmp & 0x00FF) | (index << 8)
    orr pair, index, lsl #8
    strh pair, [tmp], #2
    subs width, #1              // width--
      beq .scanline_end         // if (width == 0)

.align_right:
    tst width, #1
      beq .scanline_block_2px
    ldrb pair, [tmp, width]
    subs width, #1              // width--
    orr pair, index, pair, lsl #8
    strh pair, [tmp, width]
      beq .scanline_end         // if (width == 0)

.scanline_block_2px:
    strb index, [tmp], #2       // VRAM one as two bytes write hack
    subs width, #2
      bne .scanline_block_2px

.scanline_end:
    add Lx, Ldx                     // Lx += Ldx
    add Rx, Rdx                     // Rx += Rdx
    add pixel, #FRAME_WIDTH         // pixel += FRAME_WIDTH (240)

    subs h, #1
      bne .scanline_start
    b .loop

.exit:
    ldmfd sp!, {r4-r11, pc}