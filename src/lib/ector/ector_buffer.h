#ifndef ECTOR_BUFFER_H
#define ECTOR_BUFFER_H

#include <Ector.h>

/**
 * @typedef Ector_Buffer
 * A generic pixel buffer type (2D). May be readable or writeable or both.
 */
typedef Eo Ector_Buffer;

/* Avoid type redefinition */
#define _ECTOR_BUFFER_EO_CLASS_TYPE

#include "ector_buffer.eo.h"
#include "software/ector_software_buffer_base.eo.h"


typedef struct _Ector_Buffer_Data Ector_Buffer_Data;
typedef struct _Ector_Software_Buffer_Base_Data Ector_Software_Buffer_Base_Data;

/**
 * @internal
 * @brief Internal data structure for Ector_Buffer.
 * This structure holds the common properties of a buffer, such as its dimensions,
 * colorspace, and immutability status.
 */
struct _Ector_Buffer_Data
{
   Ector_Buffer       *eo; /**< The Evas_Object instance this data belongs to. */
   unsigned int        w, h; /**< Width and height of the buffer in pixels. */
   Efl_Gfx_Colorspace  cspace; /**< Colorspace of the buffer. See #Efl_Gfx_Colorspace. */
   Eina_Bool           immutable : 1; /**< EINA_TRUE if the pixel data cannot be modified via pixels_set, EINA_FALSE otherwise. */
};

/**
 * @internal
 * @brief Internal data structure for software-based Ector_Buffer.
 * This structure extends Ector_Buffer_Data with properties specific to
 * software-rendered buffers, such as the pixel data pointer and stride.
 */
struct _Ector_Software_Buffer_Base_Data
{
   Ector_Buffer_Data *generic; /**< Pointer to the generic buffer data. */
   union {
      unsigned int     *u32; /**< Pointer to pixel data as 32-bit unsigned integers. Used for colorspaces like ARGB8888. */
      unsigned char    *u8;  /**< Pointer to pixel data as 8-bit unsigned characters. Used for colorspaces like GRY8. */
   } pixels; /**< Union for accessing pixel data based on the colorspace and pixel format. */
   unsigned int         stride; /**< The number of bytes from the start of one row of pixels to the start of the next. Also known as pitch. */
   unsigned int         pixel_size; /**< Size of a single pixel in bytes (e.g., 4 for ARGB8888, 1 for GRY8). */
   struct {
      Eina_Inlist      *maps; /**< A list of currently active memory mappings (Ector_Software_Buffer_Map) for this buffer. */
   } internal; /**< Internal data used for managing buffer state, like memory maps. */
   Eina_Bool            writable : 1; /**< EINA_TRUE if the pixel data can be directly written to, EINA_FALSE otherwise. */
   Eina_Bool            nofree : 1; /**< EINA_TRUE if the pixel data memory should not be freed when the buffer is destroyed (e.g., if it's externally managed). EINA_FALSE if ector is responsible for freeing it. */
};

#endif
