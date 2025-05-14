#include "edje_private.h"

/**
 * @internal
 * @file
 * @brief Edje external type handling.
 *
 * This file implements the functionality for managing and interacting with
 * Edje external types. External types allow extending Edje's capabilities
 * with custom objects and behaviors.
 */

static Eina_Hash *type_registry = NULL; /**< @internal Hash table storing registered external types. Keys are type names (strings), values are const Edje_External_Type pointers. */
static int init_count = 0; /**< @internal Initialization counter for the external system. Ensures proper init/shutdown. */

/**
 * @brief Get a string representation of an Edje_External_Param_Type.
 *
 * This function is useful for debugging or logging purposes.
 *
 * @param type The parameter type.
 * @return A string representation of the type, or "(unknown)" if the type is not recognized.
 */
EAPI const char *
edje_external_param_type_str(Edje_External_Param_Type type)
{
   switch (type)
     {
      case EDJE_EXTERNAL_PARAM_TYPE_INT:
        return "INT";

      case EDJE_EXTERNAL_PARAM_TYPE_DOUBLE:
        return "DOUBLE";

      case EDJE_EXTERNAL_PARAM_TYPE_STRING:
        return "STRING";

      case EDJE_EXTERNAL_PARAM_TYPE_BOOL:
        return "BOOL";

      case EDJE_EXTERNAL_PARAM_TYPE_CHOICE:
        return "CHOICE";

      default:
        return "(unknown)";
     }
}

/**
 * @internal
 * @brief Get the Evas_Object associated with an external part.
 *
 * Retrieves the actual Evas_Object that has been swallowed into an
 * EXTERNAL part of an Edje object.
 *
 * @param ed The Edje object.
 * @param part The name of the part.
 * @return The Evas_Object if found and the part is an EXTERNAL type, otherwise NULL.
 */
Evas_Object *
_edje_object_part_external_object_get(Edje *ed, const char *part)
{
   Edje_Real_Part *rp;

   if ((!ed) || (!part)) return NULL;

   /* Need to recalc before providing the object. */
   _edje_recalc_do(ed);

   rp = _edje_real_part_recursive_get(&ed, (char *)part);
   if (!rp)
     {
        ERR("no part '%s'", part);
        return NULL;
     }
   if (rp->part->type != EDJE_PART_TYPE_EXTERNAL)
     {
        ERR("cannot get external object of a part '%s' that is not EXTERNAL",
            rp->part->name);
        return NULL;
     }
   if ((rp->type != EDJE_RP_TYPE_SWALLOW) ||
       (!rp->typedata.swallow)) return NULL;

   return rp->typedata.swallow->swallowed_object;
}

/**
 * @internal
 * @brief Set a parameter for an external part.
 *
 * This function sets a parameter for the Evas_Object swallowed in an
 * EXTERNAL part.
 *
 * @param ed The Edje object.
 * @param part The name of the part.
 * @param param The parameter to set. Must not be NULL, and param->name must not be NULL.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
Eina_Bool
_edje_object_part_external_param_set(Edje *ed, const char *part, const Edje_External_Param *param)
{
   Edje_Real_Part *rp;

   if (!ed || !part || !param || !param->name) return EINA_FALSE;

   rp = _edje_real_part_recursive_get(&ed, (char *)part);
   if (!rp)
     {
        ERR("no part '%s'", part);
        return EINA_FALSE;
     }

   if (_edje_external_param_set(ed->obj, rp, param))
     return EINA_TRUE;
   return EINA_FALSE;
}

/**
 * @internal
 * @brief Get a parameter from an external part.
 *
 * This function retrieves a parameter from the Evas_Object swallowed in an
 * EXTERNAL part.
 *
 * @param ed The Edje object.
 * @param part The name of the part.
 * @param param A pointer to an Edje_External_Param structure to be filled.
 *              param->name must be set to the name of the parameter to retrieve.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
Eina_Bool
_edje_object_part_external_param_get(Edje *ed, const char *part, Edje_External_Param *param)
{
   Edje_Real_Part *rp;

   if (!ed || !part || !param || !param->name) return EINA_FALSE;

   rp = _edje_real_part_recursive_get(&ed, (char *)part);
   if (!rp)
     {
        ERR("no part '%s'", part);
        return EINA_FALSE;
     }

   return _edje_external_param_get(ed->obj, rp, param);
}

/**
 * @internal
 * @brief Get a named content object from an external part.
 *
 * This function retrieves a specific content object by name from the
 * Evas_Object swallowed in an EXTERNAL part. This is analogous to
 * how some Evas objects can provide named sub-objects (contents).
 *
 * @param ed The Edje object.
 * @param part The name of the external part.
 * @param content The name of the content to retrieve from the external object. Must not be NULL.
 * @return The Evas_Object for the named content if found, otherwise NULL.
 */
