#include "efl_canvas_animation_private.h"
#include <math.h>
#define MY_CLASS EFL_CANVAS_ANIMATION_CLASS

/**
 * @brief Default duration for animations in seconds.
 *
 * This value is used when an animation is created without a specific duration.
 */
static double _default_animation_time = 0.2; //in seconds

EOLIAN static void
_efl_canvas_animation_duration_set(Eo *eo_obj EINA_UNUSED,
                            Efl_Canvas_Animation_Data *pd,
                            double sec)
{
   /**
    * @brief Sets the duration of the animation.
    *
    * @param sec The duration in seconds. Must be non-negative.
    */
   EINA_SAFETY_ON_FALSE_RETURN(sec >= 0.0);
   pd->duration = sec;
}

EOLIAN static double
_efl_canvas_animation_duration_get(const Eo *eo_obj EINA_UNUSED, Efl_Canvas_Animation_Data *pd)
{
   /**
    * @brief Gets the duration of the animation.
    *
    * @return The duration in seconds.
    */
   return pd->duration;
}

EOLIAN static void
_efl_canvas_animation_final_state_keep_set(Eo *eo_obj EINA_UNUSED,
                                    Efl_Canvas_Animation_Data *pd,
                                    Eina_Bool keep)
{
   /**
    * @brief Sets whether the animation should keep its final state after finishing.
    *
    * @param keep EINA_TRUE to keep the final state, EINA_FALSE otherwise.
    */
   if (pd->keep_final_state == keep) return;

   pd->keep_final_state = !!keep;
}

EOLIAN static Eina_Bool
_efl_canvas_animation_final_state_keep_get(const Eo *eo_obj EINA_UNUSED,
                                    Efl_Canvas_Animation_Data *pd)
{
   /**
    * @brief Gets whether the animation keeps its final state after finishing.
    *
    * @return EINA_TRUE if the final state is kept, EINA_FALSE otherwise.
    */
   return pd->keep_final_state;
}

EOLIAN static void
_efl_canvas_animation_repeat_mode_set(Eo *eo_obj EINA_UNUSED,
                               Efl_Canvas_Animation_Data *pd,
                               Efl_Canvas_Animation_Repeat_Mode mode)
{
   /**
    * @brief Sets the repeat mode of the animation.
    *
    * @param mode The repeat mode to set.
    *             Example: EFL_CANVAS_ANIMATION_REPEAT_MODE_RESTART, EFL_CANVAS_ANIMATION_REPEAT_MODE_REVERSE.
    */
   EINA_SAFETY_ON_FALSE_RETURN(mode >= 0 && mode < EFL_CANVAS_ANIMATION_REPEAT_MODE_LAST);
   pd->repeat_mode = mode;
}

EOLIAN static Efl_Canvas_Animation_Repeat_Mode
_efl_canvas_animation_repeat_mode_get(const Eo *eo_obj EINA_UNUSED, Efl_Canvas_Animation_Data *pd)
{
   /**
    * @brief Gets the repeat mode of the animation.
    *
    * @return The current repeat mode.
    */
   return pd->repeat_mode;
}

EOLIAN static void
_efl_canvas_animation_play_count_set(Eo *eo_obj EINA_UNUSED,
                                Efl_Canvas_Animation_Data *pd,
                                int count)
{
   /**
    * @brief Sets the number of times the animation should play.
    *
    * @param count The number of repetitions. 0 means infinite.
    */
   EINA_SAFETY_ON_FALSE_RETURN(count >= 0);

   pd->play_count = count;
}

EOLIAN static int
_efl_canvas_animation_play_count_get(const Eo *eo_obj EINA_UNUSED, Efl_Canvas_Animation_Data *pd)
{
   /**
    * @brief Gets the number of times the animation should play.
    *
    * @return The number of repetitions. 0 means infinite.
    */
   return pd->play_count;
}

EOLIAN static void
_efl_canvas_animation_start_delay_set(Eo *eo_obj EINA_UNUSED,
                               Efl_Canvas_Animation_Data *pd,
                               double sec)
{
   /**
    * @brief Sets the delay before the animation starts.
    *
    * @param sec The delay in seconds. Must be non-negative.
    */
   EINA_SAFETY_ON_FALSE_RETURN(sec >= 0.0);

   pd->start_delay_time = sec;
}

