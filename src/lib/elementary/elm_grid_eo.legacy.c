/**
 * @brief Set the virtual size of the grid.
 * @param[in] obj The object.
 * @param[in] w The virtual width of the grid.
 * @param[in] h The virtual height of the grid.
 */
EAPI void
elm_grid_size_set(Elm_Grid *obj, int w, int h)
{
   elm_obj_grid_size_set(obj, w, h);
}

/**
 * @brief Get the virtual size of the grid.
 * @param[in] obj The object.
 * @param[out] w The virtual width of the grid.
 * @param[out] h The virtual height of the grid.
 */
EAPI void
elm_grid_size_get(const Elm_Grid *obj, int *w, int *h)
{
   elm_obj_grid_size_get(obj, w, h);
}

/**
 * @brief Get the list of the children for the grid.
 * @param[in] obj The object.
 * @return List of children.
 */
EAPI Eina_List *
elm_grid_children_get(const Elm_Grid *obj)
{
   return elm_obj_grid_children_get(obj);
}

/**
 * @brief Faster way to remove all child objects from a grid object.
 * @param[in] obj The object.
 * @param[in] clear If @c EINA_TRUE, it will delete just removed children.
 */
EAPI void
elm_grid_clear(Elm_Grid *obj, Eina_Bool clear)
{
   elm_obj_grid_clear(obj, clear);
}

/**
 * @brief Unpack a child from a grid object.
 * @param[in] obj The object.
 * @param[in] subobj The child to unpack.
 */
EAPI void
elm_grid_unpack(Elm_Grid *obj, Efl_Canvas_Object *subobj)
{
   elm_obj_grid_unpack(obj, subobj);
}

/**
 * @brief Pack child at given position and size.
 * @param[in] obj The object.
 * @param[in] subobj The child to pack.
 * @param[in] x The virtual x coord at which to pack it.
 * @param[in] y The virtual y coord at which to pack it.
 * @param[in] w The virtual width at which to pack it.
 * @param[in] h The virtual height at which to pack it.
 */
EAPI void
elm_grid_pack(Elm_Grid *obj, Efl_Canvas_Object *subobj, int x, int y, int w, int h)
{
   elm_obj_grid_pack(obj, subobj, x, y, w, h);
}
