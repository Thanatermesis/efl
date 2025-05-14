EWAPI const Efl_Event_Description _ELM_TOOLBAR_EVENT_ITEM_FOCUSED =
   EFL_EVENT_DESCRIPTION("item,focused");
EWAPI const Efl_Event_Description _ELM_TOOLBAR_EVENT_ITEM_UNFOCUSED =
   EFL_EVENT_DESCRIPTION("item,unfocused");

/**
 * @internal
 * @brief Retrieves the currently selected item in the toolbar.
 * This is the actual implementation for elm_obj_toolbar_selected_item_get().
 * @param obj The toolbar object.
 * @param pd The private data of the toolbar object.
 * @return The selected item, or @c NULL if none is selected.
 */
Elm_Widget_Item *_elm_toolbar_selected_item_get(const Eo *obj, Elm_Toolbar_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_toolbar_selected_item_get, Elm_Widget_Item *, NULL);

/**
 * @internal
 * @brief Retrieves the first item in the toolbar.
 * This is the actual implementation for elm_obj_toolbar_first_item_get().
 * @param obj The toolbar object.
 * @param pd The private data of the toolbar object.
 * @return The first item, or @c NULL if the toolbar is empty.
 */
Elm_Widget_Item *_elm_toolbar_first_item_get(const Eo *obj, Elm_Toolbar_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_toolbar_first_item_get, Elm_Widget_Item *, NULL);

/**
 * @internal
 * @brief Retrieves the last item in the toolbar.
 * This is the actual implementation for elm_obj_toolbar_last_item_get().
 * @param obj The toolbar object.
 * @param pd The private data of the toolbar object.
 * @return The last item, or @c NULL if the toolbar is empty.
 */
Elm_Widget_Item *_elm_toolbar_last_item_get(const Eo *obj, Elm_Toolbar_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_toolbar_last_item_get, Elm_Widget_Item *, NULL);

/**
 * @internal
 * @brief Retrieves an iterator over all items in the toolbar.
 * This is the actual implementation for elm_obj_toolbar_items_get().
 * @param obj The toolbar object.
 * @param pd The private data of the toolbar object.
 * @return An Eina_Iterator for the toolbar items, or @c NULL on error.
 *         The caller is responsible for freeing the iterator.
 */
Eina_Iterator *_elm_toolbar_items_get(const Eo *obj, Elm_Toolbar_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_toolbar_items_get, Eina_Iterator *, NULL);

/**
 * @internal
 * @brief Sets the homogeneous mode for the toolbar.
 * This is the actual implementation for elm_obj_toolbar_homogeneous_set().
 * In homogeneous mode, all items are sized equally.
 * @param obj The toolbar object.
 * @param pd The private data of the toolbar object.
 * @param homogeneous @c EINA_TRUE to enable homogeneous mode, @c EINA_FALSE to disable.
 */
void _elm_toolbar_homogeneous_set(Eo *obj, Elm_Toolbar_Data *pd, Eina_Bool homogeneous);

/**
 * @internal
 * @brief Eolian reflection function for the 'homogeneous' property setter.
 *
 * This function is part of the Eolian reflection mechanism. It is called
 * by the Eolian runtime when the 'homogeneous' property is set via
 * efl_property_set() or equivalent. It converts the generic Eina_Value
 * to the specific C type (Eina_Bool) and then calls the
 * corresponding C implementation (elm_obj_toolbar_homogeneous_set).
 *
 * @param obj The Eo object (toolbar instance).
 * @param val An Eina_Value containing the boolean value for the homogeneous state.
 * @return EINA_ERROR_NO_ERROR on successful conversion and call,
 *         EINA_ERROR_VALUE_FAILED if the Eina_Value cannot be converted to Eina_Bool.
 */
static Eina_Error
__eolian_elm_toolbar_homogeneous_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   Eina_Bool cval;
   if (!eina_value_bool_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_toolbar_homogeneous_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_toolbar_homogeneous_set, EFL_FUNC_CALL(homogeneous), Eina_Bool homogeneous);

/**
 * @internal
 * @brief Gets the homogeneous mode of the toolbar.
 * This is the actual implementation for elm_obj_toolbar_homogeneous_get().
 * @param obj The toolbar object.
 * @param pd The private data of the toolbar object.
 * @return @c EINA_TRUE if homogeneous mode is enabled, @c EINA_FALSE otherwise.
 */
