#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include <Elementary.h>
#include "elm_priv.h"

/**
 * @brief Adds a new scrolled entry to the parent
 *
 * @deprecated This function is deprecated. Use elm_entry_add() and
 * elm_entry_scrollable_set() instead.
 *
 * @param parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup Elm_Scrolled_Entry
 */
EINA_DEPRECATED EAPI Evas_Object *
elm_scrolled_entry_add(Evas_Object *parent)
{
   Evas_Object *obj;
   obj = elm_entry_add(parent);
   elm_entry_scrollable_set(obj, EINA_TRUE);
   return obj;
}

/**
 * @brief Set the icon of the scrolled entry.
 *
 * @deprecated This function is deprecated. Use
 * elm_object_part_content_set(obj, "icon", icon) instead.
 *
 * @param obj The entry object.
 * @param icon The icon object.
 *
 * @ingroup Elm_Scrolled_Entry
 */
EINA_DEPRECATED EAPI void
elm_scrolled_entry_icon_set(Evas_Object *obj, Evas_Object *icon)
{elm_object_part_content_set(obj, "icon", icon);}

/**
 * @brief Get the icon of the scrolled entry.
 *
 * @deprecated This function is deprecated. Use
 * elm_object_part_content_get(obj, "icon") instead.
 *
 * @param obj The entry object.
 * @return The icon object, or NULL if not set.
 *
 * @ingroup Elm_Scrolled_Entry
 */
EINA_DEPRECATED EAPI Evas_Object *
elm_scrolled_entry_icon_get(const Evas_Object *obj)
{return elm_object_part_content_get(obj, "icon");}

/**
 * @brief Unset the icon of the scrolled entry.
 *
 * @deprecated This function is deprecated. Use
 * elm_object_part_content_unset(obj, "icon") instead.
 *
 * @param obj The entry object.
 * @return The previously set icon object, or NULL if none was set.
 *
 * @ingroup Elm_Scrolled_Entry
 */
EINA_DEPRECATED EAPI Evas_Object *
elm_scrolled_entry_icon_unset(Evas_Object *obj)
{return elm_object_part_content_unset(obj, "icon");}

/**
 * @brief Set the visibility of the icon.
 *
 * @deprecated This function is deprecated. Use elm_entry_icon_visible_set() instead.
 *
 * @param obj The entry object.
 * @param setting EINA_TRUE to show the icon, EINA_FALSE to hide it.
 *
 * @ingroup Elm_Scrolled_Entry
 */
EINA_DEPRECATED EAPI void
elm_scrolled_entry_icon_visible_set(Evas_Object *obj, Eina_Bool setting)
{elm_entry_icon_visible_set(obj, setting);}

/**
 * @brief Set the end widget of the scrolled entry.
 *
 * @deprecated This function is deprecated. Use
 * elm_object_part_content_set(obj, "end", end) instead.
 *
 * @param obj The entry object.
 * @param end The end object.
 *
 * @ingroup Elm_Scrolled_Entry
 */
EINA_DEPRECATED EAPI void
elm_scrolled_entry_end_set(Evas_Object *obj, Evas_Object *end)
{elm_object_part_content_set(obj, "end", end);}

/**
 * @brief Get the end widget of the scrolled entry.
 *
 * @deprecated This function is deprecated. Use
 * elm_object_part_content_get(obj, "end") instead.
 *
 * @param obj The entry object.
 * @return The end object, or NULL if not set.
 *
 * @ingroup Elm_Scrolled_Entry
 */
EINA_DEPRECATED EAPI Evas_Object *
elm_scrolled_entry_end_get(const Evas_Object *obj)
{return elm_object_part_content_get(obj, "end");}

/**
 * @brief Unset the end widget of the scrolled entry.
 *
 * @deprecated This function is deprecated. Use
 * elm_object_part_content_unset(obj, "end") instead.
 *
 * @param obj The entry object.
 * @return The previously set end object, or NULL if none was set.
 *
 * @ingroup Elm_Scrolled_Entry
 */
