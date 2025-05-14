/**
 * @brief Eolian reflection function for the 'folder_only' property set.
 *
 * This function is part of the Eolian reflection mechanism. It is called
 * when the 'folder_only' property is set via reflection (e.g., from scripting
 * languages or other Eolian-aware components). It converts the Eina_Value
 * to the C type and calls the actual C implementation.
 *
 * @param obj The Efl_Object instance.
 * @param val The Eina_Value containing the boolean value to set.
 * @return EINA_ERROR_NO_ERROR on success, or an error code if conversion fails.
 */
static Eina_Error
__eolian_elm_interface_fileselector_folder_only_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   Eina_Bool cval;
   if (!eina_value_bool_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_interface_fileselector_folder_only_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_interface_fileselector_folder_only_set, EFL_FUNC_CALL(only), Eina_Bool only);

/**
 * @brief Eolian reflection function for the 'folder_only' property get.
 *
 * This function is part of the Eolian reflection mechanism. It is called
 * when the 'folder_only' property is accessed via reflection. It calls the
 * C implementation and converts the returned C type to an Eina_Value.
 *
 * @param obj The Efl_Object instance.
 * @return An Eina_Value containing the boolean state of 'folder_only'.
 */
static Eina_Value
__eolian_elm_interface_fileselector_folder_only_get_reflect(const Eo *obj)
{
   Eina_Bool val = elm_interface_fileselector_folder_only_get(obj);
   return eina_value_bool_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_interface_fileselector_folder_only_get, Eina_Bool, 0);
EOAPI EFL_VOID_FUNC_BODYV(elm_interface_fileselector_thumbnail_size_set, EFL_FUNC_CALL(w, h), int w, int h);
EOAPI EFL_VOID_FUNC_BODYV_CONST(elm_interface_fileselector_thumbnail_size_get, EFL_FUNC_CALL(w, h), int *w, int *h);

/**
 * @brief Eolian reflection function for the 'hidden_visible' property set.
 *
 * Part of the Eolian reflection system, this function handles setting the
 * 'hidden_visible' property from reflected calls, converting Eina_Value.
 *
 * @param obj The Efl_Object instance.
 * @param val The Eina_Value containing the boolean value for hidden files visibility.
 * @return EINA_ERROR_NO_ERROR on success, or an error code on failure.
 */
static Eina_Error
__eolian_elm_interface_fileselector_hidden_visible_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   Eina_Bool cval;
   if (!eina_value_bool_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_interface_fileselector_hidden_visible_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_interface_fileselector_hidden_visible_set, EFL_FUNC_CALL(hidden), Eina_Bool hidden);

/**
 * @brief Eolian reflection function for the 'hidden_visible' property get.
 *
 * Part of the Eolian reflection system, this function handles getting the
 * 'hidden_visible' property for reflected calls, returning an Eina_Value.
 *
 * @param obj The Efl_Object instance.
 * @return An Eina_Value containing the boolean state of hidden files visibility.
 */
static Eina_Value
__eolian_elm_interface_fileselector_hidden_visible_get_reflect(const Eo *obj)
{
   Eina_Bool val = elm_interface_fileselector_hidden_visible_get(obj);
   return eina_value_bool_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_interface_fileselector_hidden_visible_get, Eina_Bool, 0);
EOAPI EFL_VOID_FUNC_BODYV(elm_interface_fileselector_sort_method_set, EFL_FUNC_CALL(sort), Elm_Fileselector_Sort sort);
EOAPI EFL_FUNC_BODY_CONST(elm_interface_fileselector_sort_method_get, Elm_Fileselector_Sort, 0);

/**
 * @brief Eolian reflection function for the 'multi_select' property set.
 *
 * Part of the Eolian reflection system, this function handles setting the
 * 'multi_select' property from reflected calls.
 *
 * @param obj The Efl_Object instance.
 * @param val The Eina_Value containing the boolean value for multi-selection.
 * @return EINA_ERROR_NO_ERROR on success, or an error code on failure.
 */
static Eina_Error
__eolian_elm_interface_fileselector_multi_select_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   Eina_Bool cval;
   if (!eina_value_bool_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_interface_fileselector_multi_select_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_interface_fileselector_multi_select_set, EFL_FUNC_CALL(multi), Eina_Bool multi);

/**
 * @brief Eolian reflection function for the 'multi_select' property get.
 *
 * Part of the Eolian reflection system, this function handles getting the
 * 'multi_select' property for reflected calls.
 *
 * @param obj The Efl_Object instance.
 * @return An Eina_Value containing the boolean state of multi-selection.
 */
