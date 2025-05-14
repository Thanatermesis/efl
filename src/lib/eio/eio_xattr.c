/* EIO - EFL data type library
 * Copyright (C) 2011 Enlightenment Developers:
 *           Cedric Bail <cedric.bail@free.fr>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library;
 * if not, see <http://www.gnu.org/licenses/>.
 */

#include "eio_private.h"
#include "Eio.h"

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/

/**
 * @cond LOCAL
 */

/**
 * @brief Worker thread function to list extended attributes of a directory.
 *
 * This function is executed in a separate thread. It iterates over the
 * extended attributes of the specified directory, filters them if a
 * filter callback is provided, and sends them back to the main thread
 * in batches.
 *
 * @param data Pointer to Eio_File_Char_Ls structure containing operation details.
 * @param thread Pointer to the Ecore_Thread executing this function.
 */
static void
_eio_ls_xattr_heavy(void *data, Ecore_Thread *thread)
{
   Eio_File_Char_Ls *async = data;
   Eina_Iterator *it;
   Eina_List *pack = NULL;
   const char *tmp;
   double start;

   it = eina_xattr_ls(async->ls.directory);
   if (!it) return;

   eio_file_container_set(&async->ls.common, eina_iterator_container_get(it));

   start = ecore_time_get();

   EINA_ITERATOR_FOREACH(it, tmp)
     {
        Eina_Bool filter = EINA_TRUE;

        if (async->filter_cb)
          {
             filter = async->filter_cb((void*) async->ls.common.data,
                                       &async->ls.common,
                                       tmp);
          }

        if (filter)
          {
             Eio_File_Char *send_fc;

             send_fc = eio_char_malloc();
             if (!send_fc) goto on_error;

             send_fc->filename = eina_stringshare_add(tmp);
             send_fc->associated = async->ls.common.worker.associated;
             async->ls.common.worker.associated = NULL;

             pack = eina_list_append(pack, send_fc);
          }
        else
          {
          on_error:
             if (async->ls.common.worker.associated)
               {
                  eina_hash_free(async->ls.common.worker.associated);
                  async->ls.common.worker.associated = NULL;
               }
          }

        pack = eio_pack_send(thread, pack, &start);

        if (ecore_thread_check(thread))
          break;
     }

   if (pack) ecore_thread_feedback(thread, pack);

   async->ls.ls = it;
}

/**
 * @brief Worker thread function to get a specific extended attribute of a file.
 *
 * This function is executed in a separate thread. It retrieves the value
 * of a specified extended attribute for a given file. The type of the
 * attribute (data, string, double, int) is determined by async->op.
 *
 * @param data Pointer to Eio_File_Xattr structure containing operation details.
 * @param thread Pointer to the Ecore_Thread executing this function.
 */
static void
_eio_file_xattr_get(void *data, Ecore_Thread *thread)
{
   Eio_File_Xattr *async = data;
   Eina_Bool failure = EINA_FALSE;
   const char *file;
   const char *attribute;

   file = async->path;
   attribute = async->attribute;

   switch (async->op)
     {
     case EIO_XATTR_DATA:
       async->todo.xdata.xattr_size = 0;
       async->todo.xdata.xattr_data = NULL;

       async->todo.xdata.xattr_data = eina_xattr_get(file, attribute, &async->todo.xdata.xattr_size);
       if (!async->todo.xdata.xattr_data) failure = EINA_TRUE;
       break;
     case EIO_XATTR_STRING:
       async->todo.xstring.xattr_string = eina_xattr_string_get(file, attribute);
       if (!async->todo.xstring.xattr_string) failure = EINA_TRUE;
       break;
     case EIO_XATTR_DOUBLE:
       failure = !eina_xattr_double_get(file, attribute, &async->todo.xdouble.xattr_double);
       break;
     case EIO_XATTR_INT:
       failure = !eina_xattr_int_get(file, attribute, &async->todo.xint.xattr_int);
       break;
     }

   if (failure)
     ecore_thread_cancel(thread);
}

/**
 * @brief Frees resources associated with an Eio_File_Xattr operation.
 *
 * This function releases stringshares for path and attribute, and frees
 * any data allocated for the attribute value if it was a get operation.
 * It also frees the base Eio_File structure.
 *
 * @param async Pointer to the Eio_File_Xattr structure to free.
 */
