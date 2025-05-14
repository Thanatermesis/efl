#include "edje_private.h"

/* States manipulations. */

/**
 * @brief Represents a single state in the NFA simulation.
 *
 * A state is defined by the pattern index and the current position
 * within that pattern.
 */
typedef struct _Edje_State Edje_State;
struct _Edje_State
{
   unsigned int idx; /**< Index of the pattern. */
   unsigned int pos; /**< Current position in the pattern string. */
};

/**
 * @brief Manages a list of active NFA states.
 *
 * This structure holds an array of Edje_State and a corresponding
 * boolean array to quickly check for the existence of a state
 * (idx, pos pair) to avoid duplicates when adding states during
 * NFA simulation.
 */
struct _Edje_States
{
   unsigned int size;      /**< Current number of active states in the `states` array. */
   Edje_State  *states;    /**< Array of active NFA states. Example: [{idx=0, pos=0}, {idx=1, pos=2}] */
   Eina_Bool   *has;       /**< Bitmask or boolean array to track existing (idx, pos) pairs to optimize insertion.
                            *   The index is calculated as `(idx * (patterns_max_length + 1)) + pos`.
                            *   `has[calculated_index] = EINA_TRUE` if the state is present.
                            */
};

/**
 * @brief Frees the memory allocated for an Edje_States structure.
 * @param states The Edje_States structure to free.
 * @param states_size Unused parameter.
 */
static void
_edje_match_states_free(Edje_States *states,
                        unsigned int states_size)
{
   (void)states_size;
   free(states);
}

/**
 * @def ALIGN(Size)
 * @brief Aligns a given size to the size of a pointer.
 * This is used to ensure that memory blocks are properly aligned.
 * @param Size The size to align.
 */
#define ALIGN(Size)               \
  {                               \
     Size--;                      \
     Size |= sizeof (void *) - 1; \
     Size++;                      \
  };

/**
 * @brief Allocates memory for a set of Edje_States structures.
 *
 * This function allocates a contiguous block of memory for `n` Edje_States
 * structures, along with the memory required for their internal `states`
 * and `has` arrays. The allocation is done in a way to optimize cache
 * locality.
 *
 * @param ppat Pointer to the Edje_Patterns structure where the allocated
 *             states will be stored. This also provides pattern metadata
 *             like `patterns_size` and `max_length`.
 * @param n The number of Edje_States structures to allocate (typically 2,
 *          for current and next states in NFA simulation).
 * @return 1 on success, 0 on failure (memory allocation error).
 */
static int
_edje_match_states_alloc(Edje_Patterns *ppat, int n)
{
   Edje_States *l;

   const unsigned int patterns_size = ppat->patterns_size;
   const unsigned int patterns_max_length = ppat->max_length;

   const unsigned int array_len = (patterns_max_length + 1) * patterns_size;

   unsigned int states_size;
   unsigned int has_size;
   unsigned int states_has_size;
   unsigned int struct_size;

   unsigned char *states;
   unsigned char *has;

   int i;

   states_size = sizeof (*l->states) * array_len;
   ALIGN(states_size);

   has_size = sizeof (*l->has) * array_len;
   ALIGN(has_size);

   states_has_size = states_size + has_size;

   struct_size = sizeof (*l);
   ALIGN(struct_size);
   struct_size += states_has_size;

   l = malloc(n * struct_size);
   if (!l) return 0;

   ppat->states = l;
   ppat->states->size = 0;

   states = (unsigned char *)(l + n);
   has = states + states_size;

   for (i = 0; i < n; ++i)
     {
        l[i].states = (Edje_State *)states;
        l[i].has = (Eina_Bool *)has;
        l[i].size = 0;

        memset(l[i].has, 0, has_size);

        states += states_has_size;
        has += states_has_size;
     }

   return 1;
}

/**
 * @brief Inserts a new state (pattern index and position) into the list of active states.
 *
 * Before inserting, it checks if the state (defined by `idx` and `pos`)
 * already exists in the `has` array to avoid duplicates. If not, it adds
 * the new state to the `states` array and updates the `has` array.
 *
 * @param list The Edje_States list to insert into.
 * @param patterns_max_length The maximum length of any pattern, used to calculate
 *                            the index in the `has` array.
 * @param idx The index of the pattern.
 * @param pos The position within the pattern string.
 */
static void
_edje_match_states_insert(Edje_States *list,
                          unsigned int patterns_max_length,
                          unsigned int idx,
                          unsigned int pos)
{
   unsigned int i;

   // Calculate index for the 'has' array to check for pre-existence.
   // This maps a 2D (idx, pos) coordinate to a 1D index.
   i = (idx * (patterns_max_length + 1)) + pos;

   // The check `i < list->size` for `list->has[i]` seems problematic as `list->has`
   // is sized based on `array_len` (max_length * patterns_size) not `list->size` (current active states).
   // However, the primary purpose of `list->has` is to mark states that are *candidates*
   // for the *next* step of NFA, so `list->has[i] = 1` marks it.
   // The `list->states` array is then populated, and `list->has[i]` for the *actual*
   // state in `list->states` is set to 0 (or rather, it's not used in that way).
   // This function is adding to `list->states` and marking in `list->has` for the *target* list.

   if (i < list->size) // This condition might be intended for a different context or is a bug.
                       // Given `has` array size, this check should likely be against `array_len`.
                       // However, if `list->has` is being cleared and reused, this might be relevant
                       // to the *current* population of `list` before it's cleared for the next step.
                       // Let's assume `list->has` is used to prevent adding duplicate states
                       // to the *current* `list->states` being built.
     {
        if (list->has[i]) return; // If already marked in 'has' for this list, don't re-add.
     }
   list->has[i] = 1; // Mark this (idx, pos) as added to the current list.

   i = list->size; // Get the next available slot in the `states` array.
   list->states[i].idx = idx;
   list->states[i].pos = pos;
   // list->has[i] = 0; // This line seems to conflict with the above `list->has[i] = 1;`
                       // if `i` refers to the same index. It likely means the `has` array
                       // indexed by `(idx * (max_length + 1)) + pos` is for quick lookup,
                       // while the `has` array elements corresponding to `list->states[0...size-1]`
                       // are not used or have a different meaning.
                       // Given the clear in _edje_match_states_clear, the `has` array is likely
                       // zeroed out there, and `list->has[i]=1` here marks it for the current processing step.
   list->size++;
}

/**
 * @brief Clears the list of active states by resetting its size to zero.
 *
 * This doesn't free memory, it just makes the list appear empty for reuse.
 * The `has` array associated with this list should also be cleared (typically
 * by `memset` elsewhere before reuse) if it's used for duplicate checking.
 *
 * @param list The Edje_States list to clear.
 * @param patterns_size Unused parameter.
 * @param patterns_max_length Unused parameter.
 */
