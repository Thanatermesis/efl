/**
 * @file
 * @brief Functions for interacting with the X Xinerama extension.
 *
 * This file provides functions to query Xinerama screen information,
 * such as the number of screens and their geometries. Xinerama allows
 * multiple physical monitors to be treated as a single large virtual screen.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* ifdef HAVE_CONFIG_H */

#include "Ecore.h"
#include "ecore_x_private.h"
#include "Ecore_X.h"
#include "Ecore_X_Atoms.h"

#ifdef ECORE_XINERAMA
static XineramaScreenInfo *_xin_info = NULL;
static int _xin_scr_num = 0;
#endif /* ifdef ECORE_XINERAMA */

/**
 * @brief Retrieves the number of screens managed by Xinerama.
 *
 * This function queries the X server for Xinerama information and returns
 * the number of available screens. If Xinerama is not active or not
 * supported, it returns 0.
 *
 * The Xinerama information is cached internally and refreshed on each call
 * if Xinerama is active.
 *
 * @return The number of Xinerama screens, or 0 if Xinerama is not active
 *         or an error occurs.
 */
EAPI int
ecore_x_xinerama_screen_count_get(void)
{
#ifdef ECORE_XINERAMA
   int event_base, error_base;

   LOGFN;
   if (_xin_info)
     XFree(_xin_info);

   _xin_info = NULL;
   if (XineramaQueryExtension(_ecore_x_disp, &event_base, &error_base))
     {
        if (_ecore_xlib_sync) ecore_x_sync();
        _xin_info = XineramaQueryScreens(_ecore_x_disp, &_xin_scr_num);
        if (_ecore_xlib_sync) ecore_x_sync();
        if (_xin_info)
          return _xin_scr_num;
     }
   if (_ecore_xlib_sync) ecore_x_sync();

#endif /* ifdef ECORE_XINERAMA */
   return 0;
}

/**
 * @brief Retrieves the geometry of a specific Xinerama screen.
 *
 * This function populates the provided pointers with the x-coordinate,
 * y-coordinate, width, and height of the specified Xinerama screen.
 *
 * If Xinerama is not active or the specified screen number is not found,
 * this function will return the geometry of the default screen (screen 0)
 * as reported by the X server (DisplayWidth/DisplayHeight) and return @c EINA_FALSE.
 *
 * The screen geometries are based on the cached Xinerama information obtained
 * from the last call to ecore_x_xinerama_screen_count_get() or a previous
 * call to this function if Xinerama is active.
 *
 * @param[in] screen The screen number whose geometry is to be retrieved.
 *                   This corresponds to the `screen_number` field in
 *                   `XineramaScreenInfo`.
 * @param[out] x Pointer to store the x-coordinate of the screen's origin. Can be NULL.
 * @param[out] y Pointer to store the y-coordinate of the screen's origin. Can be NULL.
 * @param[out] w Pointer to store the width of the screen. Can be NULL.
 * @param[out] h Pointer to store the height of the screen. Can be NULL.
 * @return @c EINA_TRUE if the Xinerama screen geometry was successfully retrieved,
 *         @c EINA_FALSE otherwise (e.g., Xinerama not active, screen not found,
 *         or Xinerama information not available).
 */
EAPI Eina_Bool
ecore_x_xinerama_screen_geometry_get(int screen EINA_UNUSED, // if no xinerama
                                     int *x,
                                     int *y,
                                     int *w,
                                     int *h)
{
   LOGFN;
#ifdef ECORE_XINERAMA
   if (_xin_info)
     {
        int i;

        for (i = 0; i < _xin_scr_num; i++)
          {
             if (_xin_info[i].screen_number == screen)
               {
                  if (x)
                    *x = _xin_info[i].x_org;

                  if (y)
                    *y = _xin_info[i].y_org;

                  if (w)
                    *w = _xin_info[i].width;

                  if (h)
                    *h = _xin_info[i].height;

                  return EINA_TRUE;
               }
          }
     }

#endif /* ifdef ECORE_XINERAMA */
   if (x)
     *x = 0;

   if (y)
     *y = 0;

   if (w)
     *w = DisplayWidth(_ecore_x_disp, 0);

   if (h)
     *h = DisplayHeight(_ecore_x_disp, 0);

   return EINA_FALSE;
}

