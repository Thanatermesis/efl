#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#ifdef HAVE_SYS_RESOURCE_H
#include <sys/time.h>
#include <sys/resource.h>
#endif

#ifdef _WIN32
# include <evil_private.h> /* fcntl */
#endif

#include <Eina.h>
#include <Eet.h>
#include <Ecore.h>
#include <Ecore_File.h>

#define EFREET_MODULE_LOG_DOM _efreet_icon_cache_log_dom
static int _efreet_icon_cache_log_dom = -1;

#include "Efreet.h"
#include "efreet_private.h"
#include "efreet_cache_private.h"

#ifndef O_BINARY
# define O_BINARY 0
#endif

/** @internal Array of recognized icon file extensions (e.g., ".png", ".svg"). */
static Eina_Array *exts = NULL;
/** @internal Array of additional directories to scan for icons. */
static Eina_Array *extra_dirs = NULL;
/** @internal Array used to store all stringshared strings for later cleanup. */
static Eina_Array *strs = NULL;
/** @internal Hash table of loaded icon themes, mapping theme name (char *) to Efreet_Cache_Icon_Theme *. */
static Eina_Hash *icon_themes = NULL;

/**
 * @internal
 * @brief Checks if a directory has been modified since the last scan.
 *
 * This function compares the current state of the directory (mtime, size)
 * with a cached state. If the directory is new or has changed, its
 * information is updated in the `dirs` hash.
 *
 * @param dirs A hash table storing Efreet_Cache_Directory objects, keyed by directory path.
 *             This hash is updated if the directory is new.
 * @param dir The path to the directory to check.
 * @return EINA_TRUE if the directory was modified or is new, EINA_FALSE otherwise or on error.
 */
static Eina_Bool
cache_directory_modified(Eina_Hash *dirs, const char *dir)
{
   Efreet_Cache_Directory *dcache;
   Efreet_Cache_Check check;

   if (!dirs) return EINA_TRUE;
   if (!efreet_file_cache_fill(dir, &check)) return EINA_FALSE;
   dcache = eina_hash_find(dirs, dir);
   if (!dcache)
     {
        dcache = malloc(sizeof (Efreet_Cache_Directory));
        if (!dcache) return EINA_TRUE;
        dcache->check = check;
        eina_hash_add(dirs, dir, dcache);
     }
   else if (efreet_file_cache_check(&check, &dcache->check))
     return EINA_FALSE;
   else
     dcache->check = check;
   return EINA_TRUE;
}

/**
 * @internal
 * @brief Checks if a given file extension is a recognized icon extension.
 *
 * It iterates through the global `exts` array to find a match.
 *
 * @param ext The file extension to check (e.g., ".png").
 * @return EINA_TRUE if the extension is recognized, EINA_FALSE otherwise.
 */
static Eina_Bool
cache_extension_lookup(const char *ext)
{
   unsigned int i;

   for (i = 0; i < exts->count; ++i)
     {
        if (!strcmp(exts->data[i], ext)) return EINA_TRUE;
     }
   return EINA_FALSE;
}

/**
 * @internal
 * @brief Scans a single directory for fallback icons.
 *
 * This function iterates through files in the given directory. If a file has a
 * recognized icon extension, it's added to the `icons` hash.
 * It also checks if the directory itself has been modified using `cache_directory_modified`.
 *
 * @param icons A hash table to store found Efreet_Cache_Fallback_Icon objects, keyed by icon name (filename without extension).
 *              This hash is populated with icons found in the directory.
 *              Example: icons["my-icon"] = Efreet_Cache_Fallback_Icon for "my-icon.png".
 * @param dirs A hash table storing Efreet_Cache_Directory objects, used by `cache_directory_modified`.
 * @param dir The path to the directory to scan.
 * @return EINA_TRUE on success or if the directory wasn't modified, EINA_FALSE on critical errors (though currently always returns EINA_TRUE).
 */
static Eina_Bool
cache_fallback_scan_dir(Eina_Hash *icons, Eina_Hash *dirs, const char *dir)
{
    Eina_Iterator *it;
    Eina_File_Direct_Info *entry;

    if (!cache_directory_modified(dirs, dir)) return EINA_TRUE;

    it = eina_file_stat_ls(dir);
    if (!it) return EINA_TRUE;

    EINA_ITERATOR_FOREACH(it, entry)
    {
        Efreet_Cache_Fallback_Icon *icon;
        char *name;
        char *ext;
        unsigned int i;
        void *p;

        if (entry->type == EINA_FILE_DIR)
            continue;

        ext = strrchr(entry->path + entry->name_start, '.');
        if (!ext || !cache_extension_lookup(ext))
            continue;

        /* icon with known extension */
        name = entry->path + entry->name_start;
        *ext = '\0';

        icon = eina_hash_find(icons, name);
        if (!icon)
        {
            icon = NEW(Efreet_Cache_Fallback_Icon, 1);
            icon->theme = NULL;
            eina_hash_add(icons, name, icon);
        }

        *ext = '.';

        for (i = 0; i < icon->icons_count; ++i)
            if (!strcmp(icon->icons[i], entry->path))
                break;

        if (i != icon->icons_count)
            continue;

        p = realloc(icon->icons, sizeof (char *) * (icon->icons_count + 1));
        if (!p)
          {
             ERR("Out of memory");
             exit(1);
          }
        icon->icons = p;
        icon->icons[icon->icons_count] = eina_stringshare_add(entry->path);
        eina_array_push(strs, icon->icons[icon->icons_count++]);
    }

    eina_iterator_free(it);

    return EINA_TRUE;
}

/**
 * @internal
 * @brief Scans all standard and extra directories for fallback icons.
 *
 * This function iterates through a predefined list of locations where fallback
 * icons might be found, including user-specific directories, XDG data directories,
 * and system-wide pixmap directories. It calls `cache_fallback_scan_dir` for each.
 *
 * @param icons A hash table to store found Efreet_Cache_Fallback_Icon objects, passed to `cache_fallback_scan_dir`.
 * @param dirs A hash table storing Efreet_Cache_Directory objects, passed to `cache_fallback_scan_dir`.
 * @return EINA_TRUE always (potential errors in `cache_fallback_scan_dir` are not propagated as EINA_FALSE).
 */
