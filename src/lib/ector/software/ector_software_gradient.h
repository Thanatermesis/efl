/**
 * @file
 * @brief Header file for Ector software gradient rendering.
 *
 * This file defines structures, macros, and inline functions used for
 * software-based gradient calculations within Ector. It includes constants
 * for color table generation, fixed-point arithmetic, and helper functions
 * for clamping gradient positions and fetching gradient pixels.
 */
#ifndef ECTOR_SOFTWARE_GRADIENT_H
# define ECTOR_SOFTWARE_GRADIENT_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <math.h>

#include <software/Ector_Software.h>

#include "ector_private.h"
#include "ector_software_private.h"
#include "draw.h"

#define GRADIENT_STOPTABLE_SIZE 1024 /**< Size of the pre-calculated gradient color table. Must match the C file. */
#define FIXPT_BITS 8 /**< Number of bits for fixed-point arithmetic precision. Must match the C file. */
#define FIXPT_SIZE (1<<FIXPT_BITS) /**< Scaling factor for fixed-point arithmetic (2^FIXPT_BITS). Must match the C file. */

#define CTABLE_NOT_READY 0  /**< Indicates the gradient color table has not been generated yet. */
#define CTABLE_PROCESSING 1 /**< Indicates the gradient color table is currently being generated. */
#define CTABLE_READY_DONE 2 /**< Indicates the gradient color table has been generated and is ready for use. */

/**
 * @brief Clamps a gradient position based on the gradient spread mode.
 *
 * This function takes an integer position (index into the color table) and
 * adjusts it according to the spread mode (pad, repeat, reflect) defined
 * in the gradient data.
 *
 * @param data Pointer to the gradient data, containing the spread mode.
 * @param ipos The raw integer position/index in the color table.
 * @return The clamped integer position/index.
 */
static inline int
_gradient_clamp(const Ector_Renderer_Software_Gradient_Data *data, int ipos)
{
   int limit;

   if (data->gd->s == EFL_GFX_GRADIENT_SPREAD_REPEAT)
     {
        ipos = ipos % GRADIENT_STOPTABLE_SIZE;
        ipos = ipos < 0 ? GRADIENT_STOPTABLE_SIZE + ipos : ipos;
     }
   else if (data->gd->s == EFL_GFX_GRADIENT_SPREAD_REFLECT)
     {
        limit = GRADIENT_STOPTABLE_SIZE * 2;
        ipos = ipos % limit;
        ipos = ipos < 0 ? limit + ipos : ipos;
        ipos = ipos >= GRADIENT_STOPTABLE_SIZE ? limit - 1 - ipos : ipos;
     }
   else
     {
        if (ipos < 0) ipos = 0;
        else if (ipos >= GRADIENT_STOPTABLE_SIZE)
          ipos = GRADIENT_STOPTABLE_SIZE-1;
     }
   return ipos;
}

/**
 * @brief Retrieves a gradient color using a fixed-point position.
 *
 * Converts a fixed-point position to an integer index, clamps it using
 * _gradient_clamp(), and returns the color from the pre-calculated color table.
 * The fixed-point position is scaled by `FIXPT_SIZE`.
 *
 * @param data Pointer to the gradient data, containing the color table and spread mode.
 * @param fixed_pos The gradient position in fixed-point format.
 *                  Example: `(int)(float_position * FIXPT_SIZE)`
 * @return The ARGB color value at the specified gradient position.
 */
static inline uint32_t
_gradient_pixel_fixed(const Ector_Renderer_Software_Gradient_Data *data, int fixed_pos)
{
   // Convert fixed-point to integer index, with rounding
   int ipos = (fixed_pos + (FIXPT_SIZE / 2)) >> FIXPT_BITS;

   return data->color_table[_gradient_clamp(data, ipos)];
}

/**
 * @brief Retrieves a gradient color using a floating-point position.
 *
 * Converts a floating-point position (0.0 to 1.0 range ideally, but will be
 * handled by spread mode) to an integer index, clamps it using _gradient_clamp(),
 * and returns the color from the pre-calculated color table.
 *
 * @param data Pointer to the gradient data, containing the color table and spread mode.
 * @param pos The gradient position as a float, typically normalized (0.0 to 1.0).
 * @return The ARGB color value at the specified gradient position.
 */
static inline uint32_t
_gradient_pixel(const Ector_Renderer_Software_Gradient_Data *data, float pos)
{
   // Convert float position to integer index, with rounding
   int ipos = (int)(pos * (GRADIENT_STOPTABLE_SIZE - 1) + (float)(0.5));

   return data->color_table[_gradient_clamp(data, ipos)];
}

#endif
