#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* ifdef HAVE_CONFIG_H */

#include <stdlib.h>
#include <string.h>

#include "Ecore.h"
#include "ecore_x_private.h"
#include "Ecore_X.h"
#include "Ecore_X_Atoms.h"
#include <inttypes.h>
#include <limits.h>

#define _ATOM_SET_CARD32(win, atom, p_val, cnt)                               \
  XChangeProperty(_ecore_x_disp, win, atom, XA_CARDINAL, 32, PropModeReplace, \
                  (unsigned char *)p_val, cnt)

/**
 * @brief Set a window property of type CARDINAL.
 *
 * This function sets a property on a window. The property consists of an array
 * of 32-bit unsigned integers (CARDINAL).
 *
 * @param win The window whose property is to be set.
 * @param atom The atom representing the property to set.
 * @param val An array of unsigned integers to be set as the property value.
 * @param num The number of unsigned integers in the @p val array.
 */
EAPI void
ecore_x_window_prop_card32_set(Ecore_X_Window win,
                               Ecore_X_Atom atom,
                               unsigned int *val,
                               unsigned int num)
{
#if SIZEOF_INT == SIZEOF_LONG
   _ATOM_SET_CARD32(win, atom, val, num);
#else /* if SIZEOF_INT == SIZEOF_LONG */
   long *v2;
   unsigned int i;

   LOGFN;
   v2 = malloc(num * sizeof(long));
   if (!v2)
     return;

   for (i = 0; i < num; i++)
     v2[i] = val[i];
   _ATOM_SET_CARD32(win, atom, v2, num);
   free(v2);
#endif /* if SIZEOF_INT == SIZEOF_LONG */
   if (_ecore_xlib_sync) ecore_x_sync();
}

/**
 * @brief Get a CARDINAL (32-bit unsigned integer) array property from a window.
 *
 * This function retrieves a property of type CARDINAL from a window and stores
 * it in a pre-allocated buffer.
 *
 * @param win The window from which to get the property.
 * @param atom The atom representing the property to get.
 * @param val A pre-allocated buffer to store the retrieved unsigned integer values.
 *            If NULL, the function will only return the number of items.
 * @param len The maximum number of items to store in @p val.
 * @return On success, the number of items stored in @p val. A value of 0 means
 *         the property exists but is empty. On failure, -1 is returned.
 */
EAPI int
ecore_x_window_prop_card32_get(Ecore_X_Window win,
                               Ecore_X_Atom atom,
                               unsigned int *val,
                               unsigned int len)
{
   unsigned char *prop_ret;
   Atom type_ret;
   unsigned long bytes_after, num_ret;
   int format_ret;
   unsigned int i;
   int num;

   LOGFN;
   prop_ret = NULL;
   if (XGetWindowProperty(_ecore_x_disp, win, atom, 0, 0x7fffffff, False,
                          XA_CARDINAL, &type_ret, &format_ret, &num_ret,
                          &bytes_after, &prop_ret) != Success)
     return -1;

   if (type_ret != XA_CARDINAL || format_ret != 32)
     num = -1;
   else if (num_ret == 0 || !prop_ret)
     num = 0;
   else
     {
        if (num_ret < len)
          len = num_ret;

        if (val)
          for (i = 0; i < len; i++)
            val[i] = ((unsigned long *)prop_ret)[i];
        num = len;
     }

   if (_ecore_xlib_sync) ecore_x_sync();
   if (prop_ret)
     XFree(prop_ret);
   return num;
}

/**
 * @brief Get a CARDINAL (32-bit unsigned integer) array property of any length.
 *
 * This function retrieves a property of type CARDINAL from a window. It
 * allocates memory for the returned list of values. The caller is responsible
 * for freeing this memory with `free()`.
 *
 * @param win The window from which to get the property.
 * @param atom The atom representing the property to get.
 * @param[out] plst A pointer to a variable that will be set to the newly
 *                  allocated array of unsigned integers. If the property does
 *                  not exist or is empty, this will be set to NULL.
 * @return The number of items in the returned list, or -1 on failure. A value
 *         of 0 means the property exists but is empty.
 */
