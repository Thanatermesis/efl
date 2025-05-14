#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define EFL_ACCESS_OBJECT_PROTECTED
#define ELM_LAYOUT_PROTECTED
#define EFL_PART_PROTECTED
#define EFL_ACCESS_WIDGET_ACTION_PROTECTED

#include <Elementary.h>

#include "elm_priv.h"
#include "elm_widget_layout.h"
#include "elm_widget_label.h"
#include "elm_label_part.eo.h"
#include "elm_part_helper.h"

#define MY_CLASS ELM_LABEL_CLASS

#define MY_CLASS_NAME "Elm_Label"
#define MY_CLASS_NAME_LEGACY "elm_label"

static const char SIG_SLIDE_END[] = "slide,end";

static const Evas_Smart_Cb_Description _smart_callbacks[] = {
   {SIG_WIDGET_LANG_CHANGED, ""}, /**< handled by elm_widget */
   {SIG_WIDGET_ACCESS_CHANGED, ""}, /**< handled by elm_widget */
   {SIG_SLIDE_END, ""},
   {NULL, NULL}
};

/**
 * @internal
 * @brief Action callback to activate the label (e.g., simulate a click).
 * This function is typically used by the accessibility framework.
 * @param obj The label object.
 * @param params Optional parameters for the action (unused in this case).
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_action_activate(Evas_Object *obj, const char *params EINA_UNUSED)
{
   efl_event_callback_legacy_call(obj, EFL_INPUT_EVENT_CLICKED, NULL);
   return EINA_TRUE;
}

/**
 * @internal
 * @brief Recalculates the minimum size of the label.
 * This is often called when text, wrap settings, or other properties affecting
 * size have changed. It considers wrap width and calculates restricted minimums.
 * @param data The label object.
 */
static void
_recalc(void *data)
{
   ELM_LABEL_DATA_GET(data, sd);
   ELM_WIDGET_DATA_GET_OR_RETURN(data, wd);

   Evas_Coord minw = -1, minh = -1;
   Evas_Coord resw, w;

   evas_event_freeze(evas_object_evas_get(data));
   edje_object_size_min_calc(wd->resize_obj, &minw, NULL);
   evas_object_geometry_get(wd->resize_obj, NULL, NULL, &w, NULL);

   if (sd->wrap_w > minw)
     resw = sd->wrap_w;
   else if ((sd->wrap_w > 0) && (minw > sd->wrap_w))
     resw = minw;
   else
     resw = w;
   edje_object_size_min_restricted_calc(wd->resize_obj, &minw, &minh, resw, 0);

   /* If wrap_w is not set, label's width has to be controlled
      by outside of label. So, we don't need to set minimum width. */
   if (sd->wrap_w == -1)
     evas_object_size_hint_min_set(data, 0, minh);
   else
     evas_object_size_hint_min_set(data, minw, minh);

   evas_event_thaw(evas_object_evas_get(data));
   evas_event_thaw_eval(evas_object_evas_get(data));
}

/**
 * @internal
 * @brief Sets or clears the user-defined text style format string on the label's text part.
 * This format string can include Edje text tags for styling (e.g., "font_size=20 wrap=word").
 * @param obj The Edje object part ("elm.text") where the style is applied.
 *            In practice, this is `wd->resize_obj`.
 * @param format The format string to apply, or NULL to pop the current user style.
 */
static void
_label_format_set(Evas_Object *obj, const char *format)
{
   if (format)
     edje_object_part_text_style_user_push(obj, "elm.text", format);
   else
     edje_object_part_text_style_user_pop(obj, "elm.text");
}

/**
 * @internal
 * @brief Manages the slide animation of the label.
 * This function stops any existing slide, checks conditions (e.g., multiline,
 * empty text), and starts a new slide animation if applicable based on the
 * current slide mode, text width, and label width. It also handles ellipsis
 * settings during sliding.
 * @param obj The label object.
 */