static Eina_Bool
cache_fallback_scan(Eina_Hash *icons, Eina_Hash *dirs)
{
    unsigned int i;
    Eina_List *xdg_dirs, *l;
    const char *dir;
    char path[PATH_MAX];

    for (i = 0; i < extra_dirs->count; i++)
        cache_fallback_scan_dir(icons, dirs, extra_dirs->data[i]);

    cache_fallback_scan_dir(icons, dirs, efreet_icon_deprecated_user_dir_get());
    cache_fallback_scan_dir(icons, dirs, efreet_icon_user_dir_get());

    xdg_dirs = efreet_data_dirs_get();
    EINA_LIST_FOREACH(xdg_dirs, l, dir)
    {
        snprintf(path, sizeof(path), "%s/icons", dir);
        cache_fallback_scan_dir(icons, dirs, path);
    }

#ifndef STRICT_SPEC
    EINA_LIST_FOREACH(xdg_dirs, l, dir)
    {
        snprintf(path, sizeof(path), "%s/pixmaps", dir);
        cache_fallback_scan_dir(icons, dirs, path);
    }
#endif

    cache_fallback_scan_dir(icons, dirs, "/usr/local/share/pixmaps");
    cache_fallback_scan_dir(icons, dirs, "/usr/share/pixmaps");

    return EINA_TRUE;
}

/**
 * @internal
 * @brief Checks if any of the directories relevant to fallback icons have changed.
 *
 * This function is similar to `cache_fallback_scan` in terms of the directories
 * it checks, but instead of scanning for icons, it only checks for modifications
 * using `cache_directory_modified`. It also checks directories previously cached
 * within the `theme->dirs` hash.
 *
 * @param theme The fallback icon theme cache data. `theme->dirs` is used to check
 *              previously known directories and is updated by `cache_directory_modified`.
 * @return EINA_TRUE if any relevant directory has changed, EINA_FALSE otherwise.
 */
static Eina_Bool
check_fallback_changed(Efreet_Cache_Icon_Theme *theme)
{
    unsigned int i;
    Eina_List *xdg_dirs, *l;
    const char *dir;
    char path[PATH_MAX];

    /* Check if the dirs we have cached are changed */
    if (theme->dirs)
    {
        Eina_Iterator *it;
        Eina_Bool changed = EINA_FALSE;

        it = eina_hash_iterator_key_new(theme->dirs);
        EINA_ITERATOR_FOREACH(it, dir)
        {
            changed = !ecore_file_exists(dir);
            if (changed) break;
            changed = cache_directory_modified(theme->dirs, dir);
            if (changed) break;
        }
        eina_iterator_free(it);
        if (changed) return EINA_TRUE;
    }

    /* Check if spec dirs have changed */
    for (i = 0; i < extra_dirs->count; i++)
        if (cache_directory_modified(theme->dirs, extra_dirs->data[i])) return EINA_TRUE;

    if (cache_directory_modified(theme->dirs, efreet_icon_deprecated_user_dir_get())) return EINA_TRUE;
    if (cache_directory_modified(theme->dirs, efreet_icon_user_dir_get())) return EINA_TRUE;

    xdg_dirs = efreet_data_dirs_get();
    EINA_LIST_FOREACH(xdg_dirs, l, dir)
    {
        snprintf(path, sizeof(path), "%s/icons", dir);
        if (cache_directory_modified(theme->dirs, path)) return EINA_TRUE;
    }

#ifndef STRICT_SPEC
    EINA_LIST_FOREACH(xdg_dirs, l, dir)
    {
        snprintf(path, sizeof(path), "%s/pixmaps", dir);
        if (cache_directory_modified(theme->dirs, path)) return EINA_TRUE;
    }
#endif

    if (cache_directory_modified(theme->dirs, "/usr/local/share/pixmaps")) return EINA_TRUE;
    if (cache_directory_modified(theme->dirs, "/usr/share/pixmaps")) return EINA_TRUE;
    return EINA_FALSE;
}

/**
 * @internal
 * @brief Structure to hold information about a scanned file entry.
 * Used to cache the results of `eina_file_stat_ls` to avoid repeated directory listings.
 */
typedef struct
{
  char *path;        /**< Full path to the file, duplicated string. */
  int name_start;    /**< Offset of the filename within the path. */
} Scanned_Entry;

/** @internal Hash table to cache directory listings. Keys are directory paths (char *), values are Eina_List * of Scanned_Entry *. A special value of (void *)(intptr_t)(-1L) indicates the directory was scanned and found empty or inaccessible. */
static Eina_Hash *already_scanned_path = NULL;

/**
 * @internal
 * @brief Verifies if any directories within a specific icon theme have changed.
 *
 * This function iterates through the directories listed in `theme->theme.directories`
 * (which are relative paths within the theme) and checks each one for modifications
 * using `cache_directory_modified`. If any directory has changed, `theme->changed`
 * is set to EINA_TRUE.
 *
 * @param theme The icon theme cache data to check and potentially update.
 *              `theme->dirs` is used by `cache_directory_modified`.
 *              `theme->changed` is set if modifications are found.
 */
static void
cache_theme_change_verify(Efreet_Cache_Icon_Theme *theme)
{
   Eina_Bool changed = EINA_FALSE;
   Eina_List *l;
   Efreet_Icon_Theme_Directory *d;
   char buf[PATH_MAX], *tdir, *sep;

   tdir = strdup(theme->path);
   sep = strrchr(tdir, '/');
   if (sep) *sep = 0;
   EINA_LIST_FOREACH(theme->theme.directories, l, d)
     {
        snprintf(buf, sizeof(buf), "%s/%s", tdir, d->name);
        if (cache_directory_modified(theme->dirs, buf))
          {
             changed = EINA_TRUE;
          }
     }
   free(tdir);
   if (changed) theme->changed = changed;
}

/**
 * @internal
 * @brief Scans a specific subdirectory within an icon theme for icons.
 *
 * This function processes a single directory (e.g., "48x48/apps") within a theme's path.
 * It lists files, filters by recognized extensions, and populates the `icons` hash
 * with `Efreet_Cache_Icon` objects. It uses `already_scanned_path` to cache
 * directory listings to avoid redundant `eina_file_stat_ls` calls.
 *
 * @param theme The current icon theme being processed (used for its name).
 * @param path The base path of the icon theme (e.g., "/usr/share/icons/Adwaita").
 * @param dir An `Efreet_Icon_Theme_Directory` structure describing the subdirectory
 *            to scan (e.g., its name "48x48/apps", type, size).
 * @param icons A hash table to store found `Efreet_Cache_Icon` objects, keyed by icon name.
 *              This hash is populated or updated with icons found.
 *              Example: icons["firefox"] = Efreet_Cache_Icon for "firefox.png".
 * @return EINA_TRUE on success, EINA_FALSE on critical error (though currently often returns EINA_TRUE even after some errors).
 */
