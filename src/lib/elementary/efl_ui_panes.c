#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define EFL_ACCESS_OBJECT_PROTECTED
#define ELM_LAYOUT_PROTECTED
#define EFL_GFX_HINT_PROTECTED
#define EFL_PART_PROTECTED
#define EFL_INPUT_CLICKABLE_PROTECTED

#include <Elementary.h>

#include "elm_priv.h"
#include "elm_widget_layout.h"
#include "efl_ui_panes_private.h"

#include "efl_ui_panes_part.eo.h"
#include "elm_part_helper.h"

#define MY_CLASS EFL_UI_PANES_CLASS
#define MY_CLASS_PFX efl_ui_panes

#define MY_CLASS_NAME "Efl.Ui.Panes"

/**
 * @defgroup Efl_Ui_Panes_Group Efl.Ui.Panes
 * @ingroup Elementary
 *
 * @brief A widget that displays two content objects with a draggable bar separating them.
 *
 * The panes widget allows users to resize the two content areas by dragging
 * the bar between them. It can be oriented horizontally or vertically.
 *
 * @{
 *
 * TODO
 * Update the minimun height of the bar in the theme.
 * No minimun should be set in the vertical theme
 * Add events (move, start ...)
 */

static const char SIG_CLICKED[] = "clicked";
static const char SIG_PRESS[] = "press";
static const char SIG_UNPRESS[] = "unpress";
static const char SIG_DOUBLE_CLICKED[] = "clicked,double";
/**< Signal for double click events */

/**
 * @brief Smart callback function descriptions.
 *
 * These descriptions define the smart callbacks available for the Efl.Ui.Panes widget.
 * Each element in the array is an Evas_Smart_Cb_Description structure:
 * - First element: The name of the signal (e.g., "clicked").
 * - Second element: A description of the signal (currently empty for these).
 *
 * Example:
 * @code
 * static const Evas_Smart_Cb_Description _smart_callbacks[] = {
 *    {"clicked", ""}, // Emitted when the panes bar is clicked.
 *    {"press", ""},   // Emitted when the panes bar is pressed.
 *    {"unpress", ""}, // Emitted when the panes bar is unpressed.
 *    {"clicked,double", ""}, // Emitted when the panes bar is double-clicked.
 *    {NULL, NULL}     // Terminator for the array.
 * };
 * @endcode
 */
static const Evas_Smart_Cb_Description _smart_callbacks[] = {
   {SIG_CLICKED, ""},
   {SIG_PRESS, ""},
   {SIG_UNPRESS, ""},
   {SIG_DOUBLE_CLICKED, ""},
   {NULL, NULL}
};

/**
 * @brief Content part aliases for the panes widget.
 *
 * This array maps user-friendly alias names to actual theme part names.
 * This allows for more intuitive naming when setting content.
 * Each element is an Elm_Layout_Part_Alias_Description structure:
 * - First element: The alias name (e.g., "left", "first").
 * - Second element: The corresponding theme part name (e.g., "elm.swallow.left").
 *
 * Example:
 * @code
 * static const Elm_Layout_Part_Alias_Description _content_aliases[] =
 * {
 *    // For horizontal orientation (legacy and new)
 *    {"left", "elm.swallow.left"},   // Alias "left" maps to "elm.swallow.left"
 *    {"right", "elm.swallow.right"}, // Alias "right" maps to "elm.swallow.right"
 *    // For vertical orientation (aliases map to the same parts as horizontal)
 *    {"top", "elm.swallow.left"},    // Alias "top" maps to "elm.swallow.left"
 *    {"bottom", "elm.swallow.right"},// Alias "bottom" maps to "elm.swallow.right"
 *    // Generic aliases
 *    {"first", "elm.swallow.left"},  // Alias "first" maps to "elm.swallow.left"
 *    {"second", "elm.swallow.right"},// Alias "second" maps to "elm.swallow.right"
 *    {NULL, NULL} // Terminator for the array.
 * };
 * @endcode
 * @note "elm.swallow.left/right" are planned to be changed to "*.first/second" in a new theme.
 */
static const Elm_Layout_Part_Alias_Description _content_aliases[] =
{
   //XXX: change elm.swallow.left/right to *.first/second in new theme.
   {"left", "elm.swallow.left"},
   {"right", "elm.swallow.right"},
   {"top", "elm.swallow.left"},
   {"bottom", "elm.swallow.right"},
   {"first", "elm.swallow.left"},
   {"second", "elm.swallow.right"},
   {NULL, NULL}
};

/**
 * @brief Sets the minimum size constraints for the panes based on content minimum sizes and split ratios (new API).
 *
 * This function calculates and applies minimum size constraints to the draggable bar
 * of the panes widget. It considers the minimum sizes of the first and second content
 * areas, their respective minimum split ratios, and the current orientation of the panes.
 * This ensures that content areas do not become smaller than their specified minimums
 * or their minimum proportion of the panes.
 *
 * @param data The Evas_Object (panes widget) whose minimum sizes are to be set.
 */
static void _set_min_size_new(void *data);

//TODO: efl_ui_slider also use this.
/**
 * @brief Helper function to find a specific suffix in a theme group string.
 *
 * This function searches for a given suffix (e.g., "horizontal" or "vertical")
 * at the end of the current theme group string. It's used to determine if the
 * orientation is already part of the group name.
 *
 * @param cur_group The current theme group string.
 * @param search The suffix string to search for.
 * @param len The length of the @p cur_group string.
 * @param is_legacy EINA_TRUE if the widget is in legacy mode, EINA_FALSE otherwise.
 *                  In legacy mode, the original @p cur_group is always returned.
 * @return A pointer to the beginning of the found suffix within @p cur_group,
 *         or @p cur_group if in legacy mode, or NULL if the suffix is not found at the end.
 */
static const char *
_theme_group_modify_pos_get(const char *cur_group, const char *search, size_t len, Eina_Bool is_legacy)
{
   const char *pos = NULL;
   const char *temp_str = NULL;

   if (is_legacy)
     return cur_group;

   temp_str = cur_group + len - strlen(search);
   if (temp_str >= cur_group)
     {
         if (!strcmp(temp_str, search))
           pos = temp_str;
     }

   return pos;
}

/**
 * @brief Constructs the appropriate theme group name based on the panes orientation.
 *
 * This function generates a theme group string (e.g., "elm/panes/horizontal" or
 * "efl/panes/vertical") by appending the current orientation to the base theme
 * group of the widget. It handles both legacy and new theme naming conventions.
 *
 * @param obj The Evas_Object (panes widget).
 * @param sd The private data of the Efl_Ui_Panes widget.
 * @return A newly allocated string containing the full theme group name.
 *         The caller is responsible for freeing this string.
 */
