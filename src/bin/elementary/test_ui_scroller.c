#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include <Efl_Ui.h>
#include <Elementary.h>

/**
 * @brief Callback for button click events.
 *
 * This function is invoked when a button in the UI is clicked. It prints
 * a confirmation message to standard output, including the memory address
 * of the clicked button object.
 *
 * @param data User data pointer (unused).
 * @param ev The event information structure.
 */
static void
_bt_clicked(void *data EINA_UNUSED, const Efl_Event *ev)
{
   printf("click went through on %p\n", ev->object);
}

/**
 * @brief Callback for scroll start events.
 *
 * This function is triggered when a scrolling action begins on a scroller
 * widget. It retrieves and prints the starting scroll position (x, y) of the
 * scroller's content.
 *
 * @param data User data pointer (unused).
 * @param ev The event information structure, with ev->object being the scroller.
 */
static void
_scroll_started_cb(void *data EINA_UNUSED, const Efl_Event *ev)
{
   Eina_Position2D pos = efl_ui_scrollable_content_pos_get(ev->object);
   printf("scroll start: %p x: %d y: %d\n", ev->object, pos.x, pos.y);
}

/**
 * @brief Callback for scroll finished events.
 *
 * This function is triggered when a scrolling action completes on a scroller
 * widget. It retrieves and prints the final scroll position (x, y) of the
 * scroller's content.
 *
 * @param data User data pointer (unused).
 * @param ev The event information structure, with ev->object being the scroller.
 */
static void
_scroll_finished_cb(void *data EINA_UNUSED, const Efl_Event *ev)
{
   Eina_Position2D pos = efl_ui_scrollable_content_pos_get(ev->object);
   printf("scroll finish: %p x: %d y: %d\n", ev->object, pos.x, pos.y);
}

/**
 * @brief UI test for complex scroller layouts.
 *
 * This test function creates a window containing a main vertical scroller (`sc`).
 * The main scroller contains a vertical box (`bx`) which holds various widgets
 * to test complex scrolling behaviors:
 *
 * - A slider.
 * - Several vertical buttons.
 * - A horizontal scroller (`sc2`) with horizontal buttons.
 * - More vertical buttons.
 * - A table (`gd`) containing a scroller (`sc3`) with a grid of buttons,
 *   demonstrating scrolling in both directions within a fixed area.
 * - A large number of vertical buttons to ensure the main scroller is active.
 *
 * This setup is designed to test nested scrollers, mixed scroll directions,
 * and scrollers within different layout containers.
 */
