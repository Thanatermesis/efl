#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#include <Elementary.h>

typedef struct _App_Data App_Data;

/**
 * @brief Application specific data structure.
 * This structure holds all the necessary data for the WM rotation test application.
 */
struct _App_Data
{
   Eina_Bool    wm_rot_supported; /**< Flag indicating if WM rotation is supported by the window manager. */
   Eina_List   *chs;             /**< List of Evas_Object check widgets, each representing a potential rotation angle (0, 90, 180, 270). */
   int          available_rots[4];/**< Array to store the selected available rotation angles. For example: `{0, 90, 270, 0}` if 0, 90, 270 are selected. The fourth element is implicitly 0 if fewer than 4 are selected. */
   Evas_Object *lb;              /**< Label widget to display the current window rotation. */
   Evas_Object *rdg;             /**< Radio group widget for selecting the preferred rotation. */
};

/**
 * @brief Callback function to set the available rotations for the window.
 *
 * This function is triggered when the "Available rotations" button is clicked.
 * It reads the states of the check boxes and sets the window manager's
 * available rotations accordingly.
 *
 * @param data The Evas_Object (window) passed during callback registration.
 * @param obj The Evas_Object (button) that triggered the callback (unused).
 * @param event_info Event-specific information (unused).
 */
static void
_bt_available_rots_set(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win = data;
   App_Data *ad = evas_object_data_get(win, "ad");
   Evas_Object *o;
   Eina_List *l;
   const char *str;
   unsigned int i = 0;

   if (!ad->wm_rot_supported) return;

   EINA_LIST_FOREACH(ad->chs, l, o)
     {
        if (!elm_check_state_get(o)) continue;
        str = elm_object_text_get(o);
        if (!str) continue;
        ad->available_rots[i] = atoi(str);
        i++;
     }

   elm_win_wm_rotation_available_rotations_set
     (win, ad->available_rots, i);
}

/**
 * @brief Callback function to set the preferred rotation for the window.
 *
 * This function is triggered when the "Preferred rotation" button is clicked.
 * It reads the selected radio button's value and sets the window manager's
 * preferred rotation. A value of -1 indicates that the preference should be unset.
 *
 * @param data The Evas_Object (window) passed during callback registration.
 * @param obj The Evas_Object (button) that triggered the callback (unused).
 * @param event_info Event-specific information (unused).
 */
static void
_bt_preferred_rot_set(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win = (Evas_Object *)(data);
   App_Data *ad = evas_object_data_get(win, "ad");

   if (!ad->wm_rot_supported) return;

   Evas_Object *rd = elm_radio_selected_object_get(ad->rdg);
   if (rd)
     {
        const char *str = elm_object_text_get(rd);
        int rot = 0;

        if (!strcmp(str, "Unset"))
          rot = -1;
        else
          rot = atoi(str);

        elm_win_wm_rotation_preferred_rotation_set(win, rot);
     }
}

/**
 * @brief Callback function invoked when the window's WM rotation changes.
 *
 * This function updates a label in the UI to display the new current rotation
 * of the window.
 *
 * @param data The Evas_Object (window) passed during callback registration.
 * @param obj The Evas_Object (window) that emitted the "wm,rotation,changed" signal (unused).
 * @param event Event-specific information (unused).
 */
static void
_win_wm_rotation_changed_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   Evas_Object *win = data;
   App_Data *ad = evas_object_data_get(win, "ad");
   int rot = elm_win_rotation_get(win);
   char buf[32];

   if (!ad->wm_rot_supported) return;

   eina_convert_itoa(rot, buf);
   elm_object_text_set(ad->lb, eina_stringshare_add(buf));
}

/**
 * @brief Callback function invoked when the window receives a delete request.
 *
 * This function is responsible for cleaning up application-specific data,
 * primarily freeing the App_Data structure and its contents.
 *
 * @param data Custom data pointer passed at callback registration (unused here, as App_Data is retrieved from the object).
 * @param obj The Evas_Object (window) that is being deleted.
 * @param event_info Event-specific information (unused).
 */
static void
_win_del_cb(void *data EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   App_Data *ad = evas_object_data_get(obj, "ad");
   Evas_Object *o;

   if (ad->wm_rot_supported)
     {
        EINA_LIST_FREE(ad->chs, o)
          evas_object_data_del(o, "rotation");
     }

   free(ad);
}

/**
 * @brief Main function to set up and run the WM rotation test.
 *
 * This function creates the main window and UI elements for testing
 * window manager rotation capabilities. It initializes UI components for:
 * - Displaying if WM rotation is supported.
 * - Setting available rotations (0, 90, 180, 270 degrees).
 * - Setting a preferred rotation.
 * - Displaying the current window rotation.
 * - An entry field for general interaction testing.
 *
 * @param data Custom data, unused in this test.
 * @param obj Parent Evas_Object, unused in this test.
 * @param event_info Event-specific information, unused in this test.
 */
