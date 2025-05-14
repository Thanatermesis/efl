#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* ifdef HAVE_CONFIG_H */

#include <stdlib.h>


#ifdef LOGRT
#include <dlfcn.h>
#endif /* ifdef LOGRT */

#include "ecore_x_private.h"
#include "Ecore_X.h"
#include "Ecore_X_Atoms.h"


//////////////////////////////////////////////////////////////////////////////
// This api and structure only for the key router and window client side
// Application do not use this

//this mask is defined by key router.
//after discussing with keyrouter module, this mask can be changed
#define GRAB_MASK 0xffff00
#define OVERRIDE_EXCLUSIVE_GRAB 0xf00000
#define EXCLUSIVE_GRAB 0x0f0000
#define TOPMOST_GRAB 0x00f000
#define SHARED_GRAB 0x000f00

//if _ecore_keyrouter = 0, not yet check keyrouter
//if _ecore_keyrouter = -1, keyrouter not exist
//if _ecore_keyrouter = 1, keyrouter exist
int _ecore_keyrouter = 0; /**< Global flag indicating the status of keyrouter detection. 0: not checked, -1: not found, 1: found. */

/**
 * @brief Structure to hold key grab information for a window.
 * This structure is used internally to manage keys grabbed by a specific window.
 */
struct _Ecore_X_Window_Key_Table
{
   Ecore_X_Window       win;       /**< The window ID for which keys are grabbed. */
   int                 *key_list;  /**< An array of grabbed keycodes, potentially including grab mode flags. */
   unsigned long        key_cnt;   /**< The number of keys in key_list. */
};

typedef struct _Ecore_X_Window_Key_Table             Ecore_X_Window_Key_Table;

static int       _ecore_x_window_keytable_key_search(Ecore_X_Window_Key_Table *keytable, int key);
static Eina_Bool _ecore_x_window_keytable_key_del(Ecore_X_Window_Key_Table *key_table, int key, Ecore_X_Atom keytable_atom);

static Eina_Bool _ecore_x_window_keytable_key_add(Ecore_X_Window_Key_Table *keytable,
                                 int keycode,
                                 Ecore_X_Win_Keygrab_Mode grab_mode);

static Eina_Bool _ecore_x_window_keygrab_set_internal(Ecore_X_Window win, const char *key, Ecore_X_Win_Keygrab_Mode grab_mode);
static Eina_Bool _ecore_x_window_keygrab_unset_internal(Ecore_X_Window win, const char *key);
static Eina_Bool _ecore_x_window_keytable_get(Ecore_X_Window win, Ecore_X_Window_Key_Table *keytable);


//(Below Atom and exclusiveness_get/set functions) should be changed after keyrouter finds the solution to avoid race condition
//solution 1. window manages two key table. keytable and keytable result
//solution 2. using client messabe between the window client and the key router.

static Atom _atom_grab_excl_win = None; /**< Atom used for managing globally exclusive key grabs. */
#define STR_ATOM_GRAB_EXCL_WIN "_GRAB_EXCL_WIN_KEYCODE" /**< String identifier for the _GRAB_EXCL_WIN_KEYCODE atom. */

/**
 * @brief Frees the memory allocated for a key table's key list.
 *
 * @param keytable Pointer to the Ecore_X_Window_Key_Table whose key_list is to be freed.
 *                 The key_list member will be set to NULL and key_cnt to 0.
 */
static void
_keytable_free(Ecore_X_Window_Key_Table *keytable)
{
   if (keytable->key_list)
     free(keytable->key_list);
   keytable->key_list = NULL;
   keytable->win = 0;
   keytable->key_cnt = 0;
}

/**
 * @brief Retrieves a list of unsigned integers from a window property.
 *
 * This function fetches a window property of type XA_CARDINAL and returns
 * it as an array of unsigned integers. The caller is responsible for freeing
 * the allocated memory for `*plst`.
 *
 * @param win The window from which to get the property.
 * @param atom The atom identifying the property.
 * @param[out] plst Pointer to store the resulting list of unsigned integers.
 *                  This will be allocated by the function and must be freed by the caller.
 * @return The number of integers in the list, or -1 on failure.
 */
