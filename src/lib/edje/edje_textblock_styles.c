#include "edje_private.h"
#include <ctype.h>

void _edje_textblock_style_update(Edje *ed, Edje_Style *stl);

/**
 * @brief Creates a copy of an Edje_Style.
 *
 * This function allocates a new Edje_Style and copies relevant
 * information from the source style. The Evas_Textblock_Style is
 * newly created, while tags, name, and cache status are referenced
 * or initialized.
 *
 * @param stl The source Edje_Style to copy.
 * @return A pointer to the newly created Edje_Style, or NULL on failure.
 */
static Edje_Style *
_edje_textblock_style_copy(Edje_Style *stl)
{
   Edje_Style *new_stl;

   new_stl = calloc(1, sizeof(Edje_Style));
   if (!new_stl) return NULL;

   new_stl->style = evas_textblock_style_new();
   evas_textblock_style_set(new_stl->style, NULL);

   // just keep a reference.
   new_stl->tags = stl->tags;
   new_stl->name = stl->name;
   new_stl->cache = EINA_FALSE;

   return new_stl;
}

/**
 * @brief Frees an Edje_Style object, typically used as a callback for eina_hash.
 *
 * This function frees the Evas_Textblock_Style associated with the
 * Edje_Style and then frees the Edje_Style structure itself.
 *
 * @param data A pointer to the Edje_Style to be freed.
 */
static void
_edje_object_textblock_styles_cache_style_free(void *data)
{
   Edje_Style *obj_stl = data;

   if (!obj_stl) return;

   if (obj_stl->style) evas_textblock_style_free(obj_stl->style);
   free(obj_stl);
}

/**
 * @brief Adds an Edje_Style to the Edje object's style cache.
 *
 * If the style (identified by its name) is not already in the cache,
 * a copy of the provided style is made and added to the cache.
 * The style is then updated.
 *
 * @param ed The Edje object.
 * @param stl The Edje_Style to add or find in the cache.
 * @return A pointer to the Edje_Style in the cache (either existing or newly added),
 *         or NULL on failure.
 */
static Edje_Style *
_edje_object_textblock_styles_cache_add(Edje *ed, Edje_Style *stl)
{
   Edje_Style *obj_stl = eina_hash_find(ed->styles, stl->name);
   // Find the style in the object cache

   if (!obj_stl)
     {
        obj_stl = _edje_textblock_style_copy(stl);

        if (obj_stl)
          {
             if (!ed->styles) ed->styles = eina_hash_stringshared_new(_edje_object_textblock_styles_cache_style_free);
             eina_hash_direct_add(ed->styles, obj_stl->name, obj_stl);
             _edje_textblock_style_update(ed, obj_stl);
          }
     }
   return obj_stl;
}

/**
 * @brief Cleans up the Edje object's textblock styles cache.
 *
 * This function frees the hash table used for caching styles at the
 * Edje object level.
 *
 * @param ed The Edje object whose style cache is to be cleaned.
 */
void
_edje_object_textblock_styles_cache_cleanup(Edje *ed)
{
   if (!ed || !ed->styles) return;
   eina_hash_free(ed->styles);
   ed->styles = NULL;
}

/**
 * @brief Retrieves an Edje_Style from the Edje object's style cache.
 *
 * @param ed The Edje object.
 * @param stl The name of the style to retrieve.
 * @return A pointer to the cached Edje_Style if found, otherwise NULL.
 */
static Edje_Style *
_edje_object_textblock_styles_cache_get(Edje *ed, const char *stl)
{
   // Find the style in the object cache
   return eina_hash_find(ed->styles, stl);
}

/**
 * @brief Checks if a font is embedded in the Edje file.
 *
 * @param edf The Edje_File data.
 * @param font The name of the font to check.
 * @return 1 if the font is embedded, 0 otherwise.
 */
static int
_edje_font_is_embedded(Edje_File *edf, const char *font)
{
   if (!eina_hash_find(edf->fonts, font)) return 0;
   return 1;
}

/**
 * @brief Parses a single key-value pair or item from a style format string.
 *
 * This function iterates through the input string `s` to extract the next
 * "word" or "item". It handles simple quoting with single quotes (') and
 * backslash escapes within quotes. The pointer `s` is advanced past the
 * parsed item.
 *
 * Example:
 *   Input: "key='value with spaces' another_key=val"
 *   First call returns: "key='value with spaces'"
 *   Second call returns: "another_key=val"
 *
 * @param s A pointer to a pointer to the current position in the format string.
 *          This will be updated to point after the parsed item.
 * @return A newly allocated string containing the parsed item, or NULL if
 *         no item could be parsed (e.g., end of string). The caller is
 *         responsible for freeing the returned string.
 */
static char *
_edje_format_parse(const char **s)
{
   const char *p;
   const char *s1 = NULL;
   const char *s2 = NULL;
   Eina_Bool quote = EINA_FALSE;

   p = *s;
   if ((!p) || (*p == 0)) return NULL;
   for (;; )
     {
        if (!s1)
          {
             if (*p != ' ') s1 = p;
             if (*p == 0) break;
          }
        else if (!s2)
          {
             if (*p == '\'')
               {
                  quote = !quote;
               }

             if ((p > *s) && (p[-1] != '\\') && (!quote))
               {
                  if (*p == ' ') s2 = p;
               }
             if (*p == 0) s2 = p;
          }
        p++;
        if (s1 && s2 && (s2 > s1))
          {
             size_t len = s2 - s1;
             char *ret = malloc(len + 1);
             memcpy(ret, s1, len);
             ret[len] = '\0';
             *s = s2;
             return ret;
          }
     }
   *s = p;
   return NULL;
}

