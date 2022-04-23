#include "common_asm.inc"

x   .req r0     // arg
y   .req r1     // arg
z   .req r2     // arg
m   .req r3
// FIQ regs
e0  .req r8
e1  .req r9
e2  .req r10
v   .req r11

.global matrixTranslateRel_asm
matrixTranslateRel_asm:
    fiq_on

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

    fiq_off
    bx lr

.global matrixTranslateAbs_asm
matrixTranslateAbs_asm:
    fiq_on

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

    fiq_off
    bx lr

.global matrixTranslateSet_asm
matrixTranslateSet_asm:
    fiq_on

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

    fiq_off
    bx lr
