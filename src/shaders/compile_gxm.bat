@echo off
rd /S /Q gxm
mkdir gxm
psp2cgc -profile sce_vp_psp2 -W4 -Wperf -pedantic -o gxm/compose_vp.gxp compose.hlsl -DPASS_COMPOSE -DVERTEX
psp2cgc -profile sce_fp_psp2 -W4 -Wperf -pedantic -o gxm/compose_fp.gxp compose.hlsl -DPASS_COMPOSE -DPIXEL
psp2cgc -profile sce_vp_psp2 -W4 -Wperf -pedantic -o gxm/shadow_vp.gxp shadow.hlsl -DPASS_SHADOW -DVERTEX
psp2cgc -profile sce_fp_psp2 -W4 -Wperf -pedantic -o gxm/shadow_fp.gxp shadow.hlsl -DPASS_SHADOW -DPIXEL
psp2cgc -profile sce_vp_psp2 -W4 -Wperf -pedantic -o gxm/ambient_vp.gxp ambient.hlsl -DPASS_AMBIENT -DVERTEX
psp2cgc -profile sce_fp_psp2 -W4 -Wperf -pedantic -o gxm/ambient_fp.gxp ambient.hlsl -DPASS_AMBIENT -DPIXEL
psp2cgc -profile sce_vp_psp2 -W4 -Wperf -pedantic -o gxm/water_vp.gxp water.hlsl -DVERTEX
psp2cgc -profile sce_fp_psp2 -W4 -Wperf -pedantic -o gxm/water_fp.gxp water.hlsl -DPIXEL
psp2cgc -profile sce_vp_psp2 -W4 -Wperf -pedantic -o gxm/filter_vp.gxp filter.hlsl -DVERTEX
psp2cgc -profile sce_fp_psp2 -W4 -Wperf -pedantic -o gxm/filter_fp.gxp filter.hlsl -DPIXEL
psp2cgc -profile sce_vp_psp2 -W4 -Wperf -pedantic -o gxm/gui_vp.gxp gui.hlsl -DVERTEX
psp2cgc -profile sce_fp_psp2 -W4 -Wperf -pedantic -o gxm/gui_fp.gxp gui.hlsl -DPIXEL
psp2cgc -profile sce_vp_psp2 -W4 -Wperf -pedantic -o gxm/clear_vp.gxp clear.hlsl -DVERTEX
psp2cgc -profile sce_fp_psp2 -W4 -Wperf -pedantic -o gxm/clear_fp.gxp clear.hlsl -DPIXEL

cd gxm
C:\pspsdk\bin\bin2c.exe compose_vp.gxp compose_vp.h COMPOSE_VP
C:\pspsdk\bin\bin2c.exe compose_fp.gxp compose_fp.h COMPOSE_FP
C:\pspsdk\bin\bin2c.exe shadow_vp.gxp shadow_vp.h SHADOW_VP
C:\pspsdk\bin\bin2c.exe shadow_fp.gxp shadow_fp.h SHADOW_FP
C:\pspsdk\bin\bin2c.exe ambient_vp.gxp ambient_vp.h AMBIENT_VP
C:\pspsdk\bin\bin2c.exe ambient_fp.gxp ambient_fp.h AMBIENT_FP
C:\pspsdk\bin\bin2c.exe water_vp.gxp water_vp.h WATER_VP
C:\pspsdk\bin\bin2c.exe water_fp.gxp water_fp.h WATER_FP
C:\pspsdk\bin\bin2c.exe filter_vp.gxp filter_vp.h FILTER_VP
C:\pspsdk\bin\bin2c.exe filter_fp.gxp filter_fp.h FILTER_FP
C:\pspsdk\bin\bin2c.exe gui_vp.gxp gui_vp.h GUI_VP
C:\pspsdk\bin\bin2c.exe gui_fp.gxp gui_fp.h GUI_FP
C:\pspsdk\bin\bin2c.exe clear_vp.gxp clear_vp.h CLEAR_VP
C:\pspsdk\bin\bin2c.exe clear_fp.gxp clear_fp.h CLEAR_FP


cd ..