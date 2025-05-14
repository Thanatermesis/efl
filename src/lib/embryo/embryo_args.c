/**
 * @file
 * @brief Functions for handling arguments passed to Embryo scripts.
 *
 * This file implements the native Embryo functions that allow scripts
 * to access arguments passed to them, similar to argc/argv in C.
 * It includes functions to get the number of arguments, retrieve
 * arguments by index (both numeric and string types), and set arguments.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Eina.h>

#include "Embryo.h"
#include "embryo_private.h"

#define STRSET(ep, par, str) {                           \
     Embryo_Cell *___cptr;                               \
     if ((___cptr = embryo_data_address_get(ep, par))) { \
          embryo_data_string_set(ep, str, ___cptr);      \
       } }

/* exported args api */

/**
 * @brief Retrieves the number of arguments passed to the script.
 * @param ep The Embryo program instance.
 * @param params Unused in this function.
 * @return The number of arguments.
 *
 * This function corresponds to the `numargs()` native call in an Embryo script.
 * It reads the argument count directly from the Embryo program's data section.
 */
static Embryo_Cell
_embryo_args_numargs(Embryo_Program *ep, Embryo_Cell *params EINA_UNUSED)
{
   Embryo_Header *hdr;
   unsigned char *data;
   Embryo_Cell bytes;

   hdr = (Embryo_Header *)ep->base;
   data = ep->base + (int)hdr->dat;
   bytes = *(Embryo_Cell *)(data + (int)ep->frm +
                            (2 * sizeof(Embryo_Cell)));
   return bytes / sizeof(Embryo_Cell);
}

/**
 * @brief Retrieves a specific argument by its index.
 * @param ep The Embryo program instance.
 * @param params An array of Embryo_Cell:
 *               - params[0]: Expected size of parameters (2 * sizeof(Embryo_Cell)).
 *               - params[1]: The index of the argument to retrieve.
 *               - params[2]: An offset to be added to the argument's address (usually 0).
 * @return The value of the requested argument as an Embryo_Cell. Returns 0 on error.
 *
 * This function corresponds to the `getarg()` or `getfarg()` native call in an Embryo script.
 * It fetches an argument from the script's stack frame based on its index.
 * The `params[2]` is typically used for array access within an argument, but here it seems
 * to be part of a more general mechanism or a potential for future extension.
 *
 * Example (Embryo script):
 *   new arg = getarg(0); // Get the first argument
 */
static Embryo_Cell
_embryo_args_getarg(Embryo_Program *ep, Embryo_Cell *params)
{
   Embryo_Header *hdr;
   unsigned char *data;
   Embryo_Cell val;

   if (params[0] != (2 * sizeof(Embryo_Cell))) return 0;
   hdr = (Embryo_Header *)ep->base;
   data = ep->base + (int)hdr->dat;
   val = *(Embryo_Cell *)(data + (int)ep->frm +
                          (((int)params[1] + 3) * sizeof(Embryo_Cell)));
   val += params[2] * sizeof(Embryo_Cell);
   val = *(Embryo_Cell *)(data + (int)val);
   return val;
}

/**
 * @brief Sets the value of a specific argument by its index.
 * @param ep The Embryo program instance.
 * @param params An array of Embryo_Cell:
 *               - params[0]: Expected size of parameters (3 * sizeof(Embryo_Cell)).
 *               - params[1]: The index of the argument to set.
 *               - params[2]: An offset to be added to the argument's address (usually 0).
 *               - params[3]: The new value for the argument.
 * @return 1 on success, 0 on failure (e.g., invalid index, out of bounds).
 *
 * This function corresponds to the `setarg()` or `setfarg()` native call in an Embryo script.
 * It modifies an argument on the script's stack frame.
 *
 * Example (Embryo script):
 *   setarg(0, 123); // Set the first argument to 123
 */
