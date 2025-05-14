#include <assert.h>

#include "private.h"

/**
 * @brief Structure to hold parameters for an Elm_Toolbar widget.
 *
 * This structure is used to parse and store parameters from an Edje external
 * interface, allowing themes to configure toolbar properties.
 */
typedef struct _Elm_Params_Toolbar
{
   Elm_Params base; /**< Base parameters, common to all Elm widgets. */
   int icon_size; /**< The size of icons in the toolbar. */
   Eina_Bool icon_size_exists:1; /**< Flag indicating if icon_size was set. */
   double align; /**< The alignment of the toolbar items (0.0 to 1.0). */
   const char *shrink_mode; /**< String representation of the shrink mode. e.g., "none", "hide", "scroll", "menu". */
   Eina_Bool align_exists:1; /**< Flag indicating if align was set. */
   Eina_Bool always_select:1; /**< Flag for ELM_OBJECT_SELECT_MODE_ALWAYS. */
   Eina_Bool always_select_exists:1; /**< Flag indicating if always_select was set. */
   Eina_Bool no_select:1; /**< Flag for ELM_OBJECT_SELECT_MODE_NONE. */
   Eina_Bool no_select_exists:1; /**< Flag indicating if no_select was set. */
   Eina_Bool horizontal:1; /**< Flag indicating if the toolbar is horizontal. */
   Eina_Bool horizontal_exists:1; /**< Flag indicating if horizontal was set. */
   Eina_Bool homogeneous:1; /**< Flag indicating if the toolbar items are homogeneous. */
   Eina_Bool homogeneous_exists:1; /**< Flag indicating if homogeneous was set. */
} Elm_Params_Toolbar;

#define SHRINK_GET(CHOICES, STR)         \
   unsigned int i;                       \
   for (i = 0; i < (sizeof(CHOICES) / sizeof(CHOICES[0])); ++i) \
     if (!strcmp(STR, CHOICES[i]))       \
       return i;

/**
 * @brief Array of strings representing the available toolbar shrink modes.
 *
 * This array maps string identifiers to Elm_Toolbar_Shrink_Mode enum values.
 * The order must match the Elm_Toolbar_Shrink_Mode enum definition.
 * Example: _toolbar_shrink_modes[ELM_TOOLBAR_SHRINK_NONE] == "none"
 */
static const char *_toolbar_shrink_modes[] =
{
   "none", "hide", "scroll", "menu", NULL
};

/**
 * @brief Converts a string representation of a shrink mode to its enum value.
 *
 * @param shrink_mode_str The string representation of the shrink mode (e.g., "scroll").
 * @return The corresponding Elm_Toolbar_Shrink_Mode enum value.
 *         Returns ELM_TOOLBAR_SHRINK_LAST if the string is not recognized.
 */
static Elm_Toolbar_Shrink_Mode
_toolbar_shrink_choices_setting_get(const char *shrink_mode_str)
{
   assert(sizeof(_toolbar_shrink_modes) /
          sizeof(_toolbar_shrink_modes[0]) == ELM_TOOLBAR_SHRINK_LAST + 1);
   SHRINK_GET(_toolbar_shrink_modes, shrink_mode_str);
   return ELM_TOOLBAR_SHRINK_LAST;
}

/**
 * @brief Sets the state of an Elm_Toolbar object based on external parameters.
 *
 * This function is called by Edje to apply theme-defined states to the toolbar.
 * It transitions the toolbar's properties from `from_params` to `to_params`.
 *
 * @param data User data (unused).
 * @param obj The Elm_Toolbar Evas_Object to modify.
 * @param from_params The Elm_Params_Toolbar structure representing the initial state.
 * @param to_params The Elm_Params_Toolbar structure representing the target state.
 * @param pos The position in the transition (unused).
 */
static void
external_toolbar_state_set(void *data EINA_UNUSED, Evas_Object *obj,
                           const void *from_params, const void *to_params,
                           float pos EINA_UNUSED)
{
   const Elm_Params_Toolbar *p;
   Elm_Toolbar_Shrink_Mode shrink_mode;

   if (to_params) p = to_params;
   else if (from_params) p = from_params;
   else return;

   if (p->icon_size_exists)
     elm_toolbar_icon_size_set(obj, p->icon_size);
   if (p->align_exists)
     elm_toolbar_align_set(obj, p->align);
   if (p->no_select_exists)
     {
        if (p->no_select)
          elm_toolbar_select_mode_set (obj, ELM_OBJECT_SELECT_MODE_NONE);
        else
          elm_toolbar_select_mode_set (obj, ELM_OBJECT_SELECT_MODE_DEFAULT);
     }
   if (p->always_select_exists)
     {
        if (p->always_select)
          elm_toolbar_select_mode_set (obj, ELM_OBJECT_SELECT_MODE_ALWAYS);
        else
          elm_toolbar_select_mode_set (obj, ELM_OBJECT_SELECT_MODE_DEFAULT);
     }
   if (p->horizontal_exists)
     elm_toolbar_horizontal_set(obj, p->horizontal);
   if (p->homogeneous_exists)
     elm_toolbar_homogeneous_set(obj, p->homogeneous);
   if (p->shrink_mode)
     {
        shrink_mode = _toolbar_shrink_choices_setting_get(p->shrink_mode);
        elm_toolbar_shrink_mode_set(obj, shrink_mode);
     }
}

