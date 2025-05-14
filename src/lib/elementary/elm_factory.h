/* this api is currently disabled due to being incomplete. you cannot
 * use it as it is not compiled in */

/**
 * @brief Adds a new factory widget to the given parent Elementary (container) object.
 *
 * @param parent The parent object.
 * @return The new object or NULL if it cannot be created.
 * @ingroup Elm_Factory
 */
EAPI Evas_Object                 *elm_factory_add(Evas_Object *parent);

/**
 * @brief Enables or disables the max/min sizing mode for the factory.
 *
 * When enabled, the factory will try to keep track of the largest minimum
 * size hinted by its content over time and use that as its own minimum size.
 *
 * @param obj The factory object.
 * @param enabled EINA_TRUE to enable max/min mode, EINA_FALSE to disable.
 * @ingroup Elm_Factory
 */
EAPI void                         elm_factory_maxmin_mode_set(Evas_Object *obj, Eina_Bool enabled);

/**
 * @brief Gets the current state of the max/min sizing mode for the factory.
 *
 * @param obj The factory object.
 * @return EINA_TRUE if max/min mode is enabled, EINA_FALSE otherwise.
 * @ingroup Elm_Factory
 */
EAPI Eina_Bool                    elm_factory_maxmin_mode_get(const Evas_Object *obj);

/**
 * @brief Resets the remembered maximum minimum size when max/min mode is enabled.
 *
 * This function will reset the stored maximum width and height to 0,
 * effectively allowing the factory to shrink if its content's minimum
 * size hints decrease.
 *
 * @param obj The factory object.
 * @ingroup Elm_Factory
 */
EAPI void                         elm_factory_maxmin_reset_set(Evas_Object *obj);
