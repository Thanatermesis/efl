/**
 * @brief Sets the corner of the bubble.
 * @see elm_bubble_pos_set() in elm_bubble_eo.legacy.h for detailed documentation.
 */
EAPI void
elm_bubble_pos_set(Elm_Bubble *obj, Elm_Bubble_Pos pos)
{
   elm_obj_bubble_pos_set(obj, pos);
}

/**
 * @brief Gets the corner of the bubble.
 * @see elm_bubble_pos_get() in elm_bubble_eo.legacy.h for detailed documentation.
 */
EAPI Elm_Bubble_Pos
elm_bubble_pos_get(const Elm_Bubble *obj)
{
   return elm_obj_bubble_pos_get(obj);
}
