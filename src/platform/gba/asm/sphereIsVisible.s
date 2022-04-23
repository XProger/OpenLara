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
vMinXY  .req z
vMaxXY  .req r

rMinX   .req vx
rMaxX   .req x
rMinY   .req vy
rMaxY   .req y

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
    fiq_off
    bx lr

.fail:
    mov r0, #0
    fiq_off
    bx lr
