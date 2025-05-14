#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include <Elementary.h>
#include <stdint.h>
#include "elm_priv.h"
#include "elm_icon_eo.h"
#include "elm_widget_menu.h"
#include "elm_widget_icon.h"

#define DBUS_PATH           "/com/canonical/dbusmenu"
#define DBUS_INTERFACE      "com.canonical.dbusmenu"
#define DBUS_MENU_VERSION   3u

#define REGISTRAR_NAME      "com.canonical.AppMenu.Registrar"
#define REGISTRAR_PATH      "/com/canonical/AppMenu/Registrar"
#define REGISTRAR_INTERFACE REGISTRAR_NAME

#define DBUS_DATA_KEY       "_Elm_DBus_Menu"

typedef struct _Callback_Data Callback_Data;

struct _Elm_DBus_Menu
{
   Eo                      *menu;
   Eldbus_Connection        *bus;
   Eldbus_Service_Interface *iface;
   unsigned                 timestamp;
   Eina_Hash               *elements;
   Ecore_Idler             *signal_idler;
   Callback_Data           *app_menu_data;
};

static const Eldbus_Service_Interface_Desc _interface;
static unsigned last_object_path;

typedef enum _Elm_DBus_Property
{
   ELM_DBUS_PROPERTY_LABEL,
   ELM_DBUS_PROPERTY_CHILDREN_DISPLAY,
   ELM_DBUS_PROPERTY_ENABLED,
   ELM_DBUS_PROPERTY_TYPE,
   ELM_DBUS_PROPERTY_ICON_NAME,
   ELM_DBUS_PROPERTY_UNKNOWN,
} Elm_DBus_Property;

enum
{
   ELM_DBUS_SIGNAL_LAYOUT_UPDATED,
   ELM_DBUS_SIGNAL_ITEM_ACTIVATION_REQUESTED,
};

struct _Callback_Data
{
   void (*result_cb)(Eina_Bool, void *);
   void          *data;
   Eldbus_Pending *pending_register;
   Ecore_X_Window xid;
};

/**
 * @brief Recursively traverses menu items and adds them to the D-Bus menu hash.
 *
 * This function assigns a unique integer ID to each menu item and stores it in
 * a hash table for quick lookup. The ID is used to reference the item in
 * D-Bus calls.
 *
 * @param dbus_menu The D-Bus menu instance.
 * @param item The menu item to start traversal from.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
_menu_add_recursive(Elm_DBus_Menu *dbus_menu, Elm_Menu_Item_Data *item)
{
   int32_t id;
   Eina_List *l;
   Elm_Object_Item *obj_subitem;

   id = ++dbus_menu->timestamp;
   if (!eina_hash_add(dbus_menu->elements, &id, item))
     return EINA_FALSE;

   item->dbus_idx = id;

   EINA_LIST_FOREACH (item->submenu.items, l, obj_subitem)
     {
        ELM_MENU_ITEM_DATA_GET(obj_subitem, subitem);
        if (!_menu_add_recursive(dbus_menu, subitem))
          return EINA_FALSE;
     }

   return EINA_TRUE;
}

/**
 * @brief Callback for the result of the RegisterWindow D-Bus call.
 *
 * This is called asynchronously after attempting to register a window with the
 * AppMenu Registrar. It invokes the user-provided result callback.
 *
 * @param data The Elm_DBus_Menu instance.
 * @param msg The reply message from D-Bus.
 * @param pending The pending D-Bus call.
 */
static void
_app_register_cb(void *data, const Eldbus_Message *msg,
                 Eldbus_Pending *pending EINA_UNUSED)
{
   Elm_DBus_Menu *menu = data;
   Callback_Data *cd = menu->app_menu_data;
   Eina_Bool result;
   const char *error_name;

   cd->pending_register = NULL;

   result = !eldbus_message_error_get(msg, &error_name, NULL);
   if (!result && !strcmp(error_name, ELDBUS_ERROR_PENDING_CANCELED))
     {
        DBG("Register canceled");
        return;
     }

   if (cd->result_cb) cd->result_cb(result, cd->data);
}

/**
 * @brief Callback for monitoring the AppMenu Registrar on D-Bus.
 *
 * This function is triggered when the AppMenu Registrar service becomes
 * available or disappears. If it becomes available (`new_id` is not empty),
 * it calls RegisterWindow. If it disappears, it notifies the user via
 * the result callback.
 *
 * @param data The Elm_DBus_Menu instance.
 * @param bus The bus name being watched (unused).
 * @param old_id The old owner of the name (unused).
 * @param new_id The new owner of the name. An empty string means the name
 *               has no owner.
 */
static void
_app_menu_watch_cb(void *data, const char *bus EINA_UNUSED,
                   const char *old_id EINA_UNUSED, const char *new_id)
{
   Elm_DBus_Menu *menu = data;
   Callback_Data *cd = menu->app_menu_data;
   Eldbus_Message *msg;
   const char *obj_path;

   if (!strcmp(new_id, ""))
     {
        if (cd->pending_register) eldbus_pending_cancel(cd->pending_register);

        if (cd->result_cb) cd->result_cb(EINA_FALSE, cd->data);
     }
   else
     {
        msg = eldbus_message_method_call_new(REGISTRAR_NAME, REGISTRAR_PATH,
                                            REGISTRAR_INTERFACE,
                                            "RegisterWindow");
        obj_path = eldbus_service_object_path_get(menu->iface);
        eldbus_message_arguments_append(msg, "uo", (unsigned)cd->xid,
                                       obj_path);
        cd->pending_register = eldbus_connection_send(menu->bus, msg,
                                                     _app_register_cb, data, -1);
     }
}

