#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include <Efl_Ui.h>
#include <Elementary.h>

static void _third_layout_push(void *data, const Efl_Event *ev EINA_UNUSED);

/**
 * @brief Callback to remove the top layout from the stack.
 *
 * This function retrieves the top-most layout from the stack container and
 * deletes it. This demonstrates removing an element from a pack container,
 * which is the underlying structure of the stack in this example.
 *
 * @param data The stack object (Efl_Ui_Spotlight_Container).
 * @param ev The event information (unused).
 */
static void
_stack_remove(void *data, const Efl_Event *ev EINA_UNUSED)
{
   Eo *stack = data;
   Eo *top_layout = efl_pack_content_get(stack, 0);
   efl_del(top_layout);
}

/**
 * @brief Callback to pop the top view from the spotlight stack.
 *
 * This function invokes the pop operation on the spotlight container, which
 * removes the currently visible view and reveals the one beneath it.
 * This is the standard way to navigate back in a spotlight/stack interface.
 *
 * @param data The stack object (Efl_Ui_Spotlight_Container).
 * @param ev The event information (unused).
 */
static void
_stack_pop(void *data, const Efl_Event *ev EINA_UNUSED)
{
   Eo *stack = data;
   efl_ui_spotlight_pop(stack, EINA_TRUE);
}

/**
 * @brief Callback to push a new layout onto the stack.
 *
 * This function demonstrates pushing another view onto the stack from an
 * existing view. It calls @_third_layout_push to create and push the 3rd layout.
 * The name "double_push" implies it's an action that triggers another push.
 *
 * @param data The stack object (Efl_Ui_Spotlight_Container).
 * @param ev The event information (unused).
 */
static void
_stack_double_push(void *data, const Efl_Event *ev EINA_UNUSED)
{
   Eo *stack = data;
   _third_layout_push(stack, NULL);
}

/**
 * @brief Callback to delete the top layout from the stack.
 *
 * Note: This function's effect is identical to @_stack_remove. It deletes
 * the top-most widget in the stack, but not the stack container itself.
 * The name might be misleading.
 *
 * @param data The stack object (Efl_Ui_Spotlight_Container).
 * @param ev The event information (unused).
 */
static void
_stack_del(void *data, const Efl_Event *ev EINA_UNUSED)
{
   Eo *stack = data;
   Eo *top_layout = efl_pack_content_get(stack, 0);
   efl_del(top_layout);
}

/**
 * @brief Callback to delete the main window.
 *
 * This function deletes the window object, which in turn will cause the
 * application to exit due to the 'autodel' property being set.
 *
 * @param data The window object to be deleted.
 * @param ev The event information (unused).
 */
static void
_win_del(void *data, const Efl_Event *ev EINA_UNUSED)
{
   Eo *win = data;
   efl_del(win);
}

/**
 * @brief Creates a standardized navigation layout.
 *
 * This helper function constructs a navigation layout which includes a
 * navigation bar with a given title. The content of the layout is set to the
 * provided `content` object.
 *
 * @param stack The parent container for the new layout.
 * @param text The title text to be displayed in the navigation bar.
 * @param content The main content object for the layout.
 * @return A new Efl_Ui_Navigation_Layout object.
 */
static Eo *
_navigation_layout_create(Eo *stack, const char *text, Eo *content)
{
   Eo *nl = efl_add(EFL_UI_NAVIGATION_LAYOUT_CLASS, stack);

   Eo *bn = efl_add(EFL_UI_NAVIGATION_BAR_CLASS, nl);
   efl_text_set(bn, text);
   efl_gfx_entity_visible_set(efl_part(bn, "back_button"), EINA_TRUE);
   efl_ui_navigation_layout_bar_set(nl, bn);

   efl_content_set(nl, content);

   printf("Create content(%p).\n\n", nl);

   return nl;
}

/**
 * @brief Sets a custom button on the left side of a navigation bar.
 *
 * This function adds a "Prev" button to the "left_content" part of the
 * navigation bar. It also hides the default "back_button" as they would
 * otherwise overlap.
 *
 * @param navigation_layout The layout whose bar will be modified.
 * @param clicked_cb The callback function to be invoked on button click.
 * @param data Custom data to be passed to the callback.
 */
static void
_bar_left_btn_set(Eo *navigation_layout, Efl_Event_Cb clicked_cb, void *data)
{
   Eo *bn = efl_ui_navigation_layout_bar_get(navigation_layout);

   Eo *left_btn = efl_add(EFL_UI_BUTTON_CLASS, bn);
   efl_text_set(left_btn, "Prev");
   efl_content_set(efl_part(bn, "left_content"), left_btn);

   efl_event_callback_add(left_btn, EFL_INPUT_EVENT_CLICKED, clicked_cb, data);

   //Positions of "left_content" and "back_button" are the same.
   efl_gfx_entity_visible_set(efl_part(bn, "back_button"), EINA_FALSE);
}

/**
 * @brief Sets a custom button on the right side of a navigation bar.
 *
 * This function adds a "Next" button to the "right_content" part of the
 * navigation bar.
 *
 * @param navigation_layout The layout whose bar will be modified.
 * @param clicked_cb The callback function to be invoked on button click.
 * @param data Custom data to be passed to the callback.
 */
