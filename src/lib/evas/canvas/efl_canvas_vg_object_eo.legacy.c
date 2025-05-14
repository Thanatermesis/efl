/**
 * @brief Legacy wrapper for efl_canvas_vg_object_viewbox_set().
 * @details This function provides a legacy C API for setting the viewbox
 * of an Evas VG object. It directly calls the EFL EAPI function
 * efl_canvas_vg_object_viewbox_set().
 * For detailed documentation on parameters and behavior, refer to the
 * declaration in efl_canvas_vg_object_eo.legacy.h.
 */
EVAS_API void
evas_object_vg_viewbox_set(Evas_Object *obj, Eina_Rect viewbox)
{
   efl_canvas_vg_object_viewbox_set(obj, viewbox);
}

/**
 * @brief Legacy wrapper for efl_canvas_vg_object_viewbox_get().
 * @details This function provides a legacy C API for getting the viewbox
 * of an Evas VG object. It directly calls the EFL EAPI function
 * efl_canvas_vg_object_viewbox_get().
 * For detailed documentation on parameters and return value, refer to the
 * declaration in efl_canvas_vg_object_eo.legacy.h.
 */
EVAS_API Eina_Rect
evas_object_vg_viewbox_get(const Evas_Object *obj)
{
   return efl_canvas_vg_object_viewbox_get(obj);
}

/**
 * @brief Legacy wrapper for efl_canvas_vg_object_viewbox_align_set().
 * @details This function provides a legacy C API for setting the viewbox alignment
 * of an Evas VG object. It directly calls the EFL EAPI function
 * efl_canvas_vg_object_viewbox_align_set().
 * For detailed documentation on parameters and behavior, refer to the
 * declaration in efl_canvas_vg_object_eo.legacy.h.
 */
EVAS_API void
evas_object_vg_viewbox_align_set(Evas_Object *obj, double align_x, double align_y)
{
   efl_canvas_vg_object_viewbox_align_set(obj, align_x, align_y);
}

/**
 * @brief Legacy wrapper for efl_canvas_vg_object_viewbox_align_get().
 * @details This function provides a legacy C API for getting the viewbox alignment
 * of an Evas VG object. It directly calls the EFL EAPI function
 * efl_canvas_vg_object_viewbox_align_get().
 * For detailed documentation on parameters and behavior, refer to the
 * declaration in efl_canvas_vg_object_eo.legacy.h.
 */
EVAS_API void
evas_object_vg_viewbox_align_get(const Evas_Object *obj, double *align_x, double *align_y)
{
   efl_canvas_vg_object_viewbox_align_get(obj, align_x, align_y);
}

/**
 * @brief Legacy wrapper for efl_canvas_vg_object_root_node_set().
 * @details This function provides a legacy C API for setting the root node
 * of an Evas VG object. It directly calls the EFL EAPI function
 * efl_canvas_vg_object_root_node_set().
 * The `root` parameter type in this implementation is `Efl_Canvas_Vg_Node *`,
 * which corresponds to the underlying EFL API.
 * For detailed documentation on the public API, refer to the
 * declaration in efl_canvas_vg_object_eo.legacy.h (which may use `Evas_Vg_Node *`).
 */
EVAS_API void
evas_object_vg_root_node_set(Evas_Object *obj, Efl_Canvas_Vg_Node *root)
{
   efl_canvas_vg_object_root_node_set(obj, root);
}

/**
 * @brief Legacy wrapper for efl_canvas_vg_object_root_node_get().
 * @details This function provides a legacy C API for getting the root node
 * of an Evas VG object. It directly calls the EFL EAPI function
 * efl_canvas_vg_object_root_node_get().
 * The return type in this implementation is `Efl_Canvas_Vg_Node *`,
 * which corresponds to the underlying EFL API.
 * For detailed documentation on the public API, refer to the
 * declaration in efl_canvas_vg_object_eo.legacy.h (which may return `Evas_Vg_Node *`).
 */
EVAS_API Efl_Canvas_Vg_Node *
evas_object_vg_root_node_get(const Evas_Object *obj)
{
   return efl_canvas_vg_object_root_node_get(obj);
}