static void
_label_slide_change(Evas_Object *obj)
{
   const Evas_Object *tb;
   char *plaintxt;
   int plainlen = 0;

   ELM_LABEL_DATA_GET(obj, sd);
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   if (!sd->slide_state) return;

   edje_object_signal_emit(wd->resize_obj, "elm,state,slide,stop", "elm");

   //doesn't support multiline slide effect
   if (sd->linewrap)
     {
        WRN("Doesn't support slide effect for multiline! : label=%p", obj);
        return;
     }

   //stop if the text is none.
   plaintxt = _elm_util_mkup_to_text
      (edje_object_part_text_get(wd->resize_obj, "elm.text"));
   if (plaintxt)
     {
        plainlen = strlen(plaintxt);
        free(plaintxt);
     }
   if (plainlen < 1) return;

   //has slide effect.
   if (sd->slide_mode != ELM_LABEL_SLIDE_MODE_NONE)
     {
        Evas_Coord w, tb_w;

        tb = edje_object_part_object_get(wd->resize_obj, "elm.text");
        evas_object_textblock_size_native_get(tb, &tb_w, NULL);
        evas_object_geometry_get(wd->resize_obj, NULL, NULL, &w, NULL);
        if (w <= 0) return;

        if (sd->ellipsis)
          {
             sd->slide_ellipsis = EINA_TRUE;
             elm_label_ellipsis_set(obj, EINA_FALSE);
          }

        //slide only if the slide area is smaller than text width size.
        if (sd->slide_mode == ELM_LABEL_SLIDE_MODE_AUTO)
          {
            if ((tb_w > 0) && (tb_w < w))
              {
                if (sd->slide_ellipsis)
                  {
                     sd->slide_ellipsis = EINA_FALSE;
                     elm_label_ellipsis_set(obj, EINA_TRUE);
                  }
                return;
              }
          }

        // calculate speed or duration
        if (!strcmp(elm_object_style_get(obj), "slide_long"))
          w = tb_w + w;
        else if (!strcmp(elm_object_style_get(obj), "slide_short") ||
                 !strcmp(elm_object_style_get(obj), "slide_bounce")) // slide_short or slide_bounce
          w = tb_w - w;
        else
          w = tb_w;

        if (sd->use_slide_speed)
          {
             if (sd->slide_speed <= 0) sd->slide_speed = 1;
             sd->slide_duration = w / sd->slide_speed;
          }
        else
          {
             if (sd->slide_duration <= 0) sd->slide_duration = 1;
             sd->slide_speed = w / sd->slide_duration;
          }

        Edje_Message_Float_Set *msg =
          alloca(sizeof(Edje_Message_Float_Set));

        msg->count = 1;
        msg->val[0] = sd->slide_duration;

        edje_object_message_send(wd->resize_obj, EDJE_MESSAGE_FLOAT_SET, 0, msg);
        edje_object_signal_emit(wd->resize_obj, "elm,state,slide,start", "elm");
     }
   //no slide effect.
   else
     {
        if (sd->slide_ellipsis)
          {
             sd->slide_ellipsis = EINA_FALSE;
             elm_label_ellipsis_set(obj, EINA_TRUE);
          }
     }
}

/**
 * @internal
 * @brief Updates the horizontal size policy of the label based on ellipsis and wrap settings.
 * If ellipsis is disabled and line wrap is none, the label is set to be horizontally expandable.
 * Otherwise, it's set to a fixed horizontal policy.
 * @param obj The label Evas object.
 * @param sd The label's smart data.
 */
static void
_elm_label_horizontal_size_policy_update(Eo *obj, Elm_Label_Data *sd)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   if (!sd->ellipsis && (sd->linewrap == ELM_WRAP_NONE))
     edje_object_signal_emit(wd->resize_obj, "elm,state,horizontal,expandable", "elm");
   else
     edje_object_signal_emit(wd->resize_obj, "elm,state,horizontal,fixed", "elm");
   edje_object_message_signal_process(wd->resize_obj);
}

