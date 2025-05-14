#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#include <Efl_Ui.h>
#include <Elementary.h>

/**
 * @brief Defines a mapping between image orientation enumerations and their
 *        human-readable names.
 *
 * This array is used to dynamically create radio buttons in the UI, allowing
 * the user to select and apply different orientations to the photocam image.
 * The array is terminated by an element with a NULL name.
 *
 * Each element in the array is a struct with the following fields:
 * - `orient`: An `Evas_Image_Orient` enum value representing a specific
 *             orientation (e.g., rotation, flip).
 * - `name`: A C-string that serves as the label for the UI element
 *           corresponding to the orientation.
 *
 * For example:
 * @code
 * { EVAS_IMAGE_ORIENT_90, "Rotate 90" }
 * @endcode
 * This entry maps the 90-degree rotation enum to the string "Rotate 90".
 */
static const struct {
 Evas_Image_Orient orient;
 const char *name;
} photocam_orient[] = {
 { EVAS_IMAGE_ORIENT_NONE, "None" },
 { EVAS_IMAGE_ORIENT_90, "Rotate 90" },
 { EVAS_IMAGE_ORIENT_180, "Rotate 180" },
 { EVAS_IMAGE_ORIENT_270, "Rotate 270" },
 { EVAS_IMAGE_FLIP_HORIZONTAL, "Horizontal Flip" },
 { EVAS_IMAGE_FLIP_VERTICAL, "Vertical Flip" },
 { EVAS_IMAGE_FLIP_TRANSPOSE, "Transpose" },
 { EVAS_IMAGE_FLIP_TRANSVERSE, "Transverse" },
 { 0, NULL }
};

/**
 * @brief Callback function for the "clicked" smart event of the photocam.
 * @param data Not used.
 * @param obj Not used.
 * @param event_info Not used.
 *
 * This function is invoked when the photocam is clicked. It simply prints
 * a message to standard output to indicate that the event was triggered.
 */
static void
my_ph_clicked(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   printf("clicked\n");
}

/**
 * @brief Callback function for the "changed" smart event of the orientation radio buttons.
 * @param data The photocam Evas_Object to modify.
 * @param obj The radio button that triggered the event.
 * @param event_info Not used.
 *
 * This function is called when the user selects a different orientation radio
 * button. It retrieves the orientation value associated with the selected
 * button and applies it to the photocam widget. It also prints the set and
 * actual orientation value to stderr for debugging purposes.
 */
static void
my_ph_ch(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
    Evas_Object *photocam = data;
    Evas_Image_Orient orient = elm_radio_value_get(obj);

    elm_photocam_image_orient_set(photocam, orient);
    fprintf(stderr, "Set %i and got %i\n",
            orient, elm_photocam_image_orient_get(photocam));
}

/**
 * @brief Callback for the "press" smart event on the photocam object.
 *
 * This is triggered when a mouse button is pressed on the photocam.
 */
static void
my_ph_press(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   printf("press\n");
}

/**
 * @brief Callback for the "longpressed" smart event on the photocam object.
 *
 * This is triggered when the mouse is held down for a certain duration.
 */
static void
my_ph_longpressed(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   printf("longpressed\n");
}

/**
 * @brief Callback for the "clicked,double" smart event on the photocam object.
 *
 * This is triggered on a double-click.
 */
static void
my_ph_clicked_double(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   printf("clicked,double\n");
}

/**
 * @brief Callback for the "load" smart event, indicating that the photocam
 * has started loading an image.
 */
static void
my_ph_load(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   printf("load\n");
}

/**
 * @brief Callback for the "loaded" smart event, indicating that the photocam
 * has finished loading the main image data.
 */
static void
my_ph_loaded(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   printf("loaded\n");
}

/**
 * @brief Callback for the "load,details" smart event. This is triggered when
 * the photocam begins to load detailed image data, like for high-resolution
 * views.
 */
static void
my_ph_load_details(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   printf("load,details\n");
}

/**
 * @brief Callback for the "loaded,details" smart event. This is triggered when
 * the photocam finishes loading detailed image data.
 */
static void
my_ph_loaded_details(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   printf("loaded,details\n");
}

/**
 * @brief Callback for the "zoom,start" smart event, indicating the beginning
 * of a zoom animation.
 */
static void
my_ph_zoom_start(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   printf("zoom,start\n");
}

/**
 * @brief Callback for the "zoom,stop" smart event, indicating the end of a
 * zoom animation.
 */
static void
my_ph_zoom_stop(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   printf("zoom,stop\n");
}

/**
 * @brief Callback for the "zoom,change" smart event, triggered whenever the
 * zoom level of the photocam changes.
 */
static void
my_ph_zoom_change(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   printf("zoom,change\n");
}

/**
 * @brief Callback for the "scroll,anim,start" smart event, indicating the
 * start of a scrolling animation.
 */
static void
my_ph_anim_start(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   printf("anim,start\n");
}

/**
 * @brief Callback for the "scroll,anim,stop" smart event, indicating the
 * end of a scrolling animation.
 */
