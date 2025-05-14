#define EFL_UI_FORMAT_PROTECTED 1

#include "config.h"
#include "Efl_Ui.h"
#include "elm_priv.h" /* To be able to use elm_widget_is_legacy() */

/**
 * @brief Internal enumeration for the type of data expected by a format string.
 *
 * This is determined by parsing the format string provided by the user.
 */
typedef enum _Format_Type
{
   /* When a format string is used, it is parsed to find out the expected data type */
   FORMAT_TYPE_INVALID, /**< The format string is invalid or not understood. */
   FORMAT_TYPE_DOUBLE,  /**< The format expects a double-precision floating-point value. */
   FORMAT_TYPE_INT,     /**< The format expects an integer value. */
   FORMAT_TYPE_TM,      /**< The format expects a `struct tm` for time/date. */
   FORMAT_TYPE_STRING,  /**< The format expects a string value. */
   FORMAT_TYPE_STATIC   /**< The format string contains no placeholders and is used as-is. */
} Format_Type;

/**
 * @brief Private data structure for the Efl_Ui_Format mixin.
 *
 * This structure holds all the formatting-related properties for a widget.
 */
typedef struct
{
   Efl_Ui_Format_Func  format_func;         /**< User-supplied formatting function. @see efl_ui_format_func_set() */
   void                *format_func_data;   /**< User data for the format_func. */
   Eina_Free_Cb        format_func_free;    /**< Callback to free format_func_data. */

   Eina_Inarray        *format_values;      /**< Sorted array of Efl_Ui_Format_Value, used for mapping discrete values to text. */

   const char          *format_string;      /**< User-supplied formatting string (e.g., "%1.2f"). @see efl_ui_format_string_set() */
   Format_Type         format_string_type;  /**< The type of data expected by format_string, determined by _format_string_check(). */
} Efl_Ui_Format_Data;

/**
 * @brief Checks if a character is a digit or a decimal point.
 *
 * @param x The character to check.
 * @return @c EINA_TRUE if the character is a digit or '.', @c EINA_FALSE otherwise.
 */
static Eina_Bool
_is_valid_digit(char x)
{
   return ((x >= '0' && x <= '9') || (x == '.')) ? EINA_TRUE : EINA_FALSE;
}

/**
 * @brief Parses a format string to determine its expected data type.
 *
 * This function analyzes the format string `fmt` to find the type of its
 * placeholder (e.g., %d for int, %f for double). It only supports a single
 * placeholder in the string. If no placeholder is found, it is considered
 * a static string.
 *
 * @param fmt The format string to check (e.g., "Value: %.2f").
 * @param type The user-specified type of format string, which can override
 *        the check for time formats.
 * @return The detected Format_Type. Returns FORMAT_TYPE_INVALID if the format
 *         string is not supported (e.g., multiple placeholders).
 */
static Format_Type
_format_string_check(const char *fmt, Efl_Ui_Format_String_Type type)
{
   const char *itr;
   Eina_Bool found = EINA_FALSE;
   Format_Type ret_type = FORMAT_TYPE_STATIC;

   if (type == EFL_UI_FORMAT_STRING_TYPE_TIME) return FORMAT_TYPE_TM;

   for (itr = fmt; *itr; itr++)
     {
        if (itr[0] != '%') continue;
        if (itr[1] == '%')
          {
             itr++;
             if (ret_type == FORMAT_TYPE_STATIC)
               ret_type = FORMAT_TYPE_STRING;
             continue;
          }

        if (!found)
          {
             found = EINA_TRUE;
             for (itr++; *itr; itr++)
               {
                  // FIXME: This does not properly support int64 or unsigned.
                  if ((*itr == 'd') || (*itr == 'u') || (*itr == 'i') ||
                      (*itr == 'o') || (*itr == 'x') || (*itr == 'X'))
                    {
                       ret_type = FORMAT_TYPE_INT;
                       break;
                    }
                  else if ((*itr == 'f') || (*itr == 'F'))
                    {
                       ret_type = FORMAT_TYPE_DOUBLE;
                       break;
                    }
                  else if (*itr == 's')
                    {
                       ret_type = FORMAT_TYPE_STRING;
                       break;
                    }
                  else if (_is_valid_digit(*itr))
                    {
                       continue;
                    }
                  else
                    {
                       ERR("Format string '%s' has unknown format element '%c' in format. It must have one format element of type 's', 'f', 'F', 'd', 'u', 'i', 'o', 'x' or 'X'", fmt, *itr);
                       found = EINA_FALSE;
                       break;
                    }
               }
             if (!(*itr)) break;
          }
        else
          {
             ret_type = FORMAT_TYPE_INVALID;
             break;
          }
     }

   if (ret_type == FORMAT_TYPE_INVALID)
     {
        ERR("Format string '%s' is invalid. It must have one and only one format element of type 's', 'f', 'F', 'd', 'u', 'i', 'o', 'x' or 'X'", fmt);
     }
   return ret_type;
}

