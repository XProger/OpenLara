#include "common_asm.inc"

mx      .req r0
my      .req r1
mz      .req r2
m       .req r3
vx      .req r4
vy      .req r5
vz      .req r6
x       .req r7
y       .req r8
z       .req r9
rMinX   .req r10
rMinY   .req r11
rMaxX   .req r12
rMaxY   .req lr

boxArg  .req mx
tmp     .req mz

bz      .req mz
offset  .req m
dz      .req offset
xx      .req rMinX
yy      .req rMinY
zz      .req rMaxX
min     .req rMaxY
max     .req rMaxY
vMinXY  .req x
vMaxXY  .req y
vp      .req x

minX    .req x
minY    .req y
minZ    .req z
maxX    .req mx
maxY    .req my
maxZ    .req mz

MAX_X   = (0 * 3 * 4)
MIN_X   = (1 * 3 * 4)
MAX_Y   = (2 * 3 * 4)
MIN_Y   = (3 * 3 * 4)
MAX_Z   = (4 * 3 * 4)
MIN_Z   = (5 * 3 * 4)
SIZE    = (6 * 3 * 4)

.macro project dx, dy, dz
    add offset, sp, \dz
    ldmia offset, {x, y, z}

    add offset, sp, \dy
    ldmia offset, {vx, vy, vz}
    add x, x, vx
    add y, y, vy
    add z, z, vz

    add offset, sp, \dx
    ldmia offset, {vx, vy, vz}
    add z, z, vz

    // check z clipping
    sub offset, z, #VIEW_MIN_F
    cmp offset, #(VIEW_MAX_F - VIEW_MIN_F)
    bhi 1f

    add x, x, vx
    add y, y, vy

    mov dz, z, lsr #(FIXED_SHIFT + 6)
    add dz, dz, z, lsr #(FIXED_SHIFT + 4)
    divLUT tmp, dz
    mul x, tmp, x
    mul y, tmp, y

    cmp x, rMinX
    movlt rMinX, x
    cmp y, rMinY
    movlt rMinY, y
    cmp x, rMaxX
    movgt rMaxX, x
    cmp y, rMaxY
    movgt rMaxY, y
1:
.endm

.global boxIsVisible_asm
boxIsVisible_asm:
    ldr m, =gMatrixPtr
    ldr m, [m]
    ldr bz, [m, #(11 * 4)]
    add bz, bz, #VIEW_OFF_F
    cmp bz, #(VIEW_OFF_F + VIEW_MAX_F)
    movhi r0, #0
    bxhi lr

    stmfd sp!, {r4-r11, lr}

    ldmia boxArg, {xx, yy, zz}

    // pre-transform min/max Z
    ldr mx, [m, #8]
    ldr vx, [m, #12]
    ldr my, [m, #24]
    ldr vy, [m, #28]
    ldr mz, [m, #40]
    ldr vz, [m, #44]

    mov min, zz, asr #16
    mla minX, min, mx, vx
    mla minY, min, my, vy
    mla minZ, min, mz, vz
    mov minX, minX, asr #FIXED_SHIFT
    mov minY, minY, asr #FIXED_SHIFT

    mov max, zz, lsl #16
    mov max, max, asr #16
    mla maxX, max, mx, vx
    mla maxY, max, my, vy
    mla maxZ, max, mz, vz
    mov maxX, maxX, asr #FIXED_SHIFT
    mov maxY, maxY, asr #FIXED_SHIFT
    stmdb sp!, {maxX, maxY, maxZ, minX, minY, minZ}

    // pre-transform min/max Y
    ldr mx, [m, #4]
    ldr my, [m, #20]
    ldr mz, [m, #36]

    mov min, yy, asr #16
    mul minX, mx, min
    mul minY, my, min
    mul minZ, mz, min
    mov minX, minX, asr #FIXED_SHIFT
    mov minY, minY, asr #FIXED_SHIFT

    mov max, yy, lsl #16
    mov max, max, asr #16
    mul maxX, max, mx
    mul maxY, max, my
    mul maxZ, max, mz
    mov maxX, maxX, asr #FIXED_SHIFT
    mov maxY, maxY, asr #FIXED_SHIFT
    stmdb sp!, {maxX, maxY, maxZ, minX, minY, minZ}

    // pre-transform min/max X
    ldr mx, [m, #0]
    ldr my, [m, #16]
    ldr mz, [m, #32]

    mov min, xx, asr #16
    mul minX, mx, min
    mul minY, my, min
    mul minZ, mz, min
    mov minX, minX, asr #FIXED_SHIFT
    mov minY, minY, asr #FIXED_SHIFT

    mov max, xx, lsl #16
    mov max, max, asr #16
    mul maxX, max, mx
    mul maxY, max, my
    mul maxZ, max, mz
    mov maxX, maxX, asr #FIXED_SHIFT
    mov maxY, maxY, asr #FIXED_SHIFT
    stmdb sp!, {maxX, maxY, maxZ, minX, minY, minZ}

    mov rMinX, #MAX_INT32
    mov rMinY, #MAX_INT32
    mov rMaxX, #MIN_INT32
    mov rMaxY, #MIN_INT32

    project #MIN_X, #MIN_Y, #MIN_Z
    project #MAX_X, #MIN_Y, #MIN_Z
    project #MIN_X, #MAX_Y, #MIN_Z
    project #MAX_X, #MAX_Y, #MIN_Z
    project #MIN_X, #MIN_Y, #MAX_Z
    project #MAX_X, #MIN_Y, #MAX_Z
    project #MIN_X, #MAX_Y, #MAX_Z
    project #MAX_X, #MAX_Y, #MAX_Z
    mov r0, #0

    mov rMinX, rMinX, asr #(16 - PROJ_SHIFT)
    mov rMaxX, rMaxX, asr #(16 - PROJ_SHIFT)

    cmp rMinX, rMaxX
    beq .done

    // rect Y must remain shifted up by 16
    mov rMinY, rMinY, lsl #PROJ_SHIFT
    mov rMaxY, rMaxY, lsl #PROJ_SHIFT

    // check xy clipping
    ldr vp, =viewportRel
    ldmia vp, {vMinXY, vMaxXY}

    cmp rMaxX, vMinXY, asr #16
    blt .done
    cmp rMaxY, vMinXY, lsl #16
    blt .done
    cmp rMinX, vMaxXY, asr #16
    bgt .done
    cmp rMinY, vMaxXY, lsl #16
    bgt .done

    mov r0, #1
.done:
    add sp, sp, #SIZE
    ldmfd sp!, {r4-r11, lr}
    bx lr