void
test_efl_ui_scroller(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Eo *win, *sc, *sc2, *sc3, *bx, *bx2, *gd, *gd2;
   int i, j;

   win = efl_add(EFL_UI_WIN_CLASS, efl_main_loop_get(),
                                  efl_text_set(efl_added, "Efl Ui Scroller"),
                 efl_ui_win_autodel_set(efl_added, EINA_TRUE));
   efl_gfx_entity_size_set(win, EINA_SIZE2D(320, 400));

   sc = efl_add(EFL_UI_SCROLLER_CLASS, win,
                efl_gfx_hint_weight_set(efl_added, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND),
                efl_event_callback_add(efl_added, EFL_UI_EVENT_SCROLL_STARTED, _scroll_started_cb, NULL),
                efl_event_callback_add(efl_added, EFL_UI_EVENT_SCROLL_FINISHED, _scroll_finished_cb, NULL),
                efl_content_set(win, efl_added));

   bx = efl_add(EFL_UI_BOX_CLASS, sc,
                efl_ui_layout_orientation_set(efl_added, EFL_UI_LAYOUT_ORIENTATION_VERTICAL),
                efl_gfx_hint_weight_set(efl_added, EVAS_HINT_EXPAND, 0),
                efl_gfx_hint_align_set(efl_added, 0.5, 0),
                efl_gfx_hint_fill_set(efl_added, EINA_TRUE, EINA_FALSE),
                efl_content_set(sc, efl_added));

   efl_add(EFL_UI_SLIDER_CLASS, bx,
           efl_gfx_hint_size_min_set(efl_added, EINA_SIZE2D(160, 0)),
           efl_pack(bx, efl_added));

   for (i = 0; i < 3; i++)
      {
        efl_add(EFL_UI_BUTTON_CLASS, bx,
                efl_text_set(efl_added, "Vertical"),
                efl_gfx_hint_weight_set(efl_added, EVAS_HINT_EXPAND, 0.0),
                efl_gfx_hint_fill_set(efl_added, EINA_TRUE, EINA_FALSE),
                efl_event_callback_add(efl_added, EFL_INPUT_EVENT_CLICKED, _bt_clicked, NULL),
                efl_pack(bx, efl_added));
      }

   sc2 = efl_add(EFL_UI_SCROLLER_CLASS, bx,
                 efl_ui_scrollable_match_content_set(efl_added, EINA_FALSE, EINA_TRUE),
                 efl_pack(bx, efl_added));

   bx2 = efl_add(EFL_UI_BOX_CLASS, sc2,
                 efl_ui_layout_orientation_set(efl_added, EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL),
                 efl_content_set(sc2, efl_added));

   for (i = 0; i < 10; i++)
      {
        efl_add(EFL_UI_BUTTON_CLASS, bx2,
                efl_text_set(efl_added, "... Horizontal scrolling ..."),
                efl_event_callback_add(efl_added, EFL_INPUT_EVENT_CLICKED, _bt_clicked, NULL),
                efl_pack(bx2, efl_added));
      }

   for (i = 0; i < 3; i++)
      {
        efl_add(EFL_UI_BUTTON_CLASS, bx,
                efl_text_set(efl_added, "Vertical"),
                efl_gfx_hint_weight_set(efl_added, EVAS_HINT_EXPAND, 0.0),
                efl_gfx_hint_fill_set(efl_added, EINA_TRUE, EINA_FALSE),
                efl_event_callback_add(efl_added, EFL_INPUT_EVENT_CLICKED, _bt_clicked, NULL),
                efl_pack(bx, efl_added));
      }

   gd = efl_add(EFL_UI_TABLE_CLASS, bx,
                efl_gfx_hint_weight_set(efl_added, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND),
                efl_gfx_hint_align_set(efl_added, 0.5, 0),
                efl_pack(bx, efl_added));

   efl_add(EFL_CANVAS_RECTANGLE_CLASS, win,
           efl_gfx_color_set(efl_added, 0, 0, 0, 0),
           efl_gfx_hint_size_min_set(efl_added, EINA_SIZE2D(200, 120)),
           efl_pack_table(gd, efl_added, 0, 0, 1, 1));

   sc3 = efl_add(EFL_UI_SCROLLER_CLASS, win,
                 efl_gfx_hint_weight_set(efl_added, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND),
                 efl_gfx_hint_fill_set(efl_added, EINA_TRUE, EINA_TRUE),
                 efl_pack_table(gd, efl_added, 0, 0, 1, 1));

   gd2 = efl_add(EFL_UI_TABLE_CLASS, sc3,
                 efl_content_set(sc3, efl_added));

   for (j = 0; j < 16; j++)
     {
        for (i = 0; i < 16; i++)
          {
             efl_add(EFL_UI_BUTTON_CLASS, win,
                     efl_text_set(efl_added, "Both"),
                     efl_event_callback_add(efl_added, EFL_INPUT_EVENT_CLICKED, _bt_clicked, NULL),
                     efl_pack_table(gd2, efl_added, i, j, 1, 1));
          }
     }

   for (i = 0; i < 200; i++)
      {
        efl_add(EFL_UI_BUTTON_CLASS, bx,
                efl_text_set(efl_added, "Vertical"),
                efl_gfx_hint_weight_set(efl_added, EVAS_HINT_EXPAND, 0.0),
                efl_gfx_hint_fill_set(efl_added, EINA_TRUE, EINA_FALSE),
                efl_event_callback_add(efl_added, EFL_INPUT_EVENT_CLICKED, _bt_clicked, NULL),
                efl_pack(bx, efl_added));
      }
}

/**
 * @brief UI test for a simple vertical scroller.
 *
 * This test creates a window with a single vertical scroller (`sc`).
 * The scroller contains a vertical box (`bx`) packed with a large number
 * (2000) of buttons. This is a basic performance and functionality test
 * for a simple, long, vertical scrolling list.
 */
