#include "private.h"
#include <assert.h>

/**
 * @brief Structure to hold the parameters for an Elm_Panes widget.
 * This structure is used to parse and store parameters from an Edje external
 * definition.
 */
typedef struct _Elm_Params_Panes Elm_Params_Panes;

/**
 * @brief Detailed structure for Elm_Panes parameters.
 */
struct _Elm_Params_Panes {
   Elm_Params base; /**< Base parameters, common to all Elm widgets. */
   Evas_Object *content_left; /**< Evas_Object to be set as the left content. */
   Evas_Object *content_right; /**< Evas_Object to be set as the right content. */
   Eina_Bool is_horizontal; /**< Flag indicating if the horizontal orientation is set. */
   Eina_Bool horizontal; /**< The horizontal orientation value (EINA_TRUE for horizontal, EINA_FALSE for vertical). */
   Eina_Bool is_left_size; /**< Flag indicating if the left content size is set. */
   double left_size; /**< The size of the left content (0.0 to 1.0). */
   Eina_Bool is_fixed; /**< Flag indicating if the fixed property is set. */
   Eina_Bool fixed; /**< The fixed property value (EINA_TRUE if fixed, EINA_FALSE otherwise). */
};

/**
 * @brief Sets the state of an Elm_Panes object based on parameters.
 *
 * This function is called to apply parameters (either `from_params` or `to_params`)
 * to the given Elm_Panes object `obj`. It configures properties like content,
 * orientation, and size.
 *
 * @param data Unused user data.
 * @param obj The Elm_Panes object to configure.
 * @param from_params The source state parameters (can be NULL).
 * @param to_params The target state parameters (can be NULL).
 * @param pos Unused position value for transitions.
 */
static void external_panes_state_set(void *data EINA_UNUSED,
      Evas_Object *obj, const void *from_params,
      const void *to_params, float pos EINA_UNUSED)
{
   const Elm_Params_Panes *p;

   if (to_params) p = to_params;
   else if (from_params) p = from_params;
   else return;

   if (p->content_left)
     elm_object_part_content_set(obj, "left", p->content_left);

   if (p->content_right)
     elm_object_part_content_set(obj, "right", p->content_right);

   if (p->is_left_size)
     elm_panes_content_left_size_set(obj, p->left_size);

   if (p->is_horizontal)
     elm_panes_horizontal_set(obj, p->horizontal);

   if (p->is_fixed)
     elm_panes_fixed_set(obj, p->fixed);
}

/**
 * @brief Sets a specific parameter for an Elm_Panes object.
 *
 * This function is called by Edje to set a single parameter on the
 * Elm_Panes object `obj`. It handles parameters like "content left",
 * "content right", "horizontal", "left size", and "fixed".
 *
 * @param data Unused user data.
 * @param obj The Elm_Panes object to modify.
 * @param param The Edje_External_Param to apply.
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., unknown parameter).
 */
