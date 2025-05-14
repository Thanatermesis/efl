#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define ELM_INTERFACE_ATSPI_ACCESSIBLE_PROTECTED
#define ELM_INTERFACE_ATSPI_TEXT_PROTECTED
#define ELM_INTERFACE_ATSPI_TEXT_EDITABLE_PROTECTED
#define ELM_LAYOUT_PROTECTED


#include <Elementary.h>
#include <Elementary_Cursor.h>
#include "elm_priv.h"

/**
 * @brief Private data structure for Efl_Ui_Dnd.
 */
typedef struct {
   Ecore_Evas *ee; /**< The Ecore_Evas instance associated with the DND object. */
   Eina_Bool registered; /**< Flag indicating if drop event listeners are registered. */
} Efl_Ui_Dnd_Data;

/**
 * @brief Structure to hold data during a drag operation start.
 */
typedef struct {
   Eo *win; /**< The drag window object. */
   Efl_Ui_Dnd *obj; /**< The DND object that initiated the drag. */
} Efl_Ui_Drag_Start;

/**
 * @internal
 * @brief Callback function invoked when a drag operation is terminated.
 *
 * This function is called by Ecore_Evas when the drag operation finishes,
 * either successfully (accepted) or unsuccessfully. It emits the
 * EFL_UI_DND_EVENT_DRAG_FINISHED event and cleans up resources
 * associated with the drag operation.
 *
 * @param ee The Ecore_Evas instance (unused).
 * @param seat The seat ID for the drag operation.
 * @param data User data, expected to be an Efl_Ui_Drag_Start struct.
 * @param accepted EINA_TRUE if the drop was accepted, EINA_FALSE otherwise.
 */
static void
_ecore_evas_drag_terminated(Ecore_Evas *ee EINA_UNUSED, unsigned int seat, void *data, Eina_Bool accepted)
{
   Efl_Ui_Drag_Start *start = data;
   Efl_Ui_Drag_Finished_Event ev = {seat, accepted};
   efl_event_callback_call(start->obj, EFL_UI_DND_EVENT_DRAG_FINISHED, &ev);
   efl_del(start->win);
   free(start);
}

/**
 * @internal
 * @brief Starts a drag operation.
 *
 * This function initiates a drag operation with the given content, action, and seat.
 * It creates a dedicated drag window and manages the lifecycle of the drag.
 *
 * @param obj The Efl_Ui_Dnd object.
 * @param pd Private data for the Efl_Ui_Dnd object.
 * @param content The content to be dragged.
 * @param action A string representing the drag action (e.g., "copy", "move").
 * @param seat The seat ID for the drag operation.
 * @return The drag window object (Efl_Content) if the drag started successfully, otherwise NULL.
 *         The caller does not own this object, it will be deleted when the drag finishes.
 */
EOLIAN static Efl_Content*
_efl_ui_dnd_drag_start(Eo *obj, Efl_Ui_Dnd_Data *pd, Eina_Content *content, const char* action, unsigned int seat)
{
   Eo *drag_win;
   Efl_Ui_Drag_Start *start;
   Efl_Ui_Drag_Started_Event ev = {seat};
   Ecore_Evas *drag_ee;
   EINA_SAFETY_ON_NULL_RETURN_VAL(pd->ee, NULL);

   start = calloc(1, sizeof(Efl_Ui_Drag_Start));
   start->obj = obj;
   start->win = drag_win = elm_win_add(NULL, "Elm-Drag", ELM_WIN_DND);
   elm_win_alpha_set(drag_win, EINA_TRUE);
   elm_win_override_set(drag_win, EINA_TRUE);
   elm_win_borderless_set(drag_win, EINA_TRUE);
   drag_ee = ecore_evas_ecore_evas_get(evas_object_evas_get(drag_win));

   if (!ecore_evas_drag_start(pd->ee, seat, content, drag_ee, action, _ecore_evas_drag_terminated, start))
     {
        efl_del(drag_win);
        free(start);
        drag_win = NULL;
     }
   else
     {
        evas_object_show(drag_win);
        efl_event_callback_call(obj, EFL_UI_DND_EVENT_DRAG_STARTED, &ev);
     }

   return drag_win;
}

/**
 * @internal
 * @brief Sets the offset for the drag window.
 *
 * This function updates the position of the drag window relative to the cursor.
 *
 * @param obj The Efl_Ui_Dnd object (unused).
 * @param pd Private data for the Efl_Ui_Dnd object.
 * @param seat The seat ID for the drag operation.
 * @param size The offset (width and height) to set.
 */
