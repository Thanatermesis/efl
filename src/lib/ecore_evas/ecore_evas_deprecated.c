#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h> /* for NULL */

#include <Ecore.h>
#include "ecore_private.h"

#include "Ecore_Evas.h"
#include "ecore_evas_private.h"


/* Ecore_Evas WinCE support was removed. However we keep the functions
 * to not break ABI.
 */

/**
 * @brief Creates a new Ecore_Evas using the WinCE software engine.
 * @deprecated WinCE support was removed. This function is kept for ABI compatibility and always returns NULL.
 * @param parent The parent window.
 * @param x The x coordinate of the Ecore_Evas.
 * @param y The y coordinate of the Ecore_Evas.
 * @param width The width of the Ecore_Evas.
 * @param height The height of the Ecore_Evas.
 * @return A new Ecore_Evas instance or NULL on failure. Always returns NULL.
 */
EAPI Ecore_Evas *
ecore_evas_software_wince_new(Ecore_WinCE_Window *parent EINA_UNUSED,
                              int                 x EINA_UNUSED,
                              int                 y EINA_UNUSED,
                              int                 width EINA_UNUSED,
                              int                 height EINA_UNUSED)
{
   return NULL;
}

/**
 * @brief Creates a new Ecore_Evas using the WinCE software engine with framebuffer.
 * @deprecated WinCE support was removed. This function is kept for ABI compatibility and always returns NULL.
 * @param parent The parent window.
 * @param x The x coordinate of the Ecore_Evas.
 * @param y The y coordinate of the Ecore_Evas.
 * @param width The width of the Ecore_Evas.
 * @param height The height of the Ecore_Evas.
 * @return A new Ecore_Evas instance or NULL on failure. Always returns NULL.
 */
EAPI Ecore_Evas *
ecore_evas_software_wince_fb_new(Ecore_WinCE_Window *parent EINA_UNUSED,
                                 int                 x EINA_UNUSED,
                                 int                 y EINA_UNUSED,
                                 int                 width EINA_UNUSED,
                                 int                 height EINA_UNUSED)
{
   return NULL;
}

/**
 * @brief Creates a new Ecore_Evas using the WinCE software engine with GAPI.
 * @deprecated WinCE support was removed. This function is kept for ABI compatibility and always returns NULL.
 * @param parent The parent window.
 * @param x The x coordinate of the Ecore_Evas.
 * @param y The y coordinate of the Ecore_Evas.
 * @param width The width of the Ecore_Evas.
 * @param height The height of the Ecore_Evas.
 * @return A new Ecore_Evas instance or NULL on failure. Always returns NULL.
 */
EAPI Ecore_Evas *
ecore_evas_software_wince_gapi_new(Ecore_WinCE_Window *parent EINA_UNUSED,
                                   int                 x EINA_UNUSED,
                                   int                 y EINA_UNUSED,
                                   int                 width EINA_UNUSED,
                                   int                 height EINA_UNUSED)
{
   return NULL;
}

/**
 * @brief Creates a new Ecore_Evas using the WinCE software engine with DDraw.
 * @deprecated WinCE support was removed. This function is kept for ABI compatibility and always returns NULL.
 * @param parent The parent window.
 * @param x The x coordinate of the Ecore_Evas.
 * @param y The y coordinate of the Ecore_Evas.
 * @param width The width of the Ecore_Evas.
 * @param height The height of the Ecore_Evas.
 * @return A new Ecore_Evas instance or NULL on failure. Always returns NULL.
 */
EAPI Ecore_Evas *
ecore_evas_software_wince_ddraw_new(Ecore_WinCE_Window *parent EINA_UNUSED,
                                    int                 x EINA_UNUSED,
                                    int                 y EINA_UNUSED,
                                    int                 width EINA_UNUSED,
                                    int                 height EINA_UNUSED)
{
   return NULL;
}

/**
 * @brief Creates a new Ecore_Evas using the WinCE software engine with GDI.
 * @deprecated WinCE support was removed. This function is kept for ABI compatibility and always returns NULL.
 * @param parent The parent window.
 * @param x The x coordinate of the Ecore_Evas.
 * @param y The y coordinate of the Ecore_Evas.
 * @param width The width of the Ecore_Evas.
 * @param height The height of the Ecore_Evas.
 * @return A new Ecore_Evas instance or NULL on failure. Always returns NULL.
 */
