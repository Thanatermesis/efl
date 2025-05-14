
/**
 * @brief Legacy API wrapper: Set the image cache.
 *
 * This function serves as a public interface to the internal evas_canvas_image_cache_set() implementation.
 * For comprehensive documentation, please see the evas_canvas_eo.legacy.h header file.
 */
EVAS_API void
evas_image_cache_set(Evas_Canvas *obj, int size)
{
   evas_canvas_image_cache_set(obj, size);
}

/**
 * @brief Legacy API wrapper: Get the image cache.
 *
 * This function serves as a public interface to the internal evas_canvas_image_cache_get() implementation.
 * For comprehensive documentation, please see the evas_canvas_eo.legacy.h header file.
 */
EVAS_API int
evas_image_cache_get(const Evas_Canvas *obj)
{
   return evas_canvas_image_cache_get(obj);
}

/**
 * @brief Legacy API wrapper: Set the default set of flags an event begins with.
 *
 * This function serves as a public interface to the internal evas_canvas_event_default_flags_set() implementation.
 * For comprehensive documentation, please see the evas_canvas_eo.legacy.h header file.
 */
EVAS_API void
evas_event_default_flags_set(Evas_Canvas *obj, Evas_Event_Flags flags)
{
   evas_canvas_event_default_flags_set(obj, flags);
}

/**
 * @brief Legacy API wrapper: Get the default set of flags an event begins with.
 *
 * This function serves as a public interface to the internal evas_canvas_event_default_flags_get() implementation.
 * For comprehensive documentation, please see the evas_canvas_eo.legacy.h header file.
 */
EVAS_API Evas_Event_Flags
evas_event_default_flags_get(const Evas_Canvas *obj)
{
   return (Evas_Event_Flags)evas_canvas_event_default_flags_get(obj);
}

/**
 * @brief Legacy API wrapper: Changes the size of font cache of the given evas.
 *
 * This function serves as a public interface to the internal evas_canvas_font_cache_set() implementation.
 * For comprehensive documentation, please see the evas_canvas_eo.legacy.h header file.
 */
EVAS_API void
evas_font_cache_set(Evas_Canvas *obj, int size)
{
   evas_canvas_font_cache_set(obj, size);
}

/**
 * @brief Legacy API wrapper: Get the size of font cache of the given evas in bytes.
 *
 * This function serves as a public interface to the internal evas_canvas_font_cache_get() implementation.
 * For comprehensive documentation, please see the evas_canvas_eo.legacy.h header file.
 */
EVAS_API int
evas_font_cache_get(const Evas_Canvas *obj)
{
   return evas_canvas_font_cache_get(obj);
}

/**
 * @brief Legacy API wrapper: Attaches a specific pointer to the evas for fetching later.
 *
 * This function serves as a public interface to the internal evas_canvas_data_attach_set() implementation.
 * For comprehensive documentation, please see the evas_canvas_eo.legacy.h header file.
 */
EVAS_API void
evas_data_attach_set(Evas_Canvas *obj, void *data)
{
   evas_canvas_data_attach_set(obj, data);
}

/**
 * @brief Legacy API wrapper: Returns the pointer attached by evas_data_attach_set.
 *
 * This function serves as a public interface to the internal evas_canvas_data_attach_get() implementation.
 * For comprehensive documentation, please see the evas_canvas_eo.legacy.h header file.
 */
EVAS_API void *
evas_data_attach_get(const Evas_Canvas *obj)
{
   return evas_canvas_data_attach_get(obj);
}

/**
 * @brief Legacy API wrapper: Retrieve the object focused by the default seat.
 *
 * This function serves as a public interface to the internal evas_canvas_focus_get() implementation.
 * For comprehensive documentation, please see the evas_canvas_eo.legacy.h header file.
 */
EVAS_API Efl_Canvas_Object *
evas_focus_get(const Evas_Canvas *obj)
{
   return evas_canvas_focus_get(obj);
}

/**
 * @brief Legacy API wrapper: Return the focused object by a given seat.
 *
 * This function serves as a public interface to the internal evas_canvas_seat_focus_get() implementation.
 * For comprehensive documentation, please see the evas_canvas_eo.legacy.h header file.
 */
