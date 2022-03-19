# OpenLara Asset Layout

## How to Set Up TR Assets for OpenLara

If you have just downloaded OpenLara and want to know how to get started with it, this
document will show how to use the assets from your Tomb Raider CD or digital purchase.

These instructions should apply for almost every version of OpenLara.

**NOTE:** You can use assets from the original PC or PS1 versions. The original PC version
can purchased at [GOG.com](https://www.gog.com/game/tomb_raider_123).

**NOTE:** Don't ask the OpenLara project for the game assets. You should be able to supply
these yourself. We don't care how you get the game, but don't ask us for the assets. The
answer will always be no.

## Folder Layout

Once you have extracted the OpenLara binaries into a folder on your drive, create three
additional subfolders within it: 'audio', 'level', and 'video'. Within each of these
subfolders, create an additional folder simply named '1'. Now you're ready to copy
the required data files.

### Using the PC Version's Assets

On the original CD are two folders titled 'DATA' and 'FMV'. Copy all of the files in 'DATA'
into the level/1 folder you created earlier, and all of the files in 'FMV' to the video/1 folder.

**NOTE:** If you bought the game off GOG.com, you may have an .iso file in the bundle, but
extraction instructions will be similar.

Please note that if you're using macOS or Linux, there may be issues with OpenLara complaining
that it can't find the files needed. This is merely an issue with case sensitivity; the file
systems used by macOS and Linux treat files with different cases differently (i.e. file.txt
and FILE.TXT are unique files and not the same). This can be remedied by renaming all the data
files to use ALL CAPS names (i.e. LEVEL1.PHD, LEVEL2.PHD, etc.) Be sure to do this for both the
level assets and the video files if needed.

Additionally, there is an audio pack that can be downloaded to add the CD Audio tracks from the
PS1 and Sega Saturn versions of Tomb Raider on the [OpenTomb Soundtrack](https://opentomb.earvillage.net/)
page, which also includes high resolution loading screens. Simply extract all of the .ogg files
into the audio/1 folder you created earlier, and copy the .png files in the archive into level/1, keeping
the case sensitivity issue in mind (i.e. rename the files to ALL CAPS if necessary).

### Using the PS1 Version's Assets

OpenLara can also use the data files from the Sony PlayStation version of Tomb Raider much in the same
way.

Create the three subfolders in your OpenLara folder as described above, with the '1' folders in each.

**NOTE:** There are two ways to extract the data files off the PS1 disc: straight copying, or using a tool
like [jPSXdec](https://github.com/m35/jpsxdec). If you use the latter method, extract the files using
the "Normal/2048 bytes" setting to prevent errors.

From the PS1 disc, copy everything in the 'psxdata' folder into the level/1 folder you created, along with
all the files in the 'deldata' folder. Then copy all of the files in the 'fmv' folder into the video/1 folder
you created. Keep the caps sensitivity issue in mind as described above if you're on macOS or Linux.

For the CD Audio tracks, they can be ripped as Ogg Vorbis files and copied into the audio/1 folder you created.
The filenames for the tracks can follow simple numeration (002.ogg, 003.ogg, etc.)

If you don't feel like ripping the CD Audio tracks, the audio pack mentioned in the previous section
has all the tracks already named and ready for use. Simply extract the files into the audio/1 folder.

## Playing OpenLara

If you followed along with this guide, you should be able to launch OpenLara and be greeted by the opening
FMVs. Give yourself a pat on the back and enjoy; you can now play Tomb Raider on your favorite platform!