#include "evas_map.h"

// FIXME: cur vs. prev is not handled (may be an issue?)
// FIXME: some render artifacts appear when this API is used (green pixels)

#define EINA_INLIST_REMOVE(l,i) do { l = (__typeof__(l)) eina_inlist_remove(EINA_INLIST_GET(l), EINA_INLIST_GET(i)); } while (0)
#define EINA_INLIST_APPEND(l,i) do { l = (__typeof__(l)) eina_inlist_append(EINA_INLIST_GET(l), EINA_INLIST_GET(i)); } while (0)
#define EINA_INLIST_PREPEND(l,i) do { l = (__typeof__(l)) eina_inlist_prepend(EINA_INLIST_GET(l), EINA_INLIST_GET(i)); } while (0)
#define EINA_INLIST_NEXT(l) (typeof(l)) EINA_INLIST_CONTAINER_GET(EINA_INLIST_GET(l)->next, typeof(*l))

#define MY_CLASS EFL_GFX_MAPPING_MIXIN

/** @internal Convenience typedef for Gfx_Map structure. */
typedef struct _Gfx_Map               Gfx_Map;
/** @internal Convenience typedef for Gfx_Map_Op structure. */
typedef struct _Gfx_Map_Op            Gfx_Map_Op;
/** @internal Convenience typedef for Gfx_Map_Pivot structure. */
typedef struct _Gfx_Map_Pivot         Gfx_Map_Pivot;
/** @internal Convenience typedef for Efl_Gfx_Mapping_Data structure. */
typedef struct _Efl_Gfx_Mapping_Data  Efl_Gfx_Mapping_Data;
/** @internal Convenience typedef for Gfx_Map_Point structure. */
typedef struct _Gfx_Map_Point         Gfx_Map_Point;
/** @internal Convenience typedef for Gfx_Map_Op_Type enum. */
typedef enum _Gfx_Map_Op_Type         Gfx_Map_Op_Type;

/**
 * @internal
 * @brief Enumerates the types of graphics mapping operations.
 *
 * This enum defines the different operations that can be applied to a graphics map,
 * such as setting coordinates, colors, rotations, etc.
 */
enum _Gfx_Map_Op_Type {
   GFX_MAPPING_RAW_COORD,      /**< Directly set point coordinates. */
   GFX_MAPPING_COLOR,          /**< Set color for points. */
   GFX_MAPPING_ROTATE_2D,      /**< Apply a 2D rotation. */
   GFX_MAPPING_ROTATE_3D,      /**< Apply a 3D rotation. */
   GFX_MAPPING_ROTATE_QUAT,    /**< Apply a quaternion rotation. */
   GFX_MAPPING_ZOOM,           /**< Apply zoom. */
   GFX_MAPPING_TRANSLATE,      /**< Apply translation. */
   GFX_MAPPING_LIGHTING_3D,    /**< Apply 3D lighting. */
   GFX_MAPPING_PERSPECTIVE_3D, /**< Apply 3D perspective. */
};

/**
 * @internal
 * @brief Represents a single operation in a graphics map.
 *
 * This structure holds the type of operation and its parameters.
 * Operations are stored in an Eina_Inlist.
 */
struct _Gfx_Map_Op {
   EINA_INLIST; /**< Intrusive list node. */

   Gfx_Map_Op_Type op; /**< The type of mapping operation. */
   union {
      /** Parameters for GFX_MAPPING_RAW_COORD operation. */
      struct {
         int idx;        /**< Index of the point to modify (-1 for all points). */
         double x, y, z;  /**< New X, Y, Z coordinates. */
      } raw_coord;
      /** Parameters for GFX_MAPPING_COLOR operation. */
      struct {
         int idx;              /**< Index of the point to color (-1 for all points). */
         uint8_t r, g, b, a;   /**< RGBA color components. */
      } color;
      /** Parameters for GFX_MAPPING_ROTATE_2D operation. */
      struct {
         double degrees;      /**< Rotation angle in degrees. */
      } rotate_2d;
      /** Parameters for GFX_MAPPING_ROTATE_3D operation. */
      struct {
         double dx, dy, dz;   /**< Rotation amounts around X, Y, Z axes. */
      } rotate_3d;
      /** Parameters for GFX_MAPPING_ROTATE_QUAT operation. */
      struct {
         double qx, qy, qz, qw; /**< Quaternion components (x, y, z, w). */
      } rotate_quat;
      /** Parameters for GFX_MAPPING_ZOOM operation. */
      struct {
         double zx, zy;       /**< Zoom factors for X and Y axes. */
      } zoom;
      /** Parameters for GFX_MAPPING_TRANSLATE operation. */
      struct {
         double dx, dy, dz;   /**< Translation amounts for X, Y, Z axes. */
      } translate;
      /** Parameters for GFX_MAPPING_LIGHTING_3D operation. */
      struct {
         uint8_t lr, lg, lb;   /**< Light color components (RGB). */
         uint8_t ar, ag, ab;   /**< Ambient color components (RGB). */
      } lighting_3d;
      /** Parameters for GFX_MAPPING_PERSPECTIVE_3D operation. */
      struct {
         double z0;           /**< Z coordinate of the vanishing point on the Z axis. */
         double foc;          /**< Focal length. */
      } perspective_3d;
   };
   /** Pivot information for the operation. */
   struct {
      Gfx_Map_Pivot  *pivot;       /**< The pivot object, if any. */
      double          cx, cy, cz;  /**< Pivot center coordinates (relative or absolute). */
      Eina_Bool       is_absolute; /**< EINA_TRUE if cx, cy, cz are absolute coordinates. */
      Eina_Bool       is_self;     /**< EINA_TRUE if the pivot is the object itself. */
   } pivot;
};

/**
 * @internal
 * @brief Represents a pivot point for mapping operations.
 *
 * A pivot can be another Efl_Gfx_Entity or the canvas itself.
 * It is used as a reference for relative transformations.
 */
struct _Gfx_Map_Pivot
{
   EINA_INLIST; /**< Intrusive list node. */

   Evas_Object_Protected_Data *map_obj; /**< The Evas object this pivot is associated with for mapping updates. */
   Eo             *eo_obj;              /**< The Efl_Gfx_Entity object used as a pivot. Strong reference. */
   Eina_Rect       geometry;            /**< Cached geometry of the pivot object. */
   Eina_Bool       event_cbs;           /**< EINA_TRUE if event callbacks for geometry changes are registered. */
   Eina_Bool       is_evas;             /**< EINA_TRUE if the pivot is a legacy Evas object. */
   Eina_Bool       is_canvas;           /**< EINA_TRUE if the pivot is a canvas (scene). */
   Eina_Bool       changed;             /**< EINA_TRUE if the pivot's geometry has changed since the last map calculation. */
};

/**
 * @internal
 * @brief Represents a UV mapping coordinate for a point.
 *
 * UV coordinates are normalized (0.0 to 1.0) and define how a texture
 * is mapped onto the geometry.
 */
struct _Gfx_Map_Point {
     double u; /**< U coordinate (horizontal). */
     double v; /**< V coordinate (vertical). */
};

