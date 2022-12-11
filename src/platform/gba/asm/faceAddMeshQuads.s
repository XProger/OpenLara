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

depth       .req vg0

i0          .req vp0
i1          .req vp1
i2          .req vp2
i3          .req vg3    // vg to save vp3 value between iterations

vp01        .req vp1
vp23        .req vp2

tmp         .req flags
next        .req vp0

.global faceAddMeshQuads_asm
faceAddMeshQuads_asm:
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
    orr mask, #(3 << 8)     // div 4 mul 4 for depth

.loop:
    // sizeof(MeshQuad) == 8
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

    CCW .skip

    // fetch [c, g, zz]
    ldr vg0, [vp0, #VERTEX_Z]
    ldr vg1, [vp1, #VERTEX_Z]
    ldr vg2, [vp2, #VERTEX_Z]
    ldr vg3, [vp3, #VERTEX_Z]

    // check clipping
    and tmp, vg0, vg1
    and tmp, vg2
    and tmp, vg3
    tst tmp, #(CLIP_DISCARD << 16)
    bne .skip

    // mark if should be clipped by viewport
    orr tmp, vg0, vg1
    orr tmp, vg2
    orr tmp, vg3
    tst tmp, #(CLIP_FRAME << 16)
    ldrh flags, [polys, #-4]
    orrne flags, #FACE_CLIPPED

    // depth = AVG_Z4
    add depth, vg0, vg1             // depth = vz0 + vz1
    add depth, vg2                  // depth += vz2
    add depth, vg3                  // depth += vz3
    bic depth, depth, mask, asr #8  // clear high half (g & clip flags) and low 2 bits

    // faceAdd
    rsb i0, vertices, vp0, lsr #3
    rsb i1, vertices, vp1, lsr #3
    rsb i2, vertices, vp2, lsr #3
    rsb i3, vertices, vp3, lsr #3

    orr vp01, i0, i1, lsl #16
    orr vp23, i2, i3, lsl #16

    ldr next, [ot, depth]
    str face, [ot, depth]
    stmia face!, {flags, next, vp01, vp23}
.skip:
    subs count, #1
    bne .loop

    ldr tmp, =gFacesBase
    str face, [tmp]

    fiq_off
    ldmfd sp!, {r4-r7}
    bx lr
