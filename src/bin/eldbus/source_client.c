#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "codegen.h"

static const char *code_prefix = NULL;
static char buffer[4028];

/**
 * @brief Returns "NULL" or "0" based on the DBus type string.
 *
 * This function is used to provide a default null-like value for
 * C variable initialization based on its corresponding DBus type.
 * - For strings ('s'), object paths ('o'), variants ('v'), or any
 *   multi-character type (like "as" for array of strings), it returns "NULL".
 * - For other single-character basic types, it returns "0".
 *
 * @param type A C-string representing the DBus type (e.g., "s", "i", "as").
 * @return "NULL" or "0" as a C-string.
 */
static const char *
null_or_zero(const char *type)
{
   if (type[0] == 's' || type[0] == 'o' || type[0] == 'v' || type[1])
     return "NULL";
   return "0";
}

/**
 * @brief Appends a global prefix to the given text, if the prefix is set.
 *
 * Uses a static buffer `buffer` for the new string. This means
 * subsequent calls will overwrite the previous result.
 *
 * @param text The text to append the prefix to.
 * @return A pointer to the new string in the static buffer if `code_prefix` is set,
 *         otherwise returns the original `text`.
 */
static const char *
prefix_append(const char *text)
{
   if (code_prefix)
     {
        sprintf(buffer, "%s_%s", code_prefix, text);
        return buffer;
     }
   return text;
}

/**
 * @brief Converts a DBus type string to its corresponding C type string.
 *
 * This version allows specifying whether the C type should be `const`.
 *
 * @param dbus_type The DBus type string (e.g., "s", "i", "a{sv}").
 *                  Only the first character is typically used for basic types.
 * @param with_const If EINA_TRUE, pointer types (like char*) will be "const char *".
 * @return A C-string representing the C type. Returns NULL for unhandled types.
 *         Example: "s" with `with_const`=EINA_TRUE returns "const char *".
 *                  "i" returns "int ".
 *                  "a" (array), "v" (variant), "{" (dict), "(" (struct) return "Eldbus_Message_Iter *".
 */
static const char *
dbus_type2c_type2(const char *dbus_type, Eina_Bool with_const)
{
   switch (dbus_type[0])
     {
      case 's'://string
      case 'o'://object path
      case 'g'://signature
        {
           if (with_const)
             return "const char *";
           else
             return "char *";
        }
      case 'h'://file descriptor
      case 'i'://int
        return "int ";
      case 'y'://byte
        return "unsigned char ";
      case 'b'://bool
        return "Eina_Bool ";
      case 'n'://int16
        return "short int ";
      case 'q'://uint16
        return "unsigned short int ";
      case 'u'://uint32
        return "unsigned int ";
      case 'x'://int64
        return "int64_t ";
      case 't'://uint64
        return "uint64_t ";
      case 'd'://double
        return "double ";
      case 'a'://array
      case 'v'://variant
      case '{'://dict
      case '('://struct
        return "Eldbus_Message_Iter *";
      default:
        {
           printf("Error type not handled: %c\n", dbus_type[0]);
           return NULL;
        }
     }
}

static const char *
dbus_type2c_type(const char *dbus_type)
{
   return dbus_type2c_type2(dbus_type, EINA_TRUE);
}

/**
 * @brief Generates C code for calling a DBus method with complex input arguments.
 *
 * Complex input arguments are typically handled using Eina_Value.
 * This function generates both the C source code and the header declaration
 * for the method call.
 *
 * @param method Pointer to the DBus_Method structure describing the method.
 * @param c_code Eina_Strbuf to append the generated C source code.
 * @param h Eina_Strbuf to append the generated C header code.
 */
static void
source_client_complex_method_call_generate(const DBus_Method *method, Eina_Strbuf *c_code, Eina_Strbuf *h)
{
   DBus_Arg *arg;
   Eina_Strbuf *full_signature = eina_strbuf_new();

   EINA_INLIST_FOREACH(method->args, arg)
     {
        if (arg->direction == 'o')
          continue;
        eina_strbuf_append(full_signature, arg->type);
     }

   if (method->no_reply)
     {
        eina_strbuf_append_printf(h, "void %s_call(Eldbus_Proxy *proxy, Eina_Value *args);\n", prefix_append(method->c_name));

        eina_strbuf_append_printf(c_code, "\nvoid \n%s_call(Eldbus_Proxy *proxy, Eina_Value *args)\n{\n", prefix_append(method->c_name));
        eina_strbuf_append_printf(c_code, "   EINA_SAFETY_ON_NULL_RETURN(proxy);\n");
        eina_strbuf_append_printf(c_code, "   Eldbus_Message *msg = eldbus_proxy_method_call_new(proxy, \"%s\");\n", method->name);
        eina_strbuf_append_printf(c_code, "   if (!eldbus_message_from_eina_value(\"%s\", msg, args))\n", eina_strbuf_string_get(full_signature));
        eina_strbuf_append_printf(c_code, "     {\n");
        eina_strbuf_append_printf(c_code, "        ERR(\"Error: Filling message from eina value.\");\n");
        eina_strbuf_append_printf(c_code, "        eldbus_message_unref(msg);\n");
        eina_strbuf_append_printf(c_code, "        return;\n");
        eina_strbuf_append_printf(c_code, "     }\n");
        eina_strbuf_append_printf(c_code, "   eldbus_proxy_send(proxy, msg, %s, NULL, -1);\n", method->cb_name);
        eina_strbuf_append_printf(c_code, "}\n");
        goto end;
     }

   eina_strbuf_append_printf(h, "Eldbus_Pending *%s_call", prefix_append(method->c_name));
   eina_strbuf_append_printf(h, "(Eldbus_Proxy *proxy, %s cb, const void *data, Eina_Value *args);\n", prefix_append(method->function_cb));

   eina_strbuf_append_printf(c_code, "\nEldbus_Pending *\n%s_call(", prefix_append(method->c_name));
   eina_strbuf_append_printf(c_code, "Eldbus_Proxy *proxy, %s cb, const void *data, Eina_Value *args)\n{\n", prefix_append(method->function_cb));
   eina_strbuf_append_printf(c_code, "   Eldbus_Message *msg;\n");
   eina_strbuf_append_printf(c_code, "   Eldbus_Pending *p;\n");
   eina_strbuf_append_printf(c_code, "   EINA_SAFETY_ON_NULL_RETURN_VAL(proxy, NULL);\n");
   eina_strbuf_append_printf(c_code, "   msg = eldbus_proxy_method_call_new(proxy, \"%s\");\n", method->name);
   eina_strbuf_append_printf(c_code, "   if (!eldbus_message_from_eina_value(\"%s\", msg, args))\n", eina_strbuf_string_get(full_signature));
   eina_strbuf_append_printf(c_code, "     {\n");
   eina_strbuf_append_printf(c_code, "        ERR(\"Error: Filling message from eina value.\");\n");
   eina_strbuf_append_printf(c_code, "        eldbus_message_unref(msg);\n");
   eina_strbuf_append_printf(c_code, "        return NULL;\n");
   eina_strbuf_append_printf(c_code, "     }\n");
   eina_strbuf_append_printf(c_code, "   p = eldbus_proxy_send(proxy, msg, %s, cb, -1);\n", method->cb_name);
   eina_strbuf_append_printf(c_code, "   if (data)\n");
   eina_strbuf_append_printf(c_code, "     eldbus_pending_data_set(p, \"__user_data\", data);\n");
   eina_strbuf_append_printf(c_code, "   eldbus_pending_data_set(p, \"__user_proxy\", proxy);\n");
   eina_strbuf_append_printf(c_code, "   return p;\n");
   eina_strbuf_append_printf(c_code, "}\n");

end:
   eina_strbuf_free(full_signature);
}

