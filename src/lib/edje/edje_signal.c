#include "edje_private.h"
#include <assert.h>

static Eina_Hash *signal_match = NULL;

/**
 * @internal
 * @brief Get the length of the key for the signal_match hash table.
 *
 * This function is used by Eina_Hash to determine the size of the key
 * structure Edje_Signal_Callback_Matches.
 *
 * @param key Unused.
 * @return The size of Edje_Signal_Callback_Matches.
 */
static unsigned int
_edje_signal_match_key_length(const void *key EINA_UNUSED)
{
   return sizeof(Edje_Signal_Callback_Matches);
}

/**
 * @internal
 * @brief Compare two keys for the signal_match hash table.
 *
 * This function is used by Eina_Hash to compare two Edje_Signal_Callback_Matches
 * structures. It compares the matches_count, and then each individual match's
 * signal, source, callback pointer, and free_cb if present.
 *
 * @param key1 The first key to compare.
 * @param key1_length Unused.
 * @param key2 The second key to compare.
 * @param key2_length Unused.
 * @return 0 if keys are equal, a negative value if key1 < key2,
 *         or a positive value if key1 > key2.
 */
static int
_edje_signal_match_key_cmp(const void *key1, int key1_length EINA_UNUSED, const void *key2, int key2_length EINA_UNUSED)
{
   const Edje_Signal_Callback_Matches *a = key1;
   const Edje_Signal_Callback_Matches *b = key2;
   unsigned int i;

#define NOTEQUAL(x) (a->x != b->x)
#define LESSMORE(x) ((a->x > b->x) ? 1 : -1)
   if (NOTEQUAL(matches_count)) return LESSMORE(matches_count);
   for (i = 0; i < a->matches_count; ++i)
     {
        if (NOTEQUAL(matches[i].signal)) return LESSMORE(matches[i].signal);
        if (NOTEQUAL(matches[i].source)) return LESSMORE(matches[i].source);
        // Callback be it legacy or eo, have the same pointer size and so can be just compared like that
        if (NOTEQUAL(matches[i].legacy)) return LESSMORE(matches[i].legacy);
        if (a->free_cb && b->free_cb)
          {
             if (NOTEQUAL(free_cb[i])) return LESSMORE(free_cb[i]);
          }
        else if (a->free_cb || b->free_cb) return LESSMORE(free_cb);
     }
   return 0;
}

/**
 * @internal
 * @brief Generate a hash value for a key in the signal_match hash table.
 *
 * This function is used by Eina_Hash to generate a hash value for an
 * Edje_Signal_Callback_Matches structure. The hash is computed based on
 * matches_count and the signal, source, callback pointer, and free_cb
 * (if present) of each match.
 *
 * @param key The key to hash.
 * @param key_length Unused.
 * @return The computed hash value.
 */
static int
_edje_signal_match_key_hash(const void *key, int key_length EINA_UNUSED)
{
   const Edje_Signal_Callback_Matches *a = key;
   unsigned int hash, i;

   hash = eina_hash_int32(&a->matches_count, sizeof(int));
   for (i = 0; i < a->matches_count; ++i)
     {
#ifdef EFL64
# define HASH(x) eina_hash_int64((const unsigned long long int *)&(a->x), sizeof(a->x))
#else
# define HASH(x) eina_hash_int32((const unsigned int *)&(a->x), sizeof(a->x))
#endif
        hash ^= HASH(matches[i].signal);
        hash ^= HASH(matches[i].source);
        // Callback be it legacy or eo, have the same pointer size and so using legacy for hash is enough
        hash ^= HASH(matches[i].legacy);
        if (a->free_cb) hash ^= HASH(free_cb[i]);
     }
   return hash;
}

/**
 * @internal
 * @brief Duplicate an Edje_Signal_Callback_Matches structure.
 *
 * This function creates a deep copy of the source Edje_Signal_Callback_Matches
 * structure, including all its associated matches and free_cb functions.
 * Stringshare references are incremented for signals and sources.
 * The new structure has its refcount initialized to 1.
 *
 * @param src The source Edje_Signal_Callback_Matches structure to duplicate.
 * @return A pointer to the newly allocated and duplicated
 *         Edje_Signal_Callback_Matches structure, or NULL on allocation failure.
 */
