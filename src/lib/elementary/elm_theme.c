#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include <Elementary.h>

#include "elm_priv.h"
#include "elm_icon_eo.h"

#include "efl_ui_theme.eo.h"

static Elm_Theme *theme_default = NULL;

static Eina_List *themes = NULL;

/**
 * @internal
 * @brief Tries to find an Edje group in a theme file and cache it.
 *
 * This function checks if a given Edje group exists in the provided theme
 * file handle. If it exists, it may perform additional checks for overlays
 * and extensions to ensure they match the base theme. If successful, the
 * file handle is cached for the given group name to speed up future lookups.
 *
 * @param th The theme context.
 * @param etf The theme file to search in.
 * @param group The name of the Edje group to find.
 * @param force If @c EINA_TRUE, bypasses the theme matching check for
 *              overlays and extensions. This is used as a fallback mechanism.
 * @return The file handle (@c Eina_File *) if the group is found and conditions
 *         are met, otherwise @c NULL. The returned handle is not a new reference.
 */
static Eina_File *
_elm_theme_find_try(Elm_Theme *th, Elm_Theme_File *etf, const char *group, Eina_Bool force)
{
   if (edje_mmap_group_exists(etf->handle, group))
     {
        if (etf->match_theme && (!force)) // overlay or extension
          {
             Elm_Theme_File *base_etf;
             Eina_Bool found = EINA_FALSE;

             EINA_INLIST_FOREACH(th->themes, base_etf)
               {
                  if (base_etf->base_theme != etf->match_theme) continue;
                  found = EINA_TRUE;
                  break;
               }
             if (!found) return NULL;
          }
        eina_hash_add(th->cache, group, eina_file_dup(etf->handle));
        return etf->handle;
     }
   return NULL;
}

/**
 * @internal
 * @brief Finalizes the addition of a theme file to a theme list.
 *
 * This function performs the final steps of adding a theme file to a list of
 * theme files. It checks the theme's version for compatibility, creates an
 * Elm_Theme_File structure, populates it with theme information, and adds it
 * to the given list of files (either at the beginning or end).
 *
 * @param files A pointer to the list of theme files (Eina_Inlist).
 * @param item The name of the theme item.
 * @param f The file handle (@c Eina_File *) for the theme file. This function
 *          takes ownership of the handle.
 * @param prepend If @c EINA_TRUE, the item is added to the beginning of the list.
 *                Otherwise, it's appended to the end.
 * @param istheme If @c EINA_TRUE, the file is treated as a base theme file.
 *                Otherwise, it's treated as an extension or overlay.
 */
static inline void
_elm_theme_item_finalize(Eina_Inlist **files,
                         const char *item,
                         Eina_File *f,
                         Eina_Bool prepend,
                         Eina_Bool istheme)
{
   Elm_Theme_File *etf;
   char *name;
   /* Theme version history:
    * <110: legacy, had no version tag
    *  110: first supported version
    *  119: switched windows to always use border
    *       win group has no menu, no blocker
    *       border group has all required swallows (conformant, bg, win)
    *       data: "elm_bg_version" matches "version" ("119")
    */
   if (!f) return;
   if (istheme)
     {
        char *version;
        int v;

        if (!(version = edje_mmap_data_get(f, "version")))
          {
             eina_file_close(f); // close matching open (finalize expected to consume eina file) OK
             return;
          }
        v = atoi(version);
        if (v < 110) // bump this version number when we need to
          {
             WRN("Selected theme is too old (version = %d, needs >= 110)", v);
          }
        free(version);
     }
   etf = calloc(1, sizeof(Elm_Theme_File));
   EINA_SAFETY_ON_NULL_RETURN(etf);
   etf->item = eina_stringshare_add(item);
   etf->handle = f; // now own/consume the file handle
   if (istheme)
     {
        name = edje_mmap_data_get(f, "efl_theme_base");
        etf->base_theme = eina_stringshare_add(name);
     }
   else
     {
        name = edje_mmap_data_get(f, "efl_theme_match");
        etf->match_theme = eina_stringshare_add(name);
     }
   free(name);
   if (prepend)
     {
        *files = eina_inlist_prepend(*files, EINA_INLIST_GET(etf));
     }
   else
     {
        *files = eina_inlist_append(*files, EINA_INLIST_GET(etf));
     }
}

/**
 * @internal
 * @brief Adds a theme file item to a list of theme files.
 *
 * This function locates a theme file by its name, opens it, and adds it to the
 * specified list of theme files. It can handle absolute paths, paths relative
 * to the home directory, and theme names that need to be looked up in the
 * standard theme search paths.
 *
 * @param files A pointer to the list of theme files.
 * @param item The theme item name or path. This could be a simple name like
 *             "default", or a path like "/path/to/my/theme.edj".
 * @param prepend If @c EINA_TRUE, add to the beginning of the list (for overlays).
 * @param istheme If @c EINA_TRUE, this is a base theme, not an extension/overlay.
 */
static void
_elm_theme_file_item_add(Eina_Inlist **files, const char *item, Eina_Bool prepend, Eina_Bool istheme)
{
   Eina_Strbuf *buf = NULL;
   Eina_File *f = NULL;
   const char *home;

   home = eina_environment_home_get();
   buf = eina_strbuf_new();

   if (!eina_file_path_relative(item) ||
       ((item[0] == '.') && (item[1] == '/')) ||
       ((item[0] == '.') && (item[1] == '.') && (item[2] == '/')))
     {
        f = eina_file_open(item, EINA_FALSE);
        if (!f) goto on_error;
     }
   else if (((item[0] == '~') && (item[1] == '/')))
     {
        eina_strbuf_append_printf(buf, "%s/%s", home, item + 2);

        f = eina_file_open(eina_strbuf_string_get(buf), EINA_FALSE);
        if (!f) goto on_error;
     }
   else
     {
        eina_strbuf_append_printf(buf,
                                  "%s/"ELEMENTARY_BASE_DIR"/themes/%s.edj",
                                  home, item);
        f = eina_file_open(eina_strbuf_string_get(buf), EINA_FALSE);
        _elm_theme_item_finalize(files, item, f, prepend, istheme);

        eina_strbuf_reset(buf);

        eina_strbuf_append_printf(buf,
                                  "%s/themes/%s.edj",
                                  _elm_data_dir, item);
        f = eina_file_open(eina_strbuf_string_get(buf), EINA_FALSE);
        /* Finalize will be done by the common one */
     }

   _elm_theme_item_finalize(files, item, f, prepend, istheme);

 on_error:
   if (buf) eina_strbuf_free(buf);
}

