#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#ifdef _WIN32
# include <evil_private.h> /* nl_langinfo */
#endif

#define EFL_ACCESS_OBJECT_PROTECTED
#define EFL_UI_L10N_PROTECTED

#include <Elementary.h>
#include "elm_priv.h"
#include <efl_ui_clock.h>
#include <efl_ui_clock_private.h>

#define MY_CLASS EFL_UI_CLOCK_CLASS

#define MY_CLASS_NAME "Efl.Ui.Clock"

#ifdef HAVE_LOCALE_H
# include <locale.h>
#endif

#ifdef HAVE_LANGINFO_H
# include <langinfo.h>
#endif

#define MAX_SEPARATOR_LEN              6
#define MIN_DAYS_IN_MONTH              28
#define BUFFER_SIZE                    1024
#define CLOCK_FIELD_COUNT       8

/* interface between EDC & C code (field & signal names). values 0 to
 * EFL_UI_CLOCK_TYPE_COUNT are in the valid range, and must get in the
 * place of "%d".
 */
#define EDC_PART_FIELD_STR             "efl.field%d"
#define EDC_PART_SEPARATOR_STR         "efl.separator%d"
#define EDC_PART_FIELD_ENABLE_SIG_STR  "field%d,enable"
#define EDC_PART_FIELD_DISABLE_SIG_STR "field%d,disable"

/* struct tm does not define the fields in the order year, month,
 * date, hour, minute. values are reassigned to an array for easy
 * handling.
 */
#define CLOCK_TM_ARRAY(intptr, tmptr) \
  int *intptr[] = {                      \
     &(tmptr)->tm_year,                  \
     &(tmptr)->tm_mon,                   \
     &(tmptr)->tm_mday,                  \
     &(tmptr)->tm_hour,                  \
     &(tmptr)->tm_min,                  \
     &(tmptr)->tm_sec,                  \
     &(tmptr)->tm_wday,                  \
     &(tmptr)->tm_hour}

/**
 * @brief Default mapping for clock fields.
 * This array defines properties for each clock field type, including:
 * - fmt_char: Accepted format characters (e.g., "Yy" for year).
 * - def_min: Default minimum value.
 * - def_max: Default maximum value.
 * - ignore_sep: Characters to ignore as separators for this field.
 *
 * Example for EFL_UI_CLOCK_TYPE_YEAR:
 *   fmt_char = "Yy" (accepts %Y or %y)
 *   def_min = -1 (dynamically set from config _elm_config->year_min)
 *   def_max = -1 (dynamically set from config _elm_config->year_max)
 *   ignore_sep = "" (no specific separators ignored beyond global ones)
 *
 * Example for EFL_UI_CLOCK_TYPE_MONTH:
 *   fmt_char = "mbBh" (accepts %m, %b, %B, or %h)
 *   def_min = 0 (January)
 *   def_max = 11 (December)
 *   ignore_sep = ""
 */
static Format_Map mapping[EFL_UI_CLOCK_TYPE_COUNT] = {
   [EFL_UI_CLOCK_TYPE_YEAR] = { "Yy", -1, -1, "" },
   [EFL_UI_CLOCK_TYPE_MONTH] = { "mbBh", 0, 11, "" },
   [EFL_UI_CLOCK_TYPE_DATE] = { "de", 1, 31, "" },
   [EFL_UI_CLOCK_TYPE_HOUR] = { "IHkl", 0, 23, "" },
   [EFL_UI_CLOCK_TYPE_MINUTE] = { "M", 0, 59, "" },
   [EFL_UI_CLOCK_TYPE_SECOND] = { "S", 0, 59, "" },
   [EFL_UI_CLOCK_TYPE_DAY] = { "Aa", 0, 6, "" },
   [EFL_UI_CLOCK_TYPE_AMPM] = { "pP", 0, 1, "" }
};

/// @brief Format characters that expand to multiple fields (e.g., %c for date and time).
static const char *multifield_formats = "cxXrRTDF";
/// @brief Characters to be globally ignored as separators between fields.
static const char *ignore_separators = "()";
/// @brief Format extensions (modifiers) to be ignored during parsing (e.g., %E_Y, %O_m).
static const char *ignore_extensions = "E0_-O^#";

/// @brief Signal emitted when the clock value changes.
static const char SIG_CHANGED[] = "changed";
static const Evas_Smart_Cb_Description _smart_callbacks[] = {
   {SIG_CHANGED, ""},
   {SIG_WIDGET_LANG_CHANGED, ""}, /**< handled by elm_widget */
   {SIG_WIDGET_ACCESS_CHANGED, ""}, /**< handled by elm_widget */
   {SIG_LAYOUT_FOCUSED, ""}, /**< handled by elm_layout */
   {SIG_LAYOUT_UNFOCUSED, ""}, /**< handled by elm_layout */
   {NULL, NULL}
};

/**
 * @brief Generates a part name for an Edje object, trying different prefixes.
 *
 * This function attempts to create a part name using a given template and index.
 * It first tries the provided template (e.g., "efl.field%d"). If that part
 * doesn't exist in the Edje object, it tries an "elm." prefix (e.g., "elm.field%d").
 * If that also fails, it tries the template without any namespace prefix
 * (e.g., "field%d") for backward compatibility.
 *
 * @param buffer The character buffer to store the resulting part name.
 * @param buffer_size The size of the buffer.
 * @param obj The Edje object to check for part existence.
 * @param template The format string template for the part name (e.g., "efl.field%d").
 * @param n The integer index to use in the template.
 */
static void _part_name_snprintf(char *buffer, int buffer_size,
   const Evas_Object *obj, const char *template, int n)
{
   snprintf(buffer, buffer_size, template, n);
   if (edje_object_part_exists (obj, buffer)) return;
   // Try 'elm' prefix instead of 'efl'
   buffer[0] = 'e';
   buffer[1] = 'l';
   buffer[2] = 'm';
   if (edje_object_part_exists (obj, buffer)) return;
   // Skip the namespace prefix "elm." which was not present
   // in previous versions
   snprintf(buffer, buffer_size, template + 4, n);
}

/**
 * @brief Callback function invoked when the AM/PM field is clicked.
 *
 * This function toggles the hour between AM and PM. If the current hour
 * is >= 12, it subtracts 12; otherwise, it adds 12. The clock's time
 * is then updated with the new hour.
 *
 * @param data The Evas_Object (clock widget) associated with this callback.
 * @param event The Efl_Event details (unused).
 */
static void
_ampm_clicked_cb(void *data, const Efl_Event *event EINA_UNUSED)
{
   struct tm curr_time;

   curr_time = efl_ui_clock_time_get(data);
   if (curr_time.tm_hour >= 12) curr_time.tm_hour -= 12;
   else curr_time.tm_hour += 12;
   efl_ui_clock_time_set(data, curr_time);
}

/**
 * @brief Sets accessibility information for a clock field.
 *
 * Assigns a descriptive string to the accessibility type based on the
 * clock field type (e.g., "datetime field, year" for EFL_UI_CLOCK_TYPE_YEAR).
 *
 * @param obj The Evas_Object representing the clock field.
 * @param field_type The type of the clock field.
 */
static void
_access_set(Evas_Object *obj, Efl_Ui_Clock_Type field_type)
{
   const char* type = NULL;

   switch (field_type)
     {
      case EFL_UI_CLOCK_TYPE_YEAR:
         type = "datetime field, year";
         break;

      case EFL_UI_CLOCK_TYPE_MONTH:
         type = "datetime field, month";
         break;

      case EFL_UI_CLOCK_TYPE_DATE:
         type = "datetime field, date";
         break;

      case EFL_UI_CLOCK_TYPE_HOUR:
         type = "datetime field, hour";
         break;

      case EFL_UI_CLOCK_TYPE_MINUTE:
         type = "datetime field, minute";
         break;

      case EFL_UI_CLOCK_TYPE_AMPM:
         type = "datetime field, AM PM";
         break;

      default:
         break;
     }

   _elm_access_text_set
     (_elm_access_info_get(obj), ELM_ACCESS_TYPE, type);
   _elm_access_callback_set
     (_elm_access_info_get(obj), ELM_ACCESS_STATE, NULL, NULL);
}