static Eina_Bool
cache_scan_path_dir(Efreet_Icon_Theme *theme,
                    const char *path,
                    Efreet_Icon_Theme_Directory *dir,
                    Eina_Hash *icons)
{
    Eina_Iterator *it;
    char buf[PATH_MAX];
    Eina_File_Direct_Info *entry;
    Eina_List *dirs = NULL;
    Eina_List *l;
    char *ext;
    Scanned_Entry *scentry;

    snprintf(buf, sizeof(buf), "%s/%s", path, dir->name);
    // we wont ever free this - no point
    if (!already_scanned_path)
      already_scanned_path = eina_hash_string_superfast_new(NULL);
    dirs = eina_hash_find(already_scanned_path, buf);
    if ((intptr_t)dirs == (intptr_t)(-1L)) return EINA_TRUE;
    else if (!dirs)
    {
       it = eina_file_stat_ls(buf);
       if (!it)
         {
            eina_hash_add(already_scanned_path, buf, (void *)(intptr_t)(-1L));
            return EINA_TRUE;
         }

       EINA_ITERATOR_FOREACH(it, entry)
         {
            if (entry->type == EINA_FILE_DIR) continue;
            ext = strrchr(entry->path + entry->name_start, '.');
            if (!ext || !cache_extension_lookup(ext)) continue;
            scentry = malloc(sizeof(Scanned_Entry));
            if (!scentry)
              {
                 ERR("Out of memory");
                 exit(1);
              }
            scentry->name_start = entry->name_start;
            scentry->path = strdup(entry->path);
            if (!scentry->path)
              {
                 ERR("Out of memory");
                 exit(1);
              }
            dirs = eina_list_append(dirs, scentry);
         }
       eina_iterator_free(it);
       if (dirs)
         eina_hash_add(already_scanned_path, buf, dirs);
       else
         eina_hash_add(already_scanned_path, buf, (void *)(intptr_t)(-1L));
    }

   EINA_LIST_FOREACH(dirs, l, scentry)
    {
        Efreet_Cache_Icon *icon;
        char *name;
        const char **tmp;
        unsigned int i;

        ext = strrchr(scentry->path + scentry->name_start, '.');
        if (!ext) continue;
        /* icon with known extension */
        name = scentry->path + scentry->name_start;
        *ext = '\0';

        icon = eina_hash_find(icons, name);
        if (!icon)
        {
            icon = NEW(Efreet_Cache_Icon, 1);
            icon->theme = eina_stringshare_add(theme->name.internal);
            eina_array_push(strs, icon->theme);
            eina_hash_add(icons, name, icon);
        }

        /* find if we have the same icon in another type */
        for (i = 0; i < icon->icons_count; ++i)
        {
            if ((icon->icons[i]->type == dir->type) &&
                (icon->icons[i]->normal == dir->size.normal) &&
                (icon->icons[i]->max == dir->size.max) &&
                (icon->icons[i]->min == dir->size.min))
                break;
        }

        *ext = '.';

        if (i != icon->icons_count)
        {
            unsigned int j;

            /* check if the path already exist */
            for (j = 0; j < icon->icons[i]->paths_count; ++j)
                if (!strcmp(icon->icons[i]->paths[j], scentry->path))
                    break;

            if (j != icon->icons[i]->paths_count)
                continue;

            /* If we are inherited, check if we already have extension */
            if (strcmp(icon->theme, theme->name.internal))
            {
                const char *ext2;
                int has_ext = 0;
                for (j = 0; j < icon->icons[i]->paths_count; ++j)
                {
                    ext2 = strrchr(icon->icons[i]->paths[j], '.');
                    if (ext2)
                    {
                        ext2++;
                        has_ext = !strcmp((ext + 1), ext2);
                        if (has_ext) break;
                    }
                }
                if (has_ext)
                    continue;
            }
        }
        /* no icon match so add a new one */
        /* only allow to add new icon for main theme
         * if we allow inherited theme to add new icons,
         * we will get weird effects when icon scales
         */
        else if (!strcmp(icon->theme, theme->name.internal))
        {
            Efreet_Cache_Icon_Element **tmp2;

            tmp2 = realloc(icon->icons,
                          sizeof(Efreet_Cache_Icon_Element *) * (++icon->icons_count));
            if (!tmp2)
            {
               ERR("Out of memory");
               exit(1);
            }
            icon->icons = tmp2;
            icon->icons[i] = NEW(Efreet_Cache_Icon_Element, 1);
            icon->icons[i]->type = dir->type;
            icon->icons[i]->normal = dir->size.normal;
            icon->icons[i]->min = dir->size.min;
            icon->icons[i]->max = dir->size.max;
            icon->icons[i]->paths = NULL;
            icon->icons[i]->paths_count = 0;
        }
        else
        {
            continue;
        }

        /* and finally store the path */
        tmp = realloc(icon->icons[i]->paths,
                      sizeof(char *) * (icon->icons[i]->paths_count + 1));
        if (!tmp)
        {
           ERR("Out of memory");
           exit(1);
        }
        icon->icons[i]->paths = tmp;
        icon->icons[i]->paths[icon->icons[i]->paths_count] = eina_stringshare_add(scentry->path);
        eina_array_push(strs, icon->icons[i]->paths[icon->icons[i]->paths_count++]);
    }
    return EINA_TRUE;
}

/**
 * @internal
 * @brief Scans all specified subdirectories within a given theme path for icons.
 *
 * Iterates over `theme->directories` (e.g., "scalable/apps", "32x32/actions")
 * and calls `cache_scan_path_dir` for each one, effectively scanning one
 * base path (e.g., "/usr/share/icons/MyTheme") of an icon theme.
 *
 * @param theme The icon theme definition, providing the list of subdirectories to scan.
 * @param icons The hash table to populate with `Efreet_Cache_Icon` objects.
 * @param path The base filesystem path for this instance of the theme (e.g., "/usr/share/icons/MyTheme").
 * @return EINA_TRUE if all subdirectory scans succeed, EINA_FALSE otherwise.
 */
static Eina_Bool
cache_scan_path(Efreet_Icon_Theme *theme, Eina_Hash *icons, const char *path)
{
    Eina_List *l;
    Efreet_Icon_Theme_Directory *dir;

    EINA_LIST_FOREACH(theme->directories, l, dir)
     {
        if (!cache_scan_path_dir(theme, path, dir, icons)) return EINA_FALSE;
     }

    return EINA_TRUE;
}