static int
_keytable_property_list_get(Ecore_X_Window win,
                            Ecore_X_Atom atom,
                            unsigned int **plst)
{
   unsigned char *prop_ret;
   Atom type_ret;
   unsigned long bytes_after, num_ret;
   int format_ret;
   unsigned int i, *val;
   int num;

   *plst = NULL;
   prop_ret = NULL;
   if (XGetWindowProperty(_ecore_x_disp, win, atom, 0, 0x7fffffff, False,
                          XA_CARDINAL, &type_ret, &format_ret, &num_ret,
                          &bytes_after, &prop_ret) != Success)
     {
        WRN("XGetWindowProperty failed");
        return -1;
     }
   else if ((num_ret == 0) || (!prop_ret))
     num = 0;
   else
     {
        val = malloc(num_ret * sizeof(unsigned int));
        if (!val)
          {
             if (prop_ret) XFree(prop_ret);
             WRN("Memory alloc failed");
             return -1;
          }
        for (i = 0; i < num_ret; i++)
          val[i] = ((unsigned long *)prop_ret)[i];
        num = num_ret;
        *plst = val;
     }

   if (_ecore_xlib_sync) ecore_x_sync();
   if (prop_ret)
     XFree(prop_ret);
   return num;
}

/**
 * @brief Checks if a keycode can be grabbed with global exclusiveness.
 *
 * This function queries a global property (identified by _atom_grab_excl_win)
 * on the root window to see if the given keycode is already exclusively grabbed
 * by another client.
 *
 * @param keycode The keycode to check.
 * @return EINA_TRUE if the keycode can be grabbed exclusively, EINA_FALSE otherwise
 *         (e.g., if already grabbed or an error occurred).
 */
static Eina_Bool
_ecore_x_window_keytable_possible_global_exclusiveness_get(int keycode)
{
   int ret = 0;

   Ecore_X_Window_Key_Table keytable;

   keytable.win = ecore_x_window_root_first_get();
   keytable.key_list = NULL;
   keytable.key_cnt = 0;

   if(_atom_grab_excl_win == None )
     _atom_grab_excl_win = XInternAtom(_ecore_x_disp, STR_ATOM_GRAB_EXCL_WIN, False);

   ret = _keytable_property_list_get(keytable.win, _atom_grab_excl_win,
                                     (unsigned int **)&(keytable.key_list));

   if (ret < 0)
     {
	    return EINA_FALSE;
     }

   keytable.key_cnt = ret;

   if (keytable.key_cnt == 0)
     {
        WRN("There is no keygrab entry in the table");
        return EINA_TRUE;
     }

   //check keycode exists in the global exclusiveness keytable

   ret = _ecore_x_window_keytable_key_search(&keytable, keycode);
   if (ret != -1)
     {
        WRN("Can't search keygrab entry in the table");
        _keytable_free(&keytable);
        return EINA_FALSE;
     }
   _keytable_free(&keytable);
   return EINA_TRUE;
}

/**
 * @brief Marks a keycode as globally exclusively grabbed.
 *
 * This function adds the given keycode to a global property (identified by
 * _atom_grab_excl_win) on the root window, indicating that it is now
 * exclusively grabbed.
 *
 * @param keycode The keycode to mark as exclusively grabbed.
 * @return EINA_TRUE if the keycode was successfully marked, EINA_FALSE otherwise
 *         (e.g., if it was already marked by another client or an error occurred).
 */
static Eina_Bool
_ecore_x_window_keytable_possible_global_exclusiveness_set(int keycode)
{
   int ret = 0;

   Ecore_X_Window_Key_Table keytable;

   keytable.win = ecore_x_window_root_first_get();
   keytable.key_list = NULL;
   keytable.key_cnt = 0;

   if(_atom_grab_excl_win == None )
     _atom_grab_excl_win = XInternAtom(_ecore_x_disp, STR_ATOM_GRAB_EXCL_WIN, False);

   ret = _keytable_property_list_get(keytable.win, _atom_grab_excl_win,
                                     (unsigned int **)&(keytable.key_list));
   if (ret < 0) return EINA_FALSE;

   keytable.key_cnt = ret;

   if (keytable.key_cnt == 0)
     {
        XChangeProperty(_ecore_x_disp, keytable.win, _atom_grab_excl_win, XA_CARDINAL, 32,
                        PropModeReplace, (unsigned char *)&keycode, 1);
        XSync(_ecore_x_disp, False);
        _keytable_free(&keytable);
        return EINA_TRUE;
     }

   //check keycode exists in the global exclusiveness keytable
   ret = _ecore_x_window_keytable_key_search(&keytable, keycode);
   if (ret != -1)
     {
        XChangeProperty(_ecore_x_disp, keytable.win, _atom_grab_excl_win, XA_CARDINAL, 32,
                        PropModeAppend, (unsigned char *)&keycode, 1);
        XSync(_ecore_x_disp, False);
        _keytable_free(&keytable);
        return EINA_TRUE;
     }
   WRN("Already key is grabbed");
   _keytable_free(&keytable);
   return EINA_FALSE;
}

