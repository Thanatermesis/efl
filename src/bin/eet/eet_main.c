#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* ifdef HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <Eet.h>

static int _eet_main_log_dom = -1;

#ifdef EET_DEFAULT_LOG_COLOR
#undef EET_DEFAULT_LOG_COLOR
#endif /* ifdef EET_DEFAULT_LOG_COLOR */
#define EET_DEFAULT_LOG_COLOR EINA_COLOR_CYAN
#ifdef ERR
#undef ERR
#endif /* ifdef ERR */
#define ERR(...)  EINA_LOG_DOM_ERR(_eet_main_log_dom, __VA_ARGS__)
#ifdef DBG
#undef DBG
#endif /* ifdef DBG */
#define DBG(...)  EINA_LOG_DOM_DBG(_eet_main_log_dom, __VA_ARGS__)
#ifdef INF
#undef INF
#endif /* ifdef INF */
#define INF(...)  EINA_LOG_DOM_INFO(_eet_main_log_dom, __VA_ARGS__)
#ifdef WRN
#undef WRN
#endif /* ifdef WRN */
#define WRN(...)  EINA_LOG_DOM_WARN(_eet_main_log_dom, __VA_ARGS__)
#ifdef CRI
#undef CRI
#endif /* ifdef CRI */
#define CRI(...) EINA_LOG_DOM_CRIT(_eet_main_log_dom, __VA_ARGS__)

/**
 * @brief Lists all entries in an Eet file.
 *
 * This function opens an Eet file, iterates through its entries,
 * and prints their names. If verbose mode is enabled, it also
 * prints additional details like alias information, offset, size,
 * and uncompressed size for each entry, as well as the total
 * payload size.
 *
 * @param file The path to the Eet file.
 * @param verbose If EINA_TRUE, print detailed information for each entry.
 *                Otherwise, print only entry names.
 */
static void
do_eet_list(const char *file, Eina_Bool verbose)
{
   Eina_Iterator *it;
   Eet_Entry *entry;
   Eet_File *ef;
   unsigned long long total = 0;

   ef = eet_open(file, EET_FILE_MODE_READ);
   if (!ef)
     {
        ERR("cannot open for reading: %s", file);
        exit(-1);
     }

   it = eet_list_entries(ef);
   EINA_ITERATOR_FOREACH(it, entry)
     {
        if (verbose)
          {
             if (entry->alias)
               {
                  printf("%s is an alias for %s\n",
                         entry->name, eet_alias_get(ef, entry->name));
               }
             else
               {
                  if (entry->compression)
                    printf("%s start at %i with a size of %i Bytes with an uncompressed size of %i Bytes.\n",
                           entry->name, entry->offset, entry->size, entry->data_size);
                  else
                    printf("%s start at %i with a size of %i Bytes.\n",
                           entry->name, entry->offset, entry->size);
                  total += entry->size;
               }
          }
        else
          {
             printf("%s\n", entry->name);
          }
     }
   eina_iterator_free(it);

   if (verbose)
     {
        printf("*** ***\n");
        printf("Total payload size : %llu.\n", total);
     }

   eet_close(ef);
} /* do_eet_list */

/**
 * @brief Prints statistics about an Eet file.
 *
 * This function opens an Eet file and displays statistics, including:
 * - Information about each section (compressed or not, size).
 * - Dictionary information (number of strings).
 * - Global statistics (total sections, number and percentage of
 *   compressed/uncompressed sections, and their respective total sizes).
 *
 * @param file The path to the Eet file.
 */