EOLIAN static void
_efl_ui_dnd_drag_offset_set(Eo *obj EINA_UNUSED, Efl_Ui_Dnd_Data *pd, unsigned int seat, Eina_Size2D size)
{
   ecore_evas_drag_offset_set(pd->ee, seat, size);
}

/**
 * @internal
 * @brief Cancels an ongoing drag operation.
 *
 * @param obj The Efl_Ui_Dnd object (unused).
 * @param pd Private data for the Efl_Ui_Dnd object.
 * @param seat The seat ID for the drag operation to cancel.
 */
EOLIAN static void
_efl_ui_dnd_drag_cancel(Eo *obj EINA_UNUSED, Efl_Ui_Dnd_Data *pd, unsigned int seat)
{
   ecore_evas_drag_cancel(pd->ee, seat);
}

/**
 * @internal
 * @brief Retrieves data from a drop operation.
 *
 * This function is called to get the data associated with a drop.
 * It requests the data from the Ecore_Evas selection mechanism.
 *
 * @param obj The Efl_Ui_Dnd object (unused).
 * @param pd Private data for the Efl_Ui_Dnd object.
 * @param seat The seat ID for the drop operation.
 * @param acceptable_types An iterator of MIME types that the drop target can accept.
 *                         Example: An iterator yielding "text/plain", "image/png".
 * @return A future that will resolve with the dropped data (Eina_Content) or an error.
 */
EOLIAN static Eina_Future*
_efl_ui_dnd_drop_data_get(Eo *obj EINA_UNUSED, Efl_Ui_Dnd_Data *pd, unsigned int seat, Eina_Iterator *acceptable_types)
{
   return ecore_evas_selection_get(pd->ee, seat, ECORE_EVAS_SELECTION_BUFFER_DRAG_AND_DROP_BUFFER, acceptable_types);
}

/**
 * @internal
 * @brief Constructor for the Efl_Ui_Dnd object.
 *
 * Initializes the Efl_Ui_Dnd object, primarily by associating it with
 * the Ecore_Evas instance of the Evas object it's mixed into.
 *
 * @param obj The Efl_Ui_Dnd object being constructed.
 * @param pd Private data for the Efl_Ui_Dnd object.
 * @return The constructed Efl_Object, or NULL on failure.
 */
EOLIAN static Efl_Object *
_efl_ui_dnd_efl_object_constructor(Eo *obj, Efl_Ui_Dnd_Data *pd)
{
   if (!efl_constructor(efl_super(obj, EFL_UI_DND_MIXIN)))
     return NULL;

   pd->ee = ecore_evas_ecore_evas_get(evas_object_evas_get(obj));

   return obj;
}

/**
 * @internal
 * @brief Invalidates the Efl_Ui_Dnd object.
 *
 * Performs cleanup when the object is being invalidated. This includes
 * unregistering drop event listeners if they were previously registered.
 *
 * @param obj The Efl_Ui_Dnd object being invalidated.
 * @param pd Private data for the Efl_Ui_Dnd object.
 */
EOLIAN static void
_efl_ui_dnd_efl_object_invalidate(Eo *obj, Efl_Ui_Dnd_Data *pd)
{
   if (pd->registered)
     {
        // Assuming _drop_event_unregister is defined elsewhere and handles
        // unregistering from Ecore_Evas events related to dropping.
        _drop_event_unregister(obj);
     }
   efl_invalidate(efl_super(obj, EFL_UI_DND_MIXIN));

}

/**
 * @internal
 * @brief Macro to check if an event description corresponds to a drop-related event.
 *
 * This macro is used to determine if specific event handling logic for drop
 * events (like registering for Ecore_Evas drop notifications) needs to be activated.
 *
 * @param D The Efl_Event_Description pointer to check.
 */
#define IS_DROP_EVENT(D) ( \
(D == EFL_UI_DND_EVENT_DROP_POSITION_CHANGED) || \
(D == EFL_UI_DND_EVENT_DROP_DROPPED) || \
(D == EFL_UI_DND_EVENT_DROP_LEFT) || \
(D == EFL_UI_DND_EVENT_DROP_ENTERED) \
)

/**
 * @internal
 * @brief Finalizes the Efl_Ui_Dnd object.
 *
 * This function is called when the object is finalized. If drop event listeners
 * were marked as registered (pd->registered is true), this function ensures
 * they are actually registered with the underlying system (e.g., Ecore_Evas).
 *
 * @param obj The Efl_Ui_Dnd object being finalized.
 * @param pd Private data for the Efl_Ui_Dnd object.
 * @return The finalized Efl_Object.
 */
