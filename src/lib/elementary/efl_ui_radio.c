#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define EFL_ACCESS_OBJECT_PROTECTED
#define EFL_ACCESS_WIDGET_ACTION_PROTECTED
#define ELM_LAYOUT_PROTECTED
#define EFL_PART_PROTECTED

#include <Elementary.h>

#include "elm_priv.h"
#include "efl_ui_radio_private.h"
#include "elm_widget_layout.h"
#include "elm_part_helper.h"

#define MY_CLASS EFL_UI_RADIO_CLASS
#define MY_CLASS_PFX efl_ui_radio

#define MY_CLASS_NAME "Efl.Ui.Radio"

static const Elm_Layout_Part_Alias_Description _text_aliases[] =
{
   {"default", "elm.text"},
   {NULL, NULL}
};

/** @internal
 * @brief Signal name for the "changed" event.
 */
static const char SIG_CHANGED[] = "changed";

/** @internal
 * @brief Descriptions for smart callbacks supported by Efl.Ui.Radio.
 *
 * Each element is an Evas_Smart_Cb_Description:
 * - First field: const char *name (signal name)
 * - Second field: const char *type (type signature of callback parameters, "" means void *event_info)
 *
 * Example: {SIG_CHANGED, ""} means a signal "changed" with no specific event_info structure.
 */
static const Evas_Smart_Cb_Description _smart_callbacks[] = {
   {SIG_CHANGED, ""}, /**< handled by efl_ui_check */
   {SIG_WIDGET_LANG_CHANGED, ""}, /**< handled by elm_widget */
   {SIG_WIDGET_ACCESS_CHANGED, ""}, /**< handled by elm_widget */
   {SIG_LAYOUT_FOCUSED, ""}, /**< handled by elm_layout */
   {SIG_LAYOUT_UNFOCUSED, ""}, /**< handled by elm_layout */
   {NULL, NULL}
};

static Eina_Bool _key_action_activate(Evas_Object *obj, const char *params);

/** @internal
 * @brief Defines keyboard actions for the radio widget.
 *
 * Each element is an Elm_Action:
 * - First field: const char *name (action name, e.g., "activate")
 * - Second field: Eina_Bool (*func)(Evas_Object *obj, const char *params) (callback function)
 *
 * Example: {"activate", _key_action_activate} maps the "activate" action to the
 * _key_action_activate function.
 */
static const Elm_Action key_actions[] = {
   {"activate", _key_action_activate},
   {NULL, NULL}
};

/**
 * @internal
 * @brief Emits a standardized Edje signal for the radio widget.
 *
 * The signal format is "<source>,<middle_term>,<state>", where:
 * - <source> is "elm" for legacy widgets or "efl" for newer widgets.
 * - <middle_term> is a string provided by the caller (e.g., "state,radio", "activate,radio").
 * - <state> is "on" if the radio is selected, "off" otherwise.
 *
 * @param obj The radio widget object.
 * @param middle_term The middle part of the signal string.
 */
static void
_radio_widget_signal_emit(Evas_Object *obj, const char *middle_term)
{
   const char *source, *state;
   char path[PATH_MAX];

   if (elm_widget_is_legacy(obj))
     source = "elm";
   else
     source = "efl";

   if (efl_ui_selectable_selected_get(obj))
     state = "on";
   else
     state = "off";

   snprintf(path, sizeof(path), "%s,%s,%s", source, middle_term, state);
   elm_layout_signal_emit(obj, path, source);
}

/**
 * @internal
 * @brief Sets the selected state of the radio button and emits appropriate signals.
 * This function overrides the efl_ui_selectable_selected_set method.
 * It also emits an accessibility state change signal if AT-SPI mode is enabled
 * and the radio button becomes checked.
 *
 * @param obj The radio widget object.
 * @param pd Private data of the radio widget (unused in this function).
 * @param value The new selected state (EINA_TRUE for selected, EINA_FALSE for unselected).
 */
