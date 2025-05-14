#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/**
 * @file evas_xlib_swapper.c
 * @brief Implementation of Xlib drawable swapping mechanisms.
 *
 * This file provides two main implementations for buffer swapping:
 * 1. XSHM/XImage-based: A fallback mechanism using X Shared Memory Extension
 *    or standard XImage for buffer management if DRI2 is not available or fails.
 *    This part is conditionally compiled with `#if 0` currently, suggesting it might
 *    be legacy or experimental.
 * 2. DRI2-based: A more modern approach using Direct Rendering Infrastructure 2
 *    for efficient buffer swapping, typically leveraging hardware acceleration.
 *    This relies on dynamic loading of libdrm, libtbm, libdri2, and libXfixes.
 *
 * The choice of implementation depends on the `HAVE_DLSYM` macro.
 * If `HAVE_DLSYM` is not defined, stub functions are provided.
 */

#include "evas_common_private.h"
#include "evas_macros.h"
#include "evas_xlib_swapper.h"

#ifdef HAVE_DLSYM
# include <dlfcn.h> /* dlopen,dlclose,etc */
# include <sys/types.h>
# include <sys/stat.h>
# include <fcntl.h>

#if 0
// X(shm)image emulation of multiple buffers + swapping /////////////////////
/**
 * @brief Represents a single buffer in the XSHM/XImage swapping mechanism.
 * @internal
 *
 * This structure holds an XImage, its associated shared memory information (if applicable),
 * pixel data, dimensions, and status flags.
 */
typedef struct
{
   XImage         *xim; /**< The XImage structure. */
   XShmSegmentInfo shm_info; /**< X Shared Memory segment information. */
   void           *data; /**< Pointer to the raw pixel data. */
   int             w, h, bpl; /**< Width, height, and bytes per line of the buffer. */
   Eina_Bool       shm : 1; /**< EINA_TRUE if using XSHM, EINA_FALSE otherwise. */
   Eina_Bool       valid : 1; /**< EINA_TRUE if the buffer contains valid data. */
} Buffer;

/**
 * @brief Main structure for the XSHM/XImage swapper.
 * @internal
 *
 * Contains the X display, drawable, visual, GC, an array of buffers,
 * dimensions, and state for managing buffer swapping.
 */
struct _X_Swapper
{
   Display  *disp; /**< The X display connection. */
   Drawable  draw; /**< The X drawable. */
   Visual   *vis; /**< The X visual. */
   GC        gc; /**< The X Graphics Context. */
   Buffer    buf[3]; /**< Array of buffers (e.g., for triple buffering). */
   int       w, h, depth; /**< Width, height, and depth of the drawable. */
   int       buf_cur, buf_num; /**< Current buffer index and total number of buffers. */
   Eina_Bool mapped : 1; /**< EINA_TRUE if a buffer is currently mapped for drawing. */
};

static Eina_Bool _x_err = EINA_FALSE; /**< Global flag to track X errors during SHM operations. */

/**
 * @brief X error handler for SHM operations.
 * @internal
 * Sets a global flag if an X error occurs. This is used to gracefully
 * handle failures during XShmAttach.
 * @param d The display where the error occurred (unused).
 * @param ev The XErrorEvent structure (unused).
 */
static void
_x_err_hand(Display *d EINA_UNUSED, XErrorEvent *ev EINA_UNUSED)
{
   _x_err = 1;
   return;
}

/**
 * @brief Creates a new buffer (XImage, possibly with SHM).
 * @internal
 * Attempts to create an XShmImage first. If that fails, falls back to a regular XCreateImage.
 *
 * @param swp The X_Swapper instance.
 * @param buf Pointer to the Buffer structure to initialize.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
_buf_new(X_Swapper *swp, Buffer *buf)
{
   buf->w = swp->w;
   buf->h = swp->h;

   // try shm first
   buf->xim = XShmCreateImage(swp->disp, swp->vis, swp->depth, ZPixmap,
                              NULL, &(buf->shm_info), buf->w, buf->h);
   if (buf->xim)
     {
        buf->bpl = buf->xim->bytes_per_line;

        buf->shm_info.shmid = shmget(IPC_PRIVATE, buf->bpl * buf->h,
                                     IPC_CREAT | 0600);
        if (buf->shm_info.shmid >= 0)
          {
             buf->shm_info.readOnly = False;
             buf->shm_info.shmaddr = buf->data = buf->xim->data =
                   shmat(buf->shm_info.shmid, 0, 0);
             if (buf->shm_info.shmaddr != ((void *)-1))
               {
                  XErrorHandler ph;

                  XSync(swp->disp, False);
                  _x_err = EINA_FALSE;
                  ph = XSetErrorHandler((XErrorHandler)_x_err_hand);
                  XShmAttach(swp->disp, &(buf->shm_info));
                  XSync(swp->disp, False);
                  XSetErrorHandler((XErrorHandler)ph);
                  if (!_x_err)
                    {
                       buf->shm = EINA_TRUE;
                    }
                  else
                    {
                       shmdt(buf->shm_info.shmaddr);
                       shmctl(buf->shm_info.shmid, IPC_RMID, 0);
                       XDestroyImage(buf->xim);
                       buf->xim = NULL;
                    }
               }
             else
               {
                  shmdt(buf->shm_info.shmaddr);
                  shmctl(buf->shm_info.shmid, IPC_RMID, 0);
                  XDestroyImage(buf->xim);
                  buf->xim = NULL;
               }
          }
        else
          {
             XDestroyImage(buf->xim);
             buf->xim = NULL;
          }
     }

   if (!buf->xim) // shm failed - try normal ximage
     {
        buf->xim = XCreateImage(swp->disp, swp->vis, swp->depth, ZPixmap,
                                0, NULL, buf->w, buf->h, 32, 0);
        if (!buf->xim) return EINA_FALSE;
        buf->bpl = buf->xim->bytes_per_line;
        buf->data = buf->xim->data = malloc(buf->bpl * buf->h);
        if (!buf->data)
          {
             XDestroyImage(buf->xim);
             buf->xim = NULL;
             return EINA_FALSE;
          }
     }
   return EINA_TRUE;
}

/**
 * @brief Frees resources associated with a buffer.
 * @internal
 * Detaches SHM segment (if used), destroys the XImage, and frees allocated memory.
 *
 * @param swp The X_Swapper instance.
 * @param buf Pointer to the Buffer structure to free.
 */
