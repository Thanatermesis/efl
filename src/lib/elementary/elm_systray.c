/**
 * @file
 * @brief This file implements the Elm_Systray widget, which provides
 *        functionality for creating and managing system tray icons
 *        using the StatusNotifierItem D-Bus specification.
 */

#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include <Elementary.h>

#include "elm_priv.h"
#include "elm_systray_eo.h"
#include "elm_systray_watcher.h"

/**
 * @brief Event type identifier for when the systray is ready.
 * This event is triggered when the D-Bus service for the systray item
 * has been successfully registered and is ready to be used.
 */
EAPI int ELM_EVENT_SYSTRAY_READY = 0;

/**
 * @brief Private data structure for the Elm_Systray object.
 * This structure holds all the properties of a StatusNotifierItem.
 */
typedef struct _Elm_Systray_Private_Data
{
   /** @brief The category of the systray item (e.g., application status, communications). */
   Elm_Systray_Category cat;
   /** @brief The current status of the systray item (e.g., active, passive, needs attention). */
   Elm_Systray_Status   status;

   /** @brief The name of the icon to be displayed when attention is required. */
   const char           *att_icon_name;
   /** @brief The primary icon name for the systray item. */
   const char           *icon_name;
   /** @brief The path to the icon theme to be used for resolving icon names. */
   const char           *icon_theme_path;
   /** @brief A unique identifier for the systray item. */
   const char           *id;
   /** @brief The title of the systray item, often displayed as a tooltip. */
   const char           *title;
   /** @brief The D-Bus object path of the menu associated with this systray item. */
   const char           *menu;
   /** @brief Pointer to the Evas_Object representing the menu. */
   const Eo             *menu_obj;
} Elm_Systray_Private_Data;

/**
 * @brief Static instance of the systray item's private data.
 * This module currently supports only a single systray item instance.
 */
static Elm_Systray_Private_Data _item = {
   .cat             = ELM_SYSTRAY_CATEGORY_APP_STATUS,
   .status          = ELM_SYSTRAY_STATUS_ACTIVE,
   .att_icon_name   = NULL,
   .icon_name       = NULL,
   .icon_theme_path = NULL,
   .id              = NULL,
   .title           = NULL,
   .menu            = NULL,
   .menu_obj        = NULL
};

/** @brief D-Bus object path for the StatusNotifierItem. */
#define OBJ_PATH  "/org/ayatana/NotificationItem/StatusNotifierItem"
/** @brief D-Bus interface name for the StatusNotifierItem. */
#define INTERFACE "org.kde.StatusNotifierItem"

/** @brief Flag indicating if the systray functionality has been initialized. */
static Eina_Bool _elm_need_systray = EINA_FALSE;

/** @brief Eldbus connection handle. */
static Eldbus_Connection        *_conn  = NULL;
/** @brief Eldbus service interface handle for the StatusNotifierItem. */
static Eldbus_Service_Interface *_iface = NULL;

/**
 * @brief Array of strings corresponding to Elm_Systray_Category enum values.
 * Used for D-Bus communication.
 */
static const char *_Elm_Systray_Cat_Str[] = {
   [ELM_SYSTRAY_CATEGORY_APP_STATUS]     = "ApplicationStatus",
   [ELM_SYSTRAY_CATEGORY_COMMUNICATIONS] = "Communications",
   [ELM_SYSTRAY_CATEGORY_SYS_SERVICES]   = "SystemServices",
   [ELM_SYSTRAY_CATEGORY_HARDWARE]       = "Hardware",
   [ELM_SYSTRAY_CATEGORY_OTHER]          = "Other"
};

/**
 * @brief Array of strings corresponding to Elm_Systray_Status enum values.
 * Used for D-Bus communication.
 */
static const char *_Elm_Systray_Status_Str[] = {
   [ELM_SYSTRAY_STATUS_ACTIVE]    = "Active",
   [ELM_SYSTRAY_STATUS_PASSIVE]   = "Passive",
   [ELM_SYSTRAY_STATUS_ATTENTION] = "NeedsAttention"
};

// =============================================================================
//                     org.kde.StatusNotifierItem Service
// =============================================================================
// =============================================================================
// Methods
// =============================================================================
/**
 * @brief Generic D-Bus method handler that returns an empty reply.
 * Used for methods that are part of the interface but not implemented
 * with specific logic (e.g., Scroll, SecondaryActivate).
 *
 * @param iface The Eldbus service interface.
 * @param msg The incoming D-Bus message.
 * @return A new D-Bus message representing an empty method return.
 */
