#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define EFL_ACCESS_WIDGET_ACTION_PROTECTED
#define EFL_ACCESS_OBJECT_PROTECTED
#define ELM_LAYOUT_PROTECTED
#define EFL_PART_PROTECTED
#define EFL_INPUT_CLICKABLE_PROTECTED

#include <Elementary.h>
#include "elm_priv.h"
#include "efl_ui_button_private.h"
#include "elm_widget_layout.h"
#include "elm_part_helper.h"

#define MY_CLASS EFL_UI_BUTTON_CLASS
#define MY_CLASS_PFX efl_ui_button

#define MY_CLASS_NAME "Efl.Ui.Button"

static const char SIG_CLICKED[] = "clicked";
static const char SIG_REPEATED[] = "repeated";
static const char SIG_PRESSED[] = "pressed";
static const char SIG_UNPRESSED[] = "unpressed";

/**
 * @internal
 * @brief Smart callbacks specific to Efl.Ui.Button.
 *
 * These callbacks are emitted by the button widget for various events.
 * SIG_LAYOUT_FOCUSED and SIG_LAYOUT_UNFOCUSED are handled by elm_layout.
 */
static const Evas_Smart_Cb_Description _smart_callbacks[] = {
   {SIG_CLICKED, ""}, /**< Emitted when the button is clicked. */
   {SIG_REPEATED, ""}, /**< Emitted when the button is held down and autorepeat is enabled. */
   {SIG_PRESSED, ""}, /**< Emitted when the button is pressed. */
   {SIG_UNPRESSED, ""}, /**< Emitted when the button is released. */
   {SIG_LAYOUT_FOCUSED, ""}, /**< Handled by elm_layout. */
   {SIG_LAYOUT_UNFOCUSED, ""}, /**< Handled by elm_layout. */
   {NULL, NULL}
};

/**
 * @internal
 * @brief Content part aliases for the Efl.Ui.Button widget.
 *
 * Maps the "icon" part name to the theme part "elm.swallow.content".
 */
static const Elm_Layout_Part_Alias_Description _content_aliases[] =
{
   {"icon", "elm.swallow.content"},
   {NULL, NULL}
};

/**
 * @internal
 * @brief Callback function for the "activate" key action.
 *
 * @param obj The Evas_Object associated with the action.
 * @param params Parameters for the action (unused).
 * @return EINA_TRUE if the action was handled, EINA_FALSE otherwise.
 */
static Eina_Bool _key_action_activate(Evas_Object *obj, const char *params);

/**
 * @internal
 * @brief Defines the key actions available for the button widget.
 *
 * Currently, only the "activate" action is defined.
 */
static const Elm_Action key_actions[] = {
   {"activate", _key_action_activate}, /**< Action to activate the button, typically via keyboard. */
   {NULL, NULL}
};

#define MY_CLASS_NAME_LEGACY "elm_button"

/**
 * @internal
 * @brief Class constructor for Efl.Ui.Button.
 *
 * Registers the legacy type name "elm_button" for this class.
 * @param klass The Efl_Class being constructed.
 */
static void
_efl_ui_button_class_constructor(Efl_Class *klass)
{
   evas_smart_legacy_type_register(MY_CLASS_NAME_LEGACY, klass);
}

/**
 * @internal
 * @brief Handles the activation of the button.
 *
 * This function is called when the button is clicked or activated
 * via keyboard. It stops any ongoing autorepeat timer, handles
 * accessibility announcements, and emits the "clicked" signal or
 * corresponding Efl.Input.Clickable events.
 *
 * @param obj The button Evas_Object.
 */