static char *
_efl_ui_panes_theme_group_get(Evas_Object *obj, Efl_Ui_Panes_Data *sd)
{
   const char *pos = NULL;
   const char *cur_group = elm_widget_theme_element_get(obj);
   Eina_Strbuf *new_group = eina_strbuf_new();
   Eina_Bool is_legacy = elm_widget_is_legacy(obj);
   size_t len = 0;

   if (cur_group)
     {
        len = strlen(cur_group);
        pos = _theme_group_modify_pos_get(cur_group, "horizontal", len, is_legacy);
        if (!pos)
          pos = _theme_group_modify_pos_get(cur_group, "vertical", len, is_legacy);

        // TODO: change separator when it is decided.
        //       can skip when prev_group == cur_group
        if (!pos)
          {
              eina_strbuf_append(new_group, cur_group);
              eina_strbuf_append(new_group, "/");
          }
        else
          {
              eina_strbuf_append_length(new_group, cur_group, pos - cur_group);
          }
     }

   if (sd->dir == EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL)
     eina_strbuf_append(new_group, "horizontal");
   else
     eina_strbuf_append(new_group, "vertical");

   return eina_strbuf_release(new_group);
}

/**
 * @internal
 * @brief Applies the theme to the Efl.Ui.Panes widget.
 *
 * This function is called when the theme of the panes widget needs to be updated.
 * It sets the correct theme group based on orientation, adjusts finger size for
 * the event area, and re-applies fixed state and content size.
 *
 * @param obj The Evas_Object (panes widget).
 * @param sd The private data of the Efl_Ui_Panes widget.
 * @return EFL_UI_THEME_APPLY_ERROR_NONE on success, or an error code on failure.
 */
EOLIAN static Eina_Error
_efl_ui_panes_efl_ui_widget_theme_apply(Eo *obj, Efl_Ui_Panes_Data *sd)
{
   double size;
   Evas_Coord minw = 0, minh = 0;
   char *group;

   Eina_Error int_ret = EFL_UI_THEME_APPLY_ERROR_GENERIC;

   group = _efl_ui_panes_theme_group_get(obj, sd);
   if (group)
     {
        elm_widget_theme_element_set(obj, group);
        free(group);
     }

   evas_object_hide(sd->event);
   elm_coords_finger_size_adjust(1, &minw, 1, &minh);
   efl_gfx_hint_size_min_set(sd->event, EINA_SIZE2D(minw, minh));

   int_ret = efl_ui_widget_theme_apply(efl_super(obj, MY_CLASS));
   if (int_ret == EFL_UI_THEME_APPLY_ERROR_GENERIC) return int_ret;

   size = elm_panes_content_left_size_get(obj);

   if (sd->fixed)
     {
        if (elm_widget_is_legacy(obj))
          {
             elm_layout_signal_emit(obj, "elm,panes,fixed", "elm");

             //TODO: remove this signal on EFL 2.0.
             // I left this due to the backward compatibility.
             elm_layout_signal_emit(obj, "elm.panes.fixed", "elm");
          }
        else
          elm_layout_signal_emit(obj, "efl,panes,fixed", "efl");
     }

   elm_panes_content_left_size_set(obj, size);

   return int_ret;
}

/**
 * @brief Callback function for the "clicked" signal from the layout.
 *
 * This function is triggered when the panes bar (or a clickable part in the theme)
 * receives a click event. It, in turn, emits the "clicked" smart callback
 * for the panes widget.
 *
 * @param data The Evas_Object (panes widget) that is the source of the event.
 * @param obj The Evas_Object from which the signal originated (unused).
 * @param emission The emitted signal string (unused).
 * @param source The source of the signal (unused).
 */
static void
_on_clicked(void *data,
            Evas_Object *obj EINA_UNUSED,
            const char *emission EINA_UNUSED,
            const char *source EINA_UNUSED)
{
   evas_object_smart_callback_call(data, "clicked", NULL);
}

/**
 * @brief Callback function for the "double_clicked" signal from the layout.
 *
 * This function is triggered when the panes bar (or a clickable part in the theme)
 * receives a double-click event. It sets a flag indicating a double click occurred,
 * which is then checked in _on_unpressed to emit the "clicked,double" smart callback.
 *
 * @param data The Evas_Object (panes widget) that is the source of the event.
 * @param obj The Evas_Object from which the signal originated (unused).
 * @param emission The emitted signal string (unused).
 * @param source The source of the signal (unused).
 */
static void
_double_clicked(void *data,
                Evas_Object *obj EINA_UNUSED,
                const char *emission EINA_UNUSED,
                const char *source EINA_UNUSED)
{
   EFL_UI_PANES_DATA_GET(data, sd);

   sd->double_clicked = EINA_TRUE;
}

/**
 * @brief Callback function for the "press" signal from the layout.
 *
 * This function is triggered when the panes bar (or a clickable part in the theme)
 * is pressed. It emits the "press" smart callback for the panes widget and
 * notifies the clickable interface that a press event occurred.
 *
 * @param data The Evas_Object (panes widget) that is the source of the event.
 * @param obj The Evas_Object from which the signal originated (unused).
 * @param emission The emitted signal string (unused).
 * @param source The source of the signal (unused).
 */
static void
_on_pressed(void *data,
            Evas_Object *obj EINA_UNUSED,
            const char *emission EINA_UNUSED,
            const char *source EINA_UNUSED)
{
   evas_object_smart_callback_call(data, "press", NULL);
   efl_input_clickable_press(data, 1);
}

/**
 * @brief Callback function for the "unpress" signal from the layout.
 *
 * This function is triggered when the panes bar (or a clickable part in the theme)
 * is unpressed. It emits the "unpress" smart callback. If a double click
 * was detected prior to this unpress, it also emits the "clicked,double"
 * smart callback.
 *
 * @param data The Evas_Object (panes widget) that is the source of the event.
 * @param obj The Evas_Object from which the signal originated (unused).
 * @param emission The emitted signal string (unused).
 * @param source The source of the signal (unused).
 */
static void
_on_unpressed(void *data,
              Evas_Object *obj EINA_UNUSED,
              const char *emission EINA_UNUSED,
              const char *source EINA_UNUSED)
{
   EFL_UI_PANES_DATA_GET(data, sd);
   evas_object_smart_callback_call(data, "unpress", NULL);
   efl_input_clickable_unpress(data, 1);
   if (sd->double_clicked)
     {
        evas_object_smart_callback_call(data, "clicked,double", NULL);
        sd->double_clicked = EINA_FALSE;
     }
}

