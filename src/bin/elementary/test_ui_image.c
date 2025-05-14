#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#include <Elementary.h>
#include <Efl_Ui.h>

/**
 * @brief Maps image orientation enums to human-readable names.
 * This array is used to populate UI elements like radio buttons
 * that control image orientation.
 * Each element is a struct with:
 *  - orient: The Efl_Gfx_Image_Orientation enum value.
 *  - name: The string representation for the UI.
 * The list is terminated by an element with a NULL name.
 */
static const struct {
   Efl_Gfx_Image_Orientation orient;
   const char *name;
} images_orient[] = {
  { EFL_GFX_IMAGE_ORIENTATION_NONE, "None" },
  { EFL_GFX_IMAGE_ORIENTATION_RIGHT, "Rotate 90" },
  { EFL_GFX_IMAGE_ORIENTATION_DOWN, "Rotate 180" },
  { EFL_GFX_IMAGE_ORIENTATION_LEFT, "Rotate 270" },
  { EFL_GFX_IMAGE_ORIENTATION_FLIP_HORIZONTAL, "Horizontal Flip" },
  { EFL_GFX_IMAGE_ORIENTATION_FLIP_VERTICAL, "Vertical Flip" },
  { 0, NULL }
};

/**
 * @brief Creates and configures a new EFL UI window.
 *
 * This is a helper function to reduce boilerplate code when creating
 * a new window for a test case.
 *
 * @param name The internal name of the window.
 * @param title The visible title of the window.
 * @return A new Eo window object.
 */
static Eo *
win_add(const char *name, const char *title)
{
   return efl_add(EFL_UI_WIN_CLASS, efl_main_loop_get(),
     efl_ui_win_name_set(efl_added, name),
     efl_ui_win_autodel_set(efl_added, EINA_TRUE),
     efl_text_set(efl_added, title));
}

/**
 * @brief Creates a new image object and adds it to a window.
 *
 * This helper function creates an Efl_Ui_Image, sets it to expand and fill,
 * loads an image file if provided, and stores the image object in the
 * window's data with the key "im".
 *
 * @param win The parent window for the image.
 * @param file The path to the image file relative to the app's data directory.
 *        If NULL, no file is loaded initially. Example: "/images/logo.png".
 * @return The new Eo image object.
 */
static Eo *
img_add(Eo *win, const char *file)
{
   char buf[PATH_MAX];
   Eo *im;

   im = efl_add(EFL_UI_IMAGE_CLASS, win,
     efl_gfx_hint_weight_set(efl_added, 1.0, 1.0),
     efl_gfx_hint_fill_set(efl_added, EINA_TRUE, EINA_TRUE)
   );
  if (file)
    {
       snprintf(buf, sizeof(buf), "%s%s", elm_app_data_dir_get(), file);
       efl_file_simple_load(im, buf, NULL);
    }
   efl_key_data_set(win, "im", im);
   return im;
}

/**
 * @brief Callback for when the image orientation radio button changes.
 *
 * This function is triggered by the EFL_UI_RADIO_GROUP_EVENT_VALUE_CHANGED event.
 * It retrieves the selected orientation value and applies it to the image
 * object stored in the window's data.
 *
 * @param data The user data, which is the window (Eo *).
 * @param ev The event information.
 */
static void
my_im_ch(void *data, const Efl_Event *ev EINA_UNUSED)
{
   Eo *win = data;
   Eo *im = efl_key_data_get(win, "im");

   Efl_Gfx_Image_Orientation v = efl_ui_radio_group_selected_value_get(ev->object);
   if (((int)v) == -1) v = 0;

   efl_gfx_image_orientation_set(im, v);
   fprintf(stderr, "Set %i and got %i\n",
           v, efl_gfx_image_orientation_get(im));
}

/**
 * @brief Test case for basic image orientation.
 *
 * Creates a window with an image and a set of radio buttons to control
 * the image's orientation (rotation and flipping). This tests the
 * efl_gfx_image_orientation_set() API.
 */