static Eldbus_Message *
_empty_method(const Eldbus_Service_Interface *iface EINA_UNUSED,
        const Eldbus_Message *msg)
{
   return eldbus_message_method_return_new(msg);
}

/**
 * @brief D-Bus methods provided by the StatusNotifierItem service.
 * These methods define actions that can be invoked on the systray item.
 * Example:
 * @code
 * // D-Bus call to Scroll method:
 * // dbus-send --session --type=method_call --print-reply \
 * //   --dest=org.kde.StatusNotifierItem-PID-1 \ // Replace PID with actual process ID
 * //   /org/ayatana/NotificationItem/StatusNotifierItem \
 * //   org.kde.StatusNotifierItem.Scroll \
 * //   int32:10 string:"vertical"
 * @endcode
 */
static const Eldbus_Method methods[] = {
      {
         "Scroll",
         ELDBUS_ARGS({"i", "delta"}, {"s", "orientation"}),
         NULL,
         _empty_method,
         0
      },
      {
         "SecondaryActivate",
         ELDBUS_ARGS({"i", "x"}, {"i", "y"}),
         NULL,
         _empty_method,
         0
      },
      {
         "XAyatanaSecondaryActivate",
         ELDBUS_ARGS({"u", "timestamp"}),
         NULL,
         _empty_method,
         0
      },
      { NULL, NULL, NULL, NULL, 0 }
};

// =============================================================================
// Signals
// =============================================================================
/**
 * @brief Enumeration of D-Bus signals that can be emitted by the StatusNotifierItem service.
 * These signals notify clients about changes in the systray item's state.
 */
typedef enum _Elm_Systray_Service_Signals
{
   /** @brief Signal emitted when the attention icon changes. */
   ELM_SYSTRAY_SIGNAL_NEWATTENTIONICON,
   /** @brief Signal emitted when the primary icon changes. */
   ELM_SYSTRAY_SIGNAL_NEWICON,
   /** @brief Signal emitted when the icon theme path changes. */
   ELM_SYSTRAY_SIGNAL_NEWICONTHEMEPATH,
   /** @brief Signal emitted when the status changes. */
   ELM_SYSTRAY_SIGNAL_NEWSTATUS,
   /** @brief Signal emitted when the title changes. */
   ELM_SYSTRAY_SIGNAL_NEWTITLE,
   /** @brief Ayatana specific signal for new label. */
   ELM_SYSTRAY_SIGNAL_XAYATANANEWLABEL
} Elm_Systray_Service_Signals;

/**
 * @brief Macro to simplify emitting D-Bus signals.
 * @param sig The signal identifier from Elm_Systray_Service_Signals.
 * @param ... Arguments for the signal.
 */
#define _elm_systray_signal_emit(sig, ...)            \
   eldbus_service_signal_emit(_iface, sig, __VA_ARGS__)

/**
 * @brief D-Bus signals provided by the StatusNotifierItem service.
 * These signals are emitted when properties of the systray item change.
 * Example of signal structure:
 * @code
 * // ELM_SYSTRAY_SIGNAL_NEWSTATUS signal:
 * // Emits "NewStatus" with a string argument for the new status.
 * // { "NewStatus", ELDBUS_ARGS({"s", "status"}), 0 }
 * @endcode
 */
static const Eldbus_Signal signals[] = {
     [ELM_SYSTRAY_SIGNAL_NEWATTENTIONICON] = {
          "NewAttentionIcon", NULL, 0
     },
     [ELM_SYSTRAY_SIGNAL_NEWICON] = {
          "NewIcon", NULL, 0
     },
     [ELM_SYSTRAY_SIGNAL_NEWICONTHEMEPATH] = {
          "NewIconThemePath", ELDBUS_ARGS({"s", "icon_theme_path"}), 0
     },
     [ELM_SYSTRAY_SIGNAL_NEWSTATUS] = {
          "NewStatus", ELDBUS_ARGS({"s", "status"}), 0
     },
     [ELM_SYSTRAY_SIGNAL_NEWTITLE] = {
          "NewTitle", NULL, 0
     },
     [ELM_SYSTRAY_SIGNAL_XAYATANANEWLABEL] = {
          "XAyatanaNewLabel", ELDBUS_ARGS({"s", "label"}, {"s", "guide"}), 0
     },
     { NULL, NULL, 0 }
};

