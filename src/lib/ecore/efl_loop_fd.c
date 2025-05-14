#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Ecore.h>

#include "ecore_private.h"

#define MY_CLASS EFL_LOOP_FD_CLASS

/**
 * @brief Private data for the Efl_Loop_Fd class.
 */
typedef struct _Efl_Loop_Fd_Data Efl_Loop_Fd_Data;
struct _Efl_Loop_Fd_Data
{
   Ecore_Fd_Handler *handler; /**< Ecore file descriptor handler */

   struct {
      unsigned int read;    /**< Number of references for read events */
      unsigned int write;   /**< Number of references for write events */
      unsigned int error;   /**< Number of references for error events */
   } references; /**< Keeps track of event listeners */

   int fd; /**< The file descriptor being monitored */

   Eina_Bool file : 1; /**< EINA_TRUE if 'fd' is a regular file, EINA_FALSE otherwise */
};

/**
 * @brief Callback function for Ecore_Fd_Handler.
 *
 * This function is invoked when there is activity on the monitored file descriptor.
 * It checks for read, write, or error conditions and triggers the corresponding
 * Efl events.
 *
 * @param data The Eo object (Efl_Loop_Fd instance).
 * @param fd_handler The Ecore_Fd_Handler that triggered the callback.
 * @return ECORE_CALLBACK_RENEW to keep the handler active.
 */
static Eina_Bool
_efl_loop_fd_read_cb(void *data, Ecore_Fd_Handler *fd_handler)
{
   Eo *obj = data;

   if (ecore_main_fd_handler_active_get(fd_handler, ECORE_FD_READ))
     {
        efl_event_callback_call(obj, EFL_LOOP_FD_EVENT_READ, NULL);
     }
   if (ecore_main_fd_handler_active_get(fd_handler, ECORE_FD_WRITE))
     {
        efl_event_callback_call(obj, EFL_LOOP_FD_EVENT_WRITE, NULL);
     }
   if (ecore_main_fd_handler_active_get(fd_handler, ECORE_FD_ERROR))
     {
        efl_event_callback_call(obj, EFL_LOOP_FD_EVENT_ERROR, NULL);
     }

   return ECORE_CALLBACK_RENEW;
}

/**
 * @brief Resets the Ecore_Fd_Handler based on current event references and fd.
 *
 * This function creates, updates, or deletes the Ecore_Fd_Handler
 * associated with the Efl_Loop_Fd object. It determines the necessary
 * Ecore_Fd_Handler flags (READ, WRITE, ERROR) based on the number
 * of active event listeners. If no listeners are active or the fd is invalid,
 * the handler is deleted.
 *
 * @param obj The Efl_Loop_Fd Eo object.
 * @param pd The private data of the Efl_Loop_Fd object.
 */
static void
_efl_loop_fd_reset(Eo *obj, Efl_Loop_Fd_Data *pd)
{
   int flags = 0;

   if (pd->fd < 0)
     {
        if (pd->handler)
          {
             ecore_main_fd_handler_del(pd->handler);
             pd->handler = NULL;
          }
        return;
     }
   flags |= pd->references.read > 0 ? ECORE_FD_READ : 0;
   flags |= pd->references.write > 0 ? ECORE_FD_WRITE : 0;
   flags |= pd->references.error > 0 ? ECORE_FD_ERROR : 0;
   if (flags == 0)
     {
        if (pd->handler)
          {
             ecore_main_fd_handler_del(pd->handler);
             pd->handler = NULL;
          }
        return;
     }

   if (pd->handler)
     ecore_main_fd_handler_active_set(pd->handler, flags);
   else if (pd->file)
     pd->handler = ecore_main_fd_handler_file_add(pd->fd, flags, _efl_loop_fd_read_cb, obj, NULL, NULL);
   else
     pd->handler = ecore_main_fd_handler_add(pd->fd, flags, _efl_loop_fd_read_cb, obj, NULL, NULL);
}

/**
 * @brief Sets the general file descriptor to monitor.
 *
 * This function updates the file descriptor being watched by the Efl_Loop_Fd object.
 * It marks the fd as not being a regular file (e.g., a socket).
 * After setting the fd, it resets the underlying Ecore_Fd_Handler.
 *
 * @param obj The Efl_Loop_Fd Eo object.
 * @param pd The private data of the Efl_Loop_Fd object.
 * @param fd The file descriptor to monitor.
 */
