/* Common header for maps: legacy Evas_Map API and Efl.Gfx.Mapping */

#ifndef EVAS_MAP_H
#define EVAS_MAP_H

#include "evas_common_private.h"
#include "evas_private.h"
#include <math.h>

/**
 * @internal
 * @brief Creates a new Evas_Map structure.
 * @param count Number of points in the map.
 * @param sync Whether the map should synchronize its points with object movements.
 * @return A new Evas_Map instance or NULL on failure.
 */
Evas_Map *_evas_map_new(int count, Eina_Bool sync);

/**
 * @internal
 * @brief Resets an Evas_Map structure to its initial state.
 * @param m The Evas_Map to reset.
 */
void _evas_map_reset(Evas_Map *m);

/**
 * @internal
 * @brief Calculates the geometry of the map based on its points and updates the object.
 * @param eo_obj The Evas object associated with the map.
 */
void _evas_map_calc_map_geometry(Evas_Object *eo_obj);

/**
 * @internal
 * @brief Rotates the map points around a center point (cx, cy).
 * @param m The Evas_Map to rotate.
 * @param degrees Rotation angle in degrees.
 * @param cx X-coordinate of the rotation center.
 * @param cy Y-coordinate of the rotation center.
 */
void _map_util_rotate(Evas_Map *m, double degrees, double cx, double cy);

/**
 * @internal
 * @brief Zooms the map points relative to a center point (cx, cy).
 * @param m The Evas_Map to zoom.
 * @param zoomx Zoom factor for the X-axis.
 * @param zoomy Zoom factor for the Y-axis.
 * @param cx X-coordinate of the zoom center.
 * @param cy Y-coordinate of the zoom center.
 */
void _map_util_zoom(Evas_Map *m, double zoomx, double zoomy, double cx, double cy);

/**
 * @internal
 * @brief Translates all points in the map by (dx, dy, dz).
 * @param m The Evas_Map to translate.
 * @param dx Translation amount for the X-axis.
 * @param dy Translation amount for the Y-axis.
 * @param dz Translation amount for the Z-axis.
 */
void _map_util_translate(Evas_Map *m, double dx, double dy, double dz);

/**
 * @internal
 * @brief Rotates the map points in 3D space around a center point (cx, cy, cz).
 * @param m The Evas_Map to rotate.
 * @param dx Rotation angle in degrees around the X-axis.
 * @param dy Rotation angle in degrees around the Y-axis.
 * @param dz Rotation angle in degrees around the Z-axis.
 * @param cx X-coordinate of the rotation center.
 * @param cy Y-coordinate of the rotation center.
 * @param cz Z-coordinate of the rotation center.
 */
void _map_util_3d_rotate(Evas_Map *m, double dx, double dy, double dz, double cx, double cy, double cz);

/**
 * @internal
 * @brief Applies 3D lighting effects to the map points.
 * @param m The Evas_Map to apply lighting to.
 * @param lx X-coordinate of the light source.
 * @param ly Y-coordinate of the light source.
 * @param lz Z-coordinate of the light source.
 * @param lr Red component of the light color (0-255).
 * @param lg Green component of the light color (0-255).
 * @param lb Blue component of the light color (0-255).
 * @param ar Red component of the ambient color (0-255).
 * @param ag Green component of the ambient color (0-255).
 * @param ab Blue component of the ambient color (0-255).
 */
void _map_util_3d_lighting(Evas_Map *m, double lx, double ly, double lz, int lr, int lg, int lb, int ar, int ag, int ab);

/**
 * @internal
 * @brief Applies a 3D perspective transformation to the map points.
 * @param m The Evas_Map to transform.
 * @param px X-coordinate of the perspective point (vanishing point X).
 * @param py Y-coordinate of the perspective point (vanishing point Y).
 * @param z0 Z-coordinate of the Z=0 plane (usually 0).
 * @param foc Focal length.
 */
void _map_util_3d_perspective(Evas_Map *m, double px, double py, double z0, double foc);

/**
 * @internal
 * @brief Rotates the map points using a quaternion around a center point (cx, cy, cz).
 * @param m The Evas_Map to rotate.
 * @param qx X component of the quaternion.
 * @param qy Y component of the quaternion.
 * @param qz Z component of the quaternion.
 * @param qw W component of the quaternion.
 * @param cx X-coordinate of the rotation center.
 * @param cy Y-coordinate of the rotation center.
 * @param cz Z-coordinate of the rotation center.
 */
void _map_util_quat_rotate(Evas_Map *m, double qx, double qy, double qz, double qw, double cx, double cy, double cz);