#define _IS_STRINGS_EQUAL(str1, len1, str2, len2) (((len1)==(len2)) && !strncmp(str1, str2, len1))

/**
 * @brief Reparses a style format string, processing specific Edje tags.
 *
 * This function iterates through a format string (e.g., "font=Sans font_size=10 color_class=mycolor"),
 * identifies specific Edje-related tags like `font_source`, `text_class`, `color_class`,
 * `font_size`, `font`, and various color tags.
 *
 * For `font` tags, if the font is embedded, it prepends "edje/fonts/" to the font name.
 * For color tags (e.g., `color`, `outline_color`), if the value starts with "cc:",
 * it extracts the color class name and stores it in the `tag_ret` structure.
 * Other tags are appended to the `result` string buffer, with values escaped if necessary.
 *
 * @param edf The Edje_File data, used for checking embedded fonts.
 * @param str The input format string to reparse.
 * @param tag_ret A pointer to an Edje_Style_Tag structure where extracted information
 *                (like text_class, color_class, font_size, font name) is stored.
 *                Can be NULL if this information is not needed.
 * @param result An Eina_Strbuf where the reparsed and potentially modified style string
 *               is built.
 */
static void
_edje_format_reparse(Edje_File *edf, const char *str, Edje_Style_Tag *tag_ret, Eina_Strbuf *result)
{
   char *s2, *item;
   const char *s;

   s = str;
   while ((item = _edje_format_parse(&s)))
     {
        const char *pos = strchr(item, '=');
        if (pos)
          {
             size_t key_len = pos - item;
             const char *key = item;
             const char *val = pos + 1;

             if (_IS_STRINGS_EQUAL(key, key_len, "font_source", 11))
               {
                  /* dont allow font sources */
               }
             else if (_IS_STRINGS_EQUAL(key, key_len, "text_class", 10))
               {
                  if (tag_ret)
                    eina_stringshare_replace(&(tag_ret->text_class), val);

                  // no need to add text_class tag to style
                  // as evas_textblock_style has no idea about
                  // text_class tag.
                  free(item);
                  continue;
               }
             else if (_IS_STRINGS_EQUAL(key, key_len, "color_class", 11))
               {
                  if (tag_ret)
                    eina_stringshare_replace(&(tag_ret->color_class), val);

                  // no need to add color_class tag to style
                  // as evas_textblock_style has no idea about
                  // color_class tag.
                  free(item);
                  continue;
               }
             else if (_IS_STRINGS_EQUAL(key, key_len, "font_size", 9))
               {
                  if (tag_ret)
                    tag_ret->font_size = eina_convert_strtod_c(val, NULL);
               }
             else if (_IS_STRINGS_EQUAL(key, key_len, "font", 4)) /* Fix fonts */
               {
                  if (tag_ret)
                    {
                       if (_edje_font_is_embedded(edf, val))
                         {
                            char buffer[120];
                            snprintf(buffer, sizeof(buffer), "edje/fonts/%s", val);
                            eina_stringshare_replace(&tag_ret->font, buffer);
                            if (eina_strbuf_length_get(result)) eina_strbuf_append(result, " ");
                            eina_strbuf_append(result, "font=");
                            eina_strbuf_append(result, buffer);
                            continue;
                         }
                       else
                         {
                            tag_ret->font = eina_stringshare_add(val);
                         }
                    }
               }
             // handle colorclass replacements of color like: color=cc:/fg/normal
             else if ((_IS_STRINGS_EQUAL(key, key_len, "color", 5)) ||
                      (_IS_STRINGS_EQUAL(key, key_len, "outline_color", 13)) ||
                      (_IS_STRINGS_EQUAL(key, key_len, "shadow_color", 12)) ||
                      (_IS_STRINGS_EQUAL(key, key_len, "underline_color", 15)) ||
                      (_IS_STRINGS_EQUAL(key, key_len, "underline2_color", 16)) ||
                      (_IS_STRINGS_EQUAL(key, key_len, "secondary_underline_color", 25)) ||
                      (_IS_STRINGS_EQUAL(key, key_len, "underline_dash_color", 20)) ||
                      (_IS_STRINGS_EQUAL(key, key_len, "underline_dashed_color", 22)) ||
                      (_IS_STRINGS_EQUAL(key, key_len, "glow_color", 10)) ||
                      (_IS_STRINGS_EQUAL(key, key_len, "glow2_color", 11)) ||
                      (_IS_STRINGS_EQUAL(key, key_len, "secondary_glow_color", 20)) ||
                      (_IS_STRINGS_EQUAL(key, key_len, "backing_color", 13)) ||
                      (_IS_STRINGS_EQUAL(key, key_len, "background_color", 16)) ||
                      (_IS_STRINGS_EQUAL(key, key_len, "strikethrough_color", 19)))
               {
                  if (!strncmp(val, "cc:", 3))
                    {
                       if ((_IS_STRINGS_EQUAL(key, key_len, "color", 5)))
                         eina_stringshare_replace(&(tag_ret->color_class), val + 3);
                       else if ((_IS_STRINGS_EQUAL(key, key_len, "outline_color", 13)))
                         eina_stringshare_replace(&(tag_ret->outline_color_class), val + 3);
                       else if ((_IS_STRINGS_EQUAL(key, key_len, "shadow_color", 12)))
                         eina_stringshare_replace(&(tag_ret->shadow_color_class), val + 3);
                       else if ((_IS_STRINGS_EQUAL(key, key_len, "underline_color", 15)))
                         eina_stringshare_replace(&(tag_ret->underline_color_class), val + 3);
                       else if ((_IS_STRINGS_EQUAL(key, key_len, "underline2_color", 16)) ||
                                (_IS_STRINGS_EQUAL(key, key_len, "secondary_underline_color", 25)))
                         eina_stringshare_replace(&(tag_ret->underline2_color_class), val + 3);
                       else if ((_IS_STRINGS_EQUAL(key, key_len, "underline_dash_color", 20)) ||
                                (_IS_STRINGS_EQUAL(key, key_len, "underline_dashed_color", 22)))
                         eina_stringshare_replace(&(tag_ret->underline_dash_color_class), val + 3);
                       else if ((_IS_STRINGS_EQUAL(key, key_len, "glow_color", 10)))
                         eina_stringshare_replace(&(tag_ret->glow_color_class), val + 3);
                       else if ((_IS_STRINGS_EQUAL(key, key_len, "glow2_color", 11)) ||
                                (_IS_STRINGS_EQUAL(key, key_len, "secondary_glow_color", 20)))
                         eina_stringshare_replace(&(tag_ret->glow2_color_class), val + 3);
                       else if ((_IS_STRINGS_EQUAL(key, key_len, "backing_color", 13)) ||
                                (_IS_STRINGS_EQUAL(key, key_len, "background_color", 16)))
                         eina_stringshare_replace(&(tag_ret->backing_color_class), val + 3);
                       else if ((_IS_STRINGS_EQUAL(key, key_len, "strikethrough_color", 19)))
                         eina_stringshare_replace(&(tag_ret->strikethrough_color_class), val + 3);
                    }
               }
             // XXX: how do we do better for:
             // color
             // outline_color
             // shadow_color
             // 
             // XXX: and do these too:
             // underline_color
             // underline2_color | secondary_underline_color
             // underline_dash_color | underline_dashed_color
             // glow_color
             // glow2_color | secondary_glow_color
             // backing_color | background_color
             // strikethrough_color
             s2 = eina_str_escape(item);
             if (s2)
               {
                  if (eina_strbuf_length_get(result)) eina_strbuf_append(result, " ");
                  eina_strbuf_append(result, s2);
                  free(s2);
               }
          }
        else
          {
             if (eina_strbuf_length_get(result)) eina_strbuf_append(result, " ");
             eina_strbuf_append(result, item);
          }
        free(item);
     }
}

