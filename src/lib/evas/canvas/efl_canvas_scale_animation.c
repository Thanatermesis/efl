#include "efl_canvas_scale_animation_private.h"

#define MY_CLASS EFL_CANVAS_SCALE_ANIMATION_CLASS

/**
 * @brief Sets the scale animation properties with a relative pivot point.
 *
 * This function defines a scale animation from a starting scale factor to an
 * ending scale factor, relative to a pivot object and a relative position
 * on that pivot object.
 *
 * @param[in] eo_obj The Eolian object.
 * @param[in,out] pd The private data of the Efl_Canvas_Scale_Animation object.
 * @param[in] from_scale The starting scale factor (e.g., EINA_VECTOR2(1.0, 1.0) for original size).
 * @param[in] to_scale The ending scale factor (e.g., EINA_VECTOR2(2.0, 2.0) for double size).
 * @param[in] pivot The Efl_Canvas_Object to use as the pivot. If NULL, the target object itself is used.
 * @param[in] rel_pivot_pos The relative position on the pivot object (e.g., EINA_VECTOR2(0.5, 0.5) for the center).
 */
EOLIAN static void
_efl_canvas_scale_animation_scale_set(Eo *eo_obj EINA_UNUSED,
                               Efl_Canvas_Scale_Animation_Data *pd,
                               Eina_Vector2 from_scale,
                               Eina_Vector2 to_scale,
                               Efl_Canvas_Object *pivot,
                               Eina_Vector2 rel_pivot_pos)
{
   pd->from = from_scale;

   pd->to = to_scale;

   pd->rel_pivot.obj = pivot;
   pd->rel_pivot.pos = rel_pivot_pos;

   pd->use_rel_pivot = EINA_TRUE;
}

/**
 * @brief Gets the scale animation properties with a relative pivot point.
 *
 * Retrieves the starting scale, ending scale, pivot object, and relative pivot position
 * if the animation was set up to use a relative pivot.
 *
 * @param[in] obj The Eolian object.
 * @param[in] pd The private data of the Efl_Canvas_Scale_Animation object.
 * @param[out] from_scale Pointer to store the starting scale factor.
 * @param[out] to_scale Pointer to store the ending scale factor.
 * @param[out] pivot Pointer to store the pivot Efl_Canvas_Object.
 * @param[out] pivot_pos Pointer to store the relative pivot position.
 */
EOLIAN static void
_efl_canvas_scale_animation_scale_get(const Eo *obj EINA_UNUSED,
                              Efl_Canvas_Scale_Animation_Data *pd,
                              Eina_Vector2 *from_scale, Eina_Vector2 *to_scale,
                              Efl_Canvas_Object **pivot, Eina_Vector2 *pivot_pos)
{
   if (!pd->use_rel_pivot)
     {
        ERR("Animation is done in absolute value.");
        return;
     }

   if (from_scale)
     *from_scale = pd->from;

   if (to_scale)
     *to_scale = pd->to;

   if (pivot)
     *pivot = pd->rel_pivot.obj;

   if (pivot_pos)
     *pivot_pos = pd->rel_pivot.pos;
}

/**
 * @brief Sets the scale animation properties with an absolute pivot point.
 *
 * This function defines a scale animation from a starting scale factor to an
 * ending scale factor, using an absolute coordinate as the pivot point.
 *
 * @param[in] obj The Eolian object.
 * @param[in,out] pd The private data of the Efl_Canvas_Scale_Animation object.
 * @param[in] from_scale The starting scale factor.
 * @param[in] to_scale The ending scale factor.
 * @param[in] pos The absolute pivot position (e.g., EINA_POSITION2D(100, 100)).
 */
EOLIAN static void
_efl_canvas_scale_animation_scale_absolute_set(Eo *obj EINA_UNUSED,
                            Efl_Canvas_Scale_Animation_Data *pd,
                            Eina_Vector2 from_scale, Eina_Vector2 to_scale,
                            Eina_Position2D pos)
{
   pd->from = from_scale;

   pd->to = to_scale;

   pd->pos = pos;

   pd->use_rel_pivot = EINA_FALSE;
}

