#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <string.h>
#include <ctype.h>
#include <limits.h>

#include "edje_cc.h"

/** @internal
 * @brief Eet data descriptor for a single source file (SrcFile).
 * Used for serializing/deserializing SrcFile structures.
 */
static Eet_Data_Descriptor *_srcfile_edd = NULL;
/** @internal
 * @brief Eet data descriptor for a list of source files (SrcFile_List).
 * Used for serializing/deserializing SrcFile_List structures.
 */
static Eet_Data_Descriptor *_srcfile_list_edd = NULL;

/** @internal
 * @brief Eet data descriptor for an external resource (External).
 * Used for serializing/deserializing External structures.
 */
static Eet_Data_Descriptor *_external_edd = NULL;
/** @internal
 * @brief Eet data descriptor for a list of external resources (External_List).
 * Used for serializing/deserializing External_List structures.
 */
static Eet_Data_Descriptor *_external_list_edd = NULL;

/** @internal
 * @brief Eet data descriptor for a font entry (Edje_Font).
 * Used for serializing/deserializing Edje_Font structures.
 */
static Eet_Data_Descriptor *_font_edd = NULL;
/** @internal
 * @brief Eet data descriptor for a list of font entries (Edje_Font_List).
 * Used for serializing/deserializing Edje_Font_List structures.
 */
static Eet_Data_Descriptor *_font_list_edd = NULL;

/** @internal
 * @brief Global list holding all source files (main file and included files).
 * Each element is a SrcFile struct.
 * Example:
 * srcfiles.list might contain:
 *   - SrcFile for "main.edc"
 *   - SrcFile for "includes/buttons.edci" (if included by main.edc)
 */
static SrcFile_List srcfiles = {NULL};

/**
 * @internal
 * @brief Initializes all Eet data descriptors related to source files,
 * externals, and fonts.
 * This function must be called before any serialization or deserialization
 * of these data types.
 */
void
source_edd(void)
{
   Eet_Data_Descriptor_Class eddc;

   eet_eina_stream_data_descriptor_class_set(&eddc, sizeof (eddc), "srcfile", sizeof (SrcFile));
   _srcfile_edd = eet_data_descriptor_stream_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_srcfile_edd, SrcFile, "name", name, EET_T_INLINED_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_srcfile_edd, SrcFile, "file", file, EET_T_INLINED_STRING);

   eet_eina_stream_data_descriptor_class_set(&eddc, sizeof (eddc), "srcfile_list", sizeof (SrcFile_List));
   _srcfile_list_edd = eet_data_descriptor_stream_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_LIST(_srcfile_list_edd, SrcFile_List, "list", list, _srcfile_edd);

   eet_eina_stream_data_descriptor_class_set(&eddc, sizeof (eddc), "external", sizeof (External));
   _external_edd = eet_data_descriptor_stream_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_external_edd, External, "name", name, EET_T_INLINED_STRING);

   eet_eina_stream_data_descriptor_class_set(&eddc, sizeof (eddc), "external_list", sizeof (External_List));
   _external_list_edd = eet_data_descriptor_stream_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_LIST(_external_list_edd, External_List, "list", list, _external_edd);

   _edje_data_font_list_desc_make(&_font_list_edd, &_font_edd);
}

/**
 * @internal
 * @brief Forward declaration for recursive include processing.
 */
static void source_fetch_file(const char *fil, const char *filname);

/**
 * @internal
 * @brief Reads a source file, stores its content, and recursively processes
 *        #include directives found within it.
 *
 * This function opens the specified file, reads its entire content into
 * memory, and adds it to the global `srcfiles` list. It then scans the
 * file line by line for `#include` statements. For each valid include,
 * it recursively calls itself to process the included file.
 *
 * @param fil The full path to the source file to be processed.
 *            Example: "/path/to/project/themes/default.edc"
 * @param filname The name of the file as it should be stored (often the
 *                relative name used in the #include directive).
 *                Example: "default.edc" or "includes/common.edci"
 */
