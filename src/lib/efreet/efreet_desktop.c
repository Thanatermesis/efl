#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* TODO : have to look why sockets must be init */
#ifdef _WIN32
# include <evil_private.h> /* evil_sockets_init|shutdown */
#endif

#include <Ecore_File.h>

/* define macros and variable for using the eina logging system  */
#define EFREET_MODULE_LOG_DOM _efreet_desktop_log_dom
int _efreet_desktop_log_dom = -1;

#include "Efreet.h"
#include "efreet_private.h"

#define DESKTOP_VERSION "1.0"

/**
 * The current desktop environment (e.g. "Enlightenment" or "Gnome")
 */
static const char *desktop_environment = NULL;

/**
 * A list of the desktop types available. Each item is an Efreet_Desktop_Type_Info.
 */
static Eina_List *efreet_desktop_types = NULL;

/** @internal Lock for thread-safe operations on shared desktop data. */
static Eina_Lock _lock;

/** Global integer representing the 'Application' desktop type. Initialized in efreet_desktop_init(). */
EAPI int EFREET_DESKTOP_TYPE_APPLICATION = 0;
/** Global integer representing the 'Link' desktop type. Initialized in efreet_desktop_init(). */
EAPI int EFREET_DESKTOP_TYPE_LINK = 0;
/** Global integer representing the 'Directory' desktop type. Initialized in efreet_desktop_init(). */
EAPI int EFREET_DESKTOP_TYPE_DIRECTORY = 0;

/**
 * @internal
 * Information about custom types
 */
typedef struct Efreet_Desktop_Type_Info Efreet_Desktop_Type_Info;
/**
 * @internal
 * @brief Structure to hold information about a registered desktop entry type.
 */
struct Efreet_Desktop_Type_Info
{
    int id; /**< Unique numerical identifier for the type. */
    const char *type; /**< String representation of the type (e.g., "Application"). */
    Efreet_Desktop_Type_Parse_Cb parse_func; /**< Callback to parse type-specific fields. */
    Efreet_Desktop_Type_Save_Cb save_func; /**< Callback to save type-specific fields. */
    Efreet_Desktop_Type_Free_Cb free_func; /**< Callback to free type-specific data. */
};

/**
 * @internal
 * @brief Reads and parses a .desktop file into an Efreet_Desktop structure.
 * @param desktop The Efreet_Desktop structure to populate. Assumed to have orig_path set.
 * @return 1 on success, 0 on failure.
 */
static int efreet_desktop_read(Efreet_Desktop *desktop);
/**
 * @internal
 * @brief Parses a type string (e.g., "Application") and returns the corresponding Efreet_Desktop_Type_Info.
 * @param type_str The string representation of the desktop type.
 * @return Pointer to Efreet_Desktop_Type_Info if found, NULL otherwise.
 */
static Efreet_Desktop_Type_Info *efreet_desktop_type_parse(const char *type_str);
/**
 * @internal
 * @brief Frees an Efreet_Desktop_Type_Info structure.
 * @param info The Efreet_Desktop_Type_Info to free.
 */
static void efreet_desktop_type_info_free(Efreet_Desktop_Type_Info *info);
/**
 * @internal
 * @brief Parses fields specific to "Application" type .desktop files.
 * @param desktop The Efreet_Desktop structure to populate.
 * @param ini The Efreet_Ini handle for the parsed .desktop file.
 * @return Always NULL (type_data is not used for standard Application type).
 */
static void *efreet_desktop_application_fields_parse(Efreet_Desktop *desktop,
                                                    Efreet_Ini *ini);
/**
 * @internal
 * @brief Parses Desktop Actions from an .ini file.
 * @param ini The Efreet_Ini handle for the parsed .desktop file.
 * @param val The string value of the "Actions" key (a semicolon-separated list of action keys).
 * @return An Eina_List of Efreet_Desktop_Action structures, or NULL on failure or if no actions.
 *         Each Efreet_Desktop_Action contains:
 *         - key: (char *) The action key (e.g., "NewWindow").
 *         - name: (char *) The localized name of the action.
 *         - icon: (char *) The icon name for the action.
 *         - exec: (char *) The command to execute for the action.
 */
static Eina_List *efreet_desktop_action_fields_parse(Efreet_Ini *ini, const char *val);
/**
 * @internal
 * @brief Saves "Application" type specific fields from an Efreet_Desktop structure to an Efreet_Ini.
 * @param desktop The Efreet_Desktop structure containing the data.
 * @param ini The Efreet_Ini handle to save data to.
 */
static void efreet_desktop_application_fields_save(Efreet_Desktop *desktop,
                                                    Efreet_Ini *ini);
/**
 * @internal
 * @brief Saves Desktop Actions from an Efreet_Desktop structure to an Efreet_Ini.
 * @param desktop The Efreet_Desktop structure containing the actions.
 * @param ini The Efreet_Ini handle to save data to.
 */
static void efreet_desktop_action_fields_save(Efreet_Desktop *desktop,
                                              Efreet_Ini *ini);
/**
 * @internal
 * @brief Parses fields specific to "Link" type .desktop files.
 * @param desktop The Efreet_Desktop structure to populate.
 * @param ini The Efreet_Ini handle for the parsed .desktop file.
 * @return Always NULL (type_data is not used for standard Link type).
 */
static void *efreet_desktop_link_fields_parse(Efreet_Desktop *desktop,
                                                Efreet_Ini *ini);
/**
 * @internal
 * @brief Saves "Link" type specific fields from an Efreet_Desktop structure to an Efreet_Ini.
 * @param desktop The Efreet_Desktop structure containing the data.
 * @param ini The Efreet_Ini handle to save data to.
 */
static void efreet_desktop_link_fields_save(Efreet_Desktop *desktop,
                                                Efreet_Ini *ini);
/**
 * @internal
 * @brief Parses generic fields common to all .desktop file types.
 * @param desktop The Efreet_Desktop structure to populate.
 * @param ini The Efreet_Ini handle for the parsed .desktop file.
 * @return 1 on success (required fields like Name found), 0 on failure.
 */
static int efreet_desktop_generic_fields_parse(Efreet_Desktop *desktop,
                                                Efreet_Ini *ini);
