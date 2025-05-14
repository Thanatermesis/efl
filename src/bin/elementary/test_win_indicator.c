#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#include <Efl_Ui.h>
#include <Elementary.h>

/**
 * @brief Callback function invoked when the clock value changes.
 * @param data User data, unused in this callback.
 * @param ev Event information, unused in this callback.
 */
static void
_changed_cb(void *data EINA_UNUSED, const Efl_Event *ev EINA_UNUSED)
{
   printf("Clock value is changed\n");
}

/**
 * @brief Creates and configures the indicator window.
 *
 * This function sets up a socket window to act as an indicator,
 * adds a clock to it, and configures the clock's appearance and behavior.
 *
 * @return A pointer to the created Evas_Object (the indicator window),
 *         or NULL on failure.
 */
static Evas_Object *
_create_indicator(void)
{
   const char *indi_name;

   Eo *win = efl_add_ref(EFL_UI_WIN_SOCKET_CLASS, NULL,
		     efl_text_set(efl_added, "indicator"),
		     efl_ui_win_autodel_set(efl_added, EINA_TRUE),
		     efl_ui_win_alpha_set(efl_added, EINA_TRUE));

   if (!win)
     {
        printf("fail to create a portrait indicator window\n");
        return NULL;
     }

   indi_name = "elm_indicator_portrait";

   if (!efl_ui_win_socket_listen(win, indi_name, 0, EINA_FALSE))
     {
        printf("failed to listen portrait window socket.\n");
        efl_del(win);
        return NULL;
     }

   Eo *bx = efl_add(EFL_UI_BOX_CLASS, win,
                    efl_content_set(win, efl_added));

   efl_add(EFL_UI_CLOCK_CLASS, bx,
           efl_gfx_hint_weight_set(efl_added, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND),
           efl_gfx_hint_align_set(efl_added, 0.5, 0.5),
           efl_ui_clock_field_visible_set(efl_added, EFL_UI_CLOCK_TYPE_HOUR, EINA_FALSE),
           efl_ui_clock_field_visible_set(efl_added, EFL_UI_CLOCK_TYPE_MINUTE, EINA_FALSE),
           efl_ui_clock_field_visible_set(efl_added, EFL_UI_CLOCK_TYPE_AMPM, EINA_FALSE),
           efl_ui_clock_field_visible_set(efl_added, EFL_UI_CLOCK_TYPE_SECOND, EINA_FALSE),
           efl_ui_clock_field_visible_set(efl_added, EFL_UI_CLOCK_TYPE_DAY, EINA_FALSE),
           efl_ui_clock_pause_set(efl_added, EINA_TRUE),
           efl_event_callback_add(efl_added, EFL_UI_CLOCK_EVENT_CHANGED, _changed_cb, NULL),
           efl_pack(bx, efl_added));

   efl_gfx_entity_size_set(win, EINA_SIZE2D(300, 30));
   return win;
}

/**
 * @brief Callback function invoked when the "Indicator Off" button is clicked.
 * Sets the indicator mode to OFF.
 * @param data The window object whose indicator mode is to be changed.
 * @param ev Event information, unused in this callback.
 */
static void
_off_clicked(void *data, const Efl_Event *ev EINA_UNUSED)
{
	efl_ui_win_indicator_mode_set(data, EFL_UI_WIN_INDICATOR_MODE_OFF);
}

/**
 * @brief Callback function invoked when the "Bg Opaque" button is clicked.
 * Sets the indicator mode to BG_OPAQUE.
 * @param data The window object whose indicator mode is to be changed.
 * @param ev Event information, unused in this callback.
 */
static void
_opaque_clicked(void *data, const Efl_Event *ev EINA_UNUSED)
{
	efl_ui_win_indicator_mode_set(data, EFL_UI_WIN_INDICATOR_MODE_BG_OPAQUE);
}

/**
 * @brief Callback function invoked when the "Bg Transparent" button is clicked.
 * Sets the indicator mode to BG_TRANSPARENT.
 * @param data The window object whose indicator mode is to be changed.
 * @param ev Event information, unused in this callback.
 */
static void
_transparent_clicked(void *data, const Efl_Event *ev EINA_UNUSED)
{
	efl_ui_win_indicator_mode_set(data, EFL_UI_WIN_INDICATOR_MODE_BG_TRANSPARENT);
}

/**
 * @brief Callback function invoked when the "Hidden" button is clicked.
 * Sets the indicator mode to HIDDEN.
 * @param data The window object whose indicator mode is to be changed.
 * @param ev Event information, unused in this callback.
 */
static void
_hidden_clicked(void *data, const Efl_Event *ev EINA_UNUSED)
{
	efl_ui_win_indicator_mode_set(data, EFL_UI_WIN_INDICATOR_MODE_HIDDEN);
}

/**
 * @brief Callback function invoked when the main window is deleted.
 * Deletes the associated indicator window.
 * @param data The indicator window object to be deleted.
 * @param ev Event information, unused in this callback.
 */
static void
_win_del(void *data, const Efl_Event *ev EINA_UNUSED)
{
	efl_del(data);
}

/**
 * @brief Main test function for the Efl.Win.Indicator.
 *
 * This function creates an indicator window and a main control window.
 * The control window contains buttons to change the indicator's mode.
 *
 * @param data Unused.
 * @param obj Unused.
 * @param event_info Unused.
 */
void
test_win_indicator(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Eo *win, *indicator;

   indicator = _create_indicator();

   // FIXME: Resizing window should no cause sizing issues!
   win = efl_add(EFL_UI_WIN_CLASS, efl_main_loop_get(),
                                  efl_text_set(efl_added, "Efl.Win.Indicator"),
                 efl_gfx_hint_size_max_set(efl_added, EINA_SIZE2D(300, -1)),
                 efl_ui_win_autodel_set(efl_added, EINA_TRUE));
   efl_event_callback_add(win, EFL_EVENT_DEL, _win_del, indicator);
   efl_gfx_entity_size_set(win, EINA_SIZE2D(300, 360));

   Eo *bx = efl_add(EFL_UI_BOX_CLASS, win,
                    efl_gfx_hint_size_min_set(efl_added, EINA_SIZE2D(300, 0)),
                    efl_content_set(win, efl_added));

   efl_add(EFL_UI_BUTTON_CLASS, win,
           efl_text_set(efl_added, "Indicator Off"),
           efl_event_callback_add(efl_added, EFL_INPUT_EVENT_CLICKED, _off_clicked, win),
           efl_pack(bx, efl_added));
   efl_add(EFL_UI_BUTTON_CLASS, win,
           efl_text_set(efl_added, "Bg Opaque"),
           efl_event_callback_add(efl_added, EFL_INPUT_EVENT_CLICKED, _opaque_clicked, win),
           efl_pack(bx, efl_added));
   efl_add(EFL_UI_BUTTON_CLASS, win,
           efl_text_set(efl_added, "Bg Transparent"),
           efl_event_callback_add(efl_added, EFL_INPUT_EVENT_CLICKED, _transparent_clicked, win),
           efl_pack(bx, efl_added));
   efl_add(EFL_UI_BUTTON_CLASS, win,
           efl_text_set(efl_added, "Hidden"),
           efl_event_callback_add(efl_added, EFL_INPUT_EVENT_CLICKED, _hidden_clicked, win),
           efl_pack(bx, efl_added));
}
