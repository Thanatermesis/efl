#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#include <Eo.h>
#include <Efl.h>
#include <Elementary.h>

#include "elm_priv.h"
#include "elm_genlist_eo.h"

#include <assert.h>

#define MY_CLASS ELM_VIEW_LIST_CLASS
#define MY_CLASS_NAME "View List"

/**
 * @brief Private data structure for the Elm_View_List widget.
 *
 * This structure holds all the necessary data for managing the view list,
 * including references to the underlying Genlist, the data model,
 * and item class configurations.
 */
typedef struct _Elm_View_List_Data Elm_View_List_Data;

/**
 * @brief Private data structure for individual items in the Elm_View_List.
 *
 * This structure holds data specific to each item displayed in the list,
 * such as its corresponding model, parent item, and associated Genlist item.
 */
typedef struct _View_List_ItemData View_List_ItemData;

/**
 * @brief Structure to hold a value associated with a part of a view list item.
 * @deprecated This struct seems unused.
 */
typedef struct _View_List_ValueItem View_List_ValueItem;

struct _Elm_View_List_Data
{
   Eo *view;                         /**< The Elm_View_List_Eo instance itself. */
   Evas_Object *genlist;             /**< The underlying Evas_Object (Genlist) used for display. */
   View_List_ItemData *rootdata;     /**< Data for the root item, used for top-level model interactions. */
   Elm_Genlist_Item_Class *itc;      /**< The Genlist item class used for creating items. */
   Elm_Genlist_Item_Type itype;      /**< The type of Genlist items (e.g., tree, item). */

   struct {
      Eina_Hash *properties;         /**< Hash table mapping UI part names to model property names. Key: (char*) part_name, Value: (char*) property_name (strdup'd) */
      Eo *model;                     /**< The Efl_Model instance providing the data. */
   } connect;                        /**< Data related to the connection with the Efl_Model. */
};

struct _View_List_ItemData
{
   Elm_View_List_Data *priv;         /**< Pointer to the parent Elm_View_List's private data. */
   Elm_Object_Item *item;            /**< The corresponding Elm_Object_Item (Genlist item). */
   Eo *model;                        /**< The Efl_Model instance for this specific item. */
   View_List_ItemData *parent;       /**< Pointer to the parent item's data, if this is a sub-item. */
   Eina_List *values EINA_UNUSED;    /**< List of View_List_ValueItem. @deprecated This field seems unused. */
};

struct _View_List_ValueItem
{
   Eina_Stringshare *part;           /**< The part name this value is for. */
   Eina_Value *value;                /**< The actual Eina_Value. */
   Elm_Object_Item *item;            /**< The item this value belongs to. */
};

/**
 * @brief Initiates loading of children for a given item from the Efl_Model.
 * @param pdata The item data for which to load children.
 */
static void _efl_model_load_children(View_List_ItemData *);
/**
 * @brief Callback invoked when children are added to the Efl_Model.
 * @param data User data, expected to be View_List_ItemData of the parent.
 * @param event Event information containing details about the added children.
 */
static void _efl_model_children_added_cb(void *, const Efl_Event *event);
/**
 * @brief Callback invoked when children are removed from the Efl_Model.
 * @param data User data, expected to be View_List_ItemData of the parent.
 * @param event Event information containing details about the removed children.
 */
static void _efl_model_children_removed_cb(void *, const Efl_Event *event);
/**
 * @brief Callback invoked when properties of an Efl_Model change.
 * @param data User data, expected to be View_List_ItemData of the affected item.
 * @param event Event information containing details about the property changes.
 */
static void _efl_model_properties_change_cb(void *, const Efl_Event *event);

/**
 * @brief Callback invoked when a Genlist item requests to be expanded.
 * @param data User data (unused).
 * @param event Event information containing the Genlist item to expand.
 */
static void _expand_request_cb(void *data EINA_UNUSED, const Efl_Event *event);
/**
 * @brief Callback invoked when a Genlist item requests to be contracted.
 * @param data User data (unused).
 * @param event Event information containing the Genlist item to contract.
 */
