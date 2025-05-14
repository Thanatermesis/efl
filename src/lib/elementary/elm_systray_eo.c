
/**
 * @internal
 * @brief Sets the internal ID for the systray object.
 *
 * This is the actual implementation for setting the systray ID,
 * called by the EO system.
 *
 * @param obj The Evas_Object instance.
 * @param pd Private data for the object.
 * @param id The new ID string.
 */
void _elm_systray_id_set(Eo *obj, void *pd, const char *id);

/**
 * @internal
 * @brief Reflection function for the 'id' property setter.
 *
 * This function is called by the Eolian reflection system to set the 'id'
 * property from an Eina_Value. It converts the Eina_Value to a string
 * and then calls the actual setter implementation.
 *
 * @param obj The Evas_Object instance.
 * @param val The Eina_Value containing the new ID string.
 * @return EINA_ERROR_NO_ERROR on success, or an error code if conversion fails.
 */
static Eina_Error
__eolian_elm_systray_id_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   const char *cval;
   if (!eina_value_string_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_systray_id_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_systray_id_set, EFL_FUNC_CALL(id), const char *id);

/**
 * @internal
 * @brief Gets the internal ID for the systray object.
 *
 * This is the actual implementation for getting the systray ID,
 * called by the EO system.
 *
 * @param obj The Evas_Object instance.
 * @param pd Private data for the object.
 * @return The current ID string.
 */
const char *_elm_systray_id_get(const Eo *obj, void *pd);

/**
 * @internal
 * @brief Reflection function for the 'id' property getter.
 *
 * This function is called by the Eolian reflection system to get the 'id'
 * property as an Eina_Value. It calls the actual getter implementation
 * and then wraps the returned string in an Eina_Value.
 *
 * @param obj The Evas_Object instance.
 * @return An Eina_Value containing the ID string, or a value of error type.
 */
static Eina_Value
__eolian_elm_systray_id_get_reflect(const Eo *obj)
{
   const char *val = elm_obj_systray_id_get(obj);
   return eina_value_string_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_systray_id_get, const char *, NULL);

/**
 * @internal
 * @brief Sets the internal category for the systray object.
 *
 * This is the actual implementation for setting the systray category,
 * called by the EO system.
 *
 * @param obj The Evas_Object instance.
 * @param pd Private data for the object.
 * @param cat The new category.
 */
void _elm_systray_category_set(Eo *obj, void *pd, Elm_Systray_Category cat);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_systray_category_set, EFL_FUNC_CALL(cat), Elm_Systray_Category cat);

/**
 * @internal
 * @brief Gets the internal category for the systray object.
 *
 * This is the actual implementation for getting the systray category,
 * called by the EO system.
 *
 * @param obj The Evas_Object instance.
 * @param pd Private data for the object.
 * @return The current category.
 */
Elm_Systray_Category _elm_systray_category_get(const Eo *obj, void *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_systray_category_get, Elm_Systray_Category, 0);

/**
 * @internal
 * @brief Sets the internal icon theme path for the systray object.
 *
 * This is the actual implementation for setting the systray icon theme path,
 * called by the EO system.
 *
 * @param obj The Evas_Object instance.
 * @param pd Private data for the object.
 * @param icon_theme_path The new icon theme path string.
 */
void _elm_systray_icon_theme_path_set(Eo *obj, void *pd, const char *icon_theme_path);

/**
 * @internal
 * @brief Reflection function for the 'icon_theme_path' property setter.
 *
 * This function is called by the Eolian reflection system to set the
 * 'icon_theme_path' property from an Eina_Value. It converts the Eina_Value
 * to a string and then calls the actual setter implementation.
 *
 * @param obj The Evas_Object instance.
 * @param val The Eina_Value containing the new icon theme path string.
 * @return EINA_ERROR_NO_ERROR on success, or an error code if conversion fails.
 */
static Eina_Error
__eolian_elm_systray_icon_theme_path_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   const char *cval;
   if (!eina_value_string_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_systray_icon_theme_path_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_systray_icon_theme_path_set, EFL_FUNC_CALL(icon_theme_path), const char *icon_theme_path);

/**
 * @internal
 * @brief Gets the internal icon theme path for the systray object.
 *
 * This is the actual implementation for getting the systray icon theme path,
 * called by the EO system.
 *
 * @param obj The Evas_Object instance.
 * @param pd Private data for the object.
 * @return The current icon theme path string.
 */
