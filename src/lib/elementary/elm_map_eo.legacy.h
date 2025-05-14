#ifndef _ELM_MAP_EO_LEGACY_H_
#define _ELM_MAP_EO_LEGACY_H_

#ifndef _ELM_MAP_EO_CLASS_TYPE
#define _ELM_MAP_EO_CLASS_TYPE

typedef Eo Elm_Map;

#endif

#ifndef _ELM_MAP_EO_TYPES
#define _ELM_MAP_EO_TYPES

/**
 * @brief Defines the types of sources that can be used with the map.
 *
 * These types are used to specify the kind of service provider (e.g., for tiles,
 * routing, or geocoding).
 *
 * @ingroup Elm_Map_Group
 */
typedef enum
{
  ELM_MAP_SOURCE_TYPE_TILE = 0, /**< Map tile provider. */
  ELM_MAP_SOURCE_TYPE_ROUTE, /**< Route service provider. */
  ELM_MAP_SOURCE_TYPE_NAME, /**< Name service provider. */
  ELM_MAP_SOURCE_TYPE_LAST /**< Sentinel value to indicate last enum field
                            * during iteration */
} Elm_Map_Source_Type;

/**
 * @brief Defines the types of routes that can be calculated.
 *
 * This enum specifies the mode of transport for route calculation.
 *
 * @ingroup Elm_Map_Group
 */
typedef enum
{
  ELM_MAP_ROUTE_TYPE_MOTOCAR = 0, /**< Route should consider an automobile will
                                   * be used. */
  ELM_MAP_ROUTE_TYPE_BICYCLE, /**< Route should consider a bicycle will be used
                               * by the user. */
  ELM_MAP_ROUTE_TYPE_FOOT, /**< Route should consider user will be walking. */
  ELM_MAP_ROUTE_TYPE_LAST /**< Sentinel value to indicate last enum field during
                           * iteration */
} Elm_Map_Route_Type;

/**
 * @brief Defines the methods for calculating routes.
 *
 * This enum specifies the optimization preference for route calculation,
 * such as the fastest or shortest path.
 *
 * @ingroup Elm_Map_Group
 */
typedef enum
{
  ELM_MAP_ROUTE_METHOD_FASTEST = 0, /**< Route should prioritize time. */
  ELM_MAP_ROUTE_METHOD_SHORTEST, /**< Route should prioritize distance. */
  ELM_MAP_ROUTE_METHOD_LAST /**< Sentinel value to indicate last enum field
                             * during iteration */
} Elm_Map_Route_Method;


#endif

/**
 * @brief Set the minimum zoom level of the map.
 *
 * This function defines the lowest zoom level the user can zoom out to.
 *
 * @param[in] obj The map object.
 * @param[in] zoom The minimum zoom level to set. For example, 0 or 1.
 *
 * @ingroup Elm_Map_Group
 */
EAPI void elm_map_zoom_min_set(Elm_Map *obj, int zoom);

/**
 * @brief Get the minimum zoom level of the map.
 *
 * This function retrieves the lowest zoom level the user can zoom out to.
 *
 * @param[in] obj The map object.
 * @return The minimum zoom level.
 *
 * @ingroup Elm_Map_Group
 */
EAPI int elm_map_zoom_min_get(const Elm_Map *obj);

/**
 * @brief Set the rotation of the map.
 *
 * Rotates the map view by a specified degree around a center point (cx, cy)
 * in canvas coordinates.
 *
 * @param[in] obj The map object.
 * @param[in] degree The rotation angle in degrees. Positive values rotate
 *            clockwise.
 * @param[in] cx The x-coordinate of the rotation center in canvas coordinates.
 * @param[in] cy The y-coordinate of the rotation center in canvas coordinates.
 *
 * @ingroup Elm_Map_Group
 */
EAPI void elm_map_rotate_set(Elm_Map *obj, double degree, int cx, int cy);

/**
 * @brief Get the current rotation of the map.
 *
 * Retrieves the map's current rotation angle and the center of rotation.
 *
 * @param[in] obj The map object.
 * @param[out] degree Pointer to a double where the rotation angle in degrees
 *             will be stored.
 * @param[out] cx Pointer to an integer where the x-coordinate of the rotation
 *            center (canvas coordinates) will be stored.
 * @param[out] cy Pointer to an integer where the y-coordinate of the rotation
 *            center (canvas coordinates) will be stored.
 *
 * @ingroup Elm_Map_Group
 */
EAPI void elm_map_rotate_get(const Elm_Map *obj, double *degree, int *cx, int *cy);

