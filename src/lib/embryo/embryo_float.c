/*  Float arithmetic for the Small AMX engine
 *
 *  Copyright (c) Artran, Inc. 1999
 *  Written by Greg Garner (gmg@artran.com)
 *  Portions Copyright (c) Carsten Haitzler, 2004 <raster@rasterman.com>
 *
 *  This software is provided "as-is", without any express or implied warranty.
 *  In no event will the authors be held liable for any damages arising from
 *  the use of this software.
 *
 *  Permission is granted to anyone to use this software for any purpose,
 *  including commercial applications, and to alter it and redistribute it
 *  freely, subject to the following restrictions:
 *
 *  1.  The origin of this software must not be misrepresented; you must not
 *      claim that you wrote the original software. If you use this software in
 *      a product, an acknowledgment in the product documentation would be
 *      appreciated but is not required.
 *  2.  Altered source versions must be plainly marked as such, and must not be
 *      misrepresented as being the original software.
 *  3.  This notice may not be removed or altered from any source distribution.
 */

/**
 * @file
 * @brief Float arithmetic functions for the Small AMX engine.
 *
 * This file implements a set of floating-point arithmetic operations
 * that can be called from Embryo scripts. These functions provide
 * basic math capabilities, trigonometric functions, and type conversions.
 */

/* CHANGES -
 * 2002-08-27: Basic conversion of source from C++ to C by Adam D. Moss
 *             <adam@gimp.org> <aspirin@icculus.org>
 * 2003-08-29: Removal of the dynamic memory allocation and replacing two
 *             type conversion functions by macros, by Thiadmer Riemersma
 * 2003-09-22: Moved the type conversion macros to AMX.H, and simplifications
 *             of some routines, by Thiadmer Riemersma
 * 2003-11-24: A few more native functions (geometry), plus minor modifications,
 *             mostly to be compatible with dynamically loadable extension
 *             modules, by Thiadmer Riemersma
 * 2004-03-20: Cleaned up and reduced size for Embryo, Modified to conform to
 *             E coding style. Added extra parameter checks.
 *             Carsten Haitzler, <raster@rasterman.com>
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <math.h>

#include <Eina.h>

#include "Embryo.h"
#include "embryo_private.h"

#define PI       3.1415926535897932384626433832795f
#ifndef MAXFLOAT
#define MAXFLOAT 3.40282347e+38f
#endif

/* internally useful calls */

/**
 * @internal
 * @brief Converts an angle from degrees or grades to radians.
 *
 * @param angle The angle value.
 * @param radix The unit of the input angle:
 *              - 1: degrees (sexagesimal system)
 *              - 2: grades (centesimal system)
 *              - other: radians (no conversion)
 * @return The angle in radians.
 */
static float
_embryo_fp_degrees_to_radians(float angle, int radix)
{
   switch (radix)
     {
      case 1: /* degrees, sexagesimal system (technically: degrees/minutes/seconds) */
        return angle * PI / 180.0f;

      case 2: /* grades, centesimal system */
        return angle * PI / 200.0f;

      default: /* assume already radian */
        break;
     }
   return angle;
}

/* exported float api */

/**
 * @brief Converts an integer to a float.
 * @param ep The Embryo program instance (unused).
 * @param params An array of Embryo cells.
 *               params[0] is the size of the parameters in bytes.
 *               params[1] is the integer value to convert.
 * @return The float value as an Embryo_Cell, or 0 on error.
 * @note Native function: `float(value)`
 */
static Embryo_Cell
_embryo_fp(Embryo_Program *ep EINA_UNUSED, Embryo_Cell *params)
{
   /* params[1] = long value to convert to a float */
   float f;

   if (params[0] != (1 * sizeof(Embryo_Cell))) return 0;
   f = (float)params[1];
   return EMBRYO_FLOAT_TO_CELL(f);
}

/**
 * @brief Converts a string to a float.
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the size of the parameters in bytes.
 *               params[1] is the virtual address of the string to convert.
 * @return The float value as an Embryo_Cell, or 0 on error or if the string is invalid.
 * @note Native function: `atof(string_address)`
 */
