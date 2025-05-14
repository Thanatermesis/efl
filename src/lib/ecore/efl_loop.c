#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#define EFL_LOOP_PROTECTED

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <sys/time.h>
#include <errno.h>

#include "Ecore.h"
#include "ecore_private.h"

#include "ecore_main_common.h"

typedef struct _Efl_Loop_Promise_Simple_Data Efl_Loop_Promise_Simple_Data;
typedef struct _Efl_Internal_Promise Efl_Internal_Promise;

/**
 * @internal
 * @brief Structure to hold data for simple promises related to loop operations like timers or idlers.
 *
 * This structure is used to associate a timer or idler with a promise, allowing
 * asynchronous operations to be managed cleanly.
 */
struct _Efl_Loop_Promise_Simple_Data
{
   union {
      Efl_Loop_Timer *timer; /**< Pointer to an Efl_Loop_Timer object, if the promise is for a timer. */
      Ecore_Idler *idler;   /**< Pointer to an Ecore_Idler object, if the promise is for an idler. */
   };
   Eina_Promise *promise; /**< The promise associated with the timer or idler. */
};
GENERIC_ALLOC_SIZE_DECLARE(Efl_Loop_Promise_Simple_Data);

/** @internal @brief Singleton instance of the main loop. */
Eo            *_mainloop_singleton = NULL;
/** @internal @brief Private data for the singleton instance of the main loop. */
Efl_Loop_Data *_mainloop_singleton_data = NULL;

/**
 * @brief Gets the main loop object.
 *
 * This function returns the main loop instance for the application.
 * It is equivalent to efl_app_main_get().
 *
 * @return The main loop object.
 */
EAPI Eo *
efl_main_loop_get(void)
{
   return efl_app_main_get();
}

/**
 * @internal
 * @brief Performs a single iteration of the main loop.
 *
 * This function processes events and timers. It's a wrapper around
 * _ecore_main_loop_iterate.
 *
 * @param obj The Efl_Loop object.
 * @param pd The private data of the Efl_Loop object.
 */
EOLIAN static void
_efl_loop_iterate(Eo *obj, Efl_Loop_Data *pd)
{
   _ecore_main_loop_iterate(obj, pd);
}

/**
 * @internal
 * @brief Performs a single iteration of the main loop, possibly blocking.
 *
 * This function processes events and timers. If @p may_block is non-zero,
 * it may block waiting for events. It's a wrapper around
 * _ecore_main_loop_iterate_may_block.
 *
 * @param obj The Efl_Loop object.
 * @param pd The private data of the Efl_Loop object.
 * @param may_block If 1, the function may block; if 0, it will not.
 * @return The result of the underlying _ecore_main_loop_iterate_may_block call.
 */
EOLIAN static int
_efl_loop_iterate_may_block(Eo *obj, Efl_Loop_Data *pd, int may_block)
{
   return _ecore_main_loop_iterate_may_block(obj, pd, may_block);
}

/**
 * @internal
 * @brief Begins the execution of the main loop.
 *
 * This function starts the main loop. It also handles the termination
 * of child threads if any exist and the loop is configured to quit
 * when the last child thread exits.
 *
 * @param obj The Efl_Loop object.
 * @param pd The private data of the Efl_Loop object.
 * @return A pointer to an Eina_Value containing the exit code of the loop.
 *         The value type is typically EINA_VALUE_TYPE_INT.
 */
EOLIAN static Eina_Value *
_efl_loop_begin(Eo *obj, Efl_Loop_Data *pd)
{
   _ecore_main_loop_begin(obj, pd);
   if (pd->thread_children)
     {
        Eina_List *l, *ll;
        Eo *child;

        // request all child threads to die and defer the quit until
        // the children have all died and returned.
        // run main loop again to clean out children and their exits
        pd->quit_on_last_thread_child_del = EINA_TRUE;
        EINA_LIST_FOREACH_SAFE(pd->thread_children, l, ll, child)
          {
             Efl_Task_Flags task_flags = efl_task_flags_get(child);

             if (task_flags & EFL_TASK_FLAGS_EXIT_WITH_PARENT)
               efl_task_end(child);
             else
               _efl_thread_child_remove(obj, pd, child);
          }
        if (pd->thread_children) _ecore_main_loop_begin(obj, pd);
     }
   return &(pd->exit_code);
}

/**
 * @internal
 * @brief Quits the main loop.
 *
 * This function signals the main loop to terminate its execution.
 * The provided @p exit_code will be set as the loop's exit code.
 *
 * @param obj The Efl_Loop object.
 * @param pd The private data of the Efl_Loop object.
 * @param exit_code The exit code for the loop. This Eina_Value will be
 *                  copied. Example: `eina_value_int_init(0)`.
 */
EOLIAN static void
_efl_loop_quit(Eo *obj, Efl_Loop_Data *pd, Eina_Value exit_code)
{
   _ecore_main_loop_quit(obj, pd);
   pd->exit_code = exit_code;
}

/**
 * @internal
 * @brief Sets the current time of the loop.
 *
 * This is typically used for time-sensitive operations or simulations
 * where the loop's perception of time needs to be controlled.
 *
 * @param obj The Efl_Loop object (unused).
 * @param pd The private data of the Efl_Loop object.
 * @param t The time to set, as a double (e.g., seconds since epoch).
 */
EOLIAN static void
_efl_loop_time_set(Eo *obj EINA_UNUSED, Efl_Loop_Data *pd, double t)
{
   pd->loop_time = t;
}

