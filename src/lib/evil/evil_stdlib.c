#ifdef HAVE_CONFIG_H
# include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <stdio.h>
#include <io.h>
#include <direct.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <errno.h>

#include "evil_private.h"

/*
 * Environment variable related functions
 *
 * int setenv (const char *name, const char *value, int overwrite);
 * void unsetenv (const char *name);
 *
 */

/**
 * @brief Sets or modifies an environment variable.
 * @param name The name of the environment variable. Must not be NULL, empty, or contain '='.
 * @param value The value to set for the environment variable. If NULL, the variable is effectively unset (see unsetenv).
 * @param overwrite If non-zero, an existing variable with the same name will be overwritten.
 *                  If zero and the variable exists, its value is not changed.
 * @return 0 on success, -1 on error. Sets errno on failure.
 *         Possible errno values:
 *         - EINVAL: If @p name is NULL, empty, or contains '='.
 *         - ENOMEM: If memory allocation fails.
 *
 * This function adds a new environment variable or modifies an existing one.
 * It constructs a string of the form "name=value" and passes it to _putenv.
 * If @p value is NULL, it constructs "name=".
 */
EVIL_API int
setenv(const char *name,
       const char *value,
       int         overwrite)
{
   char  *old_name;
   char  *str;
   size_t length;
   int    res;

   if (!name || !*name)
     return -1;

   /* if '=' is found, return EINVAL */
   if (strchr (name, '='))
     {
        errno = EINVAL;
        return -1;
     }

   /* if name is already set and overwrite is 0, we exit with success */
   old_name = getenv(name);
   if (!overwrite && old_name)
     return 0;

   length = value ? strlen(value) : 0;
   length += strlen(name) + 2;
   str = (char *)malloc(length);
   if (!str)
     {
        errno = ENOMEM;
        return -1;
     }
   if (!value)
     sprintf(str, "%s=", name);
   else
     sprintf(str, "%s=%s", name, value);
   res = _putenv(str);
   free(str);

   return res;
}

/**
 * @brief Removes an environment variable.
 * @param name The name of the environment variable to remove.
 * @return 0 on success, -1 on error. Sets errno on failure.
 *
 * This function removes an environment variable by calling setenv with
 * a NULL value and overwrite set to 1.
 */
EVIL_API int
unsetenv(const char *name)
{
   return setenv(name, NULL, 1);
}


/*
 * Files related functions
 *
 */

/**
 * @brief Resolves a relative or absolute path to a canonicalized absolute pathname.
 * @param file_name The path to resolve.
 * @param resolved_name A buffer to store the resolved path. If NULL, a buffer
 *                      of size _MAX_PATH is allocated using malloc(). The caller
 *                      is responsible for freeing this buffer if it was allocated.
 * @return A pointer to the resolved path in @p resolved_name (or the allocated buffer)
 *         on success, or NULL on failure. Sets errno on failure.
 *         Possible errno values:
 *         - EINVAL: If @p file_name is NULL.
 *         - ENOMEM: If memory allocation fails (when @p resolved_name is NULL).
 *         - ENAMETOOLONG: If the resolved path exceeds _MAX_PATH.
 *         - EACCES: If @p file_name cannot be accessed (checked via access(file_name, 4)).
 *         - Other errno values may be set by access() or _fullpath().
 *
 * This function attempts to resolve @p file_name to an absolute path.
 * It first checks if @p file_name is accessible. If @p resolved_name is NULL,
 * it allocates memory. Then, it uses _fullpath to perform the resolution.
 */
EVIL_API char *
realpath(const char *file_name, char *resolved_name)
{
   char *retname = NULL;  /* we will return this, if we fail */

   /* SUSv3 says we must set `errno = EINVAL', and return NULL,
    * if `name' is passed as a NULL pointer.
    */

   if (file_name == NULL)
     errno = EINVAL;

   /* Otherwise, `name' must refer to a readable filesystem object,
    * if we are going to resolve its absolute path name.
    */

   else if (access(file_name, 4) == 0)
     {
        /* If `name' didn't point to an existing entity,
         * then we don't get to here; we simply fall past this block,
         * returning NULL, with `errno' appropriately set by `access'.
         *
         * When we _do_ get to here, then we can use `_fullpath' to
         * resolve the full path for `name' into `resolved', but first,
         * check that we have a suitable buffer, in which to return it.
         */

       if ((retname = resolved_name) == NULL)
         {
            /* Caller didn't give us a buffer, so we'll exercise the
             * option granted by SUSv3, and allocate one.
             *
             * `_fullpath' would do this for us, but it uses `malloc', and
             * Microsoft's implementation doesn't set `errno' on failure.
             * If we don't do this explicitly ourselves, then we will not
             * know if `_fullpath' fails on `malloc' failure, or for some
             * other reason, and we want to set `errno = ENOMEM' for the
             * `malloc' failure case.
             */

           retname = malloc(_MAX_PATH);
         }

       /* By now, we should have a valid buffer.
        * If we don't, then we know that `malloc' failed,
        * so we can set `errno = ENOMEM' appropriately.
        */

       if (retname == NULL)
         errno = ENOMEM;

       /* Otherwise, when we do have a valid buffer,
        * `_fullpath' should only fail if the path name is too long.
        */

       else if ((retname = _fullpath(retname, file_name, _MAX_PATH)) == NULL)
         errno = ENAMETOOLONG;
     }

   /* By the time we get to here,
    * `retname' either points to the required resolved path name,
    * or it is NULL, with `errno' set appropriately, either of which
    * is our required return condition.
    */

   return retname;
}
