#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Eina.h>
#include <Ector.h>

#include "ector_private.h"

#define MY_CLASS ECTOR_RENDERER_CLASS

/**
 * @internal
 * @brief Destructor for the Ector_Renderer object.
 *
 * This function is called when the Ector_Renderer object is being destroyed.
 * It cleans up resources allocated by the renderer, such as the transformation
 * matrix and the surface reference.
 *
 * @param obj The Efl_Object being destroyed.
 * @param pd The private data of the Ector_Renderer.
 */
static void
_ector_renderer_efl_object_destructor(Eo *obj, Ector_Renderer_Data *pd)
{
   efl_destructor(efl_super(obj, MY_CLASS));

   if (pd->m) free(pd->m);
   /*FIXME: pd-> surface will try efl_xref whenever surface_set is called.
            desturctor is called from a subclass, ref and unref do not match.
            So, Add this condition temporarily.*/
   if (efl_ref_count(pd->surface) > 0)
     efl_unref(pd->surface);
}

/**
 * @internal
 * @brief Finalizes the Ector_Renderer object.
 *
 * This function is called when the Ector_Renderer object is being finalized.
 * It ensures that a surface has been set before finalizing. If not, it logs
 * a critical error.
 *
 * @param obj The Efl_Object being finalized.
 * @param pd The private data of the Ector_Renderer.
 * @return The finalized Efl_Object, or NULL if finalization fails (e.g., surface not set).
 */
static Efl_Object *
_ector_renderer_efl_object_finalize(Eo *obj, Ector_Renderer_Data *pd)
{
   if (!pd->surface)
     {
        CRI("surface is not set yet, go fix your code!");
        return NULL;
     }
   pd->finalized = EINA_TRUE;
   return efl_finalize(efl_super(obj, MY_CLASS));
}

/**
 * @internal
 * @brief Gets the rendering surface associated with the renderer.
 *
 * @param obj The Ector_Renderer object (unused).
 * @param pd The private data of the Ector_Renderer.
 * @return The Ector_Surface used for rendering.
 */
static Ector_Surface *
_ector_renderer_surface_get(const Eo *obj EINA_UNUSED, Ector_Renderer_Data *pd)
{
   return pd->surface;
}

/**
 * @internal
 * @brief Sets the rendering surface for the renderer.
 *
 * This function can only be called during object creation (before finalization).
 * It takes a reference to the provided surface.
 *
 * @param obj The Ector_Renderer object (unused).
 * @param pd The private data of the Ector_Renderer.
 * @param s The Ector_Surface to be used for rendering.
 */
static void
_ector_renderer_surface_set(Eo *obj EINA_UNUSED, Ector_Renderer_Data *pd, Ector_Surface *s)
{
   if (pd->finalized)
     {
        CRI("surface_set can be called during object creation only!");
        return;
     }
   pd->surface = efl_xref(s, obj);
}

/**
 * @internal
 * @brief Sets the transformation matrix for the renderer.
 *
 * If @p m is NULL, the existing transformation matrix is freed. Otherwise,
 * the provided matrix @p m is copied.
 *
 * @param obj The Ector_Renderer object (unused).
 * @param pd The private data of the Ector_Renderer.
 * @param m The 3x3 transformation matrix to set, or NULL to clear the transformation.
 *          Example:
 *          Eina_Matrix3 matrix;
 *          eina_matrix3_identity(&matrix);
 *          // ... modify matrix ...
 *          ector_renderer_transformation_set(renderer, &matrix);
 */
static void
_ector_renderer_transformation_set(Eo *obj EINA_UNUSED,
                                   Ector_Renderer_Data *pd,
                                   const Eina_Matrix3 *m)
{
   if (!m)
     {
        free(pd->m);
        pd->m = NULL;
     }
   else
     {
        if (!pd->m) pd->m = malloc(sizeof (Eina_Matrix3));
        if (!pd->m) return;
        memcpy(pd->m, m, sizeof (Eina_Matrix3));
     }
}

/**
 * @internal
 * @brief Gets the current transformation matrix of the renderer.
 *
 * @param obj The Ector_Renderer object (unused).
 * @param pd The private data of the Ector_Renderer.
 * @return A pointer to the constant Eina_Matrix3 representing the transformation,
 *         or NULL if no transformation is set.
 */
static const Eina_Matrix3 *
_ector_renderer_transformation_get(const Eo *obj EINA_UNUSED,
                                   Ector_Renderer_Data *pd)
{
   return pd->m;
}

/**
 * @internal
 * @brief Sets the origin point for transformations.
 *
 * The origin is the point around which transformations like rotation and scaling occur.
 *
 * @param obj The Ector_Renderer object (unused).
 * @param pd The private data of the Ector_Renderer.
 * @param x The x-coordinate of the origin.
 * @param y The y-coordinate of the origin.
 */
