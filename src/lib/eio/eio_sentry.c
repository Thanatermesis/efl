/* EIO - EFL data type library
 * Copyright (C) 2016 Enlightenment Developers:
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
#include "Ecore.h"
#include "Eio.h"
#include "eio_sentry_private.h"

/**
 * @internal
 * @brief Translates an EIO_MONITOR event type to its corresponding EIO_SENTRY event description.
 *
 * This function maps integer-based event types from the EIO_MONITOR system
 * to specific Efl_Event_Description pointers used by EIO_SENTRY.
 *
 * @param input_event The EIO_MONITOR event type (e.g., EIO_MONITOR_FILE_CREATED).
 * @return The corresponding Efl_Event_Description* for EIO_SENTRY, or NULL if no match is found.
 */
static const Efl_Event_Description*
_translate_event(int input_event)
{
   if (input_event == EIO_MONITOR_FILE_CREATED)
     return EIO_SENTRY_EVENT_FILE_CREATED;
   else if (input_event == EIO_MONITOR_FILE_DELETED)
     return EIO_SENTRY_EVENT_FILE_DELETED;
   else if (input_event == EIO_MONITOR_FILE_MODIFIED)
     return EIO_SENTRY_EVENT_FILE_MODIFIED;
   else if (input_event == EIO_MONITOR_FILE_CLOSED)
     return EIO_SENTRY_EVENT_FILE_CLOSED;
   else if (input_event == EIO_MONITOR_DIRECTORY_CREATED)
     return EIO_SENTRY_EVENT_DIRECTORY_CREATED;
   else if (input_event == EIO_MONITOR_DIRECTORY_DELETED)
     return EIO_SENTRY_EVENT_DIRECTORY_DELETED;
   else if (input_event == EIO_MONITOR_DIRECTORY_MODIFIED)
     return EIO_SENTRY_EVENT_DIRECTORY_MODIFIED;
   else if (input_event == EIO_MONITOR_DIRECTORY_CLOSED)
     return EIO_SENTRY_EVENT_DIRECTORY_CLOSED;
   else if (input_event == EIO_MONITOR_SELF_RENAME)
     return EIO_SENTRY_EVENT_SELF_RENAME;
   else if (input_event == EIO_MONITOR_SELF_DELETED)
     return EIO_SENTRY_EVENT_SELF_DELETED;
   else if (input_event == EIO_MONITOR_ERROR)
     return EIO_SENTRY_EVENT_ERROR;
   else
     return NULL;
}

/**
 * @internal
 * @brief Handles Ecore events from Eio.Monitor instances.
 *
 * This function is an Ecore_Event_Handler callback. It receives events
 * from monitored paths, translates them into Eio_Sentry_Event format,
 * and then dispatches them using efl_event_callback_call on the sentry object.
 * If an EIO_MONITOR_ERROR event is received, the corresponding monitor is removed.
 *
 * @param data Pointer to Eio_Sentry_Data associated with the sentry object.
 * @param type The type of the Ecore event (corresponds to EIO_MONITOR event types).
 * @param event Pointer to the Eio_Monitor_Event structure containing event details.
 * @return ECORE_CALLBACK_PASS_ON to continue processing, or ECORE_CALLBACK_DONE to stop.
 */
static unsigned char
_handle_event(void *data, int type, void *event)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(data, ECORE_CALLBACK_PASS_ON);
   EINA_SAFETY_ON_NULL_RETURN_VAL(event, ECORE_CALLBACK_PASS_ON);

   const Efl_Event_Description* translated_event = _translate_event(type);
   Eio_Sentry_Data *pd = (Eio_Sentry_Data *)data;
   Eio_Monitor_Event *monitor_event = (Eio_Monitor_Event *)event;

   Eio_Sentry_Event *event_info = malloc(sizeof(Eio_Sentry_Event));
   EINA_SAFETY_ON_NULL_RETURN_VAL(event_info, ECORE_CALLBACK_PASS_ON);

   event_info->source = eio_monitor_path_get(monitor_event->monitor);
   event_info->trigger = monitor_event->filename;

   efl_event_callback_call(pd->object, translated_event, event_info);

   // If event was error, we must delete the monitor.
   if (type == EIO_MONITOR_ERROR)
     eina_hash_del(pd->targets, event_info->source, NULL);

   free(event_info);

   return ECORE_CALLBACK_PASS_ON;
}

