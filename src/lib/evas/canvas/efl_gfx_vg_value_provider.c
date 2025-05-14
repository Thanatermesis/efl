#include "efl_gfx_vg_value_provider.h"

#define MY_CLASS EFL_GFX_VG_VALUE_PROVIDER_CLASS

/**
 * @brief Constructor for the Efl_Gfx_Vg_Value_Provider object.
 * @param obj The Eolian object to construct.
 * @param pd Private data for the object.
 * @return The constructed Eolian object.
 */
EOLIAN static Eo *
_efl_gfx_vg_value_provider_efl_object_constructor(Eo *obj,
                                           Efl_Gfx_Vg_Value_Provider_Data *pd EINA_UNUSED)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));

   pd->flag = EFL_GFX_VG_VALUE_PROVIDER_FLAGS_NONE;

   return obj;
}

/**
 * @brief Destructor for the Efl_Gfx_Vg_Value_Provider object.
 * @param obj The Eolian object to destruct.
 * @param pd Private data for the object.
 */
EOLIAN static void
_efl_gfx_vg_value_provider_efl_object_destructor(Eo *obj,
                                          Efl_Gfx_Vg_Value_Provider_Data *pd EINA_UNUSED)
{
   if (pd->keypath) eina_stringshare_del(pd->keypath);
   efl_destructor(efl_super(obj, MY_CLASS));
}

/**
 * @brief Sets the keypath for the value provider.
 *
 * The keypath is a string that identifies the target element or property
 * to which the provided values (color, transform, etc.) will apply.
 * For example, "layername.elementname" or "group.shape".
 *
 * @param obj The Eolian object.
 * @param pd Private data for the object.
 * @param keypath The keypath string. e.g., "shape_1.fill".
 */
EOLIAN void
_efl_gfx_vg_value_provider_keypath_set(Eo *obj EINA_UNUSED, Efl_Gfx_Vg_Value_Provider_Data *pd, Eina_Stringshare *keypath)
{
   if(!keypath) return;
   eina_stringshare_replace(&pd->keypath, keypath);
}

/**
 * @brief Gets the keypath for the value provider.
 * @param obj The Eolian object.
 * @param pd Private data for the object.
 * @return The current keypath string. e.g., "shape_1.fill".
 */
EOLIAN Eina_Stringshare*
_efl_gfx_vg_value_provider_keypath_get(const Eo *obj EINA_UNUSED, Efl_Gfx_Vg_Value_Provider_Data *pd)
{
   return pd->keypath;
}

/**
 * @brief Sets the transformation matrix for the value provider.
 *
 * If a matrix is provided, it will be copied and stored.
 * If NULL is provided, any existing matrix will be freed and reset.
 * This also sets the #EFL_GFX_VG_VALUE_PROVIDER_FLAGS_TRANSFORM_MATRIX flag.
 *
 * @param obj The Eolian object.
 * @param pd Private data for the object.
 * @param m Pointer to an Eina_Matrix4 transformation matrix, or NULL to clear.
 */
EOLIAN void
_efl_gfx_vg_value_provider_transform_set(Eo *obj EINA_UNUSED, Efl_Gfx_Vg_Value_Provider_Data *pd, Eina_Matrix4 *m)
{
   if (m)
     {
        if (!pd->m)
          {
             pd->m = malloc(sizeof (Eina_Matrix4));
             if (!pd->m) return;
          }
        pd->flag = pd->flag | EFL_GFX_VG_VALUE_PROVIDER_FLAGS_TRANSFORM_MATRIX;
        memcpy(pd->m, m, sizeof (Eina_Matrix4));
     }
   else
     {
        free(pd->m);
        pd->m = NULL;
     }
}

/**
 * @brief Gets the transformation matrix from the value provider.
 * @param obj The Eolian object.
 * @param pd Private data for the object.
 * @return Pointer to the stored Eina_Matrix4 transformation matrix, or NULL if not set.
 */
EOLIAN Eina_Matrix4*
_efl_gfx_vg_value_provider_transform_get(const Eo *obj EINA_UNUSED, Efl_Gfx_Vg_Value_Provider_Data *pd)
{
   return pd->m;
}

/**
 * @brief Sets the fill color for the value provider.
 *
 * This sets the RGBA components for the fill color and updates the
 * #EFL_GFX_VG_VALUE_PROVIDER_FLAGS_FILL_COLOR flag.
 *
 * @param obj The Eolian object.
 * @param pd Private data for the object.
 * @param r Red component (0-255).
 * @param g Green component (0-255).
 * @param b Blue component (0-255).
 * @param a Alpha component (0-255).
 */
EVAS_API EVAS_API_WEAK void
_efl_gfx_vg_value_provider_fill_color_set(Eo *obj EINA_UNUSED, Efl_Gfx_Vg_Value_Provider_Data *pd, int r, int g, int b, int a)
{
   pd->flag = pd->flag | EFL_GFX_VG_VALUE_PROVIDER_FLAGS_FILL_COLOR;

   pd->fill.r = r;
   pd->fill.g = g;
   pd->fill.b = b;
   pd->fill.a = a;
}