EOLIAN static Eina_Error
_elm_label_efl_ui_widget_theme_apply(Eo *obj, Elm_Label_Data *sd)
{
   Eina_Error int_ret = EFL_UI_THEME_APPLY_ERROR_GENERIC;

   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd, EFL_UI_THEME_APPLY_ERROR_GENERIC);

   evas_event_freeze(evas_object_evas_get(obj));

   int_ret = efl_ui_widget_theme_apply(efl_super(obj, MY_CLASS));
   if (int_ret == EFL_UI_THEME_APPLY_ERROR_GENERIC) return int_ret;

   _elm_label_horizontal_size_policy_update(obj, sd);

   _label_format_set(wd->resize_obj, sd->format);
   _label_slide_change(obj);

   evas_event_thaw(evas_object_evas_get(obj));
   evas_event_thaw_eval(evas_object_evas_get(obj));

   elm_layout_sizing_eval(obj);

   return int_ret;
}

EOLIAN static void
_elm_label_efl_canvas_group_group_calculate(Eo *obj, Elm_Label_Data *_pd EINA_UNUSED)
{
   Evas_Coord minw = -1, minh = -1;
   Evas_Coord resw, resh;

   ELM_LABEL_DATA_GET(obj, sd);
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   if (sd->linewrap)
     {
        evas_object_geometry_get(wd->resize_obj, NULL, NULL, &resw, &resh);
        if (resw == sd->lastw) return;
        sd->lastw = resw;
        _recalc(obj);
     }
   else
     {
        evas_event_freeze(evas_object_evas_get(obj));
        edje_object_size_min_calc(wd->resize_obj, &minw, &minh);
        if (sd->wrap_w > 0 && minw > sd->wrap_w) minw = sd->wrap_w;
        evas_object_size_hint_min_set(obj, minw, minh);
        evas_event_thaw(evas_object_evas_get(obj));
        evas_event_thaw_eval(evas_object_evas_get(obj));
     }
}

/**
 * @internal
 * @brief Callback invoked when the label's underlying Edje object is resized.
 * This function triggers a recalculation of the slide animation if active,
 * and re-evaluates sizing if line wrapping is enabled.
 * @param data The label object (passed as user data).
 * @param e The Evas canvas (unused).
 * @param obj The Evas object that was resized (the Edje object, unused).
 * @param event_info Event-specific information (unused).
 */
static void
_on_label_resize(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   ELM_LABEL_DATA_GET(data, sd);

   if (sd->slide_mode != ELM_LABEL_SLIDE_MODE_NONE) _label_slide_change(data);
   if (sd->linewrap) elm_layout_sizing_eval(data);
}

/**
 * @internal
 * @brief Finds the value associated with a key in a key-value pair string.
 * The string is expected to be in a format like "key1=value1 key2=value2".
 * This function locates the `key=` substring and extracts the subsequent value.
 * @param oldstring The string to search within.
 * @param key The key to search for (e.g., "wrap").
 * @param[out] value Pointer to a char* that will be set to the start of the value
 *                   within `oldstring`. The caller should not free this, as it
 *                   points into `oldstring`.
 * @return 0 if the key is found and value is extracted, -1 otherwise.
 */
static int
_get_value_in_key_string(const char *oldstring, const char *key, char **value)
{
   char *curlocater, *endtag;
   int firstindex = 0, foundflag = -1;

   curlocater = strstr(oldstring, key);
   if (curlocater)
     {
        int key_len = strlen(key);
        endtag = curlocater + key_len;
        if ((!endtag) || (*endtag != '='))
             return -1;

        firstindex = abs((int)(oldstring - curlocater));
        firstindex += key_len + 1; // strlen("key") + strlen("=")
        *value = (char *)oldstring + firstindex;

        foundflag = 1;
     }
   else
     {
        foundflag = 0;
     }

   if (foundflag == 1) return 0;

   return -1;
}