/**
 * @internal
 * @brief Initializes Ecore event handlers for Eio.Monitor events.
 *
 * This function creates and registers Ecore_Event_Handler instances for all
 * relevant EIO_MONITOR event types. These handlers will call _handle_event
 * when an event occurs. The handlers are stored in the `handlers` array
 * within the Eio_Sentry_Data structure.
 *
 * @param pd Pointer to the Eio_Sentry_Data structure for the sentry object.
 */
static void
_initialize_handlers(Eio_Sentry_Data *pd)
{
   Ecore_Event_Handler *h;
   EINA_SAFETY_ON_NULL_RETURN(pd);
   pd->handlers = eina_array_new(11);

   h = ecore_event_handler_add(EIO_MONITOR_FILE_CREATED, _handle_event, pd);
   eina_array_push(pd->handlers, h);
   h = ecore_event_handler_add(EIO_MONITOR_FILE_DELETED, _handle_event, pd);
   eina_array_push(pd->handlers, h);
   h = ecore_event_handler_add(EIO_MONITOR_FILE_MODIFIED, _handle_event, pd);
   eina_array_push(pd->handlers, h);
   h = ecore_event_handler_add(EIO_MONITOR_FILE_CLOSED, _handle_event, pd);
   eina_array_push(pd->handlers, h);

   h = ecore_event_handler_add(EIO_MONITOR_DIRECTORY_CREATED, _handle_event, pd);
   eina_array_push(pd->handlers, h);
   h = ecore_event_handler_add(EIO_MONITOR_DIRECTORY_DELETED, _handle_event, pd);
   eina_array_push(pd->handlers, h);
   h = ecore_event_handler_add(EIO_MONITOR_DIRECTORY_MODIFIED, _handle_event, pd);
   eina_array_push(pd->handlers, h);
   h = ecore_event_handler_add(EIO_MONITOR_DIRECTORY_CLOSED, _handle_event, pd);
   eina_array_push(pd->handlers, h);

   h = ecore_event_handler_add(EIO_MONITOR_SELF_RENAME, _handle_event, pd);
   eina_array_push(pd->handlers, h);
   h = ecore_event_handler_add(EIO_MONITOR_SELF_DELETED, _handle_event, pd);
   eina_array_push(pd->handlers, h);
   h = ecore_event_handler_add(EIO_MONITOR_ERROR, _handle_event, pd);
   eina_array_push(pd->handlers, h);
}

/**
 * @internal
 * @brief Adds a path to be monitored by the Eio.Sentry object.
 * @implements Eio.Sentry.add
 *
 * This function creates an Eio.Monitor for the given path and stores it.
 * If event handlers haven't been initialized yet, it calls _initialize_handlers.
 *
 * @param obj The Eio.Sentry Eo object (unused).
 * @param pd Pointer to the private data of the Eio.Sentry object.
 * @param path The file or directory path to monitor.
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., failed to create monitor or add to hash).
 */