void
test_win_wm_rotation(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   App_Data *ad;
   Evas_Object *win, *bx, *bx2, *lb, *ch, *bt, *en, *rd, *rdg = NULL;
   int i;
   char buf[32];

   if (!(ad = calloc(1, sizeof(App_Data)))) return;

   win = elm_win_util_standard_add("wmrotation", "WMRotation");
   elm_win_autodel_set(win, EINA_TRUE);
   evas_object_data_set(win, "ad", ad);

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bx);
   evas_object_show(bx);

   lb = elm_label_add(win);
   elm_object_text_set(lb, "<b>Window manager rotation test</b>");
   evas_object_size_hint_weight_set(lb, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_box_pack_end(bx, lb);
   evas_object_show(lb);

   ad->wm_rot_supported = elm_win_wm_rotation_supported_get(win);
   if (ad->wm_rot_supported)
     {
        int rots[] = { 0, 90, 270 };
        elm_win_wm_rotation_available_rotations_set(win, rots, (sizeof(rots) / sizeof(int)));
        elm_win_wm_rotation_preferred_rotation_set(win, 90);

        bx2 = elm_box_add(win);
        evas_object_size_hint_weight_set(bx2, EVAS_HINT_EXPAND, 0.0);
        evas_object_size_hint_align_set(bx2, EVAS_HINT_FILL, 0.0);
        elm_box_align_set(bx2, 0.0, 0.5);
        elm_box_horizontal_set(bx2, EINA_TRUE);
        elm_box_pack_end(bx, bx2);
        evas_object_show(bx2);

        for (i = 0; i < 4; i++)
          {
             ch = elm_check_add(win);
             eina_convert_itoa((i * 90), buf);
             elm_object_text_set(ch, eina_stringshare_add(buf));
             evas_object_size_hint_weight_set(ch, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
             elm_box_pack_end(bx2, ch);
             evas_object_show(ch);

             if (i != 2) elm_check_state_set(ch, EINA_TRUE);

             ad->chs = eina_list_append(ad->chs, ch);
          }

        bt = elm_button_add(win);
        elm_object_text_set(bt, "Available rotations");
        evas_object_smart_callback_add(bt, "clicked", _bt_available_rots_set, win);
        elm_box_pack_end(bx, bt);
        evas_object_show(bt);

        bx2 = elm_box_add(win);
        evas_object_size_hint_weight_set(bx2, EVAS_HINT_EXPAND, 0.0);
        evas_object_size_hint_align_set(bx2, EVAS_HINT_FILL, 0.0);
        elm_box_align_set(bx2, 0.0, 0.5);
        elm_box_horizontal_set(bx2, EINA_TRUE);
        elm_box_pack_end(bx, bx2);
        evas_object_show(bx2);

        for (i = 0; i < 5; i++)
          {
             rd = elm_radio_add(win);
             if (!rdg) rdg = rd;
             elm_radio_state_value_set(rd, i);
             elm_radio_group_add(rd, rdg);

             if (i == 0)
               elm_object_text_set(rd, "Unset");
             else
               {
                  eina_convert_itoa(((i - 1) * 90), buf);
                  elm_object_text_set(rd, eina_stringshare_add(buf));
               }

             evas_object_size_hint_weight_set(rd, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
             elm_box_pack_end(bx2, rd);
             evas_object_show(rd);
          }

        elm_radio_value_set(rdg, 2);
        ad->rdg = rdg;

        bt = elm_button_add(win);
        elm_object_text_set(bt, "Preferred rotation");
        evas_object_smart_callback_add(bt, "clicked", _bt_preferred_rot_set, win);
        elm_box_pack_end(bx, bt);
        evas_object_show(bt);

        evas_object_smart_callback_add(win, "wm,rotation,changed", _win_wm_rotation_changed_cb, win);
     }
   else
     printf("Window manager doesn't support rotation\n");

   en = elm_entry_add(win);
   elm_entry_single_line_set(en, EINA_TRUE);
   evas_object_size_hint_weight_set(en, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(en, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(bx, en);
   evas_object_show(en);

   lb = elm_label_add(win);
   elm_object_text_set(lb, "N/A");
   evas_object_size_hint_weight_set(lb, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_box_pack_end(bx, lb);
   evas_object_show(lb);
   ad->lb = lb;

   evas_object_smart_callback_add(win, "delete,request", _win_del_cb, NULL);

   evas_object_resize(win, 480 * elm_config_scale_get(),
                           400 * elm_config_scale_get());
   evas_object_show(win);
}