/**
 * @internal
 * @brief Enables or disables mapping for an Evas object.
 * @param eo_obj The Evas object.
 * @param obj The protected data of the Evas object.
 * @param enabled EINA_TRUE to enable mapping, EINA_FALSE to disable.
 */
void _evas_object_map_enable_set(Eo *eo_obj, Evas_Object_Protected_Data *obj, Eina_Bool enabled);

/**
 * @internal
 * @brief Populates the 4 points of a map from a given rectangle (x, y, w, h) and z-depth.
 * Assumes the map has 4 points.
 * The UV coordinates are set to (0,0), (w,0), (w,h), (0,h) respectively.
 *
 * @param m The Evas_Map to populate. Must have 4 points.
 * @param x The x-coordinate of the top-left corner of the rectangle.
 * @param y The y-coordinate of the top-left corner of the rectangle.
 * @param w The width of the rectangle.
 * @param h The height of the rectangle.
 * @param z The z-depth for all points.
 */
static inline void
_evas_map_util_points_populate(Evas_Map *m, const double x, const double y,
                               const double w, const double h, const double z)
{
   Evas_Map_Point *p = m->points;
   int i;

   p[0].x = x;
   p[0].y = y;
   p[0].z = z;
   p[0].u = 0.0;
   p[0].v = 0.0;

   p[1].x = x + w;
   p[1].y = y;
   p[1].z = z;
   p[1].u = w;
   p[1].v = 0.0;

   p[2].x = x + w;
   p[2].y = y + h;
   p[2].z = z;
   p[2].u = w;
   p[2].v = h;

   p[3].x = x;
   p[3].y = y + h;
   p[3].z = z;
   p[3].u = 0.0;
   p[3].v = h;

   for (i = 0; i < 4; i++)
     {
        p[i].px = p[i].x;
        p[i].py = p[i].y;
     }
}

/**
 * @internal
 * @brief Sets the world coordinates (x, y, z) of a specific point in the map.
 * Also updates the projected coordinates (px, py) to match.
 *
 * @param m The Evas_Map.
 * @param idx The index of the point to modify.
 * @param x The new x-coordinate.
 * @param y The new y-coordinate.
 * @param z The new z-coordinate.
 */
static inline void
_map_point_coord_set(Evas_Map *m, int idx, double x, double y, double z)
{
   Evas_Map_Point *p;

   EINA_SAFETY_ON_FALSE_RETURN((idx >= 0) && (idx < m->count));

   p = m->points + idx;
   p->x = p->px = x;
   p->y = p->py = y;
   p->z = z;
}

/**
 * @internal
 * @brief Gets the world coordinates (x, y, z) of a specific point in the map.
 *
 * @param m The Evas_Map.
 * @param idx The index of the point to query.
 * @param x Pointer to store the x-coordinate. Can be NULL.
 * @param y Pointer to store the y-coordinate. Can be NULL.
 * @param z Pointer to store the z-coordinate. Can be NULL.
 */
static inline void
_map_point_coord_get(const Evas_Map *m, int idx, double *x, double *y, double *z)
{
   const Evas_Map_Point *p;

   EINA_SAFETY_ON_FALSE_GOTO(m && (idx >= 0) && (idx < m->count), error);

   p = m->points + idx;
   if (x) *x = p->x;
   if (y) *y = p->y;
   if (z) *z = p->z;
   return;

error:
   if (x) *x = 0;
   if (y) *y = 0;
   if (z) *z = 0;
}

/**
 * @internal
 * @def MAP_OBJ_CHANGE
 * @brief Macro to signal that an object's map has changed.
 * This recalculates map geometry, marks the object as changed,
 * and sets the changed_map flag.
 * Assumes `eo_obj` and `obj` are in scope.
 */
#define MAP_OBJ_CHANGE() do { \
   _evas_map_calc_map_geometry(eo_obj); \
   evas_object_change(eo_obj, obj); \
   obj->changed_map = EINA_TRUE; \
   } while (0)

/**
 * @internal
 * @def MAP_POPULATE_DEFAULT
 * @brief Macro to populate a 4-point map using the object's current geometry.
 * Assumes `m`, `obj`, and `z` are in scope.
 * @param m The Evas_Map (must have 4 points).
 * @param z The z-depth to set for the map points.
 */
#define MAP_POPULATE_DEFAULT(m, z) \
   _evas_map_util_points_populate(m, obj->cur->geometry.x, obj->cur->geometry.y, \
                                  obj->cur->geometry.w, obj->cur->geometry.h, z)

#endif // EVAS_MAP_H