/**
 * @internal
 * @brief Core structure holding all data for a graphics map.
 *
 * This structure is managed by a copy-on-write (COW) mechanism.
 * It contains the list of operations, UV points, pivots, and the
 * calculated Evas_Map.
 */
struct _Gfx_Map {
   Gfx_Map_Op *ops;          /**< List of mapping operations. */
   Gfx_Map_Point *points;    /**< Array of UV mapping points. Example: `points[0] = {u=0.0, v=0.0};` */

   Gfx_Map_Pivot *pivots;    /**< List of pivot objects used by operations. */
   Evas_Map      *map;       /**< The calculated Evas_Map. */
   Gfx_Map_Op    *last_calc_op; /**< The last operation processed during map calculation. Used for partial recalculations. */
   int            imw, imh;  /**< Image/source width and height used for UV mapping. */
   int            count;     /**< Number of points in the map (typically 4 for a quad). */

   // FIXME: Those need a quality vs. performance setting instead
   Eina_Bool alpha;          /**< EINA_TRUE if alpha blending is enabled for the map. */
   Eina_Bool smooth;         /**< EINA_TRUE if smooth rendering (anti-aliasing) is enabled. */
   Eina_Bool event_cbs;      /**< EINA_TRUE if geometry change event callbacks are registered for the mapped object itself. */
};

/**
 * @internal
 * @brief Private data for the Efl_Gfx_Mapping mixin.
 *
 * Contains a pointer to the copy-on-write (COW) Gfx_Map data.
 */
struct _Efl_Gfx_Mapping_Data {
   const Gfx_Map *cow; /**< Pointer to the COW Gfx_Map data. */
};

// ----------------------------------------------------------------------------

/** @internal A dummy Eo object used as a marker for absolute pivot operations. */
static Eo *gfx_mapping_absolute = NULL;
/** @internal The Eina_Cow instance used for managing Gfx_Map data. */
static Eina_Cow *gfx_mapping_cow = NULL;
/** @internal Default state for a Gfx_Map, used for COW initialization and reset. */
static const Gfx_Map gfx_mapping_cow_default = {
   NULL, /* ops */
   NULL, /* points */
   NULL, /* pivots */
   NULL, /* map */
   NULL, /* last_calc_op */
   0, 0, /* imw, imh */
   4,    /* count */
   EINA_TRUE,  /* alpha */
   EINA_TRUE,  /* smooth */
   EINA_FALSE /* event_cbs */
};

/** @internal Macro to begin a copy-on-write operation on the Gfx_Map data. */
#define MAPCOW_BEGIN(_pd) eina_cow_write(gfx_mapping_cow, (const Eina_Cow_Data**)&(_pd->cow))
/** @internal Macro to end a copy-on-write operation. */
#define MAPCOW_END(_mapcow, _pd) eina_cow_done(gfx_mapping_cow, (const Eina_Cow_Data**)&(_pd->cow), _mapcow, EINA_FALSE)
/** @internal Macro to write a value to a field in the Gfx_Map, initiating COW if necessary. */
#define MAPCOW_WRITE(pd, name, value) do { \
   if (pd->cow->name != (value)) { \
     Gfx_Map *_cow = MAPCOW_BEGIN(pd); \
     _cow->name = (value); \
     MAPCOW_END(_cow, pd); \
   }} while (0)

/** @internal Macro to reference a pivot object. `eo_obj` is the mapping object. */
#define PIVOT_REF(_pivot_eo_obj, _mapping_eo_obj) (_pivot_eo_obj ? efl_xref((Eo *) _pivot_eo_obj, _mapping_eo_obj) : NULL)
/** @internal Macro to unreference a pivot object. `_mapping_eo_obj` is the mapping object. */
#define PIVOT_UNREF(_pivot_eo_obj, _mapping_eo_obj) (_pivot_eo_obj ? efl_xunref(_pivot_eo_obj, _mapping_eo_obj) : NULL)

/**
 * @internal
 * @brief Cleans up resources held by a Gfx_Map instance.
 *
 * This includes freeing allocated points, the Evas_Map, operations, and pivots.
 *
 * @param eo_obj The Efl_Gfx_Mapping object.
 * @param pd The private data of the Efl_Gfx_Mapping object.
 */
static inline void _map_clean(Eo *eo_obj, Efl_Gfx_Mapping_Data *pd);

// ----------------------------------------------------------------------------

/**
 * @internal
 * @brief Initializes the Efl_Gfx_Mapping subsystem.
 *
 * Sets up the Eina_Cow for Gfx_Map structures.
 * This function is typically called during Evas initialization.
 */
void
_efl_gfx_mapping_init(void)
{
   gfx_mapping_cow = eina_cow_add("Efl.Gfx.Mapping", sizeof(Gfx_Map), 8,
                              &gfx_mapping_cow_default, EINA_FALSE);
}

/**
 * @internal
 * @brief Shuts down the Efl_Gfx_Mapping subsystem.
 *
 * Deletes the Eina_Cow for Gfx_Map structures and unrefs the global absolute pivot marker.
 * This function is typically called during Evas shutdown.
 */
void
_efl_gfx_mapping_shutdown(void)
{
   eina_cow_del(gfx_mapping_cow);
   gfx_mapping_cow = NULL;

   efl_unref(gfx_mapping_absolute);
   gfx_mapping_absolute = NULL;
}

// ----------------------------------------------------------------------------

EOLIAN static Efl_Object *
_efl_gfx_mapping_efl_object_constructor(Eo *eo_obj, Efl_Gfx_Mapping_Data *pd)
{
   eo_obj = efl_constructor(efl_super(eo_obj, MY_CLASS));
   pd->cow = eina_cow_alloc(gfx_mapping_cow);
   return eo_obj;
}

EOLIAN static void
_efl_gfx_mapping_efl_object_destructor(Eo *eo_obj, Efl_Gfx_Mapping_Data *pd)
{
   if (pd->cow)
     {
        _map_clean(eo_obj, pd);
        eina_cow_free(gfx_mapping_cow, (const Eina_Cow_Data **) &pd->cow);
     }
   efl_destructor(efl_super(eo_obj, MY_CLASS));
}

// ----------------------------------------------------------------------------

/**
 * @internal
 * @brief Callback invoked when the mapped object's geometry (position or size) changes.
 *
 * Marks the map as needing an update.
 *
 * @param data The Evas_Object_Protected_Data of the mapped object.
 * @param ev The event information (unused).
 */
static void
_geometry_changed_cb(void *data, const Efl_Event *ev EINA_UNUSED)
{
   Evas_Object_Protected_Data *obj = data;
   Efl_Gfx_Mapping_Data *pd = efl_data_scope_get(obj->object, MY_CLASS);

   MAPCOW_WRITE(pd, last_calc_op, NULL); // Invalidate last calculated operation
   obj->gfx_mapping_update = EINA_TRUE; // Mark for update
}

/** @internal Array of callbacks for geometry changes of the mapped object. */
EFL_CALLBACKS_ARRAY_DEFINE(_geometry_changes,
                           { EFL_GFX_ENTITY_EVENT_POSITION_CHANGED, _geometry_changed_cb },
                           { EFL_GFX_ENTITY_EVENT_SIZE_CHANGED, _geometry_changed_cb });

