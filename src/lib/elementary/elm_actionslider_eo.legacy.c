/**
 * @brief Implements the EAPI function elm_actionslider_indicator_pos_set().
 * @see elm_actionslider_indicator_pos_set() in elm_actionslider_eo.legacy.h for details.
 */
EAPI void
elm_actionslider_indicator_pos_set(Elm_Actionslider *obj, Elm_Actionslider_Pos pos)
{
   elm_obj_actionslider_indicator_pos_set(obj, pos);
}

/**
 * @brief Implements the EAPI function elm_actionslider_indicator_pos_get().
 * @see elm_actionslider_indicator_pos_get() in elm_actionslider_eo.legacy.h for details.
 */
EAPI Elm_Actionslider_Pos
elm_actionslider_indicator_pos_get(const Elm_Actionslider *obj)
{
   return elm_obj_actionslider_indicator_pos_get(obj);
}

/**
 * @brief Implements the EAPI function elm_actionslider_magnet_pos_set().
 * @see elm_actionslider_magnet_pos_set() in elm_actionslider_eo.legacy.h for details.
 */
EAPI void
elm_actionslider_magnet_pos_set(Elm_Actionslider *obj, Elm_Actionslider_Pos pos)
{
   elm_obj_actionslider_magnet_pos_set(obj, pos);
}

/**
 * @brief Implements the EAPI function elm_actionslider_magnet_pos_get().
 * @see elm_actionslider_magnet_pos_get() in elm_actionslider_eo.legacy.h for details.
 */
EAPI Elm_Actionslider_Pos
elm_actionslider_magnet_pos_get(const Elm_Actionslider *obj)
{
   return elm_obj_actionslider_magnet_pos_get(obj);
}

/**
 * @brief Implements the EAPI function elm_actionslider_enabled_pos_set().
 * @see elm_actionslider_enabled_pos_set() in elm_actionslider_eo.legacy.h for details.
 */
EAPI void
elm_actionslider_enabled_pos_set(Elm_Actionslider *obj, Elm_Actionslider_Pos pos)
{
   elm_obj_actionslider_enabled_pos_set(obj, pos);
}

/**
 * @brief Implements the EAPI function elm_actionslider_enabled_pos_get().
 * @see elm_actionslider_enabled_pos_get() in elm_actionslider_eo.legacy.h for details.
 */
EAPI Elm_Actionslider_Pos
elm_actionslider_enabled_pos_get(const Elm_Actionslider *obj)
{
   return elm_obj_actionslider_enabled_pos_get(obj);
}

/**
 * @brief Implements the EAPI function elm_actionslider_selected_label_get().
 * @see elm_actionslider_selected_label_get() in elm_actionslider_eo.legacy.h for details.
 */
EAPI const char *
elm_actionslider_selected_label_get(const Elm_Actionslider *obj)
{
   return elm_obj_actionslider_selected_label_get(obj);
}
