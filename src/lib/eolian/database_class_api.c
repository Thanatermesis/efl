#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Eina.h>
#include "eolian_database.h"
#include "eolian_priv.h"

/**
 * @brief Get the type of an Eolian class.
 *
 * @param[in] cl The Eolian class.
 * @return The type of the class, or EOLIAN_CLASS_UNKNOWN_TYPE on error.
 */
EOLIAN_API Eolian_Class_Type
eolian_class_type_get(const Eolian_Class *cl)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(cl, EOLIAN_CLASS_UNKNOWN_TYPE);
   return cl->type;
}

/**
 * @brief Get the documentation for an Eolian class.
 *
 * @param[in] cl The Eolian class.
 * @return A pointer to the Eolian_Documentation object, or NULL on error or if no documentation exists.
 */
EOLIAN_API const Eolian_Documentation *
eolian_class_documentation_get(const Eolian_Class *cl)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(cl, NULL);
   return cl->doc;
}

/**
 * @brief Get the C prefix for an Eolian class.
 *
 * This prefix is typically used for generating C function names related to the class.
 *
 * @param[in] cl The Eolian class.
 * @return The C prefix string, or NULL on error.
 */
EOLIAN_API const char *
eolian_class_c_prefix_get(const Eolian_Class *cl)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(cl, NULL);
   return cl->c_prefix;
}

/**
 * @brief Get the C event prefix for an Eolian class.
 *
 * This prefix is used for generating C event names related to the class.
 *
 * @param[in] cl The Eolian class.
 * @return The C event prefix string, or NULL on error.
 */
EOLIAN_API const char *
eolian_class_event_c_prefix_get(const Eolian_Class *cl)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(cl, NULL);
   return cl->ev_prefix;
}

/**
 * @brief Get the data type associated with an Eolian class.
 *
 * This is often used for the type of private data structures.
 *
 * @param[in] cl The Eolian class.
 * @return The data type string, or NULL on error or if not defined.
 */
EOLIAN_API const char *
eolian_class_data_type_get(const Eolian_Class *cl)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(cl, NULL);
   return cl->data_type;
}

/**
 * @brief Get the parent class of an Eolian class.
 *
 * @param[in] cl The Eolian class.
 * @return A pointer to the parent Eolian_Class object, or NULL if it has no parent or on error.
 */
EOLIAN_API const Eolian_Class *
eolian_class_parent_get(const Eolian_Class *cl)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(cl, NULL);
  return cl->parent;
}

/**
 * @brief Get an iterator over the classes required by an Eolian class.
 *
 * 'Requires' typically means that the class needs another class to be present
 * for its functionality, but doesn't inherit from it.
 * The iterator will yield Eolian_Class pointers.
 *
 * @param[in] cl The Eolian class.
 * @return An Eina_Iterator over the required classes, or NULL if cl is NULL.
 *         The iterator should be freed by the caller using eina_iterator_free().
 */
EOLIAN_API Eina_Iterator *
eolian_class_requires_get(const Eolian_Class *cl)
{
   // EINA_SAFETY_ON_NULL_RETURN_VAL is not used here as eina_list_iterator_new handles NULL input.
   return eina_list_iterator_new(cl->requires);
}

/**
 * @brief Get an iterator over the classes extended by an Eolian class.
 *
 * 'Extends' usually refers to inheritance.
 * The iterator will yield Eolian_Class pointers.
 *
 * @param[in] cl The Eolian class.
 * @return An Eina_Iterator over the extended classes, or NULL on error or if there are no extensions.
 *         The iterator should be freed by the caller using eina_iterator_free().
 */
EOLIAN_API Eina_Iterator *
eolian_class_extensions_get(const Eolian_Class *cl)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(cl, NULL);
   return (cl->extends ? eina_list_iterator_new(cl->extends) : NULL);
}

/**
 * @brief Get an iterator over the interfaces implemented by an Eolian class.
 *
 * The iterator will yield Eolian_Class pointers (representing interfaces).
 *
 * @param[in] cl The Eolian class.
 * @return An Eina_Iterator over the implemented interfaces, or NULL on error or if none are implemented.
 *         The iterator should be freed by the caller using eina_iterator_free().
 */
EOLIAN_API Eina_Iterator *
eolian_class_implements_get(const Eolian_Class *cl)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(cl, NULL);
   return (cl->implements ? eina_list_iterator_new(cl->implements) : NULL);
}

/**
 * @brief Get an iterator over the constructors of an Eolian class.
 *
 * The iterator will yield Eolian_Function pointers, where each function
 * represents a constructor.
 *
 * @param[in] cl The Eolian class.
 * @return An Eina_Iterator over the constructors, or NULL on error or if there are no constructors.
 *         The iterator should be freed by the caller using eina_iterator_free().
 */
