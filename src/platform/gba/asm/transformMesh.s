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
dx          .req vz
dy          .req tmp
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
    asr mw0, #FIXED_SHIFT
    asr mw1, #FIXED_SHIFT
    fiq_off
    ldmia m, {mx2, my2, mz2, mw2}
    asr mw2, #(FIXED_SHIFT + OT_SHIFT)
    fiq_on

.loop:
    // unpack vertex
    ldrsh vx, [vertices], #2
    ldrsh vy, [vertices], #2
    ldrsh vz, [vertices], #2

    // transform x
    mul x, mx0, vx
    mla x, my0, vy, x
    mla x, mz0, vz, x
    add x, mw0, x, asr #(FIXED_SHIFT - MESH_SHIFT)

    // transform y
    mul y, mx1, vx
    mla y, my1, vy, y
    mla y, mz1, vz, y
    add y, mw1, y, asr #(FIXED_SHIFT - MESH_SHIFT)
    fiq_off

    // transform z
    mul z, mx2, vx
    mla z, my2, vy, z
    mla z, mz2, vz, z
    add z, mw2, z, asr #(FIXED_SHIFT - MESH_SHIFT + OT_SHIFT)

    bic vg, #CLIP_MASK  // clear clipping flags

    // z clipping
    cmp z, #(VIEW_MIN >> OT_SHIFT)
    movle z, #(VIEW_MIN >> OT_SHIFT)
    orrle vg, #CLIP_PLANE
    cmp z, #(VIEW_MAX >> OT_SHIFT)
    movge z, #(VIEW_MAX >> OT_SHIFT)
    orrge vg, #CLIP_PLANE

    // project
    add dz, z, z, lsr #2
    divLUT tmp, dz
    mul dx, x, tmp
    mul dy, y, tmp

    asr x, dx, #(16 - PROJ_SHIFT)
    asr y, dy, #(16 - PROJ_SHIFT)
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
