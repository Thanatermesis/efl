/**
 * @file evas_pipe.h
 * @brief Evas rendering pipe API.
 *
 * This header defines the API for Evas's optional rendering pipeline system.
 * This system allows for non-immediate, potentially threaded rendering of
 * Evas objects. It's designed to improve performance by parallelizing
 * parts of the rendering process.
 */
#ifndef _EVAS_PIPE_H
#define _EVAS_PIPE_H

#include <sys/time.h>
#include "language/evas_bidi_utils.h"

/* image rendering pipelines... new optional system - non-immediate and
 * threadable
 */

/**
 * @brief Initializes the common rendering pipe system.
 * @return EINA_TRUE if threading is enabled and initialized, EINA_FALSE otherwise.
 * @ingroup Evas_Common_Pipe_Group
 *
 * This function sets up the worker threads for rendering and loading/preparation
 * tasks if multi-threading is supported and enabled (typically based on CPU count).
 * It should be called before any other pipe functions are used.
 */
EVAS_API Eina_Bool evas_common_pipe_init(void);

/**
 * @brief Frees all operations in the rendering pipe associated with an image.
 * @param im The image whose rendering pipe is to be freed.
 * @ingroup Evas_Common_Pipe_Group
 *
 * This function iterates through all operations in the pipe for the given image,
 * calls their respective free functions, and deallocates the pipe segments.
 * Recycled cutout rectangles are returned to the appropriate thread's trash.
 */
EVAS_API void evas_common_pipe_free(RGBA_Image *im);

/**
 * @brief Adds a rectangle drawing operation to the image's rendering pipe.
 * @param dst The destination image.
 * @param dc The draw context.
 * @param x The x-coordinate of the rectangle.
 * @param y The y-coordinate of the rectangle.
 * @param w The width of the rectangle.
 * @param h The height of the rectangle.
 * @ingroup Evas_Common_Pipe_Group
 */
EVAS_API void evas_common_pipe_rectangle_draw(RGBA_Image *dst, RGBA_Draw_Context *dc, int x, int y, int w, int h);

/**
 * @brief Adds a line drawing operation to the image's rendering pipe.
 * @param dst The destination image.
 * @param dc The draw context.
 * @param x0 The x-coordinate of the start point of the line.
 * @param y0 The y-coordinate of the start point of the line.
 * @param x1 The x-coordinate of the end point of the line.
 * @param y1 The y-coordinate of the end point of the line.
 * @ingroup Evas_Common_Pipe_Group
 */
EVAS_API void evas_common_pipe_line_draw(RGBA_Image *dst, RGBA_Draw_Context *dc, int x0, int y0, int x1, int y1);

/**
 * @brief Adds a polygon drawing operation to the image's rendering pipe.
 * @param dst The destination image.
 * @param dc The draw context.
 * @param points A list of points defining the polygon.
 * @param x The x-offset for the polygon.
 * @param y The y-offset for the polygon.
 * @ingroup Evas_Common_Pipe_Group
 */
EVAS_API void evas_common_pipe_poly_draw(RGBA_Image *dst, RGBA_Draw_Context *dc, RGBA_Polygon_Point *points, int x, int y);

/**
 * @brief Adds a text drawing operation to the image's rendering pipe.
 * @param dst The destination image.
 * @param dc The draw context.
 * @param x The x-coordinate for the text.
 * @param y The y-coordinate for the text (baseline).
 * @param intl_props The international text properties.
 * @ingroup Evas_Common_Pipe_Group
 */
EVAS_API void evas_common_pipe_text_draw(RGBA_Image *dst, RGBA_Draw_Context *dc, int x, int y, Evas_Text_Props *intl_props);

/**
 * @brief Queues text properties for preparation (glyph generation, layout).
 * @param text_props The text properties to prepare.
 * @ingroup Evas_Common_Pipe_Group
 *
 * This function adds the text properties to a task list for a loading/preparation
 * thread if they need updating (e.g., text changed, font changed, or not yet prepared).
 */
EVAS_API void evas_common_pipe_text_prepare(Evas_Text_Props *text_props);

/**
 * @brief Queues an image for loading if its data is not yet loaded or is dirty.
 * @param im The image to potentially load.
 * @ingroup Evas_Common_Pipe_Group
 *
 * If the image is an ARGB8888 image and its pixel data hasn't been loaded from
 * its source, or if its colorspace data is missing or dirty, it's added to a
 * task list for a loading/preparation thread.
 */
EVAS_API void evas_common_pipe_image_load(RGBA_Image *im);

/**
 * @brief Adds an image drawing (scaling) operation to the destination image's rendering pipe.
 * @param src The source image.
 * @param dst The destination image.
 * @param dc The draw context.
 * @param smooth EINA_TRUE for smooth scaling, EINA_FALSE for nearest-neighbor.
 * @param src_region_x X-coordinate of the source region.
 * @param src_region_y Y-coordinate of the source region.
 * @param src_region_w Width of the source region.
 * @param src_region_h Height of the source region.
 * @param dst_region_x X-coordinate of the destination region.
 * @param dst_region_y Y-coordinate of the destination region.
 * @param dst_region_w Width of the destination region.
 * @param dst_region_h Height of the destination region.
 * @ingroup Evas_Common_Pipe_Group
 */
EVAS_API void evas_common_pipe_image_draw(RGBA_Image *src, RGBA_Image *dst, RGBA_Draw_Context *dc, int smooth, int src_region_x, int src_region_y, int src_region_w, int src_region_h, int dst_region_x, int dst_region_y, int dst_region_w, int dst_region_h);

/**
 * @brief Begins the rendering process for an image that involves map operations.
 * @param root The root image for which map rendering should start.
 * @ingroup Evas_Common_Pipe_Group
 *
 * This function first ensures that any pending loading/preparation tasks for the
 * root image and its dependencies are completed. Then, it recursively renders
 * any map operations associated with the root image and its sources.
 */
EVAS_API void evas_common_pipe_map_begin(RGBA_Image *root);

/**
 * @brief Adds a map rendering operation to the image's rendering pipe.
 * @param src The source image to be mapped.
 * @param dst The destination image.
 * @param dc The draw context.
 * @param m The map definition (points, colors, etc.).
 * @param smooth EINA_TRUE for smooth rendering, EINA_FALSE otherwise.
 * @param level The subdivision level for map rendering (higher is smoother but slower).
 * @ingroup Evas_Common_Pipe_Group
 */
EVAS_API void evas_common_pipe_map_draw(RGBA_Image *src, RGBA_Image *dst,
				    RGBA_Draw_Context *dc, RGBA_Map *m,
				    int smooth, int level);
/**
 * @brief Flushes the rendering pipe for an image, executing all queued operations.
 * @param im The image whose rendering pipe is to be flushed.
 * @ingroup Evas_Common_Pipe_Group
 *
 * If multi-threading is active, this function waits for worker threads to complete
 * their assigned rendering tasks for the image. If not, it processes the pipe
 * operations sequentially in the current thread. After rendering, it cleans up
 * CPU optimizations and frees the pipe.
 */
EVAS_API void evas_common_pipe_flush(RGBA_Image *im);

#endif /* _EVAS_PIPE_H */
