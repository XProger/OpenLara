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
spFIQ   .req r14

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
dgdx    .req R

t       .req Lt
dtdx    .req L
dtmp    .req tmp
dtmp2   .req R

Ltmp    .req spFIQ
Rtmp    .req spFIQ
Ltmp2   .req N
Rtmp2   .req N

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
    stmfd sp!, {r4-r11, lr}

    ldr TILE, =gTile
    ldr TILE, [TILE]

    mov pixel, arg_pixel
    mov L, arg_L
    mov R, arg_R

    mov Rh, #0                      // Rh = 0

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
        subs Ldx, Lx, asr #16
        mulne Ldx, tmp, Ldx         // Ldx = tmp * (N->v.x - Lx)

        ldrb Ldg, [N, #VERTEX_G]
        subs Ldg, Lg, lsr #(8 + G_EXTRA)
        mulne Ldg, tmp, Ldg         // Ldg = tmp * (N->v.g - Lg)
        asr Ldg, #(8 - G_EXTRA)     // (8 + G_EXTRA)-bit for fractional part

        ldr Ldt, [N, #VERTEX_T]
        subs Ldt, Lt                // Ldt = N->v.t - Lt
        scaleUV Ldt, Ltmp, Ltmp2, tmp
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
        subs Rdx, Rx, asr #16
        mulne Rdx, tmp, Rdx         // Rdx = tmp * (N->v.x - Rx)

        ldrb Rdg, [N, #VERTEX_G]
        subs Rdg, Rg, lsr #(8 + G_EXTRA)
        mulne Rdg, tmp, Rdg         // Rdg = tmp * (N->v.g - Rg)
        asr Rdg, #(8 - G_EXTRA)     // (8 + G_EXTRA)-bit for fractional part

        ldr Rdt, [N, #VERTEX_T]
        subs Rdt, Rt                // Rdt = N->v.t - Rt
        scaleUV Rdt, Rtmp, Rtmp2, tmp
        fiq_off
    .calc_right_end:

    orr Lg, #(LMAP_ADDR << G_EXTRA)
    orr Rg, #(LMAP_ADDR << G_EXTRA)

    cmp Rh, Lh              // if (Rh < Lh)
      movlt h, Rh           //      h = Rh
      movge h, Lh           // else h = Lh
    sub Lh, h               // Lh -= h
    sub Rh, h               // Rh -= h

    stmfd sp!, {L, R, Lh, Rh}

.scanline_start:
    asr Lh, Lx, #16                 // x1 = (Lx >> 16)
    rsbs width, Lh, Rx, asr #16     // width = (Rx >> 16) - x1
      ble .scanline_end_fast        // if (width <= 0) go next scanline

    stmfd sp!, {Lx, Lg, Lt}

    add ptr, pixel, Lx, asr #16     // ptr = pixel + x1

    divLUT inv, width               // inv = FixedInvU(width)

    subs dtdx, Rt, Lt               // dtdx = Rt - Lt
    scaleUV dtdx, dtmp, dtmp2, inv
    // t == Lt (alias)

    subs dgdx, Rg, Lg               // dgdx = Rg - Lg
    mulne dgdx, inv                 // dgdx *= FixedInvU(width)
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

    add pixel, #FRAME_WIDTH     // pixel += FRAME_WIDTH (240)

    subs h, #1
      bne .scanline_start

    ldmfd sp!, {L, R, Lh, Rh}

    cmp Lh, #0
      bne .calc_right_start
      b .calc_left_start

.exit:
    ldmfd sp!, {r4-r11, pc}