static void
do_eet_stats(const char *file)
{
   int i, num;
   int count[2] = { 0, 0 };
   int size[2] = { 0, 0 };
   char **list;
   Eet_File *ef;
   Eet_Dictionary *ed;

   ef = eet_open(file, EET_FILE_MODE_READ);
   if (!ef)
     {
        ERR("cannot open for reading: %s", file);
        exit(-1);
     }

   printf("*** sections stats ***\n");
   list = eet_list(ef, "*", &num);
   if (list)
     {
        for (i = 0; i < num; i++)
          {
             const void *ro = NULL;
             void *rw = NULL;
             int tsize;

             ro = eet_read_direct(ef, list[i], &tsize);
             if (!ro) rw = eet_read(ef, list[i], &tsize);
             printf(rw ? "%s of size %i is compressed.\n" : "%s of size %i is not compressed.\n", list[i], tsize);
             count[rw ? 0 : 1]++;
             size[rw ? 0 : 1] += tsize;
             free(rw);
          }
        free(list);
     }

   printf("*** dictionary ***\n");
   ed = eet_dictionary_get(ef);
   if (ed)
     {
        printf("%i strings inside the dictionary.\n", eet_dictionary_count(ed));
     }
   else
     {
        printf("no dictionary in this file.\n");
     }
   printf("*** global ***\n");
   printf("%i sections\n", num);
   printf("- %i of them are compressed (%02.2f%%) expanding in %i bytes.\n",
          count[0], (float) count[0] * 100 / (float) num, size[0]);
   printf("- %i of them are directly mappable in memory (%02.2f%%) representing %i bytes.\n",
          count[1], (float) count[1] * 100 / (float) num, size[1]);

   eet_close(ef);
}

/**
 * @brief Extracts data associated with a key from an Eet file.
 *
 * This function opens an Eet file, reads the data associated with the
 * specified key (optionally decrypting it if a crypto_key is provided),
 * and writes it to an output file or standard output.
 *
 * @param file The path to the Eet file.
 * @param key The key of the data entry to extract.
 * @param out The path to the output file. If NULL, data is written to stdout.
 * @param crypto_key The key for decryption. If NULL, no decryption is performed.
 */
static void
do_eet_extract(const char *file,
               const char *key,
               const char *out,
               const char *crypto_key)
{
   Eet_File *ef;
   void *data;
   int size = 0;
   FILE *f = stdout;

   ef = eet_open(file, EET_FILE_MODE_READ);
   if (!ef)
     {
        ERR("cannot open for reading: %s", file);
        exit(-1);
     }

   data = eet_read_cipher(ef, key, &size, crypto_key);
   if (!data)
     {
        ERR("cannot read key %s", key);
        exit(-1);
     }

   if (out)
     {
        f = fopen(out, "wb");
        if (!f)
          {
             ERR("cannot open %s", out);
             exit(-1);
          }
     }

   if (fwrite(data, size, 1, f) != 1)
     {
        ERR("cannot write to %s", out ? out : "standard output");
        exit(-1);
     }

   if (out) fclose(f);
   free(data);
   eet_close(ef);
} /* do_eet_extract */

/**
 * @brief Callback function to write decoded string data to a file.
 *
 * This function is used by eet_data_dump_cipher (via do_eet_decode)
 * to write chunks of decoded string data.
 *
 * @param data A pointer to a FILE structure where the string will be written.
 * @param str The null-terminated string to write.
 */
static void
do_eet_decode_dump(void       *data,
                   const char *str)
{
   fputs(str, (FILE *)data);
} /* do_eet_decode_dump */

/**
 * @brief Decodes and dumps data associated with a key from an Eet file.
 *
 * This function opens an Eet file, decodes the data associated with the
 * specified key (optionally decrypting it if a crypto_key is provided),
 * and writes the decoded string representation to an output file or
 * standard output using the do_eet_decode_dump callback.
 *
 * @param file The path to the Eet file.
 * @param key The key of the data entry to decode.
 * @param out The path to the output file. If NULL, data is written to stdout.
 * @param crypto_key The key for decryption. If NULL, no decryption is performed.
 */
static void
do_eet_decode(const char *file,
              const char *key,
              const char *out,
              const char *crypto_key)
{
   Eet_File *ef;
   FILE *f = stdout;

   ef = eet_open(file, EET_FILE_MODE_READ);
   if (!ef)
     {
        ERR("cannot open for reading: %s", file);
        exit(-1);
     }

   if (out)
     {
        f = fopen(out, "wb");
        if (!f)
          {
             ERR("cannot open %s", out);
             exit(-1);
          }
     }

   if (!eet_data_dump_cipher(ef, key, crypto_key, do_eet_decode_dump, f))
     {
        ERR("cannot write to %s", out ? out : "standard output");
        exit(-1);
     }

   if (out) fclose(f);
   eet_close(ef);
} /* do_eet_decode */

