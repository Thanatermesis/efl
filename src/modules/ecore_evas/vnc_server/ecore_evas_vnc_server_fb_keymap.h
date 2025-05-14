#include <rfb/rfb.h>
#include <Eina.h>

#ifndef ECORE_EVAS_VNC_SERVER_FB_KEY_MAP_H
#define ECORE_EVAS_VNC_SERVER_FB_KEY_MAP_H

/**
 * @brief Translates an RFB (VNC) keysym to framebuffer key information.
 *
 * This function takes an RFB keysym, converts it to a Linux framebuffer key code,
 * and then looks up the corresponding key name, key string, and compose string.
 * This is used to map key events received over VNC to the appropriate key
 * representations for the underlying framebuffer system.
 *
 * @param key The RFB keysym (typically an X11 keysym like XK_a, XK_Return).
 *            Example: `XK_a` for the 'a' key, `XK_Shift_L` for left shift.
 * @param[out] key_name Pointer to a char pointer that will be set to the
 *                      canonical name of the key (e.g., "a", "Return").
 *                      The caller should not free this string.
 *                      Example output: If `key` is `XK_a`, `*key_name` might be "a".
 * @param[out] key_str Pointer to a char pointer that will be set to the
 *                     string representation of the key, considering shift state
 *                     (e.g., "a" for `XK_a`, "A" for `XK_A`, "+" for `XK_plus`).
 *                     The caller should not free this string.
 *                     Example output: If `key` is `XK_A` (assuming 'A' is shifted 'a'), `*key_str` might be "A".
 * @param[out] compose Pointer to a char pointer that will be set to the
 *                     compose string for the key (e.g., for dead keys or multi-key sequences).
 *                     The caller should not free this string.
 *                     Example output: For most keys, this will be similar to `key_str` or `key_name`.
 * @return `EINA_TRUE` if the translation was successful and the output parameters
 *         are populated, `EINA_FALSE` otherwise (e.g., if the keysym is unknown).
 */
Eina_Bool ecore_evas_vnc_server_keysym_to_fb_translate(rfbKeySym key,
                                                       const char **key_name,
                                                       const char **key_str,
                                                       const char **compose);

#endif
