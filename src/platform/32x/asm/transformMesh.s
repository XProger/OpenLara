#include "common.i"
SEG_TRANS

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
.vg_max:
        mov     #31, tmp
        cmp/gt  tmp, vg
        bf/s    .vg_min
        cmp/pz  vg              // T = vg >= 0
        mov     tmp, vg
.vg_min:
        subc    tmp, tmp        // tmp = -T
        and     tmp, vg

        add     #1, vg          // +1 for signed lightmap fetch

        shll8   vg              // lower 8 bits = vertex.clip flags
        add     #8, res         // extra offset for @-Rn

        // pre-transform the matrix offset
        add     #M03, m
        mov.w   @m+, mx
        shll16  mx
        mov.w   @m+, my
        shll16  my
        mov.w   @m+, mz
        shll16  mz
        add     #-MATRIX_SIZEOF, m

.loop:
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
.clip_z_near:
        mov     #VIEW_MIN, minZ // 64
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

        add     #-M03, m        // reset matrix ptr

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

.apply_offset:
        // x += FRAME_WIDTH / 2 (160)
        add     #100, x         // x += 100
        add     #60, x          // x += 60
        // y += FRAME_HEIGHT / 2 (112)
        add     #112, y         // y += 112

.clip_frame_x:  // 0 < x > FRAME_WIDTH
        mov     #80, tmp
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

        // pop
        mov.l   @sp+, r13
        mov.l   @sp+, r12
        mov.l   @sp+, r11
        mov.l   @sp+, r10
        mov.l   @sp+, r9
        rts
        mov.l   @sp+, r8

.align 2
var_gVerticesBase:
        .long   _gVerticesBase
var_gMatrixPtr:
        .long   _gMatrixPtr
var_gLightAmbient:
        .long   _gLightAmbient
var_divTable:
        .long   _divTable