/**
 * @internal
 * @brief Recursively scans an icon theme and its inherited themes.
 *
 * This function scans the specified `theme` for icons by iterating through its
 * `theme->paths` and calling `cache_scan_path` for each. It then recursively
 * calls itself for any themes listed in `theme->inherits`.
 * The `themes` hash is used to avoid processing the same theme multiple times.
 * If a theme has no explicit inherits, "hicolor" is implicitly inherited.
 *
 * @param theme The icon theme to scan.
 * @param themes A hash table to keep track of already scanned themes (Efreet_Icon_Theme*), keyed by internal theme name.
 *               Prevents redundant scans and circular dependencies.
 * @param icons A hash table to populate with `Efreet_Cache_Icon` objects from all scanned themes.
 * @return EINA_TRUE if scanning succeeds for this theme and its inherits, EINA_FALSE otherwise.
 */
static Eina_Bool
cache_scan(Efreet_Icon_Theme *theme, Eina_Hash *themes, Eina_Hash *icons)
{
    Eina_List *l;
    const char *path;
    const char *name;

    if (!theme) return EINA_TRUE;
    if (eina_hash_find(themes, theme->name.internal)) return EINA_TRUE;
    eina_hash_direct_add(themes, theme->name.internal, theme);

    /* scan theme */
    EINA_LIST_FOREACH(theme->paths, l, path)
        if (!cache_scan_path(theme, icons, path)) return EINA_FALSE;

    /* scan inherits */
    if (theme->inherits)
    {
        EINA_LIST_FOREACH(theme->inherits, l, name)
        {
            Efreet_Icon_Theme *inherit;

            inherit = eina_hash_find(icon_themes, name);
            if (!inherit)
                INF("Theme `%s` not found for `%s`.",
                    name, theme->name.internal);
            if (!cache_scan(inherit, themes, icons)) return EINA_FALSE;
        }
    }
    else if (strcmp(theme->name.internal, "hicolor"))
    {
        theme = eina_hash_find(icon_themes, "hicolor");
        if (!cache_scan(theme, themes, icons)) return EINA_FALSE;
    }

    return EINA_TRUE;
}

/**
 * @internal
 * @brief Recursively checks if an icon theme or any of its inherited themes have changed.
 *
 * This function first checks if the `theme` itself is marked as changed (`theme->changed`).
 * If not, it recursively calls itself for any themes listed in `theme->theme.inherits`.
 * If a theme has no explicit inherits and is not "hicolor", "hicolor" is implicitly checked.
 * The actual directory modification checks are expected to have been done by
 * `cache_theme_change_verify` which sets `theme->changed`.
 *
 * @param theme The cached icon theme data to check.
 * @return EINA_TRUE if the theme or any of its inherited themes have changed, EINA_FALSE otherwise.
 */
static Eina_Bool
check_changed(Efreet_Cache_Icon_Theme *theme)
{
    Eina_List *l;
    const char *name;

    if (!theme) return EINA_FALSE;

    if (theme->changed) return EINA_TRUE;
    if (theme->theme.inherits)
    {
        EINA_LIST_FOREACH(theme->theme.inherits, l, name)
        {
            Efreet_Cache_Icon_Theme *inherit;

            inherit = eina_hash_find(icon_themes, name);
            if (!inherit)
                INF("Theme `%s` not found for `%s`.",
                        name, theme->theme.name.internal);
            if (check_changed(inherit)) return EINA_TRUE;
        }
    }
    else if (strcmp(theme->theme.name.internal, "hicolor"))
    {
        theme = eina_hash_find(icon_themes, "hicolor");
        if (check_changed(theme)) return EINA_TRUE;
    }
    return EINA_FALSE;
}

/**
 * @internal
 * @brief Creates and populates an Efreet_Icon_Theme_Directory structure from an INI file section.
 *
 * Reads properties like "Context", "Type", "Size", "MinSize", "MaxSize", "Threshold"
 * for a given directory section (e.g., "48x48/apps") in an "index.theme" file.
 *
 * @param ini The Efreet_Ini object representing the parsed "index.theme" file.
 * @param name The name of the directory section in the INI file (e.g., "48x48/apps").
 * @return A newly allocated Efreet_Icon_Theme_Directory structure, or NULL on failure.
 *         The `name` field in the returned struct is stringshared and added to `strs`.
 */
static Efreet_Icon_Theme_Directory *
icon_theme_directory_new(Efreet_Ini *ini, const char *name)
{
    Efreet_Icon_Theme_Directory *dir;
    int val;
    const char *tmp;

    if (!ini) return NULL;

    dir = NEW(Efreet_Icon_Theme_Directory, 1);
    if (!dir) return NULL;
    dir->name = eina_stringshare_add(name);
    eina_array_push(strs, dir->name);

    efreet_ini_section_set(ini, name);

    tmp = efreet_ini_string_get(ini, "Context");
    if (tmp)
    {
        if (!strcasecmp(tmp, "Actions"))
            dir->context = EFREET_ICON_THEME_CONTEXT_ACTIONS;

        else if (!strcasecmp(tmp, "Devices"))
            dir->context = EFREET_ICON_THEME_CONTEXT_DEVICES;

        else if (!strcasecmp(tmp, "FileSystems"))
            dir->context = EFREET_ICON_THEME_CONTEXT_FILESYSTEMS;

        else if (!strcasecmp(tmp, "MimeTypes"))
            dir->context = EFREET_ICON_THEME_CONTEXT_MIMETYPES;
    }

    /* Threshold is fallback  */
    dir->type = EFREET_ICON_SIZE_TYPE_THRESHOLD;

    tmp = efreet_ini_string_get(ini, "Type");
    if (tmp)
    {
        if (!strcasecmp(tmp, "Fixed"))
            dir->type = EFREET_ICON_SIZE_TYPE_FIXED;

        else if (!strcasecmp(tmp, "Scalable"))
            dir->type = EFREET_ICON_SIZE_TYPE_SCALABLE;
    }

    dir->size.normal = efreet_ini_int_get(ini, "Size");

    if (dir->type == EFREET_ICON_SIZE_TYPE_THRESHOLD)
    {
        val = efreet_ini_int_get(ini, "Threshold");
        if (val < 0) val = 2;
        dir->size.max = dir->size.normal + val;
        dir->size.min = dir->size.normal - val;
    }
    else if (dir->type == EFREET_ICON_SIZE_TYPE_SCALABLE)
    {
        val = efreet_ini_int_get(ini, "MinSize");
        if (val < 0) dir->size.min = dir->size.normal;
        else dir->size.min = val;

        val = efreet_ini_int_get(ini, "MaxSize");
        if (val < 0) dir->size.max = dir->size.normal;
        else dir->size.max = val;
    }

    return dir;
}

