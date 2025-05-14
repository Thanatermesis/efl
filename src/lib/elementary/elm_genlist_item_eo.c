
/**
 * @internal
 * @brief Implementation for getting the previous item in the genlist.
 *
 * Retrieves the Elm_Widget_Item that precedes the given item (@p obj)
 * in the genlist's internal item sequence. This function serves as the
 * concrete implementation for the @ref elm_obj_genlist_item_prev_get EO API method.
 *
 * @param obj The Elm_Genlist_Item object.
 * @param pd Pointer to the private data (Elm_Gen_Item) of the item.
 * @return The previous Elm_Widget_Item, or @c NULL if it's the first item or on error.
 * @see elm_obj_genlist_item_prev_get
 */
Elm_Widget_Item *_elm_genlist_item_prev_get(const Eo *obj, Elm_Gen_Item *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_genlist_item_prev_get, Elm_Widget_Item *, NULL);

/**
 * @internal
 * @brief Implementation for getting the next item in the genlist.
 *
 * Retrieves the Elm_Widget_Item that follows the given item (@p obj)
 * in the genlist's internal item sequence. This function is the
 * concrete implementation for the @ref elm_obj_genlist_item_next_get EO API method.
 *
 * @param obj The Elm_Genlist_Item object.
 * @param pd Pointer to the private data (Elm_Gen_Item) of the item.
 * @return The next Elm_Widget_Item, or @c NULL if it's the last item or on error.
 * @see elm_obj_genlist_item_next_get
 */
Elm_Widget_Item *_elm_genlist_item_next_get(const Eo *obj, Elm_Gen_Item *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_genlist_item_next_get, Elm_Widget_Item *, NULL);

/**
 * @internal
 * @brief Implementation for getting the parent item of a genlist item.
 *
 * Retrieves the parent Elm_Widget_Item of the specified item (@p obj).
 * This is relevant for tree-like structures within the genlist.
 * This function is the concrete implementation for the
 * @ref elm_obj_genlist_item_parent_item_get EO API method.
 *
 * @param obj The Elm_Genlist_Item object.
 * @param pd Pointer to the private data (Elm_Gen_Item) of the item.
 * @return The parent Elm_Widget_Item, or @c NULL if the item has no parent or on error.
 * @see elm_obj_genlist_item_parent_item_get
 */
Elm_Widget_Item *_elm_genlist_item_parent_item_get(const Eo *obj, Elm_Gen_Item *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_genlist_item_parent_item_get, Elm_Widget_Item *, NULL);

/**
 * @internal
 * @brief Implementation for getting the list of subitems of a genlist item.
 *
 * Retrieves a list of all subitems (children) of the specified genlist item (@p obj).
 * The returned list contains pointers to Elm_Widget_Item objects. This list is
 * owned by the item and should not be modified or freed by the caller.
 * This function is the concrete implementation for the
 * @ref elm_obj_genlist_item_subitems_get EO API method.
 *
 * @param obj The Elm_Genlist_Item object.
 * @param pd Pointer to the private data (Elm_Gen_Item) of the item.
 * @return A const Eina_List of Elm_Widget_Item pointers, or @c NULL on error or if no subitems.
 *         The list contains Elm_Widget_Item* elements. For example:
 *         Eina_List* subitems = elm_obj_genlist_item_subitems_get(item);
 *         Elm_Widget_Item* first_child;
 *         EINA_LIST_FOREACH(subitems, l, first_child) break; // Get first child
 * @see elm_obj_genlist_item_subitems_get
 */
const Eina_List *_elm_genlist_item_subitems_get(const Eo *obj, Elm_Gen_Item *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_genlist_item_subitems_get, const Eina_List *, NULL);

