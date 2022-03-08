#include "common.i"
.text

#define dst     r4
#define src     r5

#define e00     r0
#define e01     r1
#define e02     r2
#define e10     r6

#define e11     e00
#define e12     e01
#define e20     e02
#define e21     e10

#define e22     e00

.align 4
.global _matrixSetBasis_asm
_matrixSetBasis_asm:
        mov.l   @(M00, src), e00
        mov.l   @(M01, src), e01
        mov.l   @(M02, src), e02
        mov.l   @(M10, src), e10

        mov.l   e00, @(M00, dst)
        mov.l   e01, @(M01, dst)
        mov.l   e02, @(M02, dst)
        mov.l   e10, @(M10, dst)

        mov.l   @(M11, src), e11
        mov.l   @(M12, src), e12
        mov.l   @(M20, src), e20
        mov.l   @(M21, src), e21

        mov.l   e11, @(M11, dst)
        mov.l   e12, @(M12, dst)
        mov.l   e20, @(M20, dst)
        mov.l   e21, @(M21, dst)

        mov.l   @(M22, src), e22
        rts
        mov.l   e22, @(M22, dst)
