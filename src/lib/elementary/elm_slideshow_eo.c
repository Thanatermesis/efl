EWAPI const Efl_Event_Description _ELM_SLIDESHOW_EVENT_CHANGED =
   EFL_EVENT_DESCRIPTION("changed");
EWAPI const Efl_Event_Description _ELM_SLIDESHOW_EVENT_TRANSITION_END =
   EFL_EVENT_DESCRIPTION("transition,end");

/**
 * @internal
 * @brief Sets the number of items to cache after the current item.
 *
 * This function is the low-level C implementation for the Eolian property
 * @ref elm_obj_slideshow_cache_after_set. It is not meant to be called directly
 * by application code.
 *
 * @param obj The Evas object (Eo) representing the slideshow.
 * @param pd Pointer to the private data of the slideshow object.
 * @param count The number of items to cache.
 */
void _elm_slideshow_cache_after_set(Eo *obj, Elm_Slideshow_Data *pd, int count);

/**
 * @internal
 * @brief Eolian reflection function for the "cache_after" property setter.
 *
 * This function is invoked by the Eolian reflection system when the
 * "cache_after" property is set using generic property access mechanisms.
 * It converts an @c Eina_Value (which should contain an integer) to the
 * appropriate type and calls the actual setter implementation
 * (_elm_slideshow_cache_after_set).
 *
 * @param obj The Eolian object.
 * @param val The @c Eina_Value containing the integer value for the cache count.
 * @return @c EINA_ERROR_NO_ERROR on success, or an Eina_Error code if conversion fails.
 */
static Eina_Error
__eolian_elm_slideshow_cache_after_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   int cval;
   if (!eina_value_int_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_slideshow_cache_after_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_slideshow_cache_after_set, EFL_FUNC_CALL(count), int count);

/**
 * @internal
 * @brief Gets the number of items to cache after the current item.
 *
 * This function is the low-level C implementation for the Eolian property
 * @ref elm_obj_slideshow_cache_after_get. It is not meant to be called directly
 * by application code.
 *
 * @param obj The Evas object (Eo) representing the slideshow.
 * @param pd Pointer to the private data of the slideshow object.
 * @return The number of items cached after the current one.
 */
int _elm_slideshow_cache_after_get(const Eo *obj, Elm_Slideshow_Data *pd);

/**
 * @internal
 * @brief Eolian reflection function for the "cache_after" property getter.
 *
 * This function is invoked by the Eolian reflection system when the
 * "cache_after" property is accessed using generic property access mechanisms.
 * It calls the actual getter implementation (_elm_slideshow_cache_after_get)
 * and wraps the returned integer value in an @c Eina_Value.
 *
 * @param obj The Eolian object.
 * @return An @c Eina_Value containing the integer value of the cache count.
 *         The caller is responsible for flushing this @c Eina_Value.
 */
static Eina_Value
__eolian_elm_slideshow_cache_after_get_reflect(const Eo *obj)
{
   int val = elm_obj_slideshow_cache_after_get(obj);
   return eina_value_int_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_slideshow_cache_after_get, int, 0);

/**
 * @internal
 * @brief Sets the number of items to cache before the current item.
 *
 * This function is the low-level C implementation for the Eolian property
 * @ref elm_obj_slideshow_cache_before_set. It is not meant to be called directly
 * by application code.
 *
 * @param obj The Evas object (Eo) representing the slideshow.
 * @param pd Pointer to the private data of the slideshow object.
 * @param count The number of items to cache.
 */
void _elm_slideshow_cache_before_set(Eo *obj, Elm_Slideshow_Data *pd, int count);

/**
 * @internal
 * @brief Eolian reflection function for the "cache_before" property setter.
 *
 * This function is invoked by the Eolian reflection system when the
 * "cache_before" property is set. It converts an @c Eina_Value (integer)
 * to the appropriate type and calls _elm_slideshow_cache_before_set.
 *
 * @param obj The Eolian object.
 * @param val The @c Eina_Value containing the integer value for the cache count.
 * @return @c EINA_ERROR_NO_ERROR on success, or an Eina_Error code on failure.
 */