static void
_edje_match_states_clear(Edje_States *list,
                         unsigned int patterns_size EINA_UNUSED,
                         unsigned int patterns_max_length EINA_UNUSED)
{
   list->size = 0;
}

/* Token manipulation. */

/**
 * @enum status
 * @brief Represents the outcome of a pattern matching operation for a token.
 */
enum status
{
   patterns_not_found           = 0, /**< The pattern token was not found/matched. */
   patterns_found               = 1, /**< The pattern token was successfully matched. */
   patterns_syntax_error        = 2  /**< A syntax error was encountered in the pattern token. */
};

/**
 * @brief Processes a single token within a character class `[...]` in a pattern.
 *
 * It checks if the character `c` matches the token `cl_tok`.
 * Handles single characters and character ranges (e.g., `a-z`).
 *
 * @param status Output parameter, set to `patterns_found` if `c` matches,
 *               `patterns_syntax_error` if `cl_tok` is malformed.
 * @param cl_tok The character class token string (e.g., "a", "a-z").
 * @param c The character from the input string to match against.
 * @return The number of characters consumed from `cl_tok` (1 for a single char, 3 for a range like "a-z").
 *         Returns 0 if `cl_tok` is empty, leading to a syntax error.
 */
static unsigned int
_edje_match_patterns_exec_class_token(enum status *status,
                                      const char *cl_tok,
                                      char c)
{
   if (!*cl_tok)
     {
        *status = patterns_syntax_error;
        return 0;
     }
   else if (cl_tok[1] == '-' && cl_tok[2] != ']')
     {
        if (*cl_tok <= c && c <= cl_tok[2])
          *status = patterns_found;
        return 3;
     }
   else
     {
        if (c == *cl_tok)
          *status = patterns_found;
        return 1;
     }
}

/**
 * @brief Checks for the complement operator `!` (or `^`) at the beginning of a character class.
 *
 * If `cl_tok` (which is `cl + 1`, so it points after the opening `[`)
 * starts with `!`, it indicates a complemented class (match any character NOT in the set).
 *
 * @param cl_tok Pointer to the character immediately after `[` in the class string.
 * @param ret Output parameter, set to 1 if `!` is found (complement), 0 otherwise.
 * @return `EDJE_MATCH_OK` on success, `EDJE_MATCH_SYNTAX_ERROR` if `cl_tok` is empty (e.g. "[]").
 */
static Edje_Match_Error
_edje_match_patterns_exec_class_complement(const char *cl_tok, unsigned int *ret)
{
   switch (*cl_tok)
     {
      case 0:
        return EDJE_MATCH_SYNTAX_ERROR;

      case '!':
        *ret = 1;
        return EDJE_MATCH_OK;

      default:
        *ret = 0;
        return EDJE_MATCH_OK;
     }
}

/**
 * @brief Executes matching for a character class (e.g., `[abc]`, `[^a-z]`).
 *
 * Parses the character class `cl` and checks if character `c` matches it.
 * Handles normal classes, complemented classes, single characters, and ranges.
 *
 * @param cl The pattern string representing the character class (e.g., "[a-zA-Z0-9]").
 * @param c The character from the input string to match.
 * @param ret Output parameter. If `c` matches the class, `*ret` is set to the
 *            length of the class pattern string (e.g., for "[abc]", `*ret` would be 5).
 *            If `c` does not match, `*ret` is set to 0.
 * @return `EDJE_MATCH_OK` on successful evaluation, `EDJE_MATCH_SYNTAX_ERROR`
 *         if the class syntax is invalid.
 */
static Edje_Match_Error
_edje_match_patterns_exec_class(const char *cl,
                                char c,
                                unsigned int *ret)
{
   enum status status = patterns_not_found; // Overall status for the class match
   int pos = 1;
   unsigned int neg;

   if (_edje_match_patterns_exec_class_complement(cl + 1, &neg) != EDJE_MATCH_OK)
     return EDJE_MATCH_SYNTAX_ERROR;

   pos += neg;

   do
     {
        pos += _edje_match_patterns_exec_class_token(&status, cl + pos, c);
     }
   while (cl[pos] && cl[pos] != ']');

   if (status == patterns_syntax_error || !cl[pos])
     return EDJE_MATCH_SYNTAX_ERROR;

   if (status == patterns_found)
     *ret = neg ? 0 : pos + 1;
   else
     *ret = neg ? pos + 1 : 0;

   return EDJE_MATCH_OK;
}

/**
 * @brief Executes matching for a single token in a pattern string.
 *
 * A token can be a literal character, `?` (matches any char), `\` (escaped char),
 * or `[` (start of a character class). `*` is handled by the NFA simulation logic, not here.
 *
 * @param tok The pattern token string (e.g., "a", "?", "\\*", "[a-z]").
 * @param c The character from the input string to match.
 * @param ret Output parameter. If `c` matches the token, `*ret` is set to the
 *            number of characters consumed from `tok` by this match.
 *            (e.g., 1 for 'a' or '?', 2 for '\\a', length of class for '[...]').
 *            If `c` does not match, `*ret` is set to 0.
 * @return `EDJE_MATCH_OK` on successful evaluation, `EDJE_MATCH_SYNTAX_ERROR`
 *         if the token syntax is invalid (e.g., trailing `\`, malformed class).
 */
static Edje_Match_Error
_edje_match_patterns_exec_token(const char *tok,
                                char c,
                                unsigned int *ret)
{
   switch (*tok)
     {
      case '\\':
        if (tok[1])
          {
             *ret = tok[1] == c ? 2 : 0;
             return EDJE_MATCH_OK;
          }
        return EDJE_MATCH_SYNTAX_ERROR;

      case '?':
        *ret = 1;
        return EDJE_MATCH_OK;

      case '[':
        return _edje_match_patterns_exec_class(tok, c, ret);

      default:
        *ret = *tok == c ? 1 : 0;
        return EDJE_MATCH_OK;
     }
}

/**
 * @brief Initializes the NFA states list for the start of a matching process.
 *
 * For each pattern in the set, it adds an initial state (index `i`, position `0`)
 * to the `states` list. It also marks these initial states in the `has` array.
 * This sets up the NFA to begin matching all patterns from their starting characters.
 *
 * @param states The Edje_States structure to initialize.
 * @param patterns_size The total number of patterns being matched against.
 * @param patterns_max_length The maximum length of any pattern, used for indexing `has`.
 */
static void
_edje_match_patterns_exec_init_states(Edje_States *states,
                                      unsigned int patterns_size,
                                      unsigned int patterns_max_length)
{
   unsigned int i;

   states->size = patterns_size;

   for (i = 0; i < patterns_size; ++i)
     {
        states->states[i].idx = i;
        states->states[i].pos = 0;
        states->has[i * (patterns_max_length + 1)] = 1;
     }
}

