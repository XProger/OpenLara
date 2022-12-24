#define tmp     r0
#define Lh      r1
#define Rh      r2
#define LMAP    r3      // const
#define pixel   r4      // arg
#define L       r5      // arg
#define R       r6      // arg
#define gtile   r7      // arg
#define N       gtile
#define Lx      r8
#define Rx      r9
#define Lt      r10
#define Rt      r11
#define dup     r12
#define TILE    r13     // const
#define divLUT  r14

#define h       N

#define Ldx     h
#define Rdx     h

#define Ldt     h
#define Rdt     h

#define Ry      Rx
#define Ly      Lx

#define Rv      Rx
#define Lv      Lx

#define Lptr    L
#define Rptr    R

#define t       Lh
#define dtdx    Rh

#define index   tmp

#define iw      dup
#define ih      dup

#define sLdx    L
#define sRdx    R
#define sLdt    Lh
#define sRdt    Rh

#define SP_LDX  0
#define SP_RDX  4
#define SP_LDT  8
#define SP_RDT  12
#define SP_H    16
#define SP_L    20
#define SP_R    24
#define SP_SIZE 28

.align 4
.exit_ft:
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

.global _rasterizeFT_asm
_rasterizeFT_asm:
        // push
        mov.l   r8, @-sp
        mov.l   r9, @-sp
        mov.l   r10, @-sp
        mov.l   r11, @-sp
        mov.l   r12, @-sp
        mov.l   r13, @-sp
        mov.l   r14, @-sp
        add     #-SP_SIZE, sp

        mov.l   var_LMAP_ADDR, LMAP
        mov.b   @(VERTEX_G, L), tmp
        shll8   tmp
        or      tmp, LMAP

        mov.l   var_divTable, divLUT

        mov     gtile, TILE
        nop

        mov     #0, Rh
.loop_ft:
        extu.w  Rh, Lh  // Lh = int16(Rh)

        tst     Lh, Lh
        bf/s    .calc_left_end_ft
        nop

.calc_left_start_ft:
        mov.b   @(VERTEX_PREV, L), tmp  // [delay slot]
        mov     tmp, N

        mov.w   @(VERTEX_Y, L), tmp
        add     L, N            // N = L + (L->prev << VERTEX_SIZEOF_SHIFT)
        mov     tmp, Ly
        mov.w   @(VERTEX_Y, N), tmp
        sub     Ly, tmp
        cmp/pz  tmp
        bf/s    .exit_ft
        tst     tmp, tmp
        mov     L, Lv           // Lv = L
        bt/s    .calc_left_start_ft      // if (Lh == 0) check next vertex
        mov     N, L            // [delay slot]

        mov     tmp, Lh
        mov.l   @(VERTEX_T, Lv), Lt
        mov.w   @(VERTEX_X, Lv), tmp
        swap.w  tmp, Lx         // Lx = L->v.x << 16

        mov     Lh, tmp
        cmp/eq  #1, tmp
        bt/s    .calc_left_end_ft
        shll    tmp             // [delay slot]

        mov.w   @(tmp, divLUT), ih

        // calc Ldx
        mov.w   @(VERTEX_X, L), tmp
          swap.w  Lx, Ldx
        sub     Ldx, tmp
        muls.w  ih, tmp
          mov.l   @(VERTEX_T, L), Ldt
        sts     MACL, tmp
          sub     Lt, Ldt
        mov.l   tmp, @(SP_LDX, sp)

        // calc Ldt
        scaleUV Ldt, tmp, ih
        mov.l   tmp, @(SP_LDT, sp)
.calc_left_end_ft:

        shlr16  Rh              // Rh = (Rh >> 16)
        tst     Rh, Rh
        bf/s    .calc_right_end_ft
        nop

.calc_right_start_ft:
        mov.b   @(VERTEX_NEXT, R), tmp  // [delay slot]
        mov     tmp, N

        mov.w   @(VERTEX_Y, R), tmp
        add     R, N            // N = R + (R->next << VERTEX_SIZEOF_SHIFT)
        mov     tmp, Ry
        mov.w   @(VERTEX_Y, N), tmp
        sub     Ry, tmp
        cmp/pz  tmp
        bf/s    .exit_ft
        tst     tmp, tmp
        mov     R, Rv           // Rv = R
        bt/s    .calc_right_start_ft     // if (Rh == 0) check next vertex
        mov     N, R            // [delay slot]

        mov     tmp, Rh
        mov.l   @(VERTEX_T, Rv), Rt
        mov.w   @(VERTEX_X, Rv), tmp
        swap.w  tmp, Rx         // Rx = R->v.x << 16

        mov     Rh, tmp
        cmp/eq  #1, tmp
        bt/s    .calc_right_end_ft
        shll    tmp             // [delay slot]

        mov.w   @(tmp, divLUT), ih

        // calc Rdx
        mov.w   @(VERTEX_X, R), tmp
          swap.w  Rx, Rdx
        sub     Rdx, tmp
        muls.w  ih, tmp
          mov.l   @(VERTEX_T, R), Rdt
        sts     MACL, tmp
          sub     Rt, Rdt
        mov.l   tmp, @(SP_RDX, sp)

        // calc Rdt
        scaleUV Rdt, tmp, ih
        mov.l   tmp, @(SP_RDT, sp)