static void
_eio_file_xattr_free(Eio_File_Xattr *async)
{
   eina_stringshare_del(async->path);
   eina_stringshare_del(async->attribute);
   if (!async->set)
     {
       if (async->op == EIO_XATTR_DATA) free(async->todo.xdata.xattr_data);
       if (async->op == EIO_XATTR_STRING) free(async->todo.xstring.xattr_string);
     }
   eio_file_free(&async->common);
}

/**
 * @brief Callback executed in the main thread after _eio_file_xattr_get successfully completes.
 *
 * This function invokes the user-provided done callback with the retrieved
 * extended attribute data, based on the attribute type.
 *
 * @param data Pointer to Eio_File_Xattr structure.
 * @param thread Pointer to the Ecore_Thread (unused).
 */
static void
_eio_file_xattr_get_done(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   Eio_File_Xattr *async = data;

   switch (async->op)
     {
     case EIO_XATTR_DATA:
       if (async->todo.xdata.done_cb)
	 async->todo.xdata.done_cb((void *) async->common.data, &async->common, async->todo.xdata.xattr_data, async->todo.xdata.xattr_size);
       break;
     case EIO_XATTR_STRING:
       if (async->todo.xstring.done_cb)
	 async->todo.xstring.done_cb((void *) async->common.data, &async->common, async->todo.xstring.xattr_string);
       break;
     case EIO_XATTR_DOUBLE:
       if (async->todo.xdouble.done_cb)
	 async->todo.xdouble.done_cb((void *) async->common.data, &async->common, async->todo.xdouble.xattr_double);
       break;
     case EIO_XATTR_INT:
       if (async->todo.xint.done_cb)
	 async->todo.xint.done_cb((void *) async->common.data, &async->common, async->todo.xint.xattr_int);
       break;
     }

   _eio_file_xattr_free(async);
}

/**
 * @brief Callback executed in the main thread if _eio_file_xattr_get encounters an error.
 *
 * This function invokes the user-provided error callback and then frees
 * the Eio_File_Xattr structure.
 *
 * @param data Pointer to Eio_File_Xattr structure.
 * @param thread Pointer to the Ecore_Thread (unused).
 */
static void
_eio_file_xattr_get_error(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   Eio_File_Xattr *async = data;

   eio_file_error(&async->common);
   _eio_file_xattr_free(async);
}

/**
 * @brief Worker thread function to set a specific extended attribute of a file.
 *
 * This function is executed in a separate thread. It sets the value
 * of a specified extended attribute for a given file. The type of the
 * attribute (data, string, double, int) is determined by async->op.
 *
 * @param data Pointer to Eio_File_Xattr structure containing operation details.
 * @param thread Pointer to the Ecore_Thread executing this function.
 */
static void
_eio_file_xattr_set(void *data, Ecore_Thread *thread)
{
   Eio_File_Xattr *async = data;
   const char *file;
   const char *attribute;
   Eina_Xattr_Flags flags;
   Eina_Bool failure = EINA_FALSE;

   file = async->path;
   attribute = async->attribute;
   flags = async->flags;

   switch (async->op)
     {
     case EIO_XATTR_DATA:
       failure = !eina_xattr_set(file, attribute, async->todo.xdata.xattr_data, async->todo.xdata.xattr_size, flags);
       async->common.length = async->todo.xdata.xattr_size;
       break;
     case EIO_XATTR_STRING:
       failure = !eina_xattr_string_set(file, attribute, async->todo.xstring.xattr_string, flags);
       async->common.length = strlen(async->todo.xstring.xattr_string) + 1;
       break;
     case EIO_XATTR_DOUBLE:
       failure = !eina_xattr_double_set(file, attribute, async->todo.xdouble.xattr_double, flags);
       async->common.length = sizeof (double);
       break;
     case EIO_XATTR_INT:
       failure = !eina_xattr_int_set(file, attribute, async->todo.xint.xattr_int, flags);
       async->common.length = sizeof (int);
       break;
     }

   if (failure) eio_file_thread_error(&async->common, thread);
}

