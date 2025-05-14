#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>

#include <Eo.h>

#include "Ecore.h"
#include "ecore_private.h"

#define MY_CLASS ECORE_POLLER_CLASS

#define MY_CLASS_NAME "Ecore_Poller"

#define ECORE_POLLER_CHECK(obj)                       \
  if (!efl_isa((obj), ECORE_POLLER_CLASS)) \
    return

/**
 * @brief Structure representing an Ecore_Poller instance.
 *
 * This structure holds all the necessary information for a poller,
 * including its callback function, data, interval bit, and management flags.
 */
struct _Ecore_Poller
{
   EINA_INLIST; /**< Macro for Eina_Inlist node integration. */
   ECORE_MAGIC; /**< Magic number for Ecore object validation. */
   int           ibit; /**< The bit representing the poller's interval (0-15).
                        * Interval = (1 << ibit) * poll_interval. */
   unsigned char delete_me : 1; /**< Flag indicating if the poller should be deleted. */
   Ecore_Task_Cb func; /**< The callback function to execute for this poller. */
   void         *data; /**< User-provided data for the callback function. */
};

static Ecore_Timer *timer = NULL; /**< The main timer that drives all pollers. */
static int min_interval = -1; /**< Index of the smallest poller interval currently active (0-14, or -1 if no pollers). */
static int interval_incr = 0; /**< The increment value for poller_counters, derived from min_interval (1 << min_interval). */
static int at_tick = 0; /**< A counter incremented when inside _ecore_poller_cb_timer, used to detect re-entrancy or specific states. */
static int just_added_poller = 0; /**< Flag set if a poller was added during a poller walk, signaling a need to re-evaluate intervals. */
static int poller_delete_count = 0; /**< Count of pollers marked for deletion. */
static int poller_walking = 0; /**< A counter incremented when iterating through pollers, to handle modifications safely. */
static double poll_interval = 0.125; /**< The base time unit in seconds for poller ticks. Default is 0.125s. */
static double poll_cur_interval = 0.0; /**< The current actual interval of the main timer in seconds. */
static double last_tick = 0.0; /**< Timestamp of the last execution of _ecore_poller_cb_timer. */

/**
 * @brief Array of Ecore_Poller inlists, indexed by their interval bit.
 *
 * Each element `pollers[i]` is the head of an Eina_Inlist of pollers
 * that should be polled every `(1 << i) * poll_interval` seconds.
 * For example:
 * - `pollers[0]` contains pollers for interval `1 * poll_interval`
 * - `pollers[1]` contains pollers for interval `2 * poll_interval`
 * - `pollers[n]` contains pollers for interval `(1 << n) * poll_interval`
 * The array size is 16, allowing for `ibit` values from 0 to 15.
 * However, loops typically iterate up to 14, as `ibit = 15` is the maximum.
 */
