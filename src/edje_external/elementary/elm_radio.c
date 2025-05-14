#include "private.h"

/**
 * @brief Parameters for creating a radio widget.
 *
 * This structure holds all the parameters that can be used when creating
 * a radio widget externally.
 */
typedef struct _Elm_Params_Radio
{
   Elm_Params base; /**< Base parameters */
   const char *label; /**< The label text for the radio widget */
   Evas_Object *icon; /**< The icon object for the radio widget */
   const char* group_name; /**< The name of the group this radio belongs to */
   int value; /**< The integer value associated with this radio state */
   Eina_Bool value_exists:1; /**< Flag indicating if the value parameter is set */
} Elm_Params_Radio;

/**
 * @brief Sets the state of an external radio widget.
 *
 * This function is called to apply parameters to a radio widget,
 * typically during transitions or initial setup.
 *
 * @param data Unused.
 * @param obj The radio widget object.
 * @param from_params The previous state parameters (can be NULL).
 * @param to_params The new state parameters (can be NULL).
 * @param pos Unused.
 */
static void
external_radio_state_set(void *data EINA_UNUSED, Evas_Object *obj,
                         const void *from_params, const void *to_params,
                         float pos EINA_UNUSED)
{
   const Elm_Params_Radio *p;

   if (to_params) p = to_params;
   else if (from_params) p = from_params;
   else return;

   if (p->label)
     elm_object_text_set(obj, p->label);
   if (p->icon)
     elm_object_part_content_set(obj, "icon", p->icon);
   if (p->value_exists)
     elm_radio_state_value_set(obj, p->value);
   if (p->group_name)
     {
        Evas_Object *ed = evas_object_smart_parent_get(obj);
        Evas_Object *group = edje_object_part_swallow_get(ed, p->group_name);
        elm_radio_group_add(obj, group);
     }
}

/**
 * @brief Sets a specific parameter for an external radio widget.
 *
 * This function is called by Edje to set individual parameters on the
 * radio widget.
 *
 * @param data Unused.
 * @param obj The radio widget object.
 * @param param The parameter to set.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
external_radio_param_set(void *data EINA_UNUSED, Evas_Object *obj,
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
   else if (!strcmp(param->name, "value"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_INT)
          {
             elm_radio_value_set(obj, param->i);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "group"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
          {
             Evas_Object *ed = evas_object_smart_parent_get(obj);
             Evas_Object *group = edje_object_part_swallow_get(ed, param->s);
             elm_radio_group_add(obj, group);
             return EINA_TRUE;
          }
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Gets a specific parameter from an external radio widget.
 *
 * This function is called by Edje to retrieve individual parameters from the
 * radio widget.
 *
 * @param data Unused.
 * @param obj The radio widget object.
 * @param param The parameter to get (name is input, value is output).
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
external_radio_param_get(void *data EINA_UNUSED, const Evas_Object *obj,
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
   else if (!strcmp(param->name, "value"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_INT)
          {
             param->i = elm_radio_value_get(obj);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "group"))
     {
        /* not easy to get group name back from live object */
        return EINA_FALSE;
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Parses a list of Edje_External_Param to create an Elm_Params_Radio structure.
 *
 * This function converts a list of generic Edje parameters into a
 * radio-specific parameter structure.
 *
 * @param data Unused.
 * @param obj The Evas_Object this parameter structure is for (used for icon parsing).
 * @param params A list of Edje_External_Param objects.
 *        Example of params list structure:
 *        params = [
 *          (Edje_External_Param){ .name = "label", .type = EDJE_EXTERNAL_PARAM_TYPE_STRING, .s = "Option 1" },
 *          (Edje_External_Param){ .name = "icon", .type = EDJE_EXTERNAL_PARAM_TYPE_STRING, .s = "my_icon" },
 *          (Edje_External_Param){ .name = "group", .type = EDJE_EXTERNAL_PARAM_TYPE_STRING, .s = "group1" },
 *          (Edje_External_Param){ .name = "value", .type = EDJE_EXTERNAL_PARAM_TYPE_INT, .i = 10 }
 *        ]
 * @return A newly allocated Elm_Params_Radio structure, or NULL on failure.
 *         The caller is responsible for freeing the returned structure using
 *         external_radio_params_free().
 */
static void *
external_radio_params_parse(void *data EINA_UNUSED, Evas_Object *obj, const Eina_List *params)
{
   Elm_Params_Radio *mem;
   Edje_External_Param *param;
   const Eina_List *l;

   mem = calloc(1, sizeof(Elm_Params_Radio));
   if (!mem)
     return NULL;

   external_common_icon_param_parse(&mem->icon, obj, params);

   EINA_LIST_FOREACH(params, l, param)
     {
        if (!strcmp(param->name, "group"))
          mem->group_name = eina_stringshare_add(param->s);
        else if (!strcmp(param->name, "value"))
          {
             mem->value = param->i;
             mem->value_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "label"))
          mem->label = eina_stringshare_add(param->s);
     }

   return mem;
}

/**
 * @brief Gets content from an external radio widget.
 *
 * Radio widgets typically do not have named content parts that can be
 * retrieved this way. This function currently always returns NULL.
 *
 * @param data Unused.
 * @param obj Unused.
 * @param content Unused.
 * @return Always NULL.
 */
static Evas_Object *external_radio_content_get(void *data EINA_UNUSED,
                                               const Evas_Object *obj EINA_UNUSED,
                                               const char *content EINA_UNUSED)
{
   ERR("No content.");
   return NULL;
}

/**
 * @brief Frees the memory allocated for Elm_Params_Radio.
 *
 * @param params The Elm_Params_Radio structure to free.
 */
static void
external_radio_params_free(void *params)
{
   Elm_Params_Radio *mem = params;

   if (mem->group_name)
     eina_stringshare_del(mem->group_name);
   if (mem->label)
     eina_stringshare_del(mem->label);
   free(params);
}

static Edje_External_Param_Info external_radio_params[] = {
     DEFINE_EXTERNAL_COMMON_PARAMS,
     EDJE_EXTERNAL_PARAM_INFO_STRING("label"),
     EDJE_EXTERNAL_PARAM_INFO_STRING("icon"),
     EDJE_EXTERNAL_PARAM_INFO_STRING("group"),
     EDJE_EXTERNAL_PARAM_INFO_INT("value"),
     EDJE_EXTERNAL_PARAM_INFO_SENTINEL
};

DEFINE_EXTERNAL_ICON_ADD(radio, "radio");
DEFINE_EXTERNAL_TYPE_SIMPLE(radio, "Radio");