// =============================================================================
// Properties
// =============================================================================
/**
 * @brief D-Bus property getter that returns an empty string.
 * Used for string properties that are not implemented or should be empty.
 *
 * @param iface The Eldbus service interface.
 * @param propname The name of the property.
 * @param iter Eldbus message iterator to append the property value to.
 * @param request_msg The original D-Bus request message.
 * @param error Pointer to an Eldbus_Message to store error information if any.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_prop_str_empty_get(const Eldbus_Service_Interface *iface EINA_UNUSED,
                    const char *propname EINA_UNUSED,
                    Eldbus_Message_Iter *iter,
                    const Eldbus_Message *request_msg EINA_UNUSED,
                    Eldbus_Message **error EINA_UNUSED)
{
   eldbus_message_iter_basic_append(iter, 's', "");

   return EINA_TRUE;
}

/**
 * @brief D-Bus property getter for the "AttentionIconName" property.
 *
 * @param iface The Eldbus service interface.
 * @param propname The name of the property ("AttentionIconName").
 * @param iter Eldbus message iterator to append the property value to.
 * @param request_msg The original D-Bus request message.
 * @param error Pointer to an Eldbus_Message to store error information if any.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_prop_attention_icon_name_get(const Eldbus_Service_Interface *iface EINA_UNUSED,
                              const char *propname EINA_UNUSED,
                              Eldbus_Message_Iter *iter,
                              const Eldbus_Message *request_msg EINA_UNUSED,
                              Eldbus_Message **error EINA_UNUSED)
{
   const char *s = _item.att_icon_name ? _item.att_icon_name : "";

   eldbus_message_iter_basic_append(iter, 's', s);

   return EINA_TRUE;
}

/**
 * @brief D-Bus property getter for the "Category" property.
 *
 * @param iface The Eldbus service interface.
 * @param propname The name of the property ("Category").
 * @param iter Eldbus message iterator to append the property value to.
 * @param request_msg The original D-Bus request message.
 * @param error Pointer to an Eldbus_Message to store error information if any.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_prop_category_get(const Eldbus_Service_Interface *iface EINA_UNUSED,
                   const char *propname EINA_UNUSED,
                   Eldbus_Message_Iter *iter,
                   const Eldbus_Message *request_msg EINA_UNUSED,
                   Eldbus_Message **error EINA_UNUSED)
{
   eldbus_message_iter_basic_append(iter, 's', _Elm_Systray_Cat_Str[_item.cat]);

   return EINA_TRUE;
}

/**
 * @brief D-Bus property getter for the "IconName" property.
 *
 * @param iface The Eldbus service interface.
 * @param propname The name of the property ("IconName").
 * @param iter Eldbus message iterator to append the property value to.
 * @param request_msg The original D-Bus request message.
 * @param error Pointer to an Eldbus_Message to store error information if any.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_prop_icon_name_get(const Eldbus_Service_Interface *iface EINA_UNUSED,
                    const char *propname EINA_UNUSED,
                    Eldbus_Message_Iter *iter,
                    const Eldbus_Message *request_msg EINA_UNUSED,
                    Eldbus_Message **error EINA_UNUSED)
{
   const char *s = _item.icon_name ? _item.icon_name : "";

   eldbus_message_iter_basic_append(iter, 's', s);

   return EINA_TRUE;
}

/**
 * @brief D-Bus property getter for the "IconThemePath" property.
 *
 * @param iface The Eldbus service interface.
 * @param propname The name of the property ("IconThemePath").
 * @param iter Eldbus message iterator to append the property value to.
 * @param request_msg The original D-Bus request message.
 * @param error Pointer to an Eldbus_Message to store error information if any.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_prop_icon_theme_path_get(const Eldbus_Service_Interface *iface EINA_UNUSED,
                          const char *propname EINA_UNUSED,
                          Eldbus_Message_Iter *iter,
                          const Eldbus_Message *request_msg EINA_UNUSED,
                          Eldbus_Message **error EINA_UNUSED)
{
   const char *s = _item.icon_theme_path ? _item.icon_theme_path : "";

   eldbus_message_iter_basic_append(iter, 's', s);

   return EINA_TRUE;
}

/**
 * @brief D-Bus property getter for the "Id" property.
 *
 * @param iface The Eldbus service interface.
 * @param propname The name of the property ("Id").
 * @param iter Eldbus message iterator to append the property value to.
 * @param request_msg The original D-Bus request message.
 * @param error Pointer to an Eldbus_Message to store error information if any.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_prop_id_get(const Eldbus_Service_Interface *iface EINA_UNUSED,
             const char *propname EINA_UNUSED,
             Eldbus_Message_Iter *iter,
             const Eldbus_Message *request_msg EINA_UNUSED,
             Eldbus_Message **error EINA_UNUSED)
{
   const char *s = _item.id ? _item.id : "";

   eldbus_message_iter_basic_append(iter, 's', s);

   return EINA_TRUE;
}

/**
 * @brief D-Bus property getter for the "Menu" property.
 * Returns the D-Bus object path of the associated menu.
 *
 * @param iface The Eldbus service interface.
 * @param propname The name of the property ("Menu").
 * @param iter Eldbus message iterator to append the property value to.
 * @param request_msg The original D-Bus request message.
 * @param error Pointer to an Eldbus_Message to store error information if any.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_prop_menu_get(const Eldbus_Service_Interface *iface EINA_UNUSED,
               const char *propname EINA_UNUSED,
               Eldbus_Message_Iter *iter,
               const Eldbus_Message *request_msg EINA_UNUSED,
               Eldbus_Message **error EINA_UNUSED)
{
   const char *s = _item.menu ? _item.menu : "/";

   eldbus_message_iter_basic_append(iter, 'o', s);

   return EINA_TRUE;
}

/**
 * @brief D-Bus property getter for the "Status" property.
 *
 * @param iface The Eldbus service interface.
 * @param propname The name of the property ("Status").
 * @param iter Eldbus message iterator to append the property value to.
 * @param request_msg The original D-Bus request message.
 * @param error Pointer to an Eldbus_Message to store error information if any.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_prop_status_get(const Eldbus_Service_Interface *iface EINA_UNUSED,
                 const char *propname EINA_UNUSED,
                 Eldbus_Message_Iter *iter,
                 const Eldbus_Message *request_msg EINA_UNUSED,
                 Eldbus_Message **error EINA_UNUSED)
{
   eldbus_message_iter_basic_append(iter, 's',
                                   _Elm_Systray_Status_Str[_item.status]);

   return EINA_TRUE;
}

/**
 * @brief D-Bus property getter for the "Title" property.
 *
 * @param iface The Eldbus service interface.
 * @param propname The name of the property ("Title").
 * @param iter Eldbus message iterator to append the property value to.
 * @param request_msg The original D-Bus request message.
 * @param error Pointer to an Eldbus_Message to store error information if any.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_prop_title_get(const Eldbus_Service_Interface *iface EINA_UNUSED,
                const char *propname EINA_UNUSED,
                Eldbus_Message_Iter *iter,
                const Eldbus_Message *request_msg EINA_UNUSED,
                Eldbus_Message **error EINA_UNUSED)
{
   const char *s = _item.title ? _item.title : "";

   eldbus_message_iter_basic_append(iter, 's', s);

   return EINA_TRUE;
}

/**
 * @brief D-Bus property getter for the "XAyatanaOrderingIndex" property.
 * This is an Ayatana-specific property for ordering items. Currently returns 0.
 *
 * @param iface The Eldbus service interface.
 * @param propname The name of the property ("XAyatanaOrderingIndex").
 * @param iter Eldbus message iterator to append the property value to.
 * @param request_msg The original D-Bus request message.
 * @param error Pointer to an Eldbus_Message to store error information if any.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_prop_xayatana_orderindex_get(const Eldbus_Service_Interface *iface EINA_UNUSED,
                              const char *propname EINA_UNUSED,
                              Eldbus_Message_Iter *iter,
                              const Eldbus_Message *request_msg EINA_UNUSED,
                              Eldbus_Message **error EINA_UNUSED)
{
   eldbus_message_iter_basic_append(iter, 'u', 0);

   return EINA_TRUE;
}

/**
 * @brief D-Bus properties provided by the StatusNotifierItem service.
 * These properties define the state and appearance of the systray item.
 * Each property entry defines its name, D-Bus signature, getter function,
 * (optional) setter function, and flags.
 * Example of property structure:
 * @code
 * // "IconName" property:
 * // { "IconName", "s", _prop_icon_name_get, NULL, 0 }
 * // This means the property "IconName" is a string ("s"), its value is
 * // retrieved by _prop_icon_name_get, it has no setter (read-only from D-Bus),
 * // and has no special flags.
 * @endcode
 */
