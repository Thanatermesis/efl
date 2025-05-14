/**
 * @file
 * @brief These routines are the Evas Object (EO) C API implementation for the Elm Scroller class.
 *
 * Elm Scroller is a widget that provides a scrollable view for its content.
 * This file contains the actual C function implementations that back the EO API
 * defined in elm_scroller_eo.h. It includes method implementations,
 * event definitions, and class construction logic.
 *
 * @ingroup Elm_Scroller
 */

/**
 * @internal
 * @brief Event descriptors for the Elm Scroller widget.
 *
 * These constants define the Efl_Event_Description for events emitted by Elm_Scroller.
 * They are used internally and by applications to register event callbacks.
 * The public macros for these events are defined in elm_scroller_eo.h.
 */
EWAPI const Efl_Event_Description _ELM_SCROLLER_EVENT_SCROLL_PAGE_CHANGED =
   EFL_EVENT_DESCRIPTION("scroll,page,changed");
EWAPI const Efl_Event_Description _ELM_SCROLLER_EVENT_HBAR_UNPRESS =
   EFL_EVENT_DESCRIPTION("hbar,unpress");
EWAPI const Efl_Event_Description _ELM_SCROLLER_EVENT_HBAR_PRESS =
   EFL_EVENT_DESCRIPTION("hbar,press");
EWAPI const Efl_Event_Description _ELM_SCROLLER_EVENT_HBAR_DRAG =
   EFL_EVENT_DESCRIPTION("hbar,drag");
EWAPI const Efl_Event_Description _ELM_SCROLLER_EVENT_VBAR_UNPRESS =
   EFL_EVENT_DESCRIPTION("vbar,unpress");
EWAPI const Efl_Event_Description _ELM_SCROLLER_EVENT_VBAR_PRESS =
   EFL_EVENT_DESCRIPTION("vbar,press");
EWAPI const Efl_Event_Description _ELM_SCROLLER_EVENT_VBAR_DRAG =
   EFL_EVENT_DESCRIPTION("vbar,drag");
EWAPI const Efl_Event_Description _ELM_SCROLLER_EVENT_SCROLL_LEFT =
   EFL_EVENT_DESCRIPTION("scroll,left");
EWAPI const Efl_Event_Description _ELM_SCROLLER_EVENT_SCROLL_RIGHT =
   EFL_EVENT_DESCRIPTION("scroll,right");
EWAPI const Efl_Event_Description _ELM_SCROLLER_EVENT_SCROLL_UP =
   EFL_EVENT_DESCRIPTION("scroll,up");
EWAPI const Efl_Event_Description _ELM_SCROLLER_EVENT_SCROLL_DOWN =
   EFL_EVENT_DESCRIPTION("scroll,down");
EWAPI const Efl_Event_Description _ELM_SCROLLER_EVENT_EDGE_LEFT =
   EFL_EVENT_DESCRIPTION("edge,left");
EWAPI const Efl_Event_Description _ELM_SCROLLER_EVENT_EDGE_RIGHT =
   EFL_EVENT_DESCRIPTION("edge,right");
EWAPI const Efl_Event_Description _ELM_SCROLLER_EVENT_EDGE_TOP =
   EFL_EVENT_DESCRIPTION("edge,top");
EWAPI const Efl_Event_Description _ELM_SCROLLER_EVENT_EDGE_BOTTOM =
   EFL_EVENT_DESCRIPTION("edge,bottom");

/**
 * @internal
 * @brief Implements the EO API function elm_obj_scroller_custom_widget_base_theme_set.
 *
 * This function is the C-level implementation for setting custom theme
 * elements for the scroller. It is called via the EO system when
 * elm_obj_scroller_custom_widget_base_theme_set() is invoked on an Elm_Scroller object.
 *
 * @param obj The Eo object (Elm_Scroller instance).
 * @param pd The private data for the Elm_Scroller instance.
 * @param klass The class name for the custom theme.
 * @param group The group name for the custom theme.
 */
void _elm_scroller_custom_widget_base_theme_set(Eo *obj, Elm_Scroller_Data *pd, const char *klass, const char *group);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_scroller_custom_widget_base_theme_set, EFL_FUNC_CALL(klass, group), const char *klass, const char *group);

