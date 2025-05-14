/**
 * @file
 * @brief This file implements the Efl_Loop_Handler class, which is responsible
 * for managing event handling for file descriptors and Windows handles within
 * an Efl_Loop.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Ecore.h>

#include "ecore_private.h"

#define MY_CLASS EFL_LOOP_HANDLER_CLASS

//////////////////////////////////////////////////////////////////////////

typedef struct _Efl_Loop_Handler_Data Efl_Loop_Handler_Data;

/**
 * @brief Private data structure for Efl_Loop_Handler instances.
 */
struct _Efl_Loop_Handler_Data
{
   Eo *loop;                         /**< The parent Efl_Loop object. */
   Efl_Loop_Data *loop_data;         /**< Private data of the parent Efl_Loop. */
   Ecore_Fd_Handler *handler_fd;     /**< Ecore file descriptor handler. */
   Ecore_Win32_Handler *handler_win32; /**< Ecore Windows handle handler. */

   void *win32;                      /**< Windows handle being watched. */
   int fd;                           /**< File descriptor being watched. */

   /**
    * @brief Reference counts for different event types.
    * Used to determine if the underlying Ecore_Fd_Handler needs to listen
    * for specific events (read, write, error, buffer, prepare).
    */
   struct {
      unsigned short read;           /**< Number of listeners for read events. */
      unsigned short write;          /**< Number of listeners for write events. */
      unsigned short error;          /**< Number of listeners for error events. */
      unsigned short buffer;         /**< Number of listeners for buffer events. */
      unsigned short prepare;        /**< Number of listeners for prepare events. */
   } references;

   Efl_Loop_Handler_Flags flags : 8; /**< Flags indicating active event types (read, write, error). */
   Eina_Bool file : 1;               /**< EINA_TRUE if 'fd' refers to a regular file, EINA_FALSE for sockets/pipes. */

   Eina_Bool constructed : 1;        /**< EINA_TRUE if the object has been constructed. */
   Eina_Bool finalized : 1;          /**< EINA_TRUE if the object has been finalized. */
};

//////////////////////////////////////////////////////////////////////////

static Eina_Bool _cb_handler_fd(void *data, Ecore_Fd_Handler *fd_handler);
static Eina_Bool _cb_handler_buffer(void *data, Ecore_Fd_Handler *fd_handler);
static Eina_Bool _cb_handler_win32(void *data, Ecore_Win32_Handler *win32_handler);
static void      _cb_handler_prepare(void *data, Ecore_Fd_Handler *fd_handler);

//////////////////////////////////////////////////////////////////////////

/**
 * @internal
 * @brief Clears any existing Ecore fd or win32 handlers.
 *
 * This function is called when the handler is no longer needed, for example,
 * when the fd/handle is changed or the object is destroyed.
 *
 * @param pd Private data of the Efl_Loop_Handler.
 */
static void
_handler_clear(Efl_Loop_Handler_Data *pd)
{
   Eo *obj = pd->loop;
   Efl_Loop_Data *loop = pd->loop_data;

   if (pd->handler_fd)
     {
        _ecore_main_fd_handler_del(obj, loop, pd->handler_fd);
        pd->handler_fd = NULL;
     }
   else if (pd->handler_win32)
     {
        _ecore_main_win32_handler_del(obj, loop, pd->handler_win32);
        pd->handler_win32 = NULL;
     }
}

/**
 * @internal
 * @brief Determines the Ecore_Fd_Handler_Flags based on active flags and reference counts.
 *
 * This function translates the Efl_Loop_Handler_Flags and event reference counts
 * into the appropriate Ecore_Fd_Handler_Flags. An event type is active only if
 * its corresponding flag is set in `pd->flags` AND its reference count is greater than 0.
 *
 * @param pd Private data of the Efl_Loop_Handler.
 * @return The combined Ecore_Fd_Handler_Flags.
 */
