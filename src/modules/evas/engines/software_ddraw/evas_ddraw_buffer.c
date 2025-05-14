#include <string.h>

#include "evas_common_private.h"
#include "evas_engine.h"


/**
 * @brief Creates a new DirectDraw output buffer.
 *
 * Allocates and initializes a DD_Output_Buffer structure. If @p data is NULL,
 * it allocates memory for the buffer data itself.
 *
 * @param width The width of the buffer in pixels.
 * @param height The height of the buffer in pixels.
 * @param data Optional pointer to pre-allocated buffer data. If NULL, memory
 *             will be allocated internally. The data is expected to be in
 *             32-bit ARGB format.
 * @return A pointer to the newly created DD_Output_Buffer, or NULL on failure.
 */
DD_Output_Buffer *
evas_software_ddraw_output_buffer_new(int   width,
                                      int   height,
                                      void *data)
{
   DD_Output_Buffer *ddob;

   ddob = calloc(1, sizeof(DD_Output_Buffer));
   if (!ddob) return NULL;

   ddob->data = data;
   ddob->width = width;
   ddob->height = height;
   ddob->pitch = width * 4;
   ddob->psize = ddob->pitch * height;

   if (!ddob->data)
     {
        ddob->data = malloc(ddob->pitch * height);
        if (!ddob->data)
          {
            free(ddob);
            return NULL;
          }
     }

   return ddob;
}

/**
 * @brief Frees a DirectDraw output buffer.
 *
 * Releases the memory associated with the DD_Output_Buffer, including the
 * buffer data if it was allocated internally.
 *
 * @param ddob The DirectDraw output buffer to free.
 */
void
evas_software_ddraw_output_buffer_free(DD_Output_Buffer *ddob)
{
   /* Free the internally allocated data buffer if it exists */
   if (ddob->data) free(ddob->data);
   free(ddob);
}

/**
 * @brief Copies the content of the Evas buffer to a DirectDraw surface area.
 *
 * Performs a clipped copy from the source Evas buffer (@p ddob) to the
 * destination DirectDraw surface data (@p ddraw_data).
 *
 * @param ddob The source Evas DirectDraw output buffer.
 * @param ddraw_data Pointer to the destination DirectDraw surface memory.
 *                   Assumed to be in a compatible pixel format (e.g., 32-bit).
 * @param ddraw_width The width of the destination DirectDraw surface.
 * @param ddraw_height The height of the destination DirectDraw surface.
 * @param ddraw_pitch The pitch (bytes per scanline) of the destination DirectDraw surface.
 * @param x The horizontal offset (in pixels) on the destination surface where the copy should start.
 * @param y The vertical offset (in pixels) on the destination surface where the copy should start.
 */
void
evas_software_ddraw_output_buffer_paste(DD_Output_Buffer *ddob,
                                        void             *ddraw_data,
                                        int               ddraw_width,
                                        int               ddraw_height,
                                        int               ddraw_pitch,
                                        int               x,
                                        int               y)
{
   DATA8 *dd_data;
   DATA8 *evas_data;
   int    width;
   int    height;
   int    pitch;
   int    j;

   if ((x >= ddraw_width) || (y >= ddraw_height))
     return;

   /* compute the size of the data to copy on the back surface */
   width = ((x + ddob->width) > ddraw_width)
     ? ddraw_width - x
     : ddob->width;
   height = ((y + ddob->height) > ddraw_height)
     ? ddraw_height - y
     : ddob->height;
   pitch = width * 4;

   dd_data = (DATA8 *)ddraw_data + y * ddraw_pitch + x * 32;
   evas_data = (unsigned char *)ddob->data;
   for (j = 0; j < height; j++, evas_data += ddob->pitch, dd_data += ddraw_pitch)
     memcpy(dd_data, evas_data, pitch);
}

/**
 * @brief Retrieves the raw pixel data and pitch of the Evas buffer.
 *
 * Provides access to the underlying pixel data buffer.
 *
 * @param ddob The DirectDraw output buffer.
 * @param bytes_per_line_ret If not NULL, the pitch (bytes per scanline) of the buffer
 *                           will be stored in the location pointed to by this parameter.
 * @return A pointer to the raw pixel data (DATA8* which is typically unsigned char*).
 *         The data is arranged row by row, with each pixel usually being 32 bits (ARGB).
 *         Example structure for a 2x2 buffer:
 *         [R1G1B1A1, R1G1B1A1]  <- Row 1
 *         [R2G2B2A2, R2G2B2A2]  <- Row 2
 *         Where each R, G, B, A is a byte.
 */
DATA8 *
evas_software_ddraw_output_buffer_data(DD_Output_Buffer *ddob,
                                       int              *bytes_per_line_ret)
{
   if (bytes_per_line_ret) *bytes_per_line_ret = ddob->pitch;
   return ddob->data;
}
