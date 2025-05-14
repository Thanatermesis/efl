/** @internal
 * @brief Definition for the "forward,clicked" event.
 */
EWAPI const Efl_Event_Description _ELM_PLAYER_EVENT_FORWARD_CLICKED =
   EFL_EVENT_DESCRIPTION("forward,clicked");
/** @internal
 * @brief Definition for the "info,clicked" event.
 */
EWAPI const Efl_Event_Description _ELM_PLAYER_EVENT_INFO_CLICKED =
   EFL_EVENT_DESCRIPTION("info,clicked");
/** @internal
 * @brief Definition for the "next,clicked" event.
 */
EWAPI const Efl_Event_Description _ELM_PLAYER_EVENT_NEXT_CLICKED =
   EFL_EVENT_DESCRIPTION("next,clicked");
/** @internal
 * @brief Definition for the "pause,clicked" event.
 */
EWAPI const Efl_Event_Description _ELM_PLAYER_EVENT_PAUSE_CLICKED =
   EFL_EVENT_DESCRIPTION("pause,clicked");
/** @internal
 * @brief Definition for the "play,clicked" event.
 */
EWAPI const Efl_Event_Description _ELM_PLAYER_EVENT_PLAY_CLICKED =
   EFL_EVENT_DESCRIPTION("play,clicked");
/** @internal
 * @brief Definition for the "prev,clicked" event.
 */
EWAPI const Efl_Event_Description _ELM_PLAYER_EVENT_PREV_CLICKED =
   EFL_EVENT_DESCRIPTION("prev,clicked");
/** @internal
 * @brief Definition for the "rewind,clicked" event.
 */
EWAPI const Efl_Event_Description _ELM_PLAYER_EVENT_REWIND_CLICKED =
   EFL_EVENT_DESCRIPTION("rewind,clicked");
/** @internal
 * @brief Definition for the "quality,clicked" event.
 */
EWAPI const Efl_Event_Description _ELM_PLAYER_EVENT_QUALITY_CLICKED =
   EFL_EVENT_DESCRIPTION("quality,clicked");
/** @internal
 * @brief Definition for the "eject,clicked" event.
 */
EWAPI const Efl_Event_Description _ELM_PLAYER_EVENT_EJECT_CLICKED =
   EFL_EVENT_DESCRIPTION("eject,clicked");
/** @internal
 * @brief Definition for the "volume,clicked" event.
 */
EWAPI const Efl_Event_Description _ELM_PLAYER_EVENT_VOLUME_CLICKED =
   EFL_EVENT_DESCRIPTION("volume,clicked");
/** @internal
 * @brief Definition for the "mute,clicked" event.
 */
EWAPI const Efl_Event_Description _ELM_PLAYER_EVENT_MUTE_CLICKED =
   EFL_EVENT_DESCRIPTION("mute,clicked");

/**
 * @internal
 * @brief Implements the Efl.Object.constructor method for Elm_Player.
 *
 * This function is called when a new Elm_Player object is constructed.
 * It initializes the object and its private data.
 *
 * @param obj The Evas Object (Eo) to construct.
 * @param pd Pointer to the private data structure of the Elm_Player object.
 * @return The constructed Evas Object, or @c NULL on failure.
 */
Efl_Object *_elm_player_efl_object_constructor(Eo *obj, Elm_Player_Data *pd);

/**
 * @internal
 * @brief Implements the Efl.Ui.Widget.theme_apply method for Elm_Player.
 *
 * This function is called when the theme is applied or changed for the Elm_Player widget.
 * It allows the widget to update its appearance based on the new theme.
 *
 * @param obj The Evas Object (Eo) representing the Elm_Player widget.
 * @param pd Pointer to the private data structure of the Elm_Player object.
 * @return #EINA_ERROR_NONE on success, or an error code on failure.
 */
Eina_Error _elm_player_efl_ui_widget_theme_apply(Eo *obj, Elm_Player_Data *pd);