/**
 * @brief Gets the fill color from the value provider.
 *
 * Retrieves the RGBA components of the fill color.
 * Any of the output parameters can be NULL if that component is not needed.
 *
 * @param obj The Eolian object.
 * @param pd Private data for the object.
 * @param[out] r Pointer to store the red component.
 * @param[out] g Pointer to store the green component.
 * @param[out] b Pointer to store the blue component.
 * @param[out] a Pointer to store the alpha component.
 */
EVAS_API EVAS_API_WEAK void
_efl_gfx_vg_value_provider_fill_color_get(const Eo *obj EINA_UNUSED, Efl_Gfx_Vg_Value_Provider_Data *pd, int *r, int *g, int *b, int *a)
{
   if (r) *r = pd->fill.r;
   if (g) *g = pd->fill.g;
   if (b) *b = pd->fill.b;
   if (a) *a = pd->fill.a;
}

/**
 * @brief Sets the stroke color for the value provider.
 *
 * This sets the RGBA components for the stroke color and updates the
 * #EFL_GFX_VG_VALUE_PROVIDER_FLAGS_STROKE_COLOR flag.
 *
 * @param obj The Eolian object.
 * @param pd Private data for the object.
 * @param r Red component (0-255).
 * @param g Green component (0-255).
 * @param b Blue component (0-255).
 * @param a Alpha component (0-255).
 */
EVAS_API EVAS_API_WEAK void
_efl_gfx_vg_value_provider_stroke_color_set(Eo *obj EINA_UNUSED, Efl_Gfx_Vg_Value_Provider_Data *pd, int r, int g, int b, int a)
{
   pd->flag = pd->flag | EFL_GFX_VG_VALUE_PROVIDER_FLAGS_STROKE_COLOR;

   pd->stroke.r = r;
   pd->stroke.g = g;
   pd->stroke.b = b;
   pd->stroke.a = a;
}

/**
 * @brief Gets the stroke color from the value provider.
 *
 * Retrieves the RGBA components of the stroke color.
 * Any of the output parameters can be NULL if that component is not needed.
 *
 * @param obj The Eolian object.
 * @param pd Private data for the object.
 * @param[out] r Pointer to store the red component.
 * @param[out] g Pointer to store the green component.
 * @param[out] b Pointer to store the blue component.
 * @param[out] a Pointer to store the alpha component.
 */
EVAS_API EVAS_API_WEAK void
_efl_gfx_vg_value_provider_stroke_color_get(const Eo *obj EINA_UNUSED, Efl_Gfx_Vg_Value_Provider_Data *pd, int *r, int *g, int *b, int *a)
{
   if (r) *r = pd->stroke.r;
   if (g) *g = pd->stroke.g;
   if (b) *b = pd->stroke.b;
   if (a) *a = pd->stroke.a;
}

/**
 * @brief Sets the stroke width for the value provider.
 *
 * Sets the width for the stroke. If width is negative, the call is ignored.
 * This also updates the #EFL_GFX_VG_VALUE_PROVIDER_FLAGS_STROKE_WIDTH flag.
 *
 * @param obj The Eolian object.
 * @param pd Private data for the object.
 * @param w The stroke width. Must be non-negative.
 */
EVAS_API EVAS_API_WEAK void
_efl_gfx_vg_value_provider_stroke_width_set(Eo *obj EINA_UNUSED, Efl_Gfx_Vg_Value_Provider_Data *pd, double w)
{
  if (w < 0) return ;

  pd->flag = pd->flag | EFL_GFX_VG_VALUE_PROVIDER_FLAGS_STROKE_WIDTH;
  pd->stroke.width = w;
}

/**
 * @brief Gets the stroke width from the value provider.
 * @param obj The Eolian object.
 * @param pd Private data for the object.
 * @return The current stroke width.
 */
EVAS_API EVAS_API_WEAK double
_efl_gfx_vg_value_provider_stroke_width_get(const Eo *obj EINA_UNUSED, Efl_Gfx_Vg_Value_Provider_Data *pd)
{
   return pd->stroke.width;
}

/**
 * @brief Gets the flags indicating which properties have been updated.
 *
 * This returns a bitmask of #Efl_Gfx_Vg_Value_Provider_Flags indicating
 * which values (fill color, stroke color, transform, etc.) have been
 * explicitly set on this provider.
 *
 * @param obj The Eolian object.
 * @param pd Private data for the object.
 * @return A bitmask of flags. See #Efl_Gfx_Vg_Value_Provider_Flags.
 */
EVAS_API EVAS_API_WEAK Efl_Gfx_Vg_Value_Provider_Flags
_efl_gfx_vg_value_provider_updated_get(const Eo *obj EINA_UNUSED, Efl_Gfx_Vg_Value_Provider_Data *pd)
{
   return pd->flag;
}


#include "efl_gfx_vg_value_provider.eo.c"