EAPI int
ecore_x_window_prop_card32_list_get(Ecore_X_Window win,
                                    Ecore_X_Atom atom,
                                    unsigned int **plst)
{
   unsigned char *prop_ret;
   Atom type_ret;
   unsigned long bytes_after, num_ret;
   int format_ret;
   unsigned int i, *val;
   int num;

   LOGFN;
   if (plst) *plst = NULL;
   prop_ret = NULL;
   if (XGetWindowProperty(_ecore_x_disp, win, atom, 0, 0x7fffffff, False,
                          XA_CARDINAL, &type_ret, &format_ret, &num_ret,
                          &bytes_after, &prop_ret) != Success)
     return -1;

   if ((type_ret != XA_CARDINAL) || (format_ret != 32))
     num = -1;
   else if ((num_ret == 0) || (!prop_ret))
     num = 0;
   else if (plst)
     {
        val = malloc(num_ret * sizeof(unsigned int));
        if (!val)
          {
             if (prop_ret) XFree(prop_ret);
             return -1;
          }
        for (i = 0; i < num_ret; i++)
          val[i] = ((unsigned long *)prop_ret)[i];
        num = num_ret;
        *plst = val;
     }
   else
     num = num_ret;

   if (_ecore_xlib_sync) ecore_x_sync();
   if (prop_ret)
     XFree(prop_ret);
   return num;
}

/**
 * @brief Set a window property consisting of an array of X IDs.
 *
 * This is a generic function to set properties that are lists of X resource
 * identifiers, such as Windows, Pixmaps, or Atoms.
 *
 * @param win The window whose property is to be set.
 * @param atom The atom representing the property to set.
 * @param type The type of the property (e.g., XA_WINDOW, XA_ATOM).
 * @param lst An array of Ecore_X_ID values to set.
 * @param num The number of IDs in the @p lst array.
 */
EAPI void
ecore_x_window_prop_xid_set(Ecore_X_Window win,
                            Ecore_X_Atom atom,
                            Ecore_X_Atom type,
                            Ecore_X_ID *lst,
                            unsigned int num)
{
#if SIZEOF_INT == SIZEOF_LONG
   XChangeProperty(_ecore_x_disp, win, atom, type, 32, PropModeReplace,
                   (unsigned char *)lst, num);
#else /* if SIZEOF_INT == SIZEOF_LONG */
   unsigned long *pl;
   unsigned int i;

   LOGFN;
   pl = malloc(num * sizeof(unsigned long));
   if (!pl)
     return;

   for (i = 0; i < num; i++)
     pl[i] = lst[i];
   XChangeProperty(_ecore_x_disp, win, atom, type, 32, PropModeReplace,
                   (unsigned char *)pl, num);
   free(pl);
#endif /* if SIZEOF_INT == SIZEOF_LONG */
   if (_ecore_xlib_sync) ecore_x_sync();
}

/**
 * @brief Get a window property consisting of an array of X IDs.
 *
 * This function retrieves a property that is a list of X resource identifiers
 * and stores it in a pre-allocated buffer.
 *
 * @param win The window from which to get the property.
 * @param atom The atom representing the property to get.
 * @param type The expected type of the property (e.g., XA_WINDOW, XA_ATOM).
 * @param lst A pre-allocated buffer to store the retrieved X IDs.
 *            If NULL, the function will only return the number of items.
 * @param len The maximum number of items to store in @p lst.
 * @return On success, the number of items stored in @p lst. A value of 0 means
 *         the property exists but is empty. On failure, -1 is returned (e.g.,
 *         if the actual property type does not match @p type).
 */
