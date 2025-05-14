#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "Ecore.h"
#include "ecore_private.h"

#define MY_CLASS EFL_ACCELERATE_INTERPOLATOR_CLASS

typedef struct _Efl_Accelerate_Interpolator_Data Efl_Accelerate_Interpolator_Data;

/**
 * @brief Private data for the Efl_Accelerate_Interpolator class.
 */
struct _Efl_Accelerate_Interpolator_Data
{
   double slope; /**< The factor of acceleration. A value of 1.0 means no acceleration (linear). */
};

/**
 * @brief Interpolates a value based on the progress using an accelerate curve.
 *
 * @param eo_obj The Eolian object.
 * @param pd The private data structure.
 * @param progress The input progress value, expected to be between 0.0 and 1.0.
 *                 Values outside this range are returned unchanged.
 * @return The interpolated value.
 */
EOLIAN static double
_efl_accelerate_interpolator_efl_interpolator_interpolate(Eo *eo_obj EINA_UNUSED,
                                                          Efl_Accelerate_Interpolator_Data *pd EINA_UNUSED,
                                                          double progress)
{
   if ((progress < 0.0) || (progress > 1.0))
     return progress;

   return ecore_animator_pos_map(progress, ECORE_POS_MAP_ACCELERATE_FACTOR,
                                 pd->slope, 0);
}

/**
 * @brief Sets the slope (factor) for the accelerate interpolator.
 *
 * @param eo_obj The Eolian object.
 * @param pd The private data structure.
 * @param slope The new slope value. For example, a value of 2.0 would mean a
 *              quadratic acceleration. A value of 1.0 results in linear interpolation.
 */
EOLIAN static void
_efl_accelerate_interpolator_slope_set(Eo *eo_obj EINA_UNUSED,
                                        Efl_Accelerate_Interpolator_Data *pd,
                                        double slope)
{
   pd->slope = slope;
}

/**
 * @brief Gets the slope (factor) for the accelerate interpolator.
 *
 * @param eo_obj The Eolian object.
 * @param pd The private data structure.
 * @return The current slope value.
 */
EOLIAN static double
_efl_accelerate_interpolator_slope_get(const Eo *eo_obj EINA_UNUSED,
                                        Efl_Accelerate_Interpolator_Data *pd EINA_UNUSED)
{
   return pd->slope;
}

/**
 * @brief Constructor for the Efl_Accelerate_Interpolator class.
 *
 * Initializes the object and sets the default slope to 1.0.
 *
 * @param eo_obj The Eolian object to construct.
 * @param pd The private data structure.
 * @return The constructed Eolian object.
 */
EOLIAN static Efl_Object *
_efl_accelerate_interpolator_efl_object_constructor(Eo *eo_obj,
                                                    Efl_Accelerate_Interpolator_Data *pd)
{
   eo_obj = efl_constructor(efl_super(eo_obj, MY_CLASS));

   pd->slope = 1.0;

   return eo_obj;
}

#include "efl_accelerate_interpolator.eo.c"
