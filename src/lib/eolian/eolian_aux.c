#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "Eolian_Aux.h"

/**
 * @internal
 * @brief Free function for a hash that has Eina_List as value.
 *
 * @param ptr The Eina_List to free.
 */
static void
_hashlist_free(void *ptr)
{
   eina_list_free((Eina_List *)ptr);
}

/**
 * @brief Finds all direct children of all classes in the state.
 *
 * This creates a map from a parent class to a list of its direct children
 * classes. A class is considered a child if it inherits from or extends
 * the parent class.
 *
 * @param[in] state The Eolian state.
 *
 * @return A hash where keys are 'const Eolian_Class *' of the parent and
 *         values are 'Eina_List *' of 'const Eolian_Class *' of the children.
 *         The caller is responsible for freeing the hash.
 */
EOLIAN_API Eina_Hash *
eolian_aux_state_class_children_find(const Eolian_State *state)
{
   if (!state)
      return NULL;

   Eina_Hash *h = eina_hash_pointer_new(_hashlist_free);
   Eina_Iterator *itr = eolian_state_classes_get(state);
   if (!itr)
      return h;

   const Eolian_Class *cl;
   EINA_ITERATOR_FOREACH(itr, cl)
     {
        const Eolian_Class *icl = eolian_class_parent_get(cl);
        if (icl)
          eina_hash_set(h, &icl, eina_list_append(eina_hash_find(h, &icl), cl));
        Eina_Iterator *iitr = eolian_class_extensions_get(cl);
        EINA_ITERATOR_FOREACH(iitr, icl)
          {
             eina_hash_set(h, &icl,
               eina_list_append(eina_hash_find(h, &icl), cl));
          }
        eina_iterator_free(iitr);
     }
   eina_iterator_free(itr);

   return h;
}

/**
 * @internal
 * @brief Finds all functions and events in the given class.
 *
 * This function gets all implements and events from a class and appends them
 * to the given lists. It checks against the 'written' hash to avoid adding
 * duplicate functions that have been overridden in child classes.
 *
 * @param[in] pcl The class to search in.
 * @param[in,out] funcs The list to append found functions (implements) to.
 * @param[in,out] events The list to append found events to.
 * @param[in] written A hash of functions that have already been found.
 *
 * @return The total number of callables found in this class.
 */
static size_t
_callables_find_body(const Eolian_Class *pcl,
                     Eina_List **funcs, Eina_List **events,
                     Eina_Hash *written)
{
   Eina_Iterator *iitr;
   const Eolian_Implement *imp;
   const Eolian_Event *ev;
   size_t total = 0;

   if (!funcs)
     goto justevs;

   iitr = eolian_class_implements_get(pcl);
   EINA_ITERATOR_FOREACH(iitr, imp)
     {
        const Eolian_Function *ifn =
          eolian_implement_function_get(imp, NULL);
        if (eina_hash_find(written, &ifn))
          continue;
        *funcs = eina_list_append(*funcs, imp);
        ++total;
     }
   eina_iterator_free(iitr);

   if (!events)
     return total;
justevs:
   iitr = eolian_class_events_get(pcl);
   EINA_ITERATOR_FOREACH(iitr, ev)
     {
        /* events do not override */
        *events = eina_list_append(*events, ev);
        ++total;
     }
   eina_iterator_free(iitr);
   return total;
}

/**
 * @internal
 * @brief Recursively finds all callables in parent and extended classes.
 *
 * This function traverses the inheritance tree upwards from the given class,
 * collecting all functions and events. It uses _callables_find_body to
 * process each parent class.
 *
 * @param[in] cl The class to start searching from (upwards).
 * @param[in,out] funcs The list to append found functions to.
 * @param[in,out] events The list to append found events to.
 * @param[in] written A hash of functions that have already been found.
 *
 * @return The total number of callables found in parent classes.
 */
static size_t
_callables_find(const Eolian_Class *cl, Eina_List **funcs,
                Eina_List **events, Eina_Hash *written)
{
   size_t total = 0;
   if (!funcs && !events)
     return total;

   const Eolian_Class *pcl = eolian_class_parent_get(cl);
   if (pcl)
     {
        total += _callables_find_body(pcl, funcs, events, written);
        total += _callables_find(pcl, funcs, events, written);
     }

   Eina_Iterator *itr = eolian_class_extensions_get(cl);
   EINA_ITERATOR_FOREACH(itr, pcl)
     {
        total += _callables_find_body(pcl, funcs, events, written);
        total += _callables_find(pcl, funcs, events, written);
     }
   eina_iterator_free(itr);

   return total;
}

/**
 * @brief Gets all callables (functions and events) for a given class,
 * including inherited ones.
 *
 * This function collects all functions (as Eolian_Implement) and events
 * from the specified class and all its parent classes. It can also provide
 * counts of the callables owned directly by the class.
 *
 * @param[in] klass The class to get callables for.
 * @param[out] funcs A pointer to a list where function implements will be
 *                   appended. Can be NULL.
 * @param[out] events A pointer to a list where events will be appended.
 *                    Can be NULL.
 * @param[out] ownfuncs A pointer to store the number of functions owned by
 *                      the class. Can be NULL.
 * @param[out] ownevs A pointer to store the number of events owned by the
 *                    class. Can be NULL.
 *
 * @return The total number of callables found (own and inherited).
 */
