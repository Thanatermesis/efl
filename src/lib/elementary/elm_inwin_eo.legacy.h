#ifndef _ELM_INWIN_EO_LEGACY_H_
#define _ELM_INWIN_EO_LEGACY_H_

#ifndef _ELM_INWIN_EO_CLASS_TYPE
#define _ELM_INWIN_EO_CLASS_TYPE

/**
 * @typedef Elm_Inwin
 * @brief Represents an Elementary Inwin object.
 *
 * This type is an alias for an Eo object, specifically used for
 * in-window (Inwin) UI components within the Elementary toolkit.
 */
typedef Eo Elm_Inwin;

#endif

#ifndef _ELM_INWIN_EO_TYPES
#define _ELM_INWIN_EO_TYPES

/**
 * @file
 * @brief This section can be used to define specific types related to Elm_Inwin.
 *
 * Currently, there are no specific types defined here, but this block
 * is reserved for future extensions if Elm_Inwin requires custom data types.
 */

#endif

/**
 * @brief Activates an inwin object, ensuring its visibility
 *
 * This function will make sure that the inwin @c obj is completely visible by
 * calling evas_object_show() and evas_object_raise() on it, to bring it to the
 * front. It also sets the keyboard focus to it, which will be passed onto its
 * content.
 *
 * The object's theme will also receive the signal "elm,action,show" with
 * source "elm".
 * @param[in] obj The object.
 *
 * @ingroup Elm_Inwin_Group
 */
EAPI void elm_win_inwin_activate(Elm_Inwin *obj);

#endif