Evas_Object *
_edje_object_part_external_content_get(Edje *ed, const char *part, const char *content)
{
   Edje_Real_Part *rp;

   if (!content) return NULL;

   if ((!ed) || (!part)) return NULL;

   rp = _edje_real_part_recursive_get(&ed, (char *)part);
   if (!rp)
     {
        ERR("no part '%s'", part);
        return NULL;
     }
   if ((rp->type != EDJE_RP_TYPE_SWALLOW) ||
       (!rp->typedata.swallow)) return NULL;

   return _edje_external_content_get(rp->typedata.swallow->swallowed_object, content);
}

/**
 * @internal
 * @brief Get the type of a specific parameter of an external part.
 *
 * This function queries the external object associated with a part to determine
 * the data type of one of its parameters.
 *
 * @param ed The Edje object.
 * @param part The name of the external part.
 * @param param The name of the parameter whose type is to be retrieved.
 * @return The Edje_External_Param_Type of the parameter, or EDJE_EXTERNAL_PARAM_TYPE_MAX on error
 *         (e.g., part not found, not an external part, parameter not found).
 */
Edje_External_Param_Type
_edje_object_part_external_param_type_get(Edje *ed, const char *part, const char *param)
{
   Edje_Real_Part *rp;
   Edje_External_Type *type;
   Edje_External_Param_Info *info;

   if ((!ed) || (!part) || (!param)) return EDJE_EXTERNAL_PARAM_TYPE_MAX;

   rp = _edje_real_part_recursive_get(&ed, (char *)part);
   if (!rp)
     {
        ERR("no part '%s'", part);
        return EDJE_EXTERNAL_PARAM_TYPE_MAX;
     }
   if ((rp->type != EDJE_RP_TYPE_SWALLOW) ||
       (!rp->typedata.swallow)) return EDJE_EXTERNAL_PARAM_TYPE_MAX;
   type = evas_object_data_get(rp->typedata.swallow->swallowed_object, "Edje_External_Type");
   if (!type)
     {
        ERR("no external type for object %p", ed->obj);
        return EDJE_EXTERNAL_PARAM_TYPE_MAX;
     }
   if (!type->parameters_info)
     {
        ERR("no parameters information for external type '%s'",
            type->module_name);
        return EDJE_EXTERNAL_PARAM_TYPE_MAX;
     }
   for (info = type->parameters_info; info->name; info++)
     if (strcmp(info->name, param) == 0) return info->type;

   ERR("no parameter '%s' external type '%s'", param, type->module_name);

   return EDJE_EXTERNAL_PARAM_TYPE_MAX;
}

/**
 * @brief Register a new external type.
 *
 * Allows external modules to register their custom types with Edje.
 * The provided `type_info` structure contains callbacks and metadata
 * for the new external type.
 *
 * @param type_name The name for the new external type. This name is used in EDC files.
 * @param type_info A pointer to a const Edje_External_Type structure describing the type.
 *                  The `abi_version` field of `type_info` must match
 *                  `EDJE_EXTERNAL_TYPE_ABI_VERSION`.
 * @return EINA_TRUE on successful registration, EINA_FALSE otherwise (e.g., NULL parameters,
 *         ABI mismatch, type name already registered).
 */