static void
_buf_free(X_Swapper *swp, Buffer *buf)
{
   if (!buf->xim) return;
   if (buf->shm)
     {
        XShmDetach(swp->disp, &(buf->shm_info));
        XDestroyImage(buf->xim);
        shmdt(buf->shm_info.shmaddr);
        shmctl(buf->shm_info.shmid, IPC_RMID, 0);
     }
   else
     {
        XDestroyImage(buf->xim);
     }
   buf->xim = NULL;
   buf->shm = EINA_FALSE;
}

/**
 * @brief Puts the buffer content to the drawable.
 * @internal
 * Uses XShmPutImage or XPutImage to transfer pixel data from the buffer
 * to the X drawable, updating specified rectangular regions.
 *
 * @param swp The X_Swapper instance.
 * @param buf The Buffer to put.
 * @param rects Array of Eina_Rectangle defining regions to update.
 *              If NULL, the entire buffer is updated.
 * @param nrects Number of rectangles in `rects`.
 */
static void
_buf_put(X_Swapper *swp, Buffer *buf, Eina_Rectangle *rects, int nrects)
{
   Region tmpr;
   int i;

   if (!buf->xim) return;
   tmpr = XCreateRegion();
   if ((rects) /* && 0*/) // set to 0 to test buffer stuff
     {
        for (i = 0; i < nrects; i++)
          {
             XRectangle xr;

             xr.x = rects[i].x; xr.y = rects[i].y;
             xr.width = rects[i].w; xr.height = rects[i].h;
             XUnionRectWithRegion(&xr, tmpr, tmpr);
          }
     }
   else
     {
        XRectangle xr;

        xr.x = 0; xr.y = 0;
        xr.width = buf->w; xr.height = buf->h;
        XUnionRectWithRegion(&xr, tmpr, tmpr);
     }
   XSetRegion(swp->disp, swp->gc, tmpr);
   XDestroyRegion(tmpr);
   if (buf->shm)
     {
        XShmPutImage(swp->disp, swp->draw, swp->gc, buf->xim, 0, 0, 0, 0,
                     buf->w, buf->h, False);
     }
   else
     {
        XPutImage(swp->disp, swp->draw, swp->gc, buf->xim, 0, 0, 0, 0,
                  buf->w, buf->h);
     }
   XSync(swp->disp, False);
}

//////////////////////////////////////////////////////////////////////////////

X_Swapper *
evas_xlib_swapper_new(Display *disp, Drawable draw, Visual *vis,
                      int depth, int w, int h)
{
   X_Swapper *swp;
   XGCValues gcv;
   int i;

   int nbuf = 3; // pretend we are triple buffered

   swp = calloc(1, sizeof(X_Swapper));
   if (!swp) return NULL;
   swp->disp = disp;
   swp->draw = draw;
   swp->vis = vis;
   swp->depth = depth;
   swp->w = w;
   swp->h = h;
   swp->buf_num = nbuf;
   swp->gc = XCreateGC(swp->disp, swp->draw, 0, &gcv);
   for (i = 0; i < swp->buf_num; i++)
     {
        if (!_buf_new(swp, &(swp->buf[i])))
          {
             evas_xlib_swapper_free(swp);
             return NULL;
          }
     }
   return swp;
}

void
evas_xlib_swapper_free(X_Swapper *swp)
{
   int i;

   for (i = 0; i < swp->buf_num; i++)
     {
        _buf_free(swp, &(swp->buf[i]));
     }
   XFreeGC(swp->disp, swp->gc);
   free(swp);
}

void *
evas_xlib_swapper_buffer_map(X_Swapper *swp, int *bpl, int *w, int *h)
{
   swp->mapped = EINA_TRUE;
   if (bpl) *bpl = swp->buf[swp->buf_cur].bpl;
   if (w) *w = swp->w;
   if (h) *h = swp->h;
   return swp->buf[swp->buf_cur].data;
}

void
evas_xlib_swapper_buffer_unmap(X_Swapper *swp)
{
   swp->mapped = EINA_FALSE;
}

void
evas_xlib_swapper_swap(X_Swapper *swp, Eina_Rectangle *rects, int nrects)
{
   int n;

   n = swp->buf_cur;
   _buf_put(swp, &(swp->buf[n]), rects, nrects);
   swp->buf[n].valid = 1;
   swp->buf_cur = (swp->buf_cur + 1) % swp->buf_num;
}

Render_Output_Swap_Mode
evas_xlib_swapper_buffer_state_get(X_Swapper *swp)
{
   int i, n, count = 0;
/*
   for (i = 0; i < swp->buf_num; i++)
     {
        if ((rand() % 50) == 0)
          swp->buf[n].valid = 0;
     }
 */
   for (i = 0; i < swp->buf_num; i++)
     {
        n = (swp->buf_num + swp->buf_cur - (i)) % swp->buf_num;
        if (swp->buf[n].valid) count++;
        else break;
     }
   if (count == swp->buf_num)
     {
        if (count == 1) return MODE_COPY;
        else if (count == 2)
          return MODE_DOUBLE;
        else if (count == 3)
          return MODE_TRIPLE;
     }
   return MODE_FULL;
}

