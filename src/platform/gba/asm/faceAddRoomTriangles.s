#include "common_asm.inc"

polys       .req r0     // arg
count       .req r1     // arg
vp          .req r2
vg0         .req r3
vg1         .req r4
vg2         .req r5
vg3         .req r6
// FIQ regs
flags       .req r8
vp0         .req r9
vp1         .req r10
vp2         .req r11
vp3         .req vg3
vertices    .req r12
ot          .req r13
face        .req r14

vx0         .req vg0
vy0         .req vg1
vx1         .req vg2
vy1         .req vg3
vx2         .req vg2
vy2         .req vg2

vz0         .req vg0
vz1         .req vg1
vz2         .req vg2
depth       .req vg0

tmp         .req flags
next        .req vp0

.global faceAddRoomTriangles_asm
faceAddRoomTriangles_asm:
    stmfd sp!, {r4-r6}
    fiq_on

    ldr vp, =gVerticesBase
    ldr vp, [vp]

    ldr vertices, =gVertices
    lsr vertices, #3

    ldr face, =gFacesBase
    ldr face, [face]

    ldr ot, =gOT

.loop:
    // sizeof(RoomTriangle) = 8
    ldr vp1, [polys], #4
    ldr vp3, [polys], #4    // + flags

    // prepare to unpack indices
    lsl vp0, vp1, #16
    lsl vp2, vp3, #16

    // assume that vertex index will never exceed 8191
    add vp0, vp, vp0, lsr #(16 - 3)
    add vp1, vp, vp1, lsr #(16 - 3)
    add vp2, vp, vp2, lsr #(16 - 3)

    // fetch ((clip << 8) | g)
    ldrh vg0, [vp0, #VERTEX_G]
    ldrh vg1, [vp1, #VERTEX_G]
    ldrh vg2, [vp2, #VERTEX_G]

    // check clipping
    and tmp, vg0, vg1
    and tmp, vg2
    tst tmp, #CLIP_DISCARD
    bne .skip

    // mark if should be clipped by viewport
    orr tmp, vg0, vg1
    orr tmp, vg2
    tst tmp, #CLIP_FRAME
    mov flags, vp3, lsr #16
    orrne flags, #FACE_CLIPPED

    // shift and compare VERTEX_G for gouraud rasterization
    lsl vg0, #24
    cmp vg0, vg1, lsl #24
    cmpeq vg0, vg2, lsl #24
    addne flags, #FACE_GOURAUD

    CCW .skip

    // depth (vz0) = MAX_Z3
    ldrh vz0, [vp0, #VERTEX_Z]
    ldrh vz1, [vp1, #VERTEX_Z]
    ldrh vz2, [vp2, #VERTEX_Z]
    cmp vz0, vz1
    movlt vz0, vz1
    cmp vz0, vz2
    movlt vz0, vz2

    // faceAdd
    rsb vp0, vertices, vp0, lsr #3
    rsb vp1, vertices, vp1, lsr #3
    rsb vp2, vertices, vp2, lsr #3

    orr vp1, vp0, vp1, lsl #16

    orr flags, #FACE_TRIANGLE

    ldr next, [ot, depth, lsl #2]
    str face, [ot, depth, lsl #2]
    stmia face!, {flags, next, vp1, vp2}
.skip:
    subs count, #1
    bne .loop

    ldr tmp, =gFacesBase
    str face, [tmp]

    fiq_off
    ldmfd sp!, {r4-r6}
    bx lr