/**
 * @internal
 * @brief Replaces or deletes a key-value pair in an Eina_Strbuf.
 * The buffer is assumed to contain a string of space-separated "key=value" pairs,
 * potentially wrapped in "DEFAULT='...'" if it's the initial content.
 *
 * Example: If srcbuf contains "font_size=10 wrap=word", calling with
 * key="wrap", value="char", deleteflag=0 would change it to "font_size=10 wrap=char".
 * If deleteflag=1, it would become "font_size=10".
 * If key="align" (not present), value="center", deleteflag=0, it would become
 * "font_size=10 wrap=word align=center" (or "DEFAULT='align=center'" if empty).
 *
 * @param srcbuf The string buffer to modify.
 * @param key The key of the pair to modify/delete (e.g., "wrap", "font_size").
 * @param value The new value to set if not deleting. Unused if deleteflag is true.
 * @param deleteflag If non-zero, the key-value pair is deleted. Otherwise, it's updated or added.
 * @return Always returns 0.
 */
static int
_strbuf_key_value_replace(Eina_Strbuf *srcbuf, const char *key, const char *value, int deleteflag)
{
   char *kvalue;
   const char *srcstring = NULL;

   srcstring = eina_strbuf_string_get(srcbuf);

   if (_get_value_in_key_string(srcstring, key, &kvalue) == 0)
     {
        const char *val_end;
        int val_end_idx = 0;
        int key_start_idx = 0;
        val_end = strchr(kvalue, ' ');

        if (val_end)
          val_end_idx = val_end - srcstring;
        else
          val_end_idx = kvalue - srcstring + strlen(kvalue) - 1;

        /* -1 is because of the '=' */
        key_start_idx = kvalue - srcstring - 1 - strlen(key);
        eina_strbuf_remove(srcbuf, key_start_idx, val_end_idx);
        if (!deleteflag)
          {
             eina_strbuf_insert_printf(srcbuf, "%s=%s", key_start_idx, key,
                                       value);
          }
     }
   else if (!deleteflag)
     {
        if (*srcstring)
          {
             /* -1 because we want it before the ' */
             eina_strbuf_insert_printf (srcbuf, " %s=%s",
                                eina_strbuf_length_get(srcbuf) - 1, key, value);
          }
        else
          {
             eina_strbuf_append_printf(srcbuf, "DEFAULT='%s=%s'", key, value);
          }
     }

   return 0;
}

/**
 * @internal
 * @brief Replaces or deletes a key-value pair in an eina_stringshare'd string.
 * This is a convenience wrapper around `_strbuf_key_value_replace` for
 * strings managed by eina_stringshare. It creates a temporary Eina_Strbuf,
 * performs the operation, and then updates the stringshare.
 * @param srcstring Pointer to the eina_stringshare'd string. This string will be
 *                  deleted and replaced with the modified version.
 * @param key The key of the pair to modify/delete.
 * @param value The new value to set if not deleting.
 * @param deleteflag If non-zero, the key-value pair is deleted.
 * @return Always returns 0.
 */
static int
_stringshare_key_value_replace(const char **srcstring, const char *key, const char *value, int deleteflag)
{
   Eina_Strbuf *sharebuf = NULL;

   sharebuf = eina_strbuf_new();
   eina_strbuf_append(sharebuf, *srcstring);
   _strbuf_key_value_replace(sharebuf, key, value, deleteflag);
   eina_stringshare_del(*srcstring);
   *srcstring = eina_stringshare_add(eina_strbuf_string_get(sharebuf));
   eina_strbuf_free(sharebuf);

   return 0;
}

