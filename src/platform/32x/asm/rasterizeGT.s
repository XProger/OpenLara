#include "common.i"
SEG_RASTER

#define tmp     r0
#define Lh      r1
#define Rh      r2
#define dup     r3
#define pixel   r4      // arg
#define L       r5      // arg
#define R       r6      // arg
#define N       r7
#define Lx      r8
#define Rx      r9
#define Lg      r10
#define Rg      r11
#define Lt      r12
#define Rt      r13
#define TILE    r14     // const

#define h       N

#define Ldx     h
#define Rdx     h

#define Ldt     h
#define Rdt     h

#define Ry      Rx
#define Ly      Lx

#define Rv      Rx
#define Lv      Lx

#define Lptr    Lh
#define Rptr    Rx

#define g       Rg
#define dgdx    R

#define t       Rt
#define dtdx    L

#define index   tmp
#define LMAP    dup

#define divLUT  dup
#define iw      dup
#define ih      dup

#define dx      dgdx
#define mask    Rh

#define sLdx    L
#define sRdx    R
#define sLdt    L
#define sRdt    R
#define sLdg    L
#define sRdg    R

SP_LDX = 0
SP_RDX = 4
SP_LDT = 8
SP_RDT = 12
SP_LDG = 16
SP_RDG = 18
SP_H   = 20
SP_L   = 24
SP_R   = 28
SP_SIZE = 32

.align 4
.exit:
        // pop
        add     #SP_SIZE, sp
        mov.l   @sp+, r14
        mov.l   @sp+, r13
        mov.l   @sp+, r12
        mov.l   @sp+, r11
        mov.l   @sp+, r10
        mov.l   @sp+, r9
        rts
        mov.l   @sp+, r8
        nop

.global _rasterizeGT_asm
_rasterizeGT_asm:
        // push
        mov.l   r8, @-sp
        mov.l   r9, @-sp
        mov.l   r10, @-sp
        mov.l   r11, @-sp
        mov.l   r12, @-sp
        mov.l   r13, @-sp
        mov.l   r14, @-sp
        add     #-SP_SIZE, sp

        mov.l   var_gTile, TILE
        mov.l   @TILE, TILE

        mov     #0, Rh

.loop:
        extu.w  Rh, Lh  // Lh = int16(Rh)

        tst     Lh, Lh
        bf/s    .calc_left_end

.calc_left_start:
        mov.b   @(VERTEX_PREV, L), tmp  // [delay slot]
        mov     tmp, N

        mov.w   @(VERTEX_Y, L), tmp
        shll2   N
        shll2   N
        add     L, N            // N = L + (L->prev << VERTEX_SIZEOF_SHIFT)
        mov     tmp, Ly
        mov.w   @(VERTEX_Y, N), tmp
        sub     Ly, tmp
        cmp/pz  tmp
        bf/s    .exit
        tst     tmp, tmp
        mov     L, Lv           // Lv = L
        bt/s    .calc_left_start        // if (Lh == 0) check next vertex
        mov     N, L            // [delay slot]

        mov     tmp, Lh
        mov.b   @(VERTEX_G, Lv), tmp
        mov.l   @(VERTEX_T, Lv), Lt
        mov     tmp, Lg
        mov.w   @(VERTEX_X, Lv), tmp
          shll8   Lg
        swap.w  tmp, Lx         // Lx = L->v.x << 16

        mov     Lh, tmp
        cmp/eq  #1, tmp
        bt/s    .calc_left_end
        shll    tmp             // [delay slot]

        mov.l   var_divTable, divLUT
        mov.w   @(tmp, divLUT), ih

        // calc Ldx
        mov.w   @(VERTEX_X, L), tmp
          swap.w  Lx, Ldx
        sub     Ldx, tmp
        muls.w  ih, tmp
          mov.b   @(VERTEX_G, L), tmp
        sts     MACL, Ldx
          shll8   tmp
        mov.l   Ldx, @(SP_LDX, sp)

        // calc Ldg
        sub     Lg, tmp
        muls.w  ih, tmp
          mov.l   @(VERTEX_T, L), Ldt
        sts     MACL, tmp
          sub     Lt, Ldt
        shlr16  tmp
        mov.w   tmp, @(SP_LDG, sp)

        // calc Ldt
        scaleUV Ldt, tmp, ih
        mov.l   tmp, @(SP_LDT, sp)