/**
 * @brief Inserts data from a file into an Eet file under a specified key.
 *
 * This function reads data from an input file and writes it into an Eet file
 * associated with the given key. The data can be optionally compressed and
 * encrypted. If the Eet file does not exist, it will be created. If it
 * exists, it will be opened in read-write mode.
 *
 * @param file The path to the Eet file.
 * @param key The key under which to store the data in the Eet file.
 * @param in The path to the input file containing the data to insert.
 * @param compress An integer flag (0 or 1). If 1, the data will be compressed.
 * @param crypto_key The key for encryption. If NULL, no encryption is performed.
 */
static void
do_eet_insert(const char *file,
              const char *key,
              const char *in,
              int         compress,
              const char *crypto_key)
{
   Eet_File *ef;
   void *data;
   int size = 0;
   FILE *f;

   ef = eet_open(file, EET_FILE_MODE_READ_WRITE);
   if (!ef)
     ef = eet_open(file, EET_FILE_MODE_WRITE);

   if (!ef)
     {
        ERR("cannot open for read+write: %s", file);
        exit(-1);
     }

   f = fopen(in, "rb");
   if (!f)
     {
        ERR("cannot open %s", in);
        exit(-1);
     }

   fseek(f, 0, SEEK_END);
   size = ftell(f);
   if (size < 0)
     {
        ERR("cannot obtain current file position %s", in);
        fclose(f);
        exit(-1);
     }

   rewind(f);
   data = malloc(size);
   if (!data)
     {
        ERR("cannot allocate %i bytes", size);
        exit(-1);
     }

   if (fread(data, size, 1, f) != 1)
     {
        ERR("cannot read file %s", in);
        exit(-1);
     }

   fclose(f);
   eet_write_cipher(ef, key, data, size, compress, crypto_key);
   free(data);
   eet_close(ef);
} /* do_eet_insert */

/**
 * @brief Encodes and inserts text data from a file into an Eet file.
 *
 * This function reads text data from an input file, encodes it (which might
 * involve parsing it based on an Eet data descriptor if one were used, though
 * this function uses eet_data_undump_cipher which implies a simpler text dump),
 * and writes it into an Eet file associated with the given key. The data can be
 * optionally compressed and encrypted. If the Eet file does not exist, it will
 * be created. If it exists, it will be opened in read-write mode.
 *
 * @param file The path to the Eet file.
 * @param key The key under which to store the encoded data in the Eet file.
 * @param in The path to the input file containing the text data to encode.
 * @param compress An integer flag (0 or 1). If 1, the data will be compressed.
 * @param crypto_key The key for encryption. If NULL, no encryption is performed.
 */
static void
do_eet_encode(const char *file,
              const char *key,
              const char *in,
              int         compress,
              const char *crypto_key)
{
   Eet_File *ef;
   char *text;
   int textlen = 0;
   int size = 0;
   FILE *f;

   ef = eet_open(file, EET_FILE_MODE_READ_WRITE);
   if (!ef)
     ef = eet_open(file, EET_FILE_MODE_WRITE);

   if (!ef)
     {
        ERR("cannot open for read+write: %s", file);
        exit(-1);
     }

   f = fopen(in, "rb");
   if (!f)
     {
        ERR("cannot open %s", in);
        exit(-1);
     }

   fseek(f, 0, SEEK_END);
   textlen = ftell(f);
   if (textlen < 0)
     {
        ERR("cannot obtain current file position %s", in);
        fclose(f);
        exit(-1);
     }

   rewind(f);
   text = malloc(textlen);
   if (!text)
     {
        ERR("cannot allocate %i bytes", size);
        exit(-1);
     }

   if (fread(text, textlen, 1, f) != 1)
     {
        ERR("cannot read file %s", in);
        exit(-1);
     }

   fclose(f);
   if (!eet_data_undump_cipher(ef, key, crypto_key, text, textlen, compress))
     {
        ERR("cannot parse %s", in);
        exit(-1);
     }

   free(text);
   eet_close(ef);
} /* do_eet_encode */

/**
 * @brief Removes an entry from an Eet file.
 *
 * This function opens an Eet file in read-write mode and deletes the
 * entry associated with the specified key.
 *
 * @param file The path to the Eet file.
 * @param key The key of the entry to remove.
 */
