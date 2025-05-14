#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Eina.h>
#include "eolian_database.h"

/**
 * @brief Gets the direction of the given function parameter.
 *
 * @param[in] param The function parameter.
 * @return The direction of the parameter (e.g., EOLIAN_PARAMETER_IN, EOLIAN_PARAMETER_OUT, EOLIAN_PARAMETER_INOUT).
 *         Returns EOLIAN_PARAMETER_UNKNOWN if param is NULL.
 */
EOLIAN_API Eolian_Parameter_Direction
eolian_parameter_direction_get(const Eolian_Function_Parameter *param)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(param, EOLIAN_PARAMETER_UNKNOWN);
   return param->param_dir;
}

/**
 * @brief Gets the type of the given function parameter.
 *
 * @param[in] param The function parameter.
 * @return The Eolian_Type of the parameter.
 *         Returns NULL if param is NULL.
 */
EOLIAN_API const Eolian_Type *
eolian_parameter_type_get(const Eolian_Function_Parameter *param)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(param, NULL);
   return param->type;
}

/**
 * @brief Gets the default value of the given function parameter, if any.
 *
 * @param[in] param The function parameter.
 * @return The Eolian_Expression representing the default value of the parameter.
 *         Returns NULL if the parameter has no default value or if param is NULL.
 */
EOLIAN_API const Eolian_Expression *
eolian_parameter_default_value_get(const Eolian_Function_Parameter *param)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(param, NULL);
   return param->value;
}

/**
 * @brief Gets the documentation for the given function parameter.
 *
 * @param[in] param The function parameter.
 * @return The Eolian_Documentation associated with the parameter.
 *         Returns NULL if there is no documentation or if param is NULL.
 */
EOLIAN_API const Eolian_Documentation *
eolian_parameter_documentation_get(const Eolian_Function_Parameter *param)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(param, NULL);
   return param->doc;
}

/**
 * @brief Checks if the given function parameter is optional.
 *
 * A parameter is optional if it has a default value.
 *
 * @param[in] param The function parameter.
 * @return EINA_TRUE if the parameter is optional, EINA_FALSE otherwise.
 *         Returns EINA_FALSE if param is NULL.
 */
EOLIAN_API Eina_Bool
eolian_parameter_is_optional(const Eolian_Function_Parameter *param)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(param, EINA_FALSE);
   return param->optional;
}

/**
 * @brief Checks if the given function parameter has "move" semantics.
 *
 * "Move" semantics indicate that the ownership of the resource pointed to by the
 * parameter is transferred.
 *
 * @param[in] param The function parameter.
 * @return EINA_TRUE if the parameter has "move" semantics, EINA_FALSE otherwise.
 *         Returns EINA_FALSE if param is NULL.
 */
EOLIAN_API Eina_Bool
eolian_parameter_is_move(const Eolian_Function_Parameter *param)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(param, EINA_FALSE);
   return param->move;
}

/**
 * @brief Checks if the given function parameter is passed by reference.
 *
 * @param[in] param The function parameter.
 * @return EINA_TRUE if the parameter is passed by reference, EINA_FALSE otherwise.
 *         Returns EINA_FALSE if param is NULL.
 */
EOLIAN_API Eina_Bool
eolian_parameter_is_by_ref(const Eolian_Function_Parameter *param)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(param, EINA_FALSE);
   return param->by_ref;
}

/**
 * @brief Gets the C language type string for the given function parameter.
 *
 * This function generates a string representation of the C type for the parameter,
 * suitable for use in generated C code.
 *
 * @param[in] param_desc The function parameter description.
 * @param[in] as_return EINA_TRUE if the type is for a return value, EINA_FALSE if for a parameter.
 *                      This affects how some types (like pointers) are represented.
 * @return A new Eina_Stringshare containing the C type string. The caller is
 *         responsible for freeing this stringshare when it's no longer needed
 *         (e.g., using eina_stringshare_del()).
 *         Returns NULL if param_desc is NULL.
 *
 * @note Example: For an integer parameter 'count', this might return "int".
 *       For a string parameter 'name' passed by reference, this might return "const char **".
 *       For an array of Foo objects returned by value, this might return "Eina_Iterator<Foo_Type> *"
 *       (depending on the specific type mapping).
 */
EOLIAN_API Eina_Stringshare *
eolian_parameter_c_type_get(const Eolian_Function_Parameter *param_desc,
                            Eina_Bool as_return)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(param_desc, NULL);
   Eina_Strbuf *buf = eina_strbuf_new();
   database_type_to_str(param_desc->type, buf, NULL,
                        as_return ? EOLIAN_C_TYPE_RETURN : EOLIAN_C_TYPE_PARAM,
                        param_desc->by_ref);
   Eina_Stringshare *ret = eina_stringshare_add(eina_strbuf_string_get(buf));
   eina_strbuf_free(buf);
   return ret;
}
