#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "evas_common_private.h"
#include "evas_private.h"
#include "efl_canvas_object_animation.eo.h"
#include <Ecore.h>

#define MY_CLASS EFL_CANVAS_OBJECT_ANIMATION_MIXIN

/**
 * @brief Internal data structure for managing an animation on an Efl_Canvas_Object.
 *
 * This structure holds all the runtime state for an animation instance, including
 * the animation itself, playback speed, progress, timing information, repeat counts,
 * and the timer used for driving the animation ticks.
 */
typedef struct
{
   Efl_Canvas_Animation *animation; /**< The animation object being played. */
   double speed; /**< The playback speed multiplier. Negative values play in reverse. */
   double progress; /**< The current progress of the animation, from 0.0 to 1.0. */
   double run_start_time; /**< The timestamp when the current animation run (or segment) started. */
   double start_pos; /**< The initial starting position of the animation [0.0-1.0]. */
   int remaining_repeats; /**< Number of times the animation will still repeat. EFL_ANIMATION_PLAY_COUNT_INFINITE for infinite. */
   Efl_Loop_Timer *timer; /**< Legacy timer, not used in this implementation which relies on EFL_CANVAS_OBJECT_EVENT_ANIMATOR_TICK. */
   Eina_Bool pause_state : 1; /**< Flag indicating if the animation is currently paused. */
} Efl_Canvas_Object_Animation_Indirect_Data;

/**
 * @brief Main data structure for the Efl_Canvas_Object_Animation mixin.
 *
 * This structure primarily holds a pointer to the indirect animation data,
 * which contains the actual state of the animation.
 */
typedef struct
{
   Efl_Canvas_Object_Animation_Indirect_Data *in; /**< Pointer to the indirect animation data. NULL if no animation is active. */
} Efl_Canvas_Object_Animation_Data;

static void _end(Efl_Canvas_Object_Animation *obj, Efl_Canvas_Object_Animation_Data *pd);

/**
 * @brief Callback function executed on each animator tick.
 *
 * This function is responsible for calculating the current animation progress
 * based on elapsed time and duration, applying the animation effect to the
 * object, and handling animation repeats or completion.
 *
 * @param data The Efl_Canvas_Object that this animation is applied to.
 * @param ev The event information (unused).
 */
static void
_animator_cb(void *data, const Efl_Event *ev EINA_UNUSED)
{
   Eo *obj = data;
   Efl_Canvas_Object_Animation_Data *pd = efl_data_scope_get(obj, MY_CLASS);
   double duration, elapsed_time, vector, current;

   EINA_SAFETY_ON_NULL_RETURN(pd->in);
   current = ecore_loop_time_get();
   EINA_SAFETY_ON_FALSE_RETURN(pd->in->run_start_time <= current);

   duration = efl_animation_duration_get(pd->in->animation) / pd->in->speed;
   elapsed_time = current - pd->in->run_start_time;
   if (EINA_DBL_EQ(duration, 0))
     {
        if (pd->in->speed < 0.0)
          vector = -1.0;
        else
          vector = 1.0;
     }
   else
     vector = elapsed_time / duration;

   /* When animation player starts, _animator_cb() is called immediately so
    * both elapsed time and progress are 0.0.
    * Since it is the beginning of the animation if progress is 0.0, the
    * following codes for animation should be executed. */
   if (pd->in->speed < 0.0)
     vector += 1.0;
   pd->in->progress = CLAMP(0.0, vector, 1.0);

   /* The previously applied map effect should be reset before applying the
    * current map effect. Otherwise, the incrementally added map effects
    * increase numerical error. */
   efl_gfx_mapping_reset(obj);
   efl_animation_apply(pd->in->animation, pd->in->progress, obj);

   double progress = pd->in->progress;
   efl_event_callback_call(obj, EFL_CANVAS_OBJECT_ANIMATION_EVENT_ANIMATION_PROGRESS_UPDATED, &progress);

   //Check if animation stopped in animation_progress,updated callback.
   if (!pd->in) return;

   //Not end. Keep going.
   if ((pd->in->speed < 0 && EINA_DBL_EQ(pd->in->progress, 0)) ||
       (pd->in->speed > 0 && EINA_DBL_EQ(pd->in->progress, 1.0)))
     {
        //Repeat animation
        if ((efl_animation_play_count_get(pd->in->animation) == 0) ||
            (pd->in->remaining_repeats > 0))
          {
             pd->in->remaining_repeats--;

             if (efl_animation_repeat_mode_get(pd->in->animation) == EFL_CANVAS_ANIMATION_REPEAT_MODE_REVERSE)
               pd->in->speed *= -1;

             pd->in->run_start_time = current;
          }
        else
          {
             efl_canvas_object_animation_stop(obj);
          }
     }
}

