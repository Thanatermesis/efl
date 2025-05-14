#include <assert.h>
#include "private.h"

/**
 * @brief Parameters for the Elm_Bg widget.
 *
 * This structure holds the configuration parameters for an Elm_Bg widget,
 * including the image file and display option.
 */
typedef struct _Elm_Params_Bg
{
   Elm_Params base; /**< Base parameters */
   const char *file; /**< Path to the background image file */
   const char *option; /**< Background display option (e.g., "center", "scale") */
} Elm_Params_Bg;

#define OPTION_GET(CHOICES, STR)                                \
   unsigned int i;                                              \
   for (i = 0; i < (sizeof(CHOICES) / sizeof(CHOICES[0])); ++i) \
     if (!strcmp(STR, CHOICES[i]))                              \
       return i;

/**
 * @brief Array of available background display options as strings.
 *
 * The order of options must match the Elm_Bg_Option enum.
 * The array is NULL-terminated.
 * Example: {"center", "scale", "stretch", "tile", NULL}
 */
static const char *_bg_options[] =
{
   "center", "scale", "stretch", "tile", NULL
};

/**
 * @brief Converts a background option string to an Elm_Bg_Option enum value.
 *
 * @param option The string representation of the background option (e.g., "center", "scale").
 * @return The corresponding Elm_Bg_Option enum value, or -1 if the option string is invalid.
 */
static Elm_Bg_Option
_bg_option_get(const char *option)
{
   assert(sizeof(_bg_options) / sizeof(_bg_options[0])
          == ELM_BG_OPTION_TILE + 2);
   OPTION_GET(_bg_options, option);
   return -1;
}

/**
 * @brief Sets the state of an external background object.
 *
 * This function is called by Edje to apply parameters to the background object
 * during state transitions.
 *
 * @param data Unused user data.
 * @param obj The Evas_Object (Elm_Bg) to modify.
 * @param from_params The parameters of the previous state (Elm_Params_Bg).
 * @param to_params The parameters of the target state (Elm_Params_Bg).
 * @param pos Unused position value for animations.
 */
static void
external_bg_state_set(void *data EINA_UNUSED, Evas_Object *obj,
                      const void *from_params, const void *to_params,
                      float pos EINA_UNUSED)
{
   const Elm_Params_Bg *p;
   Elm_Bg_Option option;

   if (to_params) p = to_params;
   else if (from_params) p = from_params;
   else return;

   if (p->option)
     {
        option = _bg_option_get(p->option);
        elm_bg_option_set(obj, option);
     }
   if (p->file)
     {
        elm_bg_file_set(obj, p->file, NULL);
     }
}

/**
 * @brief Sets a specific parameter for an external background object.
 *
 * This function is called by Edje to set individual parameters on the background object.
 * Supported parameters:
 * - "file": (string) Path to the background image.
 * - "select_mode": (string) Background display option (e.g., "center", "scale").
 *
 * @param data Unused user data.
 * @param obj The Evas_Object (Elm_Bg) to modify.
 * @param param The Edje_External_Param to set.
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., unknown parameter).
 */
