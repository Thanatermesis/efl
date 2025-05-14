/** @internal Event descriptor for map press. See ELM_MAP_EVENT_PRESS. */
EWAPI const Efl_Event_Description _ELM_MAP_EVENT_PRESS =
   EFL_EVENT_DESCRIPTION("press");
/** @internal Event descriptor for map loaded. See ELM_MAP_EVENT_LOADED. */
EWAPI const Efl_Event_Description _ELM_MAP_EVENT_LOADED =
   EFL_EVENT_DESCRIPTION("loaded");
/** @internal Event descriptor for map tile load start. See ELM_MAP_EVENT_TILE_LOAD. */
EWAPI const Efl_Event_Description _ELM_MAP_EVENT_TILE_LOAD =
   EFL_EVENT_DESCRIPTION("tile,load");
/** @internal Event descriptor for map tile loaded. See ELM_MAP_EVENT_TILE_LOADED. */
EWAPI const Efl_Event_Description _ELM_MAP_EVENT_TILE_LOADED =
   EFL_EVENT_DESCRIPTION("tile,loaded");
/** @internal Event descriptor for map tile load failure. See ELM_MAP_EVENT_TILE_LOADED_FAIL. */
EWAPI const Efl_Event_Description _ELM_MAP_EVENT_TILE_LOADED_FAIL =
   EFL_EVENT_DESCRIPTION("tile,loaded,fail");
/** @internal Event descriptor for map route load start. See ELM_MAP_EVENT_ROUTE_LOAD. */
EWAPI const Efl_Event_Description _ELM_MAP_EVENT_ROUTE_LOAD =
   EFL_EVENT_DESCRIPTION("route,load");
/** @internal Event descriptor for map route loaded. See ELM_MAP_EVENT_ROUTE_LOADED. */
EWAPI const Efl_Event_Description _ELM_MAP_EVENT_ROUTE_LOADED =
   EFL_EVENT_DESCRIPTION("route,loaded");
/** @internal Event descriptor for map route load failure. See ELM_MAP_EVENT_ROUTE_LOADED_FAIL. */
EWAPI const Efl_Event_Description _ELM_MAP_EVENT_ROUTE_LOADED_FAIL =
   EFL_EVENT_DESCRIPTION("route,loaded,fail");
/** @internal Event descriptor for map name load start. See ELM_MAP_EVENT_NAME_LOAD. */
EWAPI const Efl_Event_Description _ELM_MAP_EVENT_NAME_LOAD =
   EFL_EVENT_DESCRIPTION("name,load");
/** @internal Event descriptor for map name loaded. See ELM_MAP_EVENT_NAME_LOADED. */
EWAPI const Efl_Event_Description _ELM_MAP_EVENT_NAME_LOADED =
   EFL_EVENT_DESCRIPTION("name,loaded");
/** @internal Event descriptor for map name load failure. See ELM_MAP_EVENT_NAME_LOADED_FAIL. */
EWAPI const Efl_Event_Description _ELM_MAP_EVENT_NAME_LOADED_FAIL =
   EFL_EVENT_DESCRIPTION("name,loaded,fail");
/** @internal Event descriptor for map overlay clicked. See ELM_MAP_EVENT_OVERLAY_CLICKED. */
EWAPI const Efl_Event_Description _ELM_MAP_EVENT_OVERLAY_CLICKED =
   EFL_EVENT_DESCRIPTION("overlay,clicked");
/** @internal Event descriptor for map overlay deletion. See ELM_MAP_EVENT_OVERLAY_DEL. */
EWAPI const Efl_Event_Description _ELM_MAP_EVENT_OVERLAY_DEL =
   EFL_EVENT_DESCRIPTION("overlay,del");

/**
 * @internal
 * @brief Implements the Eolian 'zoom_min_set' method.
 * @param obj The Elm_Map object.
 * @param pd Pointer to the private data of the Elm_Map object.
 * @param zoom The minimum zoom level to set.
 */
void _elm_map_zoom_min_set(Eo *obj, Elm_Map_Data *pd, int zoom);


/**
 * @internal
 * @brief Eolian reflection function for setting the 'zoom_min' property.
 * @param obj The Elm_Map object.
 * @param val An Eina_Value containing the integer for the minimum zoom.
 * @return Eina_Error EINA_ERROR_NO_ERROR on success, or an error code if conversion fails.
 */
static Eina_Error
__eolian_elm_map_zoom_min_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   int cval;
   if (!eina_value_int_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_map_zoom_min_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_map_zoom_min_set, EFL_FUNC_CALL(zoom), int zoom);

/**
 * @internal
 * @brief Implements the Eolian 'zoom_min_get' method.
 * @param obj The Elm_Map object.
 * @param pd Pointer to the private data of the Elm_Map object.
 * @return The minimum zoom level.
 */
int _elm_map_zoom_min_get(const Eo *obj, Elm_Map_Data *pd);

/**
 * @internal
 * @brief Eolian reflection function for getting the 'zoom_min' property.
 * @param obj The Elm_Map object.
 * @return An Eina_Value containing the integer for the minimum zoom, or an error Eina_Value.
 */