/**
 * @internal
 * @brief Saves generic fields common to all .desktop file types from an Efreet_Desktop structure to an Efreet_Ini.
 * @param desktop The Efreet_Desktop structure containing the data.
 * @param ini The Efreet_Ini handle to save data to.
 */
static void efreet_desktop_generic_fields_save(Efreet_Desktop *desktop,
                                                Efreet_Ini *ini);
/**
 * @internal
 * @brief Callback for eina_hash_foreach to parse X- fields from an Efreet_Ini into an Efreet_Desktop.
 * @param hash The hash being iterated (unused).
 * @param key The key of the hash element (field name, e.g., "X-Foo").
 * @param data The value associated with the key.
 * @param fdata User data, expected to be an Efreet_Desktop pointer.
 * @return EINA_TRUE to continue iteration.
 */
static Eina_Bool efreet_desktop_x_fields_parse(const Eina_Hash *hash,
                                                const void *key,
                                                void *data,
                                                void *fdata);
/**
 * @internal
 * @brief Callback for eina_hash_foreach to save X- fields from an Efreet_Desktop to an Efreet_Ini.
 * @param hash The hash being iterated (unused).
 * @param key The key of the hash element (field name, e.g., "X-Foo").
 * @param value The value associated with the key.
 * @param fdata User data, expected to be an Efreet_Ini pointer.
 * @return EINA_TRUE to continue iteration.
 */
static Eina_Bool efreet_desktop_x_fields_save(const Eina_Hash *hash,
                                                const void *key,
                                                void *value,
                                                void *fdata);
/**
 * @internal
 * @brief Checks if the desktop entry should be shown in the current environment based on OnlyShowIn/NotShowIn fields.
 * @param desktop The Efreet_Desktop to check.
 * @return 1 if the desktop entry is relevant to the current environment, 0 otherwise.
 */
static int efreet_desktop_environment_check(Efreet_Desktop *desktop);

/**
 * @internal
 * @return Returns > 0 on success or 0 on failure
 * @brief Initialize the Desktop parser subsystem
 */
int
efreet_desktop_init(void)
{
    _efreet_desktop_log_dom = eina_log_domain_register
      ("efreet_desktop", EFREET_DEFAULT_LOG_COLOR);
    if (_efreet_desktop_log_dom < 0)
    {
        EINA_LOG_ERR("Efreet: Could not create a log domain for efreet_desktop");
        return 0;
    }

#ifdef _WIN32
    if (!evil_sockets_init())
    {
        ERR("Could not initialize Winsock system");
        goto error;
    }
#endif

    if (!eina_lock_new(&_lock))
    {
        ERR("Could not create lock");
        goto error;
    }

    efreet_desktop_types = NULL;

    EFREET_DESKTOP_TYPE_APPLICATION = efreet_desktop_type_add("Application",
                                        efreet_desktop_application_fields_parse,
                                        efreet_desktop_application_fields_save,
                                        NULL);
    EFREET_DESKTOP_TYPE_LINK = efreet_desktop_type_add("Link",
                                    efreet_desktop_link_fields_parse,
                                    efreet_desktop_link_fields_save, NULL);
    EFREET_DESKTOP_TYPE_DIRECTORY = efreet_desktop_type_add("Directory", NULL,
                                                                NULL, NULL);

    return 1; // Successfully initialized
error: // Label for error handling during initialization
    eina_log_domain_unregister(_efreet_desktop_log_dom);
    _efreet_desktop_log_dom = -1;
    return 0;
}

/**
 * @internal
 * @returns the number of initializations left for this system
 * @brief Attempts to shut down the subsystem if nothing else is using it
 */
void
efreet_desktop_shutdown(void)
{
    Efreet_Desktop_Type_Info *info;

    // Release the stringshared desktop environment name
    IF_RELEASE(desktop_environment);
    // Free all registered desktop type information
    EINA_LIST_FREE(efreet_desktop_types, info)
        efreet_desktop_type_info_free(info);
    // Free the global lock
    eina_lock_free(&_lock);
#ifdef _WIN32
    // Shutdown Windows sockets if initialized
    evil_sockets_shutdown();
#endif
    // Unregister the log domain
    eina_log_domain_unregister(_efreet_desktop_log_dom);
    _efreet_desktop_log_dom = -1;
}

EAPI Efreet_Desktop *
efreet_desktop_get(const char *file)
{
    Efreet_Desktop *desktop;

    EINA_SAFETY_ON_NULL_RETURN_VAL(file, NULL);

    // Retrieve the desktop entry, potentially from cache or by parsing.
    desktop = efreet_desktop_new(file);
    if (!desktop) return NULL; // Failed to get/create desktop.
    return desktop;
   // this is wrong - start monitoring every/any dir in which a desktop file
   // exists that we load a desktop file from. imagine you browse directories
   // in efm with lots of desktop files in them - we end up monitoring lots
   // of directories that we then rememebr and don't un-monitor.
#if 0 // Disabled code block for adding non-cached files to monitor
    /* If we didn't find this file in the eet cache, add path to search path */
    if (!desktop->eet)
    {
        /* Check whether the desktop type is a system type,
         * and therefor known by the cache builder */
        Efreet_Desktop_Type_Info *info;

        info = eina_list_nth(efreet_desktop_types, desktop->type);
        if (info && (
                info->id == EFREET_DESKTOP_TYPE_APPLICATION ||
                info->id == EFREET_DESKTOP_TYPE_LINK ||
                info->id == EFREET_DESKTOP_TYPE_DIRECTORY
                ))
        {
            efreet_cache_desktop_add(desktop);
            /* Check Symbolic link */
            char *sym_file;
            Efreet_Desktop *sym_desktop;
            sym_file = ecore_file_readlink(file);
            if (sym_file)
            {
                sym_desktop = efreet_desktop_new(sym_file);
                if (sym_desktop && !sym_desktop->eet)
                  efreet_cache_desktop_add(sym_desktop);
                free(sym_file);
                efreet_desktop_free(sym_desktop);
            }
        }
    }
    return desktop;
#endif
}

