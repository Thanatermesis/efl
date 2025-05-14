#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "ecore_wl2_private.h"

#include <sys/types.h>
#include <sys/stat.h>

/**
 * @internal
 * @brief List of registered surface managers.
 *
 * This list holds pointers to Ecore_Wl2_Surface_Interface implementations,
 * allowing different mechanisms for surface creation and management.
 */
static Eina_List *_smanagers = NULL;

/**
 * @internal
 * @brief Counter for assigning unique IDs to surface managers.
 */
static int _smanager_count = 0;

/**
 * @brief Destroys a Wayland surface.
 *
 * This function cleans up resources associated with the given Ecore_Wl2_Surface.
 * It removes event handlers, calls the underlying surface manager's destroy function,
 * and frees the surface structure. It also releases a reference to ecore_wl2
 * that was taken during surface creation.
 *
 * @param surface The Ecore_Wl2_Surface to destroy.
 */
EAPI void
ecore_wl2_surface_destroy(Ecore_Wl2_Surface *surface)
{
   EINA_SAFETY_ON_NULL_RETURN(surface);

   ecore_event_handler_del(surface->offscreen_handler);
   surface->funcs->destroy(surface, surface->private_data);
   surface->wl2_win->wl2_surface = NULL;
   surface->wl2_win = NULL;

   free(surface);
   /* We took a reference to ecore_wl2 in surface create to prevent
    * modules unloading with surfaces in flight.  Release that now.
    */
   ecore_wl2_shutdown();
}

/**
 * @brief Reconfigures a Wayland surface.
 *
 * This function updates the dimensions, flags, and alpha properties of an
 * existing surface. It calls the underlying surface manager's reconfigure
 * function to apply the changes.
 *
 * @param surface The Ecore_Wl2_Surface to reconfigure.
 * @param w The new width of the surface.
 * @param h The new height of the surface.
 * @param flags Custom flags for the surface (specific to the surface manager).
 * @param alpha EINA_TRUE if the surface should support alpha, EINA_FALSE otherwise.
 */
EAPI void
ecore_wl2_surface_reconfigure(Ecore_Wl2_Surface *surface, int w, int h, uint32_t flags, Eina_Bool alpha)
{
   EINA_SAFETY_ON_NULL_RETURN(surface);

   surface->funcs->reconfigure(surface, surface->private_data, w, h, flags, alpha);
   surface->w = w;
   surface->h = h;
   surface->alpha = alpha;
}

/**
 * @brief Retrieves a pointer to the surface's pixel data.
 *
 * This function allows access to the raw pixel data of the surface. The format
 * of this data depends on the underlying surface manager.
 *
 * @param surface The Ecore_Wl2_Surface to get data from.
 * @param w Pointer to an integer where the width of the surface will be stored. Can be NULL.
 * @param h Pointer to an integer where the height of the surface will be stored. Can be NULL.
 * @return A pointer to the surface's pixel data, or NULL on failure.
 *         The data format is dependent on the surface manager.
 */
EAPI void *
ecore_wl2_surface_data_get(Ecore_Wl2_Surface *surface, int *w, int *h)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(surface, NULL);

   return surface->funcs->data_get(surface, surface->private_data, w, h);
}

/**
 * @brief Assigns the surface's content to its underlying Wayland buffer.
 *
 * This function typically prepares the surface's content to be displayed.
 * The exact behavior is defined by the surface manager.
 *
 * @param surface The Ecore_Wl2_Surface to assign.
 * @return An integer status code from the surface manager, typically 1 on success, 0 on failure.
 */
EAPI int
ecore_wl2_surface_assign(Ecore_Wl2_Surface *surface)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(surface, 0);

   return surface->funcs->assign(surface, surface->private_data);
}

