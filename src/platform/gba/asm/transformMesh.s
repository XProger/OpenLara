#include "common_asm.inc"

vertices    .req r0
count       .req r1
intensity   .req r2
m           .req r3
vg          .req intensity
vx          .req r4
vy          .req r5
vz          .req r6
mx          .req r7
my          .req r8
mz          .req r9
x           .req r10
y           .req r11
z           .req r12
res         .req lr

ambient     .req vx
vp          .req vx
minXY       .req vx
maxXY       .req vy
tmp         .req vy
dz          .req vx

SP_MINXY    = 0
SP_MAXXY    = 4
SP_SIZE     = 8

.global transformMesh_asm
transformMesh_asm:
    stmfd sp!, {r4-r11, lr}

    ldr res, =gVerticesBase
    ldr res, [res]

    ldr ambient, =gLightAmbient
    ldr ambient, [ambient]
    add vg, ambient, intensity
    mov vg, vg, asr #8
    // clamp spAmbient to 0..31
    cmp vg, #31
    movge vg, #31
    bic vg, vg, vg, asr #31

    ldr vp, =viewportRel
    ldmia vp, {minXY, maxXY}

    stmfd sp!, {minXY, maxXY}

    ldr m, =gMatrixPtr
    ldr m, [m]

.loop:
    // unpack vertex
    ldrsh vx, [vertices], #2
    ldrsh vy, [vertices], #2
    ldrsh vz, [vertices], #2

    bic vg, vg, #CLIP_MASK  // clear clipping flags

    // transform x
    ldmia m!, {mx, my, mz, x}
    mla x, mx, vx, x
    mla x, my, vy, x
    mla x, mz, vz, x
    mov x, x, asr #FIXED_SHIFT

    // transform y
    ldmia m!, {mx, my, mz, y}
    mla y, mx, vx, y
    mla y, my, vy, y
    mla y, mz, vz, y
    mov y, y, asr #FIXED_SHIFT

    // transform z
    ldmia m!, {mx, my, mz, z}
    mla z, mx, vx, z
    mla z, my, vy, z
    mla z, mz, vz, z
    mov z, z, asr #FIXED_SHIFT

    sub m, #(12 * 4)    // restore matrix ptr

    // z clipping
    cmp z, #VIEW_MIN
    movle z, #VIEW_MIN
    orrle vg, vg, #CLIP_NEAR
    cmp z, #VIEW_MAX
    movge z, #VIEW_MAX
    orrge vg, vg, #CLIP_FAR

    // project
    mov dz, z, lsr #4
    add dz, dz, z, lsr #6
    divLUT tmp, dz
    mul x, tmp, x
    mul y, tmp, y
    mov x, x, asr #(16 - PROJ_SHIFT)
    mov y, y, asr #(16 - PROJ_SHIFT)

    // viewport clipping
    ldmia sp, {minXY, maxXY}
    
    cmp x, minXY, asr #16
    orrle vg, vg, #CLIP_LEFT
    cmp x, maxXY, asr #16
    orrge vg, vg, #CLIP_RIGHT

    mov minXY, minXY, lsl #16
    mov maxXY, maxXY, lsl #16

    cmp y, minXY, asr #16
    orrle vg, vg, #CLIP_TOP
    cmp y, maxXY, asr #16
    orrge vg, vg, #CLIP_BOTTOM

    add x, x, #(FRAME_WIDTH >> 1)
    add y, y, #(FRAME_HEIGHT >> 1)

    // store the result
    strh x, [res], #2
    strh y, [res], #2
    strh z, [res], #2
    strh vg, [res], #2

    subs count, #1
    bne .loop

    add sp, sp, #SP_SIZE
    ldmfd sp!, {r4-r11, pc}
