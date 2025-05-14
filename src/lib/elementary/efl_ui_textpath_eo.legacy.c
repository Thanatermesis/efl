/**
 * @brief Legacy wrapper for efl_ui_textpath_circular_set().
 * @see efl_ui_textpath_circular_set()
 */
EAPI void
elm_textpath_circular_set(Efl_Ui_Textpath *obj, double radius, double start_angle, Efl_Ui_Textpath_Direction direction)
{
   efl_ui_textpath_circular_set(obj, radius, start_angle, direction);
}

/**
 * @brief Legacy wrapper for efl_ui_textpath_slice_number_set().
 * @see efl_ui_textpath_slice_number_set()
 */
EAPI void
elm_textpath_slice_number_set(Efl_Ui_Textpath *obj, int slice_no)
{
   efl_ui_textpath_slice_number_set(obj, slice_no);
}

/**
 * @brief Legacy wrapper for efl_ui_textpath_slice_number_get().
 * @see efl_ui_textpath_slice_number_get()
 */
EAPI int
elm_textpath_slice_number_get(const Efl_Ui_Textpath *obj)
{
   return efl_ui_textpath_slice_number_get(obj);
}

/**
 * @brief Legacy wrapper for efl_ui_textpath_ellipsis_set().
 * @see efl_ui_textpath_ellipsis_set()
 */
EAPI void
elm_textpath_ellipsis_set(Efl_Ui_Textpath *obj, Eina_Bool ellipsis)
{
   efl_ui_textpath_ellipsis_set(obj, ellipsis);
}

/**
 * @brief Legacy wrapper for efl_ui_textpath_ellipsis_get().
 * @see efl_ui_textpath_ellipsis_get()
 */
EAPI Eina_Bool
elm_textpath_ellipsis_get(const Efl_Ui_Textpath *obj)
{
   return efl_ui_textpath_ellipsis_get(obj);
}
