#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "ecore_wl2_private.h"

/**
 * @internal
 * @brief Unmaps a subsurface.
 *
 * This function destroys the Wayland subsurface and surface associated with
 * the Ecore_Wl2_Subsurface object.
 *
 * @param subsurf The subsurface to unmap.
 */
void
_ecore_wl2_subsurf_unmap(Ecore_Wl2_Subsurface *subsurf)
{
   if (subsurf->wl.subsurface) wl_subsurface_destroy(subsurf->wl.subsurface);
   if (subsurf->wl.surface) wl_surface_destroy(subsurf->wl.surface);
   subsurf->wl.subsurface = NULL;
   subsurf->wl.surface = NULL;
}

/**
 * @internal
 * @brief Frees a subsurface.
 *
 * This function unmaps the subsurface, removes it from its parent window's
 * list of subsurfaces, and frees the memory allocated for the
 * Ecore_Wl2_Subsurface object.
 *
 * @param subsurf The subsurface to free.
 */
void
_ecore_wl2_subsurf_free(Ecore_Wl2_Subsurface *subsurf)
{
   Ecore_Wl2_Window *parent;

   _ecore_wl2_subsurf_unmap(subsurf);

   parent = subsurf->parent;
   if (parent)
     {
        parent->subsurfs =
          eina_inlist_remove(parent->subsurfs, EINA_INLIST_GET(subsurf));
     }

   free(subsurf);
}

/**
 * @brief Creates a new subsurface for a given window.
 *
 * This function creates a new Wayland subsurface and associates it with the
 * provided Ecore_Wl2_Window. The subsurface is initially in synchronized mode.
 *
 * @param window The parent window for the new subsurface.
 * @return A pointer to the newly created Ecore_Wl2_Subsurface, or @c NULL on failure.
 *
 * @see ecore_wl2_subsurface_del()
 */