EINA_DEPRECATED EAPI Evas_Object *
elm_scrolled_entry_end_unset(Evas_Object *obj)
{return elm_object_part_content_unset(obj, "end");}

/**
 * @brief Set the visibility of the end widget.
 *
 * @deprecated This function is deprecated. Use elm_entry_end_visible_set() instead.
 *
 * @param obj The entry object.
 * @param setting EINA_TRUE to show the end widget, EINA_FALSE to hide it.
 *
 * @ingroup Elm_Scrolled_Entry
 */
EINA_DEPRECATED EAPI void
elm_scrolled_entry_end_visible_set(Evas_Object *obj, Eina_Bool setting)
{elm_entry_end_visible_set(obj, setting);}

/**
 * @brief Set the single line mode of the scrolled entry.
 *
 * @deprecated This function is deprecated. Use elm_entry_single_line_set() instead.
 *
 * @param obj The entry object.
 * @param single_line EINA_TRUE for single line mode, EINA_FALSE for multi-line.
 *
 * @ingroup Elm_Scrolled_Entry
 */
EINA_DEPRECATED EAPI void
elm_scrolled_entry_single_line_set(Evas_Object *obj, Eina_Bool single_line)
{elm_entry_single_line_set(obj, single_line);}

/**
 * @brief Get the single line mode of the scrolled entry.
 *
 * @deprecated This function is deprecated. Use elm_entry_single_line_get() instead.
 *
 * @param obj The entry object.
 * @return EINA_TRUE if in single line mode, EINA_FALSE otherwise.
 *
 * @ingroup Elm_Scrolled_Entry
 */
EINA_DEPRECATED EAPI Eina_Bool
elm_scrolled_entry_single_line_get(const Evas_Object *obj)
{return elm_entry_single_line_get(obj);}

/**
 * @brief Set the password mode of the scrolled entry.
 *
 * @deprecated This function is deprecated. Use elm_entry_password_set() instead.
 *
 * @param obj The entry object.
 * @param password EINA_TRUE for password mode, EINA_FALSE otherwise.
 *
 * @ingroup Elm_Scrolled_Entry
 */
EINA_DEPRECATED EAPI void
elm_scrolled_entry_password_set(Evas_Object *obj, Eina_Bool password)
{elm_entry_password_set(obj, password);}

/**
 * @brief Get the password mode of the scrolled entry.
 *
 * @deprecated This function is deprecated. Use elm_entry_password_get() instead.
 *
 * @param obj The entry object.
 * @return EINA_TRUE if in password mode, EINA_FALSE otherwise.
 *
 * @ingroup Elm_Scrolled_Entry
 */
EINA_DEPRECATED EAPI Eina_Bool
elm_scrolled_entry_password_get(const Evas_Object *obj)
{return elm_entry_password_get(obj);}

/**
 * @brief Set the text of the scrolled entry.
 *
 * @deprecated This function is deprecated. Use elm_object_text_set() instead.
 *
 * @param obj The entry object.
 * @param entry The text to set.
 *
 * @ingroup Elm_Scrolled_Entry
 */
EINA_DEPRECATED EAPI void
elm_scrolled_entry_entry_set(Evas_Object *obj, const char *entry)
{elm_object_text_set(obj, entry);}

/**
 * @brief Append text to the scrolled entry.
 *
 * @deprecated This function is deprecated. Use elm_entry_entry_append() instead.
 *
 * @param obj The entry object.
 * @param entry The text to append.
 *
 * @ingroup Elm_Scrolled_Entry
 */
EINA_DEPRECATED EAPI void
elm_scrolled_entry_entry_append(Evas_Object *obj, const char *entry)
{elm_entry_entry_append(obj, entry);}

/**
 * @brief Get the text of the scrolled entry.
 *
 * @deprecated This function is deprecated. Use elm_object_text_get() instead.
 *
 * @param obj The entry object.
 * @return The text of the entry.
 *
 * @ingroup Elm_Scrolled_Entry
 */
EINA_DEPRECATED EAPI const char *
elm_scrolled_entry_entry_get(const Evas_Object *obj)
{return elm_object_text_get(obj);}

