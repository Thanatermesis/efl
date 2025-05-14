/**
 * @brief Set the virtual resolution for the grid.
 * @param obj The Evas_Grid object.
 * @param w The virtual horizontal size (resolution) in integer units.
 * @param h The virtual vertical size (resolution) in integer units.
 */
EVAS_API void
evas_object_grid_size_set(Evas_Grid *obj, int w, int h)
{
   evas_obj_grid_size_set(obj, w, h);
}
/**
 * @brief Get the current virtual resolution for the grid.
 * @param obj The Evas_Grid object.
 * @param[out] w Pointer to store the virtual horizontal size.
 * @param[out] h Pointer to store the virtual vertical size.
 * @see evas_object_grid_size_set()
 */
EVAS_API void
evas_object_grid_size_get(const Evas_Grid *obj, int *w, int *h)
{
   evas_obj_grid_size_get(obj, w, h);
}
/**
 * @brief Get the list of children for the grid.
 * @param obj The Evas_Grid object.
 * @return A list (Eina_List *) of child objects. The caller is responsible
 *         for freeing this list.
 * @note This is a duplicate of the list kept by the grid internally. It's up
 * to the user to destroy it when it no longer needs it. It's possible to
 * remove objects from the grid when walking this list, but these removals
 * won't be reflected on it.
 */
EVAS_API Eina_List *
evas_object_grid_children_get(const Evas_Grid *obj)
{
   return evas_obj_grid_children_get(obj);
}
/**
 * @brief Get an accessor for the grid's children list.
 * @param obj The Evas_Grid object.
 * @return An Eina_Accessor for the children list.
 * @note Do not remove or delete objects while using the accessor.
 */
EVAS_API Eina_Accessor *
evas_object_grid_accessor_new(const Evas_Grid *obj)
{
   return evas_obj_grid_accessor_new(obj);
}
/**
 * @brief Removes all child objects from a grid object.
 * @param obj The Evas_Grid object.
 * @param clear If EINA_TRUE, also delete the removed children.
 */
EVAS_API void
evas_object_grid_clear(Evas_Grid *obj, Eina_Bool clear)
{
   evas_obj_grid_clear(obj, clear);
}
/**
 * @brief Get an iterator for the grid's children list.
 * @param obj The Evas_Grid object.
 * @return An Eina_Iterator for the children list.
 * @note Do not remove or delete objects while using the iterator.
 */
EVAS_API Eina_Iterator *
evas_object_grid_iterator_new(const Evas_Grid *obj)
{
   return evas_obj_grid_iterator_new(obj);
}
/**
 * @brief Adds a new grid object as a child of the given parent object.
 * @param obj The parent Evas object (Evas_Grid type).
 * @return The new Efl_Canvas_Object (grid) on success, or NULL on failure.
 * @see evas_object_grid_add()
 */
EVAS_API Efl_Canvas_Object *
evas_object_grid_add_to(Evas_Grid *obj)
{
   return evas_obj_grid_add_to(obj);
}
/**
 * @brief Removes a child object from the grid.
 * @param obj The Evas_Grid object.
 * @param child The child object to remove.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 * @note Removing a child will trigger a recalculation of grid dimensions.
 *       For removing all children, evas_object_grid_clear() is more efficient.
 */
EVAS_API Eina_Bool
evas_object_grid_unpack(Evas_Grid *obj, Efl_Canvas_Object *child)
{
   return evas_obj_grid_unpack(obj, child);
}
/**
 * @brief Retrieves the packing geometry of a child object within the grid.
 * @param obj The Evas_Grid object.
 * @param child The child object.
 * @param[out] x Pointer to store the virtual x coordinate.
 * @param[out] y Pointer to store the virtual y coordinate.
 * @param[out] w Pointer to store the virtual width.
 * @param[out] h Pointer to store the virtual height.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 * @see evas_object_grid_pack()
 */
EVAS_API Eina_Bool
evas_object_grid_pack_get(const Evas_Grid *obj, Efl_Canvas_Object *child, int *x, int *y, int *w, int *h)
{
   return evas_obj_grid_pack_get(obj, child, x, y, w, h);
}
/**
 * @brief Packs a child object into the grid at a specific virtual geometry.
 * @param obj The Evas_Grid object.
 * @param child The child object to pack.
 * @param x The virtual x coordinate for the child.
 * @param y The virtual y coordinate for the child.
 * @param w The virtual width for the child.
 * @param h The virtual height for the child.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
EVAS_API Eina_Bool
evas_object_grid_pack(Evas_Grid *obj, Efl_Canvas_Object *child, int x, int y, int w, int h)
{
   return evas_obj_grid_pack(obj, child, x, y, w, h);
}
