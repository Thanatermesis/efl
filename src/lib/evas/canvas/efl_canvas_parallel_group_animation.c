#include "efl_canvas_parallel_group_animation_private.h"

#define MY_CLASS EFL_CANVAS_PARALLEL_GROUP_ANIMATION_CLASS

/**
 * @brief Applies the animations in the parallel group to the target object
 *        based on the given progress.
 *
 * This function iterates through all animations within the parallel group.
 * For each animation, it calculates its individual progress based on the
 * overall group progress, its own duration, start delay, and repeat behavior.
 * It then applies this calculated progress to the animation on the target object.
 *
 * @param[in] eo_obj The Eolian object instance.
 * @param[in] _pd Private data, unused in this function.
 * @param[in] progress The overall progress of the parallel group animation (0.0 to 1.0).
 * @param[in] target The canvas object to which the animations are applied.
 * @return The final progress value after applying the super class's apply function.
 */
EOLIAN static double
_efl_canvas_parallel_group_animation_efl_canvas_animation_animation_apply(Eo *eo_obj,
                                                            void *_pd EINA_UNUSED,
                                                            double progress,
                                                            Efl_Canvas_Object *target)
{
   double group_length, group_elapsed_time;
   double anim_length, anim_duration, anim_start_delay, anim_progress, anim_elapsed_time;
   int anim_repeated_count;

   progress = efl_animation_apply(efl_super(eo_obj, MY_CLASS), progress, target);
   Eina_Iterator *group_anim = efl_animation_group_animations_get(eo_obj);
   if (!group_anim) return progress;

   group_length = efl_playable_length_get(eo_obj);
   group_elapsed_time = group_length * progress;

   Efl_Canvas_Animation *anim;
   EINA_ITERATOR_FOREACH(group_anim, anim)
     {
        anim_length = efl_playable_length_get(anim);
        anim_duration = efl_animation_duration_get(anim);
        anim_start_delay = efl_animation_start_delay_get(anim);

        anim_repeated_count = (int)(group_elapsed_time / anim_length);

        if ((anim_length + anim_start_delay) <= group_elapsed_time)
          {
             if (efl_animation_final_state_keep_get(anim) && (!FINAL_STATE_IS_REVERSE(anim)))
               anim_progress = 1.0;
             else
               anim_progress = 0.0;
             efl_animation_apply(anim, anim_progress, target);

             continue;
          }

        anim_elapsed_time =
           MAX(((group_elapsed_time - (anim_length * anim_repeated_count)) - anim_start_delay), 0.0);
        anim_progress = MIN((anim_elapsed_time / anim_duration), 1.0);
        if (FINAL_STATE_IS_REVERSE(anim))
          anim_progress = 1.0 - anim_progress;

        efl_animation_apply(anim, anim_progress, target);
     }
   eina_iterator_free(group_anim);

   return progress;
}

/**
 * @brief Calculates the total duration of the parallel group animation.
 *
 * The total duration of a parallel group is determined by the child animation
 * that finishes last. This includes the child animation's own duration and
 * its start delay.
 *
 * @param[in] eo_obj The Eolian object instance.
 * @param[in] _pd Private data, unused in this function.
 * @return The total duration of the parallel group animation in seconds.
 *         Returns 0.0 if there are no animations in the group.
 */
EOLIAN static double
_efl_canvas_parallel_group_animation_efl_canvas_animation_duration_get(const Eo *eo_obj, void *_pd EINA_UNUSED)
{
   double child_total_duration;
   double total_duration = 0.0;

   Eina_Iterator *group_anim = efl_animation_group_animations_get(eo_obj);
   if (!group_anim) return 0.0;

   Efl_Canvas_Animation *anim;
   EINA_ITERATOR_FOREACH(group_anim, anim)
     {
        child_total_duration = efl_playable_length_get(anim);
        child_total_duration += efl_animation_start_delay_get(anim);
        if (child_total_duration > total_duration)
          total_duration = child_total_duration;
     }
   eina_iterator_free(group_anim);

   return total_duration;
}

#include "efl_canvas_parallel_group_animation.eo.c"
