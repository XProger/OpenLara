#include "common.i"
.text

#define m       r0
#define e0      r1
#define e1      r2

.align 4
.global _matrixSetIdentity_asm
_matrixSetIdentity_asm:
        mov.l   var_gMatrixPtr, m
        mov.l   @m, m
        mov     #0, e0
        mov     #64, e1
        shll8   r1

        // row[0]
        mov.l   e1, @(M00, m)
        mov.l   e0, @(M01, m)
        mov.l   e0, @(M02, m)
        mov.l   e0, @(M03, m)

        // row[1]
        mov.l   e0, @(M10, m)
        mov.l   e1, @(M11, m)
        mov.l   e0, @(M12, m)
        mov.l   e0, @(M13, m)

        // row[2]
        mov.l   e0, @(M20, m)
        mov.l   e0, @(M21, m)
        mov.l   e1, @(M22, m)

        rts
        mov.l   e0, @(M23, m)

var_gMatrixPtr:
        .long   _gMatrixPtr