/**
 * @internal
 * @brief Calculates the minimum size of the panes widget based on its content.
 *
 * This function is part of the Efl.Canvas.Group interface. It determines the
 * combined minimum size required by the panes widget by considering the minimum
 * sizes of its first and second content objects. The calculation depends on whether
 * the content's own minimum size hint (`efl_gfx_hint_size_min_get`) or its
 * combined minimum size hint (`efl_gfx_hint_size_combined_min_get`) should be used,
 * controlled by `first_hint_min_allow` and `second_hint_min_allow` flags.
 *
 * This calculation is skipped for legacy panes to maintain backward compatibility.
 *
 * @param obj The Evas_Object (panes widget).
 * @param sd The private data of the Efl_Ui_Panes widget.
 */
EOLIAN static void
_efl_ui_panes_efl_canvas_group_group_calculate(Eo *obj, Efl_Ui_Panes_Data *sd)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   Eo *first_content, *second_content;
   Eina_Size2D min;

   /* Legacy panes did not consider its content's min size.
    * Therefore, to keep the backward compatibility, the following calculation
    * is not done for legacy panes. */
   if (elm_widget_is_legacy(obj)) return;

   efl_canvas_group_need_recalculate_set(obj, EINA_FALSE);

   first_content = efl_content_get(efl_part(obj, "first"));
   second_content = efl_content_get(efl_part(obj, "second"));

   if (first_content)
     {
        if (!sd->first_hint_min_allow)
          sd->first_min = efl_gfx_hint_size_combined_min_get(first_content);
        else
          sd->first_min = efl_gfx_hint_size_min_get(first_content);
     }

   if (second_content)
     {
        if (!sd->second_hint_min_allow)
          sd->second_min = efl_gfx_hint_size_combined_min_get(second_content);
        else
          sd->second_min = efl_gfx_hint_size_min_get(second_content);
     }

   if (sd->dir == EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL)
     {
        min.w = MAX(sd->first_min.w, sd->second_min.w);
        min.h = sd->first_min.h + sd->second_min.h;
     }
   else
     {
        min.w = sd->first_min.w + sd->second_min.w;
        min.h = MAX(sd->first_min.h, sd->second_min.h);
     }

   efl_gfx_hint_size_restricted_min_set(obj, min);
   _set_min_size_new(obj);
}

static void
_set_min_size_new(void *data)
{
   Eo *obj = data;
   EFL_UI_PANES_DATA_GET(obj, sd);
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   Eina_Size2D first_min = sd->first_min;
   Eina_Size2D second_min = sd->second_min;
   int w, h;
   double first_min_relative_size = 0.0, second_min_relative_size = 0.0;

   evas_object_geometry_get(wd->resize_obj, NULL, NULL, &w, &h);

   if (sd->dir == EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL)
     {
        if (first_min.h + second_min.h > h)
          {
             first_min_relative_size = first_min.h/(first_min.h + second_min.h);
             second_min_relative_size = second_min.h/(first_min.h + second_min.h);
          }
        else
          {
             if (h > 0)
               {
                  first_min_relative_size = first_min.h/(double)h;
                  second_min_relative_size = second_min.h/(double)h;
               }
          }

        first_min_relative_size = MAX(sd->first_min_split_ratio, first_min_relative_size);
        second_min_relative_size = MAX(sd->second_min_split_ratio, second_min_relative_size);

        edje_object_part_drag_value_set(wd->resize_obj, "right_constraint",
                                        0.0, 1.0 - second_min_relative_size);
        edje_object_part_drag_value_set(wd->resize_obj, "left_constraint",
                                        0.0, first_min_relative_size);
     }
   else
     {
        if (first_min.w + second_min.w > w)
          {
             first_min_relative_size = first_min.w/(first_min.w + second_min.w);
             second_min_relative_size = second_min.w/(first_min.w + second_min.w);
          }
        else
          {
             if (w > 0)
               {
                  first_min_relative_size = first_min.w/(double)w;
                  second_min_relative_size = second_min.w/(double)w;
               }
          }

        first_min_relative_size = MAX(sd->first_min_split_ratio, first_min_relative_size);
        second_min_relative_size = MAX(sd->second_min_split_ratio, second_min_relative_size);

        edje_object_part_drag_value_set(wd->resize_obj, "right_constraint",
                                        1.0 - second_min_relative_size, 0.0);
        edje_object_part_drag_value_set(wd->resize_obj, "left_constraint",
                                        first_min_relative_size, 0.0);
     }
}

/**
 * @brief Sets the minimum size constraints for the panes (legacy API).
 *
 * This function is used by the legacy panes API (`elm_panes_..._min_size_set`
 * and `elm_panes_..._min_relative_size_set`) to apply minimum size constraints
 * to the draggable bar. It uses `left_min_relative_size` and `right_min_relative_size`
 * to configure the drag constraints in the Edje theme.
 *
 * @param data The Evas_Object (panes widget) whose minimum sizes are to be set.
 */
static void
_set_min_size(void *data)
{
   EFL_UI_PANES_DATA_GET(data, sd);
   ELM_WIDGET_DATA_GET_OR_RETURN(data, wd);

   double sizer = sd->right_min_relative_size;
   double sizel = sd->left_min_relative_size;
   if ((sd->left_min_relative_size + sd->right_min_relative_size) > 1)
     {
        double sum = sizer + sizel;
        sizer = sizer / sum;
        sizel = sizel / sum;
     }
   if (sd->dir == EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL)
     {
        edje_object_part_drag_value_set
           (wd->resize_obj, "right_constraint", 0.0, (1 - sizer));
        edje_object_part_drag_value_set
           (wd->resize_obj, "left_constraint", 0.0, sizel);
     }
   else
     {
        edje_object_part_drag_value_set
           (wd->resize_obj, "right_constraint", (1 - sizer), 0.0);
        edje_object_part_drag_value_set
           (wd->resize_obj, "left_constraint", sizel, 0.0);
     }
}

/**
 * @brief Updates the absolute and relative minimum sizes for legacy panes.
 *
 * This function is called for legacy panes when their size or orientation changes,
 * or when minimum sizes are set. It recalculates absolute minimum sizes from
 * relative ones, or vice-versa, depending on which was last set.
 * After updating these values, it calls `_set_min_size` to apply the constraints.
 *
 * @param data The Evas_Object (panes widget) whose fixed sides need updating.
 */
