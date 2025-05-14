#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#define ECORE_EVAS_INTERNAL

#include <string.h>
#include <stdlib.h>

#include <Ecore.h>
#include <Ecore_Input.h>

#include "Ecore_Input_Evas.h"
#include "ecore_input_evas_private.h"

int _ecore_input_evas_log_dom = -1; /**< Log domain for ecore_input_evas */

/**
 * @brief Structure to hold information about a window registered for input events.
 */
typedef struct _Ecore_Input_Window Ecore_Input_Window;
struct _Ecore_Input_Window
{
   Evas *evas; /**< The Evas canvas associated with the window */
   void *window; /**< The native window handle */
   Ecore_Event_Mouse_Move_Cb move_mouse; /**< Callback for mouse move events */
   Ecore_Event_Multi_Move_Cb move_multi; /**< Callback for multi-touch move events */
   Ecore_Event_Multi_Down_Cb down_multi; /**< Callback for multi-touch down events */
   Ecore_Event_Multi_Up_Cb up_multi;     /**< Callback for multi-touch up events */
   Ecore_Event_Direct_Input_Cb direct;   /**< Callback for direct input events, bypassing Evas */
   int ignore_event; /**< Flag to indicate if events for this window should be ignored */
};

/**
 * @brief Represents the state of an input event, typically for mouse buttons.
 */
typedef enum _Ecore_Input_State {
  ECORE_INPUT_NONE = 0, /**< No specific input state */
  ECORE_INPUT_DOWN,     /**< Input (e.g., button) is currently pressed down */
  ECORE_INPUT_MOVE,     /**< Input (e.g., mouse) is moving while a button is pressed */
  ECORE_INPUT_UP,       /**< Input (e.g., button) has been released */
  ECORE_INPUT_CANCEL    /**< Input event has been cancelled */
} Ecore_Input_State;

/**
 * @brief Defines actions to take based on the current and previous input event state.
 * This is used to handle potentially erroneous or out-of-order input sequences.
 */
typedef enum _Ecore_Input_Action {
  ECORE_INPUT_CONTINUE = 0, /**< Continue processing the event normally */
  ECORE_INPUT_IGNORE,       /**< Ignore the current event */
  ECORE_INPUT_FAKE_UP       /**< Generate a fake "up" event before processing the current event */
} Ecore_Input_Action;

/**
 * @brief Structure to store information about the last mouse button event.
 * This is used for detecting double/triple clicks and handling event sequences.
 */
typedef struct _Ecore_Input_Last Ecore_Event_Last;
struct _Ecore_Input_Last
{
   Ecore_Event_Mouse_Button *ev; /**< The last mouse button event data */
   Ecore_Timer *timer;           /**< Timer for handling event timeouts (e.g., for fake up events) */
   Evas_Device *evas_device;     /**< The Evas device associated with the event */

   unsigned int device;          /**< The device ID */
   unsigned int buttons;         /**< The button(s) involved in the event */
   Ecore_Input_State state;      /**< The current state of the input (down, up, etc.) */
   Ecore_Window win;             /**< The window where the event occurred */

   Eina_Bool faked : 1;          /**< Flag indicating if the last event was a faked one */
};

static int _ecore_event_evas_init_count = 0; /**< Counter for ecore_event_evas_init() calls */
static Ecore_Event_Handler *ecore_event_evas_handlers[10]; /**< Array of Ecore event handlers */
static Eina_Hash *_window_hash = NULL; /**< Hash table mapping Ecore_Window IDs to Ecore_Input_Window structures */

static Eina_List *_last_events = NULL; /**< List of Ecore_Event_Last structures for tracking recent button events */
static double _last_events_timeout = 0.5; /**< Timeout in seconds for generating fake UP events if no real UP event is received */
static Eina_Bool _last_events_enable = EINA_FALSE; /**< Flag to enable the fake UP event generation logic */

static Eina_Bool _ecore_event_evas_mouse_button(Ecore_Event_Mouse_Button *e,
                                                Ecore_Event_Press press,
                                                Eina_Bool faked);

/**
 * @brief Checks the consistency of the current mouse button event against the last recorded state.
 *
 * This function determines if the current event is valid in sequence or if
 * corrective actions (like faking an UP event or ignoring the current event)
 * are needed. This helps to handle cases where input events might be
 * received out of order or duplicated.
 *
 * @param eel Pointer to the Ecore_Event_Last structure holding the state of the last event.
 * @param press The type of the current press event (ECORE_DOWN, ECORE_UP, ECORE_CANCEL).
 * @return Ecore_Input_Action indicating how to proceed with the current event.
 */
static Ecore_Input_Action
_ecore_event_last_check(Ecore_Event_Last *eel, Ecore_Event_Press press)
{
   switch (eel->state)
     {
      case ECORE_INPUT_NONE:
         /* 1. ECORE_INPUT_NONE => ECORE_UP : impossible
          * 2. ECORE_INPUT_NONE => ECORE_CANCEL : impossible
          * 3. ECORE_INPUT_NONE => ECORE_DOWN : ok
          */
         return ECORE_INPUT_CONTINUE;

      case ECORE_INPUT_DOWN:
         /* 1. ECORE_INPUT_DOWN => ECORE_UP : ok
          * 2. ECORE_INPUT_DOWN => ECORE_CANCEL : ok
          */
         if ((press == ECORE_UP) || (press == ECORE_CANCEL))
           return ECORE_INPUT_CONTINUE;

         /* 3. ECORE_INPUT_DOWN => ECORE_DOWN : emit a faked UP then */
         INF("Down event occurs twice. device(%d), button(%d)", eel->device, eel->buttons);
         return ECORE_INPUT_FAKE_UP;

      case ECORE_INPUT_MOVE:
         /* 1. ECORE_INPUT_MOVE => ECORE_UP : ok
          * 2. ECORE_INPUT_MOVE => ECORE_CANCEL : ok
          */
         if ((press == ECORE_UP) || (press == ECORE_CANCEL))
           return ECORE_INPUT_CONTINUE;

         /* 3. ECORE_INPUT_MOVE => ECORE_DOWN : ok
          * FIXME: handle fake button up and push for more delay here */
         //TODO: How to deal with down event after move event?
         INF("Down event occurs after move event. device(%d), button(%d)", eel->device, eel->buttons);
         return ECORE_INPUT_FAKE_UP;

      case ECORE_INPUT_UP:
      case ECORE_INPUT_CANCEL:
          /* 1. ECORE_INPUT_UP     => ECORE_DOWN : ok */
          /* 2. ECORE_INPUT_CANCEL => ECORE_DOWN : ok */
         if (press == ECORE_DOWN)
           return ECORE_INPUT_CONTINUE;

          /* 3. ECORE_INPUT_UP     => ECORE_UP :  ignore */
          /* 4. ECORE_INPUT_UP     => ECORE_CANCEL : ignore */
          /* 5. ECORE_INPUT_CANCEL => ECORE_UP : ignore */
          /* 6. ECORE_INPUT_CANCEL => ECORE_CANCEL : ignore */
         INF("Up/cancel event occurs after up/cancel event. device(%d), button(%d)", eel->device, eel->buttons);
         return ECORE_INPUT_IGNORE;
     }
  return ECORE_INPUT_IGNORE;
}

