#include <assert.h>
#include "private.h"

/**
 * @brief Structure to hold the parameters for the photocam widget.
 * This structure is used for state saving and restoration.
 */
typedef struct _Elm_Params_Photocam
{
   Elm_Params base; /**< Base parameters */
   const char *file; /**< The path to the image file to be displayed */
   double zoom; /**< The zoom level of the photocam */
   const char *zoom_mode; /**< The zoom mode as a string (e.g., "manual", "auto fit") */
   Eina_Bool paused:1; /**< Boolean indicating if the photocam animations are paused */
   Eina_Bool paused_exists:1; /**< Flag indicating if the paused parameter is set */
   Eina_Bool zoom_exists:1; /**< Flag indicating if the zoom parameter is set */
} Elm_Params_Photocam;

/**
 * @brief Array of strings representing the available zoom modes.
 * The order of strings corresponds to the Elm_Photocam_Zoom_Mode enum.
 * The array is NULL-terminated.
 * Example: `choices[0]` is "manual", `choices[1]` is "auto fit".
 */
static const char* choices[] = { "manual", "auto fit", "auto fill", NULL };

/**
 * @brief Converts a zoom mode string to an Elm_Photocam_Zoom_Mode enum value.
 *
 * @param zoom_mode_str The string representation of the zoom mode.
 *                      Example: "manual", "auto fit", "auto fill".
 * @return The corresponding Elm_Photocam_Zoom_Mode enum value,
 *         or ELM_PHOTOCAM_ZOOM_MODE_LAST if the string is not recognized.
 */
static Elm_Photocam_Zoom_Mode
_zoom_mode_setting_get(const char *zoom_mode_str)
{
   unsigned int i;

   assert(sizeof(choices)/sizeof(choices[0]) == ELM_PHOTOCAM_ZOOM_MODE_LAST + 1);

   for (i = 0; i < ELM_PHOTOCAM_ZOOM_MODE_LAST; i++)
     {
        if (!strcmp(zoom_mode_str, choices[i]))
          return i;
     }
   return ELM_PHOTOCAM_ZOOM_MODE_LAST;
}

/**
 * @brief Sets the state of the photocam widget from parameters.
 * This function is typically called during widget state restoration.
 *
 * @param data Unused.
 * @param obj The photocam Evas_Object.
 * @param from_params The previous state parameters (can be NULL).
 * @param to_params The new state parameters to apply (can be NULL).
 * @param pos Unused.
 */
static void
external_photocam_state_set(void *data EINA_UNUSED, Evas_Object *obj,
                            const void *from_params, const void *to_params,
                            float pos EINA_UNUSED)
{
   const Elm_Params_Photocam *p;

   if (to_params) p = to_params;
   else if (from_params) p = from_params;
   else return;

   if (p->file)
     elm_photocam_file_set(obj, p->file);
   if (p->zoom_exists)
     elm_photocam_zoom_set(obj, p->zoom);
   if (p->zoom_mode)
     {
        Elm_Photocam_Zoom_Mode set = _zoom_mode_setting_get(p->zoom_mode);
        if (set == ELM_PHOTOCAM_ZOOM_MODE_LAST) return;
        elm_photocam_zoom_mode_set(obj, set);
     }
   if (p->paused_exists)
     elm_photocam_paused_set(obj, p->paused);
}

/**
 * @brief Sets a specific external parameter for the photocam widget.
 * This function is called by Edje to set individual properties.
 *
 * @param data Unused.
 * @param obj The photocam Evas_Object.
 * @param param The Edje_External_Param to set.
 *              Example for param->name: "file", "zoom", "zoom mode", "paused".
 *              Example for param->type: EDJE_EXTERNAL_PARAM_TYPE_STRING for "file".
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., unknown parameter or wrong type).
 */