static Eina_Value
__eolian_elm_map_zoom_min_get_reflect(const Eo *obj)
{
   int val = elm_obj_map_zoom_min_get(obj);
   return eina_value_int_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_map_zoom_min_get, int, -1 /* +1 */);

/**
 * @internal
 * @brief Implements the Eolian 'rotate_set' method.
 * @param obj The Elm_Map object.
 * @param pd Pointer to the private data of the Elm_Map object.
 * @param degree Angle from 0.0 to 360.0 to rotate around Z axis.
 * @param cx Rotation's center horizontal position.
 * @param cy Rotation's center vertical position.
 */
void _elm_map_map_rotate_set(Eo *obj, Elm_Map_Data *pd, double degree, int cx, int cy);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_map_rotate_set, EFL_FUNC_CALL(degree, cx, cy), double degree, int cx, int cy);

/**
 * @internal
 * @brief Implements the Eolian 'rotate_get' method.
 * @param obj The Elm_Map object.
 * @param pd Pointer to the private data of the Elm_Map object.
 * @param[out] degree Pointer to store the angle from 0.0 to 360.0.
 * @param[out] cx Pointer to store the rotation's center horizontal position.
 * @param[out] cy Pointer to store the rotation's center vertical position.
 */
void _elm_map_map_rotate_get(const Eo *obj, Elm_Map_Data *pd, double *degree, int *cx, int *cy);

EOAPI EFL_VOID_FUNC_BODYV_CONST(elm_obj_map_rotate_get, EFL_FUNC_CALL(degree, cx, cy), double *degree, int *cx, int *cy);

/**
 * @internal
 * @brief Implements the Eolian 'user_agent_set' method.
 * @param obj The Elm_Map object.
 * @param pd Pointer to the private data of the Elm_Map object.
 * @param user_agent The user agent string to set.
 */
void _elm_map_user_agent_set(Eo *obj, Elm_Map_Data *pd, const char *user_agent);


/**
 * @internal
 * @brief Eolian reflection function for setting the 'user_agent' property.
 * @param obj The Elm_Map object.
 * @param val An Eina_Value containing the string for the user agent.
 * @return Eina_Error EINA_ERROR_NO_ERROR on success, or an error code if conversion fails.
 */
static Eina_Error
__eolian_elm_map_user_agent_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   const char *cval;
   if (!eina_value_string_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_map_user_agent_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_map_user_agent_set, EFL_FUNC_CALL(user_agent), const char *user_agent);

/**
 * @internal
 * @brief Implements the Eolian 'user_agent_get' method.
 * @param obj The Elm_Map object.
 * @param pd Pointer to the private data of the Elm_Map object.
 * @return The current user agent string.
 */
const char *_elm_map_user_agent_get(const Eo *obj, Elm_Map_Data *pd);

/**
 * @internal
 * @brief Eolian reflection function for getting the 'user_agent' property.
 * @param obj The Elm_Map object.
 * @return An Eina_Value containing the string for the user agent, or an error Eina_Value.
 */
static Eina_Value
__eolian_elm_map_user_agent_get_reflect(const Eo *obj)
{
   const char *val = elm_obj_map_user_agent_get(obj);
   return eina_value_string_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_map_user_agent_get, const char *, NULL);

/**
 * @internal
 * @brief Implements the Eolian 'zoom_max_set' method.
 * @param obj The Elm_Map object.
 * @param pd Pointer to the private data of the Elm_Map object.
 * @param zoom The maximum zoom level to set.
 */
void _elm_map_zoom_max_set(Eo *obj, Elm_Map_Data *pd, int zoom);

/**
 * @internal
 * @brief Eolian reflection function for setting the 'zoom_max' property.
 * @param obj The Elm_Map object.
 * @param val An Eina_Value containing the integer for the maximum zoom.
 * @return Eina_Error EINA_ERROR_NO_ERROR on success, or an error code if conversion fails.
 */
static Eina_Error
__eolian_elm_map_zoom_max_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   int cval;
   if (!eina_value_int_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_map_zoom_max_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_map_zoom_max_set, EFL_FUNC_CALL(zoom), int zoom);

/**
 * @internal
 * @brief Implements the Eolian 'zoom_max_get' method.
 * @param obj The Elm_Map object.
 * @param pd Pointer to the private data of the Elm_Map object.
 * @return The maximum zoom level.
 */
int _elm_map_zoom_max_get(const Eo *obj, Elm_Map_Data *pd);

/**
 * @internal
 * @brief Eolian reflection function for getting the 'zoom_max' property.
 * @param obj The Elm_Map object.
 * @return An Eina_Value containing the integer for the maximum zoom, or an error Eina_Value.
 */
static Eina_Value
__eolian_elm_map_zoom_max_get_reflect(const Eo *obj)
{
   int val = elm_obj_map_zoom_max_get(obj);
   return eina_value_int_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_map_zoom_max_get, int, -1 /* +1 */);

/**
 * @internal
 * @brief Implements the Eolian 'region_get' method.
 * @param obj The Elm_Map object.
 * @param pd Pointer to the private data of the Elm_Map object.
 * @param[out] lon Pointer to store the longitude.
 * @param[out] lat Pointer to store the latitude.
 */
