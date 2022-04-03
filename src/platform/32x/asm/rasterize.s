#include "common.i"
SEG_RASTER

#define type    r0
#define proc    r1

#define flags   r4      // arg
#define L       r5      // arg
#define R       r6
#define pixel   flags
#define y       type

.align 4
.global _rasterize_asm
_rasterize_asm:
        mov     flags, type
        shll2   type
        shlr16  type
        extu.b  type, proc

        cmp/eq  #FACE_TYPE_F, type      // cmp/eq #imm is 8-bit
        bf/s    .getProc
        mov     L, R
        extu.b  flags, R

.getProc: // proc = table[type]
        mova    var_table, type
        shll2   proc
        mov.l   @(type, proc), proc

        // pixel = fb + y * 320 = fb + y * 256 + y * 64
        mov.w   @(VERTEX_Y, L), y
        mov.l   var_fb, pixel
        shll8   y
        add     y, pixel        // pixel += y * 256
        shar    y
        shar    y
        jmp     @proc
        add     y, pixel        // pixel += y * 64

.align 2
var_fb:
        // overwrite image frame buffer address has the same
        // write per but allow transparent write for byte & word
        .long 0x24020200
var_table:
        .long _rasterizeS_asm
        .long _rasterizeF_asm
        .long _rasterizeFT_asm
        .long _rasterizeFT_asm
        .long _rasterizeGT_asm
        .long _rasterizeGT_asm
        .long _rasterizeSprite_c
        .long _rasterizeFillS_c
        .long _rasterizeLineH_c
        .long _rasterizeLineV_c