/**
 * @internal
 * @brief Callback invoked when a pivot object's geometry (position or size) changes.
 *
 * Marks the associated map and the pivot itself as needing an update.
 *
 * @param data The Gfx_Map_Pivot whose geometry changed.
 * @param ev The event information (unused).
 */
static void
_pivot_changed_cb(void *data, const Efl_Event *ev EINA_UNUSED)
{
   Gfx_Map_Pivot *pivot = data;
   Evas_Object_Protected_Data *obj = pivot->map_obj; // The object being mapped

   obj->gfx_mapping_update = EINA_TRUE; // Mark the main map for update
   pivot->changed = EINA_TRUE;          // Mark this pivot as changed
}

/** @internal Array of callbacks for geometry changes of pivot objects. */
EFL_CALLBACKS_ARRAY_DEFINE(_pivot_changes,
                           { EFL_GFX_ENTITY_EVENT_POSITION_CHANGED, _pivot_changed_cb },
                           { EFL_GFX_ENTITY_EVENT_SIZE_CHANGED, _pivot_changed_cb });

/**
 * @internal
 * @brief Marks the graphics map as dirty and needing recalculation.
 *
 * This function ensures that the object is flagged as changed and, if necessary,
 * sets up event callbacks for geometry changes on the object itself or its pivots.
 *
 * @param eo_obj The Efl_Gfx_Mapping object.
 * @param pd The private data of the Efl_Gfx_Mapping object.
 * @param reset If EINA_TRUE, the Evas_Map is reset immediately. Otherwise,
 *              it's just marked for update.
 */
static inline void
_map_dirty(Eo *eo_obj, Efl_Gfx_Mapping_Data *pd, Eina_Bool reset)
{
   Evas_Object_Protected_Data *obj = EVAS_OBJ_GET_OR_RETURN(eo_obj);
   Gfx_Map_Pivot *pivot;

   obj->gfx_mapping_has = EINA_TRUE;
   obj->gfx_mapping_update |= !reset;
   obj->changed_map = EINA_TRUE;
   evas_object_change(eo_obj, obj);

   if (reset)
     {
        _evas_map_reset(pd->cow->map);
        return;
     }

   // FIXME: Only add if there is a self-pivot (relative coordinates)
   if (!pd->cow->event_cbs)
     {
        MAPCOW_WRITE(pd, event_cbs, EINA_TRUE);
        efl_event_callback_array_add(eo_obj, _geometry_changes(), obj);
     }

   EINA_INLIST_FOREACH(pd->cow->pivots, pivot)
     {
        if (pivot->event_cbs) continue;
        pivot->event_cbs = EINA_TRUE;
        efl_event_callback_array_add(pivot->eo_obj, _pivot_changes(), pivot);
     }
}

/**
 * @internal
 * @brief Calculates or updates the Evas_Map based on the current Gfx_Map operations.
 *
 * This is the core function that translates the high-level Gfx_Map_Op list
 * into a concrete Evas_Map that Evas can render. It handles pivot geometry updates,
 * UV coordinate calculations, and applies all transformations.
 *
 * @param eo_obj The Efl_Gfx_Mapping object (const because map calculation should not change logical state, only cached Evas_Map).
 * @param obj The protected data of the Evas object being mapped.
 * @param pd The private data of the Efl_Gfx_Mapping object.
 * @return The calculated or updated Evas_Map, or NULL if no mapping is applied or an error occurs.
 *         The returned Evas_Map is owned by the Gfx_Map structure (pd->cow->map).
 */