static void
_update_fixed_sides(void *data)
{
   EFL_UI_PANES_DATA_GET(data, sd);
   ELM_WIDGET_DATA_GET_OR_RETURN(data, wd);
   Evas_Coord w, h;
   evas_object_geometry_get(wd->resize_obj, NULL, NULL, &w, &h);

   if (sd->right_min_size_is_relative)
     {
        if (sd->dir == EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL)
           sd->right_min_size = (int)(h * sd->right_min_relative_size);
        else
           sd->right_min_size =(int)(w * sd->right_min_relative_size);
     }
   else
     {
        sd->right_min_relative_size = 0;
        if (sd->dir == EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL && (h > 0))
              sd->right_min_relative_size = sd->right_min_size / (double)h;
        if (sd->dir == EFL_UI_LAYOUT_ORIENTATION_VERTICAL && (w > 0))
              sd->right_min_relative_size = sd->right_min_size / (double)w;
     }

   if(sd->left_min_size_is_relative)
     {
        if (sd->dir == EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL)
             sd->left_min_size = (int)(h * sd->left_min_relative_size);
        else
           sd->left_min_size = (int)(w * sd->left_min_relative_size);
     }
   else
     {
        sd->left_min_relative_size = 0;
        if (sd->dir == EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL && (h > 0))
           sd->left_min_relative_size = sd->left_min_size / (double)h;
        if (sd->dir == EFL_UI_LAYOUT_ORIENTATION_VERTICAL && (w > 0))
           sd->left_min_relative_size = sd->left_min_size / (double)w;
     }

   _set_min_size(data);
}

/**
 * @brief Callback function for the resize event of the panes widget's resize object.
 *
 * This function is triggered when the underlying Edje object (resize_obj) of the
 * panes widget is resized. It calls the appropriate function to update minimum
 * size constraints: `_update_fixed_sides` for legacy panes or `_set_min_size_new`
 * for new API panes.
 *
 * @param data The Evas_Object (panes widget) that was resized.
 * @param e The Evas canvas (unused).
 * @param obj The Evas_Object that triggered the event (unused, same as data).
 * @param event_info Additional event information (unused).
 */
static void
_on_resize(void *data,
           Evas *e EINA_UNUSED,
           Evas_Object *obj EINA_UNUSED,
           void *event_info EINA_UNUSED)
{
  if (elm_widget_is_legacy(data)) _update_fixed_sides(data);
  else _set_min_size_new(data);
}

/**
 * @internal
 * @brief Initializes the Efl.Ui.Panes widget when it's added to a canvas group.
 *
 * This function is part of the Efl.Canvas.Group interface. It performs
 * initial setup for the panes widget, including:
 * - Setting the default theme class and style.
 * - Setting the initial split ratio (0.5).
 * - Connecting signals for click, press, and unpress events from the theme.
 * - Adding a resize callback.
 * - Initializing orientation and minimum size properties.
 * - Creating and configuring an event swallowing rectangle.
 *
 * @param obj The Evas_Object (panes widget).
 * @param _pd The private data of the Efl_Ui_Panes widget (unused).
 */
EOLIAN static void
_efl_ui_panes_efl_canvas_group_group_add(Eo *obj, Efl_Ui_Panes_Data *_pd EINA_UNUSED)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);
   EFL_UI_PANES_DATA_GET(obj, sd);
   char *group;

   if (!elm_widget_theme_klass_get(obj))
     elm_widget_theme_klass_set(obj, "panes");
   efl_canvas_group_add(efl_super(obj, MY_CLASS));

   group = _efl_ui_panes_theme_group_get(obj, sd);
   if (elm_widget_theme_object_set(obj, wd->resize_obj,
                                       elm_widget_theme_klass_get(obj),
                                       group,
                                       elm_widget_theme_style_get(obj)) == EFL_UI_THEME_APPLY_ERROR_GENERIC)
     CRI("Failed to set layout!");

   free(group);

   elm_panes_content_left_size_set(obj, 0.5);

   if (elm_widget_is_legacy(obj))
     {
        edje_object_signal_callback_add
           (wd->resize_obj, "elm,action,click", "*",
            _on_clicked, obj);
        edje_object_signal_callback_add
           (wd->resize_obj, "elm,action,click,double", "*",
            _double_clicked, obj);
        edje_object_signal_callback_add
           (wd->resize_obj, "elm,action,press", "*",
            _on_pressed, obj);
        edje_object_signal_callback_add
           (wd->resize_obj, "elm,action,unpress", "*",
            _on_unpressed, obj);
     }
   else
     {
        efl_ui_action_connector_bind_clickable_to_theme(wd->resize_obj, obj);
     }
   evas_object_event_callback_add
     (wd->resize_obj, EVAS_CALLBACK_RESIZE,
     _on_resize, obj);

   sd->dir = EFL_UI_LAYOUT_ORIENTATION_VERTICAL;
   sd->right_min_size_is_relative = EINA_TRUE;
   sd->left_min_size_is_relative = EINA_TRUE;
   sd->right_min_size = 0;
   sd->left_min_size = 0;
   sd->right_min_relative_size = 0;
   sd->left_min_relative_size = 0;
   if (elm_widget_is_legacy(obj)) _update_fixed_sides(obj);
   else _set_min_size_new(obj);

   elm_widget_can_focus_set(obj, EINA_FALSE);

   sd->event = evas_object_rectangle_add(evas_object_evas_get(obj));
   evas_object_color_set(sd->event, 0, 0, 0, 0);
   evas_object_pass_events_set(sd->event, EINA_TRUE);

   if (elm_widget_is_legacy(obj))
     {
        if (edje_object_part_exists
            (wd->resize_obj, "elm.swallow.event"))
          {
             Evas_Coord minw = 0, minh = 0;

             elm_coords_finger_size_adjust(1, &minw, 1, &minh);
             efl_gfx_hint_size_min_set(sd->event, EINA_SIZE2D(minw, minh));
             elm_layout_content_set(obj, "elm.swallow.event", sd->event);
          }
     }
   else
     {
        if (edje_object_part_exists
            (wd->resize_obj, "efl.event"))
          {
             Evas_Coord minw = 0, minh = 0;

             elm_coords_finger_size_adjust(1, &minw, 1, &minh);
             efl_gfx_hint_size_min_set(sd->event, EINA_SIZE2D(minw, minh));
             elm_layout_content_set(obj, "efl.event", sd->event);
          }
     }
   elm_widget_sub_object_add(obj, sd->event);
}

/**
 * @internal
 * @brief Constructor for the Efl.Ui.Panes widget.
 *
 * This function is called when a new Efl.Ui.Panes object is created.
 * It calls the parent class's constructor, sets up smart callbacks,
 * and sets the accessibility role for the widget.
 *
 * @param obj The Evas_Object (panes widget) being constructed.
 * @param _pd The private data of the Efl_Ui_Panes widget (unused).
 * @return The constructed Evas_Object.
 */
EOLIAN static Eo *
_efl_ui_panes_efl_object_constructor(Eo *obj, Efl_Ui_Panes_Data *_pd EINA_UNUSED)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));
   evas_object_smart_callbacks_descriptions_set(obj, _smart_callbacks);
   efl_access_object_role_set(obj, EFL_ACCESS_ROLE_SPLIT_PANE);

   return obj;
}