static void
source_fetch_file(const char *fil, const char *filname)
{
   FILE *f;
   char buf[16 * 1024], *dir = NULL;
   long sz;
   size_t tmp;
   ssize_t dir_len = 0;
   SrcFile *sf;

   f = fopen(fil, "rb");
   if (!f)
     {
        ERR("Cannot open file '%s'", fil);
        exit(-1);
     }

   if (fseek(f, 0, SEEK_END) < 0)
     ERR("Error seeking");
   sz = ftell(f);
   if (fseek(f, 0, SEEK_SET) < 0)
     ERR("Error seeking");
   sf = mem_alloc(SZ(SrcFile));
   sf->name = mem_strdup(filname);
   sf->file = mem_alloc(sz + 1);
   if (sz > 0)
     {
        tmp = fread(sf->file, sz, 1, f);
        if (tmp != 1)
          {
             ERR("file length for (%s) doesn't match!", filname);
             exit(-1);
          }
     }

   sf->file[sz] = '\0';
   if (fseek(f, 0, SEEK_SET) < 0)
     ERR("Error seeking");
   srcfiles.list = eina_list_append(srcfiles.list, sf);

   while (fgets(buf, sizeof(buf), f))
     {
        char *p, *pp;
        int forgetit = 0;
        int haveinclude = 0;
        char *file = NULL, *fname = NULL;

        p = buf;
        while ((!forgetit) && (*p))
          {
             if (!isspace(*p))
               {
                  if (*p != '#')
                    forgetit = 1;
               }
             p++;

             if (!haveinclude)
               {
                  if (!isspace(*p))
                    {
                       if (!strncmp(p, "include", 7))
                         {
                            haveinclude = 1;
                            p += 7;
                         }
                       /* HACK! the logic above should be fixed so
                        * preprocessor statements don't have to begin
                        * in column 0.
                        * otoh, edje_cc should print a warning in that case,
                        * since according to the standard, preprocessor
                        * statements need to be put in column 0.
                        */
                       else if (!strncmp(p, "#include", 8))
                         {
                            haveinclude = 1;
                            p += 8;
                         }
                       else
                         forgetit = 1;
                    }
               }
             else
               {
                  if (!isspace(*p))
                    {
                       char end = '\0';

                       if (*p == '"') end = '"';
                       else if (*p == '<')
                         end = '>';

                       if (end)
                         {
                            pp = strchr(p + 1, end);
                            if (!pp)
                              forgetit = 1;
                            else
                              {
                                 ssize_t l = 0;

                                 /* get the directory of the current file
                                  * if we haven't already done so
                                  */
                                 if (!dir)
                                   {
                                      dir = ecore_file_dir_get(fil);
                                      if (dir) dir_len = strlen(dir);
                                   }

                                 l = pp - p + dir_len + 1;
                                 file = mem_alloc(l);

                                 if (!dir_len)
                                   {
                                      snprintf(file, l - 1, "%s", p + 1);
                                      file[l - 2] = 0;
                                   }
                                 else
                                   {
                                      snprintf(file, l, "%s/%s", dir, p + 1);
                                      file[l - 1] = 0;
                                   }

                                 fname = strdup(p + 1);
                                 pp = strrchr(fname, end);
                                 if (pp) *pp = 0;
                                 forgetit = 1;
                              }
                         }
                       else
                         forgetit = 1;
                    }
                  else
                    p++;
               }
          }
        if ((file) && (fname))
          source_fetch_file(file, fname);

        if (file) free(file);
        if (fname) free(fname);
     }
   free(dir);
   fclose(f);
}

/**
 * @internal
 * @brief Initiates the source file fetching process.
 *
 * This function serves as the entry point for reading the main input EDC file
 * and all its recursively included files. It calls source_fetch_file()
 * with the main input file path and its base name.
 */
void
source_fetch(void)
{
   source_fetch_file(file_in, ecore_file_file_get(file_in));
}

/**
 * @internal
 * @brief Writes the collected source file data to an Eet file.
 *
 * Serializes the global `srcfiles` list (which contains the content of
 * the main EDC file and all its includes) into the provided Eet file
 * under the key "edje_sources".
 *
 * @param ef Pointer to the opened Eet_File to write to.
 * @return Returns 1 on success, 0 on failure.
 *         (Corresponds to eet_data_write return value).
 */
int
source_append(Eet_File *ef)
{
   return eet_data_write(ef, _srcfile_list_edd, "edje_sources", &srcfiles,
                         compress_mode);
}

/**
 * @internal
 * @brief Loads source file data from an Eet file.
 *
 * Deserializes the source file list (SrcFile_List) stored under the key
 * "edje_sources" from the provided Eet file.
 *
 * @param ef Pointer to the opened Eet_File to read from.
 * @return A pointer to the loaded SrcFile_List structure, or NULL on failure.
 *         The caller is responsible for freeing the returned structure if not NULL.
 *         Example of returned structure:
 *         SrcFile_List {
 *           list: Eina_List of SrcFile* {
 *             SrcFile { name: "main.edc", file: "content of main.edc..." },
 *             SrcFile { name: "include1.edci", file: "content of include1.edci..." }
 *           }
 *         }
 */
SrcFile_List *
source_load(Eet_File *ef)
{
   SrcFile_List *s;

   s = eet_data_read(ef, _srcfile_list_edd, "edje_sources");
   return s;
}

/**
 * @internal
 * @brief Saves the font map (list of font names and their file paths) to an Eet file.
 *
 * Serializes the provided list of fonts into the Eet file under the key
 * "edje_source_fontmap".
 *
 * @param ef Pointer to the opened Eet_File to write to.
 * @param font_list An Eina_List where each item is an Edje_Font struct
 *                  (or compatible, as handled by _font_edd).
 *                  Example:
 *                  font_list might contain:
 *                    - Edje_Font { name: "Sans", file: "/usr/share/fonts/TTF/DejaVuSans.ttf" }
 *                    - Edje_Font { name: "Mono", file: "/usr/share/fonts/TTF/DejaVuSansMono.ttf" }
 * @return Returns 1 on success, 0 on failure.
 *         (Corresponds to eet_data_write return value).
 */
int
source_fontmap_save(Eet_File *ef, Eina_List *font_list)
{
   Edje_Font_List fl;

   fl.list = font_list;
   return eet_data_write(ef, _font_list_edd, "edje_source_fontmap", &fl,
                         compress_mode);
}

/**
 * @internal
 * @brief Loads the font map from an Eet file.
 *
 * Deserializes the font list (Edje_Font_List) stored under the key
 * "edje_source_fontmap" from the provided Eet file.
 *
 * @param ef Pointer to the opened Eet_File to read from.
 * @return A pointer to the loaded Edje_Font_List structure, or NULL on failure.
 *         The caller is responsible for freeing the returned structure if not NULL.
 *         Example of returned structure:
 *         Edje_Font_List {
 *           list: Eina_List of Edje_Font* {
 *             Edje_Font { name: "Sans", file: "/usr/share/fonts/TTF/DejaVuSans.ttf" },
 *             Edje_Font { name: "Mono", file: "/usr/share/fonts/TTF/DejaVuSansMono.ttf" }
 *           }
 *         }
 */
Edje_Font_List *
source_fontmap_load(Eet_File *ef)
{
   Edje_Font_List *fl;

   fl = eet_data_read(ef, _font_list_edd, "edje_source_fontmap");
   return fl;
}