static Evas_Map *
_map_calc(const Eo *eo_obj, Evas_Object_Protected_Data *obj, Efl_Gfx_Mapping_Data *pd)
{
   Gfx_Map_Op *op, *first_op = pd->cow->ops, *last_op;
   Gfx_Map_Pivot *pivot;
   Gfx_Map *mcow;
   Evas_Map *m;
   int imw, imh;
   int count;
   Eina_Bool map_alloc = EINA_FALSE;

   if (pd->cow == &gfx_mapping_cow_default)
     return NULL;

   m = pd->cow->map;
   if (!obj->gfx_mapping_update) return m;

   last_op = pd->cow->last_calc_op;
   count = pd->cow->count < 4 ? 4 : pd->cow->count;

   EINA_INLIST_FOREACH(pd->cow->pivots, pivot)
     {
        if (!pivot->changed) continue;
        last_op = NULL;
        pivot->changed = EINA_FALSE;
        if (!pivot->is_canvas)
          {
             pivot->geometry = efl_gfx_entity_geometry_get(pivot->eo_obj);
          }
        else
          {
             // Note: pivot can not be an Evas when using pure EO API
             if (pivot->is_evas)
               evas_output_size_get(pivot->eo_obj, &pivot->geometry.w, &pivot->geometry.h);
             else
               pivot->geometry.size = efl_gfx_entity_size_get(pivot->eo_obj);
             pivot->geometry.x = 0;
             pivot->geometry.y = 0;
          }
     }

   if (!pd->cow->points)
     {
        Gfx_Map_Point *ps = calloc(1, count * sizeof(Gfx_Map_Point));
        if (!ps) return m;
        ps[0].u = 0.0; ps[0].v = 0.0;
        ps[1].u = 1.0; ps[1].v = 0.0;
        ps[2].u = 1.0; ps[2].v = 1.0;
        ps[3].u = 0.0; ps[3].v = 1.0;
        MAPCOW_WRITE(pd, points, ps);
     }

   if (m && last_op)
     {
        first_op = EINA_INLIST_NEXT(last_op);
        imw = pd->cow->imw;
        imh = pd->cow->imh;
     }
   else
     {
        if (!m)
          {
             m = evas_map_new(count);
             if (!m) return NULL;
             map_alloc = EINA_TRUE;
          }
        else _evas_map_reset(m);
        m->alpha = pd->cow->alpha;
        m->smooth = pd->cow->smooth;
        m->move_sync.enabled = EINA_FALSE;

        _evas_map_util_points_populate(m,
                                       obj->cur->geometry.x, obj->cur->geometry.y,
                                       obj->cur->geometry.w, obj->cur->geometry.h,
                                       0);

        if (obj->is_image_object)
          {
             // Image is a special case in terms of geometry
             Eina_Size2D sz;
             sz = efl_gfx_view_size_get(eo_obj);
             imw = sz.w;
             imh = sz.h;
          }
        else
          {
             imw = obj->cur->geometry.w;
             imh = obj->cur->geometry.h;
          }

        last_op = NULL;
        first_op = pd->cow->ops;
     }

   for (int k = 0; k < count; k++)
     {
        Evas_Map_Point *p = &(m->points[k]);
        p->u = pd->cow->points[k].u * imw;
        p->v = pd->cow->points[k].v * imh;
     }

   EINA_INLIST_FOREACH(first_op, op)
     {
        int k, kmin = 0, kmax = count - 1;
        double cx, cy, cz;
        Evas_Map_Point *p;

        if (!op->pivot.is_absolute)
          {
             int px = 0, py = 0, pw = 1, ph = 1;

             if (op->pivot.is_self)
               {
                  px = obj->cur->geometry.x;
                  py = obj->cur->geometry.y;
                  pw = obj->cur->geometry.w;
                  ph = obj->cur->geometry.h;
               }
             else
               {
                  if (!op->pivot.pivot)
                    {
                       EINA_SAFETY_ERROR("safety check failed: op->pivot.pivot == NULL");
                       if (map_alloc) evas_map_free(m);
                       return NULL;
                    }

                  pivot = op->pivot.pivot;
                  px = pivot->geometry.x;
                  py = pivot->geometry.y;
                  pw = pivot->geometry.w;
                  ph = pivot->geometry.h;
               }

             cx = (double) px + (double) pw * op->pivot.cx;
             cy = (double) py + (double) ph * op->pivot.cy;
             cz = op->pivot.cz;
          }
        else
          {
             cx = op->pivot.cx;
             cy = op->pivot.cy;
             cz = op->pivot.cz;
          }

        switch (op->op)
          {
           case GFX_MAPPING_RAW_COORD:
             if (op->raw_coord.idx != -1)
               kmin = kmax = op->raw_coord.idx;
             for (k = kmin; k <= kmax; k++)
               {
                  p = &(m->points[k]);
                  p->px = p->x = op->raw_coord.x;
                  p->py = p->y = op->raw_coord.y;
                  p->z = op->raw_coord.z;
               }
             break;
           case GFX_MAPPING_COLOR:
             if (op->raw_coord.idx != -1)
               kmin = kmax = op->raw_coord.idx;
             for (k = kmin; k <= kmax; k++)
               {
                  p = &(m->points[k]);
                  p->r = op->color.r;
                  p->g = op->color.g;
                  p->b = op->color.b;
                  p->a = op->color.a;
               }
             break;
           case GFX_MAPPING_ROTATE_2D:
             _map_util_rotate(m, op->rotate_2d.degrees, cx, cy);
             break;
           case GFX_MAPPING_ROTATE_3D:
             _map_util_3d_rotate(m, op->rotate_3d.dx, op->rotate_3d.dy,
                                 op->rotate_3d.dz, cx, cy, cz);
             break;
           case GFX_MAPPING_ROTATE_QUAT:
             _map_util_quat_rotate(m, op->rotate_quat.qx, op->rotate_quat.qy,
                                   op->rotate_quat.qz, op->rotate_quat.qw,
                                   cx, cy, cz);
             break;
           case GFX_MAPPING_ZOOM:
             _map_util_zoom(m, op->zoom.zx, op->zoom.zy, cx, cy);
             break;
           case GFX_MAPPING_TRANSLATE:
             _map_util_translate(m, op->translate.dx, op->translate.dy,
                                 op->translate.dz);
             break;
           case GFX_MAPPING_LIGHTING_3D:
             _map_util_3d_lighting(m, cx, cy, cz, op->lighting_3d.lr,
                                   op->lighting_3d.lg, op->lighting_3d.lb,
                                   op->lighting_3d.ar, op->lighting_3d.ag,
                                   op->lighting_3d.ab);
             break;
           case GFX_MAPPING_PERSPECTIVE_3D:
             _map_util_3d_perspective(m, cx, cy, op->perspective_3d.z0,
                                      op->perspective_3d.foc);
             break;
          }

        last_op = op;
     }

   mcow = MAPCOW_BEGIN(pd);
   mcow->map = m;
   mcow->last_calc_op = last_op;
   mcow->imw = imw;
   mcow->imh = imh;
   MAPCOW_END(mcow, pd);
   obj->gfx_mapping_update = EINA_FALSE;

   return m;
}

/**
 * @internal
 * @brief Forces an update of the graphics mapping for the given object.
 *
 * This function recalculates the Evas_Map using _map_calc() and applies it
 * to the Evas object. It also updates the internal state related to whether
 * the object has mapping enabled.
 *
 * @param eo_obj The Efl_Gfx_Mapping object to update.
 */
void
_efl_gfx_mapping_update(Eo *eo_obj)
{
   Evas_Object_Protected_Data *obj = EVAS_OBJ_GET_OR_RETURN(eo_obj);
   Efl_Gfx_Mapping_Data *pd = efl_data_scope_get(eo_obj, MY_CLASS);
   Evas_Map *m;

   m = _map_calc(eo_obj, obj, pd);
   evas_object_map_set(eo_obj, m);
   _evas_object_map_enable_set(eo_obj, obj, m != NULL);
   obj->gfx_mapping_has = (m != NULL);
}

static inline void
_map_clean(Eo *eo_obj, Efl_Gfx_Mapping_Data *pd)
{
   free(pd->cow->points);
   if (pd->cow->map) evas_map_free(pd->cow->map);
   if (pd->cow->ops)
     {
        Gfx_Map_Pivot *pivot;
        Gfx_Map_Op *op;
        Gfx_Map *mcow;

        // This function modifies the cow data, so it needs to be writable.
        mcow = MAPCOW_BEGIN(pd);
        EINA_INLIST_FREE(mcow->ops, op)
          {
             EINA_INLIST_REMOVE(mcow->ops, op);
             free(op);
          }
        EINA_INLIST_FREE(mcow->pivots, pivot)
          {
             EINA_INLIST_REMOVE(mcow->pivots, pivot);
             if (pivot->event_cbs)
               efl_event_callback_array_del(pivot->eo_obj, _pivot_changes(), pivot);
             // eo_obj here is the mapping object, not the pivot object itself.
             PIVOT_UNREF(pivot->eo_obj, eo_obj);
             free(pivot);
          }
        MAPCOW_END(mcow, pd);
     }
}

EOLIAN Eina_Bool
_efl_gfx_mapping_mapping_has(Eo *eo_obj EINA_UNUSED, Efl_Gfx_Mapping_Data *pd EINA_UNUSED)
{
   // Check if the current COW data is the default (no operations)
   // or if there are any operations or an existing map.
   if (pd->cow == &gfx_mapping_cow_default) return EINA_FALSE;
   // An object has mapping if it has operations or an already calculated map.
   if (pd->cow->ops) return EINA_TRUE;
   if (pd->cow->map) return EINA_TRUE;
   return EINA_FALSE;
}

EOLIAN static void
_efl_gfx_mapping_mapping_reset(Eo *eo_obj, Efl_Gfx_Mapping_Data *pd)
{
   Evas_Object_Protected_Data *obj = EVAS_OBJ_GET_OR_RETURN(eo_obj);
   Eina_Bool alpha, smooth;

   // Preserve alpha and smooth settings across reset
   alpha = pd->cow->alpha;
   smooth = pd->cow->smooth;

   _map_clean(eo_obj, pd); // Clean up current map data (ops, pivots, evas_map)

   // Remove geometry change callbacks if they were added for the object itself
   if (pd->cow->event_cbs)
     efl_event_callback_array_del(eo_obj, _geometry_changes(), obj);

   // Reset COW data to default
   eina_cow_memcpy(gfx_mapping_cow, (const Eina_Cow_Data * const *) &pd->cow,
                   (const Eina_Cow_Data *) &gfx_mapping_cow_default);

   _map_dirty(eo_obj, pd, EINA_TRUE); // Mark as dirty and reset Evas_Map

   // Restore alpha and smooth settings
   MAPCOW_WRITE(pd, alpha, alpha);
   MAPCOW_WRITE(pd, smooth, smooth);
}

