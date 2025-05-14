#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Elementary.h>
#include "elm_priv.h"

#include "ecore_internal.h"

/**
 * @brief Private data structure for Efl_Ui_View_Model.
 * This structure holds all the internal data necessary for the view model's operation,
 * including bindings, logic blocks, text definitions, and property reference tracking.
 */
typedef struct _Efl_Ui_View_Model_Data Efl_Ui_View_Model_Data;

/**
 * @brief Represents a binding between a source property and one or more destination properties.
 * When the source property changes, all destination properties are notified.
 */
typedef struct _Efl_Ui_View_Model_Bind Efl_Ui_View_Model_Bind;

/**
 * @brief Defines a text property that can be composed from other properties.
 * It allows for dynamic text generation based on the values of other model properties.
 * For example, a definition like "Name: ${name}, Age: ${age}" would combine the 'name' and 'age' properties.
 */
typedef struct _Efl_Ui_View_Model_Text Efl_Ui_View_Model_Text;
/**
 * @brief Defines custom logic for getting and setting a property.
 * This allows for computed properties or properties that require special handling.
 */
typedef struct _Efl_Ui_View_Model_Logic Efl_Ui_View_Model_Logic;

/**
 * @brief Represents a reference-counted property string.
 * Used for deduplicating property strings to save memory and improve lookup efficiency.
 */
typedef struct _Efl_Ui_View_Model_Property_Ref Efl_Ui_View_Model_Property_Ref;

struct _Efl_Ui_View_Model_Data
{
   // FIXME: If parent is set, always access parent... recursively?
   Efl_Ui_View_Model_Data *parent; /**< Pointer to the parent view model's data, if any. */

   Eina_Hash *bound; /**< Hash table storing Efl_Ui_View_Model_Bind structures, keyed by source property name. Maps a source property to its destinations. */
   Eina_Hash *logics; /**< Hash table storing Efl_Ui_View_Model_Logic structures, keyed by property name. Defines custom logic for getting/setting properties. */
   Eina_Hash *texts; /**< Hash table storing Efl_Ui_View_Model_Text structures, keyed by text property name. Defines composite text properties. */

   Eina_Hash *deduplication; /**< Hash table storing Efl_Ui_View_Model_Property_Ref structures, keyed by property name. Used for reference counting property strings. */

   struct {
      Eina_Bool property_changed : 1; /**< Flag to prevent re-entrant emission of property_changed events. */
      Eina_Bool child_added : 1;    /**< Flag to prevent re-entrant emission of child_added events. */
      Eina_Bool child_removed : 1;  /**< Flag to prevent re-entrant emission of child_removed events. */
   } propagating; /**< Boolean flags to prevent reentrance event emission on the same object. */
   Eina_Bool finalized : 1; /**< Flag indicating if the view model has been finalized. */
   Eina_Bool children_bind : 1; /**< Defines if child objects should be automatically bound to a view model proxy. */
};

struct _Efl_Ui_View_Model_Text
{
   Eina_Stringshare *name; /**< The name of the text property. */
   Eina_Stringshare *definition; /**< The format string defining how the text is composed. e.g., "Value: ${property_a}" */
   Eina_Stringshare *not_ready; /**< Optional format string to use if any source property is not ready (e.g., future not resolved). */
   Eina_Stringshare *on_error; /**< Optional format string to use if an error occurs while fetching a source property. */
   Efl_Model *self; /**< A pointer to the Efl_Model instance this text property belongs to. */
};

struct _Efl_Ui_View_Model_Bind
{
   Eina_Stringshare *source; /**< The name of the source property. */
   Eina_List *destinations; /**< A list of Eina_Stringshare representing the names of destination properties. */
};

/**
 * @brief Defines custom logic for getting and setting a property.
 * This allows for computed properties or properties that require special handling.
 */
struct _Efl_Ui_View_Model_Logic
{
   struct {
      EflUiViewModelPropertyGet fct; /**< Function pointer for getting the property value. */
      Eina_Free_Cb free_cb; /**< Callback to free user data associated with the get function. */
      void *data; /**< User data for the get function. */
   } get;
   struct {
      EflUiViewModelPropertySet fct; /**< Function pointer for setting the property value. */
      Eina_Free_Cb free_cb; /**< Callback to free user data associated with the set function. */
      void *data; /**< User data for the set function. */
   } set;

   Efl_Object *object; /**< The Efl_Object this logic block is associated with. */
   Eina_List *sources; /**< List of Eina_Stringshare representing source properties this logic depends on. */
   Eina_Stringshare *property; /**< The name of the property this logic block defines. */
};

/**
 * @brief Represents a reference-counted property string.
 * Used for deduplicating property strings to save memory and improve lookup efficiency.
 */
struct _Efl_Ui_View_Model_Property_Ref
{
   EINA_REFCOUNT; /**< Reference count for the property string. */
   Eina_Stringshare *property; /**< The actual property name string. */
};

/**
 * @brief Frees an Efl_Ui_View_Model_Property_Ref structure.
 * Decrements the stringshare reference and frees the structure itself.
 * @param data Pointer to the Efl_Ui_View_Model_Property_Ref to free.
 */
static void
_ref_free(void *data)
{
   Efl_Ui_View_Model_Property_Ref *r = data;

   eina_stringshare_del(r->property);
   free(r);
}

/**
 * @brief Adds a reference to a property string.
 * If the property string is not already tracked, a new Efl_Ui_View_Model_Property_Ref
 * is created and added to the deduplication hash. Otherwise, the existing reference
 * count is incremented.
 * @param pd The view model's private data.
 * @param property The property string to add a reference to.
 */
static void
_ref_add(Efl_Ui_View_Model_Data *pd, Eina_Stringshare *property)
{
   Efl_Ui_View_Model_Property_Ref *r;

   r = eina_hash_find(pd->deduplication, property);
   if (!r)
     {
        r = calloc(1, sizeof (Efl_Ui_View_Model_Property_Ref));
        if (!r) return ;
        r->property = eina_stringshare_ref(property);

        eina_hash_direct_add(pd->deduplication, r->property, r);
     }

   EINA_REFCOUNT_REF(r);
}

