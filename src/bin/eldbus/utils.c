#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "codegen.h"
#include <ctype.h>

/**
 * @brief Reads the entire content of a file into a buffer.
 *
 * @param file_name The path to the file to be read.
 * @param buffer A pointer to a character pointer that will be allocated
 *               and filled with the file content. The caller is responsible
 *               for freeing this buffer.
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., file not found,
 *         memory allocation error).
 */
Eina_Bool
file_read(const char *file_name, char **buffer)
{
   FILE *xml_handler;
   int data; /* fgetc needs int to detect EOF correctly */
   Eina_Strbuf *buf;

   xml_handler = fopen(file_name, "rt");
   if (!xml_handler)
     {
        printf("Error to read file: %s\n", file_name);
        return EINA_FALSE;
     }
   buf = eina_strbuf_new();

   while ((data = fgetc(xml_handler)) != EOF)
     eina_strbuf_append_char(buf, (char)data);

   fclose(xml_handler);
   *buffer = eina_strbuf_string_steal(buf);
   eina_strbuf_free(buf);

   return EINA_TRUE;
}

/**
 * @brief Writes a buffer to a file.
 *
 * If output_dir is set (globally), the file will be created inside that directory.
 *
 * @param file_name The name of the file to be written.
 * @param buffer The content to write to the file.
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., cannot open file,
 *         write error).
 */
Eina_Bool
file_write(const char *file_name, const char *buffer)
{
   FILE *file_handler;
   const char *filename = file_name;
   Eina_Strbuf *fname = NULL;

   fname = eina_strbuf_new();

   if (output_dir)
     {
        eina_strbuf_append_printf(fname, "%s/%s", output_dir, file_name);
        filename = eina_strbuf_string_get(fname);
     }
   file_handler = fopen(filename, "wt");
   if (!file_handler)
     {
        printf("Error to write file: %s\n", filename);
        eina_strbuf_free(fname);
        return EINA_FALSE;
     }

   if (fwrite(buffer, strlen(buffer), 1, file_handler) < 1)
     {
        printf("Error writing to file: %s\n", filename);
     }
   fclose(file_handler);
   eina_strbuf_free(fname);

   return EINA_TRUE;
}

/**
 * @brief Converts a D-Bus name (interface or bus name) to a C-style identifier.
 *
 * This function transforms names like "org.example.Interface" or "/org/example/path"
 * into "org_example_interface" or "org_example_path". It handles dots and slashes
 * as separators, converts to lowercase, and prepends underscores before uppercase
 * letters in camelCase segments (e.g., "MyName" becomes "my_name").
 * Dashes are also replaced with underscores.
 *
 * @param dbus The D-Bus name string. For example, "org.freedesktop.DBus" or
 *             "/org/freedesktop/DBus/Local".
 * @return A newly allocated string with the C-style name. The caller is
 *         responsible for freeing this string. Returns "root" if the input
 *         is empty or only contains separators.
 */
char *
dbus_name_to_c(const char *dbus)
{
   char *str_cpy = strdup(dbus), *pch, *ret;
   Eina_Strbuf *buffer = eina_strbuf_new();
   unsigned i;

   pch = strtok(str_cpy, "/.");
   if (!pch)
     {
        ret = strdup("root");
        goto end;
     }
   eina_strbuf_append(buffer, pch);

   while ((pch = strtok(NULL, "/.")))
     eina_strbuf_append_printf(buffer, "_%s",pch);

   ret = eina_strbuf_string_steal(buffer);
   for (i = 0; ret[i]; i++)
     {
        if (i > 0 && ret[i-1] != '_' && ret[i] > '@' && ret[i] < '[')//upper case
          eina_strbuf_append_printf(buffer, "_%c", tolower(ret[i]));
        else
          eina_strbuf_append_char(buffer, tolower(ret[i]));
     }
   free(ret);
   eina_strbuf_replace_all(buffer, "-", "_");
   ret = eina_strbuf_string_steal(buffer);
end:
   free(str_cpy);
   eina_strbuf_free(buffer);
   return ret;
}

/**
 * @brief Replaces all occurrences of a substring within a string with another string.
 *
 * This function tokenizes the input string by the substring and rebuilds it
 * with the replacement string.
 *
 * @param string The original string.
 * @param substr The substring to be replaced. This is used as a delimiter set
 *               for strtok.
 * @param replacement The string to replace occurrences of substr.
 * @return A newly allocated string with replacements made. The caller is
 *         responsible for freeing this string. If substr is not found,
 *         a copy of the original string is returned.
 */
char *
replace_string(const char *string, const char *substr, const char *replacement)
{
   char *str_cpy = strdup(string);
   char *pch;
   char *ret;
   Eina_Strbuf *buffer = eina_strbuf_new();

   pch = strtok(str_cpy, substr);
   if (!pch)
     {
        eina_strbuf_free(buffer);
        return str_cpy;
     }
   eina_strbuf_append(buffer, pch);

   while ((pch = strtok(NULL, substr)))
     eina_strbuf_append_printf(buffer, "%s%s", replacement, pch);

   ret = eina_strbuf_string_steal(buffer);
   free(str_cpy);
   eina_strbuf_free(buffer);
   return ret;
}

