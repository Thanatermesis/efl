#include "evas_common_private.h"
#include "evas_private.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <gif_lib.h>

typedef struct _Frame_Info Frame_Info;
typedef struct _Loader_Info Loader_Info;
typedef struct _File_Info File_Info;

/**
 * @brief Holds information about the memory-mapped GIF file being read.
 */
struct _File_Info
{
   unsigned char *map; /**< Pointer to the memory-mapped file data. */
   int pos, len; /**< Current read position and total length of the mapped data. */ // yes - gif uses ints for file sizes.
};

/**
 * @brief Holds context information for the GIF loader instance.
 */
struct _Loader_Info
{
   Eina_File *f; /**< Eina file handle. */
   Evas_Image_Load_Opts *opts; /**< Image loading options. */
   Evas_Image_Animated *animated; /**< Structure holding animation details. */
   GifFileType *gif; /**< libgif file handle. */
   int imgnum; /**< Current image number being processed (for animation). */
   File_Info fi; /**< Information about the memory-mapped file. */
};

/**
 * @brief Holds information specific to a single frame in a GIF animation.
 */
struct _Frame_Info
{
   int x, y, w, h; /**< Frame offset (x, y) and dimensions (w, h). */
   unsigned short delay; /**< Delay time in 1/100ths of a second before displaying the next frame. */
   short transparent : 10; /**< Transparent color index (-1 == not transparent, otherwise the index). */
   short dispose : 6; /**< Disposal method (0-3) indicating how to treat the frame area before rendering the next frame. */
   Eina_Bool interlace : 1; /**< Flag indicating if the frame is interlaced. */
};

#define LOADERR(x) \
do { \
   *error = (x); \
   goto on_error; \
} while (0)
#define PIX(_x, _y) rows[yin + _y][xin + _x]
#define CMAP(_v) colors[_v]
#define PIXLK(_p) ARGB_JOIN(0xff, CMAP(_p).Red, CMAP(_p).Green, CMAP(_p).Blue)

// utility funcs...

// brute force find frame index - gifs are normally saml so ok for now
/**
 * @brief Finds a frame entry in the animated structure by its index.
 * @param animated The animated image structure containing the frame list.
 * @param index The index of the frame to find.
 * @return The found Image_Entry_Frame or NULL if not found.
 * @note This performs a linear search.
 */
static Image_Entry_Frame *
_find_frame(Evas_Image_Animated *animated, int index)
{
   Eina_List *l;
   Image_Entry_Frame *frame;

   if (!animated->frames) return NULL;
   EINA_LIST_FOREACH(animated->frames, l, frame)
     {
        if (frame->index == index) return frame;
     }
   return NULL;
}

// fill in am image with a specific rgba color value
/**
 * @brief Fills a rectangular area of an image buffer with a solid color.
 * @param data Pointer to the image pixel data (DATA32 format).
 * @param rowpix Width of the image in pixels (stride).
 * @param val The 32-bit ARGB color value to fill with.
 * @param x The starting X coordinate of the rectangle.
 * @param y The starting Y coordinate of the rectangle.
 * @param w The width of the rectangle.
 * @param h The height of the rectangle.
 */
static void
_fill_image(DATA32 *data, int rowpix, DATA32 val, int x, int y, int w, int h)
{
   int xx, yy;
   DATA32 *p;

   for (yy = 0; yy < h; yy++)
     {
        p = data + ((y + yy) * rowpix) + x;
        for (xx = 0; xx < w; xx++)
          {
             *p = val;
             p++;
          }
     }
}

// fix coords and work out an x and y inset in orig data if out of image bounds
/**
 * @brief Clips coordinates to fit within image bounds and calculates insets.
 * @param imw The width of the main image buffer.
 * @param imh The height of the main image buffer.
 * @param[out] xin Pointer to store the horizontal inset within the source data (if clipped left).
 * @param[out] yin Pointer to store the vertical inset within the source data (if clipped top).
 * @param x0 The original starting X coordinate.
 * @param y0 The original starting Y coordinate.
 * @param w0 The original width.
 * @param h0 The original height.
 * @param[out] x Pointer to store the clipped starting X coordinate.
 * @param[out] y Pointer to store the clipped starting Y coordinate.
 * @param[out] w Pointer to store the clipped width.
 * @param[out] h Pointer to store the clipped height.
 */
static void
_clip_coords(int imw, int imh, int *xin, int *yin,
             int x0, int y0, int w0, int h0,
             int *x, int *y, int *w, int *h)
{
   if (x0 < 0)
     {
        w0 += x0;
        *xin = -x0;
        x0 = 0;
     }
   if ((x0 + w0) > imw) w0 = imw - x0;
   if (y0 < 0)
     {
        h0 += y0;
        *yin = -y0;
        y0 = 0;
     }
   if ((y0 + h0) > imh) h0 = imh - y0;
   *x = x0;
   *y = y0;
   *w = w0;
   *h = h0;
}