static void
_efl_ui_radio_efl_ui_selectable_selected_set(Eo *obj, Efl_Ui_Radio_Data *pd EINA_UNUSED, Eina_Bool value)
{
   if (value == efl_ui_selectable_selected_get(obj)) return;
   efl_ui_selectable_selected_set(efl_super(obj, MY_CLASS), value);

   _radio_widget_signal_emit(obj, "state,radio");

   if (_elm_config->atspi_mode)
     {
        if (efl_ui_selectable_selected_get(obj))
          {
             efl_access_state_changed_signal_emit(obj, EFL_ACCESS_STATE_TYPE_CHECKED, EINA_TRUE);
          }
     }
}

/**
 * @internal
 * @brief Emits an activation signal for the radio widget.
 * This is a helper function that calls _radio_widget_signal_emit with "activate,radio".
 *
 * @param obj The radio widget object.
 */
static void
_activate_state_emit(Evas_Object *obj)
{
   _radio_widget_signal_emit(obj, "activate,radio");
}

/**
 * @internal
 * @brief Updates the selection state of all radio buttons within the same group.
 *
 * Iterates through all radio buttons in the group associated with `sd`.
 * It sets their selection state based on the group's current value (`sd->group->value`).
 * If `activate` is EINA_TRUE, it also emits activation signals for each radio button
 * during the state change.
 *
 * A special case handles scenarios where the intended selected radio button might be
 * disabled. If this occurs, and there was a previously selected radio button,
 * this function attempts to re-select that previous one.
 *
 * @param sd The private data of a radio widget in the group. This provides access
 *           to the group's information (list of radios, current group value).
 * @param activate If EINA_TRUE, emits activation signals for affected radio buttons.
 */
static void
_state_set_all(Efl_Ui_Radio_Data *sd, Eina_Bool activate)
{
   const Eina_List *l;
   Eina_Bool disabled = EINA_FALSE;
   Evas_Object *child, *selected = NULL;

   EINA_LIST_FOREACH(sd->group->radios, l, child)
     {
        ELM_RADIO_DATA_GET(child, sdc);

        if (efl_ui_selectable_selected_get(child)) selected = child;
        if (sdc->value == sd->group->value)
          {
             if (activate) _activate_state_emit(child);
             efl_ui_selectable_selected_set(child, EINA_TRUE);
             if (!efl_ui_selectable_selected_get(child)) disabled = EINA_TRUE;
          }
        else
          {
             if (activate) _activate_state_emit(child);
             efl_ui_selectable_selected_set(child, EINA_FALSE);
          }
     }

   if ((disabled) && (selected))
     {
        if (activate) _activate_state_emit(selected);
        efl_ui_selectable_selected_set(selected, EINA_TRUE);
     }
}

/**
 * @internal
 * @brief Handles the activation logic for a radio button.
 *
 * This function is called when a radio button is activated (e.g., by a click or key press).
 *
 * For legacy widgets:
 * - If the radio's value already matches the group's value, it does nothing.
 * - Otherwise, it updates the group's value (`sd->group->value`) to this radio's value.
 * - If a value pointer (`sd->group->valuep`) is set, it's also updated.
 * - It then calls `_state_set_all` to update the visual state of all radios in the group.
 * - If access mode is enabled, it announces "State: On".
 * - Finally, it triggers the "changed" smart callback.
 *
 * For non-legacy (EFL UI) widgets:
 * - It simply toggles the selected state of the current radio button. The group logic
 *   is expected to be handled by a manager or container widget, or through bindings.
 *
 * @param obj The radio widget object that was activated.
 */
static void
_activate(Evas_Object *obj)
{
   ELM_RADIO_DATA_GET(obj, sd);

   if (elm_widget_is_legacy(obj))
     {
        //in legacy, group is handeled by the widget
        if (sd->group->value == sd->value) return;

        if ((!_elm_config->access_mode) ||
            (_elm_access_2nd_click_timeout(obj)))
          {
             sd->group->value = sd->value;
             if (sd->group->valuep) *(sd->group->valuep) = sd->group->value;

             _state_set_all(sd, EINA_TRUE);

             if (_elm_config->access_mode)
               _elm_access_say(E_("State: On"));
         }
        evas_object_smart_callback_call(obj, "changed", NULL);
     }
   else
     {
        //in new API, we just toggle the state of the widget, rest will be automatically handled
        efl_ui_selectable_selected_set(obj, !efl_ui_selectable_selected_get(obj));
     }
}

