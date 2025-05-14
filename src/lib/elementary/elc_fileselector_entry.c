//FIXME this widget should inherit from file selector button
#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define EFL_ACCESS_OBJECT_PROTECTED
#define EFL_PART_PROTECTED

#include <Elementary.h>
#include "Eio.h"
#include "elm_priv.h"
#include "elm_fileselector_button_eo.h"
#include "elm_fileselector_entry_eo.h"
#include "elm_interface_fileselector.h"
#include "elm_widget_fileselector_entry.h"
#include "elm_entry_eo.h"
#include "elm_fileselector_eo.h"

#include "elm_fileselector_entry_part.eo.h"
#include "elm_part_helper.h"

#define MY_CLASS ELM_FILESELECTOR_ENTRY_CLASS

#define MY_CLASS_NAME "Elm_Fileselector_Entry"
#define MY_CLASS_NAME_LEGACY "elm_fileselector_entry"

#define ELM_PRIV_FILESELECTOR_ENTRY_SIGNALS(cmd) \
   cmd(SIG_CHANGED, "changed", "") \
   cmd(SIG_ACTIVATED, "activated", "") \
   cmd(SIG_PRESS, "press", "") \
   cmd(SIG_LONGPRESSED, "longpressed", "") \
   cmd(SIG_CLICKED, "clicked", "") \
   cmd(SIG_CLICKED_DOUBLE, "clicked,double", "") \
   cmd(SIG_FOCUSED, "focused", "") \
   cmd(SIG_UNFOCUSED, "unfocused", "") \
   cmd(SIG_SELECTION_PASTE, "selection,paste", "") \
   cmd(SIG_SELECTION_COPY, "selection,copy", "") \
   cmd(SIG_SELECTION_CUT, "selection,cut", "") \
   cmd(SIG_UNPRESSED, "unpressed", "") \
   cmd(SIG_FILE_CHOSEN, "file,chosen", "s") \

ELM_PRIV_FILESELECTOR_ENTRY_SIGNALS(ELM_PRIV_STATIC_VARIABLE_DECLARE);

static const Evas_Smart_Cb_Description _smart_callbacks[] =
{
   ELM_PRIV_FILESELECTOR_ENTRY_SIGNALS(ELM_PRIV_SMART_CALLBACKS_DESC)
   {SIG_WIDGET_LANG_CHANGED, ""}, /**< handled by elm_widget */
   {SIG_WIDGET_ACCESS_CHANGED, ""}, /**< handled by elm_widget */
   {NULL, NULL}
};
#undef ELM_PRIV_FILESELECTOR_ENTRY_SIGNALS

#define SIG_FWD(name, event)                                                      \
  static void                                                               \
  _##name##_fwd(void *data, const Efl_Event *ev EINA_UNUSED)                                          \
  {                                                                         \
     efl_event_callback_legacy_call(data, event, ev->info);          \
  }

/**
 * @internal
 * @brief Forwards the CHANGED event from the internal entry to the fileselector entry.
 * @param data The fileselector entry object.
 * @param ev The Efl_Event.
 */
SIG_FWD(CHANGED, ELM_FILESELECTOR_ENTRY_EVENT_CHANGED)

/**
 * @internal
 * @brief Forwards the PRESS event from the internal entry to the fileselector entry.
 * @param data The fileselector entry object.
 * @param ev The Efl_Event.
 */
SIG_FWD(PRESS, ELM_FILESELECTOR_ENTRY_EVENT_PRESS)

/**
 * @internal
 * @brief Forwards the SELECTION_PASTE event from the internal entry to the fileselector entry.
 * @param data The fileselector entry object.
 * @param ev The Efl_Event.
 */
SIG_FWD(SELECTION_PASTE, EFL_UI_TEXTBOX_EVENT_SELECTION_PASTE)

/**
 * @internal
 * @brief Forwards the SELECTION_COPY event from the internal entry to the fileselector entry.
 * @param data The fileselector entry object.
 * @param ev The Efl_Event.
 */
SIG_FWD(SELECTION_COPY, EFL_UI_TEXTBOX_EVENT_SELECTION_COPY)

/**
 * @internal
 * @brief Forwards the SELECTION_CUT event from the internal entry to the fileselector entry.
 * @param data The fileselector entry object.
 * @param ev The Efl_Event.
 */
SIG_FWD(SELECTION_CUT, EFL_UI_TEXTBOX_EVENT_SELECTION_CUT)
#undef SIG_FWD

