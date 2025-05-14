/* Legacy API implementations based on internal EO calls */

#include "edje_private.h"
#include "edje_part_helper.h"

/**
 * @internal
 * @brief Macro to get the Edje_Real_Part and Edje instances.
 *
 * This macro simplifies fetching the Edje_Real_Part and Edje private data
 * for a given Edje_Object and part name. It performs necessary null checks
 * and returns a specified value 'x' on failure.
 *
 * @param x The value to return if any check fails.
 */
#define GET_REAL_PART_ON_FAIL_RETURN(x) Edje_Real_Part *rp;\
                                        Edje *ed;\
                                        if (!part) return x;\
                                        ed = _edje_fetch(obj);\
                                        if (!ed) return x;\
                                        rp = _edje_real_part_recursive_get(&ed, part);\
                                        if (!rp) return x;\

/**
 * @brief Retrieves the load error for the given Edje object.
 *
 * This function returns the specific error code that occurred during
 * the loading of the Edje object's file.
 *
 * @param obj The Edje object.
 * @return The Edje_Load_Error code indicating the loading status.
 *         Returns EDJE_LOAD_ERROR_GENERIC if the object is invalid.
 */
EAPI Edje_Load_Error
edje_object_load_error_get(const Eo *obj)
{
   Edje *ed;

   ed = _edje_fetch(obj);
   if (!ed) return EDJE_LOAD_ERROR_GENERIC;
   return ed->load_error;
}

/**
 * @brief Gets the geometry of a specific part within an Edje object.
 *
 * This function retrieves the position (x, y) and size (w, h) of the
 * specified part. The geometry is relative to the Edje object's area.
 *
 * @param obj The Edje object.
 * @param part The name of the part.
 * @param[out] x Pointer to store the x-coordinate of the part, or NULL.
 * @param[out] y Pointer to store the y-coordinate of the part, or NULL.
 * @param[out] w Pointer to store the width of the part, or NULL.
 * @param[out] h Pointer to store the height of the part, or NULL.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure (e.g., if the
 *         part does not exist or the object is invalid).
 *
 * @note This function triggers a recalculation of the Edje object if needed.
 */
EAPI Eina_Bool
edje_object_part_geometry_get(const Edje_Object *obj, const char *part, int *x, int *y, int *w, int *h)
{
   Edje_Real_Part *rp;
   Edje *ed;
   EINA_SAFETY_ON_NULL_RETURN_VAL(part, EINA_FALSE);

   // Similar to geometry_get(efl_part(obj, part), x, y, w, h) but the bool
   // return value matters here.

   ed = _edje_fetch(obj);
   if (!ed)
     {
        if (x) *x = 0;
        if (y) *y = 0;
        if (w) *w = 0;
        if (h) *h = 0;
        return EINA_FALSE;
     }

   /* Need to recalc before providing the object. */
   _edje_recalc_do(ed);

   rp = _edje_real_part_recursive_get(&ed, part);
   if (!rp)
     {
        if (x) *x = 0;
        if (y) *y = 0;
        if (w) *w = 0;
        if (h) *h = 0;
        return EINA_FALSE;
     }
   if (x) *x = rp->x;
   if (y) *y = rp->y;
   if (w) *w = rp->w;
   if (h) *h = rp->h;
   return EINA_TRUE;
}

/**
 * @brief Gets the current state and value of a part.
 *
 * This function retrieves the name of the current state and its numerical value
 * for the specified part.
 *
 * @param obj The Edje object.
 * @param part The name of the part.
 * @param[out] val_ret Pointer to store the numerical value of the state (e.g., 0.0, 1.0), or NULL.
 * @return The name of the current state (e.g., "default", "clicked").
 *         Returns an empty string if the part is not found or an error occurs.
 *         The returned string is an Eina_Stringshare, do not free it.
 */
EAPI const char *
edje_object_part_state_get(const Edje_Object *obj, const char * part, double *val_ret)
{
   const char *str = "";
   efl_canvas_layout_part_state_get(efl_part(obj, part), &str, val_ret);
   return str;
}

/**
 * @brief Processes Edje messages and signals.
 *
 * This function processes any pending messages or signals for the Edje object.
 * It does not process signals recursively for sub-objects.
 *
 * @param obj The Edje object.
 * @see edje_object_message_signal_recursive_process()
 */
EAPI void
edje_object_message_signal_process(Edje_Object *obj)
{
   efl_layout_signal_process(obj, EINA_FALSE);
}

/* since 1.20 */
/**
 * @brief Processes Edje messages and signals recursively.
 *
 * This function processes any pending messages or signals for the Edje object
 * and all its sub-objects (TEXTBLOCK parts, SWALLOW parts).
 *
 * @param obj The Edje object.
 * @since 1.20
 * @see edje_object_message_signal_process()
 */
EAPI void
edje_object_message_signal_recursive_process(Edje_Object *obj)
{
   efl_layout_signal_process(obj, EINA_TRUE);
}

/**
 * @brief Adds a callback function for a specific signal from an Edje object.
 *
 * This function registers a callback that will be invoked when the Edje object
 * emits a signal matching the given emission and source patterns.
 *
 * @param obj The Edje object.
 * @param emission The emission string to match (e.g., "mouse,clicked,1"). Globbing can be used.
 * @param source The source string to match (e.g., "my_button"). Globbing can be used.
 * @param func The callback function to execute.
 * @param data User data to be passed to the callback function.
 */
EAPI void
edje_object_signal_callback_add(Evas_Object *obj, const char *emission, const char *source, Edje_Signal_Cb func, void *data)
{
   Edje *ed;

   ed = _edje_fetch(obj);
   if (!ed || ed->delete_me) return;
   _edje_object_signal_callback_add(obj, ed, emission, source, func, NULL, NULL, data);
}

/**
 * @brief Deletes a signal callback with full matching criteria.
 *
 * This function removes a previously added signal callback that matches
 * the specified emission, source, callback function, and user data.
 *
 * @param obj The Edje object.
 * @param emission The emission string of the callback to delete.
 * @param source The source string of the callback to delete.
 * @param func The callback function to delete.
 * @param data The user data associated with the callback to delete.
 * @return The user data pointer associated with the deleted callback.
 *         Historically, this function seems to have returned NULL since ~2013,
 *         despite documentation suggesting otherwise. Current behavior is to return NULL.
 */
EAPI void *
edje_object_signal_callback_del_full(Evas_Object *obj, const char *emission, const char *source, Edje_Signal_Cb func, void *data)
{
   Edje_Signal_Callback_Group *gp;
   Edje *ed = _edje_fetch(obj);
   Eina_Bool ok;

   if (!ed || ed->delete_me) return NULL;

   gp = (Edje_Signal_Callback_Group *) ed->callbacks;
   if (!gp) return NULL;

   emission = eina_stringshare_add(emission);
   source = eina_stringshare_add(source);

   // We can cast here as the function won't be used and is just going to be used for comparison
   ok = _edje_signal_callback_disable(gp, emission, source, func, NULL, NULL, data);

   // Legacy only
   if (!ok && !data)
     {
        for (unsigned i = 0; i < gp->matches->matches_count; ++i)
          {
             if (emission == gp->matches->matches[i].signal &&
                 source == gp->matches->matches[i].source &&
                 func == gp->matches->matches[i].legacy &&
                 gp->flags[i].legacy &&
                 !gp->flags[i].delete_me)
               {
                  gp->flags[i].delete_me = EINA_TRUE;
                  //return gp->custom_data[i];
                  break;
               }
          }
     }

   eina_stringshare_del(emission);
   eina_stringshare_del(source);

   // Note: This function seems to have returned NULL since ~2013, despite
   // what the documentation says.
   return NULL;
}

/**
 * @brief Deletes a signal callback.
 *
 * This function removes a previously added signal callback that matches
 * the specified emission, source, and callback function.
 * This is a convenience wrapper around edje_object_signal_callback_del_full()
 * with @p data set to @c NULL.
 *
 * @param obj The Edje object.
 * @param emission The emission string of the callback to delete.
 * @param source The source string of the callback to delete.
 * @param func The callback function to delete.
 * @return The user data pointer associated with the deleted callback.
 *         See edje_object_signal_callback_del_full() for notes on return value.
 * @note Legacy behavior: if @p data was @c NULL when adding, this function
 *       might match the first callback found with the given emission, source, and func.
 */
EAPI void *
edje_object_signal_callback_del(Evas_Object *obj, const char *emission, const char *source, Edje_Signal_Cb func)
{
   // Legacy del_full seems to have been sloppy with NULL data, as that would
   // match the first callback found. Keeping this legacy behaviour unchanged.
   return edje_object_signal_callback_del_full(obj, emission, source, func, NULL);
}

