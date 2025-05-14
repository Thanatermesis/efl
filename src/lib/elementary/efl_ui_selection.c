#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif


#include <Elementary.h>
#include "elm_priv.h"

#define MY_CLASS EFL_UI_SELECTION_MIXIN
#define MY_CLASS_NAME "Efl.Ui.Selection"

/**
 * @brief Private data structure for the Efl_Ui_Selection mixin.
 * @since 1.24
 */
typedef struct {
   Ecore_Evas *ee; /**< The Ecore_Evas instance associated with the object. */
   Eina_Bool registered : 1; /**< Flag indicating if the selection changed event listener is registered. */
} Efl_Ui_Selection_Data;

/**
 * @brief Converts an Efl_Ui_Cnp_Buffer to its corresponding Ecore_Evas_Selection_Buffer.
 *
 * @param buffer The Efl_Ui_Cnp_Buffer to convert.
 * @return The corresponding Ecore_Evas_Selection_Buffer.
 */
static inline Ecore_Evas_Selection_Buffer
_ee_buffer_get(Efl_Ui_Cnp_Buffer buffer)
{
   if (buffer == EFL_UI_CNP_BUFFER_SELECTION)
     return ECORE_EVAS_SELECTION_BUFFER_SELECTION_BUFFER;
   else
     return ECORE_EVAS_SELECTION_BUFFER_COPY_AND_PASTE_BUFFER;
}

/**
 * @brief Implements the Efl.Ui.Selection.selection_get Eolian method.
 * Retrieves the current selection content for a given buffer and seat.
 *
 * @param obj The Efl_Ui_Selection object.
 * @param pd Private data for the Efl_Ui_Selection object.
 * @param buffer The selection buffer to get data from (e.g., primary, clipboard).
 * @param seat The seat for which to get the selection (typically 0 for default).
 * @param acceptable_types An iterator of MIME types the caller is willing to accept.
 * @return A future that resolves with an Eina_Content representing the selection,
 *         or an error if the operation fails.
 */
EOLIAN static Eina_Future*
_efl_ui_selection_selection_get(Eo *obj EINA_UNUSED, Efl_Ui_Selection_Data *pd, Efl_Ui_Cnp_Buffer buffer, unsigned int seat, Eina_Iterator *acceptable_types)
{
   return ecore_evas_selection_get(pd->ee, seat, _ee_buffer_get(buffer), acceptable_types);
}

/**
 * @brief Implements the Efl.Ui.Selection.selection_set Eolian method.
 * Sets the selection content for a given buffer and seat.
 *
 * @param obj The Efl_Ui_Selection object.
 * @param pd Private data for the Efl_Ui_Selection object.
 * @param buffer The selection buffer to set data for.
 * @param content The Eina_Content to set as the selection.
 * @param seat The seat for which to set the selection.
 */
EOLIAN static void
_efl_ui_selection_selection_set(Eo *obj, Efl_Ui_Selection_Data *pd, Efl_Ui_Cnp_Buffer buffer, Eina_Content *content, unsigned int seat)
{
   _register_selection_changed(obj);
   ecore_evas_selection_set(pd->ee, seat, _ee_buffer_get(buffer), content);
}

/**
 * @brief Implements the Efl.Ui.Selection.selection_clear Eolian method.
 * Clears the selection content for a given buffer and seat.
 *
 * @param obj The Efl_Ui_Selection object.
 * @param pd Private data for the Efl_Ui_Selection object.
 * @param buffer The selection buffer to clear.
 * @param seat The seat for which to clear the selection.
 */
EOLIAN static void
_efl_ui_selection_selection_clear(Eo *obj EINA_UNUSED, Efl_Ui_Selection_Data *pd, Efl_Ui_Cnp_Buffer buffer, unsigned int seat)
{
   ecore_evas_selection_set(pd->ee, seat, _ee_buffer_get(buffer), NULL);
}

/**
 * @brief Implements the Efl.Ui.Selection.has_selection Eolian method.
 * Checks if there is a selection for a given buffer and seat.
 *
 * @param obj The Efl_Ui_Selection object.
 * @param pd Private data for the Efl_Ui_Selection object.
 * @param buffer The selection buffer to check.
 * @param seat The seat for which to check the selection.
 * @return EINA_TRUE if a selection exists, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_selection_has_selection(Eo *obj EINA_UNUSED, Efl_Ui_Selection_Data *pd, Efl_Ui_Cnp_Buffer buffer, unsigned int seat)
{
   return ecore_evas_selection_exists(pd->ee, seat, _ee_buffer_get(buffer));
}

/**
 * @brief Implements the Efl.Object.constructor Eolian method.
 * Initializes the Efl_Ui_Selection object.
 *
 * @param obj The Efl_Ui_Selection object to construct.
 * @param pd Private data for the Efl_Ui_Selection object.
 * @return The constructed Efl_Object, or NULL on failure.
 */
EOLIAN static Efl_Object*
_efl_ui_selection_efl_object_constructor(Eo *obj, Efl_Ui_Selection_Data *pd)
{
  if (!efl_constructor(efl_super(obj, EFL_UI_SELECTION_MIXIN)))
    return NULL;

  pd->ee = ecore_evas_ecore_evas_get(evas_object_evas_get(obj));

  return obj;
}