EAPI Ecore_Evas *
ecore_evas_software_wince_gdi_new(Ecore_WinCE_Window *parent EINA_UNUSED,
                                  int                 x EINA_UNUSED,
                                  int                 y EINA_UNUSED,
                                  int                 width EINA_UNUSED,
                                  int                 height EINA_UNUSED)
{
   return NULL;
}

/**
 * @brief Creates a new Ecore_Evas using the Direct3D engine.
 * @deprecated This function is kept for ABI compatibility and always returns NULL.
 * @param parent The parent Win32 window.
 * @param x The x coordinate of the Ecore_Evas.
 * @param y The y coordinate of the Ecore_Evas.
 * @param width The width of the Ecore_Evas.
 * @param height The height of the Ecore_Evas.
 * @return A new Ecore_Evas instance or NULL on failure. Always returns NULL.
 */
EAPI Ecore_Evas *
ecore_evas_direct3d_new(Ecore_Win32_Window *parent EINA_UNUSED,
			int                 x EINA_UNUSED,
			int                 y EINA_UNUSED,
			int                 width EINA_UNUSED,
			int                 height EINA_UNUSED)
{
   return NULL;
}

/**
 * @brief Creates a new Ecore_Evas using the OpenGL GLEW engine.
 * @deprecated This function is kept for ABI compatibility and always returns NULL.
 * @param parent The parent Win32 window.
 * @param x The x coordinate of the Ecore_Evas.
 * @param y The y coordinate of the Ecore_Evas.
 * @param width The width of the Ecore_Evas.
 * @param height The height of the Ecore_Evas.
 * @return A new Ecore_Evas instance or NULL on failure. Always returns NULL.
 */
EAPI Ecore_Evas *
ecore_evas_gl_glew_new(Ecore_Win32_Window *parent EINA_UNUSED,
		       int                 x EINA_UNUSED,
		       int                 y EINA_UNUSED,
		       int                 width EINA_UNUSED,
		       int                 height EINA_UNUSED)
{
   return NULL;
}

/**
 * @brief Retrieves the WinCE window associated with an Ecore_Evas.
 * @deprecated WinCE support was removed. This function is kept for ABI compatibility and always returns NULL.
 * @param ee The Ecore_Evas instance.
 * @return The WinCE window or NULL if not applicable. Always returns NULL.
 */
EAPI Ecore_WinCE_Window *
ecore_evas_software_wince_window_get(const Ecore_Evas *ee EINA_UNUSED)
{
   return NULL;
}

/* Ecore_Evas DirectFB support was removed. However we keep the functions
 * to not break ABI.
 */

/**
 * @brief Creates a new Ecore_Evas using the DirectFB engine.
 * @deprecated DirectFB support was removed. This function is kept for ABI compatibility and always returns NULL.
 * @param disp_name The display name.
 * @param windowed Whether the Ecore_Evas should be windowed.
 * @param x The x coordinate of the Ecore_Evas.
 * @param y The y coordinate of the Ecore_Evas.
 * @param w The width of the Ecore_Evas.
 * @param h The height of the Ecore_Evas.
 * @return A new Ecore_Evas instance or NULL on failure. Always returns NULL.
 */
EAPI Ecore_Evas *
ecore_evas_directfb_new(const char *disp_name EINA_UNUSED, int windowed EINA_UNUSED, int x EINA_UNUSED, int y EINA_UNUSED, int w EINA_UNUSED, int h EINA_UNUSED)
{
   return NULL;
}

/**
 * @brief Retrieves the DirectFB window associated with an Ecore_Evas.
 * @deprecated DirectFB support was removed. This function is kept for ABI compatibility and always returns NULL.
 * @param ee The Ecore_Evas instance.
 * @return The DirectFB window or NULL if not applicable. Always returns NULL.
 */
EAPI Ecore_DirectFB_Window *
ecore_evas_directfb_window_get(const Ecore_Evas *ee EINA_UNUSED)
{
  return NULL;
}

/* Ecore_Evas X11 16 bits support was removed. However we keep the functions
 * to not break ABI.
 */

/**
 * @brief Creates a new Ecore_Evas using the X11 16-bit software engine.
 * @deprecated X11 16-bit software support was removed. This function is kept for ABI compatibility and always returns NULL.
 * @param disp_name The display name.
 * @param parent The parent X window.
 * @param x The x coordinate of the Ecore_Evas.
 * @param y The y coordinate of the Ecore_Evas.
 * @param w The width of the Ecore_Evas.
 * @param h The height of the Ecore_Evas.
 * @return A new Ecore_Evas instance or NULL on failure. Always returns NULL.
 */