/**
 * @brief Looks up or creates an Ecore_Event_Last structure for a given device, button, and window.
 *
 * This function searches a global list (_last_events) for an existing
 * Ecore_Event_Last entry that matches the provided Evas device, device ID,
 * button ID, and window. If an entry is found, it is returned.
 * If no entry is found and `create_new` is EINA_TRUE, a new Ecore_Event_Last
 * structure is allocated, initialized, and added to the list before being returned.
 *
 * @param evas_device The Evas_Device associated with the event.
 * @param device The device ID.
 * @param buttons The button ID.
 * @param win The Ecore_Window where the event occurred.
 * @param create_new If EINA_TRUE, a new entry will be created if one is not found.
 * @return Pointer to the found or newly created Ecore_Event_Last structure, or NULL on failure or if not found and create_new is EINA_FALSE.
 */
static Ecore_Event_Last *
_ecore_event_evas_lookup(Evas_Device *evas_device, unsigned int device,
                         unsigned int buttons, Ecore_Window win,
                         Eina_Bool create_new)
{
   Ecore_Event_Last *eel;
   Eina_List *l;

   //the number of last event is small, simple check is ok.
   EINA_LIST_FOREACH(_last_events, l, eel)
     if ((eel->device == device) && (eel->buttons == buttons) && (eel->evas_device == evas_device))
       return eel;
   if (!create_new) return NULL;
   eel = malloc(sizeof (Ecore_Event_Last));
   if (!eel) return NULL;

   eel->timer = NULL;
   eel->ev = NULL;
   eel->device = device;
   eel->buttons = buttons;
   eel->state = ECORE_INPUT_NONE;
   eel->faked = EINA_FALSE;
   eel->win = win;
   eel->evas_device = evas_device;

   _last_events = eina_list_append(_last_events, eel);
   return eel;
}

/**
 * @brief Timer callback to generate a fake mouse button UP event.
 *
 * This function is called when a timer expires, indicating that a mouse button
 * was pressed (ECORE_INPUT_DOWN) or moved while pressed (ECORE_INPUT_MOVE),
 * but no corresponding UP event was received within the `_last_events_timeout` period.
 * It then calls _ecore_event_evas_mouse_button() to inject a faked UP event.
 *
 * @param data Pointer to the Ecore_Event_Last structure associated with the timed-out event.
 * @return EINA_FALSE to indicate the timer should not be rescheduled.
 */
static Eina_Bool
_ecore_event_evas_push_fake(void *data)
{
   Ecore_Event_Last *eel = data;

   switch (eel->state)
     {
      case ECORE_INPUT_NONE:
      case ECORE_INPUT_UP:
      case ECORE_INPUT_CANCEL:
         /* should not happen */
         break;
      case ECORE_INPUT_DOWN:
         /* use the saved Ecore_Event */
         /* No up event since timeout started ... */
      case ECORE_INPUT_MOVE:
         /* No up event since timeout started ... */
         _ecore_event_evas_mouse_button(eel->ev, ECORE_UP, EINA_TRUE);
         eel->faked = EINA_TRUE;
         break;
     }

   free(eel->ev);
   eel->ev = NULL;
   eel->timer = NULL;
   return EINA_FALSE;
}

/**
 * @brief Processes and validates a mouse button event before feeding it to Evas.
 *
 * This function is a core part of handling mouse button events. It:
 * 1. Looks up the last event state for the given device and button.
 * 2. Checks the validity of the current event sequence using `_ecore_event_last_check`.
 * 3. Based on the check, it might generate a fake UP event if necessary.
 * 4. Updates the last event state (e.g., to ECORE_INPUT_DOWN or ECORE_INPUT_UP).
 * 5. If `_last_events_enable` is true and a timeout is set, it starts a timer
 *    for DOWN events to potentially generate a fake UP event later if no
 *    actual UP event is received. For UP events, it clears any existing timer.
 *
 * @param e The Ecore_Event_Mouse_Button event data.
 * @param press The type of press (ECORE_DOWN or ECORE_UP).
 * @return EINA_TRUE if the event is considered valid and should be processed further,
 *         EINA_FALSE if the event is invalid or should be ignored.
 */
static Eina_Bool
_ecore_event_evas_push_mouse_button(Ecore_Event_Mouse_Button *e, Ecore_Event_Press press)
{
   Ecore_Event_Last *eel;
   Ecore_Input_Action action = ECORE_INPUT_CONTINUE;

   //_ecore_event_evas_mouse_button already check press or cancel without history
   eel = _ecore_event_evas_lookup(e->dev, e->multi.device, e->buttons, e->window, EINA_TRUE);
   if (!eel) return EINA_FALSE;
   INF("dev(%d), button(%d), last_press(%d), press(%d)", e->multi.device, e->buttons, eel->state, press);

   if (e->window == eel->win)
     action = _ecore_event_last_check(eel, press);
   INF("action(%d)", action);
   switch (action)
     {
      case ECORE_INPUT_FAKE_UP:
         _ecore_event_evas_mouse_button(e, ECORE_UP, EINA_TRUE);
      case ECORE_INPUT_CONTINUE:
         break;
      case ECORE_INPUT_IGNORE:
      default:
        eel->win = e->window;
        return EINA_FALSE;
     }

   switch (press)
     {
      case ECORE_DOWN:
        eel->state = ECORE_INPUT_DOWN;
        break;
      case ECORE_UP:
        eel->state = ECORE_INPUT_UP;
        break;
      default:
        break;
     }
   eel->win = e->window;

   //if up event not occurs from under layers of ecore
   //up event is generated by ecore
   if (_last_events_enable &&
       !EINA_DBL_EQ(_last_events_timeout, 0))
     {
        if (eel->timer) ecore_timer_del(eel->timer);
        eel->timer = NULL;
        if (press == ECORE_DOWN)
          {
             /* Save the Ecore_Event somehow */
             if (!eel->ev) eel->ev = malloc(sizeof (Ecore_Event_Mouse_Button));
             if (!eel->ev) return EINA_FALSE;
             memcpy(eel->ev, e, sizeof (Ecore_Event_Mouse_Button));
             eel->timer = ecore_timer_add(_last_events_timeout, _ecore_event_evas_push_fake, eel);
          }
        else
          {
             free(eel->ev);
             eel->ev = NULL;
          }
     }
   return EINA_TRUE;
}

