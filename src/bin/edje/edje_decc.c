/**
 * @file
 * @brief Edje Decompiler (edje_decc)
 *
 * This program decompiles Edje binary files (.edj) back into
 * their source (.edc) and resource files (images, fonts, sounds).
 *
 * It reads an .edj file, extracts its components, and reconstructs
 * the original source structure as closely as possible.
 *
 * @note Some information, like lossy compression details or original
 *       image formats if converted, might not be perfectly recoverable.
 */

/* ugly ugly. avert your eyes. */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <locale.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include <Ecore_File.h>
#include <Ecore_Evas.h>

#include "edje_decc.h"

int _edje_cc_log_dom = -1; /**< Log domain for edje_decc. */
static const char *progname = NULL; /**< Program name, extracted from argv[0]. */
char *file_in = NULL; /**< Path to the input .edj file. */
char *file_out = NULL; /**< Path for the main output .edc file (symbolic link target). */
char *outdir = NULL; /**< Directory where decompiled files will be saved. */
int compress_mode = EET_COMPRESSION_DEFAULT; /**< Compression mode for Eet (not actively used in decompiler). */

Edje_File *edje_file = NULL; /**< Main Edje file structure loaded from the .edj. */
SrcFile_List *srcfiles = NULL; /**< List of source files (.edc, .lua, etc.) extracted. */
Edje_Font_List *fontlist = NULL; /**< List of fonts used in the Edje file. */

int line = 0; /**< Current line number during parsing (not actively used in decompiler). */
int build_sh = 1; /**< Flag to control generation of build.sh script (1 = yes, 0 = no). */
int new_dir = 1; /**< Flag to control output directory creation (1 = create new subdir, 0 = use current dir). */

/**
 * @brief Decompiles the input Edje file.
 *
 * Opens the .edj file, reads its structure, and extracts
 * source file information, Edje file data, and font mappings.
 * @return 1 on success, 0 on failure.
 */
int        decomp(void);

/**
 * @brief Writes the decompiled files to the output directory.
 *
 * This function handles:
 * - Creating the output directory if needed.
 * - Extracting and saving images.
 * - Writing out the source files (.edc, etc.).
 * - Extracting and saving fonts.
 * - Generating a `build.sh` script to recompile the Edje.
 * - Creating a symbolic link to the main .edc file if requested.
 * - Extracting and saving sound samples.
 * - Extracting and saving vibration samples.
 */
void       output(void);
/**
 * @brief Checks if the compiler command stored in the Edje file is sane.
 *
 * A "sane" command consists only of alphanumeric characters, underscores, and hyphens.
 * This is a security measure to prevent execution of arbitrary commands.
 * @return 1 if sane, 0 otherwise.
 * @see edje_file
 * @see edje_file::compiler
 */
static int compiler_cmd_is_sane(void);
/**
 * @brief Checks if the root filename (main .edc file) is sane.
 *
 * A "sane" filename consists of alphanumeric characters, underscores, hyphens,
 * dots, and forward slashes. This is a security measure to prevent writing
 * files to unintended locations or with malicious names.
 * @return 1 if sane, 0 otherwise.
 * @see srcfiles
 * @see SrcFile_List::list
 * @see SrcFile::name
 */
static int root_filename_is_sane(void);

/**
 * @brief Custom log callback for edje_decc.
 *
 * This function formats log messages from the "edje_decc" domain,
 * adding color and prefixes based on the log level. Other domains
 * are passed to the default Eina stderr log printer.
 *
 * @param d The log domain.
 * @param level The log level.
 * @param file The source file where the log message originated.
 * @param fnc The function where the log message originated.
 * @param cur_line The line number where the log message originated.
 * @param fmt The format string for the log message.
 * @param data User data (unused).
 * @param args Variable arguments for the format string.
 */
