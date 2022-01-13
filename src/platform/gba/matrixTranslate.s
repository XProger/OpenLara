#include "common_asm.inc"

x   .req r0
y   .req r1
z   .req r2
e0  .req r3
e1  .req r4
e2  .req r5
v   .req r12
m   .req lr

.global matrixTranslateRel_asm
matrixTranslateRel_asm:
    stmfd sp!, {r4-r5, lr}

    ldr m, =gMatrixPtr
    ldr m, [m]

    // x
    ldmia m!, {e0, e1, e2, v}
    mla v, e0, x, v
    mla v, e1, y, v
    mla v, e2, z, v
    stmdb m, {v}

    // y
    ldmia m!, {e0, e1, e2, v}
    mla v, e0, x, v
    mla v, e1, y, v
    mla v, e2, z, v
    stmdb m, {v}

    // z
    ldmia m!, {e0, e1, e2, v}
    mla v, e0, x, v
    mla v, e1, y, v
    mla v, e2, z, v
    stmdb m, {v}

    ldmfd sp!, {r4-r5, lr}
    bx lr

.global matrixTranslateAbs_asm
matrixTranslateAbs_asm:
    stmfd sp!, {r4-r5, lr}

    ldr v, =gCameraViewPos
    ldmia v, {e0, e1, e2}
    sub x, x, e0
    sub y, y, e1
    sub z, z, e2

    ldr m, =gMatrixPtr
    ldr m, [m]

    // x
    ldmia m!, {e0, e1, e2}
    mul v, e0, x
    mla v, e1, y, v
    mla v, e2, z, v
    stmia m!, {v}

    // y
    ldmia m!, {e0, e1, e2}
    mul v, e0, x
    mla v, e1, y, v
    mla v, e2, z, v
    stmia m!, {v}

    // z
    ldmia m!, {e0, e1, e2}
    mul v, e0, x
    mla v, e1, y, v
    mla v, e2, z, v
    stmia m!, {v}

    ldmfd sp!, {r4-r5, lr}
    bx lr

.global matrixTranslateSet_asm
matrixTranslateSet_asm:
    stmfd sp!, {r4-r5, lr}

    ldr m, =gMatrixPtr
    ldr m, [m]

    // x
    ldmia m!, {e0, e1, e2}
    mul v, e0, x
    mla v, e1, y, v
    mla v, e2, z, v
    stmia m!, {v}

    // y
    ldmia m!, {e0, e1, e2}
    mul v, e0, x
    mla v, e1, y, v
    mla v, e2, z, v
    stmia m!, {v}

    // z
    ldmia m!, {e0, e1, e2}
    mul v, e0, x
    mla v, e1, y, v
    mla v, e2, z, v
    stmia m!, {v}

    ldmfd sp!, {r4-r5, lr}
    bx lr
