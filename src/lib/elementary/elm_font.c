#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include <Elementary.h>
#include "elm_priv.h"

/**
 * @internal
 * @brief Parses a font string and retrieves or creates font properties.
 *
 * This function parses a font string, which can be a simple font name
 * (e.g., "Sans") or a font name with styles (e.g., "Sans:style=Bold,Oblique").
 * If a @p font_hash is provided, it attempts to find an existing
 * Elm_Font_Properties structure for the base font name. If not found, or if
 * @p font_hash is NULL, it creates a new one. Styles, if present in the
 * @p font string, are added to the Elm_Font_Properties.
 *
 * The @p font_hash, if not NULL, is a pointer to an Eina_Hash. If the hash
 * itself is NULL (i.e., `*font_hash == NULL`), it will be created.
 * Font properties are stored in this hash, keyed by the base font name.
 *
 * @param font_hash Pointer to a hash table for caching font properties.
 *                  If `NULL`, no caching is performed, and a new
 *                  Elm_Font_Properties is always allocated (if font is valid).
 *                  If `*font_hash` is `NULL`, a new hash table is created.
 * @param font The font string to parse.
 *             Examples: "DejaVu Sans", "Times:style=Bold", "Arial:style=Italic,Condensed"
 * @return Pointer to an Elm_Font_Properties structure, or NULL on failure.
 *         If @p font_hash was not NULL, the returned structure is stored in the hash
 *         (or retrieved from it). If @p font_hash was NULL, the caller is responsible
 *         for freeing the returned structure using elm_font_properties_free().
 */
Elm_Font_Properties *
_elm_font_properties_get(Eina_Hash **font_hash,
                         const char *font)
{
   Elm_Font_Properties *efp = NULL;
   char *token = strchr(font, ':');

   if (token &&
       !strncmp(token, ELM_FONT_TOKEN_STYLE, strlen(ELM_FONT_TOKEN_STYLE)))
     {
        char *name, *subname, *style, *substyle;
        int len;

        /* get font name */
        len = token - font;
        name = calloc(len + 1, sizeof(char));
        if (!name) return NULL;
        strncpy(name, font, len);

        /* remove subnames from the font name (should be english)  */
        subname = strchr(name, ',');
        if (subname)
          {
             len = subname - name;
             *subname = '\0';
          }

        /* add a font name */
        if (font_hash)
          efp = eina_hash_find(*font_hash, name);
        if (!efp)
          {
             efp = calloc(1, sizeof(Elm_Font_Properties));
             if (!efp)
               {
                  free(name);
                  return NULL;
               }

             efp->name = eina_stringshare_add_length(name, len);
             if (font_hash)
               {
                  if (!*font_hash)
                    *font_hash = eina_hash_string_superfast_new(NULL);
                  eina_hash_add(*font_hash, name, efp);
               }
          }

        free(name);

        style = token + strlen(ELM_FONT_TOKEN_STYLE);
        substyle = strchr(style, ',');

        //TODO: Seems to need to add all styles. not only one.
        if (substyle)
          {
             char *style_old = style;

             len = substyle - style;
             style = calloc(len + 1, sizeof(char));
             if (style)
               {
                  strncpy(style, style_old, len);
                  efp->styles = eina_list_append(efp->styles,
                                                 eina_stringshare_add(style));
                  free(style);
               }
          }
        else
          efp->styles = eina_list_append(efp->styles,
                                         eina_stringshare_add(style));
     }
   else if ((font_hash) && (!eina_hash_find(*font_hash, font)))
     {
        efp = calloc(1, sizeof(Elm_Font_Properties));
        if (!efp) return NULL;

        efp->name = eina_stringshare_add(font);
        if (!*font_hash) *font_hash = eina_hash_string_superfast_new(NULL);
        eina_hash_add(*font_hash, font, efp);
     }

   return efp;
}

/**
 * @internal
 * @brief Adds a font (and its properties) to a font hash.
 *
 * This is a convenience wrapper around _elm_font_properties_get, ensuring
 * that the font properties for @p full_name are processed and potentially
 * added to the @p font_hash.
 *
 * @param font_hash The hash table to add font properties to. If NULL, a new
 *                  hash will be created.
 * @param full_name The full font string (e.g., "Sans:style=Regular") to process.
 * @return The (potentially newly created) font_hash with the font added.
 */
Eina_Hash *
_elm_font_available_hash_add(Eina_Hash  *font_hash,
                             const char *full_name)
{
   // Note: _elm_font_properties_get handles the case where font_hash is NULL
   // and will create it if necessary when its first parameter (a pointer to the hash)
   // is used to store the new hash.
   _elm_font_properties_get(&font_hash, full_name);
   return font_hash;
}

