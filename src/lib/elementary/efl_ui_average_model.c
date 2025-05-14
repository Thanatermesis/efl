#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Elementary.h>
#include "elm_priv.h"

// This class rely on the parent class to do all the individual holding of value,
// it only compute the average size of an item and answer for that property alone.

// FIXME: handle child being removed

/**
 * @internal
 * @brief Private data for the Efl_Ui_Average_Model class.
 *
 * This structure holds data necessary for calculating the average size of items.
 * It maintains a reference to its parent's data if it's part of a nested
 * average model structure, and accumulates total dimensions and counts of
 * items seen to compute averages.
 */
typedef struct _Efl_Ui_Average_Model_Data Efl_Ui_Average_Model_Data;
struct _Efl_Ui_Average_Model_Data
{
   Efl_Ui_Average_Model_Data *parent; /**< Pointer to the parent model's data, if any. Used for nested averaging. */

   struct {
      unsigned long long width;  /**< Accumulated total width of all child items considered for averaging. */
      unsigned long long height; /**< Accumulated total height of all child items considered for averaging. */
      unsigned long long wseen;  /**< Count of child items whose widths have been processed. */
      unsigned long long hseen;  /**< Count of child items whose heights have been processed. */
   } total; /**< Aggregated dimensions and counts from child models. */

   Eina_Bool wseen : 1; /**< Flag indicating if this specific model instance's width has been set at least once. */
   Eina_Bool hseen : 1; /**< Flag indicating if this specific model instance's height has been set at least once. */
};

/**
 * @internal
 * @brief Structure to hold data for an asynchronous property update operation.
 *
 * When a property (like 'selfw' or 'selfh') is set, the update to the
 * average calculation is done asynchronously. This structure carries the
 * necessary information for that update.
 */
typedef struct _Efl_Ui_Average_Model_Update Efl_Ui_Average_Model_Update;
struct _Efl_Ui_Average_Model_Update
{
   unsigned long long *total; /**< Pointer to the parent's total dimension (e.g., total.width or total.height) to be updated. */
   unsigned long long *seen;  /**< Pointer to the parent's seen count (e.g., total.wseen or total.hseen) to be updated. Can be NULL if the item was already seen. */
   unsigned int previous;     /**< The previous value of the property being changed, used to correctly adjust the total. */
};

/**
 * @internal
 * @brief Callback function executed upon successful completion of a property set future.
 *
 * This function updates the accumulated total dimension and seen count in the parent
 * model's data based on the new property value and the previous value stored in
 * the Efl_Ui_Average_Model_Update structure.
 *
 * @param obj The Efl_Model object (unused in this function).
 * @param data Pointer to the Efl_Ui_Average_Model_Update structure containing update details.
 * @param v The Eina_Value containing the new property value that was set.
 * @return The Eina_Value v itself, passed through.
 */
static Eina_Value
_efl_ui_average_model_update(Eo *obj EINA_UNUSED, void *data, const Eina_Value v)
{
   Efl_Ui_Average_Model_Update *request = data;
   unsigned int now;

   if (!eina_value_uint_convert(&v, &now))
     goto on_error;

   *(request->total) += now - request->previous;
   if (request->seen) *(request->seen) += 1;

 on_error:
   return v;
}

/**
 * @internal
 * @brief Callback function to clean up resources after a property set operation.
 *
 * This function is called when the future associated with a property set
 * operation is freed (either on success, error, or cancellation). It frees
 * the Efl_Ui_Average_Model_Update data and unreferences the model object
 * that was reffed during _efl_ui_average_model_prepare.
 *
 * @param obj The Efl_Model object that was reffed.
 * @param data Pointer to the Efl_Ui_Average_Model_Update structure to be freed.
 * @param dead_future The future that has completed or been cancelled (unused).
 */
static void
_efl_ui_average_model_clean(Eo *obj, void *data, const Eina_Future *dead_future EINA_UNUSED)
{
   free(data);

   efl_unref(obj);
}