/**
 * @internal
 * @brief Gets the current time of the loop.
 *
 * @param obj The Efl_Loop object (unused).
 * @param pd The private data of the Efl_Loop object.
 * @return The current time of the loop, as a double.
 */
EOLIAN static double
_efl_loop_time_get(const Eo *obj EINA_UNUSED, Efl_Loop_Data *pd)
{
   return pd->loop_time;
}

/**
 * @brief Exits the main loop with a given exit code.
 *
 * This function is a convenience wrapper around efl_loop_quit(),
 * converting an integer exit code to an Eina_Value.
 *
 * @param exit_code The integer exit code.
 */
EAPI void
efl_exit(int exit_code)
{
   Eina_Value v = EINA_VALUE_EMPTY;

   eina_value_setup(&v, EINA_VALUE_TYPE_INT);
   eina_value_set(&v, exit_code);
   efl_loop_quit(efl_main_loop_get(), v);
}

/**
 * @brief Processes an Eina_Value representing an exit code into an integer.
 *
 * This function attempts to convert various numeric Eina_Value types
 * (integers, floats) into an integer exit code. If the value is an
 * EINA_VALUE_TYPE_ERROR, it prints the error message to stderr and
 * returns -1. Other non-convertible types are stringified and printed
 * to stdout, returning 0.
 *
 * @param value A pointer to an Eina_Value containing the exit code.
 *              If NULL or if its type is not set, it defaults to an
 *              Eina_Value representing the integer 0.
 *              Example for a successful exit:
 *              `Eina_Value v; eina_value_setup(&v, EINA_VALUE_TYPE_INT); eina_value_set(&v, 0);`
 *              Example for an error:
 *              `Eina_Value v; eina_value_error_set(&v, ENOMEM);`
 * @return The processed integer exit code. Returns -1 on conversion
 *         failure for numeric types or if the input value was an error type.
 */
EAPI int
efl_loop_exit_code_process(Eina_Value *value)
{
   Eina_Value def = EINA_VALUE_EMPTY;
   const Eina_Value_Type *t;
   int r = 0;

   if (value == NULL ||
       !value->type)
     {
        def = eina_value_int_init(0);
        value = &def;
     }

   t = eina_value_type_get(value);

   if (t == EINA_VALUE_TYPE_UCHAR ||
       t == EINA_VALUE_TYPE_USHORT ||
       t == EINA_VALUE_TYPE_UINT ||
       t == EINA_VALUE_TYPE_ULONG ||
       t == EINA_VALUE_TYPE_UINT64 ||
       t == EINA_VALUE_TYPE_CHAR ||
       t == EINA_VALUE_TYPE_SHORT ||
       t == EINA_VALUE_TYPE_INT ||
       t == EINA_VALUE_TYPE_LONG ||
       t == EINA_VALUE_TYPE_INT64 ||
       t == EINA_VALUE_TYPE_FLOAT ||
       t == EINA_VALUE_TYPE_DOUBLE)
     {
        Eina_Value v = EINA_VALUE_EMPTY;

        eina_value_setup(&v, EINA_VALUE_TYPE_INT);
        if (!eina_value_convert(value, &v)) r = -1;
        else
          {
             if (!eina_value_get(&v, &r))
               r = -1;
          }
     }
   else
     {
        FILE *out = stdout;
        char *msg;

        msg = eina_value_to_string(value);

        if (t == EINA_VALUE_TYPE_ERROR)
          {
             r = -1;
             out = stderr;
          }
        fprintf(out, "%s\n", msg);
        free(msg);
     }
   return r;
}

/**
 * @internal
 * @brief Callback function to trigger a poll event on the parent loop.
 *
 * This function is called, for example, by timer ticks that simulate
 * poller events (POLL_HIGH, POLL_MEDIUM, POLL_LOW). It retrieves the
 * parent of the event object (which is the timer) and calls the
 * appropriate event callback on that parent (the main loop).
 *
 * @param data The event description (e.g., EFL_LOOP_EVENT_POLL_HIGH)
 *             to be triggered on the parent.
 * @param event The event that triggered this callback (e.g., a timer tick event).
 */
static void
_poll_trigger(void *data, const Efl_Event *event)
{
   Eo *parent = efl_parent_get(event->object);

   efl_event_callback_call(parent, data, NULL);
}

/**
 * @internal
 * @brief Callback invoked when an event catcher is added to the loop.
 *
 * This function monitors the addition of specific event listeners (IDLE, POLL_HIGH,
 * POLL_MEDIUM, POLL_LOW) to the main loop. It manages internal counters for these
 * listeners and, for poller events, creates or reuses timers that simulate
 * the polling behavior at different frequencies.
 *
 * The `event->info` is expected to be an array of `Efl_Callback_Array_Item_Full`.
 * Example structure of `array[i]`:
 * `{.desc = EFL_LOOP_EVENT_IDLE, .func = my_idle_handler_func}`
 *
 * @param data The private data of the Efl_Loop object (Efl_Loop_Data *pd).
 * @param event The EFL_EVENT_CALLBACK_ADD event. The event->info contains
 *              an array of Efl_Callback_Array_Item_Full describing the
 *              callbacks being added.
 */
