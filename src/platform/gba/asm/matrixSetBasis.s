#include "common_asm.inc"

dst .req r0     // arg
src .req r1     // arg

e0  .req r2
e1  .req r3
e2  .req r12

.global matrixSetBasis_asm
matrixSetBasis_asm:
    // row-major
    // e0 e1 e2 x
    // e0 e1 e2 y
    // e0 e1 e2 z

    ldmia src, {e0, e1, e2}
    stmia dst, {e0, e1, e2}
    add src, #(4 * 4)
    add dst, #(4 * 4)

    ldmia src, {e0, e1, e2}
    stmia dst, {e0, e1, e2}
    add src, #(4 * 4)
    add dst, #(4 * 4)

    ldmia src, {e0, e1, e2}
    stmia dst, {e0, e1, e2}
    add src, #(4 * 4)
    add dst, #(4 * 4)

    bx lr
