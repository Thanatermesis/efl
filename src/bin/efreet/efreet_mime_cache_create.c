#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#ifdef HAVE_SYS_RESOURCE_H
#include <sys/time.h>
#include <sys/resource.h>
#endif

#include <libgen.h>
#include <ctype.h>

#ifdef _WIN32
# include <evil_private.h> /* fcntl */
#endif

#include <Eina.h>
#include <Eet.h>
#include <Ecore.h>
#include <Ecore_File.h>

#ifndef O_BINARY
# define O_BINARY 0
#endif

#define EFREET_MODULE_LOG_DOM _efreet_mime_cache_log_dom
static int _efreet_mime_cache_log_dom = -1;
static Eina_List *extra_dirs = NULL;

static Eina_Hash  *mimes = NULL;
static Eina_Hash  *extn_mimes = NULL;
static Eina_Hash  *glob_mimes = NULL;
static Eina_List  *mimes_sorted = NULL;
static Eina_List  *extn_mimes_sorted = NULL;
static Eina_List  *glob_mimes_sorted = NULL;

#include "Efreet.h"
#include "efreet_private.h"
#include "efreet_cache_private.h"

/**
 * @brief Creates and locks a lock file to ensure only one instance runs.
 *
 * This function creates a lock file in the efreet cache directory.
 * If the lock file cannot be created or locked, it indicates another
 * instance might be running or there's a permission issue.
 *
 * @return The file descriptor of the lock file on success, -1 on failure.
 */
static int
cache_lock_file(void)
{
   char file[PATH_MAX];
   struct flock fl;
   int lockfd;

   snprintf(file, sizeof(file), "%s/efreet/mime_data.lock", efreet_cache_home_get());
   lockfd = open(file, O_CREAT | O_BINARY | O_RDWR, S_IRUSR | S_IWUSR);
   if (lockfd < 0) return -1;
   efreet_fsetowner(lockfd);

   memset(&fl, 0, sizeof(struct flock));
   fl.l_type = F_WRLCK;
   fl.l_whence = SEEK_SET;
   if (fcntl(lockfd, F_SETLK, &fl) < 0)
     {
        INF("LOCKED! You may want to delete %s if this persists", file);
        close(lockfd);
        return -1;
     }

   return lockfd;
}

/**
 * @brief Comparison function for sorting strings.
 *
 * Used by eina_list_sorted_insert to sort keys (strings) alphabetically.
 *
 * @param key1 The first string.
 * @param key2 The second string.
 * @return An integer less than, equal to, or greater than zero if key1 is
 *         found, respectively, to be less than, to match, or be greater
 *         than key2.
 */
static int
hash_list_sort_insert_cmp(const char *key1, const char *key2)
{
   return strcmp(key1, key2);
}

/**
 * @brief Eina_Hash_Foreach callback to insert hash keys into a sorted list.
 *
 * This function is called for each key-value pair in a hash table.
 * It inserts the key into the provided Eina_List in sorted order.
 *
 * @param hash The hash table being iterated (unused).
 * @param key The current key from the hash table.
 * @param value The current value associated with the key (unused).
 * @param data A pointer to an Eina_List pointer where keys will be inserted.
 * @return EINA_TRUE to continue iteration, EINA_FALSE to stop.
 */
static Eina_Bool
hash_list_sort_each(const Eina_Hash *hash EINA_UNUSED, const void *key, void *value EINA_UNUSED, void *data)
{
   Eina_List **list = data;
   *list = eina_list_sorted_insert(*list,
                                   EINA_COMPARE_CB(hash_list_sort_insert_cmp),
                                   key);
   return EINA_TRUE;
}

/**
 * @brief Sorts the keys of a hash table into a list.
 *
 * Iterates over the hash table and inserts each key into the provided list,
 * maintaining sorted order.
 *
 * @param hash The hash table whose keys are to be sorted.
 * @param list A pointer to an Eina_List pointer that will store the sorted keys.
 *             The list should be initialized (e.g., to NULL) before calling.
 */
static void
hash_list_sort(Eina_Hash *hash, Eina_List **list)
{
   eina_hash_foreach(hash, hash_list_sort_each, list);
}

/**
 * @brief Loads mime types and their associated extensions from a file.
 *
 * Parses a file (typically /etc/mime.types) where each line defines a
 * mime type followed by space-separated extensions.
 * Lines starting with '#' are treated as comments and ignored.
 * Example line: "text/plain txt log"
 *
 * @param file The path to the mime.types file.
 */
