#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include "Elementary.h"
#include "elm_module_helper.h"
#include "elm_widget_map.h"

#ifndef EFL_BUILD
# define EFL_BUILD
#endif
#undef ELM_MODULE_HELPER_H
#include "elm_module_helper.h"

/**
 * @brief Get the name of the map source.
 * This name is used to identify the map layer.
 * @return The map source name as a shared string. The caller must not free it.
 */
EMODAPI Eina_Stringshare *
map_module_source_name_get(void)
{
   return eina_stringshare_add("test_map");
}

/**
 * @brief Get the minimum supported zoom level for map tiles.
 * @return The minimum zoom level. For OpenStreetMap, this is typically 0.
 */
EMODAPI int
map_module_tile_zoom_min_get(void)
{
   return 0;
}

/**
 * @brief Get the maximum supported zoom level for map tiles.
 * @return The maximum zoom level. For OpenStreetMap, this is typically 18 or 19.
 */
EMODAPI int
map_module_tile_zoom_max_get(void)
{
   return 18;
}

/**
 * @brief Get the URL for a specific map tile.
 * This function constructs a URL to fetch a map tile from OpenStreetMap based on
 * the tile's coordinates (x, y) and zoom level.
 * @param obj The map object (unused).
 * @param x The x-coordinate of the tile.
 * @param y The y-coordinate of the tile.
 * @param zoom The zoom level.
 * @return A newly allocated string containing the tile URL. The caller is
 *         responsible for freeing this string. For example:
 *         "http://tile.openstreetmap.org/18/131072/87041.png"
 */
EMODAPI char *
map_module_tile_url_get(Evas_Object *obj EINA_UNUSED, int x, int y, int zoom)
{
   char buf[PATH_MAX];
   snprintf(buf, sizeof(buf), "http://tile.openstreetmap.org/%d/%d/%d.png",
            zoom, x, y);
   return strdup(buf);
}

/**
 * @brief Get the source for route data.
 * This module does not support routing.
 * @return NULL as routing is not implemented.
 */
EMODAPI char *
map_module_route_source_get(void)
{
   return NULL;
}

/**
 * @brief Parse route data from a source.
 * This is a no-op as this module does not support routing.
 * @param r The route object (unused).
 */
EMODAPI void
map_module_route_source_parse(Elm_Map_Route *r EINA_UNUSED)
{
   return;
}

/**
 * @brief Get the URL for a route request.
 * This module does not support routing.
 * @param obj The map object (unused).
 * @param type_name The route type name (unused).
 * @param method The route calculation method (unused).
 * @param flon From longitude (unused).
 * @param flat From latitude (unused).
 * @param tlon To longitude (unused).
 * @param tlat To latitude (unused).
 * @return A newly allocated empty string, as routing is not supported. The caller
 *         must free this string.
 */
EMODAPI char *
map_module_route_url_get(Evas_Object *obj EINA_UNUSED, const char *type_name EINA_UNUSED, int method EINA_UNUSED, double flon EINA_UNUSED, double flat EINA_UNUSED, double tlon EINA_UNUSED, double tlat EINA_UNUSED)
{
   return strdup("");
}

/**
 * @brief Get the URL for a geocoding (name lookup) request.
 * This module does not support geocoding.
 * @param obj The map object (unused).
 * @param method The geocoding method (unused).
 * @param name The name to search for (unused).
 * @param lon The longitude for context (unused).
 * @param lat The latitude for context (unused).
 * @return A newly allocated empty string, as geocoding is not supported. The
 *         caller must free this string.
 */
EMODAPI char *
map_module_name_url_get(Evas_Object *obj EINA_UNUSED, int method EINA_UNUSED, const char *name EINA_UNUSED, double lon EINA_UNUSED, double lat EINA_UNUSED)
{
   return strdup("");
}

/**
 * @brief Parse geocoding data from a source.
 * This is a no-op as this module does not support geocoding.
 * @param n The name object (unused).
 */
EMODAPI void
map_module_name_source_parse(Elm_Map_Name *n EINA_UNUSED)
{
   return;
}

/**
 * @brief Parse a list of geocoded names from a source.
 * This is a no-op as this module does not support geocoding.
 * @param nl The name list object (unused).
 */
EMODAPI void
map_module_name_list_source_parse(Elm_Map_Name_List *nl EINA_UNUSED)
{
   return;
}

/**
 * @brief Convert geographic coordinates (longitude, latitude) to tile coordinates.
 * This function is required for projecting geo coordinates onto the map.
 * This module does not implement this conversion.
 * @param obj The map object (unused).
 * @param zoom The zoom level (unused).
 * @param lon The longitude (unused).
 * @param lat The latitude (unused).
 * @param size The tile size (unused).
 * @param x Pointer to store the resulting tile x-coordinate (unused).
 * @param y Pointer to store the resulting tile y-coordinate (unused).
 * @return EINA_FALSE as this feature is not implemented.
 */
EMODAPI Eina_Bool
map_module_tile_geo_to_coord(const Evas_Object *obj EINA_UNUSED, int zoom EINA_UNUSED, double lon EINA_UNUSED, double lat EINA_UNUSED, int size EINA_UNUSED, int *x EINA_UNUSED, int *y EINA_UNUSED)
{
   return EINA_FALSE;
}

/**
 * @brief Convert tile coordinates to geographic coordinates (longitude, latitude).
 * This function is the inverse of geo_to_coord.
 * This module does not implement this conversion.
 * @param obj The map object (unused).
 * @param zoom The zoom level (unused).
 * @param x The tile x-coordinate (unused).
 * @param y The tile y-coordinate (unused).
 * @param size The tile size (unused).
 * @param lon Pointer to store the resulting longitude (unused).
 * @param lat Pointer to store the resulting latitude (unused).
 * @return EINA_FALSE as this feature is not implemented.
 */
EMODAPI Eina_Bool
map_module_tile_coord_to_geo(const Evas_Object *obj EINA_UNUSED, int zoom EINA_UNUSED, int x EINA_UNUSED, int y EINA_UNUSED, int size EINA_UNUSED, double *lon EINA_UNUSED, double *lat EINA_UNUSED)
{
   return EINA_FALSE;
}

/**
 * @brief Get the map scale at a specific geographic coordinate and zoom level.
 * The scale represents meters per pixel. This is useful for distance calculations.
 * This module does not provide scale information.
 * @param obj The map object (unused).
 * @param lon The longitude (unused).
 * @param lat The latitude (unused).
 * @param zoom The zoom level (unused).
 * @return 0 as scale calculation is not implemented.
 */
EMODAPI double
map_module_tile_scale_get(const Evas_Object *obj EINA_UNUSED, double lon EINA_UNUSED, double lat EINA_UNUSED, int zoom EINA_UNUSED)
{
   return 0;
}

/**
 * @brief Get a widget displaying copyright information for the map tiles.
 * Map providers often require displaying a copyright notice.
 * This module does not provide a copyright widget.
 * @param obj The parent map object (unused).
 * @return NULL as a copyright widget is not provided.
 */
EMODAPI Evas_Object *
map_module_tile_copyright_get(Evas_Object *obj EINA_UNUSED)
{
   return NULL;
}

/**
 * @brief Initializes the map module.
 * Called when the module is loaded.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_module_init(void)
{
   return EINA_TRUE;
}

/**
 * @brief Shuts down the map module.
 * Called when the module is unloaded.
 */
static void
_module_shutdown(void)
{
}

EINA_MODULE_INIT(_module_init);
EINA_MODULE_SHUTDOWN(_module_shutdown);

