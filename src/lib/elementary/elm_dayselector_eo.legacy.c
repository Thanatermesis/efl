
/**
 * @brief Set the starting day of Dayselector.
 *
 * See also @ref Elm_Dayselector_Day, @ref elm_dayselector_week_start_get.
 *
 * @param[in] obj The object.
 * @param[in] day Dayselector_Day the first day that the user wants to display.
 *
 * @ingroup Elm_Dayselector_Group
 */
EAPI void
elm_dayselector_week_start_set(Elm_Dayselector *obj, Elm_Dayselector_Day day)
{
   elm_obj_dayselector_week_start_set(obj, day);
}

/**
 * @brief Get the starting day of Dayselector.
 *
 * See also @ref Elm_Dayselector_Day, @ref elm_dayselector_week_start_set.
 *
 * @param[in] obj The object.
 *
 * @return Dayselector_Day the first day that the user wants to display.
 *
 * @ingroup Elm_Dayselector_Group
 */
EAPI Elm_Dayselector_Day
elm_dayselector_week_start_get(const Elm_Dayselector *obj)
{
   return elm_obj_dayselector_week_start_get(obj);
}

/**
 * @brief Set the weekend length of Dayselector.
 *
 * See also @ref elm_dayselector_weekend_length_get.
 *
 * @param[in] obj The object.
 * @param[in] length Weekend length, number of days as an integer.
 *
 * @ingroup Elm_Dayselector_Group
 */
EAPI void
elm_dayselector_weekend_length_set(Elm_Dayselector *obj, unsigned int length)
{
   elm_obj_dayselector_weekend_length_set(obj, length);
}

/**
 * @brief Get the weekend length of Dayselector.
 *
 * See also @ref Elm_Dayselector_Day, @ref elm_dayselector_weekend_length_set.
 *
 * @param[in] obj The object.
 *
 * @return Weekend length, number of days as an integer.
 *
 * @ingroup Elm_Dayselector_Group
 */
EAPI unsigned int
elm_dayselector_weekend_length_get(const Elm_Dayselector *obj)
{
   return elm_obj_dayselector_weekend_length_get(obj);
}

/**
 * @brief Set the weekend starting day of Dayselector.
 *
 * See also @ref Elm_Dayselector_Day, @ref elm_dayselector_weekend_start_get.
 *
 * @param[in] obj The object.
 * @param[in] day Dayselector_Day the first day from where weekend starts.
 *
 * @ingroup Elm_Dayselector_Group
 */
EAPI void
elm_dayselector_weekend_start_set(Elm_Dayselector *obj, Elm_Dayselector_Day day)
{
   elm_obj_dayselector_weekend_start_set(obj, day);
}

/**
 * @brief Get the weekend starting day of Dayselector.
 *
 * See also @ref Elm_Dayselector_Day, @ref elm_dayselector_weekend_start_set.
 *
 * @param[in] obj The object.
 *
 * @return Dayselector_Day the first day from where weekend starts.
 *
 * @ingroup Elm_Dayselector_Group
 */
EAPI Elm_Dayselector_Day
elm_dayselector_weekend_start_get(const Elm_Dayselector *obj)
{
   return elm_obj_dayselector_weekend_start_get(obj);
}

/**
 * @brief Set weekdays names to be displayed by the Dayselector.
 *
 * By default or if weekdays is @c null, weekdays abbreviations get from system
 * are displayed: E.g. for an en_US locale: "Sun, Mon, Tue, Wed, Thu, Fri, Sat"
 *
 * The first string should be related to Sunday, the second to Monday...
 *
 * See also @ref elm_dayselector_weekdays_names_get,
 * @ref elm_dayselector_weekend_start_set.
 *
 * @param[in] obj The object.
 * @param[in] weekdays Array of seven strings to be used as weekday names.
 * Warning: It must have 7 elements, or it will access invalid memory. Warning:
 * The strings must be NULL terminated ('@\0').
 *
 * @since 1.8
 *
 * @ingroup Elm_Dayselector_Group
 */
EAPI void
elm_dayselector_weekdays_names_set(Elm_Dayselector *obj, const char **weekdays)
{
   elm_obj_dayselector_weekdays_names_set(obj, weekdays);
}

/**
 * @brief Get weekdays names displayed by the calendar.
 *
 * By default, weekdays abbreviations get from system are displayed: E.g. for
 * an en_US locale: "Sun, Mon, Tue, Wed, Thu, Fri, Sat" The first string is
 * related to Sunday, the second to Monday...
 *
 * See also @ref elm_dayselector_weekdays_names_set.
 *
 * @param[in] obj The object.
 *
 * @return A list of seven strings to be used as weekday names.
 *
 * @since 1.8
 *
 * @ingroup Elm_Dayselector_Group
 */
EAPI Eina_List *
elm_dayselector_weekdays_names_get(const Elm_Dayselector *obj)
{
   return elm_obj_dayselector_weekdays_names_get(obj);
}

/**
 * @brief Set the state of given Dayselector_Day.
 *
 * See also @ref Elm_Dayselector_Day, @ref elm_dayselector_day_selected_get.
 *
 * @param[in] obj The object.
 * @param[in] day Dayselector_Day that the user want to set state.
 * @param[in] selected State of the day. @c true is selected.
 *
 * @ingroup Elm_Dayselector_Group
 */
EAPI void
elm_dayselector_day_selected_set(Elm_Dayselector *obj, Elm_Dayselector_Day day, Eina_Bool selected)
{
   elm_obj_dayselector_day_selected_set(obj, day, selected);
}

/**
 * @brief Get the state of given Dayselector_Day.
 *
 * See also @ref Elm_Dayselector_Day, @ref elm_dayselector_day_selected_set.
 *
 * @param[in] obj The object.
 * @param[in] day Dayselector_Day that the user want to know state.
 *
 * @return @c true on success, @c false on failure
 *
 * @ingroup Elm_Dayselector_Group
 */
EAPI Eina_Bool
elm_dayselector_day_selected_get(const Elm_Dayselector *obj, Elm_Dayselector_Day day)
{
   return elm_obj_dayselector_day_selected_get(obj, day);
}
