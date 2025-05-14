#include "edje_private.h"
#include "edje_part_helper.h"
#include "efl_canvas_layout_part_swallow.eo.h"
#define MY_CLASS EFL_CANVAS_LAYOUT_PART_SWALLOW_CLASS

PROXY_IMPLEMENTATION(swallow, MY_CLASS, EINA_FALSE)
#undef PROXY_IMPLEMENTATION

/* Swallow parts */

/**
 * @brief Get the content of the swallow part.
 *
 * This function retrieves the Efl_Gfx_Entity object currently swallowed by this part.
 *
 * @param obj The Efl_Canvas_Layout_Part_Swallow object.
 * @param _pd Private data for the swallow part (unused in this function).
 * @return The swallowed Efl_Gfx_Entity object, or NULL if no content is set.
 */
EOLIAN static Efl_Gfx_Entity *
_efl_canvas_layout_part_swallow_efl_content_content_get(const Eo *obj, void *_pd EINA_UNUSED)
{
   PROXY_DATA_GET(obj, pd);
   return _edje_efl_content_content_get(pd->ed, pd->part);
}

/**
 * @brief Set the content of the swallow part.
 *
 * This function sets the given Efl_Gfx_Entity object as the content for this swallow part.
 * If another content was already set, it will be replaced.
 *
 * @param obj The Efl_Canvas_Layout_Part_Swallow object.
 * @param _pd Private data for the swallow part (unused in this function).
 * @param content The Efl_Gfx_Entity object to swallow.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_canvas_layout_part_swallow_efl_content_content_set(Eo *obj, void *_pd EINA_UNUSED, Efl_Gfx_Entity *content)
{
   PROXY_DATA_GET(obj, pd);
   return _edje_efl_content_content_set(pd->ed, pd->part, content);
}

/**
 * @brief Unset the content of the swallow part.
 *
 * This function removes and returns the currently swallowed Efl_Gfx_Entity object.
 * The removed object is not deleted, only unparented from this swallow part.
 *
 * @param obj The Efl_Canvas_Layout_Part_Swallow object.
 * @param _pd Private data for the swallow part (unused in this function).
 * @return The previously swallowed Efl_Gfx_Entity object, or NULL if no content was set.
 */
EOLIAN static Efl_Gfx_Entity *
_efl_canvas_layout_part_swallow_efl_content_content_unset(Eo *obj, void *_pd EINA_UNUSED)
{
   PROXY_DATA_GET(obj, pd);
   Efl_Gfx_Entity *content = _edje_efl_content_content_get(pd->ed, pd->part);
   if (!content) return NULL;
   efl_canvas_layout_content_remove(obj, content);
   return content;
}

#include "efl_canvas_layout_part_swallow.eo.c"