static void
etc_mime_types_load(const char *file)
{
   int len;
   char buf[4096], buf2[4096], buf3[4096], *p, *p2;
   const char *mime;
   FILE *f = fopen(file, "r");

   if (!f) return;
   while (fgets(buf, sizeof(buf), f))
     {
        // remove newline at end of line string if there
        buf[sizeof(buf) - 1] = 0;
        len = strlen(buf);
        if ((len > 0) && (buf[len - 1] == '\n')) buf[len - 1] = 0;
        // buf: "# comment"
        // or
        // buf: "mime/type"
        // buf: "mime/type   "
        // buf: "mime/type     ext1"
        // buf: "mime/type     ext1 ext2"
        // buf: "mime/type     ext1 ext2 ext3"
        // ...
        p = buf;
        // find first token in line
        while ((*p) && isspace(*p)) p++;
        // if comment - skip line
        if (*p == '#') continue;
        p2 = p;
        while ((*p2) && !isspace(*p2)) p2++;
        // token is between p and p2 (not including p2)
        strncpy(buf2, p, p2 - p);
        buf2[p2 - p] = 0;
        mime = eina_stringshare_add(buf2);
        // buf2 is now the mime type token
        eina_hash_del(mimes, buf2, NULL);
        eina_hash_add(mimes, buf2, mime);
        // now lookf for all extension tokens;
        p = p2;
        // find next token in line
        while ((*p) && isspace(*p)) p++;
        while (*p)
          {
             // find end of token
             p2 = p;
             while ((*p2) && !isspace(*p2)) p2++;
             // buf3 is now the extension token
             strncpy(buf3, p, p2 - p);
             buf3[p2 - p] = 0;
             eina_hash_del(extn_mimes, buf3, NULL);
             eina_hash_add(extn_mimes, buf3, mime);
             // go to next token if any
             p = p2;
             while ((*p) && isspace(*p)) p++;
          }
     }
   fclose(f);
}

/**
 * @brief Loads mime types and their associated globs from a file.
 *
 * Parses a file (typically /usr/share/mime/globs or similar) where each
 * line defines a mime type followed by a colon and a glob pattern.
 * Lines starting with '#' are treated as comments and ignored.
 * Example line: "application/x-perl:*.pl"
 * If a glob is of the simple form "*.ext", it's treated as a direct
 * extension for faster lookup.
 *
 * @param file The path to the globs file.
 */
static void
share_mime_globs_load(const char *file)
{
   int len;
   char buf[4096], buf2[4096], buf3[4096], *p, *p2;
   const char *mime;
   FILE *f = fopen(file, "r");

   if (!f) return;
   while (fgets(buf, sizeof(buf), f))
     {
        // remove newline at end of line string if there
        buf[sizeof(buf) - 1] = 0;
        len = strlen(buf);
        if ((len > 0) && (buf[len - 1] == '\n')) buf[len - 1] = 0;
        // buf: "# comment"
        // or
        // buf: "mime/type:glob"
        // ...
        p = buf;
        // find first token in line
        while ((*p) && isspace(*p)) p++;
        // if comment - skip line
        if (*p == '#') continue;
        p2 = p;
        while ((*p2) && (*p2 != ':')) p2++;
        // token is between p and p2 (not including p2)
        strncpy(buf2, p, p2 - p);
        buf2[p2 - p] = 0;
        mime = eina_stringshare_add(buf2);
        // buf2 is now the mime type token
        eina_hash_del(mimes, buf2, NULL);
        eina_hash_add(mimes, buf2, mime);
        // now lookf for all extension tokens;
        p = p2;
        // find next token in line
        while ((*p) && (*p == ':')) p++;
        // find end of token
        p2 = p;
        while ((*p2) && !isspace(*p2)) p2++;
        // buf3 is now the extension token
        strncpy(buf3, p, p2 - p);
        buf3[p2 - p] = 0;
        // for a shortcut a glob of "*.xxx" is the same as just an ext of "xxx"
        // so if this is the case then put into the extn mimes not
        // the globl mimes for speed of lookup later on
        if ((buf3[0] == '*') && (buf3[1] == '.') &&
            (!strchr(buf3 + 2, '*')) && (!strchr(buf3 + 2, '?')) &&
            (!strchr(buf3 + 2, '[')) && (!strchr(buf3 + 2, ']')) &&
            (!strchr(buf3 + 2, '\\')))
          {
             eina_hash_del(extn_mimes, buf3 + 2, NULL);
             eina_hash_add(extn_mimes, buf3 + 2, mime);
          }
        else
          {
             eina_hash_del(glob_mimes, buf3, NULL);
             eina_hash_add(glob_mimes, buf3, mime);
          }
     }
   fclose(f);
}