/**
 * @internal
 * @brief Implements the EO API function elm_obj_scroller_page_scroll_limit_set.
 *
 * This function is the C-level implementation for setting the maximum
 * movable page at a flicking. It is called via the EO system when
 * elm_obj_scroller_page_scroll_limit_set() is invoked on an Elm_Scroller object.
 *
 * @param obj The Eo object (Elm_Scroller instance).
 * @param pd The private data for the Elm_Scroller instance.
 * @param page_limit_h The maximum of the movable horizontal page.
 * @param page_limit_v The maximum of the movable vertical page.
 */
void _elm_scroller_page_scroll_limit_set(const Eo *obj, Elm_Scroller_Data *pd, int page_limit_h, int page_limit_v);

EOAPI EFL_VOID_FUNC_BODYV_CONST(elm_obj_scroller_page_scroll_limit_set, EFL_FUNC_CALL(page_limit_h, page_limit_v), int page_limit_h, int page_limit_v);

/**
 * @internal
 * @brief Implements the EO API function elm_obj_scroller_page_scroll_limit_get.
 *
 * This function is the C-level implementation for getting the maximum
 * movable page at a flicking. It is called via the EO system when
 * elm_obj_scroller_page_scroll_limit_get() is invoked on an Elm_Scroller object.
 *
 * @param obj The Eo object (Elm_Scroller instance).
 * @param pd The private data for the Elm_Scroller instance.
 * @param page_limit_h Pointer to store the maximum of the movable horizontal page.
 * @param page_limit_v Pointer to store the maximum of the movable vertical page.
 */
void _elm_scroller_page_scroll_limit_get(const Eo *obj, Elm_Scroller_Data *pd, int *page_limit_h, int *page_limit_v);

EOAPI EFL_VOID_FUNC_BODYV_CONST(elm_obj_scroller_page_scroll_limit_get, EFL_FUNC_CALL(page_limit_h, page_limit_v), int *page_limit_h, int *page_limit_v);

/**
 * @internal
 * @brief Implements the Efl_Object constructor for Elm_Scroller.
 *
 * This function is called when an Elm_Scroller object is constructed.
 * It performs initialization specific to Elm_Scroller instances.
 *
 * @param obj The Eo object (Elm_Scroller instance) being constructed.
 * @param pd The private data for the Elm_Scroller instance.
 * @return The initialized Eo object, or @c NULL on failure.
 */
Efl_Object *_elm_scroller_efl_object_constructor(Eo *obj, Elm_Scroller_Data *pd);

/**
 * @internal
 * @brief Implements the Efl_Gfx_Entity position_set method for Elm_Scroller.
 *
 * This function is the C-level implementation for setting the position of the scroller.
 * It is called via the EO system when efl_gfx_entity_position_set() is invoked.
 *
 * @param obj The Eo object (Elm_Scroller instance).
 * @param pd The private data for the Elm_Scroller instance.
 * @param pos The new 2D position (Eina_Position2D).
 */
void _elm_scroller_efl_gfx_entity_position_set(Eo *obj, Elm_Scroller_Data *pd, Eina_Position2D pos);

/**
 * @internal
 * @brief Implements the Efl_Gfx_Entity size_set method for Elm_Scroller.
 *
 * This function is the C-level implementation for setting the size of the scroller.
 * It is called via the EO system when efl_gfx_entity_size_set() is invoked.
 *
 * @param obj The Eo object (Elm_Scroller instance).
 * @param pd The private data for the Elm_Scroller instance.
 * @param size The new 2D size (Eina_Size2D).
 */
void _elm_scroller_efl_gfx_entity_size_set(Eo *obj, Elm_Scroller_Data *pd, Eina_Size2D size);

/**
 * @internal
 * @brief Implements the Efl_Canvas_Group member_add method for Elm_Scroller.
 *
 * This function handles adding a sub-object to the scroller's canvas group.
 *
 * @param obj The Eo object (Elm_Scroller instance).
 * @param pd The private data for the Elm_Scroller instance.
 * @param sub_obj The sub-object to add.
 */
void _elm_scroller_efl_canvas_group_group_member_add(Eo *obj, Elm_Scroller_Data *pd, Efl_Canvas_Object *sub_obj);

/**
 * @internal
 * @brief Implements the Efl_Content content_set method for Elm_Scroller.
 *
 * This function sets the main content of the scroller.
 *
 * @param obj The Eo object (Elm_Scroller instance).
 * @param pd The private data for the Elm_Scroller instance.
 * @param content The Efl_Gfx_Entity to set as content.
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise.
 */
Eina_Bool _elm_scroller_efl_content_content_set(Eo *obj, Elm_Scroller_Data *pd, Efl_Gfx_Entity *content);

