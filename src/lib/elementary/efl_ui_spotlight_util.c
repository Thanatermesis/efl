#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include <Efl_Ui.h>
#include "elm_priv.h"

/**
 * @internal
 * @brief Private data for the Efl_Ui_Spotlight_Util class.
 *
 * This structure currently does not hold any data but is defined for
 * future extensions and to maintain consistency with EFL's coding style.
 */
typedef struct {

} Efl_Ui_Spotlight_Util_Data;

/**
 * @internal
 * @brief Generates a new spotlight container with a fade manager.
 *
 * This function creates a new EFL_UI_SPOTLIGHT_CONTAINER_CLASS instance
 * and associates it with a new EFL_UI_SPOTLIGHT_FADE_MANAGER_CLASS instance.
 * The generated container is intended to be a child of the provided @p parent widget.
 *
 * @param parent The parent widget for the new spotlight container.
 * @return A new Efl_Ui_Spotlight_Container instance, or @c NULL on failure.
 *         The returned container will have a fade animation manager set.
 */
EOLIAN static Efl_Ui_Spotlight_Container*
_efl_ui_spotlight_util_stack_gen(Efl_Ui_Widget *parent)
{
   Efl_Ui_Spotlight_Manager *manager = efl_new(EFL_UI_SPOTLIGHT_FADE_MANAGER_CLASS);
   return efl_add(EFL_UI_SPOTLIGHT_CONTAINER_CLASS, parent,
                  efl_ui_spotlight_manager_set(efl_added, manager));
}


#include "efl_ui_spotlight_util.eo.c"