void
test_ui_image(void *data EINA_UNUSED, Eo *obj  EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Eo *win, *box, *im, *rd;
   int i;

   win = win_add("image test", "Image Test");

   box = efl_add(EFL_UI_RADIO_BOX_CLASS, win);
   efl_gfx_hint_weight_set(box, 1.0, 1.0);
   efl_content_set(win, box);
   efl_key_data_set(win, "rdg", box);

   im = img_add(win, "/images/logo.png");
   efl_pack(box, im);

   for (i = 0; images_orient[i].name; ++i)
     {
        rd = efl_add(EFL_UI_RADIO_CLASS, win);
        efl_gfx_hint_fill_set(rd, EINA_TRUE, EINA_TRUE);
        efl_gfx_hint_weight_set(rd, 1.0, 0.0);
        efl_ui_radio_state_value_set(rd, images_orient[i].orient);
        efl_text_set(rd, images_orient[i].name);
        efl_pack(box, rd);
     }

   efl_gfx_entity_size_set(win, EINA_SIZE2D(320, 480));
   efl_event_callback_add(box, EFL_UI_RADIO_GROUP_EVENT_VALUE_CHANGED, my_im_ch, win);
}


/**
 * @brief Callback for when the alignment sliders change value.
 *
 * Retrieves the values from the horizontal and vertical alignment sliders
 * and applies them to the image's alignment hint.
 *
 * @param data The user data, which is the window (Eo *).
 * @param obj The object that triggered the callback (slider).
 * @param event_info Unused event information.
 */
static void
im_align_cb(void *data, Eo *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   double h,v;
   Eo *win = data;
   Eo *im = efl_key_data_get(win, "im");
   Eo *h_sl = efl_key_data_get(win, "h_sl");
   Eo *v_sl = efl_key_data_get(win, "v_sl");

   h = elm_slider_value_get(h_sl);
   v = elm_slider_value_get(v_sl);
   efl_gfx_hint_align_set(im, h, v);
   efl_gfx_hint_align_get(im, &h, &v);
   printf("align %.3f %.3f\n", h, v);
}

/**
 * @brief Maps image scale methods to human-readable names.
 * This array is used to populate UI elements that control how an
 * image is scaled within its bounds.
 * Each element is a struct with:
 *  - scale_type: The Efl_Gfx_Image_Scale_Method enum value.
 *  - name: The string representation for the UI.
 * The list is terminated by an element with a NULL name.
 */
static const struct {
   Efl_Gfx_Image_Scale_Method scale_type;
   const char *name;
} images_scale_type[] = {
  { EFL_GFX_IMAGE_SCALE_METHOD_NONE, "None" },
  { EFL_GFX_IMAGE_SCALE_METHOD_FILL, "Fill" },
  { EFL_GFX_IMAGE_SCALE_METHOD_FIT, "Fit" },
  { EFL_GFX_IMAGE_SCALE_METHOD_FIT_WIDTH, "Fit Horizontally" },
  { EFL_GFX_IMAGE_SCALE_METHOD_FIT_HEIGHT, "Fit Vertically" },
  { EFL_GFX_IMAGE_SCALE_METHOD_EXPAND, "Expand" },
  { EFL_GFX_IMAGE_SCALE_METHOD_TILE, "Tile" },
  { 0, NULL }
};

/**
 * @brief Callback for when the image scale method radio button changes.
 *
 * This function is triggered by the EFL_UI_RADIO_GROUP_EVENT_VALUE_CHANGED event.
 * It retrieves the selected scale method and applies it to the image object.
 *
 * @param data The user data, which is the window (Eo *).
 * @param ev The event information.
 */
static void
my_im_scale_ch(void *data, const Efl_Event *ev EINA_UNUSED)
{
   Eo *win = data;
   Eo *im = efl_key_data_get(win, "im");
   int v = efl_ui_radio_group_selected_value_get(ev->object);
   if (v == -1) v = 0;

   efl_gfx_image_scale_method_set(im, images_scale_type[v].scale_type);
   fprintf(stderr, "Set %d[%s] and got %d\n",
   images_scale_type[v].scale_type, images_scale_type[v].name, efl_gfx_image_scale_method_get(im));
}

/**
 * @brief Test case for image scaling methods.
 *
 * Creates a window with an image and a set of radio buttons to control
 * the image's scaling method (e.g., fill, fit, tile). This tests the
 * efl_gfx_image_scale_method_set() API.
 */
