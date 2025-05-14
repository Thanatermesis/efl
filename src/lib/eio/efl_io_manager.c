/* EIO - EFL data type library
 * Copyright (C) 2016 Enlightenment Developers:
 *           Felipe Magno de Almeida <felipe@expertisesolutions.com.br>
 *           Lauro Moura <lauromoura@expertisesolutions.com.br>
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Eo.h>
#include <Ecore.h>
#include "Eio.h"

#include "eio_private.h"

typedef struct _Efl_Io_Manager_Data Efl_Io_Manager_Data;

/**
 * @brief Private data structure for the Efl_Io_Manager class.
 *
 * This structure holds the instance-specific data for an Efl_Io_Manager object,
 * including a pointer to the Eo object itself and a list of pending I/O operations.
 */
struct _Efl_Io_Manager_Data
{
   Eo *object; /**< The Eo object this data belongs to. */
   Eina_List *operations; /**< A list of currently active Eio_File operations. */
};

/**
 * @brief Function pointer type for direct listing operations.
 *
 * This defines the signature for functions that perform direct (non-recursive)
 * directory listing or file information retrieval.
 *
 * @param path The file system path to list or inspect.
 * @param Eio_Filter_Direct_Cb Filter callback for each entry found.
 * @param Eio_Main_Direct_Cb Main callback to process entries in the main loop.
 * @param Eio_Done_Cb Callback invoked when the operation is complete.
 * @param Eio_Error_Cb Callback invoked if an error occurs.
 * @param data User-provided data for the callbacks.
 * @return An Eio_File handle representing the asynchronous operation.
 */
typedef Eio_File* (*Efl_Io_Manager_Direct_Ls_Func)(const char *path, Eio_Filter_Direct_Cb, Eio_Main_Direct_Cb, Eio_Done_Cb, Eio_Error_Cb, const void *data);

typedef struct _Job_Closure Job_Closure;
/**
 * @brief Structure to hold data for a job closure.
 *
 * This is likely used to pass context information to asynchronous operations
 * or their callbacks.
 */
struct _Job_Closure
{
   Eo *object; /**< The associated Eo object. */
   Efl_Io_Manager_Data *pdata; /**< Pointer to the manager's private data. */
   Eio_File *file; /**< The Eio_File handle for the operation. */
   Eina_Bool delete_me; /**< Flag indicating if this closure should be deleted. */
   void *delayed_arg; /**< Argument for a delayed operation. */
   Efl_Io_Manager_Direct_Ls_Func direct_func;  /**< Used when dispatching direct ls funcs. */
};

/* Future have to be resolved right away in the thread context */
typedef struct _Eio_Future_Entry Eio_Future_Entry;
/**
 * @brief Represents an entry for an Eina_Future scheduled by EIO.
 *
 * This structure holds the necessary information to execute a callback
 * associated with an Eina_Future when its value is ready. It's managed
 * by a custom scheduler (`eio_future_scheduler`).
 */
struct _Eio_Future_Entry
{
   Eina_Future_Schedule_Entry base; /**< Base structure for Eina_Future_Scheduler. */
   Eina_Future_Scheduler_Cb cb;    /**< The callback to execute. */
   Eina_Future *future;            /**< The Eina_Future this entry is for. */
   Eina_Value value;               /**< The value to pass to the callback. */
};

static Eina_Trash *eio_entry_trash = NULL; /**< A trash stack for recycling Eio_Future_Entry allocations. */
static unsigned int eio_entry_trash_count = 0; /**< Count of items currently in eio_entry_trash. */
static Eina_List *entries = NULL; /**< List of pending Eio_Future_Entry items to be processed. */

/**
 * @brief Schedules a callback for an Eina_Future using the EIO custom scheduler.
 *
 * This function is part of the Eina_Future_Scheduler interface. It creates
 * or reuses an Eio_Future_Entry and adds it to a list of pending entries
 * to be processed later in the main loop context.
 *
 * @param sched The scheduler invoking this function.
 * @param cb The callback function to be executed.
 * @param future The future associated with this schedule request.
 * @param value The value to be passed to the callback.
 * @return A schedule entry handle, or NULL on allocation failure.
 */