/**
 * @brief Finds the offset of a string within a list of strings.
 *
 * This function searches for a given string `str` in `strlist`. If found,
 * it returns the corresponding data element from `offlist` at the same index.
 * This is used to find the pre-calculated byte offset of a string in the
 * cache file's string table.
 *
 * @param str The string to search for.
 * @param strlist An Eina_List of strings to search within.
 * @param offlist An Eina_List of offsets, corresponding to `strlist`.
 * @return The offset (as a void pointer, to be cast to size_t) if found,
 *         otherwise (void *)-1.
 */
static void *
find_off(const char *str, Eina_List *strlist, Eina_List *offlist)
{
   Eina_List *l, *ll;
   const char *s;

   ll = offlist;
   EINA_LIST_FOREACH(strlist, l, s)
     {
        if (!strcmp(str, s)) return ll->data;
        ll = ll->next;
     }
   return (void *)-1;
}

/**
 * @brief Stores the compiled mime information into a binary cache file.
 *
 * The cache file has a specific binary format:
 * 1. Magic Header: "EfrEeT-MiMeS-001" (16 bytes)
 * 2. Mimes Array:
 *    - [int] num_mimes: Number of entries in the mimes array.
 *    - For each mime:
 *      - [int] offset_to_mime_string: Byte offset from start of file to the mime string.
 * 3. Extension Mimes Array:
 *    - [int] num_extn_mimes: Number of entries in the extension mimes array.
 *    - For each extension mime:
 *      - [int] offset_to_extn_string: Byte offset from start of file to the extension string.
 *      - [int] offset_to_mime_string: Byte offset from start of file to the corresponding mime string.
 * 4. Glob Mimes Array:
 *    - [int] num_glob_mimes: Number of entries in the glob mimes array.
 *    - For each glob mime:
 *      - [int] offset_to_glob_string: Byte offset from start of file to the glob string.
 *      - [int] offset_to_mime_string: Byte offset from start of file to the corresponding mime string.
 * 5. String Table:
 *    - A contiguous block of null-terminated strings. All offsets above point into this table.
 *      The strings are grouped: all mime strings, then all extension strings, then all glob strings.
 *
 * All integer values are written in the native endianness of the machine creating the cache.
 * The filename itself will contain ".le." or ".be." to indicate endianness.
 *
 * @param out The path to the output cache file.
 */
