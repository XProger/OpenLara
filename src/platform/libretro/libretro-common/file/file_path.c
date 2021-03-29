/* Copyright  (C) 2010-2018 The RetroArch team
 *
 * ---------------------------------------------------------------------------------------
 * The following license statement only applies to this file (file_path.c).
 * ---------------------------------------------------------------------------------------
 *
 * Permission is hereby granted, free of charge,
 * to any person obtaining a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifdef __STDC_ALLOC_LIB__
#define __STDC_WANT_LIB_EXT2__ 1
#else
#define _POSIX_C_SOURCE 200809L
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include <sys/stat.h>

#include <boolean.h>
#include <file/file_path.h>
#include <retro_assert.h>
#include <string/stdstring.h>

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#endif
#ifdef __HAIKU__
#include <kernel/image.h>
#endif
#ifndef __MACH__
#include <compat/strl.h>
#include <compat/posix_string.h>
#endif
#include <compat/strcasestr.h>
#include <retro_miscellaneous.h>
#include <encodings/utf.h>

#if defined(_WIN32)
#ifdef _MSC_VER
#define setmode _setmode
#endif
#include <sys/stat.h>
#ifdef _XBOX
#include <xtl.h>
#define INVALID_FILE_ATTRIBUTES -1
#else
#include <io.h>
#include <fcntl.h>
#include <direct.h>
#include <windows.h>
#if defined(_MSC_VER) && _MSC_VER <= 1200
#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
#endif
#endif
#elif defined(VITA)
#define SCE_ERROR_ERRNO_EEXIST 0x80010011
#include <psp2/io/fcntl.h>
#include <psp2/io/dirent.h>
#include <psp2/io/stat.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#if defined(PSP)
#include <pspkernel.h>
#endif

#if defined(__CELLOS_LV2__)
#include <cell/cell_fs.h>
#endif

#if defined(VITA)
#define FIO_S_ISDIR SCE_S_ISDIR
#endif

#if (defined(__CELLOS_LV2__) && !defined(__PSL1GHT__)) || defined(__QNX__) || defined(PSP)
#include <unistd.h> /* stat() is defined here */
#endif

/* Assume W-functions do not work below Win2K and Xbox platforms */
#if defined(_WIN32_WINNT) && _WIN32_WINNT < 0x0500 || defined(_XBOX)

#ifndef LEGACY_WIN32
#define LEGACY_WIN32
#endif

#endif

enum stat_mode
{
   IS_DIRECTORY = 0,
   IS_CHARACTER_SPECIAL,
   IS_VALID
};

static bool path_stat(const char *path, enum stat_mode mode, int32_t *size)
{
#if defined(VITA) || defined(PSP)
   SceIoStat buf;
   char *tmp  = strdup(path);
   size_t len = strlen(tmp);
   if (tmp[len-1] == '/')
      tmp[len-1]='\0';

   if (sceIoGetstat(tmp, &buf) < 0)
   {
      free(tmp);
      return false;
   }
   free(tmp);

#elif defined(__CELLOS_LV2__)
    CellFsStat buf;
    if (cellFsStat(path, &buf) < 0)
       return false;
#elif defined(_WIN32)
   struct _stat buf;
   char *path_local;
   wchar_t *path_wide;
   DWORD file_info;

   if (!path || !*path)
      return false;

   (void)path_wide;
   (void)path_local;
   (void)file_info;

#if defined(LEGACY_WIN32)
   path_local = utf8_to_local_string_alloc(path);
   file_info  = GetFileAttributes(path_local);

   _stat(path_local, &buf);

   if (path_local)
     free(path_local);
#else
   path_wide = utf8_to_utf16_string_alloc(path);
   file_info = GetFileAttributesW(path_wide);

   _wstat(path_wide, &buf);

   if (path_wide)
      free(path_wide);
#endif

   if (file_info == INVALID_FILE_ATTRIBUTES)
      return false;
#else
   struct stat buf;
   if (stat(path, &buf) < 0)
      return false;
#endif

   if (size)
      *size = (int32_t)buf.st_size;

   switch (mode)
   {
      case IS_DIRECTORY:
#if defined(VITA) || defined(PSP)
         return FIO_S_ISDIR(buf.st_mode);
#elif defined(__CELLOS_LV2__)
         return ((buf.st_mode & S_IFMT) == S_IFDIR);
#elif defined(_WIN32)
         return (file_info & FILE_ATTRIBUTE_DIRECTORY);
#else
         return S_ISDIR(buf.st_mode);
#endif
      case IS_CHARACTER_SPECIAL:
#if defined(VITA) || defined(PSP) || defined(__CELLOS_LV2__) || defined(_WIN32)
         return false;
#else
         return S_ISCHR(buf.st_mode);
#endif
      case IS_VALID:
         return true;
   }

   return false;
}

