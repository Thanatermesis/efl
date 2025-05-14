#ifndef EDJE_DECC_H
#define EDJE_DECC_H

/**
 * @file
 * @brief Header file for Edje Decompiler (edje_decc).
 *
 * Defines structures, macros, and function prototypes used by the
 * edje_decc tool.
 */

#include <edje_private.h>

/* logging variables */
extern int _edje_cc_log_dom; /**< Log domain for edje_decc, defined in edje_decc.c. */
#define EDJE_CC_DEFAULT_LOG_COLOR EINA_COLOR_CYAN /**< Default log color for edje_decc messages. */

/** @name Logging Macros
 *  Convenience macros for logging messages with the edje_decc log domain.
 *  @{
 */
#ifdef ERR
# undef ERR
#endif
#define ERR(...) EINA_LOG_DOM_ERR(_edje_cc_log_dom, __VA_ARGS__)
#ifdef INF
# undef INF
#endif
#define INF(...) EINA_LOG_DOM_INFO(_edje_cc_log_dom, __VA_ARGS__)
#ifdef WRN
# undef WRN
#endif
#define WRN(...) EINA_LOG_DOM_WARN(_edje_cc_log_dom, __VA_ARGS__)
#ifdef CRI
# undef CRI
#endif
#define CRI(...) EINA_LOG_DOM_CRIT(_edje_cc_log_dom, __VA_ARGS__)
#ifdef DBG
# undef DBG
#endif
#define DBG(...) EINA_LOG_DOM_DBG(_edje_cc_log_dom, __VA_ARGS__)
/** @} */

/* types */
/**
 * @brief Represents a single source file extracted from the Edje.
 *
 * This could be an .edc file, a Lua script, or any other text-based
 * resource that was part of the original Edje sources.
 */
typedef struct _SrcFile               SrcFile;
/**
 * @brief A list of SrcFile structures.
 */
typedef struct _SrcFile_List          SrcFile_List;

struct _SrcFile
{
   char *name; /**< The filename (e.g., "my_theme.edc", "scripts/utils.lua"). */
   char *file; /**< The content of the file as a null-terminated string. Can be NULL for empty files. */
};

struct _SrcFile_List
{
   Eina_List *list; /**< An Eina_List containing pointers to SrcFile structures. */
};

/**
 * @brief Initializes the Eet data descriptors for Edje source structures.
 *
 * This function is typically called once at the beginning to set up
 * how Eet should read/write Edje-specific data structures related to
 * source file information.
 */
void    source_edd(void);

/**
 * @brief Fetches source file information (placeholder or legacy function).
 *
 * The exact purpose in the current decompiler context is unclear;
 * it might be a remnant or for a different workflow.
 */
void    source_fetch(void);

/**
 * @brief Appends source file information to an Eet file.
 *
 * This function is likely used by the Edje compiler (edje_cc) to store
 * source file names and contents into the .edj file.
 * @param ef The Eet_File handle to write to.
 * @return Non-zero on error, 0 on success (typically, but check implementation).
 */
int     source_append(Eet_File *ef);

/**
 * @brief Loads the list of source files from an Eet file.
 *
 * This is used by the decompiler to retrieve the names and contents
 * of the original source files embedded in the .edj.
 * @param ef The Eet_File handle to read from.
 * @return A pointer to a SrcFile_List structure containing the loaded
 *         source files, or NULL on failure or if no source info is present.
 *         Example:
 *         ```c
 *         // ef is an opened Eet_File*
 *         SrcFile_List *sfl = source_load(ef);
 *         if (sfl) {
 *             Eina_List *l;
 *             SrcFile *sf_item;
 *             EINA_LIST_FOREACH(sfl->list, l, sf_item) {
 *                 printf("File: %s\nContent: %s\n", sf_item->name, sf_item->file ? sf_item->file : "(empty)");
 *             }
 *             // Remember to free sfl and its contents when done.
 *         }
 *         ```
 */
SrcFile_List *source_load(Eet_File *ef);

/**
 * @brief Saves font mapping information to an Eet file.
 *
 * Used by the Edje compiler to store the mapping between font names
 * used in the .edc and the actual font files embedded or referenced.
 * @param ef The Eet_File handle to write to.
 * @param fonts An Eina_List of Edje_Font structures (or similar) representing the font map.
 * @return Non-zero on error, 0 on success (typically, but check implementation).
 */
int     source_fontmap_save(Eet_File *ef, Eina_List *fonts);

/**
 * @brief Loads font mapping information from an Eet file.
 *
 * Used by the decompiler to retrieve the font map.
 * @param ef The Eet_File handle to read from.
 * @return An Edje_Font_List (which is an Eina_List of Edje_Font_Directory_Entry or similar)
 *         containing the loaded font mappings, or NULL on failure.
 *         Example:
 *         ```c
 *         // ef is an opened Eet_File*
 *         Edje_Font_List *font_list = source_fontmap_load(ef);
 *         if (font_list) {
 *             Eina_List *l;
 *             Edje_Font_Directory_Entry *font_entry;
 *             EINA_LIST_FOREACH(font_list, l, font_entry) {
 *                 printf("Font Entry: %s, Font File: %s\n", font_entry->entry, font_entry->file);
 *             }
 *             // Remember to free the list and its contents if necessary.
 *         }
 *         ```
 */
Edje_Font_List *source_fontmap_load(Eet_File *ef);

/**
 * @brief Allocates memory, exiting on failure.
 * @param size The number of bytes to allocate.
 * @return A pointer to the allocated memory.
 * @warning This function calls exit() if memory allocation fails.
 */
void   *mem_alloc(size_t size);

/**
 * @brief Duplicates a string, exiting on failure.
 * @param s The null-terminated string to duplicate.
 * @return A pointer to the newly allocated duplicated string.
 * @warning This function calls exit() if memory allocation fails.
 */
char   *mem_strdup(const char *s);

#define SZ sizeof /**< Convenience macro for sizeof operator. */

#endif
