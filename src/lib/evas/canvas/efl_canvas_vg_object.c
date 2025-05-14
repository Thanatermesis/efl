/**
 * @file
 * @brief This file contains the implementation of the Efl_Canvas_Vg_Object.
 *
 * It handles the rendering, manipulation, and file operations for vector graphics objects
 * within the Evas canvas.
 */

#include "evas_common_private.h"
#include "evas_private.h"

#include "evas_vg_private.h"

#define MY_CLASS EFL_CANVAS_VG_OBJECT_CLASS

/* private magic number for vector objects */
static const char o_type[] = "vectors";

const char *o_vg_type = o_type;

/**
 * @brief Renders the vector graphics object.
 * @param eo_obj The Evas object.
 * @param obj The protected data of the Evas object.
 * @param type_private_data The private data specific to this object type.
 * @param engine The rendering engine.
 * @param output The rendering output.
 * @param context The drawing context.
 * @param surface The target surface for rendering.
 * @param x The x-coordinate offset for rendering.
 * @param y The y-coordinate offset for rendering.
 * @param do_async Whether to perform rendering asynchronously.
 */
static void _efl_canvas_vg_object_render(Evas_Object *eo_obj,
                                         Evas_Object_Protected_Data *obj,
                                         void *type_private_data,
                                         void *engine, void *output, void *context, void *surface,
                                         int x, int y, Eina_Bool do_async);
/**
 * @brief Performs pre-render operations for the vector graphics object.
 * @param eo_obj The Evas object.
 * @param obj The protected data of the Evas object.
 * @param type_private_data The private data specific to this object type.
 */
static void _efl_canvas_vg_object_render_pre(Evas_Object *eo_obj,
                                             Evas_Object_Protected_Data *obj,
                                             void *type_private_data);
/**
 * @brief Performs post-render operations for the vector graphics object.
 * @param eo_obj The Evas object.
 * @param obj The protected data of the Evas object.
 * @param type_private_data The private data specific to this object type.
 */
static void _efl_canvas_vg_object_render_post(Evas_Object *eo_obj,
                                              Evas_Object_Protected_Data *obj,
                                              void *type_private_data);
/**
 * @brief Checks if the vector graphics object is currently opaque.
 * @param eo_obj The Evas object.
 * @param obj The protected data of the Evas object.
 * @param type_private_data The private data specific to this object type.
 * @return 1 if opaque, 0 otherwise.
 */
static int _efl_canvas_vg_object_is_opaque(Evas_Object *eo_obj,
                                           Evas_Object_Protected_Data *obj,
                                           void *type_private_data);
/**
 * @brief Checks if the vector graphics object was opaque in the previous state.
 * @param eo_obj The Evas object.
 * @param obj The protected data of the Evas object.
 * @param type_private_data The private data specific to this object type.
 * @return 1 if it was opaque, 0 otherwise.
 */
static int _efl_canvas_vg_object_was_opaque(Evas_Object *eo_obj,
                                            Evas_Object_Protected_Data *obj,
                                            void *type_private_data);

/**
 * @brief Structure defining the Evas object functions for vector graphics objects.
 */
static const Evas_Object_Func object_func =
{
   /* methods (compulsory) */
   NULL,
   _efl_canvas_vg_object_render,
   _efl_canvas_vg_object_render_pre,
   _efl_canvas_vg_object_render_post,
   NULL,
   /* these are optional. NULL = nothing */
   NULL,
   NULL,
   _efl_canvas_vg_object_is_opaque,
   _efl_canvas_vg_object_was_opaque,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL // render_prepare
};

/**
 * @brief Updates the viewport transformation of the vector graphics tree.
 *
 * This function calculates and applies a transformation matrix to the root node
 * of the vector graphics tree based on the object's size, viewbox, fill mode,
 * and alignment. It ensures the vector graphic scales and aligns correctly
 * within the object's bounds.
 *
 * @param obj The Evas object.
 * @param pd The private data of the Efl_Canvas_Vg_Object.
 */
static void
_update_vgtree_viewport(Eo *obj, Efl_Canvas_Vg_Object_Data *pd)
{
   double vb_w, vb_h, vp_w, vp_h, scale_w, scale_h, scale;
   Eina_Size2D sz = efl_gfx_entity_size_get(obj);
   Eina_Matrix3 m;

   eina_matrix3_identity(&m);

   vb_w = pd->viewbox.w;
   vb_h = pd->viewbox.h;
   vp_w = sz.w;
   vp_h = sz.h;

   scale_w = vp_w / vb_w;
   scale_h = vp_h / vb_h;

   if (pd->fill_mode == EFL_CANVAS_VG_FILL_MODE_STRETCH)
     { // Fill the viewport and ignore the aspect ratio
        eina_matrix3_scale(&m, scale_w, scale_h);
        eina_matrix3_translate(&m, -pd->viewbox.x, -pd->viewbox.y);
     }
   else
     {
        if (pd->fill_mode == EFL_CANVAS_VG_FILL_MODE_MEET)
          scale = scale_w < scale_h ? scale_w : scale_h;
        else // slice
          scale = scale_w > scale_h ? scale_w : scale_h;
        eina_matrix3_translate(&m, (vp_w - vb_w * scale) * pd->align_x, (vp_h - vb_h * scale) * pd->align_y);
        eina_matrix3_scale(&m, scale, scale);
        eina_matrix3_translate(&m, -pd->viewbox.x, -pd->viewbox.y);
     }

   efl_canvas_vg_node_transformation_set(pd->root, &m);

   pd->changed = EINA_TRUE;
   evas_object_change(obj, efl_data_scope_get(obj, EFL_CANVAS_OBJECT_CLASS));
}

/**
 * @brief Callback function invoked when the Evas VG object is resized.
 *
 * This function triggers an update of the vector graphics tree viewport if the
 * viewbox is valid.
 *
 * @param data The private data of the Efl_Canvas_Vg_Object (passed as Efl_Canvas_Vg_Object_Data *).
 * @param ev The Efl_Event structure containing event information.
 */
static void
_evas_vg_resize(void *data, const Efl_Event *ev)
{
   Efl_Canvas_Vg_Object_Data *pd = data;

   if (eina_rectangle_is_empty(&pd->viewbox.rect))
     return;
   _update_vgtree_viewport(ev->object, pd);
}

/**
 * @brief Gets the root node of the vector graphics tree.
 *
 * If the object is associated with a VG cache entry (e.g., loaded from a file),
 * it retrieves the root node from the cache, potentially resizing the cache entry
 * if the object's dimensions have changed. Otherwise, it returns the user-set
 * root node or the default internal root node.
 *
 * @param obj The Evas object (const).
 * @param pd The private data of the Efl_Canvas_Vg_Object.
 * @return The root Efl_VG node.
 */
EOLIAN static Efl_VG *
_efl_canvas_vg_object_root_node_get(const Eo *obj, Efl_Canvas_Vg_Object_Data *pd)
{
   Efl_VG *root;

   if (pd->vg_entry)
     {
        Evas_Coord w, h;
        evas_object_geometry_get(obj, NULL, NULL, &w, &h);

        //Update vg data with current size.
        if ((pd->vg_entry->w != w) || (pd->vg_entry->h != h))
          {
             Vg_Cache_Entry *vg_entry = evas_cache_vg_entry_resize(pd->vg_entry, w, h);
             evas_cache_vg_entry_del(pd->vg_entry);
             pd->vg_entry = vg_entry;
          }
        root = evas_cache_vg_tree_get(pd->vg_entry, pd->frame_idx);
     }
   else if (pd->user_entry) root = pd->user_entry->root;
   else root = pd->root;

   return root;
}

/**
 * @brief Sets the root node of the vector graphics tree.
 *
 * This function allows replacing the entire vector graphics tree displayed by the object.
 * If a file was previously set, its cache entry is cleared. The old root node is
 * detached and freed. If a new root_node is provided, it's associated with the object.
 *
 * @param eo_obj The Evas object.
 * @param pd The private data of the Efl_Canvas_Vg_Object.
 * @param root_node The new root Efl_VG node to set. Can be NULL to clear the current tree.
 */
