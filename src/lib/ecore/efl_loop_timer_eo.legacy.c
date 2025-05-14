/**
 * @brief Sets the interval for the timer.
 *
 * This function is a legacy wrapper for efl_loop_timer_interval_set().
 * It defines how often the timer will tick. If called during a timer's
 * execution, this will adjust the interval for the *next* tick.
 *
 * @param obj The Efl_Loop_Timer object.
 * @param in The new interval in seconds. For example, 0.5 for half a second.
 */
EAPI void
ecore_timer_interval_set(Efl_Loop_Timer *obj, double in)
{
   efl_loop_timer_interval_set(obj, in);
}

/**
 * @brief Gets the current interval of the timer.
 *
 * This function is a legacy wrapper for efl_loop_timer_interval_get().
 * It returns the time in seconds between timer ticks.
 *
 * @param obj The Efl_Loop_Timer object.
 * @return The interval in seconds. For example, 1.0 for one second.
 */
EAPI double
ecore_timer_interval_get(const Efl_Loop_Timer *obj)
{
   return efl_loop_timer_interval_get(obj);
}

/**
 * @brief Gets the pending time until the next timer tick.
 *
 * This function is a legacy wrapper for efl_loop_timer_time_pending_get().
 * It returns the remaining time in seconds before the timer is
 * due to fire next.
 *
 * @param obj The Efl_Loop_Timer object.
 * @return The pending time in seconds.
 */
EAPI double
ecore_timer_pending_get(const Efl_Loop_Timer *obj)
{
   return efl_loop_timer_time_pending_get(obj);
}

/**
 * @brief Resets the timer to its full interval.
 *
 * This function is a legacy wrapper for efl_loop_timer_reset().
 * This makes the timer start counting down from its full interval again,
 * effectively as if it were just created or as if the current tick just occurred.
 * It's equivalent to delaying the timer by the time already elapsed since
 * its last tick (or creation).
 *
 * @param obj The Efl_Loop_Timer object.
 */
EAPI void
ecore_timer_reset(Efl_Loop_Timer *obj)
{
   efl_loop_timer_reset(obj);
}

/**
 * @brief Resets the timer based on the start time of the current main loop iteration.
 *
 * This function is a legacy wrapper for efl_loop_timer_loop_reset().
 * This function resets the timer, but its new "zero" point is considered
 * to be the time when the current iteration of the main loop began.
 * This can be useful for synchronizing timers with frame rendering or
 * other loop-based activities.
 *
 * @param obj The Efl_Loop_Timer object.
 */
EAPI void
ecore_timer_loop_reset(Efl_Loop_Timer *obj)
{
   efl_loop_timer_loop_reset(obj);
}

/**
 * @brief Delays the next occurrence of the timer.
 *
 * This function is a legacy wrapper for efl_loop_timer_delay().
 * It adds an additional amount of time to the current pending time
 * before the timer's next tick. This does not change the timer's
 * overall interval, only when the *next* tick will occur.
 *
 * @param obj The Efl_Loop_Timer object.
 * @param add The amount of time in seconds to add to the delay.
 *            For example, 0.2 to delay by an additional 200ms.
 */
EAPI void
ecore_timer_delay(Efl_Loop_Timer *obj, double add)
{
   efl_loop_timer_delay(obj, add);
}
