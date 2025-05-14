
/**
 * @brief Implements the EAPI function elm_notify_align_set().
 * @see elm_notify_align_set() in elm_notify_eo.legacy.h for detailed documentation.
 */
EAPI void
elm_notify_align_set(Elm_Notify *obj, double horizontal, double vertical)
{
   elm_obj_notify_align_set(obj, horizontal, vertical);
}

/**
 * @brief Implements the EAPI function elm_notify_align_get().
 * @see elm_notify_align_get() in elm_notify_eo.legacy.h for detailed documentation.
 */
EAPI void
elm_notify_align_get(const Elm_Notify *obj, double *horizontal, double *vertical)
{
   elm_obj_notify_align_get(obj, horizontal, vertical);
}

/**
 * @brief Implements the EAPI function elm_notify_allow_events_set().
 * @see elm_notify_allow_events_set() in elm_notify_eo.legacy.h for detailed documentation.
 */
EAPI void
elm_notify_allow_events_set(Elm_Notify *obj, Eina_Bool allow)
{
   elm_obj_notify_allow_events_set(obj, allow);
}

/**
 * @brief Implements the EAPI function elm_notify_allow_events_get().
 * @see elm_notify_allow_events_get() in elm_notify_eo.legacy.h for detailed documentation.
 */
EAPI Eina_Bool
elm_notify_allow_events_get(const Elm_Notify *obj)
{
   return elm_obj_notify_allow_events_get(obj);
}

/**
 * @brief Implements the EAPI function elm_notify_timeout_set().
 * @see elm_notify_timeout_set() in elm_notify_eo.legacy.h for detailed documentation.
 */
EAPI void
elm_notify_timeout_set(Elm_Notify *obj, double timeout)
{
   elm_obj_notify_timeout_set(obj, timeout);
}

/**
 * @brief Implements the EAPI function elm_notify_timeout_get().
 * @see elm_notify_timeout_get() in elm_notify_eo.legacy.h for detailed documentation.
 */
EAPI double
elm_notify_timeout_get(const Elm_Notify *obj)
{
   return elm_obj_notify_timeout_get(obj);
}

/**
 * @brief Implements the EAPI function elm_notify_dismiss().
 * @see elm_notify_dismiss() in elm_notify_eo.legacy.h for detailed documentation.
 */
EAPI void
elm_notify_dismiss(Elm_Notify *obj)
{
   elm_obj_notify_dismiss(obj);
}
