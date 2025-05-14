#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

// Note: we do not rely on reflection here to implement select as it require to asynchronously access
// children. Could be done differently by implementing the children select in the parent instead of
// in the children. For later optimization.

#include <Elementary.h>
#include "elm_priv.h"

#include "efl_ui_select_model.eo.h"
#include "ecore_internal.h"

typedef struct _Efl_Ui_Select_Model_Data Efl_Ui_Select_Model_Data;

/**
 * @brief Private data structure for Efl_Ui_Select_Model.
 *
 * This structure holds the internal state of a select model instance,
 * including its parent, pending selection events, fallback model,
 * last selected model, and current selection mode.
 */
struct _Efl_Ui_Select_Model_Data
{
   Efl_Ui_Select_Model_Data *parent; /**< Pointer to the parent model's private data, if any. */

   Eina_Future *pending_selection_event; /**< Future for a pending selection changed event. */

   Efl_Ui_Select_Model *fallback_model; /**< The model to select if the current selection is removed (in single selection mode). */
   Efl_Ui_Select_Model *last_model; /**< The last model that was selected. */

   Efl_Ui_Select_Mode selection; /**< The current selection mode (none, single, multi). */
};

/**
 * @brief Updates the last selected model and notifies property changes.
 *
 * @param obj The Efl_Ui_Select_Model object.
 * @param pd The private data of the Efl_Ui_Select_Model.
 * @param last_model The new last selected model. Can be NULL.
 */
static void
_efl_ui_select_model_apply_last_model(Eo *obj, Efl_Ui_Select_Model_Data *pd, Eo *last_model)
{
   efl_replace(&pd->last_model, last_model);
   efl_model_properties_changed(obj, "child.selected");
}

/**
 * @brief Callback for when a child model is removed.
 *
 * If the removed child was the last selected model, this function
 * clears the last selected model.
 *
 * @param data The private data of the Efl_Ui_Select_Model.
 * @param event The Efl_Event containing information about the removed child.
 */
static void
_efl_ui_select_model_child_removed(void *data, const Efl_Event *event)
{
   Efl_Ui_Select_Model_Data *pd = data;
   Efl_Model_Children_Event *ev = event->info;

   if (ev->child == pd->last_model)
     _efl_ui_select_model_apply_last_model(event->object, pd, NULL);
}

static Eo*
_efl_ui_select_model_efl_object_constructor(Eo *obj,
                                            Efl_Ui_Select_Model_Data *pd)
{
   Eo *parent;

   obj = efl_constructor(efl_super(obj, EFL_UI_SELECT_MODEL_CLASS));

   efl_boolean_model_boolean_add(obj, "selected", EINA_FALSE);

   efl_event_callback_add(obj, EFL_MODEL_EVENT_CHILD_REMOVED, _efl_ui_select_model_child_removed, pd);

   parent = efl_parent_get(obj);
   if (efl_isa(parent, EFL_UI_SELECT_MODEL_CLASS))
     pd->parent = efl_data_scope_get(parent, EFL_UI_SELECT_MODEL_CLASS);

   return obj;
}

static void
_efl_ui_select_model_efl_object_invalidate(Eo *obj,
                                           Efl_Ui_Select_Model_Data *pd)
{
   efl_replace(&pd->fallback_model, NULL);
   efl_replace(&pd->last_model, NULL);

   efl_invalidate(efl_super(obj, EFL_UI_SELECT_MODEL_CLASS));
}

/**
 * @brief Applies the fallback selection if necessary.
 *
 * This function is called when an item is unselected, and it checks if
 * a fallback model should be selected. This typically happens in single
 * selection mode when the currently selected item is unselected and a
 * fallback has been specified.
 *
 * @param pd The private data of the parent Efl_Ui_Select_Model.
 */