static Ecore_Fd_Handler_Flags
_handler_flags_get(Efl_Loop_Handler_Data *pd)
{
   return (((pd->flags & EFL_LOOP_HANDLER_FLAGS_READ) &&
            (pd->references.read > 0)) ? ECORE_FD_READ  : 0) |
          (((pd->flags & EFL_LOOP_HANDLER_FLAGS_WRITE) &&
            (pd->references.write > 0)) ? ECORE_FD_WRITE  : 0) |
          (((pd->flags & EFL_LOOP_HANDLER_FLAGS_ERROR) &&
            (pd->references.error > 0)) ? ECORE_FD_ERROR  : 0);
}

/**
 * @internal
 * @brief Updates the active state of the Ecore_Fd_Handler.
 *
 * This function sets the active flags (read, write, error) on the
 * Ecore_Fd_Handler based on the current `pd->flags` and reference counts.
 * It also sets or clears the prepare callback based on `pd->references.prepare`.
 *
 * @param obj The Efl_Loop_Handler object.
 * @param pd Private data of the Efl_Loop_Handler.
 */
static void
_handler_active_update(Eo *obj, Efl_Loop_Handler_Data *pd)
{
   Ecore_Fd_Handler_Flags flags = _handler_flags_get(pd);

   ecore_main_fd_handler_active_set(pd->handler_fd, flags);
   if (pd->references.prepare)
     ecore_main_fd_handler_prepare_callback_set
       (pd->handler_fd, _cb_handler_prepare, obj);
   else
     ecore_main_fd_handler_prepare_callback_set
       (pd->handler_fd, NULL, NULL);
}

/**
 * @internal
 * @brief Resets and potentially recreates the Ecore fd or win32 handler.
 *
 * This function is the core logic for managing the underlying Ecore handlers.
 * It is called when:
 * - The fd or win32 handle is set/changed.
 * - Event callback references change (add/del).
 * - Active flags change.
 * - The object is finalized or its parent changes.
 *
 * It ensures that an Ecore handler is active if a valid fd/handle is set
 * and the object is constructed and finalized. It also updates the
 * Ecore_Fd_Handler's active flags and buffer/prepare callbacks.
 *
 * @param obj The Efl_Loop_Handler object.
 * @param pd Private data of the Efl_Loop_Handler.
 */
static void
_handler_reset(Eo *obj, Efl_Loop_Handler_Data *pd)
{
   if ((pd->fd < 0) && (!pd->win32))
     {
        _handler_clear(pd);
        return;
     }

   // Do not create handlers until the object is fully constructed and finalized,
   // and a loop provider is available.
   if ((!pd->constructed) || (!pd->finalized)) return;

   if (pd->fd >= 0) // Handle file descriptors
     {
        Ecore_Fd_Cb buffer_func = NULL;
        void *buffer_data = NULL;

        // Set up buffer callback if there are references
        if (pd->references.buffer > 0)
          {
             buffer_func = _cb_handler_buffer;
             buffer_data = obj;
          }

        if (pd->handler_fd) // Existing fd handler, update its active state
          _handler_active_update(obj, pd);
        else if (pd->loop_data) // No existing fd handler, create a new one
          {
             pd->handler_fd = _ecore_main_fd_handler_add
               (pd->loop, pd->loop_data, obj, pd->fd, _handler_flags_get(pd),
                _cb_handler_fd, obj, buffer_func, buffer_data,
                pd->file ? EINA_TRUE : EINA_FALSE);
             if (pd->handler_fd) _handler_active_update(obj, pd); // Update active state of new handler
          }
     }
   else if (pd->win32 && pd->loop_data) // Handle Win32 handles
     {
        // If a win32 handler already exists, it's implicitly cleared by _handler_clear
        // if fd was previously set. If win32 handle changes, _handler_clear is called before this.
        // So, we only need to add a new one if it doesn't exist.
        if (!pd->handler_win32)
          {
             pd->handler_win32 = _ecore_main_win32_handler_add
               (pd->loop, pd->loop_data, obj, pd->win32, _cb_handler_win32, obj);
          }
     }
}