/**
 * @brief Idler function to emit the LayoutUpdated D-Bus signal.
 *
 * Using an idler ensures that multiple layout changes within a single main
 * loop iteration result in only one D-Bus signal being emitted, avoiding
 * unnecessary traffic.
 *
 * @param data The Elm_DBus_Menu instance.
 * @return ECORE_CALLBACK_CANCEL to automatically remove the idler after it runs.
 */
static Eina_Bool
_layout_idler(void *data)
{
   Elm_DBus_Menu *dbus_menu = data;

   eldbus_service_signal_emit(dbus_menu->iface, ELM_DBUS_SIGNAL_LAYOUT_UPDATED,
                             dbus_menu->timestamp, 0);

   dbus_menu->signal_idler = NULL;
   return ECORE_CALLBACK_CANCEL;
}

/**
 * @brief Schedules the layout idler to be called.
 *
 * This function ensures that the idler is only added once, even if this
 * function is called multiple times before the idler has a chance to run.
 *
 * @param dbus_menu The D-Bus menu instance.
 */
static void
_layout_signal(Elm_DBus_Menu *dbus_menu)
{
   if (!dbus_menu->bus) return;
   if (dbus_menu->signal_idler) return;

   dbus_menu->signal_idler = ecore_idler_add(_layout_idler, dbus_menu);
}

/**
 * @brief Converts a property name string to its corresponding enum value.
 *
 * @param str The property name as a string (e.g., "label", "enabled").
 * @return The Elm_DBus_Property enum value, or ELM_DBUS_PROPERTY_UNKNOWN
 *         if the string is not a recognized property.
 */
static Elm_DBus_Property
_str_to_property(const char *str)
{
   if (!strcmp(str, "label"))
     return ELM_DBUS_PROPERTY_LABEL;
   else if (!strcmp(str, "children-display"))
     return ELM_DBUS_PROPERTY_CHILDREN_DISPLAY;
   else if (!strcmp(str, "enabled"))
     return ELM_DBUS_PROPERTY_ENABLED;
   else if (!strcmp(str, "type"))
     return ELM_DBUS_PROPERTY_TYPE;
   else if (!strcmp(str, "icon-name"))
     return ELM_DBUS_PROPERTY_ICON_NAME;

   return ELM_DBUS_PROPERTY_UNKNOWN;
}

/**
 * @brief Checks if a menu item has a freedesktop-compliant icon.
 *
 * D-Bus menus primarily work with icon names that follow the freedesktop.org
 * icon naming specification. This function verifies if the item's icon is of
 * that type.
 *
 * @param item The menu item to check.
 * @return EINA_TRUE if the item has a freedesktop icon, EINA_FALSE otherwise.
 */
static Eina_Bool
_freedesktop_icon_exists(Elm_Menu_Item_Data *item)
{
   if (!item->icon_str) return EINA_FALSE;

   ELM_ICON_CHECK(item->content) EINA_FALSE;

   ELM_ICON_DATA_GET(item->content, sd);
   if (sd->freedesktop.use) return EINA_TRUE;

   return EINA_FALSE;
}

/**
 * @brief Checks if a specific D-Bus property is applicable to a given menu item.
 *
 * For example, a separator item only has a "type" property, while a regular
 * item might have "label", "enabled", etc. This function determines if a
 * property makes sense for an item before trying to retrieve its value.
 *
 * @param item The menu item.
 * @param property The D-Bus property to check.
 * @return EINA_TRUE if the property exists for the item, EINA_FALSE otherwise.
 */
static Eina_Bool
_property_exists(Elm_Menu_Item_Data *item,
                 Elm_DBus_Property property)
{
   if (item->separator)
     {
        if (property == ELM_DBUS_PROPERTY_TYPE) return EINA_TRUE;
        return EINA_FALSE;
     }

   switch (property)
     {
      case ELM_DBUS_PROPERTY_LABEL:
        // Allow _property_append to handle the label
        return EINA_TRUE;

      case ELM_DBUS_PROPERTY_CHILDREN_DISPLAY:
        if (eina_list_count(item->submenu.items)) return EINA_TRUE;
        return EINA_FALSE;

      case ELM_DBUS_PROPERTY_ENABLED:
        return elm_object_item_disabled_get(EO_OBJ(item));

      case ELM_DBUS_PROPERTY_ICON_NAME:
        return _freedesktop_icon_exists(item);

      case ELM_DBUS_PROPERTY_TYPE:
      case ELM_DBUS_PROPERTY_UNKNOWN:
        return EINA_FALSE;
     }

   return EINA_FALSE;
}

/**
 * @brief Appends a single property (as a D-Bus variant) to a message iterator.
 *
 * This function retrieves the value for a given property from a menu item
 * and appends it to the D-Bus message being constructed. It assumes that
 * _property_exists() has already been called to verify the property is valid
 * for this item.
 *
 * @param item The menu item from which to get the property value.
 * @param property The property to append.
 * @param iter The D-Bus message iterator to append to.
 */
static void
_property_append(Elm_Menu_Item_Data *item,
                 Elm_DBus_Property property,
                 Eldbus_Message_Iter *iter)
{
   Eldbus_Message_Iter *variant = NULL;
   const char *t;

   switch (property)
     {
      case ELM_DBUS_PROPERTY_LABEL:
        variant = eldbus_message_iter_container_new(iter, 'v', "s");
        t = elm_object_item_part_text_get(EO_OBJ(item), NULL);
        if (!t)
          {
             t = elm_object_part_text_get(item->content, NULL);
             if (!t) t = "";
          }

        eldbus_message_iter_basic_append(variant, 's', t);
        break;

      case ELM_DBUS_PROPERTY_CHILDREN_DISPLAY:
        variant = eldbus_message_iter_container_new(iter, 'v', "s");
        eldbus_message_iter_basic_append(variant, 's', "submenu");
        break;

      case ELM_DBUS_PROPERTY_ENABLED:
        variant = eldbus_message_iter_container_new(iter, 'v', "b");
        eldbus_message_iter_basic_append(variant, 'b', EINA_FALSE);
        break;

      case ELM_DBUS_PROPERTY_TYPE:
        variant = eldbus_message_iter_container_new(iter, 'v', "s");
        eldbus_message_iter_basic_append(variant, 's', "separator");
        break;

      case ELM_DBUS_PROPERTY_ICON_NAME:
        variant = eldbus_message_iter_container_new(iter, 'v', "s");
        eldbus_message_iter_basic_append(variant, 's', item->icon_str);
        break;

      case ELM_DBUS_PROPERTY_UNKNOWN:
        ERR("Invalid code path");
        return;
     }

   eldbus_message_iter_container_close(iter, variant);
}

