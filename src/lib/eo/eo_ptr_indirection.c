#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "eo_ptr_indirection.h"

extern Eina_Thread _efl_object_main_thread; /**< Stores the main thread ID for EFL objects. */

//////////////////////////////////////////////////////////////////////////

Eina_TLS          _eo_table_data; /**< Thread-local storage key for Eo_Id_Data. Each thread gets its own instance. */
Eo_Id_Data       *_eo_table_data_shared = NULL; /**< Global pointer to the Eo_Id_Data structure for shared objects. Access is synchronized. */
Eo_Id_Table_Data *_eo_table_data_shared_data = NULL; /**< Global pointer to the Eo_Id_Table_Data for shared objects. Contains the actual tables and lock. */

//////////////////////////////////////////////////////////////////////////

/**
 * @internal
 * @brief Logs an error when an Eo pointer operation fails.
 *
 * This function is primarily a central point for logging pointer-related errors.
 * It uses eina_log for output and also calls _eo_log_obj_report for
 * more detailed object-specific logging if available.
 * Its existence facilitates setting a single breakpoint to catch all such errors.
 *
 * @param obj_id The Eo object ID that was being processed when the error occurred.
 * @param func_name The name of the function where the error was detected.
 * @param file The source file where the error was detected.
 * @param line The line number in the source file.
 * @param fmt A printf-style format string for the error message.
 * @param ... Variable arguments for the format string.
 */
void
_eo_pointer_error(const Eo *obj_id, const char *func_name, const char *file, int line, const char *fmt, ...)
{
   /* NOTE: this function exists to allow easy breakpoint on pointer errors */
   va_list args;
   va_start(args, fmt);
   eina_log_vprint(_eo_log_dom, EINA_LOG_LEVEL_ERR, file, func_name, line, fmt, args);
   va_end(args);
   _eo_log_obj_report((Eo_Id)obj_id, EINA_LOG_LEVEL_ERR, func_name, file, line);
}

/**
 * @internal
 * @brief Logs detailed information when an Eo ID is found to be invalid.
 *
 * This function is called by _eo_obj_pointer_get when an Eo ID cannot be
 * resolved to a valid object pointer. It provides comprehensive diagnostic
 * information, including the ID itself, whether it's an object or class,
 * the current thread, suspected reasons for invalidity (e.g., deleted,
 * wrong thread), domain information, generation count, and raw ID components.
 *
 * @param obj_id The invalid Eo ID.
 * @param data The Eo_Id_Data associated with the current context (thread-local or shared).
 * @param domain The domain extracted from the obj_id.
 * @param func_name The name of the function that attempted to resolve the pointer.
 * @param file The source file where the resolution was attempted.
 * @param line The line number in the source file.
 */
static void
_eo_obj_pointer_invalid(const Eo_Id obj_id,
                        Eo_Id_Data *data,
                        unsigned char domain,
                        const char *func_name,
                        const char *file,
                        int line)
{
   Eina_Thread thread = eina_thread_self();
   const char *tself = "main";
   const char *type = "object";
   const char *reason = "This ID has probably been deleted";
   char tbuf[128];
   if (obj_id & MASK_CLASS_TAG) type = "class";
   if (thread != _efl_object_main_thread)
     {
        snprintf(tbuf, sizeof(tbuf), "%p", (void *)thread);
        tself = tbuf;
     }

   if (!data->tables[(int)data->local_domain])
     reason = "This ID does not seem to belong to this thread";
   else if ((Efl_Id_Domain)domain == EFL_ID_DOMAIN_SHARED)
     reason = "This shared ID has probably been deleted";

   eina_log_print(_eo_log_dom, EINA_LOG_LEVEL_ERR,
                  file, func_name, line,
                  "Eo ID %p is not a valid %s. "
                  "Current thread: %s. "
                  "%s or this was never a valid %s ID. "
                  "(domain=%i, current_domain=%i, local_domain=%i, "
                  "available_domains=[%s %s %s %s], "
                  "generation=%lx, id=%lx, ref=%i)",
                  (void *)obj_id,
                  type,
                  tself,
                  reason,
                  type,
                  (int)domain,
                  (int)data->domain_stack[data->stack_top],
                  (int)data->local_domain,
                  (data->tables[0]) ? "0" : " ",
                  (data->tables[1]) ? "1" : " ",
                  (data->tables[2]) ? "2" : " ",
                  (data->tables[3]) ? "3" : " ",
                  (unsigned long)(obj_id & MASK_GENERATIONS),
                  (unsigned long)(obj_id >> SHIFT_ENTRY_ID) & (MAX_ENTRY_ID | MAX_TABLE_ID | MAX_MID_TABLE_ID),
                  (int)(obj_id >> REF_TAG_SHIFT) & 0x1);
   _eo_log_obj_report(obj_id, EINA_LOG_LEVEL_ERR, func_name, file, line);
}