static void
_efl_loop_fd_fd_set(Eo *obj, Efl_Loop_Fd_Data *pd, int fd)
{
   pd->fd = fd;
   pd->file = EINA_FALSE;
   _efl_loop_fd_reset(obj, pd);
}

/**
 * @brief Gets the general file descriptor being monitored.
 *
 * @param obj The Efl_Loop_Fd Eo object (unused).
 * @param pd The private data of the Efl_Loop_Fd object.
 * @return The file descriptor, or -1 if it's a regular file fd (use fd_file_get for that).
 */
static int
_efl_loop_fd_fd_get(const Eo *obj EINA_UNUSED, Efl_Loop_Fd_Data *pd)
{
   return pd->file ? -1 : pd->fd;
}

/**
 * @brief Sets a regular file descriptor to monitor.
 *
 * This function updates the file descriptor being watched by the Efl_Loop_Fd object.
 * It specifically marks the fd as being a regular file.
 * After setting the fd, it resets the underlying Ecore_Fd_Handler.
 *
 * @param obj The Efl_Loop_Fd Eo object.
 * @param pd The private data of the Efl_Loop_Fd object.
 * @param fd The file descriptor of a regular file to monitor.
 */
static void
_efl_loop_fd_fd_file_set(Eo *obj, Efl_Loop_Fd_Data *pd, int fd)
{
   pd->fd = fd;
   pd->file = EINA_TRUE;
   _efl_loop_fd_reset(obj, pd);
}

/**
 * @brief Gets the regular file descriptor being monitored.
 *
 * @param obj The Efl_Loop_Fd Eo object (unused).
 * @param pd The private data of the Efl_Loop_Fd object.
 * @return The file descriptor if it's a regular file, or -1 otherwise.
 */
static int
_efl_loop_fd_fd_file_get(const Eo *obj EINA_UNUSED, Efl_Loop_Fd_Data *pd)
{
   return pd->file ? pd->fd : -1;
}

/**
 * @brief Callback invoked when an event listener is added to Efl_Loop_Fd.
 *
 * This function updates the reference counts for read, write, or error events.
 * If a reference count for a specific event type transitions from 0 to 1,
 * it means a listener for this event type has been added for the first time,
 * and the underlying Ecore_Fd_Handler needs to be updated.
 *
 * @param data The private data (Efl_Loop_Fd_Data) of the Efl_Loop_Fd object.
 * @param event The event details, where event->info is an Efl_Callback_Array_Item_Full array
 *              describing the event(s) being added.
 *              Example `array` structure:
 *              `array[0].desc = EFL_LOOP_FD_EVENT_READ`
 *              `array[0].legacy_info_size = 0` (or other value)
 *              `array[0].api_new = EINA_TRUE` (or EINA_FALSE)
 *              `array[1].desc = NULL` (marks end of array)
 */
static void
_check_fd_event_catcher_add(void *data, const Efl_Event *event)
{
   const Efl_Callback_Array_Item_Full *array = event->info;
   Efl_Loop_Fd_Data *fd = data;
   Eina_Bool need_reset = EINA_FALSE;
   int i;

   for (i = 0; array[i].desc != NULL; i++)
     {
        if (array[i].desc == EFL_LOOP_FD_EVENT_READ)
          {
             if (fd->references.read++ > 0) continue;
             need_reset = EINA_TRUE;
          }
        else if (array[i].desc == EFL_LOOP_FD_EVENT_WRITE)
          {
             if (fd->references.write++ > 0) continue;
             need_reset = EINA_TRUE;
          }
        if (array[i].desc == EFL_LOOP_FD_EVENT_ERROR)
          {
             if (fd->references.error++ > 0) continue;
             need_reset = EINA_TRUE;
          }
     }

   if (need_reset)
     _efl_loop_fd_reset(event->object, fd);
}