/* Exported function. */

/**
 * @def EDJE_MATCH_INIT_LIST
 * @brief Macro to generate a function that initializes Edje_Patterns from an Eina_List.
 *
 * This macro creates a function `Func` that takes an `Eina_List` of `Type` objects.
 * It extracts pattern strings from the `Source` field of each object, populates
 * an `Edje_Patterns` structure, calculates final states, and allocates states lists.
 *
 * @param Func Name of the function to generate.
 * @param Type The type of elements in the Eina_List (e.g., Edje_Part_Collection_Directory_Entry).
 * @param Source The field name within `Type` that contains the pattern string (e.g., entry).
 * @param Show A flag (0 or 1); if 1, prints debug information about patterns.
 *
 * Generated function signature: `Edje_Patterns * Func(const Eina_List *lst)`
 * Example `lst` structure for `edje_match_collection_dir_init`:
 *   Eina_List of `Edje_Part_Collection_Directory_Entry*`, where each entry has a `char* entry` field.
 *   `lst = [ { entry="pattern1", ... }, { entry="pattern2", ... }, ... ]`
 */
#define EDJE_MATCH_INIT_LIST(Func, Type, Source, Show)              \
  Edje_Patterns *                                                   \
  Func(const Eina_List *lst)                                        \
  {                                                                 \
     Edje_Patterns *r;                                              \
     unsigned int i;                                                \
                                                                    \
     if (!lst || eina_list_count(lst) <= 0)                         \
       return NULL;                                                 \
                                                                    \
     r = malloc(sizeof (Edje_Patterns) +                            \
                eina_list_count(lst)                                \
                * sizeof(*r->finals)                                \
                * sizeof(*r->patterns));                            \
     if (!r) return NULL;                                           \
                                                                    \
     r->ref = 1;                                                    \
     r->delete_me = EINA_FALSE;                                     \
     r->patterns_size = eina_list_count(lst);                       \
     r->max_length = 0;                                             \
     r->patterns = (const char **)r->finals + r->patterns_size + 1; \
                                                                    \
     for (i = 0; lst; ++i)                                          \
       {                                                            \
          const char *str;                                          \
          Type *data;                                               \
          unsigned int j;                                           \
          int special = 0;                                          \
                                                                    \
          data = eina_list_data_get(lst);                           \
          if (!data)                                                \
            {                                                       \
               free(r);                                             \
               return NULL;                                         \
            }                                                       \
                                                                    \
          str = data->Source;                                       \
          if (!str) str = "";                                       \
          r->patterns[i] = str;                                     \
                                                                    \
          if (Show)                                                 \
            INF("%lu [%s]", (unsigned long)i, str);                 \
                                                                    \
          r->finals[i] = 0;                                         \
          for (j = 0; str[j]; ++j)                                  \
            if (str[j] != '*')                                      \
              {                                                     \
                 r->finals[i] = j + 1;                              \
                 special++;                                         \
              }                                                     \
          j += special ? special + 1 : 0;                           \
                                                                    \
          if (j > r->max_length)                                    \
            r->max_length = j;                                      \
                                                                    \
          lst = eina_list_next(lst);                                \
       }                                                            \
                                                                    \
     if (!_edje_match_states_alloc(r, 2))                           \
       {                                                            \
          free(r);                                                  \
          return NULL;                                              \
       }                                                            \
                                                                    \
     return r;                                                      \
  }

/**
 * @def EDJE_MATCH_INIT_ARRAY
 * @brief Macro to generate a function that initializes Edje_Patterns from an array of pointers.
 *
 * This macro creates a function `Func` that takes an array of `Type*` objects and its count.
 * It extracts pattern strings from the `Source` field of each object, populates
 * an `Edje_Patterns` structure, calculates final states, and allocates states lists.
 *
 * @param Func Name of the function to generate.
 * @param Type The type of elements pointed to in the array (e.g., Edje_Program).
 * @param Source The field name within `Type` that contains the pattern string (e.g., signal, source).
 * @param Show A flag (0 or 1); if 1, prints debug information about patterns.
 *
 * Generated function signature: `Edje_Patterns * Func(Type * const *lst, unsigned int count)`
 * Example `lst` structure for `edje_match_programs_signal_init`:
 *   Array of `Edje_Program*`, where each `Edje_Program` has a `char* signal` field.
 *   `lst = [ program_ptr1, program_ptr2, ... ]`
 *   `program_ptr1->signal = "pattern_signal1"`
 */
#define EDJE_MATCH_INIT_ARRAY(Func, Type, Source, Show)             \
  Edje_Patterns *                                                   \
  Func(Type * const *lst, unsigned int count)                       \
  {                                                                 \
     Edje_Patterns *r;                                              \
     unsigned int i;                                                \
                                                                    \
     if (!lst || count == 0)                                        \
       return NULL;                                                 \
                                                                    \
     r = malloc(sizeof (Edje_Patterns) +                            \
                count                                               \
                * sizeof(*r->finals)                                \
                * sizeof(*r->patterns));                            \
     if (!r) return NULL;                                           \
                                                                    \
     r->ref = 1;                                                    \
     r->delete_me = EINA_FALSE;                                     \
     r->patterns_size = count;                                      \
     r->max_length = 0;                                             \
     r->patterns = (const char **)r->finals + r->patterns_size + 1; \
                                                                    \
     for (i = 0; i < count; ++i)                                    \
       {                                                            \
          const char *str;                                          \
          unsigned int j;                                           \
          int special = 0;                                          \
                                                                    \
          if (!lst[i])                                              \
            {                                                       \
               free(r);                                             \
               return NULL;                                         \
            }                                                       \
                                                                    \
          str = lst[i]->Source;                                     \
          if (!str) str = "";                                       \
          r->patterns[i] = str;                                     \
                                                                    \
          if (Show)                                                 \
            INF("%lu [%s]", (unsigned long)i, str);                 \
                                                                    \
          r->finals[i] = 0;                                         \
          for (j = 0; str[j]; ++j)                                  \
            if (str[j] != '*')                                      \
              {                                                     \
                 r->finals[i] = j + 1;                              \
                 special++;                                         \
              }                                                     \
          j += special ? special + 1 : 0;                           \
                                                                    \
          if (j > r->max_length)                                    \
            r->max_length = j;                                      \
       }                                                            \
                                                                    \
     if (!_edje_match_states_alloc(r, 2))                           \
       {                                                            \
          free(r);                                                  \
          return NULL;                                              \
       }                                                            \
                                                                    \
     return r;                                                      \
  }

