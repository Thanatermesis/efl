/**
 * @file
 * @brief These routines are for the Efl Ui Progressbar widget.
 *
 * The progressbar widget is used to display progress, usually for a
 * task that will take some time to complete. It can be horizontal or
 * vertical, and can have a label and an icon.
 *
 * The progress value can be set, and the widget will update its
 * display accordingly. It also supports an "infinite" or "pulse" mode,
 * where the progressbar animates to indicate that an operation of
 * unknown duration is in progress.
 *
 * Parts of the progressbar can be individually controlled, such as
 * "efl.cur.progressbar" for the main progress indicator.
 */

#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define EFL_ACCESS_OBJECT_PROTECTED
#define ELM_LAYOUT_PROTECTED
#define EFL_PART_PROTECTED
#define EFL_UI_FORMAT_PROTECTED

#include <Elementary.h>

#include "elm_priv.h"
#include "efl_ui_progressbar_private.h"
#include "elm_widget_layout.h"

#include "efl_ui_progressbar_part.eo.h"
#include "elm_part_helper.h"

#define MY_CLASS EFL_UI_PROGRESSBAR_CLASS
#define MY_CLASS_PFX efl_ui_progressbar

#define MY_CLASS_NAME "Efl.Ui.Progressbar"

static const char SIG_CHANGED[] = "changed";

#define MIN_RATIO_LVL 0.0
#define MAX_RATIO_LVL 1.0

/* smart callbacks coming from elm progressbar objects (besides the
 * ones coming from elm layout): */
static const Evas_Smart_Cb_Description _smart_callbacks[] = {
   {SIG_CHANGED, ""},
   {SIG_WIDGET_LANG_CHANGED, ""}, /**< handled by elm_widget */
   {SIG_WIDGET_ACCESS_CHANGED, ""}, /**< handled by elm_widget */
   {SIG_LAYOUT_FOCUSED, ""}, /**< handled by elm_layout */
   {SIG_LAYOUT_UNFOCUSED, ""}, /**< handled by elm_layout */
   {NULL, NULL}
};

/**
 * @brief Aliases for content parts of the progressbar.
 * @details This maps the "icon" part name to the theme part "elm.swallow.content".
 */
static const Elm_Layout_Part_Alias_Description _content_aliases[] =
{
   {"icon", "elm.swallow.content"},
   {NULL, NULL}
};

/**
 * @brief Creates a new progress status structure.
 * @param part_name The name of the progressbar part.
 * @param val The initial value for this part.
 * @param exists Boolean indicating if the part currently exists in the theme.
 * @return A pointer to the newly allocated Efl_Ui_Progress_Status, or NULL on failure.
 * @internal
 */
static Efl_Ui_Progress_Status *
_progress_status_new(const char *part_name, double val, Eina_Bool exists)
{
   Efl_Ui_Progress_Status *ps;
   ps = calloc(1, sizeof(Efl_Ui_Progress_Status));
   if (!ps) return NULL;
   ps->part_name = eina_stringshare_add(part_name);
   ps->val = val;
   ps->part_exists = exists;
   return ps;
}

/**
 * @brief Frees a progress status structure.
 * @param ps Pointer to the Efl_Ui_Progress_Status to free.
 * @internal
 */
static inline void
_progress_status_free(Efl_Ui_Progress_Status *ps)
{
   eina_stringshare_del(ps->part_name);
   free(ps);
}

/**
 * @brief Checks if the given orientation is horizontal.
 * @param dir The layout orientation.
 * @return EINA_TRUE if horizontal, EINA_FALSE otherwise.
 * @internal
 */
static inline Eina_Bool
_is_horizontal(Efl_Ui_Layout_Orientation dir)
{
   return efl_ui_layout_orientation_is_horizontal(dir, EINA_TRUE);
}

/**
 * @brief Sets the text of the progressbar's unit/status label.
 * @param obj The progressbar object.
 * @internal
 *
 * This function updates the label that can display the progress percentage
 * or a custom format string. It handles legacy and non-legacy modes.
 */
static void
_units_set(Evas_Object *obj)
{
   EFL_UI_PROGRESSBAR_DATA_GET(obj, sd);

   if (sd->show_progress_label)
     {
        Eina_Value val;

        eina_value_setup(&val, EINA_VALUE_TYPE_DOUBLE);
        eina_value_set(&val, sd->val);

        // Keeping this bug since the legacy code was like that.
        if (sd->is_legacy_format_string && !sd->is_legacy_format_cb)
          eina_value_set(&val, 100 * sd->val);

        if (!sd->format_strbuf) sd->format_strbuf = eina_strbuf_new();
        efl_ui_format_formatted_value_get(obj, sd->format_strbuf, val);

        eina_value_flush(&val);

        if (!sd->has_status_text_part) return;

        if (elm_widget_is_legacy(obj))
          elm_layout_text_set(obj, "elm.text.status", eina_strbuf_string_get(sd->format_strbuf));
        else
          elm_layout_text_set(obj, "efl.text.status", eina_strbuf_string_get(sd->format_strbuf));
     }
   else if (sd->has_status_text_part)
     {
        if (elm_widget_is_legacy(obj))
          elm_layout_text_set(obj, "elm.text.status", NULL);
        else
          elm_layout_text_set(obj, "efl.text.status", NULL);
     }
}

/**
 * @brief Sets the visual position of the progressbar based on its value.
 * @param obj The progressbar object.
 * @internal
 *
 * This function iterates through all registered progress parts and updates
 * their visual representation (e.g., the length of the bar) based on their
 * current value, min, and max. It also considers mirroring and inverted
 * orientation.
 */
static void
_val_set(Evas_Object *obj)
{
   double pos;
   Efl_Ui_Progress_Status *ps;
   Eina_List *l;

   EFL_UI_PROGRESSBAR_DATA_GET(obj, sd);
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   EINA_LIST_FOREACH(sd->progress_status, l, ps)
     {
        if (EINA_DBL_EQ(ps->val_max, ps->val_min))
          {
             WRN("progressbar min and max are equal.");
             continue;
          }
        if (!ps->part_exists) continue;
        pos = (ps->val - ps->val_min)/(ps->val_max - ps->val_min);

        if (efl_ui_mirrored_get(obj) ^ efl_ui_layout_orientation_is_inverted(sd->dir))
          pos = MAX_RATIO_LVL - pos;

        edje_object_part_drag_value_set
              (wd->resize_obj, ps->part_name, pos, pos);
     }
}

/**
 * @brief Synchronizes the widget's theme class based on its orientation.
 * @param obj The progressbar object.
 * @param pd The private data of the progressbar.
 * @internal
 *
 * Sets the theme element to "horizontal" or "vertical".
 */
static void
_sync_widget_theme_klass(Eo *obj, Efl_Ui_Progressbar_Data *pd)
{
   if (efl_ui_layout_orientation_is_horizontal(pd->dir, EINA_TRUE))
     elm_widget_theme_element_set(obj, "horizontal");
   else
     elm_widget_theme_element_set(obj, "vertical");
}

/**
 * @brief Applies the theme to the progressbar widget.
 * @param obj The progressbar object.
 * @param sd The private data of the progressbar.
 * @return Eina_Error indicating success or failure.
 * @internal
 *
 * This function is called when the theme needs to be reapplied, for example,
 * when the orientation changes or the widget is first created. It sets up
 * signals for pulse mode, unit visibility, and inverted state. It also
 * updates the spacer size and re-checks for part existence.
 */