static Eina_Error
__eolian_elm_slideshow_cache_before_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   int cval;
   if (!eina_value_int_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_slideshow_cache_before_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_slideshow_cache_before_set, EFL_FUNC_CALL(count), int count);

/**
 * @internal
 * @brief Gets the number of items to cache before the current item.
 *
 * This function is the low-level C implementation for the Eolian property
 * @ref elm_obj_slideshow_cache_before_get. It is not meant to be called directly
 * by application code.
 *
 * @param obj The Evas object (Eo) representing the slideshow.
 * @param pd Pointer to the private data of the slideshow object.
 * @return The number of items cached before the current one.
 */
int _elm_slideshow_cache_before_get(const Eo *obj, Elm_Slideshow_Data *pd);

/**
 * @internal
 * @brief Eolian reflection function for the "cache_before" property getter.
 *
 * This function is invoked by the Eolian reflection system when the
 * "cache_before" property is accessed. It calls _elm_slideshow_cache_before_get
 * and wraps the returned integer in an @c Eina_Value.
 *
 * @param obj The Eolian object.
 * @return An @c Eina_Value containing the integer value of the cache count.
 */
static Eina_Value
__eolian_elm_slideshow_cache_before_get_reflect(const Eo *obj)
{
   int val = elm_obj_slideshow_cache_before_get(obj);
   return eina_value_int_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_slideshow_cache_before_get, int, 0);

/**
 * @internal
 * @brief Sets the current slide layout for the slideshow.
 *
 * This function is the low-level C implementation for the Eolian property
 * @ref elm_obj_slideshow_layout_set. It is not meant to be called directly
 * by application code.
 *
 * @param obj The Evas object (Eo) representing the slideshow.
 * @param pd Pointer to the private data of the slideshow object.
 * @param layout The name of the layout to set.
 */
void _elm_slideshow_layout_set(Eo *obj, Elm_Slideshow_Data *pd, const char *layout);

/**
 * @internal
 * @brief Eolian reflection function for the "layout" property setter.
 *
 * This function is invoked by the Eolian reflection system when the
 * "layout" property is set. It converts an @c Eina_Value (string)
 * to a C string and calls _elm_slideshow_layout_set.
 *
 * @param obj The Eolian object.
 * @param val The @c Eina_Value containing the string for the layout name.
 * @return @c EINA_ERROR_NO_ERROR on success, or an Eina_Error code on failure.
 */
static Eina_Error
__eolian_elm_slideshow_layout_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   const char *cval;
   if (!eina_value_string_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_slideshow_layout_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_slideshow_layout_set, EFL_FUNC_CALL(layout), const char *layout);

/**
 * @internal
 * @brief Gets the current slide layout of the slideshow.
 *
 * This function is the low-level C implementation for the Eolian property
 * @ref elm_obj_slideshow_layout_get. It is not meant to be called directly
 * by application code.
 *
 * @param obj The Evas object (Eo) representing the slideshow.
 * @param pd Pointer to the private data of the slideshow object.
 * @return The name of the current layout.
 */
const char *_elm_slideshow_layout_get(const Eo *obj, Elm_Slideshow_Data *pd);

/**
 * @internal
 * @brief Eolian reflection function for the "layout" property getter.
 *
 * This function is invoked by the Eolian reflection system when the
 * "layout" property is accessed. It calls _elm_slideshow_layout_get
 * and wraps the returned string in an @c Eina_Value.
 *
 * @param obj The Eolian object.
 * @return An @c Eina_Value containing the string name of the layout.
 */
static Eina_Value
__eolian_elm_slideshow_layout_get_reflect(const Eo *obj)
{
   const char *val = elm_obj_slideshow_layout_get(obj);
   return eina_value_string_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_slideshow_layout_get, const char *, NULL);

/**
 * @internal
 * @brief Sets the current slide transition effect for the slideshow.
 *
 * This function is the low-level C implementation for the Eolian property
 * @ref elm_obj_slideshow_transition_set. It is not meant to be called directly
 * by application code.
 *
 * @param obj The Evas object (Eo) representing the slideshow.
 * @param pd Pointer to the private data of the slideshow object.
 * @param transition The name of the transition to set.
 */
