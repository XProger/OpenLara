@echo off
rd /S /Q gxm
mkdir gxm

psp2cgc -profile sce_vp_psp2 -W4 -Wperf -pedantic -o gxm/compose_sprite_vp.gxp compose_sprite.hlsl -DVERTEX
psp2cgc -profile sce_fp_psp2 -W4 -Wperf -pedantic -o gxm/compose_sprite_fp.gxp compose_sprite.hlsl -DPIXEL
psp2cgc -profile sce_vp_psp2 -W4 -Wperf -pedantic -o gxm/compose_flash_vp.gxp compose_flash.hlsl -DVERTEX
psp2cgc -profile sce_fp_psp2 -W4 -Wperf -pedantic -o gxm/compose_flash_fp.gxp compose_flash.hlsl -DPIXEL
psp2cgc -profile sce_vp_psp2 -W4 -Wperf -pedantic -o gxm/compose_room_vp.gxp compose_room.hlsl -DVERTEX
psp2cgc -profile sce_fp_psp2 -W4 -Wperf -pedantic -o gxm/compose_room_fp.gxp compose_room.hlsl -DPIXEL
psp2cgc -profile sce_vp_psp2 -W4 -Wperf -pedantic -o gxm/compose_entity_vp.gxp compose_entity.hlsl -DVERTEX
psp2cgc -profile sce_fp_psp2 -W4 -Wperf -pedantic -o gxm/compose_entity_fp.gxp compose_entity.hlsl -DPIXEL
psp2cgc -profile sce_vp_psp2 -W4 -Wperf -pedantic -o gxm/compose_mirror_vp.gxp compose_mirror.hlsl -DVERTEX
psp2cgc -profile sce_fp_psp2 -W4 -Wperf -pedantic -o gxm/compose_mirror_fp.gxp compose_mirror.hlsl -DPIXEL

psp2cgc -profile sce_vp_psp2 -W4 -Wperf -pedantic -o gxm/shadow_vp.gxp shadow.hlsl -DVERTEX
psp2cgc -profile sce_fp_psp2 -W4 -Wperf -pedantic -o gxm/shadow_fp.gxp shadow.hlsl -DPIXEL
psp2cgc -profile sce_vp_psp2 -W4 -Wperf -pedantic -o gxm/ambient_vp.gxp ambient.hlsl -DVERTEX
psp2cgc -profile sce_fp_psp2 -W4 -Wperf -pedantic -o gxm/ambient_fp.gxp ambient.hlsl -DPIXEL

psp2cgc -profile sce_vp_psp2 -W4 -Wperf -pedantic -o gxm/water_drop_vp.gxp water_drop.hlsl -DVERTEX
psp2cgc -profile sce_fp_psp2 -W4 -Wperf -pedantic -o gxm/water_drop_fp.gxp water_drop.hlsl -DPIXE
psp2cgc -profile sce_vp_psp2 -W4 -Wperf -pedantic -o gxm/water_simulate_vp.gxp water_simulate.hlsl -DVERTEX
psp2cgc -profile sce_fp_psp2 -W4 -Wperf -pedantic -o gxm/water_simulate_fp.gxp water_simulate.hlsl -DPIXEL
psp2cgc -profile sce_vp_psp2 -W4 -Wperf -pedantic -o gxm/water_caustics_vp.gxp water_caustics.hlsl -DVERTEX
psp2cgc -profile sce_fp_psp2 -W4 -Wperf -pedantic -o gxm/water_caustics_fp.gxp water_caustics.hlsl -DPIXEL
psp2cgc -profile sce_vp_psp2 -W4 -Wperf -pedantic -o gxm/water_rays_vp.gxp water_rays.hlsl -DVERTEX
psp2cgc -profile sce_fp_psp2 -W4 -Wperf -pedantic -o gxm/water_rays_fp.gxp water_rays.hlsl -DPIXEL
psp2cgc -profile sce_vp_psp2 -W4 -Wperf -pedantic -o gxm/water_mask_vp.gxp water_mask.hlsl -DVERTEX
psp2cgc -profile sce_fp_psp2 -W4 -Wperf -pedantic -o gxm/water_mask_fp.gxp water_mask.hlsl -DPIXEL
psp2cgc -profile sce_vp_psp2 -W4 -Wperf -pedantic -o gxm/water_compose_vp.gxp water_compose.hlsl -DVERTEX
psp2cgc -profile sce_fp_psp2 -W4 -Wperf -pedantic -o gxm/water_compose_fp.gxp water_compose.hlsl -DPIXEL