/**
 * @brief Updates or appends a specific tag in a style string buffer.
 *
 * Searches for the last occurrence of `key` (e.g., "font=") in the `style`
 * string buffer. If found, it replaces the entire tag (key and its old value)
 * with `new_tag`. If not found, `new_tag` is appended to the `style` buffer,
 * prefixed with a space.
 *
 * Example:
 *   style (before): "font_size=10 font=Arial"
 *   key: "font="
 *   new_tag: "font=Times"
 *   style (after): "font_size=10 font=Times"
 *
 *   style (before): "font_size=10"
 *   key: "font="
 *   new_tag: "font=Times"
 *   style (after): "font_size=10 font=Times"
 *
 * @param style The Eina_Strbuf containing the style string to modify.
 * @param key The key of the tag to search for (e.g., "font=", "font_size=").
 * @param new_tag The new complete tag string (e.g., "font=Times", "font_size=12.0").
 */
static void
_edje_textblock_tag_update(Eina_Strbuf *style, const char *key, const char *new_tag)
{
   char *ptr = strstr(eina_strbuf_string_get(style), key);
   char *last = NULL;

   while (ptr != NULL)
     {
        last = ptr;
        ptr = strstr(ptr + 1, key);
     }

   if (last)
     {
        char *tok = strdup(last);
        int cnt = 0;
        while (*tok && !isspace(*tok))
          {
             tok++;
             cnt++;
          }
        if (*tok) *tok = 0;
        tok -= cnt;

        eina_strbuf_replace_last(style, tok, new_tag);
        free(tok);
     }
   else
     {
        eina_strbuf_append(style, " ");
        eina_strbuf_append(style, new_tag);
     }
}

/**
 * @brief Updates the font tag in a style string buffer.
 *
 * A convenience wrapper around `_edje_textblock_tag_update` specifically
 * for the "font=" tag.
 *
 * @param style The Eina_Strbuf containing the style string.
 * @param new_value The new font name (e.g., "Arial", "edje/fonts/MyFont").
 */
static void
_edje_textblock_font_tag_update(Eina_Strbuf *style, const char *new_value)
{
   const char *font_key = "font=";
   char new_font[256] = {0,};
   snprintf(new_font, sizeof(new_font), "%s%s", font_key, new_value);
   _edje_textblock_tag_update(style, font_key, new_font);
}

/**
 * @brief Updates the font_size tag in a style string buffer.
 *
 * A convenience wrapper around `_edje_textblock_tag_update` specifically
 * for the "font_size=" tag. The new value is formatted to one decimal place.
 *
 * @param style The Eina_Strbuf containing the style string.
 * @param new_value The new font size (e.g., 10.0, 12.5).
 */
