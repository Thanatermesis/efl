#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>

#include <Eo.h>

#include "Ecore.h"
#include "ecore_private.h"

EFL_CALLBACKS_ARRAY_DEFINE(ecore_idle_enterer_callbacks,
                          { EFL_LOOP_EVENT_IDLE_ENTER, _ecore_factorized_idle_process },
                          { EFL_EVENT_DEL, _ecore_factorized_idle_event_del });

/**
 * @brief Adds an idle enterer callback.
 *
 * This function adds a callback that will be called when the main loop
 * is about to enter an idle state. The callback will be called before
 * the system sleeps, allowing tasks to be performed when the application
 * is not busy.
 *
 * @param func The callback function to be executed.
 * @param data User data to be passed to the callback function.
 * @return A handle to the idle enterer, or @c NULL on failure.
 *
 * @see ecore_idle_enterer_del()
 * @see ecore_idle_enterer_before_add()
 */
EAPI Ecore_Idle_Enterer *
ecore_idle_enterer_add(Ecore_Task_Cb func,
                       const void   *data)
{
   return _ecore_factorized_idle_add(ecore_idle_enterer_callbacks(), func, data);
}

/**
 * @brief Adds an idle enterer callback that runs before other idle enterers.
 *
 * This function is similar to ecore_idle_enterer_add(), but the added
 * callback will be executed with a higher priority (EFL_CALLBACK_PRIORITY_BEFORE),
 * meaning it runs before other standard idle enterer callbacks.
 *
 * @param func The callback function to be executed.
 * @param data User data to be passed to the callback function.
 * @return A handle to the idle enterer, or @c NULL on failure.
 *
 * @see ecore_idle_enterer_del()
 * @see ecore_idle_enterer_add()
 */
EAPI Ecore_Idle_Enterer *
ecore_idle_enterer_before_add(Ecore_Task_Cb func,
                              const void   *data)
{
   Ecore_Idle_Enterer *ie = NULL;
   ie = _ecore_factorized_idle_add(ecore_idle_enterer_callbacks(), func, data);

   // This avoid us duplicating code and should only be slightly slower
   // due to a useless cycle of callback registration
   efl_event_callback_array_del(_mainloop_singleton, ecore_idle_enterer_callbacks(), ie);
   efl_event_callback_array_priority_add(_mainloop_singleton, ecore_idle_enterer_callbacks(), EFL_CALLBACK_PRIORITY_BEFORE, ie);

   return ie;
}

/**
 * @brief Deletes an idle enterer callback.
 *
 * This function removes a previously added idle enterer callback.
 *
 * @param idle_enterer The handle of the idle enterer to delete.
 * @return The data pointer originally passed to ecore_idle_enterer_add()
 *         or ecore_idle_enterer_before_add().
 *
 * @see ecore_idle_enterer_add()
 * @see ecore_idle_enterer_before_add()
 */
EAPI void *
ecore_idle_enterer_del(Ecore_Idle_Enterer *idle_enterer)
{
   return _ecore_factorized_idle_del(idle_enterer);
}