/**
 * @brief Implements the Efl.Object.invalidate Eolian method.
 * Cleans up resources used by the Efl_Ui_Selection object.
 * This includes unregistering the selection changed event listener if it was registered.
 *
 * @param obj The Efl_Ui_Selection object to invalidate.
 * @param pd Private data for the Efl_Ui_Selection object.
 */
EOLIAN static void
_efl_ui_selection_efl_object_invalidate(Eo *obj, Efl_Ui_Selection_Data *pd)
{
   if (pd->registered)
     {
        _selection_changed_event_unregister(obj);
     }
   efl_invalidate(efl_super(obj, EFL_UI_SELECTION_MIXIN));
}

/**
 * @brief Implements the Efl.Object.event_callback_priority_add Eolian method.
 * Adds an event callback for the Efl_Ui_Selection object.
 * If the event is EFL_UI_SELECTION_EVENT_WM_SELECTION_CHANGED and it's the first
 * listener for this event, it registers an internal listener for window manager
 * selection changes.
 *
 * @param obj The Efl_Ui_Selection object.
 * @param pd Private data for the Efl_Ui_Selection object.
 * @param desc The event description.
 * @param priority The callback priority.
 * @param func The callback function.
 * @param user_data User data to pass to the callback.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_selection_efl_object_event_callback_priority_add(Eo *obj, Efl_Ui_Selection_Data *pd,
                                        const Efl_Event_Description *desc,
                                        Efl_Callback_Priority priority,
                                        Efl_Event_Cb func,
                                        const void *user_data)
{
  if (desc == EFL_UI_SELECTION_EVENT_WM_SELECTION_CHANGED && !pd->registered)
    {
       // If a listener for WM_SELECTION_CHANGED is added, we need to
       // register our internal handler to receive events from ecore_evas.
       pd->registered = EINA_TRUE;
       if (efl_finalized_get(obj))
         _selection_changed_event_register(obj);
    }

  return efl_event_callback_priority_add(efl_super(obj, EFL_UI_SELECTION_MIXIN), desc, priority, func, user_data);
}

/**
 * @brief Implements the Efl.Object.event_callback_array_priority_add Eolian method.
 * Adds multiple event callbacks for the Efl_Ui_Selection object.
 * If any of the events is EFL_UI_SELECTION_EVENT_WM_SELECTION_CHANGED and it's the first
 * listener for this event, it registers an internal listener for window manager
 * selection changes.
 *
 * @param obj The Efl_Ui_Selection object.
 * @param pd Private data for the Efl_Ui_Selection object.
 * @param array An array of Efl_Callback_Array_Item.
 *              Example:
 *              @code
 *              static const Efl_Callback_Array_Item callbacks[] = {
 *                 { EFL_UI_SELECTION_EVENT_WM_SELECTION_CHANGED, _my_selection_changed_cb },
 *                 { NULL, NULL }
 *              };
 *              @endcode
 * @param priority The callback priority.
 * @param user_data User data to pass to the callbacks.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_selection_efl_object_event_callback_array_priority_add(Eo *obj, Efl_Ui_Selection_Data *pd,
                                              const Efl_Callback_Array_Item *array,
                                              Efl_Callback_Priority priority,
                                              const void *user_data)
{
   for (int i = 0; array[i].desc; ++i)
     {
        if (array[i].desc == EFL_UI_SELECTION_EVENT_WM_SELECTION_CHANGED && !pd->registered)
          {
             // If a listener for WM_SELECTION_CHANGED is added, we need to
             // register our internal handler to receive events from ecore_evas.
             pd->registered = EINA_TRUE;
             if (efl_finalized_get(obj))
               _selection_changed_event_register(obj);
          }
     }
   return efl_event_callback_array_priority_add(efl_super(obj, EFL_UI_SELECTION_MIXIN), array, priority, user_data);
}

/**
 * @brief Implements the Efl.Object.finalize Eolian method.
 * Finalizes the Efl_Ui_Selection object.
 * If a listener for EFL_UI_SELECTION_EVENT_WM_SELECTION_CHANGED was added
 * before finalization, this ensures the internal listener for window manager
 * selection changes is registered.
 *
 * @param obj The Efl_Ui_Selection object to finalize.
 * @param pd Private data for the Efl_Ui_Selection object.
 * @return The finalized Efl_Object.
 */
EOLIAN static Efl_Object*
_efl_ui_selection_efl_object_finalize(Eo *obj, Efl_Ui_Selection_Data *pd)
{
   if (pd->registered)
     _selection_changed_event_register(obj);

   return efl_finalize(efl_super(obj, MY_CLASS));
}


#define EFL_UI_SELECTION_EXTRA_OPS \
   EFL_OBJECT_OP_FUNC(efl_event_callback_priority_add, _efl_ui_selection_efl_object_event_callback_priority_add), \
   EFL_OBJECT_OP_FUNC(efl_event_callback_array_priority_add, _efl_ui_selection_efl_object_event_callback_array_priority_add), \

#include "efl_ui_selection.eo.c"