void _elm_map_region_get(const Eo *obj, Elm_Map_Data *pd, double *lon, double *lat);

EOAPI EFL_VOID_FUNC_BODYV_CONST(elm_obj_map_region_get, EFL_FUNC_CALL(lon, lat), double *lon, double *lat);

/**
 * @internal
 * @brief Implements the Eolian 'overlays_get' method.
 * @param obj The Elm_Map object.
 * @param pd Pointer to the private data of the Elm_Map object.
 * @return A list of all overlays (Elm_Map_Overlay objects).
 */
Eina_List *_elm_map_overlays_get(const Eo *obj, Elm_Map_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_map_overlays_get, Eina_List *, NULL);

/**
 * @internal
 * @brief Implements the Eolian 'tile_load_status_get' method.
 * @param obj The Elm_Map object.
 * @param pd Pointer to the private data of the Elm_Map object.
 * @param[out] try_num Pointer to store the number of tiles download requested.
 * @param[out] finish_num Pointer to store the number of tiles successfully downloaded.
 */
void _elm_map_tile_load_status_get(const Eo *obj, Elm_Map_Data *pd, int *try_num, int *finish_num);

EOAPI EFL_VOID_FUNC_BODYV_CONST(elm_obj_map_tile_load_status_get, EFL_FUNC_CALL(try_num, finish_num), int *try_num, int *finish_num);

/**
 * @internal
 * @brief Implements the Eolian 'source_set' method.
 * @param obj The Elm_Map object.
 * @param pd Pointer to the private data of the Elm_Map object.
 * @param type The source type (e.g., tile, route, name).
 * @param source_name The name of the source to use.
 */
void _elm_map_source_set(Eo *obj, Elm_Map_Data *pd, Elm_Map_Source_Type type, const char *source_name);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_map_source_set, EFL_FUNC_CALL(type, source_name), Elm_Map_Source_Type type, const char *source_name);

/**
 * @internal
 * @brief Implements the Eolian 'source_get' method.
 * @param obj The Elm_Map object.
 * @param pd Pointer to the private data of the Elm_Map object.
 * @param type The source type.
 * @return The name of the currently used source for the given type.
 */
const char *_elm_map_source_get(const Eo *obj, Elm_Map_Data *pd, Elm_Map_Source_Type type);

EOAPI EFL_FUNC_BODYV_CONST(elm_obj_map_source_get, const char *, NULL, EFL_FUNC_CALL(type), Elm_Map_Source_Type type);

/**
 * @internal
 * @brief Implements the Eolian 'route_add' method.
 * @param obj The Elm_Map object.
 * @param pd Pointer to the private data of the Elm_Map object.
 * @param type The type of transport for the route.
 * @param method The routing method (e.g., fastest, shortest).
 * @param flon The starting longitude.
 * @param flat The starting latitude.
 * @param tlon The destination longitude.
 * @param tlat The destination latitude.
 * @param route_cb Callback function for route information.
 * @param data User data for the callback.
 * @return The created Elm_Map_Route object, or NULL on failure.
 */
Elm_Map_Route *_elm_map_route_add(Eo *obj, Elm_Map_Data *pd, Elm_Map_Route_Type type, Elm_Map_Route_Method method, double flon, double flat, double tlon, double tlat, Elm_Map_Route_Cb route_cb, void *data);

EOAPI EFL_FUNC_BODYV(elm_obj_map_route_add, Elm_Map_Route *, NULL, EFL_FUNC_CALL(type, method, flon, flat, tlon, tlat, route_cb, data), Elm_Map_Route_Type type, Elm_Map_Route_Method method, double flon, double flat, double tlon, double tlat, Elm_Map_Route_Cb route_cb, void *data);

/**
 * @internal
 * @brief Implements the Eolian 'track_add' method.
 * @param obj The Elm_Map object.
 * @param pd Pointer to the private data of the Elm_Map object.
 * @param emap The Emap route object (specific type, typically internal).
 * @return The created Efl_Canvas_Object representing the track, or NULL on failure.
 */
Efl_Canvas_Object *_elm_map_track_add(Eo *obj, Elm_Map_Data *pd, void *emap);

EOAPI EFL_FUNC_BODYV(elm_obj_map_track_add, Efl_Canvas_Object *, NULL, EFL_FUNC_CALL(emap), void *emap);

/**
 * @internal
 * @brief Implements the Eolian 'region_to_canvas_convert' method.
 * @param obj The Elm_Map object.
 * @param pd Pointer to the private data of the Elm_Map object.
 * @param lon The longitude to convert.
 * @param lat The latitude to convert.
 * @param[out] x Pointer to store the canvas horizontal coordinate.
 * @param[out] y Pointer to store the canvas vertical coordinate.
 */
void _elm_map_region_to_canvas_convert(const Eo *obj, Elm_Map_Data *pd, double lon, double lat, int *x, int *y);

EOAPI EFL_VOID_FUNC_BODYV_CONST(elm_obj_map_region_to_canvas_convert, EFL_FUNC_CALL(lon, lat, x, y), double lon, double lat, int *x, int *y);