/**
 * @brief Cleans up resources when an animation ends or is stopped.
 *
 * This function removes the animator tick callback. It does not free
 * the animation data itself, as that is handled by _efl_canvas_object_animation_animation_stop().
 *
 * @param obj The Efl_Canvas_Object associated with the animation.
 * @param pd The animation data.
 */
static void
_end(Efl_Canvas_Object_Animation *obj, Efl_Canvas_Object_Animation_Data *pd)
{
   EINA_SAFETY_ON_NULL_RETURN(pd->in);
   efl_event_callback_del(obj, EFL_CANVAS_OBJECT_EVENT_ANIMATOR_TICK, _animator_cb, obj);
}

/**
 * @brief Initializes and starts the animation playback.
 *
 * Sets up the start time considering any delay and registers the animator tick callback.
 * It also calls the animator callback once immediately to apply the initial state.
 *
 * @param obj The Efl_Canvas_Object to animate.
 * @param pd The animation data.
 * @param delay The relative starting position of the animation (0.0 to 1.0),
 *              used to calculate the effective start time. For example, a delay of 0.5
 *              means the animation starts halfway through.
 */
static void
_start(Efl_Canvas_Object_Animation *obj, Efl_Canvas_Object_Animation_Data *pd, double delay)
{
   EINA_SAFETY_ON_NULL_RETURN(pd->in);
   pd->in->run_start_time = ecore_loop_time_get() - efl_animation_duration_get(pd->in->animation)*delay;
   efl_event_callback_add(obj, EFL_CANVAS_OBJECT_EVENT_ANIMATOR_TICK, _animator_cb, obj);
   _animator_cb(obj, NULL);
}

/**
 * @brief Future callback to start the animation after a specified delay.
 *
 * This function is invoked when a future, set up for a start delay, resolves.
 * It then calls the main _start() function.
 *
 * @param o The Efl_Canvas_Object.
 * @param data User data associated with the future (unused).
 * @param v The value resolved by the future (unused).
 * @return Eina_Value Returns the input value `v`, or EINA_VALUE_EMPTY if an error occurs.
 */
static Eina_Value
_start_fcb(Eo *o, void *data EINA_UNUSED, const Eina_Value v)
{
   Efl_Canvas_Object_Animation_Data *pd = efl_data_scope_safe_get(o, MY_CLASS);
   EINA_SAFETY_ON_NULL_RETURN_VAL(pd, EINA_VALUE_EMPTY);
   if (!pd->in) return v; //animation was stopped before anything started
   _start(o, pd, pd->in->start_pos);
   return v;
}

/**
 * @brief Gets the current animation object.
 * @param obj The Efl_Canvas_Object (unused).
 * @param pd The animation data.
 * @return The current Efl_Canvas_Animation object, or NULL if no animation is active.
 */
EOLIAN static Efl_Canvas_Animation*
_efl_canvas_object_animation_animation_get(const Eo *obj EINA_UNUSED, Efl_Canvas_Object_Animation_Data *pd)
{
   if (!pd->in) return NULL;
   return pd->in->animation;
}

/**
 * @brief Gets the current progress of the animation.
 *
 * The progress is a value between 0.0 (start) and 1.0 (end).
 * If the animation is playing in reverse, this still returns progress from start to end
 * (e.g. if speed is negative and internal progress is 0.2, this returns 0.8).
 *
 * @param obj The Efl_Canvas_Object (unused).
 * @param pd The animation data.
 * @return The animation progress from 0.0 to 1.0, or -1.0 if no animation is active.
 */
EOLIAN static double
_efl_canvas_object_animation_animation_progress_get(const Eo *obj EINA_UNUSED, Efl_Canvas_Object_Animation_Data *pd)
{
   if (pd->in && pd->in->animation)
     return (pd->in->speed < 0) ? fabs(1.0 - pd->in->progress) : pd->in->progress;
   else
     return -1.0;
}

/**
 * @brief Sets the pause state of the animation.
 *
 * If pausing, it stops the animator tick. If resuming, it restarts the animator tick
 * from the current progress.
 *
 * @param obj The Efl_Canvas_Object.
 * @param pd The animation data.
 * @param pause EINA_TRUE to pause, EINA_FALSE to resume.
 */
EOLIAN static void
_efl_canvas_object_animation_animation_pause_set(Eo *obj, Efl_Canvas_Object_Animation_Data *pd, Eina_Bool pause)
{
   EINA_SAFETY_ON_NULL_RETURN(pd->in);

   if (pd->in->pause_state == pause) return;

   if (pause)
     _end(obj, pd);
   else
     _start(obj, pd,(pd->in->speed < 0) ? 1.0 - pd->in->progress : pd->in->progress);
   if (pd->in) pd->in->pause_state = pause;
}

