#include "efl_canvas_alpha_animation_private.h"

#define MY_CLASS EFL_CANVAS_ALPHA_ANIMATION_CLASS

/**
 * @brief Sets the starting and ending alpha values for the animation.
 *
 * @param[in] eo_obj The Eolian object.
 * @param[in,out] pd The private data for the Efl_Canvas_Alpha_Animation.
 * @param[in] from_alpha The starting alpha value (0.0 to 1.0).
 * @param[in] to_alpha The ending alpha value (0.0 to 1.0).
 */
EOLIAN static void
_efl_canvas_alpha_animation_alpha_set(Eo *eo_obj EINA_UNUSED,
                               Efl_Canvas_Alpha_Animation_Data *pd,
                               double from_alpha,
                               double to_alpha)
{
   pd->from.alpha = from_alpha;
   pd->to.alpha = to_alpha;
}

/**
 * @brief Gets the starting and ending alpha values for the animation.
 *
 * @param[in] eo_obj The Eolian object.
 * @param[in] pd The private data for the Efl_Canvas_Alpha_Animation.
 * @param[out] from_alpha Pointer to store the starting alpha value. Can be NULL.
 * @param[out] to_alpha Pointer to store the ending alpha value. Can be NULL.
 */
EOLIAN static void
_efl_canvas_alpha_animation_alpha_get(const Eo *eo_obj EINA_UNUSED,
                               Efl_Canvas_Alpha_Animation_Data *pd,
                               double *from_alpha,
                               double *to_alpha)
{
   if (from_alpha)
     *from_alpha = pd->from.alpha;
   if (to_alpha)
     *to_alpha = pd->to.alpha;
}

/**
 * @brief Applies the alpha animation to the target Efl_Canvas_Object.
 *
 * This function calculates the current alpha based on the progress
 * and applies it to all four color points of the target object's mapping.
 *
 * @param[in] eo_obj The Eolian object.
 * @param[in] pd The private data for the Efl_Canvas_Alpha_Animation.
 * @param[in] progress The animation progress (0.0 to 1.0).
 * @param[in,out] target The Efl_Canvas_Object to apply the animation to.
 * @return The actual progress value after applying parent's animation logic.
 */
EOLIAN static double
_efl_canvas_alpha_animation_efl_canvas_animation_animation_apply(Eo *eo_obj,
                               Efl_Canvas_Alpha_Animation_Data *pd EINA_UNUSED,
                               double progress,
                               Efl_Canvas_Object *target)
{
   double from_alpha, to_alpha;
   int cur_alpha;
   int i;

   progress = efl_animation_apply(efl_super(eo_obj, MY_CLASS), progress, target);
   if (!target) return progress;

   efl_animation_alpha_get(eo_obj, &from_alpha, &to_alpha);
   cur_alpha = (int)(GET_STATUS(from_alpha, to_alpha, progress) * 255);

   for (i = 0; i < 4; i++)
     {
        efl_gfx_mapping_color_set(target, i, cur_alpha, cur_alpha, cur_alpha, cur_alpha);
     }

   return progress;
}

/**
 * @brief Constructor for the Efl_Canvas_Alpha_Animation object.
 *
 * Initializes the animation with default alpha values (from 1.0 to 1.0).
 *
 * @param[in] eo_obj The Eolian object to construct.
 * @param[in,out] pd The private data for the Efl_Canvas_Alpha_Animation.
 * @return The constructed Eolian object.
 */
EOLIAN static Efl_Object *
_efl_canvas_alpha_animation_efl_object_constructor(Eo *eo_obj,
                                            Efl_Canvas_Alpha_Animation_Data *pd)
{
   eo_obj = efl_constructor(efl_super(eo_obj, MY_CLASS));

   pd->from.alpha = 1.0;
   pd->to.alpha = 1.0;

   return eo_obj;
}

#include "efl_canvas_alpha_animation.eo.c"
