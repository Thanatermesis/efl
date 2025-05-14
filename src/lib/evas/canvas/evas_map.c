#include "evas_map.h"

/**
 * @internal
 * @brief Handles geometry changes for a mapped object.
 * This function is called when the map's geometry (bounding box) changes.
 * It marks the object as changed, dirties its clip, recalculates clippees,
 * and informs about move/resize events.
 * @param eo_obj The Evas object whose map geometry changed.
 */
static void
_evas_map_calc_geom_change(Evas_Object *eo_obj)
{
   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   if (!obj) return;
   evas_object_change(eo_obj, obj);
   evas_object_clip_dirty(eo_obj, obj);
   if (!(obj->layer->evas->is_frozen))
     {
        evas_object_recalc_clippees(obj);
        if (!obj->is_smart && obj->cur->visible)
          {
             _evas_canvas_event_pointer_in_list_mouse_move_feed(obj->layer->evas, NULL, eo_obj, obj, 1, 1,
                          EINA_TRUE, NULL);
          }
     }
   evas_object_inform_call_move(eo_obj, obj);
   evas_object_inform_call_resize(eo_obj, obj);
}

/**
 * @internal
 * @brief Calculates the bounding box of the map points and updates the object's
 * normal_geometry if it has changed.
 * This function iterates through all points of the current map to find the
 * min/max x and y coordinates, effectively determining the 2D bounding box
 * of the transformed object. It also checks if the map points themselves
 * have changed compared to the previous state to trigger necessary updates.
 *
 * @param eo_obj The Evas object whose map geometry is to be calculated.
 */
void
_evas_map_calc_map_geometry(Evas_Object *eo_obj)
{
   Evas_Coord x1, x2, yy1, yy2;
   const Evas_Map_Point *p, *p_end;
   Eina_Bool ch = EINA_FALSE;

   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   if (!obj) return;
   if (!obj->map->cur.map) return;
   if (obj->map->prev.map)
     {
        if (obj->map->prev.map != obj->map->cur.map)
          {
             // FIXME: this causes an infinite loop somewhere... hard to debug
             if (obj->map->prev.map->count == obj->map->cur.map->count)
               {
                  const Evas_Map_Point *p2;

                  p = obj->map->cur.map->points;
                  p2 = obj->map->prev.map->points;
                  if (memcmp(p, p2, sizeof(Evas_Map_Point) *
                             obj->map->prev.map->count) != 0)
                    ch = EINA_TRUE;
                  if (!ch)
                    {
                       EINA_COW_WRITE_BEGIN(evas_object_map_cow, obj->map, Evas_Object_Map_Data, map_write)
                         {
                            if (map_write->cache_map) evas_map_free(map_write->cache_map);
                            map_write->cache_map = map_write->cur.map;
                            map_write->cur.map = map_write->prev.map;
                         }
                       EINA_COW_WRITE_END(evas_object_map_cow, obj->map, map_write);
                    }
               }
             else
               ch = EINA_TRUE;
          }
     }
   else
      ch = EINA_TRUE;

   p = obj->map->cur.map->points;
   p_end = p + obj->map->cur.map->count;
   x1 = x2 = lround(p->x);
   yy1 = yy2 = lround(p->y);
   p++;
   for (; p < p_end; p++)
     {
        Evas_Coord x, y;

        x = lround(p->x);
        y = lround(p->y);
        if (x < x1) x1 = x;
        if (x > x2) x2 = x;
        if (y < yy1) yy1 = y;
        if (y > yy2) yy2 = y;
     }
// this causes clip-out bugs now mapped objs canbe opaque!!!
//   // add 1 pixel of fuzz around the map region to ensure updates are correct
//   x1 -= 1; yy1 -= 1;
//   x2 += 1; yy2 += 1;
   if (obj->map->cur.map->normal_geometry.x != x1) ch = 1;
   if (obj->map->cur.map->normal_geometry.y != yy1) ch = 1;
   if (obj->map->cur.map->normal_geometry.w != (x2 - x1)) ch = 1;
   if (obj->map->cur.map->normal_geometry.h != (yy2 - yy1)) ch = 1;
   obj->map->cur.map->normal_geometry.x = x1;
   obj->map->cur.map->normal_geometry.y = yy1;
   obj->map->cur.map->normal_geometry.w = (x2 - x1);
   obj->map->cur.map->normal_geometry.h = (yy2 - yy1);

   /* if change_map is true, it means that the prev map data
      did not render before. even though both prev and cur
      has same map points we need to draw it */
   obj->changed_map |= ch;

   // This shouldn't really be needed, but without it we do have case
   // where the clip is wrong when a map doesn't change, so always forcing
   // it, as long as someone doesn't find a better fix.
   evas_object_clip_dirty(eo_obj, obj);
   if (ch) _evas_map_calc_geom_change(eo_obj);
}

/**
 * @internal
 * @brief Synchronizes the map points with the object's movement.
 * If move_sync is enabled for the map and there's a recorded movement difference
 * (diff_x, diff_y), this function applies that difference to all map points
 * (both projected and world coordinates) and then resets the difference.
 * This is used to keep the map visually static relative to the object's content
 * when the object itself is moved, rather than the map points being transformed.
 *
 * @param eo_obj The Evas object whose map needs synchronization.
 */
static void
evas_object_map_move_sync(Evas_Object *eo_obj)
{
   Evas_Object_Protected_Data *obj;
   Evas_Map *m;
   Evas_Map_Point *p;
   Evas_Coord diff_x, diff_y;
   int i, count;

   obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   if (!obj) return;

   if ((!obj->map->cur.map->move_sync.enabled) ||
       ((obj->map->cur.map->move_sync.diff_x == 0) &&
        (obj->map->cur.map->move_sync.diff_y == 0)))
     return;

   m = obj->map->cur.map;
   p = m->points;
   count = m->count;
   diff_x = m->move_sync.diff_x;
   diff_y = m->move_sync.diff_y;

   for (i = 0; i < count; i++, p++)
     {
        p->px += diff_x;
        p->py += diff_y;
        p->x += diff_x;
        p->y += diff_y;
     }
   m->move_sync.diff_x = 0;
   m->move_sync.diff_y = 0;

   _evas_map_calc_map_geometry(eo_obj);
}

/**
 * @internal
 * @brief Initializes the fields of an Evas_Map structure.
 * Sets default values for map properties like alpha, smooth, and point colors.
 *
 * @param m The Evas_Map structure to initialize.
 * @param count The number of points this map will have.
 * @param sync EINA_TRUE if the map should sync with object movements, EINA_FALSE otherwise.
 */
static void
_evas_map_init(Evas_Map *m, int count, Eina_Bool sync)
{
   m->move_sync.enabled = sync;
   m->count = count;
   m->alpha = 1;
   m->smooth = 1;
   m->magic = MAGIC_MAP;
   for (int i = 0; i < count; i++)
     {
        m->points[i].r = 255;
        m->points[i].g = 255;
        m->points[i].b = 255;
        m->points[i].a = 255;
     }
}

/**
 * @internal
 * @brief Allocates and initializes a new Evas_Map structure.
 * The actual number of allocated points might be adjusted (e.g., minimum 4, even number)
 * for engine efficiency.
 *
 * @param count The desired number of points for the map.
 * @param sync EINA_TRUE if the map should sync with object movements.
 * @return A pointer to the newly allocated Evas_Map, or NULL on failure.
 */
Evas_Map *
_evas_map_new(int count, Eina_Bool sync)
{
   Evas_Map *m;
   int alloc;

   /* Adjust allocation such that: at least 4 points, and always an even
    * number: this allows the software engine to work efficiently */
   alloc = (count < 4) ? 4 : count;
   if (alloc & 0x1) alloc ++;

   m = calloc(1, sizeof(Evas_Map) + (alloc * sizeof(Evas_Map_Point)));
   if (!m) return NULL;
   _evas_map_init(m, count, sync);
   return m;
}

