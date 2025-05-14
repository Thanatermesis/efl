#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#define EFL_CORE_COMMAND_LINE_PROTECTED

#include <Efl_Core.h>

#define MY_CLASS EFL_CORE_COMMAND_LINE_MIXIN

typedef struct {
   Eina_Bool filled;
   char *string_command;
   Eina_Array *command; /**< Array of command arguments after parsing */
} Efl_Core_Command_Line_Data;

/**
 * @internal
 * @brief Unescapes a command string into an array of arguments.
 *
 * This function parses a command string, handling single and double quotes,
 * and escape characters. It splits the string into individual arguments.
 * For example, the string "command "arg1 with spaces" 'arg2\'s content'"
 * would be parsed into ["command", "arg1 with spaces", "arg2's content"].
 *
 * @param s The command string to unescape.
 * @return A new Eina_Array containing eina_stringshare instances for each argument,
 *         or NULL on failure or if s is NULL. The caller is responsible for freeing
 *         the returned array and its contents.
 */
static Eina_Array *
_unescape(const char *s)
{
   Eina_Array *args;
   const char *p;
   char *tmp = NULL, *d = NULL;
   if (!s) return NULL;

   Eina_Bool in_quote_dbl = EINA_FALSE;
   Eina_Bool in_quote = EINA_FALSE;

   args = eina_array_new(16);
   if (!args) return NULL;
   for (p = s; *p; p++)
     {
        if (!tmp) tmp = d = strdup(p);
        if (tmp)
          {
             if (in_quote_dbl)
               {
                  switch (*p)
                    {
                     case '\"':
                       in_quote_dbl = EINA_FALSE;
                       *d = 0;
                       eina_array_push(args, eina_stringshare_add(tmp));
                       free(tmp);
                       tmp = d = NULL;
                       break;
                     case '\\':
                       p++;
                       EINA_FALLTHROUGH
                     default:
                       *d = *p;
                       d++;
                       break;
                    }
               }
             else if (in_quote)
               {
                  switch (*p)
                    {
                     case '\'':
                       in_quote = EINA_FALSE;
                       *d = 0;
                       eina_array_push(args, eina_stringshare_add(tmp));
                       free(tmp);
                       tmp = d = NULL;
                       break;
                     case '\\':
                       p++;
                       EINA_FALLTHROUGH
                     default:
                       *d = *p;
                       d++;
                       break;
                    }
               }
             else
               {
                  switch (*p)
                    {
                     case ' ':
                     case '\t':
                     case '\r':
                     case '\n':
                       *d = 0;
                       eina_array_push(args, eina_stringshare_add(tmp));
                       free(tmp);
                       tmp = d = NULL;
                       break;
                     case '\"':
                       in_quote_dbl = EINA_TRUE;
                       break;
                     case '\'':
                       in_quote = EINA_TRUE;
                       break;
                     case '\\':
                       p++;
                       EINA_FALLTHROUGH
                     default:
                       *d = *p;
                       d++;
                       break;
                    }
               }
          }
     }
   if (tmp)
     {
        *d = 0;
        eina_array_push(args, eina_stringshare_add(tmp));
        free(tmp);
     }
   return args;
}

/**
 * @internal
 * @brief Escapes a string to be safely used as a single command line argument.
 *
 * This function takes a string and escapes special characters (', ", $, #, ;,
 * &, `, |, (, ), [, ], {, }, >, <, newline, carriage return, tab, space).
 * If any of these characters are present, the entire string is enclosed in
 * double quotes. Within the string, backslashes, single quotes, and double
 * quotes are escaped with a backslash.
 * For example, "arg with 'quotes' and spaces" becomes "\"arg with \\'quotes\\' and spaces\"".
 *
 * @param s The string to escape.
 * @return A new character string that has been escaped. The caller is responsible
 *         for freeing this string. Returns NULL on allocation failure.
 */
static char *
_escape(const char *s)
{
   Eina_Bool need_quote = EINA_FALSE;
   const char *p;
   char *s2 = malloc((strlen(s) * 2) + 1 + 2), *d;

   if (!s2) return NULL;

   for (p = s; *p; p++)
     {
        switch (*p)
          {
           case '\'':
           case '\"':
           case '$':
           case '#':
           case ';':
           case '&':
           case '`':
           case '|':
           case '(':
           case ')':
           case '[':
           case ']':
           case '{':
           case '}':
           case '>':
           case '<':
           case '\n':
           case '\r':
           case '\t':
           case ' ':
             need_quote = EINA_TRUE;
           default:
             break;
          }
     }

   d = s2;
   if (need_quote)
     {
        *d = '\"';
        d++;
     }
   for (p = s; *p; p++, d++)
     {
        switch (*p)
          {
           case '\\':
           case '\'':
           case '\"':
             *d = '\\';
             d++;
             EINA_FALLTHROUGH
           default:
             *d = *p;
             break;
          }
     }
   if (need_quote)
     {
        *d = '\"';
        d++;
     }
   *d = 0;
   return s2;
}

EOLIAN static const char *
_efl_core_command_line_command_get(const Eo *obj EINA_UNUSED, Efl_Core_Command_Line_Data *pd)
{
   return pd->string_command;
}

EOLIAN static Eina_Accessor *
_efl_core_command_line_command_access(Eo *obj EINA_UNUSED, Efl_Core_Command_Line_Data *pd)
{
   return pd->command ? eina_array_accessor_new(pd->command) : NULL;
}

