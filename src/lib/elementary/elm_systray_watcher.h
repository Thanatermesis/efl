#ifndef ELM_SYSTRAY_WATCHER_H
#define ELM_SYSTRAY_WATCHER_H

/**
 * @brief Registers a StatusNotifierItem with the StatusNotifierWatcher.
 * @param obj The D-Bus object path of the StatusNotifierItem to register.
 *            Example: "/org/ayatana/NotificationItem/my_app_indicator"
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
Eina_Bool _elm_systray_watcher_status_notifier_item_register(const char *obj);

/**
 * @brief Initializes the StatusNotifierWatcher system.
 * This function sets up the D-Bus connection and registers for
 * name owner changes of the StatusNotifierWatcher service.
 * @return EINA_TRUE on success or if already initialized, EINA_FALSE on failure.
 */
Eina_Bool _elm_systray_watcher_init(void);

/**
 * @brief Shuts down the StatusNotifierWatcher system.
 * Releases D-Bus resources and cleans up.
 */
void _elm_systray_watcher_shutdown(void);

#endif