/**
 * @internal
 * @brief Sets the text for a given part of the label widget.
 * This function applies the current format string, sets the text markup,
 * and then triggers a re-evaluation of the layout and slide animation.
 * @param obj The label Evas object.
 * @param sd The label's smart data.
 * @param part The name of the text part to set (usually NULL for the default "elm.text").
 * @param label The text string to set. If NULL, it's treated as an empty string.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_label_text_set(Eo *obj, Elm_Label_Data *sd, const char *part, const char *label)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd, EINA_FALSE);

   if (!label) label = "";
   _label_format_set(wd->resize_obj, sd->format);

   efl_text_markup_set(efl_part(efl_super(obj, MY_CLASS), part), label);

   sd->lastw = -1;
   elm_layout_sizing_eval(obj);
   _label_slide_change(obj);

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Provides accessibility information for the label.
 * Returns the custom accessibility information if set, otherwise, it converts
 * the label's markup text to plain text.
 * @param data User data associated with the callback (unused).
 * @param obj The label object.
 * @return A newly allocated string containing the accessibility information.
 *         The caller is responsible for freeing this string.
 */
static char *
_access_info_cb(void *data EINA_UNUSED, Evas_Object *obj)
{
   const char *txt = elm_widget_access_info_get(obj);

   if (!txt)
     return _elm_util_mkup_to_text(elm_layout_text_get(obj, NULL));
   else return strdup(txt);
}

/**
 * @internal
 * @brief Callback invoked when a slide animation cycle ends.
 * This function re-applies ellipsis if it was temporarily disabled for sliding
 * and emits the "slide,end" signal for the label.
 * @param data The label object (passed as user data).
 * @param obj The Edje object that emitted the signal (unused).
 * @param emission The Edje signal string (e.g., "elm,state,slide,end", unused).
 * @param source The source of the Edje signal (e.g., "elm", unused).
 */
static void
_on_slide_end(void *data, Evas_Object *obj EINA_UNUSED,
              const char *emission EINA_UNUSED, const char *source EINA_UNUSED)
{
   ELM_LABEL_DATA_GET(data, sd);

   if (sd->slide_ellipsis)
     elm_obj_label_ellipsis_set(data, EINA_TRUE);

   efl_event_callback_legacy_call(data, ELM_LABEL_EVENT_SLIDE_END, NULL);
}

EOLIAN static void
_elm_label_efl_canvas_group_group_add(Eo *obj, Elm_Label_Data *priv)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   efl_canvas_group_add(efl_super(obj, MY_CLASS));

   priv->linewrap = ELM_WRAP_NONE;
   priv->wrap_w = -1;
   priv->slide_duration = 10;

   priv->format = eina_stringshare_add("");
   _label_format_set(wd->resize_obj, priv->format);

   evas_object_event_callback_add(wd->resize_obj, EVAS_CALLBACK_RESIZE,
                                  _on_label_resize, obj);

   edje_object_signal_callback_add(wd->resize_obj, "elm,state,slide,end", "elm",
                                   _on_slide_end, obj);

   /* access */
   elm_widget_can_focus_set(obj, _elm_config->access_mode);

   _elm_access_object_register(obj, wd->resize_obj);
   _elm_access_text_set(_elm_access_info_get(obj), ELM_ACCESS_TYPE,
                        E_("Label"));
   _elm_access_callback_set(_elm_access_info_get(obj), ELM_ACCESS_INFO,
                            _access_info_cb, NULL);

   if (!elm_layout_theme_set(obj, "label", "base", elm_widget_style_get(obj)))
     CRI("Failed to set layout!");

   elm_layout_text_set(obj, NULL, "<br>");
   elm_layout_sizing_eval(obj);
}

/**
 * @brief Adds a new label widget to the given parent Evas object.
 *
 * @param parent The parent Evas object.
 * @return The new Evas_Object for the label, or NULL on failure.
 *
 * @ingroup Elm_Label_Group
 */
EAPI Evas_Object *
elm_label_add(Evas_Object *parent)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);
   return elm_legacy_add(MY_CLASS, parent);
}

