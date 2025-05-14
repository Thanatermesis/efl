#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef _WIN32
# include <evil_private.h> /* setenv unsetenv */
#endif
#include <Ecore.h>
#ifdef HAVE_CRT_EXTERNS_H
# include <crt_externs.h>
#endif
#include "ecore_private.h"

#define MY_CLASS EFL_CORE_PROC_ENV_CLASS

#if defined (__FreeBSD__) || defined (__OpenBSD__)
# include <dlfcn.h>
/**
 * @brief Pointer to the environment variables array on FreeBSD/OpenBSD.
 *
 * On FreeBSD and OpenBSD, `environ` is not directly accessible as a global
 * variable in shared libraries. Instead, it's accessed via `dlsym(NULL, "environ")`.
 * This static variable stores the address of the `environ` pointer.
 */
static char ***_dl_environ;
#elif !defined(_MSC_VER)
extern char **environ;
#endif

/**
 * @brief Global instance of the process environment.
 *
 * This is a singleton instance of Efl_Core_Proc_Env, ensuring that all parts
 * of the application access the same process environment representation.
 */
static Efl_Core_Env *env = NULL;

/**
 * @brief Private data for the Efl_Core_Proc_Env class.
 */
typedef struct {
   Eina_Bool in_sync; /**< Flag to indicate if the internal cache is synchronized with the actual process environment. */
} Efl_Core_Proc_Env_Data;

/**
 * @brief Synchronizes the internal environment cache with the process environment.
 *
 * This function reads the current process environment and updates the
 * Efl_Core_Env object. It ensures that any changes made directly to the
 * process environment (e.g., by other libraries or parts of the code not
 * using Efl_Core_Env) are reflected. It also removes any variables from
 * the internal cache that no longer exist in the process environment.
 *
 * @param obj The Efl_Core_Env object (actually Efl_Core_Proc_Env).
 * @param pd The private data for the Efl_Core_Proc_Env instance.
 */
static void
_sync(Efl_Core_Env *obj, Efl_Core_Proc_Env_Data *pd)
{
   Eina_List *existing_keys = NULL, *n;
   Eina_Iterator *content;
   const char *key;
   char **loc_env = NULL;

   pd->in_sync = EINA_TRUE;
   content = efl_core_env_content_get(obj);

   EINA_ITERATOR_FOREACH(content, key)
     {
        existing_keys = eina_list_append(existing_keys, key);
     }

#if defined (__FreeBSD__) || defined (__OpenBSD__)
   _dl_environ = dlsym(NULL, "environ");
   if (_dl_environ) loc_env = *_dl_environ;
   else ERR("Can't find envrion symbol");
#else
   loc_env = environ;
#endif
   if (loc_env)
     {
        char **p;

        for (p = loc_env; *p; p++)
          {
             char **values;

             values = eina_str_split(*p, "=", 2);
             if (!values) break;

             efl_core_env_set(obj, values[0], values[1]);

             EINA_LIST_FOREACH(existing_keys, n, key)
               {
                  if (!strcmp(key, values[0]))
                    {
                       existing_keys = eina_list_remove_list(existing_keys, n);
                       break;
                    }
               }
             free(values[0]);
             free(values);
          }
     }
   EINA_LIST_FOREACH(existing_keys, n, key)
     {
        efl_core_env_unset(obj, key);
     }
   pd->in_sync = EINA_FALSE;
}

/**
 * @brief Implements Efl.Core.Env.env_get for the process environment.
 *
 * Retrieves the value of an environment variable. If the internal cache
 * is not synchronized, it calls _sync first.
 *
 * @param obj The Efl_Core_Proc_Env object.
 * @param pd The private data for the Efl_Core_Proc_Env instance.
 * @param var The name of the environment variable to retrieve.
 * @return The value of the environment variable, or NULL if not set.
 */
EOLIAN static const char*
_efl_core_proc_env_efl_core_env_env_get(const Eo *obj, Efl_Core_Proc_Env_Data *pd, const char *var)
{
   if (!pd->in_sync)
     _sync((Eo*)obj, pd);
   return efl_core_env_get(efl_super(obj, MY_CLASS), var);
}

/**
 * @brief Implements Efl.Core.Env.env_set for the process environment.
 *
 * Sets or unsets an environment variable. If the internal cache is not
 * synchronized (meaning changes are directly applied to the process
 * environment), it calls setenv() or unsetenv() accordingly.
 *
 * @param obj The Efl_Core_Proc_Env object.
 * @param pd The private data for the Efl_Core_Proc_Env instance.
 * @param var The name of the environment variable to set.
 * @param value The value to set for the variable. If NULL, the variable is unset.
 */
EOLIAN static void
_efl_core_proc_env_efl_core_env_env_set(Eo *obj, Efl_Core_Proc_Env_Data *pd, const char *var, const char *value)
{
   efl_core_env_set(efl_super(obj, MY_CLASS), var, value);
   if (!pd->in_sync)
     {
        if (value)
          setenv(var, value, 1);
        else
          unsetenv(var);
     }
}