/**
 * @brief Sets a single parameter on an Elm_Toolbar object.
 *
 * This function is called by Edje to set individual properties of the toolbar
 * based on external parameter definitions in the theme.
 *
 * @param data User data (unused).
 * @param obj The Elm_Toolbar Evas_Object to modify.
 * @param param The Edje_External_Param to apply.
 *              Example: param->name = "icon size", param->type = EDJE_EXTERNAL_PARAM_TYPE_INT, param->i = 24
 * @return EINA_TRUE if the parameter was successfully set, EINA_FALSE otherwise.
 */
static Eina_Bool
external_toolbar_param_set(void *data EINA_UNUSED, Evas_Object *obj,
                           const Edje_External_Param *param)
{
   Elm_Toolbar_Shrink_Mode shrink_mode;

   if (!strcmp(param->name, "icon size"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_INT)
          {
             elm_toolbar_icon_size_set(obj, param->i);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "align"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_DOUBLE)
          {
             elm_toolbar_align_set(obj, param->d);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "always select"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             if (param->i)
               elm_toolbar_select_mode_set (obj, ELM_OBJECT_SELECT_MODE_ALWAYS);
             else
               elm_toolbar_select_mode_set (obj, ELM_OBJECT_SELECT_MODE_DEFAULT);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "no select"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             if (param->i)
               elm_toolbar_select_mode_set (obj, ELM_OBJECT_SELECT_MODE_NONE);
             else
               elm_toolbar_select_mode_set (obj, ELM_OBJECT_SELECT_MODE_DEFAULT);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "horizontal"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             elm_toolbar_horizontal_set(obj, param->i);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "homogeneous"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             elm_toolbar_homogeneous_set(obj, param->i);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "shrink"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
          {
             shrink_mode = _toolbar_shrink_choices_setting_get(param->s);
             elm_toolbar_shrink_mode_set(obj, shrink_mode);
             return EINA_TRUE;
          }
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Gets a single parameter from an Elm_Toolbar object.
 *
 * This function is called by Edje to retrieve the current value of a specific
 * property of the toolbar.
 *
 * @param data User data (unused).
 * @param obj The Elm_Toolbar Evas_Object to query.
 * @param param An Edje_External_Param structure to fill with the parameter's value.
 *              The `name` and `type` fields of `param` indicate which parameter to get.
 *              Example: param->name = "icon size", param->type = EDJE_EXTERNAL_PARAM_TYPE_INT
 *                       On success, param->i will be set to the current icon size.
 * @return EINA_TRUE if the parameter was successfully retrieved, EINA_FALSE otherwise.
 */
static Eina_Bool
external_toolbar_param_get(void *data EINA_UNUSED, const Evas_Object *obj,
                           Edje_External_Param *param)
{
   if (!strcmp(param->name, "icon size"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_INT)
          {
             param->i = elm_toolbar_icon_size_get(obj);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "align"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_DOUBLE)
          {
             param->d = elm_toolbar_align_get(obj);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "always select"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             if (elm_toolbar_select_mode_get (obj) ==
                 ELM_OBJECT_SELECT_MODE_ALWAYS)
               param->d = EINA_TRUE;
             else
               param->d = EINA_FALSE;
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "no select"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             if (elm_toolbar_select_mode_get (obj) ==
                 ELM_OBJECT_SELECT_MODE_NONE)
               param->i = EINA_TRUE;
             else
               param->i = EINA_FALSE;
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "horizontal"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             param->i = elm_toolbar_horizontal_get(obj);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "homogeneous"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             param->i = elm_toolbar_homogeneous_get(obj);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "shrink"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
          {
             Elm_Toolbar_Shrink_Mode shrink_mode;
             shrink_mode = elm_toolbar_shrink_mode_get(obj);
             param->s = _toolbar_shrink_modes[shrink_mode];
             return EINA_TRUE;
          }
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Parses a list of Edje external parameters and creates an Elm_Params_Toolbar structure.
 *
 * This function is called by Edje to convert a list of theme-defined parameters
 * into a structured format that can be used by `external_toolbar_state_set`.
 *
 * @param data User data (unused).
 * @param obj The Evas_Object (unused in this function, but part of the Edje API).
 * @param params A list of Edje_External_Param structures to parse.
 *               Example of `params` list elements:
 *               - Edje_External_Param { name="icon size", type=EDJE_EXTERNAL_PARAM_TYPE_INT, i=32 }
 *               - Edje_External_Param { name="shrink", type=EDJE_EXTERNAL_PARAM_TYPE_STRING, s="scroll" }
 * @return A pointer to a newly allocated Elm_Params_Toolbar structure, or NULL on failure.
 *         The caller is responsible for freeing this memory using `external_toolbar_params_free`.
 */
static void *
external_toolbar_params_parse(void *data EINA_UNUSED,
                              Evas_Object *obj EINA_UNUSED,
                              const Eina_List *params)
{
   Elm_Params_Toolbar *mem;
   Edje_External_Param *param;
   const Eina_List *l;

   mem = calloc(1, sizeof(Elm_Params_Toolbar));
   if (!mem)
     return NULL;

   EINA_LIST_FOREACH(params, l, param)
     {
        if (!strcmp(param->name, "icon size"))
          {
             mem->icon_size = param->i;
             mem->icon_size_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "align"))
          {
             mem->align = param->d;
             mem->align_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "always select"))
          {
             mem->always_select = param->i;
             mem->always_select_exists = param->i;
          }
        else if (!strcmp(param->name, "no select"))
          {
             mem->no_select = param->i;
             mem->no_select_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "horizontal"))
          {
             mem->horizontal = param->i;
             mem->horizontal_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "homogeneous"))
          {
             mem->homogeneous = param->i;
             mem->homogeneous_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "shrink"))
          mem->shrink_mode = eina_stringshare_add(param->s);
     }

   return mem;
}

/**
 * @brief Retrieves a content part of the Elm_Toolbar.
 *
 * This function is part of the Edje external interface but is not
 * typically used for toolbars as they manage their items internally.
 * Currently, it always returns NULL and logs an error.
 *
 * @param data User data (unused).
 * @param obj The Evas_Object (unused).
 * @param content The name of the content part to retrieve (unused).
 * @return Always NULL for Elm_Toolbar.
 */
static Evas_Object *external_toolbar_content_get(void *data EINA_UNUSED,
                                                 const Evas_Object *obj EINA_UNUSED,
                                                 const char *content EINA_UNUSED)
{
   ERR("No content.");
   return NULL;
}

/**
 * @brief Frees the memory allocated for Elm_Params_Toolbar.
 *
 * This function is used to clean up the structure returned by
 * `external_toolbar_params_parse`.
 *
 * @param params A pointer to the Elm_Params_Toolbar structure to free.
 */
static void
external_toolbar_params_free(void *params)
{
   Elm_Params_Toolbar *mem = params;
   if (mem->shrink_mode)
     eina_stringshare_del(mem->shrink_mode);
   free(mem);
}

/**
 * @brief Defines the external parameters recognized by the Elm_Toolbar widget.
 *
 * This array provides metadata about the parameters that can be set on a
 * toolbar from an Edje theme. It is used by Edje to validate and handle
 * these parameters.
 *
 * The structure of elements is defined by Edje_External_Param_Info:
 * - DEFINE_EXTERNAL_COMMON_PARAMS: Macro for common parameters like "visible", "disabled".
 * - EDJE_EXTERNAL_PARAM_INFO_STRING("shrink"): Defines a string parameter named "shrink".
 *   Example value: "scroll"
 * - EDJE_EXTERNAL_PARAM_INFO_INT("icon size"): Defines an integer parameter named "icon size".
 *   Example value: 32
 * - EDJE_EXTERNAL_PARAM_INFO_DOUBLE("align"): Defines a double parameter named "align".
 *   Example value: 0.5
 * - EDJE_EXTERNAL_PARAM_INFO_BOOL("always select"): Defines a boolean parameter named "always select".
 *   Example value: 1 (true) or 0 (false)
 * - ... and so on for other parameters.
 * - EDJE_EXTERNAL_PARAM_INFO_SENTINEL: Marks the end of the parameter list.
 */
static Edje_External_Param_Info external_toolbar_params[] = {
   DEFINE_EXTERNAL_COMMON_PARAMS,
   EDJE_EXTERNAL_PARAM_INFO_STRING("shrink"),
   EDJE_EXTERNAL_PARAM_INFO_INT("icon size"),
   EDJE_EXTERNAL_PARAM_INFO_DOUBLE("align"),
   EDJE_EXTERNAL_PARAM_INFO_BOOL("always select"),
   EDJE_EXTERNAL_PARAM_INFO_BOOL("no select"),
   EDJE_EXTERNAL_PARAM_INFO_BOOL("horizontal"),
   EDJE_EXTERNAL_PARAM_INFO_BOOL("homogeneous"),

   EDJE_EXTERNAL_PARAM_INFO_SENTINEL
};

DEFINE_EXTERNAL_ICON_ADD(toolbar, "toolbar");
DEFINE_EXTERNAL_TYPE_SIMPLE(toolbar, "Toolbar");