static void
_ector_renderer_origin_set(Eo *obj EINA_UNUSED,
                           Ector_Renderer_Data *pd,
                           double x, double y)
{
   pd->origin.x = x;
   pd->origin.y = y;
}

/**
 * @internal
 * @brief Gets the origin point for transformations.
 *
 * @param obj The Ector_Renderer object (unused).
 * @param pd The private data of the Ector_Renderer.
 * @param x Pointer to store the x-coordinate of the origin. Can be NULL.
 * @param y Pointer to store the y-coordinate of the origin. Can be NULL.
 */
static void
_ector_renderer_origin_get(const Eo *obj EINA_UNUSED,
                           Ector_Renderer_Data *pd,
                           double *x, double *y)
{
   if (x) *x = pd->origin.x;
   if (y) *y = pd->origin.y;
}

/**
 * @internal
 * @brief Sets the visibility of the rendered object.
 *
 * @param obj The Ector_Renderer object (unused).
 * @param pd The private data of the Ector_Renderer.
 * @param v EINA_TRUE if visible, EINA_FALSE otherwise.
 */
static void
_ector_renderer_visibility_set(Eo *obj EINA_UNUSED,
                               Ector_Renderer_Data *pd,
                               Eina_Bool v)
{
   pd->visibility = v;
}

/**
 * @internal
 * @brief Gets the visibility of the rendered object.
 *
 * @param obj The Ector_Renderer object (unused).
 * @param pd The private data of the Ector_Renderer.
 * @return EINA_TRUE if visible, EINA_FALSE otherwise.
 */
static Eina_Bool
_ector_renderer_visibility_get(const Eo *obj EINA_UNUSED,
                               Ector_Renderer_Data *pd)
{
   return pd->visibility;
}

/**
 * @internal
 * @brief Sets the color used for rendering operations.
 *
 * Color components are integers ranging from 0 to 255.
 *
 * @param obj The Ector_Renderer object (unused).
 * @param pd The private data of the Ector_Renderer.
 * @param r Red component (0-255).
 * @param g Green component (0-255).
 * @param b Blue component (0-255).
 * @param a Alpha component (0-255, 0 is transparent, 255 is opaque).
 */
static void
_ector_renderer_color_set(Eo *obj EINA_UNUSED,
                          Ector_Renderer_Data *pd,
                          int r, int g, int b, int a)
{
   pd->color.r = r;
   pd->color.g = g;
   pd->color.b = b;
   pd->color.a = a;
}

/**
 * @internal
 * @brief Gets the current color used for rendering operations.
 *
 * @param obj The Ector_Renderer object (unused).
 * @param pd The private data of the Ector_Renderer.
 * @param r Pointer to store the red component. Can be NULL.
 * @param g Pointer to store the green component. Can be NULL.
 * @param b Pointer to store the blue component. Can be NULL.
 * @param a Pointer to store the alpha component. Can be NULL.
 */
static void
_ector_renderer_color_get(const Eo *obj EINA_UNUSED,
                          Ector_Renderer_Data *pd,
                          int *r, int *g, int *b, int *a)
{
   if (r) *r = pd->color.r;
   if (g) *g = pd->color.g;
   if (b) *b = pd->color.b;
   if (a) *a = pd->color.a;
}

/**
 * @internal
 * @brief Calculates a CRC checksum for the renderer's current state.
 *
 * The CRC is based on the current color, origin, and transformation matrix (if set).
 * This can be used to quickly check if the renderer's state has changed.
 *
 * @param obj The Ector_Renderer object (unused).
 * @param pd The private data of the Ector_Renderer.
 * @return The calculated CRC value.
 */
static unsigned int
_ector_renderer_crc_get(const Eo *obj EINA_UNUSED,
                        Ector_Renderer_Data *pd)
{
   unsigned int crc;

   crc = eina_crc((void*) &pd->color, sizeof(pd->color), 0xffffffff, EINA_TRUE);
   crc = eina_crc((void*) &pd->origin, sizeof(pd->origin), crc, EINA_FALSE);

   if (pd->m) crc = eina_crc((void*) pd->m, sizeof(Eina_Matrix3), crc, EINA_FALSE);

   return crc;
}

/**
 * @internal
 * @brief Sets the composition method for vector graphics.
 *
 * @note This function is currently a no-op (does nothing).
 *
 * @param obj The Ector_Renderer object (unused).
 * @param pd The private data of the Ector_Renderer (unused).
 * @param comp The Ector_Buffer to be used for composition (unused).
 * @param method The Efl_Gfx_Vg_Composite_Method to apply (unused).
 */
static void
_ector_renderer_comp_method_set(Eo *obj EINA_UNUSED,
                                Ector_Renderer_Data *pd EINA_UNUSED,
                                Ector_Buffer *comp EINA_UNUSED,
                                Efl_Gfx_Vg_Composite_Method method EINA_UNUSED)
{
}

#include "ector_renderer.eo.c"