/**
 * @brief Retrieves the format string for a specific clock field.
 *
 * @param obj The clock widget Evas_Object.
 * @param field_type The type of the clock field for which to get the format.
 * @return The format string (e.g., "%Y", "%m") for the field, or NULL if invalid.
 */
static const char *
_field_format_get(Evas_Object *obj,
                  Efl_Ui_Clock_Type field_type)
{
   Clock_Field *field;

   if (field_type > EFL_UI_CLOCK_TYPE_AMPM) return NULL;

   EFL_UI_CLOCK_DATA_GET(obj, sd);

   field = sd->field_list + field_type;

   return field->fmt;
}

/**
 * @brief Updates the displayed text of a clock field object based on the current time.
 *
 * Formats the current time of the clock widget according to the field's
 * specific format string (e.g., "%Y" for year) and sets it as the text
 * for the given field object (item_obj). Handles AM/PM text explicitly
 * if strftime doesn't populate it for %p/%P.
 *
 * @param obj The main clock widget (Eo *).
 * @param item_obj The Evas_Object of the specific field to update (e.g., the year textbox).
 */
static void
field_value_display(Eo *obj, Evas_Object *item_obj)
{
   Efl_Ui_Clock_Type  field_type;
   struct tm tim;
   char buf[BUFFER_SIZE];
   const char *fmt;

   tim = efl_ui_clock_time_get(obj);
   field_type = (Efl_Ui_Clock_Type )evas_object_data_get(item_obj, "_field_type");
   fmt = _field_format_get(obj, field_type);
   buf[0] = 0;
   strftime(buf, sizeof(buf), fmt, &tim);
   if ((!buf[0]) && ((!strcmp(fmt, "%p")) || (!strcmp(fmt, "%P"))))
     {
        // yes BUFFER_SIZE is more than 2 bytes!
        if (tim.tm_hour < 12) strcpy(buf, "AM");
        else strcpy(buf, "PM");
     }
   efl_text_set(item_obj, buf);
}

/**
 * @brief Creates a new Evas_Object for a clock field.
 *
 * Depending on the field_type, this function creates either an
 * Efl_Ui_Button (for AM/PM) or an Efl_Ui_Textbox (for other fields like
 * year, month, day, hour, minute). It also sets up necessary properties
 * and accessibility information for the created field object.
 *
 * @param obj The parent clock widget (Eo *).
 * @param field_type The type of clock field to create.
 * @return The newly created Evas_Object for the field.
 */
static Evas_Object *
field_create(Eo *obj, Efl_Ui_Clock_Type  field_type)
{
   Evas_Object *field_obj;

   if (field_type == EFL_UI_CLOCK_TYPE_AMPM)
     {
        field_obj = efl_add(EFL_UI_BUTTON_CLASS, obj,
          efl_event_callback_add(efl_added, EFL_INPUT_EVENT_CLICKED, _ampm_clicked_cb, obj));
     }
   else
     {
        field_obj = efl_add(EFL_UI_TEXTBOX_CLASS,obj,
          efl_text_multiline_set(efl_added, EINA_FALSE),
          efl_text_interactive_editable_set(efl_added, EINA_FALSE),
          efl_input_text_input_panel_autoshow_set(efl_added, EINA_FALSE),
          efl_ui_textbox_context_menu_enabled_set(efl_added, EINA_FALSE));
     }
   evas_object_data_set(field_obj, "_field_type", (void *)field_type);

   // ACCESS
   _access_set(field_obj, field_type);

   return field_obj;
}

/**
 * @brief Updates the display of all visible and existing fields in the clock.
 *
 * Iterates through all clock fields and calls field_value_display()
 * for each field that has a format defined (fmt_exist) and is currently visible.
 *
 * @param obj The clock widget Evas_Object.
 */
static void
_field_list_display(Evas_Object *obj)
{
   Clock_Field *field;
   unsigned int idx = 0;

   EFL_UI_CLOCK_DATA_GET(obj, sd);

   for (idx = 0; idx < EFL_UI_CLOCK_TYPE_COUNT; idx++)
     {
        field = sd->field_list + idx;
        if (field->fmt_exist && field->visible)
          field_value_display(obj, field->item_obj);
     }
}

// FIXME: provide nl_langinfo on Windows if possible
/**
 * @brief Retrieves the locale-specific or standard expanded format string
 *        for a given multi-field format character.
 *
 * For example, 'c' might expand to "%a %b %e %H:%M:%S %Y" (locale-dependent date and time).
 * 'R' expands to "%H:%M".
 *
 * @param ch The multi-field format character (e.g., 'c', 'x', 'X', 'r', 'R', 'T', 'D', 'F').
 * @return A pointer to the static string representing the expanded format.
 *         Returns an empty string if the character is not a recognized multi-field format.
 */
static char *
_expanded_fmt_str_get(char ch)
{
   char *exp_fmt = "";
   switch (ch)
     {
      case 'c':
#if defined(HAVE_LANGINFO_H) || defined (_WIN32)
        exp_fmt = nl_langinfo(D_T_FMT); // Locale's appropriate date and time representation
#else
        exp_fmt = "";
#endif
        break;

      case 'x':
#if defined(HAVE_LANGINFO_H) || defined (_WIN32)
        exp_fmt = nl_langinfo(D_FMT); // Locale's appropriate date representation
#else
        exp_fmt = "";
#endif
        break;

      case 'X':
#if defined(HAVE_LANGINFO_H) || defined (_WIN32)
        exp_fmt = nl_langinfo(T_FMT); // Locale's appropriate time representation
#else
        exp_fmt = "";
#endif
        break;

      case 'r':
#if defined(HAVE_LANGINFO_H) || defined (_WIN32)
        exp_fmt = nl_langinfo(T_FMT_AMPM); // Locale's 12-hour clock time with AM/PM
#else
        exp_fmt = "";
#endif
        break;

      case 'R':
        exp_fmt = "%H:%M"; // Equivalent to %H:%M
        break;

      case 'T':
        exp_fmt = "%H:%M:%S"; // Equivalent to %H:%M:%S
        break;

      case 'D':
        exp_fmt = "%m/%d/%y"; // Equivalent to %m/%d/%y
        break;

      case 'F':
        exp_fmt = "%Y-%m-%d"; // Equivalent to %Y-%m-%d (ISO 8601 date format)
        break;

      default:
        exp_fmt = "";
        break;
     }

   return exp_fmt;
}

/**
 * @brief Expands any multi-field format specifiers within a date/time format string.
 *
 * This function iteratively replaces multi-field format characters (like %c, %x, %X, %r, %R, %T, %D, %F)
 * in the input `dt_fmt` string with their expanded forms (e.g., %c might become
 * the locale-specific date and time format). The expansion is done in place.
 * The process repeats until no more multi-field formats can be expanded.
 *
 * @param dt_fmt The date/time format string to expand. This string is modified directly.
 *               It should be large enough to hold the expanded format, up to
 *               EFL_UI_CLOCK_MAX_FORMAT_LEN.
 */
