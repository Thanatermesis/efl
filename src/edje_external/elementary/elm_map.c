#include <assert.h>
#include "private.h"

/**
 * @brief Structure to hold parameters for the Elm_Map widget.
 * This structure is used for external parameter handling, allowing
 * Edje to configure the map widget.
 */
typedef struct _Elm_Params_Map
{
   Elm_Params base; /**< Base parameters, common to all Elm widgets */
   const char *map_source; /**< Name of the map source to use (e.g., "Mapnik") */
   const char *zoom_mode; /**< String representation of the zoom mode (e.g., "manual", "auto fit") */
   double zoom; /**< Zoom level, used when zoom_mode is "manual" */
   Eina_Bool zoom_set:1; /**< Flag indicating if the zoom level has been explicitly set */
} Elm_Params_Map;

/**
 * @brief Array of strings representing the available zoom modes for the map.
 * The order of these strings corresponds to the Elm_Map_Zoom_Mode enum values.
 * The array is NULL-terminated.
 * Example: `zoom_choices[0]` is "manual".
 */
static const char *zoom_choices[] = { "manual", "auto fit", "auto fill", NULL };

/**
 * @brief Array of strings representing the available map sources.
 * These are the human-readable names for different tile providers.
 * Example: `source_choices[0]` is "Mapnik".
 */
static const char *source_choices[] =
{
   "Mapnik", "Osmarender", "CycleMap", "Maplint"
};

/**
 * @brief Converts a string representation of a zoom mode to its Elm_Map_Zoom_Mode enum value.
 *
 * @param map_src The string representation of the zoom mode (e.g., "manual", "auto fit").
 * @return The corresponding Elm_Map_Zoom_Mode enum value.
 *         Returns ELM_MAP_ZOOM_MODE_LAST if the string does not match any known zoom mode.
 */
static Elm_Map_Zoom_Mode
_zoom_mode_get(const char *map_src)
{
   unsigned int i;

   assert(sizeof(zoom_choices)/sizeof(zoom_choices[0]) ==
          ELM_MAP_ZOOM_MODE_LAST + 1);

   for (i = 0; i < ELM_MAP_ZOOM_MODE_LAST; i++)
     if (!strcmp(map_src, zoom_choices[i])) return i;

   return ELM_MAP_ZOOM_MODE_LAST;
}

/**
 * @brief Sets the state of the map widget based on external parameters.
 * This function is typically called during animations or state transitions
 * managed by Edje. It applies the map source, zoom mode, and zoom level.
 *
 * @param data Unused user data.
 * @param obj The Evas_Object (map widget) to configure.
 * @param from_params The previous state's Elm_Params_Map structure (can be NULL).
 * @param to_params The target state's Elm_Params_Map structure (can be NULL).
 * @param pos Unused position value for animations.
 */
static void
external_map_state_set(void *data EINA_UNUSED, Evas_Object *obj,
                       const void *from_params, const void *to_params,
                       float pos EINA_UNUSED)
{
   const Elm_Params_Map *p;

   if (to_params) p = to_params;
   else if (from_params) p = from_params;
   else return;

   if (p->map_source)
     {
        elm_map_source_set(obj, ELM_MAP_SOURCE_TYPE_TILE, p->map_source);
     }
   if (p->zoom_mode)
     {
        Elm_Map_Zoom_Mode set = _zoom_mode_get(p->zoom_mode);
        if (set == ELM_MAP_ZOOM_MODE_LAST) return;
        elm_map_zoom_mode_set(obj, set);
     }
   if (p->zoom_set) elm_map_zoom_set(obj, p->zoom);
}

/**
 * @brief Sets a single external parameter on the map widget.
 * This function is called by Edje when a specific parameter of the
 * external object needs to be changed.
 *
 * @param data Unused user data.
 * @param obj The Evas_Object (map widget) to configure.
 * @param param The Edje_External_Param containing the name and value of the parameter to set.
 *              Example for `param->name`: "map source", `param->s`: "Mapnik".
 *              Example for `param->name`: "zoom level", `param->d`: 10.0.
 * @return EINA_TRUE if the parameter was successfully set, EINA_FALSE otherwise.
 */