/**
 * @brief Posts updates to the Wayland surface.
 *
 * This function informs the Wayland compositor about regions of the surface
 * that have been updated and need to be redrawn.
 *
 * @param surface The Ecore_Wl2_Surface to post updates for.
 * @param rects An array of Eina_Rectangle structures defining the updated regions.
 *              Example:
 *              @code
 *              Eina_Rectangle updates[2];
 *              updates[0].x = 0; updates[0].y = 0; updates[0].w = 10; updates[0].h = 10;
 *              updates[1].x = 20; updates[1].y = 20; updates[1].w = 5; updates[1].h = 5;
 *              ecore_wl2_surface_post(surface, updates, 2);
 *              @endcode
 * @param count The number of rectangles in the @p rects array.
 */
EAPI void
ecore_wl2_surface_post(Ecore_Wl2_Surface *surface, Eina_Rectangle *rects, unsigned int count)
{
   EINA_SAFETY_ON_NULL_RETURN(surface);

   surface->funcs->post(surface, surface->private_data, rects, count);
}

/**
 * @brief Flushes pending operations for the Wayland surface.
 *
 * This function ensures that all pending drawing commands or updates are
 * sent to the Wayland compositor.
 *
 * @param surface The Ecore_Wl2_Surface to flush.
 * @param purge If EINA_TRUE, indicates that any cached data should be discarded.
 *              The exact meaning of purge can be specific to the surface manager.
 */
EAPI void
ecore_wl2_surface_flush(Ecore_Wl2_Surface *surface, Eina_Bool purge)
{
   EINA_SAFETY_ON_NULL_RETURN(surface);

   surface->funcs->flush(surface, surface->private_data, purge);
}

/**
 * @internal
 * @brief Callback for ECORE_WL2_EVENT_WINDOW_OFFSCREEN events.
 *
 * This function is triggered when a window associated with a surface goes
 * offscreen. It flushes the surface to ensure its content is up-to-date.
 *
 * @param data The Ecore_Wl2_Surface associated with this callback.
 * @param type The type of the event (unused).
 * @param event The Ecore_Wl2_Event_Window_Offscreen event data.
 * @return ECORE_CALLBACK_RENEW to keep the handler active.
 */
static Eina_Bool
_ecore_wl2_surface_cb_offscreen(void *data, int type EINA_UNUSED, void *event)
{
   Ecore_Wl2_Event_Window_Offscreen *ev = event;
   Ecore_Wl2_Surface *surf = data;

   if (surf->wl2_win == ev->win)
      ecore_wl2_surface_flush(surf, EINA_FALSE);

   return ECORE_CALLBACK_RENEW;
}

/**
 * @brief Creates a new Wayland surface for a given window.
 *
 * This function iterates through available surface managers and attempts to
 * create a surface using one of them. If successful, it associates the surface
 * with the window and sets up an event handler for offscreen events.
 * It also takes a reference to ecore_wl2 to prevent module unloading
 * while the surface is active.
 *
 * @param win The Ecore_Wl2_Window for which to create the surface.
 * @param alpha EINA_TRUE if the surface should support alpha, EINA_FALSE otherwise.
 * @return A pointer to the newly created Ecore_Wl2_Surface, or NULL on failure.
 */