#define SIG_FWD(name, event)                                                      \
  static void                                                               \
  _##name##_fwd(void *data, Evas_Object *obj EINA_UNUSED, void *event_info)             \
  {                                                                         \
     evas_object_smart_callback_call(data, event, event_info);              \
  }

/**
 * @internal
 * @brief Forwards the "clicked" smart callback from the internal entry or button to the fileselector entry.
 * @param data The fileselector entry object.
 * @param obj The Evas_Object that triggered the event (unused).
 * @param event_info Event-specific data.
 */
SIG_FWD(CLICKED, "clicked")

/**
 * @internal
 * @brief Forwards the "clicked,double" smart callback from the internal entry to the fileselector entry.
 * @param data The fileselector entry object.
 * @param obj The Evas_Object that triggered the event (unused).
 * @param event_info Event-specific data.
 */
SIG_FWD(CLICKED_DOUBLE, "clicked,double")

/**
 * @internal
 * @brief Forwards the "unpressed" smart callback from the internal button to the fileselector entry.
 * @param data The fileselector entry object.
 * @param obj The Evas_Object that triggered the event (unused).
 * @param event_info Event-specific data.
 */
SIG_FWD(UNPRESSED, "unpressed")

/**
 * @internal
 * @brief Forwards the "longpressed" smart callback from the internal entry to the fileselector entry.
 * @param data The fileselector entry object.
 * @param obj The Evas_Object that triggered the event (unused).
 * @param event_info Event-specific data.
 */
SIG_FWD(LONGPRESSED, "longpressed")
#undef SIG_FWD

/**
 * @internal
 * @brief Callback function for the "file,chosen" event from the internal fileselector button.
 *
 * This function is triggered when a file is selected in the fileselector.
 * It updates the entry's text with the chosen file path and emits the
 * ELM_FILESELECTOR_ENTRY_EVENT_FILE_CHOSEN event.
 *
 * @param data The fileselector entry object (Eo *fs).
 * @param event The Efl_Event structure containing the event information.
 *              The event->info is an Efl_Model representing the chosen file.
 */
static void
_FILE_CHOSEN_fwd(void *data, const Efl_Event *event)
{
   Efl_Model *model = event->info;
   Eo *fs = data;
   Eina_Value *path;
   char *file = NULL;
   ELM_FILESELECTOR_ENTRY_DATA_GET(fs, sd);

   efl_ui_view_model_set(sd->entry, model);
   efl_ui_property_bind(sd->entry, "default", "path");

   path = efl_model_property_get(model, "path");
   file = eina_value_to_string(path);

   _model_event_call
     (fs, ELM_FILESELECTOR_ENTRY_EVENT_FILE_CHOSEN, ELM_FILESELECTOR_ENTRY_EVENT_FILE_CHOSEN->name, model, file);

   eina_value_free(path);
   free(file);
}

/**
 * @internal
 * @brief Callback function for the "activated" event from the internal entry widget.
 *
 * This function is triggered when the entry is activated (e.g., by pressing Enter).
 * It retrieves the text from the entry, updates the model of the internal button
 * if it exists, and then forwards the ELM_FILESELECTOR_ENTRY_EVENT_ACTIVATED event.
 *
 * @param data The fileselector entry object.
 * @param event The Efl_Event structure containing the event information.
 */
static void
_ACTIVATED_fwd(void *data, const Efl_Event *event)
{
   const char *file;
   Efl_Model *bmodel, *model;

   ELM_FILESELECTOR_ENTRY_DATA_GET(data, sd);

   file = elm_object_text_get(sd->entry);

   bmodel = efl_ui_view_model_get(sd->button);
   if (bmodel)
     {
        model = efl_add(efl_class_get(bmodel), sd->button,
                        efl_io_model_path_set(efl_added, file));
        efl_ui_view_model_set(sd->button, model);
     }

   efl_event_callback_legacy_call
     (data, ELM_FILESELECTOR_ENTRY_EVENT_ACTIVATED, event->info);
}

/**
 * @internal
 * @brief Applies the theme to the fileselector entry widget.
 *
 * This function is called when the theme of the widget needs to be updated.
 * It applies the style to the base widget and its internal components (button and entry).
 *
 * @param obj The fileselector entry object.
 * @param sd The private data of the fileselector entry.
 * @return Eina_Error EFL_UI_THEME_APPLY_ERROR_NONE on success, or an error code otherwise.
 */
