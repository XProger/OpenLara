#include "common.i"
SEG_TRANS

#define tmp             r0
#define maxZ            r1
#define divLUT          r2
#define res             r3
#define vertices        r4      // arg
#define count           r5      // arg
#define vp              r6
#define m               r7
#define x               r8
#define y               r9
#define z               r10
#define vx              r11
#define vy              r12
#define vz              r13
#define vg              r14

#define minX            tmp
#define minY            tmp
#define maxX            tmp
#define maxY            tmp
#define minZ            tmp
#define dz              tmp
#define fog             vx

.macro transform v, row
        mov.w   @(\row * 6, m), tmp
        muls.w  vx, tmp
        sts     MACL, \v

        mov.w   @(\row * 6 + 2, m), tmp
        muls.w  vy, tmp
        sts     MACL, tmp
        add     tmp, \v

        mov.w   @(\row * 6 + 4, m), tmp
        muls.w  vz, tmp
        sts     MACL, tmp
        add     tmp, \v

        mov.w   @(\row * 2 + M03, m), tmp
        shll2   \v
        shlr8   \v
        exts.w  \v, \v
        add     tmp, \v
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

        mov.l   var_viewportRel, vp

        mov.l   var_gVerticesBase, tmp
        mov.l   @tmp, res

        mov.l   var_gMatrixPtr, tmp
        mov.l   @tmp, m

        mov.l   var_divTable, divLUT

        add     #8, res         // extra offset for @-Rn

.loop:
        // unpack vertex
        mov.l   @vertices+, vg
        extu.b  vg, vx
        shlr8   vg
        extu.b  vg, vy
        shlr8   vg
        extu.b  vg, vz

        // transform to view space
        transform z, 2

.z_range_check:
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
        dt      count
        bf/s    .loop
        add     #10, res
        bra     .done
        // delay slot from transform (mov.w)

.visible:
        transform x, 0
        transform y, 1

        // maxZ = VIEW_MAX = (1024 * 10) = (40 << 8)
        shll8   maxZ
        shlr8   vg
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
        shll8   vg              // clear lower 8-bits of vg for clipping flags
        mov     #VIEW_MIN, minZ // minZ = VIEW_MIN = 64
        cmp/gt  z, minZ
        bf/s    .clip_z_far
        cmp/ge  maxZ, z
        mov     minZ, z
        add     #CLIP_NEAR, vg
.clip_z_far:
        bf/s    .project
        mov     z, dz           // dz = z (delay slot)
        mov     maxZ, z
        add     #CLIP_FAR, vg

.project:
        // dz = divTable[z >> (PROJ_SHIFT = 4)]
        shlr2   dz
        shlr2   dz
        shll    dz
        mov.w   @(dz, divLUT), dz

        // x = x * dz >> (16 - PROJ_SHIFT)
        muls.w  dz, x
        sts     MACL, x
        shll2   x
        shll2   x
        shlr16  x
        exts.w  x, x

        // y = y * dz >> (16 - PROJ_SHIFT)
        muls.w  dz, y
        sts     MACL, y
        shll2   y
        shll2   y
        shlr16  y
        exts.w  y, y

        // viewport clipping
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
        bf/s    .store_vertex
        dt      count
        add     #CLIP_BOTTOM, vg

.store_vertex:
        // x += FRAME_WIDTH / 2 (160)
        add     #100, x
        add     #60, x
        // y += FRAME_HEIGHT / 2 (112)
        add     #112, y

        mov.w   vg, @-res
        mov.w   z, @-res
        mov.w   y, @-res
        mov.w   x, @-res

        bf/s    .loop
        add     #16, res
.done:
        // pop
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
var_gMatrixPtr:
        .long   _gMatrixPtr
var_divTable:
        .long   _divTable
