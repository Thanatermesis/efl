#include "edje_private.h"

/**
 * @internal
 * @brief Hash table for Edje_File structures, keyed by Eina_File.
 */
Eina_Hash *_edje_file_hash = NULL;
/**
 * @internal
 * @brief Hash table for Edje_File structures, keyed by their string ID.
 */
Eina_Hash *_edje_id_hash = NULL;

/**
 * @internal
 * @brief Hash table for pending required Edje_File structures.
 * Keyed by the required file's string ID, value is a list of Edje_File
 * that require it.
 */
static Eina_Hash *_edje_requires_pending;
/**
 * @internal
 * @brief Maximum number of Edje_File structures to keep in the cache.
 */
static int _edje_file_cache_size = 16;
/**
 * @internal
 * @brief List of recently unused Edje_File structures for caching.
 */
static Eina_List *_edje_file_cache = NULL;

/**
 * @internal
 * @brief Maximum number of Edje_Part_Collection structures to keep in the cache per Edje_File.
 */
static int _edje_collection_cache_size = 16;

/**
 * @internal
 * @brief Allocates Eina Mempools for a given Edje Part Collection Directory Entry.
 *
 * This function initializes memory pools for various Edje part description types
 * (RECTANGLE, TEXT, IMAGE, etc.) and for Edje_Part itself. These mempools
 * are used for efficient allocation and deallocation of part descriptions
 * and parts within the collection. It initializes both normal and RTL (Right-To-Left)
 * mempools for description types that may require direction-specific handling.
 *
 * @param ce Pointer to the Edje_Part_Collection_Directory_Entry for which
 *           to allocate mempools. This structure must have its `count` field
 *           populated with the number of elements for each type.
 */