static void
_edje_cc_log_cb(const Eina_Log_Domain *d,
                Eina_Log_Level level,
                const char *file,
                const char *fnc,
                int cur_line,
                const char *fmt,
                EINA_UNUSED void *data,
                va_list args)
{
   if ((d->name) && (d->namelen == sizeof("edje_decc") - 1) &&
       (memcmp(d->name, "edje_decc", sizeof("edje_decc") - 1) == 0))
     {
        const char *prefix;
        Eina_Bool use_color = !eina_log_color_disable_get();

        if (use_color)
          {
#ifndef _WIN32
             fputs(eina_log_level_color_get(level), stderr);
#else
             int color;
             switch (level)
               {
                case EINA_LOG_LEVEL_CRITICAL:
                  color = FOREGROUND_RED | FOREGROUND_INTENSITY;
                  break;

                case EINA_LOG_LEVEL_ERR:
                  color = FOREGROUND_RED;
                  break;

                case EINA_LOG_LEVEL_WARN:
                  color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
                  break;

                case EINA_LOG_LEVEL_INFO:
                  color = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
                  break;

                case EINA_LOG_LEVEL_DBG:
                  color = FOREGROUND_BLUE | FOREGROUND_INTENSITY;
                  break;

                default:
                  color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
               }
             SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
#endif
          }

        switch (level)
          {
           case EINA_LOG_LEVEL_CRITICAL:
             prefix = "Critical. ";
             break;

           case EINA_LOG_LEVEL_ERR:
             prefix = "Error. ";
             break;

           case EINA_LOG_LEVEL_WARN:
             prefix = "Warning. ";
             break;

           default:
             prefix = "";
          }
        fprintf(stderr, "%s: %s", progname, prefix);

        if (use_color)
          {
#ifndef _WIN32
             fputs(EINA_COLOR_RESET, stderr);
#else
             SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
                                     FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#endif
          }

        vfprintf(stderr, fmt, args);
        putc('\n', stderr);
     }
   else
     eina_log_print_cb_stderr(d, level, file, fnc, cur_line, fmt, NULL, args);
}

/**
 * @brief Prints the command-line help message.
 */
static void
main_help(void)
{
   printf
     ("Usage:\n"
      "\t%s input_file.edj [-main-out file.edc] [-no-build-sh] [-current-dir | -output path_to_dir]\n"
      "\n"
      " -main-out\tCreate a symbolic link to the main edc (disabled on Windows) \n"
      " -no-build-sh\tDon't output build.sh \n"
      " -output, -o\tOutput to specified directory \n"
      " -current-dir\tOutput to current directory \n"
      " -quiet\t\tProduce less output\n"
      "\n"
     , progname);
}

Eet_File *ef; /**< Eet file handle for the input .edj file. */
Eet_Dictionary *ed; /**< Eet dictionary (not actively used in this scope). */

/**
 * @brief Main entry point for the edje_decc application.
 *
 * Parses command-line arguments, initializes Eina and Edje,
 * calls the decompilation and output functions, and then cleans up.
 *
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line argument strings.
 * @return 0 on success, -1 on failure.
 */
int
main(int argc, char **argv)
{
   int i;

   setlocale(LC_NUMERIC, "C");

   ecore_app_no_system_modules();

   if (!eina_init())
     exit(-1);
   _edje_cc_log_dom = eina_log_domain_register
       ("edje_decc", EDJE_CC_DEFAULT_LOG_COLOR);
   if (_edje_cc_log_dom < 0)
     {
        EINA_LOG_ERR("Impossible to create a log domain.");
        eina_shutdown();
        exit(-1);
     }
   progname = ecore_file_file_get(argv[0]);
   eina_log_print_cb_set(_edje_cc_log_cb, NULL);
   eina_log_domain_level_set("edje_decc", EINA_LOG_LEVEL_INFO);

   for (i = 1; i < argc; i++)
     {
        if (!strcmp(argv[i], "-h"))
          {
             main_help();
             exit(0);
          }
        if (!file_in)
          file_in = argv[i];
        else if ((!strcmp(argv[i], "-main-out")) && (i < (argc - 1)))
          {
             i++;
#ifndef _WIN32
             file_out = argv[i];
#endif
          }
        else if (!strcmp(argv[i], "-no-build-sh"))
          build_sh = 0;
        else if (!strcmp(argv[i], "-current-dir"))
          new_dir = 0;
        else if (!strcmp(argv[i], "-quiet"))
          eina_log_domain_level_set("edje_decc", EINA_LOG_LEVEL_WARN);
        else if ((!strcmp(argv[i], "-o") || !strcmp(argv[i], "-output")) && (i < (argc - 1)))
          {
             i++;
             outdir = strdup(argv[i]);
          }
     }
   if (!file_in)
     {
        ERR("no input file specified.");
        main_help();
        exit(-1);
     }

   if (!edje_init())
     exit(-1);
   source_edd();

   if (!decomp()) return -1;
   output();

   WRN("If any Image or audio data was encoded in a LOSSY way, then "
       "re-encoding will drop quality even more. "
       "You need access to the original data to ensure no loss of quality.");
   eet_close(ef);
   edje_shutdown();
   eina_log_domain_unregister(_edje_cc_log_dom);
   _edje_cc_log_dom = -1;
   eina_shutdown();
   return 0;
}