// file a rgba data pixle blob with a frame color (bg or trans) depending...
/**
 * @brief Fills a frame area with either the background color or transparency.
 * @param data Pointer to the image pixel data (DATA32 format).
 * @param rowpix Width of the image in pixels (stride).
 * @param gif The libgif file handle.
 * @param finfo Frame information containing transparency details.
 * @param x The starting X coordinate of the area to fill.
 * @param y The starting Y coordinate of the area to fill.
 * @param w The width of the area to fill.
 * @param h The height of the area to fill.
 * @details If the frame is not transparent (finfo->transparent < 0), it fills
 *          with the background color defined in the GIF. Otherwise, it fills
 *          with fully transparent pixels (0).
 */
static void
_fill_frame(DATA32 *data, int rowpix, GifFileType *gif, Frame_Info *finfo,
            int x, int y, int w, int h)
{
   // solid color fill for pre frame region
   if (finfo->transparent < 0)
     {
        ColorMapObject *cmap;
        int bg;
        GifColorType colors[256];
        int cnum;

        // work out color to use from cmap
        if (gif->Image.ColorMap) cmap = gif->Image.ColorMap;
        else cmap = gif->SColorMap;
        bg = gif->SBackGroundColor;

        if (cmap)
          {
             // fill in local color table of guaranteed 256 with cmap & pad
             for (cnum = 0; cnum < cmap->ColorCount; cnum++)
               colors[cnum] = cmap->Colors[cnum];
             for (cnum = cmap->ColorCount; cnum < 256; cnum++)
               colors[cnum] = cmap->Colors[0];
          }
        else
          memset(colors, 0, sizeof(colors));
        // and do the fill
        _fill_image
          (data, rowpix,
           ARGB_JOIN(0xff, CMAP(bg).Red, CMAP(bg).Green, CMAP(bg).Blue),
           x, y, w, h);
     }
   // fill in region with 0 (transparent)
   else
     _fill_image(data, rowpix, 0, x, y, w, h);
}

// store common fields from gif file info into frame info
/**
 * @brief Copies common image descriptor fields from the libgif structure to our frame info structure.
 * @param gif The libgif file handle, positioned at the image descriptor.
 * @param finfo The destination Frame_Info structure to populate.
 */
static void
_store_frame_info(GifFileType *gif, Frame_Info *finfo)
{
   finfo->x = gif->Image.Left;
   finfo->y = gif->Image.Top;
   finfo->w = gif->Image.Width;
   finfo->h = gif->Image.Height;
   finfo->interlace = gif->Image.Interlace;
}

// check if image fills "screen space" and if so, if it is transparent
// at all then the image could be transparent - OR if image doesnt fill,
// then it could be trasnparent (full coverage of screen). some gifs will
// be recognized as solid here for faster rendering, but not all.
/**
 * @brief Checks if a frame fully covers the screen and if it uses transparency.
 * @param[in,out] full Pointer to a boolean flag, set to EINA_FALSE if the frame
 *                     does not fully cover the screen or uses transparency.
 * @param finfo Frame information containing position, dimensions, and transparency.
 * @param w The width of the logical screen (GIF canvas).
 * @param h The height of the logical screen (GIF canvas).
 * @details This helps determine if the final image might have alpha.
 */
static void
_check_transparency(Eina_Bool *full, Frame_Info *finfo, int w, int h)
{
   if ((finfo->x == 0) && (finfo->y == 0) &&
       (finfo->w == w) && (finfo->h == h))
     {
        if (finfo->transparent >= 0) *full = EINA_FALSE;
     }
   else *full = EINA_FALSE;
}

// allocate frame and frame info and append to list and store fields
/**
 * @brief Allocates and initializes a new frame entry and its associated info.
 * @param animated The animated image structure to add the frame to.
 * @param transparent The transparency index for this frame (-1 if none).
 * @param dispose The disposal method for this frame.
 * @param delay The delay for this frame (in 1/100ths of a second).
 * @param index The index number for this frame.
 * @return Pointer to the newly allocated Frame_Info structure, or NULL on failure.
 * @details Allocates both Image_Entry_Frame and Frame_Info, links them,
 *          populates the fields, and appends the frame to the animated list.
 */
static Frame_Info *
_new_frame(Evas_Image_Animated *animated,
           int transparent, int dispose, int delay,
           int index)
{
   Image_Entry_Frame *frame;
   Frame_Info *finfo;

   // allocate frame and frame info data (MUSt be separate)
   frame = calloc(1, sizeof(Image_Entry_Frame));
   if (!frame) return NULL;
   finfo = calloc(1, sizeof(Frame_Info));
   if (!finfo)
     {
        free(frame);
        return NULL;
     }
   // record transparent index to be used or -1 if none
   // for this SPECIFIC frame
   finfo->transparent = transparent;
   // record dispose mode (3 bits)
   finfo->dispose = dispose;
   // record delay (2 bytes so max 65546 /100 sec)
   finfo->delay = delay;
   // record the index number we are at
   frame->index = index;
   // that frame is stored AT image/screen size
   frame->info = finfo;
   animated->frames = eina_list_append(animated->frames, frame);
   return finfo;
}

