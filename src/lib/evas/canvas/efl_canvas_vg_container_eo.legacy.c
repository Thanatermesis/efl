/**
 * @brief Get a child node from a Evas_Vg_Container by its name.
 *
 * @param[in] obj The Evas_Vg_Container object.
 * @param[in] name The name of the child node to retrieve.
 * @return The Evas_Vg_Node child object if found, otherwise @c NULL.
 *
 * @see efl_canvas_vg_container_child_get()
 */
EVAS_API Evas_Vg_Node *
evas_vg_container_child_get(Evas_Vg_Container *obj, const char *name)
{
   return efl_canvas_vg_container_child_get(obj, name);
}

/**
 * @brief Get an iterator for all child nodes of a Evas_Vg_Container.
 *
 * @param[in] obj The Evas_Vg_Container object.
 * @return An Eina_Iterator that iterates over the Evas_Vg_Node children.
 *         The iterator must be freed using eina_iterator_free() when no longer needed.
 *         Returns @c NULL on failure or if the container has no children.
 *
 * @see efl_canvas_vg_container_children_get()
 */
EVAS_API Eina_Iterator *
evas_vg_container_children_get(Evas_Vg_Container *obj)
{
   return efl_canvas_vg_container_children_get(obj);
}