EOLIAN static void
_efl_canvas_vg_object_root_node_set(Eo *eo_obj, Efl_Canvas_Vg_Object_Data *pd, Efl_VG *root_node)
{
   // if the same root is already set
   if (pd->user_entry && pd->user_entry->root == root_node)
     return;

   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);

   // check if a file has been already set
   if (pd->vg_entry)
     {
        evas_cache_vg_entry_del(pd->vg_entry);
        pd->vg_entry = NULL;
     }

   // detach/free the old root_node
   if (pd->user_entry && pd->user_entry->root)
     {
        // drop any surface cache attached to it.
        ENFN->ector_surface_cache_drop(_evas_engine_context(obj->layer->evas), pd->user_entry->root);
        efl_canvas_vg_node_vg_obj_set(pd->user_entry->root, NULL, NULL);
        efl_replace(&pd->user_entry->root, NULL);
     }

   if (root_node)
     {
        if (!pd->user_entry)
          {
             pd->user_entry = calloc(1, sizeof(Vg_User_Entry));
             if (!pd->user_entry)
               {
                  ERR("Failed to alloc user entry data while setting root node");
                  return;
               }
          }
        else pd->user_entry->w = pd->user_entry->h = 0;

        efl_replace(&pd->user_entry->root, root_node);
        efl_canvas_vg_node_vg_obj_set(root_node, eo_obj, pd);
     }
   else if (pd->user_entry)
     {
        free(pd->user_entry);
        pd->user_entry = NULL;
     }

   // force a redraw
   pd->changed = EINA_TRUE;
   evas_object_change(eo_obj, obj);
}

/**
 * @brief Sets the fill mode for the vector graphics object.
 *
 * The fill mode determines how the vector graphic (defined by its viewbox)
 * is scaled and positioned within the object's actual area.
 *
 * @param obj The Evas object (unused).
 * @param pd The private data of the Efl_Canvas_Vg_Object.
 * @param fill_mode The desired fill mode (e.g., EFL_CANVAS_VG_FILL_MODE_STRETCH,
 *                  EFL_CANVAS_VG_FILL_MODE_MEET, EFL_CANVAS_VG_FILL_MODE_SLICE).
 */
EOLIAN static void
_efl_canvas_vg_object_fill_mode_set(Eo *obj EINA_UNUSED, Efl_Canvas_Vg_Object_Data *pd, Efl_Canvas_Vg_Fill_Mode fill_mode)
{
   pd->fill_mode = fill_mode;
}

/**
 * @brief Gets the current fill mode of the vector graphics object.
 * @param obj The Evas object (unused).
 * @param pd The private data of the Efl_Canvas_Vg_Object.
 * @return The current Efl_Canvas_Vg_Fill_Mode.
 */
EOLIAN static Efl_Canvas_Vg_Fill_Mode
_efl_canvas_vg_object_fill_mode_get(const Eo *obj EINA_UNUSED, Efl_Canvas_Vg_Object_Data *pd)
{
   return pd->fill_mode;
}

/**
 * @brief Sets the viewbox for the vector graphics object.
 *
 * The viewbox defines the coordinate system and aspect ratio of the source
 * vector graphic. If an empty rectangle is provided, the viewbox is reset.
 * Setting a valid viewbox registers a resize callback to update the rendering
 * when the object's size changes.
 *
 * @param obj The Evas object.
 * @param pd The private data of the Efl_Canvas_Vg_Object.
 * @param viewbox The Eina_Rect defining the viewbox (x, y, w, h).
 *                Example: Eina_Rect r = {0, 0, 100, 100};
 */
EOLIAN static void
_efl_canvas_vg_object_viewbox_set(Eo *obj, Efl_Canvas_Vg_Object_Data *pd, Eina_Rect viewbox)
{
   // viewbox should be a valid rectangle
   if (eina_rectangle_is_empty(&viewbox.rect))
     {
        // reset the old viewbox if any
        if (!eina_rectangle_is_empty(&pd->viewbox.rect))
          {
             Eina_Matrix3 m;

             pd->viewbox = EINA_RECT_EMPTY();
             eina_matrix3_identity(&m);
             efl_canvas_vg_node_transformation_set(pd->root, &m);
             // unregister the resize callback
             efl_event_callback_del(obj, EFL_GFX_ENTITY_EVENT_SIZE_CHANGED, _evas_vg_resize, pd);
          }
        return;
     }
   // register for resize callback if not done yet
   if (eina_rectangle_is_empty(&pd->viewbox.rect))
     efl_event_callback_add(obj, EFL_GFX_ENTITY_EVENT_SIZE_CHANGED, _evas_vg_resize, pd);

   pd->viewbox = viewbox;
   _update_vgtree_viewport(obj, pd);
}

/**
 * @brief Gets the current viewbox of the vector graphics object.
 * @param obj The Evas object (unused).
 * @param pd The private data of the Efl_Canvas_Vg_Object.
 * @return The current Eina_Rect viewbox.
 */
EOLIAN static Eina_Rect
_efl_canvas_vg_object_viewbox_get(const Eo *obj EINA_UNUSED, Efl_Canvas_Vg_Object_Data *pd)
{
   return pd->viewbox;
}

/**
 * @brief Sets the alignment for the viewbox within the object's area.
 *
 * This is used when the fill mode is EFL_CANVAS_VG_FILL_MODE_MEET or
 * EFL_CANVAS_VG_FILL_MODE_SLICE, where the aspect ratio is preserved,
 * potentially leaving empty space. The alignment values (0.0 to 1.0)
 * determine how the scaled graphic is positioned in that space.
 * (0,0) is top-left, (0.5,0.5) is center, (1,1) is bottom-right.
 *
 * @param obj The Evas object (unused).
 * @param pd The private data of the Efl_Canvas_Vg_Object.
 * @param align_x The horizontal alignment (0.0 to 1.0).
 * @param align_y The vertical alignment (0.0 to 1.0).
 */
EOLIAN static void
_efl_canvas_vg_object_viewbox_align_set(Eo *obj EINA_UNUSED, Efl_Canvas_Vg_Object_Data *pd, double align_x, double align_y)
{
   align_x = align_x < 0 ? 0 : align_x;
   align_x = align_x > 1 ? 1 : align_x;

   align_y = align_y < 0 ? 0 : align_y;
   align_y = align_y > 1 ? 1 : align_y;

   pd->align_x = align_x;
   pd->align_y = align_y;
}

/**
 * @brief Gets the current viewbox alignment.
 * @param obj The Evas object (unused).
 * @param pd The private data of the Efl_Canvas_Vg_Object.
 * @param align_x Pointer to store the horizontal alignment (can be NULL).
 * @param align_y Pointer to store the vertical alignment (can be NULL).
 */
EOLIAN static void
_efl_canvas_vg_object_viewbox_align_get(const Eo *obj EINA_UNUSED, Efl_Canvas_Vg_Object_Data *pd, double *align_x, double *align_y)
{
   if (align_x) *align_x = pd->align_x;
   if (align_y) *align_y = pd->align_y;
}

/**
 * @brief Sets the file from which to load the vector graphics.
 *
 * If a file is already loaded and the new file path is different, the existing
 * VG cache entry is deleted. This function then calls the superclass's file_set
 * method. The actual loading happens in _efl_canvas_vg_object_efl_file_load.
 *
 * @param eo_obj The Evas object.
 * @param pd The private data of the Efl_Canvas_Vg_Object (unused in this EOLIAN direct call, but used by logic).
 * @param file The path to the vector graphics file (e.g., an SVG or Lottie/JSON file).
 * @return EINA_ERROR_NONE on success, or an error code otherwise.
 */
EOLIAN static Eina_Error
_efl_canvas_vg_object_efl_file_file_set(Eo *eo_obj, Efl_Canvas_Vg_Object_Data *pd EINA_UNUSED, const char *file)
{
   /* Careful: delete previous vg entry.
      When a new efl file is set, ex-file will be invalid.
      Since vg cache hashes all file entries,
      we must remove it from vg cache before we lost file handle. */
   if (efl_file_loaded_get(eo_obj))
     {
        const char *pname = efl_file_get(eo_obj);
        int pl = pname ? strlen(pname) : 0;
        int cl = file ? strlen(file) : 0;

        if ((pl != cl) || (pname && file && strcmp(pname, file)))
          {
             Evas_Object_Protected_Data *obj;
             obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
             evas_cache_vg_entry_del(pd->vg_entry);
             evas_object_change(eo_obj, obj);
             pd->vg_entry = NULL;
             evas_object_change(eo_obj, obj);
             pd->changed = EINA_TRUE;
          }
     }

   Eina_Error err;
   err = efl_file_set(efl_super(eo_obj, MY_CLASS), file);

   if (err) return err;

   return 0;
}

/**
 * @brief Loads the vector graphics data from the previously set file.
 *
 * This function is called after efl_file_set. It loads the file content via
 * the superclass, then creates a VG cache entry for the loaded data.
 * The object's viewbox is updated based on the viewbox information from the
 * loaded file, if available.
 *
 * @param eo_obj The Evas object.
 * @param pd The private data of the Efl_Canvas_Vg_Object.
 * @return EINA_ERROR_NONE on success, or an error code if loading fails.
 */