/**
 * @internal
 * @brief Deletes a theme file item from a list of theme files.
 *
 * This function searches for a theme file by its name within the given list and
 * removes it. It also closes the associated file handle and frees allocated memory.
 *
 * @param files A pointer to the list of theme files.
 * @param str The name of the theme item to delete.
 */
static void
_elm_theme_file_item_del(Eina_Inlist **files, const char *str)
{
   Eina_Inlist *l;
   Elm_Theme_File *item;

   str = eina_stringshare_add(str);

   EINA_INLIST_FOREACH_SAFE(*files, l, item)
     {
        if (item->item != str) continue;
        eina_file_close(item->handle); // close matching open (file consumed in finalize by putting it in etf) OK
        eina_stringshare_del(item->item);
        *files = eina_inlist_remove(*files, EINA_INLIST_GET(item));
        free(item);
     }

   eina_stringshare_del(str);
}

/**
 * @internal
 * @brief Deletes a theme file item from a list using its file handle.
 *
 * This function is similar to _elm_theme_file_item_del(), but it identifies
 * the theme file to be removed by its @c Eina_File handle instead of its name.
 *
 * @param files A pointer to the list of theme files.
 * @param file The file handle of the theme to delete.
 */
static void
_elm_theme_file_mmap_del(Eina_Inlist **files, const Eina_File *file)
{
   Eina_Inlist *l;
   Elm_Theme_File *item;

   EINA_INLIST_FOREACH_SAFE(*files, l, item)
     {
        if (item->handle != file) continue;
        eina_file_close(item->handle); // close matching open (file consumed in finalize by putting it in etf) OK
        eina_stringshare_del(item->item);
        *files = eina_inlist_remove(*files, EINA_INLIST_GET(item));
        free(item);
     }
}

/**
 * @internal
 * @brief Cleans up a list of theme files.
 *
 * This function iterates through a list of theme files, closing each file
 * handle, freeing associated stringshares, and deallocating the list items.
 * The list will be empty after this call.
 *
 * @param files A pointer to the list of theme files to clean.
 */
static void
_elm_theme_file_clean(Eina_Inlist **files)
{
   while (*files)
     {
        Elm_Theme_File *etf = EINA_INLIST_CONTAINER_GET(*files, Elm_Theme_File);

        eina_stringshare_del(etf->item);
        eina_file_close(etf->handle); // close matching open (file consumed in finalize by putting it in etf) OK
        eina_stringshare_del(etf->match_theme);
        *files = eina_inlist_remove(*files, *files);
        free(etf);
     }
}

/**
 * @internal
 * @brief Clears all data associated with a theme object.
 *
 * This function resets a theme object to a clean state. It clears the lists of
 * themes, overlays, and extensions. It also frees all associated caches
 * (for groups, data, and failed lookups) and releases any referenced theme.
 *
 * @param th The theme object to clear.
 */
static void
_elm_theme_clear(Elm_Theme *th)
{
   Efl_Ui_Theme_Data *td;

   _elm_theme_file_clean(&th->themes);
   _elm_theme_file_clean(&th->overlay);
   _elm_theme_file_clean(&th->extension);

   ELM_SAFE_FREE(th->overlay_items, eina_list_free);
   ELM_SAFE_FREE(th->theme_items, eina_list_free);
   ELM_SAFE_FREE(th->extension_items, eina_list_free);

   ELM_SAFE_FREE(th->cache, eina_hash_free);
   ELM_SAFE_FREE(th->cache_data, eina_hash_free);
   ELM_SAFE_FREE(th->cache_style_load_failed, eina_hash_free);
   ELM_SAFE_FREE(th->theme, eina_stringshare_del);
   if (th->ref_theme)
     {
        th->ref_theme->referrers =
           eina_list_remove(th->ref_theme->referrers, th);
        elm_theme_free(th->ref_theme);
        th->ref_theme = NULL;
     }

   td = efl_data_scope_get(th->eo_theme, EFL_UI_THEME_CLASS);
   td->th = NULL;
   th->eo_theme = NULL;
}

/**
 * @internal
 * @brief Internal implementation for finding the file containing an Edje group.
 *
 * This function searches for an Edje group within the theme hierarchy, which
 * includes overlays, base themes, and extensions. It also recursively searches
 * in the referenced theme if one is set. It uses a cache to speed up lookups.
 *
 * @param th The theme to search in.
 * @param group The name of the Edje group to find.
 * @param force If @c EINA_TRUE, bypasses theme matching logic, used as a fallback.
 * @return The file handle (@c Eina_File *) containing the group, or @c NULL if not found.
 */
static Eina_File *
_elm_theme_group_file_find_internal(Elm_Theme *th, const char *group, Eina_Bool force)
{
   Elm_Theme_File *etf;
   Eina_File *file = eina_hash_find(th->cache, group);

   if (file) return file;

   EINA_INLIST_FOREACH(th->overlay, etf)
     {
        file = _elm_theme_find_try(th, etf, group, force);
        if (file) return file;
     }
   EINA_INLIST_FOREACH(th->themes, etf)
     {
        file = _elm_theme_find_try(th, etf, group, force);
        if (file) return file;
     }
   EINA_INLIST_FOREACH(th->extension, etf)
     {
        file = _elm_theme_find_try(th, etf, group, force);
        if (file) return file;
     }
   if (th->ref_theme) return _elm_theme_group_file_find_internal(th->ref_theme, group, force);
   return NULL;
}

/**
 * @internal
 * @brief Finds the theme file that contains a specific Edje group.
 *
 * This is a public-within-Elementary function to find which theme file (.edj)
 * provides a given Edje group. It first attempts a standard search, and if
 * that fails, it performs a "forced" search that may bypass some theme
 * matching rules as a fallback.
 *
 * @param th The theme context to search within. If NULL, the default theme is used.
 * @param group The name of the Edje group to locate.
 * @return An @c Eina_File handle to the theme file containing the group, or
 *         @c NULL if the group cannot be found.
 */