static void
_expand_format(char *dt_fmt)
{
   char *ptr, *expanded_fmt, ch;
   unsigned int idx, len = 0;
   char buf[EFL_UI_CLOCK_MAX_FORMAT_LEN] = {0, };
   Eina_Bool fmt_char, fmt_expanded;

   do {
     idx = 0;
     fmt_char = EINA_FALSE;
     fmt_expanded = EINA_FALSE;
     ptr = dt_fmt;
     while ((ch = *ptr))
       {
          if ((fmt_char) && (strchr(multifield_formats, ch)))
            {
               /* replace the multi-field format characters with
                * corresponding expanded format */
               expanded_fmt = _expanded_fmt_str_get(ch);
               len = strlen(expanded_fmt);
               if (len > 0) fmt_expanded = EINA_TRUE;
               buf[--idx] = 0;
               strncat(buf, expanded_fmt, len);
               idx += len;
            }
          else buf[idx++] = ch;

          if (ch == '%') fmt_char = EINA_TRUE;
          else fmt_char = EINA_FALSE;

          ptr++;
       }

     buf[idx] = 0;
     strncpy(dt_fmt, buf, EFL_UI_CLOCK_MAX_FORMAT_LEN);
   } while (fmt_expanded);
}

/**
 * @brief Arranges the clock fields in the layout based on their visibility and format.
 *
 * This function iterates through all clock fields. For each field, it determines
 * the Edje part name (e.g., "efl.field0", "efl.field1", etc.) where the field's
 * Evas_Object (textbox or button) should be placed.
 * If a field is visible and has a valid format, its Evas_Object is set as the
 * content of the corresponding Edje part. Otherwise, any existing content in that
 * part is unset and hidden.
 * After arranging, it triggers a recalculation of the group and updates the
 * display of all fields.
 *
 * @param obj The clock widget Evas_Object.
 */
static void
_field_list_arrange(Evas_Object *obj)
{
   Clock_Field *field;
   char buf[BUFFER_SIZE];
   int idx;
   Eina_Bool freeze;

   EFL_UI_CLOCK_DATA_GET(obj, sd);

   freeze = sd->freeze_sizing;
   sd->freeze_sizing = EINA_TRUE;
   for (idx = 0; idx < EFL_UI_CLOCK_TYPE_COUNT; idx++)
     {
        field = sd->field_list + idx;
        _part_name_snprintf(buf, sizeof(buf), obj, EDC_PART_FIELD_STR,
                            field->location);

        efl_gfx_entity_visible_set(efl_content_unset(efl_part(obj, buf)), EINA_FALSE);
        if (field->visible && field->fmt_exist)
          efl_content_set(efl_part(obj, buf), field->item_obj);
     }
   sd->freeze_sizing = freeze;

   efl_canvas_group_change(obj);
   _field_list_display(obj);
}

/**
 * @brief Parses the clock format string to identify fields and separators.
 *
 * This function iterates through the provided format string (`fmt_ptr`).
 * It identifies field specifiers (e.g., '%Y', '%m') and the separators
 * between them. For each recognized field, it updates the corresponding
 * `Clock_Field` structure in `sd->field_list` with its format character,
 * marks it as existing (`fmt_exist`), assigns it a location (order of appearance),
 * and stores the separator string that follows it.
 *
 * Format parsing state machine:
 * - `fmt_parsing`: True when a '%' has been encountered and the next char is expected to be a field specifier.
 * - `sep_parsing`: True when characters are being collected as part of a separator.
 * - `sep_lookup`: True after a field has been identified, indicating that subsequent characters should be treated as separators.
 *
 * Ignored elements:
 * - `ignore_extensions`: Characters like 'E', '0', '_', etc., when they follow a '%' and modify a specifier (e.g., %Ey).
 * - `ignore_separators`: Global characters like '(' and ')' that are never treated as separators.
 * - `mapping[idx].ignore_sep`: Field-specific characters to ignore as separators.
 * - AM/PM field does not collect separators after it.
 *
 * @param obj The clock widget Evas_Object.
 * @param fmt_ptr Pointer to the format string to be parsed.
 * @return The number of valid, unique fields parsed from the format string.
 */
static unsigned int
_parse_format(Evas_Object *obj,
              char *fmt_ptr)
{
   Eina_Bool fmt_parsing = EINA_FALSE, sep_parsing = EINA_FALSE,
             sep_lookup = EINA_FALSE;
   unsigned int len = 0, idx = 0, location = 0;
   char separator[MAX_SEPARATOR_LEN];
   Clock_Field *field = NULL;
   char cur;

   EFL_UI_CLOCK_DATA_GET(obj, sd);

   while ((cur = *fmt_ptr))
     {
        if (fmt_parsing) // Expecting a field specifier character after '%'
          {
             // Check if current char is an ignored extension (e.g., 'E' in "%Ey")
             if (strchr(ignore_extensions, cur))
               {
                  fmt_ptr++; // Skip extension
                  continue;
               }
             fmt_parsing = EINA_FALSE; // Done with '%' and potential extension
             // Iterate through known field types to find a match for 'cur'
             for (idx = 0; idx < EFL_UI_CLOCK_TYPE_COUNT; idx++)
               {
                  // Check if 'cur' is a valid format character for this field type
                  if (strchr(mapping[idx].fmt_char, cur))
                    {
                       field = sd->field_list + idx;
                       // If field already parsed (has a location), ignore re-definitions
                       if (field->location != -1) break;

                       field->fmt[1] = cur; // Store the format char (e.g., 'Y' in "%Y")
                       field->fmt_exist = EINA_TRUE;
                       field->location = location++; // Assign sequential location
                       sep_lookup = EINA_TRUE; // Next non-format chars are separators
                       len = 0; // Reset separator length
                       break; // Found field type, move to next char in format string
                    }
               }
          }
        if (cur == '%') // Start of a new format specifier
          {
             fmt_parsing = EINA_TRUE;
             sep_parsing = EINA_FALSE; // Stop collecting separator characters
             // Finalize separator for the *previous* field
             separator[len] = 0;
             if (field) eina_stringshare_replace(&field->separator, separator);
          }

        // Collect separator characters if in separator parsing mode
        if (sep_parsing &&
            (len < MAX_SEPARATOR_LEN - 1) && // Check buffer bounds
            (field && (field->type != EFL_UI_CLOCK_TYPE_AMPM)) && // AM/PM field doesn't have trailing separator
            (!strchr(ignore_separators, cur)) && // Not a globally ignored separator char
            (!strchr(mapping[idx].ignore_sep, cur))) // Not an ignored separator for current field type
          separator[len++] = cur;

        if (sep_lookup) // Just parsed a field, so now start looking for its separator
          {
             sep_parsing = EINA_TRUE;
             sep_lookup = EINA_FALSE;
          }
        fmt_ptr++; // Move to the next character in the format string
     }
   // After loop, store separator for the last field if any was being collected.
   // Note: The current logic stores the separator when '%' is encountered for the *next* field,
   // or implicitly if the string ends while `sep_parsing` is true and `field` is set.
   // The last field's separator might not be captured if the format string ends directly after it without a trailing '%'.
   // However, typical formats end with a field or have separators handled.
   // If `sep_parsing` is true at the end and `field` is valid, the collected separator should be assigned.
   // This is implicitly handled as `separator` buffer is filled and then assigned when a new '%' or end of string is processed.

   // return the number of valid fields parsed.
   return location;
}

/**
 * @brief Reloads and re-parses the clock's display format.
 *
 * This function is called when the format string changes or needs to be re-evaluated
 * (e.g., on locale change if not using a user-defined format).
 * It performs the following steps:
 * 1. Fetches the format string:
 *    - If a user format (`sd->user_format`) is set, it uses `sd->format`.
 *    - Otherwise, it retrieves the default system locale's date-time format (D_T_FMT).
 * 2. Expands any multi-field format specifiers (like %c, %x) in a temporary copy of the format string.
 * 3. Resets all clock fields to a default (disabled) state.
 * 4. Parses the expanded format string using `_parse_format()` to identify active fields,
 *    their order, and separators.
 * 5. Updates the `enabled_field_count` based on visible and format-defined fields.
 * 6. Sets the finger size multiplier for the layout based on the number of enabled fields.
 * 7. Assigns locations to any fields not defined in the format (for internal consistency).
 * 8. Emits Edje signals to enable/disable field parts in the theme based on whether they
 *    have a format and are visible.
 * 9. Sets the text for separator parts in the Edje theme.
 * 10. Processes Edje message signals to update the layout.
 * 11. Calls `_field_list_arrange()` to physically arrange and display the fields.
 *
 * @param obj The clock widget Evas_Object.
 */