/**
 * @brief Implements Efl.Core.Env.unset for the process environment.
 *
 * Unsets (removes) an environment variable. If the internal cache is not
 * synchronized, it calls unsetenv().
 *
 * @param obj The Efl_Core_Proc_Env object.
 * @param pd The private data for the Efl_Core_Proc_Env instance.
 * @param key The name of the environment variable to unset.
 */
EOLIAN static void
_efl_core_proc_env_efl_core_env_unset(Eo *obj, Efl_Core_Proc_Env_Data *pd, const char *key)
{
   efl_core_env_unset(efl_super(obj, MY_CLASS), key);
   if (!pd->in_sync)
     {
        unsetenv(key);
     }
}

/**
 * @brief Implements Efl.Core.Env.clear for the process environment.
 *
 * Clears all environment variables. If the internal cache is not
 * synchronized, it attempts to clear the process environment using
 * clearenv() or by setting `environ` to NULL.
 *
 * @param obj The Efl_Core_Proc_Env object.
 * @param pd The private data for the Efl_Core_Proc_Env instance.
 */
EOLIAN static void
_efl_core_proc_env_efl_core_env_clear(Eo *obj, Efl_Core_Proc_Env_Data *pd)
{
   efl_core_env_clear(efl_super(obj, MY_CLASS));
   if (!pd->in_sync)
     {
#ifdef HAVE_CLEARENV
        clearenv();
#else
# if defined (__FreeBSD__) || defined (__OpenBSD__)
        _dl_environ = dlsym(NULL, "environ");
        if (_dl_environ) *_dl_environ = NULL;
        else ERR("Can't find envrion symbol");
# else
        environ = NULL;
# endif
#endif
     }
}


/**
 * @brief Implements Efl.Duplicate.duplicate for the process environment.
 *
 * Creates a duplicate of the environment object. If the internal cache
 * is not synchronized, it calls _sync first.
 *
 * @param obj The Efl_Core_Proc_Env object to duplicate.
 * @param pd The private data for the Efl_Core_Proc_Env instance.
 * @return A new Efl_Duplicate object representing a copy of the environment.
 */
EOLIAN static Efl_Duplicate*
_efl_core_proc_env_efl_duplicate_duplicate(const Eo *obj, Efl_Core_Proc_Env_Data *pd)
{
   if (!pd->in_sync)
     _sync((Eo*) obj, pd);
   return efl_duplicate(efl_super(obj, MY_CLASS));
}

/**
 * @brief Implements Efl.Core.Env.content_get for the process environment.
 *
 * Retrieves an iterator over the names (keys) of all environment variables.
 * If the internal cache is not synchronized, it calls _sync first.
 *
 * @param obj The Efl_Core_Proc_Env object.
 * @param pd The private data for the Efl_Core_Proc_Env instance.
 * @return An Eina_Iterator for the environment variable names.
 *         Example of iterating:
 *         @code
 *         Eina_Iterator *it = efl_core_env_content_get(env_obj);
 *         const char *key;
 *         EINA_ITERATOR_FOREACH(it, key) {
 *           printf("Key: %s\n", key);
 *         }
 *         eina_iterator_free(it);
 *         @endcode
 */
EOLIAN static Eina_Iterator*
_efl_core_proc_env_efl_core_env_content_get(const Eo *obj, Efl_Core_Proc_Env_Data *pd)
{
   if (!pd->in_sync)
     _sync((Eo*) obj, pd);
   return efl_core_env_content_get(efl_super(obj, MY_CLASS));
}

/**
 * @brief Implements Efl.Object.constructor for Efl_Core_Proc_Env.
 *
 * Ensures that only one instance of Efl_Core_Proc_Env (the global `env`)
 * can be constructed. This enforces the singleton pattern.
 *
 * @param obj The Efl_Core_Proc_Env object being constructed.
 * @param pd The private data for the Efl_Core_Proc_Env instance.
 * @return The constructed Efl_Object, or NULL if an instance already exists.
 */
EOLIAN static Efl_Object*
_efl_core_proc_env_efl_object_constructor(Eo *obj, Efl_Core_Proc_Env_Data *pd EINA_UNUSED)
{
   EINA_SAFETY_ON_TRUE_RETURN_VAL(!!env, NULL);

   obj = efl_constructor(efl_super(obj, MY_CLASS));
   return obj;
}

/**
 * @brief Provides access to the singleton instance of the process environment.
 *
 * This function implements the Efl.Core.Proc.Env.self method. It returns
 * the global `env` instance, creating it if it doesn't exist yet.
 * The instance is created in the EFL_ID_DOMAIN_SHARED to ensure it's
 * accessible across different parts of an EFL application. A weak reference
 * is added to `env` itself to allow for its cleanup when no longer referenced
 * externally, though typically it lives for the duration of the process.
 *
 * @return The singleton Efl_Core_Env object representing the process environment.
 */
EOLIAN static Efl_Core_Env*
_efl_core_proc_env_self(void)
{
   if (!env)
     {
        efl_domain_current_push(EFL_ID_DOMAIN_SHARED);
        env = efl_add_ref(EFL_CORE_PROC_ENV_CLASS, NULL);
        efl_domain_current_pop();
        efl_wref_add(env, &env);
     }

   return env;
}

#include "efl_core_proc_env.eo.c"
