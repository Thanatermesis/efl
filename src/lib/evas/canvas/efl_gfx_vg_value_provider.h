#ifndef EFL_GFX_VG_VALUE_PROVIDER_H
#define EFL_GFX_VG_VALUE_PROVIDER_H

#include "evas_common_private.h"
#include "evas_private.h"
#include "evas_vg_private.h"

/**
 * @brief Private data structure for the Efl_Gfx_Vg_Value_Provider class.
 *
 * This structure holds all the data associated with an instance of
 * Efl_Gfx_Vg_Value_Provider, including flags indicating which properties
 * have been updated, keypath for identifying the target, transformation matrix,
 * and color/width information for fill and stroke.
 */
struct _Efl_Gfx_Vg_Value_Provider_Data
{
   Eo* obj; /**< The Eolian object instance. */
   Efl_Gfx_Vg_Value_Provider_Flags flag; /**< Flags indicating which properties have been set/updated. See #Efl_Gfx_Vg_Value_Provider_Flags for possible values. */

   Eina_Stringshare *keypath; /**< A string identifying the target element or property path. e.g., "layername.elementname" */

   Eina_Matrix4 *m; /**< Transformation matrix to be applied. This is NULL if no transform is set. */
   /**
    * @brief Fill color properties.
    */
   struct {
      int r; /**< Red component of the fill color (0-255). */
      int g; /**< Green component of the fill color (0-255). */
      int b; /**< Blue component of the fill color (0-255). */
      int a; /**< Alpha component of the fill color (0-255). */
   } fill; /**< Structure holding the fill color components. */
   /**
    * @brief Stroke properties.
    */
   struct {
      int r; /**< Red component of the stroke color (0-255). */
      int g; /**< Green component of the stroke color (0-255). */
      int b; /**< Blue component of the stroke color (0-255). */
      int a; /**< Alpha component of the stroke color (0-255). */
      double width; /**< Width of the stroke. */
   } stroke; /**< Structure holding the stroke color components and width. */
};

/**
 * @brief Typedef for the private data structure of Efl_Gfx_Vg_Value_Provider.
 */
typedef struct _Efl_Gfx_Vg_Value_Provider_Data Efl_Gfx_Vg_Value_Provider_Data;

#endif
