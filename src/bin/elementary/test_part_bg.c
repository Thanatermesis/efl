#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#include <Efl_Ui.h>
#include <Elementary.h>

#define MAX_NUM_OF_CONTENT 17

/**
 * @brief Array of Efl_Class pointers for UI widgets.
 *
 * This array holds the classes of various UI widgets that will be created
 * and displayed in the test window. Each class corresponds to a specific
 * Elementary widget type. The array is populated in the
 * test_part_background() function.
 *
 * The structure of elements is an array of pointers to Efl_Class.
 * For example:
 * `content_class[0]` will hold `EFL_UI_CALENDAR_CLASS`.
 * `content_class[1]` will hold `EFL_UI_SLIDER_CLASS`.
 */
const Efl_Class *content_class[MAX_NUM_OF_CONTENT];


/**
 * @brief Callback to reset the background of the selected widget.
 *
 * This function is called when the "reset" button is clicked. It finds the
 * currently selected widget from the radio button group and resets its
 * "background" part to be transparent and have no image.
 *
 * @param data The Efl_Ui_Radio_Group that manages the selectable widgets.
 * @param ev The event information (unused).
 */
static void
_reset_cb(void *data, const Efl_Event *ev EINA_UNUSED)
{
   Efl_Ui_Radio_Group *radio = data;
   Evas_Object *target;

   radio = efl_ui_selectable_last_selected_get(radio);
   target = evas_object_data_get(radio, "data");

   efl_gfx_color_set(efl_part(target, "background"), 0, 0, 0, 0);
   efl_file_simple_load(efl_part(target, "background"), NULL, NULL);
}

/**
 * @brief Callback to change the background color of the selected widget.
 *
 * This function is invoked on a "color" button click. It toggles the
 * background color of the currently selected widget's "background" part
 * between red and green. A static variable is used to alternate the color.
 *
 * @param data The Efl_Ui_Radio_Group that manages the selectable widgets.
 * @param ev The event information (unused).
 */
static void
_color_cb(void *data, const Efl_Event *ev EINA_UNUSED)
{
   Efl_Ui_Radio_Group *radio = data;
   Evas_Object *target;
   static Eina_Bool i;

   radio = efl_ui_selectable_last_selected_get(radio);
   target = evas_object_data_get(radio, "data");
   i ^= EINA_TRUE;
   efl_gfx_color_set(efl_part(target, "background"), (i) ? 255 : 0, (i) ? 0 : 255, 0, 255);
}

/**
 * @brief Callback to cycle through image scaling methods.
 *
 * Triggered by the "scale_type" button, this function changes the scaling
 * method of the background image of the selected widget. It loads an image
 * if not present and cycles through the available Efl_Gfx_Image_Scale_Method
 * types on each click.
 *
 * @param data The Efl_Ui_Radio_Group that manages the selectable widgets.
 * @param ev The event information (unused).
 */
static void
_scale_type_cb(void *data, const Efl_Event *ev EINA_UNUSED)
{
   Efl_Ui_Radio_Group *radio = data;
   Evas_Object *target;
   Efl_Gfx_Image_Scale_Method type;
   char buf[PATH_MAX];

   radio = efl_ui_selectable_last_selected_get(radio);
   target = evas_object_data_get(radio, "data");

   snprintf(buf, sizeof(buf), "%s/images/plant_01.jpg", elm_app_data_dir_get());
   efl_file_simple_load(efl_part(target, "background"), buf, NULL);
   type = efl_gfx_image_scale_method_get(efl_part(target, "background"));
   type = (type + 1) % 6;
   efl_gfx_image_scale_method_set(efl_part(target, "background"), type);
}

/**
 * @brief Creates a box with radio buttons and corresponding widgets.
 *
 * This function populates a given box with a series of widgets, each
 * associated with a radio button for selection. It iterates through the
 * global `content_class` array to create instances of different UI widgets.
 *
 * @param box The parent Evas_Object (a box) to add the content to.
 * @return A new Efl_Ui_Radio_Group that manages all the created radio buttons.
 */
static Efl_Ui_Radio_Group *
_create_box_contents(Evas_Object *box)
{
   Evas_Object *hbox;
   Evas_Object *radio;
   Evas_Object *content;
   Efl_Ui_Radio_Group *radio_group;
   char buf[PATH_MAX];
   unsigned int i;

   radio_group = efl_new(EFL_UI_RADIO_GROUP_IMPL_CLASS, NULL);

   hbox = efl_add(EFL_UI_BOX_CLASS, box,
                  efl_ui_layout_orientation_set(efl_added, EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL),
                  efl_gfx_hint_weight_set(efl_added, 1, 1),
                  efl_pack_end(box, efl_added));

   radio = efl_add(EFL_UI_RADIO_CLASS, hbox);
   efl_gfx_hint_weight_set(radio, 0, 0);
   efl_ui_radio_state_value_set(radio, 0);
   efl_ui_radio_group_register(radio_group, radio);
   efl_pack_end(hbox, radio);

   content = efl_add(EFL_UI_BOX_CLASS, hbox,
                     efl_pack_end(hbox, efl_added));
   evas_object_data_set(radio, "data", content);

   content = efl_add(EFL_UI_TEXTBOX_CLASS, content,
                     efl_text_set(efl_added, "box"),
                     efl_text_interactive_editable_set(efl_added, EINA_FALSE),
                     efl_pack_end(content, efl_added));

   for (i = 0; i < MAX_NUM_OF_CONTENT; i++)
     {
        if (!content_class[i]) continue;

        hbox = efl_add(EFL_UI_BOX_CLASS, box,
                       efl_ui_layout_orientation_set(efl_added, EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL),
                       efl_pack_end(box, efl_added));

        radio = efl_add(EFL_UI_RADIO_CLASS, hbox);
        efl_ui_radio_state_value_set(radio, i + 1);
        efl_gfx_hint_weight_set(radio, 0, 0);
        efl_ui_radio_group_register(radio_group, radio);
        efl_pack_end(hbox, radio);

        content = efl_add(content_class[i], hbox,
                          efl_pack_end(hbox, efl_added));

        if (efl_isa(content, efl_text_interface_get()))
          efl_text_set(content, "text");

        if (efl_isa(content, EFL_UI_IMAGE_CLASS))
          {
             snprintf(buf, sizeof(buf), "%s/images/logo.png", elm_app_data_dir_get());
             efl_file_simple_load(content, buf, NULL);
          }

        evas_object_data_set(radio, "data", content);
     }

   efl_ui_radio_group_selected_value_set(radio_group, 0);

   return radio_group;
}

