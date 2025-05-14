#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define EFL_UI_WIN_PROTECTED
#define EFL_UI_WIN_INLINED_PROTECTED

#include <Elementary.h>

#include "elm_priv.h"
#include "efl_ui_win_inlined_legacy_eo.h"

#define MY_CLASS EFL_UI_WIN_INLINED_CLASS
#define MY_CLASS_NAME "Efl.Ui.Win_Inlined"
#define MY_CLASS_NAME_LEGACY "elm_win"

/**
 * @brief Private data structure for Efl.Ui.Win_Inlined objects.
 *
 * This structure holds data specific to inlined windows, primarily
 * tracking the parent Efl_Canvas_Object.
 */
typedef struct
{
   Eo *parent; /**< This is the parent specific to the inlined window, aka parent2 */
} Efl_Ui_Win_Inlined_Data;

/**
 * @brief Sets the parent object for an inlined window.
 *
 * This function is used to establish the parent-child relationship for an
 * inlined window. The parent is an Efl_Canvas_Object within which the
 * inlined window will be rendered.
 *
 * @param obj The Efl.Ui.Win_Inlined object.
 * @param parent The Efl_Canvas_Object to set as the parent.
 */
void
efl_ui_win_inlined_parent_set(Eo *obj, Efl_Canvas_Object *parent)
{
   Efl_Ui_Win_Inlined_Data *pd = efl_data_scope_safe_get(obj, MY_CLASS);
   if (!pd) return;
   pd->parent = parent;
}

/**
 * @internal
 * @brief Gets the parent object of an inlined window.
 * @eo_description This function retrieves the Efl_Canvas_Object that acts as the parent
 * for this inlined window.
 *
 * @param obj The Efl.Ui.Win_Inlined object (unused).
 * @param pd Pointer to the private data of the Efl.Ui.Win_Inlined object.
 * @return The parent Efl_Canvas_Object, or NULL if not set.
 */
EOLIAN static Efl_Canvas_Object *
_efl_ui_win_inlined_inlined_parent_get(const Eo *obj EINA_UNUSED, Efl_Ui_Win_Inlined_Data *pd)
{
   return pd->parent;
}

/**
 * @internal
 * @brief Finalizes the Efl.Ui.Win_Inlined object.
 * @eo_description This function is called during the finalization phase of the object's
 * lifecycle. It sets the window type to EFL_UI_WIN_TYPE_INLINED_IMAGE
 * and then calls the parent class's finalize method.
 *
 * @param obj The Efl.Ui.Win_Inlined object to finalize.
 * @param pd Pointer to the private data of the Efl.Ui.Win_Inlined object (unused).
 * @return The finalized Efl_Object.
 */
EOLIAN static Efl_Object *
_efl_ui_win_inlined_efl_object_finalize(Eo *obj, Efl_Ui_Win_Inlined_Data *pd EINA_UNUSED)
{
   efl_ui_win_type_set(obj, EFL_UI_WIN_TYPE_INLINED_IMAGE);
   obj = efl_finalize(efl_super(obj, MY_CLASS));

   return obj;
}

#include "efl_ui_win_inlined.eo.c"

/**
 * @internal
 * @brief Legacy class constructor for Efl.Ui.Win_Inlined.
 * @eo_description This function is called when the Efl.Ui.Win_Inlined_Legacy class
 * is constructed. It registers the legacy type name "elm_win"
 * for this class, enabling compatibility with older code.
 *
 * @param klass The Efl_Class being constructed.
 */
static void
_efl_ui_win_inlined_legacy_class_constructor(Efl_Class *klass)
{
   evas_smart_legacy_type_register(MY_CLASS_NAME_LEGACY, klass);
}

/**
 * @internal
 * @brief Finalizes the legacy Efl.Ui.Win_Inlined object.
 * @eo_description This function is called during the finalization of a legacy
 * Efl.Ui.Win_Inlined object. It calls the superclass's finalize
 * method and then sets the Efl_Canvas_Object type to the legacy
 * class name "elm_win".
 *
 * @param obj The legacy Efl.Ui.Win_Inlined object to finalize.
 * @param pd Pointer to the private data (unused).
 * @return The finalized Eo object.
 */
EOLIAN static Eo *
_efl_ui_win_inlined_legacy_efl_object_finalize(Eo *obj, void *pd EINA_UNUSED)
{
   obj = efl_finalize(efl_super(obj, EFL_UI_WIN_INLINED_LEGACY_CLASS));
   efl_canvas_object_type_set(obj, MY_CLASS_NAME_LEGACY);
   return obj;
}

#include "efl_ui_win_inlined_legacy_eo.c"