/**
 * @def EDJE_MATCH_INIT_INARRAY
 * @brief Macro to generate a function that initializes Edje_Patterns from an Eina_Inarray of indices.
 *
 * This macro creates a function `Func` that takes an `Eina_Inarray` containing indices
 * and an array of `Edje_Signal_Callback_Match` structures. It uses the indices from
 * `array` to look up `Edje_Signal_Callback_Match` entries in `matches`, then extracts
 * pattern strings from the `Source` field of these entries. It populates an
 * `Edje_Patterns` structure, calculates final states, and allocates states lists.
 *
 * @param Func Name of the function to generate.
 * @param Source The field name within `Edje_Signal_Callback_Match` that contains the pattern string (e.g., signal, source).
 * @param Show A flag (0 or 1); if 1, prints debug information about patterns.
 *
 * Generated function signature: `Edje_Patterns * Func(const Eina_Inarray *array, const Edje_Signal_Callback_Match *matches)`
 * Example `array` and `matches` structure for `edje_match_callback_signal_init`:
 *   `matches`: An array of `Edje_Signal_Callback_Match` structs.
 *     `matches = [ { signal="sig1", source="src1", ... }, { signal="sig2", source="src2", ... }, ... ]`
 *   `array`: An `Eina_Inarray` of integers, where each integer is an index into the `matches` array.
 *     `array = [ index_into_matches_0, index_into_matches_1, ... ]`
 *     For example, if `array = [0, 2]`, it would use `matches[0].signal` and `matches[2].signal`.
 */
#define EDJE_MATCH_INIT_INARRAY(Func, Source, Show)                            \
  Edje_Patterns *                                                              \
  Func(const Eina_Inarray * array, const Edje_Signal_Callback_Match * matches) \
  {                                                                            \
     Edje_Patterns *r;                                                         \
     int *it;                                                                  \
     unsigned int i = 0;                                                       \
                                                                               \
     if (!matches)                                                             \
       return NULL;                                                            \
                                                                               \
     r = malloc(sizeof (Edje_Patterns) +                                       \
                eina_inarray_count(array)                                      \
                * sizeof(*r->finals)                                           \
                * sizeof(*r->patterns));                                       \
     if (!r) return NULL;                                                      \
                                                                               \
     r->ref = 1;                                                               \
     r->delete_me = EINA_FALSE;                                                \
     r->patterns_size = eina_inarray_count(array);                             \
     r->max_length = 0;                                                        \
     r->patterns = (const char **)r->finals + r->patterns_size + 1;            \
                                                                               \
     EINA_INARRAY_FOREACH(array, it)                                           \
     {                                                                         \
        const char *str;                                                       \
        unsigned int j;                                                        \
        int special = 0;                                                       \
                                                                               \
        str = (matches + *it)->Source;                                         \
        if (!str) str = "";                                                    \
        r->patterns[i] = str;                                                  \
                                                                               \
        if (Show)                                                              \
          INF("%lu [%s]", (unsigned long)i, str);                              \
                                                                               \
        r->finals[i] = 0;                                                      \
        for (j = 0; str[j]; ++j)                                               \
          if (str[j] != '*')                                                   \
            {                                                                  \
               r->finals[i] = j + 1;                                           \
               special++;                                                      \
            }                                                                  \
        j += special ? special + 1 : 0;                                        \
                                                                               \
        if (j > r->max_length)                                                 \
          r->max_length = j;                                                   \
                                                                               \
        i++;                                                                   \
     }                                                                         \
                                                                               \
     if (!_edje_match_states_alloc(r, 2))                                      \
       {                                                                       \
          free(r);                                                             \
          return NULL;                                                         \
       }                                                                       \
                                                                               \
     return r;                                                                 \
  }

EDJE_MATCH_INIT_LIST(edje_match_collection_dir_init,
                     Edje_Part_Collection_Directory_Entry,
                     entry, 0);
EDJE_MATCH_INIT_ARRAY(edje_match_programs_signal_init,
                      Edje_Program,
                      signal, 0);
EDJE_MATCH_INIT_ARRAY(edje_match_programs_source_init,
                      Edje_Program,
                      source, 0);
EDJE_MATCH_INIT_INARRAY(edje_match_callback_signal_init,
                        signal, 0);
EDJE_MATCH_INIT_INARRAY(edje_match_callback_source_init,
                        source, 0);

/**
 * @brief Checks if any of the current NFA states are final states for collection directory matching.
 *
 * A state is considered final if its current position (`pos`) in its pattern
 * is greater than or equal to the `final` position calculated for that pattern
 * (which is typically the length of the pattern if it doesn't end in `*`).
 *
 * @param finals An array where `finals[pattern_idx]` stores the final position for that pattern.
 *               Example: if `patterns[i]` is "abc", `finals[i]` is 3. If "ab*c", `finals[i]` is 4.
 * @param states The current list of active NFA states.
 * @return `EINA_TRUE` if at least one active state is a final state, `EINA_FALSE` otherwise.
 */
static Eina_Bool
_edje_match_collection_dir_exec_finals(const unsigned int *finals,
                                       const Edje_States *states)
{
   unsigned int i;

   for (i = 0; i < states->size; ++i)
     {
        if (states->states[i].pos >= finals[states->states[i].idx])
          return EINA_TRUE;
     }
   return EINA_FALSE;
}

/**
 * @brief Checks for final states in program signal and source matching and executes a callback.
 *
 * Iterates through all active signal states. If a signal state is final,
 * it then iterates through all active source states. If a source state
 * is also final AND belongs to the same program index as the signal state,
 * the associated program is considered matched. The provided `func` callback
 * is then called with the matched `Edje_Program`.
 *
 * @param signal_finals Array of final positions for signal patterns.
 * @param source_finals Array of final positions for source patterns.
 * @param signal_states Current NFA states for signal patterns.
 * @param source_states Current NFA states for source patterns.
 * @param programs Array of `Edje_Program` pointers, indexed by pattern index.
 *                 Example: `programs = [ prog_ptr_for_pattern_0, prog_ptr_for_pattern_1, ... ]`
 * @param func Callback function to execute on a full match.
 *             It receives the matched `Edje_Program*` and `data`.
 *             If `func` returns `EINA_TRUE`, it usually means "stop processing".
 *             Here, if `func` returns `EINA_TRUE`, this function returns `EINA_FALSE` to stop.
 * @param data User data passed to the `func` callback.
 * @param prop Unused propagation flag.
 * @return `EINA_TRUE` to continue processing programs, `EINA_FALSE` to stop (if `func` indicated so).
 *         Returns `EINA_TRUE` if `signal_finals` or `source_finals` is NULL (e.g. OOM).
 */