EOLIAN_API Eina_Iterator *
eolian_class_constructors_get(const Eolian_Class *cl)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(cl, NULL);
   return (cl->constructors ? eina_list_iterator_new(cl->constructors) : NULL);
}

/**
 * @brief Get a function (method or property) of an Eolian class by its name and type.
 *
 * @param[in] cl The Eolian class.
 * @param[in] func_name The name of the function to find.
 * @param[in] f_type The type of the function (e.g., EOLIAN_METHOD, EOLIAN_PROPERTY, EOLIAN_PROP_GET, EOLIAN_PROP_SET, or EOLIAN_UNRESOLVED to search all).
 * @return A pointer to the Eolian_Function object if found, otherwise NULL.
 */
EOLIAN_API const Eolian_Function *
eolian_class_function_by_name_get(const Eolian_Class *cl, const char *func_name, Eolian_Function_Type f_type)
{
   Eina_List *itr;
   Eolian_Function *fid;
   if (!cl) return NULL;

   if (f_type == EOLIAN_UNRESOLVED || f_type == EOLIAN_METHOD)
      EINA_LIST_FOREACH(cl->methods, itr, fid)
        {
           if (!strcmp(fid->base.name, func_name))
              return fid;
        }

   if (f_type == EOLIAN_UNRESOLVED || f_type == EOLIAN_PROPERTY ||
         f_type == EOLIAN_PROP_SET || f_type == EOLIAN_PROP_GET)
     {
        EINA_LIST_FOREACH(cl->properties, itr, fid)
          {
             if (!database_function_is_type(fid, f_type))
               continue;
             if (!strcmp(fid->base.name, func_name))
                return fid;
          }
     }

   eolian_state_log(cl->base.unit->state, "function '%s' not found in class '%s'",
                    func_name, eolian_object_short_name_get(&cl->base));
   return NULL;
}

/**
 * @brief Get an iterator over the functions (properties or methods) of an Eolian class, filtered by type.
 *
 * The iterator will yield Eolian_Function pointers.
 *
 * @param[in] cl The Eolian class.
 * @param[in] foo_type The type of functions to retrieve (EOLIAN_PROPERTY or EOLIAN_METHOD).
 * @return An Eina_Iterator over the functions of the specified type, or NULL on error or if the type is invalid or no functions of that type exist.
 *         The iterator should be freed by the caller using eina_iterator_free().
 */
EOLIAN_API Eina_Iterator *
eolian_class_functions_get(const Eolian_Class *cl, Eolian_Function_Type foo_type)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(cl, NULL);
   switch (foo_type)
     {
      case EOLIAN_PROPERTY:
         return (cl->properties ? eina_list_iterator_new(cl->properties) : NULL);
      case EOLIAN_METHOD:
         return (cl->methods ? eina_list_iterator_new(cl->methods) : NULL);
      default: return NULL;
     }
}

/**
 * @brief Get an iterator over the events of an Eolian class.
 *
 * The iterator will yield Eolian_Event pointers.
 *
 * @param[in] cl The Eolian class.
 * @return An Eina_Iterator over the events, or NULL on error or if there are no events.
 *         The iterator should be freed by the caller using eina_iterator_free().
 */
EOLIAN_API Eina_Iterator *
eolian_class_events_get(const Eolian_Class *cl)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(cl, NULL);
   return (cl->events ? eina_list_iterator_new(cl->events) : NULL);
}

/**
 * @brief Get an iterator over the parts of an Eolian class.
 *
 * 'Parts' are typically sub-objects or components of a class.
 * The iterator will yield Eolian_Part pointers.
 *
 * @param[in] cl The Eolian class.
 * @return An Eina_Iterator over the parts, or NULL on error or if there are no parts.
 *         The iterator should be freed by the caller using eina_iterator_free().
 */
EOLIAN_API Eina_Iterator *
eolian_class_parts_get(const Eolian_Class *cl)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(cl, NULL);
   return (cl->parts ? eina_list_iterator_new(cl->parts) : NULL);
}

/**
 * @brief Check if the class constructor is enabled for an Eolian class.
 *
 * This indicates whether a default constructor should be generated or is available.
 *
 * @param[in] cl The Eolian class.
 * @return EINA_TRUE if the class constructor is enabled, EINA_FALSE otherwise or on error.
 */
EOLIAN_API Eina_Bool
eolian_class_ctor_enable_get(const Eolian_Class *cl)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(cl, EINA_FALSE);
   return cl->class_ctor_enable;
}

