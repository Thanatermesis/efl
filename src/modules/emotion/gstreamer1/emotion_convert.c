#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "emotion_gstreamer.h"

/**
 * @file emotion_convert.c
 * @brief This file contains functions for converting GStreamer video frames
 * to Evas-compatible formats.
 */

/**
 * @brief Copies pixel data from GStreamer to Evas, handling BGRx format with a specific step.
 *
 * This function iterates over the image pixels, copying BGR values from the
 * GStreamer data buffer (`gst_data`) to the Evas data buffer (`evas_data`).
 * It sets the alpha channel in the Evas data to 255 (opaque).
 * The `step` parameter determines how many bytes to advance in the `gst_data`
 * buffer for each pixel (e.g., 3 for BGR, 4 for BGRx/BGRA).
 *
 * @param[out] evas_data Pointer to the destination Evas image data buffer (expects ARGB format).
 * @param[in] gst_data Pointer to the source GStreamer image data buffer.
 * @param w Width of the image in pixels.
 * @param h Height of the image in pixels (unused in this specific implementation, output_height is used).
 * @param output_height The number of lines to process.
 * @param step Number of bytes per pixel in the source `gst_data` (e.g., 3 for BGR, 4 for BGRx).
 */
static inline void
_evas_video_bgrx_step(unsigned char *evas_data, const unsigned char *gst_data,
                      unsigned int w, unsigned int h EINA_UNUSED,
                      unsigned int output_height, unsigned int step)
{
   unsigned int x, y;

   for (y = 0; y < output_height; ++y)
     {
        for (x = 0; x < w; x++)
          {
             evas_data[0] = gst_data[0];
             evas_data[1] = gst_data[1];
             evas_data[2] = gst_data[2];
             evas_data[3] = 255;
             gst_data += step;
             evas_data += 4;
          }
     }
}

/**
 * @brief Converts BGR formatted video data from GStreamer to Evas's ARGB format.
 *
 * This function utilizes _evas_video_bgrx_step with a step of 3, as BGR format
 * has 3 bytes per pixel.
 *
 * @param[out] evas_data Pointer to the destination Evas image data buffer (ARGB).
 * @param[in] gst_data Pointer to the source GStreamer image data buffer (BGR).
 * @param w Width of the image in pixels.
 * @param h Height of the image in pixels.
 * @param output_height The number of lines to process.
 * @param info Pointer to Emotion_Convert_Info structure (unused).
 */
static void
_evas_video_bgr(unsigned char *evas_data, const unsigned char *gst_data,
                unsigned int w, unsigned int h, unsigned int output_height,
                Emotion_Convert_Info *info EINA_UNUSED)
{
   // XXX: need to check offset and stride that gst provide and what they
   // mean with a non-planar format like bgra
   _evas_video_bgrx_step(evas_data, gst_data, w, h, output_height, 3);
}

/**
 * @brief Converts BGRx formatted video data from GStreamer to Evas's ARGB format.
 *
 * This function utilizes _evas_video_bgrx_step with a step of 4, as BGRx format
 * has 4 bytes per pixel (the 'x' channel is ignored from source, alpha set to opaque).
 *
 * @param[out] evas_data Pointer to the destination Evas image data buffer (ARGB).
 * @param[in] gst_data Pointer to the source GStreamer image data buffer (BGRx).
 * @param w Width of the image in pixels.
 * @param h Height of the image in pixels.
 * @param output_height The number of lines to process.
 * @param info Pointer to Emotion_Convert_Info structure (unused).
 */
static void
_evas_video_bgrx(unsigned char *evas_data, const unsigned char *gst_data,
                 unsigned int w, unsigned int h, unsigned int output_height,
                 Emotion_Convert_Info *info EINA_UNUSED)
{
   // XXX: need to check offset and stride that gst provide and what they
   // mean with a non-planar format like bgra
   _evas_video_bgrx_step(evas_data, gst_data, w, h, output_height, 4);
}