static Eina_Value
__eolian_elm_interface_fileselector_multi_select_get_reflect(const Eo *obj)
{
   Eina_Bool val = elm_interface_fileselector_multi_select_get(obj);
   return eina_value_bool_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_interface_fileselector_multi_select_get, Eina_Bool, 0);

/**
 * @brief Eolian reflection function for the 'expandable' property set.
 *
 * Part of the Eolian reflection system, this function handles setting the
 * 'expandable' (tree view) property from reflected calls.
 *
 * @param obj The Efl_Object instance.
 * @param val The Eina_Value containing the boolean value for expandability.
 * @return EINA_ERROR_NO_ERROR on success, or an error code on failure.
 */
static Eina_Error
__eolian_elm_interface_fileselector_expandable_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   Eina_Bool cval;
   if (!eina_value_bool_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_interface_fileselector_expandable_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_interface_fileselector_expandable_set, EFL_FUNC_CALL(expand), Eina_Bool expand);

/**
 * @brief Eolian reflection function for the 'expandable' property get.
 *
 * Part of the Eolian reflection system, this function handles getting the
 * 'expandable' (tree view) property for reflected calls.
 *
 * @param obj The Efl_Object instance.
 * @return An Eina_Value containing the boolean state of expandability.
 */
static Eina_Value
__eolian_elm_interface_fileselector_expandable_get_reflect(const Eo *obj)
{
   Eina_Bool val = elm_interface_fileselector_expandable_get(obj);
   return eina_value_bool_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_interface_fileselector_expandable_get, Eina_Bool, 0);
EOAPI EFL_VOID_FUNC_BODYV(elm_interface_fileselector_mode_set, EFL_FUNC_CALL(mode), Elm_Fileselector_Mode mode);
EOAPI EFL_FUNC_BODY_CONST(elm_interface_fileselector_mode_get, Elm_Fileselector_Mode, 0);

/**
 * @brief Eolian reflection function for the 'is_save' property set.
 *
 * Part of the Eolian reflection system, this function handles setting the
 * 'is_save' (save dialog mode) property from reflected calls.
 *
 * @param obj The Efl_Object instance.
 * @param val The Eina_Value containing the boolean value for save mode.
 * @return EINA_ERROR_NO_ERROR on success, or an error code on failure.
 */
static Eina_Error
__eolian_elm_interface_fileselector_is_save_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   Eina_Bool cval;
   if (!eina_value_bool_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_interface_fileselector_is_save_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_interface_fileselector_is_save_set, EFL_FUNC_CALL(is_save), Eina_Bool is_save);

/**
 * @brief Eolian reflection function for the 'is_save' property get.
 *
 * Part of the Eolian reflection system, this function handles getting the
 * 'is_save' (save dialog mode) property for reflected calls.
 *
 * @param obj The Efl_Object instance.
 * @return An Eina_Value containing the boolean state of save mode.
 */
static Eina_Value
__eolian_elm_interface_fileselector_is_save_get_reflect(const Eo *obj)
{
   Eina_Bool val = elm_interface_fileselector_is_save_get(obj);
   return eina_value_bool_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_interface_fileselector_is_save_get, Eina_Bool, 0);
EOAPI EFL_FUNC_BODY_CONST(elm_interface_fileselector_selected_models_get, const Eina_List *, NULL);

/**
 * @brief Eolian reflection function for the 'current_name' property set.
 *
 * Part of the Eolian reflection system, this function handles setting the
 * 'current_name' (filename entry) property from reflected calls.
 *
 * @param obj The Efl_Object instance.
 * @param val The Eina_Value containing the string for the current name.
 * @return EINA_ERROR_NO_ERROR on success, or an error code on failure.
 */
static Eina_Error
__eolian_elm_interface_fileselector_current_name_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   const char *cval;
   if (!eina_value_string_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_interface_fileselector_current_name_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_interface_fileselector_current_name_set, EFL_FUNC_CALL(name), const char *name);

/**
 * @brief Eolian reflection function for the 'current_name' property get.
 *
 * Part of the Eolian reflection system, this function handles getting the
 * 'current_name' (filename entry) property for reflected calls.
 *
 * @param obj The Efl_Object instance.
 * @return An Eina_Value containing the string of the current name.
 */
static Eina_Value
__eolian_elm_interface_fileselector_current_name_get_reflect(const Eo *obj)
{
   const char *val = elm_interface_fileselector_current_name_get(obj);
   return eina_value_string_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_interface_fileselector_current_name_get, const char *, NULL);