static Eina_Future_Schedule_Entry *
eio_future_schedule(Eina_Future_Scheduler *sched,
                    Eina_Future_Scheduler_Cb cb,
                    Eina_Future *future,
                    Eina_Value value)
{
   Eio_Future_Entry *ef = NULL;

   if (!eio_entry_trash)
     {
        ef = calloc(1, sizeof (Eio_Future_Entry));
        if (!ef) return NULL;
     }
   else
     {
        ef = eina_trash_pop(&eio_entry_trash);
        eio_entry_trash_count--;
     }
   ef->base.scheduler = sched;
   ef->cb = cb;
   ef->future = future;
   ef->value = value;

   entries = eina_list_append(entries, ef);

   return &ef->base;
}

/**
 * @brief Frees or recycles an Eio_Future_Entry.
 *
 * Removes the entry from the pending list and either frees its memory
 * or adds it to a trash stack for later reuse, to optimize allocations.
 *
 * @param ef The Eio_Future_Entry to free or recycle.
 */
static void
eio_future_free(Eio_Future_Entry *ef)
{
   entries = eina_list_remove(entries, ef);

   if (eio_entry_trash_count > 8)
     {
        free(ef);
        return ;
     }
   eina_trash_push(&eio_entry_trash, ef);
   eio_entry_trash_count++;
}

/**
 * @brief Recalls (cancels) a previously scheduled Eio_Future_Entry.
 *
 * This function is part of the Eina_Future_Scheduler interface. It's called
 * when a future is cancelled before its callback could be executed.
 * It flushes any associated Eina_Value and frees/recycles the entry.
 *
 * @param se The schedule entry to recall.
 */
static void
eio_future_recall(Eina_Future_Schedule_Entry *se)
{
   Eio_Future_Entry *ef = (Eio_Future_Entry *) se;

   eina_value_flush(&ef->value);
   eio_future_free(ef);
}

static Eina_Future_Scheduler eio_future_scheduler = {
   .schedule = eio_future_schedule,
   .recall = eio_future_recall,
};

/**
 * @brief A dummy cancellation function for Eina_Promise.
 *
 * This function does nothing. It's used when creating Eina_Promise instances
 * where custom cancellation logic is not needed or handled elsewhere.
 *
 * @param data User data associated with the promise (unused).
 * @param p The promise being cancelled (unused).
 */
static void
eio_dummy_cancel(void *data EINA_UNUSED, const Eina_Promise *p EINA_UNUSED)
{
}

/**
 * @brief Processes all pending Eio_Future_Entry items.
 *
 * Iterates through the `entries` list, executing the callback for each
 * Eio_Future_Entry with its associated value, and then frees/recycles the entry.
 * This ensures that future callbacks are run in the correct (main loop) context.
 */
static void
eio_process_entry(void)
{
   Eio_Future_Entry *ef;

   while (entries)
     {
        ef = eina_list_data_get(entries);
        ef->cb(ef->future, ef->value);
        eio_future_free(ef);
     }
}

/**
 * @brief Creates a new Eina_Promise associated with an Eo object.
 *
 * The promise is configured to use the `eio_future_scheduler`.
 * It checks if the associated Eo object is still alive before creating
 * the promise.
 *
 * @param obj The Eo object to associate with the promise.
 * @return A new Eina_Promise, or NULL if the object is not alive or on error.
 */
static Eina_Promise *
eio_promise_new(const Eo *obj)
{
   if (!efl_alive_get(obj)) return NULL;

   return eina_promise_new(&eio_future_scheduler, eio_dummy_cancel, NULL);
}

/* Helper functions */
/**
 * @brief Callback for successful EIO file operations that return a file length.
 *
 * This function is typically used as the `Eio_Done_Cb`. It resolves the
 * associated Eina_Promise with the file length (handler->length) as a uint64.
 *
 * @param data The Eina_Promise to resolve.
 * @param handler The Eio_File handle for the completed operation.
 */
static void
_future_file_done_cb(void *data, Eio_File *handler)
{
   Eina_Promise *p = data;

   eina_promise_resolve(p, eina_value_uint64_init(handler->length));
   eio_process_entry();
}

