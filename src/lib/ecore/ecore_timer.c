#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>

#include <Eo.h>

#include "Ecore.h"
#include "ecore_private.h"

#define MY_CLASS EFL_LOOP_TIMER_CLASS
#define MY_CLASS_NAME "Efl_Loop_Timer"

#define ECORE_TIMER_CHECK(obj) if (!efl_isa((obj), MY_CLASS)) return

#define EFL_LOOP_TIMER_DATA_GET(o, td) \
   Efl_Loop_Timer_Data *td = efl_data_scope_safe_get(o, EFL_LOOP_TIMER_CLASS)

#define EFL_LOOP_TIMER_DATA_GET_OR_RETURN(o, ptr, ...) \
   EFL_LOOP_TIMER_DATA_GET(o, ptr);                    \
   if (EINA_UNLIKELY(!ptr))                            \
     {                                                 \
        ERR("No data for timer %p", o);                \
        return __VA_ARGS__;                            \
     }

struct _Ecore_Timer_Legacy
{
   Ecore_Task_Cb func;

   const void *data;

   Eina_Bool inside_call : 1;
   Eina_Bool delete_me   : 1; /**< Flag to mark the timer for deletion. */
};
/**
 * @brief Legacy timer structure used for Ecore_Timer compatibility.
 *
 * This structure holds data for timers created using the older ecore_timer_add API.
 * It bridges the legacy API with the new Eo-based timer system.
 */
typedef struct _Ecore_Timer_Legacy Ecore_Timer_Legacy;
struct _Efl_Loop_Timer_Data
{
   EINA_INLIST;

   Eo            *object;
   Eo            *loop;
   Efl_Loop_Data *loop_data;
   Ecore_Timer_Legacy *legacy;

   double     in;
   double     at;
   double     pending;

   int        listening;

   Eina_Bool  just_added  : 1;
   Eina_Bool  frozen      : 1;
   Eina_Bool  initialized : 1;
   Eina_Bool  noparent    : 1;
   Eina_Bool  constructed : 1;
   Eina_Bool  finalized   : 1; /**< Flag indicating the object has been finalized. */
};

/**
 * @brief Delays a timer by a specified amount of time.
 *
 * This utility function adjusts the timer's scheduled execution time.
 * If the timer is frozen, its pending duration is adjusted. Otherwise,
 * its 'at' time is updated.
 *
 * @param timer The timer data structure to modify.
 * @param add The amount of time (in seconds) to add to the timer's delay.
 */
static void _efl_loop_timer_util_delay(Efl_Loop_Timer_Data *timer, double add);
/**
 * @brief (Re-)Inserts a timer into the appropriate list within a loop.
 *
 * This function first removes the timer from any list it might currently be in.
 * Then, based on its state (listening, frozen, time 'at', interval 'in'),
 * it's either added to the 'suspended' list or sorted into the 'timers' list
 * based on its scheduled execution time 'at'.
 *
 * @param loop The loop data where the timer will be managed. Can be NULL if timer is not associated with a loop.
 * @param timer The timer data structure to instanciate.
 */
static void _efl_loop_timer_util_instanciate(Efl_Loop_Data *loop, Efl_Loop_Timer_Data *timer);
/**
 * @brief Sets or updates a timer's properties and re-instanciates it.
 *
 * This function initializes or updates a timer's execution time ('at') and
 * interval ('in'). It marks the timer as 'just_added' and 'initialized'.
 * If the timer is not frozen, its 'at' time is set and 'pending' duration is cleared.
 * Finally, it calls _efl_loop_timer_util_instanciate to place the timer in the
 * correct list within its loop.
 *
 * @param timer The timer data structure to set.
 * @param at The absolute time (in seconds, typically from ecore_time_get()) when the timer should fire.
 * @param in The interval (in seconds) for recurring timers.
 */
static void _efl_loop_timer_set(Efl_Loop_Timer_Data *timer, double at, double in);

/**
 * @brief Default precision for timers.
 *
 * This value (10 microseconds) is used when comparing timer execution times,
 * for example, in _efl_loop_timer_after_get to determine if multiple timers
 * should be considered as firing "at the same time".
 */
