#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* ifdef HAVE_CONFIG_H */

#include <stdlib.h>

#ifdef ECORE_XTEST
# include <X11/extensions/XTest.h>
#endif /* ifdef ECORE_XTEST */

#include "ecore_x_private.h"
#include "Ecore_X.h"
#include <string.h>

/**
 * @brief Simulates a key down event.
 *
 * This function fakes a key press event using the XTest extension.
 * The key can be specified by its name (e.g., "Control_L", "a", "b", "F1")
 * or by its keycode string (e.g., "Keycode-37").
 *
 * @param key The name of the key (e.g., "Alt_L") or its keycode string (e.g., "Keycode-64").
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise.
 *         Returns @c EINA_FALSE if ECORE_XTEST is not defined.
 *
 * @see ecore_x_test_fake_key_up()
 * @see ecore_x_test_fake_key_press()
 *
 * @since 1.1
 */
EAPI Eina_Bool
#ifdef ECORE_XTEST
ecore_x_test_fake_key_down(const char *key)
#else
ecore_x_test_fake_key_down(const char *key EINA_UNUSED)
#endif
{
#ifdef ECORE_XTEST
   KeyCode keycode = 0;
   KeySym keysym;

   LOGFN;
   if (!strncmp(key, "Keycode-", 8))
     keycode = atoi(key + 8);
   else
     {
        keysym = XStringToKeysym(key);
        if (keysym == NoSymbol)
          return EINA_FALSE;

        keycode = XKeysymToKeycode(_ecore_x_disp, keysym);
     }

   if (keycode == 0)
     return EINA_FALSE;

   return XTestFakeKeyEvent(_ecore_x_disp, keycode, 1, 0) ? EINA_TRUE : EINA_FALSE;
#else /* ifdef ECORE_XTEST */
   return EINA_FALSE;
#endif /* ifdef ECORE_XTEST */
}

/**
 * @brief Simulates a key up event.
 *
 * This function fakes a key release event using the XTest extension.
 * The key can be specified by its name (e.g., "Control_L", "a", "b", "F1")
 * or by its keycode string (e.g., "Keycode-37").
 *
 * @param key The name of the key (e.g., "Alt_L") or its keycode string (e.g., "Keycode-64").
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise.
 *         Returns @c EINA_FALSE if ECORE_XTEST is not defined.
 *
 * @see ecore_x_test_fake_key_down()
 * @see ecore_x_test_fake_key_press()
 *
 * @since 1.1
 */
EAPI Eina_Bool
#ifdef ECORE_XTEST
ecore_x_test_fake_key_up(const char *key)
#else
ecore_x_test_fake_key_up(const char *key EINA_UNUSED)
#endif
{
#ifdef ECORE_XTEST
   KeyCode keycode = 0;
   KeySym keysym;

   LOGFN;
   if (!strncmp(key, "Keycode-", 8))
     keycode = atoi(key + 8);
   else
     {
        keysym = XStringToKeysym(key);
        if (keysym == NoSymbol)
          return EINA_FALSE;

        keycode = XKeysymToKeycode(_ecore_x_disp, keysym);
     }

   if (keycode == 0)
     return EINA_FALSE;

   return XTestFakeKeyEvent(_ecore_x_disp, keycode, 0, 0) ? EINA_TRUE : EINA_FALSE;
#else /* ifdef ECORE_XTEST */
   return EINA_FALSE;
#endif /* ifdef ECORE_XTEST */
}

/**
 * @brief Simulates a key press event (down then up).
 *
 * This function fakes a key press and release event sequence using the XTest extension.
 * It can handle keys by name (e.g., "Control_L", "a", "b", "F1") or by keycode string
 * (e.g., "Keycode-37"). It also attempts to handle shifted keys.
 * If the key is not directly mappable, it may try to temporarily remap a keycode
 * to achieve the desired keysym.
 *
 * @param key The name of the key (e.g., "Shift_L", "A") or its keycode string (e.g., "Keycode-50").
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise.
 *         Returns @c EINA_FALSE if ECORE_XTEST is not defined.
 *
 * @note This function's attempt to remap keys might have side effects on the
 *       keyboard mapping, though it tries to be transient.
 *
 * @see ecore_x_test_fake_key_down()
 * @see ecore_x_test_fake_key_up()
 *
 * @since 1.1
 */
