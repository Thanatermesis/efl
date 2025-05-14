#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* ifdef HAVE_CONFIG_H */

#include "ecore_x_private.h"
#include "Ecore_X.h"

/**
 * @internal
 * @brief Flag indicating if the XDamage extension is available.
 */
static Eina_Bool _damage_available = EINA_FALSE;
#ifdef ECORE_XDAMAGE
/**
 * @internal
 * @brief Major version of the XDamage extension.
 */
static int _damage_major;
/**
 * @internal
 * @brief Minor version of the XDamage extension.
 */
static int _damage_minor;
#endif /* ifdef ECORE_XDAMAGE */

/**
 * @internal
 * @brief Initializes the XDamage extension.
 *
 * This function queries the X server for the presence and version of the
 * XDamage extension. It sets the internal flag _damage_available accordingly.
 */
void
_ecore_x_damage_init(void)
{
#ifdef ECORE_XDAMAGE
   _damage_major = 1;
   _damage_minor = 0;

   LOGFN;
   if (XDamageQueryVersion(_ecore_x_disp, &_damage_major, &_damage_minor))
     _damage_available = EINA_TRUE;
   else
     _damage_available = EINA_FALSE;

#else /* ifdef ECORE_XDAMAGE */
   _damage_available = EINA_FALSE;
#endif /* ifdef ECORE_XDAMAGE */
}

/**
 * @brief Checks if the XDamage extension is available.
 *
 * @return @c EINA_TRUE if the XDamage extension is available,
 *         @c EINA_FALSE otherwise.
 */
EAPI Eina_Bool
ecore_x_damage_query(void)
{
   return _damage_available;
}

/**
 * @brief Creates a new XDamage object.
 *
 * This function creates an XDamage object associated with the given drawable.
 * The XDamage object can then be used to receive events when the drawable
 * is damaged.
 *
 * @param d The drawable to monitor for damage.
 * @param level The report level for damage events.
 *              Example: @c ECORE_X_DAMAGE_REPORT_RAW_RECTANGLES,
 *                       @c ECORE_X_DAMAGE_REPORT_DELTA_RECTANGLES,
 *                       @c ECORE_X_DAMAGE_REPORT_BOUNDING_BOX,
 *                       @c ECORE_X_DAMAGE_REPORT_NON_EMPTY.
 * @return The newly created XDamage object, or 0 on failure.
 */
EAPI Ecore_X_Damage
ecore_x_damage_new(Ecore_X_Drawable d,
                   Ecore_X_Damage_Report_Level level)
{
#ifdef ECORE_XDAMAGE
   Ecore_X_Damage damage;

   LOGFN;
   EINA_SAFETY_ON_NULL_RETURN_VAL(_ecore_x_disp, 0);
   damage = XDamageCreate(_ecore_x_disp, d, level);
   if (_ecore_xlib_sync) ecore_x_sync();
   return damage;
#else /* ifdef ECORE_XDAMAGE */
   return 0;
#endif /* ifdef ECORE_XDAMAGE */
}

/**
 * @brief Frees an XDamage object.
 *
 * @param damage The XDamage object to free.
 */
EAPI void
ecore_x_damage_free(Ecore_X_Damage damage)
{
#ifdef ECORE_XDAMAGE
   LOGFN;
   EINA_SAFETY_ON_NULL_RETURN(_ecore_x_disp);
   XDamageDestroy(_ecore_x_disp, damage);
#endif /* ifdef ECORE_XDAMAGE */
}

/**
 * @brief Subtracts a region from the damaged area of an XDamage object.
 *
 * This function is used to inform the X server that a region of the
 * drawable associated with the XDamage object has been repaired.
 * The server will then clear this region from its record of damaged areas.
 *
 * @param damage The XDamage object.
 * @param repair The region that has been repaired. This can be @c None.
 * @param parts The region representing the area to be kept as damaged.
 *              This can be @c None. If @c repair is not @c None, this
 *              parameter is ignored by the X server.
 */
EAPI void
ecore_x_damage_subtract(Ecore_X_Damage damage,
                        Ecore_X_Region repair,
                        Ecore_X_Region parts)
{
#ifdef ECORE_XDAMAGE
   LOGFN;
   EINA_SAFETY_ON_NULL_RETURN(_ecore_x_disp);
   XDamageSubtract(_ecore_x_disp, damage, repair, parts);
   if (_ecore_xlib_sync) ecore_x_sync();
#endif /* ifdef ECORE_XDAMAGE */
}