/**
 * @brief Callback for failed EIO file operations.
 *
 * This function is typically used as the `Eio_Error_Cb`. It rejects the
 * associated Eina_Promise with the given error code. If the error code is 0,
 * it assumes the promise was cancelled and does nothing.
 *
 * @param data The Eina_Promise to reject.
 * @param handler The Eio_File handle for the failed operation (unused).
 * @param error The error code.
 */
static void
_future_file_error_cb(void *data,
                      Eio_File *handler EINA_UNUSED,
                      int error)
{
   Eina_Promise *p = data;

   // error == 0 -> promise was cancelled, no need to reject it anymore
   if (error != 0) eina_promise_reject(p, error);
   eio_process_entry();
}

/* Basic listing callbacks */
/**
 * @brief Callback for EIO operations that gather a list of strings (e.g., file names).
 *
 * This function is used as an `Eio_Main_Direct_Cb` or similar. It retrieves
 * a user-provided callback (`EflIoPath`) and associated data from thread-local storage
 * and invokes it with the gathered array of strings.
 * It then cleans up the string array.
 *
 * @param data User data (unused in this specific callback, context is from thread-local).
 * @param handler The Eio_File handle for the operation.
 * @param gather An Eina_Array containing Eina_Stringshare* elements (file paths).
 *               Example: `gather` might contain ["file1.txt", "file2.png", "subdir"].
 */
static void
_future_string_cb(void *data EINA_UNUSED, Eio_File *handler, Eina_Array *gather)
{
   EflIoPath paths = ecore_thread_local_data_find(handler->thread, ".paths");
   void *paths_data = ecore_thread_local_data_find(handler->thread, ".paths_data");
   Eina_Stringshare *s;

   if (!paths) goto end;

   paths(paths_data, gather);

   // Cleanup strings, accessor and array
 end:
   while ((s = eina_array_pop(gather)))
     eina_stringshare_del(s);
   eina_array_free(gather);
}

/* Direct listing callbacks */
/**
 * @brief Callback for EIO operations that gather detailed file information.
 *
 * This function is used as an `Eio_Main_Direct_Cb` or similar for direct listing
 * operations (like `direct_ls`, `stat_ls`). It retrieves a user-provided callback
 * (`EflIoDirectInfo`) and associated data from thread-local storage and invokes it
 * with the gathered array of `Eio_File_Direct_Info` structures.
 * It then cleans up the array of info structures.
 *
 * @param data User data (unused in this specific callback, context is from thread-local).
 * @param handler The Eio_File handle for the operation.
 * @param gather An Eina_Array containing `Eio_File_Direct_Info*` elements.
 *               Each `Eio_File_Direct_Info` contains path, name, and stat information.
 *               Example: `gather` might contain:
 *               [
 *                 { path="dir/file1.txt", name="file1.txt", type=EINA_FILE_REG, ... },
 *                 { path="dir/subdir", name="subdir", type=EINA_FILE_DIR, ... }
 *               ]
 */
static void
_future_file_info_cb(void *data EINA_UNUSED, Eio_File *handler, Eina_Array *gather)
{
   EflIoDirectInfo info = ecore_thread_local_data_find(handler->thread, ".info");
   void *info_data = ecore_thread_local_data_find(handler->thread, ".info_data");
   Eio_File_Direct_Info *d;

   if (!info) goto end;
   if (ecore_thread_check(handler->thread)) goto end;

   info(info_data, gather);

 end:
   while ((d = eina_array_pop(gather)))
     eio_direct_info_free(d);
   eina_array_free(gather);
}

/* Method implementations */
/**
 * @brief Performs a direct (non-recursive or recursive) listing of a directory,
 *        retrieving detailed file information.
 *
 * This implements the `efl_io_manager_direct_ls` method. It creates an Eina_Promise
 * and initiates an EIO operation (`_eio_file_direct_ls` or `_eio_dir_direct_ls`).
 * The user-provided `info` callback will be called for batches of `Eio_File_Direct_Info`
 * structures.
 *
 * @param obj The Efl_Io_Manager object.
 * @param pd Private data of the Efl_Io_Manager (unused).
 * @param path The directory path to list.
 * @param recursive If EINA_TRUE, list recursively. Otherwise, list non-recursively.
 * @param info_data User data to be passed to the `info` callback.
 * @param info Callback function (`EflIoDirectInfo`) to process batches of file information.
 *             It receives `info_data` and an `Eina_Array` of `Eio_File_Direct_Info*`.
 * @param info_free_cb Callback to free `info_data` when the operation completes.
 * @return An Eina_Future that resolves with the total number of entries processed (uint64)
 *         upon completion, or rejects with an error code.
 */