const char *_elm_systray_icon_theme_path_get(const Eo *obj, void *pd);

/**
 * @internal
 * @brief Reflection function for the 'icon_theme_path' property getter.
 *
 * This function is called by the Eolian reflection system to get the
 * 'icon_theme_path' property as an Eina_Value. It calls the actual getter
 * implementation and then wraps the returned string in an Eina_Value.
 *
 * @param obj The Evas_Object instance.
 * @return An Eina_Value containing the icon theme path string, or a value of error type.
 */
static Eina_Value
__eolian_elm_systray_icon_theme_path_get_reflect(const Eo *obj)
{
   const char *val = elm_obj_systray_icon_theme_path_get(obj);
   return eina_value_string_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_systray_icon_theme_path_get, const char *, NULL);

/**
 * @internal
 * @brief Sets the internal menu object for the systray.
 *
 * This is the actual implementation for setting the systray menu,
 * called by the EO system.
 *
 * @param obj The Evas_Object instance.
 * @param pd Private data for the object.
 * @param menu The new menu object.
 */
void _elm_systray_menu_set(Eo *obj, void *pd, const Efl_Object *menu);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_systray_menu_set, EFL_FUNC_CALL(menu), const Efl_Object *menu);

/**
 * @internal
 * @brief Gets the internal menu object for the systray.
 *
 * This is the actual implementation for getting the systray menu,
 * called by the EO system.
 *
 * @param obj The Evas_Object instance.
 * @param pd Private data for the object.
 * @return The current menu object.
 */
const Efl_Object *_elm_systray_menu_get(const Eo *obj, void *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_systray_menu_get, const Efl_Object *, NULL);

/**
 * @internal
 * @brief Sets the internal attention icon name for the systray object.
 *
 * This is the actual implementation for setting the systray attention icon name,
 * called by the EO system.
 *
 * @param obj The Evas_Object instance.
 * @param pd Private data for the object.
 * @param att_icon_name The new attention icon name string.
 */
void _elm_systray_att_icon_name_set(Eo *obj, void *pd, const char *att_icon_name);

/**
 * @internal
 * @brief Reflection function for the 'att_icon_name' property setter.
 *
 * This function is called by the Eolian reflection system to set the
 * 'att_icon_name' property from an Eina_Value. It converts the Eina_Value
 * to a string and then calls the actual setter implementation.
 *
 * @param obj The Evas_Object instance.
 * @param val The Eina_Value containing the new attention icon name string.
 * @return EINA_ERROR_NO_ERROR on success, or an error code if conversion fails.
 */
static Eina_Error
__eolian_elm_systray_att_icon_name_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   const char *cval;
   if (!eina_value_string_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_systray_att_icon_name_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_systray_att_icon_name_set, EFL_FUNC_CALL(att_icon_name), const char *att_icon_name);

/**
 * @internal
 * @brief Gets the internal attention icon name for the systray object.
 *
 * This is the actual implementation for getting the systray attention icon name,
 * called by the EO system.
 *
 * @param obj The Evas_Object instance.
 * @param pd Private data for the object.
 * @return The current attention icon name string.
 */
const char *_elm_systray_att_icon_name_get(const Eo *obj, void *pd);

/**
 * @internal
 * @brief Reflection function for the 'att_icon_name' property getter.
 *
 * This function is called by the Eolian reflection system to get the
 * 'att_icon_name' property as an Eina_Value. It calls the actual getter
 * implementation and then wraps the returned string in an Eina_Value.
 *
 * @param obj The Evas_Object instance.
 * @return An Eina_Value containing the attention icon name string, or a value of error type.
 */
static Eina_Value
__eolian_elm_systray_att_icon_name_get_reflect(const Eo *obj)
{
   const char *val = elm_obj_systray_att_icon_name_get(obj);
   return eina_value_string_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_systray_att_icon_name_get, const char *, NULL);

/**
 * @internal
 * @brief Sets the internal status for the systray object.
 *
 * This is the actual implementation for setting the systray status,
 * called by the EO system.
 *
 * @param obj The Evas_Object instance.
 * @param pd Private data for the object.
 * @param st The new status.
 */
void _elm_systray_status_set(Eo *obj, void *pd, Elm_Systray_Status st);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_systray_status_set, EFL_FUNC_CALL(st), Elm_Systray_Status st);