static const Eldbus_Property properties[] = {
       { "AttentionAcessibleDesc", "s", _prop_str_empty_get, NULL, 0 },
       { "AttentionIconName", "s", _prop_attention_icon_name_get, NULL, 0 },
       { "Category", "s", _prop_category_get, NULL, 0 },
       { "IconAcessibleDesc", "s", _prop_str_empty_get, NULL, 0 },
       { "IconName", "s", _prop_icon_name_get, NULL, 0 },
       { "IconThemePath", "s", _prop_icon_theme_path_get, NULL, 0 },
       { "Id", "s", _prop_id_get, NULL, 0 },
       { "Menu", "o", _prop_menu_get, NULL, 0 },
       { "Status", "s", _prop_status_get, NULL, 0 },
       { "Title", "s", _prop_title_get, NULL, 0 },
       { "XAyatanaLabelGuide", "s", _prop_str_empty_get, NULL, 0 },
       { "XAyatanaLabel", "s", _prop_str_empty_get, NULL, 0 },
       { "XAyatanaOrderingIndex", "u", _prop_xayatana_orderindex_get, NULL, 0 },
       { NULL, NULL, NULL, NULL, 0 }
};

/**
 * @brief Descriptor for the D-Bus service interface.
 * This structure bundles the interface name, methods, signals, and properties
 * for registration with Eldbus.
 */
