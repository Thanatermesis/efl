
/**
 * @brief Implements the legacy EAPI function elm_hover_target_set.
 *
 * This function forwards the call to elm_obj_hover_target_set.
 * For detailed documentation, see the declaration in elm_hover_eo.legacy.h.
 */
EAPI void
elm_hover_target_set(Elm_Hover *obj, Efl_Canvas_Object *target)
{
   elm_obj_hover_target_set(obj, target);
}

/**
 * @brief Implements the legacy EAPI function elm_hover_target_get.
 *
 * This function forwards the call to elm_obj_hover_target_get.
 * For detailed documentation, see the declaration in elm_hover_eo.legacy.h.
 */
EAPI Efl_Canvas_Object *
elm_hover_target_get(const Elm_Hover *obj)
{
   return elm_obj_hover_target_get(obj);
}

/**
 * @brief Implements the legacy EAPI function elm_hover_best_content_location_get.
 *
 * This function forwards the call to elm_obj_hover_best_content_location_get.
 * For detailed documentation, see the declaration in elm_hover_eo.legacy.h.
 */
EAPI const char *
elm_hover_best_content_location_get(const Elm_Hover *obj, Elm_Hover_Axis pref_axis)
{
   return elm_obj_hover_best_content_location_get(obj, pref_axis);
}

/**
 * @brief Implements the legacy EAPI function elm_hover_dismiss.
 *
 * This function forwards the call to elm_obj_hover_dismiss.
 * For detailed documentation, see the declaration in elm_hover_eo.legacy.h.
 */
EAPI void
elm_hover_dismiss(Elm_Hover *obj)
{
   elm_obj_hover_dismiss(obj);
}