/**
 * @internal
 * @brief Gets the current split ratio of the panes.
 *
 * The split ratio determines the proportional size of the first content area
 * relative to the total size of the panes along its orientation.
 * For horizontal panes, it's the height ratio; for vertical panes, it's the width ratio.
 *
 * @param obj The Evas_Object (panes widget).
 * @param sd The private data of the Efl_Ui_Panes widget.
 * @return The current split ratio, a value between 0.0 and 1.0.
 */
EOLIAN static double
_efl_ui_panes_split_ratio_get(const Eo *obj, Efl_Ui_Panes_Data *sd)
{
   double w, h;
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd, 0.0);

   if (elm_widget_is_legacy(obj))
     edje_object_part_drag_value_get(wd->resize_obj, "elm.bar", &w, &h);
   else
     edje_object_part_drag_value_get(wd->resize_obj, "efl.bar", &w, &h);

   if (sd->dir == EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL)
     return h;
   else return w;
}

/**
 * @internal
 * @brief Sets the split ratio of the panes.
 *
 * This function adjusts the position of the draggable bar to achieve the
 * desired @p ratio. The @p ratio is clamped between 0.0 and 1.0.
 *
 * @param obj The Evas_Object (panes widget).
 * @param sd The private data of the Efl_Ui_Panes widget.
 * @param ratio The desired split ratio (0.0 to 1.0).
 *              0.0 means the first content area is minimized.
 *              1.0 means the second content area is minimized.
 */
EOLIAN static void
_efl_ui_panes_split_ratio_set(Eo *obj, Efl_Ui_Panes_Data *sd, double ratio)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   if (ratio < 0.0) ratio = 0.0;
   else if (ratio > 1.0) ratio = 1.0;

   if (sd->dir == EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL)
     {
        if (elm_widget_is_legacy(obj))
          edje_object_part_drag_value_set(wd->resize_obj, "elm.bar", 0.0, ratio);
        else
          edje_object_part_drag_value_set(wd->resize_obj, "efl.bar", 0.0, ratio);
     }
   else
     {
        if (elm_widget_is_legacy(obj))
          edje_object_part_drag_value_set(wd->resize_obj, "elm.bar", ratio, 0.0);
        else
          edje_object_part_drag_value_set(wd->resize_obj, "efl.bar", ratio, 0.0);
     }
}

/**
 * @internal
 * @brief Sets the orientation of the panes widget.
 *
 * This function changes the orientation of the panes to either horizontal or
 * vertical. It preserves the current split ratio, re-applies the theme,
 * and updates minimum size constraints.
 *
 * @param obj The Evas_Object (panes widget).
 * @param sd The private data of the Efl_Ui_Panes widget.
 * @param dir The desired orientation (EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL or
 *            EFL_UI_LAYOUT_ORIENTATION_VERTICAL). Other values are normalized.
 */
EOLIAN static void
_efl_ui_panes_efl_ui_layout_orientable_orientation_set(Eo *obj, Efl_Ui_Panes_Data *sd, Efl_Ui_Layout_Orientation dir)
{
   double size = elm_panes_content_left_size_get(obj);
   if (efl_ui_layout_orientation_is_horizontal(dir, EINA_FALSE))
     dir = EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL;
   else
     dir = EFL_UI_LAYOUT_ORIENTATION_VERTICAL;

   sd->dir = dir;
   efl_ui_widget_theme_apply(obj);
   if (elm_widget_is_legacy(obj)) _update_fixed_sides(obj);
   else _set_min_size_new(obj);

   elm_panes_content_left_size_set(obj, size);
}

/**
 * @internal
 * @brief Gets the current orientation of the panes widget.
 *
 * @param obj The Evas_Object (panes widget) (unused).
 * @param sd The private data of the Efl_Ui_Panes widget.
 * @return The current orientation (EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL or
 *         EFL_UI_LAYOUT_ORIENTATION_VERTICAL).
 */
EOLIAN static Efl_Ui_Layout_Orientation
_efl_ui_panes_efl_ui_layout_orientable_orientation_get(const Eo *obj EINA_UNUSED, Efl_Ui_Panes_Data *sd)
{
   return sd->dir;
}

/**
 * @internal
 * @brief Sets whether the panes bar is fixed (not draggable).
 *
 * When fixed, the bar separating the two content areas cannot be moved by the user.
 * This function emits the appropriate Edje signals ("elm,panes,fixed",
 * "elm,panes,unfixed", "efl,panes,fixed", or "efl,panes,unfixed") to update
 * the visual state of the panes.
 *
 * @param obj The Evas_Object (panes widget).
 * @param sd The private data of the Efl_Ui_Panes widget.
 * @param fixed EINA_TRUE to make the bar fixed, EINA_FALSE to make it draggable.
 */
EOLIAN static void
_efl_ui_panes_fixed_set(Eo *obj, Efl_Ui_Panes_Data *sd, Eina_Bool fixed)
{
   sd->fixed = !!fixed;
   if (sd->fixed == EINA_TRUE)
     {
        if (elm_widget_is_legacy(obj))
          {
             elm_layout_signal_emit(obj, "elm,panes,fixed", "elm");

             //TODO: remove this signal on EFL 2.0.
             // I left this due to the backward compatibility.
             elm_layout_signal_emit(obj, "elm.panes.fixed", "elm");
          }
        else
          elm_layout_signal_emit(obj, "efl,panes,fixed", "efl");
     }
   else
     {
        if (elm_widget_is_legacy(obj))
          {
             elm_layout_signal_emit(obj, "elm,panes,unfixed", "elm");

             //TODO: remove this signal on EFL 2.0.
             // I left this due to the backward compatibility.
             elm_layout_signal_emit(obj, "elm.panes.unfixed", "elm");
          }
        else
          elm_layout_signal_emit(obj, "efl,panes,unfixed", "efl");
     }
}

/**
 * @internal
 * @brief Gets whether the panes bar is fixed.
 *
 * @param obj The Evas_Object (panes widget) (unused).
 * @param sd The private data of the Efl_Ui_Panes widget.
 * @return EINA_TRUE if the bar is fixed, EINA_FALSE if it is draggable.
 */
EOLIAN static Eina_Bool
_efl_ui_panes_fixed_get(const Eo *obj EINA_UNUSED, Efl_Ui_Panes_Data *sd)
{
   return sd->fixed;
}

/* Efl.Part begin */

/**
 * @brief Checks if a given part name is a valid content part for Efl.Ui.Panes.
 *
 * For legacy panes, valid parts are "elm.swallow.left" and "elm.swallow.right".
 * For new API panes, valid parts are "first" and "second".
 *
 * @param obj The Evas_Object (panes widget).
 * @param part The name of the part to check.
 * @return EINA_TRUE if the part name is valid, EINA_FALSE otherwise.
 */