EOLIAN static Eina_Error
_efl_ui_progressbar_efl_ui_widget_theme_apply(Eo *obj, Efl_Ui_Progressbar_Data *sd)
{
   Eina_Error int_ret = EFL_UI_THEME_APPLY_ERROR_GENERIC;
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd, EFL_UI_THEME_APPLY_ERROR_GENERIC);

   const char *statuspart[] =
   {
     "efl.text.status",
     "elm.text.status",
   };
   const char *curprogresspart[] =
   {
     "efl.cur.progressbar",
     "elm.cur.progressbar",
   };
   _sync_widget_theme_klass(obj, sd);

   int_ret = efl_ui_widget_theme_apply(efl_super(obj, MY_CLASS));
   if (int_ret == EFL_UI_THEME_APPLY_ERROR_GENERIC) return int_ret;

   if (elm_widget_is_legacy(obj))
     {
        if (sd->pulse)
          elm_layout_signal_emit(obj, "elm,state,pulse", "elm");
        else
          elm_layout_signal_emit(obj, "elm,state,fraction", "elm");

        if (sd->pulse_state)
          elm_layout_signal_emit(obj, "elm,state,pulse,start", "elm");

        if (sd->show_progress_label && (!sd->pulse))
          elm_layout_signal_emit(obj, "elm,state,units,visible", "elm");
     }
   else
     {
        if (sd->pulse)
          elm_layout_signal_emit(obj, "efl,state,pulse", "efl");
        else
          elm_layout_signal_emit(obj, "efl,state,fraction", "efl");

        if (sd->pulse_state)
          elm_layout_signal_emit(obj, "efl,state,pulse,start", "efl");

        if (sd->show_progress_label && (!sd->pulse))
          elm_layout_signal_emit(obj, "efl,state,units,visible", "efl");
     }
   sd->has_status_text_part = edje_object_part_exists(obj, statuspart[elm_widget_is_legacy(obj)]);
   sd->has_cur_progressbar_part = edje_object_part_exists(obj, curprogresspart[elm_widget_is_legacy(obj)]);

   if (_is_horizontal(sd->dir))
     efl_gfx_hint_size_min_set
       (sd->spacer, EINA_SIZE2D((double)sd->size * efl_gfx_entity_scale_get(obj) *
       elm_config_scale_get(), 1));
   else
     efl_gfx_hint_size_min_set
       (sd->spacer, EINA_SIZE2D(1, (double)sd->size * efl_gfx_entity_scale_get(obj) *
       elm_config_scale_get()));

   if (elm_widget_is_legacy(obj))
     {
        if (efl_ui_layout_orientation_is_inverted(sd->dir))
          elm_layout_signal_emit(obj, "elm,state,inverted,on", "elm");
        else
          elm_layout_signal_emit(obj, "elm,state,inverted,off", "elm");
     }
   else
     {
        if (efl_ui_layout_orientation_is_inverted(sd->dir))
          elm_layout_signal_emit(obj, "efl,state,inverted,on", "efl");
        else
          elm_layout_signal_emit(obj, "efl,state,inverted,off", "efl");
     }

   {
    Efl_Ui_Progress_Status *ps;
    const Eina_List *l;
    EINA_LIST_FOREACH(sd->progress_status, l, ps)
      ps->part_exists = edje_object_part_exists(obj, ps->part_name);
   }

   _units_set(obj);
   _val_set(obj);

   edje_object_message_signal_process(wd->resize_obj);

   if (elm_widget_is_legacy(obj))
     elm_layout_content_set(obj, "elm.swallow.bar", sd->spacer);
   else
     elm_layout_content_set(obj, "efl.bar", sd->spacer);

   return int_ret;
}

/**
 * @brief Accessibility callback to get information about the progressbar.
 * @param data User data (unused).
 * @param obj The progressbar object.
 * @return A newly allocated string with accessibility information, or NULL.
 *         The caller is responsible for freeing the returned string.
 * @internal
 *
 * Returns the custom access info if set, otherwise the main text of the layout.
 */
static char *
_access_info_cb(void *data EINA_UNUSED, Evas_Object *obj)
{
   const char *txt = elm_widget_access_info_get(obj);

   if (!txt) txt = elm_layout_text_get(obj, NULL);
   if (txt) return strdup(txt);

   return NULL;
}

/**
 * @brief Accessibility callback to get the state of the progressbar.
 * @param data User data (unused).
 * @param obj The progressbar object.
 * @return A newly allocated string with the progressbar's state (e.g., value, disabled),
 *         or NULL. The caller is responsible for freeing the returned string.
 * @internal
 *
 * Provides the current formatted value and disabled state for accessibility services.
 */
static char *
_access_state_cb(void *data EINA_UNUSED, Evas_Object *obj)
{
   char *ret;
   Eina_Strbuf *buf;
   buf = eina_strbuf_new();

   const char *txt = NULL;
   EFL_UI_PROGRESSBAR_DATA_GET(obj, sd);
   if (sd->format_strbuf)
     txt = eina_strbuf_string_get(sd->format_strbuf);

   if (txt) eina_strbuf_append(buf, txt);

   if (elm_widget_disabled_get(obj))
     eina_strbuf_append(buf, " state: disabled");

   if (eina_strbuf_length_get(buf))
     {
        ret = eina_strbuf_string_steal(buf);
        eina_strbuf_free(buf);
        return ret;
     }

   eina_strbuf_free(buf);
   return NULL;
}

/**
 * @brief Called when the progressbar object is added to a canvas group.
 * @param obj The progressbar object.
 * @param priv The private data of the progressbar.
 * @internal
 *
 * Initializes default values for the progressbar, such as orientation,
 * value, format string, and spacer object. It also sets up accessibility.
 */
EOLIAN static void
_efl_ui_progressbar_efl_canvas_group_group_add(Eo *obj, Efl_Ui_Progressbar_Data *priv)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   efl_canvas_group_add(efl_super(obj, MY_CLASS));

   efl_ui_layout_finger_size_multiplier_set(obj, 0, 0);

   priv->dir = EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL;
   priv->val = MIN_RATIO_LVL;
   priv->val_max = 1.0;

   efl_ui_format_string_set(obj, "%.0f%%", EFL_UI_FORMAT_STRING_TYPE_SIMPLE);

   priv->spacer = evas_object_rectangle_add(evas_object_evas_get(obj));
   evas_object_color_set(priv->spacer, 0, 0, 0, 0);
   evas_object_pass_events_set(priv->spacer, EINA_TRUE);

   _units_set(obj);
   _val_set(obj);

   if (_elm_config->access_mode == ELM_ACCESS_MODE_ON)
     elm_widget_can_focus_set(obj, EINA_TRUE);

   _elm_access_object_register(obj, wd->resize_obj);
   _elm_access_text_set
     (_elm_access_info_get(obj), ELM_ACCESS_TYPE, E_("progressbar"));
   _elm_access_callback_set
     (_elm_access_info_get(obj), ELM_ACCESS_INFO, _access_info_cb, NULL);
   _elm_access_callback_set
     (_elm_access_info_get(obj), ELM_ACCESS_STATE, _access_state_cb, NULL);
}

/**
 * @brief Called when the progressbar object is being deleted from a canvas group.
 * @param obj The progressbar object.
 * @param sd The private data of the progressbar.
 * @internal
 *
 * Frees resources associated with the progressbar, such as the list of
 * progress statuses and the format string buffer.
 */
EOLIAN static void
_efl_ui_progressbar_efl_canvas_group_group_del(Eo *obj, Efl_Ui_Progressbar_Data *sd)
{
   Efl_Ui_Progress_Status *progress_obj;

   if (sd->progress_status)
      {
         EINA_LIST_FREE(sd->progress_status, progress_obj)
           {
              _progress_status_free(progress_obj);
           }
      }

   eina_strbuf_free(sd->format_strbuf);
   sd->format_strbuf = NULL;

   efl_canvas_group_del(efl_super(obj, MY_CLASS));
}