/**
 * @brief Check if the scrolled entry is empty.
 *
 * @deprecated This function is deprecated. Use elm_entry_is_empty() instead.
 *
 * @param obj The entry object.
 * @return EINA_TRUE if the entry is empty, EINA_FALSE otherwise.
 *
 * @ingroup Elm_Scrolled_Entry
 */
EINA_DEPRECATED EAPI Eina_Bool
elm_scrolled_entry_is_empty(const Evas_Object *obj)
{return elm_entry_is_empty(obj);}

/**
 * @brief Get the selected text in the scrolled entry.
 *
 * @deprecated This function is deprecated. Use elm_entry_selection_get() instead.
 *
 * @param obj The entry object.
 * @return The selected text, or NULL if no selection.
 *
 * @ingroup Elm_Scrolled_Entry
 */
EINA_DEPRECATED EAPI const char *
elm_scrolled_entry_selection_get(const Evas_Object *obj)
{return elm_entry_selection_get(obj);}

/**
 * @brief Insert text into the scrolled entry at the current cursor position.
 *
 * @deprecated This function is deprecated. Use elm_entry_entry_insert() instead.
 *
 * @param obj The entry object.
 * @param entry The text to insert.
 *
 * @ingroup Elm_Scrolled_Entry
 */
EINA_DEPRECATED EAPI void
elm_scrolled_entry_entry_insert(Evas_Object *obj, const char *entry)
{elm_entry_entry_insert(obj, entry);}

/**
 * @brief Set the line wrap type for the scrolled entry.
 *
 * @deprecated This function is deprecated. Use elm_entry_line_wrap_set() instead.
 *
 * @param obj The entry object.
 * @param wrap The wrap type (ELM_WRAP_NONE, ELM_WRAP_CHAR, ELM_WRAP_WORD, ELM_WRAP_MIXED).
 *
 * @ingroup Elm_Scrolled_Entry
 */
EINA_DEPRECATED EAPI void
elm_scrolled_entry_line_wrap_set(Evas_Object *obj, Elm_Wrap_Type wrap)
{elm_entry_line_wrap_set(obj, wrap);}

/**
 * @brief Set the editable state of the scrolled entry.
 *
 * @deprecated This function is deprecated. Use elm_entry_editable_set() instead.
 *
 * @param obj The entry object.
 * @param editable EINA_TRUE to make editable, EINA_FALSE to make read-only.
 *
 * @ingroup Elm_Scrolled_Entry
 */
EINA_DEPRECATED EAPI void
elm_scrolled_entry_editable_set(Evas_Object *obj, Eina_Bool editable)
{elm_entry_editable_set(obj, editable);}

/**
 * @brief Get the editable state of the scrolled entry.
 *
 * @deprecated This function is deprecated. Use elm_entry_editable_get() instead.
 *
 * @param obj The entry object.
 * @return EINA_TRUE if editable, EINA_FALSE otherwise.
 *
 * @ingroup Elm_Scrolled_Entry
 */
EINA_DEPRECATED EAPI Eina_Bool
elm_scrolled_entry_editable_get(const Evas_Object *obj)
{return elm_entry_editable_get(obj);}

/**
 * @brief Deselect any selected text in the scrolled entry.
 *
 * @deprecated This function is deprecated. Use elm_entry_select_none() instead.
 *
 * @param obj The entry object.
 *
 * @ingroup Elm_Scrolled_Entry
 */
EINA_DEPRECATED EAPI void
elm_scrolled_entry_select_none(Evas_Object *obj)
{elm_entry_select_none(obj);}

/**
 * @brief Select all text in the scrolled entry.
 *
 * @deprecated This function is deprecated. Use elm_entry_select_all() instead.
 *
 * @param obj The entry object.
 *
 * @ingroup Elm_Scrolled_Entry
 */
EINA_DEPRECATED EAPI void
elm_scrolled_entry_select_all(Evas_Object *obj)
{return elm_entry_select_all(obj);}