/**
 * @brief Emits a signal from the Edje object.
 *
 * This function programmatically triggers a signal with the given emission
 * and source strings. This can cause associated actions or callbacks to execute.
 *
 * @param obj The Edje object.
 * @param emission The emission string of the signal (e.g., "mouse,clicked,1").
 * @param source The source string of the signal (e.g., "my_button").
 */
EAPI void
edje_object_signal_emit(Evas_Object *obj, const char *emission, const char *source)
{
   efl_layout_signal_emit(obj, emission, source);
}

/**
 * @brief Sets an external parameter for a part of type EXTERNAL.
 *
 * This function allows setting parameters for EXTERNAL parts, which can
 * influence how the external object behaves or is displayed.
 *
 * @param obj The Edje object.
 * @param part The name of the EXTERNAL part.
 * @param param The Edje_External_Param structure containing the parameter to set.
 *              Example:
 *              Edje_External_Param param;
 *              param.name = "video_file";
 *              param.type = EDJE_EXTERNAL_PARAM_TYPE_STRING;
 *              param.s = "path/to/video.mp4";
 *              edje_object_part_external_param_set(obj, "my_video_part", &param);
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
EAPI Eina_Bool
edje_object_part_external_param_set(Eo *obj, const char *part, const Edje_External_Param *param)
{
   Edje *ed = _edje_fetch(obj);
   return _edje_object_part_external_param_set(ed, part, param);
}

/**
 * @brief Gets an external parameter for a part of type EXTERNAL.
 *
 * This function retrieves the value of a named parameter for an EXTERNAL part.
 * The caller must provide an Edje_External_Param structure, and the function
 * will fill it. For string types, the string is an Eina_Stringshare and should
 * not be freed by the caller if it's the same as param->name.
 *
 * @param obj The Edje object.
 * @param part The name of the EXTERNAL part.
 * @param[in,out] param An Edje_External_Param structure. The 'name' field should be
 *                      set to the parameter name to retrieve. Other fields will be
 *                      filled by the function.
 *                      Example:
 *                      Edje_External_Param param_get;
 *                      param_get.name = "video_file"; // Name of param to get
 *                      if (edje_object_part_external_param_get(obj, "my_video_part", &param_get)) {
 *                         // use param_get.s, param_get.i, etc. based on param_get.type
 *                         if (param_get.type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
 *                           eina_stringshare_del(param_get.s); // If it was duplicated
 *                      }
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
EAPI Eina_Bool
edje_object_part_external_param_get(const Eo *obj, const char *part, Edje_External_Param *param)
{
   Edje *ed = _edje_fetch(obj);
   return _edje_object_part_external_param_get(ed, part, param);
}

/**
 * @brief Gets the type of an external parameter for a part.
 *
 * This function retrieves the data type of a named external parameter
 * for an EXTERNAL part.
 *
 * @param obj The Edje object.
 * @param part The name of the EXTERNAL part.
 * @param param The name of the parameter whose type is to be retrieved.
 * @return The Edje_External_Param_Type of the parameter, or
 *         EDJE_EXTERNAL_PARAM_TYPE_MAX if the parameter or part is not found.
 */
EAPI Edje_External_Param_Type
edje_object_part_external_param_type_get(const Eo *obj, const char *part, const char *param)
{
   Edje *ed = _edje_fetch(obj);
   return _edje_object_part_external_param_type_get(ed, part, param);
}

/**
 * @brief Gets the Evas_Object associated with an EXTERNAL part.
 *
 * For parts of type EXTERNAL, this function returns the actual Evas_Object
 * that has been set for this part.
 *
 * @param obj The Edje object.
 * @param part The name of the EXTERNAL part.
 * @return The Evas_Object used by the external part, or @c NULL if none or on error.
 */
EAPI Evas_Object *
edje_object_part_external_object_get(const Edje_Object *obj, const char *part)
{
   return efl_content_get(efl_part(obj, part));
}

/* Legacy only. Shall we deprecate this API? */
/**
 * @brief Gets a named content from an EXTERNAL part. (Legacy)
 *
 * This function is a legacy way to retrieve a "content" from an external part.
 * Its usage is generally discouraged in favor of more direct external object manipulation.
 *
 * @param obj The Edje object.
 * @param part The name of the EXTERNAL part.
 * @param content The name of the content to retrieve from the external part.
 * @return The Evas_Object representing the named content, or @c NULL on error.
 * @warning This is a legacy API and its use is discouraged.
 */
EAPI Evas_Object *
edje_object_part_external_content_get(const Edje_Object *obj, const char *part, const char *content)
{
   Edje *ed = _edje_fetch(obj);
   return _edje_object_part_external_content_get(ed, part, content);
}

/* Efl.Ui.I18n APIs */

/**
 * @brief Sets the mirrored mode of an Edje object.
 *
 * This function controls the layout direction (LTR or RTL).
 *
 * @param obj The Edje object.
 * @param rtl @c EINA_TRUE for right-to-left, @c EINA_FALSE for left-to-right.
 */
EAPI void
edje_object_mirrored_set(Edje_Object *obj, Eina_Bool rtl)
{
   efl_ui_mirrored_set(obj, rtl);
}

/**
 * @brief Gets the mirrored mode of an Edje object.
 *
 * @param obj The Edje object.
 * @return @c EINA_TRUE if in right-to-left mode, @c EINA_FALSE otherwise.
 */
EAPI Eina_Bool edje_object_mirrored_get(const Edje_Object *obj)
{
   return efl_ui_mirrored_get(obj);
}

/**
 * @brief Sets the language for an Edje object.
 *
 * This can affect text display and other locale-specific behaviors.
 *
 * @param obj The Edje object.
 * @param language The language string (e.g., "en_US", "fr_FR").
 */
EAPI void edje_object_language_set(Edje_Object *obj, const char *language)
{
   efl_ui_language_set(obj, language);
}

/**
 * @brief Gets the language of an Edje object.
 *
 * @param obj The Edje object.
 * @return The current language string. The returned string is an Eina_Stringshare.
 */
EAPI const char *edje_object_language_get(const Edje_Object *obj)
{
   return efl_ui_language_get(obj);
}

/**
 * @brief Sets the scaling factor for an Edje object.
 *
 * This scales the entire Edje object and its contents.
 *
 * @param obj The Edje object.
 * @param scale The scaling factor. 1.0 is normal size.
 * @return @c EINA_TRUE always (historical reasons).
 */
EAPI Eina_Bool edje_object_scale_set(Edje_Object *obj, double scale)
{
   efl_gfx_entity_scale_set(obj, scale);
   return EINA_TRUE;
}

/**
 * @brief Gets the scaling factor of an Edje object.
 *
 * @param obj The Edje object.
 * @return The current scaling factor.
 */
EAPI double edje_object_scale_get(const Edje_Object *obj)
{
   return efl_gfx_entity_scale_get(obj);
}

/* Legacy part drag APIs */

/**
 * @brief Gets the drag direction for a draggable part.
 *
 * @param obj The Edje object.
 * @param part The name of the draggable part.
 * @return The Edje_Drag_Dir flags indicating allowed drag directions.
 */
EAPI Edje_Drag_Dir
edje_object_part_drag_dir_get(const Evas_Object *obj, const char *part)
{
   return (Edje_Drag_Dir)efl_ui_drag_dir_get(efl_part(obj, part));
}