/**
 * @internal
 * @brief Implements the Efl_Content content_get method for Elm_Scroller.
 *
 * This function retrieves the main content of the scroller.
 *
 * @param obj The Eo object (Elm_Scroller instance).
 * @param pd The private data for the Elm_Scroller instance.
 * @return The current Efl_Gfx_Entity content, or @c NULL if none is set.
 */
Efl_Gfx_Entity *_elm_scroller_efl_content_content_get(const Eo *obj, Elm_Scroller_Data *pd);

/**
 * @internal
 * @brief Implements the Efl_Content content_unset method for Elm_Scroller.
 *
 * This function unsets (removes) the main content of the scroller.
 *
 * @param obj The Eo object (Elm_Scroller instance).
 * @param pd The private data for the Elm_Scroller instance.
 * @return The previously set Efl_Gfx_Entity content, or @c NULL if none was set.
 */
Efl_Gfx_Entity *_elm_scroller_efl_content_content_unset(Eo *obj, Elm_Scroller_Data *pd);

/**
 * @internal
 * @brief Implements the Efl_Ui_Widget theme_apply method for Elm_Scroller.
 *
 * This function is called to apply the theme to the scroller widget.
 *
 * @param obj The Eo object (Elm_Scroller instance).
 * @param pd The private data for the Elm_Scroller instance.
 * @return An Eina_Error code, typically @c EINA_ERROR_NONE on success.
 */
Eina_Error _elm_scroller_efl_ui_widget_theme_apply(Eo *obj, Elm_Scroller_Data *pd);

/**
 * @internal
 * @brief Implements the Efl_Ui_Widget on_access_activate method for Elm_Scroller.
 *
 * This function handles accessibility activation for the scroller.
 *
 * @param obj The Eo object (Elm_Scroller instance).
 * @param pd The private data for the Elm_Scroller instance.
 * @param act The type of activation.
 * @return @c EINA_TRUE if handled, @c EINA_FALSE otherwise.
 */
Eina_Bool _elm_scroller_efl_ui_widget_on_access_activate(Eo *obj, Elm_Scroller_Data *pd, Efl_Ui_Activate act);

/**
 * @internal
 * @brief Implements the Efl_Ui_Widget sub_object_del method for Elm_Scroller.
 *
 * This function handles the deletion of a sub-object from the scroller.
 *
 * @param obj The Eo object (Elm_Scroller instance).
 * @param pd The private data for the Elm_Scroller instance.
 * @param sub_obj The sub-object being deleted.
 * @return @c EINA_TRUE if the sub-object was successfully processed, @c EINA_FALSE otherwise.
 */
Eina_Bool _elm_scroller_efl_ui_widget_widget_sub_object_del(Eo *obj, Elm_Scroller_Data *pd, Efl_Canvas_Object *sub_obj);

/**
 * @internal
 * @brief Implements the Efl_Ui_Widget input_event_handler method for Elm_Scroller.
 *
 * This function processes input events directed at the scroller widget.
 *
 * @param obj The Eo object (Elm_Scroller instance).
 * @param pd The private data for the Elm_Scroller instance.
 * @param eo_event The Efl_Event payload for the input event.
 * @param source The source object of the event.
 * @return @c EINA_TRUE if the event was handled, @c EINA_FALSE otherwise.
 */
Eina_Bool _elm_scroller_efl_ui_widget_widget_input_event_handler(Eo *obj, Elm_Scroller_Data *pd, const Efl_Event *eo_event, Efl_Canvas_Object *source);

/**
 * @internal
 * @brief Implements the Elm_Interface_Scrollable page_size_set method for Elm_Scroller.
 *
 * Sets the size of a page in the scroller, relative to the viewport size.
 *
 * @param obj The Eo object (Elm_Scroller instance).
 * @param pd The private data for the Elm_Scroller instance.
 * @param x The horizontal page size.
 * @param y The vertical page size.
 */
void _elm_scroller_elm_interface_scrollable_page_size_set(Eo *obj, Elm_Scroller_Data *pd, int x, int y);

/**
 * @internal
 * @brief Implements the Elm_Interface_Scrollable policy_set method for Elm_Scroller.
 *
 * Sets the scrollbar visibility policy for the scroller.
 *
 * @param obj The Eo object (Elm_Scroller instance).
 * @param pd The private data for the Elm_Scroller instance.
 * @param hbar The policy for the horizontal scrollbar.
 * @param vbar The policy for the vertical scrollbar.
 */
