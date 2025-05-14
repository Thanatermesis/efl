/**
 * @file
 * @brief These functions provide an API for accessing Eolian function objects.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Eina.h>
#include "eolian_database.h"

/**
 * @brief Get the scope of a function.
 *
 * @param[in] fid The function ID.
 * @param[in] ftype The function type (method, property get, property set).
 * @return The scope of the function (e.g., public, private, protected).
 *         Returns #EOLIAN_SCOPE_UNKNOWN on failure.
 */
EOLIAN_API Eolian_Object_Scope
eolian_function_scope_get(const Eolian_Function *fid, Eolian_Function_Type ftype)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(fid, EOLIAN_SCOPE_UNKNOWN);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(ftype != EOLIAN_UNRESOLVED, EOLIAN_SCOPE_UNKNOWN);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(ftype != EOLIAN_PROPERTY, EOLIAN_SCOPE_UNKNOWN);
   switch (ftype)
     {
      case EOLIAN_METHOD:
        if (fid->type != EOLIAN_METHOD)
          return EOLIAN_SCOPE_UNKNOWN;
        return fid->get_scope;
      case EOLIAN_PROP_GET:
        if ((fid->type != EOLIAN_PROP_GET) && (fid->type != EOLIAN_PROPERTY))
          return EOLIAN_SCOPE_UNKNOWN;
        return fid->get_scope;
      case EOLIAN_PROP_SET:
        if ((fid->type != EOLIAN_PROP_SET) && (fid->type != EOLIAN_PROPERTY))
          return EOLIAN_SCOPE_UNKNOWN;
        return fid->set_scope;
      default:
        return EOLIAN_SCOPE_UNKNOWN;
     }
}

/**
 * @brief Get the type of a function.
 *
 * @param[in] fid The function ID.
 * @return The type of the function (e.g., method, property, constructor).
 *         Returns #EOLIAN_UNRESOLVED on failure.
 */
EOLIAN_API Eolian_Function_Type
eolian_function_type_get(const Eolian_Function *fid)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(fid, EOLIAN_UNRESOLVED);
   return fid->type;
}

/**
 * @internal
 * @brief Get the C prefix for a function.
 *
 * This function retrieves the C prefix associated with the class of the
 * given function. If a specific C prefix is defined for the class, it is
 * returned. Otherwise, the class's base C name is converted to lowercase
 * and used as the prefix.
 *
 * @param[in] foo_id The function ID.
 * @param[out] buf A buffer to store the generated prefix if no explicit
 *                 c_prefix is set on the class. The buffer must be large
 *                 enough.
 * @return The C prefix for the function.
 */
static const char *
_get_c_prefix(const Eolian_Function *foo_id, char *buf)
{
    if (foo_id->klass->c_prefix)
      return foo_id->klass->c_prefix;
    strcpy(buf, foo_id->klass->base.c_name);
    eina_str_tolower(&buf);
    return buf;
}

/**
 * @internal
 * @brief Generate an abbreviated function name based on a prefix.
 *
 * This function attempts to create a shorter version of the function name
 * by removing parts of the prefix if they appear in the function name.
 * For example, if prefix is "my_object_widget" and fname is "widget_show",
 * it might produce "my_object_show". If no abbreviation is possible,
 * it returns "prefix_fname".
 *
 * @param[in] prefix The prefix string (e.g., class C name).
 * @param[in] fname The function name.
 * @return A newly allocated string containing the abbreviated name.
 *         The caller is responsible for freeing this string.
 */
static char *
_get_abbreviated_name(const char *prefix, const char *fname)
{
   Eina_Strbuf *buf = eina_strbuf_new();

   const char *last_p = strrchr(prefix, '_');
   last_p = (last_p) ? (last_p + 1) : prefix;

   const char *tmp = strstr(fname, last_p);
   int len = strlen(last_p);

   if ((tmp) &&
       ((tmp == fname) || (*(tmp - 1) == '_')) &&
       ((*(tmp + len) == '\0') || (*(tmp + len) == '_')))
     {
        int plen = strlen(prefix);
        len += (tmp - fname);

        if ((plen >= len) && !strncmp(prefix + plen - len, fname, len))
          {
             eina_strbuf_append_n(buf, prefix, plen - len);
          }
     }

   if (eina_strbuf_length_get(buf) == 0)
     eina_strbuf_append_printf(buf, "%s_", prefix);
   eina_strbuf_append(buf, fname);

   char *ret = eina_strbuf_string_steal(buf);
   eina_strbuf_free(buf);
   return ret;
}