/**
 * @internal
 * @brief Callback function for the "activate" key action.
 *
 * This function is invoked when the "activate" action (e.g., pressing Space or Enter
 * when the radio button has focus) is triggered. It calls the `_activate` function
 * to perform the radio button activation.
 *
 * @param obj The radio widget object.
 * @param params Action parameters (currently unused).
 * @return EINA_TRUE to indicate the action was handled.
 */
static Eina_Bool
_key_action_activate(Evas_Object *obj, const char *params EINA_UNUSED)
{
   _activate(obj);
   return EINA_TRUE;
}

/**
 * @internal
 * @brief Applies the theme to the radio widget and updates its visual state.
 *
 * This function overrides the `efl_ui_widget_theme_apply` EOLIAN method.
 * After calling the superclass's theme apply, it emits specific signals
 * based on whether the widget is legacy or not, and its current selected state.
 * For legacy widgets, it emits "elm,state,radio,on" or "elm,state,radio,off".
 * For non-legacy widgets, it emits "efl,state,selected" or "efl,state,unselected".
 * It also ensures that any pending Edje messages are processed.
 *
 * @param obj The radio widget object.
 * @param sd Private data of the radio widget (unused in this function).
 * @return EFL_UI_THEME_APPLY_ERROR_GENERIC on failure from super, or the result of super's apply.
 */
EOLIAN static Eina_Error
_efl_ui_radio_efl_ui_widget_theme_apply(Eo *obj, Efl_Ui_Radio_Data *sd EINA_UNUSED)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd, EFL_UI_THEME_APPLY_ERROR_GENERIC);
   Eina_Error int_ret = EFL_UI_THEME_APPLY_ERROR_GENERIC;
   int_ret = efl_ui_widget_theme_apply(efl_super(obj, EFL_UI_CHECK_CLASS));
   if (int_ret == EFL_UI_THEME_APPLY_ERROR_GENERIC) return int_ret;

   if (elm_widget_is_legacy(obj))
     {
        if (efl_ui_selectable_selected_get(obj)) elm_layout_signal_emit(obj, "elm,state,radio,on", "elm");
        else elm_layout_signal_emit(obj, "elm,state,radio,off", "elm");
     }
   else
     {
        if (efl_ui_selectable_selected_get(obj)) elm_layout_signal_emit(obj, "efl,state,selected", "efl");
        else elm_layout_signal_emit(obj, "efl,state,unselected", "efl");
     }

   edje_object_message_signal_process(wd->resize_obj);

   return int_ret;
}

/**
 * @internal
 * @brief Callback function for the "elm,action,radio,toggle" signal.
 *
 * This callback is used in legacy mode. When the "elm,action,radio,toggle" signal
 * is emitted (typically by the theme when the radio is clicked), this function
 * calls `_activate` to handle the radio button's activation.
 *
 * @param data User data, expected to be the radio widget object (`Evas_Object *`).
 * @param obj The Evas object that emitted the signal (unused).
 * @param emission The emitted signal string (e.g., "elm,action,radio,toggle") (unused).
 * @param source The source of the signal (e.g., "elm") (unused).
 */
static void
_radio_on_cb(void *data,
             Evas_Object *obj EINA_UNUSED,
             const char *emission EINA_UNUSED,
             const char *source EINA_UNUSED)
{
   _activate(data);
}