static const Eldbus_Service_Interface_Desc _iface_desc = {
     INTERFACE, methods, signals, properties, NULL, NULL
};
// =============================================================================

/**
 * @brief Callback function invoked when the associated menu object is deleted.
 * This function cleans up references to the menu, resets the menu property,
 * and notifies D-Bus clients about the change.
 *
 * @param data User data (unused).
 * @param e Evas canvas (unused).
 * @param obj The Evas_Object that was deleted (the menu).
 * @param event_info Event-specific information (unused).
 */
static void
_menu_died(void *data EINA_UNUSED,
           Evas *e EINA_UNUSED,
           Evas_Object *obj EINA_UNUSED,
           void *event_info EINA_UNUSED)
{
   _item.menu_obj = NULL;

   eina_stringshare_replace(&(_item.menu), NULL);

   eldbus_service_property_changed(_iface, "Menu");
}

/**
 * @internal
 * @brief Sets the category of the systray item.
 * Implements the Eolian `category_set` method.
 * Notifies D-Bus clients if the category changes.
 *
 * @param obj The Elm_Systray Eo object (unused).
 * @param priv Private data for the Elm_Systray object (unused).
 * @param cat The new category to set.
 * @see Elm_Systray_Category
 */
EOLIAN static void
_elm_systray_category_set(Eo *obj EINA_UNUSED, void *priv EINA_UNUSED, Elm_Systray_Category cat)
{
   if (_item.cat == cat) return;

   _item.cat = cat;
   eldbus_service_property_changed(_iface, "Category");
}

/**
 * @internal
 * @brief Gets the category of the systray item.
 * Implements the Eolian `category_get` method.
 *
 * @param obj The Elm_Systray Eo object (unused).
 * @param priv Private data for the Elm_Systray object (unused).
 * @return The current category of the systray item.
 * @see Elm_Systray_Category
 */
EOLIAN static Elm_Systray_Category
_elm_systray_category_get(const Eo *obj EINA_UNUSED, void *priv EINA_UNUSED)
{
   return _item.cat;
}

/**
 * @internal
 * @brief Sets the status of the systray item.
 * Implements the Eolian `status_set` method.
 * Notifies D-Bus clients if the status changes by emitting both a property changed
 * signal for "Status" and the "NewStatus" signal.
 *
 * @param obj The Elm_Systray Eo object (unused).
 * @param priv Private data for the Elm_Systray object (unused).
 * @param st The new status to set.
 * @see Elm_Systray_Status
 */