static void
_activate(Evas_Object *obj)
{
   ELM_BUTTON_DATA_GET_OR_RETURN(obj, sd);

   ELM_SAFE_FREE(sd->timer, ecore_timer_del);
   sd->repeating = EINA_FALSE;

   if ((_elm_config->access_mode == ELM_ACCESS_MODE_OFF) ||
       (_elm_access_2nd_click_timeout(obj)))
     {
        if (_elm_config->access_mode != ELM_ACCESS_MODE_OFF)
          _elm_access_say(E_("Clicked"));
        if (!elm_widget_disabled_get(obj) &&
            !evas_object_freeze_events_get(obj))
          {
             if (elm_widget_is_legacy(obj))
               evas_object_smart_callback_call(obj, "clicked", NULL);
             else
               {
                  efl_input_clickable_press(obj, 1);
                  efl_input_clickable_unpress(obj, 1);
               }
          }
     }
}

/**
 * @internal
 * @brief Handles the EFL_UI_WIDGET_EVENT_ACCESS_ACTIVATE event.
 *
 * This function is called when the button is activated through accessibility
 * mechanisms. It triggers the button's click action and emits activation
 * signals.
 *
 * @param obj The Eo object representing the button.
 * @param _pd Private data for the button (unused).
 * @param act The type of activation. Only EFL_UI_ACTIVATE_DEFAULT is handled.
 * @return EINA_TRUE if the activation was handled, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_button_efl_ui_widget_on_access_activate(Eo *obj, Efl_Ui_Button_Data *_pd EINA_UNUSED, Efl_Ui_Activate act)
{
   if (elm_widget_disabled_get(obj)) return EINA_FALSE;
   if (act != EFL_UI_ACTIVATE_DEFAULT) return EINA_FALSE;
   if (evas_object_freeze_events_get(obj)) return EINA_FALSE;

   if (elm_widget_is_legacy(obj))
     evas_object_smart_callback_call(obj, "clicked", NULL);
   else
     {
        efl_input_clickable_press(obj, 1);
        efl_input_clickable_unpress(obj, 1);
     }

   if (elm_widget_is_legacy(obj))
     elm_layout_signal_emit(obj, "elm,anim,activate", "elm");
   else
     elm_layout_signal_emit(obj, "efl,state,animation,activated", "efl");

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Implements the "activate" key action.
 *
 * Emits an activation animation signal and calls the internal _activate function.
 *
 * @param obj The button Evas_Object.
 * @param params Action parameters (unused).
 * @return EINA_TRUE indicating the action was handled.
 */
static Eina_Bool
_key_action_activate(Evas_Object *obj, const char *params EINA_UNUSED)
{
   if (elm_widget_is_legacy(obj))
     elm_layout_signal_emit(obj, "elm,anim,activate", "elm");
   else
     elm_layout_signal_emit(obj, "efl,state,animation,activated", "efl");
   _activate(obj);
   return EINA_TRUE;
}

/**
 * @internal
 * @brief Callback for the "elm,action,click" (legacy) or equivalent Edje signal.
 *
 * This function is triggered by the theme when the button is clicked.
 * It calls the internal _activate function.
 *
 * @param data The button Evas_Object (passed as user data).
 * @param obj The Edje Evas_Object that emitted the signal (unused).
 * @param emission The emitted signal string (unused).
 * @param source The source of the signal (unused).
 */
static void
_on_clicked_signal(void *data,
                   Evas_Object *obj EINA_UNUSED,
                   const char *emission EINA_UNUSED,
                   const char *source EINA_UNUSED)
{
   _activate(data);
}

/**
 * @internal
 * @brief Sends a "repeated" event during autorepeat.
 *
 * This function is called by an Ecore_Timer when autorepeat is active.
 * It emits the "repeated" smart callback or the EFL_UI_AUTOREPEAT_EVENT_REPEATED event.
 *
 * @param data The button Evas_Object (passed as user data).
 * @return ECORE_CALLBACK_RENEW to continue the timer if still repeating,
 *         ECORE_CALLBACK_CANCEL otherwise.
 */