EAPI Eina_Bool
edje_external_type_register(const char *type_name, const Edje_External_Type *type_info)
{
   if (!type_name)
     return EINA_FALSE;
   if (!type_info)
     return EINA_FALSE;

   if (type_info->abi_version != EDJE_EXTERNAL_TYPE_ABI_VERSION)
     {
        ERR("external type '%s' (%p) has incorrect abi version. "
            "got %#x where %#x was expected.",
            type_name, type_info,
            type_info->abi_version, EDJE_EXTERNAL_TYPE_ABI_VERSION);
        return EINA_FALSE;
     }

   if (eina_hash_find(type_registry, type_name))
     {
        ERR("External type '%s' already registered", type_name);
        return EINA_FALSE;
     }
   return eina_hash_add(type_registry, type_name, type_info);
}

/**
 * @brief Unregister an external type.
 *
 * Removes a previously registered external type from Edje.
 *
 * @param type_name The name of the external type to unregister.
 * @return EINA_TRUE if the type was found and unregistered, EINA_FALSE otherwise
 *         (e.g., type_name is NULL or type was not registered).
 */
EAPI Eina_Bool
edje_external_type_unregister(const char *type_name)
{
   if (!type_name)
     return EINA_FALSE;
   return eina_hash_del_by_key(type_registry, type_name);
}

/**
 * @brief Register an array of external types.
 *
 * This is a convenience function to register multiple external types at once.
 * The array must be NULL-terminated (i.e., the last element should have its `name` field as NULL).
 *
 * @param array A pointer to an array of Edje_External_Type_Info structures.
 *              Each element in the array describes an external type to be registered.
 *              Example:
 *              @code
 *              static const My_External_Type_Specific_Data data1 = { ... };
 *              static const Edje_External_Type type_info1 = {
 *                  EDJE_EXTERNAL_TYPE_ABI_VERSION,
 *                  "my_module",
 *                  "my_type_name1",
 *                  &data1,
 *                  my_type1_add_func,
 *                  // ... other function pointers ...
 *              };
 *
 *              static const My_External_Type_Specific_Data data2 = { ... };
 *              static const Edje_External_Type type_info2 = {
 *                  EDJE_EXTERNAL_TYPE_ABI_VERSION,
 *                  "my_module",
 *                  "my_type_name2",
 *                  &data2,
 *                  my_type2_add_func,
 *                  // ... other function pointers ...
 *              };
 *
 *              const Edje_External_Type_Info external_types[] = {
 *                  { "my_type_name1", &type_info1 },
 *                  { "my_type_name2", &type_info2 },
 *                  { NULL, NULL } // Terminator
 *              };
 *              edje_external_type_array_register(external_types);
 *              @endcode
 */
EAPI void
edje_external_type_array_register(const Edje_External_Type_Info *array)
{
   const Edje_External_Type_Info *itr;

   if (!array)
     return;

   for (itr = array; itr->name; itr++)
     {
        if (itr->info->abi_version != EDJE_EXTERNAL_TYPE_ABI_VERSION)
          {
             ERR("external type '%s' (%p) has incorrect abi "
                 "version. got %#x where %#x was expected.",
                 itr->name, itr->info,
                 itr->info->abi_version, EDJE_EXTERNAL_TYPE_ABI_VERSION);
             continue;
          }

        eina_hash_direct_add(type_registry, itr->name, itr->info);
     }
}

/**
 * @brief Unregister an array of external types.
 *
 * This is a convenience function to unregister multiple external types at once.
 * The array must be NULL-terminated (i.e., the last element should have its `name` field as NULL).
 *
 * @param array A pointer to an array of Edje_External_Type_Info structures.
 *              Each element in the array describes an external type to be unregistered.
 *              See edje_external_type_array_register() for an example of the array structure.
 */