static void
_check_event_catcher_add(void *data, const Efl_Event *event)
{
   const Efl_Callback_Array_Item_Full *array = event->info;
   Efl_Loop_Data *pd = data;
   int i;

   for (i = 0; array[i].desc != NULL; i++)
     {
        if (array[i].desc == EFL_LOOP_EVENT_IDLE)
          {
             ++pd->idlers;
          }
        // XXX: all the below are kind of bad. ecore_pollers were special.
        // they all woke up at the SAME time based on interval, (all pollers
        // of interval 1 woke up together, those with 2 woke up when 1 and
        // 2 woke up, 4 woke up together along with 1 and 2 etc.
        // the below means they will just go off whenever but at a pre
        // defined interval - 1/60th, 6 and 66 seconds. not really great
        // pollers probably should be less frequent that 1/60th even on poll
        // high, medium probably down to 1-2 sec and low - yes maybe 30 or 60
        // sec... still - not timed to wake up together. :(
        else if (array[i].desc == EFL_LOOP_EVENT_POLL_HIGH)
          {
             if (!pd->poll_high)
               {
                  // Would be better to have it in sync with normal wake up
                  // of the main loop for better energy efficiency, I guess.
                  pd->poll_high = efl_add
                    (EFL_LOOP_TIMER_CLASS, event->object,
                     efl_event_callback_add(efl_added,
                                            EFL_LOOP_TIMER_EVENT_TIMER_TICK,
                                            _poll_trigger,
                                            EFL_LOOP_EVENT_POLL_HIGH),
                     efl_loop_timer_interval_set(efl_added, 1.0 / 60.0));
               }
             ++pd->pollers.high;
          }
        else if (array[i].desc == EFL_LOOP_EVENT_POLL_MEDIUM)
          {
             if (!pd->poll_medium)
               {
                  pd->poll_medium = efl_add
                    (EFL_LOOP_TIMER_CLASS, event->object,
                     efl_event_callback_add(efl_added,
                                            EFL_LOOP_TIMER_EVENT_TIMER_TICK,
                                            _poll_trigger,
                                            EFL_LOOP_EVENT_POLL_MEDIUM),
                     efl_loop_timer_interval_set(efl_added, 6));
               }
             ++pd->pollers.medium;
          }
        else if (array[i].desc == EFL_LOOP_EVENT_POLL_LOW)
          {
             if (!pd->poll_low)
               {
                  pd->poll_low = efl_add
                    (EFL_LOOP_TIMER_CLASS, event->object,
                     efl_event_callback_add(efl_added,
                                            EFL_LOOP_TIMER_EVENT_TIMER_TICK,
                                            _poll_trigger,
                                            EFL_LOOP_EVENT_POLL_LOW),
                     efl_loop_timer_interval_set(efl_added, 66));
               }
             ++pd->pollers.low;
          }
     }
}

/**
 * @internal
 * @brief Callback invoked when an event catcher is removed from the loop.
 *
 * This function mirrors _check_event_catcher_add. It monitors the removal
 * of IDLE and POLL_* event listeners. It decrements internal counters and,
 * for poller events, deletes the associated simulation timers if no listeners
 * for that poll level remain.
 *
 * The `event->info` is expected to be an array of `Efl_Callback_Array_Item_Full`.
 * Example structure of `array[i]`:
 * `{.desc = EFL_LOOP_EVENT_IDLE, .func = my_idle_handler_func}`
 *
 * @param data The private data of the Efl_Loop object (Efl_Loop_Data *pd).
 * @param event The EFL_EVENT_CALLBACK_DEL event. The event->info contains
 *              an array of Efl_Callback_Array_Item_Full describing the
 *              callbacks being removed.
 */
static void
_check_event_catcher_del(void *data, const Efl_Event *event)
{
   const Efl_Callback_Array_Item_Full *array = event->info;
   Efl_Loop_Data *pd = data;
   int i;

   for (i = 0; array[i].desc != NULL; i++)
     {
        if (array[i].desc == EFL_LOOP_EVENT_IDLE)
          {
             --pd->idlers;
          }
        else if (array[i].desc == EFL_LOOP_EVENT_POLL_HIGH)
          {
             --pd->pollers.high;
             if (!pd->pollers.high)
               {
                  efl_del(pd->poll_high);
                  pd->poll_high = NULL;
               }
          }
        else if (array[i].desc == EFL_LOOP_EVENT_POLL_MEDIUM)
          {
             --pd->pollers.medium;
             if (!pd->pollers.medium)
               {
                  efl_del(pd->poll_medium);
                  pd->poll_medium = NULL;
               }
          }
        else if (array[i].desc == EFL_LOOP_EVENT_POLL_LOW)
          {
             --pd->pollers.low;
             if (!pd->pollers.low)
               {
                  efl_del(pd->poll_low);
                  pd->poll_low = NULL;
               }
          }
     }
}

EFL_CALLBACKS_ARRAY_DEFINE(event_catcher_watch,
                          { EFL_EVENT_CALLBACK_ADD, _check_event_catcher_add },
                          { EFL_EVENT_CALLBACK_DEL, _check_event_catcher_del });

/**
 * @internal
 * @brief Constructor for Efl_Loop objects.
 *
 * Initializes the Efl_Loop object, sets up event catcher watchers,
 * initializes loop time, and registers a future message handler.
 *
 * @param obj The Eo object to construct.
 * @param pd The private data for the Efl_Loop object.
 * @return The constructed Efl_Object, or NULL on failure.
 */
