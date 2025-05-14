#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#include <Efl_Ui.h>
#include <Elementary.h>

/**
 * @brief Callback function for the 'clicked' event on a button.
 *
 * This function is called when a button is clicked. It prints information
 * about the click event, such as whether it was a repeated click and which
 * mouse button was used.
 *
 * @param data User data pointer (unused).
 * @param ev The event information structure.
 */
static void
_clicked(void *data EINA_UNUSED, const Efl_Event *ev)
{
   Efl_Input_Clickable_Clicked *clicked = ev->info;
   printf("Button is clicked!!! repeated(%d) button(%d)\n", clicked->repeated, clicked->button);
}

/**
 * @brief Callback function for the 'pressed' event on a button.
 *
 * This function is invoked when a button is pressed down.
 *
 * @param data User data pointer (unused).
 * @param ev The event information structure (unused).
 */
static void
_pressed(void *data EINA_UNUSED, const Efl_Event *ev EINA_UNUSED)
{
   printf("Button is pressed!!!\n");
}

/**
 * @brief Callback function for the 'unpressed' event on a button.
 *
 * This function is called when a button is released.
 *
 * @param data User data pointer (unused).
 * @param ev The event information structure (unused).
 */
static void
_unpressed(void *data EINA_UNUSED, const Efl_Event *ev EINA_UNUSED)
{
   printf("Button is unpressed!!!\n");
}

/**
 * @brief Test function for Efl.Ui.Button.
 *
 * This function creates a window and demonstrates different types of buttons:
 * - A button with only text.
 * - A button with only an icon.
 * - A button with both text and an icon.
 *
 * It attaches event listeners for clicked, pressed, and unpressed events to
 * each button.
 *
 * @param data User data pointer (unused).
 * @param obj The Evas_Object parent (unused).
 * @param event_info The event information (unused).
 */
void
test_ui_button(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Eo *win, *bx, *btn;
   char buf[PATH_MAX];

   win = efl_add(EFL_UI_WIN_CLASS, efl_main_loop_get(),
                                  efl_text_set(efl_added, "Efl.Ui.Button"),
                 efl_ui_win_autodel_set(efl_added, EINA_TRUE)
                );

   bx = efl_add(EFL_UI_BOX_CLASS, win,
                efl_content_set(win, efl_added),
                efl_ui_layout_orientation_set(efl_added, EFL_UI_LAYOUT_ORIENTATION_VERTICAL));

   // Text Only Button
   efl_add(EFL_UI_BUTTON_CLASS, bx,
           efl_text_set(efl_added, "Text"),
           efl_pack(bx, efl_added),
           efl_event_callback_add(efl_added, EFL_INPUT_EVENT_CLICKED, _clicked, NULL),
           efl_event_callback_add(efl_added, EFL_INPUT_EVENT_PRESSED, _pressed, NULL),
           efl_event_callback_add(efl_added, EFL_INPUT_EVENT_UNPRESSED, _unpressed, NULL)
          );

   // Icon Only Button
   snprintf(buf, sizeof(buf), "%s/images/logo_small.png", elm_app_data_dir_get());
   btn = efl_add(EFL_UI_BUTTON_CLASS, bx,
                 efl_pack(bx, efl_added),
                 efl_event_callback_add(efl_added, EFL_INPUT_EVENT_CLICKED, _clicked, NULL),
                 efl_event_callback_add(efl_added, EFL_INPUT_EVENT_PRESSED, _pressed, NULL),
                 efl_event_callback_add(efl_added, EFL_INPUT_EVENT_UNPRESSED, _unpressed, NULL)
                );
   efl_add(EFL_UI_IMAGE_CLASS, btn,
           efl_file_set(efl_added, buf),
           efl_content_set(btn, efl_added)
          );

   // Text + Icon Button
   snprintf(buf, sizeof(buf), "%s/images/logo_small.png", elm_app_data_dir_get());
   btn = efl_add(EFL_UI_BUTTON_CLASS, bx,
                 efl_text_set(efl_added, "Text + Icon"),
                 efl_pack(bx, efl_added),
                 efl_event_callback_add(efl_added, EFL_INPUT_EVENT_CLICKED, _clicked, NULL),
                 efl_event_callback_add(efl_added, EFL_INPUT_EVENT_PRESSED, _pressed, NULL),
                 efl_event_callback_add(efl_added, EFL_INPUT_EVENT_UNPRESSED, _unpressed, NULL)
                );
   efl_add(EFL_UI_IMAGE_CLASS, btn,
           efl_file_set(efl_added, buf),
           efl_content_set(btn, efl_added)
          );

   efl_gfx_entity_size_set(win, EINA_SIZE2D(320,  400));
}