/**
 * @brief Constructor for the Efl.Ui.Progressbar object.
 * @param obj The progressbar object being constructed.
 * @param _pd Private data (unused in this function directly).
 * @return The constructed object.
 * @internal
 *
 * Sets the default theme class, smart callbacks, accessibility role,
 * initial range limits, and enables the progress label by default.
 */
EOLIAN static Eo *
_efl_ui_progressbar_efl_object_constructor(Eo *obj, Efl_Ui_Progressbar_Data *_pd EINA_UNUSED)
{
   if (!elm_widget_theme_klass_get(obj))
     elm_widget_theme_klass_set(obj, "progressbar");

   obj = efl_constructor(efl_super(obj, MY_CLASS));
   evas_object_smart_callbacks_descriptions_set(obj, _smart_callbacks);
   efl_access_object_role_set(obj, EFL_ACCESS_ROLE_PROGRESS_BAR);
   efl_ui_range_limits_set(obj, 0.0, 1.0);
   efl_ui_progressbar_show_progress_label_set(obj, EINA_TRUE);
   return obj;
}

/**
 * @brief Sets the orientation of the progressbar.
 * @param obj The progressbar object.
 * @param sd The private data of the progressbar.
 * @param dir The new orientation (horizontal or vertical, possibly inverted).
 * @internal
 *
 * If the orientation changes, it updates the internal direction state and
 * reapplies the theme to reflect the change.
 */
EOLIAN static void
_efl_ui_progressbar_efl_ui_layout_orientable_orientation_set(Eo *obj, Efl_Ui_Progressbar_Data *sd, Efl_Ui_Layout_Orientation dir)
{
   if (sd->dir == dir) return;

   sd->dir = dir;

   efl_ui_widget_theme_apply(obj);
}

/**
 * @brief Gets the orientation of the progressbar.
 * @param obj The progressbar object (unused).
 * @param sd The private data of the progressbar.
 * @return The current orientation.
 * @internal
 */
EOLIAN static Efl_Ui_Layout_Orientation
_efl_ui_progressbar_efl_ui_layout_orientable_orientation_get(const Eo *obj EINA_UNUSED, Efl_Ui_Progressbar_Data *sd)
{
   return sd->dir;
}

/**
 * @brief Sets the span size of the progressbar.
 * @param obj The progressbar object.
 * @param sd The private data of the progressbar.
 * @param size The new span size (width for vertical, height for horizontal).
 * @internal
 *
 * This function updates the minimum size of the internal spacer element,
 * which effectively controls the "thickness" of the progressbar.
 */
static void
_progressbar_span_size_set(Eo *obj, Efl_Ui_Progressbar_Data *sd, Evas_Coord size)
{
   if (sd->size == size) return;

   sd->size = size;

   if (_is_horizontal(sd->dir))
     efl_gfx_hint_size_min_set
       (sd->spacer, EINA_SIZE2D((double)sd->size * efl_gfx_entity_scale_get(obj) *
       elm_config_scale_get(), 1));
   else
     efl_gfx_hint_size_min_set
       (sd->spacer, EINA_SIZE2D(1, (double)sd->size * efl_gfx_entity_scale_get(obj) *
       elm_config_scale_get()));

   efl_canvas_group_change(obj);
}

/**
 * @brief Sets the minimum and maximum values for a specific progressbar part.
 * @param obj The progressbar object.
 * @param sd The private data of the progressbar.
 * @param part_name The name of the part to modify (e.g., "efl.cur.progressbar").
 * @param min The new minimum value.
 * @param max The new maximum value.
 * @internal
 *
 * Updates or creates a progress status entry for the given part with the new
 * min/max values. If the part is the main progressbar part ("efl.cur.progressbar"
 * or "elm.cur.progressbar"), it also updates the overall min/max in the private data.
 */
static void
_progress_part_min_max_set(Eo *obj, Efl_Ui_Progressbar_Data *sd, const char *part_name, double min, double max)
{
   Efl_Ui_Progress_Status *ps;
   Eina_Bool  existing_ps = EINA_FALSE;
   Eina_List *l;

   if (EINA_DBL_EQ(min, max))
     {
        ERR("min & max provided are equal.");
        return;
     }

   if (min > max)
     {
        WRN("min is greater than max.");
     }

   if (elm_widget_is_legacy(obj))
     {
        if (!strcmp(part_name, "elm.cur.progressbar"))
          {
             sd->val_min = min;
             sd->val_max = max;
          }
     }
   else
     {
        if (!strcmp(part_name, "efl.cur.progressbar"))
          {
             sd->val_min = min;
             sd->val_max = max;
          }
     }

   EINA_LIST_FOREACH(sd->progress_status, l, ps)
     {
        if (!strcmp(ps->part_name, part_name))
          {
             existing_ps = EINA_TRUE;
             ps->val_min = min;
             ps->val_max = max;
             break;
          }
     }
    if (!existing_ps)
    {
      ps = _progress_status_new(part_name, min, edje_object_part_exists(obj, part_name));
      ps->val_min = min;
      ps->val_max = max;
      sd->progress_status = eina_list_append(sd->progress_status, ps);
    }
    _val_set(obj);
}

/**
 * @brief Internally sets the theme to pulse mode if not already set.
 * @param obj The progressbar object.
 * @param sd The private data of the progressbar.
 * @param pulse EINA_TRUE to enable pulse mode in theme, EINA_FALSE otherwise.
 * @internal
 *
 * This function is for non-legacy progressbars. It changes the internal
 * pulse flag and reapplies the theme if the state changes. This ensures
 * the correct Edje signals ("efl,state,pulse" or "efl,state,fraction") are emitted.
 */
static void
_internal_theme_mode_pulse_set(Eo *obj, Efl_Ui_Progressbar_Data *sd, Eina_Bool pulse)
{
   if (elm_widget_is_legacy(obj))
     return;
   if (sd->pulse == pulse)
     return;
   sd->pulse = pulse;
   efl_ui_widget_theme_apply(obj);
}

/**
 * @brief Sets the value for a specific progressbar part.
 * @param obj The progressbar object.
 * @param sd The private data of the progressbar.
 * @param part_name The name of the part to modify.
 * @param val The new value for the part.
 * @internal
 *
 * Updates or creates a progress status entry for the given part with the new
 * value. It ensures the value is clamped within the part's min/max range.
 * If the part is the main progressbar, it also updates the overall value in
 * private data and triggers relevant events (changed, min_reached, max_reached).
 * Setting a value implicitly disables pulse mode for the theme.
 */
static void
_progressbar_part_value_set(Eo *obj, Efl_Ui_Progressbar_Data *sd, const char *part_name, double val)
{
   Efl_Ui_Progress_Status *ps;
   Eina_Bool  existing_ps = EINA_FALSE;
   Eina_List *l;
   double min = 0.0, max = 0.0;
   const char *curprogresspart[] =
   {
     "efl.cur.progressbar",
     "elm.cur.progressbar",
   };
   Eina_Bool is_cur_progressbar = !strcmp(part_name, curprogresspart[elm_widget_is_legacy(obj)]);

   _internal_theme_mode_pulse_set(obj, sd, EINA_FALSE);

   if ((!is_cur_progressbar) || sd->has_cur_progressbar_part)
     efl_ui_range_limits_get(efl_part(obj, part_name), &min, &max);

   if (val < min) val = min;
   if (val > max) val = max;

   if (is_cur_progressbar)
     sd->val = val;

   EINA_LIST_FOREACH(sd->progress_status, l, ps)
     {
        if (!strcmp(ps->part_name, part_name))
          {
             existing_ps = EINA_TRUE;
             break;
          }
     }

   if (!existing_ps)
      {
         ps = _progress_status_new(part_name, val, edje_object_part_exists(obj, part_name));
         ps->val_min = 0.0;
         ps->val_max = 1.0;
         ps->val = val;
         sd->progress_status = eina_list_append(sd->progress_status, ps);
      }
   else
      ps->val = val;

   _val_set(obj); // Update visual representation
   _units_set(obj); // Update text label

   // Emit change events
   if (elm_widget_is_legacy(obj))
     efl_event_callback_legacy_call
       (obj, EFL_UI_RANGE_EVENT_CHANGED, NULL);
   else
     {
        efl_event_callback_call(obj, EFL_UI_RANGE_EVENT_CHANGED, NULL);
        if (EINA_DBL_EQ(sd->val, min)) // Check against the part's min
          efl_event_callback_call(obj, EFL_UI_RANGE_EVENT_MIN_REACHED, NULL);
        if (EINA_DBL_EQ(sd->val, max)) // Check against the part's max
          efl_event_callback_call(obj, EFL_UI_RANGE_EVENT_MAX_REACHED, NULL);
     }
}