Eina_Bool _elm_toolbar_homogeneous_get(const Eo *obj, Elm_Toolbar_Data *pd);

/**
 * @internal
 * @brief Eolian reflection function for the 'homogeneous' property getter.
 *
 * This function is part of the Eolian reflection mechanism. It is called
 * by the Eolian runtime when the 'homogeneous' property is read via
 * efl_property_get() or equivalent. It calls the C implementation
 * (elm_obj_toolbar_homogeneous_get) and wraps the result in an Eina_Value.
 *
 * @param obj The Eo object (toolbar instance).
 * @return An Eina_Value containing the boolean homogeneous state.
 *         The caller is responsible for flushing this Eina_Value.
 */
static Eina_Value
__eolian_elm_toolbar_homogeneous_get_reflect(const Eo *obj)
{
   Eina_Bool val = elm_obj_toolbar_homogeneous_get(obj);
   return eina_value_bool_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_toolbar_homogeneous_get, Eina_Bool, 0);

/**
 * @internal
 * @brief Sets the alignment of items within the toolbar.
 * This is the actual implementation for elm_obj_toolbar_align_set().
 * @param obj The toolbar object.
 * @param pd The private data of the toolbar object.
 * @param align The alignment value (0.0 for left, 0.5 for center, 1.0 for right).
 */
void _elm_toolbar_align_set(Eo *obj, Elm_Toolbar_Data *pd, double align);

/**
 * @internal
 * @brief Eolian reflection function for the 'align' property setter.
 * Converts Eina_Value to double and calls elm_obj_toolbar_align_set.
 * @param obj The Eo object.
 * @param val Eina_Value containing the alignment.
 * @return EINA_ERROR_NO_ERROR on success, EINA_ERROR_VALUE_FAILED on conversion error.
 */
static Eina_Error
__eolian_elm_toolbar_align_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   double cval;
   if (!eina_value_double_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_toolbar_align_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_toolbar_align_set, EFL_FUNC_CALL(align), double align);

/**
 * @internal
 * @brief Gets the alignment of items within the toolbar.
 * This is the actual implementation for elm_obj_toolbar_align_get().
 * @param obj The toolbar object.
 * @param pd The private data of the toolbar object.
 * @return The alignment value.
 */
double _elm_toolbar_align_get(const Eo *obj, Elm_Toolbar_Data *pd);

/**
 * @internal
 * @brief Eolian reflection function for the 'align' property getter.
 * Calls elm_obj_toolbar_align_get and returns the value as an Eina_Value.
 * @param obj The Eo object.
 * @return Eina_Value containing the alignment.
 */
static Eina_Value
__eolian_elm_toolbar_align_get_reflect(const Eo *obj)
{
   double val = elm_obj_toolbar_align_get(obj);
   return eina_value_double_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_toolbar_align_get, double, 0);

/**
 * @internal
 * @brief Sets the item selection mode for the toolbar.
 * This is the actual implementation for elm_obj_toolbar_select_mode_set().
 * @param obj The toolbar object.
 * @param pd The private data of the toolbar object.
 * @param mode The selection mode to set (e.g., ELM_OBJECT_SELECT_MODE_DEFAULT).
 */
void _elm_toolbar_select_mode_set(Eo *obj, Elm_Toolbar_Data *pd, Elm_Object_Select_Mode mode);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_toolbar_select_mode_set, EFL_FUNC_CALL(mode), Elm_Object_Select_Mode mode);

/**
 * @internal
 * @brief Gets the item selection mode of the toolbar.
 * This is the actual implementation for elm_obj_toolbar_select_mode_get().
 * @param obj The toolbar object.
 * @param pd The private data of the toolbar object.
 * @return The current selection mode.
 */
Elm_Object_Select_Mode _elm_toolbar_select_mode_get(const Eo *obj, Elm_Toolbar_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_toolbar_select_mode_get, Elm_Object_Select_Mode, 4 /* Elm.Object.Select_Mode.max */);

/**
 * @internal
 * @brief Sets the icon size for toolbar items.
 * This is the actual implementation for elm_obj_toolbar_icon_size_set().
 * @param obj The toolbar object.
 * @param pd The private data of the toolbar object.
 * @param icon_size The icon size in pixels.
 */
void _elm_toolbar_icon_size_set(Eo *obj, Elm_Toolbar_Data *pd, int icon_size);