/**
 * @brief Removes a keycode from the list of globally exclusively grabbed keys.
 *
 * This function removes the given keycode from a global property (identified by
 * _atom_grab_excl_win) on the root window, indicating that it is no longer
 * exclusively grabbed.
 *
 * @param keycode The keycode to unmark.
 * @return EINA_TRUE if the keycode was successfully unmarked (though current implementation
 *         often returns EINA_FALSE even on success due to logic flow), EINA_FALSE if the key
 *         was not found or an error occurred.
 * @note The return value logic seems to indicate EINA_FALSE on success in some paths.
 */
static Eina_Bool
_ecore_x_window_keytable_possible_global_exclusiveness_unset(int keycode)
{
   int ret = 0;

   Ecore_X_Window_Key_Table keytable;

   keytable.win = ecore_x_window_root_first_get();
   keytable.key_list = NULL;
   keytable.key_cnt = 0;

   if(_atom_grab_excl_win == None )
     _atom_grab_excl_win = XInternAtom(_ecore_x_disp, STR_ATOM_GRAB_EXCL_WIN, False);

   ret = _keytable_property_list_get(keytable.win, _atom_grab_excl_win,
                                     (unsigned int **)&(keytable.key_list));
   if (ret <= 0) return EINA_FALSE;

   keytable.key_cnt = ret;

   //check keycode exists in the global exclusiveness keytable
   ret = _ecore_x_window_keytable_key_search(&keytable, keycode);
   if (ret == -1)
     {
        WRN("Already key exists");
        _keytable_free(&keytable);
        return EINA_FALSE;
     }
   else
     _ecore_x_window_keytable_key_del(&keytable, keycode, _atom_grab_excl_win);

   _keytable_free(&keytable);
   return EINA_FALSE;
}

/**
 * @brief Decodes an encoded keycode to extract the raw keycode and grab mode.
 *
 * The encoded keycode contains both the actual keycode and flags indicating
 * the grab mode. This function separates these two components.
 *
 * @param keycode_encoded The encoded keycode (keycode | grab_mask).
 * @param[out] keycode Pointer to store the decoded raw keycode.
 * @param[out] grab_mode Pointer to store the decoded Ecore_X_Win_Keygrab_Mode.
 * @return EINA_TRUE if decoding was successful, EINA_FALSE if the grab_mode
 *         is unknown.
 */
static Eina_Bool
_ecore_x_window_keytable_keycode_decode(int keycode_encoded,
                                        int *keycode,
                                        Ecore_X_Win_Keygrab_Mode *grab_mode)
{
   int key_mask = 0;

   *keycode = keycode_encoded & (~GRAB_MASK);
   key_mask = keycode_encoded & GRAB_MASK;

   if (key_mask == SHARED_GRAB)
     {
        *grab_mode = ECORE_X_WIN_KEYGRAB_SHARED;
        return EINA_TRUE;
     }
   else if (key_mask == TOPMOST_GRAB)
     {
        *grab_mode = ECORE_X_WIN_KEYGRAB_TOPMOST;
        return EINA_TRUE;
     }
   else if (key_mask == EXCLUSIVE_GRAB)
     {
        *grab_mode = ECORE_X_WIN_KEYGRAB_EXCLUSIVE;
        return EINA_TRUE;
     }
   else if (key_mask == OVERRIDE_EXCLUSIVE_GRAB)
     {
        *grab_mode = ECORE_X_WIN_KEYGRAB_OVERRIDE_EXCLUSIVE;
        return EINA_TRUE;
     }
   else
     {
        *grab_mode = ECORE_X_WIN_KEYGRAB_UNKNOWN;
        WRN("Keycode decoding failed. Unknown Keygrab mode");
        return EINA_FALSE;
     }
}