/**
 * path_is_directory:
 * @path               : path
 *
 * Checks if path is a directory.
 *
 * Returns: true (1) if path is a directory, otherwise false (0).
 */
bool path_is_directory(const char *path)
{
   return path_stat(path, IS_DIRECTORY, NULL);
}

bool path_is_character_special(const char *path)
{
   return path_stat(path, IS_CHARACTER_SPECIAL, NULL);
}

bool path_is_valid(const char *path)
{
   return path_stat(path, IS_VALID, NULL);
}

int32_t path_get_size(const char *path)
{
   int32_t filesize = 0;
   if (path_stat(path, IS_VALID, &filesize))
      return filesize;

   return -1;
}

static bool path_mkdir_error(int ret)
{
#if defined(VITA)
   return (ret == SCE_ERROR_ERRNO_EEXIST);
#elif defined(PSP) || defined(_3DS) || defined(WIIU) || defined(SWITCH)
   return (ret == -1);
#else
   return (ret < 0 && errno == EEXIST);
#endif
}

/**
 * path_mkdir:
 * @dir                : directory
 *
 * Create directory on filesystem.
 *
 * Returns: true (1) if directory could be created, otherwise false (0).
 **/
bool path_mkdir(const char *dir)
{
   /* Use heap. Real chance of stack overflow if we recurse too hard. */
   const char *target = NULL;
   bool         sret  = false;
   bool norecurse     = false;
   char     *basedir  = NULL;

   if (dir && *dir)
      basedir         = strdup(dir);

   if (!basedir)
      return false;

   path_parent_dir(basedir);
   if (!*basedir || !strcmp(basedir, dir))
      goto end;

   if (path_is_directory(basedir))
   {
      target    = dir;
      norecurse = true;
   }
   else
   {
      target    = basedir;
      sret      = path_mkdir(basedir);

      if (sret)
      {
         target    = dir;
         norecurse = true;
      }
   }

   if (norecurse)
   {
#if defined(_WIN32)
#ifdef LEGACY_WIN32
      int ret = _mkdir(dir);
#else
      wchar_t *dirW = utf8_to_utf16_string_alloc(dir);
      int ret = -1;

      if (dirW)
      {
         ret = _wmkdir(dirW);
         free(dirW);
      }
#endif
#elif defined(IOS)
      int ret = mkdir(dir, 0755);
#elif defined(VITA) || defined(PSP)
      int ret = sceIoMkdir(dir, 0777);
#elif defined(__QNX__)
      int ret = mkdir(dir, 0777);
#else
      int ret = mkdir(dir, 0750);
#endif

      /* Don't treat this as an error. */
      if (path_mkdir_error(ret) && path_is_directory(dir))
         ret = 0;

      if (ret < 0)
         printf("mkdir(%s) error: %s.\n", dir, strerror(errno));
      sret = (ret == 0);
   }

end:
   if (target && !sret)
      printf("Failed to create directory: \"%s\".\n", target);
   free(basedir);
   return sret;
}

