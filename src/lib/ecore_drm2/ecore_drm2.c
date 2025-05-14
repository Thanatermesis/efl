#include "ecore_drm2_private.h"

/** @file ecore_drm2.c
 * @brief Ecore DRM2 functions
 */

/** @internal
 * @brief Counter for ecore_drm2_init() calls.
 *
 * This variable keeps track of how many times ecore_drm2_init() has been
 * called. It is used to ensure that initialization and shutdown procedures
 * are performed only once.
 */
static int _ecore_drm2_init_count = 0;

/** @internal
 * @brief Handle to the dynamically loaded libdrm library.
 *
 * This variable stores the handle returned by dlopen() when libdrm is
 * successfully loaded. It is NULL if the library is not loaded.
 */
static void *drm_lib = NULL;

/** @internal
 * @brief Logging domain for Ecore_Drm2.
 *
 * This variable stores the Eina log domain identifier used by Ecore_Drm2
 * for logging messages.
 */
int _ecore_drm2_log_dom = -1;

/** @internal @brief Pointer to the drmHandleEvent function from libdrm. */
int (*sym_drmHandleEvent)(int fd, drmEventContext *evctx) = NULL;
/** @internal @brief Pointer to the drmGetVersion function from libdrm. */
void *(*sym_drmGetVersion)(int fd) = NULL;
/** @internal @brief Pointer to the drmFreeVersion function from libdrm. */
void (*sym_drmFreeVersion)(void *drmver) = NULL;
/** @internal @brief Pointer to the drmModeGetProperty function from libdrm. */
void *(*sym_drmModeGetProperty)(int fd, uint32_t propertyId) = NULL;
/** @internal @brief Pointer to the drmModeFreeProperty function from libdrm. */
void (*sym_drmModeFreeProperty)(drmModePropertyPtr ptr) = NULL;
/** @internal @brief Pointer to the drmModeGetPropertyBlob function from libdrm. */
void *(*sym_drmModeGetPropertyBlob)(int fd, uint32_t blob_id) = NULL;
/** @internal @brief Pointer to the drmModeFreePropertyBlob function from libdrm. */
void (*sym_drmModeFreePropertyBlob)(drmModePropertyBlobPtr ptr) = NULL;
/** @internal @brief Pointer to the drmModeDestroyPropertyBlob function from libdrm. */
int (*sym_drmModeDestroyPropertyBlob)(int fd, uint32_t id) = NULL;
/** @internal @brief Pointer to the drmIoctl function from libdrm. */
int (*sym_drmIoctl)(int fd, unsigned long request, void *arg) = NULL;
/** @internal @brief Pointer to the drmModeObjectGetProperties function from libdrm. */
void *(*sym_drmModeObjectGetProperties)(int fd, uint32_t object_id, uint32_t object_type) = NULL;
/** @internal @brief Pointer to the drmModeFreeObjectProperties function from libdrm. */
void (*sym_drmModeFreeObjectProperties)(drmModeObjectPropertiesPtr ptr) = NULL;
/** @internal @brief Pointer to the drmModeCreatePropertyBlob function from libdrm. */
int (*sym_drmModeCreatePropertyBlob)(int fd, const void *data, size_t size, uint32_t *id) = NULL;
/** @internal @brief Pointer to the drmModeAtomicAlloc function from libdrm. */
void *(*sym_drmModeAtomicAlloc)(void) = NULL;
/** @internal @brief Pointer to the drmModeAtomicFree function from libdrm. */
void (*sym_drmModeAtomicFree)(drmModeAtomicReqPtr req) = NULL;
/** @internal @brief Pointer to the drmModeAtomicAddProperty function from libdrm. */
int (*sym_drmModeAtomicAddProperty)(drmModeAtomicReqPtr req, uint32_t object_id, uint32_t property_id, uint64_t value) = NULL;
/** @internal @brief Pointer to the drmModeAtomicCommit function from libdrm. */
int (*sym_drmModeAtomicCommit)(int fd, drmModeAtomicReqPtr req, uint32_t flags, void *user_data) = NULL;
/** @internal @brief Pointer to the drmModeAtomicSetCursor function from libdrm. */
void (*sym_drmModeAtomicSetCursor)(drmModeAtomicReqPtr req, int cursor) = NULL;
/** @internal @brief Pointer to the drmModeAtomicMerge function from libdrm. */
int (*sym_drmModeAtomicMerge)(drmModeAtomicReqPtr base, drmModeAtomicReqPtr augment);
/** @internal @brief Pointer to the drmModeGetEncoder function from libdrm. */
void *(*sym_drmModeGetEncoder)(int fd, uint32_t encoder_id) = NULL;
/** @internal @brief Pointer to the drmModeFreeEncoder function from libdrm. */
void (*sym_drmModeFreeEncoder)(drmModeEncoderPtr ptr) = NULL;
/** @internal @brief Pointer to the drmModeGetCrtc function from libdrm. */
void *(*sym_drmModeGetCrtc)(int fd, uint32_t crtcId) = NULL;
/** @internal @brief Pointer to the drmModeFreeCrtc function from libdrm. */
void (*sym_drmModeFreeCrtc)(drmModeCrtcPtr ptr) = NULL;
/** @internal @brief Pointer to the drmModeSetCrtc function from libdrm. */
int (*sym_drmModeSetCrtc)(int fd, uint32_t crtcId, uint32_t bufferId, uint32_t x, uint32_t y, uint32_t *connectors, int count, drmModeModeInfoPtr mode) = NULL;
/** @internal @brief Pointer to the drmModeGetResources function from libdrm. */
void *(*sym_drmModeGetResources)(int fd) = NULL;
/** @internal @brief Pointer to the drmModeFreeResources function from libdrm. */
void (*sym_drmModeFreeResources)(drmModeResPtr ptr) = NULL;
/** @internal @brief Pointer to the drmModeGetConnector function from libdrm. */
void *(*sym_drmModeGetConnector)(int fd, uint32_t connectorId) = NULL;
/** @internal @brief Pointer to the drmModeFreeConnector function from libdrm. */
void (*sym_drmModeFreeConnector)(drmModeConnectorPtr ptr) = NULL;
/** @internal @brief Pointer to the drmModeConnectorSetProperty function from libdrm. */
int (*sym_drmModeConnectorSetProperty)(int fd, uint32_t connector_id, uint32_t property_id, uint64_t value) = NULL;
/** @internal @brief Pointer to the drmGetCap function from libdrm. */
int (*sym_drmGetCap)(int fd, uint64_t capability, uint64_t *value) = NULL;
/** @internal @brief Pointer to the drmSetClientCap function from libdrm. */
int (*sym_drmSetClientCap)(int fd, uint64_t capability, uint64_t value) = NULL;
/** @internal @brief Pointer to the drmModeGetPlaneResources function from libdrm. */
void *(*sym_drmModeGetPlaneResources)(int fd) = NULL;
/** @internal @brief Pointer to the drmModeFreePlaneResources function from libdrm. */
void (*sym_drmModeFreePlaneResources)(drmModePlaneResPtr ptr) = NULL;
/** @internal @brief Pointer to the drmModeGetPlane function from libdrm. */
void *(*sym_drmModeGetPlane)(int fd, uint32_t plane_id) = NULL;
/** @internal @brief Pointer to the drmModeFreePlane function from libdrm. */
void (*sym_drmModeFreePlane)(drmModePlanePtr ptr) = NULL;
/** @internal @brief Pointer to the drmModeAddFB function from libdrm. */
int (*sym_drmModeAddFB)(int fd, uint32_t width, uint32_t height, uint8_t depth, uint8_t bpp, uint32_t pitch, uint32_t bo_handle, uint32_t *buf_id) = NULL;
/** @internal @brief Pointer to the drmModeAddFB2 function from libdrm. */
int (*sym_drmModeAddFB2)(int fd, uint32_t width, uint32_t height, uint32_t pixel_format, uint32_t bo_handles[4], uint32_t pitches[4], uint32_t offsets[4], uint32_t *buf_id, uint32_t flags) = NULL;
/** @internal @brief Pointer to the drmModeRmFB function from libdrm. */
int (*sym_drmModeRmFB)(int fd, uint32_t bufferId) = NULL;
/** @internal @brief Pointer to the drmModePageFlip function from libdrm. */
int (*sym_drmModePageFlip)(int fd, uint32_t crtc_id, uint32_t fb_id, uint32_t flags, void *user_data) = NULL;
/** @internal @brief Pointer to the drmModeDirtyFB function from libdrm. */
int (*sym_drmModeDirtyFB)(int fd, uint32_t bufferId, drmModeClipPtr clips, uint32_t num_clips) = NULL;
/** @internal @brief Pointer to the drmModeCrtcSetGamma function from libdrm. */
int (*sym_drmModeCrtcSetGamma)(int fd, uint32_t crtc_id, uint32_t size, uint16_t *red, uint16_t *green, uint16_t *blue) = NULL;
/** @internal @brief Pointer to the drmPrimeFDToHandle function from libdrm. */
int (*sym_drmPrimeFDToHandle)(int fd, int prime_fd, uint32_t *handle) = NULL;
/** @internal @brief Pointer to the drmWaitVBlank function from libdrm. */
int (*sym_drmWaitVBlank)(int fd, drmVBlank *vbl) = NULL;

