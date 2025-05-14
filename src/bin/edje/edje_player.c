#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>

#include <Evas.h>
#include <Ecore.h>
#include <Ecore_Getopt.h>
#include <Ecore_Evas.h>
#include <Edje.h>

/**
 * @struct opts
 * @brief Holds all the command-line options for the player.
 *
 * This structure is used to store the values of the command-line
 * arguments parsed by Ecore_Getopt.
 */
struct opts
{
   char          *file; /**< Path to the Edje file. */
   char          *group; /**< Name of the group to load from the Edje file. */
   Eina_Bool      list_groups; /**< If true, list groups in the file and exit. */
   char          *engine; /**< Ecore_Evas engine to use. */
   Eina_Rectangle size; /**< Initial window size. */
   unsigned char  color[3]; /**< Background color as {R, G, B}. */
   Eina_Bool      borderless; /**< If true, create a borderless window. */
   Eina_Bool      sticky; /**< If true, create a sticky window. */
   Eina_Bool      shaped; /**< If true, create a shaped window. */
   Eina_Bool      alpha; /**< If true, create a window with an alpha channel. */
   Eina_Bool      print; /**< If true, print signals and messages to stdout. */
   Eina_Bool      slave_mode; /**< If true, enable slave mode, reading commands from stdin. */
   double         scale; /**< Global scaling factor for Edje. */
   int            pad; /**< Padding around the Edje object. */
   char          *title; /**< Custom window title. */
};

static Eina_Bool _edje_load_or_show_error(Evas_Object *edje, const char *file, const char *group);

static Ecore_Evas *win;
static Evas *evas;
static Evas_Object *bg, *bg2 = NULL, *edje;
static struct opts opts;

/**
 * @brief Sets the window title based on the loaded group and file.
 *
 * The title will be in the format "Edje_Player - <group> of <file>".
 *
 * @param group The name of the loaded Edje group.
 * @param file The path to the Edje file.
 */
static void
_win_title_set(const char *group, const char *file)
{
   char buf[1024];
   snprintf(buf, sizeof(buf), "Edje_Player - %s of %s", group, file);
   ecore_evas_title_set(win, buf);
}

/**
 * @brief Tokenizes a string for slave mode command parsing.
 *
 * This function extracts the next token from the string pointed to by @p p_arg.
 * A token is a sequence of non-whitespace characters, or a sequence of
 * characters enclosed in double quotes.
 *
 * It modifies @p p_arg to point to the beginning of the extracted token, and
 * the character after the token is null-terminated.
 *
 * @param p_arg A pointer to a string pointer. On entry, it points to the
 *              string to be tokenized. On exit, it's updated to point
 *              to the start of the token within the original string.
 * @return A pointer to the character immediately following the extracted token,
 *         or NULL if no more tokens are found or an error occurs.
 *         The returned pointer can be used for subsequent calls to find
 *         the next token.
 *
 * @note This function modifies the input string by inserting null terminators.
 */
static char *
_slave_mode_tok(char **p_arg)
{
   char *s, *e;
   Eina_Bool is_quoted;

   if (!*p_arg) return NULL;

   s = *p_arg;
   while (isspace(*s))
     s++;

   if (*s == '\0')
     {
        *p_arg = NULL;
        return NULL;
     }
   else if (*s == '"')
     {
        is_quoted = EINA_TRUE;
        s++;
        *p_arg = s;
     }
   else
     {
        is_quoted = EINA_FALSE;
        *p_arg = s;
     }

   for (e = s; *e != '\0'; e++)
     {
        if ((!is_quoted) && (isspace(*e)))
          break;
        else if ((is_quoted) && (*e == '"'))
          break;
     }

   if (*e == '\0') return NULL;

   *e = '\0';
   return e + 1;
}

/**
 * @brief Handles the "signal" command in slave mode.
 *
 * Parses emission and source from @p args and emits an Edje signal.
 * The arguments are expected to be two tokens: emission and source.
 *
 * @param edje The Edje object to send the signal to.
 * @param args A string containing the arguments for the signal command.
 *        Example: `"my_signal" "my_source"`
 */
static void
_slave_mode_signal(Evas_Object *edje, char *args)
{
   char *emission, *source;

   emission = args;
   source = _slave_mode_tok(&emission);
   _slave_mode_tok(&source);

   if ((!emission) || (!source))
     {
        fputs("ERROR: Invalid command arguments.\n", stderr);
        return;
     }

   edje_object_signal_emit(edje, emission, source);
}

/**
 * @brief Sends an EDJE_MESSAGE_STRING to an Edje object.
 *
 * @param edje The Edje object.
 * @param id The message ID.
 * @param arg The string to send.
 */
static void
_slave_mode_message_string(Evas_Object *edje, int id, char *arg)
{
   Edje_Message_String msg;
   msg.str = arg;
   edje_object_message_send(edje, EDJE_MESSAGE_STRING, id, &msg);
}

/**
 * @brief Sends an EDJE_MESSAGE_INT to an Edje object.
 *
 * @param edje The Edje object.
 * @param id The message ID.
 * @param arg A string representation of the integer to send.
 */
static void
_slave_mode_message_int(Evas_Object *edje, int id, char *arg)
{
   Edje_Message_Int msg;
   msg.val = atoi(arg);
   edje_object_message_send(edje, EDJE_MESSAGE_INT, id, &msg);
}

/**
 * @brief Sends an EDJE_MESSAGE_FLOAT to an Edje object.
 *
 * @param edje The Edje object.
 * @param id The message ID.
 * @param arg A string representation of the float to send.
 */
