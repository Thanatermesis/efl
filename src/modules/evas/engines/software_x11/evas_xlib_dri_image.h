#include "evas_engine.h"

/** @brief Opaque handle to a TBM buffer manager. */
typedef struct _tbm_bufmgr *tbm_bufmgr;
/** @brief Opaque handle to a TBM buffer object. */
typedef struct _tbm_bo *tbm_bo;

/**
 * @brief Union representing a handle to a TBM buffer object.
 * Can be accessed as different types (pointer, integers).
 */
typedef union _tbm_bo_handle
{
   void     *ptr;
   int32_t  s32;
   uint32_t u32;
   int64_t  s64;
   uint64_t u64;   /**< Unsigned 64-bit integer access */
} tbm_bo_handle;

/**
 * @brief Structure representing a DRI2 buffer.
 */
typedef struct
{
   unsigned int attachment;
   unsigned int name;
   unsigned int pitch;
   unsigned int cpp;        /**< Bytes per pixel */
   unsigned int flags;      /**< DRI2BufferFlags packed into an integer */
} DRI2Buffer;

#define DRI2_BUFFER_TYPE_WINDOW 0x0
#define DRI2_BUFFER_TYPE_PIXMAP 0x1
#define DRI2_BUFFER_TYPE_FB     0x2

/**
 * @brief Union representing flags for a DRI2 buffer.
 * Provides bitfield access to individual flags.
 */
typedef union
{
   unsigned int flags; /**< Raw flags value */
   struct
   {
      unsigned int type:1;
      unsigned int is_framebuffer:1;
      unsigned int is_mapped:1;
      unsigned int is_reused:1;
      unsigned int idx_reuse:3;    /**< Index for buffer reuse tracking */
   }
   data; /**< Bitfield access structure */
} DRI2BufferFlags;

/**
 * @brief Internal structure to cache TBM buffer object information.
 */
typedef struct
{
   unsigned int name; /**< DRI name of the buffer */
   tbm_bo   buf_bo;   /**< TBM buffer object handle */
} Buffer;

/** @brief Opaque structure holding DRI image information. */
typedef struct _Evas_DRI_Image Evas_DRI_Image;
/** @brief Opaque structure holding DRI native surface information. */
typedef struct _DRI_Native DRI_Native;

/**
 * @brief Main structure containing DRI-specific image data for X11.
 */
struct _Evas_DRI_Image
{
   Display         *dis;        /**< X Display connection */
   Visual          *visual;     /**< X Visual associated with the drawable */
   int              depth;      /**< Color depth of the drawable */
   int              w, h;       /**< Original requested width and height */
   int              bpl, bpp, rows; /**< Bytes per line, bits per pixel, number of rows (often unused here) */
   unsigned char   *data;      /**< Pointer to image data (often unused directly) */
   Drawable        draw;       /**< X Drawable (Pixmap or Window) */
   tbm_bo          buf_bo;     /**< Current TBM buffer object handle */
   DRI2Buffer      *buf;       /**< Pointer to the current DRI2 buffer information */
   void            *buf_data;  /**< Pointer to the mapped buffer data */
   int             buf_w, buf_h; /**< Actual width and height of the DRI2 buffer */
   Buffer          *buf_cache; /**< Cached buffer information for potential reuse */
};

/**
 * @brief Structure linking Evas native surface information with DRI details.
 */
struct _DRI_Native
{
   Evas_Native_Surface ns;
   Pixmap              pixmap;
   Visual             *visual;
   Display            *d;     /**< X Display connection */

   Evas_DRI_Image       *exim; /**< Pointer to the associated DRI image data */
};

/**
 * @brief Creates a new Evas_DRI_Image structure.
 * @param w Width of the image.
 * @param h Height of the image.
 * @param vis X Visual.
 * @param depth Color depth.
 * @return A newly allocated Evas_DRI_Image structure, or NULL on failure.
 */
Evas_DRI_Image *evas_xlib_image_dri_new(int w, int h, Visual *vis, int depth);

/**
 * @brief Frees resources associated with an Evas_DRI_Image.
 * @param exim The Evas_DRI_Image structure to free.
 */
void evas_xlib_image_dir_free(Evas_DRI_Image *exim);
/**
 * @brief Retrieves DRI2 buffers for a given RGBA_Image.
 * Maps the buffer and updates the image data pointer.
 * @param im The RGBA_Image to get buffers for.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
Eina_Bool evas_xlib_image_get_buffers(RGBA_Image *im);
/**
 * @brief Unmaps the currently mapped DRI2 buffer.
 * @param exim The Evas_DRI_Image structure containing the buffer to unmap.
 */
void evas_xlib_image_buffer_unmap(Evas_DRI_Image *exim);
/**
 * @brief Initializes the DRI subsystem for a specific Evas_DRI_Image.
 * Increments the global initialization counter.
 * @param exim The Evas_DRI_Image to initialize.
 * @param display The X Display connection.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
Eina_Bool evas_xlib_image_dri_init(Evas_DRI_Image *exim, Display *display);
/**
 * @brief Checks if the DRI subsystem is currently initialized and in use.
 * @return EINA_TRUE if DRI is in use (init count > 0), EINA_FALSE otherwise.
 */
Eina_Bool evas_xlib_image_dri_used(void);
/**
 * @brief Sets up an RGBA_Image to use a native X11 Pixmap via DRI.
 * Associates the image with the provided native surface information.
 * @param data Engine-specific data (Outbuf).
 * @param image The RGBA_Image to configure.
 * @param native Pointer to an Evas_Native_Surface structure (X11 type).
 * @return The configured image pointer on success, NULL on failure.
 */
void *evas_xlib_image_dri_native_set(void *data, void *image, void *native);