/**
 * @brief Sets the drag value (position) for a draggable part.
 *
 * @param obj The Edje object.
 * @param part The name of the draggable part.
 * @param dx The horizontal drag amount (0.0 to 1.0).
 * @param dy The vertical drag amount (0.0 to 1.0).
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
EAPI Eina_Bool
edje_object_part_drag_value_set(Evas_Object *obj, const char *part, double dx, double dy)
{
   return efl_ui_drag_value_set(efl_part(obj, part), dx, dy);
}

/**
 * @brief Gets the drag value (position) for a draggable part.
 *
 * @param obj The Edje object.
 * @param part The name of the draggable part.
 * @param[out] dx Pointer to store the horizontal drag amount.
 * @param[out] dy Pointer to store the vertical drag amount.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
EAPI Eina_Bool
edje_object_part_drag_value_get(const Evas_Object *obj, const char *part, double *dx, double *dy)
{
   return efl_ui_drag_value_get(efl_part(obj, part), dx, dy);
}

/**
 * @brief Sets the drag size for a draggable part.
 *
 * @param obj The Edje object.
 * @param part The name of the draggable part.
 * @param dw The horizontal size factor.
 * @param dh The vertical size factor.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
EAPI Eina_Bool
edje_object_part_drag_size_set(Evas_Object *obj, const char *part, double dw, double dh)
{
   return efl_ui_drag_size_set(efl_part(obj, part), dw, dh);
}

/**
 * @brief Gets the drag size for a draggable part.
 *
 * @param obj The Edje object.
 * @param part The name of the draggable part.
 * @param[out] dw Pointer to store the horizontal size factor.
 * @param[out] dh Pointer to store the vertical size factor.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
EAPI Eina_Bool
edje_object_part_drag_size_get(const Evas_Object *obj, const char *part, double *dw, double *dh)
{
   return efl_ui_drag_size_get(efl_part(obj, part), dw, dh);
}

/**
 * @brief Sets the drag step increment for a draggable part.
 *
 * @param obj The Edje object.
 * @param part The name of the draggable part.
 * @param dx The horizontal step increment.
 * @param dy The vertical step increment.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
EAPI Eina_Bool
edje_object_part_drag_step_set(Evas_Object *obj, const char *part, double dx, double dy)
{
   return efl_ui_drag_step_set(efl_part(obj, part), dx, dy);
}

/**
 * @brief Gets the drag step increment for a draggable part.
 *
 * @param obj The Edje object.
 * @param part The name of the draggable part.
 * @param[out] dx Pointer to store the horizontal step increment.
 * @param[out] dy Pointer to store the vertical step increment.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
EAPI Eina_Bool
edje_object_part_drag_step_get(const Evas_Object *obj, const char *part, double *dx, double *dy)
{
   return efl_ui_drag_step_get(efl_part(obj, part), dx, dy);
}

/**
 * @brief Sets the drag page increment for a draggable part.
 *
 * @param obj The Edje object.
 * @param part The name of the draggable part.
 * @param dx The horizontal page increment.
 * @param dy The vertical page increment.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
EAPI Eina_Bool
edje_object_part_drag_page_set(Evas_Object *obj, const char *part, double dx, double dy)
{
   return efl_ui_drag_page_set(efl_part(obj, part), dx, dy);
}

/**
 * @brief Gets the drag page increment for a draggable part.
 *
 * @param obj The Edje object.
 * @param part The name of the draggable part.
 * @param[out] dx Pointer to store the horizontal page increment.
 * @param[out] dy Pointer to store the vertical page increment.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
EAPI Eina_Bool
edje_object_part_drag_page_get(const Evas_Object *obj, const char *part, double *dx, double *dy)
{
   return efl_ui_drag_page_get(efl_part(obj, part), dx, dy);
}

/**
 * @brief Moves a draggable part by one step.
 *
 * @param obj The Edje object.
 * @param part The name of the draggable part.
 * @param dx The horizontal step multiplier (e.g., 1.0 for one step right, -1.0 for one step left).
 * @param dy The vertical step multiplier.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
EAPI Eina_Bool
edje_object_part_drag_step(Evas_Object *obj, const char *part, double dx, double dy)
{
   return efl_ui_drag_step_move(efl_part(obj, part), dx, dy);
}

/**
 * @brief Moves a draggable part by one page.
 *
 * @param obj The Edje object.
 * @param part The name of the draggable part.
 * @param dx The horizontal page multiplier.
 * @param dy The vertical page multiplier.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
EAPI Eina_Bool
edje_object_part_drag_page(Evas_Object *obj, const char *part, double dx, double dy)
{
   return efl_ui_drag_page_move(efl_part(obj, part), dx, dy);
}

/**
 * @brief Sets the specified cursor to the beginning of the text in a part.
 *
 * @param obj The Edje object.
 * @param part The name of the text/textblock part. (Currently unused by internal logic due to macro)
 * @param cur The cursor to modify (e.g., EDJE_CURSOR_MAIN, EDJE_CURSOR_SELECTION_BEGIN).
 */
EAPI void
edje_object_part_text_cursor_begin_set(Edje_Object *obj, const char *part EINA_UNUSED, Edje_Cursor cur)
{
   GET_REAL_PART_ON_FAIL_RETURN()
   _edje_text_cursor_begin(rp, _edje_text_cursor_get(rp, cur));
}

/**
 * @brief Sets the specified cursor to the end of the text in a part.
 *
 * @param obj The Edje object.
 * @param part The name of the text/textblock part. (Currently unused by internal logic due to macro)
 * @param cur The cursor to modify.
 */
EAPI void
edje_object_part_text_cursor_end_set(Edje_Object *obj, const char *part EINA_UNUSED, Edje_Cursor cur)
{
   GET_REAL_PART_ON_FAIL_RETURN()
   _edje_text_cursor_end(rp, _edje_text_cursor_get(rp, cur));
}

/**
 * @brief Sets the position of the specified cursor in a text part.
 *
 * @param obj The Edje object.
 * @param part The name of the text/textblock part. (Currently unused by internal logic due to macro)
 * @param cur The cursor to modify.
 * @param pos The character position to set the cursor to.
 */
EAPI void
edje_object_part_text_cursor_pos_set(Edje_Object *obj, const char * part EINA_UNUSED, Edje_Cursor cur, int pos)
{
   GET_REAL_PART_ON_FAIL_RETURN()
   _edje_text_cursor_pos_set(rp, _edje_text_cursor_get(rp, cur), pos);
}

/**
 * @brief Gets the position of the specified cursor in a text part.
 *
 * @param obj The Edje object.
 * @param part The name of the text/textblock part. (Currently unused by internal logic due to macro)
 * @param cur The cursor whose position is to be retrieved.
 * @return The character position of the cursor, or 0 on failure.
 */
EAPI int
edje_object_part_text_cursor_pos_get(const Edje_Object *obj, const char * part EINA_UNUSED, Edje_Cursor cur)
{
   GET_REAL_PART_ON_FAIL_RETURN(0)
   return _edje_text_cursor_pos_get(rp, _edje_text_cursor_get(rp, cur));
}

/**
 * @brief Sets the cursor position in a text part based on coordinates.
 *
 * @param obj The Edje object.
 * @param part The name of the text/textblock part. (Currently unused by internal logic due to macro)
 * @param cur The cursor to modify.
 * @param x The x-coordinate within the part.
 * @param y The y-coordinate within the part.
 * @return @c EINA_TRUE if the cursor position was successfully set, @c EINA_FALSE otherwise.
 */
EAPI Eina_Bool
edje_object_part_text_cursor_coord_set(Edje_Object *obj, const char *part EINA_UNUSED, Edje_Cursor cur, int x, int y)
{
   GET_REAL_PART_ON_FAIL_RETURN(EINA_FALSE)
   return _edje_text_cursor_coord_set(rp, _edje_text_cursor_get(rp, cur), x, y);
}

/**
 * @brief Sets the specified cursor to the beginning of its current line in a text part.
 *
 * @param obj The Edje object.
 * @param part The name of the text/textblock part. (Currently unused by internal logic due to macro)
 * @param cur The cursor to modify.
 */
EAPI void
edje_object_part_text_cursor_line_begin_set(Edje_Object *obj, const char *part EINA_UNUSED, Edje_Cursor cur)
{
   GET_REAL_PART_ON_FAIL_RETURN()
   _edje_text_cursor_line_begin(rp, _edje_text_cursor_get(rp, cur));
}

/**
 * @brief Sets the specified cursor to the end of its current line in a text part.
 *
 * @param obj The Edje object.
 * @param part The name of the text/textblock part. (Currently unused by internal logic due to macro)
 * @param cur The cursor to modify.
 */
EAPI void
edje_object_part_text_cursor_line_end_set(Edje_Object *obj, const char *part EINA_UNUSED, Edje_Cursor cur)
{
   GET_REAL_PART_ON_FAIL_RETURN()
   _edje_text_cursor_line_end(rp, _edje_text_cursor_get(rp, cur));
}

/**
 * @brief Moves the specified cursor one character backward in a text part.
 *
 * @param obj The Edje object.
 * @param part The name of the text/textblock part. (Currently unused by internal logic due to macro)
 * @param cur The cursor to move.
 * @return @c EINA_TRUE if the cursor was moved, @c EINA_FALSE if it was already at the beginning or on error.
 */
EAPI Eina_Bool
edje_object_part_text_cursor_prev(Edje_Object *obj, const char *part EINA_UNUSED, Edje_Cursor cur)
{
   GET_REAL_PART_ON_FAIL_RETURN(EINA_FALSE)
   return  _edje_text_cursor_prev(rp, _edje_text_cursor_get(rp, cur));
}

/**
 * @brief Moves the specified cursor one character forward in a text part.
 *
 * @param obj The Edje object.
 * @param part The name of the text/textblock part. (Currently unused by internal logic due to macro)
 * @param cur The cursor to move.
 * @return @c EINA_TRUE if the cursor was moved, @c EINA_FALSE if it was already at the end or on error.
 */