static Edje_Signal_Callback_Matches *
_edje_signal_callback_matches_dup(const Edje_Signal_Callback_Matches *src)
{
   Edje_Signal_Callback_Matches *result;
   unsigned int i;

   result = calloc(1, sizeof (Edje_Signal_Callback_Matches));
   if (!result) return NULL;

   result->matches = malloc
     (sizeof(Edje_Signal_Callback_Match) * src->matches_count);
   if (!result->matches) goto err;
   result->matches_count = src->matches_count;
   EINA_REFCOUNT_REF(result);

   if (src->free_cb)
     {
        result->free_cb = malloc
          (sizeof(Eina_Free_Cb) * src->matches_count);
        if (!result->free_cb) goto err;
        memcpy(result->free_cb, src->free_cb,
               sizeof(Eina_Free_Cb) * src->matches_count);
     }

   for (i = 0; i < src->matches_count; i++)
     {
        result->matches[i].signal = eina_stringshare_ref(src->matches[i].signal);
        result->matches[i].source = eina_stringshare_ref(src->matches[i].source);
        result->matches[i].legacy = src->matches[i].legacy;
     }

   return result;
err:
   ERR("Allocation error in callback matches dup");
   free(result->free_cb);
   free(result->matches);
   free(result);
   return NULL;
}

/**
 * @internal
 * @brief Clean (unref and NULLify) the patterns associated with a callback group.
 *
 * This function unreferences the Edje_Signals_Sources_Patterns structure
 * within the Edje_Signal_Callback_Matches of the given group and sets the
 * pointer to NULL.
 *
 * @param gp The callback group whose patterns are to be cleaned.
 */
void
_edje_callbacks_patterns_clean(Edje_Signal_Callback_Group *gp)
{
   Edje_Signal_Callback_Matches *tmp = (Edje_Signal_Callback_Matches *)gp->matches;

   if (!tmp) return;
   _edje_signal_callback_patterns_unref(tmp->patterns);
   tmp->patterns = NULL;
}

/**
 * @internal
 * @brief Initialize the signal/source patterns for a callback group.
 *
 * If patterns do not already exist for the group's matches, this function
 * allocates and initializes an Edje_Signals_Sources_Patterns structure.
 * It then builds the hash for exact matches and initializes signal and source
 * patterns for globbing. The patterns structure is refcounted.
 *
 * @param gp The callback group for which to initialize patterns.
 */
static void
_edje_callbacks_patterns_init(Edje_Signal_Callback_Group *gp)
{
   Edje_Signals_Sources_Patterns *ssp;
   Edje_Signal_Callback_Matches *tmp = (Edje_Signal_Callback_Matches *)gp->matches;

   if (!tmp) return;
   if (tmp->patterns) return;

   tmp->patterns = calloc(1, sizeof(Edje_Signals_Sources_Patterns));
   if (!tmp->patterns) goto err;

   ssp = tmp->patterns;
   edje_match_callback_hash_build(tmp->matches,
                                  tmp->matches_count,
                                  &ssp->exact_match,
                                  &ssp->u.callbacks.globing);
   ssp->signals_patterns = edje_match_callback_signal_init
     (&ssp->u.callbacks.globing, tmp->matches);
   ssp->sources_patterns = edje_match_callback_source_init
     (&ssp->u.callbacks.globing, tmp->matches);
   EINA_REFCOUNT_REF(ssp);
   return;
err:
   ERR("Alloc error on patterns init");
}

/**
 * @internal
 * @brief Initialize the Edje signal subsystem.
 *
 * This function creates the global hash table `signal_match` used for storing
 * and finding shared Edje_Signal_Callback_Matches structures.
 */
void
edje_signal_init(void)
{
   signal_match = eina_hash_new(_edje_signal_match_key_length,
                                _edje_signal_match_key_cmp,
                                _edje_signal_match_key_hash,
                                NULL,
                                3);
}

/**
 * @internal
 * @brief Shutdown the Edje signal subsystem.
 *
 * This function frees the global hash table `signal_match`.
 * @note There is a FIXME to iterate and destroy leftover signal matchers,
 *       implying potential resource leaks if not handled properly before shutdown.
 */