/**
 * @internal
 * @brief Eolian reflection function for the 'icon_size' property setter.
 * Converts Eina_Value to int and calls elm_obj_toolbar_icon_size_set.
 * @param obj The Eo object.
 * @param val Eina_Value containing the icon size.
 * @return EINA_ERROR_NO_ERROR on success, EINA_ERROR_VALUE_FAILED on conversion error.
 */
static Eina_Error
__eolian_elm_toolbar_icon_size_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   int cval;
   if (!eina_value_int_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_toolbar_icon_size_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_toolbar_icon_size_set, EFL_FUNC_CALL(icon_size), int icon_size);

/**
 * @internal
 * @brief Gets the icon size for toolbar items.
 * This is the actual implementation for elm_obj_toolbar_icon_size_get().
 * @param obj The toolbar object.
 * @param pd The private data of the toolbar object.
 * @return The icon size in pixels.
 */
int _elm_toolbar_icon_size_get(const Eo *obj, Elm_Toolbar_Data *pd);

/**
 * @internal
 * @brief Eolian reflection function for the 'icon_size' property getter.
 * Calls elm_obj_toolbar_icon_size_get and returns the value as an Eina_Value.
 * @param obj The Eo object.
 * @return Eina_Value containing the icon size.
 */
static Eina_Value
__eolian_elm_toolbar_icon_size_get_reflect(const Eo *obj)
{
   int val = elm_obj_toolbar_icon_size_get(obj);
   return eina_value_int_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_toolbar_icon_size_get, int, 0);

/**
 * @internal
 * @brief Sets the shrink mode for the toolbar.
 * This is the actual implementation for elm_obj_toolbar_shrink_mode_set().
 * The shrink mode determines how items are displayed when they don't fit.
 * @param obj The toolbar object.
 * @param pd The private data of the toolbar object.
 * @param shrink_mode The shrink mode to set (e.g., ELM_TOOLBAR_SHRINK_MENU).
 */
void _elm_toolbar_shrink_mode_set(Eo *obj, Elm_Toolbar_Data *pd, Elm_Toolbar_Shrink_Mode shrink_mode);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_toolbar_shrink_mode_set, EFL_FUNC_CALL(shrink_mode), Elm_Toolbar_Shrink_Mode shrink_mode);

/**
 * @internal
 * @brief Gets the shrink mode of the toolbar.
 * This is the actual implementation for elm_obj_toolbar_shrink_mode_get().
 * @param obj The toolbar object.
 * @param pd The private data of the toolbar object.
 * @return The current shrink mode.
 */
Elm_Toolbar_Shrink_Mode _elm_toolbar_shrink_mode_get(const Eo *obj, Elm_Toolbar_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_toolbar_shrink_mode_get, Elm_Toolbar_Shrink_Mode, 0);

/**
 * @internal
 * @brief Sets the parent object for toolbar item menus.
 * This is the actual implementation for elm_obj_toolbar_menu_parent_set().
 * @param obj The toolbar object.
 * @param pd The private data of the toolbar object.
 * @param parent The Efl_Canvas_Object to be used as the parent for menus.
 */
void _elm_toolbar_menu_parent_set(Eo *obj, Elm_Toolbar_Data *pd, Efl_Canvas_Object *parent);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_toolbar_menu_parent_set, EFL_FUNC_CALL(parent), Efl_Canvas_Object *parent);

/**
 * @internal
 * @brief Gets the parent object for toolbar item menus.
 * This is the actual implementation for elm_obj_toolbar_menu_parent_get().
 * @param obj The toolbar object.
 * @param pd The private data of the toolbar object.
 * @return The parent object for menus.
 */
Efl_Canvas_Object *_elm_toolbar_menu_parent_get(const Eo *obj, Elm_Toolbar_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_toolbar_menu_parent_get, Efl_Canvas_Object *, NULL);

/**
 * @internal
 * @brief Sets the standard priority for visible items in the toolbar.
 * This is the actual implementation for elm_obj_toolbar_standard_priority_set().
 * Items with priority up to this value are shown; others may be in a "more" menu.
 * @param obj The toolbar object.
 * @param pd The private data of the toolbar object.
 * @param priority The standard priority value.
 */
void _elm_toolbar_standard_priority_set(Eo *obj, Elm_Toolbar_Data *pd, int priority);