int
evas_xlib_swapper_depth_get(X_Swapper *swp)
{
   return swp->buf[0].xim->bits_per_pixel;
}

int
evas_xlib_swapper_byte_order_get(X_Swapper *swp)
{
   return swp->buf[0].xim->byte_order;
}

int
evas_xlib_swapper_bit_order_get(X_Swapper *swp)
{
   return swp->buf[0].xim->bitmap_bit_order;
}

#else

// DRM/DRI buffer swapping+access (driver specific) /////////////////////

static Eina_Bool tried = EINA_FALSE; /**< Flag to ensure DRM/DRI initialization is attempted only once. */
////////////////////////////////////
//libdrm.so.2
static void *drm_lib = NULL; /**< Handle for the dynamically loaded libdrm.so. */

typedef unsigned int drm_magic_t; /**< Type for DRM authentication magic number. */
static int (*sym_drmGetMagic)(int fd, drm_magic_t *magic) = NULL; /**< Function pointer for drmGetMagic. */

////////////////////////////////////
// libtbm.so.1
#define TBM_DEVICE_CPU   1 /**< TBM device type: CPU. */
#define TBM_OPTION_READ  (1 << 0) /**< TBM mapping option: Read access. */
#define TBM_OPTION_WRITE (1 << 1) /**< TBM mapping option: Write access. */
static void *tbm_lib = NULL; /**< Handle for the dynamically loaded libtbm.so or libdrm_slp.so. */

typedef struct _tbm_bufmgr *tbm_bufmgr; /**< Opaque TBM buffer manager type. */
typedef struct _tbm_bo     *tbm_bo; /**< Opaque TBM buffer object type. */

/**
 * @brief TBM buffer object handle.
 * @internal
 * Union to hold different representations of a TBM buffer object handle.
 */
typedef union _tbm_bo_handle
{
   void    *ptr; /**< Pointer handle. */
   int32_t  s32; /**< Signed 32-bit integer handle. */
   uint32_t u32; /**< Unsigned 32-bit integer handle. */
   int64_t  s64; /**< Signed 64-bit integer handle. */
   uint64_t u64; /**< Unsigned 64-bit integer handle. */
} tbm_bo_handle;

// Function pointers for TBM operations
static tbm_bo (*sym_tbm_bo_import)(tbm_bufmgr bufmgr, unsigned int key) = NULL;
static tbm_bo_handle (*sym_tbm_bo_map)(tbm_bo bo, int device, int opt) = NULL;
static int (*sym_tbm_bo_unmap)(tbm_bo bo) = NULL;
static void (*sym_tbm_bo_unref)(tbm_bo bo) = NULL;
static tbm_bufmgr (*sym_tbm_bufmgr_init)(int fd) = NULL;
static void (*sym_tbm_bufmgr_deinit)(tbm_bufmgr bufmgr) = NULL;

// Function pointers for legacy TBM (DRM SLP) compatibility
static void *(*sym_drm_slp_bo_map)(tbm_bo bo, int device, int opt) = NULL;
static int (*sym_drm_slp_bo_unmap)(tbm_bo bo, int device) = NULL;
static tbm_bufmgr (*sym_drm_slp_bufmgr_init)(int fd, void *arg) = NULL;

////////////////////////////////////
// libdri2.so.0
#define DRI2BufferBackLeft 1 /**< DRI2 attachment point: Back left buffer. */
static void *dri_lib = NULL; /**< Handle for the dynamically loaded libdri2.so. */

typedef unsigned long long CD64; /**< Type for 64-bit counter used in DRI2SwapBuffers. */

/**
 * @brief DRI2 buffer information structure.
 * @internal
 * Holds details about a DRI2 buffer, such as its name, pitch, and flags.
 */
typedef struct
{
   unsigned int attachment; /**< Attachment point (e.g., DRI2BufferBackLeft). */
   unsigned int name;       /**< Opaque name/handle of the buffer. */
   unsigned int pitch;      /**< Pitch (bytes per scanline) of the buffer. */
   unsigned int cpp;        /**< Bytes per pixel. */
   unsigned int flags;      /**< Flags associated with the buffer (see DRI2BufferFlags). */
} DRI2Buffer;

#define DRI2_BUFFER_TYPE_WINDOW 0x0 /**< DRI2 buffer type: Window. */
#define DRI2_BUFFER_TYPE_PIXMAP 0x1 /**< DRI2 buffer type: Pixmap. */
#define DRI2_BUFFER_TYPE_FB     0x2 /**< DRI2 buffer type: Framebuffer. */

/**
 * @brief DRI2 buffer flags.
 * @internal
 * Union and struct to interpret the flags field of a DRI2Buffer.
 * - type: Indicates if the buffer is for a window, pixmap, or framebuffer.
 * - is_framebuffer: True if the buffer is a framebuffer.
 * - is_mapped: True if the buffer is currently mapped.
 * - is_reused: True if the buffer is being reused.
 * - idx_reuse: Index indicating buffer reuse state (0:FULL, 1:COPY, 2:DOUBLE, 3:TRIPLE, 4:QUAD).
 */
typedef union
{
   unsigned int flags; /**< Raw flags value. */
   struct
   {
      unsigned int type : 1;           /**< Buffer type (window, pixmap, fb). */
      unsigned int is_framebuffer : 1; /**< Is it a framebuffer? */
      unsigned int is_mapped : 1;      /**< Is the buffer mapped? */
      unsigned int is_reused : 1;      /**< Is the buffer reused from a previous frame? */
      unsigned int idx_reuse : 3;      /**< Reuse index for swap state (0-4). */
   }
   data; /**< Access to bitfield data. */
} DRI2BufferFlags;

