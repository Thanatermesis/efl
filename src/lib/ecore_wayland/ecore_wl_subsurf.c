#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "ecore_wl_private.h"

/*
 * The subsurface protocol was moved into Wayland Core
 * around v1.3.90 (i.e. v1.4.0).
 * Test if subsurface protocol is part of wayland-client.h.
 * If not, we include our own copy of the protocol header.
 */
#include <wayland-client.h>
#ifndef WL_SUBSURFACE_ERROR_ENUM
# include <subsurface-client-protocol.h>
#endif

/**
 * @brief Represents a Wayland subsurface.
 *
 * This structure holds the necessary information for managing a subsurface,
 * including its parent window, Wayland surface and subsurface objects,
 * position, and synchronization state.
 */
struct _Ecore_Wl_Subsurf
{
   EINA_INLIST; /**< Macro for Eina_Inlist node */
   Ecore_Wl_Window *parent_win; /**< The parent Ecore_Wl_Window */
   struct wl_surface *surface; /**< The Wayland surface associated with this subsurface */
   struct wl_subsurface *subsurface; /**< The Wayland subsurface object */

   int x, y; /**< The x and y position of the subsurface relative to its parent */

   Eina_Bool sync : 1; /**< Flag indicating if the subsurface is synchronized with its parent */
};

/**
 * @brief Creates a new subsurface for a given Ecore Wayland window.
 *
 * This function creates a new Wayland surface and then a Wayland subsurface
 * object associated with the parent window's surface. The new subsurface is
 * added to the parent window's list of subsurfaces.
 *
 * @param win The parent Ecore_Wl_Window to which this subsurface will belong.
 * @return A pointer to the newly created Ecore_Wl_Subsurf on success,
 *         or NULL on failure (e.g., if the subcompositor is not available,
 *         or surface/subsurface creation fails).
 */
EAPI Ecore_Wl_Subsurf *
ecore_wl_subsurf_create(Ecore_Wl_Window *win)
{
   struct wl_subsurface *subsurface;
   struct wl_surface *surface;
   Ecore_Wl_Subsurf *ess;
   struct wl_subcompositor *subcomp;

   LOGFN;

   if (!win) return NULL;
   if (!win->surface) return NULL;

   subcomp = _ecore_wl_subcompositor_get();
   if (!subcomp) return NULL;

   surface = wl_compositor_create_surface(_ecore_wl_compositor_get());
   if (!surface)
     return NULL;

   subsurface = wl_subcompositor_get_subsurface
      (subcomp, surface, win->surface);
   if (!subsurface)
     {
        wl_surface_destroy(surface);
        return NULL;
     }

   ess = calloc(1, sizeof(*ess));
   ess->surface = surface;
   ess->subsurface = subsurface;
   ess->parent_win = win;

   win->subsurfs = (Ecore_Wl_Subsurf *)eina_inlist_append
      (EINA_INLIST_GET(win->subsurfs), EINA_INLIST_GET(ess));

   return ess;
}

/**
 * @internal
 * @brief Destroys a subsurface and cleans up its resources.
 *
 * This function destroys the Wayland subsurface and surface objects,
 * removes the subsurface from its parent window's list, and frees
 * the Ecore_Wl_Subsurf structure.
 *
 * @param ess The Ecore_Wl_Subsurf to destroy.
 */
static void
_ecore_wl_subsurf_destroy(Ecore_Wl_Subsurf *ess)
{
   Ecore_Wl_Window *parent;

   if (!ess) return;

   if (ess->subsurface)
      wl_subsurface_destroy(ess->subsurface);

   if (ess->surface)
      wl_surface_destroy(ess->surface);

   parent = ess->parent_win;
   parent->subsurfs = (Ecore_Wl_Subsurf *)eina_inlist_remove
      (EINA_INLIST_GET(parent->subsurfs), EINA_INLIST_GET(ess));

   free(ess);
}

/**
 * @brief Deletes an Ecore Wayland subsurface.
 *
 * This function is a public wrapper around _ecore_wl_subsurf_destroy.
 * It ensures that the provided subsurface pointer is valid before
 * attempting to destroy it.
 *
 * @param ess The Ecore_Wl_Subsurf to delete.
 */
EAPI void
ecore_wl_subsurf_del(Ecore_Wl_Subsurf *ess)
{
   LOGFN;

   if (!ess) return;

   _ecore_wl_subsurf_destroy(ess);
}

/**
 * @internal
 * @brief Deletes all subsurfaces associated with a given Ecore Wayland window.
 *
 * This function iterates through the list of subsurfaces for the specified
 * window and destroys each one.
 *
 * @param win The Ecore_Wl_Window whose subsurfaces are to be deleted.
 */
void
_ecore_wl_subsurfs_del_all(Ecore_Wl_Window *win)
{
   Ecore_Wl_Subsurf *ess;

   if (!win) return;

   EINA_INLIST_FREE(win->subsurfs, ess)
     {
        _ecore_wl_subsurf_destroy(ess);
     }
}

