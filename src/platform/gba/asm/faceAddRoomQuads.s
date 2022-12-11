#include "common_asm.inc"

polys       .req r0     // arg
count       .req r1     // arg
mask        .req r2
vg0         .req r3
vg1         .req r4
vg2         .req r5
vg3         .req r6
flags       .req r7
// FIQ regs
vp0         .req r8
vp1         .req r9
vp2         .req r10
vp3         .req r11
ot          .req r12
face        .req r13
vertices    .req r14

vx0         .req vg0
vy0         .req vg1
vx1         .req vg2
vy1         .req vg3
vx2         .req vg2
vy2         .req vg2

vz0         .req vg0
vz1         .req vg1
vz2         .req vg2
vz3         .req vg3
depth       .req vg0

i0          .req vp0
i1          .req vp1
i2          .req vp2
i3          .req vg3    // vg to save vp3 value between iterations

vp01        .req vp1
vp23        .req vp2

tmp         .req flags
next        .req vp0

.global faceAddRoomQuads_asm
faceAddRoomQuads_asm:
    stmfd sp!, {r4-r7}
    fiq_on

    ldr vp3, =gVerticesBase
    ldr vp3, [vp3]

    ldr vertices, =gVertices
    lsr vertices, #3

    ldr face, =gFacesBase
    ldr face, [face]

    ldr ot, =gOT
    mov mask, #(0xFF << 24)

.loop:
    // sizeof(RoomQuad) = 8
    ldr tmp, [polys], #8    // skip flags

    // unpack index deltas
    and vg0, mask, tmp, lsl #24
    and vg1, mask, tmp, lsl #16
    and vg2, mask, tmp, lsl #8
    and vg3, mask, tmp

    // sizeof(Vertex) = (1 << 3)
    add vp0, vp3, vg0, asr #(24 - 3)
    add vp1, vp0, vg1, asr #(24 - 3)
    add vp2, vp1, vg2, asr #(24 - 3)
    add vp3, vp2, vg3, asr #(24 - 3)

    // fetch ((clip << 8) | g)
    ldrh vg0, [vp0, #VERTEX_G]
    ldrh vg1, [vp1, #VERTEX_G]
    ldrh vg2, [vp2, #VERTEX_G]
    ldrh vg3, [vp3, #VERTEX_G]

    // check clipping
    and tmp, vg0, vg1
    and tmp, vg2
    and tmp, vg3
    tst tmp, #CLIP_DISCARD
    bne .skip

    // mark if should be clipped by viewport
    orr tmp, vg0, vg1
    orr tmp, vg2
    orr tmp, vg3
    tst tmp, #CLIP_FRAME
    ldrh flags, [polys, #-4]
    orrne flags, #FACE_CLIPPED

    // shift and compare VERTEX_G for gouraud rasterization
    lsl vg0, #24
    cmp vg0, vg1, lsl #24
    cmpeq vg0, vg2, lsl #24
    cmpeq vg0, vg3, lsl #24
    addne flags, #FACE_GOURAUD

    CCW .skip

    // depth (vz0) = MAX_Z4
    ldrh vz0, [vp0, #VERTEX_Z]
    ldrh vz1, [vp1, #VERTEX_Z]
    ldrh vz2, [vp2, #VERTEX_Z]
    ldrh vz3, [vp3, #VERTEX_Z]
    cmp vz0, vz1
    movlt vz0, vz1
    cmp vz0, vz2
    movlt vz0, vz2
    cmp vz0, vz3
    movlt vz0, vz3

    // faceAdd
    rsb i0, vertices, vp0, lsr #3
    rsb i1, vertices, vp1, lsr #3
    rsb i2, vertices, vp2, lsr #3
    rsb i3, vertices, vp3, lsr #3

    orr vp01, i0, i1, lsl #16
    orr vp23, i2, i3, lsl #16

    ldr next, [ot, depth, lsl #2]
    str face, [ot, depth, lsl #2]
    stmia face!, {flags, next, vp01, vp23}
.skip:
    subs count, #1
    bne .loop

    ldr tmp, =gFacesBase
    str face, [tmp]

    fiq_off
    ldmfd sp!, {r4-r7}
    bx lr