/**
 * @brief Processes mouse move events, primarily to update the state for fake event generation.
 *
 * If `_last_events_enable` is active, this function iterates through the tracked
 * last button events. If a button is currently in a DOWN or MOVE state,
 * this function resets its associated timer (delaying a potential fake UP event)
 * and updates the stored event's coordinates to the current mouse position.
 * It also changes the state to ECORE_INPUT_MOVE.
 *
 * @param e The Ecore_Event_Mouse_Move event data.
 */
static void
_ecore_event_evas_push_mouse_move(Ecore_Event_Mouse_Move *e)
{
   Ecore_Event_Last *eel;
   Eina_List *l;

   if (!_last_events_enable) return;

   EINA_LIST_FOREACH(_last_events, l, eel)
     switch (eel->state)
       {
        case ECORE_INPUT_NONE:
        case ECORE_INPUT_UP:
        case ECORE_INPUT_CANCEL:
           /* (none, up, or cancel) => move, sounds fine to me */
           break;
        case ECORE_INPUT_DOWN:
        case ECORE_INPUT_MOVE:
           /* Down and moving, let's see */
           if (eel->ev)
             {
                /* Add some delay to the timer */
                ecore_timer_reset(eel->timer);
                /* Update position */
                eel->ev->x = e->x;
                eel->ev->y = e->y;
                eel->ev->root.x = e->root.x;
                eel->ev->root.y = e->root.y;
                eel->state = ECORE_INPUT_MOVE;
                break;
             }
           /* FIXME: Timer did expire, do something maybe */
           break;
       }
}

/**
 * @brief Updates Evas seat key modifiers and locks based on Ecore event modifiers.
 *
 * This function translates Ecore modifier and lock flags (like Shift, Ctrl, Alt,
 * Caps Lock, Num Lock) into Evas seat-specific key modifier/lock states.
 *
 * @param e The Evas canvas.
 * @param modifiers A bitmask of Ecore_Event_Modifier and Ecore_Event_Lock flags.
 * @param seat The Evas_Device representing the seat, or NULL for the default seat.
 */
EAPI void
ecore_event_evas_seat_modifier_lock_update(Evas *e, unsigned int modifiers,
                                           Evas_Device *seat)
{
   if (modifiers & ECORE_EVENT_MODIFIER_SHIFT)
     evas_seat_key_modifier_on(e, "Shift", seat);
   else evas_seat_key_modifier_off(e, "Shift", seat);

   if (modifiers & ECORE_EVENT_MODIFIER_CTRL)
     evas_seat_key_modifier_on(e, "Control", seat);
   else evas_seat_key_modifier_off(e, "Control", seat);

   if (modifiers & ECORE_EVENT_MODIFIER_ALT)
     evas_seat_key_modifier_on(e, "Alt", seat);
   else evas_seat_key_modifier_off(e, "Alt", seat);

   if (modifiers & ECORE_EVENT_MODIFIER_WIN)
     {
        evas_seat_key_modifier_on(e, "Super", seat);
        evas_seat_key_modifier_on(e, "Hyper", seat);
     }
   else
     {
        evas_seat_key_modifier_off(e, "Super", seat);
        evas_seat_key_modifier_off(e, "Hyper", seat);
     }

   if (modifiers & ECORE_EVENT_MODIFIER_ALTGR)
     evas_seat_key_modifier_on(e, "AltGr", seat);
   else evas_seat_key_modifier_off(e, "AltGr", seat);

   if (modifiers & ECORE_EVENT_LOCK_SCROLL)
     evas_seat_key_lock_on(e, "Scroll_Lock", seat);
   else evas_seat_key_lock_off(e, "Scroll_Lock", seat);

   if (modifiers & ECORE_EVENT_LOCK_NUM)
     evas_seat_key_lock_on(e, "Num_Lock", seat);
   else evas_seat_key_lock_off(e, "Num_Lock", seat);

   if (modifiers & ECORE_EVENT_LOCK_CAPS)
     evas_seat_key_lock_on(e, "Caps_Lock", seat);
   else evas_seat_key_lock_off(e, "Caps_Lock", seat);

   if (modifiers & ECORE_EVENT_LOCK_SHIFT)
     evas_seat_key_lock_on(e, "Shift_Lock", seat);
   else evas_seat_key_lock_off(e, "Shift_Lock", seat);
}

/**
 * @brief Updates Evas key modifiers and locks based on Ecore event modifiers for the default seat.
 *
 * This is a convenience wrapper around ecore_event_evas_seat_modifier_lock_update,
 * passing NULL for the seat to affect the default Evas seat.
 *
 * @param e The Evas canvas.
 * @param modifiers A bitmask of Ecore_Event_Modifier and Ecore_Event_Lock flags.
 */
EAPI void
ecore_event_evas_modifier_lock_update(Evas *e, unsigned int modifiers)
{
   ecore_event_evas_seat_modifier_lock_update(e, modifiers, NULL);
}

/**
 * @brief Registers an Ecore_Window with an Evas canvas for input event handling.
 *
 * This function associates an Ecore_Window ID with a native window handle,
 * an Evas canvas, and optional custom callbacks for specific input events.
 * It stores this information in the `_window_hash` for later retrieval
 * when Ecore input events are received. It also adds standard Evas key
 * modifiers and locks to the Evas canvas.
 *
 * @param id The Ecore_Window ID.
 * @param window The native window handle (e.g., X11 Window, Wayland surface).
 * @param evas The Evas canvas associated with the window.
 * @param move_mouse Optional callback for mouse move events. If NULL, default Evas feeding is used.
 * @param move_multi Optional callback for multi-touch move events. If NULL, default Evas feeding is used.
 * @param down_multi Optional callback for multi-touch down events. If NULL, default Evas feeding is used.
 * @param up_multi Optional callback for multi-touch up events. If NULL, default Evas feeding is used.
 */