void
test_ui_image_scale_type(void *data EINA_UNUSED, Eo *obj  EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Eo *win, *box, *im, *rd;
   int i;

   win = win_add("image test scale type", "Image Test Scale Type");

   box = efl_add(EFL_UI_RADIO_BOX_CLASS, win);
   efl_gfx_hint_weight_set(box, 1.0, 1.0);
   efl_content_set(win, box);
   efl_key_data_set(win, "rdg", box);

   im = efl_add(EFL_UI_IMAGE_CLASS, win);
   char buf[PATH_MAX];
   snprintf(buf, sizeof(buf), "%s/images/logo.png", elm_app_data_dir_get());
   elm_image_file_set(im, buf, NULL);
   efl_gfx_hint_weight_set(im, 1.0, 1.0);
   efl_gfx_hint_fill_set(im, EINA_TRUE, EINA_TRUE);
   efl_pack(box, im);

   efl_key_data_set(win, "im", im);

   for (i = 0; images_scale_type[i].name; ++i)
     {
        rd = efl_add(EFL_UI_RADIO_CLASS, win);
        efl_gfx_hint_fill_set(rd, EINA_TRUE, EINA_TRUE);
        efl_gfx_hint_weight_set(rd, 1.0, 0.0);
        efl_ui_radio_state_value_set(rd, i);
        efl_text_set(rd, images_scale_type[i].name);
        efl_pack(box, rd);
     }
   efl_event_callback_add(box, EFL_UI_RADIO_GROUP_EVENT_VALUE_CHANGED, my_im_scale_ch, win);
   efl_gfx_entity_size_set(win, EINA_SIZE2D(320, 480));
}

/**
 * @brief Test case for image alignment within a layout's swallow part.
 *
 * This test places an image inside a "swallow" part of an Edje layout
 * and provides sliders to control the image's horizontal and vertical
 * alignment within that part. It tests the interaction between image
 * alignment hints and layouts.
 */