/**
 * @internal
 * @brief Reads and parses an "index.theme" file for a given icon theme.
 *
 * Populates the `theme->theme` (Efreet_Icon_Theme) structure with information
 * from the "index.theme" file located at `path`. This includes the theme's
 * name, comment, example icon, hidden status, inherited themes, and a list of
 * its directories (parsed by `icon_theme_directory_new`).
 * It also checks if the "index.theme" file itself has changed using `efreet_file_cache_fill`
 * and `efreet_file_cache_check`. If changed, `theme->changed` is set.
 *
 * @param theme The cache entry for the icon theme. Its `theme` sub-structure is populated.
 *              `theme->path`, `theme->check`, `theme->changed`, and `theme->valid` are updated.
 *              Various string fields are stringshared and added to `strs`.
 * @param path The full path to the "index.theme" file.
 * @return EINA_TRUE on successful parsing or if the file hasn't changed, EINA_FALSE on error (e.g., file not found, parse error).
 */
static Eina_Bool
icon_theme_index_read(Efreet_Cache_Icon_Theme *theme, const char *path)
{
    Efreet_Ini *ini;
    Efreet_Icon_Theme_Directory *dir;
    const char *tmp;
    Efreet_Cache_Check check;

    if (!theme || !path) return EINA_FALSE;

    if (!efreet_file_cache_fill(path, &check)) return EINA_FALSE;
    if (theme->path && !strcmp(theme->path, path) &&
        efreet_file_cache_check(&check, &(theme->check)))
    {
        /* no change */
        theme->valid = 1;
        return EINA_TRUE;
    }
    if (!theme->path || strcmp(theme->path, path))
    {
        theme->path = eina_stringshare_add(path);
        eina_array_push(strs, theme->path);
    }
    theme->check = check;
    theme->changed = 1;

    ini = efreet_ini_new(path);
    if (!ini) return EINA_FALSE;
    if (!ini->data)
    {
        efreet_ini_free(ini);
        return EINA_FALSE;
    }

    efreet_ini_section_set(ini, "Icon Theme");
    tmp = efreet_ini_localestring_get(ini, "Name");
    if (tmp)
    {
        theme->theme.name.name = eina_stringshare_add(tmp);
        eina_array_push(strs, theme->theme.name.name);
    }

    tmp = efreet_ini_localestring_get(ini, "Comment");
    if (tmp)
    {
        theme->theme.comment = eina_stringshare_add(tmp);
        eina_array_push(strs, theme->theme.comment);
    }

    tmp = efreet_ini_string_get(ini, "Example");
    if (tmp)
    {
        theme->theme.example_icon = eina_stringshare_add(tmp);
        eina_array_push(strs, theme->theme.example_icon);
    }

    theme->hidden = efreet_ini_boolean_get(ini, "Hidden");

    theme->valid = 1;

    /* Check the inheritance. If there is none we inherit from the hicolor theme */
    tmp = efreet_ini_string_get(ini, "Inherits");
    if (tmp)
    {
        char *t, *s, *p;
        const char *i;
        size_t len;

        len = strlen(tmp) + 1;
        t = alloca(len);
        memcpy(t, tmp, len);
        s = t;
        p = strchr(s, ',');

        while (p)
        {
            *p = '\0';

            i = eina_stringshare_add(s);
            theme->theme.inherits = eina_list_append(theme->theme.inherits, i);
            eina_array_push(strs, i);
            s = ++p;
            p = strchr(s, ',');
        }
        i = eina_stringshare_add(s);
        theme->theme.inherits = eina_list_append(theme->theme.inherits, i);
        eina_array_push(strs, i);
    }

    /* make sure this one is done last as setting the directory will change
     * the ini section ... */
    tmp = efreet_ini_string_get(ini, "Directories");
    if (tmp)
    {
        char *t, *s, *p;
        size_t len;

        len = strlen(tmp) + 1;
        t = alloca(len);
        memcpy(t, tmp, len);
        s = t;
        p = s;

        while ((p) && (*s))
        {
            p = strchr(s, ',');

            if (p) *p = '\0';

            dir = icon_theme_directory_new(ini, s);
            if (!dir) goto error;
            theme->theme.directories = eina_list_append(theme->theme.directories, dir);

            if (p) s = ++p;
        }
    }

error:
    efreet_ini_free(ini);

    return EINA_TRUE;
}

/**
 * @internal
 * @brief Scans a base directory (e.g., "/usr/share/icons" or a user's icon directory) for icon theme directories.
 *
 * For each subdirectory found (potential icon theme like "Adwaita", "Numix"):
 * - Creates or retrieves an `Efreet_Cache_Icon_Theme` from the global `icon_themes` hash.
 * - Updates its list of base paths (`theme->theme.paths`) if the current `dir/theme_name` is new.
 * - Checks for an "index.theme" file within `dir/theme_name` and parses it using `icon_theme_index_read`
 *   if the theme isn't already marked as valid or if its directory cache information needs update.
 * - Updates directory modification times in `theme->dirs`.
 *
 * @param dir The base directory to scan for themes (e.g., "~/.local/share/icons", "/usr/share/icons").
 * @return EINA_TRUE always (errors during processing of individual themes are logged but don't stop the scan).
 */
