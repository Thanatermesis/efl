/**
 * @file
 * @brief Evas image loader for PSD (Photoshop Document) files.
 *
 * This loader handles reading metadata and pixel data from PSD files,
 * supporting various color modes (Greyscale, Indexed, RGB, CMYK) and
 * both uncompressed and RLE compressed data.
 */

#define _XOPEN_SOURCE 600

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif

#include "evas_common_private.h"
#include "evas_private.h"

/**
 * @brief Represents the header of a PSD file.
 *
 * This structure holds essential metadata about the PSD image,
 * such as its dimensions, color mode, and number of channels.
 */
typedef struct _PSD_Header PSD_Header;

/**
 * @brief Enumerates the possible color modes in a PSD file.
 */
typedef enum _PSD_Mode
  {
    PSD_GREYSCALE = 1, /**< Greyscale color mode. */
    PSD_INDEXED = 2,   /**< Indexed color mode. */
    PSD_RGB = 3,       /**< RGB color mode. */
    PSD_CMYK = 4       /**< CMYK color mode. */
  } PSD_Mode;

/**
 * @struct _PSD_Header
 * @brief Detailed structure of the PSD file header.
 */
struct _PSD_Header
{
   unsigned char signature[4]; /**< File signature, should be "8BPS". */
   unsigned short version;     /**< Version number, should be 1. */
   unsigned char reserved[9];  /**< Reserved bytes, must be zero. (Note: spec says 6 bytes, but code reads 9, likely a typo in struct def or a misunderstanding of spec. The read_block for reserved is 6 bytes long in psd_get_header) */
   unsigned short channels;    /**< Number of color channels in the image, including alpha. Range: 1 to 56. */
   unsigned int height;        /**< Height of the image in pixels. Range: 1 to 30,000. */
   unsigned int width;         /**< Width of the image in pixels. Range: 1 to 30,000. */
   unsigned short depth;       /**< Number of bits per channel. Supported values: 1, 8, 16. */

   unsigned short channel_num; /**< Original number of channels, used for compressed data calculations. */

   PSD_Mode mode;              /**< Color mode of the file. See #PSD_Mode. */
};

/**
 * @brief Status codes for reading compressed channel data.
 */
enum {
  READ_COMPRESSED_SUCCESS,                 /**< Compressed data read successfully. */
  READ_COMPRESSED_ERROR_FILE_CORRUPT,      /**< Compressed data is corrupt. */
  READ_COMPRESSED_ERROR_FILE_READ_ERROR    /**< Error reading file while processing compressed data. */
};

/**
 * @brief Calculates the length of each compressed channel.
 *
 * Reads the RLE (Run-Length Encoding) table which contains the byte counts
 * for each scan line in each channel. It then sums these counts to determine
 * the total compressed length for each channel.
 *
 * @param Head Pointer to the PSD_Header structure.
 * @param map Pointer to the memory-mapped file data.
 * @param length Total length of the mapped data.
 * @param position Current reading position in the mapped data. This will be updated.
 * @param rle_table Buffer to store the RLE byte counts. Must be pre-allocated
 *                  to `Head->height * Head->channel_num * sizeof(unsigned short)`.
 * @param chanlen Output array to store the calculated length of each channel.
 *                Must be pre-allocated to `Head->channel_num * sizeof(unsigned int)`.
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., read error).
 */
static Eina_Bool get_compressed_channels_length(PSD_Header *Head,
                                                const unsigned char *map, size_t length, size_t *position,
                                                unsigned short *rle_table,
                                                unsigned int *chanlen);

/**
 * @brief Reads an unsigned short (2 bytes) from the memory map.
 *
 * Assumes big-endian byte order as per PSD specification.
 *
 * @param map Pointer to the memory-mapped file data.
 * @param length Total length of the mapped data.
 * @param position Current reading position in the mapped data. This will be updated.
 * @param ret Pointer to an unsigned short where the read value will be stored.
 * @return 1 on success, 0 on failure (e.g., reading past end of map).
 */
static int
read_ushort(const unsigned char *map, size_t length, size_t *position, unsigned short *ret)
{
   if (((*position) + 2) > length) return 0;
   // FIXME: need to check order
   *ret = (map[(*position) + 0] << 8) | map[(*position) + 1];
   *position += 2;
   return 1;
}

/**
 * @brief Reads an unsigned int (4 bytes) from the memory map.
 *
 * Assumes big-endian byte order.
 *
 * @param map Pointer to the memory-mapped file data.
 * @param length Total length of the mapped data.
 * @param position Current reading position in the mapped data. This will be updated.
 * @param ret Pointer to an unsigned int where the read value will be stored.
 * @return 1 on success, 0 on failure (e.g., reading past end of map).
 */