static Embryo_Cell
_embryo_fp_str(Embryo_Program *ep, Embryo_Cell *params)
{
   /* params[1] = virtual string address to convert to a float */
   char buf[64];
   Embryo_Cell *str;
   float f;
   int len;

   if (params[0] != (1 * sizeof(Embryo_Cell))) return 0;
   str = embryo_data_address_get(ep, params[1]);
   len = embryo_data_string_length_get(ep, str);
   if ((len == 0) || (len >= (int)sizeof(buf))) return 0;
   embryo_data_string_get(ep, str, buf);
   f = (float)eina_convert_strtod_c(buf, NULL);
   return EMBRYO_FLOAT_TO_CELL(f);
}

/**
 * @brief Multiplies two float numbers.
 * @param ep The Embryo program instance (unused).
 * @param params An array of Embryo cells.
 *               params[0] is the size of the parameters in bytes.
 *               params[1] is the first float operand.
 *               params[2] is the second float operand.
 * @return The product of the two floats as an Embryo_Cell, or 0 on error.
 * @note Native function: `float_mul(operand1, operand2)`
 */
static Embryo_Cell
_embryo_fp_mul(Embryo_Program *ep EINA_UNUSED, Embryo_Cell *params)
{
   /* params[1] = float operand 1 */
   /* params[2] = float operand 2 */
   float f;

   if (params[0] != (2 * sizeof(Embryo_Cell))) return 0;
   f = EMBRYO_CELL_TO_FLOAT(params[1]) * EMBRYO_CELL_TO_FLOAT(params[2]);
   return EMBRYO_FLOAT_TO_CELL(f);
}

/**
 * @brief Divides the first float by the second float.
 * @param ep The Embryo program instance (unused).
 * @param params An array of Embryo cells.
 *               params[0] is the size of the parameters in bytes.
 *               params[1] is the float dividend.
 *               params[2] is the float divisor.
 * @return The result of the division as an Embryo_Cell.
 *         Returns 0.0 if both dividend and divisor are 0.0.
 *         Returns -MAXFLOAT or MAXFLOAT for division by zero, depending on the sign of the dividend.
 *         Returns 0 on parameter error.
 * @note Native function: `float_div(dividend, divisor)`
 */
static Embryo_Cell
_embryo_fp_div(Embryo_Program *ep EINA_UNUSED, Embryo_Cell *params)
{
   /* params[1] = float dividend (top) */
   /* params[2] = float divisor (bottom) */
   float f, ff;

   if (params[0] != (2 * sizeof(Embryo_Cell))) return 0;
   f = EMBRYO_CELL_TO_FLOAT(params[1]);
   ff = EMBRYO_CELL_TO_FLOAT(params[2]);
   if (EINA_FLT_EQ(ff, 0.0))
     {
        if (EINA_FLT_EQ(f, 0.0))
          return EMBRYO_FLOAT_TO_CELL(0.0f);
        else if (f < 0.0)
          return EMBRYO_FLOAT_TO_CELL(-MAXFLOAT);
        else
          return EMBRYO_FLOAT_TO_CELL(MAXFLOAT);
     }
   f = f / ff;
   return EMBRYO_FLOAT_TO_CELL(f);
}

/**
 * @brief Adds two float numbers.
 * @param ep The Embryo program instance (unused).
 * @param params An array of Embryo cells.
 *               params[0] is the size of the parameters in bytes.
 *               params[1] is the first float operand.
 *               params[2] is the second float operand.
 * @return The sum of the two floats as an Embryo_Cell, or 0 on error.
 * @note Native function: `float_add(operand1, operand2)`
 */
static Embryo_Cell
_embryo_fp_add(Embryo_Program *ep EINA_UNUSED, Embryo_Cell *params)
{
   /* params[1] = float operand 1 */
   /* params[2] = float operand 2 */
   float f;

   if (params[0] != (2 * sizeof(Embryo_Cell))) return 0;
   f = EMBRYO_CELL_TO_FLOAT(params[1]) + EMBRYO_CELL_TO_FLOAT(params[2]);
   return EMBRYO_FLOAT_TO_CELL(f);
}

