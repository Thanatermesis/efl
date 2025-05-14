#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#include <Elementary.h>

#define WIDTH (500)
#define HEIGHT (1000)
#define MSG_ID_VEL (1)

/**
 * @brief Toggles an animation in the layout based on a preference setting.
 *
 * Reads the "main:animation" boolean preference. If true, it emits a "start"
 * signal to the layout's theme, otherwise it emits a "stop" signal.
 *
 * @param prefs The preferences object.
 * @param layout The layout object to control.
 */
static void
_update_animation(Evas_Object *prefs, Evas_Object *layout)
{
   Eina_Value value;
   Eina_Bool animation;

   elm_prefs_item_value_get(prefs, "main:animation", &value);
   eina_value_get(&value, &animation);
   if (animation)
     elm_layout_signal_emit(layout, "start", "animation");
   else
     elm_layout_signal_emit(layout, "stop", "animation");
}

/**
 * @brief Updates the animation time/velocity in the layout.
 *
 * Reads the "main:animation_time" float preference and sends it as a message
 * to the Edje object of the layout. This is used to control properties like
 * animation speed.
 *
 * @param prefs The preferences object.
 * @param layout The layout object containing the Edje object to message.
 */
static void
_update_animation_time(Evas_Object *prefs, Evas_Object *layout)
{
   Eina_Value value;
   float animation_time;
   Edje_Message_Float msg;

   elm_prefs_item_value_get(prefs, "main:animation_time", &value);
   eina_value_get(&value, &animation_time);
   msg.val = animation_time;
   edje_object_message_send(elm_layout_edje_get(layout), EDJE_MESSAGE_FLOAT,
                            MSG_ID_VEL, &msg);
}

/**
 * @brief Updates all animation-related settings in the layout.
 *
 * This is a convenience function that calls all individual update functions
 * to synchronize the layout's state with the current preferences.
 *
 * @param prefs The preferences object.
 * @param layout The layout object to update.
 */
static void
_update(Evas_Object *prefs, Evas_Object *layout)
{
   _update_animation(prefs, layout);
   _update_animation_time(prefs, layout);
}

/**
 * @brief Callback for the "page,loaded" event of the prefs object.
 *
 * This function is called when a preferences page is fully loaded. It
 * triggers an initial update of the layout to reflect the loaded
 * preference values.
 *
 * @param data The layout object, passed as user data.
 * @param obj The prefs object that emitted the signal.
 * @param event_info Extra event information (unused).
 */
static void
_page_loaded_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Evas_Object *layout = data;

   _update(obj, layout);
}

/**
 * @brief Callback for the "item,changed" event of the prefs object.
 *
 * Called whenever a preference item's value is changed by the user. It
 * identifies which item changed and calls the specific update function
 * for that item.
 *
 * @param data The layout object, passed as user data.
 * @param obj The prefs object that emitted the signal.
 * @param event_info A const char* containing the name of the changed item,
 *        e.g., "main:animation".
 */
static void
_item_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
   const char *item = event_info;
   Evas_Object *layout = data;

   if (!strcmp(item, "main:animation_time"))
      _update_animation_time(obj, layout);
   else if (!strcmp(item, "main:animation"))
      _update_animation(obj, layout);
}

/**
 * @brief Main function for the Elementary Prefs test.
 *
 * This function sets up a window with an elm_prefs widget. The prefs widget
 * is configured to load its structure and values from external files
 * (`.epb` and `.cfg`). It demonstrates how to interact with preference
 * items, listen for changes, and dynamically update another UI component
 * (a layout with an Edje animation) based on the preference values.
 *
 * @param data Not used.
 * @param obj Not used.
 * @param event_info Not used.
 */
void
test_prefs(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
           void *event_info EINA_UNUSED)
{
   Evas_Object *win, *bg, *prefs, *layout;
   Elm_Prefs_Data *prefs_data;
   char buf[PATH_MAX];

   win = elm_win_add(NULL, "Prefs", ELM_WIN_BASIC);
   elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);
   elm_win_title_set(win, "Prefs");
   elm_win_autodel_set(win, EINA_TRUE);

   bg = elm_bg_add(win);
   elm_win_resize_object_add(win, bg);
   evas_object_size_hint_min_set(bg, WIDTH, HEIGHT);
   evas_object_size_hint_max_set(bg, WIDTH, HEIGHT);
   evas_object_show(bg);

   layout = elm_layout_add(win);
   snprintf(buf, sizeof(buf),
            "%s/objects/test_prefs.edj", elm_app_data_dir_get());
   elm_layout_file_set(layout, buf, "prefs_edje");

   prefs = elm_prefs_add(win);
   evas_object_size_hint_weight_set(prefs, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_resize(prefs, WIDTH, HEIGHT);
   evas_object_show(prefs);

   evas_object_smart_callback_add(prefs, "page,loaded", _page_loaded_cb,
                                  layout);
   evas_object_smart_callback_add(prefs, "item,changed", _item_changed_cb,
                                  layout);

   elm_prefs_autosave_set(prefs, EINA_TRUE);

   prefs_data =
     elm_prefs_data_new("./test_prefs.cfg", NULL, EET_FILE_MODE_READ_WRITE);

   snprintf(buf, sizeof(buf),
            "%s/objects/test_prefs.epb", elm_app_data_dir_get());

   elm_prefs_file_set(prefs, buf, NULL);
   elm_prefs_data_set(prefs, prefs_data);

   elm_prefs_item_swallow(prefs, "main:swal", layout);

   _update_animation_time(prefs, layout);
   _update_animation(prefs, layout);

   evas_object_resize(win, WIDTH, HEIGHT);
   evas_object_show(win);

   elm_prefs_data_unref(prefs_data);
}
