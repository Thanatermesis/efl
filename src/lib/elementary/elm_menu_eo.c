/**
 * @internal
 * @brief Event descriptor for the "dismissed" event.
 * @details This constant is the actual Efl_Event_Description for the
 * ELM_MENU_EVENT_DISMISSED event, which is fired when the menu is dismissed.
 */
EWAPI const Efl_Event_Description _ELM_MENU_EVENT_DISMISSED =
   EFL_EVENT_DESCRIPTION("dismissed");
/**
 * @internal
 * @brief Event descriptor for the "elm,action,block_menu" event.
 * @details This constant is the actual Efl_Event_Description for the
 * ELM_MENU_EVENT_ELM_ACTION_BLOCK_MENU event, fired when menu interaction blocking is enabled.
 */
EWAPI const Efl_Event_Description _ELM_MENU_EVENT_ELM_ACTION_BLOCK_MENU =
   EFL_EVENT_DESCRIPTION("elm,action,block_menu");
/**
 * @internal
 * @brief Event descriptor for the "elm,action,unblock_menu" event.
 * @details This constant is the actual Efl_Event_Description for the
 * ELM_MENU_EVENT_ELM_ACTION_UNBLOCK_MENU event, fired when menu interaction blocking is disabled.
 */
EWAPI const Efl_Event_Description _ELM_MENU_EVENT_ELM_ACTION_UNBLOCK_MENU =
   EFL_EVENT_DESCRIPTION("elm,action,unblock_menu");

/**
 * @internal
 * @brief Internal implementation for @ref elm_obj_menu_selected_item_get.
 * @param[in] obj The Evas object (menu widget).
 * @param[in] pd The private data of the menu widget.
 * @return The selected Elm_Widget_Item, or @c NULL if no item is selected.
 */
Elm_Widget_Item *_elm_menu_selected_item_get(const Eo *obj, Elm_Menu_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_menu_selected_item_get, Elm_Widget_Item *, NULL);

/**
 * @internal
 * @brief Internal implementation for @ref elm_obj_menu_first_item_get.
 * @param[in] obj The Evas object (menu widget).
 * @param[in] pd The private data of the menu widget.
 * @return The first Elm_Widget_Item in the menu, or @c NULL if the menu is empty.
 */
Elm_Widget_Item *_elm_menu_first_item_get(const Eo *obj, Elm_Menu_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_menu_first_item_get, Elm_Widget_Item *, NULL);

/**
 * @internal
 * @brief Internal implementation for @ref elm_obj_menu_last_item_get.
 * @param[in] obj The Evas object (menu widget).
 * @param[in] pd The private data of the menu widget.
 * @return The last Elm_Widget_Item in the menu, or @c NULL if the menu is empty.
 */
Elm_Widget_Item *_elm_menu_last_item_get(const Eo *obj, Elm_Menu_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_menu_last_item_get, Elm_Widget_Item *, NULL);

/**
 * @internal
 * @brief Internal implementation for @ref elm_obj_menu_items_get.
 * @param[in] obj The Evas object (menu widget).
 * @param[in] pd The private data of the menu widget.
 * @return A const Eina_List of Elm_Widget_Item objects, or @c NULL if empty.
 *         The list should not be modified by the caller.
 */
const Eina_List *_elm_menu_items_get(const Eo *obj, Elm_Menu_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_menu_items_get, const Eina_List *, NULL);

/**
 * @internal
 * @brief Internal implementation for @ref elm_obj_menu_relative_move.
 * @param[in] obj The Evas object (menu widget).
 * @param[in] pd The private data of the menu widget.
 * @param[in] x The new X coordinate, relative to the parent.
 * @param[in] y The new Y coordinate, relative to the parent.
 */
void _elm_menu_relative_move(Eo *obj, Elm_Menu_Data *pd, int x, int y);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_menu_relative_move, EFL_FUNC_CALL(x, y), int x, int y);

/**
 * @internal
 * @brief Internal implementation for @ref elm_obj_menu_item_add.
 * @param[in] obj The Evas object (menu widget).
 * @param[in] pd The private data of the menu widget.
 * @param[in] parent The parent menu item, or @c NULL to add to the main menu.
 * @param[in] icon The icon string (e.g., "home").
 * @param[in] label The text label for the item.
 * @param[in] func The callback function to execute when the item is selected.
 * @param[in] data User data to pass to the callback function.
 * @return The newly created Elm_Widget_Item, or @c NULL on failure.
 */
Elm_Widget_Item *_elm_menu_item_add(Eo *obj, Elm_Menu_Data *pd, Elm_Widget_Item *parent, const char *icon, const char *label, Evas_Smart_Cb func, const void *data);

EOAPI EFL_FUNC_BODYV(elm_obj_menu_item_add, Elm_Widget_Item *, NULL, EFL_FUNC_CALL(parent, icon, label, func, data), Elm_Widget_Item *parent, const char *icon, const char *label, Evas_Smart_Cb func, const void *data);