void
edje_signal_shutdown(void)
{
   // FIXME: iterate and destroy leftover signal matcher
   eina_hash_free(signal_match);
}

/**
 * @internal
 * @brief Unset (clear) a callback at a specific index within a group.
 *
 * This function releases the stringshare references for the signal and source
 * of the callback match at the given index.
 *
 * @param gp The callback group.
 * @param idx The index of the callback to unset.
 */
static void
_edje_signal_callback_unset(Edje_Signal_Callback_Group *gp, int idx)
{
   Edje_Signal_Callback_Matches *tmp = (Edje_Signal_Callback_Matches *)gp->matches;
   Edje_Signal_Callback_Match *m;

   if (!tmp) return;
   if (!tmp->matches) return;
   m = tmp->matches + idx;
   eina_stringshare_del(m->signal);
   m->signal = NULL;
   eina_stringshare_del(m->source);
   m->source = NULL;
}

/**
 * @internal
 * @brief Set a callback at a specific index within a group.
 *
 * This function populates the callback match at the given index with the
 * provided signal, source, callback function (legacy or EO), free callback,
 * user data, and flags. It takes stringshare references for signal and source.
 * If a `func_free_cb` is provided and the `free_cb` array in `tmp` doesn't exist,
 * it's allocated.
 *
 * @param gp The callback group.
 * @param idx The index where the callback will be set.
 * @param sig The signal string (e.g., "mouse,clicked,1").
 * @param src The source string (e.g., "my_button").
 * @param func_legacy The legacy Edje_Signal_Cb callback function.
 * @param func_eo The EFL Efl_Signal_Cb callback function.
 * @param func_free_cb Optional callback to free user data when the callback is removed.
 * @param data User data to be passed to the callback.
 * @param flags Flags for the callback (e.g., delete_me, just_added).
 */
static void
_edje_signal_callback_set(Edje_Signal_Callback_Group *gp, int idx,
                          const char *sig, const char *src,
                          Edje_Signal_Cb func_legacy,
                          Efl_Signal_Cb func_eo, Eina_Free_Cb func_free_cb,
                          void *data, Edje_Signal_Callback_Flags flags)
{
   Edje_Signal_Callback_Matches *tmp = (Edje_Signal_Callback_Matches *)gp->matches;
   Edje_Signal_Callback_Match *m;

   if (!tmp) return;
   if (!tmp->matches) return;
   m = tmp->matches + idx;
   m->signal = eina_stringshare_ref(sig);
   m->source = eina_stringshare_ref(src);
   if (func_legacy) m->legacy = func_legacy;
   else m->eo = func_eo;
   if (func_free_cb)
     {
        if (!tmp->free_cb)
          tmp->free_cb = calloc(tmp->matches_count, sizeof(Eina_Free_Cb));
        if (!tmp->free_cb) goto err;
        tmp->free_cb[idx] = func_free_cb;
     }
   gp->custom_data[idx] = data;
   gp->flags[idx] = flags;
   return;
err:
   ERR("Alloc err in callback set");
}

/**
 * @internal
 * @brief Grow the arrays within an Edje_Signal_Callback_Group.
 *
 * This function increases the size of the `matches`, `free_cb` (if it exists),
 * `custom_data`, and `flags` arrays within the callback group by one element.
 * The `matches_count` in `tmp` (gp->matches) is incremented.
 * New elements are initialized to zero/NULL.
 *
 * @warning This function might reallocate `tmp->matches`. If `tmp->matches`
 *          changes, any previously built patterns based on the old pointer
 *          become invalid. This is handled in `_edje_signal_callback_push`.
 *
 * @param gp The callback group to grow.
 * @return The (potentially reallocated) callback group, or NULL on allocation failure.
 *         If an error occurs, `tmp->matches_count` is decremented back.
 */