/**
 * @brief Subtracts the second float from the first float.
 * @param ep The Embryo program instance (unused).
 * @param params An array of Embryo cells.
 *               params[0] is the size of the parameters in bytes.
 *               params[1] is the first float operand.
 *               params[2] is the second float operand.
 * @return The difference of the two floats as an Embryo_Cell, or 0 on error.
 * @note Native function: `float_sub(operand1, operand2)`
 */
static Embryo_Cell
_embryo_fp_sub(Embryo_Program *ep EINA_UNUSED, Embryo_Cell *params)
{
   /* params[1] = float operand 1 */
   /* params[2] = float operand 2 */
   float f;

   if (params[0] != (2 * sizeof(Embryo_Cell))) return 0;
   f = EMBRYO_CELL_TO_FLOAT(params[1]) - EMBRYO_CELL_TO_FLOAT(params[2]);
   return EMBRYO_FLOAT_TO_CELL(f);
}

/**
 * @brief Returns the fractional part of a float.
 * @param ep The Embryo program instance (unused).
 * @param params An array of Embryo cells.
 *               params[0] is the size of the parameters in bytes.
 *               params[1] is the float operand.
 * @return The fractional part of the float as an Embryo_Cell, or 0 on error.
 *         Example: `fract(3.14)` returns `0.14`.
 * @note Native function: `fract(value)`
 */
static Embryo_Cell
_embryo_fp_fract(Embryo_Program *ep EINA_UNUSED, Embryo_Cell *params)
{
   /* params[1] = float operand */
   float f;

   if (params[0] != (1 * sizeof(Embryo_Cell))) return 0;
   f = EMBRYO_CELL_TO_FLOAT(params[1]);
   f -= (floorf(f));
   return EMBRYO_FLOAT_TO_CELL(f);
}

/**
 * @brief Rounds a float to an integer value based on the specified rounding type.
 * @param ep The Embryo program instance (unused).
 * @param params An array of Embryo cells.
 *               params[0] is the size of the parameters in bytes.
 *               params[1] is the float operand.
 *               params[2] is the type of rounding (integer):
 *                         - 1: round downwards (truncate, floor). Example: `round(3.7, 1)` returns `3`.
 *                         - 2: round upwards (ceil). Example: `round(3.1, 2)` returns `4`.
 *                         - 3: round towards zero. Example: `round(-3.7, 3)` returns `-3`.
 *                         - default: standard round to nearest (floor(f + 0.5)). Example: `round(3.7)` returns `4`, `round(3.2)` returns `3`.
 * @return The rounded integer value as an Embryo_Cell, or 0 on error.
 * @note Native function: `round(value, type)`
 */
static Embryo_Cell
_embryo_fp_round(Embryo_Program *ep EINA_UNUSED, Embryo_Cell *params)
{
   /* params[1] = float operand */
   /* params[2] = Type of rounding (cell) */
   float f;

   if (params[0] != (2 * sizeof(Embryo_Cell))) return 0;
   f = EMBRYO_CELL_TO_FLOAT(params[1]);
   switch (params[2])
     {
      case 1: /* round downwards (truncate) */
        f = (floorf(f));
        break;

      case 2: /* round upwards */
        f = (ceilf(f));
        break;

      case 3: /* round towards zero */
        if (f >= 0.0) f = (floorf(f));
        else f = (ceilf(f));
        break;

      default: /* standard, round to nearest */
        f = (floorf(f + 0.5));
        break;
     }
   return (Embryo_Cell)f;
}

/**
 * @brief Compares two float numbers.
 * @param ep The Embryo program instance (unused).
 * @param params An array of Embryo cells.
 *               params[0] is the size of the parameters in bytes.
 *               params[1] is the first float operand.
 *               params[2] is the second float operand.
 * @return An integer Embryo_Cell:
 *         - 0 if operand1 is equal to operand2.
 *         - 1 if operand1 is greater than operand2.
 *         - -1 if operand1 is less than operand2.
 *         Returns 0 on parameter error.
 * @note Native function: `float_cmp(operand1, operand2)`
 */
