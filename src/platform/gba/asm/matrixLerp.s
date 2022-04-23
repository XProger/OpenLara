#include "common_asm.inc"

n       .req r0     // arg
pmul    .req r1     // arg
pdiv    .req r2     // arg
// FIQ regs
m0      .req r8
m1      .req r9
m2      .req r10
n0      .req r11
n1      .req r12
n2      .req r13
m       .req r14
tmp     .req m0

.macro load
    ldmia m, {m0, m1, m2}
    ldmia n, {n0, n1, n2}
.endm

.macro store
    stmia m, {m0, m1, m2}
.endm

.macro next
    add m, m, #16
    add n, n, #16
.endm

.macro _1_2 // a = (a + b) / 2
    load
    add m0, m0, n0
    add m1, m1, n1
    add m2, m2, n2
    mov m0, m0, asr #1
    mov m1, m1, asr #1
    mov m2, m2, asr #1
    store
.endm

.macro _1_4 // a = a + (b - a) / 4
    load
    sub n0, n0, m0
    sub n1, n1, m1
    sub n2, n2, m2
    add m0, m0, n0, asr #2
    add m1, m1, n1, asr #2
    add m2, m2, n2, asr #2
    store
.endm

.macro _3_4 // a = b - (b - a) / 4
    load
    sub m0, n0, m0
    sub m1, n1, m1
    sub m2, n2, m2
    sub m0, n0, m0, asr #2
    sub m1, n1, m1, asr #2
    sub m2, n2, m2, asr #2
    store
.endm

.macro _X_Y // a = a + (b - a) * mul / div
    load
    sub n0, n0, m0
    sub n1, n1, m1
    sub n2, n2, m2
    mul n0, pmul, n0
    mul n1, pmul, n1
    mul n2, pmul, n2
    add m0, m0, n0, asr #8
    add m1, m1, n1, asr #8
    add m2, m2, n2, asr #8
    store
.endm

.macro lerp func
    \func    // e00, e01, e02
    next
    \func    // e10, e11, e12
    next
    \func    // e20, e21, e22
.endm

.global matrixLerp_asm
matrixLerp_asm:
    fiq_on
    ldr m, =gMatrixPtr
    ldr m, [m]
.check_2:
    cmp pdiv, #2
    beq .m1_d2
.check_4:
    cmp pdiv, #4
    bne .mX_dY
    cmp pmul, #1
    beq .m1_d4
    cmp pmul, #2
    beq .m1_d2      // 2/4 = 1/2
.m3_d4:
    lerp _3_4
    b .done
.m1_d4:
    lerp _1_4
    b .done
.m1_d2:
    lerp _1_2
    b .done
.mX_dY:
    divLUT tmp, pdiv
    mul tmp, pmul, tmp
    mov pmul, tmp, asr #8
    lerp _X_Y
.done:
    fiq_off
    bx lr