/**
 * path_get_archive_delim:
 * @path               : path
 *
 * Find delimiter of an archive file. Only the first '#'
 * after a compression extension is considered.
 *
 * Returns: pointer to the delimiter in the path if it contains
 * a path inside a compressed file, otherwise NULL.
 */
const char *path_get_archive_delim(const char *path)
{
   const char *last  = find_last_slash(path);
   const char *delim = NULL;

   if (last)
   {
      delim = strcasestr(last, ".zip#");

      if (!delim)
         delim = strcasestr(last, ".apk#");
   }

   if (delim)
      return delim + 4;

   if (last)
      delim = strcasestr(last, ".7z#");

   if (delim)
      return delim + 3;

   return NULL;
}

/**
 * path_get_extension:
 * @path               : path
 *
 * Gets extension of file. Only '.'s
 * after the last slash are considered.
 *
 * Returns: extension part from the path.
 */
const char *path_get_extension(const char *path)
{
   const char *ext = !string_is_empty(path) 
      ? strrchr(path_basename(path), '.') : NULL;
   if (!ext)
      return "";
   return ext + 1;
}

/**
 * path_remove_extension:
 * @path               : path
 *
 * Mutates path by removing its extension. Removes all
 * text after and including the last '.'.
 * Only '.'s after the last slash are considered.
 *
 * Returns: 
 * 1) If path has an extension, returns path with the
 *    extension removed.
 * 2) If there is no extension, returns NULL.
 * 3) If path is empty or NULL, returns NULL
 */
char *path_remove_extension(char *path)
{
   char *last = !string_is_empty(path) 
      ? (char*)strrchr(path_basename(path), '.') : NULL;
   if (!last)
      return NULL;
   if (*last)
      *last = '\0';
   return path;
}

/**
 * path_is_compressed_file:
 * @path               : path
 *
 * Checks if path is a compressed file.
 *
 * Returns: true (1) if path is a compressed file, otherwise false (0).
 **/
bool path_is_compressed_file(const char* path)
{
   const char *ext = path_get_extension(path);

   if (     strcasestr(ext, "zip")
         || strcasestr(ext, "apk")
         || strcasestr(ext, "7z"))
      return true;

   return false;
}

/**
 * fill_pathname:
 * @out_path           : output path
 * @in_path            : input  path
 * @replace            : what to replace
 * @size               : buffer size of output path
 *
 * FIXME: Verify
 *
 * Replaces filename extension with 'replace' and outputs result to out_path.
 * The extension here is considered to be the string from the last '.'
 * to the end.
 *
 * Only '.'s after the last slash are considered as extensions.
 * If no '.' is present, in_path and replace will simply be concatenated.
 * 'size' is buffer size of 'out_path'.
 * E.g.: in_path = "/foo/bar/baz/boo.c", replace = ".asm" =>
 * out_path = "/foo/bar/baz/boo.asm"
 * E.g.: in_path = "/foo/bar/baz/boo.c", replace = ""     =>
 * out_path = "/foo/bar/baz/boo"
 */
void fill_pathname(char *out_path, const char *in_path,
      const char *replace, size_t size)
{
   char tmp_path[PATH_MAX_LENGTH];
   char *tok                      = NULL;

   tmp_path[0] = '\0';

   strlcpy(tmp_path, in_path, sizeof(tmp_path));
   if ((tok = (char*)strrchr(path_basename(tmp_path), '.')))
      *tok = '\0';

   fill_pathname_noext(out_path, tmp_path, replace, size);
}

/**
 * fill_pathname_noext:
 * @out_path           : output path
 * @in_path            : input  path
 * @replace            : what to replace
 * @size               : buffer size of output path
 *
 * Appends a filename extension 'replace' to 'in_path', and outputs
 * result in 'out_path'.
 *
 * Assumes in_path has no extension. If an extension is still
 * present in 'in_path', it will be ignored.
 *
 */
void fill_pathname_noext(char *out_path, const char *in_path,
      const char *replace, size_t size)
{
   strlcpy(out_path, in_path, size);
   strlcat(out_path, replace, size);
}