/**
 * @brief Move the cursor one character to the right.
 *
 * @deprecated This function is deprecated. Use elm_entry_cursor_next() instead.
 *
 * @param obj The entry object.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 *
 * @ingroup Elm_Scrolled_Entry
 */
EINA_DEPRECATED EAPI Eina_Bool
elm_scrolled_entry_cursor_next(Evas_Object *obj)
{return elm_entry_cursor_next(obj);}

/**
 * @brief Move the cursor one character to the left.
 *
 * @deprecated This function is deprecated. Use elm_entry_cursor_prev() instead.
 *
 * @param obj The entry object.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 *
 * @ingroup Elm_Scrolled_Entry
 */
EINA_DEPRECATED EAPI Eina_Bool
elm_scrolled_entry_cursor_prev(Evas_Object *obj)
{return elm_entry_cursor_prev(obj);}

/**
 * @brief Move the cursor one line up.
 *
 * @deprecated This function is deprecated. Use elm_entry_cursor_up() instead.
 *
 * @param obj The entry object.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 *
 * @ingroup Elm_Scrolled_Entry
 */
EINA_DEPRECATED EAPI Eina_Bool
elm_scrolled_entry_cursor_up(Evas_Object *obj)
{return elm_entry_cursor_up(obj);}

/**
 * @brief Move the cursor one line down.
 *
 * @deprecated This function is deprecated. Use elm_entry_cursor_down() instead.
 *
 * @param obj The entry object.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 *
 * @ingroup Elm_Scrolled_Entry
 */
EINA_DEPRECATED EAPI Eina_Bool
elm_scrolled_entry_cursor_down(Evas_Object *obj)
{return elm_entry_cursor_down(obj);}

/**
 * @brief Move the cursor to the beginning of the entry.
 *
 * @deprecated This function is deprecated. Use elm_entry_cursor_begin_set() instead.
 *
 * @param obj The entry object.
 *
 * @ingroup Elm_Scrolled_Entry
 */
EINA_DEPRECATED EAPI void
elm_scrolled_entry_cursor_begin_set(Evas_Object *obj)
{elm_entry_cursor_begin_set(obj);}

/**
 * @brief Move the cursor to the end of the entry.
 *
 * @deprecated This function is deprecated. Use elm_entry_cursor_end_set() instead.
 *
 * @param obj The entry object.
 *
 * @ingroup Elm_Scrolled_Entry
 */
EINA_DEPRECATED EAPI void
elm_scrolled_entry_cursor_end_set(Evas_Object *obj)
{elm_entry_cursor_end_set(obj);}

/**
 * @brief Move the cursor to the beginning of the current line.
 *
 * @deprecated This function is deprecated. Use elm_entry_cursor_line_begin_set() instead.
 *
 * @param obj The entry object.
 *
 * @ingroup Elm_Scrolled_Entry
 */
EINA_DEPRECATED EAPI void
elm_scrolled_entry_cursor_line_begin_set(Evas_Object *obj)
{elm_entry_cursor_line_begin_set(obj);}

/**
 * @brief Move the cursor to the end of the current line.
 *
 * @deprecated This function is deprecated. Use elm_entry_cursor_line_end_set() instead.
 *
 * @param obj The entry object.
 *
 * @ingroup Elm_Scrolled_Entry
 */
EINA_DEPRECATED EAPI void
elm_scrolled_entry_cursor_line_end_set(Evas_Object *obj)
{elm_entry_cursor_line_end_set(obj);}

/**
 * @brief Begin a selection from the current cursor position.
 *
 * @deprecated This function is deprecated. Use elm_entry_cursor_selection_begin() instead.
 *
 * @param obj The entry object.
 *
 * @ingroup Elm_Scrolled_Entry
 */
EINA_DEPRECATED EAPI void
elm_scrolled_entry_cursor_selection_begin(Evas_Object *obj)
{elm_entry_cursor_selection_begin(obj);}

/**
 * @brief End the current selection.
 *
 * @deprecated This function is deprecated. Use elm_entry_cursor_selection_end() instead.
 *
 * @param obj The entry object.
 *
 * @ingroup Elm_Scrolled_Entry
 */