/**
 * @brief Main function for the part background test.
 *
 * This function sets up the test window for examining the "background" part
 * of various Elementary widgets. It creates a window, control buttons
 * (reset, color, scale), and a scrollable list of widgets. The `content_class`
 * array is populated here with the widget classes to be tested.
 *
 * @param data Unused.
 * @param obj Unused.
 * @param event_info Unused.
 */
void
test_part_background(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win;
   Evas_Object *scr;
   Evas_Object *vbox, *hbox;
   Efl_Ui_Radio_Group *radio_group;
   Evas_Object *reset_btn, *color_btn, *scale_btn;

   content_class[0] = EFL_UI_CALENDAR_CLASS;
   content_class[1] = EFL_UI_SLIDER_CLASS;
   content_class[2] = EFL_UI_SLIDER_INTERVAL_CLASS;
   content_class[3] = EFL_UI_PROGRESSBAR_CLASS;
   content_class[4] = EFL_UI_CLOCK_CLASS;
   content_class[5] = EFL_UI_BUTTON_CLASS;
   content_class[6] = EFL_UI_CHECK_CLASS;
   content_class[7] = EFL_UI_RADIO_CLASS;
   content_class[8] = EFL_UI_TEXTBOX_CLASS;
   content_class[9] = EFL_UI_FLIP_CLASS;
   content_class[10] = EFL_UI_PANES_CLASS;
   content_class[11] = EFL_UI_VIDEO_CLASS;
   content_class[12] = EFL_UI_BG_CLASS;
   content_class[13] = EFL_UI_IMAGE_CLASS;
   content_class[14] = EFL_UI_IMAGE_ZOOMABLE_CLASS;
   content_class[15] = EFL_UI_SPIN_CLASS;
   content_class[16] = EFL_UI_SPIN_BUTTON_CLASS;

   win = efl_add(EFL_UI_WIN_CLASS, efl_main_loop_get(),
                 efl_text_set(efl_added, "Widget Part Background"),
                 efl_ui_win_autodel_set(efl_added, EINA_TRUE));

   vbox = efl_add(EFL_UI_BOX_CLASS, win,
                  efl_gfx_hint_weight_set(efl_added, 1, 1),
                  efl_content_set(win, efl_added));

   hbox = efl_add(EFL_UI_BOX_CLASS, vbox,
                  efl_ui_layout_orientation_set(efl_added, EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL),
                  efl_gfx_hint_weight_set(efl_added, 1.0, 0.0),
                  efl_pack_end(vbox, efl_added));

   reset_btn = efl_add(EFL_UI_BUTTON_CLASS, hbox,
                       efl_pack_end(hbox, efl_added),
                       efl_text_set(efl_added, "reset"));
   color_btn = efl_add(EFL_UI_BUTTON_CLASS, hbox,
                       efl_pack_end(hbox, efl_added),
                       efl_text_set(efl_added, "color"));
   scale_btn = efl_add(EFL_UI_BUTTON_CLASS, hbox,
                       efl_pack_end(hbox, efl_added),
                       efl_text_set(efl_added, "scale_type"));

   scr = elm_scroller_add(vbox);
   evas_object_show(scr);
   evas_object_size_hint_weight_set(scr, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(scr, EVAS_HINT_FILL, EVAS_HINT_FILL);
   efl_pack_end(vbox, scr);

   vbox = efl_add(EFL_UI_BOX_CLASS, scr,
                  efl_gfx_hint_weight_set(efl_added, 1, 1),
                  efl_content_set(scr, efl_added));

   radio_group = _create_box_contents(vbox);

   efl_event_callback_add(reset_btn, EFL_INPUT_EVENT_CLICKED, _reset_cb, radio_group);
   efl_event_callback_add(color_btn, EFL_INPUT_EVENT_CLICKED, _color_cb, radio_group);
   efl_event_callback_add(scale_btn, EFL_INPUT_EVENT_CLICKED, _scale_type_cb, radio_group);

   efl_gfx_entity_size_set(win, EINA_SIZE2D(300, 200));
}
