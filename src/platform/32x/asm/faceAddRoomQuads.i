#define tmp         r0
#define face        r1
#define vp          r2
#define flags       r3
#define polys       r4     // arg
#define count       r5     // arg
#define vp0         r6
#define vp1         r7
#define vp2         r8
#define vp3         r9
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
#define vz3         vg3

#define depth       tmp
#define next        vg1
#define ot          vg0

.align 4
.global _faceAddRoomQuads_asm
_faceAddRoomQuads_asm:
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
        nop

.loop_farq:
        // read flags and indices
        mov.l   @polys+, flags
        mov.l   @polys+, vp0

        exts.b  vp0, vp3
        shlr8   vp0
        exts.b  vp0, vp2
        shlr8   vp0
        exts.b  vp0, vp1
        shlr8   vp0
        exts.b  vp0, vp0

        // index *= 8 (VERTEX_SIZEOF)
        shll2   vp0
        shll2   vp1
        shll2   vp2
        shll2   vp3
        shll    vp0
        shll    vp1
        shll    vp2
        shll    vp3

        // get vertex address
        add     vp, vp0
        add     vp0, vp1
        add     vp1, vp2
        add     vp2, vp3
        mov     vp3, vp

        // fetch ((g << 8) | clip)
        mov     #VERTEX_G, tmp
        mov.w   @(tmp, vp0), vg0
        mov.w   @(tmp, vp1), vg1
        mov.w   @(tmp, vp2), vg2
        mov.w   @(tmp, vp3), vg3
        // g on high-byte is 5 bits long, no need for extu.w

        // check_clipping
        mov     vg0, tmp
        and     vg1, tmp
        and     vg2, tmp
        and     vg3, tmp
        tst     #CLIP_DISCARD, tmp
        bf/s    .skip_farq

        // mark if should be clipped by frame
        mov     vg0, tmp        // [delay slot]
        or      vg1, tmp
        or      vg2, tmp
        or      vg3, tmp
        tst     #CLIP_FRAME, tmp
        bt/s    1f
        mov.l   const_FACE_CLIPPED_far, tmp     // [delay slot]
        or      tmp, flags

1:      // compare VERTEX_G for gouraud rasterization
        xor     vg0, vg1
        xor     vg0, vg2
        xor     vg0, vg3
        or      vg2, vg1
        or      vg3, vg1
        shlr8   vg1             // shift down for g only
        tst     vg1, vg1
        bt/s    2f
        mov.l   const_FACE_GOURAUD_far, tmp     // [delay slot]
        add     tmp, flags

2:      // check_backface
        ccw     vp0, vp1, vp2, vx0, vy0, vx1, vy1, vx2, vy2
        bt/s    .skip_farq
        add     #VERTEX_Z, vp3  // [delay slot] ccw shifts p[0..2] address to VERTEX_Z, shift p3 too

        // max_z4
        mov.w   @vp0, depth
        mov.w   @vp1, vz1 
        // check_z1
        cmp/gt  depth, vz1
        bf/s    3f
        mov.w   @vp2, vz2       // [delay slot]
        mov     vz1, depth      // if (z1 > z0) z0 = z1
3:      // check_z2
        cmp/gt  depth, vz2
        bf/s    4f
        mov.w   @vp3, vz3       // [delay slot]
        mov     vz2, depth      // if (z2 > z0) z0 = z2
4:      // check_z3
        cmp/gt  depth, vz3
        bf/s    .face_add_farq
        sub     vertices, vp0   // [delay slot] get the first offset
        mov     vz3, depth      // if (z3 > z0) z0 = z3

.face_add_farq:
        mov.l   var_gOT_far, ot
        // offset = (p - vertices)
        sub     vertices, vp1
        sub     vertices, vp2
        sub     vertices, vp3

        shll2   depth
        mov.l   @(depth, ot), next
        mov.l   face, @(depth, ot)

        shll16  vp3
        xtrct   vp2, vp3
        shll16  vp1
        xtrct   vp0, vp1

        mov.l   flags, @(0, face)
        mov.l   next, @(4, face)
        mov.l   vp1, @(8, face)
        mov.l   vp3, @(12, face)
        add     #FACE_SIZEOF, face
.skip_farq:
        dt      count
        bf      .loop_farq

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
#undef vp3
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
#undef vz3
#undef depth
#undef next
#undef ot