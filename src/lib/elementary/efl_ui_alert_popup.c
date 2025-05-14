#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define EFL_PART_PROTECTED

#include <Elementary.h>

#include "elm_priv.h"
#include "efl_ui_alert_popup_private.h"
#include "efl_ui_alert_popup_part_title.eo.h"
#include "elm_part_helper.h"

#define MY_CLASS EFL_UI_ALERT_POPUP_CLASS
#define MY_CLASS_NAME "Efl.Ui.Alert_Popup"

// Constant for the "button" part name.
static const char PART_NAME_BUTTON[] = "button";
// Array of part names for button layouts, indexed by button count (1, 2, or 3).
// e.g., "button_layout1" for one button, "button_layout2" for two buttons.
static const char PART_NAME_BUTTON_LAYOUT[EFL_UI_ALERT_POPUP_BUTTON_COUNT][15] =
                                                {"button_layout1",
                                                 "button_layout2",
                                                 "button_layout3"};

// Array of swallow names for buttons in the layout.
// These correspond to "efl.button1", "efl.button2", "efl.button3" swallow parts in the theme.
static const char BUTTON_SWALLOW_NAME[EFL_UI_ALERT_POPUP_BUTTON_COUNT][20] =
                                                {"efl.button1",
                                                 "efl.button2",
                                                 "efl.button3"};

// Defines aliases for text parts. "title" maps to "efl.text.title".
static const Elm_Layout_Part_Alias_Description _text_aliases[] =
{
   {"title", "efl.text.title"},
   {NULL, NULL}
};

/**
 * @brief Sets the text for a given part of the alert popup.
 *
 * This function handles text setting for aliased parts, specifically the title.
 * It updates the internal stringshare for the title and emits signals
 * for visibility changes.
 *
 * @param obj The Efl.Ui.Alert_Popup object.
 * @param pd The private data of the Efl.Ui.Alert_Popup object.
 * @param part The name of the part to set text for (e.g., "title").
 * @param label The text to set.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_efl_ui_alert_popup_text_set(Eo *obj, Efl_Ui_Alert_Popup_Data *pd, const char *part, const char *label)
{
   if (!_elm_layout_part_aliasing_eval(obj, &part, EINA_TRUE))
      return EINA_FALSE;
   efl_text_set(efl_part(efl_super(obj, MY_CLASS), part), label);
   if (eina_streq(part, "efl.text.title"))
     {
        Eina_Bool changed = eina_stringshare_replace(&pd->title_text, label);
        if (changed)
          {
             efl_text_set(efl_part(efl_super(obj, MY_CLASS), part), label);
             if (label)
               elm_layout_signal_emit(obj, "efl,title,visible,on", "efl");
             else
               elm_layout_signal_emit(obj, "efl,title,visible,off", "efl");

             ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd, EINA_FALSE);
             edje_object_message_signal_process(wd->resize_obj);
             efl_canvas_group_change(obj);
          }
     }

   return EINA_TRUE;
}

/**
 * @brief Gets the text for a given part of the alert popup.
 *
 * This function handles text retrieval for aliased parts, specifically the title.
 *
 * @param obj The Efl.Ui.Alert_Popup object.
 * @param pd The private data of the Efl.Ui.Alert_Popup object.
 * @param part The name of the part to get text from (e.g., "title").
 * @return The text of the part, or NULL if not found or on error.
 */
const char *
_efl_ui_alert_popup_text_get(Eo *obj EINA_UNUSED, Efl_Ui_Alert_Popup_Data *pd, const char *part)
{
   if (!_elm_layout_part_aliasing_eval(obj, &part, EINA_TRUE))
      return NULL;
   if (eina_streq(part, "efl.text.title"))
     {
        if (pd->title_text)
          return pd->title_text;

        return NULL;
     }

   return efl_text_get(efl_part(efl_super(obj, MY_CLASS), part));
}

/**
 * @brief Callback function for when the positive button is clicked.
 *
 * Emits the EFL_UI_ALERT_POPUP_EVENT_BUTTON_CLICKED event with
 * button_type set to EFL_UI_ALERT_POPUP_BUTTON_POSITIVE.
 *
 * @param data The Efl.Ui.Alert_Popup object.
 * @param ev The Efl_Event data (unused).
 */
static void
_positive_button_clicked_cb(void *data, const Efl_Event *ev EINA_UNUSED)
{
   Eo *popup_obj = data;

   Efl_Ui_Alert_Popup_Button_Clicked_Event event;
   event.button_type = EFL_UI_ALERT_POPUP_BUTTON_POSITIVE;

   efl_event_callback_call(popup_obj, EFL_UI_ALERT_POPUP_EVENT_BUTTON_CLICKED, &event);
}

