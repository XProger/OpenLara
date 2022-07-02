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
dy          .req tmp
dz          .req vz
fog         .req vz

frame       .req vy
caust       .req vy
rand        .req vz

spMinXY     .req vx
spMaxXY     .req vy
spRandLUT   .req vz
spFrame     .req y
spCaustLUT  .req z

SP_MINXY    = 0
SP_MAXXY    = 4
SP_RAND     = 8
SP_FRAME    = 12
SP_CAUST    = 16
SP_SIZE     = 20

.global transformRoomUW_asm
transformRoomUW_asm:
    stmfd sp!, {r4-r11, lr}

    ldr res, =gVerticesBase
    ldr res, [res]
    add res, #VERTEX_G

    ldr tmp, =viewportRel
    ldmia tmp, {spMinXY, spMaxXY}

    ldr spFrame, =gCausticsFrame
    ldr spFrame, [spFrame]

    ldr spCaustLUT, =gCaustics
    ldr spRandLUT, =gRandTable

    stmfd sp!, {spMinXY, spMaxXY, spRandLUT, spFrame, spCaustLUT}

    mov mask, #(0xFF << 10)

    ldr m, =gMatrixPtr
    ldr m, [m]
    fiq_on
    ldmia m!, {mx0, my0, mz0, mw0,  mx1, my1, mz1, mw1}
    asr mw0, #FIXED_SHIFT
    asr mw1, #FIXED_SHIFT
    fiq_off
    ldmia m, {mx2, my2, mz2, mw2}
    asr mw2, #FIXED_SHIFT

.loop:
    // unpack vertex
    ldmia vertices!, {v}

    and vz, mask, v, lsr #6
    and vy, v, #0xFF00
    and vx, mask, v, lsl #10

    // transform z
    mul z, mx2, vx
    mla z, my2, vy, z
    mla z, mz2, vz, z
    add z, mw2, z, asr #FIXED_SHIFT

    // skip if vertex is out of z-range
    add z, #VIEW_OFF
    cmp z, #(VIEW_OFF + VIEW_OFF + VIEW_MAX)
    movhi vg, #(CLIP_NEAR + CLIP_FAR)
    bhi .skip

    and vg, mask, v, lsr #14
    sub z, #VIEW_OFF

    fiq_on
    // transform y
    mul y, mx1, vx
    mla y, my1, vy, y
    mla y, mz1, vz, y
    add y, mw1, y, asr #FIXED_SHIFT

    // transform x
    mul x, mx0, vx
    mla x, my0, vy, x
    mla x, mz0, vz, x
    add x, mw0, x, asr #FIXED_SHIFT
    fiq_off

    // caustics
    ldr rand, [sp, #SP_RAND]
    and tmp, count, #(MAX_RAND_TABLE - 1)
    ldr rand, [rand, tmp, lsl #2]
    ldr frame, [sp, #SP_FRAME]
    add rand, frame
    and rand, #(MAX_CAUSTICS - 1)
    ldr caust, [sp, #SP_CAUST]
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
    mov dz, z, lsr #4
    add dz, z, lsr #6
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
    strh x, [res, #-6]
    strh y, [res, #-4]
    strh z, [res, #-2]

    mov mask, #(0xFF << 10)
.skip:
    strh vg, [res], #8

    subs count, #1
    bne .loop

    add sp, #SP_SIZE
    ldmfd sp!, {r4-r11, pc}