EOLIAN const Efl_Access_Action_Data *
_elm_label_efl_access_widget_action_elm_actions_get(const Eo *obj EINA_UNUSED, Elm_Label_Data *pd EINA_UNUSED)
{
   static Efl_Access_Action_Data access_actions[] = {
        { "activate", "activate", NULL, _action_activate },
        { NULL, NULL, NULL, NULL },
   };
   return &access_actions[0];
}

EOLIAN static Eo *
_elm_label_efl_object_constructor(Eo *obj, Elm_Label_Data *_pd EINA_UNUSED)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));
   efl_canvas_object_type_set(obj, MY_CLASS_NAME_LEGACY);
   evas_object_smart_callbacks_descriptions_set(obj, _smart_callbacks);
   efl_access_object_role_set(obj, EFL_ACCESS_ROLE_LABEL);

   return obj;
}

EOLIAN static void
_elm_label_line_wrap_set(Eo *obj, Elm_Label_Data *sd, Elm_Wrap_Type wrap)
{
   const char *wrap_str, *text;
   int len;

   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   if (sd->linewrap == wrap) return;

   sd->linewrap = wrap;
   sd->lastw = -1;

   _elm_label_horizontal_size_policy_update(obj, sd);

   text = elm_layout_text_get(obj, NULL);
   if (!text) return;

   len = strlen(text);
   if (len <= 0) return;

   switch (wrap)
     {
      case ELM_WRAP_CHAR:
        wrap_str = "char";
        break;

      case ELM_WRAP_WORD:
        wrap_str = "word";
        break;

      case ELM_WRAP_MIXED:
        wrap_str = "mixed";
        break;

      default:
        wrap_str = "none";
        break;
     }

   if (_stringshare_key_value_replace(&sd->format, "wrap", wrap_str, 0) == 0)
     {
        sd->lastw = -1;
        _label_format_set(wd->resize_obj, sd->format);
        elm_layout_sizing_eval(obj);
     }
}

EOLIAN static Elm_Wrap_Type
_elm_label_line_wrap_get(const Eo *obj EINA_UNUSED, Elm_Label_Data *sd)
{
   return sd->linewrap;
}

EOLIAN static void
_elm_label_wrap_width_set(Eo *obj, Elm_Label_Data *sd, Evas_Coord w)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   if (w < 0) w = 0;

   if (sd->wrap_w == w) return;

   if (sd->ellipsis)
     _label_format_set(wd->resize_obj, sd->format);
   sd->wrap_w = w;
   sd->lastw = -1;

   elm_layout_sizing_eval(obj);
}

EOLIAN static Evas_Coord
_elm_label_wrap_width_get(const Eo *obj EINA_UNUSED, Elm_Label_Data *sd)
{
   return sd->wrap_w;
}

EOLIAN static void
_elm_label_ellipsis_set(Eo *obj, Elm_Label_Data *sd, Eina_Bool ellipsis)
{
   Eina_Strbuf *fontbuf = NULL;
   int len, removeflag = 0;
   const char *text;

   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   if (sd->ellipsis == ellipsis) return;
   sd->ellipsis = ellipsis;
   sd->lastw = -1;

   _elm_label_horizontal_size_policy_update(obj, sd);

   text = elm_layout_text_get(obj, NULL);
   if (!text) return;

   len = strlen(text);
   if (len <= 0) return;

   if (ellipsis == EINA_FALSE) removeflag = 1;  // remove fontsize tag

   fontbuf = eina_strbuf_new();
   eina_strbuf_append_printf(fontbuf, "%f", 1.0);

   if (_stringshare_key_value_replace
         (&sd->format, "ellipsis", eina_strbuf_string_get
           (fontbuf), removeflag) == 0)
     {
        _label_format_set(wd->resize_obj, sd->format);
        elm_layout_sizing_eval(obj);
     }
   eina_strbuf_free(fontbuf);
}

EOLIAN static Eina_Bool
_elm_label_ellipsis_get(const Eo *obj EINA_UNUSED, Elm_Label_Data *sd)
{
   return sd->ellipsis;
}