EOLIAN static Efl_Object *
_efl_loop_efl_object_constructor(Eo *obj, Efl_Loop_Data *pd)
{
   obj = efl_constructor(efl_super(obj, EFL_LOOP_CLASS));
   if (!obj) return NULL;

   efl_event_callback_array_add(obj, event_catcher_watch(), pd);

   pd->loop_time = ecore_time_get();
   pd->epoll_fd = -1;
   pd->timer_fd = -1;
   pd->future_message_handler = efl_add(EFL_LOOP_MESSAGE_FUTURE_HANDLER_CLASS, obj);
   efl_provider_register(obj, EFL_LOOP_MESSAGE_FUTURE_HANDLER_CLASS, pd->future_message_handler);

   return obj;
}

/**
 * @internal
 * @brief Invalidation logic for Efl_Loop objects.
 *
 * Called when the object is being invalidated. It cleans up main loop content,
 * releases poller timers, and clears the main loop singleton if this object
 * was the singleton.
 *
 * @param obj The Efl_Loop object being invalidated.
 * @param pd The private data of the Efl_Loop object.
 */
EOLIAN static void
_efl_loop_efl_object_invalidate(Eo *obj, Efl_Loop_Data *pd)
{
   efl_invalidate(efl_super(obj, EFL_LOOP_CLASS));

   _ecore_main_content_clear(obj, pd);

   pd->poll_low = NULL;
   pd->poll_medium = NULL;
   pd->poll_high = NULL;

   // After invalidate, it won't be possible to parent to the singleton anymore
   if (obj == _mainloop_singleton)
     {
        _mainloop_singleton = NULL;
        _mainloop_singleton_data = NULL;
     }
}

/**
 * @internal
 * @brief Destructor for Efl_Loop objects.
 *
 * Cleans up resources held by the Efl_Loop object, such as the
 * future message handler and any remaining child threads.
 *
 * @param obj The Efl_Loop object being destructed.
 * @param pd The private data of the Efl_Loop object.
 */
EOLIAN static void
_efl_loop_efl_object_destructor(Eo *obj, Efl_Loop_Data *pd)
{
   pd->future_message_handler = NULL;
   while (pd->thread_children)
     _efl_thread_child_remove(obj, pd, pd->thread_children->data);
   efl_destructor(efl_super(obj, EFL_LOOP_CLASS));
}

/**
 * @internal
 * @brief Sends command line arguments as an EFL_LOOP_EVENT_ARGUMENTS event.
 *
 * This function is typically called once after a job completion in the main loop,
 * effectively deferring the argument processing until the loop is running.
 * It constructs an Efl_Loop_Arguments structure and dispatches it.
 * The `initialization` flag ensures that the `Efl_Loop_Arguments::initialization`
 * field is true only for the first call.
 *
 * @param o The Eo object (unused, typically the main loop).
 * @param data An Eina_Array* containing Eina_Stringshare* elements,
 *             representing the command line arguments.
 *             Example: `arga` might contain `["/usr/bin/my_app", "--option", "value"]`
 * @param v The Eina_Value from the preceding future (typically EINA_VALUE_EMPTY).
 * @return The input Eina_Value @p v.
 */
static Eina_Value
_efl_loop_arguments_send(Eo *o EINA_UNUSED, void *data, const Eina_Value v)

{
   static Eina_Bool initialization = EINA_TRUE;
   Efl_Loop_Arguments arge;
   Eina_Array *arga = data;

   arge.argv = arga;
   arge.initialization = initialization;
   initialization = EINA_FALSE;

   efl_event_callback_call(efl_main_loop_get(),
                           EFL_LOOP_EVENT_ARGUMENTS, &arge);
   return v;
}

/**
 * @internal
 * @brief Cleans up the Eina_Array used to store command line arguments.
 *
 * This function is used as a cleanup callback for a future. It iterates
 * through the Eina_Array, deleting each Eina_Stringshare argument, and
 * then frees the array itself.
 *
 * @param o The Eo object (unused).
 * @param data An Eina_Array* that was used to store command line arguments.
 *             Each element is an Eina_Stringshare*.
 * @param dead_future The future that has completed (unused).
 */
static void
_efl_loop_arguments_cleanup(Eo *o EINA_UNUSED, void *data, const Eina_Future *dead_future EINA_UNUSED)
{
   Eina_Array *arga = data;
   Eina_Stringshare *s;

   while ((s = eina_array_pop(arga))) eina_stringshare_del(s);
   eina_array_free(arga);
}

// It doesn't make sense to send those argument to any other mainloop
// As it also doesn't make sense to allow anyone to override this, so
// should be internal for sure, not even protected.
/**
 * @brief Sends command line arguments to the main loop.
 * @ingroup Ecore_Main_Loop_Group
 *
 * This function processes the command line arguments (argc, argv) provided
 * to the application. It stores them and schedules them to be sent as an
 * EFL_LOOP_EVENT_ARGUMENTS event via a job in the main loop.
 * It also sets the command line arguments for the main application object.
 *
 * @param argc The argument count, similar to main().
 * @param argv The argument vector, similar to main().
 *             Example: `argv` could be `{"my_app", "-v", NULL}` for `argc = 2`.
 */
EAPI void
ecore_loop_arguments_send(int argc, const char **argv)
{
   Eina_Array *arga, *cml;
   int i = 0;

   arga = eina_array_new(argc);
   cml = eina_array_new(argc);
   for (i = 0; i < argc; i++)
     {
        Eina_Stringshare *arg;

        arg = eina_stringshare_add(argv[i]);
        eina_array_push(arga, arg);
        arg = eina_stringshare_add(argv[i]);
        eina_array_push(cml, arg);
     }

   efl_core_command_line_command_array_set(efl_app_main_get(), cml);
   efl_future_then(efl_main_loop_get(), efl_loop_job(efl_main_loop_get()),
                   .success = _efl_loop_arguments_send,
                   .free = _efl_loop_arguments_cleanup,
                   .data = arga);
}

