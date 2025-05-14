#include "edje_private.h"
#include "edje_part_helper.h"

#define MY_CLASS EFL_CANVAS_LAYOUT_PART_EXTERNAL_CLASS

static void _external_compose(Eo *obj, Edje *ed, const char *part);

/**
 * @brief Implements the proxy pattern for external parts.
 *
 * This macro sets up the necessary functions for proxying calls to an external object.
 * It uses the _external_compose function to attach the external object.
 */
PROXY_IMPLEMENTATION(external, MY_CLASS, EINA_TRUE, _external_compose(proxy, ed, rp->part->name))
#undef PROXY_IMPLEMENTATION

/**
 * @brief Composes the external part by attaching its object.
 *
 * This function retrieves the external Evas object associated with the given part name
 * from the Edje object and then attaches it to the provided Efl_Canvas_Layout_Part_External object.
 *
 * @param obj The Efl_Canvas_Layout_Part_External object to attach to.
 * @param ed The Edje object containing the external part.
 * @param part The name of the external part.
 */
static void
_external_compose(Eo *obj, Edje *ed, const char *part)
{
   Eo *ext_obj = _edje_object_part_external_object_get(ed, part);
   efl_composite_attach(obj, ext_obj);
}

/**
 * @brief Gets the content of the external part.
 *
 * Retrieves the Efl_Gfx_Entity that represents the actual content
 * of this external part. This is the object that was set externally.
 *
 * @param obj The Efl_Canvas_Layout_Part_External object.
 * @param _pd Private data for the external part.
 * @return The content (Efl_Gfx_Entity *) of the external part, or NULL if not set.
 */
EOLIAN static Efl_Gfx_Entity *
_efl_canvas_layout_part_external_efl_content_content_get(const Eo *obj, void *_pd EINA_UNUSED)
{
   PROXY_DATA_GET(obj, pd);
   return _edje_object_part_external_object_get(pd->ed, pd->part);
}

/**
 * @brief Sets the content of the external part (Not Permitted).
 *
 * This operation is not permitted for external parts as their content
 * is managed externally. Attempting to call this function will result
 * in an error and return EINA_FALSE.
 *
 * @param obj The Efl_Canvas_Layout_Part_External object (unused).
 * @param pd Private data for the external part (unused).
 * @param content The content to set (unused).
 * @return EINA_FALSE always, as this operation is not permitted.
 */
EOLIAN static Eina_Bool
_efl_canvas_layout_part_external_efl_content_content_set(Eo *obj EINA_UNUSED, void *pd EINA_UNUSED, Efl_Gfx_Entity *content EINA_UNUSED)
{
   ERR("Setting of content is not permitted on this part");
   return EINA_FALSE;
}

/**
 * @brief Unsets the content of the external part (Not Permitted).
 *
 * This operation is not permitted for external parts as their content
 * is managed externally. Attempting to call this function will result
 * in an error and return NULL.
 *
 * @param obj The Efl_Canvas_Layout_Part_External object (unused).
 * @param pd Private data for the external part (unused).
 * @return NULL always, as this operation is not permitted.
 */
EOLIAN static Efl_Gfx_Entity*
_efl_canvas_layout_part_external_efl_content_content_unset(Eo *obj EINA_UNUSED, void *pd EINA_UNUSED)
{
   ERR("Unsetting of content is not permitted on this part");
   return NULL;
}

#include "efl_canvas_layout_part_external.eo.c"