/**
 * @brief Gets the value of a specific progressbar part.
 * @param sd The private data of the progressbar.
 * @param part The name of the part.
 * @return The current value of the part, or 0.0 if the part is not found.
 * @internal
 */
static double
_progressbar_part_value_get(Efl_Ui_Progressbar_Data *sd, const char* part)
{
   Efl_Ui_Progress_Status *ps;
   Eina_List *l;

   EINA_LIST_FOREACH(sd->progress_status, l, ps)
     {
        if (!strcmp(ps->part_name, part)) return ps->val;
     }

   return 0.0;
}

/**
 * @brief Efl.Ui.Range.Display.range_value_set implementation.
 * @param obj The progressbar object.
 * @param sd The private data of the progressbar.
 * @param val The new value for the main progressbar.
 * @internal
 *
 * Sets the value of the main progress indicator part ("elm.cur.progressbar"
 * for legacy, "efl.cur.progressbar" for non-legacy).
 */
EOLIAN static void
_efl_ui_progressbar_efl_ui_range_display_range_value_set(Eo *obj, Efl_Ui_Progressbar_Data *sd, double val)
{
   if (EINA_DBL_EQ(sd->val, val)) return;

   if (elm_widget_is_legacy(obj))
     _progressbar_part_value_set(obj, sd, "elm.cur.progressbar", val);
   else
     {
        if (val < sd->val_min)
          {
             ERR("Error, value is less than minimum");
             return;
          }
        if (val > sd->val_max)
          {
             ERR("Error, value is greater than maximum");
             return;
          }
        _progressbar_part_value_set(obj, sd, "efl.cur.progressbar", val);
     }
}

/**
 * @brief Efl.Ui.Range.Display.range_value_get implementation.
 * @param obj The progressbar object.
 * @param sd The private data of the progressbar.
 * @return The current value of the main progressbar.
 * @internal
 *
 * Gets the value of the main progress indicator part. Returns 0.0 if the
 * part doesn't exist.
 */
EOLIAN static double
_efl_ui_progressbar_efl_ui_range_display_range_value_get(const Eo *obj, Efl_Ui_Progressbar_Data *sd)
{
   if (!sd->has_cur_progressbar_part) return 0.0;
   if (elm_widget_is_legacy(obj))
     return efl_ui_range_value_get(efl_part(obj, "elm.cur.progressbar"));
   else
     return efl_ui_range_value_get(efl_part(obj, "efl.cur.progressbar"));
}

/**
 * @brief Applies the current pulse state by emitting Edje signals.
 * @param obj The progressbar object.
 * @param sd The private data of the progressbar.
 * @internal
 *
 * Emits "elm,state,pulse,start" or "elm,state,pulse,stop" (or efl equivalents)
 * based on `sd->pulse_state`.
 */
static void
_apply_pulse_state(Eo *obj, Efl_Ui_Progressbar_Data *sd)
{
   Eina_Bool legacy = elm_widget_is_legacy(obj);
   const char *emitter = legacy ? "elm" : "efl";
   const char *signal = legacy ? "elm,state,pulse," : "efl,state,pulse,";
   char signal_buffer[strlen(signal) + strlen("start") + 1];

   snprintf(signal_buffer, sizeof(signal_buffer), "%s%s", signal, sd->pulse_state ? "start" : "stop");
   elm_layout_signal_emit(obj, signal_buffer, emitter);
}

/**
 * @brief Sets the infinite (pulse) mode of the progressbar.
 * @param obj The progressbar object.
 * @param sd The private data of the progressbar.
 * @param state EINA_TRUE to enable infinite mode, EINA_FALSE to disable.
 * @internal
 *
 * This function manages the `pulse_state` flag. If the state changes,
 * it ensures the theme is set to pulse mode (via `_internal_theme_mode_pulse_set`)
 * and then applies the new pulse state (start/stop signals via `_apply_pulse_state`).
 */
EOLIAN static void
_efl_ui_progressbar_infinite_mode_set(Eo *obj, Efl_Ui_Progressbar_Data *sd, Eina_Bool state)
{
   state = !!state; // Normalize to EINA_TRUE or EINA_FALSE

   if (sd->pulse_state == state)
     return;

   sd->pulse_state = state;

   _internal_theme_mode_pulse_set(obj, sd, EINA_TRUE);
   _apply_pulse_state(obj, sd);
}

/**
 * @brief Gets the infinite (pulse) mode state of the progressbar.
 * @param obj The progressbar object (unused).
 * @param sd The private data of the progressbar.
 * @return EINA_TRUE if infinite mode is active, EINA_FALSE otherwise.
 * @internal
 *
 * Infinite mode is considered active if both `sd->pulse_state` (user-set intent)
 * and `sd->pulse` (theme is in pulse mode) are true.
 */
EOLIAN static Eina_Bool
_efl_ui_progressbar_infinite_mode_get(const Eo *obj EINA_UNUSED, Efl_Ui_Progressbar_Data *sd)
{
   return (sd->pulse_state && sd->pulse);
}

/**
 * @brief Efl.Ui.Range.Display.range_limits_set implementation.
 * @param obj The progressbar object.
 * @param sd The private data of the progressbar.
 * @param min The new minimum value for the main progressbar.
 * @param max The new maximum value for the main progressbar.
 * @internal
 *
 * Sets the min/max limits for the main progress indicator part.
 * Logs errors if min > max or min == max.
 */
EOLIAN static void
_efl_ui_progressbar_efl_ui_range_display_range_limits_set(Eo *obj, Efl_Ui_Progressbar_Data *sd, double min, double max)
{
   if (max < min)
     {
        ERR("Wrong params. min(%lf) is greater than max(%lf).", min, max);
        return;
     }
   if (EINA_DBL_EQ(max, min))
     {
        ERR("min and max must have a different value");
        return;
     }
  if (elm_widget_is_legacy(obj))
    _progress_part_min_max_set(obj, sd, "elm.cur.progressbar", min, max);
  else
    _progress_part_min_max_set(obj, sd, "efl.cur.progressbar", min, max);
}

/**
 * @brief Efl.Ui.Range.Display.range_limits_get implementation.
 * @param obj The progressbar object (unused).
 * @param sd The private data of the progressbar.
 * @param[out] min Pointer to store the minimum value.
 * @param[out] max Pointer to store the maximum value.
 * @internal
 *
 * Gets the min/max limits of the main progress indicator part, which are
 * stored directly in the progressbar's private data (`sd->val_min`, `sd->val_max`).
 */
EOLIAN static void
_efl_ui_progressbar_efl_ui_range_display_range_limits_get(const Eo *obj EINA_UNUSED, Efl_Ui_Progressbar_Data *sd, double *min, double *max)
{
   if (min) *min = sd->val_min;
   if (max) *max = sd->val_max;
}

/* Efl.Part begin */