static double precision = 10.0 / 1000000.0;

EAPI double
ecore_timer_precision_get(void)
{
   EINA_MAIN_LOOP_CHECK_RETURN_VAL(0.0);
   return precision;
}

EAPI void
ecore_timer_precision_set(double value)
{
   EINA_MAIN_LOOP_CHECK_RETURN;
   if (value < 0.0) value = 0.0;
   precision = value;
}

/**
 * @brief Callback invoked when an event listener is added to a timer object.
 *
 * Specifically, this function checks if the added listener is for the
 * EFL_LOOP_TIMER_EVENT_TIMER_TICK event. If so, it increments the
 * `listening` counter on the timer. If this is the first listener for
 * TIMER_TICK and the timer is already finalized, it re-instanciates
 * the timer to ensure it's correctly scheduled in the loop.
 *
 * @param data Pointer to the Efl_Loop_Timer_Data associated with the timer.
 * @param event The event structure containing details about the added callback.
 *              The `event->info` is expected to be an Efl_Callback_Array_Item_Full array.
 *              Example of array structure:
 *              const Efl_Callback_Array_Item_Full array[] = {
 *                  { EFL_LOOP_TIMER_EVENT_TIMER_TICK, _my_tick_handler_func },
 *                  { NULL, NULL } // Terminator
 *              };
 */
static void
_check_timer_event_catcher_add(void *data, const Efl_Event *event)
{
   Efl_Loop_Timer_Data *timer = data;
   const Efl_Callback_Array_Item_Full *array = event->info;
   int i;

   for (i = 0; array[i].desc != NULL; i++)
     {
        if (array[i].desc == EFL_LOOP_TIMER_EVENT_TIMER_TICK)
          {
             if (timer->listening++ > 0) return;
             if (timer->finalized)
               _efl_loop_timer_util_instanciate(timer->loop_data, timer);
             // No need to walk more than once per array as you can not del
             // a partial array
             return;
          }
     }
}

/**
 * @brief Callback invoked when an event listener is removed from a timer object.
 *
 * This function checks if the removed listener was for the
 * EFL_LOOP_TIMER_EVENT_TIMER_TICK event. If so, it decrements the
 * `listening` counter. If the `listening` count drops to zero (meaning no
 * more listeners for TIMER_TICK), it re-instanciates the timer. This
 * typically moves it to a suspended list if it's no longer being listened to.
 *
 * @param data Pointer to the Efl_Loop_Timer_Data associated with the timer.
 * @param event The event structure containing details about the removed callback.
 *              The `event->info` is expected to be an Efl_Callback_Array_Item_Full array.
 *              Example of array structure:
 *              const Efl_Callback_Array_Item_Full array[] = {
 *                  { EFL_LOOP_TIMER_EVENT_TIMER_TICK, _my_tick_handler_func },
 *                  { NULL, NULL } // Terminator
 *              };
 */
static void
_check_timer_event_catcher_del(void *data, const Efl_Event *event)
{
   Efl_Loop_Timer_Data *timer = data;
   const Efl_Callback_Array_Item_Full *array = event->info;
   int i;

   for (i = 0; array[i].desc != NULL; i++)
     {
        if (array[i].desc == EFL_LOOP_TIMER_EVENT_TIMER_TICK)
          {
             if ((--timer->listening) > 0) return;
             _efl_loop_timer_util_instanciate(timer->loop_data, timer);
             return;
          }
     }
}

EFL_CALLBACKS_ARRAY_DEFINE(timer_watch,
                          { EFL_EVENT_CALLBACK_ADD, _check_timer_event_catcher_add },
                          { EFL_EVENT_CALLBACK_DEL, _check_timer_event_catcher_del });

EOLIAN static Eo *
_efl_loop_timer_efl_object_constructor(Eo *obj, Efl_Loop_Timer_Data *timer)
{
   efl_constructor(efl_super(obj, MY_CLASS));
   efl_event_callback_array_add(obj, timer_watch(), timer);
   efl_wref_add(obj, &timer->object);

   timer->in = -1.0;
   timer->constructed = EINA_TRUE;
   return obj;
}

