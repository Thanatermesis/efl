#include "evas_common_private.h"
#include "evas_private.h"

/**
 * @internal
 * @brief Creates and populates the callback descriptions array for a smart object.
 * This function iterates through the smart class hierarchy (including parent classes),
 * collects all callback descriptions, sorts them, and removes duplicates.
 * The resulting sorted and unique callback descriptions are stored in s->callbacks.
 * @param s The smart object to process.
 */
static void _evas_smart_class_callbacks_create(Evas_Smart *s);

/**
 * @internal
 * @brief Creates and populates the interface array for a smart object.
 * This function iterates through the smart class hierarchy (including parent classes)
 * and collects all registered interfaces, storing them in s->interfaces.
 * @param s The smart object to process.
 */
static void _evas_smart_class_interfaces_create(Evas_Smart *s);

/* all public */

/**
 * @brief Frees an Evas_Smart instance.
 * @param s The Evas_Smart instance to free.
 *
 * Marks the smart object for deletion. If its usage count is zero,
 * it proceeds to free all associated memory, including the smart class
 * definition (if allocated by evas_smart_class_new()), callback descriptions,
 * interface descriptions, and the Evas_Smart structure itself.
 */
EVAS_API void
evas_smart_free(Evas_Smart *s)
{
   MAGIC_CHECK(s, Evas_Smart, MAGIC_SMART);
   return;
   MAGIC_CHECK_END();
   s->delete_me = 1;
   if (s->usage > 0) return;
   if (s->class_allocated) free((void *)s->smart_class);
   free(s->callbacks.array);
   free(s->interfaces.array);

   free(s);
}

/**
 * @brief Creates a new Evas_Smart instance based on a smart class definition.
 * @param sc Pointer to the constant Evas_Smart_Class definition.
 *        This structure defines the behavior (methods, callbacks, etc.) of the smart object.
 * @return A pointer to the newly created Evas_Smart instance, or NULL on failure.
 *         Failures can occur if sc is NULL, if memory allocation fails, or if
 *         the version of the provided Evas_Smart_Class (sc->version) does not
 *         match EVAS_SMART_CLASS_VERSION.
 *
 * The created Evas_Smart instance will hold a reference to the provided 'sc'
 * and will initialize its internal callback and interface structures based on 'sc'.
 * If 'sc' was dynamically allocated and needs to be freed when the Evas_Smart
 * instance is freed, this is typically handled by the caller or by custom logic
 * within the smart object's 'del' method, not directly by evas_smart_free unless
 * it was allocated by a specific Evas function that sets s->class_allocated.
 * For Evas_Smart_Class structures provided by evas_smart_class_new (which is this function),
 * the s->class_allocated flag is NOT set, meaning the 'sc' passed here is considered
 * static or managed externally.
 *
 * If you are creating a new Evas_Smart_Class dynamically (e.g. for language bindings),
 * you might need to allocate it, then pass it here. In such cases, ensure it's freed
 * appropriately. However, the common case is to use statically defined Evas_Smart_Class
 * structures.
 */
EVAS_API Evas_Smart *
evas_smart_class_new(const Evas_Smart_Class *sc)
{
   Evas_Smart *s;

   if (!sc) return NULL;

   /* api does not match abi! for now refuse as we only have 1 version */
   if (sc->version != EVAS_SMART_CLASS_VERSION) return NULL;

   s = calloc(1, sizeof(Evas_Smart));
   if (!s) return NULL;

   s->magic = MAGIC_SMART;

   s->smart_class = sc;
   _evas_smart_class_callbacks_create(s);
   _evas_smart_class_interfaces_create(s);

   return s;
}

/**
 * @brief Retrieves the Evas_Smart_Class definition associated with an Evas_Smart instance.
 * @param s The Evas_Smart instance.
 * @return A pointer to the constant Evas_Smart_Class definition, or NULL if s is invalid.
 */
EVAS_API const Evas_Smart_Class *
evas_smart_class_get(const Evas_Smart *s)
{
   MAGIC_CHECK(s, Evas_Smart, MAGIC_SMART);
   return NULL;
   MAGIC_CHECK_END();
   return s->smart_class;
}

/**
 * @brief Retrieves the custom data associated with an Evas_Smart_Class.
 * @param s The Evas_Smart instance.
 * @return A pointer to the custom data (void *), or NULL if s is invalid or no data is set.
 *         The 'data' field is part of the Evas_Smart_Class structure.
 */
EVAS_API void *
evas_smart_data_get(const Evas_Smart *s)
{
   MAGIC_CHECK(s, Evas_Smart, MAGIC_SMART);
   return NULL;
   MAGIC_CHECK_END();
   return (void *)s->smart_class->data;
}

