/**
 * @file
 * @brief This file implements the Efl.Access.EditableText interface.
 *
 * It provides the C-side implementation for accessibility features
 * related to editable text components.
 */

#ifdef HAVE_CONFIG_H
  #include "elementary_config.h"
#endif

/**
 * @def EFL_ACCESS_EDITABLE_TEXT_PROTECTED
 * @brief Enables access to protected members of the Efl_Access_Editable_Text class.
 *
 * This macro is defined to allow the C implementation file to access
 * protected APIs of its corresponding Eo class.
 */
#define EFL_ACCESS_EDITABLE_TEXT_PROTECTED

#include "elm_priv.h"

#include "efl_access_editable_text.eo.c"