EAPI Eina_Bool
#ifdef ECORE_XTEST
ecore_x_test_fake_key_press(const char *key)
#else
ecore_x_test_fake_key_press(const char *key EINA_UNUSED)
#endif
{
#ifdef ECORE_XTEST
   KeyCode keycode = 0;
   KeySym keysym = 0;
   int shift = 0;

   LOGFN;
   if (!strncmp(key, "Keycode-", 8))
     keycode = atoi(key + 8);
   else
     {
        keysym = XStringToKeysym(key);
        if (keysym == NoSymbol)
          return EINA_FALSE;

        keycode = XKeysymToKeycode(_ecore_x_disp, keysym);
        if (_ecore_x_XKeycodeToKeysym(_ecore_x_disp, keycode, 0) != keysym)
          {
             if (_ecore_x_XKeycodeToKeysym(_ecore_x_disp, keycode, 1) == keysym)
               shift = 1;
             else
               keycode = 0;
          }
        else
          shift = 0;
     }

   if (keycode == 0)
     {
        static int mod = 0;
        KeySym *keysyms;
        int keycode_min, keycode_max, keycode_num;
        int i;

        XDisplayKeycodes(_ecore_x_disp, &keycode_min, &keycode_max);
        keysyms = XGetKeyboardMapping(_ecore_x_disp, keycode_min,
                                      keycode_max - keycode_min + 1,
                                      &keycode_num);
        mod = (mod + 1) & 0x7;
        i = (keycode_max - keycode_min - mod - 1) * keycode_num;

        keysyms[i] = keysym;
        XChangeKeyboardMapping(_ecore_x_disp, keycode_min, keycode_num,
                               keysyms, (keycode_max - keycode_min));
        XFree(keysyms);
        XSync(_ecore_x_disp, False);
        keycode = keycode_max - mod - 1;
     }

   if (shift)
     XTestFakeKeyEvent(_ecore_x_disp,
                       XKeysymToKeycode(_ecore_x_disp, XK_Shift_L), 1, 0);

   XTestFakeKeyEvent(_ecore_x_disp, keycode, 1, 0);
   XTestFakeKeyEvent(_ecore_x_disp, keycode, 0, 0);
   if (shift)
     XTestFakeKeyEvent(_ecore_x_disp,
                       XKeysymToKeycode(_ecore_x_disp, XK_Shift_L), 0, 0);

   return EINA_TRUE;
#else /* ifdef ECORE_XTEST */
   return EINA_FALSE;
#endif /* ifdef ECORE_XTEST */
}

/**
 * @brief Gets the string representation of a keysym.
 *
 * This function converts an X11 keysym value (e.g., XK_Shift_L, XK_a)
 * into its string name (e.g., "Shift_L", "a").
 *
 * @param keysym The keysym value. For example, `XK_Return` for the Enter key.
 * @return A pointer to the string name of the keysym, or @c NULL if the
 *         keysym is not valid. The returned string should not be freed.
 *
 * @since 1.1
 */
EAPI const char *
ecore_x_keysym_string_get(int keysym)
{
   return XKeysymToString(keysym);
}

/**
 * @brief Gets the keycode for a given key name or keycode string.
 *
 * This function converts a key name (e.g., "Control_L", "a") or a
 * keycode string (e.g., "Keycode-37") into its corresponding X11 keycode.
 *
 * @param keyname The name of the key (e.g., "space") or its keycode string (e.g., "Keycode-65").
 * @return The keycode for the given key name, or 0 if not found.
 *
 * @since 1.1
 */
EAPI int
ecore_x_keysym_keycode_get(const char *keyname)
{
   int keycode = 0;

   if (!strncmp(keyname, "Keycode-", 8))
     keycode = atoi(keyname + 8);
   else
     keycode = XKeysymToKeycode(_ecore_x_disp, XStringToKeysym(keyname));

   return keycode;
}

/**
 * @brief Gets the keysym for a given key string.
 *
 * This function converts a key string name (e.g., "Control_L", "a", "F1")
 * into its corresponding X11 keysym value.
 *
 * @param string The string name of the key (e.g., "Return" for the Enter key).
 * @return The keysym value for the given string, or @c NoSymbol if not found.
 *
 * @since 1.1
 */
EAPI unsigned int
ecore_x_keysym_get(const char *string)
{
   return XStringToKeysym(string);
}