/**
 * @brief Converts BGRA formatted video data from GStreamer to Evas's ARGB format with premultiplied alpha.
 *
 * This function copies BGRA data, performing alpha premultiplication for the RGB components.
 * Evas expects ARGB with premultiplied alpha.
 * (evas_R = gst_R * gst_A / 255, evas_G = gst_G * gst_A / 255, evas_B = gst_B * gst_A / 255, evas_A = gst_A)
 *
 * @param[out] evas_data Pointer to the destination Evas image data buffer (premultiplied ARGB).
 * @param[in] gst_data Pointer to the source GStreamer image data buffer (BGRA).
 * @param w Width of the image in pixels.
 * @param h Height of the image in pixels (unused).
 * @param output_height The number of lines to process.
 * @param info Pointer to Emotion_Convert_Info structure (unused).
 */
static void
_evas_video_bgra(unsigned char *evas_data, const unsigned char *gst_data,
                 unsigned int w, unsigned int h EINA_UNUSED,
                 unsigned int output_height,
                 Emotion_Convert_Info *info EINA_UNUSED)
{
   unsigned int x, y;

   // XXX: need to check offset and stride that gst provide and what they
   // mean with a non-planar format like bgra
   for (y = 0; y < output_height; ++y)
     {
        unsigned char alpha;

        for (x = 0; x < w; ++x)
          {
             alpha = gst_data[3];
             evas_data[0] = (gst_data[0] * alpha) / 255;
             evas_data[1] = (gst_data[1] * alpha) / 255;
             evas_data[2] = (gst_data[2] * alpha) / 255;
             evas_data[3] = alpha;
             gst_data += 4;
             evas_data += 4;
          }
     }
}

/**
 * @brief Sets up Evas plane pointers for I420 (planar YUV 4:2:0) format.
 *
 * For planar formats like I420, Evas expects an array of pointers to the
 * start of each plane's data (Y, U, V). This function populates `evas_data`
 * (which is cast to `const unsigned char **`) with these pointers, derived
 * from the `Emotion_Convert_Info` structure.
 *
 * `evas_data` will be structured as:
 *   rows[0] = pointer to Y plane row 0
 *   rows[1] = pointer to Y plane row 1
 *   ...
 *   rows[rh-1] = pointer to Y plane row rh-1
 *   rows[rh] = pointer to U plane row 0
 *   rows[rh+1] = pointer to U plane row 1
 *   ...
 *   rows[rh + rh/2 - 1] = pointer to U plane row rh/2 - 1
 *   rows[rh + rh/2] = pointer to V plane row 0
 *   ...
 *   rows[rh + rh/2 + rh/2 - 1] = pointer to V plane row rh/2 - 1
 *
 * @param[out] evas_data Pointer to an array that will be filled with pointers to each row of each plane.
 *                       Effectively `const unsigned char **rows`.
 * @param[in] gst_data Pointer to the source GStreamer image data buffer (unused, as data comes from `info`).
 * @param w Width of the image in pixels (unused).
 * @param h Height of the image in pixels (unused).
 * @param output_height The height of the Y plane (and the effective output height for Evas).
 * @param info Pointer to Emotion_Convert_Info structure containing plane pointers, strides, and bpp.
 */
static void
_evas_video_i420(unsigned char *evas_data,
                 const unsigned char *gst_data EINA_UNUSED,
                 unsigned int w EINA_UNUSED, unsigned int h EINA_UNUSED,
                 unsigned int output_height,
                 Emotion_Convert_Info *info)
{
   const unsigned char **rows, *ptr;
   unsigned int i, j, jump, rh;

   if (info->bpp[0] != 1) ERR("Plane 0 bpp != 1");
   if (info->bpp[1] != 1) ERR("Plane 1 bpp != 1");
   if (info->bpp[2] != 1) ERR("Plane 2 bpp != 1");

   rh = output_height;
   rows = (const unsigned char **)evas_data;

   ptr = info->plane_ptr[0];
   jump = info->stride[0];
   for (i = 0; i < rh; i++, ptr += jump) rows[i] = ptr;

   ptr = info->plane_ptr[1];
   jump = info->stride[1];
   for (j = 0; j < (rh / 2); j++, i++, ptr += jump) rows[i] = ptr;

   ptr = info->plane_ptr[2];
   jump = info->stride[2];
   for (j = 0; j < (rh / 2); j++, i++, ptr += jump) rows[i] = ptr;
}