static int
read_uint(const unsigned char *map, size_t length, size_t *position, unsigned int *ret)
{
   if (((*position) + 4) > length) return 0;
   // FIXME: need to check order
   *ret = ARGB_JOIN(map[(*position) + 0], map[(*position) + 1], map[(*position) + 2], map[(*position) + 3]);
   *position += 4;
   return 1;
}

/**
 * @brief Reads a block of data from the memory map.
 *
 * @param map Pointer to the memory-mapped file data.
 * @param length Total length of the mapped data.
 * @param position Current reading position in the mapped data. This will be updated.
 * @param target Pointer to the buffer where the read data will be copied.
 * @param size Number of bytes to read.
 * @return 1 on success, 0 on failure (e.g., reading past end of map).
 */
static int
read_block(const unsigned char *map, size_t length, size_t *position, void *target, size_t size)
{
   if (((*position) + size) > length) return 0;
   memcpy(target, map + *position, size);
   *position += size;
   return 1;
}

/**
 * @brief Reads and parses the PSD file header.
 *
 * Populates the PSD_Header structure with data read from the
 * beginning of the PSD file.
 *
 * @param header Pointer to a PSD_Header structure to be filled.
 * @param map Pointer to the memory-mapped file data.
 * @param length Total length of the mapped data.
 * @param position Current reading position in the mapped data. This will be updated.
 * @return EINA_TRUE on successful header parsing, EINA_FALSE otherwise.
 */
static Eina_Bool
psd_get_header(PSD_Header *header, const unsigned char *map, size_t length, size_t *position)
{
   unsigned short tmp;

#define CHECK_RET(Call)                         \
   if (!Call) return EINA_FALSE;

   CHECK_RET(read_block(map, length, position, header->signature, 4));
   CHECK_RET(read_ushort(map, length, position, &header->version));
   CHECK_RET(read_block(map, length, position, header->reserved, 6));
   CHECK_RET(read_ushort(map, length, position, &header->channels));
   CHECK_RET(read_uint(map, length, position, &header->height));
   CHECK_RET(read_uint(map, length, position, &header->width));
   CHECK_RET(read_ushort(map, length, position, &header->depth));

   CHECK_RET(read_ushort(map, length, position, &tmp));
   header->mode = tmp;

#undef CHECK_RET

   return EINA_TRUE;
}

/**
 * @brief Validates the parsed PSD header.
 *
 * Checks if the header fields conform to the PSD specification
 * (e.g., correct signature, version, valid dimensions, depth, channels).
 *
 * @param header Pointer to the PSD_Header structure to validate.
 * @return EINA_TRUE if the header is valid, EINA_FALSE otherwise.
 */
static Eina_Bool
is_psd(PSD_Header *header)
{
   if (strncmp((char*)header->signature, "8BPS", 4))
     return EINA_FALSE;
   if (header->version != 1)
     return EINA_FALSE;
   if (header->channels < 1 || header->channels > 24)
     return EINA_FALSE;
   if (header->height < 1 || header->width < 1)
     return EINA_FALSE;
   if (header->depth != 1 && header->depth != 8 && header->depth != 16)
     return EINA_FALSE;

   return EINA_TRUE;
}

/**
 * @brief Opens a PSD file for loading. (Evas loader plugin function)
 *
 * This function is part of the Evas image loader plugin interface.
 * For PSD, it simply returns the Eina_File handle as loader data.
 *
 * @param f Eina_File handle for the opened file.
 * @param key Unused.
 * @param opts Unused.
 * @param animated Unused.
 * @param error Unused.
 * @return The Eina_File handle `f` as loader_data.
 */
static void *
evas_image_load_file_open_psd(Eina_File *f, Eina_Stringshare *key EINA_UNUSED,
			      Evas_Image_Load_Opts *opts EINA_UNUSED,
			      Evas_Image_Animated *animated EINA_UNUSED,
			      int *error EINA_UNUSED)
{
   return f;
}

/**
 * @brief Closes a PSD file. (Evas loader plugin function)
 *
 * This function is part of the Evas image loader plugin interface.
 * For PSD, it's a no-op as Eina_File is managed externally.
 *
 * @param loader_data Unused.
 */
static void
evas_image_load_file_close_psd(void *loader_data EINA_UNUSED)
{
}

