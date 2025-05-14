
/**
 * @internal
 * @brief Implements the legacy C function elm_clock_show_am_pm_set().
 * @details This function calls the corresponding Evas Object smart function.
 *          For detailed API documentation, see elm_clock_show_am_pm_set() in elm_clock_eo.legacy.h.
 */
EAPI void
elm_clock_show_am_pm_set(Elm_Clock *obj, Eina_Bool am_pm)
{
   elm_obj_clock_show_am_pm_set(obj, am_pm);
}

/**
 * @internal
 * @brief Implements the legacy C function elm_clock_show_am_pm_get().
 * @details This function calls the corresponding Evas Object smart function.
 *          For detailed API documentation, see elm_clock_show_am_pm_get() in elm_clock_eo.legacy.h.
 */
EAPI Eina_Bool
elm_clock_show_am_pm_get(const Elm_Clock *obj)
{
   return elm_obj_clock_show_am_pm_get(obj);
}

/**
 * @internal
 * @brief Implements the legacy C function elm_clock_first_interval_set().
 * @details This function calls the corresponding Evas Object smart function.
 *          For detailed API documentation, see elm_clock_first_interval_set() in elm_clock_eo.legacy.h.
 */
EAPI void
elm_clock_first_interval_set(Elm_Clock *obj, double interval)
{
   elm_obj_clock_first_interval_set(obj, interval);
}

/**
 * @internal
 * @brief Implements the legacy C function elm_clock_first_interval_get().
 * @details This function calls the corresponding Evas Object smart function.
 *          For detailed API documentation, see elm_clock_first_interval_get() in elm_clock_eo.legacy.h.
 */
EAPI double
elm_clock_first_interval_get(const Elm_Clock *obj)
{
   return elm_obj_clock_first_interval_get(obj);
}

/**
 * @internal
 * @brief Implements the legacy C function elm_clock_show_seconds_set().
 * @details This function calls the corresponding Evas Object smart function.
 *          For detailed API documentation, see elm_clock_show_seconds_set() in elm_clock_eo.legacy.h.
 */
EAPI void
elm_clock_show_seconds_set(Elm_Clock *obj, Eina_Bool seconds)
{
   elm_obj_clock_show_seconds_set(obj, seconds);
}

/**
 * @internal
 * @brief Implements the legacy C function elm_clock_show_seconds_get().
 * @details This function calls the corresponding Evas Object smart function.
 *          For detailed API documentation, see elm_clock_show_seconds_get() in elm_clock_eo.legacy.h.
 */
EAPI Eina_Bool
elm_clock_show_seconds_get(const Elm_Clock *obj)
{
   return elm_obj_clock_show_seconds_get(obj);
}

/**
 * @internal
 * @brief Implements the legacy C function elm_clock_edit_set().
 * @details This function calls the corresponding Evas Object smart function.
 *          For detailed API documentation, see elm_clock_edit_set() in elm_clock_eo.legacy.h.
 */
EAPI void
elm_clock_edit_set(Elm_Clock *obj, Eina_Bool edit)
{
   elm_obj_clock_edit_set(obj, edit);
}

/**
 * @internal
 * @brief Implements the legacy C function elm_clock_edit_get().
 * @details This function calls the corresponding Evas Object smart function.
 *          For detailed API documentation, see elm_clock_edit_get() in elm_clock_eo.legacy.h.
 */
EAPI Eina_Bool
elm_clock_edit_get(const Elm_Clock *obj)
{
   return elm_obj_clock_edit_get(obj);
}

/**
 * @internal
 * @brief Implements the legacy C function elm_clock_pause_set().
 * @details This function calls the corresponding Evas Object smart function.
 *          For detailed API documentation, see elm_clock_pause_set() in elm_clock_eo.legacy.h.
 */
EAPI void
elm_clock_pause_set(Elm_Clock *obj, Eina_Bool paused)
{
   elm_obj_clock_pause_set(obj, paused);
}

/**
 * @internal
 * @brief Implements the legacy C function elm_clock_pause_get().
 * @details This function calls the corresponding Evas Object smart function.
 *          For detailed API documentation, see elm_clock_pause_get() in elm_clock_eo.legacy.h.
 */
EAPI Eina_Bool
elm_clock_pause_get(const Elm_Clock *obj)
{
   return elm_obj_clock_pause_get(obj);
}

/**
 * @internal
 * @brief Implements the legacy C function elm_clock_time_set().
 * @details This function calls the corresponding Evas Object smart function.
 *          For detailed API documentation, see elm_clock_time_set() in elm_clock_eo.legacy.h.
 */
EAPI void
elm_clock_time_set(Elm_Clock *obj, int hrs, int min, int sec)
{
   elm_obj_clock_time_set(obj, hrs, min, sec);
}

/**
 * @internal
 * @brief Implements the legacy C function elm_clock_time_get().
 * @details This function calls the corresponding Evas Object smart function.
 *          For detailed API documentation, see elm_clock_time_get() in elm_clock_eo.legacy.h.
 */
EAPI void
elm_clock_time_get(const Elm_Clock *obj, int *hrs, int *min, int *sec)
{
   elm_obj_clock_time_get(obj, hrs, min, sec);
}

/**
 * @internal
 * @brief Implements the legacy C function elm_clock_edit_mode_set().
 * @details This function calls the corresponding Evas Object smart function.
 *          For detailed API documentation, see elm_clock_edit_mode_set() in elm_clock_eo.legacy.h.
 */
EAPI void
elm_clock_edit_mode_set(Elm_Clock *obj, Elm_Clock_Edit_Mode digedit)
{
   elm_obj_clock_edit_mode_set(obj, digedit);
}

/**
 * @internal
 * @brief Implements the legacy C function elm_clock_edit_mode_get().
 * @details This function calls the corresponding Evas Object smart function.
 *          For detailed API documentation, see elm_clock_edit_mode_get() in elm_clock_eo.legacy.h.
 */
EAPI Elm_Clock_Edit_Mode
elm_clock_edit_mode_get(const Elm_Clock *obj)
{
   return elm_obj_clock_edit_mode_get(obj);
}