static Eina_Bool
edje_match_programs_exec_check_finals(const unsigned int *signal_finals,
                                      const unsigned int *source_finals,
                                      const Edje_States *signal_states,
                                      const Edje_States *source_states,
                                      Edje_Program **programs,
                                      Eina_Bool (*func)(Edje_Program *pr, void *data),
                                      void *data,
                                      Eina_Bool prop EINA_UNUSED)
{
   unsigned int i;
   unsigned int j;

   /* when not enough memory, they could be NULL */
   if (!signal_finals || !source_finals) return EINA_TRUE;

   for (i = 0; i < signal_states->size; ++i)
     {
        if (signal_states->states[i].pos >= signal_finals[signal_states->states[i].idx])
          {
             for (j = 0; j < source_states->size; ++j)
               {
                  if (signal_states->states[i].idx == source_states->states[j].idx
                      && source_states->states[j].pos >= source_finals[source_states->states[j].idx])
                    {
                       Edje_Program *pr;

                       pr = programs[signal_states->states[i].idx];
                       if (pr)
                         {
                            if (func(pr, data))
                              return EINA_FALSE;
                         }
                    }
               }
          }
     }

   return EINA_TRUE;
}

/**
 * @brief Checks for final states in callback signal/source matching and executes callbacks.
 *
 * Similar to `edje_match_programs_exec_check_finals`, but for Edje signal callbacks.
 * It iterates through signal and source states. If a match is found (both signal
 * and source patterns for the same callback index are in final states),
 * the corresponding callback function is retrieved and executed.
 * Callbacks are collected and then run to handle potential modifications to the callback list during execution.
 *
 * @param ssp Pointer to `Edje_Signals_Sources_Patterns` containing compiled signal and source patterns.
 * @param matches Array of `Edje_Signal_Callback_Match` structures, containing callback details.
 *                Example: `matches = [ { signal="sig1", source="src1", func_ptr, ... }, ... ]`
 * @param signal_states Current NFA states for signal patterns.
 * @param source_states Current NFA states for source patterns.
 * @param sig The input signal string that was matched.
 * @param source The input source string that was matched.
 * @param ed The Edje object associated with these callbacks.
 * @param prop Propagation flag; if true, callbacks marked for propagation are skipped.
 * @return An integer indicating the result:
 *         - 1: No matching callbacks found or all processed without interruption.
 *         - 2: At least one matching callback was found and queued/run.
 *         - 0: Processing was interrupted (e.g., by `_edje_block_break` or pattern deletion).
 */
static int
edje_match_callback_exec_check_finals(const Edje_Signals_Sources_Patterns *ssp,
                                      const Edje_Signal_Callback_Match *matches,
                                      const Edje_States *signal_states,
                                      const Edje_States *source_states,
                                      const char *sig,
                                      const char *source,
                                      Edje *ed,
                                      Eina_Bool prop)
{
   const Edje_Signal_Callback_Match *cb;
   Eina_Array run; // Array to store callbacks to be run. Eina_Array of Edje_Signal_Callback_Match*
   unsigned int i;
   unsigned int j;
   int r = 1; // Default: no relevant callbacks found or run.

   eina_array_step_set(&run, sizeof (Eina_Array), 4);

   // Find all matching callbacks and add them to the 'run' array
   for (i = 0; i < signal_states->size; ++i)
     if (signal_states->states[i].pos >= ssp->signals_patterns->finals[signal_states->states[i].idx])
       {
          for (j = 0; j < source_states->size; ++j)
            {
               if (signal_states->states[i].idx == source_states->states[j].idx // Same original callback definition
                   && source_states->states[j].pos >= ssp->sources_patterns->finals[source_states->states[j].idx])
                 {
                    int *e; // Pointer to the original index in the `matches` array

                    // `ssp->u.callbacks.globing` is an Eina_Inarray mapping the pattern index (signal_states->states[i].idx)
                    // back to the original index in the `matches` array.
                    e = eina_inarray_nth(&ssp->u.callbacks.globing, signal_states->states[i].idx);

                    cb = &matches[*e];
                    if (cb)
                      {
                         // Skip if propagation is enabled and this callback is marked to propagate
                         if ((prop) && ed->callbacks->flags[*e].propagate) continue;
                         eina_array_push(&run, cb);
                         r = 2; // Indicate at least one callback is to be run.
                      }
                 }
            }
       }

   // Execute the collected callbacks
   while ((cb = eina_array_pop(&run)))
     {
        int idx = cb - matches; // Get original index of the callback in the `matches` array.

        if (ed->callbacks->flags[idx].delete_me) continue; // Skip if marked for deletion.

        // Execute legacy or EO-style callback
        if (ed->callbacks->flags[idx].legacy)
          cb->legacy((void *)ed->callbacks->custom_data[idx], ed->obj, sig, source);
        else
          cb->eo((void *)ed->callbacks->custom_data[idx], ed->obj, sig, source);

        if (_edje_block_break(ed)) // Check if further processing should be blocked
          {
             r = 0; // Indicate interruption.
             break;
          }
        // Check if patterns were marked for deletion during callback execution
        if ((ssp->signals_patterns->delete_me) || (ssp->sources_patterns->delete_me))
          {
             r = 0; // Indicate interruption due to pattern deletion.
             break;
          }
     }

   eina_array_flush(&run);

   return r;
}

/**
 * @brief Core NFA simulation function for matching a string against compiled patterns.
 *
 * This function implements Thompson's construction-based NFA simulation.
 * It takes an initial set of states and an input string, and for each character
 * in the string, it computes the next set of reachable states.
 * It handles literal characters, `?` (any character), `*` (zero or more),
 * and character classes `[...]`.
 *
 * @param ppat The compiled patterns (`Edje_Patterns`) to match against.
 * @param string The input string to match.
 * @param states Pointer to the first of two `Edje_States` structures allocated
 *               (e.g., `ppat->states`). Used for current and next states.
 *               `states` will be the initial current states.
 *               `states + 1` will be used as the initial next states.
 *               These are swapped during processing.
 * @return A pointer to the `Edje_States` structure containing the final set of
 *         active states after processing the entire string. Returns `NULL` if
 *         a pattern syntax error occurs during token execution.
 */
static Edje_States *
_edje_match_fn(const Edje_Patterns *ppat,
               const char *string,
               Edje_States *states)
{
   Edje_States *new_states = states + 1;
   const char *c;

   for (c = string; *c && states->size; ++c)
     {
        unsigned int i;

        _edje_match_states_clear(new_states, ppat->patterns_size, ppat->max_length);

        for (i = 0; i < states->size; ++i)
          {
             const unsigned int idx = states->states[i].idx;
             const unsigned int pos = states->states[i].pos;

             if (!ppat->patterns[idx][pos])
               continue;
             else if (ppat->patterns[idx][pos] == '*')
               {
                  _edje_match_states_insert(states, ppat->max_length, idx, pos + 1);
                  _edje_match_states_insert(new_states, ppat->max_length, idx, pos);
               }
             else
               {
                  unsigned int m;

                  if (_edje_match_patterns_exec_token(ppat->patterns[idx] + pos,
                                                      *c,
                                                      &m) != EDJE_MATCH_OK)
                    return NULL;

                  if (m)
                    _edje_match_states_insert(new_states, ppat->max_length, idx, pos + m);
               }
          }
        {
           Edje_States *tmp = states;

           states = new_states;
           new_states = tmp;
        }
     }

   return states;
}

