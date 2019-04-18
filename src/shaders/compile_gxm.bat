@echo off
rd /S /Q gxm
mkdir gxm

call :compile_au compose_sprite
call :compile_au compose_room
call :compile_au compose_entity
call :compile compose_mirror
call :compile compose_flash

call :compile_a shadow_entity
call :compile_a ambient_room
call :compile_a ambient_sprite

call :compile water_drop
call :compile water_simulate
call :compile water_caustics
call :compile water_rays
call :compile water_mask
call :compile water_compose

call :compile filter    _upscale    "-DUPSCALE"
call :compile filter    _downsample "-DDOWNSAMPLE"
call :compile filter    _grayscale  "-DGRAYSCALE"
call :compile filter    _blur       "-DBLUR"

call :compile gui

call :compile clear

cd gxm
set CONV_CMD=..\bin2c.exe

for /r . %%i in (*.gxp) do (
::    ..\psp2gxpstrip -o %%i %%i
    ..\bin2c.exe %%i %%~ni.h %%~ni
)

cd ..

EXIT /B %ERRORLEVEL%

:compile
    SETLOCAL
    echo compile gxm/%~1%~2 %~3
    echo #include "%~1%~2_v.h" >> gxm/shaders.h
    echo #include "%~1%~2_f.h" >> gxm/shaders.h
    psp2cgc -profile sce_vp_psp2 -W4 -Wperf -D_GAPI_GXM=1 -pedantic %~1.hlsl -cache -o gxm/%~1%~2_v.gxp %~3 -DVERTEX
    psp2cgc -profile sce_fp_psp2 -W4 -Wperf -D_GAPI_GXM=1 -pedantic %~1.hlsl -cache -o gxm/%~1%~2_f.gxp %~3 -DPIXEL
    ENDLOCAL
EXIT /B 0

:compile_a
    SETLOCAL
    call :compile %~1
    call :compile %~1   _a      "-DALPHA_TEST"
    ENDLOCAL
EXIT /B 0

:compile_au
    SETLOCAL
    call :compile_a %~1
    call :compile %~1   _u      "-DUNDERWATER"
    call :compile %~1   _au     "-DALPHA_TEST -DUNDERWATER" 
    ENDLOCAL
EXIT /B 0
