#define tmp             r0
#define maxZ            r1
#define divLUT          r2
#define res             r3
#define vertices        r4      // arg
#define count           r5      // arg
#define stackVtx        r6
#define stackMtx        r7
#define vp              r8
#define x               r9
#define y               r10
#define z               r11
#define mx              r12     // const
#define my              r13     // const
#define mz              r14     // const

#define minX            tmp
#define minY            tmp
#define maxX            tmp
#define maxY            tmp
#define minZ            tmp
#define dz              tmp
#define vg              stackVtx
#define fog             stackMtx
#define cnt             stackVtx

#define SP_SIZE         (18 + 6)        // mat3x3 + vec3

.align 4
.global _transformRoom_asm
_transformRoom_asm:
        // push
        mov.l   r8, @-sp
        mov.l   r9, @-sp
        mov.l   r10, @-sp
        mov.l   r11, @-sp
        mov.l   r12, @-sp
        mov.l   r13, @-sp
        mov.l   r14, @-sp
        mov     sp, stackMtx 
        add     #-SP_SIZE, sp

        mov.l   var_viewportRel, vp

        mov.l   var_gVerticesBase, tmp
        mov.l   @tmp, res

        mov.l   var_divTable, divLUT

        // store matrix into stack (in reverse order)
        mov.l   var_gMatrixPtr, tmp
        mov.l   @tmp, tmp

        // copy 3x3 matrix rotation part
        mov     #9, cnt
.copyMtx_r:
        mov.w   @tmp+, mx
        dt      cnt
        bf/s    .copyMtx_r
        mov.w   mx, @-stackMtx  // [delay slot]

        // prepare offsets (const)
        mov.w   @tmp+, mx
        mov.w   @tmp+, my
        mov.w   @tmp+, mz
        shll8   mx
        shll8   my
        shll8   mz

        add     #8, res         // extra offset for @-Rn
        nop

.loop_r:
        // unpack vertex
        mov.b   @vertices+, x
        mov.b   @vertices+, y
        mov.b   @vertices+, z

        shll2   x
        shll2   y
        shll2   z

        // upload vertex coords into stack (in reverse order)
        mov     sp, stackVtx
        add     #6, stackVtx
        mov     stackVtx, stackMtx

        //shll16  x
        //xtrct   y, x
        mov.w   x, @-stackVtx
        mov.w   y, @-stackVtx
        mov.w   z, @-stackVtx

        //transform z
        lds     mz, MACL
        mac.w   @stackVtx+, @stackMtx+
        mac.w   @stackVtx+, @stackMtx+
        mac.w   @stackVtx+, @stackMtx+
        sts     MACL, z
          add     #-6, stackVtx
        shlr8   z
        exts.w  z, z


        // check if z in [-VIEW_OFF..VIEW_MAX + VIEW_OFF]
        // tmp = z + VIEW_OFF = z + 4096
        mov     #16, tmp
        shll8   tmp
        add     z, tmp
        // maxZ = VIEW_OFF + VIEW_MAX + VIEW_OFF = 18432
        mov     #72, maxZ
        shll8   maxZ
        // check if z in [-VIEW_OFF..VIEW_MAX + VIEW_OFF]
        cmp/hi  maxZ, tmp
        bf/s    .visible_r
        mov     #40, maxZ       // [delay slot] maxZ = 40
        mov     #(CLIP_NEAR + CLIP_FAR), vg
        mov.w   vg, @-res
        add     #1, vertices
        dt      count
        bf/s    .loop_r
        add     #10, res        // [delay slot]
        bra     .done_r
        nop