/**
 * @brief Generates C code for calling a DBus method with simple input arguments
 *        and no expected reply.
 *
 * Simple input arguments are passed directly as function parameters.
 * This function generates both the C source code and the header declaration
 * for the method call.
 *
 * @param method Pointer to the DBus_Method structure describing the method.
 * @param c_code Eina_Strbuf to append the generated C source code.
 * @param h Eina_Strbuf to append the generated C header code.
 */
static void
source_client_simple_method_call_no_reply_generate(const DBus_Method *method, Eina_Strbuf *c_code, Eina_Strbuf *h)
{
   DBus_Arg *arg;
   Eina_Strbuf *full_signature = eina_strbuf_new();
   Eina_Strbuf *args_call = eina_strbuf_new();

   eina_strbuf_append_printf(h, "void %s_call(Eldbus_Proxy *proxy", prefix_append(method->c_name));
   eina_strbuf_append_printf(c_code, "\nvoid\n%s_call(Eldbus_Proxy *proxy", prefix_append(method->c_name));

   EINA_INLIST_FOREACH(method->args, arg)
     {
        if (arg->direction == 'o')
          continue;
        eina_strbuf_append(full_signature, arg->type);
        eina_strbuf_append_printf(h, ", %s%s", dbus_type2c_type(arg->type), arg->c_name);
        eina_strbuf_append_printf(c_code, ", %s%s", dbus_type2c_type(arg->type), arg->c_name);
        eina_strbuf_append_printf(args_call, ", %s", arg->c_name);
     }
   eina_strbuf_append_printf(h, ");\n");
   eina_strbuf_append_printf(c_code, ")\n{\n");

   eina_strbuf_append_printf(c_code, "   Eldbus_Message *msg;\n");
   eina_strbuf_append_printf(c_code, "   EINA_SAFETY_ON_NULL_RETURN(proxy);\n");
   eina_strbuf_append_printf(c_code, "   msg = eldbus_proxy_method_call_new(proxy, \"%s\");\n", method->name);
   eina_strbuf_append_printf(c_code, "   if (!eldbus_message_arguments_append(msg, \"%s\"%s))\n", eina_strbuf_string_get(full_signature), eina_strbuf_string_get(args_call));
   eina_strbuf_append_printf(c_code, "     {\n");
   eina_strbuf_append_printf(c_code, "        ERR(\"Error: Filling message.\");\n");
   eina_strbuf_append_printf(c_code, "        eldbus_message_unref(msg);\n");
   eina_strbuf_append_printf(c_code, "        return;\n");
   eina_strbuf_append_printf(c_code, "     }\n");
   eina_strbuf_append_printf(c_code, "   eldbus_proxy_send(proxy, msg, NULL, NULL, -1);\n");
   eina_strbuf_append_printf(c_code, "}\n");

   eina_strbuf_free(full_signature);
   eina_strbuf_free(args_call);
}

/**
 * @brief Generates C code for calling a DBus method with simple input arguments
 *        and an expected reply.
 *
 * Simple input arguments are passed directly as function parameters.
 * This function generates both the C source code and the header declaration
 * for the method call, which returns an Eldbus_Pending object.
 *
 * @param method Pointer to the DBus_Method structure describing the method.
 * @param c_code Eina_Strbuf to append the generated C source code.
 * @param h Eina_Strbuf to append the generated C header code.
 */