/**
 * @internal
 * @brief Implementation for setting the selected state of a genlist item.
 *
 * Sets whether the specified genlist item (@p obj) is selected.
 * This function is the concrete implementation for the
 * @ref elm_obj_genlist_item_selected_set EO API method.
 *
 * @param obj The Elm_Genlist_Item object.
 * @param pd Pointer to the private data (Elm_Gen_Item) of the item.
 * @param selected @c EINA_TRUE to select the item, @c EINA_FALSE to unselect.
 * @see elm_obj_genlist_item_selected_set
 */
void _elm_genlist_item_selected_set(Eo *obj, Elm_Gen_Item *pd, Eina_Bool selected);

/**
 * @internal
 * @brief Eolian reflection function to set the 'selected' property from an Eina_Value.
 *
 * This function is utilized by the Eolian reflection system to update the item's
 * 'selected' state. It converts an Eina_Value (expected to hold a boolean)
 * into a C boolean type and then calls the concrete @ref elm_obj_genlist_item_selected_set
 * function to apply the change.
 *
 * @param obj The Elm_Genlist_Item object whose 'selected' property is to be set.
 * @param val An Eina_Value containing the new selected state (must be convertible to boolean).
 * @return @c EINA_ERROR_NO_ERROR on successful conversion and setting of the property,
 *         or an Eina_Error code (e.g., @c EINA_ERROR_VALUE_FAILED) if the
 *         Eina_Value cannot be converted to a boolean.
 */
static Eina_Error
__eolian_elm_genlist_item_selected_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   Eina_Bool cval;
   if (!eina_value_bool_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_genlist_item_selected_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_genlist_item_selected_set, EFL_FUNC_CALL(selected), Eina_Bool selected);

/**
 * @internal
 * @brief Implementation for getting the selected state of a genlist item.
 *
 * Retrieves the current selected state of the specified genlist item (@p obj).
 * This function is the concrete implementation for the
 * @ref elm_obj_genlist_item_selected_get EO API method.
 *
 * @param obj The Elm_Genlist_Item object.
 * @param pd Pointer to the private data (Elm_Gen_Item) of the item.
 * @return @c EINA_TRUE if the item is selected, @c EINA_FALSE otherwise.
 * @see elm_obj_genlist_item_selected_get
 */
Eina_Bool _elm_genlist_item_selected_get(const Eo *obj, Elm_Gen_Item *pd);

/**
 * @internal
 * @brief Eolian reflection function to get the 'selected' property as an Eina_Value.
 *
 * This function is utilized by the Eolian reflection system to retrieve the item's
 * current 'selected' state. It calls the concrete @ref elm_obj_genlist_item_selected_get
 * function and then wraps the resulting boolean value into an Eina_Value.
 *
 * @param obj The Elm_Genlist_Item object whose 'selected' property is to be retrieved.
 * @return An Eina_Value initialized with the boolean selected state of the item.
 *         The caller is responsible for flushing this Eina_Value if it's not @c NULL.
 */
static Eina_Value
__eolian_elm_genlist_item_selected_get_reflect(const Eo *obj)
{
   Eina_Bool val = elm_obj_genlist_item_selected_get(obj);
   return eina_value_bool_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_genlist_item_selected_get, Eina_Bool, 0);

/**
 * @internal
 * @brief Implementation for setting the expanded state of a genlist item.
 *
 * Sets whether the specified genlist item (@p obj) is expanded (if it's a tree item).
 * This function is the concrete implementation for the
 * @ref elm_obj_genlist_item_expanded_set EO API method.
 *
 * @param obj The Elm_Genlist_Item object.
 * @param pd Pointer to the private data (Elm_Gen_Item) of the item.
 * @param expanded @c EINA_TRUE to expand the item, @c EINA_FALSE to collapse.
 * @see elm_obj_genlist_item_expanded_set
 */
void _elm_genlist_item_expanded_set(Eo *obj, Elm_Gen_Item *pd, Eina_Bool expanded);

