#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>

#include <Eo.h>

#include "Ecore.h"
#include "ecore_private.h"

/**
 * @internal
 * @brief Structure to hold factorized idle handler data.
 *
 * This structure manages the callback function, user data,
 * event description, reference count, and deletion flag for an idler.
 */
struct _Ecore_Factorized_Idle
{
   Ecore_Task_Cb func; /**< The callback function to execute when idle. */
   void         *data; /**< User-provided data for the callback function. */

   const Efl_Callback_Array_Item *desc; /**< Event description for EFL. */

   short         references; /**< Reference count to manage deletion. */
   Eina_Bool     delete_me : 1; /**< Flag to mark the idler for deletion. */
};

/**
 * @internal
 * @brief Event callback for when a factorized idler is deleted.
 *
 * This function is called when the associated EFL object is deleted,
 * ensuring the idler resources are cleaned up.
 *
 * @param data The Ecore_Factorized_Idle instance.
 * @param event The EFL event data (unused).
 */
void
_ecore_factorized_idle_event_del(void *data, const Efl_Event *event EINA_UNUSED)
{
   _ecore_factorized_idle_del(data);
}

/**
 * @internal
 * @brief Processes an idle event for a factorized idler.
 *
 * This function is called by the main loop when it becomes idle.
 * It executes the idler's callback function. If the callback
 * returns ECORE_CALLBACK_CANCEL (or EINA_FALSE), the idler is marked
 * for deletion.
 *
 * @param data The Ecore_Factorized_Idle instance.
 * @param event The EFL event data (unused).
 */
void
_ecore_factorized_idle_process(void *data, const Efl_Event *event EINA_UNUSED)
{
   Ecore_Factorized_Idle *idler = data;

   idler->references++;
   if (!_ecore_call_task_cb(idler->func, idler->data))
     idler->delete_me = EINA_TRUE;
   idler->references--;

   if (idler->delete_me &&
       idler->references == 0)
     _ecore_factorized_idle_del(idler);
}

static Eina_Mempool *idler_mp = NULL;

/**
 * @internal
 * @brief Deletes a factorized idler.
 *
 * This function handles the actual deletion of an Ecore_Factorized_Idle instance.
 * If the idler is still referenced (e.g., its callback is currently executing),
 * it's marked for deletion and will be removed once the reference count drops to zero.
 * Otherwise, it's removed immediately.
 *
 * @param idler The Ecore_Factorized_Idle instance to delete.
 * @return The user data associated with the idler, or @c NULL if idler was @c NULL.
 */
void *
_ecore_factorized_idle_del(Ecore_Idler *idler)
{
   void *data;

   if (!idler) return NULL;
   EINA_MAIN_LOOP_CHECK_RETURN_VAL(NULL);

   if (idler->references > 0)
     {
        idler->delete_me = EINA_TRUE;
        return idler->data;
     }

   efl_event_callback_array_del(_mainloop_singleton, idler->desc, idler);

   data = idler->data;
   eina_mempool_free(idler_mp, idler);
   return data;
}

/**
 * @internal
 * @brief Adds a new factorized idler.
 *
 * This function creates and initializes an Ecore_Factorized_Idle instance.
 * It allocates memory for the idler, sets up its properties (callback, data, event description),
 * and registers it with the EFL main loop.
 *
 * @param desc The EFL callback array item description.
 *             Example: ecore_idler_callbacks()
 * @param func The callback function to execute when idle.
 * @param data User-provided data for the callback function.
 * @return A pointer to the newly created Ecore_Factorized_Idle instance, or @c NULL on failure.
 */
Ecore_Factorized_Idle *
_ecore_factorized_idle_add(const Efl_Callback_Array_Item *desc,
                           Ecore_Task_Cb func,
                           const void   *data)
{
   Ecore_Factorized_Idle *ret;

   EINA_MAIN_LOOP_CHECK_RETURN_VAL(NULL);

   if (!func)
     {
        ERR("callback function must be set up for an object of Ecore_Idler.");
        return NULL;
     }

   if (!idler_mp)
     {
        idler_mp = eina_mempool_add("chained_mempool", "Ecore_Idle*", NULL, sizeof (Ecore_Factorized_Idle), 23);
        if (!idler_mp) return NULL;
     }

   ret = eina_mempool_malloc(idler_mp, sizeof (Ecore_Factorized_Idle));
   if (!ret) return NULL;

   ret->func = func;
   ret->data = (void*) data;
   ret->desc = desc;
   ret->references = 0;
   ret->delete_me = EINA_FALSE;

   efl_event_callback_array_add(_mainloop_singleton, desc, ret);

   return ret;
}

/* Specific to Ecore_Idler implementation */

/**
 * @internal
 * @brief Defines the EFL callback array for Ecore_Idler events.
 *
 * This array maps EFL event types (idle and deletion) to their respective
 * handler functions for Ecore_Idler instances.
 * - EFL_LOOP_EVENT_IDLE is handled by _ecore_factorized_idle_process.
 * - EFL_EVENT_DEL is handled by _ecore_factorized_idle_event_del.
 */
EFL_CALLBACKS_ARRAY_DEFINE(ecore_idler_callbacks,
                          { EFL_LOOP_EVENT_IDLE, _ecore_factorized_idle_process },
                          { EFL_EVENT_DEL, _ecore_factorized_idle_event_del });

/**
 * @brief Adds an idler handler.
 *
 * @param func The function to call when the main loop is idle.
 * @param data The data to pass to the @p func.
 * @return A handle to the idler handler, or @c NULL on errors.
 *
 * Idlers are callbacks that are called when the main loop has no other events
 * to process. If the idler function returns ECORE_CALLBACK_RENEW (or EINA_TRUE),
 * it will be called again in the next idle period. If it returns
 * ECORE_CALLBACK_CANCEL (or EINA_FALSE), it will be deleted automatically.
 *
 * Example:
 * @code
 * static Eina_Bool
 * _my_idler_cb(void *data)
 * {
 *    printf("Idling...\n");
 *    return ECORE_CALLBACK_RENEW; // Keep idling
 * }
 *
 * // ...
 * ecore_idler_add(_my_idler_cb, NULL);
 * // ...
 * @endcode
 */
EAPI Ecore_Idler *
ecore_idler_add(Ecore_Task_Cb func,
                const void   *data)
{
   return _ecore_factorized_idle_add(ecore_idler_callbacks(), func, data);
}

/**
 * @brief Deletes the idler handler.
 *
 * @param idler The idler handler to delete.
 * @return The data pointer passed to ecore_idler_add() when the idler was added.
 *         Returns @c NULL on failure or if @p idler is @c NULL.
 *
 * Deletes an idler callback from the list of idlers. This function is
 * useful if you want to remove an idler before it has been automatically
 * deleted (i.e., before its callback has returned ECORE_CALLBACK_CANCEL).
 *
 * Example:
 * @code
 * Ecore_Idler *idler;
 *
 * static Eina_Bool
 * _my_idler_cb(void *data)
 * {
 *    printf("Idling once...\n");
 *    return ECORE_CALLBACK_CANCEL; // Delete after one run
 * }
 *
 * idler = ecore_idler_add(_my_idler_cb, NULL);
 *
 * // Sometime later, if we want to delete it before it runs or if it was renewing:
 * // void *data = ecore_idler_del(idler);
 * @endcode
 */
EAPI void *
ecore_idler_del(Ecore_Idler *idler)
{
   return _ecore_factorized_idle_del(idler);
}
