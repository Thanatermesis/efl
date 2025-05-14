#ifdef HAVE_CONFIG_H
  #include "elementary_config.h"
#endif

#define EFL_ACCESS_ACTION_PROTECTED

#include "elm_priv.h"

/**
 * @brief Gets the localized name of an accessibility action.
 *
 * This function retrieves the name of an action associated with an
 * accessible object and then localizes it using gettext if NLS
 * (Native Language Support) is enabled.
 *
 * @param[in] obj The Eolian object.
 * @param[in] pd Private data for the Eolian object (unused in this function).
 * @param[in] id The identifier of the action.
 * @return The localized name of the action, or the non-localized name
 *         if NLS is not enabled or the string is not found in translations.
 *         Returns NULL if the action name itself is NULL.
 */
EOLIAN const char *
_efl_access_action_action_localized_name_get(const Eo *obj, void *pd EINA_UNUSED, int id)
{
   const char *ret = NULL;

   ret = efl_access_action_name_get(obj, id);
#ifdef ENABLE_NLS
   ret = gettext(ret);
#endif
   return ret;
}

#include "efl_access_action.eo.c"