/**
 * @internal
 * @brief Implements the Efl.Ui.Widget.input_event_handler method for Elm_Player.
 *
 * This function handles input events (e.g., mouse clicks, key presses) for the Elm_Player widget.
 *
 * @param obj The Evas Object (Eo) representing the Elm_Player widget.
 * @param pd Pointer to the private data structure of the Elm_Player object.
 * @param eo_event The Efl_Event structure containing details about the input event.
 * @param source The Efl_Canvas_Object that originated the event.
 * @return #EINA_TRUE if the event was handled, #EINA_FALSE otherwise.
 */
Eina_Bool _elm_player_efl_ui_widget_widget_input_event_handler(Eo *obj, Elm_Player_Data *pd, const Efl_Event *eo_event, Efl_Canvas_Object *source);

/**
 * @internal
 * @brief Implements the Efl.Access.Widget.Action.elm_actions_get method for Elm_Player.
 *
 * This function retrieves the list of accessibility actions available for the Elm_Player widget.
 *
 * @param obj The Evas Object (Eo) representing the Elm_Player widget.
 * @param pd Pointer to the private data structure of the Elm_Player object.
 * @return A pointer to an array of Efl_Access_Action_Data, or @c NULL if no actions are available.
 *         The last element of the array must have its 'name' field set to @c NULL.
 *         Example:
 *         <pre>
 *         static const Efl_Access_Action_Data actions[] = {
 *           { "play", "Play the media" },
 *           { "pause", "Pause the media" },
 *           { NULL, NULL }
 *         };
 *         return actions;
 *         </pre>
 */
const Efl_Access_Action_Data *_elm_player_efl_access_widget_action_elm_actions_get(const Eo *obj, Elm_Player_Data *pd);

/**
 * @internal
 * @brief Implements the Efl.Part.part_get method for Elm_Player.
 *
 * This function retrieves a specific part of the Elm_Player widget by its name.
 * Parts are sub-objects that make up the widget, e.g., "play_button", "slider".
 *
 * @param obj The Evas Object (Eo) representing the Elm_Player widget.
 * @param pd Pointer to the private data structure of the Elm_Player object.
 * @param name The name of the part to retrieve.
 * @return The Evas Object (Eo) representing the requested part, or @c NULL if the part is not found.
 */
Efl_Object *_elm_player_efl_part_part_get(const Eo *obj, Elm_Player_Data *pd, const char *name);

/**
 * @internal
 * @brief Initializes the Elm_Player class.
 *
 * This static function is called once when the Elm_Player class is first used.
 * It sets up the class's operations (methods) and properties.
 *
 * @param klass The Efl_Class to initialize.
 * @return #EINA_TRUE on success, #EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_player_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_PLAYER_EXTRA_OPS
#define ELM_PLAYER_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(efl_constructor, _elm_player_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_theme_apply, _elm_player_efl_ui_widget_theme_apply),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_input_event_handler, _elm_player_efl_ui_widget_widget_input_event_handler),
      EFL_OBJECT_OP_FUNC(efl_access_widget_action_elm_actions_get, _elm_player_efl_access_widget_action_elm_actions_get),
      EFL_OBJECT_OP_FUNC(efl_part_get, _elm_player_efl_part_part_get),
      ELM_PLAYER_EXTRA_OPS
   );
   opsp = &ops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @internal
 * @brief Describes the Elm_Player class.
 *
 * This static constant structure provides metadata about the Elm_Player class,
 * such as its version, name, type, size of instance data, and pointers to
 * initializer and constructor functions.
 */
static const Efl_Class_Description _elm_player_class_desc = {
   EO_VERSION, /**< The EO API version for this class. */
   "Elm.Player", /**< The name of the class. */
   EFL_CLASS_TYPE_REGULAR, /**< The type of the class (regular, interface, mixin). */
   sizeof(Elm_Player_Data), /**< The size of the private data structure for instances of this class. */
   _elm_player_class_initializer, /**< Pointer to the class initializer function. */
   _elm_player_class_constructor, /**< Pointer to the class constructor function (called before object constructor). */
   NULL /**< Pointer to the class destructor function. */
};

EFL_DEFINE_CLASS(elm_player_class_get, &_elm_player_class_desc, EFL_UI_LAYOUT_BASE_CLASS, EFL_ACCESS_WIDGET_ACTION_MIXIN, ELM_LAYOUT_MIXIN, EFL_UI_LEGACY_INTERFACE, NULL);
