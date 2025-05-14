/**
 * @file
 * @brief This file implements the Efl.Ui.Layout.Orientable_Readonly interface.
 *
 * This interface is intended for layouts that have a fixed orientation
 * which cannot be changed at runtime.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Efl.h>

/**
 * @brief Private data for the Efl_Ui_Layout_Orientable_Readonly class.
 *
 * This structure currently holds no data but is defined for future use
 * and consistency with EFL coding conventions.
 */
typedef struct {

} Efl_Ui_Layout_Orientable_Readonly_Data;

/**
 * @internal
 * @brief Implements efl_ui_layout_orientable_orientation_set for readonly orientable objects.
 *
 * This function is called when an attempt is made to set the orientation
 * of an object that implements the Efl.Ui.Layout.Orientable_Readonly interface.
 * Since the orientation is readonly, this function logs an error message
 * indicating that the operation is not permitted.
 *
 * @param[in] obj The Efl.Ui.Layout.Orientable object.
 * @param[in] pd The private data for the Efl_Ui_Layout_Orientable_Readonly class.
 * @param[in] dir The desired orientation (ignored, as orientation cannot be set).
 */
EOLIAN static void
_efl_ui_layout_orientable_readonly_efl_ui_layout_orientable_orientation_set(Eo *obj EINA_UNUSED, Efl_Ui_Layout_Orientable_Readonly_Data *pd EINA_UNUSED, Efl_Ui_Layout_Orientation dir EINA_UNUSED)
{
   EINA_LOG_ERR("This object does not allow setting an orientation");
}

#include "interfaces/efl_ui_layout_orientable_readonly.eo.c"
