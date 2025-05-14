#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <sys/time.h>
#include <time.h>

#ifdef _WIN32
# include <evil_private.h> /* setenv unsetenv */
#endif

#include <Eina.h>

#include "Embryo.h"
#include "embryo_private.h"

/**
 * @brief Macro to retrieve a string from Embryo data space.
 * @param ep The Embryo_Program instance.
 * @param str Pointer to a char* which will be allocated on the stack
 *            and filled with the string data.
 * @param par The Embryo_Cell containing the address of the string.
 *
 * This macro allocates memory on the stack using alloca() for the string.
 * The caller does not need to free this memory.
 * If the string cannot be retrieved, str will be set to NULL.
 */
#define STRGET(ep, str, par) {                                \
     Embryo_Cell *___cptr;                                    \
     str = NULL;                                              \
     if ((___cptr = embryo_data_address_get(ep, par))) {      \
          int ___l;                                           \
          ___l = embryo_data_string_length_get(ep, ___cptr);  \
          (str) = alloca(___l + 1);                           \
          if (str) embryo_data_string_get(ep, ___cptr, str);  \
       } }
/* exported time api */

/**
 * @brief Get the current time in seconds since midnight.
 * @param ep The Embryo_Program instance (unused).
 * @param params The Embryo_Cell parameters (unused).
 * @return An Embryo_Cell containing the time as a float (seconds since midnight).
 *
 * This function calculates the number of seconds elapsed since the beginning
 * of the current day (midnight).
 * Example: If current time is 00:00:30.500, returns 30.5.
 */
static Embryo_Cell
_embryo_time_seconds(Embryo_Program *ep EINA_UNUSED, Embryo_Cell *params EINA_UNUSED)
{
   struct timeval timev;
   double t;
   float f;

   gettimeofday(&timev, NULL);
   t = (double)(timev.tv_sec - ((timev.tv_sec / (60 * 60 * 24)) * (60 * 60 * 24)))
     + (((double)timev.tv_usec) / 1000000);
   f = (float)t;
   return EMBRYO_FLOAT_TO_CELL(f);
}

/**
 * @brief Get the current date and time components.
 * @param ep The Embryo_Program instance.
 * @param params An array of Embryo_Cell parameters.
 *        params[0] must be (8 * sizeof(Embryo_Cell)).
 *        params[1] to params[8] are pointers to Embryo_Cell where
 *        the date/time components will be stored:
 *        - params[1]: year (e.g., 2023)
 *        - params[2]: month (1-12)
 *        - params[3]: day of month (1-31)
 *        - params[4]: day of year (0-365)
 *        - params[5]: day of week (0=Monday, 1=Tuesday, ..., 6=Sunday)
 *        - params[6]: hour (0-23)
 *        - params[7]: minute (0-59)
 *        - params[8]: second (float, 0.0-59.999999)
 * @return 0 on success, or if parameter validation fails.
 *
 * This function populates the provided Embryo_Cell addresses with the
 * current local date and time components. It calls tzset() periodically
 * to ensure timezone information is up-to-date.
 */
static Embryo_Cell
_embryo_time_date(Embryo_Program *ep, Embryo_Cell *params)
{
   static time_t last_tzset = 0;
   struct timeval timev;
   struct tm *tm;
   time_t tt;

   if (params[0] != (8 * sizeof(Embryo_Cell))) return 0;
   gettimeofday(&timev, NULL);
   tt = (time_t)(timev.tv_sec);
   if ((tt > (last_tzset + 1)) ||
       (tt < (last_tzset - 1)))
     {
        last_tzset = tt;
        tzset();
     }
   tm = localtime(&tt);
   if (tm)
     {
        Embryo_Cell *cptr;
        double t;
        float f;

        cptr = embryo_data_address_get(ep, params[1]);
        if (cptr) *cptr = tm->tm_year + 1900;
        cptr = embryo_data_address_get(ep, params[2]);
        if (cptr) *cptr = tm->tm_mon + 1;
        cptr = embryo_data_address_get(ep, params[3]);
        if (cptr) *cptr = tm->tm_mday;
        cptr = embryo_data_address_get(ep, params[4]);
        if (cptr) *cptr = tm->tm_yday;
        cptr = embryo_data_address_get(ep, params[5]);
        if (cptr) *cptr = (tm->tm_wday + 6) % 7;
        cptr = embryo_data_address_get(ep, params[6]);
        if (cptr) *cptr = tm->tm_hour;
        cptr = embryo_data_address_get(ep, params[7]);
        if (cptr) *cptr = tm->tm_min;
        cptr = embryo_data_address_get(ep, params[8]);
        t = (double)tm->tm_sec + (((double)timev.tv_usec) / 1000000);
        f = (float)t;
        if (cptr) *cptr = EMBRYO_FLOAT_TO_CELL(f);
     }
   return 0;
}