/**
 * @internal
 * @brief Provides accessibility information for the radio widget.
 *
 * This function is used as a callback for `ELM_ACCESS_INFO`.
 * It retrieves the accessibility information string, prioritizing
 * `elm_widget_access_info_get(obj)`. If that is NULL, it falls back to
 * `elm_layout_text_get(obj, NULL)` (the visible text of the radio).
 *
 * @param data User data, passed from _elm_access_callback_set (unused).
 * @param obj The radio widget object.
 * @return A newly allocated string containing the accessibility information,
 *         or NULL if no information is available. The caller is responsible
 *         for freeing the returned string.
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
 * @brief Provides the accessibility state description for the radio widget.
 *
 * This function is used as a callback for `ELM_ACCESS_STATE`.
 * It returns a string describing the current state of the radio button,
 * such as "State: Disabled", "State: On", or "State: Off".
 *
 * @param data User data, passed from _elm_access_callback_set (unused).
 * @param obj The radio widget object.
 * @return A newly allocated string containing the accessibility state description.
 *         The caller is responsible for freeing the returned string.
 */
static char *
_access_state_cb(void *data EINA_UNUSED, Evas_Object *obj)
{
   if (elm_widget_disabled_get(obj)) return strdup(E_("State: Disabled"));
   if (efl_ui_selectable_selected_get(obj)) return strdup(E_("State: On"));

   return strdup(E_("State: Off"));
}

/**
 * @internal
 * @brief Constructor for the Efl.Ui.Radio object.
 *
 * Initializes the radio widget. This includes:
 * - Setting the default theme class to "radio" if not already set.
 * - Calling the superclass constructor.
 * - Setting up smart callbacks.
 * - For legacy widgets:
 *   - Adding a signal callback for "elm,action,radio,toggle" to handle activation.
 *   - Initializing the radio group structure (`pd->group`) and adding the new
 *     radio object to this group.
 * - Setting the accessibility role to `EFL_ACCESS_ROLE_RADIO_BUTTON`.
 * - Setting up accessibility information and state callbacks.
 *
 * @param obj The Efl.Ui.Radio object being constructed.
 * @param pd The private data structure for the radio widget.
 * @return The constructed Eo object, or NULL on failure.
 */
EOLIAN static Eo *
_efl_ui_radio_efl_object_constructor(Eo *obj, Efl_Ui_Radio_Data *pd)
{
   if (!elm_widget_theme_klass_get(obj))
     elm_widget_theme_klass_set(obj, "radio");
   obj = efl_constructor(efl_super(obj, MY_CLASS));
   evas_object_smart_callbacks_descriptions_set(obj, _smart_callbacks);

   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd, NULL);

   /* in newer APIs the toggle is toggeled in check via the clickable interface */
   if (elm_widget_is_legacy(obj))
     elm_layout_signal_callback_add
        (obj, "elm,action,radio,toggle", "*", _radio_on_cb, obj);


   if (elm_widget_is_legacy(obj))
     {
        pd->group = calloc(1, sizeof(Group));
        pd->group->radios = eina_list_append(pd->group->radios, obj);
     }

   efl_access_object_role_set(obj, EFL_ACCESS_ROLE_RADIO_BUTTON);
   _elm_access_text_set
     (_elm_access_info_get(obj), ELM_ACCESS_TYPE, E_("Radio"));
   _elm_access_callback_set
     (_elm_access_info_get(obj), ELM_ACCESS_INFO, _access_info_cb, obj);
   _elm_access_callback_set
     (_elm_access_info_get(obj), ELM_ACCESS_STATE, _access_state_cb, obj);

   return obj;
}

/**
 * @internal
 * @brief Destructor for the Efl.Ui.Radio object.
 *
 * Performs cleanup for the radio widget. This includes:
 * - For legacy widgets:
 *   - Removing the radio object from its group (`pd->group->radios`).
 *   - If the group becomes empty after removal, freeing the group structure.
 * - Calling the superclass destructor.
 *
 * @param obj The Efl.Ui.Radio object being destructed (unused directly, superclass uses it).
 * @param pd The private data structure for the radio widget.
 */
EOLIAN static void
_efl_ui_radio_efl_object_destructor(Eo *obj EINA_UNUSED, Efl_Ui_Radio_Data *pd)
{
   if (elm_widget_is_legacy(obj))
     {
        pd->group->radios = eina_list_remove(pd->group->radios, obj);
        if (!pd->group->radios) free(pd->group);
     }

   efl_destructor(efl_super(obj, MY_CLASS));
}