/**
 * @internal
 * @brief Implements the Eolian 'overlay_circle_add' method.
 * @param obj The Elm_Map object.
 * @param pd Pointer to the private data of the Elm_Map object.
 * @param lon The center longitude of the circle overlay.
 * @param lat The center latitude of the circle overlay.
 * @param radius The radius of the circle overlay in pixels.
 * @return The created Elm_Map_Overlay object, or NULL on failure.
 */
Elm_Map_Overlay *_elm_map_overlay_circle_add(Eo *obj, Elm_Map_Data *pd, double lon, double lat, double radius);

EOAPI EFL_FUNC_BODYV(elm_obj_map_overlay_circle_add, Elm_Map_Overlay *, NULL, EFL_FUNC_CALL(lon, lat, radius), double lon, double lat, double radius);

/**
 * @internal
 * @brief Implements the Eolian 'overlay_class_add' method.
 * @param obj The Elm_Map object.
 * @param pd Pointer to the private data of the Elm_Map object.
 * @return The created Elm_Map_Overlay object (class type), or NULL on failure.
 */
Elm_Map_Overlay *_elm_map_overlay_class_add(Eo *obj, Elm_Map_Data *pd);

EOAPI EFL_FUNC_BODY(elm_obj_map_overlay_class_add, Elm_Map_Overlay *, NULL);

/**
 * @internal
 * @brief Implements the Eolian 'overlay_bubble_add' method.
 * @param obj The Elm_Map object.
 * @param pd Pointer to the private data of the Elm_Map object.
 * @return The created Elm_Map_Overlay object (bubble type), or NULL on failure.
 */
Elm_Map_Overlay *_elm_map_overlay_bubble_add(Eo *obj, Elm_Map_Data *pd);

EOAPI EFL_FUNC_BODY(elm_obj_map_overlay_bubble_add, Elm_Map_Overlay *, NULL);

/**
 * @internal
 * @brief Implements the Eolian 'sources_get' method.
 * @param obj The Elm_Map object.
 * @param pd Pointer to the private data of the Elm_Map object.
 * @param type The source type.
 * @return A NULL-terminated array of strings with available source names.
 */
const char **_elm_map_sources_get(const Eo *obj, Elm_Map_Data *pd, Elm_Map_Source_Type type);

EOAPI EFL_FUNC_BODYV_CONST(elm_obj_map_sources_get, const char **, NULL, EFL_FUNC_CALL(type), Elm_Map_Source_Type type);

/**
 * @internal
 * @brief Implements the Eolian 'overlay_polygon_add' method.
 * @param obj The Elm_Map object.
 * @param pd Pointer to the private data of the Elm_Map object.
 * @return The created Elm_Map_Overlay object (polygon type), or NULL on failure.
 */
Elm_Map_Overlay *_elm_map_overlay_polygon_add(Eo *obj, Elm_Map_Data *pd);

EOAPI EFL_FUNC_BODY(elm_obj_map_overlay_polygon_add, Elm_Map_Overlay *, NULL);

/**
 * @internal
 * @brief Implements the Eolian 'overlay_line_add' method.
 * @param obj The Elm_Map object.
 * @param pd Pointer to the private data of the Elm_Map object.
 * @param flon The starting longitude of the line.
 * @param flat The starting latitude of the line.
 * @param tlon The ending longitude of the line.
 * @param tlat The ending latitude of the line.
 * @return The created Elm_Map_Overlay object (line type), or NULL on failure.
 */
Elm_Map_Overlay *_elm_map_overlay_line_add(Eo *obj, Elm_Map_Data *pd, double flon, double flat, double tlon, double tlat);

EOAPI EFL_FUNC_BODYV(elm_obj_map_overlay_line_add, Elm_Map_Overlay *, NULL, EFL_FUNC_CALL(flon, flat, tlon, tlat), double flon, double flat, double tlon, double tlat);

/**
 * @internal
 * @brief Implements the Eolian 'region_show' method.
 * @param obj The Elm_Map object.
 * @param pd Pointer to the private data of the Elm_Map object.
 * @param lon The longitude to center the map on.
 * @param lat The latitude to center the map on.
 */
void _elm_map_region_show(Eo *obj, Elm_Map_Data *pd, double lon, double lat);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_map_region_show, EFL_FUNC_CALL(lon, lat), double lon, double lat);

/**
 * @internal
 * @brief Implements the Eolian 'name_add' method.
 * @param obj The Elm_Map object.
 * @param pd Pointer to the private data of the Elm_Map object.
 * @param address The address to lookup or NULL if using lon/lat.
 * @param lon The longitude for reverse lookup (if address is NULL).
 * @param lat The latitude for reverse lookup (if address is NULL).
 * @param name_cb Callback function for name resolution result.
 * @param data User data for the callback.
 * @return The created Elm_Map_Name object, or NULL on failure.
 */
Elm_Map_Name *_elm_map_name_add(const Eo *obj, Elm_Map_Data *pd, const char *address, double lon, double lat, Elm_Map_Name_Cb name_cb, void *data);

EOAPI EFL_FUNC_BODYV_CONST(elm_obj_map_name_add, Elm_Map_Name *, NULL, EFL_FUNC_CALL(address, lon, lat, name_cb, data), const char *address, double lon, double lat, Elm_Map_Name_Cb name_cb, void *data);