static void
_efl_ui_select_model_fallback(Efl_Ui_Select_Model_Data *pd)
{
   Eina_Value selected;

   if (!pd->parent) return;
   if (!pd->parent->last_model) return;
   if (!pd->parent->fallback_model) return;
   // I think it only make sense to trigger the fallback on single mode
   if (pd->parent->selection != EFL_UI_SELECT_MODE_SINGLE) return;

   selected = eina_value_bool_init(EINA_TRUE);
   efl_model_property_set(pd->parent->fallback_model, "self.selected", &selected);
   eina_value_flush(&selected);
}

/**
 * @brief Callback for the selection notification future.
 *
 * This function is executed when the future associated with a selection
 * change resolves. It triggers the EFL_UI_SELECTABLE_EVENT_SELECTION_CHANGED event
 * and clears the pending selection event future.
 *
 * @param o The Efl_Ui_Select_Model object.
 * @param data User data (unused).
 * @param v The value from the resolved future.
 * @return The original Eina_Value v.
 */
static Eina_Value
_select_notification_cb(Eo *o, void *data EINA_UNUSED, const Eina_Value v)
{
   Efl_Ui_Select_Model_Data *pd = efl_data_scope_get(o, EFL_UI_SELECT_MODEL_CLASS);

   efl_event_callback_call(o, EFL_UI_SELECTABLE_EVENT_SELECTION_CHANGED, NULL);

   pd->pending_selection_event = NULL;

   return v;
}

/**
 * @brief Schedules a selection changed notification.
 *
 * If there isn't already a pending notification, this function schedules
 * a job to emit the EFL_UI_SELECTABLE_EVENT_SELECTION_CHANGED event.
 * This is used to coalesce multiple selection changes into a single event.
 *
 * @param parent The parent Efl_Ui_Select_Model object that will emit the event.
 * @param pd The private data of the parent Efl_Ui_Select_Model.
 */
static void
_efl_ui_select_model_selection_notification(Eo *parent, Efl_Ui_Select_Model_Data *pd)
{
   if (!pd) return;
   if (pd->pending_selection_event) return;

   pd->pending_selection_event = efl_future_then(parent,
                                                 efl_loop_job(efl_loop_get(parent)),
                                                 .success = _select_notification_cb);
}

/**
 * @brief Commits a selection change on a child model.
 *
 * This function is called after a child's "selected" property has been
 * successfully set. It updates the parent's last_model, emits selection
 * events (SELECTED or UNSELECTED) for the child, and triggers a general
 * selection notification on the parent. It also handles fallback selection
 * if an item is unselected.
 *
 * @param child The child Efl_Ui_Select_Model object whose selection changed.
 * @param data User data (unused).
 * @param v The Eina_Value representing the result of the property set operation.
 *          Expected to be a boolean indicating the new selection state.
 * @return The original Eina_Value v, or an error value if issues occur.
 */
static Eina_Value
_commit_change(Eo *child, void *data EINA_UNUSED, const Eina_Value v)
{
   Efl_Ui_Select_Model_Data *pd;
   Eo *parent;
   Eina_Value *selected = NULL;
   Eina_Bool selflag = EINA_FALSE;

   if (v.type == EINA_VALUE_TYPE_ERROR)
     goto on_error;

   selected = efl_model_property_get(child, "selected");

   parent = efl_parent_get(child);
   pd = efl_data_scope_get(parent, EFL_UI_SELECT_MODEL_CLASS);
   if (!pd) goto on_error;

   eina_value_bool_get(selected, &selflag);
   if (selflag)
     {
        // select case
        _efl_ui_select_model_apply_last_model(parent, pd, child);
        efl_event_callback_call(child, EFL_UI_SELECT_MODEL_EVENT_SELECTED, child);
     }
   else
     {
        // unselect case
        // There should only be one model which represent the same data at all in memory
        if (pd->last_model == child) // direct comparison of pointer is ok
          {
             _efl_ui_select_model_apply_last_model(parent, pd, NULL);

             // Just in case we need to refill the fallback
             _efl_ui_select_model_fallback(pd);
          }
        efl_event_callback_call(child, EFL_UI_SELECT_MODEL_EVENT_UNSELECTED, child);
     }
   _efl_ui_select_model_selection_notification(parent, pd);
   efl_model_properties_changed(child, "self.selected");

 on_error:
   eina_value_free(selected);
   return v;
}

