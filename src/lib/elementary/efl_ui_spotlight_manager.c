/**
 * @file
 * @brief This file implements the Efl_Ui_Spotlight_Manager class.
 *
 * The spotlight manager is responsible for managing spotlightable widgets
 * within a given container.
 */

#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define EFL_PACK_LAYOUT_PROTECTED

#include <Elementary.h>
#include "elm_priv.h"

/**
 * @brief Private data structure for the Efl_Ui_Spotlight_Manager class.
 *
 * This structure holds all private data members for an instance of
 * Efl_Ui_Spotlight_Manager.
 * @since 1.24
 */
typedef struct {

} Efl_Ui_Spotlight_Manager_Data;


#include "efl_ui_spotlight_manager.eo.c"