static void
_reload_format(Evas_Object *obj)
{
   unsigned int idx, field_count;
   Clock_Field *field;
   char buf[BUFFER_SIZE];
   char *dt_fmt;

   EFL_UI_CLOCK_DATA_GET(obj, sd);
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   // FIXME: provide nl_langinfo on Windows if possible
   // fetch the default format from Libc.
   if (!sd->user_format)
#if defined(HAVE_LANGINFO_H) || defined (_WIN32)
     strncpy(sd->format, nl_langinfo(D_T_FMT), EFL_UI_CLOCK_MAX_FORMAT_LEN);
#else
     strncpy(sd->format, "", EFL_UI_CLOCK_MAX_FORMAT_LEN);
#endif
   sd->format[EFL_UI_CLOCK_MAX_FORMAT_LEN - 1] = '\0';

   dt_fmt = (char *)malloc(EFL_UI_CLOCK_MAX_FORMAT_LEN);
   if (!dt_fmt) return;

   strncpy(dt_fmt, sd->format, EFL_UI_CLOCK_MAX_FORMAT_LEN);

   _expand_format(dt_fmt);

   // reset all the fields to disable state
   sd->enabled_field_count = 0;
   for (idx = 0; idx < EFL_UI_CLOCK_TYPE_COUNT; idx++)
     {
        field = sd->field_list + idx;
        field->fmt_exist = EINA_FALSE;
        field->location = -1;
     }

   field_count = _parse_format(obj, dt_fmt);
   free(dt_fmt);

   for (idx = 0; idx < EFL_UI_CLOCK_TYPE_COUNT; idx++)
     {
        field = sd->field_list + idx;
        if (field->fmt_exist && field->visible)
          sd->enabled_field_count++;
     }
   efl_ui_layout_finger_size_multiplier_set(obj, sd->enabled_field_count, 1);

   // assign locations to disabled fields for uniform usage
   for (idx = 0; idx < EFL_UI_CLOCK_TYPE_COUNT; idx++)
     {
        field = sd->field_list + idx;
        if (field->location == -1) field->location = field_count++;

        if (field->fmt_exist && field->visible)
          {
             snprintf(buf, sizeof(buf), EDC_PART_FIELD_ENABLE_SIG_STR,
                      field->location);
             efl_layout_signal_emit(obj, buf, "efl");
          }
        else
          {
             snprintf(buf, sizeof(buf), EDC_PART_FIELD_DISABLE_SIG_STR,
                      field->location);
             efl_layout_signal_emit(obj, buf, "efl");
          }
        if (field->location + 1)
          {
             _part_name_snprintf(buf, sizeof(buf), obj, EDC_PART_SEPARATOR_STR,
                                 field->location + 1);
             efl_text_set(efl_part(obj, buf), field->separator);
          }
     }

   edje_object_message_signal_process(wd->resize_obj);
   _field_list_arrange(obj);
}

/**
 * @brief EOLIAN implementation for efl_ui_l10n_translation_update.
 *
 * Called when the system language or locale changes.
 * If a user-defined format is not set, it reloads the clock format to reflect
 * potential changes in the default locale's date/time representation.
 * Otherwise, it just redisplays the field list (e.g., month names might change).
 * Finally, it calls the superclass's translation update function.
 *
 * @param obj The clock widget.
 * @param sd Private data for the clock widget.
 */
EOLIAN static void
_efl_ui_clock_efl_ui_l10n_translation_update(Eo *obj, Efl_Ui_Clock_Data *sd)
{
   if (!sd->user_format) _reload_format(obj);
   else _field_list_display(obj);

   efl_ui_l10n_translation_update(efl_super(obj, MY_CLASS));
}

/**
 * @brief EOLIAN implementation for pause_set.
 *
 * Sets the paused state of the clock's ticker.
 * If paused, the ecore_timer responsible for updating the clock display
 * (especially for seconds) is frozen. If unpaused, it's thawed.
 *
 * @param obj The clock widget (unused).
 * @param sd Private data for the clock widget.
 * @param paused EINA_TRUE to pause the clock, EINA_FALSE to resume.
 */
EOLIAN static void
_efl_ui_clock_pause_set(Eo *obj EINA_UNUSED, Efl_Ui_Clock_Data *sd, Eina_Bool paused)
{
   paused = !!paused; // Normalize to EINA_TRUE or EINA_FALSE
   if (sd->paused == paused)
     return;
   sd->paused = paused;
   if (paused)
     ecore_timer_freeze(sd->ticker);
   else
     ecore_timer_thaw(sd->ticker);
}

/**
 * @brief EOLIAN implementation for pause_get.
 *
 * Gets the paused state of the clock's ticker.
 *
 * @param obj The clock widget (unused).
 * @param sd Private data for the clock widget.
 * @return EINA_TRUE if the clock is paused, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_clock_pause_get(const Eo *obj EINA_UNUSED, Efl_Ui_Clock_Data *sd)
{
   return sd->paused;
}

/**
 * @brief EOLIAN implementation for edit_mode_set.
 *
 * Sets the edit mode of the clock.
 * Note: The `edit_mode` flag is set, but its usage within this file
 * to alter behavior (e.g., making fields editable) is not apparent
 * from the provided code. It might be used by other parts of Elementary
 * or intended for future development.
 *
 * @param obj The clock widget (unused).
 * @param sd Private data for the clock widget.
 * @param edit_mode EINA_TRUE to enable edit mode, EINA_FALSE to disable.
 */
EOLIAN static void
_efl_ui_clock_edit_mode_set(Eo *obj EINA_UNUSED, Efl_Ui_Clock_Data *sd, Eina_Bool edit_mode)
{
   sd->edit_mode = edit_mode;
}

/**
 * @brief EOLIAN implementation for edit_mode_get.
 *
 * Gets the edit mode of the clock.
 *
 * @param obj The clock widget (unused).
 * @param sd Private data for the clock widget.
 * @return EINA_TRUE if edit mode is enabled, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_clock_edit_mode_get(const Eo *obj EINA_UNUSED, Efl_Ui_Clock_Data *sd)
{
   return sd->edit_mode;
}

/**
 * @brief EOLIAN implementation for efl_canvas_group_group_calculate.
 *
 * Overrides the default canvas group calculation.
 * The calculation is only performed if `sd->freeze_sizing` is EINA_FALSE.
 * This allows preventing recalculations during batch updates to the clock's structure or content.
 *
 * @param obj The clock widget.
 * @param sd Private data for the clock widget.
 */
EOLIAN static void
_efl_ui_clock_efl_canvas_group_group_calculate(Eo *obj, Efl_Ui_Clock_Data *sd)
{
   /* FIXME: this seems dumb */ // This comment suggests the condition might be overly simple or have side effects.
   if (!sd->freeze_sizing)
     efl_canvas_group_calculate(efl_super(obj, MY_CLASS));
}

