#include "common.i"
SEG_MATH

#define ptr     r0
#define m       r1
#define e0      r2
#define e1      r3
#define e2      r4
#define e3      r5
#define e4      r6
#define e5      r7

.align 4
.global _matrixPush_asm
_matrixPush_asm:
        mov.l   var_gMatrixPtr, ptr
        mov.l   @ptr, m

        mov.l   @m+, e0
        mov.l   @m+, e1
        mov.l   @m+, e2
        mov.l   @m+, e3
        mov.l   @m+, e4
        mov.l   @m+, e5

        mov.l   e0, @( 0, m)
        mov.l   e1, @( 4, m)
        mov.l   e2, @( 8, m)
        mov.l   e3, @(12, m)
        mov.l   e4, @(16, m)
        mov.l   e5, @(20, m)

        rts
        mov.l   m, @ptr

var_gMatrixPtr:
        .long   _gMatrixPtr