/**
 * @internal
 * @brief Eolian reflection function for the 'standard_priority' property setter.
 * Converts Eina_Value to int and calls elm_obj_toolbar_standard_priority_set.
 * @param obj The Eo object.
 * @param val Eina_Value containing the standard priority.
 * @return EINA_ERROR_NO_ERROR on success, EINA_ERROR_VALUE_FAILED on conversion error.
 */
static Eina_Error
__eolian_elm_toolbar_standard_priority_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   int cval;
   if (!eina_value_int_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_toolbar_standard_priority_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_toolbar_standard_priority_set, EFL_FUNC_CALL(priority), int priority);

/**
 * @internal
 * @brief Gets the standard priority for visible items in the toolbar.
 * This is the actual implementation for elm_obj_toolbar_standard_priority_get().
 * @param obj The toolbar object.
 * @param pd The private data of the toolbar object.
 * @return The standard priority value.
 */
int _elm_toolbar_standard_priority_get(const Eo *obj, Elm_Toolbar_Data *pd);

/**
 * @internal
 * @brief Eolian reflection function for the 'standard_priority' property getter.
 * Calls elm_obj_toolbar_standard_priority_get and returns the value as an Eina_Value.
 * @param obj The Eo object.
 * @return Eina_Value containing the standard priority.
 */
static Eina_Value
__eolian_elm_toolbar_standard_priority_get_reflect(const Eo *obj)
{
   int val = elm_obj_toolbar_standard_priority_get(obj);
   return eina_value_int_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_toolbar_standard_priority_get, int, 0);

/**
 * @internal
 * @brief Gets the auto-generated "more" item of the toolbar.
 * This is the actual implementation for elm_obj_toolbar_more_item_get().
 * The "more" item is shown when items overflow and shrink mode is ELM_TOOLBAR_SHRINK_MENU or ELM_TOOLBAR_SHRINK_EXPAND.
 * @param obj The toolbar object.
 * @param pd The private data of the toolbar object.
 * @return The "more" item, or @c NULL if not available.
 */
Elm_Widget_Item *_elm_toolbar_more_item_get(const Eo *obj, Elm_Toolbar_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_toolbar_more_item_get, Elm_Widget_Item *, NULL);

/**
 * @internal
 * @brief Inserts a new item before an existing item in the toolbar.
 * This is the actual implementation for elm_obj_toolbar_item_insert_before().
 * @param obj The toolbar object.
 * @param pd The private data of the toolbar object.
 * @param before The item before which to insert the new item.
 * @param icon The icon string (name or path).
 * @param label The label of the new item.
 * @param func The callback function for item selection.
 * @param data User data for the callback function.
 * @return The newly created item, or @c NULL on failure.
 */
Elm_Widget_Item *_elm_toolbar_item_insert_before(Eo *obj, Elm_Toolbar_Data *pd, Elm_Widget_Item *before, const char *icon, const char *label, Evas_Smart_Cb func, const void *data);

EOAPI EFL_FUNC_BODYV(elm_obj_toolbar_item_insert_before, Elm_Widget_Item *, NULL, EFL_FUNC_CALL(before, icon, label, func, data), Elm_Widget_Item *before, const char *icon, const char *label, Evas_Smart_Cb func, const void *data);

/**
 * @internal
 * @brief Inserts a new item after an existing item in the toolbar.
 * This is the actual implementation for elm_obj_toolbar_item_insert_after().
 * @param obj The toolbar object.
 * @param pd The private data of the toolbar object.
 * @param after The item after which to insert the new item.
 * @param icon The icon string (name or path).
 * @param label The label of the new item.
 * @param func The callback function for item selection.
 * @param data User data for the callback function.
 * @return The newly created item, or @c NULL on failure.
 */
Elm_Widget_Item *_elm_toolbar_item_insert_after(Eo *obj, Elm_Toolbar_Data *pd, Elm_Widget_Item *after, const char *icon, const char *label, Evas_Smart_Cb func, const void *data);

EOAPI EFL_FUNC_BODYV(elm_obj_toolbar_item_insert_after, Elm_Widget_Item *, NULL, EFL_FUNC_CALL(after, icon, label, func, data), Elm_Widget_Item *after, const char *icon, const char *label, Evas_Smart_Cb func, const void *data);

/**
 * @internal
 * @brief Appends a new item to the end of the toolbar.
 * This is the actual implementation for elm_obj_toolbar_item_append().
 * @param obj The toolbar object.
 * @param pd The private data of the toolbar object.
 * @param icon The icon string (name or path).
 * @param label The label of the new item.
 * @param func The callback function for item selection.
 * @param data User data for the callback function.
 * @return The newly created item, or @c NULL on failure.
 */