/**
 * @brief Formats a value using the format string and appends it to a buffer.
 *
 * This function takes a generic Eina_Value, converts it to the type expected
 * by the format string (as determined by `_format_string_check`), and then
 * performs the formatting.
 *
 * @param pd The private data for the format mixin.
 * @param str The string buffer to append the formatted string to.
 * @param value The value to format.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
static Eina_Bool
_do_format_string(Efl_Ui_Format_Data *pd, Eina_Strbuf *str, const Eina_Value value)
{
   switch (pd->format_string_type)
     {
      case FORMAT_TYPE_DOUBLE:
      {
        double v = 0.0;
        if (!eina_value_double_convert(&value, &v))
           ERR("Format conversion failed");
        eina_strbuf_append_printf(str, pd->format_string, v);
        break;
      }
      case FORMAT_TYPE_INT:
      {
        int v = 0;
        if (!eina_value_int_convert(&value, &v))
           ERR("Format conversion failed");
        eina_strbuf_append_printf(str, pd->format_string, v);
        break;
      }
      case FORMAT_TYPE_STRING:
      {
        char *v = eina_value_to_string(&value);
        eina_strbuf_append_printf(str, pd->format_string, v);
        free(v);
        break;
      }
      case FORMAT_TYPE_STATIC:
      {
        eina_strbuf_append(str, pd->format_string);
        break;
      }
      case FORMAT_TYPE_TM:
      {
        struct tm v;
        char *buf = NULL;
        eina_value_get(&value, &v);
        buf = eina_strftime(pd->format_string, &v);
        if (buf)
          {
             eina_strbuf_append(str, buf);
             free(buf);
          }
        break;
      }
      default:
        return EINA_FALSE;
     }
   return EINA_TRUE;
}

/**
 * @brief Default format function used for legacy widgets.
 *
 * When a format string is set on a legacy widget, this function is set as the
 * default formatting callback. It attempts to use the format string, but falls
 * back to a direct string conversion of the value if formatting fails. This
 * preserves the behavior of older Elm widgets.
 *
 * @param data The private data for the format mixin, passed as user data.
 * @param str The string buffer to append the formatted string to.
 * @param value The value to format.
 * @return Always returns @c EINA_TRUE.
 */
static Eina_Bool
_legacy_default_format_func(void *data, Eina_Strbuf *str, const Eina_Value value)
{
   if (!_do_format_string(data, str, value))
      {
        /* Fallback to just printing the value if format string fails (legacy behavior) */
        char *v = eina_value_to_string(&value);
        eina_strbuf_append(str, v);
        free(v);
      }
   return EINA_TRUE;
}

/**
 * @brief Implements efl_ui_format_func_set.
 *
 * Sets a custom function to format the value into a string. The previously
 * set function and data are freed, if any.
 *
 * Example of a format function:
 * @code
 * static Eina_Bool
 * _my_format_cb(void *data, Eina_Strbuf *str, const Eina_Value *value)
 * {
 *    int v = 0;
 *    eina_value_int_get(value, &v);
 *    eina_strbuf_append_printf(str, "Value is %d", v);
 *    return EINA_TRUE;
 * }
 *
 * efl_ui_format_func_set(widget, NULL, _my_format_cb, NULL);
 * @endcode
 */
EOLIAN static void
_efl_ui_format_format_func_set(Eo *obj, Efl_Ui_Format_Data *pd, void *func_data, Efl_Ui_Format_Func func, Eina_Free_Cb func_free_cb)
{
   if (pd->format_func_free)
     pd->format_func_free(pd->format_func_data);
   pd->format_func = func;
   pd->format_func_data = func_data;
   pd->format_func_free = func_free_cb;

   if (efl_alive_get(obj))
     efl_ui_format_apply_formatted_value(obj);
}

/**
 * @brief Implements efl_ui_format_func_get.
 *
 * Retrieves the custom formatting function.
 */
EOLIAN static Efl_Ui_Format_Func
_efl_ui_format_format_func_get(const Eo *obj EINA_UNUSED, Efl_Ui_Format_Data *pd)
{
   return pd->format_func;
}

