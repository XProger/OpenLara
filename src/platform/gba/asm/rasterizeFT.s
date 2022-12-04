#include "common_asm.inc"

arg_pixel .req r0   // arg
arg_L     .req r1   // arg
arg_R     .req r2   // arg

N       .req r0
tmp     .req r1
Lx      .req r2
Rx      .req r3
Lt      .req r4
Rt      .req r5
t       .req r6
dtdx    .req r7

indexA  .req r8
indexB  .req r9
LMAP    .req r10
TILE    .req r11
pixel   .req r12
width   .req lr

// FIQ regs
Ldx     .req r8
Rdx     .req r9
Ldt     .req r10
Rdt     .req r11
LRh     .req r12
L       .req r13
R       .req r14

Rh      .req LRh
Lh      .req t

h       .req N

ptr     .req tmp

Rxy     .req tmp
Ry2     .req Rh
Lxy     .req tmp
Ly2     .req Lh

inv     .req indexA
duv     .req indexB
dtmp    .req t
dtmp2   .req indexB

Ltmp    .req N
Rtmp    .req N
Ltmp2   .req dtdx
Rtmp2   .req dtdx

.macro PUT_PIXELS
    tex indexA, t
    lit indexA

    add t, dtdx, lsl #1
    strb indexA, [ptr], #2  // writing a byte to GBA VRAM will write a half word for free
.endm

.global rasterizeFT_asm
rasterizeFT_asm:
    stmfd sp!, {r4-r11, lr}

    mov pixel, arg_pixel

    mov LMAP, #LMAP_ADDR
    ldrb t, [arg_L, #VERTEX_G]
    add LMAP, t, lsl #8             // LMAP = (L->v.g << 8)

    ldr TILE, =gTile
    ldr TILE, [TILE]

    fiq_on
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
          ldrne Lt, [L, #VERTEX_T]  // Lt = L->t
          mov L, N                  // L = N
          beq .calc_left_start

        lsl Lx, Lxy, #16            // Lx = L->v.x << 16
        cmp Lh, #1                  // if (Lh <= 1) skip Ldx calc
          beq .calc_left_end

        divLUT tmp, Lh              // tmp = FixedInvU(Lh)

        ldrsh Ldx, [L, #VERTEX_X]
        subs Ldx, Lx, asr #16
        mulne Ldx, tmp, Ldx         // Ldx = tmp * (N->v.x - Lx)

        ldr Ldt, [L, #VERTEX_T]
        subs Ldt, Lt                // Ldt = N->v.t - Lt
        scaleUV Ldt, Ltmp, Ltmp2, tmp
    .calc_left_end:

    cmp Rh, #0
      bgt .calc_right_end       // if (Rh != 0) end with right

    .calc_right_start:
        ldrsb N, [R, #VERTEX_NEXT]  // N = R + R->next
        add N, R, N, lsl #VERTEX_SIZEOF_SHIFT
        ldr Rxy, [R, #VERTEX_X]     // Rxy = (R->v.y << 16) | (R->v.x)
        ldrsh Ry2, [N, #VERTEX_Y]   // Ry2 = N->v.y

        subs Rh, Ry2, Rxy, asr #16  // Rh = Ry2 - Rxy
          blt .exit                 // if (Rh < 0) return
          ldrne Rt, [R, #VERTEX_T]  // Rt = R->t
          mov R, N                  // R = N
          beq .calc_right_start

        lsl Rx, Rxy, #16            // Rx = R->v.x << 16
        cmp Rh, #1                  // if (Rh <= 1) skip Rdx calc
          beq .calc_right_end

        divLUT tmp, Rh              // tmp = FixedInvU(Rh)

        ldrsh Rdx, [R, #VERTEX_X]
        subs Rdx, Rx, asr #16
        mulne Rdx, tmp, Rdx         // Rdx = tmp * (N->v.x - Rx)

        ldr Rdt, [R, #VERTEX_T]
        subs Rdt, Rt                // Rdt = N->v.t - Rt
        scaleUV Rdt, Rtmp, Rtmp2, tmp
    .calc_right_end:

    cmp Rh, Lh              // if (Rh < Lh)
      movlt h, Rh           //      h = Rh
      movge h, Lh           // else h = Lh
    sub Lh, h               // Lh -= h
    sub Rh, h               // Rh -= h

    orr LRh, Rh, Lh, lsl #16

    fiq_off

.scanline_start:
    asr tmp, Lx, #16                // x1 = (Lx >> 16)
    rsbs width, tmp, Rx, asr #16    // width = (Rx >> 16) - x1
      ble .scanline_end             // if (width <= 0) go next scanline

    add ptr, pixel, tmp             // ptr = pixel + x1

    divLUT inv, width               // inv = FixedInvU(width)

    subs dtdx, Rt, Lt               // duv = Rt - Lt
    scaleUV dtdx, dtmp, dtmp2, inv

    mov t, Lt                       // t = Lt

    // 2 bytes alignment (VRAM write requirement)
.align_left:
    tst ptr, #1                   // if (ptr & 1)
      beq .align_right

    tex indexA, t
    lit indexA

    ldrb indexB, [ptr, #-1]!      // read pal index from VRAM (byte)
    orr indexB, indexA, lsl #8
    strh indexB, [ptr], #2

    subs width, #1              // width--
      beq .scanline_end         // if (width == 0)

    add t, dtdx

.align_right:
    tst width, #1
      beq .align_block_4px

    tex indexA, Rt
    lit indexA

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
    add pixel, #FRAME_WIDTH     // pixel += FRAME_WIDTH (240)

    fiq_on
    add Lx, Ldx
    add Rx, Rdx
    add Lt, Ldt
    add Rt, Rdt

    subs h, #1
      fiq_off_ne
      bne .scanline_start

    lsr Lh, LRh, #16
    lsl Rh, LRh, #16
    lsr Rh, Rh, #16

    cmp Lh, #0
      bne .calc_right_start
      b .calc_left_start

.exit:
    fiq_off
    ldmfd sp!, {r4-r11, pc}