EOLIAN static Eina_Error
_efl_canvas_vg_object_efl_file_load(Eo *eo_obj, Efl_Canvas_Vg_Object_Data *pd)
{
   Eina_Error err;
   if (efl_file_loaded_get(eo_obj)) return 0;

   err = efl_file_load(efl_super(eo_obj, MY_CLASS));
   if (err) return err;

   const Eina_File *file = efl_file_mmap_get(eo_obj);
   const char *key = efl_file_key_get(eo_obj);
   Evas_Object_Protected_Data *obj;

   obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   pd->vg_entry = evas_cache_vg_entry_create(evas_object_evas_get(eo_obj),
                                             file, key,
                                             obj->cur->geometry.w,
                                             obj->cur->geometry.h, NULL);

   // NOTE: Update object's viewbox. In this case, there is no need to update
   //       the root of tree. That's why We don't use viewbox_set.
   if (pd->vg_entry && pd->vg_entry->vfd)
     pd->viewbox.rect = pd->vg_entry->vfd->view_box;

   evas_object_change(eo_obj, obj);
   pd->changed = EINA_TRUE;

   return 0;
}

/**
 * @brief Unloads the vector graphics data.
 *
 * This function is called when the object's file is unloaded. It removes the
 * associated VG cache entry.
 *
 * @param eo_obj The Evas object.
 * @param pd The private data of the Efl_Canvas_Vg_Object.
 */
EOLIAN static void
_efl_canvas_vg_object_efl_file_unload(Eo *eo_obj, Efl_Canvas_Vg_Object_Data *pd)
{
   if (!efl_file_loaded_get(eo_obj)) return;

   Evas_Object_Protected_Data *obj;
   obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   evas_cache_vg_entry_del(pd->vg_entry);
   evas_object_change(eo_obj, obj);
   pd->vg_entry = NULL;
}

/**
 * @brief Saves the current vector graphics data to a file.
 *
 * If the object is associated with a VG cache entry (loaded from a file),
 * it attempts to save that entry. Otherwise, it saves the current root node
 * (user-set or default) to the specified file.
 *
 * @param obj The Evas object (const).
 * @param pd The private data of the Efl_Canvas_Vg_Object.
 * @param file The path to the file where the VG data should be saved.
 * @param key Optional key for saving (e.g., for specific formats or parts).
 * @param info Additional save information (e.g., quality, compression).
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
EOLIAN static Eina_Bool
_efl_canvas_vg_object_efl_file_save_save(const Eo *obj, Efl_Canvas_Vg_Object_Data *pd, const char *file, const char *key, const Efl_File_Save_Info *info)
{
   if (pd->vg_entry)
     return evas_cache_vg_entry_file_save(pd->vg_entry, file, key, info);

   Evas_Coord w, h;
   evas_object_geometry_get(obj, NULL, NULL, &w, &h);
   return evas_cache_vg_file_save(pd->root, w, h, file, key, info);
}

/**
 * @brief Cleans up references to renderers after a render cycle.
 *
 * This function is called as a callback after the scene rendering is complete
 * (EFL_CANVAS_SCENE_EVENT_RENDER_POST). It iterates through an array of
 * renderers that were used during asynchronous rendering and unreferences them.
 * This is crucial for managing the lifecycle of renderer objects that might be
 * destroyed asynchronously.
 *
 * The `pd->cleanup` array stores `Eo *renderer` pointers.
 * Example structure of `pd->cleanup` elements:
 * `pd->cleanup[0] = (Eo *)renderer_instance_1;`
 * `pd->cleanup[1] = (Eo *)renderer_instance_2;`
 *
 * @param data The private data of the Efl_Canvas_Vg_Object (Efl_Canvas_Vg_Object_Data *).
 * @param event The Efl_Event structure (unused).
 */
static void
_cleanup_reference(void *data, const Efl_Event *event EINA_UNUSED)
{
   Efl_Canvas_Vg_Object_Data *pd = data;
   Eo *renderer;

   /* unref all renderer and may also destroy them async */
   while ((renderer = eina_array_pop(&pd->cleanup)))
     efl_unref(renderer);
}

/**
 * @brief Invalidates the Efl_Canvas_Vg_Object.
 *
 * This function is part of the Efl_Object lifecycle. It cleans up resources
 * associated with the vector graphics object, including:
 * - Removing the render post cleanup callback.
 * - Flushing the array of renderers pending cleanup.
 * - Unreferencing the root VG node.
 * - Freeing user-specific VG entry data and associated caches.
 * - Dropping VG cache entries and associated surface caches.
 * Finally, it calls the superclass's invalidate method.
 *
 * @param eo_obj The Evas object being invalidated.
 * @param pd The private data of the Efl_Canvas_Vg_Object.
 */
EOLIAN static void
_efl_canvas_vg_object_efl_object_invalidate(Eo *eo_obj, Efl_Canvas_Vg_Object_Data *pd)
{
   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   Evas *e = evas_object_evas_get(eo_obj);

   efl_event_callback_del(e, EFL_CANVAS_SCENE_EVENT_RENDER_POST, _cleanup_reference, pd);
   eina_array_flush(&pd->cleanup);

   efl_unref(pd->root);
   pd->root = NULL;

   if (pd->user_entry)
     {
        Vg_User_Entry *user_entry = pd->user_entry;
        ENFN->ector_surface_cache_drop(ENC, user_entry->root);
        if (pd->user_entry->root) efl_unref(pd->user_entry->root);
        free(pd->user_entry);
     }
   pd->user_entry = NULL;

   //Drop cache buffers
   if (pd->vg_entry)
     {
        if (pd->ckeys[0])
          ENFN->ector_surface_cache_drop(_evas_engine_context(obj->layer->evas), pd->ckeys[0]);
        if (pd->ckeys[1])
          ENFN->ector_surface_cache_drop(_evas_engine_context(obj->layer->evas), pd->ckeys[1]);
     }
   evas_cache_vg_entry_del(pd->vg_entry);

   efl_invalidate(efl_super(eo_obj, MY_CLASS));
}

/**
 * @brief Constructor for the Efl_Canvas_Vg_Object.
 *
 * This function is part of the Efl_Object lifecycle. It initializes the
 * Evas_Object_Protected_Data with VG-specific functions and type information.
 * It also creates a default root VG container node and initializes the cleanup array
 * for asynchronous renderers.
 *
 * @param eo_obj The Evas object being constructed.
 * @param pd The private data of the Efl_Canvas_Vg_Object.
 * @return The constructed Evas object.
 */
EOLIAN static Eo *
_efl_canvas_vg_object_efl_object_constructor(Eo *eo_obj, Efl_Canvas_Vg_Object_Data *pd)
{
   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);

   eo_obj = efl_constructor(efl_super(eo_obj, MY_CLASS));

   /* set up methods (compulsory) */
   obj->func = &object_func;
   obj->private_data = efl_data_ref(eo_obj, MY_CLASS);
   obj->type = o_type;
   obj->is_vg_object = EINA_TRUE;

   /* default root node */
   pd->obj = obj;
   pd->root = efl_add_ref(EFL_CANVAS_VG_CONTAINER_CLASS, NULL);

   pd->sync_render = EINA_FALSE;

   eina_array_step_set(&pd->cleanup, sizeof(pd->cleanup), 8);

   return eo_obj;
}

/**
 * @brief Finalizer for the Efl_Canvas_Vg_Object.
 *
 * This function is part of the Efl_Object lifecycle, called after construction
 * and all parts are set up. It sets the parent of the internal root VG node
 * to the object itself. It also registers a callback for cleaning up renderer
 * references after each render cycle.
 *
 * @param obj The Evas object being finalized.
 * @param pd The private data of the Efl_Canvas_Vg_Object.
 * @return The finalized Evas object.
 */
static Efl_Object *
_efl_canvas_vg_object_efl_object_finalize(Eo *obj, Efl_Canvas_Vg_Object_Data *pd)
{
   Evas *e = evas_object_evas_get(obj);

   /* Container must have a set parent after construction.
      efl_add_ref() with a parent won't work this case
      because container needs some jobs in overriding parent_set()
      after proper intialization. */
   efl_parent_set(pd->root, obj);

   // TODO: If we start to have to many Evas_Object_VG per canvas, it may be nice
   // to actually have one event per canvas and one array per canvas to.
   efl_event_callback_add(e, EFL_CANVAS_SCENE_EVENT_RENDER_POST, _cleanup_reference, pd);

   return obj;
}

