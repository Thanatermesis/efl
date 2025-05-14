#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#include <Efl_Ui.h>
#include <Elementary.h>

#define EFL_INTERNAL_UNSTABLE
#include "Evas_Internal.h"

#define DEFAULT_TEXT "Click the white rectangle to get started"

/**
 * @brief Holds all the data needed for the event test.
 */
typedef struct {
   int down; /**< Flag to indicate if the pointer is down. */
   Eo *evdown, *evup, *evmove, *evkeydown, *evkeyup; /**< Cloned event objects for mouse and key events */
   Eo *win, *button, *text; /**< UI widgets used in the test */
   int id; /**< A counter for button clicks. */
   Eina_Future *f; /**< A future for timed operations. */
} testdata;

/**
 * @brief Callback for pointer down events on the white rectangle.
 *
 * This function is called when the pointer (mouse) button is pressed down
 * over the rectangle. It records that the pointer is down and duplicates
 * the event information for later use in synthesizing fake events.
 *
 * @param data The user data, a pointer to a @ref testdata struct.
 * @param ev The event information.
 */
static void
_pointer_down(void *data, const Efl_Event *ev)
{
   testdata *td = data;
   td->down = 1;
   efl_unref(td->evdown);
   td->evdown = efl_duplicate(ev->info);
}

/**
 * @brief Callback for pointer move events on the white rectangle.
 *
 * This function is called when the pointer (mouse) is moved over the
 * rectangle. It duplicates the event information for later use.
 *
 * @param data The user data, a pointer to a @ref testdata struct.
 * @param ev The event information.
 */
static void
_pointer_move(void *data, const Efl_Event *ev)
{
   testdata *td = data;
   efl_unref(td->evmove);
   td->evmove = efl_duplicate(ev->info);
}

/**
 * @brief Callback for pointer up events on the white rectangle.
 *
 * This function is called when the pointer (mouse) button is released
 * over the rectangle. It records that the pointer is up and duplicates
 * the event information for later use.
 *
 * @param data The user data, a pointer to a @ref testdata struct.
 * @param ev The event information.
 */
static void
_pointer_up(void *data, const Efl_Event *ev)
{
   testdata *td = data;
   td->down = 0;
   efl_unref(td->evup);
   td->evup = efl_duplicate(ev->info);
}

/**
 * @brief Callback for key down events on the window.
 *
 * Displays information about the pressed key in the text label.
 * It also duplicates the event information if it's not a fake event.
 *
 * @param data The user data, a pointer to a @ref testdata struct.
 * @param ev The event information, contains details about the key press.
 */
static void
_key_down(void *data, const Efl_Event *ev)
{
   testdata *td = data;
   char str[1024];

   // FIXME: By default the elm_win object is the focused object
   // this means that evas callbacks will transfer the KEY_UP/KEY_DOWN events
   // to the elm_win. So, we get two key_down & two key_up events:
   // 1. ecore_evas -> evas -> elm_win forward -> here
   // 2. ecore_evas -> evas -> focused obj (elm_win) -> here

   sprintf(str, "key=%s keyname=%s string=%s compose=%s",
           efl_input_key_sym_get(ev->info),
           efl_input_key_name_get(ev->info),
           efl_input_key_string_get(ev->info),
           efl_input_key_compose_string_get(ev->info));
   elm_object_text_set(td->text, str);

   if (!efl_input_fake_get(ev->info))
     {
        efl_unref(td->evkeydown);
        td->evkeydown = efl_duplicate(ev->info);
     }
}

/**
 * @brief Timeout callback to reset the text label.
 *
 * After a key event is displayed, this function is called after a delay
 * to reset the label to its default text.
 *
 * @param data The user data, a pointer to a @ref testdata struct.
 * @param t The value from the future.
 * @param dead The future that triggered this callback.
 * @return Returns the value @p t.
 */
static Eina_Value
_ecore_timeout_cb(void *data,
                  const Eina_Value t,
                  const Eina_Future  *dead EINA_UNUSED)
{
   testdata *td = data;

   elm_object_text_set(td->text, DEFAULT_TEXT);
   td->f = NULL;

   return t;
}

/**
 * @brief Callback for key up events on the window.
 *
 * Duplicates the key up event info if it is not a fake event. It then
 * starts a short timer to clear the key information display.
 *
 * @param data The user data, a pointer to a @ref testdata struct.
 * @param ev The event information.
 */
