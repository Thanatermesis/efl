#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* ifdef HAVE_CONFIG_H */

#include "ecore_x_private.h"

/**
 * @internal
 * @brief Flag to indicate if the X resource system has been initialized.
 */
static Eina_Bool _ecore_x_resource_initted = EINA_FALSE;

/**
 * @internal
 * @brief The X resource database.
 */
static XrmDatabase _ecore_x_resource_db = NULL;

/**
 * @internal
 * @brief Initializes the X resource system if it hasn't been already.
 * This function calls XrmInitialize().
 */
static void
_ecore_x_resource_init(void)
{
   if (_ecore_x_resource_initted) return;
   XrmInitialize();
   _ecore_x_resource_initted = EINA_TRUE;
}

/**
 * @internal
 * @brief Shuts down the X resource system.
 * This function currently only nullifies the internal database reference
 * if it exists and resets the initialization flag. It does not explicitly
 * free X resources, relying on Xlib's behavior or other parts of the
 * application to manage that.
 */
void
_ecore_x_resource_shutdown(void)
{
   if (!_ecore_x_disp) return;
   if (!_ecore_x_resource_initted) return;
   if (_ecore_x_resource_db) _ecore_x_resource_db = NULL;
   _ecore_x_resource_initted = EINA_FALSE;
}

/**
 * @brief Loads X resources from a specified file.
 *
 * This function reads an X resource database from the given file.
 * If a database is already loaded, it is destroyed and replaced
 * with the new one.
 *
 * @param file The path to the resource file to load.
 *             Example: "~/.Xresources"
 */
EAPI void
ecore_x_rersource_load(const char *file)
{
   XrmDatabase db;

   if (!_ecore_x_disp) return;
   _ecore_x_resource_init();
   db = XrmGetFileDatabase(file);
   if (!db) return;
   if (_ecore_x_resource_db) XrmDestroyDatabase(_ecore_x_resource_db);
   _ecore_x_resource_db = db;
// something smells fishy/broken in xlib - this segfaults in trying to free
// up the previous db bveing used for that display...
//   XrmSetDatabase(_ecore_x_disp, db);
}

/**
 * @brief Sets a string value in the X resource database.
 *
 * This function adds or updates a resource in the current X resource database.
 * The resource is specified by a key and a string value.
 * If the database does not exist, it attempts to retrieve it from the display.
 *
 * @param key The resource key (name). Example: "myProgram.mySetting"
 * @param val The string value to set for the key. Example: "true"
 */
EAPI void
ecore_x_resource_db_string_set(const char *key, const char *val)
{
   if (!_ecore_x_disp) return;
   _ecore_x_resource_init();
   if ((!key) || (!val)) return;
   if (!_ecore_x_resource_db)
     _ecore_x_resource_db = XrmGetDatabase(_ecore_x_disp);
   XrmPutStringResource(&_ecore_x_resource_db, key, val);
}

/**
 * @brief Retrieves a string value from the X resource database.
 *
 * This function looks up a resource by its key in the current X resource
 * database. It expects the resource to be of type "String".
 * If the database does not exist, it attempts to retrieve it from the display.
 *
 * @param key The resource key (name) to retrieve. Example: "myProgram.mySetting"
 * @return The string value associated with the key if found and is of type
 *         "String" with a size greater than 0; otherwise, @c NULL.
 *         The returned string is owned by the Xrm database and should not be freed.
 */
EAPI const char *
ecore_x_resource_db_string_get(const char *key)
{
   char *type = NULL;
   XrmValue xval = { 0, NULL };

   if (!_ecore_x_disp) return NULL;
   _ecore_x_resource_init();
   if (!_ecore_x_resource_db)
     _ecore_x_resource_db = XrmGetDatabase(_ecore_x_disp);
   if (XrmGetResource(_ecore_x_resource_db, key, "String", &type, &xval))
     {
        if (xval.addr && (!strcmp(type, "String")))
          {
             if (xval.size > 0) return xval.addr;
          }
     }
   return NULL;
}

/**
 * @brief Flushes the current X resource database to the X server.
 *
 * This function writes the current in-memory X resource database to a
 * temporary file, then reads its content and sets it as the
 * `RESOURCE_MANAGER` property on all root windows of the current display.
 * This makes the resources available to other X clients.
 * The temporary file is deleted after use.
 */
EAPI void
ecore_x_resource_db_flush(void)
{
   Ecore_X_Atom atom, type;
   Ecore_X_Window *roots;
   int i, num, fd;
   char *str;
   Eina_Tmpstr *path = NULL;
   off_t offset;

   if (!_ecore_x_disp) return;
   _ecore_x_resource_init();
   if (!_ecore_x_resource_db) return;
   fd = eina_file_mkstemp("ecore-x-resource-XXXXXX", &path);
   if (fd < 0) return;
   XrmPutFileDatabase(_ecore_x_resource_db, path);
   offset = lseek(fd, 0, SEEK_END);
   if (offset > 0)
     {
        lseek(fd, 0, SEEK_SET);
        str = malloc(offset + 1);
        if (str)
          {
             if (read(fd, str, offset) == offset)
               {
                  str[offset] = 0;
                  atom = XInternAtom(_ecore_x_disp, "RESOURCE_MANAGER", False);
                  type = ECORE_X_ATOM_STRING;
                  roots = ecore_x_window_root_list(&num);
                  if (roots)
                    {
                       for (i = 0; i < num; i++)
                         ecore_x_window_prop_property_set(roots[i],
                                                          atom, type,
                                                          8, str, offset);
                       free(roots);
                    }
               }
             free(str);
          }
     }
   close(fd);
   unlink(path);
   eina_tmpstr_del(path);
}
