#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#define EFL_UI_WIDGET_SCROLLABLE_CONTENT_PROTECTED
#include "elm_priv.h"

#define MY_CLASS EFL_UI_WIDGET_SCROLLABLE_CONTENT_MIXIN

#define MY_CLASS_NAME "Efl_Ui_Widget_Scrollable_Content"

typedef struct Efl_Ui_Widget_Scrollable_Content_Data
{
   Eo *scroller;
   Eo *label;
   Eina_Bool did_group_calc : 1;
} Efl_Ui_Widget_Scrollable_Content_Data;

/**
 * @brief Finalizes the sizing calculation of the widget and its scroller.
 *
 * This function takes the minimum size of the widget (obj_min) and the
 * minimum size of the scroller content (scr_min) and determines the
 * optimal size for the widget. It configures the scroller to either match
 * its content size or become scrollable, based on the widget's maximum
 * size hints.
 *
 * @param obj The widget object.
 * @param pd The private data of the widget.
 * @param obj_min The minimum size of the widget's chrome/theming, without content.
 * @param scr_min The minimum size of the content inside the scroller.
 */
static void
_scroller_sizing_eval(Eo *obj, Efl_Ui_Widget_Scrollable_Content_Data *pd,
                      Eina_Size2D obj_min, Eina_Size2D scr_min)
{
   Eina_Size2D max_size, min_size, size;
   max_size = efl_gfx_hint_size_max_get(obj);

   if (max_size.w != -1)
     max_size.w = (obj_min.w > max_size.w) ? obj_min.w : max_size.w;
   if (max_size.h != -1)
     max_size.h = (obj_min.h > max_size.h) ? obj_min.h : max_size.h;

   min_size = efl_gfx_hint_size_min_get(obj);

   size.w = (obj_min.w > min_size.w) ? obj_min.w : min_size.w;
   size.h = (obj_min.h > min_size.h) ? obj_min.h : min_size.h;

   if (pd->label)
     {
       scr_min.w = (obj_min.w > scr_min.w) ? obj_min.w : scr_min.w;
       scr_min.h = (obj_min.h > scr_min.h) ? obj_min.h : scr_min.h;
     }

   Eina_Size2D new_min = obj_min;

   if ((max_size.w == -1) && (max_size.h == -1))
     {
        efl_ui_scrollable_match_content_set(pd->scroller, EINA_FALSE, EINA_FALSE);
     }
   else if ((max_size.w == -1) && (max_size.h != -1))
     {
        if (max_size.h < scr_min.h)
          {
             efl_ui_scrollable_match_content_set(pd->scroller, EINA_FALSE, EINA_FALSE);
             size = EINA_SIZE2D(size.w, max_size.h);
          }
        else
          {
             new_min.h = scr_min.h;
             efl_ui_scrollable_match_content_set(pd->scroller, EINA_FALSE, EINA_TRUE);
             size = EINA_SIZE2D(size.w, scr_min.h);
          }
     }
   else if ((max_size.w != -1) && (max_size.h == -1))
     {
        if (max_size.w < scr_min.w)
          {
             efl_ui_scrollable_match_content_set(pd->scroller, EINA_FALSE, EINA_FALSE);
             size = EINA_SIZE2D(max_size.w, size.h);
          }
        else
          {
             new_min.w = scr_min.w;
             efl_ui_scrollable_match_content_set(pd->scroller, EINA_TRUE, EINA_FALSE);
             size = EINA_SIZE2D(scr_min.w, size.h);
          }
     }
   else if ((max_size.w != -1) && (max_size.h != -1))
     {
        Eina_Bool min_limit_w = EINA_FALSE;
        Eina_Bool min_limit_h = EINA_FALSE;

        if (max_size.w < scr_min.w)
          {
             size.w = max_size.w;
          }
        else
          {
             min_limit_w = EINA_TRUE;
             new_min.w = scr_min.w;
             size.w = scr_min.w;
          }

        if (max_size.h < scr_min.h)
          {
             size.h = max_size.h;
          }
        else
          {
             min_limit_h = EINA_TRUE;
             new_min.h = scr_min.h;
             size.h = scr_min.h;
          }

        efl_ui_scrollable_match_content_set(pd->scroller, min_limit_w, min_limit_h);
     }
   /* this event must come before the scroller recalc in order to ensure the scroller has the correct viewport size */
   efl_event_callback_call(obj, EFL_UI_WIDGET_SCROLLABLE_CONTENT_EVENT_OPTIMAL_SIZE_CALC, &size);
   efl_canvas_group_calculate(pd->scroller);

   efl_gfx_hint_size_restricted_min_set(obj, new_min);
}

