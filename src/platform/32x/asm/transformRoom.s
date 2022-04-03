#include "common.i"
SEG_TRANS

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

.macro transform v, offset
        lds     \offset, MACL
        mac.w   @stackVtx+, @stackMtx+
        mac.w   @stackVtx+, @stackMtx+
        mac.w   @stackVtx+, @stackMtx+
        add     #-6, stackVtx
        sts     MACL, \v
        shlr8   \v
        exts.w  \v, \v
.endm

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
.copyMtx:
        mov.w   @tmp+, mx
        dt      cnt
        bf/s    .copyMtx
        mov.w   mx, @-stackMtx

        // prepare offsets (const)
        mov.w   @tmp+, mx
        mov.w   @tmp+, my
        mov.w   @tmp+, mz
        shll8   mx
        shll8   my
        shll8   mz

        add     #8, res         // extra offset for @-Rn

.loop:
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

        mov.w   x, @-stackVtx
        mov.w   y, @-stackVtx
        mov.w   z, @-stackVtx

        // transform to view space
        //transform z, mz

        lds     mz, MACL
        mac.w   @stackVtx+, @stackMtx+
        mac.w   @stackVtx+, @stackMtx+
        mac.w   @stackVtx+, @stackMtx+
          add     #-6, stackVtx
        sts     MACL, z
        shlr8   z
        exts.w  z, z


.z_range_check: // check if z in [-VIEW_OFF..VIEW_MAX + VIEW_OFF]
        // tmp = z + VIEW_OFF = z + 4096
        mov     #16, tmp
        shll8   tmp
        add     z, tmp
        // maxZ = VIEW_OFF + VIEW_MAX + VIEW_OFF = 18432
        mov     #72, maxZ
        shll8   maxZ
        // check if z in [-VIEW_OFF..VIEW_MAX + VIEW_OFF]
        cmp/hi  maxZ, tmp
        bf/s    .visible
        mov     #40, maxZ       // maxZ = 40 (delay slot)
        mov     #(CLIP_NEAR + CLIP_FAR), vg
        mov.w   vg, @-res
        add     #1, vertices
        dt      count
        bf/s    .loop
        add     #10, res
        bra     .done
        nop

.visible:
        //transform y, my
        lds     my, MACL
        mac.w   @stackVtx+, @stackMtx+
        mac.w   @stackVtx+, @stackMtx+
        mac.w   @stackVtx+, @stackMtx+
          add     #-6, stackVtx
        sts     MACL, y
        shlr8   y
        exts.w  y, y


        //transform x, mx
        lds     mx, MACL
        mac.w   @stackVtx+, @stackMtx+
        mac.w   @stackVtx+, @stackMtx+
        mac.w   @stackVtx+, @stackMtx+
          shll8   maxZ  // maxZ = VIEW_MAX = (1024 * 10) = (40 << 8)
        sts     MACL, x
        shlr8   x
        exts.w  x, x

        mov.b   @vertices+, vg

        // tmp = FOG_MIN = 6144 = (24 << 8)
        mov     #24, tmp
        shll8   tmp
        // if z <= FOG_MIN -> skip fog calc
        cmp/gt  tmp, z
        bf/s    .clip_z_near
        mov     z, fog
        sub     tmp, fog        // fog = z - FOG_MIN
        shll    fog             // FOG_SHIFT
        shlr8   fog             // shift down to 0..31 range
        add     fog, vg
        // vg = min(vg, 31)
        mov     #31, tmp
        cmp/gt  tmp, vg
        bf      .clip_z_near
        mov     #31, vg

        // z clipping
.clip_z_near:
        add     #1, vg          // +1 for signed lightmap fetch
        mov     #VIEW_MIN, minZ // minZ = VIEW_MIN = 64
        cmp/gt  z, minZ
        bf/s    .clip_z_far
        shll8   vg              // clear lower 8-bits of vg for clipping flags (delay slot)
        mov     minZ, z
        add     #CLIP_NEAR, vg
.clip_z_far:
        cmp/ge  maxZ, z
        bf/s    .project
        mov     z, dz
        mov     maxZ, z
        add     #CLIP_FAR, vg

.project: // dz = divTable[z >> (PROJ_SHIFT = 4)]
        shlr2   dz
        shlr2   dz
        shll    dz
        mov.w   @(dz, divLUT), dz

.proj_x: // x = x * dz >> 12
        muls.w  dz, x
        sts     MACL, x

.proj_y: // y = y * dz >> 12
        muls.w  dz, y
        shar12  x, tmp          // do it here to hide muls.w latency
        sts     MACL, y
        shar12  y, tmp

        // portal rect clipping
.clip_vp_minX:
        mov.w   @(0, vp), minX
        cmp/gt  x, minX
        bf/s    .clip_vp_minY
        mov.w   @(2, vp), minY
        add     #CLIP_LEFT, vg
.clip_vp_minY:
        cmp/ge  y, minY
        bf/s    .clip_vp_maxX
        mov.w   @(4, vp), maxX
        add     #CLIP_TOP, vg
.clip_vp_maxX:
        cmp/gt  maxX, x
        bf/s    .clip_vp_maxY
        mov.w   @(6, vp), maxY
        add     #CLIP_RIGHT, vg
.clip_vp_maxY:
        cmp/ge  maxY, y
        bf/s    .apply_offset
        mov     #80, tmp        // tmp = 80 (delay slot)
        add     #CLIP_BOTTOM, vg

.apply_offset:
        // x += FRAME_WIDTH / 2 (160)
        add     #100, x         // x += 100
        add     #60, x          // x += 60
        // y += FRAME_HEIGHT / 2 (112)
        add     #112, y         // y += 112

        // frame rect clipping
.clip_frame_x:  // 0 < x > FRAME_WIDTH
        shll2   tmp             // tmp = 80 * 4 = 320 = FRAME_WIDTH
        cmp/hi  tmp, x
        bt/s    .clip_frame
        add     #-96, tmp       // tmp = 320 - 96 = 224 = FRAME_HEIGHT (delay slot)
.clip_frame_y:  // 0 < y > FRAME_HEIGHT
        cmp/hi  tmp, y
.clip_frame:
        movt    tmp
        or      tmp, vg         // vg |= CLIP_FRAME

.store_vertex:
        mov.w   vg, @-res
        mov.w   z, @-res
        mov.w   y, @-res
        mov.w   x, @-res

        dt      count
        bf/s    .loop
        add     #16, res
.done:
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

.align 2
var_viewportRel:
        .long   _viewportRel
var_gVerticesBase:
        .long   _gVerticesBase
var_divTable:
        .long   _divTable
var_gMatrixPtr:
        .long   _gMatrixPtr