/**
 * @brief Deletes a reference to a property string.
 * Decrements the reference count of the tracked property string. If the reference
 * count reaches zero, the Efl_Ui_View_Model_Property_Ref is removed from the
 * deduplication hash and freed.
 * @param pd The view model's private data.
 * @param property The property string to delete a reference from.
 */
static void
_ref_del(Efl_Ui_View_Model_Data *pd, Eina_Stringshare *property)
{
   Efl_Ui_View_Model_Property_Ref *r;

   r = eina_hash_find(pd->deduplication, property);
   if (!r) return ;

   EINA_REFCOUNT_UNREF(r)
     eina_hash_del(pd->deduplication, property, r);
}

/**
 * @brief Frees an Efl_Ui_View_Model_Logic structure.
 * This includes freeing any associated user data for get/set functions,
 * unbinding source properties, and freeing stringshares.
 * @param data Pointer to the Efl_Ui_View_Model_Logic to free.
 */
static void
_logic_free(void *data)
{
   Efl_Ui_View_Model_Logic *logic = data;
   Eina_Stringshare *source;

   if (logic->get.free_cb) logic->get.free_cb(logic->get.data);
   if (logic->set.free_cb) logic->set.free_cb(logic->set.data);
   EINA_LIST_FREE(logic->sources, source)
     {
        efl_ui_view_model_property_unbind(logic->object, source, logic->property);
        eina_stringshare_del(source);
     }
   eina_stringshare_del(logic->property);
   free(logic);
}

/**
 * @brief Dummy property get function.
 * Used when a logic property is defined without a get function.
 * Returns EFL_MODEL_ERROR_NOT_SUPPORTED.
 */
static Eina_Value *
_efl_ui_view_model_property_dummy_get(void *data EINA_UNUSED,
                                   const Efl_Ui_View_Model *view_model EINA_UNUSED,
                                   Eina_Stringshare *property EINA_UNUSED)
{
   return eina_value_error_new(EFL_MODEL_ERROR_NOT_SUPPORTED);
}

/**
 * @brief Dummy property set function.
 * Used when a logic property is defined without a set function.
 * Returns a future rejected with EFL_MODEL_ERROR_READ_ONLY.
 */
static Eina_Future *
_efl_ui_view_model_property_dummy_set(void *data EINA_UNUSED,
                                   Efl_Ui_View_Model *view_model,
                                   Eina_Stringshare *property EINA_UNUSED,
                                   Eina_Value *value EINA_UNUSED)
{
   return efl_loop_future_rejected(view_model, EFL_MODEL_ERROR_READ_ONLY);
}

/**
 * @brief Adds a logic block for a property.
 * A logic block defines custom functions for getting and setting a property's value.
 * It can also bind to other source properties, causing this logic property to be
 * re-evaluated when any of its sources change.
 * @param obj The Efl_Object (view model) this logic is for.
 * @param pd The view model's private data.
 * @param property The name of the property to add logic for.
 * @param get_data User data for the get function.
 * @param get The get function.
 * @param get_free_cb Callback to free get_data.
 * @param set_data User data for the set function.
 * @param set The set function.
 * @param set_free_cb Callback to free set_data.
 * @param bound An iterator of property names (char *) that this logic property depends on.
 *              Example: An iterator yielding "source_prop1", "source_prop2".
 * @return 0 on success, or an Eina_Error code on failure.
 */
static Eina_Error
_efl_ui_view_model_property_logic_add(Eo *obj, Efl_Ui_View_Model_Data *pd,
                                   const char *property,
                                   void *get_data, EflUiViewModelPropertyGet get, Eina_Free_Cb get_free_cb,
                                   void *set_data, EflUiViewModelPropertySet set, Eina_Free_Cb set_free_cb,
                                   Eina_Iterator *bound)
{
   Efl_Ui_View_Model_Logic *logic;
   Eina_Stringshare *prop;
   const char *source;

   prop = eina_stringshare_add(property);

   if (eina_hash_find(pd->logics, prop))
     {
        eina_stringshare_del(prop);
        return EFL_MODEL_ERROR_INCORRECT_VALUE;
     }

   logic = calloc(1, sizeof (Efl_Ui_View_Model_Logic));
   if (!logic) return ENOMEM;

   logic->object = obj;
   logic->property = prop;
   logic->get.fct = get ? get : _efl_ui_view_model_property_dummy_get;
   logic->get.free_cb = get_free_cb;
   logic->get.data = get_data;
   logic->set.fct = set ? set : _efl_ui_view_model_property_dummy_set;
   logic->set.free_cb = set_free_cb;
   logic->set.data = set_data;

   eina_hash_direct_add(pd->logics, prop, logic);

   EINA_ITERATOR_FOREACH(bound, source)
     {
        logic->sources = eina_list_append(logic->sources, eina_stringshare_add(source));
        efl_ui_view_model_property_bind(obj, source, property);
     }
   eina_iterator_free(bound);

   return 0;
}

/**
 * @brief Deletes a logic block for a property.
 * Removes the custom get/set logic associated with the property.
 * @param obj The Efl_Object (view model) this logic was for (unused).
 * @param pd The view model's private data.
 * @param property The name of the property whose logic to delete.
 * @return 0 on success, or EFL_MODEL_ERROR_INCORRECT_VALUE if the property logic doesn't exist.
 */
static Eina_Error
_efl_ui_view_model_property_logic_del(Eo *obj EINA_UNUSED, Efl_Ui_View_Model_Data *pd,
                                   const char *property)
{
   Efl_Ui_View_Model_Logic *logic;

   logic = eina_hash_find(pd->logics, property);
   if (!logic) return EFL_MODEL_ERROR_INCORRECT_VALUE;
   eina_hash_del(pd->logics, property, logic);
   return 0;
}

/**
 * @brief Parses a definition string to find the next literal text segment and property placeholder.
 * A definition string is composed of literal text and property placeholders like "${property_name}".
 * This function finds the first occurrence of "${" and extracts the text before it
 * and the property name within the braces.
 *
 * Example: For "Hello ${user}, welcome to ${place}!",
 * - First call: *text = "Hello ", *property = "user", returns length of "Hello ${user}".
 * - Second call (on remaining string): *text = ", welcome to ", *property = "place", returns length of ", welcome to ${place}".
 * - Third call: *text = "!", *property = NULL, returns length of "!".
 *
 * @param definition The string to parse.
 * @param[out] text Pointer to store the literal text part (eina_slstr, caller owns).
 * @param[out] property Pointer to store the property name (eina_slstr, caller owns), or NULL if no property placeholder.
 * @return The number of characters consumed from the definition string, or 0 if definition is NULL or empty.
 */
