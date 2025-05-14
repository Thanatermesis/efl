#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Ecore.h>
#include <ctype.h>

#include "ecore_private.h"

#define MY_CLASS EFL_CORE_ENV_CLASS

typedef struct {
   Eina_Hash *env; /**< Hash table to store environment variables. Keys and values are stringshared. */
} Efl_Core_Env_Data;

/**
 * @brief Checks if a string is valid (not NULL and not empty).
 *
 * @param var The string to check.
 * @return EINA_TRUE if the string is valid, EINA_FALSE otherwise.
 */
static inline Eina_Bool
str_valid(const char *var)
{
   return var && var[0] != '\0';
}

/**
 * @brief Checks if an environment variable key is valid.
 *
 * A key is valid if it's not NULL, not empty, does not start with a digit,
 * and contains only alphanumeric characters or underscores.
 *
 * @param key The key string to check.
 * @return EINA_TRUE if the key is valid, EINA_FALSE otherwise.
 */
static inline Eina_Bool
key_valid(const char *key)
{
   if (!key || key[0] == '\0') return EINA_FALSE;

   if (isdigit(key[0])) return EINA_FALSE;

   for (int i = 0; key[i] != '\0'; ++i) {
     if (!isalnum(key[i]) && key[i] != '_') return EINA_FALSE;
   }

   return EINA_TRUE;
}

/**
 * @brief Sets an environment variable.
 *
 * If the value is NULL or an empty string, the variable is effectively unset.
 *
 * @param obj The Efl_Core_Env object.
 * @param pd The private data for the Efl_Core_Env object.
 * @param var The name of the environment variable. Must be a valid key.
 * @param value The value to set for the environment variable.
 */
EOLIAN static void
_efl_core_env_env_set(Eo *obj EINA_UNUSED, Efl_Core_Env_Data *pd, const char *var, const char *value)
{
   EINA_SAFETY_ON_FALSE_RETURN(key_valid(var));
   if (str_valid(value))
     {
        Eina_Stringshare *share;
        share = eina_hash_set(pd->env, var, eina_stringshare_add(value));
        if (share) eina_stringshare_del(share);
     }
   else
     eina_hash_del(pd->env, var, NULL);
}

/**
 * @brief Gets the value of an environment variable.
 *
 * @param obj The Efl_Core_Env object.
 * @param pd The private data for the Efl_Core_Env object.
 * @param var The name of the environment variable. Must be a valid key.
 * @return The value of the environment variable as a stringshared string,
 *         or NULL if the variable is not set or the key is invalid.
 *         The returned string should not be modified or freed by the caller.
 */
EOLIAN static const char*
_efl_core_env_env_get(const Eo *obj EINA_UNUSED, Efl_Core_Env_Data *pd, const char *var)
{
   EINA_SAFETY_ON_FALSE_RETURN_VAL(key_valid(var), NULL);

   return eina_hash_find(pd->env, var);
}

/**
 * @brief Unsets (removes) an environment variable.
 *
 * @param obj The Efl_Core_Env object.
 * @param pd The private data for the Efl_Core_Env object.
 * @param var The name of the environment variable to unset. Must be a valid key.
 */
EOLIAN static void
_efl_core_env_unset(Eo *obj EINA_UNUSED, Efl_Core_Env_Data *pd, const char *var)
{
   EINA_SAFETY_ON_FALSE_RETURN(key_valid(var));
   eina_hash_del_by_key(pd->env, var);
}

/**
 * @brief Clears all environment variables.
 *
 * This removes all key-value pairs from the environment.
 *
 * @param obj The Efl_Core_Env object.
 * @param pd The private data for the Efl_Core_Env object.
 */
EOLIAN static void
_efl_core_env_clear(Eo *obj EINA_UNUSED, Efl_Core_Env_Data *pd)
{
   eina_hash_free_buckets(pd->env);
}

/**
 * @brief Duplicates the environment object.
 *
 * Creates a new Efl_Core_Env object that is a deep copy of the current environment.
 *
 * @param obj The Efl_Core_Env object to duplicate.
 * @param pd The private data for the Efl_Core_Env object.
 * @return A new Efl_Core_Env object which is a duplicate of the original,
 *         or NULL on failure. The caller owns the returned object and must
 *         efl_unref() it when no longer needed.
 */
EOLIAN static Efl_Core_Env*
_efl_core_env_efl_duplicate_duplicate(const Eo *obj EINA_UNUSED, Efl_Core_Env_Data *pd)
{
   Efl_Core_Env *fork = efl_add_ref(MY_CLASS, NULL);
   Eina_Iterator *iter;
   Eina_Hash_Tuple *tuple;

   iter = eina_hash_iterator_tuple_new(pd->env);

   EINA_ITERATOR_FOREACH(iter, tuple)
     {
        efl_core_env_set(fork, tuple->key, tuple->data);
     }

   eina_iterator_free(iter);
   return fork;
}

/**
 * @brief Constructor for the Efl_Core_Env object.
 *
 * Initializes the internal hash table for storing environment variables.
 *
 * @param obj The Efl_Core_Env object being constructed.
 * @param pd The private data for the Efl_Core_Env object.
 * @return The constructed Efl_Object.
 */
EOLIAN static Efl_Object*
_efl_core_env_efl_object_constructor(Eo *obj, Efl_Core_Env_Data *pd)
{
   pd->env = eina_hash_string_superfast_new((Eina_Free_Cb)eina_stringshare_del);

   return efl_constructor(efl_super(obj, MY_CLASS));
}

/**
 * @brief Gets an iterator over the keys of the environment variables.
 *
 * The iterator will yield stringshared keys (const char *).
 * The caller is responsible for freeing the iterator using eina_iterator_free().
 *
 * @param obj The Efl_Core_Env object.
 * @param pd The private data for the Efl_Core_Env object.
 * @return A new Eina_Iterator over the environment variable keys,
 *         or NULL on failure.
 */
EOLIAN static Eina_Iterator*
_efl_core_env_content_get(const Eo *obj EINA_UNUSED, Efl_Core_Env_Data *pd)
{
   Eina_Iterator *iter = eina_hash_iterator_key_new(pd->env);
   return iter;
}


#include "efl_core_env.eo.c"