EVAS_API Efl_Canvas_Object *
evas_seat_focus_get(const Evas_Canvas *obj, Evas_Device *seat)
{
   return evas_canvas_seat_focus_get(obj, seat);
}

/**
 * @brief Legacy API wrapper: Get the highest (stacked) Evas object on the canvas.
 *
 * This function serves as a public interface to the internal evas_canvas_object_top_get() implementation.
 * For comprehensive documentation, please see the evas_canvas_eo.legacy.h header file.
 */
EVAS_API Efl_Canvas_Object *
evas_object_top_get(const Evas_Canvas *obj)
{
   return evas_canvas_object_top_get(obj);
}

/**
 * @brief Legacy API wrapper: Returns the current known pointer coordinates for a specific device.
 *
 * This function serves as a public interface to the internal evas_canvas_pointer_canvas_xy_by_device_get() implementation.
 * For comprehensive documentation, please see the evas_canvas_eo.legacy.h header file.
 */
EVAS_API void
evas_pointer_canvas_xy_by_device_get(const Evas_Canvas *obj, Evas_Device *dev, int *x, int *y)
{
   evas_canvas_pointer_canvas_xy_by_device_get(obj, dev, x, y);
}

/**
 * @brief Legacy API wrapper: Returns the current known default pointer coordinates.
 *
 * This function serves as a public interface to the internal evas_canvas_pointer_canvas_xy_get() implementation.
 * For comprehensive documentation, please see the evas_canvas_eo.legacy.h header file.
 */
EVAS_API void
evas_pointer_canvas_xy_get(const Evas_Canvas *obj, int *x, int *y)
{
   evas_canvas_pointer_canvas_xy_get(obj, x, y);
}

/**
 * @brief Legacy API wrapper: Get the number of mouse or multi presses currently active.
 *
 * This function serves as a public interface to the internal evas_canvas_event_down_count_get() implementation.
 * For comprehensive documentation, please see the evas_canvas_eo.legacy.h header file.
 */
EVAS_API int
evas_event_down_count_get(const Evas_Canvas *obj)
{
   return evas_canvas_event_down_count_get(obj);
}

/**
 * @brief Legacy API wrapper: Gets the internal counter for smart object calculations.
 *
 * This function serves as a public interface to the internal evas_canvas_smart_objects_calculate_count_get() implementation.
 * For comprehensive documentation, please see the evas_canvas_eo.legacy.h header file.
 */
EVAS_API int
evas_smart_objects_calculate_count_get(const Evas_Canvas *obj)
{
   return evas_canvas_smart_objects_calculate_count_get(obj);
}

/**
 * @brief Legacy API wrapper: Get the focus state for the default seat.
 *
 * This function serves as a public interface to the internal evas_canvas_focus_state_get() implementation.
 * For comprehensive documentation, please see the evas_canvas_eo.legacy.h header file.
 */
EVAS_API Eina_Bool
evas_focus_state_get(const Evas_Canvas *obj)
{
   return evas_canvas_focus_state_get(obj);
}

/**
 * @brief Legacy API wrapper: Get the focus state by a given seat.
 *
 * This function serves as a public interface to the internal evas_canvas_seat_focus_state_get() implementation.
 * For comprehensive documentation, please see the evas_canvas_eo.legacy.h header file.
 */
EVAS_API Eina_Bool
evas_seat_focus_state_get(const Evas_Canvas *obj, Evas_Device *seat)
{
   return evas_canvas_seat_focus_state_get(obj, seat);
}

/**
 * @brief Legacy API wrapper: Get the changed marker for the canvas.
 *
 * This function serves as a public interface to the internal evas_canvas_changed_get() implementation.
 * For comprehensive documentation, please see the evas_canvas_eo.legacy.h header file.
 */
EVAS_API Eina_Bool
evas_changed_get(const Evas_Canvas *obj)
{
   return evas_canvas_changed_get(obj);
}

