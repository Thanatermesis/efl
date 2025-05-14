/*
 * XSync code
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* ifdef HAVE_CONFIG_H */

#include "Ecore.h"
#include "ecore_x_private.h"
#include "Ecore_X.h"
#include "Ecore_X_Atoms.h"

/**
 * @brief Creates a new X Sync Alarm.
 *
 * This function creates a new X Sync Alarm associated with the given counter.
 * The alarm is configured to trigger when the counter reaches a value of 1,
 * with a positive comparison test type. It will also increment the counter by 1
 * upon triggering and generate events.
 *
 * @param counter The X Sync Counter to associate with this alarm.
 * @return The newly created Ecore_X_Sync_Alarm, or 0 on failure.
 */
EAPI Ecore_X_Sync_Alarm
ecore_x_sync_alarm_new(Ecore_X_Sync_Counter counter)
{
   Ecore_X_Sync_Alarm alarm;
   XSyncAlarmAttributes values;
   XSyncValue init;

   LOGFN;
   XSyncIntToValue(&init, 0);
   XSyncSetCounter(_ecore_x_disp, counter, init);

   values.trigger.counter = counter;
   values.trigger.value_type = XSyncAbsolute;
   XSyncIntToValue(&values.trigger.wait_value, 1);
   values.trigger.test_type = XSyncPositiveComparison;

   XSyncIntToValue(&values.delta, 1);

   values.events = True;

   alarm = XSyncCreateAlarm(_ecore_x_disp,
                            XSyncCACounter |
                            XSyncCAValueType |
                            XSyncCAValue |
                            XSyncCATestType |
                            XSyncCADelta |
                            XSyncCAEvents,
                            &values);

   ecore_x_sync();
   return alarm;
}

/**
 * @brief Frees an X Sync Alarm.
 *
 * @param alarm The Ecore_X_Sync_Alarm to free.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 *         Typically, this function will return the result of XSyncDestroyAlarm.
 */
EAPI Eina_Bool
ecore_x_sync_alarm_free(Ecore_X_Sync_Alarm alarm)
{
   LOGFN;
   return XSyncDestroyAlarm(_ecore_x_disp, alarm);
}

/**
 * @brief Queries the current value of an X Sync Counter.
 *
 * @param counter The Ecore_X_Sync_Counter to query.
 * @param[out] val Pointer to an unsigned int where the counter's
 *                 low 32-bit value will be stored.
 * @return EINA_TRUE if the query was successful, EINA_FALSE otherwise.
 */
EAPI Eina_Bool
ecore_x_sync_counter_query(Ecore_X_Sync_Counter counter,
                           unsigned int *val)
{
   XSyncValue value;

   LOGFN;
   if (XSyncQueryCounter(_ecore_x_disp, counter, &value))
     {
        *val = (unsigned int)XSyncValueLow32(value);
        return EINA_TRUE;
     }

   return EINA_FALSE;
}

/**
 * @brief Creates a new X Sync Counter.
 *
 * @param val The initial integer value for the counter.
 * @return The newly created Ecore_X_Sync_Counter, or 0 on failure.
 */
EAPI Ecore_X_Sync_Counter
ecore_x_sync_counter_new(int val)
{
   XSyncCounter counter;
   XSyncValue v;

   LOGFN;
   XSyncIntToValue(&v, val);
   counter = XSyncCreateCounter(_ecore_x_disp, v);
   return counter;
}

/**
 * @brief Frees an X Sync Counter.
 *
 * @param counter The Ecore_X_Sync_Counter to free.
 */
EAPI void
ecore_x_sync_counter_free(Ecore_X_Sync_Counter counter)
{
   LOGFN;
   XSyncDestroyCounter(_ecore_x_disp, counter);
}

/**
 * @brief Increments an X Sync Counter by a specified amount.
 *
 * @param counter The Ecore_X_Sync_Counter to increment.
 * @param by The integer value by which to increment the counter.
 */
EAPI void
ecore_x_sync_counter_inc(Ecore_X_Sync_Counter counter,
                         int by)
{
   XSyncValue v;

   LOGFN;
   XSyncIntToValue(&v, by);
   XSyncChangeCounter(_ecore_x_disp, counter, v);
}

/**
 * @brief Waits for an X Sync Counter to reach a specific value.
 *
 * This function blocks until the specified counter reaches the given value.
 * It sets up a wait condition that triggers when the counter's value is
 * greater than or equal to 'val' (XSyncPositiveComparison).
 * The event_threshold is set to 'val + 1'.
 *
 * @param counter The Ecore_X_Sync_Counter to wait on.
 * @param val The integer value to wait for the counter to reach.
 */
EAPI void
ecore_x_sync_counter_val_wait(Ecore_X_Sync_Counter counter,
                              int val)
{
   XSyncWaitCondition cond;
   XSyncValue v, v2;

   LOGFN;
   XSyncQueryCounter(_ecore_x_disp, counter, &v);
   XSyncIntToValue(&v, val);
   XSyncIntToValue(&v2, val + 1);
   cond.trigger.counter = counter;
   cond.trigger.value_type = XSyncAbsolute;
   cond.trigger.wait_value = v;
   cond.trigger.test_type = XSyncPositiveComparison;
   cond.event_threshold = v2;
   XSyncAwait(_ecore_x_disp, &cond, 1);
//   XSync(_ecore_x_disp, False); // dont need this
}

/**
 * @brief Sets the value of an X Sync Counter.
 *
 * This function sets the counter to a 32-bit integer value.
 *
 * @param counter The Ecore_X_Sync_Counter to set.
 * @param val The integer value to set the counter to.
 */
EAPI void
ecore_x_sync_counter_set(Ecore_X_Sync_Counter counter,
                         int val)
{
   XSyncValue v;

   LOGFN;
   XSyncIntToValue(&v, val);
   XSyncSetCounter(_ecore_x_disp, counter, v);
}

/**
 * @brief Sets the 64-bit value of an X Sync Counter.
 *
 * This function sets the counter using a high 32-bit signed integer
 * and a low 32-bit unsigned integer.
 *
 * @param counter The Ecore_X_Sync_Counter to set.
 * @param val_hi The high 32-bit signed integer part of the value.
 * @param val_lo The low 32-bit unsigned integer part of the value.
 */
EAPI void
ecore_x_sync_counter_2_set(Ecore_X_Sync_Counter counter,
                           int val_hi,
                           unsigned int val_lo)
{
   XSyncValue v;

   LOGFN;
   XSyncIntsToValue(&v, val_lo, val_hi);
   XSyncSetCounter(_ecore_x_disp, counter, v);
}

/**
 * @brief Queries the 64-bit value of an X Sync Counter.
 *
 * @param counter The Ecore_X_Sync_Counter to query.
 * @param[out] val_hi Pointer to an int where the high 32-bit signed part
 *                    of the counter's value will be stored.
 * @param[out] val_lo Pointer to an unsigned int where the low 32-bit unsigned part
 *                    of the counter's value will be stored.
 * @return EINA_TRUE if the query was successful, EINA_FALSE otherwise.
 */
EAPI Eina_Bool
ecore_x_sync_counter_2_query(Ecore_X_Sync_Counter counter,
                             int *val_hi,
                             unsigned int *val_lo)
{
   XSyncValue value;

   LOGFN;
   if (XSyncQueryCounter(_ecore_x_disp, counter, &value))
     {
        *val_lo = (unsigned int)XSyncValueLow32(value);
        *val_hi = (int)XSyncValueHigh32(value);
        return EINA_TRUE;
     }
   return EINA_FALSE;
}