EOLIAN static Eo *
_efl_loop_timer_efl_object_finalize(Eo *obj, Efl_Loop_Timer_Data *pd)
{
   pd->loop = efl_provider_find(obj, EFL_LOOP_CLASS);
   pd->loop_data = efl_data_scope_get(pd->loop, EFL_LOOP_CLASS);

   if (pd->at < efl_loop_time_get(pd->loop))
     pd->at = ecore_time_get() + pd->in;
   else pd->at += pd->in;

   if (pd->in < 0.0)
     {
        ERR("You need to specify the interval of a timer to create a valid timer.");
        return NULL;
     }
   pd->initialized = EINA_TRUE;
   pd->finalized = EINA_TRUE;
   _efl_loop_timer_set(pd, pd->at, pd->in);
   return efl_finalize(efl_super(obj, MY_CLASS));
}

/**
 * @brief Callback for the EFL_EVENT_DEL event on legacy timers.
 *
 * This function is responsible for freeing the Ecore_Timer_Legacy structure
 * when a legacy timer object is deleted.
 *
 * @param data Pointer to the Ecore_Timer_Legacy structure to be freed.
 * @param event The event structure (unused in this function).
 */
static void
_ecore_timer_legacy_del(void *data, const Efl_Event *event EINA_UNUSED)
{
   free(data);
}

/**
 * @brief Callback for the EFL_LOOP_TIMER_EVENT_TIMER_TICK event on legacy timers.
 *
 * This function is invoked when a legacy timer fires. It calls the user-provided
 * callback function (`legacy->func`).
 * If the user callback returns ECORE_CALLBACK_CANCEL (or EINA_FALSE) or if
 * `legacy->delete_me` is set (e.g., by ecore_timer_del during the callback),
 * the timer is marked for deletion. If not currently inside a recursive call
 * to this tick function, the timer object is deleted immediately.
 *
 * @param data Pointer to the Efl_Loop_Timer_Data of the timer.
 *             The actual legacy data is retrieved from this.
 * @param event The event structure, `event->object` is the timer Eo object.
 */
static void
_ecore_timer_legacy_tick(void *data, const Efl_Event *event)
{
   Ecore_Timer_Legacy *legacy = data;
   Eina_Bool inside_call = legacy->inside_call;

   legacy->inside_call = 1;
   if (!_ecore_call_task_cb(legacy->func, (void *)legacy->data) || legacy->delete_me)
     {
        legacy->delete_me = EINA_TRUE;
        /* cannot destroy timer if recursing */
        if (!inside_call)
          efl_del(event->object);
     }
   /* only unset flag if not currently recursing */
   else if (!inside_call)
     legacy->inside_call = 0;
}

EFL_CALLBACKS_ARRAY_DEFINE(legacy_timer,
                          { EFL_LOOP_TIMER_EVENT_TIMER_TICK, _ecore_timer_legacy_tick },
                          { EFL_EVENT_DEL, _ecore_timer_legacy_del });

EAPI Ecore_Timer *
ecore_timer_add(double in, Ecore_Task_Cb func, const void *data)
{
   Ecore_Timer_Legacy *legacy;
   Eo *timer;

   EINA_MAIN_LOOP_CHECK_RETURN_VAL(NULL);
   if (!func)
     {
        ERR("Callback function must be set up for the class.");
        return NULL;
     }
   legacy = calloc(1, sizeof (Ecore_Timer_Legacy));
   if (!legacy) return NULL;
   legacy->func = func;
   legacy->data = data;
   timer = efl_add(MY_CLASS, efl_main_loop_get(),
                  efl_event_callback_array_add(efl_added, legacy_timer(), legacy),
                  efl_loop_timer_interval_set(efl_added, in));
   EFL_LOOP_TIMER_DATA_GET_OR_RETURN(timer, td, NULL);
   td->legacy = legacy;
   return timer;
}