/**
 * @brief Constructs a D-Bus dictionary of properties for a menu item.
 *
 * Iterates through a list of requested property names, checks if each property
 * exists for the given item, and if so, appends it to a D-Bus dictionary
 * (a{sv}).
 *
 * @param item The menu item.
 * @param property_list A list of strings, where each string is a property name.
 * @param iter The D-Bus message iterator to append the dictionary to.
 */
static void
_property_dict_build(Elm_Menu_Item_Data *item,
                     Eina_List *property_list, Eldbus_Message_Iter *iter)
{
   char *propstr;
   Elm_DBus_Property property;
   Eldbus_Message_Iter *array, *pair;
   Eina_List *l;

   array = eldbus_message_iter_container_new(iter, 'a', "{sv}");

   EINA_LIST_FOREACH (property_list, l, propstr)
     {
        property = _str_to_property(propstr);

        if (property == ELM_DBUS_PROPERTY_UNKNOWN) continue;
        if (!_property_exists(item, property)) continue;

        pair = eldbus_message_iter_container_new(array, 'e', NULL);
        eldbus_message_iter_basic_append(pair, 's', propstr);
        _property_append(item, property, pair);
        eldbus_message_iter_container_close(array, pair);
     }

   eldbus_message_iter_container_close(iter, array);
}

/**
 * @brief Recursively builds the layout structure for a menu item and its children.
 *
 * This function generates the D-Bus structure for a single menu item, including
 * its properties and any sub-menus, up to a specified recursion depth.
 * The D-Bus layout for an item is a struct: (ia{sv}av).
 * - i: The integer ID of the menu item.
 * - a{sv}: A dictionary of properties (e.g., "label", "enabled").
 * - av: An array of variants, where each variant is a struct for a child item.
 *
 * @param item The menu item to process.
 * @param property_list The list of property names to include.
 * @param recursion_depth How many levels of sub-menus to include. A value of 0
 *                        means only the current item is processed.
 * @param iter The D-Bus message iterator to append the layout to.
 */
static void
_layout_build_recursive(Elm_Menu_Item_Data *item,
                        Eina_List *property_list, unsigned recursion_depth,
                        Eldbus_Message_Iter *iter)
{
   Eina_List *l;
   Eldbus_Message_Iter *layout, *array, *variant;
   Elm_Object_Item *obj_subitem;

   layout = eldbus_message_iter_container_new(iter, 'r', NULL);
   eldbus_message_iter_basic_append(layout, 'i', item->dbus_idx);
   _property_dict_build(item, property_list, layout);
   array = eldbus_message_iter_container_new(layout, 'a', "v");

   if (recursion_depth > 0)
     {
        EINA_LIST_FOREACH (item->submenu.items, l, obj_subitem)
          {
             variant = eldbus_message_iter_container_new(array, 'v',
                                                        "(ia{sv}av)");
             ELM_MENU_ITEM_DATA_GET(obj_subitem, subitem);
             _layout_build_recursive(subitem, property_list,
                                     recursion_depth - 1, variant);
             eldbus_message_iter_container_close(array, variant);
          }
     }

   eldbus_message_iter_container_close(layout, array);
   eldbus_message_iter_container_close(iter, layout);
}

/**
 * @brief Builds the layout structure for the root of the menu.
 *
 * The root (parent ID 0) is a special case. It doesn't represent a real menu
 * item but acts as a container for the top-level items. This function
 * constructs its layout and then calls _layout_build_recursive() for each
 * top-level item.
 *
 * @param dbus_menu The D-Bus menu instance.
 * @param property_list The list of property names to include.
 * @param recursion_depth The recursion depth for child items.
 * @param iter The D-Bus message iterator.
 */
static void
_root_layout_build(Elm_DBus_Menu *dbus_menu, Eina_List *property_list,
                   unsigned recursion_depth, Eldbus_Message_Iter *iter)
{
   char *property;
   Eldbus_Message_Iter *layout, *array, *pair, *variant;
   const Eina_List *l, *it;
   Elm_Object_Item *obj_item;

   layout = eldbus_message_iter_container_new(iter, 'r', NULL);
   eldbus_message_iter_basic_append(layout, 'i', 0);
   array = eldbus_message_iter_container_new(layout, 'a', "{sv}");

   EINA_LIST_FOREACH (property_list, l, property)
     {
        if (!strcmp(property, "children-display"))
          {
             pair = eldbus_message_iter_container_new(array, 'e', NULL);
             eldbus_message_iter_basic_append(pair, 's', property);
             variant = eldbus_message_iter_container_new(pair, 'v', "s");
             eldbus_message_iter_basic_append(variant, 's', "submenu");
             eldbus_message_iter_container_close(pair, variant);
             eldbus_message_iter_container_close(array, pair);
             break;
          }
     }

   eldbus_message_iter_container_close(layout, array);
   array = eldbus_message_iter_container_new(layout, 'a', "v");

   if (recursion_depth > 0)
     {
        it = elm_menu_items_get(dbus_menu->menu);
        EINA_LIST_FOREACH (it, l, obj_item)
          {
             variant = eldbus_message_iter_container_new(array, 'v',
                                                         "(ia{sv}av)");
             ELM_MENU_ITEM_DATA_GET(obj_item, item);
             _layout_build_recursive(item, property_list,
                                     recursion_depth - 1, variant);
             eldbus_message_iter_container_close(array, variant);
          }
     }

   eldbus_message_iter_container_close(layout, array);
   eldbus_message_iter_container_close(iter, layout);
}

