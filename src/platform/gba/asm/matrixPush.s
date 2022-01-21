#include "common_asm.inc"

e0  .req r0
e1  .req r1
e2  .req r2
e3  .req r3
m   .req e0
src .req r12
dst .req lr

.global matrixPush_asm
matrixPush_asm:
    stmfd sp!, {lr}
    ldr m, =gMatrixPtr
    ldr src, [m]
    add dst, src, #(12 * 4)
    str dst, [m]

    ldmia src!, {e0, e1, e2, e3}
    stmia dst!, {e0, e1, e2, e3}

    ldmia src!, {e0, e1, e2, e3}
    stmia dst!, {e0, e1, e2, e3}

    ldmia src!, {e0, e1, e2, e3}
    stmia dst!, {e0, e1, e2, e3}

    ldmfd sp!, {lr}
    bx lr
