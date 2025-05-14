/**
 * @file
 * @brief Public header for the Evas Buffer Engine.
 *
 * This header defines the structures and function prototypes used by
 * the Evas buffer engine. It includes logging macros and the definition
 * of the Outbuf structure, which represents the output buffer.
 */
#ifndef EVAS_ENGINE_H
#define EVAS_ENGINE_H
#include "evas_common_private.h"

#include "../software_generic/Evas_Engine_Software_Generic.h"

/*  this thing is for eina_log */
extern int _evas_engine_buffer_log_dom ;

#ifdef ERR
# undef ERR
#endif
#define ERR(...) EINA_LOG_DOM_ERR(_evas_engine_buffer_log_dom, __VA_ARGS__)

#ifdef DBG
# undef DBG
#endif
#define DBG(...) EINA_LOG_DOM_DBG(_evas_engine_buffer_log_dom, __VA_ARGS__)

#ifdef INF
# undef INF
#endif
#define INF(...) EINA_LOG_DOM_INFO(_evas_engine_buffer_log_dom, __VA_ARGS__)

#ifdef WRN
# undef WRN
#endif
#define WRN(...) EINA_LOG_DOM_WARN(_evas_engine_buffer_log_dom, __VA_ARGS__)

#ifdef CRI
# undef CRI
#endif
#define CRI(...) EINA_LOG_DOM_CRIT(_evas_engine_buffer_log_dom, __VA_ARGS__)

/**
 * @brief Represents an output buffer for the Evas Buffer Engine.
 *
 * This structure holds all the necessary information about the target
 * memory buffer where Evas will render, including its dimensions,
 * color depth, pixel data pointer, and callback functions for managing
 * updates and buffer switching.
 */
struct _Outbuf
{
   int                           w; /**< Width of the buffer in pixels. */
   int                           h; /**< Height of the buffer in pixels. */
   Outbuf_Depth                  depth; /**< Color depth and format of the buffer. */

   void                         *dest; /**< Pointer to the destination pixel data. */
   unsigned int                  dest_row_bytes; /**< Number of bytes per row in the destination buffer. */
   void                         *switch_data; /**< User data for buffer switching callbacks. */

   int                           alpha_level; /**< Alpha threshold for color keying (0-255). Not typically used if use_color_key is false. */
   DATA32                        color_key;   /**< Color key value (e.g., 0xRRGGBB). Alpha component is usually ignored or set to 0. */
   Eina_Bool                     use_color_key : 1; /**< Flag indicating if color keying is enabled. */
   Eina_Bool                     first_frame : 1; /**< Flag indicating if this is the first frame being rendered. */

   /**
    * @brief Callbacks for buffer management.
    * These functions are provided by the user of the buffer engine
    * to integrate with their specific buffer handling mechanisms.
    */
   struct {
      /**
       * @brief Called to get a memory region for Evas to render an update into.
       * @param x The x-coordinate of the update region.
       * @param y The y-coordinate of the update region.
       * @param w The width of the update region.
       * @param h The height of the update region.
       * @param[out] row_bytes Pointer to store the row stride of the returned buffer.
       * @return Pointer to the memory for the update region.
       */
      void * (*new_update_region) (int x, int y, int w, int h, int *row_bytes);
      /**
       * @brief Called to free a previously allocated update region.
       * @param x The x-coordinate of the update region.
       * @param y The y-coordinate of the update region.
       * @param w The width of the update region.
       * @param h The height of the update region.
       * @param data The memory pointer returned by new_update_region.
       */
      void   (*free_update_region) (int x, int y, int w, int h, void *data);
      /**
       * @brief Called when Evas is ready to switch/display the rendered buffer.
       * @param data The switch_data provided in _Outbuf.
       * @param dest_buffer Pointer to the rendered Evas buffer content.
       * @return A new destination buffer pointer if applicable, or the same dest_buffer.
       */
      void * (*switch_buffer) (void *data, void *dest_buffer);
   } func; /**< User-provided callback functions for buffer operations. */