/**
 * @internal
 * @brief Resets an existing Evas_Map structure to its default state.
 * Preserves the original point count and sync flag, but clears all other
 * map data (points, colors, transformations) and re-initializes them.
 *
 * @param m The Evas_Map to reset.
 */
void
_evas_map_reset(Evas_Map *m)
{
   int alloc, count;
   Eina_Bool sync;

   if (!m) return;

   /* Adjust allocation such that: at least 4 points, and always an even
    * number: this allows the software engine to work efficiently */
   alloc = (m->count < 4) ? 4 : m->count;
   if (alloc & 0x1) alloc ++;

   count = m->count;
   sync = m->move_sync.enabled;
   memset(m, 0, sizeof(Evas_Map) + (alloc * sizeof(Evas_Map_Point)));
   _evas_map_init(m, count, sync);
}

/**
 * @internal
 * @brief Copies the contents of one Evas_Map to another.
 * Both maps must have the same number of points.
 * Copies points, smooth, alpha, move_sync, and perspective data.
 *
 * @param dst The destination Evas_Map.
 * @param src The source Evas_Map.
 * @return EINA_TRUE on success, EINA_FALSE if maps have different point counts.
 */
static inline Eina_Bool
_evas_map_copy(Evas_Map *dst, const Evas_Map *src)
{
   if (dst->count != src->count)
     {
        ERR("cannot copy map of different sizes: dst=%i, src=%i", dst->count, src->count);
        return EINA_FALSE;
     }
   if (dst == src) return EINA_TRUE;
   if (dst->points != src->points)
     memcpy(dst->points, src->points, src->count * sizeof(Evas_Map_Point));
   dst->smooth = src->smooth;
   dst->alpha = src->alpha;
   dst->move_sync = src->move_sync;
   dst->persp = src->persp;
   return EINA_TRUE;
}

/**
 * @internal
 * @brief Duplicates an Evas_Map structure.
 * Creates a new map and copies all data from the original map.
 * The new map's move_sync is initially set to EINA_FALSE.
 *
 * @param orig The Evas_Map to duplicate.
 * @return A pointer to the newly created Evas_Map, or NULL on failure.
 */
static inline Evas_Map *
_evas_map_dup(const Evas_Map *orig)
{
   Evas_Map *copy = _evas_map_new(orig->count, EINA_FALSE);
   if (!copy) return NULL;
   memcpy(copy->points, orig->points, orig->count * sizeof(Evas_Map_Point));
   copy->smooth = orig->smooth;
   copy->alpha = orig->alpha;
   copy->move_sync = orig->move_sync;
   copy->persp = orig->persp;
   return copy;
}

/**
 * @internal
 * @brief Frees an Evas_Map structure and associated engine resources.
 * If an Evas_Object is provided, it cleans up engine-specific map data (spans)
 * associated with that object's map.
 *
 * @param eo_obj The Evas_Object associated with this map (can be NULL if map is not tied to an object).
 * @param m The Evas_Map to free.
 */
static inline void
_evas_map_free(Evas_Object *eo_obj, Evas_Map *m)
{
   if (eo_obj)
     {
        Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
        if ((obj) && (obj->map->spans))
          {
             obj->layer->evas->engine.func->image_map_clean(ENC, obj->map->spans);
             EINA_COW_WRITE_BEGIN(evas_object_map_cow, obj->map, Evas_Object_Map_Data, map_write)
               {
                  free(map_write->spans);
                  map_write->spans = NULL;
               }
             EINA_COW_WRITE_END(evas_object_map_cow, obj->map, map_write);
          }
     }
   m->magic = 0;
   free(m);
}

/**
 * @brief Converts canvas coordinates to map image UV coordinates.
 * Given a point (x, y) in canvas coordinates, this function calculates the
 * corresponding (u, v) coordinates within the map's source image.
 * This is effectively an inverse texture mapping.
 *
 * @param m The map to use for coordinate conversion.
 * @param x The x-coordinate on the canvas.
 * @param y The y-coordinate on the canvas.
 * @param mx Pointer to store the resulting u-coordinate in the map's image. Can be NULL.
 * @param my Pointer to store the resulting v-coordinate in the map's image. Can be NULL.
 * @param grab If true, and the point (x,y) is outside the map, the function
 *        may attempt to extrapolate coordinates. (Currently, this feature seems
 *        to have limitations or is not fully implemented as per FIXME).
 * @return EINA_TRUE if the point (x,y) is inside or on the edge of the map
 *         and coordinates were successfully calculated (or if mx/my are NULL).
 *         EINA_FALSE otherwise (e.g., map has less than 4 points, or point is
 *         outside and grab is false).
 *
 * @note The current implementation uses a scanline algorithm to find intersections
 *       and interpolate UV coordinates. It assumes the map is a convex quadrilateral
 *       for accurate interpolation.
 */
EVAS_API Eina_Bool
evas_map_coords_get(const Evas_Map *m, double x, double y,
                    double *mx, double *my, int grab)
{
   MAGIC_CHECK(m, Evas_Map, MAGIC_MAP);
   return EINA_FALSE;
   MAGIC_CHECK_END();

   if (m->count < 4) return EINA_FALSE;

   Eina_Bool inside = evas_map_inside_get(m, x, y);
   if ((!mx) && (!my)) return inside;

   // FIXME: need to handle grab mode and extrapolate coords outside map
   if (grab && !inside) return EINA_FALSE;

   int i, j, edges, edge[m->count][2];
   Eina_Bool douv = EINA_FALSE;
   double xe[2];
   double u[2] = { 0.0, 0.0 };
   double v[2] = { 0.0, 0.0 };

/*
   if (grab)
     {
        double ymin, ymax;

        ymin = m->points[0].y;
        ymax = m->points[0].y;
        for (i = 1; i < m->count; i++)
          {
             if (m->points[i].y < ymin) ymin = m->points[i].y;
             else if (m->points[i].y > ymax) ymax = m->points[i].y;
          }
        if (y <= ymin) y = ymin + 1;
        if (y >= ymax) y = ymax - 1;
     }
*/
   edges = EINA_FALSE;
   for (i = 0; i < m->count; i++)
     {
        j = (i + 1) % m->count;
        if ((m->points[i].y <= y) && (m->points[j].y > y))
          {
             edge[edges][0] = i;
             edge[edges][1] = j;
             edges++;
          }
        else if ((m->points[j].y <= y) && (m->points[i].y > y))
          {
             edge[edges][0] = j;
             edge[edges][1] = i;
             edges++;
          }
     }
   if ((mx) || (my)) douv = EINA_TRUE;
   for (i = 0; i < (edges - 1); i+= 2)
     {
        double yp, yd;

        j = i + 1;
        yd = m->points[edge[i][1]].y - m->points[edge[i][0]].y;
        if (yd > 0)
          {
             yp = y - m->points[edge[i][0]].y;
             xe[0] = m->points[edge[i][1]].x - m->points[edge[i][0]].x;
             xe[0] = m->points[edge[i][0]].x + ((xe[0] * yp) / yd);
             if (douv)
               {
                  u[0] = m->points[edge[i][1]].u - m->points[edge[i][0]].u;
                  u[0] = m->points[edge[i][0]].u + ((u[0] * yp) / yd);
                  v[0] = m->points[edge[i][1]].v - m->points[edge[i][0]].v;
                  v[0] = m->points[edge[i][0]].v + ((v[0] * yp) / yd);
               }
          }
        else
          {
             xe[0] = m->points[edge[i][0]].x;
             if (douv)
               {
                  u[0] = m->points[edge[i][0]].u;
                  v[0] = m->points[edge[i][0]].v;
               }
          }
        yd = m->points[edge[j][1]].y - m->points[edge[j][0]].y;
        if (yd > 0)
          {
             yp = y - m->points[edge[j][0]].y;
             xe[1] = m->points[edge[j][1]].x - m->points[edge[j][0]].x;
             xe[1] = m->points[edge[j][0]].x + ((xe[1] * yp) / yd);
             if (douv)
               {
                  u[1] = m->points[edge[j][1]].u - m->points[edge[j][0]].u;
                  u[1] = m->points[edge[j][0]].u + ((u[1] * yp) / yd);
                  v[1] = m->points[edge[j][1]].v - m->points[edge[j][0]].v;
                  v[1] = m->points[edge[j][0]].v + ((v[1] * yp) / yd);
               }
          }
        else
          {
             xe[1] = m->points[edge[j][0]].x;
             if (douv)
               {
                  u[1] = m->points[edge[j][0]].u;
                  v[1] = m->points[edge[j][0]].v;
               }
          }
        if (xe[0] > xe[1])
          {
             int ti;

             ti = xe[0]; xe[0] = xe[1]; xe[1] = ti;
             if (douv)
               {
                  double td;

                  td = u[0]; u[0] = u[1]; u[1] = td;
                  td = v[0]; v[0] = v[1]; v[1] = td;
               }
          }
        if ((x >= xe[0]) && (x < xe[1]))
          {
             if (douv)
               {
                  if (mx)
                    *mx = u[0] + (((x - xe[0]) * (u[1] - u[0])) /
                                  (xe[1] - xe[0]));
                  if (my)
                    *my = v[0] + (((x - xe[0]) * (v[1] - v[0])) /
                                  (xe[1] - xe[0]));
               }
             return EINA_TRUE;
          }
/*
		  if (grab)
          {
             if (douv)
               {
                  if (mx)
                    *mx = u[0] + (((x - xe[0]) * (u[1] - u[0])) /
                                  (xe[1] - xe[0]));
                  if (my)
                    *my = v[0] + (((x - xe[0]) * (v[1] - v[0])) /
                                  (xe[1] - xe[0]));
               }
             return EINA_TRUE;
          }
*/
     }
   return EINA_FALSE;
}