static void
_bar_right_btn_set(Eo *navigation_layout, Efl_Event_Cb clicked_cb, void *data)
{
   Eo *bn = efl_ui_navigation_layout_bar_get(navigation_layout);

   Eo *right_btn = efl_add(EFL_UI_BUTTON_CLASS, bn);
   efl_text_set(right_btn, "Next");
   efl_content_set(efl_part(bn, "right_content"), right_btn);

   efl_event_callback_add(right_btn, EFL_INPUT_EVENT_CLICKED, clicked_cb, data);
}

/**
 * @brief Creates and pushes the fifth layout onto the stack.
 *
 * This function is a callback that constructs the "5th layout". This layout
 * contains a button that, when clicked, removes the top-most layout from
 * the stack using @_stack_remove.
 * The function name uses "insert", but it performs a push operation.
 *
 * @param data The stack object (Efl_Ui_Spotlight_Container).
 * @param ev The event information (unused).
 */
static void
_fifth_layout_insert(void *data, const Efl_Event *ev EINA_UNUSED)
{
   Eo *stack = data;

   Eo *btn = efl_add(EFL_UI_BUTTON_CLASS, stack);
   efl_text_set(btn, "Press to remove top layout");
   efl_event_callback_add(btn, EFL_INPUT_EVENT_CLICKED, _stack_remove, stack);

   Eo *nl = _navigation_layout_create(stack, "5th layout", btn);

   efl_ui_spotlight_push(stack, nl);
}

/**
 * @brief Creates and pushes the third layout onto the stack.
 *
 * This callback function constructs the "3rd layout". This layout contains a
 * button to pop the current view, and a "Next" button to push the
 * 5th layout.
 *
 * @param data The stack object (Efl_Ui_Spotlight_Container).
 * @param ev The event information (unused).
 */
static void
_third_layout_push(void *data, const Efl_Event *ev EINA_UNUSED)
{
   Eo *stack = data;

   Eo *btn = efl_add(EFL_UI_BUTTON_CLASS, stack);
   efl_text_set(btn, "Press to pop");
   efl_event_callback_add(btn, EFL_INPUT_EVENT_CLICKED, _stack_pop, stack);

   Eo *nl = _navigation_layout_create(stack, "3rd layout", btn);

   _bar_right_btn_set(nl, _fifth_layout_insert, stack);

   efl_ui_spotlight_push(stack, nl);
}

/**
 * @brief Creates and pushes the second layout onto the stack.
 *
 * This callback constructs the "2nd layout". It features a button to "double push"
 * (which pushes the 3rd layout immediately) and a "Next" button that also
 * navigates to the 3rd layout.
 *
 * @param data The stack object (Efl_Ui_Spotlight_Container).
 * @param ev The event information (unused).
 */
static void
_second_layout_push(void *data, const Efl_Event *ev EINA_UNUSED)
{
   Eo *stack = data;

   Eo *btn = efl_add(EFL_UI_BUTTON_CLASS, stack);
   efl_text_set(btn, "Press to double push");
   efl_event_callback_add(btn, EFL_INPUT_EVENT_CLICKED, _stack_double_push, stack);

   Eo *nl = _navigation_layout_create(stack, "2nd layout", btn);

   _bar_right_btn_set(nl, _third_layout_push, stack);

   efl_ui_spotlight_push(stack, nl);
}

/**
 * @brief Creates and pushes the initial layout onto the stack.
 *
 * This function sets up the first view of the application. This "1st layout"
 * includes:
 * - A "Prev" button that closes the window.
 * - A "Next" button that pushes the "2nd layout".
 * - A main button that demonstrates deleting the top-most layout from the stack.
 *
 * @param win The main application window.
 * @param stack The stack container.
 */
static void
_first_layout_push(Eo *win, Eo *stack)
{
   Eo *btn = efl_add(EFL_UI_BUTTON_CLASS, stack);
   efl_text_set(btn, "Press to delete stack");
   efl_event_callback_add(btn, EFL_INPUT_EVENT_CLICKED, _stack_del, stack);

   Eo *nl = _navigation_layout_create(stack, "1st layout", btn);

   _bar_left_btn_set(nl, _win_del, win);
   _bar_right_btn_set(nl, _second_layout_push, stack);

   efl_ui_spotlight_push(stack, nl);
}

/**
 * @brief The main test function for Efl.Ui.Stack.
 *
 * This function sets up a window and a spotlight stack. It then populates
 * the stack with an initial layout, starting a sequence of views that
 * demonstrate various stack operations like push, pop, and element removal.
 */
void
test_ui_stack(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Eo *win = efl_add(EFL_UI_WIN_CLASS, efl_main_loop_get(),
                     efl_text_set(efl_added, "Efl.Ui.Stack"),
                     efl_ui_win_autodel_set(efl_added, EINA_TRUE));

   efl_gfx_entity_size_set(win, EINA_SIZE2D(500, 500));

   Eo *stack = efl_ui_spotlight_util_stack_gen(win);

   efl_content_set(win, stack);

   _first_layout_push(win, stack);
}