/**
 * @brief Callback invoked when an event listener is removed from Efl_Loop_Fd.
 *
 * This function updates the reference counts for read, write, or error events.
 * If a reference count for a specific event type transitions from 1 to 0,
 * it means the last listener for this event type has been removed,
 * and the underlying Ecore_Fd_Handler needs to be updated.
 *
 * @param data The private data (Efl_Loop_Fd_Data) of the Efl_Loop_Fd object.
 * @param event The event details, where event->info is an Efl_Callback_Array_Item_Full array
 *              describing the event(s) being removed.
 *              Example `array` structure:
 *              `array[0].desc = EFL_LOOP_FD_EVENT_WRITE`
 *              `array[0].legacy_info_size = 0` (or other value)
 *              `array[0].api_new = EINA_TRUE` (or EINA_FALSE)
 *              `array[1].desc = NULL` (marks end of array)
 */
static void
_check_fd_event_catcher_del(void *data, const Efl_Event *event)
{
   const Efl_Callback_Array_Item_Full *array = event->info;
   Efl_Loop_Fd_Data *fd = data;
   Eina_Bool need_reset = EINA_FALSE;
   int i;

   for (i = 0; array[i].desc != NULL; i++)
     {
        if (array[i].desc == EFL_LOOP_FD_EVENT_READ)
          {
             if (fd->references.read-- > 1) continue;
             need_reset = EINA_TRUE;
          }
        else if (array[i].desc == EFL_LOOP_FD_EVENT_WRITE)
          {
             if (fd->references.write-- > 1) continue;
             need_reset = EINA_TRUE;
          }
        if (array[i].desc == EFL_LOOP_FD_EVENT_ERROR)
          {
             if (fd->references.error-- > 1) continue;
             need_reset = EINA_TRUE;
          }
     }

   if (need_reset)
     _efl_loop_fd_reset(event->object, fd);
}

EFL_CALLBACKS_ARRAY_DEFINE(fd_watch,
                          { EFL_EVENT_CALLBACK_ADD, _check_fd_event_catcher_add },
                          { EFL_EVENT_CALLBACK_DEL, _check_fd_event_catcher_del });

/**
 * @brief Constructor for Efl_Loop_Fd objects.
 *
 * Initializes the Efl_Loop_Fd object, sets up event callbacks for tracking
 * listeners, and initializes the file descriptor to an invalid state (-1).
 *
 * @param obj The Eo object to construct.
 * @param pd The private data for the Efl_Loop_Fd object.
 * @return The constructed Eo object.
 */
static Efl_Object *
_efl_loop_fd_efl_object_constructor(Eo *obj, Efl_Loop_Fd_Data *pd)
{
   efl_constructor(efl_super(obj, MY_CLASS));

   efl_event_callback_array_add(obj, fd_watch(), pd);

   pd->fd = -1;

   return obj;
}

/**
 * @brief Handles setting the parent of an Efl_Loop_Fd object.
 *
 * When the parent of an Efl_Loop_Fd object changes, this function ensures
 * that the associated Ecore_Fd_Handler is properly managed. If a handler
 * exists, it is deleted. Then, the parent is set using the superclass's
 * implementation. If a new parent is set (not NULL), the Ecore_Fd_Handler
 * is reset to reflect the current state. This is important because the
 * Efl_Loop_Fd object's lifecycle might be tied to its parent in an EFL loop.
 *
 * @param obj The Efl_Loop_Fd Eo object.
 * @param pd The private data of the Efl_Loop_Fd object.
 * @param parent The new parent object.
 */
static void
_efl_loop_fd_efl_object_parent_set(Eo *obj, Efl_Loop_Fd_Data *pd, Efl_Object *parent)
{
   if (pd->handler) ecore_main_fd_handler_del(pd->handler);
   pd->handler = NULL;

   efl_parent_set(efl_super(obj, MY_CLASS), parent);

   if (parent == NULL) return ;

   _efl_loop_fd_reset(obj, pd);
}

/**
 * @brief Invalidates the Efl_Loop_Fd object.
 *
 * This function is called when the object is being invalidated (e.g., during destruction).
 * It ensures that the Ecore_Fd_Handler is deleted to prevent further callbacks
 * on a potentially invalid object. Then, it calls the superclass's invalidate method.
 *
 * @param obj The Efl_Loop_Fd Eo object.
 * @param pd The private data of the Efl_Loop_Fd object.
 */
static void
_efl_loop_fd_efl_object_invalidate(Eo *obj, Efl_Loop_Fd_Data *pd)
{
   ecore_main_fd_handler_del(pd->handler);

   efl_invalidate(efl_super(obj, MY_CLASS));
}

#include "efl_loop_fd.eo.c"