void _elm_slideshow_transition_set(Eo *obj, Elm_Slideshow_Data *pd, const char *transition);

/**
 * @internal
 * @brief Eolian reflection function for the "transition" property setter.
 *
 * This function is invoked by the Eolian reflection system when the
 * "transition" property is set. It converts an @c Eina_Value (string)
 * to a C string and calls _elm_slideshow_transition_set.
 *
 * @param obj The Eolian object.
 * @param val The @c Eina_Value containing the string for the transition name.
 * @return @c EINA_ERROR_NO_ERROR on success, or an Eina_Error code on failure.
 */
static Eina_Error
__eolian_elm_slideshow_transition_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   const char *cval;
   if (!eina_value_string_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_slideshow_transition_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_slideshow_transition_set, EFL_FUNC_CALL(transition), const char *transition);

/**
 * @internal
 * @brief Gets the current slide transition effect of the slideshow.
 *
 * This function is the low-level C implementation for the Eolian property
 * @ref elm_obj_slideshow_transition_get. It is not meant to be called directly
 * by application code.
 *
 * @param obj The Evas object (Eo) representing the slideshow.
 * @param pd Pointer to the private data of the slideshow object.
 * @return The name of the current transition.
 */
const char *_elm_slideshow_transition_get(const Eo *obj, Elm_Slideshow_Data *pd);

/**
 * @internal
 * @brief Eolian reflection function for the "transition" property getter.
 *
 * This function is invoked by the Eolian reflection system when the
 * "transition" property is accessed. It calls _elm_slideshow_transition_get
 * and wraps the returned string in an @c Eina_Value.
 *
 * @param obj The Eolian object.
 * @return An @c Eina_Value containing the string name of the transition.
 */
static Eina_Value
__eolian_elm_slideshow_transition_get_reflect(const Eo *obj)
{
   const char *val = elm_obj_slideshow_transition_get(obj);
   return eina_value_string_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_slideshow_transition_get, const char *, NULL);

/**
 * @internal
 * @brief Sets whether the slideshow items should loop.
 *
 * This function is the low-level C implementation for the Eolian property
 * @ref elm_obj_slideshow_items_loop_set. It is not meant to be called directly
 * by application code.
 *
 * @param obj The Evas object (Eo) representing the slideshow.
 * @param pd Pointer to the private data of the slideshow object.
 * @param loop @c EINA_TRUE to enable looping, @c EINA_FALSE otherwise.
 */
void _elm_slideshow_items_loop_set(Eo *obj, Elm_Slideshow_Data *pd, Eina_Bool loop);

/**
 * @internal
 * @brief Eolian reflection function for the "items_loop" property setter.
 *
 * This function is invoked by the Eolian reflection system when the
 * "items_loop" property is set. It converts an @c Eina_Value (boolean)
 * to an @c Eina_Bool and calls _elm_slideshow_items_loop_set.
 *
 * @param obj The Eolian object.
 * @param val The @c Eina_Value containing the boolean value for loop mode.
 * @return @c EINA_ERROR_NO_ERROR on success, or an Eina_Error code on failure.
 */
static Eina_Error
__eolian_elm_slideshow_items_loop_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   Eina_Bool cval;
   if (!eina_value_bool_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_slideshow_items_loop_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_slideshow_items_loop_set, EFL_FUNC_CALL(loop), Eina_Bool loop);

/**
 * @internal
 * @brief Gets whether the slideshow items are set to loop.
 *
 * This function is the low-level C implementation for the Eolian property
 * @ref elm_obj_slideshow_items_loop_get. It is not meant to be called directly
 * by application code.
 *
 * @param obj The Evas object (Eo) representing the slideshow.
 * @param pd Pointer to the private data of the slideshow object.
 * @return @c EINA_TRUE if looping is enabled, @c EINA_FALSE otherwise.
 */
Eina_Bool _elm_slideshow_items_loop_get(const Eo *obj, Elm_Slideshow_Data *pd);

