#include "efl_canvas_rotate_animation_private.h"

#define MY_CLASS EFL_CANVAS_ROTATE_ANIMATION_CLASS

/**
 * @brief Sets the rotation animation properties relative to a pivot object.
 *
 * This function defines a rotation animation from a starting degree to an ending degree.
 * The rotation is performed around a specified point on a pivot object.
 * If the pivot object is NULL, the rotation is relative to the target object itself.
 *
 * @param[in] eo_obj The Eolian object.
 * @param[out] pd The private data for the Efl_Canvas_Rotate_Animation.
 * @param[in] from_degree The starting angle of the rotation in degrees.
 * @param[in] to_degree The ending angle of the rotation in degrees.
 * @param[in] pivot The Efl_Canvas_Object to use as the pivot. If NULL, the target object is used.
 * @param[in] center_point The point on the pivot object (or target if pivot is NULL)
 *                         around which to rotate. Values are normalized (0.0 to 1.0).
 *                         For example, {0.5, 0.5} is the center of the pivot object.
 */
EOLIAN static void
_efl_canvas_rotate_animation_rotate_set(Eo *eo_obj EINA_UNUSED,
                                 Efl_Canvas_Rotate_Animation_Data *pd,
                                 double from_degree,
                                 double to_degree,
                                 Efl_Canvas_Object *pivot,
                                 Eina_Vector2 center_point)
{
   pd->from.degree = from_degree;
   pd->to.degree = to_degree;

   //TODO: check whether ref for pivot should be added.
   pd->rel_pivot.obj = pivot;
   pd->rel_pivot.pos = center_point;
   pd->use_rel_pivot = EINA_TRUE;
}

/**
 * @brief Gets the rotation animation properties relative to a pivot object.
 *
 * Retrieves the parameters previously set by _efl_canvas_rotate_animation_rotate_set.
 * If the animation was set using absolute coordinates, this function will report an error
 * and return without populating the output parameters.
 *
 * @param[in] eo_obj The Eolian object.
 * @param[in] pd The private data for the Efl_Canvas_Rotate_Animation.
 * @param[out] from_degree Pointer to store the starting angle of the rotation. Can be NULL.
 * @param[out] to_degree Pointer to store the ending angle of the rotation. Can be NULL.
 * @param[out] pivot Pointer to store the pivot Efl_Canvas_Object. Can be NULL.
 * @param[out] center_point Pointer to store the pivot point. Can be NULL.
 */
EOLIAN static void
_efl_canvas_rotate_animation_rotate_get(const Eo *eo_obj EINA_UNUSED,
                                 Efl_Canvas_Rotate_Animation_Data *pd,
                                 double *from_degree,
                                 double *to_degree,
                                 Efl_Canvas_Object **pivot,
                                 Eina_Vector2 *center_point)
{
   if (!pd->use_rel_pivot)
     {
        ERR("Animation is done in absolute value.");
        return;
     }

   if (from_degree)
     *from_degree = pd->from.degree;

   if (to_degree)
     *to_degree = pd->to.degree;

   if (pivot)
     *pivot = pd->rel_pivot.obj;

   if (center_point)
     *center_point = pd->rel_pivot.pos;
}

/**
 * @brief Sets the rotation animation properties using absolute coordinates for the pivot.
 *
 * This function defines a rotation animation from a starting degree to an ending degree.
 * The rotation is performed around a specified absolute point on the canvas.
 *
 * @param[in] eo_obj The Eolian object.
 * @param[out] pd The private data for the Efl_Canvas_Rotate_Animation.
 * @param[in] from_degree The starting angle of the rotation in degrees.
 * @param[in] to_degree The ending angle of the rotation in degrees.
 * @param[in] abs The absolute (x, y) coordinates on the canvas to use as the pivot point.
 *                For example, {100, 150} means the pivot is at x=100, y=150.
 */
EOLIAN static void
_efl_canvas_rotate_animation_rotate_absolute_set(Eo *eo_obj EINA_UNUSED,
                                          Efl_Canvas_Rotate_Animation_Data *pd,
                                          double from_degree,
                                          double to_degree,
                                          Eina_Position2D abs)
{
   pd->from.degree = from_degree;
   pd->to.degree = to_degree;

   pd->abs_pivot = abs;
   pd->use_rel_pivot = EINA_FALSE;
}