/**
 * @brief Encodes a raw keycode and a grab mode into a single integer.
 *
 * This function combines the raw keycode with flags representing the grab mode.
 *
 * @param keycode The raw keycode.
 * @param grab_mode The Ecore_X_Win_Keygrab_Mode to apply.
 * @param[out] keycode_encoded Pointer to store the resulting encoded keycode.
 * @return EINA_TRUE if encoding was successful, EINA_FALSE if the grab_mode
 *         is invalid.
 */
static Eina_Bool
_ecore_x_window_keytable_keycode_encode(int keycode,
                                        Ecore_X_Win_Keygrab_Mode grab_mode,
                                        int *keycode_encoded)
{
   if ((grab_mode <= ECORE_X_WIN_KEYGRAB_UNKNOWN) || (grab_mode > ECORE_X_WIN_KEYGRAB_OVERRIDE_EXCLUSIVE))
     {
        *keycode_encoded = 0;
        WRN("Keycode encoding failed. Unknown Keygrab mode");
        return EINA_FALSE;
     }
   if (grab_mode == ECORE_X_WIN_KEYGRAB_SHARED)
     *keycode_encoded = keycode | SHARED_GRAB;
   else if (grab_mode == ECORE_X_WIN_KEYGRAB_TOPMOST)
     *keycode_encoded = keycode | TOPMOST_GRAB;
   else if (grab_mode == ECORE_X_WIN_KEYGRAB_EXCLUSIVE)
     *keycode_encoded = keycode | EXCLUSIVE_GRAB;
   else if (grab_mode == ECORE_X_WIN_KEYGRAB_OVERRIDE_EXCLUSIVE)
     *keycode_encoded = keycode | OVERRIDE_EXCLUSIVE_GRAB;
   return EINA_TRUE;
}

/**
 * @brief Retrieves the key table for a given window.
 *
 * This function fetches the ECORE_X_ATOM_E_KEYROUTER_WINDOW_KEYTABLE property
 * from the specified window and populates the provided keytable structure.
 *
 * @param win The window whose key table is to be retrieved.
 * @param[out] keytable Pointer to an Ecore_X_Window_Key_Table structure to be filled.
 *                      The caller should ensure keytable->win is set. keytable->key_list
 *                      will be allocated by this function (via _keytable_property_list_get)
 *                      and should be freed by the caller using _keytable_free or free().
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
_ecore_x_window_keytable_get(Ecore_X_Window win,
                             Ecore_X_Window_Key_Table *keytable)
{
   int ret = 0;

   ret = _keytable_property_list_get(win, ECORE_X_ATOM_E_KEYROUTER_WINDOW_KEYTABLE,
                                     (unsigned int **)&(keytable->key_list));
   if (ret < 0) return EINA_FALSE;

   keytable->key_cnt = ret;

   return EINA_TRUE;
}

/**
 * @brief Searches for a key (ignoring grab mode bits) within a key table.
 *
 * @param keytable Pointer to the Ecore_X_Window_Key_Table to search within.
 * @param key The keycode to search for. The grab mode bits in this key are
 *            used for comparison if present, but the search primarily matches
 *            the base keycode.
 * @return The index of the found key in keytable->key_list, or -1 if not found.
 */
static int
_ecore_x_window_keytable_key_search(Ecore_X_Window_Key_Table *keytable,
                                    int key)
{
   int  i;
   int keycode = 0;
   unsigned long key_cnt;
   int *key_list = NULL;

   keycode = key & (~GRAB_MASK);
   key_cnt = keytable->key_cnt;
   key_list = keytable->key_list;

   for (i = key_cnt - 1; i >= 0; i--)
     {
        if ((key_list[i] & (~GRAB_MASK)) == keycode) break;
     }
   return i;
}

/**
 * @brief Adds a key with a specific grab mode to a window's key table.
 *
 * This function encodes the keycode and grab mode, then updates the
 * ECORE_X_ATOM_E_KEYROUTER_WINDOW_KEYTABLE property on the window.
 *
 * @param keytable Pointer to the Ecore_X_Window_Key_Table representing the window's current state.
 *                 keytable->win should be the target window.
 *                 keytable->key_list and keytable->key_cnt should reflect the current property state.
 * @param keycode The raw keycode to add.
 * @param grab_mode The desired grab mode for this key.
 * @return EINA_TRUE if the key was successfully added, EINA_FALSE otherwise (e.g.,
 *         if the key already exists or encoding fails).
 */