// decode a gif image into rows then expand to 32bit into the destination
// data pointer
/**
 * @brief Decodes a single GIF image frame into a 32-bit RGBA buffer.
 * @param gif The libgif file handle, positioned at the image data.
 * @param data The destination 32-bit RGBA buffer.
 * @param rowpix The width (stride) of the destination buffer in pixels.
 * @param xin Horizontal inset within the source GIF frame data (due to clipping).
 * @param yin Vertical inset within the source GIF frame data (due to clipping).
 * @param transparent The transparency index for this frame (-1 if none).
 * @param fw The full width of the source GIF frame.
 * @param fh The full height of the source GIF frame.
 * @param x The target X coordinate in the destination buffer.
 * @param y The target Y coordinate in the destination buffer.
 * @param w The width of the area to decode/copy.
 * @param h The height of the area to decode/copy.
 * @param fill If EINA_TRUE, transparent pixels in the source overwrite destination
 *             pixels with 0 (fully transparent). If EINA_FALSE, destination pixels
 *             corresponding to transparent source pixels are left unchanged.
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., decoding error).
 * @details Handles interlaced and non-interlaced images, color map expansion,
 *          and transparency application based on the 'fill' flag.
 */
static Eina_Bool
_decode_image(GifFileType *gif, DATA32 *data, int rowpix, int xin, int yin,
              int transparent, int fw, int fh,
              int x, int y, int w, int h, Eina_Bool fill)
{
   int intoffset[] = { 0, 4, 2, 1 };
   int intjump[] = { 8, 8, 4, 2 };
   int i, xx, yy, pix;
   GifRowType *rows;
   GifPixelType *pixels;
   Eina_Bool ret = EINA_FALSE;
   ColorMapObject *cmap;
   DATA32 *p;
   GifColorType colors[256];
   int cnum;

   // build a blob of memory to have pointers to rows of pixels
   // AND store the decoded gif pixels (1 byte per pixel) as welll
   rows = malloc((fh * sizeof(GifRowType)) + (fw * fh * sizeof(GifPixelType)));
   if (!rows) goto on_error;

   // fill in the pointers at the start
   for (yy = 0; yy < fh; yy++)
     {
        rows[yy] = ((unsigned char *)rows) + (fh * sizeof(GifRowType)) +
          (yy * fw * sizeof(GifPixelType));
     }

   // if give is interlaced, walk interlace pattern and decode into rows
   if (gif->Image.Interlace)
     {
        for (i = 0; i < 4; i++)
          {
             for (yy = intoffset[i]; yy < fh; yy += intjump[i])
               {
                  if (DGifGetLine(gif, rows[yy], fw) != GIF_OK)
                    goto on_error;
               }
          }
     }
   // normal top to bottom - decode into rows
   else
     {
        for (yy = 0; yy < fh; yy++)
          {
             // current frame image should be shown
             // even if the current line is not complete.
             DGifGetLine(gif, rows[yy], fw);
          }
     }

   // work out what colormap to use
   if (gif->Image.ColorMap) cmap = gif->Image.ColorMap;
   else cmap = gif->SColorMap;

   if (cmap)
     {
        // fill in local color table of guaranteed 256 entries with cmap & pad
        for (cnum = 0; cnum < cmap->ColorCount; cnum++)
          colors[cnum] = cmap->Colors[cnum];
        for (cnum = cmap->ColorCount; cnum < 256; cnum++)
          colors[cnum] = cmap->Colors[0];
     }
   else
     memset(colors, 0, sizeof(colors));
   // if we need to deal with transparent pixels at all...
   if (transparent >= 0)
     {
        // if we are told to FILL (overwrite with transparency kept)
        if (fill)
          {
             for (yy = 0; yy < h; yy++)
               {
                  pixels = &(PIX(0, yy));
                  p = data + ((y + yy) * rowpix) + x;
                  for (xx = 0; xx < w; xx++)
                    {
                       pix = *pixels;
                       pixels++;
                       if (pix != transparent) *p = PIXLK(pix);
                       else *p = 0;
                       p++;
                    }
               }
          }
        // paste on top with transparent pixels untouched
        else
          {
             for (yy = 0; yy < h; yy++)
               {
                  pixels = &(PIX(0, yy));
                  p = data + ((y + yy) * rowpix) + x;
                  for (xx = 0; xx < w; xx++)
                    {
                       pix = *pixels;
                       pixels++;
                       if (pix != transparent) *p = PIXLK(pix);
                       p++;
                    }
               }
          }
     }
   else
     {
        // walk pixels without worring about transparency at all
        for (yy = 0; yy < h; yy++)
          {
             pixels = &(PIX(0, yy));
             p = data + ((y + yy) * rowpix) + x;
             for (xx = 0; xx < w; xx++)
               {
                  pix = *pixels;
                  pixels++;
                  *p = PIXLK(pix);
                  p++;
               }
          }
     }
   ret = EINA_TRUE;

on_error:
   free(rows);
   return ret;
}