/**
 * @brief Set the user agent for the map.
 *
 * The user agent string is used when making HTTP requests to map tile
 * services or other online map data providers.
 *
 * @param[in] obj The map object.
 * @param[in] user_agent The user agent string to set.
 *                 Example: "MyMapApplication/1.0".
 *
 * @ingroup Elm_Map_Group
 */
EAPI void elm_map_user_agent_set(Elm_Map *obj, const char *user_agent);

/**
 * @brief Get the user agent for the map.
 *
 * Retrieves the user agent string used for HTTP requests.
 *
 * @param[in] obj The map object.
 * @return The current user agent string. This string is valid as long as
 *         the object exists or until the user agent is set again.
 *         The caller must not free this string.
 *
 * @ingroup Elm_Map_Group
 */
EAPI const char *elm_map_user_agent_get(const Elm_Map *obj);

/**
 * @brief Set the maximum zoom level of the map.
 *
 * This function defines the highest zoom level the user can zoom in to.
 *
 * @param[in] obj The map object.
 * @param[in] zoom The maximum zoom level to set. For example, 18 or 20.
 *
 * @ingroup Elm_Map_Group
 */
EAPI void elm_map_zoom_max_set(Elm_Map *obj, int zoom);

/**
 * @brief Get the maximum zoom level of the map.
 *
 * This function retrieves the highest zoom level the user can zoom in to.
 *
 * @param[in] obj The map object.
 * @return The maximum zoom level.
 *
 * @ingroup Elm_Map_Group
 */
EAPI int elm_map_zoom_max_get(const Elm_Map *obj);

/**
 * @brief Get the geographical coordinates of the center of the map.
 *
 * Retrieves the longitude and latitude of the current map center.
 *
 * @param[in] obj The map object.
 * @param[out] lon Pointer to a double where the longitude will be stored.
 * @param[out] lat Pointer to a double where the latitude will be stored.
 *
 * @ingroup Elm_Map_Group
 */
EAPI void elm_map_region_get(const Elm_Map *obj, double *lon, double *lat);

/**
 * @brief Get the list of all overlays on the map.
 *
 * Retrieves a list of all `Elm_Map_Overlay` objects currently added to the map.
 * The list itself and its contents are owned by the map object and should not
 * be modified or freed by the caller. The list is valid until the map's
 * overlays are changed.
 *
 * @param[in] obj The map object.
 * @return A list (Eina_List *) of `Elm_Map_Overlay` objects.
 *         Each element in the list is a pointer to an `Elm_Map_Overlay`.
 *         Returns @c NULL on failure or if there are no overlays.
 *
 * @ingroup Elm_Map_Group
 */
EAPI Eina_List *elm_map_overlays_get(const Elm_Map *obj);

/**
 * @brief Get the status of tile loading.
 *
 * Retrieves the number of tiles currently being loaded (try_num) and
 * the number of tiles that have finished loading (finish_num).
 * This can be used to display a progress indicator for map loading.
 *
 * @param[in] obj The map object.
 * @param[out] try_num Pointer to an integer where the number of tiles
 *               attempting to load will be stored.
 * @param[out] finish_num Pointer to an integer where the number of tiles
 *                that have successfully loaded will be stored.
 *
 * @ingroup Elm_Map_Group
 */
EAPI void elm_map_tile_load_status_get(const Elm_Map *obj, int *try_num, int *finish_num);

/**
 * @brief Set the source for a specific map feature type.
 *
 * Configures the provider for map tiles, routing services, or geocoding services.
 * The available source names can be retrieved using `elm_map_sources_get()`.
 *
 * @param[in] obj The map object.
 * @param[in] type The type of source to set (e.g., ELM_MAP_SOURCE_TYPE_TILE).
 * @param[in] source_name The name of the source provider. For example,
 *                      "openstreetmap" for tiles.
 *
 * @ingroup Elm_Map_Group
 */
EAPI void elm_map_source_set(Elm_Map *obj, Elm_Map_Source_Type type, const char *source_name);

/**
 * @brief Get the current source for a specific map feature type.
 *
 * Retrieves the name of the provider currently configured for map tiles,
 * routing services, or geocoding services.
 *
 * @param[in] obj The map object.
 * @param[in] type The type of source to get (e.g., ELM_MAP_SOURCE_TYPE_TILE).
 * @return The name of the current source provider for the given type.
 *         The returned string is an internal string and must not be freed.
 *         It remains valid until the source is changed or the map object is destroyed.
 *         Returns @c NULL if no source is set for the type or on error.
 *
 * @ingroup Elm_Map_Group
 */
EAPI const char *elm_map_source_get(const Elm_Map *obj, Elm_Map_Source_Type type);