   /**
    * @brief Private data for the output buffer.
    * Internal structures used by the engine.
    */
   struct {
      RGBA_Image                *back_buf; /**< Internal back buffer used for rendering. */
   } priv; /**< Private engine data. */
};

/****/

/**
 * @brief Initializes the buffer output buffer system.
 *
 * This function should be called once before any other
 * evas_buffer_outbuf_buf_* functions are used. It sets up
 * internal states or resources if needed.
 */
void         evas_buffer_outbuf_buf_init                   (void);
/**
 * @brief Frees an Outbuf structure and its associated resources.
 * @param buf The Outbuf to free.
 */
void         evas_buffer_outbuf_buf_free                   (Outbuf *buf);

/**
 * @brief Updates the properties of an existing framebuffer Outbuf.
 *
 * This function is likely used to change parameters of an already
 * setup Outbuf, such as its dimensions or destination buffer, without
 * needing to free and recreate it entirely.
 *
 * @param buf The Outbuf to update.
 * @param w New width.
 * @param h New height.
 * @param depth New color depth.
 * @param dest New destination buffer pointer.
 * @param dest_row_bytes New row stride for the destination buffer.
 * @param use_color_key New color key usage flag.
 * @param color_key New color key value.
 * @param alpha_level New alpha threshold.
 * @param new_update_region New callback for acquiring update regions.
 * @param free_update_region New callback for freeing update regions.
 * @param switch_buffer New callback for buffer switching.
 * @param switch_data New user data for callbacks.
 */
void         evas_buffer_outbuf_buf_update_fb              (Outbuf *buf,
                                                            int w, int h,
                                                            Outbuf_Depth depth,
                                                            void *dest,
                                                            int dest_row_bytes,
                                                            int use_color_key,
                                                            DATA32 color_key,
                                                            int alpha_level,
                                                            void * (*new_update_region) (int x, int y, int w, int h, int *row_bytes),
                                                            void   (*free_update_region) (int x, int y, int w, int h, void *data),
                                                            void * (*switch_buffer) (void *data, void *dest_buffer),
                                                            void *switch_data);
/**
 * @brief Sets up and creates a new framebuffer Outbuf.
 *
 * This is a primary function for initializing an Outbuf for rendering.
 * It allocates and configures the Outbuf structure based on the provided
 * parameters.
 *
 * @param w Width of the buffer.
 * @param h Height of the buffer.
 * @param depth Color depth and format.
 *        Example: OUTBUF_DEPTH_ARGB_32BPP_8888_8888
 * @param dest Pointer to the target memory buffer.
 * @param dest_row_bytes Stride (bytes per row) of the target buffer.
 * @param use_color_key Boolean, true if color keying is enabled.
 * @param color_key The color key value if use_color_key is true.
 *        Example: 0x00FF00FF for a magenta key (assuming ARGB or RGBA format where alpha is opaque for key).
 * @param alpha_level Alpha threshold for transparency effects (0-255).
 * @param new_update_region Callback to get a region for Evas to draw into.
 * @param free_update_region Callback to free the region obtained by new_update_region.
 * @param switch_buffer Callback invoked when Evas has finished rendering a frame
 *                      and the buffer is ready to be displayed or processed.
 *                      The `void *dest` parameter in this callback points to the
 *                      rendered pixel data.
 * @param switch_data User data passed to the switch_buffer callback.
 * @return A pointer to the newly created Outbuf, or NULL on failure.
 */
Outbuf      *evas_buffer_outbuf_buf_setup_fb               (int w, int h, Outbuf_Depth depth, void *dest, int dest_row_bytes, int use_color_key, DATA32 color_key, int alpha_level,
							    void * (*new_update_region) (int x, int y, int w, int h, int *row_bytes),
							    void   (*free_update_region) (int x, int y, int w, int h, void *data),
                                                            void * (*switch_buffer)(void *switch_data, void *dest),
                                                            void *switch_data);