/**
 * @brief Gets the rotation animation properties using absolute coordinates for the pivot.
 *
 * Retrieves the parameters previously set by _efl_canvas_rotate_animation_rotate_absolute_set.
 * If the animation was set using a relative pivot object, this function will report an error
 * and return without populating the output parameters.
 *
 * @param[in] eo_obj The Eolian object.
 * @param[in] pd The private data for the Efl_Canvas_Rotate_Animation.
 * @param[out] from_degree Pointer to store the starting angle of the rotation. Can be NULL.
 * @param[out] to_degree Pointer to store the ending angle of the rotation. Can be NULL.
 * @param[out] abs Pointer to store the absolute pivot coordinates. Can be NULL.
 */
EOLIAN static void
_efl_canvas_rotate_animation_rotate_absolute_get(const Eo *eo_obj EINA_UNUSED,
                                          Efl_Canvas_Rotate_Animation_Data *pd,
                                          double *from_degree,
                                          double *to_degree,
                                          Eina_Position2D *abs)
{
   if (pd->use_rel_pivot)
     {
        ERR("Animation is done in relative value.");
        return;
     }

   if (from_degree)
     *from_degree = pd->from.degree;

   if (to_degree)
     *to_degree = pd->to.degree;

   if (abs)
     *abs = pd->abs_pivot;
}

/**
 * @brief Applies the rotation animation to a target object based on the progress.
 *
 * This function is called by the animation system to update the target object's
 * rotation at a given progress point (0.0 to 1.0) of the animation.
 * It calculates the interpolated degree of rotation and applies it to the target
 * object using either relative or absolute pivot mapping.
 *
 * @param[in] eo_obj The Eolian object representing this animation.
 * @param[in] pd The private data for the Efl_Canvas_Rotate_Animation.
 * @param[in] progress The current progress of the animation, typically from 0.0 (start) to 1.0 (end).
 * @param[in] target The Efl_Canvas_Object to which the rotation should be applied.
 * @return The actual progress value after applying the animation. This might be
 *         modified by the parent class's animation_apply method.
 */
EOLIAN static double
_efl_canvas_rotate_animation_efl_canvas_animation_animation_apply(Eo *eo_obj,
                                                    Efl_Canvas_Rotate_Animation_Data *pd,
                                                    double progress,
                                                    Efl_Canvas_Object *target)
{
   double new_degree;

   progress = efl_animation_apply(efl_super(eo_obj, MY_CLASS), progress, target);
   if (!target) return progress;

   new_degree = GET_STATUS(pd->from.degree, pd->to.degree, progress);

   if (pd->use_rel_pivot)
     {
        efl_gfx_mapping_rotate(target,
                               new_degree,
                               (pd->rel_pivot.obj) ? pd->rel_pivot.obj : target,
                               pd->rel_pivot.pos.x, pd->rel_pivot.pos.y);
     }
   else
     {
        efl_gfx_mapping_rotate_absolute(target,
                                        new_degree,
                                        pd->abs_pivot.x, pd->abs_pivot.y);
     }

   return progress;
}

/**
 * @brief Constructor for the Efl_Canvas_Rotate_Animation object.
 *
 * Initializes the animation object and its private data with default values.
 * By default, the animation is configured for a relative pivot, centered on the
 * target object, with no rotation (from 0.0 to 0.0 degrees).
 *
 * @param[in] eo_obj The Eolian object to construct.
 * @param[out] pd The private data to initialize.
 * @return The constructed Eolian object.
 */
EOLIAN static Efl_Object *
_efl_canvas_rotate_animation_efl_object_constructor(Eo *eo_obj,
                                             Efl_Canvas_Rotate_Animation_Data *pd)
{
   eo_obj = efl_constructor(efl_super(eo_obj, MY_CLASS));

   pd->from.degree = 0.0;
   pd->to.degree = 0.0;

   pd->rel_pivot.obj = NULL;
   pd->rel_pivot.pos.x = 0.5;
   pd->rel_pivot.pos.y = 0.5;

   pd->abs_pivot.x = 0;
   pd->abs_pivot.y = 0;

   pd->use_rel_pivot = EINA_TRUE;

   return eo_obj;
}

#include "efl_canvas_rotate_animation.eo.c"