EAPI Eina_Bool
edje_object_part_text_cursor_next(Edje_Object *obj, const char *part EINA_UNUSED, Edje_Cursor cur)
{
   GET_REAL_PART_ON_FAIL_RETURN(EINA_FALSE)
   return  _edje_text_cursor_next(rp, _edje_text_cursor_get(rp, cur));
}

/**
 * @brief Moves the specified cursor one line down in a text part.
 *
 * @param obj The Edje object.
 * @param part The name of the text/textblock part. (Currently unused by internal logic due to macro)
 * @param cur The cursor to move.
 * @return @c EINA_TRUE if the cursor was moved, @c EINA_FALSE if it was already on the last line or on error.
 */
EAPI Eina_Bool
edje_object_part_text_cursor_down(Edje_Object *obj, const char *part EINA_UNUSED, Edje_Cursor cur)
{
   GET_REAL_PART_ON_FAIL_RETURN(EINA_FALSE)
   return _edje_text_cursor_down(rp, _edje_text_cursor_get(rp, cur));
}

/**
 * @brief Moves the specified cursor one line up in a text part.
 *
 * @param obj The Edje object.
 * @param part The name of the text/textblock part. (Currently unused by internal logic due to macro)
 * @param cur The cursor to move.
 * @return @c EINA_TRUE if the cursor was moved, @c EINA_FALSE if it was already on the first line or on error.
 */
EAPI Eina_Bool
edje_object_part_text_cursor_up(Edje_Object *obj, const char *part EINA_UNUSED, Edje_Cursor cur)
{
   GET_REAL_PART_ON_FAIL_RETURN(EINA_FALSE)
   return _edje_text_cursor_up(rp, _edje_text_cursor_get(rp, cur));
}

/**
 * @brief Copies the position of one cursor to another in a text part.
 *
 * @param obj The Edje object.
 * @param part The name of the text/textblock part. (Currently unused by internal logic due to macro)
 * @param cur The source cursor.
 * @param dst The destination cursor.
 */
EAPI void
edje_object_part_text_cursor_copy(Edje_Object *obj, const char *part EINA_UNUSED, Edje_Cursor cur, Edje_Cursor dst)
{
   GET_REAL_PART_ON_FAIL_RETURN()
   _edje_text_cursor_copy(rp, _edje_text_cursor_get(rp, cur), _edje_text_cursor_get(rp, dst));
}

/**
 * @brief Gets the content (selected text) associated with a cursor in a text part.
 *
 * This typically refers to the text between selection cursors if `cur` is
 * EDJE_CURSOR_SELECTION_BEGIN or EDJE_CURSOR_SELECTION_END, or the character
 * at the main cursor if `cur` is EDJE_CURSOR_MAIN.
 *
 * @param obj The Edje object.
 * @param part The name of the text/textblock part. (Currently unused by internal logic due to macro)
 * @param cur The cursor defining the content (e.g., EDJE_CURSOR_MAIN for character at cursor,
 *            or one of the selection cursors for selected text).
 * @return A newly allocated string with the content, or @c NULL if no content or on error.
 *         The caller is responsible for freeing the returned string.
 */
EAPI char *
edje_object_part_text_cursor_content_get(const Edje_Object *obj, const char *part EINA_UNUSED, Edje_Cursor cur)
{
   GET_REAL_PART_ON_FAIL_RETURN(NULL)
   if (rp->part->entry_mode > EDJE_ENTRY_EDIT_MODE_NONE)
     {
        return _edje_entry_cursor_content_get(rp, cur);
     }

   return NULL;
}

/**
 * @brief Gets the geometry of the main cursor in a text part.
 *
 * @param obj The Edje object.
 * @param part The name of the text/textblock part. (Currently unused by internal logic due to macro)
 * @param[out] x Pointer to store the x-coordinate of the cursor, relative to the Edje object.
 * @param[out] y Pointer to store the y-coordinate of the cursor, relative to the Edje object.
 * @param[out] w Pointer to store the width of the cursor.
 * @param[out] h Pointer to store the height of the cursor.
 */
EAPI void
edje_object_part_text_cursor_geometry_get(const Edje_Object *obj, const char * part EINA_UNUSED, int *x, int *y, int *w, int *h)
{
   GET_REAL_PART_ON_FAIL_RETURN()
   if (rp->part->entry_mode > EDJE_ENTRY_EDIT_MODE_NONE)
     {
        _edje_entry_cursor_geometry_get(rp, x, y, w, h, NULL);
        if (x) *x -= ed->x;
        if (y) *y -= ed->y;
     }
}

/**
 * @brief Toggles the visibility of characters in a password entry part.
 *
 * If the part is an entry in password mode, this function will toggle
 * whether the actual characters or placeholder characters (e.g., asterisks) are shown.
 *
 * @param obj The Edje object.
 * @param part The name of the textblock part.
 * @return @c EINA_TRUE if the visibility was toggled, @c EINA_FALSE otherwise
 *         (e.g., not a password entry, part not found).
 */
EAPI Eina_Bool
edje_object_part_text_hide_visible_password(Eo *obj, const char *part)
{
   GET_REAL_PART_ON_FAIL_RETURN(EINA_FALSE)
   Eina_Bool int_ret = EINA_FALSE;
   if (rp->part->type != EDJE_PART_TYPE_TEXTBLOCK) return EINA_FALSE;
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text))
     {
        return EINA_FALSE;
     }

   if (rp->part->entry_mode == EDJE_ENTRY_EDIT_MODE_PASSWORD)
     int_ret = _edje_entry_hide_visible_password(ed, rp);

   return int_ret;
}

/**
 * @brief Checks if the specified cursor is currently over a format tag in a text part.
 *
 * @param obj The Edje object.
 * @param part The name of the text/textblock part.
 * @param cur The cursor to check.
 * @return @c EINA_TRUE if the cursor is over a format tag, @c EINA_FALSE otherwise or on error.
 */
EAPI Eina_Bool
edje_object_part_text_cursor_is_format_get(const Eo *obj, const char *part, Edje_Cursor cur)
{
   GET_REAL_PART_ON_FAIL_RETURN(EINA_FALSE)
   if (rp->part->entry_mode > EDJE_ENTRY_EDIT_MODE_NONE)
     {
        return _edje_entry_cursor_is_format_get(rp, cur);
     }
   return EINA_FALSE;
}

/**
 * @brief Checks if the specified cursor is currently over a visible format tag in a text part.
 *
 * @param obj The Edje object.
 * @param part The name of the text/textblock part.
 * @param cur The cursor to check.
 * @return @c EINA_TRUE if the cursor is over a visible format tag, @c EINA_FALSE otherwise or on error.
 */
EAPI Eina_Bool
edje_object_part_text_cursor_is_visible_format_get(const Eo *obj, const char *part, Edje_Cursor cur)
{
   GET_REAL_PART_ON_FAIL_RETURN(EINA_FALSE)
   if (rp->part->entry_mode > EDJE_ENTRY_EDIT_MODE_NONE)
     {
        return _edje_entry_cursor_is_visible_format_get(rp, cur);
     }

   return EINA_FALSE;
}

/**
 * @brief Gets a list of anchor names within a text part.
 *
 * Anchors are defined in the EDC script (e.g., <a href=anc_name>anchor</a>).
 *
 * @param obj The Edje object.
 * @param part The name of the text/textblock part.
 * @return A const Eina_List of (const char *) anchor names, or @c NULL on error.
 *         The list and its contents should not be modified or freed by the caller.
 *         The strings are Eina_Stringshare instances.
 */
EAPI const Eina_List *
edje_object_part_text_anchor_list_get(const Eo *obj, const char *part)
{
   GET_REAL_PART_ON_FAIL_RETURN(NULL)
   if (rp->part->entry_mode > EDJE_ENTRY_EDIT_MODE_NONE)
     return _edje_entry_anchors_list(rp);

   return NULL;
}

/**
 * @brief Gets the geometry of a named anchor within a text part.
 *
 * @param obj The Edje object.
 * @param part The name of the text/textblock part.
 * @param anchor The name of the anchor.
 * @return A const Eina_List of Eina_Rect structures representing the geometry
 *         of the anchor (an anchor can span multiple lines/rectangles), or @c NULL on error.
 *         The list and its contents should not be modified or freed by the caller.
 *         Example of iterating:
 *         const Eina_List *geoms; Eina_Rect *r;
 *         EINA_LIST_FOREACH(geoms, l, r) { printf("Rect: %d,%d %dx%d\n", r->x, r->y, r->w, r->h); }
 */
EAPI const Eina_List *
edje_object_part_text_anchor_geometry_get(const Eo *obj, const char *part, const char *anchor)
{
   GET_REAL_PART_ON_FAIL_RETURN(NULL)
   if (rp->part->entry_mode > EDJE_ENTRY_EDIT_MODE_NONE)
     return _edje_entry_anchor_geometry_get(rp, anchor);

   return NULL;
}