/**
 * @brief Reads the header of a PSD file to get image properties. (Evas loader plugin function)
 *
 * This function maps the file, parses the PSD header, validates it,
 * and populates the Emile_Image_Property structure with image dimensions
 * and alpha channel information.
 *
 * @param loader_data The Eina_File handle returned by evas_image_load_file_open_psd.
 * @param prop Pointer to Emile_Image_Property structure to be filled.
 * @param error Pointer to an integer to store error codes (EVAS_LOAD_ERROR_*).
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
evas_image_load_file_head_psd(void *loader_data,
			      Emile_Image_Property *prop,
			      int *error)
{
   Eina_File *f = loader_data;
   void *map;
   size_t length;
   size_t position;
   PSD_Header header;
   Eina_Bool correct;
   Eina_Bool r = EINA_FALSE;

   *error = EVAS_LOAD_ERROR_NONE;

   map = eina_file_map_all(f, EINA_FILE_RANDOM);
   length = eina_file_size_get(f);
   position = 0;
   if (!map || length < 1)
     {
        *error = EVAS_LOAD_ERROR_CORRUPT_FILE;
	goto on_error;
     }

   correct = psd_get_header(&header, map, length, &position);
   if (!correct || !is_psd(&header))
     {
        *error = EVAS_LOAD_ERROR_UNKNOWN_FORMAT;
	goto on_error;
     }

   prop->w = header.width;
   prop->h = header.height;
   if (header.channels != 3)
     prop->alpha = 1;

   r = EINA_TRUE;

 on_error:
   eina_file_map_free(f, map);
   return r;
}

/**
 * @brief Reads and decompresses a single RLE-compressed channel.
 *
 * PSD uses PackBits RLE compression. This function decodes one channel's data.
 * - A byte `n` from 0 to 127 means copy the next `n+1` bytes literally.
 * - A byte `n` from -1 to -127 means repeat the next byte `-n+1` times.
 * - A byte -128 is a no-op.
 *
 * @param map Pointer to the memory-mapped file data.
 * @param length Total length of the mapped data.
 * @param position Current reading position in the mapped data. This will be updated.
 * @param channel_length The total compressed length of this channel (currently unused in function body, but passed).
 * @param size The expected uncompressed size of the channel data (pixels_count * bytes_per_component).
 * @param channel Output buffer to store the decompressed channel data. Must be pre-allocated.
 * @return READ_COMPRESSED_SUCCESS on success.
 * @return READ_COMPRESSED_ERROR_FILE_CORRUPT if data seems corrupt (e.g., overruns).
 * @return READ_COMPRESSED_ERROR_FILE_READ_ERROR if a read operation fails.
 */
static unsigned int
read_compressed_channel(const unsigned char *map, size_t length, size_t *position,
			const unsigned int channel_length EINA_UNUSED,
                        unsigned int size,
			unsigned char* channel)
{
   // FIXME: what does channel_length means, and why is it not used
   unsigned int i;
   signed char headbyte;
   unsigned char c;

#define CHECK_RET(Call)                                         \
   if (!Call) return READ_COMPRESSED_ERROR_FILE_READ_ERROR;	\

   for (i = 0; i < size; )
     {
        CHECK_RET(read_block(map, length, position, &headbyte, 1));

        if (headbyte >= 0)
          {
             if (i + headbyte > size)
               return READ_COMPRESSED_ERROR_FILE_CORRUPT;
	     CHECK_RET(read_block(map, length, position, channel + i, headbyte + 1));

             i += headbyte + 1;
          }
        else if (headbyte >= -127)
          {
             int run;

	     CHECK_RET(read_block(map, length, position, &c, 1));

             run = c;
             /* if (run == -1) */
	     /*   return READ_COMPRESSED_ERROR_FILE_READ_ERROR; */

             if (i + (-headbyte + 1) > size)
               return READ_COMPRESSED_ERROR_FILE_CORRUPT;

             memset(channel + i, run, -headbyte + 1);
             i += -headbyte + 1;
          }
     }

#undef CHECK_RET

   return READ_COMPRESSED_SUCCESS;
}