/**
 * @internal
 * @brief Eolian reflection function for the "items_loop" property getter.
 *
 * This function is invoked by the Eolian reflection system when the
 * "items_loop" property is accessed. It calls _elm_slideshow_items_loop_get
 * and wraps the returned boolean in an @c Eina_Value.
 *
 * @param obj The Eolian object.
 * @return An @c Eina_Value containing the boolean loop status.
 */
static Eina_Value
__eolian_elm_slideshow_items_loop_get_reflect(const Eo *obj)
{
   Eina_Bool val = elm_obj_slideshow_items_loop_get(obj);
   return eina_value_bool_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_slideshow_items_loop_get, Eina_Bool, 0);

/**
 * @internal
 * @brief Sets the timeout for slideshow transitions.
 *
 * This function is the low-level C implementation for the Eolian property
 * @ref elm_obj_slideshow_timeout_set. It is not meant to be called directly
 * by application code.
 *
 * @param obj The Evas object (Eo) representing the slideshow.
 * @param pd Pointer to the private data of the slideshow object.
 * @param timeout The transition timeout in seconds.
 */
void _elm_slideshow_timeout_set(Eo *obj, Elm_Slideshow_Data *pd, double timeout);

/**
 * @internal
 * @brief Eolian reflection function for the "timeout" property setter.
 *
 * This function is invoked by the Eolian reflection system when the
 * "timeout" property is set. It converts an @c Eina_Value (double)
 * to a double and calls _elm_slideshow_timeout_set.
 *
 * @param obj The Eolian object.
 * @param val The @c Eina_Value containing the double value for the timeout.
 * @return @c EINA_ERROR_NO_ERROR on success, or an Eina_Error code on failure.
 */
static Eina_Error
__eolian_elm_slideshow_timeout_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   double cval;
   if (!eina_value_double_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_slideshow_timeout_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_slideshow_timeout_set, EFL_FUNC_CALL(timeout), double timeout);

/**
 * @internal
 * @brief Gets the timeout for slideshow transitions.
 *
 * This function is the low-level C implementation for the Eolian property
 * @ref elm_obj_slideshow_timeout_get. It is not meant to be called directly
 * by application code.
 *
 * @param obj The Evas object (Eo) representing the slideshow.
 * @param pd Pointer to the private data of the slideshow object.
 * @return The transition timeout in seconds.
 */
double _elm_slideshow_timeout_get(const Eo *obj, Elm_Slideshow_Data *pd);

/**
 * @internal
 * @brief Eolian reflection function for the "timeout" property getter.
 *
 * This function is invoked by the Eolian reflection system when the
 * "timeout" property is accessed. It calls _elm_slideshow_timeout_get
 * and wraps the returned double in an @c Eina_Value.
 *
 * @param obj The Eolian object.
 * @return An @c Eina_Value containing the double timeout value.
 */
static Eina_Value
__eolian_elm_slideshow_timeout_get_reflect(const Eo *obj)
{
   double val = elm_obj_slideshow_timeout_get(obj);
   return eina_value_double_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_slideshow_timeout_get, double, 0);

/**
 * @internal
 * @brief Gets the list of items in the slideshow.
 * This function is the C implementation for @ref elm_obj_slideshow_items_get.
 * @param obj The Eolian object.
 * @param pd The private data of the slideshow.
 * @return A const Eina_List of #Elm_Widget_Item objects.
 */
const Eina_List *_elm_slideshow_items_get(const Eo *obj, Elm_Slideshow_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_slideshow_items_get, const Eina_List *, NULL);

/**
 * @internal
 * @brief Gets the list of available transitions for the slideshow.
 * This function is the C implementation for @ref elm_obj_slideshow_transitions_get.
 * @param obj The Eolian object.
 * @param pd The private data of the slideshow.
 * @return A const Eina_List of stringshared transition names.
 */
const Eina_List *_elm_slideshow_transitions_get(const Eo *obj, Elm_Slideshow_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_slideshow_transitions_get, const Eina_List *, NULL);

/**
 * @internal
 * @brief Gets the number of items in the slideshow.
 * This function is the C implementation for @ref elm_obj_slideshow_count_get.
 * @param obj The Eolian object.
 * @param pd The private data of the slideshow.
 * @return The count of items.
 */
