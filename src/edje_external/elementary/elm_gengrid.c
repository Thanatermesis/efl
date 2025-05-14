#include <assert.h>
#include "private.h"

/**
 * @brief Structure to hold parameters for Gengrid widget.
 *
 * This structure stores various configuration options for the Gengrid widget,
 * including selection modes, bounce behavior, item sizes, and alignment.
 * Each parameter typically has a corresponding boolean flag indicating
 * whether it has been set.
 */
typedef struct _Elm_Params_Gengrid
{
   Elm_Params base; /**< Base parameters */
   Eina_Bool multi : 1; /**< Enable multi-selection */
   Eina_Bool multi_exists : 1; /**< True if multi-selection is set */
   Eina_Bool no_select : 1; /**< Disable selection */
   Eina_Bool no_select_exists : 1; /**< True if no-selection is set */
   Eina_Bool always_select : 1; /**< Enable always-select mode */
   Eina_Bool always_select_exists : 1; /**< True if always-select mode is set */
   Eina_Bool h_bounce:1; /**< Enable horizontal bounce */
   Eina_Bool h_bounce_exists:1; /**< True if horizontal bounce is set */
   Eina_Bool v_bounce:1; /**< Enable vertical bounce */
   Eina_Bool v_bounce_exists:1; /**< True if vertical bounce is set */
   double    h_pagerel; /**< Horizontal page relative size */
   Eina_Bool h_pagerel_exists : 1; /**< True if horizontal page relative size is set */
   double    v_pagerel; /**< Vertical page relative size */
   Eina_Bool v_pagerel_exists : 1; /**< True if vertical page relative size is set */
   int       h_itemsize; /**< Horizontal item size */
   Eina_Bool h_itemsize_exists : 1; /**< True if horizontal item size is set */
   int       v_itemsize; /**< Vertical item size */
   Eina_Bool v_itemsize_exists : 1; /**< True if vertical item size is set */
   Eina_Bool horizontal : 1; /**< Enable horizontal mode */
   Eina_Bool horizontal_exists : 1; /**< True if horizontal mode is set */
   Eina_Bool align_x_exists; /**< True if horizontal alignment is set */
   double align_x; /**< Horizontal alignment value */
   Eina_Bool align_y_exists; /**< True if vertical alignment is set */
   double align_y; /**< Vertical alignment value */
} Elm_Params_Gengrid;

/**
 * @brief Sets the state of the Gengrid widget based on parameters.
 *
 * This function is called to apply a set of parameters to the Gengrid widget.
 * It prioritizes `to_params` if available, otherwise uses `from_params`.
 *
 * @param data Unused.
 * @param obj The Gengrid Evas_Object to modify.
 * @param from_params The source parameters (used if `to_params` is NULL).
 * @param to_params The target parameters to apply.
 * @param pos Unused.
 */
static void
external_gengrid_state_set(void *data EINA_UNUSED, Evas_Object *obj,
                           const void *from_params, const void *to_params,
                           float pos EINA_UNUSED)
{
   const Elm_Params_Gengrid *p;

   if (to_params) p = to_params;
   else if (from_params) p = from_params;
   else return;

   if (p->multi_exists)
     elm_gengrid_multi_select_set(obj, p->multi);
   if (p->no_select_exists)
     {
        if (p->no_select)
          elm_gengrid_select_mode_set (obj, ELM_OBJECT_SELECT_MODE_NONE);
        else
          elm_gengrid_select_mode_set (obj, ELM_OBJECT_SELECT_MODE_DEFAULT);
     }
   if (p->always_select_exists)
     {
        if (p->always_select)
          elm_gengrid_select_mode_set (obj, ELM_OBJECT_SELECT_MODE_ALWAYS);
        else
          elm_gengrid_select_mode_set (obj, ELM_OBJECT_SELECT_MODE_DEFAULT);
     }
   if (p->h_bounce_exists)
     {
        Eina_Bool h_bounce, v_bounce;
        elm_scroller_bounce_get(obj, &h_bounce, &v_bounce);
        elm_scroller_bounce_set(obj, p->h_bounce, v_bounce);
     }
   if (p->v_bounce_exists)
     {
        Eina_Bool h_bounce, v_bounce;
        elm_scroller_bounce_get(obj, &h_bounce, &v_bounce);
        elm_scroller_bounce_set(obj, h_bounce, p->v_bounce);
     }
   if (p->h_pagerel_exists)
     {
        double h_pagerel, v_pagerel;
        elm_scroller_page_relative_get(obj, &h_pagerel, &v_pagerel);
        elm_scroller_page_relative_set(obj, h_pagerel, p->v_pagerel);
     }
   if (p->v_pagerel_exists)
     {
        double h_pagerel, v_pagerel;
        elm_scroller_page_relative_get(obj, &h_pagerel, &v_pagerel);
        elm_scroller_page_relative_set(obj, p->h_pagerel, v_pagerel);
     }
   if (p->h_itemsize_exists)
     {
        int h_itemsize, v_itemsize;
        elm_gengrid_item_size_get(obj, &h_itemsize, &v_itemsize);
        elm_gengrid_item_size_set(obj, h_itemsize, p->v_itemsize);
     }
   if (p->v_itemsize_exists)
     {
        int h_itemsize, v_itemsize;
        elm_gengrid_item_size_get(obj, &h_itemsize, &v_itemsize);
        elm_gengrid_item_size_set(obj, p->h_itemsize, v_itemsize);
     }
   else if (p->align_x_exists || p->align_y_exists)
     {
        double x, y;
        elm_gengrid_align_get(obj, &x, &y);
        if (p->align_x_exists)
          elm_gengrid_align_set(obj, p->align_x, y);
        else
          elm_gengrid_align_set(obj, x, p->align_y);
     }
   if (p->horizontal_exists)
     {
        elm_gengrid_horizontal_set(obj, p->horizontal);
     }
}