static void
_slave_mode_message_float(Evas_Object *edje, int id, char *arg)
{
   Edje_Message_Float msg;
   msg.val = eina_convert_strtod_c(arg, NULL);
   edje_object_message_send(edje, EDJE_MESSAGE_FLOAT, id, &msg);
}

/**
 * @brief Sends an EDJE_MESSAGE_STRING_SET to an Edje object.
 *
 * The message consists of a count and an array of strings.
 *
 * @param edje The Edje object.
 * @param id The message ID.
 * @param arg A string representation of the number of strings in the set.
 * @param extra_args A string containing the space-separated strings of the set.
 *        Example: `"string1" "string2" ...`
 */
static void
_slave_mode_message_string_set(Evas_Object *edje, int id, char *arg,
                               char *extra_args)
{
   Edje_Message_String_Set *msg;
   int count, i;

   count = atoi(arg);
   msg = alloca(sizeof(Edje_Message_String_Set) + (count - 1) * sizeof(char *));

   for (i = 0; i < count; i++)
     {
        char *next = _slave_mode_tok(&extra_args);
        if (!extra_args)
          {
             fputs("ERROR: Message missing arg.\n", stderr);
             return;
          }
        msg->str[i] = extra_args;
        extra_args = next;
     }

   msg->count = count;
   edje_object_message_send(edje, EDJE_MESSAGE_STRING_SET, id, msg);
}

/**
 * @brief Sends an EDJE_MESSAGE_INT_SET to an Edje object.
 *
 * The message consists of a count and an array of integers.
 *
 * @param edje The Edje object.
 * @param id The message ID.
 * @param arg A string representation of the number of integers in the set.
 * @param extra_args A string containing the space-separated integers of the set.
 *        Example: `123 456 ...`
 */
static void
_slave_mode_message_int_set(Evas_Object *edje, int id, char *arg,
                            char *extra_args)
{
   Edje_Message_Int_Set *msg;
   int count, i;

   count = atoi(arg);
   msg = alloca(sizeof(Edje_Message_Int_Set) + (count - 1) * sizeof(int));

   for (i = 0; i < count; i++)
     {
        char *next = _slave_mode_tok(&extra_args);
        if (!extra_args)
          {
             fputs("ERROR: Message missing arg.\n", stderr);
             return;
          }
        msg->val[i] = atoi(extra_args);
        extra_args = next;
     }

   msg->count = count;
   edje_object_message_send(edje, EDJE_MESSAGE_INT_SET, id, msg);
}

/**
 * @brief Sends an EDJE_MESSAGE_FLOAT_SET to an Edje object.
 *
 * The message consists of a count and an array of floats.
 *
 * @param edje The Edje object.
 * @param id The message ID.
 * @param arg A string representation of the number of floats in the set.
 * @param extra_args A string containing the space-separated floats of the set.
 *        Example: `1.23 4.56 ...`
 */
static void
_slave_mode_message_float_set(Evas_Object *edje, int id, char *arg,
                              char *extra_args)
{
   Edje_Message_Float_Set *msg;
   int count, i;

   count = atoi(arg);
   msg = alloca(sizeof(Edje_Message_Float_Set) + (count - 1) * sizeof(double));

   for (i = 0; i < count; i++)
     {
        char *next = _slave_mode_tok(&extra_args);
        if (!extra_args)
          {
             fputs("ERROR: Message missing arg.\n", stderr);
             return;
          }
        msg->val[i] = eina_convert_strtod_c(extra_args, NULL);
        extra_args = next;
     }

   msg->count = count;
   edje_object_message_send(edje, EDJE_MESSAGE_FLOAT_SET, id, msg);
}

/**
 * @brief Sends an EDJE_MESSAGE_STRING_INT to an Edje object.
 *
 * The message consists of a string and an integer.
 *
 * @param edje The Edje object.
 * @param id The message ID.
 * @param arg The string part of the message.
 * @param extra_args A string representation of the integer part of the message.
 */
static void
_slave_mode_message_string_int(Evas_Object *edje, int id, char *arg,
                               char *extra_args)
{
   Edje_Message_String_Int msg;

   if (!extra_args)
     {
        fputs("ERROR: Message STRING_INT requires integer arg.\n", stderr);
        return;
     }
   _slave_mode_tok(&extra_args);

   msg.str = arg;
   msg.val = atoi(extra_args);

   edje_object_message_send(edje, EDJE_MESSAGE_STRING_INT, id, &msg);
}

/**
 * @brief Sends an EDJE_MESSAGE_STRING_FLOAT to an Edje object.
 *
 * The message consists of a string and a float.
 *
 * @param edje The Edje object.
 * @param id The message ID.
 * @param arg The string part of the message.
 * @param extra_args A string representation of the float part of the message.
 */
static void
_slave_mode_message_string_float(Evas_Object *edje, int id, char *arg,
                                 char *extra_args)
{
   Edje_Message_String_Float msg;

   if (!extra_args)
     {
        fputs("ERROR: Message STRING_FLOAT requires float arg.\n", stderr);
        return;
     }
   _slave_mode_tok(&extra_args);

   msg.str = arg;
   msg.val = eina_convert_strtod_c(extra_args, NULL);

   edje_object_message_send(edje, EDJE_MESSAGE_STRING_FLOAT, id, &msg);
}

/**
 * @brief Sends an EDJE_MESSAGE_STRING_INT_SET to an Edje object.
 *
 * The message consists of a string, a count, and an array of integers.
 *
 * @param edje The Edje object.
 * @param id The message ID.
 * @param arg The string part of the message.
 * @param extra_args A string containing the count followed by space-separated
 *                   integers. Example: `3 1 2 3`
 */