char *find_last_slash(const char *str)
{
   const char *slash     = strrchr(str, '/');
#ifdef _WIN32
   const char *backslash = strrchr(str, '\\');

   if (backslash && ((slash && backslash > slash) || !slash))
      slash = backslash;
#endif

   return (char*)slash;
}

/**
 * fill_pathname_slash:
 * @path               : path
 * @size               : size of path
 *
 * Assumes path is a directory. Appends a slash
 * if not already there.
 **/
void fill_pathname_slash(char *path, size_t size)
{
   size_t        path_len = strlen(path);
   const char *last_slash = find_last_slash(path);

   /* Try to preserve slash type. */
   if (last_slash && (last_slash != (path + path_len - 1)))
   {
      char join_str[2];

      join_str[0] = '\0';

      strlcpy(join_str, last_slash, sizeof(join_str));
      strlcat(path, join_str, size);
   }
   else if (!last_slash)
      strlcat(path, path_default_slash(), size);
}

/**
 * fill_pathname_dir:
 * @in_dir             : input directory path
 * @in_basename        : input basename to be appended to @in_dir
 * @replace            : replacement to be appended to @in_basename
 * @size               : size of buffer
 *
 * Appends basename of 'in_basename', to 'in_dir', along with 'replace'.
 * Basename of in_basename is the string after the last '/' or '\\',
 * i.e the filename without directories.
 *
 * If in_basename has no '/' or '\\', the whole 'in_basename' will be used.
 * 'size' is buffer size of 'in_dir'.
 *
 * E.g..: in_dir = "/tmp/some_dir", in_basename = "/some_content/foo.c",
 * replace = ".asm" => in_dir = "/tmp/some_dir/foo.c.asm"
 **/
void fill_pathname_dir(char *in_dir, const char *in_basename,
      const char *replace, size_t size)
{
   const char *base = NULL;

   fill_pathname_slash(in_dir, size);
   base = path_basename(in_basename);
   strlcat(in_dir, base, size);
   strlcat(in_dir, replace, size);
}

/**
 * fill_pathname_base:
 * @out                : output path
 * @in_path            : input path
 * @size               : size of output path
 *
 * Copies basename of @in_path into @out_path.
 **/
void fill_pathname_base(char *out, const char *in_path, size_t size)
{
   const char     *ptr = path_basename(in_path);

   if (!ptr)
      ptr = in_path;

   strlcpy(out, ptr, size);
}

void fill_pathname_base_noext(char *out, const char *in_path, size_t size)
{
   fill_pathname_base(out, in_path, size);
   path_remove_extension(out);
}

void fill_pathname_base_ext(char *out, const char *in_path, const char *ext,
      size_t size)
{
   fill_pathname_base_noext(out, in_path, size);
   strlcat(out, ext, size);
}

/**
 * fill_pathname_basedir:
 * @out_dir            : output directory
 * @in_path            : input path
 * @size               : size of output directory
 *
 * Copies base directory of @in_path into @out_path.
 * If in_path is a path without any slashes (relative current directory),
 * @out_path will get path "./".
 **/
void fill_pathname_basedir(char *out_dir,
      const char *in_path, size_t size)
{
   if (out_dir != in_path)
      strlcpy(out_dir, in_path, size);
   path_basedir(out_dir);
}

void fill_pathname_basedir_noext(char *out_dir,
      const char *in_path, size_t size)
{
   fill_pathname_basedir(out_dir, in_path, size);
   path_remove_extension(out_dir);
}

/**
 * fill_pathname_parent_dir_name:
 * @out_dir            : output directory
 * @in_dir             : input directory
 * @size               : size of output directory
 *
 * Copies only the parent directory name of @in_dir into @out_dir.
 * The two buffers must not overlap. Removes trailing '/'.
 * Returns true on success, false if a slash was not found in the path.
 **/