/**
 * @brief Sets a single parameter for the Gengrid widget.
 *
 * This function is called by Edje to set an external parameter on the Gengrid.
 * It handles various parameters like "multi select", "no selected", "always select",
 * bounce settings, page relative sizes, item sizes, orientation, and alignment.
 *
 * @param data Unused.
 * @param obj The Gengrid Evas_Object to modify.
 * @param param The Edje_External_Param to apply.
 * @return EINA_TRUE if the parameter was successfully set, EINA_FALSE otherwise.
 */
static Eina_Bool
external_gengrid_param_set(void *data EINA_UNUSED, Evas_Object *obj,
                           const Edje_External_Param *param)
{
   if (!strcmp(param->name, "multi select"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             elm_gengrid_multi_select_set(obj, param->i);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "no selected"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             if (param->i)
               elm_gengrid_select_mode_set (obj, ELM_OBJECT_SELECT_MODE_NONE);
             else
               elm_gengrid_select_mode_set (obj, ELM_OBJECT_SELECT_MODE_DEFAULT);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "always select"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             if (param->i)
               elm_gengrid_select_mode_set (obj, ELM_OBJECT_SELECT_MODE_ALWAYS);
             else
               elm_gengrid_select_mode_set (obj, ELM_OBJECT_SELECT_MODE_DEFAULT);
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
   else if (!strcmp(param->name, "horizontal page relative"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_DOUBLE)
          {
             double h_pagerel, v_pagerel;
             elm_scroller_page_relative_get(obj, &h_pagerel, &v_pagerel);
             elm_scroller_page_relative_set(obj, param->d, v_pagerel);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "vertical page relative"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_DOUBLE)
          {
             double h_pagerel, v_pagerel;
             elm_scroller_page_relative_get(obj, &h_pagerel, &v_pagerel);
             elm_scroller_page_relative_set(obj, h_pagerel, param->d);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "horizontal item size"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_INT)
          {
             int h_itemsize, v_itemsize;
             elm_gengrid_item_size_get(obj, &h_itemsize, &v_itemsize);
             elm_gengrid_item_size_set(obj, param->i, v_itemsize);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "vertical item size"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_INT)
          {
             int h_itemsize, v_itemsize;
             elm_gengrid_item_size_get(obj, &h_itemsize, &v_itemsize);
             elm_gengrid_item_size_set(obj, h_itemsize, param->i);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "horizontal"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             elm_gengrid_horizontal_set(obj, param->i);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "align x")
            && param->type == EDJE_EXTERNAL_PARAM_TYPE_DOUBLE)
     {
        double x, y;
        elm_gengrid_align_get(obj, &x, &y);
        elm_gengrid_align_set(obj, param->d, y);
        return EINA_TRUE;
     }
   else if (!strcmp(param->name, "align y")
            && param->type == EDJE_EXTERNAL_PARAM_TYPE_DOUBLE)
     {
        double x, y;
        elm_gengrid_align_get(obj, &x, &y);
        elm_gengrid_align_set(obj, x, param->d);
        return EINA_TRUE;
     }
   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Gets a single parameter from the Gengrid widget.
 *
 * This function is called by Edje to retrieve an external parameter's value
 * from the Gengrid. It handles the same set of parameters as
 * `external_gengrid_param_set`.
 *
 * @param data Unused.
 * @param obj The Gengrid Evas_Object to query.
 * @param param An Edje_External_Param structure to fill with the parameter's value.
 *              The `name` and `type` fields are pre-filled.
 * @return EINA_TRUE if the parameter was successfully retrieved, EINA_FALSE otherwise.
 */
static Eina_Bool
external_gengrid_param_get(void *data EINA_UNUSED, const Evas_Object *obj,
                           Edje_External_Param *param)
{
   if (!strcmp(param->name, "multi select"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             param->i = elm_gengrid_multi_select_get(obj);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "no selected"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             if (elm_gengrid_select_mode_get (obj) ==
                 ELM_OBJECT_SELECT_MODE_NONE)
               param->i = EINA_TRUE;
             else
               param->i = EINA_FALSE;
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "always select"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             if (elm_gengrid_select_mode_get (obj) ==
                 ELM_OBJECT_SELECT_MODE_ALWAYS)
               param->i = EINA_TRUE;
             else
               param->i = EINA_FALSE;
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
   else if (!strcmp(param->name, "horizontal page relative"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_DOUBLE)
          {
             double h_pagerel, v_pagerel;
             elm_scroller_page_relative_get(obj, &h_pagerel, &v_pagerel);
             param->d = h_pagerel;
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "vertical page relative"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_DOUBLE)
          {
             double h_pagerel, v_pagerel;
             elm_scroller_page_relative_get(obj, &h_pagerel, &v_pagerel);
             param->d = v_pagerel;
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "horizontal item size"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_INT)
          {
             int h_itemsize, v_itemsize;
             elm_gengrid_item_size_get(obj, &h_itemsize, &v_itemsize);
             param->i = h_itemsize;
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "vertical item size"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_INT)
          {
             int h_itemsize, v_itemsize;
             elm_gengrid_item_size_get(obj, &h_itemsize, &v_itemsize);
             param->i = v_itemsize;
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "horizontal"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             param->i = elm_gengrid_horizontal_get(obj);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "align x")
            && param->type == EDJE_EXTERNAL_PARAM_TYPE_DOUBLE)
     {
        double x, y;
        elm_gengrid_align_get(obj, &x, &y);
        param->d = x;
        return EINA_TRUE;
     }
   else if (!strcmp(param->name, "align y")
            && param->type == EDJE_EXTERNAL_PARAM_TYPE_DOUBLE)
     {
        double x, y;
        elm_gengrid_align_get(obj, &x, &y);
        param->d = y;
        return EINA_TRUE;
     }
   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Parses a list of Edje external parameters into an Elm_Params_Gengrid structure.
 *
 * This function iterates over a list of Edje_External_Param objects and populates
 * an Elm_Params_Gengrid structure. This structure is then used by
 * `external_gengrid_state_set` to apply the parameters.
 *
 * @param data Unused.
 * @param obj Unused.
 * @param params A list of Edje_External_Param objects to parse.
 *               Example of `params` list structure:
 *               Eina_List containing Edje_External_Param elements.
 *               Each Edje_External_Param has:
 *               - `name`: (const char *) e.g., "multi select", "item_width"
 *               - `type`: (Edje_External_Param_Type) e.g., EDJE_EXTERNAL_PARAM_TYPE_BOOL, EDJE_EXTERNAL_PARAM_TYPE_INT
 *               - `i`: (int) value if type is BOOL or INT
 *               - `d`: (double) value if type is DOUBLE
 *               - `s`: (const char *) value if type is STRING (not used here)
 * @return A pointer to the newly allocated and populated Elm_Params_Gengrid structure,
 *         or NULL on failure. The caller is responsible for freeing this memory
 *         using `external_gengrid_params_free`.
 */
static void *
external_gengrid_params_parse(void *data EINA_UNUSED,
                              Evas_Object *obj EINA_UNUSED,
                              const Eina_List *params)
{
   Elm_Params_Gengrid *mem;
   Edje_External_Param *param;
   const Eina_List *l;

   mem = ELM_NEW(Elm_Params_Gengrid);
   if (!mem)
     return NULL;

   EINA_LIST_FOREACH(params, l, param)
     {
        if (!strcmp(param->name, "multi select"))
          {
             mem->multi = !!param->i;
             mem->multi_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "no selected"))
          {
             mem->no_select = !!param->i;
             mem->no_select_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "always select"))
          {
             mem->always_select = !!param->i;
             mem->always_select_exists = EINA_TRUE;
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
        else if (!strcmp(param->name, "horizontal page relative"))
          {
             mem->h_pagerel = param->d;
             mem->h_pagerel_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "vertical page relative"))
          {
             mem->v_pagerel = param->d;
             mem->v_pagerel_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "horizontal item size"))
          {
             mem->h_itemsize = param->i;
             mem->h_itemsize_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "vertical item size"))
          {
             mem->v_itemsize = param->i;
             mem->v_itemsize_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "horizontal"))
          {
             mem->horizontal = !!param->i;
             mem->horizontal_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "align x"))
          {
             mem->align_x = param->d;
             mem->align_x_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "align y"))
          {
             mem->align_y = param->d;
             mem->align_y_exists = EINA_TRUE;
          }
     }

   return mem;
}

