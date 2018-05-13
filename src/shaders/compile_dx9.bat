@echo off
fxc /nologo /T vs_3_0 /O3 /Vn COMPOSE_VS /Fh compose_vs.h base.hlsl /DPASS_COMPOSE /DVERTEX
fxc /nologo /T ps_3_0 /O3 /Vn COMPOSE_PS /Fh compose_ps.h base.hlsl /DPASS_COMPOSE /DPIXEL
fxc /nologo /T vs_3_0 /O3 /Vn SHADOW_VS /Fh shadow_vs.h base.hlsl /DPASS_SHADOW /DVERTEX
fxc /nologo /T ps_3_0 /O3 /Vn SHADOW_PS /Fh shadow_ps.h base.hlsl /DPASS_SHADOW /DPIXEL
fxc /nologo /T vs_3_0 /O3 /Vn AMBIENT_VS /Fh ambient_vs.h base.hlsl /DPASS_AMBIENT /DVERTEX
fxc /nologo /T ps_3_0 /O3 /Vn AMBIENT_PS /Fh ambient_ps.h base.hlsl /DPASS_AMBIENT /DPIXEL
fxc /nologo /T vs_3_0 /O3 /Vn WATER_VS /Fh water_vs.h water.hlsl /DVERTEX
fxc /nologo /T ps_3_0 /O3 /Vn WATER_PS /Fh water_ps.h water.hlsl /DPIXEL
fxc /nologo /T vs_3_0 /O3 /Vn FILTER_VS /Fh filter_vs.h filter.hlsl /DVERTEX
fxc /nologo /T ps_3_0 /O3 /Vn FILTER_PS /Fh filter_ps.h filter.hlsl /DPIXEL
fxc /nologo /T vs_3_0 /O3 /Vn GUI_VS /Fh gui_vs.h gui.hlsl /DVERTEX
fxc /nologo /T ps_3_0 /O3 /Vn GUI_PS /Fh gui_ps.h gui.hlsl /DPIXEL