EAPI int
efreet_desktop_ref(Efreet_Desktop *desktop)
{
    int ret;

    EINA_SAFETY_ON_NULL_RETURN_VAL(desktop, 0);
    // Thread-safe increment of the reference counter.
    eina_lock_take(&_lock);
    desktop->ref++;
    ret = desktop->ref;
    eina_lock_release(&_lock);
    return ret;
}

EAPI Efreet_Desktop *
efreet_desktop_empty_new(const char *file)
{
    Efreet_Desktop *desktop;

    EINA_SAFETY_ON_NULL_RETURN_VAL(file, NULL);

    // Allocate a new Efreet_Desktop structure.
    desktop = NEW(Efreet_Desktop, 1);
    if (!desktop) return NULL;

    // Store the original path of the .desktop file.
    desktop->orig_path = strdup(file);
    // Attempt to get the file's modification time to set load_time.
    // This do-while(0) loop is a common C idiom for creating a local scope
    // that can be broken out of, similar to a try block.
    do
    {
       struct stat st;

       if (!stat(desktop->orig_path, &st))
       {
          // Use the later of st_mtime or st_ctime as the load_time.
          time_t modtime = st.st_mtime;
          if (modtime < st.st_ctime) modtime = st.st_ctime;
          desktop->load_time = modtime;
       }
    } while (0);

    // Initialize reference count to 1 for the new desktop object.
    desktop->ref = 1;

    return desktop;
}

EAPI Efreet_Desktop *
efreet_desktop_new(const char *file)
{
    Efreet_Desktop *desktop = NULL;
    char *tmp; // Temporary storage for sanitized file path.

    EINA_SAFETY_ON_NULL_RETURN_VAL(file, NULL);

    // Sanitize the file path to prevent directory traversal or other issues.
    tmp = eina_file_path_sanitize(file);
    if (!tmp) return NULL; // Sanitization failed.

    // Attempt to find the desktop file in the cache first.
    eina_lock_take(&_lock);
    desktop = efreet_cache_desktop_find(tmp);
    free(tmp); // Sanitized path no longer needed.
    if (desktop)
    {
        // Found in cache, increment ref count.
        desktop->ref++;
        eina_lock_release(&_lock);
        // Check if this desktop entry is relevant for the current environment.
        if (!efreet_desktop_environment_check(desktop))
        {
            // Not relevant, free it and return NULL.
            efreet_desktop_free(desktop);
            return NULL;
        }
        return desktop; // Return cached and relevant desktop.
    }
    eina_lock_release(&_lock);
    // Not found in cache, create a new one from disk.
    return efreet_desktop_uncached_new(file);
}

EAPI Efreet_Desktop *
efreet_desktop_uncached_new(const char *file)
{
    Efreet_Desktop *desktop = NULL;
    char *tmp; // Temporary storage for sanitized file path.

    EINA_SAFETY_ON_NULL_RETURN_VAL(file, NULL);
    // Ensure the file actually exists before trying to parse it.
    if (!ecore_file_exists(file)) return NULL;

    // Sanitize the file path.
    tmp = eina_file_path_sanitize(file);
    if (!tmp) return NULL;

    // Allocate a new Efreet_Desktop structure.
    desktop = NEW(Efreet_Desktop, 1);
    if (!desktop)
    {
        free(tmp); // Clean up sanitized path if allocation fails.
        return NULL;
    }
    desktop->orig_path = tmp; // Store the sanitized path.
    desktop->ref = 1; // Initialize reference count.

    // Read and parse the .desktop file contents.
    if (!efreet_desktop_read(desktop))
    {
        // Parsing failed, free the allocated desktop structure.
        efreet_desktop_free(desktop);
        return NULL;
    }

    return desktop;
}

EAPI int
efreet_desktop_save(Efreet_Desktop *desktop)
{
    Efreet_Desktop_Type_Info *info; // Information about the desktop entry type.
    Efreet_Ini *ini; // Handle for INI file operations.
    int ok = 1; // Flag to track success of operations.

    EINA_SAFETY_ON_NULL_RETURN_VAL(desktop, 0);

    // Create a new INI structure to build the .desktop file content.
    ini = efreet_ini_new(NULL);
    if (!ini) return 0; // Failed to create INI structure.
    // Ensure the main "Desktop Entry" section exists and is active.
    efreet_ini_section_add(ini, "Desktop Entry");
    efreet_ini_section_set(ini, "Desktop Entry");

    // Get type information to save type-specific fields.
    info = eina_list_nth(efreet_desktop_types, desktop->type);
    if (info)
    {
        // Set the "Type" field.
        efreet_ini_string_set(ini, "Type", info->type);
        // If a custom save function exists for this type, call it.
        if (info->save_func) info->save_func(desktop, ini);
    }
    else
        ok = 0; // Unknown type, cannot save correctly.

    if (ok)
    {
        char *val; // Temporary string for joined lists.

        // Save "OnlyShowIn" list if present.
        if (desktop->only_show_in)
        {
            val = efreet_desktop_string_list_join(desktop->only_show_in);
            if (val)
            {
                efreet_ini_string_set(ini, "OnlyShowIn", val);
                FREE(val);
            }
        }
        // Save "NotShowIn" list if present.
        if (desktop->not_show_in)
        {
            val = efreet_desktop_string_list_join(desktop->not_show_in);
            if (val)
            {
                efreet_ini_string_set(ini, "NotShowIn", val);
                FREE(val);
            }
        }
        // Save generic fields common to all types.
        efreet_desktop_generic_fields_save(desktop, ini);
        /* When we save the file, it should be updated to the
         * latest version that we support! */
        // Set the "Version" field to the current spec version.
        efreet_ini_string_set(ini, "Version", DESKTOP_VERSION);

        // Attempt to save the INI structure to the original file path.
        if (!efreet_ini_save(ini, desktop->orig_path)) ok = 0;
    }
    // Free the INI structure.
    efreet_ini_free(ini);
    return ok;
}