/**
 * @brief EOLIAN implementation for efl_ui_widget_theme_apply.
 *
 * Applies the theme to the clock widget and its constituent field objects.
 * This involves:
 * 1. Calling the superclass's theme_apply method.
 * 2. Iterating through each clock field:
 *    - Updating the theme of the field's Evas_Object (textbox/button).
 *    - If the field is active (has a format and is visible):
 *      - Emitting an "enable" signal to its Edje part.
 *      - Setting the separator text for its preceding separator part (if applicable).
 *      - Updating the field's displayed value.
 *    - If the field is inactive:
 *      - Emitting a "disable" signal to its Edje part.
 * 3. Processing Edje message signals to ensure layout updates.
 *
 * @param obj The clock widget.
 * @param sd Private data for the clock widget.
 * @return EINA_ERROR_NONE on success, or an error code if theme application fails.
 */
EOLIAN static Eina_Error
_efl_ui_clock_efl_ui_widget_theme_apply(Eo *obj, Efl_Ui_Clock_Data *sd)
{
   Eina_Error int_ret = EFL_UI_THEME_APPLY_ERROR_GENERIC;

   Clock_Field *field;
   char buf[BUFFER_SIZE];
   unsigned int idx;

   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd, EINA_FALSE);

   int_ret = efl_ui_widget_theme_apply(efl_super(obj, MY_CLASS));
   if (int_ret == EFL_UI_THEME_APPLY_ERROR_GENERIC) return int_ret;

   for (idx = 0; idx < EFL_UI_CLOCK_TYPE_COUNT; idx++)
     {
        field = sd->field_list + idx;
        // TODO: Different group name for each field_obj may be needed.
        elm_widget_element_update(obj, field->item_obj, PART_NAME_ARRAY[idx]);
        if (field->fmt_exist && field->visible)
          {
             snprintf(buf, sizeof(buf), EDC_PART_FIELD_ENABLE_SIG_STR,
                      field->location);
             efl_layout_signal_emit(obj, buf, "efl");

             if (field->location)
               {
                  _part_name_snprintf(buf, sizeof(buf), obj, EDC_PART_SEPARATOR_STR,
                                      field->location);
                  efl_text_set(efl_part(obj, buf), field->separator);
               }

             field_value_display(obj, field->item_obj);
          }
        else
          {
             snprintf(buf, sizeof(buf), EDC_PART_FIELD_DISABLE_SIG_STR,
                      field->location);
             efl_layout_signal_emit(obj, buf, "efl");
          }
     }

   edje_object_message_signal_process(wd->resize_obj);

   return int_ret;
}

/**
 * @brief Calculates the maximum number of days in a given month and year.
 *
 * This function correctly handles leap years for February.
 * It works by setting a `struct tm` to the given year and month,
 * then iterating the day from MIN_DAYS_IN_MONTH (28) upwards. `mktime`
 * is used to normalize the date; if `tm_mday` wraps to 1, it means
 * the previous day was the last day of the month.
 * `tm_isdst` is set to -1 to make `mktime` determine DST, which can be
 * important for correct normalization across DST transitions, though for
 * just finding the number of days, its impact is usually minimal unless
 * the transition happens exactly at midnight.
 *
 * @param year The year (e.g., 2023). Note: `tm_year` is year since 1900.
 * @param month The month (0-11, where 0 is January). Note: `tm_mon` is 0-11.
 * @return The number of days in the specified month and year (e.g., 28, 29, 30, or 31).
 */
static int
_max_days_get(int year,
              int month)
{
   struct tm time1;
   time_t t;
   int day;

   t = time(NULL);
   localtime_r(&t, &time1);
   time1.tm_year = year;
   time1.tm_mon = month;
   for (day = MIN_DAYS_IN_MONTH; day <= mapping[EFL_UI_CLOCK_TYPE_DATE].def_max;
        day++)
     {
        time1.tm_mday = day;
        mktime(&time1);
        /* To restrict month wrapping because of summer time in some locales,
        * ignore day light saving mode in mktime(). */
        time1.tm_isdst = -1;
        if (time1.tm_mday == 1) break;
     }
   day--;

   return day;
}

/**
 * @brief Compares two `struct tm` date/time structures for equality across all relevant fields.
 *
 * It iterates through year, month, day, hour, minute, second, and weekday,
 * comparing each corresponding field in `time1` and `time2`.
 * The comparison stops at EFL_UI_CLOCK_TYPE_COUNT, which includes up to tm_wday.
 * The 8th element in CLOCK_TM_ARRAY is tm_hour again (for AM/PM logic), but the loop
 * condition `idx < EFL_UI_CLOCK_TYPE_COUNT` (which is 8) means it compares:
 * tm_year, tm_mon, tm_mday, tm_hour, tm_min, tm_sec, tm_wday, tm_hour (again).
 * This seems to compare tm_hour twice.
 *
 * @param time1 Pointer to the first `struct tm` to compare.
 * @param time2 Pointer to the second `struct tm` to compare.
 * @return EINA_TRUE if all compared fields are identical, EINA_FALSE otherwise.
 */
static Eina_Bool
_date_cmp(const struct tm *time1,
          const struct tm *time2)
{
   unsigned int idx;

   const CLOCK_TM_ARRAY(timearr1, time1);
   const CLOCK_TM_ARRAY(timearr2, time2);

   for (idx = 0; idx < EFL_UI_CLOCK_TYPE_COUNT; idx++)
     {
        if (*timearr1[idx] != *timearr2[idx])
          return EINA_FALSE;
     }

   return EINA_TRUE;
}

/**
 * @brief Compares a single specified field between two `struct tm` date/time structures.
 *
 * @param field_type The specific clock field (Efl_Ui_Clock_Type) to compare
 *                   (e.g., EFL_UI_CLOCK_TYPE_YEAR, EFL_UI_CLOCK_TYPE_MONTH).
 * @param time1 Pointer to the first `struct tm`.
 * @param time2 Pointer to the second `struct tm`.
 * @return EINA_TRUE if the specified field is identical in both structures, EINA_FALSE otherwise.
 */
static Eina_Bool
_field_cmp(Efl_Ui_Clock_Type field_type,
          struct tm *time1,
          struct tm *time2)
{
   CLOCK_TM_ARRAY(timearr1, time1);
   CLOCK_TM_ARRAY(timearr2, time2);

   if (*timearr1[field_type] != *timearr2[field_type])
     return EINA_FALSE;
   else
     return EINA_TRUE;
}

/**
 * @brief Validates and adjusts one time structure (`time1` or `time2`) against the other.
 *
 * This function compares `time1` and `time2` field by field (year, month, day, etc.).
 * The `swap` parameter determines the direction of validation:
 * - If `swap` is EINA_FALSE: If `time1` is found to be "less than" `time2`
 *   (e.g., year of `time1` < year of `time2`, or years are equal but month of `time1` < month of `time2`, etc.),
 *   then `time1` is updated to be a copy of `time2`. This effectively ensures `time1 >= time2`.
 *   Used for: `_validate_clock_limits(&sd->curr_time, &sd->min_limit, EINA_FALSE)` -> ensures `curr_time >= min_limit`.
 *
 * - If `swap` is EINA_TRUE: The roles of `time1` and `time2` in the comparison logic are effectively swapped.
 *   If `time2` is found to be "less than" `time1`, then `time2` is updated to be a copy of `time1`.
 *   This effectively ensures `time2 >= time1`.
 *   Used for: `_validate_clock_limits(&sd->max_limit, &sd->curr_time, EINA_TRUE)` -> ensures `max_limit >= curr_time`.
 *
 * The comparison proceeds from most significant field (year) to least significant.
 * As soon as a field in the "primary" time (determined by `swap`) is found to be
 * definitively less than the corresponding field in the "secondary" time, the primary
 * time is updated with the secondary time's values, and the function returns.
 * If a field in the primary time is greater, or if all fields are equal up to the point of
 * comparison, no change is made based on that field, and comparison continues or stops.
 *
 * The loop iterates up to `EFL_UI_CLOCK_TYPE_COUNT - 1`, which means it compares
 * year, month, day, hour, minute, second, wday.
 *
 * @param time1 Pointer to the first `struct tm`. This might be modified.
 * @param time2 Pointer to the second `struct tm`. This might be modified if `swap` is true.
 * @param swap If EINA_TRUE, `time2` is validated against `time1` and potentially modified.
 *             If EINA_FALSE, `time1` is validated against `time2` and potentially modified.
 */
