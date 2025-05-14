#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define EFL_ACCESS_OBJECT_PROTECTED
#define ELM_LAYOUT_PROTECTED
#define EFL_PART_PROTECTED

#include <Elementary.h>
#include "elm_priv.h"
#include "elm_bubble_eo.h"
#include "elm_widget_bubble.h"
#include "elm_widget_layout.h"

#include "elm_bubble_part.eo.h"
#include "elm_part_helper.h"

#define MY_CLASS ELM_BUBBLE_CLASS
#define MY_CLASS_PFX elm_bubble

#define MY_CLASS_NAME "Elm_Bubble"
#define MY_CLASS_NAME_LEGACY "elm_bubble"

static const char SIG_CLICKED[] = "clicked"; /**< Signal emitted when the bubble is clicked */

/**< @brief Smart callback function descriptions. */
static const Evas_Smart_Cb_Description _smart_callbacks[] =
{
   {SIG_CLICKED, ""}, /**< Called when the bubble is clicked by the user */
   {SIG_LAYOUT_FOCUSED, ""}, /**< handled by elm_layout */
   {SIG_LAYOUT_UNFOCUSED, ""}, /**< handled by elm_layout */
   {NULL, NULL}
};

/**< @brief Aliases for content parts. Allows users to set content using "default" or "icon". */
static const Elm_Layout_Part_Alias_Description _content_aliases[] =
{
   {"default", "elm.swallow.content"}, /**< Alias for the main content area */
   {"icon", "elm.swallow.icon"}, /**< Alias for the icon area */
   {NULL, NULL}
};

/**< @brief Aliases for text parts. Allows users to set text using "default" or "info". */
static const Elm_Layout_Part_Alias_Description _text_aliases[] =
{
   {"default", "elm.text"}, /**< Alias for the main text/label part */
   {"info", "elm.info"}, /**< Alias for the informational text part */
   {NULL, NULL}
};

/**< @brief Array of strings representing the bubble corner positions.
 * Used to set the theme element based on the Elm_Bubble_Pos enum.
 * For example, corner_string[ELM_BUBBLE_POS_TOP_LEFT] is "top_left".
 */
static const char *corner_string[] =
{
   "top_left", /**< Corresponds to ELM_BUBBLE_POS_TOP_LEFT */
   "top_right", /**< Corresponds to ELM_BUBBLE_POS_TOP_RIGHT */
   "bottom_left", /**< Corresponds to ELM_BUBBLE_POS_BOTTOM_LEFT */
   "bottom_right" /**< Corresponds to ELM_BUBBLE_POS_BOTTOM_RIGHT */
};

/**
 * @brief Callback function for the EVAS_CALLBACK_MOUSE_UP event.
 *
 * This function is called when a mouse button is released over the bubble.
 * It emits the "clicked" signal if the event was not a "hold" event.
 *
 * @param data The Evas_Object (bubble widget) passed during callback registration.
 * @param e The Evas canvas.
 * @param obj The Evas object that triggered the event (resize_obj of the bubble).
 * @param event_info Pointer to the Evas_Event_Mouse_Up structure.
 */
static void
_on_mouse_up(void *data,
             Evas *e EINA_UNUSED,
             Evas_Object *obj EINA_UNUSED,
             void *event_info)
{
   Evas_Event_Mouse_Up *ev = event_info;

   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD)
     return;

   evas_object_smart_callback_call(data, "clicked", NULL);
}

/**
 * @brief Sets the text for a given part of the bubble.
 *
 * This function handles text setting for aliased parts like "default" or "info".
 * If the part is "elm.info", it also emits signals to show or hide the info section
 * based on whether the label is set or not.
 *
 * @param obj The Evas_Object (bubble widget).
 * @param _pd The private data of the bubble widget (unused).
 * @param part The name of the part to set text for (e.g., "default", "info").
 *             This can be an alias.
 * @param label The text to set.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_bubble_text_set(Eo *obj, Elm_Bubble_Data *_pd EINA_UNUSED, const char *part, const char *label)
{
   if (!_elm_layout_part_aliasing_eval(obj, &part, EINA_TRUE)) // Resolve alias to actual part name
     return EINA_FALSE;

   efl_text_set(efl_part(efl_super(obj, MY_CLASS), part), label);

   if (!strcmp(part, "elm.info"))
     {
        if (label)
          elm_layout_signal_emit(obj, "elm,state,info,visible", "elm");
        else
          elm_layout_signal_emit(obj, "elm,state,info,hidden", "elm");
     }

   elm_layout_sizing_eval(obj);

   return EINA_TRUE;
}

/**
 * @brief Callback function to provide accessibility information for the bubble.
 *
 * This function constructs a string containing the text from the default part,
 * content part (if any), and info part of the bubble. This string is used by
 * accessibility tools.
 *
 * @param data Custom data, not used here.
 * @param obj The Evas_Object (bubble widget).
 * @return A newly allocated string with accessibility information, or NULL.
 *         The caller is responsible for freeing the returned string.
 */