Elm_Widget_Item *_elm_toolbar_item_append(Eo *obj, Elm_Toolbar_Data *pd, const char *icon, const char *label, Evas_Smart_Cb func, const void *data);

EOAPI EFL_FUNC_BODYV(elm_obj_toolbar_item_append, Elm_Widget_Item *, NULL, EFL_FUNC_CALL(icon, label, func, data), const char *icon, const char *label, Evas_Smart_Cb func, const void *data);

/**
 * @internal
 * @brief Gets the total number of items in the toolbar.
 * This is the actual implementation for elm_obj_toolbar_items_count().
 * @param obj The toolbar object.
 * @param pd The private data of the toolbar object.
 * @return The number of items.
 */
unsigned int _elm_toolbar_items_count(const Eo *obj, Elm_Toolbar_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_toolbar_items_count, unsigned int, 0);

/**
 * @internal
 * @brief Prepends a new item to the beginning of the toolbar.
 * This is the actual implementation for elm_obj_toolbar_item_prepend().
 * @param obj The toolbar object.
 * @param pd The private data of the toolbar object.
 * @param icon The icon string (name or path).
 * @param label The label of the new item.
 * @param func The callback function for item selection.
 * @param data User data for the callback function.
 * @return The newly created item, or @c NULL on failure.
 */
Elm_Widget_Item *_elm_toolbar_item_prepend(Eo *obj, Elm_Toolbar_Data *pd, const char *icon, const char *label, Evas_Smart_Cb func, const void *data);

EOAPI EFL_FUNC_BODYV(elm_obj_toolbar_item_prepend, Elm_Widget_Item *, NULL, EFL_FUNC_CALL(icon, label, func, data), const char *icon, const char *label, Evas_Smart_Cb func, const void *data);

/**
 * @internal
 * @brief Finds a toolbar item by its label.
 * This is the actual implementation for elm_obj_toolbar_item_find_by_label().
 * @param obj The toolbar object.
 * @param pd The private data of the toolbar object.
 * @param label The label of the item to find.
 * @return The found item, or @c NULL if not found.
 */
Elm_Widget_Item *_elm_toolbar_item_find_by_label(const Eo *obj, Elm_Toolbar_Data *pd, const char *label);

EOAPI EFL_FUNC_BODYV_CONST(elm_obj_toolbar_item_find_by_label, Elm_Widget_Item *, NULL, EFL_FUNC_CALL(label), const char *label);

/**
 * @internal
 * @brief Implements the Efl.Object.constructor interface.
 * This function is called when a new toolbar object is constructed.
 * @param obj The toolbar object being constructed.
 * @param pd The private data of the toolbar object.
 * @return The constructed object (typically @p obj itself), or @c NULL on failure.
 */
Efl_Object *_elm_toolbar_efl_object_constructor(Eo *obj, Elm_Toolbar_Data *pd);

/**
 * @internal
 * @brief Implements the Efl.Gfx.Entity.position_set interface.
 * Sets the position of the toolbar.
 * @param obj The toolbar object.
 * @param pd The private data of the toolbar object.
 * @param pos The new position (Eina_Position2D).
 */
void _elm_toolbar_efl_gfx_entity_position_set(Eo *obj, Elm_Toolbar_Data *pd, Eina_Position2D pos);

/**
 * @internal
 * @brief Implements the Efl.Gfx.Entity.size_set interface.
 * Sets the size of the toolbar.
 * @param obj The toolbar object.
 * @param pd The private data of the toolbar object.
 * @param size The new size (Eina_Size2D).
 */
void _elm_toolbar_efl_gfx_entity_size_set(Eo *obj, Elm_Toolbar_Data *pd, Eina_Size2D size);

/**
 * @internal
 * @brief Implements the Efl.Canvas.Group.group_calculate interface.
 * Triggers the calculation of the group's geometry.
 * @param obj The toolbar object (acting as a canvas group).
 * @param pd The private data of the toolbar object.
 */
void _elm_toolbar_efl_canvas_group_group_calculate(Eo *obj, Elm_Toolbar_Data *pd);