/**
 * @brief Legacy API wrapper: Returns the current known pointer output coordinates for a specific device.
 *
 * This function serves as a public interface to the internal evas_canvas_pointer_output_xy_by_device_get() implementation.
 * For comprehensive documentation, please see the evas_canvas_eo.legacy.h header file.
 */
EVAS_API void
evas_pointer_output_xy_by_device_get(const Evas_Canvas *obj, Evas_Device *dev, int *x, int *y)
{
   evas_canvas_pointer_output_xy_by_device_get(obj, dev, x, y);
}

/**
 * @brief Legacy API wrapper: Returns the current known default pointer output coordinates.
 *
 * This function serves as a public interface to the internal evas_canvas_pointer_output_xy_get() implementation.
 * For comprehensive documentation, please see the evas_canvas_eo.legacy.h header file.
 */
EVAS_API void
evas_pointer_output_xy_get(const Evas_Canvas *obj, int *x, int *y)
{
   evas_canvas_pointer_output_xy_get(obj, x, y);
}

/**
 * @brief Legacy API wrapper: Get the lowest (stacked) Evas object on the canvas.
 *
 * This function serves as a public interface to the internal evas_canvas_object_bottom_get() implementation.
 * For comprehensive documentation, please see the evas_canvas_eo.legacy.h header file.
 */
EVAS_API Efl_Canvas_Object *
evas_object_bottom_get(const Evas_Canvas *obj)
{
   return evas_canvas_object_bottom_get(obj);
}

/**
 * @brief Legacy API wrapper: Returns a bitmask of currently pressed mouse buttons for a specific device.
 *
 * This function serves as a public interface to the internal evas_canvas_pointer_button_down_mask_by_device_get() implementation.
 * For comprehensive documentation, please see the evas_canvas_eo.legacy.h header file.
 */
EVAS_API unsigned int
evas_pointer_button_down_mask_by_device_get(const Evas_Canvas *obj, Evas_Device *dev)
{
   return evas_canvas_pointer_button_down_mask_by_device_get(obj, dev);
}

/**
 * @brief Legacy API wrapper: Returns a bitmask of currently pressed default mouse buttons.
 *
 * This function serves as a public interface to the internal evas_canvas_pointer_button_down_mask_get() implementation.
 * For comprehensive documentation, please see the evas_canvas_eo.legacy.h header file.
 */
EVAS_API unsigned int
evas_pointer_button_down_mask_get(const Evas_Canvas *obj)
{
   return evas_canvas_pointer_button_down_mask_get(obj);
}

/**
 * @brief Legacy API wrapper: Retrieve a list of Evas objects over a given position.
 *
 * This function serves as a public interface to the internal evas_canvas_tree_objects_at_xy_get() implementation.
 * For comprehensive documentation, please see the evas_canvas_eo.legacy.h header file.
 */
EVAS_API Eina_List *
evas_tree_objects_at_xy_get(Evas_Canvas *obj, Efl_Canvas_Object *stop, int x, int y)
{
   return evas_canvas_tree_objects_at_xy_get(obj, stop, x, y);
}

/**
 * @brief Legacy API wrapper: Turns on the lock key for the default seat.
 *
 * This function serves as a public interface to the internal evas_canvas_key_lock_on() implementation.
 * For comprehensive documentation, please see the evas_canvas_eo.legacy.h header file.
 */
EVAS_API void
evas_key_lock_on(Evas_Canvas *obj, const char *keyname)
{
   evas_canvas_key_lock_on(obj, keyname);
}

/**
 * @brief Legacy API wrapper: Turns on the lock key for a given seat.
 *
 * This function serves as a public interface to the internal evas_canvas_seat_key_lock_on() implementation.
 * For comprehensive documentation, please see the evas_canvas_eo.legacy.h header file.
 */
EVAS_API void
evas_seat_key_lock_on(Evas_Canvas *obj, const char *keyname, Evas_Device *seat)
{
   evas_canvas_seat_key_lock_on(obj, keyname, seat);
}