static void
_slave_mode_message_string_int_set(Evas_Object *edje, int id, char *arg,
                                   char *extra_args)
{
   Edje_Message_String_Int_Set *msg;
   int count, i;
   char *val;

   if (!extra_args)
     {
        fputs("ERROR: Message STRING_INT_SET requires int args.\n", stderr);
        return;
     }

   val = _slave_mode_tok(&extra_args);
   count = atoi(extra_args);
   msg = alloca(sizeof(Edje_Message_String_Int_Set) +
                (count - 1) * sizeof(int));

   for (i = 0; i < count; i++)
     {
        char *next = _slave_mode_tok(&val);
        if (!val)
          {
             fputs("ERROR: Message missing arg.\n", stderr);
             return;
          }
        msg->val[i] = atoi(val);
        val = next;
     }

   msg->count = count;
   msg->str = arg;
   edje_object_message_send(edje, EDJE_MESSAGE_STRING_INT_SET, id, msg);
}

/**
 * @brief Sends an EDJE_MESSAGE_STRING_FLOAT_SET to an Edje object.
 *
 * The message consists of a string, a count, and an array of floats.
 *
 * @param edje The Edje object.
 * @param id The message ID.
 * @param arg The string part of the message.
 * @param extra_args A string containing the count followed by space-separated
 *                   floats. Example: `2 1.5 2.5`
 */
static void
_slave_mode_message_string_float_set(Evas_Object *edje, int id, char *arg,
                                     char *extra_args)
{
   Edje_Message_String_Float_Set *msg;
   int count, i;
   char *val;

   if (!extra_args)
     {
        fputs("ERROR: Message STRING_FLOAT_SET requires float set.\n", stderr);
        return;
     }

   val = _slave_mode_tok(&extra_args);
   count = atoi(extra_args);
   msg = alloca(sizeof(Edje_Message_String_Float_Set) +
                (count - 1) * sizeof(double));

   for (i = 0; i < count; i++)
     {
        char *next = _slave_mode_tok(&val);
        if (!val)
          {
             fputs("ERROR: Message missing arg.\n", stderr);
             return;
          }
        msg->val[i] = eina_convert_strtod_c(val, NULL);
        val = next;
     }

   msg->count = count;
   msg->str = arg;
   edje_object_message_send(edje, EDJE_MESSAGE_STRING_FLOAT_SET, id, msg);
}

/**
 * @brief Handles the "message" command in slave mode.
 *
 * Parses the message type and arguments, and calls the appropriate
 * function to send the message to the Edje object.
 *
 * @param edje The Edje object.
 * @param args A string containing the message arguments.
 *        Format: `<id> <type> <args...>`
 *        Example: `1 STRING "hello"`
 */
static void
_slave_mode_message(Evas_Object *edje, char *args)
{
   char *id_str, *type, *message_arg, *extra_args;
   int id;

   id_str = args;
   type = _slave_mode_tok(&id_str);
   message_arg = _slave_mode_tok(&type);
   extra_args = _slave_mode_tok(&message_arg);

   if (!id_str)
     {
        fputs("ERROR: Message id is required.\n", stderr);
        return;
     }

   id = atoi(id_str);

   if (!type)
     {
        fputs("ERROR: Message type is required.\n", stderr);
        return;
     }

   if (!message_arg)
     {
        fputs("ERROR: Missing message argument.\n", stderr);
        return;
     }

   if (!strcmp(type, "STRING"))
     {
        _slave_mode_message_string(edje, id, message_arg);
        return;
     }

   if (!strcmp(type, "INT"))
     {
        _slave_mode_message_int(edje, id, message_arg);
        return;
     }

   if (!strcmp(type, "FLOAT"))
     {
        _slave_mode_message_float(edje, id, message_arg);
        return;
     }

   if (!strcmp(type, "STRING_SET"))
     {
        _slave_mode_message_string_set(edje, id, message_arg, extra_args);
        return;
     }

   if (!strcmp(type, "INT_SET"))
     {
        _slave_mode_message_int_set(edje, id, message_arg, extra_args);
        return;
     }

   if (!strcmp(type, "FLOAT_SET"))
     {
        _slave_mode_message_float_set(edje, id, message_arg, extra_args);
        return;
     }

   if (!strcmp(type, "STRING_INT"))
     {
        _slave_mode_message_string_int(edje, id, message_arg, extra_args);
        return;
     }

   if (!strcmp(type, "STRING_FLOAT"))
     {
        _slave_mode_message_string_float(edje, id, message_arg, extra_args);
        return;
     }

   if (!strcmp(type, "STRING_INT_SET"))
     {
        _slave_mode_message_string_int_set(edje, id, message_arg, extra_args);
        return;
     }

   if (!strcmp(type, "STRING_FLOAT_SET"))
     {
        _slave_mode_message_string_float_set(edje, id, message_arg, extra_args);
        return;
     }

   fputs("ERROR: Invalid type. Check types list using \"help\".\n", stderr);
}

/**
 * @brief Handles the "info" command in slave mode.
 *
 * Prints geometry and state information about a given part.
 *
 * @param edje The Edje object.
 * @param args A string containing the part name.
 */
static void
_slave_mode_info(Evas_Object *edje, char *args)
{
   _slave_mode_tok(&args);

   if (!args)
     {
        fputs("ERROR: Invalid command arguments.\n", stderr);
        return;
     }

   if (!edje_object_part_exists(edje, args))
     {
        printf("INFO: \"%s\" does not exist.\n", args);
     }
   else
     {
        Evas_Coord x, y, w, h;
        edje_object_part_geometry_get(edje, args, &x, &y, &w, &h);
        printf("INFO: \"%s\" %d,%d,%d,%d\n", args, x, y, w, h);
     }
}

