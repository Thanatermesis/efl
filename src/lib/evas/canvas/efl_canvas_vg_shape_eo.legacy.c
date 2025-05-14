/**
 * @brief Legacy wrapper for efl_canvas_vg_shape_fill_set().
 * @see evas_vg_shape_fill_set() in efl_canvas_vg_shape_eo.legacy.h for detailed documentation.
 */
EVAS_API void
evas_vg_shape_fill_set(Efl_Canvas_Vg_Shape *obj, Efl_Canvas_Vg_Node *f)
{
   efl_canvas_vg_shape_fill_set(obj, f);
}

/**
 * @brief Legacy wrapper for efl_canvas_vg_shape_fill_get().
 * @see evas_vg_shape_fill_get() in efl_canvas_vg_shape_eo.legacy.h for detailed documentation.
 */
EVAS_API Efl_Canvas_Vg_Node *
evas_vg_shape_fill_get(const Efl_Canvas_Vg_Shape *obj)
{
   return efl_canvas_vg_shape_fill_get(obj);
}

/**
 * @brief Legacy wrapper for efl_canvas_vg_shape_stroke_fill_set().
 * @see evas_vg_shape_stroke_fill_set() in efl_canvas_vg_shape_eo.legacy.h for detailed documentation.
 */
EVAS_API void
evas_vg_shape_stroke_fill_set(Efl_Canvas_Vg_Shape *obj, Efl_Canvas_Vg_Node *f)
{
   efl_canvas_vg_shape_stroke_fill_set(obj, f);
}

/**
 * @brief Legacy wrapper for efl_canvas_vg_shape_stroke_fill_get().
 * @see evas_vg_shape_stroke_fill_get() in efl_canvas_vg_shape_eo.legacy.h for detailed documentation.
 */
EVAS_API Efl_Canvas_Vg_Node *
evas_vg_shape_stroke_fill_get(const Efl_Canvas_Vg_Shape *obj)
{
   return efl_canvas_vg_shape_stroke_fill_get(obj);
}

/**
 * @brief Legacy wrapper for efl_canvas_vg_shape_stroke_marker_set().
 * @see evas_vg_shape_stroke_marker_set() in efl_canvas_vg_shape_eo.legacy.h for detailed documentation.
 */
EVAS_API void
evas_vg_shape_stroke_marker_set(Efl_Canvas_Vg_Shape *obj, Efl_Canvas_Vg_Node *m)
{
   efl_canvas_vg_shape_stroke_marker_set(obj, m);
}

/**
 * @brief Legacy wrapper for efl_canvas_vg_shape_stroke_marker_get().
 * @see evas_vg_shape_stroke_marker_get() in efl_canvas_vg_shape_eo.legacy.h for detailed documentation.
 */
EVAS_API Efl_Canvas_Vg_Node *
evas_vg_shape_stroke_marker_get(const Efl_Canvas_Vg_Shape *obj)
{
   return efl_canvas_vg_shape_stroke_marker_get(obj);
}