Eina_File *
_elm_theme_group_file_find(Elm_Theme *th, const char *group)
{
   Eina_File *file = _elm_theme_group_file_find_internal(th, group, EINA_FALSE);
   if (file) return file;
   file = _elm_theme_group_file_find_internal(th, group, EINA_TRUE);
   return file;
}

/**
 * @internal
 * @brief Tries to read and cache a data item from a single theme file.
 *
 * This function attempts to retrieve a data value associated with a key from
 * a given Edje file. If the data is found, it is added to the theme's data
 * cache for faster subsequent access.
 *
 * @param th The theme context for caching.
 * @param f The Edje file handle to read from.
 * @param key The key of the data item to retrieve.
 * @return A stringshared pointer to the data value if found, otherwise @c NULL.
 */
static const char *
_elm_theme_find_data_try(Elm_Theme *th, const Eina_File *f, const char *key)
{
   char *data;
   const char *t;

   data = edje_mmap_data_get(f, key);
   t = eina_stringshare_add(data);
   free(data);
   if (t)
     {
        eina_hash_add(th->cache_data, key, t);
        return t;
     }
   return NULL;
}

/**
 * @internal
 * @brief Finds a data item by key within the entire theme hierarchy.
 *
 * This function searches for a data item across all parts of a theme, including
 * overlays, base themes, and extensions. It checks the cache first, and if not
 * found, it iterates through the theme files. It also searches in the
 * referenced theme if one exists.
 *
 * @param th The theme context to search within.
 * @param key The key of the data item to find.
 * @return A stringshared pointer to the data value if found, otherwise @c NULL.
 */
static const char *
_elm_theme_data_find(Elm_Theme *th, const char *key)
{
   Elm_Theme_File *etf;

   const char *data = eina_hash_find(th->cache_data, key);
   if (data) return data;

   EINA_INLIST_FOREACH(th->overlay, etf)
     {
        data = _elm_theme_find_data_try(th, etf->handle, key);
        if (data) return data;
     }
   EINA_INLIST_FOREACH(th->themes, etf)
     {
        data = _elm_theme_find_data_try(th, etf->handle, key);
        if (data) return data;
     }
   EINA_INLIST_FOREACH(th->extension, etf)
     {
        data = _elm_theme_find_data_try(th, etf->handle, key);
        if (data) return data;
     }
   if (th->ref_theme) return _elm_theme_data_find(th->ref_theme, key);
   return NULL;
}

/**
 * @internal
 * @brief Sets the theme for a widget object, deriving theme from a parent.
 *
 * This function is an internal helper used to apply a theme style to a widget.
 * It determines the correct theme to use by looking at the widget's parent.
 * This is the common path for setting a widget's theme when it's part of a
 * widget hierarchy.
 *
 * @param parent The parent widget, used to inherit the theme context.
 * @param o The Evas_Object (widget) to apply the theme to.
 * @param clas The class of the widget (e.g., "button").
 * @param group The group within the theme file (e.g., "base").
 * @param style The style to apply (e.g., "default", "custom").
 * @return An @c Efl_Ui_Theme_Apply_Error code indicating success or failure.
 */
Eina_Error
_elm_theme_object_set(Evas_Object *parent, Evas_Object *o, const char *clas, const char *group, const char *style)
{
   Elm_Theme *th = NULL;

   if (parent) th = elm_widget_theme_get(parent);

   return _elm_theme_set(th, o, clas, group, style, elm_widget_is_legacy(parent));
}

/**
 * @internal
 * @brief Sets the theme for an icon object.
 *
 * This is a specialized theme-setting function used exclusively by the icon
 * widget. It handles the logic for applying a theme to an icon, including
 * checking for standard system icons if the configuration dictates.
 *
 * @param o The icon object.
 * @param group The icon group (e.g., "home", "close").
 * @param style The style of the icon (e.g., "default").
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
/* only issued by elm_icon.c */
Eina_Bool
_elm_theme_object_icon_set(Evas_Object *o,
                           const char *group,
                           const char *style)
{
   Elm_Theme *th = elm_widget_theme_get(o);

   return _elm_theme_icon_set(th, o, group, style);
}

/**
 * @internal
 * @brief Core function for applying a theme style to an Evas object.
 *
 * This function is the heart of theme application. It constructs the full
 * Edje group name based on class, group, and style, then finds the appropriate
 * theme file and applies the group to the object. It supports both modern
 * ("efl/...") and legacy ("elm/...") naming schemes. If a specific style is
 * not found, it attempts to fall back to the "default" style for that group.
 * It also caches failed lookups to avoid repeated file system searches.
 *
 * @param th The theme context to use.
 * @param o The Evas_Object to apply the theme to.
 * @param clas The widget's class.
 * @param group The widget's part/group name.
 * @param style The desired style.
 * @param is_legacy If @c EINA_TRUE, use the old "elm/" naming scheme.
 * @return An @c Efl_Ui_Theme_Apply_Error code.
 *         - @c EFL_UI_THEME_APPLY_ERROR_NONE: Success.
 *         - @c EFL_UI_THEME_APPLY_ERROR_DEFAULT: The requested style failed, but the default fallback was successful.
 *         - @c EFL_UI_THEME_APPLY_ERROR_GENERIC: A general failure occurred.
 */
Eina_Error
_elm_theme_set(Elm_Theme *th, Evas_Object *o, const char *clas, const char *group, const char *style, Eina_Bool is_legacy)
{
   Eina_File *file;
   char buf2[1024];
   const char *group_sep = "/";
   const char *style_sep = ":";

   if ((!clas) || !o) return EFL_UI_THEME_APPLY_ERROR_GENERIC;
   if (!th) th = theme_default;
   if (!th) return EFL_UI_THEME_APPLY_ERROR_GENERIC;

   if (eina_streq(style, "default")) style = NULL;

   if (is_legacy)
     snprintf(buf2, sizeof(buf2), "elm/%s/%s/%s", clas, (group) ? group : "base", (style) ? style : "default");
   else
     snprintf(buf2, sizeof(buf2), "efl/%s%s%s%s%s", clas,
            ((group) ? group_sep : "\0"), ((group) ? group : "\0"),
            ((style) ? style_sep : "\0"), ((style) ? style : "\0"));
   if (!eina_hash_find(th->cache_style_load_failed, buf2))
     {
        file = _elm_theme_group_file_find(th, buf2);
        if (file)
          {
             if (edje_object_mmap_set(o, file, buf2)) return EFL_UI_THEME_APPLY_ERROR_NONE;
             else
               {
                  ERR("could not set theme group '%s' from file '%s': %s",
                      buf2,
                      eina_file_filename_get(file),
                      edje_load_error_str(edje_object_load_error_get(o)));
               }
          }
        //style not found, add to the not found list
        eina_hash_add(th->cache_style_load_failed, buf2, (void *)1);
     }

   if (!style)
     return EFL_UI_THEME_APPLY_ERROR_GENERIC;

   // Use the elementary default style.
   if (_elm_theme_set(th, o, clas, group, NULL, is_legacy) == EFL_UI_THEME_APPLY_ERROR_NONE)
     return EFL_UI_THEME_APPLY_ERROR_DEFAULT;
   return EFL_UI_THEME_APPLY_ERROR_GENERIC;
}

