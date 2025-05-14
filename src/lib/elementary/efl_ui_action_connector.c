#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#define EFL_INPUT_CLICKABLE_PROTECTED 1

#include <Efl_Ui.h>
#include "elm_priv.h"

typedef struct {

} Efl_Ui_Action_Connector_Data;

/**
 * @brief Callback function invoked when a "press" signal is emitted from the theme.
 *
 * This function translates a theme-level press signal into a press action
 * on the provided clickable object.
 *
 * @param data The clickable object (Efl_Input_Clickable *) to be pressed.
 * @param obj The Evas_Object that emitted the signal (unused).
 * @param emission The name of the emitted signal (unused).
 * @param source The source of the signal (unused).
 */
static void
_on_press_cb(void *data,
             Evas_Object *obj EINA_UNUSED,
             const char *emission EINA_UNUSED,
             const char *source EINA_UNUSED)
{
   efl_input_clickable_press(data, 1);
}

/**
 * @brief Callback function invoked when an "unpress" signal is emitted from the theme.
 *
 * This function translates a theme-level unpress signal into an unpress action
 * on the provided clickable object.
 *
 * @param data The clickable object (Efl_Input_Clickable *) to be unpressed.
 * @param obj The Evas_Object that emitted the signal (unused).
 * @param emission The name of the emitted signal (unused).
 * @param source The source of the signal (unused).
 */
static void
_on_unpress_cb(void *data,
             Evas_Object *obj EINA_UNUSED,
             const char *emission EINA_UNUSED,
             const char *source EINA_UNUSED)
{
   efl_input_clickable_unpress(data, 1);
}

/**
 * @brief Callback function invoked when a "mouse_out" signal is emitted from the theme.
 *
 * This function resets the button state of the clickable object when the mouse
 * pointer moves out of the theme area associated with the action.
 *
 * @param data The clickable object (Efl_Input_Clickable *) whose state is to be reset.
 * @param obj The Evas_Object that emitted the signal (unused).
 * @param emission The name of the emitted signal (unused).
 * @param source The source of the signal (unused).
 */
static void
_on_mouse_out(void *data,
             Evas_Object *obj EINA_UNUSED,
             const char *emission EINA_UNUSED,
             const char *source EINA_UNUSED)
{
   efl_input_clickable_button_state_reset(data, 1);
}

/**
 * @brief Callback function for pointer move events on the theme object.
 *
 * If the pointer event has already been processed (e.g., by a child widget),
 * this function resets the button state of the associated clickable object.
 * This is to ensure that if the mouse moves away while pressed and the event
 * is consumed elsewhere, the clickable object doesn't remain in a pressed state.
 *
 * @param data The clickable object (Efl_Input_Clickable *) whose state might be reset.
 * @param ev The Efl_Event details, containing the Efl_Input_Pointer information.
 */
static void
_theme_move_cb(void *data, const Efl_Event *ev EINA_UNUSED)
{
   Efl_Input_Pointer *pointer = ev->info;

   if (efl_input_processed_get(pointer))
     efl_input_clickable_button_state_reset(data, 1);
}

EFL_CALLBACKS_ARRAY_DEFINE(bind_clickable_to_theme_callbacks,
  {EFL_EVENT_POINTER_MOVE, _theme_move_cb},
)

/**
 * @brief Binds an Efl_Input_Clickable object's actions to signals from an Efl_Canvas_Layout (theme).
 *
 * This function sets up callbacks so that when the theme object (`object`) emits
 * specific signals (like "efl,action,press", "efl,action,unpress", "efl,action,mouse_out"),
 * the corresponding actions (press, unpress, reset state) are triggered on the
 * `clickable` object. It also handles pointer move events on the theme to reset
 * the clickable's state if necessary.
 *
 * @param object The Efl_Canvas_Layout object (theme) that will emit action signals.
 * @param clickable The Efl_Input_Clickable object that will react to the theme signals.
 */