Eina_Bool
_eio_sentry_add(Eo *obj EINA_UNUSED, Eio_Sentry_Data *pd, const char *path)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(path, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(pd, EINA_FALSE);

   if (!pd->handlers)
     _initialize_handlers(pd);

   if (eina_hash_find(pd->targets, path))
     return EINA_TRUE;

   Eio_Monitor *monitor = eio_monitor_add(path);

   if (!monitor)
     {
        EINA_LOG_ERR("Failed to create monitor.");
        return EINA_FALSE;
     }

   if (!eina_hash_add(pd->targets, path, monitor))
     {
        EINA_LOG_ERR("Failed to register monitor.");
        eio_monitor_del(monitor);
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Removes a path from being monitored by the Eio.Sentry object.
 * @implements Eio.Sentry.remove
 *
 * This function removes the Eio.Monitor associated with the given path.
 * The monitor itself will be deleted due to the Eina_Free_Cb set on the hash.
 *
 * @param obj The Eio.Sentry Eo object (unused).
 * @param pd Pointer to the private data of the Eio.Sentry object.
 * @param path The file or directory path to stop monitoring.
 */
void
_eio_sentry_remove(Eo *obj EINA_UNUSED, Eio_Sentry_Data *pd, const char *path)
{
   EINA_SAFETY_ON_NULL_RETURN(path);
   EINA_SAFETY_ON_NULL_RETURN(pd);

   eina_hash_del(pd->targets, path, NULL);
}

/**
 * @internal
 * @brief Checks if the monitor for a given path is using a fallback mechanism.
 * @implements Eio.Sentry.fallback_check
 *
 * This function retrieves the Eio.Monitor for the specified path and then
 * calls eio_monitor_fallback_check() on it to determine if a less efficient
 * polling-based fallback is being used instead of native OS events.
 *
 * @param obj The Eio.Sentry Eo object (unused).
 * @param pd Pointer to the private data of the Eio.Sentry object.
 * @param path The file or directory path to check.
 * @return EINA_TRUE if the monitor is using a fallback, EINA_FALSE otherwise or if the path is not monitored.
 */
Eina_Bool
_eio_sentry_fallback_check(const Eo *obj EINA_UNUSED, Eio_Sentry_Data *pd, const char *path)
{
   Eio_Monitor *monitor;

   EINA_SAFETY_ON_NULL_RETURN_VAL(path, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(pd, EINA_FALSE);

   monitor = eina_hash_find(pd->targets, path);
   EINA_SAFETY_ON_NULL_RETURN_VAL(monitor, EINA_FALSE);
   return eio_monitor_fallback_check(monitor);
}

/**
 * @internal
 * @brief Constructor for the Eio.Sentry object.
 * @implements Efl.Object.constructor
 *
 * Initializes the Eio.Sentry object by calling the parent constructor
 * and setting up internal data structures, such as the hash table for
 * storing monitored paths and their corresponding Eio.Monitor instances.
 *
 * @param obj The Eo object being constructed.
 * @param pd Pointer to the private data for this Eio.Sentry instance.
 * @return The constructed Eo object.
 */
Efl_Object * _eio_sentry_efl_object_constructor(Eo *obj, Eio_Sentry_Data *pd)
{
   obj = efl_constructor(efl_super(obj, EIO_SENTRY_CLASS));

   pd->object = obj;
   pd->targets = eina_hash_string_small_new((Eina_Free_Cb)&eio_monitor_del);

   return obj;
}

/**
 * @internal
 * @brief Destructor for the Eio.Sentry object.
 * @implements Efl.Object.destructor
 *
 * Cleans up resources used by the Eio.Sentry object. This includes
 * freeing the hash table of monitored paths (which also deletes the
 * Eio.Monitor instances) and removing all registered Ecore event handlers.
 * Finally, it calls the parent object's destructor.
 *
 * @param obj The Eo object being destructed.
 * @param pd Pointer to the private data for this Eio.Sentry instance.
 */
void _eio_sentry_efl_object_destructor(Eo *obj, Eio_Sentry_Data *pd)
{
   eina_hash_free(pd->targets);
   if (pd->handlers)
     {
        while (eina_array_count(pd->handlers))
          ecore_event_handler_del(eina_array_pop(pd->handlers));
        eina_array_free(pd->handlers);
     }

   efl_destructor(efl_super(obj, EIO_SENTRY_CLASS));
}

#include "eio_sentry.eo.c"