EAPI Ecore_Evas *
ecore_evas_software_x11_16_new(const char *disp_name EINA_UNUSED, Ecore_X_Window parent EINA_UNUSED,
                               int x EINA_UNUSED, int y EINA_UNUSED, int w EINA_UNUSED, int h EINA_UNUSED)
{
   return NULL;
}

/**
 * @brief Retrieves the X11 window associated with a 16-bit software Ecore_Evas.
 * @deprecated X11 16-bit software support was removed. This function is kept for ABI compatibility and always returns 0.
 * @param ee The Ecore_Evas instance.
 * @return The X11 window or 0 if not applicable. Always returns 0.
 */
EAPI Ecore_X_Window
ecore_evas_software_x11_16_window_get(const Ecore_Evas *ee EINA_UNUSED)
{
   return 0;
}

/**
 * @brief Sets whether direct resize is enabled for a 16-bit software X11 Ecore_Evas.
 * @deprecated X11 16-bit software support was removed. This function is kept for ABI compatibility and has no effect.
 * @param ee The Ecore_Evas instance.
 * @param on EINA_TRUE to enable direct resize, EINA_FALSE to disable.
 */
EAPI void
ecore_evas_software_x11_16_direct_resize_set(Ecore_Evas *ee EINA_UNUSED, Eina_Bool on EINA_UNUSED)
{
}

/**
 * @brief Gets whether direct resize is enabled for a 16-bit software X11 Ecore_Evas.
 * @deprecated X11 16-bit software support was removed. This function is kept for ABI compatibility and always returns EINA_FALSE.
 * @param ee The Ecore_Evas instance.
 * @return EINA_TRUE if direct resize is enabled, EINA_FALSE otherwise. Always returns EINA_FALSE.
 */
EAPI Eina_Bool
ecore_evas_software_x11_16_direct_resize_get(const Ecore_Evas *ee EINA_UNUSED)
{
   return EINA_FALSE;
}

/**
 * @brief Adds an extra event window for a 16-bit software X11 Ecore_Evas.
 * @deprecated X11 16-bit software support was removed. This function is kept for ABI compatibility and has no effect.
 * @param ee The Ecore_Evas instance.
 * @param win The extra X window.
 */
EAPI void
ecore_evas_software_x11_16_extra_event_window_add(Ecore_Evas *ee EINA_UNUSED, Ecore_X_Window win EINA_UNUSED)
{
}

/* Ecore_Evas X11 8 bits support was removed. However we keep the functions
 * to not break ABI.
 */

/**
 * @brief Creates a new Ecore_Evas using the X11 8-bit software engine.
 * @deprecated X11 8-bit software support was removed. This function is kept for ABI compatibility and always returns NULL.
 * @param disp_name The display name.
 * @param parent The parent X window.
 * @param x The x coordinate of the Ecore_Evas.
 * @param y The y coordinate of the Ecore_Evas.
 * @param w The width of the Ecore_Evas.
 * @param h The height of the Ecore_Evas.
 * @return A new Ecore_Evas instance or NULL on failure. Always returns NULL.
 */
EAPI Ecore_Evas *
ecore_evas_software_x11_8_new(const char *disp_name EINA_UNUSED, Ecore_X_Window parent EINA_UNUSED, int x EINA_UNUSED, int y EINA_UNUSED, int w EINA_UNUSED, int h EINA_UNUSED)
{
   return NULL;
}

/**
 * @brief Retrieves the X11 window associated with an 8-bit software Ecore_Evas.
 * @deprecated X11 8-bit software support was removed. This function is kept for ABI compatibility and always returns 0.
 * @param ee The Ecore_Evas instance.
 * @return The X11 window or 0 if not applicable. Always returns 0.
 */
EAPI Ecore_X_Window
ecore_evas_software_x11_8_window_get(const Ecore_Evas *ee EINA_UNUSED)
{
   return 0;
}

/**
 * @brief Retrieves the X11 subwindow associated with an 8-bit software Ecore_Evas.
 * @deprecated X11 8-bit software support was removed. This function is kept for ABI compatibility and always returns 0.
 * @param ee The Ecore_Evas instance.
 * @return The X11 subwindow or 0 if not applicable. Always returns 0.
 */
EAPI Ecore_X_Window
ecore_evas_software_x11_8_subwindow_get(const Ecore_Evas *ee EINA_UNUSED)
{
   return 0;
}

