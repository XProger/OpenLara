@echo off

set IN_FOLDER=orig_mono\
set OUT_FOLDER=conv\
set DEMO_FOLDER=conv_demo\

for /r %IN_FOLDER% %%i in (*.wav) do (
	ffmpeg -v panic -y -i %%i -ar 10512 -f s16le -acodec pcm_s16le temp.raw
	ad4 temp.raw %OUT_FOLDER%%%~ni.ad4 -0.5
)
del /f temp.raw

xcopy /Y %OUT_FOLDER%track_03.ad4 %DEMO_FOLDER%track_03.ad4*
xcopy /Y %OUT_FOLDER%track_04.ad4 %DEMO_FOLDER%track_04.ad4*
xcopy /Y %OUT_FOLDER%track_08.ad4 %DEMO_FOLDER%track_08.ad4*
xcopy /Y %OUT_FOLDER%track_09.ad4 %DEMO_FOLDER%track_09.ad4*
xcopy /Y %OUT_FOLDER%track_11.ad4 %DEMO_FOLDER%track_11.ad4*
xcopy /Y %OUT_FOLDER%track_12.ad4 %DEMO_FOLDER%track_12.ad4*
xcopy /Y %OUT_FOLDER%track_13.ad4 %DEMO_FOLDER%track_13.ad4*
xcopy /Y %OUT_FOLDER%track_16.ad4 %DEMO_FOLDER%track_16.ad4*
xcopy /Y %OUT_FOLDER%track_26_EN.ad4 %DEMO_FOLDER%track_26_EN.ad4*
xcopy /Y %OUT_FOLDER%track_27_EN.ad4 %DEMO_FOLDER%track_27_EN.ad4*
xcopy /Y %OUT_FOLDER%track_28_EN.ad4 %DEMO_FOLDER%track_28_EN.ad4*
xcopy /Y %OUT_FOLDER%track_29_EN.ad4 %DEMO_FOLDER%track_29_EN.ad4*
xcopy /Y %OUT_FOLDER%track_30_EN.ad4 %DEMO_FOLDER%track_30_EN.ad4*
xcopy /Y %OUT_FOLDER%track_31_EN.ad4 %DEMO_FOLDER%track_31_EN.ad4*
xcopy /Y %OUT_FOLDER%track_32_EN.ad4 %DEMO_FOLDER%track_32_EN.ad4*
xcopy /Y %OUT_FOLDER%track_33_EN.ad4 %DEMO_FOLDER%track_33_EN.ad4*
xcopy /Y %OUT_FOLDER%track_34_EN.ad4 %DEMO_FOLDER%track_34_EN.ad4*
xcopy /Y %OUT_FOLDER%track_35_EN.ad4 %DEMO_FOLDER%track_35_EN.ad4*
xcopy /Y %OUT_FOLDER%track_36_EN.ad4 %DEMO_FOLDER%track_36_EN.ad4*
xcopy /Y %OUT_FOLDER%track_37_EN.ad4 %DEMO_FOLDER%track_37_EN.ad4*
xcopy /Y %OUT_FOLDER%track_38_EN.ad4 %DEMO_FOLDER%track_38_EN.ad4*
xcopy /Y %OUT_FOLDER%track_39_EN.ad4 %DEMO_FOLDER%track_39_EN.ad4*
xcopy /Y %OUT_FOLDER%track_40_EN.ad4 %DEMO_FOLDER%track_40_EN.ad4*
xcopy /Y %OUT_FOLDER%track_41_EN.ad4 %DEMO_FOLDER%track_41_EN.ad4*
xcopy /Y %OUT_FOLDER%track_42_EN.ad4 %DEMO_FOLDER%track_42_EN.ad4*
xcopy /Y %OUT_FOLDER%track_43_EN.ad4 %DEMO_FOLDER%track_43_EN.ad4*
xcopy /Y %OUT_FOLDER%track_45_EN.ad4 %DEMO_FOLDER%track_45_EN.ad4*
xcopy /Y %OUT_FOLDER%track_47_EN.ad4 %DEMO_FOLDER%track_47_EN.ad4*
xcopy /Y %OUT_FOLDER%track_48_EN.ad4 %DEMO_FOLDER%track_48_EN.ad4*
xcopy /Y %OUT_FOLDER%track_49_EN.ad4 %DEMO_FOLDER%track_49_EN.ad4*
xcopy /Y %OUT_FOLDER%track_50_EN.ad4 %DEMO_FOLDER%track_50_EN.ad4*

pause