void _elm_scroller_elm_interface_scrollable_policy_set(Eo *obj, Elm_Scroller_Data *pd, Elm_Scroller_Policy hbar, Elm_Scroller_Policy vbar);

/**
 * @internal
 * @brief Implements the Elm_Interface_Scrollable single_direction_set method for Elm_Scroller.
 *
 * Sets the single direction scroll policy.
 *
 * @param obj The Eo object (Elm_Scroller instance).
 * @param pd The private data for the Elm_Scroller instance.
 * @param single_dir The single direction policy to set.
 */
void _elm_scroller_elm_interface_scrollable_single_direction_set(Eo *obj, Elm_Scroller_Data *pd, Elm_Scroller_Single_Direction single_dir);

/**
 * @internal
 * @brief Implements the Elm_Interface_Scrollable single_direction_get method for Elm_Scroller.
 *
 * Gets the single direction scroll policy.
 *
 * @param obj The Eo object (Elm_Scroller instance).
 * @param pd The private data for the Elm_Scroller instance.
 * @return The current single direction policy.
 */
Elm_Scroller_Single_Direction _elm_scroller_elm_interface_scrollable_single_direction_get(const Eo *obj, Elm_Scroller_Data *pd);

/**
 * @internal
 * @brief Implements the Elm_Interface_Scrollable content_loop_set method for Elm_Scroller.
 *
 * Sets content looping for horizontal and vertical directions.
 *
 * @param obj The Eo object (Elm_Scroller instance).
 * @param sd The private data for the Elm_Scroller instance.
 * @param loop_h Enable or disable horizontal looping.
 * @param loop_v Enable or disable vertical looping.
 */
void _elm_scroller_elm_interface_scrollable_content_loop_set(Eo *obj, Elm_Scroller_Data *sd, Eina_Bool loop_h, Eina_Bool loop_v);

/**
 * @internal
 * @brief Implements the Elm_Interface_Scrollable content_loop_get method for Elm_Scroller.
 *
 * Gets content looping status for horizontal and vertical directions.
 *
 * @param obj The Eo object (Elm_Scroller instance), unused.
 * @param sd The private data for the Elm_Scroller instance.
 * @param loop_h Pointer to store horizontal looping status.
 * @param loop_v Pointer to store vertical looping status.
 */
void _elm_scroller_elm_interface_scrollable_content_loop_get(Eo *obj EINA_UNUSED, Elm_Scroller_Data *sd, Eina_Bool *loop_h, Eina_Bool *loop_v);

/**
 * @internal
 * @brief Implements the Efl_Access_Widget_Action elm_actions_get method for Elm_Scroller.
 *
 * Retrieves the list of Elementary actions available for the scroller.
 *
 * @param obj The Eo object (Elm_Scroller instance).
 * @param pd The private data for the Elm_Scroller instance.
 * @return A const pointer to an array of Efl_Access_Action_Data, terminated by an entry with a NULL name.
 */
const Efl_Access_Action_Data *_elm_scroller_efl_access_widget_action_elm_actions_get(const Eo *obj, Elm_Scroller_Data *pd);

/**
 * @internal
 * @brief Implements the Efl_Part part_get method for Elm_Scroller.
 *
 * Retrieves a part object from the scroller.
 *
 * @param obj The Eo object (Elm_Scroller instance).
 * @param pd The private data for the Elm_Scroller instance.
 * @param name The name of the part to retrieve.
 * @return The Efl_Object representing the part, or @c NULL if not found.
 */
Efl_Object *_elm_scroller_efl_part_part_get(const Eo *obj, Elm_Scroller_Data *pd, const char *name);

/**
 * @internal
 * @brief Implements the Efl_Ui_Widget focus_state_apply method for Elm_Scroller.
 *
 * Applies a focus state to the scroller widget.
 *
 * @param obj The Eo object (Elm_Scroller instance).
 * @param pd The private data for the Elm_Scroller instance.
 * @param current_state The current focus state of the widget.
 * @param configured_state Pointer to store the newly configured focus state.
 * @param redirect The widget to which focus might be redirected.
 * @return @c EINA_TRUE if the focus state was applied, @c EINA_FALSE otherwise.
 */
Eina_Bool _elm_scroller_efl_ui_widget_focus_state_apply(Eo *obj, Elm_Scroller_Data *pd, Efl_Ui_Widget_Focus_State current_state, Efl_Ui_Widget_Focus_State *configured_state, Efl_Ui_Widget *redirect);