// Function pointers for DRI2 operations
static DRI2Buffer *(*sym_DRI2GetBuffers)(Display * display, XID drawable, int *width, int *height, unsigned int *attachments, int count, int *outCount) = NULL;
static Bool (*sym_DRI2QueryExtension)(Display *display, int *eventBase, int *errorBase) = NULL;
static Bool (*sym_DRI2QueryVersion)(Display *display, int *major, int *minor) = NULL;
static Bool (*sym_DRI2Connect)(Display *display, XID window, char **driverName, char **deviceName) = NULL;
static Bool (*sym_DRI2Authenticate)(Display *display, XID window, unsigned int magic) = NULL;
static void (*sym_DRI2CreateDrawable)(Display *display, XID drawable) = NULL;
static void (*sym_DRI2SwapBuffersWithRegion)(Display *display, XID drawable, XID region, CD64 *count) = NULL;
static void (*sym_DRI2SwapBuffers)(Display *display, XID drawable, CD64 target_msc, CD64 divisor, CD64 remainder, CD64 *count) = NULL;
static void (*sym_DRI2DestroyDrawable)(Display *display, XID handle) = NULL;

////////////////////////////////////
// libXfixes.so.3
static void *xfixes_lib = NULL; /**< Handle for the dynamically loaded libXfixes.so. */

// Function pointers for XFixes operations
static Bool (*sym_XFixesQueryExtension)(Display *display, int *event_base_return, int *error_base_return) = NULL;
static Status (*sym_XFixesQueryVersion)(Display *display, int *major_version_return, int *minor_version_return) = NULL;
static XID (*sym_XFixesCreateRegion)(Display *display, XRectangle *rectangles, int nrectangles) = NULL;
static void (*sym_XFixesDestroyRegion)(Display *dpy, XID region) = NULL;

////////////////////////////////////////////////////////////////////////////
#define MAX_BO_CACHE 4 /**< Maximum number of TBM buffer objects to cache. */

/**
 * @brief Cached buffer information for DRI2/TBM.
 * @internal
 * Stores the TBM buffer object and its name for caching purposes.
 */
typedef struct
{
   unsigned int name; /**< The DRI2 buffer name (identifier). */
   tbm_bo       buf_bo; /**< The corresponding TBM buffer object. */
} Buffer; /* This redefines Buffer, specific to the DRI2 implementation path. */

/**
 * @brief Main structure for the DRI2-based swapper.
 * @internal
 *
 * Contains X display, drawable, visual information, dimensions,
 * TBM buffer object, DRI2 buffer information, mapped buffer data,
 * a cache for TBM buffer objects, and state flags.
 */
struct _X_Swapper
{
   Display    *disp; /**< The X display connection. */
   Drawable    draw; /**< The X drawable. */
   Visual     *vis; /**< The X visual. */
   int         w, h, depth; /**< Original width, height, and depth. */
   tbm_bo      buf_bo; /**< Current TBM buffer object being used. */
   DRI2Buffer *buf; /**< Pointer to the current DRI2Buffer structure from DRI2GetBuffers. */
   void       *buf_data; /**< Pointer to the mapped pixel data of the current buffer. */
   int         buf_w, buf_h; /**< Actual width and height of the DRI2 buffer (may differ from w, h). */
   Eina_List  *buf_cache; /**< List of cached `Buffer` (name and tbm_bo pairs). */
   int         last_count; /**< Last seen reuse index from DRI2BufferFlags, for state tracking. */
   Eina_Bool   mapped : 1; /**< EINA_TRUE if a buffer is currently mapped. */
};

static int inits = 0; /**< Counter for DRM/DRI initializations. */
// XFixes extension information
static int xfixes_ev_base = 0, xfixes_err_base = 0;
static int xfixes_major = 0, xfixes_minor = 0;
// DRI2 extension information
static int dri2_ev_base = 0, dri2_err_base = 0;
static int dri2_major = 0, dri2_minor = 0;
static int drm_fd = -1; /**< File descriptor for the DRM device. */
static tbm_bufmgr bufmgr = NULL; /**< TBM buffer manager instance. */
static int swap_debug = -1; /**< Debug flag, initialized from EVAS_SWAPPER_DEBUG env var. */
static Eina_Bool slp_mode = EINA_FALSE; /**< EINA_TRUE if using legacy libdrm_slp.so.1. */

/**
 * @brief Initializes DRM, TBM, DRI2, and XFixes libraries and connections.
 * @internal
 * This function attempts to dynamically load necessary libraries (libdrm, libtbm,
 * libdri2, libXfixes), resolve symbols, query X extensions (DRI2, XFixes),
 * connect to the DRI2 driver, open the DRM device, authenticate, and initialize
 * the TBM buffer manager.
 *
 * @param disp The X display connection.
 * @param scr The screen number.
 * @return EINA_TRUE on successful initialization of all components, EINA_FALSE otherwise.
 */