EOLIAN static int
_efl_gfx_mapping_mapping_point_count_get(const Eo *eo_obj EINA_UNUSED, Efl_Gfx_Mapping_Data *pd)
{
   return pd->cow->count;
}

EOLIAN static void
_efl_gfx_mapping_mapping_point_count_set(Eo *eo_obj EINA_UNUSED, Efl_Gfx_Mapping_Data *pd, int count)
{
   Gfx_Map *mcow;

   if ((count <= 0) || (count % 4 != 0))
     {
        ERR("Map point count (%d) should be multiples of 4", count);
        return;
     }
   if (pd->cow->count == count) return;

   mcow = MAPCOW_BEGIN(pd);
   if (mcow->points == NULL) // If no points array exists yet
     {
        mcow->points = calloc(1, count * sizeof(Gfx_Map_Point));
        if (mcow->points)
          mcow->count = count; // Update count only on successful allocation
        else
          ERR("Failed to allocate memory with calloc for %d map points", count);
     }
   else // If points array already exists, reallocate
     {
        Gfx_Map_Point *ps = realloc(mcow->points, count * sizeof(Gfx_Map_Point));
        if (ps)
          {
             mcow->points = ps;
             // If new count is larger, initialize new points to 0 (realloc doesn't guarantee this for the extended part)
             if (count > pd->cow->count)
                memset(mcow->points + pd->cow->count, 0, (count - pd->cow->count) * sizeof(Gfx_Map_Point));
             mcow->count = count;
          }
        else
          ERR("Failed to allocate memory with realloc for %d map points", count);
     }
   MAPCOW_END(mcow, pd);
   // Note: _map_dirty is not called here as changing point count itself
   // doesn't make the map dirty until UVs or Coords are set for these points.
   // However, any subsequent operation will trigger _map_dirty.
   // For safety and consistency, one might consider calling _map_dirty here too.
}

EOLIAN static Eina_Bool
_efl_gfx_mapping_mapping_clockwise_get(const Eo *eo_obj, Efl_Gfx_Mapping_Data *pd)
{
   Evas_Object_Protected_Data *obj = EVAS_OBJ_GET_OR_RETURN(eo_obj, EINA_TRUE);
   Evas_Map *m;

   m = _map_calc(eo_obj, obj, pd);
   if (!m) return EINA_TRUE; // Default to clockwise if no map
   return evas_map_util_clockwise_get(m);
}

EOLIAN static void
_efl_gfx_mapping_mapping_smooth_set(Eo *eo_obj, Efl_Gfx_Mapping_Data *pd, Eina_Bool smooth)
{
   if (pd->cow->smooth == smooth) return;

   MAPCOW_WRITE(pd, smooth, smooth);
   // If the map itself changes (e.g. smooth property), it needs to be marked dirty.
   _map_dirty(eo_obj, pd, EINA_FALSE);
}

EOLIAN static Eina_Bool
_efl_gfx_mapping_mapping_smooth_get(const Eo *eo_obj EINA_UNUSED, Efl_Gfx_Mapping_Data *pd)
{
   return pd->cow->smooth;
}

EOLIAN static void
_efl_gfx_mapping_mapping_alpha_set(Eo *eo_obj, Efl_Gfx_Mapping_Data *pd, Eina_Bool alpha)
{
   if (pd->cow->alpha == alpha) return;

   MAPCOW_WRITE(pd, alpha, alpha);
   // If the map itself changes (e.g. alpha property), it needs to be marked dirty.
   _map_dirty(eo_obj, pd, EINA_FALSE);
}

EOLIAN static Eina_Bool
_efl_gfx_mapping_mapping_alpha_get(const Eo *eo_obj EINA_UNUSED, Efl_Gfx_Mapping_Data *pd)
{
   return pd->cow->alpha;
}

EOLIAN static void
_efl_gfx_mapping_mapping_coord_absolute_get(const Eo *eo_obj, Efl_Gfx_Mapping_Data *pd,
                                    int idx, double *x, double *y, double *z)
{
   Evas_Object_Protected_Data *obj = EVAS_OBJ_GET_OR_RETURN(eo_obj);
   Evas_Map *m;

   EINA_SAFETY_ON_FALSE_RETURN((idx >= 0) && (idx < pd->cow->count));

   m = _map_calc(eo_obj, obj, pd);
   if (!m) // If map hasn't been calculated or no ops, return default quad coords
     {
        int X, Y, W, H;

        X = obj->cur->geometry.x;
        Y = obj->cur->geometry.y;
        W = obj->cur->geometry.w;
        H = obj->cur->geometry.h;

        // Default quad points based on object geometry
        // P0: (X, Y) P1: (X+W, Y) P2: (X+W, Y+H) P3: (X, Y+H)
        if (x)
          {
             if ((idx == 0) || (idx == 3)) *x = X; // Top-left X, Bottom-left X
             else *x = X + W;                     // Top-right X, Bottom-right X
          }
        if (y)
          {
             if ((idx == 0) || (idx == 1)) *y = Y; // Top-left Y, Top-right Y
             else *y = Y + H;                     // Bottom-left Y, Bottom-right Y
          }
        if (z) *z = 0; // Default Z is 0
        return;
     }

   _map_point_coord_get(m, idx, x, y, z); // Get from calculated Evas_Map
}

EOLIAN static void
_efl_gfx_mapping_mapping_uv_set(Eo *eo_obj, Efl_Gfx_Mapping_Data *pd,
                        int idx, double u, double v)
{
   Gfx_Map *mcow;

   EINA_SAFETY_ON_FALSE_RETURN((idx >= 0) && (idx < pd->cow->count));

   // Ensure points array is allocated
   if (!pd->cow->points)
     {
        Gfx_Map_Point *ps = calloc(1, pd->cow->count * sizeof(Gfx_Map_Point));
        if (!ps) {
            ERR("Failed to allocate points for UV set");
            return;
        }
        // Initialize default UVs for a quad if count is 4 and points were just allocated
        if (pd->cow->count == 4) {
            ps[0].u = 0.0; ps[0].v = 0.0;
            ps[1].u = 1.0; ps[1].v = 0.0;
            ps[2].u = 1.0; ps[2].v = 1.0;
            ps[3].u = 0.0; ps[3].v = 1.0;
        }
        MAPCOW_WRITE(pd, points, ps);
     }

   // Check if values are actually changing
   if (EINA_DBL_EQ(pd->cow->points[idx].u, u) &&
       EINA_DBL_EQ(pd->cow->points[idx].v, v))
     return;

   mcow = MAPCOW_BEGIN(pd);
   // UV coordinates are typically clamped between 0.0 and 1.0.
   mcow->points[idx].u = CLAMP(0.0, u, 1.0);
   mcow->points[idx].v = CLAMP(0.0, v, 1.0);
   MAPCOW_END(mcow, pd);

   _map_dirty(eo_obj, pd, EINA_FALSE); // UV change makes the map dirty
}