static int
_lookup_next_token(const char *definition,
                   Eina_Slstr **text,
                   Eina_Slstr **property)
{
   const char *lookup_text;
   const char *lookup_property;

   if (!definition) return 0;

   *text = NULL;
   *property = NULL;

   lookup_text = strstr(definition, "${");
   if (!lookup_text) goto on_error;
   lookup_text += 2;

   lookup_property = strchr(lookup_text, '}');
   if (!lookup_property) goto on_error;

   *text = eina_slstr_copy_new_length(definition, lookup_text - definition - 2);
   *property = eina_slstr_copy_new_length(lookup_text, lookup_property - lookup_text);

   return lookup_property + 1 - definition;

 on_error:
   if (strlen(definition) == 0) return 0;
   *text = eina_slstr_copy_new(definition);
   return strlen(definition);
}

/**
 * @brief Adds a string property (composite text) to the view model.
 * A string property is defined by a format string that can include placeholders
 * for other properties (e.g., "Name: ${user.name}"). When this string property
 * is fetched, the placeholders are replaced with the current values of the
 * referenced properties.
 * This function also binds the new string property to all properties mentioned in its
 * definition, not_ready, and on_error strings, so it gets updated if they change.
 *
 * @param obj The Efl_Object (view model) this string property is for.
 * @param pd The view model's private data.
 * @param name The name of the string property to add.
 * @param definition The format string defining the property. Must not be empty.
 *                   Example: "User: ${username}, Status: ${status}"
 * @param not_ready Optional format string to use if a referenced property is not yet available (e.g., future pending).
 *                  Example: "Loading ${username}..."
 * @param on_error Optional format string to use if an error occurs fetching a referenced property.
 *                 Example: "Error fetching ${username}"
 * @return 0 on success, or an Eina_Error code on failure.
 */
static Eina_Error
_efl_ui_view_model_property_string_add(Eo *obj, Efl_Ui_View_Model_Data *pd,
                                    const char *name,
                                    const char *definition,
                                    const char *not_ready,
                                    const char *on_error)
{
   Efl_Ui_View_Model_Text *text;
   Eina_Stringshare *sn;
   Eina_Slstr *st = NULL;
   Eina_Slstr *sp = NULL;
   int lookup;
   Eina_Error err = ENOMEM;

   if (!name || !definition) return EFL_MODEL_ERROR_INCORRECT_VALUE;
   if (!strlen(name)) return EFL_MODEL_ERROR_INCORRECT_VALUE;
   if (!strlen(definition)) return EFL_MODEL_ERROR_INCORRECT_VALUE;
   sn = eina_stringshare_add(name);

   // Lookup if there is an existing property defined and undo it first
   text = eina_hash_find(pd->texts, sn);
   if (text) efl_ui_view_model_property_string_del(obj, sn);

   text = calloc(1, sizeof (Efl_Ui_View_Model_Text));
   if (!text) goto on_error;

   err = EFL_MODEL_ERROR_INCORRECT_VALUE;

   text->name = eina_stringshare_add(name);
   text->definition = eina_stringshare_add(definition);
   text->not_ready = not_ready ? eina_stringshare_add(not_ready) : NULL;
   text->on_error = on_error ? eina_stringshare_add(on_error) : NULL;
   text->self = obj;

   for (lookup = _lookup_next_token(definition, &st, &sp);
        lookup;
        definition += lookup, lookup = _lookup_next_token(definition, &st, &sp))
     {
        if (sp) efl_ui_view_model_property_bind(obj, sp, name);
     }

   for (lookup = _lookup_next_token(not_ready, &st, &sp);
        lookup;
        not_ready += lookup, lookup = _lookup_next_token(not_ready, &st, &sp))
     {
        if (sp) efl_ui_view_model_property_bind(obj, sp, name);
     }

   for (lookup = _lookup_next_token(on_error, &st, &sp);
        lookup;
        on_error += lookup, lookup = _lookup_next_token(on_error, &st, &sp))
     {
        if (sp) efl_ui_view_model_property_bind(obj, sp, name);
     }

   eina_hash_direct_add(pd->texts, text->name, text);

   return 0;

 on_error:
   eina_stringshare_del(sn);
   free(text);
   return err;
}

/**
 * @brief Frees an Efl_Ui_View_Model_Text structure.
 * This includes unbinding any properties that were part of its definition,
 * not_ready, or on_error strings, and freeing stringshares.
 * @param data Pointer to the Efl_Ui_View_Model_Text to free.
 */
static void
_text_free(void *data)
{
   Efl_Ui_View_Model_Text *text = data;
   Eina_Stringshare *st;
   Eina_Stringshare *sp;
   int lookup;
   const char *tmp;

   tmp = text->definition;
   for (lookup = _lookup_next_token(tmp, &st, &sp);
        lookup;
        tmp += lookup, lookup = _lookup_next_token(tmp, &st, &sp))
     {
        if (sp) efl_ui_view_model_property_unbind(text->self, sp, text->name);
     }

   tmp = text->not_ready;
   for (lookup = _lookup_next_token(tmp, &st, &sp);
        lookup;
        tmp += lookup, lookup = _lookup_next_token(tmp, &st, &sp))
     {
        if (sp) efl_ui_view_model_property_unbind(text->self, sp, text->name);
     }

   tmp = text->on_error;
   for (lookup = _lookup_next_token(tmp, &st, &sp);
        lookup;
        tmp += lookup, lookup = _lookup_next_token(tmp, &st, &sp))
     {
        if (sp) efl_ui_view_model_property_unbind(text->self, sp, text->name);
     }

   eina_stringshare_del(text->name);
   eina_stringshare_del(text->not_ready);
   eina_stringshare_del(text->on_error);
   free(text);
}

/**
 * @brief Deletes a string property (composite text) from the view model.
 * Removes the Efl_Ui_View_Model_Text definition and associated bindings.
 * @param obj The Efl_Object (view model) this string property was for (unused).
 * @param pd The view model's private data.
 * @param name The name of the string property to delete.
 * @return 0 on success, or EFL_MODEL_ERROR_INCORRECT_VALUE if the property doesn't exist.
 */