/**
 * @brief Ensures the property list is not empty.
 *
 * If the client requests layout information without specifying any properties,
 * the D-Bus specification implies that a default set of common properties
 * should be returned. This function populates the list with these defaults
 * if it's empty.
 *
 * @param property_list The list of properties, which may be empty.
 * @return The original list, or a new list containing default properties if
 *         the original was empty. The caller is responsible for freeing the list.
 */
static Eina_List *
_empty_properties_handle(Eina_List *property_list)
{
   if (!eina_list_count(property_list))
     {
        property_list = eina_list_append(property_list, "label");
        property_list = eina_list_append(property_list, "children-display");
        property_list = eina_list_append(property_list, "enabled");
        property_list = eina_list_append(property_list, "type");
        property_list = eina_list_append(property_list, "icon-name");
     }
   return property_list;
}

/**
 * @brief Handles a single D-Bus menu event, such as 'clicked'.
 *
 * This function parses an event from a D-Bus message, finds the corresponding
 * menu item, and triggers the appropriate action (e.g., simulates a click).
 *
 * @param dbus_menu The D-Bus menu instance.
 * @param iter An iterator positioned at the start of the event data.
 *             The expected signature is (isvu).
 * @param[out] error_id If the item ID is invalid, it is written to this pointer.
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., invalid item ID).
 */
static Eina_Bool
_event_handle(Elm_DBus_Menu *dbus_menu, Eldbus_Message_Iter *iter, int *error_id)
{
   Elm_Menu_Item_Data *item;
   const char *event;
   int id;
   int32_t i;
   Eldbus_Message_Iter *data;
   unsigned *timestamp;

   if (!eldbus_message_iter_arguments_get(iter, "isvu", &id, &event, &data,
                                          &timestamp))
     return EINA_FALSE;
   i = id;
   item = eina_hash_find(dbus_menu->elements, &i);
   if (!item)
     {
        if (error_id) *error_id = id;
        return EINA_FALSE;
     }

   if (!strcmp(event, "clicked"))
     _elm_dbus_menu_item_select_cb(EO_OBJ(item));

   return EINA_TRUE;
}

/**
 * @brief Creates and initializes a D-Bus menu representation from an Elm_Menu widget.
 *
 * This function allocates the Elm_DBus_Menu structure and populates its internal
 * hash table by recursively scanning all items in the provided menu widget.
 * It does not yet connect to D-Bus or register any interfaces.
 *
 * @param menu The Elm_Menu widget.
 * @return A new Elm_DBus_Menu instance, or NULL on failure.
 */
static Elm_DBus_Menu *
_elm_dbus_menu_add(Eo *menu)
{
   Elm_DBus_Menu *dbus_menu;
   Elm_Object_Item *obj_item;
   const Eina_List *it, *l;

   ELM_MENU_CHECK(menu) NULL;

   dbus_menu = calloc(1, sizeof(Elm_DBus_Menu));
   if (!dbus_menu)
     {
        ERR("Unable to allocate D-Bus data");
        return NULL;
     }

   dbus_menu->elements = eina_hash_int32_new(NULL);
   if (!dbus_menu->elements)
     {
        ERR("Unable to allocate hash table");
        goto error_menu;
     }

   dbus_menu->menu = menu;

   it = elm_menu_items_get(menu);
   EINA_LIST_FOREACH(it, l, obj_item)
     {
        ELM_MENU_ITEM_DATA_GET(obj_item, item);
        if (!_menu_add_recursive(dbus_menu, item))
          {
             ERR("Unable to add menu item");
             goto error_hash;
          }
     }

   return dbus_menu;

error_hash:
   eina_hash_free(dbus_menu->elements);
error_menu:
   free(dbus_menu);
   return NULL;
}

// =============================================================================
//                            com.canonical.dbusmenu
// =============================================================================
// =============================================================================
// Methods
// =============================================================================
/**
 * @brief Implements the GetLayout D-Bus method.
 *
 * This method is called by clients to retrieve the structure of the menu.
 * It can return the entire menu tree or a specific sub-tree.
 *
 * @param iface The service interface that received the call.
 * @param msg The incoming D-Bus message.
 * @return A new D-Bus message containing the layout reply or an error.
 */
static Eldbus_Message *
_method_layout_get(const Eldbus_Service_Interface *iface,
                   const Eldbus_Message *msg)
{
   int parent_id;
   int32_t id;
   int r;
   unsigned recursion_depth;
   char *property;
   Eina_List *property_list = NULL;
   Eldbus_Message *reply;
   Eldbus_Message_Iter *iter, *array;
   Elm_DBus_Menu *dbus_menu;
   Elm_Menu_Item_Data *item = NULL;

   dbus_menu = eldbus_service_object_data_get(iface, DBUS_DATA_KEY);

   if (!eldbus_message_arguments_get(msg, "iias", &parent_id, &r, &array))
     ERR("Invalid arguments in D-Bus message");

   recursion_depth = r;

   while (eldbus_message_iter_get_and_next(array, 's', &property))
     property_list = eina_list_append(property_list, property);

   property_list = _empty_properties_handle(property_list);

   if (parent_id)
     {
        id = parent_id;
        item = eina_hash_find(dbus_menu->elements, &id);
        if (!item)
          {
             reply = eldbus_message_error_new(msg, DBUS_INTERFACE ".Error",
                                             "Invalid parent");
             return reply;
          }
     }

   reply = eldbus_message_method_return_new(msg);
   iter = eldbus_message_iter_get(reply);
   eldbus_message_iter_basic_append(iter, 'u', dbus_menu->timestamp);

   if (parent_id)
     _layout_build_recursive(item, property_list, recursion_depth, iter);
   else
     _root_layout_build(dbus_menu, property_list, recursion_depth, iter);

   eina_list_free(property_list);
   return reply;
}