/**
 * @brief Recursively renders a vector graphics node and its children.
 *
 * This function traverses the VG node tree. If the node is a container:
 * - It handles alpha blending by rendering children to an intermediate buffer if the container has alpha < 255.
 * - Otherwise, it recursively calls itself for each child.
 * If the node is a primitive (not a container):
 * - It uses the engine's ector_renderer_draw function to render the node.
 * - If rendering asynchronously, it adds the renderer to a cleanup list.
 *
 * @param obj The protected data of the Evas object.
 * @param pd The private data of the Efl_Canvas_Vg_Object.
 * @param engine The rendering engine.
 * @param output The rendering output target.
 * @param context The drawing context.
 * @param node The Efl_VG node to render.
 * @param clips An array of clipping rectangles (currently unused, passed as NULL).
 * @param w The width of the rendering area.
 * @param h The height of the rendering area.
 * @param ector The Ector_Surface to render onto.
 * @param do_async Whether to perform rendering asynchronously.
 */
static void
_evas_vg_render(Evas_Object_Protected_Data *obj, Efl_Canvas_Vg_Object_Data *pd,
                void *engine, void *output, void *context, Efl_VG *node,
                Eina_Array *clips, int w, int h, Ector_Surface *ector, Eina_Bool do_async)
{
   if (!efl_gfx_entity_visible_get(node)) return;

   if (efl_isa(node, EFL_CANVAS_VG_CONTAINER_CLASS))
     {
        Efl_VG *child;
        Eina_List *l;
        Efl_Canvas_Vg_Container_Data *cd = efl_data_scope_get(node, EFL_CANVAS_VG_CONTAINER_CLASS);

        if (cd->comp.src) return;   //Don't draw composite target itself.

        int alpha = 255;
        efl_gfx_color_get(node, NULL, NULL, NULL, &alpha);

        if (alpha < 255)
          {
             //Replace with a new size.
             if (cd->blend.buffer)
               {
                  int w2, h2;
                  ector_buffer_size_get(cd->blend.buffer, &w2, &h2);
                  if (w2 != w || h2 != h)
                    efl_canvas_vg_container_blend_buffer_clear(node, cd);
               }

             if (!cd->blend.buffer)
               {
                  cd->blend.buffer = ENFN->ector_buffer_new(ENC, obj->layer->evas->evas,
                                                            w, h,
                                                            EFL_GFX_COLORSPACE_ARGB8888,
                                                            ECTOR_BUFFER_FLAG_DRAWABLE |
                                                            ECTOR_BUFFER_FLAG_CPU_READABLE |
                                                            ECTOR_BUFFER_FLAG_CPU_WRITABLE);
                  cd->blend.pixels = ector_buffer_map(cd->blend.buffer, &cd->blend.length,
                                                      (ECTOR_BUFFER_FLAG_DRAWABLE |
                                                       ECTOR_BUFFER_FLAG_CPU_READABLE |
                                                       ECTOR_BUFFER_FLAG_CPU_WRITABLE),
                                                      0, 0, w, h,
                                                      EFL_GFX_COLORSPACE_ARGB8888,
                                                      &cd->blend.stride);
                  if (!cd->blend.pixels) ERR("Failed to map VG blend buffer");
               }
             else
               {
                  if (cd->blend.pixels)
                    memset(cd->blend.pixels, 0, cd->blend.length);
               }

             //For recovery context
             //FIXME: It may occur async issue?
             int px, py, pw, ph, pstride;
             void *ppixels = NULL;
             ector_buffer_size_get(ector, &pw, &ph);
             ector_buffer_pixels_get(ector, &ppixels, &pw, &ph, &pstride);
             Efl_Gfx_Colorspace pcspace = ector_buffer_cspace_get(ector);
             ector_surface_reference_point_get(ector, &px, &py);

             // Buffer change
             ector_buffer_pixels_set(ector, cd->blend.pixels,
                                     w, h, cd->blend.stride,
                                     EFL_GFX_COLORSPACE_ARGB8888, EINA_TRUE);
             ector_surface_reference_point_set(ector, 0,0);

             // Draw child node to changed buffer
             EINA_LIST_FOREACH(cd->children, l, child)
                _evas_vg_render(obj, pd, engine, output, context, child, clips, w, h, ector, do_async);

             // Recover original surface
             ector_buffer_pixels_set(ector, ppixels, pw, ph, pstride, pcspace, EINA_TRUE);
             ector_surface_reference_point_set(ector, px, py);

             // Draw buffer to original surface.(Ector_Surface)
             ector_surface_draw_image(ector, cd->blend.buffer, 0, 0, alpha);

          }
        else
          {
             efl_canvas_vg_container_blend_buffer_clear(node, cd);

             EINA_LIST_FOREACH(cd->children, l, child)
                _evas_vg_render(obj, pd, engine, output, context, child, clips, w, h, ector, do_async);
          }
     }
   else
     {
        Efl_Canvas_Vg_Node_Data *nd = efl_data_scope_get(node, EFL_CANVAS_VG_NODE_CLASS);
        ENFN->ector_renderer_draw(engine, output, context, nd->renderer, clips, do_async);
        if (do_async) eina_array_push(&pd->cleanup, efl_ref(nd->renderer));
     }
}

/**
 * @brief Renders a vector graphics tree to an offscreen buffer.
 *
 * This function orchestrates the rendering of a given VG root node to a buffer.
 * If a buffer is not provided, it creates one. It initializes the drawing context,
 * begins an Ector rendering pass, calls _evas_vg_render to draw the content,
 * and then ends the Ector pass. If a cache key (ckey) is provided, the resulting
 * buffer is stored in the Ector surface cache.
 *
 * @param obj The protected data of the Evas object.
 * @param pd The private data of the Efl_Canvas_Vg_Object.
 * @param engine The rendering engine.
 * @param root The root Efl_VG node of the tree to render.
 * @param x The x-offset within the buffer (often 0 for full buffer rendering).
 * @param y The y-offset within the buffer (often 0 for full buffer rendering).
 * @param w The width of the buffer.
 * @param h The height of the buffer.
 * @param buffer An existing buffer to render to (optional, can be NULL). If NULL, a new one is created.
 * @param ckey A cache key to associate with the rendered buffer (optional).
 * @param do_async Whether to perform rendering asynchronously.
 * @return A pointer to the buffer containing the rendered image, or NULL on failure.
 *         The caller may need to manage the lifecycle of this buffer if `ckey` is NULL
 *         or if it was passed in.
 */
static void *
_render_to_buffer(Evas_Object_Protected_Data *obj, Efl_Canvas_Vg_Object_Data *pd,
                  void *engine, Efl_VG *root, int x, int y, int w, int h, void *buffer, void *ckey,
                  Eina_Bool do_async)
{
   Ector_Surface *ector;
   RGBA_Draw_Context *context;
   int error = 0;
   Eina_Bool buffer_created = EINA_FALSE;

   ector = evas_ector_get(obj->layer->evas);
   if (!ector) return NULL;

   //create a buffer
   if (!buffer)
     {
        buffer = ENFN->ector_surface_create(engine, w, h, &error);
        if (error) return NULL;
        buffer_created = EINA_TRUE;
     }

   //initialize buffer
   context = evas_common_draw_context_new();
   evas_common_draw_context_set_render_op(context, _EVAS_RENDER_COPY);
   evas_common_draw_context_set_color(context, 255, 255, 255, 255);

   //ector begin - end for drawing composite images.
   _evas_vg_render_pre(obj, root, engine, buffer, context, ector, NULL, 255, NULL, 0);

   if (pd->sync_render) do_async = EINA_FALSE;

   //Actual content drawing
   if (!ENFN->ector_begin(engine, buffer, context, ector, x, y, do_async))
     {
        ERR("Failed ector begin!");
        return NULL;
     }

   //draw on buffer
   _evas_vg_render(obj, pd,
                   engine, buffer,
                   context, root,
                   NULL,
                   w, h, ector,
                   do_async);

   ENFN->image_dirty_region(engine, buffer, 0, 0, w, h);
   ENFN->ector_end(engine, buffer, context, ector, do_async);
   evas_common_draw_context_free(context);

   if (buffer_created && ckey)
     {
        //Drop ex invalid cache buffers.
        if (pd->frame_idx == 0 && ckey != pd->ckeys[0])
          {
             if (pd->ckeys[0])
               ENFN->ector_surface_cache_drop(engine, pd->ckeys[0]);
             pd->ckeys[0] = ckey;
          }
        else if (pd->frame_idx == (int) (evas_cache_vg_anim_frame_count_get(pd->vg_entry) - 1)
                 && ckey != pd->ckeys[1])
          {
             if (pd->ckeys[1])
               ENFN->ector_surface_cache_drop(engine, pd->ckeys[1]);
             pd->ckeys[1] = ckey;
          }
        ENFN->ector_surface_cache_set(engine, ckey, buffer);
     }

   return buffer;
}