static Eina_Bool
cache_theme_scan(const char *dir)
{
    Eina_Iterator *it;
    Eina_File_Direct_Info *entry;

    it = eina_file_stat_ls(dir);
    if (!it) return EINA_TRUE;

    EINA_ITERATOR_FOREACH(it, entry)
    {
        char buf[PATH_MAX];
        Efreet_Cache_Icon_Theme *theme;
        const char *name;
        const char *path;
        Efreet_Cache_Check check;
        Efreet_Cache_Directory *d;

        if (!efreet_file_cache_fill(entry->path, &check)) continue;

        if ((entry->type != EINA_FILE_DIR) &&
            (entry->type != EINA_FILE_LNK))
            continue;

        if ((entry->type == EINA_FILE_LNK) &&
            (!ecore_file_is_dir(entry->path)))
            continue;

        name = entry->path + entry->name_start;
        theme = eina_hash_find(icon_themes, name);

        if (!theme)
        {
            theme = NEW(Efreet_Cache_Icon_Theme, 1);
            theme->theme.name.internal = eina_stringshare_add(name);
            eina_array_push(strs, theme->theme.name.internal);
            eina_hash_direct_add(icon_themes,
                          (void *)theme->theme.name.internal, theme);
            theme->changed = 1;
        }

       d = NULL;
       if (theme->dirs)
         d = eina_hash_find(theme->dirs, entry->path);
       if (!d)
         {
            if (!theme->dirs)
              theme->dirs = eina_hash_string_superfast_new(NULL);
            theme->changed = 1;
            d = NEW(Efreet_Cache_Directory, 1);
            d->check = check;
            eina_hash_add(theme->dirs, entry->path, d);
         }
       else
         {
            if (!efreet_file_cache_check(&check, &(d->check)))
              {
                 d->check = check;
                 theme->changed = 1;
              }
        }

        /* TODO: We need to handle change in order of included paths */
        if (!eina_list_search_unsorted(theme->theme.paths, EINA_COMPARE_CB(strcmp), entry->path))
        {
            path = eina_stringshare_add(entry->path);
            theme->theme.paths = eina_list_append(theme->theme.paths, path);
            eina_array_push(strs, path);
            theme->changed = 1;
        }

        /* we're already valid so no reason to check for an index.theme file */
        if (theme->valid) continue;

        /* if the index.theme file exists we parse it into the theme */
        memcpy(buf, entry->path, entry->path_length);
        memcpy(buf + entry->path_length, "/index.theme", sizeof("/index.theme"));
        if (ecore_file_exists(buf))
        {
            if (!icon_theme_index_read(theme, buf))
                theme->valid = 0;
        }
    }
    eina_iterator_free(it);
    return EINA_TRUE;
}

/**
 * @internal
 * @brief Creates and locks a lock file to ensure only one instance of the cache generator runs.
 *
 * The lock file is created at `efreet_cache_home_get()/efreet/icon_data.lock`.
 * Uses `fcntl` with `F_WRLCK` and `F_SETLK` for an advisory lock.
 *
 * @return File descriptor of the locked file on success, -1 on failure (e.g., already locked, permission error).
 */
static int
cache_lock_file(void)
{
    char file[PATH_MAX];
    struct flock fl;
    int lockfd;

    snprintf(file, sizeof(file), "%s/efreet/icon_data.lock", efreet_cache_home_get());
    lockfd = open(file, O_CREAT | O_BINARY | O_RDWR, S_IRUSR | S_IWUSR);
    if (lockfd < 0) return -1;
    efreet_fsetowner(lockfd);

    memset(&fl, 0, sizeof(struct flock));
    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    if (fcntl(lockfd, F_SETLK, &fl) < 0)
    {
        WRN("LOCKED! You may want to delete %s if this persists", file);
        close(lockfd);
        return -1;
    }

    return lockfd;
}

/**
 * @internal
 * @brief Frees resources associated with an Efreet_Cache_Icon_Theme structure.
 *
 * This function is typically used as an EINA_FREE_CB for the `icon_themes` hash.
 * It frees lists (`paths`, `inherits`, `directories`) and the `dirs` hash within the theme.
 * Note: Stringshared strings within these structures are not freed here; they are
 * managed by the global `strs` array and `eina_stringshare_del`.
 *
 * @param theme The Efreet_Cache_Icon_Theme to free.
 */
static void
icon_theme_free(Efreet_Cache_Icon_Theme *theme)
{
    void *data;

    eina_list_free(theme->theme.paths);
    eina_list_free(theme->theme.inherits);
    EINA_LIST_FREE(theme->theme.directories, data) free(data);
    if (theme->dirs) efreet_hash_free(theme->dirs, free);
    free(theme);
}

/**
 * @internal
 * @brief Main entry point for the Efreet icon cache generator.
 *
 * Orchestrates the entire cache creation process:
 * 1. Initializes Eina, Eet, Ecore, and Efreet.
 * 2. Parses command-line arguments (verbose, extensions, extra dirs, flush).
 * 3. Sets process priority to low.
 * 4. Creates cache directory and lock file.
 * 5. Loads existing theme cache (`efreet_icon_theme_cache_file()`).
 * 6. Scans system and user directories for icon themes (`cache_theme_scan`).
 * 7. For each valid theme:
 *    a. Verifies if theme directories changed (`cache_theme_change_verify`, `check_changed`).
 *    b. Opens/creates per-theme icon cache file (`efreet_icon_cache_file()`).
 *    c. If changed, rescans icons for the theme and its inherits (`cache_scan`).
 *    d. Writes updated icon data and theme metadata to caches.
 * 8. Processes fallback icons:
 *    a. Checks if fallback icon directories changed (`check_fallback_changed`).
 *    b. If changed, rescans fallback icons (`cache_fallback_scan`).
 *    c. Writes updated fallback icon data to cache.
 * 9. Writes overall theme cache version.
 * 10. Prints 'c' if changes were made, 'n' otherwise.
 * 11. Cleans up resources and shuts down libraries.
 *
 * @param argc Argument count.
 * @param argv Argument vector. Supported arguments:
 *             - "-v": Verbose mode.
 *             - "-e .ext1 .ext2 ...": Specifies recognized icon extensions. (Required)
 *             - "-d dir1 dir2 ...": Specifies additional directories to scan for icons.
 *             - "-f": Force flush; treat all themes as changed.
 * @return 0 on success, -1 on critical failure.
 */