EOLIAN static Eina_Error
_elm_fileselector_entry_efl_ui_widget_theme_apply(Eo *obj, Elm_Fileselector_Entry_Data *sd)
{
   const char *style;
   char buf[1024];

   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd, EFL_UI_THEME_APPLY_ERROR_GENERIC);

   Eina_Error int_ret = EFL_UI_THEME_APPLY_ERROR_GENERIC;
   int_ret = efl_ui_widget_theme_apply(efl_super(obj, MY_CLASS));
   if (int_ret == EFL_UI_THEME_APPLY_ERROR_GENERIC) return int_ret;

   style = elm_widget_style_get(obj);

   efl_ui_mirrored_set(sd->button, efl_ui_mirrored_get(obj));

   if (elm_object_disabled_get(obj))
     elm_layout_signal_emit(obj, "elm,state,disabled", "elm");

   if (!style) style = "default";
   snprintf(buf, sizeof(buf), "fileselector_entry/%s", style);
   elm_widget_style_set(sd->button, buf);
   elm_widget_style_set(sd->entry, buf);

   edje_object_message_signal_process(wd->resize_obj);

   elm_layout_sizing_eval(obj);

   return int_ret;
}

/**
 * @internal
 * @brief Sets the text for a specific part of the fileselector entry.
 *
 * If @p part is "elm.text" or NULL, it sets the text of the internal button.
 * Otherwise, it attempts to set the text of the specified part in the superclass.
 *
 * @param obj The fileselector entry object.
 * @param sd The private data of the fileselector entry.
 * @param part The name of the part to set the text for (e.g., "elm.text").
 * @param label The text to set.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_fileselector_entry_text_set(Eo *obj, Elm_Fileselector_Entry_Data *sd, const char *part, const char *label)
{
   if (part && strcmp(part, "elm.text"))
     {
        efl_text_set(efl_part(efl_super(obj, MY_CLASS), part), label);
     }

   elm_object_text_set(sd->button, label);
   return EINA_TRUE;
}

/**
 * @internal
 * @brief Gets the text from a specific part of the fileselector entry.
 *
 * If @p part is "elm.text" or NULL, it gets the text of the internal button.
 * Otherwise, it attempts to get the text of the specified part from the superclass.
 *
 * @param obj The fileselector entry object.
 * @param sd The private data of the fileselector entry.
 * @param part The name of the part to get the text from (e.g., "elm.text").
 * @return The text of the specified part, or NULL on failure.
 */
static const char *
_elm_fileselector_entry_text_get(Eo *obj, Elm_Fileselector_Entry_Data *sd, const char *part)
{
   if (part && strcmp(part, "elm.text"))
     {
        const char *text = NULL;
        text = efl_text_get(efl_part(efl_super(obj, MY_CLASS), part));
        return text;
     }

   return elm_object_text_get(sd->button);
}

/**
 * @internal
 * @brief Sets the content for a specific part of the fileselector entry.
 *
 * If @p part is "button icon" or NULL, it sets the content of the internal button's icon.
 * Otherwise, it attempts to set the content of the specified part in the superclass.
 *
 * @param obj The fileselector entry object.
 * @param sd The private data of the fileselector entry.
 * @param part The name of the part to set the content for (e.g., "button icon").
 * @param content The Evas_Object to set as content.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_fileselector_entry_content_set(Eo *obj, Elm_Fileselector_Entry_Data *sd, const char *part, Evas_Object *content)
{
   if (part && strcmp(part, "button icon"))
     {
        return efl_content_set(efl_part(efl_super(obj, MY_CLASS), part), content);
     }

   elm_layout_content_set(sd->button, NULL, content);

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Gets the content from a specific part of the fileselector entry.
 *
 * If @p part is "button icon" or NULL, it gets the content of the internal button's icon.
 * Otherwise, it attempts to get the content of the specified part from the superclass.
 *
 * @param obj The fileselector entry object.
 * @param sd The private data of the fileselector entry.
 * @param part The name of the part to get the content from (e.g., "button icon").
 * @return The Evas_Object content of the specified part, or NULL on failure or if not set.
 */
static Evas_Object *
_elm_fileselector_entry_content_get(Eo *obj, Elm_Fileselector_Entry_Data *sd, const char *part)
{
   if (part && strcmp(part, "button icon"))
     {
        return efl_content_get(efl_part(efl_super(obj, MY_CLASS), part));
     }

   return elm_layout_content_get(sd->button, NULL);
}

/**
 * @internal
 * @brief Unsets (removes) the content from a specific part of the fileselector entry.
 *
 * If @p part is "button icon" or NULL, it unsets the content of the internal button's icon.
 * Otherwise, it attempts to unset the content of the specified part in the superclass.
 *
 * @param obj The fileselector entry object.
 * @param sd The private data of the fileselector entry.
 * @param part The name of the part to unset the content from (e.g., "button icon").
 * @return The previously set Evas_Object content, or NULL if none was set or on failure.
 */