static char *
_access_info_cb(void *data EINA_UNUSED, Evas_Object *obj)
{
   char *ret;
   Eina_Strbuf *buf;
   buf = eina_strbuf_new();
   Evas_Object *content;
   const char *default_txt = NULL;
   const char *content_txt = NULL;
   const char *info_txt = NULL;

   default_txt = elm_widget_access_info_get(obj);
   if (!default_txt) default_txt = elm_layout_text_get(obj, NULL);
   if (default_txt) eina_strbuf_append(buf, default_txt);

   content = elm_layout_content_get(obj, NULL);
   if (content) content_txt = elm_layout_text_get(content, NULL);
   if (content_txt)
     {
        if (!eina_strbuf_length_get(buf))
          eina_strbuf_append(buf, content_txt);
        else
          eina_strbuf_append_printf(buf, ", %s", content_txt);
     }


   info_txt = edje_object_part_text_get(elm_layout_edje_get(obj), "elm.info");
   if (info_txt)
     {
        if (!eina_strbuf_length_get(buf))
          eina_strbuf_append(buf, info_txt);
        else
          eina_strbuf_append_printf(buf, ", %s", info_txt);
     }

   ret = eina_strbuf_string_steal(buf);
   eina_strbuf_free(buf);
   return ret;
}

/**
 * @internal
 * @brief Implements the efl_canvas_group_add method for Elm_Bubble.
 *
 * This function is called when the bubble widget is added to a canvas group.
 * It performs initial setup for the bubble, including setting default position,
 * focus policy, event callbacks, accessibility, and theme.
 *
 * @param obj The Evas_Object (bubble widget).
 * @param priv The private data of the bubble widget.
 */
EOLIAN static void
_elm_bubble_efl_canvas_group_group_add(Eo *obj, Elm_Bubble_Data *priv)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   efl_canvas_group_add(efl_super(obj, MY_CLASS));

   priv->pos = ELM_BUBBLE_POS_TOP_LEFT; //default

   elm_widget_can_focus_set(obj, EINA_FALSE);

   evas_object_event_callback_add
     (wd->resize_obj, EVAS_CALLBACK_MOUSE_UP,
     _on_mouse_up, obj);

   // ACCESS
   _elm_access_object_register(obj, wd->resize_obj);
   _elm_access_text_set
     (_elm_access_info_get(obj), ELM_ACCESS_TYPE, E_("Bubble"));
   _elm_access_callback_set
     (_elm_access_info_get(obj), ELM_ACCESS_INFO, _access_info_cb, NULL);

   if (!elm_layout_theme_set(obj, "bubble", "base", elm_widget_style_get(obj)))
     CRI("Failed to set layout!");

   elm_layout_sizing_eval(obj);

   if (_elm_config->access_mode == ELM_ACCESS_MODE_ON)
     elm_widget_can_focus_set(obj, EINA_TRUE);
}

/**
 * @internal
 * @brief Implements the efl_ui_widget_on_access_update method for Elm_Bubble.
 *
 * This function is called when the accessibility state of the application changes.
 * It updates the focus policy of the bubble based on the new accessibility state.
 *
 * @param obj The Evas_Object (bubble widget).
 * @param _pd The private data of the bubble widget (unused).
 * @param is_access EINA_TRUE if accessibility is enabled, EINA_FALSE otherwise.
 */