/**
 * @brief Implements the GetGroupProperties D-Bus method.
 *
 * Retrieves a list of specified properties for a list of specified menu items.
 * This is more efficient than calling GetProperty for each item individually.
 *
 * @param iface The service interface.
 * @param msg The incoming D-Bus message.
 * @return A new D-Bus message containing the properties or an error.
 */
static Eldbus_Message *
_method_group_properties_get(const Eldbus_Service_Interface *iface,
                             const Eldbus_Message *msg)
{
   Eina_Iterator *hash_iter;
   Eldbus_Message *reply;
   Eldbus_Message_Iter *ids, *property_names;
   Eldbus_Message_Iter *iter, *array, *tuple;
   Eina_List *property_list = NULL;
   Elm_DBus_Menu *dbus_menu;
   Elm_Menu_Item_Data *item;
   char *property;
   int id;
   int32_t i;
   void *data;

   dbus_menu = eldbus_service_object_data_get(iface, DBUS_DATA_KEY);

   if (!eldbus_message_arguments_get(msg, "aias", &ids, &property_names))
     ERR("Invalid arguments in D-Bus message");

   while (eldbus_message_iter_get_and_next(property_names, 's', &property))
     property_list = eina_list_append(property_list, property);

   property_list = _empty_properties_handle(property_list);

   reply = eldbus_message_method_return_new(msg);
   iter = eldbus_message_iter_get(reply);
   array = eldbus_message_iter_container_new(iter, 'a', "(ia{sv})");

   if (!eldbus_message_iter_get_and_next(ids, 'i', &id))
     {
        hash_iter = eina_hash_iterator_data_new(dbus_menu->elements);

        while (eina_iterator_next(hash_iter, &data))
          {
             item = data;
             tuple = eldbus_message_iter_container_new(array, 'r', NULL);
             eldbus_message_iter_basic_append(tuple, 'i', item->dbus_idx);
             _property_dict_build(item, property_list, tuple);
             eldbus_message_iter_container_close(array, tuple);
          }

        eina_iterator_free(hash_iter);
     }
   else
     do
       {
          i = id;
          item = eina_hash_find(dbus_menu->elements, &i);
          if (!item) continue;

          tuple = eldbus_message_iter_container_new(array, 'r', NULL);
          eldbus_message_iter_basic_append(tuple, 'i', item->dbus_idx);
          _property_dict_build(item, property_list, tuple);
          eldbus_message_iter_container_close(array, tuple);
       }
     while (eldbus_message_iter_get_and_next(ids, 'i', &id));

   eldbus_message_iter_container_close(iter, array);
   eina_list_free(property_list);

   return reply;
}

/**
 * @brief Implements the GetProperty D-Bus method.
 *
 * Retrieves a single property for a single menu item.
 *
 * @param iface The service interface.
 * @param msg The incoming D-Bus message.
 * @return A new D-Bus message containing the property value or an error.
 */
static Eldbus_Message *
_method_property_get(const Eldbus_Service_Interface *iface,
                     const Eldbus_Message *msg)
{
   Eldbus_Message *reply;
   Eldbus_Message_Iter *iter, *variant;
   Elm_DBus_Property property;
   Elm_DBus_Menu *dbus_menu;
   Elm_Menu_Item_Data *item;
   int id;
   int32_t i;
   char *name;

   dbus_menu = eldbus_service_object_data_get(iface, DBUS_DATA_KEY);

   if (!eldbus_message_arguments_get(msg, "is", &id, &name))
     ERR("Invalid arguments in D-Bus message");

   property = _str_to_property(name);

   if (property == ELM_DBUS_PROPERTY_UNKNOWN)
     {
        reply = eldbus_message_error_new(msg, DBUS_INTERFACE ".Error",
                                        "Property not found");
        return reply;
     }

   if (!id)
     {
        if (property != ELM_DBUS_PROPERTY_CHILDREN_DISPLAY)
          reply = eldbus_message_error_new(msg, DBUS_INTERFACE ".Error",
                                          "Property not found");
        else
          {
             reply = eldbus_message_method_return_new(msg);
             iter = eldbus_message_iter_get(reply);
             variant = eldbus_message_iter_container_new(iter, 'v', "s");
             eldbus_message_iter_basic_append(variant, 's', "submenu");
             eldbus_message_iter_container_close(iter, variant);
          }

        return reply;
     }

   i = id;
   item = eina_hash_find(dbus_menu->elements, &i);

   if (!item)
     {
        reply = eldbus_message_error_new(msg, DBUS_INTERFACE ".Error",
                                        "Invalid menu identifier");
        return reply;
     }

   if (!_property_exists(item, property))
     {
        reply = eldbus_message_error_new(msg, DBUS_INTERFACE ".Error",
                                        "Property not found");
        return reply;
     }

   reply = eldbus_message_method_return_new(msg);
   iter = eldbus_message_iter_get(reply);
   _property_append(item, property, iter);

   return reply;
}

/**
 * @brief Implements the Event D-Bus method.
 *
 * This is called by the client to notify the application of an event, such as
 * a menu item being clicked.
 *
 * @param iface The service interface.
 * @param msg The incoming D-Bus message.
 * @return A new D-Bus message for method return (or an error).
 */
static Eldbus_Message *
_method_event(const Eldbus_Service_Interface *iface,
              const Eldbus_Message *msg)
{
   Elm_DBus_Menu *dbus_menu;
   Eldbus_Message *reply;

   dbus_menu = eldbus_service_object_data_get(iface, DBUS_DATA_KEY);

   if (!_event_handle(dbus_menu, eldbus_message_iter_get(msg), NULL))
     reply = eldbus_message_error_new(msg, DBUS_INTERFACE ".Error",
                                     "Invalid menu");
   else
     reply = eldbus_message_method_return_new(msg);

   return reply;
}

