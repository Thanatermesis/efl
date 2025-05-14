/**
 * @brief Sets the map widget for the route object.
 *
 * This function is the C-callable EAPI implementation for setting the map widget.
 * For detailed documentation, see elm_route_emap_set() in elm_route_eo.legacy.h.
 */
EAPI void
elm_route_emap_set(Elm_Route *obj, void *emap)
{
   elm_obj_route_emap_set(obj, emap);
}

/**
 * @brief Retrieves the minimum and maximum longitude values for the route.
 *
 * This function is the C-callable EAPI implementation for getting longitude bounds.
 * For detailed documentation, see elm_route_longitude_min_max_get() in elm_route_eo.legacy.h.
 */
EAPI void
elm_route_longitude_min_max_get(const Elm_Route *obj, double *min, double *max)
{
   elm_obj_route_longitude_min_max_get(obj, min, max);
}

/**
 * @brief Retrieves the minimum and maximum latitude values for the route.
 *
 * This function is the C-callable EAPI implementation for getting latitude bounds.
 * For detailed documentation, see elm_route_latitude_min_max_get() in elm_route_eo.legacy.h.
 */
EAPI void
elm_route_latitude_min_max_get(const Elm_Route *obj, double *min, double *max)
{
   elm_obj_route_latitude_min_max_get(obj, min, max);
}