/**
 * @internal
 * @brief Implements the Efl.Canvas.Group.member_add interface.
 * Handles adding a sub-object to the toolbar group.
 * @param obj The toolbar object (acting as a canvas group).
 * @param pd The private data of the toolbar object.
 * @param sub_obj The sub-object to add.
 */
void _elm_toolbar_efl_canvas_group_group_member_add(Eo *obj, Elm_Toolbar_Data *pd, Efl_Canvas_Object *sub_obj);

/**
 * @internal
 * @brief Implements the Efl.Ui.Widget.on_access_update interface.
 * Called when accessibility state needs to be updated.
 * @param obj The toolbar widget.
 * @param pd The private data of the toolbar object.
 * @param enable Current accessibility state.
 */
void _elm_toolbar_efl_ui_widget_on_access_update(Eo *obj, Elm_Toolbar_Data *pd, Eina_Bool enable);


Eina_Error _elm_toolbar_efl_ui_widget_theme_apply(Eo *obj, Elm_Toolbar_Data *pd);


Eina_Bool _elm_toolbar_efl_ui_focus_object_on_focus_update(Eo *obj, Elm_Toolbar_Data *pd);


void _elm_toolbar_efl_ui_l10n_translation_update(Eo *obj, Elm_Toolbar_Data *pd);


Eina_Bool _elm_toolbar_efl_ui_widget_widget_input_event_handler(Eo *obj, Elm_Toolbar_Data *pd, const Efl_Event *eo_event, Efl_Canvas_Object *source);


Eina_Rect _elm_toolbar_efl_ui_widget_focus_highlight_geometry_get(const Eo *obj, Elm_Toolbar_Data *pd);


Elm_Widget_Item *_elm_toolbar_elm_widget_item_container_focused_item_get(const Eo *obj, Elm_Toolbar_Data *pd);


void _elm_toolbar_efl_ui_layout_orientable_orientation_set(Eo *obj, Elm_Toolbar_Data *pd, Efl_Ui_Layout_Orientation dir);


Efl_Ui_Layout_Orientation _elm_toolbar_efl_ui_layout_orientable_orientation_get(const Eo *obj, Elm_Toolbar_Data *pd);


Eina_Bool _elm_toolbar_efl_ui_widget_focus_state_apply(Eo *obj, Elm_Toolbar_Data *pd, Efl_Ui_Widget_Focus_State current_state, Efl_Ui_Widget_Focus_State *configured_state, Efl_Ui_Widget *redirect);


const Efl_Access_Action_Data *_elm_toolbar_efl_access_widget_action_elm_actions_get(const Eo *obj, Elm_Toolbar_Data *pd);


Eina_List *_elm_toolbar_efl_access_object_access_children_get(const Eo *obj, Elm_Toolbar_Data *pd);


Efl_Access_State_Set _elm_toolbar_efl_access_object_state_set_get(const Eo *obj, Elm_Toolbar_Data *pd);


int _elm_toolbar_efl_access_selection_selected_children_count_get(const Eo *obj, Elm_Toolbar_Data *pd);


Efl_Object *_elm_toolbar_efl_access_selection_selected_child_get(const Eo *obj, Elm_Toolbar_Data *pd, int selected_child_index);


Eina_Bool _elm_toolbar_efl_access_selection_selected_child_deselect(Eo *obj, Elm_Toolbar_Data *pd, int child_index);


Eina_Bool _elm_toolbar_efl_access_selection_child_select(Eo *obj, Elm_Toolbar_Data *pd, int child_index);


Eina_Bool _elm_toolbar_efl_access_selection_child_deselect(Eo *obj, Elm_Toolbar_Data *pd, int child_index);


Eina_Bool _elm_toolbar_efl_access_selection_is_child_selected(Eo *obj, Elm_Toolbar_Data *pd, int child_index);


Eina_Bool _elm_toolbar_efl_access_selection_all_children_select(Eo *obj, Elm_Toolbar_Data *pd);


Eina_Bool _elm_toolbar_efl_access_selection_access_selection_clear(Eo *obj, Elm_Toolbar_Data *pd);


void _elm_toolbar_efl_ui_focus_composition_prepare(Eo *obj, Elm_Toolbar_Data *pd);