/**
 * @brief Callback executed in the main thread after _eio_file_xattr_set successfully completes.
 *
 * This function invokes the user-provided done callback if the thread was not cancelled.
 * It then frees the Eio_File_Xattr structure.
 *
 * @param data Pointer to Eio_File_Xattr structure.
 * @param thread Pointer to the Ecore_Thread.
 */
static void
_eio_file_xattr_set_done(void *data, Ecore_Thread *thread)
{
   Eio_File_Xattr *async = data;

   if (!ecore_thread_check(thread))
     {
        if (async->common.done_cb)
          async->common.done_cb((void*) async->common.data, &async->common);
     }

   _eio_file_xattr_free(async);
}

/**
 * @brief Callback executed in the main thread if _eio_file_xattr_set encounters an error.
 *
 * This function invokes the user-provided error callback and then frees
 * the Eio_File_Xattr structure.
 *
 * @param data Pointer to Eio_File_Xattr structure.
 * @param thread Pointer to the Ecore_Thread (unused).
 */
static void
_eio_file_xattr_set_error(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   Eio_File_Xattr *async = data;

   eio_file_error(&async->common);
   _eio_file_xattr_free(async);
}

/**
 * @brief Sets up an Eio_File_Xattr structure for a get operation.
 *
 * Initializes the common Eio_File fields and specific fields for getting
 * an extended attribute.
 *
 * @param async Pointer to the Eio_File_Xattr structure to initialize.
 * @param path The file path.
 * @param attribute The name of the extended attribute.
 * @param error_cb Callback function for errors.
 * @param data User data for the callbacks.
 * @return An Eio_File handle for the operation, or NULL on failure.
 */
static Eio_File *
_eio_file_xattr_setup_get(Eio_File_Xattr *async,
			  const char *path,
			  const char *attribute,
			  Eio_Error_Cb error_cb,
			  const void *data)
{
   async->path = eina_stringshare_add(path);
   async->attribute = eina_stringshare_add(attribute);
   async->set = EINA_FALSE;

   if (!eio_file_set(&async->common,
                     NULL,
                     error_cb,
                     data,
                     _eio_file_xattr_get,
                     _eio_file_xattr_get_done,
                     _eio_file_xattr_get_error))
     return NULL;

   return &async->common;
}

/**
 * @brief Sets up an Eio_File_Xattr structure for a set operation.
 *
 * Initializes the common Eio_File fields and specific fields for setting
 * an extended attribute.
 *
 * @param async Pointer to the Eio_File_Xattr structure to initialize.
 * @param path The file path.
 * @param attribute The name of the extended attribute.
 * @param flags Flags for the set operation (e.g., EINA_XATTR_CREATE, EINA_XATTR_REPLACE).
 * @param done_cb Callback function for successful completion.
 * @param error_cb Callback function for errors.
 * @param data User data for the callbacks.
 * @return An Eio_File handle for the operation, or NULL on failure.
 */
static Eio_File *
_eio_file_xattr_setup_set(Eio_File_Xattr *async,
			  const char *path,
			  const char *attribute,
			  Eina_Xattr_Flags flags,
			  Eio_Done_Cb done_cb,
			  Eio_Error_Cb error_cb,
			  const void *data)
{
   async->path = eina_stringshare_add(path);
   async->attribute = eina_stringshare_add(attribute);
   async->flags = flags;
   async->set = EINA_TRUE;

   if (!eio_file_set(&async->common,
                     done_cb,
                     error_cb,
                     data,
                     _eio_file_xattr_set,
                     _eio_file_xattr_set_done,
                     _eio_file_xattr_set_error))
     return NULL;

   return &async->common;
}

/**
 * @endcond
 */


/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/

/**
 * @cond LOCAL
 */

/**
 * @endcond
 */


/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

/**
 * @internal
 * @brief Internal function to list extended attributes of a file or directory.
 *
 * This function serves as a common backend for eio_file_xattr and _eio_file_xattr.
 * It sets up an asynchronous operation to list extended attributes.
 * One of main_cb or main_internal_cb must be provided.
 *
 * @param path The path to the file or directory.
 * @param filter_cb Optional callback to filter attributes.
 * @param main_cb Callback to process each attribute name (string).
 *        Example: void main_cb(void *data, Eio_File *handler, const char *xattr_name);
 * @param main_internal_cb Callback to process an array of attribute names.
 *        Example: void main_internal_cb(void *data, Eio_File *handler, const Eina_Array *xattr_names_array);
 * @param done_cb Callback when the listing is complete.
 * @param error_cb Callback if an error occurs.
 * @param data User data for the callbacks.
 * @return An Eio_File handle for the operation, or NULL on failure.
 */