static Edje_Signal_Callback_Group *
_edje_signal_callback_grow(Edje_Signal_Callback_Group *gp)
{
   Edje_Signal_Callback_Matches *tmp;
   Edje_Signal_Callback_Match *m;
   Eina_Free_Cb *f;
   void **cd;
   Edje_Signal_Callback_Flags *fl;

   tmp = (Edje_Signal_Callback_Matches *)gp->matches;
   if (!tmp) return NULL;
   tmp->matches_count++;
   // what about data in the data build by edje_match_callback_hash_build
   // that this may kill by changing the tmp->matches ptr. this is handled
   // in _edje_signal_callback_push() by re-initting patterns
   m = realloc(tmp->matches, sizeof(Edje_Signal_Callback_Match) * tmp->matches_count);
   if (!m) goto err;
   tmp->matches = m;
   memset(&(tmp->matches[tmp->matches_count - 1]), 0, sizeof(Edje_Signal_Callback_Match));
   if (tmp->free_cb)
     {
        f = realloc(tmp->free_cb, sizeof(Eina_Free_Cb) * tmp->matches_count);
        if (!f) goto err;
        tmp->free_cb = f;
        tmp->free_cb[tmp->matches_count - 1] = NULL;
     }
   cd = realloc(gp->custom_data, sizeof(void *) * tmp->matches_count);
   if (!cd) goto err;
   gp->custom_data = cd;
   gp->custom_data[tmp->matches_count - 1] = NULL;
   fl = realloc(gp->flags, sizeof(Edje_Signal_Callback_Flags) * tmp->matches_count);
   if (!fl) goto err;
   gp->flags = fl;
   memset(&(gp->flags[tmp->matches_count - 1]), 0, sizeof(Edje_Signal_Callback_Flags));
   return gp;
err:
   ERR("Allocation error in rowing signal callback group");
   tmp->matches_count--;
   return NULL;
}

/**
 * @internal
 * @brief Add a new callback to an Edje_Signal_Callback_Group.
 *
 * This function adds a new signal callback to the group.
 * It handles several cases:
 * 1. If the group's matches are shared (hashed and refcount > 1), it
 *    duplicates the matches to make a private copy before modification.
 * 2. If the group's matches are hashed but refcount is 1, it removes them
 *    from the shared hash to make them private.
 * 3. It searches for an empty (marked as `delete_me`) slot to reuse.
 * 4. If no empty slot is found, it grows the callback group.
 * If growing the group reallocates the `matches` array, it cleans and
 * reinitializes the associated patterns.
 *
 * @param gp The callback group to add to.
 * @param sig The signal string.
 * @param src The source string.
 * @param func_legacy The legacy Edje_Signal_Cb callback function.
 * @param func_eo The EFL Efl_Signal_Cb callback function.
 * @param func_free_cb Optional callback to free user data.
 * @param data User data for the callback.
 * @param propagate EINA_TRUE if the signal should propagate, EINA_FALSE otherwise.
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., allocation error).
 */