static void
_key_up(void *data, const Efl_Event *ev)
{
   testdata *td = data;

   if (!efl_input_fake_get(ev->info))
     {
        efl_unref(td->evkeyup);
        td->evkeyup = efl_duplicate(ev->info);
     }

   if (td->f) eina_future_cancel(td->f);
   td->f = efl_loop_timeout(efl_provider_find(ev->object, EFL_LOOP_CLASS), 0.5);
   eina_future_then(td->f, _ecore_timeout_cb, td, NULL);
}

/**
 * @brief Callback for the 'Click me!' button.
 *
 * This function is called when the first button is clicked. It increments
 * a counter and displays the click count.
 *
 * @param data The user data, a pointer to a @ref testdata struct.
 * @param ev The event information (unused).
 */
static void
_clicked_button1(void *data, const Efl_Event *ev EINA_UNUSED)
{
   testdata *td = data;
   Eo *txt = td->text;
   char buf[256];

   // Note: can't do efl_input_fake_get(ev->info) because this is a click evt

   td->id++;
   sprintf(buf, "Button was clicked %d time%s", td->id, td->id > 1 ? "s" : "");
   elm_object_text_set(txt, buf);
}

/**
 * @brief Callback for the 'Send fake event' button.
 *
 * This function synthesizes and sends fake input events. If a key event
 * has been captured, it replays the key down/up events. Otherwise, it
 * generates fake pointer move, down, and up events targeted at the center
 * of the 'Click me!' button.
 *
 * @param data The user data, a pointer to a @ref testdata struct.
 * @param ev The event information (unused).
 */
static void
_clicked_button2(void *data, const Efl_Event *ev EINA_UNUSED)
{
   testdata *td = data;
   Eo *bt = td->button;
   Eina_Rect r;

   if (!td->evkeyup)
     {
        r = efl_gfx_entity_geometry_get(bt);

        r.x = r.x + r.w / 2;
        r.y = r.y + r.h / 2;
        efl_input_pointer_position_set(td->evmove, r.pos);
        efl_input_pointer_position_set(td->evdown, r.pos);
        efl_input_pointer_position_set(td->evup, r.pos);

        efl_event_callback_call(td->win, EFL_EVENT_POINTER_MOVE, td->evmove);
        efl_event_callback_call(td->win, EFL_EVENT_POINTER_DOWN, td->evdown);
        efl_event_callback_call(td->win, EFL_EVENT_POINTER_UP, td->evup);
     }
   else
     {
        efl_event_callback_call(td->win, EFL_EVENT_KEY_DOWN, td->evkeydown);
        efl_event_callback_call(td->win, EFL_EVENT_KEY_UP, td->evkeyup);
        efl_unref(td->evkeydown);
        efl_unref(td->evkeyup);
        td->evkeydown = NULL;
        td->evkeyup = NULL;
     }
}

/**
 * @brief Callback for window deletion.
 *
 * Cleans up resources when the window is closed. This includes unreferencing
 * any stored event objects and freeing the test data structure.
 *
 * @param data The user data, a pointer to a @ref testdata struct.
 * @param ev The event information (unused).
 */
static void
_win_del(void *data, const Efl_Event *ev EINA_UNUSED)
{
   testdata *td = data;
   efl_unref(td->evdown);
   efl_unref(td->evup);
   efl_unref(td->evmove);
   efl_unref(td->evkeydown);
   efl_unref(td->evkeyup);
   free(td);
}

/**
 * @brief Raw pointer down event callback for the 'Click me!' button.
 *
 * This is used to demonstrate listening to raw pointer events on a widget.
 * It prints whether the event was fake. The user data is used as a magic
 * number to ensure the callback is called with the correct context.
 *
 * @param data User data, expected to be `(void*)(intptr_t)0x1`.
 * @param ev The event information.
 */
static void
_button_pointer_down(void *data, const Efl_Event *ev)
{
   if (((intptr_t) data) != 0x1) abort();
   printf("Button raw event: DOWN. Fake = %d\n", efl_input_fake_get(ev->info));
   fflush(stdout);
}

/**
 * @brief Raw pointer up event callback for the 'Click me!' button.
 *
 * This is used to demonstrate listening to raw pointer events on a widget.
 * It prints whether the event was fake. The user data is used as a magic
 * number to ensure the callback is called with the correct context.
 *
 * @param data User data, expected to be `(void*)(intptr_t)0x1`.
 * @param ev The event information.
 */
static void
_button_pointer_up(void *data, const Efl_Event *ev)
{
   if (((intptr_t) data) != 0x1) abort();
   printf("Button raw event: UP.   Fake = %d\n", efl_input_fake_get(ev->info));
   fflush(stdout);
}

