#include "elementary_config.h"
#include <Elementary.h>

static Evas_Object *slideshow, *bt_start, *bt_stop;
static Elm_Slideshow_Item_Class itc;

/**
 * @brief Callback to show a notification widget.
 * @param data The notification widget to show.
 * @param e Not used.
 * @param obj Not used.
 * @param event_info Not used.
 */
static void
_notify_show(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   evas_object_show(data);
}

/**
 * @brief Callback to advance to the next slide.
 * @param data The slideshow widget.
 * @param obj Not used.
 * @param event_info Not used.
 */
static void
_next(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   elm_slideshow_next(data);
}

/**
 * @brief Callback to go to the previous slide.
 * @param data The slideshow widget.
 * @param obj Not used.
 * @param event_info Not used.
 */
static void
_previous(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   elm_slideshow_previous(data);
}

/**
 * @brief Callback for mouse entering the controls area.
 *
 * This function makes the notification containing the controls visible and
 * sets its timeout to 0, so it remains visible as long as the mouse is over it.
 * @param data The notification widget.
 * @param e Not used.
 * @param obj Not used.
 * @param event_info Not used.
 */
static void
_mouse_in(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   elm_notify_timeout_set(data, 0.0);
   evas_object_show(data);
}

/**
 * @brief Callback for mouse leaving the controls area.
 *
 * This function sets the timeout for the notification containing the controls,
 * so it will hide after the specified time.
 * @param data The notification widget.
 * @param e Not used.
 * @param obj Not used.
 * @param event_info Not used.
 */
static void
_mouse_out(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   elm_notify_timeout_set(data, 3.0);
}

/**
 * @brief Callback for selecting a transition effect.
 *
 * This is called when an item from the transition hoversel is selected. It applies
 * the selected transition to the slideshow.
 * @param data The name of the selected transition (e.g., "fade").
 * @param obj The hoversel widget.
 * @param event_info The selected hoversel item.
 */
static void
_hv_select(void *data, Evas_Object *obj, void *event_info)
{
   elm_slideshow_transition_set(slideshow, data);
   elm_object_text_set(obj, elm_object_item_text_get(event_info));
}

/**
 * @brief Callback for selecting a slide layout.
 *
 * This is called when an item from the layout hoversel is selected. It applies
 * the selected layout to the slideshow.
 * @param data The name of the selected layout (e.g., "fullscreen").
 * @param obj The hoversel widget.
 * @param event_info Not used.
 */
static void
_layout_select(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   elm_slideshow_layout_set(slideshow, data);
   elm_object_text_set(obj, data);
}

/**
 * @brief Callback to start the automatic slideshow.
 *
 * It retrieves the timeout value from the spinner and sets it on the slideshow,
 * starting the automatic transitions. It also disables the start button and
 * enables the stop button.
 * @param data The spinner widget from which to get the timeout value.
 * @param obj Not used.
 * @param event_info Not used.
 */
static void
_start(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   elm_slideshow_timeout_set(slideshow, elm_spinner_value_get(data));

   elm_object_disabled_set(bt_start, EINA_TRUE);
   elm_object_disabled_set(bt_stop, EINA_FALSE);
}

/**
 * @brief Callback to stop the automatic slideshow.
 *
 * It sets the slideshow timeout to 0.0 to stop automatic transitions. It also
 * enables the start button and disables the stop button.
 * @param data Not used.
 * @param obj Not used.
 * @param event_info Not used.
 */
static void
_stop(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   elm_slideshow_timeout_set(slideshow, 0.0);
   elm_object_disabled_set(bt_start, EINA_FALSE);
   elm_object_disabled_set(bt_stop, EINA_TRUE);
}

/**
 * @brief Callback for when the spinner value changes.
 *
 * If the slideshow is currently running (timeout > 0), this function updates
 * the slideshow's transition timeout with the new value from the spinner.
 * @param data The spinner widget.
 * @param obj Not used.
 * @param event_info Not used.
 */