/**
 * @brief Callback function for when the negative button is clicked.
 *
 * Emits the EFL_UI_ALERT_POPUP_EVENT_BUTTON_CLICKED event with
 * button_type set to EFL_UI_ALERT_POPUP_BUTTON_NEGATIVE.
 *
 * @param data The Efl.Ui.Alert_Popup object.
 * @param ev The Efl_Event data (unused).
 */
static void
_negative_button_clicked_cb(void *data, const Efl_Event *ev EINA_UNUSED)
{
   Eo *popup_obj = data;

   Efl_Ui_Alert_Popup_Button_Clicked_Event event;
   event.button_type = EFL_UI_ALERT_POPUP_BUTTON_NEGATIVE;

   efl_event_callback_call(popup_obj, EFL_UI_ALERT_POPUP_EVENT_BUTTON_CLICKED, &event);
}

/**
 * @brief Callback function for when the user-defined button is clicked.
 *
 * Emits the EFL_UI_ALERT_POPUP_EVENT_BUTTON_CLICKED event with
 * button_type set to EFL_UI_ALERT_POPUP_BUTTON_USER.
 *
 * @param data The Efl.Ui.Alert_Popup object.
 * @param ev The Efl_Event data (unused).
 */
static void
_user_button_clicked_cb(void *data, const Efl_Event *ev EINA_UNUSED)
{
   Eo *popup_obj = data;

   Efl_Ui_Alert_Popup_Button_Clicked_Event event;
   event.button_type = EFL_UI_ALERT_POPUP_BUTTON_USER;

   efl_event_callback_call(popup_obj, EFL_UI_ALERT_POPUP_EVENT_BUTTON_CLICKED, &event);
}

/**
 * @brief Applies the appropriate style to buttons based on their count and type.
 *
 * This function ensures buttons are styled correctly (e.g., "left_button", "right_button")
 * depending on how many buttons are visible and which specific buttons they are.
 * For example, if there are two buttons, the user button (if present) might be styled
 * as "left_button" and the positive button as "right_button".
 *
 * @param obj The Efl.Ui.Alert_Popup object.
 * @param pd The private data of the Efl.Ui.Alert_Popup object.
 * @param button_cnt The total number of currently visible buttons.
 */
static void
_apply_button_style(Eo *obj, Efl_Ui_Alert_Popup_Data *pd, int button_cnt)
{
   if (pd->button[EFL_UI_ALERT_POPUP_BUTTON_USER])
     {
        if (button_cnt > 1)
          elm_widget_element_update(obj,
                                    pd->button[EFL_UI_ALERT_POPUP_BUTTON_USER],
                                    "left_button");
     }

   if (pd->button[EFL_UI_ALERT_POPUP_BUTTON_POSITIVE])
     {
        if (button_cnt == 2)
          {
             if (pd->button[EFL_UI_ALERT_POPUP_BUTTON_USER])
               elm_widget_element_update(obj,
                                         pd->button[EFL_UI_ALERT_POPUP_BUTTON_POSITIVE],
                                         "right_button");
             else
               elm_widget_element_update(obj,
                                         pd->button[EFL_UI_ALERT_POPUP_BUTTON_POSITIVE],
                                         "left_button");
          }
     }

   if (pd->button[EFL_UI_ALERT_POPUP_BUTTON_NEGATIVE])
     {
        if (button_cnt > 1)
          elm_widget_element_update(obj,
                                    pd->button[EFL_UI_ALERT_POPUP_BUTTON_NEGATIVE],
                                    "right_button");
     }
}

/**
 * @brief Sets or updates a button in the alert popup.
 *
 * This function handles the creation or modification of a button of a specific type
 * (positive, negative, or user). It sets the button's text and icon.
 * If the button is newly created, it updates the layout to accommodate the
 * new button count and applies the correct styles.
 *
 * @param obj The Efl.Ui.Alert_Popup object.
 * @param pd The private data of the Efl.Ui.Alert_Popup object.
 * @param type The type of button to set (EFL_UI_ALERT_POPUP_BUTTON_POSITIVE,
 *             EFL_UI_ALERT_POPUP_BUTTON_NEGATIVE, or EFL_UI_ALERT_POPUP_BUTTON_USER).
 * @param text The text to display on the button. Can be NULL.
 * @param icon The icon object to display on the button. Can be NULL.
 */
