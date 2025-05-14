#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* ifdef HAVE_CONFIG_H */

#ifdef STDC_HEADERS
# include <stdlib.h>
# include <stddef.h>
#else
# ifdef HAVE_STDLIB_H
#  include <stdlib.h>
# endif
#endif

#include <string.h>

#include "Ecore.h"
#include "ecore_x_private.h"
#include "Ecore_X.h"
#include "Ecore_X_Atoms.h"
#include "ecore_x_atoms_decl.h"

/**
 * @internal
 * @brief Initializes pre-defined X atoms.
 *
 * This function is called internally to initialize a set of commonly used
 * X atoms. It iterates over the `atom_items` array (defined in
 * `ecore_x_atoms_decl.h`), calls `XInternAtoms` to get the actual
 * X atom identifiers from the X server, and stores these identifiers
 * back into the `atom_items` array. This pre-initialization helps
 * to avoid repeated calls to `XInternAtom` for these common atoms.
 */
void
_ecore_x_atoms_init(void)
{
   Atom *atoms;
   char **names;
   int i, num;

   num = sizeof(atom_items) / sizeof(Atom_Item);
   atoms = alloca(num * sizeof(Atom));
   names = alloca(num * sizeof(char *));
   for (i = 0; i < num; i++)
     names[i] = (char *) atom_items[i].name;
   XInternAtoms(_ecore_x_disp, names, num, False, atoms);
   for (i = 0; i < num; i++)
     *(atom_items[i].atom) = atoms[i];
}

/**
 * Retrieves the atom value associated with the given name.
 * @param  name The given name.
 * @return Associated atom value.
 */
EAPI Ecore_X_Atom
ecore_x_atom_get(const char *name)
{
   Ecore_X_Atom atom;

   LOGFN;
   EINA_SAFETY_ON_NULL_RETURN_VAL(_ecore_x_disp, 0);
   atom = XInternAtom(_ecore_x_disp, name, False);
   if (_ecore_xlib_sync) ecore_x_sync();
   return atom;
}

/**
 * @brief Retrieves multiple X atoms by their names.
 *
 * This function sends a request to the X server to get the atom
 * identifiers for an array of atom names.
 *
 * @param names An array of C-strings, where each string is an atom name.
 *              Example: `const char *atom_names[] = {"UTF8_STRING", "WM_NAME"};`
 * @param num The number of atom names in the `names` array.
 * @param atoms A pre-allocated array where the retrieved Ecore_X_Atom
 *              values will be stored. The size of this array must be at least `num`.
 *              Example: `Ecore_X_Atom retrieved_atoms[2];`
 */
EAPI void
ecore_x_atoms_get(const char **names,
                  int num,
                  Ecore_X_Atom *atoms)
{
   Atom *atoms_int;
   int i;

   LOGFN;
   EINA_SAFETY_ON_NULL_RETURN(_ecore_x_disp);
   atoms_int = alloca(num * sizeof(Atom));
   XInternAtoms(_ecore_x_disp, (char **)names, num, False, atoms_int);
   for (i = 0; i < num; i++)
     atoms[i] = atoms_int[i];
   if (_ecore_xlib_sync) ecore_x_sync();
}

/**
 * @brief Retrieves the name of an X atom.
 *
 * This function queries the X server for the string name associated
 * with a given atom identifier.
 *
 * @param atom The Ecore_X_Atom identifier whose name is to be retrieved.
 * @return A newly allocated string containing the atom's name,
 *         or @c NULL if the atom does not exist or an error occurs.
 *         The caller is responsible for freeing this string using `free()`.
 *         Example: `char *name = ecore_x_atom_name_get(ECORE_X_ATOM_WM_NAME); if (name) free(name);`
 */
EAPI char *
ecore_x_atom_name_get(Ecore_X_Atom atom)
{
   char *name;
   char *xname;

   LOGFN;

   EINA_SAFETY_ON_NULL_RETURN_VAL(_ecore_x_disp, NULL);

   xname = XGetAtomName(_ecore_x_disp, atom);
   if (_ecore_xlib_sync) ecore_x_sync();
   if (!xname)
     return NULL;

   name = strdup(xname);
   XFree(xname);

   return name;
}