/**
 * @internal
 * @brief Updates reference counts for events based on callback additions or deletions.
 *
 * This function is called when an event callback is added or removed for
 * one of the EFL_LOOP_HANDLER_EVENT_* types. It increments or decrements
 * the corresponding counter in `pd->references`.
 *
 * @param pd Private data of the Efl_Loop_Handler.
 * @param event The Efl_Event structure containing callback information.
 *              The `event->info` is expected to be an array of
 *              Efl_Callback_Array_Item_Full.
 *              Example:
 *              For EFL_LOOP_HANDLER_EVENT_READ, the array might look like:
 *              `static const Efl_Callback_Array_Item_Full _my_event_desc_array[] = {
 *                  { EFL_LOOP_HANDLER_EVENT_READ, _my_read_cb_func },
 *                  { NULL, NULL }
 *              };`
 * @param increment Value to add to the reference count (1 for add, -1 for delete).
 * @return EINA_TRUE if any reference count changed, EINA_FALSE otherwise.
 *         This indicates whether _handler_reset() needs to be called.
 */
static Eina_Bool
_event_references_update(Efl_Loop_Handler_Data *pd, const Efl_Event *event, int increment)
{
   const Efl_Callback_Array_Item_Full *array = event->info;
   int i;
   Eina_Bool need_reset = EINA_FALSE;

   for (i = 0; array[i].desc != NULL; i++)
     {
#define REFERENCES_MAP(_desc, _refs) \
   if (array[i].desc == _desc) { \
      pd->references._refs += increment; \
      need_reset = EINA_TRUE; \
      continue; \
   }
        REFERENCES_MAP(EFL_LOOP_HANDLER_EVENT_READ,    read);
        REFERENCES_MAP(EFL_LOOP_HANDLER_EVENT_WRITE,   write);
        REFERENCES_MAP(EFL_LOOP_HANDLER_EVENT_ERROR,   error);
        REFERENCES_MAP(EFL_LOOP_HANDLER_EVENT_BUFFER,  buffer);
        REFERENCES_MAP(EFL_LOOP_HANDLER_EVENT_PREPARE, prepare);
     }
   return need_reset;
}

//////////////////////////////////////////////////////////////////////////

/**
 * @internal
 * @brief Callback executed by Ecore when a file descriptor event occurs.
 *
 * This function is registered with `ecore_main_fd_handler_add`.
 * It checks which type of event (read, write, error) occurred on the fd
 * and emits the corresponding Efl_Loop_Handler event.
 *
 * @param data User data, which is the Efl_Loop_Handler object.
 * @param fd_handler The Ecore_Fd_Handler that triggered the callback.
 * @return ECORE_CALLBACK_RENEW to keep the handler active.
 */
static Eina_Bool
_cb_handler_fd(void *data, Ecore_Fd_Handler *fd_handler EINA_UNUSED)
{
   Eo *obj = data;

   efl_ref(obj);
   if (ecore_main_fd_handler_active_get(fd_handler, ECORE_FD_READ))
     efl_event_callback_call(obj, EFL_LOOP_HANDLER_EVENT_READ, NULL);
   if (ecore_main_fd_handler_active_get(fd_handler, ECORE_FD_WRITE))
     efl_event_callback_call(obj, EFL_LOOP_HANDLER_EVENT_WRITE, NULL);
   if (ecore_main_fd_handler_active_get(fd_handler, ECORE_FD_ERROR))
     efl_event_callback_call(obj, EFL_LOOP_HANDLER_EVENT_ERROR, NULL);
   efl_unref(obj);
   return ECORE_CALLBACK_RENEW;
}

/**
 * @internal
 * @brief Callback executed by Ecore when a buffer event occurs for an fd.
 *
 * This function is registered with `ecore_main_fd_handler_add` as the
 * buffer callback. It emits the EFL_LOOP_HANDLER_EVENT_BUFFER event.
 *
 * @param data User data, which is the Efl_Loop_Handler object.
 * @param fd_handler The Ecore_Fd_Handler that triggered the callback.
 * @return ECORE_CALLBACK_RENEW to keep the handler active.
 */