static Ecore_Poller *pollers[16] =
{
   NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
   NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

/**
 * @brief Counters for each poller interval group.
 *
 * `poller_counters[i]` corresponds to `pollers[i]`. It increments by
 * `interval_incr` at each main timer tick. When `poller_counters[i]`
 * wraps around to 0 (i.e., `poller_counters[i] >= (1 << i)`),
 * the pollers in `pollers[i]` are executed.
 */
static unsigned short poller_counters[16] =
{
   0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0
};

static Eina_Bool _ecore_poller_cb_timer(void *data);

/**
 * @brief Cleans up a poller instance.
 *
 * Removes the poller from its list and frees its memory.
 *
 * @param poller The poller to clean up.
 * @return The data pointer originally associated with the poller.
 */
static void *
_ecore_poller_cleanup(Ecore_Poller *poller)
{
   void *data;

   data = poller->data;
   pollers[poller->ibit] = (Ecore_Poller *)eina_inlist_remove(EINA_INLIST_GET(pollers[poller->ibit]), EINA_INLIST_GET(poller));

   free(poller);

   return data;
}

/**
 * @brief Evaluates and sets the next tick interval for the main poller timer.
 *
 * This function determines the shortest poller interval currently active
 * and configures the main timer (`timer`) to fire at that interval.
 * If no pollers are active, the timer is deleted.
 * It considers whether it's being called from within a tick (`at_tick`)
 * to adjust timing appropriately.
 */
static void
_ecore_poller_next_tick_eval(void)
{
   int i;
   double interval;

   min_interval = -1;
   for (i = 0; i < 15; i++)
     {
        if (pollers[i])
          {
             min_interval = i;
             break;
          }
     }
   if (min_interval < 0)
     {
        /* no pollers */
         if (timer)
           {
              ecore_timer_del(timer);
              timer = NULL;
           }
         return;
     }
   interval_incr = (1 << min_interval);
   interval = interval_incr * poll_interval;
   /* we are at the tick callback - so no need to do inter-tick adjustments
    * so we can fasttrack this as t -= last_tick in theory is 0.0 (though
    * in practice it will be a very very very small value. also the tick
    * callback will adjust the timer interval at the end anyway */
   if (at_tick)
     {
        if (!timer)
          timer = ecore_timer_add(interval, _ecore_poller_cb_timer, NULL);
     }
   else
     {
        double t;

        if (!timer)
          timer = ecore_timer_add(interval, _ecore_poller_cb_timer, NULL);
        else
          {
             t = ecore_loop_time_get();
             if (!EINA_DBL_EQ(interval, poll_cur_interval))
               {
                  t -= last_tick; /* time since we last ticked */
     /* delete the timer and reset it to tick off in the new
      * time interval. at the tick this will be adjusted */
                  ecore_timer_del(timer);
                  timer = ecore_timer_loop_add(interval - t,
                                               _ecore_poller_cb_timer, NULL);
               }
          }
     }
   poll_cur_interval = interval;
}

/**
 * @brief Callback function for the main poller timer.
 *
 * This function is executed at regular intervals determined by
 * `_ecore_poller_next_tick_eval`. It iterates through poller counters,
 * and for those counters that have "ticked over", it executes
 * the associated poller callback functions. It also handles deletion
 * of pollers marked with `delete_me` and re-evaluates the timer
 * interval if pollers were added or removed.
 *
 * @param data Unused.
 * @return ECORE_CALLBACK_RENEW to continue the timer, or ECORE_CALLBACK_CANCEL if the timer was deleted.
 */
static Eina_Bool
_ecore_poller_cb_timer(void *data EINA_UNUSED)
{
   Ecore_Poller *poller;
   int i;
   int changes = 0;

   at_tick++;
   last_tick = ecore_loop_time_get();
   /* we have 16 counters - each increments every time the poller counter
    * "ticks". it increments by the minimum interval (which can be 1, 2, 4,
    * 7, 16 etc. up to 32768) */
   for (i = 0; i < 15; i++)
     {
        poller_counters[i] += interval_incr;
        /* wrap back to 0 if we exceed out loop count for the counter */
        if (poller_counters[i] >= (1 << i)) poller_counters[i] = 0;
     }

   just_added_poller = 0;
   /* walk the pollers now */
   poller_walking++;
   for (i = 0; i < 15; i++)
     {
        /* if the counter is @ 0 - this means that counter "went off" this
         * tick interval, so run all pollers hooked to that counter */
          if (poller_counters[i] == 0)
            {
               EINA_INLIST_FOREACH(pollers[i], poller)
                 {
                    if (!poller->delete_me)
                      {
                         if (!poller->func(poller->data))
                           {
                              if (!poller->delete_me)
                                {
                                   poller->delete_me = 1;
                                   poller_delete_count++;
                                }
                           }
                      }
                 }
            }
     }
   poller_walking--;

   /* handle deletes afterwards */
   if (poller_delete_count > 0)
     {
        /* FIXME: walk all pollers and remove deleted ones */
         for (i = 0; i < 15; i++)
           {
              Eina_Inlist *l;

              EINA_INLIST_FOREACH_SAFE(pollers[i], l, poller)
                {
                   if (poller->delete_me)
                     {
                        _ecore_poller_cleanup(poller);

                        poller_delete_count--;
                        changes++;
                        if (poller_delete_count <= 0) break;
                     }
                }
              if (poller_delete_count <= 0) break;
           }
     }
   /* if we deleted or added any pollers, then we need to re-evaluate our
    * minimum poll interval */
   if ((changes > 0) || (just_added_poller > 0))
     _ecore_poller_next_tick_eval();

   just_added_poller = 0;
   poller_delete_count = 0;

   at_tick--;

   /* if the timer was deleted then there is no point returning 1 - ambiguous
    * if we do as it implies keep running me" but we have been deleted
    * anyway */
   if (!timer) return ECORE_CALLBACK_CANCEL;

   /* adjust interval */
   ecore_timer_interval_set(timer, poll_cur_interval);
   return ECORE_CALLBACK_RENEW;
}

/**
 * @brief Sets the base poll interval for a poller type.
 * @param type The poller type (currently unused, effectively global).
 * @param poll_time The new base poll interval in seconds. Must be >= 0.0.
 *
 * This function sets the global `poll_interval` which is the fundamental
 * time unit for all pollers. The actual interval for a specific poller
 * will be a multiple of this `poll_time` (specifically `(1 << ibit) * poll_time`).
 * After setting, it re-evaluates the next poller tick.
 */
EAPI void
ecore_poller_poll_interval_set(Ecore_Poller_Type type EINA_UNUSED,
                               double            poll_time)
{
   EINA_MAIN_LOOP_CHECK_RETURN;

   if (poll_time < 0.0)
     {
        ERR("Poll time %f less than zero, ignored", poll_time);
        return;
     }

   poll_interval = poll_time;
   _ecore_poller_next_tick_eval();
}

/**
 * @brief Gets the base poll interval for a poller type.
 * @param type The poller type (currently unused).
 * @return The base poll interval in seconds.
 *
 * This function returns the global `poll_interval`.
 */
EAPI double
ecore_poller_poll_interval_get(Ecore_Poller_Type type EINA_UNUSED)
{
   EINA_MAIN_LOOP_CHECK_RETURN_VAL(0.0);
   return poll_interval;
}

/**
 * @brief Adds a new poller.
 * @param type The poller type (currently unused).
 * @param interval The desired poll interval multiplier. This value must be a
 *                 power of 2 (e.g., 1, 2, 4, 8, ..., 32768). If not a power
 *                 of 2, it will be rounded down to the nearest power of 2.
 *                 The actual poll time will be `interval * ecore_poller_poll_interval_get()`.
 *                 Intervals are capped internally, effectively `1 <= interval <= 32768`.
 * @param func The callback function to call when the poller ticks.
 * @param data User data to pass to the callback function.
 * @return A new poller object on success, @c NULL on failure.
 *
 * The poller will call the given function @p func every @p interval ticks
 * of the base poller interval.
 */
EAPI Ecore_Poller *
ecore_poller_add(Ecore_Poller_Type type EINA_UNUSED,
                 int               interval,
                 Ecore_Task_Cb     func,
                 const void       *data)
{
   Ecore_Poller *poller;
   int ibit;

   poller = calloc(1, sizeof (Ecore_Poller));
   if (!poller) return NULL;

   EINA_MAIN_LOOP_CHECK_RETURN_VAL(NULL);

   if (!func)
     {
        ERR("callback function must be set up for an object of class: '%s'", MY_CLASS_NAME);
        free(poller);
        return NULL;
     }

   /* interval MUST be a power of 2, so enforce it */
   if (interval < 1) interval = 1;
   ibit = -1;
   while (interval != 0)
     {
        ibit++;
        interval >>= 1;
     }
   /* only allow up to 32768 - i.e. ibit == 15, so limit it */
   if (ibit > 15) ibit = 15;

   poller->ibit = ibit;
   poller->func = func;
   poller->data = (void *)data;
   pollers[poller->ibit] = (Ecore_Poller *)eina_inlist_prepend(EINA_INLIST_GET(pollers[poller->ibit]), EINA_INLIST_GET(poller));
   if (poller_walking)
     just_added_poller++;
   else
     _ecore_poller_next_tick_eval();

   return poller;
}

/**
 * @brief Sets the interval for an existing poller.
 * @param poller The poller to modify.
 * @param interval The new interval multiplier. Must be a power of 2.
 *                 See ecore_poller_add() for details on interval.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure or if the
 *         interval is unchanged.
 *
 * This function changes the polling frequency of an already added poller.
 */
EAPI Eina_Bool
ecore_poller_poller_interval_set(Ecore_Poller *poller, int interval)
{
   int ibit;

   if (!poller) return EINA_FALSE;
   EINA_MAIN_LOOP_CHECK_RETURN_VAL(EINA_FALSE);

   /* interval MUST be a power of 2, so enforce it */
   if (interval < 1) interval = 1;
   ibit = -1;
   while (interval != 0)
     {
        ibit++;
        interval >>= 1;
     }
   /* only allow up to 32768 - i.e. ibit == 15, so limit it */
   if (ibit > 15) ibit = 15;
   /* if interval specified is the same as interval set, return true without wasting time */
   if (poller->ibit == ibit) return EINA_TRUE;

   pollers[poller->ibit] = (Ecore_Poller *)eina_inlist_remove(EINA_INLIST_GET(pollers[poller->ibit]), EINA_INLIST_GET(poller));
   poller->ibit = ibit;
   pollers[poller->ibit] = (Ecore_Poller *)eina_inlist_prepend(EINA_INLIST_GET(pollers[poller->ibit]), EINA_INLIST_GET(poller));
   if (poller_walking)
     just_added_poller++;
   else
     _ecore_poller_next_tick_eval();

   return EINA_TRUE;
}

/**
 * @brief Gets the interval of an existing poller.
 * @param poller The poller to query.
 * @return The interval multiplier (a power of 2) of the poller,
 *         or -1 if @p poller is @c NULL.
 *
 * The returned interval is the multiplier used with the base poll interval.
 * For example, if it returns 4, the poller triggers every 4 * `poll_interval` seconds.
 */
EAPI int
ecore_poller_poller_interval_get(const Ecore_Poller *poller)
{
   int ibit, interval = 1;

   if (!poller) return -1;
   EINA_MAIN_LOOP_CHECK_RETURN_VAL(interval);

   ibit = poller->ibit;
   while (ibit != 0)
     {
        ibit--;
        interval <<= 1;
     }

   return interval;
}

/**
 * @brief Deletes a poller.
 * @param poller The poller to delete.
 * @return The data pointer originally associated with the poller,
 *         or @c NULL if @p poller is @c NULL.
 *
 * If called during a poller walk (i.e., from within a poller callback),
 * the poller is marked for deletion and cleaned up later. Otherwise,
 * it's cleaned up immediately.
 */
EAPI void *
ecore_poller_del(Ecore_Poller *poller)
{
   void *data;

   if (!poller) return NULL;
   EINA_MAIN_LOOP_CHECK_RETURN_VAL(NULL);
   /* we are walking the poller list - a bad idea to remove from it while
    * walking it, so just flag it as delete_me and come back to it after
    * the loop has finished */
   if (poller_walking > 0)
     {
        poller_delete_count++;
        poller->delete_me = 1;
        return poller->data;
     }
   /* not in loop so safe - delete immediately */
   data = _ecore_poller_cleanup(poller);

   _ecore_poller_next_tick_eval();

   return data;
}

/**
 * @internal
 * @brief Shuts down the Ecore_Poller system.
 *
 * Deletes all active pollers and the main timer. Resets all global
 * poller state variables to their initial values. This function is
 * typically called during Ecore shutdown.
 */
void
_ecore_poller_shutdown(void)
{
   Ecore_Poller *poller;
   int i;

   for (i = 0; i < 15; i++)
     {
        while ((poller = pollers[i]))
          _ecore_poller_cleanup(poller);
        poller_counters[i] = 0;
     }

   if (timer)
     {
        ecore_timer_del(timer);
        timer = NULL;
     }
   min_interval = -1;
   interval_incr = 0;
   at_tick = 0;
   just_added_poller = 0;
   poller_delete_count = 0;
   poller_walking = 0;
   poll_interval = 0.125;
   poll_cur_interval = 0.0;
   last_tick = 0.0;
}
