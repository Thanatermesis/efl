#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <math.h>
#include <float.h>

#include <Eina.h>
#include <Ector.h>
#include <software/Ector_Software.h>

#include "ector_private.h"
#include "ector_software_private.h"

#define MY_CLASS ECTOR_RENDERER_SOFTWARE_IMAGE_CLASS

typedef struct _Ector_Renderer_Software_Image_Data Ector_Renderer_Software_Image_Data;

/**
 * @brief Private data for the Ector Software Image Renderer.
 *
 * This structure holds all the necessary data for rendering an image
 * using the software rendering backend. It includes references to the
 * target surface, the image data, transformation matrices, and drawing
 * boundaries.
 */
struct _Ector_Renderer_Software_Image_Data
{
   Ector_Software_Surface_Data *surface; /**< Pointer to the software surface data where rendering occurs. */
   Ector_Renderer_Image_Data   *image; /**< Pointer to the image data to be rendered. */
   Ector_Renderer_Data         *base; /**< Pointer to the base renderer data. */
   Ector_Buffer                *comp; /**< Optional composition buffer (e.g., for masking). */
   Efl_Gfx_Vg_Composite_Method comp_method; /**< Composition method to be used. */
   int                          opacity; /**< Overall opacity for the image rendering. */
   Eina_Matrix3                 inv_m; /**< Inverse transformation matrix. */
   struct {
      int x1, y1, x2, y2; /**< Boundary coordinates (x1, y1, x2, y2) for the drawing area. */
   } boundary; /**< Structure to hold the calculated drawing boundary on the target surface. */
};

/**
 * @brief Prepares the image renderer for drawing.
 *
 * This function initializes the renderer's internal state based on the
 * current image, surface, and transformation settings. It calculates
 * the drawing boundaries and the inverse transformation matrix.
 *
 * @param obj The Ector_Renderer_Software_Image object.
 * @param pd The private data of the Ector_Renderer_Software_Image object.
 * @return EINA_TRUE if preparation was successful, EINA_FALSE otherwise.
 */
static Eina_Bool
_ector_renderer_software_image_ector_renderer_prepare(Eo *obj,
                                                      Ector_Renderer_Software_Image_Data *pd)
{
   if (!pd->surface)
     pd->surface = efl_data_xref(pd->base->surface, ECTOR_SOFTWARE_SURFACE_CLASS, obj);

   if (!pd->image->buffer || !pd->surface->rasterizer->fill_data.raster_buffer)
     return EINA_FALSE;

   Eina_Matrix3 m;
   double m11, m12, m21, m22, m31, m32;
   int x = pd->surface->x + (int)pd->base->origin.x;
   int y = pd->surface->y + (int)pd->base->origin.y;
   int image_w, image_h;
   ector_buffer_size_get(pd->image->buffer, &image_w, &image_h);

   double px[4] = {x, x + image_w, x, x + image_w};
   double py[4] = {y, y, y + image_h, y + image_h};

   //Only use alpha color
   pd->opacity = pd->base->color.a;
   /*ector_software_rasterizer_color_set(pd->surface->rasterizer,
     pd->base->color.r,
     pd->base->color.g,
     pd->base->color.b,
     pd->base->color.a);*/

   if (!pd->base->m)
     {
        eina_matrix3_identity(&m);
        eina_matrix3_scale(&m, (double)pd->surface->rasterizer->fill_data.raster_buffer->generic->w / (double)image_w,
                           (double)pd->surface->rasterizer->fill_data.raster_buffer->generic->h / (double)image_h);
     }
   else
     eina_matrix3_copy(&m, pd->base->m);
   eina_matrix3_values_get(&m, &m11, &m12, NULL,
                           &m21, &m22, NULL,
                           &m31, &m32, NULL);
   //Calc draw boundbox
   pd->boundary.x1 = MAX(pd->surface->rasterizer->fill_data.raster_buffer->generic->w , (unsigned int)image_w);
   pd->boundary.y1 = MAX(pd->surface->rasterizer->fill_data.raster_buffer->generic->h , (unsigned int)image_h);
   pd->boundary.x2 = 0; pd->boundary.y2 = 0;
   for (int i = 0; i < 4; i++)
     {
        pd->boundary.x1 = MIN(pd->boundary.x1, (int)(((px[i] * m11) + (py[i] * m21) + m31) + 0.5));
        pd->boundary.y1 = MIN(pd->boundary.y1, (int)(((px[i] * m12) + (py[i] * m22) + m32) + 0.5));

        pd->boundary.x2 = MAX(pd->boundary.x2, (int)(((px[i] * m11) + (py[i] * m21) + m31) + 0.5));
        pd->boundary.y2 = MAX(pd->boundary.y2, (int)(((px[i] * m12) + (py[i] * m22) + m32) + 0.5));
     }

   eina_matrix3_inverse(&m, &pd->inv_m);

   return EINA_TRUE;
}

