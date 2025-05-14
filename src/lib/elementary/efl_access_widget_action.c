#ifdef HAVE_CONFIG_H
  #include "elementary_config.h"
#endif

#define EFL_ACCESS_ACTION_PROTECTED
#define EFL_ACCESS_WIDGET_ACTION_PROTECTED

#include "elm_priv.h"

extern Eina_Hash *_elm_key_bindings;

/**
 * @brief Executes the action corresponding to the given ID.
 *
 * This function retrieves the list of available actions for the widget
 * and executes the action at the specified index (id).
 *
 * @param[in] obj The Efl_Access_Widget_Action object.
 * @param[in] pd Private data, unused.
 * @param[in] id The zero-based index of the action to perform.
 * @return @c EINA_TRUE if the action was successfully performed, @c EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_access_widget_action_efl_access_action_action_do(Eo *obj, void *pd EINA_UNUSED, int id)
{
   const Efl_Access_Action_Data *actions = NULL;
   const char *param;
   Eina_Bool (*func)(Eo *eo, const char *params) = NULL;
   int tmp = 0;

   actions = efl_access_widget_action_elm_actions_get(obj);
   if (!actions) return EINA_FALSE;

   while (actions[tmp].name)
     {
        if (tmp == id)
          {
             func = actions[tmp].func;
             param = actions[tmp].param;
             break;
          }
        tmp++;
     }

   if (!func)
     return EINA_FALSE;

   return func(obj, param);
}

/**
 * @brief Gets the keybinding associated with the action of the given ID.
 *
 * This function retrieves the action details for the specified ID and
 * then searches the global keybindings for a match based on the widget type,
 * action name, and parameters.
 *
 * @param[in] obj The Efl_Access_Widget_Action object.
 * @param[in] pd Private data, unused.
 * @param[in] id The zero-based index of the action.
 * @return A string representing the keybinding (e.g., "Control+A", "F1")
 *         if found, otherwise @c NULL. The caller is responsible for freeing
 *         the returned string.
 */
EOLIAN static char*
_efl_access_widget_action_efl_access_action_action_keybinding_get(Eo *obj, void *pd EINA_UNUSED, int id)
{
   const Efl_Access_Action_Data *actions = NULL;
   Eina_List *l1, *binding_list;
   const char *action = NULL, *param = NULL;
   Elm_Config_Binding_Key *binding;
   int tmp = 0;

   if (!efl_isa(obj, EFL_UI_WIDGET_CLASS))
      return NULL;

   actions = efl_access_widget_action_elm_actions_get(obj);
   if (!actions) return NULL;

   while (actions[tmp].name)
     {
        if (tmp == id)
          {
             action = actions[tmp].action;
             param = actions[tmp].param;
             break;
          }
        tmp++;
     }
   if (!action) return NULL;

   binding_list = eina_hash_find(_elm_key_bindings, elm_widget_type_get(obj));

   if (binding_list)
     {
        EINA_LIST_FOREACH(binding_list, l1, binding)
          {
             if (!strcmp(binding->action, action) && (!param ||
                 !strcmp(binding->params, param)))
               {
                  Eina_List *l2;
                  Elm_Config_Binding_Modifier *bm;
                  char *ret;
                  Eina_Strbuf *buf = eina_strbuf_new();
                  eina_strbuf_append_printf(buf, "%s", binding->key);
                  EINA_LIST_FOREACH(binding->modifiers, l2, bm)
                    if (bm->flag) eina_strbuf_append_printf(buf, "+%s", bm->mod);
                  ret = eina_strbuf_string_steal(buf);
                  eina_strbuf_free(buf);
                  return ret;
               }
          }
     }

   return NULL;
}

/**
 * @brief Gets the name of the action corresponding to the given ID.
 *
 * @param[in] obj The Efl_Access_Widget_Action object.
 * @param[in] pd Private data, unused.
 * @param[in] id The zero-based index of the action.
 * @return The name of the action (e.g., "click") if found, otherwise @c NULL.
 *         The returned string is owned by the Efl_Access_Action_Data array and
 *         must not be modified or freed.
 */
EOLIAN static const char *
_efl_access_widget_action_efl_access_action_action_name_get(const Eo *obj, void *pd EINA_UNUSED, int id)
{
   const Efl_Access_Action_Data *actions = NULL;
   int tmp = 0;

   actions = efl_access_widget_action_elm_actions_get(obj);
   if (!actions) return NULL;

   while (actions[tmp].name)
     {
        if (tmp == id) return actions[tmp].name;
        tmp++;
     }
   return NULL;
}

/**
 * @brief Sets the description of the action corresponding to the given ID.
 * @warning This function is a stub and currently does nothing.
 *
 * @param[in] obj The Efl_Access_Widget_Action object, unused.
 * @param[in] pd Private data, unused.
 * @param[in] id The zero-based index of the action, unused.
 * @param[in] description The description to set, unused.
 * @return Always @c EINA_FALSE.
 */
EOLIAN static Eina_Bool
_efl_access_widget_action_efl_access_action_action_description_set(Eo *obj EINA_UNUSED, void *pd EINA_UNUSED, int id EINA_UNUSED, const char *description EINA_UNUSED)
{
   return EINA_FALSE;
}

/**
 * @brief Gets the description of the action corresponding to the given ID.
 * @warning This function is a stub and currently always returns @c NULL.
 *
 * @param[in] obj The Efl_Access_Widget_Action object, unused.
 * @param[in] pd Private data, unused.
 * @param[in] id The zero-based index of the action, unused.
 * @return Always @c NULL.
 */
EOLIAN static const char *
_efl_access_widget_action_efl_access_action_action_description_get(const Eo *obj EINA_UNUSED, void *pd EINA_UNUSED, int id EINA_UNUSED)
{
   return NULL;
}

/**
 * @brief Gets a list of all available action names for the widget.
 *
 * The returned list contains strings which are the names of the actions.
 * For example:
 * @code
 * Eina_List *actions_list;
 * const char *action_name;
 * EINA_LIST_FREE(actions_list, action_name) {
 *    // process action_name, e.g., printf("%s\n", action_name);
 *    // Note: action_name itself should not be freed as it points to
 *    // internal data.
 * }
 * @endcode
 *
 * @param[in] obj The Efl_Access_Widget_Action object.
 * @param[in] pd Private data, unused.
 * @return An Eina_List of strings, where each string is an action name.
 *         Returns @c NULL if no actions are available or an error occurs.
 *         The caller is responsible for freeing the returned Eina_List
 *         using eina_list_free(). The strings within the list must not be freed.
 */
EOLIAN static Eina_List*
_efl_access_widget_action_efl_access_action_actions_get(const Eo *obj, void *pd EINA_UNUSED)
{
   const Efl_Access_Action_Data *actions = NULL;
   Eina_List *ret = NULL;
   int tmp = 0;

   actions = efl_access_widget_action_elm_actions_get(obj);
   if (!actions) return NULL;

   while (actions[tmp].name)
     {
        ret = eina_list_append(ret, actions[tmp].name);
        tmp++;
     }

   return ret;
}

#include "efl_access_widget_action.eo.c"
