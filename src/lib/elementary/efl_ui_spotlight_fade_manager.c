/**
 * @file
 * @brief This file implements the Efl_Ui_Spotlight_Fade_Manager class,
 *        which provides a fade animation for spotlight transitions.
 */

#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include <Efl_Ui.h>
#include "elm_priv.h"

/**
 * @brief Private data for the Efl_Ui_Spotlight_Fade_Manager class.
 *
 * This structure holds any private data specific to instances of
 * Efl_Ui_Spotlight_Fade_Manager. Currently, it is empty as this
 * manager primarily configures animations on its parent.
 */
typedef struct {

} Efl_Ui_Spotlight_Fade_Manager_Data;

/**
 * @brief Constructs a new Efl_Ui_Spotlight_Fade_Manager object.
 *
 * This function is called when a new instance of Efl_Ui_Spotlight_Fade_Manager
 * is created. It initializes the object and sets up a default fade animation
 * (alpha from 0.0 to 1.0 over 0.5 seconds) to be used for spotlight
 * transitions. This animation is configured for both "jump_in" and "jump_out"
 * scenarios.
 *
 * @param obj The Efl_Ui_Spotlight_Fade_Manager object to construct.
 * @param pd Pointer to the private data structure for this object.
 * @return The constructed Efl_Object.
 */
EOLIAN static Efl_Object*
_efl_ui_spotlight_fade_manager_efl_object_constructor(Eo *obj, Efl_Ui_Spotlight_Fade_Manager_Data *pd EINA_UNUSED)
{
   Efl_Canvas_Animation *animation;

   obj = efl_constructor(efl_super(obj, EFL_UI_SPOTLIGHT_FADE_MANAGER_CLASS));

   animation = efl_add(EFL_CANVAS_ALPHA_ANIMATION_CLASS, obj);
   efl_animation_alpha_set(animation, 0.0, 1.0);
   efl_animation_duration_set(animation, 0.5);

   efl_ui_spotlight_manager_animation_jump_setup_set(obj, animation, animation);
   efl_unref(animation);

   return obj;
}


#include "efl_ui_spotlight_fade_manager.eo.c"
