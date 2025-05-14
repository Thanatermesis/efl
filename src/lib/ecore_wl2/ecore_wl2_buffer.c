#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "ecore_wl2_private.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <drm_fourcc.h>
#include <intel_bufmgr.h>
#include <i915_drm.h>
#include <vc4_drm.h>
#include <exynos_drm.h>
#include <exynos_drmif.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#if defined(__linux__)
#include <linux/dma-buf.h>
#elif defined(__FreeBSD__)
/* begin/end dma-buf functions used for userspace mmap. */
struct dma_buf_sync {
        __u64 flags;
};

#define DMA_BUF_SYNC_READ      (1 << 0)
#define DMA_BUF_SYNC_WRITE     (2 << 0)
#define DMA_BUF_SYNC_RW        (DMA_BUF_SYNC_READ | DMA_BUF_SYNC_WRITE)
#define DMA_BUF_SYNC_START     (0 << 2)
#define DMA_BUF_SYNC_END       (1 << 2)
#define DMA_BUF_SYNC_VALID_FLAGS_MASK \
        (DMA_BUF_SYNC_RW | DMA_BUF_SYNC_END)

#define DMA_BUF_BASE            'b'
#define DMA_BUF_IOCTL_SYNC      _IOW(DMA_BUF_BASE, 0, struct dma_buf_sync)
#endif

#include "linux-dmabuf-unstable-v1-client-protocol.h"