EAPI int
ecore_x_window_prop_xid_get(Ecore_X_Window win,
                            Ecore_X_Atom atom,
                            Ecore_X_Atom type,
                            Ecore_X_ID *lst,
                            unsigned int len)
{
   unsigned char *prop_ret;
   Atom type_ret;
   unsigned long bytes_after, num_ret;
   int format_ret;
   int num;
   unsigned i;
   Eina_Bool success;

   LOGFN;
   prop_ret = NULL;
   success = (XGetWindowProperty(_ecore_x_disp, win, atom, 0, 0x7fffffff, False,
                          type, &type_ret, &format_ret, &num_ret,
                          &bytes_after, &prop_ret) == Success);
   if (_ecore_xlib_sync) ecore_x_sync();
   if (!success) return -1;

   if (type_ret != type || format_ret != 32)
     num = -1;
   else if (num_ret == 0 || !prop_ret)
     num = 0;
   else
     {
        if (num_ret < len)
          len = num_ret;

        if (lst)
          for (i = 0; i < len; i++)
            lst[i] = ((unsigned long *)prop_ret)[i];
        num = len;
     }

   if (prop_ret)
     XFree(prop_ret);

   return num;
}

/**
 * @brief Get a window property consisting of an array of X IDs of any length.
 *
 * This function retrieves a property that is a list of X resource identifiers.
 * It allocates memory for the returned list. The caller is responsible for
 * freeing this memory with `free()`.
 *
 * @param win The window from which to get the property.
 * @param atom The atom representing the property to get.
 * @param type The expected type of the property (e.g., XA_WINDOW, XA_ATOM).
 * @param[out] val A pointer to a variable that will be set to the newly
 *                 allocated array of X IDs. If the property does not exist or
 *                 is empty, this will be set to NULL.
 * @return The number of items in the returned list, or -1 on failure. A value
 *         of 0 means the property exists but is empty.
 */
EAPI int
ecore_x_window_prop_xid_list_get(Ecore_X_Window win,
                                 Ecore_X_Atom atom,
                                 Ecore_X_Atom type,
                                 Ecore_X_ID **val)
{
   unsigned char *prop_ret;
   Atom type_ret;
   unsigned long bytes_after, num_ret;
   int format_ret;
   Ecore_X_Atom *alst;
   int num;
   unsigned i;
   Eina_Bool success;

   LOGFN;
   if (val) *val = NULL;
   prop_ret = NULL;
   success = (XGetWindowProperty(_ecore_x_disp, win, atom, 0, 0x7fffffff, False,
                          type, &type_ret, &format_ret, &num_ret,
                          &bytes_after, &prop_ret) == Success);
   if (_ecore_xlib_sync) ecore_x_sync();
   if (!success) return -1;

   if (type_ret != type || format_ret != 32)
     num = -1;
   else if (num_ret == 0 || !prop_ret)
     num = 0;
   else if (val)
     {
        alst = malloc(num_ret * sizeof(Ecore_X_ID));
        for (i = 0; i < num_ret; i++)
          alst[i] = ((unsigned long *)prop_ret)[i];
        num = num_ret;
        *val = alst;
     }
   else
     num = num_ret;

   if (prop_ret)
     XFree(prop_ret);
   return num;
}

/**
 * @brief Modify a window property that is a list of X IDs.
 *
 * This function adds, removes, or toggles an item in a list-based property
 * on a window. It fetches the current list, modifies it, and then writes it
 * back to the server.
 *
 * @param win The window whose property is to be changed.
 * @param atom The atom representing the property to change.
 * @param type The type of the property (e.g. XA_ATOM, XA_WINDOW).
 * @param item The X ID to add, remove, or toggle in the list.
 * @param op The operation to perform. It can be one of:
 *           - @c ECORE_X_PROP_LIST_ADD: Add the item if it is not already in the list.
 *           - @c ECORE_X_PROP_LIST_REMOVE: Remove the item if it is in the list.
 *           - A different value (like @c ECORE_X_PROP_LIST_TOGGLE):
 *             Add the item if not present, remove it if it is present.
 */