static Eina_Bool
_part_is_efl_ui_panes_part(const Eo *obj, const char *part)
{
   if (elm_widget_is_legacy(obj))
     {
        if ((eina_streq(part, "elm.swallow.left")) || (eina_streq(part, "elm.swallow.right")))
          return EINA_TRUE;
     }

   return (eina_streq(part, "first")) || (eina_streq(part, "second"));
}

ELM_PART_OVERRIDE_PARTIAL(efl_ui_panes, EFL_UI_PANES, Efl_Ui_Panes_Data,
  _part_is_efl_ui_panes_part)

/**
 * @internal
 * @brief Sets whether a content part should use its own minimum size hint or its combined minimum size hint.
 * @see efl_part_hint_min_allow_set
 *
 * This function controls how the minimum size of a content part ("first" or "second")
 * is determined during layout calculations. If @p allow is EINA_TRUE, the part's
 * direct minimum size (`efl_gfx_hint_size_min_get`) is used. If EINA_FALSE,
 * its combined minimum size (`efl_gfx_hint_size_combined_min_get`), which includes
 * the minimum sizes of its children, is used.
 *
 * @param obj The Efl_Ui_Panes_Part object.
 * @param _pd Private data for the part (unused).
 * @param allow EINA_TRUE to use `efl_gfx_hint_size_min_get`, EINA_FALSE to use `efl_gfx_hint_size_combined_min_get`.
 */
EOLIAN static void
_efl_ui_panes_part_hint_min_allow_set(Eo *obj, void *_pd EINA_UNUSED, Eina_Bool allow)
{
   Elm_Part_Data *pd = efl_data_scope_get(obj, EFL_UI_WIDGET_PART_CLASS);
   Efl_Ui_Panes_Data *sd = efl_data_scope_get(pd->obj, EFL_UI_PANES_CLASS);

   if (!strcmp(pd->part, "first"))
     {
        if (sd->first_hint_min_allow == allow) return;
        sd->first_hint_min_allow = allow;
        efl_canvas_group_change(pd->obj);
     }
   else if (!strcmp(pd->part, "second"))
     {
        if (sd->second_hint_min_allow == allow) return;
        sd->second_hint_min_allow = allow;
        efl_canvas_group_change(pd->obj);
     }
}

/**
 * @internal
 * @brief Gets whether a content part uses its own minimum size hint or its combined minimum size hint.
 * @see efl_part_hint_min_allow_get
 *
 * @param obj The Efl_Ui_Panes_Part object.
 * @param _pd Private data for the part (unused).
 * @return EINA_TRUE if `efl_gfx_hint_size_min_get` is used, EINA_FALSE if `efl_gfx_hint_size_combined_min_get` is used.
 */
EOLIAN static Eina_Bool
_efl_ui_panes_part_hint_min_allow_get(const Eo *obj, void *_pd EINA_UNUSED)
{
   Eina_Bool ret = EINA_FALSE;
   Elm_Part_Data *pd = efl_data_scope_get(obj, EFL_UI_WIDGET_PART_CLASS);
   Efl_Ui_Panes_Data *sd = efl_data_scope_get(pd->obj, EFL_UI_PANES_CLASS);

   if (!strcmp(pd->part, "first"))
     {
        ret = sd->first_hint_min_allow;
     }
   else if (!strcmp(pd->part, "second"))
     {
        ret = sd->second_hint_min_allow;
     }

   return ret;
}

/**
 * @internal
 * @brief Gets the minimum split ratio for a content part.
 * @see efl_part_split_ratio_min_get
 *
 * The minimum split ratio defines the smallest proportion of the panes that
 * this content part ("first" or "second") can occupy.
 *
 * @param obj The Efl_Ui_Panes_Part object.
 * @param _pd Private data for the part (unused).
 * @return The minimum split ratio for the part (0.0 to 1.0).
 */
EOLIAN static double
_efl_ui_panes_part_split_ratio_min_get(const Eo *obj, void *_pd EINA_UNUSED)
{
   Elm_Part_Data *pd = efl_data_scope_get(obj, EFL_UI_WIDGET_PART_CLASS);
   Efl_Ui_Panes_Data *sd = efl_data_scope_get(pd->obj, EFL_UI_PANES_CLASS);
   double ret = 0.0;

   if (!strcmp(pd->part, "first"))
     ret = sd->first_min_split_ratio;
   else if (!strcmp(pd->part, "second"))
     ret = sd->second_min_split_ratio;

   return ret;
}

/**
 * @internal
 * @brief Sets the minimum split ratio for a content part.
 * @see efl_part_split_ratio_min_set
 *
 * This function sets the smallest proportion of the panes that this content part
 * ("first" or "second") can occupy. The @p ratio is clamped to be non-negative.
 * After setting, it triggers an update of the panes' minimum size constraints.
 *
 * @param obj The Efl_Ui_Panes_Part object.
 * @param _pd Private data for the part (unused).
 * @param ratio The minimum split ratio to set (0.0 to 1.0).
 */
EOLIAN static void
_efl_ui_panes_part_split_ratio_min_set(Eo *obj, void *_pd EINA_UNUSED, double ratio)
{
   Elm_Part_Data *pd = efl_data_scope_get(obj, EFL_UI_WIDGET_PART_CLASS);
   Efl_Ui_Panes_Data *sd = efl_data_scope_get(pd->obj, EFL_UI_PANES_CLASS);

   if (!strcmp(pd->part, "first"))
     {
        sd->first_min_split_ratio = ratio;
        if (sd->first_min_split_ratio < 0) sd->first_min_split_ratio = 0;
        _set_min_size_new(pd->obj);
     }
   else if (!strcmp(pd->part, "second"))
     {
        sd->second_min_split_ratio = ratio;
        if (sd->second_min_split_ratio < 0) sd->second_min_split_ratio = 0;
        _set_min_size_new(pd->obj);
     }
}

#include "efl_ui_panes_part.eo.c"

/* Efl.Part end */

/* Internal EO APIs and hidden overrides */

EFL_UI_LAYOUT_CONTENT_ALIASES_IMPLEMENT(efl_ui_panes)

#define EFL_UI_PANES_EXTRA_OPS \
   EFL_CANVAS_GROUP_ADD_OPS(efl_ui_panes), \
   EFL_UI_LAYOUT_CONTENT_ALIASES_OPS(efl_ui_panes)

#include "efl_ui_panes.eo.c"
#include "efl_ui_panes_eo.legacy.c"

#include "efl_ui_panes_legacy_eo.h"
#define MY_CLASS_NAME_LEGACY "elm_panes"

/**
 * @brief Legacy class constructor for elm_panes.
 *
 * Registers the legacy "elm_panes" type with the Evas smart object system.
 *
 * @param klass The Efl_Class to construct.
 */
static void
_efl_ui_panes_legacy_class_constructor(Efl_Class *klass)
{
   evas_smart_legacy_type_register(MY_CLASS_NAME_LEGACY, klass);
}