/**
 * @brief Retrieves the array of callback descriptions for an Evas_Smart instance.
 * @param s The Evas_Smart instance.
 * @param[out] count Pointer to an unsigned int where the number of callback descriptions will be stored.
 * @return A pointer to a NULL-terminated array of const Evas_Smart_Cb_Description pointers,
 *         or NULL if s is invalid or has no callbacks.
 *         The caller should not free the returned array or its contents.
 *
 * @par Example of the returned array structure:
 * If a smart object has two callbacks, "mouse_down" and "focused":
 * Evas_Smart_Cb_Description desc1 = { .name = "mouse_down", .type = "void *" };
 * Evas_Smart_Cb_Description desc2 = { .name = "focused", .type = "void" };
 * (These descriptions would typically be part of the Evas_Smart_Class definition)
 *
 * The returned array (s->callbacks.array) would look like:
 * [ &desc1, &desc2, NULL ]
 * And *count would be set to 2.
 */
EVAS_API const Evas_Smart_Cb_Description **
evas_smart_callbacks_descriptions_get(const Evas_Smart *s, unsigned int *count)
{
   MAGIC_CHECK(s, Evas_Smart, MAGIC_SMART);
   if (count) *count = 0;
   return NULL;
   MAGIC_CHECK_END();

   if (count) *count = s->callbacks.size;
   return s->callbacks.array;
}

/**
 * @brief Finds a specific callback description by name within an Evas_Smart instance.
 * @param s The Evas_Smart instance.
 * @param name The name of the callback description to find (e.g., "mouse_down").
 * @return A pointer to the const Evas_Smart_Cb_Description if found, otherwise NULL.
 *         The caller should not free the returned structure.
 */
EVAS_API const Evas_Smart_Cb_Description *
evas_smart_callback_description_find(const Evas_Smart *s, const char *name)
{
   if (!name) return NULL;
   MAGIC_CHECK(s, Evas_Smart, MAGIC_SMART);
   return NULL;
   MAGIC_CHECK_END();
   return evas_smart_cb_description_find(&s->callbacks, name);
}

/**
 * @brief Initializes a smart class ('sc') by inheriting members from a parent smart class ('parent_sc').
 * @param sc The child Evas_Smart_Class structure to be initialized.
 * @param parent_sc The parent Evas_Smart_Class structure to inherit from.
 * @param parent_sc_size The full size of the parent_sc structure. This is used to copy
 *        any private data members that might exist in a derived smart class structure
 *        beyond the base Evas_Smart_Class members.
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., parent_sc version mismatch).
 *
 * This function copies standard smart class function pointers (add, del, move, etc.)
 * from parent_sc to sc. It also sets sc->parent to parent_sc.
 * If parent_sc_size is greater than sizeof(Evas_Smart_Class), it means the parent_sc
 * is likely a custom structure that embeds Evas_Smart_Class and has additional data.
 * This additional data is also copied.
 */
EVAS_API Eina_Bool
evas_smart_class_inherit_full(Evas_Smart_Class *sc, const Evas_Smart_Class *parent_sc, unsigned int parent_sc_size)
{
   unsigned int off;

   /* api does not match abi! for now refuse as we only have 1 version */
   if (parent_sc->version != EVAS_SMART_CLASS_VERSION) return EINA_FALSE;

#define _CP(m) sc->m = parent_sc->m
   _CP(add);
   _CP(del);
   _CP(move);
   _CP(resize);
   _CP(show);
   _CP(hide);
   _CP(color_set);
   _CP(clip_set);
   _CP(clip_unset);
   _CP(calculate);
   _CP(member_add);
   _CP(member_del);
#undef _CP

   sc->parent = parent_sc;

   off = sizeof(Evas_Smart_Class);
   if (parent_sc_size == off) return EINA_TRUE;

   memcpy(((char *)sc) + off, ((char *)parent_sc) + off, parent_sc_size - off);
   return EINA_TRUE;
}

/**
 * @brief Retrieves the current usage count of an Evas_Smart instance.
 * @param s The Evas_Smart instance.
 * @return The usage count, or 0 if s is invalid.
 *
 * The usage count tracks how many Evas_Object instances are currently using this
 * Evas_Smart definition. It's incremented by evas_object_smart_use() and
 * decremented by evas_object_smart_unuse().
 */
EVAS_API int
evas_smart_usage_get(const Evas_Smart *s)
{
   MAGIC_CHECK(s, Evas_Smart, MAGIC_SMART);
   return 0;
   MAGIC_CHECK_END();
   return s->usage;
}


/* internal funcs */