.calc_left_end:

        shlr16  Rh              // Rh = (Rh >> 16)
        tst     Rh, Rh
        bf/s    .calc_right_end

.calc_right_start:
        mov.b   @(VERTEX_NEXT, R), tmp  // [delay slot]
        mov     tmp, N

        mov.w   @(VERTEX_Y, R), tmp
        shll2   N
        shll2   N
        add     R, N            // N = R + (R->next << VERTEX_SIZEOF_SHIFT)
        mov     tmp, Ry
        mov.w   @(VERTEX_Y, N), tmp
        sub     Ry, tmp
        cmp/pz  tmp
        bf/s    .exit
        tst     tmp, tmp
        mov     R, Rv           // Rv = R
        bt/s    .calc_right_start       // if (Rh == 0) check next vertex
        mov     N, R            // [delay slot]

        mov     tmp, Rh
        mov.b   @(VERTEX_G, Rv), tmp
        mov.l   @(VERTEX_T, Rv), Rt
        mov     tmp, Rg
        mov.w   @(VERTEX_X, Rv), tmp
          shll8   Rg
        swap.w  tmp, Rx         // Rx = R->v.x << 16

        mov     Rh, tmp
        cmp/eq  #1, tmp
        bt/s    .calc_right_end
        shll    tmp             // [delay slot]

        mov.l   var_divTable, divLUT
        mov.w   @(tmp, divLUT), ih

        // calc Rdx
        mov.w   @(VERTEX_X, R), tmp
          swap.w  Rx, Rdx
        sub     Rdx, tmp
        muls.w  ih, tmp
          mov.b   @(VERTEX_G, R), tmp
        sts     MACL, Rdx
          shll8   tmp
        mov.l   Rdx, @(SP_RDX, sp)

        // calc Rdg
        sub     Rg, tmp
        muls.w  ih, tmp
          mov.l   @(VERTEX_T, R), Rdt
        sts     MACL, tmp
          sub     Rt, Rdt
        shlr16  tmp
        mov.w   tmp, @(SP_RDG, sp)

        // calc Rdt
        scaleUV Rdt, tmp, ih
        mov.l   tmp, @(SP_RDT, sp)
.calc_right_end:

        // bake gLightmap address into g value
        mov.l   var_LMAP_ADDR, tmp
        or      tmp, Lg
        or      tmp, Rg

        // h = min(Lh, Rh)
        cmp/gt  Rh, Lh
        bf/s    .scanline_prepare
        mov     Lh, h           // [delay slot]
        mov     Rh, h

.scanline_prepare:
        sub     h, Lh
        sub     h, Rh

        swap.w  Rh, tmp
        or      Lh, tmp

        mov.l   tmp, @(SP_H, sp)
        mov.l   L, @(SP_L, sp)
        mov.l   R, @(SP_R, sp)

        mov.l   var_mask, mask
        