// flush out older rgba frame images to save memory but skip current frame
// and previous frame (prev needed for dispose mode 3)
/**
 * @brief Frees decoded pixel data for older frames to conserve memory.
 * @param animated The animated image structure containing the frames.
 * @param w Width of the image (used for memory calculation).
 * @param h Height of the image (used for memory calculation).
 * @param thisframe The current frame being processed (will not be flushed).
 * @param prevframe The previous frame (needed for disposal mode 3, will not be flushed).
 * @details Iterates through frames older than `prevframe` and frees their
 *          `data` buffer if memory usage exceeds a target threshold (512 KB).
 *          Stops flushing once memory usage is below the target or all eligible
 *          frames have been checked.
 */
static void
_flush_older_frames(Evas_Image_Animated *animated,
                    int w, int h,
                    Image_Entry_Frame *thisframe,
                    Image_Entry_Frame *prevframe)
{
   Eina_List *l;
   Image_Entry_Frame *frame;
   // target is the amount of memory we want to be under for stored frames
   int total = 0, target = 512 * 1024;

   // total up the amount of memory used by stored frames for this image
   EINA_LIST_FOREACH(animated->frames, l, frame)
     {
        if (frame->data) total++;
     }
   total *= (w * h * sizeof(DATA32));
   // if we use less than target (512k) for frames - dont flush
   if (total < target) return;
   // clean oldest frames first and go until below target or until we loop
   // around back to this frame (curent)
   EINA_LIST_FOREACH(animated->frames, l, frame)
     {
        if (frame == thisframe) break;
     }
   if (!l) return;
   // start on next frame after thisframe
   l = l->next;
   // handle wrap to start
   if (!l) l = animated->frames;
   // now walk until we hit thisframe again... then stop walk.
   while (l)
     {
        frame = l->data;
        if (frame == thisframe) break;
        if (frame->data)
          {
             if ((frame != thisframe) && (frame != prevframe))
               {
                  free(frame->data);
                  frame->data = NULL;
                  // subtract memory used and if below target - stop flush
                  total -= (w * h * sizeof(DATA32));
                  if (total < target) break;
               }
          }
        // go to next - handle wrap to start
        l = l->next;
        if (!l) l = animated->frames;
     }
}

/**
 * @brief libgif callback function to read data from a memory buffer.
 * @param gft The GifFileType structure (contains user data).
 * @param buf The buffer to read data into.
 * @param len The maximum number of bytes to read.
 * @return The number of bytes actually read, or 0 on EOF or error.
 */
static int
_file_read(GifFileType *gft, GifByteType *buf, int len)
{
   File_Info *fi = gft->UserData;

   if (fi->pos >= fi->len) return 0; // if at or past end - no
   if ((fi->pos + len) >= fi->len) len = fi->len - fi->pos;
   memcpy(buf, fi->map + fi->pos, len);
   fi->pos += len;
   return len;
}