static void
_edje_textblock_font_size_tag_update(Eina_Strbuf *style, double new_value)
{
   const char *font_size_key = "font_size=";
   char new_font_size[32] = {0,};
   snprintf(new_font_size, sizeof(new_font_size), "%s%.1f", font_size_key, new_value);
   _edje_textblock_tag_update(style, font_size_key, new_font_size);
}

/* Update the given evas_style
 *
 * @param ed The Edje object containing the given style which needs to be updated.
 * @param stl The Edje_Style which needs to be updated.
 *
 * @note This function should only be called when a style needs to be recomputed,
 *       typically from _edje_textblock_style_get() for styles that are not
 *       readonly and not cached, or when a dependency (like a text class or
 *       color class) changes.
 *       Misuse can lead to performance regressions.
 *
 * This function reconstructs the Evas_Textblock_Style string associated with
 * the given Edje_Style. It iterates through the style's tags, incorporates
 * properties from text classes and color classes, and applies font fallbacks
 * and font sources.
 *
 * The process involves:
 * 1. Initializing a string buffer for the Evas style string.
 * 2. Appending font source information if available from the Edje file.
 * 3. Iterating through each Edje_Style_Tag in `stl->tags`:
 *    a. Appending the tag's key and its original value.
 *    b. If the tag is "DEFAULT", append global font fallbacks and font source.
 *    c. If a text_class is associated with the tag:
 *       i. Calculate the effective font size based on the tag's font_size
 *          and the text_class's size, updating the style string if different.
 *       ii. Determine the effective font name based on the tag's font and
 *           the text_class's font, updating the style string.
 *    d. If various color_classes (e.g., color_class, outline_color_class) are set,
 *       find the corresponding Edje_Color_Class and append the resolved color
 *       (e.g., "color=#RRGGBBAA") to the style string.
 * 4. Setting the fully constructed string to `stl->style` using
 *    `evas_textblock_style_set()`.
 * 5. Marking the style as cached (`stl->cache = EINA_TRUE`).
 */
void
_edje_textblock_style_update(Edje *ed, Edje_Style *stl)
{
   Eina_List *l;
   Eina_Strbuf *txt = NULL;
   Edje_Style_Tag *tag;
   Edje_Text_Class *tc;
   Edje_Color_Class *cc;
   char *fontset = _edje_fontset_append_escaped, *fontsource = NULL;

   if (!ed->file) return;

   /* Make sure the style is already defined */
   if (!stl->style) return;

   /* this check is only here to catch misuse of this function */
   if (stl->readonly)
     {
        WRN("style_update() shouldn't be called for readonly style. performance regression : %s", stl->name);
        return;
     }

   /* this check is only here to catch misuse of this function */
   if (stl->cache)
     {
        WRN("style_update() shouldn't be called for cached style. performance regression : %s", stl->name);
        return;
     }

   if (!txt)
     txt = eina_strbuf_new();

   if (ed->file->fonts)
     fontsource = eina_str_escape(ed->file->path);

   /* Build the style from each tag */
   EINA_LIST_FOREACH(stl->tags, l, tag)
     {
        if (!tag->key) continue;

        /* Add Tag Key */
        eina_strbuf_append(txt, tag->key);
        eina_strbuf_append(txt, "='");

        /* Configure fonts from text class if it exists */
        if (tag->text_class) tc = _edje_text_class_find(ed, tag->text_class);
        else tc = NULL;

        /* Add and Handle tag parsed data */
        eina_strbuf_append(txt, tag->value);

        if (!strcmp(tag->key, "DEFAULT"))
          {
             if (fontset)
               {
                  eina_strbuf_append(txt, " font_fallbacks=");
                  eina_strbuf_append(txt, fontset);
               }
             if (fontsource)
               {
                  eina_strbuf_append(txt, " font_source=");
                  eina_strbuf_append(txt, fontsource);
               }
          }
        if (tc && tc->size)
          {
             double new_size = _edje_text_size_calc(tag->font_size, tc);
             if (!EINA_DBL_EQ(tag->font_size, new_size))
               {
                  _edje_textblock_font_size_tag_update(txt, new_size);
               }
          }
        /* Add font name last to save evas from multiple loads */
        if (tc && tc->font)
          {
             const char *f;
             char *sfont = NULL;

             f = _edje_text_font_get(tag->font, tc->font, &sfont);
             _edje_textblock_font_tag_update(txt, f);

             if (sfont) free(sfont);
          }
        if (tag->color_class)
          {
             if ((cc = _edje_color_class_recursive_find(ed, tag->color_class)))
               eina_strbuf_append_printf(txt, " color=#%02x%02x%02x%02x", cc->r,  cc->g,  cc->b,  cc->a);
          }
        if (tag->outline_color_class)
          {
             if ((cc = _edje_color_class_recursive_find(ed, tag->outline_color_class)))
               eina_strbuf_append_printf(txt, " outline_color=#%02x%02x%02x%02x", cc->r,  cc->g,  cc->b,  cc->a);
          }
        if (tag->shadow_color_class)
          {
             if ((cc = _edje_color_class_recursive_find(ed, tag->shadow_color_class)))
               eina_strbuf_append_printf(txt, " shadow_color=#%02x%02x%02x%02x", cc->r,  cc->g,  cc->b,  cc->a);
          }
        if (tag->underline_color_class)
          {
             if ((cc = _edje_color_class_recursive_find(ed, tag->underline_color_class)))
               eina_strbuf_append_printf(txt, " underline_color=#%02x%02x%02x%02x", cc->r,  cc->g,  cc->b,  cc->a);
          }
        if (tag->underline2_color_class)
          {
             if ((cc = _edje_color_class_recursive_find(ed, tag->underline2_color_class)))
               eina_strbuf_append_printf(txt, " underline2_color=#%02x%02x%02x%02x", cc->r,  cc->g,  cc->b,  cc->a);
          }
        if (tag->underline_dash_color_class)
          {
             if ((cc = _edje_color_class_recursive_find(ed, tag->underline_dash_color_class)))
               eina_strbuf_append_printf(txt, " underline_dash_color=#%02x%02x%02x%02x", cc->r,  cc->g,  cc->b,  cc->a);
          }
        if (tag->glow_color_class)
          {
             if ((cc = _edje_color_class_recursive_find(ed, tag->glow_color_class)))
               eina_strbuf_append_printf(txt, " glow_color=#%02x%02x%02x%02x", cc->r,  cc->g,  cc->b,  cc->a);
          }
        if (tag->glow2_color_class)
          {
             if ((cc = _edje_color_class_recursive_find(ed, tag->glow2_color_class)))
               eina_strbuf_append_printf(txt, " glow2_color=#%02x%02x%02x%02x", cc->r,  cc->g,  cc->b,  cc->a);
          }
        if (tag->backing_color_class)
          {
             if ((cc = _edje_color_class_recursive_find(ed, tag->backing_color_class)))
               eina_strbuf_append_printf(txt, " backing_color=#%02x%02x%02x%02x", cc->r,  cc->g,  cc->b,  cc->a);
          }
        if (tag->strikethrough_color_class)
          {
             if ((cc = _edje_color_class_recursive_find(ed, tag->strikethrough_color_class)))
               eina_strbuf_append_printf(txt, " strikethrough_color=#%02x%02x%02x%02x", cc->r,  cc->g,  cc->b,  cc->a);
          }

        eina_strbuf_append(txt, "'");
     }
   if (fontsource) free(fontsource);

   /* Configure the style */
   stl->cache = EINA_TRUE;
   evas_textblock_style_set(stl->style, eina_strbuf_string_get(txt));
   if (txt) eina_strbuf_free(txt);
}

