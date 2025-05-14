#include "efl_canvas_translate_animation_private.h"

#define MY_CLASS EFL_CANVAS_TRANSLATE_ANIMATION_CLASS

/**
 * @brief Structure to hold double precision x and y coordinates for translation.
 * This is used internally to calculate intermediate translation values.
 */
typedef struct __Translate_Property_Double
{
   double x; /**< The x-coordinate. */
   double y; /**< The y-coordinate. */
} _Translate_Property_Double;

/**
 * @brief Sets the starting and ending positions for a relative translation animation.
 *
 * The animation will translate the object *by* the specified amounts,
 * relative to its current position at the start of the animation.
 *
 * @param[in] eo_obj The Eolian object.
 * @param[in,out] pd The private data for the Efl_Canvas_Translate_Animation.
 * @param[in] from The starting relative offset for the translation (e.g., EINA_POSITION2D(0, 0)).
 * @param[in] to The ending relative offset for the translation (e.g., EINA_POSITION2D(100, 50) to move 100px right and 50px down).
 */
EOLIAN static void
_efl_canvas_translate_animation_translate_set(Eo *eo_obj EINA_UNUSED,
                                       Efl_Canvas_Translate_Animation_Data *pd,
                                       Eina_Position2D from,
                                       Eina_Position2D to)
{
   pd->from = from;

   pd->to = to;

   pd->use_rel_move = EINA_TRUE;
}

/**
 * @brief Gets the starting and ending positions for a relative translation animation.
 *
 * If the animation was set up for absolute translation, this function will log an error
 * and not return the values.
 *
 * @param[in] eo_obj The Eolian object.
 * @param[in] pd The private data for the Efl_Canvas_Translate_Animation.
 * @param[out] from Pointer to store the starting relative offset. Can be NULL.
 * @param[out] to Pointer to store the ending relative offset. Can be NULL.
 */
EOLIAN static void
_efl_canvas_translate_animation_translate_get(const Eo *eo_obj EINA_UNUSED,
                                       Efl_Canvas_Translate_Animation_Data *pd,
                                       Eina_Position2D *from,
                                       Eina_Position2D *to)
{
   if (!pd->use_rel_move)
     {
        ERR("Animation is done in absolute value.");
        return;
     }

   if (from)
     *from = pd->from;

   if (to)
     *to = pd->to;
}

/**
 * @brief Sets the starting and ending absolute positions for a translation animation.
 *
 * The animation will translate the object *to* the specified screen coordinates.
 *
 * @param[in] eo_obj The Eolian object.
 * @param[in,out] pd The private data for the Efl_Canvas_Translate_Animation.
 * @param[in] from The starting absolute screen position for the translation (e.g., EINA_POSITION2D(10, 20)).
 * @param[in] to The ending absolute screen position for the translation (e.g., EINA_POSITION2D(110, 70)).
 */
EOLIAN static void
_efl_canvas_translate_animation_translate_absolute_set(Eo *eo_obj EINA_UNUSED,
                                                Efl_Canvas_Translate_Animation_Data *pd,
                                                Eina_Position2D from,
                                                Eina_Position2D to)
{
   pd->from = from;

   pd->to = to;

   pd->use_rel_move = EINA_FALSE;
}

/**
 * @brief Gets the starting and ending absolute positions for a translation animation.
 *
 * If the animation was set up for relative translation, this function will log an error
 * and not return the values.
 *
 * @param[in] eo_obj The Eolian object.
 * @param[in] pd The private data for the Efl_Canvas_Translate_Animation.
 * @param[out] from Pointer to store the starting absolute screen position. Can be NULL.
 * @param[out] to Pointer to store the ending absolute screen position. Can be NULL.
 */
EOLIAN static void
_efl_canvas_translate_animation_translate_absolute_get(const Eo *eo_obj EINA_UNUSED,
                                                Efl_Canvas_Translate_Animation_Data *pd,
                                                Eina_Position2D *from,
                                                Eina_Position2D *to)
{
   if (pd->use_rel_move)
     {
        ERR("Animation is done in absolute value.");
        return;
     }

   if (from)
     *from = pd->from;

   if (to)
     *to = pd->to;
}

/**
 * @brief Applies the translation animation to the target object based on the progress.
 *
 * This function calculates the new position of the target object based on the
 * animation's progress and whether it's a relative or absolute translation.
 * It then applies this translation using efl_gfx_mapping_translate.
 *
 * @param[in] eo_obj The Eolian object representing this animation.
 * @param[in] pd The private data for the Efl_Canvas_Translate_Animation.
 * @param[in] progress The animation progress, typically a value between 0.0 (start) and 1.0 (end).
 * @param[in,out] target The canvas object to which the animation is applied.
 * @return The actual progress value after applying the parent class's animation logic.
 *         This might be clamped or modified by the parent.
 */
EOLIAN static double
_efl_canvas_translate_animation_efl_canvas_animation_animation_apply(Eo *eo_obj,
                                                       Efl_Canvas_Translate_Animation_Data *pd,
                                                       double progress,
                                                       Efl_Canvas_Object *target)
{
   _Translate_Property_Double new;
   Eina_Rect geometry;

   progress = efl_animation_apply(efl_super(eo_obj, MY_CLASS), progress, target);
   if (!target) return progress;

   if (pd->use_rel_move)
     {
        new.x = GET_STATUS(pd->from.x, pd->to.x, progress);
        new.y = GET_STATUS(pd->from.y, pd->to.y, progress);
     }
   else
     {
        geometry = efl_gfx_entity_geometry_get(target);
        new.x = GET_STATUS(pd->from.x, pd->to.x, progress) - geometry.x;
        new.y = GET_STATUS(pd->from.y, pd->to.y, progress) - geometry.y;
     }

   efl_gfx_mapping_translate(target, new.x, new.y, 0.0);

   return progress;
}

/**
 * @brief Constructor for the Efl_Canvas_Translate_Animation object.
 *
 * Initializes the animation with default values: from and to positions are (0,0),
 * and relative move is enabled by default.
 *
 * @param[in] eo_obj The Eolian object to be constructed.
 * @param[in,out] pd The private data for the Efl_Canvas_Translate_Animation.
 * @return The constructed Eolian object.
 */
EOLIAN static Efl_Object *
_efl_canvas_translate_animation_efl_object_constructor(Eo *eo_obj,
                                                Efl_Canvas_Translate_Animation_Data *pd)
{
   eo_obj = efl_constructor(efl_super(eo_obj, MY_CLASS));

   pd->from = EINA_POSITION2D(0,0);
   pd->to = EINA_POSITION2D(0,0);

   pd->use_rel_move = EINA_TRUE;

   return eo_obj;
}

#include "efl_canvas_translate_animation.eo.c"
