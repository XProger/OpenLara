#include "common.i"
SEG_MATH

#define v       r0
#define e0      r1
#define e1      r2
#define e2      r3
#define x       r4      // arg
#define y       r5      // arg
#define z       r6      // arg
#define m       r7
#define e       e0
#define tmp     e1


.align 4
.global _matrixTranslateRel_asm
_matrixTranslateRel_asm:
        mov.l   var_gMatrixPtr, m
        mov.l   @m, m

        // store xyz_ to the stack
        shll16  z
        shll16  y
        xtrct   x, y
        mov.l   z, @-sp
        mov.l   y, @-sp

        // x
        lds.l   @m+, MACL
        mov     sp, v
        mac.w   @v+, @m+
        mac.w   @v+, @m+
        mac.w   @v+, @m+
        add     #-6, m
        sts.l   MACL, @-m
        add     #12, m

        // y
        lds.l   @m+, MACL
        mov     sp, v
        mac.w   @v+, @m+
        mac.w   @v+, @m+
        mac.w   @v+, @m+
        add     #-6, m
        sts.l   MACL, @-m
        add     #12, m

        // z
        lds.l   @m+, MACL
        mov     sp, v
        mac.w   @v+, @m+
        mac.w   @v+, @m+
        mac.w   @v+, @m+
        add     #-6, m
        sts.l   MACL, @-m
        rts
        add     #8, sp


.align 4
.global _matrixTranslateAbs_asm
_matrixTranslateAbs_asm:
        mov.l   var_gCameraViewPos, v
        mov.l   @v+, e0
        mov.l   @v+, e1
        mov.l   @v+, e2
        sub     e0, x
        sub     e1, y
        sub     e2, z

        mov.l   var_gMatrixPtr, m
        mov.l   @m, m
        add     #M00, m

        // store yz to the stack
        shll16  z
        xtrct   y, z
        mov.l   z, @-sp

        // x
        mov.w   @m+, e
        mov     sp, v
        muls.w  x, e
        mac.w   @v+, @m+
        mac.w   @v+, @m+
        add     #-6, m
        sts.l   MACL, @-m
        add     #16, m

        // y
        mov.w   @m+, e
        mov     sp, v
        muls.w  x, e
        mac.w   @v+, @m+
        mac.w   @v+, @m+
        add     #-6, m
        sts.l   MACL, @-m
        add     #16, m

        // z
        mov.w   @m+, e
        mov     sp, v
        muls.w  x, e
        mac.w   @v+, @m+
        mac.w   @v+, @m+
        add     #-6, m
        sts.l   MACL, @-m
        rts
        add     #4, sp

.align 4
.global _matrixTranslateSet_asm
_matrixTranslateSet_asm:
        mov.l   var_gMatrixPtr, m
        mov.l   @m, m
        add     #M00, m

        // store yz to the stack
        shll16  z
        xtrct   y, z
        mov.l   z, @-sp

        // x
        mov.w   @m+, e
        mov     sp, v
        muls.w  x, e
        mac.w   @v+, @m+
        mac.w   @v+, @m+
        add     #-6, m
        sts.l   MACL, @-m
        add     #16, m

        // y
        mov.w   @m+, e
        mov     sp, v
        muls.w  x, e
        mac.w   @v+, @m+
        mac.w   @v+, @m+
        add     #-6, m
        sts.l   MACL, @-m
        add     #16, m

        // z
        mov.w   @m+, e
        mov     sp, v
        muls.w  x, e
        mac.w   @v+, @m+
        mac.w   @v+, @m+
        add     #-6, m
        sts.l   MACL, @-m
        rts
        add     #4, sp
        nop

var_gMatrixPtr:
        .long   _gMatrixPtr
var_gCameraViewPos:
        .long   _gCameraViewPos
