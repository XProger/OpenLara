#include "common.i"

#define type    r0
#define proc    r1

#define flags   r4      // arg
#define L       r5      // arg
#define R       r6
#define pixel   flags
#define y       type

.text
.align 4
.global _rasterize_asm
_rasterize_asm:
        mov     flags, type
        shll2   type
        swap.w  type, type
        and     #15, type

        cmp/eq  #FACE_TYPE_F, type
        bf/s    0f
        mov     L, R
        extu.b  flags, R

0:      // proc = table[type]
        mov     type, proc
        mova    var_table, type
        shll2   proc
        mov.l   @(type, proc), proc

        // pixel = fb + y * 320
        mov.w   @(VERTEX_Y, L), y
        mov.l   var_fb, pixel
        shll8   y
        add     y, pixel        // pixel += y * 256
        shlr2   y
        jmp     @proc
        add     y, pixel        // pixel += y * 64
        nop

var_fb:
        .long 0x24000200
var_table:
        .long _rasterizeS_c
        .long _rasterizeF_c
        .long _rasterizeFT_c
        .long _rasterizeFTA_c
        .long _rasterizeGT_c
        .long _rasterizeGTA_c
        .long _rasterizeSprite_c
        .long _rasterizeFillS_c
        .long _rasterizeLineH_c
        .long _rasterizeLineV_c