/**
 * @internal
 * @brief Prepares and initiates an asynchronous update for a size property.
 *
 * This function is called when 'selfw' or 'selfh' properties are being set.
 * It retrieves the old value of the property, sets up an
 * Efl_Ui_Average_Model_Update structure, and then calls the parent class's
 * property_set. It then chains a future to update the average totals upon
 * successful completion of the parent's set operation.
 *
 * The object `obj` is reffed to ensure it lives until the asynchronous operation
 * completes, and unreffed in `_efl_ui_average_model_clean`.
 *
 * @param obj The Efl_Model object whose property is being set.
 * @param total Pointer to the parent's total dimension (e.g., total.width or total.height) to be updated.
 * @param seen Pointer to the parent's seen count (e.g., total.wseen or total.hseen). Can be NULL if this item's dimension was already counted.
 * @param property The name of the property being set (e.g., "selfw" or "selfh").
 * @param value The new Eina_Value for the property.
 * @return An Eina_Future that resolves when the property set and subsequent average update are complete.
 *         Returns a rejected future on error (e.g., memory allocation failure, incorrect value type).
 */
static Eina_Future *
_efl_ui_average_model_prepare(Eo *obj,
                              unsigned long long *total, unsigned long long *seen,
                              const char *property, Eina_Value *value)
{
   Efl_Ui_Average_Model_Update *update;
   Eina_Value *previous;
   Eina_Future *f;

   update = calloc(1, sizeof (Efl_Ui_Average_Model_Update));
   if (!update) return efl_loop_future_rejected(obj, ENOMEM);

   previous = efl_model_property_get(obj, property);
   if (eina_value_type_get(previous) == EINA_VALUE_TYPE_ERROR)
     {
        Eina_Error err;

        // Check the case when that property hasn't been set before
        if (!eina_value_error_convert(previous, &err))
          goto on_error;
        if (err != EAGAIN) goto on_error;
     }
   else if (!eina_value_uint_convert(previous, &update->previous))
     goto on_error;
   eina_value_free(previous);

   update->total = total;
   update->seen = seen;

   // As we are operating asynchronously and we want to make sure that the object
   // survive until the transaction is commited, we will ref the object here
   // and unref on clean. This is necessary so that a nested call of property_set
   // on a model returned by children_slice_get, the user doesn't have to keep a
   // reference around to do the same. It shouldn't create any problem as this
   // future would be cancelled automatically when the parent object get destroyed.
   efl_ref(obj);

   // We have to make the change after we fetch the old value, otherwise, well, no old value left
   f = efl_model_property_set(efl_super(obj, EFL_UI_AVERAGE_MODEL_CLASS), property, value);

   return efl_future_then(obj, f,
                          .success = _efl_ui_average_model_update,
                          .free = _efl_ui_average_model_clean,
                          .data = update);
 on_error:
   eina_value_free(previous);
   free(update);
   return efl_loop_future_rejected(obj, EFL_MODEL_ERROR_INCORRECT_VALUE);
}

/**
 * @internal
 * @brief Implements Efl_Model.property_set for Efl_Ui_Average_Model.
 *
 * Intercepts setting of "selfh" (and formerly "selfw") properties to update
 * the average height (and width) calculations. If the property is "selfh",
 * it prepares an asynchronous update using `_efl_ui_average_model_prepare`.
 * For other properties, or if there's no parent model to update, it falls
 * back to the superclass's implementation.
 *
 * @param obj The Efl_Ui_Average_Model object.
 * @param pd The private data of the Efl_Ui_Average_Model.
 * @param property The name of the property to set.
 * @param value The new Eina_Value for the property.
 * @return An Eina_Future that resolves when the property set operation is complete.
 */
static Eina_Future *
_efl_ui_average_model_efl_model_property_set(Eo *obj, Efl_Ui_Average_Model_Data *pd, const char *property, Eina_Value *value)
{
   Eina_Future *f = NULL;

   if (!pd->parent) goto end;

   // In vertical list mode we do not need to compute the average width size
   /* if (eina_streq(property, _efl_model_property_selfw)) */
   /*   { */
   /*      f = _efl_ui_average_model_prepare(obj, &pd->parent->total.width, */
   /*                                        pd->wseen ? NULL : &pd->parent->total.wseen, */
   /*                                        property, value, EINA_TRUE); */
   /*      pd->wseen = EINA_TRUE; */
   /*   } */
   if (eina_streq(property, _efl_model_property_selfh))
     {
        f = _efl_ui_average_model_prepare(obj, &pd->parent->total.height,
                                          pd->hseen ? NULL : &pd->parent->total.hseen,
                                          property, value);
        pd->hseen = EINA_TRUE;
     }

 end:
   if (!f)
     f = efl_model_property_set(efl_super(obj, EFL_UI_AVERAGE_MODEL_CLASS), property, value);

   return f;
}