EAPI Ecore_Wl2_Surface *
ecore_wl2_surface_create(Ecore_Wl2_Window *win, Eina_Bool alpha)
{
   Ecore_Wl2_Surface *out;
   Eina_List *l;
   Ecore_Wl2_Surface_Interface *intf;

   EINA_SAFETY_ON_NULL_RETURN_VAL(win, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(_smanagers, NULL);

   if (win->wl2_surface) return win->wl2_surface;

   out = calloc(1, sizeof(*out));
   if (!out) return NULL;

   out->wl2_win = win;
   out->alpha = alpha;
   out->w = 0;
   out->h = 0;

   EINA_LIST_FOREACH(_smanagers, l, intf)
     {
        out->private_data = intf->setup(win);
        if (out->private_data)
          {
             out->funcs = intf;
             win->wl2_surface = out;
             out->offscreen_handler =
               ecore_event_handler_add(ECORE_WL2_EVENT_WINDOW_OFFSCREEN,
                                       _ecore_wl2_surface_cb_offscreen,
                                       out);
             /* Since we have loadable modules, we need to make sure this
              * surface keeps ecore_wl2 from de-initting and dlclose()ing
              * things until after it's destroyed
              */
             ecore_wl2_init();
             return out;
          }
     }

   free(out);
   return NULL;
}

/**
 * @brief Creates a new Ecore_Wl2_Buffer for a given surface.
 *
 * This function creates a buffer that matches the dimensions and alpha
 * properties of the specified surface. This buffer can then be used for
 * rendering content that will be displayed on the surface.
 *
 * @param surface The Ecore_Wl2_Surface for which to create the buffer.
 *                The buffer will be created with the surface's current width,
 *                height, and alpha setting.
 * @return A pointer to the newly created Ecore_Wl2_Buffer, or NULL on failure.
 */
EAPI Ecore_Wl2_Buffer *
ecore_wl2_surface_buffer_create(Ecore_Wl2_Surface *surface)
{
   Ecore_Wl2_Display *ewd;

   EINA_SAFETY_ON_NULL_RETURN_VAL(surface, NULL);

   ewd = ecore_wl2_window_display_get(surface->wl2_win);
   EINA_SAFETY_ON_NULL_RETURN_VAL(ewd, NULL);

   return ecore_wl2_buffer_create(ewd, surface->w, surface->h, surface->alpha);
}

/**
 * @brief Adds a new surface manager to the list of available managers.
 *
 * Surface managers provide different implementations for creating and
 * managing Wayland surfaces (e.g., shm, dmabuf). This function allows
 * modules to register their surface management capabilities.
 *
 * @param intf A pointer to the Ecore_Wl2_Surface_Interface implementation.
 *             The interface must have a version compatible with
 *             ECORE_WL2_SURFACE_INTERFACE_VERSION.
 * @return The ID assigned to the new surface manager on success, or 0 on failure
 *         (e.g., if the interface version is incompatible).
 */
EAPI int
ecore_wl2_surface_manager_add(Ecore_Wl2_Surface_Interface *intf)
{
   if (intf->version < ECORE_WL2_SURFACE_INTERFACE_VERSION)
     return 0;

   _smanagers = eina_list_prepend(_smanagers, intf);
   intf->id = ++_smanager_count;
   return intf->id;
}

/**
 * @brief Removes a surface manager from the list of available managers.
 *
 * This function is used to unregister a surface manager, typically when
 * a module providing that manager is unloaded.
 *
 * @param intf The Ecore_Wl2_Surface_Interface to remove.
 */
EAPI void
ecore_wl2_surface_manager_del(Ecore_Wl2_Surface_Interface *intf)
{
   _smanagers = eina_list_remove(_smanagers, intf);
}

/**
 * @brief Retrieves the Ecore_Wl2_Window associated with a surface.
 *
 * @param surface The Ecore_Wl2_Surface whose window is to be retrieved.
 * @return A pointer to the Ecore_Wl2_Window associated with the surface,
 *         or NULL if @p surface is NULL.
 */
EAPI Ecore_Wl2_Window *
ecore_wl2_surface_window_get(Ecore_Wl2_Surface *surface)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(surface, NULL);

   return surface->wl2_win;
}

/**
 * @brief Gets the alpha property of the Wayland surface.
 *
 * @param surface The Ecore_Wl2_Surface to query.
 * @return EINA_TRUE if the surface supports alpha, EINA_FALSE otherwise.
 *         Returns EINA_FALSE if @p surface is NULL.
 */
EAPI Eina_Bool
ecore_wl2_surface_alpha_get(Ecore_Wl2_Surface *surface)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(surface, EINA_FALSE);

   return surface->alpha;
}
