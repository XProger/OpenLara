#include "common_asm.inc"

e0  .req r0
e1  .req r1
e2  .req r2
e3  .req r3
m   .req e0
// FIQ regs
src .req r8
dst .req r9
e4  .req r10
e5  .req r11

.global matrixPush_asm
matrixPush_asm:
    fiq_on

    ldr m, =gMatrixPtr
    ldr src, [m]
    add dst, src, #(12 * 4)
    str dst, [m]

    ldmia src!, {e0, e1, e2, e3, e4, e5}
    stmia dst!, {e0, e1, e2, e3, e4, e5}

    ldmia src!, {e0, e1, e2, e3, e4, e5}
    stmia dst!, {e0, e1, e2, e3, e4, e5}

    fiq_off
    bx lr
