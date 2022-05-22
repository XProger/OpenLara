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

#define depth       vg0     // == vz0
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

        mov.l   var_gVerticesBase_far, vp
        mov.l   @vp, vp

        mov.l   var_gFacesBase_far, face
        mov.l   @face, face

        mov.l   var_gOT_far, ot
        nop

.loop_fart:
        // read flags and indices
        mov.w   @polys+, flags
        mov.w   @polys+, vp0
        mov.w   @polys+, vp1
        mov.w   @polys+, vp2
        extu.w  flags, flags
        // indices never exceed 32k, no need for extu.w

        // p = gVerticesBase + index * VERTEX_SIZEOF (index is already multiplied by 2)
        shll2   vp0
        shll2   vp1
        shll2   vp2

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
        mov.l   const_FACE_CLIPPED_far, tmp     // [delay slot]
        or      tmp, flags

1:      // compare VERTEX_G for gouraud rasterization
        xor     vg0, vg1
        xor     vg0, vg2
        or      vg2, vg1
        shlr8   vg1             // shift down for g only
        tst     vg1, vg1
        bt/s    2f
        mov.l   const_FACE_GOURAUD_far, tmp     // [delay slot]
        add     tmp, flags

2:      // check_backface
        ccw     vp0, vp1, vp2, vx0, vy0, vx1, vy1, vx2, vy2
        bt/s    .skip_fart
        mov.l   const_FACE_TRIANGLE_far, tmp    // [delay slot]
        or      tmp, flags

        // max_z3
        mov.w   @vp0, vz0
        mov.w   @vp1, vz1
        // check_z1
        cmp/gt  vz0, vz1
        bf/s    3f
        mov.w   @vp2, vz2       // [delay slot]
        mov     vz1, vz0        // if (z1 > z0) z0 = z1
3:      // check_z2
        cmp/gt  vz0, vz2
        bf      .face_add_fart  // TODO use delay slot but not for OT! )
        mov     vz2, vz0        // if (z2 > z0) z0 = z2

.face_add_fart:
        // get absolute indices
        // p address is 4 bytes ahead but it's fine for shlr3
        // index = (p - vertices) / VERTEX_SIZEOF
        sub     vertices, vp0
        sub     vertices, vp1
        sub     vertices, vp2
        shlr2   vp0
        shlr2   vp1
        shlr2   vp2
        shlr    vp0
        shlr    vp1
        shlr    vp2

        // depth (vz0) >>= OT_SHIFT (4)
        shlr2   depth
        shlr2   depth

        shll2   depth
        add     ot, depth   // depth = gOT[depth]
        mov.l   @depth, next
        mov.l   face, @depth

        add     #FACE_SIZEOF, face
        mov     face, tmp
        add     #-2, tmp        // skip 4th index

        mov.w   vp2, @-tmp
        mov.w   vp1, @-tmp
        mov.w   vp0, @-tmp
        mov.l   next, @-tmp
        mov.l   flags, @-tmp
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