EAPI void
ecore_x_window_prop_xid_list_change(Ecore_X_Window win,
                                    Ecore_X_Atom atom,
                                    Ecore_X_Atom type,
                                    Ecore_X_ID item,
                                    int op)
{
   Ecore_X_ID *lst, *temp;
   int i, num;

   LOGFN;
   num = ecore_x_window_prop_xid_list_get(win, atom, type, &lst);
   if (num < 0)
     {
        return; /* Error - assuming invalid window */
     }

   /* Is it there? */
   for (i = 0; i < num; i++)
     {
        if (lst[i] == item)
          break;
     }

   if (i < num)
     {
        /* Was in list */
        if (op == ECORE_X_PROP_LIST_ADD)
          goto done;  /* Remove it */

        num--;
        for (; i < num; i++)
          lst[i] = lst[i + 1];
     }
   else
     {
        /* Was not in list */
        if (op == ECORE_X_PROP_LIST_REMOVE)
          goto done;  /* Add it */

        num++;

        temp = lst;
        lst = realloc(lst, num * sizeof(Ecore_X_ID));
        if (lst)
          {
             lst[i] = item;
          }
        else
          {
             lst = temp;
             num--;
          }
     }

   ecore_x_window_prop_xid_set(win, atom, type, lst, num);

done:
   if (lst)
     free(lst);
}

/**
 * @brief Set a window property with an array of atoms.
 *
 * This is a convenience wrapper around ecore_x_window_prop_xid_set() with
 * the type fixed to @c XA_ATOM.
 *
 * @param win The window whose property is to be set.
 * @param atom The atom representing the property to set.
 * @param lst An array of atoms to set as the property value.
 * @param num The number of atoms in the @p lst array.
 */
EAPI void
ecore_x_window_prop_atom_set(Ecore_X_Window win,
                             Ecore_X_Atom atom,
                             Ecore_X_Atom *lst,
                             unsigned int num)
{
   LOGFN;
   ecore_x_window_prop_xid_set(win, atom, XA_ATOM, lst, num);
}

/**
 * @brief Get a window property consisting of an array of atoms.
 *
 * This is a convenience wrapper around ecore_x_window_prop_xid_get() with
 * the type fixed to @c XA_ATOM. It stores the result in a pre-allocated buffer.
 *
 * @param win The window from which to get the property.
 * @param atom The atom representing the property to get.
 * @param lst A pre-allocated buffer to store the retrieved atoms.
 * @param len The maximum number of atoms to store in @p lst.
 * @return On success, the number of items stored in @p lst. A value of 0 means
 *         the property exists but is empty. On failure, -1 is returned.
 */
EAPI int
ecore_x_window_prop_atom_get(Ecore_X_Window win,
                             Ecore_X_Atom atom,
                             Ecore_X_Atom *lst,
                             unsigned int len)
{
   int ret;
   LOGFN;
   ret = ecore_x_window_prop_xid_get(win, atom, XA_ATOM, lst, len);
   return ret;
}

/**
 * @brief Get a window property consisting of an array of atoms of any length.
 *
 * This is a convenience wrapper around ecore_x_window_prop_xid_list_get() with
 * the type fixed to @c XA_ATOM. It allocates memory for the returned list.
 * The caller is responsible for freeing this memory with `free()`.
 *
 * @param win The window from which to get the property.
 * @param atom The atom representing the property to get.
 * @param[out] plst A pointer to a variable that will be set to the newly
 *                  allocated array of atoms.
 * @return The number of items in the returned list, or -1 on failure.
 */
EAPI int
ecore_x_window_prop_atom_list_get(Ecore_X_Window win,
                                  Ecore_X_Atom atom,
                                  Ecore_X_Atom **plst)
{
   int ret;
   LOGFN;
   ret = ecore_x_window_prop_xid_list_get(win, atom, XA_ATOM, plst);
   return ret;
}