/**
 * @brief Gets the scale animation properties with an absolute pivot point.
 *
 * Retrieves the starting scale, ending scale, and absolute pivot position
 * if the animation was set up to use an absolute pivot.
 *
 * @param[in] obj The Eolian object.
 * @param[in] pd The private data of the Efl_Canvas_Scale_Animation object.
 * @param[out] from_scale Pointer to store the starting scale factor.
 * @param[out] to_scale Pointer to store the ending scale factor.
 * @param[out] pos Pointer to store the absolute pivot position.
 */
EOLIAN static void
_efl_canvas_scale_animation_scale_absolute_get(const Eo *obj EINA_UNUSED,
                            Efl_Canvas_Scale_Animation_Data *pd,
                            Eina_Vector2 *from_scale, Eina_Vector2 *to_scale,
                            Eina_Position2D *pos)
{
   if (pd->use_rel_pivot)
     {
        ERR("Animation is done in relative value.");
        return;
     }

   if (from_scale)
     *from_scale = pd->from;

   if (to_scale)
     *to_scale = pd->to;

   if (pos)
     *pos = pd->pos;
}

/**
 * @brief Applies the scale animation to a target object at a given progress.
 *
 * This function calculates the intermediate scale based on the progress
 * and applies it to the target object using either relative or absolute pivot
 * zooming.
 *
 * @param[in] eo_obj The Eolian object.
 * @param[in] pd The private data of the Efl_Canvas_Scale_Animation object.
 * @param[in] progress The animation progress, typically from 0.0 (start) to 1.0 (end).
 * @param[in] target The Efl_Canvas_Object to apply the scale animation to.
 * @return The actual progress value after being processed by the parent animation class.
 */
EOLIAN static double
_efl_canvas_scale_animation_efl_canvas_animation_animation_apply(Eo *eo_obj,
                                                   Efl_Canvas_Scale_Animation_Data *pd,
                                                   double progress,
                                                   Efl_Canvas_Object *target)
{
   Eina_Vector2 new_scale;

   progress = efl_animation_apply(efl_super(eo_obj, MY_CLASS), progress, target);
   if (!target) return progress;

   new_scale.x = GET_STATUS(pd->from.x, pd->to.x, progress);
   new_scale.y = GET_STATUS(pd->from.y, pd->to.y, progress);

   if (pd->use_rel_pivot)
     {
        efl_gfx_mapping_zoom(target,
                             new_scale.x, new_scale.y,
                             (pd->rel_pivot.obj) ? pd->rel_pivot.obj : target,
                             pd->rel_pivot.pos.x , pd->rel_pivot.pos.y);
     }
   else
     {
        efl_gfx_mapping_zoom_absolute(target,
                                      new_scale.x, new_scale.y,
                                      pd->pos.x, pd->pos.y);
     }

   return progress;
}

/**
 * @brief Constructor for the Efl_Canvas_Scale_Animation object.
 *
 * Initializes the scale animation with default values.
 * Default: scale from (1.0, 1.0) to (1.0, 1.0) with a relative pivot
 * at the center (0.5, 0.5) of the target object itself.
 *
 * @param[in] eo_obj The Eolian object being constructed.
 * @param[in,out] pd The private data to initialize.
 * @return The constructed Eolian object.
 */
EOLIAN static Efl_Object *
_efl_canvas_scale_animation_efl_object_constructor(Eo *eo_obj,
                                            Efl_Canvas_Scale_Animation_Data *pd)
{
   eo_obj = efl_constructor(efl_super(eo_obj, MY_CLASS));

   pd->from = EINA_VECTOR2(1.0, 1.0);
   pd->to = EINA_VECTOR2(1.0, 1.0);
   pd->rel_pivot.pos = EINA_VECTOR2(0.5, 0.5);
   pd->rel_pivot.obj = NULL;
   pd->pos = EINA_POSITION2D(0, 0);

   pd->use_rel_pivot = EINA_TRUE;

   return eo_obj;
}

#include "efl_canvas_scale_animation.eo.c"
