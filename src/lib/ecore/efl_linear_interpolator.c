/**
 * @file
 * @brief This file implements the Efl.Linear_Interpolator class.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "Ecore.h"
#include "ecore_private.h"

#define MY_CLASS EFL_LINEAR_INTERPOLATOR_CLASS

typedef struct _Efl_Linear_Interpolator_Data Efl_Linear_Interpolator_Data;

/**
 * @brief Private data for the Efl.Linear_Interpolator class.
 *
 * This structure holds any private data specific to instances of the
 * Efl_Linear_Interpolator class. In this case, it's empty as linear
 * interpolation does not require any state to be stored.
 */
struct _Efl_Linear_Interpolator_Data
{
};

EOLIAN static double
_efl_linear_interpolator_efl_interpolator_interpolate(Eo *eo_obj EINA_UNUSED,
                                                      Efl_Linear_Interpolator_Data *pd EINA_UNUSED,
                                                      double progress)
/**
 * @brief Interpolates a value using a linear mapping.
 *
 * This function implements the Efl.Interpolator.interpolate method for a
 * linear interpolator. It maps the input @p progress value (typically
 * between 0.0 and 1.0) to an output value using a linear function.
 *
 * @param[in] eo_obj The Eolian object.
 * @param[in] pd The private data for the Efl_Linear_Interpolator instance.
 * @param[in] progress The input progress value, expected to be in the
 *                     range [0.0, 1.0]. Values outside this range will
 *                     be returned clamped if ECORE_POS_MAP_CLAMP is used
 *                     by ecore_animator_pos_map, or as-is otherwise.
 *                     For ECORE_POS_MAP_LINEAR, values outside [0.0, 1.0]
 *                     are returned as-is.
 *
 * @return The interpolated value. For linear interpolation, this is
 *         typically the same as the input @p progress value when
 *         @p progress is within the [0.0, 1.0] range.
 */
{
   if ((progress < 0.0) || (progress > 1.0))
     return progress;

   return ecore_animator_pos_map(progress, ECORE_POS_MAP_LINEAR, 0, 0);
}

#include "efl_linear_interpolator.eo.c"