EAPI Ecore_Timer *
ecore_timer_loop_add(double in, Ecore_Task_Cb func, const void  *data)
{
   Ecore_Timer_Legacy *legacy;
   Eo *timer;

   EINA_MAIN_LOOP_CHECK_RETURN_VAL(NULL);
   if (!func)
     {
        ERR("Callback function must be set up for the class.");
        return NULL;
     }
   legacy = calloc(1, sizeof (Ecore_Timer_Legacy));
   if (!legacy) return NULL;
   legacy->func = func;
   legacy->data = data;
   timer = efl_add(MY_CLASS, efl_main_loop_get(),
                  efl_event_callback_array_add(efl_added, legacy_timer(), legacy),
                  efl_loop_timer_loop_reset(efl_added),
                  efl_loop_timer_interval_set(efl_added, in));
   EFL_LOOP_TIMER_DATA_GET_OR_RETURN(timer, td, NULL);
   td->legacy = legacy;
   return timer;
}

EAPI void *
ecore_timer_del(Ecore_Timer *timer)
{
   void *data;

   if (!timer) return NULL;
   EINA_MAIN_LOOP_CHECK_RETURN_VAL(NULL);

   EFL_LOOP_TIMER_DATA_GET(timer, td);
   // If legacy == NULL, this means double free or something
   if ((!td) || (!td->legacy))
     {
        // Just in case it is an Eo timer, but not a legacy one.
        ERR("You are trying to destroy a timer which seems dead already.");
        efl_unref(timer);
        return NULL;
     }

   data = (void *)td->legacy->data;
   if (td->legacy->inside_call) td->legacy->delete_me = EINA_TRUE;
   else efl_del(timer);
   return data;
}

EOLIAN static void
_efl_loop_timer_timer_interval_set(Eo *obj EINA_UNUSED, Efl_Loop_Timer_Data *timer, double in)
{
   if (in < 0.0) in = 0.0;
   timer->in = in;
}

EOLIAN static double
_efl_loop_timer_timer_interval_get(const Eo *obj EINA_UNUSED, Efl_Loop_Timer_Data *timer)
{
   return timer->in;
}

EOLIAN static void
_efl_loop_timer_timer_delay(Eo *obj EINA_UNUSED, Efl_Loop_Timer_Data *pd, double add)
{
   _efl_loop_timer_util_delay(pd, add);
}

EOLIAN static void
_efl_loop_timer_timer_reset(Eo *obj EINA_UNUSED, Efl_Loop_Timer_Data *timer)
{
   double now, add;

   if (!timer->loop_data) return;
   // Do not reset the current timer while inside the callback
   if (timer->loop_data->timer_current == timer) return;

   now = ecore_time_get();
   if (!timer->initialized)
     {
        timer->at = now;
        return;
     }

   if (timer->frozen) add = timer->pending;
   else add = timer->at - now;
   _efl_loop_timer_util_delay(timer, timer->in - add);
}

EOLIAN static void
_efl_loop_timer_timer_loop_reset(Eo *obj EINA_UNUSED, Efl_Loop_Timer_Data *timer)
{
   double now, add;

   if (!timer->loop_data) return;
   // Do not reset the current timer while inside the callback
   if (timer->loop_data->timer_current == timer) return;

   now = efl_loop_time_get(timer->loop);
   if (!timer->initialized)
     {
        timer->at = now;
        return;
     }

   if (timer->frozen) add = timer->pending;
   else add = timer->at - now;
   _efl_loop_timer_util_delay(timer, timer->in - add);
}

EOLIAN static double
_efl_loop_timer_time_pending_get(const Eo *obj EINA_UNUSED, Efl_Loop_Timer_Data *timer)
{
   double now, ret = 0.0;

   now = ecore_time_get();
   if (timer->frozen) ret = timer->pending;
   else ret = timer->at - now;
   return ret;
}

EAPI void
ecore_timer_freeze(Ecore_Timer *timer)
{
   ECORE_TIMER_CHECK(timer);
   efl_event_freeze(timer);
}