.visible_r:
        //transform y
        lds     my, MACL
        mac.w   @stackVtx+, @stackMtx+
        mac.w   @stackVtx+, @stackMtx+
        mac.w   @stackVtx+, @stackMtx+
        sts     MACL, y
          add     #-6, stackVtx
        shlr8   y
        exts.w  y, y

        //transform x
        lds     mx, MACL
        mac.w   @stackVtx+, @stackMtx+
        mac.w   @stackVtx+, @stackMtx+
        mac.w   @stackVtx+, @stackMtx+
        sts     MACL, x
          shll8   maxZ  // maxZ = VIEW_MAX = (1024 * 10) = (40 << 8)
        shlr8   x
        exts.w  x, x

        mov.b   @vertices+, vg

        // tmp = FOG_MIN = 6144 = (24 << 8)
        mov     #24, tmp
        shll8   tmp
        // if z <= FOG_MIN -> skip fog calc
        cmp/gt  tmp, z
        bf/s    .clip_z_near_r
        mov     z, fog          // [delay slot]
        sub     tmp, fog        // fog = z - FOG_MIN
        shll    fog             // FOG_SHIFT
        shlr8   fog             // shift down to 0..31 range
        add     fog, vg
        // vg = min(vg, 31)
        mov     #31, tmp
        cmp/gt  tmp, vg
        bf      .clip_z_near_r
        mov     #31, vg

        // z clipping
.clip_z_near_r:
        add     #1, vg          // +1 for signed lightmap fetch
        mov     #VIEW_MIN, minZ // minZ = VIEW_MIN = 64
        cmp/gt  z, minZ
        bf/s    .clip_z_far_r
        shll8   vg              // [delay slot] clear lower 8-bits of vg for clipping flags
        mov     minZ, z
        add     #CLIP_NEAR, vg
.clip_z_far_r:
        cmp/ge  maxZ, z
        bf/s    .project_r
        mov     z, dz           // [delay slot]
        mov     maxZ, z
        add     #CLIP_FAR, vg

.project_r: // dz = divTable[z >> (PROJ_SHIFT = 4)]
        shlr2   dz
        shlr2   dz
        shll    dz
        mov.w   @(dz, divLUT), dz

        // x = x * dz >> 12
        muls.w  dz, x
        sts     MACL, x

        // y = y * dz >> 12
        muls.w  dz, y
        sts     MACL, y

        shar12  x, tmp
        shar12  y, tmp

        // portal rect clipping
.clip_vp_minX_r:
        mov.w   @(0, vp), minX
        cmp/gt  x, minX
        bf/s    .clip_vp_minY_r
        mov.w   @(2, vp), minY  // [delay slot]
        add     #CLIP_LEFT, vg
.clip_vp_minY_r:
        cmp/ge  y, minY
        bf/s    .clip_vp_maxX_r
        mov.w   @(4, vp), maxX  // [delay slot]
        add     #CLIP_TOP, vg
.clip_vp_maxX_r:
        cmp/gt  maxX, x
        bf/s    .clip_vp_maxY_r
        mov.w   @(6, vp), maxY  // [delay slot]
        add     #CLIP_RIGHT, vg
.clip_vp_maxY_r:
        cmp/ge  maxY, y
        bf/s    .apply_offset_r
        mov     #80, tmp        // [delay slot] tmp = 80
        add     #CLIP_BOTTOM, vg

.apply_offset_r:
        // x += FRAME_WIDTH / 2 (160)
        add     #100, x         // x += 100
        add     #60, x          // x += 60
        // y += FRAME_HEIGHT / 2 (112)
        add     #112, y         // y += 112

        // frame rect clipping
        // 0 < x > FRAME_WIDTH
        shll2   tmp             // tmp = 80 * 4 = 320 = FRAME_WIDTH
        cmp/hi  tmp, x
        bt/s    .clip_frame_r
        add     #-96, tmp       // [delay slot] tmp = 320 - 96 = 224 = FRAME_HEIGHT
        // 0 < y > FRAME_HEIGHT
        cmp/hi  tmp, y
.clip_frame_r:
        movt    tmp
        or      tmp, vg         // vg |= CLIP_FRAME

        // store_vertex
        mov.w   vg, @-res
        mov.w   z, @-res
        mov.w   y, @-res
        mov.w   x, @-res

        dt      count
        bf/s    .loop_r
        add     #16, res        // [delay slot]
.done_r:
        // pop
        add     #SP_SIZE, sp
        mov.l   @sp+, r14
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
#undef stackVtx
#undef stackMtx
#undef vp
#undef x
#undef y
#undef z
#undef mx
#undef my
#undef mz
#undef minX
#undef minY
#undef maxX
#undef maxY
#undef minZ
#undef dz
#undef vg
#undef fog
#undef cnt
#undef SP_SIZE