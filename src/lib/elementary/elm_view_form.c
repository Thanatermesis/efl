#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#include <Eo.h>
#include <Efl.h>
#include <Elementary.h>

#include "elm_view_form.h"
#include "elm_view_list.h"

#include "elm_priv.h"
#include "elm_entry_eo.h"
#include "elm_thumb_eo.h"
#include "elm_label_eo.h"

#include <assert.h>

#define MY_CLASS ELM_VIEW_FORM_CLASS
#define MY_CLASS_NAME "View_Form"

/**
 * @brief Private data structure for Elm_View_Form instances.
 * This structure holds the internal state of an Elm_View_Form object,
 * including its data model and a list of linked UI widgets.
 */
typedef struct _Elm_View_Form_Data Elm_View_Form_Data;

/**
 * @brief Represents a widget within the form. (Currently unused placeholder typedef)
 * This typedef might be intended for future enhancements where widgets
 * require more complex management within the form.
 */
typedef struct _Elm_View_Form_Widget Elm_View_Form_Widget;

/**
 * @brief Represents a promise related to form operations. (Currently unused placeholder typedef)
 * This typedef could be used for asynchronous operations within the form,
 * though it's not utilized in the current implementation.
 */
typedef struct _Elm_View_Form_Promise Elm_View_Form_Promise;

/**
 * @brief Internal data for Elm_View_Form.
 */
struct _Elm_View_Form_Data
{
   Eo *model;        /**< The data model associated with this form view. */
   Eina_List *links; /**< A list of Eo objects (widgets) linked to this form view. These widgets are typically property-bound to the model. */
};

/**
 * @brief Callback invoked when a linked widget is being deleted.
 * This function removes the dying widget from the internal list of linked objects
 * to prevent dangling pointers and ensure proper resource management.
 *
 * @param data Pointer to the Elm_View_Form_Data structure.
 * @param event The EFL_EVENT_DEL event information. The event->object is the widget being deleted.
 */
static void
_link_dying(void* data, Efl_Event const* event)
{
   Elm_View_Form_Data *priv = data;

   priv->links = eina_list_remove(priv->links, event->object);
}

/**
 * @brief Adds a new widget to the form and links it to the form's model.
 * This internal helper function takes a widget, associates it with the
 * form's current data model, and binds it to a specified property of that model.
 * It also sets up a callback to manage the widget's lifecycle.
 *
 * @param priv The private data of the Elm_View_Form.
 * @param property The name of the model property to which the widget should be bound.
 *                 If NULL, "default" is used.
 * @param link The Evas_Object (widget) to be added and linked. Must implement
 *             EFL_UI_PROPERTY_BIND_INTERFACE.
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., if `link` does not
 *         implement the required interface).
 */
static Eina_Bool
_elm_view_widget_add(Elm_View_Form_Data *priv, const char *property, Evas_Object *link)
{
   if (!efl_isa(link, EFL_UI_PROPERTY_BIND_INTERFACE)) return EINA_FALSE;
   if (!property) property = "default";

   efl_ui_view_model_set(link, priv->model);
   efl_ui_property_bind(link, "default", property);
   efl_event_callback_add(link, EFL_EVENT_DEL, _link_dying, priv);

   priv->links = eina_list_append(priv->links, link);

   return EINA_TRUE;
}
/**
 * Helper functions - End
 */

/**
 * @brief Destructor for Elm_View_Form objects.
 * This function is called when an Elm_View_Form object is destroyed.
 * It frees the list of linked widgets and unreferences the data model.
 *
 * @param obj The Elm_View_Form object being destroyed.
 * @param priv The private data associated with the object.
 */
static void
_elm_view_form_efl_object_destructor(Eo *obj, Elm_View_Form_Data *priv)
{
   // The Eina_List itself is freed, but not the items it contains,
   // as their lifecycle is managed by their respective owners (they are unlinked via _link_dying).
   priv->links = eina_list_free(priv->links);
   efl_unref(priv->model);
   priv->model = NULL;

   efl_destructor(efl_super(obj, MY_CLASS));
}

/**
 * @brief Sets the data model for the Elm_View_Form.
 * This function updates the internal data model of the form. It also iterates
 * through all currently linked widgets and updates their model to the new one.
 *
 * @param obj The Elm_View_Form object.
 * @param priv The private data of the Elm_View_Form.
 * @param model The new Eo data model to set. The form will take a reference.
 */
static void
_elm_view_form_model_set(Eo *obj EINA_UNUSED, Elm_View_Form_Data *priv, Eo *model)
{
   Efl_Object *link;
   Eina_List *l;

   efl_replace(&priv->model, model);

   // Update the model for all already linked widgets
   EINA_LIST_FOREACH(priv->links, l, link)
     efl_ui_view_model_set(link, priv->model);
}

/**
 * @brief Adds a widget to the form and binds it to a property of the model.
 * This is the Efl_Ui_View_Form_widget_add interface implementation.
 * It uses the internal _elm_view_widget_add helper to perform the actual addition and linking.
 *
 * @param obj The Elm_View_Form object.
 * @param priv The private data of the Elm_View_Form.
 * @param propname The name of the model property to bind the widget to.
 *                 For example, if the model has a property "name", `propname` would be "name".
 * @param evas The Evas_Object (widget) to add to the form. This widget should
 *             typically be an input field or display element that can be bound
 *             to a model property. It must implement EFL_UI_PROPERTY_BIND_INTERFACE.
 */
static void
_elm_view_form_widget_add(Eo *obj EINA_UNUSED, Elm_View_Form_Data *priv, const char *propname, Evas_Object *evas)
{
   EINA_SAFETY_ON_NULL_RETURN(evas);

   _elm_view_widget_add(priv, propname, evas);
}

#include "elm_view_form_eo.c"
