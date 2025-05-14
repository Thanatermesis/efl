#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/**
 * @file
 * @brief Ecore_Wl2 surface module for DMABUF support.
 *
 * This module implements the Ecore_Wl2_Surface_Interface for surfaces
 * backed by DMABUF buffers. It handles buffer allocation, management,
 * and rendering synchronization with the Wayland compositor.
 */

#include "Ecore_Wl2.h"
#include "ecore_wl2_internal.h"

#include <sys/types.h>
#include <sys/stat.h>

#include "linux-dmabuf-unstable-v1-client-protocol.h"

#define MAX_BUFFERS 4
#define QUEUE_TRIM_DURATION 100

int ECORE_WL2_SURFACE_DMABUF = 0;

/**
 * @brief Private data for a DMABUF-backed surface.
 *
 * This structure holds the state for managing DMABUF buffers associated
 * with an Ecore_Wl2_Surface.
 */
typedef struct _Ecore_Wl2_Dmabuf_Private
{
   Ecore_Wl2_Buffer *current; /**< The buffer currently being drawn to or recently posted. */
   Eina_List *buffers;      /**< A list of Ecore_Wl2_Buffer objects. */
   int unused_duration;     /**< Counter for how long unused buffers have existed, to trigger trimming. */
} Ecore_Wl2_Dmabuf_Private;

/**
 * @brief Sets up the DMABUF surface private data.
 * @param win The Ecore_Wl2_Window this surface belongs to.
 * @return A pointer to the allocated Ecore_Wl2_Dmabuf_Private structure, or NULL on failure.
 *
 * Initializes the necessary resources for DMABUF buffer management,
 * checking for SHM and DMABUF support in the display.
 */
static void *
_evas_dmabuf_surface_setup(Ecore_Wl2_Window *win)
{
   Ecore_Wl2_Dmabuf_Private *priv;
   Ecore_Wl2_Display *ewd;
   Ecore_Wl2_Buffer_Type types = 0;

   priv = calloc(1, sizeof(*priv));
   if (!priv) return NULL;

   ewd = ecore_wl2_window_display_get(win);
   if (ecore_wl2_display_shm_get(ewd))
     types |= ECORE_WL2_BUFFER_SHM;
   if (ecore_wl2_display_dmabuf_get(ewd))
     types |= ECORE_WL2_BUFFER_DMABUF;

   if (!ecore_wl2_buffer_init(ewd, types))
     {
        free(priv);
        return NULL;
     }

   return priv;
}

/**
 * @brief Reconfigures the DMABUF surface, typically due to a size change.
 * @param s The Ecore_Wl2_Surface being reconfigured.
 * @param priv_data Pointer to the Ecore_Wl2_Dmabuf_Private data.
 * @param w The new width of the surface.
 * @param h The new height of the surface.
 * @param flags Surface flags (unused in this implementation).
 * @param alpha Alpha property of the surface (unused in this implementation).
 *
 * This function is called when the surface needs to be resized. It iterates
 * through existing buffers and destroys them, as they are now incorrectly sized.
 * New buffers will be created on demand by _evas_dmabuf_surface_wait.
 */
static void
_evas_dmabuf_surface_reconfigure(Ecore_Wl2_Surface *s EINA_UNUSED, void *priv_data, int w, int h, uint32_t flags EINA_UNUSED, Eina_Bool alpha EINA_UNUSED)
{
   Ecore_Wl2_Dmabuf_Private *p;
   Ecore_Wl2_Buffer *b;
   Eina_List *l, *tmp;
//   Eina_Bool alpha_change;

   p = priv_data;

   if ((!w) || (!h)) return;
//   alpha_change = ecore_wl2_surface_alpha_get(s) != alpha;
   EINA_LIST_FOREACH_SAFE(p->buffers, l, tmp, b)
     {
/*      This would be nice, but requires a partial create to follow,
        and that partial create is buffer type specific.

        if (!alpha_change && ecore_wl2_buffer_fit(b, w, h))
          continue;
*/
        ecore_wl2_buffer_destroy(b);
        p->buffers = eina_list_remove_list(p->buffers, l);
     }
}

/**
 * @brief Gets a pointer to the pixel data of the current buffer.
 * @param s The Ecore_Wl2_Surface.
 * @param priv_data Pointer to the Ecore_Wl2_Dmabuf_Private data.
 * @param[out] w Pointer to store the width of the buffer data (stride / 4).
 * @param[out] h Pointer to store the height of the buffer data.
 * @return A pointer to the mapped pixel data of the current buffer, or NULL on failure.
 *
 * This function maps the current Ecore_Wl2_Buffer to make its pixel data
 * accessible to the CPU. The width returned is based on the stride, which
 * might be larger than the requested surface width.
 */