static Eina_Bool
_elm_toolbar_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_TOOLBAR_EXTRA_OPS
#define ELM_TOOLBAR_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(elm_obj_toolbar_selected_item_get, _elm_toolbar_selected_item_get),
      EFL_OBJECT_OP_FUNC(elm_obj_toolbar_first_item_get, _elm_toolbar_first_item_get),
      EFL_OBJECT_OP_FUNC(elm_obj_toolbar_last_item_get, _elm_toolbar_last_item_get),
      EFL_OBJECT_OP_FUNC(elm_obj_toolbar_items_get, _elm_toolbar_items_get),
      EFL_OBJECT_OP_FUNC(elm_obj_toolbar_homogeneous_set, _elm_toolbar_homogeneous_set),
      EFL_OBJECT_OP_FUNC(elm_obj_toolbar_homogeneous_get, _elm_toolbar_homogeneous_get),
      EFL_OBJECT_OP_FUNC(elm_obj_toolbar_align_set, _elm_toolbar_align_set),
      EFL_OBJECT_OP_FUNC(elm_obj_toolbar_align_get, _elm_toolbar_align_get),
      EFL_OBJECT_OP_FUNC(elm_obj_toolbar_select_mode_set, _elm_toolbar_select_mode_set),
      EFL_OBJECT_OP_FUNC(elm_obj_toolbar_select_mode_get, _elm_toolbar_select_mode_get),
      EFL_OBJECT_OP_FUNC(elm_obj_toolbar_icon_size_set, _elm_toolbar_icon_size_set),
      EFL_OBJECT_OP_FUNC(elm_obj_toolbar_icon_size_get, _elm_toolbar_icon_size_get),
      EFL_OBJECT_OP_FUNC(elm_obj_toolbar_shrink_mode_set, _elm_toolbar_shrink_mode_set),
      EFL_OBJECT_OP_FUNC(elm_obj_toolbar_shrink_mode_get, _elm_toolbar_shrink_mode_get),
      EFL_OBJECT_OP_FUNC(elm_obj_toolbar_menu_parent_set, _elm_toolbar_menu_parent_set),
      EFL_OBJECT_OP_FUNC(elm_obj_toolbar_menu_parent_get, _elm_toolbar_menu_parent_get),
      EFL_OBJECT_OP_FUNC(elm_obj_toolbar_standard_priority_set, _elm_toolbar_standard_priority_set),
      EFL_OBJECT_OP_FUNC(elm_obj_toolbar_standard_priority_get, _elm_toolbar_standard_priority_get),
      EFL_OBJECT_OP_FUNC(elm_obj_toolbar_more_item_get, _elm_toolbar_more_item_get),
      EFL_OBJECT_OP_FUNC(elm_obj_toolbar_item_insert_before, _elm_toolbar_item_insert_before),
      EFL_OBJECT_OP_FUNC(elm_obj_toolbar_item_insert_after, _elm_toolbar_item_insert_after),
      EFL_OBJECT_OP_FUNC(elm_obj_toolbar_item_append, _elm_toolbar_item_append),
      EFL_OBJECT_OP_FUNC(elm_obj_toolbar_items_count, _elm_toolbar_items_count),
      EFL_OBJECT_OP_FUNC(elm_obj_toolbar_item_prepend, _elm_toolbar_item_prepend),
      EFL_OBJECT_OP_FUNC(elm_obj_toolbar_item_find_by_label, _elm_toolbar_item_find_by_label),
      EFL_OBJECT_OP_FUNC(efl_constructor, _elm_toolbar_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_gfx_entity_position_set, _elm_toolbar_efl_gfx_entity_position_set),
      EFL_OBJECT_OP_FUNC(efl_gfx_entity_size_set, _elm_toolbar_efl_gfx_entity_size_set),
      EFL_OBJECT_OP_FUNC(efl_canvas_group_calculate, _elm_toolbar_efl_canvas_group_group_calculate),
      EFL_OBJECT_OP_FUNC(efl_canvas_group_member_add, _elm_toolbar_efl_canvas_group_group_member_add),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_on_access_update, _elm_toolbar_efl_ui_widget_on_access_update),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_theme_apply, _elm_toolbar_efl_ui_widget_theme_apply),
      EFL_OBJECT_OP_FUNC(efl_ui_focus_object_on_focus_update, _elm_toolbar_efl_ui_focus_object_on_focus_update),
      EFL_OBJECT_OP_FUNC(efl_ui_l10n_translation_update, _elm_toolbar_efl_ui_l10n_translation_update),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_input_event_handler, _elm_toolbar_efl_ui_widget_widget_input_event_handler),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_focus_highlight_geometry_get, _elm_toolbar_efl_ui_widget_focus_highlight_geometry_get),
      EFL_OBJECT_OP_FUNC(elm_widget_item_container_focused_item_get, _elm_toolbar_elm_widget_item_container_focused_item_get),
      EFL_OBJECT_OP_FUNC(efl_ui_layout_orientation_set, _elm_toolbar_efl_ui_layout_orientable_orientation_set),
      EFL_OBJECT_OP_FUNC(efl_ui_layout_orientation_get, _elm_toolbar_efl_ui_layout_orientable_orientation_get),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_focus_state_apply, _elm_toolbar_efl_ui_widget_focus_state_apply),
      EFL_OBJECT_OP_FUNC(efl_access_widget_action_elm_actions_get, _elm_toolbar_efl_access_widget_action_elm_actions_get),
      EFL_OBJECT_OP_FUNC(efl_access_object_access_children_get, _elm_toolbar_efl_access_object_access_children_get),
      EFL_OBJECT_OP_FUNC(efl_access_object_state_set_get, _elm_toolbar_efl_access_object_state_set_get),
      EFL_OBJECT_OP_FUNC(efl_access_selection_selected_children_count_get, _elm_toolbar_efl_access_selection_selected_children_count_get),
      EFL_OBJECT_OP_FUNC(efl_access_selection_selected_child_get, _elm_toolbar_efl_access_selection_selected_child_get),
      EFL_OBJECT_OP_FUNC(efl_access_selection_selected_child_deselect, _elm_toolbar_efl_access_selection_selected_child_deselect),
      EFL_OBJECT_OP_FUNC(efl_access_selection_child_select, _elm_toolbar_efl_access_selection_child_select),
      EFL_OBJECT_OP_FUNC(efl_access_selection_child_deselect, _elm_toolbar_efl_access_selection_child_deselect),
      EFL_OBJECT_OP_FUNC(efl_access_selection_is_child_selected, _elm_toolbar_efl_access_selection_is_child_selected),
      EFL_OBJECT_OP_FUNC(efl_access_selection_all_children_select, _elm_toolbar_efl_access_selection_all_children_select),
      EFL_OBJECT_OP_FUNC(efl_access_selection_clear, _elm_toolbar_efl_access_selection_access_selection_clear),
      EFL_OBJECT_OP_FUNC(efl_ui_focus_composition_prepare, _elm_toolbar_efl_ui_focus_composition_prepare),
      ELM_TOOLBAR_EXTRA_OPS
   );
   opsp = &ops;

   static const Efl_Object_Property_Reflection refl_table[] = {
      {"homogeneous", __eolian_elm_toolbar_homogeneous_set_reflect, __eolian_elm_toolbar_homogeneous_get_reflect},
      {"align", __eolian_elm_toolbar_align_set_reflect, __eolian_elm_toolbar_align_get_reflect},
      {"icon_size", __eolian_elm_toolbar_icon_size_set_reflect, __eolian_elm_toolbar_icon_size_get_reflect},
      {"standard_priority", __eolian_elm_toolbar_standard_priority_set_reflect, __eolian_elm_toolbar_standard_priority_get_reflect},
   };
   static const Efl_Object_Property_Reflection_Ops rops = {
      refl_table, EINA_C_ARRAY_LENGTH(refl_table)
   };
   ropsp = &rops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

static const Efl_Class_Description _elm_toolbar_class_desc = {
   EO_VERSION,
   "Elm.Toolbar",
   EFL_CLASS_TYPE_REGULAR,
   sizeof(Elm_Toolbar_Data),
   _elm_toolbar_class_initializer,
   _elm_toolbar_class_constructor,
   NULL
};

EFL_DEFINE_CLASS(elm_toolbar_class_get, &_elm_toolbar_class_desc, EFL_UI_WIDGET_CLASS, EFL_UI_FOCUS_COMPOSITION_MIXIN, ELM_INTERFACE_SCROLLABLE_MIXIN, EFL_UI_LAYOUT_ORIENTABLE_INTERFACE, EFL_ACCESS_WIDGET_ACTION_MIXIN, EFL_ACCESS_SELECTION_INTERFACE, EFL_ACCESS_OBJECT_MIXIN, EFL_INPUT_CLICKABLE_MIXIN, EFL_UI_LEGACY_INTERFACE, ELM_WIDGET_ITEM_CONTAINER_INTERFACE, NULL);

#include "elm_toolbar_eo.legacy.c"