static Embryo_Cell
_embryo_fp_cmp(Embryo_Program *ep EINA_UNUSED, Embryo_Cell *params)
{
   /* params[1] = float operand 1 */
   /* params[2] = float operand 2 */
   float f, ff;

   if (params[0] != (2 * sizeof(Embryo_Cell))) return 0;
   f = EMBRYO_CELL_TO_FLOAT(params[1]);
   ff = EMBRYO_CELL_TO_FLOAT(params[2]);
   if (EINA_FLT_EQ(f, ff)) return 0;
   else if (f > ff)
     return 1;
   return -1;
}

/**
 * @brief Calculates the square root of a float.
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the size of the parameters in bytes.
 *               params[1] is the float operand.
 * @return The square root of the operand as an Embryo_Cell.
 *         Sets EMBRYO_ERROR_DOMAIN and returns 0 if the operand is negative.
 *         Returns 0 on parameter error.
 * @note Native function: `sqrt(value)`
 */
static Embryo_Cell
_embryo_fp_sqroot(Embryo_Program *ep, Embryo_Cell *params)
{
   /* params[1] = float operand */
   float f;

   if (params[0] != (1 * sizeof(Embryo_Cell))) return 0;
   f = EMBRYO_CELL_TO_FLOAT(params[1]);
   f = sqrtf(f);
   if (f < 0)
     {
        embryo_program_error_set(ep, EMBRYO_ERROR_DOMAIN);
        return 0;
     }
   return EMBRYO_FLOAT_TO_CELL(f);
}

/**
 * @brief Calculates the value of the first float raised to the power of the second float.
 * @param ep The Embryo program instance (unused).
 * @param params An array of Embryo cells.
 *               params[0] is the size of the parameters in bytes.
 *               params[1] is the base float operand.
 *               params[2] is the exponent float operand.
 * @return The result of base raised to the power of exponent, as an Embryo_Cell.
 *         Returns 0 on parameter error.
 * @note Native function: `pow(base, exponent)`
 */
static Embryo_Cell
_embryo_fp_power(Embryo_Program *ep EINA_UNUSED, Embryo_Cell *params)
{
   /* params[1] = float operand 1 */
   /* params[2] = float operand 2 */
   float f, ff;

   if (params[0] != (2 * sizeof(Embryo_Cell))) return 0;
   f = EMBRYO_CELL_TO_FLOAT(params[1]);
   ff = EMBRYO_CELL_TO_FLOAT(params[2]);
   f = powf(f, ff);
   return EMBRYO_FLOAT_TO_CELL(f);
}

/**
 * @brief Calculates the logarithm of a float value with a specified base.
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the size of the parameters in bytes.
 *               params[1] is the float value.
 *               params[2] is the float base of the logarithm.
 * @return The logarithm of the value with the specified base, as an Embryo_Cell.
 *         Uses `log10f()` if base is 10.0.
 *         Uses `log2f()` if base is 2.0.
 *         Otherwise, calculates `log(value) / log(base)`.
 *         Sets EMBRYO_ERROR_DOMAIN and returns 0 if value or base is non-positive.
 *         Returns 0.0 if base is 1.0 (log(1) = 0, division by zero).
 *         Returns 0 on parameter error.
 * @note Native function: `log(value, base)`
 */
static Embryo_Cell
_embryo_fp_log(Embryo_Program *ep, Embryo_Cell *params)
{
   /* params[1] = float operand 1 (value) */
   /* params[2] = float operand 2 (base) */
   float f, ff, tf;

   if (params[0] != (2 * sizeof(Embryo_Cell))) return 0;
   f = EMBRYO_CELL_TO_FLOAT(params[1]);
   ff = EMBRYO_CELL_TO_FLOAT(params[2]);
   if ((f <= 0.0) || (ff <= 0.0))
     {
        embryo_program_error_set(ep, EMBRYO_ERROR_DOMAIN);
        return 0;
     }
   if (EINA_FLT_EQ(ff, 10.0))
     f = log10f(f);
   else if (EINA_FLT_EQ(ff, 2.0))
     f = log2f(f);
   else
     {
        tf = logf(ff);
        if (EINA_FLT_EQ(tf, 0.0))
          f = 0.0;
        else f = (logf(f) / tf);
     }
   return EMBRYO_FLOAT_TO_CELL(f);
}