/**
 * @brief Implements the EventGroup D-Bus method.
 *
 * A batch version of Event, allowing multiple events to be sent in a single
 * call.
 *
 * @param iface The service interface.
 * @param msg The incoming D-Bus message.
 * @return A new D-Bus message for method return (or an error).
 */
static Eldbus_Message *
_method_event_group(const Eldbus_Service_Interface *iface,
                    const Eldbus_Message *msg)
{
   Eldbus_Message *reply;
   Eldbus_Message_Iter *iter, *array, *tuple, *errors;
   int id;
   Elm_DBus_Menu *dbus_menu;
   Eina_Bool return_error = EINA_TRUE;

   dbus_menu = eldbus_service_object_data_get(iface, DBUS_DATA_KEY);

   if (!eldbus_message_arguments_get(msg, "a(isvu)", &array))
     ERR("Invalid arguments in D-Bus message");

   reply = eldbus_message_method_return_new(msg);
   iter = eldbus_message_iter_get(reply);
   errors = eldbus_message_iter_container_new(iter, 'a', "i");

   while (eldbus_message_iter_get_and_next(array, 'r', &tuple))
     {
        id = 0;
        if (_event_handle(dbus_menu, tuple, &id))
          return_error = EINA_FALSE;
        else
          eldbus_message_iter_basic_append(errors, 'i', id);
     }

   if (return_error)
     {
        eldbus_message_unref(reply);
        reply = eldbus_message_error_new(msg, DBUS_INTERFACE ".Error",
                                        "Invalid menu identifiers");
     }
   else
     eldbus_message_iter_container_close(iter, errors);

   return reply;
}

/**
 * @brief Implements the AboutToShow D-Bus method.
 *
 * This method is called by the client just before a submenu is displayed.
 * It allows the application to dynamically update the submenu if needed.
 * This implementation currently just returns true, indicating no update is needed.
 *
 * @param iface The service interface (unused).
 * @param msg The incoming D-Bus message.
 * @return A new D-Bus message reply.
 */
static Eldbus_Message *
_method_about_to_show(const Eldbus_Service_Interface *iface EINA_UNUSED,
                      const Eldbus_Message *msg)
{
   Eldbus_Message *reply = eldbus_message_method_return_new(msg);
   eldbus_message_arguments_append(reply, "b", EINA_TRUE);

   return reply;
}

/**
 * @brief Implements the AboutToShowGroup D-Bus method.
 *
 * A batch version of AboutToShow. This implementation currently does nothing
 * and returns empty lists.
 *
 * @param iface The service interface (unused).
 * @param msg The incoming D-Bus message.
 * @return A new D-Bus message reply.
 */
static Eldbus_Message *
_method_about_to_show_group(const Eldbus_Service_Interface *iface EINA_UNUSED,
                            const Eldbus_Message *msg)
{
   Eldbus_Message *reply = eldbus_message_method_return_new(msg);
   Eldbus_Message_Iter *iter, *array;

   iter = eldbus_message_iter_get(reply);
   array = eldbus_message_iter_container_new(iter, 'a', "i");
   eldbus_message_iter_container_close(iter, array);
   array = eldbus_message_iter_container_new(iter, 'a', "i");
   eldbus_message_iter_container_close(iter, array);

   return reply;
}

static const Eldbus_Method _methods[] = {
   {
      "GetLayout",
      ELDBUS_ARGS({"i", "parentId"},
                 {"i", "recursionDepth"},
                 {"as", "propertyNames"}),
      ELDBUS_ARGS({"u", "revision"}, {"(ia{sv}av)", "layout"}),
      _method_layout_get,
      0
   },
   {
      "GetGroupProperties",
      ELDBUS_ARGS({"ai", "ids"}, {"as", "propertyNames"}),
      ELDBUS_ARGS({"a(ia{sv})", "properties"}),
      _method_group_properties_get,
      0
   },
   {
      "GetProperty",
      ELDBUS_ARGS({"i", "id"}, {"s", "name"}),
      ELDBUS_ARGS({"v", "value"}),
      _method_property_get,
      0
   },
   {
      "Event",
      ELDBUS_ARGS({"i", "id"},
                 {"s", "eventId"},
                 {"v", "data"},
                 {"u", "timestamp"}),
      NULL,
      _method_event,
      0
   },
   {
      "EventGroup",
      ELDBUS_ARGS({"a(isvu)", "events"}),
      ELDBUS_ARGS({"ai", "idErrors"}),
      _method_event_group,
      0
   },
   {
      "AboutToShow",
      ELDBUS_ARGS({"i", "id"}),
      ELDBUS_ARGS({"b", "needUpdate"}),
      _method_about_to_show,
      0
   },
   {
      "AboutToShowGroup",
      ELDBUS_ARGS({"ai", "ids"}),
      ELDBUS_ARGS({"ai", "updatesNeeded"}, {"ai", "idErrors"}),
      _method_about_to_show_group,
      0
   },

   {NULL, NULL, NULL, NULL, 0}
};

// =============================================================================
// Signals
// =============================================================================
static const Eldbus_Signal _signals[] = {
   [ELM_DBUS_SIGNAL_LAYOUT_UPDATED] = {
      "LayoutUpdated", ELDBUS_ARGS({"u", "revision"}, {"i", "parent"}), 0
   },
   [ELM_DBUS_SIGNAL_ITEM_ACTIVATION_REQUESTED] = {
      "ItemActivationRequested", ELDBUS_ARGS({"i", "id"}, {"u", "timestamp"}), 0
   },
   {NULL, NULL, 0}
};