static Eina_Bool
_drm_init(Display *disp, int scr)
{
   char *drv_name = NULL, *dev_name = NULL;
   drm_magic_t magic = 0;

   if (swap_debug == -1)
     {
        if (getenv("EVAS_SWAPPER_DEBUG")) swap_debug = 1;
        else swap_debug = 0;
     }

   if (xfixes_lib) return EINA_TRUE;
   if ((tried) && (!xfixes_lib)) return EINA_FALSE;
   tried = EINA_TRUE;
   drm_lib = dlopen("libdrm.so.2", RTLD_NOW | RTLD_LOCAL);
   if (!drm_lib)
     {
        if (swap_debug) ERR("Can't load libdrm.so.2");
        goto err;
     }
   slp_mode = EINA_FALSE;
   tbm_lib = dlopen("libtbm.so.1", RTLD_NOW | RTLD_LOCAL);
   if (!tbm_lib)
     {
        if (swap_debug) ERR("Can't load libtbm.so.1");
        tbm_lib = dlopen("libdrm_slp.so.1", RTLD_NOW | RTLD_LOCAL);
        if (tbm_lib) slp_mode = EINA_TRUE;
        else goto err;
     }
   dri_lib = dlopen("libdri2.so.0", RTLD_NOW | RTLD_LOCAL);
   if (!dri_lib)
     {
        if (swap_debug) ERR("Can't load libdri2.so.0");
        goto err;
     }
   xfixes_lib = dlopen("libXfixes.so.3", RTLD_NOW | RTLD_LOCAL);
   if (!xfixes_lib)
     {
        if (swap_debug) ERR("Can't load libXfixes.so.3");
        goto err;
     }

#define SYM(l, x)                                          \
  do { sym_ ## x = dlsym(l, #x);                           \
       if (!sym_ ## x) {                                   \
            if (swap_debug) ERR("Can't load symbol " #x);  \
            goto err;                                      \
         }                                                 \
    } while (0)

   SYM(drm_lib, drmGetMagic);

   if (!slp_mode)
     {
        SYM(tbm_lib, tbm_bo_import);
        SYM(tbm_lib, tbm_bo_map);
        SYM(tbm_lib, tbm_bo_unmap);
        SYM(tbm_lib, tbm_bo_unref);
        SYM(tbm_lib, tbm_bufmgr_init);
        SYM(tbm_lib, tbm_bufmgr_deinit);
     }
   else
     {
        // Looking up the legacy DRM SLP symbols. I don't believe this will
        // ever happen, this code is here "just in case".
        sym_tbm_bo_import = dlsym(tbm_lib, "drm_slp_bo_import");
        sym_drm_slp_bo_map = dlsym(tbm_lib, "drm_slp_bo_map");
        sym_drm_slp_bo_unmap = dlsym(tbm_lib, "drm_slp_bo_unmap");
        sym_tbm_bo_unref = dlsym(tbm_lib, "drm_slp_bo_unref");
        sym_drm_slp_bufmgr_init = dlsym(tbm_lib, "drm_slp_bufmgr_init");
        sym_tbm_bufmgr_deinit = dlsym(tbm_lib, "drm_slp_bufmgr_destroy");
        if (!sym_tbm_bo_import || !sym_drm_slp_bo_map || !sym_drm_slp_bo_unmap ||
            !sym_tbm_bo_unref || !sym_drm_slp_bufmgr_init || !sym_tbm_bufmgr_deinit)
          {
             ERR("Can't load symbols from libdrm_slp.so.1");
             goto err;
          }
     }

   SYM(dri_lib, DRI2GetBuffers);
   SYM(dri_lib, DRI2QueryExtension);
   SYM(dri_lib, DRI2QueryVersion);
   SYM(dri_lib, DRI2Connect);
   SYM(dri_lib, DRI2Authenticate);
   SYM(dri_lib, DRI2CreateDrawable);
   SYM(dri_lib, DRI2SwapBuffersWithRegion);
   SYM(dri_lib, DRI2SwapBuffers);
   SYM(dri_lib, DRI2DestroyDrawable);

   SYM(xfixes_lib, XFixesQueryExtension);
   SYM(xfixes_lib, XFixesQueryVersion);
   SYM(xfixes_lib, XFixesCreateRegion);
   SYM(xfixes_lib, XFixesDestroyRegion);

   if (!sym_XFixesQueryExtension(disp, &xfixes_ev_base, &xfixes_err_base))
     {
        if (swap_debug) ERR("XFixes extension not in xserver");
        goto err;
     }
   sym_XFixesQueryVersion(disp, &xfixes_major, &xfixes_minor);

   if (!sym_DRI2QueryExtension(disp, &dri2_ev_base, &dri2_err_base))
     {
        if (swap_debug) ERR("DRI2 extension not in xserver");
        goto err;
     }
   if (!sym_DRI2QueryVersion(disp, &dri2_major, &dri2_minor))
     {
        if (swap_debug) ERR("DRI2 query version failed");
        goto err;
     }
   if (dri2_minor < 99)
     {
        if (swap_debug)
          ERR("Not supported by DRI2 version(%i.%i)",
              dri2_major, dri2_minor);
        goto err;
     }
   if (!sym_DRI2Connect(disp, RootWindow(disp, scr), &drv_name, &dev_name))
     {
        if (swap_debug) ERR("DRI2 connect failed on screen %i", scr);
        goto err;
     }
   drm_fd = open(dev_name, O_RDWR);
   if (drm_fd < 0)
     {
        if (swap_debug) ERR("DRM FD open of '%s' failed", dev_name);
        goto err;
     }
   if (sym_drmGetMagic(drm_fd, &magic))
     {
        if (swap_debug) ERR("DRM get magic failed");
        goto err;
     }
   if (!sym_DRI2Authenticate(disp, RootWindow(disp, scr),
                             (unsigned int)magic))
     {
        if (swap_debug) ERR("DRI2 authenticate failed with magic 0x%x on screen %i", (unsigned int)magic, scr);
        goto err;
     }

   if (!slp_mode)
     bufmgr = sym_tbm_bufmgr_init(drm_fd);
   else
     bufmgr = sym_drm_slp_bufmgr_init(drm_fd, NULL);
   if (!bufmgr)
     {
        if (swap_debug) ERR("DRM bufmgr init failed");
        goto err;
     }
   if (drv_name) XFree(drv_name);
   XFree(dev_name);
   return EINA_TRUE;
err:
   if (drm_fd >= 0)
     {
        close(drm_fd);
        drm_fd = -1;
     }
   if (drm_lib)
     {
        dlclose(drm_lib);
        drm_lib = NULL;
     }
   if (tbm_lib)
     {
        dlclose(tbm_lib);
        tbm_lib = NULL;
     }
   if (dri_lib)
     {
        dlclose(dri_lib);
        dri_lib = NULL;
     }
   if (xfixes_lib)
     {
        dlclose(xfixes_lib);
        xfixes_lib = NULL;
     }
   if (drv_name) XFree(drv_name);
   if (dev_name) XFree(dev_name);
   return EINA_FALSE;
}

/**
 * @brief Shuts down DRM, TBM, DRI2 related resources.
 * @internal
 * Currently, this function is a no-op (returns immediately). The commented-out
 * code suggests it would deinitialize the TBM buffer manager, close the DRM
 * file descriptor, and unload the dynamic libraries. This is noted as potentially
 * expensive if re-initialization is frequent.
 */
static void
_drm_shutdown(void)
{
   return;
   // leave this here as notation on how to shut down stuff - never do it
   // though, as once shut down, we have to re-init and this could be
   // expensive especially if u have a single canvas that is changing config
   // and being shut down and re-initted a few times.
/*
   if (bufmgr)
     {
        sym_tbm_bufmgr_deinit(bufmgr);
        bufmgr = NULL;
     }
   if (drm_fd >= 0) close(drm_fd);
   drm_fd = -1;
   dlclose(tbm_lib);
   tbm_lib = NULL;
   dlclose(dri_lib);
   dri_lib = NULL;
   dlclose(xfixes_lib);
   xfixes_lib = NULL;
   tried = EINA_FALSE;
 */
}

/**
 * @brief Sets up DRI2 resources for a specific drawable.
 * @internal
 * Calls DRI2CreateDrawable to associate the X drawable with DRI2.
 *
 * @param swp The X_Swapper instance.
 * @return EINA_TRUE on success.
 */
static Eina_Bool
_drm_setup(X_Swapper *swp)
{
   sym_DRI2CreateDrawable(swp->disp, swp->draw);
   return EINA_TRUE;
}

/**
 * @brief Cleans up DRI2 resources for a specific drawable.
 * @internal
 * Calls DRI2DestroyDrawable to disassociate the X drawable from DRI2.
 *
 * @param swp The X_Swapper instance.
 */
static void
_drm_cleanup(X_Swapper *swp)
{
   sym_DRI2DestroyDrawable(swp->disp, swp->draw);
}

/**
 * @brief Creates a new Xlib swapper instance (DRI2 implementation).
 *
 * Initializes DRM/DRI2 if not already done. Allocates and initializes
 * the X_Swapper structure and sets up DRI2 for the given drawable.
 * Adjusts depth from 24 to 32 if necessary.
 *
 * @param disp The X display connection.
 * @param draw The X drawable.
 * @param vis The X visual.
 * @param depth The color depth.
 * @param w The width.
 * @param h The height.
 * @return A pointer to the new X_Swapper, or NULL on failure.
 */
X_Swapper *
evas_xlib_swapper_new(Display *disp, Drawable draw, Visual *vis,
                      int depth, int w, int h)
{
   X_Swapper *swp;

   if (inits <= 0)
     {
        if (!_drm_init(disp, 0)) return NULL;
     }
   inits++;

   swp = calloc(1, sizeof(X_Swapper));
   if (!swp) return NULL;
   swp->disp = disp;
   swp->draw = draw;
   swp->vis = vis;
   swp->depth = depth;
   swp->w = w;
   swp->h = h;
   swp->last_count = -1;
   if (!_drm_setup(swp))
     {
        inits--;
        if (inits == 0) _drm_shutdown();
        free(swp);
        return NULL;
     }
   if (swp->depth == 24) // need to adjust to 32bpp?? have to check
     {
        swp->depth = 32;
     }
   if (swap_debug) printf("Swapper allocated OK\n");
   return swp;
}

void
evas_xlib_swapper_free(X_Swapper *swp)
{
   Buffer *b;

   if (swap_debug) printf("Swapper free\n");
   if (swp->mapped) evas_xlib_swapper_buffer_unmap(swp);
   EINA_LIST_FREE(swp->buf_cache, b)
     {
        if (swap_debug) printf("Cached buf name %i freed\n", b->name);
        sym_tbm_bo_unref(b->buf_bo);
        free(b);
     }
   _drm_cleanup(swp);
   free(swp);
   inits--;
   if (inits == 0) _drm_shutdown();
}

/**
 * @brief Maps the DRI2 back buffer for drawing (DRI2 implementation).
 *
 * Retrieves the DRI2 back buffer using DRI2GetBuffers. If the buffer is new
 * or not in the cache, it imports it via TBM (tbm_bo_import) and adds it
 * to an LRU cache. The TBM buffer object is then mapped to CPU-accessible
 * memory (tbm_bo_map or drm_slp_bo_map).
 *
 * @param swp The X_Swapper instance.
 * @param[out] bpl Pointer to store bytes per line.
 * @param[out] w Pointer to store buffer width.
 * @param[out] h Pointer to store buffer height.
 * @return Pointer to the mapped buffer data, or NULL on failure.
 */
void *
evas_xlib_swapper_buffer_map(X_Swapper *swp, int *bpl, int *w, int *h)
{
   unsigned int attach = DRI2BufferBackLeft;
   int num;
   Eina_List *l;
   Buffer *b;
   DRI2BufferFlags *flags;

   if (swp->mapped)
     {
        if (bpl)
          {
             if ((swp->buf) && (swp->buf->pitch > 0)) *bpl = swp->buf->pitch;
             else *bpl = swp->w * 4;
          }
        if (w) *w = swp->w;
        if (h) *h = swp->h;
        return swp->buf_data;
     }
   swp->buf = sym_DRI2GetBuffers(swp->disp, swp->draw,
                                 &(swp->buf_w), &(swp->buf_h),
                                 &attach, 1, &num);
   if (!swp->buf) return NULL;
   if (!swp->buf->name) return NULL;
   flags = (DRI2BufferFlags *)(&(swp->buf->flags));
   if (!flags->data.is_reused)
     {
        if (swap_debug) printf("Buffer cache not reused - clear cache\n");
        // buffer isnt recycled - nuke the buf cache
        EINA_LIST_FREE(swp->buf_cache, b)
          {
             if (swap_debug) printf("Cached buf name %i freed\n", b->name);
             sym_tbm_bo_unref(b->buf_bo);
             free(b);
          }
     }
   else
     {
        // find a cache buf entry
        EINA_LIST_FOREACH(swp->buf_cache, l, b)
          {
             if (b->name == swp->buf->name)
               {
                  if (swap_debug) printf("Cached buf name %i found\n", b->name);
                  swp->buf_bo = b->buf_bo;
                  // LRU - least used at end. found item - promote to front
                  swp->buf_cache = eina_list_promote_list(swp->buf_cache, l);
                  break;
               }
          }
     }
   if (!swp->buf_bo)
     {
        swp->buf_bo = sym_tbm_bo_import(bufmgr, swp->buf->name);
        if (!swp->buf_bo) return NULL;
        // cache the buf entry
        b = calloc(1, sizeof(Buffer));
        b->name = swp->buf->name;
        b->buf_bo = swp->buf_bo;
        // put ah head of list
        swp->buf_cache = eina_list_prepend(swp->buf_cache, b);
        if (swap_debug) printf("Buffer cache added name %i\n", b->name);
        // keep bo cache no more than its max size
        while (eina_list_count(swp->buf_cache) > MAX_BO_CACHE)
          {
             if (swap_debug) printf("Buffer cache count %i more than max %i\n", eina_list_count(swp->buf_cache), MAX_BO_CACHE);
             l = eina_list_last(swp->buf_cache);
             if (l)
               {
                  b = l->data;
                  if (swap_debug) printf("Buffer cache overfull - free name %i\n", b->name);
                  swp->buf_cache = eina_list_remove_list(swp->buf_cache, l);
                  sym_tbm_bo_unref(b->buf_bo);
                  free(b);
               }
          }
     }

   if (!slp_mode)
     {
        tbm_bo_handle bo_handle;
        bo_handle = sym_tbm_bo_map(swp->buf_bo, TBM_DEVICE_CPU, TBM_OPTION_READ | TBM_OPTION_WRITE);
        /* If device is DEFAULT, 2D, 3D, MM,then swp->buf_data = bo_handle.u32 */
        swp->buf_data = bo_handle.ptr;
     }
   else
     {
        swp->buf_data = sym_drm_slp_bo_map(swp->buf_bo, TBM_DEVICE_CPU, TBM_OPTION_READ | TBM_OPTION_WRITE);
     }

   if (!swp->buf_data)
     {
        ERR("Buffer map name %i failed", swp->buf->name);
        return NULL;
     }
   if (bpl) *bpl = swp->buf->pitch;
   swp->mapped = EINA_TRUE;
   if (swap_debug) printf("Mapped bufer name %i OK\n", swp->buf->name);
   if ((swp->w != swp->buf_w) || (swp->h != swp->buf_h))
     {
        ERR("Evas software DRI swapper buffer size mismatch");
     }
   swp->w = swp->buf_w;
   swp->h = swp->buf_h;
   if (w) *w = swp->w;
   if (h) *h = swp->h;
   return swp->buf_data;
}

/**
 * @brief Unmaps the DRI2 buffer (DRI2 implementation).
 *
 * Unmaps the TBM buffer object from CPU memory (tbm_bo_unmap or
 * drm_slp_bo_unmap) and releases associated DRI2Buffer resources.
 *
 * @param swp The X_Swapper instance.
 */
void
evas_xlib_swapper_buffer_unmap(X_Swapper *swp)
{
   if (!swp->mapped) return;
   if (!slp_mode)
     sym_tbm_bo_unmap(swp->buf_bo);
   else
     sym_drm_slp_bo_unmap(swp->buf_bo, TBM_DEVICE_CPU);
   if (swap_debug) printf("Unmap buffer name %i\n", swp->buf->name);
   free(swp->buf);
   swp->buf = NULL;
   swp->buf_bo = NULL;
   swp->buf_data = NULL;
   swp->mapped = EINA_FALSE;
}

/**
 * @brief Swaps buffers using DRI2 (DRI2 implementation).
 *
 * Converts Eina_Rectangle array to XRectangle array. Creates an XFixes region
 * from these rectangles and then calls DRI2SwapBuffersWithRegion to perform
 * the swap. The XFixes region is then destroyed.
 *
 * @param swp The X_Swapper instance.
 * @param rects Array of Eina_Rectangle defining update regions.
 *              Example: Eina_Rectangle rects[] = {{0, 0, 100, 100}, {200, 200, 50, 50}};
 * @param nrects Number of rectangles in `rects`.
 */
void
evas_xlib_swapper_swap(X_Swapper *swp, Eina_Rectangle *rects, int nrects)
{
   XRectangle *xrects = alloca(nrects * sizeof(XRectangle));
   XID region;
   int i;
   unsigned long long sbc_count = 0;

   if (swap_debug) printf("Swap buffers\n");
   for (i = 0; i < nrects; i++)
     {
        xrects[i].x = rects[i].x; xrects[i].y = rects[i].y;
        xrects[i].width = rects[i].w; xrects[i].height = rects[i].h;
     }
   region = sym_XFixesCreateRegion(swp->disp, xrects, nrects);
   sym_DRI2SwapBuffersWithRegion(swp->disp, swp->draw, region, &sbc_count);
   sym_XFixesDestroyRegion(swp->disp, region);
}

/**
 * @brief Gets the buffer state for rendering optimization (DRI2 implementation).
 *
 * Ensures the buffer is mapped (if not already). Reads the `idx_reuse` flag
 * from the DRI2Buffer's flags. This index indicates the current swap state
 * (e.g., full, copy, double, triple, quadruple buffering).
 * If `idx_reuse` has changed since the last call, it forces MODE_FULL.
 *
 * @param swp The X_Swapper instance.
 * @return A Render_Output_Swap_Mode enum value.
 *         Example: MODE_DOUBLE, MODE_TRIPLE, MODE_FULL.
 */
Render_Output_Swap_Mode
evas_xlib_swapper_buffer_state_get(X_Swapper *swp)
{
   DRI2BufferFlags *flags;

   if (!swp->mapped) evas_xlib_swapper_buffer_map(swp, NULL, NULL, NULL);
   if (!swp->mapped) return MODE_FULL;
   flags = (DRI2BufferFlags *)(&(swp->buf->flags));
   if (flags->data.idx_reuse != swp->last_count)
     {
        swp->last_count = flags->data.idx_reuse;
        if (swap_debug) printf("Reuse changed - force FULL\n");
        return MODE_FULL;
     }
   if (swap_debug) printf("Swap state idx_reuse = %i (0=FULL, 1=COPY, 2=DOUBLE, 3=TRIPLE, 4=QUAD)\n", flags->data.idx_reuse);
   if (flags->data.idx_reuse == 0) return MODE_FULL;
   else if (flags->data.idx_reuse == 1)
     return MODE_COPY;
   else if (flags->data.idx_reuse == 2)
     return MODE_DOUBLE;
   else if (flags->data.idx_reuse == 3)
     return MODE_TRIPLE;
   else if (flags->data.idx_reuse == 4)
     return MODE_QUADRUPLE;
   return MODE_FULL;
}

/**
 * @brief Gets the depth of the swapper's buffers (DRI2 implementation).
 * @param swp The X_Swapper instance.
 * @return The color depth.
 */
int
evas_xlib_swapper_depth_get(X_Swapper *swp)
{
   return swp->depth;
}

/**
 * @brief Gets the byte order of the swapper's buffers (DRI2 implementation).
 * Assumes LSBFirst for DRI2 contexts.
 * @param swp The X_Swapper instance (unused).
 * @return LSBFirst.
 */
int
evas_xlib_swapper_byte_order_get(X_Swapper *swp EINA_UNUSED)
{
   return LSBFirst;
}

/**
 * @brief Gets the bit order of the swapper's buffers (DRI2 implementation).
 * Assumes LSBFirst for DRI2 contexts.
 * @param swp The X_Swapper instance (unused).
 * @return LSBFirst.
 */
int
evas_xlib_swapper_bit_order_get(X_Swapper *swp EINA_UNUSED)
{
   return LSBFirst;
}

#endif /* HAVE_DLSYM */

#else /* HAVE_DLSYM not defined: Stub implementations */

/**
 * @brief Stub for evas_xlib_swapper_new when dlsym is not available.
 * @return Always NULL.
 */
X_Swapper *
evas_xlib_swapper_new(Display *disp EINA_UNUSED, Drawable draw EINA_UNUSED,
                      Visual *vis EINA_UNUSED, int depth EINA_UNUSED,
                      int w EINA_UNUSED, int h EINA_UNUSED)
{
   return NULL;
}

/**
 * @brief Stub for evas_xlib_swapper_free when dlsym is not available.
 */
void
evas_xlib_swapper_free(X_Swapper *swp EINA_UNUSED)
{
}

/**
 * @brief Stub for evas_xlib_swapper_buffer_map when dlsym is not available.
 * @return Always NULL.
 */
void *
evas_xlib_swapper_buffer_map(X_Swapper *swp EINA_UNUSED, int *bpl EINA_UNUSED, int *w EINA_UNUSED, int *h EINA_UNUSED)
{
   return NULL;
}

/**
 * @brief Stub for evas_xlib_swapper_buffer_unmap when dlsym is not available.
 */
void
evas_xlib_swapper_buffer_unmap(X_Swapper *swp EINA_UNUSED)
{
}

/**
 * @brief Stub for evas_xlib_swapper_swap when dlsym is not available.
 */
void
evas_xlib_swapper_swap(X_Swapper *swp EINA_UNUSED, Eina_Rectangle *rects EINA_UNUSED, int nrects EINA_UNUSED)
{
}

/**
 * @brief Stub for evas_xlib_swapper_buffer_state_get when dlsym is not available.
 * @return Always MODE_FULL.
 */
Render_Output_Swap_Mode
evas_xlib_swapper_buffer_state_get(X_Swapper *swp EINA_UNUSED)
{
   return MODE_FULL;
}

/**
 * @brief Stub for evas_xlib_swapper_depth_get when dlsym is not available.
 * @return Always 0.
 */
int
evas_xlib_swapper_depth_get(X_Swapper *swp EINA_UNUSED)
{
   return 0;
}

/**
 * @brief Stub for evas_xlib_swapper_byte_order_get when dlsym is not available.
 * @return Always 0.
 */
int
evas_xlib_swapper_byte_order_get(X_Swapper *swp EINA_UNUSED)
{
   return 0;
}

/**
 * @brief Stub for evas_xlib_swapper_bit_order_get when dlsym is not available.
 * @return Always 0.
 */
int
evas_xlib_swapper_bit_order_get(X_Swapper *swp EINA_UNUSED)
{
   return 0;
}

#endif /* HAVE_DLSYM */
