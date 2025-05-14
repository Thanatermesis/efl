/**
 * @internal
 * @brief Legacy C API implementation for setting line coordinates.
 *
 * This function serves as a wrapper around the internal Evas function
 * @c evas_obj_line_xy_set, providing a stable C API for legacy code.
 * For detailed parameter descriptions and usage, refer to the declaration
 * in the corresponding header file.
 */
EVAS_API void
evas_object_line_xy_set(Evas_Line *obj, int x1, int y1, int x2, int y2)
{
   evas_obj_line_xy_set(obj, x1, y1, x2, y2);
}

/**
 * @internal
 * @brief Legacy C API implementation for getting line coordinates.
 *
 * This function serves as a wrapper around the internal Evas function
 * @c evas_obj_line_xy_get, providing a stable C API for legacy code.
 * For detailed parameter descriptions and usage, refer to the declaration
 * in the corresponding header file.
 */
EVAS_API void
evas_object_line_xy_get(const Evas_Line *obj, int *x1, int *y1, int *x2, int *y2)
{
   evas_obj_line_xy_get(obj, x1, y1, x2, y2);
}