/**
 * @brief Searches for an Edje_Style by name in the Edje_File's style hash.
 *
 * @param ed The Edje object (to access its Edje_File).
 * @param style The name of the style to search for.
 * @return A pointer to the Edje_Style if found, otherwise NULL.
 */
static inline Edje_Style *
_edje_textblock_style_search(Edje *ed, const char *style)
{
   if (!style) return NULL;

   return eina_hash_find(ed->file->style_hash, style);
}

/**
 * @brief Adds an observer to the text classes associated with an Edje_Style.
 *
 * If the style's tags reference any text classes, this function registers
 * the provided observer to listen for changes on those text classes.
 * This is used to invalidate style caches when text classes are modified.
 *
 * @param stl The Edje_Style whose text classes will be observed.
 * @param observer The Efl_Observer to add.
 */
static inline void
_edje_textblock_style_observer_add(Edje_Style *stl, Efl_Observer* observer)
{
   Eina_List* l;
   Edje_Style_Tag *tag;

   EINA_LIST_FOREACH(stl->tags, l, tag)
     {
        if (tag->text_class)
          efl_observable_observer_add(_edje_text_class_member, tag->text_class, observer);
     }
}

/**
 * @brief Deletes an observer from the text classes associated with an Edje_Style.
 *
 * If the style's tags reference any text classes, this function unregisters
 * the provided observer from those text classes.
 *
 * @param stl The Edje_Style whose text classes were being observed.
 * @param observer The Efl_Observer to delete.
 */
static inline void
_edje_textblock_style_observer_del(Edje_Style *stl, Efl_Observer* observer)
{
   Eina_List* l;
   Edje_Style_Tag *tag;

   EINA_LIST_FOREACH(stl->tags, l, tag)
     {
        if (tag->text_class)
          efl_observable_observer_del(_edje_text_class_member, tag->text_class, observer);
     }
}

/**
 * @brief Prepares an Edje_Style for use by an Edje object.
 *
 * If the style is not readonly, this function adds the Edje object itself
 * as an observer to any text classes referenced by the style's tags.
 * It also marks the style's cache as dirty (EINA_FALSE) to ensure
 * it's recomputed when next requested.
 *
 * @param ed The Edje object that will use this style.
 * @param stl The Edje_Style to prepare.
 */
static inline void
_edje_textblock_style_add(Edje *ed, Edje_Style *stl)
{
   if (!stl) return;

   if (stl->readonly) return;

   _edje_textblock_style_observer_add(stl, ed->obj);

   // mark it dirty to recompute it later.
   stl->cache = EINA_FALSE;
}

/**
 * @brief Cleans up an Edje_Style when it's no longer used by an Edje object.
 *
 * This function removes the Edje object as an observer from any text classes
 * referenced by the style's tags.
 *
 * @param ed The Edje object that was using this style.
 * @param stl The Edje_Style to clean up.
 */
static inline void
_edje_textblock_style_del(Edje *ed, Edje_Style *stl)
{
   if (!stl) return;

   _edje_textblock_style_observer_del(stl, ed->obj);
}

/**
 * @brief Associates textblock styles with an Edje object when a part is realized.
 *
 * For a given real part (`ep`) of type TEXTBLOCK, this function finds the
 * Edje_Style(s) referenced by its default and other descriptions.
 * It then calls `_edje_textblock_style_add()` for each found style to
 * set up observers and mark them for potential recomputation.
 *
 * @param ed The Edje object.
 * @param ep The Edje_Real_Part that is being realized.
 */
