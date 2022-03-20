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

.macro push
        // store xyz_ to the stack
        shll16  z
        xtrct   y, z
        mov.l   z, @-sp
.endm

.macro pop
        add     #4, sp
.endm

.macro Inc offset
        mov.w   @m+, v
        mov     sp, tmp
        muls.w  x, v
        mac.w   @m+, @tmp+
        mac.w   @m+, @tmp+
        mov.w   @(\offset, m), v
        sts     MACL, tmp
        shlr14  tmp
        add     tmp, v          // trash in high word, that's fine!
        mov.w   v, @(\offset, m)
.endm

.macro Set offset
        mov.w   @m+, v
        mov     sp, tmp
        muls.w  x, v
        mac.w   @m+, @tmp+
        mac.w   @m+, @tmp+
        sts     MACL, v
        shlr14  v
        mov.w   v, @(\offset, m)
.endm

.macro transform func
        \func (M03 - 6)          // x
        \func (M13 - 12)         // y
        \func (M23 - 18)         // z
.endm

.align 4
.global _matrixTranslateRel_asm
_matrixTranslateRel_asm:
        mov.l   var_gMatrixPtr, m
        mov.l   @m, m

        push
        transform Inc
        rts
        pop

.align 2
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

        push
        transform Set
        rts
        pop

.align 2
.global _matrixTranslateSet_asm
_matrixTranslateSet_asm:
        mov.l   var_gMatrixPtr, m
        mov.l   @m, m

        push
        transform Set
        rts
        pop

.align 2
var_gMatrixPtr:
        .long   _gMatrixPtr
var_gCameraViewPos:
        .long   _gCameraViewPos
