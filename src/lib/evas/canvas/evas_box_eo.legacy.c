/**
 * @brief Legacy C API wrapper for setting box alignment.
 *
 * This function calls evas_obj_box_align_set().
 * Refer to the documentation of evas_object_box_align_set() in evas_box_eo.legacy.h
 * for details on parameters and behavior.
 *
 * @ingroup Evas_Object_Box_Group
 */
EVAS_API void
evas_object_box_align_set(Evas_Box *obj, double horizontal, double vertical)
{
   evas_obj_box_align_set(obj, horizontal, vertical);
}
/**
 * @brief Legacy C API wrapper for getting box alignment.
 *
 * This function calls evas_obj_box_align_get().
 * Refer to the documentation of evas_object_box_align_get() in evas_box_eo.legacy.h
 * for details on parameters and behavior.
 *
 * @ingroup Evas_Object_Box_Group
 */
EVAS_API void
evas_object_box_align_get(const Evas_Box *obj, double *horizontal, double *vertical)
{
   evas_obj_box_align_get(obj, horizontal, vertical);
}
/**
 * @brief Legacy C API wrapper for setting box padding.
 *
 * This function calls evas_obj_box_padding_set().
 * Refer to the documentation of evas_object_box_padding_set() in evas_box_eo.legacy.h
 * for details on parameters and behavior.
 *
 * @ingroup Evas_Object_Box_Group
 */
EVAS_API void
evas_object_box_padding_set(Evas_Box *obj, int horizontal, int vertical)
{
   evas_obj_box_padding_set(obj, horizontal, vertical);
}
/**
 * @brief Legacy C API wrapper for getting box padding.
 *
 * This function calls evas_obj_box_padding_get().
 * Refer to the documentation of evas_object_box_padding_get() in evas_box_eo.legacy.h
 * for details on parameters and behavior.
 *
 * @ingroup Evas_Object_Box_Group
 */
EVAS_API void
evas_object_box_padding_get(const Evas_Box *obj, int *horizontal, int *vertical)
{
   evas_obj_box_padding_get(obj, horizontal, vertical);
}
/**
 * @brief Legacy C API wrapper for setting the box layout function.
 *
 * This function calls evas_obj_box_layout_set().
 * Refer to the documentation of evas_object_box_layout_set() in evas_box_eo.legacy.h
 * for details on parameters and behavior.
 *
 * @ingroup Evas_Object_Box_Group
 */
EVAS_API void
evas_object_box_layout_set(Evas_Box *obj, Evas_Object_Box_Layout cb, const void *data, Eina_Free_Cb free_data)
{
   evas_obj_box_layout_set(obj, cb, data, free_data);
}
/**
 * @brief Legacy C API wrapper for the horizontal box layout function.
 *
 * This function calls evas_obj_box_layout_horizontal().
 * Refer to the documentation of evas_object_box_layout_horizontal() in evas_box_eo.legacy.h
 * for details on parameters and behavior.
 *
 * @ingroup Evas_Object_Box_Group
 */
EVAS_API void
evas_object_box_layout_horizontal(Evas_Box *obj, Evas_Object_Box_Data *priv, void *data)
{
   evas_obj_box_layout_horizontal(obj, priv, data);
}
/**
 * @brief Legacy C API wrapper for the vertical box layout function.
 *
 * This function calls evas_obj_box_layout_vertical().
 * Refer to the documentation of evas_object_box_layout_vertical() in evas_box_eo.legacy.h
 * for details on parameters and behavior.
 *
 * @ingroup Evas_Object_Box_Group
 */
EVAS_API void
evas_object_box_layout_vertical(Evas_Box *obj, Evas_Object_Box_Data *priv, void *data)
{
   evas_obj_box_layout_vertical(obj, priv, data);
}
/**
 * @brief Legacy C API wrapper for the homogeneous max size horizontal box layout function.
 *
 * This function calls evas_obj_box_layout_homogeneous_max_size_horizontal().
 * Refer to the documentation of evas_object_box_layout_homogeneous_max_size_horizontal()
 * in evas_box_eo.legacy.h for details on parameters and behavior.
 *
 * @ingroup Evas_Object_Box_Group
 */
EVAS_API void
evas_object_box_layout_homogeneous_max_size_horizontal(Evas_Box *obj, Evas_Object_Box_Data *priv, void *data)
{
   evas_obj_box_layout_homogeneous_max_size_horizontal(obj, priv, data);
}
/**
 * @brief Legacy C API wrapper for the flow vertical box layout function.
 *
 * This function calls evas_obj_box_layout_flow_vertical().
 * Refer to the documentation of evas_object_box_layout_flow_vertical()
 * in evas_box_eo.legacy.h for details on parameters and behavior.
 *
 * @ingroup Evas_Object_Box_Group
 */
