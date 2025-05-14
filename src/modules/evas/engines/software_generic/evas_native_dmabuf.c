#include "evas_common_private.h"
#include "evas_private.h"
#include "evas_native_common.h"

#if defined HAVE_DLSYM
# include <dlfcn.h>      /* dlopen,dlclose,etc */
#elif _WIN32
# include <evil_private.h> /* dlopen dlclose dlsym mmap */
#else
# warning native_dmabuf should not get compiled if dlsym is not found on the system!
#endif

#ifndef _WIN32
# include <sys/mman.h>
#endif

#define DRM_FORMAT_ARGB8888           0x34325241
#define DRM_FORMAT_XRGB8888           0x34325258

/**
 * @brief Callback function to bind (map) the DMABUF memory for an Evas image.
 *
 * This function is called when Evas needs access to the pixel data of an
 * image associated with a DMABUF native surface. It maps the DMABUF file
 * descriptor into the process's address space if it hasn't been mapped already.
 * The mapped memory address is stored within the Native structure and assigned
 * to the image's data pointer.
 *
 * @param image Pointer to the RGBA_Image structure.
 * @param x Unused X coordinate.
 * @param y Unused Y coordinate.
 * @param w Unused width.
 * @param h Unused height.
 */
static void
_native_bind_cb(void *image, int x EINA_UNUSED, int y EINA_UNUSED, int w EINA_UNUSED, int h EINA_UNUSED)
{
   struct dmabuf_attributes *a;
   int size;
   RGBA_Image *im = image;

   if (!im) return;
   Native *n = im->native.data;
   if (!n) return;
   if (n->ns.type != EVAS_NATIVE_SURFACE_WL_DMABUF)
     return;

   if (im->image.data) return;

   a = (struct dmabuf_attributes *)&n->ns_data.wl_surface_dmabuf;
   if (n->ns_data.wl_surface_dmabuf.ptr)
     {
        im->image.data = n->ns_data.wl_surface_dmabuf.ptr;
        return;
     }
   size = a->height * a->stride[0];
   im->image.data = mmap(NULL, size, PROT_READ, MAP_SHARED, a->fd[0], 0);
   if (im->image.data == MAP_FAILED) im->image.data = NULL;
   n->ns_data.wl_surface_dmabuf.size = size;
   n->ns_data.wl_surface_dmabuf.ptr = im->image.data;
}

/**
 * @brief Callback function to unbind the DMABUF memory for an Evas image.
 *
 * This function is called when Evas is finished with a specific access
 * operation on the image data. In this implementation, it currently
 * does nothing, as the memory remains mapped until the image is freed.
 *
 * @param image Pointer to the RGBA_Image structure.
 */
static void
_native_unbind_cb(void *image)
{
   RGBA_Image *im = image;

   if (!im) return;
   Native *n = im->native.data;
   if (!n) return;
   if (n->ns.type != EVAS_NATIVE_SURFACE_WL_DMABUF)
     return;
}

/**
 * @brief Callback function to free resources associated with a DMABUF native surface.
 *
 * This function is called when the Evas image using the DMABUF native surface
 * is being destroyed. It unmaps the previously mapped DMABUF memory, frees the
 * associated Native data structure, and resets the native function pointers
 * on the image.
 *
 * @param image Pointer to the RGBA_Image structure being freed.
 */
static void
_native_free_cb(void *image)
{
   RGBA_Image *im = image;

   if (!im) return;
   Native *n = im->native.data;

   if (im->image.data)
     munmap(n->ns_data.wl_surface_dmabuf.ptr,
            n->ns_data.wl_surface_dmabuf.size);

   im->native.data        = NULL;
   im->native.func.bind   = NULL;
   im->native.func.unbind = NULL;
   im->native.func.free   = NULL;
   im->image.data         = NULL;

   free(n);
}