/**
 * @brief Get the full C name of a function.
 *
 * This constructs the complete C identifier for a function,
 * including its class prefix and any type-specific suffixes (like _get or _set
 * for properties).
 * For example, for a class "My_Object" and a property "name", this might
 * return "my_object_name_get" or "my_object_name_set".
 *
 * @param[in] foo_id The function ID.
 * @param[in] ftype The function type, used to determine suffixes (e.g., _get, _set).
 *                  If #EOLIAN_FUNCTION_POINTER, no class prefix is used.
 * @return A stringshared C name of the function. Returns @c NULL on failure.
 */
EOLIAN_API Eina_Stringshare *
eolian_function_full_c_name_get(const Eolian_Function *foo_id,
                                Eolian_Function_Type ftype)
{
   char tbuf[512];
   tbuf[0] = '\0';
   const char *prefix = (ftype != EOLIAN_FUNCTION_POINTER) ? _get_c_prefix(foo_id, tbuf): tbuf;

   if (!prefix)
     return NULL;

   const char  *funcn = eolian_function_name_get(foo_id);
   Eina_Strbuf *buf = eina_strbuf_new();
   Eina_Stringshare *ret;

   char *abbr = _get_abbreviated_name(prefix, funcn);
   eina_strbuf_append(buf, abbr);
   free(abbr);

   if ((ftype == EOLIAN_PROP_GET) || (ftype == EOLIAN_PROPERTY))
     eina_strbuf_append(buf, "_get");
   else if (ftype == EOLIAN_PROP_SET)
     eina_strbuf_append(buf, "_set");

   ret = eina_stringshare_add(eina_strbuf_string_get(buf));
   eina_strbuf_free(buf);
   return ret;
}

/**
 * @brief Get the implement information for a function.
 *
 * An implement refers to a function in an Eo file that is marked with the
 * `implement` keyword, indicating it provides the implementation for a
 * function defined in an interface or a parent class.
 *
 * @param[in] fid The function ID.
 * @return A pointer to the Eolian_Implement structure if the function is an
 *         implement, @c NULL otherwise or on failure.
 */
EOLIAN_API const Eolian_Implement *
eolian_function_implement_get(const Eolian_Function *fid)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(fid, NULL);
   return fid->impl;
}

/**
 * @brief Check if a function is static.
 *
 * @param[in] fid The function ID.
 * @return #EINA_TRUE if the function is static, #EINA_FALSE otherwise or on failure.
 */
EOLIAN_API Eina_Bool
eolian_function_is_static(const Eolian_Function *fid)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(fid, EINA_FALSE);
   return fid->is_static;
}

/**
 * @brief Check if a function is a constructor for a given class.
 *
 * @param[in] fid The function ID.
 * @param[in] klass The class to check against.
 * @return #EINA_TRUE if the function is a constructor for the class,
 *         #EINA_FALSE otherwise or on failure.
 */
EOLIAN_API Eina_Bool
eolian_function_is_constructor(const Eolian_Function *fid, const Eolian_Class *klass)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(fid, EINA_FALSE);
   Eina_Stringshare *s = eina_stringshare_ref(klass->base.name);
   Eina_Bool r = !!eina_list_search_sorted_list
     (fid->ctor_of, EINA_COMPARE_CB(strcmp), s);
   eina_stringshare_del(s);
   return r;
}

/**
 * @internal
 * @brief Get the list of property keys for a function.
 *
 * This function retrieves the appropriate list of property keys based on whether
 * the function type is a property setter, getter, or a general property.
 *
 * @param[in] fid The function ID (must be a property).
 * @param[in] ftype The function type (#EOLIAN_PROP_GET or #EOLIAN_PROP_SET).
 * @return A list of Eolian_Function_Parameter representing the keys.
 */
