/**
 * @file
 * @brief These routines are bindings to the EFL access window interface.
 */

#ifndef EFL_ACCESS_WINDOW_H
#define EFL_ACCESS_WINDOW_H

#ifdef EFL_BETA_API_SUPPORT
#include "efl_access_window.eo.h"

/**
 * @brief Emits 'Window:Activated' accessible signal.
 *
 * This macro should be called when a window becomes the active window.
 * It emits an accessibility event indicating that the window has been activated.
 *
 * @param obj The Efl_Access_Object representing the window.
 */
#define efl_access_window_activated_signal_emit(obj) \
   efl_access_object_event_emit(obj, EFL_ACCESS_WINDOW_EVENT_WINDOW_ACTIVATED, NULL);

/**
 * @brief Emits 'Window:Deactivated' accessible signal.
 *
 * This macro should be called when a window is no longer the active window.
 * It emits an accessibility event indicating that the window has been deactivated.
 *
 * @param obj The Efl_Access_Object representing the window.
 */
#define efl_access_window_deactivated_signal_emit(obj) \
   efl_access_object_event_emit(obj, EFL_ACCESS_WINDOW_EVENT_WINDOW_DEACTIVATED, NULL);

/**
 * @brief Emits 'Window:Created' accessible signal.
 *
 * This macro should be called when a new window is created.
 * It emits an accessibility event indicating that a window has been created.
 *
 * @param obj The Efl_Access_Object representing the newly created window.
 */
#define efl_access_window_created_signal_emit(obj) \
   efl_access_object_event_emit(obj, EFL_ACCESS_WINDOW_EVENT_WINDOW_CREATED, NULL);

/**
 * @brief Emits 'Window:Destroyed' accessible signal.
 *
 * This macro should be called when a window is about to be destroyed.
 * It emits an accessibility event indicating that a window has been destroyed.
 *
 * @param obj The Efl_Access_Object representing the window being destroyed.
 */
#define efl_access_window_destroyed_signal_emit(obj) \
   efl_access_object_event_emit(obj, EFL_ACCESS_WINDOW_EVENT_WINDOW_DESTROYED, NULL);

/**
 * @brief Emits 'Window:Maximized' accessible signal.
 *
 * This macro should be called when a window is maximized.
 * It emits an accessibility event indicating that the window has been maximized.
 *
 * @param obj The Efl_Access_Object representing the window.
 */
#define efl_access_window_maximized_signal_emit(obj) \
   efl_access_object_event_emit(obj, EFL_ACCESS_WINDOW_EVENT_WINDOW_MAXIMIZED, NULL);

/**
 * @brief Emits 'Window:Minimized' accessible signal.
 *
 * This macro should be called when a window is minimized.
 * It emits an accessibility event indicating that the window has been minimized.
 *
 * @param obj The Efl_Access_Object representing the window.
 */
#define efl_access_window_minimized_signal_emit(obj) \
   efl_access_object_event_emit(obj, EFL_ACCESS_WINDOW_EVENT_WINDOW_MINIMIZED, NULL);

/**
 * @brief Emits 'Window:Restored' accessible signal.
 *
 * This macro should be called when a window is restored from a minimized or maximized state.
 * It emits an accessibility event indicating that the window has been restored.
 *
 * @param obj The Efl_Access_Object representing the window.
 */
#define efl_access_window_restored_signal_emit(obj) \
   efl_access_object_event_emit(obj, EFL_ACCESS_WINDOW_EVENT_WINDOW_RESTORED, NULL);

#endif
#endif