static Evas_Object *
_elm_fileselector_entry_content_unset(Eo *obj, Elm_Fileselector_Entry_Data *sd, const char *part)
{
   if (part && strcmp(part, "button icon"))
     {
        return efl_content_unset(efl_part(efl_super(obj, MY_CLASS), part));
     }

   return elm_layout_content_unset(sd->button, NULL);
}

/**
 * @internal
 * @brief Adds the internal Evas objects (button and entry) to the fileselector entry.
 *
 * This function is called during the widget's construction. It creates and
 * configures the internal fileselector button and entry widgets, sets up
 * signal forwarding, and themes them.
 *
 * @param obj The fileselector entry object.
 * @param priv The private data of the fileselector entry.
 */
EOLIAN static void
_elm_fileselector_entry_efl_canvas_group_group_add(Eo *obj, Elm_Fileselector_Entry_Data *priv)
{
   efl_canvas_group_add(efl_super(obj, MY_CLASS));

   priv->button = elm_fileselector_button_add(obj);
   efl_ui_mirrored_automatic_set(priv->button, EINA_FALSE);
   efl_ui_mirrored_set(priv->button, efl_ui_mirrored_get(obj));
   elm_widget_style_set(priv->button, "fileselector_entry/default");
   efl_composite_attach(obj, priv->button);
   efl_ui_layout_finger_size_multiplier_set(obj, 0, 0);

   elm_fileselector_expandable_set
     (priv->button, _elm_config->fileselector_expand_enable);

#define SIG_FWD(name, event) \
  evas_object_smart_callback_add(priv->button, event, _##name##_fwd, obj)
   SIG_FWD(CLICKED, "clicked");
   SIG_FWD(UNPRESSED, "unpressed");
#undef SIG_FWD

#define SIG_FWD(name, event) \
  efl_event_callback_add(priv->button, event, _##name##_fwd, obj)
   SIG_FWD(FILE_CHOSEN, ELM_FILESELECTOR_BUTTON_EVENT_FILE_CHOSEN);
#undef SIG_FWD

   priv->entry = elm_entry_add(obj);
   elm_entry_scrollable_set(priv->entry, EINA_TRUE);
   efl_ui_mirrored_automatic_set(priv->entry, EINA_FALSE);
   elm_widget_style_set(priv->entry, "fileselector_entry/default");
   elm_entry_single_line_set(priv->entry, EINA_TRUE);
   elm_entry_editable_set(priv->entry, EINA_TRUE);

#define SIG_FWD(name, event) \
  efl_event_callback_add(priv->entry, event, _##name##_fwd, obj)
   SIG_FWD(CHANGED, ELM_ENTRY_EVENT_CHANGED);
   SIG_FWD(ACTIVATED, ELM_ENTRY_EVENT_ACTIVATED);
   SIG_FWD(PRESS, ELM_ENTRY_EVENT_PRESS);
   SIG_FWD(SELECTION_PASTE, EFL_UI_TEXTBOX_EVENT_SELECTION_PASTE);
   SIG_FWD(SELECTION_COPY, EFL_UI_TEXTBOX_EVENT_SELECTION_COPY);
   SIG_FWD(SELECTION_CUT, EFL_UI_TEXTBOX_EVENT_SELECTION_CUT);
#undef SIG_FWD
#define SIG_FWD(name, event) \
  evas_object_smart_callback_add(priv->entry, event, _##name##_fwd, obj)
   SIG_FWD(LONGPRESSED, "longpressed");
   SIG_FWD(CLICKED, "clicked");
   SIG_FWD(CLICKED_DOUBLE, "clicked,double");
#undef SIG_FWD
   efl_event_callback_forwarder_add(priv->entry, EFL_UI_FOCUS_OBJECT_EVENT_FOCUS_CHANGED, obj);

   if (!elm_layout_theme_set
       (obj, "fileselector_entry", "base", elm_widget_style_get(obj)))
     CRI("Failed to set layout!");
   else
     {
        elm_layout_content_set(obj, "elm.swallow.button", priv->button);
        elm_layout_content_set(obj, "elm.swallow.entry", priv->entry);
     }

   elm_layout_sizing_eval(obj);
}

/**
 * @internal
 * @brief Handles the deletion of the fileselector entry's canvas group.
 *
 * This function is called when the widget is being deleted. It frees
 * any allocated resources, specifically the cached path string.
 *
 * @param obj The fileselector entry object.
 * @param sd The private data of the fileselector entry.
 */
EOLIAN static void
_elm_fileselector_entry_efl_canvas_group_group_del(Eo *obj, Elm_Fileselector_Entry_Data *sd)
{
   free(sd->path);

   efl_canvas_group_del(efl_super(obj, MY_CLASS));
}

EAPI Evas_Object *
elm_fileselector_entry_add(Evas_Object *parent)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);
   return elm_legacy_add(MY_CLASS, parent);
}

/**
 * @internal
 * @brief Constructor for the Elm_Fileselector_Entry object.
 *
 * This function is called when a new fileselector entry object is created.
 * It initializes the object, sets its legacy type name, registers smart callbacks,
 * and sets its accessibility role.
 *
 * @param obj The fileselector entry object being constructed.
 * @param sd The private data of the fileselector entry (unused in this function).
 * @return The constructed fileselector entry object.
 */
EOLIAN static Eo *
_elm_fileselector_entry_efl_object_constructor(Eo *obj, Elm_Fileselector_Entry_Data *sd EINA_UNUSED)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));
   efl_canvas_object_type_set(obj, MY_CLASS_NAME_LEGACY);
   evas_object_smart_callbacks_descriptions_set(obj, _smart_callbacks);
   efl_access_object_role_set(obj, EFL_ACCESS_ROLE_GROUPING);
   legacy_child_focus_handle(obj);

   return obj;
}