psp2cgc -profile sce_vp_psp2 -W4 -Wperf -pedantic -o gxm/filter_vp.gxp filter.hlsl -DVERTEX
psp2cgc -profile sce_fp_psp2 -W4 -Wperf -pedantic -o gxm/filter_fp.gxp filter.hlsl -DPIXEL
psp2cgc -profile sce_vp_psp2 -W4 -Wperf -pedantic -o gxm/gui_vp.gxp gui.hlsl -DVERTEX
psp2cgc -profile sce_fp_psp2 -W4 -Wperf -pedantic -o gxm/gui_fp.gxp gui.hlsl -DPIXEL
psp2cgc -profile sce_vp_psp2 -W4 -Wperf -pedantic -o gxm/clear_vp.gxp clear.hlsl -DVERTEX
psp2cgc -profile sce_fp_psp2 -W4 -Wperf -pedantic -o gxm/clear_fp.gxp clear.hlsl -DPIXEL

cd gxm
..\bin2c.exe compose_sprite_vp.gxp compose_sprite_vp.h COMPOSE_SPRITE_VP
..\bin2c.exe compose_sprite_fp.gxp compose_sprite_fp.h COMPOSE_SPRITE_FP
..\bin2c.exe compose_flash_vp.gxp compose_flash_vp.h COMPOSE_FLASH_VP
..\bin2c.exe compose_flash_fp.gxp compose_flash_fp.h COMPOSE_FLASH_FP
..\bin2c.exe compose_room_vp.gxp compose_room_vp.h COMPOSE_ROOM_VP
..\bin2c.exe compose_room_fp.gxp compose_room_fp.h COMPOSE_ROOM_FP
..\bin2c.exe compose_entity_vp.gxp compose_entity_vp.h COMPOSE_ENTITY_VP
..\bin2c.exe compose_entity_fp.gxp compose_entity_fp.h COMPOSE_ENTITY_FP
..\bin2c.exe compose_mirror_vp.gxp compose_mirror_vp.h COMPOSE_MIRROR_VP
..\bin2c.exe compose_mirror_fp.gxp compose_mirror_fp.h COMPOSE_MIRROR_FP

..\bin2c.exe shadow_vp.gxp shadow_vp.h SHADOW_VP
..\bin2c.exe shadow_fp.gxp shadow_fp.h SHADOW_FP
..\bin2c.exe ambient_vp.gxp ambient_vp.h AMBIENT_VP
..\bin2c.exe ambient_fp.gxp ambient_fp.h AMBIENT_FP

..\bin2c.exe water_drop_vp.gxp water_drop_vp.h WATER_DROP_VP
..\bin2c.exe water_drop_fp.gxp water_drop_fp.h WATER_DROP_FP
..\bin2c.exe water_simulate_vp.gxp water_simulate_vp.h WATER_SIMULATE_VP
..\bin2c.exe water_simulate_fp.gxp water_simulate_fp.h WATER_SIMULATE_FP
..\bin2c.exe water_caustics_vp.gxp water_caustics_vp.h WATER_CAUSTICS_VP
..\bin2c.exe water_caustics_fp.gxp water_caustics_fp.h WATER_CAUSTICS_FP
..\bin2c.exe water_rays_vp.gxp water_rays_vp.h WATER_RAYS_VP
..\bin2c.exe water_rays_fp.gxp water_rays_fp.h WATER_RAYS_FP
..\bin2c.exe water_mask_vp.gxp water_mask_vp.h WATER_MASK_VP
..\bin2c.exe water_mask_fp.gxp water_mask_fp.h WATER_MASK_FP
..\bin2c.exe water_compose_vp.gxp water_compose_vp.h WATER_COMPOSE_VP
..\bin2c.exe water_compose_fp.gxp water_compose_fp.h WATER_COMPOSE_FP

..\bin2c.exe filter_vp.gxp filter_vp.h FILTER_VP
..\bin2c.exe filter_fp.gxp filter_fp.h FILTER_FP
..\bin2c.exe gui_vp.gxp gui_vp.h GUI_VP
..\bin2c.exe gui_fp.gxp gui_fp.h GUI_FP
..\bin2c.exe clear_vp.gxp clear_vp.h CLEAR_VP
..\bin2c.exe clear_fp.gxp clear_fp.h CLEAR_FP


cd ..