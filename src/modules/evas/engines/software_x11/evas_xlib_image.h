#include "evas_engine.h"
#include <Ecore_X.h>
#include "../software_generic/evas_native_common.h"

/**
 * @brief Sets a native Xlib surface for an Evas image.
 *
 * This function configures an Evas image object to use an existing Xlib
 * Pixmap as its underlying data source. This allows Evas to draw
 * directly onto Xlib surfaces or to read pixel data from them.
 *
 * @param data The Evas engine data. Not used in this Xlib implementation.
 * @param image Pointer to the Evas RGBA_Image object to be configured.
 * @param native Pointer to an Evas_Native_Surface structure describing the
 *               Xlib Pixmap and Visual to use.
 *               Example:
 *               Evas_Native_Surface ns;
 *               ns.type = EVAS_NATIVE_SURFACE_X11;
 *               ns.version = EVAS_NATIVE_SURFACE_VERSION;
 *               ns.data.x11.visual = xlib_visual;
 *               ns.data.x11.pixmap = xlib_pixmap;
 * @return Returns the `image` pointer on success, or NULL on failure.
 *         The `image`'s native surface data and callbacks will be updated.
 */
void *evas_xlib_image_native_set(void *data, void *image, void *native);