EAPI void
edje_external_type_array_unregister(const Edje_External_Type_Info *array)
{
   const Edje_External_Type_Info *itr;

   if (!array)
     return;

   for (itr = array; itr->name; itr++)
     eina_hash_del(type_registry, itr->name, itr->info);
}

/**
 * @brief Get the ABI version for Edje external types.
 *
 * External modules should check this version against their compiled
 * `EDJE_EXTERNAL_TYPE_ABI_VERSION` to ensure compatibility.
 *
 * @return The current ABI version used by Edje for external types.
 */
EAPI unsigned int
edje_external_type_abi_version_get(void)
{
   return EDJE_EXTERNAL_TYPE_ABI_VERSION;
}

/**
 * @brief Get an iterator for all registered external types.
 *
 * The iterator returns Eina_Hash_Tuple objects, where:
 * - tuple->key is the type name (const char *).
 * - tuple->data is the Edje_External_Type structure (const Edje_External_Type *).
 *
 * @return An Eina_Iterator for the registered types, or NULL if the type registry is not initialized.
 *         The caller is responsible for freeing the iterator using eina_iterator_free().
 */
EAPI Eina_Iterator *
edje_external_iterator_get(void)
{
   return eina_hash_iterator_tuple_new(type_registry);
}

/**
 * @brief Find a parameter by its key in a list of parameters.
 *
 * @param params An Eina_List of Edje_External_Param structures.
 * @param key The name (key) of the parameter to find.
 * @return A pointer to the Edje_External_Param if found, otherwise NULL.
 */
EAPI Edje_External_Param *
edje_external_param_find(const Eina_List *params, const char *key)
{
   const Eina_List *l;
   Edje_External_Param *param;

   EINA_LIST_FOREACH(params, l, param)
     if (!strcmp(param->name, key)) return param;

   return NULL;
}

/**
 * @brief Get an integer parameter value from a list of parameters.
 *
 * Searches for a parameter with the given key and type EDJE_EXTERNAL_PARAM_TYPE_INT.
 *
 * @param params An Eina_List of Edje_External_Param structures.
 * @param key The name of the integer parameter.
 * @param[out] ret Pointer to an integer where the value will be stored if found.
 * @return EINA_TRUE if the parameter was found and its value retrieved, EINA_FALSE otherwise.
 */
EAPI Eina_Bool
edje_external_param_int_get(const Eina_List *params, const char *key, int *ret)
{
   Edje_External_Param *param;

   if (!params) return EINA_FALSE;
   param = edje_external_param_find(params, key);

   if (param && param->type == EDJE_EXTERNAL_PARAM_TYPE_INT && ret)
     {
        *ret = param->i;
        return EINA_TRUE;
     }

   return EINA_FALSE;
}

/**
 * @brief Get a double parameter value from a list of parameters.
 *
 * Searches for a parameter with the given key and type EDJE_EXTERNAL_PARAM_TYPE_DOUBLE.
 *
 * @param params An Eina_List of Edje_External_Param structures.
 * @param key The name of the double parameter.
 * @param[out] ret Pointer to a double where the value will be stored if found.
 * @return EINA_TRUE if the parameter was found and its value retrieved, EINA_FALSE otherwise.
 */
EAPI Eina_Bool
edje_external_param_double_get(const Eina_List *params, const char *key, double *ret)
{
   Edje_External_Param *param;

   if (!params) return EINA_FALSE;
   param = edje_external_param_find(params, key);

   if (param && param->type == EDJE_EXTERNAL_PARAM_TYPE_DOUBLE && ret)
     {
        *ret = param->d;
        return EINA_TRUE;
     }

   return EINA_FALSE;
}