/**
 * @internal
 * @brief Implements the Eolian 'name_search' method.
 * @param obj The Elm_Map object.
 * @param pd Pointer to the private data of the Elm_Map object.
 * @param address The address string to search for.
 * @param name_cb Callback function for the list of name results.
 * @param data User data for the callback.
 */
void _elm_map_name_search(const Eo *obj, Elm_Map_Data *pd, const char *address, Elm_Map_Name_List_Cb name_cb, void *data);

EOAPI EFL_VOID_FUNC_BODYV_CONST(elm_obj_map_name_search, EFL_FUNC_CALL(address, name_cb, data), const char *address, Elm_Map_Name_List_Cb name_cb, void *data);

/**
 * @internal
 * @brief Implements the Eolian 'region_bring_in' method.
 * @param obj The Elm_Map object.
 * @param pd Pointer to the private data of the Elm_Map object.
 * @param lon The longitude to bring into the center.
 * @param lat The latitude to bring into the center.
 */
void _elm_map_map_region_bring_in(Eo *obj, Elm_Map_Data *pd, double lon, double lat);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_map_region_bring_in, EFL_FUNC_CALL(lon, lat), double lon, double lat);

/**
 * @internal
 * @brief Implements the Eolian 'region_zoom_bring_in' method.
 * @param obj The Elm_Map object.
 * @param pd Pointer to the private data of the Elm_Map object.
 * @param zoom The zoom level to set.
 * @param lon The longitude to center on.
 * @param lat The latitude to center on.
 */
void _elm_map_region_zoom_bring_in(Eo *obj, Elm_Map_Data *pd, int zoom, double lon, double lat);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_map_region_zoom_bring_in, EFL_FUNC_CALL(zoom, lon, lat), int zoom, double lon, double lat);

/**
 * @internal
 * @brief Implements the Eolian 'track_remove' method.
 * @param obj The Elm_Map object.
 * @param pd Pointer to the private data of the Elm_Map object.
 * @param route The Efl_Canvas_Object representing the track to remove.
 */
void _elm_map_track_remove(Eo *obj, Elm_Map_Data *pd, Efl_Canvas_Object *route);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_map_track_remove, EFL_FUNC_CALL(route), Efl_Canvas_Object *route);

/**
 * @internal
 * @brief Implements the Eolian 'overlay_route_add' method.
 * @param obj The Elm_Map object.
 * @param pd Pointer to the private data of the Elm_Map object.
 * @param route The Elm_Map_Route object to create an overlay for.
 * @return The created Elm_Map_Overlay object (route type), or NULL on failure.
 */
Elm_Map_Overlay *_elm_map_overlay_route_add(Eo *obj, Elm_Map_Data *pd, const Elm_Map_Route *route);

EOAPI EFL_FUNC_BODYV(elm_obj_map_overlay_route_add, Elm_Map_Overlay *, NULL, EFL_FUNC_CALL(route), const Elm_Map_Route *route);

/**
 * @internal
 * @brief Implements the Eolian 'overlay_scale_add' method.
 * @param obj The Elm_Map object.
 * @param pd Pointer to the private data of the Elm_Map object.
 * @param x The horizontal pixel coordinate for the scale overlay.
 * @param y The vertical pixel coordinate for the scale overlay.
 * @return The created Elm_Map_Overlay object (scale type), or NULL on failure.
 */
Elm_Map_Overlay *_elm_map_overlay_scale_add(Eo *obj, Elm_Map_Data *pd, int x, int y);

EOAPI EFL_FUNC_BODYV(elm_obj_map_overlay_scale_add, Elm_Map_Overlay *, NULL, EFL_FUNC_CALL(x, y), int x, int y);

/**
 * @internal
 * @brief Implements the Eolian 'overlay_add' method.
 * @param obj The Elm_Map object.
 * @param pd Pointer to the private data of the Elm_Map object.
 * @param lon The longitude for the overlay.
 * @param lat The latitude for the overlay.
 * @return The created Elm_Map_Overlay object (default type), or NULL on failure.
 */
Elm_Map_Overlay *_elm_map_overlay_add(Eo *obj, Elm_Map_Data *pd, double lon, double lat);

EOAPI EFL_FUNC_BODYV(elm_obj_map_overlay_add, Elm_Map_Overlay *, NULL, EFL_FUNC_CALL(lon, lat), double lon, double lat);

/**
 * @internal
 * @brief Implements the Eolian 'canvas_to_region_convert' method.
 * @param obj The Elm_Map object.
 * @param pd Pointer to the private data of the Elm_Map object.
 * @param x The canvas horizontal coordinate.
 * @param y The canvas vertical coordinate.
 * @param[out] lon Pointer to store the longitude.
 * @param[out] lat Pointer to store the latitude.
 */
void _elm_map_canvas_to_region_convert(const Eo *obj, Elm_Map_Data *pd, int x, int y, double *lon, double *lat);

EOAPI EFL_VOID_FUNC_BODYV_CONST(elm_obj_map_canvas_to_region_convert, EFL_FUNC_CALL(x, y, lon, lat), int x, int y, double *lon, double *lat);

