#include "common.i"
.data

.global _block_render_start
.global _block_render_end

.align 4
_block_render_start:

#include "rasterize.i"
//#include "rasterize_dummy.i"
#include "rasterizeS.i"
#include "rasterizeF.i"

.align 2
var_LMAP_ADDR_fs:
        .long   _gLightmap_base
var_divTable_fs:
        .long   _divTable
var_frameWidth_fs:
        .word   FRAME_WIDTH

#include "rasterizeFT.i"
#include "rasterizeGT.i"

.align 2
var_LMAP_ADDR:
        .long   _gLightmap_base
var_divTable:
        .long   _divTable
var_mask:
        .word   0xFF00
var_frameWidth:
        .word   FRAME_WIDTH

_block_render_end:
