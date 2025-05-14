/**
 * @brief Controls the size restriction state of an object item's tooltip.
 * @details This function sets whether a tooltip is allowed to expand beyond its
 * parent window's canvas. When size restrictions are disabled, the tooltip
 * will be limited only by the size of the display.
 * @param[in] obj The Elm_Widget_Item object.
 * @param[in] disable If @c EINA_TRUE, size restrictions are disabled;
 *                    if @c EINA_FALSE, they are enabled according to the parent window.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
EAPI Eina_Bool
elm_object_item_tooltip_window_mode_set(Elm_Widget_Item *obj, Eina_Bool disable)
{
   return elm_wdg_item_tooltip_window_mode_set(obj, disable);
}

/**
 * @brief Gets the size restriction state of an object item's tooltip.
 * @details This function returns whether a tooltip is allowed to expand beyond its
 * parent window's canvas. If @c EINA_TRUE, it means size restrictions are disabled,
 * and the tooltip is limited only by the display size.
 * @param[in] obj The Elm_Widget_Item object.
 * @return @c EINA_TRUE if size restrictions are disabled, @c EINA_FALSE otherwise (including on errors).
 */
EAPI Eina_Bool
elm_object_item_tooltip_window_mode_get(const Elm_Widget_Item *obj)
{
   return elm_wdg_item_tooltip_window_mode_get(obj);
}

/**
 * @brief Sets a different style for this item's tooltip.
 * @details Before setting a style, a tooltip should be defined using
 * @ref elm_object_item_tooltip_content_cb_set or
 * @ref elm_object_item_tooltip_text_set.
 * For more details, see the documentation for elm_object_tooltip_style_set().
 * @param[in] obj The Elm_Widget_Item object.
 * @param[in] style The theme style to set (e.g., "default", "transparent").
 *                  Passing @c NULL might reset to a default style.
 */
EAPI void
elm_object_item_tooltip_style_set(Elm_Widget_Item *obj, const char *style)
{
   elm_wdg_item_tooltip_style_set(obj, style);
}

/**
 * @brief Gets the tooltip style for this item.
 * @details This function returns the current style set for the item's tooltip.
 * If no custom style is set, it might return the default style.
 * @param[in] obj The Elm_Widget_Item object.
 * @return The theme style used for the tooltip (e.g., "default", "transparent"),
 *         or @c NULL if no style is explicitly set or on error.
 */
EAPI const char *
elm_object_item_tooltip_style_get(const Elm_Widget_Item *obj)
{
   return elm_wdg_item_tooltip_style_get(obj);
}

/**
 * @brief Sets the mouse pointer/cursor decoration for the item.
 * @details This function sets the type of mouse cursor to be shown when the
 * mouse pointer is over this specific item. Item cursors have precedence
 * over widget cursors. Calling this function twice for an item will
 * replace the previously set cursor.
 * @param[in] obj The Elm_Widget_Item object.
 * @param[in] cursor The name of the cursor type to set.
 */
EAPI void
elm_object_item_cursor_set(Elm_Widget_Item *obj, const char *cursor)
{
   elm_wdg_item_cursor_set(obj, cursor);
}

/**
 * @brief Gets the mouse pointer/cursor decoration for the item.
 * @details This function retrieves the name of the cursor type set for this item.
 * @param[in] obj The Elm_Widget_Item object.
 * @return The name of the cursor type, or @c NULL if no custom cursor is set or on error.
 */
EAPI const char *
elm_object_item_cursor_get(const Elm_Widget_Item *obj)
{
   return elm_wdg_item_cursor_get(obj);
}

/**
 * @brief Sets a style for the custom cursor of an item.
 * @details This function applies a specific style to a custom cursor previously
 * set on the item using @ref elm_object_item_cursor_set. This is useful for
 * themed cursors that support multiple styles.
 * @warning A custom cursor must be set via @ref elm_object_item_cursor_set
 *          before calling this function.
 * @param[in] obj The Elm_Widget_Item object.
 * @param[in] style The theme style to apply to the cursor (e.g., "default", "transparent").
 */
EAPI void
elm_object_item_cursor_style_set(Elm_Widget_Item *obj, const char *style)
{
   elm_wdg_item_cursor_style_set(obj, style);
}

/**
 * @brief Gets the style of the custom cursor for an item.
 * @details This function retrieves the style applied to the custom cursor of the item.
 * @warning This is relevant only if a custom cursor has been set with @ref elm_object_item_cursor_set.
 * @param[in] obj The Elm_Widget_Item object.
 * @return The theme style of the cursor, or @c NULL if no style is set or on error.
 */
EAPI const char *
elm_object_item_cursor_style_get(const Elm_Widget_Item *obj)
{
   return elm_wdg_item_cursor_style_get(obj);
}

/**
 * @brief Sets whether the item's custom cursor should be searched in its theme or only rely on the rendering engine.
 * @details By default, cursors are typically sought from the rendering engine.
 * Setting @p engine_only to @c EINA_FALSE allows searching in the widget's theme as well.
 * @note This function is effective only if a custom cursor has been set with @ref elm_object_item_cursor_set.
 * @param[in] obj The Elm_Widget_Item object.
 * @param[in] engine_only If @c EINA_TRUE, cursors are looked for only in those provided by the rendering engine.
 *                        If @c EINA_FALSE, they are also searched in the widget's theme.
 */
EAPI void
elm_object_item_cursor_engine_only_set(Elm_Widget_Item *obj, Eina_Bool engine_only)
{
   elm_wdg_item_cursor_engine_only_set(obj, engine_only);
}

/**
 * @brief Gets whether the item's custom cursor search is restricted to the rendering engine.
 * @details This function returns if the custom cursor for the item is searched only within the
 * rendering engine's provided cursors or also in the theme.
 * @note This is relevant only if a custom cursor has been set with @ref elm_object_item_cursor_set.
 * @param[in] obj The Elm_Widget_Item object.
 * @return @c EINA_TRUE if cursors are searched only in the rendering engine,
 *         @c EINA_FALSE if the theme is also searched.
 */
EAPI Eina_Bool
elm_object_item_cursor_engine_only_get(const Elm_Widget_Item *obj)
{
   return elm_wdg_item_cursor_engine_only_get(obj);
}

/**
 * @brief Sets the content of a specific part of an object item.
 * @details This function assigns an Efl_Canvas_Object as content to a named part of the
 * Elm_Widget_Item. If any object was already set in the same part, the previous
 * object will be deleted automatically.
 * @param[in] obj The Elm_Widget_Item object.
 * @param[in] part The name of the content part (e.g., "icon", "end").
 *                 Use @c NULL for the default content part if supported by the widget.
 * @param[in] content The Efl_Canvas_Object to set as content.
 */
EAPI void
elm_object_item_part_content_set(Elm_Widget_Item *obj, const char *part, Efl_Canvas_Object *content)
{
   elm_wdg_item_part_content_set(obj, part, content);
}

/**
 * @brief Gets the content of a specific part of an object item.
 * @details This function retrieves the Efl_Canvas_Object currently set as content
 * for the named part of the Elm_Widget_Item.
 * @param[in] obj The Elm_Widget_Item object.
 * @param[in] part The name of the content part.
 *                 Use @c NULL for the default content part if supported.
 * @return The Efl_Canvas_Object set as content for the part, or @c NULL if no
 *         content is set for that part or on error.
 */
EAPI Efl_Canvas_Object *
elm_object_item_part_content_get(const Elm_Widget_Item *obj, const char *part)
{
   return elm_wdg_item_part_content_get(obj, part);
}

/**
 * @brief Sets the text for a specific part of an object item.
 * @details Elm_Widget_Item objects can have multiple text parts. This function
 * sets the text label for a named part.
 * @param[in] obj The Elm_Widget_Item object.
 * @param[in] part The name of the text part (e.g., "default", "title").
 *                 Use @c NULL for the default text part if supported by the widget.
 * @param[in] label The text string to set for the part.
 */
EAPI void
elm_object_item_part_text_set(Elm_Widget_Item *obj, const char *part, const char *label)
{
   elm_wdg_item_part_text_set(obj, part, label);
}

/**
 * @brief Gets the text from a specific part of an object item.
 * @details This function retrieves the text label currently set for the named part
 * of the Elm_Widget_Item.
 * @param[in] obj The Elm_Widget_Item object.
 * @param[in] part The name of the text part.
 *                 Use @c NULL for the default text part if supported.
 * @return The text string of the part, or @c NULL if no text is set for that
 *         part or on error.
 */
EAPI const char *
elm_object_item_part_text_get(const Elm_Widget_Item *obj, const char *part)
{
   return elm_wdg_item_part_text_get(obj, part);
}

/**
 * @brief Sets the focused state of an object item.
 * @details This function programmatically sets whether the specified Elm_Widget_Item
 * is focused.
 * @param[in] obj The Elm_Widget_Item object.
 * @param[in] focused @c EINA_TRUE to set the item as focused, @c EINA_FALSE to unfocus it.
 * @since 1.10
 */
EAPI void
elm_object_item_focus_set(Elm_Widget_Item *obj, Eina_Bool focused)
{
   elm_wdg_item_focus_set(obj, focused);
}

/**
 * @brief Gets the focused state of an object item.
 * @details This function returns whether the specified Elm_Widget_Item is currently focused.
 * @param[in] obj The Elm_Widget_Item object.
 * @return @c EINA_TRUE if the item is focused, @c EINA_FALSE otherwise.
 * @since 1.10
 */
EAPI Eina_Bool
elm_object_item_focus_get(const Elm_Widget_Item *obj)
{
   return elm_wdg_item_focus_get(obj);
}

/**
 * @brief Sets the style of an object item.
 * @details This function applies a specific theme style to the Elm_Widget_Item.
 * The available styles depend on the widget and the current theme.
 * @param[in] obj The Elm_Widget_Item object.
 * @param[in] style The name of the style to apply to the object item.
 * @since 1.9
 */
EAPI void
elm_object_item_style_set(Elm_Widget_Item *obj, const char *style)
{
   elm_wdg_item_style_set(obj, style);
}

/**
 * @brief Gets the style of an object item.
 * @details This function retrieves the name of the style currently applied to the
 * Elm_Widget_Item.
 * @param[in] obj The Elm_Widget_Item object.
 * @return The name of the style applied to the object item, or @c NULL on error
 *         or if no specific style is set (implying a default style).
 * @since 1.9
 */
EAPI const char *
elm_object_item_style_get(const Elm_Widget_Item *obj)
{
   return elm_wdg_item_style_get(obj);
}

/**
 * @brief Sets the disabled state of a widget item.
 * @details Disables or enables the specified Elm_Widget_Item. A disabled item
 * typically does not receive input and may be themed differently (e.g., greyed out).
 * @param[in] obj The Elm_Widget_Item object.
 * @param[in] disable @c EINA_TRUE to disable the item, @c EINA_FALSE to enable it.
 */
EAPI void
elm_object_item_disabled_set(Elm_Widget_Item *obj, Eina_Bool disable)
{
   elm_wdg_item_disabled_set(obj, disable);
}

/**
 * @brief Gets the disabled state of a widget item.
 * @details This function returns whether the specified Elm_Widget_Item is currently disabled.
 * @param[in] obj The Elm_Widget_Item object.
 * @return @c EINA_TRUE if the item is disabled, @c EINA_FALSE if it is enabled
 *         (or on errors, where it might default to @c EINA_FALSE).
 */
EAPI Eina_Bool
elm_object_item_disabled_get(const Elm_Widget_Item *obj)
{
   return elm_wdg_item_disabled_get(obj);
}

/**
 * @brief Gets the access highlight order for an item.
 * @details Retrieves a list of Evas_Object pointers that defines a custom order
 * for accessibility highlighting within or among items.
 * @param[in] obj The Elm_Widget_Item object.
 * @return A const Eina_List of Evas_Object pointers representing the access order.
 *         The list is owned by the item and should not be modified or freed by the caller.
 *         Returns @c NULL if no specific order is set or on error.
 * @since 1.8
 */
EAPI const Eina_List *
elm_object_item_access_order_get(Elm_Widget_Item *obj)
{
   return elm_wdg_item_access_order_get(obj);
}

/**
 * @brief Sets the access highlight order for an item.
 * @details Defines a custom order for accessibility highlighting. The provided list
 * @p objs should contain Evas_Object pointers.
 * @param[in] obj The Elm_Widget_Item object.
 * @param[in] objs An Eina_List of Evas_Object pointers establishing the order.
 *                 The item typically takes ownership or copies this list as needed.
 * @since 1.8
 */
EAPI void
elm_object_item_access_order_set(Elm_Widget_Item *obj, Eina_List *objs)
{
   elm_wdg_item_access_order_set(obj, objs);
}

/**
 * @brief Gets the widget object that contains a given item.
 * @details This function returns the parent Efl_Canvas_Object (the widget) to which
 * the specified Elm_Widget_Item belongs.
 * @param[in] obj The Elm_Widget_Item object.
 * @return The parent Efl_Canvas_Object (widget) of the item, or @c NULL on error.
 */
EAPI Efl_Canvas_Object *
elm_object_item_widget_get(const Elm_Widget_Item *obj)
{
   return elm_wdg_item_widget_get(obj);
}

/**
 * @brief Sets the text to be shown in an object item's tooltip.
 * @details This function sets a simple text string as the tooltip for the
 * Elm_Widget_Item. Any previous tooltip content (text or custom content)
 * will be removed.
 * For more details, see elm_object_tooltip_text_set().
 * @param[in] obj The Elm_Widget_Item object.
 * @param[in] text The text string to set for the tooltip.
 */
EAPI void
elm_object_item_tooltip_text_set(Elm_Widget_Item *obj, const char *text)
{
   elm_wdg_item_tooltip_text_set(obj, text);
}

/**
 * @brief Unsets the tooltip from an item.
 * @details This function removes any tooltip previously set on the Elm_Widget_Item.
 * If a content callback was set via @ref elm_object_item_tooltip_content_cb_set,
 * its associated deletion callback (del_cb) will be invoked.
 * For more details, see elm_object_tooltip_unset().
 * @param[in] obj The Elm_Widget_Item object.
 */
EAPI void
elm_object_item_tooltip_unset(Elm_Widget_Item *obj)
{
   elm_wdg_item_tooltip_unset(obj);
}

/**
 * @brief Unsets any custom mouse cursor for the item.
 * @details This function removes any custom mouse pointer/cursor decoration
 * previously set on the Elm_Widget_Item using @ref elm_object_item_cursor_set.
 * The item will revert to showing the default cursor behavior.
 * For more details, see elm_object_cursor_unset().
 * @param[in] obj The Elm_Widget_Item object.
 */
EAPI void
elm_object_item_cursor_unset(Elm_Widget_Item *obj)
{
   elm_wdg_item_cursor_unset(obj);
}

/**
 * @brief Unsets the content of a specific part of an object item.
 * @details This function removes and returns the Efl_Canvas_Object from a named
 * part of the Elm_Widget_Item. The caller may be responsible for deleting
 * the returned object if it's no longer needed.
 * @param[in] obj The Elm_Widget_Item object.
 * @param[in] part The name of the content part to unset (e.g., "icon").
 *                 Use @c NULL for the default content part if supported.
 * @return The Efl_Canvas_Object that was previously set as content for the part,
 *         or @c NULL if no content was set or on error.
 */
EAPI Efl_Canvas_Object *
elm_object_item_part_content_unset(Elm_Widget_Item *obj, const char *part)
{
   return elm_wdg_item_part_content_unset(obj, part);
}

/**
 * @brief Adds a callback for a signal emitted by the object item's Edje object.
 * @details This function connects a callback function to a signal emitted by the
 * underlying Edje object of the Elm_Widget_Item. Glob patterns can be used
 * in @p emission or @p source.
 * @param[in] obj The Elm_Widget_Item object.
 * @param[in] emission The name of the signal (e.g., "mouse,clicked,1").
 * @param[in] source The source of the signal (e.g., "my_button_part").
 * @param[in] func The callback function to execute when the signal is emitted.
 * @param[in] data User data to be passed to the callback function.
 * @since 1.8
 */
EAPI void
elm_object_item_signal_callback_add(Elm_Widget_Item *obj, const char *emission, const char *source, Elm_Object_Item_Signal_Cb func, void *data)
{
   elm_wdg_item_signal_callback_add(obj, emission, source, func, data);
}

/**
 * @brief Deletes a signal callback from an object item's Edje object.
 * @details This function removes a previously added signal callback. The callback
 * is identified by matching @p emission, @p source, and @p func.
 * Only the most recently added callback matching these criteria is removed.
 * @param[in] obj The Elm_Widget_Item object.
 * @param[in] emission The signal's name.
 * @param[in] source The signal's source.
 * @param[in] func The callback function that was previously added.
 * @return The data pointer associated with the removed callback, or @c NULL if
 *         no matching callback was found or on error.
 * @since 1.8
 */
EAPI void *
elm_object_item_signal_callback_del(Elm_Widget_Item *obj, const char *emission, const char *source, Elm_Object_Item_Signal_Cb func)
{
   return elm_wdg_item_signal_callback_del(obj, emission, source, func);
}

/**
 * @brief Emits a signal to the Edje object of the widget item.
 * @details This function sends a signal with the given @p emission and @p source
 * to the underlying Edje object of the Elm_Widget_Item. This can trigger
 * actions defined in the theme's Edje program.
 * @warning Use with caution, as improper use might interfere with widget behavior,
 *          especially concerning part state management in list/grid items.
 * @param[in] obj The Elm_Widget_Item object.
 * @param[in] emission The name of the signal to emit.
 * @param[in] source The source string for the signal.
 */
EAPI void
elm_object_item_signal_emit(Elm_Widget_Item *obj, const char *emission, const char *source)
{
   elm_wdg_item_signal_emit(obj, emission, source);
}

/**
 * @brief Sets the accessibility information text for an object item.
 * @details This function provides a descriptive text for the Elm_Widget_Item,
 * which can be used by accessibility tools (e.g., screen readers).
 * @param[in] obj The Elm_Widget_Item object.
 * @param[in] txt The descriptive text for accessibility.
 */
EAPI void
elm_object_item_access_info_set(Elm_Widget_Item *obj, const char *txt)
{
   elm_wdg_item_access_info_set(obj, txt);
}

/**
 * @brief Gets the accessible object associated with an object item.
 * @details Retrieves the Efl_Canvas_Object that represents the Elm_Widget_Item
 * in the accessibility tree.
 * @param[in] obj The Elm_Widget_Item object.
 * @return The accessible Efl_Canvas_Object for the item, or @c NULL on error or
 *         if accessibility is not supported/enabled for the item.
 * @since 1.8
 */
EAPI Efl_Canvas_Object *
elm_object_item_access_object_get(const Elm_Widget_Item *obj)
{
   return elm_wdg_item_access_object_get(obj);
}

/**
 * @brief Sets the text for an item's part, marking it as translatable with a specific domain.
 * @details This function sets the text for a given part of the Elm_Widget_Item.
 * The @p label should be the original, untranslated string. Elementary will
 * handle translation using the specified @p domain. If @p domain is @c NULL,
 * the application's default text domain is used.
 * @param[in] obj The Elm_Widget_Item object.
 * @param[in] part The name of the text part to set.
 * @param[in] domain The translation domain (e.g., "my-library-translations"). Can be @c NULL.
 * @param[in] label The original, non-translated text string.
 * @since 1.8
 */
EAPI void
elm_object_item_domain_translatable_part_text_set(Elm_Widget_Item *obj, const char *part, const char *domain, const char *label)
{
   elm_wdg_item_domain_translatable_part_text_set(obj, part, domain, label);
}

/**
 * @brief Gets the original, untranslated string set for a translatable part of an item.
 * @details While @ref elm_object_item_part_text_get would return the translated text,
 * this function retrieves the original string that was set using
 * @ref elm_object_item_domain_translatable_part_text_set or a similar translatable text function.
 * @param[in] obj The Elm_Widget_Item object.
 * @param[in] part The name of the text part.
 * @return The original, untranslated string, or @c NULL if not found or on error.
 * @since 1.8
 */
EAPI const char *
elm_object_item_translatable_part_text_get(const Elm_Widget_Item *obj, const char *part)
{
   return elm_wdg_item_translatable_part_text_get(obj, part);
}

/**
 * @brief Marks an item's part text as translatable or not, with a specific domain.
 * @details This function explicitly controls the translatability of a text part
 * on an Elm_Widget_Item. If @p translatable is @c EINA_TRUE, the text in the
 * specified @p part will be treated as translatable using the given @p domain.
 * If @c EINA_FALSE, it will not be translated, overriding global policies.
 * See also elm_policy().
 * @param[in] obj The Elm_Widget_Item object.
 * @param[in] part The name of the text part.
 * @param[in] domain The translation domain to use if @p translatable is @c EINA_TRUE. Can be @c NULL.
 * @param[in] translatable @c EINA_TRUE to mark the part's text as translatable,
 *                         @c EINA_FALSE otherwise.
 * @since 1.8
 */