/**
 * @brief Performs the main sizing calculation for the widget.
 *
 * This function is the entry point for calculating the widget's minimum size.
 * It calculates the minimum size of the widget's theme elements and the
 * minimum size of the content within the scroller. It handles special logic
 * for text content, calculating its size with and without wrapping to
 * determine the natural minimum width. It then calls _scroller_sizing_eval
 * to finalize the sizing logic.
 *
 * @param obj The widget object.
 * @param pd The private data of the widget.
 */
static void
_sizing_eval(Eo *obj, Efl_Ui_Widget_Scrollable_Content_Data *pd)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);
   Evas_Coord obj_minw = -1, obj_minh = -1;
   Evas_Coord scr_minw = -1, scr_minh = -1;
   Eina_Size2D text_min;

   //Calculate popup's min size including scroller's min size
     {
        if (pd->label)
          {
             elm_label_line_wrap_set(pd->label, ELM_WRAP_NONE);
             efl_canvas_group_calculate(pd->label);
             text_min = efl_gfx_hint_size_combined_min_get(pd->label);
             elm_label_line_wrap_set(pd->label, ELM_WRAP_MIXED);
             efl_canvas_group_calculate(pd->label);
          }

        efl_ui_scrollable_match_content_set(pd->scroller, !pd->label, EINA_TRUE);
        efl_canvas_group_calculate(pd->scroller);

        elm_coords_finger_size_adjust(1, &scr_minw, 1, &scr_minh);
        edje_object_size_min_restricted_calc
           (wd->resize_obj, &scr_minw, &scr_minh, scr_minw, scr_minh);
     }

   //Calculate popup's min size except scroller's min size
     {
        efl_ui_scrollable_match_content_set(pd->scroller, EINA_FALSE, EINA_FALSE);
        efl_canvas_group_calculate(pd->scroller);

        elm_coords_finger_size_adjust(1, &obj_minw, 1, &obj_minh);
        edje_object_size_min_restricted_calc
           (wd->resize_obj, &obj_minw, &obj_minh, obj_minw, obj_minh);
     }
   if (pd->label)
     scr_minw = text_min.w;
   _scroller_sizing_eval(obj, pd, EINA_SIZE2D(obj_minw, obj_minh), EINA_SIZE2D(scr_minw, scr_minh));
}

/**
 * @brief Implements the canvas group calculation for the widget.
 *
 * This function is called when the widget's geometry needs to be recalculated.
 * It triggers the sizing evaluation and ensures that further recalculations
 * for the widget and its internal scroller are suppressed to avoid redundant
 * calculations. The did_group_calc flag is used to signal if this logic
 * was executed.
 */
EOLIAN static void
_efl_ui_widget_scrollable_content_efl_canvas_group_group_calculate(Eo *obj, Efl_Ui_Widget_Scrollable_Content_Data *pd)
{
   pd->did_group_calc = EINA_FALSE;
   if (!pd->scroller)
     {
        efl_canvas_group_calculate(efl_super(obj, MY_CLASS));
        return;
     }
   pd->did_group_calc = EINA_TRUE;
   _sizing_eval(obj, pd);
   efl_canvas_group_need_recalculate_set(pd->scroller, EINA_FALSE);
   efl_canvas_group_need_recalculate_set(obj, EINA_FALSE);
}

/**
 * @brief Sets up the internal scroller widget.
 *
 * This is called on-demand when content is first set. It creates the
 * scroller, sets a specific style on it, and sets it as the widget's
 * content.
 */
static void
_scroller_setup(Eo *obj, Efl_Ui_Widget_Scrollable_Content_Data *pd)
{
   pd->scroller = efl_add(EFL_UI_SCROLLER_CLASS, obj,
     efl_ui_widget_style_set(efl_added, "popup/no_inset_shadow")
     );
   efl_wref_add(pd->scroller, &pd->scroller);
   efl_content_set(obj, pd->scroller);
}