/**
 * @brief Get a string parameter value from a list of parameters.
 *
 * Searches for a parameter with the given key and type EDJE_EXTERNAL_PARAM_TYPE_STRING.
 * The returned string is an Eina_Stringshare instance and should not be freed by the caller
 * if it comes directly from the Edje_External_Param structure.
 *
 * @param params An Eina_List of Edje_External_Param structures.
 * @param key The name of the string parameter.
 * @param[out] ret Pointer to a const char* where the string pointer will be stored if found.
 * @return EINA_TRUE if the parameter was found and its value retrieved, EINA_FALSE otherwise.
 */
EAPI Eina_Bool
edje_external_param_string_get(const Eina_List *params, const char *key, const char **ret)
{
   Edje_External_Param *param;

   if (!params) return EINA_FALSE;
   param = edje_external_param_find(params, key);

   if (param && param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING && ret)
     {
        *ret = param->s;
        return EINA_TRUE;
     }

   return EINA_FALSE;
}

/**
 * @brief Get a boolean parameter value from a list of parameters.
 *
 * Searches for a parameter with the given key and type EDJE_EXTERNAL_PARAM_TYPE_BOOL.
 * The boolean value is stored in the `i` member of Edje_External_Param.
 *
 * @param params An Eina_List of Edje_External_Param structures.
 * @param key The name of the boolean parameter.
 * @param[out] ret Pointer to an Eina_Bool where the value will be stored if found.
 * @return EINA_TRUE if the parameter was found and its value retrieved, EINA_FALSE otherwise.
 */
EAPI Eina_Bool
edje_external_param_bool_get(const Eina_List *params, const char *key, Eina_Bool *ret)
{
   Edje_External_Param *param;

   if (!params) return EINA_FALSE;
   param = edje_external_param_find(params, key);

   if (param && param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL && ret)
     {
        *ret = param->i;
        return EINA_TRUE;
     }

   return EINA_FALSE;
}

/**
 * @brief Get a choice parameter value (string) from a list of parameters.
 *
 * Searches for a parameter with the given key and type EDJE_EXTERNAL_PARAM_TYPE_CHOICE.
 * The choice value is stored as a string in the `s` member of Edje_External_Param.
 * The returned string is an Eina_Stringshare instance.
 *
 * @param params An Eina_List of Edje_External_Param structures.
 * @param key The name of the choice parameter.
 * @param[out] ret Pointer to a const char* where the choice string pointer will be stored if found.
 * @return EINA_TRUE if the parameter was found and its value retrieved, EINA_FALSE otherwise.
 */
EAPI Eina_Bool
edje_external_param_choice_get(const Eina_List *params, const char *key, const char **ret)
{
   Edje_External_Param *param;

   if (!params) return EINA_FALSE;
   param = edje_external_param_find(params, key);

   if (param && param->type == EDJE_EXTERNAL_PARAM_TYPE_CHOICE && ret)
     {
        *ret = param->s;
        return EINA_TRUE;
     }

   return EINA_FALSE;
}

/**
 * @brief Get parameter information for a given external type.
 *
 * Retrieves the array of Edje_External_Param_Info structures that describe
 * the parameters supported by the specified external type.
 *
 * @param type_name The name of the registered external type.
 * @return A pointer to the array of Edje_External_Param_Info structures if the type is found
 *         and has parameter information, otherwise NULL. The array is NULL-terminated
 *         (last element has `name` field as NULL).
 */
EAPI const Edje_External_Param_Info *
edje_external_param_info_get(const char *type_name)
{
   Edje_External_Type *type;

   type = eina_hash_find(type_registry, type_name);
   if (!type)
     return NULL;
   return type->parameters_info;
}

/**
 * @brief Get the Edje_External_Type structure for a given external type name.
 *
 * @param type_name The name of the registered external type.
 * @return A pointer to the const Edje_External_Type structure if found, otherwise NULL.
 */
EAPI const Edje_External_Type *
edje_external_type_get(const char *type_name)
{
   return eina_hash_find(type_registry, type_name);
}

