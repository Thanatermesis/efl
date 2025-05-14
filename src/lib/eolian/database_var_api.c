#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Eina.h>
#include "eolian_database.h"

/**
 * @brief Get the documentation for a given constant.
 *
 * @param[in] var The constant object.
 *
 * @return The documentation of the constant, or @c NULL on failure.
 */
EOLIAN_API const Eolian_Documentation *
eolian_constant_documentation_get(const Eolian_Constant *var)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(var, NULL);
   return var->doc;
}

/**
 * @brief Get the base type of a given constant.
 *
 * @param[in] var The constant object.
 *
 * @return The base type of the constant, or @c NULL on failure.
 */
EOLIAN_API const Eolian_Type *
eolian_constant_type_get(const Eolian_Constant *var)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(var, NULL);
   return var->base_type;
}

/**
 * @brief Get the value of a given constant.
 *
 * The value is represented as an Eolian_Expression.
 *
 * @param[in] var The constant object.
 *
 * @return The value of the constant, or @c NULL on failure.
 */
EOLIAN_API const Eolian_Expression *
eolian_constant_value_get(const Eolian_Constant *var)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(var, NULL);
   return var->value;
}

/**
 * @brief Check if a given constant is declared as extern.
 *
 * @param[in] var The constant object.
 *
 * @return @c EINA_TRUE if the constant is extern, @c EINA_FALSE otherwise (including on failure).
 */
EOLIAN_API Eina_Bool
eolian_constant_is_extern(const Eolian_Constant *var)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(var, EINA_FALSE);
   return var->is_extern;
}