/**
 * @internal
 * @brief Computes the average dimension based on total accumulated size and number of items seen.
 *
 * This function calculates `(total * children_count) / seen`.
 * The multiplication by `children_count` effectively scales the average of
 * *seen* items to an estimated total for *all* children, assuming the unseen
 * children have the same average size as the seen ones.
 *
 * @param obj The Efl_Model object, used to get the total children count.
 * @param r The Eina_Value to be freed and reused for the result.
 * @param total The accumulated total dimension (e.g., total height).
 * @param seen The number of items whose dimensions were included in `total`.
 * @return A new Eina_Value containing the computed average as a uint.
 *         Returns a uint value of 0 if `seen` is 0 to prevent division by zero.
 */
static inline Eina_Value *
_efl_ui_average_model_compute(const Eo *obj, Eina_Value *r, unsigned long long total, unsigned long long seen)
{
   unsigned int count;

   eina_value_free(r);

   // Protection against divide by zero
   if (!seen) return eina_value_uint_new(0);

   count = efl_model_children_count_get(obj);
   // We are doing the multiply first in an attempt to not reduce the precision to early on.
   return eina_value_uint_new((total * count) / seen);
}

/**
 * @internal
 * @brief Implements Efl_Model.property_get for Efl_Ui_Average_Model.
 *
 * Retrieves properties from the superclass. If the requested property is
 * "Total.Height" (or formerly "Total.Width") and the superclass returns a UINT,
 * this function replaces that value with the computed average height (or width)
 * using `_efl_ui_average_model_compute`.
 *
 * @param obj The Efl_Ui_Average_Model object.
 * @param pd The private data of the Efl_Ui_Average_Model.
 * @param property The name of the property to get.
 * @return An Eina_Value containing the property value. The caller owns the value.
 *         Returns NULL if the property is not found by the superclass.
 */
static Eina_Value *
_efl_ui_average_model_efl_model_property_get(const Eo *obj, Efl_Ui_Average_Model_Data *pd, const char *property)
{
   const Eina_Value_Type *t;
   Eina_Value *r;

   r = efl_model_property_get(efl_super(obj, EFL_UI_AVERAGE_MODEL_CLASS), property);
   if (!r) return r;

   // We are checking that the parent class was able to provide an answer to the request for property "Total.Width"
   // or "Total.Height" which means that we are an object that should compute its size. This avoid computing the
   // pointer to the parent object.
   t = eina_value_type_get(r);
   if (t == EINA_VALUE_TYPE_UINT)
     {
        if (eina_streq(property, _efl_model_property_totalh))
          r = _efl_ui_average_model_compute(obj, r, pd->total.height, pd->total.hseen);
        // We do not need to average the width in vertical list mode as this is done by the parent class
        /* if (eina_streq(property, _efl_model_property_totalw)) */
        /*   r = _efl_ui_average_model_compute(obj, r, pd->total.width, pd->total.wseen); */
     }

   return r;
}

/**
 * @internal
 * @brief Constructor for Efl_Ui_Average_Model.
 *
 * Initializes the Efl_Ui_Average_Model. If this model has a parent that
 * is also an Efl_Ui_Average_Model, it stores a pointer to the parent's
 * private data (`pd->parent`). This allows child models to update the
 * aggregate totals stored in their parent model's data.
 *
 * @param obj The Efl_Ui_Average_Model object being constructed.
 * @param pd The private data for this object.
 * @return The constructed object, or NULL on failure.
 */
static Efl_Object *
_efl_ui_average_model_efl_object_constructor(Eo *obj, Efl_Ui_Average_Model_Data *pd)
{
   Eo *parent = efl_parent_get(obj);

   if (parent && efl_isa(parent, EFL_UI_AVERAGE_MODEL_CLASS))
     pd->parent = efl_data_scope_get(efl_parent_get(obj), EFL_UI_AVERAGE_MODEL_CLASS);

   return efl_constructor(efl_super(obj, EFL_UI_AVERAGE_MODEL_CLASS));
}

#include "efl_ui_average_model.eo.c"