#define SYM(lib, xx)               \
   do {                            \
      sym_## xx = dlsym(lib, #xx); \
      if (!(sym_ ## xx)) {         \
         fail = EINA_TRUE;         \
      }                            \
   } while (0)

static int drm_fd = -1;

typedef struct _Ecore_Wl2_Buffer Ecore_Wl2_Buffer;
typedef struct _Buffer_Handle Buffer_Handle;

/**
 * @brief Manages different buffer allocation strategies (intel, exynos, vc4, shm).
 *
 * This struct holds function pointers to implement a specific buffer
 * allocation and management backend. It allows for a unified interface
 * to different hardware or software rendering buffer types.
 */
typedef struct _Buffer_Manager Buffer_Manager;
struct _Buffer_Manager
{
   Buffer_Handle *(*alloc)(Buffer_Manager *self, const char *name, int w, int h, unsigned long *stride, int32_t *fd); /**< Allocates a new buffer handle. */
   struct wl_buffer *(*to_buffer)(Ecore_Wl2_Display *ewd, Ecore_Wl2_Buffer *db); /**< Converts an Ecore_Wl2_Buffer to a Wayland wl_buffer. */
   void *(*map)(Ecore_Wl2_Buffer *buf); /**< Maps the buffer into memory. */
   void (*unmap)(Ecore_Wl2_Buffer *buf); /**< Unmaps the buffer from memory. */
   void (*discard)(Ecore_Wl2_Buffer *buf); /**< Discards/deallocates the buffer handle. */
   void (*lock)(Ecore_Wl2_Buffer *buf); /**< Locks the buffer for access (e.g., DMA_BUF_IOCTL_SYNC START). */
   void (*unlock)(Ecore_Wl2_Buffer *buf); /**< Unlocks the buffer after access (e.g., DMA_BUF_IOCTL_SYNC END). */
   void (*manager_destroy)(void); /**< Destroys the buffer manager private data. */
   void *priv; /**< Private data for the buffer manager implementation. */
   void *dl_handle; /**< Handle for the dynamically loaded library (e.g., libdrm_intel.so). */
   int refcount;
   Eina_Bool destroyed;
};

static Buffer_Manager *buffer_manager = NULL;

static drm_intel_bufmgr *(*sym_drm_intel_bufmgr_gem_init)(int fd, int batch_size) = NULL;
static int (*sym_drm_intel_bo_unmap)(drm_intel_bo *bo) = NULL;
static int (*sym_drm_intel_bo_map)(drm_intel_bo *bo) = NULL;
static drm_intel_bo *(*sym_drm_intel_bo_alloc_tiled)(drm_intel_bufmgr *mgr, const char *name, int x, int y, int cpp, uint32_t *tile, unsigned long *pitch, unsigned long flags) = NULL;
static void (*sym_drm_intel_bo_unreference)(drm_intel_bo *bo) = NULL;
static int (*sym_drmPrimeHandleToFD)(int fd, uint32_t handle, uint32_t flags, int *prime_fd) = NULL;
static void (*sym_drm_intel_bufmgr_destroy)(drm_intel_bufmgr *) = NULL;

static struct exynos_device *(*sym_exynos_device_create)(int fd) = NULL;
static struct exynos_bo *(*sym_exynos_bo_create)(struct exynos_device *dev, size_t size, uint32_t flags) = NULL;
static void *(*sym_exynos_bo_map)(struct exynos_bo *bo) = NULL;
static void (*sym_exynos_bo_destroy)(struct exynos_bo *bo) = NULL;
static void (*sym_exynos_device_destroy)(struct exynos_device *) = NULL;

/**
 * @brief Callback for when the compositor releases a wl_buffer.
 *
 * This function is called by the Wayland compositor when it no longer
 * needs the buffer. It marks the Ecore_Wl2_Buffer as not busy.
 * If the buffer was orphaned (marked for destruction while busy),
 * it proceeds with destroying the buffer.
 *
 * @param data The Ecore_Wl2_Buffer associated with the wl_buffer.
 * @param buffer The wl_buffer being released (unused in function body).
 */
static void
buffer_release(void *data, struct wl_buffer *buffer EINA_UNUSED)
{
   Ecore_Wl2_Buffer *b = data;

   b->busy = EINA_FALSE;
   if (b->orphaned) ecore_wl2_buffer_destroy(b);
}

static const struct wl_buffer_listener buffer_listener =
{
   buffer_release
};

/**
 * @brief Creates a wl_buffer from a DMA-BUF based Ecore_Wl2_Buffer.
 *
 * This function uses the zwp_linux_dmabuf_v1 protocol to create
 * a Wayland buffer from a DMA buffer file descriptor.
 *
 * @param ewd The Ecore_Wl2_Display.
 * @param db The Ecore_Wl2_Buffer containing DMA-BUF information.
 * @return A new wl_buffer, or NULL on failure.
 */
static struct wl_buffer *
_evas_dmabuf_wl_buffer_from_dmabuf(Ecore_Wl2_Display *ewd, Ecore_Wl2_Buffer *db)
{
   struct wl_buffer *buf;
   struct zwp_linux_dmabuf_v1 *dmabuf;
   struct zwp_linux_buffer_params_v1 *dp;
   uint32_t flags = 0;
   uint32_t format;

   if (db->alpha)
     format = DRM_FORMAT_ARGB8888;
   else
     format = DRM_FORMAT_XRGB8888;

   dmabuf = ecore_wl2_display_dmabuf_get(ewd);
   dp = zwp_linux_dmabuf_v1_create_params(dmabuf);
   zwp_linux_buffer_params_v1_add(dp, db->fd, 0, 0, db->stride, 0, 0);
   buf = zwp_linux_buffer_params_v1_create_immed(dp, db->w, db->h,
                                                 format, flags);
   wl_buffer_add_listener(buf, &buffer_listener, db);
   zwp_linux_buffer_params_v1_destroy(dp);

   return buf;
}

/**
 * @brief Locks a DMA-BUF buffer for CPU access (SYNC_START).
 *
 * This function performs a DMA_BUF_IOCTL_SYNC with DMA_BUF_SYNC_START
 * to ensure that CPU access to the buffer is synchronized.
 *
 * @param b The Ecore_Wl2_Buffer to lock.
 */
static void
_dmabuf_lock(Ecore_Wl2_Buffer *b)
{
   int ret;
   struct dma_buf_sync s;

   s.flags = DMA_BUF_SYNC_START | DMA_BUF_SYNC_RW;
   do
     {
        ret = ioctl(b->fd, DMA_BUF_IOCTL_SYNC, &s);
     } while (ret && ((errno == EAGAIN) || (errno == EINTR)));

   if (ret) WRN("Failed to lock dmabuf");
}

/**
 * @brief Unlocks a DMA-BUF buffer after CPU access (SYNC_END).
 *
 * This function performs a DMA_BUF_IOCTL_SYNC with DMA_BUF_SYNC_END
 * to signal that CPU access to the buffer is finished.
 *
 * @param b The Ecore_Wl2_Buffer to unlock.
 */
static void
_dmabuf_unlock(Ecore_Wl2_Buffer *b)
{
   int ret;
   struct dma_buf_sync s;

   s.flags = DMA_BUF_SYNC_END | DMA_BUF_SYNC_RW;
   do
     {
        ret = ioctl(b->fd, DMA_BUF_IOCTL_SYNC, &s);
     } while (ret && ((errno == EAGAIN) || (errno == EINTR)));

   if (ret) WRN("Failed to unlock dmabuf");
}

/**
 * @brief Allocates a buffer using Intel's GEM buffer manager.
 *
 * @param self The buffer manager instance.
 * @param name Name for the buffer (for debugging).
 * @param w Width of the buffer.
 * @param h Height of the buffer.
 * @param stride Pointer to store the calculated stride of the buffer.
 * @param fd Pointer to store the file descriptor of the DMA-BUF.
 * @return A Buffer_Handle (drm_intel_bo *) on success, NULL on failure.
 */
static Buffer_Handle *
_intel_alloc(Buffer_Manager *self, const char *name, int w, int h, unsigned long *stride, int32_t *fd)
{
   uint32_t tile = I915_TILING_NONE;
   drm_intel_bo *out;

   out = sym_drm_intel_bo_alloc_tiled(self->priv, name, w, h, 4, &tile,
                                       stride, 0);

   if (!out) return NULL;

   if (tile != I915_TILING_NONE) goto err;
   /* First try to allocate an mmapable buffer with O_RDWR,
    * if that fails retry unmappable - if the compositor is
    * using GL it won't need to mmap the buffer and this can
    * work - otherwise it'll reject this buffer and we'll
    * have to fall back to shm rendering.
    */
   if (sym_drmPrimeHandleToFD(drm_fd, out->handle,
                              DRM_CLOEXEC | O_RDWR, fd) != 0)
     if (sym_drmPrimeHandleToFD(drm_fd, out->handle,
                                DRM_CLOEXEC, fd) != 0) goto err;

   return (Buffer_Handle *)out;

err:
   sym_drm_intel_bo_unreference(out);
   return NULL;
}

/**
 * @brief Maps an Intel GEM buffer for CPU access.
 *
 * @param buf The Ecore_Wl2_Buffer to map.
 * @return A pointer to the mapped memory, or NULL on failure.
 */
static void *
_intel_map(Ecore_Wl2_Buffer *buf)
{
   drm_intel_bo *bo;

   bo = (drm_intel_bo *)buf->bh;
   if (sym_drm_intel_bo_map(bo) != 0) return NULL;
   return bo->virtual;
}

/**
 * @brief Unmaps an Intel GEM buffer.
 *
 * @param buf The Ecore_Wl2_Buffer to unmap.
 */
static void
_intel_unmap(Ecore_Wl2_Buffer *buf)
{
   drm_intel_bo *bo;

   bo = (drm_intel_bo *)buf->bh;
   sym_drm_intel_bo_unmap(bo);
}

/**
 * @brief Discards/unreferences an Intel GEM buffer.
 *
 * @param buf The Ecore_Wl2_Buffer whose underlying drm_intel_bo to unreference.
 */
static void
_intel_discard(Ecore_Wl2_Buffer *buf)
{
   drm_intel_bo *bo;

   bo = (drm_intel_bo *)buf->bh;
   sym_drm_intel_bo_unreference(bo);
}

/**
 * @brief Destroys the Intel buffer manager instance.
 */
static void
_intel_manager_destroy()
{
   sym_drm_intel_bufmgr_destroy(buffer_manager->priv);
}

/**
 * @brief Sets up the Intel buffer manager.
 *
 * Loads libdrm_intel.so and initializes the buffer manager function pointers
 * for Intel GEM-based buffer allocation.
 *
 * @param fd The DRM device file descriptor.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
_intel_buffer_manager_setup(int fd)
{
   Eina_Bool fail = EINA_FALSE;
   void *drm_intel_lib;

   drm_intel_lib = dlopen("libdrm_intel.so", RTLD_LAZY | RTLD_GLOBAL);
   if (!drm_intel_lib) return EINA_FALSE;

   SYM(drm_intel_lib, drm_intel_bufmgr_gem_init);
   SYM(drm_intel_lib, drm_intel_bo_unmap);
   SYM(drm_intel_lib, drm_intel_bo_map);
   SYM(drm_intel_lib, drm_intel_bo_alloc_tiled);
   SYM(drm_intel_lib, drm_intel_bo_unreference);
   SYM(drm_intel_lib, drm_intel_bufmgr_destroy);
   SYM(drm_intel_lib, drmPrimeHandleToFD);

   if (fail) goto err;

   buffer_manager->priv = sym_drm_intel_bufmgr_gem_init(fd, 32);
   if (!buffer_manager->priv) goto err;

   buffer_manager->alloc = _intel_alloc;
   buffer_manager->to_buffer = _evas_dmabuf_wl_buffer_from_dmabuf;
   buffer_manager->map = _intel_map;
   buffer_manager->unmap = _intel_unmap;
   buffer_manager->discard = _intel_discard;
   buffer_manager->lock = _dmabuf_lock;
   buffer_manager->unlock = _dmabuf_unlock;
   buffer_manager->manager_destroy = _intel_manager_destroy;
   buffer_manager->dl_handle = drm_intel_lib;

   return EINA_TRUE;

err:
   dlclose(drm_intel_lib);
   return EINA_FALSE;
}

/**
 * @brief Allocates a buffer using Exynos DRM.
 *
 * @param self The buffer manager instance.
 * @param name Unused.
 * @param w Width of the buffer.
 * @param h Height of the buffer.
 * @param stride Pointer to store the calculated stride of the buffer.
 * @param fd Pointer to store the file descriptor of the DMA-BUF.
 * @return A Buffer_Handle (struct exynos_bo *) on success, NULL on failure.
 */
static Buffer_Handle *
_exynos_alloc(Buffer_Manager *self, const char *name EINA_UNUSED, int w, int h, unsigned long *stride, int32_t *fd)
{
   size_t size = w * h * 4;
   struct exynos_bo *out;

   *stride = w * 4;
   out = sym_exynos_bo_create(self->priv, size, 0);
   if (!out) return NULL;
   /* First try to allocate an mmapable buffer with O_RDWR,
    * if that fails retry unmappable - if the compositor is
    * using GL it won't need to mmap the buffer and this can
    * work - otherwise it'll reject this buffer and we'll
    * have to fall back to shm rendering.
    */
   if (sym_drmPrimeHandleToFD(drm_fd, out->handle,
                              DRM_CLOEXEC | O_RDWR, fd) != 0)
     if (sym_drmPrimeHandleToFD(drm_fd, out->handle,
                                DRM_CLOEXEC, fd) != 0) goto err;

   return (Buffer_Handle *)out;

err:
   sym_exynos_bo_destroy(out);
   return NULL;
}

/**
 * @brief Maps an Exynos buffer for CPU access using mmap on its fd.
 *
 * @param buf The Ecore_Wl2_Buffer to map.
 * @return A pointer to the mapped memory, or NULL on failure.
 */
static void *
_exynos_map(Ecore_Wl2_Buffer *buf)
{
   struct exynos_bo *bo;
   void *ptr;

   bo = (struct exynos_bo *)buf->bh;
   ptr = mmap(0, bo->size, PROT_READ | PROT_WRITE, MAP_SHARED, buf->fd, 0);
   if (ptr == MAP_FAILED) return NULL;
   return ptr;
}

/**
 * @brief Unmaps an Exynos buffer.
 *
 * @param buf The Ecore_Wl2_Buffer to unmap.
 */
static void
_exynos_unmap(Ecore_Wl2_Buffer *buf)
{
   struct exynos_bo *bo;

   bo = (struct exynos_bo *)buf->bh;
   munmap(buf->mapping, bo->size);
}

/**
 * @brief Discards/destroys an Exynos buffer object.
 *
 * @param buf The Ecore_Wl2_Buffer whose underlying exynos_bo to destroy.
 */
static void
_exynos_discard(Ecore_Wl2_Buffer *buf)
{
   struct exynos_bo *bo;

   bo = (struct exynos_bo *)buf->bh;
   sym_exynos_bo_destroy(bo);
}

/**
 * @brief Destroys the Exynos buffer manager (Exynos device).
 */
static void
_exynos_manager_destroy()
{
   sym_exynos_device_destroy(buffer_manager->priv);
}

/**
 * @brief Sets up the Exynos buffer manager.
 *
 * Loads libdrm_exynos.so and initializes the buffer manager function pointers
 * for Exynos DRM-based buffer allocation.
 *
 * @param fd The DRM device file descriptor.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
_exynos_buffer_manager_setup(int fd)
{
   Eina_Bool fail = EINA_FALSE;
   void *drm_exynos_lib;
   struct exynos_bo *bo;

   drm_exynos_lib = dlopen("libdrm_exynos.so", RTLD_LAZY | RTLD_GLOBAL);
   if (!drm_exynos_lib) return EINA_FALSE;

   SYM(drm_exynos_lib, exynos_device_create);
   SYM(drm_exynos_lib, exynos_bo_create);
   SYM(drm_exynos_lib, exynos_bo_map);
   SYM(drm_exynos_lib, exynos_bo_destroy);
   SYM(drm_exynos_lib, exynos_device_destroy);
   SYM(drm_exynos_lib, drmPrimeHandleToFD);

   if (fail) goto err;

   buffer_manager->priv = sym_exynos_device_create(fd);
   if (!buffer_manager->priv) goto err;

   /* _device_create succeeds on any arch, test harder */
   bo = sym_exynos_bo_create(buffer_manager->priv, 32, 0);
   if (!bo) goto err;

   sym_exynos_bo_destroy(bo);

   buffer_manager->alloc = _exynos_alloc;
   buffer_manager->to_buffer = _evas_dmabuf_wl_buffer_from_dmabuf;
   buffer_manager->map = _exynos_map;
   buffer_manager->unmap = _exynos_unmap;
   buffer_manager->discard = _exynos_discard;
   buffer_manager->lock = _dmabuf_lock;
   buffer_manager->unlock = _dmabuf_unlock;
   buffer_manager->manager_destroy = _exynos_manager_destroy;
   buffer_manager->dl_handle = drm_exynos_lib;
   return EINA_TRUE;

err:
   dlclose(drm_exynos_lib);
   return EINA_FALSE;
}