EOLIAN static void
_efl_ui_action_connector_bind_clickable_to_theme(Efl_Canvas_Layout *object, Efl_Input_Clickable *clickable)
{
   efl_event_callback_array_add(object, bind_clickable_to_theme_callbacks(), clickable);

   efl_layout_signal_callback_add(object, "efl,action,press", "*", clickable, _on_press_cb, NULL);
   efl_layout_signal_callback_add(object, "efl,action,unpress", "*", clickable, _on_unpress_cb, NULL);
   efl_layout_signal_callback_add(object, "efl,action,mouse_out", "*", clickable, _on_mouse_out, NULL);
}

/**
 * @brief Callback function for EFL_EVENT_POINTER_DOWN events on the bound object.
 *
 * This function triggers a press action on the `clickable` object (`data`)
 * if the pointer event has not already been processed.
 *
 * @param data The clickable object (Efl_Input_Clickable *) to be pressed.
 * @param ev The Efl_Event details, containing the Efl_Input_Pointer information.
 */
static void
_press_cb(void *data, const Efl_Event *ev)
{
   Efl_Input_Pointer *pointer = ev->info;
   if (!efl_input_processed_get(pointer))
     {
        efl_input_clickable_press(data, 1);
     }
}

/**
 * @brief Callback function for EFL_EVENT_POINTER_UP events on the bound object.
 *
 * This function handles the unpress logic for the `clickable` object (`data`).
 * It considers several conditions:
 * - If the event was already processed, it resets the clickable's state.
 * - If the mouse pointer is outside the clickable's geometry upon unpress,
 *   it resets the state and, if auto-grab is enabled, also performs an unpress.
 *   This emulates Edje behavior where a click is not registered if the mouse
 *   is released outside the object after being pressed inside.
 * - Otherwise (pointer is inside and event not processed), it performs a normal unpress.
 *
 * @param data The clickable object (Efl_Input_Clickable *) to be unpressed or have its state reset.
 * @param ev The Efl_Event details, containing the Efl_Input_Pointer information.
 */
static void
_unpress_cb(void *data, const Efl_Event *ev EINA_UNUSED)
{
   Efl_Input_Pointer *pointer = ev->info;
   Eina_Position2D mouse_pos = efl_input_pointer_position_get(pointer);
   Eina_Rect geom = efl_gfx_entity_geometry_get(data);
   if (efl_input_processed_get(pointer))
     {
        efl_input_clickable_button_state_reset(data, 1);
     }
   else if (!eina_rectangle_coords_inside(&geom.rect, mouse_pos.x, mouse_pos.y))
     {
        //we are emulating edje behavior here, do press unpress on the event, but not click
        efl_input_clickable_button_state_reset(data, 1);
        if (efl_canvas_object_pointer_mode_get(data) == EFL_INPUT_OBJECT_POINTER_MODE_AUTO_GRAB)
          {
             efl_input_clickable_unpress(data, 1);
          }
     }
   else
     {
        efl_input_clickable_unpress(data, 1);
     }
}

EFL_CALLBACKS_ARRAY_DEFINE(bind_clickable_to_object_callbacks,
  {EFL_EVENT_POINTER_DOWN, _press_cb},
  {EFL_EVENT_POINTER_UP, _unpress_cb},
)

/**
 * @brief Binds an Efl_Input_Clickable object's actions to input events from an Efl_Input_Interface object.
 *
 * This function sets up callbacks so that when the `object` (which implements
 * Efl_Input_Interface) receives pointer down (press) and pointer up (unpress) events,
 * the corresponding actions are triggered on the `clickable` object.
 *
 * @param object The Efl_Input_Interface object that will generate input events.
 * @param clickable The Efl_Input_Clickable object that will react to the input events.
 */
EOLIAN static void
_efl_ui_action_connector_bind_clickable_to_object(Efl_Input_Interface *object, Efl_Input_Clickable *clickable)
{
   efl_event_callback_array_add(object, bind_clickable_to_object_callbacks(), clickable);
}


#include "efl_ui_action_connector.eo.c"
