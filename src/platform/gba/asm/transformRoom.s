#include "common_asm.inc"

vertices    .req r0     // arg
count       .req r1     // arg
vx          .req r2
vy          .req r3
vz          .req r4
x           .req vx
y           .req r5
z           .req r6
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

m           .req vx
v           .req vg
mask        .req y

minXY       .req vy
maxXY       .req vz

tmp         .req vy
dx          .req vz
dy          .req vy
dz          .req vz
fog         .req vz

SP_MINXY    = 0
SP_MAXXY    = 4
SP_SIZE     = 8

.global transformRoom_asm
transformRoom_asm:
    stmfd sp!, {r4-r11, lr}

    ldr res, =gVerticesBase
    ldr res, [res]

    ldr tmp, =viewportRel
    ldmia tmp, {minXY, maxXY}
    stmfd sp!, {minXY, maxXY}

    ldr m, =gMatrixPtr
    ldr m, [m]
    fiq_on
    ldmia m!, {mx0, my0, mz0, mw0,  mx1, my1, mz1, mw1}
    asr mw0, #FIXED_SHIFT
    asr mw1, #FIXED_SHIFT
    fiq_off
    ldmia m, {mx2, my2, mz2, mw2}
    asr mw2, #(FIXED_SHIFT + OT_SHIFT)

.loop:
    // unpack vertex
    ldr v, [vertices], #4

    mov mask, #0xFF
    and vx, mask, v
    and vy, mask, v, lsr #8
    and vz, mask, v, lsr #16
    mov vg, v, lsr #(24 + 3)

    // transform z
    mul z, mx2, vx
    mla z, my2, vy, z
    mla z, mz2, vz, z
    add z, mw2, z, asr #(FIXED_SHIFT - 8 + OT_SHIFT)

    fiq_on
    // transform y
    mul y, mx1, vx
    mla y, my1, vy, y
    mla y, mz1, vz, y
    add y, mw1, y, asr #(FIXED_SHIFT - 8)

    // transform x
    mul x, mx0, vx
    mla x, my0, vy, x
    mla x, mz0, vz, x
    add x, mw0, x, asr #(FIXED_SHIFT - 8)
    fiq_off

    // fog
    subs fog, z, #(FOG_MIN >> OT_SHIFT)
    addgt vg, fog, lsr #(3 + FOG_SHIFT - OT_SHIFT)
    cmpgt vg, #31
    movgt vg, #31

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

    // portal rect clipping
    ldmia sp, {minXY, maxXY}

    cmp x, minXY, asr #16
    orrle vg, #CLIP_LEFT
    cmp x, maxXY, asr #16
    orrge vg, #CLIP_RIGHT

    lsl minXY, #16
    lsl maxXY, #16

    cmp y, minXY, asr #16
    orrle vg, #CLIP_TOP
    cmp y, maxXY, asr #16
    orrge vg, #CLIP_BOTTOM

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
    bne .loop

    add sp, #SP_SIZE
    ldmfd sp!, {r4-r11, pc}