static void _contract_request_cb(void *data EINA_UNUSED, const Efl_Event *event);
/**
 * @brief Callback invoked after a Genlist item has been contracted (sub-items cleared).
 * @param data User data (unused).
 * @param event Event information containing the contracted Genlist item.
 */
static void _contracted_cb(void *data EINA_UNUSED, const Efl_Event *event);

/* --- Genlist Callbacks --- */
/**
 * @brief Callbacks for Efl_Model events (child added/removed).
 */
EFL_CALLBACKS_ARRAY_DEFINE(model_callbacks,
                           { EFL_MODEL_EVENT_CHILD_ADDED, _efl_model_children_added_cb },
                           { EFL_MODEL_EVENT_CHILD_REMOVED, _efl_model_children_removed_cb });
/**
 * @brief Callbacks for Genlist events (expand/contract requests, contracted).
 */
EFL_CALLBACKS_ARRAY_DEFINE(genlist_callbacks,
                          { ELM_GENLIST_EVENT_EXPAND_REQUEST, _expand_request_cb },
                          { ELM_GENLIST_EVENT_CONTRACT_REQUEST, _contract_request_cb },
                          { ELM_GENLIST_EVENT_CONTRACTED, _contracted_cb });

/**
 * @brief Callback invoked when a Genlist item is selected.
 *
 * This function is called when an item in the Genlist is clicked by the user.
 * It triggers the ELM_VIEW_LIST_EVENT_MODEL_SELECTED event on the view list widget.
 *
 * @param data Pointer to the View_List_ItemData associated with the selected item.
 * @param obj The Evas_Object (Genlist) that triggered the event (unused).
 * @param event_info Additional event information (unused).
 */
static void
_item_sel_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   View_List_ItemData *idata = data;

   EINA_SAFETY_ON_NULL_RETURN(idata);

   efl_event_callback_legacy_call(idata->priv->view, ELM_VIEW_LIST_EVENT_MODEL_SELECTED, idata->model);
}

/**
 * @brief Callback invoked when a Genlist item is deleted.
 *
 * This function is responsible for cleaning up resources associated with
 * a View_List_ItemData when its corresponding Genlist item is deleted.
 * This includes unreferencing the Efl_Model and removing event callbacks.
 *
 * @param data Pointer to the View_List_ItemData to be deleted.
 * @param obj The Evas_Object (Genlist) from which the item is being deleted (unused).
 */
static void
_item_del(void *data, Evas_Object *obj EINA_UNUSED)
{
   View_List_ItemData *idata = data;

   if (!idata) return;

   efl_event_callback_array_del(idata->model, model_callbacks(), idata);
   efl_event_callback_del(idata->model, EFL_MODEL_EVENT_PROPERTIES_CHANGED, _efl_model_properties_change_cb, idata);

   efl_unref(idata->model);
   idata->model = NULL;
   idata->item = NULL;
   idata->parent = NULL;
   idata->priv = NULL;

   free(idata);
}

/**
 * @brief Callback to get content (Evas_Object) for a part of a Genlist item.
 *
 * This function is called by Genlist to populate parts of an item (e.g., an icon)
 * that expect an Evas_Object. It retrieves the corresponding property from the
 * Efl_Model based on the part name and attempts to create a suitable Evas_Object
 * (e.g., Elm_Image, Elm_Icon) based on the Eina_Value type.
 *
 * @param data Pointer to the View_List_ItemData for the item.
 * @param obj The parent Evas_Object (Genlist) (unused).
 * @param part The name of the part for which content is requested (e.g., "elm.swallow.icon").
 * @return An Evas_Object to be displayed in the part, or NULL if no content is available
 *         or if the property type is not supported for direct object creation.
 *         The caller (Genlist) is responsible for managing the returned object's lifecycle.
 */