/**
 * @brief Handles the "text" command in slave mode.
 *
 * Sets the text of a given part in the Edje object.
 *
 * @param edje The Edje object.
 * @param args A string containing the part name and the text to set,
 *        separated by a space. The text can be quoted.
 *        Example: `my_text_part "Some new text"`
 */
static void
_slave_mode_text(Evas_Object *edje, char *args)
{
   char *part, *text, *p;

   if (!args) return;
   p = strchr(args, ' ');
   if (!p) return;
   part = malloc(p - args + 1);
   strncpy(part, args, p - args);
   part[p - args] = 0;
   text = p + 1;
   edje_object_part_text_set(edje, part, text);
   free(part);
}

/**
 * @brief Handles the "quit" command in slave mode.
 *
 * Exits the main loop, terminating the application.
 *
 * @param edje Unused.
 * @param args Unused.
 */
static void
_slave_mode_quit(Evas_Object *edje EINA_UNUSED, char *args EINA_UNUSED)
{
   puts("Bye!");
   ecore_main_loop_quit();
}

/**
 * @brief Handles the "help" command in slave mode.
 *
 * Prints a help message detailing the available commands to stdout.
 *
 * @param edje Unused.
 * @param args Unused.
 */
static void
_slave_mode_help(Evas_Object *edje EINA_UNUSED, char *args EINA_UNUSED)
{
   puts("Help:\n"
        "One command per line, arguments separated by space.\n"
        "Strings may have spaces if enclosed in quotes (\").\n"
        "\n"
        "\t<command> [arguments]\n"
        "\n"
        "Available commands:\n"
        "\tsignal <emission> <source>\n"
        "\t   sends a signal to edje\n"
        "\tmessage <id> <type> <args list>\n"
        "\t   sends a message to edje.\n"
        "\t   Possible types (and args):\n"
        "\t     * STRING \"string\"\n"
        "\t     * INT integer\n"
        "\t     * FLOAT float\n"
        "\t     * STRING_SET <set length> \"string1\" \"string2\" ...\n"
        "\t     * INT_SET <set length> integer1 integer2 integer3 ...\n"
        "\t     * FLOAT_SET <set length> float1 float2 float3 ...\n"
        "\t     * STRING_INT \"message\" integer\n"
        "\t     * STRING_FLOAT \"message\" float\n"
        "\t     * STRING_INT_SET \"string\" <set length> integer1 ...\n"
        "\t     * STRING_FLOAT_SET \"string\" <set length> float1 float2 ...\n"
        "\tinfo <part>\n"
        "\t   Print part geometry: <x>,<y>,<w>,<h>\n"
        "\ttext <part> <text string>\n"
        "\t   Set text of named part\n"
        "\tquit\n"
        "\t   exit edje player.\n"
        "\thelp\n"
        "\t   shows this message.\n");
   /*
    * Extension ideas (are they useful?):
    *  - data: show data value
    *  - color_class: set color class values (maybe also list?)
    *  - text_class: set text class values (maybe also list?)
    *  - play_set: change play state
    *  - animation_set: change animation state
    */
}

/**
 * @struct slave_cmd
 * @brief Associates a slave mode command string with its handler function.
 */
struct slave_cmd
{
   const char *cmd; /**< The command string (e.g., "signal", "message"). */
   void        (*func)(Evas_Object *edje, char *args); /**< Pointer to the handler function. */
} /** @brief Array of available slave mode commands. */
_slave_mode_commands[] = {
   {"signal", _slave_mode_signal},
   {"message", _slave_mode_message},
   {"info", _slave_mode_info},
   {"text", _slave_mode_text},
   {"quit", _slave_mode_quit},
   {"help", _slave_mode_help},
   {NULL, NULL}
};

#ifndef _WIN32
/**
 * @brief Ecore_Fd_Handler callback for slave mode.
 *
 * Reads commands from stdin, parses them, and executes them. This function
 * is called by the main loop when there is data to be read from stdin.
 *
 * @param data The Edje object.
 * @param fd_handler The Ecore file descriptor handler.
 * @return ECORE_CALLBACK_RENEW to continue listening, or ECORE_CALLBACK_CANCEL
 *         to stop.
 */
static Eina_Bool
_slave_mode(void *data, Ecore_Fd_Handler *fd_handler)
{
   Evas_Object *edje = data;
   char buf[1024], *p;
   const struct slave_cmd *itr;
   size_t len;

   if (ecore_main_fd_handler_active_get(fd_handler, ECORE_FD_ERROR))
     {
        fputs("ERROR: error on stdin! Exit.\n", stderr);
        ecore_main_loop_quit();
        return ECORE_CALLBACK_CANCEL;
     }
   if (!ecore_main_fd_handler_active_get(fd_handler, ECORE_FD_READ))
     return ECORE_CALLBACK_RENEW;

   if (!fgets(buf, sizeof(buf), stdin))
     {
        fputs("ERROR: end of stdin! Exit.\n", stderr);
        ecore_main_loop_quit();
        return ECORE_CALLBACK_CANCEL;
     }

   len = strlen(buf);
   if (len < 1)
     {
        fputs("ERROR: no input! Try: help\n", stderr);
        return ECORE_CALLBACK_RENEW;
     }
   if (buf[len - 1] == '\n')
     {
        len--;
        buf[len] = '\0';
     }

   p = strchr(buf, ' ');
   if (p)
     {
        *p = '\0';
        p++;

        while (isspace(*p))
          p++;
        if (*p == '\0')
          p = NULL;

        if (p)
          {
             char *q = p + strlen(p) - 1;
             while (isspace(*q))
               {
                  *q = '\0';
                  q--;
               }
          }
     }

   for (itr = _slave_mode_commands; itr->cmd; itr++)
     {
        if (strcasecmp(itr->cmd, buf) == 0)
          {
             itr->func(edje, p);
             break;
          }
     }

   return ECORE_CALLBACK_RENEW;
}