static Eina_Error
_efl_ui_view_model_property_string_del(Eo *obj EINA_UNUSED,
                                    Efl_Ui_View_Model_Data *pd,
                                    const char *name)
{
   Efl_Ui_View_Model_Text *text;
   Eina_Stringshare *sn;
   Eina_Error err = EFL_MODEL_ERROR_INCORRECT_VALUE;

   if (!name) return EFL_MODEL_ERROR_INCORRECT_VALUE;

   sn = eina_stringshare_add(name);
   text = eina_hash_find(pd->texts, sn);
   if (!text) goto on_error;
   eina_hash_del(pd->texts, sn, text);
   err = 0;

 on_error:
   eina_stringshare_del(sn);
   return err;
}

/**
 * @brief Binds a source property to a destination property.
 * When the source property changes, the destination property will be marked as changed.
 * This creates an entry in the `pd->bound` hash, mapping the source property
 * to a list of its destination properties. It also increments the reference count
 * for the destination property string.
 *
 * @param obj The Efl_Object (view model) this binding is for (unused).
 * @param pd The view model's private data.
 * @param source The name of the source property.
 * @param destination The name of the destination property.
 */
static void
_efl_ui_view_model_property_bind(Eo *obj EINA_UNUSED, Efl_Ui_View_Model_Data *pd,
                              const char *source, const char *destination)
{
   Efl_Ui_View_Model_Bind *bind;
   Eina_Stringshare *src;
   Eina_Stringshare *dst;

   if (!source || !destination) return ;

   src = eina_stringshare_add(source);
   bind = eina_hash_find(pd->bound, src);
   if (!bind)
     {
        bind = calloc(1, sizeof (Efl_Ui_View_Model_Bind));
        if (!bind) goto on_error;
        bind->source = eina_stringshare_ref(src);

        eina_hash_direct_add(pd->bound, bind->source, bind);
     }

   dst = eina_stringshare_add(destination);
   bind->destinations = eina_list_append(bind->destinations, dst);
   _ref_add(pd, dst);

 on_error:
   eina_stringshare_del(src);
}

/**
 * @brief Unbinds a destination property from a source property.
 * Removes the destination from the source's list of bound properties.
 * If the source property has no more destinations, its Efl_Ui_View_Model_Bind
 * entry is removed from `pd->bound`. Decrements the reference count for the
 * destination property string.
 *
 * @param obj The Efl_Object (view model) this unbinding is for (unused).
 * @param pd The view model's private data.
 * @param source The name of the source property.
 * @param destination The name of the destination property to unbind.
 */
static void
_efl_ui_view_model_property_unbind(Eo *obj EINA_UNUSED, Efl_Ui_View_Model_Data *pd,
                                const char *source, const char *destination)
{
   Efl_Ui_View_Model_Bind *bind;
   Eina_Stringshare *src;
   Eina_Stringshare *dst;
   Eina_Stringshare *cmp;
   Eina_List *l;

   if (!source || !destination) return ;
   src = eina_stringshare_add(source);
   bind = eina_hash_find(pd->bound, src);
   if (!bind) goto on_error;

   dst = eina_stringshare_add(destination);

   EINA_LIST_FOREACH(bind->destinations, l, cmp)
     if (cmp == dst)
       {
          bind->destinations = eina_list_remove_list(bind->destinations, l);
          break;
       }

   if (!bind->destinations)
     eina_hash_del(pd->bound, dst, bind);

   _ref_del(pd, dst);
   eina_stringshare_del(dst);

 on_error:
   eina_stringshare_del(src);
}

/**
 * @brief Frees an Efl_Ui_View_Model_Bind structure.
 * This is used as a callback for the `pd->bound` hash. It frees the stringshares
 * for the source and all destination properties, and the structure itself.
 * @param data Pointer to the Efl_Ui_View_Model_Bind to free.
 */
static void
_bind_free(void *data)
{
   Efl_Ui_View_Model_Bind *bind = data;
   Eina_Stringshare *dst;

   eina_stringshare_del(bind->source);

   EINA_LIST_FREE(bind->destinations, dst)
     eina_stringshare_del(dst);

   free(bind);
}

/**
 * @brief Recursively looks up all properties affected by a change in a source property.
 * When a property `src` changes, this function finds all properties directly bound
 * to `src`. For each of those, it recursively calls itself to find further
 * dependent properties. All affected properties are added to `changed_properties`.
 * This also traverses up to parent view models.
 *
 * @param changed_properties An Eina_Array to which affected property names (Eina_Stringshare) are added.
 *                           Example (after call): ["prop_a", "prop_b_derived_from_a", "prop_c_derived_from_b"]
 * @param pd The view model's private data.
 * @param src The name of the property that initially changed.
 */
static void
_efl_ui_view_model_property_bind_lookup(Eina_Array *changed_properties,
                                     Efl_Ui_View_Model_Data *pd,
                                     Eina_Stringshare *src)
{
   Efl_Ui_View_Model_Bind *bind;

   if (!pd) return ;
   bind = eina_hash_find(pd->bound, src);
   if (bind)
     {
        Eina_Stringshare *dest;
        Eina_List *l;

        EINA_LIST_FOREACH(bind->destinations, l, dest)
          {
             // Check for duplicated entry first to avoid infinite recursion
             Eina_Stringshare *dup = NULL;
             Eina_Array_Iterator iterator;
             unsigned int i;

             EINA_ARRAY_ITER_NEXT(changed_properties, i, dup, iterator)
               if (dup == dest) break;
             if (dup == dest) continue ;

             eina_array_push(changed_properties, dest);
             _efl_ui_view_model_property_bind_lookup(changed_properties, pd, dest);
          }
     }
   _efl_ui_view_model_property_bind_lookup(changed_properties, pd->parent, src);
}