EINA_DEPRECATED EAPI void
elm_scrolled_entry_cursor_selection_end(Evas_Object *obj)
{return elm_entry_cursor_selection_end(obj);}

/**
 * @brief Check if the cursor is currently on a format tag.
 *
 * @deprecated This function is deprecated. Use elm_entry_cursor_is_format_get() instead.
 *
 * @param obj The entry object.
 * @return EINA_TRUE if on a format tag, EINA_FALSE otherwise.
 *
 * @ingroup Elm_Scrolled_Entry
 */
EINA_DEPRECATED EAPI Eina_Bool
elm_scrolled_entry_cursor_is_format_get(const Evas_Object *obj)
{return elm_entry_cursor_is_format_get(obj);}

/**
 * @brief Check if the cursor is currently on a visible format tag.
 *
 * @deprecated This function is deprecated. Use elm_entry_cursor_is_visible_format_get() instead.
 *
 * @param obj The entry object.
 * @return EINA_TRUE if on a visible format tag, EINA_FALSE otherwise.
 *
 * @ingroup Elm_Scrolled_Entry
 */
EINA_DEPRECATED EAPI Eina_Bool
elm_scrolled_entry_cursor_is_visible_format_get(const Evas_Object *obj)
{return elm_entry_cursor_is_visible_format_get(obj);}

/**
 * @brief Get the content (text or item) at the current cursor position.
 *
 * @deprecated This function is deprecated. Use elm_entry_cursor_content_get() instead.
 *
 * @param obj The entry object.
 * @return The content at the cursor, or NULL.
 *
 * @ingroup Elm_Scrolled_Entry
 */
EINA_DEPRECATED EAPI const char *
elm_scrolled_entry_cursor_content_get(const Evas_Object *obj)
{return elm_entry_cursor_content_get(obj);}

/**
 * @brief Set the cursor position in the scrolled entry.
 *
 * @deprecated This function is deprecated. Use elm_entry_cursor_pos_set() instead.
 *
 * @param obj The entry object.
 * @param pos The new cursor position.
 *
 * @ingroup Elm_Scrolled_Entry
 */
EINA_DEPRECATED EAPI void
elm_scrolled_entry_cursor_pos_set(Evas_Object *obj, int pos)
{elm_entry_cursor_pos_set(obj, pos);}

/**
 * @brief Get the current cursor position in the scrolled entry.
 *
 * @deprecated This function is deprecated. Use elm_entry_cursor_pos_get() instead.
 *
 * @param obj The entry object.
 * @return The cursor position.
 *
 * @ingroup Elm_Scrolled_Entry
 */
EINA_DEPRECATED EAPI int
elm_scrolled_entry_cursor_pos_get(const Evas_Object *obj)
{return elm_entry_cursor_pos_get(obj);}

/**
 * @brief Cut the selected text from the scrolled entry.
 *
 * @deprecated This function is deprecated. Use elm_entry_selection_cut() instead.
 *
 * @param obj The entry object.
 *
 * @ingroup Elm_Scrolled_Entry
 */
EINA_DEPRECATED EAPI void
elm_scrolled_entry_selection_cut(Evas_Object *obj)
{elm_entry_selection_cut(obj);}

/**
 * @brief Copy the selected text from the scrolled entry.
 *
 * @deprecated This function is deprecated. Use elm_entry_selection_copy() instead.
 *
 * @param obj The entry object.
 *
 * @ingroup Elm_Scrolled_Entry
 */
EINA_DEPRECATED EAPI void
elm_scrolled_entry_selection_copy(Evas_Object *obj)
{elm_entry_selection_copy(obj);}

/**
 * @brief Paste text into the scrolled entry from the clipboard.
 *
 * @deprecated This function is deprecated. Use elm_entry_selection_paste() instead.
 *
 * @param obj The entry object.
 *
 * @ingroup Elm_Scrolled_Entry
 */
