#include <assert.h>
#include "private.h"

/**
 * @brief Structure to hold parameters for an Elm_Icon widget.
 *
 * This structure is used to parse and store parameters from an Edje external
 * definition for an icon object.
 */
typedef struct _Elm_Params_Icon
{
   const char *file; /**< The image file path. */
   Eina_Bool scale_up_exists; /**< Flag indicating if scale_up parameter is set. */
   Eina_Bool scale_up : 1; /**< Whether to scale up the image if it's smaller than the object. */
   Eina_Bool scale_down_exists; /**< Flag indicating if scale_down parameter is set. */
   Eina_Bool scale_down : 1; /**< Whether to scale down the image if it's larger than the object. */
   Eina_Bool smooth_exists; /**< Flag indicating if smooth parameter is set. */
   Eina_Bool smooth : 1; /**< Whether to apply smooth scaling. */
   Eina_Bool fill_outside_exists; /**< Flag indicating if fill_outside parameter is set. */
   Eina_Bool fill_outside : 1; /**< Whether to fill outside the image area. */
   Eina_Bool no_scale_exists; /**< Flag indicating if no_scale parameter is set. */
   Eina_Bool no_scale : 1; /**< Whether to disable scaling. */
   Eina_Bool prescale_size_exists; /**< Flag indicating if prescale_size parameter is set. */
   int prescale_size; /**< The prescale size for the image. */
   Elm_Params base; /**< Base parameters. */
   const char *icon; /**< The standard icon name. */
} Elm_Params_Icon;

/**
 * @brief Sets the state of an external icon object.
 *
 * This function is called to apply parameters to an icon object,
 * typically during Edje theme transitions or initial setup.
 *
 * @param data Unused.
 * @param obj The Evas_Object (icon) to set the state for.
 * @param from_params The previous state parameters (can be NULL).
 * @param to_params The new state parameters to apply.
 * @param pos Unused.
 */
static void
external_icon_state_set(void *data EINA_UNUSED, Evas_Object *obj,
                        const void *from_params, const void *to_params,
                        float pos EINA_UNUSED)
{
   const Elm_Params_Icon *p;
   Evas_Object *edje;
   const char *file;
   Eina_Bool param;

   if (to_params) p = to_params;
   else if (from_params) p = from_params;
   else return;

   if (p->file)
     {
        elm_image_file_set(obj, p->file, NULL);
     }
   if (p->smooth_exists)
     {
        elm_image_smooth_set(obj, p->smooth);
     }
   if (p->no_scale_exists)
     {
        elm_image_no_scale_set(obj, p->no_scale);
     }
   if (p->scale_up_exists && p->scale_down_exists)
     {
        elm_image_resizable_set(obj, p->scale_up, p->scale_down);
     }
   else if (p->scale_up_exists || p->scale_down_exists)
     {
        if (p->scale_up_exists)
          {
             elm_image_resizable_get(obj, NULL, &param);
             elm_image_resizable_set(obj, p->scale_up, param);
          }
        else
          {
             elm_image_resizable_get(obj, &param, NULL);
             elm_image_resizable_set(obj, param, p->scale_down);
          }
     }
   if (p->fill_outside_exists)
     {
        elm_image_fill_outside_set(obj, p->fill_outside);
     }
   if (p->prescale_size_exists)
     {
        elm_image_prescale_set(obj, p->prescale_size);
     }
   if (p->icon)
     {
        edje = evas_object_smart_parent_get(obj);
        edje_object_file_get(edje, &file, NULL);

        if (!edje_file_group_exists(file, p->icon))
          {
            if (!elm_icon_standard_set(obj, p->icon))
              ERR("Failed to set standard icon! (%s)", p->icon);
          }
        else if (!elm_image_file_set(obj, file, p->icon))
          {
            if (!elm_icon_standard_set(obj, p->icon))
              ERR("Failed to set standard icon! (%s)", p->icon);
          }
     }
}

/**
 * @brief Sets a specific parameter for an external icon object.
 *
 * This function is called by Edje to set individual parameters on the icon
 * object. It handles various icon properties like file, smooth, scale, etc.
 *
 * @param data Unused.
 * @param obj The Evas_Object (icon) to modify.
 * @param param The Edje_External_Param to apply.
 * @return EINA_TRUE on success, EINA_FALSE on failure or if the parameter is unknown.
 */
