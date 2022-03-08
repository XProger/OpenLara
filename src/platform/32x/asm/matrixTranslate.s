#include "common.i"
.text

#define m       r0
#define e0      r1
#define e1      r2
#define e2      r3
#define x       r4      // arg
#define y       r5      // arg
#define z       r6      // arg
#define v       r7

.align 4
.global _matrixTranslateRel_asm
_matrixTranslateRel_asm:
        mov.l   var_gMatrixPtr, m
        mov.l   @m, m

        mov     #1, v
        mov.l   v, @-sp
        mov.l   z, @-sp
        mov.l   y, @-sp
        mov.l   x, @-sp

        // x
        clrmac
        mov     sp, v
        mac.l   @v+, @m+
        mac.l   @v+, @m+
        mac.l   @v+, @m+
        mac.l   @v+, @m+
        sts.l   MACL, @-m
        add     #4, m

        // y
        clrmac
        mov     sp, v
        mac.l   @v+, @m+
        mac.l   @v+, @m+
        mac.l   @v+, @m+
        mac.l   @v+, @m+
        sts.l   MACL, @-m
        add     #4, m

        // z
        clrmac
        mov     sp, v
        mac.l   @v+, @m+
        mac.l   @v+, @m+
        mac.l   @v+, @m+
        mac.l   @v+, @m+
        sts.l   MACL, @-m

        rts
        add     #16, sp

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
        mov.l   z, @-sp
        mov.l   y, @-sp
        mov.l   x, @-sp

        mov.l   var_gMatrixPtr, m
        mov.l   @m, m

        // x
        clrmac
        mov     sp, v
        mac.l   @v+, @m+
        mac.l   @v+, @m+
        mac.l   @v+, @m+
        add     #4, m
        sts.l   MACL, @-m
        add     #4, m

        // y
        clrmac
        mov     sp, v
        mac.l   @v+, @m+
        mac.l   @v+, @m+
        mac.l   @v+, @m+
        add     #4, m
        sts.l   MACL, @-m
        add     #4, m

        // z
        clrmac
        mov     sp, v
        mac.l   @v+, @m+
        mac.l   @v+, @m+
        mac.l   @v+, @m+
        add     #4, m
        sts.l   MACL, @-m
        rts
        add     #12, sp

.align 4
.global _matrixTranslateSet_asm
_matrixTranslateSet_asm:
        mov.l   z, @-sp
        mov.l   y, @-sp
        mov.l   x, @-sp

        mov.l   var_gMatrixPtr, m
        mov.l   @m, m

        // x
        clrmac
        mov     sp, v
        mac.l   @v+, @m+
        mac.l   @v+, @m+
        mac.l   @v+, @m+
        add     #4, m
        sts.l   MACL, @-m
        add     #4, m

        // y
        clrmac
        mov     sp, v
        mac.l   @v+, @m+
        mac.l   @v+, @m+
        mac.l   @v+, @m+
        add     #4, m
        sts.l   MACL, @-m
        add     #4, m

        // z
        clrmac
        mov     sp, v
        mac.l   @v+, @m+
        mac.l   @v+, @m+
        mac.l   @v+, @m+
        add     #4, m
        sts.l   MACL, @-m
        rts
        add     #12, sp

var_gMatrixPtr:
        .long   _gMatrixPtr
var_gCameraViewPos:
        .long   _gCameraViewPos