/**
 * @internal
 * @brief Initialize the Edje external system.
 *
 * Sets up the type registry. Called multiple times if Edje is initialized
 * multiple times, but only performs actual initialization on the first call.
 */
void
_edje_external_init(void)
{
   if (!type_registry)
     type_registry = eina_hash_string_superfast_new(NULL);

   init_count++;
}

/**
 * @internal
 * @brief Shutdown the Edje external system.
 *
 * Cleans up the type registry. Called multiple times if Edje is shutdown
 * multiple times, but only performs actual shutdown on the last call (when init_count reaches 0).
 */
void
_edje_external_shutdown(void)
{
   if (--init_count == 0)
     {
        eina_hash_free(type_registry);
        type_registry = NULL;
     }
}

/**
 * @internal
 * @brief Create an instance of an external type object.
 *
 * This function looks up the registered external type by `type_name` and calls its
 * `add` constructor function to create a new Evas_Object.
 *
 * @param type_name The name of the external type to instantiate.
 * @param evas The Evas canvas on which to create the object.
 * @param parent The parent Evas_Object for the new external object.
 * @param params A list of Edje_External_Param to configure the new object.
 * @param part_name The name of the Edje part this external object is being created for.
 * @return A new Evas_Object instance of the specified external type, or NULL on failure.
 *         The "Edje_External_Type" Evas_Object data is set on the returned object.
 */
Evas_Object *
_edje_external_type_add(const char *type_name, Evas *evas, Evas_Object *parent, const Eina_List *params, const char *part_name)
{
   Edje_External_Type *type;
   Evas_Object *obj;

   type = eina_hash_find(type_registry, type_name);
   if (!type)
     {
        ERR("external type '%s' not registered", type_name);
        return NULL;
     }

   obj = type->add(type->data, evas, parent, params, part_name);
   if (!obj)
     {
        ERR("External type '%s' returned NULL from constructor", type_name);
        return NULL;
     }

   evas_object_data_set(obj, "Edje_External_Type", type);

   return obj;
}

/**
 * @internal
 * @brief Emit a signal from an external object.
 *
 * This function retrieves the Edje_External_Type associated with the given
 * Evas_Object and calls its `signal_emit` callback.
 *
 * @param obj The external Evas_Object that is emitting the signal.
 * @param emission The signal string (e.g., "mouse,clicked,1").
 * @param source The source string of the signal (e.g., "button").
 */
void
_edje_external_signal_emit(Evas_Object *obj, const char *emission, const char *source)
{
   Edje_External_Type *type;

   type = evas_object_data_get(obj, "Edje_External_Type");
   if (!type)
     {
        ERR("External type data not found.");
        return;
     }

   type->signal_emit(type->data, obj, emission, source);
}

/**
 * @internal
 * @brief Set a parameter on an external object associated with a real part.
 *
 * This function finds the swallowed object within the given real part,
 * retrieves its Edje_External_Type, and then calls the `param_set` callback.
 * It includes a special case for TEXT/TEXTBLOCK parts to set the "text" parameter
 * directly if the object is not a registered external type but a standard text part.
 *
 * @param obj The parent Edje Evas_Object.
 * @param rp The Edje_Real_Part that contains (swallows) the external object.
 * @param param The parameter to set on the external object.
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., part is not a SWALLOW type,
 *         no swallowed object, no external type data, or the type's `param_set` callback fails).
 */
Eina_Bool
_edje_external_param_set(Evas_Object *obj, Edje_Real_Part *rp, const Edje_External_Param *param)
{
   Evas_Object *swallowed_object;

   if ((rp->type != EDJE_RP_TYPE_SWALLOW) ||
       (!rp->typedata.swallow)) return EINA_FALSE;
   swallowed_object = rp->typedata.swallow->swallowed_object;
   Edje_External_Type *type = evas_object_data_get(swallowed_object, "Edje_External_Type");
   if (!type)
     {
        if ((rp->part->type == EDJE_PART_TYPE_TEXT) ||
            (rp->part->type == EDJE_PART_TYPE_TEXTBLOCK))
          {
             if ((param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING) &&
                 (!strcmp(param->name, "text")) && (obj))
               {
                  return edje_object_part_text_set(obj, rp->part->name, param->s);
               }
          }

        ERR("no external type for object %p", swallowed_object);
        return EINA_FALSE;
     }
   if (!type->param_set)
     {
        ERR("external type '%s' from module '%s' does not provide param_set()",
            type->module_name, type->module);
        return EINA_FALSE;
     }
   return type->param_set(type->data, swallowed_object, param);
}