#endif

/**
 * @brief Callback to print signals received from the Edje object.
 *
 * This function is registered as a signal callback when the --print
 * option is used. It prints the emission and source of any signal.
 *
 * @param data Unused user data.
 * @param o Unused Evas_Object that emitted the signal.
 * @param emission The signal's emission string.
 * @param source The signal's source string.
 */
static void
_print_signal(void *data EINA_UNUSED, Evas_Object *o EINA_UNUSED, const char *emission, const char *source)
{
   printf("SIGNAL: \"%s\" \"%s\"\n", emission, source);
}

/**
 * @brief Callback to print messages received from the Edje object.
 *
 * This function is set as the message handler when the --print option
 * is used. It decodes and prints the content of various message types.
 *
 * @param data Unused user data.
 * @param edje Unused Edje object that received the message.
 * @param type The type of the message.
 * @param id The ID of the message.
 * @param msg A pointer to the message data structure.
 */
static void
_print_message(void *data EINA_UNUSED, Evas_Object *edje EINA_UNUSED, Edje_Message_Type type, int id, void *msg)
{
   const char *typestr;
   char buf[64];

   switch (type)
     {
      case EDJE_MESSAGE_NONE:
        typestr = "NONE";
        break;

      case EDJE_MESSAGE_SIGNAL:
        typestr = "SIGNAL";
        break;

      case EDJE_MESSAGE_STRING:
        typestr = "STRING";
        break;

      case EDJE_MESSAGE_INT:
        typestr = "INT";
        break;

      case EDJE_MESSAGE_FLOAT:
        typestr = "FLOAT";
        break;

      case EDJE_MESSAGE_STRING_SET:
        typestr = "STRING_SET";
        break;

      case EDJE_MESSAGE_INT_SET:
        typestr = "INT_SET";
        break;

      case EDJE_MESSAGE_FLOAT_SET:
        typestr = "FLOAT_SET";
        break;

      case EDJE_MESSAGE_STRING_INT:
        typestr = "STRING_INT";
        break;

      case EDJE_MESSAGE_STRING_FLOAT:
        typestr = "STRING_FLOAT";
        break;

      case EDJE_MESSAGE_STRING_INT_SET:
        typestr = "STRING_INT_SET";
        break;

      case EDJE_MESSAGE_STRING_FLOAT_SET:
        typestr = "STRING_FLOAT_SET";
        break;

      default:
        snprintf(buf, sizeof(buf), "UNKNOWN(%d)", type);
        typestr = buf;
     }

   printf("MESSAGE: type=%s, id=%d", typestr, id);

   switch (type)
     {
      case EDJE_MESSAGE_NONE: break;

      case EDJE_MESSAGE_SIGNAL: break;

      case EDJE_MESSAGE_STRING:
      {
         Edje_Message_String *m = msg;
         printf(" \"%s\"", m->str);
      }
      break;

      case EDJE_MESSAGE_INT:
      {
         Edje_Message_Int *m = msg;
         printf(" %d", m->val);
      }
      break;

      case EDJE_MESSAGE_FLOAT:
      {
         Edje_Message_Float *m = msg;
         printf(" %f", m->val);
      }
      break;

      case EDJE_MESSAGE_STRING_SET:
      {
         Edje_Message_String_Set *m = msg;
         int i;
         for (i = 0; i < m->count; i++)
           printf(" \"%s\"", m->str[i]);
      }
      break;

      case EDJE_MESSAGE_INT_SET:
      {
         Edje_Message_Int_Set *m = msg;
         int i;
         for (i = 0; i < m->count; i++)
           printf(" %d", m->val[i]);
      }
      break;

      case EDJE_MESSAGE_FLOAT_SET:
      {
         Edje_Message_Float_Set *m = msg;
         int i;
         for (i = 0; i < m->count; i++)
           printf(" %f", m->val[i]);
      }
      break;

      case EDJE_MESSAGE_STRING_INT:
      {
         Edje_Message_String_Int *m = msg;
         printf(" \"%s\" %d", m->str, m->val);
      }
      break;

      case EDJE_MESSAGE_STRING_FLOAT:
      {
         Edje_Message_String_Float *m = msg;
         printf(" \"%s\" %f", m->str, m->val);
      }
      break;

      case EDJE_MESSAGE_STRING_INT_SET:
      {
         Edje_Message_String_Int_Set *m = msg;
         int i;
         printf(" \"%s\"", m->str);
         for (i = 0; i < m->count; i++)
           printf(" %d", m->val[i]);
      }
      break;

      case EDJE_MESSAGE_STRING_FLOAT_SET:
      {
         Edje_Message_String_Float_Set *m = msg;
         int i;
         printf(" \"%s\"", m->str);
         for (i = 0; i < m->count; i++)
           printf(" %f", m->val[i]);
      }
      break;

      default:
        break;
     }

   putchar('\n');
}

/**
 * @brief Evas key down event callback.
 *
 * Handles key presses for scaling the Edje object.
 * - '=' or '+': Increase scale.
 * - '-' or '_': Decrease scale.
 * - '0': Reset scale to 1.0.
 *
 * @param data Unused user data.
 * @param e Unused Evas canvas.
 * @param obj Unused Evas object.
 * @param event_info The key down event information.
 */
