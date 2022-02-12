#include "common_asm.inc"

pixel   .req r0
L       .req r1
R       .req r2
LMAP    .req r3

TILE    .req r4
tmp     .req r5
N       .req r6
Lh      .req r7
Rh      .req r8

Lx      .req r9
Rx      .req r10
Lt      .req r11
Rt      .req r12
h       .req lr

ptr     .req tmp

Ldx     .req h
Rdx     .req h

Ldt     .req h
Rdt     .req h

indexA  .req Lh
indexB  .req Rh
Rxy     .req tmp
Ry2     .req Rh
Lxy     .req tmp
Ly2     .req Lh

inv     .req Lh
width   .req N
t       .req L
dtdx    .req R

duv     .req R
du      .req L
dv      .req R

Ldu     .req N
Ldv     .req h

Rdu     .req N
Rdv     .req h

Rti     .req indexB

sLdx    .req tmp
sLdt    .req N
sRdx    .req Lh
sRdt    .req Rh

SP_LDX = 0
SP_LDT = 4
SP_RDX = 8
SP_RDT = 12
SP_L   = 16
SP_R   = 20
SP_LH  = 24
SP_RH  = 28
SP_SIZE = 32

.macro PUT_PIXELS
    tex indexA, t
    add t, dtdx, lsl #1
    cmp indexA, #0
    ldrneb indexA, [LMAP, indexA]
    strneb indexA, [ptr]
    add ptr, #2
.endm

.global rasterizeFTA_asm
rasterizeFTA_asm:
    stmfd sp!, {r4-r11, lr}
    sub sp, #SP_SIZE                // reserve stack space for [Ldx, Ldt, Rdx, Rdt]

    mov LMAP, #LMAP_ADDR
    ldrb tmp, [L, #VERTEX_G]
    add LMAP, tmp, lsl #8           // tmp = (L->v.g << 8)

    ldr TILE, =gTile
    ldr TILE, [TILE]

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
          ldrne Lt, [L, #VERTEX_T]  // Lt = L->t
          mov L, N                  // L = N
          beq .calc_left_start

        lsl Lx, Lxy, #16            // Lx = L->v.x << 16
        cmp Lh, #1                  // if (Lh <= 1) skip Ldx calc
          beq .calc_left_end

        divLUT tmp, Lh              // tmp = FixedInvU(Lh)

        ldrsh Ldx, [L, #VERTEX_X]
        sub Ldx, Lx, asr #16
        mul Ldx, tmp                // Ldx = tmp * (N->v.x - Lx)
        str Ldx, [sp, #SP_LDX]      // store Ldx to stack

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
          ldrne Rt, [R, #VERTEX_T]  // Rt = R->t
          mov R, N                  // R = N
          beq .calc_right_start

        lsl Rx, Rxy, #16            // Rx = R->v.x << 16
        cmp Rh, #1                  // if (Rh <= 1) skip Rdx calc
          beq .calc_right_end

        divLUT tmp, Rh              // tmp = FixedInvU(Rh)

        ldrsh Rdx, [R, #VERTEX_X]
        sub Rdx, Rx, asr #16
        mul Rdx, tmp                // Rdx = tmp * (N->v.x - Rx)
        str Rdx, [sp, #SP_RDX]      // store Rdx to stack

        ldr Rdt, [R, #VERTEX_T]
        sub Rdt, Rt                 // Rdt = N->v.t - Rt
        scaleUV Rdt, Rdu, Rdv, tmp
        str Rdt, [sp, #SP_RDT]      // store Rdt to stack
    .calc_right_end:

    cmp Rh, Lh              // if (Rh < Lh)
      movlt h, Rh           //      h = Rh
      movge h, Lh           // else h = Lh
    sub Lh, h               // Lh -= h
    sub Rh, h               // Rh -= h

    add tmp, sp, #SP_L
    stmia tmp, {L, R, Lh, Rh}

.scanline_start:
    asr tmp, Lx, #16                // x1 = (Lx >> 16)
    rsbs width, tmp, Rx, asr #16    // width = (Rx >> 16) - x1
      ble .scanline_end             // if (width <= 0) go next scanline

    add ptr, pixel, tmp             // ptr = pixel + x1

    divLUT inv, width               // inv = FixedInvU(width)

    sub dtdx, Rt, Lt                // duv = Rt - Lt
    scaleUV dtdx, du, dv, inv

    mov t, Lt                       // t = Lt

    // 2 bytes alignment (VRAM write requirement)
.align_left:
    tst ptr, #1                     // if (ptr & 1)
      beq .align_right

    tex indexA, t

    cmp indexA, #0
    ldrneb indexB, [ptr, #-1]!      // read pal index from VRAM (byte)
    ldrneb indexA, [LMAP, indexA]
    orrne indexB, indexA, lsl #8
    strneh indexB, [ptr], #2
    addeq ptr, #1
    add t, dtdx

    subs width, #1              // width--
      beq .scanline_end         // if (width == 0)

.align_right:
    tst width, #1
      beq .align_block_4px

    sub Rti, Rt, dtdx
    tex indexA, Rti

    cmp indexA, #0
    ldrneb indexA, [LMAP, indexA]
    ldrneb indexB, [ptr, width]
    orrne indexB, indexA, indexB, lsl #8
    addne indexA, ptr, width
    strneh indexB, [indexA, #-1]

    subs width, #1              // width--
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
    ldmia sp, {sLdx, sLdt, sRdx, sRdt}
    add Lx, sLdx
    add Lt, sLdt
    add Rx, sRdx
    add Rt, sRdt

    add pixel, #FRAME_WIDTH         // pixel += FRAME_WIDTH (240)

    subs h, #1
      bne .scanline_start

    add tmp, sp, #SP_L
    ldmia tmp, {L, R, Lh, Rh}
    b .loop

.exit:
    add sp, #SP_SIZE            // revert reserved space for [Ldx, Ldt, Rdx, Rdt]
    ldmfd sp!, {r4-r11, pc}