/**
 * @internal
 * @brief Eolian reflection function to set the 'expanded' property from an Eina_Value.
 *
 * Used by the Eolian reflection system to update the item's 'expanded' state.
 * It converts an Eina_Value (expected boolean) and calls
 * @ref elm_obj_genlist_item_expanded_set.
 *
 * @param obj The Elm_Genlist_Item object.
 * @param val An Eina_Value containing the new expanded state (boolean).
 * @return @c EINA_ERROR_NO_ERROR on success, or an error code if conversion fails.
 */
static Eina_Error
__eolian_elm_genlist_item_expanded_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   Eina_Bool cval;
   if (!eina_value_bool_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_genlist_item_expanded_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_genlist_item_expanded_set, EFL_FUNC_CALL(expanded), Eina_Bool expanded);

/**
 * @internal
 * @brief Implementation for getting the expanded state of a genlist item.
 *
 * Retrieves the current expanded state of the specified genlist item (@p obj).
 * This function is the concrete implementation for the
 * @ref elm_obj_genlist_item_expanded_get EO API method.
 *
 * @param obj The Elm_Genlist_Item object.
 * @param pd Pointer to the private data (Elm_Gen_Item) of the item.
 * @return @c EINA_TRUE if the item is expanded, @c EINA_FALSE otherwise.
 * @see elm_obj_genlist_item_expanded_get
 */
Eina_Bool _elm_genlist_item_expanded_get(const Eo *obj, Elm_Gen_Item *pd);

/**
 * @internal
 * @brief Eolian reflection function to get the 'expanded' property as an Eina_Value.
 *
 * Used by the Eolian reflection system to retrieve the item's 'expanded' state.
 * It calls @ref elm_obj_genlist_item_expanded_get and wraps the result in an Eina_Value.
 *
 * @param obj The Elm_Genlist_Item object.
 * @return An Eina_Value initialized with the boolean expanded state.
 */
static Eina_Value
__eolian_elm_genlist_item_expanded_get_reflect(const Eo *obj)
{
   Eina_Bool val = elm_obj_genlist_item_expanded_get(obj);
   return eina_value_bool_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_genlist_item_expanded_get, Eina_Bool, 0);

/**
 * @internal
 * @brief Implementation for getting the expanded depth of a genlist item.
 *
 * Retrieves the depth of an expanded item in a tree structure within the genlist.
 * This function is the concrete implementation for the
 * @ref elm_obj_genlist_item_expanded_depth_get EO API method.
 *
 * @param obj The Elm_Genlist_Item object.
 * @param pd Pointer to the private data (Elm_Gen_Item) of the item.
 * @return The depth of the expanded item (e.g., 0 for root level items that are expanded,
 *         1 for their children if expanded, etc.). Returns a negative value or 0
 *         if not applicable or on error, depending on specific internal logic.
 * @see elm_obj_genlist_item_expanded_depth_get
 */
int _elm_genlist_item_expanded_depth_get(const Eo *obj, Elm_Gen_Item *pd);

/**
 * @internal
 * @brief Eolian reflection function to get the 'expanded_depth' property as an Eina_Value.
 *
 * Used by the Eolian reflection system to retrieve the item's 'expanded_depth'.
 * It calls @ref elm_obj_genlist_item_expanded_depth_get and wraps the integer result
 * in an Eina_Value.
 *
 * @param obj The Elm_Genlist_Item object.
 * @return An Eina_Value initialized with the integer expanded depth.
 */
static Eina_Value
__eolian_elm_genlist_item_expanded_depth_get_reflect(const Eo *obj)
{
   int val = elm_obj_genlist_item_expanded_depth_get(obj);
   return eina_value_int_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_genlist_item_expanded_depth_get, int, 0);

