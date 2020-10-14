@echo off
rd /S /Q d3d8
mkdir d3d8

call :compile_u compose_sprite
call :compile_u compose_room
call :compile_u compose_entity
call :compile compose_mirror
call :compile compose_flash

call :compile shadow_entity
call :compile ambient_room
call :compile ambient_sprite

call :compile filter    _upscale    "/D UPSCALE"
call :compile filter    _downsample "/D DOWNSAMPLE"
call :compile filter    _grayscale  "/D GRAYSCALE"
call :compile filter    _blur       "/D BLUR"
call :compile filter    _anaglyph   "/D ANAGLYPH"

call :compile gui

del tmp

EXIT /B %ERRORLEVEL%

:compile
    SETLOCAL
    echo compile d3d8/%~1%~2 %~3
    echo #include "%~1%~2_v.h" >> d3d8/shaders.h
    echo #include "%~1%~2_f.h" >> d3d8/shaders.h
    xsasm /D VERTEX /O2 %~3 %~1.asm tmp
    bin2c -n %~1%~2_v -o d3d8/%~1%~2_v.h tmp
    xsasm /D PIXEL  /O2 %~3 %~1.asm tmp
    bin2c -n %~1%~2_f -o d3d8/%~1%~2_f.h tmp
    ENDLOCAL
EXIT /B 0

:compile_u
    SETLOCAL
    call :compile %~1
    call :compile %~1   _u      "/D UNDERWATER"
    ENDLOCAL
EXIT /B 0