EAPI int
efreet_desktop_save_as(Efreet_Desktop *desktop, const char *file)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(desktop, 0);
    EINA_SAFETY_ON_NULL_RETURN_VAL(file, 0);

    /* If we save data from eet as new, we will be in trouble */
    // Saving a desktop entry that was loaded from the Eet cache to a new file path
    // is problematic because the cached entry might not have all data in mutable memory.
    if (desktop->eet) return 0;

    // Update the desktop's original path to the new file path.
    IF_FREE(desktop->orig_path);
    desktop->orig_path = strdup(file);
    // Call the standard save function.
    return efreet_desktop_save(desktop);
}

EAPI void
efreet_desktop_free(Efreet_Desktop *desktop)
{
    if (!desktop) return; // Nothing to free.

    // Thread-safe decrement of the reference counter.
    eina_lock_take(&_lock);
    desktop->ref--;
    // If still referenced, do not free yet.
    if (desktop->ref > 0)
    {
        eina_lock_release(&_lock);
        return;
    }

    // If the desktop entry was loaded from Eet cache, use cache-specific free.
    if (desktop->eet)
    {
        efreet_cache_desktop_free(desktop);
    }
    else // Otherwise, free all allocated members manually.
    {
        Efreet_Desktop_Action *action;

        /* Desktop Spec 1.0 fields */
        IF_FREE(desktop->orig_path);

        IF_FREE(desktop->version);
        IF_FREE(desktop->name);
        IF_FREE(desktop->generic_name);
        IF_FREE(desktop->comment);
        IF_FREE(desktop->icon);
        IF_FREE(desktop->url);

        IF_FREE(desktop->try_exec);
        IF_FREE(desktop->exec);
        IF_FREE(desktop->path);
        IF_FREE(desktop->startup_wm_class);

        // Free lists of stringshared strings.
        IF_FREE_LIST(desktop->only_show_in, eina_stringshare_del);
        IF_FREE_LIST(desktop->not_show_in, eina_stringshare_del);

        IF_FREE_LIST(desktop->categories, eina_stringshare_del);
        IF_FREE_LIST(desktop->mime_types, eina_stringshare_del);

        // Free the hash table for X- fields.
        IF_FREE_HASH(desktop->x);

        // Free type-specific data if a free function is provided.
        if (desktop->type_data)
        {
            Efreet_Desktop_Type_Info *info;
            info = eina_list_nth(efreet_desktop_types, desktop->type);
            if (info && info->free_func) // Check info for safety
                info->free_func(desktop->type_data);
        }

        /* Desktop Spec 1.1 fields */
        // Free the list of actions.
        EINA_LIST_FREE(desktop->actions, action)
        {
            free(action->key);
            free(action->name);
            free(action->icon);
            free(action->exec);
            free(action);
        }
        IF_FREE_LIST(desktop->implements, eina_stringshare_del);
        IF_FREE_LIST(desktop->keywords, eina_stringshare_del);

        // Finally, free the Efreet_Desktop structure itself.
        free(desktop);
    }
    eina_lock_release(&_lock);
}

/**
 * @brief Sets the global desktop environment name.
 * @param environment The name of the desktop environment (e.g., "Enlightenment", "GNOME").
 * This string is stringshared.
 */
EAPI void
efreet_desktop_environment_set(const char *environment)
{
   // Update the global stringshared variable for the desktop environment.
   eina_stringshare_replace(&desktop_environment, environment);
}

/**
 * @brief Gets the global desktop environment name.
 * @return The current desktop environment name. This is a stringshared string, do not free.
 */
EAPI const char *
efreet_desktop_environment_get(void)
{
    return desktop_environment;
}

EAPI unsigned int
efreet_desktop_category_count_get(Efreet_Desktop *desktop)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(desktop, 0);
    // Return the number of elements in the categories list.
    return eina_list_count(desktop->categories);
}

EAPI void
efreet_desktop_category_add(Efreet_Desktop *desktop, const char *category)
{
    EINA_SAFETY_ON_NULL_RETURN(desktop);
    EINA_SAFETY_ON_NULL_RETURN(category);

    // Avoid adding duplicate categories.
    if (eina_list_search_unsorted(desktop->categories,
                                  EINA_COMPARE_CB(strcmp), category)) return;

    // Thread-safe addition of the category.
    eina_lock_take(&_lock);
    // Add the stringshared category to the list.
    desktop->categories = eina_list_append(desktop->categories,
                        (void *)eina_stringshare_add(category));
    eina_lock_release(&_lock);
}

EAPI int
efreet_desktop_category_del(Efreet_Desktop *desktop, const char *category)
{
    char *found = NULL; // To store the found category string.

    EINA_SAFETY_ON_NULL_RETURN_VAL(desktop, 0);

    // Search for the category in the list.
    if ((found = eina_list_search_unsorted(desktop->categories,
                                           EINA_COMPARE_CB(strcmp), category)))
    {
        // Category found, remove it in a thread-safe manner.
        eina_lock_take(&_lock);
        desktop->categories = eina_list_remove(desktop->categories, found);
        eina_stringshare_del(found); // Release the stringshare reference.
        eina_lock_release(&_lock);

        return 1; // Successfully deleted.
    }

    return 0;
}

EAPI int
efreet_desktop_type_add(const char *type, Efreet_Desktop_Type_Parse_Cb parse_func,
                        Efreet_Desktop_Type_Save_Cb save_func,
                        Efreet_Desktop_Type_Free_Cb free_func)
{
    int id; // ID for the new type.
    Efreet_Desktop_Type_Info *info; // Structure to hold type information.

    // Allocate new type information structure.
    info = NEW(Efreet_Desktop_Type_Info, 1);
    if (!info) return 0; // Allocation failed.

    // Assign a new ID based on the current number of types.
    id = eina_list_count(efreet_desktop_types);

    info->id = id;
    info->type = eina_stringshare_add(type); // Store type string (stringshared).
    info->parse_func = parse_func; // Store callback functions.
    info->save_func = save_func;
    info->free_func = free_func;

    // Add the new type information to the global list of types.
    // This operation should ideally be locked if accessed concurrently,
    // but type addition is typically done at init time.
    efreet_desktop_types = eina_list_append(efreet_desktop_types, info);

    return id;
}

