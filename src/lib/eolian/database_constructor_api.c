#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Eina.h>
#include "eolian_database.h"

/**
 * @brief Get the class of the given constructor.
 *
 * @param[in] ctor The constructor object.
 * @return The class of the constructor, or @c NULL on error.
 */
EOLIAN_API const Eolian_Class *
eolian_constructor_class_get(const Eolian_Constructor *ctor)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(ctor, NULL);
   return ctor->klass;
}

/**
 * @brief Get the function (method) of the given constructor.
 *
 * This function retrieves the underlying Eolian_Function that this
 * constructor represents. The constructor's name is typically
 * derived from the class name and the function name (e.g.,
 * "Class.constructor_name"). This function extracts the
 * "constructor_name" part to find the corresponding function.
 *
 * @param[in] ctor The constructor object.
 * @return The function of the constructor, or @c NULL on error or if not found.
 */
EOLIAN_API const Eolian_Function *
eolian_constructor_function_get(const Eolian_Constructor *ctor)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(ctor, NULL);
   const Eolian_Class *klass = eolian_constructor_class_get(ctor);
   if (!klass)
     return NULL;
   return eolian_class_function_by_name_get(klass,
       ctor->base.name + strlen(klass->base.name) + 1, EOLIAN_UNRESOLVED);
}

/**
 * @brief Check if the given constructor is optional.
 *
 * An optional constructor is one that might not be available at runtime,
 * even if the class itself is available.
 *
 * @param[in] ctor The constructor object.
 * @return @ref EINA_TRUE if the constructor is optional, @ref EINA_FALSE otherwise (including on error).
 */
EOLIAN_API Eina_Bool
eolian_constructor_is_optional(const Eolian_Constructor *ctor)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(ctor, EINA_FALSE);
   return ctor->is_optional;
}