static Eina_Bool
_ecore_x_window_keytable_key_add(Ecore_X_Window_Key_Table *keytable,
                                 int keycode,
                                 Ecore_X_Win_Keygrab_Mode grab_mode)
{
   int i = 0;
   int keycode_masked = 0;

   Ecore_Window   win;
   unsigned long  key_cnt;

   win = keytable->win;
   key_cnt = keytable->key_cnt;

   if (!_ecore_x_window_keytable_keycode_encode(keycode, grab_mode, &keycode_masked))
     return EINA_FALSE;

   if (key_cnt == 0)
     {
        XChangeProperty(_ecore_x_disp, win, ECORE_X_ATOM_E_KEYROUTER_WINDOW_KEYTABLE, XA_CARDINAL, 32,
                        PropModeReplace, (unsigned char *)&keycode_masked, 1);
        XSync(_ecore_x_disp, False);
        return EINA_TRUE;
     }
   else
     {
        i = _ecore_x_window_keytable_key_search(keytable, keycode_masked);
        if ( i != -1 )
          {
             //already exist key in key table
             WRN("Already key exists");
             return EINA_FALSE;
          }
        XChangeProperty(_ecore_x_disp, win, ECORE_X_ATOM_E_KEYROUTER_WINDOW_KEYTABLE, XA_CARDINAL, 32,
                        PropModeAppend, (unsigned char *)&keycode_masked, 1);
        XSync(_ecore_x_disp, False);
        return EINA_TRUE;
     }
}

/**
 * @brief Deletes a key from a window's key table property.
 *
 * @param key_table Pointer to an Ecore_X_Window_Key_Table structure.
 *                  Its key_list and key_cnt members will be updated.
 *                  key_table->win specifies the window.
 * @param key The encoded keycode (including grab mode bits) to delete.
 * @param keytable_atom The atom of the property to modify (e.g.,
 *                      ECORE_X_ATOM_E_KEYROUTER_WINDOW_KEYTABLE or _atom_grab_excl_win).
 * @return EINA_TRUE if the key was successfully deleted, EINA_FALSE otherwise
 *         (e.g., if the key was not found or memory allocation failed).
 */
static Eina_Bool
_ecore_x_window_keytable_key_del(Ecore_X_Window_Key_Table *key_table,
                                 int key,
                                 Ecore_X_Atom keytable_atom)
{
   int i;
   int *new_key_list = NULL;
   unsigned long key_cnt = 0;

   // Only one element is exists in the list of grabbed key
   i = _ecore_x_window_keytable_key_search(key_table, key);

   if (i == -1)
     {
        WRN("Key doesn't exist in the key table.");
        return EINA_FALSE;
     }

   (key_table->key_cnt)--;
   key_cnt = key_table->key_cnt;

   if (key_cnt == 0)
     {
        XDeleteProperty(_ecore_x_disp, key_table->win, keytable_atom);
        XSync(_ecore_x_disp, False);
        return EINA_TRUE;
     }

   // Shrink the buffer
   new_key_list = malloc((key_cnt) * sizeof(int));

   if (new_key_list == NULL)
     return EINA_FALSE;

   // copy head
   if (i > 0)
     memcpy(new_key_list, key_table->key_list, sizeof(int) * i);

   // copy tail
   if ((key_cnt) - i > 0)
     {
        memcpy(new_key_list + i,
               key_table->key_list + i + 1,
               sizeof(int) * (key_cnt - i));
     }

   XChangeProperty(_ecore_x_disp, key_table->win, keytable_atom, XA_CARDINAL, 32,
                   PropModeReplace, (unsigned char *)new_key_list, key_cnt);
   XSync(_ecore_x_disp, False);

   free(new_key_list);
   return EINA_TRUE;
}

/**
 * @brief Internal implementation for setting a key grab on a window.
 *
 * This function handles the conversion of key string to keycode,
 * checks for global exclusiveness if required, and updates the window's
 * key table property.
 *
 * @param win The window on which to set the key grab.
 * @param key A string representing the key (e.g., "Alt+F1", "XF86AudioPlay", "Keycode-65").
 * @param grab_mode The desired grab mode for this key.
 * @return EINA_TRUE if the key grab was successfully set, EINA_FALSE otherwise.
 */