EAPI int
efreet_desktop_type_alias(int from_type, const char *alias)
{
    Efreet_Desktop_Type_Info *info;
    // Retrieve the type information for the existing type.
    info = eina_list_nth(efreet_desktop_types, from_type);
    if (!info) return -1; // Original type not found.

    // Add a new type with the alias name, using the same callbacks as the original type.
    return efreet_desktop_type_add(alias, info->parse_func, info->save_func, info->free_func);
}

EAPI Eina_Bool
efreet_desktop_x_field_set(Efreet_Desktop *desktop, const char *key, const char *data)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(desktop, EINA_FALSE);
    // Key must start with "X-"
    EINA_SAFETY_ON_TRUE_RETURN_VAL(strncmp(key, "X-", 2), EINA_FALSE);
    // Data should not be an empty string if not NULL.
    EINA_SAFETY_ON_TRUE_RETURN_VAL(data && (!data[0]), EINA_FALSE);

    eina_lock_take(&_lock);
    // Initialize the hash for X- fields if it doesn't exist.
    if (!desktop->x)
        desktop->x = eina_hash_string_superfast_new(EINA_FREE_CB(eina_stringshare_del));

    // Remove existing key to ensure update, then add new (or updated) value.
    eina_hash_del_by_key(desktop->x, key);
    eina_hash_add(desktop->x, key, eina_stringshare_add(data));
    eina_lock_release(&_lock);

    return EINA_TRUE;
}

EAPI const char *
efreet_desktop_x_field_get(Efreet_Desktop *desktop, const char *key)
{
    const char *ret;

    EINA_SAFETY_ON_NULL_RETURN_VAL(desktop, NULL);
    // Key must start with "X-"
    EINA_SAFETY_ON_TRUE_RETURN_VAL(strncmp(key, "X-", 2), NULL);
    if (!desktop->x) return NULL; // No X- fields present.

    eina_lock_take(&_lock);
    ret = eina_hash_find(desktop->x, key);
    // Stringshare the result as the caller might hold onto it.
    ret = eina_stringshare_add(ret);
    eina_lock_release(&_lock);
    // Defensive check: if an empty string was somehow stored for a key,
    // treat it as if the key doesn't exist and remove it.
    if (ret && (!ret[0]))
      {
         /* invalid null key somehow accepted; remove */
         efreet_desktop_x_field_del(desktop, key);
         eina_stringshare_replace(&ret, NULL); // Frees the empty string and sets ret to NULL
      }

    return ret;
}

EAPI Eina_Bool
efreet_desktop_x_field_del(Efreet_Desktop *desktop, const char *key)
{
    Eina_Bool ret;
    EINA_SAFETY_ON_NULL_RETURN_VAL(desktop, EINA_FALSE);
    // Key must start with "X-"
    EINA_SAFETY_ON_TRUE_RETURN_VAL(strncmp(key, "X-", 2), EINA_FALSE);
    if (!desktop->x) return EINA_FALSE; // No X- fields present.

    eina_lock_take(&_lock);
    // Delete the key from the hash.
    ret = eina_hash_del_by_key(desktop->x, key);
    eina_lock_release(&_lock);
    return ret;
}

EAPI void *
efreet_desktop_type_data_get(Efreet_Desktop *desktop)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(desktop, NULL);
    // Return the opaque pointer to type-specific data.
    return desktop->type_data;
}

EAPI Eina_List *
efreet_desktop_string_list_parse(const char *string)
{
    Eina_List *list = NULL; // The list of parsed strings.
    char *tmp; // Temporary mutable copy of the input string.
    char *s, *p; // Pointers for string tokenization.
    size_t len;

    EINA_SAFETY_ON_NULL_RETURN_VAL(string, NULL);

    // Create a mutable copy of the string on the stack.
    len = strlen(string) + 1;
    tmp = alloca(len);
    memcpy(tmp, string, len);
    s = tmp; // Start of the current token.

    // Iterate through the string, splitting by unescaped semicolons.
    while ((p = strchr(s, ';')))
    {
        // Check for escaped semicolon: '\;' should not be a delimiter.
        if (p > tmp && *(p-1) == '\\')
        {
            // This logic is currently flawed as it would just skip the check
            // and find the next ';'. Proper unescaping should happen when
            // processing the token. However, the spec does not define escaping
            // for semicolons in string lists. This check might be vestigial
            // or intended for a different context. For standard desktop entries,
            // semicolons are strict delimiters.
            // For now, we assume simple splitting.
            // To correctly handle '\;', one would need to memmove or copy,
            // but given the spec, this is likely not needed.
            // The `continue` here means if `\;` is found, `p` is advanced by `strchr`
            // in the next iteration, effectively treating `\;` as part of the string if
            // no other `;` follows, or splitting at the next unescaped `;`.
            // This is likely not the intended unescaping behavior.
            // The spec says: "Fields of type string list are a list of strings separated by semicolons."
            // "Literal semicolons can be escaped as “\;”." - This implies unescaping should occur.
            // However, this function only splits. The stringshare_add will take the string as is.
            // This function seems to only split by ';', not unescape.
            // The `if (p > tmp && *(p-1) == '\\') continue;` line is problematic for actual unescaping.
            // It should be `if (p > s && *(p-1) == '\\') { s = p + 1; continue; }` to skip over `\;`
            // and continue search from there, but that would include `\` in the string.
            // The current code will split "foo\;bar;baz" into "foo\;bar" and "baz".
            // If the intention was to treat "\;" as a literal semicolon within a token,
            // the splitting logic would need to be more complex, or unescaping done later.
            // Given the function's role, it's more likely that it's just a simple tokenizer.
        }
        *p = '\0'; // Terminate the current token.
        list = eina_list_append(list, (void *)eina_stringshare_add(s)); // Add token to list.
        s = p + 1; // Move to the start of the next potential token.
    }
    /* If this is true, the .desktop file does not follow the standard */
    // This handles the case where the string list does not end with a semicolon,
    // which is common (e.g., "foo;bar" instead of "foo;bar;").
    if (*s) // If there's a remaining part of the string.
    {
#ifdef STRICT_SPEC
        WRN("Found a string list without ';' at the end: '%s'", string);
#endif
        list = eina_list_append(list, (void *)eina_stringshare_add(s)); // Add the last token.
    }

    return list;
}