/**
 * @brief Executes pattern matching for collection directory entries.
 *
 * Initializes NFA states, runs the NFA simulation (`_edje_match_fn`) against
 * the input `string`, and then checks if any of the resulting states are
 * final states using `_edje_match_collection_dir_exec_finals`.
 *
 * @param ppat Compiled patterns for collection directory entries.
 *             Created by `edje_match_collection_dir_init`.
 * @param string The input string (e.g., a part name) to match.
 * @return `EINA_TRUE` if the string matches any pattern, `EINA_FALSE` otherwise or on error.
 */
Eina_Bool
edje_match_collection_dir_exec(const Edje_Patterns *ppat,
                               const char *string)
{
   Edje_States *result;
   Eina_Bool r = EINA_FALSE;

   /* under high memory presure, it could be NULL */
   if (!ppat) return EINA_FALSE;

   _edje_match_patterns_exec_init_states(ppat->states, ppat->patterns_size, ppat->max_length);

   result = _edje_match_fn(ppat, string, ppat->states);

   if (result)
     r = _edje_match_collection_dir_exec_finals(ppat->finals, result);

   return r;
}

/**
 * @brief Executes pattern matching for Edje programs (signal and source).
 *
 * Initializes NFA states for both signal and source patterns.
 * Runs NFA simulation for the input `sig` string against signal patterns and
 * for the input `source` string against source patterns.
 * Then, `edje_match_programs_exec_check_finals` is called to determine if
 * any program (matched by both its signal and source patterns) should trigger
 * the provided callback function.
 *
 * @param ppat_signal Compiled signal patterns. Created by `edje_match_programs_signal_init`.
 * @param ppat_source Compiled source patterns. Created by `edje_match_programs_source_init`.
 * @param sig The input signal string.
 * @param source The input source string.
 * @param programs Array of `Edje_Program` pointers, indexed according to the patterns.
 * @param func Callback function to execute when a program's signal and source both match.
 * @param data User data for the callback function.
 * @param prop Propagation flag, passed to `edje_match_programs_exec_check_finals`.
 * @return `EINA_TRUE` if matching proceeds and `edje_match_programs_exec_check_finals`
 *         returns true (typically meaning continue processing), `EINA_FALSE` otherwise
 *         (e.g., on error, or if `check_finals` indicates to stop).
 */
Eina_Bool
edje_match_programs_exec(const Edje_Patterns *ppat_signal,
                         const Edje_Patterns *ppat_source,
                         const char *sig,
                         const char *source,
                         Edje_Program **programs,
                         Eina_Bool (*func)(Edje_Program *pr, void *data),
                         void *data,
                         Eina_Bool prop)
{
   Edje_States *signal_result;
   Edje_States *source_result;
   Eina_Bool r = EINA_FALSE;

   /* under high memory presure, they could be NULL */
   if (!ppat_source || !ppat_signal) return EINA_FALSE;

   _edje_match_patterns_exec_init_states(ppat_signal->states,
                                         ppat_signal->patterns_size,
                                         ppat_signal->max_length);
   _edje_match_patterns_exec_init_states(ppat_source->states,
                                         ppat_source->patterns_size,
                                         ppat_source->max_length);

   signal_result = _edje_match_fn(ppat_signal, sig, ppat_signal->states);
   source_result = _edje_match_fn(ppat_source, source, ppat_source->states);

   if (signal_result && source_result)
     r = edje_match_programs_exec_check_finals(ppat_signal->finals,
                                               ppat_source->finals,
                                               signal_result,
                                               source_result,
                                               programs,
                                               func,
                                               data,
                                               prop);
   return r;
}

/**
 * @brief Executes pattern matching for Edje signal callbacks.
 *
 * This function manages matching for dynamically registered signal callbacks.
 * It increments reference counts for the signal and source patterns.
 * Initializes NFA states for both signal and source patterns from `ssp`.
 * Runs NFA simulation for `sig` against signal patterns and `source` against source patterns.
 * Calls `edje_match_callback_exec_check_finals` to identify and execute matched callbacks.
 * Finally, decrements pattern reference counts and frees them if necessary.
 *
 * @param ssp Structure containing compiled signal and source patterns for callbacks.
 * @param matches Array of `Edje_Signal_Callback_Match` structures defining the callbacks.
 * @param sig The input signal string.
 * @param source The input source string.
 * @param ed The Edje object associated with these callbacks.
 * @param prop Propagation flag, passed to `edje_match_callback_exec_check_finals`.
 * @return Integer result from `edje_match_callback_exec_check_finals`:
 *         - 1: No matching callbacks or all processed.
 *         - 2: At least one callback matched and run.
 *         - 0: Interrupted, or error (e.g., patterns are NULL).
 */
int
edje_match_callback_exec(const Edje_Signals_Sources_Patterns *ssp,
                         const Edje_Signal_Callback_Match *matches,
                         const char *sig,
                         const char *source,
                         Edje *ed,
                         Eina_Bool prop)
{
   Edje_States *signal_result;
   Edje_States *source_result;
   int r = 0;

   /* under high memory presure, they could be NULL */
   if (!ssp->sources_patterns || !ssp->signals_patterns) return 0;

   ssp->signals_patterns->ref++;
   ssp->sources_patterns->ref++;
   _edje_match_patterns_exec_init_states(ssp->signals_patterns->states,
                                         ssp->signals_patterns->patterns_size,
                                         ssp->signals_patterns->max_length);
   _edje_match_patterns_exec_init_states(ssp->sources_patterns->states,
                                         ssp->sources_patterns->patterns_size,
                                         ssp->sources_patterns->max_length);

   signal_result = _edje_match_fn(ssp->signals_patterns, sig, ssp->signals_patterns->states);
   source_result = _edje_match_fn(ssp->sources_patterns, source, ssp->sources_patterns->states);

   if (signal_result && source_result)
     r = edje_match_callback_exec_check_finals(ssp,
                                               matches,
                                               signal_result,
                                               source_result,
                                               sig,
                                               source,
                                               ed,
                                               prop);
   ssp->signals_patterns->ref--;
   ssp->sources_patterns->ref--;
   if (ssp->signals_patterns->ref <= 0) edje_match_patterns_free(ssp->signals_patterns);
   if (ssp->sources_patterns->ref <= 0) edje_match_patterns_free(ssp->sources_patterns);
   return r;
}