EOLIAN static void
_efl_loop_timer_efl_object_event_freeze(Eo *obj, Efl_Loop_Timer_Data *timer)
{
   double now = 0.0;

   efl_event_freeze(efl_super(obj, MY_CLASS));
   // Timer already frozen
   if (timer->frozen) return;

   /* not set if timer is not finalized */
   if (timer->loop)
     now = efl_loop_time_get(timer->loop);
   /* only if timer interval has been set */
   if (timer->initialized)
     timer->pending = timer->at - now;
   else
     timer->pending = 0.0;
   timer->at = 0.0;
   timer->frozen = 1;

   _efl_loop_timer_util_instanciate(timer->loop_data, timer);
}

EAPI Eina_Bool
ecore_timer_freeze_get(Ecore_Timer *timer)
{
   EINA_MAIN_LOOP_CHECK_RETURN_VAL(EINA_FALSE);
   return !!efl_event_freeze_count_get(timer);
}

EOLIAN static int
_efl_loop_timer_efl_object_event_freeze_count_get(const Eo *obj EINA_UNUSED, Efl_Loop_Timer_Data *timer)
{
   return timer->frozen;
}

EAPI void
ecore_timer_thaw(Ecore_Timer *timer)
{
   ECORE_TIMER_CHECK(timer);
   efl_event_thaw(timer);
}

EOLIAN static void
_efl_loop_timer_efl_object_event_thaw(Eo *obj, Efl_Loop_Timer_Data *timer)
{
   double now;

   efl_event_thaw(efl_super(obj, MY_CLASS));

   if (!timer->frozen) return; // Timer not frozen
   timer->frozen = 0;

   if (timer->loop_data)
     {
        timer->loop_data->suspended = eina_inlist_remove
          (timer->loop_data->suspended, EINA_INLIST_GET(timer));
     }
   now = ecore_time_get();
   _efl_loop_timer_set(timer, timer->pending + now, timer->in);
}

EAPI char *
ecore_timer_dump(void)
{
   return NULL;
}

/**
 * @brief Removes a timer from its current list within the loop data.
 *
 * This utility function ensures a timer is no longer part of the 'timers'
 * or 'suspended' lists in its associated loop_data. It also handles the
 * case where the timer being cleared is the 'timer_current' in the loop_data,
 * advancing 'timer_current' to the next timer to prevent processing issues.
 *
 * @param pd The timer data structure to remove from loop lists.
 */
static void
_efl_loop_timer_util_loop_clear(Efl_Loop_Timer_Data *pd)
{
   Eina_Inlist *first;

   if (!pd->loop_data) return;
   // Check if we are the current timer, if so move along
   if (pd->loop_data->timer_current == pd)
     pd->loop_data->timer_current = (Efl_Loop_Timer_Data *)
       EINA_INLIST_GET(pd)->next;

   // Remove the timer from all possible pending list
   first = eina_inlist_first(EINA_INLIST_GET(pd));
   if (first == pd->loop_data->timers)
     pd->loop_data->timers = eina_inlist_remove
       (pd->loop_data->timers, EINA_INLIST_GET(pd));
   else if (first == pd->loop_data->suspended)
     pd->loop_data->suspended = eina_inlist_remove
       (pd->loop_data->suspended, EINA_INLIST_GET(pd));
}

static void
_efl_loop_timer_util_instanciate(Efl_Loop_Data *loop, Efl_Loop_Timer_Data *timer)
{
   Efl_Loop_Timer_Data *t2;

   if (!loop) return;
   _efl_loop_timer_util_loop_clear(timer);

   // And start putting it back where it belong
   if ((!timer->listening) || (timer->frozen) ||
       (timer->at <= 0.0) || (timer->in < 0.0))
     {
        loop->suspended = eina_inlist_prepend(loop->suspended,
                                              EINA_INLIST_GET(timer));
        return;
     }

   if (!timer->initialized)
     {
        ERR("Trying to instantiate an uninitialized timer is impossible.");
        return;
     }

   EINA_INLIST_REVERSE_FOREACH(loop->timers, t2)
     {
        if (timer->at > t2->at)
          {
             loop->timers = eina_inlist_append_relative(loop->timers,
                                                        EINA_INLIST_GET(timer),
                                                        EINA_INLIST_GET(t2));
             return;
          }
     }
   loop->timers = eina_inlist_prepend(loop->timers, EINA_INLIST_GET(timer));
}

