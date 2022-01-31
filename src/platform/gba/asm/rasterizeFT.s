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
DIVLUT  .req N
DIVLUTi .req L
width   .req N
t       .req L
dtdx    .req R

duv     .req R
du      .req L
dv      .req R

Lduv    .req h
Ldu     .req N
Ldv     .req h

Rduv    .req h
Rdu     .req N
Rdv     .req h

sLdx    .req tmp
sLdt    .req N
sRdx    .req Lh
sRdt    .req Rh

SP_LDX = 0
SP_LDT = 4
SP_RDX = 8
SP_RDT = 12

.macro PUT_PIXELS
    tex indexA, t
    lit indexA

#ifndef TEX_2PX
    add t, dtdx

    tex indexB, t
    lit indexB
    add t, dtdx

    orr indexA, indexB, lsl #8
    strh indexA, [tmp], #2
#else
    add t, dtdx, lsl #1

    //orr indexA, indexA, lsl #8
    strb indexA, [tmp], #2  // writing a byte to GBA VRAM will write a half word for free
#endif
.endm

.global rasterizeFT_asm
rasterizeFT_asm:
    stmfd sp!, {r4-r11, lr}
    sub sp, #16 // reserve stack space for [Ldx, Ldt, Rdx, Rdt]

    mov LMAP, #LMAP_ADDR
    ldrb tmp, [L, #VERTEX_G]
    add LMAP, tmp, lsl #8           // tmp = (L->v.g << 8)

    ldr TILE, =tile
    ldr TILE, [TILE]

    mov Lh, #0                      // Lh = 0
    mov Rh, #0                      // Rh = 0

.loop:

    .calc_left_start:
        cmp Lh, #0
          bne .calc_left_end        // if (Lh != 0) end with left
        ldr N, [L, #VERTEX_PREV]    // N = L->prev
        ldr Lxy, [L, #VERTEX_X]     // Lxy = (L->v.y << 16) | (L->v.x)
        ldrsh Ly2, [N, #VERTEX_Y]   // Ly2 = N->v.y
        subs Lh, Ly2, Lxy, asr #16  // Lh = N->v.y - L->v.y
          blt .exit                 // if (Lh < 0) return
        lsl Lx, Lxy, #16            // Lx = L->v.x << 16
        ldr Lt, [L, #VERTEX_T]      // Lt = L->t
        mov L, N                    // L = N
        cmp Lh, #1                  // if (Lh <= 1) skip Ldx calc
          ble .calc_left_start

        lsl tmp, Lh, #1
        mov DIVLUT, #DIVLUT_ADDR
        ldrh tmp, [DIVLUT, tmp]     // tmp = FixedInvU(Lh)

        ldrsh Ldx, [L, #VERTEX_X]
        sub Ldx, Lx, asr #16
        mul Ldx, tmp                // Ldx = tmp * (N->v.x - Lx)
        str Ldx, [sp, #SP_LDX]      // store Ldx to stack

        ldr Lduv, [L, #VERTEX_T]
        sub Lduv, Lt                // Lduv = N->v.t - Lt
        asr Ldu, Lduv, #16
        mul Ldu, tmp                // Rdu = tmp * int16(Lduv >> 16)
        lsl Ldv, Lduv, #16
        asr Ldv, #16
        mul Ldv, tmp                // Rdv = tmp * int16(Lduv)
        lsr Ldu, #16
        lsl Ldu, #16
        orr Ldt, Ldu, Ldv, lsr #16  // Ldt = (Rdu & 0xFFFF0000) | (Rdv >> 16)
        str Ldt, [sp, #SP_LDT]      // store Ldt to stack
    .calc_left_end:

    .calc_right_start:
        cmp Rh, #0
          bne .calc_right_end       // if (Rh != 0) end with right
        ldr N, [R, #VERTEX_NEXT]    // N = R->next
        ldr Rxy, [R, #VERTEX_X]     // Rxy = (R->v.y << 16) | (R->v.x)
        ldrsh Ry2, [N, #VERTEX_Y]   // Ry2 = N->v.y
        subs Rh, Ry2, Rxy, asr #16  // Rh = Ry2 - Rxy
          blt .exit                 // if (Rh < 0) return
        lsl Rx, Rxy, #16            // Rx = R->v.x << 16
        ldr Rt, [R, #VERTEX_T]      // Rt = R->t
        mov R, N                    // R = N
        cmp Rh, #1                  // if (Rh <= 1) skip Rdx calc
          ble .calc_right_start

        lsl tmp, Rh, #1
        mov DIVLUT, #DIVLUT_ADDR
        ldrh tmp, [DIVLUT, tmp]     // tmp = FixedInvU(Rh)

        ldrsh Rdx, [R, #VERTEX_X]
        sub Rdx, Rx, asr #16
        mul Rdx, tmp                // Rdx = tmp * (N->v.x - Rx)
        str Rdx, [sp, #SP_RDX]      // store Rdx to stack

        ldr Rduv, [R, #VERTEX_T]
        sub Rduv, Rt                // Rduv = N->v.t - Rt
        asr Rdu, Rduv, #16
        mul Rdu, tmp                // Rdu = tmp * int16(Rduv >> 16)
        lsl Rdv, Rduv, #16
        asr Rdv, #16
        mul Rdv, tmp                // Rdv = tmp * int16(Rduv)
        lsr Rdu, #16
        lsl Rdu, #16
        orr Rdt, Rdu, Rdv, lsr #16  // Rdt = (Rdu & 0xFFFF0000) | (Rdv >> 16)
        str Rdt, [sp, #SP_RDT]      // store Rdt to stack
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

    add tmp, pixel, tmp             // tmp = pixel + x1

    mov DIVLUTi, #DIVLUT_ADDR
    lsl inv, width, #1
    ldrh inv, [DIVLUTi, inv]        // inv = FixedInvU(width)

    sub duv, Rt, Lt                 // duv = Rt - Lt
    asr du, duv, #16
    mul du, inv                     // du = inv * int16(duv >> 16)
    lsl dv, duv, #16
    asr dv, #16
    mul dv, inv                     // dv = inv * int16(duv)
    lsr du, #16
    lsl du, #16
    orr dtdx, du, dv, lsr #16       // dtdx = (du & 0xFFFF0000) | (dv >> 16)

    mov t, Lt                       // t = Lt

    // 2 bytes alignment (VRAM write requirement)
.align_left:
    tst tmp, #1                   // if (tmp & 1)
      beq .align_right
    ldrb indexB, [tmp, #-1]!      // read pal index from VRAM (byte)

    and indexA, t, #0xFF00
    orr indexA, t, lsr #24        // res = (t & 0xFF00) | (t >> 24)
    ldrb indexA, [TILE, indexA]
    ldrb indexA, [LMAP, indexA]

    orr indexB, indexA, lsl #8
    strh indexB, [tmp], #2
    add t, dtdx

    subs width, #1              // width--
      beq .scanline_end         // if (width == 0)

.align_right:
    tst width, #1
      beq .align_block_4px
    ldrb indexB, [tmp, width]

    subs width, #1              // width--

    sub Rt, dtdx
    and indexA, Rt, #0xFF00
    orr indexA, Rt, lsr #24     // res = (t & 0xFF00) | (t >> 24)
    add Rt, dtdx
    ldrb indexA, [TILE, indexA]
    ldrb indexA, [LMAP, indexA]

    orr indexB, indexA, indexB, lsl #8
    strh indexB, [tmp, width]

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
    add tmp, sp, #16
    ldmia tmp, {sLdx, sLdt, sRdx, sRdt}
    add Lx, sLdx
    add Lt, sLdt
    add Rx, sRdx
    add Rt, sRdt

    add pixel, #FRAME_WIDTH   // pixel += FRAME_WIDTH (240)

    subs h, #1
      bne .scanline_start

    ldmfd sp!, {L,R,Lh,Rh}    // sp+16
    b .loop

.exit:
    add sp, #16                 // revert reserved space for [Ldx, Ldt, Rdx, Rdt]
    ldmfd sp!, {r4-r11, pc}