/**
 * @brief Calculates the sine of an angle.
 * @param ep The Embryo program instance (unused).
 * @param params An array of Embryo cells.
 *               params[0] is the size of the parameters in bytes.
 *               params[1] is the float angle.
 *               params[2] is the radix of the angle (see _embryo_fp_degrees_to_radians()).
 * @return The sine of the angle as an Embryo_Cell, or 0 on error.
 * @note Native function: `sin(angle, radix)`
 */
static Embryo_Cell
_embryo_fp_sin(Embryo_Program *ep EINA_UNUSED, Embryo_Cell *params)
{
   /* params[1] = float operand 1 (angle) */
   /* params[2] = float operand 2 (radix) */
   float f;

   if (params[0] != (2 * sizeof(Embryo_Cell))) return 0;
   f = EMBRYO_CELL_TO_FLOAT(params[1]);
   f = _embryo_fp_degrees_to_radians(f, params[2]);
   f = sinf(f);
   return EMBRYO_FLOAT_TO_CELL(f);
}

/**
 * @brief Calculates the cosine of an angle.
 * @param ep The Embryo program instance (unused).
 * @param params An array of Embryo cells.
 *               params[0] is the size of the parameters in bytes.
 *               params[1] is the float angle.
 *               params[2] is the radix of the angle (see _embryo_fp_degrees_to_radians()).
 * @return The cosine of the angle as an Embryo_Cell, or 0 on error.
 * @note Native function: `cos(angle, radix)`
 */
static Embryo_Cell
_embryo_fp_cos(Embryo_Program *ep EINA_UNUSED, Embryo_Cell *params)
{
   /* params[1] = float operand 1 (angle) */
   /* params[2] = float operand 2 (radix) */
   float f;

   if (params[0] != (2 * sizeof(Embryo_Cell))) return 0;
   f = EMBRYO_CELL_TO_FLOAT(params[1]);
   f = _embryo_fp_degrees_to_radians(f, params[2]);
   f = cosf(f);
   return EMBRYO_FLOAT_TO_CELL(f);
}

/**
 * @brief Calculates the tangent of an angle.
 * @param ep The Embryo program instance (unused).
 * @param params An array of Embryo cells.
 *               params[0] is the size of the parameters in bytes.
 *               params[1] is the float angle.
 *               params[2] is the radix of the angle (see _embryo_fp_degrees_to_radians()).
 * @return The tangent of the angle as an Embryo_Cell, or 0 on error.
 * @note Native function: `tan(angle, radix)`
 */
static Embryo_Cell
_embryo_fp_tan(Embryo_Program *ep EINA_UNUSED, Embryo_Cell *params)
{
   /* params[1] = float operand 1 (angle) */
   /* params[2] = float operand 2 (radix) */
   float f;

   if (params[0] != (2 * sizeof(Embryo_Cell))) return 0;
   f = EMBRYO_CELL_TO_FLOAT(params[1]);
   f = _embryo_fp_degrees_to_radians(f, params[2]);
   f = tanf(f);
   return EMBRYO_FLOAT_TO_CELL(f);
}

/**
 * @brief Calculates the absolute value of a float.
 * @param ep The Embryo program instance (unused).
 * @param params An array of Embryo cells.
 *               params[0] is the size of the parameters in bytes.
 *               params[1] is the float operand.
 * @return The absolute value of the float as an Embryo_Cell, or 0 on error.
 * @note Native function: `abs(value)`
 */