/**
 * @brief Handles the EFL_MODEL_EVENT_PROPERTIES_CHANGED event from the underlying model.
 * This function intercepts the event, identifies all properties that are transitively
 * affected by the initially changed properties (due to bindings), and then re-emits
 * a new EFL_MODEL_EVENT_PROPERTIES_CHANGED event that includes all affected properties.
 * This ensures that observers of the view model are notified of all relevant changes,
 * even those resulting from indirect dependencies.
 *
 * @param data The view model's private data (Efl_Ui_View_Model_Data *).
 * @param event The original EFL_MODEL_EVENT_PROPERTIES_CHANGED event.
 *              `event->info` is an Efl_Model_Property_Event*.
 *              Example `event->info->changed_properties`: ["original_prop"]
 *              This function might emit a new event with `changed_properties`: ["original_prop", "dependent_prop1", "dependent_prop2"]
 */
static void
_efl_ui_view_model_property_changed(void *data, const Efl_Event *event)
{
   Efl_Ui_View_Model_Data *pd = data;
   Efl_Model_Property_Event *ev = event->info;
   Efl_Model_Property_Event nev = { 0 };
   const char *property;
   Eina_Stringshare *src;
   Eina_Array_Iterator iterator;
   unsigned int i;

   if (pd->propagating.property_changed) return ;
   pd->propagating.property_changed = EINA_TRUE;

   // Our strategy is to rebuild a new Property_Event and cancel the current one.
   efl_event_callback_stop(event->object);

   nev.changed_properties = eina_array_new(1);

   EINA_ARRAY_ITER_NEXT(ev->changed_properties, i, property, iterator)
     {
        eina_array_push(nev.changed_properties, property);

        src = eina_stringshare_ref(property);
        _efl_ui_view_model_property_bind_lookup(nev.changed_properties, pd, src);
     }

   efl_event_callback_call(event->object, EFL_MODEL_EVENT_PROPERTIES_CHANGED, &nev);

   eina_array_free(nev.changed_properties);

   pd->propagating.property_changed = EINA_FALSE;
}

/**
 * @brief Sets whether child models should be automatically wrapped by a view model.
 * If enabled, when a child is added to the underlying model, a corresponding
 * Efl_Ui_View_Model instance will be created for that child and exposed by this view model.
 * This setting can only be changed before the view model is finalized.
 *
 * @param obj The Efl_Object (view model) (unused).
 * @param pd The view model's private data.
 * @param enable EINA_TRUE to enable automatic child binding, EINA_FALSE to disable.
 */
static void
_efl_ui_view_model_children_bind_set(Eo *obj EINA_UNUSED, Efl_Ui_View_Model_Data *pd, Eina_Bool enable)
{
   if (pd->finalized) return;

   pd->children_bind = enable;
}

/**
 * @brief Gets whether child models are automatically wrapped by a view model.
 * @param obj The Efl_Object (view model) (unused).
 * @param pd The view model's private data.
 * @return EINA_TRUE if automatic child binding is enabled, EINA_FALSE otherwise.
 */
static Eina_Bool
_efl_ui_view_model_children_bind_get(const Eo *obj EINA_UNUSED, Efl_Ui_View_Model_Data *pd)
{
   return pd->children_bind;
}

/**
 * @brief Sets up the parent-child relationship between view model data.
 * When a child view model is created, this function links its private data
 * to the parent's private data, particularly for propagating event flags.
 *
 * @param child The newly created child Efl_Ui_View_Model.
 * @param ppd The parent view model's private data.
 */
static void
_efl_ui_view_model_parent_data(Efl_Ui_View_Model *child, Efl_Ui_View_Model_Data *ppd)
{
   Efl_Ui_View_Model_Data *cpd;

   cpd = efl_data_scope_get(child, EFL_UI_VIEW_MODEL_CLASS);
   cpd->parent = ppd;
   cpd->propagating = ppd->propagating;
}

/**
 * @brief Looks up or creates a view model proxy for a given child model.
 * If a view model proxy for `view` already exists as a composite child of `parent`,
 * it is returned. Otherwise, a new Efl_Ui_View_Model is created, configured with
 * `view` as its underlying model and `pd` (parent's data) for parent linking,
 * and then returned. The new proxy is remembered for future lookups.
 *
 * @param pd The parent view model's private data.
 * @param parent The parent Efl_Object (the current view model instance).
 * @param view The child Efl_Model for which to get/create a view model proxy.
 * @return The Efl_Ui_View_Model proxy for the child, or NULL on failure.
 */
static Efl_Ui_View_Model *
_efl_ui_view_model_child_lookup(Efl_Ui_View_Model_Data *pd, Efl_Object *parent, Efl_Model *view)
{
   EFL_COMPOSITE_LOOKUP_RETURN(co, parent, view, "_efl.ui.view_model");

   co = efl_add(EFL_UI_VIEW_MODEL_CLASS, parent,
                efl_ui_view_model_set(efl_added, view),
                _efl_ui_view_model_parent_data(efl_added, pd));
   if (!co) return NULL;

   EFL_COMPOSITE_REMEMBER_RETURN(co, view);
}

/**
 * @brief Handles the EFL_MODEL_EVENT_CHILD_ADDED event from the underlying model.
 * If `children_bind` is enabled, this function intercepts the event, creates
 * (or looks up) a view model proxy for the added child, and then re-emits
 * a new EFL_MODEL_EVENT_CHILD_ADDED event with the proxy as the child.
 *
 * @param data The view model's private data (Efl_Ui_View_Model_Data *).
 * @param event The original EFL_MODEL_EVENT_CHILD_ADDED event.
 *              `event->info` is an Efl_Model_Children_Event*.
 *              Example `event->info->child`: the raw Efl_Model child.
 *              This function might emit a new event with `child`: the Efl_Ui_View_Model proxy.
 */
static void
_efl_ui_view_model_child_added(void *data, const Efl_Event *event)
{
   Efl_Model_Children_Event *ev = event->info;
   Efl_Model_Children_Event nevt = { 0 };
   Efl_Ui_View_Model_Data *pd = data;
   Efl_Ui_View_Model *co;

   if (pd->propagating.child_added) return ;
   if (!pd->children_bind) return;
   if (!ev->child) return;

   pd->propagating.child_added = EINA_TRUE;

   // Our strategy is to rebuild a new Child_Add and cancel the current one.
   efl_event_callback_stop(event->object);

   co = _efl_ui_view_model_child_lookup(pd, event->object, ev->child);
   if (!co) return;

   nevt.index = ev->index;
   nevt.child = co;

   efl_event_callback_call(event->object, EFL_MODEL_EVENT_CHILD_ADDED, &nevt);

   pd->propagating.child_added = EINA_FALSE;
}

