#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <string.h>
#include <locale.h>
#include <limits.h>
#include <sys/stat.h>

#include "edje_cc.h"
#include "edje_convert.h"

extern Eet_Data_Descriptor *_edje_edd_old_edje_file;
extern Eet_Data_Descriptor *_edje_edd_old_edje_part_collection;

void _edje_edd_old_shutdown(void);
void _edje_edd_old_init(void);

char *progname = NULL;
int _edje_cc_log_dom = -1;

/**
 * @brief Logs an error message, closes the Eet_File, and aborts the program.
 *
 * This function is called when a critical error occurs that prevents
 * the program from continuing.
 *
 * @param ef The Eet_File to close. Can be NULL if no file is open.
 * @param fmt The format string for the error message (printf-style).
 * @param ... Additional arguments for the format string.
 */
void
error_and_abort(Eet_File *ef, const char *fmt, ...)
{
   va_list ap;

   va_start(ap, fmt);
   eina_log_vprint(_edje_cc_log_dom, EINA_LOG_LEVEL_CRITICAL,
                   "unknown", "unknown", 0, fmt, ap);
   va_end(ap);
   eet_close(ef);
   exit(-1);
}

/**
 * @brief Prints the command-line usage help for the program.
 */
static void
main_help(void)
{
   printf
     ("Usage:\n"
      "\t%s file.edj\n"
      "\n"
     , progname);
}

/**
 * @brief Creates aliases in an Eet_File for entries with integer IDs.
 *
 * This function lists entries under a given `base` path (e.g., "images/*"),
 * parses an integer ID from each entry name, and creates an alias
 * from a new path (e.g., "edje/images/ID") to the original entry.
 *
 * @param target The target file name (used for logging).
 * @param ef The Eet_File to modify.
 * @param base The base path in the Eet_File to search for entries (e.g., "images").
 *             The function expects entries like "images/0", "images/1", etc.
 * @param to The base path for the new aliases (e.g., "edje/images").
 *           Aliases will be created like "edje/images/0", "edje/images/1", etc.
 */
static void
_edje_alias_int(const char *target, Eet_File *ef, const char *base, const char *to)
{
   char buf[1024];
   char **match;
   int count;
   int i;

   snprintf(buf, sizeof (buf), "%s/*", base);
   match = eet_list(ef, buf, &count);

   snprintf(buf, sizeof (buf), "%s/", base);
   strcat(buf, "%i");
   for (i = 0; i < count && match; ++i)
     {
        char name[1024];
        int id;

        if (sscanf(match[i], buf, &id) != 1)
          {
             EINA_LOG_ERR("Wrong fonts section `%s' in `%s'. Discarding it.", match[i], target);
             continue;
          }

        snprintf(name, sizeof (name), "%s/%i", to, id);
        eet_alias(ef, name, match[i], 1);
     }
   free(match);
}

/**
 * @brief Creates aliases in an Eet_File for entries with string IDs.
 *
 * This function lists entries under a given `base` path (e.g., "fonts/*"),
 * parses a string ID from each entry name, and creates an alias
 * from a new path (e.g., "edje/fonts/ID") to the original entry.
 *
 * @param target The target file name (used for logging).
 * @param ef The Eet_File to modify.
 * @param base The base path in the Eet_File to search for entries (e.g., "fonts").
 *             The function expects entries like "fonts/font_name.ttf", etc.
 * @param to The base path for the new aliases (e.g., "edje/fonts").
 *           Aliases will be created like "edje/fonts/font_name.ttf", etc.
 */
static void
_edje_alias_string(const char *target, Eet_File *ef, const char *base, const char *to)
{
   char buf[1024];
   char **match;
   int count;
   int i;

   snprintf(buf, sizeof (buf), "%s/*", base);
   match = eet_list(ef, buf, &count);

   snprintf(buf, sizeof (buf), "%s/", base);
   strcat(buf, "%s");
   for (i = 0; i < count && match; ++i)
     {
        char name[1024];
        char id[1024];

        if (sscanf(match[i], buf, &id) != 1)
          {
             EINA_LOG_ERR("Wrong fonts section `%s' in `%s'. Discarding it.", match[i], target);
             continue;
          }

        snprintf(name, sizeof (name), "%s/%s", to, id);
        eet_alias(ef, name, match[i], 1);
     }
   free(match);
}

