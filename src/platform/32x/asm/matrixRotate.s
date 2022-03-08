#include "common.i"
.text

.macro sincos idx, sin, cos
        shll2   \idx        
        mov.l   var_gSinCosTable, \sin
        mov.l   @(\idx, \sin), \sin
        exts.w  \sin, \cos
        swap.w  \sin, \sin
        exts.w  \sin, \sin
.endm

.macro rotxy x, y, sin, cos, tx, ty
        mul.l   \x, \sin
        sts     MACL, \tx
        mul.l   \y, \sin
        sts     MACL, \ty
        mul.l   \x, \cos
        sts     MACL, \x
        mul.l   \y, \cos
        sts     MACL, \y
        sub     \ty, \x
        add     \tx, \y
        // int(x) >> (FIXED_SHIFT = 14)
        shll2   \x
        swap.w  \x, \x
        exts.w  \x, \x
        // int(y) >> (FIXED_SHIFT = 14)
        shll2   \y
        swap.w  \y, \y
        exts.w  \y, \y
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
        nop


#define e0      r0
#define e1      r1
#define sin     r2
#define cos     r3
#define angle   r4      // arg
#define tx      r5
#define ty      r6
#define idx     e0
#define m       angle

.align 4
.global _matrixRotateX_asm
_matrixRotateX_asm:
        extu.w  angle, idx
        shlr2   idx
        shlr2   idx
        sincos  idx, sin, cos

        mov.l   var_gMatrixPtr, m
        mov.l   @m, m

        mov.l   @(M01, m), e0
        mov.l   @(M02, m), e1
        rotxy   e1, e0, sin, cos, tx, ty
        mov.l   e0, @(M01, m)
        mov.l   e1, @(M02, m)

        mov.l   @(M11, m), e0
        mov.l   @(M12, m), e1
        rotxy   e1, e0, sin, cos, tx, ty
        mov.l   e0, @(M11, m)
        mov.l   e1, @(M12, m)

        mov.l   @(M21, m), e0
        mov.l   @(M22, m), e1
        rotxy   e1, e0, sin, cos, tx, ty
        mov.l   e0, @(M21, m)
        rts
        mov.l   e1, @(M22, m)

.align 4
.global _matrixRotateY_asm
_matrixRotateY_asm:
        extu.w  angle, idx
        shlr2   idx
        shlr2   idx
        sincos  idx, sin, cos

        mov.l   var_gMatrixPtr, m
        mov.l   @m, m

        mov.l   @(M00, m), e0
        mov.l   @(M02, m), e1
        rotxy   e0, e1, sin, cos, tx, ty
        mov.l   e0, @(M00, m)
        mov.l   e1, @(M02, m)

        mov.l   @(M10, m), e0
        mov.l   @(M12, m), e1
        rotxy   e0, e1, sin, cos, tx, ty
        mov.l   e0, @(M10, m)
        mov.l   e1, @(M12, m)

        mov.l   @(M20, m), e0
        mov.l   @(M22, m), e1
        rotxy   e0, e1, sin, cos, tx, ty
        mov.l   e0, @(M20, m)
        rts
        mov.l   e1, @(M22, m)

.align 4
.global _matrixRotateZ_asm
_matrixRotateZ_asm:
        extu.w  angle, idx
        shlr2   idx
        shlr2   idx
        sincos  idx, sin, cos

        mov.l   var_gMatrixPtr, m
        mov.l   @m, m

        mov.l   @(M00, m), e0
        mov.l   @(M01, m), e1
        rotxy   e1, e0, sin, cos, tx, ty
        mov.l   e0, @(M00, m)
        mov.l   e1, @(M01, m)

        mov.l   @(M10, m), e0
        mov.l   @(M11, m), e1
        rotxy   e1, e0, sin, cos, tx, ty
        mov.l   e0, @(M10, m)
        mov.l   e1, @(M11, m)

        mov.l   @(M20, m), e0
        mov.l   @(M21, m), e1
        rotxy   e1, e0, sin, cos, tx, ty
        mov.l   e0, @(M20, m)
        rts
        mov.l   e1, @(M21, m)


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

        mov.l   var_gMatrixPtr, n
        mov.l   @n, n

        mov.l   @(M00, n), e00
        mov.l   @(M02, n), e02
        mov.l   @(M10, n), e10
        mov.l   @(M12, n), e12
        mov.l   @(M20, n), e20
        bt/s    .q_1
        mov.l   @(M22, n), e22

        tst     q, q
        bt/s    .q_0
        neg     e22, e22        // for q_0 and q_3
.q_3:
        neg     e02, e02
        neg     e12, e12
        mov.l   e02, @(M00, n)
        mov.l   e00, @(M02, n)
        mov.l   e12, @(M10, n)
        mov.l   e10, @(M12, n)
        mov.l   e22, @(M20, n)
        mov.l   e20, @(M22, n)
.q_2:
        rts
        nop
.q_0:
        neg     e00, e00
        neg     e02, e02
        neg     e10, e10
        neg     e12, e12
        neg     e20, e20
        mov.l   e00, @(M00, n)
        mov.l   e02, @(M02, n)
        mov.l   e10, @(M10, n)
        mov.l   e12, @(M12, n)
        mov.l   e20, @(M20, n)
        rts
        mov.l   e22, @(M22, n)
.q_1:
        neg     e00, e00
        neg     e10, e10
        neg     e20, e20
        mov.l   e02, @(M00, n)
        mov.l   e00, @(M02, n)
        mov.l   e12, @(M10, n)
        mov.l   e10, @(M12, n)
        mov.l   e22, @(M20, n)
        rts
        mov.l   e20, @(M22, n)

var_gMatrixPtr:
        .long   _gMatrixPtr
var_gSinCosTable:
        .long   _gSinCosTable
