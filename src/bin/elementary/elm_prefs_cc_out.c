#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include "elm_prefs_cc.h"
#include "elm_prefs_edd.x"

/**
 * @brief Global variable holding the parsed preferences file data.
 *
 * This variable stores the entire structure of the preferences file,
 * including all pages and their associated data, after it has been
 * parsed from the input EDD file. It is used by various functions
 * to access and manipulate the preferences data.
 */
Elm_Prefs_File *elm_prefs_file = NULL;

/**
 * @brief Initializes the data handling system.
 *
 * This function sets up the Eet library for data serialization and
 * initializes the Elementary Prefs data descriptors. It must be called
 * before any other data operations are performed.
 */
void
data_init()
{
   eet_init();
   _elm_prefs_descriptors_init();
}

/**
 * @brief Shuts down the data handling system.
 *
 * This function cleans up resources used by the Eet library and
 * the Elementary Prefs data descriptors. It should be called when
 * the application is exiting to ensure proper cleanup.
 */
void
data_shutdown()
{
   _elm_prefs_descriptors_shutdown();
   eet_shutdown();
}

/**
 * @brief Writes the preferences data to an output file.
 *
 * This function serializes the preferences data stored in the global
 * ::elm_prefs_file variable and writes it to the specified output file
 * (defined by the `file_out` variable, which is not visible in this snippet
 * but assumed to be globally accessible or passed in a way not shown).
 *
 * The data is written in Eet format. Each page within the preferences
 * data is written as a separate Eet entry.
 *
 * @note This function will exit the program with an error code if:
 *       - No data is available to write (i.e., ::elm_prefs_file is NULL
 *         or has no pages).
 *       - The output file cannot be opened for writing.
 *       - Writing a page to the Eet file fails.
 */
void
data_write()
{
   Eina_List *l;
   Eet_File *ef;
   Elm_Prefs_Page_Node *page;

   if (!elm_prefs_file || !elm_prefs_file->pages)
     {
        ERR("No data to put in \"%s\"", file_out);
        exit(-1);
     }

   ef = eet_open(file_out, EET_FILE_MODE_WRITE);
   if (!ef)
     {
        ERR("Unable to open \"%s\" for writing output", file_out);
        exit(-1);
     }

   EINA_LIST_FOREACH (elm_prefs_file->pages, l, page)
     {
        if (!(eet_data_write
              (ef, _page_edd, page->name, page, EET_COMPRESSION_DEFAULT)))
          ERR("Failed to write page %s to file %s", page->name, file_out);
     }

   eet_close(ef);
}