EAPI void
elm_object_item_domain_part_text_translatable_set(Elm_Widget_Item *obj, const char *part, const char *domain, Eina_Bool translatable)
{
   elm_wdg_item_domain_part_text_translatable_set(obj, part, domain, translatable);
}

/**
 * @brief Gets a tracking object for an item.
 * @details This function returns an Efl_Canvas_Object that represents the item's
 * internal object, allowing its geometry, visibility, etc., to be queried using
 * Evas APIs. The track object's reference count is incremented.
 * @warning The returned track object should not be modified. After use,
 *          @ref elm_object_item_untrack() must be called to decrement the
 *          reference count and allow proper cleanup. Do not delete the track object directly.
 *          The item itself can change or be deleted by its parent widget at any time.
 * @param[in] obj The Elm_Widget_Item object.
 * @return The Efl_Canvas_Object for tracking, or @c NULL if the item does not
 *         have a trackable internal object or on error.
 * @see elm_object_item_untrack()
 * @see elm_object_item_track_get()
 * @since 1.8
 */
EAPI Efl_Canvas_Object *
elm_object_item_track(Elm_Widget_Item *obj)
{
   return elm_wdg_item_track(obj);
}

/**
 * @brief Decrements the reference count of an item's tracking object.
 * @details This function should be called to balance a previous call to
 * @ref elm_object_item_track(). When the reference count reaches zero,
 * the tracking object may be freed.
 * @param[in] obj The Elm_Widget_Item object.
 * @see elm_object_item_track()
 * @since 1.8
 */
EAPI void
elm_object_item_untrack(Elm_Widget_Item *obj)
{
   elm_wdg_item_untrack(obj);
}

/**
 * @brief Gets the reference count of an item's tracking object.
 * @details This function returns the current reference count for the track object
 * obtained via @ref elm_object_item_track().
 * @param[in] obj The Elm_Widget_Item object.
 * @return The current reference count of the track object.
 * @see elm_object_item_track()
 * @see elm_object_item_untrack()
 * @since 1.8
 */
EAPI int
elm_object_item_track_get(const Elm_Widget_Item *obj)
{
   return elm_wdg_item_track_get(obj);
}

/**
 * @brief Sets a callback function to be invoked when an item is deleted.
 * @details This function registers a callback that will be called when the
 * Elm_Widget_Item @p obj is about to be freed.
 * @param[in] obj The Elm_Widget_Item object.
 * @param[in] del_cb The Evas_Smart_Cb function to call on item deletion.
 *                   The item itself is passed as the `data` argument to the callback,
 *                   and `event_info` is typically NULL for this callback type.
 */
EAPI void
elm_object_item_del_cb_set(Elm_Widget_Item *obj, Evas_Smart_Cb del_cb)
{
   elm_wdg_item_del_cb_set(obj, del_cb);
}

/**
 * @brief Sets a content callback function for an item's tooltip.
 * @details This function configures a callback (@p func) that will be invoked
 * to generate the content (an Evas_Object) for the item's tooltip whenever
 * it needs to be displayed. The returned Evas_Object is managed by the tooltip system.
 * Any previously set tooltip (text or content callback) is removed.
 * For more details, see elm_object_tooltip_content_cb_set().
 * @param[in] obj The Elm_Widget_Item object.
 * @param[in] func The Elm_Tooltip_Item_Content_Cb function to generate tooltip content.
 * @param[in] data User data to be passed to @p func.
 * @param[in] del_cb An Evas_Smart_Cb function called when @p data is no longer needed
 *                   (e.g., tooltip is unset or item is deleted). Receives @p data as its
 *                   first parameter and the item as @c event_info.
 */
EAPI void
elm_object_item_tooltip_content_cb_set(Elm_Widget_Item *obj, Elm_Tooltip_Item_Content_Cb func, const void *data, Evas_Smart_Cb del_cb)
{
   elm_wdg_item_tooltip_content_cb_set(obj, func, data, del_cb);
}

/**
 * @brief Registers an object item as an accessible object.
 * @details This function explicitly registers the Elm_Widget_Item with the
 * accessibility system, making it part of the accessibility tree.
 * @param[in] obj The Elm_Widget_Item object.
 * @return The Efl_Canvas_Object representing the accessible object for the item,
 *         or @c NULL on error or if accessibility registration fails.
 * @since 1.8
 */