/**
 * @brief Pushes a new style onto the style stack for a textblock part.
 *
 * This allows temporarily overriding the default style of the textblock.
 *
 * @param obj The Edje object.
 * @param part The name of the TEXTBLOCK part.
 * @param style A string defining the style to push (e.g., "font_size=20 color=#FF0000").
 */
EAPI void
edje_object_part_text_style_user_push(Eo *obj, const char *part, const char *style)
{
   Evas_Textblock_Style *ts;
   GET_REAL_PART_ON_FAIL_RETURN()

   if (rp->part->type != EDJE_PART_TYPE_TEXTBLOCK) return;

   ts = evas_textblock_style_new();
   evas_textblock_style_set(ts, style);
   evas_object_textblock_style_user_push(rp->object, ts);
   evas_textblock_style_free(ts);
   ed->recalc_hints = EINA_TRUE;
#ifdef EDJE_CALC_CACHE
   rp->invalidate = EINA_TRUE;
#endif
   _edje_recalc(ed);
}

/**
 * @brief Pops the topmost style from the style stack of a textblock part.
 *
 * This reverts to the previous style on the stack.
 *
 * @param obj The Edje object.
 * @param part The name of the TEXTBLOCK part.
 */
EAPI void
edje_object_part_text_style_user_pop(Eo *obj, const char *part)
{
   GET_REAL_PART_ON_FAIL_RETURN()
   if (rp->part->type != EDJE_PART_TYPE_TEXTBLOCK) return;

   evas_object_textblock_style_user_pop(rp->object);
   ed->recalc_hints = EINA_TRUE;
#ifdef EDJE_CALC_CACHE
   rp->invalidate = EINA_TRUE;
#endif
   _edje_recalc(ed);
}

/**
 * @brief Peeks at the topmost style on the style stack of a textblock part.
 *
 * @param obj The Edje object.
 * @param part The name of the TEXTBLOCK part.
 * @return A string representing the current style, or @c NULL if no user style is pushed or on error.
 *         The returned string is owned by Evas and should not be freed.
 */
EAPI const char *
edje_object_part_text_style_user_peek(const Eo *obj, const char *part)
{
   Edje_Real_Part *rp;
   const Evas_Textblock_Style *ts;
   Edje *ed;
   EINA_SAFETY_ON_NULL_RETURN_VAL(part, NULL);

   ed = _edje_fetch(obj);

   if (!ed) return NULL;
   rp = _edje_real_part_recursive_get(&ed, part);
   if (!rp) return NULL;
   if (rp->part->type != EDJE_PART_TYPE_TEXTBLOCK) return NULL;

   ts = evas_object_textblock_style_user_peek(rp->object);
   if (ts)
     return evas_textblock_style_get(ts);

   return NULL;
}

/**
 * @brief Gets a list of item names within a text part.
 *
 * Items are typically images or other embedded objects defined in EDC (e.g., <item href=my_icon.png></item>).
 *
 * @param obj The Edje object.
 * @param part The name of the text/textblock part.
 * @return A const Eina_List of (const char *) item names, or @c NULL on error.
 *         The list and its contents should not be modified or freed by the caller.
 *         The strings are Eina_Stringshare instances.
 */
EAPI const Eina_List *
edje_object_part_text_item_list_get(const Eo *obj, const char *part)
{
   GET_REAL_PART_ON_FAIL_RETURN(NULL)
   if (rp->part->entry_mode > EDJE_ENTRY_EDIT_MODE_NONE)
     return _edje_entry_items_list(rp);

   return NULL;
}

/**
 * @brief Gets the geometry of a named item within a text part.
 *
 * @param obj The Edje object.
 * @param part The name of the text/textblock part.
 * @param item The name of the item (e.g., "my_icon.png").
 * @param[out] cx Pointer to store the x-coordinate of the item.
 * @param[out] cy Pointer to store the y-coordinate of the item.
 * @param[out] cw Pointer to store the width of the item.
 * @param[out] ch Pointer to store the height of the item.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure or if the item is not found.
 */
EAPI Eina_Bool
edje_object_part_text_item_geometry_get(const Eo *obj, const char *part, const char *item, Evas_Coord *cx, Evas_Coord *cy, Evas_Coord *cw, Evas_Coord *ch)
{
   GET_REAL_PART_ON_FAIL_RETURN(EINA_FALSE)
   if (rp->part->entry_mode > EDJE_ENTRY_EDIT_MODE_NONE)
     {
        return _edje_entry_item_geometry_get(rp, item, cx, cy, cw, ch);
     }

   return EINA_FALSE;
}

/**
 * @brief Adds a callback function to filter text being inserted into an Edje part.
 *
 * The callback can modify or reject the text before it is inserted.
 *
 * @param obj The Edje object.
 * @param part The name of the part to which the filter applies.
 * @param func The Edje_Text_Filter_Cb callback function.
 * @param data User data to be passed to the callback.
 */
EAPI void
edje_object_text_insert_filter_callback_add(Eo *obj, const char *part, Edje_Text_Filter_Cb func, void *data)
{
   Edje_Text_Insert_Filter_Callback *cb;
   Edje *ed;
   EINA_SAFETY_ON_NULL_RETURN(part);

   ed = _edje_fetch(obj);

   if (!ed) return;
   cb = calloc(1, sizeof(Edje_Text_Insert_Filter_Callback));
   cb->part = eina_stringshare_add(part);
   cb->func = func;
   cb->data = (void *)data;
   ed->text_insert_filter_callbacks =
     eina_list_append(ed->text_insert_filter_callbacks, cb);
}

/**
 * @brief Deletes a text insert filter callback.
 *
 * Removes a callback previously added with edje_object_text_insert_filter_callback_add().
 * Matches based on part name and function pointer.
 *
 * @param obj The Edje object.
 * @param part The name of the part.
 * @param func The callback function to delete.
 * @return The user data associated with the deleted callback, or @c NULL if not found.
 */
EAPI void *
edje_object_text_insert_filter_callback_del(Eo *obj, const char *part, Edje_Text_Filter_Cb func)
{
   Edje_Text_Insert_Filter_Callback *cb;
   Eina_List *l;
   Edje *ed;
   EINA_SAFETY_ON_NULL_RETURN_VAL(part, NULL);

   ed = _edje_fetch(obj);

   if (!ed) return NULL;
   EINA_LIST_FOREACH(ed->text_insert_filter_callbacks, l, cb)
     {
        if ((!strcmp(cb->part, part)) && (cb->func == func))
          {
             void *data = cb->data;
             ed->text_insert_filter_callbacks =
               eina_list_remove_list(ed->text_insert_filter_callbacks, l);
             eina_stringshare_del(cb->part);
             free(cb);
             return data;
          }
     }

   return NULL;
}

/**
 * @brief Deletes a text insert filter callback with full matching criteria.
 *
 * Removes a callback previously added, matching part name, function pointer, and data pointer.
 *
 * @param obj The Edje object.
 * @param part The name of the part.
 * @param func The callback function to delete.
 * @param data The user data to match.
 * @return The user data associated with the deleted callback, or @c NULL if not found.
 */
EAPI void *
edje_object_text_insert_filter_callback_del_full(Eo *obj, const char *part, Edje_Text_Filter_Cb func, void *data)
{
   Edje_Text_Insert_Filter_Callback *cb;
   Eina_List *l;
   Edje *ed;
   EINA_SAFETY_ON_NULL_RETURN_VAL(part, NULL);

   ed = _edje_fetch(obj);

   if (!ed) return NULL;
   EINA_LIST_FOREACH(ed->text_insert_filter_callbacks, l, cb)
     {
        if ((!strcmp(cb->part, part)) && (cb->func == func) &&
            (cb->data == data))
          {
             void *tmp = cb->data;
             ed->text_insert_filter_callbacks =
               eina_list_remove_list(ed->text_insert_filter_callbacks, l);
             eina_stringshare_del(cb->part);
             free(cb);
             return tmp;
          }
     }

   return NULL;
}

/**
 * @brief Adds a callback function to filter markup text being set or inserted into an Edje part.
 *
 * The callback can modify or reject the markup text.
 *
 * @param obj The Edje object.
 * @param part The name of the part to which the filter applies.
 * @param func The Edje_Markup_Filter_Cb callback function.
 * @param data User data to be passed to the callback.
 */
EAPI void
edje_object_text_markup_filter_callback_add(Eo *obj, const char *part, Edje_Markup_Filter_Cb func, void *data)
{
   Edje_Markup_Filter_Callback *cb;
   Edje *ed;
   EINA_SAFETY_ON_NULL_RETURN(part);

   ed = _edje_fetch(obj);

   if (!ed) return;
   cb = calloc(1, sizeof(Edje_Markup_Filter_Callback));
   cb->part = eina_stringshare_add(part);
   cb->func = func;
   cb->data = (void *)data;
   ed->markup_filter_callbacks =
     eina_list_append(ed->markup_filter_callbacks, cb);
}

