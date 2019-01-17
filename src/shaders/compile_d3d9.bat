@echo off
rd /S /Q d3d9
mkdir d3d9
fxc /nologo /T vs_3_0 /O3 /Gec /Vn COMPOSE_SPRITE_VS /Fh d3d9/compose_sprite_vs.h compose_sprite.hlsl /DVERTEX
fxc /nologo /T ps_3_0 /O3 /Gec /Vn COMPOSE_SPRITE_PS /Fh d3d9/compose_sprite_ps.h compose_sprite.hlsl /DPIXEL
fxc /nologo /T vs_3_0 /O3 /Gec /Vn COMPOSE_FLASH_VS /Fh d3d9/compose_flash_vs.h compose_flash.hlsl /DVERTEX
fxc /nologo /T ps_3_0 /O3 /Gec /Vn COMPOSE_FLASH_PS /Fh d3d9/compose_flash_ps.h compose_flash.hlsl /DPIXEL
fxc /nologo /T vs_3_0 /O3 /Gec /Vn COMPOSE_ROOM_VS /Fh d3d9/compose_room_vs.h compose_room.hlsl /DVERTEX
fxc /nologo /T ps_3_0 /O3 /Gec /Vn COMPOSE_ROOM_PS /Fh d3d9/compose_room_ps.h compose_room.hlsl /DPIXEL
fxc /nologo /T vs_3_0 /O3 /Gec /Vn COMPOSE_ENTITY_VS /Fh d3d9/compose_entity_vs.h compose_entity.hlsl /DVERTEX
fxc /nologo /T ps_3_0 /O3 /Gec /Vn COMPOSE_ENTITY_PS /Fh d3d9/compose_entity_ps.h compose_entity.hlsl /DPIXEL
fxc /nologo /T vs_3_0 /O3 /Gec /Vn COMPOSE_MIRROR_VS /Fh d3d9/compose_mirror_vs.h compose_mirror.hlsl /DVERTEX
fxc /nologo /T ps_3_0 /O3 /Gec /Vn COMPOSE_MIRROR_PS /Fh d3d9/compose_mirror_ps.h compose_mirror.hlsl /DPIXEL

fxc /nologo /T vs_3_0 /O3 /Gec /Vn SHADOW_VS /Fh d3d9/shadow_vs.h shadow.hlsl /DVERTEX
fxc /nologo /T ps_3_0 /O3 /Gec /Vn SHADOW_PS /Fh d3d9/shadow_ps.h shadow.hlsl /DPIXEL
fxc /nologo /T vs_3_0 /O3 /Gec /Vn AMBIENT_VS /Fh d3d9/ambient_vs.h ambient.hlsl /DVERTEX
fxc /nologo /T ps_3_0 /O3 /Gec /Vn AMBIENT_PS /Fh d3d9/ambient_ps.h ambient.hlsl /DPIXEL

fxc /nologo /T vs_3_0 /O3 /Gec /Vn WATER_DROP_VS /Fh d3d9/water_drop_vs.h water_drop.hlsl /DVERTEX
fxc /nologo /T ps_3_0 /O3 /Gec /Vn WATER_DROP_PS /Fh d3d9/water_drop_ps.h water_drop.hlsl /DPIXEL
fxc /nologo /T vs_3_0 /O3 /Gec /Vn WATER_SIMULATE_VS /Fh d3d9/water_simulate_vs.h water_simulate.hlsl /DVERTEX
fxc /nologo /T ps_3_0 /O3 /Gec /Vn WATER_SIMULATE_PS /Fh d3d9/water_simulate_ps.h water_simulate.hlsl /DPIXEL
fxc /nologo /T vs_3_0 /O3 /Gec /Vn WATER_CAUSTICS_VS /Fh d3d9/water_caustics_vs.h water_caustics.hlsl /DVERTEX
fxc /nologo /T ps_3_0 /O3 /Gec /Vn WATER_CAUSTICS_PS /Fh d3d9/water_caustics_ps.h water_caustics.hlsl /DPIXEL
fxc /nologo /T vs_3_0 /O3 /Gec /Vn WATER_RAYS_VS /Fh d3d9/water_rays_vs.h water_rays.hlsl /DVERTEX
fxc /nologo /T ps_3_0 /O3 /Gec /Vn WATER_RAYS_PS /Fh d3d9/water_rays_ps.h water_rays.hlsl /DPIXEL
fxc /nologo /T vs_3_0 /O3 /Gec /Vn WATER_MASK_VS /Fh d3d9/water_mask_vs.h water_mask.hlsl /DVERTEX
fxc /nologo /T ps_3_0 /O3 /Gec /Vn WATER_MASK_PS /Fh d3d9/water_mask_ps.h water_mask.hlsl /DPIXEL
fxc /nologo /T vs_3_0 /O3 /Gec /Vn WATER_COMPOSE_VS /Fh d3d9/water_compose_vs.h water_compose.hlsl /DVERTEX
fxc /nologo /T ps_3_0 /O3 /Gec /Vn WATER_COMPOSE_PS /Fh d3d9/water_compose_ps.h water_compose.hlsl /DPIXEL

fxc /nologo /T vs_3_0 /O3 /Gec /Vn FILTER_VS /Fh d3d9/filter_vs.h filter.hlsl /DVERTEX
fxc /nologo /T ps_3_0 /O3 /Gec /Vn FILTER_PS /Fh d3d9/filter_ps.h filter.hlsl /DPIXEL
fxc /nologo /T vs_3_0 /O3 /Gec /Vn GUI_VS /Fh d3d9/gui_vs.h gui.hlsl /DVERTEX
fxc /nologo /T ps_3_0 /O3 /Gec /Vn GUI_PS /Fh d3d9/gui_ps.h gui.hlsl /DPIXEL