EINA_DEPRECATED EAPI void
elm_scrolled_entry_selection_paste(Evas_Object *obj)
{elm_entry_selection_paste(obj);}

/**
 * @brief Clear the context menu of the scrolled entry.
 *
 * @deprecated This function is deprecated. Use elm_entry_context_menu_clear() instead.
 *
 * @param obj The entry object.
 *
 * @ingroup Elm_Scrolled_Entry
 */
EINA_DEPRECATED EAPI void
elm_scrolled_entry_context_menu_clear(Evas_Object *obj)
{elm_entry_context_menu_clear(obj);}

/**
 * @brief Add an item to the context menu of the scrolled entry.
 *
 * @deprecated This function is deprecated. Use elm_entry_context_menu_item_add() instead.
 *
 * @param obj The entry object.
 * @param label The label for the menu item.
 * @param icon_file The icon file path or standard icon name.
 * @param icon_type The type of the icon.
 * @param func The callback function to call when the item is clicked.
 * @param data The data to pass to the callback function.
 *
 * @ingroup Elm_Scrolled_Entry
 */
EINA_DEPRECATED EAPI void
elm_scrolled_entry_context_menu_item_add(Evas_Object *obj, const char *label, const char *icon_file, Elm_Icon_Type icon_type, Evas_Smart_Cb func, const void *data)
{elm_entry_context_menu_item_add(obj, label, icon_file, icon_type, func, data);}

/**
 * @brief Set the disabled state of the context menu.
 *
 * @deprecated This function is deprecated. Use elm_entry_context_menu_disabled_set() instead.
 *
 * @param obj The entry object.
 * @param disabled EINA_TRUE to disable the context menu, EINA_FALSE to enable it.
 *
 * @ingroup Elm_Scrolled_Entry
 */
EINA_DEPRECATED EAPI void
elm_scrolled_entry_context_menu_disabled_set(Evas_Object *obj, Eina_Bool disabled)
{elm_entry_context_menu_disabled_set(obj, disabled);}

/**
 * @brief Get the disabled state of the context menu.
 *
 * @deprecated This function is deprecated. Use elm_entry_context_menu_disabled_get() instead.
 *
 * @param obj The entry object.
 * @return EINA_TRUE if the context menu is disabled, EINA_FALSE otherwise.
 *
 * @ingroup Elm_Scrolled_Entry
 */
EINA_DEPRECATED EAPI Eina_Bool
elm_scrolled_entry_context_menu_disabled_get(const Evas_Object *obj)
{return elm_entry_context_menu_disabled_get(obj);}

/**
 * @brief Set the scrollbar policy for the scrolled entry.
 *
 * @deprecated This function is deprecated. Use elm_scroller_policy_set() instead.
 *
 * @param obj The entry object (which is a scroller).
 * @param h The horizontal scrollbar policy.
 * @param v The vertical scrollbar policy.
 *
 * @ingroup Elm_Scrolled_Entry
 */
EINA_DEPRECATED EAPI void
elm_scrolled_entry_scrollbar_policy_set(Evas_Object *obj, Elm_Scroller_Policy h, Elm_Scroller_Policy v)
{elm_scroller_policy_set(obj, h, v);}

/**
 * @brief Set the bounce behavior for the scrolled entry.
 *
 * @deprecated This function is deprecated. Use elm_scroller_bounce_set() instead.
 *
 * @param obj The entry object (which is a scroller).
 * @param h_bounce EINA_TRUE to enable horizontal bounce, EINA_FALSE to disable.
 * @param v_bounce EINA_TRUE to enable vertical bounce, EINA_FALSE to disable.
 *
 * @ingroup Elm_Scrolled_Entry
 */
EINA_DEPRECATED EAPI void
elm_scrolled_entry_bounce_set(Evas_Object *obj, Eina_Bool h_bounce, Eina_Bool v_bounce)
{elm_scroller_bounce_set(obj, h_bounce, v_bounce);}

