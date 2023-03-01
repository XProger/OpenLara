set IN_FOLDER=orig\
set OUT_FOLDER=orig_mono\

for /r %IN_FOLDER% %%i in (*.ogg) do (
	ffmpeg -y -i %%i temp.wav
	poly2mono temp.wav %OUT_FOLDER%%%~ni.wav -window:sine -blocksize:8192 -nhops:2
)
del temp.wav