/**
 * @brief Get the date and time components for a specified timezone.
 * @param ep The Embryo_Program instance.
 * @param params An array of Embryo_Cell parameters.
 *        params[0] must be (9 * sizeof(Embryo_Cell)).
 *        params[1] is a pointer to an Embryo_Cell containing the timezone string
 *                  (e.g., "America/New_York", "PST8PDT").
 *        params[2] to params[9] are pointers to Embryo_Cell where
 *        the date/time components will be stored:
 *        - params[2]: year (e.g., 2023)
 *        - params[3]: month (1-12)
 *        - params[4]: day of month (1-31)
 *        - params[5]: day of year (0-365)
 *        - params[6]: day of week (0=Monday, 1=Tuesday, ..., 6=Sunday)
 *        - params[7]: hour (0-23)
 *        - params[8]: minute (0-59)
 *        - params[9]: second (float, 0.0-59.999999)
 * @return 0 on success, or if parameter validation fails.
 *
 * This function temporarily sets the TZ environment variable to the specified
 * timezone, retrieves the local time, and then restores the original TZ setting.
 * It populates the provided Embryo_Cell addresses with the date and time
 * components for that timezone.
 */
static Embryo_Cell
_embryo_time_tzdate(Embryo_Program *ep, Embryo_Cell *params)
{
   struct timeval timev;
   struct tm *tm;
   time_t tt;
   const char *tzenv;
   char *tz, prevtz[128] = {0};

   if (params[0] != (9 * sizeof(Embryo_Cell))) return 0;
   STRGET(ep, tz, params[1]);
   tzenv = getenv("TZ");
   if (tzenv)
     strncpy(prevtz, tzenv, sizeof(prevtz) - 1);
   if (tz && tz[0])
     {
        setenv("TZ", tz, 1);
        tzset();
     }
   gettimeofday(&timev, NULL);
   tt = (time_t)(timev.tv_sec);
   tm = localtime(&tt);
   if (tz && tz[0])
     {
        if (prevtz[0])
          setenv("TZ", prevtz, 1);
        else
          unsetenv("TZ");
        tzset();
     }
   if (tm)
     {
        Embryo_Cell *cptr;
        double t;
        float f;

        cptr = embryo_data_address_get(ep, params[2]);
        if (cptr) *cptr = tm->tm_year + 1900;
        cptr = embryo_data_address_get(ep, params[3]);
        if (cptr) *cptr = tm->tm_mon + 1;
        cptr = embryo_data_address_get(ep, params[4]);
        if (cptr) *cptr = tm->tm_mday;
        cptr = embryo_data_address_get(ep, params[5]);
        if (cptr) *cptr = tm->tm_yday;
        cptr = embryo_data_address_get(ep, params[6]);
        if (cptr) *cptr = (tm->tm_wday + 6) % 7;
        cptr = embryo_data_address_get(ep, params[7]);
        if (cptr) *cptr = tm->tm_hour;
        cptr = embryo_data_address_get(ep, params[8]);
        if (cptr) *cptr = tm->tm_min;
        cptr = embryo_data_address_get(ep, params[9]);
        t = (double)tm->tm_sec + (((double)timev.tv_usec) / 1000000);
        f = (float)t;
        if (cptr) *cptr = EMBRYO_FLOAT_TO_CELL(f);
     }
   return 0;
}

/* functions used by the rest of embryo */

/**
 * @brief Initializes the time-related native calls for an Embryo program.
 * @param ep The Embryo_Program instance to register the functions with.
 *
 * This function registers the following native calls:
 * - "seconds": maps to _embryo_time_seconds()
 * - "date": maps to _embryo_time_date()
 * - "tzdate": maps to _embryo_time_tzdate()
 */
void
_embryo_time_init(Embryo_Program *ep)
{
   embryo_program_native_call_add(ep, "seconds", _embryo_time_seconds);
   embryo_program_native_call_add(ep, "date", _embryo_time_date);
   embryo_program_native_call_add(ep, "tzdate", _embryo_time_tzdate);
}