Eina_Bool
_edje_signal_callback_push(Edje_Signal_Callback_Group *gp,
                           const char *sig, const char *src,
                           Edje_Signal_Cb func_legacy,
                           Efl_Signal_Cb func_eo, Eina_Free_Cb func_free_cb,
                           void *data, Eina_Bool propagate)
{
   unsigned int i;
   Edje_Signal_Callback_Flags flags;
   Edje_Signal_Callback_Matches *tmp;
   Edje_Signal_Callback_Match *m;

   flags.delete_me = EINA_FALSE;
   flags.just_added = EINA_TRUE;
   flags.propagate = !!propagate;
   flags.legacy = !!func_legacy;

   // FIXME: properly handle legacy and non legacy case, including free function
   tmp = (Edje_Signal_Callback_Matches *)gp->matches;
   if (!tmp) return EINA_FALSE;
   if (tmp->hashed)
     {
        if (EINA_REFCOUNT_GET(tmp) == 1)
          {
             // special case - it's a single ref so make it private
             // and move it out of the shared hash to be private
             if (!eina_hash_del(signal_match, tmp, tmp))
               {
                  ERR("Can't del from hash!");
               }
             tmp->hashed = EINA_FALSE;
          }
        else
          {
             // already multiple refs to the match - so make a
             // private copy of it we can modify
             Edje_Signal_Callback_Matches *tmp_dup =
               _edje_signal_callback_matches_dup(tmp);
             if (!tmp_dup) return EINA_FALSE;
             // unreff tmp but it's > 1 ref so it'll be safe but we're not
             // using it anymore here so indicate that with the unref
             EINA_REFCOUNT_UNREF(tmp)
               {
                  (void)0; // do nothing because if refcount == 1 handle above.
               }
             gp->matches = tmp = tmp_dup;
          }
     }

   // tmp will not be hashed at this point so no need to del+add from hash
   // search an empty spot now
   for (i = 0; i < tmp->matches_count; i++)
     {
        if (gp->flags[i].delete_me)
          {
             _edje_signal_callback_unset(gp, i);
             _edje_signal_callback_set(gp, i, sig, src, func_legacy, func_eo, func_free_cb, data, flags);
             return EINA_TRUE;
          }
     }

   m = tmp->matches;
   if (_edje_signal_callback_grow(gp))
     {
        // Set propagate and just_added flags
        _edje_signal_callback_set(gp, tmp->matches_count - 1,
                                  sig, src, func_legacy, func_eo, func_free_cb, data, flags);
        if (m != tmp->matches)
          {
             _edje_callbacks_patterns_clean(gp);
             _edje_callbacks_patterns_init(gp);
          }
     }
   else
     {
        if (tmp->hashed)
          eina_hash_add(signal_match, tmp, tmp);
        goto err;
     }
   return EINA_TRUE;
err:
   ERR("Allocation error in pushing callback");
   return EINA_TRUE;
}

/**
 * @internal
 * @brief Allocate a new Edje_Signal_Callback_Group.
 *
 * This function allocates memory for an Edje_Signal_Callback_Group and its
 * initial Edje_Signal_Callback_Matches structure. The matches structure
 * is refcounted and initialized with a refcount of 1.
 *
 * @return A pointer to the newly allocated Edje_Signal_Callback_Group,
 *         or NULL on allocation failure.
 */
const Edje_Signal_Callback_Group *
_edje_signal_callback_alloc(void)
{
   Edje_Signal_Callback_Group *escg;
   Edje_Signal_Callback_Matches *m;

   escg = calloc(1, sizeof (Edje_Signal_Callback_Group));
   if (!escg) goto err;
   m = calloc(1, sizeof (Edje_Signal_Callback_Matches));
   if (!m) goto err;
   EINA_REFCOUNT_REF(m);
   escg->matches = m;
   return escg;
err:
   ERR("Alloc error in signal callback alloc");
   free(escg);
   return NULL;
}

/**
 * @internal
 * @brief Unreference an Edje_Signal_Callback_Matches structure.
 *
 * Decrements the reference count of the given Edje_Signal_Callback_Matches `m`.
 * If the reference count drops to zero, this function performs cleanup:
 * - Calls `free_cb` for each valid (not `delete_me`) match if `m->free_cb` exists.
 * - If `m` was hashed, removes it from the global `signal_match` hash table.
 * - Deletes stringshare references for all signals and sources in `m->matches`.
 * - Unreferences and cleans associated patterns (`m->patterns`).
 * - Frees `m->matches`, `m->free_cb`, and `m` itself.
 *
 * @param m The Edje_Signal_Callback_Matches structure to unreference.
 * @param flags Array of flags corresponding to each match in `m`. Used to check
 *              `delete_me` status before calling `free_cb`.
 * @param custom_data Array of custom data pointers corresponding to each match.
 *                    Passed to `free_cb`.
 */
void
_edje_signal_callback_matches_unref(Edje_Signal_Callback_Matches *m,
                                    Edje_Signal_Callback_Flags *flags,
                                    void **custom_data)
{
   unsigned int i;

   if (m->free_cb)
     {
        for (i = 0; i < m->matches_count; ++i)
          {
             if (!flags[i].delete_me && m->free_cb[i])
               m->free_cb[i](custom_data[i]);
          }
     }

   EINA_REFCOUNT_UNREF(m)
     {
        if (m->hashed)
          {
             if (!eina_hash_del(signal_match, m, m))
               {
                  ERR("Can't del from hash!");
               }
          }
        for (i = 0; i < m->matches_count; ++i)
          {
             eina_stringshare_del(m->matches[i].signal);
             eina_stringshare_del(m->matches[i].source);
             m->matches[i].signal = NULL;
             m->matches[i].source = NULL;
          }
        _edje_signal_callback_patterns_unref(m->patterns);
        free(m->matches);
        free(m->free_cb);
        m->patterns = NULL;
        m->matches = NULL;
        m->free_cb = NULL;
        m->hashed = EINA_FALSE;
        free(m);
     }
}