static void
do_eet_remove(const char *file,
              const char *key)
{
   Eet_File *ef;

   ef = eet_open(file, EET_FILE_MODE_READ_WRITE);
   if (!ef)
     {
        ERR("cannot open for read+write: %s", file);
        exit(-1);
     }

   eet_delete(ef, key);
   eet_close(ef);
} /* do_eet_remove */

/**
 * @brief Checks and reports the signature information of an Eet file.
 *
 * This function opens an Eet file and attempts to retrieve and display
 * information about its X.509 certificate and signature. It prints the
 * certificate length, the certificate details, and the signature length.
 * If no certificate is found, it reports that.
 *
 * @param file The path to the Eet file to check.
 */
static void
do_eet_check(const char *file)
{
   Eet_File *ef;
   const void *der;
   int der_length;
   int sign_length;

   ef = eet_open(file, EET_FILE_MODE_READ);
   if (!ef)
     {
        ERR("checking signature of `%s` failed", file);
        exit(-1);
     }

   der = eet_identity_x509(ef, &der_length);

   if (!der)
     {
        fprintf(stdout, "No Certificate.\n");
     }
   else
     {
        fprintf(stdout, "Certificate length %i.\n", der_length);
        eet_identity_certificate_print(der, der_length, stdout);

        eet_identity_signature(ef, &sign_length);
        fprintf(stdout, "Signature length %i.\n", sign_length);
     }

   eet_close(ef);
} /* do_eet_check */

/**
 * @brief Signs an Eet file with a private key and attaches a public key certificate.
 *
 * This function opens an Eet file in read-write mode, loads a private/public
 * key pair, and then signs the Eet file using this identity. The public key
 * part of the identity is stored as the certificate in the Eet file.
 *
 * @param file The path to the Eet file to sign.
 * @param private_key The path to the file containing the private key.
 * @param public_key The path to the file containing the public key (certificate).
 */
static void
do_eet_sign(const char *file,
            const char *private_key,
            const char *public_key)
{
   Eet_File *ef;
   Eet_Key *key;

   ef = eet_open(file, EET_FILE_MODE_READ_WRITE);
   if (!ef)
     {
        ERR("cannot open for read+write: %s.", file);
        exit(-1);
     }

   key = eet_identity_open(public_key, private_key, NULL);
   if (!key)
     {
        ERR("cannot open key '%s:%s'.", public_key, private_key);
        exit(-1);
     }

   fprintf(stdout, "Using the following key to sign `%s`.\n", file);
   eet_identity_print(key, stdout);

   eet_identity_set(ef, key);

   eet_close(ef);
} /* do_eet_sign */

/**
 * @brief Main entry point for the Eet command-line utility.
 *
 * Parses command-line arguments to perform various operations on Eet files,
 * such as listing entries, extracting data, inserting data, encoding/decoding
 * data, removing entries, checking signatures, signing files, and displaying
 * file statistics.
 *
 * @param argc The number of command-line arguments.
 * @param argv An array of command-line argument strings.
 *             Example: `argv[0]` is program name, `argv[1]` is first argument.
 *             Structure of argv for listing:
 *             `argv = {"eet", "-l", "FILE.EET", NULL}` or
 *             `argv = {"eet", "-l", "-v", "FILE.EET", NULL}`
 *             Structure of argv for extracting:
 *             `argv = {"eet", "-x", "FILE.EET", "KEY", "[OUT-FILE]", "[CRYPTO_KEY]", NULL}`
 * @return Returns 0 on success, -1 on failure or for help/version display.
 */