/**
 * @brief Legacy API wrapper: Turns off the lock key for a given seat.
 *
 * This function serves as a public interface to the internal evas_canvas_seat_key_lock_off() implementation.
 * For comprehensive documentation, please see the evas_canvas_eo.legacy.h header file.
 */
EVAS_API void
evas_seat_key_lock_off(Evas_Canvas *obj, const char *keyname, Evas_Device *seat)
{
   evas_canvas_seat_key_lock_off(obj, keyname, seat);
}

/**
 * @brief Legacy API wrapper: Adds a key to the list of modifier keys.
 *
 * This function serves as a public interface to the internal evas_canvas_key_modifier_add() implementation.
 * For comprehensive documentation, please see the evas_canvas_eo.legacy.h header file.
 */
EVAS_API void
evas_key_modifier_add(Evas_Canvas *obj, const char *keyname)
{
   evas_canvas_key_modifier_add(obj, keyname);
}

/**
 * @brief Legacy API wrapper: Turns off the modifier key for the default seat.
 *
 * This function serves as a public interface to the internal evas_canvas_key_modifier_off() implementation.
 * For comprehensive documentation, please see the evas_canvas_eo.legacy.h header file.
 */
EVAS_API void
evas_key_modifier_off(Evas_Canvas *obj, const char *keyname)
{
   evas_canvas_key_modifier_off(obj, keyname);
}

/**
 * @brief Legacy API wrapper: Render the Evas canvas asynchronously.
 *
 * This function serves as a public interface to the internal evas_canvas_render_async() implementation.
 * For comprehensive documentation, please see the evas_canvas_eo.legacy.h header file.
 */
EVAS_API Eina_Bool
evas_render_async(Evas_Canvas *obj)
{
   return evas_canvas_render_async(obj);
}

/**
 * @brief Legacy API wrapper: Inform Evas that it lost focus from the default seat.
 *
 * This function serves as a public interface to the internal evas_canvas_focus_out() implementation.
 * For comprehensive documentation, please see the evas_canvas_eo.legacy.h header file.
 */
EVAS_API void
evas_focus_out(Evas_Canvas *obj)
{
   evas_canvas_focus_out(obj);
}

/**
 * @brief Legacy API wrapper: Update canvas internal objects without immediate rendering.
 *
 * This function serves as a public interface to the internal evas_canvas_norender() implementation.
 * For comprehensive documentation, please see the evas_canvas_eo.legacy.h header file.
 */
EVAS_API void
evas_norender(Evas_Canvas *obj)
{
   evas_canvas_norender(obj);
}

/**
 * @brief Legacy API wrapper: Pop the nochange flag down.
 *
 * This function serves as a public interface to the internal evas_canvas_nochange_pop() implementation.
 * For comprehensive documentation, please see the evas_canvas_eo.legacy.h header file.
 */
EVAS_API void
evas_nochange_pop(Evas_Canvas *obj)
{
   evas_canvas_nochange_pop(obj);
}

/**
 * @brief Legacy API wrapper: Turns off the lock key for the default seat.
 *
 * This function serves as a public interface to the internal evas_canvas_key_lock_off() implementation.
 * For comprehensive documentation, please see the evas_canvas_eo.legacy.h header file.
 */
EVAS_API void
evas_key_lock_off(Evas_Canvas *obj, const char *keyname)
{
   evas_canvas_key_lock_off(obj, keyname);
}

/**
 * @brief Legacy API wrapper: Push the nochange flag up.
 *
 * This function serves as a public interface to the internal evas_canvas_nochange_push() implementation.
 * For comprehensive documentation, please see the evas_canvas_eo.legacy.h header file.
 */
EVAS_API void
evas_nochange_push(Evas_Canvas *obj)
{
   evas_canvas_nochange_push(obj);
}

/**
 * @brief Legacy API wrapper: Force the Evas engine to flush its font cache.
 *
 * This function serves as a public interface to the internal evas_canvas_font_cache_flush() implementation.
 * For comprehensive documentation, please see the evas_canvas_eo.legacy.h header file.
 */
EVAS_API void
evas_font_cache_flush(Evas_Canvas *obj)
{
   evas_canvas_font_cache_flush(obj);
}

