#ifdef HAVE_CONFIG_H
# include "config.h"
#endif /* HAVE_CONFIG_H */

#include <sys/types.h>
#include <direct.h>

#include "evil_private.h"

#undef rename

EVIL_API int
evil_rename(const char *src, const char* dst)
{
   DWORD res;

   /*
    * Check if the destination path exists and is a directory.
    * Windows' MoveFileEx fails if dst is an existing directory,
    * unlike POSIX rename which can replace an empty directory.
    */
   res = GetFileAttributes(dst);
   if ((res != 0xffffffff) && (res & FILE_ATTRIBUTE_DIRECTORY))
     {
        /* If dst is an existing directory, attempt to remove it. */
        if (!RemoveDirectory(dst))
          return -1; /* Failed to remove the directory. */
     }

   /*
    * MoveFileEx with MOVEFILE_REPLACE_EXISTING will overwrite an
    * existing file at dst, or move src to dst if dst does not exist.
    * Returns 0 on success, -1 on failure.
    */
   return MoveFileEx(src, dst, MOVEFILE_REPLACE_EXISTING) ? 0 : -1;
}

EVIL_API int
evil_mkdir(const char *dirname, mode_t mode EVIL_UNUSED)
{
   /*
    * This is a simple wrapper for the Windows _mkdir function.
    * The 'mode' parameter is ignored on Windows, similar to how
    * _mkdir behaves. It's included for API compatibility with
    * POSIX mkdir.
    */
   return _mkdir(dirname);
}

