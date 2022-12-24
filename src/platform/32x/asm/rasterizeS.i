#define tmp     r0
#define Lh      r1
#define Rh      r2
#define Lptr    r3
#define pixel   r4      // arg
#define L       r5      // arg
#define R       r6      // arg
#define h       r7
#define Lx      r8
#define Rx      r9
#define Ldx     r10
#define Rdx     r11
#define LMAP    r12     // const
#define inv     r13
#define divLUT  r14

#define index   tmp

#define Ry      inv
#define Ly      inv

#define Rptr    inv

#define ih      inv

.align 4
.exit_s:
        // pop
        mov.l   @sp+, r14
        mov.l   @sp+, r13
        mov.l   @sp+, r12
        mov.l   @sp+, r11
        mov.l   @sp+, r10
        mov.l   @sp+, r9
        rts
        mov.l   @sp+, r8

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

        mov.l   var_LMAP_ADDR_fs, LMAP
        mov     #27, tmp
        shll8   tmp
        or      tmp, LMAP

        mov.l   var_divTable_fs, divLUT

        mov     #0, Rh
        nop

.calc_left_start_s:
        mov.b   @(VERTEX_PREV, L), tmp  // [delay slot]
        add     L, tmp          // tmp = L + (L->prev << VERTEX_SIZEOF_SHIFT)

        mov.l   @L, Lx
        extu.w  Lx, Ly
        shlr16  Lx

        mov.l   @tmp, Ldx
        extu.w  Ldx, Lh
        shlr16  Ldx

        cmp/ge  Ly, Lh
        bf/s    .exit_s
        cmp/eq  Ly, Lh          // [delay slot]
        bt/s    .calc_left_start_s      // if (L->v.y == N->v.y) check next vertex
        mov     tmp, L          // [delay slot]

        sub     Lx, Ldx
        sub     Ly, Lh

        mov     Lh, tmp
        shll    tmp
        mov.w   @(tmp, divLUT), ih

        muls.w  ih, Ldx
          shll16  Lx            // [delay slot]
        sts     MACL, Ldx
.calc_left_end_s:

        tst     Rh, Rh
        bf      .calc_right_end_s
        nop

.calc_right_start_s:
        mov.b   @(VERTEX_NEXT, R), tmp
        add     R, tmp          // tmp = R + (R->next << VERTEX_SIZEOF_SHIFT)

        mov.l   @R, Rx
        extu.w  Rx, Ry
        shlr16  Rx

        mov.l   @tmp, Rdx
        extu.w  Rdx, Rh
        shlr16  Rdx

        cmp/ge  Ry, Rh
        bf/s    .exit_s
        cmp/eq  Ry, Rh          // [delay slot]
        bt/s    .calc_right_start_s     // if (R->v.y == N->v.y) check next vertex
        mov     tmp, R          // [delay slot]

        sub     Rx, Rdx
        sub     Ry, Rh

        mov     Rh, tmp
        shll    tmp
        mov.w   @(tmp, divLUT), ih

        muls.w  ih, Rdx
          shll16  Rx            // [delay slot]
        sts     MACL, Rdx
.calc_right_end_s:

        // h = min(Lh, Rh)
        cmp/gt  Rh, Lh
        bf/s    .scanline_prepare_s
        mov     Lh, h           // [delay slot]
        mov     Rh, h
        nop

.scanline_prepare_s:
        sub     h, Lh
        sub     h, Rh

.scanline_start_s:
        mov     Lx, Lptr
        mov     Rx, Rptr
        add     Ldx, Lx
        add     Rdx, Rx
        shlr16  Lptr            // Lptr = (Lx >> 16)
        shlr16  Rptr            // Rptr = (Rx >> 16)
        cmp/gt  Lptr, Rptr      // if (!(Rptr > Lptr)) skip zero length scanline
        bf/s    .scanline_end_s

        add     pixel, Lptr     // Lptr = pixel + (Lx >> 16)
        add     pixel, Rptr     // Rptr = pixel + (Rx >> 16)

.shade_pixel_s:
        mov.b   @Lptr, index
        mov.b   @(index, LMAP), index
        mov.b   index, @Lptr
        add     #1, Lptr
        cmp/gt  Lptr, Rptr
        bt      .shade_pixel_s

.scanline_end_s:
        dt      h

        mov.w   var_frameWidth_fs, tmp
        bf/s    .scanline_start_s
        add     tmp, pixel      // [delay slot] pixel += FRAME_WIDTH

        tst     Lh, Lh
        bf      .calc_right_start_s
        bra     .calc_left_start_s
        nop

#undef tmp
#undef Lh
#undef Rh
#undef Lptr
#undef pixel
#undef L
#undef R
#undef Lx
#undef Rx
#undef Ldx
#undef Rdx
#undef LMAP
#undef inv
#undef divLUT
#undef index
#undef h
#undef Ry
#undef Ly
#undef Rptr
#undef ih
