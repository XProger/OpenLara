#include "common.i"
SEG_MATH

#define m       r0
#define ZZ      r1
#define NZ      r2

// word [ N Z Z ]  // rot
//      [ Z N Z ]  // rot
//      [ Z Z N ]  // rot
//      [ Z Z Z ]  // trans
// long [ NZ ZZ NZ ZZ NZ ZZ ]

.align 4
.global _matrixSetIdentity_asm
_matrixSetIdentity_asm:
        mov.l   var_gMatrixPtr, m
        mov.l   @m, m

        // ZZ = 0x 0000 0000
        mov     #0, ZZ

        // NZ = 0x 4000 0000
        mov     #64, NZ
        shll8   NZ
        shll16  NZ

        mov.l   NZ, @( 0, m)
        mov.l   ZZ, @( 4, m)
        mov.l   NZ, @( 8, m)
        mov.l   ZZ, @(12, m)
        mov.l   NZ, @(16, m)
        rts
        mov.l   ZZ, @(20, m)
        nop

var_gMatrixPtr:
        .long   _gMatrixPtr