int
main(int argc, char **argv)
{
    /* TODO:
     * - Add file monitor on files, so that we catch changes on files
     *   during whilst this program runs.
     * - Maybe linger for a while to reduce number of cache re-creates.
     */
    Eina_Iterator *it;
    Efreet_Cache_Version *icon_version;
    Efreet_Cache_Version *theme_version;
    Efreet_Cache_Icon_Theme *theme;
    Eet_Data_Descriptor *theme_edd;
    Eet_Data_Descriptor *icon_edd;
    Eet_Data_Descriptor *fallback_edd;
    Eet_File *icon_ef;
    Eet_File *theme_ef;
    Eina_List *xdg_dirs = NULL;
    Eina_List *l = NULL;
    char file[PATH_MAX];
    const char *path;
    char *dir = NULL;
    Eina_Bool changed = EINA_FALSE;
    Eina_Bool flush = EINA_FALSE;
    int lockfd = -1;
    char **keys;
    int num, i;

    /* init external subsystems */
    if (!eina_init()) return -1;
    _efreet_icon_cache_log_dom =
        eina_log_domain_register("efreet_icon_cache", EFREET_DEFAULT_LOG_COLOR);
    if (_efreet_icon_cache_log_dom < 0)
    {
        EINA_LOG_ERR("Efreet: Could not create a log domain for efreet_icon_cache.");
        return -1;
    }

    eina_log_domain_level_set("efreet_icon_cache", EINA_LOG_LEVEL_ERR);

    exts = eina_array_new(10);
    extra_dirs = eina_array_new(10);

    for (i = 1; i < argc; i++)
    {
        if (!strcmp(argv[i], "-v"))
            eina_log_domain_level_set("efreet_icon_cache", EINA_LOG_LEVEL_DBG);
        else if ((!strcmp(argv[i], "-h")) ||
                 (!strcmp(argv[i], "-help")) ||
                 (!strcmp(argv[i], "--h")) ||
                 (!strcmp(argv[i], "--help")))
        {
            printf("Options:\n");
            printf("  -v              Verbose mode\n");
            printf("  -e .ext1 .ext2  Extensions\n");
            printf("  -d dir1 dir2    Extra dirs\n");
            printf("  -f              Flush\n");
            exit(0);
        }
        else if (!strcmp(argv[i], "-e"))
        {
            while ((i < (argc - 1)) && (argv[(i + 1)][0] != '-'))
                eina_array_push(exts, argv[++i]);
        }
        else if (!strcmp(argv[i], "-d"))
        {
            while ((i < (argc - 1)) && (argv[(i + 1)][0] != '-'))
                eina_array_push(extra_dirs, argv[++i]);
        }
        else if (!strcmp(argv[i], "-f"))
            flush = EINA_TRUE;
    }

#ifdef HAVE_SYS_RESOURCE_H
    setpriority(PRIO_PROCESS, 0, 19);
#elif _WIN32
    SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS);
#endif

    if (!eet_init()) return -1;
    if (!ecore_init()) return -1;

    efreet_cache_update = 0;
    /* finish efreet init */
    if (!efreet_init()) goto on_error;

    strs = eina_array_new(32);

    /* create homedir */
    snprintf(file, sizeof(file), "%s/efreet", efreet_cache_home_get());
    if (!ecore_file_exists(file))
    {
        if (!ecore_file_mkpath(file)) return -1;
        efreet_setowner(file);
    }

    /* lock process, so that we only run one copy of this program */
    lockfd = cache_lock_file();
    if (lockfd == -1) goto on_error;

    /* Need to init edd's, so they are like we want, not like userspace wants */
    icon_edd = efreet_icon_edd();
    fallback_edd = efreet_icon_fallback_edd();
    theme_edd = efreet_icon_theme_edd(EINA_TRUE);

    icon_themes = eina_hash_string_superfast_new(EINA_FREE_CB(icon_theme_free));

    INF("opening theme cache");
    /* open theme file */
    theme_ef = eet_open(efreet_icon_theme_cache_file(), EET_FILE_MODE_READ_WRITE);
    if (!theme_ef) goto on_error_efreet;
    theme_version = eet_data_read(theme_ef, efreet_version_edd(), EFREET_CACHE_VERSION);
    if (theme_version &&
        ((theme_version->major != EFREET_ICON_CACHE_MAJOR) ||
         (theme_version->minor != EFREET_ICON_CACHE_MINOR)))
    {
        // delete old cache
        eet_close(theme_ef);
        if (unlink(efreet_icon_theme_cache_file()) < 0)
        {
            if (errno != ENOENT) goto on_error_efreet;
        }
        theme_ef = eet_open(efreet_icon_theme_cache_file(), EET_FILE_MODE_READ_WRITE);
        if (!theme_ef) goto on_error_efreet;
    }
    if (!theme_version)
        theme_version = NEW(Efreet_Cache_Version, 1);

    theme_version->major = EFREET_ICON_CACHE_MAJOR;
    theme_version->minor = EFREET_ICON_CACHE_MINOR;

    if (flush)
        changed = EINA_TRUE;

    if (exts->count == 0)
    {
        ERR("Need to pass extensions to icon cache create process");
        goto on_error_efreet;
    }

    keys = eet_list(theme_ef, "*", &num);
    if (keys)
    {
        for (i = 0; i < num; i++)
        {
            if (!strncmp(keys[i], "__efreet", 8)) continue;
            theme = eet_data_read(theme_ef, theme_edd, keys[i]);
            if (theme)
            {
                theme->valid = 0;
                eina_hash_direct_add(icon_themes, theme->theme.name.internal, theme);
            }
        }
        free(keys);
    }

    INF("scan for themes");
    /* scan themes */
    cache_theme_scan(efreet_icon_deprecated_user_dir_get());
    cache_theme_scan(efreet_icon_user_dir_get());

    xdg_dirs = efreet_data_dirs_get();
    EINA_LIST_FOREACH(xdg_dirs, l, dir)
    {
        snprintf(file, sizeof(file), "%s/icons", dir);
        cache_theme_scan(file);
    }

#ifndef STRICT_SPEC
    EINA_LIST_FOREACH(xdg_dirs, l, dir)
    {
        snprintf(file, sizeof(file), "%s/pixmaps", dir);
        cache_theme_scan(file);
    }