/**
 * @internal
 * @brief Increments the usage count of an Evas_Smart instance.
 * @param s The Evas_Smart instance.
 * Called when an Evas_Object starts using this smart class.
 */
void
evas_object_smart_use(Evas_Smart *s)
{
   s->usage++;
}

/**
 * @internal
 * @brief Decrements the usage count of an Evas_Smart instance.
 * @param s The Evas_Smart instance.
 * If the usage count drops to zero and the smart class is marked for deletion
 * (s->delete_me is true), this function will call evas_smart_free(s) to
 * release its resources.
 */
void
evas_object_smart_unuse(Evas_Smart *s)
{
   s->usage--;
   if ((s->usage <= 0) && (s->delete_me)) evas_smart_free(s);
}

/**
 * @internal
 * @brief Resizes the array of callback descriptions.
 * @param a Pointer to the Evas_Smart_Cb_Description_Array to resize.
 *          The array `a->array` stores pointers to `Evas_Smart_Cb_Description`.
 *          Example: `a->array = { &desc1, &desc2, NULL }`
 * @param size The new number of elements (Evas_Smart_Cb_Description *) the array should hold.
 *             If size is 0, the array `a->array` is freed and set to NULL.
 * @return EINA_TRUE on success or if size is unchanged. EINA_FALSE on memory allocation failure.
 *
 * After a successful resize to a non-zero size, `a->array` will be NULL-terminated,
 * i.e., `a->array[size]` will be `NULL`.
 */
Eina_Bool
evas_smart_cb_descriptions_resize(Evas_Smart_Cb_Description_Array *a, unsigned int size)
{
   void *tmp;

   if (size == a->size)
     return EINA_TRUE;

   if (size == EINA_FALSE)
     {
        free(a->array);
        a->array = NULL;
        a->size = 0;
        return EINA_TRUE;
     }

   tmp = realloc(a->array, (size + 1) * sizeof(Evas_Smart_Cb_Description *));
   if (tmp)
     {
        a->array = tmp;
        a->size = size;
        a->array[size] = NULL;
        return EINA_TRUE;
     }
   else
     {
        ERR("realloc failed!");
        return EINA_FALSE;
     }
}

/**
 * @internal
 * @brief Comparison function for qsort to sort Evas_Smart_Cb_Description pointers by name.
 * @param p1 Pointer to the first const Evas_Smart_Cb_Description* element.
 * @param p2 Pointer to the second const Evas_Smart_Cb_Description* element.
 * @return An integer less than, equal to, or greater than zero if the name of the
 *         first description is found, respectively, to be less than, to match, or be
 *         greater than the name of the second description.
 */
static int
_evas_smart_cb_description_cmp_sort(const void *p1, const void *p2)
{
   const Evas_Smart_Cb_Description **a = (const Evas_Smart_Cb_Description **)p1;
   const Evas_Smart_Cb_Description **b = (const Evas_Smart_Cb_Description **)p2;
   return strcmp((*a)->name, (*b)->name);
}

/**
 * @internal
 * @brief Sorts and de-duplicates an array of smart callback descriptions.
 * @param a Pointer to the Evas_Smart_Cb_Description_Array to process.
 *          The array `a->array` is modified in place.
 *          Example before: `a->array = { &desc_b, &desc_a, &desc_b_dup, NULL }`
 *          Example after: `a->array = { &desc_a, &desc_b, NULL }` (assuming types match for desc_b_dup)
 *
 * This function first sorts the callback descriptions by name. Then, it iterates
 * through the sorted array to remove duplicates.
 * - If two descriptions have the same name but different types, an error is logged,
 *   and the first one encountered (in sort order) is kept.
 * - If two descriptions have the same name and type, a warning is logged about
 *   the duplication, and only one is kept.
 * Finally, the array is resized to fit the unique descriptions.
 */
void
evas_smart_cb_descriptions_fix(Evas_Smart_Cb_Description_Array *a)
{
   unsigned int i, j;

   if (!a)
     {
        ERR("no array to fix!");
        return;
     }

   qsort(a->array, a->size, sizeof(Evas_Smart_Cb_Description *),
         _evas_smart_cb_description_cmp_sort);

   DBG("%u callbacks", a->size);
   if (a->size)
     DBG("%s [type=%s]", a->array[0]->name, a->array[0]->type);

   for (i = 0, j = 1; j < a->size; j++)
     {
        const Evas_Smart_Cb_Description *cur, *prev;

        cur = a->array[j];
        prev = a->array[i];

        DBG("%s [type=%s]", cur->name, cur->type);

        if (strcmp(cur->name, prev->name) != 0)
          {
             i++;
             if (i != j)
               a->array[i] = a->array[j];
          }
        else
          {
             if (strcmp(cur->type, prev->type) == 0)
               WRN("duplicated smart callback description"
                   " with name '%s' and type '%s'", cur->name, cur->type);
             else
               ERR("callback descriptions named '%s' differ"
                   " in type, keeping '%s', ignoring '%s'",
                   cur->name, prev->type, cur->type);
          }
     }

   evas_smart_cb_descriptions_resize(a, i + 1);
}

