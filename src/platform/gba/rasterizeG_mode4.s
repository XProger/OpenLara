#include "common_asm.inc"

pixel   .req r0
L       .req r1
R       .req r2
index   .req r3
DIVLUT  .req r4
tmp     .req r5
N       .req r6
Lh      .req r7
Rh      .req r8
Lx      .req r9
Rx      .req r10
Lg      .req r11
Rg      .req r12
h       .req lr
Ldx     .req h
Rdx     .req Ldx
Ldg     .req Ldx
Rdg     .req Ldx
color   .req N
Ry1     .req tmp
Ry2     .req Rh
Ly1     .req tmp
Ly2     .req Lh
LMAP    .req index
pair    .req Lh
width   .req Rh
g       .req L
dgdx    .req R

sLdx    .req L
sLdg    .req R
sRdx    .req Lh
sRdg    .req Rh

SP_LDX = 0
SP_LDG = 4
SP_RDX = 8
SP_RDG = 12

.macro PUT_PIXELS
    lsr color, g, #16
    ldrb color, [LMAP, color, lsl #8]
    orr color, color, lsl #8
    strh color, [tmp], #2
    add g, dgdx
.endm

.global rasterizeG_mode4_asm
rasterizeG_mode4_asm:
    stmfd sp!, {r4-r11, lr}
    sub sp, #16 // reserve stack space for [Ldx, Ldg, Rdx, Rdg]

    mov tmp, #LMAP_ADDR
    add LMAP, index, tmp  // LMAP = lightmap + index
    mov DIVLUT, #DIVLUT_ADDR

    mov Lh, #0            // Lh = 0
    mov Rh, #0            // Rh = 0

.loop:
    .calc_left_start:
        cmp Lh, #0
          bne .calc_left_end        // if (Lh != 0) end with left
        ldr N, [L, #VERTEX_PREV]    // N = L->prev
        ldrsh Ly1, [L, #VERTEX_Y]   // Ly1 = L->v.y
        ldrsh Ly2, [N, #VERTEX_Y]   // Ly2 = N->v.y
        subs Lh, Ly2, Ly1           // Lh = Ly2 - Ly1
          blt .exit                 // if (Lh < 0) return
        ldrsh Lx, [L, #VERTEX_X]    // Lx = L->v.x
        ldrb Lg, [L, #VERTEX_G]     // Lg = L->v.g
        cmp Lh, #1                  // if (Lh <= 1) skip Ldx calc
          ble .skip_left_dx
        lsl tmp, Lh, #1
        ldrh tmp, [DIVLUT, tmp]     // tmp = FixedInvU(Lh)

        ldrsh Ldx, [N, #VERTEX_X]
        sub Ldx, Lx
        mul Ldx, tmp                // Ldx = tmp * (N->v.x - Lx)
        str Ldx, [sp, #SP_LDX]      // store Ldx to stack

        ldrb Ldg, [N, #VERTEX_G]
        sub Ldg, Lg
        mul Ldg, tmp                // Ldg = tmp * (N->v.g - Lg)
        str Ldg, [sp, #SP_LDG]      // store Ldg to stack

        .skip_left_dx:
        lsl Lx, #16                 // Lx <<= 16
        lsl Lg, #16                 // Lg <<= 16
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
        ldrb Rg, [R, #VERTEX_G]     // Rg = R->v.g
        cmp Rh, #1                  // if (Rh <= 1) skip Rdx calc
          ble .skip_right_dx
        lsl tmp, Rh, #1
        ldrh tmp, [DIVLUT, tmp]     // tmp = FixedInvU(Rh)

        ldrsh Rdx, [N, #VERTEX_X]
        sub Rdx, Rx
        mul Rdx, tmp                // Rdx = tmp * (N->v.x - Rx)
        str Rdx, [sp, #SP_RDX]      // store Rdx to stack

        ldrb Rdg, [N, #VERTEX_G]
        sub Rdg, Rg
        mul Rdg, tmp                // Rdg = tmp * (N->v.g - Rg)
        str Rdg, [sp, #SP_RDG]      // store Rdg to stack

        .skip_right_dx:
        lsl Rx, #16                 // Rx <<= 16
        lsl Rg, #16                 // Rg <<= 16
        mov R, N                    // R = N
        b .calc_right_start
    .calc_right_end:

    cmp Rh, Lh              // if (Rh < Lh)
      movlt h, Rh           //      h = Rh
      movge h, Lh           // else h = Lh
    sub Lh, h               // Lh -= h
    sub Rh, h               // Rh -= h

    stmfd sp!, {L,R,Lh,Rh}  // sp-16

.scanline_start:
    asr tmp, Lx, #16                // x1 = (Lx >> 16)
    rsbs width, tmp, Rx, asr #16    // width = (Rx >> 16) - x1
      ble .scanline_end             // if (width <= 0) go next scanline

    sub dgdx, Rg, Lg
    asr dgdx, #5                    // dgdx = (Rg - Lg) >> 5
    lsl g, width, #1
    ldrh g, [DIVLUT, g]
    mul dgdx, g                     // dgdx *= FixedInvU(width)
    asr dgdx, #10                   // dgdx >>= 10
    mov g, Lg                       // g = Lg

    add tmp, pixel, tmp             // tmp = pixel + x1

    // 2 bytes alignment (VRAM write requirement)
.align_left:
    tst tmp, #1                 // if (tmp & 1)
      beq .align_right
    ldrb pair, [tmp, #-1]!      // read pal index from VRAM (byte)

    lsr color, g, #16
    ldrb color, [LMAP, color, lsl #8]
    orr pair, color, lsl #8
    strh pair, [tmp], #2
    add g, dgdx, asr #1

    subs width, #1              // width--
      beq .scanline_end         // if (width == 0)

.align_right:
    tst width, #1
      beq .align_block
    ldrb pair, [tmp, width]

    subs width, #1              // width--

    lsr color, Rg, #16
    ldrb color, [LMAP, color, lsl #8]
    orr pair, color, pair, lsl #8
    strh pair, [tmp, width]

      beq .scanline_end         // if (width == 0)

.align_block:
    tst width, #2
      beq .scanline_block_4px

    PUT_PIXELS

    subs width, #2              // width -= 2
      beq .scanline_end         // if (width == 0)

.scanline_block_4px:
    PUT_PIXELS
    PUT_PIXELS

    subs width, #4
      bne .scanline_block_4px

.scanline_end:
    add tmp, sp, #16
    ldmia tmp, {sLdx, sLdg, sRdx, sRdg}
    add Lx, sLdx
    add Lg, sLdg
    add Rx, sRdx
    add Rg, sRdg

    add pixel, #VRAM_STRIDE         // pixel += FRAME_WIDTH (240)

    subs h, #1
      bne .scanline_start

    ldmfd sp!, {L,R,Lh,Rh}      // sp+16
    b .loop

.exit:
    add sp, #16                 // revert reserved space for [Ldx, Ldg, Rdx, Rdg]
    ldmfd sp!, {r4-r11, pc}