#endif

    cache_theme_scan("/usr/local/share/pixmaps");
    cache_theme_scan("/usr/share/pixmaps");

    /* scan icons */
    it = eina_hash_iterator_data_new(icon_themes);
    EINA_ITERATOR_FOREACH(it, theme)
    {
        if (!theme->valid) continue;
#ifndef STRICT_SPEC
        if (!theme->theme.name.name) continue;
#endif
        INF("scan theme %s", theme->theme.name.name);
        cache_theme_change_verify(theme);
        theme->changed = check_changed(theme);
        if (flush)
            theme->changed = EINA_TRUE;

        INF("open icon file");
        /* open icon file */
        icon_ef = eet_open(efreet_icon_cache_file(theme->theme.name.internal), EET_FILE_MODE_READ_WRITE);
        if (!icon_ef) goto on_error_efreet;
        icon_version = eet_data_read(icon_ef, efreet_version_edd(), EFREET_CACHE_VERSION);
        if ((theme->changed) || (!icon_version) ||
            ((icon_version->major != EFREET_ICON_CACHE_MAJOR) ||
             (icon_version->minor != EFREET_ICON_CACHE_MINOR)))
        {
            // delete old cache
            eet_close(icon_ef);
            if (unlink(efreet_icon_cache_file(theme->theme.name.internal)) < 0)
            {
                if (errno != ENOENT) goto on_error_efreet;
            }
            icon_ef = eet_open(efreet_icon_cache_file(theme->theme.name.internal), EET_FILE_MODE_READ_WRITE);
            if (!icon_ef) goto on_error_efreet;
            theme->changed = EINA_TRUE;
        }

        if (theme->changed)
            changed = EINA_TRUE;

        if (!icon_version)
            icon_version = NEW(Efreet_Cache_Version, 1);

        icon_version->major = EFREET_ICON_CACHE_MAJOR;
        icon_version->minor = EFREET_ICON_CACHE_MINOR;

        if (theme->changed)
        {
            Eina_Hash *themes;
            Eina_Hash *icons;

            themes = eina_hash_string_superfast_new(NULL);
            icons = eina_hash_string_superfast_new(NULL);

            INF("scan icons\n");
            if (cache_scan(&(theme->theme), themes, icons))
            {
                Eina_Iterator *icons_it;
                Eina_Hash_Tuple *tuple;

                INF("generated: '%s' %i (%i)",
                    theme->theme.name.internal,
                    theme->changed,
                    eina_hash_population(icons));

                icons_it = eina_hash_iterator_tuple_new(icons);
                EINA_ITERATOR_FOREACH(icons_it, tuple)
                    eet_data_write(icon_ef, icon_edd, tuple->key, tuple->data, EET_COMPRESSION_SUPERFAST);
                eina_iterator_free(icons_it);

                INF("theme change: %s %lld", theme->theme.name.internal, theme->check.mtime);
                eet_data_write(theme_ef, theme_edd, theme->theme.name.internal, theme, EET_COMPRESSION_SUPERFAST);
            }
            eina_hash_free(themes);
            eina_hash_free(icons);
            changed = EINA_TRUE;
        }

        eet_data_write(icon_ef, efreet_version_edd(), EFREET_CACHE_VERSION, icon_version, EET_COMPRESSION_SUPERFAST);
        eet_close(icon_ef);
        efreet_setowner(efreet_icon_cache_file(theme->theme.name.internal));
        free(icon_version);
    }
    eina_iterator_free(it);

    INF("scan fallback icons");
    theme = eet_data_read(theme_ef, theme_edd, EFREET_CACHE_ICON_FALLBACK);
    if (!theme)
    {
        theme = NEW(Efreet_Cache_Icon_Theme, 1);
        theme->changed = EINA_TRUE;
    }
    if (flush)
        theme->changed = EINA_TRUE;

    INF("open fallback file");
    /* open icon file */
    icon_ef = eet_open(efreet_icon_cache_file(EFREET_CACHE_ICON_FALLBACK), EET_FILE_MODE_READ_WRITE);
    if (!icon_ef) goto on_error_efreet;
    icon_version = eet_data_read(icon_ef, efreet_version_edd(), EFREET_CACHE_VERSION);
    if (theme->changed || (icon_version &&
        ((icon_version->major != EFREET_ICON_CACHE_MAJOR) ||
         (icon_version->minor != EFREET_ICON_CACHE_MINOR))))
    {
        // delete old cache
        eet_close(icon_ef);
        if (unlink(efreet_icon_cache_file(EFREET_CACHE_ICON_FALLBACK)) < 0)
        {
            if (errno != ENOENT) goto on_error_efreet;
        }
        icon_ef = eet_open(efreet_icon_cache_file(EFREET_CACHE_ICON_FALLBACK), EET_FILE_MODE_READ_WRITE);
        if (!icon_ef) goto on_error_efreet;
        theme->changed = EINA_TRUE;
    }
    if (!theme->changed)
        theme->changed = check_fallback_changed(theme);
    if (theme->changed && theme->dirs)
    {
        efreet_hash_free(theme->dirs, free);
        theme->dirs = NULL;
    }
    if (!theme->dirs)
        theme->dirs = eina_hash_string_superfast_new(NULL);

    if (theme->changed)
        changed = EINA_TRUE;

    if (!icon_version)
        icon_version = NEW(Efreet_Cache_Version, 1);

    icon_version->major = EFREET_ICON_CACHE_MAJOR;
    icon_version->minor = EFREET_ICON_CACHE_MINOR;

    if (theme->changed)
    {
        Eina_Hash *icons;

        icons = eina_hash_string_superfast_new(NULL);

        INF("scan fallback icons");
        /* Save fallback in the right part */
        if (cache_fallback_scan(icons, theme->dirs))
        {
            Eina_Iterator *icons_it;
            Eina_Hash_Tuple *tuple;

            INF("generated: fallback %i (%i)", theme->changed, eina_hash_population(icons));

            icons_it = eina_hash_iterator_tuple_new(icons);
            EINA_ITERATOR_FOREACH(icons_it, tuple)
                eet_data_write(icon_ef, fallback_edd, tuple->key, tuple->data, EET_COMPRESSION_SUPERFAST);
            eina_iterator_free(icons_it);
        }
        eina_hash_free(icons);

        eet_data_write(theme_ef, theme_edd, EFREET_CACHE_ICON_FALLBACK, theme, EET_COMPRESSION_SUPERFAST);
    }

    icon_theme_free(theme);

    eet_data_write(icon_ef, efreet_version_edd(), EFREET_CACHE_VERSION, icon_version, EET_COMPRESSION_SUPERFAST);
    eet_close(icon_ef);
    efreet_setowner(efreet_icon_cache_file(EFREET_CACHE_ICON_FALLBACK));
    free(icon_version);

    eina_hash_free(icon_themes);

    /* save data */
    eet_data_write(theme_ef, efreet_version_edd(), EFREET_CACHE_VERSION, theme_version, EET_COMPRESSION_SUPERFAST);

    eet_close(theme_ef);
    theme_ef = NULL;
    efreet_setowner(efreet_icon_theme_cache_file());
    free(theme_version);

    {
        char c = 'n';

        if (changed) c = 'c';
        printf("%c\n", c);
    }

    INF("done");
on_error_efreet:
    efreet_shutdown();
    if (theme_ef) eet_close(theme_ef);

on_error:
    if (lockfd >= 0) close(lockfd);

    while ((path = eina_array_pop(strs)))
        eina_stringshare_del(path);
    eina_array_free(strs);
    eina_array_free(exts);
    eina_array_free(extra_dirs);

    ecore_shutdown();
    eet_shutdown();
    eina_log_domain_unregister(_efreet_icon_cache_log_dom);
    eina_shutdown();

    return 0;
}