/**
 * @internal
 * @brief Implements the Efl_Object 'constructor' method.
 * @details This function is called when a new Elm_Map object is created.
 *          It initializes the object's private data and sets up
 *          its basic properties.
 * @param obj The Elm_Map object being constructed.
 * @param pd Pointer to the private data of the Elm_Map object.
 * @return The constructed Efl_Object, which is the same as @p obj.
 */
Efl_Object *_elm_map_efl_object_constructor(Eo *obj, Elm_Map_Data *pd);

/**
 * @internal
 * @brief Implements the Efl_Object 'invalidate' method (destructor).
 * @details This function is called when an Elm_Map object is being destroyed.
 *          It should free any resources allocated by the object.
 * @param obj The Elm_Map object being invalidated.
 * @param pd Pointer to the private data of the Elm_Map object.
 */
void _elm_map_efl_object_invalidate(Eo *obj, Elm_Map_Data *pd);

/**
 * @internal
 * @brief Implements the Efl_Gfx_Entity 'position_set' method.
 * @param obj The Elm_Map object.
 * @param pd Pointer to the private data of the Elm_Map object.
 * @param pos The new 2D position (Eina_Position2D) for the map widget.
 */
void _elm_map_efl_gfx_entity_position_set(Eo *obj, Elm_Map_Data *pd, Eina_Position2D pos);

/**
 * @internal
 * @brief Implements the Efl_Gfx_Entity 'size_set' method.
 * @param obj The Elm_Map object.
 * @param pd Pointer to the private data of the Elm_Map object.
 * @param size The new 2D size (Eina_Size2D) for the map widget.
 */
void _elm_map_efl_gfx_entity_size_set(Eo *obj, Elm_Map_Data *pd, Eina_Size2D size);

/**
 * @internal
 * @brief Implements the Efl_Canvas_Group 'member_add' method.
 * @details Handles adding sub-objects to the map's canvas group.
 * @param obj The Elm_Map object (acting as a canvas group).
 * @param pd Pointer to the private data of the Elm_Map object.
 * @param sub_obj The Efl_Canvas_Object to add as a member.
 */
void _elm_map_efl_canvas_group_group_member_add(Eo *obj, Elm_Map_Data *pd, Efl_Canvas_Object *sub_obj);

/**
 * @internal
 * @brief Implements the Efl_Ui_Widget 'theme_apply' method.
 * @details Called when the theme needs to be (re)applied to the widget.
 * @param obj The Elm_Map object.
 * @param pd Pointer to the private data of the Elm_Map object.
 * @return Eina_Error EINA_ERROR_NO_ERROR on success, or an error code.
 */
Eina_Error _elm_map_efl_ui_widget_theme_apply(Eo *obj, Elm_Map_Data *pd);

/**
 * @internal
 * @brief Implements the Efl_Ui_Focus_Object 'on_focus_update' method.
 * @details Handles focus updates for the map widget.
 * @param obj The Elm_Map object.
 * @param pd Pointer to the private data of the Elm_Map object.
 * @return EINA_TRUE if focus handling was successful, EINA_FALSE otherwise.
 */
Eina_Bool _elm_map_efl_ui_focus_object_on_focus_update(Eo *obj, Elm_Map_Data *pd);

/**
 * @internal
 * @brief Implements the Efl_Ui_Widget 'widget_input_event_handler' method.
 * @details Handles input events (mouse, keyboard, etc.) for the map widget.
 * @param obj The Elm_Map object.
 * @param pd Pointer to the private data of the Elm_Map object.
 * @param eo_event The Efl_Event details.
 * @param source The source object of the event.
 * @return EINA_TRUE if the event was handled, EINA_FALSE otherwise.
 */
Eina_Bool _elm_map_efl_ui_widget_widget_input_event_handler(Eo *obj, Elm_Map_Data *pd, const Efl_Event *eo_event, Efl_Canvas_Object *source);

/**
 * @internal
 * @brief Implements the Efl_Access_Widget_Action 'elm_actions_get' method.
 * @details Provides accessibility actions for the map widget.
 * @param obj The Elm_Map object.
 * @param pd Pointer to the private data of the Elm_Map object.
 * @return A pointer to Efl_Access_Action_Data describing available actions, or NULL.
 */
const Efl_Access_Action_Data *_elm_map_efl_access_widget_action_elm_actions_get(const Eo *obj, Elm_Map_Data *pd);

/**
 * @internal
 * @brief Implements the Efl_Ui_Zoom 'zoom_level_set' method.
 * @param obj The Elm_Map object.
 * @param pd Pointer to the private data of the Elm_Map object.
 * @param zoom The zoom level to set.
 */
void _elm_map_efl_ui_zoom_zoom_level_set(Eo *obj, Elm_Map_Data *pd, double zoom);

/**
 * @internal
 * @brief Implements the Efl_Ui_Zoom 'zoom_level_get' method.
 * @param obj The Elm_Map object.
 * @param pd Pointer to the private data of the Elm_Map object.
 * @return The current zoom level.
 */