EINA_DEPRECATED EAPI void
elm_fileselector_entry_selected_set(Evas_Object *obj, const char *path)
{
   ELM_FILESELECTOR_INTERFACE_CHECK(obj);
   elm_fileselector_selected_set(obj, path);
}

/**
 * @internal
 * @brief Internal implementation for setting the selected path.
 * @param obj The fileselector entry object.
 * @param path The file path to set.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
Eina_Bool
_elm_fileselector_entry_selected_set_internal(Evas_Object *obj, const char *path)
{
   ELM_FILESELECTOR_ENTRY_DATA_GET_OR_RETURN_VAL(obj, sd, EINA_FALSE);
   elm_fileselector_path_set(sd->button, path);
   return EINA_TRUE;
}

/**
 * @internal
 * @brief Sets the selected file model for the fileselector entry.
 * This function directly sets the model on the internal fileselector button.
 * @param obj The fileselector entry object (unused).
 * @param sd The private data of the fileselector entry.
 * @param model The Efl_Model representing the file to be selected.
 * @return EINA_TRUE on success.
 */
EOLIAN static Eina_Bool
_elm_fileselector_entry_elm_interface_fileselector_selected_model_set(Eo *obj EINA_UNUSED,
                                                                      Elm_Fileselector_Entry_Data *sd,
                                                                      Efl_Model *model)
{
   efl_ui_view_model_set(sd->button, model);

   return EINA_TRUE;
}

EINA_DEPRECATED EAPI const char *
elm_fileselector_entry_selected_get(const Evas_Object *obj)
{
   ELM_FILESELECTOR_INTERFACE_CHECK(obj, NULL);
   return elm_fileselector_selected_get((Eo *) obj);
}

/**
 * @internal
 * @brief Internal implementation for getting the selected path.
 * @param obj The fileselector entry object.
 * @return The currently selected file path, or NULL.
 */
const char *
_elm_fileselector_entry_selected_get_internal(const Evas_Object *obj)
{
   ELM_FILESELECTOR_ENTRY_DATA_GET_OR_RETURN_VAL(obj, sd, NULL);
   return elm_fileselector_path_get(sd->button);
}

/**
 * @internal
 * @brief Gets the selected file model from the fileselector entry.
 * This function retrieves the model directly from the internal fileselector button.
 * @param obj The fileselector entry object (unused).
 * @param sd The private data of the fileselector entry.
 * @return The Efl_Model representing the currently selected file, or NULL.
 */
EOLIAN static Efl_Model *
_elm_fileselector_entry_elm_interface_fileselector_selected_model_get(const Eo *obj EINA_UNUSED, Elm_Fileselector_Entry_Data *sd)
{
   return efl_ui_view_model_get(sd->button);
}

EAPI void
elm_fileselector_entry_window_title_set(Eo *obj, const char *title)
{
   ELM_FILESELECTOR_ENTRY_CHECK(obj);
   ELM_FILESELECTOR_ENTRY_DATA_GET_OR_RETURN(obj, sd);
   elm_fileselector_button_window_title_set(sd->button, title);
}