/**
 * @brief Renders a pre-rendered buffer (image) to the screen/surface.
 *
 * This function takes a buffer (typically an offscreen surface containing a rendered
 * vector graphic) and draws it onto the final rendering surface (screen).
 * It handles asynchronous unreferencing of the buffer if `do_async` is true
 * and the engine indicates the buffer can be unreferenced asynchronously.
 * If the buffer is not cacheable, it's destroyed after drawing.
 *
 * @param obj The protected data of the Evas object.
 * @param engine The rendering engine.
 * @param output The rendering output target.
 * @param context The drawing context.
 * @param surface The target surface for drawing (e.g., the screen).
 * @param buffer The source buffer (Image_Entry *) containing the image to draw.
 * @param x The x-coordinate on the target surface.
 * @param y The y-coordinate on the target surface.
 * @param w The width of the image to draw from the buffer.
 * @param h The height of the image to draw from the buffer.
 * @param do_async Whether to perform drawing asynchronously.
 * @param cacheable If EINA_FALSE, the buffer is destroyed after drawing.
 */
static void
_render_buffer_to_screen(Evas_Object_Protected_Data *obj,
                         void *engine, void *output, void *context, void *surface,
                         void *buffer,
                         int x, int y, int w, int h,
                         Eina_Bool do_async, Eina_Bool cacheable)
{
   if (!buffer) return;

   Eina_Bool async_unref;

   //Draw the buffer as image to canvas
   async_unref = ENFN->image_draw(engine, output, context, surface,
                                                           buffer,
                                                           0, 0, w, h,
                                                           x, y, w, h,
                                                           EINA_TRUE, do_async);
   if (do_async && async_unref)
     {
        //Free buffer after drawing.
        evas_cache_image_ref((Image_Entry *)buffer);
        evas_unref_queue_image_put(obj->layer->evas, buffer);
     }

   //TODO: Reuse buffer if size is same?
   if (!cacheable) ENFN->ector_surface_destroy(engine, buffer);
}

/**
 * @brief Renders a vector graphics object that is managed by a Vg_Cache_Entry.
 *
 * This function handles rendering for VG objects loaded from files (which use
 * Vg_Cache_Entry). It performs several steps:
 * 1. Updates value providers for the VG entry.
 * 2. Checks if the object's size has changed; if so, resizes the cache entry
 *    and adjusts dimensions to maintain aspect ratio if a default size is known.
 * 3. Retrieves the appropriate VG tree for the current animation frame.
 * 4. If caching is enabled, tries to get a pre-rendered buffer from the Ector surface cache.
 * 5. If not found in cache (or caching disabled), renders the VG tree to a new buffer
 *    using _render_to_buffer().
 * 6. Renders the buffer (from cache or newly rendered) to the screen using
 *    _render_buffer_to_screen().
 *
 * @param obj The protected data of the Evas object.
 * @param pd The private data of the Efl_Canvas_Vg_Object.
 * @param engine The rendering engine.
 * @param output The rendering output target.
 * @param context The drawing context.
 * @param surface The target surface for drawing.
 * @param x The x-coordinate on the target surface.
 * @param y The y-coordinate on the target surface.
 * @param w The width of the area to render into.
 * @param h The height of the area to render into.
 * @param do_async Whether to perform rendering asynchronously.
 * @param cacheable Whether the rendered buffer can be cached.
 */
static void
_cache_vg_entry_render(Evas_Object_Protected_Data *obj,
                       Efl_Canvas_Vg_Object_Data *pd,
                       void *engine, void *output, void *context, void *surface,
                       int x, int y, int w, int h, Eina_Bool do_async,
                       Eina_Bool cacheable)
{
   Vg_Cache_Entry *vg_entry = pd->vg_entry;
   Efl_VG *root;
   Eina_Position2D offset = {0, 0};  //Offset after keeping aspect ratio.
   void *buffer = NULL;
   void *key = NULL;

   evas_cache_vg_entry_value_provider_update(pd->vg_entry, efl_key_data_get(obj->object, "_vg_value_providers"));

   // if the size changed in between path set and the draw call;
   if ((vg_entry->w != w) ||
       (vg_entry->h != h))
     {
        Eina_Size2D size = evas_cache_vg_entry_default_size_get(pd->vg_entry);

        //adjust size for aspect ratio.
        if (size.w > 0 && size.h > 0)
          {
             float rw = (float) w / (float) size.w;
             float rh = (float) h / (float) size.h;

             if (rw < rh)
               {
                  size.w = w;
                  size.h = (int) ((float) size.h * rw);
               }
             else
               {
                  size.w = (int) ((float) size.w * rh);
                  size.h = h;
               }
          }
        else
          {
              size.w = w;
              size.h = h;
          }

        //Size is changed, cached data is invalid.
        if ((size.w != vg_entry->w) || (size.h != vg_entry->h))
          {
//Not necessary, but this might be helpful for precise caching.
#if 0
             if (cacheable)
               {
                  //if the size doesn't match, drop previous cache surface.
                  key = evas_cache_vg_surface_key_get(pd->vg_entry->root, vg_entry->w, vg_entry->h, 0);
                  if (key) ENFN->ector_surface_cache_drop(engine, key);

                  //Animatable... Try to drop the last frame image.
                  int last_frame = (int) (evas_cache_vg_anim_frame_count_get(pd->vg_entry) - 1);
                  if (last_frame > 0)
                    {
                       key = evas_cache_vg_surface_key_get(pd->vg_entry->root, vg_entry->w, vg_entry->h, last_frame);
                       if (key) ENFN->ector_surface_cache_drop(engine, key);
                    }
               }
#endif
             vg_entry = evas_cache_vg_entry_resize(vg_entry, size.w, size.h);
             evas_cache_vg_entry_del(pd->vg_entry);
             pd->vg_entry = vg_entry;
          }

        //update for adjusted pos and size.
        offset.x = w - size.w;
        if (offset.x > 0) offset.x /= 2;
        offset.y = h - size.h;
        if (offset.y > 0) offset.y /= 2;
        w = size.w;
        h = size.h;
     }
   root = evas_cache_vg_tree_get(vg_entry, pd->frame_idx);
   if (!root) return;

   if (cacheable)
     {
        key = evas_cache_vg_surface_key_get(root, w, h, pd->frame_idx);
        if (key) buffer = ENFN->ector_surface_cache_get(engine, key);
     }

   if (!buffer)
     {
        buffer = _render_to_buffer(obj, pd, engine, root, 0, 0, w, h, NULL, key, do_async);
     }
   else
     {
        //cache reference was increased when we get the cache.
        if (key) ENFN->ector_surface_cache_drop(engine, key);
     }

   _render_buffer_to_screen(obj,
                            engine, output, context, surface,
                            buffer,
                            x + offset.x, y + offset.y, w, h,
                            do_async, cacheable);
}

/**
 * @brief Renders a vector graphics object that is managed by a Vg_User_Entry.
 *
 * This function handles rendering for VG objects whose root node is set directly
 * by the user (using Vg_User_Entry). It performs these main steps:
 * 1. If the VG content has changed, recalculates the path bounds of the root node.
 * 2. Determines the rendering rectangle based on path bounds or object geometry.
 * 3. Adjusts the rendering rectangle based on the object's viewbox, if set.
 * 4. If the render size has changed, drops any existing surface cache for this entry.
 * 5. Tries to retrieve a cached buffer for the user entry's root node.
 * 6. If not cached or if content changed, renders the VG tree to a buffer using
 *    _render_to_buffer(). The user entry's root node itself is used as the cache key.
 * 7. Renders the buffer to the screen using _render_buffer_to_screen().
 *
 * @param obj The protected data of the Evas object.
 * @param pd The private data of the Efl_Canvas_Vg_Object.
 * @param engine The rendering engine.
 * @param output The rendering output target.
 * @param context The drawing context.
 * @param surface The target surface for drawing.
 * @param x The x-coordinate on the target surface.
 * @param y The y-coordinate on the target surface.
 * @param w The width of the area to render into.
 * @param h The height of the area to render into.
 * @param do_async Whether to perform rendering asynchronously.
 */
