#include "common_asm.inc"

.macro sincos angle, sin, cos
    ldr \sin, =gSinCosTable
    ldr \sin, [\sin, \angle, lsl #2]
    mov \cos, \sin, lsl #16
    mov \cos, \cos, asr #16
    mov \sin, \sin, asr #16
.endm

.macro sincosLUT lut, angle, sin, cos
    ldr \sin, [\lut, \angle, lsl #2]
    mov \cos, \sin, lsl #16
    mov \cos, \cos, asr #16
    mov \sin, \sin, asr #16
.endm

.macro rotxy x, y, sin, cos, t
    mul \t, \y, \cos
    mla \t, \x, \sin, \t
    mul \x, \cos, \x
    mul \y, \sin, \y
    sub \x, \y
    mov \y, \t, asr #FIXED_SHIFT
    mov \x, \x, asr #FIXED_SHIFT
.endm

angle   .req r0     // arg
s       .req r1
c       .req r2
v       .req r3
// FIQ regs
e0      .req r8
e1      .req r9
m       .req angle

.global matrixRotateX_asm
matrixRotateX_asm:
    fiq_on

    mov angle, angle, lsl #16
    mov angle, angle, lsr #20

    sincos angle, s, c

    ldr m, =gMatrixPtr
    ldr m, [m]

    add m, m, #4    // skip first column
    ldmia m, {e0, e1}
    rotxy e1, e0, s, c, v
    stmia m, {e0, e1}

    add m, #(4 * 4)
    ldmia m, {e0, e1}
    rotxy e1, e0, s, c, v
    stmia m, {e0, e1}

    add m, #(4 * 4)
    ldmia m, {e0, e1}
    rotxy e1, e0, s, c, v
    stmia m, {e0, e1}

    fiq_off
    bx lr

.global matrixRotateY_asm
matrixRotateY_asm:
    fiq_on

    mov angle, angle, lsl #16
    mov angle, angle, lsr #20

    sincos angle, s, c

    ldr m, =gMatrixPtr
    ldr m, [m]

    ldr e0, [m, #0]
    ldr e1, [m, #8]
    rotxy e0, e1, s, c, v
    str e0, [m], #8
    str e1, [m], #8

    ldr e0, [m, #0]
    ldr e1, [m, #8]
    rotxy e0, e1, s, c, v
    str e0, [m], #8
    str e1, [m], #8

    ldr e0, [m, #0]
    ldr e1, [m, #8]
    rotxy e0, e1, s, c, v
    str e0, [m], #8
    str e1, [m], #8

    fiq_off
    bx lr

.global matrixRotateZ_asm
matrixRotateZ_asm:
    fiq_on

    mov angle, angle, lsl #16
    mov angle, angle, lsr #20

    sincos angle, s, c

    ldr m, =gMatrixPtr
    ldr m, [m]

    ldmia m, {e0, e1}
    rotxy e1, e0, s, c, v
    stmia m, {e0, e1}

    add m, #(4 * 4)
    ldmia m, {e0, e1}
    rotxy e1, e0, s, c, v
    stmia m, {e0, e1}

    add m, #(4 * 4)
    ldmia m, {e0, e1}
    rotxy e1, e0, s, c, v
    stmia m, {e0, e1}

    fiq_off
    bx lr

angleX  .req r0     // arg
angleY  .req r1     // arg
angleZ  .req r2     // arg
e00     .req r3
e01     .req r4
e02     .req r5
e10     .req r6
scLUT   .req r7
// FIQ regs
e11     .req r8
e12     .req r9
e20     .req r10
e21     .req r11
tmp     .req r12
e22     .req r13
sinX    .req r14
sinY    .req sinX
sinZ    .req sinX
cosX    .req angleX
cosY    .req angleY
cosZ    .req angleZ
mask    .req tmp
mm      .req tmp

.global matrixRotateYXZ_asm, matrixRotateYXZ_fast_asm
matrixRotateYXZ_asm:
    mov mask, #0xFF
    orr mask, mask, #0xF00  ; mask = 0xFFF

    and angleX, mask, angleX, lsr #4
    and angleY, mask, angleY, lsr #4
    and angleZ, mask, angleZ, lsr #4

matrixRotateYXZ_fast_asm:   // routine for pre-shifted angles
    orr mask, angleX, angleY
    orrs mask, mask, angleZ
    bxeq lr

    stmfd sp!, {r4-r7}
    fiq_on

    ldr scLUT, =gSinCosTable

    ldr mm, =gMatrixPtr
    ldr mm, [mm]
    ldmia mm, {e00, e01, e02}
    add mm, #(4 * 4)
    ldmia mm, {e10, e11, e12}
    add mm, #(4 * 4)
    ldmia mm, {e20, e21, e22}

.rotY:
    cmp angleY, #0
    beq .rotX

    sincosLUT scLUT, angleY, sinY, cosY

    rotxy e00, e02, sinY, cosY, tmp
    rotxy e10, e12, sinY, cosY, tmp
    rotxy e20, e22, sinY, cosY, tmp

.rotX:
    cmp angleX, #0
    beq .rotZ

    sincosLUT scLUT, angleX, sinX, cosX

    rotxy e02, e01, sinX, cosX, tmp
    rotxy e12, e11, sinX, cosX, tmp
    rotxy e22, e21, sinX, cosX, tmp

.rotZ:
    cmp angleZ, #0
    beq .done

    sincosLUT scLUT, angleZ, sinZ, cosZ

    rotxy e01, e00, sinZ, cosZ, tmp
    rotxy e11, e10, sinZ, cosZ, tmp
    rotxy e21, e20, sinZ, cosZ, tmp

.done:
    ldr mm, =gMatrixPtr
    ldr mm, [mm]

    stmia mm, {e00, e01, e02}
    add mm, #(4 * 4)
    stmia mm, {e10, e11, e12}
    add mm, #(4 * 4)
    stmia mm, {e20, e21, e22}

    fiq_off
    ldmfd sp!, {r4-r7}
    bx lr

q   .req r0     // arg
n   .req r1
mx  .req r3
my  .req q

.global matrixRotateYQ_asm
matrixRotateYQ_asm:
    cmp q, #2
    bxeq lr

    ldr n, =gMatrixPtr
    ldr n, [n]

    cmp q, #0
    beq .q_0
    cmp q, #1
    beq .q_1

.q_3:
    ldr mx, [n, #0]
    ldr my, [n, #8]
    rsb my, my, #0
    str my, [n, #0]
    str mx, [n, #8]

    ldr mx, [n, #16]
    ldr my, [n, #24]
    rsb my, my, #0
    str my, [n, #16]
    str mx, [n, #24]

    ldr mx, [n, #32]
    ldr my, [n, #40]
    rsb my, my, #0
    str my, [n, #32]
    str mx, [n, #40]
    bx lr

.q_0:
    ldr mx, [n, #0]
    ldr my, [n, #8]
    rsb mx, mx, #0
    rsb my, my, #0
    str mx, [n, #0]
    str my, [n, #8]

    ldr mx, [n, #16]
    ldr my, [n, #24]
    rsb mx, mx, #0
    rsb my, my, #0
    str mx, [n, #16]
    str my, [n, #24]

    ldr mx, [n, #32]
    ldr my, [n, #40]
    rsb mx, mx, #0
    rsb my, my, #0
    str mx, [n, #32]
    str my, [n, #40]
    bx lr

.q_1:
    ldr mx, [n, #0]
    ldr my, [n, #8]
    rsb mx, mx, #0
    str my, [n, #0]
    str mx, [n, #8]

    ldr mx, [n, #16]
    ldr my, [n, #24]
    rsb mx, mx, #0
    str my, [n, #16]
    str mx, [n, #24]

    ldr mx, [n, #32]
    ldr my, [n, #40]
    rsb mx, mx, #0
    str my, [n, #32]
    str mx, [n, #40]
    bx lr