double _elm_map_efl_ui_zoom_zoom_level_get(const Eo *obj, Elm_Map_Data *pd);

/**
 * @internal
 * @brief Implements the Efl_Ui_Zoom 'zoom_mode_set' method.
 * @param obj The Elm_Map object.
 * @param pd Pointer to the private data of the Elm_Map object.
 * @param mode The zoom mode to set (e.g., manual, auto).
 */
void _elm_map_efl_ui_zoom_zoom_mode_set(Eo *obj, Elm_Map_Data *pd, Efl_Ui_Zoom_Mode mode);

/**
 * @internal
 * @brief Implements the Efl_Ui_Zoom 'zoom_mode_get' method.
 * @param obj The Elm_Map object.
 * @param pd Pointer to the private data of the Elm_Map object.
 * @return The current zoom mode.
 */
Efl_Ui_Zoom_Mode _elm_map_efl_ui_zoom_zoom_mode_get(const Eo *obj, Elm_Map_Data *pd);

/**
 * @internal
 * @brief Implements the Efl_Ui_Zoom 'zoom_animation_set' method.
 * @param obj The Elm_Map object.
 * @param pd Pointer to the private data of the Elm_Map object.
 * @param paused EINA_TRUE to pause zoom animation, EINA_FALSE to resume.
 */
void _elm_map_efl_ui_zoom_zoom_animation_set(Eo *obj, Elm_Map_Data *pd, Eina_Bool paused);

/**
 * @internal
 * @brief Implements the Efl_Ui_Zoom 'zoom_animation_get' method.
 * @param obj The Elm_Map object.
 * @param pd Pointer to the private data of the Elm_Map object.
 * @return EINA_TRUE if zoom animation is paused, EINA_FALSE otherwise.
 */
Eina_Bool _elm_map_efl_ui_zoom_zoom_animation_get(const Eo *obj, Elm_Map_Data *pd);

/**
 * @internal
 * @brief Implements the Elm_Interface_Scrollable 'wheel_disabled_set' method.
 * @param obj The Elm_Map object.
 * @param pd Pointer to the private data of the Elm_Map object.
 * @param disabled EINA_TRUE to disable wheel scrolling, EINA_FALSE to enable.
 */
void _elm_map_elm_interface_scrollable_wheel_disabled_set(Eo *obj, Elm_Map_Data *pd, Eina_Bool disabled);