EOLIAN static void
_efl_gfx_mapping_mapping_uv_get(const Eo *eo_obj EINA_UNUSED, Efl_Gfx_Mapping_Data *pd,
                        int idx, double *u, double *v)
{
   EINA_SAFETY_ON_FALSE_RETURN((idx >= 0) && (idx < pd->cow->count));
   // Ensure points array exists before trying to access it.
   // If points is NULL, it implies default UVs (0,0 for idx 0, etc.),
   // but the API expects to return stored values or fail if not set.
   // Here, we rely on safety check and assume points is valid if count > 0
   // and UVs have been set at least once or initialized.
   EINA_SAFETY_ON_NULL_RETURN(pd->cow->points);


   if (u) *u = pd->cow->points[idx].u;
   if (v) *v = pd->cow->points[idx].v;
}

EOLIAN static void
_efl_gfx_mapping_mapping_color_get(const Eo *eo_obj EINA_UNUSED, Efl_Gfx_Mapping_Data *pd,
                           int idx, int *r, int *g, int *b, int *a)
{
   Evas_Object_Protected_Data *obj = EVAS_OBJ_GET_OR_RETURN(eo_obj);
   Evas_Map_Point *p;
   Evas_Map *m;

   EINA_SAFETY_ON_FALSE_RETURN((idx >= 0) && (idx < pd->cow->count));

   if (!r && !g && !b && !a) return; // Nothing to retrieve

   m = _map_calc(eo_obj, obj, pd);
   if (!m) // If map not calculated or no ops, return default white/opaque
     {
        if (r) *r = 255;
        if (g) *g = 255;
        if (b) *b = 255;
        if (a) *a = 255;
        return;
     }

   p = &(m->points[idx]);
   if (r) *r = p->r;
   if (g) *g = p->g;
   if (b) *b = p->b;
   if (a) *a = p->a;
}

/**
 * @internal
 * @brief Adds a new graphics mapping operation to the list.
 *
 * This is a helper function to create and append a Gfx_Map_Op.
 * It handles pivot management (finding existing or creating new ones)
 * and marks the map as dirty.
 *
 * @param eo_obj The Efl_Gfx_Mapping object.
 * @param pd The private data of the Efl_Gfx_Mapping object.
 * @param type The type of operation to add.
 * @param eo_pivot The Efl_Gfx_Entity to use as a pivot (can be NULL).
 *                 If NULL and is_absolute is EINA_FALSE, the object itself is the pivot.
 * @param cx Relative or absolute X coordinate of the pivot center.
 * @param cy Relative or absolute Y coordinate of the pivot center.
 * @param cz Relative or absolute Z coordinate of thepivot center.
 * @param is_absolute EINA_TRUE if cx, cy, cz are absolute screen coordinates,
 *                    EINA_FALSE if they are relative to the pivot object's geometry (0.0-1.0).
 * @return The newly added Gfx_Map_Op, or NULL on allocation failure.
 */
static Gfx_Map_Op *
_gfx_mapping_op_add(Eo *eo_obj, Efl_Gfx_Mapping_Data *pd, Gfx_Map_Op_Type type,
                const Efl_Gfx_Entity *eo_pivot, double cx, double cy, double cz,
                Eina_Bool is_absolute)
{
   Eina_Bool is_self = EINA_FALSE;
   Gfx_Map_Pivot *pivot = NULL;
   Gfx_Map_Op *op;
   Gfx_Map *mcow;

   op = calloc(1, sizeof(*op));
   if (!op) return NULL;

   mcow = MAPCOW_BEGIN(pd); // Ensure COW data is writable

   if (!is_absolute) // Handle relative pivots
     {
        // If pivot is self (eo_obj) or NULL, it's a self-pivot
        if ((eo_pivot == eo_obj) || !eo_pivot)
          {
             eo_pivot = NULL; // Ensure eo_pivot is NULL for self-pivots for consistency
             is_self = EINA_TRUE;
          }
        else // External pivot object
          {
             // Get protected data of the object being mapped, not the pivot.
             // This is needed for pivot->map_obj.
             Evas_Object_Protected_Data *obj_pd = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS); // Assuming it's an Evas_Object

             // Check if this pivot is already known
             EINA_INLIST_FOREACH(mcow->pivots, pivot)
               if (pivot->eo_obj == eo_pivot) break;

             if (!pivot) // New pivot, create and add it
               {
                  pivot = calloc(1, sizeof(*pivot));
                  if (!pivot) { // Allocation failed
                      // Must free 'op' and revert COW if it was the first write
                      free(op);
                      // This COW_END might be problematic if mcow was not a new copy.
                      // A safer approach would be to check if eina_cow_is_writable(pd->cow) before MAPCOW_BEGIN
                      // or have a MAPCOW_CANCEL. For now, assume MAPCOW_END is okay.
                      MAPCOW_END(mcow, pd);
                      return NULL;
                  }
                  // Store a reference to the pivot object, xref against the mapping object (eo_obj)
                  pivot->eo_obj = PIVOT_REF(eo_pivot, eo_obj);
                  pivot->changed = EINA_TRUE; // New pivot, needs geometry refresh
                  // Check if pivot is a canvas/scene for special geometry handling
                  if (efl_isa(eo_pivot, EVAS_CANVAS_CLASS)) // Legacy Evas canvas
                    {
                       pivot->is_evas = EINA_TRUE;
                       pivot->is_canvas = EINA_TRUE;
                    }
                  else if (efl_isa(eo_pivot, EFL_CANVAS_SCENE_INTERFACE)) // EO Scene
                    pivot->is_canvas = EINA_TRUE;
                  pivot->map_obj = obj_pd; // Associate with the mapped object's protected data
                  EINA_INLIST_APPEND(mcow->pivots, pivot);
               }
          }
     }

   op->op = type;
   op->pivot.is_absolute = is_absolute;
   op->pivot.is_self = is_self;
   op->pivot.pivot = pivot; // Link to Gfx_Map_Pivot structure
   op->pivot.cx = cx;
   op->pivot.cy = cy;
   op->pivot.cz = cz;

   EINA_INLIST_APPEND(mcow->ops, op); // Add operation to the list
   MAPCOW_END(mcow, pd);

   _map_dirty(eo_obj, pd, EINA_FALSE); // Mark map as dirty

   return op;
}

EOLIAN static void
_efl_gfx_mapping_mapping_coord_absolute_set(Eo *eo_obj, Efl_Gfx_Mapping_Data *pd,
                                    int idx, double x, double y, double z)
{
   Gfx_Map_Op *op;

   EINA_SAFETY_ON_FALSE_RETURN((idx >= 0) && (idx < pd->cow->count));

   // GFX_MAPPING_RAW_COORD is implicitly relative to the object itself (no external pivot, not absolute screen coords)
   // The 'absolute' in the function name refers to the coordinate values, not the pivot mode.
   // The pivot parameters (cx,cy,cz) for RAW_COORD are not used, so 0,0,0 is fine.
   // is_absolute for _gfx_mapping_op_add refers to pivot center, not the operation type.
   // For RAW_COORD, the pivot is effectively the object itself, so is_absolute = EINA_FALSE.
   op = _gfx_mapping_op_add(eo_obj, pd, GFX_MAPPING_RAW_COORD, NULL, 0, 0, 0, EINA_FALSE);
   if (!op) return;

   op->raw_coord.idx = idx;
   op->raw_coord.x = x;
   op->raw_coord.y = y;
   op->raw_coord.z = z;
}