/**
 * @brief Get the bounce behavior for the scrolled entry.
 *
 * @deprecated This function is deprecated. Use elm_scroller_bounce_get() instead.
 *
 * @param obj The entry object (which is a scroller).
 * @param h_bounce Pointer to store the horizontal bounce state.
 * @param v_bounce Pointer to store the vertical bounce state.
 *
 * @ingroup Elm_Scrolled_Entry
 */
EINA_DEPRECATED EAPI void
elm_scrolled_entry_bounce_get(const Evas_Object *obj, Eina_Bool *h_bounce, Eina_Bool *v_bounce)
{elm_scroller_bounce_get(obj, h_bounce, v_bounce);}

/**
 * @brief Append an item provider to the scrolled entry.
 *
 * @deprecated This function is deprecated. Use elm_entry_item_provider_append() instead.
 *
 * @param obj The entry object.
 * @param func The item provider function.
 * @param data The data to pass to the item provider function.
 *
 * @ingroup Elm_Scrolled_Entry
 */
EINA_DEPRECATED EAPI void
elm_scrolled_entry_item_provider_append(Evas_Object *obj, Evas_Object *(*func) (void *data, Evas_Object *entry, const char *item), void *data)
{elm_entry_item_provider_append(obj, func, data);}

/**
 * @brief Prepend an item provider to the scrolled entry.
 *
 * @deprecated This function is deprecated. Use elm_entry_item_provider_prepend() instead.
 *
 * @param obj The entry object.
 * @param func The item provider function.
 * @param data The data to pass to the item provider function.
 *
 * @ingroup Elm_Scrolled_Entry
 */
EINA_DEPRECATED EAPI void
elm_scrolled_entry_item_provider_prepend(Evas_Object *obj, Evas_Object *(*func) (void *data, Evas_Object *entry, const char *item), void *data)
{elm_entry_item_provider_prepend(obj, func, data);}

/**
 * @brief Remove an item provider from the scrolled entry.
 *
 * @deprecated This function is deprecated. Use elm_entry_item_provider_remove() instead.
 *
 * @param obj The entry object.
 * @param func The item provider function to remove.
 * @param data The data associated with the item provider function.
 *
 * @ingroup Elm_Scrolled_Entry
 */
EINA_DEPRECATED EAPI void
elm_scrolled_entry_item_provider_remove(Evas_Object *obj, Evas_Object *(*func) (void *data, Evas_Object *entry, const char *item), void *data)
{elm_entry_item_provider_remove(obj, func, data);}

/**
 * @brief Append a text filter (markup filter) to the scrolled entry.
 *
 * @deprecated This function is deprecated. Use elm_entry_markup_filter_append() instead.
 *
 * @param obj The entry object.
 * @param func The filter function.
 * @param data The data to pass to the filter function.
 *
 * @ingroup Elm_Scrolled_Entry
 */
EINA_DEPRECATED EAPI void
elm_scrolled_entry_text_filter_append(Evas_Object *obj, void (*func) (void *data, Evas_Object *entry, char **text), void *data)
{elm_entry_markup_filter_append(obj, func, data);}

/**
 * @brief Prepend a text filter (markup filter) to the scrolled entry.
 *
 * @deprecated This function is deprecated. Use elm_entry_markup_filter_prepend() instead.
 *
 * @param obj The entry object.
 * @param func The filter function.
 * @param data The data to pass to the filter function.
 *
 * @ingroup Elm_Scrolled_Entry
 */
EINA_DEPRECATED EAPI void
elm_scrolled_entry_text_filter_prepend(Evas_Object *obj, void (*func) (void *data, Evas_Object *entry, char **text), void *data)
{elm_entry_markup_filter_prepend(obj, func, data);}

/**
 * @brief Remove a text filter (markup filter) from the scrolled entry.
 *
 * @deprecated This function is deprecated. Use elm_entry_markup_filter_remove() instead.
 *
 * @param obj The entry object.
 * @param func The filter function to remove.
 * @param data The data associated with the filter function.
 *
 * @ingroup Elm_Scrolled_Entry
 */
EINA_DEPRECATED EAPI void
elm_scrolled_entry_text_filter_remove(Evas_Object *obj, void (*func) (void *data, Evas_Object *entry, char **text), void *data)
{elm_entry_markup_filter_remove(obj, func, data);}

