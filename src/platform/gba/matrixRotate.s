#include "common_asm.inc"

.macro sincos angle, sin, cos
    ldr \sin, =gSinCosTable
    ldr \sin, [\sin, \angle, lsl #2]
    mov \cos, \sin, lsl #16
    mov \cos, \cos, asr #16
    mov \sin, \sin, asr #16
.endm

.macro rotxy x, y, sin, cos, t
    mul \t, \y, \cos
    mla \t, \x, \sin, \t
    mul \x, \cos, \x
    rsb \y, \y, #0
    mla \x, \y, \sin, \x
    mov \y, \t, asr #FIXED_SHIFT
    mov \x, \x, asr #FIXED_SHIFT
.endm

angle   .req r0
e0      .req r1
e1      .req r2
s       .req r3
c       .req r12
v       .req lr
m       .req angle

.global matrixRotateX_asm
matrixRotateX_asm:
    stmfd sp!, {lr}

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

    ldmfd sp!, {lr}
    bx lr

.global matrixRotateY_asm
matrixRotateY_asm:
    stmfd sp!, {lr}

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

    ldmfd sp!, {lr}
    bx lr

.global matrixRotateZ_asm
matrixRotateZ_asm:
    stmfd sp!, {lr}

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

    ldmfd sp!, {lr}
    bx lr

angleX  .req r0
angleY  .req r1
angleZ  .req r2
e00     .req r3
e01     .req r4
e02     .req r5
e10     .req r6
e11     .req r7
e12     .req r8
e20     .req r9
e21     .req r10
e22     .req r11
tmp     .req r12
sinX    .req lr
sinY    .req sinX
sinZ    .req sinX
cosX    .req angleX
cosY    .req angleY
cosZ    .req angleZ
mask    .req tmp
mm      .req tmp

.global matrixRotateYXZ_asm
matrixRotateYXZ_asm:
    mov mask, #0xFF
    orr mask, mask, #0xF00  ; mask = 0xFFF

    and angleX, mask, angleX, lsr #4
    and angleY, mask, angleY, lsr #4
    and angleZ, mask, angleZ, lsr #4

    orr mask, angleX, angleY
    orrs mask, mask, angleZ
    bxeq lr

    stmfd sp!, {r4-r11, lr}

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

    sincos angleY, sinY, cosY

    rotxy e00, e02, sinY, cosY, tmp
    rotxy e10, e12, sinY, cosY, tmp
    rotxy e20, e22, sinY, cosY, tmp

.rotX:
    cmp angleX, #0
    beq .rotZ

    sincos angleX, sinX, cosX

    rotxy e02, e01, sinX, cosX, tmp
    rotxy e12, e11, sinX, cosX, tmp
    rotxy e22, e21, sinX, cosX, tmp

.rotZ:
    cmp angleZ, #0
    beq .done

    sincos angleZ, sinZ, cosZ

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

    ldmfd sp!, {r4-r11, lr}
    bx lr

q   .req r0
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