/**
 * @brief Allocates a shared memory (SHM) buffer.
 *
 * Creates a temporary file, truncates it to the required size, and maps it.
 * This is used for wl_shm buffers.
 *
 * @param self Unused.
 * @param name Unused.
 * @param w Width of the buffer.
 * @param h Height of the buffer.
 * @param stride Pointer to store the calculated stride of the buffer.
 * @param fd Pointer to store the file descriptor of the SHM segment.
 * @return A Buffer_Handle (pointer to mapped memory) on success, NULL on failure.
 */
static Buffer_Handle *
_wl_shm_alloc(Buffer_Manager *self EINA_UNUSED, const char *name EINA_UNUSED, int w, int h, unsigned long *stride, int32_t *fd)
{
   Eina_Tmpstr *fullname;
   size_t size = w * h * 4;
   void *out = NULL;
   char *tmp;

   // XXX try memfd, then shm open then the below...
   tmp = eina_vpath_resolve("(:usr.run:)/evas-wayland_shm-XXXXXX");
   *fd = eina_file_mkstemp(tmp, &fullname);
   free(tmp);

   if (*fd < 0) return NULL;

   unlink(fullname);
   eina_tmpstr_del(fullname);

   *stride = w * 4;
   if (ftruncate(*fd, size) < 0) goto err;

   out = mmap(NULL, size, (PROT_READ | PROT_WRITE), MAP_SHARED, *fd, 0);
   if (out == MAP_FAILED) goto err;

   return out;

err:
   close(*fd);
   return NULL;
}