static Eina_Bool
external_photocam_param_set(void *data EINA_UNUSED, Evas_Object *obj,
                            const Edje_External_Param *param)
{
   if (!strcmp(param->name, "file"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
          {
             elm_photocam_file_set(obj, param->s);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "zoom"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_DOUBLE)
          {
             elm_photocam_zoom_set(obj, param->d);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "zoom mode"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
          {
             Elm_Photocam_Zoom_Mode set = _zoom_mode_setting_get(param->s);
             if (set == ELM_PHOTOCAM_ZOOM_MODE_LAST) return EINA_FALSE;
             elm_photocam_zoom_mode_set(obj, set);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "paused"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             elm_photocam_paused_set(obj, param->i);
             return EINA_TRUE;
          }
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Gets a specific external parameter from the photocam widget.
 * This function is called by Edje to retrieve individual properties.
 *
 * @param data Unused.
 * @param obj The photocam Evas_Object.
 * @param param The Edje_External_Param to fill with the retrieved value.
 *              The `param->name` field indicates which parameter to get.
 *              Example for param->name: "file", "zoom".
 *              The `param->type` field indicates the expected type.
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., unknown parameter or wrong type).
 */
static Eina_Bool
external_photocam_param_get(void *data EINA_UNUSED, const Evas_Object *obj,
                            Edje_External_Param *param)
{
   if (!strcmp(param->name, "file"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
          {
             param->s = elm_photocam_file_get(obj);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "zoom"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_DOUBLE)
          {
             param->d = elm_photocam_zoom_get(obj);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "zoom mode"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
          {
             Elm_Photocam_Zoom_Mode zoom_mode_set =
                elm_photocam_zoom_mode_get(obj);

             if (zoom_mode_set == ELM_PHOTOCAM_ZOOM_MODE_LAST)
               return EINA_FALSE;

             param->s = choices[zoom_mode_set];
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "paused"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             param->i = elm_photocam_paused_get(obj);
             return EINA_TRUE;
          }
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Parses a list of Edje_External_Param and stores them in an Elm_Params_Photocam structure.
 * This is used to convert a list of parameters (e.g., from an EDC file) into a structured format.
 *
 * @param data Unused.
 * @param obj Unused.
 * @param params A list of Edje_External_Param structures.
 *               Each element in the list is an Edje_External_Param*.
 *               Example: A list might contain params for "file", "zoom", etc.
 * @return A newly allocated Elm_Params_Photocam structure filled with the parsed parameters,
 *         or NULL on allocation failure. The caller is responsible for freeing this memory
 *         using external_photocam_params_free().
 */
static void *
external_photocam_params_parse(void *data EINA_UNUSED,
                               Evas_Object *obj EINA_UNUSED,
                               const Eina_List *params)
{
   Elm_Params_Photocam *mem;
   Edje_External_Param *param;
   const Eina_List *l;

   mem = calloc(1, sizeof(Elm_Params_Photocam));
   if (!mem)
     return NULL;

   EINA_LIST_FOREACH(params, l, param)
     {
        if (!strcmp(param->name, "file"))
          mem->file = eina_stringshare_add(param->s);
        else if (!strcmp(param->name, "zoom"))
          {
             mem->zoom = param->d;
             mem->zoom_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "zoom mode"))
          mem->zoom_mode = eina_stringshare_add(param->s);
        else if (!strcmp(param->name, "paused"))
          {
             mem->paused = !!param->i;
             mem->paused_exists = EINA_TRUE;
          }
     }

   return mem;
}

/**
 * @brief Retrieves a content part from the photocam widget.
 * Photocam does not expose named content parts through this mechanism.
 *
 * @param data Unused.
 * @param obj Unused.
 * @param content Unused.
 * @return Always NULL for photocam, as it does not support this.
 */
static Evas_Object *external_photocam_content_get(void *data EINA_UNUSED,
                                                  const Evas_Object *obj EINA_UNUSED,
                                                  const char *content EINA_UNUSED)
{
   ERR("No content.");
   return NULL;
}

/**
 * @brief Frees the memory allocated for Elm_Params_Photocam.
 * This function is used to clean up the structure returned by
 * external_photocam_params_parse().
 *
 * @param params A pointer to an Elm_Params_Photocam structure.
 */
static void
external_photocam_params_free(void *params)
{
   Elm_Params_Photocam *mem = params;

   if (mem->file)
     eina_stringshare_del(mem->file);
   if (mem->zoom_mode)
     eina_stringshare_del(mem->zoom_mode);
   free(mem);
}

/**
 * @brief Describes the external parameters supported by the photocam widget.
 * This array is used by Edje to understand how to interact with the photocam's properties.
 * Each entry defines a parameter's name, type, and other relevant information (like choices for enums).
 * - "file": string, path to the image.
 * - "zoom": double, zoom level.
 * - "zoom mode": choice, one of "manual", "auto fit", "auto fill".
 * - "paused": bool, true if animations are paused.
 */
static Edje_External_Param_Info external_photocam_params[] = {
     DEFINE_EXTERNAL_COMMON_PARAMS,
     EDJE_EXTERNAL_PARAM_INFO_STRING("file"),
     EDJE_EXTERNAL_PARAM_INFO_DOUBLE("zoom"),
     EDJE_EXTERNAL_PARAM_INFO_CHOICE_FULL("zoom mode", "manual", choices),
     EDJE_EXTERNAL_PARAM_INFO_BOOL("paused"),
     EDJE_EXTERNAL_PARAM_INFO_SENTINEL
};

DEFINE_EXTERNAL_ICON_ADD(photocam, "photocam");
DEFINE_EXTERNAL_TYPE_SIMPLE(photocam, "Photocam");