EOLIAN static void
_elm_systray_status_set(Eo *obj EINA_UNUSED, void *priv EINA_UNUSED, Elm_Systray_Status st)
{
   if (_item.status == st) return;

   _item.status = st;
   eldbus_service_property_changed(_iface, "Status");
   _elm_systray_signal_emit(ELM_SYSTRAY_SIGNAL_NEWSTATUS,
                            _Elm_Systray_Status_Str[_item.status]);
}

/**
 * @internal
 * @brief Gets the status of the systray item.
 * Implements the Eolian `status_get` method.
 *
 * @param obj The Elm_Systray Eo object (unused).
 * @param priv Private data for the Elm_Systray object (unused).
 * @return The current status of the systray item.
 * @see Elm_Systray_Status
 */
EOLIAN static Elm_Systray_Status
_elm_systray_status_get(const Eo *obj EINA_UNUSED, void *priv EINA_UNUSED)
{
   return _item.status;
}

/**
 * @internal
 * @brief Sets the attention icon name for the systray item.
 * Implements the Eolian `att_icon_name_set` method.
 * Notifies D-Bus clients if the attention icon name changes by emitting both a
 * property changed signal for "AttentionIconName" and the "NewAttentionIcon" signal.
 *
 * @param obj The Elm_Systray Eo object (unused).
 * @param priv Private data for the Elm_Systray object (unused).
 * @param att_icon_name The new attention icon name. Example: "mail-unread".
 */
EOLIAN static void
_elm_systray_att_icon_name_set(Eo *obj EINA_UNUSED, void *priv EINA_UNUSED, const char *att_icon_name)
{
   if (!eina_stringshare_replace(&(_item.att_icon_name), att_icon_name)) return;

   eldbus_service_property_changed(_iface, "AttentionIconName");
   _elm_systray_signal_emit(ELM_SYSTRAY_SIGNAL_NEWATTENTIONICON, NULL);
}

/**
 * @internal
 * @brief Gets the attention icon name of the systray item.
 * Implements the Eolian `att_icon_name_get` method.
 *
 * @param obj The Elm_Systray Eo object (unused).
 * @param priv Private data for the Elm_Systray object (unused).
 * @return The current attention icon name.
 */
EOLIAN static const char*
_elm_systray_att_icon_name_get(const Eo *obj EINA_UNUSED, void *priv EINA_UNUSED)
{
   return _item.att_icon_name;
}

/**
 * @internal
 * @brief Sets the primary icon name for the systray item.
 * Implements the Eolian `icon_name_set` method.
 * Notifies D-Bus clients if the icon name changes by emitting both a
 * property changed signal for "IconName" and the "NewIcon" signal.
 *
 * @param obj The Elm_Systray Eo object (unused).
 * @param priv Private data for the Elm_Systray object (unused).
 * @param icon_name The new primary icon name. Example: "system-search".
 */
EOLIAN static void
_elm_systray_icon_name_set(Eo *obj EINA_UNUSED, void *priv EINA_UNUSED, const char *icon_name)
{
   if (!eina_stringshare_replace(&(_item.icon_name), icon_name)) return;

   eldbus_service_property_changed(_iface, "IconName");
   _elm_systray_signal_emit(ELM_SYSTRAY_SIGNAL_NEWICON, NULL);
}

/**
 * @internal
 * @brief Gets the primary icon name of the systray item.
 * Implements the Eolian `icon_name_get` method.
 *
 * @param obj The Elm_Systray Eo object (unused).
 * @param priv Private data for the Elm_Systray object (unused).
 * @return The current primary icon name.
 */
EOLIAN static const char*
_elm_systray_icon_name_get(const Eo *obj EINA_UNUSED, void *priv EINA_UNUSED)
{
   return _item.icon_name;
}

/**
 * @internal
 * @brief Sets the icon theme path for the systray item.
 * Implements the Eolian `icon_theme_path_set` method.
 * Notifies D-Bus clients if the icon theme path changes by emitting both a
 * property changed signal for "IconThemePath" and the "NewIconThemePath" signal.
 *
 * @param obj The Elm_Systray Eo object (unused).
 * @param priv Private data for the Elm_Systray object (unused).
 * @param icon_theme_path The new icon theme path. Example: "/usr/share/icons/hicolor".
 */