static Eina_Bool
_autorepeat_send(void *data)
{
   ELM_BUTTON_DATA_GET_OR_RETURN_VAL(data, sd, ECORE_CALLBACK_CANCEL);

   if (elm_widget_is_legacy(data))
     evas_object_smart_callback_call(data, "repeated", NULL);
   else
     efl_event_callback_call(data, EFL_UI_AUTOREPEAT_EVENT_REPEATED, NULL);

   if (!sd->repeating)
     {
        sd->timer = NULL;
        return ECORE_CALLBACK_CANCEL;
     }

   return ECORE_CALLBACK_RENEW;
}

/**
 * @internal
 * @brief Sends the initial "repeated" event and starts the autorepeat timer.
 *
 * This function is called after the initial autorepeat timeout. It sets
 * the repeating flag, sends the first "repeated" event, and then starts
 * a new timer for subsequent repeats based on the gap timeout.
 *
 * @param data The button Evas_Object (passed as user data).
 * @return ECORE_CALLBACK_CANCEL as this timer callback is only for the initial event.
 */
static Eina_Bool
_autorepeat_initial_send(void *data)
{
   ELM_BUTTON_DATA_GET_OR_RETURN_VAL(data, sd, ECORE_CALLBACK_CANCEL);

   ELM_SAFE_FREE(sd->timer, ecore_timer_del);
   sd->repeating = EINA_TRUE;
   _autorepeat_send(data);
   sd->timer = ecore_timer_add(sd->ar_gap_timeout, _autorepeat_send, data);

   return ECORE_CALLBACK_CANCEL;
}

/**
 * @internal
 * @brief Callback for the "elm,action,press" (legacy) or "efl,action,press" Edje signal.
 *
 * This function is triggered by the theme when the button is pressed.
 * If autorepeat is enabled, it starts the autorepeat timer.
 * It also emits the "pressed" smart callback for legacy buttons.
 *
 * @param data The button Evas_Object (passed as user data).
 * @param obj The Edje Evas_Object that emitted the signal (unused).
 * @param emission The emitted signal string (unused).
 * @param source The source of the signal (unused).
 */
static void
_on_pressed_signal(void *data,
                   Evas_Object *obj EINA_UNUSED,
                   const char *emission EINA_UNUSED,
                   const char *source EINA_UNUSED)
{
   ELM_BUTTON_DATA_GET_OR_RETURN(data, sd);

   if ((sd->autorepeat) && (!sd->repeating))
     {
        if (sd->ar_initial_timeout <= 0.0)
          _autorepeat_initial_send(data);  /* call immediately */
        else
          sd->timer = ecore_timer_add
              (sd->ar_initial_timeout, _autorepeat_initial_send, data);
     }

   if (elm_widget_is_legacy(data))
     evas_object_smart_callback_call
        (data, "pressed", NULL);

}

/**
 * @internal
 * @brief Callback for the "elm,action,unpress" (legacy) or "efl,action,unpress" Edje signal.
 *
 * This function is triggered by the theme when the button is released.
 * It stops any ongoing autorepeat timer and sets the repeating flag to false.
 * It also emits the "unpressed" smart callback for legacy buttons.
 *
 * @param data The button Evas_Object (passed as user data).
 * @param obj The Edje Evas_Object that emitted the signal (unused).
 * @param emission The emitted signal string (unused).
 * @param source The source of the signal (unused).
 */
static void
_on_unpressed_signal(void *data,
                     Evas_Object *obj EINA_UNUSED,
                     const char *emission EINA_UNUSED,
                     const char *source EINA_UNUSED)
{
   ELM_BUTTON_DATA_GET_OR_RETURN(data, sd);

   ELM_SAFE_FREE(sd->timer, ecore_timer_del);
   sd->repeating = EINA_FALSE;

   if (elm_widget_is_legacy(data))
     evas_object_smart_callback_call
        (data, "unpressed", NULL);
}

