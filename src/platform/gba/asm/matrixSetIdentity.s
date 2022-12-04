#include "common_asm.inc"

e0  .req r0
e1  .req r1
e2  .req r2
e3  .req r3
// FIQ regs
e4  .req r8
e5  .req r9
e6  .req r10
m   .req r11

.global matrixSetIdentity_asm
matrixSetIdentity_asm:
    fiq_on
    ldr m, =gMatrixPtr
    ldr m, [m]
    mov e0, #0x4000
    mov e1, #0
    mov e2, #0
    mov e3, #0
    mov e4, #0
    mov e5, #0x4000
    mov e6, #0

    // row-major
    // e0 e1 e2 e3
    // e4 e5 e6 e2
    // e3 e4 e5 e6

    stmia m!, {e0, e1, e2, e3, e4, e5, e6}
    stmia m!, {e2, e3, e4, e5, e6}

    fiq_off
    bx lr