static Embryo_Cell
_embryo_fp_abs(Embryo_Program *ep EINA_UNUSED, Embryo_Cell *params)
{
   /* params[1] = float operand */
   float f;

   if (params[0] != (1 * sizeof(Embryo_Cell))) return 0;
   f = EMBRYO_CELL_TO_FLOAT(params[1]);
   f = (f >= 0) ? f : -f;
   return EMBRYO_FLOAT_TO_CELL(f);
}

/**
 * @brief Calculates the arc sine of a value.
 * @param ep The Embryo program instance (unused).
 * @param params An array of Embryo cells.
 *               params[0] is the size of the parameters in bytes.
 *               params[1] is the float value (between -1.0 and 1.0).
 *               params[2] is the desired radix for the output angle (see _embryo_fp_degrees_to_radians()).
 * @return The arc sine of the value in the specified radix, as an Embryo_Cell.
 *         Returns 0 on parameter error. Domain errors from asinf() (input out of range [-1,1]) result in NaN, which is converted to a cell.
 * @note Native function: `asin(value, radix)`
 */
static Embryo_Cell
_embryo_fp_asin(Embryo_Program *ep EINA_UNUSED, Embryo_Cell *params)
{
   /* params[1] = float operand 1 (angle) */
   /* params[2] = float operand 2 (radix) */
   float f;

   if (params[0] != (2 * sizeof(Embryo_Cell))) return 0;
   f = EMBRYO_CELL_TO_FLOAT(params[1]);
   f = asinf(f);
   f = _embryo_fp_degrees_to_radians(f, params[2]);
   return EMBRYO_FLOAT_TO_CELL(f);
}

/**
 * @brief Calculates the arc cosine of a value.
 * @param ep The Embryo program instance (unused).
 * @param params An array of Embryo cells.
 *               params[0] is the size of the parameters in bytes.
 *               params[1] is the float value (between -1.0 and 1.0).
 *               params[2] is the desired radix for the output angle (see _embryo_fp_degrees_to_radians()).
 * @return The arc cosine of the value in the specified radix, as an Embryo_Cell.
 *         Returns 0 on parameter error. Domain errors from acosf() (input out of range [-1,1]) result in NaN, which is converted to a cell.
 * @note Native function: `acos(value, radix)`
 */
static Embryo_Cell
_embryo_fp_acos(Embryo_Program *ep EINA_UNUSED, Embryo_Cell *params)
{
   /* params[1] = float operand 1 (angle) */
   /* params[2] = float operand 2 (radix) */
   float f;

   if (params[0] != (2 * sizeof(Embryo_Cell))) return 0;
   f = EMBRYO_CELL_TO_FLOAT(params[1]);
   f = acosf(f);
   f = _embryo_fp_degrees_to_radians(f, params[2]);
   return EMBRYO_FLOAT_TO_CELL(f);
}

/**
 * @brief Calculates the arc tangent of a value.
 * @param ep The Embryo program instance (unused).
 * @param params An array of Embryo cells.
 *               params[0] is the size of the parameters in bytes.
 *               params[1] is the float value.
 *               params[2] is the desired radix for the output angle (see _embryo_fp_degrees_to_radians()).
 * @return The arc tangent of the value in the specified radix, as an Embryo_Cell.
 *         Returns 0 on parameter error.
 * @note Native function: `atan(value, radix)`
 */
static Embryo_Cell
_embryo_fp_atan(Embryo_Program *ep EINA_UNUSED, Embryo_Cell *params)
{
   /* params[1] = float operand 1 (angle) */
   /* params[2] = float operand 2 (radix) */
   float f;

   if (params[0] != (2 * sizeof(Embryo_Cell))) return 0;
   f = EMBRYO_CELL_TO_FLOAT(params[1]);
   f = atanf(f);
   f = _embryo_fp_degrees_to_radians(f, params[2]);
   return EMBRYO_FLOAT_TO_CELL(f);
}