/**
 * @internal
 * @brief Initializes the Elm_Map Efl_Class.
 * @details This function sets up the Eolian operations (methods) and
 *          property reflection for the Elm_Map class. It is called
 *          once when the class is first used.
 * @param klass The Efl_Class to initialize.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_map_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_MAP_EXTRA_OPS
#define ELM_MAP_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(elm_obj_map_zoom_min_set, _elm_map_zoom_min_set),
      EFL_OBJECT_OP_FUNC(elm_obj_map_zoom_min_get, _elm_map_zoom_min_get),
      EFL_OBJECT_OP_FUNC(elm_obj_map_rotate_set, _elm_map_map_rotate_set),
      EFL_OBJECT_OP_FUNC(elm_obj_map_rotate_get, _elm_map_map_rotate_get),
      EFL_OBJECT_OP_FUNC(elm_obj_map_user_agent_set, _elm_map_user_agent_set),
      EFL_OBJECT_OP_FUNC(elm_obj_map_user_agent_get, _elm_map_user_agent_get),
      EFL_OBJECT_OP_FUNC(elm_obj_map_zoom_max_set, _elm_map_zoom_max_set),
      EFL_OBJECT_OP_FUNC(elm_obj_map_zoom_max_get, _elm_map_zoom_max_get),
      EFL_OBJECT_OP_FUNC(elm_obj_map_region_get, _elm_map_region_get),
      EFL_OBJECT_OP_FUNC(elm_obj_map_overlays_get, _elm_map_overlays_get),
      EFL_OBJECT_OP_FUNC(elm_obj_map_tile_load_status_get, _elm_map_tile_load_status_get),
      EFL_OBJECT_OP_FUNC(elm_obj_map_source_set, _elm_map_source_set),
      EFL_OBJECT_OP_FUNC(elm_obj_map_source_get, _elm_map_source_get),
      EFL_OBJECT_OP_FUNC(elm_obj_map_route_add, _elm_map_route_add),
      EFL_OBJECT_OP_FUNC(elm_obj_map_track_add, _elm_map_track_add),
      EFL_OBJECT_OP_FUNC(elm_obj_map_region_to_canvas_convert, _elm_map_region_to_canvas_convert),
      EFL_OBJECT_OP_FUNC(elm_obj_map_overlay_circle_add, _elm_map_overlay_circle_add),
      EFL_OBJECT_OP_FUNC(elm_obj_map_overlay_class_add, _elm_map_overlay_class_add),
      EFL_OBJECT_OP_FUNC(elm_obj_map_overlay_bubble_add, _elm_map_overlay_bubble_add),
      EFL_OBJECT_OP_FUNC(elm_obj_map_sources_get, _elm_map_sources_get),
      EFL_OBJECT_OP_FUNC(elm_obj_map_overlay_polygon_add, _elm_map_overlay_polygon_add),
      EFL_OBJECT_OP_FUNC(elm_obj_map_overlay_line_add, _elm_map_overlay_line_add),
      EFL_OBJECT_OP_FUNC(elm_obj_map_region_show, _elm_map_region_show),
      EFL_OBJECT_OP_FUNC(elm_obj_map_name_add, _elm_map_name_add),
      EFL_OBJECT_OP_FUNC(elm_obj_map_name_search, _elm_map_name_search),
      EFL_OBJECT_OP_FUNC(elm_obj_map_region_bring_in, _elm_map_map_region_bring_in),
      EFL_OBJECT_OP_FUNC(elm_obj_map_region_zoom_bring_in, _elm_map_region_zoom_bring_in),
      EFL_OBJECT_OP_FUNC(elm_obj_map_track_remove, _elm_map_track_remove),
      EFL_OBJECT_OP_FUNC(elm_obj_map_overlay_route_add, _elm_map_overlay_route_add),
      EFL_OBJECT_OP_FUNC(elm_obj_map_overlay_scale_add, _elm_map_overlay_scale_add),
      EFL_OBJECT_OP_FUNC(elm_obj_map_overlay_add, _elm_map_overlay_add),
      EFL_OBJECT_OP_FUNC(elm_obj_map_canvas_to_region_convert, _elm_map_canvas_to_region_convert),
      EFL_OBJECT_OP_FUNC(efl_constructor, _elm_map_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_invalidate, _elm_map_efl_object_invalidate),
      EFL_OBJECT_OP_FUNC(efl_gfx_entity_position_set, _elm_map_efl_gfx_entity_position_set),
      EFL_OBJECT_OP_FUNC(efl_gfx_entity_size_set, _elm_map_efl_gfx_entity_size_set),
      EFL_OBJECT_OP_FUNC(efl_canvas_group_member_add, _elm_map_efl_canvas_group_group_member_add),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_theme_apply, _elm_map_efl_ui_widget_theme_apply),
      EFL_OBJECT_OP_FUNC(efl_ui_focus_object_on_focus_update, _elm_map_efl_ui_focus_object_on_focus_update),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_input_event_handler, _elm_map_efl_ui_widget_widget_input_event_handler),
      EFL_OBJECT_OP_FUNC(efl_access_widget_action_elm_actions_get, _elm_map_efl_access_widget_action_elm_actions_get),
      EFL_OBJECT_OP_FUNC(efl_ui_zoom_level_set, _elm_map_efl_ui_zoom_zoom_level_set),
      EFL_OBJECT_OP_FUNC(efl_ui_zoom_level_get, _elm_map_efl_ui_zoom_zoom_level_get),
      EFL_OBJECT_OP_FUNC(efl_ui_zoom_mode_set, _elm_map_efl_ui_zoom_zoom_mode_set),
      EFL_OBJECT_OP_FUNC(efl_ui_zoom_mode_get, _elm_map_efl_ui_zoom_zoom_mode_get),
      EFL_OBJECT_OP_FUNC(efl_ui_zoom_animation_set, _elm_map_efl_ui_zoom_zoom_animation_set),
      EFL_OBJECT_OP_FUNC(efl_ui_zoom_animation_get, _elm_map_efl_ui_zoom_zoom_animation_get),
      EFL_OBJECT_OP_FUNC(elm_interface_scrollable_wheel_disabled_set, _elm_map_elm_interface_scrollable_wheel_disabled_set),
      ELM_MAP_EXTRA_OPS
   );
   opsp = &ops;

   static const Efl_Object_Property_Reflection refl_table[] = {
      {"zoom_min", __eolian_elm_map_zoom_min_set_reflect, __eolian_elm_map_zoom_min_get_reflect},
      {"user_agent", __eolian_elm_map_user_agent_set_reflect, __eolian_elm_map_user_agent_get_reflect},
      {"zoom_max", __eolian_elm_map_zoom_max_set_reflect, __eolian_elm_map_zoom_max_get_reflect},
   };
   static const Efl_Object_Property_Reflection_Ops rops = {
      refl_table, EINA_C_ARRAY_LENGTH(refl_table)
   };
   ropsp = &rops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @internal
 * @brief Efl_Class_Description for the Elm_Map class.
 * @details This structure provides metadata for the Elm_Map class,
 *          including its version, name, type, data size, and pointers
 *          to initializer/constructor/destructor functions.
 */
static const Efl_Class_Description _elm_map_class_desc = {
   EO_VERSION,
   "Elm.Map",
   EFL_CLASS_TYPE_REGULAR,
   sizeof(Elm_Map_Data),
   _elm_map_class_initializer,
   _elm_map_class_constructor,
   NULL
};

EFL_DEFINE_CLASS(elm_map_class_get, &_elm_map_class_desc, EFL_UI_WIDGET_CLASS, ELM_INTERFACE_SCROLLABLE_MIXIN, EFL_ACCESS_WIDGET_ACTION_MIXIN, EFL_INPUT_CLICKABLE_MIXIN, EFL_UI_LEGACY_INTERFACE, EFL_UI_ZOOM_INTERFACE, NULL);

#include "elm_map_eo.legacy.c"