/**
 * @internal
 * @brief Sets an integer value associated with this specific radio button.
 *
 * This value is used in legacy mode to determine which radio button in a group
 * should be selected based on the group's current value.
 * If the widget is legacy and its new `value` matches the `group->value`,
 * this radio button is selected; otherwise, it's deselected.
 *
 * @param obj The radio widget object.
 * @param sd The private data for the radio widget.
 * @param value The integer value to associate with this radio button.
 */
EOLIAN static void
_efl_ui_radio_state_value_set(Eo *obj, Efl_Ui_Radio_Data *sd, int value)
{
   sd->value = value;
   if (elm_widget_is_legacy(obj))
     {
        if (sd->value == sd->group->value) efl_ui_selectable_selected_set(obj, EINA_TRUE);
        else efl_ui_selectable_selected_set(obj, EINA_FALSE);
     }
}

/**
 * @internal
 * @brief Gets the integer value associated with this specific radio button.
 *
 * @param obj The radio widget object (unused).
 * @param sd The private data for the radio widget.
 * @return The integer value of this radio button.
 */
EOLIAN static int
_efl_ui_radio_state_value_get(const Eo *obj EINA_UNUSED, Efl_Ui_Radio_Data *sd)
{
   return sd->value;
}

/**
 * @internal
 * @brief Handles accessibility activation requests for the radio widget.
 *
 * This overrides the `efl_ui_widget_on_access_activate` EOLIAN method.
 * If the widget is not disabled and the activation type is `EFL_UI_ACTIVATE_DEFAULT`,
 * it calls the internal `_activate(obj)` function to perform the activation logic.
 *
 * @param obj The radio widget object.
 * @param _pd Private data of the radio widget (unused).
 * @param act The type of activation requested (e.g., `EFL_UI_ACTIVATE_DEFAULT`).
 * @return EINA_TRUE if the activation was handled (widget not disabled and default action),
 *         EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_radio_efl_ui_widget_on_access_activate(Eo *obj, Efl_Ui_Radio_Data *_pd EINA_UNUSED, Efl_Ui_Activate act)
{
   if (elm_widget_disabled_get(obj)) return EINA_FALSE;
   if (act != EFL_UI_ACTIVATE_DEFAULT) return EINA_FALSE;

   _activate(obj);

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Provides a list of Elementary accessibility actions supported by the radio widget.
 *
 * This overrides the `efl_access_widget_action_elm_actions_get` EOLIAN method.
 * Currently, it returns a static array containing only the "activate" action,
 * which is mapped to the `_key_action_activate` function.
 *
 * The structure of `Efl_Access_Action_Data` is:
 * - `name`: Programmatic name of the action.
 * - `action`: Localized name of the action.
 * - `keybinding`: Associated keybinding (if any, NULL here).
 * - `f`: Callback function `Eina_Bool (*f)(Evas_Object *obj, const char *params)`.
 *
 * @param obj The radio widget object (unused).
 * @param pd Private data of the radio widget (unused).
 * @return A pointer to a static array of `Efl_Access_Action_Data`. The array is
 *         terminated by an element with all NULL members.
 *         Example: `{{ "activate", "activate", NULL, _key_action_activate}, {NULL,NULL,NULL,NULL}}`
 */
EOLIAN const Efl_Access_Action_Data *
_efl_ui_radio_efl_access_widget_action_elm_actions_get(const Eo *obj EINA_UNUSED, Efl_Ui_Radio_Data *pd EINA_UNUSED)
{
   static Efl_Access_Action_Data atspi_actions[] = {
          { "activate", "activate", NULL, _key_action_activate},
          { NULL, NULL, NULL, NULL }
   };
   return &atspi_actions[0];
}