static Eina_Bool
_cb_handler_buffer(void *data, Ecore_Fd_Handler *fd_handler EINA_UNUSED)
{
   Eo *obj = data;

   efl_event_callback_call(obj, EFL_LOOP_HANDLER_EVENT_BUFFER, NULL);
   return ECORE_CALLBACK_RENEW;
}

/**
 * @internal
 * @brief Callback executed by Ecore when a Windows handle event occurs.
 *
 * This function is registered with `ecore_main_win32_handler_add`.
 * For Win32 handles, Ecore only signals a generic event, which is mapped
 * to EFL_LOOP_HANDLER_EVENT_READ.
 *
 * @param data User data, which is the Efl_Loop_Handler object.
 * @param win32_handler The Ecore_Win32_Handler that triggered the callback.
 * @return ECORE_CALLBACK_RENEW to keep the handler active.
 */
static Eina_Bool
_cb_handler_win32(void *data, Ecore_Win32_Handler *win32_handler EINA_UNUSED)
{
   Eo *obj = data;

   efl_event_callback_call(obj, EFL_LOOP_HANDLER_EVENT_READ, NULL);
   return ECORE_CALLBACK_RENEW;
}

/**
 * @internal
 * @brief Callback executed by Ecore before checking file descriptors in a loop iteration.
 *
 * This function is registered with `ecore_main_fd_handler_prepare_callback_set`.
 * It emits the EFL_LOOP_HANDLER_EVENT_PREPARE event. This allows applications
 * to perform actions (e.g., flushing buffers) before the fd is checked for readiness.
 *
 * @param data User data, which is the Efl_Loop_Handler object.
 * @param fd_handler The Ecore_Fd_Handler associated with this prepare callback.
 */
static void
_cb_handler_prepare(void *data, Ecore_Fd_Handler *fd_handler EINA_UNUSED)
{
   Eo *obj = data;

   efl_event_callback_call(obj, EFL_LOOP_HANDLER_EVENT_PREPARE, NULL);
}

/**
 * @internal
 * @brief Callback invoked when an Efl_Event_Callback is added to the Efl_Loop_Handler.
 *
 * This function updates the reference counts for the specific event type
 * (read, write, error, buffer, prepare) and then calls `_handler_reset`
 * to potentially reconfigure the underlying Ecore handler.
 *
 * @param data User data, which is the Efl_Loop_Handler_Data for the object.
 * @param event The Efl_Event describing the callback being added.
 */
static void
_cb_event_callback_add(void *data, const Efl_Event *event)
{
   Efl_Loop_Handler_Data *pd = data;
   if (_event_references_update(pd, event, 1))
     _handler_reset(event->object, pd);
}

/**
 * @internal
 * @brief Callback invoked when an Efl_Event_Callback is removed from the Efl_Loop_Handler.
 *
 * This function updates the reference counts for the specific event type
 * (read, write, error, buffer, prepare) and then calls `_handler_reset`
 * to potentially reconfigure or remove the underlying Ecore handler.
 *
 * @param data User data, which is the Efl_Loop_Handler_Data for the object.
 * @param event The Efl_Event describing the callback being removed.
 */
static void
_cb_event_callback_del(void *data, const Efl_Event *event)
{
   Efl_Loop_Handler_Data *pd = data;
   if (_event_references_update(pd, event, -1))
     _handler_reset(event->object, pd);
}

//////////////////////////////////////////////////////////////////////////

/**
 * @internal
 * @brief Defines the array of callbacks for watching Efl_Event_Callback_Add and Efl_Event_Callback_Del.
 * This array is used with `efl_event_callback_array_add` to automatically
 * call `_cb_event_callback_add` and `_cb_event_callback_del` when
 * event listeners are added or removed for any of the EFL_LOOP_HANDLER_EVENT_* types.
 */
EFL_CALLBACKS_ARRAY_DEFINE(_event_callback_watch,
                          { EFL_EVENT_CALLBACK_ADD, _cb_event_callback_add },
                          { EFL_EVENT_CALLBACK_DEL, _cb_event_callback_del });

static void
_efl_loop_handler_active_set(Eo *obj, Efl_Loop_Handler_Data *pd, Efl_Loop_Handler_Flags flags)
{
   pd->flags = flags;
   _handler_reset(obj, pd);
}