EAPI Ecore_Wl2_Subsurface *
ecore_wl2_subsurface_new(Ecore_Wl2_Window *window)
{
   Ecore_Wl2_Display *display;
   Ecore_Wl2_Subsurface *subsurf;

   EINA_SAFETY_ON_NULL_RETURN_VAL(window, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(window->surface, NULL);

   display = window->display;

   EINA_SAFETY_ON_NULL_RETURN_VAL(display->wl.compositor, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(display->wl.subcompositor, NULL);

   subsurf = calloc(1, sizeof(Ecore_Wl2_Subsurface));
   if (!subsurf) return NULL;

   subsurf->parent = window;

   subsurf->wl.surface = wl_compositor_create_surface(display->wl.compositor);
   if (!subsurf->wl.surface)
     {
        ERR("Failed to create surface");
        goto surf_err;
     }

   subsurf->wl.subsurface =
     wl_subcompositor_get_subsurface(display->wl.subcompositor,
                                     subsurf->wl.surface, window->surface);
   if (!subsurf->wl.subsurface)
     {
        ERR("Could not create subsurface");
        goto sub_surf_err;
     }

   /* A sub-surface is initially in the synchronized mode. */
   subsurf->sync = EINA_TRUE;

   window->subsurfs =
     eina_inlist_append(window->subsurfs, EINA_INLIST_GET(subsurf));

   return subsurf;

sub_surf_err:
   wl_surface_destroy(subsurf->wl.surface);

surf_err:
   free(subsurf);
   return NULL;
}

/**
 * @brief Deletes a subsurface.
 *
 * This function frees the resources associated with an Ecore_Wl2_Subsurface.
 *
 * @param subsurface The subsurface to delete.
 *
 * @see ecore_wl2_subsurface_new()
 */
EAPI void
ecore_wl2_subsurface_del(Ecore_Wl2_Subsurface *subsurface)
{
   EINA_SAFETY_ON_NULL_RETURN(subsurface);

   _ecore_wl2_subsurf_free(subsurface);
}

/**
 * @brief Gets the Wayland surface associated with a subsurface.
 *
 * @param subsurface The Ecore_Wl2_Subsurface.
 * @return A pointer to the wl_surface, or @c NULL if @p subsurface is @c NULL.
 */
EAPI struct wl_surface *
ecore_wl2_subsurface_surface_get(Ecore_Wl2_Subsurface *subsurface)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(subsurface, NULL);

   return subsurface->wl.surface;
}

/**
 * @brief Sets the position of a subsurface relative to its parent surface.
 *
 * @param subsurface The Ecore_Wl2_Subsurface.
 * @param x The x-coordinate.
 * @param y The y-coordinate.
 */
EAPI void
ecore_wl2_subsurface_position_set(Ecore_Wl2_Subsurface *subsurface, int x, int y)
{
   EINA_SAFETY_ON_NULL_RETURN(subsurface);
   EINA_SAFETY_ON_NULL_RETURN(subsurface->wl.subsurface);

   if ((subsurface->x == x) && (subsurface->y == y)) return;

   subsurface->x = x;
   subsurface->y = y;

   wl_subsurface_set_position(subsurface->wl.subsurface, x, y);
}

/**
 * @brief Gets the position of a subsurface relative to its parent surface.
 *
 * @param subsurface The Ecore_Wl2_Subsurface.
 * @param[out] x Pointer to store the x-coordinate. Can be @c NULL.
 * @param[out] y Pointer to store the y-coordinate. Can be @c NULL.
 */
EAPI void
ecore_wl2_subsurface_position_get(Ecore_Wl2_Subsurface *subsurface, int *x, int *y)
{
   EINA_SAFETY_ON_NULL_RETURN(subsurface);

   if (x) *x = subsurface->x;
   if (y) *y = subsurface->y;
}

/**
 * @brief Places the subsurface above a sibling surface.
 *
 * This function changes the stacking order of the subsurface, placing it
 * above the specified sibling Wayland surface.
 *
 * @param subsurface The Ecore_Wl2_Subsurface to reorder.
 * @param surface The sibling wl_surface to place @p subsurface above.
 */
EAPI void
ecore_wl2_subsurface_place_above(Ecore_Wl2_Subsurface *subsurface, struct wl_surface *surface)
{
   EINA_SAFETY_ON_NULL_RETURN(subsurface);
   EINA_SAFETY_ON_NULL_RETURN(surface);

   wl_subsurface_place_above(subsurface->wl.subsurface, surface);
}

/**
 * @brief Places the subsurface below a sibling surface.
 *
 * This function changes the stacking order of the subsurface, placing it
 * below the specified sibling Wayland surface.
 *
 * @param subsurface The Ecore_Wl2_Subsurface to reorder.
 * @param surface The sibling wl_surface to place @p subsurface below.
 */
EAPI void
ecore_wl2_subsurface_place_below(Ecore_Wl2_Subsurface *subsurface, struct wl_surface *surface)
{
   EINA_SAFETY_ON_NULL_RETURN(subsurface);
   EINA_SAFETY_ON_NULL_RETURN(surface);

   wl_subsurface_place_below(subsurface->wl.subsurface, surface);
}

/**
 * @brief Sets the synchronization mode of the subsurface.
 *
 * If @p sync is @c EINA_TRUE, the subsurface is set to synchronized mode.
 * This means that changes to the subsurface buffer will be synchronized with
 * changes to the parent surface buffer.
 * If @p sync is @c EINA_FALSE, the subsurface is set to desynchronized mode.
 *
 * @param subsurface The Ecore_Wl2_Subsurface.
 * @param sync @c EINA_TRUE for synchronized mode, @c EINA_FALSE for desynchronized.
 */
EAPI void
ecore_wl2_subsurface_sync_set(Ecore_Wl2_Subsurface *subsurface, Eina_Bool sync)
{
   EINA_SAFETY_ON_NULL_RETURN(subsurface);
   EINA_SAFETY_ON_NULL_RETURN(subsurface->wl.subsurface);

   sync = !!sync;
   if (subsurface->sync == sync) return;

   subsurface->sync = sync;

   if (subsurface->sync)
     wl_subsurface_set_sync(subsurface->wl.subsurface);
   else
     wl_subsurface_set_desync(subsurface->wl.subsurface);
}

/**
 * @brief Sets the opaque region of the subsurface.
 *
 * This function informs the compositor about the opaque region of the
 * subsurface. The compositor can use this information to optimize rendering.
 * If @p w or @p h is zero or negative, the opaque region is cleared (set to NULL).
 *
 * @param subsurface The Ecore_Wl2_Subsurface.
 * @param x The x-coordinate of the opaque region.
 * @param y The y-coordinate of the opaque region.
 * @param w The width of the opaque region.
 * @param h The height of the opaque region.
 */
EAPI void
ecore_wl2_subsurface_opaque_region_set(Ecore_Wl2_Subsurface *subsurface, int x, int y, int w, int h)
{
   EINA_SAFETY_ON_NULL_RETURN(subsurface);
   EINA_SAFETY_ON_NULL_RETURN(subsurface->wl.subsurface);

   if ((w > 0) && (h > 0))
     {
        Ecore_Wl2_Window *parent;

        parent = subsurface->parent;
        if (parent)
          {
             struct wl_region *region;

             region =
               wl_compositor_create_region(parent->display->wl.compositor);
             if (!region)
               {
                  ERR("Failed to create opaque region");
                  return;
               }

             wl_region_add(region, x, y, w, h);
             wl_surface_set_opaque_region(subsurface->wl.surface, region);
             wl_region_destroy(region);
          }
     }
   else
     wl_surface_set_opaque_region(subsurface->wl.surface, NULL);
}
