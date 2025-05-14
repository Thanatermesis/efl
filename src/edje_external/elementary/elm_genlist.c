#include <assert.h>
#include "private.h"

/**
 * @brief Structure to hold parameters for Genlist objects.
 * This structure is used to parse and store parameters from Edje external
 * interface, which are then applied to a Genlist widget.
 */
typedef struct _Elm_Params_Genlist
{
   Elm_Params base; /**< Base parameters, common to all Elm widgets. */
   const char *horizontal; /**< String representation of the horizontal mode (e.g., "scroll", "compress"). */
   Eina_Bool multi:1; /**< Boolean flag for multi-selection mode. */
   Eina_Bool multi_exists:1; /**< Flag indicating if 'multi' parameter was provided. */
   Eina_Bool always_select:1; /**< Boolean flag for always-select mode. */
   Eina_Bool always_select_exists:1; /**< Flag indicating if 'always_select' parameter was provided. */
   Eina_Bool no_select:1; /**< Boolean flag for no-selection mode. */
   Eina_Bool no_select_exists:1; /**< Flag indicating if 'no_select' parameter was provided. */
   Eina_Bool compress_exists:1; /**< Flag indicating if 'compress' mode related parameter was provided (unused in current parsing logic but present in struct). */
   Eina_Bool homogeneous:1; /**< Boolean flag for homogeneous mode (items have same size). */
   Eina_Bool homogeneous_exists:1; /**< Flag indicating if 'homogeneous' parameter was provided. */
   Eina_Bool h_bounce:1; /**< Boolean flag for horizontal bounce. */
   Eina_Bool h_bounce_exists:1; /**< Flag indicating if 'h_bounce' parameter was provided. */
   Eina_Bool v_bounce:1; /**< Boolean flag for vertical bounce. */
   Eina_Bool v_bounce_exists:1; /**< Flag indicating if 'v_bounce' parameter was provided. */
} Elm_Params_Genlist;

/**
 * @brief Array of strings representing the available horizontal modes for Genlist.
 * The order of strings must match the Elm_List_Mode enum values.
 * Example: list_horizontal_choices[ELM_LIST_COMPRESS] == "compress"
 */
static const char* list_horizontal_choices[] =
{
   "compress", "scroll", "limit", "expand",
   NULL
};

/**
 * @brief Converts a string representation of horizontal mode to Elm_List_Mode enum.
 *
 * @param horizontal_str The string to convert (e.g., "scroll", "compress").
 * @return The corresponding Elm_List_Mode enum value, or ELM_LIST_LAST if not found.
 */
static Elm_List_Mode
_list_horizontal_setting_get(const char *horizontal_str)
{
   unsigned int i;

   assert(sizeof(list_horizontal_choices) / sizeof(list_horizontal_choices[0])
          == ELM_LIST_LAST + 1);

   for (i = 0; i < ELM_LIST_LAST; i++)
     {
        if (!strcmp(horizontal_str, list_horizontal_choices[i]))
          return i;
     }
   return ELM_LIST_LAST;
}

/**
 * @brief Applies Genlist parameters to an Evas_Object.
 * This function is called by Edje to set the state of an external Genlist object,
 * typically during animations or state transitions.
 *
 * @param data User data, unused in this function.
 * @param obj The Evas_Object (Genlist) to configure.
 * @param from_params The previous state's parameters (Elm_Params_Genlist).
 * @param to_params The target state's parameters (Elm_Params_Genlist).
 * @param pos The position in the transition (0.0 to 1.0), unused.
 */
static void
external_genlist_state_set(void *data EINA_UNUSED, Evas_Object *obj,
                           const void *from_params, const void *to_params,
                           float pos EINA_UNUSED)
{
   const Elm_Params_Genlist *p;

   if (to_params) p = to_params;
   else if (from_params) p = from_params;
   else return;

   if (p->horizontal)
     {
        Elm_List_Mode set = _list_horizontal_setting_get(p->horizontal);

        if (set != ELM_LIST_LAST)
          elm_genlist_mode_set(obj, set);
     }
   if (p->multi_exists)
     elm_genlist_multi_select_set(obj, p->multi);
   if (p->no_select_exists)
     {
        if (p->no_select)
          elm_genlist_select_mode_set (obj, ELM_OBJECT_SELECT_MODE_NONE);
        else
          elm_genlist_select_mode_set (obj, ELM_OBJECT_SELECT_MODE_DEFAULT);
     }
   if (p->always_select_exists)
     {
        if (p->always_select)
          elm_genlist_select_mode_set (obj, ELM_OBJECT_SELECT_MODE_ALWAYS);
        else
          elm_genlist_select_mode_set (obj, ELM_OBJECT_SELECT_MODE_DEFAULT);
     }
   if (p->homogeneous_exists)
     elm_genlist_homogeneous_set(obj, p->homogeneous);
   if ((p->h_bounce_exists) && (p->v_bounce_exists))
     elm_scroller_bounce_set(obj, p->h_bounce, p->v_bounce);
   else if ((p->h_bounce_exists) || (p->v_bounce_exists))
     {
        Eina_Bool h_bounce, v_bounce;

        elm_scroller_bounce_get(obj, &h_bounce, &v_bounce);
        if (p->h_bounce_exists)
          elm_scroller_bounce_set(obj, p->h_bounce, v_bounce);
        else
          elm_scroller_bounce_set(obj, h_bounce, p->v_bounce);
     }
}