EOLIAN static Efl_Object*
_efl_ui_dnd_efl_object_finalize(Eo *obj, Efl_Ui_Dnd_Data *pd)
{
   if (pd->registered)
     // Assuming _drop_event_register is defined elsewhere and handles
     // registering for Ecore_Evas events related to dropping.
     _drop_event_register(obj);

   return efl_finalize(efl_super(obj, EFL_UI_DND_MIXIN));
}

/**
 * @internal
 * @brief Adds an event callback with a specific priority for the Efl_Ui_Dnd object.
 *
 * This function intercepts event callback additions. If the event being listened to
 * is a drop-related event (checked by IS_DROP_EVENT) and drop events haven't been
 * registered yet, it marks them as registered (pd->registered = EINA_TRUE).
 * If the object is already finalized, it proceeds to actually register the
 * drop event listeners (e.g., with Ecore_Evas via _drop_event_register).
 *
 * @param obj The Efl_Ui_Dnd object.
 * @param pd Private data for the Efl_Ui_Dnd object.
 * @param desc The event description.
 * @param priority The callback priority.
 * @param func The callback function.
 * @param user_data User data for the callback.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_dnd_efl_object_event_callback_priority_add(Eo *obj, Efl_Ui_Dnd_Data *pd,
                                        const Efl_Event_Description *desc,
                                        Efl_Callback_Priority priority,
                                        Efl_Event_Cb func,
                                        const void *user_data)
{
  if (IS_DROP_EVENT(desc) && !pd->registered)
    {
       pd->registered = EINA_TRUE;
       if (efl_finalized_get(obj))
         _drop_event_register(obj);
    }

  return efl_event_callback_priority_add(efl_super(obj, EFL_UI_DND_MIXIN), desc, priority, func, user_data);
}

/**
 * @internal
 * @brief Adds an array of event callbacks with a specific priority for the Efl_Ui_Dnd object.
 *
 * Similar to _efl_ui_dnd_efl_object_event_callback_priority_add, this function
 * handles an array of event callback additions. It iterates through the array,
 * and for any drop-related event, it ensures that drop event registration
 * is triggered if not already done.
 *
 * @param obj The Efl_Ui_Dnd object.
 * @param pd Private data for the Efl_Ui_Dnd object.
 * @param array An array of Efl_Callback_Array_Item.
 *              Example:
 *              @code
 *              static const Efl_Callback_Array_Item event_callbacks[] = {
 *                 { EFL_UI_DND_EVENT_DROP_ENTERED, _my_drop_entered_cb },
 *                 { EFL_UI_DND_EVENT_DROP_DROPPED, _my_drop_dropped_cb },
 *                 { NULL, NULL }
 *              };
 *              @endcode
 * @param priority The callback priority for all items in the array.
 * @param user_data User data for the callbacks.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_dnd_efl_object_event_callback_array_priority_add(Eo *obj, Efl_Ui_Dnd_Data *pd,
                                              const Efl_Callback_Array_Item *array,
                                              Efl_Callback_Priority priority,
                                              const void *user_data)
{
   for (int i = 0; array[i].desc; ++i)
     {
        if (IS_DROP_EVENT(array[i].desc) && !pd->registered)
          {
             pd->registered = EINA_TRUE;
             if (efl_finalized_get(obj))
               // Assuming _drop_event_register is defined elsewhere
               _drop_event_register(obj);
          }
     }
   return efl_event_callback_array_priority_add(efl_super(obj, EFL_UI_DND_MIXIN), array, priority, user_data);
}

/**
 * @internal
 * @brief Defines extra Evas Object operations for Efl_Ui_Dnd.
 *
 * This macro is used to inject custom implementations for standard Efl_Object
 * operations, specifically for event callback additions. This allows the
 * Efl_Ui_Dnd mixin to manage the registration of underlying system (Ecore_Evas)
 * drop events lazily, only when a listener for a drop event is actually added.
 */
#define EFL_UI_DND_EXTRA_OPS \
   EFL_OBJECT_OP_FUNC(efl_event_callback_priority_add, _efl_ui_dnd_efl_object_event_callback_priority_add), \
   EFL_OBJECT_OP_FUNC(efl_event_callback_array_priority_add, _efl_ui_dnd_efl_object_event_callback_array_priority_add), \

#include "efl_ui_dnd.eo.c"