unsigned int _elm_slideshow_count_get(const Eo *obj, Elm_Slideshow_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_slideshow_count_get, unsigned int, 0);

/**
 * @internal
 * @brief Gets the currently displayed item in the slideshow.
 * This function is the C implementation for @ref elm_obj_slideshow_item_current_get.
 * @param obj The Eolian object.
 * @param pd The private data of the slideshow.
 * @return The current #Elm_Widget_Item or @c NULL.
 */
Elm_Widget_Item *_elm_slideshow_item_current_get(const Eo *obj, Elm_Slideshow_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_slideshow_item_current_get, Elm_Widget_Item *, NULL);

/**
 * @internal
 * @brief Gets the list of available layouts for the slideshow.
 * This function is the C implementation for @ref elm_obj_slideshow_layouts_get.
 * @param obj The Eolian object.
 * @param pd The private data of the slideshow.
 * @return A const Eina_List of stringshared layout names.
 */
const Eina_List *_elm_slideshow_layouts_get(const Eo *obj, Elm_Slideshow_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_slideshow_layouts_get, const Eina_List *, NULL);

/**
 * @internal
 * @brief Navigates to the previous item in the slideshow.
 * This function is the C implementation for @ref elm_obj_slideshow_previous.
 * @param obj The Eolian object.
 * @param pd The private data of the slideshow.
 */
void _elm_slideshow_previous(Eo *obj, Elm_Slideshow_Data *pd);

EOAPI EFL_VOID_FUNC_BODY(elm_obj_slideshow_previous);

/**
 * @internal
 * @brief Gets the nth item in the slideshow.
 * This function is the C implementation for @ref elm_obj_slideshow_item_nth_get.
 * @param obj The Eolian object.
 * @param pd The private data of the slideshow.
 * @param nth The index of the item to retrieve.
 * @return The #Elm_Widget_Item at the specified index or @c NULL.
 */
Elm_Widget_Item *_elm_slideshow_item_nth_get(const Eo *obj, Elm_Slideshow_Data *pd, unsigned int nth);

EOAPI EFL_FUNC_BODYV_CONST(elm_obj_slideshow_item_nth_get, Elm_Widget_Item *, NULL, EFL_FUNC_CALL(nth), unsigned int nth);

/**
 * @internal
 * @brief Navigates to the next item in the slideshow.
 * This function is the C implementation for @ref elm_obj_slideshow_next.
 * @param obj The Eolian object.
 * @param pd The private data of the slideshow.
 */
void _elm_slideshow_next(Eo *obj, Elm_Slideshow_Data *pd);

EOAPI EFL_VOID_FUNC_BODY(elm_obj_slideshow_next);

/**
 * @internal
 * @brief Clears all items from the slideshow.
 * This function is the C implementation for @ref elm_obj_slideshow_clear.
 * @param obj The Eolian object.
 * @param pd The private data of the slideshow.
 */
void _elm_slideshow_clear(Eo *obj, Elm_Slideshow_Data *pd);

EOAPI EFL_VOID_FUNC_BODY(elm_obj_slideshow_clear);

/**
 * @internal
 * @brief Adds a new item to the slideshow.
 * This function is the C implementation for @ref elm_obj_slideshow_item_add.
 * @param obj The Eolian object.
 * @param pd The private data of the slideshow.
 * @param itc The item class for the new item.
 * @param data The data associated with the new item.
 * @return The newly added #Elm_Widget_Item or @c NULL on failure.
 */
Elm_Widget_Item *_elm_slideshow_item_add(Eo *obj, Elm_Slideshow_Data *pd, const Elm_Slideshow_Item_Class *itc, const void *data);

EOAPI EFL_FUNC_BODYV(elm_obj_slideshow_item_add, Elm_Widget_Item *, NULL, EFL_FUNC_CALL(itc, data), const Elm_Slideshow_Item_Class *itc, const void *data);

/**
 * @internal
 * @brief Inserts a new item into the slideshow in sorted order.
 * This function is the C implementation for @ref elm_obj_slideshow_item_sorted_insert.
 * @param obj The Eolian object.
 * @param pd The private data of the slideshow.
 * @param itc The item class for the new item.
 * @param data The data associated with the new item.
 * @param func The comparison function used for sorting.
 * @return The newly added #Elm_Widget_Item or @c NULL on failure.
 */
