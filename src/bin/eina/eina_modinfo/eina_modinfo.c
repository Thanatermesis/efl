/* EINA - EFL data type library
 * Copyright (C) 2016 Amitesh Singh
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library;
 * if not, see <http://www.gnu.org/licenses/>.
 *
 * --------------- usage ---------------------
 *  $ eina_modinfo <module.so>
 */

/**
 * @file
 * @brief A utility to display information about an Eina module.
 *
 * This program takes a shared object file (.so) as a command-line argument,
 * loads it as an Eina module, and prints metadata such as version,
 * description, license, and author.
 */

#include <Eina.h>

/**
 * @brief Main function for the eina_modinfo utility.
 *
 * Parses command-line arguments, loads the specified Eina module,
 * retrieves and prints module information, and then cleans up.
 *
 * @param argc The number of command-line arguments.
 * @param argv An array of command-line arguments.
 *             argv[0] is the program name.
 *             argv[1] is expected to be the path to the Eina module file.
 * @return 0 on success, 1 if arguments are incorrect, 2 if the module
 *         cannot be opened, and 3 if the module cannot be loaded.
 */
int main(int argc, char **argv)
{
   Eina_Module *em;

   eina_init();
   if (argc != 2)
     {
        fprintf(stderr, "Error: Missing module or filename\n");
        return 1;
     }

   em = eina_module_new(argv[1]);
   if (!em)
     {
        fprintf(stderr, "Error: Failed to open: %s\n", argv[1]);
        return 2;
     }
   if (!eina_module_load(em))
     {
        fprintf(stderr, "Error: Failed to load module\n");
        eina_module_free(em);
        return 3;
     }

   printf("version:        %s\n", (char *)eina_module_symbol_get(em, "__EINA_MODULE_UNIQUE_ID_ver"));
   printf("description:    %s\n", (char *)eina_module_symbol_get(em, "__EINA_MODULE_UNIQUE_ID_desc"));
   printf("license:        %s\n", (char *)eina_module_symbol_get(em, "__EINA_MODULE_UNIQUE_ID_license"));
   printf("author:         %s\n", (char *)eina_module_symbol_get(em, "__EINA_MODULE_UNIQUE_ID_author"));

   eina_module_free(em);
   eina_shutdown();

   return 0;
}
