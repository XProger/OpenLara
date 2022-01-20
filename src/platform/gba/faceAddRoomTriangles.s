#include "common_asm.inc"

polys       .req r0
count       .req r1
vp          .req r2
vg0         .req r3
vg1         .req r4
vg2         .req r5
vg3         .req r6
flags       .req r7
vp0         .req r8
vp1         .req r9
vp2         .req r10
vertices    .req r11
tmp         .req r12
face        .req lr

vx0         .req vg0
vy0         .req vg1
vx1         .req vg2
vy1         .req vg3
vx2         .req vg2
vy2         .req vg3

vz0         .req vg0
vz1         .req vg1
vz2         .req vg2
depth       .req vg0

ot          .req vg1
next        .req vg2

.global faceAddRoomTriangles_asm
faceAddRoomTriangles_asm:
    stmfd sp!, {r4-r11, lr}

    ldr vp, =gVerticesBase
    ldr vp, [vp]

    ldr face, =gFacesBase
    ldr face, [face]

    ldr vertices, =gVertices

.loop:
    ldrh flags, [polys], #2
    ldrh vp0, [polys], #2
    ldrh vp1, [polys], #2
    ldrh vp2, [polys], #2

    add vp0, vp, vp0, lsl #3
    add vp1, vp, vp1, lsl #3
    add vp2, vp, vp2, lsl #3

    // fetch ((clip << 8) | g)
    ldrh vg0, [vp0, #VERTEX_G]
    ldrh vg1, [vp1, #VERTEX_G]
    ldrh vg2, [vp2, #VERTEX_G]

    // check clipping
    and tmp, vg0, vg1
    and tmp, tmp, vg2
    tst tmp, #CLIP_MASK
    bne .skip

    // mark if should be clipped by viewport
    orr tmp, vg0, vg1
    orr tmp, tmp, vg2
    tst tmp, #CLIP_MASK_VP
    orrne flags, flags, #FACE_CLIPPED

    // shift and compare VERTEX_G for flat rasterization
    mov vg0, vg0, lsl #24
    cmp vg0, vg1, lsl #24
    cmpeq vg0, vg2, lsl #24
    addeq flags, flags, #FACE_FLAT_ADD

    CCW .skip

    // vz0 = MAX_Z3 (depth)
    ldrh vz0, [vp0, #VERTEX_Z]
    ldrh vz1, [vp1, #VERTEX_Z]
    ldrh vz2, [vp2, #VERTEX_Z]
    cmp vz0, vz1
    movlt vz0, vz1
    cmp vz0, vz2
    movlt vz0, vz2
    mov depth, vz0, lsr #OT_SHIFT

    // faceAdd
    sub vp0, vp0, vertices
    sub vp1, vp1, vertices
    sub vp2, vp2, vertices

    mov vp0, vp0, lsr #3
    orr vp1, vp0, vp1, lsl #(16 - 3)
    mov vp2, vp2, lsr #3

    ldr ot, =gOT
    ldr next, [ot, depth, lsl #2]
    str face, [ot, depth, lsl #2]
    stmia face!, {next, flags, vp1, vp2}
.skip:
    subs count, count, #1
    bne .loop

    ldr tmp, =gFacesBase
    str face, [tmp]

    ldmfd sp!, {r4-r11, pc}
