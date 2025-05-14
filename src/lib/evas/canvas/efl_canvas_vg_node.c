/**
 * @file
 * @brief This file implements the Efl.Canvas.Vg.Node class, which is a base
 * class for all vector graphics nodes in the Evas canvas.
 * It handles properties like transformation, origin, visibility, color,
 * and stacking order.
 */

#include "evas_common_private.h"
#include "evas_private.h"

#include "evas_vg_private.h"

#include <string.h>
#include <math.h>

#define MY_CLASS EFL_CANVAS_VG_NODE_CLASS

static const Efl_Canvas_Vg_Interpolation interpolation_identity = {
  { 0, 0, 0, 1 },
  { 0, 0, 0, 1 },
  { 0, 0, 0 },
  { 1, 1, 1 },
  { 0, 0, 0 }
};

/**
 * @internal
 * @brief Propagates a change notification up the Efl.Canvas.Vg.Node hierarchy.
 *
 * This function is called when a property of a VG node changes. It marks the
 * current node and its ancestors as changed until a node that is already
 * marked as changed or a non-VG_Node parent is encountered.
 * It also triggers a change on the associated Efl.Canvas.Vg.Object if present.
 *
 * @param obj The Efl_VG object that changed.
 * @param nd The private data of the Efl_Canvas_Vg_Node.
 */
static void
_node_change(Efl_VG *obj, Efl_Canvas_Vg_Node_Data *nd)
{
   if (!nd) return;
   if (nd->flags != EFL_GFX_CHANGE_FLAG_NONE)
     {
       if ((nd->vd && nd->vd->obj) &&
           (!nd->vd->obj || !nd->vd->obj->changed))
         efl_canvas_vg_object_change(nd->vd);

       return;
     }
   nd->flags = EFL_GFX_CHANGE_FLAG_ALL;

   Eo *p = obj;
   while ((p = efl_parent_get(p)))
     {
        if (!efl_isa(p, MY_CLASS)) break;
        Efl_Canvas_Vg_Node_Data *pnd = efl_data_scope_get(p, MY_CLASS);
        if (pnd->flags != EFL_GFX_CHANGE_FLAG_NONE) break;
        pnd->flags = EFL_GFX_CHANGE_FLAG_ALL;
     }
   if (!nd->vg_obj || efl_invalidated_get(nd->vg_obj)) return;
   efl_canvas_vg_object_change(nd->vd);
}

/**
 * @internal
 * @brief Sets the 3x3 transformation matrix for the VG node.
 *
 * If a matrix @p m is provided, it's copied to the node's private data.
 * If @p m is NULL, the existing transformation matrix is freed.
 * Any existing interpolation data is cleared as it becomes invalid.
 * Triggers a node change.
 *
 * @param obj The Efl_VG object.
 * @param pd The private data of the Efl_Canvas_Vg_Node.
 * @param m The transformation matrix to set, or NULL to clear.
 */
static void
_efl_canvas_vg_node_transformation_set(Eo *obj,
                                       Efl_Canvas_Vg_Node_Data *pd,
                                       const Eina_Matrix3 *m)
{
   if (pd->intp)
     {
        free(pd->intp);
        pd->intp = NULL;
     }

   if (m)
     {
        if (!pd->m)
          {
             pd->m = malloc(sizeof (Eina_Matrix3));
             if (!pd->m) return;
          }
        memcpy(pd->m, m, sizeof (Eina_Matrix3));
     }
   else
     {
        free(pd->m);
        pd->m = NULL;
     }

   /* NOTE: _node_change function is only executed
            when pd->flags is EFL_GFX_CHANGE_FLAG_NONE to prevent duplicate calls.*/
   _node_change(obj, pd);
   pd->flags |= EFL_GFX_CHANGE_FLAG_MATRIX;
}

/**
 * @internal
 * @brief Gets the 3x3 transformation matrix of the VG node.
 *
 * @param obj The Efl_VG object (unused).
 * @param pd The private data of the Efl_Canvas_Vg_Node.
 * @return A pointer to the internal Eina_Matrix3, or NULL if not set.
 *         The returned matrix should not be modified or freed by the caller.
 */
const Eina_Matrix3 *
_efl_canvas_vg_node_transformation_get(const Eo *obj EINA_UNUSED, Efl_Canvas_Vg_Node_Data *pd)
{
   return pd->m;
}

/**
 * @internal
 * @brief Sets the composition method for the VG node.
 * @warning This function is currently a no-op.
 *
 * @param obj The Efl_VG object (unused).
 * @param pd The private data of the Efl_Canvas_Vg_Node (unused).
 * @param target The target node for composition (unused).
 * @param method The composition method (unused).
 */
static void
_efl_canvas_vg_node_comp_method_set(Eo *obj EINA_UNUSED,
                                    Efl_Canvas_Vg_Node_Data *pd EINA_UNUSED,
                                    Efl_Canvas_Vg_Node *target EINA_UNUSED,
                                    Efl_Gfx_Vg_Composite_Method method EINA_UNUSED)
{
}

/**
 * @internal
 * @brief Sets the origin point (anchor point) of the VG node.
 *
 * Transformations like rotation and scaling are applied relative to this origin.
 * Triggers a node change.
 *
 * @param obj The Efl_VG object.
 * @param pd The private data of the Efl_Canvas_Vg_Node.
 * @param x The x-coordinate of the origin.
 * @param y The y-coordinate of the origin.
 */
static void
_efl_canvas_vg_node_origin_set(Eo *obj,
                               Efl_Canvas_Vg_Node_Data *pd,
                               double x, double y)
{
   pd->x = x;
   pd->y = y;

   _node_change(obj, pd);
}