/**
 * @brief Checks if a given point (x, y) is inside the boundaries of a map.
 * This function uses the Ray Casting algorithm (Jordan curve theorem variant)
 * to determine if the point is inside the polygon defined by the map's points.
 *
 * @param m The map.
 * @param x The x-coordinate of the point to check.
 * @param y The y-coordinate of the point to check.
 * @return EINA_TRUE if the point is inside the map, EINA_FALSE otherwise.
 */
Eina_Bool
evas_map_inside_get(const Evas_Map *m, Evas_Coord x, Evas_Coord y)
{
   int i = 0, j = m->count - 1;
   double pt1_x, pt1_y, pt2_x, pt2_y, tmp_x;
   Eina_Bool inside = EINA_FALSE;

   //Check the point inside the map coords by using Jordan curve theorem.
   for (i = 0; i < m->count; i++)
     {
        pt1_x = m->points[i].x;
        pt1_y = m->points[i].y;
        pt2_x = m->points[j].x;
        pt2_y = m->points[j].y;

        //Is the point inside the map on y axis?
        if (((y >= pt1_y) && (y < pt2_y)) || ((y >= pt2_y) && (y < pt1_y)))
          {
             //Check the point is left side of the line segment.
             tmp_x = (pt1_x + ((pt2_x - pt1_x) / (pt2_y - pt1_y)) *
                      ((double)y - pt1_y));
             if ((double)x < tmp_x) inside = !inside;
          }
        j = i;
     }
   return inside;
}

#if 0
static Eina_Bool
_evas_object_map_parent_check(Evas_Object *eo_parent)
{
   const Eina_Inlist *list;
   const Evas_Object_Protected_Data *o;

   if (!eo_parent) return EINA_FALSE;
   Evas_Object_Protected_Data *parent = efl_data_scope_get(eo_parent, EFL_CANVAS_OBJECT_CLASS);
   if (!parent) return EINA_FALSE;
   list = evas_object_smart_members_get_direct(parent->smart.parent);
   EINA_INLIST_FOREACH(list, o)
     if (o->map->cur.usemap) break;
   if (o) return EINA_FALSE; /* Still some child have a map enable */
   parent->child_has_map = EINA_FALSE;
   _evas_object_map_parent_check(parent->smart.parent);
   return EINA_TRUE;
}
#endif

void
_evas_object_map_enable_set(Eo *eo_obj, Evas_Object_Protected_Data *obj,
                            Eina_Bool enabled)
{
   Eina_Bool pchange = EINA_FALSE;

   enabled = !!enabled;
   if (obj->map->cur.usemap == enabled) return;
   pchange = obj->changed;

   evas_object_async_block(obj);
   EINA_COW_WRITE_BEGIN(evas_object_map_cow, obj->map, Evas_Object_Map_Data, map_write)
     map_write->cur.usemap = enabled;
   EINA_COW_WRITE_END(evas_object_map_cow, obj->map, map_write);

   if (enabled)
     {
        if (!obj->map->cur.map)
          {
             EINA_COW_WRITE_BEGIN(evas_object_map_cow, obj->map, Evas_Object_Map_Data, map_write)
               map_write->cur.map = _evas_map_new(4, EINA_FALSE);
             EINA_COW_WRITE_END(evas_object_map_cow, obj->map, map_write);
          }
        evas_object_mapped_clip_across_mark(eo_obj, obj);
        //        obj->map->cur.map->normal_geometry = obj->cur->geometry;
     }
   else
     {
        if (obj->map->surface)
          {
             EINA_COW_WRITE_BEGIN(evas_object_map_cow, obj->map, Evas_Object_Map_Data, map_write)
               {
                  obj->layer->evas->engine.func->image_free(ENC, map_write->surface);
                  map_write->surface = NULL;
               }
             EINA_COW_WRITE_END(evas_object_map_cow, obj->map, map_write);
          }
        if (obj->map->cur.map)
          {
             _evas_map_calc_geom_change(eo_obj);
             evas_object_mapped_clip_across_mark(eo_obj, obj);
          }
     }
   _evas_map_calc_map_geometry(eo_obj);
   /* This is a bit heavy handed, but it fixes the case of same geometry, but
    * changed colour or UV settings. */
   evas_object_change(eo_obj, obj);
   if (!obj->changed_pchange) obj->changed_pchange = pchange;
   obj->changed_map = EINA_TRUE;

   if (enabled)
     {
        Evas_Object *eo_parents;
        Evas_Object_Protected_Data *parents = NULL;
        for (eo_parents = obj->smart.parent; eo_parents; eo_parents = parents->smart.parent)
          {
             parents = efl_data_scope_get(eo_parents, EFL_CANVAS_OBJECT_CLASS);
             if (!parents) break;
             parents->child_has_map = EINA_TRUE;
          }
        evas_object_update_bounding_box(eo_obj, obj, NULL);
     }
   else
     {
        evas_object_update_bounding_box(eo_obj, obj, NULL);
     }
}

EVAS_API void
evas_object_map_enable_set(Eo *eo_obj, Eina_Bool enabled)
{
   Evas_Object_Protected_Data *obj = EVAS_OBJ_GET_OR_RETURN(eo_obj);

   _evas_object_map_enable_set(eo_obj, obj, enabled);
}

/**
 * @brief Gets whether mapping is enabled for an Evas object.
 *
 * @param eo_obj The Evas object.
 * @return EINA_TRUE if mapping is enabled, EINA_FALSE otherwise.
 * @see evas_object_map_enable_set()
 */
EVAS_API Eina_Bool
evas_object_map_enable_get(const Eo *eo_obj)
{
   Evas_Object_Protected_Data *obj = EVAS_OBJ_GET_OR_RETURN(eo_obj, EINA_FALSE);

   return obj->map->cur.usemap;
}