static void
_key_down(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Evas_Event_Key_Down *ev = event_info;

   if ((!strcmp(ev->keyname, "equal")) ||
       (!strcmp(ev->keyname, "plus")))
     opts.scale += 0.1;
   else if ((!strcmp(ev->keyname, "minus")) ||
            (!strcmp(ev->keyname, "underscore")))
     opts.scale -= 0.1;
   else if ((!strcmp(ev->keyname, "0")))
     opts.scale = 1.0;
   if (opts.scale < 0.1) opts.scale = 0.1;
   else if (opts.scale > 10.0)
     opts.scale = 10.0;
   edje_scale_set(opts.scale);
}

/**
 * @brief Creates a background rectangle object.
 *
 * The background is filled with the color specified by the command-line
 * options.
 *
 * @param show If EINA_TRUE, show the object after creation.
 * @return The newly created background object, or NULL on failure.
 */
static Evas_Object *
_create_bg(Eina_Bool show)
{
   const unsigned char *color = opts.color;
   Evas_Object *o = evas_object_rectangle_add(evas);
   if (!o)
     {
        fputs("ERROR: could not create background.\n", stderr);
        return NULL;
     }
   evas_object_color_set(o, color[0], color[1], color[2], 255);
   if (show) evas_object_show(o);
   return o;
}

/**
 * @brief Callback for the "edje,change,file" signal.
 *
 * This is triggered when the source .edj file is modified. It reloads the
 * current group from the file.
 *
 * @param data Unused user data.
 * @param obj The Edje object that emitted the signal.
 * @param emission Unused emission string.
 * @param source Unused source string.
 */
static void
_edje_reload(void *data EINA_UNUSED, Evas_Object *obj, const char *emission EINA_UNUSED, const char *source EINA_UNUSED)
{
   const char *file;
   const char *group;
   edje_object_signal_callback_del(obj, "edje,change,file", "edje", _edje_reload);
   edje_object_file_get(obj, &file, &group);
   _edje_load_or_show_error(obj, file, group);
}

/**
 * @brief Callback for circular dependency errors in Edje.
 *
 * Edje emits this smart event if it detects a circular dependency among
 * parts. This function logs the error to stderr.
 *
 * @param data The group name (as a string).
 * @param obj Unused Edje object.
 * @param event_info An Eina_List of part names involved in the cycle.
 */
static void
_edje_circul(void *data, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   char buf[1024] = "";
   Eina_List *parts = event_info;
   Eina_List *l;
   char *part_name;
   char *group = data;

   part_name = eina_list_data_get(eina_list_last(parts));
   strncpy(buf, part_name, sizeof(buf) - 1);
   part_name[sizeof(buf) - 1] = 0;
   EINA_LIST_FOREACH(parts, l, part_name)
     {
        strncat(buf, " -> ", sizeof(buf) - strlen(buf) - 1);
        part_name[sizeof(buf) - 1] = 0;
        strncat(buf, part_name, sizeof(buf) - strlen(buf) - 1);
        part_name[sizeof(buf) - 1] = 0;
     }

   fprintf(stderr, "Group '%s' have a circul dependency between parts: %s\n",
           group, buf);
}

/**
 * @brief Loads an Edje file and group into an Edje object.
 *
 * If loading fails, it prints a detailed error message to stderr.
 *
 * @param edje The Edje object to load into.
 * @param file The path to the .edj file.
 * @param group The name of the group to load.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
_edje_load_or_show_error(Evas_Object *edje, const char *file, const char *group)
{
   Eina_File *f = NULL;
   const char *errmsg;
   int err;

   f = eina_file_open(file, EINA_FALSE);
   if (edje_object_file_set(edje, file, group))
     {
        edje_object_signal_callback_add(edje, "edje,change,file", "edje", _edje_reload, NULL);
        evas_object_focus_set(edje, EINA_TRUE);
        eina_file_close(f);
        return EINA_TRUE;
     }

   err = edje_object_load_error_get(edje);
   errmsg = edje_load_error_str(err);
   eina_file_close(f);
   fprintf(stderr, "ERROR: could not load edje file '%s', group '%s': %s\n",
           file, group, errmsg);
   return EINA_FALSE;
}

/**
 * @brief Creates and initializes the main Edje object.
 *
 * This function creates the Edje object, determines which group to load
 * (based on command-line options or defaults), loads it, and sets up
 * various properties and callbacks.
 *
 * @return The newly created and configured Edje object, or NULL on failure.
 */
static Evas_Object *
_create_edje(void)
{
   Evas_Coord minw, minh, maxw, maxh;
   Evas_Object *o = edje_object_add(evas);
   if (!o)
     {
        fputs("ERROR: could not create edje.\n", stderr);
        return NULL;
     }

   if (opts.group)
     {
        if (!_edje_load_or_show_error(o, opts.file, opts.group))
          {
             evas_object_del(edje);
             return NULL;
          }
        if (!opts.title) _win_title_set(opts.group, opts.file);
     }
   else
     {
        if (edje_file_group_exists(opts.file, "main"))
          {
             if (!_edje_load_or_show_error(o, opts.file, "main"))
               {
                  evas_object_del(edje);
                  return NULL;
               }
             if (!opts.title) _win_title_set("main", opts.file);
          }
        else
          {
             Eina_List *groups = edje_file_collection_list(opts.file);
             const char *group;
             if (!groups)
               {
                  fprintf(stderr, "ERROR: file '%s' has no groups!\n",
                          opts.file);
                  evas_object_del(edje);
                  return NULL;
               }
             group = groups->data;
             if (!_edje_load_or_show_error(o, opts.file, group))
               {
                  edje_file_collection_list_free(groups);
                  evas_object_del(edje);
                  return NULL;
               }
             if (!opts.title) _win_title_set(group, opts.file);
             edje_file_collection_list_free(groups);
          }
     }
   evas_object_smart_callback_add(o, "circular,dependency", _edje_circul, opts.group);

   edje_object_size_max_get(o, &maxw, &maxh);
   edje_object_size_min_get(o, &minw, &minh);
   if ((minw <= 0) && (minh <= 0))
     edje_object_size_min_calc(o, &minw, &minh);

   ecore_evas_size_max_set(win,
                           maxw > 0 ? (maxw + opts.pad * 2) : 0,
                           maxh > 0 ? (maxh + opts.pad * 2) : 0);
   ecore_evas_size_min_set(win, (minw + opts.pad * 2), (minh + opts.pad * 2));

   evas_object_show(o);

   return o;
}