/**
 * @internal
 * @brief Gets the origin point (anchor point) of the VG node.
 *
 * @param obj The Efl_VG object (unused).
 * @param pd The private data of the Efl_Canvas_Vg_Node.
 * @param x Pointer to store the x-coordinate of the origin. Can be NULL.
 * @param y Pointer to store the y-coordinate of the origin. Can be NULL.
 */
static void
_efl_canvas_vg_node_origin_get(const Eo *obj EINA_UNUSED,
                        Efl_Canvas_Vg_Node_Data *pd,
                        double *x, double *y)
{
   if (x) *x = pd->x;
   if (y) *y = pd->y;
}

/**
 * @internal
 * @brief Sets the position of the VG node (implementation for Efl.Gfx.Entity).
 *
 * This effectively sets the origin of the node.
 * Triggers a node change.
 *
 * @param obj The Efl_VG object.
 * @param pd The private data of the Efl_Canvas_Vg_Node.
 * @param pos The 2D position (Eina_Position2D) to set.
 *            Example: EINA_POSITION2D(10, 20)
 */
static void
_efl_canvas_vg_node_efl_gfx_entity_position_set(Eo *obj,
                                                Efl_Canvas_Vg_Node_Data *pd,
                                                Eina_Position2D pos)
{
   pd->x = (double) pos.x;
   pd->y = (double) pos.y;

   _node_change(obj, pd);
}

/**
 * @internal
 * @brief Gets the position of the VG node (implementation for Efl.Gfx.Entity).
 *
 * This retrieves the origin of the node.
 * @note This function casts the internal double coordinates to integers.
 *
 * @param obj The Efl_VG object (unused).
 * @param pd The private data of the Efl_Canvas_Vg_Node.
 * @return The 2D position (Eina_Position2D) of the node.
 *         Example: { .x = 10, .y = 20 }
 */
static Eina_Position2D
_efl_canvas_vg_node_efl_gfx_entity_position_get(const Eo *obj EINA_UNUSED, Efl_Canvas_Vg_Node_Data *pd)
{
   // NOTE: This casts double to int!
   return EINA_POSITION2D(pd->x, pd->y);
}

/**
 * @internal
 * @brief Sets the visibility of the VG node (implementation for Efl.Gfx.Entity).
 *
 * Triggers a node change.
 *
 * @param obj The Efl_VG object.
 * @param pd The private data of the Efl_Canvas_Vg_Node.
 * @param v EINA_TRUE if visible, EINA_FALSE otherwise.
 */
static void
_efl_canvas_vg_node_efl_gfx_entity_visible_set(Eo *obj,
                                               Efl_Canvas_Vg_Node_Data *pd,
                                               Eina_Bool v)
{
   pd->visibility = v;

   _node_change(obj, pd);
}

/**
 * @internal
 * @brief Gets the visibility of the VG node (implementation for Efl.Gfx.Entity).
 *
 * @param obj The Efl_VG object (unused).
 * @param pd The private data of the Efl_Canvas_Vg_Node.
 * @return EINA_TRUE if visible, EINA_FALSE otherwise.
 */
static Eina_Bool
_efl_canvas_vg_node_efl_gfx_entity_visible_get(const Eo *obj EINA_UNUSED,
                                      Efl_Canvas_Vg_Node_Data *pd)
{
   return pd->visibility;
}

/**
 * @internal
 * @brief Sets the color of the VG node (implementation for Efl.Gfx.Color).
 *
 * Colors are pre-multiplied. If any R, G, B component is greater than A,
 * it's clamped to A, and an error is logged. Alpha (A) is clamped to 0-255.
 * Triggers a node change.
 *
 * @param obj The Efl_VG object.
 * @param pd The private data of the Efl_Canvas_Vg_Node.
 * @param r Red component (0-255).
 * @param g Green component (0-255).
 * @param b Blue component (0-255).
 * @param a Alpha component (0-255).
 */
static void
_efl_canvas_vg_node_efl_gfx_color_color_set(Eo *obj,
                                            Efl_Canvas_Vg_Node_Data *pd,
                                            int r, int g, int b, int a)
{
   Eina_Bool perr = EINA_FALSE;

   //Exception Handling.
   if (r < 0) r = 0;
   if (g < 0) g = 0;
   if (b < 0) b = 0;
   if (a > 255) a = 255;
   else if (a < 0) a = 0;

   if (r > a)
     {
        r = a;
        perr = EINA_TRUE;
     }
   if (g > a)
     {
        g = a;
        perr = EINA_TRUE;
     }
   if (b > a)
     {
        b = a;
        perr = EINA_TRUE;
     }

   if (perr) ERR("Evas only handles pre-multiplied color!");

   pd->r = r;
   pd->g = g;
   pd->b = b;
   pd->a = a;

   _node_change(obj, pd);
}

/**
 * @internal
 * @brief Gets the color of the VG node (implementation for Efl.Gfx.Color).
 *
 * @param obj The Efl_VG object (unused).
 * @param pd The private data of the Efl_Canvas_Vg_Node.
 * @param r Pointer to store the red component. Can be NULL.
 * @param g Pointer to store the green component. Can be NULL.
 * @param b Pointer to store the blue component. Can be NULL.
 * @param a Pointer to store the alpha component. Can be NULL.
 */
static void
_efl_canvas_vg_node_efl_gfx_color_color_get(const Eo *obj EINA_UNUSED,
                                    Efl_Canvas_Vg_Node_Data *pd,
                                    int *r, int *g, int *b, int *a)
{
   if (r) *r = pd->r;
   if (g) *g = pd->g;
   if (b) *b = pd->b;
   if (a) *a = pd->a;
}

/**
 * @internal
 * @brief Gets the size of the VG node (implementation for Efl.Gfx.Entity).
 *
 * This is determined by the bounding box of the node's path.
 *
 * @param obj The Efl_VG object.
 * @param pd The private data of the Efl_Canvas_Vg_Node (unused).
 * @return The 2D size (Eina_Size2D) of the node.
 *         Example: { .w = 100, .h = 50 }
 */