/**
 * @brief Handles the EFL_MODEL_EVENT_CHILD_REMOVED event from the underlying model.
 * If `children_bind` is enabled, this function intercepts the event, looks up
 * the view model proxy for the removed child, re-emits a new
 * EFL_MODEL_EVENT_CHILD_REMOVED event with the proxy as the child, and then
 * deletes the proxy.
 *
 * @param data The view model's private data (Efl_Ui_View_Model_Data *).
 * @param event The original EFL_MODEL_EVENT_CHILD_REMOVED event.
 *              `event->info` is an Efl_Model_Children_Event*.
 *              Example `event->info->child`: the raw Efl_Model child.
 *              This function might emit a new event with `child`: the Efl_Ui_View_Model proxy.
 */
static void
_efl_ui_view_model_child_removed(void *data, const Efl_Event *event)
{
   Efl_Model_Children_Event *ev = event->info;
   Efl_Model_Children_Event nevt = { 0 };
   Efl_Ui_View_Model_Data *pd = data;
   Efl_Ui_View_Model *co;

   if (pd->propagating.child_removed) return ;
   if (!pd->children_bind) return;
   if (!ev->child) return;

   pd->propagating.child_removed = EINA_TRUE;

   // Our strategy is to rebuild a new Child_Add and cancel the current one.
   efl_event_callback_stop(event->object);

   co = _efl_ui_view_model_child_lookup(pd, event->object, ev->child);
   if (!co) return;

   nevt.index = ev->index;
   nevt.child = co;

   efl_event_callback_call(event->object, EFL_MODEL_EVENT_CHILD_REMOVED, &nevt);

   // The object is being destroyed, there is no point in us keeping the ViewModel proxy alive.
   efl_del(co);

   pd->propagating.child_removed = EINA_FALSE;
}

EFL_CALLBACKS_ARRAY_DEFINE(efl_ui_view_model_intercept,
                           { EFL_MODEL_EVENT_PROPERTIES_CHANGED, _efl_ui_view_model_property_changed },
                           { EFL_MODEL_EVENT_CHILD_ADDED, _efl_ui_view_model_child_added },
                           { EFL_MODEL_EVENT_CHILD_REMOVED, _efl_ui_view_model_child_removed })

/**
 * @brief Constructor for Efl_Ui_View_Model.
 * Initializes the private data structure, including hash tables for bindings,
 * logics, texts, and property deduplication. Sets up event callbacks to intercept
 * model events.
 * @param obj The Efl_Object being constructed.
 * @param pd The view model's private data.
 * @return The constructed Efl_Object.
 */
static Efl_Object *
_efl_ui_view_model_efl_object_constructor(Eo *obj, Efl_Ui_View_Model_Data *pd)
{
   obj = efl_constructor(efl_super(obj, EFL_UI_VIEW_MODEL_CLASS));

   pd->children_bind = EINA_TRUE;
   pd->bound = eina_hash_stringshared_new(_bind_free);
   pd->logics = eina_hash_stringshared_new(_logic_free);
   pd->deduplication = eina_hash_stringshared_new(_ref_free);
   pd->texts = eina_hash_stringshared_new(_text_free);

   efl_event_callback_array_priority_add(obj, efl_ui_view_model_intercept(), EFL_CALLBACK_PRIORITY_BEFORE, pd);

   return obj;
}

/**
 * @brief Finalizer for Efl_Ui_View_Model.
 * Marks the view model as finalized. After this point, certain configurations
 * like `children_bind` cannot be changed.
 * @param obj The Efl_Object being finalized.
 * @param pd The view model's private data.
 * @return The finalized Efl_Object from the superclass.
 */
static Efl_Object *
_efl_ui_view_model_efl_object_finalize(Eo *obj, Efl_Ui_View_Model_Data *pd)
{
   pd->finalized = EINA_TRUE;

   return efl_finalize(efl_super(obj, EFL_UI_VIEW_MODEL_CLASS));
}

/**
 * @brief Destructor for Efl_Ui_View_Model.
 * Cleans up resources, including removing event callbacks and freeing hash tables.
 * @param obj The Efl_Object being destructed.
 * @param pd The view model's private data.
 */
static void
_efl_ui_view_model_efl_object_destructor(Eo *obj, Efl_Ui_View_Model_Data *pd)
{
   efl_event_callback_array_del(obj, efl_ui_view_model_intercept(), pd);

   eina_hash_free(pd->bound);
   pd->bound = NULL;

   eina_hash_free(pd->logics);
   pd->logics = NULL;

   eina_hash_free(pd->texts);
   pd->texts = NULL;

   eina_hash_free(pd->deduplication);
   pd->deduplication = NULL;

   efl_destructor(efl_super(obj, EFL_UI_VIEW_MODEL_CLASS));
}

/**
 * @brief Looks up a logic block for a property, traversing up to parent view models.
 * Searches for an Efl_Ui_View_Model_Logic definition for the given `property`
 * first in the current view model's data (`pd`), and then recursively in its
 * parent's data if not found.
 *
 * @param pd The current view model's private data.
 * @param property The name of the property to look up logic for.
 * @return The found Efl_Ui_View_Model_Logic, or NULL if no logic is defined for the property.
 */
static Efl_Ui_View_Model_Logic *
_efl_ui_view_model_property_logic_lookup(Efl_Ui_View_Model_Data *pd, Eina_Stringshare *property)
{
   Efl_Ui_View_Model_Logic *logic;

   if (!pd) return NULL;
   logic = eina_hash_find(pd->logics, property);
   if (!logic) return _efl_ui_view_model_property_logic_lookup(pd->parent, property);
   return logic;
}

/**
 * @brief Sets a property value on the view model.
 * This function first checks if there's custom logic (Efl_Ui_View_Model_Logic)
 * defined for the property. If so, it uses the custom set function.
 * If not, it checks if the property is a defined text property (which are read-only).
 * Otherwise, it falls back to setting the property on the superclass (underlying model).
 *
 * @param obj The Efl_Ui_View_Model instance.
 * @param pd The view model's private data.
 * @param property The name of the property to set.
 * @param value The Eina_Value to set the property to.
 * @return An Eina_Future that resolves when the set operation is complete, or is
 *         rejected on error (e.g., if the property is read-only or an error occurs
 *         in a custom setter).
 */