EAPI void
ecore_event_window_register(Ecore_Window id, void *window, Evas *evas,
                            Ecore_Event_Mouse_Move_Cb move_mouse,
                            Ecore_Event_Multi_Move_Cb move_multi,
                            Ecore_Event_Multi_Down_Cb down_multi,
                            Ecore_Event_Multi_Up_Cb up_multi)
{
   Ecore_Input_Window *w;

   w = calloc(1, sizeof(Ecore_Input_Window));
   if (!w) return;

   w->evas = evas;
   w->window = window;
   w->move_mouse = move_mouse;
   w->move_multi = move_multi;
   w->down_multi = down_multi;
   w->up_multi = up_multi;
   w->ignore_event = 0;

   eina_hash_add(_window_hash, &id, w);

   evas_key_modifier_add(evas, "Shift");
   evas_key_modifier_add(evas, "Control");
   evas_key_modifier_add(evas, "Alt");
   evas_key_modifier_add(evas, "Meta");
   evas_key_modifier_add(evas, "Hyper");
   evas_key_modifier_add(evas, "Super");
   evas_key_modifier_add(evas, "AltGr");
   evas_key_lock_add(evas, "Caps_Lock");
   evas_key_lock_add(evas, "Num_Lock");
   evas_key_lock_add(evas, "Scroll_Lock");
}

/**
 * @brief Unregisters an Ecore_Window from input event handling.
 *
 * Removes the window's registration information from the `_window_hash`.
 *
 * @param id The Ecore_Window ID to unregister.
 */
EAPI void
ecore_event_window_unregister(Ecore_Window id)
{
   eina_hash_del(_window_hash, &id, NULL);
}

/**
 * @brief Sets a direct input callback for a registered window.
 *
 * This allows an application to receive raw Ecore input events for a specific
 * window before (or instead of) they are fed to Evas. If the callback
 * handles the event and returns EINA_TRUE, the event is not fed to Evas.
 *
 * @param id The Ecore_Window ID.
 * @param fptr The callback function pointer of type Ecore_Event_Direct_Input_Cb.
 *             Example: `Eina_Bool my_direct_cb(void *window, Ecore_Event_Type type, void *event)`
 *                      `window` is the `window` parameter from `ecore_event_window_register`.
 *                      `type` is the Ecore event type (e.g., ECORE_EVENT_KEY_DOWN).
 *                      `event` is a pointer to the Ecore event structure.
 *                      Return EINA_TRUE to consume the event, EINA_FALSE to let it pass to Evas.
 */
EAPI void
_ecore_event_window_direct_cb_set(Ecore_Window id, Ecore_Event_Direct_Input_Cb fptr)
{
   Ecore_Input_Window *lookup;

   lookup = eina_hash_find(_window_hash, &id);
   if (!lookup) return;
   lookup->direct = fptr;
}

/**
 * @brief Retrieves the native window handle associated with an Ecore_Window ID.
 *
 * @param id The Ecore_Window ID.
 * @return The native window handle (e.g., X11 Window) if found, otherwise NULL.
 */
EAPI void *
ecore_event_window_match(Ecore_Window id)
{
   Ecore_Input_Window *lookup;

   lookup = eina_hash_find(_window_hash, &id);
   if (lookup) return lookup->window;
   return NULL;
}

/**
 * @brief Sets whether to ignore input events for a specific registered window.
 *
 * If `ignore_event` is non-zero, subsequent input events for this window
 * will be passed on by `_ecore_event_window_match` and thus not processed
 * by the Evas event feeding logic in this module.
 *
 * @param id The Ecore_Window ID.
 * @param ignore_event A non-zero value to ignore events, 0 to process them.
 */
EAPI void
ecore_event_window_ignore_events(Ecore_Window id, int ignore_event)
{
   Ecore_Input_Window *lookup;

   lookup = eina_hash_find(_window_hash, &id);
   if (!lookup) return;
   lookup->ignore_event = ignore_event;
}

/**
 * @brief Internal function to find a registered Ecore_Input_Window, considering the ignore_event flag.
 *
 * This function retrieves the Ecore_Input_Window structure associated with the
 * given Ecore_Window ID. If the window is found and its `ignore_event` flag
 * is not set, the structure is returned. Otherwise, NULL is returned.
 *
 * @param id The Ecore_Window ID.
 * @return Pointer to the Ecore_Input_Window structure if found and not ignoring events, otherwise NULL.
 */
static Ecore_Input_Window*
_ecore_event_window_match(Ecore_Window id)
{
   Ecore_Input_Window *lookup;

   lookup = eina_hash_find(_window_hash, &id);
   if (!lookup) return NULL;
   if (lookup->ignore_event) return NULL; /* Pass on event. */
   return lookup;
}

/**
 * @brief Handles Ecore key down and key up events and feeds them to Evas.
 *
 * This function is called by Ecore event handlers for key events. It:
 * 1. Finds the registered Ecore_Input_Window for the event.
 * 2. Updates Evas modifier/lock states.
 * 3. If a direct input callback is registered for the window, it's called.
 *    If the callback handles the event (returns EINA_TRUE), Evas feeding is skipped.
 * 4. Otherwise, it feeds the key event (down or up) to the Evas canvas
 *    using `evas_event_feed_key_down_with_keycode` or `evas_event_feed_key_up_with_keycode`.
 *
 * @param e The Ecore_Event_Key event data.
 * @param press Indicates whether it's a key down (ECORE_DOWN) or key up (ECORE_UP) event.
 * @return ECORE_CALLBACK_PASS_ON to allow other handlers to process the event.
 */
static Eina_Bool
_ecore_event_evas_key(Ecore_Event_Key *e, Ecore_Event_Press press)
{
   Ecore_Input_Window *lookup;
   Eo *seat;

   lookup = _ecore_event_window_match(e->event_window);
   if (!lookup) return ECORE_CALLBACK_PASS_ON;
   seat = e->dev ? efl_input_device_seat_get(e->dev) : NULL;
   ecore_event_evas_seat_modifier_lock_update(lookup->evas, e->modifiers, seat);
   if (press == ECORE_DOWN)
     {
        if (!lookup->direct ||
            !lookup->direct(lookup->window, ECORE_EVENT_KEY_DOWN, e))
          {
             evas_event_feed_key_down_with_keycode(lookup->evas,
                                                   e->keyname,
                                                   e->key,
                                                   e->string,
                                                   e->compose,
                                                   e->timestamp,
                                                   e->data,
                                                   e->keycode);
          }
     }
   else
     {
        if (!lookup->direct ||
            !lookup->direct(lookup->window, ECORE_EVENT_KEY_UP, e))
          {
             evas_event_feed_key_up_with_keycode(lookup->evas,
                                                 e->keyname,
                                                 e->key,
                                                 e->string,
                                                 e->compose,
                                                 e->timestamp,
                                                 e->data,
                                                 e->keycode);
          }
     }

   return ECORE_CALLBACK_PASS_ON;
}