/**
 * @brief Calculates the arc tangent of y/x, using the signs of both arguments to determine the quadrant of the result.
 * @param ep The Embryo program instance (unused).
 * @param params An array of Embryo cells.
 *               params[0] is the size of the parameters in bytes.
 *               params[1] is the float y-coordinate.
 *               params[2] is the float x-coordinate.
 *               params[3] is the desired radix for the output angle (see _embryo_fp_degrees_to_radians()).
 * @return The arc tangent of y/x in the specified radix, as an Embryo_Cell.
 *         Returns 0 on parameter error.
 * @note Native function: `atan2(y, x, radix)`
 */
static Embryo_Cell
_embryo_fp_atan2(Embryo_Program *ep EINA_UNUSED, Embryo_Cell *params)
{
   /* params[1] = float operand 1 (y) */
   /* params[2] = float operand 2 (x) */
   /* params[3] = float operand 3 (radix) */
   float f, ff;

   if (params[0] != (3 * sizeof(Embryo_Cell))) return 0;
   f = EMBRYO_CELL_TO_FLOAT(params[1]);
   ff = EMBRYO_CELL_TO_FLOAT(params[2]);
   f = atan2f(f, ff);
   f = _embryo_fp_degrees_to_radians(f, params[3]);
   return EMBRYO_FLOAT_TO_CELL(f);
}

/**
 * @brief Calculates the natural logarithm of 1 plus the given number (log(1+x)).
 * This function is more accurate than `log(1.0 + x)` for small values of x.
 * @param ep The Embryo program instance (unused).
 * @param params An array of Embryo cells.
 *               params[0] is the size of the parameters in bytes.
 *               params[1] is the float operand x.
 * @return The natural logarithm of (1+x) as an Embryo_Cell.
 *         Returns 0 on parameter error. Domain errors from log1pf() (input <= -1) result in -HUGE_VAL or NaN, which is converted to a cell.
 * @note Native function: `log1p(value)`
 */
static Embryo_Cell
_embryo_fp_log1p(Embryo_Program *ep EINA_UNUSED, Embryo_Cell *params)
{
   /* params[1] = float operand */
   float f;

   if (params[0] != (1 * sizeof(Embryo_Cell))) return 0;
   f = EMBRYO_CELL_TO_FLOAT(params[1]);
   f = log1pf(f);
   return EMBRYO_FLOAT_TO_CELL(f);
}

/**
 * @brief Calculates the cubic root of a float.
 * @param ep The Embryo program instance (unused).
 * @param params An array of Embryo cells.
 *               params[0] is the size of the parameters in bytes.
 *               params[1] is the float operand.
 * @return The cubic root of the operand as an Embryo_Cell, or 0 on error.
 * @note Native function: `cbrt(value)`
 */
static Embryo_Cell
_embryo_fp_cbrt(Embryo_Program *ep EINA_UNUSED, Embryo_Cell *params)
{
   /* params[1] = float operand */
   float f;

   if (params[0] != (1 * sizeof(Embryo_Cell))) return 0;
   f = EMBRYO_CELL_TO_FLOAT(params[1]);
   f = cbrtf(f);
   return EMBRYO_FLOAT_TO_CELL(f);
}

/**
 * @brief Calculates the base-e exponential of a float (e^x).
 * @param ep The Embryo program instance (unused).
 * @param params An array of Embryo cells.
 *               params[0] is the size of the parameters in bytes.
 *               params[1] is the float operand (exponent).
 * @return e raised to the power of the operand, as an Embryo_Cell.
 *         Returns 0 on parameter error.
 * @note Native function: `exp(value)`
 */
static Embryo_Cell
_embryo_fp_exp(Embryo_Program *ep EINA_UNUSED, Embryo_Cell *params)
{
   /* params[1] = float operand */
   float f;

   if (params[0] != (1 * sizeof(Embryo_Cell))) return 0;
   f = EMBRYO_CELL_TO_FLOAT(params[1]);
   f = expf(f);
   return EMBRYO_FLOAT_TO_CELL(f);
}

/**
 * @brief Calculates the base-2 exponential of a float (2^x).
 * @param ep The Embryo program instance (unused).
 * @param params An array of Embryo cells.
 *               params[0] is the size of the parameters in bytes.
 *               params[1] is the float operand (exponent).
 * @return 2 raised to the power of the operand, as an Embryo_Cell.
 *         Returns 0 on parameter error.
 * @note Native function: `exp2(value)`
 */