static void
source_client_simple_method_call_generate(const DBus_Method *method, Eina_Strbuf *c_code, Eina_Strbuf *h)
{
   DBus_Arg *arg;
   Eina_Strbuf *full_signature = eina_strbuf_new();
   Eina_Strbuf *args_call = eina_strbuf_new();

   eina_strbuf_append_printf(h, "Eldbus_Pending *%s_call", prefix_append(method->c_name));
   eina_strbuf_append_printf(h, "(Eldbus_Proxy *proxy, %s cb, const void *data", prefix_append(method->function_cb));
   eina_strbuf_append_printf(c_code, "\nEldbus_Pending *\n%s_call", prefix_append(method->c_name));
   eina_strbuf_append_printf(c_code, "(Eldbus_Proxy *proxy, %s cb, const void *data", prefix_append(method->function_cb));

   EINA_INLIST_FOREACH(method->args, arg)
     {
        if (arg->direction == 'o')
          continue;
        eina_strbuf_append(full_signature, arg->type);
        eina_strbuf_append_printf(h, ", %s%s", dbus_type2c_type(arg->type), arg->c_name);
        eina_strbuf_append_printf(c_code, ", %s%s", dbus_type2c_type(arg->type), arg->c_name);
        eina_strbuf_append_printf(args_call, ", %s", arg->c_name);
     }
   eina_strbuf_append_printf(h, ");\n");
   eina_strbuf_append_printf(c_code,")\n{\n");

   eina_strbuf_append_printf(c_code, "   Eldbus_Message *msg;\n");
   eina_strbuf_append_printf(c_code, "   Eldbus_Pending *p;\n");
   eina_strbuf_append_printf(c_code, "   EINA_SAFETY_ON_NULL_RETURN_VAL(proxy, NULL);\n");
   eina_strbuf_append_printf(c_code, "   msg = eldbus_proxy_method_call_new(proxy, \"%s\");\n", method->name);
   eina_strbuf_append_printf(c_code, "   if (!eldbus_message_arguments_append(msg, \"%s\"%s))\n", eina_strbuf_string_get(full_signature), eina_strbuf_string_get(args_call));
   eina_strbuf_append_printf(c_code, "     {\n");
   eina_strbuf_append_printf(c_code, "        ERR(\"Error: Filling message.\");\n");
   eina_strbuf_append_printf(c_code, "        eldbus_message_unref(msg);\n");
   eina_strbuf_append_printf(c_code, "        return NULL;\n");
   eina_strbuf_append_printf(c_code, "     }\n");
   eina_strbuf_append_printf(c_code, "   p = eldbus_proxy_send(proxy, msg, %s, cb, -1);\n", method->cb_name);
   eina_strbuf_append_printf(c_code, "   if (data)\n");
   eina_strbuf_append_printf(c_code, "     eldbus_pending_data_set(p, \"__user_data\", data);\n");
   eina_strbuf_append_printf(c_code, "   eldbus_pending_data_set(p, \"__user_proxy\", proxy);\n");
   eina_strbuf_append_printf(c_code, "   return p;\n");
   eina_strbuf_append_printf(c_code, "}\n");

   eina_strbuf_free(full_signature);
   eina_strbuf_free(args_call);
}

/**
 * @brief Generates C code for the callback function of a DBus method
 *        that returns complex output arguments.
 *
 * Complex output arguments are typically handled using Eina_Value.
 * This function generates the typedef for the user-provided callback and
 * the internal Eldbus callback that parses the message and calls the user's callback.
 *
 * @param method Pointer to the DBus_Method structure describing the method.
 * @param c_code Eina_Strbuf to append the generated C source code.
 * @param h Eina_Strbuf to append the generated C header code.
 */
static void
source_client_complex_method_callback_generate(const DBus_Method *method, Eina_Strbuf *c_code, Eina_Strbuf *h)
{
   eina_strbuf_append_printf(h, "typedef void (*%s)(Eldbus_Proxy *proxy, void *data, Eldbus_Pending *pending, Eldbus_Error_Info *error, Eina_Value *args);\n", prefix_append(method->function_cb));

   eina_strbuf_append_printf(c_code, "\nstatic void\n%s(void *data, const Eldbus_Message *msg, Eldbus_Pending *pending)\n{\n", method->cb_name);
   eina_strbuf_append_printf(c_code, "   void *user_data = eldbus_pending_data_del(pending, \"__user_data\");\n");
   eina_strbuf_append_printf(c_code, "   %s cb = data;\n", prefix_append(method->function_cb));
   eina_strbuf_append_printf(c_code, "   const char *error, *error_msg;\n");
   eina_strbuf_append_printf(c_code, "   Eina_Value *value;\n");
   eina_strbuf_append_printf(c_code, "   Eldbus_Proxy *proxy = eldbus_pending_data_del(pending, \"__user_proxy\");\n");
   eina_strbuf_append_printf(c_code, "   if (eldbus_message_error_get(msg, &error, &error_msg))\n");
   eina_strbuf_append_printf(c_code, "     {\n");
   eina_strbuf_append_printf(c_code, "        Eldbus_Error_Info error_info = {error, error_msg};\n");
   eina_strbuf_append_printf(c_code, "        cb(proxy, user_data, pending, &error_info, NULL);\n");
   eina_strbuf_append_printf(c_code, "        return;\n");
   eina_strbuf_append_printf(c_code, "     }\n");
   eina_strbuf_append_printf(c_code, "   value = eldbus_message_to_eina_value(msg);\n");
   eina_strbuf_append_printf(c_code, "   cb(proxy, user_data, pending, NULL, value);\n");
   eina_strbuf_append_printf(c_code, "   eina_value_free(value);\n");
   eina_strbuf_append_printf(c_code, "   return;\n");
   eina_strbuf_append_printf(c_code, "}\n");
}

/**
 * @brief Generates C code for the callback function of a DBus method
 *        that returns simple output arguments.
 *
 * Simple output arguments are passed as individual parameters to the user's callback.
 * This function generates the typedef for the user-provided callback and
 * the internal Eldbus callback that parses the message and calls the user's callback.
 *
 * @param method Pointer to the DBus_Method structure describing the method.
 * @param c_code Eina_Strbuf to append the generated C source code.
 * @param h Eina_Strbuf to append the generated C header code.
 */