static void
store_cache(const char *out)
{
   char buf[PATH_MAX + 128];
   FILE *f;
   size_t mimes_str_len = 0;
   size_t extn_mimes_str_len = 0;
   size_t glob_mimes_str_len = 0;
   size_t str_start;
   Eina_List *mimes_str_offsets = NULL;
   Eina_List *extn_mimes_str_offsets = NULL;
   Eina_List *glob_mimes_str_offsets = NULL;
   Eina_List *l, *ll;
   const char *s;
   void *ptr;
   unsigned int val;

   snprintf(buf, sizeof(buf), "%s.tmp", out);
   f = fopen(buf, "wb");
   if (!f) return;
   // write file magic - first 16 bytes
   if (fwrite("EfrEeT-MiMeS-001", 16, 1, f) != 1)
     goto write_error;
   // note: all offsets are in bytes from start of file
   // 
   // "EfrEeT-MiMeS-001" <- magic 16 byte header
   // [int] <- size of mimes array in number of entries
   // [int] <- str byte offset of 1st mime string (sorted)
   // [int] <- str byte offset of 2nd mime string
   // ...
   // [int] <- size of extn_mimes array in number of entries
   // [int] <- str byte offset of 1st extn string (sorted)
   // [int] <- str byte offset of 1st mime string
   // [int] <- str byte offset of 2nd extn string
   // [int] <- str byte offset of 2nd mine string
   // ...
   // [int] <- size of globs array in number of entries
   // [int] <- str byte offset of 1st glob string (sorted)
   // [int] <- str byte offset of 1st mime string
   // [int] <- str byte offset of 2nd glob string
   // [int] <- str byte offset of 2nd mime string
   // ...
   // strine1\0string2\0string3\0string4\0....
   EINA_LIST_FOREACH(mimes_sorted, l, s)
     {
        mimes_str_offsets = eina_list_append(mimes_str_offsets,
                                             (void *)mimes_str_len);
        mimes_str_len += strlen(s) + 1;
     }
   EINA_LIST_FOREACH(extn_mimes_sorted, l, s)
     {
        extn_mimes_str_offsets = eina_list_append(extn_mimes_str_offsets,
                                                  (void *)extn_mimes_str_len);
        extn_mimes_str_len += strlen(s) + 1;
     }
   EINA_LIST_FOREACH(glob_mimes_sorted, l, s)
     {
        glob_mimes_str_offsets = eina_list_append(glob_mimes_str_offsets,
                                                  (void *)glob_mimes_str_len);
        glob_mimes_str_len += strlen(s) + 1;
     }

   str_start = 16 + // magic header
   sizeof(int) +
   (eina_list_count(mimes_sorted) * sizeof(int)) +
   sizeof(int) +
   (eina_list_count(extn_mimes_sorted) * sizeof(int) * 2) +
   sizeof(int) +
   (eina_list_count(glob_mimes_sorted) * sizeof(int) * 2);

   val = eina_list_count(mimes_sorted);
   if (fwrite(&val, sizeof(val), 1, f) != 1)
     goto write_error;
   EINA_LIST_FOREACH(mimes_str_offsets, l, ptr)
     {
        val = (int)((size_t)ptr) + str_start;
        if (fwrite(&val, sizeof(val), 1, f) != 1)
          goto write_error;
     }

   val = eina_list_count(extn_mimes_sorted);
   if (fwrite(&val, sizeof(val), 1, f) != 1)
     goto write_error;
   ll = extn_mimes_sorted;
   EINA_LIST_FOREACH(extn_mimes_str_offsets, l, ptr)
     {
        val = (int)((size_t)ptr) + str_start + mimes_str_len;
        if (fwrite(&val, sizeof(val), 1, f) != 1)
          goto write_error;
        s = eina_hash_find(extn_mimes, ll->data);
        ptr = find_off(s, mimes_sorted, mimes_str_offsets);
        val = (int)((size_t)ptr) + str_start;
        if (fwrite(&val, sizeof(val), 1, f) != 1)
          goto write_error;
        ll = ll->next;
     }

   val = eina_list_count(glob_mimes_sorted);
   if (fwrite(&val, sizeof(val), 1, f) != 1)
     goto write_error;
   ll = glob_mimes_sorted;
   EINA_LIST_FOREACH(glob_mimes_str_offsets, l, ptr)
     {
        val = (int)((size_t)ptr) + str_start + mimes_str_len + extn_mimes_str_len;
        if (fwrite(&val, sizeof(val), 1, f) != 1)
          goto write_error;
        s = eina_hash_find(glob_mimes, ll->data);
        ptr = find_off(s, mimes_sorted, mimes_str_offsets);
        val = (int)((size_t)ptr) + str_start;
        if (fwrite(&val, sizeof(val), 1, f) != 1)
          goto write_error;
        ll = ll->next;
     }
   EINA_LIST_FOREACH(mimes_sorted, l, s)
     {
        if (fwrite(s, strlen(s) + 1, 1, f) != 1)
          goto write_error;
     }
   EINA_LIST_FOREACH(extn_mimes_sorted, l, s)
     {
        if (fwrite(s, strlen(s) + 1, 1, f) != 1)
          goto write_error;
     }
   EINA_LIST_FOREACH(glob_mimes_sorted, l, s)
     {
        if (fwrite(s, strlen(s) + 1, 1, f) != 1)
          goto write_error;
     }
   if (fclose(f) != 0)
     {
        ERR("Cannot close file %s", buf);
        f = NULL;
        goto error;
     }
   if (rename(buf, out) != 0)
     {
        ERR("Cannot rename %s to %s", buf, out);
        f = NULL;
        goto error;
     }
   return;
write_error:
   ERR("Cannot write to %s", buf);
error:
   if (f) fclose(f);
   if (unlink(buf) != 0) WRN("Cannot delete tmp file %s", buf);
}

/**
 * @brief Main function for the efreet_mime_cache_create utility.
 *
 * This program generates a binary cache file for MIME type information.
 * It parses system and user MIME databases (/etc/mime.types,
 * /usr/share/mime/globs, etc.) and compiles them into an efficient
 * lookup format.
 *
 * Command-line arguments:
 *   -v              : Verbose mode (enables debug logging).
 *   -h, --help      : Displays help message and exits.
 *   -d dir1 dir2 ...: Specifies extra directories to scan for mime/globs files.
 *                     These directories are searched after standard system locations
 *                     but before the user's data home directory.
 *
 * The program uses a lock file to prevent multiple instances from running
 * simultaneously and potentially corrupting the cache.
 *
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return 0 on success, -1 on failure.
 */