/**
 * @brief Legacy API wrapper: Turns on the modifier key for the default seat.
 *
 * This function serves as a public interface to the internal evas_canvas_key_modifier_on() implementation.
 * For comprehensive documentation, please see the evas_canvas_eo.legacy.h header file.
 */
EVAS_API void
evas_key_modifier_on(Evas_Canvas *obj, const char *keyname)
{
   evas_canvas_key_modifier_on(obj, keyname);
}

/**
 * @brief Legacy API wrapper: Turns on the modifier key for a given seat.
 *
 * This function serves as a public interface to the internal evas_canvas_seat_key_modifier_on() implementation.
 * For comprehensive documentation, please see the evas_canvas_eo.legacy.h header file.
 */
EVAS_API void
evas_seat_key_modifier_on(Evas_Canvas *obj, const char *keyname, Evas_Device *seat)
{
   evas_canvas_seat_key_modifier_on(obj, keyname, seat);
}

/**
 * @brief Legacy API wrapper: Turns off the modifier key for a given seat.
 *
 * This function serves as a public interface to the internal evas_canvas_seat_key_modifier_off() implementation.
 * For comprehensive documentation, please see the evas_canvas_eo.legacy.h header file.
 */
EVAS_API void
evas_seat_key_modifier_off(Evas_Canvas *obj, const char *keyname, Evas_Device *seat)
{
   evas_canvas_seat_key_modifier_off(obj, keyname, seat);
}

/**
 * @brief Legacy API wrapper: List available font descriptions.
 *
 * This function serves as a public interface to the internal evas_canvas_font_available_list() implementation.
 * For comprehensive documentation, please see the evas_canvas_eo.legacy.h header file.
 */
EVAS_API Eina_List *
evas_font_available_list(const Evas_Canvas *obj)
{
   return evas_canvas_font_available_list(obj);
}

/**
 * @brief Legacy API wrapper: Retrieves an object by its name.
 *
 * This function serves as a public interface to the internal evas_canvas_object_name_find() implementation.
 * For comprehensive documentation, please see the evas_canvas_eo.legacy.h header file.
 */
EVAS_API Efl_Canvas_Object *
evas_object_name_find(const Evas_Canvas *obj, const char *name)
{
   return evas_canvas_object_name_find(obj, name);
}

/**
 * @brief Legacy API wrapper: Appends a font path to the list of font paths.
 *
 * This function serves as a public interface to the internal evas_canvas_font_path_append() implementation.
 * For comprehensive documentation, please see the evas_canvas_eo.legacy.h header file.
 */
EVAS_API void
evas_font_path_append(Evas_Canvas *obj, const char *path)
{
   evas_canvas_font_path_append(obj, path);
}

/**
 * @brief Legacy API wrapper: Removes all font paths.
 *
 * This function serves as a public interface to the internal evas_canvas_font_path_clear() implementation.
 * For comprehensive documentation, please see the evas_canvas_eo.legacy.h header file.
 */
EVAS_API void
evas_font_path_clear(Evas_Canvas *obj)
{
   evas_canvas_font_path_clear(obj);
}

/**
 * @brief Legacy API wrapper: Removes a key from the list of lock keys.
 *
 * This function serves as a public interface to the internal evas_canvas_key_lock_del() implementation.
 * For comprehensive documentation, please see the evas_canvas_eo.legacy.h header file.
 */
EVAS_API void
evas_key_lock_del(Evas_Canvas *obj, const char *keyname)
{
   evas_canvas_key_lock_del(obj, keyname);
}

/**
 * @brief Legacy API wrapper: Add a damage rectangle to the canvas.
 *
 * This function serves as a public interface to the internal evas_canvas_damage_rectangle_add() implementation.
 * For comprehensive documentation, please see the evas_canvas_eo.legacy.h header file.
 */
EVAS_API void
evas_damage_rectangle_add(Evas_Canvas *obj, int x, int y, int w, int h)
{
   evas_canvas_damage_rectangle_add(obj, x, y, w, h);
}