/**
 * @internal
 * @brief Core function to apply a theme to an icon object.
 *
 * This function handles the logic for setting an icon. It first checks if a
 * standard Freedesktop icon theme should be used. If not, or if the icon is
 * not a standard one, it proceeds to look for the icon in the Elementary
 * theme files. It constructs the group name (e.g., "elm/icon/home/default")
 * and applies it. It includes a fallback to the "default" style if the
 * requested style is not found.
 *
 * @param th The theme context.
 * @param o The Evas_Object (icon) to set.
 * @param group The icon's name/group (e.g., "home").
 * @param style The icon's style.
 * @return @c EINA_TRUE if the icon was successfully set, @c EINA_FALSE otherwise.
 */
Eina_Bool
_elm_theme_icon_set(Elm_Theme *th,
                    Evas_Object *o,
                    const char *group,
                    const char *style)
{
   Eina_File *file;
   char buf2[1024];
   int w, h;

   if (efl_isa((o), ELM_ICON_CLASS) && elm_icon_standard_get(o) &&
       strcmp(elm_config_icon_theme_get(), ELM_CONFIG_ICON_THEME_ELEMENTARY))
     {
        elm_icon_standard_set(o, elm_icon_standard_get(o));
        return EINA_TRUE;
     }

   if (!th) th = theme_default;
   if (!th) return EINA_FALSE;

   snprintf(buf2, sizeof(buf2), "elm/icon/%s/%s", group, style);
   file = _elm_theme_group_file_find(th, buf2);
   if (file)
     {
        elm_image_mmap_set(o, file, buf2);
        elm_image_object_size_get(o, &w, &h);
        if (w > 0) return EINA_TRUE;
     }
   snprintf(buf2, sizeof(buf2), "elm/icon/%s/default", group);
   file = _elm_theme_group_file_find(th, buf2);

   if (!file) return EINA_FALSE;

   elm_image_mmap_set(o, file, buf2);
   elm_image_object_size_get(o, &w, &h);

   return w > 0;
}

/**
 * @internal
 * @brief Parses a theme search string and configures the theme.
 *
 * This function takes a colon-separated string of theme names, parses it,
 * and sets up the theme search order. It clears any existing theme
 * configuration and caches before applying the new one. It ensures that
 * "default" is always present in the theme list as a final fallback.
 *
 * @param th The theme to configure.
 * @param theme The theme search string, e.g., "mytheme:shiny:/path/to/other.edj".
 *              Colons can be escaped with a backslash.
 */
void
_elm_theme_parse(Elm_Theme *th, const char *theme)
{
   Eina_List *names = NULL;
   const char *p, *pe;

   if (!th) th = theme_default;
   if (!th) return;

   if (theme)
     {
        Eina_Strbuf *buf;

        buf = eina_strbuf_new();

        p = theme;
        pe = p;
        for (;;)
          {
             if ((pe[0] == '\\') && (pe[1] == ':'))
               {
                  eina_strbuf_append_char(buf, ':');
                  pe += 2;
               }
#ifdef HAVE_ELEMENTARY_WIN32
             else if (isalpha(pe[0]) && (pe[1] == ':') &&
                      ((pe[2] == '/') || (pe[2] == '\\')))
               {
                  // Correct processing file path on  Windows OS "<disk>:/" or "<disk>:\"
                  eina_strbuf_append_char(buf, *pe);
                  pe++;
                  eina_strbuf_append_char(buf, *pe);
                  pe++;
               }
#endif
             else if ((*pe == ':') || (!*pe))
               { // p -> pe == 'name:'
                  if (pe > p)
                    {
                       const char *nn;

                       nn = eina_stringshare_add(eina_strbuf_string_get(buf));
                       if (nn) names = eina_list_append(names, nn);
                       eina_strbuf_reset(buf);
                    }
                  if (!*pe) break;
                  p = pe + 1;
                  pe = p;
               }
             else
               {
                  eina_strbuf_append_char(buf, *pe);
                  pe++;
               }
          }

        eina_strbuf_free(buf);
     }
   p = eina_list_data_get(eina_list_last(names));
   if ((!p) || (strcmp(p, "default")))
     {
        p = eina_stringshare_add("default");
        if (p) names = eina_list_append(names, p);
     }
   if (th->cache) eina_hash_free(th->cache);
   th->cache = eina_hash_string_superfast_new(EINA_FREE_CB(eina_file_close));
   if (th->cache_data) eina_hash_free(th->cache_data);
   th->cache_data = eina_hash_string_superfast_new(EINA_FREE_CB(eina_stringshare_del));
   if (th->cache_style_load_failed) eina_hash_free(th->cache_style_load_failed);
   th->cache_style_load_failed = eina_hash_string_superfast_new(NULL);
   _elm_theme_file_clean(&th->themes);
   th->theme_items = eina_list_free(th->theme_items);

   EINA_LIST_FREE(names, p)
     _elm_theme_file_item_add(&th->themes, p, EINA_FALSE, EINA_TRUE);
   elm_theme_get(th);
}

/**
 * @internal
 * @brief Initializes the default theme system for Elementary.
 *
 * This function must be called during Elementary initialization. It creates
 * the singleton default theme object that will be used by all widgets unless
 * a specific theme is set on them. It's safe to call this multiple times.
 */
void
_elm_theme_init(void)
{
   Eo *theme_default_obj;
   Efl_Ui_Theme_Data *td;

   if (theme_default) return;

   theme_default_obj = efl_add(EFL_UI_THEME_CLASS, efl_main_loop_get());
   td = efl_data_scope_get(theme_default_obj, EFL_UI_THEME_CLASS);
   theme_default = td->th;
}

