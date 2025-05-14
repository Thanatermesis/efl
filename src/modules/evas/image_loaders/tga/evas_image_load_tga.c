#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>

#include "evas_common_private.h"
#include "evas_private.h"

/* TGA pixel formats */
#define TGA_TYPE_MAPPED      1 // handle
#define TGA_TYPE_COLOR       2
#define TGA_TYPE_GRAY        3
#define TGA_TYPE_MAPPED_RLE  9 // handle
#define TGA_TYPE_COLOR_RLE  10
#define TGA_TYPE_GRAY_RLE   11

/* TGA header flags */
#define TGA_DESC_ABITS      0x0f
#define TGA_DESC_HORIZONTAL 0x10
#define TGA_DESC_VERTICAL   0x20

#define TGA_SIGNATURE "TRUEVISION-XFILE"

typedef struct _tga_header tga_header;
typedef struct _tga_footer tga_footer;

/**
 * @brief Defines the header for a TGA (Truevision Targa) image file.
 *
 * This structure is packed to ensure it precisely matches the layout of a TGA
 * header on disk. It contains metadata about the image such as dimensions,
 * pixel depth, and image type.
 */
struct _tga_header
{
   unsigned char       idLength;
   unsigned char       colorMapType;
   unsigned char       imageType;
   unsigned char       colorMapIndexLo, colorMapIndexHi;
   unsigned char       colorMapLengthLo, colorMapLengthHi;
   unsigned char       colorMapSize;
   unsigned char       xOriginLo, xOriginHi;
   unsigned char       yOriginLo, yOriginHi;
   unsigned char       widthLo, widthHi;
   unsigned char       heightLo, heightHi;
   unsigned char       bpp;
   unsigned char       descriptor;
} __attribute__((packed));

/**
 * @brief Defines the footer for a TGA (Truevision Targa) image file.
 *
 * This optional footer provides a way to verify that a file is a valid TGA
 * file (using the signature). It is part of the TGA 2.0 specification. It
 * is packed to match the on-disk format.
 */
struct _tga_footer
{
   unsigned int        extensionAreaOffset;
   unsigned int        developerDirectoryOffset;
   char                signature[16];
   char                dot;
   char                null;
} __attribute__((packed));

/**
 * @brief Opens a TGA file for loading.
 *
 * This function is the entry point for loading a TGA image. It is called by
 * Evas to open a file and prepare it for reading. In this TGA loader, it
 * simply returns the file handle as no special opening procedure is needed.
 *
 * @param f The Eina_File handle to the TGA file.
 * @param key Unused.
 * @param opts Unused.
 * @param animated Unused.
 * @param error Unused.
 * @return A void pointer to the loader data, which is the file handle itself.
 */
static void *
evas_image_load_file_open_tga(Eina_File *f, Eina_Stringshare *key EINA_UNUSED,
                              Evas_Image_Load_Opts *opts EINA_UNUSED,
                              Evas_Image_Animated *animated EINA_UNUSED,
                              int *error EINA_UNUSED)
{
   return f;
}

/**
 * @brief Closes a TGA file after loading.
 *
 * This function is called by Evas when it is finished with the image loader.
 * For this TGA loader, no specific action is required to close the file, as
 * Eina_File handles are managed by the caller.
 *
 * @param loader_data The loader data, which is the file handle. Unused here.
 */
static void
evas_image_load_file_close_tga(void *loader_data EINA_UNUSED)
{
}

