#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#include <Elementary.h>

/**
 * @brief Callback function for when a position is selected on an actionslider.
 *
 * This function is called when the user selects a position on the actionslider.
 * It prints the selected position's label and debug information about the
 * indicator, magnet, and enabled positions of the actionslider.
 *
 * @param data User data pointer (unused).
 * @param obj The actionslider object.
 * @param event_info The selected position's label.
 */
static void _pos_selected_cb(void *data EINA_UNUSED, Evas_Object *obj, void *event_info)
{
   Elm_Actionslider_Pos ipos, mpos, epos;

   printf("Selection: %s\n", (char *)event_info);
   printf("Label selected: %s\n", elm_actionslider_selected_label_get(obj));

   ipos = elm_actionslider_indicator_pos_get(obj);

   switch (ipos)
     {
      case ELM_ACTIONSLIDER_NONE:
         printf("actionslider indicator pos: none!\n");
         break;
      case ELM_ACTIONSLIDER_LEFT:
         printf("actionslider indicator pos: left!\n");
         break;
      case ELM_ACTIONSLIDER_CENTER:
         printf("actionslider indicator pos: center!\n");
         break;
      case ELM_ACTIONSLIDER_RIGHT:
         printf("actionslider indicator pos: right!\n");
         break;
      case ELM_ACTIONSLIDER_ALL:
         printf("actionslider indicator pos: all!\n");
         break;
     }

   mpos = elm_actionslider_magnet_pos_get(obj);

   switch (mpos)
     {
      case ELM_ACTIONSLIDER_NONE:
         printf("actionslider magnet pos: none!\n");
         break;
      case ELM_ACTIONSLIDER_LEFT:
         printf("actionslider magnet pos: left!\n");
         break;
      case ELM_ACTIONSLIDER_CENTER:
         printf("actionslider magnet pos: center!\n");
         break;
      case ELM_ACTIONSLIDER_RIGHT:
         printf("actionslider magnet pos: right!\n");
         break;
      case ELM_ACTIONSLIDER_ALL:
         printf("actionslider magnet pos: all!\n");
         break;
     }

   epos = elm_actionslider_enabled_pos_get(obj);

   if (epos)
     {
        printf("actionslider enabled pos: ");
        if (epos & ELM_ACTIONSLIDER_LEFT)
          printf("left ");
        if (epos & ELM_ACTIONSLIDER_CENTER)
          printf("center ");
        if (epos & ELM_ACTIONSLIDER_RIGHT)
          printf("right ");
        printf("\n");
     }
}

/**
 * @brief Callback function to change magnet position based on slider position.
 *
 * This function is triggered when the slider's position changes. It sets the
 * magnet position to either left or right, making the slider snap to the
 * respective side it was moved towards.
 *
 * @param data User data pointer (unused).
 * @param obj The actionslider object.
 * @param event_info The position which the slider has passed, as a string
 *                   (e.g., "left", "right").
 */
static void
_position_change_magnetic_cb(void *data EINA_UNUSED, Evas_Object * obj, void *event_info)
{
   if (!strcmp((char *)event_info, "left"))
     elm_actionslider_magnet_pos_set(obj, ELM_ACTIONSLIDER_LEFT);
   else if (!strcmp((char *)event_info, "right"))
     elm_actionslider_magnet_pos_set(obj, ELM_ACTIONSLIDER_RIGHT);
}

/**
 * @brief Callback function to enable or disable magnet positions.
 *
 * This function is called when the slider's position changes. It adjusts the
 * magnet position based on the slider's movement. Moving left sets the magnet
 * to the center, and moving right disables the magnet. This demonstrates
 * dynamic changes to the snapping behavior.
 *
 * @param data User data pointer (unused).
 * @param obj The actionslider object.
 * @param event_info The position which the slider has passed, as a string
 *                   (e.g., "left", "right").
 */
static void
_magnet_enable_disable_cb(void *data EINA_UNUSED, Evas_Object *obj, void *event_info)
{
   if (!strcmp((char *)event_info, "left"))
      elm_actionslider_magnet_pos_set(obj, ELM_ACTIONSLIDER_CENTER);
   else if (!strcmp((char *)event_info, "right"))
      elm_actionslider_magnet_pos_set(obj, ELM_ACTIONSLIDER_NONE);
}

/**
 * @brief Test function for the Efl Actionslider widget.
 *
 * This function creates a window and adds several actionslider widgets with
 * various configurations to demonstrate different styles and functionalities,
 * such as indicator positions, magnet positions, enabled positions, and callbacks.
 *
 * @param data User data pointer (unused).
 * @param obj Parent object (unused).
 * @param event_info Event info (unused).
 */