/**
 * @brief Efl.Part.part_get implementation.
 * @param obj The progressbar object.
 * @param sd Private data (unused in this function directly).
 * @param part The name of the part to get.
 * @return The part object if found and supported, otherwise result from superclass.
 * @internal
 *
 * Returns a special part object (EFL_UI_PROGRESSBAR_PART_CLASS) for draggable
 * parts in legacy mode (any part with drag direction not NONE) or for
 * "efl.cur.progressbar" in non-legacy mode. This allows these parts to
 * implement range interfaces.
 */
EOLIAN static Eo *
_efl_ui_progressbar_efl_part_part_get(const Eo *obj, Efl_Ui_Progressbar_Data *sd EINA_UNUSED, const char *part)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(part, NULL);
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd, NULL);

   if (elm_widget_is_legacy(obj))
     {
        // Progress bars are dragable types
        if (edje_object_part_drag_dir_get(wd->resize_obj, part) != (Edje_Drag_Dir)EFL_UI_DRAG_DIR_NONE)
          return ELM_PART_IMPLEMENT(EFL_UI_PROGRESSBAR_PART_CLASS, obj, part);
     }
   else
     {
        if (eina_streq(part, "efl.cur.progressbar"))
          return ELM_PART_IMPLEMENT(EFL_UI_PROGRESSBAR_PART_CLASS, obj, part);
     }
   return efl_part_get(efl_super(obj, MY_CLASS), part);
}

/**
 * @brief Efl.Ui.Range.Display.range_value_set implementation for a progressbar part.
 * @param obj The part object (EFL_UI_PROGRESSBAR_PART_CLASS).
 * @param _pd Part private data (unused).
 * @param val The new value for the part.
 * @internal
 *
 * Retrieves the main progressbar object and its data, then calls
 * `_progressbar_part_value_set` for the specific part.
 */
EOLIAN static void
_efl_ui_progressbar_part_efl_ui_range_display_range_value_set(Eo *obj, void *_pd EINA_UNUSED, double val)
{
  Elm_Part_Data *pd = efl_data_scope_get(obj, EFL_UI_WIDGET_PART_CLASS); // Part data (name, parent obj)
  Efl_Ui_Progressbar_Data *sd = efl_data_scope_get(pd->obj, EFL_UI_PROGRESSBAR_CLASS); // Main progressbar data

  _progressbar_part_value_set(pd->obj, sd, pd->part, val);
}

/**
 * @brief Efl.Ui.Range.Display.range_value_get implementation for a progressbar part.
 * @param obj The part object.
 * @param _pd Part private data (unused).
 * @return The current value of the part.
 * @internal
 */
EOLIAN static double
_efl_ui_progressbar_part_efl_ui_range_display_range_value_get(const Eo *obj, void *_pd EINA_UNUSED)
{
   Elm_Part_Data *pd = efl_data_scope_get(obj, EFL_UI_WIDGET_PART_CLASS);
   Efl_Ui_Progressbar_Data *sd = efl_data_scope_get(pd->obj, EFL_UI_PROGRESSBAR_CLASS);

   return _progressbar_part_value_get(sd, pd->part);
}

/**
 * @brief Efl.Ui.Range.Display.range_limits_set implementation for a progressbar part.
 * @param obj The part object.
 * @param _pd Part private data (unused).
 * @param min The new minimum value for the part.
 * @param max The new maximum value for the part.
 * @internal
 */
EOLIAN static void
_efl_ui_progressbar_part_efl_ui_range_display_range_limits_set(Eo *obj, void *_pd EINA_UNUSED, double min, double max)
{
  Elm_Part_Data *pd = efl_data_scope_get(obj, EFL_UI_WIDGET_PART_CLASS);
  Efl_Ui_Progressbar_Data *sd = efl_data_scope_get(pd->obj, EFL_UI_PROGRESSBAR_CLASS);

  _progress_part_min_max_set(pd->obj, sd, pd->part, min, max);
}

/**
 * @brief Efl.Ui.Range.Display.range_limits_get implementation for a progressbar part.
 * @param obj The part object.
 * @param _pd Part private data (unused).
 * @param[out] min Pointer to store the minimum value.
 * @param[out] max Pointer to store the maximum value.
 * @internal
 *
 * Iterates through the `progress_status` list to find the specified part
 * and retrieve its min/max values.
 */
EOLIAN static void
_efl_ui_progressbar_part_efl_ui_range_display_range_limits_get(const Eo *obj, void *_pd EINA_UNUSED, double *min, double *max)
{
   Efl_Ui_Progress_Status *ps;
   Eina_List *l;

   Elm_Part_Data *pd = efl_data_scope_get(obj, EFL_UI_WIDGET_PART_CLASS);
   Efl_Ui_Progressbar_Data *sd = efl_data_scope_get(pd->obj, EFL_UI_PROGRESSBAR_CLASS);

   EINA_LIST_FOREACH(sd->progress_status, l, ps)
     {
        if (!strcmp(ps->part_name, pd->part))
          {
             if (min) *min = ps->val_min;
             if (max) *max = ps->val_max;
             break;
          }
     }
}

/**
 * @brief Sets the visibility of the progress label.
 * @param obj The progressbar object.
 * @param pd The private data of the progressbar.
 * @param show EINA_TRUE to show the label, EINA_FALSE to hide it.
 * @internal
 *
 * Updates the `show_progress_label` flag, emits Edje signals to show/hide
 * the unit text part in the theme, and updates the unit text content.
 */
EOLIAN static void
_efl_ui_progressbar_show_progress_label_set(Eo *obj EINA_UNUSED, Efl_Ui_Progressbar_Data *pd, Eina_Bool show)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);
   char signal_name[32];
   const char *ns = elm_widget_is_legacy(obj) ? "elm" : "efl";

   pd->show_progress_label = show;

   snprintf(signal_name, sizeof(signal_name), "%s,state,units,%s", ns,
            show ? "visible" : "hidden");
   elm_layout_signal_emit(obj, signal_name, ns);
   edje_object_message_signal_process(wd->resize_obj);
   _units_set(obj);
   efl_canvas_group_change(obj);
}

/**
 * @brief Gets the visibility of the progress label.
 * @param obj The progressbar object (unused).
 * @param pd The private data of the progressbar.
 * @return EINA_TRUE if the label is shown, EINA_FALSE otherwise.
 * @internal
 */
EOLIAN static Eina_Bool
_efl_ui_progressbar_show_progress_label_get(const Eo *obj EINA_UNUSED, Efl_Ui_Progressbar_Data *pd)
{
   return pd->show_progress_label;
}

/**
 * @brief Efl.Ui.Format.apply_formatted_value implementation.
 * @param obj The progressbar object.
 * @param pd Private data (unused).
 * @internal
 *
 * Called when the format string or format function changes. This function
 * simply calls `_units_set` to update the displayed text.
 */
EOLIAN static void
_efl_ui_progressbar_efl_ui_format_apply_formatted_value(Eo *obj, Efl_Ui_Progressbar_Data *pd EINA_UNUSED)
{
   _units_set(obj);
}

#include "efl_ui_progressbar_part.eo.c"

/* Efl.Part end */

/* Internal EO APIs and hidden overrides */
ELM_PART_TEXT_DEFAULT_IMPLEMENT(efl_ui_progressbar, Efl_Ui_Progressbar_Data)
ELM_PART_MARKUP_DEFAULT_IMPLEMENT(efl_ui_progressbar, Efl_Ui_Progressbar_Data)
ELM_PART_CONTENT_DEFAULT_IMPLEMENT(efl_ui_progressbar, Efl_Ui_Progressbar_Data)

EFL_UI_LAYOUT_CONTENT_ALIASES_IMPLEMENT(efl_ui_progressbar)