static void
_validate_clock_limits(struct tm *time1,
                          struct tm *time2,
                          Eina_Bool swap)
{
   struct tm *t1, *t2; // t1 is the one to be potentially modified, t2 is the reference
   unsigned int idx;

   if (!time1 || !time2) return;

   // Determine which time struct is being validated (t1) and which is the reference (t2)
   t1 = (swap) ? time2 : time1; // If swap, t1 is time2 (e.g. max_limit being validated against curr_time)
   t2 = (swap) ? time1 : time2; // If swap, t2 is time1 (e.g. curr_time is the reference for max_limit)

   // Use CLOCK_TM_ARRAY with original time1 and time2 for comparison logic based on their original roles
   // This seems a bit confusing. Let's trace:
   // If swap = FALSE: t1 = time1, t2 = time2. We compare time1 vs time2. If time1 < time2, time1 = time2.
   // If swap = TRUE:  t1 = time2, t2 = time1. We compare time2 vs time1. If time2 < time1, time2 = time1.
   // The array access below uses timearr1 (from original time1) and timearr2 (from original time2).
   CLOCK_TM_ARRAY(timearr1_orig, time1); // Pointers into original time1
   CLOCK_TM_ARRAY(timearr2_orig, time2); // Pointers into original time2

   // The loop compares fields of original time1 and original time2.
   // The decision to copy is based on this comparison.
   // The actual copy operation uses t1 and t2.
   for (idx = 0; idx < EFL_UI_CLOCK_TYPE_COUNT - 1; idx++) // Iterate year, mon, mday, hour, min, sec, wday
     {
        // Standard comparison: is time1 < time2?
        if (*timearr1_orig[idx] < *timearr2_orig[idx])
          {
             // If time1 is indeed less than time2:
             // If !swap (validating curr against min, curr is time1): copy time2 (min) to time1 (curr). Correct.
             // If swap (validating max against curr, max is time1): copy time2 (curr) to time1 (max). This seems wrong.
             // Let's re-evaluate `t1` and `t2`'s purpose.
             // `t1` is the structure that will be overwritten if the condition is met.
             // `t2` is the source structure for the overwrite.
             //
             // Case 1: `_validate_clock_limits(&sd->curr_time, &sd->min_limit, EINA_FALSE)`
             //   time1 = &sd->curr_time, time2 = &sd->min_limit, swap = EINA_FALSE
             //   t1 = &sd->curr_time, t2 = &sd->min_limit
             //   Compares curr_time fields with min_limit fields.
             //   If curr_time.field < min_limit.field:
             //     memcpy(t1, t2, sizeof(struct tm)) => memcpy(&sd->curr_time, &sd->min_limit, ...)
             //     This makes curr_time = min_limit. This is to ensure curr_time >= min_limit. Correct.
             //
             // Case 2: `_validate_clock_limits(&sd->max_limit, &sd->curr_time, EINA_TRUE)`
             //   time1 = &sd->max_limit, time2 = &sd->curr_time, swap = EINA_TRUE
             //   t1 = &sd->curr_time, t2 = &sd->max_limit
             //   Compares max_limit fields with curr_time fields (timearr1_orig vs timearr2_orig).
             //   If max_limit.field < curr_time.field:
             //     memcpy(t1, t2, sizeof(struct tm)) => memcpy(&sd->curr_time, &sd->max_limit, ...)
             //     This makes curr_time = max_limit. This is to ensure curr_time <= max_limit. Correct.
             // The key is that `timearr1_orig` and `timearr2_orig` always refer to the parameters `time1` and `time2`
             // in their original roles for the comparison logic, while `t1` and `t2` determine the target and source of the copy.

             // If the first structure (parameter `time1`) is chronologically earlier than the second (parameter `time2`)
             if (!swap) // e.g. curr_time vs min_limit. If curr_time < min_limit, set curr_time = min_limit
               memcpy(time1, time2, sizeof(struct tm));
             else // e.g. max_limit vs curr_time. If max_limit < curr_time, set curr_time = max_limit (so effectively max_limit is now curr_time)
                  // This means if max_limit is too small (less than curr_time), curr_time is clamped down to max_limit.
               memcpy(time2, time1, sizeof(struct tm)); // if time1(max_limit) < time2(curr_time), then time2(curr_time) = time1(max_limit)
             break; // Validation done for this pair
          }
        // If the first structure (parameter `time1`) is chronologically later, no adjustment needed in this direction.
        else if (*timearr1_orig[idx] > *timearr2_orig[idx])
          {
            // If !swap (curr_time vs min_limit): curr_time.field > min_limit.field. This is fine.
            // If swap (max_limit vs curr_time): max_limit.field > curr_time.field. This is fine.
            break; // No change needed based on this field comparison; further fields don't matter.
          }
        // If fields are equal, continue to the next less significant field.
     }
}

/**
 * @brief Applies the individual min/max limits defined for each field to the clock's current time.
 *
 * Iterates through each field of `sd->curr_time` (year, month, day, etc., excluding AM/PM).
 * For each field, it clamps the value in `sd->curr_time` to be within the
 * `field->min` and `field->max` limits stored in `sd->field_list`.
 * After clamping all fields, it calls `_field_list_display` to update the UI.
 *
 * @param obj The clock widget Evas_Object.
 */
static void
_apply_field_limits(Evas_Object *obj)
{
   Clock_Field *field;
   unsigned int idx = 0;
   int val;

   EFL_UI_CLOCK_DATA_GET(obj, sd);

   CLOCK_TM_ARRAY(timearr, &sd->curr_time);
   for (idx = 0; idx < EFL_UI_CLOCK_TYPE_COUNT - 1; idx++)
     {
        field = sd->field_list + idx;
        val = *timearr[idx];
        if (val < field->min)
          *timearr[idx] = field->min;
        else if (val > field->max)
          *timearr[idx] = field->max;
     }

   _field_list_display(obj);
}

/**
 * @brief Applies default range restrictions to the fields of a given `struct tm`.
 *
 * This function ensures that month, date, hour, minute, and second values
 * in the provided `tim` structure are within their standard valid ranges
 * (e.g., month 0-11, date 1-max_days_in_month).
 * For the date field, it dynamically calculates the maximum valid day for the
 * given month and year using `_max_days_get()`.
 *
 * @param tim Pointer to the `struct tm` whose fields are to be restricted.
 *            This structure is modified in place.
 */
static void
_apply_range_restrictions(struct tm *tim)
{
   unsigned int idx;
   int val, min, max;

   if (!tim) return;

   CLOCK_TM_ARRAY(timearr, tim);
   for (idx = EFL_UI_CLOCK_TYPE_MONTH; idx < EFL_UI_CLOCK_TYPE_COUNT - 1; idx++)
     {
        val = *timearr[idx];
        min = mapping[idx].def_min;
        max = mapping[idx].def_max;
        if (idx == EFL_UI_CLOCK_TYPE_DATE)
          max = _max_days_get(tim->tm_year, tim->tm_mon);
        if (val < min)
          *timearr[idx] = min;
        else if (val > max)
          *timearr[idx] = max;
     }
}