EVAS_API void
evas_object_box_layout_flow_vertical(Evas_Box *obj, Evas_Object_Box_Data *priv, void *data)
{
   evas_obj_box_layout_flow_vertical(obj, priv, data);
}
/**
 * @brief Legacy C API wrapper for inserting a child object after a reference object in a box.
 *
 * This function calls evas_obj_box_insert_after().
 * Refer to the documentation of evas_object_box_insert_after() in evas_box_eo.legacy.h
 * for details on parameters and behavior.
 *
 * @ingroup Evas_Object_Box_Group
 */
EVAS_API Evas_Object_Box_Option *
evas_object_box_insert_after(Evas_Box *obj, Efl_Canvas_Object *child, const Efl_Canvas_Object *reference)
{
   return evas_obj_box_insert_after(obj, child, reference);
}
/**
 * @brief Legacy C API wrapper for removing all child objects from a box.
 *
 * This function calls evas_obj_box_remove_all().
 * Refer to the documentation of evas_object_box_remove_all() in evas_box_eo.legacy.h
 * for details on parameters and behavior.
 *
 * @ingroup Evas_Object_Box_Group
 */
EVAS_API Eina_Bool
evas_object_box_remove_all(Evas_Box *obj, Eina_Bool clear)
{
   return evas_obj_box_remove_all(obj, clear);
}
/**
 * @brief Legacy C API wrapper for creating an iterator for box children.
 *
 * This function calls evas_obj_box_iterator_new().
 * Refer to the documentation of evas_object_box_iterator_new() in evas_box_eo.legacy.h
 * for details on parameters and behavior.
 *
 * @ingroup Evas_Object_Box_Group
 */
EVAS_API Eina_Iterator *
evas_object_box_iterator_new(const Evas_Box *obj)
{
   return evas_obj_box_iterator_new(obj);
}
/**
 * @brief Legacy C API wrapper to add a new box object as a child of @p obj.
 *
 * This function calls evas_obj_box_add_to() where @p obj is the parent object.
 * A new Evas_Box instance is created and added as a child to @p obj.
 * Refer to the documentation of evas_object_box_add_to() in evas_box_eo.legacy.h
 * for details on parameters and behavior.
 *
 * @ingroup Evas_Object_Box_Group
 */
EVAS_API Efl_Canvas_Object *
evas_object_box_add_to(Evas_Box *obj)
{
   return evas_obj_box_add_to(obj);
}
/**
 * @brief Legacy C API wrapper for appending a child object to a box.
 *
 * This function calls evas_obj_box_append().
 * Refer to the documentation of evas_object_box_append() in evas_box_eo.legacy.h
 * for details on parameters and behavior.
 *
 * @ingroup Evas_Object_Box_Group
 */
EVAS_API Evas_Object_Box_Option *
evas_object_box_append(Evas_Box *obj, Efl_Canvas_Object *child)
{
   return evas_obj_box_append(obj, child);
}
/**
 * @brief Legacy C API wrapper for getting the ID of a box option property by name.
 *
 * This function calls evas_obj_box_option_property_id_get().
 * Refer to the documentation of evas_object_box_option_property_id_get()
 * in evas_box_eo.legacy.h for details on parameters and behavior.
 *
 * @ingroup Evas_Object_Box_Group
 */
EVAS_API int
evas_object_box_option_property_id_get(const Evas_Box *obj, const char *name)
{
   return evas_obj_box_option_property_id_get(obj, name);
}
/**
 * @brief Legacy C API wrapper for prepending a child object to a box.
 *
 * This function calls evas_obj_box_prepend().
 * Refer to the documentation of evas_object_box_prepend() in evas_box_eo.legacy.h
 * for details on parameters and behavior.
 *
 * @ingroup Evas_Object_Box_Group
 */
EVAS_API Evas_Object_Box_Option *
evas_object_box_prepend(Evas_Box *obj, Efl_Canvas_Object *child)
{
   return evas_obj_box_prepend(obj, child);
}
/**
 * @brief Legacy C API wrapper for creating an accessor for box children.
 *
 * This function calls evas_obj_box_accessor_new().
 * Refer to the documentation of evas_object_box_accessor_new() in evas_box_eo.legacy.h
 * for details on parameters and behavior.
 *
 * @ingroup Evas_Object_Box_Group
 */
EVAS_API Eina_Accessor *
evas_object_box_accessor_new(const Evas_Box *obj)
{
   return evas_obj_box_accessor_new(obj);
}
/**
 * @brief Legacy C API wrapper for removing a child object from a box at a given position.
 *
 * This function calls evas_obj_box_remove_at().
 * Refer to the documentation of evas_object_box_remove_at() in evas_box_eo.legacy.h
 * for details on parameters and behavior.
 *
 * @ingroup Evas_Object_Box_Group
 */
EVAS_API Eina_Bool
evas_object_box_remove_at(Evas_Box *obj, unsigned int pos)
{
   return evas_obj_box_remove_at(obj, pos);
}
/**
 * @brief Legacy C API wrapper for inserting a child object before a reference object in a box.
 *
 * This function calls evas_obj_box_insert_before().
 * Refer to the documentation of evas_object_box_insert_before() in evas_box_eo.legacy.h
 * for details on parameters and behavior.
 *
 * @ingroup Evas_Object_Box_Group
 */