void
_edje_textblock_styles_add(Edje *ed, Edje_Real_Part *ep)
{
   Edje_Part *pt = ep->part;
   Edje_Part_Description_Text *desc;
   Edje_Style *stl = NULL;
   const char *style;
   unsigned int i;

   if (pt->type != EDJE_PART_TYPE_TEXTBLOCK) return;

   /* if text class exists in the textblock styles for this part,
      add the edje to the tc member list */
   desc = (Edje_Part_Description_Text *)pt->default_desc;
   style = edje_string_get(&desc->text.style);
   stl = _edje_textblock_style_search(ed, style);

   _edje_textblock_style_add(ed, stl);

   /* If any other classes exist add them */
   for (i = 0; i < pt->other.desc_count; ++i)
     {
        desc = (Edje_Part_Description_Text *)pt->other.desc[i];
        style = edje_string_get(&desc->text.style);
        stl = _edje_textblock_style_search(ed, style);

        _edje_textblock_style_add(ed, stl);
     }
}

/**
 * @brief Disassociates textblock styles from an Edje object, typically when a part is unrealized.
 *
 * For a given part (`pt`) of type TEXTBLOCK, this function finds the
 * Edje_Style(s) referenced by its default and other descriptions.
 * It then calls `_edje_textblock_style_del()` for each found style to
 * remove observers.
 *
 * @param ed The Edje object.
 * @param pt The Edje_Part whose styles are being disassociated.
 */
void
_edje_textblock_styles_del(Edje *ed, Edje_Part *pt)
{
   Edje_Part_Description_Text *desc;
   Edje_Style *stl = NULL;
   const char *style;
   unsigned int i;

   if (pt->type != EDJE_PART_TYPE_TEXTBLOCK) return;

   desc = (Edje_Part_Description_Text *)pt->default_desc;
   style = edje_string_get(&desc->text.style);

   stl = _edje_textblock_style_search(ed, style);

   _edje_textblock_style_del(ed, stl);

   for (i = 0; i < pt->other.desc_count; ++i)
     {
        desc = (Edje_Part_Description_Text *)pt->other.desc[i];
        style = edje_string_get(&desc->text.style);
        stl = _edje_textblock_style_search(ed, style);

        _edje_textblock_style_del(ed, stl);
     }
}

/*
 * @brief Retrieves the Evas_Textblock_Style for a given style name.
 *
 * This function implements lazy computation and caching of Evas_Textblock_Style objects.
 *
 * 1. It first searches for the style in the Edje object's local cache
 *    (`_edje_object_textblock_styles_cache_get`). This cache holds styles
 *    that might have been customized or affected by object-level properties.
 * 2. If not found in the object cache, it searches in the Edje_File's global
 *    style list (`_edje_textblock_style_search`).
 * 3. If the style is found:
 *    a. If the style is marked `readonly`, its `stl->style` (the Evas_Textblock_Style)
 *       is returned directly, as it's assumed to be pre-computed and immutable.
 *    b. If the style is not `readonly` and its `cache` flag is false (dirty),
 *       `_edje_textblock_style_update()` is called to recompute the Evas_Textblock_Style
 *       based on current Edje state (e.g., text classes, color classes).
 *    c. The (potentially updated) `stl->style` is then returned.
 *
 * @param ed The Edje object requesting the style.
 * @param style The name of the style to retrieve.
 * @return A pointer to the Evas_Textblock_Style, or NULL if the style
 *         is not found or an error occurs.
 */
Evas_Textblock_Style *
_edje_textblock_style_get(Edje *ed, const char *style)
{
   Edje_Style *stl;

   if (!style) return NULL;

   // First search in Edje_Object styles list
   stl = _edje_object_textblock_styles_cache_get(ed, style);

   if (!stl)
     {
        // If not found in Edje_Object search in Edje_File styles list
        stl = _edje_textblock_style_search(ed, style);
     }

   if (!stl) return NULL;

   /* readonly style naver change */
   if (stl->readonly) return stl->style;

   /* if style is dirty recompute */
   if (!stl->cache)
     _edje_textblock_style_update(ed, stl);

   return stl->style;
}

/**
 * @brief Updates all Edje_Styles that use a specific text_class.
 *
 * This function iterates through all styles defined in the Edje file.
 * For each style, it checks if any of its tags use the specified `text_class`.
 *
 * If `is_object_level` is EINA_TRUE:
 *   - It attempts to find the style in the Edje object's local cache.
 *   - If found, it marks the cached style as dirty (`obj_stl->cache = EINA_FALSE`).
 *   - If not found in the object cache, it means this object wasn't using a
 *     customized version of this style yet. A copy of the file-level style is
 *     added to the object's cache via `_edje_object_textblock_styles_cache_add()`,
 *     which will also mark it for update.
 *
 * If `is_object_level` is EINA_FALSE:
 *   - It marks the file-level style itself as dirty (`stl->cache = EINA_FALSE`).
 *
 * This ensures that when a text_class changes, any styles depending on it
 * will be recomputed the next time they are requested.
 *
 * @param ed The Edje object (used for object-level updates and accessing the Edje_File).
 * @param text_class The name of the text_class that has been updated.
 * @param is_object_level EINA_TRUE if the update should affect the Edje object's
 *                        cached styles, EINA_FALSE if it should affect the
 *                        Edje_File's base styles.
 */
