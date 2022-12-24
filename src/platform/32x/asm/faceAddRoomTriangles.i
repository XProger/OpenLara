#define tmp         r0
#define face        r1
#define vp          r2
#define flags       r3
#define polys       r4     // arg
#define count       r5     // arg
#define vp0         r6
#define vp1         r7
#define vp2         r8
#define ot          r9
#define vg0         r10
#define vg1         r11
#define vg2         r12
#define vg3         r13
#define vertices    r14

#define vx0         vg0
#define vy0         vg1
#define vx1         vg2
#define vy1         vg3
#define vx2         tmp
#define vy2         tmp

#define vz0         vg0
#define vz1         vg1
#define vz2         vg2

#define depth       tmp
#define next        vg1

.align 4
.global _faceAddRoomTriangles_asm
_faceAddRoomTriangles_asm:
        // push
        mov.l   r8, @-sp
        mov.l   r9, @-sp
        mov.l   r10, @-sp
        mov.l   r11, @-sp
        mov.l   r12, @-sp
        mov.l   r13, @-sp
        mov.l   r14, @-sp

        mov.l   var_gVertices_far, vertices
        add     #VERTEX_Z, vertices

        mov.l   var_gVerticesBase_far, vp
        mov.l   @vp, vp

        mov.l   var_gFacesBase_far, face
        mov.l   @face, face

        mov.l   var_gOT_far, ot

.loop_fart:
        // read flags and indices
        mov.l   @polys+, flags
        mov.l   @polys+, vp1

        extu.w  flags, vp0
        shlr16  flags

        extu.w  vp1, vp2
        shlr16  vp1

        // vp[0..2] alreay multiplied by VERTEX_SIZEOF

        // get vertex address
        add     vp, vp0
        add     vp, vp1
        add     vp, vp2

        // fetch ((g << 8) | clip)
        mov     #VERTEX_G, tmp
        mov.w   @(tmp, vp0), vg0
        mov.w   @(tmp, vp1), vg1
        mov.w   @(tmp, vp2), vg2
        // g on high-byte is 5 bits long, no need for extu.w

        // check_clipping
        mov     vg0, tmp
        and     vg1, tmp
        and     vg2, tmp 
        tst     #CLIP_DISCARD, tmp
        bf/s    .skip_fart

        // mark if should be clipped by frame
        mov     vg0, tmp        // [delay slot]
        or      vg1, tmp
        or      vg2, tmp
        tst     #CLIP_FRAME, tmp
        bt/s    1f
        mov.l   const_FACE_CLIPPED_far, tmp     // [delay slot] mov #1, tmp; rotr x2
        or      tmp, flags

1:      // compare VERTEX_G for gouraud rasterization
        xor     vg0, vg1
        xor     vg0, vg2
        or      vg2, vg1
        shlr8   vg1             // shift down for g only
        tst     vg1, vg1
        bt/s    2f
        mov.l   const_FACE_GOURAUD_far, tmp     // [delay slot] mov #128, tmp; shll8
        add     tmp, flags

2:      // check_backface
        ccw     vp0, vp1, vp2, vx0, vy0, vx1, vy1, vx2, vy2
        bt/s    .skip_fart
        mov.l   const_FACE_TRIANGLE_far, tmp    // [delay slot] mov #1, tmp; rotr
        or      tmp, flags

        // max_z3
        mov.w   @vp0, depth     // depth = vz0
        mov.w   @vp1, vz1
        // check_z1
        cmp/gt  depth, vz1
        bf/s    3f
        mov.w   @vp2, vz2       // [delay slot]
        mov     vz1, depth      // if (z1 > depth) depth = z1
3:      // check_z2
        cmp/gt  depth, vz2
        bf/s    .face_add_fart  // TODO use delay slot but not for OT! )
        sub     vertices, vp0   // [delay slot] get the first offset
        mov     vz2, depth      // if (z2 > depth) depth = z2

.face_add_fart:
        // offset = (p - vertices)
        sub     vertices, vp1
        sub     vertices, vp2

        shll2   depth
        mov.l   @(depth, ot), next
        mov.l   face, @(depth, ot)

        shll16  vp2
        shll16  vp1
        xtrct   vp0, vp1

        mov.l   flags, @(0, face)
        mov.l   next, @(4, face)
        mov.l   vp1, @(8, face)
        mov.l   vp2, @(12, face)
        add     #FACE_SIZEOF, face
.skip_fart:
        dt      count
        bf      .loop_fart

        mov.l   var_gFacesBase_far, tmp
        mov.l   face, @tmp

        // pop
        mov.l   @sp+, r14
        mov.l   @sp+, r13
        mov.l   @sp+, r12
        mov.l   @sp+, r11
        mov.l   @sp+, r10
        mov.l   @sp+, r9
        rts
        mov.l   @sp+, r8

#undef tmp
#undef face
#undef vp
#undef flags
#undef polys
#undef count
#undef vp0
#undef vp1
#undef vp2
#undef ot
#undef vg0
#undef vg1
#undef vg2
#undef vg3
#undef vertices
#undef vx0
#undef vy0
#undef vx1
#undef vy1
#undef vx2
#undef vy2
#undef vz0
#undef vz1
#undef vz2
#undef depth
#undef next