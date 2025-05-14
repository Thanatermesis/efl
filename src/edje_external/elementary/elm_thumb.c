#include <assert.h>
#include "private.h"

/**
 * @brief Parameters for the thumb widget.
 *
 * This structure holds the specific parameters for an Elm_Thumb widget,
 * extending the base Elm_Params.
 */
typedef struct _Elm_Params_Thumb
{
   Elm_Params base; /**< Base parameters */
   const char *animate; /**< Animation setting string (e.g., "loop", "start", "stop") */
} Elm_Params_Thumb;

/**
 * @brief Available choices for the animation setting.
 *
 * This array maps string representations of animation settings to their
 * corresponding Elm_Thumb_Animation_Setting enum values. The order
 * must match the enum.
 */
static const char* choices[] = { "loop", "start", "stop", NULL };

/**
 * @brief Converts an animation setting string to its enum representation.
 *
 * @param anim_str The animation setting string (e.g., "loop", "start", "stop").
 * @return The corresponding Elm_Thumb_Animation_Setting enum value,
 *         or ELM_THUMB_ANIMATION_LAST if the string is not recognized.
 */
static Elm_Thumb_Animation_Setting
_anim_setting_get(const char *anim_str)
{
   unsigned int i;

   assert(sizeof(choices)/sizeof(choices[0]) == ELM_THUMB_ANIMATION_LAST + 1);

   for (i = 0; i < ELM_THUMB_ANIMATION_LAST; i++)
     {
        if (!strcmp(anim_str, choices[i]))
          return i;
     }
   return ELM_THUMB_ANIMATION_LAST;
}

/**
 * @brief Sets the state of the external thumb widget.
 *
 * This function is called when the state of the Edje external part changes.
 * It applies the animation setting from the provided parameters to the thumb object.
 *
 * @param data User data (unused).
 * @param obj The Evas_Object (thumb widget) to set the state on.
 * @param from_params The previous state parameters (Elm_Params_Thumb).
 * @param to_params The new state parameters (Elm_Params_Thumb).
 * @param pos Transition position (unused).
 */
static void
external_thumb_state_set(void *data EINA_UNUSED, Evas_Object *obj,
                         const void *from_params, const void *to_params,
                         float pos EINA_UNUSED)
{
   const Elm_Params_Thumb *p;

   if (to_params) p = to_params;
   else if (from_params) p = from_params;
   else return;

   if (p->animate)
     {
        Elm_Thumb_Animation_Setting set = _anim_setting_get(p->animate);
        if (set != ELM_THUMB_ANIMATION_LAST)
          elm_thumb_animate_set(obj, set);
     }
}

/**
 * @brief Sets a parameter for the external thumb widget.
 *
 * This function is called by Edje to set a specific parameter on the thumb widget.
 * It currently handles the "animate" parameter.
 *
 * @param data User data (unused).
 * @param obj The Evas_Object (thumb widget) to set the parameter on.
 * @param param The Edje_External_Param to set.
 *              Example for "animate": param->name = "animate", param->type = EDJE_EXTERNAL_PARAM_TYPE_CHOICE, param->s = "loop"
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., unknown parameter or invalid value).
 */