static void
source_client_simple_method_callback_generate(const DBus_Method *method, Eina_Strbuf *c_code, Eina_Strbuf *h)
{
   Eina_Strbuf *full_signature = eina_strbuf_new();
   DBus_Arg *arg;
   Eina_Strbuf *end_cb = eina_strbuf_new();
   Eina_Strbuf *arguments_get = eina_strbuf_new();

   eina_strbuf_append_printf(h, "typedef void (*%s)(Eldbus_Proxy *proxy, void *data, Eldbus_Pending *pending, Eldbus_Error_Info *error", prefix_append(method->function_cb));

   eina_strbuf_append_printf(c_code, "\nstatic void\n%s(void *data, const Eldbus_Message *msg, Eldbus_Pending *pending)\n{\n", method->cb_name);
   eina_strbuf_append_printf(c_code, "   void *user_data = eldbus_pending_data_del(pending, \"__user_data\");\n");
   eina_strbuf_append_printf(c_code, "   %s cb = data;\n", prefix_append(method->function_cb));
   eina_strbuf_append_printf(c_code, "   const char *error, *error_msg;\n");
   eina_strbuf_append_printf(c_code, "   Eldbus_Proxy *proxy = eldbus_pending_data_del(pending, \"__user_proxy\");\n");

   EINA_INLIST_FOREACH(method->args, arg)
     {
        if (arg->direction != 'o')
          continue;
        eina_strbuf_append(full_signature, arg->type);
        eina_strbuf_append_printf(h, ", %s%s", dbus_type2c_type(arg->type), arg->c_name);
        eina_strbuf_append_printf(c_code, "   %s%s = %s;\n", dbus_type2c_type(arg->type), arg->c_name, null_or_zero(arg->type));
        eina_strbuf_append_printf(end_cb, ", %s", arg->c_name);
        eina_strbuf_append_printf(arguments_get, ", &%s", arg->c_name);
     }
   eina_strbuf_append_printf(h, ");\n");

   eina_strbuf_append_printf(c_code, "   if (eldbus_message_error_get(msg, &error, &error_msg))\n");
   eina_strbuf_append_printf(c_code, "     {\n");
   eina_strbuf_append_printf(c_code, "        Eldbus_Error_Info error_info = {error, error_msg};\n");
   eina_strbuf_append_printf(c_code, "        cb(proxy, user_data, pending, &error_info%s);\n", eina_strbuf_string_get(end_cb));
   eina_strbuf_append_printf(c_code, "        return;\n");
   eina_strbuf_append_printf(c_code, "     }\n");

   eina_strbuf_append_printf(c_code, "   if (!eldbus_message_arguments_get(msg, \"%s\"%s))\n", eina_strbuf_string_get(full_signature), eina_strbuf_string_get(arguments_get));
   eina_strbuf_append_printf(c_code, "     {\n");
   eina_strbuf_append_printf(c_code, "        Eldbus_Error_Info error_info = {\"\", \"\"};\n");
   eina_strbuf_append_printf(c_code, "        ERR(\"Error: Getting arguments from message.\");\n");
   eina_strbuf_append_printf(c_code, "        cb(proxy, user_data, pending, &error_info%s);\n", eina_strbuf_string_get(end_cb));
   eina_strbuf_append_printf(c_code, "        return;\n");
   eina_strbuf_append_printf(c_code, "     }\n");
   eina_strbuf_append_printf(c_code, "   cb(proxy, user_data, pending, NULL%s);\n", eina_strbuf_string_get(end_cb));
   eina_strbuf_append_printf(c_code, "   return;\n");
   eina_strbuf_append_printf(c_code, "}\n");

   eina_strbuf_free(full_signature);
   eina_strbuf_free(end_cb);
   eina_strbuf_free(arguments_get);
}

/**
 * @brief Generates C code for a DBus method, including its call function
 *        and callback function if applicable.
 *
 * This function dispatches to more specialized generation functions based on
 * whether the method expects a reply, and whether its input/output arguments
 * are simple or complex.
 *
 * @param method Pointer to the DBus_Method structure describing the method.
 * @param c_code Eina_Strbuf to append the generated C source code.
 * @param h Eina_Strbuf to append the generated C header code.
 */
static void
source_client_method_generate(const DBus_Method *method, Eina_Strbuf *c_code, Eina_Strbuf *h)
{
   if (!method->no_reply)
     {
        if (method->out_complex)
          source_client_complex_method_callback_generate(method, c_code, h);
        else
          source_client_simple_method_callback_generate(method, c_code, h);
     }

   if (method->in_complex)
     source_client_complex_method_call_generate(method, c_code, h);
   else
     {
        if (method->no_reply)
          source_client_simple_method_call_no_reply_generate(method, c_code, h);
        else
          source_client_simple_method_call_generate(method, c_code, h);
     }
}

/**
 * @brief Generates C code for handling a DBus signal.
 *
 * This includes:
 * - Adding a signal handler in the proxy initialization.
 * - Defining an Ecore_Event type for the signal.
 * - Defining a struct to hold signal data.
 * - Generating a free function for the signal data.
 * - Generating the callback function that receives the DBus signal,
 *   populates the struct, and emits the Ecore_Event.
 *
 * @param sig Pointer to the DBus_Signal structure describing the signal.
 * @param c_code Eina_Strbuf to append the generated C source code for callbacks and free functions.
 * @param h Eina_Strbuf to append the generated C header code (struct definition, event extern).
 * @param c_init_function Eina_Strbuf to append C code for the proxy initialization function (e.g., adding signal handlers).
 * @param c_header Eina_Strbuf to append C code for the C file's header section (e.g., event type static definition).
 */
