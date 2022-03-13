#include "common.i"
SEG_MATH

#define dst     r4      // arg
#define src     r5      // arg
#define e01     r6      // e0, e1
#define e2P     r7      // e2, pad

.align 4
.global _matrixSetBasis_asm
_matrixSetBasis_asm:
        mov.l   @(M00, src), e01
        mov.l   @(M02, src), e2P
        mov.l   e01, @(M00, dst)
        mov.l   e2P, @(M02, dst)

        mov.l   @(M10, src), e01
        mov.l   @(M12, src), e2P
        mov.l   e01, @(M10, dst)
        mov.l   e2P, @(M12, dst)

        mov.l   @(M20, src), e01
        mov.l   @(M22, src), e2P
        mov.l   e01, @(M20, dst)
        mov.l   e2P, @(M22, dst)

        rts
        nop