/**
 * @brief Frees a child object, typically used as a future cleanup callback.
 *
 * @param child The child Eo object to unreference.
 * @param data User data (unused).
 * @param dead_future The future that has completed (unused).
 */
static void
_clear_child(Eo *child,
             void *data EINA_UNUSED,
	     const Eina_Future *dead_future EINA_UNUSED)
{
   efl_unref(child);
}

/**
 * @brief Retrieves a child model from an Eina_Value array at a specific index.
 *
 * @param array Pointer to an Eina_Value of type EINA_VALUE_TYPE_ARRAY.
 *              Example: Eina_Value containing [child1_ptr, child2_ptr, ...].
 * @param idx The index of the child model to retrieve.
 * @return The Efl_Model at the specified index, or NULL if the index is
 *         out of bounds or the value is not an array.
 */
static Efl_Model *
_select_child_get(const Eina_Value *array, unsigned int idx)
{
   Efl_Model *ret = NULL;

   if (eina_value_type_get(array) != EINA_VALUE_TYPE_ARRAY)
     return NULL;

   if (idx >= eina_value_array_count(array))
     return NULL;

   eina_value_array_get(array, idx, &ret);

   return ret;
}

/**
 * @brief Checks if a child's selection state needs to change and initiates the change.
 *
 * If the child's current selection state is different from the desired `value`,
 * this function sets the "selected" property on the child. The actual commit
 * of the change is handled by the `_commit_change` callback.
 *
 * @param child The child Efl_Model to potentially change.
 * @param value The desired selection state (EINA_TRUE for selected, EINA_FALSE for unselected).
 * @return A future that resolves with the new selection state (as a boolean Eina_Value)
 *         after the change is committed, or a resolved future if no change was needed.
 */
static Eina_Future *
_check_child_change(Efl_Model *child, Eina_Bool value)
{
   Eina_Future *r = NULL;
   Eina_Value *prev;
   Eina_Bool prevflag = EINA_FALSE;

   prev = efl_model_property_get(child, "selected");
   eina_value_bool_get(prev, &prevflag);
   eina_value_free(prev);

   if (prevflag & value)
     {
        r = efl_loop_future_resolved(child, eina_value_bool_init(value));
     }
   else
     {
        r = efl_model_property_set(child, "selected", eina_value_bool_new(!!value));
        r = efl_future_then(efl_ref(child), r,
                            .success = _commit_change,
                            .free = _clear_child);
     }

   return r;
}

/**
 * @brief Initiates the selection of a child model.
 *
 * @param child The Efl_Model to select.
 * @return A future that resolves after the selection is committed.
 * @see _check_child_change
 */
static Eina_Future *
_select_child(Efl_Model *child)
{
   return _check_child_change(child, EINA_TRUE);
}

/**
 * @brief Initiates the unselection of a child model.
 *
 * @param child The Efl_Model to unselect.
 * @return A future that resolves after the unselection is committed.
 * @see _check_child_change
 */
static Eina_Future *
_unselect_child(Efl_Model *child)
{
   return _check_child_change(child, EINA_FALSE);
}

/**
 * @brief Callback executed after a child slice is retrieved for selection.
 *
 * This function takes the first child from the retrieved slice (expected to be
 * an array containing one child) and initiates its selection.
 *
 * @param obj The Efl_Ui_Select_Model object (unused).
 * @param data User data (unused).
 * @param v An Eina_Value of type EINA_VALUE_TYPE_ARRAY, expected to contain
 *          the child model to be selected at index 0.
 *          Example: Eina_Value containing [child_to_select_ptr].
 * @return An Eina_Value wrapping the future from `_select_child`, or the
 *         original Eina_Value `v` if an error occurs (e.g., child not found).
 */