/**
 * @brief Array of callbacks for pointer events on the rectangle.
 *
 * This defines a mapping between pointer events and their handler functions.
 * The array structure is:
 * @code
 * Efl_Callback_Array_Item[] = {
 *   { &EFL_EVENT_POINTER_DOWN, _pointer_down },
 *   { &EFL_EVENT_POINTER_MOVE, _pointer_move },
 *   { &EFL_EVENT_POINTER_UP,   _pointer_up },
 *   { NULL, NULL }
 * };
 * @endcode
 */
EFL_CALLBACKS_ARRAY_DEFINE(rect_pointer_callbacks,
{ EFL_EVENT_POINTER_DOWN, _pointer_down },
{ EFL_EVENT_POINTER_MOVE, _pointer_move },
{ EFL_EVENT_POINTER_UP, _pointer_up })

/**
 * @brief Array of callbacks for key events on the window.
 *
 * This defines a mapping between key events and their handler functions.
 * The array structure is:
 * @code
 * Efl_Callback_Array_Item[] = {
 *   { &EFL_EVENT_KEY_DOWN, _key_down },
 *   { &EFL_EVENT_KEY_UP,   _key_up },
 *   { NULL, NULL }
 * };
 * @endcode
 */
EFL_CALLBACKS_ARRAY_DEFINE(win_key_callbacks,
{ EFL_EVENT_KEY_DOWN, _key_down },
{ EFL_EVENT_KEY_UP, _key_up })

/**
 * @brief Array of callbacks for raw pointer events on the button.
 *
 * This defines a mapping between raw pointer events and their handler functions.
 * The array structure is:
 * @code
 * Efl_Callback_Array_Item[] = {
 *   { &EFL_EVENT_POINTER_DOWN, _button_pointer_down },
 *   { &EFL_EVENT_POINTER_UP,   _button_pointer_up },
 *   { NULL, NULL }
 * };
 * @endcode
 */
EFL_CALLBACKS_ARRAY_DEFINE(button_pointer_callbacks,
{ EFL_EVENT_POINTER_DOWN, _button_pointer_down },
{ EFL_EVENT_POINTER_UP, _button_pointer_up })

/**
 * @brief Main function for the events test.
 *
 * This function sets up the UI for testing events. It creates a window
 * with two buttons, a text label, and a rectangle. It then connects
 * various event callbacks to these widgets to demonstrate event handling,
 * including capturing real events and synthesizing fake ones.
 *
 * @param data Not used.
 * @param obj Not used.
 * @param event_info Not used.
 */
void
test_events(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   /* test fake POINTER and KEY events */

   Evas_Object *bx, *bt, *txt, *o, *win;
   testdata *td = calloc(1, sizeof(*td));

   win = elm_win_util_standard_add("buttons", "Buttons");
   elm_win_autodel_set(win, EINA_TRUE);

   bx = efl_add(EFL_UI_BOX_CLASS, win,
                efl_ui_layout_orientation_set(efl_added, EFL_UI_LAYOUT_ORIENTATION_VERTICAL));
   efl_content_set(win, bx);
   td->win = win;

   txt = elm_label_add(win);
   evas_object_size_hint_weight_set(txt, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_align_set(txt, -1, -1);
   efl_pack(bx, txt);
   elm_object_text_set(txt, DEFAULT_TEXT);
   evas_object_show(txt);
   td->text = txt;

   bt = elm_button_add(win);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_align_set(bt, -1, -1);
   elm_object_text_set(bt, "Click me!");
   efl_pack(bx, bt);
   evas_object_show(bt);
   td->button = bt;

   bt = elm_button_add(win);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_align_set(bt, -1, -1);
   elm_object_text_set(bt, "Send fake event");
   efl_pack(bx, bt);
   evas_object_show(bt);

   o = efl_add(EFL_CANVAS_RECTANGLE_CLASS, win);
   efl_pack(bx, o);

   efl_event_callback_add(td->button, EFL_INPUT_EVENT_CLICKED, _clicked_button1, td);
   efl_event_callback_array_add(td->button, button_pointer_callbacks(), (void*)(intptr_t)0x1);
   efl_event_callback_add(bt, EFL_INPUT_EVENT_CLICKED, _clicked_button2, td);
   efl_event_callback_add(win, EFL_EVENT_DEL, _win_del, td);
   efl_event_callback_array_add(o, rect_pointer_callbacks(), td);
   efl_event_callback_array_add(win, win_key_callbacks(), td);

   evas_object_resize(td->win, 200, 100);
   evas_object_show(td->win);
}