/**
 * @brief Main function for the edje_convert utility.
 *
 * This program converts older Edje file formats to the current format.
 * It reads an Edje file, converts its internal structures (collections,
 * images, fonts, scripts), and writes the updated data back to the file.
 *
 * @param argc Argument count.
 * @param argv Argument vector. Expects one argument: the path to the .edj file.
 * @return 0 on success, -1 on failure.
 */
int
main(int argc, char **argv)
{
   Edje_Part_Collection_Directory_Entry *ce;
   Old_Edje_File *oef;
   Edje_File *nef;
   Eina_Iterator *it;
   Eet_File *ef;

   setlocale(LC_NUMERIC, "C");

   ecore_app_no_system_modules();

   if (!eina_init())
     return -1;

   _edje_cc_log_dom = eina_log_domain_register
       ("edje_convert", EDJE_CC_DEFAULT_LOG_COLOR);
   if (_edje_cc_log_dom < 0)
     {
        EINA_LOG_ERR("Enable to create a log domain.");
        exit(-1);
     }

   eet_init();

   progname = argv[0];
   if (argc > 2 || argc < 2 || !strcmp(argv[1], "-h"))
     {
        main_help();
        return 0;
     }

   _edje_edd_old_init();
   _edje_edd_init();

   ef = eet_open(argv[1], EET_FILE_MODE_READ_WRITE);
   if (!ef)
     {
        EINA_LOG_ERR("Unable to open `%s'.", argv[1]);
        return -1;
     }

   oef = eet_data_read(ef, _edje_edd_old_edje_file, "edje_file");
   if (!oef)
     {
        EINA_LOG_ERR("Unable to get edje_file section from old file format, are you sure it's an old file ?");
        return -1;
     }

   nef = _edje_file_convert(ef, oef);
   _edje_file_set(nef);

   /* Iterate through each collection in the Edje file. */
   it = eina_hash_iterator_data_new(nef->collection);

   EINA_ITERATOR_FOREACH(it, ce)
     {
        /* For each collection:
         * 1. Read the old part collection data.
         * 2. Convert it to the new part collection format.
         * 3. Write the new part collection data back to the Eet_File.
         */
        Old_Edje_Part_Collection *opc;
        Edje_Part_Collection *npc;
        char buf[1024];
        int bytes = 0;

        snprintf(buf, sizeof (buf), "collections/%i", ce->id);
        opc = eet_data_read(ef, _edje_edd_old_edje_part_collection, buf);
        if (!opc)
          {
             EINA_LOG_ERR("Unable to find collection `%s'[%i] in `%s'.", ce->entry, ce->id, argv[1]);
             return -1;
          }

        npc = _edje_collection_convert(ef, ce, opc);

        snprintf(buf, sizeof (buf), "edje/collections/%i", ce->id);
        bytes = eet_data_write(ef, _edje_edd_edje_part_collection, buf, npc, 1);
        if (bytes <= 0)
          {
             EINA_LOG_ERR("Unable to save `%s' in section `%s' of `%s'.", ce->entry, buf, argv[1]);
             return -1;
          }
     }
   eina_iterator_free(it);

   /* Write the main "edje/file" section with the converted Edje_File structure. */
   if (eet_data_write(ef, _edje_edd_edje_file, "edje/file", nef, 1) <= 0)
     {
        EINA_LOG_ERR("Unable to save main section of `%s'.", argv[1]);
        return -1;
     }

   /* Create aliases for various data sections (fonts, images, scripts)
    * to map old paths to new paths within the Eet_File. */
   _edje_alias_string(argv[1], ef, "fonts", "edje/fonts");
   _edje_alias_int(argv[1], ef, "images", "edje/images");
   _edje_alias_int(argv[1], ef, "scripts", "edje/scripts/embryo/compiled");
   _edje_alias_int(argv[1], ef, "lua_script", "edje/scripts/lua");

   eet_close(ef);

   _edje_edd_shutdown();
   _edje_edd_old_shutdown();

   eet_shutdown();
   eina_shutdown();

   return 0;
}