/**
 * @brief Sets a specific Genlist parameter on an Evas_Object.
 * This function is called by Edje to set individual external parameters.
 *
 * @param data User data, unused in this function.
 * @param obj The Evas_Object (Genlist) to configure.
 * @param param The Edje_External_Param to apply.
 *              Example: param->name = "multi select", param->type = EDJE_EXTERNAL_PARAM_TYPE_BOOL, param->i = 1
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., unknown parameter or wrong type).
 */
static Eina_Bool
external_genlist_param_set(void *data EINA_UNUSED, Evas_Object *obj,
                           const Edje_External_Param *param)
{
   if (!strcmp(param->name, "horizontal mode"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_CHOICE)
          {
             Elm_List_Mode set = _list_horizontal_setting_get(param->s);

             if (set == ELM_LIST_LAST) return EINA_FALSE;
             elm_genlist_mode_set(obj, set);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "multi select"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             elm_genlist_multi_select_set(obj, param->i);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "always select"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             if (param->i)
               elm_genlist_select_mode_set (obj, ELM_OBJECT_SELECT_MODE_ALWAYS);
             else
               elm_genlist_select_mode_set (obj, ELM_OBJECT_SELECT_MODE_DEFAULT);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "no select"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             if (param->i)
               elm_genlist_select_mode_set (obj, ELM_OBJECT_SELECT_MODE_NONE);
             else
               elm_genlist_select_mode_set (obj, ELM_OBJECT_SELECT_MODE_DEFAULT);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "homogeneous"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             elm_genlist_homogeneous_set(obj, param->i);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "height bounce"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             Eina_Bool h_bounce, v_bounce;
             elm_scroller_bounce_get(obj, &h_bounce, &v_bounce);
             elm_scroller_bounce_set(obj, param->i, v_bounce);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "width bounce"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             Eina_Bool h_bounce, v_bounce;
             elm_scroller_bounce_get(obj, &h_bounce, &v_bounce);
             elm_scroller_bounce_set(obj, h_bounce, param->i);
             return EINA_TRUE;
          }
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Retrieves a specific Genlist parameter from an Evas_Object.
 * This function is called by Edje to get the current value of an external parameter.
 *
 * @param data User data, unused in this function.
 * @param obj The Evas_Object (Genlist) to query.
 * @param param An Edje_External_Param structure to fill with the parameter's value.
 *              The `name` and `type` fields are pre-filled by Edje.
 *              Example: param->name = "multi select", param->type = EDJE_EXTERNAL_PARAM_TYPE_BOOL.
 *                       This function would fill param->i.
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., unknown parameter or wrong type).
 */
static Eina_Bool
external_genlist_param_get(void *data EINA_UNUSED, const Evas_Object *obj,
                           Edje_External_Param *param)
{
   if (!strcmp(param->name, "horizontal mode"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_CHOICE)
          {
             Elm_List_Mode list_horizontal_set = elm_genlist_mode_get(obj);

             if (list_horizontal_set == ELM_LIST_LAST)
               return EINA_FALSE;

             param->s = list_horizontal_choices[list_horizontal_set];
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "multi select"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             param->i = elm_genlist_multi_select_get(obj);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "always select"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             if (elm_genlist_select_mode_get (obj) ==
                 ELM_OBJECT_SELECT_MODE_ALWAYS)
               param->i = EINA_TRUE;
             else
               param->i = EINA_FALSE;
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "no select"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             if (elm_genlist_select_mode_get (obj) ==
                 ELM_OBJECT_SELECT_MODE_NONE)
               param->i = EINA_TRUE;
             else
               param->i = EINA_FALSE;
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "homogeneous"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             param->i = elm_genlist_homogeneous_get(obj);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "height bounce"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             Eina_Bool h_bounce, v_bounce;
             elm_scroller_bounce_get(obj, &h_bounce, &v_bounce);
             param->i = h_bounce;
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "width bounce"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             Eina_Bool h_bounce, v_bounce;
             elm_scroller_bounce_get(obj, &h_bounce, &v_bounce);
             param->i = v_bounce;
             return EINA_TRUE;
          }
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Parses a list of Edje external parameters and stores them in an Elm_Params_Genlist structure.
 * This function is called by Edje when it encounters a block of parameters
 * for a Genlist object in an Edje file.
 *
 * @param data User data, unused in this function.
 * @param obj The Evas_Object (Genlist) these parameters are for, unused in this function.
 * @param params A list (Eina_List) of Edje_External_Param structures to parse.
 *               Example of an Edje_External_Param in the list:
 *               { name="multi select", type=EDJE_EXTERNAL_PARAM_TYPE_BOOL, i=1 }
 * @return A pointer to a newly allocated Elm_Params_Genlist structure filled with parsed values,
 *         or NULL on allocation failure. The caller is responsible for freeing this memory
 *         using external_genlist_params_free().
 */
static void *
external_genlist_params_parse(void *data EINA_UNUSED,
                              Evas_Object *obj EINA_UNUSED,
                              const Eina_List *params)
{
   Elm_Params_Genlist *mem;
   Edje_External_Param *param;
   const Eina_List *l;

   mem = ELM_NEW(Elm_Params_Genlist);
   if (!mem)
     return NULL;

   EINA_LIST_FOREACH(params, l, param)
     {
        if (!strcmp(param->name, "horizontal mode"))
          mem->horizontal = eina_stringshare_add(param->s);
        else if (!strcmp(param->name, "multi select"))
          {
             mem->multi = !!param->i;
             mem->multi_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "always select"))
          {
             mem->always_select = !!param->i;
             mem->always_select_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "no select"))
          {
             mem->no_select = !!param->i;
             mem->no_select_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "homogeneous"))
          {
             mem->homogeneous = !!param->i;
             mem->homogeneous_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "height bounce"))
          {
             mem->h_bounce = !!param->i;
             mem->h_bounce_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "width bounce"))
          {
             mem->v_bounce = !!param->i;
             mem->v_bounce_exists = EINA_TRUE;
          }
     }

   return mem;
}