/**
 * @brief Ecore_Getopt callback to parse a color string.
 *
 * Parses a color string in "R,G,B" format (e.g., "255,128,0") and
 * stores the result in the provided storage.
 *
 * @param parser Unused.
 * @param desc Unused.
 * @param str The string to parse.
 * @param data Unused.
 * @param storage The Ecore_Getopt_Value where the parsed color (as an
 *                unsigned char[3]) will be stored.
 * @return 1 on success, 0 on failure.
 */
static unsigned char
_parse_color(EINA_UNUSED const Ecore_Getopt *parser, EINA_UNUSED const Ecore_Getopt_Desc *desc, const char *str, EINA_UNUSED void *data, Ecore_Getopt_Value *storage)
{
   unsigned char *color = (unsigned char *)storage->ptrp;

   if (sscanf(str, "%hhu,%hhu,%hhu", color, color + 1, color + 2) != 3)
     {
        fprintf(stderr, "ERROR: incorrect color value '%s'\n", str);
        return 0;
     }

   return 1;
}

/**
 * @brief Ecore_Evas delete request callback.
 *
 * Called when the user tries to close the window. It quits the main loop.
 *
 * @param ee Unused Ecore_Evas instance.
 */
static void
_cb_delete(EINA_UNUSED Ecore_Evas *ee)
{
   ecore_main_loop_quit();
}

/**
 * @brief Ecore_Evas resize callback.
 *
 * Called when the window is resized. It resizes the internal objects
 * (background and Edje object) to fit the new window dimensions,
 * respecting any padding.
 *
 * @param ee The Ecore_Evas instance that was resized.
 */
static void
_cb_resize(Ecore_Evas *ee)
{
   int w, h;
   ecore_evas_geometry_get(ee, NULL, NULL, &w, &h);
   evas_object_move(edje, opts.pad, opts.pad);
   evas_object_resize(edje, w - (opts.pad * 2), h - (opts.pad * 2));
   evas_object_move(bg, opts.pad, opts.pad);
   evas_object_resize(bg, w - (opts.pad * 2), h - (opts.pad * 2));
   if (bg2) evas_object_resize(bg2, w, h);
}

const Ecore_Getopt optdesc = {
   "edje_player",
   "%prog [options] <filename.edj>",
   PACKAGE_VERSION,
   "(C) 2010 Enlightenment",
   "BSD with advertisement clause",
   "Simple application to view edje files.",
   0,
   {
      ECORE_GETOPT_STORE_STR
        ('g', "group", "The edje group to view (defaults to 'main')."),
      ECORE_GETOPT_STORE_TRUE
        ('G', "list-groups", "The groups in the given file."),
      ECORE_GETOPT_STORE_STR
        ('e', "engine", "The Ecore-Evas engine to use (see --list-engines)"),
      ECORE_GETOPT_CALLBACK_NOARGS
        ('E', "list-engines", "list Ecore-Evas engines",
        ecore_getopt_callback_ecore_evas_list_engines, NULL),
      ECORE_GETOPT_CALLBACK_ARGS
        ('Z', "size", "size to use in wxh form.", "WxH",
        ecore_getopt_callback_size_parse, NULL),
      ECORE_GETOPT_CALLBACK_ARGS
        ('c', "bg-color", "Color of the background (if not shaped or alpha) e.g. 255,150,50",
        "R,G,B", _parse_color, NULL),
      ECORE_GETOPT_STORE_TRUE
        ('b', "borderless", "Display window without border."),
      ECORE_GETOPT_STORE_TRUE
        ('y', "sticky", "Display window sticky."),
      ECORE_GETOPT_STORE_TRUE
        ('s', "shaped", "Display window shaped."),
      ECORE_GETOPT_STORE_TRUE
        ('a', "alpha", "Display window with alpha channel "
                       "(needs composite manager!)"),
      ECORE_GETOPT_STORE_STR
        ('t', "title", "Define the window title string"),
      ECORE_GETOPT_STORE_TRUE
        ('p', "print", "Print signals and messages to stdout"),
      ECORE_GETOPT_STORE_TRUE
        ('S', "slave-mode", "Listen for commands on stdin"),
      ECORE_GETOPT_STORE_DOUBLE
        ('z', "scale", "Set scale factor"),
      ECORE_GETOPT_STORE_INT
        ('P', "pad", "Set pixel padding around object"),
      ECORE_GETOPT_LICENSE('L', "license"),
      ECORE_GETOPT_COPYRIGHT('C', "copyright"),
      ECORE_GETOPT_VERSION('V', "version"),
      ECORE_GETOPT_HELP('h', "help"),
      ECORE_GETOPT_SENTINEL
   }
};