static void
_spin(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   if (elm_slideshow_timeout_get(slideshow) > 0)
     elm_slideshow_timeout_set(slideshow, elm_spinner_value_get(data));
}

/**
 * @brief Item provider function to get a slide's content object.
 *
 * This function is part of the Elm_Slideshow_Item_Class interface. It's called
 * by the slideshow widget to create the actual Evas_Object for a slide.
 * In this implementation, it creates an elm_image widget and loads the image
 * from the file path provided in `data`.
 *
 * @param data The data associated with the item (here, a path to an image file).
 * @param obj The parent slideshow widget.
 * @return The created Evas_Object for the slide.
 */
static Evas_Object *
_get(void *data, Evas_Object *obj)
{
   //Evas_Object *photo = elm_photocam_add(obj);
   //elm_photocam_file_set(photo, data);
   //elm_photocam_zoom_mode_set(photo, ELM_PHOTOCAM_ZOOM_MODE_AUTO_FIT);

   printf("_get (item data: '%s')\n", (char*)data);
   Evas_Object *photo = elm_image_add(obj);
   elm_image_file_set(photo, data, NULL);
   elm_image_fill_outside_set(photo, EINA_FALSE);

   return photo;
}

/**
 * @brief Item provider function to delete a slide's content object.
 *
 * This function is part of the Elm_Slideshow_Item_Class interface. It's called
 * by the slideshow widget when an item is being deleted. This is where an
 * application would free resources associated with the slide's content object.
 *
 * @param data The data associated with the item.
 * @param obj The slide's content Evas_Object that is being deleted. Not used here.
 */
static void
_del(void *data, Evas_Object *obj EINA_UNUSED)
{
   printf("_del (item data: '%s')\n", (char*)data);
}


/**
 * @brief Callback for the "changed" smart event of the slideshow.
 *
 * This function is executed whenever the currently displayed slide changes.
 * It prints the data of the new slide item.
 *
 * @param data Not used.
 * @param obj Not used.
 * @param event_info The `Elm_Object_Item` for the new current slide.
 */
static void
_changed_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Elm_Object_Item *slide_it = (Elm_Object_Item *) event_info;
   printf("CHANGED (item data: '%s')\n",
          (char*)elm_object_item_data_get(slide_it));
}

/**
 * @brief Callback for the "transition,end" smart event of the slideshow.
 *
 * This function is executed when a slide transition animation has finished. It
 * checks if the slideshow has reached its last slide by comparing the item
 * whose transition just ended (`event_info`) with the last item (`data`).
 *
 * @param data The last slide item, passed during callback registration.
 * @param obj Not used.
 * @param event_info The `Elm_Object_Item` of the slide whose transition ended.
 */
static void
_transition_end_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Elm_Object_Item *slide_it = (Elm_Object_Item *) event_info;
   printf("TRANSITION,END (item data: '%s')\n",
          (char*)elm_object_item_data_get(slide_it));
   if (data == slide_it)
     printf("Reaches to End of slides\n");
}

/**
 * @brief Main function to set up and run the slideshow test.
 *
 * This function creates the main window and all the UI components for the
 * slideshow test application, including the slideshow widget itself, navigation
 * buttons, and controls for transitions, layouts, and timing. It populates
 * the slideshow with a list of images.
 *
 * @param data Not used.
 * @param obj Not used.
 * @param event_info Not used.
 */