static Eina_Bool
external_bg_param_set(void *data EINA_UNUSED, Evas_Object *obj,
                      const Edje_External_Param *param)
{
   if ((!strcmp(param->name, "file"))
       && (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING))
     {
        return elm_bg_file_set(obj, param->s, NULL);
     }
   else if ((!strcmp(param->name, "select_mode"))
            && (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING))
     {
        Elm_Bg_Option option;
        option = _bg_option_get(param->s);
        elm_bg_option_set(obj, option);
        return EINA_TRUE;
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Gets a specific parameter from an external background object.
 *
 * This function is called by Edje to retrieve individual parameters from the background object.
 * Supported parameters:
 * - "file": (string) Path to the background image.
 * - "option": (string) Background display option (e.g., "center", "scale").
 *
 * @param data Unused user data.
 * @param obj The Evas_Object (Elm_Bg) to query.
 * @param param The Edje_External_Param to fill with the retrieved value.
 *              The `name` field indicates which parameter to get.
 *              The `s` field will be set for string parameters.
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., unknown parameter).
 */
static Eina_Bool
external_bg_param_get(void *data EINA_UNUSED,
                      const Evas_Object *obj EINA_UNUSED,
                      Edje_External_Param *param)
{
   if ((!strcmp(param->name, "file"))
       && (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING))
     {
        elm_bg_file_get(obj, &(param->s), NULL);
        return EINA_TRUE;
     }
   else if ((!strcmp(param->name, "option"))
            && (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING))
     {
        Elm_Bg_Option option;
        option = elm_bg_option_get(obj);
        param->s = _bg_options[option];
        return EINA_TRUE;
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Parses a list of Edje external parameters and creates an Elm_Params_Bg structure.
 *
 * This function is called by Edje to convert a list of parameters from an EDC file
 * into a custom structure (Elm_Params_Bg) for the background widget.
 *
 * @param data Unused user data.
 * @param obj Unused Evas_Object.
 * @param params A list of Edje_External_Param structures to parse.
 *               Example of params list structure:
 *               Eina_List* containing Edje_External_Param* elements.
 *               Each Edje_External_Param might look like:
 *               { .name = "file", .type = EDJE_EXTERNAL_PARAM_TYPE_STRING, .s = "/path/to/image.png" }
 *               { .name = "option", .type = EDJE_EXTERNAL_PARAM_TYPE_STRING, .s = "scale" }
 * @return A pointer to a newly allocated Elm_Params_Bg structure, or NULL on failure.
 *         The caller is responsible for freeing this memory using external_bg_params_free().
 */
static void *
external_bg_params_parse(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                         const Eina_List *params)
{
   Elm_Params_Bg *mem;
   Edje_External_Param *param;
   const Eina_List *l;

   mem = calloc(1, sizeof(Elm_Params_Bg));
   if (!mem)
     return NULL;

   EINA_LIST_FOREACH(params, l, param)
     {
        if (!strcmp(param->name, "file"))
          mem->file = eina_stringshare_add(param->s);
        else if (!strcmp(param->name, "option"))
          mem->option = eina_stringshare_add(param->s);
     }

   return mem;
}

/**
 * @brief Retrieves a content part from an external background object.
 *
 * Elm_Bg does not support named content parts, so this function always
 * logs an error and returns NULL.
 *
 * @param data Unused user data.
 * @param obj Unused Evas_Object.
 * @param content Unused name of the content part.
 * @return Always NULL.
 */
static Evas_Object *
external_bg_content_get(void *data EINA_UNUSED,
                        const Evas_Object *obj EINA_UNUSED,
                        const char *content EINA_UNUSED)
{
   ERR("no content");
   return NULL;
}

/**
 * @brief Frees the memory allocated for Elm_Params_Bg.
 *
 * This function is called by Edje to release the memory allocated by
 * external_bg_params_parse().
 *
 * @param params A pointer to the Elm_Params_Bg structure to free.
 */
static void
external_bg_params_free(void *params)
{
   Elm_Params_Bg *mem = params;

   if (mem->file)
     eina_stringshare_del(mem->file);

   if (mem->option)
     eina_stringshare_del(mem->option);

   free(mem);
}

/**
 * @brief Information about the external parameters supported by Elm_Bg.
 *
 * This array defines the names and types of parameters that can be used
 * with Elm_Bg in an Edje theme (EDC file).
 * Example of elements in this array:
 *   { "file", EDJE_EXTERNAL_PARAM_TYPE_STRING, ... }
 *   { "option", EDJE_EXTERNAL_PARAM_TYPE_STRING, ... }
 */
static Edje_External_Param_Info external_bg_params[] =
{
   DEFINE_EXTERNAL_COMMON_PARAMS,
   EDJE_EXTERNAL_PARAM_INFO_STRING("file"),
   EDJE_EXTERNAL_PARAM_INFO_STRING("option"),
   EDJE_EXTERNAL_PARAM_INFO_SENTINEL
};

DEFINE_EXTERNAL_ICON_ADD(bg, "bg");
DEFINE_EXTERNAL_TYPE_SIMPLE(bg, "Bg");