/**
 * @internal
 * @brief Constructor for the legacy elm_panes widget.
 *
 * This function is called when a new legacy elm_panes object is created.
 * It calls the parent class's constructor and sets the Evas object type
 * to "elm_panes".
 *
 * @param obj The Evas_Object (legacy panes widget) being constructed.
 * @param _pd Private data (unused).
 * @return The constructed Evas_Object.
 */
EOLIAN static Eo *
_efl_ui_panes_legacy_efl_object_constructor(Eo *obj, void *_pd EINA_UNUSED)
{
   obj = efl_constructor(efl_super(obj, EFL_UI_PANES_LEGACY_CLASS));
   efl_canvas_object_type_set(obj, MY_CLASS_NAME_LEGACY);
   return obj;
}

/**
 * @brief Adds a new panes widget.
 * @param parent The parent object.
 * @return The new panes object, or @c NULL on errors.
 * @ingroup Elm_Panes_Group
 * @see efl_ui_panes_add()
 */
EAPI Evas_Object *
elm_panes_add(Evas_Object *parent)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);
   return elm_legacy_add(EFL_UI_PANES_LEGACY_CLASS, parent);
}

/**
 * @brief Set the minimum size for the left content object.
 *
 * This sets the minimum size for the left content object.
 * If this size is not respected, the objects minimum size will be used.
 * This is the legacy (pre-Eo) API.
 *
 * @param obj The panes object.
 * @param size The minimum size (in pixels).
 * @ingroup Elm_Panes_Group
 * @deprecated Use efl_part_split_ratio_min_set(efl_part(obj, "first"), ratio) instead.
 */
EAPI void
elm_panes_content_left_min_size_set(Evas_Object *obj, int size)
{
   EFL_UI_PANES_DATA_GET_OR_RETURN(obj, sd);

   sd->left_min_size = size;
   if (sd->left_min_size < 0) sd->left_min_size = 0;
   sd->left_min_size_is_relative = EINA_FALSE;
   _update_fixed_sides(obj);
}

/**
 * @brief Get the minimum size for the left content object.
 *
 * This is the legacy (pre-Eo) API.
 *
 * @param obj The panes object.
 * @return The minimum size (in pixels).
 * @ingroup Elm_Panes_Group
 * @deprecated Use efl_part_split_ratio_min_get(efl_part(obj, "first")) instead.
 */
EAPI int
elm_panes_content_left_min_size_get(const Evas_Object *obj)
{
   EFL_UI_PANES_DATA_GET_OR_RETURN_VAL(obj, sd, 0);
   return sd->left_min_size;
}

/**
 * @brief Set the minimum size for the right content object.
 *
 * This sets the minimum size for the right content object.
 * If this size is not respected, the objects minimum size will be used.
 * This is the legacy (pre-Eo) API.
 *
 * @param obj The panes object.
 * @param size The minimum size (in pixels).
 * @ingroup Elm_Panes_Group
 * @deprecated Use efl_part_split_ratio_min_set(efl_part(obj, "second"), ratio) instead.
 */
EAPI void
elm_panes_content_right_min_size_set(Evas_Object *obj, int size)
{
   EFL_UI_PANES_DATA_GET_OR_RETURN(obj, sd);

   sd->right_min_size = size;
   if (sd->right_min_size < 0) sd->right_min_size = 0;
   sd->right_min_size_is_relative = EINA_FALSE;
   _update_fixed_sides(obj);
}

/**
 * @brief Get the minimum size for the right content object.
 *
 * This is the legacy (pre-Eo) API.
 *
 * @param obj The panes object.
 * @return The minimum size (in pixels).
 * @ingroup Elm_Panes_Group
 * @deprecated Use efl_part_split_ratio_min_get(efl_part(obj, "second")) instead.
 */
EAPI int
elm_panes_content_right_min_size_get(const Evas_Object *obj)
{
   EFL_UI_PANES_DATA_GET_OR_RETURN_VAL(obj, sd, 0);
   return sd->right_min_size;
}

/**
 * @brief Get the size proportion of the left content object.
 *
 * This is the legacy (pre-Eo) API.
 *
 * @param obj The panes object.
 * @return The size proportion, from 0.0 to 1.0.
 * @ingroup Elm_Panes_Group
 * @see efl_ui_panes_split_ratio_get()
 */
EAPI double
elm_panes_content_left_size_get(const Evas_Object *obj)
{
   return efl_ui_panes_split_ratio_get(obj);
}

/**
 * @brief Set the size proportion of the left content object.
 *
 * This is the legacy (pre-Eo) API.
 *
 * @param obj The panes object.
 * @param size The size proportion, from 0.0 to 1.0.
 *             0.0 means the left content is minimized.
 *             1.0 means the right content is minimized.
 * @ingroup Elm_Panes_Group
 * @see efl_ui_panes_split_ratio_set()
 */
EAPI void
elm_panes_content_left_size_set(Evas_Object *obj, double size)
{
   efl_ui_panes_split_ratio_set(obj, size);
}

/**
 * @brief Get the size proportion of the right content object.
 *
 * This is the legacy (pre-Eo) API.
 *
 * @param obj The panes object.
 * @return The size proportion, from 0.0 to 1.0.
 * @ingroup Elm_Panes_Group
 * @see efl_ui_panes_split_ratio_get() (returns left size, right is 1.0 - left)
 */
EAPI double
elm_panes_content_right_size_get(const Evas_Object *obj)
{
   EFL_UI_PANES_CHECK(obj) 0.0;

   return 1.0 - elm_panes_content_left_size_get(obj);
}

/**
 * @brief Set the size proportion of the right content object.
 *
 * This is the legacy (pre-Eo) API.
 *
 * @param obj The panes object.
 * @param size The size proportion, from 0.0 to 1.0.
 *             0.0 means the right content is minimized.
 *             1.0 means the left content is minimized.
 * @ingroup Elm_Panes_Group
 * @see efl_ui_panes_split_ratio_set() (takes left size, so use 1.0 - size)
 */
EAPI void
elm_panes_content_right_size_set(Evas_Object *obj, double size)
{
   elm_panes_content_left_size_set(obj, (1.0 - size));
}

/**
 * @brief Set the minimum relative size for the left content object.
 *
 * This sets the minimum relative size for the left content object.
 * This is the legacy (pre-Eo) API.
 *
 * @param obj The panes object.
 * @param size The minimum relative size (0.0 to 1.0).
 * @ingroup Elm_Panes_Group
 * @deprecated Use efl_part_split_ratio_min_set(efl_part(obj, "first"), ratio) instead.
 */