static Eina_Bool
_ecore_x_window_keygrab_set_internal(Ecore_X_Window win,
                                     const char *key,
                                     Ecore_X_Win_Keygrab_Mode grab_mode)
{
   KeyCode keycode = 0;
   KeySym keysym;

   Eina_Bool ret = EINA_FALSE;
   Ecore_X_Window_Key_Table keytable;

   keytable.win = win;
   keytable.key_list = NULL;
   keytable.key_cnt = 0;


   //check the key string
   if (!strncmp(key, "Keycode-", 8))
     keycode = atoi(key + 8);
   else
     {
        keysym = XStringToKeysym(key);
        if (keysym == NoSymbol)
          {
             WRN("Keysym of key(\"%s\") doesn't exist", key);
             return ret;
          }
        keycode = XKeysymToKeycode(_ecore_x_disp, keysym);
     }

   if (keycode == 0)
     {
        WRN("Keycode of key(\"%s\") doesn't exist", key);
        return ret;
     }

   if(grab_mode == ECORE_X_WIN_KEYGRAB_EXCLUSIVE)
     {
        //Only one window can grab this key;
        //keyrouter should avoid race condition
        if (!_ecore_x_window_keytable_possible_global_exclusiveness_get(keycode))
          return EINA_FALSE;
     }

   if (!_ecore_x_window_keytable_get(win, &keytable))
     return EINA_FALSE;

   ret = _ecore_x_window_keytable_key_add(&keytable, keycode, grab_mode);


   if (!ret)
     {
        WRN("Key(\"%s\") add failed", key);
        goto error;
     }

   if(grab_mode == ECORE_X_WIN_KEYGRAB_EXCLUSIVE)
     {
        //Only one window can grab this key;
        if(!_ecore_x_window_keytable_possible_global_exclusiveness_set(keycode))
          {
             _ecore_x_window_keytable_key_del(&keytable, keycode, ECORE_X_ATOM_E_KEYROUTER_WINDOW_KEYTABLE);
             WRN("Key(\"%s\") already is grabbed", key);
             goto error;
          }
     }

   _keytable_free(&keytable);
   return EINA_TRUE;
error:
   _keytable_free(&keytable);
   return EINA_FALSE;
}

/**
 * @brief Internal implementation for unsetting a key grab on a window.
 *
 * This function handles the conversion of key string to keycode,
 * finds the corresponding entry in the window's key table, removes it,
 * and updates global exclusiveness state if necessary.
 *
 * @param win The window from which to unset the key grab.
 * @param key A string representing the key to unset (e.g., "Alt+F1", "XF86AudioPlay", "Keycode-65").
 * @return EINA_TRUE if the key grab was successfully unset, EINA_FALSE otherwise.
 */
static Eina_Bool
_ecore_x_window_keygrab_unset_internal(Ecore_X_Window win,
                                       const char *key)
{
   KeyCode keycode = 0;
   KeySym keysym;

   int i;
   int key_masked = 0;
   int key_decoded = 0;

   Eina_Bool ret = EINA_FALSE;

   Ecore_X_Window_Key_Table keytable;
   Ecore_X_Win_Keygrab_Mode grab_mode = ECORE_X_WIN_KEYGRAB_UNKNOWN;

   keytable.win = win;
   keytable.key_list = NULL;
   keytable.key_cnt = 0;

   if (!strncmp(key, "Keycode-", 8))
     keycode = atoi(key + 8);
   else
     {
        keysym = XStringToKeysym(key);
        if (keysym == NoSymbol)
          {
             WRN("Keysym of key(\"%s\") doesn't exist", key);
             return EINA_FALSE;
          }
        keycode = XKeysymToKeycode(_ecore_x_disp, keysym);
     }

   if (keycode == 0)
     {
        WRN("Keycode of key(\"%s\") doesn't exist", key);
        return EINA_FALSE;
     }

   //construct the keytable structure using Xproperty
   if (!_ecore_x_window_keytable_get(win, &keytable))
      return EINA_FALSE;

   if (keytable.key_cnt == 0)
     return EINA_FALSE;

   i = _ecore_x_window_keytable_key_search(&keytable, keycode);

   if (i == -1) //cannot find key in keytable
     {
        WRN("Key(\"%s\") doesn't exist", key);
        goto error;
     }

   //find key in keytable
   key_masked = keytable.key_list[i];

   ret = _ecore_x_window_keytable_keycode_decode(key_masked, &key_decoded, &grab_mode);

   if (!ret)
     goto error;

   ret = _ecore_x_window_keytable_key_del(&keytable, key_masked, ECORE_X_ATOM_E_KEYROUTER_WINDOW_KEYTABLE);
   if (!ret)
     goto error;

   if (grab_mode == ECORE_X_WIN_KEYGRAB_EXCLUSIVE)
     {
        ret = _ecore_x_window_keytable_possible_global_exclusiveness_unset(keycode);
     }

   _keytable_free(&keytable);
   return EINA_TRUE;
error:
   _keytable_free(&keytable);
   return EINA_FALSE;
}