/**
 * @internal
 * @brief Implementation for getting the item class of a genlist item.
 *
 * Retrieves the Elm_Genlist_Item_Class associated with the specified genlist item (@p obj).
 * The item class defines how the item is rendered and behaves.
 * This function is the concrete implementation for the
 * @ref elm_obj_genlist_item_class_get EO API method.
 * (Note: This refers to the EOAPI, not the EWAPI `elm_genlist_item_class_get`.)
 *
 * @param obj The Elm_Genlist_Item object.
 * @param pd Pointer to the private data (Elm_Gen_Item) of the item.
 * @return A pointer to the const Elm_Genlist_Item_Class for this item, or @c NULL on error.
 * @see elm_obj_genlist_item_class_get
 */
const Elm_Genlist_Item_Class *_elm_genlist_item_item_class_get(const Eo *obj, Elm_Gen_Item *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_genlist_item_class_get, const Elm_Genlist_Item_Class *, NULL);

/**
 * @internal
 * @brief Implementation for getting the index of a genlist item.
 *
 * Retrieves the numerical index of the item within the genlist. The index is 1-based.
 * This function is the concrete implementation for the
 * @ref elm_obj_genlist_item_index_get EO API method.
 *
 * @param obj The Elm_Genlist_Item object.
 * @param pd Pointer to the private data (Elm_Gen_Item) of the item.
 * @return The 1-based index of the item, or -1 (or other error indicator like 0)
 *         if the item is not yet realized or on error.
 * @see elm_obj_genlist_item_index_get
 */
int _elm_genlist_item_index_get(const Eo *obj, Elm_Gen_Item *pd);


static Eina_Value
__eolian_elm_genlist_item_index_get_reflect(const Eo *obj)
{
   int val = elm_obj_genlist_item_index_get(obj);
   return eina_value_int_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_genlist_item_index_get, int, -1 /* +1 */);

const char *_elm_genlist_item_decorate_mode_get(const Eo *obj, Elm_Gen_Item *pd);


static Eina_Value
__eolian_elm_genlist_item_decorate_mode_get_reflect(const Eo *obj)
{
   const char *val = elm_obj_genlist_item_decorate_mode_get(obj);
   return eina_value_string_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_genlist_item_decorate_mode_get, const char *, NULL);

void _elm_genlist_item_flip_set(Eo *obj, Elm_Gen_Item *pd, Eina_Bool flip);


static Eina_Error
__eolian_elm_genlist_item_flip_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   Eina_Bool cval;
   if (!eina_value_bool_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_genlist_item_flip_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_genlist_item_flip_set, EFL_FUNC_CALL(flip), Eina_Bool flip);

Eina_Bool _elm_genlist_item_flip_get(const Eo *obj, Elm_Gen_Item *pd);


static Eina_Value
__eolian_elm_genlist_item_flip_get_reflect(const Eo *obj)
{
   Eina_Bool val = elm_obj_genlist_item_flip_get(obj);
   return eina_value_bool_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_genlist_item_flip_get, Eina_Bool, 0);

void _elm_genlist_item_select_mode_set(Eo *obj, Elm_Gen_Item *pd, Elm_Object_Select_Mode mode);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_genlist_item_select_mode_set, EFL_FUNC_CALL(mode), Elm_Object_Select_Mode mode);

Elm_Object_Select_Mode _elm_genlist_item_select_mode_get(const Eo *obj, Elm_Gen_Item *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_genlist_item_select_mode_get, Elm_Object_Select_Mode, 4 /* Elm.Object.Select_Mode.max */);

Elm_Genlist_Item_Type _elm_genlist_item_type_get(const Eo *obj, Elm_Gen_Item *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_genlist_item_type_get, Elm_Genlist_Item_Type, 4 /* Elm.Genlist.Item.Type.max */);

void _elm_genlist_item_pin_set(Eo *obj, Elm_Gen_Item *pd, Eina_Bool pin);


static Eina_Error
__eolian_elm_genlist_item_pin_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   Eina_Bool cval;
   if (!eina_value_bool_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_genlist_item_pin_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_genlist_item_pin_set, EFL_FUNC_CALL(pin), Eina_Bool pin);