bool fill_pathname_parent_dir_name(char *out_dir,
      const char *in_dir, size_t size)
{
   char *temp = strdup(in_dir);
   char *last = find_last_slash(temp);
   bool ret = false;

   *last = '\0';

   in_dir = find_last_slash(temp);

   if (in_dir && in_dir + 1)
   {
      strlcpy(out_dir, in_dir + 1, size);
      ret = true;
   }
   else
      ret = false;

   free(temp);

   return ret;
}

/**
 * fill_pathname_parent_dir:
 * @out_dir            : output directory
 * @in_dir             : input directory
 * @size               : size of output directory
 *
 * Copies parent directory of @in_dir into @out_dir.
 * Assumes @in_dir is a directory. Keeps trailing '/'.
 **/
void fill_pathname_parent_dir(char *out_dir,
      const char *in_dir, size_t size)
{
   if (out_dir != in_dir)
      strlcpy(out_dir, in_dir, size);
   path_parent_dir(out_dir);
}

/**
 * fill_dated_filename:
 * @out_filename       : output filename
 * @ext                : extension of output filename
 * @size               : buffer size of output filename
 *
 * Creates a 'dated' filename prefixed by 'RetroArch', and
 * concatenates extension (@ext) to it.
 *
 * E.g.:
 * out_filename = "RetroArch-{month}{day}-{Hours}{Minutes}.{@ext}"
 **/
void fill_dated_filename(char *out_filename,
      const char *ext, size_t size)
{
   time_t cur_time = time(NULL);

   strftime(out_filename, size,
         "RetroArch-%m%d-%H%M%S.", localtime(&cur_time));
   strlcat(out_filename, ext, size);
}

/**
 * fill_str_dated_filename:
 * @out_filename       : output filename
 * @in_str             : input string
 * @ext                : extension of output filename
 * @size               : buffer size of output filename
 *
 * Creates a 'dated' filename prefixed by the string @in_str, and
 * concatenates extension (@ext) to it.
 *
 * E.g.:
 * out_filename = "RetroArch-{year}{month}{day}-{Hour}{Minute}{Second}.{@ext}"
 **/
void fill_str_dated_filename(char *out_filename,
      const char *in_str, const char *ext, size_t size)
{
   char format[256];
   time_t cur_time = time(NULL);

   format[0] = '\0';

   strftime(format, sizeof(format), "-%y%m%d-%H%M%S.", localtime(&cur_time));

   fill_pathname_join_concat_noext(out_filename,
         in_str, format, ext,
         size);
}

/**
 * path_basedir:
 * @path               : path
 *
 * Extracts base directory by mutating path.
 * Keeps trailing '/'.
 **/
void path_basedir(char *path)
{
   char *last = NULL;
   if (strlen(path) < 2)
      return;

   last = find_last_slash(path);

   if (last)
      last[1] = '\0';
   else
      snprintf(path, 3, ".%s", path_default_slash());
}

/**
 * path_parent_dir:
 * @path               : path
 *
 * Extracts parent directory by mutating path.
 * Assumes that path is a directory. Keeps trailing '/'.
 **/
void path_parent_dir(char *path)
{
   size_t len = strlen(path);
   if (len && path_char_is_slash(path[len - 1]))
      path[len - 1] = '\0';
   path_basedir(path);
}

/**
 * path_basename:
 * @path               : path
 *
 * Get basename from @path.
 *
 * Returns: basename from path.
 **/
const char *path_basename(const char *path)
{
   /* We cut either at the first compression-related hash
    * or the last slash; whichever comes last */
   const char *last  = find_last_slash(path);
   const char *delim = path_get_archive_delim(path);

   if (delim)
      return delim + 1;

   if (last)
      return last + 1;

   return path;
}

/**
 * path_is_absolute:
 * @path               : path
 *
 * Checks if @path is an absolute path or a relative path.
 *
 * Returns: true if path is absolute, false if path is relative.
 **/