static void
my_ph_anim_stop(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   printf("anim,stop\n");
}

/**
 * @brief Callback for the "scroll,drag,start" smart event, indicating the
 * user has started dragging the image.
 */
static void
my_ph_drag_start(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   printf("drag,start\n");
}

/**
 * @brief Callback for the "scroll,drag,stop" smart event, indicating the
 * user has stopped dragging the image.
 */
static void
my_ph_drag_stop(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   printf("drag_stop\n");
}

/**
 * @brief Callback for the "scroll" smart event.
 * @param data Not used.
 * @param obj The photocam object being scrolled.
 * @param event_info Not used.
 *
 * Triggered when the photocam is scrolled. It retrieves the current visible
 * image region and prints its coordinates and dimensions to stdout.
 */
static void
my_ph_scroll(void *data EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   int x, y, w, h;
   elm_photocam_image_region_get(obj, &x, &y, &w, &h);
   printf("scroll %i %i %ix%i\n", x, y, w, h);
}

/**
 * @brief Callback for when a file is chosen from the fileselector button.
 * @param data The photocam object.
 * @param obj Not used.
 * @param event_info The path to the selected file.
 *
 * This function sets the file for the photocam. It handles both standard
 * image files and Edje (.edj) files. For Edje files, it attempts to load
 * the first group in the file collection.
 */
static void
my_bt_open(void *data, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Evas_Object *ph = data;
   const char *file = event_info;

   if (file && !eina_str_has_extension(file, ".edj"))
     elm_photocam_file_set(ph, file);
   else if (file)
     {
        Eina_List *grps = edje_file_collection_list(file);

        if (eina_list_count(grps) > 0)
          {
             const char *grp = eina_list_nth(grps, 0);
             efl_file_simple_load(ph, file, grp);
             printf("Successfully set the edje file: %s, group: %s\n", file, grp);
          }
        else printf("Failed to set edje file\n");

        eina_list_free(grps);
     }
}

/**
 * @brief Callback for a button to show a specific image region.
 * @param data The photocam object.
 *
 * This function makes a predefined region (30,50 500x300) of the image
 * visible in the photocam view.
 */
static void
my_bt_show_reg(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   elm_photocam_image_region_show(data, 30, 50, 500, 300);
}

/**
 * @brief Callback for a button to bring a specific image region into view.
 * @param data The photocam object.
 *
 * This function animates the photocam to display the region defined by
 * (800,300 500x300).
 */
static void
my_bt_bring_reg(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)

{
   elm_photocam_image_region_bring_in(data, 800, 300, 500, 300);
}

/**
 * @brief Callback for the "zoom in" button.
 * @param data The photocam object.
 *
 * Decreases the zoom level of the photocam. The zoom factor change is
 * non-linear, allowing for finer control at lower zoom levels and faster
 * zooming out at higher levels. It sets the zoom mode to manual before
 * applying the new zoom level.
 */
static void
my_bt_zoom_in(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   double zoom;

   zoom = elm_photocam_zoom_get(data);
   if (zoom > 1.5)
     zoom -= 0.5;
   else if ((zoom > 1.0) && (zoom <= 1.5))
     zoom = 1.0;
   else if (EINA_DBL_EQ(zoom, 1.0))
     zoom = 0.8;
   else
     zoom = zoom * zoom;

   elm_photocam_zoom_mode_set(data, ELM_PHOTOCAM_ZOOM_MODE_MANUAL);
   if (zoom >= (1.0 / 32.0))
     {
        printf("zoom %f\n", zoom);
        elm_photocam_zoom_set(data, zoom);
     }
}

/**
 * @brief Callback for the "zoom out" button.
 * @param data The photocam object.
 *
 * Increases the zoom level of the photocam by a fixed step of 0.5.
 * It sets the zoom mode to manual before applying the new zoom level.
 * The zoom is capped at 256.0.
 */
static void
my_bt_zoom_out(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   double zoom;

   zoom = elm_photocam_zoom_get(data);
   zoom += 0.5;
   elm_photocam_zoom_mode_set(data, ELM_PHOTOCAM_ZOOM_MODE_MANUAL);
   if (zoom <= 256.0)
     {
        printf("zoom %f\n", zoom);
        elm_photocam_zoom_set(data, zoom);
     }
}

/**
 * @brief Toggles the paused state of the photocam's animations.
 * @param data The photocam object.
 *
 * This can be used to pause or resume any ongoing animations, like those
 * from scrolling or zooming.
 */
static void
my_bt_pause(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   elm_photocam_paused_set(data, !elm_photocam_paused_get(data));
}

/**
 * @brief Sets the photocam zoom mode to AUTO_FIT_IN.
 * @param data The photocam object.
 *
 * In this mode, the image is scaled to fit entirely within the viewport,
 * preserving its aspect ratio.
 */
static void
my_bt_zoom_fit_in(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   elm_photocam_zoom_mode_set(data, ELM_PHOTOCAM_ZOOM_MODE_AUTO_FIT_IN);
}