Elm_Widget_Item *_elm_slideshow_item_sorted_insert(Eo *obj, Elm_Slideshow_Data *pd, const Elm_Slideshow_Item_Class *itc, const void *data, Eina_Compare_Cb func);

EOAPI EFL_FUNC_BODYV(elm_obj_slideshow_item_sorted_insert, Elm_Widget_Item *, NULL, EFL_FUNC_CALL(itc, data, func), const Elm_Slideshow_Item_Class *itc, const void *data, Eina_Compare_Cb func);

/**
 * @internal
 * @brief Implements the Efl.Object.constructor interface.
 *
 * This function is called when an Elm_Slideshow object is constructed.
 * It performs initial setup for the slideshow widget.
 *
 * @param obj The Eolian object being constructed.
 * @param pd Pointer to the private data of the slideshow object.
 * @return The constructed Eolian object, or @c NULL on failure.
 */
Efl_Object *_elm_slideshow_efl_object_constructor(Eo *obj, Elm_Slideshow_Data *pd);

/**
 * @internal
 * @brief Implements the Efl.Ui.Widget.widget_input_event_handler interface.
 *
 * This function handles input events (like key presses, mouse events) for the
 * slideshow widget. It determines if the event was consumed by the widget.
 *
 * @param obj The Eolian object (slideshow widget).
 * @param pd Pointer to the private data of the slideshow object.
 * @param eo_event The Efl_Event structure containing event details.
 * @param source The canvas object that sourced the event.
 * @return @c EINA_TRUE if the event was handled and consumed, @c EINA_FALSE otherwise.
 */
Eina_Bool _elm_slideshow_efl_ui_widget_widget_input_event_handler(Eo *obj, Elm_Slideshow_Data *pd, const Efl_Event *eo_event, Efl_Canvas_Object *source);

/**
 * @internal
 * @brief Implements the Efl.Access.Widget.Action elm_actions_get interface.
 *
 * This function provides a list of Elementary actions that can be performed
 * on the slideshow widget, primarily for accessibility purposes.
 *
 * @param obj The Eolian object (slideshow widget).
 * @param pd Pointer to the private data of the slideshow object.
 * @return A pointer to an array of #Efl_Access_Action_Data, terminated by
 *         an entry with a @c NULL name. Returns @c NULL if no actions are available.
 */
const Efl_Access_Action_Data *_elm_slideshow_efl_access_widget_action_elm_actions_get(const Eo *obj, Elm_Slideshow_Data *pd);