EAPI char *
efreet_desktop_string_list_join(Eina_List *list)
{
    Eina_List *l; // Iterator for the list.
    const char *elem; // Current string element from the list.
    char *string; // The joined string to be returned.
    size_t size, pos, len; // Variables for managing buffer size and position.

    if (!list) return strdup(""); // Return empty string if list is NULL.

    // Initial allocation for the joined string.
    size = 1024;
    string = malloc(size);
    if (!string) return NULL; // Allocation failed.
    pos = 0; // Current position in the string buffer.

    // Iterate through the list and append each element followed by a semicolon.
    EINA_LIST_FOREACH(list, l, elem)
    {
        len = strlen(elem);
        /* +1 for ';' */
        // Ensure buffer is large enough for the current element, semicolon, and null terminator.
        if ((len + pos + 1 + 1) >= size) // +1 for ';', +1 for '\0'
        {
            char *tmp;
            // Reallocate with more space.
            size = len + pos + 1024; // Add extra space to reduce realloc frequency.
            tmp = realloc(string, size);
            if (!tmp)
            {
                free(string); // Free original buffer if realloc fails.
                return NULL;
            }
            string = tmp;
        }
        strcpy(string + pos, elem); // Copy element to buffer.
        pos += len;
        strcpy(string + pos, ";"); // Append semicolon.
        pos += 1;
    }
    // Null-terminate the string. If pos is 0 (empty list processed), string[0] = '\0' is needed.
    // If list was not empty, the last char is ';', so string[pos-1] = ';'
    // The spec implies "foo;bar;", so this is correct.
    // If an empty string is desired for an empty list, the initial strdup("") handles it.
    // If the list had elements, string[pos] should be '\0'.
    if (pos > 0) string[pos] = '\0'; // Ensure null termination.
    else string[0] = '\0'; // Handle case where list was empty but not NULL.
    return string;
}

/**
 * @internal
 * @param desktop The desktop to fill
 * @return Returns 1 on success, 0 on failure
 * @brief initialize an Efreet_Desktop from the contents of @a file
 */
static int
efreet_desktop_read(Efreet_Desktop *desktop)
{
    Efreet_Ini *ini;
    int error = 0;
    int ok;

    ini = efreet_ini_new(desktop->orig_path);
    if (!ini) return 0;
    if (!ini->data)
    {
        efreet_ini_free(ini);
        return 0;
    }

    ok = efreet_ini_section_set(ini, "Desktop Entry");
    if (!ok) ok = efreet_ini_section_set(ini, "KDE Desktop Entry");
    if (!ok)
    {
        ERR("no Desktop Entry section in file '%s'", desktop->orig_path);
        error = 1;
    }

    if (!error)
    {
        Efreet_Desktop_Type_Info *info;

        info = efreet_desktop_type_parse(efreet_ini_string_get(ini, "Type"));
        if (info)
        {
            const char *val;

            desktop->type = info->id;
            val = efreet_ini_string_get(ini, "Version");
            if (val) desktop->version = strdup(val);

            if (info->parse_func)
                desktop->type_data = info->parse_func(desktop, ini);
        }
        else
            error = 1;
    }

    if (!error && !efreet_desktop_generic_fields_parse(desktop, ini)) error = 1;
    if (!error && !efreet_desktop_environment_check(desktop)) error = 1;
    if (!error)
        eina_hash_foreach(ini->section, efreet_desktop_x_fields_parse, desktop);

    efreet_ini_free(ini);

    desktop->load_time = ecore_file_mod_time(desktop->orig_path);

    if (error) return 0;

    return 1;
}

/**
 * @internal
 * @param type_str the type as a string
 * @return the parsed type
 * @brief parse the type string into an Efreet_Desktop_Type
 */
static Efreet_Desktop_Type_Info *
efreet_desktop_type_parse(const char *type_str)
{
    Efreet_Desktop_Type_Info *info;
    Eina_List *l;

    if (!type_str) return NULL;

    EINA_LIST_FOREACH(efreet_desktop_types, l, info)
    {
        if (!strcmp(info->type, type_str))
            return info;
    }

    return NULL;
}

/**
 * @internal
 * @brief Free an Efreet Desktop_Type_Info struct
 */
static void
efreet_desktop_type_info_free(Efreet_Desktop_Type_Info *info)
{
    if (!info) return;
    IF_RELEASE(info->type);
    free(info);
}

/**
 * @internal
 * @param desktop the Efreet_Desktop to store parsed fields in
 * @param ini the Efreet_Ini to parse fields from
 * @return No value
 * @brief Parse application specific desktop fields
 */
static void *
efreet_desktop_application_fields_parse(Efreet_Desktop *desktop, Efreet_Ini *ini)
{
    const char *val;

    /* Desktop Spec 1.0 */
    val = efreet_ini_string_get(ini, "TryExec");
    if (val) desktop->try_exec = strdup(val);

    val = efreet_ini_string_get(ini, "Exec");
    if (val) desktop->exec = strdup(val);

    val = efreet_ini_string_get(ini, "Path");
    if (val) desktop->path = strdup(val);

    val = efreet_ini_string_get(ini, "StartupWMClass");
    if ((val) && (val[0]) && (val[1]))
    {
        size_t len = strlen(val);
        if (((val[0] == '"')  && (val[len - 1] == '"') ) ||
            ((val[0] == '\'') && (val[len - 1] == '\'')))
        {
            // fixup for some spec-violating apps that put startupwmclass
            // in quotes... spec doesnt allow for this. just escapes.
            char *tmpval = alloca(len - 1);
            strncpy(tmpval, val + 1, len - 2);
            tmpval[len - 2] = '\0';
            desktop->startup_wm_class = strdup(tmpval);
        }
        else
            desktop->startup_wm_class = strdup(val);
    }

    val = efreet_ini_string_get(ini, "Categories");
    if (val)
      desktop->categories = efreet_desktop_string_list_parse(val);
    val = efreet_ini_string_get(ini, "MimeType");
    if (val) desktop->mime_types = efreet_desktop_string_list_parse(val);

    desktop->terminal = efreet_ini_boolean_get(ini, "Terminal");
    desktop->startup_notify = efreet_ini_boolean_get(ini, "StartupNotify");

    /* Desktop Spec 1.1 */
    val = efreet_ini_string_get(ini, "Actions");
    if (val)
        desktop->actions = efreet_desktop_action_fields_parse(ini, val);
    val = efreet_ini_string_get(ini, "Keywords");
    if (val)
        desktop->keywords = efreet_desktop_string_list_parse(val);

    return NULL;
}