static Eina_Future *
_efl_io_manager_direct_ls(const Eo *obj,
                          Efl_Io_Manager_Data *pd EINA_UNUSED,
                          const char *path,
                          Eina_Bool recursive,
                          void *info_data, EflIoDirectInfo info, Eina_Free_Cb info_free_cb)
{
   Eina_Promise *p;
   Eina_Future *future;
   Eio_File *h;

   p = eio_promise_new(obj);
   if (!p) return NULL;
   future = eina_future_new(p);

   if (!recursive)
     {
        h = _eio_file_direct_ls(path,
                                _future_file_info_cb,
                                _future_file_done_cb,
                                _future_file_error_cb,
                                p);
     }
   else
     {
        h = _eio_dir_direct_ls(path,
                               _future_file_info_cb,
                               _future_file_done_cb,
                               _future_file_error_cb,
                               p);
     }
   if (!h) goto end;

   ecore_thread_local_data_add(h->thread, ".info", info, NULL, EINA_TRUE);
   ecore_thread_local_data_add(h->thread, ".info_data", info_data, info_free_cb, EINA_TRUE);

   return _efl_io_manager_future(obj, future, h);

 end:
   return future;
}

/**
 * @brief Performs a direct (non-recursive or recursive) listing of a directory,
 *        retrieving stat information for each file.
 *
 * This implements the `efl_io_manager_stat_ls` method. Similar to `direct_ls`,
 * but uses EIO functions that specifically focus on stat information (`_eio_file_stat_ls`
 * or `_eio_dir_stat_ls`).
 *
 * @param obj The Efl_Io_Manager object.
 * @param pd Private data of the Efl_Io_Manager (unused).
 * @param path The directory path to list.
 * @param recursive If EINA_TRUE, list recursively. Otherwise, list non-recursively.
 * @param info_data User data to be passed to the `info` callback.
 * @param info Callback function (`EflIoDirectInfo`) to process batches of file information
 *             (containing stat details). It receives `info_data` and an `Eina_Array` of
 *             `Eio_File_Direct_Info*`.
 * @param info_free_cb Callback to free `info_data` when the operation completes.
 * @return An Eina_Future that resolves with the total number of entries processed (uint64)
 *         upon completion, or rejects with an error code.
 */
static Eina_Future *
_efl_io_manager_stat_ls(const Eo *obj,
                        Efl_Io_Manager_Data *pd EINA_UNUSED,
                        const char *path,
                        Eina_Bool recursive,
                        void *info_data, EflIoDirectInfo info, Eina_Free_Cb info_free_cb)
{
   Eina_Promise *p;
   Eina_Future *future;
   Eio_File *h;

   p = eio_promise_new(obj);
   if (!p) return NULL;
   future = eina_future_new(p);

   if (!recursive)
     {
        h = _eio_file_stat_ls(path,
                              _future_file_info_cb,
                              _future_file_done_cb,
                              _future_file_error_cb,
                              p);
     }
   else
     {
        h = _eio_dir_stat_ls(path,
                             _future_file_info_cb,
                             _future_file_done_cb,
                             _future_file_error_cb,
                             p);
     }
   if (!h) goto end;

   ecore_thread_local_data_add(h->thread, ".info", info, NULL, EINA_TRUE);
   ecore_thread_local_data_add(h->thread, ".info_data", info_data, info_free_cb, EINA_TRUE);

   return _efl_io_manager_future(obj, future, h);

 end:
   return future;
}