static Eina_Bool
external_icon_param_set(void *data EINA_UNUSED, Evas_Object *obj,
                        const Edje_External_Param *param)
{
   Evas_Object *edje;
   const char *file;
   Eina_Bool p;

   if (!strcmp(param->name, "file")
       && param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
     {
        return elm_image_file_set(obj, param->s, NULL);
     }
   else if (!strcmp(param->name, "smooth")
            && param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
     {
        elm_image_smooth_set(obj, param->i);
        return EINA_TRUE;
     }
   else if (!strcmp(param->name, "no scale")
            && param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
     {
        elm_image_no_scale_set(obj, param->i);
        return EINA_TRUE;
     }
   else if (!strcmp(param->name, "scale up")
            && param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
     {
        elm_image_resizable_get(obj, NULL, &p);
        elm_image_resizable_set(obj, param->i, p);
        return EINA_TRUE;
     }
   else if (!strcmp(param->name, "scale down")
            && param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
     {
        elm_image_resizable_get(obj, &p, NULL);
        elm_image_resizable_set(obj, p, param->i);
        return EINA_TRUE;
     }
   else if (!strcmp(param->name, "fill outside")
            && param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
     {
        elm_image_fill_outside_set(obj, param->i);
        return EINA_TRUE;
     }
   else if (!strcmp(param->name, "prescale")
            && param->type == EDJE_EXTERNAL_PARAM_TYPE_INT)
     {
        elm_image_prescale_set(obj, param->i);
        return EINA_TRUE;
     }
   else if (!strcmp(param->name, "icon"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
          {
             edje = evas_object_smart_parent_get(obj);
             edje_object_file_get(edje, &file, NULL);

             if (!edje_file_group_exists(file, param->s))
               {
                 if (!elm_icon_standard_set(obj, param->s))
                   ERR("Failed to set standard icon! (%s)", param->s);
               }
             else if (!elm_image_file_set(obj, file, param->s))
               {
                 if (!elm_icon_standard_set(obj, param->s))
                   ERR("Failed to set standard icon as fallback! (%s)", param->s);
               }
             return EINA_TRUE;
          }
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Gets a specific parameter from an external icon object.
 *
 * This function is called by Edje to retrieve the current value of
 * individual parameters from the icon object.
 *
 * @param data Unused.
 * @param obj The Evas_Object (icon) to query.
 * @param param An Edje_External_Param structure to fill with the parameter's value.
 *              The `name` and `type` fields are pre-filled.
 * @return EINA_TRUE on success, EINA_FALSE on failure or if the parameter is unknown/unreadable.
 */
static Eina_Bool
external_icon_param_get(void *data EINA_UNUSED,
                        const Evas_Object *obj,
                        Edje_External_Param *param)
{

   if (!strcmp(param->name, "file")
       && param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
     {
        elm_image_file_get(obj, &param->s, NULL);
        return EINA_TRUE;
     }
   else if (!strcmp(param->name, "smooth")
            && param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
     {
        param->i = elm_image_smooth_get(obj);
        return EINA_TRUE;
     }
   else if (!strcmp(param->name, "no scale")
            && param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
     {
        param->i = elm_image_no_scale_get(obj);
        return EINA_TRUE;
     }
   else if (!strcmp(param->name, "scale up")
            && param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
     {
        elm_image_resizable_get(obj, NULL, (Eina_Bool *)(&param->i));
        return EINA_TRUE;
     }
   else if (!strcmp(param->name, "scale down")
            && param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
     {
        elm_image_resizable_get(obj, (Eina_Bool *)(&param->i), NULL);
        return EINA_TRUE;
     }
   else if (!strcmp(param->name, "fill outside")
            && param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
     {
        param->i = elm_image_fill_outside_get(obj);
        return EINA_TRUE;
     }
   else if (!strcmp(param->name, "prescale")
            && param->type == EDJE_EXTERNAL_PARAM_TYPE_INT)
     {
        param->i = elm_image_prescale_get(obj);
        return EINA_TRUE;
     }
   else if (!strcmp(param->name, "icon"))
     {
        /* not easy to get icon name back from live object */
        return EINA_FALSE;
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Parses a list of Edje external parameters into an Elm_Params_Icon structure.
 *
 * This function converts a list of Edje_External_Param objects into a more
 * usable Elm_Params_Icon structure, allocating memory for it.
 *
 * @param data Unused.
 * @param obj Unused.
 * @param params A list of Edje_External_Param to parse.
 *               Example of params list structure:
 *               Eina_List* containing Edje_External_Param* elements.
 *               Each Edje_External_Param has:
 *                 - const char* name (e.g., "file", "smooth")
 *                 - Edje_External_Param_Type type (e.g., EDJE_EXTERNAL_PARAM_TYPE_STRING)
 *                 - union { int i; double d; const char *s; } value
 * @return A pointer to the newly allocated Elm_Params_Icon structure, or NULL on failure.
 */
static void *
external_icon_params_parse(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                           const Eina_List *params)
{
   Elm_Params_Icon *mem;
   Edje_External_Param *param;
   const Eina_List *l;
   mem = ELM_NEW(Elm_Params_Icon);
   if (EINA_UNLIKELY(!mem))
     return NULL;

   EINA_LIST_FOREACH(params, l, param)
     {
        if (!strcmp(param->name, "file"))
          mem->file = eina_stringshare_add(param->s);
        else if (!strcmp(param->name, "smooth"))
          {
             mem->smooth = param->i;
             mem->smooth_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "no scale"))
          {
             mem->no_scale = param->i;
             mem->no_scale_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "scale up"))
          {
             mem->scale_up = param->i;
             mem->scale_up_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "scale down"))
          {
             mem->scale_down = param->i;
             mem->scale_down_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "fill outside"))
          {
             mem->fill_outside = param->i;
             mem->fill_outside_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "prescale"))
          {
             mem->prescale_size = param->i;
             mem->prescale_size_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "icon"))
          {
             mem->icon = eina_stringshare_add(param->s);
          }
     }

   return mem;
}

/**
 * @brief Gets a content part from an external icon object.
 *
 * Icons typically do not have named content parts that can be retrieved
 * this way, so this function currently returns NULL and logs an error.
 *
 * @param data Unused.
 * @param obj Unused.
 * @param content Unused.
 * @return Always NULL for icons.
 */
static Evas_Object *
external_icon_content_get(void *data EINA_UNUSED,
                          const Evas_Object *obj EINA_UNUSED,
                          const char *content EINA_UNUSED)
{
   ERR("no content");
   return NULL;
}

/**
 * @brief Frees the memory allocated for Elm_Params_Icon.
 *
 * This function is responsible for releasing the resources held by an
 * Elm_Params_Icon structure, including any stringshared strings.
 *
 * @param params A pointer to the Elm_Params_Icon structure to free.
 */
static void
external_icon_params_free(void *params)
{
   Elm_Params_Icon *mem = params;

   if (mem->file)
     eina_stringshare_del(mem->file);

   if (mem->icon)
     eina_stringshare_del(mem->icon);
   free(mem);
}

/**
 * @brief Defines the parameters accepted by an external icon object.
 *
 * This array provides metadata about the parameters that can be used
 * in an Edje file to configure an icon object.
 */
static Edje_External_Param_Info external_icon_params[] = {
   DEFINE_EXTERNAL_COMMON_PARAMS, /**< Common parameters like "visible", "clip_to", etc. */
   EDJE_EXTERNAL_PARAM_INFO_STRING("icon"), /**< Standard icon name (e.g., "home", "close"). */
   EDJE_EXTERNAL_PARAM_INFO_STRING("file"),
   EDJE_EXTERNAL_PARAM_INFO_BOOL("smooth"),
   EDJE_EXTERNAL_PARAM_INFO_BOOL("no scale"),
   EDJE_EXTERNAL_PARAM_INFO_BOOL("scale up"),
   EDJE_EXTERNAL_PARAM_INFO_BOOL("scale down"),
   EDJE_EXTERNAL_PARAM_INFO_BOOL("fill outside"),
   EDJE_EXTERNAL_PARAM_INFO_INT("prescale"),
   EDJE_EXTERNAL_PARAM_INFO_SENTINEL
};

DEFINE_EXTERNAL_ICON_ADD(icon, "icon");
DEFINE_EXTERNAL_TYPE_SIMPLE(icon, "Icon");