static Eina_Future *
_efl_ui_view_model_efl_model_property_set(Eo *obj, Efl_Ui_View_Model_Data *pd,
                                       const char *property, Eina_Value *value)
{
   Efl_Ui_View_Model_Logic *logic;
   Eina_Stringshare *prop;
   Eina_Future *f;

   prop = eina_stringshare_add(property);
   logic = _efl_ui_view_model_property_logic_lookup(pd, prop);
   if (logic)
     f = logic->set.fct(logic->get.data, obj, prop, value);
   else
     {
        if (eina_hash_find(pd->texts, prop))
          f = efl_loop_future_rejected(obj, EFL_MODEL_ERROR_READ_ONLY);
        else
          f = efl_model_property_set(efl_super(obj, EFL_UI_VIEW_MODEL_CLASS), property, value);
     }

   eina_stringshare_del(prop);
   return f;
}

/**
 * @brief Generates a string by processing a pattern string with property placeholders.
 * Iterates through the `pattern` string, replacing placeholders like "${property_name}"
 * with the actual values of those properties fetched from `obj`.
 *
 * @param obj The Efl_Model from which to fetch property values.
 * @param out The Eina_Strbuf to append the generated string to.
 * @param pattern The pattern string to process (e.g., "Name: ${name}, Age: ${age}").
 * @param stop_on_error If EINA_TRUE, and fetching a property results in an error or
 *                      EAGAIN, generation stops and an error Eina_Value is returned.
 *                      If EINA_FALSE, errors might be replaced with "Unknown property"
 *                      or similar, and generation continues.
 * @return An Eina_Value containing the generated string on success.
 *         If `stop_on_error` is true and an error occurs, returns an Eina_Value of
 *         type EINA_VALUE_TYPE_ERROR. The specific error code (e.g., EAGAIN)
 *         can be retrieved using eina_value_error_get().
 *         Example success return: Eina_Value(type=STRING, value="Name: John, Age: 30")
 *         Example error return: Eina_Value(type=ERROR, error_code=EAGAIN)
 */
static Eina_Value *
_efl_ui_view_model_text_generate(const Eo *obj,
                              Eina_Strbuf *out,
                              Eina_Stringshare *pattern,
                              Eina_Bool stop_on_error)
{
   Eina_Stringshare *st;
   Eina_Stringshare *sp;
   int lookup;

   for (lookup = _lookup_next_token(pattern, &st, &sp);
        lookup;
        pattern += lookup, lookup = _lookup_next_token(pattern, &st, &sp))
     {
        Eina_Value *request;
        char *sr;

        eina_strbuf_append(out, st);

        if (!sp) continue;

        request = efl_model_property_get(obj, sp);
        if (!request)
          {
             if (stop_on_error)
               return eina_value_error_new(EFL_MODEL_ERROR_NOT_SUPPORTED);
             eina_strbuf_append(out, "Unknown property");
             continue;
          }
        if (eina_value_type_get(request) == EINA_VALUE_TYPE_ERROR && stop_on_error)
          return request;

        sr = eina_value_to_string(request);
        eina_strbuf_append(out, sr);

        free(sr);
        eina_value_free(request);
     }

   return eina_value_string_new(eina_strbuf_string_get(out));
}

/**
 * @brief Gets the value of a composite text property.
 * This function looks up the definition of a text property (Efl_Ui_View_Model_Text)
 * and generates its string value using `_efl_ui_view_model_text_generate`.
 * It handles `not_ready` and `on_error` fallback patterns if defined for the text property.
 * It also searches in parent view models if the text property is not defined locally.
 *
 * @param obj The Efl_Ui_View_Model instance from which to get the property.
 *            Property values referenced in the text definition are fetched from this object.
 * @param pd The view model's private data.
 * @param prop The name of the text property to get.
 * @return An Eina_Value containing the generated string for the text property.
 *         If the property is not defined or an error occurs that isn't handled by
 *         `on_error` patterns, it might return NULL or an error Eina_Value.
 *         Example return: Eina_Value(type=STRING, value="User: Alice (Online)")
 */
static Eina_Value *
_efl_ui_view_model_text_property_get(const Eo *obj, Efl_Ui_View_Model_Data *pd, Eina_Stringshare *prop)
{
   Efl_Ui_View_Model_Text *lookup;
   Eina_Strbuf *buf;
   Eina_Value *r;
   Eina_Error err = 0;

   if (!pd) return NULL;
   lookup = eina_hash_find(pd->texts, prop);
   // Lookup for property definition in the parent, but property value will be fetched on
   // the child object doing the request.
   if (!lookup) return _efl_ui_view_model_text_property_get(obj, pd->parent, prop);

   buf = eina_strbuf_new();

   r = _efl_ui_view_model_text_generate(obj, buf,
                                     lookup->definition,
                                     !!(lookup->on_error || lookup->not_ready));
   if (eina_value_type_get(r) != EINA_VALUE_TYPE_ERROR)
     goto done;
   if (eina_value_error_get(r, &err) && err == EAGAIN && lookup->not_ready)
     {
        eina_strbuf_reset(buf);
        eina_value_free(r);

        r = _efl_ui_view_model_text_generate(obj, buf, lookup->not_ready, !!lookup->on_error);
        if (eina_value_type_get(r) != EINA_VALUE_TYPE_ERROR)
          goto done;
     }
   if (lookup->on_error)
     {
        eina_strbuf_reset(buf);
        eina_value_free(r);

        r = _efl_ui_view_model_text_generate(obj, buf, lookup->on_error, 0);
     }

 done:
   eina_strbuf_free(buf);

   return r;
}

/**
 * @brief Gets a property value from the view model.
 * This function prioritizes property sources in the following order:
 * 1. Custom logic (Efl_Ui_View_Model_Logic) defined for the property.
 * 2. Composite text property (Efl_Ui_View_Model_Text) definition.
 * 3. The superclass (underlying model).
 * It checks for logic and text properties in the current view model and its parents.
 *
 * @param obj The Efl_Ui_View_Model instance.
 * @param pd The view model's private data.
 * @param property The name of the property to get.
 * @return An Eina_Value containing the property's value, or an error Eina_Value
 *         if the property cannot be retrieved.
 *         Example return: Eina_Value(type=INT, value=42)
 *         Example return: Eina_Value(type=STRING, value="Computed Text")
 */