/**
 * @brief Modify a window property that is a list of atoms.
 *
 * This is a convenience wrapper around ecore_x_window_prop_xid_list_change()
 * with the type fixed to @c XA_ATOM. It adds, removes, or toggles an atom
 * in a list-based property.
 *
 * @param win The window whose property is to be changed.
 * @param atom The atom representing the property to change.
 * @param item The atom to add, remove, or toggle in the list.
 * @param op The operation to perform (see ecore_x_window_prop_xid_list_change()).
 */
EAPI void
ecore_x_window_prop_atom_list_change(Ecore_X_Window win,
                                     Ecore_X_Atom atom,
                                     Ecore_X_Atom item,
                                     int op)
{
   LOGFN;
   ecore_x_window_prop_xid_list_change(win, atom, XA_ATOM, item, op);
}

/**
 * @brief Set a window property with an array of window IDs.
 *
 * This is a convenience wrapper around ecore_x_window_prop_xid_set() with
 * the type fixed to @c XA_WINDOW.
 *
 * @param win The window whose property is to be set.
 * @param atom The atom representing the property to set.
 * @param lst An array of window IDs to set as the property value.
 * @param num The number of window IDs in the @p lst array.
 */
EAPI void
ecore_x_window_prop_window_set(Ecore_X_Window win,
                               Ecore_X_Atom atom,
                               Ecore_X_Window *lst,
                               unsigned int num)
{
   LOGFN;
   ecore_x_window_prop_xid_set(win, atom, XA_WINDOW, lst, num);
}

/**
 * @brief Get a window property consisting of an array of window IDs.
 *
 * This is a convenience wrapper around ecore_x_window_prop_xid_get() with
 * the type fixed to @c XA_WINDOW. It stores the result in a pre-allocated buffer.
 *
 * @param win The window from which to get the property.
 * @param atom The atom representing the property to get.
 * @param lst A pre-allocated buffer to store the retrieved window IDs.
 * @param len The maximum number of window IDs to store in @p lst.
 * @return On success, the number of items stored in @p lst. A value of 0 means
 *         the property exists but is empty. On failure, -1 is returned.
 */
EAPI int
ecore_x_window_prop_window_get(Ecore_X_Window win,
                               Ecore_X_Atom atom,
                               Ecore_X_Window *lst,
                               unsigned int len)
{
   int ret;
   LOGFN;
   ret = ecore_x_window_prop_xid_get(win, atom, XA_WINDOW, lst, len);
   return ret;
}

/**
 * @brief Get a window property consisting of an array of window IDs of any length.
 *
 * This is a convenience wrapper around ecore_x_window_prop_xid_list_get() with
 * the type fixed to @c XA_WINDOW. It allocates memory for the returned list.
 * The caller is responsible for freeing this memory with `free()`.
 *
 * @param win The window from which to get the property.
 * @param atom The atom representing the property to get.
 * @param[out] plst A pointer to a variable that will be set to the newly
 *                  allocated array of window IDs.
 * @return The number of items in the returned list, or -1 on failure.
 */
EAPI int
ecore_x_window_prop_window_list_get(Ecore_X_Window win,
                                    Ecore_X_Atom atom,
                                    Ecore_X_Window **plst)
{
   int ret;
   LOGFN;
   ret = ecore_x_window_prop_xid_list_get(win, atom, XA_WINDOW, plst);
   return ret;
}

/**
 * @brief Get the atom for 'AnyPropertyType'.
 *
 * This function returns the X atom `AnyPropertyType`, which can be used in
 * functions like ecore_x_window_prop_property_get() to retrieve a property
 * without specifying its type.
 *
 * @return The `AnyPropertyType` atom.
 */
EAPI Ecore_X_Atom
ecore_x_window_prop_any_type(void)
{
   return AnyPropertyType;
}