EVAS_API void
evas_object_map_set(Evas_Object *eo_obj, const Evas_Map *map)
{
   Evas_Object_Protected_Data *obj = EVAS_OBJ_GET_OR_RETURN(eo_obj);

   evas_object_async_block(obj);

   // check if the new map and current map attributes are same
   if (map && obj->map->cur.map &&
       (obj->map->cur.map->alpha == map->alpha) &&
       (obj->map->cur.map->smooth == map->smooth) &&
       (obj->map->cur.map->move_sync.enabled == map->move_sync.enabled) &&
       (obj->map->cur.map->move_sync.diff_x == map->move_sync.diff_x) &&
       (obj->map->cur.map->move_sync.diff_y == map->move_sync.diff_y) &&
       (obj->map->cur.map->count == map->count))
     {
        const Evas_Map_Point *p1, *p2;
        p1 = obj->map->cur.map->points;
        p2 = map->points;
        if (!memcmp(p1, p2, sizeof(Evas_Map_Point) * map->count) &&
            !memcmp(&map->persp, &obj->map->cur.map->persp, sizeof(map->persp)))
          return;
     }
     /* changed_pchange means map's change.
      * This flag will be used to decide whether to redraw the map surface.
      * And value of flag would be EINA_FALSE after rendering. */
     obj->changed_pchange = EINA_TRUE;

   if ((!map) || (map->count < 4))
     {
        if (obj->map->surface)
          {
             EINA_COW_WRITE_BEGIN(evas_object_map_cow, obj->map, Evas_Object_Map_Data, map_write)
               {
                  obj->layer->evas->engine.func->image_free(ENC, map_write->surface);
                  map_write->surface = NULL;
               }
             EINA_COW_WRITE_END(evas_object_map_cow, obj->map, map_write);
          }
        if (obj->map->cur.map)
          {
             obj->changed_map = EINA_TRUE;

	     EINA_COW_STATE_WRITE_BEGIN(obj, state_write, prev)
	       {
		 state_write->geometry = obj->map->cur.map->normal_geometry;
	       }
	     EINA_COW_STATE_WRITE_END(obj, state_write, prev);

             EINA_COW_WRITE_BEGIN(evas_object_map_cow, obj->map, Evas_Object_Map_Data, map_write)
               {
                  if (map_write->prev.map == map_write->cur.map)
                    map_write->cur.map = NULL;
                  else if (!map_write->cache_map)
                    {
                       map_write->cache_map = map_write->cur.map;
                       map_write->cur.map = NULL;
                    }
                  else
                    {
                       _evas_map_free(eo_obj, map_write->cur.map);
                       map_write->cur.map = NULL;
                    }
               }
             EINA_COW_WRITE_END(evas_object_map_cow, obj->map, map_write);

             if (!obj->map->prev.map)
               {
                  evas_object_update_bounding_box(eo_obj, obj, NULL);
                  evas_object_mapped_clip_across_mark(eo_obj, obj);
                  return;
               }

             if (!obj->map->cur.usemap) _evas_map_calc_geom_change(eo_obj);
             else _evas_map_calc_map_geometry(eo_obj);
             if (obj->map->cur.usemap)
               evas_object_mapped_clip_across_mark(eo_obj, obj);
          }
        evas_object_update_bounding_box(eo_obj, obj, NULL);
        return;
     }

   if (obj->map->prev.map != NULL &&
       obj->map->prev.map == obj->map->cur.map)
     {
        EINA_COW_WRITE_BEGIN(evas_object_map_cow, obj->map, Evas_Object_Map_Data, map_write)
          map_write->cur.map = NULL;
        EINA_COW_WRITE_END(evas_object_map_cow, obj->map, map_write);
     }

   if (!obj->map->cur.map && obj->map->cache_map)
     {
        EINA_COW_WRITE_BEGIN(evas_object_map_cow, obj->map, Evas_Object_Map_Data, map_write)
          {
             map_write->cur.map = map_write->cache_map;
             map_write->cache_map = NULL;
          }
        EINA_COW_WRITE_END(evas_object_map_cow, obj->map, map_write);
     }

   // We do have the same exact count of point in this map, so just copy it
   if ((obj->map->cur.map) && (obj->map->cur.map->count == map->count))
     _evas_map_copy(obj->map->cur.map, map);
   else
     {
        if (obj->map->cur.map) _evas_map_free(eo_obj, obj->map->cur.map);
        EINA_COW_WRITE_BEGIN(evas_object_map_cow, obj->map, Evas_Object_Map_Data, map_write)
          map_write->cur.map = _evas_map_dup(map);
        EINA_COW_WRITE_END(evas_object_map_cow, obj->map, map_write);
        if (obj->map->cur.usemap)
          evas_object_mapped_clip_across_mark(eo_obj, obj);
     }

   evas_object_update_bounding_box(eo_obj, obj, NULL);
   _evas_map_calc_map_geometry(eo_obj);
}

EVAS_API const Evas_Map *
evas_object_map_get(const Evas_Object *eo_obj)
{
   Evas_Object_Protected_Data *obj = EVAS_OBJ_GET_OR_RETURN((Eo *) eo_obj, NULL);

   evas_object_async_block(obj);
   return obj->map->cur.map;
}

/**
 * @brief Creates a new map with a specified number of points.
 * The number of points must be a multiple of 4 and greater than 0.
 * Evas maps are typically defined by 4 points to map a rectangle, but
 * can have more points (e.g., 8, 12, ...) for more complex meshes.
 *
 * @param count The number of points for the map. Must be a positive multiple of 4.
 *              Example: 4 for a simple quadrilateral, 8 for a 2x1 grid of quads.
 * @return A new Evas_Map instance, or NULL on error (e.g., invalid count).
 * @see evas_map_free()
 * @see evas_object_map_set()
 */
EVAS_API Evas_Map *
evas_map_new(int count)
{
   if ((count <= 0) || (count % 4 != 0))
     {
        ERR("map point count (%i) should be multiples of 4!", count);
        return NULL;
     }

   return _evas_map_new(count, EINA_FALSE);
}

/**
 * @brief Sets whether smoothing is enabled for a map.
 * When smoothing is enabled, the rendering engine may apply anti-aliasing
 * or other filtering techniques to the mapped object for a smoother appearance.
 *
 * @param m The map.
 * @param enabled EINA_TRUE to enable smoothing, EINA_FALSE to disable.
 */
EVAS_API void
evas_map_smooth_set(Evas_Map *m, Eina_Bool enabled)
{
   MAGIC_CHECK(m, Evas_Map, MAGIC_MAP);
   return;
   MAGIC_CHECK_END();

   m->smooth = enabled;
}

/**
 * @brief Gets whether smoothing is enabled for a map.
 *
 * @param m The map.
 * @return EINA_TRUE if smoothing is enabled, EINA_FALSE otherwise.
 * @see evas_map_smooth_set()
 */
EVAS_API Eina_Bool
evas_map_smooth_get(const Evas_Map *m)
{
   MAGIC_CHECK(m, Evas_Map, MAGIC_MAP);
   return EINA_FALSE;
   MAGIC_CHECK_END();

   return m->smooth;
}

/**
 * @brief Sets whether alpha blending is enabled for a map.
 * If alpha is enabled, the per-point alpha values and the overall object alpha
 * will be considered during rendering, allowing for transparency effects.
 *
 * @param m The map.
 * @param enabled EINA_TRUE to enable alpha blending, EINA_FALSE to disable.
 */
EVAS_API void
evas_map_alpha_set(Evas_Map *m, Eina_Bool enabled)
{
   MAGIC_CHECK(m, Evas_Map, MAGIC_MAP);
   return;
   MAGIC_CHECK_END();

   m->alpha = enabled;
}

/**
 * @brief Gets whether alpha blending is enabled for a map.
 *
 * @param m The map.
 * @return EINA_TRUE if alpha blending is enabled, EINA_FALSE otherwise.
 * @see evas_map_alpha_set()
 */
EVAS_API Eina_Bool
evas_map_alpha_get(const Evas_Map *m)
{
   MAGIC_CHECK(m, Evas_Map, MAGIC_MAP);
   return EINA_FALSE;
   MAGIC_CHECK_END();

   return m->alpha;
}

/**
 * @brief Sets whether the map points should synchronize with object movements.
 * If enabled, when the object associated with this map is moved, the map points
 * will be adjusted internally to maintain their position relative to the object's
 * content, rather than staying fixed in canvas space. This is useful if the map
 * defines a transformation that should "stick" to the object as it moves.
 * If disabled, any accumulated movement difference is reset.
 *
 * @param m The map.
 * @param enabled EINA_TRUE to enable move synchronization, EINA_FALSE to disable.
 */