static void
source_client_signal_generate(const DBus_Signal *sig, Eina_Strbuf *c_code, Eina_Strbuf * h, Eina_Strbuf *c_init_function, Eina_Strbuf *c_header)
{
   DBus_Arg *arg;
   Eina_Strbuf *full_signature = eina_strbuf_new();
   Eina_Strbuf *parameters = eina_strbuf_new();
   Eina_Strbuf *string_copy = eina_strbuf_new();
   Eina_Strbuf *string_free = eina_strbuf_new();

   eina_strbuf_append_printf(c_init_function, "   eldbus_proxy_signal_handler_add(proxy, \"%s\", %s, proxy);\n", sig->name, sig->cb_name);
   eina_strbuf_append_printf(c_header, "int %s = 0;\n", sig->signal_event);
   eina_strbuf_append_printf(h, "extern int %s;\n", sig->signal_event);
   eina_strbuf_append_printf(c_init_function, "   if (!%s)\n", sig->signal_event);
   eina_strbuf_append_printf(c_init_function, "     %s = ecore_event_type_new();\n", sig->signal_event);

   eina_strbuf_append_printf(h, "typedef struct _%s\n", sig->struct_name);
   eina_strbuf_append_printf(h, "{\n");
   eina_strbuf_append_printf(h, "   Eldbus_Proxy *proxy;\n");

   if (sig->complex)
     {
        eina_strbuf_append_printf(h, "   Eina_Value *value;\n");
        goto jump_simple_stuff;
     }

   EINA_INLIST_FOREACH(sig->args, arg)
     {
        eina_strbuf_append(full_signature, arg->type);
        eina_strbuf_append_printf(parameters, ", &s_data->%s", arg->c_name);
        eina_strbuf_append_printf(h, "   %s%s;\n", dbus_type2c_type2(arg->type, EINA_FALSE), arg->c_name);

        if (!strcmp(arg->type, "s") || !strcmp(arg->type, "o"))
          {
             eina_strbuf_append_printf(string_copy, "   s_data->%s = strdup(s_data->%s);\n", arg->c_name, arg->c_name);
             eina_strbuf_append_printf(string_free, "   free(s_data->%s);\n", arg->c_name);
          }
     }

jump_simple_stuff:
   eina_strbuf_append_printf(h, "} %s;\n", sig->struct_name);

   //free function
   eina_strbuf_append_printf(c_code, "\nstatic void\n%s(void *user_data EINA_UNUSED, void *func_data)\n{\n", sig->free_function);
   eina_strbuf_append_printf(c_code, "   %s *s_data = func_data;\n", sig->struct_name);
   if (sig->complex)
     eina_strbuf_append(c_code, "   eina_value_free(s_data->value);\n");
   else
     eina_strbuf_append(c_code, eina_strbuf_string_get(string_free));
   eina_strbuf_append_printf(c_code, "   free(s_data);\n");
   eina_strbuf_append_printf(c_code, "}\n");

   //cb function
   eina_strbuf_append_printf(c_code, "\nstatic void\n%s(void *data, const Eldbus_Message *msg)\n{\n", sig->cb_name);
   eina_strbuf_append_printf(c_code, "   Eldbus_Proxy *proxy = data;\n");
   eina_strbuf_append_printf(c_code, "   %s *s_data = calloc(1, sizeof(%s));\n", sig->struct_name, sig->struct_name);
   eina_strbuf_append_printf(c_code, "   s_data->proxy = proxy;\n");
   if (sig->complex)
     {
        eina_strbuf_append_printf(c_code, "   s_data->value = eldbus_message_to_eina_value(msg);\n");
        goto end_signal;
     }
   eina_strbuf_append_printf(c_code, "   if (!eldbus_message_arguments_get(msg, \"%s\"%s))\n", eina_strbuf_string_get(full_signature), eina_strbuf_string_get(parameters));
   eina_strbuf_append_printf(c_code, "     {\n");
   eina_strbuf_append_printf(c_code, "        ERR(\"Error: Getting arguments from message.\");\n");
   eina_strbuf_append_printf(c_code, "        free(s_data);\n");
   eina_strbuf_append_printf(c_code, "        return;\n");
   eina_strbuf_append_printf(c_code, "     }\n");
   eina_strbuf_append(c_code, eina_strbuf_string_get(string_copy));

end_signal:
   eina_strbuf_append_printf(c_code, "   ecore_event_add(%s, s_data, %s, NULL);\n", sig->signal_event, sig->free_function);
   eina_strbuf_append_printf(c_code, "}\n");

   eina_strbuf_free(full_signature);
   eina_strbuf_free(parameters);
   eina_strbuf_free(string_copy);
   eina_strbuf_free(string_free);
}

/**
 * @brief Determines the appropriate Eldbus codegen callback function name for a property get operation.
 *
 * The callback name depends on whether the property is complex or its basic type.
 * These callback types are expected to be defined in "eldbus_utils.h".
 *
 * @param prop Pointer to the DBus_Property structure.
 * @return A C-string representing the name of the callback function type.
 *         Example: For a complex property, returns "Eldbus_Codegen_Property_Complex_Get_Cb".
 *                  For a string property, returns "Eldbus_Codegen_Property_String_Get_Cb".
 */
static const char *
prop_cb_get(const DBus_Property *prop)
{
   if (prop->complex)
     return "Eldbus_Codegen_Property_Complex_Get_Cb";
   switch (prop->type[0])
     {
      case 's':
      case 'o':
        return "Eldbus_Codegen_Property_String_Get_Cb";
      case 'i':
      case 'h':
        return "Eldbus_Codegen_Property_Int32_Get_Cb";
      case 'y':
         return "Eldbus_Codegen_Property_Byte_Get_Cb";
      case 'b':
         return "Eldbus_Codegen_Property_Bool_Get_Cb";
      case 'n':
         return "Eldbus_Codegen_Property_Int16_Get_Cb";
      case 'q':
         return "Eldbus_Codegen_Property_Uint16_Get_Cb";
      case 'u':
         return "Eldbus_Codegen_Property_Uint32_Get_Cb";
      case 'd':
         return "Eldbus_Codegen_Property_Double_Get_Cb";
      case 'x':
         return "Eldbus_Codegen_Property_Int64_Get_Cb";
      case 't':
         return "Eldbus_Codegen_Property_Uint64_Get_Cb";
      default:
         return "Unexpected_type";
     }
}

