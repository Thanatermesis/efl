/**
 * @internal
 * @brief Layout the child objects of a box.
 *
 * This function calculates the position and size of each child object within the
 * box based on the box's properties (horizontal/vertical, homogeneous/non-homogeneous,
 * right-to-left/left-to-right) and the size hints of the child objects.
 *
 * @param o The box Evas_Object.
 * @param priv The private data of the box object.
 * @param horizontal EINA_TRUE if the layout is horizontal, EINA_FALSE for vertical.
 * @param homogeneous EINA_TRUE if all children should have the same size in the
 *        layout direction, EINA_FALSE otherwise.
 * @param rtl EINA_TRUE if right-to-left layout is enabled, EINA_FALSE otherwise.
 */
void _els_box_layout(Evas_Object *o, Evas_Object_Box_Data *priv, Eina_Bool horizontal, Eina_Bool homogeneous, Eina_Bool rtl);