/** @brief Event type for output changes.
 *
 * This event is triggered when an output (connector) property changes,
 * such as connection status or mode.
 */
EAPI int ECORE_DRM2_EVENT_OUTPUT_CHANGED = -1;

/** @brief Event type for device activation/deactivation.
 *
 * This event is triggered when a DRM device is activated or deactivated,
 * for example, when switching VTs.
 */
EAPI int ECORE_DRM2_EVENT_ACTIVATE = -1;

/**
 * @internal
 * @brief Dynamically links to libdrm and resolves required symbols.
 *
 * This function attempts to load libdrm using dlopen and then resolves
 * all necessary DRM function symbols using dlsym. It tries a list of
 * common libdrm shared object names.
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
static Eina_Bool
_ecore_drm2_link(void)
{
   int i, fail;
   const char *drm_libs[] =
     {
        "libdrm.so.2",
        "libdrm.so.1",
        "libdrm.so.0",
        "libdrm.so",
        NULL,
     };

#define SYM(lib, xx)                         \
   do {                                      \
      sym_ ## xx = dlsym(lib, #xx);          \
      if (!(sym_ ## xx)) {                   \
         fail = 1;                           \
      }                                      \
   } while (0)

   if (drm_lib) return EINA_TRUE;

   for (i = 0; drm_libs[i]; i++)
     {
        drm_lib = dlopen(drm_libs[i], RTLD_LOCAL | RTLD_LAZY);
        if (!drm_lib) continue;

        fail = 0;

        SYM(drm_lib, drmIoctl);
        /* SYM(drm_lib, drmClose); */
        SYM(drm_lib, drmWaitVBlank);
        SYM(drm_lib, drmHandleEvent);
        SYM(drm_lib, drmGetVersion);
        SYM(drm_lib, drmFreeVersion);
        SYM(drm_lib, drmModeGetProperty);
        SYM(drm_lib, drmModeFreeProperty);
        SYM(drm_lib, drmModeGetPropertyBlob);
        SYM(drm_lib, drmModeFreePropertyBlob);
        SYM(drm_lib, drmModeDestroyPropertyBlob);
        SYM(drm_lib, drmModeObjectGetProperties);
        SYM(drm_lib, drmModeFreeObjectProperties);
        SYM(drm_lib, drmModeCreatePropertyBlob);
        SYM(drm_lib, drmModeAtomicAlloc);
        SYM(drm_lib, drmModeAtomicFree);
        SYM(drm_lib, drmModeAtomicAddProperty);
        SYM(drm_lib, drmModeAtomicCommit);
        SYM(drm_lib, drmModeAtomicSetCursor);
        SYM(drm_lib, drmModeAtomicMerge);
        SYM(drm_lib, drmModeGetEncoder);
        SYM(drm_lib, drmModeFreeEncoder);
        SYM(drm_lib, drmModeGetCrtc);
        SYM(drm_lib, drmModeFreeCrtc);
        SYM(drm_lib, drmModeSetCrtc);
        SYM(drm_lib, drmModeGetResources);
        SYM(drm_lib, drmModeFreeResources);
        SYM(drm_lib, drmModeGetConnector);
        SYM(drm_lib, drmModeFreeConnector);
        SYM(drm_lib, drmModeConnectorSetProperty);
        SYM(drm_lib, drmGetCap);
        SYM(drm_lib, drmSetClientCap);
        SYM(drm_lib, drmModeGetPlaneResources);
        SYM(drm_lib, drmModeFreePlaneResources);
        SYM(drm_lib, drmModeGetPlane);
        SYM(drm_lib, drmModeFreePlane);
        SYM(drm_lib, drmModeAddFB);
        SYM(drm_lib, drmModeAddFB2);
        SYM(drm_lib, drmModeRmFB);
        SYM(drm_lib, drmModePageFlip);
        SYM(drm_lib, drmModeDirtyFB);
        SYM(drm_lib, drmModeCrtcSetGamma);
        SYM(drm_lib, drmPrimeFDToHandle);

        if (fail)
          {
             dlclose(drm_lib);
             drm_lib = NULL;
          }
        else
          break;
     }

   if (!drm_lib) return EINA_FALSE;
   return EINA_TRUE;
}