static void
_efl_loop_timer_util_delay(Efl_Loop_Timer_Data *timer, double add)
{
   if (!timer->initialized)
     {
        ERR("Impossible to delay an uninitialized timer.");
        return;
     }
   if (timer->frozen)
     {
        timer->pending += add;
        return;
     }
   _efl_loop_timer_set(timer, timer->at + add, timer->in);
}

EOLIAN static void
_efl_loop_timer_efl_object_parent_set(Eo *obj, Efl_Loop_Timer_Data *pd, Efl_Object *parent)
{
   Eina_Inlist *first;

   efl_parent_set(efl_super(obj, EFL_LOOP_TIMER_CLASS), parent);

   if ((!pd->constructed) || (!pd->finalized)) return;

   // Remove the timer from all possible pending list
   first = eina_inlist_first(EINA_INLIST_GET(pd));
   if (first == pd->loop_data->timers)
     {
        /* if this timer is currently being processed, update the pointer here so it is not lost */
        if (pd == pd->loop_data->timer_current)
          pd->loop_data->timer_current = (Efl_Loop_Timer_Data*)EINA_INLIST_GET(pd)->next;
        pd->loop_data->timers = eina_inlist_remove(pd->loop_data->timers, EINA_INLIST_GET(pd));
     }
   else if (first == pd->loop_data->suspended)
     pd->loop_data->suspended = eina_inlist_remove
       (pd->loop_data->suspended, EINA_INLIST_GET(pd));

   if (efl_invalidated_get(obj)) return;

   pd->loop = efl_provider_find(obj, EFL_LOOP_CLASS);
   if (pd->loop)
     pd->loop_data = efl_data_scope_get(pd->loop, EFL_LOOP_CLASS);
   else
     pd->loop_data = NULL;

   if (efl_parent_get(obj) != parent) return;

   _efl_loop_timer_util_instanciate(pd->loop_data, pd);

   if (parent != NULL) pd->noparent = EINA_FALSE;
   else pd->noparent = EINA_TRUE;
}

EOLIAN static void
_efl_loop_timer_efl_object_destructor(Eo *obj, Efl_Loop_Timer_Data *pd)
{
   _efl_loop_timer_util_loop_clear(pd);
   efl_destructor(efl_super(obj, MY_CLASS));
}

/**
 * @brief Clears the 'just_added' flag for all timers in a loop.
 *
 * This function is called after newly added timers have been processed or
 * integrated into the main timer list. It iterates through all timers
 * in the `pd->timers` list and resets their `just_added` flag to 0.
 * This ensures that these timers are considered for execution in subsequent
 * timer processing cycles. The `pd->timers_added` flag is also reset.
 *
 * @param obj The loop object (unused).
 * @param pd The loop data containing the list of timers.
 */
void
_efl_loop_timer_enable_new(Eo *obj EINA_UNUSED, Efl_Loop_Data *pd)
{
   Efl_Loop_Timer_Data *timer;

   if (!pd->timers_added) return;
   pd->timers_added = 0;
   EINA_INLIST_FOREACH(pd->timers, timer) timer->just_added = 0;
}

/**
 * @brief Checks if there are any active (non-suspended) timers in the loop.
 *
 * @param obj The loop object (unused).
 * @param pd The loop data.
 * @return 1 (EINA_TRUE) if there are timers in the `pd->timers` list, 0 (EINA_FALSE) otherwise.
 */
int
_efl_loop_timers_exists(Eo *obj EINA_UNUSED, Efl_Loop_Data *pd)
{
   return !!pd->timers;
}