/**
 * @internal
 * @brief Creates a future that resolves in the next main loop iteration (job).
 *
 * This function returns a future that will be resolved when the main loop
 * processes its next batch of jobs. It's essentially a way to schedule
 * a callback for the immediate future within the loop's execution cycle.
 * The future is bound to the lifetime of the loop object @p obj.
 *
 * @param obj The Efl_Loop object.
 * @param pd The private data of the Efl_Loop object (unused).
 * @return A new Eina_Future that resolves with EINA_VALUE_EMPTY.
 */
static Eina_Future *
_efl_loop_job(Eo *obj, Efl_Loop_Data *pd EINA_UNUSED)
{
   // NOTE: Eolian should do efl_future_then() to bind future to object.
   return efl_future_then(obj,
                          eina_future_resolved(efl_loop_future_scheduler_get(obj), EINA_VALUE_EMPTY));
}

/**
 * @internal
 * @brief Sets the throttle amount for the main loop.
 *
 * Throttling can be used to introduce a delay or limit the processing rate
 * of the main loop, often for power-saving or to reduce CPU usage.
 * The amount is specified in seconds but stored internally in microseconds.
 *
 * @param obj The Efl_Loop object (unused).
 * @param pd The private data of the Efl_Loop object.
 * @param amount The throttle amount in seconds (e.g., 0.01 for 10ms).
 */
EOLIAN static void
_efl_loop_throttle_set(Eo *obj EINA_UNUSED, Efl_Loop_Data *pd, double amount)
{
   pd->throttle = ((double)amount) * 1000000.0;
}

/**
 * @internal
 * @brief Gets the current throttle amount of the main loop.
 *
 * @param obj The Efl_Loop object (unused).
 * @param pd The private data of the Efl_Loop object.
 * @return The throttle amount in seconds.
 */
EOLIAN static double
_efl_loop_throttle_get(const Eo *obj EINA_UNUSED, Efl_Loop_Data *pd)
{
   return (double)(pd->throttle) / 1000000.0;
}

/**
 * @internal
 * @brief Cancellation callback for an idle promise.
 *
 * This function is called if the promise associated with an idle event
 * is cancelled before the idler triggers. It deletes the Ecore_Idler
 * and frees the associated promise data.
 *
 * @param data Pointer to Efl_Loop_Promise_Simple_Data containing the idler and promise.
 * @param dead_ptr The promise that was cancelled (unused).
 */
static void
_efl_loop_idle_cancel(void *data, const Eina_Promise *dead_ptr EINA_UNUSED)
{
   Efl_Loop_Promise_Simple_Data *d = data;

   ecore_idler_del(d->idler);
   d->idler = NULL;
   d->promise = NULL;
   efl_loop_promise_simple_data_mp_free(d);
}

/**
 * @internal
 * @brief Callback function executed when an idler triggers.
 *
 * This function is called by Ecore when the idler conditions are met.
 * It resolves the associated promise with an empty value and cleans up
 * the Efl_Loop_Promise_Simple_Data.
 *
 * @param data Pointer to Efl_Loop_Promise_Simple_Data.
 * @return EINA_FALSE to indicate the idler should not run again (it's a one-shot).
 */
static Eina_Bool
_efl_loop_idle_done(void *data)
{
   Efl_Loop_Promise_Simple_Data *d = data;
   eina_promise_resolve(d->promise, EINA_VALUE_EMPTY);
   d->idler = NULL;
   d->promise = NULL;
   efl_loop_promise_simple_data_mp_free(d);
   return EINA_FALSE;
}

/**
 * @internal
 * @brief Creates a future that resolves when the main loop becomes idle.
 *
 * This function schedules an Ecore_Idler. When the idler runs (i.e., the loop
 * has processed other events and is now idle), the returned future is resolved.
 * The future is bound to the lifetime of the loop object @p obj.
 *
 * @param obj The Efl_Loop object.
 * @param pd The private data of the Efl_Loop object (unused).
 * @return A new Eina_Future that resolves with EINA_VALUE_EMPTY when the loop is idle,
 *         or NULL on allocation failure.
 */
static Eina_Future *
_efl_loop_idle(Eo *obj, Efl_Loop_Data *pd EINA_UNUSED)
{
   Efl_Loop_Promise_Simple_Data *d;
   Eina_Promise *p;
   Eina_Future_Scheduler *sched = efl_loop_future_scheduler_get(obj);

   d = efl_loop_promise_simple_data_calloc(1);
   EINA_SAFETY_ON_NULL_RETURN_VAL(d, NULL);

   d->idler = ecore_idler_add(_efl_loop_idle_done, d);
   EINA_SAFETY_ON_NULL_GOTO(d->idler, idler_error);

   p = eina_promise_new(sched, _efl_loop_idle_cancel, d);
   // d is dead if p is NULL
   EINA_SAFETY_ON_NULL_RETURN_VAL(p, NULL);
   d->promise = p;

   // NOTE: Eolian should do efl_future_then() to bind future to object.
   return efl_future_then(obj, eina_future_new(p));

idler_error:
   d->idler = NULL;
   d->promise = NULL;
   efl_loop_promise_simple_data_mp_free(d);
   return NULL;
}