/**
 * @internal
 * @brief Free an Edje_Signal_Callback_Group.
 *
 * This function frees all resources associated with an Edje_Signal_Callback_Group.
 * It unreferences the `matches` structure (which handles its own deallocation
 * and cleanup via `_edje_signal_callback_matches_unref`), frees the `flags`
 * and `custom_data` arrays, and then frees the group structure itself.
 *
 * @param cgp The callback group to free.
 */
void
_edje_signal_callback_free(const Edje_Signal_Callback_Group *cgp)
{
   Edje_Signal_Callback_Group *gp = (Edje_Signal_Callback_Group *)cgp;

   if (!gp) return;
   _edje_signal_callback_matches_unref
     ((Edje_Signal_Callback_Matches *)gp->matches, gp->flags, gp->custom_data);
   free(gp->flags);
   free(gp->custom_data);
   gp->matches = NULL;
   gp->flags = NULL;
   gp->custom_data = NULL;
   free(gp);
}

/**
 * @internal
 * @brief Disable (mark for deletion) a specific callback in a group.
 *
 * Searches for a callback matching the provided signal, source, function pointer
 * (legacy or EO), free callback (if applicable for EO), and data.
 * If a match is found and it's not already marked for deletion:
 * - If it's an EO callback (`func`) and `func_free_cb` is provided, `func_free_cb(data)` is called.
 * - The callback's `delete_me` flag is set to EINA_TRUE.
 *
 * @param gp The callback group.
 * @param sig The signal string to match.
 * @param src The source string to match.
 * @param func_legacy The legacy Edje_Signal_Cb function to match.
 * @param func The EflLayoutSignalCb (EO) function to match.
 * @param func_free_cb The Eina_Free_Cb associated with `func` to match.
 * @param data The user data to match.
 * @return EINA_TRUE if a callback was found and marked for deletion,
 *         EINA_FALSE otherwise or if `gp` or `gp->matches` is NULL.
 */
Eina_Bool
_edje_signal_callback_disable(Edje_Signal_Callback_Group *gp,
                              const char *sig, const char *src,
                              Edje_Signal_Cb func_legacy,
                              EflLayoutSignalCb func, Eina_Free_Cb func_free_cb, void *data)
{
   unsigned int i;

   if (!gp || !gp->matches) return EINA_FALSE;

   for (i = 0; i < gp->matches->matches_count; ++i)
     {
        if ((sig == gp->matches->matches[i].signal) &&
            (src == gp->matches->matches[i].source) &&
            (!gp->flags[i].delete_me) &&
            (((func == gp->matches->matches[i].eo) &&
              ((!gp->matches->free_cb) || (func_free_cb == gp->matches->free_cb[i])) &&
              (gp->custom_data[i] == data) &&
              (!gp->flags[i].legacy)) ||
             ((func_legacy == gp->matches->matches[i].legacy) &&
              (gp->custom_data[i] == data) &&
              (gp->flags[i].legacy)))
            )
          {
             if (func && func_free_cb) func_free_cb(data);
             gp->flags[i].delete_me = EINA_TRUE;
             //return gp->custom_data[i];
             return EINA_TRUE;
          }
     }
   return EINA_FALSE;
}