/**
 * @brief Extracts a suffix from a string, starting after a specified number
 *        of occurrences of a character, counted from the end of the string.
 *
 * For example, if string is "a.b.c.d", break_in is '.', and amount is 1,
 * it returns "d". If amount is 2, it returns "c.d".
 *
 * @param string The input string.
 * @param break_in The character to count occurrences of.
 * @param amount The number of occurrences of break_in (from the end) to find
 *               before extracting the suffix.
 * @return A newly allocated string containing the suffix. If the specified
 *         number of `break_in` characters is not found, a copy of the
 *         original string is returned. The caller is responsible for freeing
 *         this string.
 */
char *
get_pieces(const char *string, char break_in, int amount)
{
   int i;
   int found = 0;

   for (i = strlen(string) - 1; i && amount > found; i--)
     if (string[i] == break_in)
       found++;

   if (found)
     return strdup(string+i+2);
   else
     return strdup(string);
}

/**
 * @brief Builds a string using a printf-style format and variable arguments.
 *
 * This is a convenience wrapper around eina_strbuf_prepend_vprintf.
 *
 * @param fmt The format string.
 * @param ... Variable arguments corresponding to the format string.
 * @return A newly allocated string with the formatted content. The caller is
 *         responsible for freeing this string.
 */
char *
string_build(const char *fmt, ...)
{
   va_list ap;
   Eina_Strbuf *buffer = eina_strbuf_new();
   char *ret;

   va_start(ap, fmt);
   eina_strbuf_prepend_vprintf(buffer, fmt, ap);
   va_end(ap);

   ret = eina_strbuf_string_steal(buffer);
   eina_strbuf_free(buffer);

   return ret;
}

#define UTIL_H "\
#ifndef ELDBUS_UTILS_H\n\
#define ELDBUS_UTILS_H 1\n\
\n\
typedef struct _Eldbus_Error_Info\n\
{\n\
   const char *error;\n\
   const char *message;\n\
} Eldbus_Error_Info;\n\
\n\
typedef void (*Eldbus_Codegen_Property_Set_Cb)(void *data, const char *propname, Eldbus_Proxy *proxy, Eldbus_Pending *p, Eldbus_Error_Info *error_info);\n\
\n\
typedef void (*Eldbus_Codegen_Property_String_Get_Cb)(void *data, Eldbus_Pending *p, const char *propname, Eldbus_Proxy *proxy, Eldbus_Error_Info *error_info, const char *value);\n\
typedef void (*Eldbus_Codegen_Property_Int32_Get_Cb)(void *data, Eldbus_Pending *p, const char *propname, Eldbus_Proxy *proxy, Eldbus_Error_Info *error_info, int value);\n\
typedef void (*Eldbus_Codegen_Property_Byte_Get_Cb)(void *data, Eldbus_Pending *p, const char *propname, Eldbus_Proxy *proxy, Eldbus_Error_Info *error_info, unsigned char value);\n\
typedef void (*Eldbus_Codegen_Property_Bool_Get_Cb)(void *data, Eldbus_Pending *p, const char *propname, Eldbus_Proxy *proxy, Eldbus_Error_Info *error_info, Eina_Bool value);\n\
typedef void (*Eldbus_Codegen_Property_Int16_Get_Cb)(void *data, Eldbus_Pending *p, const char *propname, Eldbus_Proxy *proxy, Eldbus_Error_Info *error_info, short int value);\n\
typedef void (*Eldbus_Codegen_Property_Uint16_Get_Cb)(void *data, Eldbus_Pending *p, const char *propname, Eldbus_Proxy *proxy, Eldbus_Error_Info *error_info, unsigned short int value);\n\
typedef void (*Eldbus_Codegen_Property_Uint32_Get_Cb)(void *data, Eldbus_Pending *p, const char *propname, Eldbus_Proxy *proxy, Eldbus_Error_Info *error_info, unsigned int value);\n\
typedef void (*Eldbus_Codegen_Property_Double_Get_Cb)(void *data, Eldbus_Pending *p, const char *propname, Eldbus_Proxy *proxy, Eldbus_Error_Info *error_info, double value);\n\
typedef void (*Eldbus_Codegen_Property_Int64_Get_Cb)(void *data, Eldbus_Pending *p, const char *propname, Eldbus_Proxy *proxy, Eldbus_Error_Info *error_info, int64_t value);\n\
typedef void (*Eldbus_Codegen_Property_Uint64_Get_Cb)(void *data, Eldbus_Pending *p, const char *propname, Eldbus_Proxy *proxy, Eldbus_Error_Info *error_info, uint64_t value);\n\
typedef void (*Eldbus_Codegen_Property_Complex_Get_Cb)(void *data, Eldbus_Pending *p, const char *propname, Eldbus_Proxy *proxy, Eldbus_Error_Info *error_info, Eina_Value *value);\n\
\n\
#endif\
"

/**
 * @brief Writes the predefined content of eldbus_utils.h to a file.
 *
 * The content is defined in the UTIL_H macro.
 *
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
Eina_Bool
util_h_write(void)
{
   return file_write("eldbus_utils.h", UTIL_H);
}