/**
 * @brief Retrieves a content part from the Genlist.
 * Genlist typically does not expose named content parts via this mechanism.
 *
 * @param data User data, unused.
 * @param obj The Genlist object, unused.
 * @param content The name of the content part to retrieve, unused.
 * @return Always returns NULL for Genlist as it does not support this.
 */
static Evas_Object *external_genlist_content_get(void *data EINA_UNUSED,
                                                 const Evas_Object *obj EINA_UNUSED,
                                                 const char *content EINA_UNUSED)
{
   ERR("No content.");
   return NULL;
}

/**
 * @brief Frees the memory allocated for Elm_Params_Genlist.
 * This function is used to clean up the structure returned by
 * external_genlist_params_parse().
 *
 * @param params A pointer to the Elm_Params_Genlist structure to free.
 */
static void
external_genlist_params_free(void *params)
{
   Elm_Params_Genlist *mem = params;

   if (mem->horizontal)
     eina_stringshare_del(mem->horizontal);

   free(mem);
}

/**
 * @brief Describes the external parameters supported by the Genlist widget.
 * This array is used by Edje to understand what parameters can be set
 * on a Genlist object from an Edje file and their types.
 *
 * Example of elements in this array:
 * - DEFINE_EXTERNAL_COMMON_PARAMS: Macro that expands to common parameters like "visible", "disabled".
 * - EDJE_EXTERNAL_PARAM_INFO_CHOICE_FULL("horizontal mode", "scroll", list_horizontal_choices):
 *   Defines a parameter named "horizontal mode" which accepts one of the string values
 *   from `list_horizontal_choices`, with "scroll" being the default.
 * - EDJE_EXTERNAL_PARAM_INFO_BOOL("multi select"):
 *   Defines a boolean parameter named "multi select".
 * - EDJE_EXTERNAL_PARAM_INFO_SENTINEL: Marks the end of the parameter list.
 */
static Edje_External_Param_Info external_genlist_params[] = {
     DEFINE_EXTERNAL_COMMON_PARAMS,
     EDJE_EXTERNAL_PARAM_INFO_CHOICE_FULL("horizontal mode", "scroll",
                                          list_horizontal_choices),
     EDJE_EXTERNAL_PARAM_INFO_BOOL("multi select"),
     EDJE_EXTERNAL_PARAM_INFO_BOOL("always select"),
     EDJE_EXTERNAL_PARAM_INFO_BOOL("no select"),
     EDJE_EXTERNAL_PARAM_INFO_BOOL("homogeneous"),
     EDJE_EXTERNAL_PARAM_INFO_BOOL("height bounce"),
     EDJE_EXTERNAL_PARAM_INFO_BOOL("width bounce"),
     EDJE_EXTERNAL_PARAM_INFO_SENTINEL
};

DEFINE_EXTERNAL_ICON_ADD(genlist, "genlist");
DEFINE_EXTERNAL_TYPE_SIMPLE(genlist, "Generic List");