/**
 * @internal
 * @brief Shuts down the Elementary theme system.
 *
 * This function is called during Elementary shutdown. It cleans up all
 * created theme objects, including the default theme, freeing all associated
 * resources.
 */
void
_elm_theme_shutdown(void)
{
   Elm_Theme *th;

   while (themes)
     {
        th = eina_list_data_get(themes);

        /* In theme object destructor, theme is deallocated and the theme is
         * removed from themes list in _elm_theme_free_internal().
         * Consequently, themes list is updated.
         */
        efl_del(th->eo_theme);
     }
}





/**
 * @internal
 * @brief Gathers all used color classes from a list of theme files.
 *
 * This helper function iterates through a given list of theme file handles,
 * extracts the list of used color classes from each, and appends them to
 * the provided list.
 *
 * @param list The list to append color class names to. Can be @c NULL to start a new list.
 * @param handles An inlist of @c Elm_Theme_File handles to process.
 * @return An updated Eina_List containing the stringshared names of all found
 *         color classes.
 */
static Eina_List *
_elm_theme_file_color_class_list(Eina_List *list,
                                 Eina_Inlist *handles)
{
   Eina_Stringshare *c;
   Eina_List *ccl;
   Eina_List *ll;
   Elm_Theme_File *etf;

   EINA_INLIST_FOREACH(handles, etf)
     {
        ccl = edje_mmap_color_class_used_list(etf->handle);
        EINA_LIST_FOREACH(ccl, ll, c)
          {
             list = eina_list_append(list, eina_stringshare_add(c));
          }
        edje_file_color_class_used_free(ccl);
     }
   return list;
}

/**
 * @internal
 * @brief Hash foreach callback to build a list of unique strings.
 *
 * This callback is used with eina_hash_foreach to iterate over a hash of
 * unique color class names and append each name to an Eina_List.
 *
 * @param hash The hash being iterated.
 * @param key The hash key (the color class name).
 * @param data The data associated with the key (unused).
 * @param fdata A pointer to the Eina_List to append the key to.
 * @return @c EINA_TRUE to continue iteration.
 */
static Eina_Bool
_elm_theme_color_class_list_hash_cb(const Eina_Hash *hash EINA_UNUSED,
                                    const void *key,
                                    void *data EINA_UNUSED,
                                    void *fdata)
{
   Eina_List **list = fdata;

   *list = eina_list_append(*list, eina_stringshare_add(key));
   return EINA_TRUE;
}

EAPI Eina_List *
elm_theme_color_class_list(Elm_Theme *th)
{
   Eina_List *list;
   Eina_Hash *hash;
   const char *s;

   if (!th) th = theme_default;
   if (!th) return NULL;

   // go through all possible theme files and find collections that match
   list = _elm_theme_file_color_class_list(NULL, th->overlay);
   list = _elm_theme_file_color_class_list(list, th->themes);
   list = _elm_theme_file_color_class_list(list, th->extension);

   hash = eina_hash_string_superfast_new(NULL);
   EINA_LIST_FREE(list, s)
     {
        if (!eina_hash_find(hash, s)) eina_hash_add(hash, s, hash);
        eina_stringshare_del(s);
     }
   eina_hash_foreach(hash, _elm_theme_color_class_list_hash_cb, &list);
   eina_hash_free(hash);

   // sort the list nicely at the end
   list = eina_list_sort(list, 0, EINA_COMPARE_CB(strcmp));
   return list;
}

EAPI void
elm_theme_color_class_list_free(Eina_List *list)
{
   const char *s;

   EINA_LIST_FREE(list, s) eina_stringshare_del(s);
}

EAPI Elm_Theme *
elm_theme_new(void)
{
   Eo *obj = efl_add(EFL_UI_THEME_CLASS, efl_main_loop_get());
   Efl_Ui_Theme_Data *td = efl_data_scope_get(obj, EFL_UI_THEME_CLASS);
   return td->th;
}

EAPI void
elm_theme_free(Elm_Theme *th)
{
   EINA_SAFETY_ON_NULL_RETURN(th);

   /* Destructs theme object and theme is deallocated in
    * _elm_theme_free_internal() in theme object desctructor.
    */
   if (efl_ref_count(th->eo_theme) > 1)
     efl_unref(th->eo_theme);
   else
     efl_del(th->eo_theme);
}

/**
 * @internal
 * @brief Copies a list of theme files.
 *
 * This function performs a deep copy of an Eina_Inlist of @c Elm_Theme_File
 * structures. It duplicates the file handles and stringshares for each item.
 *
 * @param dst A pointer to the destination list.
 * @param src A pointer to the source list.
 */
static void
elm_theme_files_copy(Eina_Inlist **dst, Eina_Inlist **src)
{
   Elm_Theme_File *etf, *cpy;

   EINA_INLIST_FOREACH(*src, etf)
     {
        cpy = malloc(sizeof(Elm_Theme_File));
        EINA_SAFETY_ON_NULL_RETURN(cpy);
        cpy->item = eina_stringshare_ref(etf->item);
        cpy->handle = eina_file_dup(etf->handle);
        *dst = eina_inlist_append(*dst, EINA_INLIST_GET(cpy));
     }
}

EAPI void
elm_theme_copy(Elm_Theme *th, Elm_Theme *thdst)
{
   Eo *thdst_obj;
   Efl_Ui_Theme_Data *thdst_td;

   if (!th) th = theme_default;
   if (!th) return;
   if (!thdst) thdst = theme_default;
   if (!thdst) return;

   //Clear the given theme and restore the relationship with theme object.
   thdst_obj = thdst->eo_theme;
   _elm_theme_clear(thdst);
   thdst->eo_theme = thdst_obj;
   thdst_td = efl_data_scope_get(thdst_obj, EFL_UI_THEME_CLASS);
   thdst_td->th = thdst;

   if (th->ref_theme)
     {
        thdst->ref_theme = th->ref_theme;
        thdst->ref_theme->referrers =
           eina_list_append(thdst->ref_theme->referrers, thdst);
        efl_ref(thdst->ref_theme->eo_theme);
     }
   elm_theme_files_copy(&thdst->overlay, &th->overlay);
   elm_theme_files_copy(&thdst->themes, &th->themes);
   elm_theme_files_copy(&thdst->extension, &th->extension);

   if (th->theme) thdst->theme = eina_stringshare_add(th->theme);
   elm_theme_flush(thdst);
}

