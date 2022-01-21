#include "common_asm.inc"

e0  .req r0
e1  .req r1
e2  .req r2
e3  .req r3
e4  .req r12
m   .req lr

.global matrixSetIdentity_asm
matrixSetIdentity_asm:
    stmfd sp!, {lr}
    ldr m, =gMatrixPtr
    ldr m, [m]
    mov e0, #0x4000
    mov e1, #0
    mov e2, #0
    mov e3, #0
    mov e4, #0

    // row-major
    // e0 e1 e2 e3
    // e4 e0 e1 e2
    // e3 e4 e0 e1

    stmia m!, {e0, e1, e2, e3, e4}
    stmia m!, {e0, e1, e2, e3, e4}
    stmia m!, {e0, e1}

    ldmfd sp!, {lr}
    bx lr
