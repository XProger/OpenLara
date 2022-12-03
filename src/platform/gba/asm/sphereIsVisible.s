#include "common_asm.inc"

x       .req r0     // arg
y       .req r1     // arg
z       .req r2     // arg
r       .req r3     // arg
// FIQ regs
mx      .req r8
my      .req r9
mz      .req r10
vx      .req r11
vy      .req r12
vz      .req r13
m       .req r14
tmp     .req m
vp      .req m
dz      .req vz
minXY   .req z
maxXY   .req r

minX    .req vx
maxX    .req x
minY    .req vy
maxY    .req y

.global sphereIsVisible_asm
sphereIsVisible_asm:
    fiq_on

    ldr m, =gMatrixPtr
    ldr m, [m]

    ldmia m!, {mx, my, mz, vx}
    mla vx, mx, x, vx
    mla vx, my, y, vx
    mla vx, mz, z, vx
    ldmia m!, {mx, my, mz, vy}
    mla vy, mx, x, vy
    mla vy, my, y, vy
    mla vy, mz, z, vy
    ldmia m!, {mx, my, mz, vz}
    mla vz, mx, x, vz
    mla vz, my, y, vz
    mla vz, mz, z, vz

    cmp vz, #(VIEW_MAX << FIXED_SHIFT)
    bhi .fail

    mov x, vx, asr #FIXED_SHIFT
    mov y, vy, asr #FIXED_SHIFT
    mov z, vz, asr #(FIXED_SHIFT + OT_SHIFT)

    add dz, z, z, lsr #2
    divLUT tmp, dz
    mul x, tmp, x
    mul y, tmp, y
    mul r, tmp, r

    mov x, x, asr #(16 - PROJ_SHIFT)
    mov y, y, asr #(16 - PROJ_SHIFT)

    sub minX, x, r, lsr #(16 - PROJ_SHIFT)
    add maxX, x, r, lsr #(16 - PROJ_SHIFT)
    sub minY, y, r, lsr #(16 - PROJ_SHIFT)
    add maxY, y, r, lsr #(16 - PROJ_SHIFT)

    ldr vp, =viewportRel
    ldmia vp, {minXY, maxXY}

    cmp maxX, minXY, asr #16
    ble .fail
    cmp minX, maxXY, asr #16
    bge .fail

    lsl minXY, #16
    lsl maxXY, #16

    cmp maxY, minXY, asr #16
    ble .fail
    cmp minY, maxXY, asr #16
    bge .fail

    mov r0, #1
    fiq_off
    bx lr

.fail:
    mov r0, #0
    fiq_off
    bx lr