EOLIAN static void
_elm_label_slide_mode_set(Eo *obj EINA_UNUSED, Elm_Label_Data *sd, Elm_Label_Slide_Mode mode)
{
   sd->slide_mode = mode;
}

EOLIAN static Elm_Label_Slide_Mode
_elm_label_slide_mode_get(const Eo *obj EINA_UNUSED, Elm_Label_Data *sd)
{
   return sd->slide_mode;
}

/**
 * @brief Enable or disable the slide animation.
 * @param obj The label object.
 * @param slide EINA_TRUE to enable sliding, EINA_FALSE to disable.
 * @deprecated Use elm_label_slide_mode_set() instead with
 *             ELM_LABEL_SLIDE_MODE_ALWAYS or ELM_LABEL_SLIDE_MODE_NONE.
 * @ingroup Elm_Label_Group
 */
EINA_DEPRECATED EAPI void
elm_label_slide_set(Evas_Object *obj, Eina_Bool slide)
{
   if (slide)
     elm_label_slide_mode_set(obj, ELM_LABEL_SLIDE_MODE_ALWAYS);
   else
     elm_label_slide_mode_set(obj, ELM_LABEL_SLIDE_MODE_NONE);
}

/**
 * @brief Get whether the slide animation is enabled.
 * @param obj The label object.
 * @return EINA_TRUE if sliding is enabled (ELM_LABEL_SLIDE_MODE_ALWAYS), EINA_FALSE otherwise.
 * @deprecated Use elm_label_slide_mode_get() instead and check for
 *             ELM_LABEL_SLIDE_MODE_ALWAYS.
 * @ingroup Elm_Label_Group
 */
EINA_DEPRECATED EAPI Eina_Bool
elm_label_slide_get(const Evas_Object *obj)
{
   Eina_Bool ret = EINA_FALSE;
   if (elm_label_slide_mode_get(obj) == ELM_LABEL_SLIDE_MODE_ALWAYS)
     ret = EINA_TRUE;
   return ret;
}

EOLIAN static void
_elm_label_slide_duration_set(Eo *obj EINA_UNUSED, Elm_Label_Data *sd, double duration)
{
   sd->slide_duration = duration;
   sd->use_slide_speed = EINA_FALSE;
}

EOLIAN static void
_elm_label_slide_speed_set(Eo *obj EINA_UNUSED, Elm_Label_Data *sd, double speed)
{
   sd->slide_speed = speed;
   sd->use_slide_speed = EINA_TRUE;
}

EOLIAN static double
_elm_label_slide_speed_get(const Eo *obj EINA_UNUSED, Elm_Label_Data *sd)
{
   return sd->slide_speed;
}

EOLIAN static void
_elm_label_slide_go(Eo *obj, Elm_Label_Data *sd)
{
   if (!sd->slide_state) sd->slide_state = EINA_TRUE;

   _label_slide_change(obj);
   elm_layout_sizing_eval(obj);
}

EOLIAN static double
_elm_label_slide_duration_get(const Eo *obj EINA_UNUSED, Elm_Label_Data *sd)
{
   return sd->slide_duration;
}

EOLIAN static void
_elm_label_class_constructor(Efl_Class *klass)
{
   evas_smart_legacy_type_register(MY_CLASS_NAME_LEGACY, klass);
}

/* Efl.Part begin */
ELM_PART_OVERRIDE(elm_label, ELM_LABEL, Elm_Label_Data)
ELM_PART_OVERRIDE_TEXT_SET(elm_label, ELM_LABEL, Elm_Label_Data)

#include "elm_label_part.eo.c"
/* Efl.Part end */

/* Internal EO APIs and hidden overrides */

#define ELM_LABEL_EXTRA_OPS \
   EFL_CANVAS_GROUP_CALC_OPS(elm_label), \
   EFL_CANVAS_GROUP_ADD_OPS(elm_label)

#include "elm_label_eo.c"