static void
_user_vg_entry_render(Evas_Object_Protected_Data *obj,
                      Efl_Canvas_Vg_Object_Data *pd,
                      void *engine, void *output, void *context, void *surface,
                      int x, int y, int w, int h, Eina_Bool do_async)
{
   Vg_User_Entry *user_entry = pd->user_entry;
   Eina_Rect render_rect = EINA_RECT(x, y, w, h);

   // Get changed boundary and fit the size.
   if (pd->changed)
     efl_gfx_path_bounds_get(user_entry->root, &user_entry->path_bounds);

   if (user_entry->path_bounds.w != 0 && user_entry->path_bounds.h != 0)
     {
        EINA_RECTANGLE_SET(&render_rect, user_entry->path_bounds.x,
                           user_entry->path_bounds.y,
                           user_entry->path_bounds.w,
                           user_entry->path_bounds.h);
     }
   // If size of the drawing area is 0, no render.
   else return;

   if (pd->viewbox.w != 0 && pd->viewbox.h !=0)
     {
        double sx = 0, sy= 0;
        sx = (double)w / (double)pd->viewbox.w;
        sy = (double)h / (double)pd->viewbox.h;
        render_rect.x = (render_rect.x - pd->viewbox.x) * sx;
        render_rect.y = (render_rect.y - pd->viewbox.y) * sy;
        render_rect.w *= sx;
        render_rect.h *= sy;
     }

   //if the size doesn't match, drop previous cache surface.
   if ((user_entry->w != render_rect.w ) ||
       (user_entry->h != render_rect.h))
     {
        ENFN->ector_surface_cache_drop(engine, user_entry->root);
        user_entry->w = render_rect.w;
        user_entry->h = render_rect.h;
     }

   //if the buffer is not created yet
   void *buffer = NULL;

   buffer = ENFN->ector_surface_cache_get(engine, user_entry->root);

   if (!buffer)
     {
        // render to the buffer
        buffer = _render_to_buffer(obj, pd, engine, user_entry->root,
                                   render_rect.x, render_rect.y, render_rect.w, render_rect.h, buffer, user_entry->root, do_async);
     }
   else
     {
        // render to the buffer
        if (pd->changed)
          buffer = _render_to_buffer(obj, pd, engine,
                                     user_entry->root,
                                     render_rect.x, render_rect.y, render_rect.w, render_rect.h, buffer, NULL,
                                     do_async);
        //cache reference was increased when we get the cache.
        ENFN->ector_surface_cache_drop(engine, user_entry->root);
     }

   _render_buffer_to_screen(obj,
                            engine, output, context, surface,
                            buffer,
                            x + render_rect.x,
                            y + render_rect.y,
                            render_rect.w, render_rect.h,
                            do_async, EINA_TRUE);
}

static void
_efl_canvas_vg_object_render(Evas_Object *eo_obj EINA_UNUSED,
                             Evas_Object_Protected_Data *obj,
                             void *type_private_data,
                             void *engine, void *output, void *context, void *surface,
                             int x, int y, Eina_Bool do_async)
{
   Efl_Canvas_Vg_Object_Data *pd = type_private_data;

   /* render object to surface with context, and offxet by x,y */
   ENFN->context_color_set(engine, context, 255, 255, 255, 255);
   ENFN->context_multiplier_set(engine, context,
                                obj->cur->cache.clip.r,
                                obj->cur->cache.clip.g,
                                obj->cur->cache.clip.b,
                                obj->cur->cache.clip.a);
   ENFN->context_anti_alias_set(engine, context, obj->cur->anti_alias);
   ENFN->context_render_op_set(engine, context, obj->cur->render_op);

   //Cache surface?
   Eina_Bool cacheable = EINA_FALSE;

   /* Try caching buffer only for first and last frames
      because it's an overhead task if it caches all frame images.
      We assume the first and last frame images are the most resusable
      in generic scenarios. */
   if (pd->frame_idx == 0 ||
       (pd->frame_idx == (int) (evas_cache_vg_anim_frame_count_get(pd->vg_entry) - 1)))
     cacheable = EINA_TRUE;

   if (pd->vg_entry)
     {
        _cache_vg_entry_render(obj, pd,
                               engine, output, context, surface,
                               obj->cur->geometry.x + x, obj->cur->geometry.y + y,
                               obj->cur->geometry.w, obj->cur->geometry.h, do_async, cacheable);
     }
   if (pd->user_entry)
     {
        _user_vg_entry_render(obj, pd,
                              engine, output, context, surface,
                              obj->cur->geometry.x + x, obj->cur->geometry.y + y,
                              obj->cur->geometry.w, obj->cur->geometry.h, do_async);
     }
   pd->changed = EINA_FALSE;
}

/**
 * @brief Performs pre-render operations for the Efl_Canvas_Vg_Object.
 *
 * This is a standard Evas object lifecycle function called before rendering.
 * It checks for various changes (visibility, color, geometry, clipper, etc.)
 * and adds appropriate redraw rectangles to the Evas update system if necessary.
 * If the object's internal `pd->changed` flag is set (indicating a change in
 * the VG data itself), it forces a redraw.
 *
 * @param eo_obj The Evas object.
 * @param obj The protected data of the Evas object.
 * @param type_private_data The private data of the Efl_Canvas_Vg_Object.
 */
static void
_efl_canvas_vg_object_render_pre(Evas_Object *eo_obj,
                                 Evas_Object_Protected_Data *obj,
                                 void *type_private_data)
{
   Efl_Canvas_Vg_Object_Data *pd = type_private_data;
   int is_v, was_v;

   if (obj->pre_render_done) return;
   obj->pre_render_done = EINA_TRUE;

   /* pre-render phase. this does anything an object needs to do just before */
   /* rendering. this could mean loading the image data, retrieving it from */
   /* elsewhere, decoding video etc. */
   /* then when this is done the object needs to figure if it changed and */
   /* if so what and where and add the appropriate redraw rectangles */
   /* if someone is clipping this obj - go calculate the clipper */
   if (obj->cur->clipper)
     {
        if (obj->cur->cache.clip.dirty)
          evas_object_clip_recalc(obj->cur->clipper);
        obj->cur->clipper->func->render_pre(obj->cur->clipper->object,
                                            obj->cur->clipper,
                                            obj->cur->clipper->private_data);
     }

   /* now figure what changed and add draw rects */
   /* if it just became visible or invisible */
   is_v = evas_object_is_visible(obj);
   was_v = evas_object_was_visible(obj);
   if (!(is_v | was_v)) goto done;

   if (pd->changed)
     {
        evas_object_render_pre_prev_cur_add(&obj->layer->evas->clip_changes, eo_obj, obj);
        goto done;
     }

   if (is_v != was_v)
     {
        evas_object_render_pre_visible_change(&obj->layer->evas->clip_changes, eo_obj, is_v, was_v);
        goto done;
     }
   if (obj->changed_map || obj->changed_src_visible)
     {
        evas_object_render_pre_prev_cur_add(&obj->layer->evas->clip_changes, eo_obj, obj);
        goto done;
     }
   /* it's not visible - we accounted for it appearing or not so just abort */
   if (!is_v) goto done;
   /* clipper changed this is in addition to anything else for obj */
   evas_object_render_pre_clipper_change(&obj->layer->evas->clip_changes, eo_obj);
   /* if we restacked (layer or just within a layer) and don't clip anyone */
   if ((obj->restack) && (!obj->clip.clipees))
     {
        evas_object_render_pre_prev_cur_add(&obj->layer->evas->clip_changes, eo_obj, obj);
        goto done;
     }
   /* if it changed render op */
   if (obj->cur->render_op != obj->prev->render_op)
     {
        evas_object_render_pre_prev_cur_add(&obj->layer->evas->clip_changes, eo_obj, obj);
        goto done;
     }
   /* if it changed color */
   if ((obj->cur->color.r != obj->prev->color.r) ||
       (obj->cur->color.g != obj->prev->color.g) ||
       (obj->cur->color.b != obj->prev->color.b) ||
       (obj->cur->color.a != obj->prev->color.a) ||
       (obj->cur->cache.clip.r != obj->prev->cache.clip.r) ||
       (obj->cur->cache.clip.g != obj->prev->cache.clip.g) ||
       (obj->cur->cache.clip.b != obj->prev->cache.clip.b) ||
       (obj->cur->cache.clip.a != obj->prev->cache.clip.a))
     {
        evas_object_render_pre_prev_cur_add(&obj->layer->evas->clip_changes, eo_obj, obj);
        goto done;
     }
   /* if it changed geometry - and obviously not visibility or color */
   /* calculate differences since we have a constant color fill */
   /* we really only need to update the differences */
   if ((obj->cur->geometry.x != obj->prev->geometry.x) ||
       (obj->cur->geometry.y != obj->prev->geometry.y) ||
       (obj->cur->geometry.w != obj->prev->geometry.w) ||
       (obj->cur->geometry.h != obj->prev->geometry.h))
     {
        evas_object_render_pre_prev_cur_add(&obj->layer->evas->clip_changes, eo_obj, obj);
        goto done;
     }
   /* it obviously didn't change - add a NO obscure - this "unupdates"  this */
   /* area so if there were updates for it they get wiped. don't do it if we */
   /* arent fully opaque and we are visible */
   if (evas_object_is_visible(obj) &&
       evas_object_is_opaque(obj) &&
       (!obj->clip.clipees))
     {
        Evas_Coord x, y, w, h;

        x = obj->cur->cache.clip.x;
        y = obj->cur->cache.clip.y;
        w = obj->cur->cache.clip.w;
        h = obj->cur->cache.clip.h;
        if (obj->cur->clipper)
          {
             RECTS_CLIP_TO_RECT(x, y, w, h,
                                obj->cur->clipper->cur->cache.clip.x,
                                obj->cur->clipper->cur->cache.clip.y,
                                obj->cur->clipper->cur->cache.clip.w,
                                obj->cur->clipper->cur->cache.clip.h);
          }
        evas_render_update_del(obj->layer->evas,
                               x + obj->layer->evas->framespace.x,
                               y + obj->layer->evas->framespace.y,
                               w, h);
     }

done:
   evas_object_render_pre_effect_updates(&obj->layer->evas->clip_changes, eo_obj, is_v, was_v);
}