/**
 * @internal
 * @brief Provides accessibility information for the button.
 *
 * Returns the custom accessibility info if set, otherwise falls back
 * to the button's label text.
 *
 * @param data Custom data associated with the callback (unused).
 * @param obj The button Evas_Object.
 * @return A newly allocated string containing the accessibility information,
 *         or NULL if no information is available. The caller must free this string.
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
 * @internal
 * @brief Provides the accessibility state for the button.
 *
 * Returns "State: Disabled" if the button is disabled.
 *
 * @param data Custom data associated with the callback (unused).
 * @param obj The button Evas_Object.
 * @return A newly allocated string containing the state information if disabled,
 *         otherwise NULL. The caller must free this string.
 */
static char *
_access_state_cb(void *data EINA_UNUSED, Evas_Object *obj)
{
   if (elm_widget_disabled_get(obj))
     return strdup(E_("State: Disabled"));

   return NULL;
}

/**
 * @internal
 * @brief Efl.Canvas.Group group_add implementation for Efl.Ui.Button.
 *
 * Initializes the button's theme, sets up signal callbacks for click, press,
 * and unpress actions from the theme, registers accessibility information,
 * and makes the button focusable.
 *
 * @param obj The Eo object representing the button.
 * @param _pd Private data for the button (unused).
 */
EOLIAN static void
_efl_ui_button_efl_canvas_group_group_add(Eo *obj, Efl_Ui_Button_Data *_pd EINA_UNUSED)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   if (!elm_widget_theme_klass_get(obj))
     elm_widget_theme_klass_set(obj, "button");
   efl_canvas_group_add(efl_super(obj, MY_CLASS));

   if (elm_widget_is_legacy(obj))
     {
        edje_object_signal_callback_add
           (wd->resize_obj, "elm,action,click", "*",
            _on_clicked_signal, obj);
        edje_object_signal_callback_add
           (wd->resize_obj, "elm,action,press", "*",
            _on_pressed_signal, obj);
        edje_object_signal_callback_add
           (wd->resize_obj, "elm,action,unpress", "*",
            _on_unpressed_signal, obj);
     }
   else
     {
        edje_object_signal_callback_add
           (wd->resize_obj, "efl,action,press", "*",
            _on_pressed_signal, obj);
        edje_object_signal_callback_add
           (wd->resize_obj, "efl,action,unpress", "*",
            _on_unpressed_signal, obj);
        efl_ui_action_connector_bind_clickable_to_theme(wd->resize_obj, obj);
     }

   _elm_access_object_register(obj, wd->resize_obj);
   _elm_access_text_set
     (_elm_access_info_get(obj), ELM_ACCESS_TYPE, E_("Button"));
   _elm_access_callback_set
     (_elm_access_info_get(obj), ELM_ACCESS_INFO, _access_info_cb, NULL);
   _elm_access_callback_set
     (_elm_access_info_get(obj), ELM_ACCESS_STATE, _access_state_cb, obj);

   elm_widget_can_focus_set(obj, EINA_TRUE);

   if (elm_widget_theme_object_set(obj, wd->resize_obj,
                                       elm_widget_theme_klass_get(obj),
                                       elm_widget_theme_element_get(obj),
                                       elm_widget_theme_style_get(obj)) == EFL_UI_THEME_APPLY_ERROR_GENERIC)
     CRI("Failed to set layout!");
}

/**
 * @internal
 * @brief Efl.Object constructor for Efl.Ui.Button.
 *
 * Calls the superclass constructor, sets up smart callbacks, and sets the
 * accessibility role to EFL_ACCESS_ROLE_PUSH_BUTTON.
 *
 * @param obj The Eo object being constructed.
 * @param _pd Private data for the button (unused).
 * @return The constructed Eo object.
 */
EOLIAN static Eo *
_efl_ui_button_efl_object_constructor(Eo *obj, Efl_Ui_Button_Data *_pd EINA_UNUSED)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));
   evas_object_smart_callbacks_descriptions_set(obj, _smart_callbacks);
   efl_access_object_role_set(obj, EFL_ACCESS_ROLE_PUSH_BUTTON);

   return obj;
}

/**
 * @internal
 * @brief Sets whether autorepeat is enabled for the button.
 * @param obj The Eo object (unused).
 * @param sd Private data for the button.
 * @param on EINA_TRUE to enable autorepeat, EINA_FALSE to disable.
 */