/**
* @brief Reads the header information of a GIF file.
* @param loader_data Pointer to the Loader_Info context.
* @param[out] prop Pointer to the Emile_Image_Property structure to fill.
* @param[out] error Pointer to store the error code on failure.
* @return EINA_TRUE on success, EINA_FALSE on failure.
* @details Opens the GIF file using libgif, reads the screen descriptor,
*          parses through image descriptors and extensions to determine
*          image dimensions, frame count, loop count, and potential alpha usage.
*          Populates the `prop` and `animated` structures.
*/
static Eina_Bool
evas_image_load_file_head_gif2(void *loader_data,
                               Emile_Image_Property *prop,
                               int *error)
{
   Loader_Info *loader = loader_data;
   Evas_Image_Animated *animated = loader->animated;
   Eina_File *f = loader->f;
   Eina_Bool ret = EINA_FALSE;
   File_Info fi;
   GifRecordType rec;
   GifFileType *gif = NULL;
   // it is possible which gif file have error midle of frames,
   // in that case we should play gif file until meet error frame.
   int imgnum = 0;
   int loop_count = -1;
   Frame_Info *finfo = NULL;
   Eina_Bool full = EINA_TRUE;

   // init prop struct with some default null values
   prop->w = 0;
   prop->h = 0;

   // map the file and store/track info
   fi.map = eina_file_map_all(f, EINA_FILE_RANDOM);
   if (!fi.map) LOADERR(EVAS_LOAD_ERROR_CORRUPT_FILE);
   fi.len = eina_file_size_get(f);
   fi.pos = 0;

   // actually ask libgif to open the file
#if GIFLIB_MAJOR >= 5
   gif = DGifOpen(&fi, _file_read, NULL);
#else
   gif = DGifOpen(&fi, _file_read);
#endif
   if (!gif) LOADERR(EVAS_LOAD_ERROR_UNKNOWN_FORMAT);
   // get the gif "screen size" (the actual image size)
   prop->w = gif->SWidth;
   prop->h = gif->SHeight;
   // if size is invalid - abort here
   if ((prop->w < 1) || (prop->h < 1) ||
       (prop->w > IMG_MAX_SIZE) || (prop->h > IMG_MAX_SIZE) ||
       IMG_TOO_BIG(prop->w, prop->h))
     {
        if (IMG_TOO_BIG(prop->w, prop->h))
          LOADERR(EVAS_LOAD_ERROR_RESOURCE_ALLOCATION_FAILED);
        LOADERR(EVAS_LOAD_ERROR_GENERIC);
     }
   // walk through gif records in file to figure out info
   do
     {
        if (DGifGetRecordType(gif, &rec) == GIF_ERROR)
          {
             // if we have a gif that ends part way through a sequence
             // (or animation) consider it valid and just break - no error
             if (imgnum > 1) break;
             LOADERR(EVAS_LOAD_ERROR_UNKNOWN_FORMAT);
          }
        // get image description section
        if (rec == IMAGE_DESC_RECORD_TYPE)
          {
             int img_code;
             GifByteType *img;

             // get image desc
             if (DGifGetImageDesc(gif) == GIF_ERROR)
               LOADERR(EVAS_LOAD_ERROR_UNKNOWN_FORMAT);
             // skip decoding and just walk image to next
             if (DGifGetCode(gif, &img_code, &img) == GIF_ERROR)
               LOADERR(EVAS_LOAD_ERROR_UNKNOWN_FORMAT);
             // skip till next...
             while (img)
               {
                  img = NULL;
                  DGifGetCodeNext(gif, &img);
               }
             // store geometry in the last frame info data
             if (finfo)
               {
                  _store_frame_info(gif, finfo);
                  _check_transparency(&full, finfo, prop->w, prop->h);
               }
             // or if we dont have a finfo entry - create one even for stills
             else
               {
                  // allocate and save frame with field data
                  finfo = _new_frame(animated, -1, 0, 0, imgnum + 1);
                  if (!finfo)
                    LOADERR(EVAS_LOAD_ERROR_RESOURCE_ALLOCATION_FAILED);
                  // store geometry info from gif image
                  _store_frame_info(gif, finfo);
                  // check for transparency/alpha
                  _check_transparency(&full, finfo, prop->w, prop->h);
               }
             imgnum++;
          }
        // we have an extension code block - for animated gifs for sure
        else if (rec == EXTENSION_RECORD_TYPE)
          {
             int ext_code;
             GifByteType *ext;

             ext = NULL;
             // get the first extension entry
             DGifGetExtension(gif, &ext_code, &ext);
             while (ext)
               {
                  // graphic control extension - for animated gif data
                  // and transparent index + flag
                  if (ext_code == 0xf9)
                    {
                       // create frame and store it in image
                       finfo = _new_frame
                         (animated,
                          (ext[1] & 1) ? ext[4] : -1, // transparency index
                          (ext[1] >> 2) & 0x7, // dispose mode
                          ((int)ext[3] << 8) | (int)ext[2], // delay
                          imgnum + 1);
                       if (!finfo)
                         LOADERR(EVAS_LOAD_ERROR_RESOURCE_ALLOCATION_FAILED);
                    }
                  // netscape extension indicating loop count...
                  else if (ext_code == 0xff) /* application extension */
                    {
                       if (!strncmp((char *)(&ext[1]), "NETSCAPE2.0", 11) ||
                           !strncmp((char *)(&ext[1]), "ANIMEXTS1.0", 11))
                         {
                            ext = NULL;
                            DGifGetExtensionNext(gif, &ext);
                            if (ext[1] == 0x01)
                              {
                                 loop_count = ((int)ext[3] << 8) | (int)ext[2];
                                 if (loop_count > 0) loop_count++;
                              }
                         }
                    }
                  // and continue onto the next extension entry
                  ext = NULL;
                  DGifGetExtensionNext(gif, &ext);
               }
          }
     }
   while (rec != TERMINATE_RECORD_TYPE);

   // if the gif main says we have more than one image or our image counting
   // says so, then this image is animated - indicate this
   if ((gif->ImageCount > 1) || (imgnum > 1))
     {
        animated->animated = 1;
        animated->loop_count = loop_count;
        animated->loop_hint = EVAS_IMAGE_ANIMATED_HINT_LOOP;
        animated->frame_count = MIN(gif->ImageCount, imgnum);
     }
   if (!full) prop->alpha = 1;
   animated->cur_frame = 1;

   // no errors in header scan etc. so set err and return value
   *error = EVAS_LOAD_ERROR_NONE;
   ret = EINA_TRUE;

on_error: // jump here on any errors to clean up
#if (GIFLIB_MAJOR > 5) || ((GIFLIB_MAJOR == 5) && (GIFLIB_MINOR >= 1))
   if (gif) DGifCloseFile(gif, NULL);
#else
   if (gif) DGifCloseFile(gif);
#endif
   if (fi.map) eina_file_map_free(f, fi.map);
   return ret;
}