EVAS_API void
evas_map_util_object_move_sync_set(Evas_Map *m, Eina_Bool enabled)
{
   MAGIC_CHECK(m, Evas_Map, MAGIC_MAP);
   return;
   MAGIC_CHECK_END();

   if (!enabled)
     {
        m->move_sync.diff_x = 0;
        m->move_sync.diff_y = 0;
     }
   m->move_sync.enabled = !!enabled;
}

/**
 * @brief Gets whether map points synchronization with object movements is enabled.
 *
 * @param m The map.
 * @return EINA_TRUE if move synchronization is enabled, EINA_FALSE otherwise.
 * @see evas_map_util_object_move_sync_set()
 */
EVAS_API Eina_Bool
evas_map_util_object_move_sync_get(const Evas_Map *m)
{
   MAGIC_CHECK(m, Evas_Map, MAGIC_MAP);
   return EINA_FALSE;
   MAGIC_CHECK_END();

   return m->move_sync.enabled;
}

/**
 * @brief Duplicates an existing map.
 * Creates a new Evas_Map and copies all data (points, properties) from the
 * source map. The new map is independent of the original.
 *
 * @param m The map to duplicate.
 * @return A new Evas_Map instance which is a copy of @p m, or NULL on error.
 * @see evas_map_free()
 */
EVAS_API Evas_Map *
evas_map_dup(const Evas_Map *m)
{
   MAGIC_CHECK(m, Evas_Map, MAGIC_MAP);
   return NULL;
   MAGIC_CHECK_END();

   return _evas_map_dup(m);
}

/**
 * @brief Frees an Evas_Map instance.
 * If the map was created with evas_map_new() and not set on an object,
 * or if it's a map retrieved and then duplicated, it should be freed with this
 * function to release its memory.
 * Do not free a map currently set on an object if you haven't duplicated it first;
 * the object owns its current map.
 *
 * @param m The map to free.
 */
EVAS_API void
evas_map_free(Evas_Map *m)
{
   if (!m) return;
   _evas_map_free(NULL, m);
}

/**
 * @brief Gets the number of points in a map.
 *
 * @param m The map.
 * @return The number of points in the map, or -1 on error (e.g., m is NULL).
 */
EVAS_API int
evas_map_count_get(const Evas_Map *m)
{
   MAGIC_CHECK(m, Evas_Map, MAGIC_MAP);
   return -1;
   MAGIC_CHECK_END();

   return m->count;
}

/**
 * @brief Sets the world coordinates (x, y, z) for a specific point in the map.
 * These coordinates define the position of the map point in 3D space before
 * any perspective transformation is applied.
 * The Evas_Coord type is typically an integer, but the underlying storage and
 * calculations might use floating-point numbers for precision.
 *
 * @param m The map.
 * @param idx The index of the point to modify (0 to count-1).
 * @param x The x-coordinate.
 * @param y The y-coordinate.
 * @param z The z-coordinate (depth).
 */
EVAS_API void
evas_map_point_coord_set(Evas_Map *m, int idx, Evas_Coord x, Evas_Coord y, Evas_Coord z)
{
   MAGIC_CHECK(m, Evas_Map, MAGIC_MAP);
   return;
   MAGIC_CHECK_END();

   _map_point_coord_set(m, idx, x, y, z);
}

/**
 * @brief Gets the world coordinates (x, y, z) for a specific point in the map.
 * Retrieves the coordinates set by evas_map_point_coord_set().
 * Values are returned as Evas_Coord (typically int), rounded from internal double precision.
 *
 * @param m The map.
 * @param idx The index of the point to query (0 to count-1).
 * @param x Pointer to store the x-coordinate. Can be NULL.
 * @param y Pointer to store the y-coordinate. Can be NULL.
 * @param z Pointer to store the z-coordinate. Can be NULL.
 */
EVAS_API void
evas_map_point_coord_get(const Evas_Map *m, int idx, Evas_Coord *x, Evas_Coord *y, Evas_Coord *z)
{
   double dx, dy, dz;

   _map_point_coord_get(m, idx, &dx, &dy, &dz);
   if (x) *x = lround(dx);
   if (y) *y = lround(dy);
   if (z) *z = lround(dz);
}

/**
 * @brief Sets the texture mapping coordinates (u, v) for a specific point in the map.
 * These coordinates define which part of the source image/surface is mapped to this point.
 * (0,0) typically refers to the top-left corner of the source image, and
 * (source_width, source_height) to the bottom-right. However, these can be
 * any values to select a portion of the image or to tile/stretch it.
 *
 * @param m The map.
 * @param idx The index of the point to modify (0 to count-1).
 * @param u The u-coordinate (horizontal texture coordinate).
 * @param v The v-coordinate (vertical texture coordinate).
 *          Example: For a point that should map to the center of a 100x100 image,
 *                   u would be 50.0 and v would be 50.0.
 */
EVAS_API void
evas_map_point_image_uv_set(Evas_Map *m, int idx, double u, double v)
{
   MAGIC_CHECK(m, Evas_Map, MAGIC_MAP);
   return;
   MAGIC_CHECK_END();

   Evas_Map_Point *p;

   if ((idx < 0) || (idx >= m->count)) return;
   p = m->points + idx;
   p->u = u;
   p->v = v;
}

/**
 * @brief Gets the texture mapping coordinates (u, v) for a specific point in the map.
 *
 * @param m The map.
 * @param idx The index of the point to query (0 to count-1).
 * @param u Pointer to store the u-coordinate. Can be NULL.
 * @param v Pointer to store the v-coordinate. Can be NULL.
 */
EVAS_API void
evas_map_point_image_uv_get(const Evas_Map *m, int idx, double *u, double *v)
{
   MAGIC_CHECK(m, Evas_Map, MAGIC_MAP);
   goto error;
   MAGIC_CHECK_END();

   const Evas_Map_Point *p;

   if ((idx < 0) || (idx >= m->count)) goto error;
   p = m->points + idx;
   if (u) *u = p->u;
   if (v) *v = p->v;
   return;

 error:
   if (u) *u = 0.0;
   if (v) *v = 0.0;
}

/**
 * @brief Sets the color for a specific point in the map.
 * This color is multiplied with the source image's color at that point.
 * Values range from 0 to 255.
 *
 * @param m The map.
 * @param idx The index of the point to modify (0 to count-1).
 * @param r Red component (0-255).
 * @param g Green component (0-255).
 * @param b Blue component (0-255).
 * @param a Alpha component (0-255).
 */
EVAS_API void
evas_map_point_color_set(Evas_Map *m, int idx, int r, int g, int b, int a)
{
   MAGIC_CHECK(m, Evas_Map, MAGIC_MAP);
   return;
   MAGIC_CHECK_END();

   Evas_Map_Point *p;

   if ((idx < 0) || (idx >= m->count)) return;
   p = m->points + idx;
   p->r = r;
   p->g = g;
   p->b = b;
   p->a = a;
}

/**
 * @brief Gets the color for a specific point in the map.
 *
 * @param m The map.
 * @param idx The index of the point to query (0 to count-1).
 * @param r Pointer to store the red component. Can be NULL.
 * @param g Pointer to store the green component. Can be NULL.
 * @param b Pointer to store the blue component. Can be NULL.
 * @param a Pointer to store the alpha component. Can be NULL.
 */
EVAS_API void
evas_map_point_color_get(const Evas_Map *m, int idx, int *r, int *g, int *b, int *a)
{
   MAGIC_CHECK(m, Evas_Map, MAGIC_MAP);
   return;
   MAGIC_CHECK_END();

   const Evas_Map_Point *p;

   if ((idx < 0) || (idx >= m->count)) goto error;
   p = m->points + idx;
   if (r) *r = p->r;
   if (g) *g = p->g;
   if (b) *b = p->b;
   if (a) *a = p->a;
   return;

error:
   if (r) *r = 255;
   if (g) *g = 255;
   if (b) *b = 255;
   if (a) *a = 255;
}