/**
 * @brief Legacy API wrapper: Sync Evas canvas.
 *
 * This function serves as a public interface to the internal evas_canvas_sync() implementation.
 * For comprehensive documentation, please see the evas_canvas_eo.legacy.h header file.
 */
EVAS_API void
evas_sync(Evas_Canvas *obj)
{
   evas_canvas_sync(obj);
}

/**
 * @brief Legacy API wrapper: Retrieves the list of font paths.
 *
 * This function serves as a public interface to the internal evas_canvas_font_path_list() implementation.
 * For comprehensive documentation, please see the evas_canvas_eo.legacy.h header file.
 */
EVAS_API const Eina_List *
evas_font_path_list(const Evas_Canvas *obj)
{
   return evas_canvas_font_path_list(obj);
}

/**
 * @brief Legacy API wrapper: Reload the image cache.
 *
 * This function serves as a public interface to the internal evas_canvas_image_cache_reload() implementation.
 * For comprehensive documentation, please see the evas_canvas_eo.legacy.h header file.
 */
EVAS_API void
evas_image_cache_reload(Evas_Canvas *obj)
{
   evas_canvas_image_cache_reload(obj);
}

/**
 * @brief Legacy API wrapper: Convert a canvas X coordinate to screen coordinates.
 *
 * This function serves as a public interface to the internal evas_canvas_coord_world_x_to_screen() implementation.
 * For comprehensive documentation, please see the evas_canvas_eo.legacy.h header file.
 */
EVAS_API int
evas_coord_world_x_to_screen(const Evas_Canvas *obj, int x)
{
   return evas_canvas_coord_world_x_to_screen(obj, x);
}

/**
 * @brief Legacy API wrapper: Force immediate rendering and get update rectangles.
 *
 * This function serves as a public interface to the internal evas_canvas_render_updates() implementation.
 * For comprehensive documentation, please see the evas_canvas_eo.legacy.h header file.
 */
EVAS_API Eina_List *
evas_render_updates(Evas_Canvas *obj)
{
   return evas_canvas_render_updates(obj);
}

/**
 * @brief Legacy API wrapper: Flush the image cache.
 *
 * This function serves as a public interface to the internal evas_canvas_image_cache_flush() implementation.
 * For comprehensive documentation, please see the evas_canvas_eo.legacy.h header file.
 */
EVAS_API void
evas_image_cache_flush(Evas_Canvas *obj)
{
   evas_canvas_image_cache_flush(obj);
}

/**
 * @brief Legacy API wrapper: Convert a screen Y coordinate to canvas coordinates.
 *
 * This function serves as a public interface to the internal evas_canvas_coord_screen_y_to_world() implementation.
 * For comprehensive documentation, please see the evas_canvas_eo.legacy.h header file.
 */
EVAS_API int
evas_coord_screen_y_to_world(const Evas_Canvas *obj, int y)
{
   return evas_canvas_coord_screen_y_to_world(obj, y);
}

/**
 * @brief Legacy API wrapper: Removes a key from the list of modifier keys.
 *
 * This function serves as a public interface to the internal evas_canvas_key_modifier_del() implementation.
 * For comprehensive documentation, please see the evas_canvas_eo.legacy.h header file.
 */
EVAS_API void
evas_key_modifier_del(Evas_Canvas *obj, const char *keyname)
{
   evas_canvas_key_modifier_del(obj, keyname);
}

/**
 * @brief Legacy API wrapper: Inform Evas that it gained focus for the default seat.
 *
 * This function serves as a public interface to the internal evas_canvas_focus_in() implementation.
 * For comprehensive documentation, please see the evas_canvas_eo.legacy.h header file.
 */
EVAS_API void
evas_focus_in(Evas_Canvas *obj)
{
   evas_canvas_focus_in(obj);
}

/**
 * @brief Legacy API wrapper: Add an obscured rectangle to the canvas.
 *
 * This function serves as a public interface to the internal evas_canvas_obscured_rectangle_add() implementation.
 * For comprehensive documentation, please see the evas_canvas_eo.legacy.h header file.
 */