/**
 * @brief Sets the photocam zoom mode to AUTO_FIT.
 * @param data The photocam object.
 *
 * This mode scales the image to fit the viewport, but may not show the
 * entire image if aspect ratios differ. It fits to the smaller dimension.
 */
static void
my_bt_zoom_fit(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   elm_photocam_zoom_mode_set(data, ELM_PHOTOCAM_ZOOM_MODE_AUTO_FIT);
}

/**
 * @brief Sets the photocam zoom mode to AUTO_FILL.
 * @param data The photocam object.
 *
 * This mode scales the image to completely fill the viewport, preserving aspect
 * ratio. This may result in some parts of the image being cropped.
 */
static void
my_bt_zoom_fill(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   elm_photocam_zoom_mode_set(data, ELM_PHOTOCAM_ZOOM_MODE_AUTO_FILL);
}

/**
 * @brief Toggles gesture controls (like pinch-to-zoom) for the photocam.
 * @param data The photocam object.
 */
static void
my_bt_gesture(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
  elm_photocam_gesture_enabled_set(data, !elm_photocam_gesture_enabled_get(data));
}

/**
 * @brief Callback for the "download,progress" smart event on the photocam.
 * @param data Not used.
 * @param obj The photocam object.
 * @param event_info A pointer to an `Elm_Photocam_Progress` struct containing
 *                   progress information.
 *
 * This function updates a progress bar to reflect the download progress of a
 * remote image.
 */
static void
my_ph_download_progress(void *data EINA_UNUSED, Evas_Object *obj, void *event_info)
{
   Elm_Photocam_Progress *info = (Elm_Photocam_Progress *) event_info;
   Evas_Object *pb = evas_object_data_get(obj, "progressbar");

   if (info->total > 0.0)
     elm_progressbar_value_set(pb, info->now / info->total);
}

/**
 * @brief Callback for the "download,done" smart event on the photocam.
 * @param data Not used.
 * @param obj The photocam object.
 * @param event_info Not used.
 *
 * This function is called when the download of a remote image is complete. It
 * hides the progress bar associated with the download.
 */
static void
my_ph_download_done(void *data EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Evas_Object *pb = evas_object_data_get(obj, "progressbar");

   evas_object_hide(pb);
}

/**
 * @brief Callback for mouse wheel events to control photocam zoom.
 * @param data The photocam object.
 * @param e Not used.
 * @param obj Not used.
 * @param event_info The mouse wheel event information.
 *
 * This function handles zooming in and out of the photocam image using the
 * mouse wheel. It sets the zoom mode to manual and changes the zoom level
 * by a factor of 2 for each wheel step. The event is held to prevent it
 * from propagating further (e.g., to a parent scroller).
 */
static void
_photocam_mouse_wheel_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Evas_Object *photocam = data;
   Evas_Event_Mouse_Wheel *ev = (Evas_Event_Mouse_Wheel*) event_info;
   int zoom;
   double val;

   //unset the mouse wheel
   ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;

   zoom = elm_photocam_zoom_get(photocam);
   if ((ev->z>0) && (zoom == 1)) return;

   if (ev->z > 0)
     zoom /= 2;
   else
     zoom *= 2;

   val = 1;
   int _zoom = zoom;
   while (_zoom>1)
     {
        _zoom /= 2;
        val++;
     }

   elm_photocam_zoom_mode_set(photocam, ELM_PHOTOCAM_ZOOM_MODE_MANUAL);
   if (zoom >= 1) elm_photocam_zoom_set(photocam, zoom);
}

/**
 * @brief Callback for move and resize events on the photocam.
 * @param data An Evas_Object (a rectangle) that should be synced with the photocam.
 * @param e Not used.
 * @param obj The photocam object that was moved or resized.
 * @param event_info Not used.
 *
 * This function ensures that an overlay object (in this case, an invisible
 * rectangle used for capturing mouse wheel events) is always positioned and
 * sized identically to the photocam widget.
 */
static void
_photocam_move_resize_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   int x, y, w, h;

   evas_object_geometry_get(obj, &x, &y, &w, &h);
   evas_object_resize(data, w, h);
   evas_object_move(data, x, y);
}

/**
 * @brief Main function to set up and run the photocam test with a local image.
 *
 * This function creates a window, a photocam widget, and a set of buttons and
 * controls to test various features of the photocam, such as zooming,
 * panning, file loading, and orientation changes. An invisible rectangle is
 * layered on top of the photocam to intercept mouse wheel events for custom
 * zoom handling.
 */