/**
 * @brief Sets whether direct resize is enabled for an 8-bit software X11 Ecore_Evas.
 * @deprecated X11 8-bit software support was removed. This function is kept for ABI compatibility and has no effect.
 * @param ee The Ecore_Evas instance.
 * @param on EINA_TRUE to enable direct resize, EINA_FALSE to disable.
 */
EAPI void
ecore_evas_software_x11_8_direct_resize_set(Ecore_Evas *ee EINA_UNUSED, Eina_Bool on EINA_UNUSED)
{
}

/**
 * @brief Gets whether direct resize is enabled for an 8-bit software X11 Ecore_Evas.
 * @deprecated X11 8-bit software support was removed. This function is kept for ABI compatibility and always returns EINA_FALSE.
 * @param ee The Ecore_Evas instance.
 * @return EINA_TRUE if direct resize is enabled, EINA_FALSE otherwise. Always returns EINA_FALSE.
 */
EAPI Eina_Bool
ecore_evas_software_x11_8_direct_resize_get(const Ecore_Evas *ee EINA_UNUSED)
{
   return EINA_FALSE;
}

/**
 * @brief Adds an extra event window for an 8-bit software X11 Ecore_Evas.
 * @deprecated X11 8-bit software support was removed. This function is kept for ABI compatibility and has no effect.
 * @param ee The Ecore_Evas instance.
 * @param win The extra X window.
 */
EAPI void
ecore_evas_software_x11_8_extra_event_window_add(Ecore_Evas *ee EINA_UNUSED, Ecore_X_Window win EINA_UNUSED)
{
   return;
}

/* Ecore_Evas XRender support was removed. However we keep the functions
 * to not break ABI.
 */

/**
 * @brief Creates a new Ecore_Evas using the XRender X11 engine.
 * @deprecated XRender support was removed. This function is kept for ABI compatibility and always returns NULL.
 * @param disp_name The display name.
 * @param parent The parent X window.
 * @param x The x coordinate of the Ecore_Evas.
 * @param y The y coordinate of the Ecore_Evas.
 * @param w The width of the Ecore_Evas.
 * @param h The height of the Ecore_Evas.
 * @return A new Ecore_Evas instance or NULL on failure. Always returns NULL.
 */
EAPI Ecore_Evas *
ecore_evas_xrender_x11_new(const char *disp_name EINA_UNUSED, Ecore_X_Window parent EINA_UNUSED,
                           int x EINA_UNUSED, int y EINA_UNUSED, int w EINA_UNUSED, int h EINA_UNUSED)
{
   return NULL;
}

/**
 * @brief Retrieves the X11 window associated with an XRender Ecore_Evas.
 * @deprecated XRender support was removed. This function is kept for ABI compatibility and always returns 0.
 * @param ee The Ecore_Evas instance.
 * @return The X11 window or 0 if not applicable. Always returns 0.
 */
EAPI Ecore_X_Window
ecore_evas_xrender_x11_window_get(const Ecore_Evas *ee EINA_UNUSED)
{
   return 0;
}

/**
 * @brief Sets whether direct resize is enabled for an XRender X11 Ecore_Evas.
 * @deprecated XRender support was removed. This function is kept for ABI compatibility and has no effect.
 * @param ee The Ecore_Evas instance.
 * @param on EINA_TRUE to enable direct resize, EINA_FALSE to disable.
 */
EAPI void
ecore_evas_xrender_x11_direct_resize_set(Ecore_Evas *ee EINA_UNUSED, Eina_Bool on EINA_UNUSED)
{
}

/**
 * @brief Gets whether direct resize is enabled for an XRender X11 Ecore_Evas.
 * @deprecated XRender support was removed. This function is kept for ABI compatibility and always returns 0.
 * @param ee The Ecore_Evas instance.
 * @return EINA_TRUE if direct resize is enabled, EINA_FALSE otherwise. Always returns 0.
 */
EAPI Eina_Bool
ecore_evas_xrender_x11_direct_resize_get(const Ecore_Evas *ee EINA_UNUSED)
{
   return 0;
}

/**
 * @brief Adds an extra event window for an XRender X11 Ecore_Evas.
 * @deprecated XRender support was removed. This function is kept for ABI compatibility and has no effect.
 * @param ee The Ecore_Evas instance.
 * @param win The extra X window.
 */
EAPI void
ecore_evas_xrender_x11_extra_event_window_add(Ecore_Evas *ee EINA_UNUSED, Ecore_X_Window win EINA_UNUSED)
{
}