/**
 * @brief Reads the header of a TGA file to determine image properties.
 *
 * This function reads the TGA header to extract image metadata like width,
 * height, and whether it has an alpha channel. It performs various checks to
 * validate the TGA format. It does not load the pixel data.
 *
 * @param loader_data The loader data, which is the Eina_File handle.
 * @param prop A pointer to an Emile_Image_Property struct to be filled with
 *        image properties.
 * @param error A pointer to an integer where a load error code can be stored.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
evas_image_load_file_head_tga(void *loader_data,
                              Emile_Image_Property *prop,
                              int *error)
{
   Eina_File *f = loader_data;
   unsigned char *seg = NULL, *filedata;
   tga_header *header;
   tga_footer *footer, tfooter;
   char hasa = 0;
   int w, h, bpp;
//   int x, y;
   Eina_Bool r = EINA_FALSE;

   *error = EVAS_LOAD_ERROR_UNKNOWN_FORMAT;
   if (eina_file_size_get(f) < (off_t)(sizeof(tga_header) + sizeof(tga_footer)))
      goto close_file;
   seg = eina_file_map_all(f, EINA_FILE_RANDOM);
   if (seg == NULL) goto close_file;
   filedata = seg;

   header = (tga_header *)filedata;
   // no unaligned data accessed, so ok
   footer = (tga_footer *)(filedata + (eina_file_size_get(f) - sizeof(tga_footer)));
   memcpy((unsigned char *)(&tfooter),
          (unsigned char *)footer,
          sizeof(tga_footer));
   //printf("0\n");
   if (!memcmp(tfooter.signature, TGA_SIGNATURE, sizeof(tfooter.signature)))
     {
        if ((tfooter.dot == '.') && (tfooter.null == 0))
          {
             // footer is there and matches. this is a tga file - any problems now
             // are a corrupt file
             *error = EVAS_LOAD_ERROR_CORRUPT_FILE;
          }
     }
//   else goto close_file;

   filedata = (unsigned char *)filedata + sizeof(tga_header);
   switch (header->imageType)
     {
     case TGA_TYPE_COLOR_RLE:
     case TGA_TYPE_GRAY_RLE:
//        rle = 1;
        break;
     case TGA_TYPE_COLOR:
     case TGA_TYPE_GRAY:
//        rle = 0;
        break;
     default:
        goto close_file;
     }
   bpp = header->bpp;
   if (!((bpp == 32) || (bpp == 24) || (bpp == 16) || (bpp == 8)))
     goto close_file;
   if ((bpp == 32) && (header->descriptor & TGA_DESC_ABITS)) hasa = 1;
   if ((bpp == 16) && (header->descriptor & TGA_DESC_ABITS)) hasa = 1;
   // don't handle colormapped images
   if ((header->colorMapType) != 0)
     goto close_file;
   // if colormap size is anything other than legal sizes or 0 - not real tga
   if (!((header->colorMapSize == 0) ||
         (header->colorMapSize == 15) ||
         (header->colorMapSize == 16) ||
         (header->colorMapSize == 24) ||
         (header->colorMapSize == 32)))
     goto close_file;
//   x = (header->xOriginHi << 8) | (header->xOriginLo);
//   y = (header->yOriginHi << 8) | (header->yOriginLo);
   w = (header->widthHi << 8) | header->widthLo;
   h = (header->heightHi << 8) | header->heightLo;
   // x origin gerater that width, y origin greater than height - wrong file
//   if ((x >= w) || (y >= h))
//     goto close_file;
   // if descriptor has either of the top 2 bits set... not tga
   if (header->descriptor & 0xc0)
     goto close_file;

   if ((w < 1) || (h < 1) || (w > IMG_MAX_SIZE) || (h > IMG_MAX_SIZE) ||
       IMG_TOO_BIG(w, h))
     goto close_file;

   prop->w = w;
   prop->h = h;
   if (hasa) prop->alpha = 1;

   *error = EVAS_LOAD_ERROR_NONE;
   r = EINA_TRUE;

close_file:
   if (seg != NULL) eina_file_map_free(f, seg);
   return r;
}

/**
 * @brief Loads the actual image data from a TGA file.
 *
 * This function reads the pixel data from the TGA file into a provided
 * buffer. It handles various TGA formats, including uncompressed and
 * RLE-compressed data, and different pixel depths (8, 16, 24, 32 bpp).
 * It also handles vertically flipped images.
 *
 * @param loader_data The loader data, which is the Eina_File handle.
 * @param prop The image properties determined by the header loading function.
 * @param pixels A pointer to the memory buffer where the decoded pixel data
 *        (in ARGB8888 format) should be stored.
 * @param error A pointer to an integer where a load error code can be stored.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
evas_image_load_file_data_tga(void *loader_data,
                              Emile_Image_Property *prop,
                              void *pixels,
                              int *error)
{
   Eina_File *f = loader_data;
   unsigned char *seg = NULL, *filedata;
   tga_header *header;
   tga_footer *footer, tfooter;
   char hasa = 0, footer_present = 0, vinverted = 0, rle = 0;
   int w = 0, h = 0, x, y, bpp;
   off_t size;
   unsigned int *surface, *dataptr;
   unsigned int  datasize;
   unsigned char *bufptr, *bufend;
   int abits;
   Eina_Bool res = EINA_FALSE;

   *error = EVAS_LOAD_ERROR_UNKNOWN_FORMAT;
   if (eina_file_size_get(f) < (off_t)(sizeof(tga_header) + sizeof(tga_footer)))
      goto close_file;
   seg = eina_file_map_all(f, EINA_FILE_SEQUENTIAL);
   if (seg == NULL) goto close_file;
   filedata = seg;
   size = eina_file_size_get(f);

   header = (tga_header *)filedata;
   // no unaligned data accessed, so ok
   footer = (tga_footer *)(filedata + (size - sizeof(tga_footer)));
   memcpy((unsigned char *)&tfooter,
          (unsigned char *)footer,
          sizeof(tga_footer));
   if (!memcmp(tfooter.signature, TGA_SIGNATURE, sizeof(tfooter.signature)))
     {
        if ((tfooter.dot == '.') && (tfooter.null == 0))
          {
             // footer is there and matches. this is a tga file - any problems now
             // are a corrupt file
             *error = EVAS_LOAD_ERROR_CORRUPT_FILE;
             footer_present = 1;
          }
     }

   filedata = (unsigned char *)filedata + sizeof(tga_header);
   vinverted = !(header->descriptor & TGA_DESC_VERTICAL);
   switch (header->imageType)
     {
     case TGA_TYPE_COLOR_RLE:
     case TGA_TYPE_GRAY_RLE:
        rle = 1;
        break;
     case TGA_TYPE_COLOR:
     case TGA_TYPE_GRAY:
        rle = 0;
        break;
     default:
        goto close_file;
     }
   bpp = header->bpp;
   if (!((bpp == 32) || (bpp == 24) || (bpp == 16) || (bpp == 8)))
     goto close_file;
   if ((bpp == 32) && (header->descriptor & TGA_DESC_ABITS)) hasa = 1;
   if ((bpp == 16) && (header->descriptor & TGA_DESC_ABITS)) hasa = 1;
   abits = header->descriptor & TGA_DESC_ABITS;
   // don't handle colormapped images
   if ((header->colorMapType) != 0)
     goto close_file;
   // if colormap size is anything other than legal sizes or 0 - not real tga
   if (!((header->colorMapSize == 0) ||
         (header->colorMapSize == 15) ||
         (header->colorMapSize == 16) ||
         (header->colorMapSize == 24) ||
         (header->colorMapSize == 32)))
     goto close_file;
//   x = (header->xOriginHi << 8) | (header->xOriginLo);
//   y = (header->yOriginHi << 8) | (header->yOriginLo);
   w = (header->widthHi << 8) | header->widthLo;
   h = (header->heightHi << 8) | header->heightLo;
   // x origin gerater that width, y origin greater than height - wrong file
//   if ((x >= w) || (y >= h))
//     goto close_file;
   // if descriptor has either of the top 2 bits set... not tga
   if (header->descriptor & 0xc0)
     goto close_file;

   if ((w < 1) || (h < 1) || (w > IMG_MAX_SIZE) || (h > IMG_MAX_SIZE) ||
       IMG_TOO_BIG(w, h))
     goto close_file;

   if ((w != (int)prop->w) || (h != (int)prop->h))
     {
        *error = EVAS_LOAD_ERROR_GENERIC;
        goto close_file;
     }
   surface = pixels;

   datasize = size - sizeof(tga_header) - header->idLength;
   if (footer_present)
     datasize -= sizeof(tga_footer);

   bufptr = filedata + header->idLength;
   bufend = filedata + datasize;

   if (!rle)
     {
        for (y = 0; y < h; y++)
          {
             if (vinverted)
               /* some TGA's are stored upside-down! */
               dataptr = surface + ((h - y - 1) * w);
             else
               dataptr = surface + (y * w);
             switch (bpp)
               {
               case 32:
                  for (x = 0; (x < w) && ((bufptr + 4) <= bufend); x++)
                    {
                       if (hasa)
                         {
                            int a = bufptr[3];

                            switch (abits)
                              {
                               case 1:
                                 a = (a << 7) | (a << 6) | (a << 5) | (a << 4) | (a << 3) | (a << 2) | (a << 1) | (a);
                                 EINA_FALLTHROUGH;
                               case 2:
                                 a = (a << 6) | (a << 4) | (a << 2) | (a);
                                 EINA_FALLTHROUGH;
                               case 3:
                                 a = (a << 5) | (a << 2) | (a >> 1);
                                 EINA_FALLTHROUGH;
                               case 4:
                                 a = (a << 4) | (a);
                                 EINA_FALLTHROUGH;
                               case 5:
                                 a = (a << 3) | (a >> 2);
                                 EINA_FALLTHROUGH;
                               case 6:
                                 a = (a << 2) | (a >> 4);
                                 EINA_FALLTHROUGH;
                               case 7:
                                 a = (a << 1) | (a >> 6);
                                 EINA_FALLTHROUGH;
                               default:
                                 break;
                              }
                            *dataptr = ARGB_JOIN(a, bufptr[2], bufptr[1], bufptr[0]);
                         }
                       else
                         *dataptr = ARGB_JOIN(0xff, bufptr[2], bufptr[1], bufptr[0]);
                       dataptr++;
                       bufptr += 4;
                    }
                  break;
               case 24:
                  for (x = 0; (x < w) && ((bufptr + 3) <= bufend); x++)
                    {
                       *dataptr = ARGB_JOIN(0xff, bufptr[2], bufptr[1], bufptr[0]);
                       dataptr++;
                       bufptr += 3;
                    }
                  break;
               case 16:
                  for (x = 0; (x < w) && ((bufptr + 3) <= bufend); x++)
                    {
                       unsigned char r, g, b, a;
                       unsigned short tmp;

                       tmp =
                         (((unsigned short)bufptr[1]) << 8) |
                         (((unsigned short)bufptr[0]));
                       r = (tmp >> 7) & 0xf8; r |= r >> 5;
                       g = (tmp >> 2) & 0xf8; g |= g >> 5;
                       b = (tmp << 3) & 0xf8; b |= b >> 5;
                       a = 0xff;
                       if ((hasa) && (tmp & 0x8000)) a = 0;
                       *dataptr = ARGB_JOIN(a, r, g, b);
                       dataptr++;
                       bufptr += 2;
                    }
                  break;
               case 8:
                  for (x = 0; (x < w) && ((bufptr + 1) <= bufend); x++)
                    {
                       *dataptr = ARGB_JOIN(0xff, bufptr[0], bufptr[0], bufptr[0]);
                       dataptr++;
                       bufptr += 1;
                    }
                  break;
               default:
                  break;
               }
          }
     }
   else
     {
        int count, i;
        unsigned char val;
        unsigned int *dataend;

        dataptr = surface;
        dataend = dataptr + (w * h);
        while ((bufptr < bufend) && (dataptr < dataend))
          {
             val = *bufptr;
             bufptr++;
             count = (val & 0x7f) + 1;
             if (val & 0x80) // rel packet
               {
                  switch (bpp)
                    {
                    case 32:
                       if (bufptr < (bufend - 4))
                         {
                            unsigned char r, g, b;
                            int a = bufptr[3];

                            switch (abits)
                              {
                               case 1:
                                 a = (a << 7) | (a << 6) | (a << 5) | (a << 4) | (a << 3) | (a << 2) | (a << 1) | (a);
                                 EINA_FALLTHROUGH;
                               case 2:
                                 a = (a << 6) | (a << 4) | (a << 2) | (a);
                                 EINA_FALLTHROUGH;
                               case 3:
                                 a = (a << 5) | (a << 2) | (a >> 1);
                                 EINA_FALLTHROUGH;
                               case 4:
                                 a = (a << 4) | (a);
                                 EINA_FALLTHROUGH;
                               case 5:
                                 a = (a << 3) | (a >> 2);
                                 EINA_FALLTHROUGH;
                               case 6:
                                 a = (a << 2) | (a >> 4);
                                 EINA_FALLTHROUGH;
                               case 7:
                                 a = (a << 1) | (a >> 6);
                                 EINA_FALLTHROUGH;
                               default:
                                 break;
                              }
                            r = bufptr[2];
                            g = bufptr[1];
                            b = bufptr[0];
                            if (!hasa) a = 0xff;
                            bufptr += 4;
                            for (i = 0; (i < count) && (dataptr < dataend); i++)
                              {
                                 *dataptr = ARGB_JOIN(a, r, g, b);
                                 dataptr++;
                              }
                         }
                       break;
                    case 24:
                       if (bufptr < (bufend - 3))
                         {
                            unsigned char r, g, b;

                            r = bufptr[2];
                            g = bufptr[1];
                            b = bufptr[0];
                            bufptr += 3;
                            for (i = 0; (i < count) && (dataptr < dataend); i++)
                              {
                                 *dataptr = ARGB_JOIN(0xff, r, g, b);
                                 dataptr++;
                              }
                         }
                       break;
                    case 16:
                       if (bufptr < (bufend - 2))
                         {
                            unsigned char r, g, b, a;
                            unsigned short tmp;

                            tmp =
                              (((unsigned short)bufptr[1]) << 8) |
                              (((unsigned short)bufptr[0]));
                            r = (tmp >> 7) & 0xf8; r |= r >> 5;
                            g = (tmp >> 2) & 0xf8; g |= g >> 5;
                            b = (tmp << 3) & 0xf8; b |= b >> 5;
                            a = 0xff;
                            if ((hasa) && (tmp & 0x8000)) a = 0;
                            bufptr += 2;
                            for (i = 0; (i < count) && (dataptr < dataend); i++)
                              {
                                 *dataptr = ARGB_JOIN(a, r, g, b);
                                 dataptr++;
                              }
                         }
                       break;
                    case 8:
                       if (bufptr < (bufend - 1))
                         {
                            unsigned char g;

                            g = bufptr[0];
                            bufptr += 1;
                            for (i = 0; (i < count) && (dataptr < dataend); i++)
                              {
                                 *dataptr = ARGB_JOIN(0xff, g, g, g);
                                 dataptr++;
                              }
                         }
                       break;
                    default:
                       break;
                    }
               }
             else // raw
               {
                  switch (bpp)
                    {
                    case 32:
                       for (i = 0; (i < count) && (bufptr < (bufend - 4)) && (dataptr < dataend); i++)
                         {
                            if (hasa)
//                              *dataptr = ARGB_JOIN(255 - bufptr[3], bufptr[2], bufptr[1], bufptr[0]);
                              *dataptr = ARGB_JOIN(bufptr[3], bufptr[2], bufptr[1], bufptr[0]);
                            else
                              *dataptr = ARGB_JOIN(0xff, bufptr[2], bufptr[1], bufptr[0]);
                            dataptr++;
                            bufptr += 4;
                         }
                       break;
                    case 24:
                       for (i = 0; (i < count) && (bufptr < (bufend - 3)) && (dataptr < dataend); i++)
                         {
                            *dataptr = ARGB_JOIN(0xff, bufptr[2], bufptr[1], bufptr[0]);
                            dataptr++;
                            bufptr += 3;
                         }
                       break;
                    case 16:
                       for (i = 0; (i < count) && (bufptr < (bufend - 2)) && (dataptr < dataend); i++)
                         {
                            unsigned char r, g, b, a;
                            unsigned short tmp;

                            tmp =
                              (((unsigned short)bufptr[1]) << 8) |
                              (((unsigned short)bufptr[0]));
                            r = (tmp >> 7) & 0xf8; r |= r >> 5;
                            g = (tmp >> 2) & 0xf8; g |= g >> 5;
                            b = (tmp << 3) & 0xf8; b |= b >> 5;
                            a = 0xff;
                            if ((hasa) && (tmp & 0x8000)) a = 0;
                            *dataptr = ARGB_JOIN(a, r, g, b);
                            dataptr++;
                            bufptr += 2;
                         }
                       break;
                    case 8:
                       for (i = 0; (i < count) && (bufptr < (bufend - 1)) && (dataptr < dataend); i++)
                         {
                            *dataptr = ARGB_JOIN(0xff, bufptr[0], bufptr[0], bufptr[0]);
                            dataptr++;
                            bufptr += 1;
                         }
                       break;
                    default:
                       break;
                    }
               }
          }
        if (vinverted)
          {
             unsigned int *adv, *adv2, tmp;

             adv = surface;
             adv2 = surface + (w * (h - 1));
             for (y = 0; y < (h / 2); y++)
               {
                  for (x = 0; x < w; x++)
                    {
                       tmp = adv[x];
                       adv[x] = adv2[x];
                       adv2[x] = tmp;
                    }
                  adv2 -= w;
                  adv += w;
               }
          }
     }

   prop->premul = EINA_TRUE;

   *error = EVAS_LOAD_ERROR_NONE;
   res = EINA_TRUE;

 close_file:
   if (seg != NULL) eina_file_map_free(f, seg);
   return res;
}

