#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#include <Elementary.h>

/**
 * @brief Structure to hold application data.
 */
struct _api_data
{
   Evas_Object *win;            /**< The main window object */
   Elm_Win_Keygrab_Mode mode;  /**< Current keygrab mode selected by the user */
   char keyname[PATH_MAX];     /**< Key name to be grabbed/ungrabbed */
};

typedef struct _api_data api_data;

/**
 * @brief Callback function for radio button group changes.
 *
 * This function is called when the selected radio button in the keygrab mode
 * group changes. It updates the keygrab mode in the api_data structure.
 *
 * @param data Pointer to the api_data structure.
 * @param obj The radio object that triggered the event.
 * @param event_info Not used.
 */
static void
_rdg_changed_cb(void *data EINA_UNUSED, Evas_Object *obj,
               void *event_info EINA_UNUSED)
{
   int mode = -1;
   Elm_Win_Keygrab_Mode keygrab_mode = ELM_WIN_KEYGRAB_UNKNOWN;
   api_data *api = data;

   mode = elm_radio_value_get(obj);
   printf("radio group value : %d\n", mode);

   if (mode == 1)
     {
        keygrab_mode = ELM_WIN_KEYGRAB_SHARED;
        printf("keygrab mode = ELM_WIN_KEYGRAB_SHARED\n");
     }
   else if (mode == 2)
     {
        keygrab_mode = ELM_WIN_KEYGRAB_TOPMOST;
        printf("keygrab mode = ELM_WIN_KEYGRAB_TOPMOST\n");
     }
   else if (mode == 3)
     {
        keygrab_mode = ELM_WIN_KEYGRAB_EXCLUSIVE;
        printf("keygrab mode = ELM_WIN_KEYGRAB_EXCLUSIVE\n");
     }
   else if (mode == 4)
     {
        keygrab_mode = ELM_WIN_KEYGRAB_OVERRIDE_EXCLUSIVE;
        printf("keygrab mode = ELM_WIN_KEYGRAB_OVERRIDE_EXCLUSIVE\n");
     }
   else
     {
        keygrab_mode = ELM_WIN_KEYGRAB_UNKNOWN;
        printf("keygrab mode = ELM_WIN_KEYGRAB_UNKNOWN\n");
     }
   api->mode = keygrab_mode;
}

/**
 * @brief Callback function for entry field changes.
 *
 * This function is called when the text in the keyname entry field changes.
 * It updates the keyname in the api_data structure.
 *
 * @param data Pointer to the api_data structure.
 * @param obj The entry object that triggered the event.
 * @param event_info Not used.
 */
static void
_entry_changed_cb(void *data , Evas_Object *obj, void *event_info EINA_UNUSED)
{
   api_data *api = data;
   const char *str = elm_entry_entry_get(obj);
   if (!str) return;
   strncpy(api->keyname, str, PATH_MAX - 1);
   printf("entry: %s\n",str);
}

/**
 * @brief Callback function for cleaning up resources.
 *
 * This function is called when the main window is freed.
 * It frees the allocated api_data structure.
 *
 * @param data Pointer to the api_data structure.
 * @param e Not used.
 * @param obj Not used.
 * @param event_info Not used.
 */
static void
_cleanup_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   if (data)
     free(data);
}

/**
 * @brief Callback function for the "Keygrab Set" button click.
 *
 * This function is called when the "Keygrab Set" button is clicked.
 * It attempts to set a keygrab on the window using the current keyname and mode
 * from the api_data structure.
 *
 * @param data Pointer to the api_data structure.
 * @param obj Not used.
 * @param event_info Not used.
 */
static void
_set_bt_clicked(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Eina_Bool ret = EINA_FALSE;
   api_data *api = data;

   ret = elm_win_keygrab_set(api->win, api->keyname, 0, 0, 0, api->mode);
   printf("Keyname:\"%s\" mode:\"%d\" keygrab set. ret=%d\n", api->keyname, api->mode, ret);
}

/**
 * @brief Callback function for the "Keygrab Unset" button click.
 *
 * This function is called when the "Keygrab Unset" button is clicked.
 * It attempts to unset a keygrab on the window for the current keyname.
 *
 * @param data Pointer to the api_data structure.
 * @param obj Not used.
 * @param event_info Not used.
 */
static void
_unset_bt_clicked(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Eina_Bool ret = EINA_FALSE;
   api_data *api = data;

   ret = elm_win_keygrab_unset(api->win, api->keyname, 0, 0);
   printf("Keyname:\"%s\" mode:\"%d\" keygrab unset. ret=%d.\n", api->keyname, api->mode, ret);
}

/**
 * @brief Creates UI elements for keyname input.
 *
 * This function creates a frame containing a label and an entry field
 * for the user to input the keyname to be grabbed.
 *
 * @param bx The parent box widget to pack UI elements into.
 * @param api Pointer to the api_data structure.
 */