/**
 * @brief Retrieves a content part from the Gengrid widget.
 *
 * This function is intended to get a specific content object from the Gengrid.
 * Currently, it is not implemented and always returns NULL, logging an error.
 *
 * @param data Unused.
 * @param obj Unused.
 * @param content Unused.
 * @return Always returns NULL as Gengrid does not support named content parts
 *         through this mechanism.
 */
static Evas_Object *
external_gengrid_content_get(void *data EINA_UNUSED,
                             const Evas_Object *obj EINA_UNUSED,
                             const char *content EINA_UNUSED)
{
   ERR("No content.");
   return NULL;
}

/**
 * @brief Frees the memory allocated for Elm_Params_Gengrid.
 *
 * This function is used to release the memory allocated by
 * `external_gengrid_params_parse`.
 *
 * @param params A pointer to the Elm_Params_Gengrid structure to free.
 */
static void
external_gengrid_params_free(void *params)
{
   Elm_Params_Gengrid *mem = params;
   free(mem);
}

/**
 * @brief Array defining the external parameters supported by the Gengrid widget.
 *
 * This array is used by Edje to know which parameters can be set or retrieved
 * from the Gengrid widget. Each entry defines the parameter name and its type.
 * Example structure of elements in `external_gengrid_params` array:
 * ```c
 * {
 *   "param_name_1", // (const char *) name
 *   EDJE_EXTERNAL_PARAM_TYPE_BOOL, // (Edje_External_Param_Type) type
 *   EDJE_EXTERNAL_PARAM_FLAG_DEFAULT // (Edje_External_Param_Flags) flags
 * },
 * { "param_name_2", EDJE_EXTERNAL_PARAM_TYPE_INT, EDJE_EXTERNAL_PARAM_FLAG_DEFAULT },
 * // ...
 * { NULL, EDJE_EXTERNAL_PARAM_TYPE_INVALID, EDJE_EXTERNAL_PARAM_FLAG_NONE } // Sentinel
 * ```
 */