/**
 * @internal
 * @brief Internal implementation for @ref elm_obj_menu_open.
 * @param[in] obj The Evas object (menu widget).
 * @param[in] pd The private data of the menu widget.
 */
void _elm_menu_open(Eo *obj, Elm_Menu_Data *pd);

EOAPI EFL_VOID_FUNC_BODY(elm_obj_menu_open);

/**
 * @internal
 * @brief Internal implementation for @ref elm_obj_menu_close.
 * @param[in] obj The Evas object (menu widget).
 * @param[in] pd The private data of the menu widget.
 */
void _elm_menu_close(Eo *obj, Elm_Menu_Data *pd);

EOAPI EFL_VOID_FUNC_BODY(elm_obj_menu_close);

/**
 * @internal
 * @brief Internal implementation for @ref elm_obj_menu_item_separator_add.
 * @param[in] obj The Evas object (menu widget).
 * @param[in] pd The private data of the menu widget.
 * @param[in] parent The parent menu item under which to add the separator, or @c NULL for the main menu.
 * @return The newly created Elm_Widget_Item representing the separator, or @c NULL on failure.
 */
Elm_Widget_Item *_elm_menu_item_separator_add(Eo *obj, Elm_Menu_Data *pd, Elm_Widget_Item *parent);

EOAPI EFL_FUNC_BODYV(elm_obj_menu_item_separator_add, Elm_Widget_Item *, NULL, EFL_FUNC_CALL(parent), Elm_Widget_Item *parent);

/**
 * @internal
 * @brief Implements the EFL object constructor for Elm_Menu.
 * @param[in] obj The Evas object (menu widget) being constructed.
 * @param[in] pd The private data of the menu widget.
 * @return The constructed Efl_Object, which is @c obj.
 */
Efl_Object *_elm_menu_efl_object_constructor(Eo *obj, Elm_Menu_Data *pd);

/**
 * @internal
 * @brief Implements the EFL object destructor for Elm_Menu.
 * @param[in] obj The Evas object (menu widget) being destructed.
 * @param[in] pd The private data of the menu widget.
 */
void _elm_menu_efl_object_destructor(Eo *obj, Elm_Menu_Data *pd);

/**
 * @internal
 * @brief Implements the EFL graphics entity visible_set method for Elm_Menu.
 * @param[in] obj The Evas object (menu widget).
 * @param[in] pd The private data of the menu widget.
 * @param[in] v @c EINA_TRUE to make visible, @c EINA_FALSE to make invisible.
 */
void _elm_menu_efl_gfx_entity_visible_set(Eo *obj, Elm_Menu_Data *pd, Eina_Bool v);


/**
 * @internal
 * @brief Implements the EFL UI widget theme_apply method for Elm_Menu.
 *        This function is called when the theme changes and the widget needs to update its appearance.
 * @param[in] obj The Evas object (menu widget).
 * @param[in] pd The private data of the menu widget.
 * @return EINA_ERROR_NONE on success, or an error code on failure.
 */
Eina_Error _elm_menu_efl_ui_widget_theme_apply(Eo *obj, Elm_Menu_Data *pd);

/**
 * @internal
 * @brief Implements the EFL UI L10N translation_update method for Elm_Menu.
 *        This function is called when the language changes and the widget needs to update its translatable strings.
 * @param[in] obj The Evas object (menu widget).
 * @param[in] pd The private data of the menu widget.
 */
void _elm_menu_efl_ui_l10n_translation_update(Eo *obj, Elm_Menu_Data *pd);

/**
 * @internal
 * @brief Implements the EFL UI widget focus manager create method for Elm_Menu.
 * @param[in] obj The Evas object (menu widget).
 * @param[in] pd The private data of the menu widget.
 * @param[in] root The root focus object for the new manager.
 * @return The newly created Efl_Ui_Focus_Manager, or @c NULL on failure.
 */
Efl_Ui_Focus_Manager *_elm_menu_efl_ui_widget_focus_manager_focus_manager_create(Eo *obj, Elm_Menu_Data *pd, Efl_Ui_Focus_Object *root);


/**
 * @internal
 * @brief Implements the EFL access object access_children_get method for Elm_Menu.
 *        Retrieves the accessible child objects of the menu.
 * @param[in] obj The Evas object (menu widget).
 * @param[in] pd The private data of the menu widget.
 * @return A list of Efl_Access_Object representing the children. The caller owns the list.
 */
Eina_List *_elm_menu_efl_access_object_access_children_get(const Eo *obj, Elm_Menu_Data *pd);

/**
 * @internal
 * @brief Implements the EFL access selection selected_children_count_get method for Elm_Menu.
 *        Gets the number of selected accessible children. For a menu, this is typically 0 or 1.
 * @param[in] obj The Evas object (menu widget).
 * @param[in] pd The private data of the menu widget.
 * @return The count of selected children.
 */