/**
 * @brief Set the file from which to load the entry's content.
 *
 * @deprecated This function is deprecated. Use elm_entry_file_set() instead.
 *
 * @param obj The entry object.
 * @param file The path to the file.
 * @param format The format of the file.
 *
 * @ingroup Elm_Scrolled_Entry
 */
EINA_DEPRECATED EAPI void
elm_scrolled_entry_file_set(Evas_Object *obj, const char *file, Elm_Text_Format format)
{elm_entry_file_set(obj, file, format);}

/**
 * @brief Get the file and format from which the entry's content was loaded.
 *
 * @deprecated This function is deprecated. Use elm_entry_file_get() instead.
 *
 * @param obj The entry object.
 * @param file Pointer to store the file path.
 * @param format Pointer to store the file format.
 *
 * @ingroup Elm_Scrolled_Entry
 */
EINA_DEPRECATED EAPI void
elm_scrolled_entry_file_get(const Evas_Object *obj, const char **file, Elm_Text_Format *format)
{elm_entry_file_get(obj, file, format);}

/**
 * @brief Save the entry's content to the currently set file.
 *
 * @deprecated This function is deprecated. Use elm_entry_file_save() instead.
 *
 * @param obj The entry object.
 *
 * @ingroup Elm_Scrolled_Entry
 */
EINA_DEPRECATED EAPI void
elm_scrolled_entry_file_save(Evas_Object *obj)
{elm_entry_file_save(obj);}

/**
 * @brief Set the autosave feature for the scrolled entry.
 *
 * @deprecated This function is deprecated. Use elm_entry_autosave_set() instead.
 *
 * @param obj The entry object.
 * @param autosave EINA_TRUE to enable autosave, EINA_FALSE to disable.
 *
 * @ingroup Elm_Scrolled_Entry
 */
EINA_DEPRECATED EAPI void
elm_scrolled_entry_autosave_set(Evas_Object *obj, Eina_Bool autosave)
{elm_entry_autosave_set(obj, autosave);}

/**
 * @brief Get the autosave feature state for the scrolled entry.
 *
 * @deprecated This function is deprecated. Use elm_entry_autosave_get() instead.
 *
 * @param obj The entry object.
 * @return EINA_TRUE if autosave is enabled, EINA_FALSE otherwise.
 *
 * @ingroup Elm_Scrolled_Entry
 */
EINA_DEPRECATED EAPI Eina_Bool
elm_scrolled_entry_autosave_get(const Evas_Object *obj)
{return elm_entry_autosave_get(obj);}

/**
 * @brief Set whether copy and paste operations handle only text or also markup.
 *
 * @deprecated This function is deprecated. Use elm_entry_cnp_mode_set() instead.
 *
 * @param obj The entry object.
 * @param textonly EINA_TRUE for text-only CnP, EINA_FALSE for markup CnP.
 *
 * @ingroup Elm_Scrolled_Entry
 */
EINA_DEPRECATED EAPI void
elm_scrolled_entry_cnp_textonly_set(Evas_Object *obj, Eina_Bool textonly)
{Elm_Cnp_Mode cnp_mode = ELM_CNP_MODE_MARKUP; if (textonly) cnp_mode = ELM_CNP_MODE_NO_IMAGE; elm_entry_cnp_mode_set(obj, cnp_mode);}

/**
 * @brief Get whether copy and paste operations handle only text or also markup.
 *
 * @deprecated This function is deprecated. Use elm_entry_cnp_mode_get() instead.
 *
 * @param obj The entry object.
 * @return EINA_TRUE if CnP is text-only, EINA_FALSE if it includes markup.
 *
 * @ingroup Elm_Scrolled_Entry
 */
EINA_DEPRECATED EAPI Eina_Bool
elm_scrolled_entry_cnp_textonly_get(Evas_Object *obj)
{return elm_entry_cnp_mode_get(obj) != ELM_CNP_MODE_MARKUP;}