EOLIAN static void
_efl_ui_alert_popup_button_set(Eo *obj, Efl_Ui_Alert_Popup_Data *pd, Efl_Ui_Alert_Popup_Button type, const char *text, Eo *icon)
{
   int i;
   Eina_Bool is_btn_created = EINA_FALSE;
   Eo *cur_content;
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   if ((type < EFL_UI_ALERT_POPUP_BUTTON_POSITIVE) || (type > EFL_UI_ALERT_POPUP_BUTTON_USER))
     {
        ERR("Wrong type (%d) is passed!", type);
        return;
     }
   if (!pd->button[type])
     {
        is_btn_created = EINA_TRUE;
        pd->button[type] = efl_add(EFL_UI_BUTTON_CLASS, obj,
                                   elm_widget_element_update(obj, efl_added,
                                                             PART_NAME_BUTTON));
        switch (type)
          {
            case EFL_UI_ALERT_POPUP_BUTTON_POSITIVE:
              efl_event_callback_add(pd->button[type], EFL_INPUT_EVENT_CLICKED,
                                     _positive_button_clicked_cb, obj);
              break;
            case EFL_UI_ALERT_POPUP_BUTTON_NEGATIVE:
              efl_event_callback_add(pd->button[type], EFL_INPUT_EVENT_CLICKED,
                                     _negative_button_clicked_cb, obj);
              break;
            case EFL_UI_ALERT_POPUP_BUTTON_USER:
              efl_event_callback_add(pd->button[type], EFL_INPUT_EVENT_CLICKED,
                                     _user_button_clicked_cb, obj);
              break;
            default:
              break;
          }
     }
   else
     {
        const char *pre_text = efl_text_get(pd->button[type]);
        if ((pre_text != NULL) && (text != NULL) &&
            (!strcmp(pre_text, text)) &&
            (efl_content_get(pd->button[type]) == icon))
          return;
     }

   efl_text_set(pd->button[type], text);
   efl_content_set(pd->button[type], icon);

   if (is_btn_created)
     {
        int btn_count = !!pd->button[EFL_UI_ALERT_POPUP_BUTTON_POSITIVE] +
                        !!pd->button[EFL_UI_ALERT_POPUP_BUTTON_NEGATIVE] +
                        !!pd->button[EFL_UI_ALERT_POPUP_BUTTON_USER];

        cur_content = efl_content_get(efl_part(obj, "efl.buttons"));
        for (i = 0; i < EFL_UI_ALERT_POPUP_BUTTON_COUNT; i++)
          efl_content_unset(efl_part(cur_content, BUTTON_SWALLOW_NAME[i]));

        elm_widget_element_update(obj, cur_content, PART_NAME_BUTTON_LAYOUT[btn_count - 1]);

        _apply_button_style(obj, pd, btn_count);

        i = 0;
        if (pd->button[EFL_UI_ALERT_POPUP_BUTTON_USER])
          {
             efl_content_set(efl_part(cur_content, BUTTON_SWALLOW_NAME[i]),
                             pd->button[EFL_UI_ALERT_POPUP_BUTTON_USER]);
             i++;
          }

        if (pd->button[EFL_UI_ALERT_POPUP_BUTTON_POSITIVE])
          {
             efl_content_set(efl_part(cur_content, BUTTON_SWALLOW_NAME[i]),
                             pd->button[EFL_UI_ALERT_POPUP_BUTTON_POSITIVE]);
             i++;
          }

        if (pd->button[EFL_UI_ALERT_POPUP_BUTTON_NEGATIVE])
          {
             efl_content_set(efl_part(cur_content, BUTTON_SWALLOW_NAME[i]),
                             pd->button[EFL_UI_ALERT_POPUP_BUTTON_NEGATIVE]);
          }

        elm_layout_signal_emit(obj, "efl,buttons,visible,on", "efl");
        edje_object_message_signal_process(wd->resize_obj);
     }

   efl_canvas_group_change(obj);
}

/**
 * @brief Constructor for the Efl.Ui.Alert_Popup object.
 *
 * Initializes the alert popup, sets its theme class, and creates
 * the layout for buttons.
 *
 * @param obj The Efl.Ui.Alert_Popup object being constructed.
 * @param pd The private data of the Efl.Ui.Alert_Popup object (unused in this function).
 * @return The constructed Efl.Ui.Alert_Popup object.
 */
EOLIAN static Eo *
_efl_ui_alert_popup_efl_object_constructor(Eo *obj,
                                           Efl_Ui_Alert_Popup_Data *pd EINA_UNUSED)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd, NULL);

   if (!elm_widget_theme_klass_get(obj))
     elm_widget_theme_klass_set(obj, "alert_popup");
   obj = efl_constructor(efl_super(obj, MY_CLASS));
   efl_canvas_object_type_set(obj, MY_CLASS_NAME);

   efl_add(EFL_UI_LAYOUT_CLASS, obj,
           efl_content_set(efl_part(obj, "efl.buttons"), efl_added));

   return obj;
}

