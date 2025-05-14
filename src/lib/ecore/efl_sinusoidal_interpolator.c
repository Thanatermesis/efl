#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "Ecore.h"
#include "ecore_private.h"

#define MY_CLASS EFL_SINUSOIDAL_INTERPOLATOR_CLASS

/**
 * @brief Private data for the Efl_Sinusoidal_Interpolator class.
 */
typedef struct _Efl_Sinusoidal_Interpolator_Data Efl_Sinusoidal_Interpolator_Data;

/**
 * @brief Structure holding the private data for the Efl_Sinusoidal_Interpolator.
 */
struct _Efl_Sinusoidal_Interpolator_Data
{
   double slope; /**< The slope parameter for the sinusoidal interpolation. Affects the steepness of the curve. Default is 1.0. */
};

/**
 * @brief Interpolates a value using a sinusoidal curve.
 *
 * This function applies a sinusoidal easing to the input progress.
 * The interpolation is defined by ECORE_POS_MAP_SINUSOIDAL_FACTOR.
 *
 * @param[in] eo_obj The Efl_Interpolator object.
 * @param[in] pd The private data of the Efl_Sinusoidal_Interpolator.
 * @param[in] progress The input progress value, expected to be between 0.0 and 1.0.
 *                     Values outside this range are returned unchanged.
 * @return The interpolated value. If progress is outside [0.0, 1.0], progress is returned as is.
 *         Otherwise, the result of ecore_animator_pos_map with ECORE_POS_MAP_SINUSOIDAL_FACTOR.
 */
EOLIAN static double
_efl_sinusoidal_interpolator_efl_interpolator_interpolate(Eo *eo_obj EINA_UNUSED,
                                                          Efl_Sinusoidal_Interpolator_Data *pd EINA_UNUSED,
                                                          double progress)
{
   if ((progress < 0.0) || (progress > 1.0))
     return progress;

   return ecore_animator_pos_map(progress, ECORE_POS_MAP_SINUSOIDAL_FACTOR,
                                 pd->slope, 0);
}

/**
 * @brief Sets the slope for the sinusoidal interpolator.
 *
 * The slope parameter influences the shape of the sinusoidal curve.
 *
 * @param[in] eo_obj The Efl_Interpolator object.
 * @param[out] pd The private data of the Efl_Sinusoidal_Interpolator.
 * @param[in] slope The new slope value. For example, 0.5 for a gentler curve, 2.0 for a steeper one.
 */
EOLIAN static void
_efl_sinusoidal_interpolator_slope_set(Eo *eo_obj EINA_UNUSED,
                                        Efl_Sinusoidal_Interpolator_Data *pd,
                                        double slope)
{
   pd->slope = slope;
}

/**
 * @brief Gets the slope for the sinusoidal interpolator.
 *
 * @param[in] eo_obj The Efl_Interpolator object.
 * @param[in] pd The private data of the Efl_Sinusoidal_Interpolator.
 * @return The current slope value.
 */
EOLIAN static double
_efl_sinusoidal_interpolator_slope_get(const Eo *eo_obj EINA_UNUSED,
                                        Efl_Sinusoidal_Interpolator_Data *pd EINA_UNUSED)
{
   return pd->slope;
}

/**
 * @brief Constructor for the Efl_Sinusoidal_Interpolator.
 *
 * Initializes the object and sets the default slope to 1.0.
 *
 * @param[in] eo_obj The Efl_Object to be constructed.
 * @param[out] pd The private data of the Efl_Sinusoidal_Interpolator.
 * @return The constructed Efl_Object.
 */
EOLIAN static Efl_Object *
_efl_sinusoidal_interpolator_efl_object_constructor(Eo *eo_obj,
                                                    Efl_Sinusoidal_Interpolator_Data *pd)
{
   eo_obj = efl_constructor(efl_super(eo_obj, MY_CLASS));

   // Default slope value for sinusoidal interpolation.
   pd->slope = 1.0;

   return eo_obj;
}

#include "efl_sinusoidal_interpolator.eo.c"
