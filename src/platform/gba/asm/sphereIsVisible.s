#include "common_asm.inc"

x       .req r0
y       .req r1
z       .req r2
r       .req r3
mx      .req r4
my      .req r5
mz      .req r6
vx      .req r7
vy      .req r8
vz      .req r12
m       .req lr
tmp     .req m
vp      .req m
vMinXY  .req z
vMaxXY  .req r

rMinX   .req vx
rMaxX   .req x
rMinY   .req vy
rMaxY   .req y

.global sphereIsVisible_asm
sphereIsVisible_asm:
    stmfd sp!, {r4-r8, lr}

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

    cmp vz, #VIEW_MAX_F
    bhi .fail

    mov x, vx, asr #FIXED_SHIFT
    mov y, vy, asr #FIXED_SHIFT

    mov z, vz, lsr #(FIXED_SHIFT + 6)
    add z, z, vz, lsr #(FIXED_SHIFT + 4)
    divLUT tmp, z
    mul x, tmp, x
    mul y, tmp, y
    mul r, tmp, r

    mov x, x, asr #(16 - PROJ_SHIFT)
    mov y, y, lsl #(PROJ_SHIFT)

    sub rMinX, x, r, lsr #(16 - PROJ_SHIFT)
    add rMaxX, x, r, lsr #(16 - PROJ_SHIFT)
    sub rMinY, y, r, lsl #PROJ_SHIFT
    add rMaxY, y, r, lsl #PROJ_SHIFT

    ldr vp, =viewportRel
    ldmia vp, {vMinXY, vMaxXY}

    cmp rMaxX, vMinXY, asr #16
    blt .fail
    cmp rMaxY, vMinXY, lsl #16
    blt .fail
    cmp rMinX, vMaxXY, asr #16
    bgt .fail
    cmp rMinY, vMaxXY, lsl #16
    bgt .fail

    mov r0, #1
    ldmfd sp!, {r4-r8, lr}
    bx lr

.fail:
    mov r0, #0
    ldmfd sp!, {r4-r8, lr}
    bx lr
