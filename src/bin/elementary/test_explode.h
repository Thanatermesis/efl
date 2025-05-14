/**
 * @brief Enables the explode effect capability for a given window.
 *
 * Creates an event catcher on the window that listens for Ctrl + Middle Mouse Button
 * clicks. When triggered, the widget under the mouse cursor will "explode".
 *
 * @param win The Evas_Object representing the window (e.g., an Elm_Win)
 *            for which to enable the explode effect.
 */
void explode_win_enable(Evas_Object *win);
