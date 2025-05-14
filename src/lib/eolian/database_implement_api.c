#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Eina.h>
#include "eolian_database.h"

/**
 * @brief Gets the class that owns the implement.
 *
 * @param[in] impl The implement object.
 * @return The class that owns the implement, or @c NULL on error.
 */
EOLIAN_API const Eolian_Class *
eolian_implement_class_get(const Eolian_Implement *impl)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(impl, NULL);
   return impl->klass;
}

/**
 * @brief Gets the class that implements the function.
 *
 * This is the class where the actual implementation of the function resides.
 * It can be the same as eolian_implement_class_get() or one of its ancestors.
 *
 * @param[in] impl The implement object.
 * @return The class that implements the function, or @c NULL on error.
 */
EOLIAN_API const Eolian_Class *
eolian_implement_implementing_class_get(const Eolian_Implement *impl)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(impl, NULL);
   return impl->implklass;
}

/**
 * @brief Gets the function associated with the implement.
 *
 * This function retrieves the Eolian_Function object that this implement
 * refers to. It can also optionally return the specific function type
 * (e.g., getter, setter) if the implement refers to a property.
 *
 * @param[in] impl The implement object.
 * @param[out] func_type A pointer to store the function type. If the implement
 *                       is for a property, this will be set to
 *                       @ref EOLIAN_PROPERTY, @ref EOLIAN_PROP_GET, or
 *                       @ref EOLIAN_PROP_SET. Otherwise, it will be set to the
 *                       function's inherent type (e.g. @ref EOLIAN_METHOD).
 *                       This parameter can be @c NULL if the type is not needed.
 * @return The function object, or @c NULL on error or if the implement
 *         does not have an associated function (which is normally unreachable).
 */
EOLIAN_API const Eolian_Function *
eolian_implement_function_get(const Eolian_Implement *impl,
                              Eolian_Function_Type   *func_type)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(impl, NULL);

   if (!impl->foo_id)
     return NULL; /* normally unreachable */

   if (!func_type)
     return impl->foo_id;

   if (impl->is_prop_get && impl->is_prop_set)
     *func_type = EOLIAN_PROPERTY;
   else if (impl->is_prop_get)
     *func_type = EOLIAN_PROP_GET;
   else if (impl->is_prop_set)
     *func_type = EOLIAN_PROP_SET;
   else
     *func_type = eolian_function_type_get(impl->foo_id);

   return impl->foo_id;
}

/**
 * @brief Gets the documentation for a specific part of an implement.
 *
 * Implements can have different documentation for their getter, setter,
 * or common parts. This function retrieves the appropriate documentation
 * based on the specified function type.
 *
 * @param[in] impl The implement object.
 * @param[in] ftype The function type for which to get documentation.
 *                  Valid values are @ref EOLIAN_PROP_GET, @ref EOLIAN_PROP_SET,
 *                  or any other value (typically @ref EOLIAN_METHOD or the
 *                  function's specific type) for common documentation.
 * @return The documentation object, or @c NULL if not found or on error.
 */
EOLIAN_API const Eolian_Documentation *
eolian_implement_documentation_get(const Eolian_Implement *impl,
                                   Eolian_Function_Type ftype)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(impl, NULL);
   switch (ftype)
     {
      case EOLIAN_PROP_GET: return impl->get_doc; break;
      case EOLIAN_PROP_SET: return impl->set_doc; break;
      default: return impl->common_doc;
     }
}

/**
 * @brief Checks if the implement part is an auto-generated function.
 *
 * For properties, this checks if the getter or setter is auto-generated.
 * For methods, it checks if the method itself is auto-generated.
 *
 * @param[in] impl The implement object.
 * @param[in] ftype The function type to check (@ref EOLIAN_METHOD,
 *                  @ref EOLIAN_PROP_GET, or @ref EOLIAN_PROP_SET).
 * @return @c EINA_TRUE if the specified part is auto-generated,
 *         @c EINA_FALSE otherwise or on error.
 *         Returns @c EINA_FALSE if ftype is @ref EOLIAN_UNRESOLVED or @ref EOLIAN_PROPERTY.
 */