/**
 * @brief Sets up the internal label widget.
 *
 * This is called on-demand when text content is first set. It creates the
 * label, configures its expansion hints, and sets it as the content of the
 * internal scroller.
 */
static void
_label_setup(Eo *obj EINA_UNUSED, Efl_Ui_Widget_Scrollable_Content_Data *pd)
{
   // TODO: Change internal component to Efl.Ui.Widget
   pd->label = elm_label_add(pd->scroller);
   //elm_widget_element_update(obj, pd->label, PART_NAME_TEXT);
   efl_gfx_hint_weight_set(pd->label, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   efl_wref_add(pd->label, &pd->label);
   efl_content_set(pd->scroller, pd->label);
}

/**
 * @brief Implements Efl.Ui.Widget.Scrollable.Content.scrollable_content_did_group_calc_get.
 *
 * This is used by consuming widgets to check if the sizing logic has run
 * during a specific calculation cycle.
 *
 * @return EINA_TRUE if the group calculation was performed, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_widget_scrollable_content_scrollable_content_did_group_calc_get(const Eo *obj EINA_UNUSED, Efl_Ui_Widget_Scrollable_Content_Data *pd)
{
   return pd->did_group_calc;
}

/**
 * @brief Implements Efl.Ui.Widget.Scrollable.Content.scrollable_content_set.
 *
 * Sets a custom widget as the content. This is mutually exclusive
 * with setting text content.
 *
 * @param[in] content The content to set.
 *
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
EOLIAN static Eina_Bool
_efl_ui_widget_scrollable_content_scrollable_content_set(Eo *obj, Efl_Ui_Widget_Scrollable_Content_Data *pd, Eo *content)
{
   Eina_Bool ret;

   if (!pd->scroller)
     _scroller_setup(obj, pd);
   ret = efl_content_set(pd->scroller, content);
   if (ret) efl_canvas_group_change(obj);
   return ret;
}

/**
 * @brief Implements Efl.Ui.Widget.Scrollable.Content.scrollable_content_get.
 *
 * @return The content widget, or NULL if no custom content is set or if
 * text content is being used.
 */
EOLIAN static Eo *
_efl_ui_widget_scrollable_content_scrollable_content_get(const Eo *obj EINA_UNUSED, Efl_Ui_Widget_Scrollable_Content_Data *pd)
{
   if (pd->label) return NULL;
   if (!pd->scroller) return NULL;
   return efl_content_get(pd->scroller);
}

/**
 * @brief Implements Efl.Ui.Widget.Scrollable.Content.scrollable_text_set.
 *
 * This creates an internal label to display the text. This is mutually
 * exclusive with setting custom content.
 *
 * @param[in] text The text to set.
 */
EOLIAN static void
_efl_ui_widget_scrollable_content_scrollable_text_set(Eo *obj, Efl_Ui_Widget_Scrollable_Content_Data *pd, const char *text)
{
   if (!pd->scroller)
     _scroller_setup(obj, pd);
   if (!pd->label)
     _label_setup(obj, pd);
   elm_object_text_set(pd->label, text);
   //efl_text_set(pd->label, text);
   efl_canvas_group_change(obj);
}

/**
 * @brief Implements Efl.Ui.Widget.Scrollable.Content.scrollable_text_get.
 *
 * @return The text content, or NULL if no text is set.
 */
EOLIAN static const char *
_efl_ui_widget_scrollable_content_scrollable_text_get(const Eo *obj EINA_UNUSED, Efl_Ui_Widget_Scrollable_Content_Data *pd)
{
   if (!pd->label) return NULL;
   return elm_object_text_get(pd->label);
   //return efl_text_get(pd->label);
}

/**
 * @brief Destructor for the scrollable content mixin.
 *
 * Cleans up resources, specifically the wref to the scroller.
 */
EOLIAN static void
_efl_ui_widget_scrollable_content_efl_object_destructor(Eo *obj, Efl_Ui_Widget_Scrollable_Content_Data *pd)
{
   if (pd->scroller) efl_wref_del(pd->scroller, &pd->scroller);
   efl_destructor(efl_super(obj, MY_CLASS));
}

#define EFL_UI_WIDGET_SCROLLABLE_CONTENT_EXTRA_OPS \
   EFL_CANVAS_GROUP_CALC_OPS(efl_ui_widget_scrollable_content)

#include "efl_ui_widget_scrollable_content.eo.c"