/**
 * @brief Handles Ecore mouse button cancel events and feeds them to Evas.
 *
 * This function is called when a mouse button event sequence is cancelled
 * (e.g., due to a touch gesture ending unexpectedly). It:
 * 1. Finds the registered Ecore_Input_Window.
 * 2. If a direct input callback is set, calls it. If it handles the event,
 *    Evas feeding is skipped.
 * 3. Otherwise, feeds a mouse cancel event to Evas.
 * 4. Updates the state of tracked last button events to ECORE_INPUT_CANCEL
 *    if their current state allows for a cancel.
 *
 * @param e The Ecore_Event_Mouse_Button event data (though it's a cancel event).
 * @return ECORE_CALLBACK_PASS_ON.
 */
static Eina_Bool
_ecore_event_evas_mouse_button_cancel(Ecore_Event_Mouse_Button *e)
{
   Ecore_Input_Window *lookup;
   Ecore_Event_Last *eel;
   Eina_List *l;

   lookup = _ecore_event_window_match(e->event_window);
   if (!lookup) return ECORE_CALLBACK_PASS_ON;

   INF("ButtonEvent cancel, device(%d), button(%d)", e->multi.device, e->buttons);
   if (!lookup->direct ||
       !lookup->direct(lookup->window, ECORE_EVENT_MOUSE_BUTTON_CANCEL, e))
     {
        evas_event_feed_mouse_cancel(lookup->evas, e->timestamp, NULL);
     }

   //the number of last event is small, simple check is ok.
   EINA_LIST_FOREACH(_last_events, l, eel)
     {
        Ecore_Input_Action act = _ecore_event_last_check(eel, ECORE_CANCEL);
        INF("ButtonEvent cancel, dev(%d), button(%d), last_press(%d), action(%d)", eel->device, eel->buttons, eel->state, act);
        if (act == ECORE_INPUT_CONTINUE) eel->state = ECORE_INPUT_CANCEL;
     }

   return ECORE_CALLBACK_PASS_ON;
}

/**
 * @brief Handles Ecore mouse button down and up events, validates them, and feeds them to Evas.
 *
 * This is the primary function for processing mouse button presses and releases. It:
 * 1. Finds the registered Ecore_Input_Window.
 * 2. Determines Evas button flags (double/triple click).
 * 3. For UP or CANCEL events (implicitly, as CANCEL is handled by a separate function but shares some logic here if called directly),
 *    it checks if there's a valid preceding DOWN/MOVE state. If not, the event might be ignored.
 * 4. If the event is not faked, it calls `_ecore_event_evas_push_mouse_button` to
 *    validate the event sequence and manage timers for potential fake UP events.
 *    If `_ecore_event_evas_push_mouse_button` indicates the event is invalid, it's ignored.
 * 5. Updates Evas modifier/lock states.
 * 6. If a direct input callback is registered, it's called. If it handles the event, Evas feeding is skipped.
 * 7. Otherwise, it feeds the mouse button event (down or up) to Evas, either as a
 *    standard mouse event or a multi-touch event depending on `e->multi.device`.
 *    It uses custom multi-touch callbacks if they were provided during window registration.
 *
 * @param e The Ecore_Event_Mouse_Button event data.
 * @param press Indicates ECORE_DOWN or ECORE_UP.
 * @param faked EINA_TRUE if this is a synthetically generated event (e.g., a fake UP), EINA_FALSE otherwise.
 * @return ECORE_CALLBACK_PASS_ON.
 */