static void
_group_1_create(Evas_Object *bx, api_data *api)
{
   Evas_Object *bx2, *fr, *lb, *en;

   fr = elm_frame_add(bx);
   elm_object_text_set(fr, "Keygrab Keyname");
   evas_object_size_hint_weight_set(fr, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(fr, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(bx, fr);
   evas_object_show(fr);

   bx2 = elm_box_add(fr);
   elm_object_content_set(fr, bx2);
   evas_object_show(bx2);

   lb = elm_label_add(bx2);
   elm_object_text_set(lb, " Keyname :");
   evas_object_size_hint_weight_set(lb, 0.0, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(lb, 0.0, EVAS_HINT_FILL);
   elm_box_pack_end(bx2, lb);
   evas_object_show(lb);

   en = elm_entry_add(bx2);
   elm_entry_single_line_set(en, EINA_TRUE);
   elm_entry_scrollable_set(en, EINA_TRUE);
   evas_object_size_hint_weight_set(en, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(en, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_smart_callback_add(en, "changed,user", _entry_changed_cb, api);
   elm_box_pack_end(bx2, en);
   evas_object_show(en);
   elm_object_focus_set(en, EINA_TRUE);


}

/**
 * @brief Creates UI elements for selecting keygrab mode.
 *
 * This function creates a frame containing a radio button group
 * for the user to select the desired keygrab mode.
 *
 * @param bx The parent box widget to pack UI elements into.
 * @param api Pointer to the api_data structure.
 */
static void
_group_2_create(Evas_Object *bx, api_data *api)
{
   Evas_Object *bx2, *fr, *rd, *rd1, *rd2, *rd3;

   fr = elm_frame_add(bx);
   elm_object_text_set(fr, "Keygrab Mode");
   evas_object_size_hint_weight_set(fr, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(fr, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(bx, fr);
   evas_object_show(fr);

   bx2 = elm_box_add(fr);
   elm_object_content_set(fr, bx2);
   elm_box_align_set(bx2, 0.0, 0.0);
   evas_object_show(bx2);

   // Shared
   rd = elm_radio_add(bx2);
   elm_radio_state_value_set(rd, 1);
   evas_object_size_hint_weight_set(rd, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(rd, EVAS_HINT_FILL, 0.5);
   elm_object_text_set(rd, "Keygrab Shared Mode");
   elm_box_pack_end(bx2, rd);
   evas_object_show(rd);
   evas_object_smart_callback_add(rd, "changed", _rdg_changed_cb, api);

   // Topmost
   rd1 = elm_radio_add(bx2);
   elm_radio_state_value_set(rd1, 2);
   evas_object_size_hint_weight_set(rd1, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(rd1, EVAS_HINT_FILL, 0.5);
   elm_object_text_set(rd1, "Keygrab Topmost Mode");
   elm_radio_group_add(rd1, rd);
   elm_box_pack_end(bx2, rd1);
   evas_object_show(rd1);
   evas_object_smart_callback_add(rd1, "changed", _rdg_changed_cb, api);

   // Exclusive
   rd2 = elm_radio_add(bx2);
   elm_radio_state_value_set(rd2, 3);
   evas_object_size_hint_weight_set(rd2, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(rd2, EVAS_HINT_FILL, 0.5);
   elm_object_text_set(rd2, "Keygrab Exclusive Mode");
   elm_radio_group_add(rd2, rd);
   elm_box_pack_end(bx2, rd2);
   evas_object_show(rd2);
   evas_object_smart_callback_add(rd2, "changed", _rdg_changed_cb, api);

   //Override Exclusive
   rd3 = elm_radio_add(bx2);
   elm_radio_state_value_set(rd3, 4);
   evas_object_size_hint_weight_set(rd3, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(rd3, EVAS_HINT_FILL, 0.5);
   elm_object_text_set(rd3, "Keygrab Override Exclusive Mode");
   elm_radio_group_add(rd3, rd);
   elm_box_pack_end(bx2, rd3);
   evas_object_show(rd3);
   evas_object_smart_callback_add(rd3, "changed", _rdg_changed_cb, api);
}

/**
 * @brief Creates UI elements for keygrab actions.
 *
 * This function creates a "Keygrab Set" button and a "Keygrab Unset" button.
 *
 * @param bx The parent box widget to pack UI elements into.
 * @param api Pointer to the api_data structure.
 */
static void
_group_3_create(Evas_Object *bx, api_data *api)
{
   Evas_Object *bt;

   bt = elm_button_add(bx);
   elm_object_text_set(bt, "Keygrab Set");
   elm_box_pack_end(bx, bt);

   evas_object_smart_callback_add(bt, "clicked", _set_bt_clicked, api);
   evas_object_show(bt);


   bt = elm_button_add(bx);
   elm_object_text_set(bt, "Keygrab Unset");
   elm_box_pack_end(bx, bt);
   evas_object_smart_callback_add(bt, "clicked", _unset_bt_clicked, api);
   evas_object_show(bt);
}

/**
 * @brief Main function for the keygrab test.
 *
 * This function sets up the main window and UI elements for testing
 * the elm_win_keygrab_set and elm_win_keygrab_unset functionalities.
 * It creates input fields for the keyname, radio buttons for the grab mode,
 * and buttons to set or unset the keygrab.
 *
 * @param data Not used.
 * @param obj Not used.
 * @param event_info Not used.
 */
void
test_win_keygrab(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
           void *event_info EINA_UNUSED)
{
   Evas_Object *win, *bx, *bx1, *bx2;

   api_data *api = calloc(1, sizeof(api_data));

   win = elm_win_util_standard_add("keygrab", "Keygrab");
   elm_win_autodel_set(win, EINA_TRUE);
   evas_object_event_callback_add(win, EVAS_CALLBACK_FREE, _cleanup_cb, api);

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bx);
   evas_object_show(bx);

   bx1 = elm_box_add(win);
   evas_object_size_hint_weight_set(bx1, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_box_pack_end(bx, bx1);
   elm_box_horizontal_set(bx1, EINA_TRUE);
   evas_object_show(bx1);

   bx2 = elm_box_add(win);
   evas_object_size_hint_weight_set(bx2, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_box_pack_end(bx, bx2);
   elm_box_horizontal_set(bx2, EINA_TRUE);
   evas_object_show(bx2);

   _group_1_create(bx1, api);
   _group_2_create(bx1, api);
   _group_3_create(bx2, api);

   evas_object_show(win);
}
