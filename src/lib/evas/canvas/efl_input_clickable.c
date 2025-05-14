#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#define EFL_INPUT_CLICKABLE_PROTECTED 1

#include <Evas.h>
#include <Ecore.h>
#include "evas_common_private.h"

/**
 * @brief Represents the state of a single button.
 */
typedef struct {
   Eina_Bool pressed; /**< EINA_TRUE if the button is currently pressed, EINA_FALSE otherwise. */
   int pressed_before; /**< Counter for repeated clicks. Incremented if a click occurs within DOUBLE_CLICK_TIME of the previous click. */
   Efl_Loop_Timer *timer; /**< Timer for detecting long press events. */
   double clicked_last_time; /**< Timestamp of the last click event. Used to detect double clicks. */
} Button_State;

/**
 * @brief Private data structure for the Efl_Input_Clickable mixin.
 */
typedef struct {
   Button_State state[3]; /**< Array storing the state for up to 3 buttons (e.g., left, middle, right). */
   Eina_Bool interaction; /**< EINA_TRUE if an interaction (press/unpress) is currently being processed, EINA_FALSE otherwise. Used to prevent re-entrancy issues. */
} Efl_Input_Clickable_Data;

#define MY_CLASS EFL_INPUT_CLICKABLE_MIXIN

#define DOUBLE_CLICK_TIME ((double)0.25) /**< @brief Maximum time in seconds between clicks to be considered a double click. */
#define LONGPRESS_TIMEOUT ((double)1.0) /**< @brief Time in seconds a button must be held to trigger a long press event. */

/**
 * @internal
 * @brief Timer callback function for detecting long press events.
 *
 * This function is called when the long press timer expires. It checks which
 * button's timer triggered the event, cancels the timer, and emits the
 * EFL_INPUT_EVENT_LONGPRESSED event.
 *
 * @param data The Eo object associated with the clickable.
 * @param ev The timer event information.
 */
static void
_timer_longpress(void *data, const Efl_Event *ev)
{
   Button_State *state;
   Efl_Input_Clickable_Data *pd = efl_data_scope_get(data, MY_CLASS);

   for (int i = 0; i < 3; ++i)
     {
        state = &pd->state[i];
        if (state->timer == ev->object)
          {
             efl_del(state->timer);
             state->timer = NULL;
             efl_event_callback_call(data, EFL_INPUT_EVENT_LONGPRESSED, &i);
          }
     }
}

/**
 * @internal
 * @brief Handles a button press event.
 *
 * This function is called when a button is pressed on the clickable object.
 * It updates the button's state, starts a timer for long press detection,
 * and emits the EFL_INPUT_EVENT_PRESSED event.
 *
 * @param obj The Eo object.
 * @param pd The private data for the clickable.
 * @param button The identifier of the pressed button (0, 1, or 2).
 *               Example: 0 for left mouse button, 1 for middle, 2 for right.
 */
EOLIAN static void
_efl_input_clickable_press(Eo *obj EINA_UNUSED, Efl_Input_Clickable_Data *pd, unsigned int button)
{
   Button_State *state;
   EINA_SAFETY_ON_FALSE_RETURN(button < 3);

   pd->interaction = EINA_TRUE;
   INF("Widget %s,%p is pressed(%d)", efl_class_name_get(obj), obj, button);

   state = &pd->state[button];
   EINA_SAFETY_ON_NULL_RETURN(state);

   state->pressed = EINA_TRUE;
   if (state->timer) efl_del(state->timer);
   state->timer = efl_add(EFL_LOOP_TIMER_CLASS, obj,
                                     efl_loop_timer_interval_set(efl_added, LONGPRESS_TIMEOUT),
                                     efl_event_callback_add(efl_added, EFL_LOOP_TIMER_EVENT_TIMER_TICK, _timer_longpress, obj));

   efl_event_callback_call(obj, EFL_INPUT_EVENT_PRESSED, &button);
   pd->interaction = EINA_FALSE;
}

/**
 * @internal
 * @brief Handles a button unpress event.
 *
 * This function is called when a button is released on the clickable object.
 * It updates the button's state, cancels the long press timer, checks for
 * double clicks, and emits EFL_INPUT_EVENT_UNPRESSED, EFL_INPUT_EVENT_CLICKED (for button 1),
 * and EFL_INPUT_EVENT_CLICKED_ANY events as appropriate.
 *
 * @param obj The Eo object.
 * @param pd The private data for the clickable.
 * @param button The identifier of the unpressed button (0, 1, or 2).
 *               Example: 0 for left mouse button, 1 for middle, 2 for right.
 */