#define EFL_UI_PROGRESSBAR_EXTRA_OPS \
   EFL_UI_LAYOUT_CONTENT_ALIASES_OPS(efl_ui_progressbar), \
   EFL_CANVAS_GROUP_ADD_DEL_OPS(efl_ui_progressbar)

#include "efl_ui_progressbar.eo.c"

#include "efl_ui_progressbar_legacy_eo.h"
#include "efl_ui_progressbar_legacy_part.eo.h"

#define MY_CLASS_NAME_LEGACY "elm_progressbar"

/**
 * @brief Legacy class constructor for elm_progressbar.
 * @param klass The Efl_Class being constructed.
 * @internal
 *
 * Registers the legacy type "elm_progressbar" with the Evas smart system.
 */
static void
_efl_ui_progressbar_legacy_class_constructor(Efl_Class *klass)
{
   evas_smart_legacy_type_register(MY_CLASS_NAME_LEGACY, klass);
}

/**
 * @brief Legacy object constructor for elm_progressbar.
 * @param obj The legacy progressbar object.
 * @param pd Private data (unused).
 * @return The constructed legacy object.
 * @internal
 *
 * Calls the superclass constructor, sets the Evas object type, and
 * enables legacy focus handling.
 */
EOLIAN static Eo *
_efl_ui_progressbar_legacy_efl_object_constructor(Eo *obj, void *pd EINA_UNUSED)
{
   obj = efl_constructor(efl_super(obj, EFL_UI_PROGRESSBAR_LEGACY_CLASS));
   efl_canvas_object_type_set(obj, MY_CLASS_NAME_LEGACY);
   legacy_object_focus_handle(obj);
   return obj;
}

/**
 * @brief Legacy theme_apply implementation for elm_progressbar.
 * @param obj The legacy progressbar object.
 * @param _pd Private data (unused).
 * @return Eina_Error indicating success or failure.
 * @internal
 *
 * FIXME: This is replicated from elm_layout because progressbar's icon spot
 * is "elm.swallow.content", not "elm.swallow.icon". This should be fixed
 * when the theme API can be changed.
 *
 * Calls the superclass theme_apply and then emits legacy icon signals if finalized.
 */
EOLIAN static Eina_Error
_efl_ui_progressbar_legacy_efl_ui_widget_theme_apply(Eo *obj, void *_pd EINA_UNUSED)
{
   Eina_Error int_ret = EFL_UI_THEME_APPLY_ERROR_GENERIC;

   int_ret = efl_ui_widget_theme_apply(efl_super(obj, EFL_UI_PROGRESSBAR_LEGACY_CLASS));
   if (int_ret == EFL_UI_THEME_APPLY_ERROR_GENERIC) return int_ret;
   if (efl_finalized_get(obj)) _elm_layout_legacy_icon_signal_emit(obj);

   return int_ret;
}

/**
 * @brief Legacy sub_object_del implementation for elm_progressbar.
 * @param obj The legacy progressbar object.
 * @param _pd Private data (unused).
 * @param sobj The sub-object being deleted.
 * @return EINA_TRUE if the sub-object was handled, EINA_FALSE otherwise.
 * @internal
 *
 * FIXME: This is replicated from elm_layout for the same reasons as theme_apply.
 *
 * Calls the superclass sub_object_del and then emits legacy icon signals.
 */
EOLIAN static Eina_Bool
_efl_ui_progressbar_legacy_efl_ui_widget_widget_sub_object_del(Eo *obj, void *_pd EINA_UNUSED, Evas_Object *sobj)
{
   Eina_Bool int_ret = EINA_FALSE;

   int_ret = elm_widget_sub_object_del(efl_super(obj, EFL_UI_PROGRESSBAR_LEGACY_CLASS), sobj);
   if (!int_ret) return EINA_FALSE;

   _elm_layout_legacy_icon_signal_emit(obj);

   return EINA_TRUE;
}

/**
 * @brief Legacy content_set implementation for elm_progressbar.
 * @param obj The legacy progressbar object.
 * @param _pd Private data (unused).
 * @param part The name of the part to set content for.
 * @param content The content object.
 * @return EINA_TRUE if content was set, EINA_FALSE otherwise.
 * @internal
 *
 * FIXME: This is replicated from elm_layout for the same reasons as theme_apply.
 *
 * Calls the superclass content_set (via efl_part) and then emits legacy icon signals.
 */
static Eina_Bool
_efl_ui_progressbar_legacy_content_set(Eo *obj, void *_pd EINA_UNUSED, const char *part, Evas_Object *content)
{
   Eina_Bool int_ret = EINA_FALSE;

   int_ret = efl_content_set(efl_part(efl_super(obj, EFL_UI_PROGRESSBAR_LEGACY_CLASS), part), content);
   if (!int_ret) return EINA_FALSE;

   _elm_layout_legacy_icon_signal_emit(obj);

   return EINA_TRUE;
}

/* Efl.Part for legacy begin */

/**
 * @brief Checks if a part name corresponds to a special legacy progressbar part.
 * @param obj The object (unused).
 * @param part The part name to check.
 * @return EINA_TRUE if the part is "elm.swallow.content", EINA_FALSE otherwise.
 * @internal
 *
 * This is used by the ELM_PART_OVERRIDE_PARTIAL macro to determine if
 * the specialized legacy part implementation should be used for content_set.
 */
static Eina_Bool
_part_is_efl_ui_progressbar_legacy_part(const Eo *obj EINA_UNUSED, const char *part)
{
   return eina_streq(part, "elm.swallow.content");
}

ELM_PART_OVERRIDE_PARTIAL(efl_ui_progressbar_legacy, EFL_UI_PROGRESSBAR_LEGACY, void, _part_is_efl_ui_progressbar_legacy_part)
ELM_PART_OVERRIDE_CONTENT_SET_NO_SD(efl_ui_progressbar_legacy)
#include "efl_ui_progressbar_legacy_part.eo.c"

/* Efl.Part for legacy end */

/**
 * @brief Adds a new progressbar widget to the given parent Elementary (container) object.
 * @param parent The parent object.
 * @return A new progressbar widget handle or @c NULL, on errors.
 * @ingroup Elm_Progressbar_Group
 *
 * This function inserts a new progressbar widget on the canvas.
 * Default unit format is "%.0f%%".
 */
EAPI Evas_Object *
elm_progressbar_add(Evas_Object *parent)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);
   Eo *obj = elm_legacy_add(EFL_UI_PROGRESSBAR_LEGACY_CLASS, parent);
   elm_progressbar_unit_format_set(obj, "%.0f%%");

   return obj;
}

/**
 * @brief Sets the pulse mode of a progressbar.
 * @param obj The progressbar object.
 * @param pulse @c EINA_TRUE to enable pulse mode, @c EINA_FALSE to disable.
 * @ingroup Elm_Progressbar_Group
 *
 * In pulse mode, the progressbar will animate, indicating an ongoing
 * operation of unknown duration. This function sets the *intention* to use
 * pulse mode. The actual animation is started/stopped with elm_progressbar_pulse().
 * This also triggers a theme apply to switch between "fraction" and "pulse" states.
 */
EAPI void
elm_progressbar_pulse_set(Evas_Object *obj, Eina_Bool pulse)
{
   EFL_UI_PROGRESSBAR_DATA_GET_OR_RETURN(obj, sd);
   pulse = !!pulse;
   if (sd->pulse == pulse) return;

   sd->pulse = pulse;

   efl_ui_widget_theme_apply(obj);
}

/**
 * @brief Gets the pulse mode of a progressbar.
 * @param obj The progressbar object.
 * @return @c EINA_TRUE if pulse mode is enabled, @c EINA_FALSE otherwise.
 * @ingroup Elm_Progressbar_Group
 * @see elm_progressbar_pulse_set()
 */