/**
 * @brief Performs a basic listing of a directory, retrieving only file names.
 *
 * This implements the `efl_io_manager_ls` method. It uses `_eio_file_ls`
 * to get a list of file names. The user-provided `paths` callback will be
 * called for batches of path strings.
 *
 * @param obj The Efl_Io_Manager object.
 * @param pd Private data of the Efl_Io_Manager (unused).
 * @param path The directory path to list.
 * @param paths_data User data to be passed to the `paths` callback.
 * @param paths Callback function (`EflIoPath`) to process batches of file names.
 *              It receives `paths_data` and an `Eina_Array` of `Eina_Stringshare*`.
 * @param paths_free_cb Callback to free `paths_data` when the operation completes.
 * @return An Eina_Future that resolves with the total number of entries processed (uint64)
 *         upon completion, or rejects with an error code.
 */
static Eina_Future *
_efl_io_manager_ls(const Eo *obj,
                   Efl_Io_Manager_Data *pd EINA_UNUSED,
                   const char *path,
                   void *paths_data, EflIoPath paths, Eina_Free_Cb paths_free_cb)
{
   Eina_Promise *p;
   Eina_Future *future;
   Eio_File *h;

   p = eio_promise_new(obj);
   if (!p) return NULL;
   future = eina_future_new(p);

   h = _eio_file_ls(path,
                    _future_string_cb,
                    _future_file_done_cb,
                    _future_file_error_cb,
                    p);
   if (!h) goto end;

   ecore_thread_local_data_add(h->thread, ".paths", paths, NULL, EINA_TRUE);
   ecore_thread_local_data_add(h->thread, ".paths_data", paths_data, paths_free_cb, EINA_TRUE);

   return _efl_io_manager_future(obj, future, h);

 end:
   return future;
}

/* Stat function */
/**
 * @brief Callback for successful EIO stat operations.
 *
 * This function is used as the `Eio_Done_Cb` for `eio_file_direct_stat`.
 * It resolves the associated Eina_Promise with an Eina_Value containing
 * the `Eina_Stat` structure.
 *
 * @param data The Eina_Promise to resolve.
 * @param handle The Eio_File handle for the completed operation (unused).
 * @param st A pointer to the `Eina_Stat` structure containing file status information.
 */
static void
_file_stat_done_cb(void *data, Eio_File *handle EINA_UNUSED, const Eina_Stat *st)
{
   const Eina_Value_Struct value = { _eina_stat_desc(), (void*) st };
   Eina_Promise *p = data;
   Eina_Value r = EINA_VALUE_EMPTY;

   if (!eina_value_setup(&r, EINA_VALUE_TYPE_STRUCT))
     goto on_error;
   if (!eina_value_pset(&r, &value))
     goto on_error;

   eina_promise_resolve(p, r);
   eio_process_entry();

   return ;

 on_error:
   eina_value_flush(&r);
   eina_promise_reject(p, eina_error_get());
   eio_process_entry();
}

/**
 * @brief Retrieves status information (stat) for a single file or directory.
 *
 * This implements the `efl_io_manager_stat` method. It uses `eio_file_direct_stat`
 * to perform the operation.
 *
 * @param obj The Efl_Io_Manager object.
 * @param pd Private data of the Efl_Io_Manager (unused).
 * @param path The path to the file or directory.
 * @return An Eina_Future that resolves with an Eina_Value containing the `Eina_Stat`
 *         structure upon success, or rejects with an error code.
 */
static Eina_Future *
_efl_io_manager_stat(const Eo *obj,
                     Efl_Io_Manager_Data *pd EINA_UNUSED,
                     const char *path)
{
   Eina_Promise *p;
   Eina_Future *future;
   Eio_File *h;

   p = eio_promise_new(obj);
   if (!p) return NULL;
   future = eina_future_new(p);

   h = eio_file_direct_stat(path,
                            _file_stat_done_cb,
                            _future_file_error_cb,
                            p);
   if (!h) goto end;

   return _efl_io_manager_future(obj, future, h);

 end:
   return future;
}

/* eXtended attribute manipulation */

/**
 * @brief Lists the extended attributes of a file or directory.
 *
 * This implements the `efl_io_manager_xattr_ls` method. It uses `_eio_file_xattr`
 * to retrieve the names of extended attributes. The user-provided `paths` callback
 * will be called for batches of attribute names.
 *
 * @param obj The Efl_Io_Manager object.
 * @param pd Private data of the Efl_Io_Manager (unused).
 * @param path The path to the file or directory.
 * @param paths_data User data to be passed to the `paths` callback.
 * @param paths Callback function (`EflIoPath`) to process batches of attribute names.
 *              It receives `paths_data` and an `Eina_Array` of `Eina_Stringshare*`.
 * @param paths_free_cb Callback to free `paths_data` when the operation completes.
 * @return An Eina_Future that resolves with the total number of attributes (uint64)
 *         upon completion, or rejects with an error code.
 */
