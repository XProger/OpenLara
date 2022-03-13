#include "common.i"
SEG_MATH

#define m       r0
#define ZZ      r1
#define ZN      r2
#define NZ      r3

.align 4
.global _matrixSetIdentity_asm
_matrixSetIdentity_asm:
        mov.l   var_gMatrixPtr, m
        mov.l   @m, m
        add     #(12 * 3), m

        // ZZ = 0x0000 0000
        mov     #0, ZZ
        // NZ = 0x0000 4000
        mov     #64, ZN
        shll8   ZN
        // ZN = 0x4000 0000
        swap.w  ZN, NZ

//       e3 | e0 e1 | e2 pad
//     [ ZZ |  N  Z |  Z  Z ] - row 0
//     [ ZZ |  Z  N |  Z  Z ] - row 1
//     [ ZZ |  Z  Z |  N  Z ] - row 2
//     Z = 0x0000
//     N = 0x4000 (1 << FIXED_SHIFT)

        // row[2]
        mov.l   NZ, @-m
        mov.l   ZZ, @-m
        mov.l   ZZ, @-m

        // row[1]
        mov.l   ZZ, @-m
        mov.l   ZN, @-m
        mov.l   ZZ, @-m

        // row[0]
        mov.l   ZZ, @-m
        mov.l   NZ, @-m
        mov.l   ZZ, @-m

        rts
        nop

var_gMatrixPtr:
        .long   _gMatrixPtr