static Eina_Bool
external_map_param_set(void *data EINA_UNUSED, Evas_Object *obj,
                       const Edje_External_Param *param)
{
   if (!strcmp(param->name, "map source"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_CHOICE)
          {
             elm_map_source_set(obj, ELM_MAP_SOURCE_TYPE_TILE, param->s);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "zoom mode"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_CHOICE)
          {
             Elm_Map_Zoom_Mode set = _zoom_mode_get(param->s);
             if (set == ELM_MAP_ZOOM_MODE_LAST) return EINA_FALSE;
             elm_map_zoom_mode_set(obj, set);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "zoom level"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_DOUBLE)
          {
             elm_map_zoom_set(obj, param->d);
             return EINA_TRUE;
          }
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Gets a single external parameter from the map widget.
 * This function is called by Edje to retrieve the current value of
 * a specific parameter of the external object.
 *
 * @param data Unused user data.
 * @param obj The Evas_Object (map widget) from which to get the parameter.
 * @param param An Edje_External_Param structure to be filled with the parameter's
 *              name (input) and value (output).
 *              Example for `param->name` (input): "map source".
 *              `param->s` (output) will be set to the current map source string.
 * @return EINA_TRUE if the parameter was successfully retrieved, EINA_FALSE otherwise.
 */
static Eina_Bool
external_map_param_get(void *data EINA_UNUSED, const Evas_Object *obj,
                       Edje_External_Param *param)
{
   if (!strcmp(param->name, "map source"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_CHOICE)
          {
             const char *set = elm_map_source_get(obj, ELM_MAP_SOURCE_TYPE_TILE);
             param->s = set;
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "zoom mode"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_CHOICE)
          {
             Elm_Map_Zoom_Mode set = elm_map_zoom_mode_get(obj);
             if (set == ELM_MAP_ZOOM_MODE_LAST) return EINA_FALSE;
             param->s = zoom_choices[set];
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "zoom level"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_DOUBLE)
          {
             param->d = elm_map_zoom_get(obj);
             return EINA_TRUE;
          }
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Parses a list of Edje external parameters and creates an Elm_Params_Map structure.
 * This function is called by Edje to convert a list of parameters from an EDC
 * theme file into a custom structure that can be used by `external_map_state_set`.
 *
 * @param data Unused user data.
 * @param obj Unused Evas_Object.
 * @param params A list of Edje_External_Param structures to parse.
 *               Each element in the list is an Edje_External_Param.
 *               Example `params` element: `param->name` = "zoom level", `param->d` = 12.0.
 * @return A pointer to a newly allocated Elm_Params_Map structure filled with the parsed
 *         parameters. Returns NULL on allocation failure. The caller is responsible
 *         for freeing this memory using `external_map_params_free`.
 */
static void *
external_map_params_parse(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                          const Eina_List *params)
{
   Elm_Params_Map *mem;
   Edje_External_Param *param;
   const Eina_List *l;

   mem = calloc(1, sizeof(Elm_Params_Map));
   if (!mem)
     return NULL;

   EINA_LIST_FOREACH(params, l, param)
     {
        if (!strcmp(param->name, "map source"))
          mem->map_source = eina_stringshare_add(param->s);
        if (!strcmp(param->name, "zoom mode"))
          mem->zoom_mode = eina_stringshare_add(param->s);
        else if (!strcmp(param->name, "zoom level"))
          {
             mem->zoom = param->d;
             mem->zoom_set = EINA_TRUE;
          }
     }

   return mem;
}

/**
 * @brief Retrieves content from the map widget.
 * This function is intended to allow Edje to access named content parts
 * within the external object. For Elm_Map, this is not implemented
 * as it doesn't expose named content parts in this manner.
 *
 * @param data Unused user data.
 * @param obj Unused Evas_Object.
 * @param content Unused name of the content part to retrieve.
 * @return Always returns NULL for Elm_Map, as no content parts are exposed.
 */
static Evas_Object *external_map_content_get(void *data EINA_UNUSED,
                                             const Evas_Object *obj EINA_UNUSED,
                                             const char *content EINA_UNUSED)
{
   ERR("No content.");
   return NULL;
}

/**
 * @brief Frees the memory allocated for an Elm_Params_Map structure.
 * This function is used to clean up the structure created by
 * `external_map_params_parse`.
 *
 * @param params A pointer to the Elm_Params_Map structure to free.
 */
static void
external_map_params_free(void *params)
{
   Elm_Params_Map *mem = params;

   if (mem->map_source)
     eina_stringshare_del(mem->map_source);
   if (mem->zoom_mode)
     eina_stringshare_del(mem->zoom_mode);
   free(mem);
}

/**
 * @brief Defines the external parameters exposed by the Elm_Map widget to Edje.
 * This array provides metadata about each parameter, including its name, type,
 * and default values or choices. This information is used by tools like
 * edje_cc and edje_edit to understand and configure the external object.
 *
 * Structure of elements:
 * - `DEFINE_EXTERNAL_COMMON_PARAMS`: Macro that defines common parameters like "visible".
 * - `EDJE_EXTERNAL_PARAM_INFO_CHOICE_FULL("param name", "default value", string_array_of_choices)`:
 *   Defines a parameter that can be chosen from a list of strings.
 *   Example: `EDJE_EXTERNAL_PARAM_INFO_CHOICE_FULL("map source", "Mapnik", source_choices)`
 *     - "map source": The name of the parameter.
 *     - "Mapnik": The default value for this parameter.
 *     - `source_choices`: An array of const char* strings for available options.
 * - `EDJE_EXTERNAL_PARAM_INFO_DOUBLE("param name")`: Defines a floating-point parameter.
 *   Example: `EDJE_EXTERNAL_PARAM_INFO_DOUBLE("zoom level")`
 *     - "zoom level": The name of the parameter.
 * - `EDJE_EXTERNAL_PARAM_INFO_SENTINEL`: Marks the end of the parameter list.
 */
static Edje_External_Param_Info external_map_params[] =
{
   DEFINE_EXTERNAL_COMMON_PARAMS,
   EDJE_EXTERNAL_PARAM_INFO_CHOICE_FULL("map source", "Mapnik", source_choices),
   EDJE_EXTERNAL_PARAM_INFO_CHOICE_FULL("zoom mode", "manual", zoom_choices),
   EDJE_EXTERNAL_PARAM_INFO_DOUBLE("zoom level"),
   EDJE_EXTERNAL_PARAM_INFO_SENTINEL
};

DEFINE_EXTERNAL_ICON_ADD(map, "map");
DEFINE_EXTERNAL_TYPE_SIMPLE(map, "Map");