static void *
_evas_dmabuf_surface_data_get(Ecore_Wl2_Surface *s EINA_UNUSED, void *priv_data, int *w, int *h)
{
   Ecore_Wl2_Dmabuf_Private *p;
   Ecore_Wl2_Buffer *b;
   void *ptr;
   int stride;

   p = priv_data;

   b = p->current;
   if (!b) return NULL;

   ptr = ecore_wl2_buffer_map(b, NULL, h, &stride);
   if (!ptr) return NULL;

   /* We return stride/bpp because it may not match the allocated
    * width.  evas will figure out the clipping
    */
   if (w) *w = stride / 4;

   return ptr;
}

/**
 * @brief Waits for an available buffer or creates a new one.
 * @param s The Ecore_Wl2_Surface.
 * @param p Pointer to the Ecore_Wl2_Dmabuf_Private data.
 * @return The selected Ecore_Wl2_Buffer to be used for drawing, or NULL if none can be provided.
 *
 * This function implements a strategy to reuse buffers. It selects the
 * least recently used buffer that is not currently busy (i.e., not locked by
 * the compositor). If all buffers are busy and the maximum number of buffers
 * (MAX_BUFFERS) has not been reached, a new buffer is created.
 * It also implements a trimming mechanism: if unused buffers persist for
 * QUEUE_TRIM_DURATION cycles, the oldest one is destroyed to save resources.
 */
static Ecore_Wl2_Buffer *
_evas_dmabuf_surface_wait(Ecore_Wl2_Surface *s, Ecore_Wl2_Dmabuf_Private *p)
{
   Ecore_Wl2_Buffer *b, *best = NULL;
   Eina_List *l;
   int best_age = -1;
   int age;
   int num_required = 1, num_allocated = 0;

   EINA_LIST_FOREACH(p->buffers, l, b)
     {
        num_allocated++;
        if (ecore_wl2_buffer_busy_get(b))
          {
             num_required++;
             continue;
          }
        age = ecore_wl2_buffer_age_get(b);
        if (age > best_age)
          {
             best = b;
             best_age = age;
          }
     }

   if (num_required < num_allocated)
      p->unused_duration++;
   else
      p->unused_duration = 0;

   /* If we've had unused buffers for longer than QUEUE_TRIM_DURATION, then
    * destroy the oldest buffer (currently in best) and recursively call
    * ourself to get the next oldest.
    */
   if (best && (p->unused_duration > QUEUE_TRIM_DURATION))
     {
        p->unused_duration = 0;
        p->buffers = eina_list_remove(p->buffers, best);
        ecore_wl2_buffer_destroy(best);
        best = _evas_dmabuf_surface_wait(s, p);
     }

   if (!best && (eina_list_count(p->buffers) < MAX_BUFFERS))
     {
        best = ecore_wl2_surface_buffer_create(s);
        /* Start at -1 so it's age is incremented to 0 for first draw */
        ecore_wl2_buffer_age_set(best, -1);
        p->buffers = eina_list_append(p->buffers, best);
     }
   return best;
}

/**
 * @brief Assigns a buffer for the next rendering operation.
 * @param s The Ecore_Wl2_Surface.
 * @param priv_data Pointer to the Ecore_Wl2_Dmabuf_Private data.
 * @return The age of the assigned buffer, or 0 if no buffer could be assigned.
 *
 * This function calls _evas_dmabuf_surface_wait to get an available buffer
 * and sets it as the current buffer (p->current). It then increments the age
 * of all buffers in the pool. If no buffer is available (which should ideally
 * not happen), it resets the age of all buffers.
 */
static int
_evas_dmabuf_surface_assign(Ecore_Wl2_Surface *s, void *priv_data)
{
   Ecore_Wl2_Dmabuf_Private *p;
   Ecore_Wl2_Buffer *b;
   Eina_List *l;

   p = priv_data;
   p->current = _evas_dmabuf_surface_wait(s, p);
   if (!p->current)
     {
        /* Should be unreachable and will result in graphical
         * anomalies - we should probably blow away all the
         * existing buffers and start over if we actually
         * see this happen...
         */
//        WRN("No free DMAbuf buffers, dropping a frame");
        EINA_LIST_FOREACH(p->buffers, l, b)
          ecore_wl2_buffer_age_set(b, 0);
        return 0;
     }
   EINA_LIST_FOREACH(p->buffers, l, b)
     ecore_wl2_buffer_age_inc(b);

   return ecore_wl2_buffer_age_get(p->current);
}

/**
 * @brief Posts the current buffer to the Wayland compositor.
 * @param s The Ecore_Wl2_Surface.
 * @param priv_data Pointer to the Ecore_Wl2_Dmabuf_Private data.
 * @param rects Array of Eina_Rectangle structs defining the damaged regions.
 *              Example: rects[0] = { .x = 0, .y = 0, .w = 100, .h = 100 };
 * @param count The number of rectangles in the `rects` array.
 *
 * This function finalizes the rendering to the current buffer. It unlocks the
 * buffer, marks it as busy (as the compositor now owns it), resets its age,
 * attaches it to the Wayland surface, damages the specified regions, and
 * commits the surface changes to the compositor.
 */