static Eina_Value
_select_slice_then(Eo *obj EINA_UNUSED,
                   void *data EINA_UNUSED,
                   const Eina_Value v)
{
   Efl_Model *child = NULL;
   Eina_Future *r;

   child = _select_child_get(&v, 0);
   if (!child) goto on_error;

   r = _select_child(child);
   return eina_future_as_value(r);

 on_error:
   return v;
}

/**
 * @brief Callback executed after a child slice is retrieved for unselection.
 *
 * This function takes the first child from the retrieved slice (expected to be
 * an array containing one child) and initiates its unselection.
 *
 * @param obj The Efl_Ui_Select_Model object (unused).
 * @param data User data (unused).
 * @param v An Eina_Value of type EINA_VALUE_TYPE_ARRAY, expected to contain
 *          the child model to be unselected at index 0.
 *          Example: Eina_Value containing [child_to_unselect_ptr].
 * @return An Eina_Value wrapping the future from `_unselect_child`, or the
 *         original Eina_Value `v` if an error occurs (e.g., child not found).
 */
static Eina_Value
_unselect_slice_then(Eo *obj EINA_UNUSED,
                     void *data EINA_UNUSED,
                     const Eina_Value v)
{
   Efl_Model *child = NULL;
   Eina_Future *r;

   child = _select_child_get(&v, 0);
   if (!child) goto on_error;

   r = _unselect_child(child);
   return eina_future_as_value(r);

 on_error:
   return v;
}

/**
 * @brief Regenerates an Eina_Error value, typically used in a future chain.
 *
 * This function is used to propagate an error that was stored earlier.
 * If the input value `v` is already an error, it's returned directly.
 * Otherwise, a new error value is created from the `error` data.
 *
 * @param data Pointer to an Eina_Error that was previously stored. This memory is freed by the function.
 * @param v The current Eina_Value in the future chain.
 * @param dead_future The future that has completed (unused).
 * @return An Eina_Value containing an error.
 */
static Eina_Value
_regenerate_error(void *data,
                  const Eina_Value v,
                  const Eina_Future *dead_future EINA_UNUSED)
{
   Eina_Error *error = data;
   Eina_Value r = v;

   if (v.type == EINA_VALUE_TYPE_ERROR)
     goto cleanup;

   r = eina_value_error_init(*error);

 cleanup:
   free(error);

   return r;
}

/**
 * @brief Extracts the first element from an Eina_Value array.
 *
 * This is used in scenarios where a future chain (like `eina_future_all`)
 * returns an array of results, but only the first result (typically from
 * the primary operation, not a side effect like unselecting a previous item)
 * is needed.
 *
 * @param data User data (unused).
 * @param v An Eina_Value of type EINA_VALUE_TYPE_ARRAY.
 *          Example: Eina_Value containing [result_of_commit_ptr, result_of_unselect_ptr].
 * @return The first Eina_Value from the input array. If `v` is not an array
 *         or is empty, an empty Eina_Value is returned.
 */
static Eina_Value
_untangle_array(void *data EINA_UNUSED,
                const Eina_Value v)
{
   Eina_Value va = EINA_VALUE_EMPTY;

   // Only return the commit change, not the result of the unselect
   eina_value_array_get(&v, 0, &va);
   return va;
}

/**
 * @brief Handles errors during complex selection operations, potentially rolling back changes.
 *
 * In single selection mode, if selecting a new item fails after the old item
 * has been told to unselect, this function attempts to roll back the state by
 * re-selecting the "selected" property of the `child` to EINA_FALSE (as the
 * operation to make it TRUE failed). It then re-propagates the original error.
 *
 * @param data The Efl_Model object (`child`) that was intended to be selected.
 * @param err The Eina_Error that occurred.
 * @return An Eina_Value wrapping a future that will resolve after the rollback
 *         attempt and error propagation.
 */