/**
 * @internal
 * @brief Compact the callback array by moving a valid entry into a deleted slot.
 *
 * This function is typically called when compacting the callback list after
 * some callbacks have been marked with `delete_me`. It attempts to move the
 * last valid (not `delete_me`) callback from the end of the `matches` array
 * into the slot at index `i`.
 *
 * If the `matches` structure `m` is hashed, it's temporarily removed from
 * the `signal_match` hash before modification and re-added afterwards.
 * The function iterates backwards from the end of the `matches` array.
 * If a non-`delete_me` entry is found at index `j`:
 *  - The entry at index `i` is unset (strings unref'd).
 *  - The entry from `j` is copied to `i`.
 *  - Corresponding `flags` and `custom_data` are also copied.
 *  - `m->matches_count` is decremented.
 * If an entry at `j` is `delete_me`, it's unset and `m->matches_count` is
 * decremented, and the loop continues.
 *
 * @param gp The callback group.
 * @param i The index of the slot to fill (which was presumably marked `delete_me`).
 */
static void
_edje_signal_callback_move_last(Edje_Signal_Callback_Group *gp,
                                unsigned int i)
{
   Edje_Signal_Callback_Matches *m;
   unsigned int j;

   m = (Edje_Signal_Callback_Matches *)gp->matches;
   if (!m) return;

   if (m->hashed)
     {
        if (!eina_hash_del(signal_match, m, m))
          {
             ERR("Can't del from hash!");
          }
     }
   for (j = --m->matches_count; j > i; --j)
     {
        if (!gp->flags[j].delete_me)
          {
             _edje_signal_callback_unset(gp, i);
             memcpy(&m->matches[i], &m->matches[j], sizeof(Edje_Signal_Callback_Match));
             gp->flags[i] = gp->flags[j];
             gp->custom_data[i] = gp->custom_data[j];
             if (m->hashed)
               eina_hash_add(signal_match, m, m);
             return;
          }
        else
          {
             _edje_signal_callback_unset(gp, j);
             m->matches_count--;
          }
     }
   if (m->hashed)
     eina_hash_add(signal_match, m, m);
}

/**
 * @internal
 * @brief Get a reference to the signal/source patterns for a callback group.
 *
 * This function ensures that the callback group `gp` uses a shared, canonical
 * `Edje_Signal_Callback_Matches` structure if an identical one already exists
 * in the `signal_match` hash. It also ensures that the associated
 * `Edje_Signals_Sources_Patterns` are initialized.
 *
 * Logic:
 * 1. If `gp->matches` (aliased as `tmp`) is already hashed, its patterns are returned.
 * 2. If not hashed, it searches `signal_match` for an identical `Edje_Signal_Callback_Matches` (`m`).
 *    a. If no match `m` is found:
 *       - Compacts `tmp->matches` by removing `delete_me` entries if its patterns
 *         are not shared (refcount <= 1). This is done before building new patterns.
 *       - Cleans any existing patterns on `tmp`.
 *       - Initializes new patterns for `tmp` using `_edje_callbacks_patterns_init`.
 *       - Tries to find `m` again (should ideally not be found if logic is correct,
 *         but a WRN exists if it is).
 *       - Adds `tmp` to `signal_match` and marks it as hashed.
 *    b. If a match `m` is found and `m` is different from `tmp`:
 *       - It means `tmp` is a duplicate of an existing shared `m`.
 *       - `tmp` is unreferenced (which might free it).
 *       - `gp->matches` is updated to point to the shared `m`.
 *       - The refcount of the new `gp->matches` (which is `m`) is incremented.
 *       - Handles potential size mismatches between `tmp` and `m` for `custom_data` and `flags`.
 * 3. Finally, the refcount of `gp->matches->patterns` is incremented and it's returned.
 *
 * @param gp The callback group.
 * @return A refcounted pointer to the `Edje_Signals_Sources_Patterns` for the group,
 *         or NULL if `gp` or `gp->matches` is NULL.
 */