/**
 * @brief Comparison function for sorting Efl_Ui_Format_Value structs by value.
 *
 * Used with eina_inarray_insert_sorted() to keep the format_values array sorted.
 *
 * @param val1 The first value to compare.
 * @param val2 The second value to compare.
 * @return An integer less than, equal to, or greater than zero if val1->value
 *         is found, respectively, to be less than, to match, or be greater
 *         than val2->value.
 */
static int
_value_compare(const Efl_Ui_Format_Value *val1, const Efl_Ui_Format_Value *val2)
{
   return val1->value - val2->value;
}

/**
 * @brief Implements efl_ui_format_values_set.
 *
 * Sets a list of value-to-text mappings. This is useful when the widget needs
 * to display specific strings for a set of discrete integer values. For example,
 * mapping 0 to "off" and 1 to "on".
 *
 * The provided accessor must yield `Efl_Ui_Format_Value` structs.
 * The internal list is kept sorted by the `value` field.
 *
 * Example of a values array:
 * @code
 * const Efl_Ui_Format_Value values[] = {
 *   { 0, "None" },
 *   { 1, "Some" },
 *   { 10, "Many" },
 *   { -1, "Error" }
 * };
 * Eina_Array *array = eina_array_new(EINA_C_ARRAY_LENGTH(values));
 * for (size_t i = 0; i < EINA_C_ARRAY_LENGTH(values); i++)
 *   eina_array_push(array, &values[i]);
 *
 * efl_ui_format_values_set(widget, eina_array_accessor_new(array));
 * eina_array_free(array);
 * @endcode
 */
EOLIAN static void
_efl_ui_format_format_values_set(Eo *obj, Efl_Ui_Format_Data *pd, Eina_Accessor *values)
{
   Efl_Ui_Format_Value *v;
   int i;
   if (pd->format_values)
     {
        Efl_Ui_Format_Value *vptr;
        /* Delete previous values array */
        EINA_INARRAY_FOREACH(pd->format_values, vptr)
          {
             eina_stringshare_del(vptr->text);
          }
        eina_inarray_free(pd->format_values);
        pd->format_values = NULL;
     }
   if (values == NULL)
     {
        if (efl_alive_get(obj))
          efl_ui_format_apply_formatted_value(obj);
        return;
     }

   /* Copy the values to our internal array */
   pd->format_values = eina_inarray_new(sizeof(Efl_Ui_Format_Value), 4);
   EINA_ACCESSOR_FOREACH(values, i, v)
     {
        Efl_Ui_Format_Value vcopy = { v->value, eina_stringshare_add(v->text) };
        eina_inarray_insert_sorted(pd->format_values, &vcopy, (Eina_Compare_Cb)_value_compare);
     }
   eina_accessor_free(values);

   if (efl_alive_get(obj))
     efl_ui_format_apply_formatted_value(obj);
}

/**
 * @brief Implements efl_ui_format_values_get.
 *
 * Retrieves the list of value-to-text mappings. The returned accessor will
 * yield pointers to `Efl_Ui_Format_Value` structs.
 *
 * @return An accessor over the list of format values, or @c NULL if none is set.
 *         The caller is responsible for freeing the accessor.
 */
EOLIAN static Eina_Accessor *
_efl_ui_format_format_values_get(const Eo *obj EINA_UNUSED, Efl_Ui_Format_Data *pd)
{
   if (!pd->format_values) return NULL;
   return eina_inarray_accessor_new(pd->format_values);
}

/**
 * @brief Implements efl_ui_format_string_set.
 *
 * Sets the format string used for formatting the value. The string must contain
 * at most one format specifier of type d, u, i, o, x, X, f, F, or s.
 *
 * For legacy widgets, setting a format string also installs a default format
 * function to maintain compatibility.
 *
 * @param string The format string (e.g., "Value: %d" or "%.2f units").
 * @param type The type hint for the format string, particularly for time formats.
 */
EOLIAN static void
_efl_ui_format_format_string_set(Eo *obj EINA_UNUSED, Efl_Ui_Format_Data *sd, const char *string, Efl_Ui_Format_String_Type type)
{
   eina_stringshare_replace(&sd->format_string, string);
   if (string)
     sd->format_string_type = _format_string_check(sd->format_string, type);
   else
     sd->format_string_type = FORMAT_TYPE_INVALID;

   /* In legacy, setting the format string installs a default format func.
      Some widgets then override the format_func_set method so we keep that behavior. */
   if (elm_widget_is_legacy(obj))
     efl_ui_format_func_set(obj, sd, _legacy_default_format_func, NULL);

   if (efl_alive_get(obj))
     efl_ui_format_apply_formatted_value(obj);
}

/**
 * @brief Implements efl_ui_format_string_get.
 *
 * Retrieves the format string and its type.
 */