int
main(int argc, char **argv)
{
   Eina_Bool quit_option = EINA_FALSE;
   int args;
   Eina_List *groups;
   const char *group;
   Ecore_Getopt_Value values[] = {
      ECORE_GETOPT_VALUE_STR(opts.group),
      ECORE_GETOPT_VALUE_BOOL(opts.list_groups),
      ECORE_GETOPT_VALUE_STR(opts.engine),
      ECORE_GETOPT_VALUE_BOOL(quit_option),
      ECORE_GETOPT_VALUE_PTR_CAST(opts.size),
      ECORE_GETOPT_VALUE_PTR_CAST(opts.color),
      ECORE_GETOPT_VALUE_BOOL(opts.borderless),
      ECORE_GETOPT_VALUE_BOOL(opts.sticky),
      ECORE_GETOPT_VALUE_BOOL(opts.shaped),
      ECORE_GETOPT_VALUE_BOOL(opts.alpha),
      ECORE_GETOPT_VALUE_STR(opts.title),
      ECORE_GETOPT_VALUE_BOOL(opts.print),
      ECORE_GETOPT_VALUE_BOOL(opts.slave_mode),
      ECORE_GETOPT_VALUE_DOUBLE(opts.scale),
      ECORE_GETOPT_VALUE_INT(opts.pad),
      ECORE_GETOPT_VALUE_BOOL(quit_option),
      ECORE_GETOPT_VALUE_BOOL(quit_option),
      ECORE_GETOPT_VALUE_BOOL(quit_option),
      ECORE_GETOPT_VALUE_BOOL(quit_option),
      ECORE_GETOPT_VALUE_NONE
   };

   memset(&opts, 0, sizeof(opts));
   opts.scale = 1.0;

   if (!ecore_evas_init())
     return EXIT_FAILURE;
   if (!edje_init())
     goto shutdown_ecore_evas;
   edje_frametime_set(1.0 / 60.0);

   args = ecore_getopt_parse(&optdesc, values, argc, argv);
   if (args < 0)
     {
        fputs("Could not parse arguments.\n", stderr);
        goto shutdown_edje;
     }
   else if (quit_option)
     {
        goto end;
     }
   else if (args >= argc)
     {
        fputs("Missing edje file to load.\n", stderr);
        goto shutdown_edje;
     }

   ecore_app_args_set(argc, (const char **)argv);
   edje_scale_set(opts.scale);

   // check if the given edj file is there
   if (access(argv[args], R_OK) == -1)
     {
        int e = errno;
        fprintf(stderr, "ERROR: file '%s' not accessible, error %d (%s).\n",
                argv[args], e, strerror(e));
        goto end;
     }

   opts.file = argv[args];
   groups = edje_file_collection_list(opts.file);
   if (opts.list_groups)
     {
        Eina_List *n;
        printf("%d groups in file '%s':\n", eina_list_count(groups), opts.file);
        EINA_LIST_FOREACH(groups, n, group)
          printf("\t'%s'\n", group);
        edje_file_collection_list_free(groups);
        goto end;
     }

   if (opts.size.w <= 0) opts.size.w = 320;
   if (opts.size.h <= 0) opts.size.h = 240;
   win = ecore_evas_new(opts.engine, 0, 0, opts.size.w, opts.size.h, NULL);
   if (!win)
     {
        fprintf(stderr,
                "ERROR: could not create window of "
                "size %dx%d using engine %s.\n",
                opts.size.w, opts.size.h, opts.engine ? opts.engine : "(auto)");
        goto shutdown_edje;
     }

   ecore_evas_callback_delete_request_set(win, _cb_delete);
   evas = ecore_evas_get(win);

   if (opts.alpha)
     ecore_evas_alpha_set(win, EINA_TRUE);
   else if (opts.shaped)
     ecore_evas_shaped_set(win, EINA_TRUE);

   if ((opts.pad > 0) && (!opts.alpha))
     {
        bg2 = evas_object_rectangle_add(evas);
        evas_object_resize(bg2, opts.size.w, opts.size.h);
        if (opts.alpha)
          evas_object_color_set(bg2, 0, 0, 0, 64);
        else
          evas_object_color_set(bg2, 64, 64, 64, 255);
        evas_object_show(bg2);
     }
   bg = _create_bg(!opts.alpha);

   edje = _create_edje();
   if (!edje) goto free_ecore_evas;

   ecore_evas_callback_resize_set(win, _cb_resize);
   _cb_resize(win);
   evas_object_focus_set(bg, EINA_TRUE);
   evas_object_event_callback_add(bg, EVAS_CALLBACK_KEY_DOWN,
                                  _key_down, &opts);

   if (opts.print)
     {
        edje_object_signal_callback_add(edje, "*", "*", _print_signal, NULL);
        edje_object_message_handler_set(edje, _print_message, NULL);
     }

   if (opts.slave_mode)
     {
#ifndef _WIN32
        int flags;
        flags = fcntl(STDIN_FILENO, F_GETFL, 0);
        flags |= O_NONBLOCK;
        if (fcntl(STDIN_FILENO, F_SETFL, flags) < 0)
          {
             fprintf(stderr, "ERROR: Could not set stdin to non-block: %s\n",
                     strerror(errno));
             goto free_ecore_evas;
          }
        ecore_main_fd_handler_add(STDIN_FILENO, ECORE_FD_READ | ECORE_FD_ERROR,
                                  _slave_mode, edje, NULL, NULL);
#else
        /* TODO: port the code above to Windows */
        fprintf(stderr, "ERROR: slave mode not working on Windows\n");
        goto free_ecore_evas;
#endif
     }

   ecore_evas_borderless_set(win, opts.borderless);
   ecore_evas_sticky_set(win, opts.sticky);
   if (opts.title)
     ecore_evas_title_set(win, opts.title);

   ecore_evas_show(win);
   ecore_main_loop_begin();

   ecore_evas_free(win);
end:
   edje_shutdown();
   ecore_evas_shutdown();

   return 0;

free_ecore_evas:
   ecore_evas_free(win);
shutdown_edje:
   edje_shutdown();
shutdown_ecore_evas:
   ecore_evas_shutdown();
   return EXIT_FAILURE;
}