EOAPI EFL_FUNC_BODYV(elm_interface_fileselector_selected_model_set, Eina_Bool, 0, EFL_FUNC_CALL(model), Efl_Io_Model *model);
EOAPI EFL_FUNC_BODY_CONST(elm_interface_fileselector_selected_model_get, Efl_Io_Model *, NULL);
EOAPI EFL_FUNC_BODYV(elm_interface_fileselector_custom_filter_append, Eina_Bool, 0, EFL_FUNC_CALL(func, data, filter_name), Elm_Fileselector_Filter_Func func, void *data, const char *filter_name);
EOAPI EFL_VOID_FUNC_BODY(elm_interface_fileselector_filters_clear);
EOAPI EFL_FUNC_BODYV(elm_interface_fileselector_mime_types_filter_append, Eina_Bool, 0, EFL_FUNC_CALL(mime_types, filter_name), const char *mime_types, const char *filter_name);

/**
 * @brief Initializes the Elm_Interface_Fileselector Efl_Class.
 *
 * This function is called once when the Efl_Class for Elm_Interface_Fileselector
 * is being constructed. It sets up the Efl_Object operations (methods)
 * and Eolian property reflection operations for this interface.
 *
 * @param klass The Efl_Class to initialize.
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_interface_fileselector_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_INTERFACE_FILESELECTOR_EXTRA_OPS
#define ELM_INTERFACE_FILESELECTOR_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_folder_only_set, NULL),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_folder_only_get, NULL),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_thumbnail_size_set, NULL),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_thumbnail_size_get, NULL),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_hidden_visible_set, NULL),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_hidden_visible_get, NULL),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_sort_method_set, NULL),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_sort_method_get, NULL),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_multi_select_set, NULL),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_multi_select_get, NULL),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_expandable_set, NULL),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_expandable_get, NULL),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_mode_set, NULL),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_mode_get, NULL),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_is_save_set, NULL),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_is_save_get, NULL),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_selected_models_get, NULL),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_current_name_set, NULL),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_current_name_get, NULL),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_selected_model_set, NULL),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_selected_model_get, NULL),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_custom_filter_append, NULL),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_filters_clear, NULL),
      EFL_OBJECT_OP_FUNC(elm_interface_fileselector_mime_types_filter_append, NULL),
      ELM_INTERFACE_FILESELECTOR_EXTRA_OPS
   );
   opsp = &ops;

   static const Efl_Object_Property_Reflection refl_table[] = {
      {"folder_only", __eolian_elm_interface_fileselector_folder_only_set_reflect, __eolian_elm_interface_fileselector_folder_only_get_reflect},
      {"hidden_visible", __eolian_elm_interface_fileselector_hidden_visible_set_reflect, __eolian_elm_interface_fileselector_hidden_visible_get_reflect},
      {"multi_select", __eolian_elm_interface_fileselector_multi_select_set_reflect, __eolian_elm_interface_fileselector_multi_select_get_reflect},
      {"expandable", __eolian_elm_interface_fileselector_expandable_set_reflect, __eolian_elm_interface_fileselector_expandable_get_reflect},
      {"is_save", __eolian_elm_interface_fileselector_is_save_set_reflect, __eolian_elm_interface_fileselector_is_save_get_reflect},
      {"current_name", __eolian_elm_interface_fileselector_current_name_set_reflect, __eolian_elm_interface_fileselector_current_name_get_reflect},
   };
   static const Efl_Object_Property_Reflection_Ops rops = {
      refl_table, EINA_C_ARRAY_LENGTH(refl_table)
   };
   ropsp = &rops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @brief Describes the Elm_Interface_Fileselector Efl_Class.
 *
 * This static structure provides metadata for the Elm_Interface_Fileselector class,
 * such as its version, name, type (interface), size of its instance data (0 for interfaces),
 * and pointers to its class initializer and constructor/destructor functions (NULL if not custom).
 */
static const Efl_Class_Description _elm_interface_fileselector_class_desc = {
   EO_VERSION, /**< Eolian version for this class. */
   "Elm.Interface.Fileselector", /**< Full name of the class. */
   EFL_CLASS_TYPE_INTERFACE,
   0,
   _elm_interface_fileselector_class_initializer,
   NULL, /**< Class part constructor (usually NULL for interfaces). */
   NULL /**< Class part destructor (usually NULL for interfaces). */
};

/**
 * @brief Defines the Efl_Class for Elm_Interface_Fileselector.
 *
 * This macro call is responsible for the actual creation and registration
 * of the Elm_Interface_Fileselector Efl_Class with the Efl object system.
 * It uses the class description (`_elm_interface_fileselector_class_desc`),
 * specifies its parent class (NULL for pure interfaces or a base interface),
 * and any mixins (EFL_UI_VIEW_INTERFACE in this case).
 * The `elm_interface_fileselector_interface_get` function is generated to retrieve this class.
 */
EFL_DEFINE_CLASS(elm_interface_fileselector_interface_get, &_elm_interface_fileselector_class_desc, NULL, EFL_UI_VIEW_INTERFACE, NULL);