/**
 * @internal
 * @brief Initializes the Elm_Slideshow Efl_Class.
 *
 * This function is called once when the Elm_Slideshow class is being set up.
 * It defines the Eolian operations (ops) and property reflection operations (ropsp)
 * for the class. These operations map the Eolian API functions and properties
 * to their C implementations.
 *
 * @param klass The Efl_Class to initialize.
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_slideshow_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_SLIDESHOW_EXTRA_OPS
#define ELM_SLIDESHOW_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(elm_obj_slideshow_cache_after_set, _elm_slideshow_cache_after_set),
      EFL_OBJECT_OP_FUNC(elm_obj_slideshow_cache_after_get, _elm_slideshow_cache_after_get),
      EFL_OBJECT_OP_FUNC(elm_obj_slideshow_cache_before_set, _elm_slideshow_cache_before_set),
      EFL_OBJECT_OP_FUNC(elm_obj_slideshow_cache_before_get, _elm_slideshow_cache_before_get),
      EFL_OBJECT_OP_FUNC(elm_obj_slideshow_layout_set, _elm_slideshow_layout_set),
      EFL_OBJECT_OP_FUNC(elm_obj_slideshow_layout_get, _elm_slideshow_layout_get),
      EFL_OBJECT_OP_FUNC(elm_obj_slideshow_transition_set, _elm_slideshow_transition_set),
      EFL_OBJECT_OP_FUNC(elm_obj_slideshow_transition_get, _elm_slideshow_transition_get),
      EFL_OBJECT_OP_FUNC(elm_obj_slideshow_items_loop_set, _elm_slideshow_items_loop_set),
      EFL_OBJECT_OP_FUNC(elm_obj_slideshow_items_loop_get, _elm_slideshow_items_loop_get),
      EFL_OBJECT_OP_FUNC(elm_obj_slideshow_timeout_set, _elm_slideshow_timeout_set),
      EFL_OBJECT_OP_FUNC(elm_obj_slideshow_timeout_get, _elm_slideshow_timeout_get),
      EFL_OBJECT_OP_FUNC(elm_obj_slideshow_items_get, _elm_slideshow_items_get),
      EFL_OBJECT_OP_FUNC(elm_obj_slideshow_transitions_get, _elm_slideshow_transitions_get),
      EFL_OBJECT_OP_FUNC(elm_obj_slideshow_count_get, _elm_slideshow_count_get),
      EFL_OBJECT_OP_FUNC(elm_obj_slideshow_item_current_get, _elm_slideshow_item_current_get),
      EFL_OBJECT_OP_FUNC(elm_obj_slideshow_layouts_get, _elm_slideshow_layouts_get),
      EFL_OBJECT_OP_FUNC(elm_obj_slideshow_previous, _elm_slideshow_previous),
      EFL_OBJECT_OP_FUNC(elm_obj_slideshow_item_nth_get, _elm_slideshow_item_nth_get),
      EFL_OBJECT_OP_FUNC(elm_obj_slideshow_next, _elm_slideshow_next),
      EFL_OBJECT_OP_FUNC(elm_obj_slideshow_clear, _elm_slideshow_clear),
      EFL_OBJECT_OP_FUNC(elm_obj_slideshow_item_add, _elm_slideshow_item_add),
      EFL_OBJECT_OP_FUNC(elm_obj_slideshow_item_sorted_insert, _elm_slideshow_item_sorted_insert),
      EFL_OBJECT_OP_FUNC(efl_constructor, _elm_slideshow_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_input_event_handler, _elm_slideshow_efl_ui_widget_widget_input_event_handler),
      EFL_OBJECT_OP_FUNC(efl_access_widget_action_elm_actions_get, _elm_slideshow_efl_access_widget_action_elm_actions_get),
      ELM_SLIDESHOW_EXTRA_OPS
   );
   opsp = &ops;

   static const Efl_Object_Property_Reflection refl_table[] = {
      {"cache_after", __eolian_elm_slideshow_cache_after_set_reflect, __eolian_elm_slideshow_cache_after_get_reflect},
      {"cache_before", __eolian_elm_slideshow_cache_before_set_reflect, __eolian_elm_slideshow_cache_before_get_reflect},
      {"layout", __eolian_elm_slideshow_layout_set_reflect, __eolian_elm_slideshow_layout_get_reflect},
      {"transition", __eolian_elm_slideshow_transition_set_reflect, __eolian_elm_slideshow_transition_get_reflect},
      {"items_loop", __eolian_elm_slideshow_items_loop_set_reflect, __eolian_elm_slideshow_items_loop_get_reflect},
      {"timeout", __eolian_elm_slideshow_timeout_set_reflect, __eolian_elm_slideshow_timeout_get_reflect},
   };
   static const Efl_Object_Property_Reflection_Ops rops = {
      refl_table, EINA_C_ARRAY_LENGTH(refl_table)
   };
   ropsp = &rops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

static const Efl_Class_Description _elm_slideshow_class_desc = {
   EO_VERSION,
   "Elm.Slideshow",
   EFL_CLASS_TYPE_REGULAR,
   sizeof(Elm_Slideshow_Data),
   _elm_slideshow_class_initializer,
   _elm_slideshow_class_constructor,
   NULL
};

EFL_DEFINE_CLASS(elm_slideshow_class_get, &_elm_slideshow_class_desc, EFL_UI_LAYOUT_BASE_CLASS, EFL_ACCESS_WIDGET_ACTION_MIXIN, ELM_LAYOUT_MIXIN, EFL_UI_LEGACY_INTERFACE, NULL);

#include "elm_slideshow_eo.legacy.c"