/**
 * @brief Populates the points of a 4-point map from an object's geometry, with a specified Z value.
 * This is a utility function to quickly set up a map to match an object's
 * current position and size (x, y, w, h) at a given depth z.
 * The map's points will be:
 * - Point 0: (obj.x, obj.y, z) with UV (0, 0)
 * - Point 1: (obj.x + obj.w, obj.y, z) with UV (obj.w, 0)
 * - Point 2: (obj.x + obj.w, obj.y + obj.h, z) with UV (obj.w, obj.h)
 * - Point 3: (obj.x, obj.y + obj.h, z) with UV (0, obj.h)
 *
 * @warning This function requires the map @p m to have exactly 4 points.
 *
 * @param m The 4-point map to populate.
 * @param eo_obj The Evas object whose geometry will be used.
 * @param z The z-coordinate to set for all 4 points.
 */
EVAS_API void
evas_map_util_points_populate_from_object_full(Evas_Map *m, const Evas_Object *eo_obj, Evas_Coord z)
{
   MAGIC_CHECK(m, Evas_Map, MAGIC_MAP);
   return;
   MAGIC_CHECK_END();

   MAGIC_CHECK(eo_obj, Evas_Object, MAGIC_OBJ);
   return;
   MAGIC_CHECK_END();
   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);

   if (!obj) return;
   if (m->count != 4)
     {
        ERR("map has count=%d where 4 was expected.", m->count);
        return;
     }
   _evas_map_util_points_populate(m, obj->cur->geometry.x, obj->cur->geometry.y,
                                  obj->cur->geometry.w, obj->cur->geometry.h, z);
}

/**
 * @brief Populates the points of a 4-point map from an object's geometry, with Z=0.
 * This is a convenience function, equivalent to calling
 * evas_map_util_points_populate_from_object_full() with z = 0.
 *
 * @warning This function requires the map @p m to have exactly 4 points.
 *
 * @param m The 4-point map to populate.
 * @param eo_obj The Evas object whose geometry will be used.
 * @see evas_map_util_points_populate_from_object_full()
 */
EVAS_API void
evas_map_util_points_populate_from_object(Evas_Map *m, const Evas_Object *eo_obj)
{
   MAGIC_CHECK(m, Evas_Map, MAGIC_MAP);
   return;
   MAGIC_CHECK_END();

   MAGIC_CHECK(eo_obj, Evas_Object, MAGIC_OBJ);
   return;
   MAGIC_CHECK_END();
   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);

   if (!obj) return;
   if (m->count != 4)
     {
        ERR("map has count=%d where 4 was expected.", m->count);
        return;
     }
   _evas_map_util_points_populate(m, obj->cur->geometry.x, obj->cur->geometry.y,
                                  obj->cur->geometry.w, obj->cur->geometry.h, 0);
}

/**
 * @brief Populates the points of a 4-point map from a given geometry.
 * This utility function sets up a map to cover a rectangle defined by
 * (x, y, w, h) at a depth z.
 * The map's points will be:
 * - Point 0: (x, y, z) with UV (0, 0)
 * - Point 1: (x + w, y, z) with UV (w, 0)
 * - Point 2: (x + w, y + h, z) with UV (w, h)
 * - Point 3: (x, y + h, z) with UV (0, h)
 *
 * @warning This function requires the map @p m to have exactly 4 points.
 *
 * @param m The 4-point map to populate.
 * @param x The x-coordinate of the rectangle.
 * @param y The y-coordinate of the rectangle.
 * @param w The width of the rectangle.
 * @param h The height of the rectangle.
 * @param z The z-coordinate (depth) for all points.
 */
EVAS_API void
evas_map_util_points_populate_from_geometry(Evas_Map *m, Evas_Coord x, Evas_Coord y, Evas_Coord w, Evas_Coord h, Evas_Coord z)
{
   MAGIC_CHECK(m, Evas_Map, MAGIC_MAP);
   return;
   MAGIC_CHECK_END();

   if (m->count != 4)
     {
        ERR("map has count=%d where 4 was expected.", m->count);
        return;
     }
   _evas_map_util_points_populate(m, x, y, w, h, z);
}

/**
 * @brief Sets the color for all points in the map.
 * This is a utility function to apply a uniform color to every point in the map.
 *
 * @param m The map.
 * @param r Red component (0-255).
 * @param g Green component (0-255).
 * @param b Blue component (0-255).
 * @param a Alpha component (0-255).
 */
EVAS_API void
evas_map_util_points_color_set(Evas_Map *m, int r, int g, int b, int a)
{
   MAGIC_CHECK(m, Evas_Map, MAGIC_MAP);
   return;
   MAGIC_CHECK_END();

   Evas_Map_Point *p, *p_end;

   p = m->points;
   p_end = p + m->count;
   for (; p < p_end; p++)
     {
        p->r = r;
        p->g = g;
        p->b = b;
        p->a = a;
     }
}

void
_map_util_rotate(Evas_Map *m, double degrees, double cx, double cy)
{
   double r = (degrees * M_PI) / 180.0;
   Evas_Map_Point *p, *p_end;

   p = m->points;
   p_end = p + m->count;

   for (; p < p_end; p++)
     {
        double x, y, xx, yy;

        x = p->x - cx;
        y = p->y - cy;

        xx = x * cos(r);
        yy = x * sin(r);
        x = xx - (y * sin(r));
        y = yy + (y * cos(r));

        p->px = p->x = x + cx;
        p->py = p->y = y + cy;
     }
}

EVAS_API void
evas_map_util_rotate(Evas_Map *m, double degrees, Evas_Coord cx, Evas_Coord cy)
{
   MAGIC_CHECK(m, Evas_Map, MAGIC_MAP);
   return;
   MAGIC_CHECK_END();

   _map_util_rotate(m, degrees, (double) cx, (double) cy);
}

/**
 * @internal
 * @brief Zooms the map points relative to a center point (cx, cy) using double precision.
 * This is an internal helper used by the public Evas_Coord version.
 *
 * @param m The Evas_Map to zoom.
 * @param zoomx Zoom factor for the X-axis.
 * @param zoomy Zoom factor for the Y-axis.
 * @param cx X-coordinate of the zoom center.
 * @param cy Y-coordinate of the zoom center.
 */
void
_map_util_zoom(Evas_Map *m, double zoomx, double zoomy, double cx, double cy)
{
   Evas_Map_Point *p, *p_end;

   p = m->points;
   p_end = p + m->count;

   for (; p < p_end; p++)
     {
        double x, y;

        x = p->x - cx;
        y = p->y - cy;

        x *= zoomx;
        y *= zoomy;

        p->px = p->x = x + cx;
        p->py = p->y = y + cy;
     }
}

EVAS_API void
evas_map_util_zoom(Evas_Map *m, double zoomx, double zoomy, Evas_Coord cx, Evas_Coord cy)
{
   MAGIC_CHECK(m, Evas_Map, MAGIC_MAP);
   return;
   MAGIC_CHECK_END();

   _map_util_zoom(m, zoomx, zoomy, (double) cx, (double) cy);
}

/**
 * @internal
 * @brief Translates all points in the map by (dx, dy, dz) using double precision.
 * This is an internal helper.
 *
 * @param m The Evas_Map to translate.
 * @param dx Translation amount for the X-axis.
 * @param dy Translation amount for the Y-axis.
 * @param dz Translation amount for the Z-axis.
 */
void
_map_util_translate(Evas_Map *m, double dx, double dy, double dz)
{
   Evas_Map_Point *p, *p_end;

   p = m->points;
   p_end = p + m->count;

   for (; p < p_end; p++)
     {
        p->px = (p->x += dx);
        p->py = (p->y += dy);
        p->z += dz;
     }
}

