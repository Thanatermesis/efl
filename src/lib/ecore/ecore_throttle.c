#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <unistd.h>

#include "Ecore.h"
#include "ecore_private.h"

/**
 * @internal
 * @brief The current throttle value in microseconds.
 *
 * This value represents the duration for which the main loop will pause
 * during each iteration if throttling is active. It is adjusted by
 * ecore_throttle_adjust() and used by _ecore_throttle().
 */
static int throttle_val = 0;

/**
 * @brief Adjusts the current throttle value.
 *
 * This function modifies the global throttle value. The `amount` is
 * interpreted as seconds. A positive value increases the throttle (slows
 * down the main loop), and a negative value decreases it. The throttle
 * value cannot go below zero.
 *
 * @param amount The amount in seconds to adjust the throttle by.
 *               For example, 0.1 means add 100ms to the throttle,
 *               -0.05 means subtract 50ms.
 */
EAPI void
ecore_throttle_adjust(double amount)
{
   EINA_MAIN_LOOP_CHECK_RETURN;
   int adj = amount * 1000000.0;
   throttle_val += adj;
   if (throttle_val < 0) throttle_val = 0;
}

/**
 * @brief Gets the current throttle value.
 *
 * This function returns the current throttle value in seconds.
 *
 * @return The current throttle value in seconds. For example, if the
 *         throttle is set to 100ms, this function returns 0.1.
 */
EAPI double
ecore_throttle_get(void)
{
   EINA_MAIN_LOOP_CHECK_RETURN_VAL(0.0);
   return (double)throttle_val / 1000000.0;
}

/**
 * @internal
 * @brief Applies the current throttle value by pausing execution.
 *
 * This function is called internally by the Ecore main loop during each
 * iteration. If the `throttle_val` is greater than zero, this function
 * will pause execution for `throttle_val` microseconds using `usleep()`.
 * Event logging for throttle start and end is also performed if enabled.
 */
void
_ecore_throttle(void)
{
   if (throttle_val <= 0) return;
   eina_evlog("+throttle", NULL, 0.0, NULL);
   usleep(throttle_val);
   eina_evlog("-throttle", NULL, 0.0, NULL);
}