/**
 * @brief Implements the decompilation logic.
 *
 * This function is the core of the decompilation process. It opens the
 * input .edj file using Eet, loads the embedded source files, validates the
 * main source filename, reads the main Edje file data structure, and loads
 * the font map. It populates the global `edje_file`, `srcfiles`, and
 * `fontlist` structures.
 *
 * @note The function overrides the compiler command found in the Edje file
 *       to "edje_cc" for security and consistency. It also performs a sanity
 *       check on the original compiler command.
 *
 * @return 1 on success, 0 on failure (e.g., file not found, not a valid
 *         Edje file, no decompile information present).
 */
int
decomp(void)
{
   ef = eet_open(file_in, EET_FILE_MODE_READ);
   if (!ef)
     {
        ERR("cannot open %s", file_in);
        return 0;
     }

   srcfiles = source_load(ef);
   if (!srcfiles || !srcfiles->list)
     {
        ERR("%s has no decompile information", file_in);
        eet_close(ef);
        return 0;
     }
   if (!eina_list_data_get(srcfiles->list) || !root_filename_is_sane())
     {
        ERR("Invalid root filename: '%s'", (char *)eina_list_data_get(srcfiles->list));
        eet_close(ef);
        return 0;
     }
   edje_file = eet_data_read(ef, _edje_edd_edje_file, "edje/file");
   if (!edje_file)
     {
        ERR("%s does not appear to be an edje file", file_in);
        eet_close(ef);
        return 0;
     }
   /* force compiler to be edje_cc */
   edje_file->compiler = strdup("edje_cc");
   if (!edje_file->compiler)
     {
        edje_file->compiler = strdup("edje_cc");
     }
   else if (!compiler_cmd_is_sane())
     {
        ERR("invalid compiler executable: '%s'", edje_file->compiler);
        eet_close(ef);
        return 0;
     }
   fontlist = source_fontmap_load(ef);
   return 1;
}

/**
 * @brief Writes the decompiled files to the output directory.
 *
 * This function handles:
 * - Creating the output directory if needed.
 * - Extracting and saving images.
 * - Writing out the source files (.edc, etc.).
 * - Extracting and saving fonts.
 * - Generating a `build.sh` script to recompile the Edje.
 * - Creating a symbolic link to the main .edc file if requested.
 * - Extracting and saving sound samples.
 * - Extracting and saving vibration samples.
 */