/**
* @brief Loads the pixel data for a specific frame of a GIF file.
* @param loader_data Pointer to the Loader_Info context.
* @param prop Pointer to the image properties (mostly read-only here).
* @param[out] pixels Pointer to the destination buffer for the pixel data (DATA32 format).
* @param[out] error Pointer to store the error code on failure.
* @return EINA_TRUE on success, EINA_FALSE on failure.
* @details Locates the requested frame data. If the frame is already decoded and
*          cached, it copies the cached data. Otherwise, it opens/re-opens the
*          GIF file, seeks to the correct frame, handles frame disposal methods
*          from the previous frame, decodes the current frame using _decode_image,
*          caches the result (if animated), and copies the final pixel data to
*          the `pixels` buffer. It also manages flushing older cached frames.
*/
static Eina_Bool
evas_image_load_file_data_gif2(void *loader_data,
                               Emile_Image_Property *prop,
                               void *pixels,
                               int *error)
{
   Loader_Info *loader = loader_data;
   Evas_Image_Animated *animated = loader->animated;
   Eina_File *f = loader->f;
   Eina_Bool ret = EINA_FALSE;
   GifRecordType rec;
   GifFileType *gif = NULL;
   Image_Entry_Frame *frame;
   int index = 0, imgnum = 0;
   Frame_Info *finfo;

   // XXX: this is so wrong - storing current frame IN the image
   // so we have to load multiple times to animate. what if the
   // same image is shared/loaded in 2 ore more places AND animated
   // there?

   // use index stored in image (XXX: yuk!)
   index = animated->cur_frame;
   // if index is invalid for animated image - error out
   if ((animated->animated) &&
       ((index <= 0) || (index > animated->frame_count)))
     LOADERR(EVAS_LOAD_ERROR_GENERIC);
   // find the given frame index
   frame = _find_frame(animated, index);
   if (frame)
     {
        if ((frame->loaded) && (frame->data))
          {
             // frame is already there and decoded - jump to end
             goto on_ok;
          }
     }
   else
     LOADERR(EVAS_LOAD_ERROR_CORRUPT_FILE);

open_file:
   // actually ask libgif to open the file
   gif = loader->gif;
   if (!gif)
     {
        // there was no file previously opened
        // map the file and store/track info
        loader->fi.map = eina_file_map_all(f, EINA_FILE_RANDOM);
        if (!loader->fi.map) LOADERR(EVAS_LOAD_ERROR_CORRUPT_FILE);
        loader->fi.len = eina_file_size_get(f);
        loader->fi.pos = 0;

#if GIFLIB_MAJOR >= 5
        gif = DGifOpen(&(loader->fi), _file_read, NULL);
#else
        gif = DGifOpen(&(loader->fi), _file_read);
#endif
        // if gif open failed... get out of here
        if (!gif)
          {
             if ((loader->fi.map) && (loader->f))
               eina_file_map_free(loader->f, loader->fi.map);
             loader->fi.map = NULL;
             LOADERR(EVAS_LOAD_ERROR_UNKNOWN_FORMAT);
          }
        loader->gif = gif;
        loader->imgnum = 1;
     }

   // if we want to go backwards, we likely need/want to re-decode from the
   // start as we have nothnig to build on
   if ((index > 0) && (index < loader->imgnum) && (animated->animated))
     {
#if (GIFLIB_MAJOR > 5) || ((GIFLIB_MAJOR == 5) && (GIFLIB_MINOR >= 1))
        if (loader->gif) DGifCloseFile(loader->gif, NULL);
#else
        if (loader->gif) DGifCloseFile(loader->gif);
#endif
        if ((loader->fi.map) && (loader->f))
          eina_file_map_free(loader->f, loader->fi.map);
        loader->gif = NULL;
        loader->fi.map = NULL;
        loader->imgnum = 0;
        goto open_file;
     }

   // our current position is the previous frame we decoded from the file
   imgnum = loader->imgnum;

   // walk through gif records in file to figure out info
   do
     {
        // if getting the recored value is failed,
        // it will be retried until the termination
        DGifGetRecordType(gif, &rec);
        if (rec == EXTENSION_RECORD_TYPE)
          {
             int                 ext_code;
             GifByteType        *ext;

             ext = NULL;
             DGifGetExtension(gif, &ext_code, &ext);
             while (ext)
               {
                  ext = NULL;
                  DGifGetExtensionNext(gif, &ext);
               }
          }
        // get image description section
        else if (rec == IMAGE_DESC_RECORD_TYPE)
          {
             int xin = 0, yin = 0, x = 0, y = 0, w = 0, h = 0;
             int img_code;
             GifByteType *img;
             Image_Entry_Frame *prevframe = NULL;
             Image_Entry_Frame *thisframe = NULL;

             // get image desc
             if (DGifGetImageDesc(gif) == GIF_ERROR)
               LOADERR(EVAS_LOAD_ERROR_UNKNOWN_FORMAT);
             // get the previous frame entry AND the current one to fill in
             prevframe = _find_frame(animated, imgnum - 1);
             thisframe = _find_frame(animated, imgnum);
             // if we have a frame AND we're animated AND we have no data...
             if ((thisframe) && (!thisframe->data) && (animated->animated))
               {
                  Eina_Bool first = EINA_FALSE;

                  // allocate it
                  thisframe->data =
                    malloc(prop->w * prop->h * sizeof(DATA32));
                  if (!thisframe->data)
                    LOADERR(EVAS_LOAD_ERROR_RESOURCE_ALLOCATION_FAILED);
                  // if we have no prior frame OR prior frame data... empty
                  if ((!prevframe) || (!prevframe->data))
                    {
                       first = EINA_TRUE;
                       finfo = thisframe->info;
                       memset(thisframe->data, 0,
                              prop->w * prop->h * sizeof(DATA32));
                    }
                  // we have a prior frame to copy data from...
                  else
                    {
                       finfo = prevframe->info;

                       // fix coords of sub image in case it goes out...
                       _clip_coords(prop->w, prop->h, &xin, &yin,
                                    finfo->x, finfo->y, finfo->w, finfo->h,
                                    &x, &y, &w, &h);
                       // if dispose mode is not restore - then copy pre frame
                       if (finfo->dispose != 3) // GIF_DISPOSE_RESTORE
                         memcpy(thisframe->data, prevframe->data,
                              prop->w * prop->h * sizeof(DATA32));
                       // if dispose mode is "background" then fill with bg
                       if (finfo->dispose == 2) // GIF_DISPOSE_BACKGND
                         _fill_frame(thisframe->data, prop->w, gif,
                                     finfo, x, y, w, h);
                       else if (finfo->dispose == 3) // GIF_DISPOSE_RESTORE
                         {
                            Image_Entry_Frame *prevframe2;

                            // we need to copy data from one frame back
                            // from the prev frame into the current frame
                            // (copy the whole image - at least the sample
                            // GifWin.cpp from libgif indicates this is what
                            // needs doing
                            prevframe2 = _find_frame(animated, imgnum - 2);
                            if (prevframe2)
                              memcpy(thisframe->data, prevframe2->data,
                                     prop->w * prop->h * sizeof(DATA32));
                         }
                       finfo = thisframe->info;
                    }
                  // now draw this frame on top
                  _clip_coords(prop->w, prop->h, &xin, &yin,
                               finfo->x, finfo->y, finfo->w, finfo->h,
                               &x, &y, &w, &h);
                  if (!_decode_image(gif, thisframe->data, prop->w,
                                     xin, yin, finfo->transparent,
                                     finfo->w, finfo->h,
                                     x, y, w, h, first))
                    LOADERR(EVAS_LOAD_ERROR_CORRUPT_FILE);
                  // mark as loaded and done
                  thisframe->loaded = EINA_TRUE;
                  // and flush old memory if needed (too much)
                  _flush_older_frames(animated, prop->w, prop->h,
                                      thisframe, prevframe);
               }
             // if we hve a frame BUT the image is not animated... different
             // path
             else if ((thisframe) && (!thisframe->data) &&
                      (!animated->animated))
               {
                  // if we don't have the data decoded yet - decode it
                  if ((!thisframe->loaded) || (!thisframe->data))
                    {
                       // use frame info but we WONT allocate frame pixels
                       finfo = thisframe->info;
                       _clip_coords(prop->w, prop->h, &xin, &yin,
                                    finfo->x, finfo->y, finfo->w, finfo->h,
                                    &x, &y, &w, &h);
                       // clear out all pixels
                       _fill_frame(pixels, prop->w, gif,
                                   finfo, 0, 0, prop->w, prop->h);
                       // and decode the gif with overwriting
                       if (!_decode_image(gif, pixels, prop->w,
                                          xin, yin, finfo->transparent,
                                          finfo->w, finfo->h,
                                          x, y, w, h, EINA_TRUE))
                         LOADERR(EVAS_LOAD_ERROR_CORRUPT_FILE);
                       // mark as loaded and done
                       thisframe->loaded = EINA_TRUE;
                    }
                  // flush mem we don't need (at expense of decode cpu)
               }
             else
               {
                  // skip decoding and just walk image to next
                  if (DGifGetCode(gif, &img_code, &img) == GIF_ERROR)
                    LOADERR(EVAS_LOAD_ERROR_UNKNOWN_FORMAT);
                  while (img)
                    {
                       img = NULL;
                       DGifGetCodeNext(gif, &img);
                    }
               }
             imgnum++;
             // if we found the image we wanted - get out of here
             if (imgnum > index) break;
          }
     }
   while (rec != TERMINATE_RECORD_TYPE);

   // if we are at the end of the animation or not animated, close file
   loader->imgnum = imgnum;
   if ((animated->frame_count <= 1) || (rec == TERMINATE_RECORD_TYPE))
     {
#if (GIFLIB_MAJOR > 5) || ((GIFLIB_MAJOR == 5) && (GIFLIB_MINOR >= 1))
        if (loader->gif) DGifCloseFile(loader->gif, NULL);
#else
        if (loader->gif) DGifCloseFile(loader->gif);
#endif
        if ((loader->fi.map) && (loader->f))
          eina_file_map_free(loader->f, loader->fi.map);
        loader->gif = NULL;
        loader->fi.map = NULL;
        loader->imgnum = 0;
     }

on_ok:
   // no errors in header scan etc. so set err and return value
   *error = EVAS_LOAD_ERROR_NONE;
   ret = EINA_TRUE;

   // if it was an animated image we need to copy the data to the
   // pixels for the image from the frame holding the data
   if (animated->animated && frame->data)
     memcpy(pixels, frame->data, prop->w * prop->h * sizeof(DATA32));
   prop->premul = EINA_TRUE;

on_error: // jump here on any errors to clean up
   return ret;
}