/**
 * @internal
 * @brief Resolves an Eo_Id to an _Eo_Object pointer.
 *
 * This is the core function for converting an Eo object ID into a usable
 * memory pointer. It handles:
 * - NULL ID checks.
 * - Distinguishing between thread-local and shared domains.
 * - Cache lookups for frequently accessed objects.
 * - Navigating the multi-level ID tables to find the object entry.
 * - Generation counter checks to ensure the ID refers to the current instance
 *   of the object and not a stale one.
 * - Storing successfully resolved pointers in the cache.
 * - Locking for shared domain access (lock is taken here and expected to be
 *   released by _eo_obj_pointer_done() via EO_OBJ_DONE()).
 * - Error reporting via _eo_obj_pointer_invalid() for failed resolutions.
 *
 * @param obj_id The Eo ID to resolve.
 * @param func_name Name of the calling function (for error reporting).
 * @param file Source file of the calling function (for error reporting).
 * @param line Line number in the source file (for error reporting).
 * @return A pointer to the _Eo_Object if resolution is successful and the ID
 *         is valid; NULL otherwise. For shared objects, if successful,
 *         _eo_table_data_shared_data->obj_lock is held upon return.
 */
_Eo_Object *
_eo_obj_pointer_get(const Eo_Id obj_id, const char *func_name, const char *file, int line)
{
   _Eo_Id_Entry *entry;
   Generation_Counter generation;
   Table_Index mid_table_id, table_id, entry_id;
   Eo_Id tag_bit;
   Eo_Id_Data *data;
   Eo_Id_Table_Data *tdata;
   unsigned char domain;

   // NULL objects will just be sensibly ignored. not worth complaining
   // every single time.

   data = _eo_table_data_get();
   EINA_PREFETCH(&(data->tables[0]));
   domain = (obj_id >> SHIFT_DOMAIN) & MASK_DOMAIN;
   tdata = _eo_table_data_table_get(data, domain);
   if (EINA_UNLIKELY(!tdata)) goto err;
   _eo_cache_prefetch(tdata);

   if (EINA_LIKELY(domain != EFL_ID_DOMAIN_SHARED))
     {
        _Eo_Object *obj;

        obj = _eo_cache_find(tdata, obj_id);
        if (obj) return obj;

        mid_table_id = (obj_id >> SHIFT_MID_TABLE_ID) & MASK_MID_TABLE_ID;
        EINA_PREFETCH(&(tdata->eo_ids_tables[mid_table_id])); //prefetch for line 119
        table_id = (obj_id >> SHIFT_TABLE_ID) & MASK_TABLE_ID;
        EINA_PREFETCH((tdata->eo_ids_tables[mid_table_id] + table_id)); //prefetch for line 121
        entry_id = (obj_id >> SHIFT_ENTRY_ID) & MASK_ENTRY_ID;
        generation = obj_id & MASK_GENERATIONS;

        // get tag bit to check later down below - pipelining
        tag_bit = (obj_id) & MASK_OBJ_TAG;
        if (!obj_id) goto err_null;
        else if (!tag_bit) goto err;

        // Check the validity of the entry
        if (tdata->eo_ids_tables[mid_table_id])
          {
             _Eo_Ids_Table *tab = TABLE_FROM_IDS;
             EINA_PREFETCH_NOCACHE(tab); //prefetch for line 125
             if (tab)
               {
                  entry = &(tab->entries[entry_id]);
                  if (entry->active && (entry->generation == generation))
                    {
                       // Cache the result of that lookup
                       _eo_cache_store(tdata, obj_id, entry->ptr);
                       return entry->ptr;
                    }
               }
          }
        goto err;
     }
   else
     {
        _Eo_Object *obj;

        eina_lock_take(&(_eo_table_data_shared_data->obj_lock));
        // yes we return keeping the lock locked. that's why
        // you must call _eo_obj_pointer_done() wrapped
        // by EO_OBJ_DONE() to release
        obj = _eo_cache_find(tdata, obj_id);
        if (obj) return obj;

        mid_table_id = (obj_id >> SHIFT_MID_TABLE_ID) & MASK_MID_TABLE_ID;
        EINA_PREFETCH(&(tdata->eo_ids_tables[mid_table_id]));
        table_id = (obj_id >> SHIFT_TABLE_ID) & MASK_TABLE_ID;
        EINA_PREFETCH((tdata->eo_ids_tables[mid_table_id] + table_id));
        entry_id = (obj_id >> SHIFT_ENTRY_ID) & MASK_ENTRY_ID;
        generation = obj_id & MASK_GENERATIONS;

        // get tag bit to check later down below - pipelining
        tag_bit = (obj_id) & MASK_OBJ_TAG;
        if (!obj_id) goto err_shared_null;
        else if (!tag_bit) goto err_shared;

        // Check the validity of the entry
        if (tdata->eo_ids_tables[mid_table_id])
          {
             _Eo_Ids_Table *tab = TABLE_FROM_IDS;
             EINA_PREFETCH_NOCACHE(tab);

             if (tab)
               {
                  entry = &(tab->entries[entry_id]);
                  if (entry->active && (entry->generation == generation))
                    {
                       // Cache the result of that lookup
                       _eo_cache_store(tdata, obj_id, entry->ptr);
                       // yes we return keeping the lock locked. that's why
                       // you must call _eo_obj_pointer_done() wrapped
                       // by EO_OBJ_DONE() to release
                       return entry->ptr;
                    }
               }
          }
        goto err_shared;
     }
err_shared_null:
   eina_lock_release(&(_eo_table_data_shared_data->obj_lock));
err_null:
   eina_log_print(_eo_log_dom,
                  EINA_LOG_LEVEL_DBG,
                  file, func_name, line,
                  "obj_id is NULL. Possibly unintended access?");
   return NULL;
err_shared:
   eina_lock_release(&(_eo_table_data_shared_data->obj_lock));
err:
   _eo_obj_pointer_invalid(obj_id, data, domain, func_name, file, line);
   return NULL;
}
