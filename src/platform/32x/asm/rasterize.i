#define type    r0
#define proc    r1

#define flags   r4      // arg
#define L       r5      // arg
#define tile    r6      // arg
#define R       tile
#define pixel   flags
#define y       type

.align 4
.global _rasterize_asm
_rasterize_asm:
        mov     tile, r7                // set 4th arg for proc
        mov     flags, type
        shll2   type
        shlr16  type
        extu.b  type, proc

        cmp/eq  #FACE_TYPE_F, type      // cmp/eq #imm is 8-bit
        bf/s    .getProc
        mov     L, R                    // [delay slot]
        extu.b  flags, R

.getProc: // proc = table[type]
        mova    var_table, type
        shll2   proc
        mov.l   @(type, proc), proc

        // pixel = fb + y * 320 = fb + y * 256 + y * 64
        mov.w   @(VERTEX_Y, L), y
        // FRAME_WIDTH = 320 = 256 + 64
        mov.l   var_fb, pixel
        shll8   y
        add     y, pixel        // pixel += y * 256
        shar    y
        shar    y
        jmp     @proc
        add     y, pixel        // [delay slot] pixel += y * 64

.align 2
var_fb:
        // overwrite image frame buffer address, it has the same
        // write latency but allow transparent write for byte & word
        .long 0x24020200
var_table:
#ifdef ON_CHIP_RENDER
        .long 0xC0000000 + _rasterizeS_asm - _block_render_start
        .long 0xC0000000 + _rasterizeF_asm - _block_render_start
        .long 0xC0000000 + _rasterizeFT_asm - _block_render_start
        .long 0xC0000000 + _rasterizeFT_asm - _block_render_start
        .long 0xC0000000 + _rasterizeGT_asm - _block_render_start
        .long 0xC0000000 + _rasterizeGT_asm - _block_render_start
#else
        .long _rasterizeS_asm
        .long _rasterizeF_asm
        .long _rasterizeFT_asm
        .long _rasterizeFT_asm
        .long _rasterizeGT_asm
        .long _rasterizeGT_asm
#endif
        .long _rasterizeSprite_c
        .long _rasterizeFillS_c
        .long _rasterizeLineH_c
        .long _rasterizeLineV_c

#undef type
#undef proc
#undef flags
#undef L
#undef R
#undef pixel
#undef y