/**
 * @brief Add a route to the map and request its calculation.
 *
 * This function initiates a request to calculate a route between two geographical
 * points (flon, flat) and (tlon, tlat) using the specified type and method.
 * The result of the route calculation will be delivered asynchronously via
 * the `route_cb` callback.
 *
 * @param[in] obj The map object.
 * @param[in] type The type of route to calculate (e.g., ELM_MAP_ROUTE_TYPE_MOTOCAR).
 * @param[in] method The method for route calculation (e.g., ELM_MAP_ROUTE_METHOD_FASTEST).
 * @param[in] flon The longitude of the starting point.
 * @param[in] flat The latitude of the starting point.
 * @param[in] tlon The longitude of the destination point.
 * @param[in] tlat The latitude of the destination point.
 * @param[in] route_cb A callback function that will be invoked when the route
 *                 calculation is complete. The callback will receive the
 *                 `Elm_Map_Route` object.
 * @param[in] data User data to be passed to the `route_cb` callback.
 * @return An `Elm_Map_Route` object representing the requested route. This object
 *         can be used to track the status of the route calculation or to display
 *         it on the map once calculated. Returns @c NULL on failure to initiate
 *         the route request.
 *
 * @ingroup Elm_Map_Group
 */
EAPI Elm_Map_Route *elm_map_route_add(Elm_Map *obj, Elm_Map_Route_Type type, Elm_Map_Route_Method method, double flon, double flat, double tlon, double tlat, Elm_Map_Route_Cb route_cb, void *data);

/**
 * @brief Add a track to be displayed on the map.
 *
 * This function is used to display a pre-defined path or track on the map.
 * The `emap` parameter likely refers to an Evas Map object or similar
 * data structure containing the track information.
 *
 * @param[in] obj The map object.
 * @param[in] emap A pointer to the track data (e.g., an Evas_Object representing
 *              the track, or a specific track data structure). The exact type
 *              and interpretation of this parameter depend on the underlying
 *              map implementation.
 * @return An `Efl_Canvas_Object` representing the track on the map. This object
 *         can be used to manipulate the track's appearance or to remove it later.
 *         Returns @c NULL on failure.
 *
 * @ingroup Elm_Map_Group
 */
EAPI Efl_Canvas_Object *elm_map_track_add(Elm_Map *obj, void *emap);

/**
 * @brief Convert geographical coordinates (longitude, latitude) to canvas coordinates (x, y).
 *
 * This function translates a point from the map's geographical coordinate system
 * to the widget's local canvas coordinate system.
 *
 * @param[in] obj The map object.
 * @param[in] lon The longitude of the geographical point.
 * @param[in] lat The latitude of the geographical point.
 * @param[out] x Pointer to an integer where the corresponding x-coordinate on
 *             the canvas will be stored.
 * @param[out] y Pointer to an integer where the corresponding y-coordinate on
 *             the canvas will be stored.
 *
 * @ingroup Elm_Map_Group
 */
EAPI void elm_map_region_to_canvas_convert(const Elm_Map *obj, double lon, double lat, int *x, int *y);

/**
 * @brief Add a circle overlay to the map.
 *
 * Creates and adds a circular overlay centered at the given geographical
 * coordinates (lon, lat) with a specified radius.
 *
 * @param[in] obj The map object.
 * @param[in] lon The longitude of the circle's center.
 * @param[in] lat The latitude of the circle's center.
 * @param[in] radius The radius of the circle in meters.
 * @return An `Elm_Map_Overlay` object representing the added circle.
 *         Returns @c NULL on failure.
 *
 * @ingroup Elm_Map_Group
 */
EAPI Elm_Map_Overlay *elm_map_overlay_circle_add(Elm_Map *obj, double lon, double lat, double radius);

/**
 * @brief Add a new "class" overlay to the map.
 *
 * A class overlay is a container for other overlays, allowing them to be
 * grouped and managed together (e.g., for styling or collective visibility).
 *
 * @param[in] obj The map object.
 * @return An `Elm_Map_Overlay` object representing the added class overlay.
 *         Returns @c NULL on failure.
 *
 * @ingroup Elm_Map_Group
 */
EAPI Elm_Map_Overlay *elm_map_overlay_class_add(Elm_Map *obj);

/**
 * @brief Add a bubble overlay to the map.
 *
 * Bubble overlays are typically used to display informational pop-ups or
 * callouts associated with a point on the map. The content and position
 * of the bubble are usually set using other overlay functions.
 *
 * @param[in] obj The map object.
 * @return An `Elm_Map_Overlay` object representing the added bubble.
 *         Returns @c NULL on failure.
 *
 * @ingroup Elm_Map_Group
 */