/**
 * @brief Gets the first timer in the loop that is not 'just_added'.
 *
 * This function iterates through the `pd->timers` list and returns the
 * Eo object of the first timer it finds whose `just_added` flag is false.
 * Timers marked as 'just_added' are typically those recently created or
 * rescheduled and might not be ready for immediate processing in some contexts
 * (e.g., calculating next sleep time).
 *
 * @param ob The loop object (unused).
 * @param pd The loop data containing the list of timers.
 * @return The Eo object of the first eligible timer, or NULL if no such timer is found.
 */
static inline Ecore_Timer *
_efl_loop_timer_first_get(Eo *ob EINA_UNUSED, Efl_Loop_Data *pd)
{
   Efl_Loop_Timer_Data *timer;

   EINA_INLIST_FOREACH(pd->timers, timer)
     {
        if (!timer->just_added) return timer->object;
     }
   return NULL;
}

/**
 * @brief Finds the last timer in a sequence that should fire "at the same time" as a base timer.
 *
 * Starting from the timer immediately following `base` in its inlist, this function
 * iterates through subsequent timers. It considers timers to be "at the same time"
 * if their scheduled `at` time is within `base->at + precision`.
 * It skips uninitialized timers and timers marked as `just_added`.
 * The function returns the data of the last such timer found (which could be the
 * `base` timer itself if no subsequent timers meet the criteria).
 *
 * This is used to group timers that are scheduled very close together, allowing
 * the main loop to potentially wake up for the latest one in such a group,
 * processing all of them in one go.
 *
 * @param base The base timer data from which to start the search.
 * @return The Efl_Loop_Timer_Data of the last timer considered to be firing
 *         at approximately the same time as or shortly after the base timer.
 */
static inline Efl_Loop_Timer_Data *
_efl_loop_timer_after_get(Efl_Loop_Timer_Data *base)
{
   Efl_Loop_Timer_Data *timer;
   Efl_Loop_Timer_Data *valid_timer = base;
   double maxtime = base->at + precision;

   EINA_INLIST_FOREACH(EINA_INLIST_GET(base)->next, timer)
     {
        if (EINA_UNLIKELY(!timer->initialized)) continue; // This shouldn't happen
        if (timer->at >= maxtime) break;
        if (!timer->just_added) valid_timer = timer;
     }
   return valid_timer;
}

double
_efl_loop_timer_next_get(Eo *obj, Efl_Loop_Data *pd)
{
   Ecore_Timer *object;
   Efl_Loop_Timer_Data *first;
   double now;
   double in;

   object = _efl_loop_timer_first_get(obj, pd);
   if (!object) return -1;

   first = _efl_loop_timer_after_get(efl_data_scope_get(object, MY_CLASS));
   now = efl_loop_time_get(obj);
   in = first->at - now;
   if (in < 0) in = 0;
   return in;
}

/**
 * @brief Reschedules a timer after it has ticked (or if its time was adjusted).
 *
 * This function is called after a timer's callback has been executed or if
 * the system time jumped. It calculates the next firing time for the timer.
 *
 * If the timer is frozen, invalidated, or marked for deletion (for legacy timers),
 * it does nothing.
 *
 * It first ensures the timer is removed from its current list in the loop_data
 * if it's part of one and not a 'noparent' timer.
 *
 * A key piece of logic here is handling system time jumps or long delays:
 * If the timer's next scheduled time (`timer->at + timer->in`) would have been
 * more than 15 seconds in the past relative to `when` (current time),
 * it's assumed a significant hang or suspend occurred. In this case, the timer
 * is rescheduled to fire `timer->in` seconds from `when`.
 * Otherwise, it's rescheduled to `timer->at + timer->in`.
 *
 * Finally, it calls `_efl_loop_timer_set` to update the timer's properties and
 * re-insert it into the loop's timer list.
 *
 * @param timer The timer data to reschedule.
 * @param when The current time, used as a reference for rescheduling.
 */