static Eina_List *
_get_prop_keys(const Eolian_Function *fid, Eolian_Function_Type ftype)
{
   Eina_List *l = fid->prop_keys_get;
   if (ftype == EOLIAN_PROP_SET) l = fid->prop_keys_set;
   if (!l) return fid->prop_keys;
   return l;
}

/**
 * @internal
 * @brief Get the list of property values for a function.
 *
 * This function retrieves the appropriate list of property values based on whether
 * the function type is a property setter, getter, or a general property.
 *
 * @param[in] fid The function ID (must be a property).
 * @param[in] ftype The function type (#EOLIAN_PROP_GET or #EOLIAN_PROP_SET).
 * @return A list of Eolian_Function_Parameter representing the values.
 */
static Eina_List *
_get_prop_values(const Eolian_Function *fid, Eolian_Function_Type ftype)
{
   Eina_List *l = fid->prop_values_get;
   if (ftype == EOLIAN_PROP_SET) l = fid->prop_values_set;
   if (!l) return fid->prop_values;
   return l;
}

/**
 * @brief Get an iterator over the keys of a property.
 *
 * For a property like `prop(int k1, string k2): int val;`, this function,
 * when ftype is #EOLIAN_PROP_GET or #EOLIAN_PROP_SET, would return an
 * iterator for the key parameters `k1` and `k2`.
 *
 * @param[in] fid The function ID (must be a property).
 * @param[in] ftype The function type, must be #EOLIAN_PROP_GET or #EOLIAN_PROP_SET.
 * @return An iterator over Eolian_Function_Parameter objects representing the keys.
 *         Returns @c NULL on failure or if not a property.
 *         The caller is responsible for freeing the iterator.
 */
EOLIAN_API Eina_Iterator *
eolian_property_keys_get(const Eolian_Function *fid, Eolian_Function_Type ftype)
{
   Eina_List *l = NULL;
   EINA_SAFETY_ON_NULL_RETURN_VAL(fid, NULL);
   if (ftype != EOLIAN_PROP_GET && ftype != EOLIAN_PROP_SET)
     return NULL;
   l = _get_prop_keys(fid, ftype);
   return (l ? eina_list_iterator_new(l) : NULL);
}

/**
 * @brief Get an iterator over the values of a property.
 *
 * For a property like `prop(int k1): int val1, string val2;`, this function,
 * when ftype is #EOLIAN_PROP_GET or #EOLIAN_PROP_SET, would return an
 * iterator for the value parameters `val1` and `val2`.
 *
 * @param[in] fid The function ID (must be a property).
 * @param[in] ftype The function type, must be #EOLIAN_PROP_GET or #EOLIAN_PROP_SET.
 * @return An iterator over Eolian_Function_Parameter objects representing the values.
 *         Returns @c NULL on failure or if not a property.
 *         The caller is responsible for freeing the iterator.
 */
EOLIAN_API Eina_Iterator *
eolian_property_values_get(const Eolian_Function *fid, Eolian_Function_Type ftype)
{
   Eina_List *l = NULL;
   EINA_SAFETY_ON_NULL_RETURN_VAL(fid, NULL);
   if (ftype != EOLIAN_PROP_GET && ftype != EOLIAN_PROP_SET)
     return NULL;
   l = _get_prop_values(fid, ftype);
   return (l ? eina_list_iterator_new(l) : NULL);
}

/**
 * @brief Get an iterator over the parameters of a function or function pointer.
 *
 * This is applicable for functions of type #EOLIAN_METHOD or
 * #EOLIAN_FUNCTION_POINTER.
 *
 * @param[in] fid The function ID.
 * @return An iterator over Eolian_Function_Parameter objects.
 *         Returns @c NULL on failure or if the function type is not applicable.
 *         The caller is responsible for freeing the iterator.
 */