EOLIAN static void
_efl_gfx_mapping_mapping_color_set(Eo *eo_obj, Efl_Gfx_Mapping_Data *pd,
                           int idx, int r, int g, int b, int a)
{
   Gfx_Map_Op *op;

   // idx can be -1 to affect all points, or a valid index.
   EINA_SAFETY_ON_FALSE_RETURN((idx >= -1) && (idx < pd->cow->count));

   // GFX_MAPPING_COLOR does not use a pivot.
   op = _gfx_mapping_op_add(eo_obj, pd, GFX_MAPPING_COLOR, NULL, 0, 0, 0, EINA_FALSE);
   if (!op) return;

   op->color.idx = idx;
   op->color.r = CLAMP(0, r, 255);
   op->color.g = CLAMP(0, g, 255);
   op->color.b = CLAMP(0, b, 255);
   op->color.a = CLAMP(0, a, 255);
}

EOLIAN static void
_efl_gfx_mapping_translate(Eo *eo_obj, Efl_Gfx_Mapping_Data *pd,
                       double dx, double dy, double dz)
{
   Gfx_Map_Op *op;

   // Translate operation does not use a pivot center; it's a global shift.
   op = _gfx_mapping_op_add(eo_obj, pd, GFX_MAPPING_TRANSLATE, NULL, 0, 0, 0, EINA_FALSE);
   if (!op) return;

   op->translate.dx = dx;
   op->translate.dy = dy;
   op->translate.dz = dz;
}

/**
 * @internal
 * @brief Helper function to add a 2D rotation operation.
 *
 * @param eo_obj The Efl_Gfx_Mapping object.
 * @param pd The private data.
 * @param degrees Rotation angle.
 * @param pivot Pivot entity (can be NULL for self-pivot if absolute is EINA_FALSE, or ignored if absolute is EINA_TRUE).
 * @param cx Pivot center X.
 * @param cy Pivot center Y.
 * @param absolute If EINA_TRUE, cx, cy are absolute screen coordinates.
 *                 If EINA_FALSE, cx, cy are relative to the pivot (0.0-1.0).
 */
static inline void
_map_rotate(Eo *eo_obj, Efl_Gfx_Mapping_Data *pd,
            double degrees, const Efl_Gfx_Entity *pivot, double cx, double cy,
            Eina_Bool absolute)
{
   Gfx_Map_Op *op;

   // For 2D rotation, Z component of pivot center is 0.
   op = _gfx_mapping_op_add(eo_obj, pd, GFX_MAPPING_ROTATE_2D, pivot, cx, cy, 0, absolute);
   if (!op) return;

   op->rotate_2d.degrees = degrees;
}

EOLIAN static void
_efl_gfx_mapping_rotate(Eo *eo_obj, Efl_Gfx_Mapping_Data *pd,
                    double degrees, const Efl_Gfx_Entity *pivot, double cx, double cy)
{
   // Relative rotation: cx, cy are relative to the 'pivot' object's geometry.
   // If 'pivot' is NULL or eo_obj, it's relative to self.
   _map_rotate(eo_obj, pd, degrees, pivot, cx, cy, EINA_FALSE);
}

EOLIAN static void
_efl_gfx_mapping_rotate_absolute(Eo *eo_obj, Efl_Gfx_Mapping_Data *pd, double degrees, double cx, double cy)
{
   // Absolute rotation: cx, cy are absolute screen coordinates. Pivot entity is ignored.
   _map_rotate(eo_obj, pd, degrees, NULL, cx, cy, EINA_TRUE);
}

/**
 * @internal
 * @brief Helper function to add a 3D rotation operation.
 *
 * @param eo_obj The Efl_Gfx_Mapping object.
 * @param pd The private data.
 * @param dx Rotation around X-axis.
 * @param dy Rotation around Y-axis.
 * @param dz Rotation around Z-axis.
 * @param pivot Pivot entity.
 * @param cx Pivot center X.
 * @param cy Pivot center Y.
 * @param cz Pivot center Z.
 * @param absolute If EINA_TRUE, cx, cy, cz are absolute screen coordinates.
 */
static inline void
_map_rotate_3d(Eo *eo_obj, Efl_Gfx_Mapping_Data *pd,
               double dx, double dy, double dz,
               const Efl_Gfx_Entity *pivot, double cx, double cy, double cz,
               Eina_Bool absolute)
{
   Gfx_Map_Op *op;

   op = _gfx_mapping_op_add(eo_obj, pd, GFX_MAPPING_ROTATE_3D, pivot, cx, cy, cz, absolute);
   if (!op) return;

   op->rotate_3d.dx = dx;
   op->rotate_3d.dy = dy;
   op->rotate_3d.dz = dz;
}

EOLIAN static void
_efl_gfx_mapping_rotate_3d(Eo *eo_obj, Efl_Gfx_Mapping_Data *pd,
                       double dx, double dy, double dz,
                       const Efl_Gfx_Entity *pivot, double cx, double cy, double cz)
{
   _map_rotate_3d(eo_obj, pd, dx, dy, dz, pivot, cx, cy, cz, EINA_FALSE);
}

EOLIAN static void
_efl_gfx_mapping_rotate_3d_absolute(Eo *eo_obj, Efl_Gfx_Mapping_Data *pd,
                                double dx, double dy, double dz, double cx, double cy, double cz)
{
   _map_rotate_3d(eo_obj, pd, dx, dy, dz, NULL, cx, cy, cz, EINA_TRUE);
}

/**
 * @internal
 * @brief Helper function to add a quaternion rotation operation.
 *
 * @param eo_obj The Efl_Gfx_Mapping object.
 * @param pd The private data.
 * @param qx Quaternion X component.
 * @param qy Quaternion Y component.
 * @param qz Quaternion Z component.
 * @param qw Quaternion W component.
 * @param pivot Pivot entity.
 * @param cx Pivot center X.
 * @param cy Pivot center Y.
 * @param cz Pivot center Z.
 * @param absolute If EINA_TRUE, cx, cy, cz are absolute screen coordinates.
 */
static inline void
_map_rotate_quat(Eo *eo_obj, Efl_Gfx_Mapping_Data *pd,
                 double qx, double qy, double qz, double qw,
                 const Efl_Gfx_Entity *pivot, double cx, double cy, double cz,
                 Eina_Bool absolute)
{
   Gfx_Map_Op *op;

   op = _gfx_mapping_op_add(eo_obj, pd, GFX_MAPPING_ROTATE_QUAT, pivot, cx, cy, cz, absolute);
   if (!op) return;

   op->rotate_quat.qx = qx;
   op->rotate_quat.qy = qy;
   op->rotate_quat.qz = qz;
   op->rotate_quat.qw = qw;
}