static Eio_File *
_eio_file_internal_xattr(const char *path,
                         Eio_Filter_Cb filter_cb,
                         Eio_Main_Cb main_cb,
                         Eio_Array_Cb main_internal_cb,
                         Eio_Done_Cb done_cb,
                         Eio_Error_Cb error_cb,
                         const void *data)
{
  Eio_File_Char_Ls *async;

  EINA_SAFETY_ON_NULL_RETURN_VAL(path, NULL);
  EINA_SAFETY_ON_NULL_RETURN_VAL(done_cb, NULL);
  EINA_SAFETY_ON_NULL_RETURN_VAL(error_cb, NULL);

  async = eio_common_alloc(sizeof (Eio_File_Char_Ls));
  EINA_SAFETY_ON_NULL_RETURN_VAL(async, NULL);

  async->ls.directory = eina_stringshare_add(path);
  async->filter_cb = filter_cb;

  if (main_internal_cb)
    {
       async->main_internal_cb = main_internal_cb;
       async->ls.gather = EINA_TRUE;
    }
  else
    {
       async->main_cb = main_cb;
    }

  if (!eio_long_file_set(&async->ls.common,
                         done_cb,
                         error_cb,
                         data,
                         _eio_ls_xattr_heavy,
                         _eio_string_notify,
                         eio_async_end,
                         eio_async_error))
    return NULL;

  return &async->ls.common;
}

/**
 * @brief Asynchronously lists all extended attributes for a given path.
 *
 * This function initiates an asynchronous operation to list the names of
 * all extended attributes associated with the file or directory specified by @p path.
 * For each attribute found, @p main_cb is called.
 *
 * @param path The path to the file or directory.
 * @param filter_cb An optional function to filter attribute names.
 *        It receives user data, the Eio_File handler, and the attribute name.
 *        Return EINA_TRUE to include the attribute, EINA_FALSE to exclude.
 *        Example: Eina_Bool filter_cb(void *data, Eio_File *handler, const char *xattr_name);
 * @param main_cb A callback function invoked for each attribute name found.
 *        Example: void main_cb(void *data, Eio_File *handler, const char *xattr_name);
 * @param done_cb A callback function invoked when the listing is complete.
 *        Example: void done_cb(void *data, Eio_File *handler);
 * @param error_cb A callback function invoked if an error occurs.
 *        Example: void error_cb(void *data, Eio_File *handler, int error_code);
 * @param data User-specific data to be passed to the callbacks.
 * @return An Eio_File handle for the asynchronous operation, or @c NULL on failure.
 *         This handle can be used with eio_file_cancel() or eio_file_direct_do().
 */
EIO_API Eio_File *
eio_file_xattr(const char *path,
               Eio_Filter_Cb filter_cb,
               Eio_Main_Cb main_cb,
               Eio_Done_Cb done_cb,
               Eio_Error_Cb error_cb,
               const void *data)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(main_cb, NULL);

  return _eio_file_internal_xattr(path, filter_cb, main_cb, NULL, done_cb, error_cb, data);
}

/**
 * @internal
 * @brief Asynchronously lists all extended attributes, delivering results as an array.
 *
 * Similar to eio_file_xattr(), but calls @p main_internal_cb with an Eina_Array
 * of attribute names. This is typically used internally or when batch processing
 * of attribute names is preferred.
 *
 * @param path The path to the file or directory.
 * @param main_internal_cb A callback function invoked with an array of attribute names.
 *        The Eina_Array contains (char *) elements.
 *        Example: void main_internal_cb(void *data, Eio_File *handler, const Eina_Array *xattr_names_array);
 * @param done_cb A callback function invoked when the listing is complete.
 * @param error_cb A callback function invoked if an error occurs.
 * @param data User-specific data to be passed to the callbacks.
 * @return An Eio_File handle for the asynchronous operation, or @c NULL on failure.
 */