/**
 * @brief Check if the class destructor is enabled for an Eolian class.
 *
 * This indicates whether a default destructor should be generated or is available.
 *
 * @param[in] cl The Eolian class.
 * @return EINA_TRUE if the class destructor is enabled, EINA_FALSE otherwise or on error.
 */
EOLIAN_API Eina_Bool
eolian_class_dtor_enable_get(const Eolian_Class *cl)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(cl, EINA_FALSE);
   return cl->class_dtor_enable;
}

/**
 * @brief Get the C name for the "get" function of an Eolian class.
 *
 * This function typically returns an instance of the class, interface, or mixin.
 * The name is generated based on the class name and type (e.g., "my_class_class_get",
 * "my_interface_interface_get", "my_mixin_mixin_get").
 * The returned string is stringshared and should be freed with eina_stringshare_del()
 * when no longer needed.
 *
 * @param[in] cl The Eolian class.
 * @return The C "get" function name as an Eina_Stringshare, or NULL on error.
 */
EOLIAN_API Eina_Stringshare *
eolian_class_c_get_function_name_get(const Eolian_Class *cl)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(cl, NULL);
   Eina_Stringshare *ret;
   Eina_Strbuf *buf = eina_strbuf_new();
   char *bufp;
   eina_strbuf_append(buf, cl->base.c_name);
   switch (cl->type)
     {
      case EOLIAN_CLASS_INTERFACE:
        eina_strbuf_append(buf, "_interface_get");
        break;
      case EOLIAN_CLASS_MIXIN:
        eina_strbuf_append(buf, "_mixin_get");
        break;
      default:
        eina_strbuf_append(buf, "_class_get");
        break;
     }
   bufp = eina_strbuf_string_steal(buf);
   eina_str_tolower(&bufp);
   ret = eina_stringshare_add(bufp);
   free(bufp);
   eina_strbuf_free(buf);
   return ret;
}

/**
 * @brief Get the C macro name associated with an Eolian class.
 *
 * This macro is often used to refer to the class type or related definitions.
 * The name is generated based on the class name and type, and converted to uppercase
 * (e.g., "MY_CLASS_CLASS", "MY_INTERFACE_INTERFACE", "MY_MIXIN_MIXIN").
 * The returned string is stringshared and should be freed with eina_stringshare_del()
 * when no longer needed.
 *
 * @param[in] cl The Eolian class.
 * @return The C macro name as an Eina_Stringshare, or NULL on error.
 */
EOLIAN_API Eina_Stringshare *
eolian_class_c_macro_get(const Eolian_Class *cl)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(cl, NULL);
   Eina_Stringshare *ret;
   Eina_Strbuf *buf = eina_strbuf_new();
   char *bufp;
   eina_strbuf_append(buf, cl->base.c_name);
   switch (cl->type)
     {
      case EOLIAN_CLASS_INTERFACE:
        eina_strbuf_append(buf, "_INTERFACE");
        break;
      case EOLIAN_CLASS_MIXIN:
        eina_strbuf_append(buf, "_MIXIN");
        break;
      default:
        eina_strbuf_append(buf, "_CLASS");
        break;
     }
   bufp = eina_strbuf_string_steal(buf);
   eina_str_toupper(&bufp);
   ret = eina_stringshare_add(bufp);
   free(bufp);
   eina_strbuf_free(buf);
   return ret;
}

/**
 * @brief Get the C data type name for an Eolian class.
 *
 * This function determines the C data type string for the class.
 * - If `cl->data_type` is NULL, it defaults to `ClassName_Data`.
 * - If `cl->data_type` is "null", it returns "void".
 * - Otherwise, it uses `cl->data_type`, replacing '.' with '_'.
 * The returned string is stringshared and should be freed with eina_stringshare_del()
 * when no longer needed.
 *
 * @param[in] cl The Eolian class.
 * @return The C data type name as an Eina_Stringshare, or NULL on error.
 *         Example: For a class "My.Object" with no explicit data_type, it might return "My_Object_Data".
 *                  If data_type is "My.Custom.Data", it returns "My_Custom_Data".
 */
EOLIAN_API Eina_Stringshare *
eolian_class_c_data_type_get(const Eolian_Class *cl)
{
   char buf[512];
   EINA_SAFETY_ON_NULL_RETURN_VAL(cl, NULL);
   if (!cl->data_type)
     snprintf(buf, sizeof(buf), "%s_Data", cl->base.c_name);
   else if (!strcmp(cl->data_type, "null"))
     return eina_stringshare_add("void");
   else
     snprintf(buf, sizeof(buf), "%s", cl->data_type);
   for (char *p = strchr(buf, '.'); p; p = strchr(p, '.'))
     *p = '_';
   return eina_stringshare_add(buf);
}