int
main(int argc, char **argv)
{
   char buf[PATH_MAX];
   const char *s;
   int i;
   int ret = -1, lockfd = -1;
   Eina_List *datadirs, *l;

   if (!eina_init()) goto eina_error;
   if (!eet_init()) goto eet_error;
   if (!ecore_init()) goto ecore_error;
   if (!ecore_file_init()) goto ecore_file_error;
   if (!efreet_init()) goto efreet_error;

   _efreet_mime_cache_log_dom =
     eina_log_domain_register("efreet_mime_cache", EFREET_DEFAULT_LOG_COLOR);
   if (_efreet_mime_cache_log_dom < 0)
     {
        EINA_LOG_ERR("Efreet: Could not create a log domain for efreet_mime_cache.");
        return -1;
     }

   for (i = 1; i < argc; i++)
     {
        if (!strcmp(argv[i], "-v"))
          eina_log_domain_level_set("efreet_mime_cache", EINA_LOG_LEVEL_DBG);
        else if ((!strcmp(argv[i], "-h")) ||
                 (!strcmp(argv[i], "-help")) ||
                 (!strcmp(argv[i], "--h")) ||
                 (!strcmp(argv[i], "--help")))
          {
             printf("Options:\n");
             printf("  -v              Verbose mode\n");
             printf("  -d dir1 dir2    Extra dirs\n");
             exit(0);
          }
        else if (!strcmp(argv[i], "-d"))
          {
             while ((i < (argc - 1)) && (argv[(i + 1)][0] != '-'))
               extra_dirs = eina_list_append(extra_dirs, argv[++i]);
          }
     }
   extra_dirs = eina_list_sort(extra_dirs, -1, EINA_COMPARE_CB(strcmp));

   /* create homedir */
   snprintf(buf, sizeof(buf), "%s/efreet", efreet_cache_home_get());
   if (!ecore_file_exists(buf))
     {
        if (!ecore_file_mkpath(buf)) goto error;
        efreet_setowner(buf);
     }

   /* lock process, so that we only run one copy of this program */
   lockfd = cache_lock_file();
   if (lockfd == -1) goto error;

   mimes = eina_hash_string_superfast_new(NULL);
   extn_mimes = eina_hash_string_superfast_new(NULL);
   glob_mimes = eina_hash_string_superfast_new(NULL);

   etc_mime_types_load("/etc/mime.types");
   share_mime_globs_load("/usr/share/mime/globs");
   datadirs = efreet_data_dirs_get();
   EINA_LIST_FOREACH(datadirs, l, s)
     {
        snprintf(buf, sizeof(buf), "%s/mime/globs", s);
        share_mime_globs_load(buf);
     }
   EINA_LIST_FOREACH(extra_dirs, l, s)
     {
        snprintf(buf, sizeof(buf), "%s/mime/globs", s);
        share_mime_globs_load(buf);
     }
   snprintf(buf, sizeof(buf), "%s/mime/globs", efreet_data_home_get());
   share_mime_globs_load(buf);
   // XXX: load user files and other dirs etc.
   // XXX: load globs

   hash_list_sort(mimes, &mimes_sorted);
   hash_list_sort(extn_mimes, &extn_mimes_sorted);
   hash_list_sort(glob_mimes, &glob_mimes_sorted);

#ifdef WORDS_BIGENDIAN
   snprintf(buf, sizeof(buf), "%s/efreet/mime_cache_%s.be.dat",
            efreet_cache_home_get(), efreet_hostname_get());
#else
   snprintf(buf, sizeof(buf), "%s/efreet/mime_cache_%s.le.dat",
            efreet_cache_home_get(), efreet_hostname_get());
#endif
   store_cache(buf);

   ret = 0;
   close(lockfd);
error:
   efreet_shutdown();
efreet_error:
   ecore_file_shutdown();
ecore_file_error:
   ecore_shutdown();
ecore_error:
   eet_shutdown();
eet_error:
   eina_list_free(extra_dirs);
   eina_log_domain_unregister(_efreet_mime_cache_log_dom);
   eina_shutdown();
eina_error:
   return ret;
}