/**
 * @brief Sets up Evas plane pointers for YV12 (planar YUV 4:2:0, V plane before U) format.
 *
 * Similar to _evas_video_i420, this function populates `evas_data` with pointers
 * to the Y, V, and U planes. The key difference from I420 is the order of
 * the chroma planes (V then U).
 *
 * `evas_data` will be structured as:
 *   rows[0] = pointer to Y plane row 0
 *   ...
 *   rows[rh-1] = pointer to Y plane row rh-1
 *   rows[rh] = pointer to V plane row 0 (Note: plane_ptr[1] is V for YV12)
 *   ...
 *   rows[rh + rh/2 - 1] = pointer to V plane row rh/2 - 1
 *   rows[rh + rh/2] = pointer to U plane row 0 (Note: plane_ptr[2] is U for YV12)
 *   ...
 *   rows[rh + rh/2 + rh/2 - 1] = pointer to U plane row rh/2 - 1
 *
 * @param[out] evas_data Pointer to an array that will be filled with pointers to each row of each plane.
 *                       Effectively `const unsigned char **rows`.
 * @param[in] gst_data Pointer to the source GStreamer image data buffer (unused).
 * @param w Width of the image in pixels (unused).
 * @param h Height of the image in pixels (unused).
 * @param output_height The height of the Y plane.
 * @param info Pointer to Emotion_Convert_Info structure containing plane pointers, strides, and bpp.
 *             For YV12, info->plane_ptr[1] is expected to be the V plane and info->plane_ptr[2] the U plane.
 */
static void
_evas_video_yv12(unsigned char *evas_data,
                 const unsigned char *gst_data EINA_UNUSED,
                 unsigned int w EINA_UNUSED, unsigned int h EINA_UNUSED,
                 unsigned int output_height,
                 Emotion_Convert_Info *info)
{
   const unsigned char **rows, *ptr;
   unsigned int i, j, jump, rh;

   if (info->bpp[0] != 1) ERR("Plane 0 bpp != 1");
   if (info->bpp[1] != 1) ERR("Plane 1 bpp != 1");
   if (info->bpp[2] != 1) ERR("Plane 2 bpp != 1");

   rh = output_height;
   rows = (const unsigned char **)evas_data;

   ptr = info->plane_ptr[0];
   jump = info->stride[0];
   for (i = 0; i < rh; i++, ptr += jump) rows[i] = ptr;

   ptr = info->plane_ptr[1];
   jump = info->stride[1];
   for (j = 0; j < (rh / 2); j++, i++, ptr += jump) rows[i] = ptr;

   ptr = info->plane_ptr[2];
   jump = info->stride[2];
   for (j = 0; j < (rh / 2); j++, i++, ptr += jump) rows[i] = ptr;
}

/**
 * @brief Sets up Evas plane pointers for YUY2 (packed YUV 4:2:2) format.
 *
 * For YUY2, which is a packed format (not planar like I420/YV12), Evas still
 * expects an array of row pointers. Each pointer in `rows` will point to the
 * beginning of a line in the `gst_data` buffer.
 * The YUY2 format stores pixels as Y0-U0-Y1-V0, Y2-U1-Y3-V1, etc.
 *
 * `evas_data` will be structured as:
 *   rows[0] = pointer to gst_data row 0
 *   rows[1] = pointer to gst_data row 1
 *   ...
 *   rows[output_height-1] = pointer to gst_data row output_height-1
 *
 * @param[out] evas_data Pointer to an array that will be filled with pointers to each row of YUY2 data.
 *                       Effectively `const unsigned char **rows`.
 * @param[in] gst_data Pointer to the source GStreamer image data buffer (YUY2 packed).
 * @param w Width of the image in pixels.
 * @param h Height of the image in pixels (unused).
 * @param output_height The number of lines to process.
 * @param info Pointer to Emotion_Convert_Info structure (unused).
 */