/**
 * @internal
 * @brief Cancellation callback for a timeout promise.
 *
 * This function is called if the promise associated with a timeout event
 * is cancelled before the timer fires. It deletes the Efl_Loop_Timer
 * if it still exists. The associated Efl_Loop_Promise_Simple_Data will be
 * freed when the timer object itself is deleted (see _efl_loop_timeout_del).
 *
 * @param data Pointer to Efl_Loop_Promise_Simple_Data containing the timer and promise.
 * @param dead_ptr The promise that was cancelled (unused).
 */
static void
_efl_loop_timeout_cancel(void *data, const Eina_Promise *dead_ptr EINA_UNUSED)
{
   Efl_Loop_Promise_Simple_Data *d = data;

   if (d->timer)
     efl_del(d->timer);
}

/**
 * @internal
 * @brief Callback function executed when a timer fires (timeout occurs).
 *
 * This function is called when the Efl_Loop_Timer associated with a timeout
 * future emits its EFL_LOOP_TIMER_EVENT_TIMER_TICK event. It resolves the
 * associated promise with an empty value. The timer object itself is then
 * deleted, which will trigger _efl_loop_timeout_del for cleanup.
 *
 * @param data Pointer to Efl_Loop_Promise_Simple_Data.
 * @param event The EFL_LOOP_TIMER_EVENT_TIMER_TICK event from the timer.
 */
static void
_efl_loop_timeout_done(void *data, const Efl_Event *event)
{
   Efl_Loop_Promise_Simple_Data *d = data;

   eina_promise_resolve(d->promise, EINA_VALUE_EMPTY);
   d->timer = NULL;
   efl_del(event->object);
}

/**
 * @internal
 * @brief Callback function executed when the timer object for a timeout is deleted.
 *
 * This function is called when the Efl_Loop_Timer (created for a timeout future)
 * is deleted (e.g., after it fires or is cancelled). It cleans up the
 * Efl_Loop_Promise_Simple_Data.
 *
 * @param data Pointer to Efl_Loop_Promise_Simple_Data.
 * @param event The EFL_EVENT_DEL event from the timer (unused).
 */
static void
_efl_loop_timeout_del(void *data, const Efl_Event *event EINA_UNUSED)
{
   Efl_Loop_Promise_Simple_Data *d = data;

   d->timer = NULL;
   d->promise = NULL;
   efl_loop_promise_simple_data_mp_free(d);
}

/**
 * @internal
 * @brief Creates a future that resolves after a specified timeout.
 *
 * This function creates an Efl_Loop_Timer that will fire after @p tim seconds.
 * When the timer fires, the returned future is resolved. The future is bound
 * to the lifetime of the loop object @p obj.
 *
 * @param obj The Efl_Loop object, which will be the parent of the timer.
 * @param pd The private data of the Efl_Loop object (unused).
 * @param tim The timeout duration in seconds. Example: `1.5` for 1.5 seconds.
 * @return A new Eina_Future that resolves with EINA_VALUE_EMPTY after the timeout,
 *         or NULL on allocation failure.
 */
static Eina_Future *
_efl_loop_timeout(Eo *obj, Efl_Loop_Data *pd EINA_UNUSED, double tim)
{
   Efl_Loop_Promise_Simple_Data *d;
   Eina_Promise *p;
   Eina_Future_Scheduler *sched = efl_loop_future_scheduler_get(obj);

   d = efl_loop_promise_simple_data_calloc(1);
   EINA_SAFETY_ON_NULL_RETURN_VAL(d, NULL);

   d->timer = efl_add(EFL_LOOP_TIMER_CLASS, obj,
                      efl_loop_timer_interval_set(efl_added, tim),
                      efl_event_callback_add(efl_added,
                                             EFL_LOOP_TIMER_EVENT_TIMER_TICK,
                                             _efl_loop_timeout_done, d),
                      efl_event_callback_add(efl_added,
                                             EFL_EVENT_DEL,
                                             _efl_loop_timeout_del, d)
                     );
   EINA_SAFETY_ON_NULL_GOTO(d->timer, timer_error);

   p = eina_promise_new(sched, _efl_loop_timeout_cancel, d);
   // d is dead if p is NULL
   EINA_SAFETY_ON_NULL_RETURN_VAL(p, NULL);
   d->promise = p;

   // NOTE: Eolian should do efl_future_then() to bind future to object.
   return efl_future_then(obj, eina_future_new(p));

timer_error:
   d->timer = NULL;
   d->promise = NULL;
   efl_loop_promise_simple_data_mp_free(d);
   return NULL;
}

