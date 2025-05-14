
/**
 * @internal
 * @brief Legacy wrapper for elm_obj_glview_resize_policy_set().
 * @param[in] obj The object.
 * @param[in] policy The resize policy. See #Elm_GLView_Resize_Policy for details.
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise.
 * @ingroup Elm_Glview_Group
 * @see elm_glview_resize_policy_set() in elm_glview_eo.legacy.h for more details.
 */
EAPI Eina_Bool
elm_glview_resize_policy_set(Elm_Glview *obj, Elm_GLView_Resize_Policy policy)
{
   return elm_obj_glview_resize_policy_set(obj, policy);
}

/**
 * @internal
 * @brief Legacy wrapper for elm_obj_glview_render_policy_set().
 * @param[in] obj The object.
 * @param[in] policy The render policy. See #Elm_GLView_Render_Policy for details.
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise.
 * @ingroup Elm_Glview_Group
 * @see elm_glview_render_policy_set() in elm_glview_eo.legacy.h for more details.
 */
EAPI Eina_Bool
elm_glview_render_policy_set(Elm_Glview *obj, Elm_GLView_Render_Policy policy)
{
   return elm_obj_glview_render_policy_set(obj, policy);
}

/**
 * @internal
 * @brief Legacy wrapper for elm_obj_glview_mode_set().
 * @param[in] obj The object.
 * @param[in] mode The GLView mode. See #Elm_GLView_Mode for details.
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise.
 * @ingroup Elm_Glview_Group
 * @see elm_glview_mode_set() in elm_glview_eo.legacy.h for more details.
 */
EAPI Eina_Bool
elm_glview_mode_set(Elm_Glview *obj, Elm_GLView_Mode mode)
{
   return elm_obj_glview_mode_set(obj, mode);
}

/**
 * @internal
 * @brief Legacy wrapper for elm_obj_glview_gl_api_get().
 * @param[in] obj The object.
 * @return The Evas GL API structure, or @c NULL on failure.
 * @ingroup Elm_Glview_Group
 * @see elm_glview_gl_api_get() in elm_glview_eo.legacy.h for more details.
 */
EAPI Evas_GL_API *
elm_glview_gl_api_get(const Elm_Glview *obj)
{
   return elm_obj_glview_gl_api_get(obj);
}

/**
 * @internal
 * @brief Legacy wrapper for elm_obj_glview_evas_gl_get().
 * @param[in] obj The object.
 * @return The Evas_GL instance, or @c NULL on failure.
 * @ingroup Elm_Glview_Group
 * @see elm_glview_evas_gl_get() in elm_glview_eo.legacy.h for more details.
 */
EAPI Evas_GL *
elm_glview_evas_gl_get(const Elm_Glview *obj)
{
   return elm_obj_glview_evas_gl_get(obj);
}

/**
 * @internal
 * @brief Legacy wrapper for elm_obj_glview_rotation_get().
 * @param[in] obj The object.
 * @return The rotation in degrees (0, 90, 180, or 270).
 * @ingroup Elm_Glview_Group
 * @see elm_glview_rotation_get() in elm_glview_eo.legacy.h for more details.
 */
EAPI int
elm_glview_rotation_get(const Elm_Glview *obj)
{
   return elm_obj_glview_rotation_get(obj);
}
