@echo off
fxc /nologo /T vs_3_0 /O3 /Vn BASE_VS /Fh base_vs.h base.hlsl /DVERTEX
fxc /nologo /T ps_3_0 /O3 /Vn BASE_PS /Fh base_ps.h base.hlsl /DPIXEL
fxc /nologo /T vs_3_0 /O3 /Vn WATER_VS /Fh water_vs.h water.hlsl /DVERTEX
fxc /nologo /T ps_3_0 /O3 /Vn WATER_PS /Fh water_ps.h water.hlsl /DPIXEL
fxc /nologo /T vs_3_0 /O3 /Vn FILTER_VS /Fh filter_vs.h filter.hlsl /DVERTEX
fxc /nologo /T ps_3_0 /O3 /Vn FILTER_PS /Fh filter_ps.h filter.hlsl /DPIXEL
fxc /nologo /T vs_3_0 /O3 /Vn GUI_VS /Fh gui_vs.h gui.hlsl /DVERTEX
fxc /nologo /T ps_3_0 /O3 /Vn GUI_PS /Fh gui_ps.h gui.hlsl /DPIXEL