EAPI Elm_Map_Overlay *elm_map_overlay_bubble_add(Elm_Map *obj);

/**
 * @brief Get the available source names for a specific map feature type.
 *
 * Retrieves a list of names for available providers of map tiles, routing
 * services, or geocoding services. These names can be used with
 * `elm_map_source_set()`.
 *
 * @param[in] obj The map object.
 * @param[in] type The type of source for which to list available providers
 *                 (e.g., ELM_MAP_SOURCE_TYPE_TILE).
 * @return A NULL-terminated array of strings, where each string is a source name.
 *         The array and its strings are owned by the map object and must not
 *         be freed or modified by the caller. The data is valid until the
 *         map object is destroyed or its sources change.
 *         Example: For `ELM_MAP_SOURCE_TYPE_TILE`, might return `{"openstreetmap", "another_provider", NULL}`.
 *         Returns @c NULL on failure or if no sources are available for the type.
 *
 * @ingroup Elm_Map_Group
 */
EAPI const char **elm_map_sources_get(const Elm_Map *obj, Elm_Map_Source_Type type);

/**
 * @brief Add a polygon overlay to the map.
 *
 * Creates an empty polygon overlay. Points (vertices) must be added to this
 * polygon overlay using other specific overlay functions to define its shape.
 *
 * @param[in] obj The map object.
 * @return An `Elm_Map_Overlay` object representing the added polygon.
 *         Returns @c NULL on failure.
 *
 * @ingroup Elm_Map_Group
 */
EAPI Elm_Map_Overlay *elm_map_overlay_polygon_add(Elm_Map *obj);

/**
 * @brief Add a line overlay to the map.
 *
 * Creates a line overlay connecting two geographical points: from (flon, flat)
 * to (tlon, tlat). For polylines with more than two points, multiple segments
 * might need to be added, or a more complex overlay type (if available) should be used.
 *
 * @param[in] obj The map object.
 * @param[in] flon The longitude of the starting point of the line.
 * @param[in] flat The latitude of the starting point of the line.
 * @param[in] tlon The longitude of the ending point of the line.
 * @param[in] tlat The latitude of the ending point of the line.
 * @return An `Elm_Map_Overlay` object representing the added line.
 *         Returns @c NULL on failure.
 *
 * @ingroup Elm_Map_Group
 */
EAPI Elm_Map_Overlay *elm_map_overlay_line_add(Elm_Map *obj, double flon, double flat, double tlon, double tlat);

/**
 * @brief Pan the map to show the specified geographical region.
 *
 * This function centers the map view on the given longitude and latitude.
 * The current zoom level is maintained.
 *
 * @param[in] obj The map object.
 * @param[in] lon The longitude of the target region's center.
 * @param[in] lat The latitude of the target region's center.
 *
 * @ingroup Elm_Map_Group
 */
EAPI void elm_map_region_show(Elm_Map *obj, double lon, double lat);

/**
 * @brief Add a "name" entry (reverse geocoding) to the map.
 *
 * This function initiates a reverse geocoding request to find an address or
 * place name for the given geographical coordinates (lon, lat).
 * If an `address` is provided, it might be used as a hint or fallback.
 * The result is delivered asynchronously via the `name_cb` callback.
 *
 * @param[in] obj The map object.
 * @param[in] address An optional address string. Its usage depends on the
 *                  name service provider (e.g., could be a fallback or hint).
 *                  Can be @c NULL.
 * @param[in] lon The longitude for which to find the name/address.
 * @param[in] lat The latitude for which to find the name/address.
 * @param[in] name_cb A callback function invoked when the name lookup is complete.
 *                  It will receive an `Elm_Map_Name` object.
 * @param[in] data User data to be passed to the `name_cb` callback.
 * @return An `Elm_Map_Name` object representing the geocoding request.
 *         Returns @c NULL on failure to initiate the request.
 *
 * @ingroup Elm_Map_Group
 */
EAPI Elm_Map_Name *elm_map_name_add(const Elm_Map *obj, const char *address, double lon, double lat, Elm_Map_Name_Cb name_cb, void *data);

/**
 * @brief Search for a location by address (geocoding).
 *
 * This function initiates a geocoding request to find geographical coordinates
 * for a given `address` string.
 * The results (which may be multiple matches) are delivered asynchronously
 * via the `name_cb` callback.
 *
 * @param[in] obj The map object.
 * @param[in] address The address or place name to search for.
 *                  Example: "Eiffel Tower, Paris".
 * @param[in] name_cb A callback function invoked when the search is complete.
 *                  It will receive a list of `Elm_Map_Name` objects.
 * @param[in] data User data to be passed to the `name_cb` callback.
 *
 * @ingroup Elm_Map_Group
 */