/**
 * @internal
 * @brief Gets the accessibility state set for the radio object.
 *
 * This overrides the `efl_access_object_state_set_get` EOLIAN method.
 * It retrieves the state set from the superclass and then adds the
 * `EFL_ACCESS_STATE_TYPE_CHECKED` state if this radio button is currently
 * the selected one in its group (determined by `elm_radio_selected_object_get(obj)`).
 * This is particularly relevant for AT-SPI (Accessibility Toolkit Service Provider Interface).
 *
 * @param obj The radio widget object.
 * @param pd Private data of the radio widget (unused).
 * @return The `Efl_Access_State_Set` for the object, indicating its current states
 *         (e.g., focusable, checked, etc.).
 */
EOLIAN Efl_Access_State_Set
_efl_ui_radio_efl_access_object_state_set_get(const Eo *obj, Efl_Ui_Radio_Data *pd EINA_UNUSED)
{
   Efl_Access_State_Set ret;

   ret = efl_access_object_state_set_get(efl_super(obj, EFL_UI_RADIO_CLASS));
   if (obj == elm_radio_selected_object_get(obj))
     STATE_TYPE_SET(ret, EFL_ACCESS_STATE_TYPE_CHECKED);

   return ret;
}

/* Internal EO APIs and hidden overrides */

ELM_WIDGET_KEY_DOWN_DEFAULT_IMPLEMENT(efl_ui_radio, Efl_Ui_Radio_Data)
EFL_UI_LAYOUT_TEXT_ALIASES_IMPLEMENT(MY_CLASS_PFX)

#define EFL_UI_RADIO_EXTRA_OPS \
   EFL_UI_LAYOUT_TEXT_ALIASES_OPS(MY_CLASS_PFX)

#include "efl_ui_radio.eo.c"
#include "efl_ui_radio_group.eo.c"
#include "efl_ui_radio_eo.legacy.c"

#include "efl_ui_radio_legacy_eo.h"
#include "efl_ui_radio_legacy_part.eo.h"

#define MY_CLASS_NAME_LEGACY "elm_radio"
/* Legacy APIs */

/**
 * @internal
 * @brief Legacy class constructor for elm_radio.
 *
 * Registers the legacy type name "elm_radio" with the Evas smart system.
 * This allows old code using `elm_radio_add` to work with the new Eo-based widget.
 *
 * @param klass The Efl_Class being constructed for the legacy radio type.
 */
static void
_efl_ui_radio_legacy_class_constructor(Efl_Class *klass)
{
   evas_smart_legacy_type_register(MY_CLASS_NAME_LEGACY, klass);
}

/**
 * @internal
 * @brief Legacy object constructor for elm_radio.
 *
 * This function is called when an `elm_radio` is created using legacy APIs.
 * It calls the superclass constructor for `EFL_UI_RADIO_LEGACY_CLASS`,
 * sets the Evas object type to "elm_radio" for compatibility, and
 * initializes legacy focus handling.
 *
 * @param obj The legacy radio widget object being constructed.
 * @param _pd Private data for the legacy radio widget (unused).
 * @return The constructed Eo object.
 */
EOLIAN static Eo *
_efl_ui_radio_legacy_efl_object_constructor(Eo *obj, void *_pd EINA_UNUSED)
{
   obj = efl_constructor(efl_super(obj, EFL_UI_RADIO_LEGACY_CLASS));
   efl_canvas_object_type_set(obj, MY_CLASS_NAME_LEGACY);
   legacy_object_focus_handle(obj);
   return obj;
}

/**
 * @internal
 * @brief Legacy theme application for elm_radio.
 *
 * Overrides `efl_ui_widget_theme_apply` for the legacy radio.
 * After the superclass applies the theme, this function calls
 * `_elm_layout_legacy_icon_signal_emit(obj)` if the object is finalized.
 * This is a FIXME related to how icons/content are handled in legacy layouts,
 * specifically because the radio uses "elm.swallow.content" instead of a
 * standard "elm.swallow.icon".
 *
 * @param obj The legacy radio widget object.
 * @param _pd Private data for the legacy radio widget (unused).
 * @return The result of the superclass's theme_apply operation, or
 *         EFL_UI_THEME_APPLY_ERROR_GENERIC on early failure.
 */
