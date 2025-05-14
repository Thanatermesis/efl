#ifndef EVAS_ENGINE_H
# define EVAS_ENGINE_H

# include <sys/ipc.h>
# include <sys/shm.h>

# include <X11/Xlib.h>
# include <X11/Xutil.h>
# include <X11/Xatom.h>
# include <X11/extensions/XShm.h>
# include <X11/Xresource.h> // xres - dpi

#include "../software_generic/Evas_Engine_Software_Generic.h"

extern int _evas_engine_soft_x11_log_dom;

# ifdef ERR
#  undef ERR
# endif
# define ERR(...) EINA_LOG_DOM_ERR(_evas_engine_soft_x11_log_dom, __VA_ARGS__)

# ifdef DBG
#  undef DBG
# endif
# define DBG(...) EINA_LOG_DOM_DBG(_evas_engine_soft_x11_log_dom, __VA_ARGS__)

# ifdef INF
#  undef INF
# endif
# define INF(...) EINA_LOG_DOM_INFO(_evas_engine_soft_x11_log_dom, __VA_ARGS__)

# ifdef WRN
#  undef WRN
# endif
# define WRN(...) EINA_LOG_DOM_WARN(_evas_engine_soft_x11_log_dom, __VA_ARGS__)

# ifdef CRI
#  undef CRI
# endif
# define CRI(...) \
   EINA_LOG_DOM_CRIT(_evas_engine_soft_x11_log_dom, __VA_ARGS__)

/**
 * @brief Structure representing an output buffer for rendering.
 *
 * This structure holds information about the rendering target, including its
 * dimensions, rotation, depth, and specific details related to the X11
 * backend (like Display, Window, Visual, Colormap, GC). It also manages
 * update regions and buffering strategies.
 */
struct _Outbuf
{
   Outbuf_Depth depth; /**< Output buffer depth setting. */
   int w, h;           /**< Width and height of the buffer. */
   int rot;            /**< Rotation angle (0, 90, 180, 270). */
   int onebuf;         /**< Flag indicating if a single buffer strategy is used. */

   struct
     {
        Convert_Pal *pal; /**< Palette for color conversion, if applicable. */
        union
          {
             /** X11 specific data */
             struct
               {
                  Display *disp;      /**< X11 Display connection. */
                  Window win;         /**< Target X11 Window. */
                  Pixmap mask;        /**< Optional shape mask Pixmap. */
                  Visual *vis;        /**< X11 Visual being used. */
                  Colormap cmap;      /**< X11 Colormap being used. */
                  int depth;          /**< Depth of the X11 drawable. */
                  int imdepth;        /**< Depth of the XImage used internally. */
                  int shm;            /**< Flag indicating if SHM extension is used. */
                  GC gc;              /**< Graphics Context for drawing. */
                  GC gcm;             /**< Graphics Context for mask operations. */
                  unsigned char swap : 1; /**< Byte swap needed flag. */
                  unsigned char bit_swap : 1; /**< Bit swap needed flag. */
               } xlib;
          } x11;
        struct
          {
             DATA32 r, g, b; /**< Mask color components (unused?). */
          } mask;

        /* 1 big buffer for updates - flush on idle_flush */
        RGBA_Image *onebuf; /**< The single buffer used if onebuf is enabled. */
        Eina_Array  onebuf_regions; /**< Array of Eina_Rectangle structs updated in the single buffer. Example: [{x=0, y=0, w=100, h=50}, {x=200, y=100, w=50, h=50}] */

        void *swapper; /**< Pointer to swapper data if double/triple buffering is used. */

        /* a list of pending regions to write to the target */
        Eina_List *pending_writes; /**< List of Eina_Rectangle structs scheduled for writing. Example: [{x=10, y=10, w=20, h=20}, ...] */

        /* a list of previous frame pending regions to write to the target */
        Eina_List *prev_pending_writes; /**< List of Eina_Rectangle structs from the previous frame, kept for synchronization. Example: [{x=10, y=10, w=20, h=20}, ...] */
        Eina_Spinlock lock; /**< Spinlock for thread safety on pending writes. */

        unsigned char mask_dither : 1; /**< Dithering enabled for shape mask. */
        unsigned char destination_alpha : 1; /**< Destination surface has an alpha channel. */
        unsigned char debug : 1; /**< Debugging enabled for this output buffer. */
        unsigned char synced : 1; /**< Synchronization status flag. */
     } priv; /**< Private data specific to the output buffer implementation. */
};

/**
 * @brief Initializes Xlib specific resources needed by the engine.
 *
 * This function should be called once before any Xlib operations are performed
 * by the Evas software X11 engine. It sets up necessary X atoms, etc.
 */
void evas_software_xlib_x_init(void);

#endif