static void
_edje_textblock_style_all_update_text_class(Edje *ed, const char *text_class, Eina_Bool is_object_level)
{
   Eina_List *ll, *l;
   Edje_Style *stl;
   Edje_Style *obj_stl;
   Edje_File *edf;

   if (!ed) return;
   if (!ed->file) return;
   if (!text_class) return;

   edf = ed->file;

   // check if there is styles in file that uses this text_class
   EINA_LIST_FOREACH(edf->styles, l, stl)
     {
        Edje_Style_Tag *tag;

        if (stl->readonly) continue;

        EINA_LIST_FOREACH(stl->tags, ll, tag)
          {
             if (!tag->text_class) continue;

             if (!strcmp(tag->text_class, text_class))
               {
                  if (is_object_level)
                    {
                       obj_stl = _edje_object_textblock_styles_cache_get(ed, stl->name);
                       if (obj_stl)
                         // If already in Edje styles just make it dirty
                         obj_stl->cache = EINA_FALSE;
                       else
                         // create a copy from it if it's not exists
                         _edje_object_textblock_styles_cache_add(ed, stl);
                    }
                  else
                    {
                       // just mark it dirty so the next request
                       // for this style will trigger recomputation.
                       stl->cache = EINA_FALSE;
                    }
                  // don't need to continue searching
                  break;
               }
          }
     }
}

/*
 * Finds all the styles within a specific Edje object's scope that use a given
 * text_class and marks them for update.
 * This typically means their cached Evas_Textblock_Style will be recomputed
 * on next use.
 *
 * @param ed The Edje object.
 * @param text_class The name of the text_class that has changed.
 */
void
_edje_object_textblock_style_all_update_text_class(Edje *ed, const char *text_class)
{
   _edje_textblock_style_all_update_text_class(ed, text_class, EINA_TRUE);
}

/**
 * @brief Marks all styles in an Edje file that use a specific text_class for update.
 *
 * Finds all the styles defined in the Edje file (not specific to one Edje object)
 * that reference the given `text_class` and marks their file-level cache as dirty.
 * This ensures that any subsequent Edje object using these styles will get the
 * updated version.
 *
 * @param ed The Edje object (primarily to access its associated Edje_File).
 * @param text_class The name of the text_class that has changed.
 */
void
_edje_file_textblock_styles_all_update_text_class(Edje *ed, const char *text_class)
{
   _edje_textblock_style_all_update_text_class(ed, text_class, EINA_FALSE);
}

/**
 * @brief Parses and finalizes all textblock styles defined in an Edje file.
 *
 * This function is called after an Edje file has been loaded into memory.
 * It iterates through all `Edje_Style` definitions within the `edf->styles` list.
 * For each style:
 * 1. If `stl->style` (the Evas_Textblock_Style) is already created, it skips (though
 *    this condition `if (stl->style) break;` seems to imply it might stop processing
 *    further styles, which could be an issue if styles are not processed in a specific order
 *    or if some styles are pre-created).
 * 2. Initializes `stl->readonly` to `EINA_TRUE`. This flag will be set to `EINA_FALSE`
 *    if any dynamic elements (like text_class or color_class references) are found.
 * 3. Creates a new `Evas_Textblock_Style` and associates it with `stl->style`.
 * 4. Iterates through each `Edje_Style_Tag` of the current style:
 *    a. Appends the tag's key (e.g., "DEFAULT", "font") to a temporary style string buffer.
 *    b. Calls `_edje_format_reparse()` to process the tag's value. This function
 *       handles special Edje interpretations, like font embedding paths ("edje/fonts/...")
 *       and extraction of class names (e.g., from "cc:my_color_class"). The reparsed
 *       value updates `tag->value` and is appended to the style string buffer.
 *    c. If the tag key is "DEFAULT", it appends global font fallbacks and the
 *       font source path (if fonts are embedded in the Edje file).
 *    d. Checks if the tag introduces dynamic dependencies (text_class, various color_classes).
 *       If so, `stl->readonly` is set to `EINA_FALSE`.
 * 5. If, after processing all tags, `stl->readonly` remains `EINA_TRUE`, the fully
 *    constructed style string is set on `stl->style` using `evas_textblock_style_set()`.
 *    Readonly styles are fully defined at load time and don't change. Non-readonly
 *    styles will have their Evas_Textblock_Style string generated on-demand by
 *    `_edje_textblock_style_update()` when `_edje_textblock_style_get()` is called.
 *
 * This function essentially prepares the static parts of styles and identifies
 * which styles are dynamic due to class dependencies.
 *
 * @param edf Pointer to the Edje_File structure containing all loaded Edje data.
 */
/* When we get to here the edje file had been read into memory
 * the name of the style is established as well as the name and
 * data for the tags.  This function will create the Evas_Style
 * object for each style. The style is composed of a base style
 * followed by a list of tags.
 */