/**
 * @brief Deletes a text markup filter callback.
 *
 * Removes a callback previously added with edje_object_text_markup_filter_callback_add().
 * Matches based on part name and function pointer.
 *
 * @param obj The Edje object.
 * @param part The name of the part.
 * @param func The callback function to delete.
 * @return The user data associated with the deleted callback, or @c NULL if not found.
 */
EAPI void *
edje_object_text_markup_filter_callback_del(Eo *obj, const char *part, Edje_Markup_Filter_Cb func)
{
   Edje_Markup_Filter_Callback *cb;
   Eina_List *l;
   Edje *ed;
   EINA_SAFETY_ON_NULL_RETURN_VAL(part, NULL);

   ed = _edje_fetch(obj);

   if (!ed) return NULL;
   EINA_LIST_FOREACH(ed->markup_filter_callbacks, l, cb)
     {
        if ((!strcmp(cb->part, part)) && (cb->func == func))
          {
             void *data = cb->data;
             ed->markup_filter_callbacks =
               eina_list_remove_list(ed->markup_filter_callbacks, l);
             eina_stringshare_del(cb->part);
             free(cb);
             return data;
          }
     }

   return NULL;
}

/**
 * @brief Deletes a text markup filter callback with full matching criteria.
 *
 * Removes a callback previously added, matching part name, function pointer, and data pointer.
 *
 * @param obj The Edje object.
 * @param part The name of the part.
 * @param func The callback function to delete.
 * @param data The user data to match.
 * @return The user data associated with the deleted callback, or @c NULL if not found.
 */
EAPI void *
edje_object_text_markup_filter_callback_del_full(Eo *obj, const char *part, Edje_Markup_Filter_Cb func, void *data)
{
   Edje_Markup_Filter_Callback *cb;
   Eina_List *l;
   Edje *ed;
   EINA_SAFETY_ON_NULL_RETURN_VAL(part, NULL);

   ed = _edje_fetch(obj);

   if (!ed) return NULL;
   EINA_LIST_FOREACH(ed->markup_filter_callbacks, l, cb)
     {
        if ((!strcmp(cb->part, part)) && (cb->func == func) &&
            (cb->data == data))
          {
             void *tmp = cb->data;
             ed->markup_filter_callbacks =
               eina_list_remove_list(ed->markup_filter_callbacks, l);
             eina_stringshare_del(cb->part);
             free(cb);
             return tmp;
          }
     }

   return NULL;
}

/**
 * @brief Inserts text at the current user cursor position in an editable text part.
 *
 * This function is intended for parts in an entry mode (e.g., editable text fields).
 *
 * @param obj The Edje object.
 * @param part The name of the editable text/textblock part.
 * @param text The text to insert. This text is treated as markup.
 */
EAPI void
edje_object_part_text_user_insert(const Eo *obj, const char *part, const char *text)
{
   Edje_Real_Part *rp;
   Edje *ed;
   EINA_SAFETY_ON_NULL_RETURN(part);

   ed = _edje_fetch(obj);

   if (!ed) return;
   rp = _edje_real_part_recursive_get(&ed, part);
   if (!rp) return;
   if (rp->part->entry_mode > EDJE_ENTRY_EDIT_MODE_NONE)
     _edje_entry_user_insert(rp, text);
}

/**
 * @internal
 * @brief Appends raw text to a text part.
 *
 * This is an internal helper function. For TEXT parts, it appends plain text.
 * For TEXTBLOCK parts in entry mode, it appends markup.
 * It handles memory allocation and string sharing.
 *
 * @param ed The Edje private data.
 * @param obj The Edje Evas_Object.
 * @param rp The Edje_Real_Part corresponding to the text part.
 * @param part The name of the part (used for text change callback).
 * @param text The text to append.
 * @return @c EINA_TRUE on success or if no action was needed, effectively always true.
 */
Eina_Bool
_edje_object_part_text_raw_append(Edje *ed, Evas_Object *obj, Edje_Real_Part *rp, const char *part, const char *text)
{
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return EINA_TRUE;
   if (rp->part->entry_mode > EDJE_ENTRY_EDIT_MODE_NONE)
     _edje_entry_text_markup_append(rp, text);
   else if (text)
     {
        if (rp->typedata.text->text)
          {
             char *new = NULL;
             int len_added = strlen(text);
             int len_old = strlen(rp->typedata.text->text);
             new = malloc(len_old + len_added + 1);
             memcpy(new, rp->typedata.text->text, len_old);
             memcpy(new + len_old, text, len_added);
             new[len_old + len_added] = '\0';
             eina_stringshare_replace(&rp->typedata.text->text, new);
             free(new);
          }
        else
          {
             eina_stringshare_replace(&rp->typedata.text->text, text);
          }
     }
   ed->dirty = EINA_TRUE;
   ed->recalc_call = 1;
#ifdef EDJE_CALC_CACHE
   rp->invalidate = EINA_TRUE;
#endif
   _edje_recalc(ed);
   if (ed->text_change.func)
     ed->text_change.func(ed->text_change.data, obj, part);
   return EINA_TRUE;
}

/**
 * @brief Appends text to a TEXTBLOCK part.
 *
 * The appended text is treated as markup.
 *
 * @param obj The Edje object.
 * @param part The name of the TEXTBLOCK part.
 * @param text The markup text to append.
 */
EAPI void
edje_object_part_text_append(Eo *obj, const char *part, const char *text)
{
   Edje_Real_Part *rp;
   Edje *ed;
   EINA_SAFETY_ON_NULL_RETURN(part);

   ed = _edje_fetch(obj);

   if (!ed) return;
   rp = _edje_real_part_recursive_get(&ed, part);
   if (!rp) return;
   if ((rp->part->type != EDJE_PART_TYPE_TEXTBLOCK)) return;
   _edje_object_part_text_raw_append(ed, obj, rp, part, text);
   ed->dirty = EINA_TRUE;
   ed->recalc_call = EINA_TRUE;
   ed->recalc_hints = EINA_TRUE;
#ifdef EDJE_CALC_CACHE
   rp->invalidate = EINA_TRUE;
#endif
   _edje_recalc(ed);
   if (ed->text_change.func)
     ed->text_change.func(ed->text_change.data, obj, part);
}

/**
 * @brief Sets the text of a part, assuming the input text is already escaped.
 *
 * For TEXT parts, this function processes escape sequences like "&amp;", "&lt;", etc.
 * within the input `text` to convert them to their literal characters.
 * For TEXTBLOCK parts, the text is set as is (raw).
 *
 * @param obj The Edje object.
 * @param part The name of the TEXT or TEXTBLOCK part.
 * @param text The text to set, which is assumed to contain Evas Textblock escape sequences
 *             if the part is of type TEXT. For TEXTBLOCK, it's treated as raw markup.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
EAPI Eina_Bool
edje_object_part_text_escaped_set(Eo *obj, const char *part, const char *text)
{
   Edje_Real_Part *rp;
   Eina_Bool int_ret;
   Edje *ed;
   EINA_SAFETY_ON_NULL_RETURN_VAL(part, EINA_FALSE);

   ed = _edje_fetch(obj);

   if (!ed) return EINA_FALSE;
   rp = _edje_real_part_recursive_get(&ed, part);
   if (!rp) return EINA_FALSE;
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return EINA_FALSE;
   if (rp->part->type != EDJE_PART_TYPE_TEXTBLOCK &&
       rp->part->type != EDJE_PART_TYPE_TEXT)
     return EINA_FALSE;
   if ((rp->part->type == EDJE_PART_TYPE_TEXT) && (text))
     {
        Eina_Strbuf *sbuf;
        char *esc_start = NULL, *esc_end = NULL;
        char *s, *p;

        sbuf = eina_strbuf_new();
        p = (char *)text;
        s = p;
        for (;; )
          {
             if ((*p == 0) || (esc_end) || (esc_start))
               {
                  if (esc_end)
                    {
                       const char *escape;

                       escape = evas_textblock_escape_string_range_get
                           (esc_start, esc_end + 1);
                       if (escape) eina_strbuf_append(sbuf, escape);
                       esc_start = esc_end = NULL;
                    }
                  else if (*p == 0)
                    {
                       /* This would happen when there is & that isn't escaped */
                       if (!s && esc_start) s = esc_start;
                       if (s) eina_strbuf_append_length(sbuf, s, p - s);
                       s = NULL;
                    }
                  if (*p == 0)
                    break;
               }

             if (*p == '&')
               {
                  /* This would happen when there is & that isn't escaped */
                  if (!s && esc_start) s = esc_start;
                  if (s) eina_strbuf_append_length(sbuf, s, p - s);
                  esc_start = p;
                  esc_end = NULL;
                  s = NULL;
               }
             else if (*p == ';')
               {
                  if (esc_start)
                    {
                       esc_end = p;
                       s = p + 1;
                    }
               }
             p++;
          }
        int_ret = _edje_object_part_text_raw_set(ed, obj, rp, part, eina_strbuf_string_get(sbuf));
        _edje_user_define_string(ed, part, rp->typedata.text->text, EDJE_TEXT_TYPE_ESCAPED);
        eina_strbuf_free(sbuf);
        return int_ret;
     }
   int_ret = _edje_object_part_text_raw_set(ed, obj, rp, part, text);
   _edje_user_define_string(ed, part, rp->typedata.text->text, EDJE_TEXT_TYPE_ESCAPED);

   return int_ret;
}