/**
 * @internal
 * @brief Get a parameter from an external object associated with a real part.
 *
 * This function finds the swallowed object within the given real part,
 * retrieves its Edje_External_Type, and then calls the `param_get` callback.
 * It includes a special case for TEXT/TEXTBLOCK parts to get the "text" parameter
 * directly if the object is not a registered external type but a standard text part.
 *
 * @param obj The parent Edje Evas_Object.
 * @param rp The Edje_Real_Part that contains (swallows) the external object.
 * @param param An Edje_External_Param structure where `name` is set to the parameter
 *              to retrieve, and other fields will be filled by the function.
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., part is not a SWALLOW type,
 *         no swallowed object, no external type data, or the type's `param_get` callback fails).
 */
Eina_Bool
_edje_external_param_get(const Evas_Object *obj, Edje_Real_Part *rp, Edje_External_Param *param)
{
   Evas_Object *swallowed_object;

   if ((rp->type != EDJE_RP_TYPE_SWALLOW) ||
       (!rp->typedata.swallow)) return EINA_FALSE;
   swallowed_object = rp->typedata.swallow->swallowed_object;
   Edje_External_Type *type = evas_object_data_get(swallowed_object, "Edje_External_Type");
   if (!type)
     {
        if ((rp->part->type == EDJE_PART_TYPE_TEXT) ||
            (rp->part->type == EDJE_PART_TYPE_TEXTBLOCK))
          {
             const char *text;
             param->type = EDJE_EXTERNAL_PARAM_TYPE_STRING;
             param->name = "text";
             text = edje_object_part_text_get(obj, rp->part->name);
             param->s = text;
             return EINA_TRUE;
          }

        ERR("no external type for object %p", swallowed_object);
        return EINA_FALSE;
     }
   if (!type->param_get)
     {
        ERR("external type '%s' from module '%s' does not provide param_get()",
            type->module_name, type->module);
        return EINA_FALSE;
     }
   return type->param_get(type->data, swallowed_object, param);
}

/**
 * @internal
 * @brief Get a named content from an external object.
 *
 * Retrieves the Edje_External_Type associated with the given Evas_Object
 * and calls its `content_get` callback to retrieve a sub-object.
 *
 * @param obj The external Evas_Object from which to get content.
 * @param content The name of the content to retrieve.
 * @return The Evas_Object representing the content if found and supported, otherwise NULL.
 */
Evas_Object *
_edje_external_content_get(const Evas_Object *obj, const char *content)
{
   Edje_External_Type *type = evas_object_data_get(obj, "Edje_External_Type");
   if (!type)
     {
        ERR("no external type for object %p", obj);
        return NULL;
     }
   if (!type->content_get)
     {
        ERR("external type '%s' from module '%s' does not provide content_get()",
            type->module_name, type->module);
        return NULL;
     }
   return type->content_get(type->data, obj, content);
}

/**
 * @internal
 * @brief Free a list of Edje_External_Param structures.
 *
 * Iterates through an Eina_List of Edje_External_Param pointers,
 * optionally freeing the stringshare members (`name` and `s`) and then
 * freeing the parameter structure itself.
 *
 * @param external_params The Eina_List of Edje_External_Param pointers to free.
 *                        The list itself is also freed.
 * @param free_strings If EINA_TRUE, eina_stringshare_del() will be called on
 *                     param->name and param->s for each parameter.
 */
