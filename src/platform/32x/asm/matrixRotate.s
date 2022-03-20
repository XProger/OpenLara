#include "common.i"
SEG_MATH

// r0 is required
.macro sincos angle, sin, cos
        extu.w  \angle, r0
        shlr2   r0
        shlr2   r0
        shll2   r0
        mov.l   var_gSinCosTable, \cos
        mov.l   @(r0, \cos), \cos
        swap.w  \cos, \sin
        // we don't need to exts.w's of sin & cos because of muls.w
.endm

.macro rotxy x, y, sin, cos, tx, ty
        muls.w  \x, \sin
        sts     MACL, \tx
        muls.w  \y, \sin
        sts     MACL, \ty
        muls.w  \x, \cos
        sts     MACL, \x
        muls.w  \y, \cos
        sts     MACL, \y
        sub     \ty, \x
        add     \tx, \y
        shlr14  \x      // int(x) >> (FIXED_SHIFT = 14)
        shlr14  \y      // int(y) >> (FIXED_SHIFT = 14)
.endm


#define angleX  r4      // arg
#define angleY  r5      // arg
#define angleZ  r6      // arg
#define tmpX    r7      // unused in subroutines
#define tmpZ    r8
#define arg     angleX

.align 4
.global _matrixRotateYXZ_asm
_matrixRotateYXZ_asm:
        mov.l   r8, @-sp
        sts.l   pr, @-sp
        mov     angleX, tmpX
        mov     angleZ, tmpZ
.rotY:
        tst     angleY, angleY
        bt      .rotX
        bsr     _matrixRotateY_asm
        mov     angleY, arg
.rotX:
        tst     tmpX, tmpX
        bt      .rotZ
        bsr     _matrixRotateX_asm
        mov     tmpX, arg
.rotZ:
        tst     tmpZ, tmpZ
        bt      .done
        bsr     _matrixRotateZ_asm
        mov     tmpZ, arg
.done:
        lds.l   @sp+, pr
        rts
        mov.l   @sp+, r8

#define m       r0
#define e1      r1
#define sin     r2
#define cos     r3
#define angle   r4      // arg
#define tx      r5
#define ty      r6
#define e0      angle

.align 2
.global _matrixRotateX_asm
_matrixRotateX_asm:
        sincos  angle, sin, cos

        mov.l   var_gMatrixPtr, m
        mov.l   @m, m
        add     #M01, m

        mov.w   @m+, e0
        mov.w   @m+, e1
        rotxy   e1, e0, sin, cos, tx, ty
        mov.w   e1, @-m
        mov.w   e0, @-m

        add     #6, m

        mov.w   @m+, e0
        mov.w   @m+, e1
        rotxy   e1, e0, sin, cos, tx, ty
        mov.w   e1, @-m
        mov.w   e0, @-m

        add     #6, m

        mov.w   @m+, e0
        mov.w   @m+, e1
        rotxy   e1, e0, sin, cos, tx, ty
        mov.w   e1, @-m
        rts
        mov.w   e0, @-m

.align 2
.global _matrixRotateY_asm
_matrixRotateY_asm:
        sincos  angle, sin, cos

        mov.l   var_gMatrixPtr, m
        mov.l   @m, m

        mov.w   @m+, e0
        add     #2, m
        mov.w   @m+, e1
        rotxy   e0, e1, sin, cos, tx, ty
        mov.w   e1, @-m
        add     #-2, m
        mov.w   e0, @-m

        add     #6, m

        mov.w   @m+, e0
        add     #2, m
        mov.w   @m+, e1
        rotxy   e0, e1, sin, cos, tx, ty
        mov.w   e1, @-m
        add     #-2, m
        mov.w   e0, @-m

        add     #6, m

        mov.w   @m+, e0
        add     #2, m
        mov.w   @m+, e1
        rotxy   e0, e1, sin, cos, tx, ty
        mov.w   e1, @-m
        add     #-2, m
        rts
        mov.w   e0, @-m

.align 2
.global _matrixRotateZ_asm
_matrixRotateZ_asm:
        sincos  angle, sin, cos

        mov.l   var_gMatrixPtr, m
        mov.l   @m, m

        mov.w   @m+, e0
        mov.w   @m+, e1
        rotxy   e1, e0, sin, cos, tx, ty
        mov.w   e1, @-m
        mov.w   e0, @-m

        add     #6, m

        mov.w   @m+, e0
        mov.w   @m+, e1
        rotxy   e1, e0, sin, cos, tx, ty
        mov.w   e1, @-m
        mov.w   e0, @-m

        add     #6, m

        mov.w   @m+, e0
        mov.w   @m+, e1
        rotxy   e1, e0, sin, cos, tx, ty
        mov.w   e1, @-m
        rts
        mov.w   e0, @-m

.align 2
var_gMatrixPtr:
        .long   _gMatrixPtr
var_gSinCosTable:
        .long   _gSinCosTable

#define q       r0
#define n       r1
#define e00     r2
#define e02     r3
#define qarg    r4      // arg
#define e10     r5
#define e12     r6
#define e20     r7
#define e22     qarg

.align 4
.global _matrixRotateYQ_asm
_matrixRotateYQ_asm:
        mov     qarg, q
        cmp/eq  #2, q
        bt/s    .q_2
        cmp/eq  #1, q

        mov.l   var_gMatrixPtr_YQ, n
        mov.l   @n, n

        mov.w   @n+, e00
        add     #2, n
        mov.w   @n+, e02

        mov.w   @n+, e10
        add     #2, n
        mov.w   @n+, e12

        mov.w   @n+, e20
        add     #2, n
        bt/s    .q_1
        mov.w   @n+, e22

        tst     q, q
        bt/s    .q_0
        neg     e22, e22        // for q_0 and q_3
.q_3:
        neg     e02, e02
        neg     e12, e12
        mov.w   e20, @-n
        add     #-2, n
        mov.w   e22, @-n
        mov.w   e10, @-n
        add     #-2, n
        mov.w   e12, @-n
        mov.w   e00, @-n
        add     #-2, n
        mov.w   e02, @-n
.q_2:
        rts
.q_0:
        neg     e00, e00
        neg     e02, e02
        neg     e10, e10
        neg     e12, e12
        neg     e20, e20
        mov.w   e22, @-n
        add     #-2, n
        mov.w   e20, @-n
        mov.w   e12, @-n
        add     #-2, n
        mov.w   e10, @-n
        mov.w   e02, @-n
        add     #-2, n
        rts
        mov.w   e00, @-n
.q_1:
        neg     e00, e00
        neg     e10, e10
        neg     e20, e20
        mov.w   e20, @-n
        add     #-2, n
        mov.w   e22, @-n
        mov.w   e10, @-n
        add     #-2, n
        mov.w   e12, @-n
        mov.w   e00, @-n
        add     #-2, n
        rts
        mov.w   e02, @-n

.align 2
var_gMatrixPtr_YQ:
        .long   _gMatrixPtr