EAPI Eina_Bool
elm_progressbar_pulse_get(const Evas_Object *obj)
{
   EFL_UI_PROGRESSBAR_DATA_GET_OR_RETURN(obj, sd, EINA_FALSE);
   return sd->pulse;
}

/**
 * @brief Starts or stops the pulsing animation of a progressbar.
 * @param obj The progressbar object.
 * @param state @c EINA_TRUE to start pulsing, @c EINA_FALSE to stop.
 * @ingroup Elm_Progressbar_Group
 *
 * This function actually starts or stops the animation. Pulse mode must
 * first be enabled by elm_progressbar_pulse_set().
 */
EAPI void
elm_progressbar_pulse(Evas_Object *obj, Eina_Bool state)
{
   EFL_UI_PROGRESSBAR_DATA_GET_OR_RETURN(obj, sd);

   state = !!state;
   if ((!sd->pulse) || (sd->pulse_state == state)) return;

   sd->pulse_state = state;

   _apply_pulse_state(obj, sd);
}

/**
 * @brief Gets whether the progressbar is currently pulsing.
 * @param obj The progressbar object.
 * @return @c EINA_TRUE if the progressbar is pulsing, @c EINA_FALSE otherwise.
 * @ingroup Elm_Progressbar_Group
 *
 * This reflects the actual animation state, which depends on both
 * elm_progressbar_pulse_set() and elm_progressbar_pulse() having been called appropriately.
 */
EAPI Eina_Bool
elm_progressbar_is_pulsing_get(const Evas_Object *obj)
{
   EFL_UI_PROGRESSBAR_DATA_GET_OR_RETURN(obj, sd, EINA_FALSE);
   return (sd->pulse_state && sd->pulse);
}

/**
 * @brief Sets the value of a specific progressbar part.
 * @param obj The progressbar object.
 * @param part The name of the part (e.g., "elm.cur.progressbar").
 * @param val The value to set (typically between 0.0 and 1.0, unless limits are changed).
 * @ingroup Elm_Progressbar_Group
 * @deprecated Use efl_ui_range_value_set(efl_part(obj, part), val) instead.
 *
 * This function directly calls the Efl.Ui.Range.Display interface on the part.
 */
EAPI void
elm_progressbar_part_value_set(Evas_Object *obj, const char *part, double val)
{
   if (EINA_DBL_EQ(efl_ui_range_value_get(efl_part(obj, part)), val)) return;
   efl_ui_range_value_set(efl_part(obj, part), val);
}

/**
 * @brief Gets the value of a specific progressbar part.
 * @param obj The progressbar object.
 * @param part The name of the part.
 * @return The current value of the part.
 * @ingroup Elm_Progressbar_Group
 * @deprecated Use efl_ui_range_value_get(efl_part(obj, part)) instead.
 */
EAPI double
elm_progressbar_part_value_get(const Evas_Object *obj, const char *part)
{
   return efl_ui_range_value_get(efl_part(obj, part));
}

/**
 * @brief Gets the orientation of a progressbar.
 * @param obj The progressbar object.
 * @return @c EINA_TRUE if horizontal, @c EINA_FALSE if vertical.
 * @ingroup Elm_Progressbar_Group
 * @deprecated Use efl_ui_layout_orientation_get() and check if it's horizontal.
 */
EAPI Eina_Bool
elm_progressbar_horizontal_get(const Evas_Object *obj)
{
   EFL_UI_PROGRESSBAR_DATA_GET_OR_RETURN(obj, sd, EINA_FALSE);

   return _is_horizontal(sd->dir);
}

/**
 * @brief Sets the inverted mode of a progressbar.
 * @param obj The progressbar object.
 * @param inverted @c EINA_TRUE to invert, @c EINA_FALSE for normal.
 * @ingroup Elm_Progressbar_Group
 * @deprecated Use efl_ui_layout_orientation_set() with the inverted flag.
 *
 * In inverted mode, the progressbar's values are represented in reverse.
 * For example, a horizontal progressbar will fill from right to left.
 */
EAPI void
elm_progressbar_inverted_set(Evas_Object *obj, Eina_Bool inverted)
{
   Efl_Ui_Layout_Orientation dir;
   EFL_UI_PROGRESSBAR_DATA_GET_OR_RETURN(obj, sd);

   dir = sd->dir & EFL_UI_LAYOUT_ORIENTATION_AXIS_BITMASK;
   if (inverted) dir |= EFL_UI_LAYOUT_ORIENTATION_INVERTED;

   efl_ui_layout_orientation_set(obj, dir);
}

/**
 * @brief Gets the inverted mode of a progressbar.
 * @param obj The progressbar object.
 * @return @c EINA_TRUE if inverted, @c EINA_FALSE otherwise.
 * @ingroup Elm_Progressbar_Group
 * @deprecated Use efl_ui_layout_orientation_get() and check the inverted flag.
 */
EAPI Eina_Bool
elm_progressbar_inverted_get(const Evas_Object *obj)
{
   EFL_UI_PROGRESSBAR_DATA_GET_OR_RETURN(obj, sd, EINA_FALSE);

   return efl_ui_layout_orientation_is_inverted(sd->dir);
}

/**
 * @brief Sets the orientation of a progressbar.
 * @param obj The progressbar object.
 * @param horizontal @c EINA_TRUE for horizontal, @c EINA_FALSE for vertical.
 * @ingroup Elm_Progressbar_Group
 * @deprecated Use efl_ui_layout_orientation_set() instead.
 */
EAPI void
elm_progressbar_horizontal_set(Evas_Object *obj, Eina_Bool horizontal)
{
   Efl_Ui_Layout_Orientation dir;
   EFL_UI_PROGRESSBAR_DATA_GET_OR_RETURN(obj, sd);

   dir = horizontal ? EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL : EFL_UI_LAYOUT_ORIENTATION_VERTICAL;
   dir |= (sd->dir & EFL_UI_LAYOUT_ORIENTATION_INVERTED);

   efl_ui_layout_orientation_set(obj, dir);
}

typedef struct
{
   progressbar_func_type format_cb;
   progressbar_freefunc_type format_free_cb; /**< Legacy free function for the formatted string. */
} Pb_Format_Wrapper_Data;

/**
 * @brief Adapter callback to bridge legacy progressbar_func_type to Efl_Ui_Format_Func.
 * @param data Pointer to Pb_Format_Wrapper_Data containing the legacy callbacks.
 * @param str The Eina_Strbuf to append the formatted string to.
 * @param value The Eina_Value (double) representing the progressbar value.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 * @internal
 *
 * This function is used when elm_progressbar_unit_format_function_set() is called.
 * It calls the legacy formatting function and appends its result to the strbuf.
 * It also handles freeing the string returned by the legacy function if a free_cb is provided.
 */
static Eina_Bool
_format_legacy_to_format_eo_cb(void *data, Eina_Strbuf *str, const Eina_Value value)
{
   Pb_Format_Wrapper_Data *pfwd = data;
   char *buf = NULL;
   double val = 0;
   const Eina_Value_Type *type = eina_value_type_get(&value);

   if (type == EINA_VALUE_TYPE_DOUBLE)
     eina_value_get(&value, &val);

   if (pfwd->format_cb)
     buf = pfwd->format_cb(val);
   if (buf)
     eina_strbuf_append(str, buf);
   if (pfwd->format_free_cb) pfwd->format_free_cb(buf);

   return EINA_TRUE;
}

/**
 * @brief Free callback for the Pb_Format_Wrapper_Data structure.
 * @param data Pointer to Pb_Format_Wrapper_Data to be freed.
 * @internal
 */
static void
_format_legacy_to_format_eo_free_cb(void *data)
{
   Pb_Format_Wrapper_Data *pfwd = data;
   free(pfwd);
}