static Embryo_Cell
_embryo_args_setarg(Embryo_Program *ep, Embryo_Cell *params)
{
   Embryo_Header *hdr;
   unsigned char *data;
   Embryo_Cell val;

   if (params[0] != (3 * sizeof(Embryo_Cell))) return 0;
   hdr = (Embryo_Header *)ep->base;
   data = ep->base + (int)hdr->dat;
   val = *(Embryo_Cell *)(data + (int)ep->frm +
                          (((int)params[1] + 3) * sizeof(Embryo_Cell)));
   val += params[2] * sizeof(Embryo_Cell);
   if ((val < 0) || ((val >= ep->hea) && (val < ep->stk))) return 0;
   *(Embryo_Cell *)(data + (int)val) = params[3];
   return 1;
}

/**
 * @brief Retrieves a specific string argument by its index.
 * @param ep The Embryo program instance.
 * @param params An array of Embryo_Cell:
 *               - params[0]: Expected size of parameters (3 * sizeof(Embryo_Cell)).
 *               - params[1]: The index of the string argument to retrieve.
 *               - params[2]: The Embryo_Cell address (parameter in script) where the string will be stored.
 *               - params[3]: The maximum length of the buffer (in Embryo_Cell units, i.e., characters) to store the string.
 * @return The number of characters written to the buffer (excluding the null terminator),
 *         or 0 on error (e.g., invalid index, buflen <= 0).
 *
 * This function corresponds to the `getsarg()` native call in an Embryo script.
 * It reads a sequence of characters (Embryo_Cell values) from the script's data
 * section, treating them as a string, and copies it into a buffer provided by the script.
 * The string is null-terminated.
 *
 * The string in Embryo's memory is stored as a sequence of Embryo_Cell,
 * where each cell holds a character.
 *
 * Example (Embryo script):
 *   new str_buf[32];
 *   new len = getsarg(1, str_buf, sizeof(str_buf)); // Get the second argument as a string
 *   // str_buf now contains the string, len is its length
 */
static Embryo_Cell
_embryo_args_getsarg(Embryo_Program *ep, Embryo_Cell *params)
{
   Embryo_Header *hdr;
   unsigned char *data;
   Embryo_Cell base_cell;
   char *s;
   int i = 0;

   /* params[1] = arg_no */
   /* params[2] = buf */
   /* params[3] = buflen */
   if (params[0] != (3 * sizeof(Embryo_Cell))) return 0;
   if (params[3] <= 0) return 0;  /* buflen must be > 0 */
   hdr = (Embryo_Header *)ep->base;
   data = ep->base + (int)hdr->dat;
   base_cell = *(Embryo_Cell *)(data + (int)ep->frm +
                                (((int)params[1] + 3) * sizeof(Embryo_Cell)));

   s = alloca(params[3]);

   while (i < params[3])
     {
        int offset = base_cell + (i * sizeof(Embryo_Cell));

        s[i] = *(Embryo_Cell *)(data + offset);
        if (!s[i++]) break;
     }

   s[i - 1] = 0;
   STRSET(ep, params[2], s);

   return i - 1; /* characters written minus terminator */
}

/* functions used by the rest of embryo */

/**
 * @brief Initializes the argument handling native calls for an Embryo program.
 * @param ep The Embryo program instance to register the native calls with.
 *
 * This function is called during the setup of an Embryo program to make
 * the argument-related functions available to Embryo scripts. It registers
 * "numargs", "getarg", "setarg", "getfarg", "setfarg", and "getsarg"
 * as native calls.
 */
void
_embryo_args_init(Embryo_Program *ep)
{
   embryo_program_native_call_add(ep, "numargs", _embryo_args_numargs);
   embryo_program_native_call_add(ep, "getarg", _embryo_args_getarg);
   embryo_program_native_call_add(ep, "setarg", _embryo_args_setarg);
   embryo_program_native_call_add(ep, "getfarg", _embryo_args_getarg);
   embryo_program_native_call_add(ep, "setfarg", _embryo_args_setarg);
   embryo_program_native_call_add(ep, "getsarg", _embryo_args_getsarg);
}

