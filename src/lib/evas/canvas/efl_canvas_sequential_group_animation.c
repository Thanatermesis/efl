#include "efl_canvas_sequential_group_animation_private.h"

#define MY_CLASS EFL_CANVAS_SEQUENTIAL_GROUP_ANIMATION_CLASS

/**
 * @brief Applies the animation to the target object at a given progress.
 *
 * This function calculates the current state of a sequence of animations
 * based on the overall progress and applies the relevant child animation's
 * state to the target.
 *
 * @param eo_obj The Eolian object.
 * @param _pd Private data, unused.
 * @param progress The progress of the animation, from 0.0 to 1.0.
 * @param target The target Efl_Canvas_Object to which the animation is applied.
 * @return The actual progress applied, which might be modified by the parent class.
 */
EOLIAN static double
_efl_canvas_sequential_group_animation_efl_canvas_animation_animation_apply(Eo *eo_obj,
                                                              void *_pd EINA_UNUSED,
                                                              double progress,
                                                              Efl_Canvas_Object *target)
{
   double group_length, group_elapsed_time;
   double anim_length, anim_duration, anim_start_delay, anim_progress, anim_play_time, anim_position;
   double total_anim_elapsed_time = 0.0;
   double temp;
   int anim_repeated_count;

   progress = efl_animation_apply(efl_super(eo_obj, MY_CLASS), progress, target);
   Eina_Iterator *group_anim = efl_animation_group_animations_get(eo_obj);
   if (!group_anim) return progress;

   group_length = efl_playable_length_get(eo_obj);
   group_elapsed_time = group_length * progress;

   Efl_Canvas_Animation *anim;
   EINA_ITERATOR_FOREACH(group_anim, anim)
     {
        anim_start_delay = efl_animation_start_delay_get(anim);
        anim_length = efl_playable_length_get(anim) + anim_start_delay;
        anim_duration = efl_animation_duration_get(anim);

        // Check if this animation has already completed.
        // temp calculates the time at which the current animation in the sequence would end.
        temp = total_anim_elapsed_time + anim_length + anim_start_delay;
        if (temp <= group_elapsed_time)
          {
             // If the animation is completed, apply its final state or initial state
             // based on keep_final_state and reverse flags.
             if (efl_animation_final_state_keep_get(anim) && (!FINAL_STATE_IS_REVERSE(anim)))
               anim_progress = 1.0;
             else
               anim_progress = 0.0;
             efl_animation_apply(anim, anim_progress, target);
             total_anim_elapsed_time = temp;
             continue;
          }

        // This animation is currently active or has not started yet.
        // Calculate the effective play time for this animation within the group's elapsed time.
        anim_play_time = group_elapsed_time - total_anim_elapsed_time - anim_start_delay;
        // TODO: check infinite repeat
        // Calculate how many times this animation has repeated.
        anim_repeated_count = (int)(anim_play_time / anim_length);
        // Calculate the current position within a single run of this animation.
        anim_position = MAX(((anim_play_time - anim_duration * anim_repeated_count)), 0.0);
        // Normalize the position to a progress value (0.0 to 1.0).
        anim_progress = MIN((anim_position / anim_duration), 1.0);
        // If the animation is set to reverse its final state, invert the progress.
        if (FINAL_STATE_IS_REVERSE(anim))
          anim_progress = 1.0 - anim_progress;
        efl_animation_apply(anim, anim_progress, target);

        // Since animations in a sequential group play one after another,
        // once the currently playing animation is found and applied, we can stop.
        break;
     }
   eina_iterator_free(group_anim);

   return progress;
}

/**
 * @brief Gets the total duration of the sequential group animation.
 *
 * This is the sum of the durations of all child animations, including their
 * start delays.
 *
 * @param eo_obj The Eolian object.
 * @param _pd Private data, unused.
 * @return The total duration of the animation in seconds.
 */
EOLIAN static double
_efl_canvas_sequential_group_animation_efl_canvas_animation_duration_get(const Eo *eo_obj, void *_pd EINA_UNUSED)
{
   double total_duration = 0.0;
   double child_total_duration;

   Eina_Iterator *group_anim = efl_animation_group_animations_get(eo_obj);
   if (!group_anim) return 0.0;

   Efl_Canvas_Animation *anim;
   EINA_ITERATOR_FOREACH(group_anim, anim)
     {
        child_total_duration = efl_playable_length_get(anim);
        child_total_duration += efl_animation_start_delay_get(anim);
        total_duration += child_total_duration;
     }
   eina_iterator_free(group_anim);

   return total_duration;
}

#include "efl_canvas_sequential_group_animation.eo.c"