EOLIAN static void
_elm_bubble_efl_ui_widget_on_access_update(Eo *obj, Elm_Bubble_Data *_pd EINA_UNUSED, Eina_Bool is_access)
{
   ELM_BUBBLE_CHECK(obj);

   if (is_access)
     elm_widget_can_focus_set(obj, EINA_TRUE);
   else
     elm_widget_can_focus_set(obj, EINA_FALSE);
}

EAPI Evas_Object *
elm_bubble_add(Evas_Object *parent)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);
   return elm_legacy_add(MY_CLASS, parent);
}

/**
 * @internal
 * @brief Implements the efl_object_constructor method for Elm_Bubble.
 *
 * This function is called during the construction of a new bubble object.
 * It calls the parent constructor, sets the object type, registers smart callbacks,
 * and sets the default accessibility role.
 *
 * @param obj The Evas_Object (bubble widget) being constructed.
 * @param _pd The private data of the bubble widget (unused).
 * @return The constructed Evas_Object.
 */
EOLIAN static Eo *
_elm_bubble_efl_object_constructor(Eo *obj, Elm_Bubble_Data *_pd EINA_UNUSED)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));
   efl_canvas_object_type_set(obj, MY_CLASS_NAME_LEGACY);
   evas_object_smart_callbacks_descriptions_set(obj, _smart_callbacks);
   efl_access_object_role_set(obj, EFL_ACCESS_ROLE_FILLER);

   return obj;
}

/**
 * @internal
 * @brief Sets the corner position of the bubble's arrow.
 *
 * The position determines where the bubble's "tail" or arrow points.
 * It updates the theme element accordingly.
 *
 * @param obj The Evas_Object (bubble widget).
 * @param sd The private data of the bubble widget.
 * @param pos The desired corner position (e.g., ELM_BUBBLE_POS_TOP_LEFT).
 */
EOLIAN static void
_elm_bubble_pos_set(Eo *obj, Elm_Bubble_Data *sd, Elm_Bubble_Pos pos)
{
   /* FIXME: Why is this dealing with layout data directly? */
   if (pos < ELM_BUBBLE_POS_TOP_LEFT || pos > ELM_BUBBLE_POS_BOTTOM_RIGHT)
     return;

   sd->pos = pos;

   // Sets the theme part based on the corner_string array, e.g., "top_left"
   elm_widget_theme_element_set(obj, corner_string[sd->pos]);

   efl_ui_widget_theme_apply(obj); // Apply the theme changes
}

/**
 * @internal
 * @brief Gets the current corner position of the bubble's arrow.
 *
 * @param obj The Evas_Object (bubble widget) (unused).
 * @param sd The private data of the bubble widget.
 * @return The current corner position.
 */
EOLIAN static Elm_Bubble_Pos
_elm_bubble_pos_get(const Eo *obj EINA_UNUSED, Elm_Bubble_Data *sd)
{
   return sd->pos;
}

/**
 * @internal
 * @brief Class constructor for Elm_Bubble.
 *
 * This function is called once when the class is initialized.
 * It registers the legacy type name for the widget.
 *
 * @param klass The Efl_Class for Elm_Bubble.
 */
EOLIAN static void
_elm_bubble_class_constructor(Efl_Class *klass)
{
   evas_smart_legacy_type_register(MY_CLASS_NAME_LEGACY, klass);
}

/* Efl.Part begin */
ELM_PART_OVERRIDE(elm_bubble, ELM_BUBBLE, Elm_Bubble_Data)
ELM_PART_OVERRIDE_TEXT_SET(elm_bubble, ELM_BUBBLE, Elm_Bubble_Data)

#include "elm_bubble_part.eo.c"
/* Efl.Part end */

/* Internal EO APIs and hidden overrides */

EFL_UI_LAYOUT_CONTENT_ALIASES_IMPLEMENT(MY_CLASS_PFX)
EFL_UI_LAYOUT_TEXT_ALIASES_IMPLEMENT(MY_CLASS_PFX)

#define ELM_BUBBLE_EXTRA_OPS \
   EFL_UI_LAYOUT_CONTENT_ALIASES_OPS(MY_CLASS_PFX), \
   EFL_UI_LAYOUT_TEXT_ALIASES_OPS(MY_CLASS_PFX), \
   EFL_CANVAS_GROUP_ADD_OPS(elm_bubble)

#include "elm_bubble_eo.c"