static Edje_External_Param_Info external_gengrid_params[] = {
   DEFINE_EXTERNAL_COMMON_PARAMS, /**< Common parameters like "disabled" */
   EDJE_EXTERNAL_PARAM_INFO_BOOL("multi select"), /**< Enables/disables multi-selection mode. */
   EDJE_EXTERNAL_PARAM_INFO_BOOL("no select"), /**< Disables item selection. */
   EDJE_EXTERNAL_PARAM_INFO_BOOL("always select"), /**< Enables always-select mode. */
   EDJE_EXTERNAL_PARAM_INFO_BOOL("height bounce"), /**< Enables/disables vertical bounce (scrolling past boundaries). */
   EDJE_EXTERNAL_PARAM_INFO_BOOL("width bounce"), /**< Enables/disables horizontal bounce. */
   EDJE_EXTERNAL_PARAM_INFO_DOUBLE("horizontal page relative"), /**< Sets the horizontal page step relative to viewport width. */
   EDJE_EXTERNAL_PARAM_INFO_DOUBLE("vertical page relative"), /**< Sets the vertical page step relative to viewport height. */
   EDJE_EXTERNAL_PARAM_INFO_INT("horizontal item size"), /**< Sets the width of items in pixels. */
   EDJE_EXTERNAL_PARAM_INFO_INT("vertical item size"), /**< Sets the height of items in pixels. */
   EDJE_EXTERNAL_PARAM_INFO_BOOL("horizontal"), /**< Sets the Gengrid orientation to horizontal. */
   EDJE_EXTERNAL_PARAM_INFO_DOUBLE("align x"), /**< Sets the horizontal alignment of items (0.0 to 1.0). */
   EDJE_EXTERNAL_PARAM_INFO_DOUBLE("align y"), /**< Sets the vertical alignment of items (0.0 to 1.0). */
   EDJE_EXTERNAL_PARAM_INFO_SENTINEL /**< Marks the end of the parameter list. */
};

DEFINE_EXTERNAL_ICON_ADD(gengrid, "gengrid");
DEFINE_EXTERNAL_TYPE_SIMPLE(gengrid, "Generic Grid");