EVAS_API Evas_Object_Box_Option *
evas_object_box_insert_before(Evas_Box *obj, Efl_Canvas_Object *child, const Efl_Canvas_Object *reference)
{
   return evas_obj_box_insert_before(obj, child, reference);
}
/**
 * @brief Legacy C API wrapper for getting the name of a box option property by ID.
 *
 * This function calls evas_obj_box_option_property_name_get().
 * Refer to the documentation of evas_object_box_option_property_name_get()
 * in evas_box_eo.legacy.h for details on parameters and behavior.
 *
 * @ingroup Evas_Object_Box_Group
 */
EVAS_API const char *
evas_object_box_option_property_name_get(const Evas_Box *obj, int property)
{
   return evas_obj_box_option_property_name_get(obj, property);
}
/**
 * @brief Legacy C API wrapper for the homogeneous horizontal box layout function.
 *
 * This function calls evas_obj_box_layout_homogeneous_horizontal().
 * Refer to the documentation of evas_object_box_layout_homogeneous_horizontal()
 * in evas_box_eo.legacy.h for details on parameters and behavior.
 *
 * @ingroup Evas_Object_Box_Group
 */
EVAS_API void
evas_object_box_layout_homogeneous_horizontal(Evas_Box *obj, Evas_Object_Box_Data *priv, void *data)
{
   evas_obj_box_layout_homogeneous_horizontal(obj, priv, data);
}
/**
 * @brief Legacy C API wrapper for the homogeneous max size vertical box layout function.
 *
 * This function calls evas_obj_box_layout_homogeneous_max_size_vertical().
 * Refer to the documentation of evas_object_box_layout_homogeneous_max_size_vertical()
 * in evas_box_eo.legacy.h for details on parameters and behavior.
 *
 * @ingroup Evas_Object_Box_Group
 */
EVAS_API void
evas_object_box_layout_homogeneous_max_size_vertical(Evas_Box *obj, Evas_Object_Box_Data *priv, void *data)
{
   evas_obj_box_layout_homogeneous_max_size_vertical(obj, priv, data);
}
/**
 * @brief Legacy C API wrapper for inserting a child object into a box at a given position.
 *
 * This function calls evas_obj_box_insert_at().
 * Refer to the documentation of evas_object_box_insert_at() in evas_box_eo.legacy.h
 * for details on parameters and behavior.
 *
 * @ingroup Evas_Object_Box_Group
 */
EVAS_API Evas_Object_Box_Option *
evas_object_box_insert_at(Evas_Box *obj, Efl_Canvas_Object *child, unsigned int pos)
{
   return evas_obj_box_insert_at(obj, child, pos);
}
/**
 * @brief Legacy C API wrapper for removing a specific child object from a box.
 *
 * This function calls evas_obj_box_remove().
 * Refer to the documentation of evas_object_box_remove() in evas_box_eo.legacy.h
 * for details on parameters and behavior.
 *
 * @ingroup Evas_Object_Box_Group
 */
EVAS_API Eina_Bool
evas_object_box_remove(Evas_Box *obj, Efl_Canvas_Object *child)
{
   return evas_obj_box_remove(obj, child);
}
/**
 * @brief Legacy C API wrapper for the stack box layout function.
 *
 * This function calls evas_obj_box_layout_stack().
 * Refer to the documentation of evas_object_box_layout_stack() in evas_box_eo.legacy.h
 * for details on parameters and behavior.
 *
 * @ingroup Evas_Object_Box_Group
 */
EVAS_API void
evas_object_box_layout_stack(Evas_Box *obj, Evas_Object_Box_Data *priv, void *data)
{
   evas_obj_box_layout_stack(obj, priv, data);
}
/**
 * @brief Legacy C API wrapper for the homogeneous vertical box layout function.
 *
 * This function calls evas_obj_box_layout_homogeneous_vertical().
 * Refer to the documentation of evas_object_box_layout_homogeneous_vertical()
 * in evas_box_eo.legacy.h for details on parameters and behavior.
 *
 * @ingroup Evas_Object_Box_Group
 */
EVAS_API void
evas_object_box_layout_homogeneous_vertical(Evas_Box *obj, Evas_Object_Box_Data *priv, void *data)
{
   evas_obj_box_layout_homogeneous_vertical(obj, priv, data);
}
/**
 * @brief Legacy C API wrapper for the flow horizontal box layout function.
 *
 * This function calls evas_obj_box_layout_flow_horizontal().
 * Refer to the documentation of evas_object_box_layout_flow_horizontal()
 * in evas_box_eo.legacy.h for details on parameters and behavior.
 *
 * @ingroup Evas_Object_Box_Group
 */
EVAS_API void
evas_object_box_layout_flow_horizontal(Evas_Box *obj, Evas_Object_Box_Data *priv, void *data)
{
   evas_obj_box_layout_flow_horizontal(obj, priv, data);
}