void
test_actionslider(void *data EINA_UNUSED, Evas_Object * obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *bx, *as;

   win = elm_win_util_standard_add("actionslider", "Actionslider");
   elm_win_autodel_set(win, EINA_TRUE);

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, 0);
   elm_win_resize_object_add(win, bx);
   evas_object_show(bx);

   as = elm_actionslider_add(win);
   evas_object_size_hint_weight_set(as, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_align_set(as, EVAS_HINT_FILL, 0);
   elm_actionslider_indicator_pos_set(as, ELM_ACTIONSLIDER_RIGHT);
   elm_actionslider_magnet_pos_set(as, ELM_ACTIONSLIDER_RIGHT);
   elm_object_part_text_set(as, "left", "Snooze");
   elm_object_part_text_set(as, "center", NULL);
   elm_object_part_text_set(as, "right", "Stop");
   elm_actionslider_enabled_pos_set(as, ELM_ACTIONSLIDER_LEFT |
                                    ELM_ACTIONSLIDER_RIGHT);
   evas_object_smart_callback_add(as, "pos_changed",
                                  _position_change_magnetic_cb, NULL);
   evas_object_smart_callback_add(as, "selected", _pos_selected_cb, NULL);
   evas_object_show(as);
   elm_box_pack_end(bx, as);

   as = elm_actionslider_add(win);
   evas_object_size_hint_weight_set(as, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_align_set(as, EVAS_HINT_FILL, 0);
   elm_actionslider_indicator_pos_set(as, ELM_ACTIONSLIDER_CENTER);
   elm_actionslider_magnet_pos_set(as, ELM_ACTIONSLIDER_CENTER);
   elm_object_part_text_set(as, "left", "Snooze");
   elm_object_part_text_set(as, "center", NULL);
   elm_object_part_text_set(as, "right", "Stop");
   elm_actionslider_enabled_pos_set(as, ELM_ACTIONSLIDER_LEFT |
                                    ELM_ACTIONSLIDER_RIGHT);
   evas_object_smart_callback_add(as, "selected", _pos_selected_cb, NULL);
   evas_object_show(as);
   elm_box_pack_end(bx, as);

   as = elm_actionslider_add(win);
   elm_object_style_set(as, "bar");
   evas_object_size_hint_weight_set(as, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_align_set(as, EVAS_HINT_FILL, 0);
   elm_actionslider_indicator_pos_set(as, ELM_ACTIONSLIDER_LEFT);
   elm_actionslider_magnet_pos_set(as, ELM_ACTIONSLIDER_CENTER|
                                   ELM_ACTIONSLIDER_RIGHT);
   elm_actionslider_enabled_pos_set(as, ELM_ACTIONSLIDER_CENTER |
                                    ELM_ACTIONSLIDER_RIGHT);
   elm_object_part_text_set(as, "left", NULL);
   elm_object_part_text_set(as, "center", "Accept");
   elm_object_part_text_set(as, "right", "Reject");
   evas_object_smart_callback_add(as, "selected", _pos_selected_cb, NULL);
   evas_object_show(as);
   elm_box_pack_end(bx, as);

   as = elm_actionslider_add(win);
   elm_object_style_set(as, "bar");
   evas_object_size_hint_weight_set(as, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_align_set(as, EVAS_HINT_FILL, 0);
   elm_actionslider_indicator_pos_set(as, ELM_ACTIONSLIDER_LEFT);
   elm_actionslider_magnet_pos_set(as, ELM_ACTIONSLIDER_LEFT);
   elm_object_part_text_set(as, "left", NULL);
   elm_object_part_text_set(as, "center", "Accept");
   elm_object_part_text_set(as, "right", "Reject");
   elm_object_text_set(as, "Go");
   evas_object_smart_callback_add(as, "pos_changed",
                                  _position_change_magnetic_cb, NULL);
   evas_object_smart_callback_add(as, "selected", _pos_selected_cb, NULL);
   evas_object_show(as);
   elm_box_pack_end(bx, as);

   as = elm_actionslider_add(win);
   elm_object_style_set(as, "bar");
   elm_object_disabled_set(as, EINA_TRUE);
   evas_object_size_hint_weight_set(as, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_align_set(as, EVAS_HINT_FILL, 0);
   elm_actionslider_indicator_pos_set(as, ELM_ACTIONSLIDER_LEFT);
   elm_actionslider_magnet_pos_set(as, ELM_ACTIONSLIDER_LEFT);
   elm_object_part_text_set(as, "left", NULL);
   elm_object_part_text_set(as, "center", "Accept");
   elm_object_part_text_set(as, "right", "Reject");
   elm_object_text_set(as, "Go");
   evas_object_smart_callback_add(as, "pos_changed",
                                  _position_change_magnetic_cb, NULL);
   evas_object_smart_callback_add(as, "selected", _pos_selected_cb, NULL);
   evas_object_show(as);
   elm_box_pack_end(bx, as);

   as = elm_actionslider_add(win);
   evas_object_size_hint_weight_set(as, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_align_set(as, EVAS_HINT_FILL, 0);
   elm_actionslider_indicator_pos_set(as, ELM_ACTIONSLIDER_LEFT);
   elm_actionslider_magnet_pos_set(as, ELM_ACTIONSLIDER_ALL);
   elm_object_part_text_set(as, "left", "Left");
   elm_object_part_text_set(as, "center", "Center");
   elm_object_part_text_set(as, "right", "Right");
   elm_object_text_set(as, "Go");
   evas_object_smart_callback_add(as, "selected", _pos_selected_cb, NULL);
   evas_object_show(as);
   elm_box_pack_end(bx, as);

   as = elm_actionslider_add(win);
   evas_object_size_hint_weight_set(as, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_align_set(as, EVAS_HINT_FILL, 0);
   elm_actionslider_indicator_pos_set(as, ELM_ACTIONSLIDER_CENTER);
   elm_actionslider_magnet_pos_set(as, ELM_ACTIONSLIDER_CENTER);
   elm_object_part_text_set(as, "left", "Enable");
   elm_object_part_text_set(as, "center", "Magnet");
   elm_object_part_text_set(as, "right", "Disable");
   evas_object_smart_callback_add(as, "pos_changed",
                                  _magnet_enable_disable_cb, NULL);
   evas_object_smart_callback_add(as, "selected", _pos_selected_cb, NULL);
   evas_object_show(as);
   elm_box_pack_end(bx, as);

   evas_object_resize(win, 320 * elm_config_scale_get(),
                           400 * elm_config_scale_get());
   evas_object_show(win);
}