/**
 * @brief Sets or creates an Evas image based on a DMABUF native surface.
 *
 * This function associates an Evas RGBA_Image with a Wayland DMABUF native
 * surface.
 *
 * If `image` is NULL, it attempts to create a new Evas image matching the
 * properties specified in the `native` surface's attributes (width, height,
 * format). It validates that the DMABUF attributes are supported (single plane,
 * ARGB8888 or XRGB8888 format).
 *
 * If `image` is not NULL, it configures the existing image to use the provided
 * DMABUF native surface. It frees any existing native data, allocates a new
 * Native structure, copies the native surface information, validates the
 * attributes, sets up the image properties (width, height, colorspace, alpha),
 * and assigns the native bind/unbind/free callbacks. The actual memory mapping
 * is deferred until the `_native_bind_cb` is called.
 *
 * @param image Pointer to an existing RGBA_Image, or NULL to create a new one.
 * @param native Pointer to an Evas_Native_Surface structure of type
 *               EVAS_NATIVE_SURFACE_WL_DMABUF containing the DMABUF details.
 *               The `native->data.wl_dmabuf.attr` should point to a valid
 *               `struct dmabuf_attributes`.
 *               Example `dmabuf_attributes` structure:
 *               ```c
 *               struct dmabuf_attributes {
 *                   int32_t version; // Must be EVAS_DMABUF_ATTRIBUTE_VERSION
 *                   int32_t width;   // Width in pixels
 *                   int32_t height;  // Height in pixels
 *                   uint32_t format; // Pixel format (e.g., DRM_FORMAT_ARGB8888)
 *                   int32_t n_planes;// Number of planes (must be 1)
 *                   int32_t stride[4]; // Stride for each plane (only stride[0] used)
 *                   int32_t offset[4]; // Offset for each plane (unused here)
 *                   int fd[4];       // File descriptor for each plane (only fd[0] used)
 *                   uint64_t modifier[4]; // Modifier for each plane (unused here)
 *               };
 *               ```
 * @return Pointer to the configured or newly created RGBA_Image on success,
 *         or NULL on failure (e.g., invalid parameters, unsupported format,
 *         allocation failure).
 */
void *
_evas_native_dmabuf_surface_image_set(void *image, void *native)
{
   Evas_Native_Surface *ns = native;
   RGBA_Image *im = image;

   if (!im)
     {
        if (ns && ns->type == EVAS_NATIVE_SURFACE_WL_DMABUF &&
            !ns->data.wl_dmabuf.resource)
          {
             struct dmabuf_attributes *attr;

             attr = ns->data.wl_dmabuf.attr;
             if (attr->version != EVAS_DMABUF_ATTRIBUTE_VERSION)
               return NULL;
             if (attr->n_planes != 1)
               return NULL;
             if (attr->format != DRM_FORMAT_ARGB8888 &&
                 attr->format != DRM_FORMAT_XRGB8888)
               return NULL;

             return evas_cache_image_data(evas_common_image_cache_get(),
                                          attr->width, attr->height,
                                          NULL, 1,
                                          EVAS_COLORSPACE_ARGB8888);
          }
       return NULL;
     }

   if (ns)
     {
        struct dmabuf_attributes *a;
        int h, stride;
        int32_t format;
        Native *n;

        if (ns->type != EVAS_NATIVE_SURFACE_WL_DMABUF)
          return NULL;

        n = im->native.data;
        if (n)
           {
              if (n->ns_data.wl_surface_dmabuf.ptr)
                {
                   munmap(n->ns_data.wl_surface_dmabuf.ptr,
                          n->ns_data.wl_surface_dmabuf.size);
                   n->ns_data.wl_surface_dmabuf.size = 0;
                   n->ns_data.wl_surface_dmabuf.ptr = NULL;
                }
              free(im->native.data);
           }
        n = calloc(1, sizeof(Native));
        if (!n) return NULL;

        a = ns->data.wl_dmabuf.attr;
        if (a->version != EVAS_DMABUF_ATTRIBUTE_VERSION)
          {
             free(n);
             return NULL;
          }

        h = a->height;
        stride = a->stride[0];
        format = a->format;
        im->cache_entry.w = stride;
        im->cache_entry.h = h;

        /* This block assumes single planar formats, which are all we
         * currently support. */
        im->cache_entry.w = stride / 4;
        evas_cache_image_colorspace(&im->cache_entry, EVAS_COLORSPACE_ARGB8888);
        im->cache_entry.flags.alpha = (format == DRM_FORMAT_XRGB8888 ? 0 : 1);
        im->image.data = NULL;;
        im->image.no_free = 1;

        memcpy(n, ns, sizeof(Evas_Native_Surface));
        memcpy(&n->ns_data.wl_surface_dmabuf.attr, a, sizeof(*a));
        im->native.data = n;
        im->native.func.bind   = _native_bind_cb;
        im->native.func.unbind = _native_unbind_cb;
        im->native.func.free   = _native_free_cb;
     }

   return im;
}