/**
 * @internal
 * @param key the key to look up Desktop Action entry
 * @return list of Desktop Actions
 */
static Eina_List *
efreet_desktop_action_fields_parse(Efreet_Ini *ini, const char *actions)
{
    Eina_List *l;
    Eina_List *ret = NULL;
    const char *section;
    const char *key;

    // TODO: section = efreet_ini_section_get(ini);
    section = "Desktop Entry";

    l = efreet_desktop_string_list_parse(actions);
    EINA_LIST_FREE(l, key)
    {
        char entry[4096];
        Efreet_Desktop_Action *act;

        snprintf(entry, sizeof(entry), "Desktop Action %s", key);

        if (!efreet_ini_section_set(ini, entry)) continue;

        act = NEW(Efreet_Desktop_Action, 1);
        if (!act) continue;
        ret = eina_list_append(ret, act);
        act->key = strdup(key);
        act->name = eina_strdup(efreet_ini_localestring_get(ini, "Name"));
        act->icon = eina_strdup(efreet_ini_localestring_get(ini, "Icon"));
        act->exec = eina_strdup(efreet_ini_string_get(ini, "Exec"));

        /* TODO: Non-standard keys OnlyShowIn, NotShowIn used by Unity */

        eina_stringshare_del(key);
    }
    efreet_ini_section_set(ini, section);
    return ret;
}

/**
 * @internal
 * @param desktop the Efreet_Desktop to save fields from
 * @param ini the Efreet_Ini to save fields to
 * @return Returns no value
 * @brief Save application specific desktop fields
 */
static void
efreet_desktop_application_fields_save(Efreet_Desktop *desktop, Efreet_Ini *ini)
{
    char *val;

    /* Desktop Spec 1.0 */
    if (desktop->try_exec)
        efreet_ini_string_set(ini, "TryExec", desktop->try_exec);

    if (desktop->exec)
        efreet_ini_string_set(ini, "Exec", desktop->exec);

    if (desktop->path)
        efreet_ini_string_set(ini, "Path", desktop->path);

    if (desktop->startup_wm_class)
        efreet_ini_string_set(ini, "StartupWMClass", desktop->startup_wm_class);

    if (desktop->categories)
    {
        val = efreet_desktop_string_list_join(desktop->categories);
        if (val)
        {
            efreet_ini_string_set(ini, "Categories", val);
            FREE(val);
        }
    }

    if (desktop->mime_types)
    {
        val = efreet_desktop_string_list_join(desktop->mime_types);
        if (val)
        {
           efreet_ini_string_set(ini, "MimeType", val);
           FREE(val);
        }
    }

    efreet_ini_boolean_set(ini, "Terminal", desktop->terminal);
    efreet_ini_boolean_set(ini, "StartupNotify", desktop->startup_notify);

    /* Desktop Spec 1.1 */
    if (desktop->actions)
        efreet_desktop_action_fields_save(desktop, ini);
    if (desktop->keywords)
    {
        val = efreet_desktop_string_list_join(desktop->keywords);
        if (val)
        {
            efreet_ini_string_set(ini, "Keywords", val);
            free(val);
        }
    }
}

/**
 * @internal
 * @param desktop the Efreet_Desktop to save fields from
 * @param ini the Efreet_Ini to save fields to
 * @return Returns no value
 * @brief Save action specific desktop fields
 */
static void
efreet_desktop_action_fields_save(Efreet_Desktop *desktop, Efreet_Ini *ini)
{
    Eina_List *actions = NULL, *l;
    const char *section;
    char *join;
    Efreet_Desktop_Action *action;

    // TODO: section = efreet_ini_section_get(ini);
    section = "Desktop Entry";

    EINA_LIST_FOREACH(desktop->actions, l, action)
    {
        char entry[4096];

        actions = eina_list_append(actions, action->key);
        snprintf(entry, sizeof(entry), "Desktop Action %s", action->key);
        efreet_ini_section_add(ini, entry);
        efreet_ini_section_set(ini, entry);

        efreet_ini_localestring_set(ini, "Name", action->name);
        efreet_ini_localestring_set(ini, "Icon", action->icon);
        efreet_ini_string_set(ini, "Exec", action->exec);
    }
    efreet_ini_section_set(ini, section);
    join = efreet_desktop_string_list_join(actions);
    if (join)
    {
        efreet_ini_string_set(ini, "Actions", join);
        free(join);
    }
    eina_list_free(actions);
}

/**
 * @internal
 * @param desktop the Efreet_Desktop to store parsed fields in
 * @param ini the Efreet_Ini to parse fields from
 * @return Returns no value
 * @brief Parse link specific desktop fields
 */
static void *
efreet_desktop_link_fields_parse(Efreet_Desktop *desktop, Efreet_Ini *ini)
{
    const char *val;

    val = efreet_ini_string_get(ini, "URL");
    if (val) desktop->url = strdup(val);
    return NULL;
}

/**
 * @internal
 * @param desktop the Efreet_Desktop to save fields from
 * @param ini the Efreet_Ini to save fields in
 * @return Returns no value
 * @brief Save link specific desktop fields
 */
static void
efreet_desktop_link_fields_save(Efreet_Desktop *desktop, Efreet_Ini *ini)
{
    if (desktop->url) efreet_ini_string_set(ini, "URL", desktop->url);
}

/**
 * @internal
 * @param desktop the Efreet_Desktop to store parsed fields in
 * @param ini the Efreet_Ini to parse fields from
 * @return 1 if parsed successfully, 0 otherwise
 * @brief Parse desktop fields that all types can include
 */
