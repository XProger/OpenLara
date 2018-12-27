@echo off
rd /S /Q d3d9
mkdir d3d9
fxc /nologo /T vs_3_0 /O3 /Gec /Vn COMPOSE_VS /Fh d3d9/compose_vs.h compose.hlsl /DPASS_COMPOSE /DVERTEX
fxc /nologo /T ps_3_0 /O3 /Gec /Vn COMPOSE_PS /Fh d3d9/compose_ps.h compose.hlsl /DPASS_COMPOSE /DPIXEL
fxc /nologo /T vs_3_0 /O3 /Gec /Vn SHADOW_VS /Fh d3d9/shadow_vs.h shadow.hlsl /DPASS_SHADOW /DVERTEX
fxc /nologo /T ps_3_0 /O3 /Gec /Vn SHADOW_PS /Fh d3d9/shadow_ps.h shadow.hlsl /DPASS_SHADOW /DPIXEL
fxc /nologo /T vs_3_0 /O3 /Gec /Vn AMBIENT_VS /Fh d3d9/ambient_vs.h ambient.hlsl /DPASS_AMBIENT /DVERTEX
fxc /nologo /T ps_3_0 /O3 /Gec /Vn AMBIENT_PS /Fh d3d9/ambient_ps.h ambient.hlsl /DPASS_AMBIENT /DPIXEL
fxc /nologo /T vs_3_0 /O3 /Gec /Vn WATER_VS /Fh d3d9/water_vs.h water.hlsl /DVERTEX
fxc /nologo /T ps_3_0 /O3 /Gec /Vn WATER_PS /Fh d3d9/water_ps.h water.hlsl /DPIXEL
fxc /nologo /T vs_3_0 /O3 /Gec /Vn FILTER_VS /Fh d3d9/filter_vs.h filter.hlsl /DVERTEX
fxc /nologo /T ps_3_0 /O3 /Gec /Vn FILTER_PS /Fh d3d9/filter_ps.h filter.hlsl /DPIXEL
fxc /nologo /T vs_3_0 /O3 /Gec /Vn GUI_VS /Fh d3d9/gui_vs.h gui.hlsl /DVERTEX
fxc /nologo /T ps_3_0 /O3 /Gec /Vn GUI_PS /Fh d3d9/gui_ps.h gui.hlsl /DPIXEL