static void
_evas_video_yuy2(unsigned char *evas_data, const unsigned char *gst_data,
                 unsigned int w, unsigned int h EINA_UNUSED,
                 unsigned int output_height,
                 Emotion_Convert_Info *info EINA_UNUSED)
{
   const unsigned char **rows;
   unsigned int i, stride;

   // XXX: need to check offset and stride that gst provide and what they
   // mean with a non-planar format like yuy2
   rows = (const unsigned char **)evas_data;

   stride = GST_ROUND_UP_4(w * 2);

   for (i = 0; i < output_height; i++) rows[i] = &gst_data[i * stride];
}

/**
 * @brief Sets up Evas plane pointers for NV12 (semi-planar YUV 4:2:0) format.
 *
 * NV12 has two planes: the first is the Y plane, and the second is an interleaved
 * U/V plane (e.g., U0V0, U1V1, ...).
 * This function populates `evas_data` with pointers to rows in these planes.
 *
 * `evas_data` will be structured as:
 *   rows[0] = pointer to Y plane row 0
 *   ...
 *   rows[rh-1] = pointer to Y plane row rh-1
 *   rows[rh] = pointer to UV plane row 0
 *   ...
 *   rows[rh + rh/2 - 1] = pointer to UV plane row rh/2 - 1
 *
 * @param[out] evas_data Pointer to an array that will be filled with pointers to each row of each plane.
 *                       Effectively `const unsigned char **rows`.
 * @param[in] gst_data Pointer to the source GStreamer image data buffer (unused).
 * @param w Width of the image in pixels (unused).
 * @param h Height of the image in pixels (unused).
 * @param output_height The height of the Y plane.
 * @param info Pointer to Emotion_Convert_Info structure containing plane pointers, strides, and bpp.
 *             info->plane_ptr[0] is Y plane, info->plane_ptr[1] is UV interleaved plane.
 */
static void
_evas_video_nv12(unsigned char *evas_data,
                 const unsigned char *gst_data EINA_UNUSED,
                 unsigned int w EINA_UNUSED, unsigned int h EINA_UNUSED,
                 unsigned int output_height, Emotion_Convert_Info *info)
{
   const unsigned char **rows, *ptr;
   unsigned int i, j, jump, rh;

   if (info->bpp[0] != 1) ERR("Plane 0 bpp != 1");
   // XXX: not sure this should be 1 but 2 bytes per pixel... no?
   //if (info->bpp[1] != 1) ERR("Plane 1 bpp != 1");

   rh = output_height;
   rows = (const unsigned char **)evas_data;

   ptr = info->plane_ptr[0];
   jump = info->stride[0];
   for (i = 0; i < rh; i++, ptr += jump) rows[i] = ptr;

   ptr = info->plane_ptr[1];
   jump = info->stride[1];
   for (j = 0; j < (rh / 2); j++, i++, ptr += jump) rows[i] = ptr;
}