/**
 * @brief Retrieves the Wayland surface associated with an Ecore subsurface.
 *
 * @param ess The Ecore_Wl_Subsurf from which to get the surface.
 * @return A pointer to the wl_surface object, or NULL if ess is invalid.
 */
EAPI struct wl_surface *
ecore_wl_subsurf_surface_get(Ecore_Wl_Subsurf *ess)
{
   LOGFN;

   if (!ess) return NULL;

   return ess->surface;
}

/**
 * @brief Sets the position of the subsurface relative to its parent surface.
 *
 * @param ess The Ecore_Wl_Subsurf to position.
 * @param x The new x-coordinate.
 * @param y The new y-coordinate.
 */
EAPI void
ecore_wl_subsurf_position_set(Ecore_Wl_Subsurf *ess, int x, int y)
{
   LOGFN;

   if (!ess) return;
   if (!ess->subsurface) return;

   if ((x == ess->x) && (y == ess->y))
     return;

   ess->x = x;
   ess->y = y;

   wl_subsurface_set_position(ess->subsurface, x, y);
}

/**
 * @brief Gets the current position of the subsurface.
 *
 * The position is relative to the parent surface.
 *
 * @param ess The Ecore_Wl_Subsurf whose position is to be retrieved.
 * @param x Pointer to an integer where the x-coordinate will be stored. Can be NULL.
 * @param y Pointer to an integer where the y-coordinate will be stored. Can be NULL.
 */
EAPI void
ecore_wl_subsurf_position_get(Ecore_Wl_Subsurf *ess, int *x, int *y)
{
   LOGFN;

   if (!ess) return;

   if (x) *x = ess->x;
   if (y) *y = ess->y;
}

/**
 * @brief Places the subsurface above a sibling Wayland surface.
 *
 * This function changes the stacking order of the subsurface.
 * The `surface` parameter must be a sibling of the subsurface's parent surface.
 *
 * @param ess The Ecore_Wl_Subsurf to reorder.
 * @param surface The sibling wl_surface above which to place ess.
 */
EAPI void
ecore_wl_subsurf_place_above(Ecore_Wl_Subsurf *ess, struct wl_surface *surface)
{
   LOGFN;

   if (!ess) return;
   if (!surface) return;
   if (!ess->subsurface) return;

   wl_subsurface_place_above(ess->subsurface, surface);
}

/**
 * @brief Places the subsurface below a sibling Wayland surface.
 *
 * This function changes the stacking order of the subsurface.
 * The `surface` parameter must be a sibling of the subsurface's parent surface.
 *
 * @param ess The Ecore_Wl_Subsurf to reorder.
 * @param surface The sibling wl_surface below which to place ess.
 */
EAPI void
ecore_wl_subsurf_place_below(Ecore_Wl_Subsurf *ess, struct wl_surface *surface)
{
   LOGFN;

   if (!ess) return;
   if (!surface) return;
   if (!ess->subsurface) return;

   wl_subsurface_place_below(ess->subsurface, surface);
}

/**
 * @brief Sets the synchronization state of the subsurface.
 *
 * If synchronized (val = EINA_TRUE), the subsurface's surface contents
 * are updated synchronously with the parent surface. If desynchronized
 * (val = EINA_FALSE), the subsurface can be updated independently.
 *
 * @param ess The Ecore_Wl_Subsurf whose synchronization state is to be set.
 * @param val EINA_TRUE to synchronize, EINA_FALSE to desynchronize.
 */
EAPI void
ecore_wl_subsurf_sync_set(Ecore_Wl_Subsurf *ess, Eina_Bool val)
{
   LOGFN;

   if (!ess) return;
   if (!ess->subsurface) return;

   val = !!val;
   if (val == ess->sync) return;

   ess->sync = val;

   if (ess->sync)
     wl_subsurface_set_sync(ess->subsurface);
   else
     wl_subsurface_set_desync(ess->subsurface);
}

/**
 * @brief Sets the opaque region of the subsurface.
 *
 * This function informs the compositor about the opaque region of the
 * subsurface. This can be used by the compositor for optimizations.
 * If w or h is zero or negative, the opaque region is set to NULL (meaning
 * no opaque region or the entire surface is transparent).
 *
 * @param ess The Ecore_Wl_Subsurf for which to set the opaque region.
 * @param x The x-coordinate of the opaque region.
 * @param y The y-coordinate of the opaque region.
 * @param w The width of the opaque region.
 * @param h The height of the opaque region.
 */
EAPI void
ecore_wl_subsurf_opaque_region_set(Ecore_Wl_Subsurf *ess, int x, int y, int w, int h)
{
   struct wl_region *region = NULL;

   LOGFN;

   if (!ess) return;
   if (!ess->surface) return;

   if ((w > 0) && (h > 0))
     {
        region = wl_compositor_create_region(_ecore_wl_compositor_get());
        if (!region) return;

        wl_region_add(region, x, y, w, h);
        wl_surface_set_opaque_region(ess->surface, region);
        wl_region_destroy(region);
     }
   else
     wl_surface_set_opaque_region(ess->surface, NULL);
}
