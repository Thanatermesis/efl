/**
 * @file
 * @brief Embryo random number generation functions.
 *
 * This file provides functions for generating random numbers within the
 * Embryo scripting environment. It includes functions for integer and
 * floating-point random number generation.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>

#include <Eina.h>

#include "Embryo.h"
#include "embryo_private.h"

/* exported random number api */

/**
 * @brief Generates a random integer.
 * @param ep The Embryo program (unused).
 * @param params Parameters passed to the native call (unused).
 * @return A random integer value masked to 16 bits (0-65535).
 *
 * This function is exposed to Embryo scripts as `rand()`.
 * It uses the standard C library `rand()` function and masks the result
 * to fit within a 16-bit range.
 */
static Embryo_Cell
_embryo_rand_rand(Embryo_Program *ep EINA_UNUSED, Embryo_Cell *params EINA_UNUSED)
{
   return (Embryo_Cell)(rand() & 0xffff);
}

/**
 * @brief Generates a random floating-point number between 0.0 and 1.0.
 * @param ep The Embryo program (unused).
 * @param params Parameters passed to the native call (unused).
 * @return A random float value, converted to Embryo_Cell.
 *
 * This function is exposed to Embryo scripts as `randf()`.
 * It generates a random integer, scales it to the range [0.0, 1.0],
 * and then converts it to a float.
 */
static Embryo_Cell
_embryo_rand_randf(Embryo_Program *ep EINA_UNUSED, Embryo_Cell *params EINA_UNUSED)
{
   double r;
   float f;

   r = (double)(rand() & 0xffff) / 65535.0;
   f = (float)r;
   return EMBRYO_FLOAT_TO_CELL(f);
}

/* functions used by the rest of embryo */

/**
 * @brief Initializes the random number generation native calls.
 * @param ep The Embryo program to register the native calls with.
 *
 * This function is called during Embryo program initialization to make
 * the `rand()` and `randf()` functions available to scripts.
 */
void
_embryo_rand_init(Embryo_Program *ep)
{
   embryo_program_native_call_add(ep, "rand", _embryo_rand_rand);
   embryo_program_native_call_add(ep, "randf", _embryo_rand_randf);
}

