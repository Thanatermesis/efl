/**
 * @file
 * @brief Ecore functions for managing idle exiters.
 *
 * Idle exiters are callbacks that are called when the main loop is idle
 * and about to exit.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>

#include <Eo.h>

#include "Ecore.h"
#include "ecore_private.h"

/**
 * @internal
 * @brief Callbacks for Ecore_Idle_Exiter.
 *
 * This array defines the Eo callbacks for an Ecore_Idle_Exiter.
 * - EFL_LOOP_EVENT_IDLE_EXIT: Calls _ecore_factorized_idle_process when the loop is idle and about to exit.
 * - EFL_EVENT_DEL: Calls _ecore_factorized_idle_event_del when the idle exiter is deleted.
 */
EFL_CALLBACKS_ARRAY_DEFINE(ecore_idle_exiter_callbacks,
                          { EFL_LOOP_EVENT_IDLE_EXIT, _ecore_factorized_idle_process },
                          { EFL_EVENT_DEL, _ecore_factorized_idle_event_del });

/**
 * @brief Adds an idle exiter to the main loop.
 *
 * @param func The function to call when the main loop is idle and about to exit.
 * @param data The data to pass to @p func.
 * @return A handle to the new idle exiter, or @c NULL on failure.
 *
 * This function adds an idle exiter to the main loop. The function @p func
 * will be called when the main loop has no more events, timers, or other
 * handlers to process and is about to exit.
 *
 * Example:
 * @code
 * static Eina_Bool
 * _my_idle_exiter_cb(void *data)
 * {
 *    printf("Main loop is about to exit with data: %s\n", (const char *)data);
 *    return ECORE_CALLBACK_RENEW; // or ECORE_CALLBACK_CANCEL
 * }
 *
 * // ...
 * const char *my_data = "example data";
 * ecore_idle_exiter_add(_my_idle_exiter_cb, my_data);
 * // ...
 * @endcode
 */
EAPI Ecore_Idle_Exiter *
ecore_idle_exiter_add(Ecore_Task_Cb func,
                      const void   *data)
{
   return  _ecore_factorized_idle_add(ecore_idle_exiter_callbacks(), func, data);
}

/**
 * @brief Deletes an idle exiter from the main loop.
 *
 * @param idle_exiter The idle exiter to delete.
 * @return The data pointer that was passed to ecore_idle_exiter_add()
 *         when the idle exiter was added, or @c NULL if the handle is invalid.
 *
 * This function removes the specified idle exiter from the main loop.
 * If the idle exiter was successfully deleted, the function returns the
 * @c data pointer that was associated with it. Otherwise, it returns @c NULL.
 *
 * Example:
 * @code
 * Ecore_Idle_Exiter *exiter;
 * const char *my_data = "example data";
 *
 * static Eina_Bool _my_cb(void *data) { return ECORE_CALLBACK_CANCEL; }
 *
 * exiter = ecore_idle_exiter_add(_my_cb, my_data);
 * if (exiter)
 * {
 *    void *ret_data = ecore_idle_exiter_del(exiter);
 *    if (ret_data == my_data)
 *      printf("Idle exiter deleted successfully.\n");
 * }
 * @endcode
 */
EAPI void *
ecore_idle_exiter_del(Ecore_Idle_Exiter *idle_exiter)
{
   return _ecore_factorized_idle_del(idle_exiter);
}