static Eina_Size2D
_efl_canvas_vg_node_efl_gfx_entity_size_get(const Eo *obj, Efl_Canvas_Vg_Node_Data *pd EINA_UNUSED)
{
   Eina_Rect r;

   efl_gfx_path_bounds_get(obj, &r);
   return r.size;
}

/**
 * @internal
 * @brief Gets the geometry (position and size) of the VG node (implementation for Efl.Gfx.Entity).
 *
 * @param obj The Efl_VG object.
 * @param pd The private data of the Efl_Canvas_Vg_Node (unused).
 * @return The Eina_Rect representing the node's geometry.
 *         Example: { .x = 10, .y = 20, .w = 100, .h = 50 }
 */
EOLIAN static Eina_Rect
_efl_canvas_vg_node_efl_gfx_entity_geometry_get(const Eo *obj, Efl_Canvas_Vg_Node_Data *pd EINA_UNUSED)
{
   Eina_Rect r;
   r.pos = efl_gfx_entity_position_get(obj);
   r.size = efl_gfx_entity_size_get(obj);
   return r;
}

/**
 * @internal
 * @brief Checks the parent of a VG node and retrieves its container data if applicable.
 *
 * This function verifies if the parent is a valid type (EFL_CANVAS_VG_CONTAINER_CLASS
 * or EFL_CANVAS_VG_OBJECT_CLASS). If the parent is not of an authorized class,
 * an error is logged, and `*parent` is set to NULL.
 *
 * @param obj The Efl_VG object whose parent is being checked.
 * @param[out] parent Pointer to store the parent Eo object. Will be set to NULL on error or if no valid parent.
 * @param[out] cd Pointer to store the Efl_Canvas_Vg_Container_Data of the parent if it's a container.
 *                Will be set to NULL if the parent is not a container or on error.
 * @return EINA_TRUE if the parent is valid or NULL, EINA_FALSE if the parent is of an unauthorized class.
 */