/**
 * @brief Generates C code for getting a DBus property.
 *
 * This includes:
 * - The internal Eldbus callback function that handles the reply from `org.freedesktop.DBus.Properties.Get`.
 *   This callback parses the variant message, extracts the property value, and invokes the user-provided callback.
 * - The public `_propget` function that initiates the property get call.
 *
 * @param prop Pointer to the DBus_Property structure describing the property.
 * @param c_code Eina_Strbuf to append the generated C source code.
 * @param h Eina_Strbuf to append the generated C header code.
 */
static void
source_client_property_generate_get(const DBus_Property *prop, Eina_Strbuf *c_code, Eina_Strbuf *h)
{
   //callback
   eina_strbuf_append_printf(c_code, "\nstatic void\n%s(void *data, const Eldbus_Message *msg, Eldbus_Pending *pending)\n{\n", prop->cb_name);
   eina_strbuf_append_printf(c_code, "   void *user_data = eldbus_pending_data_del(pending, \"__user_data\");\n");
   eina_strbuf_append_printf(c_code, "   const char *error, *error_msg;\n");
   eina_strbuf_append_printf(c_code, "   %s cb = data;\n", prop_cb_get(prop));
   eina_strbuf_append_printf(c_code, "   Eldbus_Proxy *proxy = eldbus_pending_data_del(pending, \"__user_proxy\");\n");
   eina_strbuf_append_printf(c_code, "   Eldbus_Message_Iter *variant;\n");
   if (prop->complex)
     eina_strbuf_append_printf(c_code, "   Eina_Value *v, stack_value;\n");
   else
     eina_strbuf_append_printf(c_code, "   %sv;\n", dbus_type2c_type(prop->type));
   eina_strbuf_append_printf(c_code, "   if (eldbus_message_error_get(msg, &error, &error_msg))\n");
   eina_strbuf_append_printf(c_code, "     {\n");
   eina_strbuf_append_printf(c_code, "        Eldbus_Error_Info error_info = {error, error_msg};\n");
   eina_strbuf_append_printf(c_code, "        cb(user_data, pending, \"%s\", proxy, &error_info, %s);\n", prop->name, null_or_zero(prop->type));
   eina_strbuf_append_printf(c_code, "        return;\n");
   eina_strbuf_append_printf(c_code, "     }\n");

   eina_strbuf_append_printf(c_code, "   if (!eldbus_message_arguments_get(msg, \"v\", &variant))\n");
   eina_strbuf_append_printf(c_code, "     {\n");
   eina_strbuf_append_printf(c_code, "        Eldbus_Error_Info error_info = {\"\", \"\"};\n");
   eina_strbuf_append_printf(c_code, "        cb(user_data, pending, \"%s\", proxy, &error_info, %s);\n", prop->name, null_or_zero(prop->type));
   eina_strbuf_append_printf(c_code, "        return;\n");
   eina_strbuf_append_printf(c_code, "     }\n");

   if (prop->complex)
     {
        eina_strbuf_append_printf(c_code, "   v = eldbus_message_iter_struct_like_to_eina_value(variant);\n");
        eina_strbuf_append_printf(c_code, "   eina_value_struct_value_get(v, \"arg0\", &stack_value);\n");
        eina_strbuf_append_printf(c_code, "   cb(user_data, pending, \"%s\", proxy, NULL, &stack_value);\n", prop->name);
        eina_strbuf_append_printf(c_code, "   eina_value_flush(&stack_value);\n");
        eina_strbuf_append_printf(c_code, "   eina_value_free(v);\n");
     }
   else
     {
        eina_strbuf_append_printf(c_code, "   if (!eldbus_message_iter_arguments_get(variant, \"%s\", &v))\n", prop->type);
        eina_strbuf_append_printf(c_code, "     {\n");
        eina_strbuf_append_printf(c_code, "        Eldbus_Error_Info error_info = {\"\", \"\"};\n");
        eina_strbuf_append_printf(c_code, "        cb(user_data, pending, \"%s\", proxy, &error_info, %s);\n", prop->name, null_or_zero(prop->type));
        eina_strbuf_append_printf(c_code, "        return;\n");
        eina_strbuf_append_printf(c_code, "     }\n");
        eina_strbuf_append_printf(c_code, "   cb(user_data, pending, \"%s\", proxy, NULL, v);\n", prop->name);
     }
   eina_strbuf_append_printf(c_code, "}\n");

   //call
   eina_strbuf_append_printf(h, "Eldbus_Pending *%s_propget(Eldbus_Proxy *proxy, %s cb, const void *data);\n", prefix_append(prop->c_name), prop_cb_get(prop));

   eina_strbuf_append_printf(c_code, "\nEldbus_Pending *\n%s_propget(Eldbus_Proxy *proxy, %s cb, const void *data)\n{\n", prefix_append(prop->c_name), prop_cb_get(prop));
   eina_strbuf_append_printf(c_code, "   Eldbus_Pending *p;\n");
   eina_strbuf_append_printf(c_code, "   EINA_SAFETY_ON_NULL_RETURN_VAL(proxy, NULL);\n");
   eina_strbuf_append_printf(c_code, "   p = eldbus_proxy_property_get(proxy, \"%s\", %s, cb);\n", prop->name, prop->cb_name);
   eina_strbuf_append_printf(c_code, "   if (data)\n");
   eina_strbuf_append_printf(c_code, "     eldbus_pending_data_set(p, \"__user_data\", data);\n");
   eina_strbuf_append_printf(c_code, "   eldbus_pending_data_set(p, \"__user_proxy\", proxy);\n");
   eina_strbuf_append_printf(c_code, "   return p;\n");
   eina_strbuf_append_printf(c_code, "}\n");
}