EOLIAN_API Eina_Iterator *
eolian_function_parameters_get(const Eolian_Function *fid)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(fid, NULL);
   if (fid->type != EOLIAN_METHOD && fid->type != EOLIAN_FUNCTION_POINTER)
     return NULL;
   return (fid->params ? eina_list_iterator_new(fid->params) : NULL);
}

/**
 * @brief Get the return type of a function.
 *
 * @param[in] fid The function ID.
 * @param[in] ftype The function type (method, property get/set, function pointer).
 *                  Cannot be #EOLIAN_UNRESOLVED or #EOLIAN_PROPERTY.
 * @return The Eolian_Type of the return value.
 *         Returns @c NULL on failure or if the function type is not applicable
 *         (e.g., asking for return type of a property setter that returns void,
 *         or if the function type doesn't match the actual function's type).
 */
EOLIAN_API const Eolian_Type *
eolian_function_return_type_get(const Eolian_Function *fid, Eolian_Function_Type ftype)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(fid, NULL);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(ftype != EOLIAN_UNRESOLVED, NULL);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(ftype != EOLIAN_PROPERTY, NULL);
   switch (ftype)
     {
      case EOLIAN_METHOD:
      case EOLIAN_FUNCTION_POINTER:
        if (fid->type != ftype)
          return NULL;
        return fid->get_ret_type;
      case EOLIAN_PROP_GET:
        if ((fid->type != EOLIAN_PROP_GET) && (fid->type != EOLIAN_PROPERTY))
          return NULL;
        return fid->get_ret_type;
      case EOLIAN_PROP_SET:
        if ((fid->type != EOLIAN_PROP_SET) && (fid->type != EOLIAN_PROPERTY))
          return NULL;
        return fid->set_ret_type;
      default:
        return NULL;
     }
}

/**
 * @brief Get the default value of a function's return.
 *
 * @param[in] fid The function ID.
 * @param[in] ftype The function type (method, property get/set, function pointer).
 *                  Cannot be #EOLIAN_UNRESOLVED or #EOLIAN_PROPERTY.
 * @return The Eolian_Expression representing the default value of the return.
 *         Returns @c NULL if no default value is specified, on failure, or if
 *         the function type is not applicable.
 */
EOLIAN_API const Eolian_Expression *
eolian_function_return_default_value_get(const Eolian_Function *fid, Eolian_Function_Type ftype)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(fid, NULL);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(ftype != EOLIAN_UNRESOLVED, NULL);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(ftype != EOLIAN_PROPERTY, NULL);
   switch (ftype)
     {
      case EOLIAN_METHOD:
      case EOLIAN_FUNCTION_POINTER:
        if (fid->type != ftype)
          return NULL;
        return fid->get_ret_val;
      case EOLIAN_PROP_GET:
        if ((fid->type != EOLIAN_PROP_GET) && (fid->type != EOLIAN_PROPERTY))
          return NULL;
        return fid->get_ret_val;
      case EOLIAN_PROP_SET:
        if ((fid->type != EOLIAN_PROP_SET) && (fid->type != EOLIAN_PROPERTY))
          return NULL;
        return fid->set_ret_val;
      default:
        return NULL;
     }
}

/**
 * @brief Get the documentation for a function's return value.
 *
 * @param[in] fid The function ID.
 * @param[in] ftype The function type (method, property get/set, function pointer).
 *                  Cannot be #EOLIAN_UNRESOLVED or #EOLIAN_PROPERTY.
 * @return The Eolian_Documentation for the return value.
 *         Returns @c NULL if no documentation is specified, on failure, or if
 *         the function type is not applicable.
 */
EOLIAN_API const Eolian_Documentation *
eolian_function_return_documentation_get(const Eolian_Function *fid, Eolian_Function_Type ftype)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(fid, NULL);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(ftype != EOLIAN_UNRESOLVED, NULL);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(ftype != EOLIAN_PROPERTY, NULL);
   switch (ftype)
     {
      case EOLIAN_METHOD:
      case EOLIAN_FUNCTION_POINTER:
        if (fid->type != ftype)
          return NULL;
        return fid->get_return_doc;
      case EOLIAN_PROP_GET:
        if ((fid->type != EOLIAN_PROP_GET) && (fid->type != EOLIAN_PROPERTY))
          return NULL;
        return fid->get_return_doc;
      case EOLIAN_PROP_SET:
        if ((fid->type != EOLIAN_PROP_SET) && (fid->type != EOLIAN_PROPERTY))
          return NULL;
        return fid->set_return_doc;
      default:
        return NULL;
     }
}