EAPI void
elm_theme_ref_set(Elm_Theme *th, Elm_Theme *thref)
{
   Eo *th_obj;
   Efl_Ui_Theme_Data *th_td;

   if (!th) th = theme_default;
   if (!th) return;
   if (!thref) thref = theme_default;
   if (!thref) return;
   if (th->ref_theme == thref) return;

   //Clear the given theme and restore the relationship with theme object.
   th_obj = th->eo_theme;
   _elm_theme_clear(th);
   th->eo_theme = th_obj;
   th_td = efl_data_scope_get(th_obj, EFL_UI_THEME_CLASS);
   th_td->th = th;

   if (thref)
     {
        thref->referrers = eina_list_append(thref->referrers, th);
        efl_ref(thref->eo_theme);
     }
   th->ref_theme = thref;
   elm_theme_flush(th);
}

EAPI Elm_Theme *
elm_theme_ref_get(const Elm_Theme *th)
{
   if (!th) th = theme_default;
   if (!th) return NULL;
   return th->ref_theme;
}

EAPI Elm_Theme *
elm_theme_default_get(void)
{
   return theme_default;
}

EAPI void
elm_theme_overlay_add(Elm_Theme *th, const char *item)
{
   if (!th) th = theme_default;
   if (!th) return;
   efl_ui_theme_overlay_add(th->eo_theme, item);
}

EAPI void
elm_theme_overlay_del(Elm_Theme *th, const char *item)
{
   if (!th) th = theme_default;
   if (!th) return;
   efl_ui_theme_overlay_del(th->eo_theme, item);
}

EAPI void
elm_theme_overlay_mmap_add(Elm_Theme *th, const Eina_File *f)
{
   Eina_File *file = eina_file_dup(f);

   if (!th) th = theme_default;
   if (!th)
     {
        eina_file_close(file); // close matching open (finalize expected to consume eina file) OK
        return;
     }
   th->overlay_items = eina_list_free(th->overlay_items);
   _elm_theme_item_finalize(&th->overlay, eina_file_filename_get(file), file, EINA_TRUE, EINA_FALSE);
   elm_theme_flush(th);
}

EAPI void
elm_theme_overlay_mmap_del(Elm_Theme *th, const Eina_File *f)
{
   if (!f) return;
   if (!th) th = theme_default;
   if (!th) return;
   th->overlay_items = eina_list_free(th->overlay_items);
   _elm_theme_file_mmap_del(&th->overlay, f);
   elm_theme_flush(th);
}

EAPI const Eina_List *
elm_theme_overlay_list_get(const Elm_Theme *th)
{
   if (!th) th = theme_default;
   if (!th) return NULL;
   if (!th->overlay_items)
     {
        Elm_Theme_File *etf;

        EINA_INLIST_FOREACH(th->overlay, etf)
          ((Elm_Theme*)th)->overlay_items = eina_list_append(th->overlay_items, etf->item);
     }
   return th->overlay_items;
}

EAPI void
elm_theme_extension_add(Elm_Theme *th, const char *item)
{
   if (!th) th = theme_default;
   if (!th) return;
   efl_ui_theme_extension_add(th->eo_theme, item);
}

EAPI void
elm_theme_extension_del(Elm_Theme *th, const char *item)
{
   if (!th) th = theme_default;
   if (!th) return;
   efl_ui_theme_extension_del(th->eo_theme, item);
}

EAPI void
elm_theme_extension_mmap_add(Elm_Theme *th, const Eina_File *f)
{
   Eina_File *file;

   if (!f) return;
   if (!th) th = theme_default;
   if (!th) return;
   file = eina_file_dup(f);
   th->extension_items = eina_list_free(th->extension_items);
   _elm_theme_item_finalize(&th->extension, eina_file_filename_get(file), file, EINA_FALSE, EINA_FALSE);
   elm_theme_flush(th);
}

EAPI void
elm_theme_extension_mmap_del(Elm_Theme *th, const Eina_File *f)
{
   if (!f) return;
   if (!th) th = theme_default;
   if (!th) return;
   th->extension_items = eina_list_free(th->extension_items);
   _elm_theme_file_mmap_del(&th->extension, f);
   elm_theme_flush(th);
}

EAPI const Eina_List *
elm_theme_extension_list_get(const Elm_Theme *th)
{
   if (!th) th = theme_default;
   if (!th) return NULL;
   if (!th->extension_items)
     {
        Elm_Theme_File *etf;

        EINA_INLIST_FOREACH(th->extension, etf)
          ((Elm_Theme*)th)->extension_items = eina_list_append(th->extension_items, etf->item);
     }
   return th->extension_items;
}

EAPI void
elm_theme_set(Elm_Theme *th, const char *theme)
{
   if (!th) th = theme_default;
   if (!th) return;
   _elm_theme_parse(th, theme);
   ELM_SAFE_FREE(th->theme, eina_stringshare_del);
   elm_theme_flush(th);
   if (th == theme_default)
     eina_stringshare_replace(&_elm_config->theme, theme);
}

EAPI const char *
elm_theme_get(Elm_Theme *th)
{
   if (!th) th = theme_default;
   if (!th) return NULL;
   if (!th->theme)
     {
        Eina_Strbuf *buf;
        Elm_Theme_File *etf;
        const char *f;

        buf = eina_strbuf_new();
        EINA_INLIST_FOREACH(th->themes, etf)
          {
             f = etf->item;
             while (*f)
               {
                  if (*f == ':')
                    eina_strbuf_append_char(buf, '\\');
                  eina_strbuf_append_char(buf, *f);

                  f++;
               }
             if (EINA_INLIST_GET(etf)->next)
               eina_strbuf_append_char(buf, ':');
          }
        th->theme = eina_stringshare_add(eina_strbuf_string_get(buf));
        eina_strbuf_free(buf);
     }
   return th->theme;
}

EAPI const Eina_List *
elm_theme_list_get(const Elm_Theme *th)
{
   if (!th) th = theme_default;
   if (!th) return NULL;
   if (!th->theme_items)
     {
        Elm_Theme_File *etf;

        EINA_INLIST_FOREACH(th->themes, etf)
          ((Elm_Theme*)th)->theme_items = eina_list_append(th->theme_items, etf->item);
     }
   return th->theme_items;
}

