#include "config.h"
#include "Efl.h"

/**
 * @internal
 * @brief Converts a hexadecimal character to its integer value.
 *
 * This function takes a character representing a hexadecimal digit (0-9, A-F, a-f)
 * and converts it into its corresponding integer value (0-15).
 * If the character is not a valid hexadecimal digit, the `ok` flag is set to EINA_FALSE.
 *
 * @param[in] ch The character to convert.
 * @param[out] ok Pointer to a boolean that will be set to EINA_FALSE if ch is not a valid hex digit.
 * @return The integer value of the hexadecimal character, or 0 if invalid.
 */
static int
_hex_string_get(char ch, Eina_Bool *ok)
{
   if ((ch >= '0') && (ch <= '9')) return (ch - '0');
   else if ((ch >= 'A') && (ch <= 'F')) return (ch - 'A' + 10);
   else if ((ch >= 'a') && (ch <= 'f')) return (ch - 'a' + 10);
   *ok = EINA_FALSE;
   return 0;
}

/**
 * @internal
 * Parses a string of one of the formas:
 * 1. "#RRGGBB"
 * 2. "#RRGGBBAA"
 * 3. "#RGB"
 * 4. "#RGBA"
 * To the rgba values.
 *
 * @param[in] str The string to parse - NOT NULL.
 * @param[out] r The Red value - NOT NULL.
 * @param[out] g The Green value - NOT NULL.
 * @param[out] b The Blue value - NOT NULL.
 * @param[out] a The Alpha value - NOT NULL.
 */
static Eina_Bool
_format_color_parse(const char *str, int slen,
                    unsigned char *r, unsigned char *g,
                    unsigned char *b, unsigned char *a)
{
   Eina_Bool v = EINA_TRUE;

   *r = *g = *b = *a = 0;

   if (slen == 7) /* #RRGGBB */
     {
        *r = (_hex_string_get(str[1], &v) << 4) | (_hex_string_get(str[2], &v));
        *g = (_hex_string_get(str[3], &v) << 4) | (_hex_string_get(str[4], &v));
        *b = (_hex_string_get(str[5], &v) << 4) | (_hex_string_get(str[6], &v));
        *a = 0xff;
     }
   else if (slen == 9) /* #RRGGBBAA */
     {
        *r = (_hex_string_get(str[1], &v) << 4) | (_hex_string_get(str[2], &v));
        *g = (_hex_string_get(str[3], &v) << 4) | (_hex_string_get(str[4], &v));
        *b = (_hex_string_get(str[5], &v) << 4) | (_hex_string_get(str[6], &v));
        *a = (_hex_string_get(str[7], &v) << 4) | (_hex_string_get(str[8], &v));
     }
   else if (slen == 4) /* #RGB */
     {
        *r = _hex_string_get(str[1], &v);
        *r = (*r << 4) | *r;
        *g = _hex_string_get(str[2], &v);
        *g = (*g << 4) | *g;
        *b = _hex_string_get(str[3], &v);
        *b = (*b << 4) | *b;
        *a = 0xff;
     }
   else if (slen == 5) /* #RGBA */
     {
        *r = _hex_string_get(str[1], &v);
        *r = (*r << 4) | *r;
        *g = _hex_string_get(str[2], &v);
        *g = (*g << 4) | *g;
        *b = _hex_string_get(str[3], &v);
        *b = (*b << 4) | *b;
        *a = _hex_string_get(str[4], &v);
        *a = (*a << 4) | *a;
     }
   else v = EINA_FALSE;

   /* Apply pre-multiplied alpha */
   *r = (*r * *a) / 255;
   *g = (*g * *a) / 255;
   *b = (*b * *a) / 255;
   return v;
}

EOLIAN static void
_efl_gfx_color_color_code_set(Eo *obj, void *_pd EINA_UNUSED, const char *colorcode)
{
    int len;
    unsigned char r, g, b, a;

    len = (size_t) strlen(colorcode);

    _format_color_parse(colorcode, len, &r, &g, &b, &a);
    efl_gfx_color_set(obj, r, g, b, a);
}

/**
 * @internal
 * @brief Retrieves the color of the object as a hexadecimal string.
 *
 * The returned string is in the format "#RRGGBBAA".
 * The string is allocated using eina_slstr_printf and should be freed by the caller
 * if it's no longer needed and eina_slstr is not being used.
 *
 * @param[in] obj The Eolian object.
 * @param[in] _pd Efl_Gfx_Color_Data (unused).
 * @return A string representing the color in #RRGGBBAA format, or NULL on error.
 */
EOLIAN static const char *
_efl_gfx_color_color_code_get(const Eo *obj, void *_pd EINA_UNUSED)
{
    int r, g, b, a;

    efl_gfx_color_get(obj, &r, &g, &b, &a);
    return eina_slstr_printf("#%02X%02X%02X%02X", r, g, b, a);
}

/**
 * @internal
 * @brief Sets a color for a specific color class and layer using a hexadecimal string.
 *
 * Parses the colorcode string (e.g., "#RRGGBB", "#RRGGBBAA") and applies it
 * to the specified color_class and layer.
 *
 * @param[in] obj The Eolian object.
 * @param[in] _pd Efl_Gfx_Color_Data (unused).
 * @param[in] color_class The name of the color class to modify.
 * @param[in] layer The layer within the color class to modify.
 * @param[in] colorcode The hexadecimal color string.
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., invalid colorcode format).
 */
EOLIAN static Eina_Bool
_efl_gfx_color_class_color_class_code_set(Eo *obj, void *_pd EINA_UNUSED, const char *color_class,
                                    Efl_Gfx_Color_Class_Layer layer, const char *colorcode)
{
   int len;
   unsigned char r, g, b, a;

   len = (size_t) strlen(colorcode);

   _format_color_parse(colorcode, len, &r, &g, &b, &a);
   return efl_gfx_color_class_set(obj, color_class, layer, r, g, b, a);
}

/**
 * @internal
 * @brief Retrieves the color of a specific color class and layer as a hexadecimal string.
 *
 * The returned string is in the format "#RRGGBBAA".
 * The string is allocated using eina_slstr_printf and should be freed by the caller
 * if it's no longer needed and eina_slstr is not being used.
 *
 * @param[in] obj The Eolian object (unused).
 * @param[in] pd Efl_Gfx_Color_Data (unused).
 * @param[in] color_class The name of the color class.
 * @param[in] layer The layer within the color class.
 * @return A string representing the color in #RRGGBBAA format if found, otherwise NULL.
 */
EOLIAN static const char *
_efl_gfx_color_class_color_class_code_get(const Eo *obj EINA_UNUSED, void *pd EINA_UNUSED,
                                    const char *color_class, Efl_Gfx_Color_Class_Layer layer)
{
   int r, g, b, a;

   if (efl_gfx_color_class_get(obj, color_class, layer, &r, &g, &b, &a))
     return eina_slstr_printf("#%02X%02X%02X%02X", r, g, b, a);
   return NULL;
}

#include "interfaces/efl_gfx_color.eo.c"
#include "interfaces/efl_gfx_color_class.eo.c"