EAPI void elm_map_name_search(const Elm_Map *obj, const char *address, Elm_Map_Name_List_Cb name_cb, void *data);

/**
 * @brief Pan the map to show the specified geographical region with an animation.
 *
 * This function smoothly animates the map view to center on the given
 * longitude and latitude. The current zoom level is maintained.
 *
 * @param[in] obj The map object.
 * @param[in] lon The longitude of the target region's center.
 * @param[in] lat The latitude of the target region's center.
 *
 * @ingroup Elm_Map_Group
 */
EAPI void elm_map_region_bring_in(Elm_Map *obj, double lon, double lat);

/**
 * @brief Pan and zoom the map to show the specified region with an animation.
 *
 * This function smoothly animates the map view to center on the given
 * longitude and latitude, and sets the zoom level.
 *
 * @param[in] obj The map object.
 * @param[in] zoom The target zoom level.
 * @param[in] lon The longitude of the target region's center.
 * @param[in] lat The latitude of the target region's center.
 *
 * @ingroup Elm_Map_Group
 */
EAPI void elm_map_region_zoom_bring_in(Elm_Map *obj, int zoom, double lon, double lat);

/**
 * @brief Remove a track from the map.
 *
 * Removes a previously added track object from the map display.
 *
 * @param[in] obj The map object.
 * @param[in] route The `Efl_Canvas_Object` representing the track to remove.
 *              This should be the object returned by `elm_map_track_add()`.
 *
 * @ingroup Elm_Map_Group
 */
EAPI void elm_map_track_remove(Elm_Map *obj, Efl_Canvas_Object *route);

/**
 * @brief Add an overlay representing a calculated route to the map.
 *
 * Displays a route, previously calculated (e.g., via `elm_map_route_add`),
 * on the map as an overlay.
 *
 * @param[in] obj The map object.
 * @param[in] route A pointer to the `Elm_Map_Route` object containing the
 *              route data to be displayed.
 * @return An `Elm_Map_Overlay` object representing the route on the map.
 *         Returns @c NULL on failure.
 *
 * @ingroup Elm_Map_Group
 */
EAPI Elm_Map_Overlay *elm_map_overlay_route_add(Elm_Map *obj, const Elm_Map_Route *route);

/**
 * @brief Add a scale overlay to the map.
 *
 * Adds a map scale indicator (e.g., a bar showing distances like "1 km" or "1 mile")
 * as an overlay at the specified canvas coordinates (x, y).
 *
 * @param[in] obj The map object.
 * @param[in] x The x-coordinate on the canvas where the scale overlay should be placed.
 * @param[in] y The y-coordinate on the canvas where the scale overlay should be placed.
 * @return An `Elm_Map_Overlay` object representing the scale indicator.
 *         Returns @c NULL on failure.
 *
 * @ingroup Elm_Map_Group
 */
EAPI Elm_Map_Overlay *elm_map_overlay_scale_add(Elm_Map *obj, int x, int y);

/**
 * @brief Add a generic overlay to the map at a specific geographical position.
 *
 * This function adds a basic overlay (often a marker or icon by default,
 * depending on the theme or further overlay configuration) at the given
 * longitude and latitude.
 *
 * @param[in] obj The map object.
 * @param[in] lon The longitude where the overlay will be placed.
 * @param[in] lat The latitude where the overlay will be placed.
 * @return An `Elm_Map_Overlay` object representing the added overlay.
 *         Returns @c NULL on failure.
 *
 * @ingroup Elm_Map_Group
 */
EAPI Elm_Map_Overlay *elm_map_overlay_add(Elm_Map *obj, double lon, double lat);

/**
 * @brief Convert canvas coordinates (x, y) to geographical coordinates (longitude, latitude).
 *
 * This function translates a point from the widget's local canvas coordinate system
 * to the map's geographical coordinate system.
 *
 * @param[in] obj The map object.
 * @param[in] x The x-coordinate on the canvas.
 * @param[in] y The y-coordinate on the canvas.
 * @param[out] lon Pointer to a double where the corresponding longitude
 *               will be stored.
 * @param[out] lat Pointer to a double where the corresponding latitude
 *               will be stored.
 *
 * @ingroup Elm_Map_Group
 */
EAPI void elm_map_canvas_to_region_convert(const Elm_Map *obj, int x, int y, double *lon, double *lat);

#endif