static void
_evas_dmabuf_surface_post(Ecore_Wl2_Surface *s, void *priv_data, Eina_Rectangle *rects, unsigned int count)
{
   Ecore_Wl2_Dmabuf_Private *p;
   Ecore_Wl2_Buffer *b;
   Ecore_Wl2_Window *win;
   struct wl_buffer *wlb;

   p = priv_data;

   b = p->current;
   if (!b) return;

   ecore_wl2_buffer_unlock(b);

   p->current = NULL;
   ecore_wl2_buffer_busy_set(b);
   ecore_wl2_buffer_age_set(b, 0);

   win = ecore_wl2_surface_window_get(s);

   wlb = ecore_wl2_buffer_wl_buffer_get(b);
   ecore_wl2_window_buffer_attach(win, wlb, 0, 0, EINA_FALSE);
   ecore_wl2_window_damage(win, rects, count);

   ecore_wl2_window_commit(win, EINA_TRUE);
}

/**
 * @brief Destroys the DMABUF surface and its associated resources.
 * @param s The Ecore_Wl2_Surface (unused).
 * @param priv_data Pointer to the Ecore_Wl2_Dmabuf_Private data.
 *
 * This function is called when the Ecore_Wl2_Surface is being destroyed.
 * It frees all Ecore_Wl2_Buffer objects in the pool and then frees the
 * private data structure itself.
 */
static void
_evas_dmabuf_surface_destroy(Ecore_Wl2_Surface *s EINA_UNUSED, void *priv_data)
{
   Ecore_Wl2_Dmabuf_Private *p;
   Ecore_Wl2_Buffer *b;

   p = priv_data;

   EINA_LIST_FREE(p->buffers, b)
     ecore_wl2_buffer_destroy(b);

   free(p);
}

/**
 * @brief Flushes DMABUF surface resources.
 * @param surface The Ecore_Wl2_Surface (unused).
 * @param priv_data Pointer to the Ecore_Wl2_Dmabuf_Private data.
 * @param purge If EINA_TRUE, all buffers are destroyed. Otherwise, only non-busy buffers are destroyed.
 *
 * This function is used to release buffer resources. Depending on the `purge`
 * flag, it either destroys all buffers or only those not currently locked by
 * the compositor. This can be used to free memory, for example, when the
 * surface is hidden.
 */
static void
_evas_dmabuf_surface_flush(Ecore_Wl2_Surface *surface EINA_UNUSED, void *priv_data, Eina_Bool purge)
{
   Ecore_Wl2_Dmabuf_Private *p;
   Ecore_Wl2_Buffer *b;

   p = priv_data;

   EINA_LIST_FREE(p->buffers, b)
     {
        if (purge || !ecore_wl2_buffer_busy_get(b))
          {
             if (p->current == b)
               p->current = NULL;
             ecore_wl2_buffer_destroy(b);
          }
     }
}

/**
 * @brief The Ecore_Wl2_Surface_Interface implementation for DMABUF surfaces.
 *
 * This structure maps the internal DMABUF handling functions to the
 * interface expected by the Ecore_Wl2 surface management system.
 */
static Ecore_Wl2_Surface_Interface dmabuf_smanager =
{
   .version = 1,
   .setup = _evas_dmabuf_surface_setup,
   .destroy = _evas_dmabuf_surface_destroy,
   .reconfigure = _evas_dmabuf_surface_reconfigure,
   .data_get = _evas_dmabuf_surface_data_get,
   .assign = _evas_dmabuf_surface_assign,
   .post = _evas_dmabuf_surface_post,
   .flush = _evas_dmabuf_surface_flush
};

/**
 * @brief Initializes the DMABUF surface module.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 *
 * Registers the DMABUF surface interface (`dmabuf_smanager`) with the
 * Ecore_Wl2 surface management system.
 */
Eina_Bool
ecore_wl2_surface_module_dmabuf_init(void)
{
   ECORE_WL2_SURFACE_DMABUF = ecore_wl2_surface_manager_add(&dmabuf_smanager);

   if (ECORE_WL2_SURFACE_DMABUF < 1)
     return EINA_FALSE;

   return EINA_TRUE;
}

/**
 * @brief Shuts down the DMABUF surface module.
 *
 * Unregisters the DMABUF surface interface from the Ecore_Wl2 surface
 * management system.
 */
void
ecore_wl2_surface_module_dmabuf_shutdown(void)
{
   ecore_wl2_surface_manager_del(&dmabuf_smanager);
}

EINA_MODULE_INIT(ecore_wl2_surface_module_dmabuf_init);
EINA_MODULE_SHUTDOWN(ecore_wl2_surface_module_dmabuf_shutdown);