static Eina_Bool external_panes_param_set(void *data EINA_UNUSED,
      Evas_Object *obj, const Edje_External_Param *param)
{
   if ((!strcmp(param->name, "content left"))
       && (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING))
   {
      Evas_Object *content = external_common_param_elm_layout_get(obj, param);
      if ((strcmp(param->s, "")) && (!content))
         return EINA_FALSE;
      elm_object_part_content_set(obj, "left", content);
      return EINA_TRUE;
   }
   else if ((!strcmp(param->name, "content right"))
            && (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING))
   {
      Evas_Object *content = external_common_param_elm_layout_get(obj, param);
      if ((strcmp(param->s, "")) && (!content))
        return EINA_FALSE;
      elm_object_part_content_set(obj, "right", content);
      return EINA_TRUE;
   }
   else if ((!strcmp(param->name, "horizontal"))
            && (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL))
   {
      elm_panes_horizontal_set(obj, param->i);
      return EINA_TRUE;
   }
   else if ((!strcmp(param->name, "left size"))
            && (param->type == EDJE_EXTERNAL_PARAM_TYPE_DOUBLE))
   {
      elm_panes_content_left_size_set(obj, param->d);
      return EINA_TRUE;
   }
   else if ((!strcmp(param->name, "fixed"))
            && (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL))
   {
      elm_panes_fixed_set(obj, param->i);
      return EINA_TRUE;
   }

   ERR("unknown parameter '%s' of type '%s'",
         param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Gets a specific parameter value from an Elm_Panes object.
 *
 * This function is called by Edje to retrieve the current value of a
 * single parameter from the Elm_Panes object `obj`. It handles parameters
 * like "horizontal", "left size", and "fixed". Getting content names
 * is not supported.
 *
 * @param data Unused user data.
 * @param obj The Elm_Panes object to query.
 * @param param An Edje_External_Param structure to fill with the parameter's value.
 *              The `name` field indicates which parameter to get.
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., unknown parameter or unsupported).
 */
static Eina_Bool
external_panes_param_get(void *data EINA_UNUSED, const Evas_Object *obj,
                         Edje_External_Param *param)
{
   if (!strcmp(param->name, "content left"))
     {
        /* not easy to get content name back from live object */
        return EINA_FALSE;
     }
   else if (!strcmp(param->name, "content right"))
     {
        /* not easy to get content name back from live object */
        return EINA_FALSE;
     }
   else if ((!strcmp(param->name, "horizontal"))
            && (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL))
     {
        param->i = elm_panes_horizontal_get(obj);
        return EINA_TRUE;
     }
   else if ((!strcmp(param->name, "left size"))
            && (param->type == EDJE_EXTERNAL_PARAM_TYPE_DOUBLE))
     {
        param->d = elm_panes_content_left_size_get(obj);
        return EINA_TRUE;
     }
   else if ((!strcmp(param->name, "fixed"))
            && (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL))
     {
        param->i = elm_panes_fixed_get(obj);
        return EINA_TRUE;
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Parses a list of Edje external parameters for an Elm_Panes object.
 *
 * This function iterates through a list of `Edje_External_Param` objects and
 * populates an `Elm_Params_Panes` structure with the parsed values.
 * The `obj` parameter is used to resolve string parameters that might refer
 * to other Evas_Objects within the same Edje layout (e.g., for content).
 *
 * @param data Unused user data.
 * @param obj The Evas_Object (Elm_Panes) context, used for resolving named content.
 * @param params A list of `Edje_External_Param` to parse.
 *               Example of `params` structure:
 *               Eina_List containing Edje_External_Param elements like:
 *               - { .name = "content left", .type = EDJE_EXTERNAL_PARAM_TYPE_STRING, .s = "my_left_content_object_name" }
 *               - { .name = "horizontal", .type = EDJE_EXTERNAL_PARAM_TYPE_BOOL, .i = EINA_TRUE }
 *               - { .name = "left size", .type = EDJE_EXTERNAL_PARAM_TYPE_DOUBLE, .d = 0.25 }
 * @return A pointer to a newly allocated `Elm_Params_Panes` structure filled
 *         with the parsed parameters, or NULL on allocation failure. The caller
 *         is responsible for freeing this memory using `external_panes_params_free`.
 */
static void *
external_panes_params_parse(void *data EINA_UNUSED, Evas_Object *obj,
                            const Eina_List *params)
{
   Elm_Params_Panes *mem;
   Edje_External_Param *param;
   const Eina_List *l;

   mem = calloc(1, sizeof(Elm_Params_Panes));
   if (!mem)
     return NULL;

   EINA_LIST_FOREACH(params, l, param)
     {
        if (!strcmp(param->name, "content left"))
          mem->content_left = external_common_param_elm_layout_get(obj, param);
        else if (!strcmp(param->name, "content right"))
          mem->content_right = external_common_param_elm_layout_get(obj, param);
        else if (!strcmp(param->name, "horizontal"))
          {
             mem->is_horizontal = EINA_TRUE;
             mem->horizontal = param->i;
          }
        else if (!strcmp(param->name, "left size"))
          {
             mem->is_left_size = EINA_TRUE;
             mem->left_size = param->d;
          }
        else if (!strcmp(param->name, "fixed"))
          {
             mem->is_fixed = EINA_TRUE;
             mem->fixed = param->i;
          }
     }

   return mem;
}

/**
 * @brief Retrieves a content part of an Elm_Panes object by name.
 *
 * This function allows querying for specific content parts of the Elm_Panes
 * widget, such as "left" or "right".
 *
 * @param data Unused user data.
 * @param obj The Elm_Panes object from which to get the content.
 * @param content A string identifying the content part to retrieve.
 *                Expected values are "left" or "right".
 * @return The Evas_Object set as the specified content part, or NULL if
 *         the content part is not found or not set.
 */
static Evas_Object *
external_panes_content_get(void *data EINA_UNUSED, const Evas_Object *obj,
                           const char *content)
{
   if (!strcmp(content, "left"))
     return elm_object_part_content_get(obj, "left");
   else if (!strcmp(content, "right"))
     return elm_object_part_content_get(obj, "right");

   ERR("unknown content '%s'", content);

   return NULL;
}

/**
 * @brief Frees the memory allocated for Elm_Panes parameters.
 *
 * This function is used to release the `Elm_Params_Panes` structure
 * that was allocated by `external_panes_params_parse`.
 *
 * @param params A pointer to the `Elm_Params_Panes` structure to free.
 */
static void external_panes_params_free(void *params)
{
   free(params);
}

/**
 * @brief Array defining the external parameters for Elm_Panes.
 *
 * This array lists all the parameters that can be used in an Edje external
 * definition for an Elm_Panes widget. It includes their names and types.
 */
static Edje_External_Param_Info external_panes_params[] = {
   DEFINE_EXTERNAL_COMMON_PARAMS, /**< Common parameters like "disabled", "visible". */
   EDJE_EXTERNAL_PARAM_INFO_STRING("content left"), /**< Name of the Evas_Object for the left pane. */
   EDJE_EXTERNAL_PARAM_INFO_STRING("content right"), /**< Name of the Evas_Object for the right pane. */
   EDJE_EXTERNAL_PARAM_INFO_BOOL("horizontal"), /**< Sets orientation: EINA_TRUE for horizontal, EINA_FALSE for vertical. */
   EDJE_EXTERNAL_PARAM_INFO_DOUBLE("left size"), /**< Proportional size of the left/top pane (0.0 to 1.0). */
   EDJE_EXTERNAL_PARAM_INFO_BOOL("fixed"), /**< If EINA_TRUE, the separator cannot be moved by user interaction. */
   EDJE_EXTERNAL_PARAM_INFO_SENTINEL /**< Marks the end of the parameter list. */
};

DEFINE_EXTERNAL_ICON_ADD(panes, "panes");
DEFINE_EXTERNAL_TYPE_SIMPLE(panes, "Panes");