/**
 * @brief Performs post-render operations for the Efl_Canvas_Vg_Object.
 *
 * This is a standard Evas object lifecycle function called after rendering.
 * It cleans up any clip changes recorded during the render cycle and updates
 * the object's state by copying current state to previous state.
 *
 * @param eo_obj The Evas object (unused).
 * @param obj The protected data of the Evas object.
 * @param type_private_data The private data of the Efl_Canvas_Vg_Object (unused).
 */
static void
_efl_canvas_vg_object_render_post(Evas_Object *eo_obj EINA_UNUSED,
                                  Evas_Object_Protected_Data *obj,
                                  void *type_private_data EINA_UNUSED)
{
   /* this moves the current data to the previous state parts of the object */
   /* in whatever way is safest for the object. also if we don't need object */
   /* data anymore we can free it if the object deems this is a good idea */
   /* remove those pesky changes */
   evas_object_clip_changes_clean(obj);
   /* move cur to prev safely for object data */
   evas_object_cur_prev(obj);
}

/**
 * @brief Checks if the VG object is currently opaque.
 * @note Currently, VG objects are always considered non-opaque.
 * @param eo_obj The Evas object (unused).
 * @param obj The protected data of the Evas object (unused).
 * @param type_private_data The private data of the Efl_Canvas_Vg_Object (unused).
 * @return Always 0 (not opaque).
 */
static int
_efl_canvas_vg_object_is_opaque(Evas_Object *eo_obj EINA_UNUSED,
                                Evas_Object_Protected_Data *obj EINA_UNUSED,
                                void *type_private_data EINA_UNUSED)
{
   return 0;
}

/**
 * @brief Checks if the VG object was opaque in its previous state.
 * @note Currently, VG objects are always considered non-opaque.
 * @param eo_obj The Evas object (unused).
 * @param obj The protected data of the Evas object (unused).
 * @param type_private_data The private data of the Efl_Canvas_Vg_Object (unused).
 * @return Always 0 (not opaque).
 */
static int
_efl_canvas_vg_object_was_opaque(Evas_Object *eo_obj EINA_UNUSED,
                                 Evas_Object_Protected_Data *obj EINA_UNUSED,
                                 void *type_private_data EINA_UNUSED)
{
   return 0;
}

/* animated feature */
/**
 * @brief Checks if the vector graphic is animated.
 * @param eo_obj The Evas object (unused).
 * @param pd The private data of the Efl_Canvas_Vg_Object (unused).
 * @return EINA_TRUE if animated (currently hardcoded), EINA_FALSE otherwise.
 * @todo Implement proper check based on VG data.
 */
EOLIAN static Eina_Bool
_efl_canvas_vg_object_efl_gfx_frame_controller_animated_get(const Eo *eo_obj EINA_UNUSED,
                                                                      Efl_Canvas_Vg_Object_Data *pd EINA_UNUSED EINA_UNUSED)
{
   //TODO:
   return EINA_TRUE;
}

/**
 * @brief Gets the total number of frames in the animation.
 * @param eo_obj The Evas object (unused).
 * @param pd The private data of the Efl_Canvas_Vg_Object.
 * @return The frame count if loaded from a file (via vg_entry), 0 otherwise.
 */
EOLIAN static int
_efl_canvas_vg_object_efl_gfx_frame_controller_frame_count_get(const Eo *eo_obj EINA_UNUSED,
                                                                                  Efl_Canvas_Vg_Object_Data *pd EINA_UNUSED)
{
   if (!pd->vg_entry) return 0;
   return evas_cache_vg_anim_frame_count_get(pd->vg_entry);
}

/**
 * @brief Gets the loop type hint for the animation.
 * @param eo_obj The Evas object (unused).
 * @param pd The private data of the Efl_Canvas_Vg_Object (unused).
 * @return The loop hint (currently EFL_GFX_FRAME_CONTROLLER_LOOP_HINT_NONE).
 * @todo Implement proper loop type retrieval.
 */
EOLIAN static Efl_Gfx_Frame_Controller_Loop_Hint
_efl_canvas_vg_object_efl_gfx_frame_controller_loop_type_get(const Eo *eo_obj EINA_UNUSED,
                                                                                Efl_Canvas_Vg_Object_Data *pd EINA_UNUSED)
{
   //TODO:
   return EFL_GFX_FRAME_CONTROLLER_LOOP_HINT_NONE;
}

/**
 * @brief Gets the loop count for the animation.
 * @param eo_obj The Evas object (unused).
 * @param pd The private data of the Efl_Canvas_Vg_Object (unused).
 * @return The loop count (currently 0).
 * @todo Implement proper loop count retrieval.
 */
EOLIAN static int
_efl_canvas_vg_object_efl_gfx_frame_controller_loop_count_get(const Eo *eo_obj EINA_UNUSED,
                                                                                 Efl_Canvas_Vg_Object_Data *pd EINA_UNUSED)
{
   //TODO:
   return 0;
}

/**
 * @brief Gets the duration of a specific frame or sequence of frames.
 * @param eo_obj The Evas object (unused).
 * @param pd The private data of the Efl_Canvas_Vg_Object.
 * @param start_frame The starting frame index (unused in current implementation).
 * @param frame_num The number of frames (unused in current implementation).
 * @return The total animation duration if loaded from a file, 0 otherwise.
 * @todo Implement per-frame duration if supported by the backend.
 */
EOLIAN static double
_efl_canvas_vg_object_efl_gfx_frame_controller_frame_duration_get(const Eo *eo_obj EINA_UNUSED,
                                                                                     Efl_Canvas_Vg_Object_Data *pd,
                                                                                     int start_frame EINA_UNUSED,
                                                                                     int frame_num EINA_UNUSED)
{
   if (!pd->vg_entry) return 0;
   return evas_cache_vg_anim_duration_get(pd->vg_entry);
}

/**
 * @brief Sets a named animation sector (a range of frames).
 * @param obj The Evas object (unused).
 * @param pd The private data of the Efl_Canvas_Vg_Object.
 * @param name The name of the sector.
 * @param startframe The starting frame index of the sector.
 * @param endframe The ending frame index of the sector.
 * @return EINA_TRUE on success, EINA_FALSE if not supported or vg_entry is NULL.
 */
Eina_Bool _efl_canvas_vg_object_efl_gfx_frame_controller_sector_set(Eo *obj EINA_UNUSED,
                                                                    Efl_Canvas_Vg_Object_Data *pd,
                                                                    const char *name,
                                                                    int startframe,
                                                                    int endframe)
{
   if (!pd->vg_entry) return EINA_FALSE;
   if (!evas_cache_vg_anim_sector_set(pd->vg_entry, name, startframe, endframe))
     return EINA_FALSE;
   return EINA_TRUE;
}

/**
 * @brief Gets a named animation sector.
 * @param obj The Evas object (unused).
 * @param pd The private data of the Efl_Canvas_Vg_Object.
 * @param name The name of the sector.
 * @param startframe Pointer to store the starting frame index.
 * @param endframe Pointer to store the ending frame index.
 * @return EINA_TRUE on success, EINA_FALSE if sector not found or vg_entry is NULL.
 */
Eina_Bool _efl_canvas_vg_object_efl_gfx_frame_controller_sector_get(const Eo *obj EINA_UNUSED,
                                                                    Efl_Canvas_Vg_Object_Data *pd,
                                                                    const char *name,
                                                                    int *startframe,
                                                                    int *endframe)
{
   if (!pd->vg_entry) return EINA_FALSE;
   if (!evas_cache_vg_anim_sector_get(pd->vg_entry, name, startframe, endframe))
     return EINA_FALSE;
   return EINA_TRUE;
}

/**
 * @brief Sets the current animation frame index.
 * @param eo_obj The Evas object.
 * @param pd The private data of the Efl_Canvas_Vg_Object.
 * @param frame_index The frame index to set.
 * @return EINA_TRUE on success.
 * @todo Add validation for frame_index range.
 */