// =============================================================================
// Properties
// =============================================================================
/**
 * @brief Getter for the 'Version' D-Bus property.
 *
 * Returns the version of the D-Bus menu protocol supported.
 *
 * @param iface The service interface (unused).
 * @param propname The property name (unused).
 * @param iter The iterator to append the value to.
 * @param request_msg The original request message (unused).
 * @param error D-Bus error to be set on failure (unused).
 * @return EINA_TRUE on success.
 */
static Eina_Bool
_prop_version_get(const Eldbus_Service_Interface *iface EINA_UNUSED,
                  const char *propname EINA_UNUSED,
                  Eldbus_Message_Iter *iter,
                  const Eldbus_Message *request_msg EINA_UNUSED,
                  Eldbus_Message **error EINA_UNUSED)
{
   eldbus_message_iter_basic_append(iter, 'u', DBUS_MENU_VERSION);

   return EINA_TRUE;
}

/**
 * @brief Getter for the 'TextDirection' D-Bus property.
 *
 * Returns whether the application is in a right-to-left ("rtl") or
 * left-to-right ("ltr") mode.
 *
 * @param iface Unused.
 * @param propname Unused.
 * @param iter The iterator to append the value to.
 * @param request_msg Unused.
 * @param error Unused.
 * @return EINA_TRUE on success.
 */
static Eina_Bool
_prop_text_direction_get(const Eldbus_Service_Interface *iface EINA_UNUSED,
                         const char *propname EINA_UNUSED,
                         Eldbus_Message_Iter *iter,
                         const Eldbus_Message *request_msg EINA_UNUSED,
                         Eldbus_Message **error EINA_UNUSED)
{
   if (_elm_config->is_mirrored)
     eldbus_message_iter_basic_append(iter, 's', "rtl");
   else
     eldbus_message_iter_basic_append(iter, 's', "ltr");

   return EINA_TRUE;
}

/**
 * @brief Getter for the 'Status' D-Bus property.
 *
 * Returns the status of the menu, which is always "normal".
 *
 * @param iface Unused.
 * @param propname Unused.
 * @param iter The iterator to append the value to.
 * @param request_msg Unused.
 * @param error Unused.
 * @return EINA_TRUE on success.
 */
static Eina_Bool
_prop_status_get(const Eldbus_Service_Interface *iface EINA_UNUSED,
                 const char *propname EINA_UNUSED,
                 Eldbus_Message_Iter *iter,
                 const Eldbus_Message *request_msg EINA_UNUSED,
                 Eldbus_Message **error EINA_UNUSED)
{
   static const char *normal = "normal";
   eldbus_message_iter_basic_append(iter, 's', normal);

   return EINA_TRUE;
}

/**
 * @brief Getter for the 'IconThemePath' D-Bus property.
 *
 * Returns the search paths for icon themes. This implementation returns only
 * Elementary's own icon directory.
 *
 * @param iface Unused.
 * @param propname Unused.
 * @param iter The iterator to append the value to.
 * @param request_msg Unused.
 * @param error Unused.
 * @return EINA_TRUE on success.
 */
static Eina_Bool
_prop_icon_theme_path_get(const Eldbus_Service_Interface *iface EINA_UNUSED,
                          const char *propname EINA_UNUSED,
                          Eldbus_Message_Iter *iter,
                          const Eldbus_Message *request_msg EINA_UNUSED,
                          Eldbus_Message **error EINA_UNUSED)
{
   Eldbus_Message_Iter *actions;
   eldbus_message_iter_arguments_append(iter, "as", &actions);
   eldbus_message_iter_arguments_append(actions, "s", ICON_DIR);
   eldbus_message_iter_container_close(iter, actions);

   return EINA_TRUE;
}

static const Eldbus_Property _properties[] = {
   { "Version", "u", _prop_version_get, NULL, 0 },
   { "TextDirection", "s", _prop_text_direction_get, NULL, 0 },
   { "Status", "s", _prop_status_get, NULL, 0 },
   { "IconThemePath", "as", _prop_icon_theme_path_get, NULL, 0 },
   { NULL, NULL, NULL, NULL, 0 },
};

static const Eldbus_Service_Interface_Desc _interface = {
   DBUS_INTERFACE, _methods, _signals, _properties, NULL, NULL
};
// =============================================================================

/**
 * @brief Registers an Elementary menu widget on D-Bus.
 *
 * This function exposes an Elm_Menu widget over D-Bus using the
 * com.canonical.dbusmenu interface. It creates a D-Bus object with a unique
 * path and registers the interface on it.
 *
 * @param obj The menu widget to register.
 * @return The D-Bus object path of the registered menu, or NULL on failure.
 */
const char *
_elm_dbus_menu_register(Eo *obj)
{
   char buf[60];
   ELM_MENU_CHECK(obj) NULL;
   ELM_MENU_DATA_GET(obj, sd);

   elm_need_eldbus();

   if (sd->dbus_menu)
     goto end;

   sd->dbus_menu = _elm_dbus_menu_add(obj);
   sd->dbus_menu->bus = eldbus_connection_get(ELDBUS_CONNECTION_TYPE_SESSION);
   snprintf(buf, sizeof(buf), "%s/%u", DBUS_PATH, ++last_object_path);
   sd->dbus_menu->iface = eldbus_service_interface_register(sd->dbus_menu->bus,
                                                           buf,
                                                           &_interface);
   eldbus_service_object_data_set(sd->dbus_menu->iface, DBUS_DATA_KEY,
                                 sd->dbus_menu);

end:
   return eldbus_service_object_path_get(sd->dbus_menu->iface);
}

/**
 * @brief Unregisters an Elementary menu widget from D-Bus.
 *
 * Cleans up all resources associated with the D-Bus menu, including unregistering
 * the D-Bus interface and freeing allocated memory.
 *
 * @param obj The menu widget to unregister.
 */
