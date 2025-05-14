#include "efl_canvas_surface.h"

#define MY_CLASS EFL_CANVAS_SURFACE_WAYLAND_CLASS

/**
 * @brief Private data for the Efl_Canvas_Surface_Wayland class.
 *
 * This structure holds any private data specific to Wayland surfaces.
 * Currently, it is empty, but it is defined for future extensions.
 */
typedef struct _Efl_Canvas_Surface_Wayland_Data
{
} Efl_Canvas_Surface_Wayland_Data;

/**
 * @brief Constructor for the Efl_Canvas_Surface_Wayland object.
 *
 * This function initializes a new Wayland surface. It sets up
 * the native surface support for Wayland and ensures that the
 * underlying Evas engine can handle Wayland surfaces.
 *
 * @param eo The Efl_Object being constructed.
 * @param pd Pointer to the private data of the Efl_Canvas_Surface_Wayland instance.
 * @return The constructed Efl_Object, or NULL on failure.
 */
EOLIAN static Eo *
_efl_canvas_surface_wayland_efl_object_constructor(Eo *eo, Efl_Canvas_Surface_Wayland_Data *pd EINA_UNUSED)
{
   Evas_Object_Protected_Data *obj;
   Efl_Canvas_Surface_Data *sd;

   eo = efl_constructor(efl_super(eo, MY_CLASS));
   obj = efl_data_scope_get(eo, EFL_CANVAS_OBJECT_CLASS);
   if (!obj) return NULL;

   if (!ENFN->image_native_init(ENC, EVAS_NATIVE_SURFACE_WL))
     {
        ERR("Wayland surfaces are not supported on this platform");
        return NULL;
     }

   sd = efl_data_scope_get(eo, EFL_CANVAS_SURFACE_CLASS);
   sd->surf.type = EVAS_NATIVE_SURFACE_WL;
   return eo;
}

/**
 * @brief Destructor for the Efl_Canvas_Surface_Wayland object.
 *
 * This function cleans up resources used by the Wayland surface.
 * It shuts down the native surface support for Wayland.
 *
 * @param eo The Efl_Object being destructed.
 * @param pd Pointer to the private data of the Efl_Canvas_Surface_Wayland instance.
 */
EOLIAN static void
_efl_canvas_surface_wayland_efl_object_destructor(Eo *eo, Efl_Canvas_Surface_Wayland_Data *pd EINA_UNUSED)
{
   Evas_Object_Protected_Data *obj;

   obj = efl_data_scope_get(eo, EFL_CANVAS_OBJECT_CLASS);

   ENFN->image_native_shutdown(ENC, EVAS_NATIVE_SURFACE_WL);
   efl_destructor(eo);
}

/**
 * @brief Sets the native Wayland buffer for the surface.
 *
 * This function allows an external Wayland buffer (e.g., a wl_buffer)
 * to be used as the pixel data source for this Evas image object.
 * The provided buffer will be directly used by Evas for rendering.
 *
 * @param eo The Efl_Canvas_Surface object.
 * @param pd Pointer to the private data of the Efl_Canvas_Surface_Wayland instance.
 * @param buffer A pointer to the native Wayland buffer.
 *               For example, this could be a `struct wl_buffer *`.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
EOLIAN static Eina_Bool
_efl_canvas_surface_wayland_efl_canvas_surface_native_buffer_set(Eo *eo, Efl_Canvas_Surface_Wayland_Data *pd EINA_UNUSED, void *buffer)
{
   Efl_Canvas_Surface_Data *sd = efl_data_scope_get(eo, EFL_CANVAS_SURFACE_CLASS);

   sd->surf.data.wl.legacy_buffer = buffer;
   if (!_evas_image_native_surface_set(eo, &sd->surf))
     {
        ERR("failed to set native buffer");
        sd->buffer = NULL;
        return EINA_FALSE;
     }
   sd->buffer = buffer;
   return EINA_TRUE;
}

#include "efl_canvas_surface_wayland.eo.c"