EAPI char *
elm_theme_list_item_path_get(const char *f, Eina_Bool *in_search_path)
{
   static const char *home = NULL;
   char buf[PATH_MAX];

   if (!f)
     {
        if (in_search_path) *in_search_path = EINA_FALSE;
        return NULL;
     }

   if (!home)
     {
        home = eina_environment_home_get();
        if (!home) home = "";
     }

   if (!eina_file_path_relative(f) ||
       ((f[0] == '.') && (f[1] == '/')) ||
       ((f[0] == '.') && (f[1] == '.') && (f[2] == '/')))
     {
        if (in_search_path) *in_search_path = EINA_FALSE;
        return strdup(f);
     }
   else if (((f[0] == '~') && (f[1] == '/')))
     {
        if (in_search_path) *in_search_path = EINA_FALSE;
        snprintf(buf, sizeof(buf), "%s/%s", home, f + 2);
        return strdup(buf);
     }
   snprintf(buf, sizeof(buf), "%s/"ELEMENTARY_BASE_DIR"/themes/%s.edj", home, f);
   if (ecore_file_exists(buf))
     {
        if (in_search_path) *in_search_path = EINA_TRUE;
        return strdup(buf);
     }

   snprintf(buf, sizeof(buf), "%s/themes/%s.edj", _elm_data_dir, f);
   if (ecore_file_exists(buf))
     {
        if (in_search_path) *in_search_path = EINA_TRUE;
        return strdup(buf);
     }

   if (in_search_path) *in_search_path = EINA_FALSE;
   return NULL;
}

EAPI void
elm_theme_flush(Elm_Theme *th)
{
   if (!th) th = theme_default;
   if (!th) return;
   if (th->cache) eina_hash_free(th->cache);
   th->cache = eina_hash_string_superfast_new(EINA_FREE_CB(eina_file_close));
   if (th->cache_data) eina_hash_free(th->cache_data);
   th->cache_data = eina_hash_string_superfast_new(EINA_FREE_CB(eina_stringshare_del));
   if (th->cache_style_load_failed) eina_hash_free(th->cache_style_load_failed);
   th->cache_style_load_failed = eina_hash_string_superfast_new(NULL);
   _elm_win_rescale(th, EINA_TRUE);
   if (th->referrers)
     {
        Eina_List *l;
        Elm_Theme *th2;

        EINA_LIST_FOREACH(th->referrers, l, th2) elm_theme_flush(th2);
     }
}

EAPI void
elm_theme_full_flush(void)
{
   Eina_List *l;
   Elm_Theme *th;

   EINA_LIST_FOREACH(themes, l, th)
     {
        elm_theme_flush(th);
     }
   elm_theme_flush(theme_default);
}

EAPI Eina_List *
elm_theme_name_available_list_new(void)
{
   Eina_List *list = NULL;
   Eina_List *dir, *l;
   char buf[PATH_MAX], *file, *s, *th;
   static const char *home = NULL;

   if (!home)
     {
        home = eina_environment_home_get();
        if (!home) home = "";
     }

   snprintf(buf, sizeof(buf), "%s/"ELEMENTARY_BASE_DIR"/themes", home);
   dir = ecore_file_ls(buf);
   EINA_LIST_FREE(dir, file)
     {
        snprintf(buf, sizeof(buf), "%s/"ELEMENTARY_BASE_DIR"/themes/%s", home, file);
        if ((!ecore_file_is_dir(buf)) && (ecore_file_size(buf) > 0))
          {
             if (eina_str_has_extension(file, ".edj"))
               {
                  th = strdup(file);
                  s = strrchr(th, '.');
                  *s = 0;
                  list = eina_list_append(list, th);
               }
          }
        free(file);
     }

   snprintf(buf, sizeof(buf), "%s/themes", _elm_data_dir);
   dir = ecore_file_ls(buf);
   EINA_LIST_FREE(dir, file)
     {
        snprintf(buf, sizeof(buf), "%s/themes/%s", _elm_data_dir, file);
        if ((!ecore_file_is_dir(buf)) && (ecore_file_size(buf) > 0))
          {
             if (eina_str_has_extension(file, ".edj"))
               {
                  int dupp;

                  th = strdup(file);
                  s = strrchr(th, '.');
                  *s = 0;
                  dupp = 0;
                  EINA_LIST_FOREACH(list, l, s)
                    {
                       if (!strcmp(s, th))
                         {
                            dupp = 1;
                            break;
                         }
                    }
                  if (dupp) free(th);
                  else list = eina_list_append(list, th);
               }
          }
        free(file);
     }
   list = eina_list_sort(list, 0, EINA_COMPARE_CB(strcasecmp));
   return list;
}

EAPI void
elm_theme_name_available_list_free(Eina_List *list)
{
   char *s;
   EINA_LIST_FREE(list, s) free(s);
}

EAPI void
elm_object_theme_set(Evas_Object *obj, Elm_Theme *th)
{
   EINA_SAFETY_ON_NULL_RETURN(obj);
   elm_widget_theme_set(obj, th);
}

EAPI Elm_Theme *
elm_object_theme_get(const Evas_Object *obj)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(obj, NULL);
   return elm_widget_theme_get(obj);
}

EAPI const char *
elm_theme_data_get(Elm_Theme *th, const char *key)
{
   if (!th) th = theme_default;
   if (!th) return NULL;
   return _elm_theme_data_find(th, key);
}

EAPI const char *
elm_theme_group_path_find(Elm_Theme *th, const char *group)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(group, NULL);
   Eina_File *th_file = NULL;
   if (!th) th = theme_default;
   if (!th) return NULL;

   th_file = _elm_theme_group_file_find(th, group);
   if (th_file)
     return eina_file_filename_get(th_file);
   return NULL;
}

/**
 * @internal
 * @brief Gathers all Edje groups starting with a base string from a list of theme files.
 *
 * This helper function iterates through a list of theme files and finds all
 * Edje group (collection) names that start with the given base string. It
 * ensures that each unique group name is added to the output list only once.
 *
 * @param list The list to append group names to. Can be @c NULL to start a new list.
 * @param handles An inlist of @c Elm_Theme_File handles to process.
 * @param base The base string to match at the beginning of group names.
 * @param len The length of the base string, passed for efficiency.
 * @return An updated Eina_List containing the stringshared names of all matching
 *         groups.
 */
