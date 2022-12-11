#include "common_asm.inc"

polys       .req r0     // arg
count       .req r1     // arg
mask        .req r2
flags       .req r3
vp0         .req r4
vp1         .req r5
vp2         .req r6
// FIQ regs
vg0         .req r8
vg1         .req r9
vg2         .req r10
vg3         .req r11
vertices    .req r12
ot          .req r13
face        .req r14

vx0         .req vg0
vy0         .req vg1
vx1         .req vg2
vy1         .req vg3
vx2         .req vg2
vy2         .req vg2

depth       .req vg0

i0          .req vg1
i1          .req vg2
i2          .req vg3    // vg to save vp2 value between iterations

vp01        .req vp1

tmp         .req flags
next        .req vp0

.global faceAddMeshTriangles_asm
faceAddMeshTriangles_asm:
    stmfd sp!, {r4-r6}
    fiq_on

    ldr vp2, =gVerticesBase
    ldr vp2, [vp2]

    ldr vertices, =gVertices
    lsr vertices, #3

    ldr face, =gFacesBase
    ldr face, [face]

    ldr ot, =gOT
    mov mask, #(0xFF << 24)
    orr mask, #(3 << 8)     // div 4 mul 4 for depth

.loop:
    // sizeof(MeshTriangle) == 8
    ldr tmp, [polys], #8    // skip flags

    // unpack index deltas
    and vg0, mask, tmp, lsl #24
    and vg1, mask, tmp, lsl #16

    // sizeof(Vertex) = (1 << 3)
    add vp0, vp2, vg0, asr #(24 - 3)
    add vp1, vp0, vg1, asr #(24 - 3)
    add vp2, vp1, tmp, asr #(24 - 3)    // 3rd vertex in 4th byte after zero byte to save one masking op

    CCW .skip

    // fetch [c, g, zz]
    ldr vg0, [vp0, #VERTEX_Z]
    ldr vg1, [vp1, #VERTEX_Z]
    ldr vg2, [vp2, #VERTEX_Z]

    // check clipping
    and tmp, vg0, vg1
    and tmp, vg2
    tst tmp, #(CLIP_DISCARD << 16)
    bne .skip

    // mark if should be clipped by viewport
    orr tmp, vg0, vg1
    orr tmp, vg2
    tst tmp, #(CLIP_FRAME << 16)
    ldrh flags, [polys, #-4]
    orrne flags, #FACE_CLIPPED

    // depth = AVG_Z3
    add depth, vg0, vg1             // depth = vz0 + vz1
    add depth, vg2, lsl #1          // depth += vz2 * 2
    bic depth, depth, mask, asr #8  // clear high half (g & clip flags) and low 2 bits

    // faceAdd
    rsb i0, vertices, vp0, lsr #3
    rsb i1, vertices, vp1, lsr #3
    rsb i2, vertices, vp2, lsr #3

    orr vp01, i0, i1, lsl #16

    orr flags, #FACE_TRIANGLE

    ldr next, [ot, depth]
    str face, [ot, depth]
    stmia face!, {flags, next, vp01, i2}
.skip:
    subs count, #1
    bne .loop

    ldr tmp, =gFacesBase
    str face, [tmp]

    fiq_off
    ldmfd sp!, {r4-r6}
    bx lr