EAPI Efl_Canvas_Object *
elm_object_item_access_register(Elm_Widget_Item *obj)
{
   return elm_wdg_item_access_register(obj);
}

/**
 * @brief Unregisters an object item from the accessibility system.
 * @details This function removes the Elm_Widget_Item from the accessibility tree
 * if it was previously registered.
 * @param[in] obj The Elm_Widget_Item object.
 * @since 1.8
 */
EAPI void
elm_object_item_access_unregister(Elm_Widget_Item *obj)
{
   elm_wdg_item_access_unregister(obj);
}

/**
 * @brief Unsets the custom accessibility highlight order for an item.
 * @details This function removes any custom accessibility highlight order previously
 * set on the Elm_Widget_Item using @ref elm_object_item_access_order_set.
 * The item will revert to default accessibility navigation behavior.
 * @param[in] obj The Elm_Widget_Item object.
 * @since 1.8
 */
EAPI void
elm_object_item_access_order_unset(Elm_Widget_Item *obj)
{
   elm_wdg_item_access_order_unset(obj);
}

/**
 * @brief Gets the next focusable Efl_Canvas_Object in a specific direction relative to the item.
 * @details This function determines the next Efl_Canvas_Object that would receive focus
 * if navigation occurs in the given @p dir from the current Elm_Widget_Item.
 * @param[in] obj The Elm_Widget_Item object.
 * @param[in] dir The Elm_Focus_Direction indicating the direction of focus navigation.
 * @return The next Efl_Canvas_Object to receive focus in the specified direction,
 *         or @c NULL if no such object exists or on error.
 * @since 1.16
 */
EAPI Efl_Canvas_Object *
elm_object_item_focus_next_object_get(const Elm_Widget_Item *obj, Elm_Focus_Direction dir)
{
   return elm_wdg_item_focus_next_object_get(obj, dir);
}

/**
 * @brief Sets the next focusable Efl_Canvas_Object in a specific direction for an item.
 * @details This function explicitly defines which Efl_Canvas_Object (@p next) should
 * receive focus when navigating from the Elm_Widget_Item @p obj in the direction @p dir.
 * This overrides the default focus navigation order.
 * @param[in] obj The Elm_Widget_Item object.
 * @param[in] next The Efl_Canvas_Object to be focused next.
 * @param[in] dir The Elm_Focus_Direction for which this custom next object applies.
 * @since 1.16
 */
EAPI void
elm_object_item_focus_next_object_set(Elm_Widget_Item *obj, Efl_Canvas_Object *next, Elm_Focus_Direction dir)
{
   elm_wdg_item_focus_next_object_set(obj, next, dir);
}

/**
 * @brief Gets the next focusable Elm_Widget_Item in a specific direction relative to the item.
 * @details This function determines the next Elm_Widget_Item that would receive focus
 * if navigation occurs in the given @p dir from the current Elm_Widget_Item.
 * This is useful when focus navigation is primarily between items of the same widget.
 * @param[in] obj The Elm_Widget_Item object.
 * @param[in] dir The Elm_Focus_Direction indicating the direction of focus navigation.
 * @return The next Elm_Widget_Item to receive focus in the specified direction,
 *         or @c NULL if no such item exists or on error.
 * @since 1.16
 */
EAPI Elm_Widget_Item *
elm_object_item_focus_next_item_get(const Elm_Widget_Item *obj, Elm_Focus_Direction dir)
{
   return elm_wdg_item_focus_next_item_get(obj, dir);
}

/**
 * @brief Sets the next focusable Elm_Widget_Item in a specific direction for an item.
 * @details This function explicitly defines which Elm_Widget_Item (@p next_item) should
 * receive focus when navigating from the Elm_Widget_Item @p obj in the direction @p dir.
 * This overrides the default focus navigation order among items.
 * @param[in] obj The Elm_Widget_Item object.
 * @param[in] next_item The Elm_Widget_Item to be focused next.
 * @param[in] dir The Elm_Focus_Direction for which this custom next item applies.
 * @since 1.16
 */
EAPI void
elm_object_item_focus_next_item_set(Elm_Widget_Item *obj, Elm_Widget_Item *next_item, Elm_Focus_Direction dir)
{
   elm_wdg_item_focus_next_item_set(obj, next_item, dir);
}