/**
 * @internal
 * @brief Escapes special characters in a text string for Evas Textblock.
 *
 * Converts characters like '&', '<', '>', etc., into their Evas Textblock
 * escaped equivalents (e.g., "&amp;", "&lt;", "&gt;").
 *
 * @param text The input string with plain characters.
 * @return A newly allocated string with characters escaped, or @c NULL if input is @c NULL.
 *         The caller is responsible for freeing the returned string.
 */
char *
_edje_text_escape(const char *text)
{
   Eina_Strbuf *txt;
   char *ret;
   const char *text_end;
   size_t text_len;

   if (!text) return NULL;

   txt = eina_strbuf_new();
   text_len = strlen(text);

   text_end = text + text_len;
   while (text < text_end)
     {
        int advance;
        const char *escaped = evas_textblock_string_escape_get(text, &advance);
        if (!escaped)
          {
             eina_strbuf_append_char(txt, text[0]);
             advance = 1;
          }
        else
          eina_strbuf_append(txt, escaped);

        text += advance;
     }

   ret = eina_strbuf_string_steal(txt);
   eina_strbuf_free(txt);
   return ret;
}

/**
 * @internal
 * @brief Unescapes Evas Textblock special sequences in a text string.
 *
 * Converts Evas Textblock escaped sequences (e.g., "&amp;", "&lt;") back
 * to their literal characters ('&', '<').
 *
 * @param text The input string with Evas Textblock escaped sequences.
 * @return A newly allocated string with sequences unescaped, or @c NULL if input is @c NULL.
 *         The caller is responsible for freeing the returned string.
 */
char *
_edje_text_unescape(const char *text)
{
   Eina_Strbuf *txt;
   char *ret;
   const char *text_end, *last, *escape_start;
   size_t text_len;

   if (!text) return NULL;

   txt = eina_strbuf_new();
   text_len = strlen(text);

   text_end = text + text_len;
   last = text;
   escape_start = NULL;
   for (; text < text_end; text++)
     {
        if (*text == '&')
          {
             size_t len;
             const char *str;

             if (last)
               {
                  len = text - last;
                  str = last;
               }
             else
               {
                  len = text - escape_start;
                  str = escape_start;
               }

             if (len > 0)
               eina_strbuf_append_n(txt, str, len);

             escape_start = text;
             last = NULL;
          }
        else if ((*text == ';') && (escape_start))
          {
             size_t len;
             const char *str = evas_textblock_escape_string_range_get(escape_start, text);

             if (str)
               len = strlen(str);
             else
               {
                  str = escape_start;
                  len = text + 1 - escape_start;
               }

             eina_strbuf_append_n(txt, str, len);

             escape_start = NULL;
             last = text + 1;
          }
     }

   if (!last && escape_start)
     last = escape_start;

   if (last && (text > last))
     {
        size_t len = text - last;
        eina_strbuf_append_n(txt, last, len);
     }

   ret = eina_strbuf_string_steal(txt);
   eina_strbuf_free(txt);
   return ret;
}

/**
 * @brief Sets the text of a part, escaping special characters in the input text.
 *
 * For TEXT parts, the input `text_to_escape` is set directly (no escaping).
 * For TEXTBLOCK parts, special characters in `text_to_escape` (like '&', '<')
 * are converted to their Evas Textblock escaped equivalents (e.g., "&amp;", "&lt;")
 * before being set as the part's markup.
 *
 * @param obj The Edje object.
 * @param part The name of the TEXT or TEXTBLOCK part.
 * @param text_to_escape The plain text to set. If the part is a TEXTBLOCK,
 *                       this text will be escaped.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
EAPI Eina_Bool
edje_object_part_text_unescaped_set(Eo *obj, const char *part, const char *text_to_escape)
{
   Edje_Real_Part *rp;
   Eina_Bool int_ret = EINA_FALSE;
   Edje *ed;
   EINA_SAFETY_ON_NULL_RETURN_VAL(part, EINA_FALSE);

   ed = _edje_fetch(obj);

   if (!ed) return EINA_FALSE;
   rp = _edje_real_part_recursive_get(&ed, part);
   if (!rp) return EINA_FALSE;
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return EINA_FALSE;
   if (rp->part->type == EDJE_PART_TYPE_TEXT)
     int_ret = _edje_object_part_text_raw_set(ed, obj, rp, part, text_to_escape);
   else if (rp->part->type == EDJE_PART_TYPE_TEXTBLOCK)
     {
        char *text = _edje_text_escape(text_to_escape);

        int_ret = _edje_object_part_text_raw_set(ed, obj, rp, part, text);
        free(text);
     }
   _edje_user_define_string(ed, part, rp->typedata.text->text, EDJE_TEXT_TYPE_UNESCAPED);

   return int_ret;
}

/**
 * @brief Gets the unescaped (plain) text from a part.
 *
 * For TEXT parts, this returns the direct string content.
 * For TEXTBLOCK parts, this retrieves the markup and then unescapes any
 * Evas Textblock escape sequences (e.g., "&amp;" becomes '&').
 * For editable entries, it gets the current entry text and unescapes it.
 *
 * @param obj The Edje object.
 * @param part The name of the TEXT or TEXTBLOCK part.
 * @return A newly allocated string containing the unescaped text, or @c NULL on error.
 *         The caller is responsible for freeing the returned string.
 */
EAPI char *
edje_object_part_text_unescaped_get(const Eo *obj, const char *part)
{
   Edje_Real_Part *rp;
   Edje *ed;
   EINA_SAFETY_ON_NULL_RETURN_VAL(part, NULL);

   ed = _edje_fetch(obj);

   if (!ed) return NULL;

   /* Need to recalc before providing the object. */
   _edje_recalc_do(ed);

   rp = _edje_real_part_recursive_get(&ed, part);
   if (!rp) return NULL;
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return NULL;
   if (rp->part->entry_mode > EDJE_ENTRY_EDIT_MODE_NONE)
     {
        const char *t = _edje_entry_text_get(rp);
        return _edje_text_unescape(t);
     }
   else
     {
        if (rp->part->type == EDJE_PART_TYPE_TEXT)
          {
             return strdup(rp->typedata.text->text);
          }
        if (rp->part->type == EDJE_PART_TYPE_TEXTBLOCK)
          {
             const char *t = evas_object_textblock_text_markup_get(rp->object);
             return _edje_text_unescape(t);
          }
     }

   return NULL;
}

/**
 * @internal
 * @brief Inserts markup text into an editable textblock part.
 *
 * This is an internal helper function. It only operates on TEXTBLOCK parts
 * that are in an entry mode.
 *
 * @param ed The Edje private data.
 * @param rp The Edje_Real_Part corresponding to the textblock part.
 * @param text The markup text to insert.
 */
void
_edje_object_part_text_insert(Edje *ed, Edje_Real_Part *rp, const char *text)
{
   if (!rp) return;
   if ((rp->part->type != EDJE_PART_TYPE_TEXTBLOCK)) return;
   if (rp->part->entry_mode <= EDJE_ENTRY_EDIT_MODE_NONE) return;
   _edje_entry_text_markup_insert(rp, text);
   ed->dirty = EINA_TRUE;
   ed->recalc_call = EINA_TRUE;
   ed->recalc_hints = EINA_TRUE;
#ifdef EDJE_CALC_CACHE
   rp->invalidate = EINA_TRUE;
#endif
   _edje_recalc(ed);
}

/**
 * @brief Inserts markup text into an editable TEXTBLOCK part at the current cursor position.
 *
 * This function is for TEXTBLOCK parts that are configured as entries (editable).
 *
 * @param obj The Edje object.
 * @param part The name of the editable TEXTBLOCK part.
 * @param text The markup text to insert.
 */