EOLIAN static double
_efl_canvas_animation_start_delay_get(const Eo *eo_obj EINA_UNUSED,
                               Efl_Canvas_Animation_Data *pd)
{
   /**
    * @brief Gets the delay before the animation starts.
    *
    * @return The delay in seconds.
    */
   return pd->start_delay_time;
}

EOLIAN static void
_efl_canvas_animation_interpolator_set(Eo *eo_obj EINA_UNUSED,
                                Efl_Canvas_Animation_Data *pd,
                                Efl_Interpolator *interpolator)
{
   /**
    * @brief Sets the interpolator for the animation.
    *
    * The interpolator defines the rate of change of the animation over time.
    *
    * @param interpolator The interpolator object.
    */
   pd->interpolator = interpolator;
}

EOLIAN static Efl_Interpolator *
_efl_canvas_animation_interpolator_get(const Eo *eo_obj EINA_UNUSED,
                                Efl_Canvas_Animation_Data *pd)
{
   /**
    * @brief Gets the interpolator for the animation.
    *
    * @return The interpolator object, or NULL if none is set.
    */
   return pd->interpolator;
}

EOLIAN static double
_efl_canvas_animation_animation_apply(Eo *eo_obj,
                               Efl_Canvas_Animation_Data *pd EINA_UNUSED,
                               double progress,
                               Efl_Canvas_Object *target EINA_UNUSED)
{
   /**
    * @brief Applies the animation to a target object based on the progress.
    *
    * This function calculates the effective progress after applying an interpolator,
    * if one is set. Subclasses should override this to implement the actual
    * animation logic (e.g., changing object properties).
    *
    * @param progress The raw animation progress, typically from 0.0 (start) to 1.0 (end).
    * @param target The canvas object to which the animation is applied. (Currently unused in base).
    * @return The effective progress after interpolation.
    */
   Efl_Interpolator *interpolator = efl_animation_interpolator_get(eo_obj);
   if (interpolator)
     progress = efl_interpolator_interpolate(interpolator, progress);

   return progress;
}

EOLIAN static double
_efl_canvas_animation_efl_playable_length_get(const Eo *eo_obj, Efl_Canvas_Animation_Data *pd EINA_UNUSED)
{
   /**
    * @brief Gets the total playable length of the animation.
    *
    * This is calculated as duration * play_count.
    * If play_count is 0 (infinite), returns INFINITY.
    *
    * @return The total length in seconds, or INFINITY.
    */
   if (efl_animation_play_count_get(eo_obj) == 0)
     {
        return INFINITY;
     }

   return (efl_animation_duration_get(eo_obj) * efl_animation_play_count_get(eo_obj));
}

EOLIAN static Eina_Bool
_efl_canvas_animation_efl_playable_playable_get(const Eo *eo_obj EINA_UNUSED, Efl_Canvas_Animation_Data *pd EINA_UNUSED)
{
   /**
    * @brief Gets whether the animation is playable.
    *
    * Canvas animations are always considered playable.
    *
    * @return EINA_TRUE.
    */
   return EINA_TRUE;
}

EOLIAN static Eina_Bool
_efl_canvas_animation_efl_playable_seekable_get(const Eo *eo_obj EINA_UNUSED, Efl_Canvas_Animation_Data *pd EINA_UNUSED)
{
   /**
    * @brief Gets whether the animation is seekable.
    *
    * Canvas animations are always considered seekable.
    *
    * @return EINA_TRUE.
    */
   return EINA_TRUE;
}

EOLIAN static Efl_Object*
_efl_canvas_animation_efl_object_constructor(Eo *obj, Efl_Canvas_Animation_Data *pd)
{
   /**
    * @brief Constructor for Efl_Canvas_Animation objects.
    *
    * Initializes animation properties to default values:
    * - duration: _default_animation_time
    * - play_count: 1
    */
   pd->duration = _default_animation_time;
   pd->play_count = 1;
   return efl_constructor(efl_super(obj, MY_CLASS));
}

EOLIAN static void
_efl_canvas_animation_default_duration_set(double animation_time)
{
   /**
    * @brief Sets the global default duration for new animations.
    *
    * @param animation_time The new default duration in seconds. Must be non-negative.
    */
   EINA_SAFETY_ON_FALSE_RETURN(animation_time >= 0.0);
   _default_animation_time = animation_time;
}

EOLIAN static double
_efl_canvas_animation_default_duration_get(void)
{
   /**
    * @brief Gets the global default duration for new animations.
    *
    * @return The default duration in seconds.
    */
   return _default_animation_time;
}

#include "efl_canvas_animation.eo.c"
