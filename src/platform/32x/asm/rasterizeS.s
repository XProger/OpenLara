#include "common.i"
SEG_RASTER

#define tmp     r0
#define Lh      r1
#define Rh      r2
#define Lptr    r3
#define pixel   r4      // arg
#define L       r5      // arg
#define R       r6      // arg
#define N       r7
#define Lx      r8
#define Rx      r9
#define Ldx     r10
#define Rdx     r11
#define LMAP    r12     // const
#define inv     r13
#define divLUT  r14

#define index   tmp
#define h       N

#define Ry      inv
#define Ly      inv

#define Rptr    R

#define iw      inv
#define ih      inv

.align 4
.exit:
        // pop
        mov.l   @sp+, r14
        mov.l   @sp+, r13
        mov.l   @sp+, r12
        mov.l   @sp+, r11
        mov.l   @sp+, r10
        mov.l   @sp+, r9
        rts
        mov.l   @sp+, r8
        nop

.global _rasterizeS_asm
_rasterizeS_asm:
        // push
        mov.l   r8, @-sp
        mov.l   r9, @-sp
        mov.l   r10, @-sp
        mov.l   r11, @-sp
        mov.l   r12, @-sp
        mov.l   r13, @-sp
        mov.l   r14, @-sp

        mov.l   var_LMAP_ADDR, LMAP
        mov     #27, tmp
        shll8   tmp
        or      tmp, LMAP

        mov.l   var_divTable, divLUT

        mov     #0, Rh
        mov     #0, Lh
.loop:
        tst     Lh, Lh
        bf/s    .calc_left_end

.calc_left_start:
        mov.b   @(VERTEX_PREV, L), tmp  // [delay slot]
        mov     tmp, N
        shll2   N
        shll2   N
        add     L, N            // N = L + (L->prev << VERTEX_SIZEOF_SHIFT)

        mov.w   @L+, Lx
        mov.w   @L+, Ly

        mov     N, tmp
        mov.w   @tmp+, Ldx
        mov.w   @tmp+, Lh

        cmp/ge  Ly, Lh
        bf/s    .exit
        cmp/eq  Ly, Lh          // [delay slot]
        bt/s    .calc_left_start        // if (L->v.y == N->v.y) check next vertex
        mov     N, L            // [delay slot]

        sub     Lx, Ldx
        sub     Ly, Lh

        mov     Lh, tmp
        shll    tmp
        mov.w   @(tmp, divLUT), ih

        muls.w  ih, Ldx
          shll16  Lx            // [delay slot]
        sts     MACL, Ldx
.calc_left_end:

        tst     Rh, Rh
        bf/s    .calc_right_end

.calc_right_start:
        mov.b   @(VERTEX_NEXT, R), tmp  // [delay slot]
        mov     tmp, N
        shll2   N
        shll2   N
        add     R, N            // N = R + (R->next << VERTEX_SIZEOF_SHIFT)

        mov.w   @R+, Rx
        mov.w   @R+, Ry

        mov     N, tmp
        mov.w   @tmp+, Rdx
        mov.w   @tmp+, Rh

        cmp/ge  Ry, Rh
        bf/s    .exit
        cmp/eq  Ry, Rh          // [delay slot]
        bt/s    .calc_right_start       // if (R->v.y == N->v.y) check next vertex
        mov     N, R            // [delay slot]

        sub     Rx, Rdx
        sub     Ry, Rh

        mov     Rh, tmp
        shll    tmp
        mov.w   @(tmp, divLUT), ih

        muls.w  ih, Rdx
          shll16  Rx            // [delay slot]
        sts     MACL, Rdx
.calc_right_end:

        // h = min(Lh, Rh)
        cmp/gt  Rh, Lh
        bf/s    .scanline_prepare
        mov     Lh, h           // [delay slot]
        mov     Rh, h

.scanline_prepare:
        sub     h, Lh
        sub     h, Rh

        mov.l   R, @-sp
        
.scanline_start:
        mov     Lx, Lptr
        mov     Rx, Rptr
        add     Ldx, Lx
        add     Rdx, Rx
        shlr16  Lptr            // Lptr = (Lx >> 16)
        shlr16  Rptr            // Rptr = (Rx >> 16)
        cmp/gt  Lptr, Rptr      // if (!(Rptr > Lptr)) skip zero length scanline
        bf/s    .scanline_end

        // iw = divTable[Rptr - Lptr]
        mov     Rptr, tmp       // [delay slot]
        sub     Lptr, tmp
        shll    tmp
        mov.w   @(tmp, divLUT), iw

        add     pixel, Lptr   // Lptr = pixel + (Lx >> 16)
        add     pixel, Rptr   // Rptr = pixel + (Rx >> 16)

.shade_pixel:
        mov.b   @Lptr, index
        mov.b   @(index, LMAP), index
        mov.b   index, @Lptr
        add     #1, Lptr
        cmp/gt  Lptr, Rptr
        bt      .shade_pixel

.scanline_end:
        dt      h

        mov.w   var_frameWidth, tmp
        bf/s    .scanline_start
        add     tmp, pixel      // [delay slot] pixel += 120 + 120 + 80

        bra     .loop
        mov.l   @sp+, R

var_frameWidth:
        .word   FRAME_WIDTH
.align 2
var_LMAP_ADDR:
        .long   _gLightmap_base
var_divTable:
        .long   _divTable
