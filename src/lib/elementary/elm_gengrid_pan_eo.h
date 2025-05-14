/**
 * @file
 * @brief These routines are routines to manipulate Gengrid Pan objects.
 *
 * Elm_Gengrid_Pan is a Gengrid Pan.
 * It is used for Gengrid widget.
 *
 * @ingroup Elm_Gengrid_Pan
 */

#ifndef _ELM_GENGRID_PAN_EO_H_
#define _ELM_GENGRID_PAN_EO_H_

#ifndef _ELM_GENGRID_PAN_EO_CLASS_TYPE
#define _ELM_GENGRID_PAN_EO_CLASS_TYPE

/**
 * @brief Opaque handle to the Gengrid Pan object.
 * @ingroup Elm_Gengrid_Pan
 */
typedef Eo Elm_Gengrid_Pan;

#endif

#ifndef _ELM_GENGRID_PAN_EO_TYPES
#define _ELM_GENGRID_PAN_EO_TYPES


#endif
/**
 * @brief Elementary gengrid pan class.
 *
 * @ingroup Elm_Gengrid_Pan
 */
#define ELM_GENGRID_PAN_CLASS elm_gengrid_pan_class_get()

/**
 * @brief Get the Efl_Class for the Elm_Gengrid_Pan.
 *
 * @return The Efl_Class for the Elm_Gengrid_Pan.
 * @ingroup Elm_Gengrid_Pan
 */
EWAPI const Efl_Class *elm_gengrid_pan_class_get(void) EINA_CONST;

#endif