static Evas_Object *
_item_content_get(void *data, Evas_Object *obj EINA_UNUSED, const char *part)
{
   const Eina_Value_Type *vtype;
   const char *prop;
   Eina_Value *value = NULL;
   Evas_Object *content = NULL;
   View_List_ItemData *idata = data;
   EINA_SAFETY_ON_NULL_RETURN_VAL(data, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(part, NULL);

   if (!idata->item) return NULL;

   prop = eina_hash_find(idata->priv->connect.properties, part);
   // If no property are connected, let's not try to guess randomly.
   if (!prop) return NULL;

   value = efl_model_property_get(idata->model, prop);
   if (value == NULL) return NULL;

   vtype = eina_value_type_get(value);
   if (vtype == EINA_VALUE_TYPE_BLOB)
     {
        Eina_Value_Blob out;

        eina_value_get(value, &out);
        if (out.memory != NULL)
          {
             content = elm_image_add(obj);
             //XXX: need copy memory??
             elm_image_memfile_set(content, out.memory, out.size, NULL, NULL);
          }
     }
   else if (vtype == EINA_VALUE_TYPE_FILE)
     {
        Eina_File *f = NULL;

        eina_value_get(value, &f);

        content = elm_image_add(obj);
        elm_image_mmap_set(content, f, NULL);
     }
   else if (vtype == EINA_VALUE_TYPE_OBJECT)
     {
        eina_value_get(value, &content);
     }
   else
     {
         char *str = NULL;
         str = eina_value_to_string(value);
         content = elm_icon_add(obj);
         if (elm_icon_standard_set(content, str))
           {
               evas_object_size_hint_aspect_set(content, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
           }
         else
           {
               evas_object_del(content);
               content = NULL;
           }
         free(str);
     }
   eina_value_free(value);

   return content;
}

/**
 * @brief Callback to get text for a part of a Genlist item.
 *
 * This function is called by Genlist to retrieve textual content for parts of an item
 * (e.g., "elm.text"). It fetches the corresponding property from the Efl_Model
 * based on the part name (or the part name itself if no mapping exists)
 * and converts the Eina_Value to a string.
 *
 * @param data Pointer to the View_List_ItemData for the item.
 * @param obj The parent Evas_Object (Genlist) (unused).
 * @param part The name of the part for which text is requested (e.g., "elm.text").
 * @return A newly allocated string containing the text for the part, or NULL if
 *         the property is not found or cannot be converted to a string.
 *         The caller (Genlist) is responsible for freeing the returned string.
 */
static char *
_item_text_get(void *data, Evas_Object *obj EINA_UNUSED, const char *part)
{
   Eina_Value *value = NULL;
   const char *prop;
   char *text = NULL;
   View_List_ItemData *idata = data;

   EINA_SAFETY_ON_NULL_RETURN_VAL(data, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(part, NULL);
   if (!idata->item) return NULL;

   prop = eina_hash_find(idata->priv->connect.properties, part);
   if (!prop) prop = part;

   value = efl_model_property_get(idata->model, prop);
   if (value == NULL) return NULL;

   text = eina_value_to_string(value);

   eina_value_free(value);

   return text;
}

/**
 * @brief Handles the "expand_request" event from a Genlist item.
 *
 * When a user action triggers an item expansion in the Genlist (e.g., clicking an
 * expander arrow on a tree item), this callback is invoked. It sets up
 * Efl_Model event listeners for child additions/removals on the item's model
 * and then initiates the loading of child items.
 *
 * @param data User-provided data, unused in this callback.
 * @param event The Efl_Event containing the Elm_Object_Item that requested expansion.
 */
static void
_expand_request_cb(void *data EINA_UNUSED, const Efl_Event *event)
{
   Elm_Object_Item *item = event->info;
   View_List_ItemData *idata = elm_object_item_data_get(item);

   EINA_SAFETY_ON_NULL_RETURN(idata);

   efl_event_callback_array_add(idata->model, model_callbacks(), idata);

   _efl_model_load_children(idata);
}

/**
 * @brief Handles the "contract_request" event from a Genlist item.
 *
 * When a user action triggers an item contraction in the Genlist, this callback
 * is invoked. It removes Efl_Model event listeners for child changes from the
 * item's model and sets the Genlist item's state to not expanded.
 * The actual clearing of sub-items is typically handled by the "contracted" event.
 *
 * @param data User-provided data, unused in this callback.
 * @param event The Efl_Event containing the Elm_Object_Item that requested contraction.
 */
static void
_contract_request_cb(void *data EINA_UNUSED, const Efl_Event *event)
{
   Elm_Object_Item *item = event->info;
   View_List_ItemData *idata = elm_object_item_data_get(item);

   efl_event_callback_array_del(idata->model, model_callbacks(), idata);
   elm_genlist_item_expanded_set(item, EINA_FALSE);
}

/**
 * @brief Handles the "contracted" event from a Genlist item.
 *
 * This callback is invoked after a Genlist item has been successfully contracted.
 * Its primary role here is to clear all sub-items from the Genlist item,
 * effectively removing them from the display.
 *
 * @param data User-provided data, unused in this callback.
 * @param event The Efl_Event containing the Elm_Object_Item that was contracted.
 */
static void
_contracted_cb(void *data EINA_UNUSED, const Efl_Event *event)
{
   Elm_Object_Item *glit = event->info;
   elm_genlist_item_subitems_clear(glit);
}

/**
 * @brief Callback invoked when the underlying Genlist Evas_Object is deleted.
 *
 * This function ensures proper cleanup when the Genlist widget associated with
 * the Elm_View_List is deleted. It removes event callbacks and unreferences
 * the Genlist object to prevent dangling pointers and resource leaks.
 *
 * @param data Pointer to the Elm_View_List_Data.
 * @param e The Evas canvas (unused).
 * @param obj The Evas_Object (Genlist) being deleted.
 * @param event_info Additional event information (unused).
 */
static void
_genlist_deleted(void *data, Evas *e EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Elm_View_List_Data *priv = data;

   if (priv && priv->genlist && priv->genlist == obj)
     {
        efl_event_callback_array_del(priv->genlist, genlist_callbacks(), priv);
        efl_unref(priv->genlist);
        priv->genlist = NULL;
     }
}

/* --- Efl_Model Callbacks --- */
/**
 * @brief Callback for Efl_Model's EFL_MODEL_EVENT_PROPERTIES_CHANGED.
 *
 * This function is invoked when one or more properties of an Efl_Model,
 * associated with a list item, have changed. It triggers an update
 * of the corresponding Genlist item to reflect these changes in the UI.
 *
 * @param data Pointer to the View_List_ItemData whose model's properties changed.
 * @param event The Efl_Event containing an Efl_Model_Property_Event with details
 *              about the changed properties (e.g., which properties were invalidated).
 */
static void
_efl_model_properties_change_cb(void *data, const Efl_Event *event)
{
   View_List_ItemData *idata = data;
   Efl_Model_Property_Event *evt = event->info;

   EINA_SAFETY_ON_NULL_RETURN(idata);
   EINA_SAFETY_ON_NULL_RETURN(evt);

   if (idata->item)
     elm_genlist_item_update(idata->item);
}

/**
 * @brief Processes the result of fetching children from an Efl_Model.
 *
 * This function is a `then` callback for an Eina_Future that was initiated
 * to get a slice of children from an Efl_Model. It iterates through the
 * fetched child models, creates corresponding View_List_ItemData and
 * Genlist items, and appends them to the parent Genlist item.
 *
 * @param data Pointer to the View_List_ItemData of the parent item.
 * @param v An Eina_Value containing an array (EINA_VALUE_TYPE_ARRAY) of Efl_Model
 *          objects representing the children. If an error occurred during the fetch,
 *          this will be an Eina_Value of type EINA_VALUE_TYPE_ERROR.
 *          Example of `v` structure for successful fetch:
 *          Eina_Value (Type: EINA_VALUE_TYPE_ARRAY, SubType: EFL_MODEL_TYPE)
 *          [
 *             Efl_Model* child1,
 *             Efl_Model* child2,
 *             ...
 *          ]
 * @param ev The Eina_Future that resolved to this callback (unused).
 * @return The original Eina_Value `v` is returned, to be potentially used by
 *         further chained future operations.
 */
static Eina_Value
_efl_model_load_children_then(void *data, const Eina_Value v,
                              const Eina_Future *ev EINA_UNUSED)
{
   View_List_ItemData *pdata = data;
   Elm_View_List_Data *priv = pdata->priv;
   unsigned int i, len;
   Efl_Model *child = NULL;

   if (eina_value_type_get(&v) == EINA_VALUE_TYPE_ERROR)
     goto end;

   EINA_VALUE_ARRAY_FOREACH(&v, len, i, child)
     {
        View_List_ItemData *idata = calloc(1, sizeof(View_List_ItemData));
        if (!idata) continue ;

        idata->priv = priv;
        idata->parent = pdata;
        idata->model = efl_ref(child);

        efl_event_callback_add(idata->model, EFL_MODEL_EVENT_PROPERTIES_CHANGED,
                               _efl_model_properties_change_cb, idata);

        idata->item = elm_genlist_item_append(priv->genlist, priv->itc, idata, pdata->item,
                                              priv->itype, _item_sel_cb, idata);
     }

   if (i > 0 && pdata->item)
     elm_genlist_item_expanded_set(pdata->item, EINA_TRUE);

 end:
   return v;
}

/**
 * @brief Initiates the asynchronous loading of all children for a given parent item.
 *
 * This function requests all children of the model associated with `pdata`
 * (or the root model if `pdata` represents the root). It uses `efl_model_children_slice_get`
 * to fetch the children and then chains `_efl_model_load_children_then` to process them
 * once the future resolves.
 *
 * @param pdata The View_List_ItemData of the parent item whose children are to be loaded.
 *              If this is the root item, children of the main connected model are loaded.
 */
static void
_efl_model_load_children(View_List_ItemData *pdata)
{
   Eina_Future *f;

   f = efl_model_children_slice_get(pdata->priv->connect.model, 0,
                                    efl_model_children_count_get(pdata->priv->connect.model));
   f = eina_future_then(f, _efl_model_load_children_then, pdata, NULL);
   efl_future_then(pdata->priv->genlist, f);
}

/**
 * @brief Callback for Efl_Model's EFL_MODEL_EVENT_CHILD_REMOVED.
 *
 * This function is invoked when a child model is removed from a parent model
 * that is being observed. It finds the corresponding Genlist item for the
 * removed child and deletes it from the Genlist.
 *
 * @param data Pointer to the View_List_ItemData of the parent item whose child was removed.
 * @param event The Efl_Event containing an Efl_Model_Children_Event with details
 *              about the removed child, specifically its `index`.
 */
static void
_efl_model_children_removed_cb(void *data, const Efl_Event *event)
{
   Efl_Model_Children_Event* evt = event->info;
   View_List_ItemData *idata = data;
   Elm_Object_Item *item;
   const Eina_List *subitems, *l;
   unsigned int i = 0;

   subitems = elm_genlist_item_subitems_get(idata->item);

   EINA_LIST_FOREACH(subitems, l, item)
     {
        if (i == evt->index) break ;
        i++;
     }

   if (i != evt->index) return ;
   elm_object_item_del(item);
}

/**
 * @brief Callback for Efl_Model's EFL_MODEL_EVENT_CHILD_ADDED.
 *
 * This function is invoked when a new child model is added to a parent model
 * that is being observed. It initiates an asynchronous fetch of the newly added
 * child model (using its `index` from the event) and then uses
 * `_efl_model_load_children_then` to create and add the corresponding
 * Genlist item to the view.
 *
 * @param data Pointer to the View_List_ItemData of the parent item to which a child was added.
 * @param event The Efl_Event containing an Efl_Model_Children_Event with details
 *              about the added child, specifically its `index`.
 */
static void
_efl_model_children_added_cb(void *data, const Efl_Event *event)
{
   Efl_Model_Children_Event* evt = event->info;
   View_List_ItemData *idata = data;
   Eina_Future *f;

   f = efl_model_children_slice_get(idata->priv->connect.model, evt->index, 1);
   f = eina_future_then(f, _efl_model_load_children_then, idata, NULL);
   efl_future_then(idata->priv->genlist, f);
}

/**
 * @brief Internal function to set or change the Efl_Model for the view list.
 *
 * This function handles the logic of disconnecting from an old model (if any)
 * and connecting to a new model. It clears existing items from the Genlist,
 * updates the internal model reference, sets up callbacks for model events
 * (child added/removed) on the new model, and initiates loading of the
 * top-level items from the new model.
 *
 * @param priv The private data of the Elm_View_List widget.
 * @param model The new Efl_Model to be used as the data source. Can be NULL
 *              to disconnect the current model and clear the list.
 */
static void
_priv_model_set(Elm_View_List_Data *priv, Eo *model)
{
   if (priv->connect.model)
     {
        efl_event_callback_array_del(priv->connect.model, model_callbacks(), priv->rootdata);
        elm_obj_genlist_clear(priv->genlist);
     }

   efl_replace(&priv->connect.model, model);

   if (model == NULL) return;

   priv->rootdata->model = priv->connect.model;

   efl_event_callback_array_add(priv->connect.model, model_callbacks(), priv->rootdata);
   _efl_model_load_children(priv->rootdata);
}

/**
 * @brief Elm View List Class impl.
 */
/**
 * @internal
 * @brief Sets the underlying Genlist object and its configuration for the view list.
 *
 * This function is called to initialize the Elm_View_List with a specific
 * Evas_Object (Genlist). It configures the item class for Genlist items,
 * including callbacks for text/content retrieval and item deletion.
 * It also sets up event listeners for Genlist interactions (expand/contract).
 * This is typically called by an external manager or builder that creates
 * the Genlist widget.
 *
 * @param obj The Elm_View_List_Eo instance.
 * @param priv The private data of the Elm_View_List widget.
 * @param genlist The Evas_Object (Genlist) to be used by this view list.
 * @param itype The Elm_Genlist_Item_Type for items (e.g., ELM_GENLIST_ITEM_TREE, ELM_GENLIST_ITEM_NONE).
 * @param istyle The style string for the Genlist items (e.g., "default").
 */
static void
_elm_view_list_genlist_set(Eo *obj, Elm_View_List_Data *priv, Evas_Object *genlist,
                           Elm_Genlist_Item_Type itype, const char *istyle)
{
   priv->view = obj;
   priv->genlist = genlist;
   priv->itype = itype;
   EINA_SAFETY_ON_NULL_RETURN(priv->genlist);
   efl_ref(priv->genlist);

   priv->rootdata = calloc(1, sizeof(View_List_ItemData));
   priv->rootdata->priv = priv;

   priv->itc = elm_genlist_item_class_new();
   if (istyle)
     priv->itc->item_style = strdup(istyle);
   priv->itc->func.text_get = _item_text_get;
   priv->itc->func.content_get = _item_content_get;
   priv->itc->func.state_get = NULL;
   priv->itc->func.del = _item_del;
   priv->connect.properties = eina_hash_string_superfast_new(free);

   efl_event_callback_array_add(priv->genlist, genlist_callbacks(), priv);
   evas_object_event_callback_add(priv->genlist, EVAS_CALLBACK_DEL, _genlist_deleted, priv);
}

/**
 * @internal
 * @brief Destructor for the Elm_View_List_Eo object.
 *
 * This function is called when the Elm_View_List_Eo object is being destroyed.
 * It performs necessary cleanup, such as removing model event callbacks,
 * clearing the Genlist, freeing allocated resources (item class, style string,
 * properties hash, root data), and unreferencing the model and Genlist objects.
 *
 * @param obj The Elm_View_List_Eo object being destroyed.
 * @param priv The private data of the Elm_View_List widget.
 */
static void
_elm_view_list_efl_object_destructor(Eo *obj, Elm_View_List_Data *priv)
{
   EINA_SAFETY_ON_NULL_RETURN(priv);
   EINA_SAFETY_ON_NULL_RETURN(obj);

   efl_event_callback_array_del(priv->connect.model, model_callbacks(), priv->rootdata);

   elm_obj_genlist_clear(priv->genlist);
   free((void *)priv->itc->item_style);
   elm_genlist_item_class_free(priv->itc);

   eina_hash_free(priv->connect.properties);
   free(priv->rootdata);
   priv->rootdata = NULL;

   if (priv->genlist)
     {
        evas_object_event_callback_del(priv->genlist, EVAS_CALLBACK_DEL, _genlist_deleted);
        efl_event_callback_array_del(priv->genlist, genlist_callbacks(), priv);
        efl_unref(priv->genlist);
     }

   efl_unref(priv->connect.model);

   efl_destructor(efl_super(obj, MY_CLASS));
}

/**
 * @internal
 * @brief Gets the underlying Evas_Object (Genlist) used by the view list.
 *
 * This function provides access to the actual widget (Genlist) that
 * Elm_View_List uses for rendering.
 *
 * @param obj The Elm_View_List_Eo instance (unused).
 * @param priv The private data of the Elm_View_List widget.
 * @param widget Pointer to an Evas_Object* where the Genlist widget will be stored.
 */
static void
_elm_view_list_evas_object_get(Eo *obj, Elm_View_List_Data *priv, Evas_Object **widget)
{
   EINA_SAFETY_ON_NULL_RETURN(priv);
   EINA_SAFETY_ON_NULL_RETURN(obj);
   EINA_SAFETY_ON_NULL_RETURN(widget);

   *widget = priv->genlist;
}

/**
 * @internal
 * @brief Connects an Efl_Model property to a UI part name.
 *
 * This function establishes a mapping between a named property of the Efl_Model
 * and a named part of the Genlist item's theme/EDC file (e.g., "elm.text", "elm.swallow.icon").
 * When Genlist requests text or content for a part, this mapping is used to
 * determine which model property to query.
 *
 * @param obj The Elm_View_List_Eo instance (unused).
 * @param priv The private data of the Elm_View_List widget.
 * @param property The name of the property in the Efl_Model.
 * @param part The name of the UI part in the Genlist item (e.g., "elm.text").
 */
static void
_elm_view_list_property_connect(Eo *obj EINA_UNUSED, Elm_View_List_Data *priv,
                                const char *property, const char *part)
{
   EINA_SAFETY_ON_NULL_RETURN(priv);

   EINA_SAFETY_ON_NULL_RETURN(priv->connect.properties);
   EINA_SAFETY_ON_NULL_RETURN(property);
   EINA_SAFETY_ON_NULL_RETURN(part);

   free(eina_hash_set(priv->connect.properties, part, strdup(property)));
}

/**
 * @internal
 * @brief Sets the Efl_Model for the view list.
 *
 * This is the Eolian implementation for setting the model. It calls the
 * internal `_priv_model_set` function to handle the actual logic.
 *
 * @param obj The Elm_View_List_Eo instance (unused).
 * @param priv The private data of the Elm_View_List widget.
 * @param model The Efl_Model to be set as the data source.
 */
static void
_elm_view_list_model_set(Eo *obj EINA_UNUSED, Elm_View_List_Data *priv, Efl_Model *model)
{
   _priv_model_set(priv, model);
}

/**
 * @internal
 * @brief Gets the Efl_Model currently used by the view list.
 *
 * This is the Eolian implementation for getting the model.
 *
 * @param obj The Elm_View_List_Eo instance (unused).
 * @param priv The private data of the Elm_View_List widget.
 * @return The current Efl_Model, or NULL if no model is set.
 */
static Efl_Model *
_elm_view_list_model_get(const Eo *obj EINA_UNUSED, Elm_View_List_Data *priv)
{
   return priv->connect.model;
}
#include "elm_view_list_eo.c"