/**
 * @brief Sets a key grab on a window using the E keyrouter mechanism.
 *
 * This function allows an application to request a key grab with different modes
 * (shared, topmost, exclusive, override_exclusive). The actual grabbing is
 * managed by an external keyrouter component.
 *
 * The `mod`, `not_mod`, and `priority` parameters are currently unused by this
 * implementation but are kept for API compatibility or future extension.
 *
 * @param win The Ecore_X_Window to grab the key for.
 * @param key A string identifying the key to grab. This can be a keysym name
 *            (e.g., "Control_L", "a", "F1") or a keycode string (e.g., "Keycode-37").
 *            Modifiers are typically part of the keyrouter's configuration, not this string.
 * @param mod Unused by this implementation.
 * @param not_mod Unused by this implementation.
 * @param priority Unused by this implementation.
 * @param grab_mode The desired grabbing mode (e.g., ECORE_X_WIN_KEYGRAB_SHARED,
 *                  ECORE_X_WIN_KEYGRAB_EXCLUSIVE).
 * @return EINA_TRUE if the request to grab the key was successfully processed,
 *         EINA_FALSE otherwise (e.g., keyrouter not available, key already grabbed
 *         exclusively by another client, invalid key string).
 *
 * @note This function relies on an external E keyrouter. If the keyrouter is not
 *       running or not detected, key grabbing will fail.
 * @note For ECORE_X_WIN_KEYGRAB_EXCLUSIVE, this function attempts to register
 *       the grab globally. If another client has already exclusively grabbed
 *       this key, the operation will fail.
 */
EAPI Eina_Bool
ecore_x_window_keygrab_set(Ecore_X_Window win,
                           const char *key,
                           int mod EINA_UNUSED,
                           int not_mod EINA_UNUSED,
                           int priority EINA_UNUSED,
                           Ecore_X_Win_Keygrab_Mode grab_mode)
{
   if (_ecore_keyrouter == 0)
     {
        if(ecore_x_e_keyrouter_get(win))
          _ecore_keyrouter = 1;
        else
          {
             WRN("Keyrouter is not supported");
             _ecore_keyrouter = -1;
          }
     }
   if (_ecore_keyrouter < 0)
     return EINA_FALSE;

   return _ecore_x_window_keygrab_set_internal(win, key, grab_mode);
}

/**
 * @brief Unsets a key grab on a window using the E keyrouter mechanism.
 *
 * This function requests the removal of a previously set key grab.
 *
 * The `mod` and `any_mod` parameters are currently unused by this
 * implementation.
 *
 * @param win The Ecore_X_Window from which to ungrab the key.
 * @param key A string identifying the key to ungrab, matching the string
 *            used in ecore_x_window_keygrab_set().
 * @param mod Unused by this implementation.
 * @param any_mod Unused by this implementation.
 * @return EINA_TRUE if the request to ungrab the key was successfully processed,
 *         EINA_FALSE otherwise (e.g., keyrouter not available, key was not
 *         grabbed by this window, invalid key string).
 *
 * @note This function relies on an external E keyrouter.
 */
EAPI Eina_Bool
ecore_x_window_keygrab_unset(Ecore_X_Window win,
                             const char *key,
                             int mod EINA_UNUSED,
                             int any_mod EINA_UNUSED)
{
   if (_ecore_keyrouter != 1)
     {
        WRN("Keyrouter is not supported");
        return EINA_FALSE;
     }

   return _ecore_x_window_keygrab_unset_internal(win, key);
}