EVAS_API void
evas_obscured_rectangle_add(Evas_Canvas *obj, int x, int y, int w, int h)
{
   evas_canvas_obscured_rectangle_add(obj, x, y, w, h);
}

/**
 * @brief Legacy API wrapper: Make the canvas discard cached rendering data.
 *
 * This function serves as a public interface to the internal evas_canvas_render_dump() implementation.
 * For comprehensive documentation, please see the evas_canvas_eo.legacy.h header file.
 */
EVAS_API void
evas_render_dump(Evas_Canvas *obj)
{
   evas_canvas_render_dump(obj);
}

/**
 * @brief Legacy API wrapper: Force rendering of the canvas.
 *
 * This function serves as a public interface to the internal evas_canvas_render() implementation.
 * For comprehensive documentation, please see the evas_canvas_eo.legacy.h header file.
 */
EVAS_API void
evas_render(Evas_Canvas *obj)
{
   evas_canvas_render(obj);
}

/**
 * @brief Legacy API wrapper: Prepends a font path to the list of font paths.
 *
 * This function serves as a public interface to the internal evas_canvas_font_path_prepend() implementation.
 * For comprehensive documentation, please see the evas_canvas_eo.legacy.h header file.
 */
EVAS_API void
evas_font_path_prepend(Evas_Canvas *obj, const char *path)
{
   evas_canvas_font_path_prepend(obj, path);
}

/**
 * @brief Legacy API wrapper: Remove all obscured regions from the canvas.
 *
 * This function serves as a public interface to the internal evas_canvas_obscured_clear() implementation.
 * For comprehensive documentation, please see the evas_canvas_eo.legacy.h header file.
 */
EVAS_API void
evas_obscured_clear(Evas_Canvas *obj)
{
   evas_canvas_obscured_clear(obj);
}

/**
 * @brief Legacy API wrapper: Convert a screen X coordinate to canvas coordinates.
 *
 * This function serves as a public interface to the internal evas_canvas_coord_screen_x_to_world() implementation.
 * For comprehensive documentation, please see the evas_canvas_eo.legacy.h header file.
 */
EVAS_API int
evas_coord_screen_x_to_world(const Evas_Canvas *obj, int x)
{
   return evas_canvas_coord_screen_x_to_world(obj, x);
}

/**
 * @brief Legacy API wrapper: Adds a key to the list of lock keys.
 *
 * This function serves as a public interface to the internal evas_canvas_key_lock_add() implementation.
 * For comprehensive documentation, please see the evas_canvas_eo.legacy.h header file.
 */
EVAS_API void
evas_key_lock_add(Evas_Canvas *obj, const char *keyname)
{
   evas_canvas_key_lock_add(obj, keyname);
}

/**
 * @brief Legacy API wrapper: Make the canvas discard cached data used for idle rendering.
 *
 * This function serves as a public interface to the internal evas_canvas_render_idle_flush() implementation.
 * For comprehensive documentation, please see the evas_canvas_eo.legacy.h header file.
 */
EVAS_API void
evas_render_idle_flush(Evas_Canvas *obj)
{
   evas_canvas_render_idle_flush(obj);
}

/**
 * @brief Legacy API wrapper: Return the default device of a given type.
 *
 * This function serves as a public interface to the internal evas_canvas_default_device_get() implementation.
 * For comprehensive documentation, please see the evas_canvas_eo.legacy.h header file.
 */
EVAS_API Evas_Device *
evas_default_device_get(const Evas_Canvas *obj, Evas_Device_Class type)
{
   return evas_canvas_default_device_get(obj, type);
}

/**
 * @brief Legacy API wrapper: Convert a canvas Y coordinate to screen coordinates.
 *
 * This function serves as a public interface to the internal evas_canvas_coord_world_y_to_screen() implementation.
 * For comprehensive documentation, please see the evas_canvas_eo.legacy.h header file.
 */
EVAS_API int
evas_coord_world_y_to_screen(const Evas_Canvas *obj, int y)
{
   return evas_canvas_coord_world_y_to_screen(obj, y);
}