EOLIAN static void
_efl_gfx_mapping_rotate_quat(Eo *eo_obj, Efl_Gfx_Mapping_Data *pd,
                         double qx, double qy, double qz, double qw,
                         const Efl_Gfx_Entity *pivot, double cx, double cy, double cz)
{
   _map_rotate_quat(eo_obj, pd, qx, qy, qz, qw, pivot, cx, cy, cz, EINA_FALSE);
}

EOLIAN static void
_efl_gfx_mapping_rotate_quat_absolute(Eo *eo_obj, Efl_Gfx_Mapping_Data *pd,
                                  double qx, double qy, double qz, double qw,
                                  double cx, double cy, double cz)
{
   _map_rotate_quat(eo_obj, pd, qx, qy, qz, qw, NULL, cx, cy, cz, EINA_TRUE);
}

/**
 * @internal
 * @brief Helper function to add a zoom operation.
 *
 * @param eo_obj The Efl_Gfx_Mapping object.
 * @param pd The private data.
 * @param zoomx Zoom factor X.
 * @param zoomy Zoom factor Y.
 * @param pivot Pivot entity.
 * @param cx Pivot center X.
 * @param cy Pivot center Y.
 * @param absolute If EINA_TRUE, cx, cy are absolute screen coordinates.
 */
static inline void
_map_zoom(Eo *eo_obj, Efl_Gfx_Mapping_Data *pd,
          double zoomx, double zoomy,
          const Efl_Gfx_Entity *pivot, double cx, double cy,
          Eina_Bool absolute)
{
   Gfx_Map_Op *op;

   // Zoom is effectively 2D, so pivot Z is 0.
   op = _gfx_mapping_op_add(eo_obj, pd, GFX_MAPPING_ZOOM, pivot, cx, cy, 0, absolute);
   if (!op) return;

   op->zoom.zx = zoomx;
   op->zoom.zy = zoomy;
}

EOLIAN static void
_efl_gfx_mapping_zoom(Eo *eo_obj, Efl_Gfx_Mapping_Data *pd,
                  double zoomx, double zoomy,
                  const Efl_Gfx_Entity *pivot, double cx, double cy)
{
   _map_zoom(eo_obj, pd, zoomx, zoomy, pivot, cx, cy, EINA_FALSE);
}

EOLIAN static void
_efl_gfx_mapping_zoom_absolute(Eo *eo_obj, Efl_Gfx_Mapping_Data *pd,
                           double zoomx, double zoomy, double cx, double cy)
{
   _map_zoom(eo_obj, pd, zoomx, zoomy, NULL, cx, cy, EINA_TRUE);
}

/**
 * @internal
 * @brief Helper function to add a 3D lighting operation.
 *
 * @param eo_obj The Efl_Gfx_Mapping object.
 * @param pd The private data.
 * @param pivot Pivot entity (light position reference).
 * @param lx Light X position.
 * @param ly Light Y position.
 * @param lz Light Z position.
 * @param lr Light red component (0-255).
 * @param lg Light green component (0-255).
 * @param lb Light blue component (0-255).
 * @param ar Ambient red component (0-255).
 * @param ag Ambient green component (0-255).
 * @param ab Ambient blue component (0-255).
 * @param absolute If EINA_TRUE, lx, ly, lz are absolute screen coordinates.
 */
static inline void
_map_lighting_3d(Eo *eo_obj, Efl_Gfx_Mapping_Data *pd,
                  const Efl_Gfx_Entity *pivot, double lx, double ly, double lz,
                  int lr, int lg, int lb, int ar, int ag, int ab,
                  Eina_Bool absolute)
{
   Gfx_Map_Op *op;

   op = _gfx_mapping_op_add(eo_obj, pd, GFX_MAPPING_LIGHTING_3D, pivot, lx, ly, lz, absolute);
   if (!op) return;

   op->lighting_3d.lr = CLAMP(0, lr, 255);
   op->lighting_3d.lg = CLAMP(0, lg, 255);
   op->lighting_3d.lb = CLAMP(0, lb, 255);
   op->lighting_3d.ar = CLAMP(0, ar, 255);
   op->lighting_3d.ag = CLAMP(0, ag, 255);
   op->lighting_3d.ab = CLAMP(0, ab, 255);
}

EOLIAN static void
_efl_gfx_mapping_lighting_3d(Eo *eo_obj, Efl_Gfx_Mapping_Data *pd,
                          const Efl_Gfx_Entity *pivot, double lx, double ly, double lz,
                          int lr, int lg, int lb, int ar, int ag, int ab)
{
   _map_lighting_3d(eo_obj, pd, pivot, lx, ly, lz, lr, lg, lb, ar, ag, ab, EINA_FALSE);
}

EOLIAN static void
_efl_gfx_mapping_lighting_3d_absolute(Eo *eo_obj, Efl_Gfx_Mapping_Data *pd,
                                   double lx, double ly, double lz,
                                   int lr, int lg, int lb, int ar, int ag, int ab)
{
   _map_lighting_3d(eo_obj, pd, NULL, lx, ly, lz, lr, lg, lb, ar, ag, ab, EINA_TRUE);
}

/**
 * @internal
 * @brief Helper function to add a 3D perspective operation.
 *
 * @param eo_obj The Efl_Gfx_Mapping object.
 * @param pd The private data.
 * @param pivot Pivot entity (perspective center reference).
 * @param px Perspective center X.
 * @param py Perspective center Y.
 * @param z0 Vanishing point on Z axis.
 * @param foc Focal length.
 * @param absolute If EINA_TRUE, px, py are absolute screen coordinates.
 */
static inline void
_map_perspective_3d(Eo *eo_obj, Efl_Gfx_Mapping_Data *pd,
                    const Efl_Gfx_Entity *pivot, double px, double py,
                    double z0, double foc,
                    Eina_Bool absolute)
{
   Gfx_Map_Op *op;

   if (foc <= 0.0) // Focal length must be positive
     {
        ERR("Focal length must be greater than 0! Got %f", foc);
        return;
     }

   // Perspective center Z is implicitly 0 for this operation's pivot.
   op = _gfx_mapping_op_add(eo_obj, pd, GFX_MAPPING_PERSPECTIVE_3D, pivot, px, py, 0, absolute);
   if (!op) return;

   op->perspective_3d.z0 = z0;
   op->perspective_3d.foc = foc;
}

EOLIAN static void
_efl_gfx_mapping_perspective_3d(Eo *eo_obj, Efl_Gfx_Mapping_Data *pd,
                            const Efl_Gfx_Entity *pivot, double px, double py,
                            double z0, double foc)
{
   _map_perspective_3d(eo_obj, pd, pivot, px, py, z0, foc, EINA_FALSE);
}

EOLIAN static void
_efl_gfx_mapping_perspective_3d_absolute(Eo *eo_obj, Efl_Gfx_Mapping_Data *pd,
                                     double px, double py, double z0, double foc)
{
   _map_perspective_3d(eo_obj, pd, NULL, px, py, z0, foc, EINA_TRUE);
}

#include "canvas/efl_gfx_mapping.eo.c"