/**
 * @brief Reads and processes pixel data from the PSD file.
 *
 * This function handles both uncompressed and RLE compressed pixel data.
 * It reads channel data plane by plane, then interleaves it into the
 * output buffer. It also handles alpha channel accumulation for formats
 * with more than 3/4 channels.
 *
 * @param head Pointer to the PSD_Header structure.
 * @param map Pointer to the memory-mapped file data.
 * @param length Total length of the mapped data.
 * @param position Current reading position in the mapped data. This will be updated.
 * @param buffer Output buffer to store the final RGBA pixel data.
 *               Must be pre-allocated to `head->width * head->height * 4`.
 * @param compressed EINA_TRUE if the data is RLE compressed, EINA_FALSE otherwise.
 * @param error Pointer to an integer to store error codes (EVAS_LOAD_ERROR_*).
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
psd_get_data(PSD_Header *head,
             const unsigned char *map, size_t length, size_t *position,
	     unsigned char *buffer, Eina_Bool compressed,
	     int *error)
{
   unsigned int c, x, y, numchan, bps, bpc, bpp;
   unsigned int pixels_count;
   unsigned char *channel = NULL;
   unsigned char *data = NULL;

   // Added 01-07-2009: This is needed to correctly load greyscale and
   //  paletted images.
   switch (head->mode)
     {
      case PSD_GREYSCALE:
      case PSD_INDEXED:
         numchan = 1;
         break;
      default:
         numchan = 3;
     }

   bpp = head->channels;
   bpc = head->depth / 8;
   pixels_count = head->width * head->height;

   data = malloc(sizeof (unsigned char) * pixels_count * bpp);
   if (!data) return EINA_FALSE;

   channel = malloc(sizeof (unsigned char) * pixels_count * bpc);
   if (!channel)
     {
        free(data);
        return EINA_FALSE;
     }

   bps = head->width * head->channels * bpc;
   // @TODO: Add support for this in, though I have yet to run across a .psd
   //	file that uses this.
   if (compressed && bpc == 2)
     {
        free(data);
	free(channel);
        *error = EVAS_LOAD_ERROR_UNKNOWN_FORMAT;
        return EINA_FALSE;
     }

#define CHECK_RET(Call)                         \
   if (!Call)                                   \
     {						\
        free(data);				\
        free(channel);				\
        return EINA_FALSE;			\
     }

   if (!compressed)
     {
        if (bpc == 1)
          {
             for (c = 0; c < numchan; c++)
               {
                  unsigned char *tmp = channel;

                  CHECK_RET(read_block(map, length, position, tmp, pixels_count));

                  for (y = 0; y < head->height * bps; y += bps)
                    {
                       for (x = 0; x < bps; x += bpp, tmp++)
                         {
                            data[y + x + c] = *tmp;
                         }
                    }
               }

             // Accumulate any remaining channels into a single alpha channel
             //@TODO: This needs to be changed for greyscale images.
             for (; c < head->channels; c++)
               {
                  unsigned char *tmp = channel;

                  CHECK_RET(read_block(map, length, position, channel, pixels_count));

                  for (y = 0; y < head->height * bps; y += bps)
                    {
                       for (x = 0; x < bps; x += bpp, tmp++)
                         {
                            unsigned short newval;

                            // previous formula was : (old / 255 * new / 255) * 255
                            newval = (*tmp) * data[y + x + 3];

                            data[y + x + 3] = newval >> 8;
                         }
                    }
               }
          }
        else
          {
             int bps2;

             bps2 = bps / 2;

             // iCurImage->Bpc == 2
             for (c = 0; c < numchan; c++)
               {
                  unsigned short *shortptr = (unsigned short*) channel;

                  CHECK_RET(read_block(map, length, position, channel, pixels_count * 2));

                  for (y = 0; y < head->height * bps2; y += bps2)
                    {
                       for (x = 0; x < (unsigned int)bps2; x += bpp, shortptr++)
                         {
                            ((unsigned short*)data)[y + x + c] = *shortptr;
                         }
                    }
               }

             // Accumulate any remaining channels into a single alpha channel
             //@TODO: This needs to be changed for greyscale images.
             for (; c < head->channels; c++)
               {
		  unsigned short *shortptr = (unsigned short*) channel;

		  CHECK_RET(read_block(map, length, position, channel, pixels_count * 2));

		  for (y = 0; y < head->height * bps2; y += bps2)
                    {
                       for (x = 0; x < (unsigned int)bps2; x += bpp, shortptr++)
                         {
                            unsigned int newval;

                            newval = *shortptr * ((unsigned short*)data)[y + x + 3];

                            ((unsigned short*)data)[y + x + 3] = newval >> 16;
                         }
                    }
               }
          }
     }
   else
     {
        unsigned short *rle_table;
	unsigned int *chanlen;

	rle_table = alloca(head->height * head->channel_num * sizeof (unsigned short));
	chanlen = alloca(head->channel_num * sizeof (unsigned int));
        if (!get_compressed_channels_length(head, map, length, position, rle_table, chanlen))
	  goto file_read_error;

        for (c = 0; c < numchan; c++)
          {
	     unsigned char *tmp = channel;
	     int err;

	     err = read_compressed_channel(map, length, position,
					   chanlen[c],
					   pixels_count,
					   channel);
             if (err == READ_COMPRESSED_ERROR_FILE_CORRUPT)
               goto file_corrupt;
             else if (err == READ_COMPRESSED_ERROR_FILE_READ_ERROR)
               goto file_read_error;

             for (y = 0; y < head->height * bps; y += bps)
               {
		  for (x = 0; x < bps; x += bpp, tmp++)
                    {
                       data[y + x + c] = *tmp;
                    }
               }
          }

        // Initialize the alpha channel to solid
        //@TODO: This needs to be changed for greyscale images.
        if (head->channels >= 4)
          {
	     for (y = 0; y < head->height * bps; y += bps)
               {
                  for (x = 0; x < bps; x += bpp)
		    {
                       data[y + x + 3] = 255;
		    }
               }

             for (; c < head->channels; c++)
               {
		  unsigned char *tmp = channel;
		  int err;

                  err = read_compressed_channel(map, length, position,
						chanlen[c],
						pixels_count,
						channel);
                  if (err == READ_COMPRESSED_ERROR_FILE_CORRUPT)
                    goto file_corrupt;
                  else if (err == READ_COMPRESSED_ERROR_FILE_READ_ERROR)
                    goto file_read_error;

                  for (y = 0; y < head->height * bps; y += bps)
                    {
		       for (x = 0; x < bps; x += bpp, tmp++)
                         {
			    unsigned short newval;

			    newval = *tmp * data[y + x + 3];

			    data[y + x + 3] = newval >> 8;
                         }
                    }
               }
          }
     }

   if (bpp == 3)
     {
        for (x = 0; x < pixels_count; x++)
          {
             buffer[x * 4 + 0] = data[(x * 3) + 2];
             buffer[x * 4 + 1] = data[(x * 3) + 1];
             buffer[x * 4 + 2] = data[(x * 3) + 0];
             buffer[x * 4 + 3] = 255;
          }
     }
   else if (bpp == 4)
     {
        // BRGA to RGBA
        for (x= 0; x < pixels_count; x++)
          {
             buffer[x * 4 + 0] = data[(x * 4) + 2];
             buffer[x * 4 + 1] = data[(x * 4) + 1];
             buffer[x * 4 + 2] = data[(x * 4) + 0];
             buffer[x * 4 + 3] = data[(x * 4) + 3];
          }
     }
   else
     {
        // can;'t handle non rgb formats
        *error = EVAS_LOAD_ERROR_UNKNOWN_FORMAT;
        goto file_read_error;
     }

   free(channel);
   free(data);
   return EINA_TRUE;

#undef CHECK_RET

 file_corrupt:
   *error = EVAS_LOAD_ERROR_CORRUPT_FILE;

 file_read_error:
   free(channel);
   free(data);

   return EINA_FALSE;
}

/**
 * @brief Reads a single channel's data, handling compression.
 *
 * This function is used primarily for CMYK images to read the 'K' (black)
 * channel separately after the CMY channels have been processed by psd_get_data.
 * It supports both uncompressed and RLE compressed data.
 *
 * @param head Pointer to the PSD_Header structure.
 * @param map Pointer to the memory-mapped file data.
 * @param length Total length of the mapped data.
 * @param position Current reading position in the mapped data. This will be updated.
 * @param buffer Output buffer to store the channel data.
 *               Must be pre-allocated to `head->width * head->height * (head->depth / 8)`.
 * @param compressed EINA_TRUE if the data is RLE compressed, EINA_FALSE otherwise.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
get_single_channel(PSD_Header *head,
		   const unsigned char *map, size_t length, size_t *position,
		   unsigned char *buffer,
		   Eina_Bool compressed)
{
   unsigned int i, bpc;
   signed char headbyte;
   int c;
   int pixels_count;

   bpc = (head->depth / 8);
   pixels_count = head->width * head->height;

#define CHECK_RET(Call)                  \
   if (!Call) return EINA_FALSE;

   if (!compressed)
     {
        if (bpc == 1)
          {
             CHECK_RET(read_block(map, length, position, buffer, pixels_count));
          }
        else
          {  // Bpc == 2
             CHECK_RET(read_block(map, length, position, buffer, pixels_count * 2));
          }
     }
   else
     {
        for (i = 0; i < (unsigned int)pixels_count; )
          {
             CHECK_RET(read_block(map, length, position, &headbyte, 1));

             if (headbyte >= 0)
               {  //  && HeadByte <= 127
                  CHECK_RET(read_block(map, length, position, buffer + i, headbyte + 1));

                  i += headbyte + 1;
               }
             if (headbyte >= -127 && headbyte <= -1)
               {
                  int run;

                  CHECK_RET(read_block(map, length, position, &c, 1));

                  run = c;
                  if (run == -1) return EINA_FALSE;

                  memset(buffer + i, run, -headbyte + 1);
                  i += -headbyte + 1;
               }
          }
     }

#undef CHECK_RET

   return EINA_TRUE;
}

/**
 * @brief Reads pixel data for Greyscale PSD images.
 *
 * Skips Color Mode Data, Image Resources, and Layer/Mask Info sections.
 * Then calls psd_get_data to read the actual pixel data.
 *
 * @param pixels Output buffer for RGBA pixel data.
 * @param head Pointer to the PSD_Header structure.
 * @param map Pointer to the memory-mapped file data.
 * @param length Total length of the mapped data.
 * @param position Current reading position in the mapped data. This will be updated.
 * @param error Pointer to an integer to store error codes (EVAS_LOAD_ERROR_*).
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
read_psd_grey(void *pixels, PSD_Header *head, const unsigned char *map, size_t length, size_t *position, int *error)
{
   unsigned int color_mode, resource_size, misc_info;
   unsigned short compressed;

   *error = EVAS_LOAD_ERROR_CORRUPT_FILE;

#define CHECK_RET(Call)                  \
   if (!Call) return EINA_FALSE;

   CHECK_RET(read_uint(map, length, position, &color_mode));
   // Skip over the 'color mode data section'
   *position += color_mode;
   if ((*position) >= length) return EINA_FALSE;

   CHECK_RET(read_uint(map, length, position, &resource_size));
   // Read the 'image resources section'
   *position += resource_size;
   if ((*position) >= length) return EINA_FALSE;

   CHECK_RET(read_uint(map, length, position, &misc_info));
   *position += misc_info;
   if ((*position) >= length) return EINA_FALSE;

   CHECK_RET(read_ushort(map, length, position, &compressed));
   if (compressed != 0) compressed = EINA_TRUE;

   head->channel_num = head->channels;
   // Temporary to read only one channel...some greyscale .psd files have 2.
   head->channels = 1;

   switch (head->depth)
     {
      case 8:
      case 16:
         break;
      default:
         *error = EVAS_LOAD_ERROR_UNKNOWN_FORMAT;
         return EINA_FALSE;
     }

   if (!psd_get_data(head, map, length, position, pixels, compressed, error))
     goto cleanup_error;

   return EINA_TRUE;

#undef CHECK_RET

 cleanup_error:
   return EINA_FALSE;
}

/**
 * @brief Reads pixel data for Indexed Color PSD images.
 *
 * Skips Color Mode Data (palette), Image Resources, and Layer/Mask Info sections.
 * Then calls psd_get_data to read the indexed pixel data, which psd_get_data
 * will treat as single-channel data. The actual palette conversion to RGB
 * is not explicitly handled here, implying psd_get_data might produce grayscale
 * or that the Evas pipeline handles palette conversion later if needed.
 *
 * @param pixels Output buffer for RGBA pixel data.
 * @param head Pointer to the PSD_Header structure.
 * @param map Pointer to the memory-mapped file data.
 * @param length Total length of the mapped data.
 * @param position Current reading position in the mapped data. This will be updated.
 * @param error Pointer to an integer to store error codes (EVAS_LOAD_ERROR_*).
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
read_psd_indexed(void *pixels, PSD_Header *head, const unsigned char *map, size_t length, size_t *position, int *error)
{
   unsigned int color_mode, resource_size, misc_info;
   unsigned short compressed;

   *error = EVAS_LOAD_ERROR_CORRUPT_FILE;

#define CHECK_RET(Call)                  \
   if (!(Call)) return EINA_FALSE;

   CHECK_RET(read_uint(map, length, position, &color_mode));
   CHECK_RET(!(color_mode % 3));
   /*
     Palette = (unsigned char*)malloc(Colormode);
     if (Palette == NULL)
     return EINA_FALSE;
     if (fread(&Palette, 1, Colormode, file) != Colormode)
     goto cleanup_error;
   */
   // Skip over the 'color mode data section'
   *position += color_mode;
   if ((*position) >= length) return EINA_FALSE;

   // Read the 'image resources section'
   CHECK_RET(read_uint(map, length, position, &resource_size));
   *position += resource_size;
   if ((*position) >= length) return EINA_FALSE;

   CHECK_RET(read_uint(map, length, position, &misc_info));
   *position += misc_info;
   if ((*position) >= length) return EINA_FALSE;

   CHECK_RET(read_ushort(map, length, position, &compressed));
   if (compressed != 0) compressed = EINA_TRUE;

   if (head->channels != 1 || head->depth != 8)
     {
        *error = EVAS_LOAD_ERROR_UNKNOWN_FORMAT;
        return EINA_FALSE;
     }
   head->channel_num = head->channels;

   if (!psd_get_data(head, map, length, position, pixels, compressed, error))
     return EINA_FALSE;
   return EINA_TRUE;