/**
 * @brief Sets a custom function to format the progressbar's unit label.
 * @param obj The progressbar object.
 * @param func The function to call to get the formatted string.
 *             It takes a double (the progress value) and returns a char*.
 * @param free_func A function to free the string returned by @p func.
 * @ingroup Elm_Progressbar_Group
 * @deprecated Use efl_ui_format_func_set() instead.
 *
 * This allows for dynamic formatting of the label text. The `sd->is_legacy_format_cb`
 * flag is set to ensure correct value scaling if needed (legacy functions might expect 0-100).
 */
EAPI void
elm_progressbar_unit_format_function_set(Evas_Object *obj, progressbar_func_type func, progressbar_freefunc_type free_func)
{
   EFL_UI_PROGRESSBAR_DATA_GET_OR_RETURN(obj, sd);
   Pb_Format_Wrapper_Data *pfwd = malloc(sizeof(Pb_Format_Wrapper_Data));
   if (!pfwd) return;

   pfwd->format_cb = func;
   pfwd->format_free_cb = free_func;
   sd->is_legacy_format_cb = EINA_TRUE;

   efl_ui_format_func_set(obj, pfwd, _format_legacy_to_format_eo_cb,
                          _format_legacy_to_format_eo_free_cb);
}

typedef struct
{
   progressbar_func_full_type format_cb;
   progressbar_freefunc_type format_free_cb; /**< Legacy free function for the formatted string. */
   void *format_func_data; /**< User data for the legacy full format function. */
} Pb_Full_Format_Wrapper_Data;

/**
 * @brief Adapter callback to bridge legacy progressbar_func_full_type to Efl_Ui_Format_Func.
 * @param data Pointer to Pb_Full_Format_Wrapper_Data containing the legacy callbacks and user data.
 * @param str The Eina_Strbuf to append the formatted string to.
 * @param value The Eina_Value (double) representing the progressbar value.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 * @internal
 *
 * Similar to _format_legacy_to_format_eo_cb, but for the "full" version of the
 * legacy format function which includes user data.
 */
static Eina_Bool
_format_legacy_to_format_eo_cb_full(void *data, Eina_Strbuf *str, const Eina_Value value)
{
   Pb_Full_Format_Wrapper_Data *pfwd = data;
   char *buf = NULL;
   double val = 0;
   const Eina_Value_Type *type = eina_value_type_get(&value);

   if (type == EINA_VALUE_TYPE_DOUBLE)
     eina_value_get(&value, &val);

   if (pfwd->format_cb)
     buf = pfwd->format_cb(val,pfwd->format_func_data);
   if (buf)
     eina_strbuf_append(str, buf);
   if (pfwd->format_free_cb) pfwd->format_free_cb(buf);

   return EINA_TRUE;
}

/**
 * @brief Free callback for the Pb_Full_Format_Wrapper_Data structure.
 * @param data Pointer to Pb_Full_Format_Wrapper_Data to be freed.
 * @internal
 */
static void
_format_legacy_to_format_eo_full_free_cb(void *data)
{
   Pb_Full_Format_Wrapper_Data *pfwd = data;
   free(pfwd);
}

/**
 * @brief Sets a custom function (with user data) to format the progressbar's unit label.
 * @param obj The progressbar object.
 * @param func The function to call to get the formatted string.
 *             It takes a double (progress value) and user data, returning a char*.
 * @param free_func A function to free the string returned by @p func.
 * @param data User data to be passed to @p func.
 * @ingroup Elm_Progressbar_Group
 * @deprecated Use efl_ui_format_func_set() instead, managing user data within the Efl_Ui_Format_Func itself.
 *
 * Similar to elm_progressbar_unit_format_function_set(), but allows passing user data
 * to the formatting function.
 */
EAPI void
elm_progressbar_unit_format_function_set_full(Evas_Object *obj, progressbar_func_full_type func, progressbar_freefunc_type free_func, void* data)
{
   EFL_UI_PROGRESSBAR_DATA_GET_OR_RETURN(obj, sd);
   Pb_Full_Format_Wrapper_Data *pfwd = malloc(sizeof(Pb_Full_Format_Wrapper_Data));
   if (!pfwd) return;

   pfwd->format_cb = func;
   pfwd->format_free_cb = free_func;
   pfwd->format_func_data = data;
   sd->is_legacy_format_cb = EINA_TRUE;

   efl_ui_format_func_set(obj, pfwd, _format_legacy_to_format_eo_cb_full,
                          _format_legacy_to_format_eo_full_free_cb);
}

/**
 * @brief Sets the span size of the progressbar.
 * @param obj The progressbar object.
 * @param size The new span size. For a horizontal progressbar, this is its height.
 *             For a vertical progressbar, this is its width.
 * @ingroup Elm_Progressbar_Group
 *
 * This controls the "thickness" of the progressbar.
 */
EAPI void
elm_progressbar_span_size_set(Evas_Object *obj, Evas_Coord size)
{
   EFL_UI_PROGRESSBAR_DATA_GET_OR_RETURN(obj, sd);
   _progressbar_span_size_set(obj, sd, size);
}

/**
 * @brief Gets the span size of the progressbar.
 * @param obj The progressbar object.
 * @return The span size.
 * @ingroup Elm_Progressbar_Group
 * @see elm_progressbar_span_size_set()
 */
EAPI Evas_Coord
elm_progressbar_span_size_get(const Evas_Object *obj)
{
   EFL_UI_PROGRESSBAR_DATA_GET_OR_RETURN(obj, sd, 0);
   return sd->size;
}

/**
 * @brief Sets the format string for the progressbar's unit label.
 * @param obj The progressbar object.
 * @param units The format string (e.g., "%.2f %%", "%1.2f units").
 *              Must be a C-style printf format specifier for a double.
 * @ingroup Elm_Progressbar_Group
 * @deprecated Use efl_ui_format_string_set() with EFL_UI_FORMAT_STRING_TYPE_SIMPLE.
 *
 * If @c NULL, the label will be hidden. The `sd->is_legacy_format_string` flag
 * is set to ensure correct value scaling (legacy format strings often expect 0-100).
 */
EAPI void
elm_progressbar_unit_format_set(Evas_Object *obj, const char *units)
{
   EFL_UI_PROGRESSBAR_DATA_GET_OR_RETURN(obj, sd);

   sd->is_legacy_format_string = EINA_TRUE;
   efl_ui_format_string_set(obj, units, EFL_UI_FORMAT_STRING_TYPE_SIMPLE);
}

/**
 * @brief Gets the format string for the progressbar's unit label.
 * @param obj The progressbar object.
 * @return The format string, or @c NULL if not set.
 * @ingroup Elm_Progressbar_Group
 * @deprecated Use efl_ui_format_string_get().
 */
EAPI const char *
elm_progressbar_unit_format_get(const Evas_Object *obj)
{
   const char *fmt = NULL;
   efl_ui_format_string_get(obj, &fmt, NULL);
   return fmt;
}

/**
 * @brief Sets the value of the progressbar.
 * @param obj The progressbar object.
 * @param val The value (typically between 0.0 and 1.0, unless limits are changed).
 * @ingroup Elm_Progressbar_Group
 * @deprecated Use efl_ui_range_value_set() instead.
 */
EAPI void
elm_progressbar_value_set(Evas_Object *obj, double val)
{
   efl_ui_range_value_set(obj, val);
}

/**
 * @brief Gets the value of the progressbar.
 * @param obj The progressbar object.
 * @return The current value.
 * @ingroup Elm_Progressbar_Group
 * @deprecated Use efl_ui_range_value_get() instead.
 */
EAPI double
elm_progressbar_value_get(const Evas_Object *obj)
{
   return efl_ui_range_value_get(obj);
}

#include "efl_ui_progressbar_legacy_eo.c"