/**
 * @brief "Maps" a wl_shm buffer.
 *
 * For wl_shm, the buffer handle (bh) is already the mapped address.
 *
 * @param buf The Ecore_Wl2_Buffer.
 * @return The pointer to the mapped memory (buf->bh).
 */
static void *
_wl_shm_map(Ecore_Wl2_Buffer *buf)
{
   return buf->bh;
}

/**
 * @brief "Unmaps" a wl_shm buffer.
 *
 * This is a no-op as wl_shm buffers are typically mapped for their lifetime.
 *
 * @param buf Unused.
 */
static void
_wl_shm_unmap(Ecore_Wl2_Buffer *buf EINA_UNUSED)
{
   /* wl_shm is mapped for its lifetime */
}

/**
 * @brief Discards a wl_shm buffer.
 *
 * Unmaps the shared memory region. The file descriptor is closed
 * when the wl_buffer is created by _wl_shm_to_buffer.
 *
 * @param buf The Ecore_Wl2_Buffer whose SHM mapping to unmap.
 */
static void
_wl_shm_discard(Ecore_Wl2_Buffer *buf)
{
   munmap(buf->bh, buf->size);
}

/**
 * @brief Destroys the wl_shm buffer manager.
 *
 * This is a no-op as there's no specific manager data to free for wl_shm.
 */
static void
_wl_shm_manager_destroy()
{
   /* Nop. */
}