#undef CHECK_RET
}

/**
 * @brief Reads pixel data for RGB Color PSD images.
 *
 * Skips Color Mode Data, Image Resources, and Layer/Mask Info sections.
 * Then calls psd_get_data to read the RGB (and possibly Alpha) pixel data.
 *
 * @param pixels Output buffer for RGBA pixel data.
 * @param head Pointer to the PSD_Header structure.
 * @param map Pointer to the memory-mapped file data.
 * @param length Total length of the mapped data.
 * @param position Current reading position in the mapped data. This will be updated.
 * @param error Pointer to an integer to store error codes (EVAS_LOAD_ERROR_*).
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
read_psd_rgb(void *pixels, PSD_Header *head, const unsigned char *map, size_t length, size_t *position, int *error)
{
   unsigned int color_mode, resource_size, misc_info;
   unsigned short compressed;

#define CHECK_RET(Call)                  \
   if (!Call) return EINA_FALSE;

   CHECK_RET(read_uint(map, length, position, &color_mode));
   // Skip over the 'color mode data section'
   *position += color_mode;
   if ((*position) >= length) return EINA_FALSE;

   // Read the 'image resources section'
   CHECK_RET(read_uint(map, length, position, &resource_size));
   *position += resource_size;
   if ((*position) >= length) return EINA_FALSE;

   CHECK_RET(read_uint(map, length, position, &misc_info));
   *position += misc_info;
   if ((*position) >= length) return EINA_FALSE;

   CHECK_RET(read_ushort(map, length, position, &compressed));
   if (compressed != 0) compressed = EINA_TRUE;

   head->channel_num = head->channels;

   switch (head->depth)
     {
      case 8:
      case 16:
         break;
      default:
         *error = EVAS_LOAD_ERROR_UNKNOWN_FORMAT;
         return EINA_FALSE;
     }

   if (!psd_get_data(head, map, length, position, pixels, compressed, error))
     return EINA_FALSE;

   return EINA_TRUE;

#undef CHECK_RET
}

/**
 * @brief Reads pixel data for CMYK Color PSD images.
 *
 * Skips Color Mode Data, Image Resources, and Layer/Mask Info sections.
 * Calls psd_get_data to read CMY channels (treating it as RGB temporarily).
 * Then reads the K (black) channel separately using get_single_channel.
 * Finally, it converts CMYK to RGB using a simple formula:
 * R = C * K / 255 (and similarly for G, B).
 * If an alpha channel is present (5 channels total), it's handled as alpha.
 *
 * @param prop Pointer to Emile_Image_Property (used for width/height).
 * @param pixels Output buffer for RGBA pixel data.
 * @param head Pointer to the PSD_Header structure.
 * @param map Pointer to the memory-mapped file data.
 * @param length Total length of the mapped data.
 * @param position Current reading position in the mapped data. This will be updated.
 * @param error Pointer to an integer to store error codes (EVAS_LOAD_ERROR_*).
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
read_psd_cmyk(Emile_Image_Property *prop, void *pixels, PSD_Header *head, const unsigned char *map, size_t length, size_t *position, int *error)
{
   unsigned int color_mode, resource_size, misc_info, size, j, data_size;
   unsigned short compressed;
   unsigned int format, type;
   unsigned char *kchannel = NULL;
   Eina_Bool r = EINA_FALSE;

   *error = EVAS_LOAD_ERROR_CORRUPT_FILE;

#define CHECK_RET(Call)                  \
   if (!Call) return EINA_FALSE;

   CHECK_RET(read_uint(map, length, position, &color_mode));
   // Skip over the 'color mode data section'
   *position += color_mode;
   if ((*position) >= length) return EINA_FALSE;

   CHECK_RET(read_uint(map, length, position, &resource_size));
   // Read the 'image resources section'
   *position += resource_size;
   if ((*position) >= length) return EINA_FALSE;

   CHECK_RET(read_uint(map, length, position, &misc_info));
   *position += misc_info;
   if ((*position) >= length) return EINA_FALSE;

   CHECK_RET(read_ushort(map, length, position, &compressed));
   if (compressed != 0) compressed = EINA_TRUE;

   switch (head->channels)
     {
      case 4:
         format = 0x1907;
         head->channel_num = 4;
         head->channels = 3;
         break;
      case 5:
         format = 0x1908;
         head->channel_num = 5;
         head->channels = 4;
         break;
      default:
         *error = EVAS_LOAD_ERROR_UNKNOWN_FORMAT;
         return EINA_FALSE;
     }

   switch (head->depth)
     {
      case 8:
         type = 1;
         break;
      case 16:
         type = 2;
         break;
      default:
         *error = EVAS_LOAD_ERROR_UNKNOWN_FORMAT;
         return EINA_FALSE;
     }

   if (!psd_get_data(head, map, length, position, pixels, compressed, error))
     goto cleanup_error;

   size = type * prop->w * prop->h;
   kchannel = malloc(size);
   if (kchannel == NULL)
     goto cleanup_error;
   if (!get_single_channel(head, map, length, position, kchannel, compressed))
     goto cleanup_error;

   data_size = head->channels * type * prop->w * prop->h;
   if (format == 0x1907)
     {
        unsigned char *tmp = pixels;
        const unsigned char *limit = tmp + data_size;

        for (j = 0; tmp < limit; tmp++, j++)
          {
             int k;

             for (k = 0; k < 3; k++)
               *tmp = (*tmp * kchannel[j]) >> 8;

             // FIXME: tmp[i+3] = 255;
          }
     }
   else
     {  // RGBA
        unsigned char *tmp = pixels;
        const unsigned char *limit = tmp + data_size;

        // The KChannel array really holds the alpha channel on this one.
        for (j = 0; tmp < limit; tmp += 4, j++)
          {
             tmp[0] = (tmp[0] * tmp[3]) >> 8;
             tmp[1] = (tmp[1] * tmp[3]) >> 8;
             tmp[2] = (tmp[2] * tmp[3]) >> 8;
             tmp[3] = kchannel[j];  // Swap 'K' with alpha channel.
          }
     }

   r = EINA_TRUE;

 cleanup_error:
   free(kchannel);
   return r;
}

/**
 * @brief Loads the actual image data from a PSD file. (Evas loader plugin function)
 *
 * This function is the main entry point for loading pixel data after the
 * header has been parsed. It maps the file, re-parses and validates the header,
 * then dispatches to color mode specific functions (read_psd_grey,
 * read_psd_indexed, read_psd_rgb, read_psd_cmyk) to load the pixel data
 * into the provided buffer.
 *
 * @param loader_data The Eina_File handle.
 * @param prop Pointer to Emile_Image_Property containing expected image properties.
 *             Used to verify against the re-parsed header.
 * @param pixels Output buffer to store the final RGBA pixel data.
 * @param error Pointer to an integer to store error codes (EVAS_LOAD_ERROR_*).
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
evas_image_load_file_data_psd(void *loader_data,
			      Emile_Image_Property *prop,
			      void *pixels,
			      int *error)
{
   Eina_File *f = loader_data;

   void *map;
   size_t length;
   size_t position;
   PSD_Header header;
   Eina_Bool bpsd = EINA_FALSE;

   map = eina_file_map_all(f, EINA_FILE_SEQUENTIAL);
   length = eina_file_size_get(f);
   position = 0;
   *error = EVAS_LOAD_ERROR_CORRUPT_FILE;
   if (!map || length < 1)
     goto on_error;

   *error = EVAS_LOAD_ERROR_GENERIC;
   if (!psd_get_header(&header, map, length, &position) || !is_psd(&header))
     goto on_error;

   if (header.width != prop->w ||
       header.height != prop->h)
     goto on_error;

   *error = EVAS_LOAD_ERROR_NONE;

   switch (header.mode)
     {
      case PSD_GREYSCALE:  // Greyscale
         bpsd = read_psd_grey(pixels, &header, map, length, &position, error);
         break;
      case PSD_INDEXED:  // Indexed
         bpsd = read_psd_indexed(pixels, &header, map, length, &position, error);
         break;
      case PSD_RGB:  // RGB
         bpsd = read_psd_rgb(pixels, &header, map, length, &position, error);
	 prop->premul = EINA_TRUE;
         break;
      case PSD_CMYK:  // CMYK
         bpsd = read_psd_cmyk(prop, pixels, &header, map, length, &position, error);
         prop->premul = EINA_TRUE;
         break;
      default :
         *error = EVAS_LOAD_ERROR_UNKNOWN_FORMAT;
         bpsd = EINA_FALSE;
     }

 on_error:
   if (map) eina_file_map_free(f, map);

   return bpsd;
}

/*
 * get_compressed_channels_length is defined earlier with Doxygen comments.
 * This is the actual implementation.
 */