EOLIAN static void
_efl_ui_button_efl_ui_autorepeat_autorepeat_enabled_set(Eo *obj EINA_UNUSED, Efl_Ui_Button_Data *sd, Eina_Bool on)
{
   ELM_SAFE_FREE(sd->timer, ecore_timer_del);
   sd->autorepeat = on;
   sd->repeating = EINA_FALSE;
}

/**
 * @internal
 * @brief Gets whether autorepeat is enabled for the button.
 * @param obj The Eo object (unused).
 * @param sd Private data for the button.
 * @return EINA_TRUE if autorepeat is enabled, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_button_efl_ui_autorepeat_autorepeat_enabled_get(const Eo *obj EINA_UNUSED, Efl_Ui_Button_Data *sd)
{
   return (sd->autorepeat);
}

/**
 * @internal
 * @brief Sets the initial timeout for autorepeat.
 *
 * This is the time before the first "repeated" event is generated after
 * the button is pressed and held.
 * @param obj The Eo object (unused).
 * @param sd Private data for the button.
 * @param t The initial timeout value in seconds.
 */
EOLIAN static void
_efl_ui_button_efl_ui_autorepeat_autorepeat_initial_timeout_set(Eo *obj EINA_UNUSED, Efl_Ui_Button_Data *sd, double t)
{
   if (EINA_DBL_EQ(sd->ar_initial_timeout, t)) return;
   ELM_SAFE_FREE(sd->timer, ecore_timer_del);
   sd->ar_initial_timeout = t;
}

/**
 * @internal
 * @brief Gets the initial timeout for autorepeat.
 * @param obj The Eo object (unused).
 * @param sd Private data for the button.
 * @return The initial timeout value in seconds.
 */
EOLIAN static double
_efl_ui_button_efl_ui_autorepeat_autorepeat_initial_timeout_get(const Eo *obj EINA_UNUSED, Efl_Ui_Button_Data *sd)
{
   return sd->ar_initial_timeout;
}

/**
 * @internal
 * @brief Sets the gap timeout for autorepeat.
 *
 * This is the time interval between subsequent "repeated" events after the
 * initial timeout.
 * @param obj The Eo object (unused).
 * @param sd Private data for the button.
 * @param t The gap timeout value in seconds.
 */
EOLIAN static void
_efl_ui_button_efl_ui_autorepeat_autorepeat_gap_timeout_set(Eo *obj EINA_UNUSED, Efl_Ui_Button_Data *sd, double t)
{
   if (EINA_DBL_EQ(sd->ar_gap_timeout, t)) return;

   sd->ar_gap_timeout = t;
   if ((sd->repeating) && (sd->timer)) ecore_timer_interval_set(sd->timer, t);
}

/**
 * @internal
 * @brief Gets the gap timeout for autorepeat.
 * @param obj The Eo object (unused).
 * @param sd Private data for the button.
 * @return The gap timeout value in seconds.
 */
EOLIAN static double
_efl_ui_button_efl_ui_autorepeat_autorepeat_gap_timeout_get(const Eo *obj EINA_UNUSED, Efl_Ui_Button_Data *sd)
{
   return sd->ar_gap_timeout;
}

/**
 * @internal
 * @brief Gets the accessibility actions for the button.
 *
 * Provides the "activate" action for accessibility services.
 * @param obj The Eo object (unused).
 * @param pd Private data for the button (unused).
 * @return A pointer to an array of Efl_Access_Action_Data, terminated by an
 *         entry with NULL fields.
 *         Example:
 *         `static Efl_Access_Action_Data atspi_actions[] = {`
 *         `  { "activate", "activate", NULL, _key_action_activate },`
 *         `  { NULL, NULL, NULL, NULL}`
 *         `};`
 *         `return &atspi_actions[0];`
 */