/**
 * @internal
 * @brief Gets the internal status for the systray object.
 *
 * This is the actual implementation for getting the systray status,
 * called by the EO system.
 *
 * @param obj The Evas_Object instance.
 * @param pd Private data for the object.
 * @return The current status.
 */
Elm_Systray_Status _elm_systray_status_get(const Eo *obj, void *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_systray_status_get, Elm_Systray_Status, 0);

/**
 * @internal
 * @brief Sets the internal icon name for the systray object.
 *
 * This is the actual implementation for setting the systray icon name,
 * called by the EO system.
 *
 * @param obj The Evas_Object instance.
 * @param pd Private data for the object.
 * @param icon_name The new icon name string.
 */
void _elm_systray_icon_name_set(Eo *obj, void *pd, const char *icon_name);

/**
 * @internal
 * @brief Reflection function for the 'icon_name' property setter.
 *
 * This function is called by the Eolian reflection system to set the
 * 'icon_name' property from an Eina_Value. It converts the Eina_Value
 * to a string and then calls the actual setter implementation.
 *
 * @param obj The Evas_Object instance.
 * @param val The Eina_Value containing the new icon name string.
 * @return EINA_ERROR_NO_ERROR on success, or an error code if conversion fails.
 */
static Eina_Error
__eolian_elm_systray_icon_name_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   const char *cval;
   if (!eina_value_string_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_systray_icon_name_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_systray_icon_name_set, EFL_FUNC_CALL(icon_name), const char *icon_name);

/**
 * @internal
 * @brief Gets the internal icon name for the systray object.
 *
 * This is the actual implementation for getting the systray icon name,
 * called by the EO system.
 *
 * @param obj The Evas_Object instance.
 * @param pd Private data for the object.
 * @return The current icon name string.
 */
const char *_elm_systray_icon_name_get(const Eo *obj, void *pd);

/**
 * @internal
 * @brief Reflection function for the 'icon_name' property getter.
 *
 * This function is called by the Eolian reflection system to get the
 * 'icon_name' property as an Eina_Value. It calls the actual getter
 * implementation and then wraps the returned string in an Eina_Value.
 *
 * @param obj The Evas_Object instance.
 * @return An Eina_Value containing the icon name string, or a value of error type.
 */
static Eina_Value
__eolian_elm_systray_icon_name_get_reflect(const Eo *obj)
{
   const char *val = elm_obj_systray_icon_name_get(obj);
   return eina_value_string_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_systray_icon_name_get, const char *, NULL);

/**
 * @internal
 * @brief Sets the internal title for the systray object.
 *
 * This is the actual implementation for setting the systray title,
 * called by the EO system.
 *
 * @param obj The Evas_Object instance.
 * @param pd Private data for the object.
 * @param title The new title string.
 */
void _elm_systray_title_set(Eo *obj, void *pd, const char *title);

/**
 * @internal
 * @brief Reflection function for the 'title' property setter.
 *
 * This function is called by the Eolian reflection system to set the
 * 'title' property from an Eina_Value. It converts the Eina_Value
 * to a string and then calls the actual setter implementation.
 *
 * @param obj The Evas_Object instance.
 * @param val The Eina_Value containing the new title string.
 * @return EINA_ERROR_NO_ERROR on success, or an error code if conversion fails.
 */
static Eina_Error
__eolian_elm_systray_title_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   const char *cval;
   if (!eina_value_string_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_systray_title_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_systray_title_set, EFL_FUNC_CALL(title), const char *title);

/**
 * @internal
 * @brief Gets the internal title for the systray object.
 *
 * This is the actual implementation for getting the systray title,
 * called by the EO system.
 *
 * @param obj The Evas_Object instance.
 * @param pd Private data for the object.
 * @return The current title string.
 */
const char *_elm_systray_title_get(const Eo *obj, void *pd);

/**
 * @internal
 * @brief Reflection function for the 'title' property getter.
 *
 * This function is called by the Eolian reflection system to get the
 * 'title' property as an Eina_Value. It calls the actual getter
 * implementation and then wraps the returned string in an Eina_Value.
 *
 * @param obj The Evas_Object instance.
 * @return An Eina_Value containing the title string, or a value of error type.
 */
static Eina_Value
__eolian_elm_systray_title_get_reflect(const Eo *obj)
{
   const char *val = elm_obj_systray_title_get(obj);
   return eina_value_string_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_systray_title_get, const char *, NULL);