/**
 * @brief Check if the return value of a function can be unused.
 *
 * This typically corresponds to attributes like `[[warn_unused_result]]` in C/C++.
 * If this function returns #EINA_FALSE, it means the return value should ideally
 * be used by the caller.
 *
 * @param[in] fid The function ID.
 * @param[in] ftype The function type (method, property get/set, function pointer).
 *                  Cannot be #EOLIAN_UNRESOLVED or #EOLIAN_PROPERTY.
 * @return #EINA_TRUE if the return value can be unused (default),
 *         #EINA_FALSE if it should not be unused. Returns #EINA_TRUE on failure
 *         or if not applicable.
 */
EOLIAN_API Eina_Bool
eolian_function_return_allow_unused(const Eolian_Function *fid,
      Eolian_Function_Type ftype)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(fid, EINA_TRUE);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(ftype != EOLIAN_UNRESOLVED, EINA_TRUE);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(ftype != EOLIAN_PROPERTY, EINA_TRUE);
   switch (ftype)
     {
      case EOLIAN_METHOD:
      case EOLIAN_FUNCTION_POINTER:
        if (fid->type != ftype)
          return EINA_TRUE;
        return !fid->get_return_no_unused;
      case EOLIAN_PROP_GET:
        if ((fid->type != EOLIAN_PROP_GET) && (fid->type != EOLIAN_PROPERTY))
          return EINA_TRUE;
        return !fid->get_return_no_unused;
      case EOLIAN_PROP_SET:
        if ((fid->type != EOLIAN_PROP_SET) && (fid->type != EOLIAN_PROPERTY))
          return EINA_TRUE;
        return !fid->set_return_no_unused;
      default:
        return EINA_TRUE;
     }
}

/**
 * @brief Check if the return value of a function is passed by reference.
 *
 * @param[in] fid The function ID.
 * @param[in] ftype The function type (method, property get/set, function pointer).
 *                  Cannot be #EOLIAN_UNRESOLVED or #EOLIAN_PROPERTY.
 * @return #EINA_TRUE if the return value is by reference, #EINA_FALSE otherwise.
 *         Returns #EINA_FALSE on failure or if not applicable.
 */
EOLIAN_API Eina_Bool
eolian_function_return_is_by_ref(const Eolian_Function *fid,
      Eolian_Function_Type ftype)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(fid, EINA_FALSE);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(ftype != EOLIAN_UNRESOLVED, EINA_FALSE);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(ftype != EOLIAN_PROPERTY, EINA_FALSE);
   switch (ftype)
     {
      case EOLIAN_METHOD:
      case EOLIAN_FUNCTION_POINTER:
        if (fid->type != ftype)
          return EINA_FALSE;
        return fid->get_return_by_ref;
      case EOLIAN_PROP_GET:
        if ((fid->type != EOLIAN_PROP_GET) && (fid->type != EOLIAN_PROPERTY))
          return EINA_FALSE;
        return fid->get_return_by_ref;
      case EOLIAN_PROP_SET:
        if ((fid->type != EOLIAN_PROP_SET) && (fid->type != EOLIAN_PROPERTY))
          return EINA_FALSE;
        return fid->set_return_by_ref;
      default:
        return EINA_FALSE;
     }
}

/**
 * @brief Check if the return value of a function is to be moved.
 *
 * This indicates that ownership of the returned resource is transferred to the caller.
 *
 * @param[in] fid The function ID.
 * @param[in] ftype The function type (method, property get/set, function pointer).
 *                  Cannot be #EOLIAN_UNRESOLVED or #EOLIAN_PROPERTY.
 * @return #EINA_TRUE if the return value is to be moved, #EINA_FALSE otherwise.
 *         Returns #EINA_FALSE on failure or if not applicable.
 */