EAPI const char *
elm_fileselector_entry_window_title_get(const Eo *obj)
{
   ELM_FILESELECTOR_ENTRY_CHECK(obj) NULL;
   ELM_FILESELECTOR_ENTRY_DATA_GET_OR_RETURN_VAL(obj, sd, NULL);
   return elm_fileselector_button_window_title_get(sd->button);
}

EAPI void
elm_fileselector_entry_window_size_set(Eo *obj, Evas_Coord width, Evas_Coord height)
{
   ELM_FILESELECTOR_ENTRY_CHECK(obj);
   ELM_FILESELECTOR_ENTRY_DATA_GET_OR_RETURN(obj, sd);
   elm_fileselector_button_window_size_set(sd->button, width, height);
}

EAPI void
elm_fileselector_entry_window_size_get(const Eo *obj, Evas_Coord *width, Evas_Coord *height)
{
   if (width) *width = 0;
   if (height) *height = 0;
   ELM_FILESELECTOR_ENTRY_CHECK(obj);
   ELM_FILESELECTOR_ENTRY_DATA_GET_OR_RETURN(obj, sd);
   elm_fileselector_button_window_size_get(sd->button, width, height);
}

EINA_DEPRECATED EAPI void
elm_fileselector_entry_path_set(Evas_Object *obj,
                                const char *path)
{
   ELM_FILESELECTOR_INTERFACE_CHECK(obj);
   elm_fileselector_path_set(obj, path);
}

/**
 * @internal
 * @brief Internal implementation for setting the path displayed in the entry and button.
 *
 * This function sets the path for the internal fileselector button and updates
 * the text of the internal entry widget with a markup version of the path.
 *
 * @param obj The fileselector entry object.
 * @param path The file system path to set.
 */
void
_elm_fileselector_entry_path_set_internal(Evas_Object *obj, const char *path)
{
   ELM_FILESELECTOR_ENTRY_DATA_GET_OR_RETURN(obj, sd);
   char *s;

   elm_fileselector_path_set(sd->button, path);

   s = elm_entry_utf8_to_markup(path);
   if (s)
     {
        elm_object_text_set(sd->entry, s);
        free(s);
     }
}

/**
 * @internal
 * @brief Sets the Efl_Model for the fileselector entry.
 *
 * This function sets the provided model to both the internal fileselector button
 * and the internal entry. It also binds the "path" property of the model
 * to the "default" text property of the entry.
 * It expects the model to be an Efl_Io_Model.
 *
 * @param obj The fileselector entry object (unused).
 * @param sd The private data of the fileselector entry.
 * @param model The Efl_Model to set. Must be an Efl_Io_Model.
 */
EOLIAN static void
_elm_fileselector_entry_efl_ui_view_model_set(Eo *obj EINA_UNUSED, Elm_Fileselector_Entry_Data *sd, Efl_Model *model)
{
   if (!efl_isa(model, EFL_IO_MODEL_CLASS))
     return ;
   efl_ui_view_model_set(sd->button, model);
   efl_ui_view_model_set(sd->entry, model);
   efl_ui_property_bind(sd->entry, "default", "path");
}

EINA_DEPRECATED EAPI const char *
elm_fileselector_entry_path_get(const Evas_Object *obj)
{
   ELM_FILESELECTOR_INTERFACE_CHECK(obj, NULL);
   return elm_fileselector_path_get(obj);
}

/**
 * @internal
 * @brief Internal implementation for getting the path currently displayed in the entry.
 *
 * This function retrieves the text from the internal entry widget, converts it
 * from markup to UTF-8, and returns it. The returned string is stored in sd->path
 * and should not be freed by the caller; it will be freed on the next call or
 * when the widget is destroyed.
 *
 * @param obj The fileselector entry object.
 * @return The file system path currently in the entry, or NULL on failure.
 */
const char *
_elm_fileselector_entry_path_get_internal(const Evas_Object *obj)
{
   ELM_FILESELECTOR_ENTRY_DATA_GET_OR_RETURN_VAL(obj, sd, NULL);
   free(sd->path);
   sd->path = elm_entry_markup_to_utf8(elm_object_text_get(sd->entry));
   return sd->path;
}

/**
 * @internal
 * @brief Gets the Efl_Model representing the current path in the fileselector entry.
 *
 * This function retrieves the model from the internal fileselector button.
 * If the path in the entry widget differs from the path in the button's model,
 * a new volatile model is created with the entry's path.
 *
 * @param obj The fileselector entry object.
 * @param sd The private data of the fileselector entry.
 * @return An Efl_Model representing the current path. This model might be
 *         the button's model or a new volatile model. The new model is
 *         managed by a postponed free queue. Returns NULL if no base model exists.
 */