static Eina_Value
_untangle_error(void *data, Eina_Error err)
{
   Efl_Model *child = data;
   Eina_Future *f;

   // We need to roll back the change, which means in this
   // case to unselect this child as this is the only case
   // where we could end up here.
   Eina_Error *error = calloc(1, sizeof (Eina_Error));

   f = efl_model_property_set(efl_super(child, EFL_UI_SELECT_MODEL_CLASS),
                              "selected", eina_value_bool_new(EINA_FALSE));
   // Once this is done, we need to repropagate the error
   *error = err;
   f = eina_future_then(f, _regenerate_error, error, NULL);

   return eina_future_as_value(f);
}

/**
 * @brief Frees data associated with an untangle operation, typically an Eo object.
 *
 * @param data The Eo object to unreference.
 * @param dead_future The future that has completed (unused).
 */
static void
_untangle_free(void *data,
               const Eina_Future *dead_future EINA_UNUSED)
{
   Eo *obj = data;

   efl_unref(obj);
}

static Eina_Iterator *
_efl_ui_select_model_efl_model_properties_get(const Eo *obj,
                                              Efl_Ui_Select_Model_Data *pd EINA_UNUSED)
{
   EFL_COMPOSITE_MODEL_PROPERTIES_SUPER(props,
                                        obj, EFL_UI_SELECT_MODEL_CLASS,
                                        NULL,
                                        "self.selected", "child.selected", "single_selection");
   return props;
}