static Eina_Value *
_efl_ui_view_model_efl_model_property_get(const Eo *obj, Efl_Ui_View_Model_Data *pd,
                                       const char *property)
{
   Efl_Ui_View_Model_Logic *logic;
   Eina_Stringshare *prop;
   Eina_Value *r;

   prop = eina_stringshare_add(property);
   logic = _efl_ui_view_model_property_logic_lookup(pd, prop);
   if (logic)
     r = logic->get.fct(logic->get.data, obj, prop);
   else
     {
        r = _efl_ui_view_model_text_property_get(obj, pd, prop);
        if (!r) r = efl_model_property_get(efl_super(obj, EFL_UI_VIEW_MODEL_CLASS), property);
     }

   eina_stringshare_del(prop);
   return r;
}

/**
 * @brief Gets an iterator over all property names available in the view model.
 * This includes properties from the underlying model (via superclass) and
 * properties defined by this view model (logics, texts), which are tracked
 * in `pd->deduplication`.
 *
 * @param obj The Efl_Ui_View_Model instance.
 * @param pd The view model's private data.
 * @return An Eina_Iterator that yields property names (const char *).
 *         Example iteration: "name", "age", "custom_logic_prop", "formatted_text_prop".
 */
static Eina_Iterator *
_efl_ui_view_model_efl_model_properties_get(const Eo *obj, Efl_Ui_View_Model_Data *pd)
{
   EFL_COMPOSITE_MODEL_PROPERTIES_SUPER(props, obj, EFL_UI_VIEW_MODEL_CLASS,
                                        eina_hash_iterator_key_new(pd->deduplication));

   return props;
}

/**
 * @brief Private structure for holding data related to a children slice request.
 * Used to pass context to the `then` callback of a future.
 */
typedef struct _Efl_Ui_View_Model_Slice_Request Efl_Ui_View_Model_Slice_Request;
struct _Efl_Ui_View_Model_Slice_Request
{
   Efl_Ui_View_Model_Data *pd; /**< The parent view model's private data. */
   unsigned int start; /**< The start index of the requested slice (currently unused in _efl_ui_view_model_slice_then). */
};

/**
 * @brief `then` callback for handling the result of a children_slice_get future from the superclass.
 * This function takes the Eina_Value array of Efl_Model children returned by the
 * superclass, and for each child, it looks up or creates an Efl_Ui_View_Model proxy.
 * It then returns a new Eina_Value array containing these view model proxies.
 *
 * @param o The Efl_Ui_View_Model instance on which the original slice_get was called.
 * @param data Pointer to an Efl_Ui_View_Model_Slice_Request structure.
 * @param v The Eina_Value returned by the superclass's children_slice_get. This is
 *          expected to be an array of Efl_Model objects.
 *          Example `v`: Eina_Value(type=ARRAY, elements=[Efl_Model_Child1, Efl_Model_Child2])
 * @return An Eina_Value array containing Efl_Ui_View_Model proxies for the children.
 *         Example return: Eina_Value(type=ARRAY, elements=[Efl_Ui_View_Model_Proxy1, Efl_Ui_View_Model_Proxy2])
 */
static Eina_Value
_efl_ui_view_model_slice_then(Eo *o, void *data, const Eina_Value v)
{
   Efl_Ui_View_Model_Slice_Request *req = data;
   Eo *target;
   Eina_Value r = EINA_VALUE_EMPTY;
   unsigned int i, len;

   eina_value_array_setup(&r, EINA_VALUE_TYPE_OBJECT, 4);

   EINA_VALUE_ARRAY_FOREACH(&v, len, i, target)
     {
        Eo *composite;

        composite = _efl_ui_view_model_child_lookup(req->pd, o, target);
        eina_value_array_append(&r, composite);
     }

   return r;
}

/**
 * @brief Cleans up data associated with a children slice request.
 * This is typically used as the `free` callback for an efl_future_then operation.
 * @param o The Efl_Object (unused).
 * @param data Pointer to the Efl_Ui_View_Model_Slice_Request to free.
 * @param dead_future The future that has completed (unused).
 */
static void
_efl_ui_view_model_slice_clean(Eo *o EINA_UNUSED, void *data, const Eina_Future *dead_future EINA_UNUSED)
{
   free(data);
}

/**
 * @brief Gets a slice of children from the view model.
 * This function calls `efl_model_children_slice_get` on the superclass (underlying model)
 * and then processes the result using `_efl_ui_view_model_slice_then`. The `then`
 * callback wraps each child model from the superclass result in an Efl_Ui_View_Model proxy.
 * This is done if `children_bind` is enabled.
 *
 * @param obj The Efl_Ui_View_Model instance.
 * @param pd The view model's private data.
 * @param start The starting index of the slice.
 * @param count The number of children in the slice.
 * @return An Eina_Future that resolves to an Eina_Value array of Efl_Ui_View_Model
 *         proxies for the children in the slice.
 *         Example resolved future value: Eina_Value(type=ARRAY, elements=[Efl_Ui_View_Model_Proxy_Child1, Efl_Ui_View_Model_Proxy_Child2])
 */
static Eina_Future *
_efl_ui_view_model_efl_model_children_slice_get(Eo *obj, Efl_Ui_View_Model_Data *pd,
                                             unsigned int start, unsigned int count)
{
   Efl_Ui_View_Model_Slice_Request *req;
   Eina_Future *f;

   f = efl_model_children_slice_get(efl_super(obj, EFL_UI_VIEW_MODEL_CLASS), start, count);

   req = malloc(sizeof (Efl_Ui_View_Model_Slice_Request));
   if (!req)
     {
        eina_future_cancel(f);
        return efl_loop_future_rejected(obj, ENOMEM);
     }

   req->pd = pd;
   req->start = start;

   return efl_future_then(obj, f, .success_type = EINA_VALUE_TYPE_ARRAY,
                          .success = _efl_ui_view_model_slice_then,
                          .free = _efl_ui_view_model_slice_clean,
                          .data = req);
}

#include "efl_ui_view_model.eo.c"
