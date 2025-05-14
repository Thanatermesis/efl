#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#include <Efl_Ui.h>
#include <Elementary.h>

/**
 * @brief Test case for Efl.Ui.Panes minimum size handling.
 *
 * This test creates a window with a nested panes layout to verify
 * that minimum size hints are correctly handled.
 *
 * The layout consists of a main vertical panes widget. The first part
 * contains a button with a user-defined minimum size. The second part
 * contains a horizontal panes widget.
 *
 * The nested horizontal panes widget also has two parts, each with a
 * button and its own minimum size hints. One of these parts has
 * the `hint_min_allow` property enabled, which is crucial for
 * testing if the minimum size hints are respected by the panes logic.
 *
 * @param data Unused.
 * @param obj Unused.
 * @param event_info Unused.
 */
void
test_panes_minsize(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Eo *win, *panes, *panes_h;

   win = efl_add(EFL_UI_WIN_CLASS, efl_main_loop_get(),
                                  efl_text_set(efl_added, "Efl.Ui.Panes"),
                 efl_ui_win_autodel_set(efl_added, EINA_TRUE)
                );

   panes = efl_add(EFL_UI_PANES_CLASS, win,
                   efl_content_set(win, efl_added),
                   efl_ui_panes_split_ratio_set(efl_added, 0.7)
                  );

   efl_add(EFL_UI_BUTTON_CLASS, win,
           efl_text_set(efl_added, "Left - user set min size(110,110)"),
           efl_gfx_hint_size_min_set(efl_added, EINA_SIZE2D(110, 110)),
           efl_content_set(efl_part(panes, "first"), efl_added)
          );

   panes_h = efl_add(EFL_UI_PANES_CLASS, win,
                     efl_ui_layout_orientation_set(efl_added, EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL),
                     efl_content_set(efl_part(panes, "second"), efl_added)
                    );
   efl_add(EFL_UI_BUTTON_CLASS, win,
           efl_text_set(efl_added, "Up - user set min size(10,0)"),
           efl_gfx_hint_size_min_set(efl_added, EINA_SIZE2D(10, 0)),
           efl_content_set(efl_part(panes_h, "first"), efl_added)
          );
   efl_ui_panes_part_hint_min_allow_set(efl_part(panes_h, "first"), EINA_TRUE);

   efl_add(EFL_UI_BUTTON_CLASS, win,
           efl_text_set(efl_added, "Down - min size 50 40"),
           efl_gfx_hint_size_min_set(efl_added, EINA_SIZE2D(50, 40)),
           efl_content_set(efl_part(panes_h, "second"), efl_added)
          );

   efl_gfx_entity_size_set(win, EINA_SIZE2D(320,  400));
}

