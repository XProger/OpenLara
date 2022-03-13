#include "common.i"
SEG_MATH

#define ptr     r0
#define x       r1
#define y       r2
#define z       r3
#define m       r5

.macro COPY_ROW
        mov.l   @m+, x
        mov.l   @m+, y
        mov.l   @m+, z
        mov.l   x, @(24, m)
        mov.l   y, @(28, m)
        mov.l   z, @(32, m)
.endm

.align 4
.global _matrixPush_asm
_matrixPush_asm:
        mov.l   var_gMatrixPtr, ptr
        mov.l   @ptr, m

        COPY_ROW  // row[0]
        COPY_ROW  // row[1]
        COPY_ROW  // row[2]

        rts
        mov.l   m, @ptr

var_gMatrixPtr:
        .long   _gMatrixPtr
