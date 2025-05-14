#include "private.h"

/**
 * @brief Structure to hold parameters for an Elm_Hoversel widget.
 *
 * This structure is used to pass parameters when creating or configuring
 * an Elm_Hoversel widget through the Edje external interface.
 */
typedef struct _Elm_Params_Hoversel
{
   Elm_Params base; /**< Base parameters, common to all Elm widgets */
   const char *label; /**< The text label to set on the hoversel button */
   Evas_Object *icon; /**< The icon object to set on the hoversel button */
   Eina_Bool horizontal:1; /**< If true, the hoversel menu expands horizontally */
   Eina_Bool horizontal_exists:1; /**< Internal flag to check if horizontal was set */
} Elm_Params_Hoversel;

/**
 * @brief Sets the state of an Elm_Hoversel object based on external parameters.
 *
 * This function is called by Edje to apply a set of parameters (either
 * initial or a transition state) to the hoversel widget.
 *
 * @param data Unused user data.
 * @param obj The Elm_Hoversel object to modify.
 * @param from_params The previous state parameters (can be NULL).
 * @param to_params The new state parameters to apply (can be NULL).
 * @param pos The position in a transition (0.0 to 1.0), unused here.
 */
static void
external_hoversel_state_set(void *data EINA_UNUSED, Evas_Object *obj,
                            const void *from_params, const void *to_params,
                            float pos EINA_UNUSED)
{
   const Elm_Params_Hoversel *p;

   if (to_params) p = to_params;
   else if (from_params) p = from_params;
   else return;

   if (p->label)
     elm_object_text_set(obj, p->label);
   if (p->icon)
     elm_object_part_content_set(obj, "icon", p->icon);
   if (p->horizontal_exists)
     elm_hoversel_horizontal_set(obj, p->horizontal);
}

/**
 * @brief Sets a single external parameter on an Elm_Hoversel object.
 *
 * This function is called by Edje to set an individual property of the
 * hoversel widget.
 *
 * @param data Unused user data.
 * @param obj The Elm_Hoversel object to modify.
 * @param param The external parameter to set.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
external_hoversel_param_set(void *data EINA_UNUSED, Evas_Object *obj,
                            const Edje_External_Param *param)
{
   if (!strcmp(param->name, "label"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
          {
             elm_object_text_set(obj, param->s);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "icon"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
          {
             Evas_Object *icon = external_common_param_icon_get(obj, param);
             if ((strcmp(param->s, "")) && (!icon)) return EINA_FALSE;
             elm_object_part_content_set(obj, "icon", icon);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "horizontal"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             elm_hoversel_horizontal_set(obj, param->i);
             return EINA_TRUE;
          }
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Gets a single external parameter from an Elm_Hoversel object.
 *
 * This function is called by Edje to retrieve an individual property of the
 * hoversel widget.
 *
 * @param data Unused user data.
 * @param obj The Elm_Hoversel object to query.
 * @param param The external parameter to get (name is input, value is output).
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
external_hoversel_param_get(void *data EINA_UNUSED, const Evas_Object *obj,
                            Edje_External_Param *param)
{
   if (!strcmp(param->name, "label"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
          {
             param->s = elm_object_text_get(obj);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "icon"))
     {
        /* not easy to get icon name back from live object */
        return EINA_FALSE;
     }
   else if (!strcmp(param->name, "horizontal"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             param->i = elm_hoversel_horizontal_get(obj);
             return EINA_TRUE;
          }
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Parses a list of Edje_External_Param into an Elm_Params_Hoversel structure.
 *
 * This function allocates and populates an Elm_Params_Hoversel structure
 * from a list of parameters provided by Edje.
 *
 * @param data Unused user data.
 * @param obj The Evas_Object this parse is for (used for icon path resolution).
 * @param params A list of Edje_External_Param to parse.
 * @return A newly allocated Elm_Params_Hoversel structure, or NULL on failure.
 *         The caller is responsible for freeing this structure using
 *         external_hoversel_params_free().
 */
static void *
external_hoversel_params_parse(void *data EINA_UNUSED, Evas_Object *obj,
                               const Eina_List *params)
{
   Elm_Params_Hoversel *mem;
   Edje_External_Param *param;
   const Eina_List *l;

   mem = calloc(1, sizeof(Elm_Params_Hoversel));
   if (!mem)
     return NULL;

   external_common_icon_param_parse(&mem->icon, obj, params);

   EINA_LIST_FOREACH(params, l, param)
     {
        if (!strcmp(param->name, "horizontal"))
          {
             mem->horizontal = !!param->i;
             mem->horizontal_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "label"))
          mem->label = eina_stringshare_add(param->s);
     }

   return mem;
}

/**
 * @brief Retrieves content from an Elm_Hoversel object.
 *
 * Elm_Hoversel does not support named content parts through the external interface.
 * This function will always log an error and return NULL.
 *
 * @param data Unused user data.
 * @param obj Unused.
 * @param content Unused.
 * @return Always NULL.
 */
static Evas_Object *external_hoversel_content_get(void *data EINA_UNUSED,
                                                  const Evas_Object *obj EINA_UNUSED,
                                                  const char *content EINA_UNUSED)
{
   ERR("No content.");
   return NULL;
}

/**
 * @brief Frees an Elm_Params_Hoversel structure.
 *
 * This function is used to clean up the memory allocated by
 * external_hoversel_params_parse().
 *
 * @param params The Elm_Params_Hoversel structure to free.
 */
static void
external_hoversel_params_free(void *params)
{
   Elm_Params_Hoversel *mem = params;
   if (mem->label)
     eina_stringshare_del(mem->label);
   free(params);
}

/**
 * @brief Describes the external parameters available for Elm_Hoversel.
 *
 * This array provides metadata about the parameters that can be used
 * to configure an Elm_Hoversel widget from an Edje theme.
 */
static Edje_External_Param_Info external_hoversel_params[] = {
     DEFINE_EXTERNAL_COMMON_PARAMS, /**< Common parameters like "disabled" */
     EDJE_EXTERNAL_PARAM_INFO_STRING("label"), /**< Parameter for setting the hoversel button text */
     EDJE_EXTERNAL_PARAM_INFO_STRING("icon"), /**< Parameter for setting the hoversel button icon */
     EDJE_EXTERNAL_PARAM_INFO_BOOL("horizontal"), /**< Parameter for setting horizontal mode */
     EDJE_EXTERNAL_PARAM_INFO_SENTINEL /**< Marks the end of the parameter list */
};

DEFINE_EXTERNAL_ICON_ADD(hoversel, "hoversel");
DEFINE_EXTERNAL_TYPE_SIMPLE(hoversel, "Hoversel");
