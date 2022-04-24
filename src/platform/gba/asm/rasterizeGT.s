#include "common_asm.inc"

arg_pixel .req r0   // arg
arg_L     .req r1   // arg
arg_R     .req r2   // arg

N       .req r0
tmp     .req r1
Lx      .req r2
Rx      .req r3
Lg      .req r4
Rg      .req r5
Lt      .req r6
Rt      .req r7

L       .req r8
R       .req r9
Lh      .req r10
Rh      .req r11
pixel   .req r12
TILE    .req lr

// FIQ regs
Ldx     .req r8
Rdx     .req r9
Ldg     .req r10
Rdg     .req r11
Ldt     .req r12
Rdt     .req r13

h       .req N

LMAP    .req tmp

indexA  .req Lh
indexB  .req tmp

Rxy     .req tmp
Ry2     .req Rh
Lxy     .req tmp
Ly2     .req Lh

inv     .req Lh

ptr     .req Lx
width   .req Rh

g       .req Lg
dgdx    .req L

t       .req Lt
dtdx    .req R
dtmp    .req L

Ltmp    .req N
Rtmp    .req N

SP_TILE = 0
SP_SIZE = 4

G_EXTRA = 5   // extra bits of precision for gouraud shading (8 + G_EXTRA)

.macro PUT_PIXELS
    tex indexA, t

    mov LMAP, g, lsr #(8 + G_EXTRA)
    ldrb indexA, [indexA, LMAP, lsl #8]

    strb indexA, [ptr], #2  // writing a byte to GBA VRAM will write a half word for free

    add g, dgdx, lsl #1
    add t, dtdx, lsl #1
.endm

.global rasterizeGT_asm
rasterizeGT_asm:
    ldr r3, =gTile
    ldr r3, [r3]
    stmfd sp!, {r3-r11, lr}

    mov pixel, arg_pixel
    mov L, arg_L
    mov R, arg_R

    mov Lh, #0                      // Lh = 0
    mov Rh, #0                      // Rh = 0

.loop:

    cmp Lh, #0
      bne .calc_left_end        // if (Lh != 0) end with left

    .calc_left_start:
        ldrsb N, [L, #VERTEX_PREV]  // N = L + L->prev
        add N, L, N, lsl #VERTEX_SIZEOF_SHIFT
        ldr Lxy, [L, #VERTEX_X]     // Lxy = (L->v.y << 16) | (L->v.x)
        ldrsh Ly2, [N, #VERTEX_Y]   // Ly2 = N->v.y

        subs Lh, Ly2, Lxy, asr #16  // Lh = N->v.y - L->v.y
          blt .exit                 // if (Lh < 0) return
          ldrneb Lg, [L, #VERTEX_G] // Lg = L->v.g
          ldrne Lt, [L, #VERTEX_T]  // Lt = L->t
          mov L, N                  // L = N
          beq .calc_left_start

        lsl Lx, Lxy, #16            // Lx = L->v.x << 16
        lsl Lg, #(8 + G_EXTRA)      // Lg <<= 8 + G_EXTRA
        cmp Lh, #1                  // if (Lh <= 1) skip Ldx calc
          beq .calc_left_end

        divLUT tmp, Lh              // tmp = FixedInvU(Lh)

        fiq_on
        ldrsh Ldx, [N, #VERTEX_X]
        sub Ldx, Lx, asr #16
        mul Ldx, tmp                // Ldx = tmp * (N->v.x - Lx)

        ldrb Ldg, [N, #VERTEX_G]
        sub Ldg, Lg, lsr #(8 + G_EXTRA)
        mul Ldg, tmp                // Ldg = tmp * (N->v.g - Lg)
        asr Ldg, #(8 - G_EXTRA)     // (8 + G_EXTRA)-bit for fractional part

        ldr Ldt, [N, #VERTEX_T]
        sub Ldt, Lt                 // Ldt = N->v.t - Lt
        scaleUV Ldt, Ltmp, tmp
        fiq_off
    .calc_left_end:

    cmp Rh, #0
      bne .calc_right_end       // if (Rh != 0) end with right

    .calc_right_start:
        ldrsb N, [R, #VERTEX_NEXT]  // N = R + R->next
        add N, R, N, lsl #VERTEX_SIZEOF_SHIFT
        ldr Rxy, [R, #VERTEX_X]     // Rxy = (R->v.y << 16) | (R->v.x)
        ldrsh Ry2, [N, #VERTEX_Y]   // Ry2 = N->v.y

        subs Rh, Ry2, Rxy, asr #16  // Rh = N->v.y - R->v.y
          blt .exit                 // if (Rh < 0) return
          ldrneb Rg, [R, #VERTEX_G] // Rg = R->v.g
          ldrne Rt, [R, #VERTEX_T]  // Rt = R->t
          mov R, N                  // R = N
          beq .calc_right_start

        lsl Rx, Rxy, #16            // Rx = R->v.x << 16
        lsl Rg, #(8 + G_EXTRA)      // Rg <<= 8 + G_EXTRA
        cmp Rh, #1                  // if (Rh <= 1) skip Rdx calc
          beq .calc_right_end

        divLUT tmp, Rh              // tmp = FixedInvU(Rh)

        fiq_on
        ldrsh Rdx, [N, #VERTEX_X]
        sub Rdx, Rx, asr #16
        mul Rdx, tmp                // Rdx = tmp * (N->v.x - Rx)

        ldrb Rdg, [N, #VERTEX_G]
        sub Rdg, Rg, lsr #(8 + G_EXTRA)
        mul Rdg, tmp                // Rdg = tmp * (N->v.g - Rg)
        asr Rdg, #(8 - G_EXTRA)     // (8 + G_EXTRA)-bit for fractional part

        ldr Rdt, [N, #VERTEX_T]
        sub Rdt, Rt                 // Rdt = N->v.t - Rt
        scaleUV Rdt, Rtmp, tmp
        fiq_off
    .calc_right_end:

    orr Lg, #(LMAP_ADDR << G_EXTRA)
    orr Rg, #(LMAP_ADDR << G_EXTRA)

    cmp Rh, Lh              // if (Rh < Lh)
      movlt h, Rh           //      h = Rh
      movge h, Lh           // else h = Lh
    sub Lh, h               // Lh -= h
    sub Rh, h               // Rh -= h

    ldr TILE, [sp, #SP_TILE]

    stmfd sp!, {L, R, Lh, Rh}

.scanline_start:
    asr Lh, Lx, #16                 // x1 = (Lx >> 16)
    rsbs width, Lh, Rx, asr #16     // width = (Rx >> 16) - x1
      ble .scanline_end_fast        // if (width <= 0) go next scanline

    stmfd sp!, {Lx, Lg, Lt}

    add ptr, pixel, Lx, asr #16     // ptr = pixel + x1

    divLUT inv, width               // inv = FixedInvU(width)

    sub dtdx, Rt, Lt                // dtdx = Rt - Lt
    scaleUV dtdx, dtmp, inv
    // t == Lt (alias)

    sub dgdx, Rg, Lg                // dgdx = Rg - Lg
    mul dgdx, inv                   // dgdx *= FixedInvU(width)
    asr dgdx, #16                   // dgdx >>= 16
    // g == Lg (alias)

    // 2 bytes alignment (VRAM write requirement)
.align_left:
    tst ptr, #1                   // if (ptr & 1)
      beq .align_right

    tex indexA, t
    mov LMAP, g, lsr #(8 + G_EXTRA)
    ldrb indexA, [indexA, LMAP, lsl #8]

    ldrb indexB, [ptr, #-1]!      // read pal index from VRAM (byte)
    orr indexB, indexA, lsl #8
    strh indexB, [ptr], #2

    subs width, #1              // width--
      beq .scanline_end         // if (width == 0)

    add g, dgdx
    add t, dtdx

.align_right:
    tst width, #1
      beq .align_block_4px

    tex indexA, Rt
    mov LMAP, Rg, lsr #(8 + G_EXTRA)
    ldrb indexA, [indexA, LMAP, lsl #8]

    ldrb indexB, [ptr, width]
    subs width, #1              // width--
    orr indexB, indexA, indexB, lsl #8
    strh indexB, [ptr, width]

      beq .scanline_end         // if (width == 0)

.align_block_4px:
    tst width, #2
      beq .align_block_8px

    PUT_PIXELS

    subs width, #2
      beq .scanline_end

.align_block_8px:
    tst width, #4
      beq .scanline_block_8px

    PUT_PIXELS
    PUT_PIXELS

    subs width, #4
      beq .scanline_end

.scanline_block_8px:
    PUT_PIXELS
    PUT_PIXELS
    PUT_PIXELS
    PUT_PIXELS

    subs width, #8
      bne .scanline_block_8px

.scanline_end:
    ldmfd sp!, {Lx, Lg, Lt}

.scanline_end_fast:
    fiq_on
    add Lx, Ldx
    add Rx, Rdx
    add Lg, Ldg
    add Rg, Rdg
    add Lt, Ldt
    add Rt, Rdt
    fiq_off

    add pixel, #FRAME_WIDTH         // pixel += FRAME_WIDTH (240)

    subs h, #1
      bne .scanline_start

    ldmfd sp!, {L, R, Lh, Rh}
    b .loop

.exit:
    add sp, #SP_SIZE                // revert reserved space for [TILE]
    ldmfd sp!, {r4-r11, pc}