/**
 * @brief Initializes the Ecore_Drm2 library.
 *
 * This function initializes all the necessary subsystems for Ecore_Drm2 to
 * operate, including Eina, Ecore, Eeze, and Elput. It also registers a
 * logging domain and creates event types. Finally, it attempts to link
 * against libdrm.
 *
 * This function should be called before any other Ecore_Drm2 functions.
 *
 * @return The new init count, or 0 on failure.
 *
 * @see ecore_drm2_shutdown()
 */
EAPI int
ecore_drm2_init(void)
{
   if (++_ecore_drm2_init_count != 1) return _ecore_drm2_init_count;

   if (!eina_init()) goto eina_err;

   if (!ecore_init())
     {
        EINA_LOG_ERR("Could not initialize Ecore library");
        goto ecore_err;
     }

   if (!eeze_init())
     {
        EINA_LOG_ERR("Could not initialize Eeze library");
        goto eeze_err;
     }

   if (!elput_init())
     {
        EINA_LOG_ERR("Could not initialize Elput library");
        goto elput_err;
     }

   _ecore_drm2_log_dom =
     eina_log_domain_register("ecore_drm2", ECORE_DRM2_DEFAULT_LOG_COLOR);
   if (!_ecore_drm2_log_dom)
     {
        EINA_LOG_ERR("Could not create logging domain for Ecore_Drm2");
        goto log_err;
     }

   ECORE_DRM2_EVENT_OUTPUT_CHANGED = ecore_event_type_new();
   ECORE_DRM2_EVENT_ACTIVATE = ecore_event_type_new();

   if (!_ecore_drm2_link()) goto link_err;

   return _ecore_drm2_init_count;

link_err:
   eina_log_domain_unregister(_ecore_drm2_log_dom);
   _ecore_drm2_log_dom = -1;
log_err:
   elput_shutdown();
elput_err:
   eeze_shutdown();
eeze_err:
   ecore_shutdown();
ecore_err:
   eina_shutdown();
eina_err:
   return --_ecore_drm2_init_count;
}

