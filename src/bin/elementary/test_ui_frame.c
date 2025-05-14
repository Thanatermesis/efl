/**
 * @file
 * @brief Test suite for Efl.Ui.Frame.
 *
 * This test creates a window with several Efl.Ui.Frame widgets
 * to demonstrate and test their collapse and autocollapse features.
 */

#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#include <Efl_Ui.h>
#include <Elementary.h>

/**
 * @brief Callback function for the 'clicked' event of the button.
 *
 * This function collapses the frame passed in the @p data parameter.
 *
 * @param data The Efl.Ui.Frame object to be collapsed.
 * @param ev The Efl_Event data (unused).
 */
static void
_inc_clicked(void *data, const Efl_Event *ev EINA_UNUSED)
{
   efl_ui_frame_collapse_go(data, EINA_TRUE);
}

/**
 * @brief Test function for Efl.Ui.Frame.
 *
 * This function sets up a window with multiple Efl.Ui.Frame instances
 * to test different configurations:
 * - A non-collapsible frame.
 * - An auto-collapsible frame.
 * - A frame that can be collapsed externally via a button.
 *
 * @param data Not used.
 * @param obj Not used.
 * @param event_info Not used.
 */
void
test_ui_frame(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Eo *win, *bx, *f, *txt;

   win = efl_add(EFL_UI_WIN_CLASS, efl_main_loop_get(),
                                  efl_text_set(efl_added, "Efl.Ui.Frame"),
                 efl_ui_win_autodel_set(efl_added, EINA_TRUE));

   bx = efl_add(EFL_UI_BOX_CLASS, win,
                efl_content_set(win, efl_added),
                efl_ui_layout_orientation_set(efl_added, EFL_UI_LAYOUT_ORIENTATION_VERTICAL));

   f = efl_add(EFL_UI_FRAME_CLASS, win,
               efl_pack_end(bx, efl_added),
               efl_ui_frame_autocollapse_set(efl_added, EINA_FALSE));
   efl_text_set(f, "Test 1");

   txt = efl_add(EFL_UI_TEXTBOX_CLASS, f);
   efl_text_set(txt, "Not collapseable");
   efl_content_set(f, txt);

   f = efl_add(EFL_UI_FRAME_CLASS, win,
               efl_pack_end(bx, efl_added),
               efl_ui_frame_autocollapse_set(efl_added, EINA_TRUE));
   efl_text_set(f, "Test2");

   txt = efl_add(EFL_UI_TEXTBOX_CLASS, f);
   efl_text_set(txt, "Collapseable");
   efl_content_set(f, txt);

   f = efl_add(EFL_UI_FRAME_CLASS, win,
               efl_pack_end(bx, efl_added));
   efl_text_set(f, "Test 3");

   efl_add(EFL_UI_BUTTON_CLASS, bx,
           efl_text_set(efl_added, "frame collapse externally"),
           efl_event_callback_add(efl_added, EFL_INPUT_EVENT_CLICKED, _inc_clicked, f),
           efl_content_set(f, efl_added));

   efl_gfx_entity_size_set(win, EINA_SIZE2D(100, 120));
}