static Eina_Bool
_ecore_event_evas_mouse_button(Ecore_Event_Mouse_Button *e, Ecore_Event_Press press, Eina_Bool faked)
{
   Ecore_Event_Last *eel;
   Ecore_Input_Window *lookup;
   Evas_Button_Flags flags = EVAS_BUTTON_NONE;

   lookup = _ecore_event_window_match(e->event_window);
   if (!lookup) return ECORE_CALLBACK_PASS_ON;
   if (e->double_click) flags |= EVAS_BUTTON_DOUBLE_CLICK;
   if (e->triple_click) flags |= EVAS_BUTTON_TRIPLE_CLICK;
   INF("\tButtonEvent:ecore_event_evas press(%d), device(%d), button(%d), fake(%d)", press, e->multi.device, e->buttons, faked);

   //handle all mouse error from under layers of ecore
   //error handle
   // 1. ecore up without ecore down
   // 2. ecore cancel without ecore down
   if (press != ECORE_DOWN)
     {
        //ECORE_UP or ECORE_CANCEL
        eel = _ecore_event_evas_lookup(e->dev, e->multi.device, e->buttons, e->window, EINA_FALSE);
        if (!eel)
          {
             WRN("ButtonEvent has no history.");
             return ECORE_CALLBACK_PASS_ON;
          }

        if ((e->window == eel->win) &&
            ((eel->state == ECORE_INPUT_UP) ||
             (eel->state == ECORE_INPUT_CANCEL)))
          {
             WRN("ButtonEvent has wrong history. Last state=%d", eel->state);
             return ECORE_CALLBACK_PASS_ON;
          }
     }

   if (!faked)
     {
        Eina_Bool ret = EINA_FALSE;
        ret = _ecore_event_evas_push_mouse_button(e, press);
        /* This ButtonEvent is worng */
        if (!ret) return ECORE_CALLBACK_PASS_ON;
     }

   if (e->multi.device == 0)
     {
        Eo *seat = e->dev ? efl_input_device_seat_get(e->dev) : NULL;
        ecore_event_evas_seat_modifier_lock_update(lookup->evas, e->modifiers, seat);
        if (press == ECORE_DOWN)
          {
             if (!lookup->direct ||
                 !lookup->direct(lookup->window, ECORE_EVENT_MOUSE_BUTTON_DOWN, e))
               {
                  evas_event_feed_mouse_down(lookup->evas, e->buttons, flags,
                                             e->timestamp, NULL);
               }
          }
        else
          {
             if (!lookup->direct ||
                 !lookup->direct(lookup->window, ECORE_EVENT_MOUSE_BUTTON_UP, e))
               {
                  evas_event_feed_mouse_up(lookup->evas, e->buttons, flags,
                                           e->timestamp, NULL);
               }
          }
     }
   else
     {
        if (press == ECORE_DOWN)
          {
             if (!lookup->direct ||
                 !lookup->direct(lookup->window, ECORE_EVENT_MOUSE_BUTTON_DOWN, e))
               {
                  if (lookup->down_multi)
                    lookup->down_multi(lookup->window, e->multi.device,
                                       e->x, e->y, e->multi.radius,
                                       e->multi.radius_x, e->multi.radius_y,
                                       e->multi.pressure, e->multi.angle,
                                       e->multi.x, e->multi.y, flags,
                                       e->timestamp);
                  else
                     evas_event_input_multi_down(lookup->evas, e->multi.device,
                                                 e->x, e->y, e->multi.radius,
                                                 e->multi.radius_x, e->multi.radius_y,
                                                 e->multi.pressure, e->multi.angle,
                                                 e->multi.x, e->multi.y, flags,
                                                 e->timestamp, NULL);
               }
          }
        else
          {
             if (!lookup->direct ||
                 !lookup->direct(lookup->window, ECORE_EVENT_MOUSE_BUTTON_UP, e))
               {
                  if (lookup->up_multi)
                    lookup->up_multi(lookup->window, e->multi.device,
                                     e->x, e->y, e->multi.radius,
                                     e->multi.radius_x, e->multi.radius_y,
                                     e->multi.pressure, e->multi.angle,
                                     e->multi.x, e->multi.y, flags,
                                     e->timestamp);
                  else
                     evas_event_input_multi_up(lookup->evas, e->multi.device,
                                               e->x, e->y, e->multi.radius,
                                               e->multi.radius_x, e->multi.radius_y,
                                               e->multi.pressure, e->multi.angle,
                                               e->multi.x, e->multi.y, flags,
                                               e->timestamp, NULL);
               }
          }
     }
   return ECORE_CALLBACK_PASS_ON;
}

/**
 * @brief Ecore event handler for mouse move events.
 *
 * This function is registered as a callback for ECORE_EVENT_MOUSE_MOVE. It:
 * 1. Finds the registered Ecore_Input_Window.
 * 2. If it's a standard mouse move (not multi-touch):
 *    a. Calls `_ecore_event_evas_push_mouse_move` to update timers for fake UP events.
 *    b. Updates Evas modifier/lock states.
 *    c. If a direct input callback is set, calls it. If it handles the event, Evas feeding is skipped.
 *    d. Otherwise, calls the custom `move_mouse` callback if provided, or feeds the
 *       event to Evas using `evas_event_input_mouse_move`.
 * 3. If it's a multi-touch move:
 *    a. If a direct input callback is set, calls it. If it handles the event, Evas feeding is skipped.
 *    b. Otherwise, calls the custom `move_multi` callback if provided, or feeds the
 *       event to Evas using `evas_event_input_multi_move`.
 *
 * @param data User data (unused).
 * @param type Event type (unused).
 * @param event Pointer to the Ecore_Event_Mouse_Move structure.
 * @return ECORE_CALLBACK_PASS_ON.
 */
EAPI Eina_Bool
ecore_event_evas_mouse_move(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
   Ecore_Event_Mouse_Move *e;
   Ecore_Input_Window *lookup;

   e = event;
   lookup = _ecore_event_window_match(e->event_window);
   if (!lookup) return ECORE_CALLBACK_PASS_ON;
   if (e->multi.device == 0)
     {
        Eo *seat = e->dev ? efl_input_device_seat_get(e->dev) : NULL;
        _ecore_event_evas_push_mouse_move(e);
        ecore_event_evas_seat_modifier_lock_update(lookup->evas, e->modifiers, seat);
        if (!lookup->direct ||
            !lookup->direct(lookup->window, ECORE_EVENT_MOUSE_MOVE, e))
          {
             if (lookup->move_mouse)
               lookup->move_mouse(lookup->window, e->x, e->y, e->timestamp);
             else
                evas_event_input_mouse_move(lookup->evas, e->x, e->y, e->timestamp,
                                            NULL);
          }
     }
   else
     {
        if (!lookup->direct ||
            !lookup->direct(lookup->window, ECORE_EVENT_MOUSE_MOVE, e))
          {
             if (lookup->move_multi)
               lookup->move_multi(lookup->window, e->multi.device,
                                  e->x, e->y, e->multi.radius,
                                  e->multi.radius_x, e->multi.radius_y,
                                  e->multi.pressure, e->multi.angle,
                                  e->multi.x, e->multi.y, e->timestamp);
             else
               evas_event_input_multi_move(lookup->evas, e->multi.device,
                                           e->x, e->y, e->multi.radius,
                                           e->multi.radius_x, e->multi.radius_y,
                                           e->multi.pressure, e->multi.angle,
                                           e->multi.x, e->multi.y, e->timestamp,
                                           NULL);
          }
     }
   return ECORE_CALLBACK_PASS_ON;
}

/**
 * @brief Ecore event handler for mouse button down events.
 * Wraps _ecore_event_evas_mouse_button with ECORE_DOWN and faked=EINA_FALSE.
 * @param data User data (unused).
 * @param type Event type (unused).
 * @param event Pointer to the Ecore_Event_Mouse_Button structure.
 * @return Result of _ecore_event_evas_mouse_button.
 */
EAPI Eina_Bool
ecore_event_evas_mouse_button_down(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
   return _ecore_event_evas_mouse_button((Ecore_Event_Mouse_Button *)event, ECORE_DOWN, EINA_FALSE);
}

/**
 * @brief Ecore event handler for mouse button up events.
 * Wraps _ecore_event_evas_mouse_button with ECORE_UP and faked=EINA_FALSE.
 * @param data User data (unused).
 * @param type Event type (unused).
 * @param event Pointer to the Ecore_Event_Mouse_Button structure.
 * @return Result of _ecore_event_evas_mouse_button.
 */
EAPI Eina_Bool
ecore_event_evas_mouse_button_up(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
   return _ecore_event_evas_mouse_button((Ecore_Event_Mouse_Button *)event, ECORE_UP, EINA_FALSE);
}