/**
 * @brief Set a property on a window with raw data.
 *
 * This function provides a low-level way to set a window property. It is more
 * flexible than the type-specific functions like ecore_x_window_prop_card32_set().
 *
 * @param win The window on which to set the property. If 0, the root window is used.
 * @param property The atom of the property to set.
 * @param type The atom of the property's type.
 * @param size The format of the property, which can be 8, 16, or 32 (bits).
 *             This defines the size of each element in @p data.
 * @param data A pointer to the raw data to be set.
 * @param number The number of elements in @p data (not the number of bytes).
 */
EAPI void
ecore_x_window_prop_property_set(Ecore_X_Window win,
                                 Ecore_X_Atom property,
                                 Ecore_X_Atom type,
                                 int size,
                                 void *data,
                                 int number)
{
   LOGFN;
   if (win == 0)
     win = DefaultRootWindow(_ecore_x_disp);

   if (size != 32)
     XChangeProperty(_ecore_x_disp,
                     win,
                     property,
                     type,
                     size,
                     PropModeReplace,
                     (unsigned char *)data,
                     number);
   else
     {
        unsigned long *dat;
        int i, *ptr;

        dat = malloc(sizeof(unsigned long) * number);
        if (dat)
          {
             for (ptr = (int *)data, i = 0; i < number; i++)
               dat[i] = ptr[i];
             XChangeProperty(_ecore_x_disp, win, property, type, size,
                             PropModeReplace, (unsigned char *)dat, number);
             free(dat);
          }
     }
   if (_ecore_xlib_sync) ecore_x_sync();
}

/**
 * @brief Get a property from a window as raw data.
 *
 * This function provides a low-level way to retrieve a window property. It
 * allocates memory for the retrieved data, which must be freed by the caller
 * using `free()`.
 *
 * @param win The window from which to get the property. If 0, the root window is used.
 * @param property The atom of the property to get.
 * @param type The expected atom of the property's type. Use
 *             ecore_x_window_prop_any_type() to match any type.
 * @param size This parameter is unused.
 * @param[out] data A pointer to a variable that will be set to the newly
 *                  allocated buffer containing the property data.
 * @param[out] num A pointer to an integer that will be filled with the number
 *                 of elements in the returned data array.
 * @return The format of the returned property (8, 16, or 32 bits) on success,
 *         or 0 on failure or if the property does not exist.
 */
EAPI int
ecore_x_window_prop_property_get(Ecore_X_Window win,
                                 Ecore_X_Atom property,
                                 Ecore_X_Atom type,
                                 int size EINA_UNUSED,
                                 unsigned char **data,
                                 int *num)
{
   Atom type_ret = 0;
   int ret, size_ret = 0;
   unsigned long num_ret = 0, bytes = 0, i;
   unsigned char *prop_ret = NULL;

   /* make sure these are initialized */
   if (num)
     *num = 0;

   if (data)
     *data = NULL;
   else /* we can't store the retrieved data, so just return */
     return 0;

   LOGFN;
   if (!win)
     win = DefaultRootWindow(_ecore_x_disp);

   ret = XGetWindowProperty(_ecore_x_disp, win, property, 0, LONG_MAX,
                            False, type, &type_ret, &size_ret,
                            &num_ret, &bytes, &prop_ret);
   if (_ecore_xlib_sync) ecore_x_sync();
   if (ret != Success)
     return 0;
   if ((!num_ret) || (size_ret <= 0))
     {
        XFree(prop_ret);
        return 0;
     }

   if (!(*data = malloc(num_ret * size_ret / 8)))
     {
        XFree(prop_ret);
        return 0;
     }

   switch (size_ret) {
      case 8:
        for (i = 0; i < num_ret; i++)
          (*data)[i] = prop_ret[i];
        break;

      case 16:
        for (i = 0; i < num_ret; i++)
          ((unsigned short *)*data)[i] = ((unsigned short *)prop_ret)[i];
        break;

      case 32:
        for (i = 0; i < num_ret; i++)
          ((unsigned int *)*data)[i] = ((unsigned long *)prop_ret)[i];
        break;
     }

   XFree(prop_ret);

   if (num)
     *num = num_ret;

   return size_ret;
}