EOLIAN_API size_t
eolian_aux_class_callables_get(const Eolian_Class *klass,
                               Eina_List **funcs, Eina_List **events,
                               size_t *ownfuncs, size_t *ownevs)
{
   size_t of = 0, oe = 0, total = 0;
   if (!klass || (!funcs && !events))
     {
        if (ownfuncs) *ownfuncs = of;
        if (ownevs) *ownevs = oe;
        return total;
     }

   Eina_Hash *written = eina_hash_pointer_new(NULL);
   if (funcs)
     {
        const Eolian_Implement *imp;
        Eina_Iterator *itr = eolian_class_implements_get(klass);
        EINA_ITERATOR_FOREACH(itr, imp)
          {
             const Eolian_Function *ifn =
               eolian_implement_function_get(imp, NULL);
             eina_hash_set(written, &ifn, ifn);
             *funcs = eina_list_append(*funcs, imp);
             ++of;
          }
        eina_iterator_free(itr);
     }
   if (events)
     {
        const Eolian_Event *ev;
        Eina_Iterator *itr = eolian_class_events_get(klass);
        EINA_ITERATOR_FOREACH(itr, ev)
          {
             /* no need to mark in written, events do not override */
             *events = eina_list_append(*events, ev);
             ++oe;
          }
     }
   if (ownfuncs) *ownfuncs = of;
   if (ownevs) *ownevs = oe;
   total = of + oe;
   total += _callables_find(klass, funcs, events, written);
   eina_hash_free(written);
   return total;
}

/**
 * @internal
 * @brief Recursively finds all implementations of a function in a class and
 * its descendants.
 *
 * This function traverses the class hierarchy downwards, starting from 'cl',
 * to find all implementations of a specific 'func'.
 *
 * @param[in,out] l The list to append found implementations to.
 * @param[in] cl The class to search in.
 * @param[in] func The function to find implementations of.
 * @param[in,out] got A hash to keep track of visited classes to avoid cycles.
 * @param[in] children A map from parent classes to their children, used for
 *                     traversal.
 */
static void
_all_impls_find(Eina_List **l, const Eolian_Class *cl,
                const Eolian_Function *func, Eina_Hash *got,
                Eina_Hash *children)
{
   if (eina_hash_find(got, &cl))
     return;
   eina_hash_add(got, &cl, cl);
   Eina_Iterator *itr = eolian_class_implements_get(cl);
   const Eolian_Implement *imp;
   EINA_ITERATOR_FOREACH(itr, imp)
     {
        const Eolian_Function *ofn = eolian_implement_function_get(imp, NULL);
        if (ofn == func)
          {
             *l = eina_list_append(*l, imp);
             break;
          }
     }
   eina_iterator_free(itr);
   Eina_List *tl;
   const Eolian_Class *icl;
   EINA_LIST_FOREACH(eina_hash_find(children, &cl), tl, icl)
     _all_impls_find(l, icl, func, got, children);
}

/**
 * @brief Gets a list of all implementations of a given function in the
 * class hierarchy.
 *
 * This function finds all classes that implement the given function,
 * including the class that originally defines it and all its descendants.
 *
 * @param[in] func The function to find implementations for.
 * @param[in] class_children A hash mapping parent classes to their children.
 *                           This can be created with
 *                           eolian_aux_state_class_children_find().
 *
 * @return A list of 'Eolian_Implement *' for the given function.
 *         The caller is responsible for freeing the list.
 */
EOLIAN_API Eina_List *
eolian_aux_function_all_implements_get(const Eolian_Function *func,
                                       Eina_Hash *class_children)
{
   if (!class_children)
     return NULL;

   const Eolian_Class *cl = eolian_implement_class_get(
     eolian_function_implement_get(func));

   Eina_List *ret = NULL;
   Eina_Hash *got = eina_hash_pointer_new(NULL);
   _all_impls_find(&ret, cl, func, got, class_children);

   eina_hash_free(got);
   return ret;
}

static const Eolian_Implement * _parent_impl_find(
   const char *fulln, const Eolian_Class *cl);

static const Eolian_Implement *
_parent_impl_find_body(const Eolian_Class *icl, const char *fulln)
{
   Eina_Iterator *iitr = eolian_class_implements_get(icl);
   const Eolian_Implement *iimpl;
   EINA_ITERATOR_FOREACH(iitr, iimpl)
     {
        if (eolian_implement_name_get(iimpl) == fulln)
          {
             eina_iterator_free(iitr);
             return iimpl;
          }
     }
   eina_iterator_free(iitr);
   return _parent_impl_find(fulln, icl);
 }