/**
 * @brief Creates a wl_buffer from an SHM-based Ecore_Wl2_Buffer.
 *
 * This function uses the wl_shm protocol to create a Wayland buffer
 * from a shared memory file descriptor.
 *
 * @param ewd The Ecore_Wl2_Display.
 * @param db The Ecore_Wl2_Buffer containing SHM information.
 * @return A new wl_buffer, or NULL on failure.
 */
static struct wl_buffer *
_wl_shm_to_buffer(Ecore_Wl2_Display *ewd, Ecore_Wl2_Buffer *db)
{
   struct wl_buffer *buf;
   struct wl_shm_pool *pool;
   struct wl_shm *shm;
   uint32_t format;

   if (db->alpha)
     format = WL_SHM_FORMAT_ARGB8888;
   else
     format = WL_SHM_FORMAT_XRGB8888;

   shm = ecore_wl2_display_shm_get(ewd);
   pool = wl_shm_create_pool(shm, db->fd, db->size);
   buf = wl_shm_pool_create_buffer(pool, 0, db->w, db->h, db->stride, format);
   wl_shm_pool_destroy(pool);
   close(db->fd);
   db->fd = -1;
   wl_buffer_add_listener(buf, &buffer_listener, db);
   return buf;
}

/**
 * @brief Sets up the wl_shm buffer manager.
 *
 * Initializes the buffer manager function pointers for wl_shm-based
 * buffer allocation.
 *
 * @param fd Unused.
 * @return EINA_TRUE always.
 */
static Eina_Bool
_wl_shm_buffer_manager_setup(int fd EINA_UNUSED)
{
   buffer_manager->alloc = _wl_shm_alloc;
   buffer_manager->to_buffer = _wl_shm_to_buffer;
   buffer_manager->map = _wl_shm_map;
   buffer_manager->unmap = _wl_shm_unmap;
   buffer_manager->discard = _wl_shm_discard;
   buffer_manager->manager_destroy = _wl_shm_manager_destroy;
   return EINA_TRUE;
}

struct internal_vc4_bo
{
   __u32 handle;
   int size;
   int fd; /**< File descriptor for the buffer object, if mmapped directly. */
};

/**
 * @brief Aligns a value to a specified boundary.
 * @param v The value to align.
 * @param a The alignment boundary.
 * @return The aligned value.
 */
static int
align(int v, int a)
{
   return (v + a - 1) & ~((uint64_t)a - 1);
}

/**
 * @brief Allocates a buffer using VC4 DRM (e.g., Raspberry Pi).
 *
 * @param self Unused.
 * @param name Unused.
 * @param w Width of the buffer.
 * @param h Height of the buffer.
 * @param stride Pointer to store the calculated stride of the buffer.
 * @param fd Pointer to store the file descriptor of the DMA-BUF.
 * @return A Buffer_Handle (struct internal_vc4_bo *) on success, NULL on failure.
 */
static Buffer_Handle *
_vc4_alloc(Buffer_Manager *self EINA_UNUSED, const char *name EINA_UNUSED, int w, int h, unsigned long *stride, int32_t *fd)
{
   struct drm_vc4_create_bo bo;
   struct internal_vc4_bo *obo;
   struct drm_gem_close cl;
   size_t size;
   int ret;

   obo = malloc(sizeof(struct internal_vc4_bo));
   if (!obo) return NULL;

   *stride = align(w * 4, 16);
   size = *stride * h;
   memset(&bo, 0, sizeof(bo));
   bo.size = size;
   ret = ioctl(drm_fd, DRM_IOCTL_VC4_CREATE_BO, &bo);
   if (ret)
     {
        free(obo);
        return NULL;
     }

   obo->handle = bo.handle;
   obo->size = size;
   /* First try to allocate an mmapable buffer with O_RDWR,
    * if that fails retry unmappable - if the compositor is
    * using GL it won't need to mmap the buffer and this can
    * work - otherwise it'll reject this buffer and we'll
    * have to fall back to shm rendering.
    */
   if (sym_drmPrimeHandleToFD(drm_fd, bo.handle,
                              DRM_CLOEXEC | O_RDWR, fd) != 0)
     if (sym_drmPrimeHandleToFD(drm_fd, bo.handle,
                                DRM_CLOEXEC, fd) != 0) goto err;

   obo->fd = *fd;
   return (Buffer_Handle *)obo;

err:
   memset(&cl, 0, sizeof(cl));
   cl.handle = bo.handle;
   ioctl(drm_fd, DRM_IOCTL_GEM_CLOSE, &cl);
   free(obo);
   return NULL;
}

/**
 * @brief Maps a VC4 buffer for CPU access.
 *
 * Uses DRM_IOCTL_VC4_MMAP_BO to get an offset, then mmap on the DRM fd.
 *
 * @param buf The Ecore_Wl2_Buffer to map.
 * @return A pointer to the mapped memory, or NULL on failure.
 */
static void *
_vc4_map(Ecore_Wl2_Buffer *buf)
{
   struct drm_vc4_mmap_bo map;
   struct internal_vc4_bo *bo;
   void *ptr;
   int ret;

   bo = (struct internal_vc4_bo *)buf->bh;

   memset(&map, 0, sizeof(map));
   map.handle = bo->handle;
   ret = ioctl(drm_fd, DRM_IOCTL_VC4_MMAP_BO, &map);
   if (ret) return NULL;

   ptr = mmap(NULL, bo->size, PROT_READ | PROT_WRITE, MAP_SHARED, drm_fd,
              map.offset);
   if (ptr == MAP_FAILED) return NULL;

   return ptr;
}