/**
 * @internal
 * @brief Registers a provider for a given class on the loop object.
 *
 * This is a wrapper around efl_provider_register.
 *
 * @param obj The Efl_Loop object.
 * @param pd The private data of the Efl_Loop object (unused).
 * @param klass The Efl_Class to register a provider for.
 * @param provider The Efl_Object that provides the implementation for @p klass.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_efl_loop_register(Eo *obj, Efl_Loop_Data *pd EINA_UNUSED,
                   const Efl_Class *klass, const Efl_Object *provider)
{
   return efl_provider_register(obj, klass, provider);
}

EFL_FUNC_BODYV(efl_loop_register, Eina_Bool, EINA_FALSE, EFL_FUNC_CALL(klass, provider), const Efl_Class *klass, const Efl_Object *provider);

/**
 * @internal
 * @brief Unregisters a provider for a given class on the loop object.
 *
 * This is a wrapper around efl_provider_unregister.
 *
 * @param obj The Efl_Loop object.
 * @param pd The private data of the Efl_Loop object (unused).
 * @param klass The Efl_Class to unregister the provider for.
 * @param provider The Efl_Object that was providing the implementation.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_efl_loop_unregister(Eo *obj, Efl_Loop_Data *pd EINA_UNUSED,
                     const Efl_Class *klass, const Efl_Object *provider)
{
   return efl_provider_unregister(obj, klass, provider);
}

EFL_FUNC_BODYV(efl_loop_unregister, Eina_Bool, EINA_FALSE, EFL_FUNC_CALL(klass, provider), const Efl_Class *klass, const Efl_Object *provider);

/**
 * @internal
 * @brief Filters messages in the loop's message queue.
 *
 * Iterates through the current message queue and applies event filters
 * (via _ecore_event_do_filter). If a filter indicates a message should be
 * dropped, the message is marked for deletion. This function increments
 * `pd->message_walking` to prevent modification of the queue during iteration.
 *
 * @param obj The Efl_Loop object (unused).
 * @param pd The private data of the Efl_Loop object, containing the message queue.
 * @param handler_pd Data passed to the filter function (_ecore_event_do_filter).
 */
void
_efl_loop_messages_filter(Eo *obj EINA_UNUSED, Efl_Loop_Data *pd, void *handler_pd)
{
   Message *msg;

   pd->message_walking++;
   EINA_INLIST_FOREACH(pd->message_queue, msg)
     {
        if ((msg->handler) && (msg->message) && (!msg->delete_me))
          {
             if (!_ecore_event_do_filter(handler_pd,
                                         msg->handler, msg->message))
               {
                  efl_del(msg->message);
                  msg->handler = NULL;
                  msg->message = NULL;
                  msg->delete_me = EINA_TRUE;
               }
          }
     }
   pd->message_walking--;
}

/**
 * @internal
 * @brief Calls a function for each message in the loop's message queue.
 *
 * Iterates through the current message queue. For each valid message,
 * it calls the provided function @p func. If @p func returns EINA_FALSE,
 * the message is marked for deletion. This function increments
 * `pd->message_walking` to prevent modification of the queue during iteration.
 * This is used by Ecore event system for generic processing of queued events.
 *
 * @param obj The Efl_Loop object (unused).
 * @param pd The private data of the Efl_Loop object, containing the message queue.
 * @param func A function pointer of type `Eina_Bool (*)(void *data, void *handler, void *msg)`
 *             to be called for each message.
 * @param data Custom data to be passed as the first argument to @p func.
 */
void
_efl_loop_messages_call(Eo *obj EINA_UNUSED, Efl_Loop_Data *pd, void *func, void *data)
{
   Message *msg;

   pd->message_walking++;
   EINA_INLIST_FOREACH(pd->message_queue, msg)
     {
        if ((msg->handler) && (msg->message) && (!msg->delete_me))
          {
             Eina_Bool (*fn) (void *data, void *handler, void *msg);

             fn = func;
             if (!fn(data, msg->handler, msg->message))
               {
                  efl_del(msg->message);
                  msg->handler = NULL;
                  msg->message = NULL;
                  msg->delete_me = EINA_TRUE;
               }
          }
     }
   pd->message_walking--;
}

/**
 * @internal
 * @brief Processes all messages in the main loop's queue.
 *
 * This function is the core of message dispatching in the Efl_Loop.
 * It first applies filters to the message queue. Then, it iterates through
 * the queue, calling the appropriate message handler for each message
 * that has not been marked for deletion. After processing, it cleans up
 * deleted messages and moves any pending messages to the main queue.
 * The `pd->message_walking` counter is used to manage nested processing
 * and ensure correct cleanup.
 *
 * @param obj The Efl_Loop object.
 * @param pd The private data of the Efl_Loop object.
 * @return EINA_TRUE if any messages were processed, EINA_FALSE if the
 *         message queue was initially empty.
 */
EOLIAN static Eina_Bool
_efl_loop_message_process(Eo *obj, Efl_Loop_Data *pd)
{
   if (!pd->message_queue) return EINA_FALSE;
   pd->message_walking++;
   _ecore_event_filters_call(obj, pd);
   while (pd->message_queue)
     {
        Message *msg = (Message *)pd->message_queue;
        if (!msg->delete_me)
          efl_loop_message_handler_message_call(msg->handler, msg->message);
        else
          {
             if (msg->message) efl_del(msg->message);
             pd->message_queue =
               eina_inlist_remove(pd->message_queue,
                                  pd->message_queue);
             free(msg);
          }
     }
   pd->message_walking--;
   if (pd->message_walking == 0)
     {
        Message *msg;

        EINA_INLIST_FREE(pd->message_queue, msg)
          {
             if (msg->message)
               {
                  if (!msg->delete_me)
                    ERR("Found unprocessed event msg=%p handler=%p on queue",
                        msg->message, msg->handler);
                  efl_del(msg->message);
               }
             else free(msg);
          }

        while (pd->message_pending_queue)
          {
             msg = (Message *)pd->message_pending_queue;
             pd->message_pending_queue = eina_inlist_remove(pd->message_pending_queue,
                                                            pd->message_pending_queue);
             pd->message_queue = eina_inlist_append(pd->message_queue, EINA_INLIST_GET(msg));
          }
     }
   return EINA_TRUE;
}

EOAPI EFL_FUNC_BODY(efl_loop_message_process, Eina_Bool, 0);