//FIXME: We need to implement that apply op, clips and mul_col.
/**
 * @brief Draws the image onto the target surface.
 *
 * This function performs the actual rendering of the image. It iterates
 * over the pixels within the calculated boundary, applies the inverse
 * transformation to find the corresponding source image pixel, and blends
 * it onto the destination surface.
 *
 * @param obj The Ector_Renderer_Software_Image object (unused).
 * @param pd The private data of the Ector_Renderer_Software_Image object.
 * @param op The rendering operation (unused).
 * @param clips An array of clipping regions (unused).
 * @param mul_col A color multiplier (unused).
 * @return EINA_TRUE if drawing was successful or not needed (e.g., fully transparent), EINA_FALSE on error.
 */
static Eina_Bool
_ector_renderer_software_image_ector_renderer_draw(Eo *obj EINA_UNUSED,
                                                   Ector_Renderer_Software_Image_Data *pd,
                                                   Efl_Gfx_Render_Op op EINA_UNUSED, Eina_Array *clips EINA_UNUSED,
                                                   unsigned int mul_col EINA_UNUSED)
{
   if (!pd->image->buffer || !pd->surface->rasterizer->fill_data.raster_buffer->pixels.u32)
          return EINA_FALSE;

   if (pd->opacity == 0)
      return EINA_TRUE;

   const int pix_stride = pd->surface->rasterizer->fill_data.raster_buffer->stride / 4;
   Ector_Software_Buffer_Base_Data *comp = pd->comp ? efl_data_scope_get(pd->comp, ECTOR_SOFTWARE_BUFFER_BASE_MIXIN) : NULL;
   Ector_Software_Buffer_Base_Data *bpd = efl_data_scope_get(pd->image->buffer, ECTOR_SOFTWARE_BUFFER_BASE_MIXIN);
   double im11, im12, im21, im22, im31, im32;
   uint32_t *dst_buf, *src_buf;
   int image_w, image_h;

   int dst_buf_width = MIN(pd->boundary.x2, (int)pd->surface->rasterizer->fill_data.raster_buffer->generic->w);
   int dst_buf_height = MIN(pd->boundary.y2, (int)pd->surface->rasterizer->fill_data.raster_buffer->generic->h);

   ector_buffer_size_get(pd->image->buffer, &image_w, &image_h);

   dst_buf = pd->surface->rasterizer->fill_data.raster_buffer->pixels.u32;
   src_buf = bpd->pixels.u32;

   eina_matrix3_values_get(&pd->inv_m, &im11, &im12, NULL,
                                       &im21, &im22, NULL,
                                       &im31, &im32, NULL);

   //Draw
   for (int local_y = pd->boundary.y1; local_y < dst_buf_height; local_y++)
     {
        for (int local_x = pd->boundary.x1; local_x <  dst_buf_width; local_x++)
          {
             uint32_t *dst = dst_buf + ((int)local_x + ((int)local_y * pix_stride));
             int rx, ry;
             rx = (int)(((double)local_x * im11) + ((double)local_y * im21) + im31 + 0.5);
             ry = (int)(((double)local_x * im12) + ((double)local_y * im22) + im32 + 0.5);
             if (rx < 0 || rx >= image_w || ry < 0 || ry >= image_h)
               continue;
             uint32_t *src = src_buf + (rx + (ry * image_w));  //FIXME: use to stride
             uint32_t temp = 0x0;
             if (comp)
               {
                  uint32_t *m = comp->pixels.u32 + ((int)local_x + ((int)local_y * comp->generic->w));
                  //FIXME : This comping can work only matte case.
                  //        We need consider to inverse matte case.
                  temp = draw_mul_256((((*m)>>24) * pd->opacity)>>8, *src);
               }
             else
               {
                  temp = draw_mul_256(pd->opacity, *src);
               }
             int inv_alpha = 255 - ((temp) >> 24);
             *dst = temp + draw_mul_256(inv_alpha, *dst);
          }
     }

   return EINA_TRUE;
}