EOLIAN_API Eina_Bool
eolian_implement_is_auto(const Eolian_Implement *impl, Eolian_Function_Type ftype)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(impl, EINA_FALSE);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(ftype != EOLIAN_UNRESOLVED, EINA_FALSE);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(ftype != EOLIAN_PROPERTY, EINA_FALSE);
   switch (ftype)
     {
      case EOLIAN_METHOD:
        return impl->get_auto && !impl->is_prop_get && !impl->is_prop_set;
      case EOLIAN_PROP_GET:
        return impl->get_auto && impl->is_prop_get;
      case EOLIAN_PROP_SET:
        return impl->set_auto && impl->is_prop_set;
      default:
        return EINA_FALSE;
     }
}

/**
 * @brief Checks if the implement part is an empty function.
 *
 * An empty function is one that is explicitly defined as doing nothing.
 * For properties, this checks if the getter or setter is empty.
 * For methods, it checks if the method itself is empty.
 *
 * @param[in] impl The implement object.
 * @param[in] ftype The function type to check (@ref EOLIAN_METHOD,
 *                  @ref EOLIAN_PROP_GET, or @ref EOLIAN_PROP_SET).
 * @return @c EINA_TRUE if the specified part is empty,
 *         @c EINA_FALSE otherwise or on error.
 *         Returns @c EINA_FALSE if ftype is @ref EOLIAN_UNRESOLVED or @ref EOLIAN_PROPERTY.
 */
EOLIAN_API Eina_Bool
eolian_implement_is_empty(const Eolian_Implement *impl, Eolian_Function_Type ftype)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(impl, EINA_FALSE);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(ftype != EOLIAN_UNRESOLVED, EINA_FALSE);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(ftype != EOLIAN_PROPERTY, EINA_FALSE);
   switch (ftype)
     {
      case EOLIAN_METHOD:
        return impl->get_empty && !impl->is_prop_get && !impl->is_prop_set;
      case EOLIAN_PROP_GET:
        return impl->get_empty && impl->is_prop_get;
      case EOLIAN_PROP_SET:
        return impl->set_empty && impl->is_prop_set;
      default:
        return EINA_FALSE;
     }
}

/**
 * @brief Checks if the implement part is a pure virtual function.
 *
 * A pure virtual function must be implemented by a subclass.
 * For properties, this checks if the getter or setter is pure virtual.
 * For methods, it checks if the method itself is pure virtual.
 *
 * @param[in] impl The implement object.
 * @param[in] ftype The function type to check (@ref EOLIAN_METHOD,
 *                  @ref EOLIAN_PROP_GET, or @ref EOLIAN_PROP_SET).
 * @return @c EINA_TRUE if the specified part is pure virtual,
 *         @c EINA_FALSE otherwise or on error.
 *         Returns @c EINA_FALSE if ftype is @ref EOLIAN_UNRESOLVED or @ref EOLIAN_PROPERTY.
 */
EOLIAN_API Eina_Bool
eolian_implement_is_pure_virtual(const Eolian_Implement *impl, Eolian_Function_Type ftype)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(impl, EINA_FALSE);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(ftype != EOLIAN_UNRESOLVED, EINA_FALSE);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(ftype != EOLIAN_PROPERTY, EINA_FALSE);
   switch (ftype)
     {
      case EOLIAN_METHOD:
        return impl->get_pure_virtual && !impl->is_prop_get && !impl->is_prop_set;
      case EOLIAN_PROP_GET:
        return impl->get_pure_virtual && impl->is_prop_get;
      case EOLIAN_PROP_SET:
        return impl->set_pure_virtual && impl->is_prop_set;
      default:
        return EINA_FALSE;
     }
}

/**
 * @brief Checks if the implement refers to a property getter.
 *
 * @param[in] impl The implement object.
 * @return @c EINA_TRUE if the implement is for a property getter,
 *         @c EINA_FALSE otherwise or on error.
 */
EOLIAN_API Eina_Bool
eolian_implement_is_prop_get(const Eolian_Implement *impl)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(impl, EINA_FALSE);
   return impl->is_prop_get;
}

/**
 * @brief Checks if the implement refers to a property setter.
 *
 * @param[in] impl The implement object.
 * @return @c EINA_TRUE if the implement is for a property setter,
 *         @c EINA_FALSE otherwise or on error.
 */
EOLIAN_API Eina_Bool
eolian_implement_is_prop_set(const Eolian_Implement *impl)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(impl, EINA_FALSE);
   return impl->is_prop_set;
}
