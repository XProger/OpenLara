#include "common_asm.inc"

vertices    .req r0
count       .req r1
m           .req r2
v           .req r3
vx          .req r4
vy          .req r5
vz          .req r6
vg          .req v
mx          .req r7
my          .req r8
mz          .req r9
x           .req r10
y           .req r11
z           .req r12
res         .req lr
t           .req y

spMinXY     .req mx
spMaxXY     .req my
spFrame     .req mz
spCaustLUT  .req x
spRandLUT   .req y

mask        .req x
vp          .req vx
minXY       .req vx
maxXY       .req vy

dz          .req mz
fog         .req mz

frame       .req vx
caust       .req vy
rand        .req vz
tmp         .req mx

SP_MINXY    = 0
SP_MAXXY    = 4
SP_FRAME    = 8
SP_CAUST    = 12
SP_RAND     = 16
SP_SIZE     = 20

.global transformRoomUW_asm
transformRoomUW_asm:
    stmfd sp!, {r4-r11, lr}

    ldr res, =gVerticesBase
    ldr res, [res]
    add res, #VERTEX_G

    ldr m, =gMatrixPtr
    ldr m, [m]

    ldr vp, =viewportRel
    ldmia vp, {spMinXY, spMaxXY}

    ldr spFrame, =gCausticsFrame
    ldr spFrame, [spFrame]

    ldr spCaustLUT, =gCaustics
    ldr spRandLUT, =gRandTable

    stmfd sp!, {spMinXY, spMaxXY, spFrame, spCaustLUT, spRandLUT}

    // preload mask, matrix and z-row
    mov mask, #(0xFF << 10)
    add m, #(12 * 4)
    ldmdb m!, {mx, my, mz, z}

.loop:
    // unpack vertex
    ldmia vertices!, {v}

    and vz, mask, v, lsr #6
    and vy, v, #0xFF00
    and vx, mask, v, lsl #10

    // transform z
    mla t, mx, vx, z
    mla t, my, vy, t
    mla t, mz, vz, t
    asr t, #FIXED_SHIFT

    // skip if vertex is out of z-range
    add t, #VIEW_OFF
    cmp t, #(VIEW_OFF + VIEW_OFF + VIEW_MAX)
    movhi vg, #(CLIP_NEAR + CLIP_FAR)
    bhi .skip

    and vg, mask, v, lsr #14
    sub z, t, #VIEW_OFF

    // transform y
    ldmdb m!, {mx, my, mz, y}
    mla y, mx, vx, y
    mla y, my, vy, y
    mla y, mz, vz, y
    asr y, #FIXED_SHIFT

    // transform x
    ldmdb m!, {mx, my, mz, x}
    mla x, mx, vx, x
    mla x, my, vy, x
    mla x, mz, vz, x
    asr x, #FIXED_SHIFT

    // caustics
    add tmp, sp, #SP_FRAME
    ldmia tmp, {frame, caust, rand}
    and tmp, count, #(MAX_RAND_TABLE - 1)
    ldr rand, [rand, tmp, lsl #2]
    add rand, frame
    and rand, #(MAX_CAUSTICS - 1)
    ldr caust, [caust, rand, lsl #2]
    add vg, caust, lsl #5

    // fog
    cmp z, #FOG_MIN
    subgt fog, z, #FOG_MIN
    addgt vg, fog, lsl #6
    lsr vg, #13
    cmp vg, #31
    movgt vg, #31

    // z clipping
    cmp z, #VIEW_MIN
    movle z, #VIEW_MIN
    orrle vg, #CLIP_NEAR
    cmp z, #VIEW_MAX
    movge z, #VIEW_MAX
    orrge vg, #CLIP_FAR

    // project
    mov dz, z, lsr #6
    add dz, z, lsr #4
    divLUT tmp, dz
    mul x, tmp, x
    mul y, tmp, y
    asr x, #(16 - PROJ_SHIFT)
    asr y, #(16 - PROJ_SHIFT)

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
    strh x, [res, #-6]
    strh y, [res, #-4]
    strh z, [res, #-2]

    // preload mask, matrix and z-row
    mov mask, #(0xFF << 10)
    add m, #(12 * 4)
    ldmdb m!, {mx, my, mz, z}

.skip:
    strh vg, [res], #8

    subs count, #1
    bne .loop

    add sp, #SP_SIZE
    ldmfd sp!, {r4-r11, pc}