EOLIAN static Eina_Bool
_efl_canvas_vg_object_efl_gfx_frame_controller_frame_set(Eo *eo_obj,
                                                         Efl_Canvas_Vg_Object_Data *pd,
                                                         int frame_index)
{
   //TODO: Validate frame_index range
   if (pd->frame_idx == frame_index) return EINA_TRUE;

   //Image is changed, drop previous cached image.
   pd->frame_idx = frame_index;
   pd->changed = EINA_TRUE;
   evas_object_change(eo_obj, efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS));

   return EINA_TRUE;
}

/**
 * @brief Gets the current animation frame index.
 * @param eo_obj The Evas object (unused).
 * @param pd The private data of the Efl_Canvas_Vg_Object.
 * @return The current frame index.
 */
EOLIAN static int
_efl_canvas_vg_object_efl_gfx_frame_controller_frame_get(const Eo *eo_obj EINA_UNUSED,
                                                                            Efl_Canvas_Vg_Object_Data *pd EINA_UNUSED)
{
   return pd->frame_idx;
}

/**
 * @brief Gets the default size of the vector graphic.
 *
 * This typically comes from the metadata of a loaded VG file.
 *
 * @param eo_obj The Evas object (unused).
 * @param pd The private data of the Efl_Canvas_Vg_Object.
 * @return The default Eina_Size2D of the VG content, or {0,0} if not available.
 *         Example: `{ .w = 100, .h = 100 }`
 */
EOLIAN static Eina_Size2D
_efl_canvas_vg_object_default_size_get(const Eo *eo_obj EINA_UNUSED,
                                       Efl_Canvas_Vg_Object_Data *pd EINA_UNUSED)
{
   return evas_cache_vg_entry_default_size_get(pd->vg_entry);
}

/**
 * @brief Adds a new vector graphics object to the Evas canvas.
 * @param e The Evas canvas to add the object to.
 * @return A new Evas_Object (Eo *) on success, NULL on failure.
 * @ingroup Evas_Object_Group
 */
EVAS_API Eo *
evas_object_vg_add(Evas *e)
{
   e = evas_find(e);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(efl_isa(e, EVAS_CANVAS_CLASS), NULL);
   // TODO: Ask backend to return the main Ector_Surface
   return efl_add(MY_CLASS, e, efl_canvas_object_legacy_ctor(efl_added));
}

/**
 * @brief Gets the current frame index of an animated vector graphics object.
 * @param obj The Evas object.
 * @return The current frame index.
 * @see efl_gfx_frame_controller_frame_get()
 * @ingroup Evas_Object_Group_VG_Animation
 */
EVAS_API int
evas_object_vg_animated_frame_get(const Evas_Object *obj)
{
   return efl_gfx_frame_controller_frame_get(obj);
}

/**
 * @brief Gets the duration of a specific frame or sequence of frames in an animated vector graphics object.
 * @param obj The Evas object.
 * @param start_frame The starting frame index.
 * @param frame_num The number of frames.
 * @return The duration in seconds.
 * @see efl_gfx_frame_controller_frame_duration_get()
 * @ingroup Evas_Object_Group_VG_Animation
 */
EVAS_API double
evas_object_vg_animated_frame_duration_get(const Evas_Object *obj, int start_frame, int frame_num)
{
   return efl_gfx_frame_controller_frame_duration_get(obj, start_frame, frame_num);
}

/**
 * @brief Gets the total number of frames in an animated vector graphics object.
 * @param obj The Evas object.
 * @return The total frame count.
 * @see efl_gfx_frame_controller_frame_count_get()
 * @ingroup Evas_Object_Group_VG_Animation
 */
EVAS_API int
evas_object_vg_animated_frame_count_get(const Evas_Object *obj)
{
   return efl_gfx_frame_controller_frame_count_get(obj);
}

/**
 * @brief Sets the current frame index of an animated vector graphics object.
 * @param obj The Evas object.
 * @param frame_index The frame index to set.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 * @see efl_gfx_frame_controller_frame_set()
 * @ingroup Evas_Object_Group_VG_Animation
 */
EVAS_API Eina_Bool
evas_object_vg_animated_frame_set(Evas_Object *obj, int frame_index)
{
   return efl_gfx_frame_controller_frame_set(obj, frame_index);
}

/**
 * @brief Sets the file and key for the vector graphics object and loads it.
 * @param obj The Evas object.
 * @param file The path to the vector graphics file.
 * @param key Optional key within the file (e.g., for Lottie animations or SVG elements).
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 * @see efl_file_simple_load()
 * @ingroup Evas_Object_Group_VG_File
 */
EVAS_API Eina_Bool
evas_object_vg_file_set(Evas_Object *obj, const char *file, const char *key)
{
   return efl_file_simple_load(obj, file, key);
}

/**
 * @brief Converts an Evas_Object_Vg_Fill_Mode to Efl_Canvas_Vg_Fill_Mode.
 * @param mode The Evas_Object_Vg_Fill_Mode to convert.
 * @return The corresponding Efl_Canvas_Vg_Fill_Mode.
 */
static inline Efl_Canvas_Vg_Fill_Mode
_evas_object_vg_fill_mode_to_efl_ui_canvas_object_vg_fill_mode(Evas_Object_Vg_Fill_Mode mode)
{
   switch (mode)
     {
#define CONVERT_MODE(MODE) case EVAS_OBJECT_VG_FILL_MODE_##MODE: return EFL_CANVAS_VG_FILL_MODE_##MODE
       CONVERT_MODE(NONE);
       CONVERT_MODE(STRETCH);
       CONVERT_MODE(MEET);
       CONVERT_MODE(SLICE);
       default: break;
     }
#undef CONVERT_MODE
   return EFL_CANVAS_VG_FILL_MODE_NONE;
}

/**
 * @brief Converts an Efl_Canvas_Vg_Fill_Mode to Evas_Object_Vg_Fill_Mode.
 * @param mode The Efl_Canvas_Vg_Fill_Mode to convert.
 * @return The corresponding Evas_Object_Vg_Fill_Mode.
 */
static inline Evas_Object_Vg_Fill_Mode
_efl_ui_canvas_object_vg_fill_mode_to_evas_object_vg_fill_mode(Efl_Canvas_Vg_Fill_Mode mode)
{
   switch (mode)
     {
#define CONVERT_MODE(MODE) case EFL_CANVAS_VG_FILL_MODE_##MODE: return EVAS_OBJECT_VG_FILL_MODE_##MODE
       CONVERT_MODE(NONE);
       CONVERT_MODE(STRETCH);
       CONVERT_MODE(MEET);
       CONVERT_MODE(SLICE);
       default: break;
     }
#undef CONVERT_MODE
   return EVAS_OBJECT_VG_FILL_MODE_NONE;
}

/**
 * @brief Sets the fill mode of the vector graphics object.
 *
 * The fill mode determines how the vector graphic is scaled and positioned
 * within the object's area when its viewbox aspect ratio differs from the
 * object's aspect ratio.
 *
 * @param obj The Evas object.
 * @param fill_mode The Evas_Object_Vg_Fill_Mode to set.
 *                  Example: EVAS_OBJECT_VG_FILL_MODE_MEET
 * @see efl_canvas_vg_object_fill_mode_set()
 * @ingroup Evas_Object_Group_VG
 */
EVAS_API void
evas_object_vg_fill_mode_set(Evas_Object *obj, Evas_Object_Vg_Fill_Mode fill_mode)
{
   efl_canvas_vg_object_fill_mode_set(obj, _evas_object_vg_fill_mode_to_efl_ui_canvas_object_vg_fill_mode(fill_mode));
}

/**
 * @brief Gets the current fill mode of the vector graphics object.
 * @param obj The Evas object.
 * @return The current Evas_Object_Vg_Fill_Mode.
 * @see efl_canvas_vg_object_fill_mode_get()
 * @ingroup Evas_Object_Group_VG
 */
EVAS_API Evas_Object_Vg_Fill_Mode
evas_object_vg_fill_mode_get(const Evas_Object *obj)
{
   return _efl_ui_canvas_object_vg_fill_mode_to_evas_object_vg_fill_mode(efl_canvas_vg_object_fill_mode_get(obj));
}

/**
 * @brief Checks if the vector graphics object's content has changed.
 *
 * This is an internal Evas function to query the `changed` flag of the object's
 * private data.
 *
 * @param obj The protected data of the Evas object.
 * @return EINA_TRUE if the content has changed, EINA_FALSE otherwise.
 * @internal
 */
Eina_Bool
evas_object_vg_changed_get(Evas_Object_Protected_Data *obj)
{
   Efl_Canvas_Vg_Object_Data *pd = obj->private_data;
   return pd->changed;
}

#include "efl_canvas_vg_object.eo.c"
#include "efl_canvas_vg_object_eo.legacy.c"