/**
 * @brief Constructor for the Ector_Renderer_Software_Image object.
 *
 * Initializes the Ector_Renderer_Software_Image object and its private data.
 * It also establishes references to related image and base renderer data.
 *
 * @param obj The Eo object to construct.
 * @param pd The private data for the Ector_Renderer_Software_Image.
 * @return The constructed Eo object, or NULL on failure.
 */
static Eo *
_ector_renderer_software_image_efl_object_constructor(Eo *obj, Ector_Renderer_Software_Image_Data *pd)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));
   if (!obj) return NULL;

   pd->image = efl_data_xref(obj, ECTOR_RENDERER_IMAGE_MIXIN, obj);
   pd->base = efl_data_xref(obj, ECTOR_RENDERER_CLASS, obj);

   return obj;
}

/**
 * @brief Destructor for the Ector_Renderer_Software_Image object.
 *
 * Cleans up resources used by the Ector_Renderer_Software_Image object,
 * including unreferencing related data.
 *
 * @param obj The Eo object to destruct.
 * @param pd The private data of the Ector_Renderer_Software_Image.
 */
static void
_ector_renderer_software_image_efl_object_destructor(Eo *obj, Ector_Renderer_Software_Image_Data *pd)
{
   efl_data_xunref(pd->base->surface, pd->surface, obj);
   efl_data_xunref(obj, pd->base, obj);
   efl_data_xunref(obj, pd->image, obj);

   efl_destructor(efl_super(obj, MY_CLASS));
}

/**
 * @brief Calculates a CRC checksum for the renderer's current state.
 *
 * This function computes a CRC value based on the renderer's superclass
 * CRC and the image data. This can be used to detect changes in the
 * renderer's configuration or input.
 *
 * @param obj The Ector_Renderer_Software_Image object.
 * @param pd The private data of the Ector_Renderer_Software_Image object.
 * @return The calculated CRC value.
 */
unsigned int
_ector_renderer_software_image_ector_renderer_crc_get(const Eo *obj,
                                                      Ector_Renderer_Software_Image_Data *pd)
{
   unsigned int crc;

   crc = ector_renderer_crc_get(efl_super(obj, MY_CLASS));

   crc = eina_crc((void*) pd->image, sizeof (Ector_Renderer_Image_Data), crc, EINA_FALSE);
   return crc;
}

/**
 * @brief Sets the composition method and buffer for the renderer.
 *
 * This function configures how the image is composited with the target
 * surface, potentially using a composition buffer (e.g., for masking).
 *
 * @param obj The Ector_Renderer_Software_Image object (unused).
 * @param pd The private data of the Ector_Renderer_Software_Image object.
 * @param comp The Ector_Buffer to be used for composition (e.g., a mask).
 *             Can be NULL if no composition buffer is used.
 * @param method The Efl_Gfx_Vg_Composite_Method specifying the composition operation.
 *               Example: EFL_GFX_VG_COMPOSITE_METHOD_MASK for alpha masking.
 */
static void
_ector_renderer_software_image_ector_renderer_comp_method_set(Eo *obj EINA_UNUSED,
                                                              Ector_Renderer_Software_Image_Data *pd,
                                                              Ector_Buffer *comp,
                                                              Efl_Gfx_Vg_Composite_Method method)
{
   pd->comp = comp;
   pd->comp_method = method;
}

#include "ector_renderer_software_image.eo.c"
