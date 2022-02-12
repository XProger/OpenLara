#include "common_asm.inc"

pixel   .req r0
L       .req r1
R       .req r2

Lh      .req r3
Rh      .req r4

Lx      .req r5
Rx      .req r6

Lg      .req r7
Rg      .req r8

Lt      .req r9
Rt      .req r10

tmp     .req r11
N       .req r12

TILE    .req lr

h       .req N

LMAP    .req tmp

Ldx     .req h
Rdx     .req h

Ldg     .req h
Rdg     .req h

Ldt     .req h
Rdt     .req h

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

duv     .req R
du      .req L
dv      .req R

Ldu     .req TILE
Ldv     .req N

Rdu     .req TILE
Rdv     .req N

Rti     .req tmp
Rgi     .req tmp

sLdx    .req L
sLdg    .req R
sLdt    .req Lh
sRdx    .req Rh
sRdg    .req tmp
sRdt    .req tmp  // not enough regs for one ldmia

SP_LDX = 0
SP_LDG = 4
SP_LDT = 8
SP_RDX = 12
SP_RDG = 16
SP_RDT = 20
SP_L   = 24
SP_R   = 28
SP_LH  = 32
SP_RH  = 36
SP_SIZE = 40
SP_TILE = SP_SIZE

.macro PUT_PIXELS
    bic LMAP, g, #255
    add g, dgdx

    tex indexA, t
    add t, dtdx, lsl #1
    cmp indexA, #0
    ldrneb indexA, [LMAP, indexA]
    strneb indexA, [ptr]
    add ptr, #2
.endm

.global rasterizeGTA_asm
rasterizeGTA_asm:
    ldr r3, =gTile
    ldr r3, [r3]

    stmfd sp!, {r3-r11, lr}
    sub sp, #SP_SIZE                // reserve stack space for [Ldx, Ldg, Ldt, Rdx, Rdg, Rdt]

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
        lsl Lg, #8                  // Lg <<= 8
        cmp Lh, #1                  // if (Lh <= 1) skip Ldx calc
          beq .calc_left_end

        divLUT tmp, Lh              // tmp = FixedInvU(Lh)

        ldrsh Ldx, [L, #VERTEX_X]
        sub Ldx, Lx, asr #16
        mul Ldx, tmp                // Ldx = tmp * (N->v.x - Lx)
        str Ldx, [sp, #SP_LDX]      // store Ldx to stack

        ldrb Ldg, [L, #VERTEX_G]
        sub Ldg, Lg, lsr #8
        mul Ldg, tmp                // Ldg = tmp * (N->v.g - Lg)
        asr Ldg, #8                 // 8-bit for fractional part
        str Ldg, [sp, #SP_LDG]      // store Ldg to stack

        ldr Ldt, [L, #VERTEX_T]
        sub Ldt, Lt                 // Ldt = N->v.t - Lt
        scaleUV Ldt, Ldu, Ldv, tmp
        str Ldt, [sp, #SP_LDT]      // store Ldt to stack
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
          ldrb Rg, [R, #VERTEX_G]   // Rg = R->v.g
          ldr Rt, [R, #VERTEX_T]    // Rt = R->t
          mov R, N                  // R = N
          beq .calc_right_start

        lsl Rx, Rxy, #16            // Rx = R->v.x << 16
        lsl Rg, #8                  // Rg <<= 8
        cmp Rh, #1                  // if (Rh <= 1) skip Rdx calc
          beq .calc_right_end

        divLUT tmp, Rh              // tmp = FixedInvU(Rh)

        ldrsh Rdx, [R, #VERTEX_X]
        sub Rdx, Rx, asr #16
        mul Rdx, tmp                // Rdx = tmp * (N->v.x - Rx)
        str Rdx, [sp, #SP_RDX]      // store Rdx to stack

        ldrb Rdg, [R, #VERTEX_G]
        sub Rdg, Rg, lsr #8
        mul Rdg, tmp                // Rdg = tmp * (N->v.g - Rg)
        asr Rdg, #8                 // 8-bit for fractional part
        str Rdg, [sp, #SP_RDG]      // store Ldg to stack

        ldr Rdt, [R, #VERTEX_T]
        sub Rdt, Rt                 // Rdt = N->v.t - Rt
        scaleUV Rdt, Rdu, Rdv, tmp
        str Rdt, [sp, #SP_RDT]      // store Rdt to stack
    .calc_right_end:

    orr Lg, #LMAP_ADDR
    orr Rg, #LMAP_ADDR

    cmp Rh, Lh              // if (Rh < Lh)
      movlt h, Rh           //      h = Rh
      movge h, Lh           // else h = Lh
    sub Lh, h               // Lh -= h
    sub Rh, h               // Rh -= h

    ldr TILE, [sp, #SP_TILE]

    add tmp, sp, #SP_L
    stmia tmp, {L, R, Lh, Rh}

.scanline_start:
    stmfd sp!, {Lx, Lg, Lt}

    asr Lx, Lx, #16                 // x1 = (Lx >> 16)
    rsbs width, Lx, Rx, asr #16     // width = (Rx >> 16) - x1
      ble .scanline_end             // if (width <= 0) go next scanline

    add ptr, pixel, Lx              // ptr = pixel + x1

    divLUT inv, width               // inv = FixedInvU(width)

    sub dtdx, Rt, Lt                // dtdx = Rt - Lt
    scaleUV dtdx, du, dv, inv
    // t == Lt (alias)

    sub dgdx, Rg, Lg                // dgdx = Rg - Lg
    mul dgdx, inv                   // dgdx *= FixedInvU(width)
    asr dgdx, #15                   // dgdx >>= 15
    // g == Lg (alias)

    // 2 bytes alignment (VRAM write requirement)
.align_left:
    tst ptr, #1                   // if (ptr & 1)
      beq .align_right

    tex indexA, t

    cmp indexA, #0
      beq .skip_left

    bic LMAP, g, #255
    ldrb indexA, [LMAP, indexA]

    ldrb indexB, [ptr, #-1]!     // read pal index from VRAM (byte)
    orr indexB, indexA, lsl #8
    strh indexB, [ptr], #1

.skip_left:
    add ptr, #1
    add t, dtdx
    add g, dgdx, asr #1

    subs width, #1              // width--
      beq .scanline_end         // if (width == 0)

.align_right:
    tst width, #1
      beq .align_block_4px

    sub Rti, Rt, dtdx
    tex indexA, Rti

    cmp indexA, #0
      subeq width, #1
      beq .skip_right

    sub Rgi, Rg, dgdx, asr #1
    bic LMAP, Rgi, #255
    lit indexA

    ldrb indexB, [ptr, width]
    sub width, #1               // width--
    orr indexB, indexA, indexB, lsl #8
    strh indexB, [ptr, width]

.skip_right:
    cmp width, #0
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

    ldmia sp, {sLdx, sLdg, sLdt, sRdx, sRdg}

    add Lx, sLdx
    add Lg, sLdg
    add Lt, sLdt
    add Rx, sRdx
    add Rg, sRdg

    ldr sRdt, [sp, #SP_RDT]
    add Rt, sRdt

    add pixel, #FRAME_WIDTH         // pixel += FRAME_WIDTH (240)

    subs h, #1
      bne .scanline_start

    add tmp, sp, #SP_L
    ldmia tmp, {L, R, Lh, Rh}
    b .loop

.exit:
    add sp, #(SP_SIZE + 4)          // revert reserved space for [Ldx, Ldg, Ldt, Rdx, Rdg, Rdt, TILE]
    ldmfd sp!, {r4-r11, pc}