/**
 * @brief Reconfigures an existing Outbuf.
 *
 * Allows changing properties like width, height, rotation, and color depth
 * of an already initialized Outbuf.
 *
 * @param ob The Outbuf to reconfigure.
 * @param w New width.
 * @param h New height.
 * @param rot New rotation value (specifics depend on engine capabilities).
 * @param depth New color depth.
 */
void         evas_buffer_outbuf_reconfigure                (Outbuf *ob, int w, int h, int rot, Outbuf_Depth depth);
/**
 * @brief Allocates a new region within the Outbuf for an update.
 *
 * This function is called by the Evas core to obtain a memory area
 * where it can render a part of the canvas that needs updating.
 * The actual memory might be provided by the user via the
 * `new_update_region` callback in `Outbuf->func`.
 *
 * @param buf The Outbuf.
 * @param x X-coordinate of the region.
 * @param y Y-coordinate of the region.
 * @param w Width of the region.
 * @param h Height of the region.
 * @param[out] cx Clipped X-coordinate (if region is clipped).
 * @param[out] cy Clipped Y-coordinate.
 * @param[out] cw Clipped width.
 * @param[out] ch Clipped height.
 * @return Pointer to the image data for the update region, or NULL on failure.
 *         This is typically an RGBA_Image.
 */
void        *evas_buffer_outbuf_buf_new_region_for_update  (Outbuf *buf, int x, int y, int w, int h, int *cx, int *cy, int *cw, int *ch);
/**
 * @brief Frees a region previously allocated for update.
 *
 * After Evas is done rendering to an update region, this function
 * is called to release it. This might involve calling the user-provided
 * `free_update_region` callback.
 *
 * @param buf The Outbuf.
 * @param update The RGBA_Image (update region) to free.
 */
void         evas_buffer_outbuf_buf_free_region_for_update (Outbuf *buf, RGBA_Image *update);
/**
 * @brief Pushes an updated region to the output.
 *
 * This function is called after Evas has rendered an update. It's
 * responsible for making the updated pixels visible, which might involve
 * copying data to the final destination buffer or notifying the user.
 *
 * @param buf The Outbuf.
 * @param update The RGBA_Image containing the updated pixels.
 * @param x X-coordinate of the update.
 * @param y Y-coordinate of the update.
 * @param w Width of the update.
 * @param h Height of the update.
 */
void         evas_buffer_outbuf_buf_push_updated_region    (Outbuf *buf, RGBA_Image *update, int x, int y, int w, int h);
/**
 * @brief Handles buffer switching after rendering.
 *
 * This function is called when Evas has completed rendering a frame
 * (or parts of it, depending on the render_mode). It finalizes the
 * frame and calls the user's `switch_buffer` callback.
 *
 * @param buf The Outbuf.
 * @param surface_damage Regions of the Evas surface that were damaged (unused by this engine).
 * @param buffer_damage Regions of the output buffer that were damaged (unused by this engine).
 * @param render_mode The current Evas render mode.
 */
void         evas_buffer_outbuf_buf_switch_buffer          (Outbuf *buf, Tilebuf_Rect *surface_damage, Tilebuf_Rect *buffer_damage, Evas_Render_Mode render_mode);
/**
 * @brief Gets the swap mode of the output buffer.
 * @param buf The Outbuf.
 * @return The current Render_Output_Swap_Mode.
 *         Example: RENDER_OUTPUT_SWAP_MODE_FULL, RENDER_OUTPUT_SWAP_MODE_COPY.
 */
Render_Output_Swap_Mode evas_buffer_outbuf_buf_swap_mode_get(Outbuf *buf);
/**
 * @brief Gets the rotation of the output buffer.
 * @param buf The Outbuf.
 * @return The current rotation value (0, 90, 180, 270).
 */
int          evas_buffer_outbuf_buf_rot_get                (Outbuf *buf);

#endif