void
_elm_dbus_menu_unregister(Eo *obj)
{
   // TODO: support refcounting object paths

   ELM_MENU_CHECK(obj);
   ELM_MENU_DATA_GET(obj, sd);

   if (!sd->dbus_menu) return;

   if (sd->dbus_menu->app_menu_data)
     _elm_dbus_menu_app_menu_unregister(obj);
   eldbus_service_interface_unregister(sd->dbus_menu->iface);
   eldbus_connection_unref(sd->dbus_menu->bus);
   ecore_idler_del(sd->dbus_menu->signal_idler);

   eina_hash_free(sd->dbus_menu->elements);
   ELM_SAFE_FREE(sd->dbus_menu, free);
}

/**
 * @brief Registers the menu as the application menu for a specific window.
 *
 * This communicates with the 'com.canonical.AppMenu.Registrar' service to
 * associate this D-Bus menu with a given X11 window ID. This is how services
 * like the Unity panel or MATE menu bar find the menu for a window.
 *
 * @param xid The X11 window ID.
 * @param obj The menu widget.
 * @param result_cb A callback to be invoked with the result of the registration.
 * @param data User data to be passed to the result callback.
 */
void
_elm_dbus_menu_app_menu_register(Ecore_X_Window xid, Eo *obj,
                                 void (*result_cb)(Eina_Bool, void *), void *data)
{
   Callback_Data *cd;

   ELM_MENU_CHECK(obj);
   ELM_MENU_DATA_GET(obj, sd);

   if (!sd->dbus_menu || !sd->dbus_menu->bus)
     {
        ERR("D-Bus is inactive for menu: %p", obj);
        return;
     }

   if (sd->dbus_menu->app_menu_data)
     {
        if (sd->dbus_menu->app_menu_data->xid != xid)
          ERR("There's another XID registered: %x",
              sd->dbus_menu->app_menu_data->xid);

        return;
     }

   sd->dbus_menu->app_menu_data = malloc(sizeof(Callback_Data));
   if (!sd->dbus_menu->app_menu_data) return;

   cd = sd->dbus_menu->app_menu_data;
   cd->result_cb = result_cb;
   cd->data = data;
   cd->pending_register = NULL;
   cd->xid = xid;
   eldbus_name_owner_changed_callback_add(sd->dbus_menu->bus, REGISTRAR_NAME,
                                         _app_menu_watch_cb, sd->dbus_menu,
                                         EINA_TRUE);
}

/**
 * @brief Unregisters the application menu for a window.
 *
 * Notifies the 'com.canonical.AppMenu.Registrar' that the menu for the given
 * window is no longer available.
 *
 * @param obj The menu widget that was registered.
 */
void
_elm_dbus_menu_app_menu_unregister(Eo *obj)
{
   Eldbus_Message *msg;
   Callback_Data *cd;

   ELM_MENU_CHECK(obj);
   ELM_MENU_DATA_GET(obj, sd);

   if (!sd->dbus_menu || !sd->dbus_menu->bus)
     {
        ERR("D-Bus is inactive for menu: %p", obj);
        return;
     }

   cd = sd->dbus_menu->app_menu_data;

   if (!cd) return;

   if (cd->pending_register)
     eldbus_pending_cancel(cd->pending_register);

   msg = eldbus_message_method_call_new(REGISTRAR_NAME, REGISTRAR_PATH,
                                       REGISTRAR_INTERFACE, "UnregisterWindow");
   eldbus_message_arguments_append(msg, "u", (unsigned)cd->xid);
   eldbus_connection_send(sd->dbus_menu->bus, msg, NULL, NULL, -1);
   eldbus_name_owner_changed_callback_del(sd->dbus_menu->bus, REGISTRAR_NAME,
                                         _app_menu_watch_cb, sd->dbus_menu);
   free(cd);
   sd->dbus_menu->app_menu_data = NULL;
}

/**
 * @brief Informs the D-Bus service that a new menu item has been added.
 *
 * This adds the new item to the internal hash and schedules a LayoutUpdated
 * signal to be sent.
 *
 * @param dbus_menu The D-Bus menu instance.
 * @param item_obj The new menu item that was added.
 * @return The new unique ID assigned to the item, or -1 on failure.
 */
int
_elm_dbus_menu_item_add(Elm_DBus_Menu *dbus_menu, Elm_Object_Item *item_obj)
{
   ELM_MENU_ITEM_DATA_GET(item_obj, item);
   int32_t id = dbus_menu->timestamp + 1;

   if (!eina_hash_add(dbus_menu->elements, &id, item))
     {
        ERR("Unable to add menu");
        return -1;
     }

   _layout_signal(dbus_menu);
   return ++dbus_menu->timestamp;
}

/**
 * @brief Informs the D-Bus service that a menu item has been removed.
 *
 * Removes the item from the internal hash and schedules a LayoutUpdated
 * signal to be sent.
 *
 * @param dbus_menu The D-Bus menu instance.
 * @param id The unique ID of the item that was removed.
 */
void
_elm_dbus_menu_item_delete(Elm_DBus_Menu *dbus_menu, int id)
{
   int32_t i;

   i = id;

   if (!eina_hash_del_by_key(dbus_menu->elements, &i))
     {
        ERR("Invalid menu ID: %d", id);
        return;
     }

   dbus_menu->timestamp++;
   _layout_signal(dbus_menu);
}

/**
 * @brief Signals a generic update to the menu layout.
 *
 * This should be called when properties of existing items change (e.g., a label
 * or enabled state). It increments the revision timestamp and schedules a
 * LayoutUpdated signal.
 *
 * @param dbus_menu The D-Bus menu instance to update.
 */
void
_elm_dbus_menu_update(Elm_DBus_Menu *dbus_menu)
{
   dbus_menu->timestamp++;
   _layout_signal(dbus_menu);
}