int
main(int    argc,
     char **argv)
{
   if (argc < 2)
     {
help:
        printf(
          "Usage:\n"
          "  eet -l [-v] FILE.EET                               list all keys in FILE.EET\n"
          "  eet -x FILE.EET KEY [OUT-FILE] [CRYPTO_KEY]        extract data stored in KEY in FILE.EET and write to OUT-FILE or standard output\n"
          "  eet -d FILE.EET KEY [OUT-FILE] [CRYPTO_KEY]        extract and decode data stored in KEY in FILE.EET and write to OUT-FILE or standard output\n"
          "  eet -i FILE.EET KEY IN-FILE COMPRESS [CRYPTO_KEY]  insert data to KEY in FILE.EET from IN-FILE and if COMPRESS is 1, compress it\n"
          "  eet -e FILE.EET KEY IN-FILE COMPRESS [CRYPTO_KEY]  insert and encode to KEY in FILE.EET from IN-FILE and if COMPRESS is 1, compress it\n"
          "  eet -r FILE.EET KEY                                remove KEY in FILE.EET\n"
          "  eet -c FILE.EET                                    report and check the signature information of an eet file\n"
          "  eet -s FILE.EET PRIVATE_KEY PUBLIC_KEY             sign FILE.EET with PRIVATE_KEY and attach PUBLIC_KEY as it's certificate\n"
          "  eet -t FILE.EET                                    give some statistic about a file\n"
          "  eet -h                                             print out this help message\n"
          "  eet -V [--version]                                 show program version\n"
          );

        return -1;
     }

   if (!eet_init())
     return -1;

   _eet_main_log_dom = eina_log_domain_register("eet_main", EINA_COLOR_CYAN);
   if(_eet_main_log_dom < -1)
     {
        EINA_LOG_ERR("Impossible to create a log domain for eet_main.");
        eet_shutdown();
        return -1;
     }

   if ((!strncmp(argv[1], "-h", 2)))
     goto help;
   else if ((!strncmp(argv[1], "-V", 2)) || (!strncmp(argv[1], "--version",9 )))
     printf("Version: %s\n", PACKAGE_VERSION);
   else if (((!strcmp(argv[1], "-l")) || (!strcmp(argv[1], "-v"))) && (argc > 2))
     {
        if (argc == 3)
          do_eet_list(argv[2], EINA_FALSE);
        else if ((!strcmp(argv[2], "-l")) || (!strcmp(argv[2], "-v")))
          do_eet_list(argv[3], EINA_TRUE);
        else
          goto help;
     }
   else if ((!strcmp(argv[1], "-x")) && (argc > 3))
     {
        switch (argc)
          {
           case 4:
             {
               do_eet_extract(argv[2], argv[3], NULL, NULL);
               break;
             }
           case 5:
             {
               do_eet_extract(argv[2], argv[3], argv[4], NULL);
               break;
             }
           default:
             {
               do_eet_extract(argv[2], argv[3], argv[4], argv[5]);
               break;
             }
          }
     }
   else if ((!strcmp(argv[1], "-d")) && (argc > 3))
     {
        switch (argc)
          {
           case 4:
             {
               do_eet_decode(argv[2], argv[3], NULL, NULL);
               break;
             }
           case 5:
             {
               do_eet_decode(argv[2], argv[3], argv[4], NULL);
               break;
             }
           default:
             {
               do_eet_decode(argv[2], argv[3], argv[4], argv[5]);
               break;
             }
          }
     }
   else if ((!strcmp(argv[1], "-i")) && (argc > 5))
     {
        if (argc > 6)
          do_eet_insert(argv[2], argv[3], argv[4], atoi(argv[5]), argv[6]);
        else
          do_eet_insert(argv[2], argv[3], argv[4], atoi(argv[5]), NULL);
     }
   else if ((!strcmp(argv[1], "-e")) && (argc > 5))
     {
        if (argc > 6)
          do_eet_encode(argv[2], argv[3], argv[4], atoi(argv[5]), argv[6]);
        else
          do_eet_encode(argv[2], argv[3], argv[4], atoi(argv[5]), NULL);
     }
   else if ((!strcmp(argv[1], "-r")) && (argc > 3))
     do_eet_remove(argv[2], argv[3]);
   else if ((!strcmp(argv[1], "-c")) && (argc > 2))
     do_eet_check(argv[2]);
   else if ((!strcmp(argv[1], "-s")) && (argc > 4))
     do_eet_sign(argv[2], argv[3], argv[4]);
   else if ((!strcmp(argv[1], "-t")) && (argc > 2))
     do_eet_stats(argv[2]);
   else
     goto help;

   eina_log_domain_unregister(_eet_main_log_dom);
   eet_shutdown();
   return 0;
} /* main */