// Parent should be a container otherwise dismissing the stacking operation
static Eina_Bool
_efl_canvas_vg_node_parent_checked_get(Eo *obj,
                                       Eo **parent,
                                       Efl_Canvas_Vg_Container_Data **cd)
{
   *cd = NULL;
   *parent = efl_parent_get(obj);

   if (efl_isa(*parent, EFL_CANVAS_VG_CONTAINER_CLASS))
     *cd = efl_data_scope_get(*parent, EFL_CANVAS_VG_CONTAINER_CLASS);
   else if (efl_isa(*parent, EFL_CANVAS_VG_OBJECT_CLASS))
     *parent = NULL;
   else if (*parent)
     {
        ERR("Parent of unauthorized class '%s'.", efl_class_name_get(efl_class_get(*parent)));
        *parent = NULL;
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Constructor for Efl_Canvas_Vg_Node.
 *
 * Initializes the node, checks its parent, and sets up initial state
 * (e.g., visibility, change flags). Inherits vg_obj and vd from parent if available.
 *
 * @param obj The Efl_VG object being constructed.
 * @param nd The private data of the Efl_Canvas_Vg_Node.
 * @return The constructed Eo object, or NULL on failure.
 */
static Eo *
_efl_canvas_vg_node_efl_object_constructor(Eo *obj,
                                           Efl_Canvas_Vg_Node_Data *nd)
{
   Efl_Canvas_Vg_Container_Data *cd;
   Eo *parent;

   obj = efl_constructor(efl_super(obj, MY_CLASS));

   if (!_efl_canvas_vg_node_parent_checked_get(obj, &parent, &cd))
     {
        ERR("Failed");
        return NULL;
     }

   if (parent)
     {
        // If parent is a Vg.Node (e.g. Vg.Container), inherit its Vg.Object context.
        // This is important for _node_change propagation.
        Efl_Canvas_Vg_Node_Data *parent_nd =
           efl_data_scope_get(parent, MY_CLASS);
        nd->vg_obj = parent_nd->vg_obj;
        nd->vd = parent_nd->vd;
     }

   nd->flags = EFL_GFX_CHANGE_FLAG_ALL;
   nd->changed = EINA_TRUE;
   nd->visibility = EINA_TRUE;

   return obj;
}

/**
 * @internal
 * @brief Invalidation handler for Efl_Canvas_Vg_Node.
 *
 * Releases any renderer associated with the node.
 *
 * @param obj The Efl_VG object being invalidated.
 * @param pd The private data of the Efl_Canvas_Vg_Node.
 */
static void
_efl_canvas_vg_node_efl_object_invalidate(Eo *obj, Efl_Canvas_Vg_Node_Data *pd)
{
   if (pd->renderer)
     {
        efl_unref(pd->renderer);
        pd->renderer = NULL;
     }

   efl_invalidate(efl_super(obj, MY_CLASS));
}

/**
 * @internal
 * @brief Destructor for Efl_Canvas_Vg_Node.
 *
 * Frees allocated resources like the transformation matrix and interpolation data.
 *
 * @param obj The Efl_VG object being destructed.
 * @param pd The private data of the Efl_Canvas_Vg_Node.
 */
static void
_efl_canvas_vg_node_efl_object_destructor(Eo *obj, Efl_Canvas_Vg_Node_Data *pd)
{
   if (pd->m)
     {
        free(pd->m);
        pd->m = NULL;
     }
   if (pd->intp)
     {
        free(pd->intp);
        pd->intp = NULL;
     }

   efl_destructor(efl_super(obj, MY_CLASS));
}

/**
 * @internal
 * @brief Inserts the node's name into its parent container's name hash.
 *
 * If the name already exists in the hash for a different object, an error is logged,
 * and the current object's name is cleared.
 *
 * @param obj The Efl_VG node object.
 * @param cd The private data of the parent Efl_Canvas_Vg_Container.
 */
static void
_efl_canvas_vg_node_name_insert(Eo *obj, Efl_Canvas_Vg_Container_Data *cd)
{
   Eo *set;
   const char *name = efl_name_get(efl_super(obj, MY_CLASS));
   if (!name) return;

   set = eina_hash_find(cd->names, name);
   if (set == obj) return;

   if (set)
     {
        ERR("node name(%s) is already exist in container but child node(%p) is different...", name, obj);
        efl_name_set(efl_super(obj, MY_CLASS), NULL);
     }
   else
     {
        eina_hash_direct_add(cd->names, name, obj);
     }
}

/**
 * @internal
 * @brief Sets the name of the VG node (implementation for Efl.Object).
 *
 * Updates the parent container's name hash by removing the old name (if any)
 * and inserting the new name.
 *
 * @param obj The Efl_VG node object.
 * @param pd The private data of the Efl_Canvas_Vg_Node (unused).
 * @param name The new name to set for the node.
 */
static void
_efl_canvas_vg_node_efl_object_name_set(Eo *obj, Efl_Canvas_Vg_Node_Data *pd EINA_UNUSED, const char *name)
{
   Efl_Canvas_Vg_Container_Data *cd;
   Eo *parent;
   const char *pname = efl_name_get(obj);

   if (_efl_canvas_vg_node_parent_checked_get(obj, &parent, &cd))
     {
        if (pname) eina_hash_del(cd->names, pname, obj);
     }

   efl_name_set(efl_super(obj, MY_CLASS), name);

   if (cd) _efl_canvas_vg_node_name_insert(obj, cd);
}

/**
 * @internal
 * @brief Sets the parent of the VG node (implementation for Efl.Object).
 *
 * Handles reparenting logic, including:
 * - Validating the new parent type.
 * - Updating the node's `vg_obj` and `vd` if the parent is an Efl.Canvas.Vg.Object.
 * - Removing the node from its old parent container's children list and name hash.
 * - Adding the node to the new parent container's children list and name hash.
 * - Propagating `vg_obj` and `vd` from the new container parent if necessary.
 * - Triggering a node change.
 *
 * @param obj The Efl_VG node object.
 * @param nd The private data of the Efl_Canvas_Vg_Node.
 * @param parent The new parent Eo object.
 */
static void
_efl_canvas_vg_node_efl_object_parent_set(Eo *obj,
                                          Efl_Canvas_Vg_Node_Data *nd,
                                          Eo *parent)
{
   Efl_Canvas_Vg_Container_Data *cd = NULL;
   Efl_Canvas_Vg_Container_Data *old_cd;
   Efl_VG *old_parent;

   if (parent)
     {
        if (efl_isa(parent, EFL_CANVAS_VG_CONTAINER_CLASS))
          cd = efl_data_scope_get(parent, EFL_CANVAS_VG_CONTAINER_CLASS);
        else if (efl_isa(parent, EFL_CANVAS_VG_OBJECT_CLASS))
          {
             if (nd->vg_obj != parent)
               {
                  nd->vg_obj = parent;
                  nd->vd = efl_data_scope_get(parent, EFL_CANVAS_VG_OBJECT_CLASS);
               }
          }
        else
          {
             ERR("parent(%p, class = %s) is not allowed by vg node(%p).",
                 parent, efl_class_name_get(efl_class_get(parent)), obj);
             return;
          }
     }
   else
     {
        nd->vg_obj = NULL;
        nd->vd = NULL;
     }

   if (!_efl_canvas_vg_node_parent_checked_get(obj, &old_parent, &old_cd))
     return;

   // FIXME: this may become slow with to much object
   if (old_cd)
     {
        old_cd->children = eina_list_remove(old_cd->children, obj);
        eina_hash_del(old_cd->names, efl_name_get(efl_super(obj, MY_CLASS)), obj);
        _node_change(old_parent, efl_data_scope_get(old_parent, MY_CLASS));
     }

   efl_parent_set(efl_super(obj, MY_CLASS), parent);

   if (cd)
     {
        cd->children = eina_list_append(cd->children, obj);
        _efl_canvas_vg_node_name_insert(obj, cd);

        Efl_Canvas_Vg_Node_Data *parent_nd = efl_data_scope_get(parent, MY_CLASS);
        if (nd->vg_obj != parent_nd->vg_obj)
          {
             nd->vg_obj = parent_nd->vg_obj;
             nd->vd = parent_nd->vd;
          }
     }

   if (parent) _node_change(obj, nd);
}

/**
 * @internal
 * @brief Raises the VG node to the top of its parent container's stacking order
 * (implementation for Efl.Gfx.Stack).
 *
 * If the parent is not an Efl_Canvas_Vg_Container or the node is already at the top,
 * this function does nothing. Otherwise, it moves the node to the end of the
 * parent's children list and triggers a change notification on the parent.
 *
 * @param obj The Efl_VG node object.
 * @param pd The private data of the Efl_Canvas_Vg_Node (unused).
 */
static void
_efl_canvas_vg_node_efl_gfx_stack_raise_to_top(Eo *obj, Efl_Canvas_Vg_Node_Data *pd EINA_UNUSED)
{
   Efl_Canvas_Vg_Node *parent = efl_parent_get(obj);
   if (!efl_isa(parent, EFL_CANVAS_VG_CONTAINER_CLASS)) return;

   Efl_Canvas_Vg_Container_Data *cd = efl_data_scope_get(parent, EFL_CANVAS_VG_CONTAINER_CLASS);
   if (!cd->children) return;
   if (eina_list_data_get(eina_list_last(cd->children)) == obj) return; // Already at top
   cd->children = eina_list_remove(cd->children, obj);
   cd->children = eina_list_append(cd->children, obj);
   _node_change(parent, efl_data_scope_get(parent, MY_CLASS));
}

/**
 * @internal
 * @brief Stacks the VG node above a specified sibling node (implementation for Efl.Gfx.Stack).
 *
 * If the parent is not an Efl_Canvas_Vg_Container, or if either this node or the
 * `above` node are not children of the same parent, an error occurs.
 * Otherwise, this node is moved in the parent's children list to be immediately
 * after the `above` node. Triggers a change notification on the parent.
 *
 * @param obj The Efl_VG node object.
 * @param pd The private data of the Efl_Canvas_Vg_Node (unused).
 * @param above The sibling Efl_Gfx_Stack object to stack this node above.
 */
static void
_efl_canvas_vg_node_efl_gfx_stack_stack_above(Eo *obj,
                                              Efl_Canvas_Vg_Node_Data *pd EINA_UNUSED,
                                              Efl_Gfx_Stack *above)
{
   Efl_Canvas_Vg_Container_Data *cd;
   Eina_List *lookup, *ref;
   Eo *parent;

   parent = efl_parent_get(obj);
   if (!efl_isa(parent, EFL_CANVAS_VG_CONTAINER_CLASS)) goto on_error;
   cd = efl_data_scope_get(parent, EFL_CANVAS_VG_CONTAINER_CLASS);

   // FIXME: this could become slow with to much object
   lookup = eina_list_data_find_list(cd->children, obj);
   if (!lookup) goto on_error;

   ref = eina_list_data_find_list(cd->children, above);
   if (!ref) goto on_error;

   cd->children = eina_list_remove_list(cd->children, lookup);
   cd->children = eina_list_append_relative_list(cd->children, obj, ref);

   _node_change(parent, efl_data_scope_get(parent, MY_CLASS));
   return;

 on_error:
   ERR("Err");
}

/**
 * @internal
 * @brief Stacks the VG node below a specified sibling node (implementation for Efl.Gfx.Stack).
 *
 * If the parent is not an Efl_Canvas_Vg_Container, or if either this node or the
 * `below` node are not children of the same parent, an error occurs.
 * Otherwise, this node is moved in the parent's children list to be immediately
 * before the `below` node. Triggers a change notification on the parent.
 *
 * @param obj The Efl_VG node object.
 * @param pd The private data of the Efl_Canvas_Vg_Node (unused).
 * @param below The sibling Efl_Gfx_Stack object to stack this node below.
 */
static void
_efl_canvas_vg_node_efl_gfx_stack_stack_below(Eo *obj,
                                              Efl_Canvas_Vg_Node_Data *pd EINA_UNUSED,
                                              Efl_Gfx_Stack *below)
{
   Efl_Canvas_Vg_Container_Data *cd;
   Eina_List *lookup, *ref;
   Eo *parent;

   parent = efl_parent_get(obj);
   if (!efl_isa(parent, EFL_CANVAS_VG_CONTAINER_CLASS)) goto on_error;
   cd = efl_data_scope_get(parent, EFL_CANVAS_VG_CONTAINER_CLASS);

   // FIXME: this could become slow with to much object
   lookup = eina_list_data_find_list(cd->children, obj);
   if (!lookup) goto on_error;

   ref = eina_list_data_find_list(cd->children, below);
   if (!ref) goto on_error;

   cd->children = eina_list_remove_list(cd->children, lookup);
   cd->children = eina_list_prepend_relative_list(cd->children, obj, ref);

   _node_change(parent, efl_data_scope_get(parent, MY_CLASS));
   return;

 on_error:
   ERR("Err");
}

/**
 * @internal
 * @brief Lowers the VG node to the bottom of its parent container's stacking order
 * (implementation for Efl.Gfx.Stack).
 *
 * If the parent is not an Efl_Canvas_Vg_Container or the node is already at the bottom,
 * this function does nothing. Otherwise, it moves the node to the beginning of the
 * parent's children list and triggers a change notification on the parent.
 *
 * @param obj The Efl_VG node object.
 * @param pd The private data of the Efl_Canvas_Vg_Node (unused).
 */
static void
_efl_canvas_vg_node_efl_gfx_stack_lower_to_bottom(Eo *obj, Efl_Canvas_Vg_Node_Data *pd EINA_UNUSED)
{
   Efl_Canvas_Vg_Node *parent = efl_parent_get(obj);
   if (!efl_isa(parent, EFL_CANVAS_VG_CONTAINER_CLASS)) return;

   Efl_Canvas_Vg_Container_Data *cd = efl_data_scope_get(parent, EFL_CANVAS_VG_CONTAINER_CLASS);
   if (!cd->children) return;
   if (eina_list_data_get(cd->children) == obj) return; // Already at bottom
   cd->children = eina_list_remove(cd->children, obj);
   cd->children = eina_list_prepend(cd->children, obj);
   _node_change(parent, efl_data_scope_get(parent, MY_CLASS));
}

/**
 * @internal
 * @brief Gets the sibling node immediately below this VG node in the stacking order
 * (implementation for Efl.Gfx.Stack).
 *
 * @param obj The Efl_VG node object.
 * @param pd The private data of the Efl_Canvas_Vg_Node (unused).
 * @return The Efl_Gfx_Stack object below this node, or NULL if none (e.g., this node is at the bottom,
 *         or not in a valid container).
 */
static Efl_Gfx_Stack *
_efl_canvas_vg_node_efl_gfx_stack_below_get(const Eo *obj, Efl_Canvas_Vg_Node_Data *pd EINA_UNUSED)
{
   Eo *parent, *below;
   const Eina_List *list;

   parent = efl_parent_get(obj);
   if (!efl_isa(parent, EFL_CANVAS_VG_CONTAINER_CLASS)) goto on_error;

   list = efl_canvas_vg_container_children_direct_get(parent);
   if (list == NULL) goto on_error;

   list = eina_list_data_find_list(list, obj);
   if (list == NULL) goto on_error; // Should not happen if parent is correct

   list = eina_list_prev(list);
   if (list == NULL) goto on_error; // obj is the first child

   below = list->data;

   return below;

 on_error:
   return NULL;
}

/**
 * @internal
 * @brief Gets the sibling node immediately above this VG node in the stacking order
 * (implementation for Efl.Gfx.Stack).
 *
 * @param obj The Efl_VG node object.
 * @param pd The private data of the Efl_Canvas_Vg_Node (unused).
 * @return The Efl_Gfx_Stack object above this node, or NULL if none (e.g., this node is at the top,
 *         or not in a valid container).
 */
static Efl_Gfx_Stack *
_efl_canvas_vg_node_efl_gfx_stack_above_get(const Eo *obj, Efl_Canvas_Vg_Node_Data *pd EINA_UNUSED)
{
   Eo *parent, *above;
   const Eina_List *list;

   parent = efl_parent_get(obj);
   if (!efl_isa(parent, EFL_CANVAS_VG_CONTAINER_CLASS)) goto on_error;

   list = efl_canvas_vg_container_children_direct_get(parent);
   if (list == NULL) goto on_error;

   list = eina_list_data_find_list(list, obj);
   if (list == NULL) goto on_error; // Should not happen if parent is correct

   list = eina_list_next(list);
   if (list == NULL) goto on_error; // obj is the last child

   above = list->data;

   return above;

 on_error:
   return NULL;
}

/**
 * @internal
 * @brief Retrieves or computes the interpolation data for a VG node's transformation.
 *
 * If interpolation data (rotation, perspective, translation, scale, skew)
 * already exists in `pd->intp`, it's returned. Otherwise, it's computed
 * from the node's transformation matrix `pd->m`, stored in `pd->intp`, and then returned.
 * If `pd->m` is NULL, or if matrix decomposition fails, NULL is returned.
 * The returned pointer is owned by the node data and should not be freed by the caller.
 *
 * @param pd The private data of the Efl_Canvas_Vg_Node.
 * @return A pointer to Efl_Canvas_Vg_Interpolation data, or NULL on failure or if no transform.
 */
static Efl_Canvas_Vg_Interpolation *
_efl_canvas_vg_node_interpolation_get(Efl_Canvas_Vg_Node_Data *pd)
{
   Eina_Matrix4 m;

   if (!pd->m) return NULL;
   if (pd->intp) return pd->intp;

   pd->intp = calloc(1, sizeof (Efl_Canvas_Vg_Interpolation));
   if (!pd->intp) return NULL;

   eina_matrix3_matrix4_to(&m, pd->m);

   if (eina_matrix4_quaternion_to(&pd->intp->rotation,
                                  &pd->intp->perspective,
                                  &pd->intp->translation,
                                  &pd->intp->scale,
                                  &pd->intp->skew,
                                  &m))
     return pd->intp;

   free(pd->intp);
   pd->intp = NULL;

   return NULL;
}

/**
 * @internal
 * @brief Linearly interpolates between two 3D points.
 *
 * Computes `d = a * from_map + b * pos_map`.
 *
 * @param[out] d The resulting interpolated point.
 * @param a The starting point (corresponds to `from_map` weight).
 * @param b The ending point (corresponds to `pos_map` weight).
 * @param pos_map The interpolation factor for point `b` (typically 0.0 to 1.0).
 * @param from_map The interpolation factor for point `a` (typically `1.0 - pos_map`).
 */
static inline void
_efl_canvas_vg_node_interpolate_point(Eina_Point_3D *d,
                                      const Eina_Point_3D *a, const Eina_Point_3D *b,
                                      double pos_map, double from_map)
{
   d->x = a->x * from_map + b->x * pos_map;
   d->y = a->y * from_map + b->y * pos_map;
   d->z = a->z * from_map + b->z * pos_map;
}

/**
 * @internal
 * @brief Interpolates the properties of this VG node between two other VG nodes.
 * (implementation for Efl.Gfx.Path).
 *
 * @warning Node itself doesn't have any path. This function only interpolates
 *          node-specific properties like transformation, position, color, and visibility.
 *          It does not call the superclass (Efl.Gfx.Path)'s interpolate method.
 *
 * Interpolates:
 * - Transformation matrix (using Slerp for rotation and Lerp for other components).
 * - Position (origin x, y).
 * - Color (r, g, b, a).
 * - Visibility (takes `to`'s visibility if `pos_map >= 0.5`, else `from`'s).
 *
 * Clears any existing renderer on the target node `obj`.
 * Triggers a node change on `obj`.
 *
 * @param obj The Efl_VG node object to store the interpolated state.
 * @param pd The private data of `obj`.
 * @param from The starting Efl_VG node for interpolation. Must be an Efl_Canvas_Vg_Node.
 * @param to The ending Efl_VG node for interpolation. Must be an Efl_Canvas_Vg_Node.
 * @param pos_map The interpolation factor (0.0 for `from`, 1.0 for `to`).
 * @return EINA_TRUE if interpolation was successful, EINA_FALSE if `from` or `to`
 *         are not Efl_Canvas_Vg_Node instances.
 */
/* Warning! Node itself doesn't have any path. Don't call super class(Path)'s */
static Eina_Bool
_efl_canvas_vg_node_efl_gfx_path_interpolate(Eo *obj,
                                             Efl_Canvas_Vg_Node_Data *pd,
                                             const Efl_VG *from,
                                             const Efl_VG *to,
                                             double pos_map)
{
   Efl_Canvas_Vg_Node_Data *fromd, *tod;
   double from_map;

   //Check if both objects have same type
   if (!(efl_isa(from, MY_CLASS) && efl_isa(to, MY_CLASS)))
     return EINA_FALSE;

   fromd = efl_data_scope_get(from, MY_CLASS);
   tod = efl_data_scope_get(to, MY_CLASS);
   from_map = 1.0 - pos_map;

   efl_unref(pd->renderer);
   pd->renderer = NULL;

   //Interpolates Node Transform Matrix
   if (fromd->m || tod->m)
     {
        if (!pd->m) pd->m = malloc(sizeof (Eina_Matrix3));
        if (pd->m)
          {
             const Efl_Canvas_Vg_Interpolation *fi, *ti;
             Efl_Canvas_Vg_Interpolation result;
             Eina_Matrix4 m;

             fi = _efl_canvas_vg_node_interpolation_get(fromd);
             if (!fi) fi = &interpolation_identity;

             ti = _efl_canvas_vg_node_interpolation_get(tod);
             if (!ti) ti = &interpolation_identity;

             eina_quaternion_slerp(&result.rotation,
                                   &fi->rotation, &ti->rotation,
                                   pos_map);
             _efl_canvas_vg_node_interpolate_point(&result.translation,
                                       &fi->translation, &ti->translation,
                                       pos_map, from_map);
             _efl_canvas_vg_node_interpolate_point(&result.scale,
                                       &fi->scale, &ti->scale,
                                       pos_map, from_map);
             _efl_canvas_vg_node_interpolate_point(&result.skew,
                                       &fi->skew, &ti->skew,
                                       pos_map, from_map);

             result.perspective.x =
                fi->perspective.x * from_map + ti->perspective.x * pos_map;
             result.perspective.y =
                fi->perspective.y * from_map + ti->perspective.y * pos_map;
             result.perspective.z =
                fi->perspective.z * from_map + ti->perspective.z * pos_map;
             result.perspective.w =
                fi->perspective.w * from_map + ti->perspective.w * pos_map;

             eina_quaternion_matrix4_to(&m,
                                        &result.rotation,
                                        &result.perspective,
                                        &result.translation,
                                        &result.scale,
                                        &result.skew);

             eina_matrix4_matrix3_to(pd->m, &m);
          }
     }

   //Position
   pd->x = fromd->x * from_map + tod->x * pos_map;
   pd->y = fromd->y * from_map + tod->y * pos_map;

   //Color
   pd->r = fromd->r * from_map + tod->r * pos_map;
   pd->g = fromd->g * from_map + tod->g * pos_map;
   pd->b = fromd->b * from_map + tod->b * pos_map;
   pd->a = fromd->a * from_map + tod->a * pos_map;

   pd->visibility = pos_map >= 0.5 ? tod->visibility : fromd->visibility;

   _node_change(obj, pd);

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Sets the associated Efl.Canvas.Vg.Object for a VG node and its hierarchy.
 *
 * This function updates the `vg_obj` (the root Efl.Canvas.Vg.Object) and `vd`
 * (its private data) for the given `node`. It then recursively calls
 * `efl_canvas_vg_container_vg_obj_update` to propagate this information to
 * any children if the `node` is a container. This ensures that all nodes
 * within a VG scene tree are aware of their root Vg.Object, which is crucial
 * for change propagation and rendering.
 *
 * @param node The Efl_VG node (typically the root of a scene or a sub-scene).
 * @param vg_obj The Efl_Canvas_Vg_Object to associate with this node and its descendants.
 * @param vd The private data of the `vg_obj`.
 */
void
efl_canvas_vg_node_vg_obj_set(Efl_VG *node, Efl_VG *vg_obj, Efl_Canvas_Vg_Object_Data *vd)
{
   Efl_Canvas_Vg_Node_Data *nd = efl_data_scope_get(node, MY_CLASS);
   if (nd->vg_obj == vg_obj) return;
   nd->vg_obj = vg_obj;
   nd->vd = vd;

   //root node is always container.
   // This will recursively update children if 'node' is a container.
   efl_canvas_vg_container_vg_obj_update(node, nd);
}

/**
 * @internal
 * @brief Triggers a change notification for a given VG node.
 *
 * This is a public wrapper around the internal `_node_change` function.
 *
 * @param obj The Efl_VG node object that has changed.
 */
void
efl_canvas_vg_node_change(Eo *obj)
{
   if (!obj) return;
   _node_change(obj, efl_data_scope_get(obj, EFL_CANVAS_VG_NODE_CLASS));
}

/**
 * @internal
 * @brief Duplicates a VG node (implementation for Efl.Duplicate).
 *
 * Creates a new instance of the same class as `obj` and copies properties:
 * - Name
 * - Transformation matrix (if any)
 * - Origin (x, y)
 * - Color (r, g, b, a)
 * - Visibility
 *
 * The new node is not parented.
 *
 * @param obj The Efl_VG node object to duplicate.
 * @param pd The private data of `obj`.
 * @return A new, unparented Efl_VG node that is a copy of `obj`, or NULL on failure.
 *         The returned object has a reference count of 1.
 */
EOLIAN static Efl_VG *
_efl_canvas_vg_node_efl_duplicate_duplicate(const Eo *obj, Efl_Canvas_Vg_Node_Data *pd)
{
   Efl_VG *node;
   Efl_Canvas_Vg_Node_Data *nd;

   node = efl_add_ref(efl_class_get(obj), NULL); // Creates new instance with ref count 1
   nd = efl_data_scope_get(node, MY_CLASS);

   //Hmm...? // Copy name
   efl_name_set(efl_super(node, MY_CLASS), efl_name_get(efl_super(obj, MY_CLASS)));

   if (pd->m) // Copy transformation matrix
     {
        nd->m = malloc(sizeof(Eina_Matrix3));
        if (nd->m) memcpy(nd->m, pd->m, sizeof(Eina_Matrix3));
     }

   // Copy other properties
   nd->x = pd->x;
   nd->y = pd->y;
   nd->r = pd->r;
   nd->g = pd->g;
   nd->b = pd->b;
   nd->a = pd->a;
   nd->visibility = pd->visibility;

   return node;
}

/**
 * @brief Gets the visibility of a VG node. (Legacy Evas API)
 * @param obj The VG node.
 * @return EINA_TRUE if visible, EINA_FALSE otherwise.
 * @deprecated Use efl_gfx_entity_visible_get() instead.
 */
EVAS_API Eina_Bool
evas_vg_node_visible_get(Evas_Vg_Node *obj)
{
   return efl_gfx_entity_visible_get(obj);
}

/**
 * @brief Sets the visibility of a VG node. (Legacy Evas API)
 * @param obj The VG node.
 * @param v EINA_TRUE to make visible, EINA_FALSE to make invisible.
 * @deprecated Use efl_gfx_entity_visible_set() instead.
 */
EVAS_API void
evas_vg_node_visible_set(Evas_Vg_Node *obj, Eina_Bool v)
{
   efl_gfx_entity_visible_set(obj, v);
}

/**
 * @brief Gets the color of a VG node. (Legacy Evas API)
 * @param obj The VG node.
 * @param r Pointer to store the red component (0-255). Can be NULL.
 * @param g Pointer to store the green component (0-255). Can be NULL.
 * @param b Pointer to store the blue component (0-255). Can be NULL.
 * @param a Pointer to store the alpha component (0-255). Can be NULL.
 * @deprecated Use efl_gfx_color_get() instead.
 */
EVAS_API void
evas_vg_node_color_get(Evas_Vg_Node *obj, int *r, int *g, int *b, int *a)
{
   efl_gfx_color_get(obj, r, g, b, a);
}

/**
 * @brief Sets the color of a VG node. (Legacy Evas API)
 * Colors are pre-multiplied.
 * @param obj The VG node.
 * @param r Red component (0-255).
 * @param g Green component (0-255).
 * @param b Blue component (0-255).
 * @param a Alpha component (0-255).
 * @deprecated Use efl_gfx_color_set() instead.
 */
EVAS_API void
evas_vg_node_color_set(Evas_Vg_Node *obj, int r, int g, int b, int a)
{
   efl_gfx_color_set(obj, r, g, b, a);
}

/**
 * @brief Gets the geometry (position and size) of a VG node. (Legacy Evas API)
 * @param obj The VG node.
 * @param x Pointer to store the x-coordinate. Can be NULL.
 * @param y Pointer to store the y-coordinate. Can be NULL.
 * @param w Pointer to store the width. Can be NULL.
 * @param h Pointer to store the height. Can be NULL.
 * @deprecated Use efl_gfx_entity_geometry_get() or individual efl_gfx_entity_position_get()
 *             and efl_gfx_entity_size_get() instead.
 */
EVAS_API void
evas_vg_node_geometry_get(Evas_Vg_Node *obj, int *x, int *y, int *w, int *h)
{
   Eina_Rect r;
   r.pos = efl_gfx_entity_position_get(obj);
   r.size = efl_gfx_entity_size_get(obj);
   if (x) *x = r.x;
   if (y) *y = r.y;
   if (w) *w = r.w;
   if (h) *h = r.h;
}

/**
 * @brief Sets the geometry (position and size) of a VG node. (Legacy Evas API)
 * @param obj The VG node.
 * @param x The x-coordinate.
 * @param y The y-coordinate.
 * @param w The width.
 * @param h The height.
 * @deprecated Use efl_gfx_entity_position_set() and efl_gfx_entity_size_set() instead.
 *             Note that setting size on a generic VG node might not have the intended effect
 *             as size is usually derived from content (like path bounds).
 */
/* deprecated */
EVAS_API void
evas_vg_node_geometry_set(Evas_Vg_Node *obj, int x, int y, int w, int h)
{
   efl_gfx_entity_position_set(obj, EINA_POSITION2D(x, y));
   efl_gfx_entity_size_set(obj, EINA_SIZE2D(w,  h));
}

/**
 * @brief Stacks a VG node below a specified sibling. (Legacy Evas API)
 * @param obj The VG node to stack.
 * @param below The sibling node to stack `obj` below.
 * @deprecated Use efl_gfx_stack_below() instead.
 */
EVAS_API void
evas_vg_node_stack_below(Evas_Vg_Node *obj, Eo *below)
{
   efl_gfx_stack_below(obj, below);
}

/**
 * @brief Stacks a VG node above a specified sibling. (Legacy Evas API)
 * @param obj The VG node to stack.
 * @param above The sibling node to stack `obj` above.
 * @deprecated Use efl_gfx_stack_above() instead.
 */
EVAS_API void
evas_vg_node_stack_above(Evas_Vg_Node *obj, Eo *above)
{
   efl_gfx_stack_above(obj, above);
}

/**
 * @brief Raises a VG node to the top of its parent's stacking order. (Legacy Evas API)
 * @param obj The VG node to raise.
 * @deprecated Use efl_gfx_stack_raise_to_top() instead.
 */
EVAS_API void
evas_vg_node_raise(Evas_Vg_Node *obj)
{
   efl_gfx_stack_raise_to_top(obj);
}

/**
 * @brief Lowers a VG node to the bottom of its parent's stacking order. (Legacy Evas API)
 * @param obj The VG node to lower.
 * @deprecated Use efl_gfx_stack_lower_to_bottom() instead.
 */
EVAS_API void
evas_vg_node_lower(Evas_Vg_Node *obj)
{
   efl_gfx_stack_lower_to_bottom(obj);
}

#include "efl_canvas_vg_node.eo.c"
#include "efl_canvas_vg_node_eo.legacy.c"