static int
efreet_desktop_generic_fields_parse(Efreet_Desktop *desktop, Efreet_Ini *ini)
{
    const char *val;

    /* Desktop Spec 1.0 */
    val = efreet_ini_localestring_get(ini, "Name");
#ifndef STRICT_SPEC
    if (!val) val = efreet_ini_localestring_get(ini, "_Name");
#endif
    if (val) desktop->name = strdup(val);
    else
    {
        ERR("no Name or _Name fields in file '%s'", desktop->orig_path);
        return 0;
    }

    val = efreet_ini_localestring_get(ini, "GenericName");
    if (val) desktop->generic_name = strdup(val);

    val = efreet_ini_localestring_get(ini, "Comment");
#ifndef STRICT_SPEC
    if (!val) val = efreet_ini_localestring_get(ini, "_Comment");
#endif
    if (val) desktop->comment = strdup(val);

    val = efreet_ini_localestring_get(ini, "Icon");
    if (val) desktop->icon = strdup(val);

    desktop->no_display = efreet_ini_boolean_get(ini, "NoDisplay");
    desktop->hidden = efreet_ini_boolean_get(ini, "Hidden");

    val = efreet_ini_string_get(ini, "OnlyShowIn");
    if (val) desktop->only_show_in = efreet_desktop_string_list_parse(val);
    val = efreet_ini_string_get(ini, "NotShowIn");
    if (val) desktop->not_show_in = efreet_desktop_string_list_parse(val);

    /* Desktop Spec 1.1 */
    desktop->dbus_activatable = efreet_ini_boolean_get(ini, "DBusActivatable");
    val = efreet_ini_string_get(ini, "Implements");
    if (val) desktop->implements = efreet_desktop_string_list_parse(val);
    return 1;
}

/**
 * @internal
 * @param desktop the Efreet_Desktop to save fields from
 * @param ini the Efreet_Ini to save fields to
 * @return Returns nothing
 * @brief Save desktop fields that all types can include
 */
static void
efreet_desktop_generic_fields_save(Efreet_Desktop *desktop, Efreet_Ini *ini)
{
    const char *val;

    /* Desktop Spec 1.0 */
    if (desktop->name)
    {
        efreet_ini_localestring_set(ini, "Name", desktop->name);
        val = efreet_ini_string_get(ini, "Name");
        if (!val)
            efreet_ini_string_set(ini, "Name", desktop->name);
    }
    if (desktop->generic_name)
    {
        efreet_ini_localestring_set(ini, "GenericName", desktop->generic_name);
        val = efreet_ini_string_get(ini, "GenericName");
        if (!val)
            efreet_ini_string_set(ini, "GenericName", desktop->generic_name);
    }
    if (desktop->comment)
    {
        efreet_ini_localestring_set(ini, "Comment", desktop->comment);
        val = efreet_ini_string_get(ini, "Comment");
        if (!val)
            efreet_ini_string_set(ini, "Comment", desktop->comment);
    }
    if (desktop->icon)
    {
        efreet_ini_localestring_set(ini, "Icon", desktop->icon);
        val = efreet_ini_string_get(ini, "Icon");
        if (!val)
            efreet_ini_string_set(ini, "Icon", desktop->icon);
    }

    efreet_ini_boolean_set(ini, "NoDisplay", desktop->no_display);
    efreet_ini_boolean_set(ini, "Hidden", desktop->hidden);

    if (desktop->x) eina_hash_foreach(desktop->x, efreet_desktop_x_fields_save,
                                        ini);

    /* Desktop Spec 1.1 */
    efreet_ini_boolean_set(ini, "DBusActivatable", desktop->dbus_activatable);
    if (desktop->implements)
    {
        char *join;

        join = efreet_desktop_string_list_join(desktop->implements);
        if (join)
        {
           efreet_ini_string_set(ini, "Implements", join);
           free(join);
        }
    }
}

/**
 * @internal
 * @param node The node to work with
 * @param desktop The desktop file to work with
 * @return Returns always true, to be used in eina_hash_foreach()
 * @brief Parses out an X- key from @a node and stores in @a desktop
 */
static Eina_Bool
efreet_desktop_x_fields_parse(const Eina_Hash *hash EINA_UNUSED, const void *key, void *value, void *fdata)
{
    Efreet_Desktop * desktop = fdata;

    if (!desktop) return EINA_TRUE;
    if (strncmp(key, "X-", 2)) return EINA_TRUE;

    if (!desktop->x)
        desktop->x = eina_hash_string_superfast_new(EINA_FREE_CB(eina_stringshare_del));
    eina_hash_del_by_key(desktop->x, key);
    eina_hash_add(desktop->x, key, (void *)eina_stringshare_add(value));

    return EINA_TRUE;
}

/**
 * @internal
 * @param node The node to work with
 * @param ini The ini file to work with
 * @return Returns no value
 * @brief Stores an X- key from @a node and stores in @a ini
 */
static Eina_Bool
efreet_desktop_x_fields_save(const Eina_Hash *hash EINA_UNUSED, const void *key, void *value, void *fdata)
{
    Efreet_Ini *ini = fdata;
    efreet_ini_string_set(ini, key, value);

    return EINA_TRUE;
}


/**
 * @internal
 * @param ini The Efreet_Ini to parse values from
 * @return 1 if desktop should be included in current environement, 0 otherwise
 * @brief Determines if a desktop should be included in the current environment,
 * based on the values of the OnlyShowIn and NotShowIn fields
 */
static int
efreet_desktop_environment_check(Efreet_Desktop *desktop)
{
    Eina_List *list;
    int found = 0;
    char *val;

    if (!desktop_environment)
    {
        //if (desktop->only_show_in) return 0;
        return 1;
    }

    if (desktop->only_show_in)
    {
        EINA_LIST_FOREACH(desktop->only_show_in, list, val)
        {
            if (!strcmp(val, desktop_environment))
            {
                found = 1;
                break;
            }
        }
        return found;
    }

    if (desktop->not_show_in)
    {
        EINA_LIST_FOREACH(desktop->not_show_in, list, val)
        {
            if (!strcmp(val, desktop_environment))
            {
                found = 1;
                break;
            }
        }
        return !found;
    }

    return 1;
}