EOLIAN static void
_elm_systray_icon_theme_path_set(Eo *obj EINA_UNUSED, void *priv EINA_UNUSED, const char *icon_theme_path)
{
   if (!eina_stringshare_replace(&(_item.icon_theme_path), icon_theme_path))
     return;

   eldbus_service_property_changed(_iface, "IconThemePath");
   _elm_systray_signal_emit(ELM_SYSTRAY_SIGNAL_NEWICONTHEMEPATH,
                            _item.icon_theme_path);
}

/**
 * @internal
 * @brief Gets the icon theme path of the systray item.
 * Implements the Eolian `icon_theme_path_get` method.
 *
 * @param obj The Elm_Systray Eo object (unused).
 * @param priv Private data for the Elm_Systray object (unused).
 * @return The current icon theme path.
 */
EOLIAN static const char*
_elm_systray_icon_theme_path_get(const Eo *obj EINA_UNUSED, void *priv EINA_UNUSED)
{
   return _item.icon_theme_path;
}

/**
 * @internal
 * @brief Sets the ID of the systray item.
 * Implements the Eolian `id_set` method.
 * Notifies D-Bus clients if the ID changes.
 *
 * @param obj The Elm_Systray Eo object (unused).
 * @param priv Private data for the Elm_Systray object (unused).
 * @param id The new ID for the systray item. Example: "my-app-indicator".
 */
EOLIAN static void
_elm_systray_id_set(Eo *obj EINA_UNUSED, void *priv EINA_UNUSED, const char *id)
{
   if (!eina_stringshare_replace(&(_item.id), id)) return;

   eldbus_service_property_changed(_iface, "Id");
}

/**
 * @internal
 * @brief Gets the ID of the systray item.
 * Implements the Eolian `id_get` method.
 *
 * @param obj The Elm_Systray Eo object (unused).
 * @param priv Private data for the Elm_Systray object (unused).
 * @return The current ID of the systray item.
 */
EOLIAN static const char*
_elm_systray_id_get(const Eo *obj EINA_UNUSED, void *priv EINA_UNUSED)
{
   return _item.id;
}

/**
 * @internal
 * @brief Sets the title of the systray item.
 * Implements the Eolian `title_set` method.
 * Notifies D-Bus clients if the title changes by emitting both a
 * property changed signal for "Title" and the "NewTitle" signal.
 *
 * @param obj The Elm_Systray Eo object (unused).
 * @param priv Private data for the Elm_Systray object (unused).
 * @param title The new title for the systray item. Example: "My Application Status".
 */
EOLIAN static void
_elm_systray_title_set(Eo *obj EINA_UNUSED, void *priv EINA_UNUSED, const char *title)
{
   if (!eina_stringshare_replace(&(_item.title), title)) return;

   eldbus_service_property_changed(_iface, "Title");
   _elm_systray_signal_emit(ELM_SYSTRAY_SIGNAL_NEWTITLE, NULL);
}

/**
 * @internal
 * @brief Gets the title of the systray item.
 * Implements the Eolian `title_get` method.
 *
 * @param obj The Elm_Systray Eo object (unused).
 * @param priv Private data for the Elm_Systray object (unused).
 * @return The current title of the systray item.
 */
EOLIAN static const char*
_elm_systray_title_get(const Eo *obj EINA_UNUSED, void *priv EINA_UNUSED)
{
   return _item.title;
}

/**
 * @internal
 * @brief Sets the menu object associated with the systray item.
 * Implements the Eolian `menu_set` method.
 * Registers the menu with the D-Bus menu service and sets up a callback
 * for when the menu object is deleted. Notifies D-Bus clients if the menu changes.
 *
 * @param obj The Elm_Systray Eo object (unused).
 * @param priv Private data for the Elm_Systray object (unused).
 * @param menu_obj The Evas_Object representing the menu to associate.
 *                 Pass NULL to remove the current menu.
 */
EOLIAN static void
_elm_systray_menu_set(Eo *obj EINA_UNUSED, void *priv EINA_UNUSED, const Eo *menu_obj)
{
   const char *menu = NULL;

   if (_item.menu_obj == menu_obj) return;

   if (menu_obj)
     {
        menu = _elm_dbus_menu_register((Eo *) menu_obj);
        evas_object_event_callback_add((Evas_Object *) menu_obj,
                                       EVAS_CALLBACK_DEL, _menu_died, NULL);
     }

   eina_stringshare_replace(&(_item.menu), menu);

   if (_item.menu_obj)
     evas_object_event_callback_del_full((Evas_Object *) _item.menu_obj,
                                         EVAS_CALLBACK_DEL, _menu_died, NULL);

   _item.menu_obj = menu_obj;

   eldbus_service_property_changed(_iface, "Menu");
}