EOLIAN const Efl_Access_Action_Data *
_efl_ui_button_efl_access_widget_action_elm_actions_get(const Eo *obj EINA_UNUSED, Efl_Ui_Button_Data *pd EINA_UNUSED)
{
   static Efl_Access_Action_Data atspi_actions[] = {
          { "activate", "activate", NULL, _key_action_activate },
          { NULL, NULL, NULL, NULL}
   };
   return &atspi_actions[0];
}

/* Standard widget overrides */

ELM_WIDGET_KEY_DOWN_DEFAULT_IMPLEMENT(efl_ui_button, Efl_Ui_Button_Data)
ELM_PART_TEXT_DEFAULT_IMPLEMENT(efl_ui_button, Efl_Ui_Button_Data)
ELM_PART_CONTENT_DEFAULT_IMPLEMENT(efl_ui_button, Efl_Ui_Button_Data)

EAPI void
elm_button_autorepeat_initial_timeout_set(Evas_Object *obj, double t)
{
   efl_ui_autorepeat_initial_timeout_set(obj, t);
}

EAPI double
elm_button_autorepeat_initial_timeout_get(const Evas_Object *obj)
{
   return efl_ui_autorepeat_initial_timeout_get(obj);
}

EAPI void
elm_button_autorepeat_gap_timeout_set(Evas_Object *obj, double t)
{
   efl_ui_autorepeat_gap_timeout_set(obj, t);
}

EAPI double
elm_button_autorepeat_gap_timeout_get(const Evas_Object *obj)
{
   return efl_ui_autorepeat_gap_timeout_get(obj);
}

EAPI void
elm_button_autorepeat_set(Evas_Object *obj, Eina_Bool on)
{
   efl_ui_autorepeat_enabled_set(obj, on);
}

EAPI Eina_Bool
elm_button_autorepeat_get(const Evas_Object *obj)
{
   return efl_ui_autorepeat_enabled_get(obj);
}

/* Internal EO APIs and hidden overrides */

EFL_UI_LAYOUT_CONTENT_ALIASES_IMPLEMENT(MY_CLASS_PFX)

#define EFL_UI_BUTTON_EXTRA_OPS \
   EFL_UI_LAYOUT_CONTENT_ALIASES_OPS(MY_CLASS_PFX), \
   EFL_CANVAS_GROUP_ADD_OPS(efl_ui_button)

#include "efl_ui_button.eo.c"

#include "efl_ui_button_legacy_eo.h"
#include "efl_ui_button_legacy_part.eo.h"

/**
 * @internal
 * @brief Efl.Object constructor for the legacy Efl.Ui.Button.
 *
 * Calls the superclass constructor, handles legacy focus, and sets the
 * legacy object type name.
 *
 * @param obj The Eo object being constructed.
 * @param _pd Private data (unused).
 * @return The constructed Eo object.
 */
EOLIAN static Eo *
_efl_ui_button_legacy_efl_object_constructor(Eo *obj, void *_pd EINA_UNUSED)
{
   obj = efl_constructor(efl_super(obj, EFL_UI_BUTTON_LEGACY_CLASS));
   legacy_object_focus_handle(obj);
   efl_canvas_object_type_set(obj, MY_CLASS_NAME_LEGACY);
   return obj;
}

/**
 * @internal
 * @brief Applies the theme to the legacy button.
 *
 * This function is replicated from elm_layout due to differences in
 * icon part naming ("elm.swallow.content" vs "elm.swallow.icon").
 * It calls the superclass theme_apply and then emits the legacy icon signal.
 *
 * @param obj The Eo object representing the legacy button.
 * @param _pd Private data (unused).
 * @return EINA_ERROR_NONE on success, or an error code if theme application fails.
 */
/* FIXME: replicated from elm_layout just because button's icon spot
 * is elm.swallow.content, not elm.swallow.icon. Fix that whenever we
 * can changed the theme API */