EOLIAN static void
_efl_input_clickable_unpress(Eo *obj EINA_UNUSED, Efl_Input_Clickable_Data *pd, unsigned int button)
{
   Efl_Input_Clickable_Clicked clicked;
   Button_State *state;
   Eina_Bool pressed;
   EINA_SAFETY_ON_FALSE_RETURN(button < 3);

   pd->interaction = EINA_TRUE;

   state = &pd->state[button];
   EINA_SAFETY_ON_NULL_RETURN(state);

   INF("Widget %s,%p is unpressed(%d):%d", efl_class_name_get(obj), obj, button, state->pressed);

   //eval if this is a repeated click
   if (state->clicked_last_time > 0.0 && ecore_time_unix_get() - state->clicked_last_time < DOUBLE_CLICK_TIME)
     state->pressed_before++;
   else
     state->pressed_before = 0;
   //reset state
   state->clicked_last_time = ecore_time_unix_get();
   pressed = state->pressed;
   state->pressed = EINA_FALSE;
   if (state->timer)
     efl_del(state->timer);
   state->timer = NULL;

   //populate state
   efl_event_callback_call(obj, EFL_INPUT_EVENT_UNPRESSED, &button);
   if (pressed)
     {
        INF("Widget %s,%p is clicked(%d)", efl_class_name_get(obj), obj, button);
        clicked.repeated = state->pressed_before;
        clicked.button = button;
        if (button == 1)
          efl_event_callback_call(obj, EFL_INPUT_EVENT_CLICKED, &clicked);
        efl_event_callback_call(obj, EFL_INPUT_EVENT_CLICKED_ANY, &clicked);
     }
   pd->interaction = EINA_FALSE;
}

/**
 * @internal
 * @brief Resets the state of a specific button.
 *
 * This function is typically called when a press sequence is aborted (e.g.,
 * mouse leaving the widget while pressed). It cancels any active long press
 * timer and resets the pressed state for the specified button.
 *
 * @param obj The Eo object.
 * @param pd The private data for the clickable.
 * @param button The identifier of the button whose state is to be reset (0, 1, or 2).
 */
EOLIAN static void
_efl_input_clickable_button_state_reset(Eo *obj EINA_UNUSED, Efl_Input_Clickable_Data *pd, unsigned int button)
{
   Button_State *state;
   EINA_SAFETY_ON_FALSE_RETURN(button < 3);

   state = &pd->state[button];
   EINA_SAFETY_ON_NULL_RETURN(state);

   INF("Widget %s,%p is press is aborted(%d):%d", efl_class_name_get(obj), obj, button, state->pressed);

   if (state->timer)
     efl_del(state->timer);
   state->timer = NULL;
   state->pressed = EINA_FALSE;
}

/**
 * @internal
 * @brief Aborts the long press detection for a specific button.
 *
 * This function cancels the long press timer for the specified button.
 * It's used when an action occurs that should prevent a long press event
 * from firing, even if the button is still held down (e.g., mouse pointer moved
 * significantly).
 *
 * @param obj The Eo object.
 * @param pd The private data for the clickable.
 * @param button The identifier of the button whose long press is to be aborted (0, 1, or 2).
 */
EOLIAN static void
_efl_input_clickable_longpress_abort(Eo *obj EINA_UNUSED, Efl_Input_Clickable_Data *pd, unsigned int button)
{
   Button_State *state;
   EINA_SAFETY_ON_FALSE_RETURN(button < 3);

   state = &pd->state[button];
   EINA_SAFETY_ON_NULL_RETURN(state);

   INF("Widget %s,%p - longpress is aborted(%d)", efl_class_name_get(obj), obj, button);

   if (state->timer)
     efl_del(state->timer);
   state->timer = NULL;
}

/**
 * @internal
 * @brief Gets whether an interaction is currently being processed.
 *
 * This is used to prevent re-entrant calls or to understand if a press/unpress
 * sequence is currently active.
 *
 * @param obj The Eo object.
 * @param pd The private data for the clickable.
 * @return @c EINA_TRUE if an interaction is in progress, @c EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_input_clickable_interaction_get(const Eo *obj EINA_UNUSED, Efl_Input_Clickable_Data *pd)
{
   return pd->interaction;
}

#include "efl_input_clickable.eo.c"