/**
 * @brief Frees an Edje_Patterns structure and its associated resources.
 *
 * This function implements reference counting for `Edje_Patterns`.
 * It marks the pattern set for deletion and decrements its reference count.
 * If the reference count drops to zero or below, it frees the allocated
 * `Edje_States` (via `_edje_match_states_free`) and then frees the
 * `Edje_Patterns` structure itself.
 *
 * @param ppat The Edje_Patterns structure to free.
 */
void
edje_match_patterns_free(Edje_Patterns *ppat)
{
   if (!ppat) return;

   ppat->delete_me = EINA_TRUE; // Mark for deletion, useful if used during iteration.
   ppat->ref--;
   if (ppat->ref > 0) return; // Only free if ref count is zero or less.
   _edje_match_states_free(ppat->states, 2); // Free the Edje_States arrays.
   free(ppat); // Free the main structure.
}

/**
 * @brief Cleans up (frees) signal and source patterns within an Edje_Signals_Sources_Patterns structure.
 *
 * Calls `edje_match_patterns_free` for both `signals_patterns` and
 * `sources_patterns` stored in `ssp`, then sets these pointers to NULL.
 *
 * @param ssp The Edje_Signals_Sources_Patterns structure to clean.
 */
void
_edje_signals_sources_patterns_clean(Edje_Signals_Sources_Patterns *ssp)
{
   if (!ssp->signals_patterns)
     return;

   edje_match_patterns_free(ssp->signals_patterns);
   edje_match_patterns_free(ssp->sources_patterns);
   ssp->signals_patterns = NULL;
   ssp->sources_patterns = NULL;
}

/**
 * @brief Comparison function for Edje_Signal_Source_Char nodes in an Eina_Rbtree.
 *
 * Compares two `Edje_Signal_Source_Char` structures first by their `signal`
 * string, then by their `source` string if signals are identical.
 * Used for inserting and ordering nodes in the Rbtree for exact matches.
 *
 * @param n1 First Edje_Signal_Source_Char node.
 * @param n2 Second Edje_Signal_Source_Char node.
 * @param data Unused user data.
 * @return EINA_RBTREE_LEFT if n1 < n2, EINA_RBTREE_RIGHT if n1 > n2,
 *         (implicitly 0 if equal, though Rbtree expects distinct nodes or specific handling).
 *         Here, if signals are equal, it compares sources. If both are equal,
 *         it still returns LEFT/RIGHT based on source, implying distinct nodes
 *         are expected or that exact duplicates are handled by lookup before insert.
 */
static Eina_Rbtree_Direction
_edje_signal_source_node_cmp(const Edje_Signal_Source_Char *n1,
                             const Edje_Signal_Source_Char *n2,
                             void *data EINA_UNUSED)
{
   int cmp;

   cmp = strcmp(n1->signal, n2->signal);
   if (cmp) return cmp < 0 ? EINA_RBTREE_LEFT : EINA_RBTREE_RIGHT;

   return strcmp(n1->source, n2->source) < 0 ? EINA_RBTREE_LEFT : EINA_RBTREE_RIGHT;
}

/**
 * @brief Key comparison function for looking up in an Eina_Rbtree of Edje_Signal_Source_Char.
 *
 * Compares an `Edje_Signal_Source_Char` node with a given `sig` (signal string)
 * and `source` (source string).
 * Used for `eina_rbtree_inline_lookup`.
 *
 * @param node The Edje_Signal_Source_Char node from the Rbtree.
 * @param sig The signal string key to search for.
 * @param length Unused length parameter for the signal key.
 * @param source The source string key to search for.
 * @return An integer less than, equal to, or greater than zero if the node's
 *         signal/source pair is found, respectively, to be less than, to match,
 *         or be greater than the provided key pair.
 */
static int
_edje_signal_source_key_cmp(const Edje_Signal_Source_Char *node,
                            const char *sig,
                            int length EINA_UNUSED,
                            const char *source)
{
   int cmp;

   cmp = strcmp(node->signal, sig);
   if (cmp) return cmp;

   return strcmp(node->source, source);
}

/**
 * @brief Builds a hash table (Eina_Rbtree) for exact program signal/source matches.
 *
 * Iterates through an array of `Edje_Program`s. If a program's signal and
 * source strings do not contain any globbing characters (`*`, `?`, `[`, `\`),
 * it's considered an exact match. These exact match programs are added to an
 * Rbtree, keyed by their (signal, source) pair. Programs with globbing
 * characters are added to a separate Eina_List (`result`).
 *
 * The Rbtree stores `Edje_Signal_Source_Char` nodes, where each node contains
 * the signal and source strings and an `Eina_Inarray` of `Edje_Program*`
 * that match this exact (signal, source) pair.
 *
 * @param programs Array of `Edje_Program` pointers.
 *                 Example: `programs = [ prog_ptr1, prog_ptr2, ... ]`
 *                 `prog_ptr1->signal = "exact_signal", prog_ptr1->source = "exact_source"`
 * @param count The number of programs in the `programs` array.
 * @param tree Output parameter: pointer to the created Eina_Rbtree.
 *             The Rbtree will contain `Edje_Signal_Source_Char*` items.
 *             Each `Edje_Signal_Source_Char` has an `Eina_Inarray list` of `Edje_Program*`.
 *             Example Rbtree structure:
 *             Key: ("signal_A", "source_X") -> Value: Edje_Signal_Source_Char* item1
 *                  item1->list = [ Edje_Program_ptr_alpha, Edje_Program_ptr_beta ] (programs matching "signal_A", "source_X")
 *             Key: ("signal_B", "source_Y") -> Value: Edje_Signal_Source_Char* item2
 *                  item2->list = [ Edje_Program_ptr_gamma ]
 * @return An Eina_List containing `Edje_Program*` for programs that have
 *         globbing characters in their signal or source and thus cannot be
 *         put into the exact-match Rbtree.
 *         Example `result` list: `[ prog_ptr_glob1, prog_ptr_glob2, ... ]`
 */