/**
 * @internal
 * @brief Registers the systray item.
 *
 * This is the actual implementation for registering the systray item,
 * called by the EO system.
 *
 * @param obj The Evas_Object instance.
 * @param pd Private data for the object.
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise.
 */
Eina_Bool _elm_systray_register(Eo *obj, void *pd);

EOAPI EFL_FUNC_BODY(elm_obj_systray_register, Eina_Bool, 0);

/**
 * @internal
 * @brief Initializes the Elm_Systray Efl_Class.
 *
 * This function is called once when the Elm_Systray class is first used.
 * It sets up the Efl_Object operations (methods) and property reflection
 * capabilities for instances of this class.
 *
 * @param klass The Efl_Class to initialize.
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_systray_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_SYSTRAY_EXTRA_OPS
#define ELM_SYSTRAY_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(elm_obj_systray_id_set, _elm_systray_id_set),
      EFL_OBJECT_OP_FUNC(elm_obj_systray_id_get, _elm_systray_id_get),
      EFL_OBJECT_OP_FUNC(elm_obj_systray_category_set, _elm_systray_category_set),
      EFL_OBJECT_OP_FUNC(elm_obj_systray_category_get, _elm_systray_category_get),
      EFL_OBJECT_OP_FUNC(elm_obj_systray_icon_theme_path_set, _elm_systray_icon_theme_path_set),
      EFL_OBJECT_OP_FUNC(elm_obj_systray_icon_theme_path_get, _elm_systray_icon_theme_path_get),
      EFL_OBJECT_OP_FUNC(elm_obj_systray_menu_set, _elm_systray_menu_set),
      EFL_OBJECT_OP_FUNC(elm_obj_systray_menu_get, _elm_systray_menu_get),
      EFL_OBJECT_OP_FUNC(elm_obj_systray_att_icon_name_set, _elm_systray_att_icon_name_set),
      EFL_OBJECT_OP_FUNC(elm_obj_systray_att_icon_name_get, _elm_systray_att_icon_name_get),
      EFL_OBJECT_OP_FUNC(elm_obj_systray_status_set, _elm_systray_status_set),
      EFL_OBJECT_OP_FUNC(elm_obj_systray_status_get, _elm_systray_status_get),
      EFL_OBJECT_OP_FUNC(elm_obj_systray_icon_name_set, _elm_systray_icon_name_set),
      EFL_OBJECT_OP_FUNC(elm_obj_systray_icon_name_get, _elm_systray_icon_name_get),
      EFL_OBJECT_OP_FUNC(elm_obj_systray_title_set, _elm_systray_title_set),
      EFL_OBJECT_OP_FUNC(elm_obj_systray_title_get, _elm_systray_title_get),
      EFL_OBJECT_OP_FUNC(elm_obj_systray_register, _elm_systray_register),
      ELM_SYSTRAY_EXTRA_OPS
   );
   opsp = &ops;

   static const Efl_Object_Property_Reflection refl_table[] = {
      {"id", __eolian_elm_systray_id_set_reflect, __eolian_elm_systray_id_get_reflect},
      {"icon_theme_path", __eolian_elm_systray_icon_theme_path_set_reflect, __eolian_elm_systray_icon_theme_path_get_reflect},
      {"att_icon_name", __eolian_elm_systray_att_icon_name_set_reflect, __eolian_elm_systray_att_icon_name_get_reflect},
      {"icon_name", __eolian_elm_systray_icon_name_set_reflect, __eolian_elm_systray_icon_name_get_reflect},
      {"title", __eolian_elm_systray_title_set_reflect, __eolian_elm_systray_title_get_reflect},
   };
   static const Efl_Object_Property_Reflection_Ops rops = {
      refl_table, EINA_C_ARRAY_LENGTH(refl_table)
   };
   ropsp = &rops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

static const Efl_Class_Description _elm_systray_class_desc = {
   EO_VERSION,
   "Elm.Systray",
   EFL_CLASS_TYPE_REGULAR,
   0,
   _elm_systray_class_initializer,
   NULL,
   NULL
};

EFL_DEFINE_CLASS(elm_systray_class_get, &_elm_systray_class_desc, EFL_OBJECT_CLASS, NULL);

#include "elm_systray_eo.legacy.c"
