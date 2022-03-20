#include "common.i"
SEG_MATH

#define e8      r0      // [ e22 ]
#define e01     r1      // [ e00, e01 ]
#define e23     r2      // [ e02, e10 ]
#define e45     r3      // [ e11, e12 ]
#define dst     r4      // arg
#define src     r5      // arg
#define e67     r6      // [ e20, e21 ]

.align 4
.global _matrixSetBasis_asm
_matrixSetBasis_asm:
        mov.l   @(M00, src), e01
        mov.l   @(M02, src), e23
        mov.l   @(M11, src), e45
        mov.l   @(M20, src), e67
        mov.w   @(M22, src), e8

        mov.l   e01, @(M00, dst)
        mov.l   e23, @(M02, dst)
        mov.l   e45, @(M11, dst)
        mov.l   e67, @(M20, dst)
        rts
        mov.w   e8, @(M22, dst)
