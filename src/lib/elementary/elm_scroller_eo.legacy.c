/**
 * @brief Sets custom theme elements for the scroller (legacy API).
 *
 * This function is the C implementation for the legacy API call
 * elm_scroller_custom_widget_base_theme_set(). It forwards the call to
 * the underlying Eo-based implementation `elm_obj_scroller_custom_widget_base_theme_set()`.
 * Refer to the header file (elm_scroller_eo.legacy.h) for detailed parameter descriptions
 * and usage.
 */
EAPI void
elm_scroller_custom_widget_base_theme_set(Elm_Scroller *obj, const char *klass, const char *group)
{
   elm_obj_scroller_custom_widget_base_theme_set(obj, klass, group);
}

/**
 * @brief Sets the maximum page scroll limit (legacy API).
 *
 * This function is the C implementation for the legacy API call
 * elm_scroller_page_scroll_limit_set(). It forwards the call to
 * the underlying Eo-based implementation `elm_obj_scroller_page_scroll_limit_set()`.
 * Refer to the header file (elm_scroller_eo.legacy.h) for detailed parameter descriptions
 * and usage.
 * @since 1.8
 */
EAPI void
elm_scroller_page_scroll_limit_set(const Elm_Scroller *obj, int page_limit_h, int page_limit_v)
{
   elm_obj_scroller_page_scroll_limit_set(obj, page_limit_h, page_limit_v);
}

/**
 * @brief Gets the maximum page scroll limit (legacy API).
 *
 * This function is the C implementation for the legacy API call
 * elm_scroller_page_scroll_limit_get(). It forwards the call to
 * the underlying Eo-based implementation `elm_obj_scroller_page_scroll_limit_get()`.
 * Refer to the header file (elm_scroller_eo.legacy.h) for detailed parameter descriptions
 * and usage.
 * @since 1.8
 */
EAPI void
elm_scroller_page_scroll_limit_get(const Elm_Scroller *obj, int *page_limit_h, int *page_limit_v)
{
   elm_obj_scroller_page_scroll_limit_get(obj, page_limit_h, page_limit_v);
}