void
_map_util_3d_rotate(Evas_Map *m, double dx, double dy, double dz,
                    double cx, double cy, double cz)
{
   double rz = (dz * M_PI) / 180.0;
   double rx = (dx * M_PI) / 180.0;
   double ry = (dy * M_PI) / 180.0;
   Evas_Map_Point *p, *p_end;

   p = m->points;
   p_end = p + m->count;

   for (; p < p_end; p++)
     {
        double x, y, z, xx, yy, zz;

        x = p->x - cx;
        y = p->y - cy;
        z = p->z - cz;

        if (!EINA_DBL_EQ(rz, 0.0))
          {
             xx = x * cos(rz);
             yy = x * sin(rz);
             x = xx - (y * sin(rz));
             y = yy + (y * cos(rz));
          }

        if (!EINA_DBL_EQ(ry, 0.0))
          {
             xx = x * cos(ry);
             zz = x * sin(ry);
             x = xx - (z * sin(ry));
             z = zz + (z * cos(ry));
          }

        if (!EINA_DBL_EQ(rx, 0.0))
          {
             zz = z * cos(rx);
             yy = z * sin(rx);
             z = zz - (y * sin(rx));
             y = yy + (y * cos(rx));
          }

        p->px = p->x = x + cx;
        p->py = p->y = y + cy;
        p->z = z + cz;
     }
}

EVAS_API void
evas_map_util_3d_rotate(Evas_Map *m, double dx, double dy, double dz,
                        Evas_Coord cx, Evas_Coord cy, Evas_Coord cz)
{
   MAGIC_CHECK(m, Evas_Map, MAGIC_MAP);
   return;
   MAGIC_CHECK_END();

   _map_util_3d_rotate(m, dx, dy, dz, (double) cx, (double) cy, (double) cz);
}

/**
 * @internal
 * @brief Rotates map points using a quaternion around a center point (cx, cy, cz)
 * using double precision coordinates. This is an internal helper.
 *
 * @param m The Evas_Map to rotate.
 * @param qx X component of the quaternion.
 * @param qy Y component of the quaternion.
 * @param qz Z component of the quaternion.
 * @param qw W component of the quaternion.
 * @param cx X-coordinate of the rotation center.
 * @param cy Y-coordinate of the rotation center.
 * @param cz Z-coordinate of the rotation center.
 */
void
_map_util_quat_rotate(Evas_Map *m, double qx, double qy, double qz,
                      double qw, double cx, double cy, double cz)
{
   Eina_Quaternion q;
   Eina_Point_3D c;

   MAGIC_CHECK(m, Evas_Map, MAGIC_MAP);
   return;
   MAGIC_CHECK_END();

   Evas_Map_Point *p, *p_end;

   p = m->points;
   p_end = p + m->count;

   q.x = qx;
   q.y = qy;
   q.z = qz;
   q.w = qw;

   c.x = cx;
   c.y = cy;
   c.z = cz;

   for (; p < p_end; p++)
     {
        Eina_Point_3D current;

        current.x = p->x;
        current.y = p->y;
        current.z = p->z;

        eina_quaternion_rotate(&current, &c, &q);

        p->px = p->x = current.x;
        p->py = p->y = current.y;
        p->z = current.z;
     }
}

EVAS_API void
evas_map_util_quat_rotate(Evas_Map *m, double qx, double qy, double qz,
                          double qw, double cx, double cy, double cz)
{
   MAGIC_CHECK(m, Evas_Map, MAGIC_MAP);
   return;
   MAGIC_CHECK_END();

   _map_util_quat_rotate(m, qx, qy, qz, qw, cx, cy, cz);
}

/**
 * @internal
 * @brief Applies 3D lighting effects to the map points using double precision coordinates for light position.
 * This is an internal helper. It calculates a simple diffuse lighting model.
 * For each point, it computes a normal (assuming points form quads),
 * then calculates the dot product with the light vector to determine brightness.
 * The final color is a mix of ambient and light color based on this brightness.
 *
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
void
_map_util_3d_lighting(Evas_Map *m,
                      double lx, double ly, double lz,
                      int lr, int lg, int lb, int ar, int ag, int ab)
{
   int i;

   for (i = 0; i < m->count; i++)
     {
        double x, y, z;
        double nx, ny, nz, x1, yy1, z1, x2, yy2, z2, ln, br;
        int h, j, mr, mg, mb;

        x = m->points[i].x;
        y = m->points[i].y;
        z = m->points[i].z;
        // calc normal
        h = (i - 1 + 4) % 4 + (i & ~0x3); // prev point
        j = (i + 1)     % 4 + (i & ~0x3); // next point

        x1 = m->points[h].x - x;
        yy1 = m->points[h].y - y;
        z1 = m->points[h].z - z;

        x2 = m->points[j].x - x;
        yy2 = m->points[j].y - y;
        z2 = m->points[j].z - z;
        nx = (yy1 * z2) - (z1 * yy2);
        ny = (z1 * x2) - (x1 * z2);
        nz = (x1 * yy2) - (yy1 * x2);

        ln = (nx * nx) + (ny * ny) + (nz * nz);
        ln = sqrt(ln);

        if (!EINA_DBL_EQ(ln, 0.0))
          {
             nx /= ln;
             ny /= ln;
             nz /= ln;
          }

        // calc point -> light vector
        x = lx - x;
        y = ly - y;
        z = lz - z;

        ln = (x * x) + (y * y) + (z * z);
        ln = sqrt(ln);

        if (!EINA_DBL_EQ(ln, 0.0))
          {
             x /= ln;
             y /= ln;
             z /= ln;
          }

        // brightness - tan (0.0 -> 1.0 brightness really)
        br = (nx * x) + (ny * y) + (nz * z);
        if (br < 0.0) br = 0.0;

        mr = ar + ((lr - ar) * br);
        mg = ag + ((lg - ag) * br);
        mb = ab + ((lb - ab) * br);
        if (m->points[i].a != 255)
          {
             mr = (mr * m->points[i].a) / 255;
             mg = (mg * m->points[i].a) / 255;
             mb = (mb * m->points[i].a) / 255;
          }
        m->points[i].r = (m->points[i].r * mr) / 255;
        m->points[i].g = (m->points[i].g * mg) / 255;
        m->points[i].b = (m->points[i].b * mb) / 255;
     }
}

EVAS_API void
evas_map_util_3d_lighting(Evas_Map *m,
                          Evas_Coord lx, Evas_Coord ly, Evas_Coord lz,
                          int lr, int lg, int lb, int ar, int ag, int ab)
{
   MAGIC_CHECK(m, Evas_Map, MAGIC_MAP);
   return;
   MAGIC_CHECK_END();

   _map_util_3d_lighting(m, (double) lx, (double) ly, (double)
                         lz, lr, lg, lb, ar, ag, ab);
}

/**
 * @internal
 * @brief Applies a 3D perspective transformation to the map points using double precision.
 * This is an internal helper. It projects the 3D points onto a 2D plane.
 * The perspective parameters (px, py, z0, foc) are stored in the map.
 *
 * @param m The Evas_Map to transform.
 * @param px X-coordinate of the perspective point (vanishing point X on the projection plane).
 * @param py Y-coordinate of the perspective point (vanishing point Y on the projection plane).
 * @param z0 Z-coordinate of the Z=0 plane (distance from viewer to the Z=0 plane, typically 0 if px,py is screen center).
 * @param foc Focal length (distance from the viewer to the projection plane).
 *            A larger focal length results in less perspective distortion.
 */
void
_map_util_3d_perspective(Evas_Map *m, double px, double py, double z0, double foc)
{
   Evas_Map_Point *p, *p_end;

   p = m->points;
   p_end = p + m->count;

   m->persp.px = px;
   m->persp.py = py;
   m->persp.z0 = z0;
   m->persp.foc = foc;

   if (foc <= 0) return;

   for (; p < p_end; p++)
     {
        double x, y, zz;

        x = p->x - px;
        y = p->y - py;

        zz = ((p->z - z0) + foc);

        if (zz > 0)
          {
             x = (x * foc) / zz;
             y = (y * foc) / zz;
          }

        p->x = px + x;
        p->y = py + y;
     }
}