Eina_Bool _elm_genlist_item_pin_get(const Eo *obj, Elm_Gen_Item *pd);


static Eina_Value
__eolian_elm_genlist_item_pin_get_reflect(const Eo *obj)
{
   Eina_Bool val = elm_obj_genlist_item_pin_get(obj);
   return eina_value_bool_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_genlist_item_pin_get, Eina_Bool, 0);

unsigned int _elm_genlist_item_subitems_count(Eo *obj, Elm_Gen_Item *pd);

EOAPI EFL_FUNC_BODY(elm_obj_genlist_item_subitems_count, unsigned int, 0);

void _elm_genlist_item_subitems_clear(Eo *obj, Elm_Gen_Item *pd);

EOAPI EFL_VOID_FUNC_BODY(elm_obj_genlist_item_subitems_clear);

void _elm_genlist_item_promote(Eo *obj, Elm_Gen_Item *pd);

EOAPI EFL_VOID_FUNC_BODY(elm_obj_genlist_item_promote);

void _elm_genlist_item_demote(Eo *obj, Elm_Gen_Item *pd);

EOAPI EFL_VOID_FUNC_BODY(elm_obj_genlist_item_demote);

void _elm_genlist_item_show(Eo *obj, Elm_Gen_Item *pd, Elm_Genlist_Item_Scrollto_Type type);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_genlist_item_show, EFL_FUNC_CALL(type), Elm_Genlist_Item_Scrollto_Type type);

void _elm_genlist_item_bring_in(Eo *obj, Elm_Gen_Item *pd, Elm_Genlist_Item_Scrollto_Type type);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_genlist_item_bring_in, EFL_FUNC_CALL(type), Elm_Genlist_Item_Scrollto_Type type);

void _elm_genlist_item_all_contents_unset(Eo *obj, Elm_Gen_Item *pd, Eina_List **l);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_genlist_item_all_contents_unset, EFL_FUNC_CALL(l), Eina_List **l);

void _elm_genlist_item_update(Eo *obj, Elm_Gen_Item *pd);

EOAPI EFL_VOID_FUNC_BODY(elm_obj_genlist_item_update);

void _elm_genlist_item_fields_update(Eo *obj, Elm_Gen_Item *pd, const char *parts, Elm_Genlist_Item_Field_Type itf);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_genlist_item_fields_update, EFL_FUNC_CALL(parts, itf), const char *parts, Elm_Genlist_Item_Field_Type itf);

void _elm_genlist_item_item_class_update(Eo *obj, Elm_Gen_Item *pd, const Elm_Genlist_Item_Class *itc);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_genlist_item_class_update, EFL_FUNC_CALL(itc), const Elm_Genlist_Item_Class *itc);

void _elm_genlist_item_decorate_mode_set(Eo *obj, Elm_Gen_Item *pd, const char *decorate_it_type, Eina_Bool decorate_it_set);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_genlist_item_decorate_mode_set, EFL_FUNC_CALL(decorate_it_type, decorate_it_set), const char *decorate_it_type, Eina_Bool decorate_it_set);

Efl_Object *_elm_genlist_item_efl_object_constructor(Eo *obj, Elm_Gen_Item *pd);


void _elm_genlist_item_elm_widget_item_del_pre(Eo *obj, Elm_Gen_Item *pd);


void _elm_genlist_item_elm_widget_item_disable(Eo *obj, Elm_Gen_Item *pd);


void _elm_genlist_item_elm_widget_item_signal_emit(Eo *obj, Elm_Gen_Item *pd, const char *emission, const char *source);


void _elm_genlist_item_elm_widget_item_style_set(Eo *obj, Elm_Gen_Item *pd, const char *style);


const char *_elm_genlist_item_elm_widget_item_style_get(const Eo *obj, Elm_Gen_Item *pd);


void _elm_genlist_item_elm_widget_item_item_focus_set(Eo *obj, Elm_Gen_Item *pd, Eina_Bool focused);


