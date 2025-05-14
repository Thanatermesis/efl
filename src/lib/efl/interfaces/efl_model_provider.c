#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "Efl.h"

/**
 * @internal
 * @brief Private data structure for Efl_Model_Provider.
 *
 * This structure holds the model associated with an Efl_Model_Provider instance.
 */
typedef struct _Efl_Model_Provider_Data Efl_Model_Provider_Data;
struct _Efl_Model_Provider_Data
{
   Efl_Model *model; /**< The model instance provided by this object. */
};

/**
 * @internal
 * @brief Sets the model for the Efl_Model_Provider.
 *
 * This function replaces the current model with the new model.
 * It also emits the "model,changed" event with the previous and current models.
 * References are taken on both previous and current models during the event emission
 * and released afterwards.
 *
 * @param obj The Efl_Model_Provider object.
 * @param pd The private data of the Efl_Model_Provider object.
 * @param model The new model to set.
 */
static void
_efl_model_provider_efl_ui_view_model_set(Eo *obj, Efl_Model_Provider_Data *pd,
                                          Efl_Model *model)
{
   Efl_Model_Changed_Event ev;

   ev.previous = efl_ref(pd->model);
   ev.current = efl_ref(model);
   efl_replace(&pd->model, model);

   efl_event_callback_call(obj, EFL_UI_VIEW_EVENT_MODEL_CHANGED, &ev);

   efl_unref(ev.previous);
   efl_unref(ev.current);
}

/**
 * @internal
 * @brief Gets the model from the Efl_Model_Provider.
 *
 * @param obj The Efl_Model_Provider object (unused).
 * @param pd The private data of the Efl_Model_Provider object.
 * @return The current model associated with the provider.
 */
static Efl_Model *
_efl_model_provider_efl_ui_view_model_get(const Eo *obj EINA_UNUSED,
                                          Efl_Model_Provider_Data *pd)
{
   return pd->model;
}

#include "efl_model_provider.eo.c"