/**
 * @internal
 * @brief Replaces invalid characters in a command string.
 *
 * This function iterates through the command string and replaces any
 * characters outside the printable ASCII range (0x20-0x7E) or the DEL
 * character (0x7F) with a placeholder character (0x12 - Device Control Two).
 * This is done to prevent issues with non-standard characters in commands.
 *
 * @param command The command string to modify in-place.
 */
static void
_remove_invalid_chars(char *command)
{
   for (unsigned int i = 0; i < strlen(command); ++i)
     {
        char c = command[i];
        if (c < 0x20 || c == 0x7f)
          command[i] = '\x12';
     }
}

/**
 * @internal
 * @brief Clears the internal command array.
 *
 * This function frees all the stringshared arguments stored in the
 * `pd->command` array and then frees the array itself. It sets
 * `pd->command` to NULL.
 *
 * @param pd Pointer to the Efl_Core_Command_Line_Data structure.
 */
static void
_clear_command(Efl_Core_Command_Line_Data *pd)
{
   if (!pd->command) return;
   while (eina_array_count(pd->command) > 0)
     eina_stringshare_del(eina_array_pop(pd->command));
   eina_array_free(pd->command);
   pd->command = NULL;
}

/**
 * @brief Sets the command and its arguments from an Eina_Array.
 *
 * Each element in the input array should be a C string (char *).
 * These strings are then stringshared and stored internally.
 * The function also constructs a single string representation of the command
 * by escaping and joining the arguments.
 *
 * Example of array structure:
 * Eina_Array *my_array = eina_array_new(3);
 * eina_array_push(my_array, eina_stringshare_add("command"));
 * eina_array_push(my_array, eina_stringshare_add("arg1"));
 * eina_array_push(my_array, eina_stringshare_add("arg with space"));
 * // This would result in pd->string_command being "command arg1 \"arg with space\""
 * // and pd->command containing ["command", "arg1", "arg with space"].
 *
 * @param[in] obj The Efl_Core_Command_Line object.
 * @param[in,out] pd The private data for the Efl_Core_Command_Line object.
 * @param[in] array An Eina_Array of C strings representing the command and its arguments.
 *                  The function takes ownership of the strings within the array (by stringsharing)
 *                  and frees the array itself.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_core_command_line_command_array_set(Eo *obj EINA_UNUSED, Efl_Core_Command_Line_Data *pd, Eina_Array *array)
{
   EINA_SAFETY_ON_TRUE_RETURN_VAL(pd->filled, EINA_FALSE);
   Eina_Strbuf *command = eina_strbuf_new();
   unsigned int i = 0;

   _clear_command(pd);
   pd->command = eina_array_new(array ? eina_array_count(array) : 0);
   for (i = 0; i < (array ? eina_array_count(array) : 0); ++i)
     {
        char *content = eina_array_data_get(array, i);
        char *param = calloc(1, strlen(content) + 1);
        char *esc;

        if (!param)
          {
             while (eina_array_count(pd->command) > 0)
              eina_stringshare_del(eina_array_pop(pd->command));
             eina_array_free(pd->command);
             pd->command = NULL;
             for (;i < eina_array_count(array); ++i)
               {
                  content = eina_array_data_get(array, i);
                  eina_stringshare_del(content);
               }
             eina_array_free(array);
             eina_strbuf_free(command);
             return EINA_FALSE;
          }

        //build the command
        if (i != 0)
          eina_strbuf_append(command, " ");
        esc = _escape(content);
        eina_strbuf_append(command, esc);
        free(esc);
        //convert string to stringshare
        strcpy(param, content);
        _remove_invalid_chars(param);
        eina_array_push(pd->command, eina_stringshare_add(param));
        free(param);
        eina_stringshare_del(content);
     }
   pd->string_command = eina_strbuf_release(command);
   pd->filled = EINA_TRUE;
   eina_array_free(array);

   return EINA_TRUE;
}

/**
 * @brief Sets the command and its arguments from a single string.
 *
 * The input string is parsed to separate the command and its arguments,
 * handling quotes and escapes. Invalid characters in the string are replaced.
 *
 * @param[in] obj The Efl_Core_Command_Line object.
 * @param[in,out] pd The private data for the Efl_Core_Command_Line object.
 * @param[in] str The command string to set. For example, "my_command -o \"output file.txt\" --enable-feature".
 * @return EINA_TRUE on success, EINA_FALSE otherwise (e.g., if parsing fails).
 */
EOLIAN static Eina_Bool
_efl_core_command_line_command_string_set(Eo *obj EINA_UNUSED, Efl_Core_Command_Line_Data *pd, const char *str)
{
   EINA_SAFETY_ON_TRUE_RETURN_VAL(pd->filled, EINA_FALSE);

   pd->string_command = eina_strdup(str);

   if (pd->string_command)
     _remove_invalid_chars(pd->string_command);
   pd->command = _unescape(str);
   if (!pd->command)
     {
        if (pd->string_command)
          free(pd->string_command);
        pd->string_command = NULL;
        return EINA_FALSE;
     }
   pd->filled = EINA_TRUE;

   return EINA_TRUE;
}

EOLIAN static void
_efl_core_command_line_efl_object_destructor(Eo *obj EINA_UNUSED, Efl_Core_Command_Line_Data *pd)
{
   free(pd->string_command);
   pd->string_command = NULL;
   _clear_command(pd);
   efl_destructor(efl_super(obj, MY_CLASS));
}
#include "efl_core_command_line.eo.c"