static Embryo_Cell
_embryo_fp_exp2(Embryo_Program *ep EINA_UNUSED, Embryo_Cell *params)
{
   /* params[1] = float operand */
   float f;

   if (params[0] != (1 * sizeof(Embryo_Cell))) return 0;
   f = EMBRYO_CELL_TO_FLOAT(params[1]);
   f = exp2f(f);
   return EMBRYO_FLOAT_TO_CELL(f);
}

/**
 * @brief Calculates the hypotenuse of a right-angled triangle (sqrt(x*x + y*y)).
 * @param ep The Embryo program instance (unused).
 * @param params An array of Embryo cells.
 *               params[0] is the size of the parameters in bytes.
 *               params[1] is the float x.
 *               params[2] is the float y.
 * @return The length of the hypotenuse as an Embryo_Cell.
 *         Returns 0 on parameter error.
 * @note Native function: `hypot(x, y)`
 */
static Embryo_Cell
_embryo_fp_hypot(Embryo_Program *ep EINA_UNUSED, Embryo_Cell *params)
{
   /* params[1] = float operand */
   float f, ff;

   if (params[0] != (2 * sizeof(Embryo_Cell))) return 0;
   f = EMBRYO_CELL_TO_FLOAT(params[1]);
   ff = EMBRYO_CELL_TO_FLOAT(params[2]);
   f = hypotf(f, ff);
   return EMBRYO_FLOAT_TO_CELL(f);
}

/* functions used by the rest of embryo */

/**
 * @brief Initializes and registers all float native functions for an Embryo program.
 * @param ep The Embryo program instance to register the functions with.
 *
 * This function is called to make the float arithmetic functions available
 * to Embryo scripts. It maps script function names (e.g., "float", "sin")
 * to their corresponding C implementations.
 */
void
_embryo_fp_init(Embryo_Program *ep)
{
   embryo_program_native_call_add(ep, "float", _embryo_fp);
   embryo_program_native_call_add(ep, "atof", _embryo_fp_str);
   embryo_program_native_call_add(ep, "float_mul", _embryo_fp_mul);
   embryo_program_native_call_add(ep, "float_div", _embryo_fp_div);
   embryo_program_native_call_add(ep, "float_add", _embryo_fp_add);
   embryo_program_native_call_add(ep, "float_sub", _embryo_fp_sub);
   embryo_program_native_call_add(ep, "fract", _embryo_fp_fract);
   embryo_program_native_call_add(ep, "round", _embryo_fp_round);
   embryo_program_native_call_add(ep, "float_cmp", _embryo_fp_cmp);
   embryo_program_native_call_add(ep, "sqrt", _embryo_fp_sqroot);
   embryo_program_native_call_add(ep, "pow", _embryo_fp_power);
   embryo_program_native_call_add(ep, "log", _embryo_fp_log);
   embryo_program_native_call_add(ep, "sin", _embryo_fp_sin);
   embryo_program_native_call_add(ep, "cos", _embryo_fp_cos);
   embryo_program_native_call_add(ep, "tan", _embryo_fp_tan);
   embryo_program_native_call_add(ep, "abs", _embryo_fp_abs);
   /* Added in embryo 1.2 */
   embryo_program_native_call_add(ep, "asin", _embryo_fp_asin);
   embryo_program_native_call_add(ep, "acos", _embryo_fp_acos);
   embryo_program_native_call_add(ep, "atan", _embryo_fp_atan);
   embryo_program_native_call_add(ep, "atan2", _embryo_fp_atan2);
   embryo_program_native_call_add(ep, "log1p", _embryo_fp_log1p);
   embryo_program_native_call_add(ep, "cbrt", _embryo_fp_cbrt);
   embryo_program_native_call_add(ep, "exp", _embryo_fp_exp);
   embryo_program_native_call_add(ep, "exp2", _embryo_fp_exp2);
   embryo_program_native_call_add(ep, "hypot", _embryo_fp_hypot);
}

