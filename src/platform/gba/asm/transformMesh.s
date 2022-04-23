#include "common_asm.inc"

vertices    .req r0     // arg
count       .req r1     // arg
intensity   .req r2     // arg
vx          .req intensity
vy          .req r3
vz          .req r4
x           .req r5
y           .req r6
z           .req vx
mx0         .req r7

mx2         .req r8
my2         .req r9
mz2         .req r10
mw2         .req r11
res         .req r12
vg          .req lr

// FIQ regs
my0         .req r8
mz0         .req r9
mw0         .req r10
mx1         .req r11
my1         .req r12
mz1         .req r13
mw1         .req r14

ambient     .req vz
tmp         .req vy
dz          .req vz
m           .req vz

.global transformMesh_asm
transformMesh_asm:
    stmfd sp!, {r4-r11, lr}

    ldr res, =gVerticesBase
    ldr res, [res]

    ldr ambient, =gLightAmbient
    ldr ambient, [ambient]
    add vg, ambient, intensity
    asr vg, #8
    // clamp spAmbient to 0..31
    cmp vg, #31
    movge vg, #31
    bic vg, vg, asr #31

    ldr m, =gMatrixPtr
    ldr m, [m]
    fiq_on
    ldmia m!, {mx0, my0, mz0, mw0,  mx1, my1, mz1, mw1}
    ldmia m, {mx2, my2, mz2, mw2}^

.loop:
    // unpack vertex
    ldrsh vx, [vertices], #2
    ldrsh vy, [vertices], #2
    ldrsh vz, [vertices], #2

    // transform x
    mla x, mx0, vx, mw0
    mla x, my0, vy, x
    mla x, mz0, vz, x
    asr x, #FIXED_SHIFT

    // transform y
    mla y, mx1, vx, mw1
    mla y, my1, vy, y
    mla y, mz1, vz, y
    asr y, #FIXED_SHIFT
    fiq_off

    // transform z
    mla z, mx2, vx, mw2
    mla z, my2, vy, z
    mla z, mz2, vz, z
    asr z, #FIXED_SHIFT

    bic vg, #CLIP_MASK  // clear clipping flags

    // z clipping
    cmp z, #VIEW_MIN
    movle z, #VIEW_MIN
    orrle vg, #CLIP_NEAR
    cmp z, #VIEW_MAX
    movge z, #VIEW_MAX
    orrge vg, #CLIP_FAR

    // project
    mov dz, z, lsr #4
    add dz, z, lsr #6
    divLUT tmp, dz
    mul x, tmp, x
    mul y, tmp, y
    asr x, #(16 - PROJ_SHIFT)
    asr y, #(16 - PROJ_SHIFT)

    add x, #(FRAME_WIDTH >> 1)
    add y, #(FRAME_HEIGHT >> 1)

    // frame rect clipping
    cmp x, #FRAME_WIDTH
    cmpls y, #FRAME_HEIGHT
    orrhi vg, #CLIP_FRAME

    // store the result
    strh x, [res], #2
    strh y, [res], #2
    strh z, [res], #2
    strh vg, [res], #2

    subs count, #1
    fiq_on_ne
    bne .loop

    ldmfd sp!, {r4-r11, pc}
