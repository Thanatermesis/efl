#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include <Efl_Ui.h>

const char *countries[] =
{
  "Germany",
  "USA",
  "France",
  "Korea",
  "UK",
  "Romania",
  "Italy",
  NULL,
};
#define MAX_INDEX 8

/**
 * @brief Callback function invoked when the selection of a radio button changes.
 *
 * This function prints a message to the console indicating whether the radio
 * button that triggered the event has been selected or unselected.
 *
 * @param data Unused user data pointer.
 * @param ev The event information, containing the radio button object.
 */
static void
_check_button_selection_changed_cb(void *data EINA_UNUSED, const Efl_Event *ev)
{
   if (efl_ui_selectable_selected_get(ev->object))
     printf("Object %p is now selected\n", ev->object);
   else
     printf("Object %p is now unselected\n", ev->object);
}

/**
 * @brief Creates a series of radio buttons.
 *
 * This function iterates through the global `countries` array and creates
 * a radio button for each country. Each radio button is configured with a
 * state value and text.
 *
 * @param win The parent window to which the radio buttons are added.
 * @return An Eina_Array containing the created Efl_Ui_Radio widgets.
 *         The caller is responsible for freeing this array.
 *         Example of the returned array structure:
 *         [
 *           (Efl_Ui_Radio *) rbtn_germany,
 *           (Efl_Ui_Radio *) rbtn_usa,
 *           ...
 *         ]
 */
static Eina_Array*
create_radios(Efl_Ui_Win *win)
{
   Eina_Array *arr = eina_array_new(5);

   for (unsigned int i = 0; countries[i]; ++i)
     {
        Efl_Ui_Radio *rbtn = efl_add(EFL_UI_RADIO_CLASS, win);
        efl_ui_radio_state_value_set(rbtn, i);
        efl_text_set(rbtn, countries[i]);
        efl_event_callback_add(rbtn, EFL_UI_EVENT_SELECTED_CHANGED, _check_button_selection_changed_cb, NULL);
        eina_array_push(arr, rbtn);
     }

   return arr;
}

/**
 * @brief Callback function for the 'value_changed' event on the radio group.
 *
 * This function is called when the selected radio button within the group
 * changes. It retrieves the new selected index and prints the corresponding
 * country name.
 *
 * @param data Unused user data pointer.
 * @param ev The event information, containing the radio group object.
 */
static void
_value_changed_cb(void *data EINA_UNUSED, const Efl_Event *ev)
{
   Efl_Ui_Radio_Group *g = ev->object;
   int index = efl_ui_radio_group_selected_value_get(g);
   if (index == -1)
     {
        printf("Nothing is selected anymore\n");
     }
   else
     {
        EINA_SAFETY_ON_FALSE_RETURN((index >= 0) && index < MAX_INDEX);
        printf("Now selected value %s\n", countries[index]);
     }

}

/**
 * @brief Callback function to programmatically select a radio button.
 *
 * This is triggered by a button click and sets the 'selected' property of a
 * specific radio button to EINA_TRUE.
 *
 * @param data A pointer to the Efl_Ui_Check (radio button) to be selected.
 * @param ev Unused event information.
 */
static void
_select_btn_clicked(void *data, const Efl_Event *ev EINA_UNUSED)
{
   Efl_Ui_Check *c = data;

   efl_ui_selectable_selected_set(c, EINA_TRUE);
}

/**
 * @brief Callback function to set the selected value of the radio group.
 *
 * Triggered by a button click, this function directly sets the selected
 * value of the radio group, which in turn selects the corresponding radio
 * button.
 *
 * @param data A pointer to the Efl_Ui_Radio_Group.
 * @param ev Unused event information.
 */
static void
_set_selected_btn_clicked(void *data, const Efl_Event *ev EINA_UNUSED)
{
   Efl_Ui_Radio_Group *group = data;

   efl_ui_radio_group_selected_value_set(group, 0);
}

/**
 * @brief Toggles a fallback selection on the radio group.
 *
 * When clicked, this function checks if a fallback selection is already set
 * on the radio group. If not, it sets the fifth radio button ("UK") as the
 * fallback. If a fallback is already set, it clears it. A fallback selection
 * is used when no item is selected and one is requested.
 *
 * @param data A pointer to the Efl_Ui_Radio_Group (or any Efl_Ui_Selectable).
 * @param ev Unused event information.
 */
static void
_set_fallback_radio_btn_clicked(void *data, const Efl_Event *ev EINA_UNUSED)
{
   if (!efl_ui_selectable_fallback_selection_get(data))
     efl_ui_selectable_fallback_selection_set(data, efl_pack_content_get(data, 4));
   else
     efl_ui_selectable_fallback_selection_set(data, NULL);
}

/**
 * @brief Sets up and runs the Efl.Ui.Radio test.
 *
 * This function creates a window and populates it with a radio box containing
 * several radio buttons. It also adds buttons to test different interactions
 * with the radio buttons and the radio group, such as programmatic selection,
 * setting group value, and toggling a fallback selection.
 */
void test_efl_ui_radio(void *data EINA_UNUSED,
                                   Eo *obj EINA_UNUSED,
                                   void *event_info EINA_UNUSED)
{
   Efl_Ui_Win *win;
   Efl_Ui_Table *table;
   Efl_Ui_Box *bx;
   Eina_Array *arr;
   Efl_Ui_Button *o;

   win = efl_add(EFL_UI_WIN_CLASS, efl_main_loop_get(),
                                  efl_text_set(efl_added, "Efl.Ui.Radio_Box"),
                 efl_ui_win_autodel_set(efl_added, EINA_TRUE));
   table = efl_add(EFL_UI_TABLE_CLASS, win);
   efl_content_set(win, table);

   bx = efl_add(EFL_UI_RADIO_BOX_CLASS, table);
   efl_pack_table(table, bx, 0, 0, 1, 3);
   efl_event_callback_add(bx, EFL_UI_RADIO_GROUP_EVENT_VALUE_CHANGED, _value_changed_cb, NULL);

   arr = create_radios(win);
   for (unsigned int i = 0; i < eina_array_count(arr); ++i)
     {
        Efl_Ui_Radio *r = eina_array_data_get(arr, i);
        efl_pack_end(bx, r);
     }

   o = efl_add(EFL_UI_BUTTON_CLASS, table);
   efl_pack_table(table, o, 1, 0, 1, 1);
   efl_text_set(o, "Selected France check");
   efl_event_callback_add(o, EFL_INPUT_EVENT_CLICKED, _select_btn_clicked, eina_array_data_get(arr, 2));

   o = efl_add(EFL_UI_BUTTON_CLASS, table);
   efl_pack_table(table, o, 1, 1, 1, 1);
   efl_text_set(o, "Set value for Germany");
   efl_event_callback_add(o, EFL_INPUT_EVENT_CLICKED, _set_selected_btn_clicked, bx);

   o = efl_add(EFL_UI_BUTTON_CLASS, table);
   efl_pack_table(table, o, 1, 2, 1, 1);
   efl_text_set(o, "Fallback set to UK");
   efl_event_callback_add(o, EFL_INPUT_EVENT_CLICKED, _set_fallback_radio_btn_clicked, bx);

   eina_array_free(arr);
}