/**
 * @brief Array defining mappings between GStreamer video formats/colorimetry
 * and Evas colorspaces, along with their respective conversion functions.
 *
 * Each entry in this array represents a supported conversion path.
 *
 * Structure of each element:
 * {
 *   const char *name;                   ///< Human-readable name for the format (e.g., "I420-709").
 *   GstVideoFormat format;              ///< GStreamer video format (e.g., GST_VIDEO_FORMAT_I420).
 *   GstVideoColorMatrix matrix;         ///< GStreamer color matrix (e.g., GST_VIDEO_COLOR_MATRIX_BT709).
 *   Evas_Colorspace evas_colorspace;    ///< Corresponding Evas colorspace (e.g., EVAS_COLORSPACE_YCBCR422P709_PL).
 *   EmotionVideoConverterFunc func;     ///< Pointer to the conversion function for this format.
 *   Eina_Bool direct_render;            ///< EINA_TRUE if Evas can render this format directly (planar/packed setup),
 *                                       ///< EINA_FALSE if pixel-by-pixel conversion is needed.
 * }
 *
 * Example entry:
 * { "I420-709", GST_VIDEO_FORMAT_I420, GST_VIDEO_COLOR_MATRIX_BT709,
 *    EVAS_COLORSPACE_YCBCR422P709_PL, _evas_video_i420, EINA_TRUE }
 * This means:
 * - Named "I420-709".
 * - GStreamer format is I420.
 * - GStreamer color matrix is BT.709.
 * - Evas will use YCbCr 4:2:2 Planar with BT.709 colorimetry.
 * - The function `_evas_video_i420` will be used to prepare the data for Evas.
 * - Evas can directly use the plane pointers set up by `_evas_video_i420`.
 */
const ColorSpace_Format_Conversion colorspace_format_conversion[] = {
  { "I420-709", GST_VIDEO_FORMAT_I420, GST_VIDEO_COLOR_MATRIX_BT709,
     EVAS_COLORSPACE_YCBCR422P709_PL, _evas_video_i420, EINA_TRUE },
  { "I420", GST_VIDEO_FORMAT_I420, GST_VIDEO_COLOR_MATRIX_BT601,
     EVAS_COLORSPACE_YCBCR422P601_PL, _evas_video_i420, EINA_TRUE },

  { "YV12-709", GST_VIDEO_FORMAT_YV12, GST_VIDEO_COLOR_MATRIX_BT709,
     EVAS_COLORSPACE_YCBCR422P709_PL, _evas_video_yv12, EINA_TRUE },
  { "YV12", GST_VIDEO_FORMAT_YV12, GST_VIDEO_COLOR_MATRIX_BT601,
     EVAS_COLORSPACE_YCBCR422P601_PL, _evas_video_yv12, EINA_TRUE },

  { "YUY2", GST_VIDEO_FORMAT_YUY2, GST_VIDEO_COLOR_MATRIX_BT601,
     EVAS_COLORSPACE_YCBCR422601_PL, _evas_video_yuy2, EINA_FALSE },
  { "NV12", GST_VIDEO_FORMAT_NV12, GST_VIDEO_COLOR_MATRIX_BT601,
     EVAS_COLORSPACE_YCBCR420NV12601_PL, _evas_video_nv12, EINA_TRUE },
   // XXX:
   // XXX: need to add nv12 709 colorspace support to evas itself.
   // XXX: this makes gst streams work when they are nv12 709 but maybe
   // XXX: will display in slightly off color.. but in the end this needs
   // XXX: fixing to display correctly.
   // XXX:
  { "NV12-709", GST_VIDEO_FORMAT_NV12, GST_VIDEO_COLOR_MATRIX_BT709,
     EVAS_COLORSPACE_YCBCR420NV12601_PL, _evas_video_nv12, EINA_TRUE },
   // XXX:
   // XXX:
   // XXX:

  { "BGR", GST_VIDEO_FORMAT_BGR, GST_VIDEO_COLOR_MATRIX_UNKNOWN,
     EVAS_COLORSPACE_ARGB8888, _evas_video_bgr, EINA_FALSE },
  { "BGRx", GST_VIDEO_FORMAT_BGRx, GST_VIDEO_COLOR_MATRIX_UNKNOWN,
     EVAS_COLORSPACE_ARGB8888, _evas_video_bgrx, EINA_FALSE },
  { "BGRA", GST_VIDEO_FORMAT_BGRA, GST_VIDEO_COLOR_MATRIX_UNKNOWN,
     EVAS_COLORSPACE_ARGB8888, _evas_video_bgra, EINA_FALSE },

  { NULL, 0, 0, 0, NULL, 0 }
};