static Eina_Future *
_efl_io_manager_xattr_ls(const Eo *obj,
                         Efl_Io_Manager_Data *pd EINA_UNUSED,
                         const char *path,
                         void *paths_data, EflIoPath paths, Eina_Free_Cb paths_free_cb)
{
   Eina_Promise *p;
   Eina_Future *future;
   Eio_File *h;

   p = eio_promise_new(obj);
   if (!p) return NULL;
   future = eina_future_new(p);

   h = _eio_file_xattr(path,
                       _future_string_cb,
                       _future_file_done_cb,
                       _future_file_error_cb,
                       p);
   if (!h) goto end;

   // There is no race condition here as all the callback are called in the main loop after this
   ecore_thread_local_data_add(h->thread, ".paths", paths, NULL, EINA_TRUE);
   ecore_thread_local_data_add(h->thread, ".paths_data", paths_data, paths_free_cb, EINA_TRUE);

   return _efl_io_manager_future(obj, future, h);

 end:
   return efl_future_then(obj, future);;
}

/**
 * @brief Callback for successful EIO operations that return raw data (e.g., xattr value).
 *
 * This function is used as a done callback (like `Eio_Xattr_Get_Cb`).
 * It resolves the associated Eina_Promise with an Eina_Value of type BLOB,
 * containing a copy of the received data.
 *
 * @param data The Eina_Promise to resolve.
 * @param handler The Eio_File handle for the completed operation (unused).
 * @param attr_data Pointer to the raw data (e.g., extended attribute value).
 * @param size Size of the `attr_data` in bytes.
 */
static void
_future_file_done_data_cb(void *data, Eio_File *handler EINA_UNUSED, const char *attr_data, unsigned int size)
{
   Eina_Promise *p = data;
   Eina_Value_Blob blob = { EINA_VALUE_BLOB_OPERATIONS_MALLOC, NULL, size };
   Eina_Value v = EINA_VALUE_EMPTY;
   char *tmp;

   tmp = malloc(size);
   memcpy(tmp, attr_data, size);
   blob.memory = tmp;

   eina_value_setup(&v, EINA_VALUE_TYPE_BLOB);
   eina_value_set(&v, &blob);
   eina_promise_resolve(p, v);
   eio_process_entry();
}

/**
 * @brief Sets an extended attribute on a file or directory.
 *
 * This implements the `efl_io_manager_xattr_set` method. It uses `eio_file_xattr_set`
 * to perform the operation.
 *
 * @param obj The Efl_Io_Manager object.
 * @param pd Private data of the Efl_Io_Manager (unused).
 * @param path The path to the file or directory.
 * @param attribute The name of the extended attribute to set.
 * @param data An Eina_Binbuf containing the value of the attribute.
 * @param flags Flags for the xattr operation (e.g., EINA_XATTR_CREATE, EINA_XATTR_REPLACE).
 * @return An Eina_Future that resolves with 0 (uint64) upon success,
 *         or rejects with an error code.
 */
static Eina_Future *
_efl_io_manager_xattr_set(Eo *obj,
                          Efl_Io_Manager_Data *pd EINA_UNUSED,
                          const char *path,
                          const char *attribute,
                          Eina_Binbuf *data,
                          Eina_Xattr_Flags flags)
{
   Eina_Promise *p;
   Eina_Future *future;
   Eio_File *h;

   p = eio_promise_new(obj);
   if (!p) return NULL;
   future = eina_future_new(p);

   h = eio_file_xattr_set(path, attribute,
                          (const char *) eina_binbuf_string_get(data),
                          eina_binbuf_length_get(data),
                          flags,
                          _future_file_done_cb,
                          _future_file_error_cb,
                          p);
   if (!h) goto end;

   return _efl_io_manager_future(obj, future, h);

 end:
   return future;
}