/**
 * @brief Delete a property from a window.
 *
 * @param win The window from which to delete the property.
 * @param property The atom of the property to delete.
 */
EAPI void
ecore_x_window_prop_property_del(Ecore_X_Window win,
                                 Ecore_X_Atom property)
{
   LOGFN;
   XDeleteProperty(_ecore_x_disp, win, property);
   if (_ecore_xlib_sync) ecore_x_sync();
}

/**
 * @brief List all properties of a window.
 *
 * This function retrieves a list of all property atoms set on a given window.
 * It allocates memory for the returned list, which must be freed by the
 * caller using `free()`.
 *
 * @param win The window to query for properties.
 * @param[out] num_ret A pointer to an integer that will be filled with the
 *                     number of properties found.
 * @return A newly allocated array of `Ecore_X_Atom` containing the property
 *         atoms, or NULL on failure or if no properties are set.
 */
EAPI Ecore_X_Atom *
ecore_x_window_prop_list(Ecore_X_Window win,
                         int *num_ret)
{
   Ecore_X_Atom *atoms;
   Atom *atom_ret;
   int num = 0, i;

   LOGFN;
   if (num_ret)
     *num_ret = 0;

   atom_ret = XListProperties(_ecore_x_disp, win, &num);
   if (_ecore_xlib_sync) ecore_x_sync();
   if (!atom_ret)
     return NULL;

   atoms = malloc(num * sizeof(Ecore_X_Atom));
   if (atoms)
     {
        for (i = 0; i < num; i++)
          atoms[i] = atom_ret[i];
        if (num_ret)
          *num_ret = num;
     }

   XFree(atom_ret);
   return atoms;
}

/**
 * @brief Set a window string property.
 *
 * This function sets a property on a window using a UTF-8 encoded string.
 * It uses XSetTextProperty with an encoding of ECORE_X_ATOM_UTF8_STRING.
 *
 * @param win The window on which to set the property. If 0, the root window is used.
 * @param type The atom representing the property to set (e.g., XA_WM_NAME).
 * @param str The UTF-8 string to set as the property value.
 */
EAPI void
ecore_x_window_prop_string_set(Ecore_X_Window win,
                               Ecore_X_Atom type,
                               const char *str)
{
   XTextProperty xtp;

   LOGFN;
   if (win == 0)
     win = DefaultRootWindow(_ecore_x_disp);

   xtp.value = (unsigned char *)str;
   xtp.format = 8;
   xtp.encoding = ECORE_X_ATOM_UTF8_STRING;
   xtp.nitems = strlen(str);
   XSetTextProperty(_ecore_x_disp, win, &xtp, type);
   if (_ecore_xlib_sync) ecore_x_sync();
}

/**
 * @brief Get a window string property.
 *
 * This function retrieves a string property from a window. It attempts to
 * convert the property to a UTF-8 string if it is not already. The returned
 * string is allocated with `strdup()` and must be freed by the caller.
 *
 * @param win The window from which to get the property. If 0, the root window is used.
 * @param type The atom representing the property to get.
 * @return A newly allocated string containing the property value, or NULL if
 *         the property could not be retrieved or is empty.
 */
