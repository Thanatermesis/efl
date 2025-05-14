/* EINA - EFL data type library
 * Copyright (C) 2008 Cedric Bail, Vincent Torri
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library;
 * if not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "eina_config.h"
#include "eina_private.h"
#include "eina_inlist.h"

/* undefs EINA_ARG_NONULL() so NULL checks are not compiled out! */
#include "eina_safety_checks.h"
#include "eina_counter.h"

#include "eina_private.h"

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/

/**
 * @cond LOCAL
 */

/**
 * @struct _Eina_Counter
 * @brief Represents a counter, holding a list of clock measurements and a name.
 *
 * This structure is used to group multiple time measurements (clocks)
 * under a single named counter. The `EINA_INLIST` macro allows instances
 * of this structure to be part of an Eina_Inlist.
 */
typedef struct _Eina_Clock Eina_Clock;

struct _Eina_Counter
{
   EINA_INLIST; /**< Macro for intrusive linked list support. */

   Eina_Inlist *clocks; /**< A list of Eina_Clock structures associated with this counter. */
   const char *name;    /**< The name of the counter, used for identification. */
};

/**
 * @struct _Eina_Clock
 * @brief Represents a single time measurement (a clock).
 *
 * This structure stores the start and end times of a measurement,
 * an identifier for the specimen (test run), and a validity flag.
 * The `EINA_INLIST` macro allows instances of this structure to be
 * part of an Eina_Inlist, typically managed by an Eina_Counter.
 */
struct _Eina_Clock
{
   EINA_INLIST; /**< Macro for intrusive linked list support. */

   Eina_Nano_Time start; /**< The starting timestamp of the measurement. */
   Eina_Nano_Time end;   /**< The ending timestamp of the measurement. */
   int specimen;         /**< An identifier for the specific test run or sample being measured. */

   Eina_Bool valid;      /**< A flag indicating whether this clock measurement is valid (EINA_TRUE) or not (EINA_FALSE). */
};

/**
 * @brief Appends a formatted string to an existing string, reallocating as needed.
 * @param base The base string to append to. Can be NULL.
 * @param position A pointer to an integer holding the current end position (length) of the base string. This will be updated.
 * @param format The format string, as in printf.
 * @param ... Additional arguments for the format string.
 * @return A pointer to the (potentially reallocated) string with the new content appended, or the original base if reallocation failed at a critical point.
 *
 * This function is similar to asprintf, but it appends to an existing,
 * dynamically allocated string. It handles reallocation if the current
 * buffer `base` is not large enough. The `position` argument tracks the
 * current length of the content in `base` and is updated to reflect the
 * new length after appending.
 */
static char *
_eina_counter_asiprintf(char *base, int *position, const char *format, ...)
{
   char *tmp, *result;
   int size = 32;
   int n;
   va_list ap;

   tmp = realloc(base, sizeof (char) * (*position + size));
   if (!tmp)
      return base;

   result = tmp;

   while (1)
     {
        va_start(ap, format);
        n = vsnprintf(result + *position, size, format, ap);
        va_end(ap);

        if (n > -1 && n < size)
          {
             /* If we always have glibc > 2.2, we could just return *position += n. */
             *position += strlen(result + *position);
             return result;
          }

        if (n > -1)
           size = n + 1;
        else
           size <<= 1;

        tmp = realloc(result, sizeof (char) * (*position + size));
        if (!tmp)
           return result;

        result = tmp;
     }
}

/**
 * @endcond
 */

/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

EINA_API Eina_Counter *
eina_counter_new(const char *name)
{
   Eina_Counter *counter;
   size_t length;

   EINA_SAFETY_ON_NULL_RETURN_VAL(name, NULL);

   length = strlen(name) + 1;
   counter = calloc(1, sizeof (Eina_Counter) + length);
   if (!counter) return NULL;

   counter->name = (char *)(counter + 1);
   memcpy((char *)counter->name, name, length);

   return counter;
}

EINA_API void
eina_counter_free(Eina_Counter *counter)
{
   EINA_SAFETY_ON_NULL_RETURN(counter);

   while (counter->clocks)
     {
        Eina_Clock *clk = (Eina_Clock *)counter->clocks;

        counter->clocks = eina_inlist_remove(counter->clocks, counter->clocks);
        free(clk);
     }

   free(counter);
}

EINA_API void
eina_counter_start(Eina_Counter *counter)
{
   Eina_Clock *clk;
   Eina_Nano_Time tp;

   EINA_SAFETY_ON_NULL_RETURN(counter);
   if (_eina_time_get(&tp) != 0) return;

   clk = calloc(1, sizeof (Eina_Clock));
   if (!clk) return;

   counter->clocks = eina_inlist_prepend(counter->clocks, EINA_INLIST_GET(clk));

   clk->valid = EINA_FALSE;
   clk->start = tp;
}

EINA_API void
eina_counter_stop(Eina_Counter *counter, int specimen)
{
   Eina_Clock *clk;
   Eina_Nano_Time tp;

   EINA_SAFETY_ON_NULL_RETURN(counter);
   if (_eina_time_get(&tp) != 0)
      return;

   clk = (Eina_Clock *)counter->clocks;

   if (!clk || clk->valid == EINA_TRUE)
      return;

   clk->end = tp;
   clk->specimen = specimen;
   clk->valid = EINA_TRUE;
}

EINA_API char *
eina_counter_dump(Eina_Counter *counter)
{
   Eina_Clock *clk;
   char *result = NULL;
   int position = 0;

   EINA_SAFETY_ON_NULL_RETURN_VAL(counter, NULL);

   result = _eina_counter_asiprintf(
         result,
         &position,
         "# specimen\texperiment time\tstarting time\tending time\n");
   if (!result)
      return NULL;

   EINA_INLIST_REVERSE_FOREACH(counter->clocks, clk)
   {
      long int start;
      long int end;
      long int diff;

      if (clk->valid == EINA_FALSE)
         continue;

      start = _eina_time_convert(&clk->start);
      end = _eina_time_convert(&clk->end);
      diff = _eina_time_delta(&clk->start, &clk->end);

      result = _eina_counter_asiprintf(result, &position,
                                       "%i\t%li\t%li\t%li\n",
                                       clk->specimen,
                                       diff,
                                       start,
                                       end);
   }

   return result;
}