static inline void
_efl_loop_timer_reschedule(Efl_Loop_Timer_Data *timer, double when)
{
   if (timer->frozen || efl_invalidated_get(timer->object) ||
       (timer->legacy && timer->legacy->delete_me)) return;

   if (timer->loop_data &&
       (EINA_INLIST_GET(timer)->next || EINA_INLIST_GET(timer)->prev))
     {
        if (timer->loop_data->timers && (!timer->noparent))
          timer->loop_data->timers = eina_inlist_remove
            (timer->loop_data->timers, EINA_INLIST_GET(timer));
     }

   /* if the timer would have gone off more than 15 seconds ago,
    * assume that the system hung and set the timer to go off
    * timer->in from now. this handles system hangs, suspends
    * and more, so ecore will only "replay" the timers while
    * the system is suspended if it is suspended for less than
    * 15 seconds (basically). this also handles if the process
    * is stopped in a debugger or IO and other handling gets
    * really slow within the main loop.
    */
   if ((timer->at + timer->in) < (when - 15.0))
     _efl_loop_timer_set(timer, when + timer->in, timer->in);
   else
     _efl_loop_timer_set(timer, timer->at + timer->in, timer->in);
}

void
_efl_loop_timer_expired_timers_call(Eo *obj, Efl_Loop_Data *pd, double when)
{
   // call the first expired timer until no expired timers exist
   while (_efl_loop_timer_expired_call(obj, pd, when));
}

int
_efl_loop_timer_expired_call(Eo *obj EINA_UNUSED, Efl_Loop_Data *pd, double when)
{
   if (!pd->timers) return 0;
   if (pd->last_check > when)
     {
        Efl_Loop_Timer_Data *timer;
        // User set time backwards
        EINA_INLIST_FOREACH(pd->timers, timer)
          timer->at -= (pd->last_check - when);
     }
   pd->last_check = when;

   if (!pd->timer_current) // regular main loop, start from head
     pd->timer_current = (Efl_Loop_Timer_Data *)pd->timers;
   else
     {
        // recursive main loop, continue from where we were
        Efl_Loop_Timer_Data *timer_old = pd->timer_current;

        pd->timer_current = (Efl_Loop_Timer_Data *)
          EINA_INLIST_GET(pd->timer_current)->next;
        _efl_loop_timer_reschedule(timer_old, when);
     }

   while (pd->timer_current)
     {
        Efl_Loop_Timer_Data *timer = pd->timer_current;

        if (timer->at > when)
          {
             pd->timer_current = NULL; // ended walk, next should restart.
             return 0;
          }

        if (timer->just_added)
          {
             pd->timer_current = (Efl_Loop_Timer_Data *)
               EINA_INLIST_GET(pd->timer_current)->next;
             continue;
          }

        efl_ref(timer->object);
        eina_evlog("+timer", timer, 0.0, NULL);
        /* this can remove timer from its inlist in the legacy codepath */
        efl_event_callback_call(timer->object, EFL_LOOP_TIMER_EVENT_TIMER_TICK, NULL);
        eina_evlog("-timer", timer, 0.0, NULL);

        // may have changed in recursive main loops
        // this current timer can not die yet as we hold a reference on it
        /* this is tricky: the current timer cannot be deleted, but it CAN be removed from its inlist,
         * thus breaking timer processing
         */
        if (pd->timer_current)
          {
             if (pd->timer_current == timer)
               pd->timer_current = (Efl_Loop_Timer_Data *)EINA_INLIST_GET(pd->timer_current)->next;
             /* assume this has otherwise been modified either due to recursive mainloop processing or
              * the timer being removed from its inlist and carefully updating pd->timer_current in the
              * process as only the most elite of engineers would think to do
              */
          }
        _efl_loop_timer_reschedule(timer, when);
        efl_unref(timer->object);
     }
   return 0;
}

static void
_efl_loop_timer_set(Efl_Loop_Timer_Data *timer, double at, double in)
{
   if (!timer->loop_data) return;
   timer->loop_data->timers_added = 1;
   timer->in = in;
   timer->just_added = 1;
   timer->initialized = 1;
   if (!timer->frozen)
     {
        timer->at = at;
        timer->pending = 0.0;
     }
   _efl_loop_timer_util_instanciate(timer->loop_data, timer);
}

#include "efl_loop_timer.eo.c"
#include "efl_loop_timer_eo.legacy.c"