const Edje_Signals_Sources_Patterns *
_edje_signal_callback_patterns_ref(Edje_Signal_Callback_Group *gp)
{
   const Edje_Signal_Callback_Matches *m;
   Edje_Signal_Callback_Matches *tmp;

   tmp = (Edje_Signal_Callback_Matches *)gp->matches;
   if (!tmp) return NULL;
   if (tmp->hashed) goto got_it;
   m = eina_hash_find(signal_match, tmp);
   if (!m)
     {
        if (!(tmp->patterns && (EINA_REFCOUNT_GET(tmp->patterns) > 1)))
          {
             // Let compact it and remove uneeded pattern before building it
             // We can do that because the custom data are kept local into the matching code.
             unsigned int i;

             for (i = 0; i < tmp->matches_count; i++)
               {
                  if (gp->flags[i].delete_me)
                    _edje_signal_callback_move_last((Edje_Signal_Callback_Group *)gp, i);
               }
          }
        _edje_signal_callback_patterns_unref(tmp->patterns);
        tmp->patterns = NULL;
        _edje_callbacks_patterns_init((Edje_Signal_Callback_Group *)gp);
        m = eina_hash_find(signal_match, tmp);
        if (m)
          {
             WRN("Found exact match in signal matches this would conflict with");
             goto got_it;
          }
        // We should be able to use direct_add, but if I do so valgrind stack explode and
        // it bagain to be a pain to debug efl apps. I can't understand what is going on.
        // eina_hash_direct_add(signal_match, tmp, tmp);
        eina_hash_add(signal_match, tmp, tmp);
        tmp->hashed = EINA_TRUE;
     }
   else
     {
        if (m == tmp)
          {
             WRN("Should not happen - gp->match == hash found match");
             goto got_it;
          }
        if (tmp->matches_count != m->matches_count)
          {
             unsigned int i, smaller, larger;
             void **cd;
             Edje_Signal_Callback_Flags *fl;

             ERR("Match replacement match count don't match");
             smaller = tmp->matches_count;
             larger = m->matches_count;
             if (larger > smaller)
               {
                  cd = realloc(gp->custom_data, sizeof(void *) * larger);
                  for (i = smaller; i < larger; i++) cd[i] = NULL;
                  gp->custom_data = cd;
                  fl = realloc(gp->flags, sizeof(Edje_Signal_Callback_Flags) * larger);
                  for (i = smaller; i < larger; i++) memset(&(fl[i]), 0, sizeof(Edje_Signal_Callback_Flags));
                  gp->flags = fl;
               }
          }
        _edje_signal_callback_matches_unref
          ((Edje_Signal_Callback_Matches *)tmp, gp->flags, gp->custom_data);
        ((Edje_Signal_Callback_Group *)gp)->matches = m;
        tmp = (Edje_Signal_Callback_Matches *)gp->matches;
        EINA_REFCOUNT_REF(tmp);
     }

got_it:
   if (tmp->patterns) EINA_REFCOUNT_REF(tmp->patterns);
   return tmp->patterns;
}

/**
 * @internal
 * @brief Unreference an Edje_Signals_Sources_Patterns structure.
 *
 * Decrements the reference count of the given patterns structure `ssp`.
 * If the reference count drops to zero, this function cleans up its internal
 * data (exact_match rbtree, globing inarray) and frees the `ssp` structure itself.
 *
 * @param essp The Edje_Signals_Sources_Patterns structure to unreference.
 */
void
_edje_signal_callback_patterns_unref(const Edje_Signals_Sources_Patterns *essp)
{
   Edje_Signals_Sources_Patterns *ssp = (Edje_Signals_Sources_Patterns *)essp;

   if (!ssp) return;
   EINA_REFCOUNT_UNREF(ssp)
     {
        _edje_signals_sources_patterns_clean(ssp);
        eina_rbtree_delete(ssp->exact_match,
                           EINA_RBTREE_FREE_CB(edje_match_signal_source_free),
                           NULL);
        ssp->exact_match = NULL;
        eina_inarray_flush(&ssp->u.callbacks.globing);
        free(ssp);
     }
}

/**
 * @internal
 * @brief Reset the `just_added` flag for all callbacks in a flag array.
 *
 * Iterates through an array of `Edje_Signal_Callback_Flags` and sets the
 * `just_added` member of each flag to `EINA_FALSE`. This is typically called
 * after processing newly added callbacks in a signal emission cycle.
 *
 * @param flags Pointer to the array of Edje_Signal_Callback_Flags.
 * @param length The number of elements in the `flags` array.
 */
void
_edje_signal_callback_reset(Edje_Signal_Callback_Flags *flags, unsigned int length)
{
   unsigned int i;
   for (i = 0; i < length; ++i)
     flags[i].just_added = EINA_FALSE;
}