// get the time between 2 frames in the timeline
/**
* @brief Calculates the total duration for a sequence of frames.
* @param loader_data Pointer to the Loader_Info context.
* @param start_frame The index of the first frame in the sequence.
* @param frame_num The number of frames in the sequence.
* @return The total duration in seconds, or -1.0 on error (e.g., invalid frame index).
* @details Sums the delays (stored in 1/100ths of a second) for the specified
*          range of frames. Uses a default delay of 10/100s if a frame's delay is 0.
*/
static double
evas_image_load_frame_duration_gif2(void *loader_data,
                                    int start_frame,
                                    int frame_num)
{
   Loader_Info *loader = loader_data;
   Evas_Image_Animated *animated = loader->animated;
   Image_Entry_Frame *frame;
   int i, total = 0;

   // if its not animated or requested frame data is invalid
   if (!animated->animated) return -1.0;
   if ((start_frame + frame_num) > animated->frame_count) return -1.0;
   if (frame_num < 0) return -1.0;

   if (frame_num < 1) frame_num = 1;
   // walk frames from start frame though and total up delays
   for (i = start_frame; i < (start_frame + frame_num); i++)
     {
        Frame_Info *finfo;

        // find the frame
        frame = _find_frame(animated, i);
        // no frame? barf - bad file or i/o?
        if (!frame) return -1.0;
        // get delay and total it up
        finfo = frame->info;
        // if delay is sensible - use it else assume 10/100ths of a sec
        if (finfo->delay > 0) total += finfo->delay;
        else total += 10;
     }
   // return delay in seconds (since timing in gifs is in 1/100ths of a sec)
   return (double)total / 100.0;
}