/**
 * @brief Gets the value of an extended attribute from a file or directory.
 *
 * This implements the `efl_io_manager_xattr_get` method. It uses `eio_file_xattr_get`
 * to retrieve the attribute's value.
 *
 * @param obj The Efl_Io_Manager object.
 * @param pd Private data of the Efl_Io_Manager (unused).
 * @param path The path to the file or directory.
 * @param attribute The name of the extended attribute to get.
 * @return An Eina_Future that resolves with an Eina_Value of type BLOB containing
 *         the attribute's data upon success, or rejects with an error code.
 */
static Eina_Future *
_efl_io_manager_xattr_get(const Eo *obj,
                          Efl_Io_Manager_Data *pd EINA_UNUSED,
                          const char *path,
                          const char *attribute)
{
   Eina_Promise *p;
   Eina_Future *future;
   Eio_File *h;

   p = eio_promise_new(obj);
   if (!p) return NULL;
   future = eina_future_new(p);

   h = eio_file_xattr_get(path, attribute,
                          _future_file_done_data_cb,
                          _future_file_error_cb,
                          p);
   if (!h) goto end;

   return _efl_io_manager_future(obj, future, h);

 end:
   return future;
}

/**
 * @brief Callback for successful EIO file open operations.
 *
 * This function is used as the `Eio_Open_Cb`. It resolves the associated
 * Eina_Promise with an Eina_Value containing the opened `Eina_File` handle.
 *
 * @param data The Eina_Promise to resolve.
 * @param handler The Eio_File handle for the completed operation (unused).
 * @param file The opened `Eina_File` handle.
 */
static void
_future_file_open_cb(void *data, Eio_File *handler EINA_UNUSED, Eina_File *file)
{
   Eina_Promise *p = data;
   Eina_Value v = EINA_VALUE_EMPTY;

   eina_value_setup(&v, EINA_VALUE_TYPE_FILE);
   eina_value_set(&v, file);
   eina_promise_resolve(p, v);
   eio_process_entry();
}

/**
 * @brief Opens a file.
 *
 * This implements the `efl_io_manager_open` method. It uses `eio_file_open`
 * to perform the operation.
 *
 * @param obj The Efl_Io_Manager object.
 * @param pd Private data of the Efl_Io_Manager (unused).
 * @param path The path to the file to open.
 * @param shared If EINA_TRUE, open the file in shared mode.
 * @return An Eina_Future that resolves with an Eina_Value containing the
 *         `Eina_File` handle upon success, or rejects with an error code.
 */
static Eina_Future *
_efl_io_manager_open(const Eo *obj,
                     Efl_Io_Manager_Data *pd EINA_UNUSED,
                     const char *path,
                     Eina_Bool shared)
{
   Eina_Promise *p;
   Eina_Future *future;
   Eio_File *h;

   p = eio_promise_new(obj);
   if (!p) return NULL;
   future = eina_future_new(p);

   h = eio_file_open(path, shared,
                     _future_file_open_cb,
                     _future_file_error_cb,
                     p);
   if (!h) goto end;

   return _efl_io_manager_future(obj, future, h);

 end:
   return future;
}

/**
 * @brief Closes an opened file.
 *
 * This implements the `efl_io_manager_close` method. It uses `eio_file_close`
 * to perform the operation.
 *
 * @param obj The Efl_Io_Manager object.
 * @param pd Private data of the Efl_Io_Manager (unused).
 * @param file The `Eina_File` handle to close.
 * @return An Eina_Future that resolves with 0 (uint64) upon success,
 *         or rejects with an error code.
 */
static Eina_Future *
_efl_io_manager_close(const Eo *obj,
                      Efl_Io_Manager_Data *pd EINA_UNUSED,
                      Eina_File *file)
{
   Eina_Promise *p;
   Eina_Future *future;
   Eio_File *h;

   p = eio_promise_new(obj);
   if (!p) return NULL;
   future = eina_future_new(p);

   h = eio_file_close(file,
                      _future_file_done_cb,
                      _future_file_error_cb,
                      p);
   if (!h) goto end;

   return _efl_io_manager_future(obj, future, h);

 end:
   return future;
}

#include "efl_io_manager.eo.c"