EAPI void
elm_panes_content_left_min_relative_size_set(Evas_Object *obj, double size)
{
   EFL_UI_PANES_DATA_GET_OR_RETURN(obj, sd);
   sd->left_min_relative_size = size;
   if (sd->left_min_relative_size < 0) sd->left_min_relative_size = 0;
   sd->left_min_size_is_relative = EINA_TRUE;
   _update_fixed_sides(obj);
}

/**
 * @brief Get the minimum relative size for the left content object.
 *
 * This is the legacy (pre-Eo) API.
 *
 * @param obj The panes object.
 * @return The minimum relative size (0.0 to 1.0).
 * @ingroup Elm_Panes_Group
 * @deprecated Use efl_part_split_ratio_min_get(efl_part(obj, "first")) instead.
 */
EAPI double
elm_panes_content_left_min_relative_size_get(const Evas_Object *obj)
{
   EFL_UI_PANES_DATA_GET_OR_RETURN_VAL(obj, sd, 0.0);
   return sd->left_min_relative_size;
}

/**
 * @brief Set the minimum relative size for the right content object.
 *
 * This sets the minimum relative size for the right content object.
 * This is the legacy (pre-Eo) API.
 *
 * @param obj The panes object.
 * @param size The minimum relative size (0.0 to 1.0).
 * @ingroup Elm_Panes_Group
 * @deprecated Use efl_part_split_ratio_min_set(efl_part(obj, "second"), ratio) instead.
 */
EAPI void
elm_panes_content_right_min_relative_size_set(Evas_Object *obj, double size)
{
   EFL_UI_PANES_DATA_GET_OR_RETURN(obj, sd);

   sd->right_min_relative_size = size;
   if (sd->right_min_relative_size < 0) sd->right_min_relative_size = 0;
   sd->right_min_size_is_relative = EINA_TRUE;
   _update_fixed_sides(obj);
}

/**
 * @brief Get the minimum relative size for the right content object.
 *
 * This is the legacy (pre-Eo) API.
 *
 * @param obj The panes object.
 * @return The minimum relative size (0.0 to 1.0).
 * @ingroup Elm_Panes_Group
 * @deprecated Use efl_part_split_ratio_min_get(efl_part(obj, "second")) instead.
 */
EAPI double
elm_panes_content_right_min_relative_size_get(const Evas_Object *obj)
{
   EFL_UI_PANES_DATA_GET_OR_RETURN_VAL(obj, sd, 0.0);
   return sd->right_min_relative_size;
}

/**
 * @brief Set the orientation of the panes widget.
 *
 * This is the legacy (pre-Eo) API.
 *
 * @param obj The panes object.
 * @param horizontal If @c EINA_TRUE, the panes are split horizontally (left and right panes).
 *                   If @c EINA_FALSE, the panes are split vertically (top and bottom panes).
 * @ingroup Elm_Panes_Group
 * @see efl_ui_layout_orientation_set()
 */
EAPI void
elm_panes_horizontal_set(Evas_Object *obj, Eina_Bool horizontal)
{
   EFL_UI_PANES_CHECK(obj);

   Efl_Ui_Layout_Orientation dir;

   if (horizontal)
     dir = EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL;
   else
     dir = EFL_UI_LAYOUT_ORIENTATION_VERTICAL;

   efl_ui_layout_orientation_set(obj, dir);
}

/**
 * @brief Get the orientation of the panes widget.
 *
 * This is the legacy (pre-Eo) API.
 *
 * @param obj The panes object.
 * @return @c EINA_TRUE if the panes are split horizontally, @c EINA_FALSE otherwise.
 * @ingroup Elm_Panes_Group
 * @see efl_ui_layout_orientation_get()
 */
EAPI Eina_Bool
elm_panes_horizontal_get(const Evas_Object *obj)
{
   EFL_UI_PANES_CHECK(obj) EINA_FALSE;

   Efl_Ui_Layout_Orientation dir = efl_ui_layout_orientation_get(obj);

   if (dir == EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL)
     return EINA_TRUE;

   return EINA_FALSE;
}

/**
 * @brief Set the content of the left (or top) pane.
 *
 * @deprecated Use efl_content_set(efl_part(obj, "first"), content) instead.
 * @param obj The panes object.
 * @param content The content object to set.
 * @ingroup Elm_Panes_Group
 */
EINA_DEPRECATED EAPI void
elm_panes_content_left_set(Evas_Object *obj,
                           Evas_Object *content)
{
   elm_layout_content_set(obj, "left", content);
}

/**
 * @brief Set the content of the right (or bottom) pane.
 *
 * @deprecated Use efl_content_set(efl_part(obj, "second"), content) instead.
 * @param obj The panes object.
 * @param content The content object to set.
 * @ingroup Elm_Panes_Group
 */
EINA_DEPRECATED EAPI void
elm_panes_content_right_set(Evas_Object *obj,
                            Evas_Object *content)
{
   elm_layout_content_set(obj, "right", content);
}

/**
 * @brief Get the content of the left (or top) pane.
 *
 * @deprecated Use efl_content_get(efl_part(obj, "first")) instead.
 * @param obj The panes object.
 * @return The content object, or @c NULL if none.
 * @ingroup Elm_Panes_Group
 */
EINA_DEPRECATED EAPI Evas_Object *
elm_panes_content_left_get(const Evas_Object *obj)
{
   return elm_layout_content_get(obj, "left");
}

/**
 * @brief Get the content of the right (or bottom) pane.
 *
 * @deprecated Use efl_content_get(efl_part(obj, "second")) instead.
 * @param obj The panes object.
 * @return The content object, or @c NULL if none.
 * @ingroup Elm_Panes_Group
 */
EINA_DEPRECATED EAPI Evas_Object *
elm_panes_content_right_get(const Evas_Object *obj)
{
   return elm_layout_content_get(obj, "right");
}

/**
 * @brief Unset (remove) the content of the left (or top) pane.
 *
 * @deprecated Use efl_content_unset(efl_part(obj, "first")) instead.
 * @param obj The panes object.
 * @return The previously set content object, or @c NULL if none.
 * @ingroup Elm_Panes_Group
 */
EINA_DEPRECATED EAPI Evas_Object *
elm_panes_content_left_unset(Evas_Object *obj)
{
   return elm_layout_content_unset(obj, "left");
}

/**
 * @brief Unset (remove) the content of the right (or bottom) pane.
 *
 * @deprecated Use efl_content_unset(efl_part(obj, "second")) instead.
 * @param obj The panes object.
 * @return The previously set content object, or @c NULL if none.
 * @ingroup Elm_Panes_Group
 */
EINA_DEPRECATED EAPI Evas_Object *
elm_panes_content_right_unset(Evas_Object *obj)
{
   return elm_layout_content_unset(obj, "right");
}

/**
 * @} // End of Efl_Ui_Panes_Group
 */
#include "efl_ui_panes_legacy_eo.c"
