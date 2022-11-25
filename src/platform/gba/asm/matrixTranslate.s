#include "common_asm.inc"

x   .req r0     // arg
y   .req r1     // arg
z   .req r2     // arg
m   .req r3
// FIQ regs
e0  .req r8
e1  .req r9
e2  .req r10
vx  .req r11
e3  .req r12
e4  .req r13
e5  .req r14

v   .req vx
vy  .req e0
vp  .req e1
e6  .req e2
e7  .req vx
e8  .req e3
vz  .req e4

.global matrixTranslateRel_asm
matrixTranslateRel_asm:
    fiq_on

    ldr m, =gMatrixPtr
    ldr m, [m]

    // x
    ldmia m!, {e0, e1, e2, vx, e3, e4, e5}
    mla vx, e0, x, vx
    mla vx, e1, y, vx
    mla vx, e2, z, vx
    str vx, [m, #-16]

    // y
    mul vy, e3, x
    mla vy, e4, y, vy
    mla vy, e5, z, vy
    ldmia m, {vp, e6, e7, e8, vz}
    add vy, vy, vp
    str vy, [m]

    // z
    mla vz, e6, x, vz
    mla vz, e7, y, vz
    mla vz, e8, z, vz
    str vz, [m, #16]

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