EOLIAN_API Eina_Bool
eolian_function_return_is_move(const Eolian_Function *fid,
      Eolian_Function_Type ftype)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(fid, EINA_FALSE);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(ftype != EOLIAN_UNRESOLVED, EINA_FALSE);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(ftype != EOLIAN_PROPERTY, EINA_FALSE);
   switch (ftype)
     {
      case EOLIAN_METHOD:
      case EOLIAN_FUNCTION_POINTER:
        if (fid->type != ftype)
          return EINA_FALSE;
        return fid->get_return_move;
      case EOLIAN_PROP_GET:
        if ((fid->type != EOLIAN_PROP_GET) && (fid->type != EOLIAN_PROPERTY))
          return EINA_FALSE;
        return fid->get_return_move;
      case EOLIAN_PROP_SET:
        if ((fid->type != EOLIAN_PROP_SET) && (fid->type != EOLIAN_PROPERTY))
          return EINA_FALSE;
        return fid->set_return_move;
      default:
        return EINA_FALSE;
     }
}

/**
 * @brief Get the C type string for a function's return value.
 *
 * This function generates the C language representation of the return type,
 * considering whether it's passed by reference. For example, "int", "const char *",
 * "My_Type **" (if by_ref is true for a pointer type).
 *
 * @param[in] fid The function ID.
 * @param[in] ftype The function type (method, property get/set, function pointer).
 *                  Cannot be #EOLIAN_UNRESOLVED or #EOLIAN_PROPERTY.
 * @return A stringshared C type string. Returns @c NULL on failure or if not applicable.
 */
EOLIAN_API Eina_Stringshare *
eolian_function_return_c_type_get(const Eolian_Function *fid,
                                  Eolian_Function_Type ftype)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(fid, NULL);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(ftype != EOLIAN_UNRESOLVED, NULL);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(ftype != EOLIAN_PROPERTY, NULL);
   const Eolian_Type *tp = NULL;
   Eina_Bool by_ref = EINA_FALSE;
   switch (ftype)
     {
      case EOLIAN_METHOD:
      case EOLIAN_FUNCTION_POINTER:
        if (fid->type != ftype)
          return NULL;
        tp = fid->get_ret_type;
        by_ref = fid->get_return_by_ref;
        break;
      case EOLIAN_PROP_GET:
        if ((fid->type != EOLIAN_PROP_GET) && (fid->type != EOLIAN_PROPERTY))
          return NULL;
        tp = fid->get_ret_type;
        by_ref = fid->get_return_by_ref;
        break;
      case EOLIAN_PROP_SET:
        if ((fid->type != EOLIAN_PROP_SET) && (fid->type != EOLIAN_PROPERTY))
          return NULL;
        tp = fid->set_ret_type;
        by_ref = fid->set_return_by_ref;
        break;
      default:
        return NULL;
     }
   Eina_Strbuf *buf = eina_strbuf_new();
   database_type_to_str(tp, buf, NULL, EOLIAN_C_TYPE_RETURN, by_ref);
   Eina_Stringshare *ret = eina_stringshare_add(eina_strbuf_string_get(buf));
   eina_strbuf_free(buf);
   return ret;
}

/**
 * @brief Check if the object instance for a method call is const.
 *
 * This applies to methods and indicates if the method can be called on a
 * const instance of the object (i.e., the method itself is const).
 *
 * @param[in] fid The function ID (typically a method).
 * @return #EINA_TRUE if the object instance is const for this function call,
 *         #EINA_FALSE otherwise or on failure.
 */
EOLIAN_API Eina_Bool
eolian_function_object_is_const(const Eolian_Function *fid)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(fid, EINA_FALSE);
   return fid->obj_is_const;
}

/**
 * @brief Get the class that this function belongs to.
 *
 * @param[in] fid The function ID.
 * @return A pointer to the Eolian_Class. Returns @c NULL on failure.
 */
EOLIAN_API const Eolian_Class *
eolian_function_class_get(const Eolian_Function *fid)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(fid, NULL);
   return fid->klass;
}