/**
 * @brief Initializes the clock's field data structures.
 *
 * This function performs the initial setup for all clock fields:
 * 1. Sets `sd->curr_time` to the current system time.
 * 2. Sets the default min/max year limits from `_elm_config`.
 * 3. For each field type (Year, Month, ..., AMPM):
 *    - Initializes its `Clock_Field` structure:
 *      - Sets `type` (e.g., EFL_UI_CLOCK_TYPE_YEAR).
 *      - Sets the base format character `fmt[0]` to '%'.
 *      - Marks `fmt_exist` as EINA_FALSE (no specific format char like 'Y' or 'm' yet).
 *      - Sets `visible` to EINA_TRUE by default.
 *      - Sets `min` and `max` to default values from the `mapping` array.
 * 4. Initializes `sd->min_limit` and `sd->max_limit` `struct tm` members to overall
 *    default minimums and maximums based on the `mapping` array (e.g., min_limit.tm_mon = 0,
 *    max_limit.tm_mon = 11).
 *
 * @param obj The clock widget Evas_Object (used to get private data `sd`).
 */
static void
_field_list_init(Evas_Object *obj)
{
   Clock_Field *field;
   unsigned int idx;
   time_t t;

   EFL_UI_CLOCK_DATA_GET(obj, sd);

   t = time(NULL);
   localtime_r(&t, &sd->curr_time);

   mapping[EFL_UI_CLOCK_TYPE_YEAR].def_min = _elm_config->year_min;
   mapping[EFL_UI_CLOCK_TYPE_YEAR].def_max = _elm_config->year_max;
   for (idx = 0; idx < EFL_UI_CLOCK_TYPE_COUNT; idx++)
     {
        field = sd->field_list + idx;
        field->type = EFL_UI_CLOCK_TYPE_YEAR + idx;
        field->fmt[0] = '%';
        field->fmt_exist = EINA_FALSE;
        field->visible = EINA_TRUE;
        field->min = mapping[idx].def_min;
        field->max = mapping[idx].def_max;
     }
   CLOCK_TM_ARRAY(min_timearr, &sd->min_limit);
   CLOCK_TM_ARRAY(max_timearr, &sd->max_limit);
   for (idx = 0; idx < EFL_UI_CLOCK_TYPE_COUNT - 1; idx++)
     {
        *min_timearr[idx] = mapping[idx].def_min;
        *max_timearr[idx] = mapping[idx].def_max;
     }
}

/**
 * @brief Callback function to provide accessibility information for the clock widget.
 *
 * Constructs a string describing the current date and time set in the clock.
 * Example: "2023 year, 12 month, 25 date, 10 hour, 30 minute".
 * Note: Month is displayed as 1-12 for accessibility.
 *
 * @param data The clock widget Evas_Object.
 * @param obj The Evas_Object that triggered the accessibility request (unused).
 * @return A newly allocated string containing the accessibility information.
 *         The caller is responsible for freeing this string.
 */
static char *
_access_info_cb(void *data, Evas_Object *obj EINA_UNUSED)
{
   char *ret;
   Eina_Strbuf *buf;
   buf = eina_strbuf_new();

   EFL_UI_CLOCK_DATA_GET(data, sd);
   eina_strbuf_append_printf(buf,
                             "%d year, %d month, %d date, %d hour, %d minute",
                             sd->curr_time.tm_year, sd->curr_time.tm_mon + 1,
                             sd->curr_time.tm_mday, sd->curr_time.tm_hour,
                             sd->curr_time.tm_min);

   ret = eina_strbuf_string_steal(buf);
   eina_strbuf_free(buf);
   return ret;
}

/**
 * @brief Ecore_Timer callback function to update the clock display periodically.
 *
 * This function is typically called every second (or fraction of a second
 * to align with the next second boundary).
 * It updates `sd->curr_time` to the current system time.
 * If the seconds field has changed to 0 (i.e., a new minute has begun),
 * it calls `_field_list_display()` to update all visible fields.
 * Otherwise (if seconds > 0), it only updates the seconds field display
 * if that field is active and visible.
 * It then reschedules itself to run again, calculating the delay until
 * the next exact second.
 *
 * @param data The clock widget Evas_Object.
 * @return ECORE_CALLBACK_CANCEL, as the timer is re-added manually for precise timing.
 */
static Eina_Bool
_ticker(void *data)
{
   double t;
   time_t tt;
   struct timeval timev;
   Clock_Field *field;

   EFL_UI_CLOCK_DATA_GET(data, sd);

   tt = time(NULL);
   localtime_r(&tt, &sd->curr_time);

   if (sd->curr_time.tm_sec > 0)
     {
        field = sd->field_list + EFL_UI_CLOCK_TYPE_SECOND;
        if (field->fmt_exist && field->visible)
          field_value_display(data, field->item_obj);
     }
   else
     _field_list_display(data);

   gettimeofday(&timev, NULL);
   t = ((double)(1000000 - timev.tv_usec)) / 1000000.0;
   sd->ticker = ecore_timer_add(t, _ticker, data);

   return ECORE_CALLBACK_CANCEL;
}

EOLIAN static void
_efl_ui_clock_efl_canvas_group_group_add(Eo *obj, Efl_Ui_Clock_Data *priv)
{
   Clock_Field *field;
   int idx;
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   if (!elm_widget_theme_klass_get(obj))
     elm_widget_theme_klass_set(obj, "uiclock");
   efl_canvas_group_add(efl_super(obj, MY_CLASS));

   if (elm_widget_theme_object_set(obj, wd->resize_obj,
                                       elm_widget_theme_klass_get(obj),
                                       elm_widget_theme_element_get(obj),
                                       elm_widget_theme_style_get(obj)) == EFL_UI_THEME_APPLY_ERROR_GENERIC)
     CRI("Failed to set layout!");

   for (idx = 0; idx < EFL_UI_CLOCK_TYPE_COUNT; idx++)
     {
        field = priv->field_list + idx;
        field->item_obj = field_create(obj, idx);
     }

   priv->freeze_sizing = EINA_TRUE;

   _field_list_init(obj);
   _reload_format(obj);
   _ticker(obj);

   elm_widget_can_focus_set(obj, EINA_TRUE);

   priv->freeze_sizing = EINA_FALSE;

   // ACCESS
   if (_elm_config->access_mode == ELM_ACCESS_MODE_ON)
     {
        Elm_Access_Info *ai;

        priv->access_obj = _elm_access_edje_object_part_object_register
          (obj, elm_layout_edje_get(obj), "efl.access");
        if (!priv->access_obj)
          priv->access_obj = _elm_access_edje_object_part_object_register
          (obj, elm_layout_edje_get(obj), "access");

        ai = _elm_access_info_get(priv->access_obj);
        _elm_access_text_set(ai, ELM_ACCESS_TYPE, "date time");
        _elm_access_callback_set(ai, ELM_ACCESS_INFO, _access_info_cb, obj);
     }
}

EOLIAN static void
_efl_ui_clock_efl_canvas_group_group_del(Eo *obj, Efl_Ui_Clock_Data *sd)
{
   Clock_Field *tmp;
   unsigned int idx;

   ecore_timer_del(sd->ticker);
   for (idx = 0; idx < EFL_UI_CLOCK_TYPE_COUNT; idx++)
     {
        tmp = sd->field_list + idx;
        evas_object_del(tmp->item_obj);
        eina_stringshare_del(tmp->separator);
     }

   efl_canvas_group_del(efl_super(obj, MY_CLASS));
}

EOLIAN static Eo *
_efl_ui_clock_efl_object_constructor(Eo *obj, Efl_Ui_Clock_Data *_pd EINA_UNUSED)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));
   evas_object_smart_callbacks_descriptions_set(obj, _smart_callbacks);
   efl_access_object_role_set(obj, EFL_ACCESS_ROLE_DATE_EDITOR);

   return obj;
}

EOLIAN static const char*
_efl_ui_clock_format_get(const Eo *obj EINA_UNUSED, Efl_Ui_Clock_Data *sd)
{
   return sd->format;
}