static Eina_Future *
_efl_ui_select_model_efl_model_property_set(Eo *obj,
                                            Efl_Ui_Select_Model_Data *pd,
                                            const char *property, Eina_Value *value)
{
   Eina_Value vf = EINA_VALUE_EMPTY;

   if (eina_streq("single_selection", property))
     {
        Eina_Bool single_selection = pd->selection == EFL_UI_SELECT_MODE_SINGLE;
        Eina_Bool new_selection;
        Eina_Bool changed;

        if (!eina_value_bool_get(value, &new_selection))
          return efl_loop_future_rejected(obj, EINVAL);

        changed = (!!new_selection != !!single_selection);
        if (new_selection) pd->selection = EFL_UI_SELECT_MODE_SINGLE;
        else pd->selection = EFL_UI_SELECT_MODE_MULTI;

        if (changed) efl_model_properties_changed(obj, "single_selection");

        return efl_loop_future_resolved(obj, vf);
     }

   if (eina_streq("child.selected", property))
     {
        unsigned long l = 0;

        if (pd->selection == EFL_UI_SELECT_MODE_NONE)
          return efl_loop_future_rejected(obj, EFL_MODEL_ERROR_READ_ONLY);

        if (!eina_value_ulong_convert(value, &l))
          return efl_loop_future_rejected(obj, EFL_MODEL_ERROR_INCORRECT_VALUE);

        return efl_future_then(efl_ref(obj), efl_model_children_slice_get(obj, l, 1),
                               .success = _select_slice_then,
                               .success_type = EINA_VALUE_TYPE_ARRAY,
                               .free = _clear_child);
     }

   if (pd->parent && eina_streq("self.selected", property))
     {
        Eina_Bool prevflag = EINA_FALSE, newflag = EINA_FALSE;
        Eina_Bool single_selection = EINA_FALSE;
        Eina_Bool success;
        Eina_Value *prev;
        Eina_Future *chain;

        if (pd->parent->selection == EFL_UI_SELECT_MODE_NONE)
          return efl_loop_future_rejected(obj, EFL_MODEL_ERROR_INCORRECT_VALUE);

        prev = efl_model_property_get(efl_super(obj, EFL_UI_SELECT_MODEL_CLASS), "selected");
        success = eina_value_bool_get(prev, &prevflag);
        success &= eina_value_bool_convert(value, &newflag);

        if (!success) return efl_loop_future_rejected(obj, EFL_MODEL_ERROR_INCORRECT_VALUE);

        // Nothing changed
        if (newflag == prevflag)
          return efl_loop_future_resolved(obj, eina_value_bool_init(newflag));

        single_selection = !!(pd->parent->selection == EFL_UI_SELECT_MODE_SINGLE);

        // First store the new value in the boolean model we inherit from
        chain = efl_model_property_set(efl_super(obj, EFL_UI_SELECT_MODEL_CLASS),
                                       "selected", value);

        // Now act !
        if (single_selection)
          {
             // We are here either, because we weren't and are after this call
             // or because we were selected and are not anymore. In the later case,
             // there is nothing special to do, just normal commit change will do.
             if (!newflag)
               {
                  if (pd->parent->last_model == obj && !newflag)
                    {
                       _efl_ui_select_model_apply_last_model(efl_parent_get(obj), pd->parent, NULL);

                       _efl_ui_select_model_fallback(pd);
                    }
               }
             else if (pd->parent->last_model)
               {
                  Eo *parent;
                  unsigned long selected = 0;

                  // In this case we need to first unselect the previously selected one
                  // and then commit the change to this one.
                  selected = efl_composite_model_index_get(pd->parent->last_model);

                  // There was, need to unselect the previous one along setting the new value
                  parent = efl_parent_get(obj);
                  chain = eina_future_all(chain,
                                          efl_future_then(efl_ref(parent),
                                                          efl_model_children_slice_get(parent, selected, 1),
                                                          .success = _unselect_slice_then,
                                                          .success_type = EINA_VALUE_TYPE_ARRAY,
                                                          .free = _clear_child));
                  chain = eina_future_then_easy(chain,
                                                .success_type = EINA_VALUE_TYPE_ARRAY,
                                                .success = _untangle_array,
                                                .data = efl_ref(obj),
                                                .error = _untangle_error,
                                                .free = _untangle_free);
               }
             else
               {
                  _efl_ui_select_model_apply_last_model(efl_parent_get(obj), pd->parent, obj);
               }
          }

        return efl_future_then(efl_ref(obj), chain,
                               .success = _commit_change,
                               .free = _clear_child);
     }

   return efl_model_property_set(efl_super(obj, EFL_UI_SELECT_MODEL_CLASS),
                                 property, value);
}

static Eina_Value *
_efl_ui_select_model_efl_model_property_get(const Eo *obj, Efl_Ui_Select_Model_Data *pd, const char *property)
{
   if (eina_streq("single_selection", property))
     return eina_value_bool_new(pd->selection == EFL_UI_SELECT_MODE_SINGLE);
   // Last selected child
   if (eina_streq("child.selected", property))
     {
        if (pd->last_model)
          return eina_value_ulong_new(efl_composite_model_index_get(pd->last_model));
        else if (pd->fallback_model)
          return eina_value_ulong_new(efl_composite_model_index_get(pd->fallback_model));
        else // Nothing selected yet, try again later
          return eina_value_error_new(EAGAIN);
     }
   // Redirect to are we ourself selected
   if (pd->parent && eina_streq("self.selected", property))
     {
        if (pd->parent->selection == EFL_UI_SELECT_MODE_NONE)
          return eina_value_bool_new(EINA_FALSE);
        return efl_model_property_get(efl_super(obj, EFL_UI_SELECT_MODEL_CLASS), "selected");
     }

   return efl_model_property_get(efl_super(obj, EFL_UI_SELECT_MODEL_CLASS), property);
}