void
test_photocam(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   char buf[PATH_MAX];
   Evas_Object *win, *ph, *tb2, *bt, *box, *rd, *rdg = NULL;
   int i;
   Evas_Object *rect = NULL;
   win = elm_win_util_standard_add("photocam", "PhotoCam");
   elm_win_autodel_set(win, EINA_TRUE);

   ph = elm_photocam_add(win);
   evas_object_size_hint_weight_set(ph, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   snprintf(buf, sizeof(buf), "%s/images/rock_01.jpg", elm_app_data_dir_get());
   elm_photocam_file_set(ph, buf);
   elm_win_resize_object_add(win, ph);

   // this rectangle hooks the event prior to scroller
   rect = evas_object_rectangle_add(evas_object_evas_get(win));
   evas_object_color_set(rect, 0, 0, 0, 0);
   evas_object_repeat_events_set(rect, EINA_TRUE);
   evas_object_show(rect);
   evas_object_event_callback_add(rect, EVAS_CALLBACK_MOUSE_WHEEL, _photocam_mouse_wheel_cb, ph);
   evas_object_raise(rect);

   // add move/resize callbacks to resize rect manually
   evas_object_event_callback_add(ph, EVAS_CALLBACK_RESIZE, _photocam_move_resize_cb, rect);
   evas_object_event_callback_add(ph, EVAS_CALLBACK_MOVE, _photocam_move_resize_cb, rect);

   evas_object_smart_callback_add(ph, "clicked", my_ph_clicked, win);
   evas_object_smart_callback_add(ph, "press", my_ph_press, win);
   evas_object_smart_callback_add(ph, "longpressed", my_ph_longpressed, win);
   evas_object_smart_callback_add(ph, "clicked,double", my_ph_clicked_double, win);
   evas_object_smart_callback_add(ph, "load", my_ph_load, win);
   evas_object_smart_callback_add(ph, "loaded", my_ph_loaded, win);
   evas_object_smart_callback_add(ph, "load,details", my_ph_load_details, win);
   evas_object_smart_callback_add(ph, "loaded,details", my_ph_loaded_details, win);
   evas_object_smart_callback_add(ph, "zoom,start", my_ph_zoom_start, win);
   evas_object_smart_callback_add(ph, "zoom,stop", my_ph_zoom_stop, win);
   evas_object_smart_callback_add(ph, "zoom,change", my_ph_zoom_change, win);
   evas_object_smart_callback_add(ph, "scroll,anim,start", my_ph_anim_start, win);
   evas_object_smart_callback_add(ph, "scroll,anim,stop", my_ph_anim_stop, win);
   evas_object_smart_callback_add(ph, "scroll,drag,start", my_ph_drag_start, win);
   evas_object_smart_callback_add(ph, "scroll,drag,stop", my_ph_drag_stop, win);
   evas_object_smart_callback_add(ph, "scroll", my_ph_scroll, win);

   evas_object_show(ph);

   tb2 = elm_table_add(win);
   evas_object_size_hint_weight_set(tb2, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, tb2);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Z -");
   evas_object_smart_callback_add(bt, "clicked", my_bt_zoom_out, ph);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(bt, 0.1, 0.1);
   elm_table_pack(tb2, bt, 0, 0, 1, 1);
   evas_object_show(bt);

   bt = elm_fileselector_button_add(win);
   elm_object_text_set(bt, "Select Photo");
   evas_object_smart_callback_add(bt, "file,chosen", my_bt_open, ph);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(bt, 0.5, 0.1);
   elm_table_pack(tb2, bt, 1, 0, 1, 1);
   evas_object_show(bt);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Z +");
   evas_object_smart_callback_add(bt, "clicked", my_bt_zoom_in, ph);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(bt, 0.9, 0.1);
   elm_table_pack(tb2, bt, 2, 0, 1, 1);
   evas_object_show(bt);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Show 30,50 500x300");
   evas_object_smart_callback_add(bt, "clicked", my_bt_show_reg, ph);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(bt, 0.1, 0.5);
   elm_table_pack(tb2, bt, 0, 1, 1, 1);
   evas_object_show(bt);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Bring 800,300 500x300");
   evas_object_smart_callback_add(bt, "clicked", my_bt_bring_reg, ph);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(bt, 0.9, 0.5);
   elm_table_pack(tb2, bt, 2, 1, 1, 1);
   evas_object_show(bt);

   box = elm_box_add(win);
   elm_box_horizontal_set(box, EINA_TRUE);
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(box, 0.0, 0.9);
   elm_table_pack(tb2, box, 0, 2, 1, 1);
   evas_object_show(box);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Pause On/Off");
   evas_object_smart_callback_add(bt, "clicked", my_bt_pause, ph);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_box_pack_end(box, bt);
   evas_object_show(bt);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Gesture On/Off");
   evas_object_smart_callback_add(bt, "clicked", my_bt_gesture, ph);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_box_pack_end(box, bt);
   evas_object_show(bt);

   box = elm_box_add(win);
   elm_box_horizontal_set(box, EINA_TRUE);
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(box, 0.9, 0.9);
   elm_table_pack(tb2, box, 2, 2, 1, 1);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Fit");
   evas_object_smart_callback_add(bt, "clicked", my_bt_zoom_fit, ph);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_box_pack_end(box, bt);
   evas_object_show(bt);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Fit_In");
   evas_object_smart_callback_add(bt, "clicked", my_bt_zoom_fit_in, ph);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_box_pack_end(box, bt);
   evas_object_show(bt);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Fill");
   evas_object_smart_callback_add(bt, "clicked", my_bt_zoom_fill, ph);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_box_pack_end(box, bt);
   evas_object_show(bt);

   box = elm_box_add(tb2);
   elm_box_horizontal_set(box, EINA_TRUE);
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(box, 0.9, 0.9);
   elm_table_pack(tb2, box, 1, 2, 1, 1);
   evas_object_show(box);

   for (i = 0; photocam_orient[i].name; ++i)
     {
        rd = elm_radio_add(win);
        evas_object_size_hint_weight_set(rd, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        elm_radio_state_value_set(rd, photocam_orient[i].orient);
        elm_object_text_set(rd, photocam_orient[i].name);
        elm_box_pack_end(box, rd);
        evas_object_show(rd);
        evas_object_smart_callback_add(rd, "changed", my_ph_ch, ph);
        if (!rdg)
          rdg = rd;
        else
          elm_radio_group_add(rd, rdg);
     }
   evas_object_show(box);
   evas_object_show(tb2);

   evas_object_resize(win, 800 * elm_config_scale_get(),
                           800 * elm_config_scale_get());
   evas_object_show(win);
}

/**
 * @brief Main function to test photocam with a remote image.
 *
 * Similar to test_photocam(), this function sets up a window and a photocam
 * widget. However, it initializes the photocam with a URL to a remote image.
 * It includes a progress bar to show the download status of the image.
 */
void
test_photocam_remote(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *ph, *tb2, *bt, *box, *pb, *rd;
   Evas_Object *rdg = NULL;
   Evas_Object *rect = NULL;
   // these were just testing - use the "select photo" browser to select one
   static const char *url = "http://eoimages.gsfc.nasa.gov/images/imagerecords/73000/73751/world.topo.bathy.200407.3x21600x10800.jpg";
   int i;

   win = elm_win_util_standard_add("photocam", "PhotoCam");
   elm_win_autodel_set(win, EINA_TRUE);

   ph = elm_photocam_add(win);
   evas_object_size_hint_weight_set(ph, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, ph);

   // this rectangle hooks the event prior to scroller
   rect = evas_object_rectangle_add(evas_object_evas_get(win));
   evas_object_color_set(rect, 0, 0, 0, 0);
   evas_object_repeat_events_set(rect, EINA_TRUE);
   evas_object_show(rect);
   evas_object_event_callback_add(rect, EVAS_CALLBACK_MOUSE_WHEEL, _photocam_mouse_wheel_cb, ph);
   evas_object_raise(rect);

   // add move/resize callbacks to resize rect manually
   evas_object_event_callback_add(ph, EVAS_CALLBACK_RESIZE, _photocam_move_resize_cb, rect);
   evas_object_event_callback_add(ph, EVAS_CALLBACK_MOVE, _photocam_move_resize_cb, rect);

   evas_object_smart_callback_add(ph, "clicked", my_ph_clicked, win);
   evas_object_smart_callback_add(ph, "press", my_ph_press, win);
   evas_object_smart_callback_add(ph, "longpressed", my_ph_longpressed, win);
   evas_object_smart_callback_add(ph, "clicked,double", my_ph_clicked_double, win);
   evas_object_smart_callback_add(ph, "load", my_ph_load, win);
   evas_object_smart_callback_add(ph, "loaded", my_ph_loaded, win);
   evas_object_smart_callback_add(ph, "load,details", my_ph_load_details, win);
   evas_object_smart_callback_add(ph, "loaded,details", my_ph_loaded_details, win);
   evas_object_smart_callback_add(ph, "zoom,start", my_ph_zoom_start, win);
   evas_object_smart_callback_add(ph, "zoom,stop", my_ph_zoom_stop, win);
   evas_object_smart_callback_add(ph, "zoom,change", my_ph_zoom_change, win);
   evas_object_smart_callback_add(ph, "scroll,anim,start", my_ph_anim_start, win);
   evas_object_smart_callback_add(ph, "scroll,anim,stop", my_ph_anim_stop, win);
   evas_object_smart_callback_add(ph, "scroll,drag,start", my_ph_drag_start, win);
   evas_object_smart_callback_add(ph, "scroll,drag,stop", my_ph_drag_stop, win);
   evas_object_smart_callback_add(ph, "scroll", my_ph_scroll, win);
   evas_object_smart_callback_add(ph, "download,progress", my_ph_download_progress, win);
   evas_object_smart_callback_add(ph, "download,done", my_ph_download_done, win);

   elm_photocam_file_set(ph, url);
   evas_object_show(ph);

   tb2 = elm_table_add(win);
   evas_object_size_hint_weight_set(tb2, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, tb2);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Z -");
   evas_object_smart_callback_add(bt, "clicked", my_bt_zoom_out, ph);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(bt, 0.1, 0.1);
   elm_table_pack(tb2, bt, 0, 0, 1, 1);
   evas_object_show(bt);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Z +");
   evas_object_smart_callback_add(bt, "clicked", my_bt_zoom_in, ph);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(bt, 0.9, 0.1);
   elm_table_pack(tb2, bt, 2, 0, 1, 1);
   evas_object_show(bt);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Show 30,50 500x300");
   evas_object_smart_callback_add(bt, "clicked", my_bt_show_reg, ph);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(bt, 0.1, 0.5);
   elm_table_pack(tb2, bt, 0, 1, 1, 1);
   evas_object_show(bt);

   pb = elm_progressbar_add(win);
   elm_progressbar_unit_format_set(pb, "Loading %.2f %%");
   evas_object_size_hint_weight_set(pb, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(pb, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_table_pack(tb2, pb, 1, 1, 1, 1);
   evas_object_show(pb);
   evas_object_data_set(ph, "progressbar", pb);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Bring 800,300 500x300");
   evas_object_smart_callback_add(bt, "clicked", my_bt_bring_reg, ph);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(bt, 0.9, 0.5);
   elm_table_pack(tb2, bt, 2, 1, 1, 1);
   evas_object_show(bt);

   box = elm_box_add(win);
   elm_box_horizontal_set(box, EINA_TRUE);
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(box, 0.0, 0.9);
   elm_table_pack(tb2, box, 0, 2, 1, 1);
   evas_object_show(box);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Pause On/Off");
   evas_object_smart_callback_add(bt, "clicked", my_bt_pause, ph);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_box_pack_end(box, bt);
   evas_object_show(bt);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Gesture On/Off");
   evas_object_smart_callback_add(bt, "clicked", my_bt_gesture, ph);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_box_pack_end(box, bt);
   evas_object_show(bt);

   box = elm_box_add(win);
   elm_box_horizontal_set(box, EINA_TRUE);
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(box, 0.9, 0.9);
   elm_table_pack(tb2, box, 2, 2, 1, 1);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Fit");
   evas_object_smart_callback_add(bt, "clicked", my_bt_zoom_fit, ph);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_box_pack_end(box, bt);
   evas_object_show(bt);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Fit_In");
   evas_object_smart_callback_add(bt, "clicked", my_bt_zoom_fit_in, ph);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_box_pack_end(box, bt);
   evas_object_show(bt);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Fill");
   evas_object_smart_callback_add(bt, "clicked", my_bt_zoom_fill, ph);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_box_pack_end(box, bt);
   evas_object_show(bt);

   box = elm_box_add(tb2);
   elm_box_horizontal_set(box, EINA_TRUE);
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(box, 0.9, 0.9);
   elm_table_pack(tb2, box, 1, 2, 1, 1);
   evas_object_show(box);

   for (i = 0; photocam_orient[i].name; ++i)
     {
        rd = elm_radio_add(win);
        evas_object_size_hint_weight_set(rd, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        elm_radio_state_value_set(rd, photocam_orient[i].orient);
        elm_object_text_set(rd, photocam_orient[i].name);
        elm_box_pack_end(box, rd);
        evas_object_show(rd);
        evas_object_smart_callback_add(rd, "changed", my_ph_ch, ph);
        if (!rdg)
          rdg = rd;
        else
          elm_radio_group_add(rd, rdg);
     }
   evas_object_show(box);
   evas_object_show(tb2);

   evas_object_resize(win, 800 * elm_config_scale_get(),
                           800 * elm_config_scale_get());
   evas_object_show(win);
}

/**
 * @brief Maps icon names to arbitrary values for use with radio buttons.
 *
 * This array provides a set of standard icon names that can be selected in the
 * UI to be displayed on the photocam widget when it's not showing a photo.
 * The `val` field is a unique identifier for each radio button.
 * The list is terminated by an entry with a NULL name.
 *
 * Each element in the array is a struct with the following fields:
 * - `val`: An `unsigned char` used as the state value for a radio button.
 * - `name`: A C-string representing a freedesktop.org standard icon name.
 *
 * For example:
 * @code
 * {0, "home"}
 * @endcode
 * This entry maps the value 0 to the "home" icon.
 */
static const struct {
     unsigned char val;
     const char *name;
} photocam_icons[] = {
       {0, "home"},
       {1, "folder"},
       {2, "network-server"},
       {3, "folder-music"},
       {4, "user-trash"},
       {5, "start-here"},
       {6, "folder-download"},
       {7, "emblem-system"},
       {8, "emblem-mail"},
       {9, "None"},
       {0, NULL}
};

/**
 * @brief Callback for when a radio button in the icon selection group is changed.
 * @param data The photocam object.
 * @param obj The radio button that was changed.
 * @param event_info Not used.
 *
 * This function retrieves the value of the selected radio button, which
 * corresponds to an index in the `photocam_icons` array. It then sets the
 * icon of the photocam to the icon name at that index.
 */
static void
_radio_changed_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   unsigned char index = elm_radio_value_get(obj);
   efl_ui_image_icon_set(data, photocam_icons[index].name);
   printf("icon is %s\n", efl_ui_image_icon_get(data));
}

/**
 * @brief Test function for displaying icons on the photocam widget.
 *
 * This test sets up a photocam widget and a group of radio buttons. Each radio
 * button corresponds to an icon from the `photocam_icons` array. Selecting a
 * radio button changes the icon displayed by the photocam. This demonstrates
 * the photocam's ability to display themed icons instead of a photographic image.
 */
void
test_photocam_icon(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *ph, *tb2, *bt, *bx, *rd;
   Evas_Object *rdg = NULL;
   Evas_Object *rect = NULL;
   int i;

   win = elm_win_util_standard_add("photocam", "PhotoCam");
   elm_win_autodel_set(win, EINA_TRUE);

   ph = elm_photocam_add(win);
   evas_object_size_hint_weight_set(ph, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   efl_ui_image_icon_set(ph, "home");
   elm_win_resize_object_add(win, ph);

   rect = evas_object_rectangle_add(evas_object_evas_get(win));
   evas_object_color_set(rect, 0, 0, 0, 0);
   evas_object_repeat_events_set(rect, EINA_TRUE);
   evas_object_show(rect);
   evas_object_event_callback_add(rect, EVAS_CALLBACK_MOUSE_WHEEL, _photocam_mouse_wheel_cb, ph);
   evas_object_raise(rect);

   evas_object_event_callback_add(ph, EVAS_CALLBACK_RESIZE, _photocam_move_resize_cb, rect);
   evas_object_event_callback_add(ph, EVAS_CALLBACK_MOVE, _photocam_move_resize_cb, rect);

   evas_object_smart_callback_add(ph, "clicked", my_ph_clicked, win);
   evas_object_smart_callback_add(ph, "press", my_ph_press, win);
   evas_object_smart_callback_add(ph, "longpressed", my_ph_longpressed, win);
   evas_object_smart_callback_add(ph, "clicked,double", my_ph_clicked_double, win);
   evas_object_smart_callback_add(ph, "load", my_ph_load, win);
   evas_object_smart_callback_add(ph, "loaded", my_ph_loaded, win);
   evas_object_smart_callback_add(ph, "load,details", my_ph_load_details, win);
   evas_object_smart_callback_add(ph, "loaded,details", my_ph_loaded_details, win);
   evas_object_smart_callback_add(ph, "zoom,start", my_ph_zoom_start, win);
   evas_object_smart_callback_add(ph, "zoom,stop", my_ph_zoom_stop, win);
   evas_object_smart_callback_add(ph, "zoom,change", my_ph_zoom_change, win);
   evas_object_smart_callback_add(ph, "scroll,anim,start", my_ph_anim_start, win);
   evas_object_smart_callback_add(ph, "scroll,anim,stop", my_ph_anim_stop, win);
   evas_object_smart_callback_add(ph, "scroll,drag,start", my_ph_drag_start, win);
   evas_object_smart_callback_add(ph, "scroll,drag,stop", my_ph_drag_stop, win);
   evas_object_smart_callback_add(ph, "scroll", my_ph_scroll, win);

   evas_object_show(ph);

   tb2 = elm_table_add(win);
   evas_object_size_hint_weight_set(tb2, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, tb2);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Z -");
   evas_object_smart_callback_add(bt, "clicked", my_bt_zoom_out, ph);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(bt, 0.1, 0.1);
   elm_table_pack(tb2, bt, 0, 0, 1, 1);
   evas_object_show(bt);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Z +");
   evas_object_smart_callback_add(bt, "clicked", my_bt_zoom_in, ph);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(bt, 0.9, 0.1);
   elm_table_pack(tb2, bt, 2, 0, 1, 1);
   evas_object_show(bt);

   evas_object_show(tb2);

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_box_horizontal_set(bx, EINA_TRUE);
   elm_table_pack(tb2, bx, 1, 2, 1, 1);
   evas_object_show(bx);

   for (i = 0; photocam_icons[i].name; ++i)
     {
        rd = elm_radio_add(win);
        evas_object_size_hint_weight_set(rd, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        elm_object_text_set(rd, photocam_icons[i].name);
        efl_ui_radio_state_value_set(rd, photocam_icons[i].val);
        elm_box_pack_end(bx, rd);
        evas_object_smart_callback_add(rd, "changed", _radio_changed_cb, ph);
        if(!rdg)
          rdg = rd;
        else
          elm_radio_group_add(rd, rdg);
        evas_object_show(rd);
     }

   evas_object_resize(win, 150 * elm_config_scale_get(),
                           150 * elm_config_scale_get());
   evas_object_show(win);
}

/**
 * @brief Callback for click events on the zoomable image.
 * @param data Not used.
 * @param ev The event information.
 *
 * This function is triggered when the zoomable image is clicked. It toggles
 * the paused state of the image's animation if it is playable.
 */
static void
_zoomable_clicked_cb(void *data EINA_UNUSED, const Efl_Event *ev)
{
   Eina_Bool paused;

   paused = efl_player_paused_get(ev->object);
   printf("image clicked! paused = %d\n", paused);
   efl_player_paused_set(ev->object, !paused);
}

/**
 * @brief Callback for move and resize events on the zoomable image.
 * @param data An Evas_Object (a rectangle) to sync with the zoomable image.
 * @param ev The event information.
 *
 * This function ensures that an overlay object (an invisible rectangle for
 * capturing mouse wheel events) matches the position and size of the zoomable
 * image widget.
 */
static void
_zoomable_move_resize_cb(void *data, const Efl_Event *ev)
{
   Eina_Rect r;

   r = efl_gfx_entity_geometry_get(ev->object);
   efl_gfx_entity_size_set(data, EINA_SIZE2D(r.w,  r.h));
   efl_gfx_entity_position_set(data, r.pos);
}

/**
 * @brief Callback for mouse wheel events to control zoomable image zoom level.
 * @param data The zoomable image object.
 * @param e The event information.
 *
 * This function handles zooming in and out of the zoomable image using the
 * mouse wheel. It sets the zoom mode to manual and changes the zoom level by a
 * factor of 2 for each wheel step.
 */
static void
_zoomable_mouse_wheel_cb(void *data, const Efl_Event *e)
{
   Eo *zoomable = data;
   Efl_Input_Pointer *ev = e->info;
   int zoom, _zoom, delta, val;

   zoom = efl_ui_zoom_level_get(zoomable);
   delta = efl_input_pointer_wheel_delta_get(ev);
   if ((delta > 0) && (zoom == 1)) return;

   if (delta > 0)
     zoom /= 2;
   else
     zoom *= 2;

   val = 1;
   _zoom = zoom;
   while (_zoom>1)
     {
        _zoom /= 2;
        val++;
     }

   efl_ui_zoom_mode_set(zoomable, EFL_UI_ZOOM_MODE_MANUAL);
   if (zoom >= 1) efl_ui_zoom_level_set(zoomable, zoom);
}

/**
 * @brief Test function for an animated, zoomable image.
 *
 * This function sets up a window with an `Efl_Ui_Image_Zoomable` widget that
 * displays an animated GIF. It demonstrates how to handle animations (play,
 * pause on click) and zooming for such a widget. It also uses an overlay
 * rectangle to capture mouse wheel events for zooming, similar to the
 * other photocam tests.
 */
void
test_image_zoomable_animated(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Eo *win, *bx, *zoomable, *rect;
   char buf[PATH_MAX];

   win = efl_add(EFL_UI_WIN_CLASS, efl_main_loop_get(),
                                  efl_text_set(efl_added, "Efl.Ui.Image_Zoomable animation"),
                 efl_ui_win_autodel_set(efl_added, EINA_TRUE));

   bx = efl_add(EFL_UI_BOX_CLASS, win,
                efl_ui_layout_orientation_set(efl_added, EFL_UI_LAYOUT_ORIENTATION_VERTICAL),
                efl_content_set(win, efl_added));

   efl_add(EFL_UI_TEXTBOX_CLASS, bx,
           efl_text_set(efl_added, "Clicking the image will play/pause animation."),
           efl_text_interactive_editable_set(efl_added, EINA_FALSE),
           efl_gfx_hint_weight_set(efl_added, 1, 0),
           efl_text_font_family_set(efl_added, "Sans"),
           efl_text_font_size_set(efl_added, 10),
           efl_text_color_set(efl_added, 255, 255, 255, 255),
           efl_pack(bx, efl_added)
          );

   snprintf(buf, sizeof(buf), "%s/images/animated_logo.gif", elm_app_data_dir_get());
   zoomable = efl_add(EFL_UI_IMAGE_ZOOMABLE_CLASS, win,
                      efl_file_set(efl_added, buf),
                      efl_file_load(efl_added),
                      efl_pack(bx, efl_added),
                      efl_event_callback_add(efl_added, EFL_INPUT_EVENT_CLICKED, _zoomable_clicked_cb, NULL)
                     );

   if (efl_playable_get(zoomable))
     {
        printf("animation is available for this image.\n");
        efl_player_autoplay_set(zoomable, EINA_TRUE);
        efl_player_playback_loop_set(zoomable, EINA_TRUE);
     }

   rect = efl_add(EFL_CANVAS_RECTANGLE_CLASS, win,
                  efl_gfx_color_set(efl_added, 0, 0, 0, 0),
                  efl_gfx_stack_raise_to_top(efl_added),
                  efl_canvas_object_repeat_events_set(efl_added, EINA_TRUE),
                  efl_event_callback_add(efl_added, EFL_EVENT_POINTER_WHEEL, _zoomable_mouse_wheel_cb, zoomable)
                 );

   // add move/resize callbacks to resize rect manually
   efl_event_callback_add(zoomable, EFL_GFX_ENTITY_EVENT_SIZE_CHANGED, _zoomable_move_resize_cb, rect);
   efl_event_callback_add(zoomable, EFL_GFX_ENTITY_EVENT_POSITION_CHANGED, _zoomable_move_resize_cb, rect);

   efl_gfx_entity_size_set(win, EINA_SIZE2D(300,  320));
}