static Efl_Loop_Handler_Flags
_efl_loop_handler_active_get(const Eo *obj EINA_UNUSED, Efl_Loop_Handler_Data *pd)
{
   return pd->flags;
}

static void
_efl_loop_handler_fd_set(Eo *obj, Efl_Loop_Handler_Data *pd, int fd)
{
   pd->fd = fd;
   pd->file = EINA_FALSE;
   pd->win32 = NULL;
   _handler_reset(obj, pd);
}

static int
_efl_loop_handler_fd_get(const Eo *obj EINA_UNUSED, Efl_Loop_Handler_Data *pd)
{
   if (pd->win32) return -1;
   return pd->file ? -1 : pd->fd;
}

static void
_efl_loop_handler_fd_file_set(Eo *obj, Efl_Loop_Handler_Data *pd, int fd)
{
   pd->fd = fd;
   pd->file = EINA_TRUE;
   pd->win32 = NULL;
   _handler_reset(obj, pd);
}

static int
_efl_loop_handler_fd_file_get(const Eo *obj EINA_UNUSED, Efl_Loop_Handler_Data *pd)
{
   if (pd->win32) return -1;
   return pd->file ? pd->fd : -1;
}

static void
_efl_loop_handler_win32_set(Eo *obj, Efl_Loop_Handler_Data *pd, void *handle)
{
   pd->fd = -1;
   pd->file = EINA_FALSE;
   pd->win32 = handle;
   _handler_reset(obj, pd);
}

static void *
_efl_loop_handler_win32_get(const Eo *obj EINA_UNUSED, Efl_Loop_Handler_Data *pd)
{
   return pd->win32;
}

static void
_efl_loop_handler_efl_object_parent_set(Eo *obj, Efl_Loop_Handler_Data *pd, Efl_Object *parent)
{
   efl_parent_set(efl_super(obj, MY_CLASS), parent);

   if ((!pd->constructed) || (!pd->finalized)) return;

   _handler_clear(pd);

   if (pd->loop)
     {
        pd->loop_data->fd_handlers_obj = eina_list_remove
          (pd->loop_data->fd_handlers_obj, obj);
        pd->loop = NULL;
        pd->loop_data = NULL;
     }

   if (parent == NULL) return;

   pd->loop = efl_provider_find(obj, EFL_LOOP_CLASS);
   pd->loop_data = efl_data_scope_get(pd->loop, EFL_LOOP_CLASS);
   if (pd->loop_data)
     pd->loop_data->fd_handlers_obj =
       eina_list_append(pd->loop_data->fd_handlers_obj, obj);
   _handler_reset(obj, pd);
}

static Efl_Object *
_efl_loop_handler_efl_object_constructor(Eo *obj, Efl_Loop_Handler_Data *pd)
{
   efl_constructor(efl_super(obj, MY_CLASS));
   efl_event_callback_array_add(obj, _event_callback_watch(), pd);
   pd->constructed = EINA_TRUE;
   return obj;
}

static Efl_Object *
_efl_loop_handler_efl_object_finalize(Eo *obj, Efl_Loop_Handler_Data *pd)
{
   pd->loop = efl_provider_find(obj, EFL_LOOP_CLASS);
   pd->loop_data = efl_data_scope_get(pd->loop, EFL_LOOP_CLASS);
   if (pd->loop_data)
     pd->loop_data->fd_handlers_obj =
       eina_list_append(pd->loop_data->fd_handlers_obj, obj);
   pd->finalized = EINA_TRUE;
   _handler_reset(obj, pd);
   return efl_finalize(efl_super(obj, MY_CLASS));
}

static void
_efl_loop_handler_efl_object_destructor(Eo *obj, Efl_Loop_Handler_Data *pd)
{
   if (pd->loop_data)
     pd->loop_data->fd_handlers_obj =
       eina_list_remove(pd->loop_data->fd_handlers_obj, obj);
   _handler_clear(pd);
   efl_destructor(efl_super(obj, MY_CLASS));
}

#include "efl_loop_handler.eo.c"