/**
 * @brief Unmaps a VC4 buffer.
 *
 * @param buf The Ecore_Wl2_Buffer to unmap.
 */
static void
_vc4_unmap(Ecore_Wl2_Buffer *buf)
{
   struct internal_vc4_bo *bo;

   bo = (struct internal_vc4_bo *)buf->bh;
   munmap(buf->mapping, bo->size);
}

/**
 * @brief Discards/closes a VC4 buffer object.
 *
 * Uses DRM_IOCTL_GEM_CLOSE.
 *
 * @param buf The Ecore_Wl2_Buffer whose underlying VC4 BO to close.
 */
static void
_vc4_discard(Ecore_Wl2_Buffer *buf)
{
   struct drm_gem_close cl;
   struct internal_vc4_bo *bo;

   bo = (struct internal_vc4_bo *)buf->bh;

   memset(&cl, 0, sizeof(cl));
   cl.handle = bo->handle;
   ioctl(drm_fd, DRM_IOCTL_GEM_CLOSE, &cl);
}

/**
 * @brief Sets up the VC4 buffer manager.
 *
 * Checks for VC4 support by trying to create a small BO.
 * Loads libdrm.so for drmPrimeHandleToFD.
 * Initializes the buffer manager function pointers for VC4 DRM-based allocation.
 *
 * @param fd The DRM device file descriptor.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
_vc4_buffer_manager_setup(int fd)
{
   struct drm_gem_close cl;
   struct drm_vc4_create_bo bo;
   Eina_Bool fail = EINA_FALSE;
   void *drm_lib;

   memset(&bo, 0, sizeof(bo));
   bo.size = 32;
   if (ioctl(fd, DRM_IOCTL_VC4_CREATE_BO, &bo)) return EINA_FALSE;

   memset(&cl, 0, sizeof(cl));
   cl.handle = bo.handle;
   ioctl(fd, DRM_IOCTL_GEM_CLOSE, &cl);

   drm_lib = dlopen("libdrm.so", RTLD_LAZY | RTLD_GLOBAL);
   if (!drm_lib) return EINA_FALSE;

   SYM(drm_lib, drmPrimeHandleToFD);

   if (fail) goto err;

   buffer_manager->alloc = _vc4_alloc;
   buffer_manager->to_buffer = _evas_dmabuf_wl_buffer_from_dmabuf;
   buffer_manager->map = _vc4_map;
   buffer_manager->unmap = _vc4_unmap;
   buffer_manager->discard = _vc4_discard;
   buffer_manager->lock = _dmabuf_lock;
   buffer_manager->unlock = _dmabuf_unlock;
   buffer_manager->manager_destroy = NULL;
   buffer_manager->dl_handle = drm_lib;
   return EINA_TRUE;
err:
   dlclose(drm_lib);
   return EINA_FALSE;
}

/**
 * @brief Initializes the buffer management system.
 *
 * This function sets up the appropriate buffer manager (Intel, Exynos, VC4, or SHM)
 * based on availability and requested types. It opens /dev/dri/renderD128
 * for DMA-BUF capable managers.
 *
 * @param ewd The Ecore_Wl2_Display.
 * @param types A bitmask of Ecore_Wl2_Buffer_Type indicating preferred buffer types.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
EAPI Eina_Bool
ecore_wl2_buffer_init(Ecore_Wl2_Display *ewd, Ecore_Wl2_Buffer_Type types)
{
   int fd = -1;
   Eina_Bool dmabuf = ewd->wl.dmabuf && (types & ECORE_WL2_BUFFER_DMABUF);
   Eina_Bool shm = ewd->wl.shm && (types & ECORE_WL2_BUFFER_SHM);
   Eina_Bool success = EINA_FALSE;

   if (buffer_manager)
     {
        buffer_manager->refcount++;
        return EINA_TRUE;
     }

   buffer_manager = calloc(1, sizeof(Buffer_Manager));
   if (!buffer_manager) goto err_alloc;

   if (!getenv("EVAS_WAYLAND_SHM_DISABLE_DMABUF") && dmabuf)
     {
        fd = open("/dev/dri/renderD128", O_RDWR | O_CLOEXEC);
        if (fd < 0)
          {
             ERR("Tried to use dmabufs, but can't find /dev/dri/renderD128 . Falling back to regular SHM");
             goto fallback_shm;
          }

        success = _intel_buffer_manager_setup(fd);
        if (!success) success = _exynos_buffer_manager_setup(fd);
        if (!success) success = _vc4_buffer_manager_setup(fd);
     }
fallback_shm:
   if (!success) success = shm && _wl_shm_buffer_manager_setup(0);
   if (!success) goto err_bm;

   drm_fd = fd;
   buffer_manager->refcount = 1;
   return EINA_TRUE;

err_bm:
   if (fd >= 0) close(fd);
   free(buffer_manager);
   buffer_manager = NULL;
err_alloc:
   return EINA_FALSE;
}

/**
 * @brief Increments the reference count of the global buffer manager.
 */
static void
_buffer_manager_ref(void)
{
   buffer_manager->refcount++;
}

/**
 * @brief Decrements the reference count of the global buffer manager.
 *
 * If the reference count drops to zero and the manager is marked as destroyed,
 * it cleans up and frees the buffer manager resources.
 */
static void
_buffer_manager_deref(void)
{
   buffer_manager->refcount--;
   if (buffer_manager->refcount || !buffer_manager->destroyed) return;

   if (buffer_manager->manager_destroy) buffer_manager->manager_destroy();
   free(buffer_manager);
   buffer_manager = NULL;
   if (drm_fd >=0) close(drm_fd);
}