void
test_ui_image_swallow_align(void *data EINA_UNUSED, Eo *obj  EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Eo *win, *box, *im, *ly, *sl;
   char buf[PATH_MAX];

   win = win_add("image align", "Test Align Inside Layout");

   box = efl_add(EFL_UI_BOX_CLASS, win);
   efl_gfx_hint_weight_set(box, 1.0, 1.0);
   efl_content_set(win, box);

   ly = efl_add(EFL_UI_LAYOUT_CLASS, win);
   snprintf(buf, sizeof(buf), "%s/objects/test.edj", elm_app_data_dir_get());
   efl_file_simple_load(ly, buf, "image_align");
   efl_gfx_hint_weight_set(ly, 1.0, 1.0);
   efl_gfx_hint_fill_set(ly, EINA_TRUE, EINA_TRUE);
   efl_pack(box, ly);

   im = img_add(win, "/images/logo.png");
   efl_gfx_hint_weight_set(im, 0, 0);
   efl_gfx_hint_fill_set(im, EINA_FALSE, EINA_FALSE);
   efl_content_set(efl_part(ly, "swallow"), im);

   sl = elm_slider_add(win);
   elm_slider_value_set(sl, 0.5);
   efl_text_set(sl, "Horiz Align");
   efl_gfx_hint_weight_set(sl, 1.0, 0.0);
   evas_object_size_hint_align_set(sl, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_smart_callback_add(sl, "changed", im_align_cb, win);
   efl_pack(box, sl);
   evas_object_show(sl);
   efl_key_data_set(win, "h_sl", sl);

   sl = elm_slider_add(win);
   elm_slider_value_set(sl, 0.5);
   efl_text_set(sl, "Vert Align");
   efl_gfx_hint_weight_set(sl, 1.0, 0.0);
   evas_object_size_hint_align_set(sl, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_smart_callback_add(sl, "changed", im_align_cb, win);
   efl_pack(box, sl);
   evas_object_show(sl);
   efl_key_data_set(win, "v_sl", sl);

   efl_gfx_entity_size_set(win, EINA_SIZE2D(300, 600));
}

/**
 * @brief Callback for the "download,start" smart event.
 *
 * Updates a text display to indicate that the remote image download has begun.
 *
 * @param data The user data, which is the window (Eo *).
 * @param obj The image object that started downloading.
 * @param event_info Unused event information.
 */
static void
_download_start_cb(void *data, Eo *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Eo *win = data, *txt, *im;
   const char *url = NULL;
   char buf[4096] = {0};

   txt = efl_key_data_get(win, "txt");
   im = efl_key_data_get(win, "im");
   url = efl_file_get(im);
   snprintf(buf, sizeof(buf) - 1, "Remote image download started:\n%s", url);
   efl_text_set(txt, buf);
   printf("%s\n", buf);
   fflush(stdout);
}

/**
 * @brief Callback for the "download,progress" smart event.
 *
 * Updates a text display with the current download progress.
 *
 * @param data The user data, which is the window (Eo *).
 * @param obj The image object that is downloading.
 * @param event_info The progress information (Elm_Image_Progress *).
 */
static void
_download_progress_cb(void *data EINA_UNUSED, Eo *obj EINA_UNUSED, void *event_info)
{
   Elm_Image_Progress *p = event_info;
   Eo *win = data, *txt;
   char buf[4096] = {0};

   txt = efl_key_data_get(win, "txt");
   snprintf(buf, sizeof(buf) - 1, "Remote image download progress %.2f/%.2f.", p->now, p->total);
   efl_text_set(txt, buf);
   printf("%s\n", buf);
   fflush(stdout);
}

/**
 * @brief Callback for the "download,done" smart event.
 *
 * Updates a text display to indicate that the download has finished
 * successfully, and then hides the text display.
 *
 * @param data The user data, which is the window (Eo *).
 * @param obj The image object.
 * @param event_info Unused event information.
 */
static void
_download_done_cb(void *data, Eo *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Eo *win = data, *txt;
   char buf[4096] = {0};

   txt = efl_key_data_get(win, "txt");
   snprintf(buf, sizeof(buf) - 1, "Remote image download done.");
   efl_text_set(txt, buf);
   printf("%s\n", buf);
   fflush(stdout);

   evas_object_hide(txt);
}

/**
 * @brief Callback for the "download,error" smart event.
 *
 * Updates a text display to indicate that the download has failed.
 *
 * @param data The user data, which is the window (Eo *).
 * @param obj The image object.
 * @param event_info Unused event information.
 */
static void
_download_error_cb(void *data, Eo *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Eo *win = data, *txt;
   char buf[4096] = {0};

   txt = efl_key_data_get(win, "txt");
   snprintf(buf, sizeof(buf) - 1, "Remote image download failed.");
   efl_text_set(txt, buf);
   printf("%s\n", buf);
   fflush(stdout);

}

/**
 * @brief Callback for when the URL entry is activated (e.g., Enter pressed).
 *
 * Retrieves the URL from the entry, sets it as the file for the image
 * object, which triggers the download process. It also shows the status text.
 *
 * @param data The user data, which is the window (Eo *).
 * @param obj The entry object containing the URL.
 * @param event_info Unused event information.
 */
static void
_url_activate_cb(void *data, Eo *obj, void *event_info EINA_UNUSED)
{
   Eo *win = data, *txt, *im;
   const char *url;

   im = efl_key_data_get(win, "im");
   txt = efl_key_data_get(win, "txt");

   url = elm_object_text_get(obj);
   elm_image_file_set(im, url, NULL);

   evas_object_show(txt);
}

/**
 * @brief Test case for loading remote images over HTTP.
 *
 * Creates a window with an image view, a text entry for a URL, and status
 * text. It demonstrates the asynchronous download of an image from a URL
 * and the associated events for start, progress, completion, and error.
 */
void
test_remote_ui_image(void *data EINA_UNUSED, Eo *obj  EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Eo *win, *box, *im, *rd, *box2, *o;
   int i;

   win = win_add("image test", "Image Test");

   box = efl_add(EFL_UI_BOX_CLASS, win);
   efl_gfx_hint_weight_set(box, 1.0, 1.0);
   efl_content_set(win, box);

   o = efl_add(EFL_UI_TEXTBOX_CLASS, box, efl_text_interactive_editable_set(efl_added, EINA_FALSE));
   efl_text_wrap_set(o, EFL_TEXT_FORMAT_WRAP_MIXED);
   efl_gfx_hint_weight_set(o, 1.0, 1.0);
   efl_gfx_hint_fill_set(o, EINA_TRUE, EINA_TRUE);
   efl_pack(box, o);
   efl_key_data_set(win, "txt", o);
   evas_object_hide(o);

   im = o = img_add(win, NULL);
   efl_pack(box, o);

   evas_object_smart_callback_add(im, "download,start", _download_start_cb, win);
   evas_object_smart_callback_add(im, "download,progress", _download_progress_cb, win);
   evas_object_smart_callback_add(im, "download,done", _download_done_cb, win);
   evas_object_smart_callback_add(im, "download,error", _download_error_cb, win);

   box2 = efl_add(EFL_UI_RADIO_BOX_CLASS, win);
   efl_gfx_hint_weight_set(box2, 1.0, 1.0);
   efl_key_data_set(win, "rdg", box2);
   efl_pack(box, box2);

   for (i = 0; images_orient[i].name; ++i)
     {
        rd = efl_add(EFL_UI_RADIO_CLASS, win);
        efl_gfx_hint_fill_set(rd, EINA_TRUE, EINA_TRUE);
        efl_gfx_hint_weight_set(rd, 1.0, 0.0);
        efl_ui_radio_state_value_set(rd, images_orient[i].orient);
        efl_text_set(rd, images_orient[i].name);
        efl_pack(box2, rd);
     }
   efl_event_callback_add(box2, EFL_UI_RADIO_GROUP_EVENT_VALUE_CHANGED, my_im_ch, win);

   box2 = o = efl_add(EFL_UI_BOX_CLASS, box);
   efl_ui_layout_orientation_set(o, EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL);
   efl_gfx_hint_weight_set(o, 1.0, 0);
   efl_gfx_hint_fill_set(o, EINA_TRUE, EINA_TRUE);
   efl_pack(box, box2);

   o = efl_add(EFL_UI_TEXTBOX_CLASS, box2,
     efl_text_interactive_editable_set(efl_added, EINA_FALSE)
   );
   efl_text_set(o, "URL:");
   efl_pack(box2, o);

   o = elm_entry_add(box2);
   elm_entry_scrollable_set(o, 1);
   elm_entry_single_line_set(o, 1);
   efl_gfx_hint_weight_set(o, 1.0, 0);
   evas_object_size_hint_align_set(o, EVAS_HINT_FILL, EVAS_HINT_FILL);
   //elm_entry_entry_set(o, "http://41.media.tumblr.com/29f1ecd4f98aaff73fb21f479b450d4c/tumblr_mqsxdciQmB1rrju89o1_1280.jpg");
   elm_entry_entry_set(o, "http://68.media.tumblr.com/d14765b2cc4ec25d1e7d640f3ec77a40/tumblr_ohtpjtRNlm1rrju89o1_500.jpg");
   evas_object_smart_callback_add(o, "activated", _url_activate_cb, win);
   evas_object_show(o);
   efl_pack(box2, o);


   // set file now
   _url_activate_cb(win, o, NULL);

   efl_gfx_entity_size_set(win, EINA_SIZE2D(320, 480));
}

/**
 * @brief Callback for the "clicked" smart event on an image.
 *
 * Simply prints a message to stderr when the image is clicked.
 *
 * @param data Unused user data.
 * @param obj The image object that was clicked.
 * @param event_info Unused event information.
 */
static void
_img_clicked_cb(void *data EINA_UNUSED, Eo *obj, void *event_info EINA_UNUSED)
{
   fprintf(stderr, "%p - clicked\n", obj);
}

/**
 * @brief Test case for image clickability.
 *
 * Creates a window with a focusable image. When the image is clicked or
 * activated with the keyboard, the `_img_clicked_cb` is called. This
 * tests that an image can receive focus and handle input events.
 */
void
test_click_ui_image(void *data EINA_UNUSED, Eo *obj  EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Eo *win, *box, *im, *label;

   win = win_add("image test", "Image Test");
   elm_win_focus_highlight_enabled_set(win, EINA_TRUE);

   box = efl_add(EFL_UI_BOX_CLASS, win);
   efl_gfx_hint_weight_set(box, 1.0, 1.0);
   efl_content_set(win, box);

   im = img_add(win, "/images/logo.png");
   elm_object_focus_allow_set(im, EINA_TRUE);
   evas_object_smart_callback_add(im, "clicked", _img_clicked_cb, im);
   efl_pack(box, im);
   elm_object_focus_set(im, EINA_TRUE);

   label = efl_add(EFL_UI_TEXTBOX_CLASS, win, efl_text_interactive_editable_set(efl_added, EINA_FALSE));
   efl_text_set(label, "<b>Press Return/Space/KP_Return key on image to transit.</b>");
   efl_gfx_hint_weight_set(label, 0.0, 0.0);
   efl_gfx_hint_fill_set(label, EINA_TRUE, EINA_TRUE);
   efl_pack(box, label);

   efl_gfx_entity_size_set(win, EINA_SIZE2D(320, 480));
}

/**
 * @brief A macro to set the text of a status label and print it to stderr.
 * @param obj The text object to update.
 * @param fmt The format string for the status message.
 */
#define STATUS_SET(obj, fmt) do { \
   efl_text_set(obj, fmt); \
   fprintf(stderr, "%s\n", fmt); fflush(stderr); \
   } while (0)

/**
 * @brief Callback for the "load,open" smart event.
 *
 * Called when an asynchronous file open operation is completed.
 *
 * @param data The status text object.
 * @param obj The image object.
 * @param event_info Unused event info.
 */
static void
_img_load_open_cb(void *data, Eo *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Eo *status_text = data;

   STATUS_SET(status_text, "Async file open done.");
}

/**
 * @brief Callback for the "load,ready" smart event.
 *
 * Called when the image data has been loaded and is ready to be displayed.
 *
 * @param data The status text object.
 * @param obj The image object.
 * @param event_info Unused event info.
 */
static void
_img_load_ready_cb(void *data, Eo *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Eo *status_text = data;

   STATUS_SET(status_text, "Image is ready to show.");
}

/**
 * @brief Callback for the "load,error" smart event.
 *
 * Called if an error occurs during asynchronous file loading.
 *
 * @param data The status text object.
 * @param obj The image object.
 * @param event_info Unused event info.
 */
static void
_img_load_error_cb(void *data, Eo *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Eo *status_text = data;

   STATUS_SET(status_text, "Async file load failed.");
}

/**
 * @brief Callback for the "load,cancel" smart event.
 *
 * Called if the asynchronous file open operation is cancelled before it starts.
 *
 * @param data The status text object.
 * @param obj The image object.
 * @param event_info Unused event info.
 */
static void
_img_load_cancel_cb(void *data, Eo *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Eo *status_text = data;

   STATUS_SET(status_text, "Async file open has been cancelled.");
}

/**
 * @brief Creates and configures an image for async loading tests.
 *
 * This helper function creates an image, sets up its async loading and
 * preload properties, attaches callbacks for loading events, and starts
 * loading either a very large image or a small logo.
 *
 * @param data The parent window object.
 * @param async Whether to use asynchronous file opening.
 * @param preload Whether to disable preloading of image data.
 * @param logo If EINA_TRUE, load "logo.png"; otherwise, load
 *        "insanely_huge_test_image.jpg".
 */
static void
_create_image(Eo *data, Eina_Bool async, Eina_Bool preload, Eina_Bool logo)
{
   Eo *win = data;
   Eo *im, *status_text;
   Eo *box = efl_key_data_get(win, "box");
   char buf[PATH_MAX] = {0};

   im = img_add(win, NULL);
   elm_image_async_open_set(im, async);
   elm_image_preload_disabled_set(im, preload);

   efl_pack_begin(box, im);

   status_text = efl_key_data_get(win, "phld");
   if (!status_text)
     {
        status_text = efl_add(EFL_UI_TEXTBOX_CLASS, win, efl_text_interactive_editable_set(efl_added, EINA_FALSE));
        efl_gfx_hint_weight_set(status_text, 1.0, 0);
        efl_gfx_hint_fill_set(status_text, EINA_TRUE, EINA_TRUE);
        efl_key_data_set(win, "phld", status_text);
        efl_pack_after(box, status_text, im);
     }

   evas_object_smart_callback_add(im, "load,open", _img_load_open_cb, status_text);
   evas_object_smart_callback_add(im, "load,ready", _img_load_ready_cb, status_text);
   evas_object_smart_callback_add(im, "load,error", _img_load_error_cb, status_text);
   evas_object_smart_callback_add(im, "load,cancel", _img_load_cancel_cb, status_text);

   STATUS_SET(status_text, "Loading image...");
   if (!logo)
     snprintf(buf, sizeof(buf) - 1, "%s/images/insanely_huge_test_image.jpg", elm_app_data_dir_get());
   else
     snprintf(buf, sizeof(buf) - 1, "%s/images/logo.png", elm_app_data_dir_get());
   efl_file_simple_load(im, buf, NULL);
}

/**
 * @brief Callback for the "Reload" button's clicked event.
 *
 * Deletes the current image and creates a new one with the same file
 * but with async/preload settings taken from the UI checkboxes. This
 * effectively re-runs the load process with new settings.
 *
 * @param data The window object.
 * @param ev The click event information.
 */
static void
_reload_clicked(void *data, const Efl_Event *ev EINA_UNUSED)
{
   Eo *win = data;
   Eo *im = efl_key_data_get(win, "im");
   Eo *chk1 = efl_key_data_get(win, "chk1");
   Eo *chk2 = efl_key_data_get(win, "chk2");
   Eina_Bool async = efl_ui_selectable_selected_get(chk1);
   Eina_Bool preload = efl_ui_selectable_selected_get(chk2);
   Eina_Bool logo = EINA_FALSE;
   const char *file = NULL;

   file = efl_file_get(im);
   logo = (file && strstr(file, "logo"));

   efl_del(im);
   _create_image(win, async, preload, logo);
}

/**
 * @brief Callback for the "Switch" button's clicked event.
 *
 * Switches the image file between a large image and a small logo,
 * applying the current async/preload settings from the UI checkboxes.
 *
 * @param data The window object.
 * @param ev The click event information.
 */
static void
_switch_clicked(void *data, const Efl_Event *ev EINA_UNUSED)
{
   Eo *win = data;
   Eo *im = efl_key_data_get(win, "im");
   Eo *chk1 = efl_key_data_get(win, "chk1");
   Eo *chk2 = efl_key_data_get(win, "chk2");
   Eina_Bool async = efl_ui_selectable_selected_get(chk1);
   Eina_Bool preload = efl_ui_selectable_selected_get(chk2);
   char buf[PATH_MAX] = {0};
   Eina_Bool logo = EINA_FALSE;
   const char *file = NULL;

   file = efl_file_get(im);
   logo = (file && strstr(file, "logo"));

   if (logo)
     {
        snprintf(buf, sizeof(buf) - 1, "%s/images/insanely_huge_test_image.jpg", elm_app_data_dir_get());
     }
   else
     {
        snprintf(buf, sizeof(buf) - 1, "%s/images/logo.png", elm_app_data_dir_get());
     }

   elm_image_async_open_set(im, async);
   elm_image_preload_disabled_set(im, preload);
   efl_file_simple_load(im, buf, NULL);
}

/**
 * @brief Test case for asynchronous image loading.
 *
 * This test creates a window that loads a large image and provides UI
 * controls (checkboxes and buttons) to test different asynchronous loading
 * behaviors, including async open, preload disabling, reloading, and
 * switching images. It uses various "load,*" smart callbacks to display
 * status updates.
 */
void
test_load_ui_image(void *data EINA_UNUSED, Eo *obj  EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Eo *win, *box, *hbox, *label, *chk1, *chk2, *bt;

   win = win_add("image test", "Image Test");

   box = efl_add(EFL_UI_BOX_CLASS, win);
   efl_gfx_arrangement_content_align_set(box, 0.5, 1.0);
   efl_gfx_hint_weight_set(box, 1.0, 1.0);
   efl_content_set(win, box);
   efl_key_data_set(win, "box", box);

   _create_image(win, EINA_FALSE, EINA_FALSE, EINA_FALSE);

   hbox = efl_add(EFL_UI_BOX_CLASS, win);
   efl_ui_layout_orientation_set(hbox, EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL);
   efl_gfx_arrangement_content_align_set(hbox, 0, 0.5);
   efl_gfx_hint_weight_set(hbox, 1.0, 0.0);
   efl_gfx_hint_fill_set(hbox, EINA_TRUE, EINA_FALSE);
   {
      label = efl_add(EFL_UI_TEXTBOX_CLASS, win,
        efl_text_set(efl_added, "Async load options:"),
        efl_gfx_hint_weight_set(efl_added, 0.0, 0.0),
        efl_gfx_hint_fill_set(efl_added, EINA_TRUE, EINA_FALSE),
        efl_text_interactive_editable_set(efl_added, EINA_FALSE)
      );
      efl_pack(hbox, label);

      chk1 = efl_add(EFL_UI_CHECK_CLASS, hbox);
      efl_text_set(chk1, "Async file open");
      efl_gfx_hint_weight_set(chk1, 0.0, 0.0);
      efl_gfx_hint_fill_set(chk1, EINA_TRUE, EINA_FALSE);
      efl_pack(hbox, chk1);
      efl_key_data_set(win, "chk1", chk1);

      chk2 = efl_add(EFL_UI_CHECK_CLASS, hbox);
      efl_text_set(chk2, "Disable preload");
      efl_gfx_hint_weight_set(chk2, 0.0, 0.0);
      efl_gfx_hint_fill_set(chk2, EINA_TRUE, EINA_FALSE);
      efl_pack(hbox, chk2);
      efl_key_data_set(win, "chk2", chk2);
   }
   efl_pack(box, hbox);

   hbox = efl_add(EFL_UI_BOX_CLASS, win);
   efl_ui_layout_orientation_set(hbox, EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL);
   efl_gfx_arrangement_content_align_set(hbox, 0.5, 0.5);
   efl_gfx_hint_weight_set(hbox, 1.0, 0.0);
   efl_gfx_hint_fill_set(hbox, EINA_TRUE, EINA_FALSE),
   efl_gfx_hint_align_set(hbox, 0.5, 0.0);
   {
      bt = efl_add(EFL_UI_BUTTON_CLASS, win,
        efl_text_set(efl_added, "Image Reload"),
        efl_event_callback_add(efl_added, EFL_INPUT_EVENT_CLICKED, _reload_clicked, win)
      );
      efl_pack(hbox, bt);

      bt = efl_add(EFL_UI_BUTTON_CLASS, win,
        efl_text_set(efl_added, "Image Switch"),
        efl_event_callback_add(efl_added, EFL_INPUT_EVENT_CLICKED, _switch_clicked, win)
      );
      efl_pack(hbox, bt);
   }
   efl_pack(box, hbox);

   efl_gfx_entity_size_set(win, EINA_SIZE2D(320, 480));
}

/**
 * @brief Callback for when the prescale radio button selection changes.
 *
 * Sets the image prescale size based on the selected radio button's value.
 *
 * @param data The image object to modify.
 * @param ev The value changed event.
 */
static void
_cb_prescale_radio_changed(void *data, const Efl_Event *ev)
{
   Eo *o_bg = data;
   int size;
   size = efl_ui_radio_group_selected_value_get(ev->object);
   if (size == -1) size = 0;
   //FIXME
   elm_image_prescale_set(o_bg, size);
}

/**
 * @brief Test case for image prescaling.
 *
 * Creates a window with an image and radio buttons to change the prescale
 * size. Prescaling tells the loader to load a smaller version of the image
 * from the file directly, which can save memory and improve performance
 * for large images when only a smaller version is needed.
 */
void
test_ui_image_prescale(void *data EINA_UNUSED, Eo *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Eo *win, *im;
   Eo *box, *hbox;
   Eo *rd;

   win = win_add("image-prescale", "Image Prescale Test");

   box = efl_add(EFL_UI_BOX_CLASS, win);
   efl_content_set(win, box);

   im = img_add(win, "/images/plant_01.jpg");
   efl_pack(box, im);

   hbox = efl_add(EFL_UI_RADIO_BOX_CLASS, win);
   efl_ui_layout_orientation_set(hbox, EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL);
   efl_gfx_hint_weight_set(hbox, 1.0, 1.0);
   efl_gfx_hint_fill_set(hbox, EINA_TRUE, EINA_TRUE);

   rd = efl_add(EFL_UI_RADIO_CLASS, win);
   efl_ui_radio_state_value_set(rd, 50);
   efl_text_set(rd, "50");
   efl_gfx_hint_weight_set(rd, 1.0, 1.0);
   efl_pack(hbox, rd);

   rd = efl_add(EFL_UI_RADIO_CLASS, win);
   efl_ui_radio_state_value_set(rd, 100);
   efl_text_set(rd, "100");
   efl_gfx_hint_weight_set(rd, 1.0, 1.0);
   efl_pack(hbox, rd);

   rd = efl_add(EFL_UI_RADIO_CLASS, win);
   efl_ui_radio_state_value_set(rd, 200);
   efl_text_set(rd, "200");
   efl_gfx_hint_weight_set(rd, 1.0, 1.0);
   efl_pack(hbox, rd);
   efl_event_callback_add(hbox, EFL_UI_RADIO_GROUP_EVENT_VALUE_CHANGED, _cb_prescale_radio_changed, im);

   efl_ui_selectable_selected_set(rd, EINA_TRUE);

   efl_pack(box, hbox);

   efl_gfx_entity_size_set(win, EINA_SIZE2D(320, 320));
}