/**
 * @brief Ecore event handler for mouse button cancel events.
 * Wraps _ecore_event_evas_mouse_button_cancel.
 * @param data User data (unused).
 * @param type Event type (unused).
 * @param event Pointer to the Ecore_Event_Mouse_Button structure.
 * @return Result of _ecore_event_evas_mouse_button_cancel.
 */
EAPI Eina_Bool
ecore_event_evas_mouse_button_cancel(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
   return _ecore_event_evas_mouse_button_cancel((Ecore_Event_Mouse_Button *)event);
}

/**
 * @brief Handles Ecore mouse in and mouse out events and feeds them to Evas.
 *
 * This function is called for ECORE_EVENT_MOUSE_IN and ECORE_EVENT_MOUSE_OUT. It:
 * 1. Finds the registered Ecore_Input_Window.
 * 2. Updates Evas modifier/lock states.
 * 3. If a direct input callback is set, calls it. If it handles the event, Evas feeding is skipped.
 * 4. Otherwise, feeds the mouse in/out event to Evas.
 * 5. Calls the custom `move_mouse` callback with the current coordinates, as mouse in/out
 *    events often imply a position.
 *
 * @param e The Ecore_Event_Mouse_IO event data.
 * @param io Indicates ECORE_IN or ECORE_OUT.
 * @return ECORE_CALLBACK_PASS_ON.
 */
static Eina_Bool
_ecore_event_evas_mouse_io(Ecore_Event_Mouse_IO *e, Ecore_Event_IO io)
{
   Ecore_Input_Window *lookup;
   Eo *seat;

   lookup = _ecore_event_window_match(e->event_window);
   if (!lookup) return ECORE_CALLBACK_PASS_ON;
   seat = e->dev ? efl_input_device_seat_get(e->dev) : NULL;
   ecore_event_evas_seat_modifier_lock_update(lookup->evas, e->modifiers, seat);

   switch (io)
     {
      case ECORE_IN:
        if (!lookup->direct ||
            !lookup->direct(lookup->window, ECORE_EVENT_MOUSE_IN, e))
          {
             evas_event_feed_mouse_in(lookup->evas, e->timestamp, NULL);
          }
         break;
      case ECORE_OUT:
        if (!lookup->direct ||
            !lookup->direct(lookup->window, ECORE_EVENT_MOUSE_OUT, e))
          {
             evas_event_feed_mouse_out(lookup->evas, e->timestamp, NULL);
          }
         break;
      default:
         break;
     }

   lookup->move_mouse(lookup->window, e->x, e->y, e->timestamp);
   return ECORE_CALLBACK_PASS_ON;
}

/**
 * @brief Ecore event handler for key down events.
 * Wraps _ecore_event_evas_key with ECORE_DOWN.
 * @param data User data (unused).
 * @param type Event type (unused).
 * @param event Pointer to the Ecore_Event_Key structure.
 * @return Result of _ecore_event_evas_key.
 */
EAPI Eina_Bool
ecore_event_evas_key_down(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
   return _ecore_event_evas_key((Ecore_Event_Key *)event, ECORE_DOWN);
}

/**
 * @brief Ecore event handler for key up events.
 * Wraps _ecore_event_evas_key with ECORE_UP.
 * @param data User data (unused).
 * @param type Event type (unused).
 * @param event Pointer to the Ecore_Event_Key structure.
 * @return Result of _ecore_event_evas_key.
 */
EAPI Eina_Bool
ecore_event_evas_key_up(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
   return _ecore_event_evas_key((Ecore_Event_Key *)event, ECORE_UP);
}

/**
 * @brief Ecore event handler for mouse wheel events.
 *
 * This function is registered as a callback for ECORE_EVENT_MOUSE_WHEEL. It:
 * 1. Finds the registered Ecore_Input_Window.
 * 2. Updates Evas modifier/lock states.
 * 3. If a direct input callback is set, calls it. If it handles the event, Evas feeding is skipped.
 * 4. Otherwise, feeds the mouse wheel event to Evas.
 *
 * @param data User data (unused).
 * @param type Event type (unused).
 * @param event Pointer to the Ecore_Event_Mouse_Wheel structure.
 * @return ECORE_CALLBACK_PASS_ON.
 */
EAPI Eina_Bool
ecore_event_evas_mouse_wheel(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
   Ecore_Event_Mouse_Wheel *e;
   Ecore_Input_Window *lookup;
   Eo *seat;

   e = event;
   lookup = _ecore_event_window_match(e->event_window);
   if (!lookup) return ECORE_CALLBACK_PASS_ON;
   seat = e->dev ? efl_input_device_seat_get(e->dev) : NULL;
   ecore_event_evas_seat_modifier_lock_update(lookup->evas, e->modifiers, seat);
   if (!lookup->direct ||
       !lookup->direct(lookup->window, ECORE_EVENT_MOUSE_WHEEL, e))
     {
        evas_event_feed_mouse_wheel(lookup->evas, e->direction, e->z, e->timestamp, NULL);
     }

   return ECORE_CALLBACK_PASS_ON;
}

/**
 * @brief Ecore event handler for mouse in events.
 * Wraps _ecore_event_evas_mouse_io with ECORE_IN.
 * @param data User data (unused).
 * @param type Event type (unused).
 * @param event Pointer to the Ecore_Event_Mouse_IO structure.
 * @return Result of _ecore_event_evas_mouse_io.
 */
EAPI Eina_Bool
ecore_event_evas_mouse_in(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
   return _ecore_event_evas_mouse_io((Ecore_Event_Mouse_IO *)event, ECORE_IN);
}

/**
 * @brief Ecore event handler for mouse out events.
 * Wraps _ecore_event_evas_mouse_io with ECORE_OUT.
 * @param data User data (unused).
 * @param type Event type (unused).
 * @param event Pointer to the Ecore_Event_Mouse_IO structure.
 * @return Result of _ecore_event_evas_mouse_io.
 */
EAPI Eina_Bool
ecore_event_evas_mouse_out(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
   return _ecore_event_evas_mouse_io((Ecore_Event_Mouse_IO *)event, ECORE_OUT);
}

