@echo off
rd /S /Q d3d9
mkdir d3d9

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

call :compile filter    _upscale    "/DUPSCALE"
call :compile filter    _downsample "/DDOWNSAMPLE"
call :compile filter    _grayscale  "/DGRAYSCALE"
call :compile filter    _blur       "/DBLUR"

call :compile gui


EXIT /B %ERRORLEVEL%

:compile
    SETLOCAL
    echo compile d3d9/%~1%~2 %~3
    echo #include "%~1%~2_v.h" >> d3d9/shaders.h
    echo #include "%~1%~2_f.h" >> d3d9/shaders.h
    fxc /nologo /T vs_3_0 /O3 /Gec /Vn %~1%~2_v /Fh d3d9/%~1%~2_v.h %~1.hlsl /DVERTEX %~3
    fxc /nologo /T ps_3_0 /O3 /Gec /Vn %~1%~2_f /Fh d3d9/%~1%~2_f.h %~1.hlsl /DPIXEL %~3
    ENDLOCAL
EXIT /B 0

:compile_a
    SETLOCAL
    call :compile %~1
    call :compile %~1   _a      "/DALPHA_TEST"
    ENDLOCAL
EXIT /B 0

:compile_au
    SETLOCAL
    call :compile_a %~1
    call :compile %~1   _u      "/DUNDERWATER"
    call :compile %~1   _au     "/DALPHA_TEST /DUNDERWATER" 
    ENDLOCAL
EXIT /B 0