/**
 * @brief Sets the EFL build version information.
 * @ingroup Ecore_Application_Group
 *
 * This function is typically called very early during application startup,
 * often before full EFL initialization (like eina_init). It stores the
 * provided version details globally. This information can then be retrieved
 * by applications or libraries to check the EFL version they are running against.
 *
 * @param vmaj Major version number.
 * @param vmin Minor version number.
 * @param vmic Micro version number.
 * @param revision Revision number (e.g., from version control).
 * @param flavor A string describing the flavor of the build (e.g., "beta", "nightly").
 *               The string is duplicated. Can be NULL.
 * @param build_id A string identifying the specific build (e.g., a git commit hash).
 *                 The string is duplicated. Can be NULL.
 *
 * @warning This function may be called before eina_init(), so it must not
 *          use Eina functions that require initialization (like eina_stringshare_add).
 *          It uses `strdup` and `free` for string management.
 */
EWAPI void
efl_build_version_set(int vmaj, int vmin, int vmic, int revision,
                      const char *flavor, const char *build_id)
{
   // note: EFL has not been initialized yet at this point (ie. no eina call)
   _app_efl_version.major = vmaj;
   _app_efl_version.minor = vmin;
   _app_efl_version.micro = vmic;
   _app_efl_version.revision = revision;
   free((char *)_app_efl_version.flavor);
   free((char *)_app_efl_version.build_id);
   _app_efl_version.flavor = flavor ? strdup(flavor) : NULL;
   _app_efl_version.build_id = build_id ? strdup(build_id) : NULL;
}

/**
 * @internal
 * @brief Implementation of the Efl_Task run method for the main loop.
 *
 * This function starts the main loop by calling efl_loop_begin() and
 * processes its exit code. It's the entry point for running the loop
 * when it's treated as an Efl_Task.
 *
 * @param obj The Efl_Loop object, acting as an Efl_Task.
 * @param pd The private data of the Efl_Loop object (unused).
 * @return EINA_TRUE, though the actual exit status is determined by the loop's exit code.
 */
EOLIAN static Eina_Bool
_efl_loop_efl_task_run(Eo *obj, Efl_Loop_Data *pd EINA_UNUSED)
{
   efl_loop_exit_code_process(efl_loop_begin(obj));
   return EINA_TRUE;
}

/**
 * @internal
 * @brief Implementation of the Efl_Task end method for the main loop.
 *
 * This function requests the main loop to quit with an exit code of 0.
 * It's called when the task associated with the loop is requested to end.
 *
 * @param obj The Efl_Loop object, acting as an Efl_Task.
 * @param pd The private data of the Efl_Loop object (unused).
 */
EOLIAN static void
_efl_loop_efl_task_end(Eo *obj, Efl_Loop_Data *pd EINA_UNUSED)
{
   efl_loop_quit(obj, eina_value_int_init(0));
}

/**
 * @internal
 * @brief Defines the scheduler events for the Efl_Loop.
 * This is used by efl_event_future_scheduler_get to create a scheduler
 * that ties future resolution to loop idle events.
 * - EFL_LOOP_EVENT_IDLE_ENTER: Event that signals the loop is about to enter an idle state.
 * - EFL_LOOP_EVENT_IDLE: Event that signals the loop is currently idle.
 */
EFL_SCHEDULER_ARRAY_DEFINE(loop_scheduler,
                           EFL_LOOP_EVENT_IDLE_ENTER,
                           EFL_LOOP_EVENT_IDLE);

/**
 * @brief Gets the Eina_Future_Scheduler associated with an Efl_Loop or Efl_Loop_Consumer.
 *
 * This function retrieves a scheduler that allows Eina_Future objects to be
 * processed in synchronization with the main loop's idle events. This is crucial
 * for ensuring that future callbacks are executed at appropriate times within
 * the EFL event lifecycle.
 *
 * If @p obj is an Efl_Loop, it gets the scheduler directly.
 * If @p obj is an Efl_Loop_Consumer, it gets the scheduler from the loop it consumes.
 * If @p obj can provide an Efl_Loop (via efl_provider_find), it uses that loop.
 *
 * @param obj The Efl_Loop, Efl_Loop_Consumer, or an object that can provide an Efl_Loop.
 * @return The Eina_Future_Scheduler for the loop, or NULL if one cannot be found/created.
 */
EAPI Eina_Future_Scheduler *
efl_loop_future_scheduler_get(const Eo *obj)
{
   Efl_Loop *loop;

   if (!obj) return NULL;

   if (efl_isa(obj, EFL_LOOP_CLASS))
     {
        Efl_Loop_Data *pd = efl_data_scope_get(obj, EFL_LOOP_CLASS);

        if (!pd) return NULL;
        return efl_event_future_scheduler_get(obj, loop_scheduler());
     }
   if (efl_isa(obj, EFL_LOOP_CONSUMER_CLASS))
     return efl_loop_future_scheduler_get(efl_loop_get(obj));

   loop = efl_provider_find(obj, EFL_LOOP_CLASS);
   if (loop)
     return efl_loop_future_scheduler_get(loop);

   return NULL;
}

#define EFL_LOOP_EXTRA_OPS                                              \
  EFL_OBJECT_OP_FUNC(efl_loop_message_process, _efl_loop_message_process), \
  EFL_OBJECT_OP_FUNC(efl_loop_register, _efl_loop_register),          \
  EFL_OBJECT_OP_FUNC(efl_loop_unregister, _efl_loop_unregister)

#include "efl_loop.eo.c"
