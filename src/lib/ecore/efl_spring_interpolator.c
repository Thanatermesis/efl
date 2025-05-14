#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "Ecore.h"
#include "ecore_private.h"

#define MY_CLASS EFL_SPRING_INTERPOLATOR_CLASS

/**
 * @brief Private data for the Efl.Spring_Interpolator class.
 */
typedef struct _Efl_Spring_Interpolator_Data Efl_Spring_Interpolator_Data;

/**
 * @brief Private data for the Efl.Spring_Interpolator class.
 *
 * This structure holds the configuration for the spring interpolator,
 * specifically its decay rate and the number of oscillations.
 */
struct _Efl_Spring_Interpolator_Data
{
   double decay;        /**< The decay factor for the spring. Determines how quickly the spring settles. A higher value means faster settling. Example: 1.0 */
   int oscillations;    /**< The number of oscillations the spring will make before settling. Example: 1 */
};

/**
 * @brief Interpolates a value using a spring model.
 *
 * This function implements the Efl.Interpolator.interpolate method.
 * It uses the ecore_animator_pos_map function with ECORE_POS_MAP_SPRING
 * to calculate the interpolated value based on the spring's decay and
 * oscillations.
 *
 * @param obj The Efl.Spring_Interpolator object.
 * @param pd The private data of the Efl.Spring_Interpolator object.
 * @param progress The input progress value, typically between 0.0 and 1.0.
 *                 Values outside this range are returned unchanged.
 * @return The interpolated value. If progress is outside [0.0, 1.0],
 *         progress itself is returned. Otherwise, a value calculated
 *         by the spring easing function.
 */
EOLIAN static double
_efl_spring_interpolator_efl_interpolator_interpolate(Eo *obj EINA_UNUSED,
                                                      Efl_Spring_Interpolator_Data *pd,
                                                      double progress)
{
   if ((progress < 0.0) || (progress > 1.0))
     return progress;

   return ecore_animator_pos_map(progress, ECORE_POS_MAP_SPRING,
                                 pd->decay, (double)pd->oscillations);
}

/**
 * @brief Sets the decay factor for the spring interpolator.
 *
 * The decay factor determines how quickly the spring oscillation diminishes.
 *
 * @param eo_obj The Efl.Spring_Interpolator object.
 * @param pd The private data of the Efl.Spring_Interpolator object.
 * @param decay The new decay factor. For example, 1.0.
 */
EOLIAN static void
_efl_spring_interpolator_decay_set(Eo *eo_obj EINA_UNUSED,
                                   Efl_Spring_Interpolator_Data *pd,
                                   double decay)
{
   pd->decay = decay;
}

/**
 * @brief Gets the decay factor of the spring interpolator.
 *
 * @param eo_obj The Efl.Spring_Interpolator object.
 * @param pd The private data of the Efl.Spring_Interpolator object.
 * @return The current decay factor.
 */
EOLIAN static double
_efl_spring_interpolator_decay_get(const Eo *eo_obj EINA_UNUSED,
                                     Efl_Spring_Interpolator_Data *pd)
{
   return pd->decay;
}

/**
 * @brief Sets the number of oscillations for the spring interpolator.
 *
 * This determines how many times the spring will bounce before settling.
 *
 * @param eo_obj The Efl.Spring_Interpolator object.
 * @param pd The private data of the Efl.Spring_Interpolator object.
 * @param oscillations The new number of oscillations. For example, 1.
 */
EOLIAN static void
_efl_spring_interpolator_oscillations_set(Eo *eo_obj EINA_UNUSED,
                                          Efl_Spring_Interpolator_Data *pd,
                                          int oscillations)
{
   pd->oscillations = oscillations;
}

/**
 * @brief Gets the number of oscillations of the spring interpolator.
 *
 * @param eo_obj The Efl.Spring_Interpolator object.
 * @param pd The private data of the Efl.Spring_Interpolator object.
 * @return The current number of oscillations.
 */
EOLIAN static int
_efl_spring_interpolator_oscillations_get(const Eo *eo_obj EINA_UNUSED,
                                          Efl_Spring_Interpolator_Data *pd)
{
   return pd->oscillations;
}

/**
 * @brief Constructor for the Efl.Spring_Interpolator object.
 *
 * Initializes the spring interpolator with default values for decay (1.0)
 * and oscillations (1).
 *
 * @param eo_obj The Efl.Spring_Interpolator object being constructed.
 * @param pd The private data of the Efl.Spring_Interpolator object.
 * @return The constructed Efl.Object.
 */
EOLIAN static Efl_Object *
_efl_spring_interpolator_efl_object_constructor(Eo *eo_obj,
                                                Efl_Spring_Interpolator_Data *pd)
{
   eo_obj = efl_constructor(efl_super(eo_obj, MY_CLASS));

   pd->decay = 1.0;
   pd->oscillations = 1;

   return eo_obj;
}

#include "efl_spring_interpolator.eo.c"