/**
 * @brief Shuts down the Ecore_Drm2 library.
 *
 * This function shuts down all the subsystems initialized by ecore_drm2_init()
 * and unregisters the logging domain and event types. It should be called
 * when Ecore_Drm2 is no longer needed.
 *
 * @return The new init count.
 *
 * @see ecore_drm2_init()
 */
EAPI int
ecore_drm2_shutdown(void)
{
   if (_ecore_drm2_init_count < 1)
     {
        ERR("Ecore_Drm2 shutdown called without init");
        return 0;
     }

   if (--_ecore_drm2_init_count != 0) return _ecore_drm2_init_count;

   ECORE_DRM2_EVENT_OUTPUT_CHANGED = -1;
   ECORE_DRM2_EVENT_ACTIVATE = -1;

   eina_log_domain_unregister(_ecore_drm2_log_dom);
   _ecore_drm2_log_dom = -1;

   elput_shutdown();
   eeze_shutdown();
   ecore_shutdown();
   eina_shutdown();

   return _ecore_drm2_init_count;
}

/**
 * @brief Handles DRM events.
 *
 * This function processes pending DRM events for the given device using
 * the provided DRM context. It wraps the `drmHandleEvent` function from libdrm.
 *
 * @param dev The Ecore_Drm2_Device to handle events for.
 * @param drmctx The Ecore_Drm2_Context containing event handlers.
 *               The `page_flip_handler` and `vblank_handler` from this
 *               context will be used.
 * @return 0 on success, or a negative error code on failure.
 *
 * @see drmHandleEvent
 * @see Ecore_Drm2_Context
 */
EAPI int
ecore_drm2_event_handle(Ecore_Drm2_Device *dev, Ecore_Drm2_Context *drmctx)
{
   drmEventContext ctx;

   EINA_SAFETY_ON_NULL_RETURN_VAL(dev, -1);

   memset(&ctx, 0, sizeof(ctx));
   ctx.version = 2;
   ctx.page_flip_handler = drmctx->page_flip_handler;
   ctx.vblank_handler = drmctx->vblank_handler;

   return sym_drmHandleEvent(dev->fd, &ctx);
}