bool path_is_absolute(const char *path)
{
   if (path[0] == '/')
      return true;
#ifdef _WIN32
   /* Many roads lead to Rome ... */
   if ((    strstr(path, "\\\\") == path)
         || strstr(path, ":/")
         || strstr(path, ":\\")
         || strstr(path, ":\\\\"))
      return true;
#elif defined(__wiiu__)
   if (strstr(path, ":/"))
      return true;
#endif
   return false;
}

/**
 * path_resolve_realpath:
 * @buf                : buffer for path
 * @size               : size of buffer
 *
 * Turns relative paths into absolute path.
 * If relative, rebases on current working dir.
 **/
void path_resolve_realpath(char *buf, size_t size)
{
#if !defined(RARCH_CONSOLE) && defined(RARCH_INTERNAL)
   char tmp[PATH_MAX_LENGTH];

   tmp[0] = '\0';

   strlcpy(tmp, buf, sizeof(tmp));

#ifdef _WIN32
   if (!_fullpath(buf, tmp, size))
      strlcpy(buf, tmp, size);
#else

   /* NOTE: realpath() expects at least PATH_MAX_LENGTH bytes in buf.
    * Technically, PATH_MAX_LENGTH needn't be defined, but we rely on it anyways.
    * POSIX 2008 can automatically allocate for you,
    * but don't rely on that. */
   if (!realpath(tmp, buf))
      strlcpy(buf, tmp, size);
#endif
#endif
}

/**
 * fill_pathname_resolve_relative:
 * @out_path           : output path
 * @in_refpath         : input reference path
 * @in_path            : input path
 * @size               : size of @out_path
 *
 * Joins basedir of @in_refpath together with @in_path.
 * If @in_path is an absolute path, out_path = in_path.
 * E.g.: in_refpath = "/foo/bar/baz.a", in_path = "foobar.cg",
 * out_path = "/foo/bar/foobar.cg".
 **/
void fill_pathname_resolve_relative(char *out_path,
      const char *in_refpath, const char *in_path, size_t size)
{
   if (path_is_absolute(in_path))
   {
      strlcpy(out_path, in_path, size);
      return;
   }

   fill_pathname_basedir(out_path, in_refpath, size);
   strlcat(out_path, in_path, size);
}

/**
 * fill_pathname_join:
 * @out_path           : output path
 * @dir                : directory
 * @path               : path
 * @size               : size of output path
 *
 * Joins a directory (@dir) and path (@path) together.
 * Makes sure not to get  two consecutive slashes
 * between directory and path.
 **/
void fill_pathname_join(char *out_path,
      const char *dir, const char *path, size_t size)
{
   if (out_path != dir)
      strlcpy(out_path, dir, size);

   if (*out_path)
      fill_pathname_slash(out_path, size);

   strlcat(out_path, path, size);
}

void fill_pathname_join_special_ext(char *out_path,
      const char *dir,  const char *path,
      const char *last, const char *ext,
      size_t size)
{
   fill_pathname_join(out_path, dir, path, size);
   if (*out_path)
      fill_pathname_slash(out_path, size);

   strlcat(out_path, last, size);
   strlcat(out_path, ext, size);
}

void fill_pathname_join_concat_noext(
      char *out_path,
      const char *dir, const char *path,
      const char *concat,
      size_t size)
{
   fill_pathname_noext(out_path, dir, path, size);
   strlcat(out_path, concat, size);
}

void fill_pathname_join_concat(char *out_path,
      const char *dir, const char *path,
      const char *concat,
      size_t size)
{
   fill_pathname_join(out_path, dir, path, size);
   strlcat(out_path, concat, size);
}

void fill_pathname_join_noext(char *out_path,
      const char *dir, const char *path, size_t size)
{
   fill_pathname_join(out_path, dir, path, size);
   path_remove_extension(out_path);
}


/**
 * fill_pathname_join_delim:
 * @out_path           : output path
 * @dir                : directory
 * @path               : path
 * @delim              : delimiter
 * @size               : size of output path
 *
 * Joins a directory (@dir) and path (@path) together
 * using the given delimiter (@delim).
 **/