static Eina_List *
_elm_theme_file_group_base_list(Eina_List *list,
                                Eina_Inlist *handles,
                                const char *base, int len)
{
   Eina_Stringshare *c, *c2;
   Eina_List *coll;
   Eina_List *in, *ll;
   Elm_Theme_File *etf;

   EINA_INLIST_FOREACH(handles, etf)
     {
        coll = edje_mmap_collection_list(etf->handle);
        EINA_LIST_FOREACH(coll, ll, c)
          {
             // if base == start of collection str
             if (!strncmp(c, base, len))
               {
                  // check if already in list
                  EINA_LIST_FOREACH(list, in, c2)
                    if (c == c2)
                      break;

                  // if not already in list append shared str to list
                  if (!in)
                    list = eina_list_append(list, eina_stringshare_ref(c));
               }
          }
        edje_mmap_collection_list_free(coll);
     }

   return list;
}

EAPI Eina_List *
elm_theme_group_base_list(Elm_Theme *th, const char *base)
{
   Eina_List *list;
   int len;

   EINA_SAFETY_ON_NULL_RETURN_VAL(base, NULL);
   if (!th) th = theme_default;
   if (!th) return NULL;

   // XXX: look results up in a hash for speed
   len = strlen(base);
   // go through all possible theme files and find collections that match
   list = _elm_theme_file_group_base_list(NULL, th->overlay,
                                          base, len);
   list = _elm_theme_file_group_base_list(list, th->themes,
                                          base, len);
   list = _elm_theme_file_group_base_list(list, th->extension,
                                          base, len);

   // sort the list nicely at the end
   list = eina_list_sort(list, 0, EINA_COMPARE_CB(strcmp));
   // XXX: store results in hash for fast lookup...
   return list;
}

EAPI const char *
elm_theme_system_dir_get(void)
{
   static char *path = NULL;
   char buf[PATH_MAX];

   if (path) return path;

   snprintf(buf, sizeof(buf), "%s/themes", _elm_data_dir);
   path = strdup(buf);

   return path;
}

EAPI const char *
elm_theme_user_dir_get(void)
{
   static char *path = NULL;
   char buf[PATH_MAX];
   const char *home;

   if (path) return path;

   home = eina_environment_home_get();
   snprintf(buf, sizeof(buf), "%s/"ELEMENTARY_BASE_DIR"/themes", home);
   path = strdup(buf);

   return path;
}

/**
 * @internal
 * @brief Internal constructor for a new Elm_Theme.
 *
 * This function allocates and initializes a new @c Elm_Theme structure. It adds
 * the "default" theme to its search path and registers it in the global list
 * of active themes. This is called by the Efl_Object constructor.
 *
 * @param obj The parent Eo object for this theme.
 * @return A newly allocated and initialized @c Elm_Theme, or @c NULL on failure.
 */
static Elm_Theme *
_elm_theme_new_internal(Eo *obj)
{
   Elm_Theme *th = calloc(1, sizeof(Elm_Theme));
   if (!th) return NULL;
   _elm_theme_file_item_add(&th->themes, "default", EINA_FALSE, EINA_TRUE);
   themes = eina_list_append(themes, th);
   th->eo_theme = obj;
   return th;
}

EOLIAN static Eo *
_efl_ui_theme_efl_object_constructor(Eo *obj, Efl_Ui_Theme_Data *pd)
{
   obj = efl_constructor(efl_super(obj, EFL_UI_THEME_CLASS));

   pd->th = _elm_theme_new_internal(obj);

   return obj;
}

/**
 * @internal
 * @brief Internal destructor for an Elm_Theme.
 *
 * This function cleans up all resources used by an @c Elm_Theme, removes it
 * from the global list of active themes, and frees the structure itself.
 * This is called by the Efl_Object destructor.
 *
 * @param th The theme to free.
 */
static void
_elm_theme_free_internal(Elm_Theme *th)
{
   _elm_theme_clear(th);
   themes = eina_list_remove(themes, th);
   free(th);
}

EOLIAN static void
_efl_ui_theme_efl_object_destructor(Eo *obj, Efl_Ui_Theme_Data *pd)
{
   Eina_Bool is_theme_default = EINA_FALSE;

   if (pd->th == theme_default)
     is_theme_default = EINA_TRUE;

   _elm_theme_free_internal(pd->th);

   if (is_theme_default) theme_default = NULL;

   efl_destructor(efl_super(obj, EFL_UI_THEME_CLASS));
}

EOLIAN static Efl_Ui_Theme *
_efl_ui_theme_default_get(void)
{
   if (theme_default)
     return theme_default->eo_theme;

   return NULL;
}

EOLIAN static void
_efl_ui_theme_extension_add(Eo *obj EINA_UNUSED, Efl_Ui_Theme_Data *pd, const char *item)
{
   if (!item) return;
   pd->th->extension_items = eina_list_free(pd->th->extension_items);
   _elm_theme_file_item_add(&pd->th->extension, item, EINA_FALSE, EINA_FALSE);
   elm_theme_flush(pd->th);
}

EOLIAN static void
_efl_ui_theme_extension_del(Eo *obj EINA_UNUSED, Efl_Ui_Theme_Data *pd, const char *item)
{
   if (!item) return;
   pd->th->extension_items = eina_list_free(pd->th->extension_items);
   _elm_theme_file_item_del(&pd->th->extension, item);
   elm_theme_flush(pd->th);
}

EOLIAN static void
_efl_ui_theme_overlay_add(Eo *obj EINA_UNUSED, Efl_Ui_Theme_Data *pd, const char *item)
{
   if (!item) return;
   pd->th->overlay_items = eina_list_free(pd->th->overlay_items);
   _elm_theme_file_item_add(&pd->th->overlay, item, EINA_TRUE, EINA_FALSE);
   elm_theme_flush(pd->th);
}

EOLIAN static void
_efl_ui_theme_overlay_del(Eo *obj EINA_UNUSED, Efl_Ui_Theme_Data *pd, const char *item)
{
   if (!item) return;
   pd->th->overlay_items = eina_list_free(pd->th->overlay_items);
   _elm_theme_file_item_del(&pd->th->overlay, item);
   elm_theme_flush(pd->th);
}

#include "efl_ui_theme.eo.c"