static Evas_Image_Load_Func evas_image_load_tga_func =
{
  EVAS_IMAGE_LOAD_VERSION,
  evas_image_load_file_open_tga,
  evas_image_load_file_close_tga,
  (void*) evas_image_load_file_head_tga,
  NULL,
  (void*) evas_image_load_file_data_tga,
  NULL,
  EINA_TRUE,
  EINA_FALSE
};

/**
 * @brief Evas module initialization function.
 *
 * This function is called by Evas when the TGA loader module is loaded.
 * It registers the loader's function table with the Evas module system.
 *
 * @param em The Evas_Module handle.
 * @return 1 on success, 0 on failure.
 */
static int
module_open(Evas_Module *em)
{
   if (!em) return 0;
   em->functions = (void *)(&evas_image_load_tga_func);
   return 1;
}

/**
 * @brief Evas module shutdown function.
 *
 * This function is called by Evas when the TGA loader module is unloaded.
 *
 * @param em The Evas_Module handle (unused).
 */
static void
module_close(Evas_Module *em EINA_UNUSED)
{
}

static Evas_Module_Api evas_modapi =
{
   EVAS_MODULE_API_VERSION,
   "tga",
   "none",
   {
     module_open,
     module_close
   }
};

EVAS_MODULE_DEFINE(EVAS_MODULE_TYPE_IMAGE_LOADER, image_loader, tga);

#ifndef EVAS_STATIC_BUILD_TGA
EVAS_EINA_MODULE_DEFINE(image_loader, tga);
#endif