static Eina_Bool
get_compressed_channels_length(PSD_Header *head,
                               const unsigned char *map, size_t length, size_t *position,
			       unsigned short *rle_table,
			       unsigned int *chanlen)
{
   unsigned int j;
   unsigned int c;

   if (!read_block(map, length, position, rle_table,
                   sizeof (unsigned short) * head->height * head->channel_num))
     return EINA_FALSE;

   memset(chanlen, 0, head->channel_num * sizeof(unsigned int));
   for (c = 0; c < head->channel_num; c++)
     {
        unsigned int i;

        j = c * head->height;
        for (i = 0; i < head->height; i++)
          {
	     chanlen[c] += rle_table[i + j];
          }
     }

   return EINA_TRUE;
}

static const Evas_Image_Load_Func evas_image_load_psd_func = {
  EVAS_IMAGE_LOAD_VERSION,
  evas_image_load_file_open_psd,
  evas_image_load_file_close_psd,
  (void*) evas_image_load_file_head_psd,
  NULL,
  (void*) evas_image_load_file_data_psd,
  NULL,
  EINA_TRUE,
  EINA_FALSE
};

/**
 * @brief Opens the Evas image loader module for PSD. (Evas module function)
 *
 * Initializes the module by setting up the function pointers for the
 * Evas image loading API.
 *
 * @param em Pointer to the Evas_Module structure.
 * @return 1 on success, 0 on failure.
 */
static int
module_open(Evas_Module *em)
{
   if (!em) return 0;
   em->functions = (void *)(&evas_image_load_psd_func);
   return 1;
}

/**
 * @brief Closes the Evas image loader module for PSD. (Evas module function)
 *
 * Currently a no-op for this loader.
 *
 * @param em Unused.
 */
static void
module_close(Evas_Module *em EINA_UNUSED)
{
}

/**
 * @brief Evas module API structure for the PSD loader.
 */
static Evas_Module_Api evas_modapi =
  {
    EVAS_MODULE_API_VERSION,
    "psd",
    "none",
    {
      module_open,
      module_close
    }
  };

EVAS_MODULE_DEFINE(EVAS_MODULE_TYPE_IMAGE_LOADER, image_loader, psd);


#ifndef EVAS_STATIC_BUILD_PSD
EVAS_EINA_MODULE_DEFINE(image_loader, psd);
#endif