int _elm_menu_efl_access_selection_selected_children_count_get(const Eo *obj, Elm_Menu_Data *pd);

/**
 * @internal
 * @brief Implements the EFL access selection selected_child_get method for Elm_Menu.
 *        Gets the specified selected accessible child.
 * @param[in] obj The Evas object (menu widget).
 * @param[in] pd The private data of the menu widget.
 * @param[in] selected_child_index The index of the selected child to retrieve.
 * @return The Efl_Access_Object for the selected child, or @c NULL if index is out of bounds or no child is selected.
 */
Efl_Object *_elm_menu_efl_access_selection_selected_child_get(const Eo *obj, Elm_Menu_Data *pd, int selected_child_index);

/**
 * @internal
 * @brief Implements the EFL object provider_find method for Elm_Menu.
 *        Finds an object that provides a given class/interface.
 * @param[in] obj The Evas object (menu widget).
 * @param[in] pd The private data of the menu widget.
 * @param[in] klass The class/interface to find.
 * @return The Efl_Object that provides the klass, or @c NULL if not found.
 */
Efl_Object *_elm_menu_efl_object_provider_find(const Eo *obj, Elm_Menu_Data *pd, const Efl_Class *klass);

/**
 * @internal
 * @brief Initializes the Elm_Menu class.
 * @param[in] klass The Efl_Class to initialize.
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise.
 * @details This function sets up the operations (methods) for the Elm_Menu class.
 */
static Eina_Bool
_elm_menu_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_MENU_EXTRA_OPS
#define ELM_MENU_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(elm_obj_menu_selected_item_get, _elm_menu_selected_item_get),
      EFL_OBJECT_OP_FUNC(elm_obj_menu_first_item_get, _elm_menu_first_item_get),
      EFL_OBJECT_OP_FUNC(elm_obj_menu_last_item_get, _elm_menu_last_item_get),
      EFL_OBJECT_OP_FUNC(elm_obj_menu_items_get, _elm_menu_items_get),
      EFL_OBJECT_OP_FUNC(elm_obj_menu_relative_move, _elm_menu_relative_move),
      EFL_OBJECT_OP_FUNC(elm_obj_menu_item_add, _elm_menu_item_add),
      EFL_OBJECT_OP_FUNC(elm_obj_menu_open, _elm_menu_open),
      EFL_OBJECT_OP_FUNC(elm_obj_menu_close, _elm_menu_close),
      EFL_OBJECT_OP_FUNC(elm_obj_menu_item_separator_add, _elm_menu_item_separator_add),
      EFL_OBJECT_OP_FUNC(efl_constructor, _elm_menu_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_destructor, _elm_menu_efl_object_destructor),
      EFL_OBJECT_OP_FUNC(efl_gfx_entity_visible_set, _elm_menu_efl_gfx_entity_visible_set),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_theme_apply, _elm_menu_efl_ui_widget_theme_apply),
      EFL_OBJECT_OP_FUNC(efl_ui_l10n_translation_update, _elm_menu_efl_ui_l10n_translation_update),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_focus_manager_create, _elm_menu_efl_ui_widget_focus_manager_focus_manager_create),
      EFL_OBJECT_OP_FUNC(efl_access_object_access_children_get, _elm_menu_efl_access_object_access_children_get),
      EFL_OBJECT_OP_FUNC(efl_access_selection_selected_children_count_get, _elm_menu_efl_access_selection_selected_children_count_get),
      EFL_OBJECT_OP_FUNC(efl_access_selection_selected_child_get, _elm_menu_efl_access_selection_selected_child_get),
      EFL_OBJECT_OP_FUNC(efl_provider_find, _elm_menu_efl_object_provider_find),
      ELM_MENU_EXTRA_OPS
   );
   opsp = &ops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @internal
 * @brief Describes the Elm_Menu class.
 * @details This structure provides metadata for the Elm_Menu class,
 * including its version, name, type, data size, and initializer/constructor functions.
 */
static const Efl_Class_Description _elm_menu_class_desc = {
   EO_VERSION,
   "Elm.Menu",
   EFL_CLASS_TYPE_REGULAR,
   sizeof(Elm_Menu_Data),
   _elm_menu_class_initializer,
   _elm_menu_class_constructor,
   NULL
};

EFL_DEFINE_CLASS(elm_menu_class_get, &_elm_menu_class_desc, EFL_UI_WIDGET_CLASS, EFL_INPUT_CLICKABLE_MIXIN, EFL_ACCESS_SELECTION_INTERFACE, EFL_UI_WIDGET_FOCUS_MANAGER_MIXIN, EFL_UI_LEGACY_INTERFACE, NULL);

#include "elm_menu_eo.legacy.c"