/**
 * @brief Marks the global buffer manager for destruction.
 *
 * This function sets a flag indicating the buffer manager should be destroyed
 * and then calls _buffer_manager_deref() to potentially perform the cleanup
 * if the refcount is also zero.
 */
static void
_buffer_manager_destroy(void)
{
   if (buffer_manager->destroyed) return;
   buffer_manager->destroyed = EINA_TRUE;
   _buffer_manager_deref();
}

/**
 * @brief Allocates a buffer using the current buffer manager.
 *
 * This is a wrapper around the active buffer manager's alloc function.
 * It also handles reference counting for the buffer manager.
 *
 * @param name Name for the buffer.
 * @param w Width of the buffer.
 * @param h Height of the buffer.
 * @param stride Pointer to store the calculated stride.
 * @param fd Pointer to store the buffer's file descriptor.
 * @return A Buffer_Handle on success, NULL on failure.
 */
static Buffer_Handle *
_buffer_manager_alloc(const char *name, int w, int h, unsigned long *stride, int32_t *fd)
{
   Buffer_Handle *out;

   _buffer_manager_ref();
   out = buffer_manager->alloc(buffer_manager, name, w, h, stride, fd);
   if (!out) _buffer_manager_deref();
   return out;
}

/**
 * @brief Gets the Wayland wl_buffer associated with an Ecore_Wl2_Buffer.
 *
 * @param buf The Ecore_Wl2_Buffer.
 * @return The wl_buffer, or NULL if not created or if buf is NULL.
 */
EAPI struct wl_buffer *
ecore_wl2_buffer_wl_buffer_get(Ecore_Wl2_Buffer *buf)
{
   return buf->wl_buffer;
}

EAPI void *
ecore_wl2_buffer_map(Ecore_Wl2_Buffer *buf, int *w, int *h, int *stride)
{
   void *out;

   EINA_SAFETY_ON_NULL_RETURN_VAL(buf, NULL);

   if (buf->mapping)
     {
        out = buf->mapping;
     }
   else
     {
        _buffer_manager_ref();
        out = buffer_manager->map(buf);
        if (!out)
          {
             _buffer_manager_deref();
             return NULL;
          }
        buf->locked = EINA_TRUE;
        buf->mapping = out;
     }
   if (w) *w = buf->w;
   if (h) *h = buf->h;
   if (stride) *stride = (int)buf->stride;

   if (!buf->locked) ecore_wl2_buffer_lock(buf);

   return out;
}

EAPI void
ecore_wl2_buffer_unmap(Ecore_Wl2_Buffer *buf)
{
   buffer_manager->unmap(buf);
   _buffer_manager_deref();
}

EAPI void
ecore_wl2_buffer_discard(Ecore_Wl2_Buffer *buf)
{
   buffer_manager->discard(buf);
   _buffer_manager_deref();
}

EAPI void
ecore_wl2_buffer_lock(Ecore_Wl2_Buffer *b)
{
   if (b->locked) ERR("Buffer already locked\n");
   if (buffer_manager->lock) buffer_manager->lock(b);
   b->locked = EINA_TRUE;
}

EAPI void
ecore_wl2_buffer_unlock(Ecore_Wl2_Buffer *b)
{
   if (!b->locked) ERR("Buffer already unlocked\n");
   if (buffer_manager->unlock) buffer_manager->unlock(b);
   b->locked = EINA_FALSE;
}

EAPI void
ecore_wl2_buffer_destroy(Ecore_Wl2_Buffer *b)
{
   if (!b) return;

   if (b->locked || b->busy)
     {
        b->orphaned = EINA_TRUE;
        return;
     }
   if (b->fd != -1) close(b->fd);
   if (b->mapping) ecore_wl2_buffer_unmap(b);
   ecore_wl2_buffer_discard(b);
   if (b->wl_buffer) wl_buffer_destroy(b->wl_buffer);
   b->wl_buffer = NULL;
   free(b);
}

EAPI Eina_Bool
ecore_wl2_buffer_busy_get(Ecore_Wl2_Buffer *buffer)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(buffer, EINA_FALSE);

   return (buffer->locked) || (buffer->busy);
}

EAPI void
ecore_wl2_buffer_busy_set(Ecore_Wl2_Buffer *buffer)
{
   EINA_SAFETY_ON_NULL_RETURN(buffer);

   buffer->busy = EINA_TRUE;
}

EAPI int
ecore_wl2_buffer_age_get(Ecore_Wl2_Buffer *buffer)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(buffer, 0);

   return buffer->age;
}

EAPI void ecore_wl2_buffer_age_set(Ecore_Wl2_Buffer *buffer, int age)
{
   EINA_SAFETY_ON_NULL_RETURN(buffer);

   buffer->age = age;
}

EAPI void
ecore_wl2_buffer_age_inc(Ecore_Wl2_Buffer *buffer)
{
   EINA_SAFETY_ON_NULL_RETURN(buffer);

   buffer->age++;
}

/* The only user of this function has been removed, but it
 * will likely come back later.  The problem is that
 * a dmabuf buffer needs to be resized on the compositor
 * even if the allocation still fits.  Doing the resize
 * properly isn't something that will be fixed in the 1.21
 * timeframe, so the optimization has been (temporarily)
 * removed.
 *
 * This is currently beta api - don't move it out of beta
 * with no users...
 */