EAPI void
edje_object_part_text_insert(Eo *obj, const char *part, const char *text)
{
   Edje_Real_Part *rp;
   Edje *ed;
   EINA_SAFETY_ON_NULL_RETURN(part);

   ed = _edje_fetch(obj);

   if (!ed) return;
   rp = _edje_real_part_recursive_get(&ed, part);
   _edje_object_part_text_insert(ed, rp, text);
   if (ed->text_change.func)
     ed->text_change.func(ed->text_change.data, obj, part);
}

/* Calc interface APIs */

/**
 * @brief Enables or disables automatic update of layout hints.
 *
 * If enabled, Edje will automatically update evas hints when the object is resized or recalculated.
 *
 * @param obj The Edje object.
 * @param update @c EINA_TRUE to enable auto-update, @c EINA_FALSE to disable.
 */
EAPI void
edje_object_update_hints_set(Edje_Object *obj, Eina_Bool update)
{
   efl_layout_calc_auto_update_hints_set(obj, update);
}

/**
 * @brief Gets whether automatic update of layout hints is enabled.
 *
 * @param obj The Edje object.
 * @return @c EINA_TRUE if auto-update is enabled, @c EINA_FALSE otherwise.
 */
EAPI Eina_Bool
edje_object_update_hints_get(const Edje_Object *obj)
{
   return efl_layout_calc_auto_update_hints_get(obj);
}

/**
 * @brief Calculates the minimum size of an Edje object.
 *
 * This is equivalent to calling edje_object_size_min_restricted_calc()
 * with restricted dimensions set to 0.
 *
 * @param obj The Edje object.
 * @param[out] minw Pointer to store the minimum width.
 * @param[out] minh Pointer to store the minimum height.
 */
EAPI void
edje_object_size_min_calc(Edje_Object *obj, int *minw, int *minh)
{
   edje_object_size_min_restricted_calc(obj, minw, minh, 0, 0);
}

/**
 * @brief Calculates the minimum size of an Edje object, possibly restricted.
 *
 * This function calculates the minimum width and height the Edje object
 * needs to display its content, optionally considering a restricted size.
 *
 * @param obj The Edje object.
 * @param[out] minw Pointer to store the calculated minimum width.
 * @param[out] minh Pointer to store the calculated minimum height.
 * @param restrictedw A width to restrict the calculation against.
 * @param restrictedh A height to restrict the calculation against.
 */
EAPI void
edje_object_size_min_restricted_calc(Edje_Object *obj, int *minw, int *minh, int restrictedw, int restrictedh)
{
   Eina_Size2D sz = { restrictedw, restrictedh };
   Edje *ed;

   ed = _edje_fetch(obj);
   if (!ed)
     {
        if (minw) *minw = sz.w;
        if (minh) *minh = sz.h;
        return;
     }
   sz = efl_layout_calc_size_min(obj, EINA_SIZE2D(restrictedw, restrictedh));
   if (minw) *minw = sz.w;
   if (minh) *minh = sz.h;
}

/**
 * @brief Calculates the rectangle that encompasses all parts of an Edje object.
 *
 * This function determines the bounding box (x, y, width, height) that
 * contains all visible parts of the Edje object.
 *
 * @param obj The Edje object.
 * @param[out] x Pointer to store the x-coordinate of the bounding box.
 * @param[out] y Pointer to store the y-coordinate of the bounding box.
 * @param[out] w Pointer to store the width of the bounding box.
 * @param[out] h Pointer to store the height of the bounding box.
 * @return @c EINA_TRUE if the calculation was successful (object was valid),
 *         @c EINA_FALSE otherwise.
 */
EAPI Eina_Bool
edje_object_parts_extends_calc(Edje_Object *obj, int *x, int *y, int *w, int *h)
{
   Eina_Rect r = EINA_RECT_ZERO();
   Edje *ed;

   ed = _edje_fetch(obj);
   if (ed) r = efl_layout_calc_parts_extends(obj);
   if (x) *x = r.x;
   if (y) *y = r.y;
   if (w) *w = r.w;
   if (h) *h = r.h;
   return (ed != NULL);
}

/**
 * @brief Freezes the calculation state of an Edje object.
 *
 * When frozen, the Edje object will not recalculate its layout, even if
 * changes occur that would normally trigger a recalc. This can be used
 * to batch multiple changes for performance.
 *
 * @param obj The Edje object.
 * @return The new freeze count. Each call to freeze increments the count.
 * @see edje_object_thaw()
 */
EAPI int
edje_object_freeze(Edje_Object *obj)
{
   return efl_layout_calc_freeze(obj);
}

/**
 * @brief Thaws the calculation state of an Edje object.
 *
 * Decrements the freeze count. If the count reaches zero, the object
 * will resume normal recalculation behavior and may trigger an immediate
 * recalculation if changes were made while frozen.
 *
 * @param obj The Edje object.
 * @return The new freeze count.
 * @see edje_object_freeze()
 */
EAPI int
edje_object_thaw(Edje_Object *obj)
{
   return efl_layout_calc_thaw(obj);
}

/**
 * @brief Forces an immediate recalculation of the Edje object's layout.
 *
 * This function bypasses the freeze state and forces a recalc.
 *
 * @param obj The Edje object.
 */
EAPI void
edje_object_calc_force(Edje_Object *obj)
{
   efl_layout_calc_force(obj);
}

/**
 * @brief Sets the playback state of animations within an Edje object.
 *
 * @param obj The Edje object.
 * @param play @c EINA_TRUE to play animations, @c EINA_FALSE to pause.
 */
EAPI void
edje_object_play_set(Evas_Object *obj, Eina_Bool play)
{
   efl_player_paused_set(obj, !play);
}

/**
 * @brief Gets the playback state of animations within an Edje object.
 *
 * @param obj The Edje object.
 * @return @c EINA_TRUE if animations are playing, @c EINA_FALSE if paused or if obj is not an Edje object.
 */
EAPI Eina_Bool
edje_object_play_get(const Evas_Object *obj)
{
   if (!efl_isa(obj, EFL_CANVAS_LAYOUT_CLASS)) return EINA_FALSE;
   return !efl_player_paused_get(obj);
}

/**
 * @brief Sets the speed factor for transitions (animations) in an Edje object.
 *
 * A scale of 1.0 is normal speed. Values > 1.0 slow down transitions
 * (duration increases), and values < 1.0 speed them up (duration decreases).
 * The input `scale` is effectively the factor by which durations are multiplied.
 *
 * @param obj The Edje object.
 * @param scale The duration scaling factor. Must be > 0.0.
 *              Example: scale = 2.0 means transitions take twice as long.
 *                       scale = 0.5 means transitions take half as long.
 */
EAPI void
edje_object_transition_duration_factor_set(Evas_Object *obj, double scale)
{
   if (scale <= 0.0) return;
   efl_player_playback_speed_set(obj, 1.0/scale);
}

/**
 * @brief Gets the speed factor for transitions (animations) in an Edje object.
 *
 * @param obj The Edje object.
 * @return The current duration scaling factor.
 *         Returns 1.0 if the internal playback speed is invalid (<= 0.0).
 */
EAPI double
edje_object_transition_duration_factor_get(const Evas_Object *obj)
{
   double speed = efl_player_playback_speed_get(obj);

   if (speed <= 0.0) speed = 1.0;
   return 1.0/speed;
}

/**
 * @brief Gets the minimum size of the Edje object as set by its group definition.
 *
 * This refers to the `min` property in the EDC group definition.
 *
 * @param obj The Edje object.
 * @param[out] minw Pointer to store the minimum width.
 * @param[out] minh Pointer to store the minimum height.
 */
EAPI void
edje_object_size_min_get(const Edje_Object *obj, int *minw, int *minh)
{
   Eina_Size2D sz;
   sz = efl_layout_group_size_min_get(obj);
   if (minw) *minw = sz.w;
   if (minh) *minh = sz.h;
}

/**
 * @brief Gets the maximum size of the Edje object as set by its group definition.
 *
 * This refers to the `max` property in the EDC group definition.
 *
 * @param obj The Edje object.
 * @param[out] maxw Pointer to store the maximum width.
 * @param[out] maxh Pointer to store the maximum height.
 */
EAPI void
edje_object_size_max_get(const Edje_Object *obj, int *maxw, int *maxh)
{
   Eina_Size2D sz;
   sz = efl_layout_group_size_max_get(obj);
   if (maxw) *maxw = sz.w;
   if (maxh) *maxh = sz.h;
}

/**
 * @brief Checks if a part with the given name exists in the Edje object's definition.
 *
 * @param obj The Edje object.
 * @param part The name of the part to check.
 * @return @c EINA_TRUE if the part exists, @c EINA_FALSE otherwise.
 */
EAPI Eina_Bool
edje_object_part_exists(const Eo *obj, const char *part)
{
   return efl_layout_group_part_exist_get(obj, part);
}
