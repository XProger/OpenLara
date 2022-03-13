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
#define minX            r11
#define minY            r12
#define maxX            r13
#define maxY            r14

#define vg              intensity
#define ambient         tmp
#define dz              tmp
#define minZ            tmp

.macro transform v
        lds.l   @m+, MACL
        mac.w   @vertices+, @m+
        mac.w   @vertices+, @m+
        mac.w   @vertices+, @m+
        sts     MACL, \v
        // v >>= FIXED_SHIFT
        shll2   \v
        shlr16  \v
        exts.w  \v, \v
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
        mov.l   r14, @-sp

        mov.l   var_viewport, tmp
        mov.l   @tmp+, minX
        mov.l   @tmp+, minY
        mov.l   @tmp+, maxX
        mov.l   @tmp+, maxY

        mov.l   var_gMatrixPtr, tmp
        mov.l   @tmp, m

        mov.l   var_gVerticesBase, tmp
        mov.l   @tmp, res

        mov.l   var_gLightAmbient, tmp
        mov.l   @tmp, ambient

        mov.l   var_divTable, divLUT

        // maxZ = VIEW_MAX = (1024 * 10) = (40 << 8)
        mov     #40, maxZ
        shll8   maxZ

        add     intensity, ambient
        shlr8   ambient
        exts.b  ambient, vg

        // vg = clamp(vg, 0, 31)
.vg_max:
        mov     #31, tmp
        cmp/gt  tmp, vg
        bf/s    .vg_min
        cmp/pz  vg              // T = vg >= 0
        mov     tmp, vg
.vg_min:
        subc    tmp, tmp        // tmp = -T
        and     tmp, vg

        shll8   vg              // lower 8 bits = vertex.clip flags
        add     #8, res         // extra offset for @-Rn

.loop:
        // clear clipping flags
        shlr8   vg
        shll8   vg

        // transform to view space
        transform x
        add     #2, m           // next row
        add     #-6, vertices   // reset vertex ptr
        transform y
        add     #2, m           // next row
        add     #-6, vertices   // reset vertex ptr
        transform z
        add     #-34, m         // reset matrix ptr

        // z clipping
.clip_z_near:
        mov     #VIEW_MIN, minZ // 64
        cmp/ge  z, minZ
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

        // x += FRAME_WIDTH / 2 (160)
        add     #100, x
        add     #60, x
        // y += FRAME_HEIGHT / 2 (112)
        add     #100, y
        add     #12, y

        // viewport clipping
.clip_vp_minX:
        cmp/ge  x, minX
        bf/s    .clip_vp_minY
        cmp/ge  y, minY
        add     #CLIP_LEFT, vg
.clip_vp_minY:
        bf/s    .clip_vp_maxX
        cmp/ge  maxX, x
        add     #CLIP_TOP, vg
.clip_vp_maxX:
        bf/s    .clip_vp_maxY
        cmp/ge  maxY, y
        add     #CLIP_RIGHT, vg
.clip_vp_maxY:
        bf/s    .store_vertex
        dt      count
        add     #CLIP_BOTTOM, vg

.store_vertex:
        mov.w   vg, @-res
        mov.w   z, @-res
        mov.w   y, @-res
        mov.w   x, @-res

        bf/s    .loop
        add     #16, res

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
var_viewport:
        .long   _viewport
var_gMatrixPtr:
        .long   _gMatrixPtr
var_gVerticesBase:
        .long   _gVerticesBase
var_gLightAmbient:
        .long   _gLightAmbient
var_divTable:
        .long   _divTable