Eio_File *
_eio_file_xattr(const char *path,
                Eio_Array_Cb main_internal_cb,
                Eio_Done_Cb done_cb,
                Eio_Error_Cb error_cb,
                const void *data)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(main_internal_cb, NULL);

   return _eio_file_internal_xattr(path, NULL, NULL, main_internal_cb, done_cb, error_cb, data);
}

/**
 * @brief Asynchronously retrieves an extended attribute as raw data.
 *
 * Initiates an asynchronous operation to get the value of the specified
 * extended attribute @p attribute for the file @p path. The result is
 * returned as a data buffer and its size.
 *
 * @param path The path to the file.
 * @param attribute The name of the extended attribute to retrieve.
 * @param done_cb Callback invoked upon successful retrieval.
 *        It receives the user data, Eio_File handler, a pointer to the attribute data,
 *        and the size of the data. The data buffer is owned by Eio and freed after the callback.
 *        Example: void done_cb(void *data, Eio_File *handler, const void *xattr_data, unsigned int xattr_size);
 * @param error_cb Callback invoked if an error occurs.
 * @param data User-specific data for the callbacks.
 * @return An Eio_File handle for the operation, or @c NULL on failure.
 */
EIO_API Eio_File *
eio_file_xattr_get(const char *path,
		   const char *attribute,
		   Eio_Done_Data_Cb done_cb,
		   Eio_Error_Cb error_cb,
                   const void *data)
{
   Eio_File_Xattr *async;

   EINA_SAFETY_ON_NULL_RETURN_VAL(path, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(attribute, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(done_cb, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(error_cb, NULL);

   async = malloc(sizeof (Eio_File_Xattr));
   if (!async) return NULL;

   async->op = EIO_XATTR_DATA;
   async->todo.xdata.done_cb = done_cb;

   return _eio_file_xattr_setup_get(async, path, attribute, error_cb, data);
}

/**
 * @brief Asynchronously retrieves an extended attribute as a string.
 *
 * Initiates an asynchronous operation to get the value of the specified
 * extended attribute @p attribute for the file @p path. The result is
 * returned as a null-terminated string.
 *
 * @param path The path to the file.
 * @param attribute The name of the extended attribute to retrieve.
 * @param done_cb Callback invoked upon successful retrieval.
 *        It receives user data, Eio_File handler, and the attribute string.
 *        The string is owned by Eio and freed after the callback.
 *        Example: void done_cb(void *data, Eio_File *handler, const char *xattr_string);
 * @param error_cb Callback invoked if an error occurs.
 * @param data User-specific data for the callbacks.
 * @return An Eio_File handle for the operation, or @c NULL on failure.
 */
EIO_API Eio_File *
eio_file_xattr_string_get(const char *path,
			  const char *attribute,
			  Eio_Done_String_Cb done_cb,
			  Eio_Error_Cb error_cb,
			  const void *data)
{
   Eio_File_Xattr *async;

   EINA_SAFETY_ON_NULL_RETURN_VAL(path, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(attribute, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(done_cb, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(error_cb, NULL);

   async = malloc(sizeof (Eio_File_Xattr));
   if (!async) return NULL;

   async->op = EIO_XATTR_STRING;
   async->todo.xstring.done_cb = done_cb;

   return _eio_file_xattr_setup_get(async, path, attribute, error_cb, data);
}

/**
 * @brief Asynchronously retrieves an extended attribute as a double.
 *
 * Initiates an asynchronous operation to get the value of the specified
 * extended attribute @p attribute for the file @p path. The result is
 * returned as a double-precision floating-point number.
 *
 * @param path The path to the file.
 * @param attribute The name of the extended attribute to retrieve.
 * @param done_cb Callback invoked upon successful retrieval.
 *        It receives user data, Eio_File handler, and the attribute value as a double.
 *        Example: void done_cb(void *data, Eio_File *handler, double xattr_double);
 * @param error_cb Callback invoked if an error occurs.
 * @param data User-specific data for the callbacks.
 * @return An Eio_File handle for the operation, or @c NULL on failure.
 */
EIO_API Eio_File *
eio_file_xattr_double_get(const char *path,
			  const char *attribute,
			  Eio_Done_Double_Cb done_cb,
			  Eio_Error_Cb error_cb,
			  const void *data)
{
   Eio_File_Xattr *async;

   EINA_SAFETY_ON_NULL_RETURN_VAL(path, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(attribute, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(done_cb, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(error_cb, NULL);

   async = malloc(sizeof (Eio_File_Xattr));
   if (!async) return NULL;

   async->op = EIO_XATTR_DOUBLE;
   async->todo.xdouble.done_cb = done_cb;

   return _eio_file_xattr_setup_get(async, path, attribute, error_cb, data);
}

/**
 * @brief Asynchronously retrieves an extended attribute as an integer.
 *
 * Initiates an asynchronous operation to get the value of the specified
 * extended attribute @p attribute for the file @p path. The result is
 * returned as an integer.
 *
 * @param path The path to the file.
 * @param attribute The name of the extended attribute to retrieve.
 * @param done_cb Callback invoked upon successful retrieval.
 *        It receives user data, Eio_File handler, and the attribute value as an int.
 *        Example: void done_cb(void *data, Eio_File *handler, int xattr_int);
 * @param error_cb Callback invoked if an error occurs.
 * @param data User-specific data for the callbacks.
 * @return An Eio_File handle for the operation, or @c NULL on failure.
 */
EIO_API Eio_File *
eio_file_xattr_int_get(const char *path,
		       const char *attribute,
		       Eio_Done_Int_Cb done_cb,
		       Eio_Error_Cb error_cb,
		       const void *data)
{
   Eio_File_Xattr *async;

   EINA_SAFETY_ON_NULL_RETURN_VAL(path, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(attribute, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(done_cb, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(error_cb, NULL);

   async = malloc(sizeof (Eio_File_Xattr));
   if (!async) return NULL;

   async->op = EIO_XATTR_INT;
   async->todo.xint.done_cb = done_cb;

   return _eio_file_xattr_setup_get(async, path, attribute, error_cb, data);
}

/**
 * @brief Asynchronously sets an extended attribute as raw data.
 *
 * Initiates an asynchronous operation to set the value of the extended
 * attribute @p attribute for the file @p path using the provided raw data.
 *
 * @param path The path to the file.
 * @param attribute The name of the extended attribute to set.
 * @param xattr_data Pointer to the data to be set.
 * @param xattr_size Size of the data in bytes.
 * @param flags Flags for the operation (e.g., EINA_XATTR_CREATE, EINA_XATTR_REPLACE).
 * @param done_cb Callback invoked upon successful completion.
 *        Example: void done_cb(void *data, Eio_File *handler);
 * @param error_cb Callback invoked if an error occurs.
 * @param data User-specific data for the callbacks.
 * @return An Eio_File handle for the operation, or @c NULL on failure.
 */
EIO_API Eio_File *
eio_file_xattr_set(const char *path,
                   const char *attribute,
                   const char *xattr_data,
                   unsigned int xattr_size,
                   Eina_Xattr_Flags flags,
                   Eio_Done_Cb done_cb,
                   Eio_Error_Cb error_cb,
                   const void *data)
{
   Eio_File_Xattr *async;

   EINA_SAFETY_ON_NULL_RETURN_VAL(path, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(attribute, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(done_cb, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(xattr_data, NULL);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(xattr_size, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(error_cb, NULL);

   async = malloc(sizeof (Eio_File_Xattr) + xattr_size);
   if (!async) return NULL;

   async->op = EIO_XATTR_DATA;
   async->todo.xdata.xattr_size = xattr_size;
   async->todo.xdata.xattr_data = (char*) (async + 1);
   memcpy(async->todo.xdata.xattr_data, xattr_data, xattr_size);

   return _eio_file_xattr_setup_set(async, path, attribute, flags, done_cb, error_cb, data);
}

/**
 * @brief Asynchronously sets an extended attribute as a string.
 *
 * Initiates an asynchronous operation to set the value of the extended
 * attribute @p attribute for the file @p path using the provided null-terminated string.
 *
 * @param path The path to the file.
 * @param attribute The name of the extended attribute to set.
 * @param xattr_string The null-terminated string to be set.
 * @param flags Flags for the operation (e.g., EINA_XATTR_CREATE, EINA_XATTR_REPLACE).
 * @param done_cb Callback invoked upon successful completion.
 * @param error_cb Callback invoked if an error occurs.
 * @param data User-specific data for the callbacks.
 * @return An Eio_File handle for the operation, or @c NULL on failure.
 */
EIO_API Eio_File *
eio_file_xattr_string_set(const char *path,
			  const char *attribute,
			  const char *xattr_string,
			  Eina_Xattr_Flags flags,
			  Eio_Done_Cb done_cb,
			  Eio_Error_Cb error_cb,
			  const void *data)
{
   Eio_File_Xattr *async;
   int length;

   EINA_SAFETY_ON_NULL_RETURN_VAL(path, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(attribute, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(xattr_string, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(done_cb, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(error_cb, NULL);

   async = malloc(sizeof (Eio_File_Xattr));
   if (!async) return NULL;

   length = strlen(xattr_string) + 1;

   async->op = EIO_XATTR_STRING;
   async->todo.xstring.xattr_string = malloc(length);
   if (!async->todo.xstring.xattr_string)
     {
       free(async);
       return NULL;
     }
   memcpy(async->todo.xstring.xattr_string, xattr_string, length);

   return _eio_file_xattr_setup_set(async, path, attribute, flags, done_cb, error_cb, data);
}

/**
 * @brief Asynchronously sets an extended attribute as a double.
 *
 * Initiates an asynchronous operation to set the value of the extended
 * attribute @p attribute for the file @p path using the provided double value.
 *
 * @param path The path to the file.
 * @param attribute The name of the extended attribute to set.
 * @param xattr_double The double value to be set.
 * @param flags Flags for the operation (e.g., EINA_XATTR_CREATE, EINA_XATTR_REPLACE).
 * @param done_cb Callback invoked upon successful completion.
 * @param error_cb Callback invoked if an error occurs.
 * @param data User-specific data for the callbacks.
 * @return An Eio_File handle for the operation, or @c NULL on failure.
 */
EIO_API Eio_File *
eio_file_xattr_double_set(const char *path,
			  const char *attribute,
			  double xattr_double,
			  Eina_Xattr_Flags flags,
			  Eio_Done_Cb done_cb,
			  Eio_Error_Cb error_cb,
			  const void *data)
{
   Eio_File_Xattr *async;

   EINA_SAFETY_ON_NULL_RETURN_VAL(path, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(attribute, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(done_cb, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(error_cb, NULL);

   async = malloc(sizeof (Eio_File_Xattr));
   if (!async) return NULL;

   async->op = EIO_XATTR_DOUBLE;
   async->todo.xdouble.xattr_double = xattr_double;

   return _eio_file_xattr_setup_set(async, path, attribute, flags, done_cb, error_cb, data);
}

/**
 * @brief Asynchronously sets an extended attribute as an integer.
 *
 * Initiates an asynchronous operation to set the value of the extended
 * attribute @p attribute for the file @p path using the provided integer value.
 *
 * @param path The path to the file.
 * @param attribute The name of the extended attribute to set.
 * @param xattr_int The integer value to be set.
 * @param flags Flags for the operation (e.g., EINA_XATTR_CREATE, EINA_XATTR_REPLACE).
 * @param done_cb Callback invoked upon successful completion.
 * @param error_cb Callback invoked if an error occurs.
 * @param data User-specific data for the callbacks.
 * @return An Eio_File handle for the operation, or @c NULL on failure.
 */
EIO_API Eio_File *
eio_file_xattr_int_set(const char *path,
		       const char *attribute,
		       int xattr_int,
		       Eina_Xattr_Flags flags,
		       Eio_Done_Cb done_cb,
		       Eio_Error_Cb error_cb,
		       const void *data)
{
   Eio_File_Xattr *async;

   EINA_SAFETY_ON_NULL_RETURN_VAL(path, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(attribute, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(done_cb, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(error_cb, NULL);

   async = malloc(sizeof (Eio_File_Xattr));
   if (!async) return NULL;

   async->op = EIO_XATTR_INT;
   async->todo.xint.xattr_int = xattr_int;

   return _eio_file_xattr_setup_set(async, path, attribute, flags, done_cb, error_cb, data);
}