EOLIAN static Eina_Error
_efl_ui_button_legacy_efl_ui_widget_theme_apply(Eo *obj, void *_pd EINA_UNUSED)
{
   Eina_Error int_ret = EFL_UI_THEME_APPLY_ERROR_GENERIC;

   int_ret = efl_ui_widget_theme_apply(efl_super(obj, EFL_UI_BUTTON_LEGACY_CLASS));
   if (int_ret == EFL_UI_THEME_APPLY_ERROR_GENERIC) return int_ret;
   if (efl_finalized_get(obj)) _elm_layout_legacy_icon_signal_emit(obj);

   return int_ret;
}

/**
 * @internal
 * @brief Handles deletion of a sub-object from the legacy button.
 *
 * This function is replicated from elm_layout due to differences in
 * icon part naming. It calls the superclass sub_object_del and then
 * emits the legacy icon signal.
 *
 * @param obj The Eo object representing the legacy button.
 * @param _pd Private data (unused).
 * @param sobj The sub-object to delete.
 * @return EINA_TRUE if the sub-object was successfully deleted, EINA_FALSE otherwise.
 */
/* FIXME: replicated from elm_layout just because button's icon spot
 * is elm.swallow.content, not elm.swallow.icon. Fix that whenever we
 * can changed the theme API */
EOLIAN static Eina_Bool
_efl_ui_button_legacy_efl_ui_widget_widget_sub_object_del(Eo *obj, void *_pd EINA_UNUSED, Evas_Object *sobj)
{
   Eina_Bool int_ret = EINA_FALSE;

   int_ret = elm_widget_sub_object_del(efl_super(obj, EFL_UI_BUTTON_LEGACY_CLASS), sobj);
   if (!int_ret) return EINA_FALSE;

   _elm_layout_legacy_icon_signal_emit(obj);

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Sets content on a part of the legacy button.
 *
 * This function is replicated from elm_layout due to differences in
 * icon part naming. It calls the superclass efl_content_set and then
 * emits the legacy icon signal.
 *
 * @param obj The Eo object representing the legacy button.
 * @param _pd Private data (unused).
 * @param part The name of the part to set content on.
 * @param content The Evas_Object to set as content.
 * @return EINA_TRUE if content was successfully set, EINA_FALSE otherwise.
 */
/* FIXME: replicated from elm_layout just because button's icon spot
 * is elm.swallow.content, not elm.swallow.icon. Fix that whenever we
 * can changed the theme API */
static Eina_Bool
_efl_ui_button_legacy_content_set(Eo *obj, void *_pd EINA_UNUSED, const char *part, Evas_Object *content)
{
   Eina_Bool int_ret = EINA_FALSE;

   int_ret = efl_content_set(efl_part(efl_super(obj, EFL_UI_BUTTON_LEGACY_CLASS), part), content);
   if (!int_ret) return EINA_FALSE;

   _elm_layout_legacy_icon_signal_emit(obj);

   return EINA_TRUE;
}

/* Efl.Part begin */

/**
 * @internal
 * @brief Checks if a given part name is a valid part for the legacy button.
 *
 * For legacy buttons, the only recognized content part is "elm.swallow.content".
 *
 * @param obj The Eo object (unused).
 * @param part The name of the part to check.
 * @return EINA_TRUE if the part name is "elm.swallow.content", EINA_FALSE otherwise.
 */
static Eina_Bool
_part_is_efl_ui_button_legacy_part(const Eo *obj EINA_UNUSED, const char *part)
{
   return eina_streq(part, "elm.swallow.content");
}

ELM_PART_OVERRIDE_PARTIAL(efl_ui_button_legacy, EFL_UI_BUTTON_LEGACY, void, _part_is_efl_ui_button_legacy_part)
ELM_PART_OVERRIDE_CONTENT_SET_NO_SD(efl_ui_button_legacy)
#include "efl_ui_button_legacy_part.eo.c"

/* Efl.Part end */

EAPI Evas_Object *
elm_button_add(Evas_Object *parent)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);
   return elm_legacy_add(EFL_UI_BUTTON_LEGACY_CLASS, parent);
}

#include "efl_ui_button_legacy_eo.c"