EVAS_API void
evas_map_util_3d_perspective(Evas_Map *m,
                             Evas_Coord px, Evas_Coord py,
                             Evas_Coord z0, Evas_Coord foc)
{
   MAGIC_CHECK(m, Evas_Map, MAGIC_MAP);
   return;
   MAGIC_CHECK_END();

   _map_util_3d_perspective(m, (double) px, (double) py, (double) z0, (double) foc);
}

/**
 * @brief Determines if the map points are defined in a clockwise order.
 * This is useful for back-face culling or lighting calculations.
 * It calculates the sum of signed areas of triangles formed by consecutive triplets
 * of vertices (shoelace formula variant). A positive sum typically indicates
 * clockwise winding order for a coordinate system where Y increases downwards.
 *
 * @param m The map.
 * @return EINA_TRUE if the points are in clockwise order, EINA_FALSE otherwise or if count < 3.
 */
EVAS_API Eina_Bool
evas_map_util_clockwise_get(Evas_Map *m)
{
   MAGIC_CHECK(m, Evas_Map, MAGIC_MAP);
   return EINA_FALSE;
   MAGIC_CHECK_END();

   int i, j, k, count;
   long long c;

   if (m->count < 3) return EINA_FALSE;

   count = 0;
   for (i = 0; i < m->count; i++)
     {
        j = (i + 1) % m->count;
        k = (i + 2) % m->count;
        c =
          ((m->points[j].x - m->points[i].x) *
           (m->points[k].y - m->points[j].y))
          -
          ((m->points[j].y - m->points[i].y) *
           (m->points[k].x - m->points[j].x));
        if (c < 0) count--;
        else if (c > 0) count++;
     }
   if (count > 0) return EINA_TRUE;
   return EINA_FALSE;
}

/****************************************************************************/
/* If the return value is true, the map surface should be redrawn.          */
/****************************************************************************/
/**
 * @internal
 * @brief Updates the rendering data (spans) for a mapped object.
 * This function is called by the rendering pipeline to prepare the map data
 * for the engine. It converts Evas_Map_Point data into RGBA_Map_Point data,
 * applying offsets, scaling UV coordinates, and handling perspective.
 *
 * It checks if the map data or related parameters (object position, image size)
 * have changed to determine if an update is necessary.
 *
 * @param eo_obj The Evas object being updated.
 * @param x The current x-offset of the object on the canvas.
 * @param y The current y-offset of the object on the canvas.
 * @param imagew The width of the source image/surface being mapped.
 * @param imageh The height of the source image/surface being mapped.
 * @param uvw The width dimension used for UV coordinate normalization (often same as imagew).
 * @param uvh The height dimension used for UV coordinate normalization (often same as imageh).
 * @return EINA_TRUE if the map surface should be redrawn (due to pchange flag),
 *         EINA_FALSE otherwise. Note that even if EINA_FALSE is returned,
 *         the span data might have been updated if obj->changed_map was true.
 */
Eina_Bool
evas_object_map_update(Evas_Object *eo_obj,
                       int x, int y,
                       int imagew, int imageh,
                       int uvw, int uvh)
{
   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   const Evas_Map_Point *p, *p_end;
   RGBA_Map_Point *pts, *pt;

   if (!obj) return EINA_FALSE;
   if (obj->map->spans)
     {
        if (obj->map->spans->x != x || obj->map->spans->y != y ||
            obj->map->spans->image.w != imagew || obj->map->spans->image.h != imageh ||
            obj->map->spans->uv.w != uvw || obj->map->spans->uv.h != uvh)
          obj->changed_map = EINA_TRUE;
     }
   else
     {
        obj->changed_map = EINA_TRUE;
     }

   evas_object_map_move_sync(eo_obj);

   if (!obj->changed_map) return EINA_FALSE;

   if (obj->map->spans && obj->map->cur.map->count != obj->map->spans->count)
     {
        EINA_COW_WRITE_BEGIN(evas_object_map_cow, obj->map, Evas_Object_Map_Data, map_write)
          {
             if (map_write->spans)
               {
                  // Destroy engine side spans
                  free(map_write->spans);
               }
             map_write->spans = NULL;
          }
        EINA_COW_WRITE_END(evas_object_map_cow, obj->map, map_write);
     }

   if (!obj->map->spans)
     {
        EINA_COW_WRITE_BEGIN(evas_object_map_cow, obj->map, Evas_Object_Map_Data, map_write)
          map_write->spans = calloc(1, sizeof (RGBA_Map) +
                                    sizeof (RGBA_Map_Point) * (map_write->cur.map->count - 1));
        EINA_COW_WRITE_END(evas_object_map_cow, obj->map, map_write);
     }

   if (!obj->map->spans) return EINA_FALSE;

   EINA_COW_WRITE_BEGIN(evas_object_map_cow, obj->map, Evas_Object_Map_Data, map_write)
     {
        map_write->spans->count = obj->map->cur.map->count;
        map_write->spans->x = x;
        map_write->spans->y = y;
        map_write->spans->uv.w = uvw;
        map_write->spans->uv.h = uvh;
        map_write->spans->image.w = imagew;
        map_write->spans->image.h = imageh;

        pts = obj->map->spans->pts;

        p = obj->map->cur.map->points;
        p_end = p + obj->map->cur.map->count;
        pt = pts;
     }
   EINA_COW_WRITE_END(evas_object_map_cow, obj->map, map_write);

   pts[0].px = obj->map->cur.map->persp.px << FP;
   pts[0].py = obj->map->cur.map->persp.py << FP;
   pts[0].foc = obj->map->cur.map->persp.foc << FP;
   pts[0].z0 = obj->map->cur.map->persp.z0 << FP;

   double uscale = 1.0, vscale = 1.0;
   if (obj->is_image_object)
     {
        _evas_image_proxy_source_scale_get(eo_obj, &uscale, &vscale);
     }
   // draw geom +x +y
   for (; p < p_end; p++, pt++)
     {
        pt->x = (lround(p->x) + x) * FP1;
        pt->y = (lround(p->y) + y) * FP1;
        pt->z = (lround(p->z)    ) * FP1;
        pt->fx = p->x + (float) x;
        pt->fy = p->y + (float) y;
        pt->fz = p->z;
        if ((uvw == 0) || (imagew == 0)) pt->u = 0;
        else pt->u = ((lround(p->u * uscale) * imagew) / uvw) * FP1;
        if ((uvh == 0) || (imageh == 0)) pt->v = 0;
        else pt->v = ((lround(p->v * vscale) * imageh) / uvh) * FP1;
        if      (pt->u < 0) pt->u = 0;
        else if (pt->u > (imagew * FP1)) pt->u = (imagew * FP1);
        if      (pt->v < 0) pt->v = 0;
        else if (pt->v > (imageh * FP1)) pt->v = (imageh * FP1);
        pt->col = ARGB_JOIN(p->a, p->r, p->g, p->b);
     }
   if (obj->map->cur.map->count & 0x1)
     {
        pts[obj->map->cur.map->count] = pts[obj->map->cur.map->count -1];
     }

   // Request engine to update it's point

   obj->changed_map = EINA_FALSE;

   return obj->changed_pchange;
}

/**
 * @internal
 * @brief Accumulates movement differences for a map with move_sync enabled.
 * When an object with a synchronized map is moved, instead of directly
 * transforming the map points, the difference in movement is stored.
 * This difference is then applied by evas_object_map_move_sync() before rendering.
 *
 * @param m The map.
 * @param diff_x The change in x-coordinate.
 * @param diff_y The change in y-coordinate.
 */
void
evas_map_object_move_diff_set(Evas_Map *m,
                              Evas_Coord diff_x,
                              Evas_Coord diff_y)
{
   MAGIC_CHECK(m, Evas_Map, MAGIC_MAP);
   return;
   MAGIC_CHECK_END();

   m->move_sync.diff_x += diff_x;
   m->move_sync.diff_y += diff_y;
}