/**
 * @brief Ecore event handler for axis update events (e.g., from a tablet stylus).
 *
 * This function is registered as a callback for ECORE_EVENT_AXIS_UPDATE. It:
 * 1. Finds the registered Ecore_Input_Window.
 * 2. If a direct input callback is set, calls it. If it handles the event, Evas feeding is skipped.
 * 3. Otherwise, feeds the axis update event to Evas.
 *    The `e->axis` field is an array of Ecore_Axis structures.
 *    Example `e->axis` structure:
 *    `Ecore_Axis axis_data[] = { { label = "X Tilt", value = 10.5 }, { label = "Y Tilt", value = -5.2 } };`
 *    `e->naxis` would be 2 in this case.
 *
 * @param data User data (unused).
 * @param type Event type (unused).
 * @param event Pointer to the Ecore_Event_Axis_Update structure.
 * @return ECORE_CALLBACK_PASS_ON.
 */
EAPI Eina_Bool
ecore_event_evas_axis_update(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
   Ecore_Event_Axis_Update *e;
   Ecore_Input_Window *lookup;

   e = event;
   lookup = _ecore_event_window_match(e->event_window);
   if (!lookup) return ECORE_CALLBACK_PASS_ON;
   if (!lookup->direct ||
       !lookup->direct(lookup->window, ECORE_EVENT_AXIS_UPDATE, e))
     {
        evas_event_feed_axis_update(lookup->evas, e->timestamp, e->device,
                                    e->toolid, e->naxis,
                                    (Evas_Axis *)e->axis, NULL);
     }

   return ECORE_CALLBACK_PASS_ON;
}

/**
 * @brief Initializes the Ecore Evas event handling module.
 *
 * This function sets up the necessary Ecore event handlers to intercept
 * input events (key, mouse, axis) and feed them to registered Evas canvases.
 * It also initializes a log domain for this module and the `_window_hash`
 * for mapping Ecore_Window IDs to Evas instances.
 * It reads environment variables `ECORE_INPUT_FIX` and `ECORE_INPUT_TIMEOUT_FIX`
 * to enable and configure a workaround for missing mouse UP events.
 *
 * This function uses a reference counter (`_ecore_event_evas_init_count`).
 * Ecore and Ecore_Event are initialized only on the first call.
 *
 * @return The current initialization count. Returns 0 or a negative value on failure.
 */
EAPI int
ecore_event_evas_init(void)
{
   if (++_ecore_event_evas_init_count !=  1)
     return _ecore_event_evas_init_count;

   _ecore_input_evas_log_dom = eina_log_domain_register
     ("ecore_input_evas",  ECORE_INPUT_EVAS_DEFAULT_LOG_COLOR);
   if (_ecore_input_evas_log_dom < 0)
     {
        EINA_LOG_ERR("Impossible to create a log domain for the ecore input evas_module.");
        return --_ecore_event_evas_init_count;
     }

   if (!ecore_init())
     {
        return --_ecore_event_evas_init_count;
     }

   if (!ecore_event_init())
     {
        goto shutdown_ecore;
     }

   ecore_event_evas_handlers[0] = ecore_event_handler_add(ECORE_EVENT_KEY_DOWN,
                                                          ecore_event_evas_key_down,
                                                          NULL);
   ecore_event_evas_handlers[1] = ecore_event_handler_add(ECORE_EVENT_KEY_UP,
                                                          ecore_event_evas_key_up,
                                                          NULL);
   ecore_event_evas_handlers[2] = ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_DOWN,
                                                          ecore_event_evas_mouse_button_down,
                                                          NULL);
   ecore_event_evas_handlers[3] = ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_UP,
                                                          ecore_event_evas_mouse_button_up,
                                                          NULL);
   ecore_event_evas_handlers[4] = ecore_event_handler_add(ECORE_EVENT_MOUSE_MOVE,
                                                          ecore_event_evas_mouse_move,
                                                          NULL);
   ecore_event_evas_handlers[5] = ecore_event_handler_add(ECORE_EVENT_MOUSE_WHEEL,
                                                          ecore_event_evas_mouse_wheel,
                                                          NULL);
   ecore_event_evas_handlers[6] = ecore_event_handler_add(ECORE_EVENT_MOUSE_IN,
                                                          ecore_event_evas_mouse_in,
                                                          NULL);
   ecore_event_evas_handlers[7] = ecore_event_handler_add(ECORE_EVENT_MOUSE_OUT,
                                                          ecore_event_evas_mouse_out,
                                                          NULL);
   ecore_event_evas_handlers[8] = ecore_event_handler_add(ECORE_EVENT_AXIS_UPDATE,
                                                          ecore_event_evas_axis_update,
                                                          NULL);
   ecore_event_evas_handlers[9] = ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_CANCEL,
                                                          ecore_event_evas_mouse_button_cancel,
                                                          NULL);

   _window_hash = eina_hash_pointer_new(free);

   if (getenv("ECORE_INPUT_FIX"))
     {
        const char *tmp;

        _last_events_enable = EINA_TRUE;

        tmp = getenv("ECORE_INPUT_TIMEOUT_FIX");
        if (tmp)
          _last_events_timeout = ((double) atoi(tmp)) / 60;
     }

   return _ecore_event_evas_init_count;

   shutdown_ecore:
   ecore_shutdown();

   return --_ecore_event_evas_init_count;
}

/**
 * @brief Shuts down the Ecore Evas event handling module.
 *
 * This function cleans up resources allocated by `ecore_event_evas_init`.
 * It removes the Ecore event handlers, frees the `_window_hash`, clears
 * the list of last events, and unregisters the log domain.
 * Ecore_Event and Ecore are shut down only when the reference counter
 * (`_ecore_event_evas_init_count`) reaches zero.
 *
 * @return The current initialization count (0 if fully shut down).
 */
EAPI int
ecore_event_evas_shutdown(void)
{
   size_t i;
   Ecore_Event_Last *eel;

   if (--_ecore_event_evas_init_count != 0)
     return _ecore_event_evas_init_count;

   EINA_LIST_FREE(_last_events, eel)
      free(eel);

   eina_hash_free(_window_hash);
   _window_hash = NULL;
   for (i = 0; i < sizeof(ecore_event_evas_handlers) / sizeof(Ecore_Event_Handler *); i++)
     {
        ecore_event_handler_del(ecore_event_evas_handlers[i]);
        ecore_event_evas_handlers[i] = NULL;
     }

   ecore_event_shutdown();
   ecore_shutdown();

   eina_log_domain_unregister(_ecore_input_evas_log_dom);
   _ecore_input_evas_log_dom = -1;

   return _ecore_event_evas_init_count;
}