EOLIAN static Eina_Error
_efl_ui_radio_legacy_efl_ui_widget_theme_apply(Eo *obj, void *_pd EINA_UNUSED)
{
   Eina_Error int_ret = EFL_UI_THEME_APPLY_ERROR_GENERIC;
   int_ret = efl_ui_widget_theme_apply(efl_super(obj, EFL_UI_RADIO_LEGACY_CLASS));
   if (int_ret == EFL_UI_THEME_APPLY_ERROR_GENERIC) return int_ret;

   /* FIXME: replicated from elm_layout just because radio's icon
    * spot is elm.swallow.content, not elm.swallow.icon. Fix that
    * whenever we can changed the theme API */
   if (efl_finalized_get(obj)) _elm_layout_legacy_icon_signal_emit(obj);

   return int_ret;
}

/* FIXME: replicated from elm_layout just because radio's icon spot
 * is elm.swallow.content, not elm.swallow.icon. Fix that whenever we
 * can changed the theme API */
/**
 * @internal
 * @brief Legacy sub-object deletion for elm_radio.
 *
 * Overrides `efl_ui_widget_sub_object_del` for the legacy radio.
 * After the superclass handles sub-object deletion, this function calls
 * `_elm_layout_legacy_icon_signal_emit(obj)`.
 * This is part of the FIXME related to legacy icon/content handling,
 * ensuring UI updates correctly when content changes.
 *
 * @param obj The legacy radio widget object.
 * @param _pd Private data for the legacy radio widget (unused).
 * @param sobj The sub-object being deleted.
 * @return EINA_TRUE if the sub-object was successfully deleted by the superclass,
 *         EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_radio_legacy_efl_ui_widget_widget_sub_object_del(Eo *obj, void *_pd EINA_UNUSED, Evas_Object *sobj)
{
   Eina_Bool int_ret = EINA_FALSE;

   int_ret = elm_widget_sub_object_del(efl_super(obj, EFL_UI_RADIO_LEGACY_CLASS), sobj);
   if (!int_ret) return EINA_FALSE;

   _elm_layout_legacy_icon_signal_emit(obj);

   return EINA_TRUE;
}

/* FIXME: replicated from elm_layout just because radio's icon spot
 * is elm.swallow.content, not elm.swallow.icon. Fix that whenever we
 * can changed the theme API */
/**
 * @internal
 * @brief Legacy content setting for elm_radio.
 *
 * Implements `efl_content_set` for the legacy radio, typically for the
 * "elm.swallow.content" part. After the superclass sets the content,
 * this function calls `_elm_layout_legacy_icon_signal_emit(obj)`.
 * This is part of the FIXME related to legacy icon/content handling.
 *
 * @param obj The legacy radio widget object.
 * @param _pd Private data for the legacy radio widget (unused).
 * @param part The name of the part to set content into (e.g., "elm.swallow.content").
 * @param content The Evas_Object to set as content.
 * @return EINA_TRUE if the content was successfully set by the superclass,
 *         EINA_FALSE otherwise.
 */
static Eina_Bool
_efl_ui_radio_legacy_content_set(Eo *obj, void *_pd EINA_UNUSED, const char *part, Evas_Object *content)
{
   Eina_Bool int_ret = EINA_FALSE;

   int_ret = efl_content_set(efl_part(efl_super(obj, EFL_UI_RADIO_LEGACY_CLASS), part), content);
   if (!int_ret) return EINA_FALSE;

   _elm_layout_legacy_icon_signal_emit(obj);

   return EINA_TRUE;
}

/* Efl.Part begin */

/**
 * @internal
 * @brief Checks if a given part name is the specific content part for legacy radio.
 *
 * This function is used by the Efl.Part interface implementation to identify
 * if a part name refers to the main content swallow part of the legacy radio,
 * which is "elm.swallow.content".
 *
 * @param obj The legacy radio widget object (unused).
 * @param part The part name string to check.
 * @return EINA_TRUE if the part name is "elm.swallow.content", EINA_FALSE otherwise.
 */