void
output(void)
{
   Eina_List *l;
   Eet_File *tef;
   SrcFile *sf;
   char *p;

   if (!outdir)
     {
        if (!new_dir)
          outdir = strdup(".");
        else
          {
             p = strrchr(file_in, '/');
             if (p)
               outdir = strdup(p + 1);
             else
               outdir = strdup(file_in);
             p = strrchr(outdir, '.');
             if (p) *p = 0;
             ecore_file_mkpath(outdir);
          }
     }

   tef = eet_open(file_in, EET_FILE_MODE_READ);

   if (edje_file->image_dir)
     {
        Edje_Image_Directory_Entry *ei;
        unsigned int i;

        for (i = 0; i < edje_file->image_dir->entries_count; ++i)
          {
             ei = &edje_file->image_dir->entries[i];

             if ((ei->source_type > EDJE_IMAGE_SOURCE_TYPE_NONE) &&
                 (ei->source_type < EDJE_IMAGE_SOURCE_TYPE_LAST) &&
                 (ei->source_type != EDJE_IMAGE_SOURCE_TYPE_USER) &&
                 (ei->source_type != EDJE_IMAGE_SOURCE_TYPE_EXTERNAL) &&
                 (ei->entry))
               {
                  Ecore_Evas *ee;
                  Evas *evas;
                  Evas_Object *im;
                  char buf[4096];
                  char out[4096];
                  char *pp;

                  ecore_init();
                  ecore_evas_init();
                  ee = ecore_evas_buffer_new(1, 1);
                  if (!ee)
                    {
                       ERR("Cannot create buffer engine canvas for image save.");
                       exit(-1);
                    }
                  evas = ecore_evas_get(ee);
                  im = evas_object_image_add(evas);
                  if (!im)
                    {
                       ERR("Cannot create image object for save.");
                       exit(-1);
                    }
                  snprintf(buf, sizeof(buf), "edje/images/%i", ei->id);
                  evas_object_image_file_set(im, file_in, buf);
                  snprintf(out, sizeof(out), "%s/%s", outdir, ei->entry);
                  INF("Output Image: %s", out);
                  pp = strdup(out);
                  p = strrchr(pp, '/');
                  if (p) *p = 0;
                  if (strstr(pp, "../"))
                    {
                       ERR("Potential security violation. attempt to write in parent dir.");
                       exit(-1);
                    }
                  ecore_file_mkpath(pp);
                  free(pp);
                  if (!evas_object_image_save(im, out, NULL, "quality=100 compress=9 encoding=auto"))
                    {
                       ERR("Cannot write file %s. Perhaps missing JPEG or PNG saver modules for Evas.", out);
                       exit(-1);
                    }
                  evas_object_del(im);
                  ecore_evas_free(ee);
                  ecore_evas_shutdown();
                  ecore_shutdown();
               }
          }
     }

   EINA_LIST_FOREACH(srcfiles->list, l, sf)
     {
        char out[4096];
        FILE *f;
        char *pp;

        snprintf(out, sizeof(out), "%s/%s", outdir, sf->name);
        INF("Output Source File: %s", out);
        pp = strdup(out);
        p = strrchr(pp, '/');
        if (p) *p = 0;
        if (strstr(pp, "../"))
          {
             ERR("Potential security violation. attempt to write in parent dir.");
             exit(-1);
          }
        ecore_file_mkpath(pp);
        free(pp);
        if (strstr(out, "../"))
          {
             ERR("Potential security violation. attempt to write in parent dir.");
             exit(-1);
          }
        f = fopen(out, "wb");
        if (!f)
          {
             ERR("Unable to write file (%s).", out);
             exit(-1);
          }

        /* if the file is empty, sf->file will be NULL.
         * note that that's not an error
         */
        if (sf->file) fputs(sf->file, f);
        fclose(f);
     }
   if (edje_file->fonts)
     {
        Edje_Font_Directory_Entry *fn;
        Eina_Iterator *it;

        it = eina_hash_iterator_data_new(edje_file->fonts);
        EINA_ITERATOR_FOREACH(it, fn)
          {
             void *font;
             int fontsize;
             char out[4096];
             /* FIXME!!!! */
             /* should be fn->entry -v */
             snprintf(out, sizeof(out), "edje/fonts/%s", fn->file);
             font = eet_read(tef, out, &fontsize);
             if (font)
               {
                  FILE *f;
                  char *pp;

                  /* should be fn->file -v */
                  snprintf(out, sizeof(out), "%s/%s", outdir, fn->entry);
                  INF("Output Font: %s", out);
                  pp = strdup(out);
                  p = strrchr(pp, '/');
                  if (p) *p = 0;
                  if (strstr(pp, "../"))
                    {
                       ERR("Potential security violation. attempt to write in parent dir.");
                       exit(-1);
                    }
                  ecore_file_mkpath(pp);
                  free(pp);
                  if (strstr(out, "../"))
                    {
                       ERR("Potential security violation. attempt to write in parent dir.");
                       exit(-1);
                    }
                  if (!(f = fopen(out, "wb")))
                    {
                       ERR("Could not open file: %s", out);
                       exit(-1);
                    }
                  if (fwrite(font, fontsize, 1, f) != 1)
                    ERR("Could not write font: %s", strerror(errno));
                  if (f) fclose(f);
                  free(font);
               }
          }
        eina_iterator_free(it);
     }
   {
      char out[4096];
      FILE *f;
      sf = eina_list_data_get(srcfiles->list);

      if (build_sh)
        {
           snprintf(out, sizeof(out), "%s/build.sh", outdir);
           INF("Output Build Script: %s", out);
           if (strstr(out, "../"))
             {
                ERR("potential security violation. attempt to write in parent dir.");
                exit(-1);
             }
           if ((f = fopen(out, "wb")))
             {
                fprintf(f, "#!/bin/sh\n");
                fprintf(f, "%s $@ -id . -fd . %s -o %s.edj\n",
                        edje_file->compiler, sf->name, outdir);
                fclose(f);
                if (chmod(out,
                          S_IRUSR | S_IWUSR | S_IXUSR |
                          S_IRGRP | S_IWGRP | S_IXGRP) < 0)
                  ERR("chmod on %s failed", out);
             }

           WRN("*** CAUTION ***\n"
               "Please check the build script for anything malicious "
               "before running it!\n\n");
        }

      if (file_out)
        {
           snprintf(out, sizeof(out), "%s/%s", outdir, file_out);
           if (ecore_file_symlink(sf->name, out) != EINA_TRUE)
             {
                ERR("symlink %s -> %s failed", sf->name, out);
             }
        }
   }

   if (edje_file->sound_dir)
     {
        Edje_Sound_Sample *sample;
        void *sound_data;
        char out[PATH_MAX];
        char out1[PATH_MAX];
        char *pp;
        int sound_data_size;
        FILE *f;
        int i;

        for (i = 0; i < (int)edje_file->sound_dir->samples_count; i++)
          {
             sample = &edje_file->sound_dir->samples[i];
             if ((!sample) || (!sample->name)) continue;
             snprintf(out, sizeof(out), "edje/sounds/%i", sample->id);
             sound_data = (void *)eet_read_direct(tef, out, &sound_data_size);
             if (sound_data)
               {
                  snprintf(out1, sizeof(out1), "%s/%s", outdir, sample->snd_src);
                  pp = strdup(out1);
                  p = strrchr(pp, '/');
                  if (p) *p = 0;
                  if (strstr(pp, "../"))
                    {
                       ERR("Potential security violation. attempt to write in parent dir.");
                       exit(-1);
                    }
                  ecore_file_mkpath(pp);
                  free(pp);
                  if (strstr(out, "../"))
                    {
                       ERR("Potential security violation. attempt to write in parent dir.");
                       exit(-1);
                    }
                  f = fopen(out1, "wb");
                  if (f)
                    {
                       if (fwrite(sound_data, sound_data_size, 1, f) != 1)
                         ERR("Could not write sound: %s: %s", out1, strerror(errno));
                       fclose(f);
                    }
                  else ERR("Could not open for writing sound: %s: %s", out1, strerror(errno));
               }
          }
     }
   if (edje_file->vibration_dir)
     {
        Edje_Vibration_Sample *sample;
        void *data;
        char out[PATH_MAX];
        char out1[PATH_MAX];
        char *pp;
        int data_size;
        FILE *f;
        int i;

        for (i = 0; i < (int)edje_file->vibration_dir->samples_count; i++)
          {
             sample = &edje_file->vibration_dir->samples[i];
             if ((!sample) || (!sample->name)) continue;
             snprintf(out, sizeof(out), "edje/vibrations/%i", sample->id);
             data = (void *)eet_read_direct(tef, out, &data_size);
             if (data)
               {
                  snprintf(out1, sizeof(out1), "%s/%s", outdir, sample->src);
                  pp = strdup(out1);
                  p = strrchr(pp, '/');
                  if (p) *p = 0;
                  if (strstr(pp, "../"))
                    {
                       ERR("Potential security violation. attempt to write in parent dir.");
                       exit(-1);
                    }
                  ecore_file_mkpath(pp);
                  free(pp);
                  if (strstr(out, "../"))
                    {
                       ERR("Potential security violation. attempt to write in parent dir.");
                       exit(-1);
                    }
                  f = fopen(out1, "wb");
                  if (f)
                    {
                       if (fwrite(data, data_size, 1, f) != 1)
                         ERR("Could not write sound: %s", strerror(errno));
                       fclose(f);
                    }
                  else ERR("Could not open for writing sound: %s: %s", out1, strerror(errno));
               }
          }
     }

   eet_close(tef);
   if (outdir) free(outdir);
}