void fill_pathname_join_delim(char *out_path, const char *dir,
      const char *path, const char delim, size_t size)
{
   size_t copied;
   /* behavior of strlcpy is undefined if dst and src overlap */
   if (out_path == dir)
      copied = strlen(dir);
   else
      copied = strlcpy(out_path, dir, size);

   out_path[copied]   = delim;
   out_path[copied+1] = '\0';

   strlcat(out_path, path, size);
}

void fill_pathname_join_delim_concat(char *out_path, const char *dir,
      const char *path, const char delim, const char *concat,
      size_t size)
{
   fill_pathname_join_delim(out_path, dir, path, delim, size);
   strlcat(out_path, concat, size);
}

/**
 * fill_short_pathname_representation:
 * @out_rep            : output representation
 * @in_path            : input path
 * @size               : size of output representation
 *
 * Generates a short representation of path. It should only
 * be used for displaying the result; the output representation is not
 * binding in any meaningful way (for a normal path, this is the same as basename)
 * In case of more complex URLs, this should cut everything except for
 * the main image file.
 *
 * E.g.: "/path/to/game.img" -> game.img
 *       "/path/to/myarchive.7z#folder/to/game.img" -> game.img
 */
void fill_short_pathname_representation(char* out_rep,
      const char *in_path, size_t size)
{
   char path_short[PATH_MAX_LENGTH];

   path_short[0] = '\0';

   fill_pathname(path_short, path_basename(in_path), "",
            sizeof(path_short));

   strlcpy(out_rep, path_short, size);
}

void fill_short_pathname_representation_noext(char* out_rep,
      const char *in_path, size_t size)
{
   fill_short_pathname_representation(out_rep, in_path, size);
   path_remove_extension(out_rep);
}

void fill_pathname_expand_special(char *out_path,
      const char *in_path, size_t size)
{
#if !defined(RARCH_CONSOLE) && defined(RARCH_INTERNAL)
   if (*in_path == '~')
   {
      const char *home = getenv("HOME");
      if (home)
      {
         size_t src_size = strlcpy(out_path, home, size);
         retro_assert(src_size < size);

         out_path  += src_size;
         size      -= src_size;
         in_path++;
      }
   }
   else if ((in_path[0] == ':') &&
         (
         (in_path[1] == '/')
#ifdef _WIN32
         || (in_path[1] == '\\')
#endif
         )
            )
   {
      size_t src_size;
      char *application_dir = (char*)malloc(PATH_MAX_LENGTH * sizeof(char));

      application_dir[0]    = '\0';

      fill_pathname_application_path(application_dir,
            PATH_MAX_LENGTH * sizeof(char));
      path_basedir_wrapper(application_dir);

      src_size   = strlcpy(out_path, application_dir, size);
      retro_assert(src_size < size);

      free(application_dir);

      out_path  += src_size;
      size      -= src_size;
      in_path   += 2;
   }
#endif

   retro_assert(strlcpy(out_path, in_path, size) < size);
}

void fill_pathname_abbreviate_special(char *out_path,
      const char *in_path, size_t size)
{
#if !defined(RARCH_CONSOLE) && defined(RARCH_INTERNAL)
   unsigned i;
   const char *candidates[3];
   const char *notations[3];
   char *application_dir     = (char*)malloc(PATH_MAX_LENGTH * sizeof(char));
   const char *home          = getenv("HOME");

   application_dir[0] = '\0';

   /* application_dir could be zero-string. Safeguard against this.
    *
    * Keep application dir in front of home, moving app dir to a
    * new location inside home would break otherwise. */

   /* ugly hack - use application_dir pointer
    * before filling it in. C89 reasons */
   candidates[0] = application_dir;
   candidates[1] = home;
   candidates[2] = NULL;

   notations [0] = ":";
   notations [1] = "~";
   notations [2] = NULL;

   fill_pathname_application_path(application_dir,
         PATH_MAX_LENGTH * sizeof(char));
   path_basedir_wrapper(application_dir);

   for (i = 0; candidates[i]; i++)
   {
      if (!string_is_empty(candidates[i]) &&
            strstr(in_path, candidates[i]) == in_path)
      {
         size_t src_size  = strlcpy(out_path, notations[i], size);

         retro_assert(src_size < size);

         out_path        += src_size;
         size            -= src_size;
         in_path         += strlen(candidates[i]);

         if (!path_char_is_slash(*in_path))
         {
            retro_assert(strlcpy(out_path,
                     path_default_slash(), size) < size);
            out_path++;
            size--;
         }

         break; /* Don't allow more abbrevs to take place. */
      }
   }

   free(application_dir);
#endif

   retro_assert(strlcpy(out_path, in_path, size) < size);
}

