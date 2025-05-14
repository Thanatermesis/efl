#include "efl_canvas_surface.h"

#define MY_CLASS EFL_CANVAS_SURFACE_X11_CLASS

/**
 * @brief Private data for the Efl_Canvas_Surface_X11 class.
 *
 * This structure holds the X11 specific pixmap data associated with the canvas surface.
 */
typedef struct _Efl_Canvas_Surface_X11_Data
{
   Efl_Canvas_Surface_X11_Pixmap px; /**< The X11 pixmap and visual information. */
} Efl_Canvas_Surface_X11_Data;

/**
 * @brief Constructor for the Efl_Canvas_Surface_X11 object.
 *
 * Initializes the X11 native surface capabilities.
 *
 * @param eo The Efl_Canvas_Surface_X11 object.
 * @param pd Private data for the Efl_Canvas_Surface_X11 object.
 * @return The constructed Efl_Canvas_Surface_X11 object, or NULL on failure.
 */
EOLIAN static Eo *
_efl_canvas_surface_x11_efl_object_constructor(Eo *eo, Efl_Canvas_Surface_X11_Data *pd EINA_UNUSED)
{
   Evas_Object_Protected_Data *obj;
   Efl_Canvas_Surface_Data *sd;

   eo = efl_constructor(efl_super(eo, MY_CLASS));
   obj = efl_data_scope_get(eo, EFL_CANVAS_OBJECT_CLASS);
   if (!obj) return NULL;

   if (!ENFN->image_native_init(ENC, EVAS_NATIVE_SURFACE_X11))
     {
        ERR("X11 is not supported on this platform");
        return NULL;
     }

   sd = efl_data_scope_get(eo, EFL_CANVAS_SURFACE_CLASS);
   sd->surf.version = EVAS_NATIVE_SURFACE_VERSION;
   sd->surf.type = EVAS_NATIVE_SURFACE_X11;
   return eo;
}

/**
 * @brief Destructor for the Efl_Canvas_Surface_X11 object.
 *
 * Shuts down the X11 native surface capabilities.
 *
 * @param eo The Efl_Canvas_Surface_X11 object.
 * @param pd Private data for the Efl_Canvas_Surface_X11 object.
 */
EOLIAN static void
_efl_canvas_surface_x11_efl_object_destructor(Eo *eo, Efl_Canvas_Surface_X11_Data *pd EINA_UNUSED)
{
   Evas_Object_Protected_Data *obj;

   obj = efl_data_scope_get(eo, EFL_CANVAS_OBJECT_CLASS);

   ENFN->image_native_shutdown(ENC, EVAS_NATIVE_SURFACE_X11);
   efl_destructor(eo);
}

/**
 * @brief Sets the X11 pixmap for the canvas surface.
 *
 * @param eo The Efl_Canvas_Surface_X11 object.
 * @param pd Private data for the Efl_Canvas_Surface_X11 object.
 * @param visual Pointer to the X11 visual.
 * @param pixmap The X11 pixmap ID.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_canvas_surface_x11_pixmap_set(Eo *eo, Efl_Canvas_Surface_X11_Data *pd EINA_UNUSED, void *visual, unsigned long pixmap)
{
   Efl_Canvas_Surface_Data *sd = efl_data_scope_get(eo, EFL_CANVAS_SURFACE_CLASS);

   pd->px.pixmap = pixmap;
   pd->px.visual = visual;
   if (!_evas_image_native_surface_set(eo, &sd->surf))
     {
        ERR("failed to set native buffer");
        sd->buffer = NULL;
        return EINA_FALSE;
     }
   sd->buffer = &pd->px;
   return EINA_TRUE;
}

/**
 * @brief Gets the X11 pixmap from the canvas surface.
 *
 * @param eo The Efl_Canvas_Surface_X11 object.
 * @param pd Private data for the Efl_Canvas_Surface_X11 object.
 * @param visual Pointer to store the X11 visual. Can be NULL.
 * @param pixmap Pointer to store the X11 pixmap ID. Can be NULL.
 */
EOLIAN static void
_efl_canvas_surface_x11_pixmap_get(const Eo *eo EINA_UNUSED, Efl_Canvas_Surface_X11_Data *pd, void **visual, unsigned long *pixmap)
{
   if (pixmap) *pixmap = pd->px.pixmap;
   if (visual) *visual = pd->px.visual;
}

/**
 * @brief Sets the native X11 buffer for the canvas surface.
 *
 * This function allows setting the X11 pixmap and visual directly using an
 * Efl_Canvas_Surface_X11_Pixmap structure.
 *
 * @param eo The Efl_Canvas_Surface_X11 object.
 * @param pd Private data for the Efl_Canvas_Surface_X11 object.
 * @param buffer A pointer to an Efl_Canvas_Surface_X11_Pixmap structure containing
 *               the pixmap and visual information. If NULL, the current buffer is cleared.
 *               Example:
 *               Efl_Canvas_Surface_X11_Pixmap px_data;
 *               px_data.pixmap = my_x11_pixmap_id;
 *               px_data.visual = my_x11_visual_ptr;
 *               // ... set buffer with &px_data
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_canvas_surface_x11_efl_canvas_surface_native_buffer_set(Eo *eo, Efl_Canvas_Surface_X11_Data *pd, void *buffer)
{
   Efl_Canvas_Surface_X11_Pixmap *px = buffer;
   Efl_Canvas_Surface_Data *sd = efl_data_scope_get(eo, EFL_CANVAS_SURFACE_CLASS);

   if (px)
     {
        pd->px.pixmap = px->pixmap;
        pd->px.visual = px->visual;
        sd->buffer = &pd->px;
     }
   else
     {
        pd->px.pixmap = 0L;
        pd->px.visual = NULL;
        sd->buffer = NULL;
     }
   return efl_canvas_surface_x11_pixmap_set(eo, pd->px.visual, pd->px.pixmap);
}

#include "efl_canvas_surface_x11.eo.c"