EOLIAN static void
_efl_ui_clock_format_set(Eo *obj, Efl_Ui_Clock_Data *sd, const char *fmt)
{
   if (fmt)
     {
        strncpy(sd->format, fmt, EFL_UI_CLOCK_MAX_FORMAT_LEN);
        sd->format[EFL_UI_CLOCK_MAX_FORMAT_LEN - 1] = '\0';
        sd->user_format = EINA_TRUE;
     }
   else sd->user_format = EINA_FALSE;

   _reload_format(obj);
}

EOLIAN static Eina_Bool
_efl_ui_clock_field_visible_get(const Eo *obj EINA_UNUSED, Efl_Ui_Clock_Data *sd, Efl_Ui_Clock_Type fieldtype)
{
   Clock_Field *field;

   if (fieldtype > EFL_UI_CLOCK_TYPE_AMPM) return EINA_FALSE;

   field = sd->field_list + fieldtype;

   return field->visible;
}

EOLIAN static void
_efl_ui_clock_field_visible_set(Eo *obj, Efl_Ui_Clock_Data *sd, Efl_Ui_Clock_Type fieldtype, Eina_Bool visible)
{
   char buf[BUFFER_SIZE];
   Clock_Field *field;

   if (fieldtype > EFL_UI_CLOCK_TYPE_AMPM) return;

   field = sd->field_list + fieldtype;
   visible = !!visible;
   if (field->visible == visible) return;

   field->visible = visible;

   sd->freeze_sizing = EINA_TRUE;
   if (visible)
     {
        sd->enabled_field_count++;

        if (!field->fmt_exist) return;

        snprintf(buf, sizeof(buf), EDC_PART_FIELD_ENABLE_SIG_STR,
                 field->location);
        efl_layout_signal_emit(obj, buf, "efl");

        ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);
        edje_object_message_signal_process(wd->resize_obj);

        _part_name_snprintf(buf, sizeof(buf), obj, EDC_PART_FIELD_STR,
                            field->location);
        efl_content_unset(efl_part(obj, buf));
        efl_content_set(efl_part(obj, buf), field->item_obj);
     }
   else
     {
        sd->enabled_field_count--;

        if (!field->fmt_exist) return;

        snprintf(buf, sizeof(buf), EDC_PART_FIELD_DISABLE_SIG_STR,
                 field->location);
        efl_layout_signal_emit(obj, buf, "efl");

        ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);
        edje_object_message_signal_process(wd->resize_obj);

        _part_name_snprintf(buf, sizeof(buf), obj, EDC_PART_FIELD_STR,
                            field->location);
        efl_gfx_entity_visible_set(efl_content_unset(efl_part(obj, buf)), EINA_FALSE);
     }
   sd->freeze_sizing = EINA_FALSE;
   efl_ui_layout_finger_size_multiplier_set(obj, sd->enabled_field_count, 1);

   efl_canvas_group_change(obj);

   if (!visible) return;
   field_value_display(obj, field->item_obj);
}

EOLIAN static void
_efl_ui_clock_field_limit_get(const Eo *obj EINA_UNUSED, Efl_Ui_Clock_Data *sd, Efl_Ui_Clock_Type fieldtype, int *min, int *max)
{
   Clock_Field *field;

   if (fieldtype >= EFL_UI_CLOCK_TYPE_AMPM) return;

   field = sd->field_list + fieldtype;
   if (min) *min = field->min;
   if (max) *max = field->max;
}

EOLIAN static void
_efl_ui_clock_field_limit_set(Eo *obj, Efl_Ui_Clock_Data *sd, Efl_Ui_Clock_Type fieldtype, int min, int max)
{
   Clock_Field *field;
   struct tm old_time;

   if (fieldtype >= EFL_UI_CLOCK_TYPE_AMPM) return;

   if (min > max) return;

   old_time = sd->curr_time;
   field = sd->field_list + fieldtype;
   if (((min >= mapping[fieldtype].def_min) &&
        (min <= mapping[fieldtype].def_max)) ||
       (field->type == EFL_UI_CLOCK_TYPE_YEAR))
     field->min = min;
   if (((max >= mapping[fieldtype].def_min) &&
        (max <= mapping[fieldtype].def_max)) ||
       (field->type == EFL_UI_CLOCK_TYPE_YEAR))
     field->max = max;

   _apply_field_limits(obj);

   if (!_field_cmp(fieldtype, &old_time, &sd->curr_time))
     efl_event_callback_legacy_call(obj, EFL_UI_CLOCK_EVENT_CHANGED, NULL);
}

EOLIAN static Efl_Time
_efl_ui_clock_time_get(const Eo *obj EINA_UNUSED, Efl_Ui_Clock_Data *sd)
{
   return sd->curr_time;
}

EOLIAN static void
_efl_ui_clock_time_set(Eo *obj, Efl_Ui_Clock_Data *sd, Efl_Time newtime)
{
   if (_date_cmp(&sd->curr_time, &newtime)) return;
   sd->curr_time = newtime;
   // apply default field restrictions for curr_time
   _apply_range_restrictions(&sd->curr_time);
   // validate the curr_time according to the min_limt and max_limt
   _validate_clock_limits(&sd->curr_time, &sd->min_limit, EINA_FALSE);
   _validate_clock_limits(&sd->max_limit, &sd->curr_time, EINA_TRUE);
   _apply_field_limits(obj);

   efl_event_callback_legacy_call(obj, EFL_UI_CLOCK_EVENT_CHANGED, NULL);
}

EOLIAN static Efl_Time
_efl_ui_clock_time_min_get(const Eo *obj EINA_UNUSED, Efl_Ui_Clock_Data *sd)
{
   return sd->min_limit;
}

EOLIAN static void
_efl_ui_clock_time_min_set(Eo *obj, Efl_Ui_Clock_Data *sd, Efl_Time mintime)
{
   struct tm old_time;

   if (_date_cmp(&sd->min_limit, &mintime)) return;
   sd->min_limit = mintime;
   old_time = sd->curr_time;
   // apply default field restrictions for min_limit
   _apply_range_restrictions(&sd->min_limit);
   // validate curr_time and max_limt according to the min_limit
   _validate_clock_limits(&sd->max_limit, &sd->min_limit, EINA_FALSE);
   _validate_clock_limits(&sd->curr_time, &sd->min_limit, EINA_FALSE);
   _apply_field_limits(obj);

   if (!_date_cmp(&old_time, &sd->curr_time))
     efl_event_callback_legacy_call(obj, EFL_UI_CLOCK_EVENT_CHANGED, NULL);
}

EOLIAN static Efl_Time
_efl_ui_clock_time_max_get(const Eo *obj EINA_UNUSED, Efl_Ui_Clock_Data *sd)
{
   return sd->max_limit;
}

EOLIAN static void
_efl_ui_clock_time_max_set(Eo *obj, Efl_Ui_Clock_Data *sd, Efl_Time maxtime)
{
   struct tm old_time;

   if (_date_cmp(&sd->max_limit, &maxtime)) return;
   sd->max_limit = maxtime;
   old_time = sd->curr_time;
   // apply default field restrictions for max_limit
   _apply_range_restrictions(&sd->max_limit);
   // validate curr_time and min_limt according to the max_limit
   _validate_clock_limits(&sd->max_limit, &sd->min_limit, EINA_TRUE);
   _validate_clock_limits(&sd->max_limit, &sd->curr_time, EINA_TRUE);
   _apply_field_limits(obj);

   if (!_date_cmp(&old_time, &sd->curr_time))
     efl_event_callback_legacy_call(obj, EFL_UI_CLOCK_EVENT_CHANGED, NULL);
}

/* Internal EO APIs and hidden overrides */

#define EFL_UI_CLOCK_EXTRA_OPS \
   EFL_CANVAS_GROUP_ADD_DEL_OPS(efl_ui_clock)

#include "efl_ui_clock.eo.c"
