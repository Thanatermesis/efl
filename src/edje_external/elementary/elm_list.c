#include <assert.h>
#include "private.h"

/**
 * @brief Structure to hold parameters for an Elm_List widget.
 *
 * This structure is used to parse and store configuration parameters
 * for an Elm_List widget, typically from an Edje external definition.
 */
typedef struct _Elm_Params_List
{
   Elm_Params base; /**< Base parameters, common to all Elm widgets. */
   const char *policy_h; /**< String representation of horizontal scroller policy (e.g., "auto", "on", "off"). */
   const char *policy_v; /**< String representation of vertical scroller policy (e.g., "auto", "on", "off"). */
   const char *mode; /**< String representation of list mode (e.g., "compress", "scroll"). */
   Eina_Bool h_mode : 1; /**< Boolean flag for horizontal mode. */
   Eina_Bool h_mode_exists : 1; /**< Flag indicating if h_mode was specified. */
   Eina_Bool multi : 1; /**< Boolean flag for multi-select mode. */
   Eina_Bool multi_exists : 1; /**< Flag indicating if multi was specified. */
   Eina_Bool always_select : 1; /**< Boolean flag for always-select mode. */
   Eina_Bool always_select_exists : 1; /**< Flag indicating if always_select was specified. */
} Elm_Params_List;

/**
 * @brief Macro to find the index of a string in an array of strings.
 *
 * This macro iterates through a given array of C strings (CHOICES)
 * and returns the index of the first string that matches the input
 * string (STR).
 *
 * @param CHOICES The array of C strings to search within.
 *                Example: `static const char *my_choices[] = {"one", "two", "three", NULL};`
 * @param STR The C string to search for.
 *            Example: `"two"`
 * @return The index of the found string in CHOICES, or the size of CHOICES if not found (implicitly).
 */
#define CHOICE_GET(CHOICES, STR)                \
  unsigned int i;                               \
  for (i = 0; i < (sizeof(CHOICES)/sizeof(CHOICES[0])); i++)         \
    if (strcmp(STR, CHOICES[i]) == 0)           \
      return i

/** @brief Array of strings representing scroller policy choices.
 * Used to map string values from Edje external params to Elm_Scroller_Policy enum values.
 * The order must match the Elm_Scroller_Policy enum.
 */
static const char *scroller_policy_choices[] = { "auto", "on", "off", NULL };
/** @brief Array of strings representing list mode choices.
 * Used to map string values from Edje external params to Elm_List_Mode enum values.
 * The order must match the Elm_List_Mode enum.
 */
static const char *list_mode_choices[] =
{
   "compress", "scroll", "limit", "expand", NULL
};

/**
 * @brief Converts a scroller policy string to an Elm_Scroller_Policy enum value.
 *
 * @param policy_str The string representation of the scroller policy (e.g., "auto", "on", "off").
 * @return The corresponding Elm_Scroller_Policy enum value.
 *         Returns ELM_SCROLLER_POLICY_LAST if the string is not a valid policy.
 */
static Elm_Scroller_Policy
_scroller_policy_choices_setting_get(const char *policy_str)
{
   assert(sizeof(scroller_policy_choices)/
          sizeof(scroller_policy_choices[0]) == ELM_SCROLLER_POLICY_LAST + 1);
   CHOICE_GET(scroller_policy_choices, policy_str);
   return ELM_SCROLLER_POLICY_LAST;
}

/**
 * @brief Converts a list mode string to an Elm_List_Mode enum value.
 *
 * @param mode_str The string representation of the list mode (e.g., "compress", "scroll").
 * @return The corresponding Elm_List_Mode enum value.
 *         Returns ELM_LIST_LAST if the string is not a valid mode.
 */
static Elm_List_Mode
_list_mode_setting_get(const char *mode_str)
{
   assert(sizeof(list_mode_choices)/sizeof(list_mode_choices[0]) ==
          ELM_LIST_LAST + 1);
   CHOICE_GET(list_mode_choices, mode_str);
   return ELM_LIST_LAST;
}