static Eina_Iterator *
_efl_ui_select_model_efl_ui_multi_selectable_index_range_selected_ndx_iterator_new(Eo *obj,
                                                                         Efl_Ui_Select_Model_Data *pd)
{
   if (pd->parent && pd->parent->selection == EFL_UI_SELECT_MODE_NONE)
     return eina_list_iterator_new(NULL); // Quick hack to get a valid empty iterator
   return efl_boolean_model_boolean_iterator_get(obj, "selected", EINA_TRUE);
}

static Eina_Iterator *
_efl_ui_select_model_efl_ui_multi_selectable_index_range_unselected_ndx_iterator_new(Eo *obj,
                                                                           Efl_Ui_Select_Model_Data *pd EINA_UNUSED)
{
   return efl_boolean_model_boolean_iterator_get(obj, "selected", EINA_FALSE);
}

static Efl_Ui_Selectable *
_efl_ui_select_model_efl_ui_single_selectable_last_selected_get(const Eo *obj EINA_UNUSED,
                                                                Efl_Ui_Select_Model_Data *pd)
{
   return pd->last_model;
}

static void
_efl_ui_select_model_efl_ui_multi_selectable_select_mode_set(Eo *obj,
                                                                   Efl_Ui_Select_Model_Data *pd,
                                                                   Efl_Ui_Select_Mode mode)
{
   switch (mode)
     {
      case EFL_UI_SELECT_MODE_SINGLE:
         mode = EFL_UI_SELECT_MODE_SINGLE;
         if (pd->selection == EFL_UI_SELECT_MODE_MULTI)
           efl_ui_multi_selectable_all_unselect(obj);
         break;
      case EFL_UI_SELECT_MODE_NONE:
         if (pd->selection == EFL_UI_SELECT_MODE_MULTI)
           efl_ui_multi_selectable_all_unselect(obj);
         else if (pd->last_model)
           {
              Eina_Value unselect = eina_value_bool_init(EINA_FALSE);

              efl_model_property_set(pd->last_model, "self.selected", &unselect);
              eina_value_flush(&unselect);
           }
         break;
      case EFL_UI_SELECT_MODE_MULTI:
         break;
      default:
         ERR("Unknown select mode passed to %s: %i.", efl_debug_name_get(obj), mode);
         return;
     }

   pd->selection = mode;
   efl_model_properties_changed(obj, "single_selection", "child.selected");
}

static Efl_Ui_Select_Mode
_efl_ui_select_model_efl_ui_multi_selectable_select_mode_get(const Eo *obj EINA_UNUSED,
                                                                   Efl_Ui_Select_Model_Data *pd)
{
   return pd->selection;
}

static void
_efl_ui_select_model_efl_ui_multi_selectable_all_select(Eo *obj,
                                                              Efl_Ui_Select_Model_Data *pd EINA_UNUSED)
{
   unsigned int count, i;

   // Not the fastest way to implement it, but will reuse more code and be easier as a v1.
   // It also make it not very async which could be noticeable.
   count = efl_model_children_count_get(obj);

   for (i = 0; i < count; i++)
     {
        Eina_Value p = eina_value_uint_init(i);

        efl_model_property_set(obj, "child.selected", &p);

        eina_value_flush(&p);
     }
}

static void
_efl_ui_select_model_efl_ui_multi_selectable_all_unselect(Eo *obj,
                                                          Efl_Ui_Select_Model_Data *pd EINA_UNUSED)
{
   unsigned int count = efl_model_children_count_get(obj);

   efl_ui_multi_selectable_ndx_range_unselect(obj, 0, count - 1);
}

static void
_efl_ui_select_model_efl_ui_multi_selectable_index_range_ndx_range_select(Eo *obj,
                                                                          Efl_Ui_Select_Model_Data *pd EINA_UNUSED,
                                                                          unsigned int a, unsigned int b)
{
   unsigned long count, i;

   // Not the fastest way to implement it, but will reuse more code and be easier as a v1.
   // It also make it not very async which could be noticeable.
   count = MIN(efl_model_children_count_get(obj), b + 1);

   for (i = a; i < count; i++)
     {
        Eina_Value p = eina_value_ulong_init(i);

        efl_model_property_set(obj, "child.selected", &p);

        eina_value_flush(&p);
     }
}