/**
 * @brief Generates C code for setting a DBus property.
 *
 * This includes:
 * - The internal Eldbus callback function that handles the reply from `org.freedesktop.DBus.Properties.Set`.
 *   This callback invokes the user-provided callback, passing any error information.
 * - The public `_propset` function that initiates the property set call.
 *
 * @param prop Pointer to the DBus_Property structure describing the property.
 * @param c_code Eina_Strbuf to append the generated C source code.
 * @param h Eina_Strbuf to append the generated C header code.
 */
static void
source_client_property_generate_set(const DBus_Property *prop, Eina_Strbuf *c_code, Eina_Strbuf *h)
{
   //callback
   eina_strbuf_append_printf(c_code, "\nstatic void\n%s_set(void *data, const Eldbus_Message *msg, Eldbus_Pending *pending)\n{\n", prop->cb_name);
   eina_strbuf_append_printf(c_code, "   const char *error, *error_msg;\n");
   eina_strbuf_append_printf(c_code, "   void *user_data = eldbus_pending_data_del(pending, \"__user_data\");\n");
   eina_strbuf_append_printf(c_code, "   Eldbus_Proxy *proxy = eldbus_pending_data_del(pending, \"__user_proxy\");\n");
   eina_strbuf_append_printf(c_code, "   Eldbus_Codegen_Property_Set_Cb cb = data;\n");
   eina_strbuf_append_printf(c_code, "   if (eldbus_message_error_get(msg, &error, &error_msg))");
   eina_strbuf_append_printf(c_code, "     {\n");
   eina_strbuf_append_printf(c_code, "        Eldbus_Error_Info error_info = {error, error_msg};\n\n");
   eina_strbuf_append_printf(c_code, "        cb(user_data, \"%s\", proxy, pending, &error_info);\n", prop->name);
   eina_strbuf_append_printf(c_code, "        return;\n");
   eina_strbuf_append_printf(c_code, "     }\n");
   eina_strbuf_append_printf(c_code, "   cb(user_data, \"%s\", proxy, pending, NULL);\n", prop->name);
   eina_strbuf_append_printf(c_code, "}\n");

    //call
   eina_strbuf_append_printf(h, "Eldbus_Pending *%s_propset(Eldbus_Proxy *proxy, Eldbus_Codegen_Property_Set_Cb cb, const void *data, const void *value);\n", prefix_append(prop->c_name));

   eina_strbuf_append_printf(c_code, "\nEldbus_Pending *\n%s_propset(Eldbus_Proxy *proxy, Eldbus_Codegen_Property_Set_Cb cb, const void *data, const void *value)\n{\n", prop->c_name);
   eina_strbuf_append_printf(c_code, "   Eldbus_Pending *p;\n");
   eina_strbuf_append_printf(c_code, "   EINA_SAFETY_ON_NULL_RETURN_VAL(proxy, NULL);\n");
   eina_strbuf_append_printf(c_code, "   EINA_SAFETY_ON_NULL_RETURN_VAL(value, NULL);\n");
   eina_strbuf_append_printf(c_code, "   p = eldbus_proxy_property_set(proxy, \"%s\", \"%s\", value, %s_set, cb);\n", prop->name, prop->type, prop->cb_name);
   eina_strbuf_append_printf(c_code, "   eldbus_pending_data_set(p, \"__user_data\", data);\n");
   eina_strbuf_append_printf(c_code, "   eldbus_pending_data_set(p, \"__user_proxy\", proxy);\n");
   eina_strbuf_append_printf(c_code, "   return p;\n");
   eina_strbuf_append_printf(c_code, "}\n");
}

/**
 * @brief Generates C code for a DBus property, including its get and/or set functions.
 *
 * This function checks the access rights of the property (read, write) and calls
 * the appropriate generation functions (`source_client_property_generate_get`
 * and/or `source_client_property_generate_set`).
 *
 * @param prop Pointer to the DBus_Property structure describing the property.
 * @param c_code Eina_Strbuf to append the generated C source code.
 * @param h Eina_Strbuf to append the generated C header code.
 */
static void
source_client_property_generate(const DBus_Property *prop, Eina_Strbuf *c_code, Eina_Strbuf *h)
{
   if ((prop->access & ACCESS_READ) == ACCESS_READ)
     source_client_property_generate_get(prop, c_code, h);
   if ((prop->access & ACCESS_WRITE) == ACCESS_WRITE)
     source_client_property_generate_set(prop, c_code, h);
}

/**
 * @brief Main function to generate client-side C source and header files
 *        for DBus interfaces.
 *
 * Iterates through interfaces in the provided DBus_Object. For each matching
 * interface (or all if `interface_name` is NULL), it generates:
 * - A header file (`.h`) with function prototypes, typedefs, and struct definitions.
 * - A source file (`.c`) with implementations for method calls, signal handlers,
 *   and property accessors.
 *
 * @param path Pointer to the DBus_Object containing interface definitions.
 * @param prefix Optional prefix to be added to generated function and type names.
 * @param interface_name Optional specific interface name to generate code for.
 *                       If NULL, code is generated for all interfaces in `path`.
 * @param output_name Optional base name for the output files. If NULL, names are
 *                    derived from the interface name (e.g., `eldbus_IFACE_NAME.h`).
 */