.scanline_start:
        mov.l   Rx, @-sp        // alias Rptr

        mov     Lx, Lptr
        shlr16  Lptr            // Lptr = (Lx >> 16)
        shlr16  Rptr            // Rptr = (Rx >> 16)
        cmp/gt  Lptr, Rptr      // if (!(Rptr > Lptr)) skip zero length scanline
        bf/s    .scanline_end_fast

        // iw = divTable[Rptr - Lptr]
        mov     Rptr, tmp       // [delay slot]
        sub     Lptr, tmp
        mov.l   var_divTable, divLUT
        shll    tmp
        mov.w   @(tmp, divLUT), iw

        add     pixel, Lptr     // Lptr = pixel + (Lx >> 16)
        add     pixel, Rptr     // Rptr = pixel + (Rx >> 16)

        mov.l   Rt, @-sp        // alias t
        mov.l   Rg, @-sp        // alias g

        // calc dtdx
        mov     Rt, tmp
        sub     Lt, tmp
        muls.w  tmp, iw
          shlr16  tmp
        sts     MACL, dtdx      // v = int16(uv) * f (16-bit shift)
        muls.w  tmp, iw
          mov     Rg, tmp
        sts     MACL, dx        // u = int16(uv >> 16) * f (16-bit shift)
          sub     Lg, tmp
        shlr16  dx
        xtrct   dx, dtdx        // out = uint16(v >> 16) | (u & 0xFFFF0000)

        // calc dgdx
        muls.w  tmp, iw
          mov     #1, tmp
        sts     MACL, dgdx
          tst     tmp, Lptr
        shlr16  dgdx
        exts.w  dgdx, dgdx

.align_left:
        bt/s    .align_right
        tst     tmp, Rptr       // [delay slot]

        getUV   Lt, index
        mov.b   @(index, TILE), index
        mov     Lg, LMAP
        and     mask, LMAP
        mov.b   @(index, LMAP), index

        mov.b   index, @Lptr
        add     #1, Lptr

        mov     #1, tmp         // tmp = 1 (for align_right)
        cmp/gt  Lptr, Rptr
        bf/s    .scanline_end
        tst     tmp, Rptr

.align_right:
        bt/s    .block_prepare
        mov     g, LMAP

        getUV   t, index
        mov.b   @(index, TILE), index
        and     mask, LMAP
        sub     dgdx, g
        mov.b   @(index, LMAP), index

        sub     dtdx, t

        mov.b   index, @-Rptr

        cmp/gt  Lptr, Rptr
        bf/s    .scanline_end

.block_prepare:
        shll    dtdx            // [delay slot] optional
        shll    dgdx

.block_2px:
        swap.b  t, index        // UUuuvvVV
        swap.w  index, index    // vvVVUUuu
        shll8   index           // VVUUuu00
        shlr16  index           // 0000VVUU
        mov.b   @(index, TILE), index

        mov     g, LMAP
        and     mask, LMAP      // LMAP = (g & 0xFFFFFF00)
        mov.b   @(index, LMAP), index
        sub     dgdx, g         // g -= dgdx

        extu.b  index, index
        swap.b  index, dup
        or      index, dup      // dup = index | (index << 8)
        mov.w   dup, @-Rptr

        cmp/gt  Lptr, Rptr
        bt/s    .block_2px
        sub     dtdx, t         // [delay slot] t -= dtdx

.scanline_end:
        mov.l   @sp+, Rg
        mov.l   @sp+, Rt
.scanline_end_fast:
        mov.l   @sp+, Rx

        mov     sp, tmp

        mov.l   @tmp+, sLdx
        mov.l   @tmp+, sRdx

        add     sLdx, Lx
        add     sRdx, Rx

        mov.l   @tmp+, sLdt
        mov.l   @tmp+, sRdt

        add     sLdt, Lt
        add     sRdt, Rt

        mov.w   @tmp+, sLdg
        mov.w   @tmp+, sRdg

        add     sLdg, Lg
        add     sRdg, Rg

        dt      h

        mov.w   var_frameWidth, tmp
        bf/s    .scanline_start
        add     tmp, pixel      // [delay slot] pixel += 120 + 120 + 80

        mov.l   @(SP_L, sp), L
        mov.l   @(SP_R, sp), R
        bra     .loop
        mov.l   @(SP_H, sp), Rh

var_frameWidth:
        .word   FRAME_WIDTH
.align 2
var_LMAP_ADDR:
        .long   _gLightmap_base
var_mask:
        .long    0xFFFFFF00
var_divTable:
        .long   _divTable
var_gTile:
        .long   _gTile