void
test_slideshow(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *notify, *bx, *bt, *hv, *spin;
   const Eina_List *l;
   const char *transition, *layout;
   Elm_Object_Item *slide_last_it = NULL;
   unsigned long i;

   /*
    * A NULL-terminated array of image file names. These are relative to
    * the application's data directory. For example:
    * { "image1.png", "image2.jpg", NULL }
    */
   const char *imgs[] = {
     "logo.png",
     "rock_01.jpg",
     "rock_02.jpg",
     "sky_01.jpg",
     "sky_04.jpg",
     "wood_01.jpg",
     "mystrale.jpg",
     "mystrale_2.jpg",
     NULL
   };


   win = elm_win_util_standard_add("slideshow", "Slideshow");
   elm_win_autodel_set(win, EINA_TRUE);

   slideshow = elm_slideshow_add(win);
   elm_slideshow_loop_set(slideshow, EINA_TRUE);
   evas_object_size_hint_weight_set(slideshow, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_smart_callback_add(slideshow, "changed", _changed_cb, NULL);
   elm_win_resize_object_add(win, slideshow);
   evas_object_show(slideshow);

   itc.func.get = _get;
   itc.func.del = _del;

   for (i = 0; imgs[i]; i++)
     {
        const char *img = eina_stringshare_printf("%s/images/%s", elm_app_data_dir_get(), imgs[i]);
        slide_last_it = elm_slideshow_item_add(slideshow, &itc, img);
     }

   evas_object_smart_callback_add(slideshow, "transition,end",
                                  _transition_end_cb, slide_last_it);

   notify = elm_notify_add(win);
   elm_notify_align_set(notify, 0.5, 1.0);
   evas_object_size_hint_weight_set(notify, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, notify);
   elm_notify_timeout_set(notify, 3.0);

   bx = elm_box_add(win);
   elm_box_horizontal_set(bx, EINA_TRUE);
   elm_object_content_set(notify, bx);
   evas_object_show(bx);

   evas_object_event_callback_add(bx, EVAS_CALLBACK_MOUSE_IN, _mouse_in,
                                  notify);
   evas_object_event_callback_add(bx, EVAS_CALLBACK_MOUSE_OUT, _mouse_out,
                                  notify);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Previous");
   evas_object_smart_callback_add(bt, "clicked", _previous, slideshow);
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Next");
   evas_object_smart_callback_add(bt, "clicked", _next, slideshow);
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);

   hv = elm_hoversel_add(win);
   elm_box_pack_end(bx, hv);
   elm_hoversel_hover_parent_set(hv, win);
   EINA_LIST_FOREACH(elm_slideshow_transitions_get(slideshow), l, transition)
      elm_hoversel_item_add(hv, transition, NULL, 0, _hv_select, transition);
   elm_hoversel_item_add(hv, "None", NULL, 0, _hv_select, NULL);
   elm_object_text_set(hv, eina_list_data_get(elm_slideshow_transitions_get(slideshow)));
   evas_object_show(hv);

   hv = elm_hoversel_add(win);
   elm_box_pack_end(bx, hv);
   elm_hoversel_hover_parent_set(hv, win);
   EINA_LIST_FOREACH(elm_slideshow_layouts_get(slideshow), l, layout)
       elm_hoversel_item_add(hv, layout,  NULL, 0, _layout_select, layout);
   elm_object_text_set(hv, elm_slideshow_layout_get(slideshow));
   evas_object_show(hv);

   spin = elm_spinner_add(win);
   elm_spinner_label_format_set(spin, "%2.0f secs.");
   evas_object_smart_callback_add(spin, "changed", _spin, spin);
   elm_spinner_step_set(spin, 1);
   elm_spinner_min_max_set(spin, 1, 30);
   elm_spinner_value_set(spin, 3);
   elm_box_pack_end(bx, spin);
   evas_object_show(spin);

   bt = elm_button_add(win);
   bt_start = bt;
   elm_object_text_set(bt, "Start");
   evas_object_smart_callback_add(bt, "clicked", _start, spin);
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);

   bt = elm_button_add(win);
   bt_stop = bt;
   elm_object_text_set(bt, "Stop");
   evas_object_smart_callback_add(bt, "clicked", _stop, spin);
   elm_box_pack_end(bx, bt);
   elm_object_disabled_set(bt, EINA_TRUE);
   evas_object_show(bt);

   evas_object_event_callback_add(slideshow, EVAS_CALLBACK_MOUSE_UP,
                                  _notify_show, notify);
   evas_object_event_callback_add(slideshow, EVAS_CALLBACK_MOUSE_MOVE,
                                  _notify_show, notify);

   evas_object_resize(win, 500 * elm_config_scale_get(),
                           400 * elm_config_scale_get());
   evas_object_show(win);
}