void
test_efl_ui_scroller_simple(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Eo *win, *sc, *bx;
   int i;

   win = efl_add(EFL_UI_WIN_CLASS, efl_main_loop_get(),
                                  efl_text_set(efl_added, "Efl Ui Scroller Simple"),
                 efl_ui_win_autodel_set(efl_added, EINA_TRUE));
   efl_gfx_entity_size_set(win, EINA_SIZE2D(320, 400));

   sc = efl_add(EFL_UI_SCROLLER_CLASS, win,
                efl_gfx_hint_weight_set(efl_added, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND),
                efl_event_callback_add(efl_added, EFL_UI_EVENT_SCROLL_STARTED, _scroll_started_cb, NULL),
                efl_event_callback_add(efl_added, EFL_UI_EVENT_SCROLL_FINISHED, _scroll_finished_cb, NULL),
                efl_content_set(win, efl_added));

   bx = efl_add(EFL_UI_BOX_CLASS, sc,
                efl_ui_layout_orientation_set(efl_added, EFL_UI_LAYOUT_ORIENTATION_VERTICAL),
                efl_gfx_hint_weight_set(efl_added, EVAS_HINT_EXPAND, 0),
                efl_gfx_hint_align_set(efl_added, 0.5, 0),
                efl_gfx_hint_fill_set(efl_added, EINA_TRUE, EINA_FALSE),
                efl_content_set(sc, efl_added));

   for (i = 0; i < 2000; i++)
      {
        efl_add(EFL_UI_BUTTON_CLASS, bx,
                efl_text_set(efl_added, "Vertical"),
                efl_gfx_hint_weight_set(efl_added, EVAS_HINT_EXPAND, 0.0),
                efl_gfx_hint_fill_set(efl_added, EINA_TRUE, EINA_FALSE),
                efl_event_callback_add(efl_added, EFL_INPUT_EVENT_CLICKED, _bt_clicked, NULL),
                efl_pack(bx, efl_added));
      }
}

/**
 * @brief UI test for a scroller with a table layout.
 *
 * This test sets up a window with a scroller (`sc`) containing a table (`tb`)
 * with two columns. It then populates the table with 1000 rows, where each
 * row has two buttons. This tests the scroller's behavior when its content
 * is a table layout manager.
 */
void
test_efl_ui_scroller_simple2(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Eo *win, *sc, *tb;
   int i;

   win = efl_add(EFL_UI_WIN_CLASS, efl_main_loop_get(),
                                  efl_text_set(efl_added, "Efl Ui Scroller Simple2"),
                 efl_ui_win_autodel_set(efl_added, EINA_TRUE));
   efl_gfx_entity_size_set(win, EINA_SIZE2D(320, 400));

   sc = efl_add(EFL_UI_SCROLLER_CLASS, win,
                efl_gfx_hint_weight_set(efl_added, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND),
                efl_event_callback_add(efl_added, EFL_UI_EVENT_SCROLL_STARTED, _scroll_started_cb, NULL),
                efl_event_callback_add(efl_added, EFL_UI_EVENT_SCROLL_FINISHED, _scroll_finished_cb, NULL),
                efl_content_set(win, efl_added));

   tb = efl_add(EFL_UI_TABLE_CLASS, sc,
                efl_pack_table_columns_set(efl_added, 2),
                efl_gfx_hint_weight_set(efl_added, EVAS_HINT_EXPAND, 0),
                efl_gfx_hint_align_set(efl_added, 0.5, 0),
                efl_gfx_hint_fill_set(efl_added, EINA_TRUE, EINA_FALSE),
                efl_content_set(sc, efl_added));

   for (i = 0; i < 1000; i++)
      {
        efl_add(EFL_UI_BUTTON_CLASS, tb,
                efl_text_set(efl_added, "Vertical"),
                efl_gfx_hint_weight_set(efl_added, EVAS_HINT_EXPAND, 0.0),
                efl_gfx_hint_fill_set(efl_added, EINA_TRUE, EINA_FALSE),
                efl_event_callback_add(efl_added, EFL_INPUT_EVENT_CLICKED, _bt_clicked, NULL),
                efl_pack_table(tb, efl_added, 0, i, 1, 1));
        efl_add(EFL_UI_BUTTON_CLASS, tb,
                efl_text_set(efl_added, "Horizontal"),
                efl_gfx_hint_weight_set(efl_added, EVAS_HINT_EXPAND, 0.0),
                efl_gfx_hint_fill_set(efl_added, EINA_TRUE, EINA_FALSE),
                efl_event_callback_add(efl_added, EFL_INPUT_EVENT_CLICKED, _bt_clicked, NULL),
                efl_pack_table(tb, efl_added, 1, i, 1, 1));
      }
}