/**
 * @internal
 * @brief Frees the memory allocated for an Elm_Font_Properties structure.
 *
 * This includes freeing the list of styles and the font name stringshare.
 *
 * @param efp Pointer to the Elm_Font_Properties structure to free.
 */
static void
_elm_font_properties_free(Elm_Font_Properties *efp)
{
   const char *str;

   EINA_LIST_FREE(efp->styles, str)
     eina_stringshare_del(str);

   eina_stringshare_del(efp->name);
   free(efp);
}

/**
 * @internal
 * @brief Callback function used by eina_hash_foreach to free Elm_Font_Properties.
 *
 * This function is called for each element in the font hash when the hash
 * is being freed. It casts the @p data to Elm_Font_Properties and calls
 * _elm_font_properties_free on it.
 *
 * @param hash The hash being iterated (unused).
 * @param key The key of the hash element (unused).
 * @param data The data (value) of the hash element, expected to be Elm_Font_Properties*.
 * @param fdata User data passed to eina_hash_foreach (unused).
 * @return EINA_TRUE to continue iteration (though for freeing, it always processes all).
 */
static Eina_Bool
_font_hash_free_cb(const Eina_Hash *hash EINA_UNUSED, const void *key EINA_UNUSED, void *data, void *fdata EINA_UNUSED)
{
   Elm_Font_Properties *efp;

   efp = data;
   _elm_font_properties_free(efp);
   return EINA_TRUE;
}

/**
 * @internal
 * @brief Deletes a hash table of font properties.
 *
 * This function iterates through the provided @p hash, freeing each
 * Elm_Font_Properties structure using _font_hash_free_cb, and then
 * frees the hash table itself.
 *
 * @param hash The font hash table to delete. If NULL, the function does nothing.
 */
void
_elm_font_available_hash_del(Eina_Hash *hash)
{
   if (!hash) return;

   eina_hash_foreach(hash, _font_hash_free_cb, NULL);
   eina_hash_free(hash);
}

EAPI Elm_Font_Properties *
elm_font_properties_get(const char *font)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(font, NULL);
   return _elm_font_properties_get(NULL, font);
}

EAPI void
elm_font_properties_free(Elm_Font_Properties *efp)
{
   const char *str;

   EINA_SAFETY_ON_NULL_RETURN(efp);
   EINA_LIST_FREE(efp->styles, str)
     eina_stringshare_del(str);
   eina_stringshare_del(efp->name);
   free(efp);
}

EAPI char *
elm_font_fontconfig_name_get(const char *name,
                             const char *style)
{
   char buf[256];

   EINA_SAFETY_ON_NULL_RETURN_VAL(name, NULL);
   if (!style || style[0] == 0) return (char *) eina_stringshare_add(name);
   snprintf(buf, 256, "%s" ELM_FONT_TOKEN_STYLE "%s", name, style);
   return (char *) eina_stringshare_add(buf);
}

EAPI void
elm_font_fontconfig_name_free(char *name)
{
   eina_stringshare_del(name);
}

EAPI Eina_Hash *
elm_font_available_hash_add(Eina_List *list)
{
   Eina_Hash *font_hash;
   Eina_List *l;
   void *key;

   font_hash = NULL;

   /* populate with default font families */
   //FIXME: Need to check whether fonts are being added multiple times.
   font_hash = _elm_font_available_hash_add(font_hash, "Sans:style=Regular");
   font_hash = _elm_font_available_hash_add(font_hash, "Sans:style=Bold");
   font_hash = _elm_font_available_hash_add(font_hash, "Sans:style=Oblique");
   font_hash = _elm_font_available_hash_add(font_hash, "Sans:style=Bold Oblique");
   font_hash = _elm_font_available_hash_add(font_hash, "Serif:style=Regular");
   font_hash = _elm_font_available_hash_add(font_hash, "Serif:style=Bold");
   font_hash = _elm_font_available_hash_add(font_hash, "Serif:style=Oblique");
   font_hash = _elm_font_available_hash_add(font_hash, "Serif:style=Bold Oblique");
   font_hash = _elm_font_available_hash_add(font_hash, "Monospace:style=Regular");
   font_hash = _elm_font_available_hash_add(font_hash, "Monospace:style=Bold");
   font_hash = _elm_font_available_hash_add(font_hash, "Monospace:style=Oblique");
   font_hash = _elm_font_available_hash_add(font_hash, "Monospace:style=Bold Oblique");

   EINA_LIST_FOREACH(list, l, key)
     if (key) font_hash = _elm_font_available_hash_add(font_hash, key);

   return font_hash;
}

EAPI void
elm_font_available_hash_del(Eina_Hash *hash)
{
   _elm_font_available_hash_del(hash);
}