EOLIAN static void
_efl_ui_format_format_string_get(const Eo *obj EINA_UNUSED, Efl_Ui_Format_Data *sd, const char **string, Efl_Ui_Format_String_Type *type)
{
   if (string) *string = sd->format_string;
   if (type) *type = sd->format_string_type == FORMAT_TYPE_TM ?
     EFL_UI_FORMAT_STRING_TYPE_TIME : EFL_UI_FORMAT_STRING_TYPE_SIMPLE;
}

/**
 * @brief Implements efl_ui_format_formatted_value_get.
 *
 * Generates a formatted string for a given value and stores it in `str`.
 * The formatting follows a specific precedence:
 *
 * 1. If `format_values` are set, it looks for a direct mapping from the value
 *    (converted to an integer) to a text string. If a match is found, that
 *    text is used.
 *
 * 2. If no mapping is found or `format_values` is not set, it tries to use the
 *    `format_func` if one is provided. If the function returns `EINA_TRUE`,
 *    its result is used.
 *
 * 3. If the `format_func` is not set or returns `EINA_FALSE`, it attempts to
 *    use the `format_string`.
 *
 * 4. If all of the above fail or are not set, it falls back to a simple
 *    conversion of the `value` to a string.
 *
 * @param str The string buffer to store the result. The buffer is reset
 *            before being used.
 * @param value The value to be formatted.
 */
EOLIAN static void
_efl_ui_format_formatted_value_get(Eo *obj EINA_UNUSED, Efl_Ui_Format_Data *pd, Eina_Strbuf *str, const Eina_Value value)
{
   char *v;
   eina_strbuf_reset(str);
   if (pd->format_values)
     {
        /* Search in the format_values array if we have one */
        Efl_Ui_Format_Value val = { 0 };
        int ndx;
        if (!eina_value_int_convert(&value, &val.value))
           ERR("Format conversion failed");
        ndx = eina_inarray_search_sorted(pd->format_values, &val, (Eina_Compare_Cb)_value_compare);
        if (ndx > -1) {
          Efl_Ui_Format_Value *entry = eina_inarray_nth(pd->format_values, ndx);
          eina_strbuf_append(str, entry->text);
          return;
        }
     }
   if (pd->format_func)
     {
        /* If we have a formatting function, try to use it */
        if (pd->format_func(pd->format_func_data, str, value))
          return;
     }
   if (pd->format_string)
     {
        /* If we have a formatting string, use it */
        if (_do_format_string(pd, str, value))
          return;
     }

   /* Fallback to just printing the value if everything else fails */
   v = eina_value_to_string(&value);
   eina_strbuf_append(str, v);
   free(v);
}

/**
 * @brief Implements efl_ui_format_decimal_places_get.
 *
 * Determines the number of decimal places specified in a format string for
 * floating-point numbers. For example, for "%.3f", it would return 3.
 *
 * This function is not a full-featured printf parser. It finds the first
 * format specifier (`%` not followed by `%`), finds the `.` within it,
 * and reads the number before the `f`.
 *
 * @return The number of decimal places, or 0 if not specified or not applicable.
 */
EOLIAN static int
_efl_ui_format_decimal_places_get(Eo *obj EINA_UNUSED, Efl_Ui_Format_Data *pd)
{
   char result[16] = "0";
   const char *start;

   /* This method can only be called if a format_string has been supplied */
   if (!pd->format_string) return 0;

   start = strchr(pd->format_string, '%');
   while (start)
     {
        if (start[1] != '%')
          {
             start = strchr(start, '.');
             if (start)
                start++;
             break;
          }
        else
          start = strchr(start + 2, '%');
     }

   if (start)
     {
        const char *p = strchr(start, 'f');

        if ((p) && ((p - start) < 15))
          sscanf(start, "%[^f]", result);
     }

   return atoi(result);
}

/**
 * @brief Implements efl_object_destructor.
 *
 * Cleans up resources allocated by the format mixin, such as the format
 * string, format values, and custom format function data.
 */
EOLIAN static void
_efl_ui_format_efl_object_destructor(Eo *obj, Efl_Ui_Format_Data *pd EINA_UNUSED)
{
   if (pd->format_func_free)
     {
        efl_ui_format_func_set(obj, NULL, NULL, NULL);
     }
   if (pd->format_values)
     {
        efl_ui_format_values_set(obj, NULL);
     }
   if (pd->format_string_type)
     {
        efl_ui_format_string_set(obj, NULL, 0);
     }
   efl_destructor(efl_super(obj, EFL_UI_FORMAT_MIXIN));
}

#include "efl_ui_format.eo.c"

