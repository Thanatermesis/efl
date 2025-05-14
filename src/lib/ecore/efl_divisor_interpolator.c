#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "Ecore.h"
#include "ecore_private.h"

#define MY_CLASS EFL_DIVISOR_INTERPOLATOR_CLASS

/**
 * @brief Private data for the Efl_Divisor_Interpolator class.
 */
typedef struct _Efl_Divisor_Interpolator_Data Efl_Divisor_Interpolator_Data;

struct _Efl_Divisor_Interpolator_Data
{
   double divisor; /**< The divisor factor for the interpolation. Determines the steepness of the curve. Default is 1.0. */
   int power;      /**< The power factor for the interpolation. Determines the curvature. Default is 1. */
};

/**
 * @brief Interpolates a value based on the divisor and power parameters.
 *
 * This function implements the Efl.Interpolator.interpolate interface.
 * It uses the ecore_animator_pos_map function with ECORE_POS_MAP_DIVISOR_INTERP
 * to calculate the interpolated value.
 *
 * @param[in] eo_obj The Eolian object.
 * @param[in] pd The private data structure.
 * @param[in] progress The input progress value, typically between 0.0 and 1.0.
 * @return The interpolated progress value. If the input progress is outside the
 *         0.0 to 1.0 range, it is returned unchanged.
 */
EOLIAN static double
_efl_divisor_interpolator_efl_interpolator_interpolate(Eo *eo_obj EINA_UNUSED,
                                                       Efl_Divisor_Interpolator_Data *pd EINA_UNUSED,
                                                       double progress)
{
   if ((progress < 0.0) || (progress > 1.0))
     return progress;

   return ecore_animator_pos_map(progress, ECORE_POS_MAP_DIVISOR_INTERP,
                                 pd->divisor, (double)pd->power);
}

/**
 * @brief Sets the divisor for the interpolation.
 *
 * @param[in] eo_obj The Eolian object.
 * @param[in,out] pd The private data structure.
 * @param[in] divisor The new divisor value. For example, 2.0.
 */
EOLIAN static void
_efl_divisor_interpolator_divisor_set(Eo *eo_obj EINA_UNUSED,
                                      Efl_Divisor_Interpolator_Data *pd,
                                      double divisor)
{
   pd->divisor = divisor;
}

/**
 * @brief Gets the current divisor for the interpolation.
 *
 * @param[in] eo_obj The Eolian object.
 * @param[in] pd The private data structure.
 * @return The current divisor value.
 */
EOLIAN static double
_efl_divisor_interpolator_divisor_get(const Eo *eo_obj EINA_UNUSED,
                                      Efl_Divisor_Interpolator_Data *pd)
{
   return pd->divisor;
}

/**
 * @brief Sets the power for the interpolation.
 *
 * @param[in] eo_obj The Eolian object.
 * @param[in,out] pd The private data structure.
 * @param[in] power The new power value. For example, 2.
 */
EOLIAN static void
_efl_divisor_interpolator_power_set(Eo *eo_obj EINA_UNUSED,
                                    Efl_Divisor_Interpolator_Data *pd,
                                    int power)
{
   pd->power = power;
}

/**
 * @brief Gets the current power for the interpolation.
 *
 * @param[in] eo_obj The Eolian object.
 * @param[in] pd The private data structure.
 * @return The current power value.
 */
EOLIAN static int
_efl_divisor_interpolator_power_get(const Eo *eo_obj EINA_UNUSED,
                                    Efl_Divisor_Interpolator_Data *pd)
{
   return pd->power;
}

/**
 * @brief Constructor for the Efl_Divisor_Interpolator object.
 *
 * Initializes the object and sets default values for divisor (1.0) and power (1).
 *
 * @param[in] eo_obj The Eolian object to construct.
 * @param[in,out] pd The private data structure.
 * @return The constructed Eolian object.
 */
EOLIAN static Efl_Object *
_efl_divisor_interpolator_efl_object_constructor(Eo *eo_obj,
                                                 Efl_Divisor_Interpolator_Data *pd)
{
   eo_obj = efl_constructor(efl_super(eo_obj, MY_CLASS));

   pd->divisor = 1.0;
   pd->power = 1;

   return eo_obj;
}

#include "efl_divisor_interpolator.eo.c"