/**
 * @brief Destructor for the Efl.Ui.Alert_Popup object.
 *
 * Frees resources allocated by the alert popup, such as the title text.
 *
 * @param obj The Efl.Ui.Alert_Popup object being destructed.
 * @param pd The private data of the Efl.Ui.Alert_Popup object.
 */
EOLIAN static void
_efl_ui_alert_popup_efl_object_destructor(Eo *obj, Efl_Ui_Alert_Popup_Data *pd)
{
   ELM_SAFE_FREE(pd->title_text, eina_stringshare_del);
   efl_destructor(efl_super(obj, MY_CLASS));
}

/**
 * @brief Checks if a given part name corresponds to the title part.
 *
 * This function resolves part aliasing before performing the check.
 *
 * @param obj The Efl.Ui.Alert_Popup object.
 * @param part The name of the part to check.
 * @return EINA_TRUE if the part is the title part, EINA_FALSE otherwise.
 */
static Eina_Bool
_part_is_efl_ui_alert_popup_part_title(const Eo *obj, const char *part)
{
   if (!_elm_layout_part_aliasing_eval(obj, &part, EINA_TRUE)) return EINA_FALSE;
   return eina_streq(part, "efl.text.title");
}

/* Efl.Part begin */
/**
 * @brief Implements Efl.Part.part_get for the alert popup.
 *
 * Retrieves a specific part of the alert popup. If the requested part is the title,
 * it returns an EFL_UI_ALERT_POPUP_PART_TITLE_CLASS object. Otherwise, it
 * calls the superclass's implementation.
 *
 * @param obj The Efl.Ui.Alert_Popup object.
 * @param priv The private data of the Efl.Ui.Alert_Popup object (unused).
 * @param part The name of the part to retrieve.
 * @return The Efl_Object representing the part, or NULL if not found.
 */
EOLIAN static Efl_Object *
_efl_ui_alert_popup_efl_part_part_get(const Eo *obj, Efl_Ui_Alert_Popup_Data *priv EINA_UNUSED, const char *part)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(part, NULL);
   if (_part_is_efl_ui_alert_popup_part_title(obj, part))
     return ELM_PART_IMPLEMENT(EFL_UI_ALERT_POPUP_PART_TITLE_CLASS, obj, part);
   return efl_part_get(efl_super(obj, EFL_UI_ALERT_POPUP_CLASS), part);
}

/**
 * @brief Implements Efl.Text.text_set for the title part of the alert popup.
 *
 * This function is called when efl_text_set is used on the title part object.
 * It retrieves the parent alert popup and its private data, then calls
 * _efl_ui_alert_popup_text_set to perform the actual text setting.
 *
 * @param obj The title part object (EFL_UI_ALERT_POPUP_PART_TITLE_CLASS).
 * @param _pd The private data of the title part object (unused).
 * @param text The text to set for the title.
 */
EOLIAN static void
_efl_ui_alert_popup_part_title_efl_text_text_set(Eo *obj, void *_pd EINA_UNUSED, const char *text)
{
   Elm_Part_Data *pd = efl_data_scope_get(obj, EFL_UI_WIDGET_PART_CLASS);
   Efl_Ui_Alert_Popup_Data *sd = efl_data_scope_get(pd->obj, EFL_UI_ALERT_POPUP_CLASS);

   _efl_ui_alert_popup_text_set(pd->obj, sd, pd->part, text);
}

/**
 * @brief Implements Efl.Text.text_get for the title part of the alert popup.
 *
 * This function is called when efl_text_get is used on the title part object.
 * It retrieves the parent alert popup and its private data, then calls
 * _efl_ui_alert_popup_text_get to perform the actual text retrieval.
 *
 * @param obj The title part object (EFL_UI_ALERT_POPUP_PART_TITLE_CLASS).
 * @param _pd The private data of the title part object (unused).
 * @return The text of the title part.
 */
EOLIAN static const char*
_efl_ui_alert_popup_part_title_efl_text_text_get(const Eo *obj, void *_pd EINA_UNUSED)
{
   Elm_Part_Data *pd = efl_data_scope_get(obj, EFL_UI_WIDGET_PART_CLASS);
   Efl_Ui_Alert_Popup_Data *sd = efl_data_scope_get(pd->obj, EFL_UI_ALERT_POPUP_CLASS);

   return _efl_ui_alert_popup_text_get(pd->obj, sd, pd->part);
}


#include "efl_ui_alert_popup_part_title.eo.c"

/* Efl.Part end */
EFL_UI_LAYOUT_TEXT_ALIASES_IMPLEMENT(efl_ui_alert_popup)

#define EFL_UI_ALERT_POPUP_EXTRA_OPS \
   EFL_UI_LAYOUT_TEXT_ALIASES_OPS(efl_ui_alert_popup)
#include "efl_ui_alert_popup.eo.c"
