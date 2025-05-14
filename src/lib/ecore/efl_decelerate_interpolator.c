#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "Ecore.h"
#include "ecore_private.h"

#define MY_CLASS EFL_DECELERATE_INTERPOLATOR_CLASS

typedef struct _Efl_Decelerate_Interpolator_Data Efl_Decelerate_Interpolator_Data;

/**
 * @brief Private data for the Efl_Decelerate_Interpolator class.
 */
struct _Efl_Decelerate_Interpolator_Data
{
   double slope; /**< The slope factor for the deceleration. Determines how sharply the interpolation decelerates. A higher value means a sharper deceleration. */
};

/**
 * @brief Interpolates a value based on the progress using a decelerate curve.
 *
 * This function implements the Efl.Interpolator.interpolate method.
 * It uses a deceleration curve, meaning the rate of change decreases as progress
 * approaches 1.0. The exact shape of the curve is influenced by the 'slope'
 * property.
 *
 * @param[in] eo_obj The Efl_Decelerate_Interpolator object.
 * @param[in] pd The private data for the Efl_Decelerate_Interpolator.
 * @param[in] progress The input progress value, typically between 0.0 (start) and 1.0 (end).
 *                     Values outside this range are returned as is.
 * @return The interpolated value. This will also be between 0.0 and 1.0 if the
 *         input progress is within that range.
 */
EOLIAN static double
_efl_decelerate_interpolator_efl_interpolator_interpolate(Eo *eo_obj EINA_UNUSED,
                                                          Efl_Decelerate_Interpolator_Data *pd EINA_UNUSED,
                                                          double progress)
{
   if ((progress < 0.0) || (progress > 1.0))
     return progress;

   return ecore_animator_pos_map(progress, ECORE_POS_MAP_DECELERATE_FACTOR,
                                 pd->slope, 0);
}

/**
 * @brief Sets the slope factor for the deceleration curve.
 *
 * The slope factor affects how sharply the interpolation decelerates.
 * A higher value results in a more pronounced deceleration towards the end
 * of the interpolation.
 *
 * @param[in] eo_obj The Efl_Decelerate_Interpolator object.
 * @param[in,out] pd The private data for the Efl_Decelerate_Interpolator.
 * @param[in] slope The new slope factor. For example, 1.0 is a common default,
 *                  2.0 would be a sharper deceleration.
 */
EOLIAN static void
_efl_decelerate_interpolator_slope_set(Eo *eo_obj EINA_UNUSED,
                                        Efl_Decelerate_Interpolator_Data *pd,
                                        double slope)
{
   pd->slope = slope;
}

/**
 * @brief Gets the slope factor for the deceleration curve.
 *
 * @param[in] eo_obj The Efl_Decelerate_Interpolator object.
 * @param[in] pd The private data for the Efl_Decelerate_Interpolator.
 * @return The current slope factor.
 */
EOLIAN static double
_efl_decelerate_interpolator_slope_get(const Eo *eo_obj EINA_UNUSED,
                                        Efl_Decelerate_Interpolator_Data *pd EINA_UNUSED)
{
   return pd->slope;
}

/**
 * @brief Constructor for the Efl_Decelerate_Interpolator object.
 *
 * Initializes the object and sets the default slope to 1.0.
 *
 * @param[in] eo_obj The Efl_Decelerate_Interpolator object being constructed.
 * @param[in,out] pd The private data for the Efl_Decelerate_Interpolator.
 * @return The constructed Efl_Object.
 */
EOLIAN static Efl_Object *
_efl_decelerate_interpolator_efl_object_constructor(Eo *eo_obj,
                                                    Efl_Decelerate_Interpolator_Data *pd)
{
   eo_obj = efl_constructor(efl_super(eo_obj, MY_CLASS));

   pd->slope = 1.0;

   return eo_obj;
}

#include "efl_decelerate_interpolator.eo.c"