static Eina_Value
_children_unselect_then(Eo *o EINA_UNUSED, void *data EINA_UNUSED, const Eina_Value v)
{
   Eo *target;
   Eina_Value unselect;
   unsigned int i, len;

   unselect = eina_value_bool_init(EINA_FALSE);

   EINA_VALUE_ARRAY_FOREACH(&v, len, i, target)
     {
        efl_model_property_set(target, "self.selected", &unselect);
     }

   eina_value_flush(&unselect);

   return v;
}

#define BATCH_MAX 100

static void
_efl_ui_select_model_efl_ui_multi_selectable_index_range_ndx_range_unselect(Eo *obj,
                                                                            Efl_Ui_Select_Model_Data *pd EINA_UNUSED,
                                                                            unsigned int a, unsigned int b)
{
   unsigned int count, batch, i;

   count = MIN(efl_model_children_count_get(obj), b + 1);

   // Fetch group request of children in batches not to big to allow for throttling
   // In the callback edit said object property to be unselected
   i = a;
   batch = 0;

   while (i < count)
     {
        Eina_Future *f;

        batch = MIN(i + BATCH_MAX, count);
        batch -= i;

        f = efl_model_children_slice_get(obj, i, batch);
        efl_future_then(obj, f, .success_type = EINA_VALUE_TYPE_ARRAY,
                        .success = _children_unselect_then);

        i += batch;
     }
}

static void
_efl_ui_select_model_efl_ui_single_selectable_fallback_selection_set(Eo *obj,
                                                                     Efl_Ui_Select_Model_Data *pd,
                                                                     Efl_Ui_Selectable *fallback)
{
   Eina_Value *index;

   if (!efl_isa(fallback, EFL_UI_SELECT_MODEL_CLASS))
     {
        ERR("Class of object '%s' does not provide the necessary interface for Efl.Ui.Select_Model.fallback.", efl_debug_name_get(fallback));
        return;
     }
   if (efl_parent_get(fallback) != obj)
     {
        ERR("Provided object '%s' for fallback isn't a child of '%s'.",
            efl_debug_name_get(fallback), efl_debug_name_get(obj));
        return;
     }

   efl_replace(&pd->fallback_model, fallback);

   if (!pd->last_model) return ;

   // When we provide a fallback, we should use it!
   index = efl_model_property_get(fallback, EFL_COMPOSITE_MODEL_CHILD_INDEX);
   efl_model_property_set(obj, "child.selected", index);
   eina_value_free(index);
}

static Efl_Ui_Selectable *
_efl_ui_select_model_efl_ui_single_selectable_fallback_selection_get(const Eo *obj EINA_UNUSED,
                                                                     Efl_Ui_Select_Model_Data *pd)
{
   return pd->fallback_model;
}

static void
_efl_ui_select_model_efl_ui_selectable_selected_set(Eo *obj,
                                                    Efl_Ui_Select_Model_Data *pd EINA_UNUSED,
                                                    Eina_Bool selected)
{
   Eina_Value set = eina_value_bool_init(selected);

   efl_model_property_set(obj, "self.selected", &set);

   eina_value_flush(&set);
}

static Eina_Bool
_efl_ui_select_model_efl_ui_selectable_selected_get(const Eo *obj,
                                                    Efl_Ui_Select_Model_Data *pd EINA_UNUSED)
{
   Eina_Value *selected;
   Eina_Bool r = EINA_FALSE;

   selected = efl_model_property_get(obj, "self.selected");
   eina_value_bool_convert(selected, &r);
   eina_value_free(selected);

   return r;
}

#include "efl_ui_select_model.eo.c"
#include "efl_ui_multi_selectable_index_range.eo.c"