/**
 * @internal
 * @brief Gets the menu object associated with the systray item.
 * Implements the Eolian `menu_get` method.
 *
 * @param obj The Elm_Systray Eo object (unused).
 * @param priv Private data for the Elm_Systray object (unused).
 * @return The Evas_Object representing the current menu, or NULL if no menu is set.
 */
EOLIAN static const Eo*
_elm_systray_menu_get(const Eo *obj EINA_UNUSED, void *priv EINA_UNUSED)
{
   return _item.menu_obj;
}

/**
 * @internal
 * @brief Registers the systray item with the StatusNotifierWatcher.
 * Implements the Eolian `register` method.
 * This makes the systray item visible in the system tray.
 *
 * @param obj The Elm_Systray Eo object (unused).
 * @param priv Private data for the Elm_Systray object (unused).
 * @return EINA_TRUE if registration was successful or systray is already initialized,
 *         EINA_FALSE otherwise (e.g., if systray is not needed/initialized).
 */
EOLIAN static Eina_Bool
_elm_systray_register(Eo *obj EINA_UNUSED, void *priv EINA_UNUSED)
{
   if (!_elm_need_systray) return EINA_FALSE;

   return _elm_systray_watcher_status_notifier_item_register(OBJ_PATH);
}

/**
 * @brief Initializes the systray system if not already initialized.
 * This function sets up the D-Bus connection and registers the
 * StatusNotifierItem service. It also initializes the systray watcher.
 * This must be called before any systray items can be created or registered.
 *
 * @return EINA_TRUE if the systray system was successfully initialized or
 *         was already initialized. EINA_FALSE on failure.
 * @ingroup Elm_Systray
 */
EAPI Eina_Bool
elm_need_systray(void)
{
   if (_elm_need_systray) return EINA_TRUE;

   if (!elm_need_eldbus()) return EINA_FALSE;

   ELM_EVENT_SYSTRAY_READY = ecore_event_type_new();

   if (!_elm_systray_watcher_init()) return EINA_FALSE;

   _conn = eldbus_connection_get(ELDBUS_CONNECTION_TYPE_SESSION);
   if (!_conn) goto err;

   _iface = eldbus_service_interface_register(_conn, OBJ_PATH, &_iface_desc);
   if (!_iface) goto err;

   _elm_need_systray = EINA_TRUE;
   return EINA_TRUE;

err:
   if (_conn)
     {
        eldbus_connection_unref(_conn);
        _conn = NULL;
     }

   _elm_systray_watcher_shutdown();
   return EINA_FALSE;
}

/**
 * @internal
 * @brief Shuts down the systray system.
 * This function unregisters the D-Bus service, closes the D-Bus connection,
 * shuts down the systray watcher, and cleans up resources.
 * This is typically called during application shutdown if elm_need_systray()
 * was previously called.
 */
void
_elm_unneed_systray(void)
{
   if (!_elm_need_systray) return;

   ecore_event_type_flush(ELM_EVENT_SYSTRAY_READY);

   _elm_need_systray = EINA_FALSE;

   eldbus_service_interface_unregister(_iface);

   eldbus_connection_unref(_conn);

   _elm_systray_watcher_shutdown();

   eina_stringshare_del(_item.att_icon_name);
   eina_stringshare_del(_item.icon_name);
   eina_stringshare_del(_item.icon_theme_path);
   eina_stringshare_del(_item.id);
   eina_stringshare_del(_item.title);
   eina_stringshare_del(_item.menu);

   if (_item.menu_obj)
     {
        evas_object_event_callback_del_full((Evas_Object *) _item.menu_obj,
                                            EVAS_CALLBACK_DEL, _menu_died,
                                            NULL);
        _item.menu_obj = NULL;
     }
}

EAPI Elm_Systray*
elm_systray_add(Evas_Object *win)
{
   return efl_add(ELM_SYSTRAY_CLASS, win);
}


#include "elm_systray_eo.c"
