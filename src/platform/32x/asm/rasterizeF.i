#define tmp     r0
#define Lh      r1
#define Rh      r2
#define Lptr    r3
#define pixel   r4      // arg
#define L       r5      // arg
#define index   r6      // arg
#define gtile   r7      // arg (unused)
#define N       gtile
#define Lx      r8
#define Rx      r9
#define Ldx     r10
#define Rdx     r11
#define dup     r12     // const
#define inv     r13
#define divLUT  r14

#define R       index
#define h       N

#define Ry      inv
#define Ly      inv

#define Rptr    R

#define iw      inv
#define ih      inv
#define LMAP    inv

.align 4
.exit_f:
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

.global _rasterizeF_asm
_rasterizeF_asm:
        // push
        mov.l   r8, @-sp
        mov.l   r9, @-sp
        mov.l   r10, @-sp
        mov.l   r11, @-sp
        mov.l   r12, @-sp
        mov.l   r13, @-sp
        mov.l   r14, @-sp

        mov.l   var_LMAP_ADDR_fs, LMAP
        mov.b   @(VERTEX_G, L), tmp
        shll8   tmp
        add     index, tmp
        or      tmp, LMAP
        mov.b   @LMAP, dup
        extu.b  dup, dup
        swap.b  dup, index
        or      index, dup      // dup = index | (index << 8)

        mov     L, R

        mov.l   var_divTable_fs, divLUT

        mov     #0, Rh
        mov     #0, Lh
.loop_f:
        tst     Lh, Lh
        bf/s    .calc_left_end_f

.calc_left_start_f:
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
        bf/s    .exit_f
        cmp/eq  Ly, Lh          // [delay slot]
        bt/s    .calc_left_start_f      // if (L->v.y == N->v.y) check next vertex
        mov     N, L            // [delay slot]

        sub     Lx, Ldx
        sub     Ly, Lh

        mov     Lh, tmp
        shll    tmp
        mov.w   @(tmp, divLUT), ih

        muls.w  ih, Ldx
          shll16  Lx            // [delay slot]
        sts     MACL, Ldx
.calc_left_end_f:

        tst     Rh, Rh
        bf/s    .calc_right_end_f

.calc_right_start_f:
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
        bf/s    .exit_f
        cmp/eq  Ry, Rh          // [delay slot]
        bt/s    .calc_right_start_f     // if (R->v.y == N->v.y) check next vertex
        mov     N, R            // [delay slot]

        sub     Rx, Rdx
        sub     Ry, Rh

        mov     Rh, tmp
        shll    tmp
        mov.w   @(tmp, divLUT), ih

        muls.w  ih, Rdx
          shll16  Rx            // [delay slot]
        sts     MACL, Rdx
.calc_right_end_f:

        // h = min(Lh, Rh)
        cmp/gt  Rh, Lh
        bf/s    .scanline_prepare_f
        mov     Lh, h           // [delay slot]
        mov     Rh, h

.scanline_prepare_f:
        sub     h, Lh
        sub     h, Rh

        mov.l   R, @-sp
        
.scanline_start_f:
        mov     Lx, Lptr
        mov     Rx, Rptr
        add     Ldx, Lx
        add     Rdx, Rx
        shlr16  Lptr            // Lptr = (Lx >> 16)
        shlr16  Rptr            // Rptr = (Rx >> 16)
        cmp/gt  Lptr, Rptr      // if (!(Rptr > Lptr)) skip zero length scanline
        bf/s    .scanline_end_f

        // iw = divTable[Rptr - Lptr]
        mov     Rptr, tmp       // [delay slot]
        sub     Lptr, tmp
        shll    tmp
        mov.w   @(tmp, divLUT), iw

        add     pixel, Lptr   // Lptr = pixel + (Lx >> 16)
        add     pixel, Rptr   // Rptr = pixel + (Rx >> 16)

.align_left_f:
        mov     #1, tmp
        tst     tmp, Lptr
        bt/s    .align_right_f
        tst     tmp, Rptr       // [delay slot]

        mov.b   dup, @Lptr
        add     #1, Lptr

        mov     #1, tmp         // tmp = 1 (for align_right)
        cmp/gt  Lptr, Rptr
        bf/s    .scanline_end_f
        tst     tmp, Rptr

.align_right_f:
        bt      .block_2px_f
        mov.b   dup, @-Rptr
        cmp/gt  Lptr, Rptr
        bf      .scanline_end_f

.block_2px_f:
        mov.w   dup, @-Rptr
        cmp/gt  Lptr, Rptr
        bt      .block_2px_f

.scanline_end_f:
        dt      h

        mov.w   var_frameWidth_fs, tmp
        bf/s    .scanline_start_f
        add     tmp, pixel      // [delay slot] pixel += 120 + 120 + 80

        bra     .loop_f
        mov.l   @sp+, R

#undef tmp
#undef Lh
#undef Rh
#undef Lptr
#undef pixel
#undef L
#undef index
#undef N
#undef Lx
#undef Rx
#undef Ldx
#undef Rdx
#undef dup
#undef inv
#undef divLUT
#undef R
#undef h
#undef Ry
#undef Ly
#undef Rptr
#undef iw
#undef ih
#undef LMAP