static void
_evas_smart_class_callbacks_create(Evas_Smart *s)
{
   const Evas_Smart_Class *sc;
   unsigned int n = 0;

   for (sc = s->smart_class; sc; sc = sc->parent)
     {
        const Evas_Smart_Cb_Description *d;
        for (d = sc->callbacks; d && d->name; d++)
          n++;
     }

   if (n == 0) return;
   if (!evas_smart_cb_descriptions_resize(&s->callbacks, n)) return;
   s->callbacks.size = n;
   for (n = 0, sc = s->smart_class; sc; sc = sc->parent)
     {
        const Evas_Smart_Cb_Description *d;
        for (d = sc->callbacks; d && d->name; d++)
          s->callbacks.array[n++] = d;
     }
   evas_smart_cb_descriptions_fix(&s->callbacks);
}

static void
_evas_smart_class_interfaces_create(Evas_Smart *s)
{
   unsigned int i;
   const Evas_Smart_Class *sc;

   /* get number of interfaces on the smart */
   for (i = 0, sc = s->smart_class; sc; sc = sc->parent)
     {
        const Evas_Smart_Interface **ifaces_array = sc->interfaces;
        if (!ifaces_array) continue;

        while (*ifaces_array)
          {
             const Evas_Smart_Interface *iface = *ifaces_array;

             if (!iface->name) break;

             i++;

             if (iface->private_size > 0)
               {
                  unsigned int size = iface->private_size;

                  if (size % sizeof(void *) != 0)
                    size += sizeof(void *) - (size % sizeof(void *));
               }

             ifaces_array++;
          }
     }

   if (!i) return;

   s->interfaces.array = malloc(i * sizeof(Evas_Smart_Interface *));
   if (!s->interfaces.array)
     {
        ERR("malloc failed!");
        return;
     }

   s->interfaces.size = i;

   for (i = 0, sc = s->smart_class; sc; sc = sc->parent)
     {
        const Evas_Smart_Interface **ifaces_array = sc->interfaces;
        if (!ifaces_array) continue;

        while (*ifaces_array)
          {
             const Evas_Smart_Interface *iface = *ifaces_array;

             if (!iface->name) break;

             s->interfaces.array[i++] = iface;
             ifaces_array++;
          }
     }
}

/**
 * @internal
 * @brief Comparison function for bsearch to find an Evas_Smart_Cb_Description by name.
 * @param p1 Pointer to the key (const char *name) being searched for.
 * @param p2 Pointer to an element in the array (const Evas_Smart_Cb_Description **).
 * @return An integer less than, equal to, or greater than zero if the key (name)
 *         is found, respectively, to be less than, to match, or be greater than
 *         the name of the description pointed to by p2.
 *         Includes an optimization for direct pointer comparison if name strings are shared.
 */
static int
_evas_smart_cb_description_cmp_search(const void *p1, const void *p2)
{
   const char *name = p1;
   const Evas_Smart_Cb_Description **v = (const Evas_Smart_Cb_Description **)p2;
   /* speed up string shares searches (same pointers) */
   if (name == (*v)->name) return 0;
   return strcmp(name, (*v)->name);
}

/**
 * @internal
 * @brief Finds a callback description by name in a pre-sorted array of callback descriptions.
 * @param a Pointer to the const Evas_Smart_Cb_Description_Array (assumed to be sorted by name).
 *          The array `a->array` stores pointers to `Evas_Smart_Cb_Description`.
 *          Example: `a->array = { &desc_a, &desc_b, &desc_c, NULL }`
 * @param name The name of the callback description to find.
 * @return A pointer to the const Evas_Smart_Cb_Description if found, otherwise NULL.
 *
 * This function uses `bsearch` for efficient searching.
 */
const Evas_Smart_Cb_Description *
evas_smart_cb_description_find(const Evas_Smart_Cb_Description_Array *a, const char *name)
{
   const Evas_Smart_Cb_Description **found = NULL;

   if (!a->array) return NULL;
   found = bsearch(name, a->array, a->size, sizeof(Evas_Smart_Cb_Description *),
                   _evas_smart_cb_description_cmp_search);

   return found ? (*found) : NULL;
}
