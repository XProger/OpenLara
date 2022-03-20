#include "common.i"
SEG_MATH

#define m0      r0
#define m1      r1
#define m2      r2
#define n0      r3
#define n       r4      // arg
#define pmul    r5      // arg
#define pdiv    r6      // arg
#define n1      pdiv
#define n2      r7
#define m       r8
#define tmp     m0
#define divLUT  m1

.macro load
        mov.w   @m+, m0
        mov.w   @m+, m1
        mov.w   @m+, m2
        mov.w   @n+, n0
        mov.w   @n+, n1
        mov.w   @n+, n2
.endm

.macro store
        mov.w   m2, @-m
        mov.w   m1, @-m
        mov.w   m0, @-m
.endm

.macro next
        add     #6, m
.endm

.macro _1_2 // a = (a + b) / 2
        load
        add     n0, m0
        add     n1, m1
        add     n2, m2
        shar    m0
        shar    m1
        shar    m2
        store
.endm

.macro _1_4 // a = a + (b - a) / 4
        load
        sub     m0, n0
        sub     m1, n1
        sub     m2, n2
        shar    n0
        shar    n1
        shar    n2
        shar    n0
        shar    n1
        shar    n2
        add     n0, m0
        add     n1, m1
        add     n2, m2
        store
.endm

.macro _3_4 // a = b - (b - a) / 4 = b + (a - b) / 4
        load
        sub     n0, m0
        sub     n1, m1
        sub     n2, m2
        shar    m0
        shar    m1
        shar    m2
        shar    m0
        shar    m1
        shar    m2
        add     n0, m0
        add     n1, m1
        add     n2, m2
        store
.endm

.macro masr_8 x
        muls.w  \x, pmul
        sts     MACL, \x
        shlr8   \x
        exts.w  \x, \x
.endm

.macro _X_Y // a = a + (b - a) * mul / div
        load
        sub     m0, n0
        sub     m1, n1
        sub     m2, n2

        masr_8  n0
        masr_8  n1
        masr_8  n2

        add     n0, m0
        add     n1, m1
        add     n2, m2
        store
.endm

.macro lerp func
        \func   // e00, e01, e02
        next
        \func   // e10, e11, e12
        next
        \func   // e20, e21, e22
.endm


.align 4
.global _matrixLerp_asm
_matrixLerp_asm:
        mov.l   r8, @-sp
        mov.l   var_gMatrixPtr, m
        mov.l   @m, m
        mov     pdiv, tmp
.check_2:
        cmp/eq  #2, tmp
        bf/s    .check_XY
        cmp/eq  #4, tmp
.m1_d2:
        lerp    _1_2
        rts
        mov.l   @sp+, r8
.check_XY:
        bt      .check_4
        bra     .mX_dY
        nop
.check_4:
        mov     pmul, tmp
        cmp/eq  #2, tmp
        bt      .m1_d2          // 2/4 = 1/2
        cmp/eq  #1, tmp
        bt      .m1_d4
.m3_d4:
        lerp    _3_4
        rts
        mov.l   @sp+, r8
.m1_d4:
        lerp    _1_4
        rts
        mov.l   @sp+, r8
.mX_dY:
        mov.l   var_divTable, divLUT
        shll    tmp
        mov.w   @(tmp, divLUT), tmp     // tmp = pdiv
        mulu.w  tmp, pmul
        sts     MACL, pmul
        shlr8   pmul

        lerp    _X_Y
        rts
        mov.l   @sp+, r8
        nop

var_gMatrixPtr:
        .long   _gMatrixPtr     // skip e03
var_divTable:
        .long   _divTable