/**
 * @brief Sets the state of an Elm_List widget based on parsed parameters.
 *
 * This function is called to apply the state defined in Elm_Params_List
 * to the actual Elm_List object. It handles setting list mode, scroller policies,
 * horizontal mode, multi-select mode, and always-select mode.
 *
 * @param data Unused user data.
 * @param obj The Elm_List Evas_Object to configure.
 * @param from_params The previous state parameters (used if to_params is NULL).
 * @param to_params The target state parameters.
 * @param pos Unused position value for transitions.
 */
static void
external_list_state_set(void *data EINA_UNUSED, Evas_Object *obj,
                        const void *from_params, const void *to_params,
                        float pos EINA_UNUSED)
{
   const Elm_Params_List *p;
   Elm_Scroller_Policy policy_h, policy_v;

   if (to_params) p = to_params;
   else if (from_params) p = from_params;
   else return;

   if (p->mode)
     {
        Elm_List_Mode set = _list_mode_setting_get(p->mode);

        if (set != ELM_LIST_LAST)
          elm_list_mode_set(obj, set);
     }

   if ((p->policy_h) && (p->policy_v))
     {
        policy_h = _scroller_policy_choices_setting_get(p->policy_h);
        policy_v = _scroller_policy_choices_setting_get(p->policy_v);
        elm_scroller_policy_set(obj, policy_h, policy_v);
     }
   else if ((p->policy_h) || (p->policy_v))
     {
        elm_scroller_policy_get(obj, &policy_h, &policy_v);
        if (p->policy_h)
          {
             policy_h = _scroller_policy_choices_setting_get(p->policy_h);
             elm_scroller_policy_set(obj, policy_h, policy_v);
          }
        else
          {
             policy_v = _scroller_policy_choices_setting_get(p->policy_v);
             elm_scroller_policy_set(obj, policy_h, policy_v);
          }
     }

   if (p->h_mode_exists)
     elm_list_horizontal_set(obj, p->h_mode);
   if (p->multi_exists)
     elm_list_multi_select_set(obj, p->multi);
   if (p->always_select_exists)
     {
        if (p->always_select)
          elm_list_select_mode_set (obj, ELM_OBJECT_SELECT_MODE_ALWAYS);
        else
          elm_list_select_mode_set (obj, ELM_OBJECT_SELECT_MODE_DEFAULT);
     }
}

/**
 * @brief Sets a single external parameter on an Elm_List widget.
 *
 * This function is called by Edje to set individual properties of the
 * Elm_List widget based on external parameters defined in an EDC file.
 * It handles parameters like "list mode", "horizontal scroll",
 * "vertical scroll", "horizontal mode", "multi-select mode", and
 * "always-select mode".
 *
 * @param data Unused user data.
 * @param obj The Elm_List Evas_Object to configure.
 * @param param The Edje_External_Param to apply.
 *              Example for param->name: "list mode"
 *              Example for param->type: EDJE_EXTERNAL_PARAM_TYPE_CHOICE
 *              Example for param->s (if type is CHOICE): "scroll"
 *              Example for param->i (if type is BOOL): EINA_TRUE
 * @return EINA_TRUE if the parameter was successfully set, EINA_FALSE otherwise.
 */