void
_edje_external_params_free(Eina_List *external_params, Eina_Bool free_strings)
{
   Edje_External_Param *param;

   EINA_LIST_FREE(external_params, param)
     {
        if (free_strings)
          {
             if (param->name) eina_stringshare_del(param->name);
             if (param->s) eina_stringshare_del(param->s);
          }
        free(param);
     }
}

/**
 * @internal
 * @brief Apply state changes to an external part during Edje recalculation.
 *
 * This function is called during the Edje recalculation process for parts
 * of type EXTERNAL. It retrieves the external type of the swallowed object
 * and calls its `state_set` callback if available. This allows the external
 * object to update its appearance or behavior based on the current and
 * previous states defined in Edje part descriptions.
 *
 * @param ed The Edje object (unused in this function but part of a common signature).
 * @param ep The Edje_Real_Part representing the external part.
 * @param params The Edje calculation parameters (unused in this function).
 * @param chosen_desc The chosen part description (unused in this function).
 */
void
_edje_external_recalc_apply(Edje *ed EINA_UNUSED, Edje_Real_Part *ep,
                            Edje_Calc_Params *params EINA_UNUSED,
                            Edje_Part_Description_Common *chosen_desc EINA_UNUSED)
{
   Edje_External_Type *type;
   Edje_Part_Description_External *ext;
   void *params1, *params2 = NULL;

   if ((ep->type != EDJE_RP_TYPE_SWALLOW) ||
       (!ep->typedata.swallow)) return;
   if (!ep->typedata.swallow->swallowed_object) return;
   type = evas_object_data_get(ep->typedata.swallow->swallowed_object, "Edje_External_Type");

   if ((!type) || (!type->state_set)) return;

   ext = (Edje_Part_Description_External *)ep->param1.description;

   params1 = ep->param1.external_params ?
     ep->param1.external_params : ext->external_params;

   if (ep->param2 && ep->param2->description)
     {
        ext = (Edje_Part_Description_External *)ep->param2->description;

        params2 = ep->param2->external_params ?
          ep->param2->external_params : ext->external_params;
     }

   type->state_set(type->data, ep->typedata.swallow->swallowed_object,
                   params1, params2, ep->description_pos);
}

/**
 * @internal
 * @brief Parse a list of Edje_External_Param into a custom format for an external type.
 *
 * This function retrieves the Edje_External_Type associated with the given
 * Evas_Object and calls its `params_parse` callback. This allows an external
 * type to convert a generic list of parameters into a more optimized or
 * type-specific internal representation.
 *
 * @param obj The external Evas_Object.
 * @param params An Eina_List of Edje_External_Param structures to be parsed.
 * @return A pointer to the custom parsed parameters structure, or NULL if parsing is not
 *         supported by the type or fails. The ownership of this data depends on the
 *         external type's implementation.
 */
void *
_edje_external_params_parse(Evas_Object *obj, const Eina_List *params)
{
   Edje_External_Type *type;

   type = evas_object_data_get(obj, "Edje_External_Type");
   if (!type) return NULL;

   if (!type->params_parse) return NULL;

   return type->params_parse(type->data, obj, params);
}

/**
 * @internal
 * @brief Free custom parsed parameters for an external type.
 *
 * This function retrieves the Edje_External_Type associated with the given
 * Evas_Object and calls its `params_free` callback. This is used to clean up
 * the custom parameter data that might have been allocated by `_edje_external_params_parse`.
 *
 * @param obj The external Evas_Object.
 * @param params A pointer to the custom parsed parameters structure to be freed.
 *               If NULL, the function does nothing.
 */
void
_edje_external_parsed_params_free(Evas_Object *obj, void *params)
{
   Edje_External_Type *type;

   if (!params) return;

   type = evas_object_data_get(obj, "Edje_External_Type");
   if (!type) return;

   if (!type->params_free) return;

   type->params_free(params);
}

