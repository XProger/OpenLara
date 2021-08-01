#include "rasterizer.inc"

pixel   .req r0
L       .req r1
R       .req r2
index   .req r3
Lh      .req r4
Rh      .req r5
Lx      .req ip
Rx      .req lr
Ldx     .req r6
Rdx     .req r7
N       .req r8
tmp     .req r9
DIVLUT  .req r10
width   .req r11
h       .req N
Ry1     .req tmp
Ry2     .req Rh
Ly1     .req tmp
Ly2     .req Lh
LMAP    .req Lx
pair    .req DIVLUT
blocks  .req DIVLUT

.global rasterizeF_mode4_asm
rasterizeF_mode4_asm:
    stmfd sp!, {r4,r5,r6,r7,r8,r9,r10,r11,lr}

    mov LMAP, #LMAP_ADDR

    ldrb tmp, [L, #VERTEX_G]
    orr tmp, index, tmp, lsl #8     // tmp = index | (L->v.g << 8)
    ldrb index, [LMAP, tmp]         // tmp = lightmap[tmp]
    orr index, index, lsl #8

    mov Lh, #0                      // Lh = 0
    mov Rh, #0                      // Rh = 0

.loop:
    mov DIVLUT, #DIVLUT_ADDR

    .calc_left_start:
        cmp Lh, #0
          bne .calc_left_end        // if (Lh != 0) end with left
        ldr N, [L, #VERTEX_PREV]    // N = L->prev
        ldrsh Ly1, [L, #VERTEX_Y]   // Ly1 = L->v.y
        ldrsh Ly2, [N, #VERTEX_Y]   // Ly2 = N->v.y
        subs Lh, Ly2, Ly1           // Lh = Ly2 - Ly1
          blt .exit                 // if (Lh < 0) return
        ldrsh Lx, [L, #VERTEX_X]    // Lx = L->v.x
        cmp Lh, #1                  // if (Lh <= 1) skip Ldx calc
          ble .skip_left_dx
        lsl tmp, Lh, #1
        ldrh tmp, [DIVLUT, tmp]     // tmp = FixedInvU(Lh)

        ldrsh Ldx, [N, #VERTEX_X]
        sub Ldx, Lx
        mul Ldx, tmp                // Ldx = tmp * (N->v.x - Lx)

        .skip_left_dx:
        lsl Lx, #16                 // Lx <<= 16
        mov L, N                    // L = N
        b .calc_left_start
    .calc_left_end:

    .calc_right_start:
        cmp Rh, #0
          bne .calc_right_end       // if (Rh != 0) end with right
        ldr N, [R, #VERTEX_NEXT]    // N = R->next
        ldrsh Ry1, [R, #VERTEX_Y]   // Ry1 = R->v.y
        ldrsh Ry2, [N, #VERTEX_Y]   // Ry2 = N->v.y
        subs Rh, Ry2, Ry1           // Rh = Ry2 - Ry1
          blt .exit                 // if (Rh < 0) return
        ldrsh Rx, [R, #VERTEX_X]    // Rx = R->v.x
        cmp Rh, #1                  // if (Rh <= 1) skip Rdx calc
          ble .skip_right_dx
        lsl tmp, Rh, #1
        ldrh tmp, [DIVLUT, tmp]     // tmp = FixedInvU(Rh)

        ldrsh Rdx, [N, #VERTEX_X]
        sub Rdx, Rx
        mul Rdx, tmp                // Rdx = tmp * (N->v.x - Rx)

        .skip_right_dx:
        lsl Rx, #16                 // Rx <<= 16
        mov R, N                    // R = N
        b .calc_right_start
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
      beq .align_block
    ldrb pair, [tmp, width]
    subs width, #1              // width--
    lsl pair, #8
    orr pair, index, lsr #8
    strh pair, [tmp, width]
      beq .scanline_end         // if (width == 0)

.align_block:
    // 8px alignment
    lsr blocks, width, #1
    and blocks, blocks, #3          // blocks = (width / 2) % 4
    rsbs blocks, blocks, #2
    add pc, blocks, lsl #2          // pc += (2 - blocks) * 4

    strh index, [tmp], #2           // if (width % 4) == 3
    strh index, [tmp], #2           // if (width % 4) == 2
    strh index, [tmp], #2           // if (width % 4) == 1

    lsrs blocks, width, #3          // blocks = (width / 2) / 4
      beq .scanline_end             // if (blocks == 0) no 8px blocks

.scanline_block_8px:
    strh index, [tmp], #2
    strh index, [tmp], #2
    strh index, [tmp], #2
    strh index, [tmp], #2
    subs blocks, #1
      bne .scanline_block_8px

.scanline_end:
    add Lx, Ldx                     // Lx += Ldx
    add Rx, Rdx                     // Rx += Rdx
    add pixel, #VRAM_STRIDE         // pixel += FRAME_WIDTH (240)

    subs h, #1
      bne .scanline_start
    b .loop

.exit:
    ldmfd sp!, {r4,r5,r6,r7,r8,r9,r10,r11,pc}