EAPI Eina_Bool
ecore_wl2_buffer_fit(Ecore_Wl2_Buffer *b, int w, int h)
{
   int stride;

   EINA_SAFETY_ON_NULL_RETURN_VAL(b, EINA_FALSE);

   stride = b->stride;
   if ((w >= b->w) && (w <= stride / 4) && (h == b->h))
     {
        b->w = w;
        return EINA_TRUE;
     }

   return EINA_FALSE;
}

/**
 * @brief Partially creates an Ecore_Wl2_Buffer.
 *
 * This function allocates the Ecore_Wl2_Buffer struct and the underlying
 * hardware/SHM buffer handle using the active buffer manager.
 * It does not yet create the Wayland `wl_buffer` object.
 *
 * @param w Width of the buffer.
 * @param h Height of the buffer.
 * @param alpha Whether the buffer should support alpha.
 * @return A new partially initialized Ecore_Wl2_Buffer, or NULL on failure.
 */
static Ecore_Wl2_Buffer *
_ecore_wl2_buffer_partial_create(int w, int h, Eina_Bool alpha)
{
   Ecore_Wl2_Buffer *out;

   out = calloc(1, sizeof(Ecore_Wl2_Buffer));
   if (!out) return NULL;

   out->fd = -1;
   out->alpha = alpha;
   out->bh = _buffer_manager_alloc("name", w, h, &out->stride, &out->fd);
   if (!out->bh)
     {
        free(out);
        return NULL;
     }
   out->w = w;
   out->h = h;
   out->size = out->stride * h;

   return out;
}

/**
 * @brief Creates a complete Ecore_Wl2_Buffer, including the Wayland wl_buffer.
 *
 * This function first calls _ecore_wl2_buffer_partial_create to allocate
 * the buffer and its handle, then uses the buffer manager's to_buffer
 * function to create the corresponding Wayland `wl_buffer`.
 *
 * @param ewd The Ecore_Wl2_Display.
 * @param w Width of the buffer.
 * @param h Height of the buffer.
 * @param alpha Whether the buffer should support alpha.
 * @return A new Ecore_Wl2_Buffer, or NULL on failure.
 */
EAPI Ecore_Wl2_Buffer *
ecore_wl2_buffer_create(Ecore_Wl2_Display *ewd, int w, int h, Eina_Bool alpha)
{
   Ecore_Wl2_Buffer *out;

   out = _ecore_wl2_buffer_partial_create(w, h, alpha);
   if (!out) return NULL;

   out->wl_buffer = buffer_manager->to_buffer(ewd, out);

   return out;
}

/**
 * @brief Callback for successful creation of a wl_buffer via zwp_linux_buffer_params_v1.
 *
 * Used during DMA-BUF capability testing. Destroys the temporary buffer and params.
 *
 * @param data Unused user data.
 * @param params The zwp_linux_buffer_params_v1 object used for creation.
 * @param new_buffer The newly created (temporary) wl_buffer.
 */
static void
_create_succeeded(void *data EINA_UNUSED,
                 struct zwp_linux_buffer_params_v1 *params,
                 struct wl_buffer *new_buffer)
{
   wl_buffer_destroy(new_buffer);
   zwp_linux_buffer_params_v1_destroy(params);
}

/**
 * @brief Callback for failed creation of a wl_buffer via zwp_linux_buffer_params_v1.
 *
 * Used during DMA-BUF capability testing. Destroys the params,
 * cleans up the buffer manager, and marks DMA-BUF as unavailable on the display.
 *
 * @param data The Ecore_Wl2_Display.
 * @param params The zwp_linux_buffer_params_v1 object that failed.
 */
static void
_create_failed(void *data, struct zwp_linux_buffer_params_v1 *params)
{
   Ecore_Wl2_Display *ewd = data;

   zwp_linux_buffer_params_v1_destroy(params);
   _buffer_manager_destroy();
   ewd->wl.dmabuf = NULL;
}

static const struct zwp_linux_buffer_params_v1_listener params_listener =
{
   _create_succeeded,
   _create_failed
};

/**
 * @brief Tests DMA-BUF buffer creation capability.
 *
 * This function attempts to create a minimal (1x1) DMA-BUF buffer.
 * If successful, it means the system supports DMA-BUF with the current
 * DRM driver. If it fails, DMA-BUF support is disabled for the display.
 * This is typically called during display initialization.
 *
 * @param ewd The Ecore_Wl2_Display to test DMA-BUF on.
 */
void
_ecore_wl2_buffer_test(Ecore_Wl2_Display *ewd)
{
   struct zwp_linux_buffer_params_v1 *dp;
   Ecore_Wl2_Buffer *buf;

   if (!ecore_wl2_buffer_init(ewd, ECORE_WL2_BUFFER_DMABUF)) return;

   buf = _ecore_wl2_buffer_partial_create(1, 1, EINA_TRUE);
   if (!buf) goto fail;

   dp = zwp_linux_dmabuf_v1_create_params(ewd->wl.dmabuf);
   zwp_linux_buffer_params_v1_add(dp, buf->fd, 0, 0, buf->stride, 0, 0);
   zwp_linux_buffer_params_v1_add_listener(dp, &params_listener, ewd);
   zwp_linux_buffer_params_v1_create(dp, buf->w, buf->h,
                                     DRM_FORMAT_ARGB8888, 0);

   ecore_wl2_buffer_destroy(buf);

   return;

fail:
  _buffer_manager_destroy();
  ewd->wl.dmabuf = NULL;
}