static Eina_Bool
_part_is_efl_ui_radio_legacy_part(const Eo *obj EINA_UNUSED, const char *part)
{
   return eina_streq(part, "elm.swallow.content");
}

ELM_PART_OVERRIDE_PARTIAL(efl_ui_radio_legacy, EFL_UI_RADIO_LEGACY, void, _part_is_efl_ui_radio_legacy_part)
ELM_PART_OVERRIDE_CONTENT_SET_NO_SD(efl_ui_radio_legacy)
#include "efl_ui_radio_legacy_part.eo.c"

/* Efl.Part end */

EAPI Evas_Object *
elm_radio_add(Evas_Object *parent)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);
   return elm_legacy_add(EFL_UI_RADIO_LEGACY_CLASS, parent);
}

EAPI void
elm_radio_value_set(Evas_Object *obj, int value)
{
   EINA_SAFETY_ON_FALSE_RETURN(elm_widget_is_legacy(obj));
   ELM_RADIO_DATA_GET(obj, sd);

   if (value == sd->group->value) return;
   sd->group->value = value;
   if (sd->group->valuep) *(sd->group->valuep) = sd->group->value;
   _state_set_all(sd, EINA_FALSE);
}

EAPI int
elm_radio_value_get(const Evas_Object *obj)
{
   EINA_SAFETY_ON_FALSE_RETURN_VAL(elm_widget_is_legacy(obj), 0);
   ELM_RADIO_DATA_GET(obj, sd);
   return sd->group->value;
}

EAPI void
elm_radio_value_pointer_set(Efl_Ui_Radio *obj, int *valuep)
{
   EINA_SAFETY_ON_FALSE_RETURN(elm_widget_is_legacy(obj));
   ELM_RADIO_DATA_GET(obj, sd);

   if (valuep)
     {
        sd->group->valuep = valuep;
        if (*(sd->group->valuep) != sd->group->value)
          {
             sd->group->value = *(sd->group->valuep);
             _state_set_all(sd, EINA_FALSE);
          }
     }
   else sd->group->valuep = NULL;
}

EAPI Efl_Canvas_Object *
elm_radio_selected_object_get(const Efl_Ui_Radio *obj)
{
   EINA_SAFETY_ON_FALSE_RETURN_VAL(elm_widget_is_legacy(obj), NULL);
   ELM_RADIO_DATA_GET(obj, sd);

   Eina_List *l;
   Evas_Object *child;

   EINA_LIST_FOREACH(sd->group->radios, l, child)
     {
        ELM_RADIO_DATA_GET(child, sdc);

        if (sdc->value == sd->group->value) return child;
     }

   return NULL;
}

EAPI void
elm_radio_group_add(Efl_Ui_Radio *obj, Efl_Ui_Radio *group)
{
   EINA_SAFETY_ON_FALSE_RETURN(elm_widget_is_legacy(obj));
   EINA_SAFETY_ON_FALSE_RETURN(elm_widget_is_legacy(group));
   ELM_RADIO_DATA_GET(group, sdg);
   ELM_RADIO_DATA_GET(obj, sd);

   if (!sdg)
     {
        if (eina_list_count(sd->group->radios) == 1) return;
        sd->group->radios = eina_list_remove(sd->group->radios, obj);
        sd->group = calloc(1, sizeof(Group));
        sd->group->radios = eina_list_append(sd->group->radios, obj);
     }
   else if (sd->group == sdg->group)
     return;
   else
     {
        sd->group->radios = eina_list_remove(sd->group->radios, obj);
        if (!sd->group->radios) free(sd->group);
        sd->group = sdg->group;
        sd->group->radios = eina_list_append(sd->group->radios, obj);
     }
   if (sd->value == sd->group->value) efl_ui_selectable_selected_set(obj, EINA_TRUE);
   else efl_ui_selectable_selected_set(obj, EINA_FALSE);
}

#include "efl_ui_radio_legacy_eo.c"