/**
 * @internal
 * @brief Recursively finds an implement with a given name in the parent
 * classes.
 *
 * This function searches for an implement by its full name in the inheritance
 * hierarchy of a class, going upwards.
 *
 * @param[in] fulln The full name of the implement to find.
 * @param[in] cl The class from which to start the upward search.
 *
 * @return The found 'Eolian_Implement *' or NULL if not found.
 */
static const Eolian_Implement *
_parent_impl_find(const char *fulln, const Eolian_Class *cl)
{
   const Eolian_Implement *iret = NULL;
   const Eolian_Class *icl = eolian_class_parent_get(cl);
   if (icl)
     iret = _parent_impl_find_body(icl, fulln);
   if (iret)
     return iret;
   Eina_Iterator *itr = eolian_class_extensions_get(cl);
   EINA_ITERATOR_FOREACH(itr, icl)
     {
        iret = _parent_impl_find_body(icl, fulln);
        if (iret)
          {
             eina_iterator_free(itr);
             return iret;
          }
     }
   eina_iterator_free(itr);
   return NULL;
}

/**
 * @brief Gets the parent implement of a given implement.
 *
 * A parent implement is the implement that is being overridden by 'impl'.
 * This is found by searching for an implement with the same name in the
 * parent classes of the implementing class of 'impl'.
 *
 * @param[in] impl The implement to find the parent for.
 *
 * @return The parent 'Eolian_Implement *' or NULL if it doesn't override
 *         anything.
 */
EOLIAN_API const Eolian_Implement *
eolian_aux_implement_parent_get(const Eolian_Implement *impl)
{
   return _parent_impl_find(eolian_implement_name_get(impl),
                            eolian_implement_implementing_class_get(impl));
}

/**
 * @internal
 * @brief Recursively finds documentation in parent implements.
 *
 * This function searches up the inheritance chain for an implement that has
 * documentation for a specific function type.
 *
 * @param[in] impl The implement to start searching from.
 * @param[in] ftype The type of function documentation to look for (get/set).
 *
 * @return The found 'Eolian_Documentation *' or NULL.
 */
static const Eolian_Documentation *
_parent_documentation_find(const Eolian_Implement *impl,
                           Eolian_Function_Type ftype)
{
   const Eolian_Implement *pimpl = eolian_aux_implement_parent_get(impl);
   if (!pimpl)
     return NULL;

   const Eolian_Documentation *pdoc =
     eolian_implement_documentation_get(pimpl, ftype);
   if (!pdoc)
     return _parent_documentation_find(pimpl, ftype);

   return pdoc;
}

/**
 * @brief Gets documentation for an implement, falling back to parent
 * implements if necessary.
 *
 * This function first checks for documentation on the implement itself. If not
 * found, it searches for documentation on parent implements. This is useful
 * for inheriting documentation for overridden methods. For properties that are
 * implemented in a child class but defined in a parent, this allows fetching
 * documentation from the original definition.
 *
 * @param[in] impl The implement to get documentation for.
 * @param[in] ftype The function type (get/set) for which to get documentation.
 *
 * @return The 'Eolian_Documentation *' or NULL if no documentation is found.
 */
EOLIAN_API const Eolian_Documentation *
eolian_aux_implement_documentation_get(const Eolian_Implement *impl,
                                    Eolian_Function_Type ftype)
{
   const Eolian_Documentation *ret =
     eolian_implement_documentation_get(impl, ftype);

   if (ret)
     return ret;

   const Eolian_Class *icl = eolian_implement_implementing_class_get(impl);
   if (eolian_implement_class_get(impl) == icl)
     return NULL;

   const Eolian_Implement *oimp = eolian_function_implement_get(
     eolian_implement_function_get(impl, NULL));
   if ((ftype == EOLIAN_PROP_GET) && !eolian_implement_is_prop_get(oimp))
     return NULL;
   if ((ftype == EOLIAN_PROP_SET) && !eolian_implement_is_prop_set(oimp))
     return NULL;

   return _parent_documentation_find(impl, ftype);
}

/**
 * @brief Gets documentation for a property implement as a fallback.
 *
 * This function is used when documentation for a property is needed, but it's
 * not clear whether to use the get or set documentation. If an implement is
 * only a getter or only a setter, it returns the documentation for that
 * specific part. This is useful for properties where get and set are
 * implemented separately.
 *
 * @param[in] impl The property implement.
 *
 * @return The 'Eolian_Documentation *' if it's exclusively a getter or
 *         setter, otherwise NULL.
 */
EOLIAN_API const Eolian_Documentation *
eolian_aux_implement_documentation_fallback_get(const Eolian_Implement *impl)
{
   Eina_Bool ig = eolian_implement_is_prop_get(impl),
             is = eolian_implement_is_prop_set(impl);

   if (ig && !is)
     return eolian_implement_documentation_get(impl, EOLIAN_PROP_GET);
   else if (is && !ig)
     return eolian_implement_documentation_get(impl, EOLIAN_PROP_SET);

   return NULL;
}