/**
 * @brief Checks if the compiler command stored in the Edje file is sane.
 *
 * A "sane" command consists only of alphanumeric characters, underscores, and hyphens.
 * This is a security measure to prevent execution of arbitrary commands.
 * @return 1 if sane, 0 otherwise.
 * @see edje_file
 * @see edje_file::compiler
 */
static int
compiler_cmd_is_sane()
{
   const char *c = edje_file->compiler, *ptr;

   if ((!c) || (!*c))
     {
        return 0;
     }

   for (ptr = c; ptr && *ptr; ptr++)
     {
        /* only allow [a-z][A-Z][0-9]_- */
        if ((!isalnum(*ptr)) && (*ptr != '_') && (*ptr != '-'))
          {
             return 0;
          }
     }

   return 1;
}

/**
 * @brief Checks if the root filename (main .edc file) is sane.
 *
 * A "sane" filename consists of alphanumeric characters, underscores, hyphens,
 * dots, and forward slashes. This is a security measure to prevent writing
 * files to unintended locations or with malicious names.
 * @return 1 if sane, 0 otherwise.
 * @see srcfiles
 * @see SrcFile_List::list
 * @see SrcFile::name
 */
static int
root_filename_is_sane()
{
   SrcFile *sf = eina_list_data_get(srcfiles->list);
   char *f = sf->name, *ptr;

   if (!f || !*f)
     {
        return 0;
     }

   for (ptr = f; ptr && *ptr; ptr++)
     {
        /* only allow [a-z][A-Z][0-9]_-./ */
        switch (*ptr)
          {
           case '_':
           case '-':
           case '.':
           case '/':
             break;

           default:
             if (!isalnum(*ptr))
               {
                  return 0;
               }
          }
     }
   return 1;
}