/**
 * @internal
 * @brief Initializes the Elm_Scroller Efl_Class.
 *
 * This function is called once when the Elm_Scroller class is being set up.
 * It registers the Efl_Object_Ops (method implementations) for the class.
 *
 * @param klass The Efl_Class to initialize.
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_scroller_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_SCROLLER_EXTRA_OPS
#define ELM_SCROLLER_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(elm_obj_scroller_custom_widget_base_theme_set, _elm_scroller_custom_widget_base_theme_set),
      EFL_OBJECT_OP_FUNC(elm_obj_scroller_page_scroll_limit_set, _elm_scroller_page_scroll_limit_set),
      EFL_OBJECT_OP_FUNC(elm_obj_scroller_page_scroll_limit_get, _elm_scroller_page_scroll_limit_get),
      EFL_OBJECT_OP_FUNC(efl_constructor, _elm_scroller_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_gfx_entity_position_set, _elm_scroller_efl_gfx_entity_position_set),
      EFL_OBJECT_OP_FUNC(efl_gfx_entity_size_set, _elm_scroller_efl_gfx_entity_size_set),
      EFL_OBJECT_OP_FUNC(efl_canvas_group_member_add, _elm_scroller_efl_canvas_group_group_member_add),
      EFL_OBJECT_OP_FUNC(efl_content_set, _elm_scroller_efl_content_content_set),
      EFL_OBJECT_OP_FUNC(efl_content_get, _elm_scroller_efl_content_content_get),
      EFL_OBJECT_OP_FUNC(efl_content_unset, _elm_scroller_efl_content_content_unset),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_theme_apply, _elm_scroller_efl_ui_widget_theme_apply),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_on_access_activate, _elm_scroller_efl_ui_widget_on_access_activate),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_sub_object_del, _elm_scroller_efl_ui_widget_widget_sub_object_del),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_input_event_handler, _elm_scroller_efl_ui_widget_widget_input_event_handler),
      EFL_OBJECT_OP_FUNC(elm_interface_scrollable_page_size_set, _elm_scroller_elm_interface_scrollable_page_size_set),
      EFL_OBJECT_OP_FUNC(elm_interface_scrollable_policy_set, _elm_scroller_elm_interface_scrollable_policy_set),
      EFL_OBJECT_OP_FUNC(elm_interface_scrollable_single_direction_set, _elm_scroller_elm_interface_scrollable_single_direction_set),
      EFL_OBJECT_OP_FUNC(elm_interface_scrollable_single_direction_get, _elm_scroller_elm_interface_scrollable_single_direction_get),
      EFL_OBJECT_OP_FUNC(elm_interface_scrollable_content_loop_set, _elm_scroller_elm_interface_scrollable_content_loop_set),
      EFL_OBJECT_OP_FUNC(elm_interface_scrollable_content_loop_get, _elm_scroller_elm_interface_scrollable_content_loop_get),
      EFL_OBJECT_OP_FUNC(efl_access_widget_action_elm_actions_get, _elm_scroller_efl_access_widget_action_elm_actions_get),
      EFL_OBJECT_OP_FUNC(efl_part_get, _elm_scroller_efl_part_part_get),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_focus_state_apply, _elm_scroller_efl_ui_widget_focus_state_apply),
      ELM_SCROLLER_EXTRA_OPS
   );
   opsp = &ops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @internal
 * @brief Describes the Elm_Scroller Efl_Class.
 *
 * This structure provides metadata for the Elm_Scroller class,
 * including its name, type, size of private data, and pointers
 * to initializer and constructor functions.
 */
static const Efl_Class_Description _elm_scroller_class_desc = {
   EO_VERSION,
   "Elm.Scroller",
   EFL_CLASS_TYPE_REGULAR,
   sizeof(Elm_Scroller_Data),
   _elm_scroller_class_initializer,
   _elm_scroller_class_constructor,
   NULL
};

EFL_DEFINE_CLASS(elm_scroller_class_get, &_elm_scroller_class_desc, EFL_UI_LAYOUT_BASE_CLASS, ELM_INTERFACE_SCROLLABLE_MIXIN, EFL_ACCESS_WIDGET_ACTION_MIXIN, EFL_UI_SCROLLABLE_INTERFACE, EFL_CONTENT_INTERFACE, ELM_LAYOUT_MIXIN, EFL_UI_LEGACY_INTERFACE, NULL);

#include "elm_scroller_eo.legacy.c"
