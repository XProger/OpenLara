#define tmp             r0
#define maxZ            r1
#define divLUT          r2
#define res             r3
#define vertices        r4      // arg
#define count           r5      // arg
#define intensity       r6      // arg
#define m               r7
#define x               r8
#define y               r9
#define z               r10
#define mx              r11
#define my              r12
#define mz              r13

#define vg              intensity
#define ambient         tmp
#define dz              tmp
#define minZ            tmp

.macro transform v, offset
        lds     \offset, MACL
        mac.w   @vertices+, @m+
        mac.w   @vertices+, @m+
        mac.w   @vertices+, @m+
        sts     MACL, tmp
        // v += tmp >> (FIXED_SHIFT + FP16_SHIFT)
        shlr16  tmp
        exts.w  tmp, \v
.endm

.align 4
.global _transformMesh_asm
_transformMesh_asm:
        // push
        mov.l   r8, @-sp
        mov.l   r9, @-sp
        mov.l   r10, @-sp
        mov.l   r11, @-sp
        mov.l   r12, @-sp
        mov.l   r13, @-sp

        mov.l   var_gVerticesBase, tmp
        mov.l   @tmp, res

        mov.l   var_gMatrixPtr, tmp
        mov.l   @tmp, m

        mov.l   var_gLightAmbient, tmp
        mov.l   @tmp, ambient

        mov.l   var_divTable, divLUT

        // maxZ = VIEW_MAX = (1024 * 10) = (40 << 8)
        mov     #40, maxZ
        shll8   maxZ

        add     intensity, ambient
        shlr8   ambient
        exts.b  ambient, vg

        // vg = clamp(vg, 0, 31) + 1
.vg_max_m:
        mov     #31, tmp
        cmp/gt  tmp, vg
        bf/s    .vg_min_m
        cmp/pz  vg              // [delay slot] T = vg >= 0
        mov     tmp, vg
.vg_min_m:
        subc    tmp, tmp        // tmp = -T
        and     tmp, vg

        add     #1, vg          // +1 for signed lightmap fetch

        shll8   vg              // lower 8 bits = vertex.clip flags
        add     #8, res         // extra offset for @-Rn

        // pre-transform the matrix offset
        add     #M03, m
        mov.w   @m+, mx
        mov.w   @m+, my
        mov.w   @m+, mz
        shll16  mx
        shll16  my
        shll16  mz
        add     #-MATRIX_SIZEOF, m

.loop_m:
        // clear clipping flags
        shlr8   vg
        shll8   vg

        // transform to view space
        transform x, mx
        add     #-6, vertices   // reset vertex ptr
        transform y, my
        add     #-6, vertices   // reset vertex ptr
        transform z, mz

        // z clipping
.clip_z_near_m:
        mov     #VIEW_MIN, minZ
        cmp/gt  z, minZ
        bf/s    .clip_z_far_m
        cmp/ge  maxZ, z         // [delay slot]
        mov     minZ, z
        add     #CLIP_PLANE, vg
.clip_z_far_m:
        bf      .project_m
        mov     maxZ, z
        add     #CLIP_PLANE, vg

.project_m:
        // z >>= OT_SHIFT
        shlr2   z
        shlr2   z

        // dz = divTable[z]
        mov     z, dz
        shll    dz
        mov.w   @(dz, divLUT), dz

        // x = x * dz >> 12
        // y = y * dz >> 12
        muls.w  dz, x
        sts     MACL, x
          add     #-M03, m        // reset matrix ptr
        muls.w  dz, y
          shll2   x
          shll2   x
          shlr16  x
        sts     MACL, y
          exts.w  x, x
        shll2   y
        shll2   y
        shlr16  y
        exts.w  y, y

        // apply_offset
        // x += FRAME_WIDTH / 2 (160)
        add     #100, x         // x += 100
        add     #60, x          // x += 60
        // y += FRAME_HEIGHT / 2 (112)
        add     #112, y         // y += 112

        // 0 < x > FRAME_WIDTH
        mov     #80, tmp
        shll2   tmp             // tmp = 80 * 4 = 320 = FRAME_WIDTH
        cmp/hi  tmp, x
        bt/s    .clip_frame_m
        add     #-96, tmp       // [delay slot] tmp = 320 - 96 = 224 = FRAME_HEIGHT
        // 0 < y > FRAME_HEIGHT
        cmp/hi  tmp, y
.clip_frame_m:
        movt    tmp
        or      tmp, vg         // vg |= CLIP_FRAME

        // store_vertex
        mov.w   vg, @-res
        mov.w   z, @-res
        mov.w   y, @-res
        mov.w   x, @-res

        dt      count
        bf/s    .loop_m
        add     #16, res        // [delay slot]

        // pop
        mov.l   @sp+, r13
        mov.l   @sp+, r12
        mov.l   @sp+, r11
        mov.l   @sp+, r10
        mov.l   @sp+, r9
        rts
        mov.l   @sp+, r8

#undef tmp
#undef maxZ
#undef divLUT
#undef res
#undef vertices
#undef count
#undef intensity
#undef m
#undef x
#undef y
#undef z
#undef mx
#undef my
#undef mz
#undef vg
#undef ambient
#undef dz
#undef minZ