void
source_client_generate(DBus_Object *path, const char *prefix, const char *interface_name, const char *output_name)
{
   DBus_Interface *iface;
   Eina_Bool found = EINA_FALSE;
   code_prefix = prefix;
   EINA_INLIST_FOREACH(path->ifaces, iface)
     {
        Eina_Strbuf *h, *c_init_function, *c_header, *c_code;
        DBus_Method *method;
        DBus_Signal *sig;
        DBus_Property *prop;
        char *file_name, *aux;
        int i;

        if (interface_name && strcmp(interface_name, iface->name))
          continue;
        found = EINA_TRUE;
        h = eina_strbuf_new();//.h file
        c_init_function = eina_strbuf_new();
        c_header = eina_strbuf_new();
        c_code = eina_strbuf_new();

        aux = string_build("ELDBUS_%s_H", iface->c_name);
        for (i = 0; aux[i]; i++)
          aux[i] = toupper(aux[i]);
        eina_strbuf_append_printf(h, "#ifndef %s\n", aux);
        eina_strbuf_append_printf(h, "#define %s\n\n", aux);
        free(aux);

        eina_strbuf_append_printf(h, "#include <Eina.h>\n");
        eina_strbuf_append_printf(h, "#include <Ecore.h>\n");
        eina_strbuf_append_printf(h, "#include <Eldbus.h>\n");
        eina_strbuf_append_printf(h, "#include \"eldbus_utils.h\"\n\n");
        eina_strbuf_append_printf(h, "Eldbus_Proxy *%s_proxy_get(Eldbus_Connection *conn, const char *bus, const char *path);\n", prefix_append(iface->c_name));
        eina_strbuf_append_printf(h, "void %s_proxy_unref(Eldbus_Proxy *proxy);\n", prefix_append(iface->c_name));
        eina_strbuf_append_printf(h, "void %s_log_domain_set(int id);\n", prefix_append(iface->c_name));

        if (interface_name && output_name)
          eina_strbuf_append_printf(c_header, "#include \"%s.h\"\n\n", output_name);
        else
          eina_strbuf_append_printf(c_header, "#include \"eldbus_%s.h\"\n\n", iface->c_name);

        eina_strbuf_append_printf(c_header, "static int _log_main = -1;\n");
        eina_strbuf_append_printf(c_header, "#undef ERR\n");
        eina_strbuf_append_printf(c_header, "#define ERR(...) EINA_LOG_DOM_ERR(_log_main, __VA_ARGS__);\n");

        eina_strbuf_append_printf(c_init_function, "void\n%s_log_domain_set(int id)\n{\n", prefix_append(iface->c_name));
        eina_strbuf_append_printf(c_init_function, "   _log_main = id;\n");
        eina_strbuf_append_printf(c_init_function, "}\n");

        eina_strbuf_append_printf(c_init_function, "\nvoid\n%s_proxy_unref(Eldbus_Proxy *proxy)\n{\n", prefix_append(iface->c_name));
        eina_strbuf_append_printf(c_init_function, "   Eldbus_Object *obj = eldbus_proxy_object_get(proxy);\n");
        eina_strbuf_append_printf(c_init_function, "   eldbus_proxy_unref(proxy);\n");
        eina_strbuf_append_printf(c_init_function, "   eldbus_object_unref(obj);\n");
        eina_strbuf_append_printf(c_init_function, "}\n");

        eina_strbuf_append_printf(c_init_function, "\nEldbus_Proxy *\n%s_proxy_get(Eldbus_Connection *conn, const char *bus, const char *path)\n{\n", prefix_append(iface->c_name));
        eina_strbuf_append_printf(c_init_function, "   Eldbus_Object *obj;\n");
        eina_strbuf_append_printf(c_init_function, "   Eldbus_Proxy *proxy;\n");
        eina_strbuf_append_printf(c_init_function, "   EINA_SAFETY_ON_NULL_RETURN_VAL(conn, NULL);\n");
        eina_strbuf_append_printf(c_init_function, "   EINA_SAFETY_ON_NULL_RETURN_VAL(bus, NULL);\n");
        eina_strbuf_append_printf(c_init_function, "   if (!path) path = \"%s\";\n", path->name);
        eina_strbuf_append_printf(c_init_function, "   obj = eldbus_object_get(conn, bus, path);\n");
        eina_strbuf_append_printf(c_init_function, "   proxy = eldbus_proxy_get(obj, \"%s\");\n", iface->name);

        EINA_INLIST_FOREACH(iface->methods, method)
          source_client_method_generate(method, c_code, h);

        EINA_INLIST_FOREACH(iface->signals, sig)
          source_client_signal_generate(sig, c_code, h, c_init_function, c_header);

        EINA_INLIST_FOREACH(iface->properties, prop)
          source_client_property_generate(prop, c_code, h);

        eina_strbuf_append_printf(c_init_function, "   return proxy;\n");
        eina_strbuf_append_printf(c_init_function, "}\n");

        eina_strbuf_append(h, "\n#endif");

        if (interface_name && output_name)
          file_name = string_build("%s.h", output_name);
        else
          file_name = string_build("eldbus_%s.h", iface->c_name);
        file_write(file_name, eina_strbuf_string_get(h));
        eina_strbuf_free(h);
        free(file_name);

        eina_strbuf_append(c_header, eina_strbuf_string_get(c_code));
        eina_strbuf_free(c_code);
        eina_strbuf_append(c_header, "\n");
        eina_strbuf_append(c_header, eina_strbuf_string_get(c_init_function));
        eina_strbuf_free(c_init_function);
        if (interface_name && output_name)
          file_name = string_build("%s.c", output_name);
        else
          file_name = string_build("eldbus_%s.c", iface->c_name);
        file_write(file_name, eina_strbuf_string_get(c_header));
        eina_strbuf_free(c_header);
        free(file_name);
     }

   if (interface_name && !found)
     printf("Error: Interface %s not found.\n", interface_name);
}
