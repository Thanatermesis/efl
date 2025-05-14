#include "efl_canvas_surface.h"

#define MY_CLASS EFL_CANVAS_SURFACE_TBM_CLASS

/**
 * @brief Private data for the Efl_Canvas_Surface_Tbm class.
 *
 * This structure holds any private data specific to the TBM surface implementation.
 * Currently, it is empty, but it is defined for future extensions.
 */
typedef struct _Efl_Canvas_Surface_Tbm_Data
{
} Efl_Canvas_Surface_Tbm_Data;

/**
 * @brief Constructor for the Efl_Canvas_Surface_Tbm object.
 *
 * This function is called when a new Efl_Canvas_Surface_Tbm object is created.
 * It initializes the TBM native surface and sets up the necessary Evas internal
 * structures.
 *
 * @param eo The Efl_Canvas_Surface_Tbm object being constructed.
 * @param pd Pointer to the private data of the object.
 * @return The newly constructed Eo object, or NULL on failure.
 */
EOLIAN static Eo *
_efl_canvas_surface_tbm_efl_object_constructor(Eo *eo, Efl_Canvas_Surface_Tbm_Data *pd EINA_UNUSED)
{
   Evas_Object_Protected_Data *obj;
   Efl_Canvas_Surface_Data *sd;

   eo = efl_constructor(efl_super(eo, MY_CLASS));
   obj = efl_data_scope_get(eo, EFL_CANVAS_OBJECT_CLASS);
   if (!obj) return NULL;

   if (!ENFN->image_native_init(ENC, EVAS_NATIVE_SURFACE_TBM))
     {
        ERR("TBM is not supported on this platform");
        return NULL;
     }

   sd = efl_data_scope_get(eo, EFL_CANVAS_SURFACE_CLASS);
   sd->surf.type = EVAS_NATIVE_SURFACE_TBM;
   return eo;
}

/**
 * @brief Destructor for the Efl_Canvas_Surface_Tbm object.
 *
 * This function is called when an Efl_Canvas_Surface_Tbm object is being destroyed.
 * It cleans up resources associated with the TBM native surface.
 *
 * @param eo The Efl_Canvas_Surface_Tbm object being destructed.
 * @param pd Pointer to the private data of the object.
 */
EOLIAN static void
_efl_canvas_surface_tbm_efl_object_destructor(Eo *eo, Efl_Canvas_Surface_Tbm_Data *pd EINA_UNUSED)
{
   Evas_Object_Protected_Data *obj;

   obj = efl_data_scope_get(eo, EFL_CANVAS_OBJECT_CLASS);

   ENFN->image_native_shutdown(ENC, EVAS_NATIVE_SURFACE_TBM);
   efl_destructor(eo);
}

/**
 * @brief Sets the native TBM buffer for the surface.
 *
 * This function allows associating an externally allocated TBM buffer
 * with the Efl_Canvas_Surface_Tbm object. The rendering engine will then
 * use this buffer as its drawing target.
 *
 * @param eo The Efl_Canvas_Surface_Tbm object.
 * @param pd Pointer to the private data of the object.
 * @param buffer A pointer to the native TBM buffer (e.g., tbm_surface_h).
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
EOLIAN static Eina_Bool
_efl_canvas_surface_tbm_efl_canvas_surface_native_buffer_set(Eo *eo, Efl_Canvas_Surface_Tbm_Data *pd EINA_UNUSED, void *buffer)
{
   Efl_Canvas_Surface_Data *sd = efl_data_scope_get(eo, EFL_CANVAS_SURFACE_CLASS);

   sd->surf.data.tbm.buffer = buffer;
   if (!_evas_image_native_surface_set(eo, &sd->surf))
     {
        ERR("failed to set native buffer");
        sd->buffer = NULL;
        return EINA_FALSE;
     }
   sd->buffer = buffer;
   return EINA_TRUE;
}

#include "efl_canvas_surface_tbm.eo.c"