EAPI char *
ecore_x_window_prop_string_get(Ecore_X_Window win,
                               Ecore_X_Atom type)
{
   XTextProperty xtp;
   char *str = NULL;

   LOGFN;
   if (win == 0)
     win = DefaultRootWindow(_ecore_x_disp);

   if (XGetTextProperty(_ecore_x_disp, win, &xtp, type))
     {
        int items;
        char **list = NULL;
        Status s;

        if (_ecore_xlib_sync) ecore_x_sync();
        if (xtp.encoding == ECORE_X_ATOM_UTF8_STRING)
          str = strdup((char *)xtp.value);
        else
          {
#ifdef X_HAVE_UTF8_STRING
             s = Xutf8TextPropertyToTextList(_ecore_x_disp, &xtp,
                                             &list, &items);
#else /* ifdef X_HAVE_UTF8_STRING */
             s = XmbTextPropertyToTextList(_ecore_x_disp, &xtp,
                                           &list, &items);
#endif /* ifdef X_HAVE_UTF8_STRING */
             if (_ecore_xlib_sync) ecore_x_sync();
             if ((s == XLocaleNotSupported) ||
                 (s == XNoMemory) || (s == XConverterNotFound))
               str = strdup((char *)xtp.value);
             else if ((s >= Success) && (items > 0))
               str = strdup(list[0]);

             if (list)
               XFreeStringList(list);
          }

        XFree(xtp.value);
     }
   return str;
}

/**
 * @brief Check if a specific WM protocol is supported by a window.
 *
 * This function checks the WM_PROTOCOLS property on a window to see if a
 * given protocol is listed.
 *
 * @param win The window to check.
 * @param protocol The WM protocol to check for (e.g., ECORE_X_WM_PROTOCOL_DELETE_REQUEST).
 * @return @c EINA_TRUE if the protocol is set, @c EINA_FALSE otherwise or on error.
 */
EAPI Eina_Bool
ecore_x_window_prop_protocol_isset(Ecore_X_Window win,
                                   Ecore_X_WM_Protocol protocol)
{
   Atom proto, *protos = NULL;
   int i, protos_count = 0;
   Eina_Bool ret = EINA_FALSE;

   /* check for invalid values */
   if (protocol >= ECORE_X_WM_PROTOCOL_NUM)
     return EINA_FALSE;

   LOGFN;
   proto = _ecore_x_atoms_wm_protocols[protocol];

   ret = XGetWMProtocols(_ecore_x_disp, win, &protos, &protos_count);
   if (_ecore_xlib_sync) ecore_x_sync();
   if (!ret)
     return ret;

   for (i = 0; i < protos_count; i++)
     if (protos[i] == proto)
       {
          ret = EINA_TRUE;
          break;
       }

   XFree(protos);
   return ret;
}

/**
 * @brief Get the list of WM protocols supported by a window.
 *
 * This function retrieves the WM_PROTOCOLS property from a window and returns
 * an array of corresponding `Ecore_X_WM_Protocol` enum values. The returned
 * array must be freed by the caller using `free()`.
 *
 * @param win The window to query.
 * @param[out] num_ret A pointer to an integer that will be filled with the number
 *                     of protocols in the returned array.
 * @return A newly allocated array of `Ecore_X_WM_Protocol` values, or NULL
 *         on failure or if no protocols are set. Unrecognized protocols in the
 *         property will be represented by the value -1 in the array.
 */
EAPI Ecore_X_WM_Protocol *
ecore_x_window_prop_protocol_list_get(Ecore_X_Window win,
                                      int *num_ret)
{
   Atom *protos = NULL;
   int i, protos_count = 0;
   Ecore_X_WM_Protocol *prot_ret = NULL;
   Eina_Bool success;

   LOGFN;
   success = XGetWMProtocols(_ecore_x_disp, win, &protos, &protos_count);
   if (_ecore_xlib_sync) ecore_x_sync();
   if (!success)
     return NULL;

   if ((!protos) || (protos_count <= 0))
     return NULL;

   prot_ret = calloc(1, protos_count * sizeof(Ecore_X_WM_Protocol));
   if (!prot_ret)
     {
        XFree(protos);
        return NULL;
     }

   for (i = 0; i < protos_count; i++)
     {
        Ecore_X_WM_Protocol j;

        prot_ret[i] = -1;
        for (j = 0; j < ECORE_X_WM_PROTOCOL_NUM; j++)
          {
             if (_ecore_x_atoms_wm_protocols[j] == protos[i])
               prot_ret[i] = j;
          }
     }
   XFree(protos);
   *num_ret = protos_count;
   return prot_ret;
}