/**
 * path_basedir:
 * @path               : path
 *
 * Extracts base directory by mutating path.
 * Keeps trailing '/'.
 **/
void path_basedir_wrapper(char *path)
{
   char *last = NULL;
   if (strlen(path) < 2)
      return;

#ifdef HAVE_COMPRESSION
   /* We want to find the directory with the archive in basedir. */
   last = (char*)path_get_archive_delim(path);
   if (last)
      *last = '\0';
#endif

   last = find_last_slash(path);

   if (last)
      last[1] = '\0';
   else
      snprintf(path, 3, ".%s", path_default_slash());
}

#if !defined(RARCH_CONSOLE) && defined(RARCH_INTERNAL)
void fill_pathname_application_path(char *s, size_t len)
{
   size_t i;
#ifdef __APPLE__
  CFBundleRef bundle = CFBundleGetMainBundle();
#endif
#ifdef _WIN32
   DWORD ret;
   wchar_t wstr[PATH_MAX_LENGTH] = {0};
#endif
#ifdef __HAIKU__
   image_info info;
   int32_t cookie = 0;
#endif
   (void)i;

   if (!len)
      return;

#ifdef _WIN32
#ifdef LEGACY_WIN32
   ret    = GetModuleFileNameA(GetModuleHandle(NULL), s, len);
#else
   ret    = GetModuleFileNameW(GetModuleHandle(NULL), wstr, ARRAY_SIZE(wstr));

   if (*wstr)
   {
      char *str = utf16_to_utf8_string_alloc(wstr);

      if (str)
      {
         strlcpy(s, str, len);
         free(str);
      }
   }
#endif
   s[ret] = '\0';
#elif defined(__APPLE__)
   if (bundle)
   {
      CFURLRef bundle_url = CFBundleCopyBundleURL(bundle);
      CFStringRef bundle_path = CFURLCopyPath(bundle_url);
      CFStringGetCString(bundle_path, s, len, kCFStringEncodingUTF8);
      CFRelease(bundle_path);
      CFRelease(bundle_url);

      retro_assert(strlcat(s, "nobin", len) < len);
      return;
   }
#elif defined(__HAIKU__)
   while (get_next_image_info(0, &cookie, &info) == B_OK)
   {
      if (info.type == B_APP_IMAGE)
      {
         strlcpy(s, info.name, len);
         return;
      }
   }
#elif defined(__QNX__)
   char *buff = malloc(len);

   if(_cmdname(buff))
      strlcpy(s, buff, len);

   free(buff);
#else
   {
      pid_t pid;
      static const char *exts[] = { "exe", "file", "path/a.out" };
      char link_path[255];

      link_path[0] = *s = '\0';
      pid       = getpid();

      /* Linux, BSD and Solaris paths. Not standardized. */
      for (i = 0; i < ARRAY_SIZE(exts); i++)
      {
         ssize_t ret;

         snprintf(link_path, sizeof(link_path), "/proc/%u/%s",
               (unsigned)pid, exts[i]);
         ret = readlink(link_path, s, len - 1);

         if (ret >= 0)
         {
            s[ret] = '\0';
            return;
         }
      }
   }
#endif
}
#endif