void
_edje_file_textblock_style_parse_and_fix(Edje_File *edf)
{
   Eina_List *l, *ll;
   Edje_Style *stl;
   char *fontset = _edje_fontset_append_escaped;
   Eina_Strbuf *reparseBuffer = eina_strbuf_new();
   Eina_Strbuf *styleBuffer = eina_strbuf_new();
   char *fontsource = edf->fonts ? eina_str_escape(edf->path) : NULL;

   EINA_LIST_FOREACH(edf->styles, l, stl)
     {
        Edje_Style_Tag *tag;

        if (stl->style) break;

        stl->readonly = EINA_TRUE;

        stl->style = evas_textblock_style_new();
        evas_textblock_style_set(stl->style, NULL);

        eina_strbuf_reset(styleBuffer);
        /* Build the style from each tag */
        EINA_LIST_FOREACH(stl->tags, ll, tag)
          {
             if (!tag->key) continue;

             eina_strbuf_reset(reparseBuffer);

             /* Add Tag Key */
             eina_strbuf_append(styleBuffer, tag->key);
             eina_strbuf_append(styleBuffer, "='");

             _edje_format_reparse(edf, tag->value, tag, reparseBuffer);

             /* Add and Handle tag parsed data */
             if (eina_strbuf_length_get(reparseBuffer))
               {
                  if (edf->allocated_strings &&
                      eet_dictionary_string_check(eet_dictionary_get(edf->ef), tag->value) == 0)
                    eina_stringshare_del(tag->value);
                  tag->value = eina_stringshare_add(eina_strbuf_string_get(reparseBuffer));
                  eina_strbuf_append(styleBuffer, tag->value);
               }

             if (!strcmp(tag->key, "DEFAULT"))
               {
                  if (fontset)
                    {
                       eina_strbuf_append(styleBuffer, " font_fallbacks=");
                       eina_strbuf_append(styleBuffer, fontset);
                    }
                  if (fontsource)
                    {
                       eina_strbuf_append(styleBuffer, " font_source=");
                       eina_strbuf_append(styleBuffer, fontsource);
                    }
               }
             eina_strbuf_append(styleBuffer, "'");

             if (tag->text_class) stl->readonly = EINA_FALSE;
             else if (tag->color_class) stl->readonly = EINA_FALSE;
             else if (tag->outline_color_class) stl->readonly = EINA_FALSE;
             else if (tag->shadow_color_class) stl->readonly = EINA_FALSE;
             else if (tag->underline_color_class) stl->readonly = EINA_FALSE;
             else if (tag->underline2_color_class) stl->readonly = EINA_FALSE;
             else if (tag->underline_dash_color_class) stl->readonly = EINA_FALSE;
             else if (tag->glow_color_class) stl->readonly = EINA_FALSE;
             else if (tag->glow2_color_class) stl->readonly = EINA_FALSE;
             else if (tag->backing_color_class) stl->readonly = EINA_FALSE;
             else if (tag->strikethrough_color_class) stl->readonly = EINA_FALSE;
          }
        /* Configure the style  only if it will never change again*/
        if (stl->readonly)
          evas_textblock_style_set(stl->style, eina_strbuf_string_get(styleBuffer));
     }

   if (fontsource) free(fontsource);
   eina_strbuf_free(styleBuffer);
   eina_strbuf_free(reparseBuffer);
}

/**
 * @brief Cleans up all textblock styles associated with an Edje file.
 *
 * This function is called when an Edje_File structure is being freed.
 * It iterates through all `Edje_Style` objects in `edf->styles` and
 * performs the following cleanup for each:
 * 1. Frees all associated `Edje_Style_Tag` objects:
 *    a. If `edf->allocated_strings` is true, it conditionally frees `tag->value`
 *       using `eina_stringshare_del()` only if it's not part of the EET dictionary
 *       (meaning it was likely allocated during reparsing).
 *    b. If `edf->free_strings` is true, it frees various stringshared fields within
 *       the tag (key, class names, font name) using `eina_stringshare_del()`.
 *    c. Frees the `tag` structure itself.
 * 2. If `edf->free_strings` is true, frees `stl->name` using `eina_stringshare_del()`.
 * 3. Frees the associated `Evas_Textblock_Style` (`stl->style`) if it exists.
 * 4. Frees the `stl` (Edje_Style) structure itself.
 *
 * Finally, it clears the `edf->styles` list.
 *
 * @param edf Pointer to the Edje_File structure whose styles are to be cleaned up.
 */
void
_edje_file_textblock_style_cleanup(Edje_File *edf)
{
   Edje_Style *stl;

   EINA_LIST_FREE(edf->styles, stl)
     {
        Edje_Style_Tag *tag;

        EINA_LIST_FREE(stl->tags, tag)
          {
             if (edf->allocated_strings &&
                 tag->value &&
                 eet_dictionary_string_check(eet_dictionary_get(edf->ef), tag->value) == 0)
               eina_stringshare_del(tag->value);
             if (edf->free_strings)
               {
#define STRSHRDEL(_x) if (tag->_x) eina_stringshare_del(tag->_x)
                  STRSHRDEL(key);
/*                FIXME: Find a proper way to handle it. */
                  STRSHRDEL(text_class);
                  STRSHRDEL(color_class);
                  STRSHRDEL(outline_color_class);
                  STRSHRDEL(shadow_color_class);
                  STRSHRDEL(underline_color_class);
                  STRSHRDEL(underline2_color_class);
                  STRSHRDEL(underline_dash_color_class);
                  STRSHRDEL(glow_color_class);
                  STRSHRDEL(glow2_color_class);
                  STRSHRDEL(backing_color_class);
                  STRSHRDEL(strikethrough_color_class);
                  STRSHRDEL(font);
               }
             free(tag);
          }
        if (edf->free_strings && stl->name) eina_stringshare_del(stl->name);
        if (stl->style) evas_textblock_style_free(stl->style);
        free(stl);
     }
}