EAPI void
edje_cache_emp_alloc(Edje_Part_Collection_Directory_Entry *ce)
{
   /* Init Eina Mempools this is also used in edje_pick.c */
   char *buffer;
   ce->mp = calloc(1, sizeof(Edje_Part_Collection_Directory_Entry_Mp));
   if (!ce->mp) return;

#define INIT_EMP(Tp, Sz, Ce) \
  buffer = alloca(strlen(ce->entry) + strlen(#Tp) + 2); \
  sprintf(buffer, "%s/%s", ce->entry, #Tp); \
  Ce->mp->mp.Tp = eina_mempool_add("one_big", buffer, NULL, sizeof (Sz), Ce->count.Tp); \
  _emp_##Tp = Ce->mp->mp.Tp;

#define INIT_EMP_BOTH(Tp, Sz, Ce) \
  INIT_EMP(Tp, Sz, Ce) \
  Ce->mp->mp_rtl.Tp = eina_mempool_add("one_big", buffer, NULL, \
                                       sizeof(Sz), Ce->count.Tp);

  INIT_EMP_BOTH(RECTANGLE, Edje_Part_Description_Common, ce);
  INIT_EMP_BOTH(TEXT, Edje_Part_Description_Text, ce);
  INIT_EMP_BOTH(IMAGE, Edje_Part_Description_Image, ce);
  INIT_EMP_BOTH(PROXY, Edje_Part_Description_Proxy, ce);
  INIT_EMP_BOTH(SWALLOW, Edje_Part_Description_Common, ce);
  INIT_EMP_BOTH(TEXTBLOCK, Edje_Part_Description_Text, ce);
  INIT_EMP_BOTH(GROUP, Edje_Part_Description_Common, ce);
  INIT_EMP_BOTH(BOX, Edje_Part_Description_Box, ce);
  INIT_EMP_BOTH(TABLE, Edje_Part_Description_Table, ce);
  INIT_EMP_BOTH(EXTERNAL, Edje_Part_Description_External, ce);
  INIT_EMP_BOTH(SPACER, Edje_Part_Description_Common, ce);
  INIT_EMP_BOTH(SNAPSHOT, Edje_Part_Description_Snapshot, ce);
  INIT_EMP_BOTH(VECTOR, Edje_Part_Description_Vector, ce);
  INIT_EMP(part, Edje_Part, ce);
}

/**
 * @internal
 * @brief Frees Eina Mempools associated with an Edje Part Collection Directory Entry.
 *
 * This function deallocates all memory pools that were previously allocated by
 * edje_cache_emp_alloc() for the given collection entry. It also clears
 * the mempool structures and frees the `ce->mp` itself.
 *
 * @param ce Pointer to the Edje_Part_Collection_Directory_Entry whose
 *           mempools are to be freed.
 */
EAPI void
edje_cache_emp_free(Edje_Part_Collection_Directory_Entry *ce)
{  /* Free Eina Mempools this is also used in edje_pick.c */
   /* Destroy all part and description. */
   ce->ref = NULL;

   if (!ce->mp) return;
   eina_mempool_del(ce->mp->mp.RECTANGLE);
   eina_mempool_del(ce->mp->mp.TEXT);
   eina_mempool_del(ce->mp->mp.IMAGE);
   eina_mempool_del(ce->mp->mp.PROXY);
   eina_mempool_del(ce->mp->mp.SWALLOW);
   eina_mempool_del(ce->mp->mp.TEXTBLOCK);
   eina_mempool_del(ce->mp->mp.GROUP);
   eina_mempool_del(ce->mp->mp.BOX);
   eina_mempool_del(ce->mp->mp.TABLE);
   eina_mempool_del(ce->mp->mp.EXTERNAL);
   eina_mempool_del(ce->mp->mp.SPACER);
   eina_mempool_del(ce->mp->mp.SNAPSHOT);
   eina_mempool_del(ce->mp->mp.VECTOR);
   eina_mempool_del(ce->mp->mp.part);
   memset(&ce->mp->mp, 0, sizeof(ce->mp->mp));

   eina_mempool_del(ce->mp->mp_rtl.RECTANGLE);
   eina_mempool_del(ce->mp->mp_rtl.TEXT);
   eina_mempool_del(ce->mp->mp_rtl.IMAGE);
   eina_mempool_del(ce->mp->mp_rtl.PROXY);
   eina_mempool_del(ce->mp->mp_rtl.SWALLOW);
   eina_mempool_del(ce->mp->mp_rtl.TEXTBLOCK);
   eina_mempool_del(ce->mp->mp_rtl.GROUP);
   eina_mempool_del(ce->mp->mp_rtl.BOX);
   eina_mempool_del(ce->mp->mp_rtl.TABLE);
   eina_mempool_del(ce->mp->mp_rtl.EXTERNAL);
   eina_mempool_del(ce->mp->mp_rtl.SPACER);
   eina_mempool_del(ce->mp->mp_rtl.SNAPSHOT);
   eina_mempool_del(ce->mp->mp_rtl.VECTOR);
   memset(&ce->mp->mp_rtl, 0, sizeof(ce->mp->mp_rtl));

   free(ce->mp);
}

/**
 * @internal
 * @brief Initializes signal and source patterns for programs in an Edje Part Collection.
 *
 * This function processes the programs defined in an Edje_Part_Collection
 * and prepares them for efficient matching. It categorizes programs based on
 * their comparison type (strcmp, strncmp, etc.) and builds specialized data
 * structures (hash tables, globing lists) to speed up the process of
 * finding matching programs when signals are emitted.
 *
 * If the environment variable `EDJE_DUMP_PROGRAMS` is set, it will print
 * information about the programs in the collection.
 *
 * @param edc Pointer to the Edje_Part_Collection whose programs are to be initialized.
 */
void
_edje_programs_patterns_init(Edje_Part_Collection *edc)
{
   Edje_Signals_Sources_Patterns *ssp = &edc->patterns.programs;
   Edje_Program **all;
   unsigned int i, j;

   if (ssp->signals_patterns)
     return;

   static signed char dump_programs = -1;

   if (dump_programs == -1)
     {
        if (getenv("EDJE_DUMP_PROGRAMS")) dump_programs = 1;
        else dump_programs = 0;
     }
   if (dump_programs == 1)
     {
        INF("Group '%s' programs:", edc->part);
#define EDJE_DUMP_PROGRAM(Section)                    \
  for (i = 0; i < edc->programs.Section##_count; i++) \
    INF(#Section " for ('%s', '%s')", edc->programs.Section[i]->signal, edc->programs.Section[i]->source);

        EDJE_DUMP_PROGRAM(strcmp);
        EDJE_DUMP_PROGRAM(strncmp);
        EDJE_DUMP_PROGRAM(strrncmp);
        EDJE_DUMP_PROGRAM(fnmatch);
        EDJE_DUMP_PROGRAM(nocmp);
     }

   edje_match_program_hash_build(edc->programs.strcmp,
                                 edc->programs.strcmp_count,
                                 &ssp->exact_match);

   j = edc->programs.strncmp_count
     + edc->programs.strrncmp_count
     + edc->programs.fnmatch_count
     + edc->programs.nocmp_count;
   if (j == 0) return;

   all = malloc(sizeof (Edje_Program *) * j);
   if (!all) return;
   j = 0;

   /* FIXME: Build specialized data type for each case */
#define EDJE_LOAD_PROGRAMS_ADD(Array, Edc, It, Git, All)                \
   for (It = 0; It < Edc->programs.Array##_count; ++It, ++Git)          \
     All[Git] = Edc->programs.Array[It];

   EDJE_LOAD_PROGRAMS_ADD(fnmatch, edc, i, j, all);
   EDJE_LOAD_PROGRAMS_ADD(strncmp, edc, i, j, all);
   EDJE_LOAD_PROGRAMS_ADD(strrncmp, edc, i, j, all);
   /* FIXME: Do a special pass for that one */
   EDJE_LOAD_PROGRAMS_ADD(nocmp, edc, i, j, all);

   ssp->u.programs.globing = all;
   ssp->u.programs.count = j;
   ssp->signals_patterns = edje_match_programs_signal_init(all, j);
   ssp->sources_patterns = edje_match_programs_source_init(all, j);
}

/**
 * @internal
 * @brief Performs consistency checks and fixes on an Edje_Part_Collection.
 *
 * This function iterates through the parts in a collection and validates
 * properties like `confine_id`, `event_id`, and `clip_to_id`. It checks for:
 * - IDs that are out of bounds (greater than or equal to `edc->parts_count`).
 * - Circular dependencies (loops) in `confine_to`, `events_to`, and `clip_to` chains.
 * - `events_to` pointing to a part that is not draggable.
 *
 * If any invalid configurations are found, the corresponding ID is set to -1
 * to disable the feature and prevent crashes or undefined behavior.
 * The function ensures it only processes each collection once by checking and
 * setting the `edc->checked` flag.
 *
 * @param edc Pointer to the Edje_Part_Collection to be checked and fixed.
 */
static inline void
_edje_part_collection_fix(Edje_Part_Collection *edc)
{
   if (edc->checked) return;

   edc->checked = 1;

   unsigned int j;
   Edje_Part *ep;
   Eina_Array hist;

   eina_array_step_set(&hist, sizeof(Eina_Array), 5);

   for (j = 0; j < edc->parts_count; ++j)
     {
        Edje_Part *ep2;
        ep = edc->parts[j];

        /* Register any color classes in this parts descriptions. */
        eina_array_push(&hist, ep);
        ep2 = ep;
        while (ep2->dragable.confine_id >= 0)
          {
             if (ep2->dragable.confine_id >= (int)edc->parts_count)
               {
                  ERR("confine_to above limit. invalidating it.");
                  ep2->dragable.confine_id = -1;
                  break;
               }

             ep2 = edc->parts[ep2->dragable.confine_id];
             if (eina_array_find(&hist, ep2, NULL))
               {
                  ERR("confine_to loops. invalidating loop.");
                  ep2->dragable.confine_id = -1;
                  break;
               }
             eina_array_push(&hist, ep2);
          }
        eina_array_clean(&hist);

        eina_array_push(&hist, ep);
        ep2 = ep;
        while (ep2->dragable.event_id >= 0)
          {
             Edje_Part *prev;

             if (ep2->dragable.event_id >= (int)edc->parts_count)
               {
                  ERR("event_id above limit. invalidating it.");
                  ep2->dragable.event_id = -1;
                  break;
               }
             prev = ep2;

             ep2 = edc->parts[ep2->dragable.event_id];
             /* events_to may be used only with dragable */
             if (!ep2->dragable.x && !ep2->dragable.y)
               {
                  prev->dragable.event_id = -1;
                  break;
               }

             if (eina_array_find(&hist, ep2, NULL))
               {
                  ERR("events_to loops. invalidating loop.");
                  ep2->dragable.event_id = -1;
                  break;
               }
             eina_array_push(&hist, ep2);
          }
        eina_array_clean(&hist);

        eina_array_push(&hist, ep);
        ep2 = ep;
        while (ep2->clip_to_id >= 0)
          {
             if (ep2->clip_to_id >= (int)edc->parts_count)
               {
                  ERR("clip_to_id above limit. invalidating it.");
                  ep2->clip_to_id = -1;
                  break;
               }

             ep2 = edc->parts[ep2->clip_to_id];
             if (eina_array_find(&hist, ep2, NULL))
               {
                  ERR("clip_to loops. invalidating loop.");
                  ep2->clip_to_id = -1;
                  break;
               }
             eina_array_push(&hist, ep2);
          }
        eina_array_clean(&hist);
     }
     eina_array_flush(&hist);
}

/**
 * @internal
 * @brief Opens and loads an Edje_Part_Collection from an Edje_File.
 *
 * This function retrieves a specific collection (group) from a loaded Edje file.
 * It first checks if the collection is already referenced or cached. If not,
 * it reads the collection data from the EET file, loads associated Embryo and Lua
 * scripts, initializes program patterns, and performs necessary fixes.
 * The reference count of the collection is incremented.
 *
 * @param edf Pointer to the loaded Edje_File.
 * @param coll The name of the collection (group) to open.
 * @return A pointer to the loaded Edje_Part_Collection, or NULL on failure.
 *         The returned collection has its reference count incremented.
 */
static Edje_Part_Collection *
_edje_file_coll_open(Edje_File *edf, const char *coll)
{
   Edje_Part_Collection *edc = NULL;
   Edje_Part_Collection_Directory_Entry *ce;
   int id = -1, size = 0;
   unsigned int n;
   Eina_List *l;
   char buf[256];
   void *data;

   ce = eina_hash_find(edf->collection, coll);
   if (!ce) return NULL;

   if (ce->ref)
     {
        ce->ref->references++;
        return ce->ref;
     }

   EINA_LIST_FOREACH(edf->collection_cache, l, edc)
     {
        if (!strcmp(edc->part, coll))
          {
             edc->references = 1;
             ce->ref = edc;

             edf->collection_cache = eina_list_remove_list(edf->collection_cache, l);
             return ce->ref;
          }
     }

   id = ce->id;
   if (id < 0) return NULL;

   edje_cache_emp_alloc(ce);
   snprintf(buf, sizeof(buf), "edje/collections/%i", id);
   edc = eet_data_read(edf->ef, _edje_edd_edje_part_collection, buf);
   if (!edc) return NULL;

   edc->references = 1;
   edc->part = ce->entry;

   /* For Edje file build with Edje 1.0 */
   if (edf->version <= 3 && edf->minor <= 1)
     {
        /* This will preserve previous rendering */
        unsigned int i;

        /* people expect signal to not be broadcasted */
        edc->broadcast_signal = EINA_FALSE;

        /* people expect text.align to be 0.0 0.0 */
        for (i = 0; i < edc->parts_count; ++i)
          {
             if (edc->parts[i]->type == EDJE_PART_TYPE_TEXTBLOCK)
               {
                  Edje_Part_Description_Text *text;
                  unsigned int j;

                  text = (Edje_Part_Description_Text *)edc->parts[i]->default_desc;
                  text->text.align.x = TO_DOUBLE(0.0);
                  text->text.align.y = TO_DOUBLE(0.0);

                  for (j = 0; j < edc->parts[i]->other.desc_count; ++j)
                    {
                       text = (Edje_Part_Description_Text *)edc->parts[i]->other.desc[j];
                       text->text.align.x = TO_DOUBLE(0.0);
                       text->text.align.y = TO_DOUBLE(0.0);
                    }
               }
          }
     }

   snprintf(buf, sizeof(buf), "edje/scripts/embryo/compiled/%i", id);
   data = eet_read(edf->ef, buf, &size);

   if (data)
     {
        edc->script = embryo_program_new(data, size);
        _edje_embryo_script_init(edc);
        free(data);
     }

   snprintf(buf, sizeof(buf), "edje/scripts/lua/%i", id);
   data = eet_read(edf->ef, buf, &size);

   if (data)
     {
        _edje_lua2_script_load(edc, data, size);
        free(data);
     }

   ce->ref = edc;

   _edje_programs_patterns_init(edc);

   n = edc->programs.fnmatch_count +
     edc->programs.strcmp_count +
     edc->programs.strncmp_count +
     edc->programs.strrncmp_count +
     edc->programs.nocmp_count;

   if (n > 0)
     {
        Edje_Program *pr;
        unsigned int i;

        edc->patterns.table_programs = malloc(sizeof(Edje_Program *) * n);
        if (edc->patterns.table_programs)
          {
             edc->patterns.table_programs_size = n;

#define EDJE_LOAD_BUILD_TABLE(Array, Edc, It, Tmp)                      \
             for (It = 0; It < Edc->programs.Array##_count; ++It)       \
               {                                                        \
                  Tmp = Edc->programs.Array[It];                        \
                  Edc->patterns.table_programs[Tmp->id] = Tmp;          \
                  if (!Edc->need_seat && Tmp->signal && !strncmp(Tmp->signal, "seat,", 5)) \
                    Edc->need_seat = EINA_TRUE;                         \
               }

             EDJE_LOAD_BUILD_TABLE(fnmatch, edc, i, pr);
             EDJE_LOAD_BUILD_TABLE(strcmp, edc, i, pr);
             EDJE_LOAD_BUILD_TABLE(strncmp, edc, i, pr);
             EDJE_LOAD_BUILD_TABLE(strrncmp, edc, i, pr);
             EDJE_LOAD_BUILD_TABLE(nocmp, edc, i, pr);
          }
     }

   /* Search for the use of allowed seat used by part if we still do not know if seat are needed. */
   if (!edc->need_seat)
     {
        unsigned int i;

        for (i = 0; i < edc->parts_count; i++)
          {
             Edje_Part *part = edc->parts[i];

             if (part->allowed_seats)
               {
                  edc->need_seat = EINA_TRUE;
                  break;
               }
          }
     }

   _edje_part_collection_fix(edc);

   return edc;
}

/**
 * @internal
 * @brief Extracts .mo (Machine Object) translation files from an Edje_File.
 *
 * This function iterates through the MO entries specified in the Edje file's
 * MO directory. For each entry, it reads the compressed MO data from the EET
 * archive and writes it to a cache location in the user's Efreet cache home
 * directory (typically `~/.cache/efreet/edje/`).
 *
 * The cached MO files are named using a combination of the Edje file's
 * modification time, size, a CRC of its filename, and the original MO source
 * filename (e.g., `~/.cache/efreet/edje/LOCALE/LC_MESSAGES/MTIME-SIZE-CRC-FILENAME.mo`).
 * This helps in managing cache validity. If an existing cached file is older
 * than the Edje file's modification time, it is removed and re-extracted.
 *
 * @param edf Pointer to the Edje_File from which to extract MO files.
 */
void
_edje_extract_mo_files(Edje_File *edf)
{
   Eina_Strbuf *mo_id_str;
   const void *data;
   const char *cache_path;
   const char *filename;
   unsigned int crc;
   time_t t;
   size_t sz;
   unsigned int i;
   int len;

   cache_path = efreet_cache_home_get();

   t = eina_file_mtime_get(edf->f);
   sz = eina_file_size_get(edf->f);
   filename = eina_file_filename_get(edf->f);
   crc = eina_crc(filename, strlen(filename), 0xffffffff, EINA_TRUE);

   snprintf(edf->fid, sizeof(edf->fid), "%lld-%lld-%x",
            (long long int)t,
            (long long int)sz,
            crc);

   mo_id_str = eina_strbuf_new();

   for (i = 0; i < edf->mo_dir->mo_entries_count; i++)
     {
        Edje_Mo *mo_entry;
        char out[PATH_MAX + PATH_MAX + 128];
        char outdir[PATH_MAX];
        char *sub_str;
        char *mo_src;

        mo_entry = &edf->mo_dir->mo_entries[i];

        eina_strbuf_append_printf(mo_id_str,
                                  "edje/mo/%i/%s/LC_MESSAGES",
                                  mo_entry->id,
                                  mo_entry->locale);
        data = eet_read_direct(edf->ef,
                               eina_strbuf_string_get(mo_id_str),
                               &len);

        if (data)
          {
             snprintf(outdir, sizeof(outdir),
                      "%s/edje/%s/LC_MESSAGES",
                      cache_path, mo_entry->locale);
             ecore_file_mkpath(outdir);
             mo_src = strdup(mo_entry->mo_src);
             sub_str = strstr(mo_src, ".po");

             if (sub_str)
               sub_str[1] = 'm';

             snprintf(out, sizeof(out), "%s/%s-%s",
                      outdir, edf->fid, mo_src);
             if (ecore_file_exists(out))
               {
                  if (edf->mtime > ecore_file_mod_time(out))
                    ecore_file_remove(out);
               }
             if (!ecore_file_exists(out))
               {
                  FILE *f;

                  f = fopen(out, "wb");
                  if (f)
                    {
                       if (fwrite(data, len, 1, f) != 1)
                         ERR("Could not write mo: %s: %s", out, strerror(errno));
                       fclose(f);
                    }
                  else
                    ERR("Could not open for writing mo: %s: %s", out, strerror(errno));
               }
             free(mo_src);
          }

        eina_strbuf_reset(mo_id_str);
     }

   eina_strbuf_free(mo_id_str);
}

// XXX: this is not pretty. some oooooold edje files do not store strings
// in their dictionary for hashes. this works around crashes loading such
// files
extern size_t  _edje_data_string_mapping_size;
extern void   *_edje_data_string_mapping;

/**
 * @internal
 * @brief Opens an Edje file and loads its main structure.
 *
 * This function opens an Edje file from the given Eina_File handle. It memory-maps
 * the file using EET, reads the main "edje/file" data structure, and performs
 * various initializations. These include:
 * - Checking file version and compatibility.
 * - Setting up string shares for paths and other strings.
 * - Parsing and fixing textblock styles.
 * - Initializing hash tables for styles, color classes, text classes, and size classes.
 * - Handling required external Edje files (dependencies).
 * - Loading external modules (plugins).
 * - Extracting embedded MO (translation) files.
 *
 * A workaround for very old Edje files that do not store strings correctly in
 * their dictionary for hashes is included.
 *
 * @param f Pointer to the Eina_File representing the Edje file to open.
 * @param error_ret Pointer to an integer where an Edje_Load_Error code will be
 *                  stored in case of failure.
 * @param mtime The modification time of the file, used for cache validation.
 * @param coll A boolean indicating if collection data is expected to be present.
 *             If EINA_TRUE and no collection directory is found, it's an error.
 * @return A pointer to the loaded Edje_File structure, or NULL on failure.
 *         If successful, the returned Edje_File has a reference count of 1.
 */
static Edje_File *
_edje_file_open(const Eina_File *f, int *error_ret, time_t mtime, Eina_Bool coll)
{
   Edje_Color_Tree_Node *ctn;
   Edje_Color_Class *cc;
   Edje_Text_Class *tc;
   Edje_Size_Class *sc;
   Edje_Style      *stl;
   Edje_File *edf;
   Eina_List *l, *ll;
   Eet_File *ef;
   char *name;
   void *mapping;
   size_t mapping_size;

   ef = eet_mmap(f);
   if (!ef)
     {
        *error_ret = EDJE_LOAD_ERROR_UNKNOWN_FORMAT;
        return NULL;
     }
   // XXX: ancient edje file workaround
   mapping = eina_file_map_all((Eina_File *)f, EINA_FILE_SEQUENTIAL);
   if (mapping)
     {
        mapping_size = eina_file_size_get(f);
        _edje_data_string_mapping = mapping;
        _edje_data_string_mapping_size = mapping_size;
     }
   edf = eet_data_read(ef, _edje_edd_edje_file, "edje/file");
   // XXX: ancient edje file workaround
   if (mapping)
     {
        eina_file_map_free((Eina_File *)f, mapping);
        _edje_data_string_mapping = NULL;
     }
   if (!edf)
     {
        *error_ret = EDJE_LOAD_ERROR_CORRUPT_FILE;
        eet_close(ef);
        return NULL;
     }

   edf->f = eina_file_dup(f);
   edf->ef = ef;
   edf->mtime = mtime;

   if (edf->version != EDJE_FILE_VERSION)
     {
        *error_ret = EDJE_LOAD_ERROR_INCOMPATIBLE_FILE;
        _edje_file_free(edf);
        return NULL;
     }
   if (!edf->collection && coll)
     {
        *error_ret = EDJE_LOAD_ERROR_CORRUPT_FILE;
        _edje_file_free(edf);
        return NULL;
     }

   if (edf->minor > EDJE_FILE_MINOR)
     {
        WRN("`%s` may use feature from a newer edje and could not show up as expected.",
            eina_file_filename_get(f));
     }
   if (edf->base_scale <= ZERO)
     {
        edf->base_scale = FROM_INT(1);
        WRN("The base_scale can not be a 0.0. It is changed the default value(1.0)");
     }

   /* Set default efl_version if there is no version information */
   if ((edf->efl_version.major == 0) && (edf->efl_version.minor == 0))
     {
        edf->efl_version.major = 1;
        edf->efl_version.minor = 18;
     }

   edf->path = eina_stringshare_add(eina_file_filename_get(f));
   edf->references = 1;

   /* This should be done at edje generation time */
   _edje_file_textblock_style_parse_and_fix(edf);

   edf->style_hash = eina_hash_string_small_new(NULL);
   EINA_LIST_FOREACH(edf->styles, l, stl)
     if (stl->name)
       eina_hash_direct_add(edf->style_hash, stl->name, stl);

   edf->color_tree_hash = eina_hash_string_small_new(NULL);
   EINA_LIST_FOREACH(edf->color_tree, l, ctn)
     EINA_LIST_FOREACH(ctn->color_classes, ll, name)
       eina_hash_add(edf->color_tree_hash, name, ctn);

   edf->color_hash = eina_hash_string_small_new(NULL);
   EINA_LIST_FOREACH(edf->color_classes, l, cc)
     if (cc->name)
       eina_hash_direct_add(edf->color_hash, cc->name, cc);

   edf->text_hash = eina_hash_string_small_new(NULL);
   EINA_LIST_FOREACH(edf->text_classes, l, tc)
     {
        if (tc->name)
          eina_hash_direct_add(edf->text_hash, tc->name, tc);

        if (tc->font)
          tc->font = eina_stringshare_add(tc->font);
     }

   edf->size_hash = eina_hash_string_small_new(NULL);
   EINA_LIST_FOREACH(edf->size_classes, l, sc)
     if (sc->name)
       eina_hash_direct_add(edf->size_hash, sc->name, sc);

   if (edf->requires_count)
     {
        unsigned int i;
        Edje_File *required_edf;
        char **requires = (char**)edf->requires;

        /* EET_DATA_DESCRIPTOR_ADD_VAR_ARRAY_STRING returns an mmapped blob, not stringshares */
        edf->requires = malloc(edf->requires_count * sizeof(char*));
        for (i = 0; i < edf->requires_count; i++)
          {
             char *str = (char*)requires[i];

             edf->requires[i] = eina_stringshare_add(str);
             required_edf = eina_hash_find(_edje_id_hash, edf->requires[i]);
             if (required_edf)
               {
                  required_edf->references++;
                  continue;
               }
             ERR("edje file '%s' REQUIRES file '%s' which is not loaded", edf->path, edf->requires[i]);
             if (!_edje_requires_pending)
               _edje_requires_pending = eina_hash_stringshared_new(NULL);
             eina_hash_list_append(_edje_requires_pending, edf->requires[i], edf);
          }
        free(requires);
     }
   if (_edje_requires_pending && edf->id)
     {
        l = eina_hash_set(_edje_requires_pending, edf->id, NULL);

        edf->references += eina_list_count(l);
        eina_list_free(l);

        if (!eina_hash_population(_edje_requires_pending))
          {
             eina_hash_free(_edje_requires_pending);
             _edje_requires_pending = NULL;
          }
     }

   if (edf->external_dir)
     {
        unsigned int i;

        for (i = 0; i < edf->external_dir->entries_count; ++i)
          edje_module_load(edf->external_dir->entries[i].entry);
     }

   // this call is unnecessary as we are doing same opeartion
   // inside _edje_textblock_style_parse_and_fix() function
   // remove ??
   //_edje_textblock_style_all_update(ed);

   if (edf->mo_dir)
     _edje_extract_mo_files(edf);

   return edf;
}

#if 0
// FIXME: find a way to remove dangling file earlier
static void
_edje_file_dangling(Edje_File *edf)
{
   if (edf->dangling) return;
   edf->dangling = EINA_TRUE;

   eina_hash_del(_edje_file_hash, &edf->f, edf);
   if (!eina_hash_population(_edje_file_hash))
     {
        eina_hash_free(_edje_file_hash);
        _edje_file_hash = NULL;
     }
}

#endif

/**
 * @internal
 * @brief Initializes the global Edje file cache structures if they haven't been already.
 *
 * This function ensures that `_edje_id_hash` (for mapping file IDs to Edje_File pointers)
 * and `_edje_file_hash` (for mapping Eina_File pointers to Edje_File pointers)
 * are created. These hash tables are fundamental for the Edje file caching mechanism.
 */
static inline void
_edje_file_cache_init()
{
   if (!_edje_id_hash)
     _edje_id_hash = eina_hash_stringshared_new(NULL);
   if (!_edje_file_hash)
     _edje_file_hash = eina_hash_pointer_new(NULL);
}

/**
 * @internal
 * @brief Retrieves an Edje_File from the cache's "trash" list.
 *
 * The "trash" list (`_edje_file_cache`) holds Edje_File structures that are
 * no longer actively referenced but are kept for a short period in case they
 * are needed again soon. This function searches this list for an Edje_File
 * corresponding to the given Eina_File.
 *
 * If found, the Edje_File is removed from the trash list, its reference count
 * is set to 1, it's re-added to the main active file hash (`_edje_file_hash`),
 * and then returned.
 *
 * @param file Pointer to the Eina_File whose corresponding Edje_File is sought
 *             in the trash cache.
 * @return Pointer to the Edje_File if found and successfully "popped" from trash,
 *         otherwise NULL.
 */
static inline Edje_File*
_edje_file_cache_trash_pop(const Eina_File *file)
{
   Edje_File *edf;
   Eina_List *l;

   EINA_LIST_FOREACH(_edje_file_cache, l, edf)
     {
        if (edf->f == file)
          {
             edf->references = 1;
             _edje_file_cache = eina_list_remove_list(_edje_file_cache, l);
             eina_hash_direct_add(_edje_file_hash, &edf->f, edf);
             return edf;
          }
     }
   return NULL;
}

/**
 * @internal
 * @brief Finds an Edje_File in the cache or loads it if not found.
 *
 * This function attempts to locate an Edje_File associated with the given
 * Eina_File handle. It performs the search in two stages:
 * 1. Checks the main active file hash (`_edje_file_hash`). If found,
 *    its reference count is incremented and it's returned.
 * 2. If not in the active hash, checks the "trash" list (`_edje_file_cache`)
 *    via `_edje_file_cache_trash_pop()`. If found there, it's reactivated.
 *
 * The cache structures are initialized via `_edje_file_cache_init()` if they
 * haven't been already.
 *
 * @param file Pointer to the Eina_File to find in the cache.
 * @return Pointer to the cached Edje_File with its reference count incremented,
 *         or NULL if the file is not in any part of the cache. This function
 *         does *not* open the file if it's not cached; that's typically handled
 *         by its callers like `_edje_cache_file_coll_open`.
 */
Edje_File*
_edje_file_cache_find(const Eina_File *file)
{
   Edje_File *edf;

   // initialize cache.
   _edje_file_cache_init();

   // serach in the file_hash.
   edf = eina_hash_find(_edje_file_hash, &file);
   if (edf)
     {
        edf->references++;
        return edf;
     }

   // search in the trash list
   return _edje_file_cache_trash_pop(file);
}

/**
 * @internal
 * @brief Adds a newly opened Edje_File to the active cache.
 *
 * This function registers an Edje_File into the main caching structures:
 * - It's added to `_edje_file_hash`, keyed by its `edf->f` (Eina_File pointer).
 * - If the Edje_File has an ID (`edf->id`), it's also added to `_edje_id_hash`,
 *   keyed by this string ID. This allows lookup by ID, which is useful for
 *   handling inter-file dependencies (requirements).
 *
 * @param edf Pointer to the Edje_File to add to the cache. It's assumed
 *            this file has just been opened and its reference count is
 *            appropriately set (usually to 1).
 */
static inline void
_edje_file_cache_add(Edje_File *edf)
{
   eina_hash_direct_add(_edje_file_hash, &edf->f, edf);
   if (edf->id)
     eina_hash_list_append(_edje_id_hash, edf->id, edf);
}

/**
 * @internal
 * @brief Opens an Edje file and a specific collection within it, utilizing the cache.
 *
 * This is a central function for accessing Edje collections. It first tries to
 * find the Edje_File in the cache using `_edje_file_cache_find()`. If not found,
 * it opens the file using `_edje_file_open()` and adds it to the cache.
 *
 * If a collection name (`coll`) is provided, it then attempts to open that
 * collection from the Edje_File using `_edje_file_coll_open()`.
 *
 * @param file Pointer to the Eina_File representing the Edje file.
 * @param coll The name of the collection (group) to open. Can be NULL if only
 *             the Edje_File itself is needed.
 * @param error_ret Pointer to an integer where an Edje_Load_Error code will be
 *                  stored in case of failure.
 * @param edc_ret Pointer to an Edje_Part_Collection pointer, which will be
 *                set to the opened collection if `coll` is not NULL and opening
 *                is successful.
 * @param ed Unused parameter (marked EINA_UNUSED).
 * @return Pointer to the Edje_File. This will be non-NULL if the file was
 *         successfully found in cache or opened. If `coll` was specified and
 *         the collection failed to open, `*edc_ret` will be NULL and
 *         `*error_ret` will contain the error, but the Edje_File might still
 *         be returned if the file itself was valid. Returns NULL if the
 *         Edje_File itself cannot be opened.
 */
Edje_File *
_edje_cache_file_coll_open(const Eina_File *file, const char *coll, int *error_ret, Edje_Part_Collection **edc_ret, Edje *ed EINA_UNUSED)
{
   Edje_File *edf;
   Edje_Part_Collection *edc;

   edf = _edje_file_cache_find(file);

   if (!edf)
     {
        edf = _edje_file_open(file, error_ret, eina_file_mtime_get(file), !!coll);
        if (!edf) return NULL;
        _edje_file_cache_add(edf);
     }

   if (!coll)
     return edf;

   edc = _edje_file_coll_open(edf, coll);
   if (!edc)
     *error_ret = EDJE_LOAD_ERROR_UNKNOWN_COLLECTION;

   if (edc_ret) *edc_ret = edc;

   return edf;
}

/**
 * @internal
 * @brief Cleans the collection cache for a given Edje_File.
 *
 * This function ensures that the number of cached collections for the specified
 * Edje_File does not exceed `_edje_collection_cache_size`. If it does,
 * the least recently used collections (those at the end of `edf->collection_cache` list)
 * are freed using `_edje_collection_free()` until the cache size is within limit.
 *
 * @param edf Pointer to the Edje_File whose collection cache needs cleaning.
 */
void
_edje_cache_coll_clean(Edje_File *edf)
{
   while ((edf->collection_cache) &&
          (eina_list_count(edf->collection_cache) > (unsigned int)_edje_collection_cache_size))
     {
        Edje_Part_Collection_Directory_Entry *ce;
        Edje_Part_Collection *edc;

        edc = eina_list_data_get(eina_list_last(edf->collection_cache));
        edf->collection_cache = eina_list_remove_list(edf->collection_cache, eina_list_last(edf->collection_cache));

        ce = eina_hash_find(edf->collection, edc->part);
        _edje_collection_free(edf, edc, ce);
     }
}

/**
 * @internal
 * @brief Flushes (empties) the collection cache for a given Edje_File.
 *
 * This function removes and frees all collections currently held in the
 * `edf->collection_cache` list for the specified Edje_File.
 * Each collection is freed using `_edje_collection_free()`.
 *
 * @param edf Pointer to the Edje_File whose collection cache is to be flushed.
 */
void
_edje_cache_coll_flush(Edje_File *edf)
{
   while (edf->collection_cache)
     {
        Edje_Part_Collection_Directory_Entry *ce;
        Edje_Part_Collection *edc;
        Eina_List *last;

        last = eina_list_last(edf->collection_cache);
        edc = eina_list_data_get(last);
        edf->collection_cache = eina_list_remove_list(edf->collection_cache,
                                                      last);

        ce = eina_hash_find(edf->collection, edc->part);
        _edje_collection_free(edf, edc, ce);
     }
}

/**
 * @internal
 * @brief Decrements the reference count of an Edje_Part_Collection.
 *
 * If the reference count drops to zero, the collection is either freed
 * immediately (if its parent Edje_File is marked as 'dangling') or moved
 * to the `edf->collection_cache` list for potential reuse.
 * The `_edje_cache_coll_clean()` function is then called to ensure the
 * cache size limits are respected.
 *
 * @param edf Pointer to the Edje_File to which the collection belongs.
 * @param edc Pointer to the Edje_Part_Collection whose reference count is to be decremented.
 */
void
_edje_cache_coll_unref(Edje_File *edf, Edje_Part_Collection *edc)
{
   Edje_Part_Collection_Directory_Entry *ce;

   edc->references--;
   if (edc->references != 0) return;

   ce = eina_hash_find(edf->collection, edc->part);
   if (!ce)
     {
        ERR("Something is wrong with reference count of '%s'.", edc->part);
     }
   else if (ce->ref)
     {
        ce->ref = NULL;

        if (edf->dangling)
          {
             /* No need to keep the collection around if the file is dangling */
             _edje_collection_free(edf, edc, ce);
             _edje_cache_coll_flush(edf);
          }
        else
          {
             edf->collection_cache = eina_list_prepend(edf->collection_cache, edc);
             _edje_cache_coll_clean(edf);
          }
     }
}

/**
 * @internal
 * @brief Cleans the global Edje file cache.
 *
 * This function ensures that the number of Edje_File structures in the
 * `_edje_file_cache` (the "trash" list of unreferenced files) does not
 * exceed `_edje_file_cache_size`. If it does, the least recently used
 * files (those at the end of the list) are freed using `_edje_file_free()`
 * until the cache size is within limit.
 */
static void
_edje_cache_file_clean(void)
{
   int count;

   count = eina_list_count(_edje_file_cache);
   while ((_edje_file_cache) && (count > _edje_file_cache_size))
     {
        Eina_List *last;
        Edje_File *edf;

        last = eina_list_last(_edje_file_cache);
        edf = eina_list_data_get(last);
        _edje_file_cache = eina_list_remove_list(_edje_file_cache, last);
        _edje_file_free(edf);
        count = eina_list_count(_edje_file_cache);
     }
}

/**
 * @internal
 * @brief Decrements the reference count of an Edje_File.
 *
 * If the reference count drops to zero, this function handles the file's
 * removal or caching.
 * - It recursively decrements references to any files this Edje_File `requires`.
 * - If the file is marked 'dangling', it's freed immediately.
 * - Otherwise, it's removed from the active file hashes (`_edje_file_hash`, `_edje_id_hash`)
 *   and added to the `_edje_file_cache` (the "trash" list) for potential reuse.
 * - `_edje_cache_file_clean()` is then called to maintain cache size limits.
 *
 * If hash tables become empty after removal, they are freed.
 *
 * @param edf Pointer to the Edje_File whose reference count is to be decremented.
 */
EAPI void
_edje_cache_file_unref(Edje_File *edf)
{
   edf->references--;
   if (edf->references != 0) return;

   if (edf->requires_count)
     {
        unsigned int i;

        for (i = 0; i < edf->requires_count; i++)
          {
             Edje_File *required_edf = eina_hash_find(_edje_id_hash, edf->requires[i]);

             if (required_edf)
               _edje_cache_file_unref(edf);
             else if (_edje_requires_pending)
               {
                  eina_hash_list_remove(_edje_requires_pending, edf->requires[i], edf);
                  if (!eina_hash_population(_edje_requires_pending))
                    {
                       eina_hash_free(_edje_requires_pending);
                       _edje_requires_pending = NULL;
                    }
               }
          }
     }

   if (edf->dangling)
     {
        _edje_file_free(edf);
        return;
     }

   if (edf->id)
     eina_hash_list_remove(_edje_id_hash, edf->id, edf);
   if (!eina_hash_population(_edje_id_hash))
     {
        eina_hash_free(_edje_id_hash);
        _edje_id_hash = NULL;
     }
   eina_hash_del(_edje_file_hash, &edf->f, edf);
   if (!eina_hash_population(_edje_file_hash))
     {
        eina_hash_free(_edje_file_hash);
        _edje_file_hash = NULL;
     }
   _edje_file_cache = eina_list_prepend(_edje_file_cache, edf);
   _edje_cache_file_clean();
}

/**
 * @internal
 * @brief Shuts down the Edje file cache system.
 *
 * This function effectively flushes the entire file cache by calling
 * `edje_file_cache_flush()`. This ensures all cached Edje_File
 * structures are freed.
 *
 * This is typically called during Edje library shutdown.
 */
void
_edje_file_cache_shutdown(void)
{
   edje_file_cache_flush();
}

/*============================================================================*
*                                 Global                                     *
*============================================================================*/

/*============================================================================*
*                                   API                                      *
*============================================================================*/

/**
 * @brief Sets the maximum number of Edje files to keep in the cache.
 *
 * This function adjusts the size of the global Edje file cache.
 * If the new `count` is smaller than the current number of cached files,
 * `_edje_cache_file_clean()` is called to trim the cache.
 *
 * @param count The new maximum number of files to cache. If negative, it's treated as 0.
 *
 * @see edje_file_cache_get()
 * @see edje_file_cache_flush()
 */
EAPI void
edje_file_cache_set(int count)
{
   if (count < 0) count = 0;
   _edje_file_cache_size = count;
   _edje_cache_file_clean();
}

/**
 * @brief Gets the current maximum number of Edje files to keep in the cache.
 *
 * @return The current maximum size of the Edje file cache.
 *
 * @see edje_file_cache_set()
 */
EAPI int
edje_file_cache_get(void)
{
   return _edje_file_cache_size;
}

/**
 * @brief Flushes the entire Edje file cache.
 *
 * This function clears all Edje_File structures currently held in the
 * file cache. It temporarily sets the cache size to 0, calls
 * `_edje_cache_file_clean()` to free all cached files, and then restores
 * the original cache size.
 *
 * @see edje_file_cache_set()
 */
EAPI void
edje_file_cache_flush(void)
{
   int ps;

   ps = _edje_file_cache_size;
   _edje_file_cache_size = 0;
   _edje_cache_file_clean();
   _edje_file_cache_size = ps;
}

/**
 * @brief Sets the maximum number of Edje collections to keep cached per Edje file.
 *
 * This function adjusts the size of the collection cache for each Edje file.
 * If the new `count` is smaller, `_edje_cache_coll_clean()` is called for
 * each file currently in the file cache to trim their respective collection caches.
 *
 * @note This function currently only affects files in `_edje_file_cache` (the "trash" list).
 *       A FIXME comment indicates it should also iterate through files in `_edje_file_hash` (active files).
 *
 * @param count The new maximum number of collections to cache per file.
 *              If negative, it's treated as 0.
 *
 * @see edje_collection_cache_get()
 * @see edje_collection_cache_flush()
 */
EAPI void
edje_collection_cache_set(int count)
{
   Eina_List *l;
   Edje_File *edf;

   if (count < 0) count = 0;
   _edje_collection_cache_size = count;
   EINA_LIST_FOREACH(_edje_file_cache, l, edf)
     _edje_cache_coll_clean(edf);
   /* FIXME: freach in file hash too! */
}

/**
 * @brief Gets the current maximum number of Edje collections cached per Edje file.
 *
 * @return The current maximum size of the collection cache per file.
 *
 * @see edje_collection_cache_set()
 */
EAPI int
edje_collection_cache_get(void)
{
   return _edje_collection_cache_size;
}

/**
 * @brief Flushes the collection cache for all Edje files.
 *
 * This function clears all Edje_Part_Collection structures currently held in
 * the collection caches of all Edje files. It temporarily sets the global
 * collection cache size to 0, calls `_edje_cache_coll_flush()` for each
 * file in the file cache, and then restores the original collection cache size.
 *
 * @note This function currently only affects files in `_edje_file_cache` (the "trash" list).
 *       A FIXME comment indicates it should also iterate through files in `_edje_file_hash` (active files).
 *
 * @see edje_collection_cache_set()
 */
EAPI void
edje_collection_cache_flush(void)
{
   int ps;
   Eina_List *l;
   Edje_File *edf;

   ps = _edje_collection_cache_size;
   _edje_collection_cache_size = 0;
   EINA_LIST_FOREACH(_edje_file_cache, l, edf)
     _edje_cache_coll_flush(edf);
   /* FIXME: freach in file hash too! */
   _edje_collection_cache_size = ps;
}