static Eina_Bool
external_thumb_param_set(void *data EINA_UNUSED, Evas_Object *obj,
                         const Edje_External_Param *param)
{
   if (!strcmp(param->name, "animate"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_CHOICE)
          {
             Elm_Thumb_Animation_Setting set = _anim_setting_get(param->s);
             if (set == ELM_THUMB_ANIMATION_LAST) return EINA_FALSE;
             elm_thumb_animate_set(obj, set);
             return EINA_TRUE;
          }
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Gets a parameter from the external thumb widget.
 *
 * This function is called by Edje to retrieve the value of a specific parameter
 * from the thumb widget. It currently handles the "animate" parameter.
 *
 * @param data User data (unused).
 * @param obj The Evas_Object (thumb widget) to get the parameter from.
 * @param param The Edje_External_Param to fill with the retrieved value.
 *              Example for "animate": param->name = "animate", param->type = EDJE_EXTERNAL_PARAM_TYPE_CHOICE.
 *              On success, param->s will be set to the current animation choice (e.g., "loop").
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., unknown parameter).
 */
static Eina_Bool
external_thumb_param_get(void *data EINA_UNUSED, const Evas_Object *obj,
                         Edje_External_Param *param)
{
   if (!strcmp(param->name, "animate"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_CHOICE)
          {
             Elm_Thumb_Animation_Setting anim_set = elm_thumb_animate_get(obj);

             if (anim_set == ELM_THUMB_ANIMATION_LAST)
               return EINA_FALSE;

             param->s = choices[anim_set];
             return EINA_TRUE;
          }
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Parses a list of Edje external parameters and creates an Elm_Params_Thumb structure.
 *
 * This function converts a list of Edje parameters (typically from an EDC file)
 * into a structured Elm_Params_Thumb object.
 *
 * @param data User data (unused).
 * @param obj The Evas_Object (unused in this function but part of the Edje external API).
 * @param params A list of Edje_External_Param structures.
 *               Example list structure:
 *               params = (
 *                 (Edje_External_Param){ .name = "animate", .type = EDJE_EXTERNAL_PARAM_TYPE_STRING, .s = "loop" },
 *                 // ... other common params ...
 *               )
 * @return A pointer to a newly allocated Elm_Params_Thumb structure, or NULL on failure.
 *         The caller is responsible for freeing this memory using external_thumb_params_free().
 */
static void *
external_thumb_params_parse(void *data EINA_UNUSED,
                            Evas_Object *obj EINA_UNUSED,
                            const Eina_List *params)
{
   Elm_Params_Thumb *mem;
   Edje_External_Param *param;
   const Eina_List *l;

   mem = calloc(1, sizeof(Elm_Params_Thumb));
   if (!mem)
     return NULL;

   EINA_LIST_FOREACH(params, l, param)
     {
        if (!strcmp(param->name, "animate"))
          mem->animate = eina_stringshare_add(param->s);
     }

   return mem;
}

/**
 * @brief Gets content from the external thumb widget.
 *
 * This function is part of the Edje external API but is not implemented
 * for the thumb widget, as it does not provide named content parts.
 *
 * @param data User data (unused).
 * @param obj The Evas_Object (unused).
 * @param content The name of the content part to get (unused).
 * @return Always NULL, as thumb does not provide content parts.
 */
static Evas_Object *external_thumb_content_get(void *data EINA_UNUSED,
                                               const Evas_Object *obj EINA_UNUSED,
                                               const char *content EINA_UNUSED)
{
   ERR("No content.");
   return NULL;
}

/**
 * @brief Frees the memory allocated for Elm_Params_Thumb.
 *
 * @param params A pointer to the Elm_Params_Thumb structure to free.
 */
static void
external_thumb_params_free(void *params)
{
   Elm_Params_Thumb *mem = params;

   if (mem->animate)
     eina_stringshare_del(mem->animate);
   free(mem);
}

/**
 * @brief Defines the external parameters for the thumb widget.
 *
 * This array describes the parameters that can be set on a thumb widget
 * from an Edje theme (EDC file). It includes common parameters and
 * the specific "animate" parameter.
 *
 * The structure of elements is defined by Edje_External_Param_Info:
 * - DEFINE_EXTERNAL_COMMON_PARAMS: Macro that expands to common parameters like "file", "group".
 * - EDJE_EXTERNAL_PARAM_INFO_CHOICE_FULL("animate", "loop", choices):
 *   - name: "animate"
 *   - default_value: "loop" (string)
 *   - info.choices.list: points to the `choices` array ({"loop", "start", "stop", NULL})
 * - EDJE_EXTERNAL_PARAM_INFO_SENTINEL: Marks the end of the parameter list.
 */
static Edje_External_Param_Info external_thumb_params[] =
  {
    DEFINE_EXTERNAL_COMMON_PARAMS,
    EDJE_EXTERNAL_PARAM_INFO_CHOICE_FULL("animate", "loop", choices),
    EDJE_EXTERNAL_PARAM_INFO_SENTINEL
  };

/**
 * @brief Adds a new thumb widget as an Edje external part.
 *
 * This function is called by Edje when it needs to create an instance
 * of the "thumb" external type. It creates an Elm_Thumb widget, sets it up,
 * and proxies signals.
 *
 * @param data User data (unused).
 * @param evas The Evas canvas (unused, parent's Evas is used).
 * @param edje The Edje object that is requesting the external part.
 * @param params List of parameters for the new thumb (unused in this function,
 *               applied via state_set or param_set).
 * @param part_name The name of the Edje part this thumb is being added to.
 * @return A new Evas_Object (Elm_Thumb widget) or NULL on failure.
 */
static Evas_Object *
external_thumb_add(void *data EINA_UNUSED, Evas *evas EINA_UNUSED,
                   Evas_Object *edje, const Eina_List *params EINA_UNUSED,
                   const char *part_name)
{
   Evas_Object *parent, *obj;
   external_elm_init();
   parent = elm_widget_parent_widget_get(edje);
   if (!parent) parent = edje;
   elm_need_ethumb(); /* extra command needed */
   obj = elm_thumb_add(parent);
   external_signals_proxy(obj, edje, part_name);
   return obj;
}

DEFINE_EXTERNAL_ICON_ADD(thumb, "thumb");
DEFINE_EXTERNAL_TYPE(thumb, "Thumb");