EOLIAN static Efl_Model *
_elm_fileselector_entry_efl_ui_view_model_get(const Eo *obj, Elm_Fileselector_Entry_Data *sd)
{
   Efl_Model *bmodel, *ret;

   bmodel = efl_ui_view_model_get(sd->button);
   if (!bmodel)
     {
        WRN("no base Efl.Model");
        return NULL;
     }

   free(sd->path);
   sd->path = elm_entry_markup_to_utf8(elm_object_text_get(sd->entry));

   if (eina_streq(sd->path, efl_io_model_path_get(bmodel)))
     return bmodel;

   ret = efl_add_ref(efl_class_get(bmodel), (Eo*) obj,
                     efl_io_model_path_set(efl_added, sd->path),
                     efl_loop_model_volatile_make(efl_added));
   eina_freeq_ptr_add(postponed_fq, ret, EINA_FREE_CB(efl_unref), 0);

   return ret;
}

EINA_DEPRECATED EAPI void
elm_fileselector_entry_expandable_set(Evas_Object *obj,
                                      Eina_Bool value)
{
   ELM_FILESELECTOR_INTERFACE_CHECK(obj);
   elm_interface_fileselector_expandable_set(obj, value);
}

/**
 * @internal
 * @brief Sets whether the internal fileselector is expandable (shows a tree view).
 * @param obj The fileselector entry object (unused).
 * @param sd The private data of the fileselector entry.
 * @param value EINA_TRUE to make it expandable, EINA_FALSE otherwise.
 */
EOLIAN static void
_elm_fileselector_entry_elm_interface_fileselector_expandable_set(Eo *obj EINA_UNUSED, Elm_Fileselector_Entry_Data *sd, Eina_Bool value)
{
   elm_fileselector_expandable_set(sd->button, value);
}

EINA_DEPRECATED EAPI Eina_Bool
elm_fileselector_entry_expandable_get(const Evas_Object *obj)
{
   ELM_FILESELECTOR_INTERFACE_CHECK(obj, EINA_FALSE);
   return elm_interface_fileselector_expandable_get((Eo *) obj);
}

/**
 * @internal
 * @brief Gets whether the internal fileselector is expandable.
 * @param obj The fileselector entry object (unused).
 * @param sd The private data of the fileselector entry.
 * @return EINA_TRUE if expandable, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_elm_fileselector_entry_elm_interface_fileselector_expandable_get(const Eo *obj EINA_UNUSED, Elm_Fileselector_Entry_Data *sd)
{
   return elm_fileselector_expandable_get(sd->button);
}

EINA_DEPRECATED EAPI void
elm_fileselector_entry_folder_only_set(Evas_Object *obj,
                                       Eina_Bool value)
{
   ELM_FILESELECTOR_INTERFACE_CHECK(obj);
   elm_interface_fileselector_folder_only_set(obj, value);
}

/**
 * @internal
 * @brief Sets whether the internal fileselector shows only folders.
 * @param obj The fileselector entry object (unused).
 * @param sd The private data of the fileselector entry.
 * @param value EINA_TRUE to show only folders, EINA_FALSE otherwise.
 */
EOLIAN static void
_elm_fileselector_entry_elm_interface_fileselector_folder_only_set(Eo *obj EINA_UNUSED, Elm_Fileselector_Entry_Data *sd, Eina_Bool value)
{
   elm_fileselector_folder_only_set(sd->button, value);
}

EINA_DEPRECATED EAPI Eina_Bool
elm_fileselector_entry_folder_only_get(const Evas_Object *obj)
{
   ELM_FILESELECTOR_INTERFACE_CHECK(obj, EINA_FALSE);
   return elm_interface_fileselector_folder_only_get((Eo *) obj);
}

/**
 * @internal
 * @brief Gets whether the internal fileselector shows only folders.
 * @param obj The fileselector entry object (unused).
 * @param sd The private data of the fileselector entry.
 * @return EINA_TRUE if showing only folders, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_elm_fileselector_entry_elm_interface_fileselector_folder_only_get(const Eo *obj EINA_UNUSED, Elm_Fileselector_Entry_Data *sd)
{
   return elm_fileselector_folder_only_get(sd->button);
}

EINA_DEPRECATED EAPI void
elm_fileselector_entry_is_save_set(Evas_Object *obj,
                                   Eina_Bool value)
{
   ELM_FILESELECTOR_INTERFACE_CHECK(obj);
   elm_interface_fileselector_is_save_set(obj, value);
}

/**
 * @internal
 * @brief Sets whether the internal fileselector is in "save" mode.
 * In "save" mode, the fileselector allows entering new filenames.
 * @param obj The fileselector entry object (unused).
 * @param sd The private data of the fileselector entry.
 * @param value EINA_TRUE for "save" mode, EINA_FALSE otherwise.
 */