.calc_right_end_ft:

        // h = min(Lh, Rh)
        cmp/gt  Rh, Lh
        bf/s    .scanline_prepare_ft
        mov     Lh, h           // [delay slot]
        mov     Rh, h

.scanline_prepare_ft:
        sub     h, Lh
        sub     h, Rh

        swap.w  Rh, tmp
        or      Lh, tmp

        mov.l   tmp, @(SP_H, sp)
        mov.l   L, @(SP_L, sp)
        mov.l   R, @(SP_R, sp)
        nop

.scanline_start_ft:
        mov     Lx, Lptr
        mov     Rx, Rptr
        shlr16  Lptr            // Lptr = (Lx >> 16)
        shlr16  Rptr            // Rptr = (Rx >> 16)
        cmp/gt  Lptr, Rptr      // if (!(Rptr > Lptr)) skip zero length scanline
        bf/s    .scanline_end_ft

        // iw = divTable[Rptr - Lptr]
        mov     Rptr, tmp       // [delay slot]
        sub     Lptr, tmp
        shll    tmp
        mov.w   @(tmp, divLUT), iw

        // calc dtdx
        mov     Rt, tmp
        sub     Lt, tmp
        muls.w  tmp, iw
          add     pixel, Lptr   // Lptr = pixel + (Lx >> 16)
        sts     MACL, dtdx      // v = int16(uv) * f (16-bit shift)
          shlr16  tmp
        muls.w  tmp, iw
          add     pixel, Rptr   // Rptr = pixel + (Rx >> 16)
        sts     MACL, tmp       // u = int16(uv >> 16) * f (16-bit shift)
          mov     Rt, t
        shlr16  tmp
        xtrct   tmp, dtdx       // out = uint16(v >> 16) | (u & 0xFFFF0000)

.align_left_ft:
        mov     #1, tmp
        tst     tmp, Lptr
        bt/s    .align_right_ft
        tst     tmp, Rptr       // [delay slot]

        getUV   Lt, index
        mov.b   @(index, TILE), index
        mov.b   @(index, LMAP), index
        mov.b   index, @Lptr
        add     #1, Lptr

        mov     #1, tmp         // tmp = 1 (for align_right)
        cmp/gt  Lptr, Rptr
        bf/s    .scanline_end_ft
        tst     tmp, Rptr

.align_right_ft:
        bt/s    .block_prepare_ft

        getUV   t, index
        mov.b   @(index, TILE), index
        mov.b   @(index, LMAP), index
        sub     dtdx, t
        mov.b   index, @-Rptr

        cmp/gt  Lptr, Rptr
        bf/s    .scanline_end_ft
        nop

.block_prepare_ft:
        shll    dtdx            // [delay slot] optional
        nop

.block_2px_ft:
        getUV   t, index

        mov.b   @(index, TILE), index
        mov.b   @(index, LMAP), index

        extu.b  index, index
        swap.b  index, dup
        or      index, dup      // dup = index | (index << 8)
        mov.w   dup, @-Rptr

        cmp/gt  Lptr, Rptr
        bt/s    .block_2px_ft
        sub     dtdx, t         // [delay slot] t -= dtdx
        nop

.scanline_end_ft:
        mov.l   @(SP_LDX, sp), sLdx
        mov.l   @(SP_RDX, sp), sRdx
        mov.l   @(SP_LDT, sp), sLdt
        mov.l   @(SP_RDT, sp), sRdt

        add     sLdx, Lx
        add     sRdx, Rx
        add     sLdt, Lt
        add     sRdt, Rt

        dt      h

        mov.w   var_frameWidth_ft, tmp
        bf/s    .scanline_start_ft
        add     tmp, pixel      // [delay slot] pixel += 120 + 120 + 80

        mov.l   @(SP_L, sp), L
        mov.l   @(SP_R, sp), R
        bra     .loop_ft
        mov.l   @(SP_H, sp), Rh

var_frameWidth_ft:
        .word   FRAME_WIDTH

#undef tmp
#undef Lh
#undef Rh
#undef LMAP
#undef pixel
#undef L
#undef R
#undef N
#undef Lx
#undef Rx
#undef Lt
#undef Rt
#undef dup
#undef TILE
#undef divLUT
#undef h
#undef Ldx
#undef Rdx
#undef Ldt
#undef Rdt
#undef Ry
#undef Ly
#undef Rv
#undef Lv
#undef Lptr
#undef Rptr
#undef t
#undef dtdx
#undef index
#undef iw
#undef ih
#undef sLdx
#undef sRdx
#undef sLdt
#undef sRdt
#undef SP_LDX
#undef SP_RDX
#undef SP_LDT
#undef SP_RDT
#undef SP_H
#undef SP_L
#undef SP_R
#undef SP_SIZE