Eina_Bool _elm_genlist_item_elm_widget_item_item_focus_get(const Eo *obj, Elm_Gen_Item *pd);


const char *_elm_genlist_item_elm_widget_item_part_text_get(const Eo *obj, Elm_Gen_Item *pd, const char *part);


Efl_Canvas_Object *_elm_genlist_item_elm_widget_item_part_content_get(const Eo *obj, Elm_Gen_Item *pd, const char *part);


void _elm_genlist_item_elm_widget_item_tooltip_text_set(Eo *obj, Elm_Gen_Item *pd, const char *text);


void _elm_genlist_item_elm_widget_item_tooltip_style_set(Eo *obj, Elm_Gen_Item *pd, const char *style);


const char *_elm_genlist_item_elm_widget_item_tooltip_style_get(const Eo *obj, Elm_Gen_Item *pd);


Eina_Bool _elm_genlist_item_elm_widget_item_tooltip_window_mode_set(Eo *obj, Elm_Gen_Item *pd, Eina_Bool disable);


Eina_Bool _elm_genlist_item_elm_widget_item_tooltip_window_mode_get(const Eo *obj, Elm_Gen_Item *pd);


void _elm_genlist_item_elm_widget_item_tooltip_content_cb_set(Eo *obj, Elm_Gen_Item *pd, Elm_Tooltip_Item_Content_Cb func, const void *data, Evas_Smart_Cb del_cb);


void _elm_genlist_item_elm_widget_item_tooltip_unset(Eo *obj, Elm_Gen_Item *pd);


void _elm_genlist_item_elm_widget_item_cursor_set(Eo *obj, Elm_Gen_Item *pd, const char *cursor);


void _elm_genlist_item_elm_widget_item_cursor_unset(Eo *obj, Elm_Gen_Item *pd);


const char *_elm_genlist_item_efl_access_object_i18n_name_get(const Eo *obj, Elm_Gen_Item *pd);


Efl_Access_State_Set _elm_genlist_item_efl_access_object_state_set_get(const Eo *obj, Elm_Gen_Item *pd);


void _elm_genlist_item_efl_ui_focus_object_setup_order_non_recursive(Eo *obj, Elm_Gen_Item *pd);


Efl_Ui_Focus_Object *_elm_genlist_item_efl_ui_focus_object_focus_parent_get(const Eo *obj, Elm_Gen_Item *pd);


static Eina_Bool
_elm_genlist_item_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

/**
 * @internal
 * @brief Initializes the Elm_Genlist_Item Efl_Class.
 *
 * This function is called once during the Efl class construction phase.
 * It is responsible for setting up the operations (methods) that instances
 * of Elm_Genlist_Item will support, as well as defining how properties
 * are reflected for introspection and scripting purposes.
 *
 * The EFL_OPS_DEFINE macro is used to associate EO API function names
 * (e.g., elm_obj_genlist_item_prev_get) with their corresponding internal
 * implementation functions (e.g., _elm_genlist_item_prev_get).
 *
 * Similarly, it sets up a table for property reflection, mapping property
 * names (e.g., "selected") to their getter and setter reflection functions
 * (e.g., __eolian_elm_genlist_item_selected_get_reflect,
 * __eolian_elm_genlist_item_selected_set_reflect).
 *
 * @param klass The Efl_Class (Elm_Genlist_Item's class) to initialize.
 * @return @c EINA_TRUE on successful initialization, @c EINA_FALSE otherwise.
 */