// called on opening of a file load
/**
* @brief Opens a GIF file for loading.
* @param f The Eina_File handle to the file.
* @param key Optional key (unused here).
* @param opts Load options (unused here).
* @param animated Pointer to the animated structure to be populated/used.
* @param[out] error Pointer to store the error code on failure.
* @return A pointer to the allocated Loader_Info context, or NULL on failure.
* @details Allocates the loader context, duplicates the file handle, and stores
*          pointers to the options and animated structure.
*/
static void *
evas_image_load_file_open_gif2(Eina_File *f,
                               Eina_Stringshare *key EINA_UNUSED, // XXX: we need to use key for frame #
                               Evas_Image_Load_Opts *opts,
                               Evas_Image_Animated *animated,
                               int *error)
{
   Loader_Info *loader = calloc(1, sizeof (Loader_Info));
   if (!loader)
     {
        *error = EVAS_LOAD_ERROR_RESOURCE_ALLOCATION_FAILED;
        return NULL;
     }
   loader->f = eina_file_dup(f);
   loader->opts = opts;
   loader->animated = animated;
   return loader;
}

// called on closing of an image file load (end of load)
/**
* @brief Closes the GIF file and cleans up loader resources.
* @param loader_data Pointer to the Loader_Info context created by evas_image_load_file_open_gif2.
* @details Closes the libgif file handle (if open), unmaps the memory-mapped file
*          (if mapped), closes the Eina_File handle, and frees the loader context.
*/
static void
evas_image_load_file_close_gif2(void *loader_data)
{
   Loader_Info *loader = loader_data;
#if (GIFLIB_MAJOR > 5) || ((GIFLIB_MAJOR == 5) && (GIFLIB_MINOR >= 1))
   if (loader->gif) DGifCloseFile(loader->gif, NULL);
#else
   if (loader->gif) DGifCloseFile(loader->gif);
#endif
   if ((loader->fi.map) && (loader->f))
     eina_file_map_free(loader->f, loader->fi.map);
   if (loader->f) eina_file_close(loader->f);
   free(loader);
}

// general module delcaration stuff
static Evas_Image_Load_Func evas_image_load_gif_func =
{
  EVAS_IMAGE_LOAD_VERSION,
  evas_image_load_file_open_gif2,
  evas_image_load_file_close_gif2,
  (void*) evas_image_load_file_head_gif2,
  NULL,
  (void*) evas_image_load_file_data_gif2,
  evas_image_load_frame_duration_gif2,
  EINA_TRUE,
  EINA_FALSE
};

// raw module api that the rest of the world sees
/**
* @brief Evas module initialization function.
* @param em The Evas_Module structure to initialize.
* @return 1 on success, 0 on failure.
* @details Assigns the loader function table (`evas_image_load_gif_func`)
*          to the module structure.
*/
static int
module_open(Evas_Module *em)
{
   if (!em) return 0;
   em->functions = (void *)(&evas_image_load_gif_func);
   return 1;
}

/**
* @brief Evas module shutdown function.
* @param em The Evas_Module structure (unused).
* @details Currently does nothing.
*/
static void
module_close(Evas_Module *em EINA_UNUSED)
{
}

static Evas_Module_Api evas_modapi =
{
   EVAS_MODULE_API_VERSION,
   "gif",
   "none",
   {
      module_open,
      module_close
   }
};

EVAS_MODULE_DEFINE(EVAS_MODULE_TYPE_IMAGE_LOADER, image_loader, gif);

#ifndef EVAS_STATIC_BUILD_GIF
EVAS_EINA_MODULE_DEFINE(image_loader, gif);
#endif
