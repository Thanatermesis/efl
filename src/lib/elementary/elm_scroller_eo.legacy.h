#ifndef _ELM_SCROLLER_EO_LEGACY_H_
#define _ELM_SCROLLER_EO_LEGACY_H_

#ifndef _ELM_SCROLLER_EO_CLASS_TYPE
#define _ELM_SCROLLER_EO_CLASS_TYPE

typedef Eo Elm_Scroller;

#endif

#ifndef _ELM_SCROLLER_EO_TYPES
#define _ELM_SCROLLER_EO_TYPES


#endif

/**
 * @brief Set custom theme elements for the scroller.
 *
 * This allows for specific parts of the scroller widget to be themed
 * differently by providing custom class and group names for theme lookups.
 * For example, one might use this to apply a "compact" style or a
 * "highlighted" style to the scroller's visual elements if defined in the theme.
 *
 * @param[in] obj The object.
 * @param[in] klass Klass name, e.g., "scroller/compact".
 * @param[in] group Group name, e.g., "base".
 *
 * @ingroup Elm_Scroller_Group
 */
EAPI void elm_scroller_custom_widget_base_theme_set(Elm_Scroller *obj, const char *klass, const char *group);

/**
 * @brief Set the maximum number of pages that can be scrolled at once with a flick gesture.
 *
 * This controls how many "pages" (discrete scrollable units, often screen-sized)
 * the scroller will attempt to move when a flick gesture is performed.
 * The value of maximum movable page should be more than 1, as per original API notes.
 * For example, setting to 2 allows flicking two pages at a time.
 *
 * @param[in] obj The object.
 * @param[in] page_limit_h The maximum number of movable horizontal pages (must be > 1).
 * @param[in] page_limit_v The maximum number of movable vertical pages (must be > 1).
 *
 * @since 1.8
 *
 * @ingroup Elm_Scroller_Group
 */
EAPI void elm_scroller_page_scroll_limit_set(const Elm_Scroller *obj, int page_limit_h, int page_limit_v);

/**
 * @brief Get the maximum number of pages that can be scrolled at once with a flick gesture.
 *
 * Retrieves the limits set by @ref elm_scroller_page_scroll_limit_set.
 * These limits determine how many "pages" the scroller attempts to move
 * per flick gesture.
 *
 * @param[in] obj The object.
 * @param[out] page_limit_h Pointer to store the maximum number of movable horizontal pages.
 * @param[out] page_limit_v Pointer to store the maximum number of movable vertical pages.
 *
 * @since 1.8
 *
 * @ingroup Elm_Scroller_Group
 */
EAPI void elm_scroller_page_scroll_limit_get(const Elm_Scroller *obj, int *page_limit_h, int *page_limit_v);

#endif
