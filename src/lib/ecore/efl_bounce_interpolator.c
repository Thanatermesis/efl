#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "Ecore.h"
#include "ecore_private.h"

#define MY_CLASS EFL_BOUNCE_INTERPOLATOR_CLASS

typedef struct _Efl_Bounce_Interpolator_Data Efl_Bounce_Interpolator_Data;

/**
 * @brief Private data for the Efl_Bounce_Interpolator class.
 */
struct _Efl_Bounce_Interpolator_Data
{
   double rigidness; /**< The rigidness of the bounce. Higher values make the bounce more stiff. */
   int bounces;      /**< The number of bounces. */
};

/**
 * @brief Interpolates the input progress value using a bounce effect.
 *
 * @param eo_obj The Efl_Interpolator object.
 * @param pd The private data of the Efl_Bounce_Interpolator.
 * @param progress The input progress value, typically between 0.0 and 1.0.
 *
 * @return The interpolated progress value. If progress is outside the [0.0, 1.0] range,
 *         it is returned unchanged. Otherwise, it returns the progress mapped through
 *         a bounce animation curve.
 */
EOLIAN static double
_efl_bounce_interpolator_efl_interpolator_interpolate(Eo *eo_obj EINA_UNUSED,
                                                      Efl_Bounce_Interpolator_Data *pd EINA_UNUSED,
                                                      double progress)
{
   if ((progress < 0.0) || (progress > 1.0))
     return progress;

   return ecore_animator_pos_map(progress, ECORE_POS_MAP_BOUNCE,
                                 pd->rigidness, (double)pd->bounces);
}

/**
 * @brief Sets the rigidness of the bounce interpolator.
 *
 * @param eo_obj The Efl_Interpolator object.
 * @param pd The private data of the Efl_Bounce_Interpolator.
 * @param rigidness The new rigidness value. For example, 1.0 is a common default.
 */
EOLIAN static void
_efl_bounce_interpolator_rigidness_set(Eo *eo_obj EINA_UNUSED,
                                       Efl_Bounce_Interpolator_Data *pd,
                                       double rigidness)
{
   pd->rigidness = rigidness;
}

/**
 * @brief Gets the rigidness of the bounce interpolator.
 *
 * @param eo_obj The Efl_Interpolator object.
 * @param pd The private data of the Efl_Bounce_Interpolator.
 *
 * @return The current rigidness value.
 */
EOLIAN static double
_efl_bounce_interpolator_rigidness_get(const Eo *eo_obj EINA_UNUSED,
                                       Efl_Bounce_Interpolator_Data *pd)
{
   return pd->rigidness;
}

/**
 * @brief Sets the number of bounces for the interpolator.
 *
 * @param eo_obj The Efl_Interpolator object.
 * @param pd The private data of the Efl_Bounce_Interpolator.
 * @param bounces The new number of bounces. For example, 3 bounces.
 */
EOLIAN static void
_efl_bounce_interpolator_bounces_set(Eo *eo_obj EINA_UNUSED,
                                     Efl_Bounce_Interpolator_Data *pd,
                                     int bounces)
{
   pd->bounces = bounces;
}

/**
 * @brief Gets the number of bounces for the interpolator.
 *
 * @param eo_obj The Efl_Interpolator object.
 * @param pd The private data of the Efl_Bounce_Interpolator.
 *
 * @return The current number of bounces.
 */
EOLIAN static int
_efl_bounce_interpolator_bounces_get(const Eo *eo_obj EINA_UNUSED,
                                     Efl_Bounce_Interpolator_Data *pd)
{
   return pd->bounces;
}

/**
 * @brief Constructor for the Efl_Bounce_Interpolator object.
 *
 * Initializes the bounce interpolator with default values for rigidness (1.0)
 * and number of bounces (1).
 *
 * @param eo_obj The Efl_Object to be constructed.
 * @param pd The private data of the Efl_Bounce_Interpolator.
 *
 * @return The constructed Efl_Object.
 */
EOLIAN static Efl_Object *
_efl_bounce_interpolator_efl_object_constructor(Eo *eo_obj,
                                                Efl_Bounce_Interpolator_Data *pd)
{
   eo_obj = efl_constructor(efl_super(eo_obj, MY_CLASS));

   pd->rigidness = 1.0;
   pd->bounces = 1;

   return eo_obj;
}

#include "efl_bounce_interpolator.eo.c"