#ifndef ELM_GENLIST_ITEM_EXTRA_OPS
#define ELM_GENLIST_ITEM_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(elm_obj_genlist_item_prev_get, _elm_genlist_item_prev_get),
      EFL_OBJECT_OP_FUNC(elm_obj_genlist_item_next_get, _elm_genlist_item_next_get),
      EFL_OBJECT_OP_FUNC(elm_obj_genlist_item_parent_item_get, _elm_genlist_item_parent_item_get),
      EFL_OBJECT_OP_FUNC(elm_obj_genlist_item_subitems_get, _elm_genlist_item_subitems_get),
      EFL_OBJECT_OP_FUNC(elm_obj_genlist_item_selected_set, _elm_genlist_item_selected_set),
      EFL_OBJECT_OP_FUNC(elm_obj_genlist_item_selected_get, _elm_genlist_item_selected_get),
      EFL_OBJECT_OP_FUNC(elm_obj_genlist_item_expanded_set, _elm_genlist_item_expanded_set),
      EFL_OBJECT_OP_FUNC(elm_obj_genlist_item_expanded_get, _elm_genlist_item_expanded_get),
      EFL_OBJECT_OP_FUNC(elm_obj_genlist_item_expanded_depth_get, _elm_genlist_item_expanded_depth_get),
      EFL_OBJECT_OP_FUNC(elm_obj_genlist_item_class_get, _elm_genlist_item_item_class_get),
      EFL_OBJECT_OP_FUNC(elm_obj_genlist_item_index_get, _elm_genlist_item_index_get),
      EFL_OBJECT_OP_FUNC(elm_obj_genlist_item_decorate_mode_get, _elm_genlist_item_decorate_mode_get),
      EFL_OBJECT_OP_FUNC(elm_obj_genlist_item_flip_set, _elm_genlist_item_flip_set),
      EFL_OBJECT_OP_FUNC(elm_obj_genlist_item_flip_get, _elm_genlist_item_flip_get),
      EFL_OBJECT_OP_FUNC(elm_obj_genlist_item_select_mode_set, _elm_genlist_item_select_mode_set),
      EFL_OBJECT_OP_FUNC(elm_obj_genlist_item_select_mode_get, _elm_genlist_item_select_mode_get),
      EFL_OBJECT_OP_FUNC(elm_obj_genlist_item_type_get, _elm_genlist_item_type_get),
      EFL_OBJECT_OP_FUNC(elm_obj_genlist_item_pin_set, _elm_genlist_item_pin_set),
      EFL_OBJECT_OP_FUNC(elm_obj_genlist_item_pin_get, _elm_genlist_item_pin_get),
      EFL_OBJECT_OP_FUNC(elm_obj_genlist_item_subitems_count, _elm_genlist_item_subitems_count),
      EFL_OBJECT_OP_FUNC(elm_obj_genlist_item_subitems_clear, _elm_genlist_item_subitems_clear),
      EFL_OBJECT_OP_FUNC(elm_obj_genlist_item_promote, _elm_genlist_item_promote),
      EFL_OBJECT_OP_FUNC(elm_obj_genlist_item_demote, _elm_genlist_item_demote),
      EFL_OBJECT_OP_FUNC(elm_obj_genlist_item_show, _elm_genlist_item_show),
      EFL_OBJECT_OP_FUNC(elm_obj_genlist_item_bring_in, _elm_genlist_item_bring_in),
      EFL_OBJECT_OP_FUNC(elm_obj_genlist_item_all_contents_unset, _elm_genlist_item_all_contents_unset),
      EFL_OBJECT_OP_FUNC(elm_obj_genlist_item_update, _elm_genlist_item_update),
      EFL_OBJECT_OP_FUNC(elm_obj_genlist_item_fields_update, _elm_genlist_item_fields_update),
      EFL_OBJECT_OP_FUNC(elm_obj_genlist_item_class_update, _elm_genlist_item_item_class_update),
      EFL_OBJECT_OP_FUNC(elm_obj_genlist_item_decorate_mode_set, _elm_genlist_item_decorate_mode_set),
      EFL_OBJECT_OP_FUNC(efl_constructor, _elm_genlist_item_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_del_pre, _elm_genlist_item_elm_widget_item_del_pre),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_disable, _elm_genlist_item_elm_widget_item_disable),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_signal_emit, _elm_genlist_item_elm_widget_item_signal_emit),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_style_set, _elm_genlist_item_elm_widget_item_style_set),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_style_get, _elm_genlist_item_elm_widget_item_style_get),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_focus_set, _elm_genlist_item_elm_widget_item_item_focus_set),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_focus_get, _elm_genlist_item_elm_widget_item_item_focus_get),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_part_text_get, _elm_genlist_item_elm_widget_item_part_text_get),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_part_content_get, _elm_genlist_item_elm_widget_item_part_content_get),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_tooltip_text_set, _elm_genlist_item_elm_widget_item_tooltip_text_set),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_tooltip_style_set, _elm_genlist_item_elm_widget_item_tooltip_style_set),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_tooltip_style_get, _elm_genlist_item_elm_widget_item_tooltip_style_get),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_tooltip_window_mode_set, _elm_genlist_item_elm_widget_item_tooltip_window_mode_set),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_tooltip_window_mode_get, _elm_genlist_item_elm_widget_item_tooltip_window_mode_get),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_tooltip_content_cb_set, _elm_genlist_item_elm_widget_item_tooltip_content_cb_set),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_tooltip_unset, _elm_genlist_item_elm_widget_item_tooltip_unset),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_cursor_set, _elm_genlist_item_elm_widget_item_cursor_set),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_cursor_unset, _elm_genlist_item_elm_widget_item_cursor_unset),
      EFL_OBJECT_OP_FUNC(efl_access_object_i18n_name_get, _elm_genlist_item_efl_access_object_i18n_name_get),
      EFL_OBJECT_OP_FUNC(efl_access_object_state_set_get, _elm_genlist_item_efl_access_object_state_set_get),
      EFL_OBJECT_OP_FUNC(efl_ui_focus_object_setup_order_non_recursive, _elm_genlist_item_efl_ui_focus_object_setup_order_non_recursive),
      EFL_OBJECT_OP_FUNC(efl_ui_focus_object_focus_parent_get, _elm_genlist_item_efl_ui_focus_object_focus_parent_get),
      ELM_GENLIST_ITEM_EXTRA_OPS
   );
   opsp = &ops;

   static const Efl_Object_Property_Reflection refl_table[] = {
      {"selected", __eolian_elm_genlist_item_selected_set_reflect, __eolian_elm_genlist_item_selected_get_reflect},
      {"expanded", __eolian_elm_genlist_item_expanded_set_reflect, __eolian_elm_genlist_item_expanded_get_reflect},
      {"expanded_depth", NULL, __eolian_elm_genlist_item_expanded_depth_get_reflect},
      {"index", NULL, __eolian_elm_genlist_item_index_get_reflect},
      {"decorate_mode", NULL, __eolian_elm_genlist_item_decorate_mode_get_reflect},
      {"flip", __eolian_elm_genlist_item_flip_set_reflect, __eolian_elm_genlist_item_flip_get_reflect},
      {"pin", __eolian_elm_genlist_item_pin_set_reflect, __eolian_elm_genlist_item_pin_get_reflect},
   };
   static const Efl_Object_Property_Reflection_Ops rops = {
      refl_table, EINA_C_ARRAY_LENGTH(refl_table)
   };
   ropsp = &rops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

static const Efl_Class_Description _elm_genlist_item_class_desc = {
   EO_VERSION,
   "Elm.Genlist.Item",
   EFL_CLASS_TYPE_REGULAR,
   sizeof(Elm_Gen_Item),
   _elm_genlist_item_class_initializer,
   NULL,
   NULL
};

EFL_DEFINE_CLASS(elm_genlist_item_class_get, &_elm_genlist_item_class_desc, ELM_WIDGET_ITEM_STATIC_FOCUS_CLASS, EFL_UI_LEGACY_INTERFACE, NULL);

#include "elm_genlist_item_eo.legacy.c"