static Eina_Bool
external_list_param_set(void *data EINA_UNUSED, Evas_Object *obj,
                        const Edje_External_Param *param)
{
   if (!strcmp(param->name, "list mode"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_CHOICE)
          {
             Elm_List_Mode set = _list_mode_setting_get(param->s);
             if (set == ELM_LIST_LAST) return EINA_FALSE;
             elm_list_mode_set(obj, set);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "horizontal scroll"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_CHOICE)
          {
             Elm_Scroller_Policy h, v;
             elm_scroller_policy_get(obj, &h, &v);
             h = _scroller_policy_choices_setting_get(param->s);
             if (h == ELM_SCROLLER_POLICY_LAST) return EINA_FALSE;
             elm_scroller_policy_set(obj, h, v);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "vertical scroll"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_CHOICE)
          {
             Elm_Scroller_Policy h, v;
             elm_scroller_policy_get(obj, &h, &v);
             v = _scroller_policy_choices_setting_get(param->s);
             if (v == ELM_SCROLLER_POLICY_LAST) return EINA_FALSE;
             elm_scroller_policy_set(obj, h, v);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "horizontal mode"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             elm_list_horizontal_set(obj, param->i);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "multi-select mode"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             elm_list_multi_select_set(obj, param->i);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "always-select mode"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             if (param->i)
               elm_list_select_mode_set (obj, ELM_OBJECT_SELECT_MODE_ALWAYS);
             else
               elm_list_select_mode_set (obj, ELM_OBJECT_SELECT_MODE_DEFAULT);
             return EINA_TRUE;
          }
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Gets a single external parameter value from an Elm_List widget.
 *
 * This function is called by Edje to retrieve the current value of
 * an external parameter from the Elm_List widget.
 * It handles parameters like "horizontal mode", "multi-select mode",
 * "always-select mode", "horizontal scroll", "vertical scroll", and "list mode".
 *
 * @param data Unused user data.
 * @param obj The Elm_List Evas_Object to query.
 * @param param An Edje_External_Param structure to fill with the parameter's value.
 *              The `param->name` field indicates which parameter to get.
 *              The `param->type` field indicates the expected type.
 *              The function will fill `param->i` for BOOL or `param->s` for CHOICE.
 * @return EINA_TRUE if the parameter was successfully retrieved, EINA_FALSE otherwise.
 */
static Eina_Bool
external_list_param_get(void *data EINA_UNUSED, const Evas_Object *obj,
                        Edje_External_Param *param)
{
   if (!strcmp(param->name, "horizontal mode"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             param->i = elm_list_horizontal_get(obj);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "multi-select mode"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             param->i = elm_list_multi_select_get(obj);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "always-select mode"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             if (elm_list_select_mode_get (obj) ==
                 ELM_OBJECT_SELECT_MODE_ALWAYS)
               param->i = EINA_TRUE;
             else
               param->i = EINA_FALSE;
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "horizontal scroll"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_CHOICE)
          {
             Elm_Scroller_Policy h, v;
             elm_scroller_policy_get(obj, &h, &v);

             param->s = scroller_policy_choices[h];
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "vertical scroll"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_CHOICE)
          {
             Elm_Scroller_Policy h, v;
             elm_scroller_policy_get(obj, &h, &v);

             param->s = scroller_policy_choices[v];
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "list mode"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_CHOICE)
          {
             Elm_List_Mode m = elm_list_mode_get(obj);

             if (m == ELM_LIST_LAST)
               return EINA_FALSE;

             param->s = list_mode_choices[m];
             return EINA_TRUE;
          }
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Parses a list of Edje external parameters into an Elm_Params_List structure.
 *
 * This function iterates over a list of Edje_External_Param structures,
 * extracts the relevant values for an Elm_List, and stores them in a
 * newly allocated Elm_Params_List structure.
 *
 * @param data Unused user data.
 * @param obj Unused Evas_Object.
 * @param params A list (Eina_List) of Edje_External_Param structures to parse.
 *               Each element in the list is an Edje_External_Param*.
 *               Example structure of an Edje_External_Param in the list:
 *               `{ .name = "list mode", .type = EDJE_EXTERNAL_PARAM_TYPE_CHOICE, .s = "scroll" }`
 *               `{ .name = "horizontal mode", .type = EDJE_EXTERNAL_PARAM_TYPE_BOOL, .i = EINA_TRUE }`
 * @return A pointer to a newly allocated Elm_Params_List structure filled
 *         with the parsed parameters. Returns NULL on allocation failure.
 *         The caller is responsible for freeing this structure using
 *         external_list_params_free().
 */
static void *
external_list_params_parse(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                           const Eina_List *params)
{
   Elm_Params_List *mem;
   Edje_External_Param *param;
   const Eina_List *l;

   mem = ELM_NEW(Elm_Params_List);
   if (!mem)
     return NULL;

   EINA_LIST_FOREACH(params, l, param)
     {
        if (!strcmp(param->name, "horizontal mode"))
          {
             mem->h_mode = param->i;
             mem->h_mode_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "multi-select mode"))
          {
             mem->multi = param->i;
             mem->multi_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "always-select mode"))
          {
             mem->always_select = param->i;
             mem->always_select_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "horizontal scroll"))
          mem->policy_h = eina_stringshare_add(param->s);
        else if (!strcmp(param->name, "vertical scroll"))
          mem->policy_v = eina_stringshare_add(param->s);
        else if (!strcmp(param->name, "list mode"))
          mem->mode = eina_stringshare_add(param->s);
     }
   return mem;
}

/**
 * @brief Retrieves content from an Elm_List widget.
 *
 * This function is intended to get a specific content part of the widget.
 * For Elm_List, it currently does not support named content parts and
 * will always return NULL and log an error.
 *
 * @param data Unused user data.
 * @param obj Unused Evas_Object (the list itself).
 * @param content Unused name of the content part to retrieve.
 * @return Always returns NULL for Elm_List.
 */
static Evas_Object *external_list_content_get(void *data EINA_UNUSED,
                                              const Evas_Object *obj EINA_UNUSED,
                                              const char *content EINA_UNUSED)
{
   ERR("No content.");
   return NULL;
}

/**
 * @brief Frees an Elm_Params_List structure.
 *
 * This function releases the memory allocated for an Elm_Params_List
 * structure, including any stringshared strings it might hold.
 *
 * @param params A pointer to the Elm_Params_List structure to free.
 */
static void
external_list_params_free(void *params)
{
   Elm_Params_List *mem = params;

   if (mem->mode)
     eina_stringshare_del(mem->mode);
   if (mem->policy_h)
     eina_stringshare_del(mem->policy_h);
   if (mem->policy_v)
     eina_stringshare_del(mem->policy_v);

   free(mem);
}

/**
 * @brief Array defining the external parameters for an Elm_List widget.
 *
 * This array provides metadata for each parameter that can be configured
 * for an Elm_List widget via Edje external interface. It includes the
 * parameter name, its type, default value, and possible choices for
 * choice-type parameters.
 *
 * Example structure of elements:
 * - `DEFINE_EXTERNAL_COMMON_PARAMS`: Macro that expands to common widget parameters.
 * - `EDJE_EXTERNAL_PARAM_INFO_CHOICE_FULL("list mode", "scroll", list_mode_choices)`:
 *   Defines a parameter named "list mode", of type choice, with a default
 *   value of "scroll", and the possible choices are listed in `list_mode_choices`.
 *   `list_mode_choices` would be an array like: `{"compress", "scroll", "limit", "expand", NULL}`.
 * - `EDJE_EXTERNAL_PARAM_INFO_BOOL("horizontal mode")`:
 *   Defines a boolean parameter named "horizontal mode". Default is typically EINA_FALSE.
 */
static Edje_External_Param_Info external_list_params[] = {
  DEFINE_EXTERNAL_COMMON_PARAMS,
  EDJE_EXTERNAL_PARAM_INFO_CHOICE_FULL("list mode", "scroll",
                                       list_mode_choices),
  EDJE_EXTERNAL_PARAM_INFO_CHOICE_FULL("horizontal scroll", "auto",
                                       scroller_policy_choices),
  EDJE_EXTERNAL_PARAM_INFO_CHOICE_FULL("vertical scroll", "auto",
                                       scroller_policy_choices),
  EDJE_EXTERNAL_PARAM_INFO_BOOL("horizontal mode"),
  EDJE_EXTERNAL_PARAM_INFO_BOOL("multi-select mode"),
  EDJE_EXTERNAL_PARAM_INFO_BOOL("always-select mode"),
  EDJE_EXTERNAL_PARAM_INFO_SENTINEL
};

DEFINE_EXTERNAL_ICON_ADD(list, "list");
DEFINE_EXTERNAL_TYPE_SIMPLE(list, "List");
