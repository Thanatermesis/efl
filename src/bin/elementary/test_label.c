#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#ifdef _WIN32
# include <evil_private.h> /* strcasestr */
#endif

#include <Elementary.h>

/**
 * @enum _slide_style
 * @brief Defines the different slide styles for the label widget.
 */
enum _slide_style
{
   SLIDE_SHORT,
   SLIDE_LONG,
   SLIDE_BOUNCE,
   SLIDE_STYLE_LAST
};

/*** Label *******************************************************************/
/**
 * @brief Creates a window to test basic features of the elm_label widget.
 *
 * This test displays several labels with different properties:
 * - A simple bolded label.
 * - A multi-line label with fixed formatting.
 * - A label that line-wraps on character boundaries.
 * - Another wrapping label.
 * - A label demonstrating various markup tags like <hilight>, <success>, etc.
 *
 * @param data Unused.
 * @param obj Unused.
 * @param event_info Unused.
 */
void
test_label(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *bx, *lb;

   win = elm_win_util_standard_add("label", "Label");
   elm_win_autodel_set(win, EINA_TRUE);

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

   lb = elm_label_add(win);
   elm_object_text_set(lb,
                       "<b>This is a small label</b>"
                       );
   evas_object_size_hint_weight_set(lb, 0.0, 0.0);
   evas_object_size_hint_align_set(lb, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(bx, lb);
   evas_object_show(lb);

   lb = elm_label_add(win);
   elm_object_text_set(lb,
                       "This is a larger label with newlines<br/>"
                       "to make it bigger, bit it won't expand or wrap<br/>"
                       "just be a block of text that can't change its<br/>"
                       "formatting as it's fixed based on text<br/>"
                       );
   evas_object_size_hint_weight_set(lb, 0.0, 0.0);
   evas_object_size_hint_align_set(lb, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(bx, lb);
   evas_object_show(lb);

   lb = elm_label_add(win);
   elm_label_line_wrap_set(lb, ELM_WRAP_CHAR);
   elm_object_text_set(lb,
                       "<b>"
                       "This is more text designed to line-wrap here as "
                       "This object is resized horizontally. As it is "
                       "resized vertically though, nothing should change. "
                       "The amount of space allocated vertically should "
                       "change as horizontal size changes."
                       "</b>"
                       );
   evas_object_size_hint_weight_set(lb, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(lb, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(bx, lb);
   evas_object_show(lb);

   lb = elm_label_add(win);
   elm_object_text_set(lb,
                       "This small label set to wrap"
                       );
   evas_object_size_hint_weight_set(lb, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(lb, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(bx, lb);
   evas_object_show(lb);

   lb = elm_label_add(win);
   elm_object_text_set(lb,
                   "You can also use all the default entry style tags, like "
                   "<hilight>hilight</hilight>, <success>success</success>, "
                   "<bigger>bigger</bigger>, <smaller>smaller</smaller>, etc..."
                   );
   elm_label_line_wrap_set(lb, ELM_WRAP_WORD);
   evas_object_size_hint_weight_set(lb, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(lb, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(bx, lb);
   evas_object_show(lb);

   evas_object_resize(win, 320 * elm_config_scale_get(),
                           300 * elm_config_scale_get());

   evas_object_show(bx);
   evas_object_show(win);
   elm_win_resize_object_add(win, bx);
}

/*** Label Slide *************************************************************/
/**
 * @brief Callback function for radio button changes in the slide test.
 *
 * This function is called when a radio button for selecting the slide style is
 * changed. It sets the style of the target label based on the selected
 * radio button's value.
 *
 * @param data The label Evas_Object to modify.
 * @param obj The radio button Evas_Object that triggered the event.
 * @param event Unused.
 */
static void
_cb_size_radio_changed(void *data, Evas_Object *obj, void *event EINA_UNUSED)
{
   Evas_Object *lb = data;
   int style =  elm_radio_value_get((Evas_Object *)obj);
   switch (style)
     {
      case SLIDE_SHORT:
         elm_object_style_set(lb, "slide_short");
         break;

      case SLIDE_LONG:
         elm_object_style_set(lb, "slide_long");
         break;

      case SLIDE_BOUNCE:
         elm_object_style_set(lb, "slide_bounce");
         break;

      default:
         return;
     }
}

/**
 * @brief Callback for slide duration slider changes.
 *
 * Triggered when the user changes the value of the slide duration slider.
 * It updates the label's slide duration and restarts the slide animation.
 * It also updates the value of the speed slider to reflect the change.
 *
 * @param data The label Evas_Object whose slide duration is to be changed.
 * @param obj The slider widget that triggered the callback.
 * @param event_info Unused.
 */
static void
_duration_change_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Evas_Object *lb = data;
   double val = elm_slider_value_get(obj);

   elm_label_slide_duration_set(lb, val);
   elm_label_slide_go(lb);

   Evas_Object *sl = evas_object_data_get(lb, "speed_slider");
   elm_slider_value_set(sl, elm_label_slide_speed_get(lb));
}

/**
 * @brief Callback for slide speed slider changes.
 *
 * Triggered when the user changes the value of the slide speed slider.
 * It updates the label's slide speed and restarts the slide animation.
 * It also updates the value of the duration slider to reflect the change.
 *
 * @param data The label Evas_Object whose slide speed is to be changed.
 * @param obj The slider widget that triggered the callback.
 * @param event_info Unused.
 */
static void
_speed_change_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Evas_Object *lb = data;
   double val = elm_slider_value_get(obj);

   elm_label_slide_speed_set(lb, val);
   elm_label_slide_go(lb);

   Evas_Object *sl = evas_object_data_get(lb, "duration_slider");
   elm_slider_value_set(sl, elm_label_slide_duration_get(lb));
}

/**
 * @brief Callback for label resize events.
 *
 * When the label is resized, the slide duration and speed might change.
 * This callback updates the duration and speed sliders to show the current
 * values.
 *
 * @param data Unused.
 * @param e Unused.
 * @param obj The label object that was resized.
 * @param event_info Unused.
 */
static void
_label_resize_cb(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *obj,
                 void *event_info EINA_UNUSED)
{
   Evas_Object *speed_slider = evas_object_data_get(obj, "speed_slider");
   Evas_Object *duration_slider = evas_object_data_get(obj, "duration_slider");

   elm_slider_value_set(duration_slider, elm_label_slide_duration_get(obj));
   elm_slider_value_set(speed_slider, elm_label_slide_speed_get(obj));
}

/**
 * @brief Callback to stop the label slide animation.
 *
 * Iterates through a list of labels and sets their slide speed to a very low
 * value, effectively stopping the animation. This is triggered by a button click.
 *
 * @param data An Eina_List of labels to stop.
 * @param obj Unused.
 * @param event_info Unused.
 */
static void
_label_slide_stop_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Eina_List *lbs = data, *itr;
   Evas_Object *lb;
   EINA_LIST_FOREACH(lbs, itr, lb)
     {
        elm_label_slide_speed_set(lb, 0.01);
        elm_label_slide_go(lb);
     }
}

/**
 * @brief Creates a window to test the slide feature of the elm_label widget.
 *
 * This test demonstrates various slide functionalities including:
 * - Different slide styles (short, long, bounce).
 * - Dynamically changing slide duration and speed via sliders.
 * - Starting and stopping the slide animation.
 * - Synchronizing slide speed between multiple labels.
 *
 * @param data Unused.
 * @param obj Unused.
 * @param event_info EINA_UNUSED Unused.
 */
void
test_label_slide(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *gd, *rect, *lb, *lb1, *lb2, *rd, *rdg, *sl, *bt;
   Eina_List *lbs = NULL;

   win = elm_win_util_standard_add("label-slide", "Label Slide");
   elm_win_autodel_set(win, EINA_TRUE);

   gd = elm_grid_add(win);
   elm_grid_size_set(gd, 100, 100);
   evas_object_size_hint_weight_set(gd, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, gd);
   evas_object_show(gd);

   /* Test Label Slide */
   lb = elm_label_add(win);
   elm_object_text_set(lb, "<b>Test Label Slide:</b>");
   elm_grid_pack(gd, lb, 5, 0, 90, 10);
   evas_object_show(lb);

   rect = evas_object_rectangle_add(evas_object_evas_get(win));
   elm_grid_pack(gd, rect, 5, 10, 90, 10);
   evas_object_color_set(rect, 55, 125, 250, 255);
   evas_object_show(rect);

   lb = elm_label_add(win);
   elm_object_style_set(lb, "slide_short");
   elm_object_text_set(lb,
                       "This is a label set to slide. "
                       "If set slide to true the text of the label "
                       "will slide/scroll through the length of label."
                       "This only works with the themes \"slide_short\", "
                       "\"slide_long\" and \"slide_bounce\"."
                       );
   elm_label_slide_mode_set(lb, ELM_LABEL_SLIDE_MODE_AUTO);
   elm_label_slide_speed_set(lb, 40.0);
   elm_label_slide_go(lb);
   elm_grid_pack(gd, lb, 5, 10, 90, 10);
   evas_object_show(lb);
   lbs = eina_list_append(lbs, lb);

   /* The speed or the duration of the slide animation will change when the
    * label change size, so we need to update the sliders on resize. */
   evas_object_event_callback_add(lb, EVAS_CALLBACK_RESIZE, _label_resize_cb, NULL);

   rd = elm_radio_add(win);
   elm_radio_state_value_set(rd, SLIDE_SHORT);
   elm_object_text_set(rd, "slide_short");
   evas_object_size_hint_weight_set(rd, EVAS_HINT_EXPAND, EVAS_HINT_FILL);
   evas_object_smart_callback_add(rd, "changed", _cb_size_radio_changed, lb);
   elm_grid_pack(gd, rd, 5, 20, 30, 10);
   evas_object_show(rd);
   rdg = rd;

   rd = elm_radio_add(win);
   elm_radio_state_value_set(rd, SLIDE_LONG);
   elm_radio_group_add(rd, rdg);
   elm_object_text_set(rd, "slide_long");
   evas_object_size_hint_weight_set(rd, EVAS_HINT_EXPAND, EVAS_HINT_FILL);
   evas_object_smart_callback_add(rd, "changed", _cb_size_radio_changed, lb);
   elm_grid_pack(gd, rd, 35, 20, 30, 10);
   evas_object_show(rd);

   rd = elm_radio_add(win);
   elm_radio_state_value_set(rd, SLIDE_BOUNCE);
   elm_radio_group_add(rd, rdg);
   elm_object_text_set(rd, "slide_bounce");
   evas_object_size_hint_weight_set(rd, EVAS_HINT_EXPAND, EVAS_HINT_FILL);
   evas_object_smart_callback_add(rd, "changed", _cb_size_radio_changed, lb);
   elm_grid_pack(gd, rd, 65, 20, 30, 10);
   evas_object_show(rd);

   sl = elm_slider_add(win);
   elm_object_text_set(sl, "Slide Duration");
   elm_slider_unit_format_set(sl, "%1.1f sec");
   elm_slider_min_max_set(sl, 3, 40);
   elm_slider_value_set(sl, elm_label_slide_duration_get(lb));
   evas_object_size_hint_align_set(sl, EVAS_HINT_FILL, 0.5);
   evas_object_size_hint_weight_set(sl, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_smart_callback_add(sl, "changed", _duration_change_cb, lb);
   evas_object_data_set(lb, "duration_slider", sl);
   elm_grid_pack(gd, sl, 5, 30, 90, 10);
   evas_object_show(sl);

   sl = elm_slider_add(win);
   elm_object_text_set(sl, "Slide Speed");
   elm_slider_unit_format_set(sl, "%1.1f px/sec");
   elm_slider_min_max_set(sl, 40, 300);
   elm_slider_value_set(sl, elm_label_slide_speed_get(lb));
   evas_object_size_hint_align_set(sl, EVAS_HINT_FILL, 0.5);
   evas_object_size_hint_weight_set(sl, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_smart_callback_add(sl, "changed", _speed_change_cb, lb);
   evas_object_data_set(lb, "speed_slider", sl);
   elm_grid_pack(gd, sl, 5, 40, 90, 10);
   evas_object_show(sl);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Stop sliding");
   elm_grid_pack(gd, bt, 40, 50, 20, 10);
   evas_object_show(bt);

   /* Test 2 label at the same speed */
   lb = elm_label_add(win);
   elm_object_text_set(lb, "<b>Test 2 label with the same speed:</b>");
   elm_grid_pack(gd, lb, 5, 60, 90, 10);
   evas_object_show(lb);

   rect = evas_object_rectangle_add(evas_object_evas_get(win));
   elm_grid_pack(gd, rect, 5, 70, 90, 20);
   evas_object_color_set(rect, 55, 125, 250, 255);
   evas_object_show(rect);

   lb1 = elm_label_add(win);
   elm_object_style_set(lb1, "slide_short");
   elm_object_text_set(lb1, "This is a label set to slide with a fixed speed,"
                            " should match the speed with the below label."
                            " This label has few extra char for testing.");
   elm_label_slide_mode_set(lb1, ELM_LABEL_SLIDE_MODE_ALWAYS);
   elm_label_slide_speed_set(lb1, 40.0);
   elm_label_slide_go(lb1);
   elm_grid_pack(gd, lb1, 5, 70, 90, 10);
   evas_object_show(lb1);
   lbs = eina_list_append(lbs, lb1);

   lb2 = elm_label_add(win);
   elm_object_style_set(lb2, "slide_long");
   elm_object_text_set(lb2, "This is a label set to slide and will"
                            " match the speed of the upper label.");
   elm_label_slide_mode_set(lb2, ELM_LABEL_SLIDE_MODE_ALWAYS);
   elm_label_slide_speed_set(lb2, 40.0);
   elm_label_slide_go(lb2);
   elm_grid_pack(gd, lb2, 5, 80, 90, 10);
   evas_object_show(lb2);
   lbs = eina_list_append(lbs, lb2);

   evas_object_smart_callback_add(bt, "clicked", _label_slide_stop_cb, lbs);

   evas_object_resize(win, 320 * elm_config_scale_get(),
                           320 * elm_config_scale_get());
   evas_object_show(win);
}



/*** FIT TEXT **************************************************************/
enum BUTTON{
   BUTTON_MODE             = 0,
   BUTTON_MAX              = 1,
   BUTTON_MIN              = 2,
   BUTTON_STEP             = 3,
   BUTTON_ARRAY            = 4,
   BUTTON_CONTENT          = 5,
   BUTTON_STYLE            = 6,
   BUTTON_SIZE             = 7,
   BUTTON_ALL              = BUTTON_SIZE+1,
};

/** @brief String representations for the buttons in the text fit test. */
char* BUTTON_STR[BUTTON_ALL] ={
   "MODE",
   "MAX",
   "MIN",
   "STEP",
   "ARRAY",
   "CONTENT",
   "STYLE",
   "Get Size",
};

/** @brief Sample text contents for the text fit test. */
char *contents[] = {
   "Hello World",
   "This is Line<br>THis is other Line",
   "This text contains <font_size=20 color=#F00>SPECIFIC SIZE</font_size>that does not effected by fit mode"
   };

/** @brief Sample style strings for the text fit test. */
char *styles[] = {
   "DEFAULT='font=sans font_size=30 color=#000 wrap=mixed ellipsis=1.0'",
   "DEFAULT='font=sans font_size=30 color=#000 wrap=mixed'",
   "DEFAULT='font=sans font_size=30 color=#000 ellipsis=1.0'",
   "DEFAULT='font=sans font_size=30 color=#000'",
   };

/** @brief Names for the sample styles used in the text fit test status display. */
char *styles_names[] = {
   "wrap=<color=#F00>mixed</color> ellipsis=<color=#F00>1.0</color>",
   "wrap=<color=#F00>mixed</color> ellipsis=<color=#F00>NONE</color>",
   "wrap=<color=#F00>NONE</color> ellipsis=<color=#F00>1.0</color>",
   "wrap=<color=#F00>NONE</color> ellipsis=<color=#F00>NONE</color>",
   };

/**
 * @struct _APP
 * @brief Holds the application state for the text fit test.
 */
typedef struct _APP
{
   Evas_Object *win, *box, *txtblock,*bg, *boxHor, *boxHor2;
   Eo *btn[BUTTON_ALL];
   Eo *lbl_status;
   char * str;
   unsigned int i_contnet, i_style;
} APP;
APP *app;

char * get_fit_status(Eo * textblock);

/**
 * @brief Callback for button clicks in the text fit test UI.
 *
 * Handles clicks for all buttons in the test, modifying the textblock's
 * fit properties based on which button was pressed. It cycles through
 * fit modes, adjusts size ranges, step sizes, and content.
 *
 * @param data Unused.
 * @param obj The button that was clicked.
 * @param eventInfo Unused.
 */
static void _btn_clicked(void *data EINA_UNUSED, Eo *obj, void *eventInfo EINA_UNUSED){
   if (obj == app->btn[BUTTON_MODE])
     {
        unsigned int options;
        evas_textblock_fit_options_get(app->txtblock, &options);
        if (options == TEXTBLOCK_FIT_MODE_NONE)
           evas_textblock_fit_options_set(app->txtblock, TEXTBLOCK_FIT_MODE_HEIGHT);
        else if (options == TEXTBLOCK_FIT_MODE_HEIGHT)
           evas_textblock_fit_options_set(app->txtblock, TEXTBLOCK_FIT_MODE_WIDTH);
        else if (options == TEXTBLOCK_FIT_MODE_WIDTH)
           evas_textblock_fit_options_set(app->txtblock, TEXTBLOCK_FIT_MODE_ALL);
        else if (options == TEXTBLOCK_FIT_MODE_ALL)
           evas_textblock_fit_options_set(app->txtblock, TEXTBLOCK_FIT_MODE_NONE);
     }
   else if (obj == app->btn[BUTTON_MAX])
     {
        unsigned int min, max;
        evas_textblock_fit_size_range_get(app->txtblock, &min, &max);
        max -= 5;
        evas_textblock_fit_size_range_set(app->txtblock, min, max);
     }
   else if (obj == app->btn[BUTTON_MIN])
     {
        unsigned int min, max;
        evas_textblock_fit_size_range_get(app->txtblock, &min, &max);
        min += 5;
        evas_textblock_fit_size_range_set(app->txtblock, min, max);
     }
   else if (obj == app->btn[BUTTON_STEP])
     {
        unsigned int step;
        evas_textblock_fit_step_size_get(app->txtblock, &step);
        step++;
        evas_textblock_fit_step_size_set(app->txtblock, step);
     }
   else if (obj == app->btn[BUTTON_ARRAY])
     {
        unsigned int font_size[] = {10, 50, 100 ,150};
        evas_textblock_fit_size_array_set(app->txtblock,font_size,4);
     }
   else if (obj == app->btn[BUTTON_CONTENT])
     {
        app->i_contnet++;
        if(app->i_contnet>=sizeof(contents)/sizeof(char*))
           app->i_contnet=0;
        evas_object_textblock_text_markup_set(app->txtblock,contents[app->i_contnet]);
     }
   else if (obj == app->btn[BUTTON_STYLE])
     {
        app->i_style++;
        if(app->i_style>=sizeof(styles)/sizeof(char*))
           app->i_style=0;

        Evas_Textblock_Style *style = evas_object_textblock_style_get(app->txtblock);
        evas_textblock_style_set(style,styles[app->i_style]);
     }

   elm_object_text_set(app->lbl_status, get_fit_status(app->txtblock));
}

/**
 * @brief Generates a status string describing the current textblock fit settings.
 *
 * This function queries the textblock for its current fit mode, size range, step
 * size, and font size array, and formats this information into a human-readable
 * markup string.
 *
 * @param textblock The textblock object to query.
 * @return A pointer to a static buffer containing the status string.
 * The buffer is overwritten on each call.
 */
char * get_fit_status(Eo * textblock)
{
   static char status[0xFFF];
   unsigned int options,min,max,step,size_array[256];
   size_t size_array_len;
   int current_fitting_fontsize = 0;
   current_fitting_fontsize = evas_textblock_fit_font_size_get(textblock);
   evas_textblock_fit_options_get(textblock,&options);
   evas_textblock_fit_size_range_get(textblock,&min,&max);
   evas_textblock_fit_step_size_get(textblock,&step);
   evas_textblock_fit_size_array_get(textblock,NULL,&size_array_len,0);
   if (size_array_len>255)
      size_array_len = 255;
   evas_textblock_fit_size_array_get(textblock,size_array,NULL,size_array_len);

   strcpy(status,"Mode : ");
   if (options == TEXTBLOCK_FIT_MODE_NONE)
      strcat(status,"MODE_NONE");
   else if (options == TEXTBLOCK_FIT_MODE_HEIGHT)
      strcat(status,"MODE_HEIGHT");
   else if (options == TEXTBLOCK_FIT_MODE_WIDTH)
      strcat(status,"MODE_WIDTH");
   else if (options == TEXTBLOCK_FIT_MODE_ALL)
      strcat(status,"MODE_ALL");

   strcat(status,"<br>");
   sprintf(status + strlen(status),"Max   : %d<br>",max);
   sprintf(status + strlen(status),"Min   : %d<br>",min);
   sprintf(status + strlen(status),"Step  : %d<br>",step);
   sprintf(status + strlen(status),"Array  : [ ");
   for (size_t i = 0 ; i < 10 ; i++)
     {
        if(i<size_array_len)
           sprintf(status + strlen(status)," %d,",size_array[i]);
     }

   if(10<size_array_len)
      sprintf(status + strlen(status)," ... ");
   sprintf(status + strlen(status)," ]");

   sprintf(status + strlen(status),"<br>");
   sprintf(status + strlen(status),"%s<br>",styles_names[app->i_style]);
   if (current_fitting_fontsize == -1)
     {
        sprintf(status + strlen(status),"Current Font Size = No Fitting");
     }
   else
     {
        sprintf(status + strlen(status),"Current Font Size = %d", current_fitting_fontsize);
     }


   return status;
}

/**
 * @brief Creates a window to test evas_textblock_fit_* APIs.
 *
 * This test provides an interactive way to explore text fitting within a
 * textblock. It includes buttons to change:
 * - Fit mode (none, height, width, all).
 * - Maximum and minimum font size for fitting.
 * - Step size for font scaling.
 * - Font size array.
 * - Text content and style.
 * A status label displays the current settings.
 *
 * @param data Unused.
 * @param obj Unused.
 * @param event_info Unused.
 */
void
test_textblock_fit(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   app = calloc(1, sizeof(APP));

   elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);

   app->win = elm_win_util_standard_add("Main", "App");
   elm_win_autodel_set(app->win, EINA_TRUE);

   app->box = elm_box_add(app->win);
   app->boxHor = elm_box_add(app->box);
   app->boxHor2 = elm_box_add(app->box);
   app->txtblock = evas_object_textblock_add(app->box);
   app->bg = elm_bg_add(app->box);
   elm_bg_color_set(app->bg,255,255,255);

   Evas_Textblock_Style *style = evas_textblock_style_new();
   evas_textblock_style_set(style,styles[0]);
   evas_object_textblock_style_set(app->txtblock,style);
   evas_object_textblock_text_markup_set(app->txtblock,contents[0]);
   evas_textblock_style_free(style);
   style = NULL;

   elm_box_horizontal_set(app->boxHor, EINA_TRUE);
   elm_box_horizontal_set(app->boxHor2, EINA_TRUE);

   evas_object_size_hint_weight_set(app->box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(app->box, EVAS_HINT_FILL, EVAS_HINT_FILL);


   evas_object_size_hint_weight_set(app->box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(app->box, EVAS_HINT_FILL, EVAS_HINT_FILL);

   evas_object_show(app->txtblock);
   evas_object_show(app->bg);
   evas_object_show(app->box);
   evas_object_show(app->boxHor);
   evas_object_show(app->boxHor2);

   elm_box_pack_end(app->box, app->bg);
   elm_box_pack_end(app->box, app->boxHor);
   elm_box_pack_end(app->box, app->boxHor2);

   elm_object_content_set(app->bg,app->txtblock);

   elm_win_resize_object_add(app->win, app->box);
   evas_object_resize(app->win, 320, 480);

   for(int i = 0 ; i < BUTTON_ALL ; i++)
     {
        app->btn[i] = elm_button_add(app->boxHor);
        evas_object_smart_callback_add(app->btn[i], "clicked", _btn_clicked, NULL);
        elm_object_text_set(app->btn[i], BUTTON_STR[i]);
        elm_box_pack_end(app->boxHor, app->btn[i]);
        evas_object_show(app->btn[i]);
     }

   app->lbl_status = elm_label_add(app->boxHor2);
   elm_object_text_set(app->lbl_status, get_fit_status(app->txtblock));
   elm_box_pack_end(app->boxHor2, app->lbl_status);
   evas_object_show(app->lbl_status);

   evas_object_size_hint_weight_set(app->txtblock, EVAS_HINT_EXPAND,EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(app->txtblock, EVAS_HINT_FILL, EVAS_HINT_FILL);

   evas_object_size_hint_weight_set(app->bg, EVAS_HINT_EXPAND,EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(app->bg, EVAS_HINT_FILL, EVAS_HINT_FILL);

   evas_object_show(app->win);
}

/*** Text Memory Configuration **************************************************************/
enum BUTTON_MEM{
   BUTTON_MEM_SET_TEXT         = 0,
   BUTTON_MEM_APPLY_MEM        = 1,
   BUTTON_MEM_APPLY_FONT_SIZE  = 2,
   BUTTON_MEM_ALL              = BUTTON_MEM_APPLY_FONT_SIZE+1,
};

/** @brief String representations for buttons in the text memory test. */
char* BUTTON_MEM_STR[BUTTON_MEM_ALL] ={
   "Load Emojis Text",
   "Memory Limit(MB)",
   "Font Size"
};

char *content = "<align=center>😀😁😂🤣😃😄😅😆😉😊😋😎😍😘😗😙😚☺🙂🤗🤔😐😑😶🙄😏😣😥😮🤐😯😪😫😴😌🤓😛😜😝🤤😒😓😔😕🙃🤑😲☹🙁😖😞😟😤😢😭😦😧😨😩😬😰😱😳😵😡😠😇🤠🤡🤥😷🤒🤕🤢🤧☻😈👿👹👺💀☠👻👽👾🤖💩😺😸😹😻😼😽🙀😿😾🙈🙉🙊👦👧👨👩👵👶👼👨‍⚕️👩‍⚕️👨‍🎓👩‍🎓👨‍🏫👩‍🏫👨‍⚖👩‍⚖👨‍🌾👩‍🌾👨‍🍳👩‍🍳👨‍🔧👩‍🔧👨‍🏭👩‍🏭👨‍💼👩‍💼👨‍🔬👩‍🔬👨‍💻👩‍💻👨‍🎤👩‍🎤👨‍🎨👩‍🎨👨‍✈️👩‍✈️👨‍🚀👩‍🚀👨‍🚒👩‍🚒👮‍♂️👮‍♀️🕵️‍♂️🕵️‍♀️💂‍♂️💂‍♀️👷‍♂️👷‍♀️👳‍♂️👳‍♀️👱‍♂️👱‍♀️🎅🤶👸🤴👰🤵🤰👲🙍‍♂️🙍‍♀️🙎‍♂️🙎‍♀️🙅‍♂️🙅‍♀️🙆‍♂️🙆‍♀️💁‍♂️💁‍♀️🙋‍♂️🙋‍♀️🙇‍♂️🙇‍♀️🤦‍♂️🤦‍♀️🤷‍♂️🤷‍♀️💆‍♂️💆‍♀️💇‍♂️💇‍♀️🚶‍♂️🚶‍♀️🏃‍♂️🏃‍♀️💃🕺👯‍♂️👯‍♀️🕴🗣👤👥👫👬👭💏💑👪👨‍👩‍👧👨‍👩‍👧‍👦👨‍👩‍👦‍👦👨‍👩‍👧‍👧👨‍👦👨‍👦‍👦👨‍👧👨‍👧‍👦👨‍👧‍👧👩‍👦👩‍👦‍👦👩‍👧👩‍👧‍👦👩‍👧‍👧💪🤳👈👉☝️👆🖕👇✌🤞🖖🤘👊🖐✋👌👍👎✊👊🤛🤜🤚👋👏✍👐🙌🙏🤝💅👂👃👣👀👁👅👄💋💘❤💓💔💕💖💗💙💚💛💜🖤💝💞💟❣💌💤💢💣💥💦💨💫💬🗨🗯💭🕳👓🕶👔👕👖👗👘👙👚👛👜👝🛍🎒👞👟👠👡👢👑👒🎩🎓⛑📿💄💍💎🐵🐒🦍🐶🐕🐩🐺🦊🐱🐈🦁🐯🐅🐆🐴🐎🦌🦄🐮🐂🐃🐄🐷🐖🐗🐽🐏🐑🐐🐪🐫🐘🦏🐭🐁🐀🐹🐰🐇🐿🦇🐻🐨🐼🐾🦃🐔🐓🐣🐤🐥🐦🐧🕊🦅🦆🦉🐸🐊🐢🦎🐍🐲🐉🐳🐋🐬🐟🐠🐡🦈🐙🐚🦀🦐🦑🦋🐌🐛🐜🐝🐞🕷🕸🦂💐🌸💮🏵🌹🥀🌺🌻🌼🌷⚘🌱🌲🌳🌴🌵🌾🌿☘🍀🍁🍂🍃🍇🍈🍉🍊🍋🍌🍍🍎🍏🍐🍑🍒🍓🍅🥝🥑🍆🥔🥕🌽🌶🥒🍄🥜🌰🍞🥐🥖🥞🧀🍖🍗🥓🍔🍟🍕🌭🌮🌯🥙🥚🍳🥘🍲🥗🍿🍱🍘🍙🍚🍛🍜🍝🍠🍢🍣🍤🍥🍡🍦🍧🍨🍩🍪🎂🍰🍫🍬🍭🍮🍯🍼🥛☕🍵🍶🍾🍷🍸🍹🍺🍻🥂🍽🍴🥄🔪🏺🎃🎄🎆🎇✨🎈🎉🎊🎋🎍🎎🎏🎐🎑🎀🎁🎗🎟🎫🎖🏆🏅🥇🥈🥉⚽️⚾️🏀🏐🏈🏉🎾🎱🎳🏏🏑🏒🏓🏸🥊🥋🥅🎯⛳🏌️‍♂️🏌️‍♀️⛸🎣🎽🎿⛷🏂🏄‍♂️🏄‍♀️🏇🏊‍♂️🏊‍♀️⛹️‍♂️⛹️‍♀️🏋️‍♂️🏋️‍♀️🚴‍♂️🚴‍♀️🚵‍♂️🚵‍♀️🏎🏍🤸‍♂️🤸‍♀️🤼‍♂️🤼‍♀️🤽‍♂️🤽‍♀️🤾‍♂️🤾‍♀️🤺🤹‍♂️🤹‍♀️🎮🕹🎲♠️♥️♦️♣️🃏🀄🎴🌍🌎🌏🌐🗺🏔⛰🌋🗻🏕🏖🏜🏝🏞🏟🏛🏗🏘🏙🏚🏠🏡🏢🏣🏤🏥🏦🏨🏩🏪🏫🏬🏭🏯🏰💒🗼🗽⛪🕌🕍⛩🕋⛲⛺🌁🌃🌄🌅🌆🌇🌉⛼♨️🌌🎠🎡🎢💈🎪🎭🖼🎨🎰🚂🚃🚄🚅🚆🚇🚈🚉🚊🚝🚞🚋🚌🚍🚎🚐🚑🚒🚓🚔🚕🚖🚗🚘🚙🚚🚛⛟🚜🚲🛴🛵🚏🛣🛤⛽🚨🚥🚦🚧🛑⚓⛵🚣‍♂️🚣‍♀️🛶🚤🛳⛴🛥🚢✈🛩🛫🛬💺🚁🚟🚠🚡🚀🛰🛎🚪🛌🛏🛋🚽🚿🛀🛁⌛⏳⌚⏰⏱⏲🕰🕛🕧🕐🕜🕑🕝🕒🕞🕓🕟🕔🕠🕕🕡🕖🕢🕗🕣🕘🕤🕙🕥🕚🕦🌑🌒🌓🌔🌕🌖🌗🌘🌙🌚🌛🌜🌡☀️🌝🌞⭐🌟🌠☁️⛅⛈🌤🌥🌦🌧🌨🌩🌪🌫🌬🌀🌈🌂☂️☔⛱⚡❄☃️⛄☄🔥💧🌊🔇🔈🔉🔊📢📣📯🔔🔕🎼🎵🎶🎙🎚🎛🎤🎧📻🎷🎸🎹🎺🎻🥁📱📲☎️📞📟📠🔋🔌💻🖥🖨⌨🖱🖲💽💾💿📀🎥🎞📽🎬📺📷📸📹📼🔍🔎🔬🔭📡🕯💡🔦🏮📔📕📖📗📘📙📚📓📒📃📜📄📰🗞📑🔖🏷💰💴💵💶💷💸💳💱💲✉📧📨📩📤📥📦📫📪📬📭📮🗳✏✒🖋🖊🖌🖍📝💼📁📂🗂📅📆🗒🗓📇📈📉📊📋📌📍📎🖇📏📐✂️🗃🗄🗑🔒🔓🔏🔐🔑🗝🔨⛏⚒🛠🗡⚔🔫🏹🛡🔧🔩⚙🗜⚗⚖🔗⛓💉💊🚬⚰⚱🗿🛢🔮🔮🏧🚮🚰♿🚹🚺🚻🚼🚾🛂🛃🛄🛅⚠️🚸⛔🚫🚳🚭🚯🚱🚷📵🔞☢☣⬆️↗️➡️↘️⬇️↙️⬅️↖️↕️↔️↩↪⤴️⤵️🔃🔄🔙🔚🔛🔜🔝🛐⚛🕉✡☸☯️☦☮🕎🔯♈♉♊♋♌♍♎♏♐♑♒♓⛎🔀🔁🔂▶️⏩⏭⏯◀️⏪⏮🔼⏫🔽⏬⏸⏹⏺⏏🎦🔅🔆📶📳📴♻️📛⚜🔰🔱⭕✅☑✔✖❌❎➕♀️♂️⚕➖➗➰➿〽✳✴❇⁉️❓❔❕❗〰🔟💯🔠🔡🔢🔣🔤🅰️🆎️🅱️🆑️🆒️🆓️ℹ🆔️Ⓜ️🆕️🆖️🅾️🆗️🅿️🆘️🆙️🆚️🈁🈂🈷🈶🈯🉐🈹🈚🈲🉑🈸🈴🈳㊗㊙🈺🈵▫️◻◼◽◾⬛⬜🔶️🔷️🔸️🔹️🔺️🔻💠🔘🔲🔳⚪⚫🔴🔵🏁🚩🏴🏳🏳️‍🌈⚀⚁⚂⚃⚄⚅⛾♾🇦🇨🇦🇩🇦🇪🇦🇫🇦🇬🇦🇮🇦🇱🇦🇲🇦🇴🇦🇶🇦🇷🇦🇸🇦🇹🇦🇺🇦🇼🇦🇽🇦🇿🇧🇦🇧🇧🇧🇩🇧🇪🇧🇫🇧🇬🇧🇭🇧🇮🇧🇯🇧🇱🇧🇲🇧🇳🇧🇴🇧🇶🇧🇷🇧🇸🇧🇹🇧🇼🇧🇾🇧🇿🇨🇦🇨🇨🇨🇩🇨🇫🇨🇬🇨🇭🇨🇮🇨🇰🇨🇱🇨🇲🇨🇳🇨🇴🇨🇷🇨🇺🇨🇻🇨🇼🇨🇽🇨🇾🇨🇿🇩🇪🇩🇯🇩🇰🇩🇲🇩🇴🇩🇿🇪🇨🇪🇪🇪🇬🇪🇭🇪🇷🇪🇸🇪🇹🇪🇺🇫🇮🇫🇯🇫🇰🇫🇲🇫🇴🇫🇷🇬🇦🇬🇧🇬🇩🇬🇪🇬🇫🇬🇬🇬🇭🇬🇮🇬🇱🇬🇲🇬🇳🇬🇵🇬🇶🇬🇷🇬🇸🇬🇹🇬🇺🇬🇼🇬🇾🇭🇰🇭🇲🇭🇳🇭🇷🇭🇹🇭🇺🇮🇨🇮🇩🇮🇪🇮🇱🇮🇲🇮🇳🇮🇴🇮🇶🇮🇷🇮🇸🇮🇹🇯🇪🇯🇲🇯🇴🇯🇵🇰🇪🇰🇬🇰🇭🇰🇮🇰🇲🇰🇳🇰🇵🇰🇷🇰🇼🇰🇾🇰🇿🇱🇦🇱🇧🇱🇨🇱🇮🇱🇰🇱🇷🇱🇸🇱🇹🇱🇺🇱🇻🇱🇾🇲🇦🇲🇨🇲🇩🇲🇪🇲🇬🇲🇭🇲🇰🇲🇱🇲🇲🇲🇳🇲🇴🇲🇵🇲🇶🇲🇷🇲🇸🇲🇹🇲🇺🇲🇻🇲🇼🇲🇽🇲🇾🇲🇿🇳🇦🇳🇨🇳🇪🇳🇫🇳🇬🇳🇮🇳🇱🇳🇴🇳🇵🇳🇷🇳🇺🇳🇿🇴🇲🇵🇦🇵🇪🇵🇫🇵🇬🇵🇭🇵🇰🇵🇱🇵🇲🇵🇳🇵🇷🇵🇸🇵🇹🇵🇼🇵🇾🇶🇦🇷🇪🇷🇴🇷🇸🇷🇺🇷🇼🇸🇦🇸🇧🇸🇨🇸🇩🇸🇪🇸🇬🇸🇭🇸🇮🇸🇰🇸🇱🇸🇲🇸🇳🇸🇴🇸🇷🇸🇸🇸🇹🇸🇻🇸🇽🇸🇾🇸🇿🇹🇦🇹🇨🇹🇩🇹🇫🇹🇬🇹🇭🇹🇯🇹🇰🇹🇱🇹🇲🇹🇳🇹🇴🇹🇷🇹🇹🇹🇻🇹🇼🇹🇿🇺🇦🇺🇬🇺🇳🇺🇸🇺🇾🇺🇿🇻🇦🇻🇨🇻🇪🇻🇬🇻🇮🇻🇳🇻🇺🇼🇫🇼🇸🇽🇰🇾🇪🇾🇹🇿🇦🇿🇲🇿🇼</align>";

/**
 * @struct _APP_MEM
 * @brief Holds application state for the text memory configuration test.
 */
typedef struct _APP_MEM
{
   Evas_Object *win, *box, *main_entry;
   Eo *btn[BUTTON_MEM_ALL];
   Eo *entry[BUTTON_MEM_ALL];
} APP_MEM;
APP_MEM *app_mem;

/**
 * @brief Callback for button clicks in the text memory test UI.
 *
 * Handles clicks for various buttons:
 * - Loads a large string of emojis into the entry.
 * - Sets the Evas font texture cache size.
 * - Applies a new font size to the entry.
 *
 * @param data Unused.
 * @param obj The button that was clicked.
 * @param eventInfo Unused.
 */
static void _btn_clicked_mem(void *data EINA_UNUSED, Eo *obj, void *eventInfo EINA_UNUSED){
   if (obj == app_mem->btn[BUTTON_MEM_SET_TEXT])
     {
        elm_object_text_set(app_mem->main_entry, content);
     }
   else if (obj == app_mem->btn[BUTTON_MEM_APPLY_MEM])
     {
        int size = atoi(elm_object_text_get(app_mem->entry[BUTTON_MEM_APPLY_MEM]));
        if (size > 0 && size < 4000)
          {
            evas_font_data_cache_set(EVAS_FONT_DATA_CACHE_TEXTURE, size * 1024 * 1024);
          }
        else
          {
             elm_object_text_set(app_mem->entry[BUTTON_MEM_APPLY_MEM], "NAN");
          }
     }
   else if (obj == app_mem->btn[BUTTON_MEM_APPLY_FONT_SIZE])
     {
        int font_size = atoi(elm_object_text_get(app_mem->entry[BUTTON_MEM_APPLY_FONT_SIZE]));
        if (font_size > 0 && font_size < 1000)
          {
            char sfont_size[256] = {0};
            sprintf(sfont_size,"DEFAULT='font_size=%i'", font_size);
            elm_entry_text_style_user_push(app_mem->main_entry, sfont_size);
          }
        else
          {
             elm_object_text_set(app_mem->entry[BUTTON_MEM_APPLY_FONT_SIZE], "NAN");
          }
     }
}

/**
 * @brief Creates a window for testing text rendering memory configuration.
 *
 * This test is designed to check memory usage and performance when rendering
 * large amounts of complex text, such as a long string of emojis. It provides
 * controls to:
 * - Load a pre-defined emoji string.
 * - Set the font texture cache limit.
 * - Change the font size.
 * This is useful for performance and memory profiling of the text rendering system.
 *
 * @param data Unused.
 * @param obj Unused.
 * @param event_info Unused.
 */
void
test_text_memory(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   app_mem = calloc(1, sizeof(APP_MEM));

   elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);

   app_mem->win = elm_win_util_standard_add("Main", "App");
   elm_win_autodel_set(app_mem->win, EINA_TRUE);

   app_mem->box = elm_box_add(app_mem->win);
   app_mem->main_entry = elm_entry_add(app_mem->box);

   evas_object_size_hint_weight_set(app_mem->box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(app_mem->box, EVAS_HINT_FILL, EVAS_HINT_FILL);

   app_mem->btn[BUTTON_MEM_SET_TEXT] = elm_button_add(app_mem->box);
   evas_object_smart_callback_add(app_mem->btn[BUTTON_MEM_SET_TEXT], "clicked", _btn_clicked_mem, NULL);
   elm_object_text_set(app_mem->btn[BUTTON_MEM_SET_TEXT], BUTTON_MEM_STR[BUTTON_MEM_SET_TEXT]);
   evas_object_size_hint_align_set(app_mem->btn[BUTTON_MEM_SET_TEXT], EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(app_mem->box, app_mem->btn[BUTTON_MEM_SET_TEXT]);
   evas_object_show(app_mem->btn[BUTTON_MEM_SET_TEXT]);

   elm_entry_scrollable_set(app_mem->main_entry, EINA_TRUE);
   evas_object_show(app_mem->main_entry);
   evas_object_show(app_mem->box);

   elm_box_pack_end(app_mem->box, app_mem->main_entry);
   evas_object_smart_callback_add(app_mem->btn[BUTTON_MEM_SET_TEXT], "clicked", _btn_clicked_mem, NULL);
   evas_object_show(app_mem->btn[BUTTON_MEM_SET_TEXT]);

   elm_win_resize_object_add(app_mem->win, app_mem->box);
   evas_object_resize(app_mem->win, 320, 320);

   elm_entry_text_style_user_push(app_mem->main_entry, "DEFAULT='font_size=20'");

   for(int i = BUTTON_MEM_APPLY_MEM ; i < BUTTON_MEM_ALL ; i++)
     {
        app_mem->btn[i] = elm_button_add(app_mem->box);
        app_mem->entry[i] = elm_entry_add(app_mem->box);
        evas_object_smart_callback_add(app_mem->btn[i], "clicked", _btn_clicked_mem, NULL);
        elm_object_text_set(app_mem->btn[i], BUTTON_MEM_STR[i]);
        elm_box_pack_end(app_mem->box, app_mem->btn[i]);
        elm_box_pack_end(app_mem->box, app_mem->entry[i]);

        evas_object_size_hint_align_set(app_mem->btn[i], EVAS_HINT_FILL, EVAS_HINT_FILL);
        evas_object_size_hint_align_set(app_mem->entry[i], EVAS_HINT_FILL, EVAS_HINT_FILL);

        evas_object_show(app_mem->btn[i]);
        evas_object_show(app_mem->entry[i]);
     }

   elm_object_text_set(app_mem->entry[BUTTON_MEM_APPLY_MEM],"Texture limit in MB");
   elm_object_text_set(app_mem->entry[BUTTON_MEM_APPLY_FONT_SIZE],"20");

   evas_object_size_hint_weight_set(app_mem->main_entry, EVAS_HINT_EXPAND,EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(app_mem->main_entry, EVAS_HINT_FILL, EVAS_HINT_FILL);

   evas_object_show(app_mem->win);
}

/*** Label Wrap **************************************************************/
/**
 * @brief Creates a window to test elm_label's line wrapping feature.
 *
 * This test displays a single label with a long text. The label is configured
 * to wrap lines at character boundaries based on a fixed wrap width,
 * demonstrating how `elm_label_line_wrap_set()` and `elm_label_wrap_width_set()`
 * work together.
 *
 * @param data Unused.
 * @param obj Unused.
 * @param event_info Unused.
 */
void
test_label_wrap(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *lb;

   win = elm_win_util_standard_add("label-wrap", "Label Wrap");
   elm_win_autodel_set(win, EINA_TRUE);

   lb = elm_label_add(win);
   elm_object_text_set(lb,
                       "This is text for our label, that is long but "
                       "not too long. The label is designed to have line-wrap."
                       );
   elm_label_line_wrap_set(lb, ELM_WRAP_CHAR);
   elm_label_wrap_width_set(lb, ELM_SCALE_SIZE(200));
   evas_object_size_hint_weight_set(lb, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, lb);
   evas_object_show(lb);

   evas_object_show(win);
}


/*** Label Ellipsis **********************************************************/
/**
 * @brief Creates a window to test the ellipsis feature of elm_label.
 *
 * This test shows how a label truncates oversized text with an ellipsis (...)
 * at the end. It includes two labels:
 * - A label with plain text.
 * - A label with rich text markup.
 * Both are configured to use ellipsis when their content doesn't fit.
 *
 * @param data Unused.
 * @param obj Unused.
 * @param event_info Unused.
 */
void
test_label_ellipsis(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *bx, *lb;

   win = elm_win_util_standard_add("label-ellipsis", "Label Ellipsis");
   elm_win_autodel_set(win, EINA_TRUE);

   bx = elm_box_add(win);
   elm_box_padding_set(bx, 0, ELM_SCALE_SIZE(10));
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bx);
   evas_object_show(bx);

   lb = elm_label_add(win);
   elm_object_text_set(lb,
                       "This is a label set to ellipsis. "
                       "If set ellipsis to true and the text doesn't fit "
                       "in the label an ellipsis(\"...\") will be shown "
                       "at the end of the widget."
                       );
   elm_label_ellipsis_set(lb, EINA_TRUE);
   evas_object_size_hint_weight_set(lb, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(lb, EVAS_HINT_FILL, 0.0);
   elm_box_pack_end(bx, lb);
   evas_object_show(lb);

   lb = elm_label_add(win);
   elm_object_text_set(lb,
                       "<big>"
                       "This label also have <b>markups</b>, "
                       "<hilight>hilight</hilight>, "
                       "<success>success</success>, "
                       "<failure>failure</failure>."
                       "</big>"
                       );
   elm_label_ellipsis_set(lb, EINA_TRUE);
   evas_object_size_hint_weight_set(lb, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(lb, EVAS_HINT_FILL, 0.0);
   elm_box_pack_end(bx, lb);
   evas_object_show(lb);

   evas_object_resize(win, 300 * elm_config_scale_get(),
                           100 * elm_config_scale_get());
   evas_object_show(win);
}

/*** Label colors **************************************************************/
/**
 * @brief Creates a window to demonstrate color markup tags in elm_label.
 *
 * This test displays a label with text colored using various color tag formats:
 * - Named colors (e.g., `<color=red>`).
 * - Hexadecimal color codes (e.g., `<color=#0000FF>`).
 * - RGB color values (e.g., `<color=rgb(0,255,0)>`).
 * - RGBA color values (e.g., `<color=rgba(0,255,0,255)>`).
 *
 * @param data Unused.
 * @param obj Unused.
 * @param event_info Unused.
 */
void
test_label_colors(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *lb;

   win = elm_win_util_standard_add("label-colors", "Label colors");
   elm_win_autodel_set(win, EINA_TRUE);

   lb = elm_label_add(win);
   elm_object_text_set(lb,
                       "<color=red>this is red color line(color = red)<color><br>"
                       "<color=#0000FF>this is blue color line (color = #0000FF)<color><br>"
                       "<color=rgb(0,255,0)>this is green color line (color = rgb(0,255,0))<color><br>"
                       "<color=rgba(0,255,0,255)>this is blue color line (color = rgba(0,255,0,255))<color><br>"
                       );
   evas_object_size_hint_weight_set(lb, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, lb);
   evas_object_show(lb);

   evas_object_show(win);
}

/*** Label variation sequence **************************************************************/
/**
 * @brief Creates a window to test Unicode variation sequence rendering.
 *
 * This test displays a label containing Unicode variation sequences to check
 * if the font and rendering system handle them correctly. Variation sequences
 * allow a base character to have different graphical representations.
 *
 * @param data Unused.
 * @param obj Unused.
 * @param event_info Unused.
 */
void
test_label_variation_sequence(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *lb;

   win = elm_win_util_standard_add("label-variation sequence", "Label variation sequnece");
   elm_win_autodel_set(win, EINA_TRUE);

   lb = elm_label_add(win);
   elm_object_text_set(lb,
                       "You need to have at least on font contains variation sequence<br>"
                       "Three different 8 glyphs : <br>"
                       "8<tab>8&#xfe0f;<tab>8&#xfe0f;&#x20E3;<br>"
                       "line with 3 variation glyphs : <br>"
                       "8&#xfe0f;&#x20E3;&#x262a;&#xfe0f;AAA&#x262a;&#xfe0E;1234567&#xfe0f;&#x20E3;"
                       );
   evas_object_size_hint_weight_set(lb, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, lb);
   evas_object_show(lb);

   evas_object_show(win);
}

/*** Label Emoji *************************************************************/
/**
 * @brief Genlist item callback to get the text for a font item.
 *
 * This function is used by the genlist in the combobox to display font names.
 * It simply duplicates the string provided in the `data` pointer.
 *
 * @param data A C-string containing the font name.
 * @param obj Unused.
 * @param part Unused.
 * @return A newly allocated string with the font name. The caller is
 *         responsible for freeing it.
 */
static char *
_fontlist_text_get(void *data, Evas_Object *obj EINA_UNUSED, const char *part EINA_UNUSED)
{
   return strdup(data);
}

/**
 * @brief Sets the text of a label to a string of emojis with a specified font.
 *
 * This helper function formats a markup string containing various emojis
 * and sets it as the text for the given label. The font for the emojis is
 * specified by the `font` parameter.
 *
 * @param label The label object to set the text on.
 * @param font The name of the font to use for rendering the emojis.
 */
static void
_font_set(Evas_Object *label, const char *font)
{
   // Note: u1F420 doesn't work but U0001F420 does.
   const char *emojis = ""
                        "<font='%s',Sans font_size=16 align=left>"
                        "Grinning Face: \U0001f600<br/>"
                        "Face with Tears of Joy: \xf0\x9f\x98\x82<br/>"
                        "Panda Face: \xf0\x9f\x90\xbc<br/>"
                        "Ghost: \xf0\x9f\x91\xbb<br/>"
                        "Tropical Fish: \U0001F420<br/>"
                        "Mosque: \U0001f54c<br/>"
                        "Pencil: \u270f<br/>"
                        "Person With Ball: \u26f9<br/>"
                        "Birthday Cake: \U0001F382<br/>"
                        "Dog Face: \U0001F436<br/>"
                        "Music: \U0001F3B5 \U0001F3B6 \U0001F3BC<br/>"
                        "Person Shrugging: \U0001F937<br/>"
                        "</>";

   elm_object_text_set(label, eina_slstr_printf(emojis, font));
}

/**
 * @brief Callback for when a font is selected from the combobox.
 *
 * Triggered when the user presses an item in the font selection combobox.
 * It retrieves the selected font name and updates the emoji label to use
 * that font.
 *
 * @param data The label Evas_Object to be updated.
 * @param obj The combobox widget that triggered the event.
 * @param event_info The selected Elm_Object_Item.
 */
static void
_font_item_sel_cb(void *data, Evas_Object *obj, void *event_info)
{
   Elm_Object_Item *item = event_info;
   Evas_Object *lb = data;
   const char *font;

   font = elm_object_item_text_get(item);
   elm_object_text_set(obj, font);
   elm_combobox_hover_end(obj);
   _font_set(lb, font);
}

/**
 * @brief Callback for when a font list item is deleted.
 *
 * This function is called when an item in the genlist (used by the combobox)
 * is deleted. It frees the memory allocated for the font name string.
 *
 * @param data The font name string (char *) to be freed.
 * @param obj Unused.
 * @param event_info Unused.
 */
static void
_font_item_del_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   free(data);
}

/**
 * @brief Creates a window to test emoji rendering in a label.
 *
 * This test provides a way to check how different fonts render a set of
 * common emojis. It scans for available system fonts that contain "emoji"
 * or "symbola" in their names, lists them in a combobox, and allows the
 * user to select a font to apply to the emoji label.
 *
 * @param data Unused.
 * @param obj Unused.
 * @param event_info Unused.
 */
void
test_label_emoji(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *bx, *txt, *cmb;
   Elm_Genlist_Item_Class *itc;
   Eina_List *list, *l;
   const char *font;
   Evas *evas;

   const char *font_patterns[] = { "emoji", "symbola" };

   win = elm_win_util_standard_add("label-emoji", "Label Emoji");
   elm_win_autodel_set(win, EINA_TRUE);

   bx = elm_box_add(win);
   elm_box_padding_set(bx, 0, ELM_SCALE_SIZE(10));
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(bx, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_win_resize_object_add(win, bx);
   evas_object_show(bx);

   txt = elm_entry_add(win);
   elm_entry_editable_set(txt, EINA_TRUE);
   elm_entry_scrollable_set(txt, EINA_TRUE);
   evas_object_size_hint_weight_set(txt, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(txt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(bx, txt);
   evas_object_show(txt);

   cmb = elm_combobox_add(win);
   elm_entry_editable_set(cmb, EINA_FALSE);
   evas_object_size_hint_weight_set(cmb, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(cmb, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(bx, cmb);

   itc = elm_genlist_item_class_new();
   itc->item_style = "default";
   itc->func.text_get = _fontlist_text_get;

   evas = evas_object_evas_get(win);
   list = evas_font_available_list(evas);
   EINA_LIST_FOREACH(list, l, font)
     {
        size_t k;

        if (!font) continue;
        for (k = 0; k < EINA_C_ARRAY_LENGTH(font_patterns); k++)
          {
             if (strcasestr(font, font_patterns[k]))
               {
                  Elm_Object_Item *it;

                  it = elm_genlist_item_append(cmb, itc, strdup(font), NULL,
                                               ELM_GENLIST_ITEM_NONE,
                                               NULL, NULL);
                  elm_object_item_del_cb_set(it, _font_item_del_cb);
               }
          }
     }
   evas_font_available_list_free(evas, list);

   evas_object_smart_callback_add(cmb, "item,pressed", _font_item_sel_cb, txt);

   elm_genlist_item_class_free(itc);

   elm_object_text_set(cmb, "Sans");
   _font_set(txt, "Sans");
   evas_object_show(cmb);

   evas_object_resize(win, 300 * elm_config_scale_get(),
                           400 * elm_config_scale_get());
   evas_object_show(win);
}