EOLIAN static void
_elm_fileselector_entry_elm_interface_fileselector_is_save_set(Eo *obj EINA_UNUSED, Elm_Fileselector_Entry_Data *sd, Eina_Bool value)
{
   elm_fileselector_is_save_set(sd->button, value);
}

EINA_DEPRECATED EAPI Eina_Bool
elm_fileselector_entry_is_save_get(const Evas_Object *obj)
{
   ELM_FILESELECTOR_INTERFACE_CHECK(obj, EINA_FALSE);
   return elm_interface_fileselector_is_save_get((Eo *) obj);
}

/**
 * @internal
 * @brief Gets whether the internal fileselector is in "save" mode.
 * @param obj The fileselector entry object (unused).
 * @param sd The private data of the fileselector entry.
 * @return EINA_TRUE if in "save" mode, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_elm_fileselector_entry_elm_interface_fileselector_is_save_get(const Eo *obj EINA_UNUSED, Elm_Fileselector_Entry_Data *sd)
{
   return elm_fileselector_is_save_get(sd->button);
}

EAPI void
elm_fileselector_entry_inwin_mode_set(Eo *obj, Eina_Bool value)
{
   ELM_FILESELECTOR_ENTRY_CHECK(obj);
   ELM_FILESELECTOR_ENTRY_DATA_GET_OR_RETURN(obj, sd);
   elm_fileselector_button_inwin_mode_set(sd->button, value);
}

EAPI Eina_Bool
elm_fileselector_entry_inwin_mode_get(const Eo *obj)
{
   ELM_FILESELECTOR_ENTRY_CHECK(obj) EINA_FALSE;
   ELM_FILESELECTOR_ENTRY_DATA_GET_OR_RETURN_VAL(obj, sd, EINA_FALSE);
   return elm_fileselector_button_inwin_mode_get(sd->button);
}

/**
 * @internal
 * @brief Class constructor for Elm_Fileselector_Entry.
 *
 * This function is called when the Elm_Fileselector_Entry class is being set up.
 * It registers the legacy type name for the class.
 *
 * @param klass The Efl_Class for Elm_Fileselector_Entry.
 */
EOLIAN static void
_elm_fileselector_entry_class_constructor(Efl_Class *klass)
{
   evas_smart_legacy_type_register(MY_CLASS_NAME_LEGACY, klass);
}

/* Efl.Part begin */

ELM_PART_OVERRIDE(elm_fileselector_entry, ELM_FILESELECTOR_ENTRY, Elm_Fileselector_Entry_Data)
ELM_PART_OVERRIDE_CONTENT_SET(elm_fileselector_entry, ELM_FILESELECTOR_ENTRY, Elm_Fileselector_Entry_Data)
ELM_PART_OVERRIDE_CONTENT_GET(elm_fileselector_entry, ELM_FILESELECTOR_ENTRY, Elm_Fileselector_Entry_Data)
ELM_PART_OVERRIDE_CONTENT_UNSET(elm_fileselector_entry, ELM_FILESELECTOR_ENTRY, Elm_Fileselector_Entry_Data)
ELM_PART_OVERRIDE_TEXT_SET(elm_fileselector_entry, ELM_FILESELECTOR_ENTRY, Elm_Fileselector_Entry_Data)
ELM_PART_OVERRIDE_TEXT_GET(elm_fileselector_entry, ELM_FILESELECTOR_ENTRY, Elm_Fileselector_Entry_Data)
ELM_PART_CONTENT_DEFAULT_GET(elm_fileselector_entry, "button icon")
#include "elm_fileselector_entry_part.eo.c"

/* Efl.Part end */

/* Internal EO APIs and hidden overrides */

#define ELM_FILESELECTOR_ENTRY_EXTRA_OPS \
   ELM_PART_CONTENT_DEFAULT_OPS(elm_fileselector_entry), \
   EFL_CANVAS_GROUP_ADD_DEL_OPS(elm_fileselector_entry)

#include "elm_fileselector_entry_eo.c"