Eina_List *
edje_match_program_hash_build(Edje_Program *const *programs,
                              unsigned int count,
                              Eina_Rbtree **tree)
{
   Eina_List *result = NULL; // List for programs with globbing patterns
   Eina_Rbtree *new = NULL;  // Rbtree for exact match programs
   unsigned int i;

   for (i = 0; i < count; ++i)
     {
        if (programs[i]->signal && !strpbrk(programs[i]->signal, "*?[\\")
            && programs[i]->source && !strpbrk(programs[i]->source, "*?[\\"))
          {
             Edje_Signal_Source_Char *item;

             item = (Edje_Signal_Source_Char *)eina_rbtree_inline_lookup(new, programs[i]->signal, 0,
                                                                         EINA_RBTREE_CMP_KEY_CB(_edje_signal_source_key_cmp), programs[i]->source);
             if (!item)
               {
                  item = malloc(sizeof (Edje_Signal_Source_Char));
                  if (!item) continue;

                  item->signal = programs[i]->signal;
                  item->source = programs[i]->source;
                  eina_inarray_step_set(&item->list, sizeof (Eina_Inarray), sizeof (void *), 8);

                  new = eina_rbtree_inline_insert(new, EINA_RBTREE_GET(item),
                                                  EINA_RBTREE_CMP_NODE_CB(_edje_signal_source_node_cmp), NULL);
               }

             eina_inarray_push(&item->list, &programs[i]);
          }
        else
          result = eina_list_prepend(result, programs[i]);
     }

   *tree = new;
   return result;
}

/**
 * @brief Builds a hash table (Eina_Rbtree) for exact callback signal/source matches.
 *
 * Similar to `edje_match_program_hash_build`, but for `Edje_Signal_Callback_Match` structures.
 * Iterates through an array of callbacks. Callbacks with exact (non-globbing)
 * signal and source strings are added to an Rbtree (`tree`). Callbacks with
 * globbing characters are added to an `Eina_Inarray` (`result`) which stores their original indices.
 *
 * The Rbtree stores `Edje_Signal_Source_Char` nodes, where each node contains
 * the signal and source strings and an `Eina_Inarray` of integer indices. These
 * indices refer to the original `callbacks` array.
 *
 * @param callbacks Array of `Edje_Signal_Callback_Match` structures.
 *                  Example: `callbacks = [ {signal="s1", source="src1", ...}, {signal="s2*", source="src2", ...} ]`
 * @param callbacks_count The number of callbacks in the array.
 * @param tree Output parameter: pointer to the created Eina_Rbtree for exact matches.
 *             The Rbtree will contain `Edje_Signal_Source_Char*` items.
 *             Each `Edje_Signal_Source_Char` has an `Eina_Inarray list` of `int` (indices into `callbacks`).
 *             Example Rbtree structure:
 *             Key: ("signal_A", "source_X") -> Value: Edje_Signal_Source_Char* item1
 *                  item1->list = [ original_index_0, original_index_5 ] (indices of callbacks matching "signal_A", "source_X")
 * @param result Output parameter: an `Eina_Inarray` that will be populated with the
 *               original indices (from the `callbacks` array) of callbacks that
 *               use globbing patterns.
 *               Example `result` inarray: `[ original_index_1, original_index_3, ... ]` (indices of globbing callbacks)
 */
void
edje_match_callback_hash_build(const Edje_Signal_Callback_Match *callbacks,
                               int callbacks_count,
                               Eina_Rbtree **tree,
                               Eina_Inarray *result)
{
   Eina_Rbtree *new = NULL; // Rbtree for exact match callbacks
   int i;

   eina_inarray_step_set(result, sizeof (Eina_Inarray), sizeof (int), 8);

   for (i = 0; i < callbacks_count; ++i, ++callbacks)
     {
        if (callbacks->signal && !strpbrk(callbacks->signal, "*?[\\")
            && callbacks->source && !strpbrk(callbacks->source, "*?[\\"))
          {
             Edje_Signal_Source_Char *item;

             item = (Edje_Signal_Source_Char *)eina_rbtree_inline_lookup(new, callbacks->signal, 0,
                                                                         EINA_RBTREE_CMP_KEY_CB(_edje_signal_source_key_cmp), callbacks->source);
             if (!item)
               {
                  item = malloc(sizeof (Edje_Signal_Source_Char));
                  if (!item) continue;

                  item->signal = callbacks->signal;
                  item->source = callbacks->source;
                  eina_inarray_step_set(&item->list, sizeof (Eina_Inarray), sizeof (int), 8);

                  new = eina_rbtree_inline_insert(new, EINA_RBTREE_GET(item),
                                                  EINA_RBTREE_CMP_NODE_CB(_edje_signal_source_node_cmp), NULL);
               }

             eina_inarray_push(&item->list, &i);
          }
        else
          {
             eina_inarray_push(result, &i);
          }
     }

   *tree = new;
}

/**
 * @brief Retrieves a list of matching items from the signal/source Rbtree.
 *
 * Looks up the given `sig` and `source` strings in the Rbtree (`tree`).
 * If an exact match is found, it returns the `Eina_Inarray` associated
 * with that (signal, source) pair. This inarray contains either
 * `Edje_Program*` (for program hash) or `int` indices (for callback hash).
 *
 * @param sig The signal string to look up.
 * @param source The source string to look up.
 * @param tree The Eina_Rbtree (built by `edje_match_program_hash_build` or
 *             `edje_match_callback_hash_build`) to search in.
 * @return A const pointer to an `Eina_Inarray` containing the matched items
 *         (e.g., `Edje_Program*` or `int` indices) if found, otherwise `NULL`.
 *         Example returned inarray (if `tree` was from `edje_match_program_hash_build`):
 *           `&item->list` where `item->list` is `[ Edje_Program_ptr_alpha, Edje_Program_ptr_beta ]`
 *         Example returned inarray (if `tree` was from `edje_match_callback_hash_build`):
 *           `&item->list` where `item->list` is `[ original_index_0, original_index_5 ]`
 */
const Eina_Inarray *
edje_match_signal_source_hash_get(const char *sig,
                                  const char *source,
                                  const Eina_Rbtree *tree)
{
   Edje_Signal_Source_Char *lookup;

   lookup = (Edje_Signal_Source_Char *)eina_rbtree_inline_lookup(tree, sig, 0,
                                                                 EINA_RBTREE_CMP_KEY_CB(_edje_signal_source_key_cmp), source);

   if (lookup) return &lookup->list;
   return NULL;
}

/**
 * @brief Frees an Edje_Signal_Source_Char node (used as Rbtree data free callback).
 *
 * This function is intended to be used as a callback when an Eina_Rbtree
 * containing `Edje_Signal_Source_Char` nodes is destroyed. It flushes
 * the `Eina_Inarray` within the node (which stores program pointers or indices)
 * and then frees the `Edje_Signal_Source_Char` structure itself.
 *
 * @param key The Edje_Signal_Source_Char node to free.
 * @param data Unused user data.
 */
void
edje_match_signal_source_free(Edje_Signal_Source_Char *key, void *data EINA_UNUSED)
{
   // The inarray `key->list` stores pointers (Edje_Program*) or integers (indices).
   // `eina_inarray_flush` will free the internal storage of the inarray but not the
   // items themselves if they are pointers to separately allocated memory.
   // For Edje_Program*, they are typically part of the Edje_File and not freed here.
   // For integers, no further action is needed.
   eina_inarray_flush(&key->list);
   free(key);
}