/**
 * @brief Gets the current pause state of the animation.
 *
 * @param obj The Efl_Canvas_Object (unused).
 * @param pd The animation data.
 * @return EINA_TRUE if paused, EINA_FALSE otherwise or if no animation is active.
 */
EOLIAN static Eina_Bool
_efl_canvas_object_animation_animation_pause_get(const Eo *obj EINA_UNUSED, Efl_Canvas_Object_Animation_Data *pd)
{
   if (!pd->in) return EINA_FALSE;

   return pd->in->pause_state;
}

/**
 * @brief Starts a new animation on the object.
 *
 * If an animation is already running, it will be stopped first.
 * This function initializes the animation state, including speed, start position,
 * and repeat count. It then either starts the animation immediately or schedules
 * it to start after a delay specified in the animation object.
 *
 * @param obj The Efl_Canvas_Object to animate.
 * @param pd The animation data for this object.
 * @param animation The Efl_Canvas_Animation object to play. Must be seekable.
 * @param speed The playback speed multiplier. Positive values play forward,
 *              negative values play in reverse. Cannot be 0.0.
 *              Example: 1.0 for normal speed, 2.0 for double speed, -1.0 for reverse.
 * @param start_pos The normalized starting position of the animation (0.0 to 1.0).
 *                  Example: 0.0 to start from the beginning, 0.5 to start from the middle.
 */
EOLIAN static void
_efl_canvas_object_animation_animation_start(Eo *obj, Efl_Canvas_Object_Animation_Data *pd, Efl_Canvas_Animation *animation, double speed, double start_pos)
{
   Efl_Canvas_Object_Animation_Indirect_Data *in;
   if (pd->in && pd->in->animation)
     efl_canvas_object_animation_stop(obj);
   EINA_SAFETY_ON_FALSE_RETURN(!pd->in);
   in = pd->in = calloc(1, sizeof(Efl_Canvas_Object_Animation_Indirect_Data));

   EINA_SAFETY_ON_NULL_RETURN(animation);
   EINA_SAFETY_ON_FALSE_RETURN(start_pos >= 0.0 && start_pos <= 1.0);
   EINA_SAFETY_ON_FALSE_RETURN(!EINA_DBL_EQ(speed, 0.0));
   EINA_SAFETY_ON_FALSE_RETURN(efl_playable_seekable_get(animation));

   in->pause_state = EINA_FALSE;
   in->animation = efl_ref(animation);
   in->remaining_repeats = efl_animation_play_count_get(animation) - 1; // -1 because one run is already going on
   in->speed = speed;
   in->start_pos = start_pos;
   efl_event_callback_call(obj, EFL_CANVAS_OBJECT_ANIMATION_EVENT_ANIMATION_CHANGED, in->animation);
   //You should not rely on in beeing available after calling the above event.
   in = NULL;

   if (efl_animation_start_delay_get(animation) > 0.0)
     {
        Eina_Future *f = efl_loop_timeout(efl_loop_get(obj), efl_animation_start_delay_get(animation));

        efl_future_then(obj, f, .success = _start_fcb);
     }
   else
     _start(obj, pd, start_pos);
}

/**
 * @brief Stops the currently running animation.
 *
 * This function cleans up all resources associated with the current animation.
 * It respects the `final_state_keep` property of the animation, meaning
 * it may or may not reset the object's mapping to its pre-animation state.
 * It also emits an EFL_CANVAS_OBJECT_ANIMATION_EVENT_ANIMATION_CHANGED event
 * with a NULL animation payload.
 *
 * @param obj The Efl_Canvas_Object whose animation is to be stopped.
 * @param pd The animation data.
 */
EOLIAN static void
_efl_canvas_object_animation_animation_stop(Eo *obj, Efl_Canvas_Object_Animation_Data *pd)
{
   if (!pd->in) return;

   if (!efl_animation_final_state_keep_get(pd->in->animation))
     efl_gfx_mapping_reset(obj);
   _end(obj, pd);
   efl_unref(pd->in->animation);
   pd->in->animation = NULL;

   efl_event_callback_call(obj, EFL_CANVAS_OBJECT_ANIMATION_EVENT_ANIMATION_CHANGED, pd->in->animation);

   //this could be NULL if some weird callstack calls stop again while the above event is executed
   if (pd->in)
     free(pd->in);
   pd->in = NULL;
}

#include "efl_canvas_object_animation.eo.c"
