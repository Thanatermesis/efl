/**
 * @brief Provides the C legacy API implementation for elm_win_inwin_activate().
 *
 * This function serves as a wrapper, calling the underlying
 * `elm_obj_win_inwin_activate()` function to perform the actual activation
 * of the in-window object. For detailed information on the behavior and
 * parameters, refer to the documentation of `elm_win_inwin_activate()`
 * in `elm_inwin_eo.legacy.h`.
 *
 * @param obj The Elm_Inwin object to be activated.
 * @see elm_win_inwin_activate() in elm_inwin_eo.legacy.h
 */
EAPI void
elm_win_inwin_activate(Elm_Inwin *obj)
{
   elm_obj_win_inwin_activate(obj);
}
